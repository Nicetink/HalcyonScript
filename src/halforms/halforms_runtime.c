/*
 * HalForms - HalcyonScript Runtime Integration
 * Connects HalcyonScript syntax to HalForms C API
 * Similar to halgui_runtime.c but for Windows Forms-style UI
 */

#include "halforms.h"
#include "../runtime.h"
#include <stdio.h>
#include <string.h>

/* ============================================
   HalForms Runtime State
   ============================================ */

typedef struct {
    char* name;
    void* control;      /* Can be HalForm*, HalControl*, or specialized control */
    int type;           /* Control type identifier */
} HalFormsWidget;

typedef struct {
    HalForm* mainForm;
    HalFormsWidget* widgets;
    int widgetCount;
    int widgetCapacity;
    HcsRuntime* hcsRuntime;
    bool initialized;
} HalFormsRuntime;

static HalFormsRuntime g_halforms_rt = {0};

/* Control type identifiers */
#define HALFORMS_TYPE_FORM          1
#define HALFORMS_TYPE_LABEL         2
#define HALFORMS_TYPE_BUTTON        3
#define HALFORMS_TYPE_TEXTBOX       4
#define HALFORMS_TYPE_CHECKBOX      5
#define HALFORMS_TYPE_RADIOBUTTON   6
#define HALFORMS_TYPE_COMBOBOX      7
#define HALFORMS_TYPE_LISTBOX       8
#define HALFORMS_TYPE_GROUPBOX      9
#define HALFORMS_TYPE_PANEL         10
#define HALFORMS_TYPE_PROGRESSBAR   11
#define HALFORMS_TYPE_TRACKBAR      12
#define HALFORMS_TYPE_TREEVIEW      13
#define HALFORMS_TYPE_LISTVIEW      14
#define HALFORMS_TYPE_TABCONTROL    15
#define HALFORMS_TYPE_CODEEDITOR    16
#define HALFORMS_TYPE_PROPERTYGRID  17
#define HALFORMS_TYPE_TIMELINE      18
#define HALFORMS_TYPE_MENU          19
#define HALFORMS_TYPE_MENUITEM      20
#define HALFORMS_TYPE_TOOLBAR       21
#define HALFORMS_TYPE_STATUSBAR     22

/* ============================================
   Widget Management
   ============================================ */

static void halforms_rt_add_widget(const char* name, void* control, int type) {
    if (g_halforms_rt.widgetCount >= g_halforms_rt.widgetCapacity) {
        int nc = g_halforms_rt.widgetCapacity == 0 ? 32 : g_halforms_rt.widgetCapacity * 2;
        g_halforms_rt.widgets = realloc(g_halforms_rt.widgets, sizeof(HalFormsWidget) * nc);
        g_halforms_rt.widgetCapacity = nc;
    }
    g_halforms_rt.widgets[g_halforms_rt.widgetCount].name = strdup(name);
    g_halforms_rt.widgets[g_halforms_rt.widgetCount].control = control;
    g_halforms_rt.widgets[g_halforms_rt.widgetCount].type = type;
    g_halforms_rt.widgetCount++;
}

static HalFormsWidget* halforms_rt_find_widget(const char* name) {
    for (int i = 0; i < g_halforms_rt.widgetCount; i++) {
        if (strcmp(g_halforms_rt.widgets[i].name, name) == 0) {
            return &g_halforms_rt.widgets[i];
        }
    }
    return NULL;
}

/* Get main form for external use */
HalForm* halforms_rt_get_main_form(void) {
    return g_halforms_rt.mainForm;
}

/* ============================================
   Event Handlers Bridge
   ============================================ */

typedef struct {
    char* elementName;
    char* eventType;
    HcsAstNode* handler;
} HalFormsHandler;

static HalFormsHandler* g_handlers = NULL;
static int g_handlerCount = 0;
static int g_handlerCapacity = 0;

