/**
 * @file app_settings.c
 * iOS-style Settings. Real: radios, brightness, volume, notification previews, Focus, home indicator,
 * quick app switching, wallpapers (presets or a picture from the PC), lock clock style, About style.
 * Everything else is a working-looking placeholder (toggles are remembered) for the demo.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../os/sdk.h"

void lv_image_cache_drop(const void * src);

extern const app_desc_t app_notify;

static lv_obj_t * s_root;
static lv_obj_t * s_about;      /* About page body, rebuilt when its style changes */

static void changed(void)
{
    shell_settings_apply();
    os_save();
}

static void flag_cb(lv_event_t * e)
{
    bool * flag = lv_event_get_user_data(e);
    *flag = lv_obj_has_state(lv_event_get_target_obj(e), LV_STATE_CHECKED);
    if(flag == &os_settings.airplane && *flag) os_settings.wifi = os_settings.bluetooth = false;
    changed();
}

static void slider_cb(lv_event_t * e)
{
    uint8_t * v = lv_event_get_user_data(e);
    *v = (uint8_t)lv_slider_get_value(lv_event_get_target_obj(e));
    shell_settings_apply();
    if(lv_event_get_code(e) == LV_EVENT_RELEASED) os_save();
}

static lv_obj_t * slider_row(lv_obj_t * card, const char * sym, uint32_t accent, uint8_t * value, int min)
{
    lv_obj_t * row = ui_row(card, sym, accent, "");
    lv_obj_t * sl = lv_slider_create(row);
    lv_obj_set_flex_grow(sl, 1);
    lv_obj_set_height(sl, 6);
    lv_slider_set_range(sl, min, 100);
    lv_slider_set_value(sl, *value, LV_ANIM_OFF);
    lv_obj_add_event_cb(sl, slider_cb, LV_EVENT_VALUE_CHANGED, value);
    lv_obj_add_event_cb(sl, slider_cb, LV_EVENT_RELEASED, value);
    return sl;
}

/*================ Placeholder pages from a tiny spec ================*/
/* "C:caption" new section, "S:label" remembered switch, "V:label:value" value row,
 * "N:label" chevron row, "T:text" footnote */

static void kv_switch_cb(lv_event_t * e)
{
    const char * key = lv_event_get_user_data(e);
    sdk_set_int(key, lv_obj_has_state(lv_event_get_target_obj(e), LV_STATE_CHECKED));
}

static void spec_build(lv_obj_t * body, const char * const * spec)
{
    lv_obj_t * card = NULL;
    for(; *spec; spec++) {
        const char * s = *spec;
        char label[48];
        const char * val = strchr(s + 2, ':');
        size_t n = val ? (size_t)(val - (s + 2)) : strlen(s + 2);
        if(n >= sizeof(label)) n = sizeof(label) - 1;
        memcpy(label, s + 2, n);
        label[n] = '\0';

        if(s[0] == 'C') {
            card = ui_section(body, label[0] ? label : NULL);
            continue;
        }
        if(s[0] == 'T') {
            lv_obj_t * t = ui_label(body, s + 2, &lv_font_montserrat_12, UI_DIM);
            lv_obj_set_width(t, LV_PCT(100));
            lv_label_set_long_mode(t, LV_LABEL_LONG_MODE_WRAP);
            lv_obj_set_style_pad_hor(t, 12, 0);
            continue;
        }
        if(!card) card = ui_section(body, NULL);
        if(s[0] == 'S') {
            /* The spec string itself is static, so it doubles as the storage key */
            ui_switch_row(card, NULL, 0, label, sdk_get_int(s, 0) != 0, kv_switch_cb, (void *)s);
        }
        else if(s[0] == 'V') {
            lv_obj_t * row = ui_row(card, NULL, 0, label);
            ui_label(row, val ? val + 1 : "", &lv_font_montserrat_14, UI_DIM);
        }
        else {
            ui_nav_row(card, NULL, 0, label, val ? val + 1 : NULL, NULL, NULL);
        }
    }
}

static void spec_page_cb(lv_event_t * e)
{
    const char * const * spec = lv_event_get_user_data(e);
    /* spec[0] is the title */
    spec_build(ui_nav_push(s_root, spec[0] + 2, "Settings"), spec + 1);
}

static void battery_page_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    lv_obj_t * body = ui_nav_push(s_root, "Battery", "Settings");
    lv_obj_t * card = ui_section(body, NULL);
    ui_switch_row(card, LV_SYMBOL_BATTERY_FULL, UI_GREEN, "Low Power Mode", os_settings.low_power,
                  flag_cb, &os_settings.low_power);
    ui_label(body, "The Clock icon ticks once per second when Low Power Mode is on.",
             &lv_font_montserrat_12, UI_DIM);
}

static const char * const k_account[] = { "T:Account", "C:", "V:Name:ESP User", "V:Email:user@espringboard.dev",
                                          "N:Password & Security", "N:Payment & Shipping", "C:", "N:Cloud:5 GB",
                                          "N:Family:Set Up", "S:Find My", NULL };
static const char * const k_hotspot[] = { "T:Personal Hotspot", "C:", "S:Allow Others to Join", "V:Wi-Fi Password:esp32s3!",
                                          "T:Placeholder - the simulator has no modem.", NULL };
static const char * const k_sounds[] = { "T:Sounds & Haptics", "C:RINGTONE AND ALERTS", "N:Ringtone:Reflection",
                                         "N:Text Tone:Note", "S:Keyboard Clicks", "S:Lock Sound", "S:System Haptics", NULL };
static const char * const k_screen_time[] = { "T:Screen Time", "C:DAILY AVERAGE", "V:Today:2h 14m", "V:This Week:-12%",
                                              "C:LIMIT USAGE", "N:Downtime:Off", "N:App Limits", "N:Communication Limits",
                                              "S:Screen Distance", NULL };
