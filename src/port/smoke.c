#include "hardware.h"
#include "src/os/sdk.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include <math.h>
#include <stdio.h>

static const char * TAG = "s3_test";
typedef enum { APP_OPEN, APP_WAIT, APP_HOLD, APP_CLOSE, APP_CLOSED,
    CC_OPEN, CC_WAIT, CC_CLOSE, CC_CLOSED, NOTIF_OPEN, NOTIF_WAIT, NOTIF_CLEAR,
    NOTIF_CLEAR_WAIT, NOTIF_CLOSE, NOTIF_CLOSED, PHYSICS, PHYSICS_RETARGET,
    PHYSICS_WAIT, RECENTS_OPEN, RECENTS_WAIT, RECENTS_CLEAR, RECENTS_CLEAR_WAIT, BLUR_TEST, BLUR_WAIT, LENS_TEST, LENS_WAIT, CYCLE_DONE } phase_t;
static struct {
    lv_timer_t * timer;
    phase_t phase;
    int id, cycle, passed, failed, skipped;
    uint32_t since, initial_heap, stage_flush;
    bool cancel;
    lv_obj_t * fixture;
} run;

bool s3_smoke_running(void) { return run.timer != NULL; }
void s3_smoke_cancel(void) { run.cancel = true; }
static void phase(phase_t next) { run.phase = next; run.since = lv_tick_get(); }
static void result(const char * stage, bool ok)
{
    if(ok) run.passed++; else run.failed++;
    ESP_LOGI(TAG, "%s %s elapsed=%ums heap=%u min=%u largest=%u flushes=%u", ok ? "PASS" : "FAIL", stage,
        (unsigned)lv_tick_elaps(run.since), (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
        (unsigned)heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL),
        (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL), s3_flush_count() - run.stage_flush);
    run.stage_flush = s3_flush_count();
}
static bool timed_out(void) { return lv_tick_elaps(run.since) > 4000; }
static void finish(void)
{
    s3_effect_test("off");
    shell_go_home_instant(); cc_close(); s3_notify_open(false);
    if(run.fixture) { fx_stop(run.fixture); lv_obj_delete(run.fixture); run.fixture = NULL; }
    ESP_LOGI(TAG, "%s: pass=%d fail=%d capability_skips=%d initial_heap=%u final_heap=%u",
        run.cancel ? "CANCELLED" : "COMPLETE", run.passed, run.failed, run.skipped,
        (unsigned)run.initial_heap, (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
    lv_timer_delete(run.timer); run.timer = NULL;
    s3_perf_report();
}
static void tick(lv_timer_t * timer)
{
    (void)timer;
    if(run.cancel) { finish(); return; }
    switch(run.phase) {
        case APP_OPEN:
            if(lv_tick_elaps(run.since) < 750) break;
            if(run.id >= os_app_count) { phase(CC_OPEN); break; }
            ESP_LOGI(TAG, "OPEN %d/%d %s", run.id + 1, os_app_count, os_apps[run.id]->name);
            char cmd[24]; snprintf(cmd, sizeof(cmd), "app %d", run.id); s3_command(cmd); phase(APP_WAIT); break;
        case APP_WAIT:
            if(!s3_shell_busy() || timed_out()) {
                result("app launch/settle", s3_app_active() && !s3_shell_busy());
                if(!s3_app_supported(run.id)) {
                    run.skipped++; ESP_LOGW(TAG, "SKIP %s hardware capability (UI launch only)", os_apps[run.id]->name);
                }
                phase(APP_HOLD);
            }
            break;
        case APP_HOLD: if(lv_tick_elaps(run.since) > 500) phase(APP_CLOSE); break;
        case APP_CLOSE: s3_command("home"); phase(APP_CLOSED); break;
        case APP_CLOSED:
            if(!s3_app_active() || timed_out()) {
                result("app close", !s3_app_active()); shell_go_home_instant();
                run.id++; phase(APP_OPEN);
            }
            break;
        case CC_OPEN: s3_command("cc open"); phase(CC_WAIT); break;
        case CC_WAIT:
            if(!s3_cc_busy() || timed_out()) { result("Control Center open", cc_visible() && !s3_cc_busy()); phase(CC_CLOSE); }
            break;
        case CC_CLOSE: s3_command("cc close"); phase(CC_CLOSED); break;
        case CC_CLOSED:
            if(!s3_cc_busy() || timed_out()) { result("Control Center close", !cc_visible() && !s3_cc_busy()); phase(NOTIF_OPEN); }
            break;
        case NOTIF_OPEN:
            s3_command("clear");
            notify_post(0, "Test 1", "Notification movement");
            notify_post(0, "Test 2", "Final card fade");
            s3_command("shade open"); phase(NOTIF_WAIT); break;
        case NOTIF_WAIT:
            if(!s3_shade_busy() || timed_out()) { result("Notification shade open", lock_visible() && notify_count() == 2 && !s3_shade_busy()); phase(NOTIF_CLEAR); }
            break;
        case NOTIF_CLEAR: s3_command("clear-one"); phase(NOTIF_CLEAR_WAIT); break;
        case NOTIF_CLEAR_WAIT:
            if((!s3_notify_busy() && lv_tick_elaps(run.since) > 750) || timed_out()) {
                bool stuck = timed_out() && s3_notify_busy();
                result("Notification fade/reflow", !s3_notify_busy());
                phase(notify_count() ? NOTIF_CLEAR : NOTIF_CLOSE);
                if(stuck) phase(NOTIF_CLOSE);
            }
            break;
        case NOTIF_CLOSE: s3_command("shade close"); phase(NOTIF_CLOSED); break;
        case NOTIF_CLOSED:
            if(!s3_shade_busy() || timed_out()) { result("Notification shade close", !lock_visible() && !s3_shade_busy() && notify_count() == 0); phase(PHYSICS); }
            break;
        case PHYSICS:
            run.fixture = ui_box(lv_layer_top());
            lv_obj_set_size(run.fixture, 48, 48); lv_obj_set_pos(run.fixture, 30, 120);
            lv_obj_set_style_bg_opa(run.fixture, LV_OPA_COVER, 0);
            lv_obj_set_style_bg_color(run.fixture, lv_color_hex(UI_BLUE), 0);
            fx_to(run.fixture, FX_X, 270, FX_BOUNCY, 0, NULL);
            phase(PHYSICS_RETARGET); break;
        case PHYSICS_RETARGET:
            if(lv_tick_elaps(run.since) > 120) {
                fx_to(run.fixture, FX_X, 90, FX_SNAPPY, 0, NULL);
                fx_kick(run.fixture, FX_X, -240);
                fx_to(run.fixture, FX_OPA, 80, 0.35f, 1.0f, 0, NULL);
                phase(PHYSICS_WAIT);
            }
            break;
        case PHYSICS_WAIT:
            if((!fx_busy(run.fixture, FX_X) && !fx_busy(run.fixture, FX_OPA)) || timed_out()) {
                result("Physics retarget/kick/opacity", fabsf(fx_get(run.fixture, FX_X) - 90) < 1 &&
                    fabsf(fx_get(run.fixture, FX_OPA) - 80) < 1 && !fx_busy(run.fixture, FX_X));
                fx_stop(run.fixture); lv_obj_delete(run.fixture); run.fixture = NULL;
                phase(RECENTS_OPEN);
            }
            break;
        case RECENTS_OPEN:
            s3_command("recents pull"); phase(RECENTS_WAIT); break;
        case RECENTS_WAIT:
            if(lv_tick_elaps(run.since) < 1200) break;
            result("Recents pull-up gesture", s3_recents_visible()); phase(RECENTS_CLEAR); break;
        case RECENTS_CLEAR:
            if(!s3_recents_visible() || !s3_recents_count()) { s3_command("home"); phase(BLUR_TEST); break; }
            run.id = s3_recents_count(); s3_command("recents clear-one"); phase(RECENTS_CLEAR_WAIT); break;
        case RECENTS_CLEAR_WAIT:
            if(lv_tick_elaps(run.since) < 700) break;
            result("Recents card clear/collapse", s3_recents_count() < run.id);
            if(s3_recents_count() >= run.id) { s3_command("home"); phase(BLUR_TEST); }
            else phase(RECENTS_CLEAR);
            break;
        case BLUR_TEST: s3_command("effects test blur"); run.stage_flush = s3_flush_count(); phase(BLUR_WAIT); break;
        case BLUR_WAIT:
            if(lv_tick_elaps(run.since) < 3000) break;
            result("Blur render test", s3_flush_count() - run.stage_flush > 0);
            s3_command("perf"); s3_command("effects test off"); phase(LENS_TEST); break;
        case LENS_TEST: s3_command("effects test lens"); run.stage_flush = s3_flush_count(); phase(LENS_WAIT); break;
        case LENS_WAIT:
            if(lv_tick_elaps(run.since) < 3000) break;
            result("Lens render test", s3_flush_count() - run.stage_flush > 0);
            s3_command("perf"); s3_command("effects test off"); phase(CYCLE_DONE); break;
        case CYCLE_DONE:
            if(lv_tick_elaps(run.since) < 500) break;
            ESP_LOGI(TAG, "Cycle %d settled heap=%u", run.cycle + 1, (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
            if(++run.cycle == 3) finish(); else { run.id = 0; phase(APP_OPEN); }
            break;
    }
}
void s3_smoke_start(void)
{
    if(run.timer) return;
    s3_command("unlock");
    shell_go_home_instant();
    run = (typeof(run)){0};
    run.initial_heap = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
    result("SIMD fill/copy/guards/fallback", s3_simd_self_test());
    run.stage_flush = s3_flush_count();
    phase(APP_OPEN);
    run.timer = lv_timer_create(tick, 20, NULL);
    run.since = lv_tick_get();
    ESP_LOGI(TAG, "START three app/CC/notification/physics cycles; BOOT again cancels");
}
