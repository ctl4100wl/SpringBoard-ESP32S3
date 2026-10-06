#ifndef ESP_PLATFORM

#include "browser_rich.h"
#include <litehtml.h>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cstring>
#include <map>
#include <string>
#include <vector>

extern "C" {
typedef void (*rich_data_cb_t)(bool ok, const uint8_t * data, size_t len, void * user_data);
void sdk_http_get_data(const char * url, rich_data_cb_t cb, void * user_data, lv_obj_t * owner);
lv_draw_buf_t * sdk_image_decode_mem(const uint8_t * data, size_t len, int32_t max_w, int32_t max_h);
size_t sdk_http_fetch_sync(const char * url, char * buf, size_t cap);
void lv_image_cache_drop(const void * src);
}

static bool s_images = true;

void rich_set_images(bool on)
{
    s_images = on;
}

static std::string resolve(const std::string & base, const std::string & href)
{
    if(href.compare(0, 7, "http://") == 0 || href.compare(0, 8, "https://") == 0 || href.compare(0, 5, "data:") == 0 ||
       href.compare(0, 6, "about:") == 0) return href;
    if(href.compare(0, 2, "//") == 0) return "https:" + href;
    size_t scheme = base.find("://");
    size_t host = scheme == std::string::npos ? 0 : scheme + 3;
    size_t path = base.find('/', host);
    std::string origin = path == std::string::npos ? base : base.substr(0, path);
    std::string dir = path == std::string::npos ? "/" : base.substr(path, base.rfind('/') - path + 1);
    std::string p;
    if(!href.empty() && href[0] == '/') p = href;
    else p = dir + href;
    std::string query;
    size_t q = p.find_first_of("?#");
    if(q != std::string::npos) {
        query = p.substr(q);
        p = p.substr(0, q);
    }
    std::vector<std::string> parts;
    size_t i = 0;
    while(i <= p.size()) {
        size_t j = p.find('/', i);
        if(j == std::string::npos) j = p.size();
        std::string seg = p.substr(i, j - i);
        if(seg == "..") {
            if(!parts.empty()) parts.pop_back();
        }
        else if(seg != "." && !(seg.empty() && i != 0 && j != p.size())) parts.push_back(seg);
        i = j + 1;
    }
    std::string out;
    for(size_t k = 0; k < parts.size(); k++) {
        if(k) out += "/";
        out += parts[k];
    }
    if(out.empty() || out[0] != '/') out = "/" + out;
    return origin + out + query;
}

struct Fnt {
    const lv_font_t * f;
    int deco;
};

static const struct { int px; const lv_font_t * f; } k_fonts[] = {
    { 12, &lv_font_montserrat_12 }, { 14, &lv_font_montserrat_14 }, { 16, &lv_font_montserrat_16 },
    { 18, &lv_font_montserrat_18 }, { 20, &lv_font_montserrat_20 }, { 22, &lv_font_montserrat_22 },
    { 24, &lv_font_montserrat_24 }, { 28, &lv_font_montserrat_28 }, { 32, &lv_font_montserrat_32 },
    { 36, &lv_font_montserrat_36 }, { 40, &lv_font_montserrat_40 }, { 44, &lv_font_montserrat_44 },
    { 48, &lv_font_montserrat_48 },
};

/* System fonts that cover what Montserrat lacks: accents, Cyrillic, Greek, CJK, Hangul, Arabic, Indic, symbols.
 * Each file is read once; every Montserrat size gets its own chain of TinyTTF fallbacks. */
static const char * const k_fallback_files[] = {
    "C:/Windows/Fonts/NotoSans-Regular.ttf", "C:/Windows/Fonts/arial.ttf",   "C:/Windows/Fonts/msyh.ttc",
    "C:/Windows/Fonts/malgun.ttf",           "C:/Windows/Fonts/NotoSansArabic-Regular.ttf",
    "C:/Windows/Fonts/Nirmala.ttc",          "C:/Windows/Fonts/seguisym.ttf",
};
struct FontFile {
    std::vector<uint8_t> data;
    bool tried = false;
};
static FontFile s_ff[sizeof(k_fallback_files) / sizeof(k_fallback_files[0])];

