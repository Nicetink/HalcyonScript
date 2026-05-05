/*
 * HalcyonScript - HalGUI Runtime Integration
 * 
 * Connects HalcyonScript syntax to HalGUI C API
 */

#include "runtime.h"
#include "halgui/halgui.h"
#include <stdio.h>
#include <string.h>

/* ============================================
   HalGUI Runtime State
   ============================================ */

typedef struct {
    char* name;
    HalWidget* widget;
    HalWidgetType type;
} HalGuiWidget;

typedef struct {
    HalWindow* main_window;
    HalGuiWidget* widgets;
    int widget_count;
    int widget_capacity;
    HcsRuntime* hcs_runtime;
    bool initialized;
    HalWidget* current_panel;  // Track current panel for layout
} HalGuiRuntime;

static HalGuiRuntime g_halgui = {0};

/* ============================================
   Widget Management
   ============================================ */

static void halgui_add_widget(const char* name, HalWidget* widget, HalWidgetType type) {
    if (g_halgui.widget_count >= g_halgui.widget_capacity) {
        int nc = g_halgui.widget_capacity == 0 ? 32 : g_halgui.widget_capacity * 2;
        g_halgui.widgets = realloc(g_halgui.widgets, sizeof(HalGuiWidget) * nc);
        g_halgui.widget_capacity = nc;
    }
    g_halgui.widgets[g_halgui.widget_count].name = strdup(name);
    g_halgui.widgets[g_halgui.widget_count].widget = widget;
    g_halgui.widgets[g_halgui.widget_count].type = type;
    g_halgui.widget_count++;
}

static HalWidget* halgui_find_widget(const char* name) {
    for (int i = 0; i < g_halgui.widget_count; i++) {
        if (strcmp(g_halgui.widgets[i].name, name) == 0) {
            return g_halgui.widgets[i].widget;
        }
    }
    return NULL;
}

/* ============================================
   Event Handlers Bridge
   ============================================ */

typedef struct {
    char* element_name;
    char* event_type;
    HcsAstNode* handler;
} HalGuiHandler;

static HalGuiHandler* g_handlers = NULL;
static int g_handler_count = 0;
static int g_handler_capacity = 0;

static void halgui_add_handler(const char* element, const char* event, HcsAstNode* handler) {
    if (g_handler_count >= g_handler_capacity) {
        int nc = g_handler_capacity == 0 ? 32 : g_handler_capacity * 2;
        g_handlers = realloc(g_handlers, sizeof(HalGuiHandler) * nc);
        g_handler_capacity = nc;
    }
    g_handlers[g_handler_count].element_name = strdup(element);
    g_handlers[g_handler_count].event_type = strdup(event);
    g_handlers[g_handler_count].handler = handler;
    g_handler_count++;
}

static void halgui_fire_event(const char* element, const char* event) {
    FILE* log = fopen("debug.log", "a");
    if (log) {
        fprintf(log, "[FIRE_EVENT] element=%s event=%s handler_count=%d\n", element, event, g_handler_count);
        fclose(log);
    }
    
    if (!g_halgui.hcs_runtime) return;
    
    // Need to call execute_statement directly, not runtime_execute
    extern void execute_statement(HcsRuntime* rt, HcsAstNode* node);
    
    for (int i = 0; i < g_handler_count; i++) {
        if (strcmp(g_handlers[i].element_name, element) == 0 &&
            strcmp(g_handlers[i].event_type, event) == 0) {
            log = fopen("debug.log", "a");
            if (log) {
                fprintf(log, "[FIRE_EVENT] MATCH! Handler found\n");
                fclose(log);
            }
            
            HcsAstNode* h = g_handlers[i].handler;
            if (h && h->type == HCS_AST_EVENT_HANDLER) {
                log = fopen("debug.log", "a");
                if (log) {
                    fprintf(log, "[FIRE_EVENT] Handler has %d statements\n", h->data.event_handler.body.count);
                    fclose(log);
                }
                
                // CRITICAL FIX: Event handlers should use global scope as parent
                // This ensures global variables are accessible
                HcsScope* es = scope_create(g_halgui.hcs_runtime->global_scope);
                HcsScope* old = g_halgui.hcs_runtime->current_scope;
                g_halgui.hcs_runtime->current_scope = es;
                
                // Debug: Check global variables
                log = fopen("debug.log", "a");
                if (log) {
                    fprintf(log, "[FIRE_EVENT] Global scope has %d variables\n", g_halgui.hcs_runtime->global_scope->var_count);
                    for (int k = 0; k < g_halgui.hcs_runtime->global_scope->var_count; k++) {
                        fprintf(log, "[FIRE_EVENT]   var[%d]: %s\n", k, g_halgui.hcs_runtime->global_scope->vars[k].name);
                    }
                    fclose(log);
                }
                
                for (int j = 0; j < h->data.event_handler.body.count; j++) {
                    HcsAstNode* stmt = h->data.event_handler.body.items[j];
                    execute_statement(g_halgui.hcs_runtime, stmt);
                    if (g_halgui.hcs_runtime->should_return || g_halgui.hcs_runtime->should_break) break;
                }
                
                g_halgui.hcs_runtime->current_scope = old;
                scope_free(es);
            }
        }
    }
}

