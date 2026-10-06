/**
 * @file lock.c
 * Lock Screen, Notification Center and notification banners (iOS 18 style).
 *
 *   locked:      swipe up anywhere to unlock (the sheet follows the finger, flings with velocity)
 *   hold:        customize: the face shrinks into a card (spring), swipe sideways for clock styles,
 *                tap a colour / wallpaper, tap the card or Done to expand back out
 *   pull down from the top-left edge (anywhere): Notification Center = the lock screen, no unlock needed
 *   notifications: banner drops in from the top when unlocked; stacked on the lock screen otherwise
 */
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "sdk.h"

#define PULL_ZONE_H     26
#define LONG_PRESS_MS   550
#define SLOP            8
#define NOTIF_MAX       24
#define BOTTOM_H        110     /* flashlight / camera / hint strip under the list */
#define SHADE_BLUR      30
#define BANNER_MS       4200
#define CLOCK_TOP       58

void lv_image_cache_drop(const void * src);
extern const lv_font_t font_clock_96;   /* digits-only 96 px (font_clock_96.c): sharp, no bitmap scaling */

typedef struct {
    int app;
    char title[48];
    char body[140];
    time_t t;
} notif_t;

typedef enum { M_IDLE, M_PRESS, M_SLIDE, M_EDIT_SWIPE, M_LIST, M_CARD, M_NONE } lmode_t;

static struct {
    int32_t W, H;
    lv_obj_t * root;        /* full screen, bottom of the top layer */
    lv_obj_t * face;        /* wallpaper + clock + lock widgets: shrinks into a card when customizing */
    lv_obj_t * clock;       /* style-specific group inside face */
    lv_obj_t * time_lbl[3];
    lv_obj_t * halo;        /* Glow style: blurred copy of the numerals */
    lv_draw_buf_t * halo_buf;
    char halo_txt[8];
    int32_t glass_bezel, glass_strength;
    bool glass_chroma;
    lv_obj_t * date_lbl;
    lv_obj_t * wrow;        /* lock screen widgets */
    lv_obj_t * list;        /* notification cards (scrolls when they overflow) */
    lv_obj_t * shade;       /* blur layer between the wallpaper and the clock, used by the pull-down shade */
    float shade_p;
    int32_t scroll0;        /* list scroll position when a list drag began */
    lv_obj_t * sw_card;     /* notification card being swiped (or resting open) */
    lv_obj_t * sw_btn;      /* its Clear pill */
    bool sw_open, sw_armed;
    lv_obj_t * bottom;      /* flashlight, camera, hint */
    lv_obj_t * torch;
    lv_obj_t * edit;        /* customize chrome */
    lv_obj_t * edit_name;
    lv_obj_t * edit_grid;
    lv_obj_t * widget_picker;
    lv_obj_t * zone;        /* top-left pull for Notification Center */
    lv_timer_t * tick;
    int32_t face_bottom;
    int32_t clock_bottom;

    bool visible, locked, editing;
    lmode_t mode;
    lv_point_t p0, last;
    uint32_t t0, last_tick;
    float vx, vy;
    bool from_zone;

    notif_t n[NOTIF_MAX];
    int nn;
} k;

