/**
 * @file sdk.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#ifndef _WIN32
#include <sys/stat.h>
#endif
#include "sdk.h"
#ifdef ESP_PLATFORM
#include "nvs.h"
#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include "esp_netif.h"
#include "esp_pthread.h"
#include "esp_heap_caps.h"
#endif

void lv_image_cache_drop(const void * src);

#define KV_FILE     "apps.kv"
#define KV_MAX      96
#define KV_KEY      40
#define KV_VAL      200
#ifdef ESP_PLATFORM
#define HTTP_MAX    (64 * 1024)
#else
#define HTTP_MAX    (512 * 1024)
#endif
#ifdef ESP_PLATFORM
#define BIN_MAX     (64 * 1024)
#else
#define BIN_MAX     (4 * 1024 * 1024)
#endif

/*================ Widget size ================*/

static widget_size_t s_widget_size;

widget_size_t sdk_widget_size(void)
{
    return s_widget_size;
}

void sdk_widget_size_set(widget_size_t size)
{
    s_widget_size = size;
}

static int s_widget_inst;

int sdk_widget_inst(void)
{
    return s_widget_inst;
}

void sdk_widget_inst_set(int k)
{
    s_widget_inst = k;
}

/*================ Timers ================*/

static void timer_owner_deleted(lv_event_t * e)
{
    lv_timer_delete(lv_event_get_user_data(e));
}

lv_timer_t * sdk_timer(lv_obj_t * owner, lv_timer_cb_t cb, uint32_t period_ms, void * user_data)
{
    lv_timer_t * t = lv_timer_create(cb, period_ms, user_data);
    lv_obj_add_event_cb(owner, timer_owner_deleted, LV_EVENT_DELETE, t);
    return t;
}

/*================ Key/value storage ================*/

static struct {
    char key[KV_KEY];
    char val[KV_VAL];
} s_kv[KV_MAX];
static int s_kv_n = -1;

static void kv_load(void)
{
    if(s_kv_n >= 0) return;
    s_kv_n = 0;
#ifdef ESP_PLATFORM
    nvs_handle_t handle;
    if(nvs_open("spring_apps", NVS_READONLY, &handle) != ESP_OK) return;
    size_t bytes = sizeof(s_kv);
    if(nvs_get_blob(handle, "kv_v1", s_kv, &bytes) == ESP_OK && bytes % sizeof(s_kv[0]) == 0) {
        s_kv_n = (int)(bytes / sizeof(s_kv[0]));
        for(int i = 0; i < s_kv_n; i++) {
            s_kv[i].key[KV_KEY - 1] = 0;
            s_kv[i].val[KV_VAL - 1] = 0;
        }
    }
    nvs_close(handle);
#else
    FILE * f = fopen(KV_FILE, "r");
    if(!f) return;
    char line[KV_KEY + KV_VAL + 4];
    while(s_kv_n < KV_MAX && fgets(line, sizeof(line), f)) {
        char * eq = strchr(line, '=');
        if(!eq) continue;
        *eq = '\0';
        line[strcspn(line, "\r\n")] = '\0';
        eq[1 + strcspn(eq + 1, "\r\n")] = '\0';
        lv_strncpy(s_kv[s_kv_n].key, line, KV_KEY - 1);
        lv_strncpy(s_kv[s_kv_n].val, eq + 1, KV_VAL - 1);
        s_kv_n++;
    }
    fclose(f);
#endif
}

static void kv_save(void)
{
#ifdef ESP_PLATFORM
    nvs_handle_t handle;
    if(nvs_open("spring_apps", NVS_READWRITE, &handle) != ESP_OK) return;
    if(nvs_set_blob(handle, "kv_v1", s_kv, (size_t)s_kv_n * sizeof(s_kv[0])) == ESP_OK) nvs_commit(handle);
    nvs_close(handle);
#else
    FILE * f = fopen(KV_FILE, "w");
    if(!f) return;
    for(int i = 0; i < s_kv_n; i++) fprintf(f, "%s=%s\n", s_kv[i].key, s_kv[i].val);
    fclose(f);
#endif
}

const char * sdk_get_str(const char * key, const char * def)
{
    kv_load();
    for(int i = 0; i < s_kv_n; i++) if(strcmp(s_kv[i].key, key) == 0) return s_kv[i].val;
    return def;
}

void sdk_set_str(const char * key, const char * value)
{
    kv_load();
    int i;
    for(i = 0; i < s_kv_n; i++) if(strcmp(s_kv[i].key, key) == 0) break;
    if(i == s_kv_n) {
        if(s_kv_n == KV_MAX) return;
        lv_strncpy(s_kv[i].key, key, KV_KEY - 1);
        s_kv_n++;
    }
    /* Newlines would break the line format */
    size_t j = 0;
    for(; value[j] && j < KV_VAL - 1; j++) s_kv[i].val[j] = (value[j] == '\n' || value[j] == '\r') ? ' ' : value[j];
    s_kv[i].val[j] = '\0';
    kv_save();
}