/* Generic click handler */
static void on_widget_click(HalWidget* widget, HalEvent* event, void* userData) {
    const char* name = (const char*)userData;
    printf("[CLICK] Button clicked! name=%s\n", name ? name : "NULL");
    fflush(stdout);
    if (name) {
        printf("[CLICK] Firing event for %s\n", name);
        fflush(stdout);
        halgui_fire_event(name, "clicked");
        printf("[CLICK] Event fired\n");
        fflush(stdout);
    }
    event->handled = true;
}

static void on_widget_change(HalWidget* widget, HalEvent* event, void* userData) {
    const char* name = (const char*)userData;
    if (name) {
        halgui_fire_event(name, "changed");
    }
    event->handled = true;
}

/* ============================================
   HalGUI Initialization
   ============================================ */

void halgui_runtime_init(HcsRuntime* rt) {
    if (g_halgui.initialized) return;
    
    if (!hal_init()) {
        fprintf(stderr, "Failed to initialize HalGUI\n");
        return;
    }
    
    hal_set_theme(&HAL_THEME_DARK);
    g_halgui.hcs_runtime = rt;
    g_halgui.initialized = true;
}

void halgui_runtime_shutdown(void) {
    if (!g_halgui.initialized) return;
    
    for (int i = 0; i < g_halgui.widget_count; i++) {
        free(g_halgui.widgets[i].name);
    }
    free(g_halgui.widgets);
    
    for (int i = 0; i < g_handler_count; i++) {
        free(g_handlers[i].element_name);
        free(g_handlers[i].event_type);
    }
    free(g_handlers);
    
    hal_shutdown();
    
    memset(&g_halgui, 0, sizeof(g_halgui));
    g_handlers = NULL;
    g_handler_count = 0;
    g_handler_capacity = 0;
}

/* ============================================
   Create Window (HalGUI version)
   ============================================ */

void halgui_create_window(HcsRuntime* rt, HcsAstNode* node) {
    if (!g_halgui.initialized) {
        halgui_runtime_init(rt);
    }
    
    const char* name = node->data.create_window.name;
    const char* title = node->data.create_window.title ? node->data.create_window.title : "HalcyonScript";
    int width = node->data.create_window.width;
    int height = node->data.create_window.height;
    
    HalWindow* window = hal_window_create(title, width, height);
    if (!window) {
        fprintf(stderr, "Failed to create HalGUI window\n");
        return;
    }
    
    // Store original window size for responsive layout
    window->restoreBounds.width = width;
    window->restoreBounds.height = height;
    
    // Apply current global theme to window
    window->theme = hal_get_theme();
    
    g_halgui.main_window = window;
    halgui_add_widget(name, (HalWidget*)window, HAL_WIDGET_WINDOW);
    
    hal_window_center(window);
}

/* ============================================
   Create Control (HalGUI version)
   ============================================ */

void halgui_create_control(HcsRuntime* rt, HcsAstNode* node) {
    if (!g_halgui.main_window) return;
    
    const char* type = node->data.create_control.control_type;
    const char* name = node->data.create_control.name;
    const char* text = node->data.create_control.text ? node->data.create_control.text : "";
    HcsPropertyList* props = &node->data.create_control.properties;
    
    int x = 10, y = 10, w = 100, h = 30;
    
    for (int i = 0; i < props->count; i++) {
        const char* pn = props->items[i].name;
        HcsAstNode* pv = props->items[i].value;
        if (!pv) continue;
        
        if (pv->type == HCS_AST_NUMBER) {
            int val = (int)pv->data.number_value;
            if (strcmp(pn, "x") == 0) x = val;
            else if (strcmp(pn, "y") == 0) y = val;
            else if (strcmp(pn, "width") == 0) w = val;
            else if (strcmp(pn, "height") == 0) h = val;
        }
    }
    
    HalWidget* parent = g_halgui.current_panel ? g_halgui.current_panel : (HalWidget*)g_halgui.main_window;
    HalWidget* widget = NULL;
    HalWidgetType wtype = HAL_WIDGET_CUSTOM;
    
    if (strcmp(type, "button") == 0) {
        widget = hal_button_create(parent, text);
        wtype = HAL_WIDGET_BUTTON;
        hal_widget_on(widget, HAL_EVENT_CLICK, on_widget_click, (void*)strdup(name));
    }
    else if (strcmp(type, "label") == 0) {
        widget = hal_label_create(parent, text);
        wtype = HAL_WIDGET_LABEL;
    }
    else if (strcmp(type, "input") == 0) {
        widget = hal_input_create(parent, text);
        wtype = HAL_WIDGET_INPUT;
        hal_widget_on(widget, HAL_EVENT_CHANGE, on_widget_change, (void*)strdup(name));
    }
    else if (strcmp(type, "textarea") == 0) {
        widget = hal_textarea_create(parent, text);
        wtype = HAL_WIDGET_TEXTAREA;
        // Native textarea handles its own events
    }
    else if (strcmp(type, "checkbox") == 0) {
        widget = hal_checkbox_create(parent, text);
        wtype = HAL_WIDGET_CHECKBOX;
        hal_widget_on(widget, HAL_EVENT_CLICK, on_widget_click, (void*)strdup(name));
    }
    else if (strcmp(type, "slider") == 0) {
        int min = 0, max = 100, val = 50;
        for (int i = 0; i < props->count; i++) {
            const char* pn = props->items[i].name;
            HcsAstNode* pv = props->items[i].value;
            if (!pv || pv->type != HCS_AST_NUMBER) continue;
            if (strcmp(pn, "min") == 0) min = (int)pv->data.number_value;
            else if (strcmp(pn, "max") == 0) max = (int)pv->data.number_value;
            else if (strcmp(pn, "value") == 0) val = (int)pv->data.number_value;
        }
        widget = hal_slider_create(parent, min, max, val);
        wtype = HAL_WIDGET_SLIDER;
        hal_widget_on(widget, HAL_EVENT_CHANGE, on_widget_change, (void*)strdup(name));
    }
    else if (strcmp(type, "progress") == 0) {
        widget = hal_progress_create(parent);
        wtype = HAL_WIDGET_PROGRESS;
        float val = 0.0f;
        for (int i = 0; i < props->count; i++) {
            const char* pn = props->items[i].name;
            HcsAstNode* pv = props->items[i].value;
            if (!pv || pv->type != HCS_AST_NUMBER) continue;
            if (strcmp(pn, "value") == 0) val = (float)pv->data.number_value / 100.0f;
        }
        hal_progress_set_value(widget, val);
    }
    else if (strcmp(type, "toggle") == 0) {
        widget = hal_toggle_create(parent, text);
        wtype = HAL_WIDGET_TOGGLE;
        hal_widget_on(widget, HAL_EVENT_CLICK, on_widget_click, (void*)strdup(name));
    }
    else if (strcmp(type, "panel") == 0) {
        widget = hal_panel_create(parent);
        wtype = HAL_WIDGET_PANEL;
        // Set this panel as current for child widgets
        g_halgui.current_panel = widget;
        
        // Apply layout immediately after creating all children
        // This will be called when layout is set via HalGUI.setLayout()
    }
    else if (strcmp(type, "calendar") == 0) {
        widget = hal_calendar_create(parent);
        wtype = HAL_WIDGET_CUSTOM;
        hal_widget_on(widget, HAL_EVENT_CHANGE, on_widget_change, (void*)strdup(name));
    }
    
    if (widget) {
        hal_widget_set_bounds(widget, x, y, w, h);
        
        // Auto-calculate percentage constraints based on window size
        int winW = g_halgui.main_window->restoreBounds.width;
        int winH = g_halgui.main_window->restoreBounds.height;
        if (winW == 0) winW = g_halgui.main_window->base.bounds.width;
        if (winH == 0) winH = g_halgui.main_window->base.bounds.height;
        
        if (winW > 0 && winH > 0) {
            float leftPct = (float)x / winW;
            float topPct = (float)y / winH;
            float widthPct = (float)w / winW;
            float heightPct = (float)h / winH;
            hal_widget_set_constraints(widget, leftPct, topPct, widthPct, heightPct);
        }
        
        halgui_add_widget(name, widget, wtype);
    }
}

