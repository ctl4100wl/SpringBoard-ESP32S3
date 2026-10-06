/**
 * @file app_notes.c
 * Notes list + editor. Empty notes are discarded when leaving the editor.
 */
#include <string.h>
#include "../os/os.h"
#include "../os/ui.h"

#define MAX_NOTES   12
#define NOTE_LEN    512

static struct {
    char text[MAX_NOTES][NOTE_LEN];
    int n;
} s = {
    {
        "Welcome to Notes\nTap a note to edit it. The keyboard works with your real keyboard too.",
        "Groceries\nMilk, eggs, coffee",
    },
    2,
};

static struct {
    lv_obj_t * root, * list_view, * list, * edit_view, * ta, * kb;
    int editing;
} u;

static void list_rebuild(void);

static void title_of(const char * text, char * out, size_t n)
{
    size_t i = 0;
    while(text[i] && text[i] != '\n' && i < n - 1) {
        out[i] = text[i];
        i++;
    }
    out[i] = '\0';
    if(!out[0]) lv_strncpy(out, "New Note", n);
}

static void editor_close(void)
{
    if(u.editing < 0) return;
    const char * t = lv_textarea_get_text(u.ta);
    if(t[0]) {
        lv_strncpy(s.text[u.editing], t, NOTE_LEN - 1);
        s.text[u.editing][NOTE_LEN - 1] = '\0';
    }
    else {
        for(int i = u.editing + 1; i < s.n; i++) memcpy(s.text[i - 1], s.text[i], NOTE_LEN);
        s.n--;
    }
    u.editing = -1;
    ui_keyboard_hide(u.kb);
    lv_obj_set_hidden(u.edit_view, true);
    lv_obj_set_hidden(u.list_view, false);
    list_rebuild();
}

static void editor_open(int i)
{
    u.editing = i;
    lv_textarea_set_text(u.ta, s.text[i]);
    lv_obj_set_hidden(u.list_view, true);
    lv_obj_set_hidden(u.edit_view, false);
}

static void note_cb(lv_event_t * e)
{
    editor_open((int)(intptr_t)lv_event_get_user_data(e));
}

static void new_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    if(s.n == MAX_NOTES) return;
    s.text[s.n][0] = '\0';
    editor_open(s.n++);
    ui_keyboard_show(u.kb);
}

static void back_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    editor_close();
}

static void ta_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    ui_keyboard_show(u.kb);
}

static void list_rebuild(void)
{
    lv_obj_clean(u.list);
    char title[40];
    for(int i = s.n - 1; i >= 0; i--) {
        lv_obj_t * row = ui_box(u.list);
        lv_obj_set_size(row, LV_PCT(100), 58);
        lv_obj_set_style_pad_hor(row, 12, 0);
        lv_obj_set_style_border_side(row, LV_BORDER_SIDE_BOTTOM, 0);
        lv_obj_set_style_border_width(row, i > 0 ? 1 : 0, 0);
        lv_obj_set_style_border_color(row, lv_color_hex(UI_SEP), 0);
        lv_obj_set_style_bg_color(row, lv_color_hex(UI_SEP), LV_STATE_PRESSED);
        lv_obj_set_style_bg_opa(row, LV_OPA_COVER, LV_STATE_PRESSED);
        lv_obj_set_clickable(row, true);
        lv_obj_add_event_cb(row, note_cb, LV_EVENT_CLICKED, (void *)(intptr_t)i);

        title_of(s.text[i], title, sizeof(title));
        lv_obj_t * t = ui_label(row, title, &lv_font_montserrat_16, 0xFFFFFF);
        lv_obj_set_width(t, LV_PCT(100));
        lv_label_set_long_mode(t, LV_LABEL_LONG_MODE_DOTS);
        lv_obj_align(t, LV_ALIGN_LEFT_MID, 0, -10);

        const char * nl = strchr(s.text[i], '\n');
        lv_obj_t * p = ui_label(row, nl ? nl + 1 : "No preview", &lv_font_montserrat_12, UI_DIM);
        lv_obj_set_width(p, LV_PCT(100));
        lv_label_set_long_mode(p, LV_LABEL_LONG_MODE_DOTS);
        lv_obj_align(p, LV_ALIGN_LEFT_MID, 0, 12);
    }
    if(s.n == 0) {
        lv_obj_t * l = ui_label(u.list, "No Notes", &lv_font_montserrat_16, UI_DIM);
        lv_obj_set_style_pad_all(l, 14, 0);
    }
}

static void root_delete_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    /* Leaving the app mid-edit keeps what was typed */
    if(u.editing >= 0) {
        const char * t = lv_textarea_get_text(u.ta);
        lv_strncpy(s.text[u.editing], t, NOTE_LEN - 1);
        s.text[u.editing][NOTE_LEN - 1] = '\0';
        if(!t[0]) {
            for(int i = u.editing + 1; i < s.n; i++) memcpy(s.text[i - 1], s.text[i], NOTE_LEN);
            s.n--;
        }
        u.editing = -1;
    }
}

static void create(lv_obj_t * root)
{
    u.root = root;
    u.editing = -1;

    /* List */
    u.list_view = ui_box(root);
    lv_obj_set_size(u.list_view, LV_PCT(100), LV_PCT(100));
    lv_obj_t * body = ui_app_scaffold(u.list_view, "Notes");
    u.list = ui_card(body);
    lv_obj_t * nb = ui_button(body, LV_SYMBOL_EDIT "  New Note", UI_GROUP, new_cb, NULL);
    lv_obj_set_width(nb, LV_PCT(100));
    lv_obj_set_style_text_color(lv_obj_get_child(nb, 0), lv_color_hex(0xE5B800), 0);

    /* Editor */
    u.edit_view = ui_box(root);
    lv_obj_set_size(u.edit_view, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_pad_top(u.edit_view, 38, 0);
    lv_obj_set_style_pad_hor(u.edit_view, 10, 0);
    lv_obj_set_style_pad_bottom(u.edit_view, OS_GESTURE_H, 0);
    lv_obj_set_flex_flow(u.edit_view, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(u.edit_view, 6, 0);

    lv_obj_t * back = ui_button(u.edit_view, LV_SYMBOL_LEFT " Notes", 0x000000, back_cb, NULL);
    lv_obj_set_style_text_color(lv_obj_get_child(back, 0), lv_color_hex(0xE5B800), 0);
    lv_obj_set_style_pad_hor(back, 4, 0);

    u.ta = lv_textarea_create(u.edit_view);
    lv_obj_set_width(u.ta, LV_PCT(100));
    lv_obj_set_flex_grow(u.ta, 1);
    lv_textarea_set_max_length(u.ta, NOTE_LEN - 1);
    lv_obj_set_style_bg_color(u.ta, lv_color_black(), 0);
    lv_obj_set_style_border_width(u.ta, 0, 0);
    lv_obj_set_style_text_font(u.ta, &lv_font_montserrat_16, 0);
    lv_obj_add_event_cb(u.ta, ta_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_set_hidden(u.edit_view, true);

    u.kb = ui_keyboard(root, u.ta);
    lv_obj_add_event_cb(root, root_delete_cb, LV_EVENT_DELETE, NULL);
    list_rebuild();
}

const app_desc_t app_notes = { "Notes", LV_SYMBOL_FILE, 0xE5B800, create };