int sdk_get_int(const char * key, int def)
{
    const char * v = sdk_get_str(key, NULL);
    return v ? atoi(v) : def;
}

void sdk_set_int(const char * key, int value)
{
    char buf[16];
    lv_snprintf(buf, sizeof(buf), "%d", value);
    sdk_set_str(key, buf);
}

/*================ Networking ================*/

/* The worker only touches its own request; `done` hands it back to the UI thread. */
typedef struct http_req {
    struct http_req * next;
    char url[512];
    char * body;
    bool ok;
    volatile int done;
    sdk_http_cb_t cb;
    void * user_data;
    lv_obj_t * owner;
    size_t cap, len;                    /* buffer size / bytes received */
    sdk_http_data_cb_t bin_cb;
    sdk_job_fn_t job;                   /* generic background job instead of an HTTP request */
    sdk_job_done_t job_done;
    uint32_t t0, deadline;
} http_req_t;

static http_req_t * s_reqs;
static lv_timer_t * s_poll;
static bool s_online;

#ifdef ESP_PLATFORM
static size_t esp_get(const char * url, char * out, size_t cap, bool * ok)
{
    *ok = false;
    if(!out || cap < 2) return 0;
    out[0] = 0;
    esp_netif_t * interface = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    esp_netif_ip_info_t ip;
    if(!interface || !esp_netif_is_netif_up(interface) ||
       esp_netif_get_ip_info(interface, &ip) != ESP_OK || !ip.ip.addr) return 0;
    esp_http_client_config_t config = { .url = url, .timeout_ms = 5000,
                                       .crt_bundle_attach = esp_crt_bundle_attach };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if(!client) return 0;
    size_t bytes = 0;
    if(esp_http_client_open(client, 0) == ESP_OK && esp_http_client_fetch_headers(client) >= 0) {
        int code = esp_http_client_get_status_code(client);
        int got = 0;
        while(bytes < cap - 1 && (got = esp_http_client_read(client, out + bytes, cap - 1 - bytes)) > 0)
            bytes += (size_t)got;
        *ok = code >= 200 && code < 300 && esp_http_client_is_complete_data_received(client);
    }
    out[bytes] = 0;
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
    return bytes;
}
#endif