/* ============================================
   Set Property (HalGUI version)
   ============================================ */

void halgui_set_property(HcsRuntime* rt, HcsAstNode* node) {
    const char* element = node->data.set_prop.element_name;
    const char* prop = node->data.set_prop.property;
    HcsValue* val = runtime_eval(rt, node->data.set_prop.value);
    
    printf("[SET_PROP] element=%s prop=%s\n", element, prop);
    fflush(stdout);
    
    HalWidget* widget = halgui_find_widget(element);
    if (!widget) {
        printf("[SET_PROP] Widget '%s' not found!\n", element);
        fflush(stdout);
        value_release(val);
        return;
    }
    
    printf("[SET_PROP] Widget found, type=%d\n", widget->type);
    fflush(stdout);
    
    if (strcmp(prop, "text") == 0) {
        char* text = value_to_string(val);
        printf("[SET_PROP] Setting text to: '%s'\n", text);
        fflush(stdout);
        switch (widget->type) {
            case HAL_WIDGET_BUTTON:
                hal_button_set_text(widget, text);
                break;
            case HAL_WIDGET_LABEL:
                hal_label_set_text(widget, text);
                break;
            case HAL_WIDGET_INPUT:
                hal_input_set_text(widget, text);
                break;
            case HAL_WIDGET_TEXTAREA:
                hal_textarea_set_text(widget, text);
                break;
            default:
                break;
        }
        free(text);
    }
    else if (strcmp(prop, "value") == 0) {
        double num = value_to_number(val);
        switch (widget->type) {
            case HAL_WIDGET_SLIDER:
                hal_slider_set_value(widget, (int)num);
                break;
            case HAL_WIDGET_PROGRESS:
                hal_progress_set_value(widget, (float)num / 100.0f);
                break;
            default:
                break;
        }
    }
    else if (strcmp(prop, "checked") == 0) {
        if (widget->type == HAL_WIDGET_CHECKBOX) {
            hal_checkbox_set_checked(widget, value_is_truthy(val));
        }
    }
    else if (strcmp(prop, "visible") == 0) {
        if (value_is_truthy(val)) {
            hal_widget_show(widget);
        } else {
            hal_widget_hide(widget);
        }
    }
    else if (strcmp(prop, "enabled") == 0) {
        if (value_is_truthy(val)) {
            hal_widget_enable(widget);
        } else {
            hal_widget_disable(widget);
        }
    }
    
    hal_widget_invalidate(widget);
    value_release(val);
}

/* ============================================
   Get Property (HalGUI version)
   ============================================ */

