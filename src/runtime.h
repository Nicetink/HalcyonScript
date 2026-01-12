/*
 * HalcyonScript - Runtime
 */

#ifndef RUNTIME_H
#define RUNTIME_H

#include "ast.h"
#include "value.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#endif

typedef struct {
    char* name;
    char* type;
    HWND hwnd;
    int x, y, width, height;
} HcsGuiControl;

typedef struct {
    char* element_name;
    char* event_type;
    HcsAstNode* handler;
} HcsGuiHandler;

typedef struct {
    char* name;
    UINT_PTR id;
    int interval;
    bool running;
} HcsGuiTimer;

typedef struct HcsScope {
    struct { char* name; HcsValue* value; bool is_const; }* vars;
    int var_count, var_capacity;
    struct HcsScope* parent;
} HcsScope;

typedef struct {
    char* name;
    HcsAstNode* node;
} HcsFuncDef;

typedef struct {
    HcsGuiControl* controls;
    int control_count, control_capacity;
    HcsGuiHandler* handlers;
    int handler_count, handler_capacity;
    HcsGuiTimer* timers;
    int timer_count, timer_capacity;
    HWND main_window;
    bool running;
    HcsScope* global_scope;
    HcsScope* current_scope;
    HcsFuncDef* functions;
    int func_count, func_capacity;
    bool should_return, should_break, should_continue;
    HcsValue* return_value;
} HcsRuntime;

HcsRuntime* runtime_create(void);
void runtime_free(HcsRuntime* rt);
void runtime_execute(HcsRuntime* rt, HcsAstNode* program);
void execute_statement(HcsRuntime* rt, HcsAstNode* node);
HcsValue* runtime_eval(HcsRuntime* rt, HcsAstNode* node);

HcsScope* scope_create(HcsScope* parent);
void scope_free(HcsScope* scope);
void scope_set(HcsScope* scope, const char* name, HcsValue* value, bool is_const);
HcsValue* scope_get(HcsScope* scope, const char* name);

void gui_init(HcsRuntime* rt);
void gui_run(HcsRuntime* rt);
HcsGuiControl* gui_find_control(HcsRuntime* rt, const char* name);
void gui_fire_event(HcsRuntime* rt, const char* element, const char* event);
void gui_execute_program(HcsRuntime* rt, HcsAstNode* program);

#endif
