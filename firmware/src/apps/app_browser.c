/**
 * @file app_browser.c
 * Lite Browser: a tiny text browser that only works on the PC build (it needs WinHTTP / curl; the ESP32 build shows
 * a polite refusal). No CSS and no layout: headings, paragraphs, lists, numbered links and pictures, in the
 * spirit of Lynx. JavaScript runs once at load (QuickJS + a small DOM stand-in) and images (PNG / JPEG) are fetched. Type a URL or a search; links appear inline as [n] and again in a list at the bottom.
 *
 *   about:home is a built-in page; anything without a dot is searched on DuckDuckGo Lite.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../os/sdk.h"

void lv_image_cache_drop(const void * src);

#define MAX_LINKS   48
#define MAX_BLOCKS  260
#define HIST_N      16
#define URL_LEN     240

static struct {
    lv_obj_t * root, * ta, * kb, * page, * status, * title, * spin;
    char url[URL_LEN];
    char hist[HIST_N][URL_LEN];
    int hist_n;
    char link_url[MAX_LINKS][URL_LEN];
    char link_txt[MAX_LINKS][44];
    int link_n;
    uint32_t t0;
    int blocks;
    bool js_on, img_on;
    lv_obj_t * js_btn, * img_btn, * rich_btn;
    bool rich;
} b;

static const char * const k_loading[] = {
    "Asking the tubes nicely...", "Reticulating splines...", "Teaching pigeons TCP...",
    "Warming up the packets...", "Convincing the router...", "Downloading the internet (just this page)...",
};

static const char * const k_oops[] = {
    "The pigeons got lost.", "A packet tripped over a cable.", "The server is on a coffee break.",
    "404 braincells not found.", "Offline? Touch grass, then retry.",
};

static void go(const char * url, bool push);
static void images_start(void);
static void images_free(void);

/*---------------- URL helpers ----------------*/

static void resolve(const char * base, const char * href, char * out, size_t n)
{
    if(!strncmp(href, "http://", 7) || !strncmp(href, "https://", 8) || !strncmp(href, "about:", 6)) {
        lv_strncpy(out, href, n - 1);
    }
    else if(!strncmp(href, "//", 2)) {
        lv_snprintf(out, n, "https:%s", href);
    }
    else {
        const char * host = strstr(base, "://");
        host = host ? host + 3 : base;
        const char * path = strchr(host, '/');
        size_t origin = path ? (size_t)(path - base) : strlen(base);
        if(href[0] == '/') {
            lv_snprintf(out, n, "%.*s%s", (int)origin, base, href);
        }
        else {
            const char * slash = strrchr(host, '/');
            size_t dir = slash ? (size_t)(slash - base) + 1 : strlen(base);
            lv_snprintf(out, n, "%.*s%s%s", (int)dir, base, slash ? "" : "/", href);
        }
    }
    out[n - 1] = '\0';
}

static bool bad_href(const char * h)
{
    return !h[0] || h[0] == '#' || !strncmp(h, "javascript:", 11) || !strncmp(h, "mailto:", 7);
}

/*---------------- page building ----------------*/

typedef enum { K_P, K_H1, K_H2, K_LI, K_PRE } kind_t;

static void add_block(kind_t k, const char * text)
{
    if(b.blocks >= MAX_BLOCKS) return;
    b.blocks++;
    const lv_font_t * f = k == K_H1 ? &lv_font_montserrat_24 : k == K_H2 ? &lv_font_montserrat_18 :
                          &lv_font_montserrat_14;
    uint32_t col = k == K_H1 ? 0xFFFFFF : k == K_H2 ? 0xFFD60A : k == K_PRE ? 0x30D158 : 0xE5E5EA;
    lv_obj_t * l = ui_label(b.page, text, f, col);
    lv_obj_set_width(l, LV_PCT(100));
    lv_label_set_long_mode(l, LV_LABEL_LONG_MODE_WRAP);
    if(k == K_LI) lv_obj_set_style_pad_left(l, 12, 0);
    if(k == K_H1 || k == K_H2) lv_obj_set_style_pad_top(l, 6, 0);
}

static void link_cb(lv_event_t * e)
{
    int i = (int)(intptr_t)lv_event_get_user_data(e);
    char u[URL_LEN];
    lv_strncpy(u, b.link_url[i], sizeof(u) - 1);
    u[sizeof(u) - 1] = '\0';
    go(u, true);
}

static void links_section(void)
{
    if(!b.link_n) return;
    lv_obj_t * h = ui_label(b.page, "LINKS", &lv_font_montserrat_12, UI_DIM);
    lv_obj_set_style_pad_top(h, 10, 0);
    for(int i = 0; i < b.link_n; i++) {
        lv_obj_t * r = ui_box(b.page);
        lv_obj_set_size(r, LV_PCT(100), 36);
        lv_obj_set_style_radius(r, 10, 0);
        lv_obj_set_style_bg_opa(r, LV_OPA_COVER, 0);
        lv_obj_set_style_bg_color(r, lv_color_hex(UI_GROUP), 0);
        lv_obj_set_style_bg_color(r, lv_color_hex(0x2C2C2E), LV_STATE_PRESSED);
        lv_obj_set_clickable(r, true);
        lv_obj_add_event_cb(r, link_cb, LV_EVENT_CLICKED, (void *)(intptr_t)i);
        lv_obj_t * l = ui_label(r, "", &lv_font_montserrat_14, UI_BLUE);
        lv_label_set_text_fmt(l, "[%d]  %s", i + 1, b.link_txt[i][0] ? b.link_txt[i] : b.link_url[i]);
        lv_label_set_long_mode(l, LV_LABEL_LONG_MODE_DOTS);
        lv_obj_set_width(l, LV_PCT(94));
        lv_obj_align(l, LV_ALIGN_LEFT_MID, 10, 0);
    }
}

static int entity(const char * s, char * out)
{
    static const struct { const char * e; char c; } t[] = {
        { "&amp;", '&' }, { "&lt;", '<' }, { "&gt;", '>' }, { "&quot;", '"' }, { "&#39;", '\'' }, { "&apos;", '\'' },
        { "&nbsp;", ' ' }, { "&copy;", 'c' }, { "&mdash;", '-' }, { "&ndash;", '-' }, { "&hellip;", '.' },
    };
    for(unsigned i = 0; i < sizeof(t) / sizeof(t[0]); i++) {
        size_t n = strlen(t[i].e);
        if(!strncmp(s, t[i].e, n)) {
            *out = t[i].c;
            return (int)n;
        }
    }
    if(s[1] == '#') {
        const char * q = strchr(s, ';');
        if(q && q - s < 9) {
            *out = '?';
            return (int)(q - s) + 1;
        }
    }
    return 0;
}