void halgui_get_property(HcsRuntime* rt, HcsAstNode* node) {
    const char* element = node->data.get_prop.element_name;
    const char* prop = node->data.get_prop.property;
    const char* result_var = node->data.get_prop.result_var;
    
    HalWidget* widget = halgui_find_widget(element);
    HcsValue* result = value_null();
    
    if (widget) {
        if (strcmp(prop, "text") == 0) {
            const char* text = NULL;
            switch (widget->type) {
                case HAL_WIDGET_BUTTON:
                    text = hal_button_get_text(widget);
                    break;
                case HAL_WIDGET_LABEL:
                    text = hal_label_get_text(widget);
                    break;
                case HAL_WIDGET_INPUT:
                    text = hal_input_get_text(widget);
                    break;
                case HAL_WIDGET_TEXTAREA:
                    text = hal_textarea_get_text(widget);
                    break;
                default:
                    break;
            }
            if (text) result = value_string(text);
        }
        else if (strcmp(prop, "value") == 0) {
            switch (widget->type) {
                case HAL_WIDGET_SLIDER:
                    result = value_number(hal_slider_get_value(widget));
                    break;
                case HAL_WIDGET_PROGRESS:
                    result = value_number(hal_progress_get_value(widget) * 100.0);
                    break;
                default:
                    break;
            }
        }
        else if (strcmp(prop, "checked") == 0) {
            if (widget->type == HAL_WIDGET_CHECKBOX) {
                result = value_bool(hal_checkbox_is_checked(widget));
            }
        }
        else if (strcmp(prop, "visible") == 0) {
            result = value_bool(hal_widget_is_visible(widget));
        }
        else if (strcmp(prop, "enabled") == 0) {
            result = value_bool(hal_widget_is_enabled(widget));
        }
    }
    
    scope_set(rt->current_scope, result_var, result, false);
}

/* ============================================
   Register Event Handler
   ============================================ */

void halgui_register_handler(HcsAstNode* node) {
    halgui_add_handler(
        node->data.event_handler.element_name,
        node->data.event_handler.event_type,
        node
    );
}

/* ============================================
   Run HalGUI Event Loop
   ============================================ */

void halgui_run(void) {
    if (!g_halgui.main_window) return;
    
    // Apply layout to all panels before showing window
    for (int i = 0; i < g_halgui.widget_count; i++) {
        HalWidget* widget = g_halgui.widgets[i].widget;
        if (widget && widget->layout != HAL_LAYOUT_NONE) {
            hal_widget_apply_layout(widget);
        }
    }
    
    // hal_window_show will call hal_init_native_widgets internally
    hal_window_show(g_halgui.main_window);
    hal_run();
}

/* ============================================
   Theme Support
   ============================================ */

void halgui_set_theme(const char* theme_name) {
    HalTheme* theme = &HAL_THEME_DARK;
    
    if (strcmp(theme_name, "light") == 0) {
        theme = &HAL_THEME_LIGHT;
    } else if (strcmp(theme_name, "midnight") == 0) {
        theme = &HAL_THEME_MIDNIGHT;
    } else if (strcmp(theme_name, "ocean") == 0) {
        theme = &HAL_THEME_OCEAN;
    } else if (strcmp(theme_name, "teal") == 0) {
        theme = &HAL_THEME_TEAL;
    }
    
    printf("[THEME] Setting theme to: %s\n", theme_name);
    fflush(stdout);
    
    hal_set_theme(theme);
    
    if (g_halgui.main_window) {
        printf("[THEME] Applying to main window\n");
        fflush(stdout);
        hal_window_set_theme(g_halgui.main_window, theme);
    }
}

/* ============================================
   Responsive Layout Support
   ============================================ */

void halgui_enable_responsive(bool enable) {
    if (g_halgui.main_window) {
        hal_window_enable_responsive(g_halgui.main_window, enable);
    }
}

void halgui_set_widget_constraints(const char* name, float leftPct, float topPct, float widthPct, float heightPct) {
    HalWidget* widget = halgui_find_widget(name);
    if (widget) {
        hal_widget_set_constraints(widget, leftPct, topPct, widthPct, heightPct);
    }
}

void halgui_set_widget_anchors(const char* name, int anchors) {
    HalWidget* widget = halgui_find_widget(name);
    if (widget) {
        hal_widget_set_anchors(widget, (HalAnchor)anchors);
    }
}

/* ============================================
   Dialog Support
   ============================================ */

HcsValue* halgui_dialog_message(HcsRuntime* rt, const char* title, const char* message, int buttons, int icon) {
    HalDialogButtons btn = HAL_DIALOG_OK;
    HalDialogIcon ico = HAL_DIALOG_INFO;
    
    switch (buttons) {
        case 1: btn = HAL_DIALOG_OK_CANCEL; break;
        case 2: btn = HAL_DIALOG_YES_NO; break;
        case 3: btn = HAL_DIALOG_YES_NO_CANCEL; break;
    }
    
    switch (icon) {
        case 1: ico = HAL_DIALOG_WARNING; break;
        case 2: ico = HAL_DIALOG_ERROR; break;
        case 3: ico = HAL_DIALOG_QUESTION; break;
    }
    
    int result = hal_dialog_message(g_halgui.main_window, title, message, btn, ico);
    return value_number(result);
}

HcsValue* halgui_dialog_open_file(HcsRuntime* rt, const char* title, const char* filter) {
    char* file = hal_dialog_open_file(g_halgui.main_window, title, filter);
    if (file) {
        HcsValue* result = value_string(file);
        free(file);
        return result;
    }
    return value_null();
}

HcsValue* halgui_dialog_save_file(HcsRuntime* rt, const char* title, const char* filter, const char* default_name) {
    char* file = hal_dialog_save_file(g_halgui.main_window, title, filter, default_name);
    if (file) {
        HcsValue* result = value_string(file);
        free(file);
        return result;
    }
    return value_null();
}