static const char * const k_access[] = { "T:Accessibility", "C:VISION", "N:VoiceOver:Off", "N:Zoom:Off",
                                         "N:Display & Text Size", "S:Reduce Motion", "C:PHYSICAL AND MOTOR",
                                         "N:Touch", "N:Back Tap:Off", NULL };
static const char * const k_privacy[] = { "T:Privacy & Security", "C:", "N:Location Services:On", "N:Tracking",
                                          "C:", "N:Contacts", "N:Calendars", "N:Photos", "N:Camera", "N:Microphone",
                                          "C:", "S:Lockdown Mode", NULL };
static const char * const k_update[] = { "T:Software Update", "C:", "V:" OS_NAME ":1.0",
                                         "T:Your software is up to date.", "C:", "S:Automatic Updates", NULL };
static const char * const k_storage[] = { "T:Storage", "C:", "V:Used:4.8 MB of 16 MB", "V:Apps:1.9 MB",
                                          "V:System:2.6 MB", "V:Photos:0.3 MB", "C:RECOMMENDATIONS",
                                          "N:Offload Unused Apps", NULL };
static const char * const k_datetime[] = { "T:Date & Time", "C:", "S:24-Hour Time", "S:Set Automatically",
                                           "V:Time Zone:Local", NULL };
static const char * const k_keyboard[] = { "T:Keyboards", "C:", "N:Keyboards:2", "S:Auto-Capitalization",
                                           "S:Auto-Correction", "S:Smart Punctuation", NULL };
static const char * const k_language[] = { "T:Language & Region", "C:", "V:Language:English", "V:Region:World",
                                           "V:Calendar:Gregorian", NULL };
static const char * const k_cc[] = { "T:Control Center", "C:", "S:Access Within Apps",
                                     "T:Pull down from the top-right edge. Drag past the end to stretch it; "
                                     "the modules spring in after the blur.", NULL };

/*================ Real pages ================*/

static void radios_page_cb(lv_event_t * e)
{
    int which = (int)(intptr_t)lv_event_get_user_data(e);
    static const char * const titles[] = { "Wi-Fi", "Bluetooth", "Cellular" };
    bool * flags[] = { &os_settings.wifi, &os_settings.bluetooth, &os_settings.cellular };
    lv_obj_t * body = ui_nav_push(s_root, titles[which], "Settings");
    lv_obj_t * card = ui_section(body, NULL);
    ui_switch_row(card, NULL, 0, titles[which], *flags[which], flag_cb, flags[which]);
    if(which == 0) {
#ifdef ESP_PLATFORM
        extern void s3_wifi_settings(lv_obj_t * body);
        s3_wifi_settings(body);
        return;
#endif
        card = ui_section(body, "NETWORKS");
        ui_nav_row(card, LV_SYMBOL_WIFI, UI_BLUE, "Home", "Connected", NULL, NULL);
        ui_nav_row(card, LV_SYMBOL_WIFI, 0x3A3A3C, "ESP32-Lab", NULL, NULL, NULL);
        ui_nav_row(card, LV_SYMBOL_WIFI, 0x3A3A3C, "Cafe Guest", NULL, NULL, NULL);
        ui_label(body, "Network list is a placeholder; internet access uses the PC's connection.",
                 &lv_font_montserrat_12, UI_DIM);
    }
    else if(which == 1) {
        card = ui_section(body, "MY DEVICES");
        ui_nav_row(card, LV_SYMBOL_AUDIO, UI_BLUE, "Buds Pro", "Not Connected", NULL, NULL);
        ui_nav_row(card, LV_SYMBOL_KEYBOARD, UI_BLUE, "Keyboard", "Not Connected", NULL, NULL);
    }
}

static void open_notify_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    sdk_open_app(&app_notify);
}

static void notif_page_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    lv_obj_t * body = ui_nav_push(s_root, "Notifications", "Settings");
    lv_obj_t * card = ui_section(body, "DISPLAY AS");
    ui_switch_row(card, NULL, 0, "Show Previews", os_settings.previews, flag_cb, &os_settings.previews);
    ui_switch_row(card, NULL, 0, "Focus (silence banners)", os_settings.dnd, flag_cb, &os_settings.dnd);
    card = ui_section(body, NULL);
    ui_nav_row(card, LV_SYMBOL_BELL, UI_RED, "Send a Test Notification", NULL, open_notify_cb, NULL);
    ui_label(body, "Previews off hides notification text on the Lock Screen.", &lv_font_montserrat_12, UI_DIM);
}

static void sounds_page_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    lv_obj_t * body = ui_nav_push(s_root, "Sounds & Haptics", "Settings");
    slider_row(ui_section(body, "VOLUME"), LV_SYMBOL_VOLUME_MAX, UI_RED, &os_settings.volume, 0);
    spec_build(body, k_sounds + 1);
}

static void display_page_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    lv_obj_t * body = ui_nav_push(s_root, "Display & Brightness", "Settings");
    slider_row(ui_section(body, "BRIGHTNESS"), LV_SYMBOL_EYE_OPEN, UI_BLUE, &os_settings.brightness, 10);
    static const char * const rest[] = { "C:APPEARANCE", "V:Mode:Dark", "S:True Tone", "C:", "N:Auto-Lock:Never",
                                         "S:Raise to Wake", NULL };
    spec_build(body, rest);
}

static void lock_now_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    lock_now();
}

static lv_obj_t * s_edge_lbl;

static void edge_size_cb(lv_event_t * e)
{
    os_settings.edge_size = (uint8_t)lv_slider_get_value(lv_event_get_target_obj(e));
    lv_label_set_text_fmt(s_edge_lbl, "%d px", os_settings.edge_size);
    shell_settings_apply();
    if(lv_event_get_code(e) == LV_EVENT_RELEASED) os_save();
}

