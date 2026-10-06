/**
 * @file app_phone.c
 * Keypad with a call screen (timer, mute/speaker toggles) and a short call history.
 */
#include <string.h>
#include "../os/os.h"
#include "../os/ui.h"

#define KEY         64
#define NUM_LEN     20
#define HIST_N      5

static const char * const k_digits[] = { "1", "2", "3", "4", "5", "6", "7", "8", "9", "*", "0", "#" };
static const char * const k_sub[] = { "", "ABC", "DEF", "GHI", "JKL", "MNO", "PQRS", "TUV", "WXYZ", "", "+", "" };

static struct {
    char number[NUM_LEN + 1];
    char history[HIST_N][NUM_LEN + 1];
    uint32_t hist_secs[HIST_N];
    int hist_n;
} s;

static struct {
    lv_obj_t * root, * num, * call_view, * call_time, * hist;
    lv_timer_t * timer;
    uint32_t call_start;
} u;

static void hist_refresh(void)
{
    if(s.hist_n == 0) {
        lv_label_set_text(u.hist, "");
        return;
    }
    lv_label_set_text_fmt(u.hist, "Last call: %s (%u:%02u)", s.history[0],
                          (unsigned)(s.hist_secs[0] / 60), (unsigned)(s.hist_secs[0] % 60));
}

static void num_refresh(void)
{
    lv_label_set_text(u.num, s.number);
}

static void key_cb(lv_event_t * e)
{
    const char * d = lv_event_get_user_data(e);
    size_t n = strlen(s.number);
    if(n < NUM_LEN) {
        s.number[n] = d[0];
        s.number[n + 1] = '\0';
    }
    num_refresh();
}

static void back_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    size_t n = strlen(s.number);
    if(n) s.number[n - 1] = '\0';
    num_refresh();
}

static void tick_cb(lv_timer_t * t)
{
    LV_UNUSED(t);
    uint32_t sec = lv_tick_elaps(u.call_start) / 1000;
    lv_label_set_text_fmt(u.call_time, "%02u:%02u", (unsigned)(sec / 60), (unsigned)(sec % 60));
}

static void end_call(void)
{
    if(!u.timer) return;
    uint32_t sec = lv_tick_elaps(u.call_start) / 1000;
    lv_timer_delete(u.timer);
    u.timer = NULL;

    memmove(s.history[1], s.history[0], sizeof(s.history[0]) * (HIST_N - 1));
    memmove(&s.hist_secs[1], &s.hist_secs[0], sizeof(s.hist_secs[0]) * (HIST_N - 1));
    lv_strncpy(s.history[0], s.number, NUM_LEN);
    s.hist_secs[0] = sec;
    if(s.hist_n < HIST_N) s.hist_n++;
}

static void end_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    end_call();
    lv_obj_set_hidden(u.call_view, true);
    hist_refresh();
}

static void toggle_cb(lv_event_t * e)
{
    lv_obj_t * b = lv_event_get_target_obj(e);
    bool on = !lv_obj_has_state(b, LV_STATE_CHECKED);
    if(on) lv_obj_add_state(b, LV_STATE_CHECKED);
    else lv_obj_remove_state(b, LV_STATE_CHECKED);
    lv_obj_set_style_bg_color(b, lv_color_hex(on ? 0xFFFFFF : 0x3A3A3C), 0);
    lv_obj_set_style_text_color(lv_obj_get_child(b, 0), lv_color_hex(on ? 0x000000 : 0xFFFFFF), 0);
}

static void call_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    if(!s.number[0]) {
        /* Empty keypad redials the last number, like iOS */
        if(!s.hist_n) return;
        lv_strncpy(s.number, s.history[0], NUM_LEN);
        num_refresh();
        return;
    }
    lv_obj_t * name = lv_obj_get_child(u.call_view, 0);
    lv_label_set_text(name, s.number);
    lv_label_set_text(u.call_time, "calling...");
    lv_obj_set_hidden(u.call_view, false);
    u.call_start = lv_tick_get();
    u.timer = lv_timer_create(tick_cb, 500, NULL);
}

static void root_delete_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    end_call();
}