#ifdef _WIN32
#include <windows.h>
#include <winhttp.h>
/* WinHTTP: no helper process, so no console window flashing up in the release (GUI subsystem) build */
static size_t win_get(const char * url, char * out, size_t cap, bool * ok)
{
    size_t n = 0;
    *ok = false;
    wchar_t wurl[640];
    if(!MultiByteToWideChar(CP_UTF8, 0, url, -1, wurl, 640)) return 0;
    wchar_t host[256] = L"";
    URL_COMPONENTSW uc;
    memset(&uc, 0, sizeof(uc));
    uc.dwStructSize = sizeof(uc);
    uc.lpszHostName = host;
    uc.dwHostNameLength = 256;
    uc.dwUrlPathLength = (DWORD)-1;
    uc.dwExtraInfoLength = (DWORD)-1;
    if(!WinHttpCrackUrl(wurl, 0, 0, &uc)) return 0;
    HINTERNET ses = WinHttpOpen(L"S3-Springboard/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, NULL, NULL, 0);
    if(!ses) return 0;
    /* No decompression option: with it WinHttpReadData returned 0 bytes and every request looked failed */
    WinHttpSetTimeouts(ses, 5000, 5000, 8000, 8000);
    HINTERNET con = WinHttpConnect(ses, host, uc.nPort, 0);
    HINTERNET req = NULL;
    if(con) {
        req = WinHttpOpenRequest(con, L"GET", uc.lpszUrlPath,
                                 NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
                                 uc.nScheme == INTERNET_SCHEME_HTTPS ? WINHTTP_FLAG_SECURE : 0);
    }
    static const wchar_t accept[] = L"Accept: text/html,application/xhtml+xml,application/json,image/png,image/jpeg,image/gif,image/svg+xml,*/*;q=0.5\r\nAccept-Language: en-US,en;q=0.9\r\n";
    if(req && WinHttpSendRequest(req, accept, (DWORD)-1L, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) &&
       WinHttpReceiveResponse(req, NULL)) {
        DWORD code = 0, sz = sizeof(code);
        WinHttpQueryHeaders(req, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, NULL, &code, &sz, NULL);
        DWORD got;
        while(n < cap - 1 && WinHttpReadData(req, out + n, (DWORD)(cap - 1 - n), &got) && got > 0) n += got;
        *ok = code > 0 && code < 400 && n > 0;
    }
    if(req) WinHttpCloseHandle(req);
    if(con) WinHttpCloseHandle(con);
    WinHttpCloseHandle(ses);
    return n;
}
#endif

static void * http_worker(void * arg)
{
    http_req_t * r = arg;
    if(r->job) {
        r->job(r->user_data);
        r->ok = true;
        __atomic_store_n(&r->done, 1, __ATOMIC_RELEASE);
        return NULL;
    }
    size_t cap = r->cap ? r->cap : HTTP_MAX;
    r->body = malloc(cap);
    size_t n = 0;
#ifdef _WIN32
    if(r->body) n = win_get(r->url, r->body, cap, &r->ok);
    if(r->body) r->body[n] = '\0';
#elif defined(ESP_PLATFORM)
    if(r->body) n = esp_get(r->url, r->body, cap, &r->ok);
#else
    char cmd[640];
    /* curl ships with macOS and most Linux distros */
    lv_snprintf(cmd, sizeof(cmd), "curl -s -L -m 10 \"%s\"", r->url);
    FILE * p = popen(cmd, "r");
    if(p && r->body) {
        size_t k;
        while(n < cap - 1 && (k = fread(r->body + n, 1, cap - 1 - n, p)) > 0) n += k;
    }
    if(r->body) r->body[n] = '\0';
    int rc = p ? pclose(p) : -1;
    r->ok = rc == 0 && n > 0;
#endif
    r->len = n;
    __atomic_store_n(&r->done, 1, __ATOMIC_RELEASE);
    return NULL;
}

static void owner_deleted_cb(lv_event_t * e)
{
    http_req_t * r = lv_event_get_user_data(e);
    r->owner = NULL;
    r->cb = NULL;
    r->bin_cb = NULL;
    r->job_done = NULL;
}

static void http_poll_cb(lv_timer_t * t)
{
    LV_UNUSED(t);
    http_req_t ** pp = &s_reqs;
    while(*pp) {
        http_req_t * r = *pp;
        if(!__atomic_load_n(&r->done, __ATOMIC_ACQUIRE)) {
            if(r->job && r->deadline && lv_tick_elaps(r->t0) > r->deadline) {      /* runaway job: abandon it */
                *pp = r->next;
                if(r->owner) lv_obj_remove_event_cb_with_user_data(r->owner, owner_deleted_cb, r);
                if(r->job_done) r->job_done(r->user_data, false);
                continue;                       /* the thread still owns `r`: leaked on purpose */
            }
            pp = &r->next;
            continue;
        }
        *pp = r->next;
        if(!r->job && !r->bin_cb) s_online = r->ok;
        if(r->owner) lv_obj_remove_event_cb_with_user_data(r->owner, owner_deleted_cb, r);
        if(r->job_done) r->job_done(r->user_data, r->ok);
        else if(r->bin_cb) r->bin_cb(r->ok, (const uint8_t *)(r->body ? r->body : ""), r->len, r->user_data);
        else if(r->cb) r->cb(r->ok, r->body ? r->body : "", r->user_data);
        free(r->body);
        free(r);
    }
    if(!s_reqs) lv_timer_pause(s_poll);
}

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
/* Web demo networking: the browser's fetch(). Sites that allow cross-origin requests (weather APIs etc.) work
 * directly. Everything else is retried through a CORS proxy: window.S3_PROXY (a template ending in the target URL,
 * e.g. a Cloudflare Worker you host) first, then the free r.jina.ai reader, which returns the page's raw HTML. */
EM_JS(void, js_http_fetch, (void * req, const char * url_c, int cap), {
    var url = UTF8ToString(url_c);
    var tries = [{ u: url }];
    if(window.S3_PROXY) tries.push({ u: window.S3_PROXY + encodeURIComponent(url) });
    tries.push({ u: 'https://r.jina.ai/' + url, h: { 'X-Return-Format': 'html' } });
    function attempt(i) {
        if(i >= tries.length) { Module.ccall('s3_web_http_done', null, ['number', 'number', 'number', 'number'], [req, 0, 0, 0]); return; }
        fetch(tries[i].u, tries[i].h ? { headers: tries[i].h } : {}).then(function(r) {
            if(!r.ok) throw new Error('HTTP ' + r.status);
            return r.arrayBuffer();
        }).then(function(b) {
            var n = Math.min(b.byteLength, cap - 1);
            var p = _malloc(cap);
            HEAPU8.set(new Uint8Array(b, 0, n), p);
            HEAPU8[p + n] = 0;
            Module.ccall('s3_web_http_done', null, ['number', 'number', 'number', 'number'], [req, p, n, 1]);
        }).catch(function() { attempt(i + 1); });
    }
    attempt(0);
});

EMSCRIPTEN_KEEPALIVE void s3_web_http_done(http_req_t * r, char * body, int len, int ok)
{
    r->body = body;
    r->len = (size_t)len;
    r->ok = ok && len > 0;
    r->done = 1;
}
#endif

static void queue_req(http_req_t * r, lv_obj_t * owner)
{
    r->owner = owner;
    r->t0 = lv_tick_get();
    if(owner) lv_obj_add_event_cb(owner, owner_deleted_cb, LV_EVENT_DELETE, r);
    r->next = s_reqs;
    s_reqs = r;
    if(!s_poll) s_poll = lv_timer_create(http_poll_cb, 50, NULL);
    lv_timer_resume(s_poll);

#ifdef __EMSCRIPTEN__
    if(r->job) http_worker(r);          /* no threads on the web: jobs run inline */
    else js_http_fetch(r, r->url, (int)(r->cap ? r->cap : HTTP_MAX));
    return;
#endif
    pthread_t th;
#ifdef ESP_PLATFORM
    esp_pthread_cfg_t cfg = esp_pthread_get_default_config();
    cfg.stack_size = r->job ? 32768 : 8192;
    cfg.stack_alloc_caps = MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT;
    cfg.pin_to_core = 0;
    cfg.prio = 2;
    esp_pthread_set_cfg(&cfg);
#endif
    if(pthread_create(&th, NULL, http_worker, r) == 0) pthread_detach(th);
    else __atomic_store_n(&r->done, 1, __ATOMIC_RELEASE);
}

void sdk_http_get(const char * url, sdk_http_cb_t cb, void * user_data, lv_obj_t * owner)
{
    http_req_t * r = calloc(1, sizeof(*r));
    if(!r) return;
    lv_strncpy(r->url, url, sizeof(r->url) - 1);
    r->cb = cb;
    r->user_data = user_data;
    queue_req(r, owner);
}

void sdk_http_get_data_ex(const char * url, sdk_http_data_cb_t cb, void * user_data, lv_obj_t * owner, size_t max_bytes)
{
    http_req_t * r = calloc(1, sizeof(*r));
    if(!r) return;
    lv_strncpy(r->url, url, sizeof(r->url) - 1);
    r->bin_cb = cb;
#ifdef ESP_PLATFORM
    r->cap = LV_MIN(max_bytes, BIN_MAX);
#else
    r->cap = max_bytes;
#endif
    r->user_data = user_data;
    queue_req(r, owner);
}

void sdk_http_get_data(const char * url, sdk_http_data_cb_t cb, void * user_data, lv_obj_t * owner)
{
    sdk_http_get_data_ex(url, cb, user_data, owner, BIN_MAX);
}

void sdk_job_run(sdk_job_fn_t work, sdk_job_done_t done, void * user_data, lv_obj_t * owner, uint32_t timeout_ms)
{
    http_req_t * r = calloc(1, sizeof(*r));
    if(!r) return;
    r->job = work;
    r->job_done = done;
    r->user_data = user_data;
    r->deadline = timeout_ms;
    queue_req(r, owner);
}

size_t sdk_http_fetch_sync(const char * url, char * buf, size_t cap)
{
#ifdef ESP_PLATFORM
    bool ok = false;
    size_t n = esp_get(url, buf, cap, &ok);
    return ok ? n : 0;
#elif defined(_WIN32)
    bool ok = false;
    size_t n = win_get(url, buf, cap, &ok);
    if(ok) {
        buf[n] = '\0';
        return n;
    }
    return 0;
#else
    char cmd[640];
    lv_snprintf(cmd, sizeof(cmd), "curl -s -L -m 10 \"%s\"", url);
    FILE * p = popen(cmd, "r");
    size_t n = 0, k;
    if(p) {
        while(n < cap - 1 && (k = fread(buf + n, 1, cap - 1 - n, p)) > 0) n += k;
        pclose(p);
    }
    buf[n] = '\0';
    return n;
#endif
}

bool sdk_online(void)
{
    return s_online;
}

/*================ JSON ================*/

const char * sdk_json_find(const char * json, const char * key)
{
    if(!json) return NULL;
    char pat[64];
    lv_snprintf(pat, sizeof(pat), "\"%s\":", key);
    const char * p = strstr(json, pat);
    return p ? p + strlen(pat) : NULL;
}

bool sdk_json_num(const char * json, const char * key, double * out)
{
    const char * p = sdk_json_find(json, key);
    if(!p) return false;
    char * end;
    double v = strtod(p, &end);
    if(end == p) return false;
    *out = v;
    return true;
}

int sdk_json_num_array(const char * json, const char * key, double * out, int max)
{
    const char * p = sdk_json_find(json, key);
    if(!p || *p != '[') return 0;
    p++;
    int n = 0;
    while(n < max && *p && *p != ']') {
        char * end;
        double v = strtod(p, &end);
        if(end == p) break;
        out[n++] = v;
        p = end;
        while(*p == ',' || *p == ' ') p++;
    }
    return n;
}

/*================ System UI ================*/

void sdk_toast(const char * title, const char * text)
{
    notify_post(-1, title, text);
}

void sdk_notify(const app_desc_t * app, const char * title, const char * body)
{
    notify_post(app ? sdk_app_index(app) : -1, title, body);
}

/*================ Pull to refresh ================*/

#define PR_ARM      64
#define PR_GAP      48

typedef struct {
    sdk_refresh_cb_t cb;
    void * ud;
    lv_obj_t * spin;
    int32_t pad0;
    bool armed, busy;
} pr_t;

static bool pointer_down(void)
{
    for(lv_indev_t * in = lv_indev_get_next(NULL); in; in = lv_indev_get_next(in)) {
        if(lv_indev_get_type(in) == LV_INDEV_TYPE_POINTER && lv_indev_get_state(in) == LV_INDEV_STATE_PRESSED) return true;
    }
    return false;
}

static void pr_event_cb(lv_event_t * e)
{
    pr_t * pr = lv_event_get_user_data(e);
    lv_obj_t * body = lv_event_get_current_target_obj(e);
    if(lv_event_get_code(e) == LV_EVENT_DELETE) {
        lv_free(pr);
        return;
    }
    if(pr->busy) return;
    int32_t sy = lv_obj_get_scroll_y(body);
    int32_t pull = sy < 0 ? -sy : 0;
    lv_obj_set_style_opa(pr->spin, (lv_opa_t)LV_MIN(255, pull * 255 / PR_ARM), 0);
    lv_obj_set_style_transform_rotation(pr->spin, pull * 40, 0);
    if(pull >= PR_ARM && pointer_down()) pr->armed = true;
    if(pr->armed && !pointer_down()) {
        /* Released past the threshold: hold the content down while refreshing */
        pr->armed = false;
        pr->busy = true;
        lv_obj_set_style_opa(pr->spin, LV_OPA_COVER, 0);
        lv_obj_set_style_pad_top(body, pr->pad0 + PR_GAP, 0);
        lv_obj_set_style_arc_opa(pr->spin, LV_OPA_COVER, LV_PART_INDICATOR);
        pr->cb(body, pr->ud);
    }
}

static pr_t * pr_find(lv_obj_t * body)
{
    uint32_t n = lv_obj_get_event_count(body);
    for(uint32_t i = 0; i < n; i++) {
        lv_event_dsc_t * d = lv_obj_get_event_dsc(body, i);
        if(lv_event_dsc_get_cb(d) == pr_event_cb) return lv_event_dsc_get_user_data(d);
    }
    return NULL;
}

void sdk_pull_refresh(lv_obj_t * body, sdk_refresh_cb_t cb, void * user_data)
{
    pr_t * pr = lv_malloc_zeroed(sizeof(pr_t));
    pr->cb = cb;
    pr->ud = user_data;
    pr->pad0 = lv_obj_get_style_pad_top(body, 0);
    lv_obj_set_scroll_elastic(body, true);

    pr->spin = lv_spinner_create(body);
    lv_obj_set_floating(pr->spin, true);
    lv_obj_set_size(pr->spin, 26, 26);
    lv_obj_align(pr->spin, LV_ALIGN_TOP_MID, 0, -PR_GAP + 6);
    lv_obj_set_style_arc_width(pr->spin, 3, 0);
    lv_obj_set_style_arc_width(pr->spin, 3, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(pr->spin, lv_color_hex(0x48484A), 0);
    lv_obj_set_style_arc_color(pr->spin, lv_color_hex(0xC7C7CC), LV_PART_INDICATOR);
    lv_obj_set_style_opa(pr->spin, LV_OPA_TRANSP, 0);
    lv_obj_set_clickable(pr->spin, false);

    lv_obj_add_event_cb(body, pr_event_cb, LV_EVENT_SCROLL, pr);
    lv_obj_add_event_cb(body, pr_event_cb, LV_EVENT_DELETE, pr);
}

void sdk_pull_refresh_done(lv_obj_t * body)
{
    pr_t * pr = body ? pr_find(body) : NULL;
    if(!pr || !pr->busy) return;
    pr->busy = false;
    lv_obj_set_style_pad_top(body, pr->pad0, 0);
    lv_obj_set_style_opa(pr->spin, LV_OPA_TRANSP, 0);
}

/*================ Images from the PC ================*/

#ifdef _WIN32
#include <windows.h>
typedef struct {
    DWORD lStructSize; HWND hwndOwner; HINSTANCE hInstance; LPCSTR lpstrFilter; LPSTR lpstrCustomFilter;
    DWORD nMaxCustFilter; DWORD nFilterIndex; LPSTR lpstrFile; DWORD nMaxFile; LPSTR lpstrFileTitle;
    DWORD nMaxFileTitle; LPCSTR lpstrInitialDir; LPCSTR lpstrTitle; DWORD Flags; WORD nFileOffset;
    WORD nFileExtension; LPCSTR lpstrDefExt; LPARAM lCustData; void * lpfnHook; LPCSTR lpTemplateName;
    void * pvReserved; DWORD dwReserved; DWORD FlagsEx;
} ofn_t;    /* OPENFILENAMEA; comdlg32 is loaded at run time so no extra link library is needed */
#endif

bool sdk_pick_file(const char * title, const char * filter, char * path_out, size_t n)
{
#ifdef _WIN32
    HMODULE dll = LoadLibraryA("comdlg32.dll");
    if(!dll) return false;
    typedef BOOL (WINAPI * open_fn_t)(ofn_t *);
    open_fn_t open_fn = (open_fn_t)(void *)GetProcAddress(dll, "GetOpenFileNameA");
    bool ok = false;
    if(open_fn) {
        char file[MAX_PATH] = "";
        ofn_t o;
        memset(&o, 0, sizeof(o));
        o.lStructSize = sizeof(o);
        o.lpstrFilter = filter;
        o.lpstrFile = file;
        o.nMaxFile = sizeof(file);
        o.lpstrTitle = title;
        o.Flags = 0x00001000 | 0x00000800 | 0x00000008;     /* FILEMUSTEXIST | PATHMUSTEXIST | NOCHANGEDIR */
        if(open_fn(&o)) {
            lv_strncpy(path_out, file, n - 1);
            path_out[n - 1] = '\0';
            ok = true;
        }
    }
    FreeLibrary(dll);
    return ok;
#else
    LV_UNUSED(title);
    LV_UNUSED(filter);
    LV_UNUSED(path_out);
    LV_UNUSED(n);
    return false;
#endif
}

#ifdef __EMSCRIPTEN__
#include <emscripten.h>

static sdk_pick_cb_t s_pick_cb;
static void * s_pick_ud;

EM_JS(void, js_pick_file, (const char * accept), {
    var inp = document.createElement('input');
    inp.type = 'file';
    inp.accept = UTF8ToString(accept);
    inp.onchange = function() {
        var f = inp.files[0];
        if(!f) { Module.ccall('s3_web_picked', null, ['string'], ['']); return; }
        f.arrayBuffer().then(function(b) {
            try { FS.mkdir('/picked'); } catch(e) {}
            var path = '/picked/' + f.name.replace(/[^A-Za-z0-9._-]/g, '_');
            FS.writeFile(path, new Uint8Array(b));
            Module.ccall('s3_web_picked', null, ['string'], [path]);
        });
    };
    inp.click();
});

EMSCRIPTEN_KEEPALIVE void s3_web_picked(const char * path)
{
    sdk_pick_cb_t cb = s_pick_cb;
    s_pick_cb = NULL;
    if(cb) cb(path && path[0] ? path : NULL, s_pick_ud);
}

void sdk_pick_file_async(const char * title, const char * filter, sdk_pick_cb_t cb, void * user_data)
{
    LV_UNUSED(title);
    char accept[200] = "";
    const char * pat = filter + strlen(filter) + 1;
    size_t n = 0;
    for(; *pat && n < sizeof(accept) - 2; pat++) {
        if(*pat == '*') continue;
        accept[n++] = *pat == ';' ? ',' : *pat;
    }
    accept[n] = '\0';
    s_pick_cb = cb;
    s_pick_ud = user_data;
    js_pick_file(accept);
}
#else
void sdk_pick_file_async(const char * title, const char * filter, sdk_pick_cb_t cb, void * user_data)
{
    char path[260];
    bool ok = sdk_pick_file(title, filter, path, sizeof(path));
    cb(ok ? path : NULL, user_data);
}
#endif

bool sdk_pick_image(char * path_out, size_t n)
{
    return sdk_pick_file("Choose an image", "Images (*.png;*.jpg;*.jpeg;*.bmp)\0*.png;*.jpg;*.jpeg;*.bmp\0", path_out, n);
}

#ifndef ESP_PLATFORM
/* PC only: stb_image decodes what LVGL's built-in decoders cannot (progressive JPEG, odd PNGs) */
#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_STDIO
#define STBI_ONLY_JPEG
#define STBI_ONLY_PNG
#define STBI_ONLY_BMP
#define STBI_ONLY_GIF
#include "../third_party/stb/stb_image.h"

#define NANOSVG_IMPLEMENTATION
#define NANOSVGRAST_IMPLEMENTATION
#include "../third_party/nanosvg/nanosvg.h"
#include "../third_party/nanosvg/nanosvgrast.h"

/* RGBA bytes (w x h) -> scaled draw buffer. cover = fill w x h and crop; otherwise fit inside without upscaling.
 * alpha = ARGB8888 (keeps transparency), else RGB565. */
static lv_draw_buf_t * rgba_to_buf(const uint8_t * px, int iw, int ih, int32_t w, int32_t h, bool cover, bool alpha)
{
    float sc = cover ? LV_MAX((float)w / iw, (float)h / ih) : LV_MIN(1.0f, LV_MIN((float)w / iw, (float)h / ih));
    int32_t ow = cover ? w : LV_MAX(1, (int32_t)(iw * sc));
    int32_t oh = cover ? h : LV_MAX(1, (int32_t)(ih * sc));
    lv_draw_buf_t * buf = lv_draw_buf_create((uint32_t)ow, (uint32_t)oh, alpha ? LV_COLOR_FORMAT_ARGB8888 : LV_COLOR_FORMAT_RGB565, 0);
    if(!buf) return NULL;
    float ox = cover ? (iw * sc - ow) / 2.0f : 0.0f, oy = cover ? (ih * sc - oh) / 2.0f : 0.0f;
    for(int32_t y = 0; y < oh; y++) {
        uint8_t * rowp = (uint8_t *)buf->data + (size_t)y * buf->header.stride;
        int sy0 = (int)((y + oy) / sc), sy1 = (int)((y + 1 + oy) / sc);
        sy1 = LV_MAX(sy1, sy0 + 1);
        for(int32_t x = 0; x < ow; x++) {
            int sx0 = (int)((x + ox) / sc), sx1 = (int)((x + 1 + ox) / sc);
            sx1 = LV_MAX(sx1, sx0 + 1);
            unsigned r = 0, g = 0, b = 0, a = 0, c = 0;
            int stepy = LV_MAX(1, (sy1 - sy0) / 3), stepx = LV_MAX(1, (sx1 - sx0) / 3);
            for(int yy = sy0; yy < sy1; yy += stepy) {
                if(yy < 0 || yy >= ih) continue;
                for(int xx = sx0; xx < sx1; xx += stepx) {
                    if(xx < 0 || xx >= iw) continue;
                    const uint8_t * q = px + ((size_t)yy * iw + xx) * 4;
                    unsigned qa = q[3];
                    r += q[0] * qa;
                    g += q[1] * qa;
                    b += q[2] * qa;
                    a += qa;
                    c++;
                }
            }
            if(!c) c = 1;
            unsigned rr, gg, bb, aa = a / c;
            if(a) {
                rr = r / a;
                gg = g / a;
                bb = b / a;
            }
            else rr = gg = bb = 0;
            if(alpha) {
                uint8_t * d = rowp + (size_t)x * 4;
                d[0] = (uint8_t)bb;
                d[1] = (uint8_t)gg;
                d[2] = (uint8_t)rr;
                d[3] = (uint8_t)aa;
            }
            else {
                ((uint16_t *)rowp)[x] = (uint16_t)((rr >> 3) << 11 | (gg >> 2) << 5 | (bb >> 3));
            }
        }
    }
    return buf;
}

static lv_draw_buf_t * stb_to_buf(const uint8_t * data, size_t len, int32_t w, int32_t h, bool cover, bool alpha)
{
    int iw, ih, n;
    uint8_t * px = stbi_load_from_memory(data, (int)len, &iw, &ih, &n, 4);
    if(!px) return NULL;
    lv_draw_buf_t * buf = rgba_to_buf(px, iw, ih, w, h, cover, alpha);
    stbi_image_free(px);
    return buf;
}

static bool looks_like_svg(const uint8_t * d, size_t len)
{
    size_t lim = len < 600 ? len : 600;
    for(size_t i = 0; i + 4 < lim; i++) if(d[i] == '<' && (d[i + 1] == 's' || d[i + 1] == 'S') && (d[i + 2] == 'v' || d[i + 2] == 'V') && (d[i + 3] == 'g' || d[i + 3] == 'G')) return true;
    return false;
}

static lv_draw_buf_t * svg_to_buf(const uint8_t * data, size_t len, int32_t max_w, int32_t max_h)
{
    char * txt = malloc(len + 1);
    if(!txt) return NULL;
    memcpy(txt, data, len);
    txt[len] = '\0';
    NSVGimage * im = nsvgParse(txt, "px", 96.0f);
    free(txt);
    if(!im) return NULL;
    lv_draw_buf_t * out = NULL;
    if(im->width > 0 && im->height > 0) {
        float sc = LV_MIN(2.0f, LV_MIN((float)max_w / im->width, (float)max_h / im->height));
        int iw = LV_MAX(1, (int)(im->width * sc)), ih = LV_MAX(1, (int)(im->height * sc));
        NSVGrasterizer * rast = nsvgCreateRasterizer();
        uint8_t * px = malloc((size_t)iw * ih * 4);
        if(rast && px) {
            memset(px, 0, (size_t)iw * ih * 4);
            nsvgRasterize(rast, im, 0, 0, sc, px, iw, ih, iw * 4);
            out = rgba_to_buf(px, iw, ih, iw, ih, false, true);
        }
        free(px);
        if(rast) nsvgDeleteRasterizer(rast);
    }
    nsvgDelete(im);
    return out;
}

static lv_draw_buf_t * stb_load_file(const char * path, int32_t w, int32_t h)
{
    FILE * f = fopen(path, "rb");
    if(!f) return NULL;
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t * data = len > 16 && len < 64L * 1024 * 1024 ? malloc((size_t)len) : NULL;
    lv_draw_buf_t * out = NULL;
    if(data && fread(data, 1, (size_t)len, f) == (size_t)len) out = stb_to_buf(data, (size_t)len, w, h, true, false);
    free(data);
    fclose(f);
    return out;
}
#endif

lv_draw_buf_t * sdk_image_load(const char * path, int32_t w, int32_t h)
{
    if(!path || !path[0]) return NULL;
#ifndef ESP_PLATFORM
    lv_draw_buf_t * stb = stb_load_file(path, w, h);
    if(stb) return stb;
#endif
    char src[300];
    lv_snprintf(src, sizeof(src), "A:%s", path);
    lv_image_header_t hd;
    if(lv_image_decoder_get_info(src, &hd) != LV_RESULT_OK) return NULL;

    /* Render once, off-screen, into a screen-sized buffer: no decoding on every frame later */
    lv_obj_t * box = ui_box(lv_layer_sys());
    lv_obj_set_size(box, w, h);
    lv_obj_set_pos(box, -w - 100, 0);
    lv_obj_t * img = lv_image_create(box);
    lv_image_set_src(img, src);
    lv_obj_set_size(img, w, h);
    lv_image_set_inner_align(img, LV_IMAGE_ALIGN_COVER);
    lv_obj_update_layout(box);
    lv_draw_buf_t * buf = lv_snapshot_take(box, LV_COLOR_FORMAT_RGB565);
    lv_obj_delete(box);
    lv_image_cache_drop(src);
    return buf;
}

lv_draw_buf_t * sdk_image_decode_mem(const uint8_t * data, size_t len, int32_t max_w, int32_t max_h)
{
    if(!data || len < 16) return NULL;
#ifndef ESP_PLATFORM
    lv_draw_buf_t * stb = looks_like_svg(data, len) ? svg_to_buf(data, len, max_w, max_h) : stb_to_buf(data, len, max_w, max_h, false, true);
    if(stb) return stb;
#endif
    lv_image_dsc_t dsc;
    lv_memzero(&dsc, sizeof(dsc));
    dsc.header.magic = LV_IMAGE_HEADER_MAGIC;
    dsc.header.cf = LV_COLOR_FORMAT_RAW;
    dsc.data_size = (uint32_t)len;
    dsc.data = data;
    lv_image_header_t hd;
    if(lv_image_decoder_get_info(&dsc, &hd) != LV_RESULT_OK || hd.w <= 0 || hd.h <= 0) return NULL;
    int32_t tw = (int32_t)hd.w, th = (int32_t)hd.h;
#ifdef S3_NATIVE_LAYOUT
    tw *= 3; th *= 3;
#endif
    if(tw > max_w || th > max_h) {
        int32_t sw = max_w * 1024 / tw, sh = max_h * 1024 / th;
        int32_t sc = LV_MIN(sw, sh);
        tw = LV_MAX(8, tw * sc / 1024);
        th = LV_MAX(8, th * sc / 1024);
    }
    lv_obj_t * box = ui_box(lv_layer_sys());
    lv_obj_set_size(box, tw, th);
    lv_obj_set_pos(box, -tw - 100, 0);
    lv_obj_t * img = lv_image_create(box);
    lv_image_set_src(img, &dsc);
    lv_obj_set_size(img, tw, th);
    lv_image_set_inner_align(img, LV_IMAGE_ALIGN_STRETCH);
    lv_obj_update_layout(box);
    lv_draw_buf_t * buf = lv_snapshot_take(box, LV_COLOR_FORMAT_RGB565);
    lv_obj_delete(box);
    lv_image_cache_drop(&dsc);
    return buf;
}

void sdk_ensure_dir(const char * path)
{
    char tmp[260];
    lv_strncpy(tmp, path, sizeof(tmp) - 1);
    for(char * p = tmp + 1; ; p++) {
        if(*p == '/' || *p == '\\' || *p == '\0') {
            char c = *p;
            *p = '\0';
#ifdef _WIN32
            CreateDirectoryA(tmp, NULL);
#else
            mkdir(tmp, 0755);
#endif
            *p = c;
            if(c == '\0') break;
        }
    }
}

int sdk_app_index(const app_desc_t * app)
{
    for(int i = 0; i < os_app_count; i++) if(os_apps[i] == app) return i;
    return -1;
}

void sdk_open_app(const app_desc_t * app)
{
    int id = sdk_app_index(app);
    if(id >= 0) shell_open_app(id);
}
