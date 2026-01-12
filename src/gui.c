/*
 * HalcyonScript - Win32 GUI + HalGUI Support
 */

#include "runtime.h"
#include <commctrl.h>
#include <stdlib.h>

static HcsRuntime* g_runtime = NULL;
static int g_ctrl_id = 1000;
static bool g_use_halgui_mode = false;

// Check if HalGUI mode is active
bool gui_is_halgui_mode(void) {
    return g_use_halgui_mode;
}

// HalGUI runtime functions
extern void halgui_runtime_init(HcsRuntime* rt);
extern void halgui_runtime_shutdown(void);
extern void halgui_create_window(HcsRuntime* rt, HcsAstNode* node);
extern void halgui_create_control(HcsRuntime* rt, HcsAstNode* node);
extern void halgui_set_property(HcsRuntime* rt, HcsAstNode* node);
extern void halgui_get_property(HcsRuntime* rt, HcsAstNode* node);
extern void halgui_register_handler(HcsAstNode* node);
extern void halgui_run(void);
extern void halgui_set_theme(const char* theme_name);

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE:
            if (g_runtime) gui_fire_event(g_runtime, "main", "started");
            return 0;
        case WM_COMMAND: {
            int id = LOWORD(wParam), code = HIWORD(wParam);
            if (g_runtime) {
                for (int i = 0; i < g_runtime->control_count; i++) {
                    if (GetDlgCtrlID(g_runtime->controls[i].hwnd) == id) {
                        const char* n = g_runtime->controls[i].name;
                        const char* t = g_runtime->controls[i].type;
                        if (code == BN_CLICKED) gui_fire_event(g_runtime, n, "clicked");
                        else if (code == EN_CHANGE && strcmp(t, "input") == 0) gui_fire_event(g_runtime, n, "changed");
                        break;
                    }
                }
            }
            return 0;
        }
        case WM_TIMER:
            if (g_runtime) {
                for (int i = 0; i < g_runtime->timer_count; i++) {
                    if (g_runtime->timers[i].id == wParam) {
                        gui_fire_event(g_runtime, g_runtime->timers[i].name, "tick");
                        break;
                    }
                }
            }
            return 0;
        case WM_CLOSE:
            if (g_runtime) gui_fire_event(g_runtime, "main", "closed");
            DestroyWindow(hwnd);
            return 0;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

void gui_init(HcsRuntime* rt) {
    g_runtime = rt;
    INITCOMMONCONTROLSEX icex = {sizeof(INITCOMMONCONTROLSEX), ICC_WIN95_CLASSES | ICC_BAR_CLASSES};
    InitCommonControlsEx(&icex);
}

static void create_window_node(HcsRuntime* rt, HcsAstNode* node) {
    if (node->type != HCS_AST_CREATE_WINDOW) return;
    const char* name = node->data.create_window.name;
    const char* title = node->data.create_window.title ? node->data.create_window.title : "HalcyonScript";
    int w = node->data.create_window.width, h = node->data.create_window.height;
    
    WNDCLASSEXA wc = {sizeof(WNDCLASSEXA), CS_HREDRAW | CS_VREDRAW, WndProc, 0, 0,
        GetModuleHandle(NULL), NULL, LoadCursor(NULL, IDC_ARROW), (HBRUSH)(COLOR_WINDOW + 1), NULL, "HcsWnd", NULL};
    RegisterClassExA(&wc);
    
    HWND hwnd = CreateWindowExA(0, "HcsWnd", title, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, w, h, NULL, NULL, GetModuleHandle(NULL), NULL);
    rt->main_window = hwnd;
    
    if (rt->control_count >= rt->control_capacity) {
        int nc = rt->control_capacity == 0 ? 32 : rt->control_capacity * 2;
        rt->controls = realloc(rt->controls, sizeof(HcsGuiControl) * nc);
        rt->control_capacity = nc;
    }
    rt->controls[rt->control_count].name = strdup(name);
    rt->controls[rt->control_count].type = strdup("window");
    rt->controls[rt->control_count].hwnd = hwnd;
    rt->control_count++;
}