static void halforms_rt_add_handler(const char* element, const char* event, HcsAstNode* handler) {
    if (g_handlerCount >= g_handlerCapacity) {
        int nc = g_handlerCapacity == 0 ? 32 : g_handlerCapacity * 2;
        g_handlers = realloc(g_handlers, sizeof(HalFormsHandler) * nc);
        g_handlerCapacity = nc;
    }
    g_handlers[g_handlerCount].elementName = strdup(element);
    g_handlers[g_handlerCount].eventType = strdup(event);
    g_handlers[g_handlerCount].handler = handler;
    g_handlerCount++;
}

static void halforms_rt_fire_event(const char* element, const char* event) {
    if (!g_halforms_rt.hcsRuntime) return;
    
    extern void execute_statement(HcsRuntime* rt, HcsAstNode* node);
    
    for (int i = 0; i < g_handlerCount; i++) {
        if (strcmp(g_handlers[i].elementName, element) == 0 &&
            strcmp(g_handlers[i].eventType, event) == 0) {
            
            HcsAstNode* h = g_handlers[i].handler;
            if (h && h->type == HCS_AST_EVENT_HANDLER) {
                HcsScope* es = scope_create(g_halforms_rt.hcsRuntime->global_scope);
                HcsScope* old = g_halforms_rt.hcsRuntime->current_scope;
                g_halforms_rt.hcsRuntime->current_scope = es;
                
                for (int j = 0; j < h->data.event_handler.body.count; j++) {
                    HcsAstNode* stmt = h->data.event_handler.body.items[j];
                    execute_statement(g_halforms_rt.hcsRuntime, stmt);
                    if (g_halforms_rt.hcsRuntime->should_return || 
                        g_halforms_rt.hcsRuntime->should_break) break;
                }
                
                g_halforms_rt.hcsRuntime->current_scope = old;
                scope_free(es);
            }
        }
    }
}

/* Dispatch menu event by handler name - calls the function directly */
void halforms_rt_dispatch_menu_event(const char* handlerName) {
    if (!handlerName || !g_halforms_rt.hcsRuntime) return;
    
    /* First try to fire as event handler (on element clicked) */
    halforms_rt_fire_event(handlerName, "clicked");
    
    /* Also try to call as a function */
    extern HcsValue* runtime_eval(HcsRuntime* rt, HcsAstNode* node);
    extern void execute_statement(HcsRuntime* rt, HcsAstNode* node);
    
    /* Find and call the function by name */
    HcsRuntime* rt = g_halforms_rt.hcsRuntime;
    for (int i = 0; i < rt->func_count; i++) {
        if (strcmp(rt->functions[i].name, handlerName) == 0) {
            HcsAstNode* fn = rt->functions[i].node;
            if (fn && fn->type == HCS_AST_FUNC_DECL) {
                /* Create scope and execute function body */
                HcsScope* fs = scope_create(rt->global_scope);
                HcsScope* old = rt->current_scope;
                rt->current_scope = fs;
                rt->should_return = false;
                rt->return_value = NULL;
                
                for (int j = 0; j < fn->data.func_decl.body.count; j++) {
                    execute_statement(rt, fn->data.func_decl.body.items[j]);
                    if (rt->should_return) break;
                }
                
                rt->should_return = false;
                rt->return_value = NULL;
                rt->current_scope = old;
                scope_free(fs);
            }
            break;
        }
    }
}

/* Generic event handler callback */
static void on_control_click(HalControl* sender, HalFormEvent* event, void* userData) {
    const char* name = (const char*)userData;
    if (name) {
        halforms_rt_fire_event(name, "clicked");
    }
}

static void on_control_change(HalControl* sender, HalFormEvent* event, void* userData) {
    const char* name = (const char*)userData;
    if (name) {
        halforms_rt_fire_event(name, "changed");
    }
}

/* ============================================
   Initialization
   ============================================ */

void halforms_runtime_init(HcsRuntime* rt) {
    if (g_halforms_rt.initialized) return;
    
    if (!halforms_init()) {
        fprintf(stderr, "Failed to initialize HalForms\n");
        return;
    }
    
    g_halforms_rt.hcsRuntime = rt;
    g_halforms_rt.initialized = true;
}

