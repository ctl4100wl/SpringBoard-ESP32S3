#ifndef MEDIA_H
#define MEDIA_H

#include "../os/sdk.h"

typedef struct {
    char title[128], artist[128], album[128];
    double duration;
} media_info_t;

bool media_available(void);
bool media_probe(const char * path, media_info_t * out);
lv_draw_buf_t * media_cover(const char * path, int32_t max_w, int32_t max_h);

bool media_audio_play(const char * path, double start_sec);
void media_audio_stop(void);
void media_audio_pause(bool pause);
bool media_audio_active(void);
bool media_audio_ended(void);
double media_audio_pos(void);
double media_audio_duration(void);          /* 0 when unknown (the web backend learns it from the element) */
void media_audio_volume(int percent);

typedef struct media_video media_video_t;
bool media_probe_video(const char * path, int * w, int * h, double * duration);
media_video_t * media_video_open(const char * path, double start_sec, int w, int h);
lv_draw_buf_t * media_video_frame(media_video_t * v, bool * fresh);
bool media_video_ended(media_video_t * v);
void media_video_pause(media_video_t * v, bool pause);
bool media_video_has_audio(void);           /* true when the video itself makes the sound (web) */
double media_video_pos(media_video_t * v);
double media_video_duration(media_video_t * v);
void media_video_close(media_video_t * v);

#endif