static void create_control_node(HcsRuntime* rt, HcsAstNode* node) {
    if (node->type != HCS_AST_CREATE_CONTROL || !rt->main_window) return;
    const char* type = node->data.create_control.control_type;
    const char* name = node->data.create_control.name;
    const char* text = node->data.create_control.text ? node->data.create_control.text : "";
    HcsPropertyList* props = &node->data.create_control.properties;
    
    int x = 10, y = 10, w = 100, h = 25;
    for (int i = 0; i < props->count; i++) {
        const char* pn = props->items[i].name;
        HcsAstNode* pv = props->items[i].value;
        if (!pv || pv->type != HCS_AST_NUMBER) continue;
        int val = (int)pv->data.number_value;
        if (strcmp(pn, "x") == 0) x = val;
        else if (strcmp(pn, "y") == 0) y = val;
        else if (strcmp(pn, "width") == 0) w = val;
        else if (strcmp(pn, "height") == 0) h = val;
    }
    
    HWND hwnd = NULL;
    DWORD style = WS_CHILD | WS_VISIBLE;
    const char* cls = NULL;
    
    if (strcmp(type, "button") == 0) { cls = "BUTTON"; style |= BS_PUSHBUTTON; }
    else if (strcmp(type, "label") == 0) { cls = "STATIC"; style |= SS_LEFT; }
    else if (strcmp(type, "input") == 0) { cls = "EDIT"; style |= WS_BORDER | ES_AUTOHSCROLL; }
    else if (strcmp(type, "textarea") == 0) { cls = "EDIT"; style |= WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL; }
    else if (strcmp(type, "checkbox") == 0) { cls = "BUTTON"; style |= BS_AUTOCHECKBOX; }
    else if (strcmp(type, "listbox") == 0) { cls = "LISTBOX"; style |= WS_BORDER | WS_VSCROLL | LBS_NOTIFY; }
    else if (strcmp(type, "dropdown") == 0) { cls = "COMBOBOX"; style |= CBS_DROPDOWNLIST | WS_VSCROLL; h = 200; }
    else if (strcmp(type, "slider") == 0) { cls = TRACKBAR_CLASSA; style |= TBS_HORZ; }
    else if (strcmp(type, "progress") == 0) { cls = PROGRESS_CLASSA; }
    else if (strcmp(type, "panel") == 0) { cls = "STATIC"; style |= SS_ETCHEDFRAME; }
    
    if (cls) {
        hwnd = CreateWindowExA(0, cls, text, style, x, y, w, h, rt->main_window, (HMENU)(intptr_t)(g_ctrl_id++), GetModuleHandle(NULL), NULL);
        SendMessage(hwnd, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
    }
    
    if (hwnd) {
        if (rt->control_count >= rt->control_capacity) {
            int nc = rt->control_capacity == 0 ? 32 : rt->control_capacity * 2;
            rt->controls = realloc(rt->controls, sizeof(HcsGuiControl) * nc);
            rt->control_capacity = nc;
        }
        rt->controls[rt->control_count].name = strdup(name);
        rt->controls[rt->control_count].type = strdup(type);
        rt->controls[rt->control_count].hwnd = hwnd;
        rt->controls[rt->control_count].x = x;
        rt->controls[rt->control_count].y = y;
        rt->controls[rt->control_count].width = w;
        rt->controls[rt->control_count].height = h;
        rt->control_count++;
    }
}

static void create_timer_node(HcsRuntime* rt, HcsAstNode* node) {
    if (node->type != HCS_AST_CREATE_TIMER || !rt->main_window) return;
    const char* name = node->data.create_timer.name;
    HcsValue* iv = runtime_eval(rt, node->data.create_timer.interval);
    int interval = (int)value_to_number(iv);
    value_release(iv);
    
    if (rt->timer_count >= rt->timer_capacity) {
        int nc = rt->timer_capacity == 0 ? 16 : rt->timer_capacity * 2;
        rt->timers = realloc(rt->timers, sizeof(HcsGuiTimer) * nc);
        rt->timer_capacity = nc;
    }
    UINT_PTR tid = rt->timer_count + 1;
    rt->timers[rt->timer_count].name = strdup(name);
    rt->timers[rt->timer_count].id = tid;
    rt->timers[rt->timer_count].interval = interval;
    rt->timers[rt->timer_count].running = false;
    rt->timer_count++;
    
    if (node->data.create_timer.auto_start) {
        SetTimer(rt->main_window, tid, interval, NULL);
        rt->timers[rt->timer_count - 1].running = true;
    }
}

void gui_run(HcsRuntime* rt) {
    if (!rt->main_window) return;
    ShowWindow(rt->main_window, SW_SHOW);
    UpdateWindow(rt->main_window);
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    rt->running = false;
}

void gui_execute_program(HcsRuntime* rt, HcsAstNode* program) {
    if (!program || program->type != HCS_AST_PROGRAM) return;
    
    // Check if program uses HalGUI (look for HalGUI.init() call or @halgui directive)
    for (int i = 0; i < program->data.program.statements.count; i++) {
        HcsAstNode* s = program->data.program.statements.items[i];
        if (s->type == HCS_AST_FUNC_CALL && s->data.func_call.name) {
            if (strcmp(s->data.func_call.name, "HalGUI.init") == 0) {
                g_use_halgui_mode = true;
                break;
            }
        }
    }
    
    if (g_use_halgui_mode) {
        // Use HalGUI mode
        halgui_runtime_init(rt);
        
        // Forward declaration
        extern void execute_statement(HcsRuntime* rt, HcsAstNode* node);
        
        for (int i = 0; i < program->data.program.statements.count; i++) {
            HcsAstNode* s = program->data.program.statements.items[i];
            switch (s->type) {
                case HCS_AST_CREATE_WINDOW:
                    halgui_create_window(rt, s);
                    break;
                case HCS_AST_CREATE_CONTROL:
                    halgui_create_control(rt, s);
                    break;
                case HCS_AST_EVENT_HANDLER:
                    halgui_register_handler(s);
                    break;
                case HCS_AST_SET_PROPERTY:
                    halgui_set_property(rt, s);
                    break;
                case HCS_AST_GET_PROPERTY:
                    halgui_get_property(rt, s);
                    break;
                default:
                    execute_statement(rt, s);
                    break;
            }
        }
        
        halgui_run();
        halgui_runtime_shutdown();
        
        // Exit process after HalGUI shutdown to ensure clean termination
        exit(0);
    } else {
        // Use Win32 mode (legacy)
        gui_init(rt);
        
        for (int i = 0; i < program->data.program.statements.count; i++) {
            HcsAstNode* s = program->data.program.statements.items[i];
            switch (s->type) {
                case HCS_AST_CREATE_WINDOW: create_window_node(rt, s); break;
                case HCS_AST_CREATE_CONTROL: create_control_node(rt, s); break;
                case HCS_AST_CREATE_TIMER: create_timer_node(rt, s); break;
                case HCS_AST_EVENT_HANDLER:
                    if (rt->handler_count >= rt->handler_capacity) {
                        int nc = rt->handler_capacity == 0 ? 32 : rt->handler_capacity * 2;
                        rt->handlers = realloc(rt->handlers, sizeof(HcsGuiHandler) * nc);
                        rt->handler_capacity = nc;
                    }
                    rt->handlers[rt->handler_count].element_name = strdup(s->data.event_handler.element_name);
                    rt->handlers[rt->handler_count].event_type = strdup(s->data.event_handler.event_type);
                    rt->handlers[rt->handler_count].handler = s;
                    rt->handler_count++;
                    break;
                case HCS_AST_TIMER_ACTION: {
                    const char* act = s->data.timer_action.action;
                    const char* tn = s->data.timer_action.timer_name;
                    for (int j = 0; j < rt->timer_count; j++) {
                        if (strcmp(rt->timers[j].name, tn) == 0) {
                            if (strcmp(act, "start") == 0 && !rt->timers[j].running) {
                                SetTimer(rt->main_window, rt->timers[j].id, rt->timers[j].interval, NULL);
                                rt->timers[j].running = true;
                            } else if (strcmp(act, "stop") == 0 && rt->timers[j].running) {
                                KillTimer(rt->main_window, rt->timers[j].id);
                                rt->timers[j].running = false;
                            }
                            break;
                        }
                    }
                    break;
                }
                default: runtime_execute(rt, s); break;
            }
        }
        
        if (rt->main_window) gui_run(rt);
    }
}