static void recents_style_cb(lv_event_t * e)
{
    sdk_set_int("recents.style", lv_obj_has_state(lv_event_get_target_obj(e), LV_STATE_CHECKED) ? 1 : 0);
}

static void gestures_page_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    lv_obj_t * body = ui_nav_push(s_root, "Gestures", "Settings");
    lv_obj_t * card = ui_section(body, "NAVIGATION");
    ui_switch_row(card, NULL, 0, "Show Home Indicator", os_settings.pill, flag_cb, &os_settings.pill);
    ui_switch_row(card, NULL, 0, "Swipe Between Apps", os_settings.quick_switch, flag_cb, &os_settings.quick_switch);
    ui_switch_row(card, NULL, 0, "Stacked App Switcher", sdk_get_int("recents.style", 0) != 0, recents_style_cb, NULL);
    lv_obj_t * card2 = ui_section(body, "BACK GESTURE");
    ui_switch_row(card2, NULL, 0, "Swipe from Edge to Go Back", os_settings.edge_back, flag_cb, &os_settings.edge_back);
    lv_obj_t * srow = ui_row(card2, NULL, 0, "Edge Size");
    s_edge_lbl = ui_label(srow, "", &lv_font_montserrat_14, UI_DIM);
    lv_label_set_text_fmt(s_edge_lbl, "%d px", os_settings.edge_size);
    lv_obj_t * sl = lv_slider_create(card2);
    lv_obj_set_width(sl, LV_PCT(90));
    lv_obj_set_style_margin_ver(sl, 10, 0);
    lv_obj_set_style_margin_hor(sl, 12, 0);
    lv_slider_set_range(sl, 8, 64);
    lv_slider_set_value(sl, os_settings.edge_size, LV_ANIM_OFF);
    lv_obj_add_event_cb(sl, edge_size_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(sl, edge_size_cb, LV_EVENT_RELEASED, NULL);
    lv_obj_t * t = ui_label(body,
                            "Swipe up from the bottom edge: Home.  Swipe up and hold: App Switcher.\n"
                            "Swipe sideways along the bottom edge: previous / next app.\n"
                            "Top-left edge: Notification Center.  Top-right edge: Control Center.\n"
                            "Swipe down on the Home Screen: Search.  Long-press: edit icons and widgets.",
                            &lv_font_montserrat_12, UI_DIM);
    lv_obj_set_width(t, LV_PCT(100));
    lv_label_set_long_mode(t, LV_LABEL_LONG_MODE_WRAP);
}

/*---------------- Wallpaper ----------------*/

static lv_obj_t * s_sw_home[OS_WALLPAPER_COUNT + 1], * s_sw_lock[OS_WALLPAPER_COUNT + 1];

/* The preset wallpapers are real photos from picsum.photos, cached next to the program on first use */
static const int k_wall_net[OS_WALLPAPER_COUNT] = { 1018, 1015, 1039, 1043, 1036, 1011 };

static void wall_file(int i, char * out, size_t n)
{
    char rel[64];
    lv_snprintf(rel, sizeof(rel), "cache/wallpapers/wall_net_%d.jpg", k_wall_net[i]);
#ifdef _WIN32
    if(_fullpath(out, rel, n)) return;
#endif
    lv_strncpy(out, rel, n - 1);
}

static bool wall_cached(int i)
{
    char path[260];
    wall_file(i, path, sizeof(path));
    FILE * f = fopen(path, "rb");
    if(f) fclose(f);
    return f != NULL;
}

static bool sw_selected(bool lock, int i)
{
    uint8_t cur = lock ? os_settings.lock_wall : os_settings.wallpaper;
    if(i < OS_WALLPAPER_COUNT) {
        if(cur == OS_WALL_CUSTOM) {
            char path[260];
            wall_file(i, path, sizeof(path));
            return strcmp(sdk_get_str(lock ? "wall.lock" : "wall.home", ""), path) == 0;
        }
        return cur == i;
    }
    if(cur != OS_WALL_CUSTOM) return false;
    for(int k = 0; k < OS_WALLPAPER_COUNT; k++) if(sw_selected(lock, k)) return false;
    return true;
}

static void swatches_refresh(void)
{
    for(int i = 0; i <= OS_WALLPAPER_COUNT; i++) {
        if(s_sw_home[i]) lv_obj_set_style_border_width(s_sw_home[i], sw_selected(false, i) ? 3 : 0, 0);
        if(s_sw_lock[i]) lv_obj_set_style_border_width(s_sw_lock[i], sw_selected(true, i) ? 3 : 0, 0);
    }
}

static void sw_buf_free_cb(lv_event_t * e)
{
    lv_draw_buf_t * b = lv_event_get_user_data(e);
    lv_image_cache_drop(b);
    lv_draw_buf_destroy(b);
}

static void sw_photo_show(lv_obj_t * sw, int i)
{
    if(!sw) return;
    char path[260];
    wall_file(i, path, sizeof(path));
    lv_draw_buf_t * b = sdk_image_load(path, 54, 86);
    if(!b) return;
    lv_obj_clean(sw);
    lv_obj_set_style_clip_corner(sw, true, 0);
    lv_image_set_src(lv_image_create(sw), b);
    lv_obj_add_event_cb(sw, sw_buf_free_cb, LV_EVENT_DELETE, b);
}

static void wall_dl_cb(bool ok, const uint8_t * data, size_t len, void * ud)
{
    int i = (int)(intptr_t)ud;
    if(!ok || len < 64) return;
    char path[260];
    wall_file(i, path, sizeof(path));
    sdk_ensure_dir("cache/wallpapers");
    FILE * f = fopen(path, "wb");
    if(!f) return;
    fwrite(data, 1, len, f);
    fclose(f);
    sw_photo_show(s_sw_home[i], i);
    sw_photo_show(s_sw_lock[i], i);
}

