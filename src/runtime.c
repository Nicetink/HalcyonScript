/*
 * HalcyonScript - Runtime implementation
 */

#include "runtime.h"
#include "halgui_runtime.h"
#include <stdio.h>
#include <math.h>
#include <windows.h>

/* Helper function to convert UTF-8 string to Wide string for Windows API */
static wchar_t* utf8_to_wide(const char* utf8) {
    if (!utf8) return NULL;
    int len = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, NULL, 0);
    if (len == 0) return NULL;
    wchar_t* wide = (wchar_t*)malloc(len * sizeof(wchar_t));
    if (!wide) return NULL;
    MultiByteToWideChar(CP_UTF8, 0, utf8, -1, wide, len);
    return wide;
}

/* Helper function to show MessageBox with UTF-8 text */
static int msgbox_utf8(const char* text, const char* title, UINT type) {
    wchar_t* wtext = utf8_to_wide(text);
    wchar_t* wtitle = utf8_to_wide(title);
    int result = MessageBoxW(NULL, wtext ? wtext : L"", wtitle ? wtitle : L"HalcyonScript", type);
    free(wtext);
    free(wtitle);
    return result;
}

// Forward declarations for GPU runtime
extern void halgui_gpu_init(HcsRuntime* rt, const char* title, int width, int height);
extern void halgui_gpu_shutdown(void);
extern void halgui_gpu_run(HcsRuntime* rt, HcsAstNode* render_callback);
extern HcsValue* halgui_gpu_draw_rect_fn(HcsRuntime* rt, HcsAstList* args);
extern HcsValue* halgui_gpu_draw_rounded_rect_fn(HcsRuntime* rt, HcsAstList* args);
extern HcsValue* halgui_gpu_draw_circle_fn(HcsRuntime* rt, HcsAstList* args);
extern HcsValue* halgui_gpu_draw_line_fn(HcsRuntime* rt, HcsAstList* args);
extern HcsValue* halgui_gpu_draw_gradient_fn(HcsRuntime* rt, HcsAstList* args);
extern HcsValue* halgui_gpu_draw_shadow_fn(HcsRuntime* rt, HcsAstList* args);
extern HcsValue* halgui_gpu_draw_text_fn(HcsRuntime* rt, HcsAstList* args);
extern HcsValue* halgui_gpu_get_width_fn(HcsRuntime* rt, HcsAstList* args);
extern HcsValue* halgui_gpu_get_height_fn(HcsRuntime* rt, HcsAstList* args);
extern HcsValue* halgui_gpu_set_vsync_fn(HcsRuntime* rt, HcsAstList* args);
extern HcsValue* halgui_gpu_rgb_fn(HcsRuntime* rt, HcsAstList* args);

// Forward declarations for HalGUI runtime
extern void halgui_runtime_init(HcsRuntime* rt);
extern void halgui_runtime_shutdown(void);
extern void halgui_create_window(HcsRuntime* rt, HcsAstNode* node);
extern void halgui_create_control(HcsRuntime* rt, HcsAstNode* node);
extern void halgui_set_property(HcsRuntime* rt, HcsAstNode* node);
extern void halgui_get_property(HcsRuntime* rt, HcsAstNode* node);
extern void halgui_register_handler(HcsAstNode* node);
extern void halgui_run(void);
extern void halgui_set_theme(const char* theme_name);
extern HcsValue* halgui_dialog_message(HcsRuntime* rt, const char* title, const char* message, int buttons, int icon);
extern HcsValue* halgui_dialog_open_file(HcsRuntime* rt, const char* title, const char* filter);
extern HcsValue* halgui_dialog_save_file(HcsRuntime* rt, const char* title, const char* filter, const char* default_name);
extern HcsValue* halgui_dialog_select_folder(HcsRuntime* rt, const char* title);

// Forward declarations for Audio runtime
extern HcsValue* halgui_audio_create(HcsRuntime* rt, const char* name);
extern HcsValue* halgui_audio_load(HcsRuntime* rt, const char* name, const char* filePath);
extern HcsValue* halgui_audio_play(HcsRuntime* rt, const char* name);
extern HcsValue* halgui_audio_pause(HcsRuntime* rt, const char* name);
extern HcsValue* halgui_audio_stop(HcsRuntime* rt, const char* name);
extern HcsValue* halgui_audio_resume(HcsRuntime* rt, const char* name);
extern HcsValue* halgui_audio_seek(HcsRuntime* rt, const char* name, int positionMs);
extern HcsValue* halgui_audio_set_volume(HcsRuntime* rt, const char* name, int volume);
extern HcsValue* halgui_audio_get_volume(HcsRuntime* rt, const char* name);
extern HcsValue* halgui_audio_get_position(HcsRuntime* rt, const char* name);
extern HcsValue* halgui_audio_get_duration(HcsRuntime* rt, const char* name);
extern HcsValue* halgui_audio_get_state(HcsRuntime* rt, const char* name);
extern HcsValue* halgui_audio_set_loop(HcsRuntime* rt, const char* name, bool loop);
extern HcsValue* halgui_audio_set_mute(HcsRuntime* rt, const char* name, bool mute);
extern HcsValue* halgui_audio_is_muted(HcsRuntime* rt, const char* name);
extern HcsValue* halgui_audio_destroy(HcsRuntime* rt, const char* name);

// Forward declarations for HalForms runtime
extern void halforms_runtime_init(HcsRuntime* rt);
extern void halforms_runtime_shutdown(void);
extern void halforms_rt_create_form(HcsRuntime* rt, const char* name, const char* title, int width, int height, int style);
extern void halforms_rt_create_control(HcsRuntime* rt, const char* type, const char* name, const char* text, int x, int y, int w, int h);
extern void halforms_rt_set_property(HcsRuntime* rt, const char* element, const char* prop, HcsValue* val);
extern HcsValue* halforms_rt_get_property(HcsRuntime* rt, const char* element, const char* prop);
extern void halforms_rt_register_handler(HcsAstNode* node);
extern void halforms_rt_run(void);
extern void halforms_rt_add_item(const char* element, const char* item);
extern void halforms_rt_clear_items(const char* element);

// Forward declarations for Paint Canvas API
extern HcsValue* hcs_paintcanvas_create(HcsRuntime* rt, HcsValue** args, int argc);
extern HcsValue* hcs_paintcanvas_set_tool(HcsRuntime* rt, HcsValue** args, int argc);
extern HcsValue* hcs_paintcanvas_set_color(HcsRuntime* rt, HcsValue** args, int argc);
extern HcsValue* hcs_paintcanvas_set_brush_size(HcsRuntime* rt, HcsValue** args, int argc);
extern HcsValue* hcs_paintcanvas_set_brush_type(HcsRuntime* rt, HcsValue** args, int argc);
extern HcsValue* hcs_paintcanvas_clear(HcsRuntime* rt, HcsValue** args, int argc);
extern HcsValue* hcs_paintcanvas_undo(HcsRuntime* rt, HcsValue** args, int argc);
extern HcsValue* hcs_paintcanvas_redo(HcsRuntime* rt, HcsValue** args, int argc);
extern HcsValue* hcs_paintcanvas_save(HcsRuntime* rt, HcsValue** args, int argc);
extern HcsValue* hcs_paintcanvas_load(HcsRuntime* rt, HcsValue** args, int argc);
extern HcsValue* hcs_paintcanvas_invert(HcsRuntime* rt, HcsValue** args, int argc);
extern HcsValue* hcs_paintcanvas_grayscale(HcsRuntime* rt, HcsValue** args, int argc);
extern HcsValue* hcs_paintcanvas_brightness(HcsRuntime* rt, HcsValue** args, int argc);
extern HcsValue* hcs_paintcanvas_blur(HcsRuntime* rt, HcsValue** args, int argc);
extern HcsValue* halforms_rt_msgbox(const char* text, const char* title, int buttons, int icon);
extern HcsValue* halforms_rt_open_file(const char* title, const char* filter);
extern HcsValue* halforms_rt_save_file(const char* title, const char* filter, const char* defaultName);
extern HcsValue* halforms_rt_browse_folder(const char* title);
extern HcsValue* halforms_rt_color_dialog(int initialColor);
extern HcsValue* halforms_rt_input_dialog(const char* title, const char* prompt, const char* defaultValue);
extern void halforms_rt_create_menu(const char* name);
extern void halforms_rt_add_menu_item(const char* menuName, const char* text, const char* handlerName);
extern void halforms_rt_set_form_menu(const char* formName, const char* menuName);
extern void halforms_rt_create_statusbar(const char* name, int partCount);
extern void halforms_rt_set_statusbar_text(const char* name, int part, const char* text);
extern void halforms_rt_tree_add_item(const char* treeName, const char* parentPath, const char* text);
extern void halforms_rt_tab_add_tab(const char* tabName, const char* title);

// Flag to use HalGUI instead of Win32
static bool g_use_halgui = false;
// Flag to use HalForms
static bool g_use_halforms = false;

void execute_statement(HcsRuntime* rt, HcsAstNode* node);
HcsValue* eval_expression(HcsRuntime* rt, HcsAstNode* node);

HcsScope* scope_create(HcsScope* parent) {
    HcsScope* s = (HcsScope*)calloc(1, sizeof(HcsScope));
    s->parent = parent;
    return s;
}

void scope_free(HcsScope* scope) {
    if (!scope) return;
    for (int i = 0; i < scope->var_count; i++) {
        free(scope->vars[i].name);
        value_release(scope->vars[i].value);
    }
    free(scope->vars);
    free(scope);
}

void scope_set(HcsScope* scope, const char* name, HcsValue* value, bool is_const) {
    for (int i = 0; i < scope->var_count; i++) {
        if (strcmp(scope->vars[i].name, name) == 0) {
            if (scope->vars[i].is_const) return;
            value_release(scope->vars[i].value);
            scope->vars[i].value = value;
            return;
        }
    }
    HcsScope* s = scope->parent;
    while (s) {
        for (int i = 0; i < s->var_count; i++) {
            if (strcmp(s->vars[i].name, name) == 0) {
                if (s->vars[i].is_const) return;
                value_release(s->vars[i].value);
                s->vars[i].value = value;
                return;
            }
        }
        s = s->parent;
    }
    if (scope->var_count >= scope->var_capacity) {
        int nc = scope->var_capacity == 0 ? 16 : scope->var_capacity * 2;
        scope->vars = realloc(scope->vars, sizeof(scope->vars[0]) * nc);
        scope->var_capacity = nc;
    }
    scope->vars[scope->var_count].name = strdup(name);
    scope->vars[scope->var_count].value = value;
    scope->vars[scope->var_count].is_const = is_const;
    scope->var_count++;
}

HcsValue* scope_get(HcsScope* scope, const char* name) {
    while (scope) {
        for (int i = 0; i < scope->var_count; i++) {
            if (strcmp(scope->vars[i].name, name) == 0) return scope->vars[i].value;
        }
        scope = scope->parent;
    }
    return NULL;
}

HcsRuntime* runtime_create(void) {
    HcsRuntime* rt = (HcsRuntime*)calloc(1, sizeof(HcsRuntime));
    rt->global_scope = scope_create(NULL);
    rt->current_scope = rt->global_scope;
    rt->running = true;
    return rt;
}

void runtime_free(HcsRuntime* rt) {
    if (!rt) return;
    for (int i = 0; i < rt->control_count; i++) { free(rt->controls[i].name); free(rt->controls[i].type); }
    free(rt->controls);
    for (int i = 0; i < rt->handler_count; i++) { free(rt->handlers[i].element_name); free(rt->handlers[i].event_type); }
    free(rt->handlers);
    for (int i = 0; i < rt->timer_count; i++) free(rt->timers[i].name);
    free(rt->timers);
    for (int i = 0; i < rt->func_count; i++) free(rt->functions[i].name);
    free(rt->functions);
    scope_free(rt->global_scope);
    free(rt);
}

