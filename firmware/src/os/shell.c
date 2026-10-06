/**
 * @file shell.c
 * Window manager: icon <-> window spring morph, pill-less gesture navigation, recents.
 *
 * System gestures live in an invisible strip at the bottom edge (no home indicator):
 *   in an app:   swipe up -> home        swipe up and hold -> recents
 *   on home:     swipe up and hold -> recents (swipe up while jiggling -> done)
 *   in recents:  swipe up -> home
 *   sideways on the bottom edge (in an app): switch to the previous / next app
 *
 * Launch sources decide the exit: icons and widgets morph back into themselves; apps opened from
 * Control Center, Spotlight or a notification fade out instead (there is nothing to fly back to).
 */

/*********************
 *      INCLUDES
 *********************/

#include <math.h>
#include <time.h>
#include <string.h>
#include "sdk.h"

extern const app_desc_t app_clock;

/* Not exported through the public headers, but needed to forget a snapshot before freeing it */
void lv_image_cache_drop(const void * src);

/*********************
 *      DEFINES
 *********************/

#define ICON_RADIUS         14
#define CARD_SCALE          0.6f
#define CARD_RADIUS         16
#define CARD_GAP            18

static int s_rec_style;                      /* 0 = carousel, 1 = stacked depth deck */
static int32_t rec_gap(void);
#define MRU_MAX             8
#define PHYS_PERIOD_MS      8
#define PHYS_SUBSTEP_S      0.004f
#define FRAME_PERIOD_MS     16

                                                                    /* Gesture tuning (px/ms for velocities) */
#define DRAG_FULL_TRAVEL    0.55f                                                           /* fraction of screen height for progress = 1 */
#define DRAG_MIN_SCALE      0.45f
#define HOLD_MIN_PROGRESS   0.12f
#define HOLD_MAX_SPEED      0.12f
#define HOLD_TIME_MS        120
#define FLING_HOME_VY       -0.35f
#define FLING_CANCEL_VY     0.25f
#define HOME_PROGRESS       0.30f
#define CARD_DISMISS_DY     -90
#define SWIPE_UP_DY         -24                                                  /* recents -> home, home jiggle -> done */
#define HOME_HOLD_DY        -36                                               /* lift needed on home before a hold arms recents */

/**********************
 *      TYPEDEFS
 **********************/

typedef enum {
    ST_HOME,
    ST_OPENING,
    ST_APP,
    ST_DRAG,
    ST_SNAPBACK,
    ST_CLOSING,
    ST_TO_RECENTS,
    ST_RECENTS,
    ST_FADING,
} ui_state_t;

typedef enum { SRC_ICON, SRC_WIDGET, SRC_FADE, SRC_SLIDE } launch_src_t;

typedef enum { SP_CX, SP_CY, SP_W, SP_H, SP_R, SP_COUNT } spring_axis_t;

typedef struct {
    float x;    /* value */
    float v;    /* velocity, units/s */
    float t;    /* target */
} spring_t;

typedef struct {
    lv_point_t p0;          /* finger at press */
    lv_point_t last;
    uint32_t last_tick;
    float vx, vy;           /* px/ms, smoothed */
    uint32_t still_ms;
} track_t;

/**********************
 *  STATIC PROTOTYPES
 **********************/

static void spring_config(float response_s, float damping);
static void spring_set_target(float cx, float cy, float w, float h, float r);
static void spring_set_target_area(const lv_area_t * a, float r);
static void spring_snap_area(const lv_area_t * a, float r);
static bool spring_step(float dt);
static void phys_timer_cb(lv_timer_t * t);

static void set_state(ui_state_t s);
static void apply_card(void);
static void on_settle(void);

static void app_launch(int id, const lv_area_t * from, float from_r, launch_src_t src);
static void app_close_fade(void);
static void quick_switch(int dir);
static void pill_refresh(void);
static void edge_refresh(void);
static void bar_color_timer_cb(lv_timer_t * t);
static void edge_event_cb(lv_event_t * e);
static void app_reopen(void);
static void app_close_to_home(void);
static void app_destroy(void);

static void track_begin(void);
static uint32_t track_update(lv_point_t * p);

static void drag_begin(void);
static void drag_update(void);
static void drag_end(void);
static void home_hold_update(void);
static void home_hold_end(bool released);

static lv_obj_t * recents_build(int focus_id);
static void recents_enter_from_drag(void);
static void recents_open_from_home(void);
static void recents_dismiss(void);
static void recents_launch(lv_obj_t * card, int id);
static lv_obj_t * recents_item_create(lv_obj_t * parent, int id);

static void snap_take(int id);
static void snap_free(int id);
static void mru_touch(int id);
static void mru_remove(int id);

static void dim_to(lv_opa_t opa);
static void statusbar_refresh(void);

static void frame_event_cb(lv_event_t * e);
static void zone_event_cb(lv_event_t * e);
static void recents_bg_event_cb(lv_event_t * e);
static void recents_card_event_cb(lv_event_t * e);
static void clock_timer_cb(lv_timer_t * t);

/**********************
 *  STATIC VARIABLES
 **********************/

static const uint32_t k_wallpapers[OS_WALLPAPER_COUNT][2] = {
    { 0x141E30, 0x35577D },
    { 0x2B1055, 0x7597DE },
    { 0x0F2027, 0x2C5364 },
    { 0x3A1C71, 0xD76D77 },
    { 0x000000, 0x434343 },
    { 0xF7797D, 0x4A1A8C },
};

static struct {
    int32_t W, H;
    int32_t card_w, card_h;
    ui_state_t state;

    spring_t sp[SP_COUNT];
    float k, c;
    bool animating;
    uint32_t last_tick;

    lv_obj_t * frame;
    lv_obj_t * content;
    lv_obj_t * overlay;
    lv_obj_t * overlay_sym;
    int app_id;

    lv_obj_t * dim;
    lv_obj_t * rec;
    lv_obj_t * row;
    lv_obj_t * handoff;

    lv_obj_t * zone;
    lv_obj_t * edge;                    /* left edge strip: swipe in = back */
    float bar_mix;                      /* 0 = white status bar, 1 = dark (bright app behind) */
    float nav_mix;
    lv_obj_t * clock;
    lv_obj_t * sys;
    lv_obj_t * bright;

    track_t t;
    float cx0, bottom0, s0; /* card state at press (grab where it is) */
    float prog;
    bool armed;             /* hold detected -> release opens recents */

    int8_t mru[MRU_MAX];    /* oldest first */
    int mru_n;
    lv_draw_buf_t * snap[OS_MAX_APPS];

    launch_src_t src;
    lv_obj_t * src_obj;     /* widget the app came from */
    lv_draw_buf_t * src_snap;
    float src_w;            /* width the overlay starts fading at */
    lv_obj_t * pill;
    bool locked, bar_visible;
    lv_draw_buf_t * wall_img[2];
    char wall_path[2][260];
} g;

/* Recents card drag (only one pointer) */
static int32_t s_card_y0;
static int32_t s_card_dy;
static bool s_card_dragged;

/**********************
 *      MACROS
 **********************/