/* ============================================
   Audio Support
   ============================================ */

#include "halgui/halgui_audio.h"

/* Audio player storage */
typedef struct {
    char* name;
    HalAudioPlayer* player;
} HalAudioEntry;

static HalAudioEntry* g_audio_players = NULL;
static int g_audio_player_count = 0;
static int g_audio_player_capacity = 0;

static void halgui_add_audio_player(const char* name, HalAudioPlayer* player) {
    if (g_audio_player_count >= g_audio_player_capacity) {
        int nc = g_audio_player_capacity == 0 ? 8 : g_audio_player_capacity * 2;
        g_audio_players = realloc(g_audio_players, sizeof(HalAudioEntry) * nc);
        g_audio_player_capacity = nc;
    }
    g_audio_players[g_audio_player_count].name = strdup(name);
    g_audio_players[g_audio_player_count].player = player;
    g_audio_player_count++;
}

static HalAudioPlayer* halgui_find_audio_player(const char* name) {
    for (int i = 0; i < g_audio_player_count; i++) {
        if (strcmp(g_audio_players[i].name, name) == 0) {
            return g_audio_players[i].player;
        }
    }
    return NULL;
}

/* Create audio player */
HcsValue* halgui_audio_create(HcsRuntime* rt, const char* name) {
    if (!name) return value_bool(false);
    
    /* Check if already exists */
    if (halgui_find_audio_player(name)) {
        return value_bool(true);
    }
    
    HalAudioPlayer* player = hal_audio_create();
    if (!player) return value_bool(false);
    
    halgui_add_audio_player(name, player);
    printf("[Audio] Created player: %s\n", name);
    return value_bool(true);
}

/* Load audio file */
HcsValue* halgui_audio_load(HcsRuntime* rt, const char* name, const char* filePath) {
    HalAudioPlayer* player = halgui_find_audio_player(name);
    if (!player) {
        /* Auto-create player */
        halgui_audio_create(rt, name);
        player = halgui_find_audio_player(name);
    }
    if (!player) return value_bool(false);
    
    bool result = hal_audio_load(player, filePath);
    return value_bool(result);
}

/* Play audio */
HcsValue* halgui_audio_play(HcsRuntime* rt, const char* name) {
    HalAudioPlayer* player = halgui_find_audio_player(name);
    if (!player) return value_bool(false);
    
    bool result = hal_audio_play(player);
    return value_bool(result);
}

/* Pause audio */
HcsValue* halgui_audio_pause(HcsRuntime* rt, const char* name) {
    HalAudioPlayer* player = halgui_find_audio_player(name);
    if (!player) return value_bool(false);
    
    bool result = hal_audio_pause(player);
    return value_bool(result);
}

/* Stop audio */
HcsValue* halgui_audio_stop(HcsRuntime* rt, const char* name) {
    HalAudioPlayer* player = halgui_find_audio_player(name);
    if (!player) return value_bool(false);
    
    bool result = hal_audio_stop(player);
    return value_bool(result);
}

/* Resume audio */
HcsValue* halgui_audio_resume(HcsRuntime* rt, const char* name) {
    HalAudioPlayer* player = halgui_find_audio_player(name);
    if (!player) return value_bool(false);
    
    bool result = hal_audio_resume(player);
    return value_bool(result);
}

/* Seek to position */
HcsValue* halgui_audio_seek(HcsRuntime* rt, const char* name, int positionMs) {
    HalAudioPlayer* player = halgui_find_audio_player(name);
    if (!player) return value_bool(false);
    
    bool result = hal_audio_seek(player, positionMs);
    return value_bool(result);
}

/* Set volume */
HcsValue* halgui_audio_set_volume(HcsRuntime* rt, const char* name, int volume) {
    HalAudioPlayer* player = halgui_find_audio_player(name);
    if (!player) return value_bool(false);
    
    bool result = hal_audio_set_volume(player, volume);
    return value_bool(result);
}

/* Get volume */
HcsValue* halgui_audio_get_volume(HcsRuntime* rt, const char* name) {
    HalAudioPlayer* player = halgui_find_audio_player(name);
    if (!player) return value_number(0);
    
    return value_number(hal_audio_get_volume(player));
}

/* Get position */
HcsValue* halgui_audio_get_position(HcsRuntime* rt, const char* name) {
    HalAudioPlayer* player = halgui_find_audio_player(name);
    if (!player) return value_number(0);
    
    return value_number(hal_audio_get_position(player));
}

/* Get duration */
HcsValue* halgui_audio_get_duration(HcsRuntime* rt, const char* name) {
    HalAudioPlayer* player = halgui_find_audio_player(name);
    if (!player) return value_number(0);
    
    return value_number(hal_audio_get_duration(player));
}

/* Get state */
HcsValue* halgui_audio_get_state(HcsRuntime* rt, const char* name) {
    HalAudioPlayer* player = halgui_find_audio_player(name);
    if (!player) return value_string("stopped");
    
    HalAudioState state = hal_audio_get_state(player);
    switch (state) {
        case HAL_AUDIO_PLAYING: return value_string("playing");
        case HAL_AUDIO_PAUSED: return value_string("paused");
        default: return value_string("stopped");
    }
}

