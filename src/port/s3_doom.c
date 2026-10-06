#define S3_NATIVE_DRIVER 1
#include "s3_doom.h"
#include "src/os/ui.h"
#include "doomkeys.h"
#include "esp_partition.h"
#include "esp_vfs.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/idf_additions.h"
#include "freertos/semphr.h"
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
extern bool doom_start(const char *, void (*)(const uint32_t *));
extern bool s3_doom_init_tables(void);
extern void doom_tick(void);
extern void doom_key(int, unsigned char);
extern bool doom_dead(void);
extern const char * doom_last_error(void);
extern void lv_image_cache_drop(const void * src);
static const uint8_t * wad;
static size_t wad_size;
static off_t file_pos[8];
static bool file_open[8];
static TaskHandle_t worker;
static SemaphoreHandle_t mutex;
static uint16_t * frame;
static _Atomic bool started, failed, new_frame;
static lv_obj_t * image, * message;
static lv_draw_buf_t * canvas;
static lv_timer_t * timer;
static int file_stat(const char * path, struct stat * st)
{
    if(strcmp(path, "/doom1.wad")) { errno = ENOENT; return -1; }
    memset(st, 0, sizeof(*st)); st->st_mode = S_IFREG | 0444; st->st_size = wad_size; return 0;
}
static int file_create(const char * path, int flags, int mode)
{
    (void)flags; (void)mode;
    struct stat st;
    if(file_stat(path, &st)) return -1;
    for(int i = 0; i < 8; i++) if(!file_open[i]) { file_open[i] = true; file_pos[i] = 0; return i; }
    errno = EMFILE; return -1;
}
static ssize_t file_read(int fd, void * out, size_t n)
{
    if(fd < 0 || fd >= 8 || !file_open[fd]) { errno = EBADF; return -1; }
    n = LV_MIN(n, wad_size - file_pos[fd]); memcpy(out, wad + file_pos[fd], n); file_pos[fd] += n; return n;
}
static off_t file_seek(int fd, off_t offset, int whence)
{
    if(fd < 0 || fd >= 8 || !file_open[fd]) { errno = EBADF; return -1; }
    off_t pos = offset + (whence == SEEK_CUR ? file_pos[fd] : whence == SEEK_END ? (off_t)wad_size : 0);
    if(pos < 0 || pos > (off_t)wad_size) { errno = EINVAL; return -1; }
    return file_pos[fd] = pos;
}
static int file_close(int fd) { if(fd < 0 || fd >= 8) return -1; file_open[fd] = false; return 0; }
static int file_fstat(int fd, struct stat * st) { (void)fd; return file_stat("/doom1.wad", st); }
static bool mount_wad(void)
{
    const esp_partition_t * part = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, "doom_wad");
    if(!part) return false;
    esp_partition_mmap_handle_t handle;
    if(esp_partition_mmap(part, 0, part->size, ESP_PARTITION_MMAP_DATA, (const void **)&wad, &handle) != ESP_OK) return false;
    if(memcmp(wad, "IWAD", 4)) { esp_partition_munmap(handle); wad = NULL; return false; }
    uint32_t count, directory; memcpy(&count, wad + 4, 4); memcpy(&directory, wad + 8, 4);
    if(directory > part->size || count > (part->size - directory)/16) return false;
    wad_size = directory + count * 16;
    esp_vfs_t vfs = { .flags = ESP_VFS_FLAG_DEFAULT, .open = file_create, .read = file_read,
                     .lseek = file_seek, .close = file_close, .fstat = file_fstat, .stat = file_stat };
    return esp_vfs_register("/doom", &vfs, NULL) == ESP_OK;
}
static void present(const uint32_t * pixels)
{
    xSemaphoreTake(mutex, portMAX_DELAY);
    for(int y = 0; y < 80; y++) for(int x = 0; x < 128; x++) {
        uint32_t c = pixels[(y * 200 / 80) * 320 + x * 320 / 128];
        frame[y * 128 + x] = ((c >> 8) & 0xf800) | ((c >> 5) & 0x07e0) | ((c >> 3) & 0x001f);
    }
    new_frame = true;
    xSemaphoreGive(mutex);
}
static void run(void * arg)
{
    (void)arg;
    if(!s3_doom_init_tables() || !doom_start("/doom/doom1.wad", present)) failed = true;
    else { started = true; ESP_LOGI("doom_ui", "Doom engine started from flash WAD"); }
    while(true) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        if(started && !failed) { doom_tick(); if(doom_dead()) failed = true; }
    }
}
static void refresh(lv_timer_t * t)
{
    (void)t;
    if(failed) { lv_label_set_text_fmt(message, "Doom: %s", doom_last_error()[0] ? doom_last_error() : "Flash DOOM1.WAD to the doom_wad partition."); lv_obj_remove_flag(message, LV_OBJ_FLAG_HIDDEN); return; }
    if(started) lv_obj_add_flag(message, LV_OBJ_FLAG_HIDDEN);
    if(new_frame && xSemaphoreTake(mutex, 0) == pdTRUE) {
        memcpy(canvas->data, frame, 128 * 80 * sizeof(*frame)); new_frame = false; xSemaphoreGive(mutex);
        lv_obj_invalidate(image);
    }
    if(worker) xTaskNotifyGive(worker);
}
static void deleted(lv_event_t * e)
{
    (void)e;
    const unsigned char keys[] = { KEY_UPARROW, KEY_DOWNARROW, KEY_LEFTARROW, KEY_RIGHTARROW, KEY_FIRE, KEY_USE, KEY_ENTER, KEY_ESCAPE };
    for(unsigned i = 0; i < sizeof(keys); i++) doom_key(0, keys[i]);
    lv_timer_delete(timer); timer = NULL; image = message = NULL;
    lv_image_cache_drop(canvas); lv_draw_buf_destroy(canvas); canvas = NULL;
}
static void key(lv_event_t * e)
{
    int pressed = lv_event_get_code(e) == LV_EVENT_PRESSED;
    if(pressed || lv_event_get_code(e) == LV_EVENT_RELEASED || lv_event_get_code(e) == LV_EVENT_PRESS_LOST)
        doom_key(pressed, (unsigned char)(uintptr_t)lv_event_get_user_data(e));
}
void s3_doom_command(const char * name, int pressed)
{
    static const char * names[] = {"up", "down", "left", "right", "fire", "use", "enter", "escape"};
    static const unsigned char keys[] = {KEY_UPARROW, KEY_DOWNARROW, KEY_LEFTARROW, KEY_RIGHTARROW, KEY_FIRE, KEY_USE, KEY_ENTER, KEY_ESCAPE};
    for(int i = 0; i < 8; i++) if(!strcmp(name, names[i])) { doom_key(pressed, keys[i]); return; }
}
void s3_doom_create(lv_obj_t * root)
{
    canvas = lv_draw_buf_create(128, 80, LV_COLOR_FORMAT_RGB565, LV_STRIDE_AUTO);
    image = lv_image_create(root); lv_image_set_src(image, canvas); lv_obj_set_pos(image, 0, 24);
    message = lv_label_create(root); lv_obj_set_width(message, 120); lv_obj_set_pos(message, 4, 108); lv_label_set_text(message, "Starting Doom…");
    static const char * labels[] = {"<", "^", ">", "Fire", "Use", "Enter"};
    static const unsigned char keys[] = {KEY_LEFTARROW, KEY_UPARROW, KEY_RIGHTARROW, KEY_FIRE, KEY_USE, KEY_ENTER};
    for(int i = 0; i < 6; i++) {
        lv_obj_t * button = ui_button(root, labels[i], 0x303030, NULL, NULL);
        lv_obj_set_size(button, 39, 15); lv_obj_set_pos(button, 3 + (i % 3) * 41, 116 + (i / 3) * 17);
        lv_obj_add_event_cb(button, key, LV_EVENT_ALL, (void *)(uintptr_t)keys[i]);
    }
    if(!worker) {
        mutex = xSemaphoreCreateMutex();
        frame = heap_caps_malloc(128 * 80 * sizeof(*frame), MALLOC_CAP_SPIRAM);
        /* Mapping changes cache/MMU state and must run on the UI's internal stack. */
        if(!mutex || !frame || !mount_wad() || xTaskCreatePinnedToCoreWithCaps(run, "doom", 16384, NULL, 2, &worker, 0,
                                                           MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT) != pdPASS) failed = true;
    }
    timer = lv_timer_create(refresh, 33, NULL);
    lv_obj_add_event_cb(root, deleted, LV_EVENT_DELETE, NULL);
}
