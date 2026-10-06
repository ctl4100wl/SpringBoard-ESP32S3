/**
 * @file media_av.c
 * FFmpeg (libavformat / libavcodec / libswscale / libswresample) linked into the program: tags, cover art,
 * audio through SDL and video frames, all decoded in-process.
 */
#include "media.h"

#include <SDL.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/channel_layout.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
#ifdef _WIN32
#include <windows.h>
#endif

void lv_image_cache_drop(const void * src);

#define RATE 44100

/* File dialogs hand us ANSI paths on Windows; FFmpeg wants UTF-8 */
static void to_utf8(const char * in, char * out, size_t n)
{
#ifdef _WIN32
    wchar_t w[1024];
    if(MultiByteToWideChar(CP_ACP, 0, in, -1, w, 1024) && WideCharToMultiByte(CP_UTF8, 0, w, -1, out, (int)n, NULL, NULL)) return;
#endif
    lv_strncpy(out, in, n - 1);
    out[n - 1] = '\0';
}

static AVFormatContext * open_file(const char * path)
{
    char u[1100];
    to_utf8(path, u, sizeof(u));
    AVFormatContext * fc = NULL;
    if(avformat_open_input(&fc, u, NULL, NULL) < 0) return NULL;
    if(avformat_find_stream_info(fc, NULL) < 0) {
        avformat_close_input(&fc);
        return NULL;
    }
    return fc;
}

bool media_available(void)
{
    return true;
}

static const char * tag(AVFormatContext * fc, const char * key)
{
    AVDictionaryEntry * e = av_dict_get(fc->metadata, key, NULL, 0);
    for(unsigned i = 0; !e && i < fc->nb_streams; i++) e = av_dict_get(fc->streams[i]->metadata, key, NULL, 0);
    return e ? e->value : "";
}

bool media_probe(const char * path, media_info_t * out)
{
    memset(out, 0, sizeof(*out));
    AVFormatContext * fc = open_file(path);
    if(!fc) return false;
    if(fc->duration > 0) out->duration = (double)fc->duration / AV_TIME_BASE;
    lv_strncpy(out->title, tag(fc, "title"), sizeof(out->title) - 1);
    lv_strncpy(out->artist, tag(fc, "artist"), sizeof(out->artist) - 1);
    if(!out->artist[0]) lv_strncpy(out->artist, tag(fc, "album_artist"), sizeof(out->artist) - 1);
    lv_strncpy(out->album, tag(fc, "album"), sizeof(out->album) - 1);
    avformat_close_input(&fc);
    return true;
}

lv_draw_buf_t * media_cover(const char * path, int32_t max_w, int32_t max_h)
{
    AVFormatContext * fc = open_file(path);
    if(!fc) return NULL;
    lv_draw_buf_t * buf = NULL;
    for(unsigned i = 0; i < fc->nb_streams && !buf; i++) {
        AVStream * st = fc->streams[i];
        if(st->disposition & AV_DISPOSITION_ATTACHED_PIC) {
            AVPacket * pk = &st->attached_pic;
            if(pk->size > 64) buf = sdk_image_decode_mem(pk->data, (size_t)pk->size, max_w, max_h);
        }
    }
    avformat_close_input(&fc);
    return buf;
}

/*---------------- audio ----------------*/

static struct {
    SDL_AudioDeviceID dev;
    pthread_t th;
    volatile int run, eof;
    bool started;
    uint64_t queued;
    double start;
    int volume;
    char path[1100];
} A = { 0, 0, 0, 0, false, 0, 0.0, 80, "" };

