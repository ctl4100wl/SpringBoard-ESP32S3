#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
extern void s3_fill32_aligned(uint32_t * dst, uint32_t color, int blocks);
uint32_t s3_thorvg_simd_hits;
void s3_thorvg_fill32(uint32_t * dst, uint32_t color, int count)
{
    while(count > 0 && ((uintptr_t)dst & 15)) { *dst++ = color; count--; }
    int blocks = count / 4;
    if(blocks) { s3_fill32_aligned(dst, color, blocks); s3_thorvg_simd_hits++; dst += blocks * 4; count -= blocks * 4; }
    while(count-- > 0) *dst++ = color;
}
bool s3_thorvg_self_test(void)
{
    uint32_t pixels[48] __attribute__((aligned(16)));
    for(int offset = 0; offset < 4; offset++) for(int n = 0; n < 33; n++) {
        for(int i = 0; i < 48; i++) pixels[i] = 0x12345678;
        s3_thorvg_fill32(pixels + 4 + offset, 0xaabbccdd, n);
        for(int i = 0; i < 48; i++)
            if(pixels[i] != ((i >= 4 + offset && i < 4 + offset + n) ? 0xaabbccdd : 0x12345678)) return false;
    }
    return true;
}