static bool ci_prefix(const char * s, const char * pre)
{
    for(; *pre; s++, pre++) if(!*s || (*s | 0x20) != *pre) return false;
    return true;
}

static bool script_start(const char * s)
{
    return ci_prefix(s, "<script") && (s[7] == '>' || s[7] == '/' || s[7] == ' ' ||
                                      s[7] == '\n' || s[7] == '\t');
}

static bool tag_is(const char * tag, const char * name)
{
    size_t n = strlen(name);
    if(tag[0] == '/') tag++;
    for(size_t i = 0; i < n; i++) if((tag[i] | 0x20) != name[i]) return false;
    char c = tag[n];
    return c == '\0' || c == ' ' || c == '/' || c == '\n' || c == '\t' || c == '>';
}

static bool attr_get_simple(const char * tag, const char * name, char * out, size_t n)
{
    char pat[24];
    lv_snprintf(pat, sizeof(pat), "%s=", name);
    const char * p = tag;
    while((p = strstr(p, pat)) != NULL) {
        if(p == tag || p[-1] == ' ' || p[-1] == '\n' || p[-1] == '\t') break;
        p += strlen(pat);
    }
    if(!p) return false;
    p += strlen(pat);
    char q = (*p == '"' || *p == '\'') ? *p++ : ' ';
    size_t k = 0;
    while(p[k] && p[k] != q && p[k] != '>' && k < n - 1) k++;
    memcpy(out, p, k);
    out[k] = '\0';
    return true;
}

/*---------------- Images ----------------
 * <img> becomes a placeholder in the flow; after the page is on screen the pictures are fetched one by one (max 30),
 * decoded from memory (PNG / JPEG) and swapped in. Unsupported formats (SVG, WebP, GIF) keep their alt text. */

#define IMG_MAX 30
static struct {
    char url[URL_LEN];
    char alt[48];
    lv_obj_t * ph;
    lv_draw_buf_t * buf;
} s_img[IMG_MAX];
static int s_img_n, s_img_next;
static unsigned s_gen;

static void images_free(void)
{
    for(int i = 0; i < s_img_n; i++) {
        if(s_img[i].buf) {
            lv_image_cache_drop(s_img[i].buf);
            lv_draw_buf_destroy(s_img[i].buf);
        }
    }
    lv_memzero(s_img, sizeof(s_img));
    s_img_n = s_img_next = 0;
    s_gen++;
}

