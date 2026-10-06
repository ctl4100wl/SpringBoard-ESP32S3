#include "media.h"

void lv_image_cache_drop(const void * src);

#ifdef S3_FFMPEG
#include "media_av.inc"
#elif defined(__EMSCRIPTEN__)
#include "media_web.inc"
#elif defined(ESP_PLATFORM)

bool media_available(void) { return false; }
bool media_probe(const char * path, media_info_t * out) { LV_UNUSED(path); LV_UNUSED(out); return false; }
lv_draw_buf_t * media_cover(const char * path, int32_t w, int32_t h) { LV_UNUSED(path); LV_UNUSED(w); LV_UNUSED(h); return NULL; }
bool media_audio_play(const char * path, double s) { LV_UNUSED(path); LV_UNUSED(s); return false; }
void media_audio_stop(void) {}
void media_audio_pause(bool p) { LV_UNUSED(p); }
bool media_audio_active(void) { return false; }
bool media_audio_ended(void) { return false; }
double media_audio_pos(void) { return 0; }
double media_audio_duration(void) { return 0; }
bool media_video_has_audio(void) { return false; }
double media_video_pos(media_video_t * v) { LV_UNUSED(v); return 0; }
double media_video_duration(media_video_t * v) { LV_UNUSED(v); return 0; }
void media_audio_volume(int p) { LV_UNUSED(p); }
bool media_probe_video(const char * path, int * w, int * h, double * d) { LV_UNUSED(path); LV_UNUSED(w); LV_UNUSED(h); LV_UNUSED(d); return false; }
media_video_t * media_video_open(const char * path, double s, int w, int h) { LV_UNUSED(path); LV_UNUSED(s); LV_UNUSED(w); LV_UNUSED(h); return NULL; }
lv_draw_buf_t * media_video_frame(media_video_t * v, bool * fresh) { LV_UNUSED(v); if(fresh) *fresh = false; return NULL; }
bool media_video_ended(media_video_t * v) { LV_UNUSED(v); return true; }
void media_video_pause(media_video_t * v, bool p) { LV_UNUSED(v); LV_UNUSED(p); }
void media_video_close(media_video_t * v) { LV_UNUSED(v); }

#else

#include <SDL.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#include <fcntl.h>
#include <io.h>
#endif

#define RATE 44100

typedef struct {
    FILE * f;
#ifdef _WIN32
    HANDLE proc;
#endif
} proc_t;

static bool proc_start(proc_t * p, const char * cmd)
{
    memset(p, 0, sizeof(*p));
#ifdef _WIN32
    SECURITY_ATTRIBUTES sa = { sizeof(sa), NULL, TRUE };
    HANDLE rd = NULL, wr = NULL;
    if(!CreatePipe(&rd, &wr, &sa, 1 << 16)) return false;
    SetHandleInformation(rd, HANDLE_FLAG_INHERIT, 0);
    HANDLE nul = CreateFileA("NUL", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, &sa, OPEN_EXISTING, 0, NULL);
    STARTUPINFOA si;
    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    si.hStdInput = nul;
    si.hStdOutput = wr;
    si.hStdError = nul;
    PROCESS_INFORMATION pi;
    char line[1200];
    lv_snprintf(line, sizeof(line), "%s", cmd);
    BOOL ok = CreateProcessA(NULL, line, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi);
    CloseHandle(wr);
    if(nul != INVALID_HANDLE_VALUE) CloseHandle(nul);
    if(!ok) {
        CloseHandle(rd);
        return false;
    }
    CloseHandle(pi.hThread);
    p->proc = pi.hProcess;
    int fd = _open_osfhandle((intptr_t)rd, _O_RDONLY | _O_BINARY);
    p->f = fd >= 0 ? _fdopen(fd, "rb") : NULL;
    if(!p->f) {
        TerminateProcess(p->proc, 1);
        CloseHandle(p->proc);
        return false;
    }
    return true;
#else
    p->f = popen(cmd, "r");
    return p->f != NULL;
#endif
}

static void proc_kill(proc_t * p)
{
    if(!p->f) return;
#ifdef _WIN32
    TerminateProcess(p->proc, 1);
    fclose(p->f);
    CloseHandle(p->proc);
#else
    pclose(p->f);
#endif
    p->f = NULL;
}

/* ffmpeg / ffprobe: next to the program first, then PATH */
static char s_ffmpeg[320], s_ffprobe[320];
static int s_avail = -1;

