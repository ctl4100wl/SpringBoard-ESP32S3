/**
 * @file app_calculator.c
 * iOS-style calculator with chained operations; state survives relaunch.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "../os/os.h"
#include "../os/ui.h"

#ifdef S3_COMPACT
#define BTN         24
#define BTN_GAP     3
#else
#define BTN         66
#define BTN_GAP     8
#endif
#define MAX_DIGITS  9

static struct {
    double acc;
    char op;            /* 0, '+', '-', '*', '/' */
    char entry[24];
    bool entering;
    bool error;
} s = { 0, 0, "0", false, false };

static lv_obj_t * s_display;
static lv_obj_t * s_op_btn[4];

static const char * const k_keys[] = {
    "AC", "+/-", "%", "/",
    "7", "8", "9", "x",
    "4", "5", "6", "-",
    "1", "2", "3", "+",
    "0", ".", "=",
};

static void fmt(double v, char * out, size_t n)
{
    if(!isfinite(v)) {
        lv_snprintf(out, n, "Error");
        return;
    }
    if(fabs(v) < 1e9 && v == floor(v)) lv_snprintf(out, n, "%.0f", v);
    else snprintf(out, n, "%.8g", v);
}

static double current(void)
{
    return s.entering ? atof(s.entry) : s.acc;
}

static double apply(double a, double b, char op)
{
    switch(op) {
        case '+': return a + b;
        case '-': return a - b;
        case '*': return a * b;
        case '/': return b == 0.0 ? NAN : a / b;
        default:  return b;
    }
}

static void refresh(void)
{
    char buf[24];
    if(s.error) lv_snprintf(buf, sizeof(buf), "Error");
    else if(s.entering) lv_snprintf(buf, sizeof(buf), "%s", s.entry);
    else fmt(s.acc, buf, sizeof(buf));
    lv_label_set_text(s_display, buf);

    static const char ops[4] = { '/', '*', '-', '+' };
    for(int i = 0; i < 4; i++) {
        bool sel = !s.entering && s.op == ops[i];
        lv_obj_set_style_bg_color(s_op_btn[i], lv_color_hex(sel ? 0xFFFFFF : UI_ORANGE), 0);
        lv_obj_set_style_text_color(s_op_btn[i], lv_color_hex(sel ? UI_ORANGE : 0xFFFFFF), 0);
    }
}

static void press(const char * k)
{
    if(s.error && strcmp(k, "AC") != 0) return;

    if(k[0] >= '0' && k[0] <= '9') {
        if(!s.entering) {
            strcpy(s.entry, "0");
            s.entering = true;
        }
        int digits = 0;
        for(const char * c = s.entry; *c; c++) if(*c >= '0' && *c <= '9') digits++;
        if(digits >= MAX_DIGITS) return;
        if(strcmp(s.entry, "0") == 0) s.entry[0] = '\0';
        else if(strcmp(s.entry, "-0") == 0) s.entry[1] = '\0';
        strncat(s.entry, k, sizeof(s.entry) - strlen(s.entry) - 1);
    }
    else if(strcmp(k, ".") == 0) {
        if(!s.entering) {
            strcpy(s.entry, "0");
            s.entering = true;
        }
        if(!strchr(s.entry, '.')) strcat(s.entry, ".");
    }
    else if(strcmp(k, "AC") == 0) {
        s.acc = 0;
        s.op = 0;
        strcpy(s.entry, "0");
        s.entering = false;
        s.error = false;
    }
    else if(strcmp(k, "+/-") == 0) {
        if(s.entering) {
            if(s.entry[0] == '-') memmove(s.entry, s.entry + 1, strlen(s.entry));
            else if(strlen(s.entry) < sizeof(s.entry) - 1) {
                memmove(s.entry + 1, s.entry, strlen(s.entry) + 1);
                s.entry[0] = '-';
            }
        }
        else {
            s.acc = -s.acc;
        }
    }
    else if(strcmp(k, "%") == 0) {
        double v = current() / 100.0;
        if(s.entering) fmt(v, s.entry, sizeof(s.entry));
        else s.acc = v;
    }
    else if(strcmp(k, "=") == 0) {
        if(s.op) {
            s.acc = apply(s.acc, current(), s.op);
            s.op = 0;
        }
        else if(s.entering) {
            s.acc = current();
        }
        s.entering = false;
        s.error = !isfinite(s.acc);
    }
    else {
        char op = k[0] == 'x' ? '*' : k[0];
        if(s.entering) {
            s.acc = s.op ? apply(s.acc, current(), s.op) : current();
            s.entering = false;
            s.error = !isfinite(s.acc);
        }
        s.op = op;
    }
    refresh();
}