void halforms_runtime_shutdown(void) {
    if (!g_halforms_rt.initialized) return;
    
    for (int i = 0; i < g_halforms_rt.widgetCount; i++) {
        free(g_halforms_rt.widgets[i].name);
    }
    free(g_halforms_rt.widgets);
    
    for (int i = 0; i < g_handlerCount; i++) {
        free(g_handlers[i].elementName);
        free(g_handlers[i].eventType);
    }
    free(g_handlers);
    
    halforms_shutdown();
    
    memset(&g_halforms_rt, 0, sizeof(g_halforms_rt));
    g_handlers = NULL;
    g_handlerCount = 0;
    g_handlerCapacity = 0;
}

/* ============================================
   Create Form
   ============================================ */

void halforms_rt_create_form(HcsRuntime* rt, const char* name, const char* title, 
                              int width, int height, int style) {
    if (!g_halforms_rt.initialized) {
        halforms_runtime_init(rt);
    }
    
    HalFormStyle formStyle = HALFORM_NORMAL;
    switch (style) {
        case 1: formStyle = HALFORM_DIALOG; break;
        case 2: formStyle = HALFORM_TOOL; break;
        case 3: formStyle = HALFORM_MDI_PARENT; break;
        case 4: formStyle = HALFORM_MDI_CHILD; break;
    }
    
    HalForm* form = halform_create_ex(title, CW_USEDEFAULT, CW_USEDEFAULT, 
                                       width, height, formStyle);
    if (!form) {
        fprintf(stderr, "Failed to create HalForms form\n");
        return;
    }
    
    g_halforms_rt.mainForm = form;
    halforms_rt_add_widget(name, form, HALFORMS_TYPE_FORM);
    
    halform_center(form);
}

/* ============================================
   Create Control
   ============================================ */

void halforms_rt_create_control(HcsRuntime* rt, const char* type, const char* name,
                                 const char* text, int x, int y, int w, int h) {
    if (!g_halforms_rt.mainForm) return;
    
    HalForm* parent = g_halforms_rt.mainForm;
    void* control = NULL;
    int ctrlType = 0;
    
    if (strcmp(type, "label") == 0) {
        control = halctrl_label(parent, text, x, y, w, h);
        ctrlType = HALFORMS_TYPE_LABEL;
    }
    else if (strcmp(type, "button") == 0) {
        control = halctrl_button(parent, text, x, y, w, h);
        ctrlType = HALFORMS_TYPE_BUTTON;
        halctrl_on_click((HalControl*)control, on_control_click, (void*)strdup(name));
    }
    else if (strcmp(type, "textbox") == 0) {
        control = halctrl_textbox(parent, text, x, y, w, h);
        ctrlType = HALFORMS_TYPE_TEXTBOX;
        halctrl_on_textchanged((HalControl*)control, on_control_change, (void*)strdup(name));
    }
    else if (strcmp(type, "textarea") == 0) {
        control = halctrl_textbox_multiline(parent, x, y, w, h);
        ctrlType = HALFORMS_TYPE_TEXTBOX;
    }
    else if (strcmp(type, "checkbox") == 0) {
        control = halctrl_checkbox(parent, text, x, y, w, h);
        ctrlType = HALFORMS_TYPE_CHECKBOX;
        halctrl_on_click((HalControl*)control, on_control_click, (void*)strdup(name));
    }
    else if (strcmp(type, "radiobutton") == 0) {
        control = halctrl_radiobutton(parent, text, x, y, w, h);
        ctrlType = HALFORMS_TYPE_RADIOBUTTON;
        halctrl_on_click((HalControl*)control, on_control_click, (void*)strdup(name));
    }
    else if (strcmp(type, "combobox") == 0) {
        control = halctrl_combobox(parent, x, y, w, h);
        ctrlType = HALFORMS_TYPE_COMBOBOX;
        halctrl_on_textchanged((HalControl*)control, on_control_change, (void*)strdup(name));
    }
    else if (strcmp(type, "listbox") == 0) {
        control = halctrl_listbox(parent, x, y, w, h);
        ctrlType = HALFORMS_TYPE_LISTBOX;
        halctrl_on_textchanged((HalControl*)control, on_control_change, (void*)strdup(name));
    }
    else if (strcmp(type, "groupbox") == 0) {
        control = halctrl_groupbox(parent, text, x, y, w, h);
        ctrlType = HALFORMS_TYPE_GROUPBOX;
    }
    else if (strcmp(type, "panel") == 0) {
        control = halctrl_panel(parent, x, y, w, h);
        ctrlType = HALFORMS_TYPE_PANEL;
    }
    else if (strcmp(type, "progressbar") == 0) {
        control = halctrl_progressbar(parent, x, y, w, h);
        ctrlType = HALFORMS_TYPE_PROGRESSBAR;
    }
    else if (strcmp(type, "trackbar") == 0 || strcmp(type, "slider") == 0) {
        control = halctrl_trackbar(parent, x, y, w, h, 0, 100);
        ctrlType = HALFORMS_TYPE_TRACKBAR;
    }
    else if (strcmp(type, "treeview") == 0) {
        control = haltreeview_create(parent, x, y, w, h);
        ctrlType = HALFORMS_TYPE_TREEVIEW;
    }
    else if (strcmp(type, "listview") == 0) {
        control = hallistview_create(parent, x, y, w, h);
        ctrlType = HALFORMS_TYPE_LISTVIEW;
    }
    else if (strcmp(type, "tabcontrol") == 0) {
        control = haltabcontrol_create(parent, x, y, w, h);
        ctrlType = HALFORMS_TYPE_TABCONTROL;
    }
    else if (strcmp(type, "codeeditor") == 0) {
        control = halcodeeditor_create(parent, x, y, w, h);
        ctrlType = HALFORMS_TYPE_CODEEDITOR;
    }
    else if (strcmp(type, "propertygrid") == 0) {
        control = halpropertygrid_create(parent, x, y, w, h);
        ctrlType = HALFORMS_TYPE_PROPERTYGRID;
    }
    else if (strcmp(type, "timeline") == 0) {
        control = haltimeline_create(parent, x, y, w, h);
        ctrlType = HALFORMS_TYPE_TIMELINE;
    }
    
    if (control) {
        halforms_rt_add_widget(name, control, ctrlType);
    }
}

