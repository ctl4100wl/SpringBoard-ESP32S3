/**
 * @file app_video.c
 * Video player: ffmpeg decodes (any format it knows), frames are scaled to the screen and drawn here, sound goes to SDL.
 */
#include <stdio.h>
#include <string.h>
#include "../os/os.h"
#include "../os/ui.h"
#include "media.h"

static struct {
    lv_obj_t * img, * title, * seek, * t_now, * t_left, * play, * note;
    lv_timer_t * timer;
    bool seeking;
} u;

static struct {
    char path[260];
    double dur, start;
    int vw, vh, w, h;
    bool playing, loaded;
    media_video_t * v;
} s;

static const char * base_name(const char * path)
{
    const char * a = strrchr(path, '\\');
    const char * b = strrchr(path, '/');
    const char * x = a > b ? a : b;
    return x ? x + 1 : path;
}

static void stop(void)
{
    media_video_close(s.v);
    s.v = NULL;
    media_audio_stop();
    s.playing = false;
}

static void start_at(double sec)
{
    stop();
    if(!s.loaded) return;
    s.start = sec;
    s.v = media_video_open(s.path, sec, s.w, s.h);
    if(!s.v) {
        sdk_toast("Video", "ffmpeg could not open this file.");
        return;
    }
    media_audio_volume(80);
    if(!media_video_has_audio()) media_audio_play(s.path, sec);
    s.playing = true;
}

static void refresh(void)
{
    if(!u.title) return;
    lv_label_set_text(lv_obj_get_child(u.play, 0), s.playing ? LV_SYMBOL_PAUSE : LV_SYMBOL_PLAY);
    double pos = s.v ? media_video_pos(s.v) : s.start;
    if(pos > s.dur) pos = s.dur;
    if(!u.seeking) {
        lv_slider_set_range(u.seek, 0, s.dur > 1 ? (int32_t)s.dur : 1);
        lv_slider_set_value(u.seek, (int32_t)pos, LV_ANIM_OFF);
    }
    unsigned p = (unsigned)pos, l = s.dur > pos ? (unsigned)(s.dur - pos) : 0;
    lv_label_set_text_fmt(u.t_now, "%u:%02u", p / 60, p % 60);
    lv_label_set_text_fmt(u.t_left, "-%u:%02u", l / 60, l % 60);
}

static void timer_cb(lv_timer_t * t)
{
    LV_UNUSED(t);
    if(s.v) {
        double d = media_video_duration(s.v);
        if(d > 0) s.dur = d;
        bool fresh = false;
        lv_draw_buf_t * f = media_video_frame(s.v, &fresh);
        if(fresh && f) {
            lv_image_set_src(u.img, f);
            lv_obj_invalidate(u.img);
        }
        if(s.playing && media_video_ended(s.v) && (!media_audio_active() || media_audio_ended())) {
            s.playing = false;
            s.start = 0;
            stop();
        }
    }
    refresh();
}

static void opened_cb(const char * path, void * ud)
{
    LV_UNUSED(ud);
    if(!path || !u.title) return;
    stop();
    int vw, vh;
    double dur;
    if(!media_probe_video(path, &vw, &vh, &dur)) {
        sdk_toast("Video", "This file has no video that ffmpeg can read.");
        return;
    }
    lv_strncpy(s.path, path, sizeof(s.path) - 1);
    s.vw = vw;
    s.vh = vh;
    s.dur = dur;
    float sc = LV_MIN(296.0f / vw, 330.0f / vh);
    s.w = LV_MAX(16, ((int)(vw * sc)) & ~1);
    s.h = LV_MAX(16, ((int)(vh * sc)) & ~1);
    s.loaded = true;
    lv_label_set_text(u.title, base_name(path));
    lv_obj_set_size(u.img, s.w, s.h);
    start_at(0);
}

static void open_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    if(!media_available()) {
        sdk_toast("Video", "ffmpeg was not found. Put ffmpeg.exe and ffprobe.exe next to the program.");
        return;
    }
    sdk_pick_file_async("Open a video", "Video (mp4, mkv, avi, mov, webm...)\0*.mp4;*.mkv;*.avi;*.mov;*.webm;*.m4v;*.flv;*.wmv;*.mpg;*.ts\0",
                        opened_cb, NULL);
}

