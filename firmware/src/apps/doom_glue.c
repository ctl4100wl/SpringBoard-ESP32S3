/**
 * @file doom_glue.c
 * Platform layer for doomgeneric (the portable Doom engine, GPL-2.0): frame, keys and timing hooks, plus a
 * setjmp so that quitting or an engine error returns to the phone instead of ending the whole program.
 */
#ifdef S3_DOOM

#include <setjmp.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#ifdef ESP_PLATFORM
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#else
#include <SDL.h>
#endif
#include "doomgeneric.h"
#include "doomkeys.h"

typedef void (*doom_frame_cb_t)(const uint32_t * pixels);

static jmp_buf dg_jmp;
static volatile int dg_dead;
static doom_frame_cb_t s_frame_cb;

#define KQ 64
static struct { unsigned char key; int pressed; } kq[KQ];
static _Atomic int kq_r, kq_w;

void doom_key(int pressed, unsigned char key)
{
    int n = (kq_w + 1) % KQ;
    if(n == kq_r) return;
    kq[kq_w].pressed = pressed;
    kq[kq_w].key = key;
    kq_w = n;
}

void DG_Init(void) {}

void DG_DrawFrame(void)
{
    if(s_frame_cb) s_frame_cb((const uint32_t *)DG_ScreenBuffer);
}

void DG_SleepMs(uint32_t ms)
{
#ifdef ESP_PLATFORM
    vTaskDelay(pdMS_TO_TICKS(ms) ? pdMS_TO_TICKS(ms) : 1);
#else
    SDL_Delay(ms);
#endif
}

uint32_t DG_GetTicksMs(void)
{
#ifdef ESP_PLATFORM
    return (uint32_t)(esp_timer_get_time() / 1000);
#else
    return SDL_GetTicks();
#endif
}

int DG_GetKey(int * pressed, unsigned char * key)
{
    if(kq_r == kq_w) return 0;
    *pressed = kq[kq_r].pressed;
    *key = kq[kq_r].key;
    kq_r = (kq_r + 1) % KQ;
    return 1;
}

void DG_SetWindowTitle(const char * title)
{
    (void)title;
}

static char dg_err[300];

void DG_SetError(const char * msg)
{
    strncpy(dg_err, msg, sizeof(dg_err) - 1);
    dg_err[sizeof(dg_err) - 1] = '\0';
}

const char * doom_last_error(void)
{
    return dg_err;
}

void DG_Quit(void)
{
    dg_dead = 1;
    longjmp(dg_jmp, 1);
}

bool doom_start(const char * wad, doom_frame_cb_t cb)
{
#ifdef ESP_PLATFORM
    static char * argv[] = { "doom", "-iwad", NULL, "-mb", "3", "-nosound", "-nomusic", "-savedir", "/doom/", "-config", "/doom/default.cfg", NULL };
#else
    static char * argv[] = { "doom", "-iwad", NULL, "-savedir", "cache/doom/saves/", "-config", "cache/doom/default.cfg", NULL };
#endif
    static char wad_copy[300];
    strncpy(wad_copy, wad, sizeof(wad_copy) - 1);
    argv[2] = wad_copy;
    s_frame_cb = cb;
    dg_dead = 0;
    if(setjmp(dg_jmp) == 0) {
        doomgeneric_Create((int)(sizeof(argv)/sizeof(argv[0])) - 1, argv);
        return true;
    }
    return false;
}

void doom_tick(void)
{
    if(dg_dead) return;
    if(setjmp(dg_jmp) == 0) doomgeneric_Tick();
}

bool doom_dead(void)
{
    return dg_dead != 0;
}

#endif
