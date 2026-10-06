#ifndef S3_SIMD_H
#define S3_SIMD_H
#ifdef __cplusplus
extern "C" {
#endif
/* This header is pulled in while LVGL public types are still being declared. */
int s3_simd_fill(void * descriptor);
int s3_simd_copy(void * descriptor);
#ifdef __cplusplus
}
#endif
#define LV_DRAW_SW_COLOR_BLEND_TO_RGB565(dsc) s3_simd_fill(dsc)
#define LV_DRAW_SW_RGB565_BLEND_NORMAL_TO_RGB565(dsc) s3_simd_copy(dsc)
#endif