/* ============================================
   Set Property
   ============================================ */

void halforms_rt_set_property(HcsRuntime* rt, const char* element, 
                               const char* prop, HcsValue* val) {
    HalFormsWidget* widget = halforms_rt_find_widget(element);
    if (!widget) {
        value_release(val);
        return;
    }
    
    /* Handle form properties */
    if (widget->type == HALFORMS_TYPE_FORM) {
        HalForm* form = (HalForm*)widget->control;
        
        if (strcmp(prop, "title") == 0) {
            char* text = value_to_string(val);
            halform_set_title(form, text);
            free(text);
        }
        else if (strcmp(prop, "icon") == 0) {
            char* path = value_to_string(val);
            halform_set_icon(form, path);
            free(path);
        }
    }
    /* Handle control properties */
    else {
        HalControl* ctrl = (HalControl*)widget->control;
        
        if (strcmp(prop, "text") == 0) {
            char* text = value_to_string(val);
            halctrl_set_text(ctrl, text);
            free(text);
        }
        else if (strcmp(prop, "visible") == 0) {
            halctrl_set_visible(ctrl, value_is_truthy(val));
        }
        else if (strcmp(prop, "enabled") == 0) {
            halctrl_set_enabled(ctrl, value_is_truthy(val));
        }
        else if (strcmp(prop, "checked") == 0) {
            halctrl_set_checked(ctrl, value_is_truthy(val));
        }
        else if (strcmp(prop, "value") == 0) {
            halctrl_set_value(ctrl, (int)value_to_number(val));
        }
        else if (strcmp(prop, "x") == 0) {
            ctrl->x = (int)value_to_number(val);
            halctrl_set_bounds(ctrl, ctrl->x, ctrl->y, ctrl->width, ctrl->height);
        }
        else if (strcmp(prop, "y") == 0) {
            ctrl->y = (int)value_to_number(val);
            halctrl_set_bounds(ctrl, ctrl->x, ctrl->y, ctrl->width, ctrl->height);
        }
        else if (strcmp(prop, "width") == 0) {
            ctrl->width = (int)value_to_number(val);
            halctrl_set_bounds(ctrl, ctrl->x, ctrl->y, ctrl->width, ctrl->height);
        }
        else if (strcmp(prop, "height") == 0) {
            ctrl->height = (int)value_to_number(val);
            halctrl_set_bounds(ctrl, ctrl->x, ctrl->y, ctrl->width, ctrl->height);
        }
    }
    
    value_release(val);
}

