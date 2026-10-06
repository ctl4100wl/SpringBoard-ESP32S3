/*********************
 *      INCLUDES
 *********************/

#include <string.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include "sdk.h"

extern const app_desc_t app_clock;
void lv_image_cache_drop(const void * src);

/*********************
 *      DEFINES
 *********************/

#define ICON            60
#define ICON_R          14
#define TILE_W          76
#define TILE_H          82
#define TILE_OX         ((TILE_W - ICON) / 2)
#define GRID_TOP        48
#define ROW_PITCH       80
#define DOCK_H          78
#define DOCK_BOTTOM     16
#define LONG_PRESS_MS   450
#define MOVE_SLOP       8
#define EDGE_PX         20
#define EDGE_FLIP_MS    600
#define MOVE_ANIM_MS    240
#define SEARCH_OPEN_DY  50
#define PAGE_RESPONSE   0.42f
#define PAGE_DAMPING    0.82f
#define CARD_SCALE      212
#define CARD_RADIUS     34

/**********************
 *      TYPEDEFS
 **********************/

typedef enum {
    T_IDLE,
    T_PRESS,
    T_PAGE,
    T_DRAG,
    T_SEARCH,
    T_WDRAG,
    T_WRESIZE,
    T_FOLDER,
    T_NONE,
} touch_mode_t;

/**********************
 *  STATIC PROTOTYPES
 **********************/

static void layout_default(int8_t * l);
static void layout_apply(const int8_t * l, int skip_id, bool anim);
static void loc_pos(int loc, lv_obj_t ** parent, int32_t * x, int32_t * y);
static int loc_at(int32_t cx, int32_t cy);
static void tile_create(int id);
static void tile_move(int id, int loc, bool anim);
static void reparent_keep_pos(lv_obj_t * obj, lv_obj_t * parent);
static int hit_test(lv_point_t p, bool * on_badge);
static void press_visual(int id, bool pressed);

static void page_go(int page, bool anim, float velocity);
static void spring_cb(lv_timer_t * t);
static bool loc_blocked(int loc);
static void jiggle(int id, bool on);

static void drag_start(lv_point_t p);
static void drag_move(lv_point_t p, uint32_t el);
static void drag_drop(void);

static void app_remove(int id);
static void sheet_open(void);
static void sheet_close(void);

static void root_event_cb(lv_event_t * e);
static void done_event_cb(lv_event_t * e);
static void wall_event_cb(lv_event_t * e);
static void add_event_cb(lv_event_t * e);
static void sheet_event_cb(lv_event_t * e);
static void sheet_row_event_cb(lv_event_t * e);
static void widgets_build(void);
static bool handle_hit(int k, lv_point_t p, int * which);
static void wdrag_start(lv_point_t p);
static void wdrag_move(lv_point_t p);
static void wdrag_drop(void);
static void wresize_start(lv_point_t p);
static void wresize_move(lv_point_t p);
static void wresize_drop(void);
static void ghost_hide(void);
static void search_prog(lv_obj_t * o, float p);
static void search_anim(lv_obj_t * o, float to, lv_anim_completed_cb_t done);
static void search_open(void);
static void search_close(void);
static void clock_icon_tick(lv_timer_t * t);
static void folder_refresh_icon(int owner);
static void folder_open(int owner);
static void folder_close(void);
static bool folder_add(int owner, int id);
static void folder_dissolve(int owner, bool remove_owner);
static void folder_drag_out(int id, lv_point_t p);
static void drag_start(lv_point_t p);

/**********************
 *  STATIC VARIABLES
 **********************/

static int8_t s_layout[HOME_LOC_COUNT];
static bool s_layout_valid;
static home_widget_t s_wid[HOME_WIDGETS];
static bool s_wid_valid;
static bool s_veil;
static struct {
    int8_t member[HOME_FOLDER_ITEMS];
    uint8_t count;
} s_folder[OS_MAX_APPS];

static struct {
    lv_obj_t * root;
    lv_obj_t * pages;
    lv_obj_t * dock;
    lv_obj_t * clock_hand[3];
    lv_point_precise_t clock_points[3][2];
    lv_point_precise_t clock_ticks[12][2];
    lv_obj_t * folder_overlay;
    lv_obj_t * fplate;
    int fplate_owner;
    int reflow_loc;
    uint32_t reflow_tick;
    lv_obj_t * folder_panel;
    lv_obj_t * folder_dim;
    lv_obj_t * folder_grid;
    lv_obj_t * folder_snap_img;
    lv_draw_buf_t * folder_snap;
    lv_obj_t * folder_title;
    lv_area_t folder_from, folder_to;
    bool folder_closing;
    int folder_owner;
    int folder_hit;
    lv_obj_t * fcell[OS_MAX_APPS];
    lv_obj_t * mini[OS_MAX_APPS];
    lv_obj_t * mcell[OS_MAX_APPS][HOME_FOLDER_ITEMS];
    lv_obj_t * dots[HOME_PAGES];
    lv_obj_t * chrome;
    lv_obj_t * btn_add;
    lv_obj_t * btn_done;
    lv_obj_t * btn_wall;
    bool card;
    lv_obj_t * sheet;

    lv_obj_t * tile[OS_MAX_APPS];
    lv_obj_t * icon[OS_MAX_APPS];
    lv_obj_t * label[OS_MAX_APPS];
    lv_obj_t * badge[OS_MAX_APPS];
    int8_t where[OS_MAX_APPS];
    int dock_count;
    int8_t dock_rank[HOME_DOCK];

    int32_t W, H, gap, dock_y;
    int page;
    bool edit;

    touch_mode_t mode;
    lv_point_t p0;
    lv_point_t last;
    uint32_t t0, last_tick;
    float vx;
    int hit;
    bool hit_badge;

    int drag_id;
    int drag_from;
    int target;
    int folder_hover_loc;
    uint32_t folder_hover_tick;
    bool folder_ready;
    lv_point_t grab;
    uint32_t edge_ms;
    int8_t preview[HOME_LOC_COUNT];

    lv_obj_t * handle[HOME_WIDGETS][2];
    lv_obj_t * inner[HOME_WIDGETS];
    lv_obj_t * ghost;
    int wk, wres_axis, hit_handle;
    lv_point_t wgrab;
    uint32_t wedge_ms, wedge_t;
    int32_t wres_w0, wres_h0;
    lv_obj_t * widget[HOME_WIDGETS];
    lv_obj_t * wbadge[HOME_WIDGETS];
    int hit_widget;
    bool hit_wbadge;

    lv_obj_t * search;
    lv_obj_t * search_ta;
    lv_obj_t * search_list;
    lv_obj_t * search_kb;

    lv_timer_t * spring;
    float px, pv;
    uint32_t spring_tick;
} h;

static void widgets_default(void)
{
    for(int k = 0; k < HOME_WIDGETS; k++) s_wid[k].widget = -1;

    for(int k = 0; k < os_widget_count && k < 2; k++) {
        s_wid[k].widget = (int8_t)k;
        s_wid[k].page = 1;
        s_wid[k].cell = (int8_t)(k * 2);
    }
    s_wid_valid = true;
}

static void widget_cells_sz(widget_size_t sz, int * cols, int * rows)
{
    *cols = sz == WIDGET_SMALL ? 2 : 4;
    *rows = sz == WIDGET_LARGE ? 4 : 2;
}

static void widget_cells(int widget, int * cols, int * rows)
{
    widget_size_t sz = os_widgets[widget]->size;
    *cols = sz == WIDGET_SMALL ? 2 : 4;
    *rows = sz == WIDGET_LARGE ? 4 : 2;
}