static void find_tools(void)
{
    lv_strncpy(s_ffmpeg, "ffmpeg", sizeof(s_ffmpeg) - 1);
    lv_strncpy(s_ffprobe, "ffprobe", sizeof(s_ffprobe) - 1);
#ifdef _WIN32
    char dir[300];
    DWORD n = GetModuleFileNameA(NULL, dir, sizeof(dir));
    if(n > 0 && n < sizeof(dir)) {
        char * slash = strrchr(dir, '\\');
        if(slash) {
            *slash = '\0';
            char a[340], b[340];
            lv_snprintf(a, sizeof(a), "%s\\ffmpeg.exe", dir);
            lv_snprintf(b, sizeof(b), "%s\\ffprobe.exe", dir);
            FILE * f = fopen(a, "rb");
            if(f) {
                fclose(f);
                lv_snprintf(s_ffmpeg, sizeof(s_ffmpeg), "\"%s\"", a);
                lv_snprintf(s_ffprobe, sizeof(s_ffprobe), "\"%s\"", b);
            }
        }
    }
#endif
}

bool media_available(void)
{
    if(s_avail < 0) {
        find_tools();
        proc_t p;
        char cmd[400];
        lv_snprintf(cmd, sizeof(cmd), "%s -version", s_ffmpeg);
        s_avail = 0;
        if(proc_start(&p, cmd)) {
            char b[16];
            if(fread(b, 1, sizeof(b), p.f) > 0) s_avail = 1;
            proc_kill(&p);
        }
    }
    return s_avail == 1;
}

static size_t run_capture(const char * cmd, uint8_t * buf, size_t cap)
{
    proc_t p;
    if(!proc_start(&p, cmd)) return 0;
    size_t n = 0, k;
    while(n < cap && (k = fread(buf + n, 1, cap - n, p.f)) > 0) n += k;
    proc_kill(&p);
    return n;
}

static bool key_is(const char * k, const char * name)
{
    for(; *name; k++, name++) {
        char c = *k;
        if(c >= 'A' && c <= 'Z') c = (char)(c + 32);
        if(c != *name) return false;
    }
    return *k == '\0';
}

bool media_probe(const char * path, media_info_t * out)
{
    if(!media_available()) return false;
    memset(out, 0, sizeof(*out));
    char cmd[700];
    lv_snprintf(cmd, sizeof(cmd), "%s -v quiet -show_entries format=duration:format_tags -of default=nw=1 \"%s\"", s_ffprobe, path);
    static char text[24 * 1024];
    size_t n = run_capture(cmd, (uint8_t *)text, sizeof(text) - 1);
    text[n] = '\0';
    char album_artist[128] = "";
    for(char * line = strtok(text, "\r\n"); line; line = strtok(NULL, "\r\n")) {
        if(!strncmp(line, "duration=", 9)) out->duration = atof(line + 9);
        else if(!strncmp(line, "TAG:", 4)) {
            char * eq = strchr(line + 4, '=');
            if(!eq) continue;
            *eq = '\0';
            const char * k = line + 4;
            const char * v = eq + 1;
            if(key_is(k, "title")) lv_strncpy(out->title, v, sizeof(out->title) - 1);
            else if(key_is(k, "artist")) lv_strncpy(out->artist, v, sizeof(out->artist) - 1);
            else if(key_is(k, "album_artist")) lv_strncpy(album_artist, v, sizeof(album_artist) - 1);
            else if(key_is(k, "album")) lv_strncpy(out->album, v, sizeof(out->album) - 1);
        }
    }
    if(!out->artist[0]) lv_strncpy(out->artist, album_artist, sizeof(out->artist) - 1);
    return n > 0;
}

lv_draw_buf_t * media_cover(const char * path, int32_t max_w, int32_t max_h)
{
    if(!media_available()) return NULL;
    char cmd[700];
    lv_snprintf(cmd, sizeof(cmd), "%s -v quiet -i \"%s\" -an -frames:v 1 -f image2pipe -c:v png -", s_ffmpeg, path);
    const size_t cap = 12u * 1024 * 1024;
    uint8_t * data = malloc(cap);
    if(!data) return NULL;
    size_t n = run_capture(cmd, data, cap);
    lv_draw_buf_t * buf = n > 64 ? sdk_image_decode_mem(data, n, max_w, max_h) : NULL;
    free(data);
    return buf;
}

/*---------------- audio ----------------*/

static struct {
    SDL_AudioDeviceID dev;
    proc_t p;
    pthread_t th;
    volatile int run, eof;
    bool started;
    uint64_t queued;
    double start;
    int volume;
} A = { 0, { 0 }, 0, 0, 0, false, 0, 0.0, 80 };

static void * audio_thread(void * arg)
{
    LV_UNUSED(arg);
    uint8_t buf[16384];
    while(A.run) {
        while(A.run && SDL_GetQueuedAudioSize(A.dev) > RATE * 4 / 2) SDL_Delay(15);
        if(!A.run) break;
        size_t n = fread(buf, 1, sizeof(buf), A.p.f);
        if(n == 0) {
            A.eof = 1;
            break;
        }
        if(A.volume < 100) {
            int16_t * s = (int16_t *)buf;
            for(size_t i = 0; i < n / 2; i++) s[i] = (int16_t)(s[i] * A.volume / 100);
        }
        SDL_QueueAudio(A.dev, buf, (Uint32)n);
        A.queued += n;
    }
    return NULL;
}