static lv_obj_t * round_key(lv_obj_t * parent, const char * main_t, const char * sub, uint32_t bg)
{
    lv_obj_t * b = lv_button_create(parent);
    lv_obj_set_size(b, KEY, KEY);
    lv_obj_set_style_radius(b, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(b, lv_color_hex(bg), 0);
    lv_obj_set_style_shadow_width(b, 0, 0);
    lv_obj_set_flex_flow(b, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(b, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(b, 0, 0);
    lv_obj_set_style_pad_row(b, 0, 0);
    ui_label(b, main_t, &lv_font_montserrat_28, 0xFFFFFF);
    if(sub && sub[0]) ui_label(b, sub, &lv_font_montserrat_12, 0xD0D0D0);
    return b;
}

static void create(lv_obj_t * root)
{
    u.root = root;
    lv_obj_set_style_pad_top(root, 44, 0);
    lv_obj_set_style_pad_bottom(root, OS_GESTURE_H + 6, 0);
    lv_obj_set_flex_flow(root, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(root, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(root, 8, 0);

    u.num = ui_label(root, "", &lv_font_montserrat_32, 0xFFFFFF);
    lv_obj_set_height(u.num, 40);
    u.hist = ui_label(root, "", &lv_font_montserrat_12, UI_DIM);

    lv_obj_t * pad = ui_box(root);
    lv_obj_set_size(pad, KEY * 3 + 22 * 2, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(pad, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_style_pad_column(pad, 22, 0);
    lv_obj_set_style_pad_row(pad, 12, 0);
    for(int i = 0; i < 12; i++) {
        lv_obj_t * b = round_key(pad, k_digits[i], k_sub[i], 0x333333);
        lv_obj_add_event_cb(b, key_cb, LV_EVENT_CLICKED, (void *)k_digits[i]);
    }

    lv_obj_t * actions = ui_box(root);
    lv_obj_set_size(actions, KEY * 3 + 22 * 2, KEY);
    lv_obj_t * call = round_key(actions, LV_SYMBOL_CALL, NULL, UI_GREEN);
    lv_obj_align(call, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_event_cb(call, call_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t * back = ui_button(actions, LV_SYMBOL_BACKSPACE, 0x000000, back_cb, NULL);
    lv_obj_set_size(back, KEY, KEY);
    lv_obj_align(back, LV_ALIGN_RIGHT_MID, 0, 0);

    /* Call screen overlay */
    u.call_view = ui_box(root);
    lv_obj_set_floating(u.call_view, true);
    lv_obj_set_size(u.call_view, lv_pct(100), lv_pct(100));
    lv_obj_set_pos(u.call_view, 0, 0);
    lv_obj_set_style_bg_opa(u.call_view, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(u.call_view, lv_color_hex(0x1B2A3A), 0);
    lv_obj_set_style_bg_grad_color(u.call_view, lv_color_hex(0x0B0F14), 0);
    lv_obj_set_style_bg_grad_dir(u.call_view, LV_GRAD_DIR_VER, 0);
    lv_obj_set_clickable(u.call_view, true);
    lv_obj_set_flex_flow(u.call_view, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(u.call_view, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_top(u.call_view, 60, 0);
    lv_obj_set_style_pad_row(u.call_view, 10, 0);
    ui_label(u.call_view, "", &lv_font_montserrat_32, 0xFFFFFF);
    u.call_time = ui_label(u.call_view, "", &lv_font_montserrat_16, UI_DIM);

    lv_obj_t * opts = ui_box(u.call_view);
    lv_obj_set_size(opts, LV_PCT(100), 160);
    lv_obj_set_flex_flow(opts, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(opts, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    const char * opt_sym[] = { LV_SYMBOL_MUTE, LV_SYMBOL_KEYBOARD, LV_SYMBOL_VOLUME_MAX };
    for(int i = 0; i < 3; i++) {
        lv_obj_t * b = round_key(opts, opt_sym[i], NULL, 0x3A3A3C);
        lv_obj_add_event_cb(b, toggle_cb, LV_EVENT_CLICKED, NULL);
    }
    lv_obj_t * end = round_key(u.call_view, LV_SYMBOL_CALL, NULL, UI_RED);
    lv_obj_add_event_cb(end, end_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_set_hidden(u.call_view, true);

    lv_obj_add_event_cb(root, root_delete_cb, LV_EVENT_DELETE, NULL);
    num_refresh();
    hist_refresh();
}

const app_desc_t app_phone = { "Phone", LV_SYMBOL_CALL, 0x30D158, create };