static void add_image(const char * base, const char * src, const char * alt)
{
    if(b.blocks >= MAX_BLOCKS) return;
    b.blocks++;
    lv_obj_t * ph = ui_box(b.page);
    lv_obj_set_size(ph, LV_PCT(100), 54);
    lv_obj_set_style_radius(ph, 10, 0);
    lv_obj_set_style_bg_opa(ph, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(ph, lv_color_hex(UI_GROUP), 0);
    lv_obj_t * l = ui_label(ph, alt[0] ? alt : "image", &lv_font_montserrat_12, UI_DIM);
    lv_obj_set_width(l, LV_PCT(90));
    lv_label_set_long_mode(l, LV_LABEL_LONG_MODE_DOTS);
    lv_obj_center(l);
    if(s_img_n >= IMG_MAX || !b.img_on || !strncmp(src, "data:", 5)) return;
    resolve(base, src, s_img[s_img_n].url, URL_LEN);
    lv_strncpy(s_img[s_img_n].alt, alt, sizeof(s_img[0].alt) - 1);
    s_img[s_img_n].ph = ph;
    s_img_n++;
}

static void images_next(void);

static void img_cb(bool ok, const uint8_t * data, size_t len, void * ud)
{
    unsigned tag = (unsigned)(uintptr_t)ud;
    int i = (int)(tag & 0xFF);
    if((tag >> 8) != (s_gen & 0xFFFFFF) || i >= s_img_n) return;      /* the page changed meanwhile */
    lv_draw_buf_t * buf = ok ? sdk_image_decode_mem(data, len, 296, 420) : NULL;
    if(buf) {
        s_img[i].buf = buf;
        lv_obj_clean(s_img[i].ph);
#ifdef S3_NATIVE_LAYOUT
        lv_obj_set_size(s_img[i].ph, (int32_t)buf->header.w * 3, (int32_t)buf->header.h * 3);
#else
        lv_obj_set_size(s_img[i].ph, (int32_t)buf->header.w, (int32_t)buf->header.h);
#endif
        lv_obj_set_style_bg_opa(s_img[i].ph, LV_OPA_TRANSP, 0);
        lv_obj_set_style_radius(s_img[i].ph, 10, 0);
        lv_obj_set_style_clip_corner(s_img[i].ph, true, 0);
        lv_image_set_src(lv_image_create(s_img[i].ph), buf);
    }
    else if(s_img[i].ph && lv_obj_get_child_count(s_img[i].ph)) {
        lv_label_set_text_fmt(lv_obj_get_child(s_img[i].ph, 0), "%s (%s)", s_img[i].alt[0] ? s_img[i].alt : "image",
                              ok ? "unsupported format, e.g. WebP" : "download failed");
    }
    images_next();
}

static void images_next(void)
{
    if(s_img_next >= s_img_n) return;
    int i = s_img_next++;
    sdk_http_get_data(s_img[i].url, img_cb, (void *)(uintptr_t)(((s_gen & 0xFFFFFF) << 8) | (unsigned)i), b.page);
}

static void images_start(void)
{
    images_next();
}

static void render_html(const char * html, const char * base)
{
    static char text[1400];
    int tn = 0;
    kind_t kind = K_P;
    bool in_a = false, in_title = false, in_pre = false;
    int cur_link = -1;
    char title[64] = "";
    int ttn = 0;
    int ltn = 0;
    char ltxt[44] = "";

#define FLUSH()                                                        \
    do {                                                               \
        while(tn > 0 && text[tn - 1] == ' ') tn--;                     \
        text[tn] = '\0';                                               \
        const char * st = text;                                        \
        while(*st == ' ') st++;                                        \
        if(*st) add_block(in_pre ? K_PRE : kind, st);                  \
        tn = 0;                                                        \
    } while(0)

    for(const char * p = html; *p;) {
        if(*p == '<') {
            if(!strncmp(p, "<!--", 4)) {
                const char * e = strstr(p, "-->");
                p = e ? e + 3 : p + strlen(p);
                continue;
            }
            const char * end = p + 1;
            while(*end && *end != '>') end++;
            char tag[400];
            size_t tl = (size_t)(end - p - 1);
            if(tl >= sizeof(tag)) tl = sizeof(tag) - 1;
            memcpy(tag, p + 1, tl);
            tag[tl] = '\0';
            p = *end ? end + 1 : end;

            if(tag_is(tag, "script") || tag_is(tag, "style")) {
                if(tag[0] != '/') {
                    const char * close = tag_is(tag, "script") ? "</script" : "</style";
                    const char * e = p;
                    while(*e && !ci_prefix(e, close)) e++;
                    p = e;
                }
            }
            else if(tag_is(tag, "title")) in_title = tag[0] != '/';
            else if(tag_is(tag, "a")) {
                if(tag[0] != '/') {
                    const char * hr = strstr(tag, "href=");
                    char href[URL_LEN] = "";
                    if(hr) {
                        hr += 5;
                        char q = (*hr == '"' || *hr == '\'') ? *hr++ : ' ';
                        size_t n = 0;
                        while(hr[n] && hr[n] != q && hr[n] != '>' && n < sizeof(href) - 1) n++;
                        memcpy(href, hr, n);
                        href[n] = '\0';
                    }
                    if(!bad_href(href) && b.link_n < MAX_LINKS) {
                        cur_link = b.link_n++;
                        resolve(base, href, b.link_url[cur_link], URL_LEN);
                        b.link_txt[cur_link][0] = '\0';
                        ltn = 0;
                        in_a = true;
                    }
                }
                else if(in_a && cur_link >= 0) {
                    int n = tn + 6 < (int)sizeof(text) ? tn : (int)sizeof(text) - 7;
                    n += lv_snprintf(text + n, 8, "[%d]", cur_link + 1);
                    tn = n;
                    ltxt[ltn] = '\0';
                    lv_strncpy(b.link_txt[cur_link], ltxt, 43);
                    in_a = false;
                    cur_link = -1;
                    ltn = 0;
                }
            }
            else if(tag_is(tag, "img")) {
                FLUSH();
                char src[URL_LEN] = "", alt[48] = "";
                if(!attr_get_simple(tag, "src", src, sizeof(src)) || !strncmp(src, "data:", 5)) {
                    if(!attr_get_simple(tag, "data-src", src, sizeof(src))) {
                        char set[URL_LEN];
                        if(attr_get_simple(tag, "srcset", set, sizeof(set))) {         /* first candidate of the srcset */
                            size_t k = 0;
                            while(set[k] && set[k] != ' ' && set[k] != ',' && k < sizeof(src) - 1) { src[k] = set[k]; k++; }
                            src[k] = '\0';
                        }
                    }
                }
                attr_get_simple(tag, "alt", alt, sizeof(alt));
                char dim[12];
                int iw = 0, ih = 0;
                if(attr_get_simple(tag, "width", dim, sizeof(dim))) iw = atoi(dim);
                if(attr_get_simple(tag, "height", dim, sizeof(dim))) ih = atoi(dim);
                bool tiny = (iw > 0 && iw < 40) || (ih > 0 && ih < 40);       /* icons and spacers are not worth a placeholder */
                if(src[0] && !tiny) add_image(base, src, alt);
                else if(!src[0] && alt[0]) add_block(K_P, alt);
            }
            else if(tag_is(tag, "h1") || tag_is(tag, "h2") || tag_is(tag, "h3")) {
                FLUSH();
                kind = tag[0] == '/' ? K_P : (tag_is(tag, "h1") ? K_H1 : K_H2);
            }
            else if(tag_is(tag, "pre")) {
                FLUSH();
                in_pre = tag[0] != '/';
            }
            else if(tag_is(tag, "li")) {
                FLUSH();
                if(tag[0] != '/') {
                    kind = K_LI;
                    text[tn++] = '*';
                    text[tn++] = ' ';
                }
                else kind = K_P;
            }
            else if(tag_is(tag, "p") || tag_is(tag, "div") || tag_is(tag, "br") || tag_is(tag, "tr") ||
                    tag_is(tag, "ul") || tag_is(tag, "ol") || tag_is(tag, "table") || tag_is(tag, "hr") ||
                    tag_is(tag, "blockquote") || tag_is(tag, "article") || tag_is(tag, "section")) {
                FLUSH();
            }
            continue;
        }
        char c = *p;
        int skip = 1;
        if(c == '&') {
            char d;
            int n = entity(p, &d);
            if(n) {
                c = d;
                skip = n;
            }
        }
        p += skip;
        if(in_title) {
            if(ttn < 62) title[ttn++] = (c == '\n' ? ' ' : c);
            continue;
        }
        if(c == '\r') continue;
        if(!in_pre && (c == '\n' || c == '\t')) c = ' ';
        if(!in_pre && c == ' ' && (tn == 0 || text[tn - 1] == ' ')) continue;
        if(in_pre && c == '\n') {
            if(tn < (int)sizeof(text) - 2) text[tn++] = '\n';
            continue;
        }
        if(tn < (int)sizeof(text) - 10) text[tn++] = c;
        if(in_a && ltn < 42) ltxt[ltn++] = c;
        if(tn > 1100 && c == ' ') FLUSH();              /* never build absurdly long labels */
    }
    FLUSH();
#undef FLUSH
    title[ttn] = '\0';
    lv_label_set_text(b.title, title[0] ? title : b.url);
    links_section();
}

static const char k_home[] =
    "<h1>Lite Browser</h1><p>Text-first browsing with QuickJS. Complex website layouts are still simplified.</p>"
    "<pre>       __\n      / _)     rawr.\n .-^^^-/ /\n__/       /\n&lt;__.|_|-|_|</pre>"
    "<h2>Try these</h2><ul>"
    "<li><a href=\"https://example.com\">example.com</a> - the classic</li>"
    "<li><a href=\"http://info.cern.ch\">info.cern.ch</a> - the first website ever</li>"
    "<li><a href=\"https://text.npr.org\">text.npr.org</a> - news with zero bloat</li>"
    "<li><a href=\"https://lite.duckduckgo.com/lite/?q=esp32+s3\">Search: esp32 s3</a></li>"
    "<li><a href=\"https://en.wikipedia.org/wiki/Special:Random\">A random Wikipedia page</a></li>"
    "<li><a href=\"about:js\">JavaScript demo</a></li>"
    "<li><a href=\"about:joke\">Tell me a joke</a></li></ul>"
    "<p>Type a web address, or any words to search. This page is served by pigeons.</p>";

static const char k_js_demo[] =
    "<h1>JavaScript demo</h1><p id=\"result\">Waiting for JavaScript...</p>"
    "<script>const numbers=[1,2,3].map(x=>x*2);"
    "document.getElementById('result').textContent='QuickJS ran: '+numbers.join(', ');"
    "document.write('<p>Modern arrow functions and const work.</p>');</script>";

static const char * const k_jokes[] = {
    "<h1>Joke</h1><p>Why do programmers prefer dark mode?</p><p>Because light attracts bugs.</p><p><a href=\"about:joke\">Another one</a></p>",
    "<h1>Joke</h1><p>There are 10 kinds of people: those who understand binary and those who don't.</p<p><a href=\"about:joke\">Another one</a></p>",
    "<h1>Joke</h1><p>A SQL query walks into a bar, walks up to two tables and asks: can I join you?</p<p><a href=\"about:joke\">Another one</a></p>",
    "<h1>Joke</h1><p>ESP32 walks into a bar. Bartender: what'll it be? ESP32: whatever's on Wi-Fi.</p<p><a href=\"about:joke\">Another one</a></p>",
};

/*---------------- navigation ----------------*/

static void status_set(const char * t)
{
    lv_label_set_text(b.status, t);
}

static void clear_page(void)
{
    images_free();
    lv_obj_clean(b.page);
    lv_obj_scroll_to_y(b.page, 0, LV_ANIM_OFF);
    b.link_n = 0;
    b.blocks = 0;
}

/*---------------- JavaScript (QuickJS) ----------------
 * Scripts run once at load on a background thread: external <script src> files are fetched, every script is run in
 * order against a small DOM stand-in (document.write / getElementById(...).innerHTML = ... / textContent / title /
 * createElement + appendChild / DOMContentLoaded / setTimeout), and the result is spliced back into the page text
 * before it is rendered. No layout, events or network calls from scripts (fetch / XHR are inert).
 */
#if !defined(__EMSCRIPTEN__)
#include "../third_party/quickjs/browser_js.h"
#if !defined(ESP_PLATFORM) && !defined(__EMSCRIPTEN__)
#include "browser_rich.h"
#endif

typedef struct {
    char * p;
    size_t n, cap;
    bool failed;
} sbuf_t;

typedef struct {
    char * html;                    /* page source (owned) */
    char base[URL_LEN];
    sbuf_t out;                     /* the expanded page */
    char log[700];
    int scripts, errors;
    char title[64];
    bool ok;
} jsjob_t;

static void sb_add(sbuf_t * s, const char * t, size_t n)
{
    if(s->failed || n > SIZE_MAX - s->n - 1) { s->failed = true; return; }
    if(s->n + n + 1 > s->cap) {
        size_t nc = s->cap ? s->cap * 2 : 8192;
        while(nc < s->n + n + 1) nc *= 2;
        char * np = realloc(s->p, nc);
        if(!np) { s->failed = true; return; }
        s->p = np;
        s->cap = nc;
    }
    memcpy(s->p + s->n, t, n);
    s->n += n;
    s->p[s->n] = '\0';
}

/* Inner HTML range of the element with id="..." (nesting of the same tag name is counted) */
static bool find_inner(const char * html, const char * id, size_t * start, size_t * end)
{
    char pat1[80], pat2[80];
    lv_snprintf(pat1, sizeof(pat1), "id=\"%s\"", id);
    lv_snprintf(pat2, sizeof(pat2), "id='%s'", id);
    const char * at = strstr(html, pat1);
    if(!at) at = strstr(html, pat2);
    if(!at) return false;
    const char * lt = at;
    while(lt > html && *lt != '<') lt--;
    if(*lt != '<') return false;
    char name[16];
    size_t nl = 0;
    for(const char * c = lt + 1; *c && nl < 15 && ((*c >= 'a' && *c <= 'z') || (*c >= 'A' && *c <= 'Z') || (*c >= '0' && *c <= '9')); c++)
        name[nl++] = (char)(*c | 0x20);
    name[nl] = '\0';
    const char * gt = strchr(at, '>');
    if(!gt || !nl) return false;
    *start = (size_t)(gt + 1 - html);
    int depth = 1;
    for(const char * c = gt + 1; *c; c++) {
        if(*c != '<') continue;
        bool close = c[1] == '/';
        const char * nm = c + (close ? 2 : 1);
        size_t k = 0;
        while(k < nl && (nm[k] | 0x20) == name[k]) k++;
        if(k == nl && !((nm[k] >= 'a' && nm[k] <= 'z') || (nm[k] >= 'A' && nm[k] <= 'Z') || (nm[k] >= '0' && nm[k] <= '9'))) {
            if(close) {
                if(--depth == 0) {
                    *end = (size_t)(c - html);
                    return true;
                }
            }
            else depth++;
        }
    }
    return false;
}

static void js_write(js_State * J)
{
    jsjob_t * j = js_getcontext(J);
    const char * t = js_tostring(J, 1);
    sb_add(&j->out, t, strlen(t));
    js_pushundefined(J);
}

static void js_logf(js_State * J)
{
    jsjob_t * j = js_getcontext(J);
    const char * t = js_tostring(J, 1);
    size_t n = strlen(j->log);
    if(n < sizeof(j->log) - 80) lv_snprintf(j->log + n, sizeof(j->log) - n, "%s\n", t);
    js_pushundefined(J);
}

static void js_orig(js_State * J)
{
    jsjob_t * j = js_getcontext(J);
    const char * id = js_tostring(J, 1);
    size_t s, e;
    if(find_inner(j->html, id, &s, &e) && e - s < 3000) {
        char tmp[3004];
        memcpy(tmp, j->html + s, e - s);
        tmp[e - s] = '\0';
        js_pushstring(J, tmp);
    }
    else js_pushstring(J, "");
}

static void js_report_cb(js_State * J, const char * message)
{
    jsjob_t * j = js_getcontext(J);
    j->errors++;
    size_t n = strlen(j->log);
    if(n < sizeof(j->log) - 120) lv_snprintf(j->log + n, sizeof(j->log) - n, "JS error: %.100s\n", message);
}

static const char k_boot[] =
    "var __els={},__ready=[],__timers=[];"
    "function __esc(s){return String(s).replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;');}"
    "function __strip(h){return String(h).replace(/<[^>]*>/g,'');}"
    "function __el(id){var e=__els[id];if(e)return e;"
    " e={id:id,tagName:'DIV',style:{},className:'',children:[],value:'',checked:false,disabled:false,innerHTML:__orig(id),"
    " classList:{add:function(){},remove:function(){},toggle:function(){},contains:function(){return false;}},"
    " setAttribute:function(){},getAttribute:function(){return null;},addEventListener:function(){},"
    " insertAdjacentHTML:function(where,h){this.innerHTML+=String(h);__write(String(h));},"
    " removeEventListener:function(){},focus:function(){},click:function(){},"
    " appendChild:function(c){this.innerHTML=String(this.innerHTML)+((c&&c.outerHTML)||('<span>'+((c&&(c.innerHTML||c.textContent))||'')+'</span>'));return c;},"
    " insertBefore:function(c){return this.appendChild(c);},removeChild:function(c){return c;},"
    " querySelector:function(){return __el('__x');},querySelectorAll:function(){return [];}};"
    " e._oi=e.innerHTML;e._ot=e.textContent=e.innerText=__strip(e.innerHTML);__els[id]=e;return e;}"
    "function __mk(t){return {tagName:String(t).toUpperCase(),style:{},children:[],innerHTML:'',textContent:'',className:'',"
    " setAttribute:function(){},addEventListener:function(){},"
    " insertAdjacentHTML:function(where,h){this.innerHTML+=String(h);__write(String(h));},"
    " appendChild:function(c){this.children.push(c);this.innerHTML+=(c.outerHTML||('<span>'+(c.textContent||c.innerHTML||'')+'</span>'));return c;}};}"
    "var document={title:'',cookie:'',readyState:'loading',referrer:'',URL:__URL,"
    " write:function(){for(var i=0;i<arguments.length;i++)__write(String(arguments[i]));},"
    " writeln:function(){for(var i=0;i<arguments.length;i++)__write(String(arguments[i]));__write('\\n');},"
    " getElementById:function(id){return __el(id);},"
    " querySelector:function(q){q=String(q);if(q.charAt(0)=='#')return __el(q.substring(1));return __mk('div');},"
    " querySelectorAll:function(){return [];},getElementsByTagName:function(){return [];},getElementsByClassName:function(){return [];},"
    " createElement:function(t){return __mk(t);},createTextNode:function(t){return {textContent:String(t),outerHTML:__esc(t)};},"
    " addEventListener:function(t,f){if(t=='DOMContentLoaded'||t=='load')__ready.push(f);},removeEventListener:function(){},"
    " head:{appendChild:function(c){return c;}},documentElement:{style:{}}};"
    "document.body={style:{},innerHTML:'',children:[{lang:'',insertAdjacentHTML:function(where,h){__write(String(h));}}],"
    " querySelectorAll:function(q){return q=='p'?this.children:[];},prepend:function(){},"
    " insertAdjacentHTML:function(where,h){__write(String(h));},addEventListener:function(){},"
    " appendChild:function(c){__write(c.outerHTML||('<div>'+(c.innerHTML||c.textContent||'')+'</div>'));return c;}};"
    "var window=this;window.document=document;"
    "var navigator={userAgent:'S3-Springboard LiteBrowser/1.0',language:'en-US',languages:['en-US'],platform:'ESP32-S3',onLine:true};"
    "var location={href:__URL,protocol:'https:',host:'',hostname:'',pathname:'/',search:'',hash:'',reload:function(){},assign:function(){}};"
    "var console={log:function(){var a=[];for(var i=0;i<arguments.length;i++)a.push(String(arguments[i]));__log(a.join(' '));}};"
    "console.warn=console.error=console.info=console.log;"
    "function alert(s){__log('alert: '+s);}function confirm(){return true;}function prompt(){return null;}"
    "function setTimeout(f,d){__timers.push({f:f,d:d||0,i:__timers.length});return __timers.length;}"
    "function setInterval(f,d){return setTimeout(f,d);}function clearTimeout(){}function clearInterval(){}"
    "function requestAnimationFrame(f){return setTimeout(f,16);}"
    "var localStorage={getItem:function(){return null;},setItem:function(){},removeItem:function(){}};var sessionStorage=localStorage;"
    "function XMLHttpRequest(){this.open=function(){};this.send=function(){};this.setRequestHeader=function(){};this.readyState=0;}"
    "function fetch(){return {then:function(){return this;},'catch':function(){return this;}};}"
    "function __finish(){var i;document.readyState='complete';"
    " for(i=0;i<__ready.length;i++){try{__ready[i]();}catch(e){__log('error: '+e);}}"
    " if(typeof window.onload=='function'){try{window.onload();}catch(e){__log('error: '+e);}}"
    " __timers.sort(function(a,b){return a.d-b.d||a.i-b.i;});"
    " for(i=0;i<__timers.length&&i<60;i++){try{if(typeof __timers[i].f=='function')__timers[i].f();else eval(String(__timers[i].f));}catch(e){__log('error: '+e);}}}"
    "function __changes(){var out=[];for(var id in __els){var e=__els[id];var h=null;"
    " if(e.innerHTML!==e._oi)h=String(e.innerHTML);else if(e.textContent!==e._ot)h=__esc(e.textContent);"
    " else if(e.innerText!==e._ot)h=__esc(e.innerText);if(h!==null){out.push(id);out.push(h);}}return out;}";

static bool attr_get(const char * tag, const char * name, char * out, size_t n)
{
    const char * p = tag;
    size_t want = strlen(name);
    while(*p && *p != '>') {
        while(*p == ' ' || *p == '\n' || *p == '\t') p++;
        const char * key = p;
        while((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') || *p == '-') p++;
        size_t key_len = (size_t)(p - key);
        while(*p == ' ' || *p == '\n' || *p == '\t') p++;
        if(key_len == want && ci_prefix(key, name) && *p == '=') {
            p++;
            while(*p == ' ' || *p == '\n' || *p == '\t') p++;
            char q = (*p == '"' || *p == '\'') ? *p++ : ' ';
            size_t k = 0;
            while(p[k] && p[k] != q && p[k] != '>' && k < n - 1) k++;
            memcpy(out, p, k);
            out[k] = '\0';
            return true;
        }
        if(*p == '=') {
            p++;
            while(*p == ' ' || *p == '\n' || *p == '\t') p++;
            if(*p == '"' || *p == '\'') {
                char q = *p++;
                while(*p && *p != q) p++;
                if(*p) p++;
            }
            else while(*p && *p != ' ' && *p != '\n' && *p != '\t' && *p != '>') p++;
        }
        else if(!key_len && *p) p++;
    }
    return false;
}

static void js_work(void * ud)
{
    jsjob_t * j = ud;
    js_State * J = js_newstate(NULL, NULL, 0);
    if(!J) return;
    js_setcontext(J, j);
    js_setreport(J, js_report_cb);
    js_newcfunction(J, js_write, "__write", 1);
    js_setglobal(J, "__write");
    js_newcfunction(J, js_logf, "__log", 1);
    js_setglobal(J, "__log");
    js_newcfunction(J, js_orig, "__orig", 1);
    js_setglobal(J, "__orig");
    js_pushstring(J, j->base);
    js_setglobal(J, "__URL");
    js_dostring(J, k_boot);

    char * ext = malloc(400 * 1024);
    int fetched = 0;
    const char * html = j->html;
    size_t i = 0, len = strlen(html);
    while(i < len) {
        const char * lt = strchr(html + i, '<');
        if(!lt) {
            sb_add(&j->out, html + i, len - i);
            break;
        }
        sb_add(&j->out, html + i, (size_t)(lt - html) - i);
        i = (size_t)(lt - html);
        if(ci_prefix(lt, "<noscript")) {                /* JS is on: the fallback text is not wanted */
            const char * e = lt;
            while(*e && !ci_prefix(e, "</noscript")) e++;
            const char * g = *e ? strchr(e, '>') : NULL;
            i = g ? (size_t)(g + 1 - html) : len;
            continue;
        }
        if(!script_start(lt)) {
            sb_add(&j->out, "<", 1);
            i++;
            continue;
        }
        const char * gt = strchr(lt, '>');
        if(!gt) break;
        char tag[500];
        size_t tl = LV_MIN((size_t)(gt - lt), sizeof(tag) - 1);
        memcpy(tag, lt, tl);
        tag[tl] = '\0';
        const char * body = gt + 1;
        const char * end = body;
        while(*end && !ci_prefix(end, "</script")) end++;
        const char * eg = *end ? strchr(end, '>') : NULL;
        i = eg ? (size_t)(eg + 1 - html) : len;

        char type[64] = "";
        attr_get(tag, "type", type, sizeof(type));
        bool skip = type[0] && (strstr(type, "json") || strstr(type, "template") || strstr(type, "x-") ||
                                strstr(type, "module"));
        if(skip || j->scripts >= 14) continue;
        char src[URL_LEN];
        const char * code = NULL;
        char * inl = NULL;
        if(attr_get(tag, "src", src, sizeof(src)) && src[0]) {
            char full[URL_LEN];
            resolve(j->base, src, full, sizeof(full));
            if(ext && fetched < 6 && sdk_http_fetch_sync(full, ext, 400 * 1024)) {
                fetched++;
                code = ext;
            }
            else {
                j->errors++;
                continue;
            }
        }
        else {
            size_t n = (size_t)(end - body);
            inl = malloc(n + 1);
            if(!inl) continue;
            memcpy(inl, body, n);
            inl[n] = '\0';
            code = inl;
        }
        j->scripts++;
        js_dostring(J, code);
        free(inl);
    }
    free(ext);

    js_dostring(J, "__finish()");

    /* element edits: [id, html, id, html, ...] */
    js_getglobal(J, "__changes");
    js_pushundefined(J);
    if(js_pcall(J, 0) == 0 && js_isarray(J, -1)) {
        int n = js_getlength(J, -1);
        for(int c = 0; c + 1 < n; c += 2) {
            js_getindex(J, -1, c);
            char id[64];
            lv_strncpy(id, js_tostring(J, -1), sizeof(id) - 1);
            id[sizeof(id) - 1] = '\0';
            js_pop(J, 1);
            js_getindex(J, -1, c + 1);
            const char * nh = js_tostring(J, -1);
            size_t s, e;
            if(j->out.p && find_inner(j->out.p, id, &s, &e) && s <= e && e <= j->out.n) {
                sbuf_t r = { 0 };
                sb_add(&r, j->out.p, s);
                sb_add(&r, nh, strlen(nh));
                sb_add(&r, j->out.p + e, j->out.n - e);
                free(j->out.p);
                j->out = r;
            }
            js_pop(J, 1);
        }
    }
    js_pop(J, 1);
    js_getglobal(J, "document");
    js_getproperty(J, -1, "title");
    lv_strncpy(j->title, js_tostring(J, -1), sizeof(j->title) - 1);
    js_pop(J, 2);
    js_freestate(J);
    j->ok = true;
}

#ifndef ESP_PLATFORM
static void rich_start(const char * html, const char * base);
#endif

static void js_done(void * ud, bool ok)
{
    jsjob_t * j = ud;
    lv_obj_set_hidden(b.spin, true);
    clear_page();
    if(!ok) {                                           /* runaway script: show the page without JS, leak the job */
        render_html(j->html, j->base);
        status_set("script timed out - shown without JS");
        add_block(K_P, "(A script ran too long and was stopped.)");
        return;
    }
#ifndef ESP_PLATFORM
    if(b.rich) {
        rich_start(j->out.p ? j->out.p : j->html, j->base);
        free(j->html);
        free(j->out.p);
        free(j);
        return;
    }
#endif
    render_html(j->out.p ? j->out.p : j->html, j->base);
    if(!b.blocks) add_block(K_P, "This site needs browser layout features that Lite Browser does not support yet.");
    if(j->title[0]) lv_label_set_text(b.title, j->title);
    if(j->log[0]) add_block(K_PRE, j->log);
    char s[96];
    lv_snprintf(s, sizeof(s), "JS: %d scripts, %d errors | %d blocks, %d links, %ums", j->scripts, j->errors, b.blocks,
                b.link_n, (unsigned)lv_tick_elaps(b.t0));
    status_set(s);
    images_start();
    free(j->html);
    free(j->out.p);
    free(j);
}
#endif /* !ESP_PLATFORM */

#if !defined(ESP_PLATFORM) && !defined(__EMSCRIPTEN__)
/*---------------- Rich mode: litehtml lays the page out (CSS, tables, floats), pictures come from the same fetcher ----------------*/

typedef struct {
    char * html;
    char base[URL_LEN];
} rjob_t;

static void rich_link_cb(const char * url, void * ud)
{
    LV_UNUSED(ud);
    go(url, true);
}

static void rich_work(void * ud)
{
    rjob_t * r = ud;
    char * o = rich_inline_css(r->html, r->base);
    if(o) {
        free(r->html);
        r->html = o;
    }
}

static void rich_done(void * ud, bool ok)
{
    rjob_t * r = ud;
    lv_obj_set_hidden(b.spin, true);
    clear_page();
    lv_obj_update_layout(b.page);
    char title[64] = "";
    rich_set_images(b.img_on);
    lv_obj_t * o = rich_create(b.page, r->html, r->base, lv_obj_get_content_width(b.page), rich_link_cb, NULL, title, sizeof(title));
    if(o) {
        lv_label_set_text(b.title, title[0] ? title : r->base);
        char s[64];
        lv_snprintf(s, sizeof(s), "rich engine%s | %ums", ok ? "" : " (styles skipped)", (unsigned)lv_tick_elaps(b.t0));
        status_set(s);
    }
    else {
        render_html(r->html, r->base);
        status_set("rich engine failed - text mode");
    }
    free(r->html);
    free(r);
}

static void rich_start(const char * html, const char * base)
{
    rjob_t * r = calloc(1, sizeof(*r));
    if(!r) return;
    r->html = malloc(strlen(html) + 1);
    if(!r->html) {
        free(r);
        return;
    }
    strcpy(r->html, html);
    lv_strncpy(r->base, base, sizeof(r->base) - 1);
    lv_obj_set_hidden(b.spin, false);
    status_set("loading styles...");
    sdk_job_run(rich_work, rich_done, r, b.page, 12000);
}
#endif

static void on_reply(bool ok, const char * body, void * ud)
{
    LV_UNUSED(ud);
    lv_obj_set_hidden(b.spin, true);
    clear_page();
    if(!ok) {
        add_block(K_H1, "Oops");
        add_block(K_P, k_oops[lv_rand(0, 4)]);
        add_block(K_P, "Check your connection and try again. (HTTPS sites that insist on JavaScript will look empty.)");
        lv_label_set_text(b.title, "Could not load");
        status_set("failed");
        return;
    }
#if !defined(__EMSCRIPTEN__)
    bool has_script = false;
    for(const char * p = body; *p; p++) if(*p == '<' && script_start(p)) { has_script = true; break; }
    if(b.js_on && has_script) {                           /* run the page's scripts first */
        jsjob_t * j = calloc(1, sizeof(*j));
        if(j) {
            j->html = malloc(strlen(body) + 1);
            if(j->html) {
                strcpy(j->html, body);
                lv_strncpy(j->base, b.url, sizeof(j->base) - 1);
                lv_obj_set_hidden(b.spin, false);
                status_set("running scripts...");
                sdk_job_run(js_work, js_done, j, b.page, 8000);
                return;
            }
            free(j);
        }
    }
#endif
#if !defined(ESP_PLATFORM) && !defined(__EMSCRIPTEN__)
    if(b.rich) {
        rich_start(body, b.url);
        return;
    }
#endif
    render_html(body, b.url);
    char s[64];
    lv_snprintf(s, sizeof(s), "%d blocks, %d links, %ums", b.blocks, b.link_n, (unsigned)lv_tick_elaps(b.t0));
    status_set(s);
    images_start();
    if(!b.blocks) add_block(K_P, "(This page has no text a lite browser can read - it probably needs JavaScript.)");
}

static void go(const char * url, bool push)
{
    char u[URL_LEN];
    lv_strncpy(u, url, sizeof(u) - 1);
    u[sizeof(u) - 1] = '\0';
    if(push && b.url[0] && b.hist_n < HIST_N) lv_strncpy(b.hist[b.hist_n++], b.url, URL_LEN - 1);
    lv_strncpy(b.url, u, URL_LEN - 1);
    lv_textarea_set_text(b.ta, u);
    ui_keyboard_hide(b.kb);
    lv_obj_scroll_to_y(b.page, 0, LV_ANIM_OFF);

    if(!strncmp(u, "about:", 6)) {
        clear_page();
        lv_obj_set_hidden(b.spin, true);
        if(!strcmp(u, "about:js")) { b.t0 = lv_tick_get(); on_reply(true, k_js_demo, NULL); return; }
        render_html(!strcmp(u, "about:joke") ? k_jokes[lv_rand(0, 3)] : k_home, u);
        status_set("about page");
        return;
    }
#if !defined(_WIN32) && !defined(__EMSCRIPTEN__) && !defined(ESP_PLATFORM)
    clear_page();
    add_block(K_H1, "PC only");
    add_block(K_P, "Lite Browser needs the Windows build. On the ESP32 there is no network stack in the simulator shell.");
    return;
#else
    clear_page();
    add_block(K_P, k_loading[lv_rand(0, 5)]);
    lv_obj_set_hidden(b.spin, false);
    status_set("loading");
    b.t0 = lv_tick_get();
    sdk_http_get(u, on_reply, NULL, b.page);
#endif
}

#ifdef ESP_PLATFORM
void app_browser_navigate(const char * url)
{
    if(b.root) go(url, true);
}
static void browser_deleted(lv_event_t * e)
{
    (void)e; images_free(); b.root = NULL;
}
#endif

static void submit_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    const char * in = lv_textarea_get_text(b.ta);
    char u[URL_LEN];
    if(!in[0]) return;
    if(!strncmp(in, "http://", 7) || !strncmp(in, "https://", 8) || !strncmp(in, "about:", 6)) {
        lv_strncpy(u, in, sizeof(u) - 1);
    }
    else if(strchr(in, ' ') || !strchr(in, '.')) {      /* words: search */
        char q[160];
        size_t n = 0;
        for(const char * c = in; *c && n < sizeof(q) - 4; c++) {
            if(*c == ' ') q[n++] = '+';
            else if((*c >= '0' && *c <= '9') || (*c >= 'a' && *c <= 'z') || (*c >= 'A' && *c <= 'Z')) q[n++] = *c;
            else n += (size_t)lv_snprintf(q + n, 4, "%%%02X", (unsigned char)*c);
        }
        q[n] = '\0';
        lv_snprintf(u, sizeof(u), "https://lite.duckduckgo.com/lite/?q=%s", q);
    }
    else {
        lv_snprintf(u, sizeof(u), "https://%s", in);
    }
    u[sizeof(u) - 1] = '\0';
    go(u, true);
}

static void back_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    if(b.hist_n <= 0) return;
    char u[URL_LEN];
    lv_strncpy(u, b.hist[--b.hist_n], URL_LEN - 1);
    go(u, false);
}

static void home_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    go("about:home", true);
}

