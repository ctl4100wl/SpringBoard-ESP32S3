/**
 * @file app_photos.c
 * Real-photo gallery: online pictures cached next to the program plus pictures imported from the PC,
 * full-screen viewer with swipe, favorites, "Set as Wallpaper", and the Photo widget.
 */
#include <stdio.h>
#include <stdlib.h>
#include "../os/sdk.h"

void lv_image_cache_drop(const void * src);

#define PHOTO_MAX   40
#define NET_N       12
#define THUMB_GAP   3
#define SLIDE_H     420

static const uint32_t k_pal[][2] = {
    { 0xFF9A8B, 0xFF6A88 }, { 0x667EEA, 0x764BA2 }, { 0x43E97B, 0x38F9D7 }, { 0xFA709A, 0xFEE140 },
    { 0x30CFD0, 0x330867 }, { 0xF6D365, 0xFDA085 }, { 0x5EE7DF, 0xB490CA }, { 0xD299C2, 0xFEF9D7 },
    { 0x89F7FE, 0x66A6FF }, { 0xFDDB92, 0xD1FDFF },
};
static const int k_net_id[NET_N] = { 10, 11, 14, 15, 16, 17, 18, 19, 20, 22, 24, 27 };   /* picsum.photos ids */

typedef struct {
    char path[260];
    lv_draw_buf_t * thumb, * big;
    bool fav, imported;
} photo_t;

static photo_t s_ph[PHOTO_MAX];
static int s_n;
static lv_obj_t * s_root, * s_grid, * s_count;
static lv_obj_t * s_thumb[PHOTO_MAX];
static int32_t s_side, s_W, s_H;

static void buf_free(lv_draw_buf_t ** b)
{
    if(!*b) return;
    lv_image_cache_drop(*b);
    lv_draw_buf_destroy(*b);
    *b = NULL;
}

static bool file_ok(const char * path)
{
    FILE * f = fopen(path, "rb");
    if(!f) return false;
    fclose(f);
    return true;
}

static void photos_load_list(void)
{
    for(int i = 0; i < PHOTO_MAX; i++) {
        buf_free(&s_ph[i].thumb);
        buf_free(&s_ph[i].big);
    }
    s_n = 0;
    for(int i = 0; i < NET_N; i++) {
        char rel[64];
        lv_snprintf(rel, sizeof(rel), "cache/photos/photo_net_%d.jpg", k_net_id[i]);
#ifdef _WIN32
        if(!_fullpath(s_ph[s_n].path, rel, sizeof(s_ph[0].path))) lv_strncpy(s_ph[s_n].path, rel, sizeof(s_ph[0].path) - 1);
#else
        lv_strncpy(s_ph[s_n].path, rel, sizeof(s_ph[0].path) - 1);
#endif
        s_ph[s_n].imported = false;
        s_n++;
    }
    int cnt = sdk_get_int("photos.n", 0);
    for(int i = 0; i < cnt && s_n < PHOTO_MAX; i++) {
        char key[24];
        lv_snprintf(key, sizeof(key), "photos.p%d", i);
        const char * p = sdk_get_str(key, "");
        if(!p[0]) continue;
        lv_strncpy(s_ph[s_n].path, p, sizeof(s_ph[0].path) - 1);
        s_ph[s_n].imported = true;
        s_n++;
    }
}

static struct {
    lv_obj_t * viewer, * strip, * caption, * fav;
    lv_obj_t * slide[3], * slide_img[3], * slide_sym[3];
    lv_obj_t * wall;
    int cur, pending, mode;
    lv_point_t p0, last;
    uint32_t t_last;
    float vx, vy;
} u;

