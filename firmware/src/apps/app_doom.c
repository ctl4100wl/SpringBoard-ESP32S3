/**
 * @file app_doom.c
 * Doom for fun on the PC build: doomgeneric engine, touch buttons plus the PC keyboard.
 * Uses your own doom1.wad / doom.wad / doom2.wad (next to the program or in cache/doom), otherwise downloads the free
 * Freedoom game data on request.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../os/os.h"
#include "../os/ui.h"
#include "../os/sdk.h"
#include "../third_party/stb/stb_image.h"

void lv_image_cache_drop(const void * src);

#if defined(ESP_PLATFORM) && defined(S3_DOOM)
#include "s3_doom.h"
#define create s3_doom_create
#elif defined(S3_DOOM)
#include <SDL.h>
#include "doomkeys.h"

typedef void (*doom_frame_cb_t)(const uint32_t * pixels);
bool doom_start(const char * wad, doom_frame_cb_t cb);
void doom_tick(void);
void doom_key(int pressed, unsigned char key);
bool doom_dead(void);
const char * doom_last_error(void);

#define DW 320
#define DH 200

static const char * const k_wads[] = { "cache/doom/doom.wad", "cache/doom/doom1.wad", "cache/doom/doom2.wad", "cache/doom/freedoom1.wad",
                                       "doom.wad", "doom1.wad", "doom2.wad", "freedoom1.wad" };

static struct {
    lv_obj_t * img, * msg, * screen, * dl;
    lv_timer_t * timer;
    lv_draw_buf_t * buf;
} u;
static bool s_started, s_dead;
static unsigned char s_prev[SDL_NUM_SCANCODES];

static bool exists(const char * p)
{
    FILE * f = fopen(p, "rb");
    if(f) fclose(f);
    return f != NULL;
}

static const char * find_wad(void)
{
    for(size_t i = 0; i < sizeof(k_wads) / sizeof(k_wads[0]); i++) if(exists(k_wads[i])) return k_wads[i];
    return NULL;
}

static void frame_cb(const uint32_t * px)
{
    if(!u.buf) return;
    for(int y = 0; y < DH; y++) memcpy((uint8_t *)u.buf->data + (size_t)y * u.buf->header.stride, px + (size_t)y * DW, DW * 4);
    if(u.img) lv_obj_invalidate(u.img);
}

static struct { int scancode; unsigned char key; } const k_map[] = {
    { SDL_SCANCODE_UP, KEY_UPARROW },   { SDL_SCANCODE_DOWN, KEY_DOWNARROW }, { SDL_SCANCODE_LEFT, KEY_LEFTARROW },
    { SDL_SCANCODE_RIGHT, KEY_RIGHTARROW }, { SDL_SCANCODE_LCTRL, KEY_FIRE },  { SDL_SCANCODE_SPACE, KEY_USE },
    { SDL_SCANCODE_RETURN, KEY_ENTER }, { SDL_SCANCODE_ESCAPE, KEY_ESCAPE },  { SDL_SCANCODE_LSHIFT, KEY_RSHIFT },
    { SDL_SCANCODE_Y, 'y' },            { SDL_SCANCODE_N, 'n' },
};

static void poll_keyboard(void)
{
    const Uint8 * st = SDL_GetKeyboardState(NULL);
    for(size_t i = 0; i < sizeof(k_map) / sizeof(k_map[0]); i++) {
        unsigned char now = st[k_map[i].scancode];
        if(now != s_prev[k_map[i].scancode]) {
            s_prev[k_map[i].scancode] = now;
            doom_key(now, k_map[i].key);
        }
    }
}

static void tick_cb(lv_timer_t * t)
{
    LV_UNUSED(t);
    if(!s_started || s_dead) return;
    poll_keyboard();
    doom_tick();
    if(doom_dead()) {
        s_dead = true;
        lv_label_set_text_fmt(u.msg, "Game over%s%s - leave Doom and the OS to start again.", doom_last_error()[0] ? ": " : "", doom_last_error());
        lv_obj_set_hidden(u.msg, false);
    }
}

static void begin(const char * wad)
{
    sdk_ensure_dir("cache/doom/saves");
    lv_obj_set_hidden(u.dl, true);
    lv_obj_set_hidden(u.msg, true);
    if(!doom_start(wad, frame_cb)) {
        s_dead = true;
        lv_label_set_text_fmt(u.msg, "Doom could not start: %s", doom_last_error()[0] ? doom_last_error() : "unknown error");
        lv_obj_set_hidden(u.msg, false);
        return;
    }
    s_started = true;
}

/* Minimal zip reader: finds *freedoom1.wad in the downloaded archive and writes it to cache/doom */
static bool extract_wad(const uint8_t * z, size_t n, const char * out_path)
{
    if(n < 22) return false;
    size_t eocd = n - 22;
    while(eocd > 0 && !(z[eocd] == 0x50 && z[eocd + 1] == 0x4b && z[eocd + 2] == 5 && z[eocd + 3] == 6)) eocd--;
    uint32_t cd_off = z[eocd + 16] | z[eocd + 17] << 8 | z[eocd + 18] << 16 | (uint32_t)z[eocd + 19] << 24;
    unsigned cd_n = z[eocd + 10] | z[eocd + 11] << 8;
    size_t p = cd_off;
    for(unsigned i = 0; i < cd_n && p + 46 < n; i++) {
        unsigned method = z[p + 10] | z[p + 11] << 8;
        uint32_t csize = z[p + 20] | z[p + 21] << 8 | z[p + 22] << 16 | (uint32_t)z[p + 23] << 24;
        uint32_t usize = z[p + 24] | z[p + 25] << 8 | z[p + 26] << 16 | (uint32_t)z[p + 27] << 24;
        unsigned fl = z[p + 28] | z[p + 29] << 8, el = z[p + 30] | z[p + 31] << 8, cl = z[p + 32] | z[p + 33] << 8;
        uint32_t lho = z[p + 42] | z[p + 43] << 8 | z[p + 44] << 16 | (uint32_t)z[p + 45] << 24;
        if(fl >= 13 && !memcmp((const char *)z + p + 46 + fl - 13, "freedoom1.wad", 13)) {
            size_t d = lho + 30 + (z[lho + 26] | z[lho + 27] << 8) + (z[lho + 28] | z[lho + 29] << 8);
            if(d + csize > n) return false;
            uint8_t * data = NULL;
            int len = 0;
            if(method == 0) {
                data = malloc(usize);
                if(data) memcpy(data, z + d, usize);
                len = (int)usize;
            }
            else if(method == 8) data = (uint8_t *)stbi_zlib_decode_noheader_malloc((const char *)z + d, (int)csize, &len);
            if(!data) return false;
            FILE * f = fopen(out_path, "wb");
            bool ok = f && fwrite(data, 1, (size_t)len, f) == (size_t)len;
            if(f) fclose(f);
            free(data);
            return ok;
        }
        p += 46 + fl + el + cl;
    }
    return false;
}

