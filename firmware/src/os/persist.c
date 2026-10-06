/**
 * @file persist.c
 * Settings, home layout and placed widgets saved to a small text file in the working directory.
 */
#include <stdio.h>
#include <string.h>
#include "sdk.h"

#define CFG_FILE    "espringboard.cfg"
#define CFG_MAGIC   "espringboard-v3"
#define CFG_NUMS    17

os_settings_t os_settings = {
    .wifi = true, .bluetooth = true, .brightness = 100, .volume = 70, .cellular = true,
    .quick_switch = true, .previews = true, .lock_wall = 1, .edge_back = true, .edge_size = 22,
    .lock_widget = { 0, 1, -1, -1 }, .lock_widget_span = { 1, 1, 1, 1 },
};

#ifdef ESP_PLATFORM
#include "nvs.h"
typedef struct {
 uint32_t version;
 os_settings_t settings;
 int8_t layout[HOME_LOC_COUNT];
 home_widget_t widgets[HOME_WIDGETS];
 int8_t members[OS_MAX_APPS][HOME_FOLDER_ITEMS];
 uint8_t counts[OS_MAX_APPS];
 int32_t bezel, strength; bool chroma;
} saved_shell_t;
void os_load(void)
{
 nvs_handle_t h; saved_shell_t saved; size_t size = sizeof(saved);
 if(nvs_open("spring_og", NVS_READONLY, &h) != ESP_OK) return;
 esp_err_t result = nvs_get_blob(h, "layout_v1", &saved, &size); nvs_close(h);
 if(result != ESP_OK || size != sizeof(saved) || saved.version != 1) return;
 os_settings = saved.settings;
 home_widgets_import(saved.widgets); home_layout_import(saved.layout);
 home_folders_import(saved.members, saved.counts);
 ui_lens_settings_set(saved.bezel, saved.strength, saved.chroma);
}
void os_save(void)
{
 nvs_handle_t h; saved_shell_t saved = { .version = 1, .settings = os_settings };
 home_layout_export(saved.layout); home_widgets_export(saved.widgets);
 home_folders_export(saved.members, saved.counts);
 ui_lens_settings_get(&saved.bezel, &saved.strength, &saved.chroma);
 if(nvs_open("spring_og", NVS_READWRITE, &h) != ESP_OK) return;
 if(nvs_set_blob(h, "layout_v1", &saved, sizeof(saved)) == ESP_OK) nvs_commit(h);
 nvs_close(h);
}
#else
void os_load(void)
{
    FILE * f = fopen(CFG_FILE, "r");
    if(!f) return;

    char magic[32] = { 0 };
    int v[CFG_NUMS];
    if(fscanf(f, "%31s", magic) != 1 || strcmp(magic, CFG_MAGIC) != 0) {
        fclose(f);
        return;
    }
    for(int i = 0; i < CFG_NUMS; i++) {
        if(fscanf(f, "%d", &v[i]) != 1) {
            fclose(f);
            return;
        }
    }
    os_settings_t * s = &os_settings;
    s->wifi = v[0] != 0;
    s->bluetooth = v[1] != 0;
    s->airplane = v[2] != 0;
    s->brightness = (uint8_t)LV_CLAMP(10, v[3], 100);
    s->wallpaper = (uint8_t)(v[4] == OS_WALL_CUSTOM ? OS_WALL_CUSTOM : LV_CLAMP(0, v[4], OS_WALLPAPER_COUNT - 1));
    s->volume = (uint8_t)LV_CLAMP(0, v[5], 100);
    s->dnd = v[6] != 0;
    s->cellular = v[7] != 0;
    s->pill = v[8] != 0;
    s->quick_switch = v[9] != 0;
    s->previews = v[10] != 0;
    s->lock_style = (uint8_t)LV_CLAMP(0, v[11], OS_LOCK_STYLES - 1);
    s->lock_color = (uint8_t)LV_CLAMP(0, v[12], OS_LOCK_COLOR_AUTO);
    s->lock_wall = (uint8_t)(v[13] == OS_WALL_CUSTOM ? OS_WALL_CUSTOM : LV_CLAMP(0, v[13], OS_WALLPAPER_COUNT - 1));
    s->about_style = (uint8_t)LV_CLAMP(0, v[14], 2);
    if(v[16] > 0) {         /* 0 = file written before these existed: keep the defaults */
        s->edge_back = v[15] != 0;
        s->edge_size = (uint8_t)LV_CLAMP(8, v[16], 64);
    }

    int8_t layout[HOME_LOC_COUNT];
    int i;
    for(i = 0; i < HOME_LOC_COUNT; i++) {
        int x;
        if(fscanf(f, "%d", &x) != 1) break;
        layout[i] = (int8_t)x;
    }
    home_widget_t w[HOME_WIDGETS];
    int k;
    for(k = 0; k < HOME_WIDGETS; k++) {
        int a, b, c;
        if(fscanf(f, "%d %d %d", &a, &b, &c) != 3) break;
        w[k].widget = (int8_t)a;
        w[k].page = (int8_t)b;
        w[k].cell = (int8_t)(c & 15);
        w[k].size = (int8_t)LV_CLAMP(0, c >> 4, 3);
    }
    /* Widgets first: they decide which grid cells the icons may use */
    if(k == HOME_WIDGETS) home_widgets_import(w);
    if(i == HOME_LOC_COUNT) home_layout_import(layout);
    int lw0, lw1;
    if(fscanf(f, "%d %d", &lw0, &lw1) == 2) {
        s->lock_widget[0] = (int8_t)LV_CLAMP(-1, lw0, os_widget_count - 1);
        s->lock_widget[1] = (int8_t)LV_CLAMP(-1, lw1, os_widget_count - 1);
    }
    int folder_n;
    if(fscanf(f, "%d", &folder_n) == 1 && folder_n >= 0 && folder_n <= OS_MAX_APPS) {
        int8_t members[OS_MAX_APPS][HOME_FOLDER_ITEMS] = { { 0 } };
        uint8_t counts[OS_MAX_APPS] = { 0 };
        bool valid = true;
        for(int fidx = 0; fidx < folder_n && valid; fidx++) {
            int owner, count;
            if(fscanf(f, "%d %d", &owner, &count) != 2 || owner < 0 || owner >= os_app_count ||
               count < 2 || count > HOME_FOLDER_ITEMS || counts[owner]) { valid = false; break; }
            counts[owner] = (uint8_t)count;
            for(int j = 0; j < count; j++) {
                int id;
                if(fscanf(f, "%d", &id) != 1) { valid = false; break; }
                members[owner][j] = (int8_t)id;
            }
        }
        if(valid) home_folders_import(members, counts);
    }
    int bezel, strength, chroma;
    if(fscanf(f, "%d %d %d", &bezel, &strength, &chroma) == 3)
        ui_lens_settings_set(bezel, strength, chroma != 0);
    int low_power;
    if(fscanf(f, "%d", &low_power) == 1) s->low_power = low_power != 0;
    int extra[6];
    if(fscanf(f, "%d %d %d %d %d %d", &extra[0], &extra[1], &extra[2], &extra[3], &extra[4], &extra[5]) == 6) {
        s->lock_widget[2] = (int8_t)LV_CLAMP(-1, extra[0], os_widget_count - 1);
        s->lock_widget[3] = (int8_t)LV_CLAMP(-1, extra[1], os_widget_count - 1);
        for(int i = 0; i < 4; i++) s->lock_widget_span[i] = (uint8_t)LV_CLAMP(1, extra[i + 2], 2);
    }
    fclose(f);
}