#define CLAMPF(v, lo, hi) ((v) < (lo) ? (lo) : ((v) > (hi) ? (hi) : (v)))

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void shell_init(void)
{
    lv_display_t * disp = lv_display_get_default();
    lv_obj_t * scr = lv_screen_active();

    lv_memzero(&g, sizeof(g));
    g.W = lv_display_get_horizontal_resolution(disp);
    g.H = lv_display_get_vertical_resolution(disp);
    g.card_w = (int32_t)lroundf(g.W * CARD_SCALE);
    g.card_h = (int32_t)lroundf(g.H * CARD_SCALE);
    g.app_id = -1;

    lv_theme_t * th = lv_theme_default_init(disp, lv_color_hex(UI_BLUE), lv_color_hex(UI_RED), true,
                                            &lv_font_montserrat_14);
    lv_display_set_theme(disp, th);

    /* 60 fps refresh + input for 1:1 finger tracking */
    lv_timer_t * refr = lv_display_get_refr_timer(disp);
    if(refr) lv_timer_set_period(refr, FRAME_PERIOD_MS);
    for(lv_indev_t * in = lv_indev_get_next(NULL); in; in = lv_indev_get_next(in)) {
        lv_timer_t * rt = lv_indev_get_read_timer(in);
        if(rt) lv_timer_set_period(rt, FRAME_PERIOD_MS);
    }

    os_load();

    lv_obj_clean(scr);
    lv_obj_set_scrollable(scr, false);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_grad_dir(scr, LV_GRAD_DIR_VER, 0);

    home_create(scr);

    /* Dim layer: switcher backdrop (above home screen, below recents/app frame) */
    g.dim = ui_box(scr);
    lv_obj_set_size(g.dim, g.W, g.H);
    lv_obj_set_style_bg_color(g.dim, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(g.dim, LV_OPA_TRANSP, 0);
    lv_obj_set_hidden(g.dim, true);

    /* Status bar (top layer, always above apps) */
    g.clock = ui_label(lv_layer_top(), "", &lv_font_montserrat_14, 0xFFFFFF);
    lv_obj_set_pos(g.clock, 26, 12);
    g.sys = statusbar_icons_create(lv_layer_top());

    /* Invisible system gesture strip: there is no home indicator pill */
    g.zone = ui_box(lv_layer_top());
    lv_obj_set_size(g.zone, g.W, OS_GESTURE_H);
    lv_obj_align(g.zone, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_clickable(g.zone, true);
    lv_obj_set_press_lock(g.zone, true);
    lv_obj_set_gesture_bubble(g.zone, false);
    lv_obj_add_event_cb(g.zone, zone_event_cb, LV_EVENT_ALL, NULL);

    g.edge = ui_box(lv_layer_top());
    lv_obj_set_size(g.edge, os_settings.edge_size, g.H - 100 - OS_GESTURE_H);
    lv_obj_set_pos(g.edge, 0, 72);
    lv_obj_set_clickable(g.edge, true);
    lv_obj_set_press_lock(g.edge, true);
    lv_obj_set_gesture_bubble(g.edge, false);
    lv_obj_add_event_cb(g.edge, edge_event_cb, LV_EVENT_ALL, NULL);
    lv_obj_set_hidden(g.edge, true);

    g.pill = ui_box(g.zone);
    lv_obj_set_size(g.pill, 118, 5);
    lv_obj_align(g.pill, LV_ALIGN_BOTTOM_MID, 0, -7);
    lv_obj_set_style_radius(g.pill, 3, 0);
    lv_obj_set_style_bg_opa(g.pill, LV_OPA_80, 0);
    lv_obj_set_style_bg_color(g.pill, lv_color_white(), 0);
    lv_obj_set_hidden(g.pill, true);
    g.bar_visible = true;

    /* Brightness: a black veil under the mouse cursor on the system layer */
    g.bright = ui_box(lv_layer_sys());
    lv_obj_set_size(g.bright, g.W, g.H);
    lv_obj_set_style_bg_color(g.bright, lv_color_black(), 0);
    lv_obj_move_background(g.bright);

    shell_settings_apply();
    clock_timer_cb(NULL);
    lv_timer_create(clock_timer_cb, 1000, NULL);
    lv_timer_create(bar_color_timer_cb, 33, NULL);

    set_state(ST_HOME);
    cc_init();
    lock_init();
    import_init();

    g.last_tick = lv_tick_get();
    lv_timer_create(phys_timer_cb, PHYS_PERIOD_MS, NULL);
}

void shell_wallpaper_colors_lock(int idx, uint32_t * top, uint32_t * bottom)
{
    shell_wallpaper_colors(idx, top, bottom);
}

lv_draw_buf_t * shell_wallpaper_image(bool lock)
{
    int i = lock ? 1 : 0;
    const char * path = sdk_get_str(lock ? "wall.lock" : "wall.home", "");
    if(strcmp(path, g.wall_path[i]) != 0 || (!g.wall_img[i] && path[0])) {
        if(g.wall_img[i]) {
            if(!lock) lv_obj_set_style_bg_image_src(lv_screen_active(), NULL, 0);
            lv_image_cache_drop(g.wall_img[i]);
            lv_draw_buf_destroy(g.wall_img[i]);
            g.wall_img[i] = NULL;
        }
        lv_strncpy(g.wall_path[i], path, sizeof(g.wall_path[i]) - 1);
        if(path[0]) g.wall_img[i] = sdk_image_load(path, g.W, g.H);
    }
    return g.wall_img[i];
}

void shell_lock_changed(bool visible)
{
    g.locked = visible;
    home_lock_veil(visible);
    edge_refresh();
    lv_obj_set_style_text_opa(g.clock, LV_OPA_COVER, 0);
    lv_obj_set_hidden(g.clock, visible || !g.bar_visible);
    lv_obj_set_hidden(g.zone, visible || g.state == ST_CLOSING || g.state == ST_TO_RECENTS || g.state == ST_FADING);
    cc_set_enabled(visible || g.bar_visible);
}

void shell_clock_shade_progress(float progress)
{
    lv_obj_set_style_text_opa(g.clock, (lv_opa_t)(255.0f * (1.0f - LV_CLAMP(0.0f, progress, 1.0f))), 0);
}

/* Lock Now: every app goes away at once so the unlock animation lands on the Home Screen */
void shell_go_home_instant(void)
{
    if(g.rec) recents_dismiss();
    if(g.frame) app_destroy();
    g.animating = false;
    dim_to(LV_OPA_TRANSP);
    set_state(ST_HOME);
}

void shell_launch_from_widget(int id, lv_obj_t * widget_box)
{
    if(g.state != ST_HOME && g.state != ST_CLOSING && g.state != ST_FADING) return;
    lv_area_t a;
    lv_obj_get_coords(widget_box, &a);
    app_destroy();
    g.src_obj = widget_box;
    g.src_snap = lv_snapshot_take(widget_box, LV_COLOR_FORMAT_ARGB8888);
    app_launch(id, &a, 22, SRC_WIDGET);
}

void shell_open_app_from(int id, const lv_area_t * from)
{
    if(id < 0 || id >= os_app_count) return;
    cc_close();
    if(g.state == ST_RECENTS || g.state == ST_TO_RECENTS) {
        if(g.rec) lv_obj_delete_async(g.rec);
        g.rec = g.row = NULL;
        g.handoff = NULL;
        dim_to(LV_OPA_TRANSP);
    }
    if(g.app_id == id && (g.state == ST_APP || g.state == ST_OPENING)) return;
    lv_area_t a;
    if(from) a = *from;
    else lv_area_set(&a, g.W / 2 - 30, g.H / 2 - 30, g.W / 2 + 29, g.H / 2 + 29);
    app_launch(id, &a, ICON_RADIUS, SRC_FADE);
}

void shell_open_app(int id)
{
    shell_open_app_from(id, NULL);
}

void shell_wallpaper_colors(int idx, uint32_t * top, uint32_t * bottom)
{
    idx = LV_CLAMP(0, idx, OS_WALLPAPER_COUNT - 1);
    *top = k_wallpapers[idx][0];
    *bottom = k_wallpapers[idx][1];
}

void shell_settings_apply(void)
{
    uint32_t top, bottom;
    shell_wallpaper_colors(os_settings.wallpaper, &top, &bottom);
    lv_obj_t * scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, lv_color_hex(top), 0);
    lv_obj_set_style_bg_grad_color(scr, lv_color_hex(bottom), 0);
    lv_draw_buf_t * img = os_settings.wallpaper == OS_WALL_CUSTOM ? shell_wallpaper_image(false) : NULL;
    lv_obj_set_style_bg_image_src(scr, img, 0);
    pill_refresh();
    edge_refresh();
    lock_refresh();
    home_card_sync();

    int32_t veil = (100 - os_settings.brightness) * 2;
    lv_obj_set_style_bg_opa(g.bright, (lv_opa_t)LV_CLAMP(0, veil, 180), 0);

    statusbar_refresh();
}

void shell_statusbar_set_visible(bool visible)
{
    g.bar_visible = visible;
    lv_obj_set_hidden(g.clock, !visible || g.locked);
    lv_obj_set_hidden(g.sys, !visible);
    cc_set_enabled(visible);
}

bool shell_home_idle(void)
{
    return g.state == ST_HOME;
}

void shell_launch_from_home(int id, const lv_area_t * icon_area)
{
    if(g.state == ST_HOME || g.state == ST_FADING) {
        app_launch(id, icon_area, ICON_RADIUS, SRC_ICON);
    }
    else if(g.state == ST_CLOSING) {
        /* Tap the icon the window is flying into -> reverse mid-flight */
        if(id == g.app_id) {
            app_reopen();
        }
        else {
            app_destroy();
            app_launch(id, icon_area, ICON_RADIUS, SRC_ICON);
        }
    }
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/*---------------- Spring physics (interruptible:                               velocity survives retargeting) ----------------*/

static void spring_config(float response_s, float damping)
{
    float w = 2.0f * 3.14159265f / response_s;
    g.k = w * w;
    g.c = 2.0f * damping * w;
}

static void spring_set_target(float cx, float cy, float w, float h, float r)
{
    g.sp[SP_CX].t = cx;
    g.sp[SP_CY].t = cy;
    g.sp[SP_W].t  = w;
    g.sp[SP_H].t  = h;
    g.sp[SP_R].t  = r;
}

static void spring_set_target_area(const lv_area_t * a, float r)
{
    float w = (float)lv_area_get_width(a);
    float h = (float)lv_area_get_height(a);
    spring_set_target(a->x1 + w / 2.0f, a->y1 + h / 2.0f, w, h, r);
}

static void spring_snap_area(const lv_area_t * a, float r)
{
    spring_set_target_area(a, r);
    for(int i = 0; i < SP_COUNT; i++) {
        g.sp[i].x = g.sp[i].t;
        g.sp[i].v = 0.0f;
    }
}

static bool spring_step(float dt)
{
    bool settled = true;
    for(int i = 0; i < SP_COUNT; i++) {
        spring_t * s = &g.sp[i];
        float a = -g.k * (s->x - s->t) - g.c * s->v;
        s->v += a * dt;           /* semi-implicit Euler */
        s->x += s->v * dt;
        if(fabsf(s->x - s->t) > 0.5f || fabsf(s->v) > 8.0f) settled = false;
    }
    return settled;
}

static void phys_timer_cb(lv_timer_t * t)
{
    LV_UNUSED(t);
    uint32_t elapsed = lv_tick_elaps(g.last_tick);
    g.last_tick = lv_tick_get();
    if(!g.animating || !g.frame) return;

    /* Wall-clock time: slow frames drop, motion keeps its real duration */
    float dt = CLAMPF(elapsed / 1000.0f, 0.0f, 0.1f);
    bool settled = false;
    while(dt > 0.0f) {
        float h = dt < PHYS_SUBSTEP_S ? dt : PHYS_SUBSTEP_S;
        settled = spring_step(h);
        dt -= h;
    }

    if(settled && g.state != ST_DRAG) {
        for(int i = 0; i < SP_COUNT; i++) {
            g.sp[i].x = g.sp[i].t;
            g.sp[i].v = 0.0f;
        }
        apply_card();
        g.animating = false;
        on_settle();
        return;
    }
    apply_card();
}

/*---------------- Card rendering: live UI scaled inside a morphing rounded frame ----------------*/

static void apply_card(void)
{
    if(!g.frame) return;

    float w  = g.sp[SP_W].x < 1.0f ? 1.0f : g.sp[SP_W].x;
    float h  = g.sp[SP_H].x < 1.0f ? 1.0f : g.sp[SP_H].x;
    float cx = g.sp[SP_CX].x;
    float cy = g.sp[SP_CY].x;
    float r  = g.sp[SP_R].x < 0.0f ? 0.0f : g.sp[SP_R].x;
    int32_t iw = (int32_t)lroundf(w);
    int32_t ih = (int32_t)lroundf(h);

    lv_obj_set_pos(g.frame, (int32_t)lroundf(cx - w / 2.0f), (int32_t)lroundf(cy - h / 2.0f));
    lv_obj_set_size(g.frame, iw, ih);
    lv_obj_set_style_radius(g.frame, (int32_t)lroundf(r), 0);

    /* Aspect-fill: content keeps its native layout, only the rendered layer is scaled */
    float s = fmaxf(w / (float)g.W, h / (float)g.H);
    int32_t sc = (int32_t)lroundf(s * 256.0f);
    lv_obj_set_pos(g.content, (int32_t)lroundf((w - g.W) / 2.0f), (int32_t)lroundf((h - g.H) / 2.0f));
    lv_obj_set_style_transform_scale(g.content, sc < 1 ? 1 : sc, 0);

    /* Icon / widget crossfades out as the window grows past ~2.4x its size */
    float span = fminf(g.src_w * 1.4f, (float)g.W - g.src_w);
    float t = g.src_w > 0.0f ? CLAMPF((w - g.src_w) / fmaxf(span, 1.0f), 0.0f, 1.0f) : 1.0f;
    lv_opa_t opa = (lv_opa_t)lroundf((1.0f - t) * 255.0f);
    if(opa <= 2) {
        lv_obj_set_hidden(g.overlay, true);
    }
    else {
        lv_obj_set_hidden(g.overlay, false);
        lv_obj_set_size(g.overlay, iw, ih);
        lv_obj_set_style_opa(g.overlay, opa, 0);
    }
}

static void frame_fade_done_cb(lv_anim_t * a)
{
    if(a->var != g.frame) return;
    app_destroy();
    if(g.state == ST_FADING) set_state(ST_HOME);
}

static void opa_exec(void * var, int32_t v)
{
    lv_obj_set_style_opa((lv_obj_t *)var, (lv_opa_t)v, 0);
}

static void on_settle(void)
{
    switch(g.state) {
        case ST_OPENING:
        case ST_SNAPBACK:
            set_state(ST_APP);
            break;
        case ST_CLOSING:
            app_destroy();
            set_state(ST_HOME);
            break;
        case ST_TO_RECENTS: {
            /* Hand off to the snapshot card, then throw the live UI away */
            if(g.handoff) lv_obj_set_style_opa(g.handoff, LV_OPA_COVER, 0);
            g.handoff = NULL;
            set_state(ST_RECENTS);
            lv_anim_t a;
            lv_anim_init(&a);
            lv_anim_set_var(&a, g.frame);
            lv_anim_set_exec_cb(&a, opa_exec);
            lv_anim_set_values(&a, LV_OPA_COVER, LV_OPA_TRANSP);
            lv_anim_set_duration(&a, 160);
            lv_anim_set_completed_cb(&a, frame_fade_done_cb);
            lv_anim_start(&a);
            break;
        }
        default:
            break;
    }
}

static void set_state(ui_state_t s)
{
    g.state = s;
    /* The strip stays put mid-gesture; it only steps aside while the window is in flight */
    lv_obj_set_hidden(g.zone, g.locked || s == ST_CLOSING || s == ST_TO_RECENTS || s == ST_FADING);
    pill_refresh();
    edge_refresh();
}

/*---------------- Adaptive status bar colour ----------------
 * Every ~150 ms the shell looks at what is under the bar (the topmost object with a solid background at the
 * top centre of the screen) and eases the clock + icons between white and near black so they stay readable. */

static bool bg_under(lv_obj_t * o, lv_point_t p, lv_color_t * out, int depth)
{
    if(!o || lv_obj_is_hidden(o) || depth > 14) return false;
    lv_area_t a;
    lv_obj_get_coords(o, &a);
    if(p.x < a.x1 || p.x > a.x2 || p.y < a.y1 || p.y > a.y2) return false;
    for(int32_t i = (int32_t)lv_obj_get_child_count(o) - 1; i >= 0; i--) {
        if(bg_under(lv_obj_get_child(o, i), p, out, depth + 1)) return true;
    }
    if(lv_obj_get_style_bg_opa(o, 0) >= LV_OPA_80 && lv_obj_get_style_opa(o, 0) >= LV_OPA_80) {
        *out = lv_obj_get_style_bg_color(o, 0);
        return true;
    }
    return false;
}

static float luma(lv_color_t c)
{
    return 0.299f * c.red + 0.587f * c.green + 0.114f * c.blue;
}

static bool bar_wants_dark(void)
{
    if(cc_visible()) return false;
    lv_point_t p = { g.W / 2, 14 };
    lv_color_t c;
    if(g.locked) {
        uint32_t top, bottom;
        shell_wallpaper_colors_lock(os_settings.lock_wall == OS_WALL_CUSTOM ? 0 : os_settings.lock_wall, &top, &bottom);
        return luma(lv_color_hex(top)) > 165.0f;
    }
    if((g.state == ST_APP || g.state == ST_OPENING || g.state == ST_SNAPBACK || g.state == ST_DRAG) && g.content) {
        if(bg_under(g.content, p, &c, 0)) return luma(c) > 165.0f;
        return false;
    }
    if(os_settings.wallpaper == OS_WALL_CUSTOM) return false;
    uint32_t top, bottom;
    shell_wallpaper_colors(os_settings.wallpaper, &top, &bottom);
    return luma(lv_color_hex(top)) > 165.0f;
}

static bool nav_wants_dark(void)
{
    lv_point_t p = { g.W / 2, g.H - 10 };
    lv_color_t c;
    if(g.locked) {
        uint32_t top, bottom;
        shell_wallpaper_colors_lock(os_settings.lock_wall == OS_WALL_CUSTOM ? 0 : os_settings.lock_wall, &top, &bottom);
        return luma(lv_color_hex(bottom)) > 165.0f;
    }
    if((g.state == ST_APP || g.state == ST_OPENING || g.state == ST_SNAPBACK || g.state == ST_DRAG) && g.content)
        return bg_under(g.content, p, &c, 0) && luma(c) > 165.0f;
    if(os_settings.wallpaper == OS_WALL_CUSTOM) return false;
    uint32_t top, bottom;
    shell_wallpaper_colors(os_settings.wallpaper, &top, &bottom);
    return luma(lv_color_hex(bottom)) > 165.0f;
}

void shell_statusbar_raise(void)
{
    if(g.clock) lv_obj_move_foreground(g.clock);
    if(g.sys) lv_obj_move_foreground(g.sys);
}

static void bar_color_timer_cb(lv_timer_t * t)
{
    LV_UNUSED(t);
    static uint32_t n;
    static bool target, nav_target;
    if(n++ % 5 == 0) {
        target = bar_wants_dark();
        nav_target = nav_wants_dark();
    }
    float goal = target ? 1.0f : 0.0f;
    g.bar_mix += (goal - g.bar_mix) * 0.22f;            /* eases both ways, ~0.4 s */
    if(LV_ABS(goal - g.bar_mix) < 0.01f) g.bar_mix = goal;
    uint8_t v = (uint8_t)(255.0f - g.bar_mix * (255.0f - 18.0f));
    lv_color_t c = lv_color_make(v, v, v);
    lv_obj_set_style_text_color(g.clock, c, 0);
    statusbar_icons_set_color(c);
    goal = nav_target ? 1.0f : 0.0f;
    g.nav_mix += (goal - g.nav_mix) * 0.22f;
    if(LV_ABS(goal - g.nav_mix) < 0.01f) g.nav_mix = goal;
    v = (uint8_t)(255.0f - g.nav_mix * (255.0f - 18.0f));
    lv_obj_set_style_bg_color(g.pill, lv_color_make(v, v, v), 0);
}

/*---------------- Edge back: swipe in from the left edge, anywhere in an app ----------------*/

static void edge_refresh(void)
{
    if(!g.edge) return;
    lv_obj_set_width(g.edge, os_settings.edge_size);
    bool on = os_settings.edge_back && g.state == ST_APP && !g.locked;
    lv_obj_set_hidden(g.edge, !on);
}

static void edge_event_cb(lv_event_t * e)
{
    static lv_point_t p0;
    static lv_obj_t * page;
    static float vx;
    static int32_t lastx;
    static uint32_t lastt;
    lv_event_code_t code = lv_event_get_code(e);
    lv_point_t p;
    lv_indev_get_point(lv_indev_active(), &p);
    if(code == LV_EVENT_PRESSED) {
        p0 = p;
        lastx = p.x;
        lastt = lv_tick_get();
        vx = 0;
        page = ui_nav_top(g.content);
    }
    else if(code == LV_EVENT_PRESSING) {
        uint32_t el = lv_tick_elaps(lastt);
        if(el) {
            vx = 0.6f * vx + 0.4f * (float)(p.x - lastx) / (float)el;
            lastx = p.x;
            lastt = lv_tick_get();
        }
        int32_t dx = LV_MAX(0, p.x - p0.x);
        if(page) {                                       /* the page follows the finger, the one below slides in */
            fx_set(page, FX_X, (float)dx);
            lv_obj_t * under = lv_obj_get_sibling(page, -1);
            if(under && lv_obj_is_floating(under)) fx_set(under, FX_X, -(float)(g.W - dx) * 0.28f);
        }
    }
    else if(code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
        int32_t dx = p.x - p0.x;
        bool go = code == LV_EVENT_RELEASED && (dx > g.W / 4 || (vx > 0.5f && dx > 24));
        if(go) {
            if(ui_keyboard_dismiss_visible(g.content)) {
                if(page) {
                    fx_to(page, FX_X, 0, FX_SNAPPY, 0, NULL);
                    lv_obj_t * under = lv_obj_get_sibling(page, -1);
                    if(under && lv_obj_is_floating(under)) fx_to(under, FX_X, 0, FX_SNAPPY, 0, NULL);
                }
            }
            else if(page) {
                ui_nav_pop(g.content);
            }
            else if(g.state == ST_APP) {
                app_close_to_home();
            }
        }
        else if(page) {
            fx_to(page, FX_X, 0, FX_BOUNCY, 0, NULL);
            lv_obj_t * under = lv_obj_get_sibling(page, -1);
            if(under && lv_obj_is_floating(under)) fx_to(under, FX_X, 0, FX_SNAPPY, 0, NULL);
        }
        page = NULL;
    }
}

static void pill_refresh(void)
{
    if(!g.pill) return;
    bool in_app = g.state == ST_APP || g.state == ST_OPENING || g.state == ST_SNAPBACK;
    lv_obj_set_hidden(g.pill, !(os_settings.pill && in_app));
}

/*---------------- App lifecycle (UI is created on launch and destroyed on leave) ----------------*/

static void app_launch(int id, const lv_area_t * from, float from_r, launch_src_t src)
{
    /* A widget source is set up by the caller right before (app_destroy would clear it) */
    lv_obj_t * src_obj = g.src_obj;
    lv_draw_buf_t * src_snap = g.src_snap;
    g.src_obj = NULL;
    g.src_snap = NULL;
    app_destroy();
    g.src = src;
    g.src_obj = src_obj;
    g.src_snap = src_snap;
    if(src == SRC_ICON && os_apps[id] == &app_clock) {
        lv_obj_t * clock_icon = home_get_icon_object(id);
        if(clock_icon) g.src_snap = lv_snapshot_take(clock_icon, LV_COLOR_FORMAT_ARGB8888);
    }
    g.src_w = src == SRC_SLIDE ? 0.0f : (float)lv_area_get_width(from);
    home_edit_exit();
    g.app_id = id;
    mru_touch(id);

    lv_obj_t * frame = ui_box(lv_screen_active());
    lv_obj_set_style_bg_opa(frame, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(frame, lv_color_black(), 0);
    lv_obj_set_style_clip_corner(frame, true, 0);
    lv_obj_set_clickable(frame, true);
    lv_obj_add_event_cb(frame, frame_event_cb, LV_EVENT_CLICKED, NULL);

    /* Fixed native size: the app never relayouts while the window scales */
    g.content = ui_box(frame);
    lv_obj_set_size(g.content, g.W, g.H);
    lv_obj_set_style_bg_opa(g.content, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(g.content, lv_color_black(), 0);
    lv_obj_set_style_transform_pivot_x(g.content, g.W / 2, 0);
    lv_obj_set_style_transform_pivot_y(g.content, g.H / 2, 0);
    os_apps[id]->create(g.content);

    g.overlay = ui_box(frame);
    if((src == SRC_WIDGET || (src == SRC_ICON && os_apps[id] == &app_clock)) && g.src_snap) {
        lv_obj_t * img = lv_image_create(g.overlay);
        if(src == SRC_WIDGET && g.src_obj && lv_obj_get_child_count(g.src_obj)) {
            /* the widget snapshot is see-through glass: back it with the face colour so the app never shows through */
            lv_obj_t * face = lv_obj_get_child(g.src_obj, 0);
            lv_obj_set_style_bg_color(g.overlay, lv_obj_get_style_bg_color(face, LV_PART_MAIN), 0);
            lv_obj_set_style_bg_grad_color(g.overlay, lv_obj_get_style_bg_color(face, LV_PART_MAIN), 0);
            lv_obj_set_style_bg_opa(g.overlay, LV_OPA_COVER, 0);
        }
        lv_image_set_src(img, g.src_snap);
        lv_obj_set_size(img, LV_PCT(100), LV_PCT(100));
        lv_image_set_inner_align(img, LV_IMAGE_ALIGN_STRETCH);
        g.overlay_sym = img;
    }
    else {
        ui_icon_style(g.overlay, os_apps[id]->color);
        g.overlay_sym = ui_label(g.overlay, os_apps[id]->symbol, &lv_font_montserrat_28, 0xFFFFFF);
        lv_obj_center(g.overlay_sym);
    }

    g.frame = frame;
    if(src == SRC_ICON) home_set_icon_hidden(id, true);
    if(src == SRC_WIDGET && g.src_obj) lv_obj_set_style_opa(g.src_obj, LV_OPA_TRANSP, 0);

    spring_snap_area(from, from_r);
    spring_config(0.50f, 0.78f);
    spring_set_target(g.W / 2.0f, g.H / 2.0f, (float)g.W, (float)g.H, 0.0f);
    set_state(ST_OPENING);
    g.animating = true;
    apply_card();
}

static void app_reopen(void)
{
    spring_config(0.50f, 0.78f);
    spring_set_target(g.W / 2.0f, g.H / 2.0f, (float)g.W, (float)g.H, 0.0f);
    set_state(ST_OPENING);
    g.animating = true;
}

static void clock_snap_refresh(void)
{
    lv_obj_t * icon = home_get_icon_object(g.app_id);
    if(!icon || !g.overlay_sym) return;
    lv_draw_buf_t * snap = lv_snapshot_take(icon, LV_COLOR_FORMAT_ARGB8888);
    if(!snap) return;
    lv_image_set_src(g.overlay_sym, snap);
    if(g.src_snap) {
        lv_image_cache_drop(g.src_snap);
        lv_draw_buf_destroy(g.src_snap);
    }
    g.src_snap = snap;
}

static void app_close_to_home(void)
{
    lv_area_t a;
    float r = ICON_RADIUS;
    if(g.src == SRC_ICON && g.app_id >= 0 && os_apps[g.app_id] == &app_clock) clock_snap_refresh();
    if(g.src == SRC_WIDGET && g.src_obj && lv_obj_is_valid(g.src_obj) && !lv_obj_has_flag(g.src_obj, LV_OBJ_FLAG_HIDDEN)) {
        lv_obj_get_coords(g.src_obj, &a);       /* morph back into the widget */
        r = 22;
        g.src_w = (float)lv_area_get_width(&a);
    }
    else if(!home_get_icon_area(g.app_id, &a)) {
        /* Nothing to fly back to (Control Center, Spotlight, icon on another page): fade away */
        app_close_fade();
        return;
    }
    else {
        g.src_w = (float)lv_area_get_width(&a);
        home_set_icon_hidden(g.app_id, true);
    }
    spring_config(0.46f, 0.84f);
    spring_set_target_area(&a, r);
    set_state(ST_CLOSING);
    g.animating = true;
}

static void app_close_fade(void)
{
    /* Shrink a little around where the card is now while fading out */
    spring_config(0.40f, 1.0f);
    spring_set_target(g.sp[SP_CX].x, g.sp[SP_CY].x, g.sp[SP_W].x * 0.8f, g.sp[SP_H].x * 0.8f, 30.0f);
    g.src_w = 0.0f;
    lv_obj_set_hidden(g.overlay, true);
    set_state(ST_FADING);
    g.animating = true;
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, g.frame);
    lv_anim_set_exec_cb(&a, opa_exec);
    lv_anim_set_values(&a, LV_OPA_COVER, LV_OPA_TRANSP);
    lv_anim_set_duration(&a, 220);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_in);
    lv_anim_set_completed_cb(&a, frame_fade_done_cb);
    lv_anim_start(&a);
}

static void ghost_gone_cb(lv_obj_t * o)
{
    lv_obj_delete_async(o);
}

/* Sideways swipe on the bottom edge: slide the current app out and the neighbour in */
static void quick_switch(int dir)
{
    int cur = g.app_id;
    /* Consecutive swipes walk through every recent app instead of ping-ponging between two */
    static int8_t order[MRU_MAX];
    static int order_n, cursor;
    static uint32_t last_qs;
    if(order_n == 0 || lv_tick_elaps(last_qs) > 4000 || order[cursor] != cur) {
        order_n = g.mru_n;
        for(int i = 0; i < order_n; i++) order[i] = g.mru[i];
        cursor = order_n - 1;
        for(int i = 0; i < order_n; i++) if(order[i] == cur) cursor = i;
    }
    if(order_n < 2) return;
    cursor = (cursor + (dir > 0 ? -1 : 1) + order_n) % order_n;
    last_qs = lv_tick_get();
    int next = order[cursor];
    if(next == cur) return;

    lv_area_t fr;
    lv_obj_get_coords(g.frame, &fr);
    if(g.snap[cur]) {
        lv_obj_t * ghost = lv_image_create(lv_screen_active());
        lv_image_set_src(ghost, g.snap[cur]);
        lv_obj_set_pos(ghost, fr.x1, fr.y1);
        lv_obj_set_size(ghost, lv_area_get_width(&fr), lv_area_get_height(&fr));
        lv_image_set_inner_align(ghost, LV_IMAGE_ALIGN_STRETCH);
        fx_to(ghost, FX_X, (float)(dir * g.W), 0.32f, 1.0f, 0, ghost_gone_cb);
    }
    app_destroy();
    lv_area_t from;
    lv_area_set(&from, -dir * g.W, 0, -dir * g.W + g.W - 1, g.H - 1);
    app_launch(next, &from, 0, SRC_SLIDE);
    spring_config(0.36f, 0.95f);
}

static void app_destroy(void)
{
    if(g.frame) lv_obj_delete(g.frame);
    g.frame = g.content = g.overlay = g.overlay_sym = NULL;
    if(g.src_obj) lv_obj_set_style_opa(g.src_obj, LV_OPA_COVER, 0);
    g.src_obj = NULL;
    if(g.src_snap) {
        lv_image_cache_drop(g.src_snap);
        lv_draw_buf_destroy(g.src_snap);
        g.src_snap = NULL;
    }
    if(g.app_id >= 0) home_set_icon_hidden(g.app_id, false);
    g.app_id = -1;
    g.animating = false;
}

/*---------------- Finger tracking ----------------*/

static void track_begin(void)
{
    lv_indev_get_point(lv_indev_active(), &g.t.p0);
    g.t.last = g.t.p0;
    g.t.last_tick = lv_tick_get();
    g.t.vx = g.t.vy = 0.0f;
    g.t.still_ms = 0;
    g.armed = false;
}

static uint32_t track_update(lv_point_t * p)
{
    lv_indev_get_point(lv_indev_active(), p);
    uint32_t el = lv_tick_elaps(g.t.last_tick);
    if(el > 0) {
        float ivx = (float)(p->x - g.t.last.x) / (float)el;
        float ivy = (float)(p->y - g.t.last.y) / (float)el;
        g.t.vx = 0.6f * g.t.vx + 0.4f * ivx;
        g.t.vy = 0.6f * g.t.vy + 0.4f * ivy;
        g.t.last = *p;
        g.t.last_tick = lv_tick_get();
    }
    return el;
}

static float track_speed(void)
{
    return sqrtf(g.t.vx * g.t.vx + g.t.vy * g.t.vy);
}

/*---------------- App gesture: drag card, fling home, hold for recents ----------------*/

static void drag_begin(void)
{
    /* Settled window = a clean frame for the recents card */
    if(g.state == ST_APP) snap_take(g.app_id);

    track_begin();
    /* Grab the card where it currently is (interrupts an in-flight launch) */
    g.cx0 = g.sp[SP_CX].x;
    g.bottom0 = g.sp[SP_CY].x + g.sp[SP_H].x / 2.0f;
    g.s0 = CLAMPF(g.sp[SP_W].x / (float)g.W, 0.1f, 1.0f);
    g.prog = 0.0f;

    spring_config(0.09f, 1.0f);   /* stiff, critically damped: near 1:1 tracking */
    set_state(ST_DRAG);
    g.animating = true;
}

static void drag_update(void)
{
    lv_point_t p;
    uint32_t el = track_update(&p);

    float dx = (float)(p.x - g.t.p0.x);
    float dy = (float)(p.y - g.t.p0.y);
    float prog = CLAMPF(-dy / (g.H * DRAG_FULL_TRAVEL), 0.0f, 1.0f);
    float dyr = dy > 0.0f ? dy * 0.25f : dy;      /* rubber-band downward */
    g.prog = prog;

    if(!g.armed) {
        if(prog > HOLD_MIN_PROGRESS && track_speed() < HOLD_MAX_SPEED) g.t.still_ms += el;
        else g.t.still_ms = 0;
        if(g.t.still_ms >= HOLD_TIME_MS) {
            g.armed = true;
            dim_to(LV_OPA_50);
        }
    }
    else if(g.t.vy < -0.9f) {
        /* Fast upward flick after the pause still means "home" */
        g.armed = false;
        g.t.still_ms = 0;
        dim_to(LV_OPA_TRANSP);
    }

    float s = g.armed ? CARD_SCALE : g.s0 * (1.0f - (1.0f - DRAG_MIN_SCALE) * prog);
    float w = g.W * s;
    float h = g.H * s;
    float r = 28.0f * CLAMPF(prog * 5.0f, 0.0f, 1.0f);
    spring_set_target(g.cx0 + dx, g.bottom0 + dyr - h / 2.0f, w, h, g.armed ? CARD_RADIUS : r);
}

static void drag_end(void)
{
    if(g.armed) {
        recents_enter_from_drag();
        return;
    }

    dim_to(LV_OPA_TRANSP);
    float sdx = (float)(g.t.last.x - g.t.p0.x);
    float sdy = (float)(g.t.last.y - g.t.p0.y);
    if(os_settings.quick_switch && g.mru_n > 1 && fabsf(sdx) > fabsf(sdy) * 1.3f &&
       (fabsf(sdx) > g.W / 4.0f || fabsf(g.t.vx) > 0.5f)) {
        quick_switch(sdx > 0 ? 1 : -1);
        return;
    }
    bool go_home = (g.t.vy < FLING_HOME_VY) || (g.prog > HOME_PROGRESS && g.t.vy < 0.15f);
    if(g.t.vy > FLING_CANCEL_VY) go_home = false;

    if(go_home) {
        app_close_to_home();
    }
    else {
        spring_config(0.38f, 0.86f);
        spring_set_target(g.W / 2.0f, g.H / 2.0f, (float)g.W, (float)g.H, 0.0f);
        set_state(ST_SNAPBACK);
        g.animating = true;
    }
}

/*---------------- Home gesture: swipe up and hold -> recents ----------------*/

static void home_hold_update(void)
{
    lv_point_t p;
    uint32_t el = track_update(&p);
    int32_t dy = p.y - g.t.p0.y;
    if(g.armed || g.mru_n == 0 || home_in_edit()) return;

    if(dy < HOME_HOLD_DY) g.t.still_ms += el;
    else g.t.still_ms = 0;
    if(g.t.still_ms >= HOLD_TIME_MS) {
        g.armed = true;
        dim_to(LV_OPA_40);
    }
}

static void home_hold_end(bool released)
{
    if(g.armed && released) {
        recents_open_from_home();
        return;
    }
    if(g.armed) dim_to(LV_OPA_TRANSP);
    g.armed = false;
    if(released && g.t.last.y - g.t.p0.y < SWIPE_UP_DY && home_in_edit()) home_edit_exit();
}

static void zone_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_point_t p;

    if(code == LV_EVENT_PRESSED) {
        track_begin();
        if(g.frame && (g.state == ST_OPENING || g.state == ST_APP || g.state == ST_SNAPBACK)) drag_begin();
    }
    else if(code == LV_EVENT_PRESSING) {
        if(g.state == ST_DRAG) drag_update();
        else if(g.state == ST_HOME) home_hold_update();
        else track_update(&p);
    }
    else if(code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
        bool released = code == LV_EVENT_RELEASED;
        if(g.state == ST_DRAG) {
            drag_end();
        }
        else if(g.state == ST_HOME) {
            home_hold_end(released);
        }
        else if(g.state == ST_RECENTS && released) {
            if(g.t.last.y - g.t.p0.y < SWIPE_UP_DY || g.t.vy < FLING_HOME_VY) recents_dismiss();
        }
    }
}

static void frame_event_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    /* Tap the window while it flies home -> reverse mid-flight */
    if(g.state == ST_CLOSING) {
        if(g.src == SRC_ICON || g.src == SRC_SLIDE) home_set_icon_hidden(g.app_id, true);
        app_reopen();
    }
}