static void wad_cb(bool ok, const uint8_t * data, size_t len, void * ud)
{
    LV_UNUSED(ud);
    if(!u.dl) return;
    sdk_ensure_dir("cache/doom");
    if(ok && extract_wad(data, len, "cache/doom/freedoom1.wad")) {
        begin("cache/doom/freedoom1.wad");
        return;
    }
    lv_label_set_text(u.msg, "Download failed. Put doom1.wad next to the program.");
    lv_obj_set_hidden(u.msg, false);
    lv_obj_set_hidden(u.dl, false);
}

static void dl_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    lv_label_set_text(u.msg, "Downloading Freedoom (24 MB)...");
    lv_obj_set_hidden(u.msg, false);
    lv_obj_set_hidden(u.dl, true);
    sdk_http_get_data_ex("https://github.com/freedoom/freedoom/releases/download/v0.13.0/freedoom-0.13.0.zip", wad_cb, NULL, u.screen,
                         40u * 1024 * 1024);
}

static void key_btn_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    unsigned char key = (unsigned char)(intptr_t)lv_event_get_user_data(e);
    if(code == LV_EVENT_PRESSED) doom_key(1, key);
    else if(code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) doom_key(0, key);
}

static lv_obj_t * pad_btn(lv_obj_t * parent, const char * text, unsigned char key, int32_t w, int32_t h)
{
    lv_obj_t * b = ui_button(parent, text, 0x2C2C2E, NULL, NULL);
    lv_obj_set_size(b, w, h);
    lv_obj_set_style_radius(b, 14, 0);
    lv_obj_add_event_cb(b, key_btn_cb, LV_EVENT_ALL, (void *)(intptr_t)key);
    return b;
}

static void root_delete_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    if(u.timer) lv_timer_delete(u.timer);
    if(u.buf) {
        lv_image_cache_drop(u.buf);
        lv_draw_buf_destroy(u.buf);
    }
    memset(&u, 0, sizeof(u));
}