static void paint(lv_obj_t * o, int i)
{
    int p = (i * 7) % (int)(sizeof(k_pal) / sizeof(k_pal[0]));
    lv_obj_set_style_bg_opa(o, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(o, lv_color_hex(k_pal[p][0]), 0);
    lv_obj_set_style_bg_grad_color(o, lv_color_hex(k_pal[p][1]), 0);
    lv_obj_set_style_bg_grad_dir(o, i % 2 ? LV_GRAD_DIR_VER : LV_GRAD_DIR_HOR, 0);
}

static void viewer_labels(void)
{
    lv_label_set_text_fmt(u.caption, "%s  \xE2\x80\xA2  %d of %d", s_ph[u.cur].imported ? "From PC" : "Photo", u.cur + 1, s_n);
    lv_obj_set_style_text_color(lv_obj_get_child(u.fav, 0), lv_color_hex(s_ph[u.cur].fav ? UI_RED : 0xFFFFFF), 0);
}

/* The strip holds previous / current / next; it sits one screen to the left so "current" is in view */
static void slides_paint(void)
{
    bool keep[PHOTO_MAX] = { false };
    for(int s = 0; s < 3; s++) {
        int idx = (u.cur + s - 1 + s_n) % s_n;
        keep[idx] = true;
        paint(u.slide[s], idx);
        if(!s_ph[idx].big && file_ok(s_ph[idx].path)) s_ph[idx].big = sdk_image_load(s_ph[idx].path, s_W, SLIDE_H);
        lv_image_set_src(u.slide_img[s], s_ph[idx].big);
        lv_obj_set_hidden(u.slide_img[s], !s_ph[idx].big);
        lv_obj_set_hidden(u.slide_sym[s], s_ph[idx].big != NULL);
    }
    for(int i = 0; i < s_n; i++) if(!keep[i] && s_ph[i].big) buf_free(&s_ph[i].big);
    viewer_labels();
}

static void viewer_show(int i)
{
    u.cur = (i + s_n) % s_n;
    slides_paint();
    bool was_hidden = lv_obj_is_hidden(u.viewer);
    lv_obj_set_hidden(u.viewer, false);
    fx_set(u.strip, FX_X, 0);
    fx_set(u.strip, FX_Y, 0);
    if(was_hidden) {                                    /* opens by growing out of the grid */
        fx_set(u.viewer, FX_OPA, 0);
        fx_to(u.viewer, FX_OPA, 255, 0.22f, 1.0f, 0, NULL);
        fx_set(u.strip, FX_SCALE, 190);
        fx_to(u.strip, FX_SCALE, 256, 0.46f, 0.68f, 0, NULL);
    }
}

static void viewer_hidden_cb(lv_obj_t * o)
{
    lv_obj_set_hidden(o, true);
    fx_set(u.strip, FX_Y, 0);
    fx_set(u.viewer, FX_OPA, 255);
}

static void viewer_close(void)
{
    fx_to(u.viewer, FX_OPA, 0, 0.24f, 1.0f, 40, viewer_hidden_cb);
    fx_to(u.strip, FX_SCALE, 200, 0.30f, 1.0f, 0, NULL);
}

static void thumb_cb(lv_event_t * e)
{
    viewer_show((int)(intptr_t)lv_event_get_user_data(e));
}

static void slid_cb(lv_obj_t * o)
{
    LV_UNUSED(o);
    u.cur = (u.cur + u.pending + s_n) % s_n;
    u.pending = 0;
    slides_paint();
    fx_set(u.strip, FX_X, 0);
}

/* Photo strip that follows the finger and settles on a spring; pull down to dismiss */
static void viewer_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_point_t p;
    lv_indev_get_point(lv_indev_active(), &p);
    int32_t W = lv_obj_get_width(u.viewer);
    if(code == LV_EVENT_PRESSED) {
        u.p0 = u.last = p;
        u.t_last = lv_tick_get();
        u.vx = u.vy = 0;
        u.mode = 0;
        if(u.pending) {                                 /* grabbed mid-slide: land it first */
            fx_stop(u.strip);
            slid_cb(NULL);
        }
        fx_stop(u.strip);
    }
    else if(code == LV_EVENT_PRESSING) {
        uint32_t el = lv_tick_elaps(u.t_last);
        if(el) {
            u.vx = 0.6f * u.vx + 0.4f * (float)(p.x - u.last.x) / (float)el;
            u.vy = 0.6f * u.vy + 0.4f * (float)(p.y - u.last.y) / (float)el;
            u.last = p;
            u.t_last = lv_tick_get();
        }
        int32_t dx = p.x - u.p0.x, dy = p.y - u.p0.y;
        if(u.mode == 0 && (LV_ABS(dx) > 8 || LV_ABS(dy) > 8)) u.mode = LV_ABS(dx) >= LV_ABS(dy) ? 1 : (dy > 0 ? 2 : 3);
        if(u.mode == 1) {
            fx_set(u.strip, FX_X, (float)dx);
        }
        else if(u.mode == 2) {
            fx_set(u.strip, FX_Y, (float)dy * 0.9f);
            float q = LV_CLAMP(0.0f, 1.0f - dy / 360.0f, 1.0f);
            fx_set(u.strip, FX_SCALE, 256.0f * (0.78f + 0.22f * q));
            fx_set(u.viewer, FX_OPA, 90.0f + 165.0f * q);
        }
    }
    else if(code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
        int32_t dx = p.x - u.p0.x, dy = p.y - u.p0.y;
        if(u.mode == 1) {
            int dir = (dx < -W / 4 || u.vx < -0.4f) ? 1 : (dx > W / 4 || u.vx > 0.4f) ? -1 : 0;
            if(dir) {
                u.pending = dir;
                fx_to(u.strip, FX_X, -(float)dir * (float)W, 0.42f, 0.84f, 0, slid_cb);
                fx_kick(u.strip, FX_X, u.vx * 1000.0f);
            }
            else {
                fx_to(u.strip, FX_X, 0, 0.45f, 0.66f, 0, NULL);
                fx_kick(u.strip, FX_X, u.vx * 600.0f);
            }
        }
        else if(u.mode == 2) {
            if(dy > 110 || u.vy > 0.6f) {
                fx_to(u.strip, FX_Y, (float)lv_obj_get_height(u.viewer), 0.32f, 1.0f, 0, NULL);
                viewer_close();
            }
            else {
                fx_to(u.strip, FX_Y, 0, 0.45f, 0.66f, 0, NULL);
                fx_to(u.strip, FX_SCALE, 256, 0.45f, 0.66f, 0, NULL);
                fx_to(u.viewer, FX_OPA, 255, 0.25f, 1.0f, 0, NULL);
            }
        }
    }
}