static const std::vector<uint8_t> * font_file(size_t i)
{
    FontFile & ff = s_ff[i];
    if(!ff.tried) {
        ff.tried = true;
        FILE * f = fopen(k_fallback_files[i], "rb");
        if(f) {
            fseek(f, 0, SEEK_END);
            long n = ftell(f);
            fseek(f, 0, SEEK_SET);
            if(n > 1024 && n < 64L * 1024 * 1024) {
                ff.data.resize((size_t)n);
                if(fread(ff.data.data(), 1, (size_t)n, f) != (size_t)n) ff.data.clear();
            }
            fclose(f);
        }
    }
    return ff.data.empty() ? nullptr : &ff.data;
}

static const lv_font_t * with_fallbacks(const lv_font_t * base, int px)
{
    static std::map<int, lv_font_t *> cache;
    auto it = cache.find(px);
    if(it != cache.end()) return it->second;
    lv_font_t * head = new lv_font_t(*base);
    lv_font_t * prev = head;
    bool have_latin = false;
    for(size_t i = 0; i < sizeof(k_fallback_files) / sizeof(k_fallback_files[0]); i++) {
        if(i == 1 && have_latin) continue;                       /* arial only when Noto Sans is missing */
        const std::vector<uint8_t> * d = font_file(i);
        if(!d) continue;
        lv_font_t * tt = lv_tiny_ttf_create_data(d->data(), d->size(), px);
        if(!tt) continue;
        if(i == 0) have_latin = true;
        prev->fallback = tt;
        prev = tt;
    }
    cache[px] = head;
    return head;
}

static const lv_font_t * pick_font(float px)
{
    int best = 0;
    float bd = 1e9f;
    for(int i = 0; i < (int)(sizeof(k_fonts) / sizeof(k_fonts[0])); i++) {
        float d = std::fabs((float)k_fonts[i].px - px);
        if(d < bd) {
            bd = d;
            best = i;
        }
    }
    return with_fallbacks(k_fonts[best].f, k_fonts[best].px);
}

struct Img {
    lv_draw_buf_t * buf = nullptr;
    bool requested = false;
};

struct Rich : litehtml::document_container {
    lv_obj_t * obj = nullptr;
    litehtml::document::ptr doc;
    std::string base, title;
    int32_t W = 296, H = 400;
    rich_link_cb_t on_link = nullptr;
    void * ud = nullptr;
    std::map<std::string, Img> imgs;
    int fetches = 0;
    lv_layer_t * layer = nullptr;
    int ox = 0, oy = 0;

    ~Rich() override
    {
        for(auto & kv : imgs) {
            if(kv.second.buf) {
                lv_image_cache_drop(kv.second.buf);
                lv_draw_buf_destroy(kv.second.buf);
            }
        }
    }

    litehtml::uint_ptr create_font(const litehtml::font_description & d, const litehtml::document *,
                                   litehtml::font_metrics * fm) override
    {
        float px = d.size.value() * 0.875f;
        if(d.weight >= 700) px *= 1.08f;
        const lv_font_t * f = pick_font(px);
        if(fm) {
            fm->font_size = d.size;
            fm->height = (float)f->line_height;
            fm->ascent = (float)(f->line_height - f->base_line);
            fm->descent = (float)f->base_line;
            fm->x_height = f->line_height * 0.5f;
            fm->ch_width = (float)tw("0", f);
            fm->draw_spaces = true;
            fm->sub_shift = f->line_height * 0.2f;
            fm->super_shift = f->line_height * 0.3f;
        }
        return (litehtml::uint_ptr) new Fnt{ f, d.decoration_line };
    }

    void delete_font(litehtml::uint_ptr h) override
    {
        delete (Fnt *)h;
    }