static int widget_at_loc(int loc)
{
    if(loc >= HOME_LOC_DOCK0) return -1;
    int page = loc / HOME_SLOTS, col = loc % HOME_COLS, row = (loc % HOME_SLOTS) / HOME_COLS;
    for(int k = 0; k < HOME_WIDGETS; k++) {
        if(s_wid[k].widget < 0 || s_wid[k].page != page) continue;
        int cols, rows;
        widget_cells_sz(s_wid[k].size > 0 ? (widget_size_t)(s_wid[k].size - 1) : os_widgets[s_wid[k].widget]->size, &cols, &rows);
        int c0 = s_wid[k].cell % HOME_COLS, r0 = s_wid[k].cell / HOME_COLS;
        if(col >= c0 && col < c0 + cols && row >= r0 && row < r0 + rows) return k;
    }
    return -1;
}

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void home_create(lv_obj_t * parent)
{
    lv_memzero(&h, sizeof(h));
    if(!s_wid_valid) widgets_default();
    lv_display_t * disp = lv_display_get_default();
    h.W = lv_display_get_horizontal_resolution(disp);
    h.H = lv_display_get_vertical_resolution(disp);
    h.gap = (h.W - HOME_COLS * ICON) / (HOME_COLS + 1);
    h.dock_y = h.H - DOCK_H - DOCK_BOTTOM;
    h.hit = -1;
    h.fplate_owner = -1;
    if(!s_layout_valid) {
        layout_default(s_layout);
        s_layout_valid = true;
    }

    h.root = ui_box(parent);
    lv_obj_set_size(h.root, h.W, h.H);
    lv_obj_set_clickable(h.root, true);
    lv_obj_set_press_lock(h.root, true);
    lv_obj_add_event_cb(h.root, root_event_cb, LV_EVENT_ALL, NULL);

    h.pages = ui_box(h.root);
    lv_obj_set_size(h.pages, h.W * HOME_PAGES, h.dock_y);

    lv_obj_t * dots = ui_box(h.root);
    lv_obj_set_size(dots, LV_SIZE_CONTENT, 8);
    lv_obj_set_flex_flow(dots, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(dots, 8, 0);
    lv_obj_align(dots, LV_ALIGN_TOP_MID, 0, h.dock_y - 18);
    for(int i = 0; i < HOME_PAGES; i++) {
        h.dots[i] = ui_box(dots);
        lv_obj_set_size(h.dots[i], 7, 7);
        lv_obj_set_style_radius(h.dots[i], LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(h.dots[i], lv_color_white(), 0);
    }

    if(s_veil) lv_obj_set_style_opa(h.root, LV_OPA_TRANSP, 0);
    h.dock = ui_box(h.root);
    lv_obj_set_size(h.dock, h.W - 16, DOCK_H);
    lv_obj_set_pos(h.dock, 8, h.dock_y);
    lv_obj_set_style_radius(h.dock, 26, 0);
    ui_glass(h.dock, 0xFFFFFF, LV_OPA_30, 20);

    for(int id = 0; id < os_app_count && id < OS_MAX_APPS; id++) {
        h.where[id] = -1;
        tile_create(id);
    }
    sdk_timer(h.root, clock_icon_tick, 50, NULL);
    clock_icon_tick(NULL);

    h.chrome = ui_box(parent);
    lv_obj_set_size(h.chrome, h.W, h.H);
    lv_obj_set_hidden(h.chrome, true);

    h.btn_add = ui_button(h.chrome, LV_SYMBOL_PLUS, 0x3A3A3C, add_event_cb, NULL);
    lv_obj_set_size(h.btn_add, 36, 28);
    lv_obj_set_pos(h.btn_add, 14, 10);
    lv_obj_set_style_radius(h.btn_add, 14, 0);
    lv_obj_set_hidden(h.btn_add, true);

    h.btn_done = ui_button(h.chrome, "Done", 0x3A3A3C, done_event_cb, NULL);
    lv_obj_set_size(h.btn_done, 68, 28);
    lv_obj_align(h.btn_done, LV_ALIGN_TOP_RIGHT, -14, 10);
    lv_obj_set_style_radius(h.btn_done, 14, 0);
    lv_obj_set_hidden(h.btn_done, true);

    h.btn_wall = ui_button(h.chrome, LV_SYMBOL_IMAGE "  Wallpaper", 0x3A3A3C, wall_event_cb, NULL);
    lv_obj_set_size(h.btn_wall, 150, 30);
    lv_obj_align(h.btn_wall, LV_ALIGN_BOTTOM_MID, 0, -6);
    lv_obj_set_style_radius(h.btn_wall, 15, 0);

    if(!s_wid_valid) widgets_default();
    widgets_build();

    h.spring = lv_timer_create(spring_cb, 8, NULL);
    lv_timer_pause(h.spring);
    layout_apply(s_layout, -1, false);
    page_go(0, false, 0);
}

static bool loc_on_page(int loc)
{
    return loc >= 0 && (loc >= HOME_LOC_DOCK0 || loc / HOME_SLOTS == h.page);
}

/* What an app window flies out of / back into: the icon in an open folder, the mini
 * icon inside a closed folder (iOS), or the app's own Home Screen icon. */
static lv_obj_t * icon_target(int id)
{
    if(id < 0 || id >= os_app_count) return NULL;
    if(h.folder_overlay && h.fcell[id]) return h.fcell[id];
    for(int owner = 0; owner < os_app_count; owner++) {
        if(s_folder[owner].count < 2) continue;
        for(int i = 0; i < s_folder[owner].count; i++) {
            if(s_folder[owner].member[i] != id) continue;
            if(!loc_on_page(h.where[owner])) return NULL;
            return h.mcell[owner][i] ? h.mcell[owner][i] : h.icon[owner];
        }
    }
    return loc_on_page(h.where[id]) ? h.icon[id] : NULL;
}

bool home_get_icon_area(int id, lv_area_t * out)
{
    lv_obj_t * o = icon_target(id);
    if(!o) return false;
    lv_obj_get_coords(o, out);
    return true;
}

void home_set_icon_hidden(int id, bool hidden)
{
    if(id < 0 || id >= os_app_count) return;
    lv_obj_t * o = icon_target(id);
    if(hidden) {
        /* never hide a whole folder icon, only the app's own slot in it */
        bool folder_icon = false;
        for(int owner = 0; owner < os_app_count; owner++)
            if(o == h.icon[owner] && s_folder[owner].count >= 2) folder_icon = true;
        if(o && !folder_icon) lv_obj_set_style_opa(o, LV_OPA_TRANSP, 0);
        return;
    }
    lv_obj_set_style_opa(h.icon[id], LV_OPA_COVER, 0);
    if(h.fcell[id]) lv_obj_set_style_opa(h.fcell[id], LV_OPA_COVER, 0);
    for(int owner = 0; owner < os_app_count; owner++)
        for(int i = 0; i < HOME_FOLDER_ITEMS; i++)
            if(h.mcell[owner][i]) lv_obj_set_style_opa(h.mcell[owner][i], LV_OPA_COVER, 0);
}

bool home_in_edit(void)
{
    return h.edit;
}

static void radius_exec(void * o, int32_t v)
{
    lv_obj_set_style_radius((lv_obj_t *)o, v, 0);
}

static void radius_to(int32_t to)
{
    lv_anim_delete(h.root, radius_exec);
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, h.root);
    lv_anim_set_exec_cb(&a, radius_exec);
    lv_anim_set_values(&a, lv_obj_get_style_radius(h.root, 0), to);
    lv_anim_set_duration(&a, 320);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    lv_anim_start(&a);
}

void home_card_sync(void)
{
    if(!h.card) return;
    lv_obj_t * scr = lv_screen_active();
    lv_obj_set_style_bg_color(h.root, lv_obj_get_style_bg_color(scr, 0), 0);
    lv_obj_set_style_bg_grad_color(h.root, lv_obj_get_style_bg_grad_color(scr, 0), 0);
    lv_obj_set_style_bg_grad_dir(h.root, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_bg_image_src(h.root, lv_obj_get_style_bg_image_src(scr, 0), 0);
    lv_obj_set_style_bg_opa(h.root, LV_OPA_COVER, 0);
    lv_obj_set_style_clip_corner(h.root, true, 0);
    lv_obj_set_style_bg_color(scr, lv_color_black(), 0);
    lv_obj_set_style_bg_grad_color(scr, lv_color_black(), 0);
    lv_obj_set_style_bg_image_src(scr, NULL, 0);
}

static void card_released_cb(lv_obj_t * o)
{
    LV_UNUSED(o);
    if(h.edit) return;
    h.card = false;
    lv_obj_set_style_bg_opa(h.root, LV_OPA_TRANSP, 0);
    lv_obj_set_style_bg_image_src(h.root, NULL, 0);
    lv_obj_set_style_clip_corner(h.root, false, 0);
    lv_obj_set_style_radius(h.root, 0, 0);
    shell_settings_apply();
}

static void get_point(lv_point_t * p)
{
    lv_indev_get_point(lv_indev_active(), p);
    if(!h.card) return;
    float s = fx_get(h.root, FX_SCALE) / 256.0f;
    float ty = fx_get(h.root, FX_Y);
    if(s < 0.2f) return;
    p->x = (int32_t)((p->x - h.W / 2) / s + h.W / 2);
    p->y = (int32_t)((p->y - h.H / 2 - ty) / s + h.H / 2);
}

static void chrome_pop(lv_obj_t * o, uint32_t delay)
{
    fx_set(o, FX_OPA, 0);
    fx_to(o, FX_OPA, 255, 0.25f, 1.0f, delay, NULL);
#if OS_FX_HEAVY
    fx_set(o, FX_SCALE, 80);
    fx_to(o, FX_SCALE, 256, 0.48f, 0.60f, delay, NULL);
#endif
}

static void card_enter(void)
{
    h.card = true;
    home_card_sync();
    lv_obj_set_style_radius(h.root, 0, 0);
    radius_to(CARD_RADIUS);
    lv_obj_set_hidden(h.chrome, false);
    fx_to(h.root, FX_Y, -4, 0.50f, 0.66f, 0, NULL);
#if OS_FX_HEAVY
    fx_to(h.root, FX_SCALE, CARD_SCALE, 0.50f, 0.66f, 0, NULL);
#endif
    chrome_pop(h.btn_add, 60);
    chrome_pop(h.btn_done, 100);
    chrome_pop(h.btn_wall, 160);
}

static void card_exit(void)
{
    radius_to(0);
    lv_obj_set_hidden(h.chrome, true);
    fx_to(h.root, FX_Y, 0, 0.46f, 0.60f, 0, card_released_cb);
#if OS_FX_HEAVY
    fx_to(h.root, FX_SCALE, 256, 0.46f, 0.60f, 0, NULL);

    int n = 0;
    for(int id = 0; id < os_app_count; id++) {
        if(h.where[id] < 0) continue;
        fx_set(h.tile[id], FX_SCALE, 190);
        fx_to(h.tile[id], FX_SCALE, 256, 0.48f, 0.60f, 90 + (uint32_t)(n++ % 8) * 22, NULL);
    }
#endif
}

void home_edit_exit(void)
{
    if(!h.edit) return;
    folder_close();
    ghost_hide();
    if(h.mode == T_DRAG) drag_drop();
    h.edit = false;
    sheet_close();
    for(int id = 0; id < os_app_count; id++) {
        jiggle(id, false);
        lv_obj_set_hidden(h.badge[id], true);
    }
    for(int k = 0; k < HOME_WIDGETS; k++) {
        if(h.wbadge[k]) lv_obj_set_hidden(h.wbadge[k], true);
        if(h.handle[k][0]) {
            lv_obj_set_hidden(h.handle[k][0], true);
            lv_obj_set_hidden(h.handle[k][1], true);
        }
    }
    lv_obj_set_hidden(h.btn_add, true);
    lv_obj_set_hidden(h.btn_done, true);
    card_exit();
    shell_statusbar_set_visible(true);
}

void home_reset_layout(void)
{
    folder_close();
    memset(s_folder, 0, sizeof(s_folder));
    layout_default(s_layout);
    if(h.root) {
        layout_apply(s_layout, -1, true);
        page_go(0, true, 0);
    }
}

void home_layout_export(int8_t out[HOME_LOC_COUNT])
{
    if(!s_layout_valid) {
        layout_default(s_layout);
        s_layout_valid = true;
    }
    memcpy(out, s_layout, HOME_LOC_COUNT);
}

void home_widgets_export(home_widget_t out[HOME_WIDGETS])
{
    if(!s_wid_valid) widgets_default();
    memcpy(out, s_wid, sizeof(s_wid));
}

void home_widgets_import(const home_widget_t in[HOME_WIDGETS])
{
    for(int k = 0; k < HOME_WIDGETS; k++) {
        if(in[k].widget >= os_widget_count || in[k].page < 0 || in[k].page >= HOME_PAGES ||
           in[k].cell < 0 || in[k].cell >= HOME_SLOTS) return;
    }
    memcpy(s_wid, in, sizeof(s_wid));
    s_wid_valid = true;
    if(h.root) widgets_build();
}

static void unlock_done_cb(lv_obj_t * o)
{
    lv_obj_set_style_transform_scale(o, 256, 0);
}

void home_lock_veil(bool on)
{
    s_veil = on;
    if(!h.root) return;
    if(on) {
        fx_stop(h.root);
        fx_set(h.root, FX_OPA, 0);
    }
    else if(!fx_busy(h.root, FX_OPA) && fx_get(h.root, FX_OPA) < 255.0f) {
        fx_set(h.root, FX_OPA, 255);
    }
}

void home_unlock_anim(void)
{
    if(!h.root) return;
    fx_set(h.root, FX_OPA, 0);
    fx_to(h.root, FX_OPA, 255, 0.18f, 1.0f, 0, NULL);
#if OS_FX_HEAVY

    lv_obj_update_layout(h.root);
    int32_t cx = h.W / 2, cy = h.H / 2;
    for(int id = 0; id < os_app_count; id++) {
        if(h.where[id] < 0 || !h.icon[id] || !h.tile[id]) continue;
        lv_area_t a;
        lv_obj_get_coords(h.tile[id], &a);
        float dx = (float)((a.x1 + a.x2) / 2 - cx), dy = (float)((a.y1 + a.y2) / 2 - cy);
        fx_set(h.icon[id], FX_X, dx * 1.15f);
        fx_set(h.icon[id], FX_Y, dy * 1.15f);
        fx_set(h.icon[id], FX_SCALE, 420);
        fx_to(h.icon[id], FX_X, 0, 0.46f, 0.74f, 0, NULL);
        fx_to(h.icon[id], FX_Y, 0, 0.46f, 0.74f, 0, NULL);
        fx_to(h.icon[id], FX_SCALE, 256, 0.46f, 0.74f, 0, NULL);
        lv_obj_t * name = lv_obj_get_child(h.tile[id], 1);
        if(name) {
            fx_set(name, FX_X, dx * 1.5f);
            fx_set(name, FX_Y, dy * 1.5f);
            fx_set(name, FX_OPA, 0);
            fx_to(name, FX_X, 0, 0.52f, 0.78f, 30, NULL);
            fx_to(name, FX_Y, 0, 0.52f, 0.78f, 30, NULL);
            fx_to(name, FX_OPA, 255, 0.22f, 1.0f, 60, NULL);
        }
    }
    if(h.dock) {
        fx_set(h.dock, FX_SCALE, 330);
        fx_set(h.dock, FX_Y, 50);
        fx_set(h.dock, FX_OPA, 0);
        fx_to(h.dock, FX_SCALE, 256, 0.46f, 0.74f, 0, NULL);
        fx_to(h.dock, FX_Y, 0, 0.46f, 0.74f, 0, NULL);
        fx_to(h.dock, FX_OPA, 255, 0.24f, 1.0f, 0, NULL);
    }
    for(int k = 0; k < HOME_WIDGETS; k++) {
        if(!h.widget[k]) continue;
        lv_area_t a;
        lv_obj_get_coords(h.widget[k], &a);
        float dx = (float)((a.x1 + a.x2) / 2 - cx), dy = (float)((a.y1 + a.y2) / 2 - cy);
        fx_set(h.widget[k], FX_X, dx * 0.7f);
        fx_set(h.widget[k], FX_Y, dy * 0.7f);
        fx_set(h.widget[k], FX_SCALE, 340);
        fx_to(h.widget[k], FX_X, 0, 0.46f, 0.74f, 0, NULL);
        fx_to(h.widget[k], FX_Y, 0, 0.46f, 0.74f, 0, NULL);
        fx_to(h.widget[k], FX_SCALE, 256, 0.46f, 0.74f, 0, NULL);
    }
#endif
}

void home_set_hidden(bool hidden)
{
    if(h.root) lv_obj_set_hidden(h.root, hidden);
}

void home_layout_import(const int8_t in[HOME_LOC_COUNT])
{
    bool seen[OS_MAX_APPS] = { false };
    for(int i = 0; i < HOME_LOC_COUNT; i++) {
        int id = in[i];
        if(id < -1 || id >= os_app_count || (id >= 0 && loc_blocked(i))) return;
        if(id >= 0) {
            if(seen[id]) return;
            seen[id] = true;
        }
    }
    memcpy(s_layout, in, HOME_LOC_COUNT);
    s_layout_valid = true;
}

lv_obj_t * home_get_icon_object(int id)
{
    return id >= 0 && id < os_app_count ? h.icon[id] : NULL;
}

void home_folders_export(int8_t members[OS_MAX_APPS][HOME_FOLDER_ITEMS], uint8_t counts[OS_MAX_APPS])
{
    for(int owner = 0; owner < OS_MAX_APPS; owner++) {
        counts[owner] = s_folder[owner].count;
        memcpy(members[owner], s_folder[owner].member, HOME_FOLDER_ITEMS);
    }
}

void home_folders_import(const int8_t members[OS_MAX_APPS][HOME_FOLDER_ITEMS], const uint8_t counts[OS_MAX_APPS])
{
    bool used[OS_MAX_APPS] = { false };
    for(int loc = 0; loc < HOME_LOC_COUNT; loc++)
        if(s_layout[loc] >= 0) used[s_layout[loc]] = true;
    for(int owner = 0; owner < os_app_count; owner++) {
        int count = counts[owner];
        if(count == 0) continue;
        if(count < 2 || count > HOME_FOLDER_ITEMS || !used[owner] || members[owner][0] != owner) return;
        for(int i = 1; i < count; i++) {
            int id = members[owner][i];
            if(id < 0 || id >= os_app_count || used[id]) return;
            used[id] = true;
        }
    }
    for(int owner = 0; owner < OS_MAX_APPS; owner++) {
        s_folder[owner].count = counts[owner];
        memcpy(s_folder[owner].member, members[owner], HOME_FOLDER_ITEMS);
    }
    if(h.root) layout_apply(s_layout, -1, false);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void layout_default(int8_t * l)
{
    memset(l, -1, HOME_LOC_COUNT);
    int grid = 0;
    for(int id = 0; id < os_app_count; id++) {
        if(id >= os_dock_start && id < os_dock_start + HOME_DOCK) {
            l[HOME_LOC_DOCK0 + (id - os_dock_start)] = (int8_t)id;
            continue;
        }
        int loc = 0;
        for(int k = 0; k <= grid; loc++) if(!loc_blocked(loc) && ++k > grid) break;
        if(loc < HOME_LOC_DOCK0) l[loc] = (int8_t)id;
        grid++;
    }
}

static void loc_pos(int loc, lv_obj_t ** parent, int32_t * x, int32_t * y)
{
    if(loc >= HOME_LOC_DOCK0) {
        int i = loc - HOME_LOC_DOCK0;
        *parent = h.root;
        int32_t width = h.dock_count * TILE_W + 28;
        *x = (h.W - width) / 2 + 14 + h.dock_rank[i] * TILE_W;
        *y = h.dock_y + (DOCK_H - ICON) / 2;
    }
    else {
        int page = loc / HOME_SLOTS;
        int s = loc % HOME_SLOTS;
        *parent = h.pages;
        *x = page * h.W + h.gap + (s % HOME_COLS) * (ICON + h.gap) - TILE_OX;
        *y = GRID_TOP + (s / HOME_COLS) * ROW_PITCH;
    }
}

static bool loc_blocked(int loc)
{
    return widget_at_loc(loc) >= 0;
}

static int loc_at(int32_t cx, int32_t cy)
{
    int32_t col = cx < h.gap / 2 ? 0 : (cx - h.gap / 2) / (ICON + h.gap);
    col = LV_CLAMP(0, col, HOME_COLS - 1);
    if(cy >= h.dock_y - 6) {
        const int8_t * layout = h.mode == T_DRAG ? h.preview : s_layout;
        int32_t width = h.dock_count * TILE_W + 28;
        int rank = LV_CLAMP(0, (cx - (h.W - width) / 2 - 14) / TILE_W, HOME_DOCK - 1);
        for(int i = 0; i < HOME_DOCK; i++)
            if(layout[HOME_LOC_DOCK0 + i] >= 0 && h.dock_rank[i] == rank) return HOME_LOC_DOCK0 + i;
        for(int i = 0; i < HOME_DOCK; i++) if(layout[HOME_LOC_DOCK0 + i] < 0) return HOME_LOC_DOCK0 + i;
        return HOME_LOC_DOCK0 + HOME_DOCK - 1;
    }

    int32_t ry = cy - GRID_TOP + (ROW_PITCH - ICON) / 2;
    int32_t row = ry < 0 ? 0 : ry / ROW_PITCH;
    row = LV_CLAMP(0, row, HOME_ROWS - 1);
    int loc = h.page * HOME_SLOTS + row * HOME_COLS + col;
    return loc_blocked(loc) ? -1 : loc;
}

static void layout_apply(const int8_t * l, int skip_id, bool anim)
{
    int old_count = h.dock_count;
    int8_t old_rank[HOME_DOCK];
    memcpy(old_rank, h.dock_rank, sizeof(old_rank));
    h.dock_count = 0;
    for(int i = 0; i < HOME_DOCK; i++) {
        h.dock_rank[i] = (int8_t)h.dock_count;
        if(l[HOME_LOC_DOCK0 + i] >= 0) h.dock_count++;
    }
    int32_t dock_width = h.dock_count * TILE_W + 28;
    lv_obj_set_hidden(h.dock, h.dock_count == 0);
    lv_obj_set_width(h.dock, dock_width);
    lv_obj_set_x(h.dock, (h.W - dock_width) / 2);
    int8_t where_new[OS_MAX_APPS];
    memset(where_new, -1, sizeof(where_new));
    for(int loc = 0; loc < HOME_LOC_COUNT; loc++) {
        if(l[loc] >= 0) where_new[l[loc]] = (int8_t)loc;
    }

    for(int id = 0; id < os_app_count; id++) {
        if(id == skip_id) continue;
        lv_obj_t * t = h.tile[id];
        folder_refresh_icon(id);
        if(where_new[id] < 0) {
            lv_obj_set_hidden(t, true);
            h.where[id] = -1;
            continue;
        }
        bool was_visible = !lv_obj_is_hidden(t);
        lv_obj_set_hidden(t, false);
        bool dock_shift = where_new[id] >= HOME_LOC_DOCK0 &&
                          (old_count != h.dock_count || old_rank[where_new[id] - HOME_LOC_DOCK0] !=
                                                       h.dock_rank[where_new[id] - HOME_LOC_DOCK0]);
        if(where_new[id] != h.where[id] || !anim || dock_shift)
            tile_move(id, where_new[id], anim && was_visible);
        h.where[id] = where_new[id];
        lv_obj_set_hidden(h.badge[id], !h.edit);
    }
}

static void clock_icon_tick(lv_timer_t * t)
{
    LV_UNUSED(t);
    if(!h.clock_hand[2] || notify_banner_active()) return;
    time_t now = time(NULL);
    struct tm * local = localtime(&now);
    if(!local) return;
    static time_t last_second;
    static uint32_t changed_at;
    if(now != last_second) {
        last_second = now;
        changed_at = lv_tick_get();
    }
    int32_t fraction = os_settings.low_power ? 0 : LV_MIN(lv_tick_elaps(changed_at), 999);
    float angle[3] = {
        ((local->tm_hour % 12) + local->tm_min / 60.0f) * (float)M_PI / 6.0f,
        (local->tm_min + local->tm_sec / 60.0f) * (float)M_PI / 30.0f,
        (local->tm_sec + fraction / 1000.0f) * (float)M_PI / 30.0f
    };

    const float tip[3] = { 14.0f, 22.0f, 23.5f };
    const float tail[3] = { 0.0f, 0.0f, 7.0f };
    for(int i = 0; i < 3; i++) {
        float sn = sinf(angle[i]), cs = cosf(angle[i]);
        h.clock_points[i][0].x = 30 - sn * tail[i];
        h.clock_points[i][0].y = 30 + cs * tail[i];
        h.clock_points[i][1].x = 30 + sn * tip[i];
        h.clock_points[i][1].y = 30 - cs * tip[i];
#ifdef S3_NATIVE_LAYOUT
        for(int p=0;p<2;p++) { h.clock_points[i][p].x/=3; h.clock_points[i][p].y/=3; }
#endif
        lv_line_set_points_mutable(h.clock_hand[i], h.clock_points[i], 2);
    }
}

static void tile_create(int id)
{
    const app_desc_t * app = os_apps[id];

    lv_obj_t * t = ui_box(h.pages);
    lv_obj_set_size(t, TILE_W, TILE_H);
    lv_obj_set_overflow_visible(t, true);
    lv_obj_set_hidden(t, true);

    lv_obj_t * icon = ui_box(t);
    lv_obj_set_size(icon, ICON, ICON);
    lv_obj_set_pos(icon, TILE_OX, 0);
    lv_obj_set_style_radius(icon, ICON_R, 0);
    ui_icon_style(icon, app->color);
    lv_obj_set_style_transform_pivot_x(icon, ICON / 2, 0);
    lv_obj_set_style_transform_pivot_y(icon, ICON / 2, 0);
    if(app == &app_clock) {
        lv_obj_t * symbol = ui_label(icon, app->symbol, &lv_font_montserrat_28, 0xFFFFFF);
        lv_obj_center(symbol);
        lv_obj_set_hidden(symbol, true);
        lv_obj_set_style_bg_color(icon, lv_color_white(), 0);
        lv_obj_set_style_bg_grad_color(icon, lv_color_white(), 0);
        for(int m = 0; m < 12; m++) {
            float sn = sinf(m * (float)M_PI / 6.0f), cs = cosf(m * (float)M_PI / 6.0f);
            float r0 = m % 3 == 0 ? 21.5f : 23.5f, r1 = 27.0f;
            h.clock_ticks[m][0].x = 30 + sn * r0;
            h.clock_ticks[m][0].y = 30 - cs * r0;
            h.clock_ticks[m][1].x = 30 + sn * r1;
            h.clock_ticks[m][1].y = 30 - cs * r1;
#ifdef S3_NATIVE_LAYOUT
            for(int p=0;p<2;p++) { h.clock_ticks[m][p].x/=3; h.clock_ticks[m][p].y/=3; }
#endif
            lv_obj_t * tk = lv_line_create(icon);
            lv_obj_set_size(tk, ICON, ICON);
            lv_obj_set_pos(tk, 0, 0);
            lv_obj_set_style_line_width(tk, m % 3 == 0 ? 3 : 2, 0);
            lv_obj_set_style_line_color(tk, lv_color_hex(m % 3 == 0 ? 0x1C1C1E : 0x8E8E93), 0);
            lv_obj_set_style_line_rounded(tk, false, 0);
            lv_line_set_points(tk, h.clock_ticks[m], 2);
        }
        static const int8_t hw[3] = { 4, 3, 2 };
        for(int hand = 0; hand < 3; hand++) {
            lv_obj_t * line = lv_line_create(icon);
            lv_obj_set_size(line, ICON, ICON);
            lv_obj_set_pos(line, 0, 0);
            lv_obj_set_style_line_width(line, hw[hand], 0);
            lv_obj_set_style_line_color(line, hand == 2 ? lv_color_hex(0xFF9500) : lv_color_hex(0x1C1C1E), 0);
            lv_obj_set_style_line_rounded(line, false, 0);
            h.clock_hand[hand] = line;
        }
        lv_obj_t * hub = ui_box(icon);
        lv_obj_set_size(hub, 6, 6);
        lv_obj_set_pos(hub, 27, 27);
        lv_obj_set_style_radius(hub, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(hub, lv_color_hex(0xFF9500), 0);
        lv_obj_set_style_bg_opa(hub, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(hub, 0, 0);
    }
    else lv_obj_center(ui_label(icon, app->symbol, &lv_font_montserrat_28, 0xFFFFFF));

    lv_obj_t * name = ui_label(t, app->name, &lv_font_montserrat_12, 0xFFFFFF);
    lv_obj_set_width(name, TILE_W);
    lv_label_set_long_mode(name, LV_LABEL_LONG_MODE_DOTS);
    lv_obj_set_style_text_align(name, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_pos(name, 0, ICON + 5);

    lv_obj_t * badge = ui_box(t);
    lv_obj_set_size(badge, 22, 22);
    lv_obj_set_pos(badge, TILE_OX - 8, -8);
    lv_obj_set_style_radius(badge, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(badge, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(badge, lv_color_hex(0x5A5A5E), 0);
    lv_obj_set_style_border_width(badge, 1, 0);
    lv_obj_set_style_border_color(badge, lv_color_hex(0x8E8E93), 0);
    lv_obj_center(ui_label(badge, LV_SYMBOL_MINUS, &lv_font_montserrat_12, 0xFFFFFF));
    lv_obj_set_hidden(badge, true);

    h.tile[id] = t;
    h.icon[id] = icon;
    h.label[id] = name;
    h.badge[id] = badge;
}

static lv_obj_t * mini_icon(lv_obj_t * parent, int id, int32_t size)
{
    lv_obj_t * m = ui_box(parent);
    lv_obj_set_size(m, size, size);
    lv_obj_set_style_radius(m, size / 4 + 1, 0);
    uint32_t color = os_apps[id] == &app_clock ? 0xFFFFFF : os_apps[id]->color;
    ui_icon_style(m, color);
    lv_obj_set_style_shadow_width(m, 0, 0);
    lv_obj_t * l = ui_label(m, os_apps[id]->symbol, &lv_font_montserrat_12,
                            os_apps[id] == &app_clock ? 0x1C1C1E : 0xFFFFFF);
    lv_obj_center(l);
    lv_obj_set_style_transform_pivot_x(l, LV_PCT(50), 0);
    lv_obj_set_style_transform_pivot_y(l, LV_PCT(50), 0);
    lv_obj_set_style_transform_scale(l, size * 256 / 22, 0);
    return m;
}

static void folder_refresh_icon(int owner)
{
    if(owner < 0 || owner >= os_app_count || !h.icon[owner]) return;
    bool folder = s_folder[owner].count >= 2;
    lv_obj_t * icon = h.icon[owner];
    if(h.mini[owner]) {
        lv_obj_delete(h.mini[owner]);
        h.mini[owner] = NULL;
    }
    for(int i = 0; i < HOME_FOLDER_ITEMS; i++) h.mcell[owner][i] = NULL;

    uint32_t n = lv_obj_get_child_count(icon);
    for(uint32_t i = 0; i < n; i++) {
        lv_obj_t * c = lv_obj_get_child(icon, (int32_t)i);
        bool clock_symbol = os_apps[owner] == &app_clock && i == 0;
        lv_obj_set_hidden(c, folder || clock_symbol);
    }
    if(!folder) {
        ui_icon_style(icon, os_apps[owner] == &app_clock ? 0xFFFFFF : os_apps[owner]->color);
        lv_label_set_text(h.label[owner], os_apps[owner]->name);
        return;
    }
    /* frosted folder plate */
    lv_obj_set_style_bg_color(icon, lv_color_hex(0xD8DEE8), 0);
    lv_obj_set_style_bg_grad_color(icon, lv_color_hex(0xB8C2D2), 0);
    lv_obj_set_style_bg_opa(icon, LV_OPA_50, 0);
    lv_obj_set_style_border_width(icon, 1, 0);
    lv_obj_set_style_border_color(icon, lv_color_white(), 0);
    lv_obj_set_style_border_opa(icon, LV_OPA_30, 0);

    lv_obj_t * grid = ui_box(icon);
    lv_obj_set_size(grid, ICON, ICON);
    lv_obj_set_pos(grid, 0, 0);
    h.mini[owner] = grid;
    const int32_t cell = 13, gap = 4, org = (ICON - 3 * cell - 2 * gap) / 2;
    for(int i = 0; i < s_folder[owner].count && i < 9; i++) {
        lv_obj_t * m = mini_icon(grid, s_folder[owner].member[i], cell);
        lv_obj_set_pos(m, org + (i % 3) * (cell + gap), org + (i / 3) * (cell + gap));
        h.mcell[owner][i] = m;
    }
    lv_label_set_text(h.label[owner], "Folder");
}

static void x_exec(void * o, int32_t v)
{
    lv_obj_set_x((lv_obj_t *)o, v);
}

static void y_exec(void * o, int32_t v)
{
    lv_obj_set_y((lv_obj_t *)o, v);
}

static void reparent_keep_pos(lv_obj_t * obj, lv_obj_t * parent)
{
    if(lv_obj_get_parent(obj) == parent) return;
    lv_area_t a, pa;
    lv_obj_get_coords(obj, &a);
    lv_obj_set_parent(obj, parent);
    lv_obj_get_coords(parent, &pa);
    lv_obj_set_pos(obj, a.x1 - pa.x1, a.y1 - pa.y1);
}

static void tile_move(int id, int loc, bool anim)
{
    lv_obj_t * t = h.tile[id];
    lv_obj_t * parent;
    int32_t x, y;
    loc_pos(loc, &parent, &x, &y);

    lv_anim_delete(t, x_exec);
    lv_anim_delete(t, y_exec);
    reparent_keep_pos(t, parent);
    lv_obj_set_hidden(h.label[id], loc >= HOME_LOC_DOCK0);

    if(!anim) {
        lv_obj_set_pos(t, x, y);
        return;
    }
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, t);
    lv_anim_set_duration(&a, MOVE_ANIM_MS);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    lv_anim_set_exec_cb(&a, x_exec);
    lv_anim_set_values(&a, lv_obj_get_x(t), x);
    lv_anim_start(&a);
    lv_anim_set_exec_cb(&a, y_exec);
    lv_anim_set_values(&a, lv_obj_get_y(t), y);
    lv_anim_start(&a);
}

static bool point_in(const lv_area_t * a, lv_point_t p)
{
    return p.x >= a->x1 && p.x <= a->x2 && p.y >= a->y1 && p.y <= a->y2;
}

static int hit_test(lv_point_t p, bool * on_badge)
{
    *on_badge = false;
    h.hit_widget = -1;
    h.hit_wbadge = false;
    h.hit_handle = 0;
    for(int k = 0; k < HOME_WIDGETS; k++) {
        lv_area_t a;
        if(!h.widget[k] || s_wid[k].page != h.page) continue;
        if(h.edit && handle_hit(k, p, &h.hit_handle)) {
            h.hit_widget = k;
            return -1;
        }
        if(h.edit) {
            lv_obj_get_coords(h.wbadge[k], &a);
            lv_area_increase(&a, 6, 6);
            if(point_in(&a, p)) {
                h.hit_widget = k;
                h.hit_wbadge = true;
                return -1;
            }
        }
        lv_obj_get_coords(h.widget[k], &a);
        if(point_in(&a, p)) {
            h.hit_widget = k;
            return -1;
        }
    }
    for(int id = 0; id < os_app_count; id++) {
        int loc = h.where[id];
        if(loc < 0) continue;
        if(loc < HOME_LOC_DOCK0 && loc / HOME_SLOTS != h.page) continue;

        lv_area_t a;
        if(h.edit) {
            lv_obj_get_coords(h.badge[id], &a);
            lv_area_increase(&a, 6, 6);
            if(point_in(&a, p)) {
                *on_badge = true;
                return id;
            }
        }
        lv_obj_get_coords(h.icon[id], &a);
        lv_area_increase(&a, 4, 4);
        if(point_in(&a, p)) return id;
    }
    return -1;
}

static void press_visual(int id, bool pressed)
{
    if(id < 0) return;
    lv_obj_set_style_transform_scale(h.icon[id], pressed ? 236 : 256, 0);
    lv_obj_set_style_opa(h.icon[id], pressed ? LV_OPA_80 : LV_OPA_COVER, 0);
}

static void dots_refresh(void)
{
    for(int i = 0; i < HOME_PAGES; i++) {
        lv_obj_set_style_bg_opa(h.dots[i], i == h.page ? LV_OPA_COVER : LV_OPA_40, 0);
    }
}

static void spring_cb(lv_timer_t * t)
{
    LV_UNUSED(t);
    float dt = LV_MIN(lv_tick_elaps(h.spring_tick), 50) / 1000.0f;
    h.spring_tick = lv_tick_get();
    float w = 2.0f * 3.14159265f / PAGE_RESPONSE;
    float k = w * w, c = 2.0f * PAGE_DAMPING * w;
    float target = (float)(-h.page * h.W);
    while(dt > 0.0f) {
        float st = LV_MIN(dt, 0.004f);
        h.pv += (-k * (h.px - target) - c * h.pv) * st;
        h.px += h.pv * st;
        dt -= st;
    }
    if(fabsf(h.px - target) < 0.5f && fabsf(h.pv) < 10.0f) {
        h.px = target;
        h.pv = 0;
        lv_timer_pause(h.spring);
    }
    lv_obj_set_x(h.pages, (int32_t)lroundf(h.px));
}

static void spring_stop(void)
{
    if(h.spring) lv_timer_pause(h.spring);
    h.px = (float)lv_obj_get_x(h.pages);
}

static void page_go(int page, bool anim, float velocity)
{
    h.page = LV_CLAMP(0, page, HOME_PAGES - 1);
    dots_refresh();
    if(!anim) {
        spring_stop();
        h.px = (float)(-h.page * h.W);
        lv_obj_set_x(h.pages, -h.page * h.W);
        return;
    }
    h.px = (float)lv_obj_get_x(h.pages);
    h.pv = velocity;
    h.spring_tick = lv_tick_get();
    lv_timer_resume(h.spring);
}

static void rot_exec(void * o, int32_t v)
{
    lv_obj_set_style_transform_rotation((lv_obj_t *)o, v, 0);
}

static void jiggle(int id, bool on)
{
    lv_obj_t * icon = h.icon[id];
    lv_anim_delete(icon, rot_exec);
    lv_obj_set_style_transform_rotation(icon, 0, 0);
    if(!on) return;

    uint32_t d = 110 + lv_rand(0, 40);
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, icon);
    lv_anim_set_exec_cb(&a, rot_exec);
    lv_anim_set_values(&a, -22, 22);
    lv_anim_set_duration(&a, d);
    lv_anim_set_reverse_duration(&a, d);
    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_delay(&a, lv_rand(0, 120));
    lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);
    lv_anim_start(&a);
}

void home_edit_enter(void)
{
    if(h.edit) return;
    h.edit = true;
    for(int id = 0; id < os_app_count; id++) {
        if(h.where[id] < 0) continue;
        jiggle(id, true);
        lv_obj_set_hidden(h.badge[id], false);
    }
    for(int k = 0; k < HOME_WIDGETS; k++) {
        if(!h.wbadge[k]) continue;
        lv_obj_set_hidden(h.wbadge[k], false);
        if(h.handle[k][0]) {
            lv_obj_set_hidden(h.handle[k][0], false);
            lv_obj_set_hidden(h.handle[k][1], false);
            fx_set(h.handle[k][0], FX_OPA, 0);
            fx_set(h.handle[k][1], FX_OPA, 0);
            fx_to(h.handle[k][0], FX_OPA, 255, 0.3f, 1.0f, 200, NULL);
            fx_to(h.handle[k][1], FX_OPA, 255, 0.3f, 1.0f, 240, NULL);
        }
        fx_set(h.widget[k], FX_SCALE, 240);
        fx_to(h.widget[k], FX_SCALE, 256, FX_BOUNCY, k * 30, NULL);
    }
    lv_obj_set_hidden(h.btn_add, false);
    lv_obj_set_hidden(h.btn_done, false);
    card_enter();
    shell_statusbar_set_visible(false);
}

static void drag_start(lv_point_t p)
{
    int id = h.hit;
    h.mode = T_DRAG;
    h.drag_id = id;
    h.folder_hover_loc = -1;
    h.folder_ready = false;
    h.reflow_loc = -1;
    h.drag_from = h.where[id];
    h.target = h.drag_from;
    h.edge_ms = 0;
    memcpy(h.preview, s_layout, HOME_LOC_COUNT);

    lv_obj_t * t = h.tile[id];
    lv_anim_delete(t, x_exec);
    lv_anim_delete(t, y_exec);
    reparent_keep_pos(t, h.root);
    lv_obj_move_foreground(t);

    lv_area_t a;
    lv_obj_get_coords(t, &a);
    h.grab.x = p.x - a.x1;
    h.grab.y = p.y - a.y1;

    jiggle(id, false);
    press_visual(id, false);
    lv_obj_set_style_transform_scale(h.icon[id], 290, 0);
    lv_obj_set_style_opa(h.icon[id], LV_OPA_90, 0);
    lv_obj_set_hidden(h.label[id], true);
    lv_obj_set_hidden(h.badge[id], true);
}

/* Folder hover: the app under the finger shrinks and a frosted folder outline pops out behind it */
static void fplate_gone_cb(lv_obj_t * o)
{
    lv_obj_delete_async(o);
}

static void fplate_hide(void)
{
    if(h.fplate) {
        /* glide out instead of vanishing: shrink toward the icon while fading */
        lv_obj_t * pl = h.fplate;
        fx_to(pl, FX_SCALE, 214, 0.30f, 1.0f, 0, NULL);
        fx_to(pl, FX_OPA, 0, 0.26f, 1.0f, 0, fplate_gone_cb);
        h.fplate = NULL;
        if(h.fplate_owner >= 0) fx_to(h.icon[h.fplate_owner], FX_SCALE, 256, 0.36f, 0.62f, 0, NULL);
    }
    h.fplate_owner = -1;
}

static void fplate_show(int owner)
{
    if(h.fplate && h.fplate_owner == owner) return;
    fplate_hide();
    h.fplate_owner = owner;
    lv_obj_t * pl = ui_box(h.tile[owner]);
    lv_obj_move_to_index(pl, 0);
    lv_obj_set_size(pl, ICON + 14, ICON + 14);
    lv_obj_set_pos(pl, TILE_OX - 7, -7);
    lv_obj_set_style_radius(pl, ICON_R + 4, 0);
    lv_obj_set_style_bg_color(pl, lv_color_hex(0xE4E9F2), 0);
    lv_obj_set_style_bg_opa(pl, LV_OPA_50, 0);
    lv_obj_set_style_border_width(pl, 1, 0);
    lv_obj_set_style_border_color(pl, lv_color_white(), 0);
    lv_obj_set_style_border_opa(pl, LV_OPA_40, 0);
    h.fplate = pl;
    fx_set(pl, FX_SCALE, 150);
    fx_to(pl, FX_SCALE, 256, 0.40f, 0.58f, 0, NULL);
    if(s_folder[owner].count < 2) fx_to(h.icon[owner], FX_SCALE, 196, 0.36f, 0.62f, 0, NULL);
}

static void drag_move(lv_point_t p, uint32_t el)
{
    lv_obj_t * t = h.tile[h.drag_id];
    int32_t x = p.x - h.grab.x;
    int32_t y = p.y - h.grab.y;
    lv_obj_set_pos(t, x, y);

    int32_t cx = x + TILE_OX + ICON / 2;
    int32_t cy = y + ICON / 2;

    int dir = p.x < EDGE_PX ? -1 : (p.x > h.W - EDGE_PX ? 1 : 0);
    int np = h.page + dir;
    if(dir != 0 && np >= 0 && np < HOME_PAGES && cy < h.dock_y) {
        h.edge_ms += el;
        if(h.edge_ms >= EDGE_FLIP_MS) {
            h.edge_ms = 0;
            page_go(np, true, 0);
            h.target = -1;
            fplate_hide();
        }
    }
    else {
        h.edge_ms = 0;
    }

    int target = loc_at(cx, cy);
    /* Over the middle of another app (as currently shown): hold still -> folder. No reflow while hovering. */
    int under = target >= 0 ? h.preview[target] : -1;
    bool over_icon = false;
    if(under >= 0 && under != h.drag_id && s_folder[h.drag_id].count < 2 &&
       s_folder[under].count < HOME_FOLDER_ITEMS) {
        lv_obj_t * parent;
        int32_t tx, ty;
        loc_pos(target, &parent, &tx, &ty);
        if(parent == h.pages) tx += lv_obj_get_x(h.pages);
        over_icon = LV_ABS(cx - (tx + TILE_OX + ICON / 2)) < 22 && LV_ABS(cy - (ty + ICON / 2)) < 22;
    }
    if(over_icon) {
        if(h.folder_hover_loc != target) {
            h.folder_hover_loc = target;
            h.folder_hover_tick = lv_tick_get();
            h.folder_ready = false;
            fplate_hide();
        }
        else if(!h.folder_ready && lv_tick_elaps(h.folder_hover_tick) > 1000) {
            h.folder_ready = true;
            fplate_show(under);
        }
        return;
    }
    if(h.folder_hover_loc >= 0) {
        h.folder_hover_loc = -1;
        h.folder_ready = false;
        fplate_hide();
    }
    if(target < 0 || target == h.target) {
        h.reflow_loc = -1;
        return;
    }
    /* Entering an occupied cell: wait a beat before pushing its app aside, so there is time to settle on its
     * centre and make a folder instead. Empty cells reflow at once. */
    if(under >= 0 && under != h.drag_id) {
        if(h.reflow_loc != target) {
            h.reflow_loc = target;
            h.reflow_tick = lv_tick_get();
            return;
        }
        if(lv_tick_elaps(h.reflow_tick) < 420) return;
    }
    h.reflow_loc = -1;
    h.target = target;

    int8_t * l = h.preview;
    memcpy(l, s_layout, HOME_LOC_COUNT);
    l[h.drag_from] = -1;
    if(target != h.drag_from && l[target] >= 0) l[h.drag_from] = l[target];
    l[target] = (int8_t)h.drag_id;
    layout_apply(l, h.drag_id, true);
}

static void sink_gone_cb(lv_obj_t * o)
{
    lv_draw_buf_t * b = (lv_draw_buf_t *)lv_image_get_src(o);
    lv_obj_delete(o);
    if(b) {
        lv_image_cache_drop(b);
        lv_draw_buf_destroy(b);
    }
}

static void drag_drop(void)
{
    int id = h.drag_id;
    int owner = h.folder_ready && h.folder_hover_loc >= 0 ? h.preview[h.folder_hover_loc] : -1;
    bool into_folder = false;
    lv_area_t from;
    lv_obj_get_coords(h.icon[id], &from);
    if(owner >= 0 && owner != id) {
        /* commit the layout as shown (the folder target may have been reflowed), minus the dragged app */
        int8_t l[HOME_LOC_COUNT];
        memcpy(l, h.preview, HOME_LOC_COUNT);
        for(int i = 0; i < HOME_LOC_COUNT; i++) if(l[i] == id) l[i] = -1;
        if(folder_add(owner, id)) {
            memcpy(s_layout, l, HOME_LOC_COUNT);
            into_folder = true;
        }
    }
    if(!into_folder) memcpy(s_layout, h.preview, HOME_LOC_COUNT);
    fplate_hide();
    h.mode = T_IDLE;

    lv_draw_buf_t * snap = into_folder ? lv_snapshot_take(h.icon[id], LV_COLOR_FORMAT_ARGB8888) : NULL;
    lv_obj_set_style_transform_scale(h.icon[id], 256, 0);
    lv_obj_set_style_opa(h.icon[id], LV_OPA_COVER, 0);
    h.where[id] = -1;
    layout_apply(s_layout, -1, true);
    if(into_folder) {
        /* the dragged app sinks into the folder; the folder pops */
        lv_area_t to;
        lv_obj_get_coords(h.icon[owner], &to);
        if(snap) {
            lv_obj_t * g = lv_image_create(h.root);
            lv_image_set_src(g, snap);
            lv_obj_set_pos(g, from.x1 - lv_obj_get_x(h.root), from.y1 - lv_obj_get_y(h.root));
            fx_to(g, FX_X, (float)((to.x1 + to.x2) / 2 - (from.x1 + from.x2) / 2), 0.34f, 0.9f, 0, NULL);
            fx_to(g, FX_Y, (float)((to.y1 + to.y2) / 2 - (from.y1 + from.y2) / 2), 0.34f, 0.9f, 0, NULL);
            fx_to(g, FX_SCALE, 60, 0.34f, 0.9f, 0, NULL);
            fx_to(g, FX_OPA, 0, 0.30f, 1.0f, 120, sink_gone_cb);
        }
        fx_set(h.icon[owner], FX_SCALE, 236);
        fx_to(h.icon[owner], FX_SCALE, 256, 0.36f, 0.90f, 120, NULL);
    }
    if(h.edit) jiggle(id, true);
    os_save();
}

static int folder_free_loc(void)
{
    for(int loc = 0; loc < HOME_LOC_DOCK0; loc++)
        if(s_layout[loc] < 0 && !loc_blocked(loc)) return loc;
    return -1;
}

static bool folder_add(int owner, int id)
{
    if(owner < 0 || owner >= os_app_count || id < 0 || id >= os_app_count || owner == id ||
       s_folder[owner].count >= HOME_FOLDER_ITEMS) return false;
    if(!s_folder[owner].count) {
        s_folder[owner].member[0] = (int8_t)owner;
        s_folder[owner].count = 1;
    }
    for(int i = 0; i < s_folder[owner].count; i++) if(s_folder[owner].member[i] == id) return false;
    s_folder[owner].member[s_folder[owner].count++] = (int8_t)id;
    folder_refresh_icon(owner);
    return true;
}

static void folder_dissolve(int owner, bool remove_owner)
{
    if(owner < 0 || owner >= os_app_count || s_folder[owner].count < 2) return;
    int n = s_folder[owner].count;
    int8_t members[HOME_FOLDER_ITEMS];
    memcpy(members, s_folder[owner].member, (size_t)n);
    s_folder[owner].count = 0;
    if(remove_owner) for(int loc = 0; loc < HOME_LOC_COUNT; loc++)
        if(s_layout[loc] == owner) s_layout[loc] = -1;
    for(int i = 1; i < n; i++) {
        int loc = folder_free_loc();
        if(loc >= 0) s_layout[loc] = members[i];
    }
    folder_refresh_icon(owner);
}

static void app_remove(int id)
{
    folder_dissolve(id, true);
    for(int loc = 0; loc < HOME_LOC_COUNT; loc++) {
        if(s_layout[loc] == id) s_layout[loc] = -1;
    }
    jiggle(id, false);
    layout_apply(s_layout, -1, true);
    os_save();
}

static void app_add(int id)
{
    int order[HOME_LOC_COUNT];
    int n = 0;
    for(int s = 0; s < HOME_SLOTS; s++) order[n++] = h.page * HOME_SLOTS + s;
    for(int loc = 0; loc < HOME_LOC_DOCK0; loc++) if(loc / HOME_SLOTS != h.page) order[n++] = loc;
    for(int i = 0; i < HOME_DOCK; i++) order[n++] = HOME_LOC_DOCK0 + i;

    for(int i = 0; i < n; i++) {
        int loc = order[i];
        if(s_layout[loc] >= 0 || loc_blocked(loc)) continue;
        s_layout[loc] = (int8_t)id;
        layout_apply(s_layout, -1, false);
        if(loc < HOME_LOC_DOCK0) page_go(loc / HOME_SLOTS, true, 0);
        if(h.edit) jiggle(id, true);
        os_save();
        return;
    }
}

static void sheet_open(void)
{
    if(h.sheet) return;
    h.sheet = ui_box(h.chrome);
    lv_obj_set_size(h.sheet, h.W, h.H);
    lv_obj_set_style_bg_color(h.sheet, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(h.sheet, LV_OPA_60, 0);
    lv_obj_set_clickable(h.sheet, true);
    lv_obj_add_event_cb(h.sheet, sheet_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t * panel = ui_box(h.sheet);
    lv_obj_set_width(panel, h.W - 24);
    lv_obj_set_height(panel, LV_SIZE_CONTENT);
    lv_obj_set_style_max_height(panel, h.H - 120, 0);
    lv_obj_align(panel, LV_ALIGN_BOTTOM_MID, 0, -OS_GESTURE_H - 10);
    lv_obj_set_style_radius(panel, 20, 0);
    lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(panel, lv_color_hex(UI_GROUP), 0);
    lv_obj_set_style_pad_all(panel, 14, 0);
    lv_obj_set_style_pad_row(panel, 6, 0);
    lv_obj_set_flex_flow(panel, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_clickable(panel, true);
    lv_obj_set_scrollable(panel, true);
    lv_obj_set_scroll_dir(panel, LV_DIR_VER);

    ui_label(panel, "Add to Home Screen", &lv_font_montserrat_18, 0xFFFFFF);

    int n = 0;
    for(int id = 0; id < os_app_count; id++) {
        if(h.where[id] >= 0) continue;
        const app_desc_t * app = os_apps[id];
        lv_obj_t * row = ui_box(panel);
        lv_obj_set_size(row, LV_PCT(100), 46);
        lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_column(row, 12, 0);
        lv_obj_set_style_radius(row, 10, 0);
        lv_obj_set_style_bg_color(row, lv_color_hex(UI_SEP), LV_STATE_PRESSED);
        lv_obj_set_style_bg_opa(row, LV_OPA_COVER, LV_STATE_PRESSED);
        lv_obj_set_clickable(row, true);
        lv_obj_add_event_cb(row, sheet_row_event_cb, LV_EVENT_CLICKED, (void *)(intptr_t)id);

        lv_obj_t * ic = ui_box(row);
        lv_obj_set_size(ic, 34, 34);
        lv_obj_set_style_radius(ic, 9, 0);
        ui_icon_style(ic, app->color);
        lv_obj_center(ui_label(ic, app->symbol, &lv_font_montserrat_16, 0xFFFFFF));
        ui_label(row, app->name, &lv_font_montserrat_16, UI_TEXT);
        n++;
    }
    if(n == 0) ui_label(panel, "All apps are already on your Home Screen.", &lv_font_montserrat_14, UI_DIM);

    ui_label(panel, "Widgets", &lv_font_montserrat_18, 0xFFFFFF);
    static const char * const k_size[] = { "Small", "Medium", "Large" };
    for(int w = 0; w < os_widget_count; w++) {
        const widget_desc_t * wd = os_widgets[w];
        lv_obj_t * row = ui_box(panel);
        lv_obj_set_size(row, LV_PCT(100), 46);
        lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_column(row, 12, 0);
        lv_obj_set_style_radius(row, 10, 0);
        lv_obj_set_style_bg_color(row, lv_color_hex(UI_SEP), LV_STATE_PRESSED);
        lv_obj_set_style_bg_opa(row, LV_OPA_COVER, LV_STATE_PRESSED);
        lv_obj_set_clickable(row, true);
        lv_obj_add_event_cb(row, sheet_row_event_cb, LV_EVENT_CLICKED, (void *)(intptr_t)(1000 + w));
        lv_obj_t * ic = ui_box(row);
        lv_obj_set_size(ic, wd->size == WIDGET_SMALL ? 34 : 50, 34);
        lv_obj_set_style_radius(ic, 9, 0);
        ui_icon_style(ic, wd->app ? wd->app->color : 0x3A3A3C);
        if(wd->app) lv_obj_center(ui_label(ic, wd->app->symbol, &lv_font_montserrat_16, 0xFFFFFF));
        lv_obj_t * l = ui_label(row, "", &lv_font_montserrat_16, UI_TEXT);
        lv_label_set_text_fmt(l, "%s  #8E8E93 %s#", wd->name, k_size[wd->size]);
        lv_label_set_recolor(l, true);
    }
}

static void sheet_close(void)
{
    if(!h.sheet) return;
    lv_obj_delete_async(h.sheet);
    h.sheet = NULL;
}

/* Folder morph: the panel's rect, radius, inner grid scale and the backdrop blur all follow one progress value,
 * so opening grows the folder icon into the panel and closing shrinks it back into the icon (iOS). */
static float lerpf(float a, float b, float t)
{
    return a + (b - a) * t;
}

static void folder_morph_exec(void * var, int32_t v)
{
    LV_UNUSED(var);
    if(!h.folder_panel) return;
    float t = v / 1024.0f;
    lv_area_t * a = &h.folder_from, * b = &h.folder_to;
    float tp = LV_CLAMP(0.0f, t, 1.0f);           /* centre never overshoots; only the size does */
    float cx = lerpf((a->x1 + a->x2) / 2.0f, (b->x1 + b->x2) / 2.0f, tp);
    float cy = lerpf((a->y1 + a->y2) / 2.0f, (b->y1 + b->y2) / 2.0f, tp);
    float hw = lerpf(lv_area_get_width(a) / 2.0f, lv_area_get_width(b) / 2.0f, t);
    float hh2 = lerpf(lv_area_get_height(a) / 2.0f, lv_area_get_height(b) / 2.0f, t);
    int32_t x1 = (int32_t)(cx - hw), y1 = (int32_t)(cy - hh2), x2 = (int32_t)(cx + hw), y2 = (int32_t)(cy + hh2);
    int32_t pw = lv_area_get_width(b), ph = lv_area_get_height(b);
    int32_t w = x2 - x1 + 1, hh = y2 - y1 + 1;
    lv_obj_set_pos(h.folder_panel, x1, y1);
    lv_obj_set_size(h.folder_panel, w, hh);
    lv_obj_set_style_radius(h.folder_panel, (int32_t)lerpf(ICON_R, 34, LV_CLAMP(0.0f, t, 1.0f)), 0);
    float tc = LV_CLAMP(0.0f, t, 1.0f);
    lv_obj_set_pos(h.folder_grid, (w - pw) / 2, (hh - ph) / 2);
    if(h.folder_snap_img) {
        /* icons ride along as one snapshot, scaled (never past 1:1, so they don't overshoot with the panel) */
        float sc = lerpf((float)ICON / (float)pw, 1.0f, tc);
        int32_t iw = (int32_t)(pw * sc), ih = (int32_t)(ph * sc);
        lv_obj_set_size(h.folder_snap_img, iw, ih);
        lv_obj_set_pos(h.folder_snap_img, (w - iw) / 2, (hh - ih) / 2);
    }
    /* The morphing panel and the real folder icon crossfade over the first third of the motion, so there is no
     * frame where one is swapped for the other */
    float xf = LV_CLAMP(0.0f, tc / 0.35f, 1.0f);
    lv_obj_set_style_opa(h.folder_panel, (lv_opa_t)(255.0f * xf), 0);
    lv_obj_set_style_opa(h.icon[h.folder_owner], (lv_opa_t)(255.0f * (1.0f - xf)), 0);
    ui_glass_set(h.folder_dim, tc, 22, LV_OPA_40);
    lv_obj_set_style_opa(h.folder_title, (lv_opa_t)(255.0f * tc * tc), 0);
}

/* Folder morph runs on a damped spring (same model as the app window and page springs): progress x chases
 * target t, so open/close can be interrupted mid-flight and keeps its velocity. */
static struct {
    lv_timer_t * timer;
    lv_obj_t * ov;
    float x, v, t, k, c;
    uint32_t last;
} fs;

static void folder_snap_free(void)
{
    if(h.folder_snap_img) lv_obj_delete(h.folder_snap_img);
    h.folder_snap_img = NULL;
    if(h.folder_snap) {
        lv_image_cache_drop(h.folder_snap);
        lv_draw_buf_destroy(h.folder_snap);
        h.folder_snap = NULL;
    }
}

/* Swap the live icon grid for a snapshot image while the folder morphs */
static void folder_snap_begin(void)
{
    folder_snap_free();
    if(!h.folder_grid || !h.folder_panel) return;
    lv_obj_set_hidden(h.folder_grid, false);
    lv_obj_set_pos(h.folder_grid, 0, 0);
    lv_obj_update_layout(h.folder_grid);
    h.folder_snap = lv_snapshot_take(h.folder_grid, LV_COLOR_FORMAT_ARGB8888);
    if(!h.folder_snap) return;
    h.folder_snap_img = lv_image_create(h.folder_panel);
    lv_image_set_src(h.folder_snap_img, h.folder_snap);
    lv_image_set_inner_align(h.folder_snap_img, LV_IMAGE_ALIGN_STRETCH);
    lv_image_set_antialias(h.folder_snap_img, true);
    lv_obj_set_hidden(h.folder_grid, true);
}

static void folder_spring_cb(lv_timer_t * tm)
{
    LV_UNUSED(tm);
    float dt = LV_MIN(lv_tick_elaps(fs.last), 50) / 1000.0f;
    fs.last = lv_tick_get();
    while(dt > 0.0f) {
        float hstep = LV_MIN(dt, 0.004f);
        float a = -fs.k * (fs.x - fs.t) - fs.c * fs.v;
        fs.v += a * hstep;
        fs.x += fs.v * hstep;
        dt -= hstep;
    }
    bool settled = fabsf(fs.x - fs.t) < 0.002f && fabsf(fs.v) < 0.02f;
    if(settled) fs.x = fs.t;
    folder_morph_exec(NULL, (int32_t)(fs.x * 1024.0f));
    if(!settled) return;
    lv_timer_pause(fs.timer);
    if(fs.t == 1.0f && h.folder_grid) {       /* settled open: back to the live, tappable icons */
        folder_snap_free();
        lv_obj_set_hidden(h.folder_grid, false);
    }
    if(fs.t == 0.0f && h.folder_closing) {
        folder_snap_free();
        lv_obj_set_style_opa(h.icon[h.folder_owner], LV_OPA_COVER, 0);
        h.folder_closing = false;
        h.folder_panel = NULL;
        lv_obj_delete_async(fs.ov);
        h.folder_grid = NULL;
    }
}

static void folder_morph(lv_obj_t * ov, bool open)
{
    float response = open ? 0.46f : 0.36f, damping = open ? 0.74f : 0.92f;
    float w = 2.0f * 3.14159265f / response;
    fs.k = w * w;
    fs.c = 2.0f * damping * w;
    if(fs.ov != ov) {
        fs.x = open ? 0.0f : 1.0f;
        fs.v = 0.0f;
    }
    fs.ov = ov;
    fs.t = open ? 1.0f : 0.0f;
    folder_snap_begin();
    fs.last = lv_tick_get();
    if(!fs.timer) fs.timer = lv_timer_create(folder_spring_cb, 8, NULL);
    lv_timer_resume(fs.timer);
}

static void folder_close(void)
{
    if(!h.folder_overlay) return;
    int owner = h.folder_owner;
    bool fly = h.folder_panel && loc_on_page(h.where[owner]) && !lv_obj_is_hidden(h.tile[owner]);
    if(fly) {
        lv_obj_get_coords(h.icon[owner], &h.folder_from);
        h.folder_closing = true;
        lv_obj_set_style_opa(h.folder_grid, LV_OPA_COVER, 0);
        folder_morph(h.folder_overlay, false);
    }
    else {
        lv_obj_set_style_opa(h.icon[owner], LV_OPA_COVER, 0);
        lv_obj_delete_async(h.folder_overlay);
    }
    h.folder_overlay = NULL;
    lv_memzero(h.fcell, sizeof(h.fcell));
    if(h.mode == T_FOLDER) h.mode = T_NONE;
}

bool home_app_installed(int id)
{
    if(id < 0 || id >= os_app_count) return false;
    for(int loc = 0; loc < HOME_LOC_COUNT; loc++) if(s_layout[loc] == id) return true;
    for(int owner = 0; owner < os_app_count; owner++)
        for(int i = 0; i < s_folder[owner].count; i++)
            if(s_folder[owner].member[i] == id) return true;
    return false;
}

bool home_app_install(int id)
{
    if(id < 0 || id >= os_app_count) return false;
    if(home_app_installed(id)) return true;
    app_add(id);
    return home_app_installed(id);
}











static void folder_open(int owner)
{
    if(owner < 0 || owner >= os_app_count || s_folder[owner].count < 2) return;
    if(h.folder_closing && h.folder_panel) {
        if(fs.timer) lv_timer_pause(fs.timer);
        lv_obj_set_style_opa(h.icon[h.folder_owner], LV_OPA_COVER, 0);
        folder_snap_free();
        lv_obj_delete(lv_obj_get_parent(h.folder_panel));
        h.folder_closing = false;
        fs.ov = NULL;
    }
    if(h.folder_overlay) {
        folder_snap_free();
        lv_obj_delete(h.folder_overlay);
        h.folder_overlay = NULL;
    }
    lv_memzero(h.fcell, sizeof(h.fcell));
    h.folder_closing = false;
    h.folder_owner = owner;
    int n = s_folder[owner].count;

    h.folder_overlay = ui_box(h.root);
    lv_obj_set_size(h.folder_overlay, h.W, h.H);
    lv_obj_move_foreground(h.folder_overlay);

    h.folder_dim = ui_box(h.folder_overlay);
    lv_obj_set_size(h.folder_dim, h.W, h.H);
    ui_glass(h.folder_dim, 0x000000, LV_OPA_50, 18);
    lv_obj_set_style_radius(h.folder_dim, 0, 0);
    lv_obj_set_style_border_width(h.folder_dim, 0, 0);

    const int32_t cols = n <= 3 ? n : (n == 4 ? 2 : 3), pad = 18;
    int rows = (n + cols - 1) / cols;
    int32_t pw = cols * TILE_W + 2 * pad, ph = rows * (TILE_H + 6) + 2 * pad - 6;
    h.folder_title = ui_label(h.folder_overlay, "Folder", &lv_font_montserrat_28, 0xFFFFFF);
    lv_obj_align(h.folder_title, LV_ALIGN_CENTER, 0, -ph / 2 - 30);

    lv_obj_t * panel = ui_box(h.folder_overlay);
    h.folder_panel = panel;
    ui_glass(panel, 0xC8D0DC, LV_OPA_40, 24);
    lv_obj_set_style_clip_corner(panel, true, 0);
    h.folder_grid = ui_box(panel);
    lv_obj_set_size(h.folder_grid, pw, ph);
    lv_obj_set_style_transform_pivot_x(h.folder_grid, pw / 2, 0);
    lv_obj_set_style_transform_pivot_y(h.folder_grid, ph / 2, 0);
    for(int i = 0; i < n; i++) {
        int id = s_folder[owner].member[i];
        lv_obj_t * cell = ui_box(h.folder_grid);
        lv_obj_set_size(cell, TILE_W, TILE_H);
        lv_obj_set_pos(cell, pad + (i % cols) * TILE_W, pad + (i / cols) * (TILE_H + 6));
        lv_obj_t * ic = mini_icon(cell, id, ICON);
        lv_obj_set_pos(ic, TILE_OX, 0);
        lv_obj_set_style_radius(ic, ICON_R, 0);
        lv_obj_set_style_transform_scale(lv_obj_get_child(ic, 0), 256 + 64, 0);
        lv_obj_t * name = ui_label(cell, os_apps[id]->name, &lv_font_montserrat_12, 0xFFFFFF);
        lv_obj_set_width(name, TILE_W);
        lv_label_set_long_mode(name, LV_LABEL_LONG_MODE_DOTS);
        lv_obj_set_style_text_align(name, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_pos(name, 0, ICON + 5);
        h.fcell[id] = ic;
    }

    lv_obj_get_coords(h.icon[owner], &h.folder_from);
    lv_area_set(&h.folder_to, (h.W - pw) / 2, (h.H - ph) / 2, (h.W - pw) / 2 + pw - 1, (h.H - ph) / 2 + ph - 1);
    h.folder_closing = false;
    folder_morph_exec(NULL, 0);
    folder_morph(h.folder_overlay, true);
}

static int folder_hit(lv_point_t p, bool * in_panel)
{
    *in_panel = false;
    if(!h.folder_panel) return -1;
    lv_area_t a;
    lv_obj_get_coords(h.folder_panel, &a);
    *in_panel = point_in(&a, p);
    for(int id = 0; id < os_app_count; id++) {
        if(!h.fcell[id]) continue;
        lv_obj_get_coords(h.fcell[id], &a);
        lv_area_increase(&a, 6, 6);
        if(point_in(&a, p)) return id;
    }
    return -1;
}

/* Pull `id` out of `owner`'s folder. A folder left with one app collapses into that app,
 * which takes the folder's slot. Returns false when there is no free slot. */
static bool folder_extract(int owner, int id, int * folder_loc_out)
{
    int floc = -1;
    for(int loc = 0; loc < HOME_LOC_COUNT; loc++) if(s_layout[loc] == owner) floc = loc;
    int free_loc = folder_free_loc();
    if(floc < 0 || free_loc < 0) return false;
    int8_t m[HOME_FOLDER_ITEMS];
    int n = 0;
    for(int i = 0; i < s_folder[owner].count; i++)
        if(s_folder[owner].member[i] != id) m[n++] = s_folder[owner].member[i];
    s_folder[owner].count = 0;
    int keeper = m[0];
    if(n >= 2) {
        memcpy(s_folder[keeper].member, m, (size_t)n);
        s_folder[keeper].count = (uint8_t)n;
    }
    s_layout[floc] = (int8_t)keeper;
    s_layout[free_loc] = (int8_t)id;
    *folder_loc_out = floc;
    return true;
}

static void folder_drag_out(int id, lv_point_t p)
{
    int owner = h.folder_owner, floc;
    if(h.folder_overlay) {
        folder_snap_free();
        lv_obj_delete(h.folder_overlay);
        h.folder_overlay = h.folder_panel = h.folder_dim = NULL;
        lv_memzero(h.fcell, sizeof(h.fcell));
    }
    h.folder_closing = false;
    lv_obj_set_style_opa(h.icon[owner], LV_OPA_COVER, 0);
    if(!folder_extract(owner, id, &floc)) {
        sdk_toast("Folder", "No free Home Screen slot.");
        h.mode = T_NONE;
        return;
    }
    int keeper = s_layout[floc];
    bool collapsed = s_folder[keeper].count < 2;
    layout_apply(s_layout, -1, false);
    for(int i = 0; i < os_app_count; i++) folder_refresh_icon(i);
    if(collapsed) {
        /* the folder plate shrinks away and the last app pops out in its place */
        fx_set(h.icon[keeper], FX_SCALE, 120);
        fx_to(h.icon[keeper], FX_SCALE, 256, 0.40f, 0.62f, 0, NULL);
    }
    lv_obj_t * t = h.tile[id];
    reparent_keep_pos(t, h.root);
    lv_obj_set_pos(t, p.x - TILE_OX - ICON / 2, p.y - ICON / 2);
    lv_obj_set_style_opa(h.icon[id], LV_OPA_COVER, 0);
    if(!h.edit) home_edit_enter();
    h.hit = id;
    drag_start(p);
    os_save();
}

static void root_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_point_t p;

    if(code == LV_EVENT_PRESSED) {
        get_point(&p);
        h.p0 = h.last = p;
        h.t0 = h.last_tick = lv_tick_get();
        h.vx = 0.0f;
        if(h.folder_overlay) {
            bool in_panel;
            h.mode = T_FOLDER;
            h.folder_hit = folder_hit(p, &in_panel);
            h.hit = in_panel ? -2 : -1;
            if(h.folder_hit >= 0) lv_obj_set_style_opa(h.fcell[h.folder_hit], LV_OPA_70, 0);
            return;
        }
        h.mode = T_PRESS;
        h.hit = hit_test(p, &h.hit_badge);
        if(!h.edit) press_visual(h.hit, true);
        return;
    }

    if(code == LV_EVENT_PRESSING) {
        get_point(&p);
        uint32_t el = lv_tick_elaps(h.last_tick);
        if(el > 0) {
            h.vx = 0.6f * h.vx + 0.4f * (float)(p.x - h.last.x) / (float)el;
            h.last = p;
            h.last_tick = lv_tick_get();
        }
        int32_t dx = p.x - h.p0.x;
        int32_t dy = p.y - h.p0.y;
        bool moved = LV_ABS(dx) > MOVE_SLOP || LV_ABS(dy) > MOVE_SLOP;
        bool horiz = LV_ABS(dx) > LV_ABS(dy);

        switch(h.mode) {
            case T_FOLDER:
                if(h.folder_hit >= 0 && (moved || lv_tick_elaps(h.t0) >= LONG_PRESS_MS))
                    folder_drag_out(h.folder_hit, p);
                break;
            case T_PRESS:
                if(!shell_home_idle()) {
                    if(moved) {
                        press_visual(h.hit, false);
                        h.mode = T_NONE;
                    }
                }
                else if(h.edit) {
                    bool mv4 = LV_ABS(dx) > 4 || LV_ABS(dy) > 4;
                    if(h.hit >= 0 && !h.hit_badge && mv4) drag_start(p);
                    else if(h.hit_widget >= 0 && !h.hit_wbadge && mv4) {
                        if(h.hit_handle) wresize_start(p);
                        else wdrag_start(p);
                    }
                    else if(moved) h.mode = (horiz && h.hit < 0 && h.hit_widget < 0) ? T_PAGE : T_NONE;
                }
                else if(moved) {
                    press_visual(h.hit, false);
                    h.mode = horiz ? T_PAGE : (dy > 0 && !h.search ? T_SEARCH : T_NONE);
                    if(h.mode == T_SEARCH) search_open();
                }
                else if(lv_tick_elaps(h.t0) >= LONG_PRESS_MS) {
                    press_visual(h.hit, false);
                    home_edit_enter();
                }
                break;
            case T_PAGE: {
                spring_stop();
                bool at_edge = (h.page == 0 && dx > 0) || (h.page == HOME_PAGES - 1 && dx < 0);
                lv_obj_set_x(h.pages, -h.page * h.W + (at_edge ? dx / 3 : dx));
                break;
            }
            case T_DRAG:
                drag_move(p, el);
                break;
            case T_WDRAG:
                wdrag_move(p);
                break;
            case T_WRESIZE:
                wresize_move(p);
                break;
            case T_SEARCH:
                if(h.search) {
                    int32_t pull = LV_CLAMP(0, dy, 160);
                    search_prog(h.search, (float)pull / (float)SEARCH_OPEN_DY);
                    lv_obj_set_style_translate_y(h.search, pull / 2 - 40, 0);
                }
                break;
            default:
                break;
        }
        return;
    }

    if(code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
        bool released = code == LV_EVENT_RELEASED;
        int32_t dx = h.last.x - h.p0.x;
        int32_t dy = h.last.y - h.p0.y;
        bool tap = released && LV_ABS(dx) <= MOVE_SLOP && LV_ABS(dy) <= MOVE_SLOP;
        touch_mode_t mode = h.mode;
        h.mode = T_IDLE;

        if(mode == T_FOLDER) {
            int id = h.folder_hit;
            if(id >= 0 && h.fcell[id]) lv_obj_set_style_opa(h.fcell[id], LV_OPA_COVER, 0);
            if(!tap) return;
            if(id >= 0 && h.fcell[id]) {
                lv_area_t a;
                lv_obj_get_coords(h.fcell[id], &a);
                shell_launch_from_home(id, &a);
            }
            else if(h.hit != -2) folder_close();
            return;
        }
        if(mode == T_PRESS) {
            if(!h.edit) press_visual(h.hit, false);
            if(!tap) return;
            if(h.edit) {
                if(h.hit >= 0 && h.hit_badge) app_remove(h.hit);
                else if(h.hit >= 0 && s_folder[h.hit].count >= 2) folder_open(h.hit);
                else if(h.hit_widget >= 0 && h.hit_wbadge) {
                    s_wid[h.hit_widget].widget = -1;
                    s_wid[h.hit_widget].size = 0;
                    widgets_build();
                    os_save();
                }
                else if(h.hit < 0 && h.hit_widget < 0) home_edit_exit();
            }
            else if(h.hit_widget >= 0 && lv_tick_elaps(h.t0) < LONG_PRESS_MS) {
                int id = sdk_app_index(os_widgets[s_wid[h.hit_widget].widget]->app);
                if(id >= 0) shell_launch_from_widget(id, h.widget[h.hit_widget]);
            }
            else if(h.hit >= 0 && lv_tick_elaps(h.t0) < LONG_PRESS_MS) {
                if(s_folder[h.hit].count >= 2) folder_open(h.hit);
                else {
                    lv_area_t a;
                    lv_obj_get_coords(h.icon[h.hit], &a);
                    shell_launch_from_home(h.hit, &a);
                }
            }
        }
        else if(mode == T_PAGE) {
            int np = h.page;
            if(dx < -h.W / 4 || h.vx < -0.3f) np++;
            else if(dx > h.W / 4 || h.vx > 0.3f) np--;
            page_go(np, true, h.vx * 1000.0f);
        }
        else if(mode == T_DRAG) {
            h.mode = T_DRAG;
            drag_drop();
        }
        else if(mode == T_WDRAG) wdrag_drop();
        else if(mode == T_WRESIZE) wresize_drop();
        else if(mode == T_SEARCH && h.search) {
            if(dy > SEARCH_OPEN_DY) {
                fx_to(h.search, FX_Y, 0, FX_BOUNCY, 0, NULL);
                search_anim(h.search, 1.0f, NULL);
                if(h.search_kb) fx_to(h.search_kb, FX_Y, 0, 0.30f, 0.86f, 0, NULL);
                lv_obj_add_state(h.search_ta, LV_STATE_FOCUSED);
            }
            else {
                search_close();
            }
        }
    }
}

static void done_event_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    home_edit_exit();
}

static void wall_event_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    char path[300];
    if(sdk_pick_image(path, sizeof(path))) import_offer(path);
    else sdk_toast("Wallpaper", "Drop an image file on the window, or use the Windows file picker.");
}

static void add_event_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    sheet_open();
}

static void sheet_event_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    sheet_close();
}

static bool widget_add(int w)
{
    int slot = -1;
    for(int k = 0; k < HOME_WIDGETS; k++) if(s_wid[k].widget < 0) { slot = k; break; }
    if(slot < 0) return false;
    int cols, rows;
    s_wid[slot].size = 0;
    s_wid[slot].widget = (int8_t)w;
    widget_cells(w, &cols, &rows);
    s_wid[slot].widget = -1;

    for(int pi = 0; pi < HOME_PAGES; pi++) {
        int page = (h.page + pi) % HOME_PAGES;
        for(int r = 0; r + rows <= HOME_ROWS; r += 2) {
            for(int c = 0; c + cols <= HOME_COLS; c += 2) {
                bool free = true;
                for(int y = r; y < r + rows && free; y++) {
                    for(int x = c; x < c + cols && free; x++) {
                        int loc = page * HOME_SLOTS + y * HOME_COLS + x;
                        if(s_layout[loc] >= 0 || loc_blocked(loc)) free = false;
                    }
                }
                if(!free) continue;
                s_wid[slot].widget = (int8_t)w;
                s_wid[slot].page = (int8_t)page;
                s_wid[slot].cell = (int8_t)(r * HOME_COLS + c);
                widgets_build();
                page_go(page, true, 0);
                fx_set(h.widget[slot], FX_SCALE, 128);
                fx_to(h.widget[slot], FX_SCALE, 256, FX_BOUNCY, 120, NULL);
                os_save();
                return true;
            }
        }
    }
    return false;
}

static widget_size_t wsize(int k)
{
    int ov = s_wid[k].size;
    return ov > 0 ? (widget_size_t)(ov - 1) : os_widgets[s_wid[k].widget]->size;
}

static void size_cells(widget_size_t sz, int * cols, int * rows)
{
    *cols = sz == WIDGET_SMALL ? 2 : 4;
    *rows = sz == WIDGET_LARGE ? 4 : 2;
}

static void wpix(int page, int c0, int r0, int cols, int rows, int32_t * x, int32_t * y, int32_t * w, int32_t * hh)
{
    *x = page * h.W + h.gap + c0 * (ICON + h.gap);
    *y = GRID_TOP + r0 * ROW_PITCH;
    *w = cols * ICON + (cols - 1) * h.gap;
    *hh = ICON + (rows - 1) * ROW_PITCH + 16;
}

static bool widget_fits(int k, int page, int c0, int r0, int cols, int rows)
{
    if(c0 < 0 || r0 < 0 || c0 + cols > HOME_COLS || r0 + rows > HOME_ROWS) return false;
    for(int o = 0; o < HOME_WIDGETS; o++) {
        if(o == k || s_wid[o].widget < 0 || s_wid[o].page != page) continue;
        int oc, orows;
        size_cells(wsize(o), &oc, &orows);
        int oc0 = s_wid[o].cell % HOME_COLS, or0 = s_wid[o].cell / HOME_COLS;
        if(c0 < oc0 + oc && c0 + cols > oc0 && r0 < or0 + orows && r0 + rows > or0) return false;
    }
    return true;
}

static void displace_icons(void)
{
    for(int loc = 0; loc < HOME_LOC_DOCK0; loc++) {
        if(s_layout[loc] < 0 || !loc_blocked(loc)) continue;
        int id = s_layout[loc];
        s_layout[loc] = -1;
        for(int l2 = 0; l2 < HOME_LOC_DOCK0; l2++) {
            if(s_layout[l2] < 0 && !loc_blocked(l2)) {
                s_layout[l2] = (int8_t)id;
                break;
            }
        }
    }
}

static void handles_layout(int k)
{
    lv_obj_t * w = h.widget[k];
    if(!w || !h.handle[k][0]) return;
    int32_t W = lv_obj_get_width(w), H = lv_obj_get_height(w);
    const int32_t D = 64, R = 29;
    lv_obj_set_pos(h.handle[k][0], W - 4 - R - D / 2, H / 2 - D / 2);
    lv_obj_set_pos(h.handle[k][1], W / 2 - D / 2, H - 4 - R - D / 2);
}

static lv_obj_t * handle_arc(lv_obj_t * parent, int32_t a0, int32_t a1)
{
    lv_obj_t * a = lv_arc_create(parent);
    lv_obj_set_floating(a, true);
    lv_obj_set_size(a, 64, 64);
    lv_arc_set_bg_angles(a, (uint16_t)a0, (uint16_t)a1);
    lv_arc_set_range(a, 0, 100);
    lv_arc_set_value(a, 0);
    lv_obj_remove_style(a, NULL, LV_PART_KNOB);
    lv_obj_set_style_arc_width(a, 5, 0);
    lv_obj_set_style_arc_color(a, lv_color_white(), 0);
    lv_obj_set_style_arc_opa(a, LV_OPA_80, 0);
    lv_obj_set_style_arc_rounded(a, true, 0);
    lv_obj_set_style_arc_opa(a, LV_OPA_TRANSP, LV_PART_INDICATOR);
    lv_obj_set_style_pad_all(a, 0, 0);
    lv_obj_set_clickable(a, false);
    return a;
}

static void handles_make(int k)
{
    h.handle[k][0] = h.handle[k][1] = NULL;
    if(!os_widgets[s_wid[k].widget]->resizable) return;
    h.handle[k][0] = handle_arc(h.widget[k], 328, 32);
    h.handle[k][1] = handle_arc(h.widget[k], 58, 122);
    handles_layout(k);
    lv_obj_set_hidden(h.handle[k][0], !h.edit);
    lv_obj_set_hidden(h.handle[k][1], !h.edit);
}

static bool handle_hit(int k, lv_point_t p, int * which)
{
    if(!h.handle[k][0]) return false;
    lv_area_t a;
    lv_obj_get_coords(h.widget[k], &a);
    int32_t cx = (a.x1 + a.x2) / 2, cy = (a.y1 + a.y2) / 2;
    if(p.x >= a.x2 - 30 && p.x <= a.x2 + 12 && p.y >= cy - 38 && p.y <= cy + 38) { *which = 1; return true; }
    if(p.y >= a.y2 - 30 && p.y <= a.y2 + 12 && p.x >= cx - 38 && p.x <= cx + 38) { *which = 2; return true; }
    return false;
}

typedef struct {
    lv_obj_t * w;
    int k;
    int32_t x0, y0, w0, h0, x1, y1, w1, h1;
} rs_t;
static rs_t s_rs;

static int32_t spring_path(const lv_anim_t * a)
{
    float t = (float)a->act_time / (float)a->duration;
    if(t >= 1.0f) return a->end_value;
    float s = 1.0f - expf(-7.0f * t) * (cosf(8.0f * t) + (7.0f / 8.0f) * sinf(8.0f * t));
    return a->start_value + (int32_t)((float)(a->end_value - a->start_value) * s);
}

static void rs_exec(void * var, int32_t v)
{
    LV_UNUSED(var);
    if(!s_rs.w) return;
    float t = v / 1000.0f;
    lv_obj_set_pos(s_rs.w, s_rs.x0 + (int32_t)((s_rs.x1 - s_rs.x0) * t), s_rs.y0 + (int32_t)((s_rs.y1 - s_rs.y0) * t));
    lv_obj_set_size(s_rs.w, LV_MAX(24, s_rs.w0 + (int32_t)((s_rs.w1 - s_rs.w0) * t)),
                    LV_MAX(24, s_rs.h0 + (int32_t)((s_rs.h1 - s_rs.h0) * t)));
    handles_layout(s_rs.k);
}

static void frame_to(int k, int32_t x, int32_t y, int32_t w, int32_t hh)
{
    lv_obj_t * o = h.widget[k];
    s_rs.w = o;
    s_rs.k = k;
    s_rs.x0 = lv_obj_get_x(o); s_rs.y0 = lv_obj_get_y(o);
    s_rs.w0 = lv_obj_get_width(o); s_rs.h0 = lv_obj_get_height(o);
    s_rs.x1 = x; s_rs.y1 = y; s_rs.w1 = w; s_rs.h1 = hh;
    lv_anim_delete(&s_rs, rs_exec);
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, &s_rs);
    lv_anim_set_exec_cb(&a, rs_exec);
    lv_anim_set_values(&a, 0, 1000);
    lv_anim_set_duration(&a, 560);
    lv_anim_set_path_cb(&a, spring_path);
    lv_anim_start(&a);
}

static lv_obj_t * make_inner(lv_obj_t * face, int k, widget_size_t sz, int32_t fw, int32_t fh)
{
    lv_obj_t * inner = ui_box(face);
    lv_obj_set_size(inner, fw, fh);
    lv_obj_set_pos(inner, 0, 0);
    lv_obj_set_style_pad_all(inner, 12, 0);
    sdk_widget_size_set(sz);
    sdk_widget_inst_set(k);
    os_widgets[s_wid[k].widget]->create(inner);
    return inner;
}

static void inner_gone_cb(lv_obj_t * o)
{
    lv_obj_delete_async(o);
}

static void swap_inner(int k, widget_size_t sz, int32_t fw, int32_t fh)
{
    lv_obj_t * face = lv_obj_get_child(h.widget[k], 0);
    lv_obj_t * old = h.inner[k];
    lv_obj_t * nw = make_inner(face, k, sz, fw, fh);
    fx_set(nw, FX_OPA, 0);
    fx_to(nw, FX_OPA, 255, 0.30f, 1.0f, 120, NULL);
    if(old) fx_to(old, FX_OPA, 0, 0.18f, 1.0f, 0, inner_gone_cb);
    h.inner[k] = nw;
}

static void widget_place(int k, int page, int c0, int r0, widget_size_t sz)
{
    int cols, rows;
    size_cells(sz, &cols, &rows);
    int32_t x, y, w, hh;
    wpix(page, c0, r0, cols, rows, &x, &y, &w, &hh);
    bool resized = wsize(k) != sz;
    s_wid[k].page = (int8_t)page;
    s_wid[k].cell = (int8_t)(r0 * HOME_COLS + c0);
    s_wid[k].size = (int8_t)(sz + 1);
    if(resized || !h.inner[k]) swap_inner(k, sz, w, hh);
    frame_to(k, x, y, w, hh);
    displace_icons();
    layout_apply(s_layout, -1, true);
    os_save();
}

static void ghost_hide(void)
{
    if(h.ghost) lv_obj_set_hidden(h.ghost, true);
}

static void ghost_show(int page, int c0, int r0, int cols, int rows, bool ok)
{
    if(!h.ghost) {
        h.ghost = ui_box(h.pages);
        lv_obj_set_style_radius(h.ghost, 22, 0);
        lv_obj_set_style_border_width(h.ghost, 2, 0);
        lv_obj_set_style_bg_opa(h.ghost, LV_OPA_10, 0);
        lv_obj_set_style_bg_color(h.ghost, lv_color_white(), 0);
    }
    int32_t x, y, w, hh;
    wpix(page, c0, r0, cols, rows, &x, &y, &w, &hh);
    lv_obj_set_pos(h.ghost, x, y);
    lv_obj_set_size(h.ghost, w, hh);
    lv_obj_set_style_border_color(h.ghost, lv_color_hex(ok ? 0xFFFFFF : 0xFF453A), 0);
    lv_obj_set_style_border_opa(h.ghost, LV_OPA_60, 0);
    lv_obj_set_hidden(h.ghost, false);
    lv_obj_move_background(h.ghost);
    lv_obj_move_to_index(h.ghost, 0);
}

static void snap_cell(int cols, int rows, int32_t xl, int32_t yl, int32_t wcur, int * page, int * c0, int * r0)
{
    int pg = (int)((xl + wcur / 2) / h.W);
    pg = LV_CLAMP(0, pg, HOME_PAGES - 1);
    float cf = (float)(xl - pg * h.W - h.gap) / (float)(ICON + h.gap);
    float rf = (float)(yl - GRID_TOP) / (float)ROW_PITCH;
    int c = 2 * (int)lroundf(cf / 2.0f);
    int r = 2 * (int)lroundf(rf / 2.0f);
    *page = pg;
    *c0 = LV_CLAMP(0, c, HOME_COLS - cols);
    *r0 = LV_CLAMP(0, r, HOME_ROWS - rows);
}

static void wdrag_start(lv_point_t p)
{
    int k = h.hit_widget;
    h.wk = k;
    h.mode = T_WDRAG;
    lv_obj_t * w = h.widget[k];
    lv_anim_delete(&s_rs, rs_exec);
    h.wgrab.x = p.x - (lv_obj_get_x(w) + lv_obj_get_x(h.pages));
    h.wgrab.y = p.y - lv_obj_get_y(w);
    h.wedge_ms = 0;
    h.wedge_t = lv_tick_get();
    lv_obj_move_foreground(w);
#if OS_FX_HEAVY
    fx_to(w, FX_SCALE, 268, 0.30f, 0.60f, 0, NULL);
#endif
    lv_obj_set_style_shadow_width(lv_obj_get_child(w, 0), 30, 0);
    lv_obj_set_style_shadow_opa(lv_obj_get_child(w, 0), LV_OPA_40, 0);
}

static void wdrag_move(lv_point_t p)
{
    int k = h.wk;
    lv_obj_t * w = h.widget[k];
    int32_t x = p.x - h.wgrab.x - lv_obj_get_x(h.pages), y = p.y - h.wgrab.y;
    lv_obj_set_pos(w, x, y);
    uint32_t dt = lv_tick_elaps(h.wedge_t);
    h.wedge_t = lv_tick_get();
    if((p.x < 18 && h.page > 0) || (p.x > h.W - 18 && h.page < HOME_PAGES - 1)) {
        h.wedge_ms += dt;
        if(h.wedge_ms > 550) {
            h.wedge_ms = 0;
            page_go(h.page + (p.x < 18 ? -1 : 1), true, 0);
        }
    }
    else h.wedge_ms = 0;
    int cols, rows;
    size_cells(wsize(k), &cols, &rows);
    int page, c0, r0;
    snap_cell(cols, rows, x, y, lv_obj_get_width(w), &page, &c0, &r0);
    ghost_show(page, c0, r0, cols, rows, widget_fits(k, page, c0, r0, cols, rows));
}

static void wdrag_drop(void)
{
    int k = h.wk;
    lv_obj_t * w = h.widget[k];
    int cols, rows;
    size_cells(wsize(k), &cols, &rows);
    int page, c0, r0;
    snap_cell(cols, rows, lv_obj_get_x(w), lv_obj_get_y(w), lv_obj_get_width(w), &page, &c0, &r0);
    ghost_hide();
#if OS_FX_HEAVY
    fx_to(w, FX_SCALE, 256, 0.40f, 0.55f, 0, NULL);
#endif
    lv_obj_set_style_shadow_width(lv_obj_get_child(w, 0), 0, 0);
    if(!widget_fits(k, page, c0, r0, cols, rows)) {
        page = s_wid[k].page;
        c0 = s_wid[k].cell % HOME_COLS;
        r0 = s_wid[k].cell / HOME_COLS;
    }
    widget_place(k, page, c0, r0, wsize(k));
    page_go(page, true, 0);
}

static void sz_candidates(float fx, float fy, widget_size_t * best)
{
    static const float pt[3][2] = { { 0, 0 }, { 1, 0 }, { 1, 1 } };
    float bd = 1e9f;
    for(int i = 0; i < 3; i++) {
        float d = (fx - pt[i][0]) * (fx - pt[i][0]) + (fy - pt[i][1]) * (fy - pt[i][1]);
        if(d < bd) {
            bd = d;
            *best = (widget_size_t)i;
        }
    }
}

static void resize_bounds(int32_t * w2, int32_t * w4, int32_t * h2, int32_t * h4)
{
    int32_t x, y;
    wpix(0, 0, 0, 2, 2, &x, &y, w2, h2);
    wpix(0, 0, 0, 4, 4, &x, &y, w4, h4);
}

static widget_size_t resize_target(int k, int * c0, int * r0)
{
    lv_obj_t * w = h.widget[k];
    int32_t w2, w4, h2, h4;
    resize_bounds(&w2, &w4, &h2, &h4);
    float fx = LV_CLAMP(0.0f, (float)(lv_obj_get_width(w) - w2) / (float)(w4 - w2), 1.0f);
    float fy = LV_CLAMP(0.0f, (float)(lv_obj_get_height(w) - h2) / (float)(h4 - h2), 1.0f);
    widget_size_t sz;
    sz_candidates(fx, fy, &sz);
    int cols, rows;
    size_cells(sz, &cols, &rows);
    *c0 = LV_MIN(s_wid[k].cell % HOME_COLS, HOME_COLS - cols);
    *r0 = LV_MIN(s_wid[k].cell / HOME_COLS, HOME_ROWS - rows);
    *c0 -= *c0 % 2;
    *r0 -= *r0 % 2;
    return sz;
}

static void wresize_start(lv_point_t p)
{
    int k = h.hit_widget;
    h.wk = k;
    h.mode = T_WRESIZE;
    h.wres_axis = h.hit_handle;
    lv_obj_t * w = h.widget[k];
    lv_anim_delete(&s_rs, rs_exec);
    h.wres_w0 = lv_obj_get_width(w);
    h.wres_h0 = lv_obj_get_height(w);
    h.p0 = p;
    lv_obj_move_foreground(w);
}

static void wresize_move(lv_point_t p)
{
    int k = h.wk;
    lv_obj_t * w = h.widget[k];
    int32_t w2, w4, h2, h4;
    resize_bounds(&w2, &w4, &h2, &h4);
    int32_t nw = h.wres_w0 + (h.wres_axis == 1 ? p.x - h.p0.x : 0);
    int32_t nh = h.wres_h0 + (h.wres_axis == 2 ? p.y - h.p0.y : 0);
    lv_obj_set_size(w, LV_CLAMP(w2, nw, w4), LV_CLAMP(h2, nh, h4));
    handles_layout(k);
    int c0, r0;
    widget_size_t sz = resize_target(k, &c0, &r0);
    int cols, rows;
    size_cells(sz, &cols, &rows);
    ghost_show(s_wid[k].page, c0, r0, cols, rows, widget_fits(k, s_wid[k].page, c0, r0, cols, rows));
}

static void wresize_drop(void)
{
    int k = h.wk;
    int c0, r0;
    widget_size_t sz = resize_target(k, &c0, &r0);
    int cols, rows;
    size_cells(sz, &cols, &rows);
    ghost_hide();
    if(!widget_fits(k, s_wid[k].page, c0, r0, cols, rows)) {
        sz = wsize(k);
        c0 = s_wid[k].cell % HOME_COLS;
        r0 = s_wid[k].cell / HOME_COLS;
    }
    widget_place(k, s_wid[k].page, c0, r0, sz);
}

void home_debug_resize(int k, int size)
{
    int cols, rows;
    size_cells((widget_size_t)size, &cols, &rows);
    int c0 = LV_MIN(s_wid[k].cell % HOME_COLS, HOME_COLS - cols);
    int r0 = LV_MIN(s_wid[k].cell / HOME_COLS, HOME_ROWS - rows);
    widget_place(k, s_wid[k].page, c0 - c0 % 2, r0 - r0 % 2, (widget_size_t)size);
}

void home_debug_wres(int k, int axis, int dx, int dy)
{
    h.hit_widget = k;
    h.hit_handle = axis;
    lv_point_t p = { 0, 0 };
    wresize_start(p);
    lv_point_t q = { dx, dy };
    wresize_move(q);
}

void home_debug_wdrag(int k, int x, int y)
{
    h.hit_widget = k;
    lv_area_t a;
    lv_obj_get_coords(h.widget[k], &a);
    lv_point_t p = { (a.x1 + a.x2) / 2, (a.y1 + a.y2) / 2 };
    wdrag_start(p);
    lv_point_t q = { x, y };
    wdrag_move(q);
}

void home_debug_wdrop(void)
{
    if(h.mode == T_WRESIZE) wresize_drop();
    else if(h.mode == T_WDRAG) wdrag_drop();
    h.mode = T_IDLE;
}

int home_widget_count(int widget)
{
    int n = 0;
    for(int k = 0; k < HOME_WIDGETS; k++) if(s_wid[k].widget == widget) n++;
    return n;
}

int home_widget_free_slots(void)
{
    int n = 0;
    for(int k = 0; k < HOME_WIDGETS; k++) if(s_wid[k].widget < 0) n++;
    return n;
}

bool home_widget_add(int widget)
{
    if(!h.root || widget < 0 || widget >= os_widget_count) return false;
    return widget_add(widget);
}

bool home_widget_remove(int widget)
{
    for(int k = HOME_WIDGETS - 1; k >= 0; k--) {
        if(s_wid[k].widget != widget) continue;
        s_wid[k].widget = -1;
        s_wid[k].size = 0;
        if(h.root) widgets_build();
        os_save();
        return true;
    }
    return false;
}

static void sheet_row_event_cb(lv_event_t * e)
{
    int id = (int)(intptr_t)lv_event_get_user_data(e);
    sheet_close();
    if(id >= 1000) {
        if(!widget_add(id - 1000)) sdk_toast("No Space", "Make room by moving or removing apps first.");
    }
    else {
        app_add(id);
    }
}

static void widgets_build(void)
{
    for(int k = 0; k < HOME_WIDGETS; k++) {
        if(h.widget[k]) lv_obj_delete(h.widget[k]);
        h.widget[k] = h.wbadge[k] = h.inner[k] = NULL;
        h.handle[k][0] = h.handle[k][1] = NULL;
        if(s_wid[k].widget < 0 || s_wid[k].widget >= os_widget_count) continue;

        const widget_desc_t * wd = os_widgets[s_wid[k].widget];
        int cols, rows;
        widget_size_t wsz = s_wid[k].size > 0 ? (widget_size_t)(s_wid[k].size - 1) : wd->size;
        widget_cells_sz(wsz, &cols, &rows);
        int c0 = s_wid[k].cell % HOME_COLS, r0 = s_wid[k].cell / HOME_COLS;

        lv_obj_t * w = ui_box(h.pages);
        lv_obj_set_overflow_visible(w, true);
        lv_obj_set_pos(w, s_wid[k].page * h.W + h.gap + c0 * (ICON + h.gap), GRID_TOP + r0 * ROW_PITCH);
        lv_obj_set_size(w, cols * ICON + (cols - 1) * h.gap, ICON + (rows - 1) * ROW_PITCH + 16);

        lv_obj_t * face = ui_box(w);
        lv_obj_set_size(face, LV_PCT(100), LV_PCT(100));
        lv_obj_set_style_radius(face, 22, 0);
        lv_obj_set_style_clip_corner(face, true, 0);
        ui_glass(face, 0x1C1C1E, LV_OPA_50, 20);
        int32_t fw = cols * ICON + (cols - 1) * h.gap, fh = ICON + (rows - 1) * ROW_PITCH + 16;
        h.inner[k] = make_inner(face, k, wsz, fw, fh);

        lv_obj_t * b = ui_box(w);
        lv_obj_set_size(b, 22, 22);
        lv_obj_set_pos(b, -8, -8);
        lv_obj_set_style_radius(b, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_opa(b, LV_OPA_COVER, 0);
        lv_obj_set_style_bg_color(b, lv_color_hex(0x5A5A5E), 0);
        lv_obj_center(ui_label(b, LV_SYMBOL_MINUS, &lv_font_montserrat_12, 0xFFFFFF));
        lv_obj_set_hidden(b, !h.edit);
        h.widget[k] = w;
        h.wbadge[k] = b;
        handles_make(k);
    }

    for(int loc = 0; loc < HOME_LOC_DOCK0; loc++) {
        if(s_layout[loc] < 0 || !loc_blocked(loc)) continue;
        int id = s_layout[loc];
        s_layout[loc] = -1;
        for(int l2 = 0; l2 < HOME_LOC_DOCK0; l2++) {
            if(s_layout[l2] < 0 && !loc_blocked(l2)) {
                s_layout[l2] = (int8_t)id;
                break;
            }
        }
    }
    layout_apply(s_layout, -1, true);
}

static void search_row_cb(lv_event_t * e)
{
    int id = (int)(intptr_t)lv_event_get_user_data(e);
    lv_area_t a;
    lv_obj_get_coords(lv_obj_get_child(lv_event_get_current_target_obj(e), 0), &a);
    search_close();
    shell_open_app_from(id, &a);
}

static void search_filter(void)
{
    const char * q = lv_textarea_get_text(h.search_ta);
    lv_obj_clean(h.search_list);
    int n = 0;
    for(int id = 0; id < os_app_count; id++) {
        const app_desc_t * app = os_apps[id];
        bool match = true;
        if(q[0]) {
            match = false;
            size_t ql = strlen(q);
            for(const char * c = app->name; *c && !match; c++) {
                size_t i = 0;
                while(i < ql && c[i] && ((c[i] | 32) == (q[i] | 32))) i++;
                match = i == ql;
            }
        }
        if(!match) continue;
        lv_obj_t * row = ui_box(h.search_list);
        lv_obj_set_size(row, LV_PCT(100), 50);
        lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_column(row, 12, 0);
        lv_obj_set_style_pad_hor(row, 8, 0);
        lv_obj_set_style_radius(row, 12, 0);
        lv_obj_set_style_bg_color(row, lv_color_white(), LV_STATE_PRESSED);
        lv_obj_set_style_bg_opa(row, LV_OPA_20, LV_STATE_PRESSED);
        lv_obj_set_clickable(row, true);
        lv_obj_add_event_cb(row, search_row_cb, LV_EVENT_CLICKED, (void *)(intptr_t)id);
        lv_obj_t * ic = ui_box(row);
        lv_obj_set_size(ic, 38, 38);
        lv_obj_set_style_radius(ic, 10, 0);
        ui_icon_style(ic, app->color);
        lv_obj_center(ui_label(ic, app->symbol, &lv_font_montserrat_18, 0xFFFFFF));
        ui_label(row, app->name, &lv_font_montserrat_16, 0xFFFFFF);
        if(++n == 6) break;
    }
    if(n == 0) ui_label(h.search_list, "No Results", &lv_font_montserrat_16, UI_DIM);
}

static void search_ta_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    search_filter();
}

static void search_bg_cb(lv_event_t * e)
{
    if(lv_event_get_target_obj(e) == h.search) search_close();
}

/* Spotlight: one progress value drives the backdrop blur radius, tint and content opacity together */
static void search_prog(lv_obj_t * o, float p)
{
    p = LV_CLAMP(0.0f, p, 1.0f);
    lv_obj_set_style_opa(o, (lv_opa_t)(255.0f * p), 0);
    ui_glass_set(o, p, 30, LV_OPA_50);
}

static void search_prog_exec(void * o, int32_t v)
{
    search_prog((lv_obj_t *)o, v / 1000.0f);
}

static void search_anim(lv_obj_t * o, float to, lv_anim_completed_cb_t done)
{
    lv_anim_delete(o, search_prog_exec);
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, o);
    lv_anim_set_exec_cb(&a, search_prog_exec);
    lv_anim_set_values(&a, (int32_t)(lv_obj_get_style_opa(o, 0) * 1000 / 255), (int32_t)(to * 1000.0f));
    lv_anim_set_duration(&a, to > 0.5f ? 260 : 220);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    lv_anim_set_completed_cb(&a, done);
    lv_anim_start(&a);
}

static void search_open(void)
{
    if(h.search) return;
    h.search = ui_box(h.root);
    lv_obj_set_size(h.search, h.W, h.H);
    ui_glass(h.search, 0x000000, LV_OPA_50, 30);
    lv_obj_set_clickable(h.search, true);
    lv_obj_add_event_cb(h.search, search_bg_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_set_style_opa(h.search, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_top(h.search, 40, 0);
    lv_obj_set_style_pad_hor(h.search, 14, 0);
    lv_obj_set_style_pad_row(h.search, 10, 0);
    lv_obj_set_flex_flow(h.search, LV_FLEX_FLOW_COLUMN);

    h.search_ta = lv_textarea_create(h.search);
    lv_obj_set_width(h.search_ta, LV_PCT(100));
    lv_textarea_set_one_line(h.search_ta, true);
    lv_textarea_set_placeholder_text(h.search_ta, LV_SYMBOL_EYE_OPEN "  Search");
    lv_obj_set_style_radius(h.search_ta, 14, 0);
    lv_obj_set_style_bg_color(h.search_ta, lv_color_hex(0x3A3A3C), 0);
    lv_obj_set_style_bg_opa(h.search_ta, LV_OPA_80, 0);
    lv_obj_set_style_border_width(h.search_ta, 0, 0);
    lv_obj_add_event_cb(h.search_ta, search_ta_cb, LV_EVENT_VALUE_CHANGED, NULL);

    ui_label(h.search, "Siri Suggestions", &lv_font_montserrat_12, 0xC7C7CC);
    h.search_list = ui_box(h.search);
    lv_obj_set_size(h.search_list, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(h.search_list, LV_FLEX_FLOW_COLUMN);

    lv_obj_t * kb = ui_keyboard(h.search, h.search_ta);
    lv_obj_set_size(kb, h.W, 180);
    lv_obj_align(kb, LV_ALIGN_BOTTOM_MID, 0, 0);
    ui_keyboard_show(kb);
    h.search_kb = kb;
    fx_set(kb, FX_Y, 190);                  /* rises when the pull is released */
    search_prog(h.search, 0.0f);
    search_filter();
}

static void search_gone_anim_cb(lv_anim_t * a)
{
    lv_obj_delete_async((lv_obj_t *)a->var);
}

static void search_close(void)
{
    if(!h.search) return;
    lv_obj_set_clickable(h.search, false);
    fx_to(h.search, FX_Y, -40, FX_SNAPPY, 0, NULL);
    if(h.search_kb) fx_to(h.search_kb, FX_Y, 190, 0.24f, 1.0f, 0, NULL);
    search_anim(h.search, 0.0f, search_gone_anim_cb);
    h.search = NULL;
    h.search_kb = NULL;
}

/* Simulator hook: "fold=a:b" puts b into a's folder, "fopen=a", "ftap=id", "fdrag=id:x:y", "drop" */
void home_debug_cmd(const char * c)
{
    int a, b, x, y;
    if(sscanf(c, "fold=%d:%d", &a, &b) == 2) {
        for(int loc = 0; loc < HOME_LOC_COUNT; loc++) if(s_layout[loc] == b) s_layout[loc] = -1;
        folder_add(a, b);
        layout_apply(s_layout, -1, false);
    }
    else if(sscanf(c, "fopen=%d", &a) == 1) folder_open(a);
    else if(strcmp(c, "fclose") == 0) folder_close();
    else if(sscanf(c, "ftap=%d", &a) == 1 && h.fcell[a]) {
        lv_area_t r;
        lv_obj_get_coords(h.fcell[a], &r);
        shell_launch_from_home(a, &r);
    }
    else if(sscanf(c, "fdrag=%d:%d:%d", &a, &x, &y) == 3) {
        lv_point_t p = { x, y };
        folder_drag_out(a, p);
        drag_move(p, 16);
    }
    else if(strcmp(c, "drop") == 0 && h.mode == T_DRAG) drag_drop();
}

/* Serial/BOOT launches use the actual icon and original morph path. */
bool home_prepare_app(int id)
{
 if(id < 0 || id >= os_app_count) return false;
 if(h.folder_overlay && h.fcell[id]) return true;
 int loc = h.where[id];
 if(loc < 0) {
  for(int owner = 0; owner < os_app_count; owner++) {
   if(s_folder[owner].count < 2) continue;
   for(int i = 0; i < s_folder[owner].count; i++)
    if(s_folder[owner].member[i] == id) loc = h.where[owner];
  }
 }
 if(loc < 0) return false;
 if(loc < HOME_LOC_DOCK0) page_go(loc / HOME_SLOTS, false, 0);
 lv_obj_update_layout(h.root);
 return true;
}
