#include "lvgl/lvgl.h"
#ifdef S3_NATIVE_LAYOUT
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
const lv_font_t * s3_native_font(const lv_font_t * f) {
if(!f) return &s3_font_5;
if(f==&s3_font_clock_32) return &s3_font_clock_32;
if(f==&s3_font_4) return f;
if(f==&s3_font_5) return f;
if(f==&s3_font_6) return f;
if(f==&s3_font_7) return f;
if(f==&s3_font_8) return f;
if(f==&s3_font_9) return f;
if(f==&s3_font_10) return f;
if(f==&s3_font_11) return f;
if(f==&s3_font_12) return f;
if(f==&s3_font_13) return f;
if(f==&s3_font_14) return f;
if(f==&s3_font_15) return f;
if(f==&s3_font_16) return f;
/* Built-in line_height includes leading; map the requested face size. */
int size=LV_CLAMP(4,(f->line_height+1)/3,16);
switch(size) {
case 4: return &s3_font_4;
case 5: return &s3_font_5;
case 6: return &s3_font_6;
case 7: return &s3_font_7;
case 8: return &s3_font_8;
case 9: return &s3_font_9;
case 10: return &s3_font_10;
case 11: return &s3_font_11;
case 12: return &s3_font_12;
case 13: return &s3_font_13;
case 14: return &s3_font_14;
case 15: return &s3_font_15;
case 16: return &s3_font_16;
default:return &s3_font_5;
}
}
#endif