/*---------------- Dim backdrop ----------------*/

static void dim_exec(void * var, int32_t v)
{
    lv_obj_set_style_bg_opa((lv_obj_t *)var, (lv_opa_t)v, 0);
    lv_obj_set_hidden((lv_obj_t *)var, v == 0);
#if OS_FX_BLUR
    int32_t r = (v * 24 / 153) / 3 * 3;                 /* the switcher blurs what is behind it as it fades in */
    lv_obj_set_style_blur_radius((lv_obj_t *)var, r, 0);
    lv_obj_set_style_blur_backdrop((lv_obj_t *)var, r > 0, 0);
#endif
}

static void dim_to(lv_opa_t opa)
{
    lv_anim_delete(g.dim, dim_exec);
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, g.dim);
    lv_anim_set_exec_cb(&a, dim_exec);
    lv_anim_set_values(&a, lv_obj_get_style_bg_opa(g.dim, 0), opa);
    lv_anim_set_duration(&a, 180);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    lv_anim_start(&a);
}

/*---------------- Status bar ----------------*/

static void statusbar_refresh(void)
{
    statusbar_icons_update();
}

static void clock_timer_cb(lv_timer_t * t)
{
    LV_UNUSED(t);
    time_t now = time(NULL);
    struct tm * tm = localtime(&now);
    if(!tm) return;
    int h = tm->tm_hour % 12;
    lv_label_set_text_fmt(g.clock, "%d:%02d", h == 0 ? 12 : h, tm->tm_min);
}

