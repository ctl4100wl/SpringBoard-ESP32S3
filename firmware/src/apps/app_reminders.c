/**
 * @file app_reminders.c
 * Checklist: add with the keyboard, tap to complete, clear completed.
 */
#include <string.h>
#include "../os/sdk.h"

#define MAX_ITEMS   24
#define ITEM_LEN    64

static struct {
    char text[MAX_ITEMS][ITEM_LEN];
    bool done[MAX_ITEMS];
    int n;
} s = {
    { "Try jiggle mode (long press home)", "Swipe up + hold for recents", "Water the plants" },
    { false, false, true },
    3,
};

static lv_obj_t * s_list;
static lv_obj_t * s_ta;
static lv_obj_t * s_kb;
static lv_obj_t * s_count;

static void list_rebuild(void);

static void item_cb(lv_event_t * e)
{
    int i = (int)(intptr_t)lv_event_get_user_data(e);
    s.done[i] = !s.done[i];
    list_rebuild();
}

static void clear_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    int w = 0;
    for(int r = 0; r < s.n; r++) {
        if(s.done[r]) continue;
        if(w != r) memcpy(s.text[w], s.text[r], ITEM_LEN);
        s.done[w++] = false;
    }
    s.n = w;
    list_rebuild();
}

static void add_current(void)
{
    const char * t = lv_textarea_get_text(s_ta);
    if(!t[0] || s.n == MAX_ITEMS) return;
    lv_strncpy(s.text[s.n], t, ITEM_LEN - 1);
    s.text[s.n][ITEM_LEN - 1] = '\0';
    s.done[s.n++] = false;
    lv_textarea_set_text(s_ta, "");
    list_rebuild();
}

static void ta_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if(code == LV_EVENT_CLICKED || code == LV_EVENT_FOCUSED) ui_keyboard_show(s_kb);
    else if(code == LV_EVENT_READY) add_current();
}

static void kb_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    add_current();
    ui_keyboard_hide(s_kb);
}

static void list_rebuild(void)
{
    lv_obj_clean(s_list);
    int open = 0;
    for(int i = 0; i < s.n; i++) {
        if(!s.done[i]) open++;
        lv_obj_t * row = ui_box(s_list);
        lv_obj_set_size(row, LV_PCT(100), LV_SIZE_CONTENT);
        lv_obj_set_style_min_height(row, 44, 0);
        lv_obj_set_style_pad_ver(row, 8, 0);
        lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_column(row, 12, 0);
        lv_obj_set_style_border_side(row, LV_BORDER_SIDE_BOTTOM, 0);
        lv_obj_set_style_border_width(row, 1, 0);
        lv_obj_set_style_border_color(row, lv_color_hex(UI_SEP), 0);
        lv_obj_set_clickable(row, true);
        lv_obj_add_event_cb(row, item_cb, LV_EVENT_CLICKED, (void *)(intptr_t)i);

        lv_obj_t * dot = ui_box(row);
        lv_obj_set_size(dot, 22, 22);
        lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_border_width(dot, 2, 0);
        lv_obj_set_style_border_color(dot, lv_color_hex(s.done[i] ? UI_BLUE : 0x5A5A5E), 0);
        lv_obj_set_style_bg_color(dot, lv_color_hex(UI_BLUE), 0);
        lv_obj_set_style_bg_opa(dot, s.done[i] ? LV_OPA_COVER : LV_OPA_TRANSP, 0);
        if(s.done[i]) lv_obj_center(ui_label(dot, LV_SYMBOL_OK, &lv_font_montserrat_12, 0xFFFFFF));

        lv_obj_t * l = ui_label(row, s.text[i], &lv_font_montserrat_16, s.done[i] ? UI_DIM : UI_TEXT);
        lv_obj_set_flex_grow(l, 1);
        lv_label_set_long_mode(l, LV_LABEL_LONG_MODE_WRAP);
        if(s.done[i]) lv_obj_set_style_text_decor(l, LV_TEXT_DECOR_STRIKETHROUGH, 0);
    }
    lv_label_set_text_fmt(s_count, "%d open", open);
}

static void create(lv_obj_t * root)
{
    lv_obj_t * body = ui_app_scaffold(root, "Reminders");

    lv_obj_t * bar = ui_box(body);
    lv_obj_set_size(bar, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(bar, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    s_count = ui_label(bar, "", &lv_font_montserrat_14, UI_DIM);
    lv_obj_t * clr = ui_button(bar, "Clear Done", UI_GROUP, clear_cb, NULL);
    lv_obj_set_height(clr, 30);
    lv_obj_set_style_text_color(lv_obj_get_child(clr, 0), lv_color_hex(UI_BLUE), 0);
    lv_obj_set_style_text_font(lv_obj_get_child(clr, 0), &lv_font_montserrat_14, 0);

    s_ta = lv_textarea_create(body);
    lv_obj_set_width(s_ta, LV_PCT(100));
    lv_textarea_set_one_line(s_ta, true);
    lv_textarea_set_max_length(s_ta, ITEM_LEN - 1);
    lv_textarea_set_placeholder_text(s_ta, "New reminder...");
    lv_obj_add_event_cb(s_ta, ta_cb, LV_EVENT_ALL, NULL);

    s_list = ui_box(body);
    lv_obj_set_size(s_list, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(s_list, LV_FLEX_FLOW_COLUMN);

    s_kb = ui_keyboard(root, s_ta);
    lv_obj_add_event_cb(s_kb, kb_cb, LV_EVENT_READY, NULL);
    list_rebuild();
}

const app_desc_t app_reminders = { "Reminders", LV_SYMBOL_OK, 0xFF9F0A, create };

/*---------------- Widget: open reminders ----------------*/

static void widget_tick(lv_timer_t * t)
{
    lv_obj_t * box = lv_timer_get_user_data(t);
    lv_obj_clean(box);
    int open = 0;
    for(int i = 0; i < s.n; i++) if(!s.done[i]) open++;
    lv_obj_t * head = ui_label(box, "", &lv_font_montserrat_14, UI_ORANGE);
    lv_label_set_text_fmt(head, LV_SYMBOL_OK "  Reminders  %d", open);
    int shown = 0;
    for(int i = 0; i < s.n && shown < 3; i++) {
        if(s.done[i]) continue;
        lv_obj_t * l = ui_label(box, s.text[i], &lv_font_montserrat_12, 0xFFFFFF);
        lv_obj_set_width(l, LV_PCT(100));
        lv_label_set_long_mode(l, LV_LABEL_LONG_MODE_DOTS);
        shown++;
    }
    if(!open) ui_label(box, "All done!", &lv_font_montserrat_12, UI_DIM);
}

WIDGET(widget_reminders, "Reminders", &app_reminders, WIDGET_MEDIUM, NULL)
{
    lv_obj_set_flex_flow(box, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(box, 6, 0);
    widget_tick(sdk_timer(box, widget_tick, 2000, box));
}