static void swatch_cb(lv_event_t * e)
{
    int code = (int)(intptr_t)lv_event_get_user_data(e);
    bool lock = code >= 100;
    int i = code % 100;
    uint8_t * target = lock ? &os_settings.lock_wall : &os_settings.wallpaper;
    if(i == OS_WALLPAPER_COUNT) {
        char path[260];
        if(!sdk_pick_image(path, sizeof(path))) {
            if(!sdk_get_str(lock ? "wall.lock" : "wall.home", "")[0]) return;
        }
        else {
            sdk_set_str(lock ? "wall.lock" : "wall.home", path);
        }
        *target = OS_WALL_CUSTOM;
        changed();
        if(!shell_wallpaper_image(lock)) sdk_toast("Wallpaper", "Could not decode that image (PNG, JPG or BMP).");
    }
    else if(wall_cached(i)) {
        char path[260];
        wall_file(i, path, sizeof(path));
        sdk_set_str(lock ? "wall.lock" : "wall.home", path);
        *target = OS_WALL_CUSTOM;
        changed();
        if(!shell_wallpaper_image(lock)) {
            *target = (uint8_t)i;
            changed();
        }
    }
    else {
        *target = (uint8_t)i;
        changed();
    }
    swatches_refresh();
}

static void swatch_row(lv_obj_t * body, const char * caption, bool lock, lv_obj_t ** out)
{
    lv_obj_t * page_owner = body;
    lv_obj_t * card = ui_section(body, caption);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(card, 10, 0);
    lv_obj_set_style_pad_row(card, 10, 0);
    for(int i = 0; i <= OS_WALLPAPER_COUNT; i++) {
        lv_obj_t * s = ui_box(card);
        lv_obj_set_size(s, 54, 86);
        lv_obj_set_style_radius(s, 10, 0);
        lv_obj_set_style_bg_opa(s, LV_OPA_COVER, 0);
        lv_obj_set_style_border_color(s, lv_color_white(), 0);
        lv_obj_set_clickable(s, true);
        lv_obj_add_event_cb(s, swatch_cb, LV_EVENT_CLICKED, (void *)(intptr_t)((lock ? 100 : 0) + i));
        if(i < OS_WALLPAPER_COUNT) {
            uint32_t top, bottom;
            shell_wallpaper_colors(i, &top, &bottom);
            lv_obj_set_style_bg_color(s, lv_color_hex(top), 0);
            lv_obj_set_style_bg_grad_color(s, lv_color_hex(bottom), 0);
            lv_obj_set_style_bg_grad_dir(s, LV_GRAD_DIR_VER, 0);
            out[i] = s;
            if(wall_cached(i)) sw_photo_show(s, i);
            else if(!lock) {
                char url[96];
                lv_snprintf(url, sizeof(url), "https://picsum.photos/id/%d/720/1280", k_wall_net[i]);
                sdk_http_get_data(url, wall_dl_cb, (void *)(intptr_t)i, page_owner);
            }
        }
        else {
            lv_obj_set_style_bg_color(s, lv_color_hex(0x3A3A3C), 0);
            lv_obj_t * l = ui_label(s, LV_SYMBOL_DIRECTORY "\nPC", &lv_font_montserrat_14, 0xFFFFFF);
            lv_obj_set_style_text_align(l, LV_TEXT_ALIGN_CENTER, 0);
            lv_obj_center(l);
        }
        out[i] = s;
    }
}

static lv_obj_t * s_style_btn[OS_LOCK_STYLES];

static void lock_style_cb(lv_event_t * e)
{
    os_settings.lock_style = (uint8_t)(intptr_t)lv_event_get_user_data(e);
    for(int i = 0; i < OS_LOCK_STYLES; i++) {
        lv_obj_set_style_bg_color(s_style_btn[i], lv_color_hex(i == os_settings.lock_style ? UI_BLUE : UI_SEP), 0);
    }
    changed();
}

static void glass_apply(int32_t bezel, int32_t strength, bool chroma)
{
    ui_lens_settings_set(bezel, strength, chroma);
    lock_refresh();
    os_save();
}

static void glass_bezel_cb(lv_event_t * e)
{
    int32_t b, st;
    bool c;
    ui_lens_settings_get(&b, &st, &c);
    glass_apply(lv_slider_get_value(lv_event_get_target_obj(e)), st, c);
}

static void glass_strength_cb(lv_event_t * e)
{
    int32_t b, st;
    bool c;
    ui_lens_settings_get(&b, &st, &c);
    glass_apply(b, lv_slider_get_value(lv_event_get_target_obj(e)), c);
}

static void glass_chroma_cb(lv_event_t * e)
{
    int32_t b, st;
    bool c;
    ui_lens_settings_get(&b, &st, &c);
    glass_apply(b, st, lv_obj_has_state(lv_event_get_target_obj(e), LV_STATE_CHECKED));
}

static void glass_slider(lv_obj_t * card, const char * name, int32_t min, int32_t max, int32_t val, lv_event_cb_t cb)
{
    lv_obj_t * row = ui_row(card, NULL, 0, name);
    lv_obj_t * sl = lv_slider_create(card);
    lv_obj_set_width(sl, LV_PCT(90));
    lv_obj_set_style_margin_ver(sl, 8, 0);
    lv_slider_set_range(sl, min, max);
    lv_slider_set_value(sl, val, LV_ANIM_OFF);
    lv_obj_add_event_cb(sl, cb, LV_EVENT_VALUE_CHANGED, NULL);
    LV_UNUSED(row);
}

