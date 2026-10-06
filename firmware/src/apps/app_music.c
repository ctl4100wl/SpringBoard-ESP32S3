/**
 * @file app_music.c
 * Real music player: your own files, tags and cover art read with ffprobe / ffmpeg, playback through SDL.
 * Keeps playing (and moves on to the next song) while the app is closed.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../os/os.h"
#include "../os/ui.h"
#include "media.h"

void lv_image_cache_drop(const void * src);

#define MAX_TRK 40

typedef struct {
    char path[260];
    char title[96];
    char artist[96];
    double dur;
    bool probed;
} track_t;

static track_t T[MAX_TRK];
static int tn, cur = -1;
static bool playing, loaded;
static int volume = 70;
static lv_draw_buf_t * s_cover;
static lv_timer_t * s_bg;

static struct {
    lv_obj_t * art, * art_img, * art_sym, * title, * artist, * seek, * t_now, * t_left, * play, * list, * note;
    lv_timer_t * timer;
    bool seeking;
} u;

static const char * base_name(const char * path)
{
    const char * a = strrchr(path, '\\');
    const char * b = strrchr(path, '/');
    const char * s = a > b ? a : b;
    return s ? s + 1 : path;
}

static void default_title(const char * path, char * out, size_t n)
{
    lv_strncpy(out, base_name(path), n - 1);
    char * dot = strrchr(out, '.');
    if(dot) *dot = '\0';
}

static void load_list(void)
{
    if(loaded) return;
    loaded = true;
    volume = sdk_get_int("music.vol", 70);
    int cnt = sdk_get_int("music.n", 0);
    for(int i = 0; i < cnt && tn < MAX_TRK; i++) {
        char key[24];
        lv_snprintf(key, sizeof(key), "music.p%d", i);
        const char * p = sdk_get_str(key, "");
        if(!p[0]) continue;
        lv_strncpy(T[tn].path, p, sizeof(T[tn].path) - 1);
        default_title(p, T[tn].title, sizeof(T[tn].title));
        tn++;
    }
}

static void probe(int i)
{
    track_t * t = &T[i];
    if(t->probed) return;
    t->probed = true;
    media_info_t m;
    if(media_probe(t->path, &m)) {
        if(m.title[0]) lv_strncpy(t->title, m.title, sizeof(t->title) - 1);
        lv_strncpy(t->artist, m.artist, sizeof(t->artist) - 1);
        t->dur = m.duration;
    }
}

static void set_cover(int i)
{
    if(s_cover) {
        lv_image_cache_drop(s_cover);
        lv_draw_buf_destroy(s_cover);
        s_cover = NULL;
    }
    if(i >= 0) s_cover = media_cover(T[i].path, 150, 150);
}

static void list_build(void);

static void refresh(void)
{
    if(!u.title) return;
    if(cur >= 0) {
        lv_label_set_text(u.title, T[cur].title);
        lv_label_set_text(u.artist, T[cur].artist[0] ? T[cur].artist : "Unknown artist");
    }
    else {
        lv_label_set_text(u.title, tn ? "Pick a song" : "No music yet");
        lv_label_set_text(u.artist, tn ? "" : "Use Add songs");
    }
    lv_label_set_text(lv_obj_get_child(u.play, 0), playing ? LV_SYMBOL_PAUSE : LV_SYMBOL_PLAY);
    if(s_cover) {
        lv_image_set_src(u.art_img, s_cover);
        lv_obj_set_hidden(u.art_img, false);
        lv_obj_set_hidden(u.art_sym, true);
        lv_obj_set_style_bg_opa(u.art, LV_OPA_TRANSP, 0);
    }
    else {
        lv_obj_set_hidden(u.art_img, true);
        lv_obj_set_hidden(u.art_sym, false);
        lv_obj_set_style_bg_opa(u.art, LV_OPA_COVER, 0);
        lv_obj_set_style_bg_color(u.art, lv_color_hex(0xFF375F), 0);
        lv_obj_set_style_bg_grad_color(u.art, lv_color_hex(0x5A0F24), 0);
    }
    uint32_t pos = (uint32_t)media_audio_pos();
    if(cur >= 0 && media_audio_duration() > 0) T[cur].dur = media_audio_duration();
    uint32_t dur = cur >= 0 ? (uint32_t)T[cur].dur : 0;
    if(!u.seeking) {
        lv_slider_set_range(u.seek, 0, dur > 0 ? (int32_t)dur : 1);
        lv_slider_set_value(u.seek, (int32_t)pos, LV_ANIM_OFF);
    }
    lv_label_set_text_fmt(u.t_now, "%u:%02u", (unsigned)(pos / 60), (unsigned)(pos % 60));
    uint32_t left = dur > pos ? dur - pos : 0;
    lv_label_set_text_fmt(u.t_left, "-%u:%02u", (unsigned)(left / 60), (unsigned)(left % 60));
    lv_obj_set_style_transform_scale(u.art, playing ? 256 : 232, 0);
}

static void play_track(int i, double start)
{
    if(i < 0 || i >= tn) return;
    probe(i);
    if(i != cur || !s_cover) set_cover(i);
    cur = i;
    media_audio_volume(volume);
    playing = media_audio_play(T[i].path, start);
    if(!playing) sdk_toast("Music", "Could not play this file. Is ffmpeg installed?");
    refresh();
    list_build();
}

static void bg_cb(lv_timer_t * t)
{
    LV_UNUSED(t);
    if(playing && media_audio_ended()) {
        if(tn > 1) play_track((cur + 1) % tn, 0);
        else {
            playing = false;
            media_audio_stop();
            refresh();
        }
    }
}

static void ui_timer_cb(lv_timer_t * t)
{
    LV_UNUSED(t);
    refresh();
}

static void ensure_bg(void)
{
    if(!s_bg) s_bg = lv_timer_create(bg_cb, 400, NULL);
}

static void play_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    if(cur < 0) {
        if(tn) {
            ensure_bg();
            play_track(0, 0);
        }
        return;
    }
    ensure_bg();
    if(playing) {
        media_audio_pause(true);
        playing = false;
    }
    else if(media_audio_active()) {
        media_audio_pause(false);
        playing = true;
    }
    else play_track(cur, 0);
    refresh();
}

static void skip_cb(lv_event_t * e)
{
    int dir = (int)(intptr_t)lv_event_get_user_data(e);
    if(!tn) return;
    ensure_bg();
    if(dir < 0 && cur >= 0 && media_audio_pos() > 3.0) play_track(cur, 0);
    else play_track(((cur < 0 ? 0 : cur) + dir + tn) % tn, 0);
}

static void seek_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if(code == LV_EVENT_PRESSED) u.seeking = true;
    else if(code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
        u.seeking = false;
        if(cur >= 0) {
            ensure_bg();
            play_track(cur, (double)lv_slider_get_value(u.seek));
        }
    }
}

static void vol_cb(lv_event_t * e)
{
    volume = lv_slider_get_value(lv_event_get_target_obj(e));
    media_audio_volume(volume);
    sdk_set_int("music.vol", volume);
}

static void track_cb(lv_event_t * e)
{
    ensure_bg();
    play_track((int)(intptr_t)lv_event_get_user_data(e), 0);
}

static void added_cb(const char * path, void * ud)
{
    LV_UNUSED(ud);
    if(!path || !u.list) return;
    if(tn >= MAX_TRK) {
        sdk_toast("Music", "The playlist is full.");
        return;
    }
    int cnt = sdk_get_int("music.n", 0);
    char key[24];
    lv_snprintf(key, sizeof(key), "music.p%d", cnt);
    sdk_set_str(key, path);
    sdk_set_int("music.n", cnt + 1);
    lv_strncpy(T[tn].path, path, sizeof(T[tn].path) - 1);
    default_title(path, T[tn].title, sizeof(T[tn].title));
    T[tn].probed = false;
    probe(tn);
    tn++;
    list_build();
    refresh();
}

static void add_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    if(!media_available()) {
        sdk_toast("Music", "ffmpeg was not found. Put ffmpeg.exe and ffprobe.exe next to the program.");
        return;
    }
    sdk_pick_file_async("Add a song", "Audio (mp3, flac, m4a, ogg, wav...)\0*.mp3;*.flac;*.m4a;*.aac;*.ogg;*.opus;*.wav;*.wma;*.mp4;*.mkv\0",
                        added_cb, NULL);
}

static void list_build(void)
{
    if(!u.list) return;
    lv_obj_clean(u.list);
    for(int i = 0; i < tn; i++) {
        probe(i);
        lv_obj_t * row = ui_row(u.list, i == cur ? LV_SYMBOL_VOLUME_MAX : LV_SYMBOL_AUDIO, i == cur ? 0xFF375F : 0x636366, T[i].title);
        lv_obj_set_clickable(row, true);
        lv_obj_add_event_cb(row, track_cb, LV_EVENT_CLICKED, (void *)(intptr_t)i);
        if(T[i].dur > 0) {
            lv_obj_t * d = lv_label_create(row);
            lv_label_set_text_fmt(d, "%u:%02u", (unsigned)T[i].dur / 60u, (unsigned)T[i].dur % 60u);
            lv_obj_set_style_text_color(d, lv_color_hex(UI_DIM), 0);
        }
    }
    if(!tn) ui_label(u.list, "Add songs from your PC", &lv_font_montserrat_14, UI_DIM);
}

static void root_delete_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    if(u.timer) lv_timer_delete(u.timer);
    memset(&u, 0, sizeof(u));
}

static lv_obj_t * ctrl_btn(lv_obj_t * parent, const char * sym, int32_t size, lv_event_cb_t cb, intptr_t ud)
{
    lv_obj_t * b = ui_button(parent, sym, 0x000000, cb, (void *)ud);
    lv_obj_set_size(b, size, size);
    lv_obj_set_style_radius(b, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_text_font(lv_obj_get_child(b, 0), size > 60 ? &lv_font_montserrat_36 : &lv_font_montserrat_24, 0);
    return b;
}

static void create(lv_obj_t * root)
{
    load_list();
    lv_obj_t * body = ui_app_scaffold(root, "Music");
    lv_obj_set_flex_align(body, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(body, 8, 0);

    if(!media_available()) {
        u.note = ui_label(body, LV_SYMBOL_WARNING " ffmpeg not found - put ffmpeg.exe and ffprobe.exe next to the program",
                          &lv_font_montserrat_12, UI_ORANGE);
        lv_obj_set_width(u.note, LV_PCT(100));
        lv_label_set_long_mode(u.note, LV_LABEL_LONG_MODE_WRAP);
    }

    u.art = ui_box(body);
    lv_obj_set_size(u.art, 150, 150);
    lv_obj_set_style_radius(u.art, 16, 0);
    lv_obj_set_style_clip_corner(u.art, true, 0);
    lv_obj_set_style_bg_grad_dir(u.art, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_transform_pivot_x(u.art, 75, 0);
    lv_obj_set_style_transform_pivot_y(u.art, 75, 0);
    u.art_img = lv_image_create(u.art);
    lv_obj_center(u.art_img);
    u.art_sym = ui_label(u.art, LV_SYMBOL_AUDIO, &lv_font_montserrat_48, 0xFFFFFF);
    lv_obj_center(u.art_sym);

    u.title = ui_label(body, "", &lv_font_montserrat_20, 0xFFFFFF);
    lv_obj_set_width(u.title, LV_PCT(100));
    lv_label_set_long_mode(u.title, LV_LABEL_LONG_MODE_DOTS);
    lv_obj_set_style_text_align(u.title, LV_TEXT_ALIGN_CENTER, 0);
    u.artist = ui_label(body, "", &lv_font_montserrat_14, UI_DIM);

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

    lv_obj_t * ctl = ui_box(body);
    lv_obj_set_size(ctl, LV_PCT(100), 70);
    lv_obj_set_flex_flow(ctl, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(ctl, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    ctrl_btn(ctl, LV_SYMBOL_PREV, 52, skip_cb, -1);
    u.play = ctrl_btn(ctl, LV_SYMBOL_PLAY, 68, play_cb, 0);
    ctrl_btn(ctl, LV_SYMBOL_NEXT, 52, skip_cb, 1);

    lv_obj_t * vol = ui_box(body);
    lv_obj_set_size(vol, LV_PCT(100), 24);
    lv_obj_set_flex_flow(vol, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(vol, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(vol, 10, 0);
    ui_label(vol, LV_SYMBOL_VOLUME_MID, &lv_font_montserrat_14, UI_DIM);
    lv_obj_t * vs = lv_slider_create(vol);
    lv_obj_set_flex_grow(vs, 1);
    lv_obj_set_height(vs, 5);
    lv_slider_set_value(vs, volume, LV_ANIM_OFF);
    lv_obj_add_event_cb(vs, vol_cb, LV_EVENT_VALUE_CHANGED, NULL);
    ui_label(vol, LV_SYMBOL_VOLUME_MAX, &lv_font_montserrat_14, UI_DIM);

    lv_obj_t * add = ui_button(body, LV_SYMBOL_PLUS " Add songs", UI_GROUP, add_cb, NULL);
    lv_obj_set_width(add, LV_PCT(100));

    ui_label(body, "Playlist", &lv_font_montserrat_16, 0xFFFFFF);
    u.list = ui_card(body);
    list_build();
    refresh();
    u.timer = lv_timer_create(ui_timer_cb, 250, NULL);
    lv_obj_add_event_cb(root, root_delete_cb, LV_EVENT_DELETE, NULL);
}

const app_desc_t app_music = { "Music", LV_SYMBOL_AUDIO, 0xFF375F, create };
