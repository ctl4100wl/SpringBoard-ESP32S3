/**
 * @file media_web.inc
 * Browser backend for the media API: the page's own <audio> / <video> decode and play, and frames are copied into
 * LVGL buffers. Files arrive through the web file picker, which writes them into the in-memory file system.
 */
#include "media.h"
#include <emscripten.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void lv_image_cache_drop(const void * src);

EM_JS(void, js_audio_play, (const char * path, double start, int vol), {
    var p = UTF8ToString(path);
    if(Module.s3a) { Module.s3a.pause(); Module.s3a = null; }
    var data = FS.readFile(p);
    var a = new Audio(URL.createObjectURL(new Blob([data])));
    a.volume = vol / 100;
    a.onended = function() { a.s3ended = true; };
    a.addEventListener('loadedmetadata', function() { if(start > 0.5) a.currentTime = start; });
    Module.s3a = a;
    a.play().catch(function(e) { console.warn('audio play blocked', e); });
});
EM_JS(void, js_audio_stop, (), { if(Module.s3a) { Module.s3a.pause(); Module.s3a = null; } });
EM_JS(void, js_audio_pause, (int pause), { if(Module.s3a) { if(pause) Module.s3a.pause(); else Module.s3a.play(); } });
EM_JS(int, js_audio_active, (), { return Module.s3a ? 1 : 0; });
EM_JS(int, js_audio_ended, (), { return Module.s3a && Module.s3a.s3ended ? 1 : 0; });
EM_JS(double, js_audio_pos, (), { return Module.s3a ? Module.s3a.currentTime : 0; });
EM_JS(double, js_audio_dur, (), { return Module.s3a && isFinite(Module.s3a.duration) ? Module.s3a.duration : 0; });
EM_JS(void, js_audio_vol, (int vol), { if(Module.s3a) Module.s3a.volume = vol / 100; });

EM_JS(void, js_video_open, (const char * path, double start, int w, int h, int vol), {
    var p = UTF8ToString(path);
    if(Module.s3v) { Module.s3v.pause(); Module.s3v = null; }
    var data = FS.readFile(p);
    var v = document.createElement('video');
    v.playsInline = true;
    v.volume = vol / 100;
    v.src = URL.createObjectURL(new Blob([data]));
    v.addEventListener('loadedmetadata', function() { if(start > 0.5) v.currentTime = start; });
    v.onended = function() { v.s3ended = true; };
    var c = document.createElement('canvas');
    c.width = w;
    c.height = h;
    Module.s3vc = c;
    Module.s3v = v;
    v.play().catch(function(e) { console.warn('video play blocked', e); });
});
EM_JS(int, js_video_grab, (unsigned char * dst, int stride, int w, int h), {
    var v = Module.s3v, c = Module.s3vc;
    if(!v || v.readyState < 2 || v.paused && Module.s3vlast === v.currentTime) return 0;
    Module.s3vlast = v.currentTime;
    var ctx = c.getContext('2d');
    ctx.fillStyle = '#000';
    ctx.fillRect(0, 0, w, h);
    var vw = v.videoWidth || w, vh = v.videoHeight || h;
    var s = Math.min(w / vw, h / vh);
    var dw = Math.round(vw * s), dh = Math.round(vh * s);
    ctx.drawImage(v, (w - dw) >> 1, (h - dh) >> 1, dw, dh);
    var px = ctx.getImageData(0, 0, w, h).data;
    var out = new Uint16Array(HEAPU8.buffer, dst, (stride * h) >> 1);
    var half = stride >> 1;
    for(var y = 0; y < h; y++) {
        var o = y * half, i = y * w * 4;
        for(var x = 0; x < w; x++, i += 4) out[o + x] = ((px[i] >> 3) << 11) | ((px[i + 1] >> 2) << 5) | (px[i + 2] >> 3);
    }
    return 1;
});
EM_JS(void, js_video_close, (), { if(Module.s3v) { Module.s3v.pause(); Module.s3v = null; } });
EM_JS(void, js_video_pause, (int pause), { if(Module.s3v) { if(pause) Module.s3v.pause(); else Module.s3v.play(); } });
EM_JS(int, js_video_ended, (), { return !Module.s3v || Module.s3v.s3ended ? 1 : 0; });
EM_JS(double, js_video_pos, (), { return Module.s3v ? Module.s3v.currentTime : 0; });
EM_JS(double, js_video_dur, (), { return Module.s3v && isFinite(Module.s3v.duration) ? Module.s3v.duration : 0; });

static int s_vol = 80;

bool media_available(void) { return true; }

bool media_probe(const char * path, media_info_t * out)
{
    LV_UNUSED(path);
    memset(out, 0, sizeof(*out));
    return true;
}

lv_draw_buf_t * media_cover(const char * path, int32_t w, int32_t h)
{
    LV_UNUSED(path);
    LV_UNUSED(w);
    LV_UNUSED(h);
    return NULL;
}

bool media_audio_play(const char * path, double start_sec)
{
    js_audio_play(path, start_sec, s_vol);
    return true;
}
void media_audio_stop(void) { js_audio_stop(); }
void media_audio_pause(bool pause) { js_audio_pause(pause ? 1 : 0); }
bool media_audio_active(void) { return js_audio_active() != 0; }
bool media_audio_ended(void) { return js_audio_ended() != 0; }
double media_audio_pos(void) { return js_audio_pos(); }
double media_audio_duration(void) { return js_audio_dur(); }
void media_audio_volume(int percent)
{
    s_vol = percent < 0 ? 0 : percent > 100 ? 100 : percent;
    js_audio_vol(s_vol);
}

struct media_video {
    lv_draw_buf_t * buf;
    int w, h;
};

bool media_probe_video(const char * path, int * w, int * h, double * duration)
{
    LV_UNUSED(path);
    *w = 640;
    *h = 360;
    *duration = 0;
    return true;
}

media_video_t * media_video_open(const char * path, double start_sec, int w, int h)
{
    media_video_t * v = calloc(1, sizeof(*v));
    if(!v) return NULL;
    v->w = w;
    v->h = h;
    v->buf = lv_draw_buf_create((uint32_t)w, (uint32_t)h, LV_COLOR_FORMAT_RGB565, 0);
    if(!v->buf) {
        free(v);
        return NULL;
    }
    lv_draw_buf_clear(v->buf, NULL);
    js_video_open(path, start_sec, w, h, s_vol);
    return v;
}

lv_draw_buf_t * media_video_frame(media_video_t * v, bool * fresh)
{
    if(!v) return NULL;
    bool nf = js_video_grab((unsigned char *)v->buf->data, (int)v->buf->header.stride, v->w, v->h) != 0;
    if(fresh) *fresh = nf;
    return v->buf;
}

bool media_video_ended(media_video_t * v) { return !v || js_video_ended() != 0; }
void media_video_pause(media_video_t * v, bool pause) { if(v) js_video_pause(pause ? 1 : 0); }
bool media_video_has_audio(void) { return true; }
double media_video_pos(media_video_t * v) { return v ? js_video_pos() : 0; }
double media_video_duration(media_video_t * v) { return v ? js_video_dur() : 0; }

void media_video_close(media_video_t * v)
{
    if(!v) return;
    js_video_close();
    lv_image_cache_drop(v->buf);
    lv_draw_buf_destroy(v->buf);
    free(v);
}