static void opt_refresh(void)
{
    lv_obj_set_style_bg_color(b.js_btn, lv_color_hex(b.js_on ? UI_BLUE : 0x3A3A3C), 0);
    lv_obj_set_style_bg_color(b.img_btn, lv_color_hex(b.img_on ? UI_BLUE : 0x3A3A3C), 0);
    lv_label_set_text(lv_obj_get_child(b.js_btn, 0), b.js_on ? "JS on" : "JS off");
    lv_label_set_text(lv_obj_get_child(b.img_btn, 0), b.img_on ? "Images on" : "Images off");
    lv_obj_set_style_bg_color(b.rich_btn, lv_color_hex(b.rich ? UI_BLUE : 0x3A3A3C), 0);
    lv_label_set_text(lv_obj_get_child(b.rich_btn, 0), b.rich ? "Rich layout" : "Text only");
}

static void js_toggle_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    b.js_on = !b.js_on;
    sdk_set_int("br.js", b.js_on);
    opt_refresh();
    if(b.url[0] && (strncmp(b.url, "about:", 6) || !strcmp(b.url, "about:js"))) go(b.url, false);
}

static void rich_toggle_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    b.rich = !b.rich;
    sdk_set_int("br.rich", b.rich);
    opt_refresh();
    if(b.url[0] && strncmp(b.url, "about:", 6)) go(b.url, false);
}

