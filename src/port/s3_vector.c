#define S3_NATIVE_DRIVER 1
#include "lvgl/lvgl.h"
#include "esp_log.h"
extern uint32_t s3_thorvg_simd_hits;
extern void lv_image_cache_drop(const void * src);
bool s3_vector_self_test(void)
{
    lv_draw_buf_t * buf = lv_draw_buf_create(64, 64, LV_COLOR_FORMAT_ARGB8888, LV_STRIDE_AUTO);
    if(!buf) return false;
    lv_obj_t * canvas = lv_canvas_create(lv_layer_top());
    lv_canvas_set_draw_buf(canvas, buf); lv_obj_add_flag(canvas, LV_OBJ_FLAG_HIDDEN);
    lv_canvas_fill_bg(canvas, lv_color_black(), LV_OPA_COVER);
    lv_layer_t layer; lv_canvas_init_layer(canvas, &layer);
    lv_draw_vector_dsc_t * dsc = lv_draw_vector_dsc_create(&layer);
    lv_vector_path_t * path = lv_vector_path_create(LV_VECTOR_PATH_QUALITY_MEDIUM);
    lv_vector_path_append_rectangle(path, 4, 4, 56, 56, 0, 0);
    lv_draw_vector_dsc_set_fill_color(dsc, lv_color_hex(0x22cc55));
    lv_draw_vector_dsc_add_path(dsc, path);
    uint32_t before = s3_thorvg_simd_hits;
    lv_draw_vector(dsc); lv_vector_path_delete(path); lv_draw_vector_dsc_delete(dsc);
    lv_canvas_finish_layer(canvas, &layer);
    uint32_t middle = *(uint32_t *)(buf->data + 32 * buf->header.stride + 32 * 4);
    bool ok = s3_thorvg_simd_hits > before && middle == 0xff22cc55;
    ESP_LOGI("s3_vector", "ThorVG pixel=%08lx SIMD spans=%lu", (unsigned long)middle, (unsigned long)(s3_thorvg_simd_hits - before));
    lv_obj_delete(canvas); lv_image_cache_drop(buf); lv_draw_buf_destroy(buf);
    return ok;
}