static void play_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    if(!s.loaded) return;
    if(s.v && s.playing) {
        media_video_pause(s.v, true);
        media_audio_pause(true);
        s.playing = false;
    }
    else if(s.v) {
        media_video_pause(s.v, false);
        media_audio_pause(false);
        s.playing = true;
    }
    else start_at(s.start);
    refresh();
}

static void seek_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if(code == LV_EVENT_PRESSED) u.seeking = true;
    else if(code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
        u.seeking = false;
        if(s.loaded) start_at((double)lv_slider_get_value(u.seek));
    }
}

static void root_delete_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    if(u.timer) lv_timer_delete(u.timer);
    stop();
    memset(&u, 0, sizeof(u));
}

static void create(lv_obj_t * root)
{
    lv_obj_t * body = ui_app_scaffold(root, "Video");
    lv_obj_set_flex_align(body, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(body, 10, 0);

    if(!media_available()) {
        u.note = ui_label(body, LV_SYMBOL_WARNING " ffmpeg not found - put ffmpeg.exe and ffprobe.exe next to the program",
                          &lv_font_montserrat_12, UI_ORANGE);
        lv_obj_set_width(u.note, LV_PCT(100));
        lv_label_set_long_mode(u.note, LV_LABEL_LONG_MODE_WRAP);
    }

    lv_obj_t * open = ui_button(body, LV_SYMBOL_VIDEO " Open video", UI_GROUP, open_cb, NULL);
    lv_obj_set_width(open, LV_PCT(100));

    lv_obj_t * screen = ui_box(body);
    lv_obj_set_size(screen, LV_PCT(100), 200);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(screen, lv_color_black(), 0);
    lv_obj_set_style_radius(screen, 12, 0);
    lv_obj_set_style_clip_corner(screen, true, 0);
    u.img = lv_image_create(screen);
    lv_obj_center(u.img);
    if(s.loaded) lv_obj_set_size(u.img, s.w, s.h);
    lv_obj_set_height(screen, s.loaded ? s.h : 200);

    u.title = ui_label(body, s.loaded ? base_name(s.path) : "No video selected", &lv_font_montserrat_14, UI_DIM);
    lv_obj_set_width(u.title, LV_PCT(100));
    lv_label_set_long_mode(u.title, LV_LABEL_LONG_MODE_DOTS);
    lv_obj_set_style_text_align(u.title, LV_TEXT_ALIGN_CENTER, 0);

    u.seek = lv_slider_create(body);
    lv_obj_set_width(u.seek, LV_PCT(100));
    lv_obj_set_height(u.seek, 6);
    lv_obj_set_style_bg_color(u.seek, lv_color_white(), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(u.seek, lv_color_white(), LV_PART_KNOB);
    lv_obj_set_style_pad_all(u.seek, 4, LV_PART_KNOB);
    lv_obj_add_event_cb(u.seek, seek_cb, LV_EVENT_ALL, NULL);

    lv_obj_t * times = ui_box(body);
    lv_obj_set_size(times, LV_PCT(100), 16);
    u.t_now = ui_label(times, "", &lv_font_montserrat_12, UI_DIM);
    lv_obj_align(u.t_now, LV_ALIGN_LEFT_MID, 0, 0);
    u.t_left = ui_label(times, "", &lv_font_montserrat_12, UI_DIM);
    lv_obj_align(u.t_left, LV_ALIGN_RIGHT_MID, 0, 0);

    u.play = ui_button(body, LV_SYMBOL_PLAY, 0x000000, play_cb, NULL);
    lv_obj_set_size(u.play, 64, 64);
    lv_obj_set_style_radius(u.play, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_text_font(lv_obj_get_child(u.play, 0), &lv_font_montserrat_28, 0);

    refresh();
    u.timer = lv_timer_create(timer_cb, 33, NULL);
    lv_obj_add_event_cb(root, root_delete_cb, LV_EVENT_DELETE, NULL);
}

const app_desc_t app_video = { "Video", LV_SYMBOL_VIDEO, 0x5E5CE6, create };