static void add_function(HcsRuntime* rt, const char* name, HcsAstNode* node) {
    if (rt->func_count >= rt->func_capacity) {
        int nc = rt->func_capacity == 0 ? 32 : rt->func_capacity * 2;
        rt->functions = realloc(rt->functions, sizeof(HcsFuncDef) * nc);
        rt->func_capacity = nc;
    }
    rt->functions[rt->func_count].name = strdup(name);
    rt->functions[rt->func_count].node = node;
    rt->func_count++;
}

static HcsAstNode* find_function(HcsRuntime* rt, const char* name) {
    for (int i = 0; i < rt->func_count; i++) {
        if (strcmp(rt->functions[i].name, name) == 0) return rt->functions[i].node;
    }
    return NULL;
}

HcsGuiControl* gui_find_control(HcsRuntime* rt, const char* name) {
    for (int i = 0; i < rt->control_count; i++) {
        if (strcmp(rt->controls[i].name, name) == 0) return &rt->controls[i];
    }
    return NULL;
}

void gui_fire_event(HcsRuntime* rt, const char* element, const char* event) {
    for (int i = 0; i < rt->handler_count; i++) {
        if (strcmp(rt->handlers[i].element_name, element) == 0 &&
            strcmp(rt->handlers[i].event_type, event) == 0) {
            HcsAstNode* h = rt->handlers[i].handler;
            if (h && h->type == HCS_AST_EVENT_HANDLER) {
                HcsScope* es = scope_create(rt->current_scope);
                HcsScope* old = rt->current_scope;
                rt->current_scope = es;
                for (int j = 0; j < h->data.event_handler.body.count; j++) {
                    execute_statement(rt, h->data.event_handler.body.items[j]);
                    if (rt->should_return || rt->should_break) break;
                }
                rt->current_scope = old;
                scope_free(es);
            }
        }
    }
}

