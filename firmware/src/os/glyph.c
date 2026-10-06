/**
 * @file glyph.c
 * iOS 18 style slider glyphs (brightness sun, volume speaker) drawn from primitives.
 *
 * Each glyph is laid out on a 24 x 24 unit grid and scaled to the object's size, so it stays sharp at any size
 * and costs no font or image memory. The colour comes from the object's text colour, so callers that recolour
 * a toggle's child 0 (paint_toggle) recolour the glyph too.
 * LVGL arcs: 0 deg = 3 o'clock, angles grow clockwise.
 */
#include <math.h>
#include "sdk.h"
#include "glyph.h"

typedef struct {
    lv_layer_t * layer;
    float ox, oy, s;            /* unit -> px */
    lv_color_t col;
    lv_opa_t opa;
} pen_t;

static lv_point_precise_t P(pen_t * p, float x, float y)
{
    lv_point_precise_t r = { (lv_value_precise_t)(p->ox + x * p->s), (lv_value_precise_t)(p->oy + y * p->s) };
    return r;
}

static void line(pen_t * p, float x1, float y1, float x2, float y2, float w)
{
    lv_draw_line_dsc_t d;
    lv_draw_line_dsc_init(&d);
    d.p1 = P(p, x1, y1);
    d.p2 = P(p, x2, y2);
    d.width = LV_MAX(1, (int32_t)(w * p->s + 0.5f));
    d.color = p->col;
    d.opa = p->opa;
    d.round_start = d.round_end = 1;
    lv_draw_line(p->layer, &d);
}

static void arc(pen_t * p, float cx, float cy, float r, float a0, float a1, float w)
{
    lv_draw_arc_dsc_t d;
    lv_draw_arc_dsc_init(&d);
    d.center.x = (int32_t)(p->ox + cx * p->s + 0.5f);
    d.center.y = (int32_t)(p->oy + cy * p->s + 0.5f);
    d.radius = (uint16_t)LV_MAX(1, (int32_t)(r * p->s + 0.5f));
    d.start_angle = a0;
    d.end_angle = a1;
    d.width = LV_MAX(1, (int32_t)(w * p->s + 0.5f));
    d.color = p->col;
    d.opa = p->opa;
    d.rounded = 1;
    lv_draw_arc(p->layer, &d);
}

static void rect(pen_t * p, float x, float y, float w, float h, float r, bool fill, float bw)
{
    lv_draw_rect_dsc_t d;
    lv_draw_rect_dsc_init(&d);
    d.radius = (int32_t)(r * p->s + 0.5f);
    d.bg_opa = fill ? p->opa : LV_OPA_TRANSP;
    d.bg_color = p->col;
    d.border_width = fill ? 0 : LV_MAX(1, (int32_t)(bw * p->s + 0.5f));
    d.border_color = p->col;
    d.border_opa = p->opa;
    lv_area_t a = { (int32_t)(p->ox + x * p->s + 0.5f), (int32_t)(p->oy + y * p->s + 0.5f),
                    (int32_t)(p->ox + (x + w) * p->s + 0.5f) - 1, (int32_t)(p->oy + (y + h) * p->s + 0.5f) - 1 };
    lv_draw_rect(p->layer, &d, &a);
}

static void dot(pen_t * p, float cx, float cy, float r)
{
    rect(p, cx - r, cy - r, 2 * r, 2 * r, r, true, 0);
}

static void tri(pen_t * p, float x1, float y1, float x2, float y2, float x3, float y3)
{
    lv_draw_triangle_dsc_t d;
    lv_draw_triangle_dsc_init(&d);
    d.p[0] = P(p, x1, y1);
    d.p[1] = P(p, x2, y2);
    d.p[2] = P(p, x3, y3);
    d.color = p->col;
    d.opa = p->opa;
    lv_draw_triangle(p->layer, &d);
}

static void draw_glyph(pen_t * p, glyph_t g)
{
    switch(g) {
        case GLYPH_SUN:
            dot(p, 12, 12, 4.2f);
            for(int i = 0; i < 8; i++) {
                float a = i * 3.14159265f / 4.0f;
                float cs = cosf(a), sn = sinf(a);
                line(p, 12 + cs * 7.2f, 12 + sn * 7.2f, 12 + cs * 9.8f, 12 + sn * 9.8f, 1.9f);
            }
            break;
        case GLYPH_SPEAKER:
            rect(p, 3, 9, 5, 6, 1, true, 0);
            tri(p, 6, 9, 13, 3.5f, 13, 20.5f);
            tri(p, 6, 9, 13, 20.5f, 6, 15);
            arc(p, 13, 12, 4.5f, 315, 405, 1.8f);
            arc(p, 13, 12, 8.0f, 310, 410, 1.8f);
            break;
        default:
            break;
    }
}

static void glyph_draw_cb(lv_event_t * e)
{
    lv_obj_t * o = lv_event_get_current_target_obj(e);
    lv_area_t a;
    lv_obj_get_coords(o, &a);
    pen_t p;
    p.layer = lv_event_get_layer(e);
    float w = (float)lv_area_get_width(&a), h = (float)lv_area_get_height(&a);
    p.s = LV_MIN(w, h) / 24.0f;
    p.ox = a.x1 + (w - 24.0f * p.s) / 2.0f;
    p.oy = a.y1 + (h - 24.0f * p.s) / 2.0f;
    p.col = lv_obj_get_style_text_color(o, LV_PART_MAIN);
    p.opa = lv_obj_get_style_text_opa(o, LV_PART_MAIN);
    draw_glyph(&p, (glyph_t)(intptr_t)lv_event_get_user_data(e));
}

lv_obj_t * glyph_create(lv_obj_t * parent, glyph_t g, int32_t size, uint32_t color)
{
    lv_obj_t * o = ui_box(parent);
    lv_obj_set_size(o, size, size);
    lv_obj_set_style_text_color(o, lv_color_hex(color), 0);
    lv_obj_add_event_cb(o, glyph_draw_cb, LV_EVENT_DRAW_MAIN, (void *)(intptr_t)g);
    return o;
}
