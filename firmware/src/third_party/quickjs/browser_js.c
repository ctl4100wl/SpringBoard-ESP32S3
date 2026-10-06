#include <stdlib.h>
#include <string.h>
#include "browser_js.h"
#ifdef ESP_PLATFORM
#include "esp_heap_caps.h"
#include "esp_timer.h"
static size_t qjs_size(const void * p) { return p ? heap_caps_get_allocated_size((void *)p) : 0; }
static void * qjs_malloc(JSMallocState * s, size_t size) {
    if(s->malloc_size >= s->malloc_limit || size > s->malloc_limit - s->malloc_size) return NULL;
    void * p = heap_caps_malloc(size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if(p) { s->malloc_count++; s->malloc_size += qjs_size(p); } return p;
}
static void qjs_free(JSMallocState * s, void * p) {
    if(p) { s->malloc_size -= qjs_size(p); s->malloc_count--; heap_caps_free(p); }
}
static void * qjs_realloc(JSMallocState * s, void * p, size_t size) {
    if(!p) return qjs_malloc(s, size);
    if(!size) { qjs_free(s, p); return NULL; }
    size_t old = qjs_size(p);
    if(size > s->malloc_limit - (s->malloc_size - old)) return NULL;
    void * replacement = heap_caps_realloc(p, size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if(replacement) s->malloc_size += qjs_size(replacement) - old; return replacement;
}
#endif

#define STACK_MAX 32
#define FUNC_MAX 8

struct js_State {
    JSRuntime * rt;
    JSContext * ctx;
    JSValue stack[STACK_MAX];
    int sp;
    js_CFunction func[FUNC_MAX];
    int func_n;
    void * user;
    js_Report report;
    JSValueConst * args;
    int argc;
    const char * temp;
#ifdef ESP_PLATFORM
    int64_t deadline;
#endif
};
#ifdef ESP_PLATFORM
static int qjs_interrupt(JSRuntime * rt, void * opaque) { (void)rt; return esp_timer_get_time() > ((js_State *)opaque)->deadline; }
#endif

static void push(js_State * J, JSValue value)
{
    if(J->sp < STACK_MAX) J->stack[J->sp++] = value;
    else JS_FreeValue(J->ctx, value);
}

static JSValueConst value_at(js_State * J, int index)
{
    if(J->args && index > 0 && index <= J->argc) return J->args[index - 1];
    int pos = index < 0 ? J->sp + index : index - 1;
    return pos >= 0 && pos < J->sp ? J->stack[pos] : JS_UNDEFINED;
}

static void report_exception(js_State * J)
{
    JSValue ex = JS_GetException(J->ctx);
    const char * message = JS_ToCString(J->ctx, ex);
    if(J->report) J->report(J, message ? message : "JavaScript exception");
    if(message) JS_FreeCString(J->ctx, message);
    JS_FreeValue(J->ctx, ex);
}

static JSValue dispatch(JSContext * ctx, JSValueConst self, int argc, JSValueConst * argv, int magic)
{
    (void)self;
    js_State * J = JS_GetContextOpaque(ctx);
    if(!J || magic < 0 || magic >= J->func_n) return JS_UNDEFINED;
    J->args = argv;
    J->argc = argc;
    int start = J->sp;
    J->func[magic](J);
    JSValue result = J->sp > start ? JS_DupValue(ctx, J->stack[J->sp - 1]) : JS_UNDEFINED;
    while(J->sp > start) JS_FreeValue(ctx, J->stack[--J->sp]);
    J->args = NULL;
    J->argc = 0;
    return result;
}

js_State * js_newstate(void * alloc, void * userdata, int flags)
{
    (void)alloc; (void)userdata; (void)flags;
    js_State * J = calloc(1, sizeof(*J));
    if(!J) return NULL;
#ifdef ESP_PLATFORM
    const JSMallocFunctions functions = { qjs_malloc, qjs_free, qjs_realloc, qjs_size };
    J->rt = JS_NewRuntime2(&functions, NULL);
#else
    J->rt = JS_NewRuntime();
#endif
    if(J->rt) {
#ifdef ESP_PLATFORM
        JS_SetMemoryLimit(J->rt, 1536 * 1024);
        JS_SetMaxStackSize(J->rt, 24 * 1024);
        J->deadline = esp_timer_get_time() + 5000000;
        JS_SetInterruptHandler(J->rt, qjs_interrupt, J);
#else
        JS_SetMemoryLimit(J->rt, 64 * 1024 * 1024);
        JS_SetMaxStackSize(J->rt, 1024 * 1024);
#endif
        J->ctx = JS_NewContext(J->rt);
    }
    if(!J->ctx) { js_freestate(J); return NULL; }
    JS_SetContextOpaque(J->ctx, J);
    return J;
}

void js_freestate(js_State * J)
{
    if(!J) return;
    if(J->ctx) {
        if(J->temp) JS_FreeCString(J->ctx, J->temp);
        while(J->sp) JS_FreeValue(J->ctx, J->stack[--J->sp]);
        JS_FreeContext(J->ctx);
    }
    if(J->rt) JS_FreeRuntime(J->rt);
    free(J);
}

void js_setcontext(js_State * J, void * context) { J->user = context; }
void * js_getcontext(js_State * J) { return J->user; }
void js_setreport(js_State * J, js_Report report) { J->report = report; }

void js_newcfunction(js_State * J, js_CFunction func, const char * name, int nargs)
{
    if(J->func_n >= FUNC_MAX) { push(J, JS_UNDEFINED); return; }
    int magic = J->func_n;
    J->func[J->func_n++] = func;
    push(J, JS_NewCFunctionMagic(J->ctx, dispatch, name, nargs, JS_CFUNC_generic_magic, magic));
}

void js_setglobal(js_State * J, const char * name)
{
    if(!J->sp) return;
    JSValue global = JS_GetGlobalObject(J->ctx);
    JS_SetPropertyStr(J->ctx, global, name, J->stack[--J->sp]);
    JS_FreeValue(J->ctx, global);
}

void js_getglobal(js_State * J, const char * name)
{
    JSValue global = JS_GetGlobalObject(J->ctx);
    push(J, JS_GetPropertyStr(J->ctx, global, name));
    JS_FreeValue(J->ctx, global);
}

void js_getproperty(js_State * J, int index, const char * name)
{
    push(J, JS_GetPropertyStr(J->ctx, value_at(J, index), name));
}

void js_getindex(js_State * J, int index, int key)
{
    push(J, JS_GetPropertyUint32(J->ctx, value_at(J, index), (uint32_t)key));
}

int js_getlength(js_State * J, int index)
{
    JSValue length = JS_GetPropertyStr(J->ctx, value_at(J, index), "length");
    int32_t n = 0;
    JS_ToInt32(J->ctx, &n, length);
    JS_FreeValue(J->ctx, length);
    return n;
}

int js_isarray(js_State * J, int index) { return JS_IsArray(J->ctx, value_at(J, index)); }
void js_pushstring(js_State * J, const char * value) { push(J, JS_NewString(J->ctx, value)); }
void js_pushundefined(js_State * J) { push(J, JS_UNDEFINED); }

const char * js_tostring(js_State * J, int index)
{
    if(J->temp) JS_FreeCString(J->ctx, J->temp);
    J->temp = JS_ToCString(J->ctx, value_at(J, index));
    return J->temp ? J->temp : "";
}

void js_pop(js_State * J, int n)
{
    while(n-- > 0 && J->sp) JS_FreeValue(J->ctx, J->stack[--J->sp]);
}

int js_pcall(js_State * J, int nargs)
{
    if(J->sp < nargs + 2) return 1;
    int base = J->sp - nargs - 2;
    JSValue result = JS_Call(J->ctx, J->stack[base], J->stack[base + 1], nargs,
                             (JSValueConst *)(J->stack + base + 2));
    while(J->sp > base) JS_FreeValue(J->ctx, J->stack[--J->sp]);
    if(JS_IsException(result)) { report_exception(J); return 1; }
    push(J, result);
    return 0;
}

int js_dostring(js_State * J, const char * source)
{
#ifdef ESP_PLATFORM
    J->deadline = esp_timer_get_time() + 2000000;
#endif
    JSValue result = JS_Eval(J->ctx, source, strlen(source), "[page]", JS_EVAL_TYPE_GLOBAL);
    if(JS_IsException(result)) { report_exception(J); return 1; }
    JS_FreeValue(J->ctx, result);
    JSContext * pending;
    for(int i = 0; i < 100 && JS_IsJobPending(J->rt); i++) {
        if(JS_ExecutePendingJob(J->rt, &pending) < 0) { report_exception(J); return 1; }
    }
    return 0;
}