    static int32_t tw(const char * text, const lv_font_t * f)
    {
        int sp = (int)lv_font_get_glyph_width(f, ' ', 0);
        if(sp <= 0) sp = LV_MAX(2, f->line_height / 4);
        const char * e = text + std::strlen(text);
        int lead = 0, trail = 0;
        while(*text == ' ') { text++; lead++; }
        while(e > text && e[-1] == ' ') { e--; trail++; }
        int32_t w = 0;
        if(e > text) {
            std::string core(text, e - text);
            lv_point_t sz;
            lv_text_get_size(&sz, core.c_str(), f, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
            w = sz.x;
        }
        return w + (lead + trail) * sp;
    }

    litehtml::pixel_t text_width(const char * text, litehtml::uint_ptr h) override
    {
        return (float)tw(text, ((Fnt *)h)->f);
    }

    void draw_text(litehtml::uint_ptr hdc, const char * text, litehtml::uint_ptr h, litehtml::web_color c,
                   const litehtml::position & pos) override
    {
        Rich * r = (Rich *)hdc;
        Fnt * fn = (Fnt *)h;
        lv_draw_label_dsc_t d;
        lv_draw_label_dsc_init(&d);
        d.text = text;
        d.text_local = 1;
        d.font = fn->f;
        d.color = lv_color_make(c.red, c.green, c.blue);
        d.opa = c.alpha;
        if(fn->deco & litehtml::text_decoration_line_underline) d.decor = LV_TEXT_DECOR_UNDERLINE;
        else if(fn->deco & litehtml::text_decoration_line_line_through) d.decor = LV_TEXT_DECOR_STRIKETHROUGH;
        lv_area_t a;
        a.x1 = r->ox + (int)pos.x.value();
        a.y1 = r->oy + (int)pos.y.value();
        a.x2 = a.x1 + (int)pos.width.value() + 60;
        a.y2 = a.y1 + (int)pos.height.value() + 4;
        lv_draw_label(r->layer, &d, &a);
    }

    litehtml::pixel_t pt_to_px(float pt) const override
    {
        return pt * 96.0f / 72.0f;
    }
    litehtml::pixel_t get_default_font_size() const override
    {
        return 16;
    }
    const char * get_default_font_name() const override
    {
        return "montserrat";
    }

    lv_area_t box(const litehtml::position & p) const
    {
        lv_area_t a;
        a.x1 = ox + (int)p.x.value();
        a.y1 = oy + (int)p.y.value();
        a.x2 = a.x1 + LV_MAX(1, (int)p.width.value()) - 1;
        a.y2 = a.y1 + LV_MAX(1, (int)p.height.value()) - 1;
        return a;
    }

    void rect(const litehtml::position & p, litehtml::web_color c, int radius = 0)
    {
        if(c.alpha == 0 || p.width.value() <= 0 || p.height.value() <= 0) return;
        lv_draw_rect_dsc_t d;
        lv_draw_rect_dsc_init(&d);
        d.bg_color = lv_color_make(c.red, c.green, c.blue);
        d.bg_opa = c.alpha;
        d.radius = radius;
        lv_area_t a = box(p);
        lv_draw_rect(layer, &d, &a);
    }

    void draw_list_marker(litehtml::uint_ptr hdc, const litehtml::list_marker & m) override
    {
        Rich * r = (Rich *)hdc;
        r->layer = ((Rich *)hdc)->layer;
        switch(m.marker_type) {
            case litehtml::list_style_type_disc:
                r->rect(m.pos, m.color, LV_RADIUS_CIRCLE);
                break;
            case litehtml::list_style_type_square:
                r->rect(m.pos, m.color);
                break;
            case litehtml::list_style_type_circle: {
                lv_draw_rect_dsc_t d;
                lv_draw_rect_dsc_init(&d);
                d.bg_opa = LV_OPA_TRANSP;
                d.border_width = 1;
                d.border_color = lv_color_make(m.color.red, m.color.green, m.color.blue);
                d.radius = LV_RADIUS_CIRCLE;
                lv_area_t a = r->box(m.pos);
                lv_draw_rect(r->layer, &d, &a);
                break;
            }
            case litehtml::list_style_type_decimal: {
                char t[16];
                lv_snprintf(t, sizeof(t), "%d.", m.index);
                lv_draw_label_dsc_t d;
                lv_draw_label_dsc_init(&d);
                d.text = t;
                d.text_local = 1;
                d.font = m.font ? ((Fnt *)m.font)->f : &lv_font_montserrat_14;
                d.color = lv_color_make(m.color.red, m.color.green, m.color.blue);
                lv_area_t a = r->box(m.pos);
                a.x1 -= 10;
                a.x2 += 30;
                lv_draw_label(r->layer, &d, &a);
                break;
            }
            default:
                break;
        }
    }

    struct ImgCtx {
        Rich * self;
        std::string url;
    };

    static void img_cb(bool ok, const uint8_t * data, size_t len, void * user)
    {
        ImgCtx * c = (ImgCtx *)user;
        Rich * r = c->self;
        if(ok) {
            lv_draw_buf_t * buf = sdk_image_decode_mem(data, len, 640, 900);
            if(buf) {
                r->imgs[c->url].buf = buf;
                r->relayout();
            }
        }
        delete c;
    }

    void relayout()
    {
        if(!doc) return;
        doc->render(W);
        H = LV_MAX(40, (int)doc->height());
        lv_obj_set_height(obj, H);
        lv_obj_invalidate(obj);
    }

    void load_image(const char * src, const char * baseurl, bool) override
    {
        if(!s_images || !src || !*src) return;
        std::string url = resolve(baseurl && *baseurl ? baseurl : base, src);
        Img & im = imgs[url];
        if(im.requested) return;
        im.requested = true;
        if(url.compare(0, 5, "data:") == 0 || fetches >= 40) return;
        fetches++;
        sdk_http_get_data(url.c_str(), &Rich::img_cb, new ImgCtx{ this, url }, obj);
    }

    void get_image_size(const char * src, const char * baseurl, litehtml::size & sz) override
    {
        std::string url = resolve(baseurl && *baseurl ? baseurl : base, src ? src : "");
        auto it = imgs.find(url);
        if(it != imgs.end() && it->second.buf) {
            sz.width = (float)it->second.buf->header.w;
            sz.height = (float)it->second.buf->header.h;
        }
        else {
            sz.width = 0;
            sz.height = 0;
        }
    }

    void draw_image(litehtml::uint_ptr hdc, const litehtml::background_layer & l, const std::string & url,
                    const std::string & base_url) override
    {
        Rich * r = (Rich *)hdc;
        auto it = imgs.find(resolve(base_url.empty() ? base : base_url, url));
        if(it == imgs.end() || !it->second.buf) return;
        lv_draw_buf_t * buf = it->second.buf;
        int iw = (int)buf->header.w, ih = (int)buf->header.h;
        if(iw <= 0 || ih <= 0) return;
        lv_area_t clip = r->box(l.clip_box), saved = r->layer->_clip_area, cur;
        cur.x1 = LV_MAX(saved.x1, clip.x1);
        cur.y1 = LV_MAX(saved.y1, clip.y1);
        cur.x2 = LV_MIN(saved.x2, clip.x2);
        cur.y2 = LV_MIN(saved.y2, clip.y2);
        if(cur.x1 > cur.x2 || cur.y1 > cur.y2) return;
        r->layer->_clip_area = cur;
        int dw = LV_MAX(1, (int)l.origin_box.width.value()), dh = LV_MAX(1, (int)l.origin_box.height.value());
        int x0 = r->ox + (int)l.origin_box.x.value(), y0 = r->oy + (int)l.origin_box.y.value();
        int nx = 1, ny = 1;
        if(l.repeat == litehtml::background_repeat_repeat || l.repeat == litehtml::background_repeat_repeat_x)
            nx = LV_CLAMP(1, (int)(l.clip_box.width.value() / dw) + 2, 40);
        if(l.repeat == litehtml::background_repeat_repeat || l.repeat == litehtml::background_repeat_repeat_y)
            ny = LV_CLAMP(1, (int)(l.clip_box.height.value() / dh) + 2, 40);
        for(int yy = 0; yy < ny; yy++) {
            for(int xx = 0; xx < nx; xx++) {
                lv_draw_image_dsc_t d;
                lv_draw_image_dsc_init(&d);
                d.src = buf;
                d.scale_x = 256 * dw / iw;
                d.scale_y = 256 * dh / ih;
                d.pivot.x = 0;
                d.pivot.y = 0;
                lv_area_t a;
                a.x1 = x0 + xx * dw;
                a.y1 = y0 + yy * dh;
                a.x2 = a.x1 + iw - 1;
                a.y2 = a.y1 + ih - 1;
                lv_draw_image(r->layer, &d, &a);
            }
        }
        r->layer->_clip_area = saved;
    }

    void draw_solid_fill(litehtml::uint_ptr hdc, const litehtml::background_layer & l,
                         const litehtml::web_color & c) override
    {
        Rich * r = (Rich *)hdc;
        r->rect(l.border_box, c, (int)l.border_radius.top_left_x.value());
    }

    void fill_first_stop(litehtml::uint_ptr hdc, const litehtml::background_layer & l,
                         const std::vector<litehtml::background_layer::color_point> & pts)
    {
        if(!pts.empty()) draw_solid_fill(hdc, l, pts[0].color);
    }

    void draw_linear_gradient(litehtml::uint_ptr hdc, const litehtml::background_layer & l,
                              const litehtml::background_layer::linear_gradient & g) override
    {
        fill_first_stop(hdc, l, g.color_points);
    }
    void draw_radial_gradient(litehtml::uint_ptr hdc, const litehtml::background_layer & l,
                              const litehtml::background_layer::radial_gradient & g) override
    {
        fill_first_stop(hdc, l, g.color_points);
    }
    void draw_conic_gradient(litehtml::uint_ptr hdc, const litehtml::background_layer & l,
                             const litehtml::background_layer::conic_gradient & g) override
    {
        fill_first_stop(hdc, l, g.color_points);
    }

    static bool visible(const litehtml::border & b)
    {
        return b.width.value() > 0 && b.style != litehtml::border_style_none && b.style != litehtml::border_style_hidden;
    }

    void draw_borders(litehtml::uint_ptr hdc, const litehtml::borders & b, const litehtml::position & p, bool) override
    {
        Rich * r = (Rich *)hdc;
        if(visible(b.top)) r->rect(litehtml::position(p.x, p.y, p.width, b.top.width), b.top.color);
        if(visible(b.bottom))
            r->rect(litehtml::position(p.x, p.y + p.height - b.bottom.width, p.width, b.bottom.width), b.bottom.color);
        if(visible(b.left)) r->rect(litehtml::position(p.x, p.y, b.left.width, p.height), b.left.color);
        if(visible(b.right))
            r->rect(litehtml::position(p.x + p.width - b.right.width, p.y, b.right.width, p.height), b.right.color);
    }

    void set_caption(const char * c) override
    {
        title = c ? c : "";
    }
    void set_base_url(const char * u) override
    {
        if(u && *u) base = resolve(base, u);
    }
    void link(const std::shared_ptr<litehtml::document> &, const litehtml::element::ptr &) override {}
    void on_anchor_click(const char * url, const litehtml::element::ptr &) override
    {
        if(url && *url && on_link) on_link(resolve(base, url).c_str(), ud);
    }
    void on_mouse_event(const litehtml::element::ptr &, litehtml::mouse_event) override {}
    void set_cursor(const char *) override {}
    void transform_text(std::string & t, litehtml::text_transform tt) override
    {
        if(tt == litehtml::text_transform_uppercase) for(auto & c : t) c = (char)toupper((unsigned char)c);
        else if(tt == litehtml::text_transform_lowercase) for(auto & c : t) c = (char)tolower((unsigned char)c);
    }
    void import_css(std::string &, const std::string &, std::string &) override {}
    void set_clip(const litehtml::position &, const litehtml::border_radiuses &) override {}
    void del_clip() override {}
    void get_viewport(litehtml::position & v) const override
    {
        v = litehtml::position(0, 0, (float)W, 520.0f);
    }
    litehtml::element::ptr create_element(const char *, const litehtml::string_map &,
                                          const std::shared_ptr<litehtml::document> &) override
    {
        return nullptr;
    }
    void get_media_features(litehtml::media_features & m) const override
    {
        m.type = litehtml::media_type_screen;
        m.width = (float)W;
        m.height = 520.0f;
        m.device_width = (float)W;
        m.device_height = 520.0f;
        m.color = 8;
        m.monochrome = 0;
        m.color_index = 0;
        m.resolution = 96;
    }
    void get_language(std::string & lang, std::string & culture) const override
    {
        lang = "en";
        culture = "US";
    }
};

static void rich_draw_cb(lv_event_t * e)
{
    Rich * r = (Rich *)lv_event_get_user_data(e);
    lv_layer_t * layer = lv_event_get_layer(e);
    lv_area_t oa;
    lv_obj_get_coords(r->obj, &oa);
    r->layer = layer;
    r->ox = oa.x1;
    r->oy = oa.y1;
    lv_area_t ca = layer->_clip_area;
    litehtml::position clip((float)(ca.x1 - oa.x1), (float)(ca.y1 - oa.y1), (float)(ca.x2 - ca.x1 + 1),
                            (float)(ca.y2 - ca.y1 + 1));
    r->doc->draw((litehtml::uint_ptr)r, 0, 0, &clip);
}

static void rich_click_cb(lv_event_t * e)
{
    Rich * r = (Rich *)lv_event_get_user_data(e);
    lv_point_t p;
    lv_indev_get_point(lv_indev_active(), &p);
    lv_area_t oa;
    lv_obj_get_coords(r->obj, &oa);
    int x = p.x - oa.x1, y = p.y - oa.y1;
    auto redraw = [](const litehtml::position &) {};
    r->doc->on_lbutton_down(x, y, x, y, redraw);
    r->doc->on_lbutton_up(x, y, x, y, redraw);
}

static void rich_delete_cb(lv_event_t * e)
{
    delete (Rich *)lv_event_get_user_data(e);
}

extern "C" lv_obj_t * rich_create(lv_obj_t * parent, const char * html, const char * base_url, int32_t width,
                                  rich_link_cb_t on_link, void * user_data, char * title_out, size_t title_n)
{
    Rich * r = new Rich();
    r->base = base_url ? base_url : "";
    r->W = width;
    r->on_link = on_link;
    r->ud = user_data;
    lv_obj_t * o = lv_obj_create(parent);
    lv_obj_remove_style_all(o);
    lv_obj_set_size(o, width, 40);
    lv_obj_set_style_bg_opa(o, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(o, lv_color_white(), 0);
    lv_obj_set_style_radius(o, 8, 0);
    lv_obj_set_style_clip_corner(o, true, 0);
    lv_obj_add_flag(o, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(o, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(o, LV_OBJ_FLAG_EVENT_BUBBLE);
    r->obj = o;
    lv_obj_add_event_cb(o, rich_draw_cb, LV_EVENT_DRAW_MAIN, r);
    lv_obj_add_event_cb(o, rich_click_cb, LV_EVENT_CLICKED, r);
    lv_obj_add_event_cb(o, rich_delete_cb, LV_EVENT_DELETE, r);
    r->doc = litehtml::document::createFromString(html ? html : "", r);
    if(!r->doc) {
        lv_obj_delete(o);
        return nullptr;
    }
    r->relayout();
    if(title_out && title_n) {
        std::strncpy(title_out, r->title.c_str(), title_n - 1);
        title_out[title_n - 1] = '\0';
    }
    return o;
}

static std::string lower(std::string s)
{
    for(auto & c : s) c = (char)tolower((unsigned char)c);
    return s;
}

static bool attr_of(const std::string & tag, const char * name, std::string & out)
{
    std::string lt = lower(tag);
    std::string key = std::string(name) + "=";
    size_t p = 0;
    while((p = lt.find(key, p)) != std::string::npos) {
        if(p == 0 || lt[p - 1] == ' ' || lt[p - 1] == '\t' || lt[p - 1] == '\n') break;
        p += key.size();
    }
    if(p == std::string::npos) return false;
    p += key.size();
    char q = (p < tag.size() && (tag[p] == '"' || tag[p] == '\'')) ? tag[p++] : ' ';
    size_t e = p;
    while(e < tag.size() && tag[e] != q && tag[e] != '>') e++;
    out = tag.substr(p, e - p);
    return true;
}

extern "C" char * rich_inline_css(const char * html, const char * base_url)
{
    std::string in = html ? html : "", out;
    std::string base = base_url ? base_url : "";
    std::string lin = lower(in);
    size_t pos = 0;
    int sheets = 0;
    std::vector<char> buf(300 * 1024);
    while(true) {
        size_t t = lin.find("<link", pos);
        if(t == std::string::npos) break;
        size_t e = lin.find('>', t);
        if(e == std::string::npos) break;
        std::string tag = in.substr(t, e - t + 1);
        std::string rel, href;
        out += in.substr(pos, t - pos);
        pos = e + 1;
        if(attr_of(tag, "rel", rel) && lower(rel).find("stylesheet") != std::string::npos && attr_of(tag, "href", href) &&
           sheets < 8) {
            sheets++;
            size_t n = sdk_http_fetch_sync(resolve(base, href).c_str(), buf.data(), buf.size());
            if(n > 0) {
                out += "<style>";
                out.append(buf.data(), n);
                out += "</style>";
                continue;
            }
        }
        out += tag;
    }
    out += in.substr(pos);
    char * r = (char *)std::malloc(out.size() + 1);
    if(r) std::memcpy(r, out.c_str(), out.size() + 1);
    return r;
}

#endif