/* Set loop */
HcsValue* halgui_audio_set_loop(HcsRuntime* rt, const char* name, bool loop) {
    HalAudioPlayer* player = halgui_find_audio_player(name);
    if (!player) return value_bool(false);
    
    bool result = hal_audio_set_loop(player, loop);
    return value_bool(result);
}

/* Set mute */
HcsValue* halgui_audio_set_mute(HcsRuntime* rt, const char* name, bool mute) {
    HalAudioPlayer* player = halgui_find_audio_player(name);
    if (!player) return value_bool(false);
    
    bool result = hal_audio_set_mute(player, mute);
    return value_bool(result);
}

/* Is muted */
HcsValue* halgui_audio_is_muted(HcsRuntime* rt, const char* name) {
    HalAudioPlayer* player = halgui_find_audio_player(name);
    if (!player) return value_bool(false);
    
    return value_bool(hal_audio_is_muted(player));
}

/* Destroy audio player */
HcsValue* halgui_audio_destroy(HcsRuntime* rt, const char* name) {
    for (int i = 0; i < g_audio_player_count; i++) {
        if (strcmp(g_audio_players[i].name, name) == 0) {
            hal_audio_destroy(g_audio_players[i].player);
            free(g_audio_players[i].name);
            
            /* Remove from array */
            for (int j = i; j < g_audio_player_count - 1; j++) {
                g_audio_players[j] = g_audio_players[j + 1];
            }
            g_audio_player_count--;
            return value_bool(true);
        }
    }
    return value_bool(false);
}

/* Process audio events (call in main loop) */
void halgui_process_updates(void) {
    hal_audio_process_events();
    
    /* Update UI for playing audio */
    for (int i = 0; i < g_audio_player_count; i++) {
        HalAudioPlayer* player = g_audio_players[i].player;
        if (!player) continue;
        
        HalAudioState state = hal_audio_get_state(player);
        if (state == HAL_AUDIO_PLAYING) {
            /* Get position and duration */
            int position = hal_audio_get_position(player);
            int duration = hal_audio_get_duration(player);
            
            /* Format time strings */
            int posSec = position / 1000;
            int durSec = duration / 1000;
            int posMin = posSec / 60;
            int posSecs = posSec % 60;
            int durMin = durSec / 60;
            int durSecs = durSec % 60;
            
            char timeStr[64];
            snprintf(timeStr, sizeof(timeStr), "%02d:%02d / %02d:%02d", posMin, posSecs, durMin, durSecs);
            
            /* Update time label if exists */
            HalWidget* lblTime = halgui_find_widget("lblTime");
            if (lblTime && lblTime->type == HAL_WIDGET_LABEL) {
                hal_label_set_text(lblTime, timeStr);
                hal_widget_invalidate(lblTime);
            }
            
            /* Update progress slider if exists and duration > 0 */
            if (duration > 0) {
                HalWidget* sliderProgress = halgui_find_widget("sliderProgress");
                if (sliderProgress && sliderProgress->type == HAL_WIDGET_SLIDER) {
                    int progress = (position * 100) / duration;
                    /* Only update if not being dragged by user */
                    extern HalWidget* g_draggingSlider;
                    if (g_draggingSlider != sliderProgress) {
                        hal_slider_set_value(sliderProgress, progress);
                        hal_widget_invalidate(sliderProgress);
                    }
                }
            }
        }
    }
}

/* Cleanup all audio players */
void halgui_audio_cleanup(void) {
    for (int i = 0; i < g_audio_player_count; i++) {
        hal_audio_destroy(g_audio_players[i].player);
        free(g_audio_players[i].name);
    }
    free(g_audio_players);
    g_audio_players = NULL;
    g_audio_player_count = 0;
    g_audio_player_capacity = 0;
    
    hal_audio_shutdown();
}


/* ============================================
   Extended Widgets - New in 0.27.26 (Security Update)
   ============================================ */

/* TreeView */
extern HalWidget* hal_treeview_create(HalWidget* parent);
extern void* hal_treeview_add_node(HalWidget* tree, void* parent, const char* text, void* userData);
extern void hal_treeview_expand(HalWidget* tree, void* node);
extern void hal_treeview_collapse(HalWidget* tree, void* node);

void halgui_create_treeview(const char* name) {
    HalWidget* parent = g_halgui.main_window ? (HalWidget*)g_halgui.main_window : NULL;
    HalWidget* tree = hal_treeview_create(parent);
    halgui_add_widget(name, tree, HAL_WIDGET_LIST);
}

/* DataGrid */
extern HalWidget* hal_datagrid_create(HalWidget* parent);
extern void hal_datagrid_add_column(HalWidget* grid, const char* name, int width, int align);
extern void hal_datagrid_add_row(HalWidget* grid, const char** cells, void* userData);
extern void hal_datagrid_clear(HalWidget* grid);

void halgui_create_datagrid(const char* name) {
    HalWidget* parent = g_halgui.main_window ? (HalWidget*)g_halgui.main_window : NULL;
    HalWidget* grid = hal_datagrid_create(parent);
    halgui_add_widget(name, grid, HAL_WIDGET_LIST);
}

void halgui_datagrid_add_column(const char* name, const char* colName, int width) {
    HalWidget* grid = halgui_find_widget(name);
    if (grid) {
        hal_datagrid_add_column(grid, colName, width, HAL_ALIGN_LEFT);
    }
}

/* Chart */
extern HalWidget* hal_chart_create(HalWidget* parent, int type);
extern void hal_chart_add_series(HalWidget* chart, const char* label, float* values, int count, uint32_t color);
extern void hal_chart_set_title(HalWidget* chart, const char* title);

