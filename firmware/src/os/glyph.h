/**
 * @file glyph.h
 * Control Center glyphs drawn from primitives (see glyph.c). Colour = the object's text colour.
 */
#ifndef GLYPH_H
#define GLYPH_H

#include "lvgl/lvgl.h"

typedef enum {
    GLYPH_SUN,
    GLYPH_SPEAKER,
} glyph_t;

lv_obj_t * glyph_create(lv_obj_t * parent, glyph_t g, int32_t size, uint32_t color);

#endif