static void wallpaper_page_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    lv_obj_t * body = ui_nav_push(s_root, "Wallpaper", "Settings");
    memset(s_sw_home, 0, sizeof(s_sw_home));
    memset(s_sw_lock, 0, sizeof(s_sw_lock));
    swatch_row(body, "HOME SCREEN", false, s_sw_home);
    swatch_row(body, "LOCK SCREEN", true, s_sw_lock);
    swatches_refresh();

    lv_obj_t * card = ui_section(body, "LOCK SCREEN CLOCK");
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_style_pad_row(card, 6, 0);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(card, 8, 0);
    static const char * const names[OS_LOCK_STYLES] = { "Classic", "Glass", "Stacked", "Glow", "Frosted", "Prism" };
    for(int i = 0; i < OS_LOCK_STYLES; i++) {
        s_style_btn[i] = ui_button(card, names[i], i == os_settings.lock_style ? UI_BLUE : UI_SEP, lock_style_cb,
                                   (void *)(intptr_t)i);
        lv_obj_set_style_pad_hor(s_style_btn[i], 8, 0);
        lv_obj_set_style_text_font(lv_obj_get_child(s_style_btn[i], 0), &lv_font_montserrat_12, 0);
    }
    lv_obj_t * gcard = ui_section(body, "GLASS CLOCK (liquid glass)");
    int32_t gb, gs;
    bool gc;
    ui_lens_settings_get(&gb, &gs, &gc);
    glass_slider(gcard, "Bezel width", 6, 40, gb, glass_bezel_cb);
    glass_slider(gcard, "Glass thickness", 0, 100, gs, glass_strength_cb);
    ui_switch_row(gcard, NULL, 0, "Color fringing", gc, glass_chroma_cb, NULL);
    ui_label(body, "\"PC\" opens a file picker: PNG, JPG or BMP from your computer. You can also hold the "
             "Lock Screen to customize it.", &lv_font_montserrat_12, UI_DIM);
    lv_obj_set_width(lv_obj_get_child(body, -1), LV_PCT(100));
    lv_label_set_long_mode(lv_obj_get_child(body, -1), LV_LABEL_LONG_MODE_WRAP);
}

static void home_page_cb(lv_event_t * e);

/*---------------- About (iOS / HyperOS / Hybrid) ----------------*/

static void about_build(void);

static void about_style_cb(lv_event_t * e)
{
    os_settings.about_style = (uint8_t)lv_buttonmatrix_get_selected_button(lv_event_get_target_obj(e));
    os_save();
    about_build();
}

static void name_ready_cb(lv_event_t * e)
{
    lv_obj_t * ta = lv_event_get_user_data(e);
    const char * key = lv_obj_get_user_data(ta);
    sdk_set_str(key, lv_textarea_get_text(ta));
    about_build();
}

static void ta_show_kb_cb(lv_event_t * e)
{
    lv_obj_set_hidden(lv_event_get_user_data(e), false);
}

static void edit_field_cb(lv_event_t * e)
{
    const char * key = lv_event_get_user_data(e);
    bool model = strcmp(key, "about.model") == 0;
    lv_obj_t * body = ui_nav_push(s_root, model ? "Model Name" : "Name", "About");
    lv_obj_t * ta = lv_textarea_create(body);
    lv_obj_set_width(ta, LV_PCT(100));
    lv_textarea_set_one_line(ta, true);
    lv_textarea_set_max_length(ta, 40);
    lv_textarea_set_text(ta, sdk_get_str(key, model ? "ESP32-S3 DevKit" : "ESP User's SpringBoard"));
    lv_obj_set_user_data(ta, (void *)key);
    lv_obj_add_event_cb(ta, name_ready_cb, LV_EVENT_VALUE_CHANGED, ta);
    lv_obj_t * kb = ui_keyboard(lv_obj_get_parent(body), ta);
    ui_keyboard_show(kb);
    lv_obj_add_event_cb(ta, ta_show_kb_cb, LV_EVENT_CLICKED, kb);
    ui_label(body, "Shown in About and in the profile card.", &lv_font_montserrat_12, UI_DIM);
}