static HcsValue* call_builtin(HcsRuntime* rt, const char* name, HcsAstList* args) {
    if (strcmp(name, "abs") == 0 && args->count > 0) {
        HcsValue* v = eval_expression(rt, args->items[0]);
        double r = fabs(value_to_number(v)); value_release(v);
        return value_number(r);
    }
    if (strcmp(name, "sqrt") == 0 && args->count > 0) {
        HcsValue* v = eval_expression(rt, args->items[0]);
        double r = sqrt(value_to_number(v)); value_release(v);
        return value_number(r);
    }
    if (strcmp(name, "round") == 0 && args->count > 0) {
        HcsValue* v = eval_expression(rt, args->items[0]);
        double r = round(value_to_number(v)); value_release(v);
        return value_number(r);
    }
    if (strcmp(name, "floor") == 0 && args->count > 0) {
        HcsValue* v = eval_expression(rt, args->items[0]);
        double r = floor(value_to_number(v)); value_release(v);
        return value_number(r);
    }
    if (strcmp(name, "ceil") == 0 && args->count > 0) {
        HcsValue* v = eval_expression(rt, args->items[0]);
        double r = ceil(value_to_number(v)); value_release(v);
        return value_number(r);
    }
    if (strcmp(name, "min") == 0 && args->count > 1) {
        HcsValue* a = eval_expression(rt, args->items[0]);
        HcsValue* b = eval_expression(rt, args->items[1]);
        double r = fmin(value_to_number(a), value_to_number(b));
        value_release(a); value_release(b);
        return value_number(r);
    }
    if (strcmp(name, "max") == 0 && args->count > 1) {
        HcsValue* a = eval_expression(rt, args->items[0]);
        HcsValue* b = eval_expression(rt, args->items[1]);
        double r = fmax(value_to_number(a), value_to_number(b));
        value_release(a); value_release(b);
        return value_number(r);
    }
    if (strcmp(name, "random") == 0) return value_number((double)rand() / RAND_MAX);
    if (strcmp(name, "len") == 0 && args->count > 0) {
        HcsValue* v = eval_expression(rt, args->items[0]);
        int len = 0;
        if (v->type == HCS_VAL_STRING) len = strlen(v->data.string);
        else if (v->type == HCS_VAL_ARRAY) len = value_array_length(v);
        value_release(v);
        return value_number(len);
    }
    if (strcmp(name, "str") == 0 && args->count > 0) {
        HcsValue* v = eval_expression(rt, args->items[0]);
        char* s = value_to_string(v);
        HcsValue* r = value_string(s);
        free(s); value_release(v);
        return r;
    }
    if (strcmp(name, "int") == 0 && args->count > 0) {
        HcsValue* v = eval_expression(rt, args->items[0]);
        int n = (int)value_to_number(v); value_release(v);
        return value_number(n);
    }
    if (strcmp(name, "float") == 0 && args->count > 0) {
        HcsValue* v = eval_expression(rt, args->items[0]);
        double n = value_to_number(v); value_release(v);
        return value_number(n);
    }
    
    // Input/Output functions
    if (strcmp(name, "input") == 0) {
        // Optional prompt message
        if (args->count > 0) {
            HcsValue* prompt = eval_expression(rt, args->items[0]);
            char* prompt_str = value_to_string(prompt);
            printf("%s", prompt_str);
            fflush(stdout);
            free(prompt_str);
            value_release(prompt);
        }
        
        char buffer[4096];
        if (fgets(buffer, sizeof(buffer), stdin)) {
            size_t len = strlen(buffer);
            if (len > 0 && buffer[len-1] == '\n') buffer[len-1] = '\0';
            return value_string(buffer);
        }
        return value_string("");
    }
    
    // Array functions
    if (strcmp(name, "array") == 0) {
        // Create empty array or array with initial size
        HcsValue* arr = value_array();
        if (args->count > 0) {
            HcsValue* size_v = eval_expression(rt, args->items[0]);
            int size = (int)value_to_number(size_v);
            value_release(size_v);
            for (int i = 0; i < size; i++) {
                value_array_push(arr, value_null());
            }
        }
        return arr;
    }
    if (strcmp(name, "range") == 0 && args->count >= 1) {
        // range(end) or range(start, end) or range(start, end, step)
        int start = 0, end = 0, step = 1;
        if (args->count == 1) {
            HcsValue* end_v = eval_expression(rt, args->items[0]);
            end = (int)value_to_number(end_v);
            value_release(end_v);
        } else if (args->count >= 2) {
            HcsValue* start_v = eval_expression(rt, args->items[0]);
            HcsValue* end_v = eval_expression(rt, args->items[1]);
            start = (int)value_to_number(start_v);
            end = (int)value_to_number(end_v);
            value_release(start_v);
            value_release(end_v);
            if (args->count >= 3) {
                HcsValue* step_v = eval_expression(rt, args->items[2]);
                step = (int)value_to_number(step_v);
                value_release(step_v);
            }
        }
        HcsValue* arr = value_array();
        if (step > 0) {
            for (int i = start; i < end; i += step) {
                value_array_push(arr, value_number(i));
            }
        } else if (step < 0) {
            for (int i = start; i > end; i += step) {
                value_array_push(arr, value_number(i));
            }
        }
        return arr;
    }
    
    // String functions
    if (strcmp(name, "upper") == 0 && args->count > 0) {
        HcsValue* v = eval_expression(rt, args->items[0]);
        char* s = value_to_string(v);
        for (int i = 0; s[i]; i++) s[i] = toupper(s[i]);
        HcsValue* r = value_string(s);
        free(s); value_release(v);
        return r;
    }
    if (strcmp(name, "lower") == 0 && args->count > 0) {
        HcsValue* v = eval_expression(rt, args->items[0]);
        char* s = value_to_string(v);
        for (int i = 0; s[i]; i++) s[i] = tolower(s[i]);
        HcsValue* r = value_string(s);
        free(s); value_release(v);
        return r;
    }
    if (strcmp(name, "trim") == 0 && args->count > 0) {
        HcsValue* v = eval_expression(rt, args->items[0]);
        char* s = value_to_string(v);
        // Trim leading spaces
        char* start = s;
        while (*start && isspace(*start)) start++;
        // Trim trailing spaces
        char* end = start + strlen(start) - 1;
        while (end > start && isspace(*end)) end--;
        *(end + 1) = '\0';
        HcsValue* r = value_string(start);
        free(s); value_release(v);
        return r;
    }
    if (strcmp(name, "split") == 0 && args->count >= 2) {
        HcsValue* str_v = eval_expression(rt, args->items[0]);
        HcsValue* delim_v = eval_expression(rt, args->items[1]);
        char* str = value_to_string(str_v);
        char* delim = value_to_string(delim_v);
        
        HcsValue* arr = value_array();
        char* copy = strdup(str);
        char* token = strtok(copy, delim);
        while (token) {
            value_array_push(arr, value_string(token));
            token = strtok(NULL, delim);
        }
        
        free(copy);
        free(str);
        free(delim);
        value_release(str_v);
        value_release(delim_v);
        return arr;
    }
    if (strcmp(name, "replace") == 0 && args->count >= 3) {
        HcsValue* str_v = eval_expression(rt, args->items[0]);
        HcsValue* old_v = eval_expression(rt, args->items[1]);
        HcsValue* new_v = eval_expression(rt, args->items[2]);
        
        char* str = value_to_string(str_v);
        char* old = value_to_string(old_v);
        char* new = value_to_string(new_v);
        
        char* pos = strstr(str, old);
        if (pos) {
            size_t len = strlen(str) - strlen(old) + strlen(new) + 1;
            char* result = malloc(len);
            size_t prefix_len = pos - str;
            strncpy(result, str, prefix_len);
            result[prefix_len] = '\0';
            strcat(result, new);
            strcat(result, pos + strlen(old));
            HcsValue* r = value_string(result);
            free(result);
            free(str); free(old); free(new);
            value_release(str_v); value_release(old_v); value_release(new_v);
            return r;
        }
        
        free(old); free(new);
        value_release(old_v); value_release(new_v);
        HcsValue* r = value_string(str);
        free(str); value_release(str_v);
        return r;
    }
    
    // Math functions
    if (strcmp(name, "sin") == 0 && args->count > 0) {
        HcsValue* v = eval_expression(rt, args->items[0]);
        double r = sin(value_to_number(v)); value_release(v);
        return value_number(r);
    }
    if (strcmp(name, "cos") == 0 && args->count > 0) {
        HcsValue* v = eval_expression(rt, args->items[0]);
        double r = cos(value_to_number(v)); value_release(v);
        return value_number(r);
    }
    if (strcmp(name, "tan") == 0 && args->count > 0) {
        HcsValue* v = eval_expression(rt, args->items[0]);
        double r = tan(value_to_number(v)); value_release(v);
        return value_number(r);
    }
    if (strcmp(name, "pow") == 0 && args->count > 1) {
        HcsValue* base = eval_expression(rt, args->items[0]);
        HcsValue* exp = eval_expression(rt, args->items[1]);
        double r = pow(value_to_number(base), value_to_number(exp));
        value_release(base); value_release(exp);
        return value_number(r);
    }
    if (strcmp(name, "log") == 0 && args->count > 0) {
        HcsValue* v = eval_expression(rt, args->items[0]);
        double r = log(value_to_number(v)); value_release(v);
        return value_number(r);
    }
    if (strcmp(name, "exp") == 0 && args->count > 0) {
        HcsValue* v = eval_expression(rt, args->items[0]);
        double r = exp(value_to_number(v)); value_release(v);
        return value_number(r);
    }
    
    // Math.* namespace functions
    if (strncmp(name, "Math.", 5) == 0) {
        const char* func = name + 5;
        if (strcmp(func, "sqrt") == 0 && args->count > 0) {
            HcsValue* v = eval_expression(rt, args->items[0]);
            double r = sqrt(value_to_number(v)); value_release(v);
            return value_number(r);
        }
        if (strcmp(func, "abs") == 0 && args->count > 0) {
            HcsValue* v = eval_expression(rt, args->items[0]);
            double r = fabs(value_to_number(v)); value_release(v);
            return value_number(r);
        }
        if (strcmp(func, "sin") == 0 && args->count > 0) {
            HcsValue* v = eval_expression(rt, args->items[0]);
            double r = sin(value_to_number(v)); value_release(v);
            return value_number(r);
        }
        if (strcmp(func, "cos") == 0 && args->count > 0) {
            HcsValue* v = eval_expression(rt, args->items[0]);
            double r = cos(value_to_number(v)); value_release(v);
            return value_number(r);
        }
        if (strcmp(func, "tan") == 0 && args->count > 0) {
            HcsValue* v = eval_expression(rt, args->items[0]);
            double r = tan(value_to_number(v)); value_release(v);
            return value_number(r);
        }
        if (strcmp(func, "pow") == 0 && args->count > 1) {
            HcsValue* base = eval_expression(rt, args->items[0]);
            HcsValue* exp = eval_expression(rt, args->items[1]);
            double r = pow(value_to_number(base), value_to_number(exp));
            value_release(base); value_release(exp);
            return value_number(r);
        }
        if (strcmp(func, "floor") == 0 && args->count > 0) {
            HcsValue* v = eval_expression(rt, args->items[0]);
            double r = floor(value_to_number(v)); value_release(v);
            return value_number(r);
        }
        if (strcmp(func, "ceil") == 0 && args->count > 0) {
            HcsValue* v = eval_expression(rt, args->items[0]);
            double r = ceil(value_to_number(v)); value_release(v);
            return value_number(r);
        }
        if (strcmp(func, "round") == 0 && args->count > 0) {
            HcsValue* v = eval_expression(rt, args->items[0]);
            double r = round(value_to_number(v)); value_release(v);
            return value_number(r);
        }
        if (strcmp(func, "random") == 0) {
            return value_number((double)rand() / RAND_MAX);
        }
        if (strcmp(func, "min") == 0 && args->count > 1) {
            HcsValue* a = eval_expression(rt, args->items[0]);
            HcsValue* b = eval_expression(rt, args->items[1]);
            double r = fmin(value_to_number(a), value_to_number(b));
            value_release(a); value_release(b);
            return value_number(r);
        }
        if (strcmp(func, "max") == 0 && args->count > 1) {
            HcsValue* a = eval_expression(rt, args->items[0]);
            HcsValue* b = eval_expression(rt, args->items[1]);
            double r = fmax(value_to_number(a), value_to_number(b));
            value_release(a); value_release(b);
            return value_number(r);
        }
        if (strcmp(func, "PI") == 0) {
            return value_number(3.14159265358979323846);
        }
        if (strcmp(func, "E") == 0) {
            return value_number(2.71828182845904523536);
        }
    }
    
    // File I/O functions
    if (strcmp(name, "File.read") == 0 && args->count > 0) {
        HcsValue* path_v = eval_expression(rt, args->items[0]);
        char* path = value_to_string(path_v);
        FILE* f = fopen(path, "rb");
        if (f) {
            fseek(f, 0, SEEK_END);
            long size = ftell(f);
            fseek(f, 0, SEEK_SET);
            char* content = (char*)malloc(size + 1);
            fread(content, 1, size, f);
            content[size] = '\0';
            fclose(f);
            HcsValue* result = value_string(content);
            free(content);
            free(path);
            value_release(path_v);
            return result;
        }
        free(path);
        value_release(path_v);
        return value_null();
    }
    if (strcmp(name, "File.write") == 0 && args->count >= 2) {
        HcsValue* path_v = eval_expression(rt, args->items[0]);
        HcsValue* content_v = eval_expression(rt, args->items[1]);
        char* path = value_to_string(path_v);
        char* content = value_to_string(content_v);
        FILE* f = fopen(path, "wb");
        if (f) {
            fwrite(content, 1, strlen(content), f);
            fclose(f);
            free(path);
            free(content);
            value_release(path_v);
            value_release(content_v);
            return value_bool(true);
        }
        free(path);
        free(content);
        value_release(path_v);
        value_release(content_v);
        return value_bool(false);
    }
    if (strcmp(name, "File.exists") == 0 && args->count > 0) {
        HcsValue* path_v = eval_expression(rt, args->items[0]);
        char* path = value_to_string(path_v);
        FILE* f = fopen(path, "r");
        bool exists = (f != NULL);
        if (f) fclose(f);
        free(path);
        value_release(path_v);
        return value_bool(exists);
    }
    
    // HalGUI.GPU.* functions
    if (strncmp(name, "HalGUI.GPU.", 11) == 0) {
        const char* func = name + 11;
        
        if (strcmp(func, "init") == 0 && args->count >= 3) {
            char* title = value_to_string(eval_expression(rt, args->items[0]));
            int width = (int)value_to_number(eval_expression(rt, args->items[1]));
            int height = (int)value_to_number(eval_expression(rt, args->items[2]));
            halgui_gpu_init(rt, title, width, height);
            free(title);
            return value_null();
        }
        if (strcmp(func, "run") == 0 && args->count >= 1) {
            // Get render callback function
            HcsValue* callback_val = eval_expression(rt, args->items[0]);
            if (callback_val->type == HCS_VAL_STRING) {
                // Function name passed as string
                HcsAstNode* fn = find_function(rt, callback_val->data.string);
                value_release(callback_val);
                if (fn) {
                    halgui_gpu_run(rt, fn);
                }
            }
            return value_null();
        }
        if (strcmp(func, "drawRect") == 0) return halgui_gpu_draw_rect_fn(rt, args);
        if (strcmp(func, "drawRoundedRect") == 0) return halgui_gpu_draw_rounded_rect_fn(rt, args);
        if (strcmp(func, "drawCircle") == 0) return halgui_gpu_draw_circle_fn(rt, args);
        if (strcmp(func, "drawLine") == 0) return halgui_gpu_draw_line_fn(rt, args);
        if (strcmp(func, "drawGradient") == 0) return halgui_gpu_draw_gradient_fn(rt, args);
        if (strcmp(func, "drawShadow") == 0) return halgui_gpu_draw_shadow_fn(rt, args);
        if (strcmp(func, "drawText") == 0) return halgui_gpu_draw_text_fn(rt, args);
        if (strcmp(func, "getWidth") == 0) return halgui_gpu_get_width_fn(rt, args);
        if (strcmp(func, "getHeight") == 0) return halgui_gpu_get_height_fn(rt, args);
        if (strcmp(func, "setVSync") == 0) return halgui_gpu_set_vsync_fn(rt, args);
        if (strcmp(func, "rgb") == 0) return halgui_gpu_rgb_fn(rt, args);
    }
    
    // HalGUI.* functions (new modern API)
    if (strncmp(name, "HalGUI.", 7) == 0) {
        const char* func = name + 7;
        
        // HalGUI.init() - enable HalGUI mode
        if (strcmp(func, "init") == 0) {
            g_use_halgui = true;
            halgui_runtime_init(rt);
            return value_null();
        }
        
        // HalGUI.setTheme(name)
        if (strcmp(func, "setTheme") == 0 && args->count >= 1) {
            HcsValue* v = eval_expression(rt, args->items[0]);
            char* theme = value_to_string(v);
            halgui_set_theme(theme);
            free(theme);
            value_release(v);
            return value_null();
        }
        
        // HalGUI.run() - start event loop
        if (strcmp(func, "run") == 0) {
            halgui_run();
            return value_null();
        }
        
        // HalGUI.messageBox(title, message, buttons, icon)
        if (strcmp(func, "messageBox") == 0 && args->count >= 2) {
            HcsValue* title_v = eval_expression(rt, args->items[0]);
            HcsValue* msg_v = eval_expression(rt, args->items[1]);
            char* title = value_to_string(title_v);
            char* msg = value_to_string(msg_v);
            int buttons = args->count > 2 ? (int)value_to_number(eval_expression(rt, args->items[2])) : 0;
            int icon = args->count > 3 ? (int)value_to_number(eval_expression(rt, args->items[3])) : 0;
            HcsValue* result = halgui_dialog_message(rt, title, msg, buttons, icon);
            free(title); free(msg);
            value_release(title_v); value_release(msg_v);
            return result;
        }
        
        // HalGUI.openFile(title, filter)
        if (strcmp(func, "openFile") == 0) {
            char* title = args->count > 0 ? value_to_string(eval_expression(rt, args->items[0])) : strdup("Open File");
            char* filter = args->count > 1 ? value_to_string(eval_expression(rt, args->items[1])) : strdup("All Files|*.*||");
            HcsValue* result = halgui_dialog_open_file(rt, title, filter);
            free(title); free(filter);
            return result;
        }
        
        // HalGUI.saveFile(title, filter, defaultName)
        if (strcmp(func, "saveFile") == 0) {
            char* title = args->count > 0 ? value_to_string(eval_expression(rt, args->items[0])) : strdup("Save File");
            char* filter = args->count > 1 ? value_to_string(eval_expression(rt, args->items[1])) : strdup("All Files|*.*||");
            char* defname = args->count > 2 ? value_to_string(eval_expression(rt, args->items[2])) : strdup("");
            HcsValue* result = halgui_dialog_save_file(rt, title, filter, defname);
            free(title); free(filter); free(defname);
            return result;
        }
        
        // HalGUI.selectFolder(title)
        extern HcsValue* halgui_dialog_select_folder(HcsRuntime* rt, const char* title);
        if (strcmp(func, "selectFolder") == 0) {
            char* title = args->count > 0 ? value_to_string(eval_expression(rt, args->items[0])) : strdup("Select Folder");
            HcsValue* result = halgui_dialog_select_folder(rt, title);
            free(title);
            return result;
        }
        
        // HalGUI.showNotification(title, message, type, duration)
        extern void halgui_show_notification(const char* title, const char* message, const char* type, int duration);
        if (strcmp(func, "showNotification") == 0 && args->count >= 2) {
            HcsValue* title_v = eval_expression(rt, args->items[0]);
            HcsValue* msg_v = eval_expression(rt, args->items[1]);
            char* title = value_to_string(title_v);
            char* msg = value_to_string(msg_v);
            char* type = args->count > 2 ? value_to_string(eval_expression(rt, args->items[2])) : strdup("info");
            int duration = args->count > 3 ? (int)value_to_number(eval_expression(rt, args->items[3])) : 3000;
            halgui_show_notification(title, msg, type, duration);
            free(title); free(msg); free(type);
            value_release(title_v); value_release(msg_v);
            return value_null();
        }
        
        // HalGUI.setLayout(panelName, layoutType)
        if (strcmp(func, "setLayout") == 0 && args->count >= 2) {
            HcsValue* panel_v = eval_expression(rt, args->items[0]);
            HcsValue* layout_v = eval_expression(rt, args->items[1]);
            char* panel = value_to_string(panel_v);
            char* layout = value_to_string(layout_v);
            halgui_set_layout(panel, layout);
            free(panel); free(layout);
            value_release(panel_v); value_release(layout_v);
            return value_null();
        }
        
        // HalGUI.setGap(panelName, gap)
        if (strcmp(func, "setGap") == 0 && args->count >= 2) {
            HcsValue* panel_v = eval_expression(rt, args->items[0]);
            HcsValue* gap_v = eval_expression(rt, args->items[1]);
            char* panel = value_to_string(panel_v);
            int gap = (int)value_to_number(gap_v);
            halgui_set_gap(panel, gap);
            free(panel);
            value_release(panel_v); value_release(gap_v);
            return value_null();
        }
        
        // HalGUI.setAlign(widgetName, horizontal, vertical)
        if (strcmp(func, "setAlign") == 0 && args->count >= 3) {
            HcsValue* widget_v = eval_expression(rt, args->items[0]);
            HcsValue* h_v = eval_expression(rt, args->items[1]);
            HcsValue* v_v = eval_expression(rt, args->items[2]);
            char* widget = value_to_string(widget_v);
            char* h_align = value_to_string(h_v);
            char* v_align = value_to_string(v_v);
            halgui_set_align(widget, h_align, v_align);
            free(widget); free(h_align); free(v_align);
            value_release(widget_v); value_release(h_v); value_release(v_v);
            return value_null();
        }
        
        // HalGUI.setFlex(widgetName, flex)
        if (strcmp(func, "setFlex") == 0 && args->count >= 2) {
            HcsValue* widget_v = eval_expression(rt, args->items[0]);
            HcsValue* flex_v = eval_expression(rt, args->items[1]);
            char* widget = value_to_string(widget_v);
            float flex = (float)value_to_number(flex_v);
            halgui_set_widget_flex(widget, flex);
            free(widget);
            value_release(widget_v); value_release(flex_v);
            return value_null();
        }
        
        // HalGUI.setMargin(widgetName, top, right, bottom, left)
        if (strcmp(func, "setMargin") == 0 && args->count >= 5) {
            HcsValue* widget_v = eval_expression(rt, args->items[0]);
            HcsValue* top_v = eval_expression(rt, args->items[1]);
            HcsValue* right_v = eval_expression(rt, args->items[2]);
            HcsValue* bottom_v = eval_expression(rt, args->items[3]);
            HcsValue* left_v = eval_expression(rt, args->items[4]);
            char* widget = value_to_string(widget_v);
            int top = (int)value_to_number(top_v);
            int right = (int)value_to_number(right_v);
            int bottom = (int)value_to_number(bottom_v);
            int left = (int)value_to_number(left_v);
            halgui_set_widget_margin(widget, top, right, bottom, left);
            free(widget);
            value_release(widget_v); value_release(top_v); value_release(right_v);
            value_release(bottom_v); value_release(left_v);
            return value_null();
        }
        
        // HalGUI.applyLayout(panelName)
        if (strcmp(func, "applyLayout") == 0 && args->count >= 1) {
            HcsValue* panel_v = eval_expression(rt, args->items[0]);
            char* panel = value_to_string(panel_v);
            halgui_apply_layout(panel);
            free(panel);
            value_release(panel_v);
            return value_null();
        }
    }
    
    // HalForms.* functions (Windows Forms-like API)
    if (strncmp(name, "HalForms.", 9) == 0) {
        const char* func = name + 9;
        
        // HalForms.init() - enable HalForms mode
        if (strcmp(func, "init") == 0) {
            g_use_halforms = true;
            halforms_runtime_init(rt);
            return value_null();
        }
        
        // HalForms.createForm(name, title, width, height, style)
        if (strcmp(func, "createForm") == 0 && args->count >= 4) {
            HcsValue* name_v = eval_expression(rt, args->items[0]);
            HcsValue* title_v = eval_expression(rt, args->items[1]);
            HcsValue* w_v = eval_expression(rt, args->items[2]);
            HcsValue* h_v = eval_expression(rt, args->items[3]);
            char* form_name = value_to_string(name_v);
            char* title = value_to_string(title_v);
            int width = (int)value_to_number(w_v);
            int height = (int)value_to_number(h_v);
            int style = args->count > 4 ? (int)value_to_number(eval_expression(rt, args->items[4])) : 0;
            halforms_rt_create_form(rt, form_name, title, width, height, style);
            free(form_name); free(title);
            value_release(name_v); value_release(title_v); value_release(w_v); value_release(h_v);
            return value_null();
        }
        
        // HalForms.createControl(type, name, text, x, y, w, h)
        if (strcmp(func, "createControl") == 0 && args->count >= 7) {
            HcsValue* type_v = eval_expression(rt, args->items[0]);
            HcsValue* name_v = eval_expression(rt, args->items[1]);
            HcsValue* text_v = eval_expression(rt, args->items[2]);
            HcsValue* x_v = eval_expression(rt, args->items[3]);
            HcsValue* y_v = eval_expression(rt, args->items[4]);
            HcsValue* w_v = eval_expression(rt, args->items[5]);
            HcsValue* h_v = eval_expression(rt, args->items[6]);
            char* type = value_to_string(type_v);
            char* ctrl_name = value_to_string(name_v);
            char* text = value_to_string(text_v);
            int x = (int)value_to_number(x_v);
            int y = (int)value_to_number(y_v);
            int w = (int)value_to_number(w_v);
            int h = (int)value_to_number(h_v);
            halforms_rt_create_control(rt, type, ctrl_name, text, x, y, w, h);
            free(type); free(ctrl_name); free(text);
            value_release(type_v); value_release(name_v); value_release(text_v);
            value_release(x_v); value_release(y_v); value_release(w_v); value_release(h_v);
            return value_null();
        }
        
        // HalForms.setProperty(element, property, value)
        if (strcmp(func, "setProperty") == 0 && args->count >= 3) {
            HcsValue* elem_v = eval_expression(rt, args->items[0]);
            HcsValue* prop_v = eval_expression(rt, args->items[1]);
            HcsValue* val_v = eval_expression(rt, args->items[2]);
            char* element = value_to_string(elem_v);
            char* prop = value_to_string(prop_v);
            halforms_rt_set_property(rt, element, prop, val_v);
            free(element); free(prop);
            value_release(elem_v); value_release(prop_v);
            return value_null();
        }
        
        // HalForms.getProperty(element, property)
        if (strcmp(func, "getProperty") == 0 && args->count >= 2) {
            HcsValue* elem_v = eval_expression(rt, args->items[0]);
            HcsValue* prop_v = eval_expression(rt, args->items[1]);
            char* element = value_to_string(elem_v);
            char* prop = value_to_string(prop_v);
            HcsValue* result = halforms_rt_get_property(rt, element, prop);
            free(element); free(prop);
            value_release(elem_v); value_release(prop_v);
            return result;
        }
        
        // HalForms.addItem(element, item)
        if (strcmp(func, "addItem") == 0 && args->count >= 2) {
            HcsValue* elem_v = eval_expression(rt, args->items[0]);
            HcsValue* item_v = eval_expression(rt, args->items[1]);
            char* element = value_to_string(elem_v);
            char* item = value_to_string(item_v);
            halforms_rt_add_item(element, item);
            free(element); free(item);
            value_release(elem_v); value_release(item_v);
            return value_null();
        }
        
        // HalForms.clearItems(element)
        if (strcmp(func, "clearItems") == 0 && args->count >= 1) {
            HcsValue* elem_v = eval_expression(rt, args->items[0]);
            char* element = value_to_string(elem_v);
            halforms_rt_clear_items(element);
            free(element);
            value_release(elem_v);
            return value_null();
        }
        
        // HalForms.run() - start event loop
        if (strcmp(func, "run") == 0) {
            halforms_rt_run();
            return value_null();
        }
        
        // HalForms.messageBox(text, title, buttons, icon)
        if (strcmp(func, "messageBox") == 0 && args->count >= 2) {
            HcsValue* text_v = eval_expression(rt, args->items[0]);
            HcsValue* title_v = eval_expression(rt, args->items[1]);
            char* text = value_to_string(text_v);
            char* title = value_to_string(title_v);
            int buttons = args->count > 2 ? (int)value_to_number(eval_expression(rt, args->items[2])) : 0;
            int icon = args->count > 3 ? (int)value_to_number(eval_expression(rt, args->items[3])) : 0;
            HcsValue* result = halforms_rt_msgbox(text, title, buttons, icon);
            free(text); free(title);
            value_release(text_v); value_release(title_v);
            return result;
        }
        
        // HalForms.openFile(title, filter)
        if (strcmp(func, "openFile") == 0) {
            char* title = args->count > 0 ? value_to_string(eval_expression(rt, args->items[0])) : strdup("Open File");
            char* filter = args->count > 1 ? value_to_string(eval_expression(rt, args->items[1])) : strdup("All Files (*.*)\0*.*\0");
            HcsValue* result = halforms_rt_open_file(title, filter);
            free(title); free(filter);
            return result;
        }
        
        // HalForms.saveFile(title, filter, defaultName)
        if (strcmp(func, "saveFile") == 0) {
            char* title = args->count > 0 ? value_to_string(eval_expression(rt, args->items[0])) : strdup("Save File");
            char* filter = args->count > 1 ? value_to_string(eval_expression(rt, args->items[1])) : strdup("All Files (*.*)\0*.*\0");
            char* defname = args->count > 2 ? value_to_string(eval_expression(rt, args->items[2])) : strdup("");
            HcsValue* result = halforms_rt_save_file(title, filter, defname);
            free(title); free(filter); free(defname);
            return result;
        }
        
        // HalForms.browseFolder(title)
        if (strcmp(func, "browseFolder") == 0) {
            char* title = args->count > 0 ? value_to_string(eval_expression(rt, args->items[0])) : strdup("Select Folder");
            HcsValue* result = halforms_rt_browse_folder(title);
            free(title);
            return result;
        }
        
        // HalForms.colorDialog(initialColor)
        if (strcmp(func, "colorDialog") == 0) {
            int initial = args->count > 0 ? (int)value_to_number(eval_expression(rt, args->items[0])) : 0xFFFFFF;
            return halforms_rt_color_dialog(initial);
        }
        
        // HalForms.inputDialog(title, prompt, defaultValue)
        if (strcmp(func, "inputDialog") == 0) {
            char* title = args->count > 0 ? value_to_string(eval_expression(rt, args->items[0])) : strdup("Input");
            char* prompt = args->count > 1 ? value_to_string(eval_expression(rt, args->items[1])) : strdup("Enter value:");
            char* defval = args->count > 2 ? value_to_string(eval_expression(rt, args->items[2])) : strdup("");
            HcsValue* result = halforms_rt_input_dialog(title, prompt, defval);
            free(title); free(prompt); free(defval);
            return result;
        }
        
        // HalForms.createMenu(name)
        if (strcmp(func, "createMenu") == 0 && args->count >= 1) {
            HcsValue* name_v = eval_expression(rt, args->items[0]);
            char* menu_name = value_to_string(name_v);
            halforms_rt_create_menu(menu_name);
            free(menu_name);
            value_release(name_v);
            return value_null();
        }
        
        // HalForms.addMenuItem(menuName, text, handlerName)
        if (strcmp(func, "addMenuItem") == 0 && args->count >= 2) {
            HcsValue* menu_v = eval_expression(rt, args->items[0]);
            HcsValue* text_v = eval_expression(rt, args->items[1]);
            char* menu_name = value_to_string(menu_v);
            char* text = value_to_string(text_v);
            char* handler = args->count > 2 ? value_to_string(eval_expression(rt, args->items[2])) : NULL;
            halforms_rt_add_menu_item(menu_name, text, handler);
            free(menu_name); free(text);
            if (handler) free(handler);
            value_release(menu_v); value_release(text_v);
            return value_null();
        }
        
        // HalForms.setFormMenu(formName, menuName)
        if (strcmp(func, "setFormMenu") == 0 && args->count >= 2) {
            HcsValue* form_v = eval_expression(rt, args->items[0]);
            HcsValue* menu_v = eval_expression(rt, args->items[1]);
            char* form_name = value_to_string(form_v);
            char* menu_name = value_to_string(menu_v);
            halforms_rt_set_form_menu(form_name, menu_name);
            free(form_name); free(menu_name);
            value_release(form_v); value_release(menu_v);
            return value_null();
        }
        
        // HalForms.createStatusBar(name, partCount)
        if (strcmp(func, "createStatusBar") == 0 && args->count >= 2) {
            HcsValue* name_v = eval_expression(rt, args->items[0]);
            HcsValue* count_v = eval_expression(rt, args->items[1]);
            char* sb_name = value_to_string(name_v);
            int count = (int)value_to_number(count_v);
            halforms_rt_create_statusbar(sb_name, count);
            free(sb_name);
            value_release(name_v); value_release(count_v);
            return value_null();
        }
        
        // HalForms.setStatusBarText(name, part, text)
        if (strcmp(func, "setStatusBarText") == 0 && args->count >= 3) {
            HcsValue* name_v = eval_expression(rt, args->items[0]);
            HcsValue* part_v = eval_expression(rt, args->items[1]);
            HcsValue* text_v = eval_expression(rt, args->items[2]);
            char* sb_name = value_to_string(name_v);
            int part = (int)value_to_number(part_v);
            char* text = value_to_string(text_v);
            halforms_rt_set_statusbar_text(sb_name, part, text);
            free(sb_name); free(text);
            value_release(name_v); value_release(part_v); value_release(text_v);
            return value_null();
        }
        
        // HalForms.treeAddItem(treeName, parentPath, text)
        if (strcmp(func, "treeAddItem") == 0 && args->count >= 3) {
            HcsValue* tree_v = eval_expression(rt, args->items[0]);
            HcsValue* parent_v = eval_expression(rt, args->items[1]);
            HcsValue* text_v = eval_expression(rt, args->items[2]);
            char* tree_name = value_to_string(tree_v);
            char* parent_path = value_to_string(parent_v);
            char* text = value_to_string(text_v);
            halforms_rt_tree_add_item(tree_name, parent_path, text);
            free(tree_name); free(parent_path); free(text);
            value_release(tree_v); value_release(parent_v); value_release(text_v);
            return value_null();
        }
        
        // HalForms.tabAddTab(tabName, title)
        if (strcmp(func, "tabAddTab") == 0 && args->count >= 2) {
            HcsValue* tab_v = eval_expression(rt, args->items[0]);
            HcsValue* title_v = eval_expression(rt, args->items[1]);
            char* tab_name = value_to_string(tab_v);
            char* title = value_to_string(title_v);
            halforms_rt_tab_add_tab(tab_name, title);
            free(tab_name); free(title);
            value_release(tab_v); value_release(title_v);
            return value_null();
        }
        
        // HalForms.showNotification(title, message, durationMs)
        if (strcmp(func, "showNotification") == 0 && args->count >= 2) {
            HcsValue* title_v = eval_expression(rt, args->items[0]);
            HcsValue* msg_v = eval_expression(rt, args->items[1]);
            char* title = value_to_string(title_v);
            char* msg = value_to_string(msg_v);
            int duration = args->count > 2 ? (int)value_to_number(eval_expression(rt, args->items[2])) : 3000;
            extern void halforms_show_notification(const char*, const char*, int);
            halforms_show_notification(title, msg, duration);
            free(title); free(msg);
            value_release(title_v); value_release(msg_v);
            return value_null();
        }
        
        // HalForms.clipboardSetText(text)
        if (strcmp(func, "clipboardSetText") == 0 && args->count >= 1) {
            HcsValue* text_v = eval_expression(rt, args->items[0]);
            char* text = value_to_string(text_v);
            extern bool halforms_clipboard_set_text(const char*);
            bool result = halforms_clipboard_set_text(text);
            free(text);
            value_release(text_v);
            return value_bool(result);
        }
        
        // HalForms.clipboardGetText()
        if (strcmp(func, "clipboardGetText") == 0) {
            extern char* halforms_clipboard_get_text(void);
            char* text = halforms_clipboard_get_text();
            if (text) {
                HcsValue* result = value_string(text);
                free(text);
                return result;
            }
            return value_null();
        }
        
        // HalForms.createPaintCanvas(name, x, y, w, h)
        if (strcmp(func, "createPaintCanvas") == 0 && args->count >= 5) {
            HcsValue* name_v = eval_expression(rt, args->items[0]);
            HcsValue* x_v = eval_expression(rt, args->items[1]);
            HcsValue* y_v = eval_expression(rt, args->items[2]);
            HcsValue* w_v = eval_expression(rt, args->items[3]);
            HcsValue* h_v = eval_expression(rt, args->items[4]);
            
            HcsValue* hcs_args[6];
            hcs_args[0] = value_string("main"); // parent form
            hcs_args[1] = name_v;
            hcs_args[2] = x_v;
            hcs_args[3] = y_v;
            hcs_args[4] = w_v;
            hcs_args[5] = h_v;
            
            HcsValue* result = hcs_paintcanvas_create(rt, hcs_args, 6);
            value_release(name_v); value_release(x_v); value_release(y_v);
            value_release(w_v); value_release(h_v);
            return result;
        }
        
        // HalForms.paintCanvasSetTool(name, tool)
        if (strcmp(func, "paintCanvasSetTool") == 0 && args->count >= 2) {
            HcsValue* name_v = eval_expression(rt, args->items[0]);
            HcsValue* tool_v = eval_expression(rt, args->items[1]);
            
            HcsValue* hcs_args[2];
            hcs_args[0] = name_v;
            hcs_args[1] = tool_v;
            
            HcsValue* result = hcs_paintcanvas_set_tool(rt, hcs_args, 2);
            value_release(name_v); value_release(tool_v);
            return result;
        }
        
        // HalForms.paintCanvasSetColor(name, r, g, b)
        if (strcmp(func, "paintCanvasSetColor") == 0 && args->count >= 4) {
            HcsValue* name_v = eval_expression(rt, args->items[0]);
            HcsValue* r_v = eval_expression(rt, args->items[1]);
            HcsValue* g_v = eval_expression(rt, args->items[2]);
            HcsValue* b_v = eval_expression(rt, args->items[3]);
            
            HcsValue* hcs_args[4];
            hcs_args[0] = name_v;
            hcs_args[1] = r_v;
            hcs_args[2] = g_v;
            hcs_args[3] = b_v;
            
            HcsValue* result = hcs_paintcanvas_set_color(rt, hcs_args, 4);
            value_release(name_v); value_release(r_v); value_release(g_v); value_release(b_v);
            return result;
        }
        
        // HalForms.paintCanvasSetBrushSize(name, size)
        if (strcmp(func, "paintCanvasSetBrushSize") == 0 && args->count >= 2) {
            HcsValue* name_v = eval_expression(rt, args->items[0]);
            HcsValue* size_v = eval_expression(rt, args->items[1]);
            
            HcsValue* hcs_args[2];
            hcs_args[0] = name_v;
            hcs_args[1] = size_v;
            
            HcsValue* result = hcs_paintcanvas_set_brush_size(rt, hcs_args, 2);
            value_release(name_v); value_release(size_v);
            return result;
        }
        
        // HalForms.paintCanvasSetBrushType(name, type)
        if (strcmp(func, "paintCanvasSetBrushType") == 0 && args->count >= 2) {
            HcsValue* name_v = eval_expression(rt, args->items[0]);
            HcsValue* type_v = eval_expression(rt, args->items[1]);
            
            HcsValue* hcs_args[2];
            hcs_args[0] = name_v;
            hcs_args[1] = type_v;
            
            HcsValue* result = hcs_paintcanvas_set_brush_type(rt, hcs_args, 2);
            value_release(name_v); value_release(type_v);
            return result;
        }
        
        // HalForms.paintCanvasClear(name, r, g, b)
        if (strcmp(func, "paintCanvasClear") == 0 && args->count >= 4) {
            HcsValue* name_v = eval_expression(rt, args->items[0]);
            HcsValue* r_v = eval_expression(rt, args->items[1]);
            HcsValue* g_v = eval_expression(rt, args->items[2]);
            HcsValue* b_v = eval_expression(rt, args->items[3]);
            
            HcsValue* hcs_args[4];
            hcs_args[0] = name_v;
            hcs_args[1] = r_v;
            hcs_args[2] = g_v;
            hcs_args[3] = b_v;
            
            HcsValue* result = hcs_paintcanvas_clear(rt, hcs_args, 4);
            value_release(name_v); value_release(r_v); value_release(g_v); value_release(b_v);
            return result;
        }
        
        // HalForms.paintCanvasUndo(name)
        if (strcmp(func, "paintCanvasUndo") == 0 && args->count >= 1) {
            HcsValue* name_v = eval_expression(rt, args->items[0]);
            
            HcsValue* hcs_args[1];
            hcs_args[0] = name_v;
            
            HcsValue* result = hcs_paintcanvas_undo(rt, hcs_args, 1);
            value_release(name_v);
            return result;
        }
        
        // HalForms.paintCanvasRedo(name)
        if (strcmp(func, "paintCanvasRedo") == 0 && args->count >= 1) {
            HcsValue* name_v = eval_expression(rt, args->items[0]);
            
            HcsValue* hcs_args[1];
            hcs_args[0] = name_v;
            
            HcsValue* result = hcs_paintcanvas_redo(rt, hcs_args, 1);
            value_release(name_v);
            return result;
        }
        
        // HalForms.paintCanvasSave(name, filename)
        if (strcmp(func, "paintCanvasSave") == 0 && args->count >= 2) {
            HcsValue* name_v = eval_expression(rt, args->items[0]);
            HcsValue* file_v = eval_expression(rt, args->items[1]);
            
            HcsValue* hcs_args[2];
            hcs_args[0] = name_v;
            hcs_args[1] = file_v;
            
            HcsValue* result = hcs_paintcanvas_save(rt, hcs_args, 2);
            value_release(name_v); value_release(file_v);
            return result;
        }
        
        // HalForms.paintCanvasLoad(name, filename)
        if (strcmp(func, "paintCanvasLoad") == 0 && args->count >= 2) {
            HcsValue* name_v = eval_expression(rt, args->items[0]);
            HcsValue* file_v = eval_expression(rt, args->items[1]);
            
            HcsValue* hcs_args[2];
            hcs_args[0] = name_v;
            hcs_args[1] = file_v;
            
            HcsValue* result = hcs_paintcanvas_load(rt, hcs_args, 2);
            value_release(name_v); value_release(file_v);
            return result;
        }
        
        // HalForms.paintCanvasGrayscale(name)
        if (strcmp(func, "paintCanvasGrayscale") == 0 && args->count >= 1) {
            HcsValue* name_v = eval_expression(rt, args->items[0]);
            
            HcsValue* hcs_args[1];
            hcs_args[0] = name_v;
            
            HcsValue* result = hcs_paintcanvas_grayscale(rt, hcs_args, 1);
            value_release(name_v);
            return result;
        }
        
        // HalForms.paintCanvasInvert(name)
        if (strcmp(func, "paintCanvasInvert") == 0 && args->count >= 1) {
            HcsValue* name_v = eval_expression(rt, args->items[0]);
            
            HcsValue* hcs_args[1];
            hcs_args[0] = name_v;
            
            HcsValue* result = hcs_paintcanvas_invert(rt, hcs_args, 1);
            value_release(name_v);
            return result;
        }
        
        // HalForms.paintCanvasBrightness(name, amount)
        if (strcmp(func, "paintCanvasBrightness") == 0 && args->count >= 2) {
            HcsValue* name_v = eval_expression(rt, args->items[0]);
            HcsValue* amt_v = eval_expression(rt, args->items[1]);
            
            HcsValue* hcs_args[2];
            hcs_args[0] = name_v;
            hcs_args[1] = amt_v;
            
            HcsValue* result = hcs_paintcanvas_brightness(rt, hcs_args, 2);
            value_release(name_v); value_release(amt_v);
            return result;
        }
        
        // HalForms.paintCanvasBlur(name, radius)
        if (strcmp(func, "paintCanvasBlur") == 0 && args->count >= 2) {
            HcsValue* name_v = eval_expression(rt, args->items[0]);
            HcsValue* rad_v = eval_expression(rt, args->items[1]);
            
            HcsValue* hcs_args[2];
            hcs_args[0] = name_v;
            hcs_args[1] = rad_v;
            
            HcsValue* result = hcs_paintcanvas_blur(rt, hcs_args, 2);
            value_release(name_v); value_release(rad_v);
            return result;
        }
    }
    
    // Audio.* functions
    if (strncmp(name, "Audio.", 6) == 0) {
        const char* func = name + 6;
        
        // Audio.create(name)
        if (strcmp(func, "create") == 0 && args->count >= 1) {
            HcsValue* name_v = eval_expression(rt, args->items[0]);
            char* player_name = value_to_string(name_v);
            HcsValue* result = halgui_audio_create(rt, player_name);
            free(player_name);
            value_release(name_v);
            return result;
        }
        
        // Audio.load(name, filePath)
        if (strcmp(func, "load") == 0 && args->count >= 2) {
            HcsValue* name_v = eval_expression(rt, args->items[0]);
            HcsValue* path_v = eval_expression(rt, args->items[1]);
            char* player_name = value_to_string(name_v);
            char* file_path = value_to_string(path_v);
            HcsValue* result = halgui_audio_load(rt, player_name, file_path);
            free(player_name); free(file_path);
            value_release(name_v); value_release(path_v);
            return result;
        }
        
        // Audio.play(name)
        if (strcmp(func, "play") == 0 && args->count >= 1) {
            HcsValue* name_v = eval_expression(rt, args->items[0]);
            char* player_name = value_to_string(name_v);
            HcsValue* result = halgui_audio_play(rt, player_name);
            free(player_name);
            value_release(name_v);
            return result;
        }
        
        // Audio.pause(name)
        if (strcmp(func, "pause") == 0 && args->count >= 1) {
            HcsValue* name_v = eval_expression(rt, args->items[0]);
            char* player_name = value_to_string(name_v);
            HcsValue* result = halgui_audio_pause(rt, player_name);
            free(player_name);
            value_release(name_v);
            return result;
        }
        
        // Audio.stop(name)
        if (strcmp(func, "stop") == 0 && args->count >= 1) {
            HcsValue* name_v = eval_expression(rt, args->items[0]);
            char* player_name = value_to_string(name_v);
            HcsValue* result = halgui_audio_stop(rt, player_name);
            free(player_name);
            value_release(name_v);
            return result;
        }
        
        // Audio.resume(name)
        if (strcmp(func, "resume") == 0 && args->count >= 1) {
            HcsValue* name_v = eval_expression(rt, args->items[0]);
            char* player_name = value_to_string(name_v);
            HcsValue* result = halgui_audio_resume(rt, player_name);
            free(player_name);
            value_release(name_v);
            return result;
        }
        
        // Audio.seek(name, positionMs)
        if (strcmp(func, "seek") == 0 && args->count >= 2) {
            HcsValue* name_v = eval_expression(rt, args->items[0]);
            HcsValue* pos_v = eval_expression(rt, args->items[1]);
            char* player_name = value_to_string(name_v);
            int position = (int)value_to_number(pos_v);
            HcsValue* result = halgui_audio_seek(rt, player_name, position);
            free(player_name);
            value_release(name_v); value_release(pos_v);
            return result;
        }
        
        // Audio.setVolume(name, volume)
        if (strcmp(func, "setVolume") == 0 && args->count >= 2) {
            HcsValue* name_v = eval_expression(rt, args->items[0]);
            HcsValue* vol_v = eval_expression(rt, args->items[1]);
            char* player_name = value_to_string(name_v);
            int volume = (int)value_to_number(vol_v);
            HcsValue* result = halgui_audio_set_volume(rt, player_name, volume);
            free(player_name);
            value_release(name_v); value_release(vol_v);
            return result;
        }
        
        // Audio.getVolume(name)
        if (strcmp(func, "getVolume") == 0 && args->count >= 1) {
            HcsValue* name_v = eval_expression(rt, args->items[0]);
            char* player_name = value_to_string(name_v);
            HcsValue* result = halgui_audio_get_volume(rt, player_name);
            free(player_name);
            value_release(name_v);
            return result;
        }
        
        // Audio.getPosition(name)
        if (strcmp(func, "getPosition") == 0 && args->count >= 1) {
            HcsValue* name_v = eval_expression(rt, args->items[0]);
            char* player_name = value_to_string(name_v);
            HcsValue* result = halgui_audio_get_position(rt, player_name);
            free(player_name);
            value_release(name_v);
            return result;
        }
        
        // Audio.getDuration(name)
        if (strcmp(func, "getDuration") == 0 && args->count >= 1) {
            HcsValue* name_v = eval_expression(rt, args->items[0]);
            char* player_name = value_to_string(name_v);
            HcsValue* result = halgui_audio_get_duration(rt, player_name);
            free(player_name);
            value_release(name_v);
            return result;
        }
        
        // Audio.getState(name)
        if (strcmp(func, "getState") == 0 && args->count >= 1) {
            HcsValue* name_v = eval_expression(rt, args->items[0]);
            char* player_name = value_to_string(name_v);
            HcsValue* result = halgui_audio_get_state(rt, player_name);
            free(player_name);
            value_release(name_v);
            return result;
        }
        
        // Audio.setLoop(name, loop)
        if (strcmp(func, "setLoop") == 0 && args->count >= 2) {
            HcsValue* name_v = eval_expression(rt, args->items[0]);
            HcsValue* loop_v = eval_expression(rt, args->items[1]);
            char* player_name = value_to_string(name_v);
            bool loop = value_to_bool(loop_v);
            HcsValue* result = halgui_audio_set_loop(rt, player_name, loop);
            free(player_name);
            value_release(name_v); value_release(loop_v);
            return result;
        }
        
        // Audio.setMute(name, mute)
        if (strcmp(func, "setMute") == 0 && args->count >= 2) {
            HcsValue* name_v = eval_expression(rt, args->items[0]);
            HcsValue* mute_v = eval_expression(rt, args->items[1]);
            char* player_name = value_to_string(name_v);
            bool mute = value_to_bool(mute_v);
            HcsValue* result = halgui_audio_set_mute(rt, player_name, mute);
            free(player_name);
            value_release(name_v); value_release(mute_v);
            return result;
        }
        
        // Audio.isMuted(name)
        if (strcmp(func, "isMuted") == 0 && args->count >= 1) {
            HcsValue* name_v = eval_expression(rt, args->items[0]);
            char* player_name = value_to_string(name_v);
            HcsValue* result = halgui_audio_is_muted(rt, player_name);
            free(player_name);
            value_release(name_v);
            return result;
        }
        
        // Audio.destroy(name)
        if (strcmp(func, "destroy") == 0 && args->count >= 1) {
            HcsValue* name_v = eval_expression(rt, args->items[0]);
            char* player_name = value_to_string(name_v);
            HcsValue* result = halgui_audio_destroy(rt, player_name);
            free(player_name);
            value_release(name_v);
            return result;
        }
    }
    
    extern HcsValue* call_system_api(HcsRuntime* rt, const char* name, HcsAstList* args);
    HcsValue* sys_result = call_system_api(rt, name, args);
    if (sys_result) return sys_result;
    
    return NULL;
}

