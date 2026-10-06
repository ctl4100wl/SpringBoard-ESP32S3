#include "lvgl/src/draw/sw/blend/lv_draw_sw_blend_private.h"
#include <stdint.h>
#include <stddef.h>
/* Stable assembly ABI used by Espressif. Never cast an LVGL descriptor into it. */
typedef struct {
 uint32_t opa; void * dst; uint32_t w, h, stride; const void * src;
 uint32_t src_stride; const void * mask; uint32_t mask_stride;
} asm_dsc_t;
_Static_assert(offsetof(asm_dsc_t, src) == 20, "S3 assembly ABI");
extern int lv_color_blend_to_rgb565_esp(asm_dsc_t * d);
extern int lv_rgb565_blend_normal_to_rgb565_esp(asm_dsc_t * d);
uint32_t s3_simd_hits;
static bool aligned(void * p, int w, int h, int stride)
{
 return !((uintptr_t)p & 15) && w >= 16 && !(w & 7) && h > 0 && !(stride & 15);
}
int s3_simd_fill(void * descriptor)
{
 lv_draw_sw_blend_fill_dsc_t * d = descriptor;
 if(d->mask_buf || d->opa != 255 || d->dest_w < 16 || d->dest_h <= 0 ||
    ((uintptr_t)d->dest_buf & 1) || (d->dest_stride & 1)) return LV_RESULT_INVALID;
 uint32_t color = lv_color_to_u32(d->color);
 if(aligned(d->dest_buf,d->dest_w,d->dest_h,d->dest_stride)) {
  asm_dsc_t a = {255, d->dest_buf, d->dest_w, d->dest_h, d->dest_stride, &color, 0, NULL, 0};
  lv_color_blend_to_rgb565_esp(&a); s3_simd_hits++; return LV_RESULT_OK;
 }
 uint16_t c = lv_color_to_u16(d->color);
 for(int y=0; y<d->dest_h; y++) {
  uint16_t * row=(uint16_t *)((uint8_t *)d->dest_buf+y*d->dest_stride);
  int remaining=d->dest_w;
  while(remaining && ((uintptr_t)row & 15)) { *row++=c; remaining--; }
  int bulk=remaining & ~7;
  if(bulk >= 16) {
   asm_dsc_t a={255,row,bulk,1,bulk*2,&color,0,NULL,0};
   lv_color_blend_to_rgb565_esp(&a); s3_simd_hits++; row+=bulk; remaining-=bulk;
  }
  while(remaining-- > 0) *row++=c;
 }
 return LV_RESULT_OK;
}
int s3_simd_copy(void * descriptor)
{
 lv_draw_sw_blend_image_dsc_t * d = descriptor;
 if(d->mask_buf || d->opa != 255 || !aligned(d->dest_buf, d->dest_w, d->dest_h, d->dest_stride) ||
    ((uintptr_t)d->src_buf & 15) || (d->src_stride & 15)) return LV_RESULT_INVALID;
 asm_dsc_t a = {255, d->dest_buf, d->dest_w, d->dest_h, d->dest_stride, d->src_buf, d->src_stride, NULL, 0};
 lv_rgb565_blend_normal_to_rgb565_esp(&a); s3_simd_hits++; return LV_RESULT_OK;
}

bool s3_simd_self_test(void)
{
 uint16_t src[80] __attribute__((aligned(16))), dst[80] __attribute__((aligned(16)));
 for(int i=0;i<80;i++) { src[i] = i*997; dst[i] = 0xa55a; }
 lv_draw_sw_blend_fill_dsc_t fill = { .dest_buf=dst+8, .dest_w=16, .dest_h=2, .dest_stride=48,
                                     .color=lv_color_hex(0x123456), .opa=255 };
 if(s3_simd_fill(&fill) != LV_RESULT_OK) return false;
 for(int i=0;i<80;i++) {
  bool written = (i>=8 && i<24) || (i>=32 && i<48);
  if(dst[i] != (written ? lv_color_to_u16(fill.color) : 0xa55a)) return false;
 }
 lv_draw_sw_blend_image_dsc_t image = { .dest_buf=dst+8, .dest_w=16, .dest_h=2, .dest_stride=48,
                                       .src_buf=src+8, .src_stride=48, .opa=255 };
 if(s3_simd_copy(&image) != LV_RESULT_OK) return false;
 for(int i=0;i<80;i++) {
  bool written = (i>=8 && i<24) || (i>=32 && i<48);
  if(dst[i] != (written ? src[i] : 0xa55a)) return false;
 }
 for(int offset=0;offset<8;offset++) for(int width=16;width<=24;width++) {
  for(int i=0;i<80;i++) dst[i]=0xa55a;
  fill.dest_buf=dst+8+offset; fill.dest_w=width; fill.dest_h=2; fill.dest_stride=64;
  if(s3_simd_fill(&fill)!=LV_RESULT_OK) return false;
  for(int i=0;i<80;i++) {
   bool written=(i>=8+offset && i<8+offset+width) || (i>=40+offset && i<40+offset+width);
   if(dst[i]!=(written ? lv_color_to_u16(fill.color) : 0xa55a)) return false;
  }
 }
 fill.opa=128;
 return s3_simd_fill(&fill) == LV_RESULT_INVALID;
}