static lv_obj_t * spec_tile(lv_obj_t * parent, const char * caption, const char * value, const char * sub)
{
    lv_obj_t * t = ui_box(parent);
    lv_obj_set_size(t, LV_PCT(48), 84);
    lv_obj_set_style_radius(t, 18, 0);
    lv_obj_set_style_bg_opa(t, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(t, lv_color_hex(UI_GROUP), 0);
    lv_obj_set_style_pad_all(t, 12, 0);
    lv_obj_set_flex_flow(t, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(t, 2, 0);
    ui_label(t, caption, &lv_font_montserrat_12, UI_DIM);
    lv_obj_t * v = ui_label(t, value, &lv_font_montserrat_16, 0xFFFFFF);
    lv_obj_set_width(v, LV_PCT(100));
    lv_label_set_long_mode(v, LV_LABEL_LONG_MODE_DOTS);
    if(sub) {
        lv_obj_t * s = ui_label(t, sub, &lv_font_montserrat_12, UI_DIM);
        lv_obj_set_width(s, LV_PCT(100));
        lv_label_set_long_mode(s, LV_LABEL_LONG_MODE_DOTS);
    }
    return t;
}

static void hyper_hero(lv_obj_t * body, const char * name)
{
    /* HyperOS: big brand card with oversized version numerals */
    lv_obj_t * hero = ui_box(body);
    lv_obj_set_size(hero, LV_PCT(100), 150);
    lv_obj_set_style_radius(hero, 24, 0);
    lv_obj_set_style_bg_opa(hero, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(hero, lv_color_hex(0x1D4ED8), 0);
    lv_obj_set_style_bg_grad_color(hero, lv_color_hex(0x9333EA), 0);
    lv_obj_set_style_bg_grad_dir(hero, LV_GRAD_DIR_HOR, 0);
    lv_obj_set_style_pad_all(hero, 16, 0);
    lv_obj_t * big = ui_label(hero, "1", &lv_font_montserrat_48, 0xFFFFFF);
    lv_obj_set_style_transform_scale(big, 640, 0);
    lv_obj_set_style_transform_pivot_x(big, LV_PCT(100), 0);
    lv_obj_set_style_transform_pivot_y(big, LV_PCT(100), 0);
    lv_obj_set_style_text_opa(big, LV_OPA_40, 0);
    lv_obj_align(big, LV_ALIGN_BOTTOM_RIGHT, -6, 6);
    lv_obj_t * brand = ui_label(hero, OS_NAME, &lv_font_montserrat_22, 0xFFFFFF);
    lv_obj_align(brand, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_t * ver = ui_label(hero, "OS 1.0.0  " LV_SYMBOL_OK " Up to date", &lv_font_montserrat_12, 0xE0E7FF);
    lv_obj_align(ver, LV_ALIGN_TOP_LEFT, 0, 30);
    lv_obj_t * dn = ui_label(hero, name, &lv_font_montserrat_14, 0xFFFFFF);
    lv_obj_align(dn, LV_ALIGN_BOTTOM_LEFT, 0, 0);
}

static void about_build(void)
{
    if(!s_about) return;
    lv_obj_clean(s_about);
    const char * name = sdk_get_str("about.name", "ESP User's SpringBoard");
    const char * model = sdk_get_str("about.model", "ESP32-S3 DevKit");
    char sw[48];
    lv_snprintf(sw, sizeof(sw), "1.0 (LVGL %d.%d.%d)", LVGL_VERSION_MAJOR, LVGL_VERSION_MINOR, LVGL_VERSION_PATCH);
    char apps[8];
    lv_snprintf(apps, sizeof(apps), "%d", os_app_count);

    static const char * const styles[] = { "iOS", "HyperOS", "Hybrid", "" };
    lv_obj_t * seg = lv_buttonmatrix_create(s_about);
    lv_buttonmatrix_set_map(seg, styles);
    lv_buttonmatrix_set_one_checked(seg, true);
    lv_buttonmatrix_set_button_ctrl_all(seg, LV_BUTTONMATRIX_CTRL_CHECKABLE);
    lv_buttonmatrix_set_button_ctrl(seg, os_settings.about_style, LV_BUTTONMATRIX_CTRL_CHECKED);
    lv_obj_set_size(seg, LV_PCT(100), 36);
    lv_obj_set_style_pad_all(seg, 3, 0);
    lv_obj_set_style_radius(seg, 10, 0);
    lv_obj_set_style_bg_color(seg, lv_color_hex(UI_GROUP), 0);
    lv_obj_set_style_border_width(seg, 0, 0);
    lv_obj_set_style_radius(seg, 8, LV_PART_ITEMS);
    lv_obj_set_style_bg_color(seg, lv_color_hex(UI_GROUP), LV_PART_ITEMS);
    lv_obj_set_style_bg_color(seg, lv_color_hex(0x636366), LV_PART_ITEMS | LV_STATE_CHECKED);
    lv_obj_set_style_text_color(seg, lv_color_white(), LV_PART_ITEMS);
    lv_obj_add_event_cb(seg, about_style_cb, LV_EVENT_VALUE_CHANGED, NULL);

    int st = os_settings.about_style;
    if(st == 1) {
        hyper_hero(s_about, name);
        lv_obj_t * grid = ui_box(s_about);
        lv_obj_set_size(grid, LV_PCT(100), LV_SIZE_CONTENT);
        lv_obj_set_flex_flow(grid, LV_FLEX_FLOW_ROW_WRAP);
        lv_obj_set_flex_align(grid, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
        lv_obj_set_style_pad_row(grid, 8, 0);
        lv_obj_t * t = spec_tile(grid, "Device name", name, "Tap to edit");
        lv_obj_set_clickable(t, true);
        lv_obj_add_event_cb(t, edit_field_cb, LV_EVENT_CLICKED, (void *)"about.name");
        lv_obj_t * st_tile = spec_tile(grid, "Storage", "4.8 / 16 MB", NULL);
        lv_obj_t * bar = lv_bar_create(st_tile);
        lv_obj_set_size(bar, LV_PCT(100), 6);
        lv_bar_set_value(bar, 30, LV_ANIM_OFF);
        lv_obj_set_style_bg_color(bar, lv_color_hex(UI_ORANGE), LV_PART_INDICATOR);
        spec_tile(grid, "Processor", "ESP32-S3", "Xtensa LX7 x2 @ 240 MHz");
        spec_tile(grid, "Memory", "8 MB PSRAM", "512 KB SRAM");
        spec_tile(grid, "Display", "320 x 480", "Touch, 60 Hz");
        t = spec_tile(grid, "Model", model, "Tap to edit");
        lv_obj_set_clickable(t, true);
        lv_obj_add_event_cb(t, edit_field_cb, LV_EVENT_CLICKED, (void *)"about.model");
        lv_obj_t * card = ui_section(s_about, NULL);
        ui_nav_row(card, NULL, 0, "Detailed info and specs", NULL, NULL, NULL);
        ui_nav_row(card, NULL, 0, "Legal information", NULL, NULL, NULL);
        return;
    }
    if(st == 2) hyper_hero(s_about, name);

    lv_obj_t * card = ui_section(s_about, NULL);
    ui_nav_row(card, NULL, 0, "Name", name, edit_field_cb, (void *)"about.name");
    lv_obj_t * row = ui_row(card, NULL, 0, "Software Version");
    ui_label(row, sw, &lv_font_montserrat_12, UI_DIM);
    ui_nav_row(card, NULL, 0, "Model Name", model, edit_field_cb, (void *)"about.model");
    row = ui_row(card, NULL, 0, "Model Number");
    ui_label(row, "ESP-S3-N16R8", &lv_font_montserrat_14, UI_DIM);
    row = ui_row(card, NULL, 0, "Serial Number");
    ui_label(row, "ESPSB0001", &lv_font_montserrat_14, UI_DIM);
    card = ui_section(s_about, NULL);
    row = ui_row(card, NULL, 0, "Apps");
    ui_label(row, apps, &lv_font_montserrat_14, UI_DIM);
    static const char * const more[] = { "V:Capacity:16 MB", "V:Available:11.2 MB", "C:",
                                         "V:Wi-Fi Address:7C:DF:A1:00:5E:01", "V:Bluetooth:7C:DF:A1:00:5E:02", NULL };
    spec_build(s_about, more);
}

static void about_deleted_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    s_about = NULL;
}

static void about_page_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    s_about = ui_nav_push(s_root, "About", "General");
    lv_obj_add_event_cb(s_about, about_deleted_cb, LV_EVENT_DELETE, NULL);
    about_build();
}

static void general_page_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    lv_obj_t * body = ui_nav_push(s_root, "General", "Settings");
    lv_obj_t * card = ui_section(body, NULL);
    ui_nav_row(card, NULL, 0, "About", NULL, about_page_cb, NULL);
    ui_nav_row(card, NULL, 0, "Software Update", NULL, spec_page_cb, (void *)k_update);
    card = ui_section(body, NULL);
    ui_nav_row(card, NULL, 0, "Storage", NULL, spec_page_cb, (void *)k_storage);
    ui_nav_row(card, NULL, 0, "Date & Time", NULL, spec_page_cb, (void *)k_datetime);
    ui_nav_row(card, NULL, 0, "Keyboard", NULL, spec_page_cb, (void *)k_keyboard);
    ui_nav_row(card, NULL, 0, "Language & Region", NULL, spec_page_cb, (void *)k_language);
    card = ui_section(body, NULL);
    ui_nav_row(card, NULL, 0, "Lock Now", NULL, lock_now_cb, NULL);
}

/*---------------- Home Screen ----------------*/

static void reset_cb(lv_event_t * e)
{
    home_reset_layout();
    os_save();
    lv_label_set_text(lv_obj_get_child(lv_event_get_current_target_obj(e), 0), "Home Screen reset");
}

/*---------------- Home screen widgets: add / remove ----------------*/

#define WROW_MAX 24
static struct {
    lv_obj_t * cnt;
    lv_obj_t * btn[2];
} s_wrow[WROW_MAX];

static const char * const k_wsize[3] = { "Small", "Medium", "Large" };

static void wrow_refresh(int i)
{
    if(i < 0 || i >= os_widget_count || i >= WROW_MAX || !s_wrow[i].cnt) return;
    int n = home_widget_count(i);
    lv_label_set_text_fmt(s_wrow[i].cnt, "%d", n);
}

static void wrow_cb(lv_event_t * e)
{
    int code = (int)(intptr_t)lv_event_get_user_data(e);    /* idx * 2 + (0 remove, 1 add) */
    int i = code / 2;
    if(code % 2) {
        if(!home_widget_add(i)) sdk_toast("Widgets", home_widget_free_slots() ? "No room on the Home Screen." :
                                                                                 "Widget limit reached (6).");
    }
    else {
        home_widget_remove(i);
    }
    for(int k = 0; k < os_widget_count; k++) wrow_refresh(k);
}

static void widgets_page_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    lv_obj_t * body = ui_nav_push(s_root, "Widgets", "Home Screen");
    lv_obj_t * card = ui_section(body, "ON YOUR HOME SCREEN");
    lv_memzero(s_wrow, sizeof(s_wrow));
    for(int i = 0; i < os_widget_count && i < WROW_MAX; i++) {
        lv_obj_t * row = ui_row(card, os_widgets[i]->app->symbol, os_widgets[i]->app->color, os_widgets[i]->name);
        lv_obj_t * sz = ui_label(row, os_widgets[i]->resizable ? "S M L" : k_wsize[os_widgets[i]->size],
                                 &lv_font_montserrat_12, UI_DIM);
        LV_UNUSED(sz);
        for(int b = 0; b < 2; b++) {
            lv_obj_t * btn = ui_button(row, b ? LV_SYMBOL_PLUS : LV_SYMBOL_MINUS, b ? UI_GREEN : 0x48484A, wrow_cb,
                                       (void *)(intptr_t)(i * 2 + b));
            lv_obj_set_size(btn, 34, 30);
            lv_obj_set_style_pad_all(btn, 0, 0);
            s_wrow[i].btn[b] = btn;
            if(b == 0) {
                s_wrow[i].cnt = ui_label(row, "0", &lv_font_montserrat_16, 0xFFFFFF);
                lv_obj_set_width(s_wrow[i].cnt, 18);
                lv_obj_set_style_text_align(s_wrow[i].cnt, LV_TEXT_ALIGN_CENTER, 0);
            }
        }
        /* order: -  count  + */
        lv_obj_move_to_index(s_wrow[i].btn[0], -3);
        lv_obj_move_to_index(s_wrow[i].cnt, -2);
        wrow_refresh(i);
    }
    lv_obj_t * t = ui_label(body, "Up to 6 widgets fit on the Home Screen. Tap a resizable widget while editing the Home Screen "
                                  "to make it Small, Medium or Large.", &lv_font_montserrat_12, UI_DIM);
    lv_obj_set_width(t, LV_PCT(100));
    lv_label_set_long_mode(t, LV_LABEL_LONG_MODE_WRAP);
}

static void home_page_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    lv_obj_t * body = ui_nav_push(s_root, "Home Screen", "Settings");
    lv_obj_t * card = ui_section(body, NULL);
    ui_nav_row(card, LV_SYMBOL_LIST, 0x5856D6, "Widgets", NULL, widgets_page_cb, NULL);
    lv_obj_t * row = ui_nav_row(card, LV_SYMBOL_REFRESH, UI_RED, "Reset Home Screen Layout", NULL, NULL, NULL);
    lv_obj_add_event_cb(row, reset_cb, LV_EVENT_CLICKED, NULL);
    ui_label(body, "Long-press the Home Screen to move icons, add widgets (+) or remove them (-).",
             &lv_font_montserrat_12, UI_DIM);
}