void halgui_create_chart(const char* name, const char* type) {
    HalWidget* parent = g_halgui.main_window ? (HalWidget*)g_halgui.main_window : NULL;
    int chartType = 0;  // LINE
    if (strcmp(type, "bar") == 0) chartType = 1;
    else if (strcmp(type, "pie") == 0) chartType = 2;
    else if (strcmp(type, "area") == 0) chartType = 3;
    
    HalWidget* chart = hal_chart_create(parent, chartType);
    halgui_add_widget(name, chart, HAL_WIDGET_CANVAS);
}

void halgui_chart_set_title(const char* name, const char* title) {
    HalWidget* chart = halgui_find_widget(name);
    if (chart) {
        hal_chart_set_title(chart, title);
    }
}

/* Calendar */
extern HalWidget* hal_calendar_create(HalWidget* parent);
extern void hal_calendar_set_date(HalWidget* cal, int year, int month, int day);
extern void hal_calendar_get_date(HalWidget* cal, int* year, int* month, int* day);

void halgui_create_calendar(const char* name) {
    HalWidget* parent = g_halgui.main_window ? (HalWidget*)g_halgui.main_window : NULL;
    HalWidget* cal = hal_calendar_create(parent);
    halgui_add_widget(name, cal, HAL_WIDGET_CUSTOM);
}

void halgui_calendar_set_date(const char* name, int year, int month, int day) {
    HalWidget* cal = halgui_find_widget(name);
    if (cal) {
        hal_calendar_set_date(cal, year, month, day);
    }
}

/* Notification */
extern HalWidget* hal_notification_create(HalWindow* window, const char* title, const char* message, int type, int durationMs);
extern void hal_notification_show(HalWidget* notif);

void halgui_show_notification(const char* title, const char* message, const char* type, int duration) {
    int notifType = 0;  // INFO
    if (strcmp(type, "success") == 0) notifType = 1;
    else if (strcmp(type, "warning") == 0) notifType = 2;
    else if (strcmp(type, "error") == 0) notifType = 3;
    
    HalWidget* notif = hal_notification_create(g_halgui.main_window, title, message, notifType, duration);
    hal_notification_show(notif);
}

/* Context Menu */
extern HalWidget* hal_contextmenu_create(void);
extern void hal_contextmenu_add_item(HalWidget* menu, const char* text, void* callback, void* userData);
extern void hal_contextmenu_add_separator(HalWidget* menu);
extern void hal_contextmenu_show(HalWidget* menu, int x, int y);

void halgui_create_contextmenu(const char* name) {
    HalWidget* menu = hal_contextmenu_create();
    halgui_add_widget(name, menu, HAL_WIDGET_MENU);
}

void halgui_contextmenu_add_item(const char* name, const char* text) {
    HalWidget* menu = halgui_find_widget(name);
    if (menu) {
        hal_contextmenu_add_item(menu, text, NULL, NULL);
    }
}

void halgui_contextmenu_show(const char* name, int x, int y) {
    HalWidget* menu = halgui_find_widget(name);
    if (menu) {
        hal_contextmenu_show(menu, x, y);
    }
}

/* Tooltip */
extern HalWidget* hal_tooltip_create(HalWidget* target, const char* text);
extern void hal_tooltip_set_text(HalWidget* tooltip, const char* text);

void halgui_set_tooltip(const char* widgetName, const char* text) {
    HalWidget* widget = halgui_find_widget(widgetName);
    if (widget) {
        (void)hal_tooltip_create(widget, text);
        // Tooltip is automatically managed by HalGUI
    }
}

/* Rich Text Editor */
extern HalWidget* hal_richtexteditor_create(HalWidget* parent);
extern void hal_richtexteditor_set_style(HalWidget* editor, uint32_t style);
extern void hal_richtexteditor_set_font_size(HalWidget* editor, int size);
extern void hal_richtexteditor_set_text_color(HalWidget* editor, uint32_t color);

void halgui_create_richtexteditor(const char* name) {
    HalWidget* parent = g_halgui.main_window ? (HalWidget*)g_halgui.main_window : NULL;
    HalWidget* editor = hal_richtexteditor_create(parent);
    halgui_add_widget(name, editor, HAL_WIDGET_TEXTAREA);
}

void halgui_richtexteditor_set_bold(const char* name, bool bold) {
    HalWidget* editor = halgui_find_widget(name);
    if (editor) {
        hal_richtexteditor_set_style(editor, bold ? 1 : 0);
    }
}

void halgui_richtexteditor_set_font_size(const char* name, int size) {
    HalWidget* editor = halgui_find_widget(name);
    if (editor) {
        hal_richtexteditor_set_font_size(editor, size);
    }
}


HcsValue* halgui_dialog_select_folder(HcsRuntime* rt, const char* title) {
    (void)rt;
    char* folder = hal_dialog_select_folder(NULL, title);
    if (folder) {
        HcsValue* result = value_string(folder);
        free(folder);
        return result;
    }
    return value_null();
}

/* ============================================
   Layout System Support
   ============================================ */