/* ============================================
   Get Property
   ============================================ */

HcsValue* halforms_rt_get_property(HcsRuntime* rt, const char* element, const char* prop) {
    HalFormsWidget* widget = halforms_rt_find_widget(element);
    if (!widget) return value_null();
    
    /* Handle form properties */
    if (widget->type == HALFORMS_TYPE_FORM) {
        HalForm* form = (HalForm*)widget->control;
        
        if (strcmp(prop, "title") == 0) {
            return value_string(form->base.text ? form->base.text : "");
        }
        else if (strcmp(prop, "width") == 0) {
            return value_number(form->base.width);
        }
        else if (strcmp(prop, "height") == 0) {
            return value_number(form->base.height);
        }
    }
    /* Handle control properties */
    else {
        HalControl* ctrl = (HalControl*)widget->control;
        
        if (strcmp(prop, "text") == 0) {
            char* text = halctrl_get_text(ctrl);
            HcsValue* result = value_string(text ? text : "");
            free(text);
            return result;
        }
        else if (strcmp(prop, "visible") == 0) {
            return value_bool(ctrl->visible);
        }
        else if (strcmp(prop, "enabled") == 0) {
            return value_bool(ctrl->enabled);
        }
        else if (strcmp(prop, "checked") == 0) {
            return value_bool(halctrl_get_checked(ctrl));
        }
        else if (strcmp(prop, "value") == 0) {
            return value_number(halctrl_get_value(ctrl));
        }
        else if (strcmp(prop, "x") == 0) {
            return value_number(ctrl->x);
        }
        else if (strcmp(prop, "y") == 0) {
            return value_number(ctrl->y);
        }
        else if (strcmp(prop, "width") == 0) {
            return value_number(ctrl->width);
        }
        else if (strcmp(prop, "height") == 0) {
            return value_number(ctrl->height);
        }
        else if (strcmp(prop, "selectedIndex") == 0) {
            return value_number(halctrl_get_selected_index(ctrl));
        }
        else if (strcmp(prop, "selectedItem") == 0) {
            char* item = halctrl_get_selected_item(ctrl);
            HcsValue* result = item ? value_string(item) : value_null();
            free(item);
            return result;
        }
    }
    
    return value_null();
}

/* ============================================
   Control Methods
   ============================================ */

void halforms_rt_add_item(const char* element, const char* item) {
    HalFormsWidget* widget = halforms_rt_find_widget(element);
    if (!widget) return;
    
    if (widget->type == HALFORMS_TYPE_COMBOBOX || widget->type == HALFORMS_TYPE_LISTBOX) {
        halctrl_add_item((HalControl*)widget->control, item);
    }
    else if (widget->type == HALFORMS_TYPE_LISTVIEW) {
        hallistview_add_item((HalListView*)widget->control, item, -1);
    }
    else if (widget->type == HALFORMS_TYPE_TABCONTROL) {
        haltabcontrol_add_tab((HalTabControl*)widget->control, item, -1);
    }
}

void halforms_rt_clear_items(const char* element) {
    HalFormsWidget* widget = halforms_rt_find_widget(element);
    if (!widget) return;
    
    if (widget->type == HALFORMS_TYPE_COMBOBOX || widget->type == HALFORMS_TYPE_LISTBOX) {
        halctrl_clear_items((HalControl*)widget->control);
    }
    else if (widget->type == HALFORMS_TYPE_LISTVIEW) {
        hallistview_clear((HalListView*)widget->control);
    }
    else if (widget->type == HALFORMS_TYPE_TREEVIEW) {
        haltreeview_clear((HalTreeView*)widget->control);
    }
}