static void create(lv_obj_t * root)
{
    lv_obj_t * body = ui_app_scaffold(root, "Doom");
    lv_obj_set_flex_align(body, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(body, 8, 0);

    u.screen = ui_box(body);
    lv_obj_set_size(u.screen, 296, 185);
    lv_obj_set_style_bg_opa(u.screen, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(u.screen, lv_color_black(), 0);
    lv_obj_set_style_radius(u.screen, 8, 0);
    lv_obj_set_style_clip_corner(u.screen, true, 0);
    u.buf = lv_draw_buf_create(DW, DH, LV_COLOR_FORMAT_XRGB8888, 0);
    if(u.buf) lv_draw_buf_clear(u.buf, NULL);
    u.img = lv_image_create(u.screen);
    lv_image_set_src(u.img, u.buf);
    lv_image_set_scale(u.img, 256 * 296 / DW);
    lv_obj_center(u.img);

    u.msg = ui_label(body, "", &lv_font_montserrat_14, UI_ORANGE);
    lv_obj_set_width(u.msg, LV_PCT(100));
    lv_label_set_long_mode(u.msg, LV_LABEL_LONG_MODE_WRAP);
    lv_obj_set_hidden(u.msg, true);
    u.dl = ui_button(body, LV_SYMBOL_DOWNLOAD " Get free game data (Freedoom)", UI_GROUP, dl_cb, NULL);
    lv_obj_set_width(u.dl, LV_PCT(100));
    lv_obj_set_hidden(u.dl, true);

    lv_obj_t * row = ui_box(body);
    lv_obj_set_size(row, LV_PCT(100), 150);
    lv_obj_t * left = ui_box(row);
    lv_obj_set_size(left, 150, 150);
    lv_obj_align(left, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_t * b = pad_btn(left, LV_SYMBOL_UP, KEY_UPARROW, 48, 48);
    lv_obj_align(b, LV_ALIGN_TOP_MID, 0, 0);
    b = pad_btn(left, LV_SYMBOL_DOWN, KEY_DOWNARROW, 48, 48);
    lv_obj_align(b, LV_ALIGN_BOTTOM_MID, 0, 0);
    b = pad_btn(left, LV_SYMBOL_LEFT, KEY_LEFTARROW, 48, 48);
    lv_obj_align(b, LV_ALIGN_LEFT_MID, 0, 0);
    b = pad_btn(left, LV_SYMBOL_RIGHT, KEY_RIGHTARROW, 48, 48);
    lv_obj_align(b, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_t * right = ui_box(row);
    lv_obj_set_size(right, 130, 150);
    lv_obj_align(right, LV_ALIGN_RIGHT_MID, 0, 0);
    b = pad_btn(right, "FIRE", KEY_FIRE, 110, 44);
    lv_obj_set_style_bg_color(b, lv_color_hex(0x8B1A1A), 0);
    lv_obj_align(b, LV_ALIGN_TOP_MID, 0, 0);
    b = pad_btn(right, "USE", KEY_USE, 110, 38);
    lv_obj_align(b, LV_ALIGN_CENTER, 0, 4);
    b = pad_btn(right, "MENU", KEY_ESCAPE, 52, 36);
    lv_obj_align(b, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    b = pad_btn(right, "OK", KEY_ENTER, 52, 36);
    lv_obj_align(b, LV_ALIGN_BOTTOM_RIGHT, 0, 0);

    ui_label(body, "PC keys: arrows, Ctrl fire, Space use, Shift run, Esc menu", &lv_font_montserrat_12, UI_DIM);

    lv_obj_add_event_cb(root, root_delete_cb, LV_EVENT_DELETE, NULL);
    u.timer = lv_timer_create(tick_cb, 14, NULL);

    if(s_dead) {
        lv_label_set_text(u.msg, "Game over - restart the OS to play again.");
        lv_obj_set_hidden(u.msg, false);
    }
    else if(!s_started) {
        const char * wad = find_wad();
        if(wad) begin(wad);
        else {
#ifdef __EMSCRIPTEN__
            lv_label_set_text(u.msg, "No Doom game data found. Put doom1.wad next to index.html (the site root) and reload.");
            lv_obj_set_hidden(u.msg, false);
#else
            lv_label_set_text(u.msg, "No Doom game data found. Put doom1.wad next to the program, or download the free Freedoom data.");
            lv_obj_set_hidden(u.msg, false);
            lv_obj_set_hidden(u.dl, false);
#endif
        }
    }
}
#else
static void create(lv_obj_t * root)
{
    lv_obj_t * body = ui_app_scaffold(root, "Doom");
    ui_label(body, "Doom runs on the PC build only.", &lv_font_montserrat_14, UI_DIM);
}
#endif

const app_desc_t app_doom = { "Doom", LV_SYMBOL_PLAY, 0x8B1A1A, create };