static void close_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    viewer_close();
}

static void fav_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    s_ph[u.cur].fav = !s_ph[u.cur].fav;
    viewer_labels();
}

static void thumb_fill(int i)
{
    if(i < 0 || i >= s_n || !s_thumb[i]) return;
    if(!s_ph[i].thumb && file_ok(s_ph[i].path)) s_ph[i].thumb = sdk_image_load(s_ph[i].path, s_side, s_side);
    if(!s_ph[i].thumb) return;
    lv_obj_clean(s_thumb[i]);
    lv_obj_set_style_bg_opa(s_thumb[i], LV_OPA_TRANSP, 0);
    lv_image_set_src(lv_image_create(s_thumb[i]), s_ph[i].thumb);
}

static void net_cb(bool ok, const uint8_t * data, size_t len, void * ud)
{
    int i = (int)(intptr_t)ud;
    if(!ok || len < 64 || i < 0 || i >= NET_N) return;
    sdk_ensure_dir("cache/photos");
    FILE * f = fopen(s_ph[i].path, "wb");
    if(f) {
        fwrite(data, 1, len, f);
        fclose(f);
    }
    thumb_fill(i);
}

static void grid_build(void)
{
    lv_obj_clean(s_grid);
    lv_memzero(s_thumb, sizeof(s_thumb));
    for(int i = 0; i < s_n; i++) {
        lv_obj_t * t = ui_box(s_grid);
        lv_obj_set_size(t, s_side, s_side);
        paint(t, i);
        lv_obj_set_style_clip_corner(t, true, 0);
        lv_obj_set_clickable(t, true);
        lv_obj_set_style_opa(t, LV_OPA_70, LV_STATE_PRESSED);
        lv_obj_add_event_cb(t, thumb_cb, LV_EVENT_CLICKED, (void *)(intptr_t)i);
        s_thumb[i] = t;
        if(file_ok(s_ph[i].path)) thumb_fill(i);
        else {
            lv_obj_center(ui_label(t, LV_SYMBOL_IMAGE, &lv_font_montserrat_24, 0xFFFFFF));
            if(!s_ph[i].imported) {
                char url[96];
                lv_snprintf(url, sizeof(url), "https://picsum.photos/id/%d/600/800", k_net_id[i]);
                sdk_http_get_data(url, net_cb, (void *)(intptr_t)i, s_root);
            }
        }
    }
    lv_label_set_text_fmt(s_count, "%d Photos", s_n);
}