/* ============================================
   Register Event Handler
   ============================================ */

void halforms_rt_register_handler(HcsAstNode* node) {
    halforms_rt_add_handler(
        node->data.event_handler.element_name,
        node->data.event_handler.event_type,
        node
    );
}

/* ============================================
   Run Event Loop
   ============================================ */

void halforms_rt_run(void) {
    if (!g_halforms_rt.mainForm) return;
    
    halform_show(g_halforms_rt.mainForm);
    halforms_run();
}

/* ============================================
   Dialog Functions for Runtime
   ============================================ */

HcsValue* halforms_rt_msgbox(const char* text, const char* title, int buttons, int icon) {
    int result = halforms_msgbox(text, title, buttons, icon);
    return value_number(result);
}

HcsValue* halforms_rt_open_file(const char* title, const char* filter) {
    char* file = halforms_open_file(title, filter);
    if (file) {
        HcsValue* result = value_string(file);
        free(file);
        return result;
    }
    return value_null();
}

HcsValue* halforms_rt_save_file(const char* title, const char* filter, const char* defaultName) {
    char* file = halforms_save_file(title, filter, defaultName);
    if (file) {
        HcsValue* result = value_string(file);
        free(file);
        return result;
    }
    return value_null();
}

HcsValue* halforms_rt_browse_folder(const char* title) {
    char* folder = halforms_browse_folder(title);
    if (folder) {
        HcsValue* result = value_string(folder);
        free(folder);
        return result;
    }
    return value_null();
}

HcsValue* halforms_rt_color_dialog(int initialColor) {
    COLORREF color = halforms_color_dialog((COLORREF)initialColor);
    return value_number((double)color);
}

HcsValue* halforms_rt_input_dialog(const char* title, const char* prompt, const char* defaultValue) {
    char* input = halforms_input_dialog(title, prompt, defaultValue);
    if (input) {
        HcsValue* result = value_string(input);
        free(input);
        return result;
    }
    return value_null();
}

/* ============================================
   Menu Functions for Runtime
   ============================================ */

/* External function from halforms_menu.c */
extern HalMenuItem* halmenuitem_add_item_ex(HalMenuItem* parent, const char* text, const char* handlerName);
extern HalMenuItem* halmenuitem_add_submenu(HalMenuItem* parent, const char* text);

void halforms_rt_create_menu(const char* name) {
    HalMenu* menu = halmenu_create();
    if (menu) {
        halforms_rt_add_widget(name, menu, HALFORMS_TYPE_MENU);
    }
}

/* External function from halforms_menu.c */
extern void register_menu_item_ext(int id, const char* handlerName);