void halgui_set_layout(const char* panelName, const char* layoutType) {
    HalWidget* panel = halgui_find_widget(panelName);
    if (!panel) return;
    
    HalLayoutType layout = HAL_LAYOUT_NONE;
    
    if (strcmp(layoutType, "horizontal") == 0) {
        layout = HAL_LAYOUT_HORIZONTAL;
    } else if (strcmp(layoutType, "vertical") == 0) {
        layout = HAL_LAYOUT_VERTICAL;
    } else if (strcmp(layoutType, "grid") == 0) {
        layout = HAL_LAYOUT_GRID;
    } else if (strcmp(layoutType, "flex") == 0) {
        layout = HAL_LAYOUT_FLEX;
    }
    
    hal_widget_set_layout(panel, layout);
    
    // Force invalidation to trigger redraw
    hal_widget_invalidate(panel);
}

void halgui_set_gap(const char* panelName, int gap) {
    HalWidget* panel = halgui_find_widget(panelName);
    if (panel) {
        hal_widget_set_gap(panel, gap);
        // Force invalidation to trigger redraw
        hal_widget_invalidate(panel);
    }
}

void halgui_set_align(const char* widgetName, const char* horizontal, const char* vertical) {
    HalWidget* widget = halgui_find_widget(widgetName);
    if (widget) {
        HalAlignment h = HAL_ALIGN_LEFT;
        HalAlignment v = HAL_ALIGN_MIDDLE;
        
        // Parse horizontal alignment
        if (strcmp(horizontal, "center") == 0) {
            h = HAL_ALIGN_CENTER;
        } else if (strcmp(horizontal, "right") == 0) {
            h = HAL_ALIGN_RIGHT;
        } else {
            h = HAL_ALIGN_LEFT;
        }
        
        // Parse vertical alignment
        if (strcmp(vertical, "top") == 0) {
            v = HAL_ALIGN_TOP;
        } else if (strcmp(vertical, "bottom") == 0) {
            v = HAL_ALIGN_BOTTOM;
        } else {
            v = HAL_ALIGN_MIDDLE;
        }
        
        hal_widget_set_align(widget, h, v);
        
        // Re-apply layout if this widget has one
        if (widget->layout != HAL_LAYOUT_NONE) {
            hal_widget_apply_layout(widget);
        }
        
        // Force invalidation to trigger redraw
        hal_widget_invalidate(widget);
    }
}

void halgui_set_widget_flex(const char* widgetName, float flex) {
    HalWidget* widget = halgui_find_widget(widgetName);
    if (widget) {
        hal_widget_set_flex(widget, flex);
    }
}

void halgui_set_widget_margin(const char* widgetName, int top, int right, int bottom, int left) {
    HalWidget* widget = halgui_find_widget(widgetName);
    if (widget) {
        hal_widget_set_margin(widget, top, right, bottom, left);
        // Re-apply parent layout
        if (widget->parent && widget->parent->layout != HAL_LAYOUT_NONE) {
            hal_widget_apply_layout(widget->parent);
        }
    }
}

void halgui_apply_layout(const char* panelName) {
    HalWidget* panel = halgui_find_widget(panelName);
    if (panel) {
        hal_widget_apply_layout(panel);
        hal_widget_invalidate(panel);
    }
}


/* ============================================
   Clipboard Functions
   ============================================ */

/* Clipboard.setText(text) - Set clipboard text */
HcsValue* halgui_clipboard_set_text(HcsRuntime* rt, const char* text) {
    (void)rt;
    if (!text) return value_bool(false);
    
    hal_clipboard_set_text(text);
    return value_bool(true);
}

/* Clipboard.getText() - Get clipboard text */
HcsValue* halgui_clipboard_get_text(HcsRuntime* rt) {
    (void)rt;
    char* text = hal_clipboard_get_text();
    if (text) {
        HcsValue* result = value_string(text);
        free(text);
        return result;
    }
    return value_string("");
}

/* Clipboard.hasText() - Check if clipboard has text */
HcsValue* halgui_clipboard_has_text(HcsRuntime* rt) {
    (void)rt;
    return value_bool(hal_clipboard_has_text());
}

/* ============================================
   Drag & Drop Functions
   ============================================ */

/* Widget.enableDrag(name, enable) - Enable drag for widget */
HcsValue* halgui_widget_enable_drag(HcsRuntime* rt, const char* name, bool enable) {
    (void)rt;
    HalWidget* widget = halgui_find_widget(name);
    if (!widget) return value_bool(false);
    
    hal_widget_enable_drag(widget, enable);
    return value_bool(true);
}

/* Widget.enableDrop(name, enable) - Enable drop for widget */
HcsValue* halgui_widget_enable_drop(HcsRuntime* rt, const char* name, bool enable) {
    (void)rt;
    HalWidget* widget = halgui_find_widget(name);
    if (!widget) return value_bool(false);
    
    hal_widget_enable_drop(widget, enable);
    return value_bool(true);
}

/* Widget.setDragData(name, data) - Set drag data for widget */
HcsValue* halgui_widget_set_drag_data(HcsRuntime* rt, const char* name, const char* data) {
    (void)rt;
    HalWidget* widget = halgui_find_widget(name);
    if (!widget || !data) return value_bool(false);
    
    hal_widget_set_drag_data(widget, data);
    return value_bool(true);
}

/* Widget.getDragData(name) - Get drag data from widget */
HcsValue* halgui_widget_get_drag_data(HcsRuntime* rt, const char* name) {
    (void)rt;
    HalWidget* widget = halgui_find_widget(name);
    if (!widget) return value_string("");
    
    char* data = hal_widget_get_drag_data(widget);
    if (data) {
        HcsValue* result = value_string(data);
        free(data);
        return result;
    }
    return value_string("");
}