static void imported_cb(const char * path, void * ud)
{
    LV_UNUSED(ud);
    if(!path || !s_grid) return;
    lv_draw_buf_t * test = sdk_image_load(path, 32, 32);
    if(!test) {
        sdk_toast("Photos", "This photo couldn't be opened.");
        return;
    }
    buf_free(&test);
    int cnt = sdk_get_int("photos.n", 0);
    if(s_n >= PHOTO_MAX) {
        sdk_toast("Photos", "The library is full.");
        return;
    }
    char key[24];
    lv_snprintf(key, sizeof(key), "photos.p%d", cnt);
    sdk_set_str(key, path);
    sdk_set_int("photos.n", cnt + 1);
    sdk_set_str("photos.import", path);
    photos_load_list();
    grid_build();
    viewer_show(s_n - 1);
}

static void import_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    sdk_pick_file_async("Choose an image", "Images (*.png;*.jpg;*.jpeg;*.bmp)\0*.png;*.jpg;*.jpeg;*.bmp\0", imported_cb, NULL);
}

static void wallpaper_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    if(u.cur < 0 || u.cur >= s_n || !file_ok(s_ph[u.cur].path)) return;
    import_offer(s_ph[u.cur].path);                    /* sheet: Home / Lock / Both */
}

static void root_deleted_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    for(int i = 0; i < PHOTO_MAX; i++) {
        buf_free(&s_ph[i].thumb);
        buf_free(&s_ph[i].big);
    }
    s_root = s_grid = s_count = NULL;
    lv_memzero(s_thumb, sizeof(s_thumb));
}