static const uint32_t k_tints[8] = { 0xFFFFFF, 0xFFD60A, 0xFF9F0A, 0xFF375F, 0xBF5AF2, 0x64D2FF, 0x30D158, 0xA2845E };
static const char * const k_style_names[OS_LOCK_STYLES] = { "Classic", "Glass", "Stacked", "Glow", "Frosted", "Prism" };
static const char * const k_wday[] = { "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };
static const char * const k_month[] = { "January", "February", "March", "April", "May", "June", "July",
                                        "August", "September", "October", "November", "December" };

static void halo_update(const char * full);
static void clock_fade(float q);
static int32_t list_max_scroll(void);
static void swipe_close(bool animate);
static void swipe_dismiss(int dir);
static bool hit(lv_obj_t * o, lv_point_t p);
static void notif_remove(int i);
static void list_build(int pop_index);
static void face_build(void);
static void list_build(int pop_index);
static void edit_enter(void);
static void edit_exit(void);
static void show(bool locked, bool anim_from_top);
static void hide(bool unlock);

/*================ Clock ================*/

static void time_text(char * hh, char * mm, char * full, char * date)
{
    time_t now = time(NULL);
    struct tm * tm = localtime(&now);
    int h = tm ? tm->tm_hour % 12 : 9;
    if(h == 0) h = 12;
    int m = tm ? tm->tm_min : 41;
    sprintf(hh, "%d", h);
    sprintf(mm, "%02d", m);
    sprintf(full, "%d:%02d", h, m);
    if(tm) sprintf(date, "%s, %s %d", k_wday[tm->tm_wday], k_month[tm->tm_mon], tm->tm_mday);
    else strcpy(date, "Monday, June 9");
}

static void tick_cb(lv_timer_t * t)
{
    LV_UNUSED(t);
    if(!k.visible || !k.clock) return;
    char hh[4], mm[4], full[8], date[40];
    time_text(hh, mm, full, date);
    if(k.date_lbl) lv_label_set_text(k.date_lbl, date);
    if(os_settings.lock_style == 2) {
        lv_label_set_text(k.time_lbl[0], hh);
        lv_label_set_text(k.time_lbl[1], mm);
    }
    else {
        if(k.time_lbl[0]) lv_label_set_text(k.time_lbl[0], full);
        if(k.halo) {
            lv_obj_update_layout(k.clock);
            halo_update(full);
        }
    }
}

static lv_obj_t * big_label(lv_obj_t * parent, uint32_t color)
{
    return ui_label(parent, "", &font_clock_96, color);
}

/* Adaptive colour: look at the wallpaper behind the clock and pick a tint that reads on it, a light version
 * of the picture's own hue on dark wallpapers, a deep one on bright wallpapers (like iOS "auto" clock colour). */
static uint32_t auto_tint(void)
{
    float r = 200, g = 200, b = 200;
    int w = os_settings.lock_wall;
    lv_draw_buf_t * img = w == OS_WALL_CUSTOM ? shell_wallpaper_image(true) : NULL;
    if(img && img->header.cf == LV_COLOR_FORMAT_RGB565) {
        int32_t y0 = CLOCK_TOP, y1 = LV_MIN(CLOCK_TOP + 150, (int32_t)img->header.h);
        float n = 0;
        r = g = b = 0;
        for(int32_t y = y0; y < y1; y += 5) {
            const uint16_t * row = (const uint16_t *)(img->data + (size_t)y * img->header.stride);
            for(int32_t x = 0; x < (int32_t)img->header.w; x += 5) {
                uint16_t v = row[x];
                r += (float)(((v >> 11) & 31) << 3);
                g += (float)(((v >> 5) & 63) << 2);
                b += (float)((v & 31) << 3);
                n++;
            }
        }
        if(n > 0) { r /= n; g /= n; b /= n; }
    }
    else {
        uint32_t top, bottom;
        shell_wallpaper_colors_lock(w == OS_WALL_CUSTOM ? 0 : w, &top, &bottom);
        float t = (float)(CLOCK_TOP + 75) / (float)k.H;
        r = ((top >> 16) & 0xFF) * (1 - t) + ((bottom >> 16) & 0xFF) * t;
        g = ((top >> 8) & 0xFF) * (1 - t) + ((bottom >> 8) & 0xFF) * t;
        b = (top & 0xFF) * (1 - t) + (bottom & 0xFF) * t;
    }
    float lum = 0.299f * r + 0.587f * g + 0.114f * b;
    float sat = 1.35f;                                  /* push the hue a bit, grey wallpapers stay grey */
    r = lum + (r - lum) * sat;
    g = lum + (g - lum) * sat;
    b = lum + (b - lum) * sat;
    if(lum > 150.0f) { r *= 0.22f; g *= 0.22f; b *= 0.22f; }
    else { r += (255 - r) * 0.82f; g += (255 - g) * 0.82f; b += (255 - b) * 0.82f; }
    return ((uint32_t)LV_CLAMP(0, (int)r, 255) << 16) | ((uint32_t)LV_CLAMP(0, (int)g, 255) << 8) |
           (uint32_t)LV_CLAMP(0, (int)b, 255);
}

static uint32_t clock_tint(void)
{
    return os_settings.lock_color == OS_LOCK_COLOR_AUTO ? auto_tint() : k_tints[os_settings.lock_color & 7];
}

/* Glow / Glass: rebuild the blurred or glass numerals only when the text changed (true text blur, not a backdrop blur) */
#define GLASS_STYLE(s) ((s) == 1 || (s) == 4 || (s) == 5)

static void halo_update(const char * full)
{
    if(!k.halo) return;
    int32_t bezel, strength;
    bool chroma;
    ui_lens_settings_get(&bezel, &strength, &chroma);
    if(strcmp(k.halo_txt, full) == 0 && k.halo_buf &&
       (!GLASS_STYLE(os_settings.lock_style) || (k.glass_bezel == bezel && k.glass_strength == strength &&
                                       k.glass_chroma == chroma))) return;
    lv_strncpy(k.halo_txt, full, sizeof(k.halo_txt) - 1);
    k.glass_bezel = bezel;
    k.glass_strength = strength;
    k.glass_chroma = chroma;
    if(GLASS_STYLE(os_settings.lock_style)) {                  /* Glass: the digits themselves are liquid glass */
        const int32_t pad = 14;
        lv_area_t a, fa;
        lv_obj_get_coords(k.time_lbl[0], &a);
        lv_obj_get_coords(k.face, &fa);
        lv_obj_set_hidden(k.clock, true);              /* the picture to bend must not contain the old digits */
        ui_glass_text_mode(os_settings.lock_style == 4 ? 1 : os_settings.lock_style == 5 ? 2 : 0);
        lv_draw_buf_t * gb = ui_glass_text(k.face, full, &font_clock_96, pad, a.x1 - pad - fa.x1, a.y1 - pad - fa.y1,
                                           clock_tint(), bezel, strength, chroma);
        lv_obj_set_hidden(k.clock, false);
        if(!gb) return;
        lv_image_set_src(k.halo, gb);
        if(k.halo_buf) lv_draw_buf_destroy(k.halo_buf);
        k.halo_buf = gb;
        lv_obj_align_to(k.halo, k.time_lbl[0], LV_ALIGN_CENTER, 0, 0);
        return;
    }
    lv_draw_buf_t * nb = ui_text_halo(full, &font_clock_96, 24, 28);
    if(!nb) return;
    lv_image_set_src(k.halo, nb);
    if(k.halo_buf) lv_draw_buf_destroy(k.halo_buf);
    k.halo_buf = nb;
    lv_obj_align_to(k.halo, k.time_lbl[0], LV_ALIGN_CENTER, 0, 0);
}

static void clock_build(void)
{
    if(k.clock) lv_obj_delete(k.clock);
    k.time_lbl[0] = k.time_lbl[1] = k.time_lbl[2] = NULL;
    k.halo = NULL;
    k.halo_txt[0] = ' ';
    uint32_t tint = clock_tint();
    int st = os_settings.lock_style;

    k.clock = ui_box(k.face);
    lv_obj_set_width(k.clock, k.W);
    lv_obj_set_pos(k.clock, 0, CLOCK_TOP);
    lv_obj_set_overflow_visible(k.clock, true);
    int32_t h = 150;

    k.date_lbl = ui_label(k.clock, "", &lv_font_montserrat_18, tint);
    lv_obj_set_style_text_opa(k.date_lbl, LV_OPA_90, 0);
    int32_t time_top = 22;
#ifdef S3_NATIVE_LAYOUT
    lv_obj_update_layout(k.date_lbl);
    time_top = LV_MAX(time_top, lv_obj_get_height(k.date_lbl) + 3);
#endif

    if(st == 0) {               /* Classic: large centred numerals */
        lv_obj_align(k.date_lbl, LV_ALIGN_TOP_MID, 0, 0);
        k.time_lbl[0] = big_label(k.clock, tint);
        lv_obj_align(k.time_lbl[0], LV_ALIGN_TOP_MID, 0, time_top);
    }
    else if(GLASS_STYLE(st)) {          /* Glass / Frosted / Prism: the numerals are liquid glass (bezel refracts the wallpaper) */
        lv_obj_align(k.date_lbl, LV_ALIGN_TOP_MID, 0, 0);
        k.time_lbl[0] = big_label(k.clock, tint);
        lv_obj_align(k.time_lbl[0], LV_ALIGN_TOP_MID, 0, time_top);
        lv_obj_set_style_text_opa(k.time_lbl[0], LV_OPA_TRANSP, 0);    /* only gives the glass its size and place */
        k.halo = lv_image_create(k.clock);
        h = 140;
    }
    else if(st == 2) {          /* Stacked: hours over minutes, left aligned */
        lv_obj_align(k.date_lbl, LV_ALIGN_TOP_LEFT, 26, 0);
        k.time_lbl[0] = big_label(k.clock, tint);
        k.time_lbl[1] = big_label(k.clock, tint);
        lv_obj_set_pos(k.time_lbl[0], 20, 12);
        lv_obj_set_pos(k.time_lbl[1], 20, 112);
        h = 240;
    }
    else {                      /* Glow: soft blurred halo behind the numerals */
        lv_obj_align(k.date_lbl, LV_ALIGN_TOP_MID, 0, 0);
        k.time_lbl[0] = big_label(k.clock, lv_color_to_u32(lv_color_lighten(lv_color_hex(tint), 160)) & 0xFFFFFF);
        lv_obj_align(k.time_lbl[0], LV_ALIGN_TOP_MID, 0, time_top);
        k.halo = lv_image_create(k.clock);          /* behind the sharp numerals */
        lv_obj_move_to_index(k.halo, 0);
        lv_obj_set_style_image_recolor(k.halo, lv_color_hex(tint), 0);
        lv_obj_set_style_image_recolor_opa(k.halo, LV_OPA_COVER, 0);
        lv_obj_set_style_opa(k.halo, LV_OPA_90, 0);
        h = 140;
    }
    lv_obj_set_height(k.clock, h);
    k.face_bottom = CLOCK_TOP + h;
    k.clock_bottom = k.face_bottom;
    tick_cb(NULL);
}

static void wrow_build(void)
{
    k.face_bottom = k.clock_bottom;
    if(k.wrow) lv_obj_delete(k.wrow);
    k.wrow = NULL;
    int selected = 0;
    int rows = 0;
    for(int s = 0; s < 4; s++) {
        int i = os_settings.lock_widget[s];
        if(i >= 0 && i < os_widget_count) { selected++; rows = s / 2 + 1; }
    }
    if(!selected) return;
    k.wrow = ui_box(k.face);
    lv_obj_set_size(k.wrow, k.W, rows * 54);
    lv_obj_set_pos(k.wrow, 0, k.face_bottom + 4);
    for(int s = 0; s < 4; s++) {
        int i = os_settings.lock_widget[s];
        if(i < 0 || i >= os_widget_count) continue;
        if(s % 2 == 1 && os_settings.lock_widget_span[s - 1] == 2) continue;
        lv_obj_t * slot = ui_box(k.wrow);
        int wide = os_settings.lock_widget_span[s] == 2 && s % 2 == 0;
        lv_obj_set_size(slot, wide ? 264 : 128, 44);
        lv_obj_set_pos(slot, (k.W - 264) / 2 + (s % 2) * 136, (s / 2) * 54);
        lv_obj_set_style_radius(slot, 12, 0);
        ui_glass(slot, 0x2C2C2E, LV_OPA_40, 14);             /* blurred glass behind the widget */
        lv_obj_set_style_border_width(slot, 1, 0);
        lv_obj_set_style_border_color(slot, lv_color_white(), 0);
        lv_obj_set_style_border_opa(slot, LV_OPA_20, 0);
        lv_obj_set_style_pad_hor(slot, 8, 0);
        lv_obj_set_style_pad_ver(slot, 3, 0);
        lv_obj_remove_flag(slot, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_clip_corner(slot, true, 0);
        lv_obj_set_style_clip_corner(slot, true, 0);
        if(os_widgets[i]->lock_create) os_widgets[i]->lock_create(slot);
        else {
            lv_obj_t * icon = ui_label(slot, os_widgets[i]->app ? os_widgets[i]->app->symbol : LV_SYMBOL_LIST,
                                        &lv_font_montserrat_16, 0xFFFFFF);
            lv_obj_align(icon, LV_ALIGN_LEFT_MID, 3, 0);
            lv_obj_t * title = ui_label(slot, os_widgets[i]->name, &lv_font_montserrat_12, 0xFFFFFF);
            lv_obj_align(title, LV_ALIGN_LEFT_MID, 26, 0);
        }
        /* Nothing may spill out of the slot: long text is cut with dots instead of clipping mid-glyph */
        int32_t inner = (wide ? 264 : 128) - 16;
        for(uint32_t c = 0; c < lv_obj_get_child_count(slot); c++) {
            lv_obj_t * ch = lv_obj_get_child(slot, (int32_t)c);
            if(!lv_obj_check_type(ch, &lv_label_class)) continue;
            lv_label_set_long_mode(ch, LV_LABEL_LONG_MODE_DOTS);
            lv_obj_set_style_max_width(ch, inner - 30, 0);
        }
    }
    k.face_bottom += rows * 54 + 4;
}

/* Locked: list stops above the flashlight/camera strip. Shade (unlocked): the strip fades and the list gets the room */
static void list_fit(void)
{
    if(!k.list) return;
    int32_t top = k.face_bottom + (k.wrow ? 6 : -10);
    lv_obj_set_y(k.list, top);
    lv_obj_set_height(k.list, LV_MAX(60, k.H - top - (k.locked ? BOTTOM_H : 24)));
    if(k.bottom && !k.editing) fx_set(k.bottom, FX_OPA, k.locked ? 255 : 0);
}

static void face_build(void)
{
    uint32_t top, bottom;
    lv_draw_buf_t * img = os_settings.lock_wall == OS_WALL_CUSTOM ? shell_wallpaper_image(true) : NULL;
    shell_wallpaper_colors_lock(os_settings.lock_wall == OS_WALL_CUSTOM ? 0 : os_settings.lock_wall, &top, &bottom);
    lv_obj_set_style_bg_color(k.face, lv_color_hex(top), 0);
    lv_obj_set_style_bg_grad_color(k.face, lv_color_hex(bottom), 0);
    lv_obj_set_style_bg_image_src(k.face, img, 0);
    clock_build();
    wrow_build();
    list_fit();
}

/* Pop the face elements in one after the other */
static void face_pop_in(uint32_t delay)
{
#if OS_FX_HEAVY
    lv_obj_t * parts[2] = { k.clock, k.wrow };
    for(int i = 0; i < 2; i++) {
        if(!parts[i]) continue;
        fx_set(parts[i], FX_SCALE, 150);
        fx_set(parts[i], FX_OPA, 0);
        fx_to(parts[i], FX_SCALE, 256, 0.50f, 0.62f, delay + i * 80, NULL);
        fx_to(parts[i], FX_OPA, 255, 0.25f, 1.0f, delay + i * 80, NULL);
    }
#else
    LV_UNUSED(delay);
#endif
}

/*================ Notifications ================*/

static void ago(time_t t, char * out)
{
    long s = (long)(time(NULL) - t);
    if(s < 60) strcpy(out, "now");
    else if(s < 3600) sprintf(out, "%ldm ago", s / 60);
    else sprintf(out, "%ldh ago", s / 3600);
}

static lv_obj_t * notif_card(lv_obj_t * parent, const notif_t * n, bool previews)
{
    const app_desc_t * app = n->app >= 0 ? os_apps[n->app] : NULL;
    lv_obj_t * card = ui_box(parent);
    lv_obj_set_size(card, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_style_radius(card, 20, 0);
    ui_glass(card, 0x2C2C2E, LV_OPA_60, 24);
    lv_obj_set_style_pad_all(card, 11, 0);
    lv_obj_set_style_pad_column(card, 10, 0);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    lv_obj_t * ic = ui_box(card);
    lv_obj_set_size(ic, 34, 34);
    lv_obj_set_style_radius(ic, 9, 0);
    ui_icon_style(ic, app ? app->color : 0x636366);
    lv_obj_center(ui_label(ic, app ? app->symbol : LV_SYMBOL_BELL, &lv_font_montserrat_16, 0xFFFFFF));

    lv_obj_t * col = ui_box(card);
    lv_obj_set_flex_grow(col, 1);
    lv_obj_set_height(col, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(col, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(col, 1, 0);

    lv_obj_t * head = ui_box(col);
    lv_obj_set_size(head, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_t * t = ui_label(head, n->title, &lv_font_montserrat_14, 0xFFFFFF);
    lv_obj_set_width(t, LV_PCT(72));
    lv_label_set_long_mode(t, LV_LABEL_LONG_MODE_DOTS);
    char a[16];
    ago(n->t, a);
    lv_obj_t * tm = ui_label(head, a, &lv_font_montserrat_12, 0xC7C7CC);
    lv_obj_align(tm, LV_ALIGN_TOP_RIGHT, 0, 1);

    lv_obj_t * b = ui_label(col, previews ? n->body : "Notification", &lv_font_montserrat_12, 0xE5E5EA);
    lv_obj_set_width(b, LV_PCT(100));
    lv_label_set_long_mode(b, LV_LABEL_LONG_MODE_WRAP);
    lv_obj_set_style_max_height(b, 34, 0);
    return card;
}

static void list_build(int pop_index)
{
    k.sw_card = k.sw_btn = NULL;                         /* children are deleted below */
    k.sw_open = false;
    lv_obj_clean(k.list);
    lv_obj_scroll_to_y(k.list, 0, LV_ANIM_OFF);
    if(k.nn == 0) return;

    lv_obj_t * head = ui_box(k.list);
    lv_obj_set_size(head, LV_PCT(100), 22);
    ui_label(head, "Notification Center", &lv_font_montserrat_14, 0xFFFFFF);
    lv_obj_t * clr = ui_box(head);
    lv_obj_set_size(clr, 26, 22);
    lv_obj_align(clr, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_style_radius(clr, 11, 0);
    ui_glass(clr, 0x3A3A3C, LV_OPA_70, 10);
    lv_obj_center(ui_label(clr, LV_SYMBOL_CLOSE, &lv_font_montserrat_12, 0xFFFFFF));
    lv_obj_set_user_data(head, (void *)(intptr_t) - 1);

    int shown = 0;
    for(int i = k.nn - 1; i >= 0; i--, shown++) {
        lv_obj_t * card = notif_card(k.list, &k.n[i], os_settings.previews || !k.locked);
        lv_obj_set_user_data(card, (void *)(intptr_t)(i + 1));
        if(i == pop_index) {
            fx_set(card, FX_Y, -30);
            fx_set(card, FX_OPA, 0);
            fx_to(card, FX_Y, 0, FX_BOUNCY, 0, NULL);
            fx_to(card, FX_OPA, 255, 0.25f, 1.0f, 0, NULL);
#if OS_FX_HEAVY
            fx_set(card, FX_SCALE, 210);
            fx_to(card, FX_SCALE, 256, FX_BOUNCY, 0, NULL);
#endif
        }
    }
}

static void notif_remove(int i)
{
    for(int j = i + 1; j < k.nn; j++) k.n[j - 1] = k.n[j];
    k.nn--;
}

static void list_empty_faded(lv_obj_t * o)
{
    LV_UNUSED(o);
    list_build(-1);
    fx_set(k.list, FX_OPA, 255);
}

int notify_count(void)
{
    return k.nn;
}

void notify_clear(void)
{
    k.nn = 0;
    if(k.list && k.visible && lv_obj_get_child_count(k.list)) {
        uint32_t n = lv_obj_get_child_count(k.list);
        for(uint32_t i = 1; i < n; i++) {
            lv_obj_t * card = lv_obj_get_child(k.list, (int32_t)i);
            if((int)(intptr_t)lv_obj_get_user_data(card) <= 0) continue;
            fx_to(card, FX_Y, -14, 0.28f, 1.0f, 0, NULL);
        }
        fx_to(k.list, FX_OPA, 0, 0.25f, 1.0f, 0, list_empty_faded);
    }
    else if(k.list) list_build(-1);
}

/*---------------- Banner ----------------*/

typedef struct {
    int app;
    int32_t y0, dy;
    bool moved;
    lv_timer_t * timer;
} banner_t;

static void banner_gone_cb(lv_obj_t * o)
{
    lv_obj_delete_async(o);
}

static void banner_dismiss(lv_obj_t * b)
{
    if(!lv_obj_get_user_data(b)) return;
    lv_obj_set_clickable(b, false);
    fx_to(b, FX_Y, -130, 0.32f, 1.0f, 0, banner_gone_cb);
}

static void banner_timer_cb(lv_timer_t * t)
{
    lv_obj_t * b = lv_timer_get_user_data(t);
    banner_t * bn = lv_obj_get_user_data(b);
    bn->timer = NULL;
    lv_timer_delete(t);
    banner_dismiss(b);
}

static void banner_event_cb(lv_event_t * e)
{
    lv_obj_t * b = lv_event_get_current_target_obj(e);
    banner_t * bn = lv_event_get_user_data(e);
    lv_event_code_t code = lv_event_get_code(e);
    lv_point_t p;
    if(code == LV_EVENT_DELETE) {
        if(bn->timer) lv_timer_delete(bn->timer);
        lv_free(bn);
        return;
    }
    lv_indev_t * in = lv_indev_active();
    if(!in) return;
    lv_indev_get_point(in, &p);
    if(code == LV_EVENT_PRESSED) {
        bn->y0 = p.y;
        bn->dy = 0;
        bn->moved = false;
#if OS_FX_HEAVY
        fx_to(b, FX_SCALE, 246, 0.2f, 1.0f, 0, NULL);
#endif
    }
    else if(code == LV_EVENT_PRESSING) {
        bn->dy = p.y - bn->y0;
        if(LV_ABS(bn->dy) > 6) bn->moved = true;
        if(bn->moved) fx_set(b, FX_Y, bn->dy < 0 ? (float)bn->dy : bn->dy / 4.0f);
    }
    else if(code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
#if OS_FX_HEAVY
        fx_to(b, FX_SCALE, 256, 0.4f, 0.55f, 0, NULL);
#endif
        if(bn->moved && bn->dy < -24) {
            banner_dismiss(b);
        }
        else if(bn->moved || code == LV_EVENT_PRESS_LOST) {
            fx_to(b, FX_Y, 0, FX_BOUNCY, 0, NULL);
        }
        else if(bn->app >= 0) {
            lv_area_t a;
            lv_obj_get_coords(b, &a);
            /* Opening from a notification consumes it */
            for(int i = k.nn - 1; i >= 0; i--) {
                if(k.n[i].app == bn->app) {
                    notif_remove(i);
                    break;
                }
            }
            banner_dismiss(b);
            shell_open_app_from(bn->app, &a);
        }
    }
}

static int s_banners;

bool notify_banner_active(void)
{
    return s_banners > 0;
}

static void banner_del_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    if(s_banners > 0) s_banners--;
}

static void banner_show(const notif_t * n)
{
    lv_obj_t * b = ui_box(lv_layer_top());
    lv_obj_set_width(b, k.W - 16);
    lv_obj_set_height(b, LV_SIZE_CONTENT);
    lv_obj_set_pos(b, 8, 8);
    lv_obj_set_overflow_visible(b, true);
    lv_obj_t * card = notif_card(b, n, true);
    lv_obj_set_style_shadow_width(card, 24, 0);
    lv_obj_set_style_shadow_opa(card, LV_OPA_40, 0);
    lv_obj_set_clickable(b, true);
    lv_obj_set_press_lock(b, true);
    banner_t * bn = lv_malloc_zeroed(sizeof(banner_t));
    bn->app = n->app;
    lv_obj_set_user_data(b, bn);
    s_banners++;
    lv_obj_add_event_cb(b, banner_del_cb, LV_EVENT_DELETE, NULL);
    lv_obj_add_event_cb(b, banner_event_cb, LV_EVENT_ALL, bn);

    /* Drops in from above the screen with a little bounce */
    fx_set(b, FX_Y, -120);
    fx_to(b, FX_Y, 0, 0.50f, 0.70f, 0, NULL);
#if OS_FX_HEAVY
    fx_set(b, FX_SCALE, 225);
    fx_to(b, FX_SCALE, 256, 0.50f, 0.65f, 0, NULL);
#endif
    bn->timer = lv_timer_create(banner_timer_cb, BANNER_MS, b);
}

void notify_post(int app_id, const char * title, const char * body)
{
    if(k.nn == NOTIF_MAX) notif_remove(0);
    notif_t * n = &k.n[k.nn++];
    n->app = app_id;
    lv_strncpy(n->title, title ? title : "", sizeof(n->title) - 1);
    n->title[sizeof(n->title) - 1] = '\0';
    lv_strncpy(n->body, body ? body : "", sizeof(n->body) - 1);
    n->body[sizeof(n->body) - 1] = '\0';
    n->t = time(NULL);

    if(k.visible) list_build(k.nn - 1);
    else if(!os_settings.dnd) banner_show(n);
}

/*================ Shade backdrop: blur grows with the pull ================*/

/* The shade blur is a snapshot of the wallpaper, shrunk 4x, box blurred and stretched back up: taken once when
 * the shade starts to show, then only its opacity follows the pull (no per-frame backdrop blur). */
static lv_obj_t * s_shade_img;
static lv_draw_buf_t * s_shade_buf;

static void shade_snap_free(void)
{
    if(s_shade_img) lv_image_set_src(s_shade_img, NULL);
    if(s_shade_buf) {
        lv_image_cache_drop(s_shade_buf);
        lv_draw_buf_destroy(s_shade_buf);
        s_shade_buf = NULL;
    }
}

static void shade_snap_take(void)
{
    shade_snap_free();
    uint32_t n = lv_obj_get_child_count(k.face);
    bool hid[32];
    for(uint32_t i = 0; i < n && i < 32; i++) {
        lv_obj_t * c = lv_obj_get_child(k.face, (int32_t)i);
        hid[i] = lv_obj_is_hidden(c);
        lv_obj_set_hidden(c, true);
    }
    lv_draw_buf_t * snap = lv_snapshot_take(k.face, LV_COLOR_FORMAT_ARGB8888);
    for(uint32_t i = 0; i < n && i < 32; i++) lv_obj_set_hidden(lv_obj_get_child(k.face, (int32_t)i), hid[i]);
    if(!snap) return;
    const int32_t f = 4, r = 3;
    int32_t sw = (int32_t)snap->header.w / f, sh = (int32_t)snap->header.h / f;
    lv_draw_buf_t * out = lv_draw_buf_create(sw, sh, LV_COLOR_FORMAT_ARGB8888, 0);
    uint16_t * acc = lv_malloc((size_t)sw * sh * 3 * sizeof(uint16_t));
    uint16_t * tmp = lv_malloc((size_t)sw * sh * 3 * sizeof(uint16_t));
    if(out && acc && tmp) {
        for(int32_t y = 0; y < sh; y++)
            for(int32_t x = 0; x < sw; x++)
                for(int c = 0; c < 3; c++) {
                    uint32_t s = 0;
                    for(int32_t yy = 0; yy < f; yy++)
                        for(int32_t xx = 0; xx < f; xx++)
                            s += snap->data[(size_t)(y * f + yy) * snap->header.stride + (size_t)(x * f + xx) * 4 + c];
                    acc[(y * sw + x) * 3 + c] = (uint16_t)(s / (f * f));
                }
        for(int pass = 0; pass < 3; pass++) {
            for(int32_t y = 0; y < sh; y++)
                for(int32_t x = 0; x < sw; x++)
                    for(int c = 0; c < 3; c++) {
                        uint32_t s = 0;
                        for(int32_t d = -r; d <= r; d++) s += acc[(y * sw + LV_CLAMP(0, x + d, sw - 1)) * 3 + c];
                        tmp[(y * sw + x) * 3 + c] = (uint16_t)(s / (2 * r + 1));
                    }
            for(int32_t y = 0; y < sh; y++)
                for(int32_t x = 0; x < sw; x++)
                    for(int c = 0; c < 3; c++) {
                        uint32_t s = 0;
                        for(int32_t d = -r; d <= r; d++) s += tmp[(LV_CLAMP(0, y + d, sh - 1) * sw + x) * 3 + c];
                        acc[(y * sw + x) * 3 + c] = (uint16_t)(s / (2 * r + 1));
                    }
        }
        for(int32_t y = 0; y < sh; y++) {
            uint8_t * o = out->data + (size_t)y * out->header.stride;
            for(int32_t x = 0; x < sw; x++, o += 4) {
                o[0] = (uint8_t)acc[(y * sw + x) * 3 + 0];
                o[1] = (uint8_t)acc[(y * sw + x) * 3 + 1];
                o[2] = (uint8_t)acc[(y * sw + x) * 3 + 2];
                o[3] = 255;
            }
        }
        s_shade_buf = out;
        out = NULL;
    }
    if(out) lv_draw_buf_destroy(out);
    lv_free(acc);
    lv_free(tmp);
    lv_draw_buf_destroy(snap);
    if(!s_shade_buf) return;
    if(!s_shade_img) {
        s_shade_img = lv_image_create(k.shade);
        lv_obj_set_size(s_shade_img, k.W, k.H);
        lv_image_set_inner_align(s_shade_img, LV_IMAGE_ALIGN_STRETCH);
        lv_image_set_antialias(s_shade_img, true);
    }
    lv_image_set_src(s_shade_img, s_shade_buf);
}

static void shade_set(float p)
{
    p = LV_CLAMP(0.0f, p, 1.0f);
    if(p > 0.0f && k.shade_p <= 0.0f) shade_snap_take();
    k.shade_p = p;
    if(s_shade_img) lv_obj_set_style_opa(s_shade_img, (lv_opa_t)(255.0f * p), 0);
    lv_obj_set_style_bg_opa(k.shade, (lv_opa_t)(LV_OPA_40 * p), 0);
    lv_obj_set_hidden(k.shade, p <= 0.0f);
    if(p <= 0.0f) shade_snap_free();
}

static void shade_exec(void * o, int32_t v)
{
    LV_UNUSED(o);
    shade_set(v / 1000.0f);
}

static void shade_to(float target, uint32_t delay)
{
    lv_anim_delete(k.shade, shade_exec);
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, k.shade);
    lv_anim_set_exec_cb(&a, shade_exec);
    lv_anim_set_values(&a, (int32_t)(k.shade_p * 1000.0f), (int32_t)(target * 1000.0f));
    lv_anim_set_duration(&a, 280);
    lv_anim_set_delay(&a, delay);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    lv_anim_start(&a);
}

/* Keep the lock face clock visible in Notification Center. */
static void clock_fade(float q)
{
    if(k.clock) fx_set(k.clock, FX_OPA, 255.0f * q);
    if(k.wrow) fx_set(k.wrow, FX_OPA, 255.0f * q);
}

/* The sheet follows the finger by `dy`; the wallpaper blurs a little ahead of it */
static void shade_pose(int32_t dy)
{
    fx_set(k.root, FX_Y, (float)LV_MIN(0, -k.H + dy));
    lv_anim_delete(k.shade, shade_exec);
    shade_set(LV_CLAMP(0.0f, dy / (k.H * 0.4f), 1.0f));
    shell_clock_shade_progress(dy / (k.H * 0.35f));
    clock_fade(1.0f);
}

/*================ Show / hide ================*/

static void hidden_cb(lv_obj_t * o)
{
    if(k.visible) return;
    clock_fade(1.0f);
    lv_obj_set_hidden(o, true);
    lv_obj_set_style_translate_y(o, 0, 0);
}

static void show(bool locked, bool anim_from_top)
{
    k.locked = locked;
    k.visible = true;
    list_fit();
    if(locked) {
        lv_anim_delete(k.shade, shade_exec);
        shade_set(0);
        shell_clock_shade_progress(0.0f);
        clock_fade(1.0f);
    }
    else {
        shade_to(1.0f, 0);
        clock_fade(1.0f);
    }
    lv_obj_set_hidden(k.root, false);
    lv_obj_set_hidden(k.zone, true);
    list_build(-1);
    tick_cb(NULL);
    shell_lock_changed(true);
    home_lock_veil(locked);                    /* only a real lock screen hides the home screen; the pulled-down shade keeps it visible */
    cc_close();
    if(anim_from_top) {
        fx_to(k.root, FX_Y, 0, 0.42f, 0.86f, 0, NULL);
    }
    else {
        fx_set(k.root, FX_Y, 0);
        face_pop_in(0);
    }
}

static void hide(bool unlock)
{
    if(k.editing) edit_exit();
    k.visible = false;
    bool was_locked = k.locked;
    k.locked = false;
    lv_obj_set_hidden(k.zone, false);
    shell_lock_changed(false);
    fx_to(k.root, FX_Y, (float)-k.H, 0.36f, 1.0f, 0, hidden_cb);
    fx_kick(k.root, FX_Y, k.vy * 1000.0f);
    shade_to(0.0f, 60);
    if(unlock && was_locked) home_unlock_anim();
}

void lock_now(void)
{
    if(k.visible && k.locked) return;
    shell_go_home_instant();
    cc_close();
    show(true, false);
}

bool lock_visible(void)
{
    return k.visible;
}

void lock_refresh(void)
{
    if(!k.root) return;
    face_build();
    if(k.edit_name) lv_label_set_text(k.edit_name, k_style_names[os_settings.lock_style]);
}

/*================ Customize ================*/

static void pop(lv_obj_t * o, uint32_t delay)
{
    fx_set(o, FX_SCALE, 1);
    fx_to(o, FX_SCALE, 256, 0.48f, 0.60f, delay, NULL);
}

static void style_step(int dir)
{
    os_settings.lock_style = (uint8_t)((os_settings.lock_style + OS_LOCK_STYLES + dir) % OS_LOCK_STYLES);
    lock_refresh();
    os_save();
    /* New style slides in from the side it was swiped from */
    fx_set(k.clock, FX_X, dir * (float)k.W * 0.6f);
    fx_to(k.clock, FX_X, 0, FX_BOUNCY, 0, NULL);
    pop(k.edit_name, 0);
}

static void color_cb(lv_event_t * e)
{
    os_settings.lock_color = (uint8_t)(intptr_t)lv_event_get_user_data(e);
    clock_build();
#if OS_FX_HEAVY
    pop(k.clock, 0);
#endif
    os_save();
}

static void wall_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    bool has_img = sdk_get_str("wall.lock", "")[0] != '\0';
    int w = os_settings.lock_wall == OS_WALL_CUSTOM ? -1 : os_settings.lock_wall;
    w++;
    if(w >= OS_WALLPAPER_COUNT) w = has_img ? OS_WALL_CUSTOM : 0;
    os_settings.lock_wall = (uint8_t)w;
    face_build();
    os_save();
}

static void done_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    edit_exit();
}

static void edit_grid_build(void);
static void widget_picker_open(int slot);

static void widget_picker_close(lv_event_t * e)
{
    LV_UNUSED(e);
    if(k.widget_picker) lv_obj_delete_async(k.widget_picker);
    k.widget_picker = NULL;
}

static void widget_choose_cb(lv_event_t * e)
{
    int value = (int)(intptr_t)lv_event_get_user_data(e);
    int slot = value / 100 - 1, id = value % 100;
    if(slot < 0 || slot >= 4 || id < 0 || id >= os_widget_count) return;
    os_settings.lock_widget[slot] = (int8_t)id;
    widget_picker_close(NULL);
    wrow_build();
    list_fit();
    edit_grid_build();
    os_save();
}

static void widget_slot_cb(lv_event_t * e)
{
    int slot = (int)(intptr_t)lv_event_get_user_data(e);
    widget_picker_open(slot);
}

static void widget_add_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    for(int slot = 0; slot < 4; slot++) {
        if(slot % 2 && os_settings.lock_widget_span[slot - 1] == 2) continue;
        if(os_settings.lock_widget[slot] < 0) { widget_picker_open(slot); return; }
    }
    sdk_toast("Widgets", "Remove a widget to make room.");
}

static void widget_picker_open(int slot)
{
    widget_picker_close(NULL);
    k.widget_picker = ui_box(k.edit);
    lv_obj_set_size(k.widget_picker, k.W, k.H);
    lv_obj_set_style_bg_color(k.widget_picker, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(k.widget_picker, LV_OPA_60, 0);
    lv_obj_set_clickable(k.widget_picker, true);
    lv_obj_t * panel = ui_card(k.widget_picker);
    lv_obj_set_size(panel, k.W - 24, LV_MIN(k.H - 125, 430));
    lv_obj_align(panel, LV_ALIGN_BOTTOM_MID, 0, -OS_GESTURE_H - 10);
    lv_obj_set_style_radius(panel, 20, 0);
    lv_obj_set_style_pad_all(panel, 14, 0);
    lv_obj_set_style_pad_row(panel, 8, 0);
    lv_obj_set_scrollable(panel, true);
    lv_obj_set_scroll_dir(panel, LV_DIR_VER);
    ui_label(panel, "Add Widget", &lv_font_montserrat_20, 0xFFFFFF);
    for(int id = 0; id < os_widget_count; id++) {
        lv_obj_t * row = ui_box(panel);
        lv_obj_set_size(row, LV_PCT(100), 44);
        lv_obj_set_style_pad_column(row, 10, 0);
        lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_clickable(row, true);
        lv_obj_add_event_cb(row, widget_choose_cb, LV_EVENT_CLICKED,
                            (void *)(intptr_t)((slot + 1) * 100 + id));
        lv_obj_t * ic = ui_box(row);
        lv_obj_set_size(ic, 32, 32);
        lv_obj_set_style_radius(ic, 8, 0);
        ui_icon_style(ic, os_widgets[id]->app ? os_widgets[id]->app->color : UI_SEP);
        lv_obj_center(ui_label(ic, os_widgets[id]->app ? os_widgets[id]->app->symbol : LV_SYMBOL_LIST,
                               &lv_font_montserrat_14, 0xFFFFFF));
        ui_label(row, os_widgets[id]->name, &lv_font_montserrat_16, UI_TEXT);
    }
    lv_obj_t * done = ui_button(panel, "Cancel", UI_SEP, widget_picker_close, NULL);
    lv_obj_set_width(done, LV_PCT(100));
}

static void widget_remove_cb(lv_event_t * e)
{
    int slot = (int)(intptr_t)lv_event_get_user_data(e);
    os_settings.lock_widget[slot] = -1;
    os_settings.lock_widget_span[slot] = 1;
    wrow_build();
    list_fit();
    edit_grid_build();
    os_save();
}

static void widget_size_cb(lv_event_t * e)
{
    int slot = (int)(intptr_t)lv_event_get_user_data(e);
    if(slot % 2) return;
    if(os_settings.lock_widget_span[slot] == 1 && os_settings.lock_widget[slot + 1] >= 0) {
        sdk_toast("Widgets", "Remove the next widget to make this one wider.");
        return;
    }
    os_settings.lock_widget_span[slot] = os_settings.lock_widget_span[slot] == 1 ? 2 : 1;
    wrow_build();
    list_fit();
    edit_grid_build();
    os_save();
}

static void edit_grid_build(void)
{
    if(k.edit_grid) lv_obj_delete_async(k.edit_grid);
    k.edit_grid = ui_box(k.face);
    lv_obj_set_size(k.edit_grid, k.W, 108);
    lv_obj_set_pos(k.edit_grid, 0, k.clock_bottom + 4);
    for(int slot = 0; slot < 4; slot++) {
        if(slot % 2 && os_settings.lock_widget_span[slot - 1] == 2) continue;
        int id = os_settings.lock_widget[slot];
        bool wide = slot % 2 == 0 && os_settings.lock_widget_span[slot] == 2;
        lv_obj_t * cell = ui_box(k.edit_grid);
        lv_obj_set_size(cell, wide ? 264 : 128, 44);
        lv_obj_set_pos(cell, (k.W - 264) / 2 + (slot % 2) * 136, slot / 2 * 54);
        lv_obj_set_style_radius(cell, 10, 0);
        lv_obj_set_style_bg_opa(cell, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(cell, 1, 0);
        lv_obj_set_style_border_color(cell, lv_color_white(), 0);
        lv_obj_set_style_border_opa(cell, LV_OPA_70, 0);
        lv_obj_set_clickable(cell, true);
        lv_obj_add_event_cb(cell, widget_slot_cb, LV_EVENT_CLICKED, (void *)(intptr_t)slot);
        if(id >= 0) {
            if(slot % 2 == 0) {
                lv_obj_t * size = ui_button(cell, wide ? LV_SYMBOL_LEFT : LV_SYMBOL_RIGHT, UI_SEP, widget_size_cb,
                                              (void *)(intptr_t)slot);
                lv_obj_set_size(size, 24, 24);
                lv_obj_align(size, LV_ALIGN_TOP_RIGHT, -27, -8);
            }
            lv_obj_t * remove = ui_button(cell, LV_SYMBOL_CLOSE, UI_SEP, widget_remove_cb,
                                           (void *)(intptr_t)slot);
            lv_obj_set_size(remove, 24, 24);
            lv_obj_align(remove, LV_ALIGN_TOP_RIGHT, 3, -8);
        }
        else lv_obj_center(ui_label(cell, LV_SYMBOL_PLUS, &lv_font_montserrat_16, 0xFFFFFF));
    }
}

static void edit_gone_cb(lv_obj_t * o)
{
    lv_obj_delete_async(o);
}

static void edit_enter(void)
{
    if(k.editing) return;
    k.editing = true;
    lv_obj_set_style_bg_color(k.root, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(k.root, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(k.face, 38, 0);
    lv_obj_set_style_clip_corner(k.face, true, 0);
#if OS_FX_HEAVY
    fx_to(k.face, FX_SCALE, 196, 0.50f, 0.66f, 0, NULL);
#endif
    fx_to(k.face, FX_Y, -18, 0.50f, 0.66f, 0, NULL);
    fx_to(k.list, FX_OPA, 0, 0.2f, 1.0f, 0, NULL);
    fx_to(k.bottom, FX_OPA, 0, 0.2f, 1.0f, 0, NULL);

    k.edit = ui_box(k.root);
    lv_obj_set_size(k.edit, k.W, k.H);

    lv_obj_t * hint = ui_label(k.edit, "Swipe to change the clock", &lv_font_montserrat_12, 0xC7C7CC);
    lv_obj_align(hint, LV_ALIGN_TOP_MID, 0, 22);
    k.edit_name = ui_label(k.edit, k_style_names[os_settings.lock_style], &lv_font_montserrat_16, 0xFFFFFF);
    lv_obj_align(k.edit_name, LV_ALIGN_TOP_MID, 0, 38);

    edit_grid_build();
    lv_obj_t * add = ui_button(k.edit, LV_SYMBOL_PLUS, 0x3A3A3C, widget_add_cb, NULL);
    lv_obj_set_size(add, 34, 30);
    lv_obj_align(add, LV_ALIGN_TOP_LEFT, 14, 18);

    lv_obj_t * colors = ui_box(k.edit);
    lv_obj_set_size(colors, LV_SIZE_CONTENT, 30);
    lv_obj_set_flex_flow(colors, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(colors, 7, 0);
    lv_obj_align(colors, LV_ALIGN_BOTTOM_MID, 0, -66);
    lv_obj_set_style_pad_column(colors, 7, 0);
    for(int i = 0; i < 9; i++) {
        lv_obj_t * d = ui_box(colors);
        lv_obj_set_size(d, 24, 24);
        lv_obj_set_style_radius(d, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_opa(d, LV_OPA_COVER, 0);
        lv_obj_set_style_bg_color(d, lv_color_hex(i == OS_LOCK_COLOR_AUTO ? 0x636366 : k_tints[i]), 0);
        if(i == OS_LOCK_COLOR_AUTO) {                  /* adaptive: colour follows the wallpaper */
            lv_obj_set_style_bg_grad_color(d, lv_color_hex(0xFFD60A), 0);
            lv_obj_set_style_bg_grad_dir(d, LV_GRAD_DIR_HOR, 0);
            lv_obj_center(ui_label(d, "A", &lv_font_montserrat_12, 0x000000));
        }
        lv_obj_set_style_border_color(d, lv_color_white(), 0);
        lv_obj_set_style_border_width(d, 2, LV_STATE_PRESSED);
        lv_obj_set_ext_click_area(d, 3);
        lv_obj_set_clickable(d, true);
        lv_obj_add_event_cb(d, color_cb, LV_EVENT_CLICKED, (void *)(intptr_t)i);
        pop(d, 120 + i * 25);
    }

    lv_obj_t * wall = ui_button(k.edit, LV_SYMBOL_IMAGE "  Wallpaper", 0x3A3A3C, wall_cb, NULL);
    lv_obj_set_size(wall, 140, 36);
    lv_obj_align(wall, LV_ALIGN_BOTTOM_LEFT, 16, -18);
    lv_obj_set_style_radius(wall, 18, 0);
    lv_obj_t * done = ui_button(k.edit, "Done", UI_BLUE, done_cb, NULL);
    lv_obj_set_size(done, 120, 36);
    lv_obj_align(done, LV_ALIGN_BOTTOM_RIGHT, -16, -18);
    lv_obj_set_style_radius(done, 18, 0);
    pop(hint, 60);
    pop(k.edit_name, 80);
    pop(wall, 200);
    pop(done, 240);
}

static void edit_exit(void)
{
    if(!k.editing) return;
    k.editing = false;
    lv_obj_set_style_bg_opa(k.root, LV_OPA_TRANSP, 0);
    lv_obj_set_style_radius(k.face, 0, 0);
    if(k.edit_grid) { lv_obj_delete(k.edit_grid); k.edit_grid = NULL; }
    if(k.edit) {
        lv_obj_t * e = k.edit;
        k.edit = NULL;
        k.edit_name = NULL;
        k.edit_grid = k.widget_picker = NULL;
        fx_to(e, FX_OPA, 0, 0.18f, 1.0f, 0, edit_gone_cb);
    }
    /* Expand back out past full size and settle, then the content pops in */
#if OS_FX_HEAVY
    fx_to(k.face, FX_SCALE, 256, 0.46f, 0.60f, 0, NULL);
#endif
    fx_to(k.face, FX_Y, 0, 0.46f, 0.60f, 0, NULL);
    fx_to(k.list, FX_OPA, 255, 0.3f, 1.0f, 180, NULL);
    fx_to(k.bottom, FX_OPA, 255, 0.3f, 1.0f, 180, NULL);
    face_pop_in(90);
}

/*================ Touch ================*/

static bool hit(lv_obj_t * o, lv_point_t p)
{
    lv_area_t a;
    lv_obj_get_coords(o, &a);
    return p.x >= a.x1 && p.x <= a.x2 && p.y >= a.y1 && p.y <= a.y2;
}

static void tap(lv_point_t p)
{
    if(k.editing) {
        if(hit(k.face, p)) edit_exit();
        return;
    }
    if(hit(k.torch, p)) {
        os_settings.flashlight = !os_settings.flashlight;
        lv_obj_set_style_bg_color(k.torch, lv_color_hex(os_settings.flashlight ? 0xFFFFFF : 0x2C2C2E), 0);
        lv_obj_set_style_text_color(lv_obj_get_child(k.torch, 0), lv_color_hex(os_settings.flashlight ? 0 : 0xFFFFFF), 0);
#if OS_FX_HEAVY
        fx_set(k.torch, FX_SCALE, 300);
        fx_to(k.torch, FX_SCALE, 256, 0.4f, 0.5f, 0, NULL);
#endif
        return;
    }
    if(k.sw_open) {                                     /* a card is resting open: Clear it, or close it */
        if(k.sw_btn && hit(k.sw_btn, p)) swipe_dismiss(fx_get(k.sw_card, FX_X) < 0 ? -1 : 1);
        else swipe_close(true);
        return;
    }
    if(!hit(k.list, p)) return;
    uint32_t n = lv_obj_get_child_count(k.list);
    for(uint32_t i = 0; i < n; i++) {
        lv_obj_t * c = lv_obj_get_child(k.list, (int32_t)i);
        if(c == k.sw_btn || !hit(c, p)) continue;
        int idx = (int)(intptr_t)lv_obj_get_user_data(c);
        if(idx == -1) {
            if(hit(lv_obj_get_child(c, 1), p)) notify_clear();
        }
        else if(idx > 0) {
            int app = k.n[idx - 1].app;
            lv_area_t a;
            lv_obj_get_coords(c, &a);
            notif_remove(idx - 1);
            k.vy = 0;
            hide(true);
            if(app >= 0) shell_open_app_from(app, &a);
        }
        return;
    }
}

/*---------------- Swipe a notification sideways: Clear pops out, drag all the way to delete ----------------*/

#define CLEAR_REST      84      /* how far a card stays open */

static void swipe_close(bool animate)
{
    if(k.sw_card) {
        if(animate) fx_to(k.sw_card, FX_X, 0, FX_BOUNCY, 0, NULL);
        else fx_set(k.sw_card, FX_X, 0);
    }
    if(k.sw_btn) {
        lv_obj_delete(k.sw_btn);
        k.sw_btn = NULL;
    }
    k.sw_card = NULL;
    k.sw_open = false;
}

static lv_obj_t * card_at(lv_point_t p)
{
    uint32_t n = lv_obj_get_child_count(k.list);
    for(uint32_t i = 0; i < n; i++) {
        lv_obj_t * c = lv_obj_get_child(k.list, (int32_t)i);
        if(c == k.sw_btn) continue;
        if((int)(intptr_t)lv_obj_get_user_data(c) > 0 && hit(c, p)) return c;
    }
    return NULL;
}

/* Place the red pill in the gap the card left behind; it widens with the drag and pops when it can be released */
static void swipe_layout(int32_t dx)
{
    if(!k.sw_card) return;
    if(!k.sw_btn) {
        k.sw_btn = ui_box(k.list);
        lv_obj_set_floating(k.sw_btn, true);
        lv_obj_set_style_radius(k.sw_btn, 20, 0);
        lv_obj_set_style_bg_opa(k.sw_btn, LV_OPA_COVER, 0);
        lv_obj_set_style_bg_color(k.sw_btn, lv_color_hex(0xFF453A), 0);
        lv_obj_set_clickable(k.sw_btn, false);
        lv_obj_t * l = ui_label(k.sw_btn, LV_SYMBOL_TRASH "  Clear", &lv_font_montserrat_14, 0xFFFFFF);
        lv_obj_center(l);
        k.sw_armed = false;
    }
    int32_t cw = lv_obj_get_width(k.sw_card), ch = lv_obj_get_height(k.sw_card);
    int32_t gap = LV_ABS(dx) - 6;
    if(gap < 0) gap = 0;
    lv_obj_set_size(k.sw_btn, gap, ch);
    lv_obj_set_pos(k.sw_btn, lv_obj_get_x(k.sw_card) + (dx < 0 ? cw - gap : 0), lv_obj_get_y(k.sw_card));
    lv_obj_set_style_opa(lv_obj_get_child(k.sw_btn, 0), gap > 70 ? LV_OPA_COVER : LV_OPA_TRANSP, 0);
    bool armed = LV_ABS(dx) > cw * 55 / 100;
    if(armed != k.sw_armed) {                           /* crossing the release point: the label pops */
        k.sw_armed = armed;
        lv_obj_t * l = lv_obj_get_child(k.sw_btn, 0);
#if OS_FX_HEAVY
        fx_set(l, FX_SCALE, armed ? 340 : 200);
        fx_to(l, FX_SCALE, 256, 0.34f, 0.45f, 0, NULL);
#endif
        LV_UNUSED(l);
    }
}

static unsigned dismiss_pending;
static void swipe_gone_cb(lv_obj_t * o)
{
    if(dismiss_pending) dismiss_pending--;
    lv_obj_delete_async(o);
}

static void swipe_dismiss(int dir)
{
    lv_obj_t * c = k.sw_card;
    if(!c) return;
    int idx = (int)(intptr_t)lv_obj_get_user_data(c) - 1;
    if(idx < 0 || idx >= k.nn) return;
    int32_t oldy[NOTIF_MAX];
    for(int i = 0; i < NOTIF_MAX; i++) oldy[i] = INT32_MIN;
    uint32_t n = lv_obj_get_child_count(k.list);
    for(uint32_t i = 0; i < n; i++) {
        lv_obj_t * item = lv_obj_get_child(k.list, (int32_t)i);
        int id = (int)(intptr_t)lv_obj_get_user_data(item);
        if(id > 0 && id <= NOTIF_MAX && item != k.sw_btn) oldy[id - 1] = lv_obj_get_y(item);
    }
    int32_t scroll = lv_obj_get_scroll_y(k.list);
    lv_area_t pos, root_pos;
    lv_obj_get_coords(c, &pos);
    lv_obj_get_coords(k.root, &root_pos);
    lv_obj_set_parent(c, k.root);
    lv_obj_set_size(c, lv_area_get_width(&pos), lv_area_get_height(&pos));
    lv_obj_set_pos(c, pos.x1 - root_pos.x1, pos.y1 - root_pos.y1);
    lv_obj_set_floating(c, true);
    lv_obj_move_foreground(c);
    k.sw_card = NULL;
    if(k.sw_btn) {
        lv_obj_delete(k.sw_btn);
        k.sw_btn = NULL;
    }
    k.sw_open = false;
    notif_remove(idx);
    if(k.nn == 0) {
        fx_to(k.list, FX_OPA, 0, 0.25f, 1.0f, 0, list_empty_faded);
    }
    else {
        list_build(-1);
        lv_obj_update_layout(k.list);
        lv_obj_scroll_to_y(k.list, LV_MIN(scroll, list_max_scroll()), LV_ANIM_OFF);
        n = lv_obj_get_child_count(k.list);
        for(uint32_t i = 0; i < n; i++) {
            lv_obj_t * item = lv_obj_get_child(k.list, (int32_t)i);
            int id = (int)(intptr_t)lv_obj_get_user_data(item);
            if(id <= 0) continue;
            int was = id - 1 < idx ? id - 1 : id;
            if(was >= NOTIF_MAX || oldy[was] == INT32_MIN) continue;
            int32_t dy = oldy[was] - lv_obj_get_y(item);
            if(dy) {
                fx_set(item, FX_Y, (float)dy);
                fx_to(item, FX_Y, 0, 0.34f, 1.0f, 0, NULL);
            }
        }
    }
    dismiss_pending++;
    fx_to(c, FX_X, (float)dir * (float)(k.W + 20), 0.32f, 1.0f, 0, swipe_gone_cb);
    fx_to(c, FX_OPA, 0, 0.25f, 1.0f, 0, NULL);
}

static void scroll_exec(void * o, int32_t v)
{
    lv_obj_scroll_to_y((lv_obj_t *)o, v, LV_ANIM_OFF);
}

static int32_t list_max_scroll(void)
{
    return lv_obj_get_scroll_y(k.list) + lv_obj_get_scroll_bottom(k.list);
}

static void root_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_indev_t * in = lv_indev_active();
    if(!in) return;
    lv_point_t p;
    lv_indev_get_point(in, &p);

    if(code == LV_EVENT_PRESSED) {
        k.p0 = k.last = p;
        k.t0 = k.last_tick = lv_tick_get();
        k.vx = k.vy = 0;
        k.mode = M_PRESS;
        fx_stop(k.root);
        lv_anim_delete(k.list, scroll_exec);
        return;
    }
    if(code == LV_EVENT_PRESSING) {
        uint32_t el = lv_tick_elaps(k.last_tick);
        if(el) {
            k.vx = 0.6f * k.vx + 0.4f * (p.x - k.last.x) / (float)el;
            k.vy = 0.6f * k.vy + 0.4f * (p.y - k.last.y) / (float)el;
            k.last = p;
            k.last_tick = lv_tick_get();
        }
        int32_t dx = p.x - k.p0.x, dy = p.y - k.p0.y;
        bool moved = LV_ABS(dx) > SLOP || LV_ABS(dy) > SLOP;
        if(k.mode == M_PRESS) {
            if(k.editing) {
                if(moved) k.mode = LV_ABS(dx) > LV_ABS(dy) ? M_EDIT_SWIPE : M_NONE;
            }
            else if(moved) {
                bool vert = LV_ABS(dy) > LV_ABS(dx);
                lv_obj_t * swc = !vert && k.nn > 0 && hit(k.list, k.p0) ? card_at(k.p0) : NULL;
                int32_t sy = lv_obj_get_scroll_y(k.list);
                bool can = k.nn > 0 && hit(k.list, k.p0) && ((dy < 0 && list_max_scroll() > sy) || (dy > 0 && sy > 0));
                if(swc) {                   /* sideways on a card: swipe it */
                    if(k.sw_card && k.sw_card != swc) swipe_close(true);
                    k.sw_card = swc;
                    k.scroll0 = (int32_t)fx_get(swc, FX_X);
                    k.mode = M_CARD;
                }
                else if(vert && can) {      /* the notification list takes the drag while it can still scroll */
                    k.mode = M_LIST;
                    k.scroll0 = sy;
                }
                else k.mode = dy < 0 && vert ? M_SLIDE : M_NONE;
                if(k.sw_open && (k.mode == M_SLIDE || k.mode == M_LIST)) swipe_close(true);
            }
            else if(lv_tick_elaps(k.t0) > LONG_PRESS_MS && k.locked) {
                k.mode = M_NONE;
                edit_enter();
            }
        }
        if(k.mode == M_SLIDE) {
            fx_set(k.root, FX_Y, (float)LV_MIN(0, dy));
#if OS_FX_HEAVY
            float q = LV_CLAMP(0.0f, -dy / (float)k.H, 1.0f);
            fx_set(k.face, FX_OPA, 255.0f * (1.0f - q * 0.6f));
#endif
        }
        else if(k.mode == M_CARD && k.sw_card) {
            int32_t x = k.scroll0 + dx;
            fx_set(k.sw_card, FX_X, (float)x);
            swipe_layout(x);
        }
        else if(k.mode == M_LIST) {
            lv_obj_scroll_to_y(k.list, LV_CLAMP(0, k.scroll0 - dy, list_max_scroll()), LV_ANIM_OFF);
        }
        else if(k.mode == M_EDIT_SWIPE) {
            fx_set(k.clock, FX_X, (float)dx);
        }
        return;
    }
    if(code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
        int32_t dx = k.last.x - k.p0.x, dy = k.last.y - k.p0.y;
        lmode_t m = k.mode;
        k.mode = M_IDLE;
        if(m == M_SLIDE) {
#if OS_FX_HEAVY
            fx_to(k.face, FX_OPA, 255, 0.3f, 1.0f, 0, NULL);
#endif
            if(dy < -k.H / 4 || k.vy < -0.5f) hide(true);
            else {
                fx_to(k.root, FX_Y, 0, FX_BOUNCY, 0, NULL);
                fx_kick(k.root, FX_Y, k.vy * 1000.0f);
            }
        }
        else if(m == M_CARD && k.sw_card) {
            int32_t x = (int32_t)fx_get(k.sw_card, FX_X);
            int32_t cw = lv_obj_get_width(k.sw_card);
            int dir = x < 0 ? -1 : 1;
            bool flick = LV_ABS(x) > 40 && ((k.vx < -0.8f && dir < 0) || (k.vx > 0.8f && dir > 0));
            if(LV_ABS(x) > cw * 55 / 100 || flick) swipe_dismiss(dir);
            else if(LV_ABS(x) > 44) {                   /* rest open with the pill showing */
                fx_to(k.sw_card, FX_X, (float)(dir * CLEAR_REST), FX_BOUNCY, 0, NULL);
                swipe_layout(dir * CLEAR_REST);
                k.sw_open = true;
            }
            else swipe_close(true);
        }
        else if(m == M_LIST) {                      /* fling with the finger's velocity */
            int32_t y = lv_obj_get_scroll_y(k.list);
            int32_t to = LV_CLAMP(0, y - (int32_t)(k.vy * 380.0f), list_max_scroll());
            lv_anim_t a;
            lv_anim_init(&a);
            lv_anim_set_var(&a, k.list);
            lv_anim_set_exec_cb(&a, scroll_exec);
            lv_anim_set_values(&a, y, to);
            lv_anim_set_duration(&a, 120 + (uint32_t)LV_MIN(500, LV_ABS(to - y) * 2));
            lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
            lv_anim_start(&a);
        }
        else if(m == M_EDIT_SWIPE) {
            if(dx < -60 || k.vx < -0.4f) style_step(1);
            else if(dx > 60 || k.vx > 0.4f) style_step(-1);
            else fx_to(k.clock, FX_X, 0, FX_BOUNCY, 0, NULL);
        }
        else if(m == M_PRESS && code == LV_EVENT_RELEASED) {
            tap(p);
        }
    }
}

/* Pull down from the top-left edge: the lock screen slides down as Notification Center */
static void zone_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_point_t p;
    lv_indev_get_point(lv_indev_active(), &p);
    if(code == LV_EVENT_PRESSED) {
        k.p0 = k.last = p;
        k.last_tick = lv_tick_get();
        k.vy = 0;
        lv_obj_set_hidden(k.root, false);
        list_fit();
        list_build(-1);
        tick_cb(NULL);
        fx_set(k.root, FX_Y, (float)-k.H);
        lv_anim_delete(k.shade, shade_exec);
        shade_set(0);
        shell_clock_shade_progress(0.0f);
    }
    else if(code == LV_EVENT_PRESSING) {
        uint32_t el = lv_tick_elaps(k.last_tick);
        if(el) {
            k.vy = 0.6f * k.vy + 0.4f * (p.y - k.last.y) / (float)el;
            k.last = p;
            k.last_tick = lv_tick_get();
        }
        shade_pose(p.y - k.p0.y);
    }
    else if(code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
        int32_t dy = k.last.y - k.p0.y;
        if(dy > 90 || k.vy > 0.4f) {
            show(false, true);
            fx_kick(k.root, FX_Y, k.vy * 1000.0f);
        }
        else {
            k.visible = false;
            shade_to(0.0f, 0);
            shell_clock_shade_progress(0.0f);
            fx_to(k.root, FX_Y, (float)-k.H, 0.3f, 1.0f, 0, hidden_cb);
        }
    }
}

/*================ Init ================*/

static lv_obj_t * round_btn(lv_obj_t * parent, const char * sym, lv_align_t align, int32_t x)
{
    lv_obj_t * b = ui_box(parent);
    lv_obj_set_size(b, 50, 50);
    lv_obj_align(b, align, x, -34);
    lv_obj_set_style_radius(b, LV_RADIUS_CIRCLE, 0);
    ui_glass(b, 0x2C2C2E, LV_OPA_60, 16);
    lv_obj_center(ui_label(b, sym, &lv_font_montserrat_20, 0xFFFFFF));
    return b;
}

void lock_init(void)
{
    k.W = lv_display_get_horizontal_resolution(lv_display_get_default());
    k.H = lv_display_get_vertical_resolution(lv_display_get_default());

    k.root = ui_box(lv_layer_top());
    lv_obj_move_to_index(k.root, 0);    /* below the status bar, Control Center and banners */
    lv_obj_set_size(k.root, k.W, k.H);
    lv_obj_set_clickable(k.root, true);
    lv_obj_set_press_lock(k.root, true);
    lv_obj_add_event_cb(k.root, root_cb, LV_EVENT_ALL, NULL);

    k.face = ui_box(k.root);
    lv_obj_set_size(k.face, k.W, k.H);
    lv_obj_set_style_bg_opa(k.face, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_grad_dir(k.face, LV_GRAD_DIR_VER, 0);

    k.list = ui_box(k.root);
    lv_obj_set_size(k.list, k.W - 20, 100);
    lv_obj_set_x(k.list, 10);
    lv_obj_set_scrollable(k.list, true);
    lv_obj_set_scroll_dir(k.list, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(k.list, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scroll_elastic(k.list, false);
    lv_obj_set_scroll_momentum(k.list, false);
    lv_obj_set_style_pad_bottom(k.list, 16, 0);
    lv_obj_set_flex_flow(k.list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(k.list, 8, 0);

    k.bottom = ui_box(k.root);
    lv_obj_set_size(k.bottom, k.W, 110);
    lv_obj_align(k.bottom, LV_ALIGN_BOTTOM_MID, 0, 0);
    k.torch = round_btn(k.bottom, LV_SYMBOL_CHARGE, LV_ALIGN_BOTTOM_LEFT, 34);
    round_btn(k.bottom, LV_SYMBOL_IMAGE, LV_ALIGN_BOTTOM_RIGHT, -34);
    lv_obj_t * bar = ui_box(k.bottom);
    lv_obj_set_size(bar, 120, 5);
    lv_obj_set_style_radius(bar, 3, 0);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(bar, lv_color_white(), 0);
    lv_obj_align(bar, LV_ALIGN_BOTTOM_MID, 0, -8);
    lv_obj_t * hint = ui_label(k.bottom, "Swipe up to open  " LV_SYMBOL_UP, &lv_font_montserrat_12, 0xE5E5EA);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -20);

    k.shade = ui_box(k.face);                       /* wallpaper blur for the shade; the clock stays crisp above it */
    lv_obj_set_size(k.shade, k.W, k.H);
    lv_obj_set_style_bg_color(k.shade, lv_color_black(), 0);
    lv_obj_move_to_index(k.shade, 0);
    shade_set(0);

    face_build();

    k.zone = ui_box(lv_layer_top());
    lv_obj_set_size(k.zone, k.W / 2, PULL_ZONE_H);
    lv_obj_align(k.zone, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_clickable(k.zone, true);
    lv_obj_set_press_lock(k.zone, true);
    lv_obj_set_gesture_bubble(k.zone, false);
    lv_obj_add_event_cb(k.zone, zone_cb, LV_EVENT_ALL, NULL);

    k.tick = lv_timer_create(tick_cb, 1000, NULL);

    /* Boot to the lock screen with a welcome notification */
    show(true, false);
    notify_post(-1, OS_NAME, "Swipe up to unlock. Hold to customize the Lock Screen.");
}

/*================ Simulator test hooks ================*/

void lock_debug_unlock(void)
{
    k.vy = 0;
    if(k.visible) hide(true);
}

void lock_debug_shade(float p)
{
    lv_obj_set_hidden(k.root, false);
    lv_obj_set_hidden(k.zone, true);
    k.locked = false;
    list_fit();
    list_build(-1);
    shade_pose((int32_t)(p * k.H));
}

void s3_notify_open(bool open)
{
 if(open && !k.visible) { fx_set(k.root, FX_Y, -k.H); show(false, true); }
 else if(!open && k.visible) { k.vy = 0; hide(false); }
}
bool s3_notify_busy(void)
{
 if(dismiss_pending || (k.list && fx_busy(k.list, FX_OPA))) return true;
 if(k.list) for(uint32_t i = 0; i < lv_obj_get_child_count(k.list); i++) {
  lv_obj_t * card = lv_obj_get_child(k.list, i);
  if(fx_busy(card, FX_Y) || fx_busy(card, FX_OPA)) return true;
 }
 return false;
}
void s3_notify_clear_one(void)
{
 if(!k.list) return;
 for(uint32_t i = 0; i < lv_obj_get_child_count(k.list); i++) {
  lv_obj_t * card = lv_obj_get_child(k.list, i);
  int id = (int)(intptr_t)lv_obj_get_user_data(card);
  if(id > 0 && id <= k.nn && card != k.sw_btn) {
   k.sw_card = card; swipe_dismiss(-1); return;
  }
 }
}

bool s3_shade_busy(void)
{
 return (k.root && fx_busy(k.root, FX_Y)) || (k.shade && lv_anim_get(k.shade, shade_exec));
}
