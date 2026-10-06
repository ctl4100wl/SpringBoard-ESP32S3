/**
 * @file fx.c
 */
#include <math.h>
#include "fx.h"

#define FX_POOL     256
#define FX_STEP_S   0.004f

typedef struct {
    lv_obj_t * obj;
    fx_done_cb_t done;
    float x, v, t, k, c;
    uint32_t delay;
    uint8_t prop;
    bool active;
} fx_t;

static fx_t s_fx[FX_POOL];
static lv_timer_t * s_timer;
static uint32_t s_tick;

static void apply(lv_obj_t * o, int prop, float x)
{
    int32_t v = (int32_t)lroundf(x);
    switch(prop) {
        case FX_X: lv_obj_set_style_translate_x(o, v, 0); break;
        case FX_Y: lv_obj_set_style_translate_y(o, v, 0); break;
        case FX_SCALE:
            lv_obj_set_style_transform_pivot_x(o, LV_PCT(50), 0);
            lv_obj_set_style_transform_pivot_y(o, LV_PCT(50), 0);
            lv_obj_set_style_transform_scale(o, v < 1 ? 1 : v, 0);
            break;
        case FX_OPA: lv_obj_set_style_opa(o, (lv_opa_t)LV_CLAMP(0, v, 255), 0); break;
        case FX_DW: lv_obj_set_style_transform_width(o, v, 0); break;
        case FX_DH: lv_obj_set_style_transform_height(o, v, 0); break;
        default: break;
    }
}

float fx_get(lv_obj_t * o, fx_prop_t prop)
{
    for(int i = 0; i < FX_POOL; i++) {
        if(s_fx[i].active && s_fx[i].obj == o && s_fx[i].prop == prop) return s_fx[i].x;
    }
    switch(prop) {
        case FX_X: return (float)lv_obj_get_style_translate_x(o, 0);
        case FX_Y: return (float)lv_obj_get_style_translate_y(o, 0);
        case FX_SCALE: return (float)lv_obj_get_style_transform_scale_x(o, 0);
        case FX_OPA: return (float)lv_obj_get_style_opa(o, 0);
        case FX_DW: return (float)lv_obj_get_style_transform_width(o, 0);
        case FX_DH: return (float)lv_obj_get_style_transform_height(o, 0);
        default: return 0;
    }
}

static fx_t * find(lv_obj_t * o, int prop)
{
    for(int i = 0; i < FX_POOL; i++) {
        if(s_fx[i].active && s_fx[i].obj == o && s_fx[i].prop == prop) return &s_fx[i];
    }
    return NULL;
}

static bool obj_has_any(lv_obj_t * o)
{
    for(int i = 0; i < FX_POOL; i++) if(s_fx[i].active && s_fx[i].obj == o) return true;
    return false;
}

static void obj_deleted_cb(lv_event_t * e)
{
    lv_obj_t * o = lv_event_get_current_target_obj(e);
    for(int i = 0; i < FX_POOL; i++) if(s_fx[i].obj == o) s_fx[i].active = false;
}

static void release(fx_t * f)
{
    f->active = false;
    if(!obj_has_any(f->obj)) lv_obj_remove_event_cb_with_user_data(f->obj, obj_deleted_cb, NULL);
}

static void timer_cb(lv_timer_t * t)
{
    LV_UNUSED(t);
    float dt = LV_MIN(lv_tick_elaps(s_tick), 60) / 1000.0f;
    uint32_t ms = lv_tick_elaps(s_tick);
    s_tick = lv_tick_get();
    bool any = false;

    for(int i = 0; i < FX_POOL; i++) {
        fx_t * f = &s_fx[i];
        if(!f->active) continue;
        any = true;
        if(f->delay) {
            f->delay = f->delay > ms ? f->delay - ms : 0;
            continue;
        }
        float rest = dt;
        while(rest > 0.0f) {
            float h = rest < FX_STEP_S ? rest : FX_STEP_S;
            f->v += (-f->k * (f->x - f->t) - f->c * f->v) * h;
            f->x += f->v * h;
            rest -= h;
        }
        float tol = f->prop == FX_SCALE ? 0.6f : 0.4f;
        if(fabsf(f->x - f->t) < tol && fabsf(f->v) < 6.0f) {
            f->x = f->t;
            apply(f->obj, f->prop, f->x);
            lv_obj_t * o = f->obj;
            fx_done_cb_t done = f->done;
            release(f);
            if(done) done(o);   /* may start new springs or delete o */
            continue;
        }
        apply(f->obj, f->prop, f->x);
    }
    if(!any) lv_timer_pause(s_timer);
}

static fx_t * get_or_alloc(lv_obj_t * o, int prop)
{
    fx_t * f = find(o, prop);
    if(f) return f;
    for(int i = 0; i < FX_POOL; i++) {
        if(s_fx[i].active) continue;
        f = &s_fx[i];
        if(!obj_has_any(o)) lv_obj_add_event_cb(o, obj_deleted_cb, LV_EVENT_DELETE, NULL);
        lv_memzero(f, sizeof(*f));
        f->obj = o;
        f->prop = (uint8_t)prop;
        f->x = fx_get(o, prop);
        f->active = true;
        return f;
    }
    return NULL;
}

void fx_set(lv_obj_t * o, fx_prop_t prop, float value)
{
    fx_t * f = find(o, prop);
    if(f) release(f);
    apply(o, prop, value);
}

void fx_to(lv_obj_t * o, fx_prop_t prop, float target, float response, float damping, uint32_t delay_ms,
           fx_done_cb_t done)
{
    if(!s_timer) {
        s_timer = lv_timer_create(timer_cb, 8, NULL);
        s_tick = lv_tick_get();
    }
    fx_t * f = get_or_alloc(o, prop);
    if(!f) {        /* pool exhausted: jump */
        apply(o, prop, target);
        if(done) done(o);
        return;
    }
    float w = 2.0f * 3.14159265f / response;
    f->k = w * w;
    f->c = 2.0f * damping * w;
    f->t = target;
    f->delay = delay_ms;
    f->done = done;
    if(lv_timer_get_paused(s_timer)) s_tick = lv_tick_get();
    lv_timer_resume(s_timer);
}

void fx_kick(lv_obj_t * o, fx_prop_t prop, float velocity)
{
    fx_t * f = find(o, prop);
    if(f) f->v += velocity;
}

bool fx_busy(lv_obj_t * o, fx_prop_t prop)
{
    return find(o, prop) != NULL;
}

void fx_stop(lv_obj_t * o)
{
    for(int i = 0; i < FX_POOL; i++) if(s_fx[i].active && s_fx[i].obj == o) release(&s_fx[i]);
}

bool fx_any_busy(void)
{
    for(int i=0;i<FX_POOL;i++) if(s_fx[i].active) return true;
    return false;
}