/*================ Root ================*/

static void profile_card(lv_obj_t * body)
{
    const char * name = sdk_get_str("about.name", "ESP User's SpringBoard");
    lv_obj_t * card = ui_card(body);
    lv_obj_t * row = ui_box(card);
    lv_obj_set_size(row, LV_PCT(100), 70);
    lv_obj_set_style_pad_hor(row, 12, 0);
    lv_obj_set_style_pad_column(row, 12, 0);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_clickable(row, true);
    lv_obj_set_style_bg_color(row, lv_color_hex(UI_SEP), LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(row, LV_OPA_COVER, LV_STATE_PRESSED);
    lv_obj_add_event_cb(row, spec_page_cb, LV_EVENT_CLICKED, (void *)k_account);

    lv_obj_t * av = ui_box(row);
    lv_obj_set_size(av, 50, 50);
    lv_obj_set_style_radius(av, LV_RADIUS_CIRCLE, 0);
    ui_icon_style(av, 0x8E8E93);
    char ini[3] = { name[0] ? name[0] : 'E', 0, 0 };
    lv_obj_center(ui_label(av, ini, &lv_font_montserrat_22, 0xFFFFFF));
    lv_obj_t * col = ui_box(row);
    lv_obj_set_flex_grow(col, 1);
    lv_obj_set_height(col, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(col, LV_FLEX_FLOW_COLUMN);
    lv_obj_t * n = ui_label(col, name, &lv_font_montserrat_18, 0xFFFFFF);
    lv_obj_set_width(n, LV_PCT(100));
    lv_label_set_long_mode(n, LV_LABEL_LONG_MODE_DOTS);
    ui_label(col, "Account, Cloud & more", &lv_font_montserrat_12, UI_DIM);
    ui_label(row, LV_SYMBOL_RIGHT, &lv_font_montserrat_12, 0x5A5A5E);
}

static void root_deleted_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    s_root = NULL;
    s_about = NULL;
}

static void create(lv_obj_t * root)
{
    s_root = root;
    lv_obj_add_event_cb(root, root_deleted_cb, LV_EVENT_DELETE, NULL);
    lv_obj_t * body = ui_app_scaffold(root, "Settings");
    lv_obj_set_style_pad_row(body, 6, 0);
    profile_card(body);

    lv_obj_t * card = ui_section(body, NULL);
    ui_switch_row(card, LV_SYMBOL_SHUFFLE, UI_ORANGE, "Airplane Mode", os_settings.airplane, flag_cb,
                  &os_settings.airplane);
    ui_nav_row(card, LV_SYMBOL_WIFI, UI_BLUE, "Wi-Fi", os_settings.wifi ? "Home" : "Off", radios_page_cb, (void *)0);
    ui_nav_row(card, LV_SYMBOL_BLUETOOTH, UI_BLUE, "Bluetooth", os_settings.bluetooth ? "On" : "Off", radios_page_cb,
               (void *)1);
    ui_nav_row(card, LV_SYMBOL_CALL, UI_GREEN, "Cellular", NULL, radios_page_cb, (void *)2);
    ui_nav_row(card, LV_SYMBOL_LOOP, UI_GREEN, "Personal Hotspot", "Off", spec_page_cb, (void *)k_hotspot);

    card = ui_section(body, NULL);
    ui_nav_row(card, LV_SYMBOL_BELL, UI_RED, "Notifications", NULL, notif_page_cb, NULL);
    ui_nav_row(card, LV_SYMBOL_VOLUME_MAX, 0xFF2D55, "Sounds & Haptics", NULL, sounds_page_cb, NULL);
    ui_nav_row(card, LV_SYMBOL_EYE_CLOSE, 0x5E5CE6, "Focus", os_settings.dnd ? "On" : NULL, notif_page_cb, NULL);
    ui_nav_row(card, LV_SYMBOL_LOOP, 0x5E5CE6, "Screen Time", NULL, spec_page_cb, (void *)k_screen_time);

    card = ui_section(body, NULL);
    ui_nav_row(card, LV_SYMBOL_SETTINGS, 0x8E8E93, "General", NULL, general_page_cb, NULL);
    ui_nav_row(card, LV_SYMBOL_EYE_OPEN, UI_BLUE, "Accessibility", NULL, spec_page_cb, (void *)k_access);
    ui_nav_row(card, LV_SYMBOL_LIST, 0x8E8E93, "Control Center", NULL, spec_page_cb, (void *)k_cc);
    ui_nav_row(card, LV_SYMBOL_IMAGE, UI_BLUE, "Display & Brightness", NULL, display_page_cb, NULL);
    ui_nav_row(card, LV_SYMBOL_HOME, 0x5856D6, "Home Screen", NULL, home_page_cb, NULL);
    ui_nav_row(card, LV_SYMBOL_UP, 0x5856D6, "Gestures", NULL, gestures_page_cb, NULL);
    ui_nav_row(card, LV_SYMBOL_IMAGE, 0x32ADE6, "Wallpaper", NULL, wallpaper_page_cb, NULL);

    card = ui_section(body, NULL);
    ui_nav_row(card, LV_SYMBOL_BATTERY_FULL, UI_GREEN, "Battery", NULL, battery_page_cb, NULL);
    ui_nav_row(card, LV_SYMBOL_EYE_CLOSE, UI_BLUE, "Privacy & Security", NULL, spec_page_cb, (void *)k_privacy);
}

const app_desc_t app_settings = { "Settings", LV_SYMBOL_SETTINGS, 0x636366, create };
