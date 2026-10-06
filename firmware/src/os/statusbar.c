/**
 * @file statusbar.c
 * iOS 18 style status icons drawn from primitives (no icon font): cellular bars with rounded tops, the
 * Wi-Fi fan (dot + two arcs), and the pill battery with its nub. Sizes follow the iPhone status bar
 * (battery ~25x12 with a 4 px corner, bars 3 px wide, 90 degree Wi-Fi arcs).
 */
#include "sdk.h"

#define BAR_N   4

static struct {
    lv_obj_t * row;
    lv_obj_t * air;
    lv_obj_t * bt;
    lv_obj_t * sig;
    lv_obj_t * bars[BAR_N];
    lv_obj_t * wifi;
    lv_obj_t * fill;
} s;

typedef enum { T_BG, T_ARC, T_BORDER, T_TEXT } tint_kind_t;
static struct { lv_obj_t * o; uint8_t kind; } s_parts[24];
static int s_part_n;

static void part(lv_obj_t * o, tint_kind_t k)
{
    if(s_part_n < 24) {
        s_parts[s_part_n].o = o;
        s_parts[s_part_n].kind = (uint8_t)k;
        s_part_n++;
    }
}

/* Re-colour every icon (used when the app behind the bar is bright: white icons become dark) */
void statusbar_icons_set_color(lv_color_t c)
{
    for(int i = 0; i < s_part_n; i++) {
        lv_obj_t * o = s_parts[i].o;
        switch(s_parts[i].kind) {
            case T_BG: lv_obj_set_style_bg_color(o, c, 0); break;
            case T_ARC: lv_obj_set_style_arc_color(o, c, 0); break;
            case T_BORDER: lv_obj_set_style_border_color(o, c, 0); break;
            default: lv_obj_set_style_text_color(o, c, 0); break;
        }
    }
}

static lv_obj_t * solid(lv_obj_t * parent, int32_t w, int32_t h, int32_t radius, lv_opa_t opa)
{
    lv_obj_t * o = ui_box(parent);
    lv_obj_set_size(o, w, h);
    lv_obj_set_style_radius(o, radius, 0);
    lv_obj_set_style_bg_opa(o, opa, 0);
    lv_obj_set_style_bg_color(o, lv_color_white(), 0);
    part(o, T_BG);
    return o;
}

static void wifi_arc(lv_obj_t * parent, int32_t d, int32_t cx, int32_t cy)
{
    lv_obj_t * a = lv_arc_create(parent);
    lv_obj_set_size(a, d, d);
    lv_obj_set_pos(a, cx - d / 2, cy - d / 2);
    lv_arc_set_bg_angles(a, 225, 315);
    lv_arc_set_range(a, 0, 100);
    lv_arc_set_value(a, 0);
    lv_obj_remove_style(a, NULL, LV_PART_KNOB);
    lv_obj_set_style_arc_width(a, 2, 0);
    lv_obj_set_style_arc_color(a, lv_color_white(), 0);
    part(a, T_ARC);
    lv_obj_set_style_arc_rounded(a, true, 0);
    lv_obj_set_style_arc_opa(a, LV_OPA_TRANSP, LV_PART_INDICATOR);
    lv_obj_set_style_pad_all(a, 0, 0);
    lv_obj_set_clickable(a, false);
}

lv_obj_t * statusbar_icons_create(lv_obj_t * parent)
{
    s.row = ui_box(parent);
    lv_obj_set_size(s.row, LV_SIZE_CONTENT, 16);
    lv_obj_set_overflow_visible(s.row, true);
    lv_obj_set_flex_flow(s.row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(s.row, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(s.row, 6, 0);
    lv_obj_align(s.row, LV_ALIGN_TOP_RIGHT, -18, 11);

    s.air = ui_label(s.row, "Airplane", &lv_font_montserrat_12, 0xFFFFFF);
    s.bt = ui_label(s.row, LV_SYMBOL_BLUETOOTH, &lv_font_montserrat_12, 0xFFFFFF);
    part(s.air, T_TEXT);
    part(s.bt, T_TEXT);

    /* Cellular: four bars, 3 px wide, rounded */
    s.sig = ui_box(s.row);
    lv_obj_set_size(s.sig, 17, 11);
    static const int32_t bh[BAR_N] = { 4, 6, 8, 11 };
    for(int i = 0; i < BAR_N; i++) {
        s.bars[i] = solid(s.sig, 3, bh[i], 1, LV_OPA_COVER);
        lv_obj_set_pos(s.bars[i], (i * 46) / 10, 11 - bh[i]);
    }

    /* Wi-Fi: dot plus two 90 degree arcs, all centred on the same point */
    s.wifi = ui_box(s.row);
    lv_obj_set_size(s.wifi, 17, 13);
    lv_obj_set_overflow_visible(s.wifi, true);
    wifi_arc(s.wifi, 27, 8, 13);
    wifi_arc(s.wifi, 17, 8, 13);
    lv_obj_t * dot = solid(s.wifi, 4, 4, 2, LV_OPA_COVER);
    lv_obj_set_pos(dot, 6, 9);

    /* Battery: outline pill, inset fill, nub */
    lv_obj_t * bat = ui_box(s.row);
    lv_obj_set_size(bat, 27, 12);
    lv_obj_t * body = solid(bat, 24, 12, 4, LV_OPA_TRANSP);
    lv_obj_set_style_border_width(body, 1, 0);
    lv_obj_set_style_border_color(body, lv_color_white(), 0);
    part(body, T_BORDER);
    lv_obj_set_style_border_opa(body, LV_OPA_40, 0);
    s.fill = solid(bat, 20, 8, 2, LV_OPA_COVER);
    lv_obj_set_pos(s.fill, 2, 2);
    lv_obj_t * nub = solid(bat, 2, 4, 1, LV_OPA_40);
    lv_obj_set_pos(nub, 25, 4);

    statusbar_icons_update();
    return s.row;
}

void statusbar_icons_update(void)
{
    if(!s.row) return;
    bool air = os_settings.airplane;
    lv_obj_set_hidden(s.air, !air);
    lv_obj_set_hidden(s.bt, air || !os_settings.bluetooth);
    lv_obj_set_hidden(s.wifi, air || !os_settings.wifi);
    bool cell = os_settings.cellular && !air;
    for(int i = 0; i < BAR_N; i++) lv_obj_set_style_bg_opa(s.bars[i], cell ? LV_OPA_COVER : LV_OPA_30, 0);
    lv_obj_set_width(s.fill, 20);
}
