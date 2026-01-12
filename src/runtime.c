/*
 * HalcyonScript - Runtime implementation
 */

#include "runtime.h"
#include <stdio.h>
#include <math.h>

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

// Flag to use HalGUI instead of Win32
static bool g_use_halgui = false;

void execute_statement(HcsRuntime* rt, HcsAstNode* node);
static HcsValue* eval_expression(HcsRuntime* rt, HcsAstNode* node);

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
        
        printf("[HALGUI] Function called: %s\n", func);
        fflush(stdout);
        
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
    
    return NULL;
}

static HcsValue* eval_expression(HcsRuntime* rt, HcsAstNode* node) {
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
                }
            } else if (obj->type == HCS_VAL_ARRAY) {
                if (strcmp(m, "push") == 0 && node->data.method_call.args.count > 0) {
                    HcsValue* item = eval_expression(rt, node->data.method_call.args.items[0]);
                    value_array_push(obj, item);
                    res = value_number(value_array_length(obj));
                } else if (strcmp(m, "pop") == 0 && obj->data.array.count > 0) {
                    res = value_copy(value_array_get(obj, obj->data.array.count - 1));
                    obj->data.array.count--;
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
                printf("%s", s);
                if (i < node->data.print.values.count - 1) printf(" ");
                free(s); value_release(v);
            }
            printf("\n");
            break;
        }
        case HCS_AST_ALERT: {
            HcsValue* msg = eval_expression(rt, node->data.alert.message);
            char* ms = value_to_string(msg);
            MessageBoxA(NULL, ms, "HalcyonScript", MB_OK | MB_ICONINFORMATION);
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
    
    FILE* log = fopen("debug.log", "a");
    if (log) {
        fprintf(log, "[RUNTIME_EXECUTE] Program has %d statements\n", program->data.program.statements.count);
        fclose(log);
    }
    
    for (int i = 0; i < program->data.program.statements.count; i++) {
        log = fopen("debug.log", "a");
        if (log) {
            fprintf(log, "[RUNTIME_EXECUTE] Statement %d type=%d\n", i, program->data.program.statements.items[i]->type);
            fclose(log);
        }
        execute_statement(rt, program->data.program.statements.items[i]);
        if (!rt->running) break;
    }
}

HcsValue* runtime_eval(HcsRuntime* rt, HcsAstNode* node) { return eval_expression(rt, node); }