static void key_cb(lv_event_t * e)
{
    press(lv_event_get_user_data(e));
}

static void create(lv_obj_t * root)
{
#ifdef S3_COMPACT
    lv_obj_set_style_pad_hor(root, 4, 0);
    lv_obj_set_style_pad_bottom(root, 4, 0);
#else
    lv_obj_set_style_pad_hor(root, 16, 0);
    lv_obj_set_style_pad_bottom(root, OS_GESTURE_H + 6, 0);
#endif
    lv_obj_set_flex_flow(root, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(root, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_END);
    lv_obj_set_style_pad_row(root, 10, 0);
#ifdef S3_COMPACT
    lv_obj_set_style_pad_row(root, 2, 0);
#endif

    s_display = ui_label(root, "0", &lv_font_montserrat_48, 0xFFFFFF);
    lv_obj_set_width(s_display, LV_PCT(100));
    lv_obj_set_style_text_align(s_display, LV_TEXT_ALIGN_RIGHT, 0);
    lv_label_set_long_mode(s_display, LV_LABEL_LONG_MODE_CLIP);

    lv_obj_t * pad = ui_box(root);
    lv_obj_set_size(pad, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(pad, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_style_pad_row(pad, BTN_GAP, 0);
    lv_obj_set_style_pad_column(pad, BTN_GAP, 0);

    int button_size = BTN;
    int button_gap = BTN_GAP;
#ifdef S3_NATIVE_LAYOUT
    /* Native Vietnamese glyph leading needs room above the five keypad rows. */
    button_gap = 9;
    lv_obj_update_layout(root);
    int display_height = lv_obj_get_height(s_display);
    button_size = LV_MIN((lv_obj_get_content_width(root) - 3 * button_gap) / 4,
                        (lv_obj_get_content_height(root) - display_height - 10 - 4 * button_gap) / 5);
    button_size = LV_MAX(36, button_size);
    button_size = button_size / 3 * 3;
    lv_obj_set_style_pad_row(pad, button_gap, 0);
    lv_obj_set_style_pad_column(pad, button_gap, 0);
    lv_obj_set_width(pad, button_size * 4 + button_gap * 3);
    lv_obj_set_scrollable(root, false);
#endif

    int op_i = 0;
    for(size_t i = 0; i < sizeof(k_keys) / sizeof(k_keys[0]); i++) {
        const char * k = k_keys[i];
        bool is_op = (i % 4 == 3 && i < 16) || strcmp(k, "=") == 0;
        bool is_fn = i < 3;
        uint32_t bg = is_op ? UI_ORANGE : (is_fn ? 0xA5A5A5 : 0x333333);

        lv_obj_t * b = ui_button(pad, k, bg, key_cb, (void *)k);
        bool wide = strcmp(k, "0") == 0;
        lv_obj_set_size(b, wide ? button_size * 2 + button_gap : button_size, button_size);
        lv_obj_set_style_radius(b, button_size / 2, 0);
        lv_obj_t * l = lv_obj_get_child(b, 0);
        lv_obj_set_style_text_font(l, ui_font_vietnamese(&lv_font_montserrat_24), 0);
        if(is_fn) lv_obj_set_style_text_color(l, lv_color_black(), 0);
        if(is_op && strcmp(k, "=") != 0) s_op_btn[op_i++] = b;
    }
    refresh();
}

const app_desc_t app_calculator = { "Calculator", LV_SYMBOL_PLUS, 0xFF9500, create };