void media_audio_stop(void)
{
    if(!A.started) return;
    A.run = 0;
    proc_kill(&A.p);
    pthread_join(A.th, NULL);
    if(A.dev) SDL_ClearQueuedAudio(A.dev);
    A.started = false;
}

bool media_audio_play(const char * path, double start_sec)
{
    if(!media_available()) return false;
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
    char cmd[800];
    lv_snprintf(cmd, sizeof(cmd), "%s -v quiet -ss %.2f -i \"%s\" -vn -f s16le -acodec pcm_s16le -ar %d -ac 2 -", s_ffmpeg,
                start_sec, path, RATE);
    if(!proc_start(&A.p, cmd)) return false;
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

#define VFPS 24

struct media_video {
    proc_t p;
    pthread_t th;
    volatile int run, newf, eof, paused;
    int w, h;
    lv_draw_buf_t * buf[2];
    volatile int front;
    double start;
};

bool media_probe_video(const char * path, int * w, int * h, double * duration)
{
    if(!media_available()) return false;
    char cmd[700];
    lv_snprintf(cmd, sizeof(cmd), "%s -v quiet -select_streams v:0 -show_entries stream=width,height:format=duration -of default=nw=1 \"%s\"",
                s_ffprobe, path);
    static char text[4096];
    size_t n = run_capture(cmd, (uint8_t *)text, sizeof(text) - 1);
    text[n] = '\0';
    *w = *h = 0;
    *duration = 0;
    for(char * line = strtok(text, "\r\n"); line; line = strtok(NULL, "\r\n")) {
        if(!strncmp(line, "width=", 6)) *w = atoi(line + 6);
        else if(!strncmp(line, "height=", 7)) *h = atoi(line + 7);
        else if(!strncmp(line, "duration=", 9)) *duration = atof(line + 9);
    }
    return *w > 0 && *h > 0;
}

static void * video_thread(void * arg)
{
    media_video_t * v = arg;
    size_t fsz = (size_t)v->w * v->h * 2;
    uint8_t * tmp = malloc(fsz);
    uint64_t n = 0;
    uint32_t t0 = SDL_GetTicks();
    int back = 0;
    while(v->run && tmp) {
        if(fread(tmp, 1, fsz, v->p.f) != fsz) {
            v->eof = 1;
            break;
        }
        lv_draw_buf_t * b = v->buf[back];
        for(int y = 0; y < v->h; y++) memcpy((uint8_t *)b->data + (size_t)y * b->header.stride, tmp + (size_t)y * v->w * 2, (size_t)v->w * 2);
        double target = v->start + (double)n / VFPS;
        while(v->run && v->paused) {
            SDL_Delay(15);
            t0 = SDL_GetTicks() - (uint32_t)((double)n * 1000.0 / VFPS);
        }
        if(media_audio_active()) {
            while(v->run && !v->paused && media_audio_pos() < target && !media_audio_ended()) SDL_Delay(3);
        }
        else {
            while(v->run && (int32_t)(SDL_GetTicks() - t0) < (int32_t)((double)n * 1000.0 / VFPS)) SDL_Delay(3);
        }
        v->front = back;
        v->newf = 1;
        back = 1 - back;
        n++;
    }
    free(tmp);
    return NULL;
}

media_video_t * media_video_open(const char * path, double start_sec, int w, int h)
{
    if(!media_available()) return NULL;
    media_video_t * v = calloc(1, sizeof(*v));
    if(!v) return NULL;
    v->w = w;
    v->h = h;
    v->start = start_sec;
    for(int i = 0; i < 2; i++) {
        v->buf[i] = lv_draw_buf_create((uint32_t)w, (uint32_t)h, LV_COLOR_FORMAT_RGB565, 0);
        if(!v->buf[i]) {
            media_video_close(v);
            return NULL;
        }
        lv_draw_buf_clear(v->buf[i], NULL);
    }
    char cmd[900];
    lv_snprintf(cmd, sizeof(cmd), "%s -v quiet -ss %.2f -i \"%s\" -an -vf fps=%d,scale=%d:%d -f rawvideo -pix_fmt rgb565le -", s_ffmpeg,
                start_sec, path, VFPS, w, h);
    if(!proc_start(&v->p, cmd)) {
        media_video_close(v);
        return NULL;
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
        proc_kill(&v->p);
        pthread_join(v->th, NULL);
    }
    else proc_kill(&v->p);
    for(int i = 0; i < 2; i++) {
        if(v->buf[i]) {
            lv_image_cache_drop(v->buf[i]);
            lv_draw_buf_destroy(v->buf[i]);
        }
    }
    free(v);
}

#endif
