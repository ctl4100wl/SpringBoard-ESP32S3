/**
 * @file app_2048.c
 * 2048: swipe on the board (or use arrow keys). Game and best score survive relaunch.
 */
#include <string.h>
#include "../os/os.h"
#include "../os/ui.h"

#define N       4
#ifdef S3_COMPACT
#define CELL    24
#define GAP     3
#else
#define CELL    64
#define GAP     6
#endif

static struct {
    uint16_t b[N][N];
    uint32_t score, best;
    bool started, over;
} s;

static lv_obj_t * s_cell[N][N];
static lv_obj_t * s_num[N][N];
static lv_obj_t * s_score;
static lv_obj_t * s_status;

static void spawn(void)
{
    int free_n = 0;
    for(int r = 0; r < N; r++) for(int c = 0; c < N; c++) if(!s.b[r][c]) free_n++;
    if(!free_n) return;
    int k = (int)lv_rand(0, (uint32_t)free_n - 1);
    for(int r = 0; r < N; r++) for(int c = 0; c < N; c++) {
            if(s.b[r][c]) continue;
            if(k-- == 0) {
                s.b[r][c] = lv_rand(0, 9) == 0 ? 4 : 2;
                return;
            }
        }
}

static void new_game(void)
{
    memset(s.b, 0, sizeof(s.b));
    s.score = 0;
    s.over = false;
    s.started = true;
    spawn();
    spawn();
}

/* Slide one line toward index 0; returns true if anything moved */
static bool slide(uint16_t * line[N])
{
    uint16_t out[N] = { 0 };
    int w = 0;
    bool merged = false;
    for(int i = 0; i < N; i++) {
        uint16_t v = *line[i];
        if(!v) continue;
        if(w > 0 && out[w - 1] == v && !merged) {
            out[w - 1] = v * 2;
            s.score += v * 2u;
            merged = true;
        }
        else {
            out[w++] = v;
            merged = false;
        }
    }
    bool moved = false;
    for(int i = 0; i < N; i++) {
        if(*line[i] != out[i]) moved = true;
        *line[i] = out[i];
    }
    return moved;
}

static bool can_move(void)
{
    for(int r = 0; r < N; r++) for(int c = 0; c < N; c++) {
            if(!s.b[r][c]) return true;
            if(c + 1 < N && s.b[r][c] == s.b[r][c + 1]) return true;
            if(r + 1 < N && s.b[r][c] == s.b[r + 1][c]) return true;
        }
    return false;
}

static uint32_t tile_color(uint16_t v)
{
    switch(v) {
        case 0:    return 0x3A3A3C;
        case 2:    return 0xEEE4DA;
        case 4:    return 0xEDE0C8;
        case 8:    return 0xF2B179;
        case 16:   return 0xF59563;
        case 32:   return 0xF67C5F;
        case 64:   return 0xF65E3B;
        case 128:  return 0xEDCF72;
        case 256:  return 0xEDCC61;
        case 512:  return 0xEDC850;
        case 1024: return 0xEDC53F;
        default:   return 0xEDC22E;
    }
}

static void refresh(void)
{
    for(int r = 0; r < N; r++) for(int c = 0; c < N; c++) {
            uint16_t v = s.b[r][c];
            lv_obj_set_style_bg_color(s_cell[r][c], lv_color_hex(tile_color(v)), 0);
            if(v) lv_label_set_text_fmt(s_num[r][c], "%u", v);
            else lv_label_set_text(s_num[r][c], "");
            lv_obj_set_style_text_color(s_num[r][c], lv_color_hex(v <= 4 ? 0x776E65 : 0xFFFFFF), 0);
            lv_obj_set_style_text_font(s_num[r][c], v >= 1024 ? &lv_font_montserrat_18 :
                                       (v >= 128 ? &lv_font_montserrat_22 : &lv_font_montserrat_28), 0);
        }
    if(s.score > s.best) s.best = s.score;
    lv_label_set_text_fmt(s_score, "Score %u   Best %u", (unsigned)s.score, (unsigned)s.best);
    lv_label_set_text(s_status, s.over ? "No moves left - tap New Game" : "Swipe to merge tiles");
}