/*---------------- Snapshots: what the app looked like when it was left ----------------*/

static void snap_take(int id)
{
    if(id < 0 || id >= OS_MAX_APPS || !g.content) return;
    lv_draw_buf_t * buf = lv_snapshot_take(g.content, LV_COLOR_FORMAT_RGB565);
    if(!buf) return;    /* out of memory: the card falls back to the app icon */
    snap_free(id);
    g.snap[id] = buf;
}

static void snap_free(int id)
{
    if(!g.snap[id]) return;
    lv_image_cache_drop(g.snap[id]);
    lv_draw_buf_destroy(g.snap[id]);
    g.snap[id] = NULL;
}

/*---------------- Recents ----------------*/

static void mru_touch(int id)
{
    mru_remove(id);
    if(g.mru_n == MRU_MAX) {
        snap_free(g.mru[0]);
        for(int i = 1; i < MRU_MAX; i++) g.mru[i - 1] = g.mru[i];
        g.mru_n--;
    }
    g.mru[g.mru_n++] = (int8_t)id;
}

static void mru_remove(int id)
{
    for(int i = 0; i < g.mru_n; i++) {
        if(g.mru[i] != id) continue;
        for(int j = i + 1; j < g.mru_n; j++) g.mru[j - 1] = g.mru[j];
        g.mru_n--;
        return;
    }
}