void os_save(void)
{
    FILE * f = fopen(CFG_FILE, "w");
    if(!f) return;
    const os_settings_t * s = &os_settings;
    fprintf(f, "%s %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d\n", CFG_MAGIC, s->wifi, s->bluetooth,
            s->airplane, s->brightness, s->wallpaper, s->volume, s->dnd, s->cellular, s->pill, s->quick_switch,
            s->previews, s->lock_style, s->lock_color, s->lock_wall, s->about_style, s->edge_back, s->edge_size);

    int8_t layout[HOME_LOC_COUNT];
    home_layout_export(layout);
    for(int i = 0; i < HOME_LOC_COUNT; i++) fprintf(f, "%d%c", layout[i], i == HOME_LOC_COUNT - 1 ? '\n' : ' ');
    home_widget_t w[HOME_WIDGETS];
    home_widgets_export(w);
    for(int k = 0; k < HOME_WIDGETS; k++) fprintf(f, "%d %d %d\n", w[k].widget, w[k].page, w[k].cell | (w[k].size << 4));
    fprintf(f, "%d %d\n", s->lock_widget[0], s->lock_widget[1]);
    int8_t members[OS_MAX_APPS][HOME_FOLDER_ITEMS];
    uint8_t counts[OS_MAX_APPS];
    home_folders_export(members, counts);
    int folder_n = 0;
    for(int owner = 0; owner < os_app_count; owner++) if(counts[owner] >= 2) folder_n++;
    fprintf(f, "%d\n", folder_n);
    for(int owner = 0; owner < os_app_count; owner++) {
        if(counts[owner] < 2) continue;
        fprintf(f, "%d %d", owner, counts[owner]);
        for(int j = 0; j < counts[owner]; j++) fprintf(f, " %d", members[owner][j]);
        fputc('\n', f);
    }
    int32_t bezel, strength;
    bool chroma;
    ui_lens_settings_get(&bezel, &strength, &chroma);
    fprintf(f, "%ld %ld %d\n", (long)bezel, (long)strength, chroma);
    fprintf(f, "%d\n", s->low_power);
    fprintf(f, "%d %d %d %d %d %d\n", s->lock_widget[2], s->lock_widget[3],
            s->lock_widget_span[0], s->lock_widget_span[1], s->lock_widget_span[2], s->lock_widget_span[3]);
    fclose(f);
}

#endif
