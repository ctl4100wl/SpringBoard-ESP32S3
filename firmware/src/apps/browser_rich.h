#ifndef BROWSER_RICH_H
#define BROWSER_RICH_H

#include "lvgl/lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*rich_link_cb_t)(const char * url, void * user_data);

/* Lay out and draw an HTML page with the litehtml engine (real CSS, tables, floats, images). */
lv_obj_t * rich_create(lv_obj_t * parent, const char * html, const char * base_url, int32_t width,
                       rich_link_cb_t on_link, void * user_data, char * title_out, size_t title_n);

/* Blocking: inlines <link rel="stylesheet"> files into <style>. Call from a background job only. Returns malloc()ed text. */
char * rich_inline_css(const char * html, const char * base_url);

void rich_set_images(bool on);

#ifdef __cplusplus
}
#endif

#endif