static void img_toggle_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    b.img_on = !b.img_on;
    sdk_set_int("br.img", b.img_on);
    opt_refresh();
    if(b.url[0] && strncmp(b.url, "about:", 6)) go(b.url, false);
}

static void focus_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    ui_keyboard_show(b.kb);
}

static lv_obj_t * chrome_btn(lv_obj_t * parent, const char * sym, lv_event_cb_t cb)
{
    lv_obj_t * x = ui_button(parent, sym, 0x2C2C2E, cb, NULL);
    lv_obj_set_size(x, 34, 34);
    lv_obj_set_style_radius(x, 17, 0);
    lv_obj_set_style_pad_all(x, 0, 0);
    return x;
}

APP(app_browser, "Browser", LV_SYMBOL_GPS, 0x0A84FF)
{
    lv_memzero(&b, sizeof(b));
    b.root = root;
#ifdef ESP_PLATFORM
    lv_obj_add_event_cb(root, browser_deleted, LV_EVENT_DELETE, NULL);
#endif
    lv_obj_set_style_pad_top(root, 38, 0);
    lv_obj_set_style_pad_hor(root, 12, 0);
    lv_obj_set_style_pad_row(root, 8, 0);
    lv_obj_set_flex_flow(root, LV_FLEX_FLOW_COLUMN);

    b.title = ui_label(root, "Lite Browser", &lv_font_montserrat_14, UI_DIM);
    lv_obj_set_width(b.title, LV_PCT(100));
    lv_label_set_long_mode(b.title, LV_LABEL_LONG_MODE_DOTS);

    lv_obj_t * bar = ui_box(root);
    lv_obj_set_size(bar, LV_PCT(100), 36);
    lv_obj_set_flex_flow(bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(bar, 6, 0);
    lv_obj_set_flex_align(bar, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    chrome_btn(bar, LV_SYMBOL_LEFT, back_cb);
    chrome_btn(bar, LV_SYMBOL_HOME, home_cb);
    b.ta = lv_textarea_create(bar);
    lv_textarea_set_one_line(b.ta, true);
    lv_textarea_set_placeholder_text(b.ta, "Search or type a web address");
    lv_obj_set_flex_grow(b.ta, 1);
    lv_obj_set_height(b.ta, 34);
    lv_obj_set_style_radius(b.ta, 17, 0);
    lv_obj_set_style_bg_color(b.ta, lv_color_hex(0x2C2C2E), 0);
    lv_obj_set_style_text_color(b.ta, lv_color_white(), 0);
    lv_obj_set_style_border_width(b.ta, 0, 0);
    lv_obj_set_style_text_font(b.ta, &lv_font_montserrat_14, 0);
    lv_obj_add_event_cb(b.ta, submit_cb, LV_EVENT_READY, NULL);
    lv_obj_add_event_cb(b.ta, focus_cb, LV_EVENT_FOCUSED, NULL);
    lv_obj_add_event_cb(b.ta, focus_cb, LV_EVENT_CLICKED, NULL);
    chrome_btn(bar, LV_SYMBOL_PLAY, submit_cb);

    lv_obj_t * opts = ui_box(root);
    lv_obj_set_size(opts, LV_PCT(100), 28);
    lv_obj_set_flex_flow(opts, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(opts, 6, 0);
    b.js_on = sdk_get_int("br.js", 1) != 0;
    b.img_on = sdk_get_int("br.img", 1) != 0;
    b.js_btn = ui_button(opts, "JS", 0x3A3A3C, js_toggle_cb, NULL);
    b.img_btn = ui_button(opts, "Images", 0x3A3A3C, img_toggle_cb, NULL);
    b.rich_btn = ui_button(opts, "Rich", 0x3A3A3C, rich_toggle_cb, NULL);
    b.rich = sdk_get_int("br.rich", 1) != 0;
#ifdef ESP_PLATFORM
    b.rich = false;
    lv_obj_add_state(b.rich_btn, LV_STATE_DISABLED);
#endif
    lv_obj_t * ob[3] = { b.js_btn, b.img_btn, b.rich_btn };
    for(int i = 0; i < 3; i++) {
        lv_obj_set_height(ob[i], 26);
        lv_obj_set_style_radius(ob[i], 13, 0);
        lv_obj_set_style_pad_hor(ob[i], 12, 0);
        lv_obj_set_style_text_font(lv_obj_get_child(ob[i], 0), &lv_font_montserrat_12, 0);
    }
    opt_refresh();

    b.spin = lv_spinner_create(root);
    lv_obj_set_size(b.spin, 22, 22);
    lv_obj_set_style_arc_width(b.spin, 3, 0);
    lv_obj_set_style_arc_width(b.spin, 3, LV_PART_INDICATOR);
    lv_obj_set_hidden(b.spin, true);
    lv_obj_set_floating(b.spin, true);
    lv_obj_align(b.spin, LV_ALIGN_TOP_RIGHT, 0, 4);

    b.page = lv_obj_create(root);
    lv_obj_remove_style_all(b.page);
    lv_obj_set_width(b.page, LV_PCT(100));
    lv_obj_set_flex_grow(b.page, 1);
    lv_obj_set_flex_flow(b.page, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(b.page, 8, 0);
    lv_obj_set_style_pad_bottom(b.page, 16, 0);
    lv_obj_set_scroll_dir(b.page, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(b.page, LV_SCROLLBAR_MODE_OFF);

    b.status = ui_label(root, "", &lv_font_montserrat_12, UI_DIM);
    lv_obj_set_style_pad_bottom(b.status, OS_GESTURE_H, 0);

    b.kb = ui_keyboard(root, b.ta);
    go(getenv("ESPRING_URL") ? getenv("ESPRING_URL") : "about:home", false);    /* simulator: open a page directly */
}