void halforms_rt_add_menu_item(const char* menuName, const char* text, const char* handlerName) {
    /* First check if it's a submenu (registered as MENUITEM type) */
    HalFormsWidget* submenuWidget = halforms_rt_find_widget(menuName);
    if (submenuWidget && submenuWidget->type == HALFORMS_TYPE_MENUITEM) {
        /* Adding to a submenu */
        HalMenuItem* parentItem = (HalMenuItem*)submenuWidget->control;
        
        /* Check if separator */
        if (strcmp(text, "-") == 0) {
            halmenuitem_add_item_ex(parentItem, "-", NULL);
        } else if (handlerName && strlen(handlerName) > 0) {
            /* Regular menu item with handler */
            HalMenuItem* item = halmenuitem_add_item_ex(parentItem, text, handlerName);
            if (item) {
                item->userData = (void*)_strdup(handlerName);
            }
        } else {
            /* Item without handler */
            halmenuitem_add_item_ex(parentItem, text, NULL);
        }
        return;
    }
    
    /* Check if it's a main menu */
    HalFormsWidget* widget = halforms_rt_find_widget(menuName);
    if (!widget || widget->type != HALFORMS_TYPE_MENU) return;
    
    HalMenu* menu = (HalMenu*)widget->control;
    
    /* Check if we're adding a submenu (handlerName is another menu name) */
    HalFormsWidget* targetWidget = halforms_rt_find_widget(handlerName);
    if (targetWidget && targetWidget->type == HALFORMS_TYPE_MENU) {
        /* This is a submenu reference - need to copy items from the source menu */
        HalMenu* sourceMenu = (HalMenu*)targetWidget->control;
        
        /* Create a popup submenu */
        HalMenuItem* submenu = halmenu_add_submenu(menu, text);
        if (submenu && sourceMenu) {
            /* Copy all items from source menu to submenu */
            for (int i = 0; i < sourceMenu->itemCount; i++) {
                HalMenuItem* srcItem = sourceMenu->items[i];
                if (srcItem) {
                    if (srcItem->text && strcmp(srcItem->text, "-") == 0) {
                        /* Separator */
                        AppendMenuA(submenu->hmenu, MF_SEPARATOR, 0, NULL);
                    } else {
                        /* Regular item - copy with handler */
                        HalMenuItem* newItem = halmenuitem_add_item_ex(submenu, srcItem->text, 
                            srcItem->userData ? (const char*)srcItem->userData : NULL);
                        if (newItem && srcItem->userData) {
                            newItem->userData = (void*)_strdup((const char*)srcItem->userData);
                        }
                    }
                }
            }
        }
    } else if (strcmp(text, "-") == 0) {
        /* Separator */
        halmenu_add_separator(menu);
    } else {
        /* Regular menu item - store handler name */
        HalMenuItem* item = halmenu_add_item(menu, text, NULL);
        if (item && handlerName && strlen(handlerName) > 0) {
            item->userData = (void*)_strdup(handlerName);
            /* Also register in menu registry for WM_COMMAND lookup */
            register_menu_item_ext(item->id, handlerName);
        }
    }
}

void halforms_rt_set_form_menu(const char* formName, const char* menuName) {
    HalFormsWidget* formWidget = halforms_rt_find_widget(formName);
    HalFormsWidget* menuWidget = halforms_rt_find_widget(menuName);
    
    if (!formWidget || formWidget->type != HALFORMS_TYPE_FORM) return;
    if (!menuWidget || menuWidget->type != HALFORMS_TYPE_MENU) return;
    
    halform_set_menu((HalForm*)formWidget->control, (HalMenu*)menuWidget->control);
}

/* ============================================
   StatusBar Functions for Runtime
   ============================================ */

void halforms_rt_create_statusbar(const char* name, int partCount) {
    if (!g_halforms_rt.mainForm) return;
    
    HalStatusBar* statusBar = halstatusbar_create(g_halforms_rt.mainForm, partCount);
    if (statusBar) {
        halforms_rt_add_widget(name, statusBar, HALFORMS_TYPE_STATUSBAR);
        halform_set_statusbar(g_halforms_rt.mainForm, statusBar);
    }
}

void halforms_rt_set_statusbar_text(const char* name, int part, const char* text) {
    HalFormsWidget* widget = halforms_rt_find_widget(name);
    if (!widget || widget->type != HALFORMS_TYPE_STATUSBAR) return;
    
    halstatusbar_set_text((HalStatusBar*)widget->control, part, text);
}

/* ============================================
   TreeView Functions for Runtime
   ============================================ */

void halforms_rt_tree_add_item(const char* treeName, const char* parentPath, const char* text) {
    HalFormsWidget* widget = halforms_rt_find_widget(treeName);
    if (!widget || widget->type != HALFORMS_TYPE_TREEVIEW) return;
    
    HalTreeView* tree = (HalTreeView*)widget->control;
    
    /* For now, add to root - parentPath parsing can be added later */
    haltreeview_add_node(tree, NULL, text, -1);
}

/* ============================================
   TabControl Functions for Runtime
   ============================================ */

void halforms_rt_tab_add_tab(const char* tabName, const char* title) {
    HalFormsWidget* widget = halforms_rt_find_widget(tabName);
    if (!widget || widget->type != HALFORMS_TYPE_TABCONTROL) return;
    
    HalTabControl* tab = (HalTabControl*)widget->control;
    haltabcontrol_add_tab(tab, title, -1);
}