HcsValue* eval_expression(HcsRuntime* rt, HcsAstNode* node) {
    if (!node) return value_null();
    switch (node->type) {
        case HCS_AST_NUMBER: return value_number(node->data.number_value);
        case HCS_AST_STRING: return value_string(node->data.string_value);
        case HCS_AST_BOOL: return value_bool(node->data.bool_value);
        case HCS_AST_NULL: return value_null();
        case HCS_AST_VAR_REF: {
            HcsValue* v = scope_get(rt->current_scope, node->data.var_ref.var_name);
            return v ? value_copy(v) : value_null();
        }
        case HCS_AST_ARRAY: {
            HcsValue* arr = value_array();
            for (int i = 0; i < node->data.array.elements.count; i++)
                value_array_push(arr, eval_expression(rt, node->data.array.elements.items[i]));
            return arr;
        }
        case HCS_AST_ARRAY_ACCESS: {
            HcsValue* arr = eval_expression(rt, node->data.array_access.array);
            HcsValue* idx = eval_expression(rt, node->data.array_access.index);
            HcsValue* r = value_copy(value_array_get(arr, (int)value_to_number(idx)));
            value_release(arr); value_release(idx);
            return r;
        }
        case HCS_AST_PROPERTY_ACCESS: {
            HcsValue* obj = eval_expression(rt, node->data.prop_access.object);
            const char* prop = node->data.prop_access.property;
            HcsValue* r = value_null();
            if (obj->type == HCS_VAL_ARRAY && strcmp(prop, "length") == 0)
                r = value_number(value_array_length(obj));
            else if (obj->type == HCS_VAL_STRING && strcmp(prop, "length") == 0)
                r = value_number(strlen(obj->data.string));
            value_release(obj);
            return r;
        }
        case HCS_AST_BINARY_EXPR: {
            HcsValue* l = eval_expression(rt, node->data.binary.left);
            HcsValue* r = eval_expression(rt, node->data.binary.right);
            const char* op = node->data.binary.op;
            HcsValue* res;
            if (strcmp(op, "+") == 0) {
                if (l->type == HCS_VAL_STRING || r->type == HCS_VAL_STRING) {
                    char* ls = value_to_string(l); char* rs = value_to_string(r);
                    char* c = malloc(strlen(ls) + strlen(rs) + 1);
                    strcpy(c, ls); strcat(c, rs);
                    res = value_string(c);
                    free(ls); free(rs); free(c);
                } else res = value_number(value_to_number(l) + value_to_number(r));
            }
            else if (strcmp(op, "-") == 0) res = value_number(value_to_number(l) - value_to_number(r));
            else if (strcmp(op, "*") == 0) res = value_number(value_to_number(l) * value_to_number(r));
            else if (strcmp(op, "/") == 0) { double rv = value_to_number(r); res = value_number(rv != 0 ? value_to_number(l) / rv : 0); }
            else if (strcmp(op, "%") == 0) res = value_number(fmod(value_to_number(l), value_to_number(r)));
            else if (strcmp(op, "==") == 0) res = value_bool(value_equals(l, r));
            else if (strcmp(op, "!=") == 0) res = value_bool(!value_equals(l, r));
            else if (strcmp(op, ">") == 0) res = value_bool(value_to_number(l) > value_to_number(r));
            else if (strcmp(op, "<") == 0) res = value_bool(value_to_number(l) < value_to_number(r));
            else if (strcmp(op, ">=") == 0) res = value_bool(value_to_number(l) >= value_to_number(r));
            else if (strcmp(op, "<=") == 0) res = value_bool(value_to_number(l) <= value_to_number(r));
            else if (strcmp(op, "&&") == 0) res = value_bool(value_is_truthy(l) && value_is_truthy(r));
            else if (strcmp(op, "||") == 0) res = value_bool(value_is_truthy(l) || value_is_truthy(r));
            else res = value_null();
            value_release(l); value_release(r);
            return res;
        }
        case HCS_AST_UNARY_EXPR: {
            HcsValue* o = eval_expression(rt, node->data.unary.operand);
            const char* op = node->data.unary.op;
            HcsValue* res;
            if (strcmp(op, "-") == 0) res = value_number(-value_to_number(o));
            else if (strcmp(op, "!") == 0 || strcmp(op, "not") == 0) res = value_bool(!value_is_truthy(o));
            else res = value_null();
            value_release(o);
            return res;
        }
        case HCS_AST_FUNC_CALL:
        case HCS_AST_FUNC_CALL_EXPR: {
            HcsValue* r = call_builtin(rt, node->data.func_call.name, &node->data.func_call.args);
            if (r) return r;
            HcsAstNode* fn = find_function(rt, node->data.func_call.name);
            if (fn) {
                HcsScope* fs = scope_create(rt->global_scope);
                for (int i = 0; i < fn->data.func_decl.param_count && i < node->data.func_call.args.count; i++) {
                    HcsValue* arg = eval_expression(rt, node->data.func_call.args.items[i]);
                    scope_set(fs, fn->data.func_decl.params[i], arg, false);
                }
                HcsScope* old = rt->current_scope;
                rt->current_scope = fs;
                rt->should_return = false;
                rt->return_value = NULL;
                for (int i = 0; i < fn->data.func_decl.body.count; i++) {
                    execute_statement(rt, fn->data.func_decl.body.items[i]);
                    if (rt->should_return) break;
                }
                r = rt->return_value ? rt->return_value : value_null();
                rt->should_return = false;
                rt->return_value = NULL;
                rt->current_scope = old;
                scope_free(fs);
                return r;
            }
            return value_null();
        }
        case HCS_AST_METHOD_CALL: {
            HcsValue* obj = eval_expression(rt, node->data.method_call.object);
            const char* m = node->data.method_call.method;
            HcsValue* res = value_null();
            
            // Check if object is a variable reference (like HalGUI, Audio, etc.)
            // If so, treat as a namespaced function call
            if (node->data.method_call.object->type == HCS_AST_VAR_REF) {
                char* full_name = malloc(strlen(node->data.method_call.object->data.var_ref.var_name) + strlen(m) + 2);
                sprintf(full_name, "%s.%s", node->data.method_call.object->data.var_ref.var_name, m);
                
                // Try to call as builtin function
                HcsValue* r = call_builtin(rt, full_name, &node->data.method_call.args);
                free(full_name);
                value_release(obj);
                if (r) return r;
                return value_null();
            }
            
            // Otherwise, handle as object method
            if (obj->type == HCS_VAL_STRING) {
                if (strcmp(m, "upper") == 0) {
                    char* s = strdup(obj->data.string);
                    for (char* p = s; *p; p++) *p = toupper(*p);
                    res = value_string(s); free(s);
                } else if (strcmp(m, "lower") == 0) {
                    char* s = strdup(obj->data.string);
                    for (char* p = s; *p; p++) *p = tolower(*p);
                    res = value_string(s); free(s);
                } else if (strcmp(m, "toUpperCase") == 0) {
                    char* s = strdup(obj->data.string);
                    for (char* p = s; *p; p++) *p = toupper(*p);
                    res = value_string(s); free(s);
                } else if (strcmp(m, "toLowerCase") == 0) {
                    char* s = strdup(obj->data.string);
                    for (char* p = s; *p; p++) *p = tolower(*p);
                    res = value_string(s); free(s);
                } else if (strcmp(m, "trim") == 0) {
                    char* s = strdup(obj->data.string);
                    char* start = s;
                    while (*start && isspace(*start)) start++;
                    char* end = s + strlen(s) - 1;
                    while (end > start && isspace(*end)) end--;
                    *(end + 1) = '\0';
                    res = value_string(start); free(s);
                } else if (strcmp(m, "contains") == 0 && node->data.method_call.args.count > 0) {
                    HcsValue* search_v = eval_expression(rt, node->data.method_call.args.items[0]);
                    char* search = value_to_string(search_v);
                    res = value_bool(strstr(obj->data.string, search) != NULL);
                    free(search); value_release(search_v);
                } else if (strcmp(m, "startsWith") == 0 && node->data.method_call.args.count > 0) {
                    HcsValue* prefix_v = eval_expression(rt, node->data.method_call.args.items[0]);
                    char* prefix = value_to_string(prefix_v);
                    res = value_bool(strncmp(obj->data.string, prefix, strlen(prefix)) == 0);
                    free(prefix); value_release(prefix_v);
                } else if (strcmp(m, "endsWith") == 0 && node->data.method_call.args.count > 0) {
                    HcsValue* suffix_v = eval_expression(rt, node->data.method_call.args.items[0]);
                    char* suffix = value_to_string(suffix_v);
                    int slen = strlen(obj->data.string);
                    int suflen = strlen(suffix);
                    res = value_bool(slen >= suflen && strcmp(obj->data.string + slen - suflen, suffix) == 0);
                    free(suffix); value_release(suffix_v);
                } else if (strcmp(m, "indexOf") == 0 && node->data.method_call.args.count > 0) {
                    HcsValue* search_v = eval_expression(rt, node->data.method_call.args.items[0]);
                    char* search = value_to_string(search_v);
                    char* found = strstr(obj->data.string, search);
                    res = value_number(found ? (found - obj->data.string) : -1);
                    free(search); value_release(search_v);
                } else if (strcmp(m, "substring") == 0 && node->data.method_call.args.count > 0) {
                    HcsValue* start_v = eval_expression(rt, node->data.method_call.args.items[0]);
                    int start = (int)value_to_number(start_v);
                    int len = strlen(obj->data.string);
                    if (start < 0) start = 0;
                    if (start > len) start = len;
                    int end = len;
                    if (node->data.method_call.args.count > 1) {
                        HcsValue* end_v = eval_expression(rt, node->data.method_call.args.items[1]);
                        end = (int)value_to_number(end_v);
                        value_release(end_v);
                        if (end > len) end = len;
                        if (end < start) end = start;
                    }
                    char* sub = (char*)malloc(end - start + 1);
                    strncpy(sub, obj->data.string + start, end - start);
                    sub[end - start] = '\0';
                    res = value_string(sub);
                    free(sub);
                    value_release(start_v);
                } else if (strcmp(m, "replace") == 0 && node->data.method_call.args.count >= 2) {
                    HcsValue* old_v = eval_expression(rt, node->data.method_call.args.items[0]);
                    HcsValue* new_v = eval_expression(rt, node->data.method_call.args.items[1]);
                    char* old_str = value_to_string(old_v);
                    char* new_str = value_to_string(new_v);
                    char* src = obj->data.string;
                    int old_len = strlen(old_str);
                    int new_len = strlen(new_str);
                    int count = 0;
                    char* p = src;
                    while ((p = strstr(p, old_str)) != NULL) { count++; p += old_len; }
                    int result_len = strlen(src) + count * (new_len - old_len);
                    char* result = (char*)malloc(result_len + 1);
                    char* dest = result;
                    p = src;
                    while (*p) {
                        if (strncmp(p, old_str, old_len) == 0) {
                            strcpy(dest, new_str);
                            dest += new_len;
                            p += old_len;
                        } else {
                            *dest++ = *p++;
                        }
                    }
                    *dest = '\0';
                    res = value_string(result);
                    free(result); free(old_str); free(new_str);
                    value_release(old_v); value_release(new_v);
                } else if (strcmp(m, "split") == 0 && node->data.method_call.args.count > 0) {
                    HcsValue* delim_v = eval_expression(rt, node->data.method_call.args.items[0]);
                    char* delim = value_to_string(delim_v);
                    res = value_array();
                    char* str = strdup(obj->data.string);
                    char* token = strtok(str, delim);
                    while (token) {
                        value_array_push(res, value_string(token));
                        token = strtok(NULL, delim);
                    }
                    free(str); free(delim);
                    value_release(delim_v);
                }
            } else if (obj->type == HCS_VAL_ARRAY) {
                if (strcmp(m, "push") == 0 && node->data.method_call.args.count > 0) {
                    HcsValue* item = eval_expression(rt, node->data.method_call.args.items[0]);
                    value_array_push(obj, item);
                    res = value_number(value_array_length(obj));
                } else if (strcmp(m, "pop") == 0 && obj->data.array.count > 0) {
                    res = value_copy(value_array_get(obj, obj->data.array.count - 1));
                    obj->data.array.count--;
                } else if (strcmp(m, "shift") == 0 && obj->data.array.count > 0) {
                    res = value_copy(value_array_get(obj, 0));
                    for (int i = 0; i < obj->data.array.count - 1; i++) {
                        obj->data.array.items[i] = obj->data.array.items[i + 1];
                    }
                    obj->data.array.count--;
                } else if (strcmp(m, "unshift") == 0 && node->data.method_call.args.count > 0) {
                    HcsValue* item = eval_expression(rt, node->data.method_call.args.items[0]);
                    if (obj->data.array.count >= obj->data.array.capacity) {
                        int nc = obj->data.array.capacity == 0 ? 8 : obj->data.array.capacity * 2;
                        obj->data.array.items = realloc(obj->data.array.items, sizeof(HcsValue*) * nc);
                        obj->data.array.capacity = nc;
                    }
                    for (int i = obj->data.array.count; i > 0; i--) {
                        obj->data.array.items[i] = obj->data.array.items[i - 1];
                    }
                    obj->data.array.items[0] = item;
                    obj->data.array.count++;
                    res = value_number(obj->data.array.count);
                } else if (strcmp(m, "indexOf") == 0 && node->data.method_call.args.count > 0) {
                    HcsValue* search = eval_expression(rt, node->data.method_call.args.items[0]);
                    int idx = -1;
                    for (int i = 0; i < obj->data.array.count; i++) {
                        if (value_equals(obj->data.array.items[i], search)) {
                            idx = i;
                            break;
                        }
                    }
                    value_release(search);
                    res = value_number(idx);
                } else if (strcmp(m, "includes") == 0 && node->data.method_call.args.count > 0) {
                    HcsValue* search = eval_expression(rt, node->data.method_call.args.items[0]);
                    bool found = false;
                    for (int i = 0; i < obj->data.array.count; i++) {
                        if (value_equals(obj->data.array.items[i], search)) {
                            found = true;
                            break;
                        }
                    }
                    value_release(search);
                    res = value_bool(found);
                } else if (strcmp(m, "join") == 0) {
                    char* delim = ",";
                    char* custom_delim = NULL;
                    if (node->data.method_call.args.count > 0) {
                        HcsValue* delim_v = eval_expression(rt, node->data.method_call.args.items[0]);
                        custom_delim = value_to_string(delim_v);
                        delim = custom_delim;
                        value_release(delim_v);
                    }
                    int total_len = 0;
                    char** strs = (char**)malloc(sizeof(char*) * obj->data.array.count);
                    for (int i = 0; i < obj->data.array.count; i++) {
                        strs[i] = value_to_string(obj->data.array.items[i]);
                        total_len += strlen(strs[i]);
                    }
                    total_len += strlen(delim) * (obj->data.array.count > 0 ? obj->data.array.count - 1 : 0);
                    char* result = (char*)malloc(total_len + 1);
                    result[0] = '\0';
                    for (int i = 0; i < obj->data.array.count; i++) {
                        if (i > 0) strcat(result, delim);
                        strcat(result, strs[i]);
                        free(strs[i]);
                    }
                    free(strs);
                    res = value_string(result);
                    free(result);
                    if (custom_delim) free(custom_delim);
                } else if (strcmp(m, "reverse") == 0) {
                    for (int i = 0; i < obj->data.array.count / 2; i++) {
                        HcsValue* tmp = obj->data.array.items[i];
                        obj->data.array.items[i] = obj->data.array.items[obj->data.array.count - 1 - i];
                        obj->data.array.items[obj->data.array.count - 1 - i] = tmp;
                    }
                    res = value_copy(obj);
                } else if (strcmp(m, "slice") == 0) {
                    int start = 0, end = obj->data.array.count;
                    if (node->data.method_call.args.count > 0) {
                        HcsValue* start_v = eval_expression(rt, node->data.method_call.args.items[0]);
                        start = (int)value_to_number(start_v);
                        value_release(start_v);
                        if (start < 0) start = obj->data.array.count + start;
                        if (start < 0) start = 0;
                    }
                    if (node->data.method_call.args.count > 1) {
                        HcsValue* end_v = eval_expression(rt, node->data.method_call.args.items[1]);
                        end = (int)value_to_number(end_v);
                        value_release(end_v);
                        if (end < 0) end = obj->data.array.count + end;
                        if (end > obj->data.array.count) end = obj->data.array.count;
                    }
                    res = value_array();
                    for (int i = start; i < end; i++) {
                        value_array_push(res, value_copy(obj->data.array.items[i]));
                    }
                }
            }
            value_release(obj);
            return res;
        }
        default: return value_null();
    }
}