static lv_obj_t * recents_item_create(lv_obj_t * parent, int id)
{
    const app_desc_t * app = os_apps[id];

    lv_obj_t * item = ui_box(parent);
    lv_obj_set_size(item, g.card_w, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(item, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(item, 8, 0);
    lv_obj_set_overflow_visible(item, true);

    lv_obj_t * head = ui_box(item);
    lv_obj_set_size(head, LV_PCT(100), 24);
    lv_obj_set_flex_flow(head, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(head, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(head, 8, 0);

    lv_obj_t * ic = ui_box(head);
    lv_obj_set_size(ic, 22, 22);
    lv_obj_set_style_radius(ic, 6, 0);
    ui_icon_style(ic, app->color);
    lv_obj_center(ui_label(ic, app->symbol, &lv_font_montserrat_12, 0xFFFFFF));
    ui_label(head, app->name, &lv_font_montserrat_14, 0xFFFFFF);

    lv_obj_t * card = ui_box(item);
    lv_obj_set_size(card, g.card_w, g.card_h);
    lv_obj_set_style_radius(card, CARD_RADIUS, 0);
    lv_obj_set_style_clip_corner(card, true, 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(card, lv_color_black(), 0);
    lv_obj_set_clickable(card, true);
    lv_obj_set_press_lock(card, true);
    lv_obj_set_user_data(card, (void *)(intptr_t)id);
    lv_obj_add_event_cb(card, recents_card_event_cb, LV_EVENT_ALL, (void *)(intptr_t)id);

    if(g.snap[id]) {
        lv_obj_t * img = lv_image_create(card);
        lv_image_set_src(img, g.snap[id]);
        lv_obj_set_size(img, LV_PCT(100), LV_PCT(100));
        lv_image_set_inner_align(img, LV_IMAGE_ALIGN_STRETCH);
        lv_obj_set_clickable(img, false);
    }
    else {
        ui_icon_style(card, app->color);
        lv_obj_center(ui_label(card, app->symbol, &lv_font_montserrat_48, 0xFFFFFF));
    }
    return card;
}

static int32_t rec_gap(void)
{
    return s_rec_style ? -(g.card_w * 22 / 100) : CARD_GAP;
}

static void rec_depth_cb(lv_event_t * e)
{
    lv_obj_t * row = lv_event_get_target_obj(e);
    int32_t pitch = g.card_w + rec_gap();
    int32_t sx = lv_obj_get_scroll_x(row);
    int32_t pad = (g.W - g.card_w) / 2;
    for(uint32_t i = 0; i < lv_obj_get_child_count(row); i++) {
        lv_obj_t * it = lv_obj_get_child(row, (int32_t)i);
        float d = (float)(pad + (int32_t)i * pitch - sx) / (float)pitch;     /* 0 = centred card */
        float a = fabsf(d) > 1.6f ? 1.6f : fabsf(d);
        lv_obj_set_style_transform_pivot_x(it, g.card_w / 2, 0);
        lv_obj_set_style_transform_pivot_y(it, lv_obj_get_height(it) / 2, 0);
        lv_obj_set_style_transform_scale(it, (int32_t)(256.0f - a * 46.0f), 0);
        lv_obj_set_style_translate_y(it, (int32_t)(a * 16.0f), 0);
        lv_obj_set_style_opa(it, (lv_opa_t)(255.0f - a * 70.0f), 0);
    }
}

static lv_obj_t * recents_build(int focus_id)
{
    s_rec_style = sdk_get_int("recents.style", 0);
    g.rec = ui_box(lv_screen_active());
    lv_obj_set_size(g.rec, g.W, g.H);
    lv_obj_set_clickable(g.rec, true);
    lv_obj_add_event_cb(g.rec, recents_bg_event_cb, LV_EVENT_CLICKED, NULL);

    g.row = lv_obj_create(g.rec);
    lv_obj_remove_style_all(g.row);
    lv_obj_set_size(g.row, g.W, g.card_h + 40);
    lv_obj_align(g.row, LV_ALIGN_CENTER, 0, 6);
    lv_obj_set_flex_flow(g.row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(g.row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_END);
    lv_obj_set_style_pad_hor(g.row, (g.W - g.card_w) / 2, 0);
    lv_obj_set_style_pad_column(g.row, rec_gap(), 0);
    lv_obj_set_scroll_dir(g.row, LV_DIR_HOR);
    lv_obj_set_scroll_snap_x(g.row, LV_SCROLL_SNAP_CENTER);
    lv_obj_set_scrollbar_mode(g.row, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_overflow_visible(g.row, true);
    lv_obj_add_event_cb(g.row, recents_bg_event_cb, LV_EVENT_CLICKED, NULL);

    /* Oldest on the left, most recent centered on the right */
    lv_obj_t * focus = NULL;
    for(int i = 0; i < g.mru_n; i++) {
        lv_obj_t * card = recents_item_create(g.row, g.mru[i]);
        if(g.mru[i] == focus_id) focus = card;
    }

    lv_obj_update_layout(g.rec);
    if(g.mru_n > 1) lv_obj_scroll_to_x(g.row, (g.mru_n - 1) * (g.card_w + rec_gap()), LV_ANIM_OFF);
    lv_obj_update_layout(g.rec);
    if(s_rec_style) {
        lv_obj_add_event_cb(g.row, rec_depth_cb, LV_EVENT_SCROLL, NULL);
        lv_obj_send_event(g.row, LV_EVENT_SCROLL, NULL);
    }

    if(g.frame) lv_obj_move_foreground(g.frame);
    return focus;
}

static void recents_enter_from_drag(void)
{
    dim_to(LV_OPA_60);
    /* The app now lives in the switcher: give its home icon back right away */
    home_set_icon_hidden(g.app_id, false);

    lv_obj_t * card = recents_build(g.app_id);
    g.handoff = card;
    if(card) {
        lv_area_t a;
        lv_obj_set_style_opa(card, LV_OPA_TRANSP, 0);
        lv_obj_get_coords(card, &a);
        spring_config(0.40f, 0.86f);
        spring_set_target_area(&a, CARD_RADIUS);
    }
    set_state(ST_TO_RECENTS);
    g.animating = true;
}

static void translate_y_exec(void * var, int32_t v)
{
    lv_obj_set_style_translate_y((lv_obj_t *)var, v, 0);
}

static void recents_open_from_home(void)
{
    home_edit_exit();
    dim_to(LV_OPA_60);
    recents_build(-1);

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, g.row);
    lv_anim_set_exec_cb(&a, translate_y_exec);
    lv_anim_set_values(&a, 80, 0);
    lv_anim_set_duration(&a, 280);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    lv_anim_start(&a);
    lv_anim_set_exec_cb(&a, opa_exec);
    lv_anim_set_values(&a, LV_OPA_TRANSP, LV_OPA_COVER);
    lv_anim_start(&a);

    set_state(ST_RECENTS);
}

void shell_recents_open(void)
{
    if(g.state != ST_HOME || g.locked) return;
    if(g.mru_n == 0) { sdk_toast("Recents", "No recent apps yet."); return; }
    recents_open_from_home();
}

static void recents_dismiss(void)
{
    if(g.rec) lv_obj_delete_async(g.rec);
    g.rec = g.row = NULL;
    g.handoff = NULL;
    dim_to(LV_OPA_TRANSP);
    set_state(ST_HOME);
}

static void recents_launch(lv_obj_t * card, int id)
{
    lv_area_t a;
    lv_obj_get_coords(card, &a);
    lv_obj_delete_async(g.rec);
    g.rec = g.row = NULL;
    g.handoff = NULL;
    dim_to(LV_OPA_TRANSP);
    app_launch(id, &a, CARD_RADIUS, SRC_SLIDE);
    g.src = SRC_ICON;       /* morphs back into its icon on exit */
    g.src_w = 60.0f;
    home_set_icon_hidden(id, true);
}

static void recents_bg_event_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    if(g.state == ST_RECENTS) recents_dismiss();
}

/* A dismissed card's slot closes up from either side. If it was left of the centred card
 * (or the last one) the strip scrolls along, so the neighbour on the left slides in. */
typedef struct {
    lv_obj_t * item;
    int id;
    int32_t w0, fp_prev;
    bool first, follow;
} collapse_t;

static void collapse_item_deleted_cb(lv_event_t * e)
{
    collapse_t * c = lv_event_get_user_data(e);
    c->item = NULL;
}

static void collapse_exec(void * var, int32_t v)
{
    collapse_t * c = var;
    if(!c->item || !g.row) return;
    int32_t w = c->w0 * v / 1000;
    int32_t m = -rec_gap() * (1000 - v) / 1000;
    lv_obj_set_width(c->item, w);
    if(c->first) lv_obj_set_style_margin_right(c->item, m, 0);
    else lv_obj_set_style_margin_left(c->item, m, 0);
    int32_t fp = (c->w0 + rec_gap()) * v / 1000;
    if(c->follow) lv_obj_scroll_by(g.row, c->fp_prev - fp, 0, LV_ANIM_OFF);
    c->fp_prev = fp;
}

static void collapse_done_cb(lv_anim_t * a)
{
    collapse_t * c = a->var;
    if(c->item) {
        lv_obj_remove_event_cb_with_user_data(c->item, collapse_item_deleted_cb, c);
        lv_obj_delete(c->item);
    }
    snap_free(c->id);
    if(g.mru_n == 0 && g.state == ST_RECENTS) {
        recents_dismiss();
        return;
    }
    if(g.row) {
        int32_t pitch = g.card_w + rec_gap();
        lv_obj_update_layout(g.row);
        int32_t idx = (lv_obj_get_scroll_x(g.row) + pitch / 2) / pitch;
        idx = LV_CLAMP(0, idx, g.mru_n - 1);
        lv_obj_scroll_to_x(g.row, idx * pitch, LV_ANIM_ON);
    }
}

static void collapse_deleted_cb(lv_anim_t * a)
{
    lv_free(a->var);
}

static void card_removed_cb(lv_anim_t * a)
{
    lv_obj_t * card = (lv_obj_t *)a->var;
    lv_obj_t * item = lv_obj_get_parent(card);
    if(!g.row || lv_obj_get_parent(item) != g.row) return;

    int32_t pitch = g.card_w + rec_gap();
    int32_t n = (int32_t)lv_obj_get_child_count(g.row);
    int32_t idx = lv_obj_get_index(item);
    int32_t center = (lv_obj_get_scroll_x(g.row) + pitch / 2) / pitch;

    collapse_t * c = lv_malloc_zeroed(sizeof(collapse_t));
    c->item = item;
    c->id = (int)(intptr_t)lv_obj_get_user_data(card);
    c->w0 = lv_obj_get_width(item);
    c->fp_prev = c->w0 + rec_gap();
    c->first = idx == 0;
    c->follow = idx < center || (idx == n - 1 && n > 1);
    lv_obj_add_event_cb(item, collapse_item_deleted_cb, LV_EVENT_DELETE, c);
    lv_obj_set_style_opa(item, LV_OPA_TRANSP, 0);

    lv_anim_t an;
    lv_anim_init(&an);
    lv_anim_set_var(&an, c);
    lv_anim_set_exec_cb(&an, collapse_exec);
    lv_anim_set_values(&an, 1000, 0);
    lv_anim_set_duration(&an, 260);
    lv_anim_set_path_cb(&an, lv_anim_path_ease_in_out);
    lv_anim_set_completed_cb(&an, collapse_done_cb);
    lv_anim_set_deleted_cb(&an, collapse_deleted_cb);
    lv_anim_start(&an);
}

static void card_translate_to(lv_obj_t * card, int32_t to, bool remove)
{
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, card);
    lv_anim_set_exec_cb(&a, translate_y_exec);
    lv_anim_set_values(&a, lv_obj_get_style_translate_y(card, 0), to);
    lv_anim_set_duration(&a, remove ? 220 : 260);
    lv_anim_set_path_cb(&a, remove ? lv_anim_path_ease_in : lv_anim_path_overshoot);
    if(remove) lv_anim_set_completed_cb(&a, card_removed_cb);
    lv_anim_start(&a);
}

static void recents_card_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * card = lv_event_get_target_obj(e);
    int id = (int)(intptr_t)lv_event_get_user_data(e);
    if(g.state != ST_RECENTS) return;

    lv_point_t p;
    if(code == LV_EVENT_PRESSED) {
        lv_indev_get_point(lv_indev_active(), &p);
        s_card_y0 = p.y;
        s_card_dy = 0;
        s_card_dragged = false;
        lv_anim_delete(card, translate_y_exec);
    }
    else if(code == LV_EVENT_PRESSING) {
        lv_indev_get_point(lv_indev_active(), &p);
        s_card_dy = p.y - s_card_y0;
        if(s_card_dy < -6) s_card_dragged = true;
        if(s_card_dragged) lv_obj_set_style_translate_y(card, s_card_dy < 0 ? s_card_dy : s_card_dy / 4, 0);
    }
    else if(code == LV_EVENT_RELEASED) {
        if(!s_card_dragged) return;
        if(s_card_dy < CARD_DISMISS_DY) {
            mru_remove(id);
            card_translate_to(card, -g.H, true);
        }
        else {
            card_translate_to(card, 0, false);
        }
    }
    else if(code == LV_EVENT_PRESS_LOST) {
        card_translate_to(card, 0, false);
    }
    else if(code == LV_EVENT_CLICKED) {
        if(!s_card_dragged) recents_launch(card, id);
    }
}

void shell_debug_home(void)
{
    if(g.frame && g.state == ST_APP) app_close_to_home();
}

bool s3_shell_busy(void) { return g.animating; }
bool s3_app_active(void) { return g.frame != NULL; }
void s3_app_close(void)
{
 if(g.state == ST_RECENTS) recents_dismiss();
 else shell_debug_home();
}

int s3_recents_count(void) { return g.mru_n; }
bool s3_recents_visible(void) { return g.state == ST_RECENTS; }
void s3_recents_clear_one(void)
{
 if(g.state != ST_RECENTS || !g.row || !lv_obj_get_child_count(g.row)) return;
 lv_obj_t * item = lv_obj_get_child(g.row, 0);
 /* Child 0 is the app heading; child 1 is the dismissible snapshot card. */
 if(lv_obj_get_child_count(item)>1) {
  lv_obj_t * card=lv_obj_get_child(item, 1);
  mru_remove((int)(intptr_t)lv_obj_get_user_data(card));
  card_translate_to(card, -g.H, true);
 }
}
void s3_back(void)
{
 if(g.content && (ui_keyboard_dismiss_visible(g.content) || ui_nav_pop(g.content))) return;
 s3_app_close();
}

static lv_timer_t * recents_clear_timer;
static void recents_clear_tick(lv_timer_t * t)
{
 if(g.state != ST_RECENTS || !g.mru_n) { lv_timer_delete(t); recents_clear_timer=NULL; return; }
 s3_recents_clear_one();
}
void s3_recents_clear_all(void)
{
 if(g.state == ST_RECENTS && !recents_clear_timer) recents_clear_timer=lv_timer_create(recents_clear_tick,800,NULL);
}
