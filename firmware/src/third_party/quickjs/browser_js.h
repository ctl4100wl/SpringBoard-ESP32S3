#ifndef ESPRING_BROWSER_JS_H
#define ESPRING_BROWSER_JS_H

#include "quickjs.h"

typedef struct js_State js_State;
typedef void (*js_CFunction)(js_State * J);
typedef void (*js_Report)(js_State * J, const char * message);

js_State * js_newstate(void * alloc, void * userdata, int flags);
void js_freestate(js_State * J);
void js_setcontext(js_State * J, void * context);
void * js_getcontext(js_State * J);
void js_setreport(js_State * J, js_Report report);
void js_newcfunction(js_State * J, js_CFunction func, const char * name, int nargs);
void js_setglobal(js_State * J, const char * name);
void js_getglobal(js_State * J, const char * name);
void js_getproperty(js_State * J, int index, const char * name);
void js_getindex(js_State * J, int index, int key);
int js_getlength(js_State * J, int index);
int js_isarray(js_State * J, int index);
void js_pushstring(js_State * J, const char * value);
void js_pushundefined(js_State * J);
const char * js_tostring(js_State * J, int index);
void js_pop(js_State * J, int n);
int js_pcall(js_State * J, int nargs);
int js_dostring(js_State * J, const char * source);

#endif