static void create(lv_obj_t * root)
{
    s_root = root;
    lv_obj_t * body = ui_app_scaffold(root, "Photos");
    s_W = lv_display_get_horizontal_resolution(lv_display_get_default());
    s_H = lv_display_get_vertical_resolution(lv_display_get_default());
    s_side = (s_W - 32 - 2 * THUMB_GAP) / 3;
    photos_load_list();

    lv_obj_t * bar = ui_box(body);
    lv_obj_set_size(bar, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(bar, 8, 0);
    lv_obj_t * b = ui_button(bar, LV_SYMBOL_DOWNLOAD " Add from PC", UI_GROUP, import_cb, NULL);
    lv_obj_set_flex_grow(b, 1);
    lv_obj_set_style_text_font(lv_obj_get_child(b, 0), &lv_font_montserrat_14, 0);

    s_grid = ui_box(body);
    lv_obj_set_size(s_grid, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(s_grid, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_style_pad_column(s_grid, THUMB_GAP, 0);
    lv_obj_set_style_pad_row(s_grid, THUMB_GAP, 0);
    s_count = ui_label(body, "", &lv_font_montserrat_14, UI_DIM);
    grid_build();

    int32_t W = s_W, H = s_H;
    u.viewer = ui_box(root);
    lv_obj_set_floating(u.viewer, true);
    lv_obj_set_size(u.viewer, W, H);
    lv_obj_set_pos(u.viewer, -16, -40);
    lv_obj_set_style_bg_opa(u.viewer, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(u.viewer, lv_color_black(), 0);
    lv_obj_set_clickable(u.viewer, true);
    lv_obj_set_gesture_bubble(u.viewer, false);
    lv_obj_add_event_cb(u.viewer, viewer_cb, LV_EVENT_ALL, NULL);

    u.strip = ui_box(u.viewer);
    lv_obj_set_size(u.strip, W * 3, SLIDE_H);
    lv_obj_set_pos(u.strip, -W, (H - SLIDE_H) / 2);
    for(int k = 0; k < 3; k++) {
        u.slide[k] = ui_box(u.strip);
        lv_obj_set_size(u.slide[k], W, SLIDE_H);
        lv_obj_set_pos(u.slide[k], k * W, 0);
        u.slide_img[k] = lv_image_create(u.slide[k]);
        lv_obj_set_size(u.slide_img[k], W, SLIDE_H);
        lv_image_set_inner_align(u.slide_img[k], LV_IMAGE_ALIGN_COVER);
        u.slide_sym[k] = ui_label(u.slide[k], LV_SYMBOL_IMAGE, &lv_font_montserrat_48, 0xFFFFFF);
        lv_obj_center(u.slide_sym[k]);
    }

    u.caption = ui_label(u.viewer, "", &lv_font_montserrat_14, 0xFFFFFF);
    lv_obj_align(u.caption, LV_ALIGN_TOP_MID, 0, 46);

    lv_obj_t * close = ui_button(u.viewer, LV_SYMBOL_CLOSE, 0x2C2C2E, close_cb, NULL);
    lv_obj_set_size(close, 36, 36);
    lv_obj_set_style_radius(close, LV_RADIUS_CIRCLE, 0);
    lv_obj_align(close, LV_ALIGN_TOP_LEFT, 12, 40);

    u.fav = ui_button(u.viewer, LV_SYMBOL_OK " Favorite", 0x2C2C2E, fav_cb, NULL);
    lv_obj_align(u.fav, LV_ALIGN_BOTTOM_LEFT, 16, -OS_GESTURE_H - 14);
    u.wall = ui_button(u.viewer, LV_SYMBOL_IMAGE " Set as Wallpaper", UI_BLUE, wallpaper_cb, NULL);
    lv_obj_align(u.wall, LV_ALIGN_BOTTOM_RIGHT, -16, -OS_GESTURE_H - 14);

    lv_obj_set_hidden(u.viewer, true);
    lv_obj_add_event_cb(root, root_deleted_cb, LV_EVENT_DELETE, NULL);
}

const app_desc_t app_photos = { "Photos", LV_SYMBOL_IMAGE, 0xFF9F0A, create };

/*---------------- Photo widget: your latest imported picture, or the first online photo ----------------*/

static void photo_widget_deleted(lv_event_t * e)
{
    lv_draw_buf_t * buf = lv_event_get_user_data(e);
    lv_image_cache_drop(buf);
    lv_draw_buf_destroy(buf);
}

WIDGET(widget_photo, "Photo", &app_photos, WIDGET_SMALL, NULL)
{
    lv_obj_set_style_pad_all(box, 0, 0);
    lv_obj_update_layout(box);
    const char * path = sdk_get_str("photos.import", "");
    if(!path[0] && file_ok("cache/photos/photo_net_10.jpg")) path = "cache/photos/photo_net_10.jpg";
    lv_draw_buf_t * buf = path[0] ? sdk_image_load(path, lv_obj_get_width(box), lv_obj_get_height(box)) : NULL;
    if(buf) {
        lv_obj_t * img = lv_image_create(box);
        lv_image_set_src(img, buf);
        lv_obj_add_event_cb(box, photo_widget_deleted, LV_EVENT_DELETE, buf);
        return;
    }
    paint(box, 3);
    lv_obj_t * l = ui_label(box, LV_SYMBOL_IMAGE "\nImport a photo\nin Photos", &lv_font_montserrat_14, 0xFFFFFF);
    lv_obj_set_style_text_align(l, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_center(l);
}
