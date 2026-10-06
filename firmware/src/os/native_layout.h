/* ESP32 native raster layout adapter.
 * Existing shell geometry and spring velocities use design units. Convert
 * those values before LVGL stores styles or draws: every backing surface,
 * glyph and display buffer is rasterized at the physical resolution.
 * The renderer and pixel-processing code never include these aliases.
 */
#ifndef S3_NATIVE_LAYOUT_H
#define S3_NATIVE_LAYOUT_H
#include "lvgl/lvgl.h"
#ifdef S3_NATIVE_LAYOUT
const lv_font_t * s3_native_font(const lv_font_t * font);
static inline int32_t s3_native_px(int32_t v) {
    if(!LV_COORD_IS_PX(v) || v == LV_RADIUS_CIRCLE) return v;
    return v < 0 ? -((-v + 1)/3) : (v + 1)/3;
}
static inline int32_t s3_native_stroke(int32_t v) { return v > 0 ? LV_MAX(1,s3_native_px(v)) : 0; }
#ifndef S3_NATIVE_DRIVER
/* Only native glyph bitmaps are referenced by the hardware UI. */
extern const lv_font_t s3_font_4;
extern const lv_font_t s3_font_5;
extern const lv_font_t s3_font_6;
extern const lv_font_t s3_font_7;
extern const lv_font_t s3_font_8;
extern const lv_font_t s3_font_9;
extern const lv_font_t s3_font_10;
extern const lv_font_t s3_font_11;
extern const lv_font_t s3_font_12;
extern const lv_font_t s3_font_13;
extern const lv_font_t s3_font_14;
extern const lv_font_t s3_font_15;
extern const lv_font_t s3_font_16;
extern const lv_font_t s3_font_clock_32;
#define lv_font_montserrat_10 s3_font_4
#define lv_font_montserrat_12 s3_font_4
#define lv_font_montserrat_14 s3_font_5
#define lv_font_montserrat_16 s3_font_5
#define lv_font_montserrat_18 s3_font_6
#define lv_font_montserrat_20 s3_font_7
#define lv_font_montserrat_22 s3_font_7
#define lv_font_montserrat_24 s3_font_8
#define lv_font_montserrat_26 s3_font_9
#define lv_font_montserrat_28 s3_font_9
#define lv_font_montserrat_30 s3_font_10
#define lv_font_montserrat_32 s3_font_11
#define lv_font_montserrat_34 s3_font_11
#define lv_font_montserrat_36 s3_font_12
#define lv_font_montserrat_38 s3_font_13
#define lv_font_montserrat_40 s3_font_13
#define lv_font_montserrat_42 s3_font_14
#define lv_font_montserrat_44 s3_font_15
#define lv_font_montserrat_46 s3_font_15
#define lv_font_montserrat_48 s3_font_16
#define font_clock_96 s3_font_clock_32
static inline void s3n_lv_obj_set_x(lv_obj_t * o, int32_t v) { lv_obj_set_x(o,s3_native_px(v)); }
static inline void s3n_lv_obj_set_y(lv_obj_t * o, int32_t v) { lv_obj_set_y(o,s3_native_px(v)); }
static inline void s3n_lv_obj_set_width(lv_obj_t * o, int32_t v) { lv_obj_set_width(o,s3_native_px(v)); }
static inline void s3n_lv_obj_set_height(lv_obj_t * o, int32_t v) { lv_obj_set_height(o,s3_native_px(v)); }
static inline void s3n_lv_obj_set_ext_click_area(lv_obj_t * o, int32_t v) { lv_obj_set_ext_click_area(o,s3_native_px(v)); }
static inline void s3n_lv_obj_set_pos(lv_obj_t * o, int32_t x, int32_t y) { lv_obj_set_pos(o,s3_native_px(x),s3_native_px(y)); }
static inline void s3n_lv_obj_set_size(lv_obj_t * o, int32_t x, int32_t y) { lv_obj_set_size(o,s3_native_px(x),s3_native_px(y)); }
static inline void s3n_lv_tabview_set_tab_bar_size(lv_obj_t * o, int32_t v) { lv_tabview_set_tab_bar_size(o,s3_native_px(v)); }
static inline int32_t s3n_lv_obj_get_x(const lv_obj_t * o) { return lv_obj_get_x(o)*3; }
static inline int32_t s3n_lv_obj_get_y(const lv_obj_t * o) { return lv_obj_get_y(o)*3; }
static inline int32_t s3n_lv_obj_get_width(const lv_obj_t * o) { return lv_obj_get_width(o)*3; }
static inline int32_t s3n_lv_obj_get_height(const lv_obj_t * o) { return lv_obj_get_height(o)*3; }
static inline int32_t s3n_lv_obj_get_content_width(const lv_obj_t * o) { return lv_obj_get_content_width(o)*3; }
static inline int32_t s3n_lv_obj_get_content_height(const lv_obj_t * o) { return lv_obj_get_content_height(o)*3; }
static inline int32_t s3n_lv_obj_get_scroll_x(const lv_obj_t * o) { return lv_obj_get_scroll_x(o)*3; }
static inline int32_t s3n_lv_obj_get_scroll_y(const lv_obj_t * o) { return lv_obj_get_scroll_y(o)*3; }
static inline int32_t s3n_lv_obj_get_scroll_bottom(const lv_obj_t * o) { return lv_obj_get_scroll_bottom(o)*3; }
static inline int32_t s3n_lv_obj_get_scroll_top(const lv_obj_t * o) { return lv_obj_get_scroll_top(o)*3; }
static inline int32_t s3n_lv_obj_get_scroll_left(const lv_obj_t * o) { return lv_obj_get_scroll_left(o)*3; }
static inline int32_t s3n_lv_obj_get_scroll_right(const lv_obj_t * o) { return lv_obj_get_scroll_right(o)*3; }
static inline void s3n_lv_obj_get_coords(const lv_obj_t * o, lv_area_t * a) { lv_obj_get_coords(o,a); a->x1*=3; a->y1*=3; a->x2=(a->x2+1)*3-1; a->y2=(a->y2+1)*3-1; }
static inline int32_t s3n_lv_display_get_horizontal_resolution(const lv_display_t * d) { return lv_display_get_horizontal_resolution(d)*3; }
static inline int32_t s3n_lv_display_get_vertical_resolution(const lv_display_t * d) { return lv_display_get_vertical_resolution(d)*3; }
static inline void s3n_lv_indev_get_point(const lv_indev_t * i, lv_point_t * p) { lv_indev_get_point(i,p); p->x*=3; p->y*=3; }
static inline void s3n_lv_obj_align(lv_obj_t * o, lv_align_t a, int32_t x, int32_t y) { lv_obj_align(o,a,s3_native_px(x),s3_native_px(y)); }
static inline void s3n_lv_obj_align_to(lv_obj_t * o, const lv_obj_t * b, lv_align_t a, int32_t x, int32_t y) { lv_obj_align_to(o,b,a,s3_native_px(x),s3_native_px(y)); }
static inline void s3n_lv_obj_scroll_to_x(lv_obj_t * o, int32_t v, lv_anim_enable_t a) { lv_obj_scroll_to_x(o,s3_native_px(v),a); }
static inline void s3n_lv_obj_scroll_to_y(lv_obj_t * o, int32_t v, lv_anim_enable_t a) { lv_obj_scroll_to_y(o,s3_native_px(v),a); }
static inline void s3n_lv_obj_scroll_by(lv_obj_t * o, int32_t x, int32_t y, lv_anim_enable_t a) { lv_obj_scroll_by(o,s3_native_px(x),s3_native_px(y),a); }
static inline void s3n_lv_obj_set_style_arc_width(lv_obj_t * o, int32_t v, lv_style_selector_t s) { lv_obj_set_style_arc_width(o,s3_native_stroke(v),s); }
static inline void s3n_lv_obj_set_style_blur_radius(lv_obj_t * o, int32_t v, lv_style_selector_t s) { lv_obj_set_style_blur_radius(o,s3_native_px(v),s); }
static inline void s3n_lv_obj_set_style_border_width(lv_obj_t * o, int32_t v, lv_style_selector_t s) { lv_obj_set_style_border_width(o,s3_native_stroke(v),s); }
static inline void s3n_lv_obj_set_style_line_width(lv_obj_t * o, int32_t v, lv_style_selector_t s) { lv_obj_set_style_line_width(o,s3_native_stroke(v),s); }
static inline void s3n_lv_obj_set_style_margin_hor(lv_obj_t * o, int32_t v, lv_style_selector_t s) { lv_obj_set_style_margin_hor(o,s3_native_px(v),s); }
static inline void s3n_lv_obj_set_style_margin_left(lv_obj_t * o, int32_t v, lv_style_selector_t s) { lv_obj_set_style_margin_left(o,s3_native_px(v),s); }
static inline void s3n_lv_obj_set_style_margin_right(lv_obj_t * o, int32_t v, lv_style_selector_t s) { lv_obj_set_style_margin_right(o,s3_native_px(v),s); }
static inline void s3n_lv_obj_set_style_margin_ver(lv_obj_t * o, int32_t v, lv_style_selector_t s) { lv_obj_set_style_margin_ver(o,s3_native_px(v),s); }
static inline void s3n_lv_obj_set_style_max_height(lv_obj_t * o, int32_t v, lv_style_selector_t s) { lv_obj_set_style_max_height(o,s3_native_px(v),s); }
static inline void s3n_lv_obj_set_style_max_width(lv_obj_t * o, int32_t v, lv_style_selector_t s) { lv_obj_set_style_max_width(o,s3_native_px(v),s); }
static inline void s3n_lv_obj_set_style_min_height(lv_obj_t * o, int32_t v, lv_style_selector_t s) { lv_obj_set_style_min_height(o,s3_native_px(v),s); }
static inline void s3n_lv_obj_set_style_min_width(lv_obj_t * o, int32_t v, lv_style_selector_t s) { lv_obj_set_style_min_width(o,s3_native_px(v),s); }
static inline void s3n_lv_obj_set_style_pad_all(lv_obj_t * o, int32_t v, lv_style_selector_t s) { lv_obj_set_style_pad_all(o,s3_native_px(v),s); }
static inline void s3n_lv_obj_set_style_pad_bottom(lv_obj_t * o, int32_t v, lv_style_selector_t s) { lv_obj_set_style_pad_bottom(o,s3_native_px(v),s); }
static inline void s3n_lv_obj_set_style_pad_column(lv_obj_t * o, int32_t v, lv_style_selector_t s) { lv_obj_set_style_pad_column(o,s3_native_px(v),s); }
static inline void s3n_lv_obj_set_style_pad_gap(lv_obj_t * o, int32_t v, lv_style_selector_t s) { lv_obj_set_style_pad_gap(o,s3_native_px(v),s); }
static inline void s3n_lv_obj_set_style_pad_hor(lv_obj_t * o, int32_t v, lv_style_selector_t s) { lv_obj_set_style_pad_hor(o,s3_native_px(v),s); }
static inline void s3n_lv_obj_set_style_pad_left(lv_obj_t * o, int32_t v, lv_style_selector_t s) { lv_obj_set_style_pad_left(o,s3_native_px(v),s); }
static inline void s3n_lv_obj_set_style_pad_right(lv_obj_t * o, int32_t v, lv_style_selector_t s) { lv_obj_set_style_pad_right(o,s3_native_px(v),s); }
static inline void s3n_lv_obj_set_style_pad_row(lv_obj_t * o, int32_t v, lv_style_selector_t s) { lv_obj_set_style_pad_row(o,s3_native_px(v),s); }
static inline void s3n_lv_obj_set_style_pad_top(lv_obj_t * o, int32_t v, lv_style_selector_t s) { lv_obj_set_style_pad_top(o,s3_native_px(v),s); }
static inline void s3n_lv_obj_set_style_pad_ver(lv_obj_t * o, int32_t v, lv_style_selector_t s) { lv_obj_set_style_pad_ver(o,s3_native_px(v),s); }
static inline void s3n_lv_obj_set_style_radius(lv_obj_t * o, int32_t v, lv_style_selector_t s) { lv_obj_set_style_radius(o,s3_native_px(v),s); }
static inline void s3n_lv_obj_set_style_shadow_width(lv_obj_t * o, int32_t v, lv_style_selector_t s) { lv_obj_set_style_shadow_width(o,s3_native_px(v),s); }
static inline void s3n_lv_obj_set_style_shadow_offset_x(lv_obj_t * o, int32_t v, lv_style_selector_t s) { lv_obj_set_style_shadow_offset_x(o,s3_native_px(v),s); }
static inline void s3n_lv_obj_set_style_shadow_offset_y(lv_obj_t * o, int32_t v, lv_style_selector_t s) { lv_obj_set_style_shadow_offset_y(o,s3_native_px(v),s); }
static inline void s3n_lv_obj_set_style_shadow_spread(lv_obj_t * o, int32_t v, lv_style_selector_t s) { lv_obj_set_style_shadow_spread(o,s3_native_px(v),s); }
static inline void s3n_lv_obj_set_style_outline_width(lv_obj_t * o, int32_t v, lv_style_selector_t s) { lv_obj_set_style_outline_width(o,s3_native_stroke(v),s); }
static inline void s3n_lv_obj_set_style_outline_pad(lv_obj_t * o, int32_t v, lv_style_selector_t s) { lv_obj_set_style_outline_pad(o,s3_native_px(v),s); }
static inline void s3n_lv_obj_set_style_transform_height(lv_obj_t * o, int32_t v, lv_style_selector_t s) { lv_obj_set_style_transform_height(o,s3_native_px(v),s); }
static inline void s3n_lv_obj_set_style_transform_pivot_x(lv_obj_t * o, int32_t v, lv_style_selector_t s) { lv_obj_set_style_transform_pivot_x(o,s3_native_px(v),s); }
static inline void s3n_lv_obj_set_style_transform_pivot_y(lv_obj_t * o, int32_t v, lv_style_selector_t s) { lv_obj_set_style_transform_pivot_y(o,s3_native_px(v),s); }
static inline void s3n_lv_obj_set_style_transform_width(lv_obj_t * o, int32_t v, lv_style_selector_t s) { lv_obj_set_style_transform_width(o,s3_native_px(v),s); }
static inline void s3n_lv_obj_set_style_translate_x(lv_obj_t * o, int32_t v, lv_style_selector_t s) { lv_obj_set_style_translate_x(o,s3_native_px(v),s); }
static inline void s3n_lv_obj_set_style_translate_y(lv_obj_t * o, int32_t v, lv_style_selector_t s) { lv_obj_set_style_translate_y(o,s3_native_px(v),s); }
static inline void s3n_lv_obj_set_style_text_letter_space(lv_obj_t * o, int32_t v, lv_style_selector_t s) { lv_obj_set_style_text_letter_space(o,s3_native_px(v),s); }
static inline void s3n_lv_obj_set_style_text_line_space(lv_obj_t * o, int32_t v, lv_style_selector_t s) { lv_obj_set_style_text_line_space(o,s3_native_px(v),s); }
static inline int32_t s3n_lv_obj_get_style_pad_left(const lv_obj_t * o, lv_part_t p) { int32_t v=lv_obj_get_style_pad_left(o,p); return v==LV_RADIUS_CIRCLE ? v : v*3; }
static inline int32_t s3n_lv_obj_get_style_pad_right(const lv_obj_t * o, lv_part_t p) { int32_t v=lv_obj_get_style_pad_right(o,p); return v==LV_RADIUS_CIRCLE ? v : v*3; }
static inline int32_t s3n_lv_obj_get_style_pad_top(const lv_obj_t * o, lv_part_t p) { int32_t v=lv_obj_get_style_pad_top(o,p); return v==LV_RADIUS_CIRCLE ? v : v*3; }
static inline int32_t s3n_lv_obj_get_style_pad_bottom(const lv_obj_t * o, lv_part_t p) { int32_t v=lv_obj_get_style_pad_bottom(o,p); return v==LV_RADIUS_CIRCLE ? v : v*3; }
static inline int32_t s3n_lv_obj_get_style_radius(const lv_obj_t * o, lv_part_t p) { int32_t v=lv_obj_get_style_radius(o,p); return v==LV_RADIUS_CIRCLE ? v : v*3; }
static inline int32_t s3n_lv_obj_get_style_transform_height(const lv_obj_t * o, lv_part_t p) { int32_t v=lv_obj_get_style_transform_height(o,p); return v==LV_RADIUS_CIRCLE ? v : v*3; }
static inline int32_t s3n_lv_obj_get_style_transform_width(const lv_obj_t * o, lv_part_t p) { int32_t v=lv_obj_get_style_transform_width(o,p); return v==LV_RADIUS_CIRCLE ? v : v*3; }
static inline int32_t s3n_lv_obj_get_style_translate_x(const lv_obj_t * o, lv_part_t p) { int32_t v=lv_obj_get_style_translate_x(o,p); return v==LV_RADIUS_CIRCLE ? v : v*3; }
static inline int32_t s3n_lv_obj_get_style_translate_y(const lv_obj_t * o, lv_part_t p) { int32_t v=lv_obj_get_style_translate_y(o,p); return v==LV_RADIUS_CIRCLE ? v : v*3; }
static inline void s3n_lv_obj_set_style_text_font(lv_obj_t * o, const lv_font_t * f, lv_style_selector_t s) { lv_obj_set_style_text_font(o,s3_native_font(f),s); }
static inline void s3n_lv_draw_line(lv_layer_t * l, const lv_draw_line_dsc_t * p) { lv_draw_line_dsc_t d=*p; d.p1.x/=3; d.p1.y/=3; d.p2.x/=3; d.p2.y/=3; d.width=s3_native_stroke(d.width); d.dash_width=s3_native_px(d.dash_width); d.dash_gap=s3_native_px(d.dash_gap); lv_draw_line(l,&d); }
static inline void s3n_lv_draw_arc(lv_layer_t * l, const lv_draw_arc_dsc_t * p) { lv_draw_arc_dsc_t d=*p; d.center.x=s3_native_px(d.center.x); d.center.y=s3_native_px(d.center.y); d.radius=s3_native_stroke(d.radius); d.width=s3_native_stroke(d.width); lv_draw_arc(l,&d); }
static inline void s3n_lv_draw_triangle(lv_layer_t * l, const lv_draw_triangle_dsc_t * p) { lv_draw_triangle_dsc_t d=*p; for(int i=0;i<3;i++) {d.p[i].x/=3;d.p[i].y/=3;} lv_draw_triangle(l,&d); }
static inline void s3n_lv_draw_rect(lv_layer_t * l, const lv_draw_rect_dsc_t * p, const lv_area_t * a) { lv_draw_rect_dsc_t d=*p; d.radius=s3_native_px(d.radius); d.border_width=s3_native_stroke(d.border_width); d.outline_width=s3_native_stroke(d.outline_width); d.outline_pad=s3_native_px(d.outline_pad); d.shadow_width=s3_native_px(d.shadow_width); d.shadow_offset_x=s3_native_px(d.shadow_offset_x); d.shadow_offset_y=s3_native_px(d.shadow_offset_y); d.shadow_spread=s3_native_px(d.shadow_spread); lv_area_t b={s3_native_px(a->x1),s3_native_px(a->y1),s3_native_px(a->x2+1)-1,s3_native_px(a->y2+1)-1}; lv_draw_rect(l,&d,&b); }
#define lv_obj_set_x s3n_lv_obj_set_x
#define lv_obj_set_y s3n_lv_obj_set_y
#define lv_obj_set_width s3n_lv_obj_set_width
#define lv_obj_set_height s3n_lv_obj_set_height
#define lv_obj_set_ext_click_area s3n_lv_obj_set_ext_click_area
#define lv_obj_set_pos s3n_lv_obj_set_pos
#define lv_obj_set_size s3n_lv_obj_set_size
#define lv_tabview_set_tab_bar_size s3n_lv_tabview_set_tab_bar_size
#define lv_obj_get_x s3n_lv_obj_get_x
#define lv_obj_get_y s3n_lv_obj_get_y
#define lv_obj_get_width s3n_lv_obj_get_width
#define lv_obj_get_height s3n_lv_obj_get_height
#define lv_obj_get_content_width s3n_lv_obj_get_content_width
#define lv_obj_get_content_height s3n_lv_obj_get_content_height
#define lv_obj_get_scroll_x s3n_lv_obj_get_scroll_x
#define lv_obj_get_scroll_y s3n_lv_obj_get_scroll_y
#define lv_obj_get_scroll_bottom s3n_lv_obj_get_scroll_bottom
#define lv_obj_get_scroll_top s3n_lv_obj_get_scroll_top
#define lv_obj_get_scroll_left s3n_lv_obj_get_scroll_left
#define lv_obj_get_scroll_right s3n_lv_obj_get_scroll_right
#define lv_obj_get_coords s3n_lv_obj_get_coords
#define lv_display_get_horizontal_resolution s3n_lv_display_get_horizontal_resolution
#define lv_display_get_vertical_resolution s3n_lv_display_get_vertical_resolution
#define lv_indev_get_point s3n_lv_indev_get_point
#define lv_obj_align s3n_lv_obj_align
#define lv_obj_align_to s3n_lv_obj_align_to
#define lv_obj_scroll_to_x s3n_lv_obj_scroll_to_x
#define lv_obj_scroll_to_y s3n_lv_obj_scroll_to_y
#define lv_obj_scroll_by s3n_lv_obj_scroll_by
#define lv_obj_set_style_arc_width s3n_lv_obj_set_style_arc_width
#define lv_obj_set_style_blur_radius s3n_lv_obj_set_style_blur_radius
#define lv_obj_set_style_border_width s3n_lv_obj_set_style_border_width
#define lv_obj_set_style_line_width s3n_lv_obj_set_style_line_width
#define lv_obj_set_style_margin_hor s3n_lv_obj_set_style_margin_hor
#define lv_obj_set_style_margin_left s3n_lv_obj_set_style_margin_left
#define lv_obj_set_style_margin_right s3n_lv_obj_set_style_margin_right
#define lv_obj_set_style_margin_ver s3n_lv_obj_set_style_margin_ver
#define lv_obj_set_style_max_height s3n_lv_obj_set_style_max_height
#define lv_obj_set_style_max_width s3n_lv_obj_set_style_max_width
#define lv_obj_set_style_min_height s3n_lv_obj_set_style_min_height
#define lv_obj_set_style_min_width s3n_lv_obj_set_style_min_width
#define lv_obj_set_style_pad_all s3n_lv_obj_set_style_pad_all
#define lv_obj_set_style_pad_bottom s3n_lv_obj_set_style_pad_bottom
#define lv_obj_set_style_pad_column s3n_lv_obj_set_style_pad_column
#define lv_obj_set_style_pad_gap s3n_lv_obj_set_style_pad_gap
#define lv_obj_set_style_pad_hor s3n_lv_obj_set_style_pad_hor
#define lv_obj_set_style_pad_left s3n_lv_obj_set_style_pad_left
#define lv_obj_set_style_pad_right s3n_lv_obj_set_style_pad_right
#define lv_obj_set_style_pad_row s3n_lv_obj_set_style_pad_row
#define lv_obj_set_style_pad_top s3n_lv_obj_set_style_pad_top
#define lv_obj_set_style_pad_ver s3n_lv_obj_set_style_pad_ver
#define lv_obj_set_style_radius s3n_lv_obj_set_style_radius
#define lv_obj_set_style_shadow_width s3n_lv_obj_set_style_shadow_width
#define lv_obj_set_style_shadow_offset_x s3n_lv_obj_set_style_shadow_offset_x
#define lv_obj_set_style_shadow_offset_y s3n_lv_obj_set_style_shadow_offset_y
#define lv_obj_set_style_shadow_spread s3n_lv_obj_set_style_shadow_spread
#define lv_obj_set_style_outline_width s3n_lv_obj_set_style_outline_width
#define lv_obj_set_style_outline_pad s3n_lv_obj_set_style_outline_pad
#define lv_obj_set_style_transform_height s3n_lv_obj_set_style_transform_height
#define lv_obj_set_style_transform_pivot_x s3n_lv_obj_set_style_transform_pivot_x
#define lv_obj_set_style_transform_pivot_y s3n_lv_obj_set_style_transform_pivot_y
#define lv_obj_set_style_transform_width s3n_lv_obj_set_style_transform_width
#define lv_obj_set_style_translate_x s3n_lv_obj_set_style_translate_x
#define lv_obj_set_style_translate_y s3n_lv_obj_set_style_translate_y
#define lv_obj_set_style_text_letter_space s3n_lv_obj_set_style_text_letter_space
#define lv_obj_set_style_text_line_space s3n_lv_obj_set_style_text_line_space
#define lv_obj_get_style_pad_left s3n_lv_obj_get_style_pad_left
#define lv_obj_get_style_pad_right s3n_lv_obj_get_style_pad_right
#define lv_obj_get_style_pad_top s3n_lv_obj_get_style_pad_top
#define lv_obj_get_style_pad_bottom s3n_lv_obj_get_style_pad_bottom
#define lv_obj_get_style_radius s3n_lv_obj_get_style_radius
#define lv_obj_get_style_transform_height s3n_lv_obj_get_style_transform_height
#define lv_obj_get_style_transform_width s3n_lv_obj_get_style_transform_width
#define lv_obj_get_style_translate_x s3n_lv_obj_get_style_translate_x
#define lv_obj_get_style_translate_y s3n_lv_obj_get_style_translate_y
#define lv_obj_set_style_text_font s3n_lv_obj_set_style_text_font
#define lv_draw_line s3n_lv_draw_line
#define lv_draw_arc s3n_lv_draw_arc
#define lv_draw_triangle s3n_lv_draw_triangle
#define lv_draw_rect s3n_lv_draw_rect
#endif /* !S3_NATIVE_DRIVER */
#endif /* S3_NATIVE_LAYOUT */
#endif