static void move(lv_dir_t dir)
{
    if(s.over) return;
    bool moved = false;
    for(int i = 0; i < N; i++) {
        uint16_t * line[N];
        for(int j = 0; j < N; j++) {
            switch(dir) {
                case LV_DIR_LEFT:  line[j] = &s.b[i][j]; break;
                case LV_DIR_RIGHT: line[j] = &s.b[i][N - 1 - j]; break;
                case LV_DIR_TOP:   line[j] = &s.b[j][i]; break;
                default:           line[j] = &s.b[N - 1 - j][i]; break;
            }
        }
        if(slide(line)) moved = true;
    }
    if(moved) spawn();
    s.over = !can_move();
    refresh();
}

static void board_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if(code == LV_EVENT_GESTURE) {
        lv_dir_t d = lv_indev_get_gesture_dir(lv_indev_active());
        if(d == LV_DIR_LEFT || d == LV_DIR_RIGHT || d == LV_DIR_TOP || d == LV_DIR_BOTTOM) move(d);
    }
    else if(code == LV_EVENT_KEY) {
        uint32_t k = lv_event_get_key(e);
        if(k == LV_KEY_LEFT) move(LV_DIR_LEFT);
        else if(k == LV_KEY_RIGHT) move(LV_DIR_RIGHT);
        else if(k == LV_KEY_UP) move(LV_DIR_TOP);
        else if(k == LV_KEY_DOWN) move(LV_DIR_BOTTOM);
    }
}

static void new_cb(lv_event_t * e)
{
    LV_UNUSED(e);
    new_game();
    refresh();
}

static void create(lv_obj_t * root)
{
    if(!s.started) new_game();

    lv_obj_t * body = ui_app_scaffold(root, "2048");
#ifndef S3_COMPACT
    lv_obj_set_scrollable(body, false);
#endif
    lv_obj_set_flex_align(body, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    s_score = ui_label(body, "", &lv_font_montserrat_16, UI_TEXT);

    int32_t side = N * CELL + (N + 1) * GAP;
    lv_obj_t * board = ui_box(body);
    lv_obj_set_size(board, side, side);
    lv_obj_set_style_radius(board, 10, 0);
    lv_obj_set_style_bg_opa(board, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(board, lv_color_hex(0x2C2C2E), 0);
    lv_obj_set_clickable(board, true);
    lv_obj_set_gesture_bubble(board, false);
    lv_obj_add_event_cb(board, board_cb, LV_EVENT_GESTURE, NULL);
    lv_obj_add_event_cb(board, board_cb, LV_EVENT_KEY, NULL);
    lv_group_add_obj(lv_group_get_default(), board);
    lv_group_focus_obj(board);

    for(int r = 0; r < N; r++) for(int c = 0; c < N; c++) {
            lv_obj_t * cell = ui_box(board);
            lv_obj_set_size(cell, CELL, CELL);
            lv_obj_set_pos(cell, GAP + c * (CELL + GAP), GAP + r * (CELL + GAP));
            lv_obj_set_style_radius(cell, 8, 0);
            lv_obj_set_style_bg_opa(cell, LV_OPA_COVER, 0);
            s_num[r][c] = ui_label(cell, "", &lv_font_montserrat_28, 0xFFFFFF);
            lv_obj_center(s_num[r][c]);
            s_cell[r][c] = cell;
        }

    s_status = ui_label(body, "", &lv_font_montserrat_14, UI_DIM);
    lv_obj_t * nb = ui_button(body, "New Game", 0x8F7A66, new_cb, NULL);
    lv_obj_set_size(nb, 140, 40);
#ifdef S3_COMPACT
    lv_obj_set_size(nb, 100, 24);
#endif
    refresh();
}

const app_desc_t app_2048 = { "2048", LV_SYMBOL_SHUFFLE, 0xEDC22E, create };