static void * audio_thread(void * arg)
{
    LV_UNUSED(arg);
    AVFormatContext * fc = open_file(A.path);
    if(!fc) {
        A.eof = 1;
        return NULL;
    }
    int si = av_find_best_stream(fc, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
    if(si < 0) {
        avformat_close_input(&fc);
        A.eof = 1;
        return NULL;
    }
    AVStream * st = fc->streams[si];
    const AVCodec * codec = avcodec_find_decoder(st->codecpar->codec_id);
    AVCodecContext * dec = codec ? avcodec_alloc_context3(codec) : NULL;
    if(!dec || avcodec_parameters_to_context(dec, st->codecpar) < 0 || avcodec_open2(dec, codec, NULL) < 0) {
        avcodec_free_context(&dec);
        avformat_close_input(&fc);
        A.eof = 1;
        return NULL;
    }
    SwrContext * swr = NULL;
    AVChannelLayout out_l = AV_CHANNEL_LAYOUT_STEREO;
    swr_alloc_set_opts2(&swr, &out_l, AV_SAMPLE_FMT_S16, RATE, &dec->ch_layout, dec->sample_fmt, dec->sample_rate, 0, NULL);
    if(!swr || swr_init(swr) < 0) {
        swr_free(&swr);
        avcodec_free_context(&dec);
        avformat_close_input(&fc);
        A.eof = 1;
        return NULL;
    }
    if(A.start > 0.5) av_seek_frame(fc, -1, (int64_t)(A.start * AV_TIME_BASE), AVSEEK_FLAG_BACKWARD);

    AVPacket * pk = av_packet_alloc();
    AVFrame * fr = av_frame_alloc();
    uint8_t * out = NULL;
    int out_cap = 0;
    bool drain = false;
    while(A.run) {
        int r = av_read_frame(fc, pk);
        if(r < 0) {
            if(drain) break;
            drain = true;
            avcodec_send_packet(dec, NULL);
        }
        else if(pk->stream_index != si) {
            av_packet_unref(pk);
            continue;
        }
        else {
            avcodec_send_packet(dec, pk);
            av_packet_unref(pk);
        }
        while(A.run && avcodec_receive_frame(dec, fr) == 0) {
            double pts = fr->best_effort_timestamp != AV_NOPTS_VALUE ? fr->best_effort_timestamp * av_q2d(st->time_base) : A.start;
            if(pts + (double)fr->nb_samples / dec->sample_rate < A.start - 0.01) {         /* before the seek target */
                av_frame_unref(fr);
                continue;
            }
            int need = swr_get_out_samples(swr, fr->nb_samples) + 64;
            if(need * 4 > out_cap) {
                free(out);
                out_cap = need * 4;
                out = malloc((size_t)out_cap);
            }
            int got = swr_convert(swr, &out, need, (const uint8_t **)fr->extended_data, fr->nb_samples);
            av_frame_unref(fr);
            if(got <= 0) continue;
            int16_t * s = (int16_t *)out;
            if(A.volume < 100) for(int i = 0; i < got * 2; i++) s[i] = (int16_t)(s[i] * A.volume / 100);
            while(A.run && SDL_GetQueuedAudioSize(A.dev) > RATE * 4 / 2) SDL_Delay(15);
            if(!A.run) break;
            SDL_QueueAudio(A.dev, out, (Uint32)(got * 4));
            A.queued += (uint64_t)got * 4;
        }
        if(drain) break;
    }
    A.eof = 1;
    free(out);
    av_frame_free(&fr);
    av_packet_free(&pk);
    swr_free(&swr);
    avcodec_free_context(&dec);
    avformat_close_input(&fc);
    return NULL;
}

void media_audio_stop(void)
{
    if(!A.started) return;
    A.run = 0;
    pthread_join(A.th, NULL);
    if(A.dev) SDL_ClearQueuedAudio(A.dev);
    A.started = false;
}

bool media_audio_play(const char * path, double start_sec)
{
    media_audio_stop();
    if(!A.dev) {
        if(!SDL_WasInit(SDL_INIT_AUDIO)) SDL_InitSubSystem(SDL_INIT_AUDIO);
        SDL_AudioSpec want, have;
        SDL_zero(want);
        want.freq = RATE;
        want.format = AUDIO_S16SYS;
        want.channels = 2;
        want.samples = 2048;
        A.dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
        if(!A.dev) return false;
    }
    SDL_ClearQueuedAudio(A.dev);
    lv_strncpy(A.path, path, sizeof(A.path) - 1);
    A.run = 1;
    A.eof = 0;
    A.queued = 0;
    A.start = start_sec;
    A.started = true;
    SDL_PauseAudioDevice(A.dev, 0);
    pthread_create(&A.th, NULL, audio_thread, NULL);
    return true;
}

void media_audio_pause(bool pause)
{
    if(A.dev) SDL_PauseAudioDevice(A.dev, pause ? 1 : 0);
}

bool media_audio_active(void)
{
    return A.started;
}

bool media_audio_ended(void)
{
    return A.started && A.eof && SDL_GetQueuedAudioSize(A.dev) == 0;
}

double media_audio_pos(void)
{
    if(!A.started) return 0.0;
    uint64_t queued = SDL_GetQueuedAudioSize(A.dev);
    uint64_t played = A.queued > queued ? A.queued - queued : 0;
    return A.start + (double)played / (RATE * 4.0);
}

double media_audio_duration(void)
{
    return 0;
}

void media_audio_volume(int percent)
{
    A.volume = percent < 0 ? 0 : percent > 100 ? 100 : percent;
}

/*---------------- video ----------------*/

struct media_video {
    pthread_t th;
    volatile int run, newf, eof, paused;
    int w, h;
    lv_draw_buf_t * buf[2];
    volatile int front;
    double start;
    char path[1100];
};

bool media_probe_video(const char * path, int * w, int * h, double * duration)
{
    AVFormatContext * fc = open_file(path);
    if(!fc) return false;
    int si = av_find_best_stream(fc, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    *w = *h = 0;
    *duration = fc->duration > 0 ? (double)fc->duration / AV_TIME_BASE : 0.0;
    if(si >= 0) {
        *w = fc->streams[si]->codecpar->width;
        *h = fc->streams[si]->codecpar->height;
    }
    avformat_close_input(&fc);
    return *w > 0 && *h > 0;
}

static void * video_thread(void * arg)
{
    media_video_t * v = arg;
    AVFormatContext * fc = open_file(v->path);
    if(!fc) {
        v->eof = 1;
        return NULL;
    }
    int si = av_find_best_stream(fc, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    AVStream * st = si >= 0 ? fc->streams[si] : NULL;
    const AVCodec * codec = st ? avcodec_find_decoder(st->codecpar->codec_id) : NULL;
    AVCodecContext * dec = codec ? avcodec_alloc_context3(codec) : NULL;
    if(!dec || avcodec_parameters_to_context(dec, st->codecpar) < 0) {
        avcodec_free_context(&dec);
        avformat_close_input(&fc);
        v->eof = 1;
        return NULL;
    }
    dec->thread_count = 0;
    if(avcodec_open2(dec, codec, NULL) < 0) {
        avcodec_free_context(&dec);
        avformat_close_input(&fc);
        v->eof = 1;
        return NULL;
    }
    struct SwsContext * sws = NULL;
    if(v->start > 0.5) av_seek_frame(fc, -1, (int64_t)(v->start * AV_TIME_BASE), AVSEEK_FLAG_BACKWARD);
    AVPacket * pk = av_packet_alloc();
    AVFrame * fr = av_frame_alloc();
    uint32_t t0 = SDL_GetTicks();
    int back = 0;
    bool drain = false;
    while(v->run) {
        int r = av_read_frame(fc, pk);
        if(r < 0) {
            if(drain) break;
            drain = true;
            avcodec_send_packet(dec, NULL);
        }
        else if(pk->stream_index != si) {
            av_packet_unref(pk);
            continue;
        }
        else {
            avcodec_send_packet(dec, pk);
            av_packet_unref(pk);
        }
        while(v->run && avcodec_receive_frame(dec, fr) == 0) {
            double pts = fr->best_effort_timestamp != AV_NOPTS_VALUE ? fr->best_effort_timestamp * av_q2d(st->time_base) : v->start;
            if(pts < v->start - 0.02) {
                av_frame_unref(fr);
                continue;
            }
            sws = sws_getCachedContext(sws, fr->width, fr->height, (enum AVPixelFormat)fr->format, v->w, v->h, AV_PIX_FMT_RGB565LE,
                                       SWS_BILINEAR, NULL, NULL, NULL);
            lv_draw_buf_t * b = v->buf[back];
            uint8_t * dst[1] = { (uint8_t *)b->data };
            int dst_stride[1] = { (int)b->header.stride };
            if(sws) sws_scale(sws, (const uint8_t * const *)fr->data, fr->linesize, 0, fr->height, dst, dst_stride);
            av_frame_unref(fr);

            while(v->run && v->paused) {
                SDL_Delay(15);
                t0 = SDL_GetTicks() - (uint32_t)((pts - v->start) * 1000.0);
            }
            if(media_audio_active()) {
                while(v->run && !v->paused && media_audio_pos() < pts && !media_audio_ended()) SDL_Delay(3);
            }
            else {
                while(v->run && (int32_t)(SDL_GetTicks() - t0) < (int32_t)((pts - v->start) * 1000.0)) SDL_Delay(3);
            }
            v->front = back;
            v->newf = 1;
            back = 1 - back;
        }
        if(drain) break;
    }
    v->eof = 1;
    sws_freeContext(sws);
    av_frame_free(&fr);
    av_packet_free(&pk);
    avcodec_free_context(&dec);
    avformat_close_input(&fc);
    return NULL;
}

media_video_t * media_video_open(const char * path, double start_sec, int w, int h)
{
    media_video_t * v = calloc(1, sizeof(*v));
    if(!v) return NULL;
    v->w = w;
    v->h = h;
    v->start = start_sec;
    lv_strncpy(v->path, path, sizeof(v->path) - 1);
    for(int i = 0; i < 2; i++) {
        v->buf[i] = lv_draw_buf_create((uint32_t)w, (uint32_t)h, LV_COLOR_FORMAT_RGB565, 0);
        if(!v->buf[i]) {
            media_video_close(v);
            return NULL;
        }
        lv_draw_buf_clear(v->buf[i], NULL);
    }
    v->run = 1;
    pthread_create(&v->th, NULL, video_thread, v);
    return v;
}

lv_draw_buf_t * media_video_frame(media_video_t * v, bool * fresh)
{
    if(!v) return NULL;
    bool nf = v->newf != 0;
    v->newf = 0;
    if(fresh) *fresh = nf;
    return v->buf[v->front];
}

bool media_video_ended(media_video_t * v)
{
    return !v || v->eof;
}

void media_video_pause(media_video_t * v, bool pause)
{
    if(v) v->paused = pause;
}

bool media_video_has_audio(void)
{
    return false;
}

double media_video_pos(media_video_t * v)
{
    return media_audio_active() ? media_audio_pos() : (v ? v->start : 0.0);
}

double media_video_duration(media_video_t * v)
{
    (void)v;
    return 0;
}

void media_video_close(media_video_t * v)
{
    if(!v) return;
    if(v->run) {
        v->run = 0;
        pthread_join(v->th, NULL);
    }
    for(int i = 0; i < 2; i++) {
        if(v->buf[i]) {
            lv_image_cache_drop(v->buf[i]);
            lv_draw_buf_destroy(v->buf[i]);
        }
    }
    free(v);
}