void execute_statement(HcsRuntime* rt, HcsAstNode* node) {
    if (!node || !rt->running) return;
    if (rt->should_return || rt->should_break || rt->should_continue) return;
    
    switch (node->type) {
        case HCS_AST_VAR_DECL: {
            HcsValue* v = node->data.var_decl.init_value ? eval_expression(rt, node->data.var_decl.init_value) : value_null();
            HcsScope* t = node->data.var_decl.is_global ? rt->global_scope : rt->current_scope;
            
            FILE* log = fopen("debug.log", "a");
            if (log) {
                fprintf(log, "[VAR_DECL] name=%s is_global=%d is_const=%d scope=%p global_scope=%p\n", 
                    node->data.var_decl.name, 
                    node->data.var_decl.is_global,
                    node->data.var_decl.is_const,
                    (void*)t,
                    (void*)rt->global_scope);
                fclose(log);
            }
            
            scope_set(t, node->data.var_decl.name, v, node->data.var_decl.is_const);
            
            log = fopen("debug.log", "a");
            if (log) {
                fprintf(log, "[VAR_DECL] After set: global_scope has %d variables\n", rt->global_scope->var_count);
                fclose(log);
            }
            break;
        }
        case HCS_AST_ASSIGNMENT: {
            HcsValue* v = eval_expression(rt, node->data.assignment.value);
            const char* op = node->data.assignment.op;
            if (strcmp(op, "=") != 0) {
                HcsValue* old = scope_get(rt->current_scope, node->data.assignment.var_name);
                if (old) {
                    double on = value_to_number(old), nn = value_to_number(v);
                    value_release(v);
                    if (strcmp(op, "+=") == 0) {
                        if (old->type == HCS_VAL_STRING) {
                            char* s = value_to_string(v);
                            char* c = malloc(strlen(old->data.string) + strlen(s) + 1);
                            strcpy(c, old->data.string); strcat(c, s);
                            v = value_string(c); free(c); free(s);
                        } else v = value_number(on + nn);
                    }
                    else if (strcmp(op, "-=") == 0) v = value_number(on - nn);
                    else if (strcmp(op, "*=") == 0) v = value_number(on * nn);
                    else if (strcmp(op, "/=") == 0) v = value_number(nn != 0 ? on / nn : 0);
                }
            }
            scope_set(rt->current_scope, node->data.assignment.var_name, v, false);
            break;
        }
        case HCS_AST_ARRAY_ASSIGNMENT: {
            HcsValue* arr = scope_get(rt->current_scope, node->data.array_assign.array_name);
            if (arr && arr->type == HCS_VAL_ARRAY) {
                HcsValue* idx = eval_expression(rt, node->data.array_assign.index);
                HcsValue* val = eval_expression(rt, node->data.array_assign.value);
                value_array_set(arr, (int)value_to_number(idx), val);
                value_release(idx);
            }
            break;
        }
        case HCS_AST_FUNC_DECL:
            add_function(rt, node->data.func_decl.name, node);
            break;
        case HCS_AST_FUNC_CALL: {
            HcsValue* r = eval_expression(rt, node);
            value_release(r);
            break;
        }
        case HCS_AST_RETURN:
            rt->return_value = node->data.return_stmt.value ? eval_expression(rt, node->data.return_stmt.value) : value_null();
            rt->should_return = true;
            break;
        case HCS_AST_BREAK: rt->should_break = true; break;
        case HCS_AST_CONTINUE: rt->should_continue = true; break;
        case HCS_AST_IF: {
            HcsValue* c = eval_expression(rt, node->data.if_stmt.condition);
            bool t = value_is_truthy(c); value_release(c);
            HcsAstList* b = t ? &node->data.if_stmt.then_body : &node->data.if_stmt.else_body;
            for (int i = 0; i < b->count; i++) {
                execute_statement(rt, b->items[i]);
                if (rt->should_return || rt->should_break || rt->should_continue) break;
            }
            break;
        }
        case HCS_AST_WHILE: {
            while (rt->running && !rt->should_return && !rt->should_break) {
                HcsValue* c = eval_expression(rt, node->data.while_loop.condition);
                bool cont = value_is_truthy(c); value_release(c);
                if (!cont) break;
                rt->should_continue = false;
                for (int i = 0; i < node->data.while_loop.body.count; i++) {
                    execute_statement(rt, node->data.while_loop.body.items[i]);
                    if (rt->should_return || rt->should_break || rt->should_continue) break;
                }
            }
            rt->should_break = false; rt->should_continue = false;
            break;
        }
        case HCS_AST_FOR: {
            HcsValue* from = eval_expression(rt, node->data.for_loop.from);
            HcsValue* to = eval_expression(rt, node->data.for_loop.to);
            HcsValue* step_v = node->data.for_loop.step ? eval_expression(rt, node->data.for_loop.step) : value_number(1);
            double i = value_to_number(from), end = value_to_number(to), step = value_to_number(step_v);
            value_release(from); value_release(to); value_release(step_v);
            while (rt->running && !rt->should_return && !rt->should_break) {
                if ((step > 0 && i > end) || (step < 0 && i < end)) break;
                scope_set(rt->current_scope, node->data.for_loop.var_name, value_number(i), false);
                rt->should_continue = false;
                for (int j = 0; j < node->data.for_loop.body.count; j++) {
                    execute_statement(rt, node->data.for_loop.body.items[j]);
                    if (rt->should_return || rt->should_break || rt->should_continue) break;
                }
                i += step;
            }
            rt->should_break = false; rt->should_continue = false;
            break;
        }
        case HCS_AST_FOR_EACH: {
            HcsValue* coll = eval_expression(rt, node->data.foreach_loop.collection);
            if (coll->type == HCS_VAL_ARRAY) {
                for (int i = 0; i < coll->data.array.count && rt->running && !rt->should_return && !rt->should_break; i++) {
                    scope_set(rt->current_scope, node->data.foreach_loop.var_name, value_copy(coll->data.array.items[i]), false);
                    rt->should_continue = false;
                    for (int j = 0; j < node->data.foreach_loop.body.count; j++) {
                        execute_statement(rt, node->data.foreach_loop.body.items[j]);
                        if (rt->should_return || rt->should_break || rt->should_continue) break;
                    }
                }
            }
            value_release(coll);
            rt->should_break = false; rt->should_continue = false;
            break;
        }
        case HCS_AST_PRINT: {
            for (int i = 0; i < node->data.print.values.count; i++) {
                HcsValue* v = eval_expression(rt, node->data.print.values.items[i]);
                char* s = value_to_string(v);
                
                // Use printf for simple console output
                printf("%s", s);
                
                if (i < node->data.print.values.count - 1) {
                    printf(" ");
                }
                free(s); value_release(v);
            }
            printf("\n");
            fflush(stdout);
            break;
        }
        case HCS_AST_ALERT: {
            HcsValue* msg = eval_expression(rt, node->data.alert.message);
            char* ms = value_to_string(msg);
            msgbox_utf8(ms, "HalcyonScript", MB_OK | MB_ICONINFORMATION);
            free(ms); value_release(msg);
            break;
        }
        case HCS_AST_SET_PROPERTY: {
            // Check if HalGUI mode is active
            extern bool gui_is_halgui_mode(void);
            extern void halgui_set_property(HcsRuntime* rt, HcsAstNode* node);
            
            FILE* log = fopen("debug.log", "a");
            if (log) {
                fprintf(log, "[RUNTIME] SET_PROPERTY: halgui_mode=%d\n", gui_is_halgui_mode());
                fclose(log);
            }
            
            if (gui_is_halgui_mode()) {
                halgui_set_property(rt, node);
            } else {
                HcsGuiControl* ctrl = gui_find_control(rt, node->data.set_prop.element_name);
                if (ctrl && ctrl->hwnd) {
                    HcsValue* v = eval_expression(rt, node->data.set_prop.value);
                    const char* prop = node->data.set_prop.property;
                    if (strcmp(prop, "text") == 0) {
                        char* t = value_to_string(v);
                        SetWindowTextA(ctrl->hwnd, t);
                        free(t);
                    } else if (strcmp(prop, "visible") == 0) {
                        ShowWindow(ctrl->hwnd, value_is_truthy(v) ? SW_SHOW : SW_HIDE);
                    } else if (strcmp(prop, "enabled") == 0) {
                        EnableWindow(ctrl->hwnd, value_is_truthy(v));
                    }
                    value_release(v);
                }
            }
            break;
        }
        case HCS_AST_GET_PROPERTY: {
            // Check if HalGUI mode is active
            extern bool gui_is_halgui_mode(void);
            extern void halgui_get_property(HcsRuntime* rt, HcsAstNode* node);
            
            if (gui_is_halgui_mode()) {
                halgui_get_property(rt, node);
            } else {
                HcsGuiControl* ctrl = gui_find_control(rt, node->data.get_prop.element_name);
                HcsValue* r = value_null();
                if (ctrl && ctrl->hwnd) {
                    const char* prop = node->data.get_prop.property;
                    if (strcmp(prop, "text") == 0) {
                        int len = GetWindowTextLengthA(ctrl->hwnd);
                        char* t = malloc(len + 1);
                        GetWindowTextA(ctrl->hwnd, t, len + 1);
                        r = value_string(t);
                        free(t);
                    } else if (strcmp(prop, "visible") == 0) {
                        r = value_bool(IsWindowVisible(ctrl->hwnd));
                    } else if (strcmp(prop, "enabled") == 0) {
                        r = value_bool(IsWindowEnabled(ctrl->hwnd));
                    }
                }
                scope_set(rt->current_scope, node->data.get_prop.result_var, r, false);
            }
            break;
        }
        case HCS_AST_WAIT: {
            HcsValue* d = eval_expression(rt, node->data.wait.duration);
            Sleep((DWORD)value_to_number(d));
            value_release(d);
            break;
        }
        default: break;
    }
}

void runtime_execute(HcsRuntime* rt, HcsAstNode* program) {
    if (!program || program->type != HCS_AST_PROGRAM) return;
    
    for (int i = 0; i < program->data.program.statements.count; i++) {
        execute_statement(rt, program->data.program.statements.items[i]);
        if (!rt->running) break;
    }
}

HcsValue* runtime_eval(HcsRuntime* rt, HcsAstNode* node) { return eval_expression(rt, node); }
