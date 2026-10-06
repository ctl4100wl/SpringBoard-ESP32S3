/** On-device app collection. Restores apps removed from the Home Screen. */
#include "../os/sdk.h"

extern const app_desc_t app_store;

static lv_obj_t * s_list;

static void store_refresh(void);

static void store_action(lv_event_t * e)
{
    int id = (int)(intptr_t)lv_event_get_user_data(e);
    if(home_app_installed(id)) {
        shell_open_app(id);
        return;
    }
    if(!home_app_install(id)) sdk_toast("App Store", "There isn't room on the Home Screen.");
    store_refresh();
}

static void store_refresh(void)
{
    if(!s_list) return;
    lv_obj_clean(s_list);
    for(int id = 0; id < os_app_count; id++) {
        if(os_apps[id]->create == app_store.create) continue;
        lv_obj_t * row = ui_card(s_list);
        lv_obj_set_width(row, LV_PCT(100));
        lv_obj_set_height(row, 70);
        lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_hor(row, 12, 0);
        lv_obj_t * icon = ui_box(row);
        lv_obj_set_size(icon, 42, 42);
        lv_obj_set_style_radius(icon, 10, 0);
        ui_icon_style(icon, os_apps[id]->color);
        lv_obj_center(ui_label(icon, os_apps[id]->symbol, &lv_font_montserrat_18, 0xFFFFFF));
        lv_obj_t * name = ui_label(row, os_apps[id]->name, &lv_font_montserrat_14, 0xFFFFFF);
        lv_obj_set_flex_grow(name, 1);
        lv_obj_t * button = ui_button(row, home_app_installed(id) ? "OPEN" : "GET", UI_BLUE,
                                      store_action, (void *)(intptr_t)id);
        lv_obj_set_width(button, 62);
    }
}

static void store_deleted(lv_event_t * e)
{
    LV_UNUSED(e);
    s_list = NULL;
}

APP(app_store, "App Store", LV_SYMBOL_DOWNLOAD, 0x0A84FF)
{
    lv_obj_t * body = ui_app_scaffold(root, "App Store");
    lv_obj_add_event_cb(root, store_deleted, LV_EVENT_DELETE, NULL);
    ui_label(body, "Apps on this device", &lv_font_montserrat_18, 0xFFFFFF);
    ui_label(body, "Get an app to add it to your Home Screen.", &lv_font_montserrat_12, UI_DIM);
    s_list = ui_box(body);
    lv_obj_set_width(s_list, LV_PCT(100));
    lv_obj_set_height(s_list, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(s_list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(s_list, 8, 0);
    store_refresh();
}
