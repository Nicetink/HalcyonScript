/*
 * HalGUI - Widget Implementation
 * 
 * All standard widgets: Button, Label, Input, etc.
 */

#include "halgui.h"
#include <stdlib.h>
#include <string.h>

/* ============================================
   Widget Base Functions
   ============================================ */

static HalWidget* hal_widget_create(HalWidgetType type, HalWidget* parent) {
    HalWidget* widget = (HalWidget*)calloc(1, sizeof(HalWidget));
    if (!widget) return NULL;
    
    widget->type = type;
    widget->visible = true;
    widget->enabled = true;
    widget->opacity = 1.0f;
    widget->layout = HAL_LAYOUT_NONE;
    widget->alignH = HAL_ALIGN_LEFT;
    widget->alignV = HAL_ALIGN_MIDDLE;
    
    if (parent) {
        hal_widget_add_child(parent, widget);
    }
    
    return widget;
}

void hal_widget_destroy(HalWidget* widget) {
    if (!widget) return;
    
    // Destroy children first
    for (int i = 0; i < widget->childCount; i++) {
        hal_widget_destroy(widget->children[i]);
    }
    
    // Remove from parent
    if (widget->parent) {
        hal_widget_remove_child(widget->parent, widget);
    }
    
    // Free allocated data
    free(widget->id);
    free(widget->className);
    free(widget->data);
    free(widget->bgColor);
    free(widget->fgColor);
    free(widget->borderColor);
    free(widget->borderWidth);
    free(widget->borderRadius);
    
    free(widget);
}

void hal_widget_add_child(HalWidget* parent, HalWidget* child) {
    if (!parent || !child) return;
    if (parent->childCount >= HAL_MAX_CHILDREN) return;
    
    child->parent = parent;
    parent->children[parent->childCount++] = child;
}

void hal_widget_remove_child(HalWidget* parent, HalWidget* child) {
    if (!parent || !child) return;
    
    for (int i = 0; i < parent->childCount; i++) {
        if (parent->children[i] == child) {
            // Shift remaining children
            for (int j = i; j < parent->childCount - 1; j++) {
                parent->children[j] = parent->children[j + 1];
            }
            parent->childCount--;
            child->parent = NULL;
            return;
        }
    }
}

HalWidget* hal_widget_find_by_id(HalWidget* root, const char* id) {
    if (!root || !id) return NULL;
    
    if (root->id && strcmp(root->id, id) == 0) {
        return root;
    }
    
    for (int i = 0; i < root->childCount; i++) {
        HalWidget* found = hal_widget_find_by_id(root->children[i], id);
        if (found) return found;
    }
    
    return NULL;
}

/* ============================================
   Geometry Functions
   ============================================ */

void hal_widget_set_bounds(HalWidget* widget, int x, int y, int width, int height) {
    if (!widget) return;
    widget->bounds.x = x;
    widget->bounds.y = y;
    widget->bounds.width = width;
    widget->bounds.height = height;
}

void hal_widget_set_position(HalWidget* widget, int x, int y) {
    if (!widget) return;
    widget->bounds.x = x;
    widget->bounds.y = y;
}

void hal_widget_set_size(HalWidget* widget, int width, int height) {
    if (!widget) return;
    widget->bounds.width = width;
    widget->bounds.height = height;
}

HalRect hal_widget_get_bounds(HalWidget* widget) {
    if (!widget) {
        HalRect empty = {0};
        return empty;
    }
    return widget->bounds;
}

void hal_widget_set_padding(HalWidget* widget, int top, int right, int bottom, int left) {
    if (!widget) return;
    widget->padding.top = top;
    widget->padding.right = right;
    widget->padding.bottom = bottom;
    widget->padding.left = left;
}

void hal_widget_set_margin(HalWidget* widget, int top, int right, int bottom, int left) {
    if (!widget) return;
    widget->margin.top = top;
    widget->margin.right = right;
    widget->margin.bottom = bottom;
    widget->margin.left = left;
}

/* ============================================
   State Functions
   ============================================ */

void hal_widget_show(HalWidget* widget) {
    if (widget) widget->visible = true;
}

void hal_widget_hide(HalWidget* widget) {
    if (widget) widget->visible = false;
}

void hal_widget_enable(HalWidget* widget) {
    if (!widget) return;
    widget->enabled = true;
    widget->state &= ~HAL_STATE_DISABLED;
}

void hal_widget_disable(HalWidget* widget) {
    if (!widget) return;
    widget->enabled = false;
    widget->state |= HAL_STATE_DISABLED;
}

void hal_widget_focus(HalWidget* widget) {
    if (widget) widget->state |= HAL_STATE_FOCUSED;
}

bool hal_widget_is_visible(HalWidget* widget) {
    return widget ? widget->visible : false;
}

bool hal_widget_is_enabled(HalWidget* widget) {
    return widget ? widget->enabled : false;
}

bool hal_widget_has_focus(HalWidget* widget) {
    return widget ? (widget->state & HAL_STATE_FOCUSED) != 0 : false;
}

/* ============================================
   Style Functions
   ============================================ */

void hal_widget_set_background(HalWidget* widget, HalColor color) {
    if (!widget) return;
    if (!widget->bgColor) widget->bgColor = (HalColor*)malloc(sizeof(HalColor));
    *widget->bgColor = color;
}

void hal_widget_set_foreground(HalWidget* widget, HalColor color) {
    if (!widget) return;
    if (!widget->fgColor) widget->fgColor = (HalColor*)malloc(sizeof(HalColor));
    *widget->fgColor = color;
}

void hal_widget_set_border(HalWidget* widget, int width, HalColor color) {
    if (!widget) return;
    if (!widget->borderWidth) widget->borderWidth = (int*)malloc(sizeof(int));
    if (!widget->borderColor) widget->borderColor = (HalColor*)malloc(sizeof(HalColor));
    *widget->borderWidth = width;
    *widget->borderColor = color;
}

void hal_widget_set_border_radius(HalWidget* widget, int radius) {
    if (!widget) return;
    if (!widget->borderRadius) widget->borderRadius = (int*)malloc(sizeof(int));
    *widget->borderRadius = radius;
}

void hal_widget_set_opacity(HalWidget* widget, float opacity) {
    if (widget) widget->opacity = opacity;
}

void hal_widget_set_flex(HalWidget* widget, float flex) {
    if (widget) widget->flex = flex;
}

void hal_widget_set_align(HalWidget* widget, HalAlignment h, HalAlignment v) {
    if (!widget) return;
    widget->alignH = h;
    widget->alignV = v;
}

void hal_widget_invalidate(HalWidget* widget) {
    if (!widget) return;
    
    // Find parent window and invalidate
    HalWidget* current = widget;
    while (current->parent) {
        current = current->parent;
    }
    
    if (current->type == HAL_WIDGET_WINDOW) {
        HalWindow* window = (HalWindow*)current;
        InvalidateRect(window->hwnd, NULL, FALSE);
    }
}

void hal_widget_update(HalWidget* widget) {
    if (!widget) return;
    
    HalWidget* current = widget;
    while (current->parent) {
        current = current->parent;
    }
    
    if (current->type == HAL_WIDGET_WINDOW) {
        HalWindow* window = (HalWindow*)current;
        UpdateWindow(window->hwnd);
    }
}

/* ============================================
   Panel Widget
   ============================================ */

HalWidget* hal_panel_create(HalWidget* parent) {
    HalWidget* panel = hal_widget_create(HAL_WIDGET_PANEL, parent);
    if (panel) {
        panel->layout = HAL_LAYOUT_NONE;
    }
    return panel;
}

void hal_panel_set_layout(HalWidget* panel, HalLayoutType layout) {
    if (panel && panel->type == HAL_WIDGET_PANEL) {
        panel->layout = layout;
    }
}

/* ============================================
   Button Widget
   ============================================ */

HalWidget* hal_button_create(HalWidget* parent, const char* text) {
    HalWidget* button = hal_widget_create(HAL_WIDGET_BUTTON, parent);
    if (button) {
        button->data = text ? _strdup(text) : NULL;
        button->bounds.width = 120;
        button->bounds.height = 36;
    }
    return button;
}

void hal_button_set_text(HalWidget* button, const char* text) {
    if (!button || button->type != HAL_WIDGET_BUTTON) return;
    free(button->data);
    button->data = text ? _strdup(text) : NULL;
}

const char* hal_button_get_text(HalWidget* button) {
    if (!button || button->type != HAL_WIDGET_BUTTON) return NULL;
    return (const char*)button->data;
}

/* ============================================
   Label Widget
   ============================================ */

HalWidget* hal_label_create(HalWidget* parent, const char* text) {
    HalWidget* label = hal_widget_create(HAL_WIDGET_LABEL, parent);
    if (label) {
        label->data = text ? _strdup(text) : NULL;
        label->bounds.width = 200;
        label->bounds.height = 24;
    }
    return label;
}

void hal_label_set_text(HalWidget* label, const char* text) {
    if (!label || label->type != HAL_WIDGET_LABEL) return;
    free(label->data);
    label->data = text ? _strdup(text) : NULL;
}

const char* hal_label_get_text(HalWidget* label) {
    if (!label || label->type != HAL_WIDGET_LABEL) return NULL;
    return (const char*)label->data;
}

void hal_label_set_align(HalWidget* label, HalAlignment align) {
    if (label) label->alignH = align;
}

/* ============================================
   Input Widget
   Now uses native Win32 EDIT control for proper text input
   ============================================ */

typedef struct {
    char* text;
    char* placeholder;
    bool isPassword;
    bool readonly;
    int cursorPos;
    int selectionStart;
    int selectionEnd;
    HWND hwnd;           // Native HWND (set by halgui_native.c)
    HBRUSH bgBrush;
    COLORREF bgColor;
    COLORREF textColor;
} HalInputData;

HalWidget* hal_input_create(HalWidget* parent, const char* placeholder) {
    HalWidget* input = hal_widget_create(HAL_WIDGET_INPUT, parent);
    if (input) {
        HalInputData* data = (HalInputData*)calloc(1, sizeof(HalInputData));
        data->text = _strdup("");
        data->placeholder = placeholder ? _strdup(placeholder) : NULL;
        input->data = data;
        input->bounds.width = 200;
        input->bounds.height = 36;
    }
    return input;
}

void hal_input_set_text(HalWidget* input, const char* text) {
    if (!input || input->type != HAL_WIDGET_INPUT) return;
    HalInputData* data = (HalInputData*)input->data;
    if (data) {
        free(data->text);
        data->text = text ? _strdup(text) : _strdup("");
        
        // Update native control if exists
        if (data->hwnd) {
            int len = MultiByteToWideChar(CP_UTF8, 0, data->text, -1, NULL, 0);
            wchar_t* wText = (wchar_t*)malloc(len * sizeof(wchar_t));
            MultiByteToWideChar(CP_UTF8, 0, data->text, -1, wText, len);
            SetWindowTextW(data->hwnd, wText);
            free(wText);
        }
    }
}

const char* hal_input_get_text(HalWidget* input) {
    if (!input || input->type != HAL_WIDGET_INPUT) return NULL;
    HalInputData* data = (HalInputData*)input->data;
    if (!data) return NULL;
    
    // Sync from native control if exists
    if (data->hwnd) {
        int len = GetWindowTextLengthW(data->hwnd) + 1;
        wchar_t* wText = (wchar_t*)malloc(len * sizeof(wchar_t));
        GetWindowTextW(data->hwnd, wText, len);
        
        // Convert to UTF-8
        int utf8Len = WideCharToMultiByte(CP_UTF8, 0, wText, -1, NULL, 0, NULL, NULL);
        free(data->text);
        data->text = (char*)malloc(utf8Len);
        WideCharToMultiByte(CP_UTF8, 0, wText, -1, data->text, utf8Len, NULL, NULL);
        free(wText);
    }
    
    return data->text;
}

void hal_input_set_placeholder(HalWidget* input, const char* placeholder) {
    if (!input || input->type != HAL_WIDGET_INPUT) return;
    HalInputData* data = (HalInputData*)input->data;
    if (data) {
        free(data->placeholder);
        data->placeholder = placeholder ? _strdup(placeholder) : NULL;
    }
}

void hal_input_set_password(HalWidget* input, bool isPassword) {
    if (!input || input->type != HAL_WIDGET_INPUT) return;
    HalInputData* data = (HalInputData*)input->data;
    if (data) data->isPassword = isPassword;
}

void hal_input_set_readonly(HalWidget* input, bool readonly) {
    if (!input || input->type != HAL_WIDGET_INPUT) return;
    HalInputData* data = (HalInputData*)input->data;
    if (data) data->readonly = readonly;
}

/* ============================================
   Checkbox Widget
   ============================================ */

HalWidget* hal_checkbox_create(HalWidget* parent, const char* label) {
    HalWidget* checkbox = hal_widget_create(HAL_WIDGET_CHECKBOX, parent);
    if (checkbox) {
        checkbox->data = label ? _strdup(label) : NULL;
        checkbox->bounds.width = 200;
        checkbox->bounds.height = 24;
    }
    return checkbox;
}

void hal_checkbox_set_checked(HalWidget* checkbox, bool checked) {
    if (!checkbox || checkbox->type != HAL_WIDGET_CHECKBOX) return;
    if (checked) {
        checkbox->state |= HAL_STATE_CHECKED;
    } else {
        checkbox->state &= ~HAL_STATE_CHECKED;
    }
}

bool hal_checkbox_is_checked(HalWidget* checkbox) {
    if (!checkbox || checkbox->type != HAL_WIDGET_CHECKBOX) return false;
    return (checkbox->state & HAL_STATE_CHECKED) != 0;
}

/* ============================================
   Toggle Switch Widget (Modern)
   ============================================ */

HalWidget* hal_toggle_create(HalWidget* parent, const char* label) {
    HalWidget* toggle = hal_widget_create(HAL_WIDGET_TOGGLE, parent);
    if (toggle) {
        toggle->data = label ? _strdup(label) : NULL;
        toggle->bounds.width = 200;
        toggle->bounds.height = 28;
    }
    return toggle;
}

void hal_toggle_set_checked(HalWidget* toggle, bool checked) {
    if (!toggle || toggle->type != HAL_WIDGET_TOGGLE) return;
    if (checked) {
        toggle->state |= HAL_STATE_CHECKED;
    } else {
        toggle->state &= ~HAL_STATE_CHECKED;
    }
}

bool hal_toggle_is_checked(HalWidget* toggle) {
    if (!toggle || toggle->type != HAL_WIDGET_TOGGLE) return false;
    return (toggle->state & HAL_STATE_CHECKED) != 0;
}

/* ============================================
   Slider Widget
   ============================================ */

typedef struct {
    int min;
    int max;
    int value;
} HalSliderData;

HalWidget* hal_slider_create(HalWidget* parent, int min, int max, int value) {
    HalWidget* slider = hal_widget_create(HAL_WIDGET_SLIDER, parent);
    if (slider) {
        HalSliderData* data = (HalSliderData*)calloc(1, sizeof(HalSliderData));
        data->min = min;
        data->max = max;
        data->value = value;
        slider->data = data;
        slider->bounds.width = 200;
        slider->bounds.height = 24;
        
        // Store normalized value in animProgress
        if (max > min) {
            slider->animProgress = (float)(value - min) / (max - min);
        }
    }
    return slider;
}

void hal_slider_set_value(HalWidget* slider, int value) {
    if (!slider || slider->type != HAL_WIDGET_SLIDER) return;
    HalSliderData* data = (HalSliderData*)slider->data;
    if (data) {
        data->value = value;
        if (data->max > data->min) {
            slider->animProgress = (float)(value - data->min) / (data->max - data->min);
        }
    }
}

int hal_slider_get_value(HalWidget* slider) {
    if (!slider || slider->type != HAL_WIDGET_SLIDER) return 0;
    HalSliderData* data = (HalSliderData*)slider->data;
    return data ? data->value : 0;
}

void hal_slider_set_range(HalWidget* slider, int min, int max) {
    if (!slider || slider->type != HAL_WIDGET_SLIDER) return;
    HalSliderData* data = (HalSliderData*)slider->data;
    if (data) {
        data->min = min;
        data->max = max;
    }
}

/* ============================================
   Progress Widget
   ============================================ */

HalWidget* hal_progress_create(HalWidget* parent) {
    HalWidget* progress = hal_widget_create(HAL_WIDGET_PROGRESS, parent);
    if (progress) {
        progress->bounds.width = 200;
        progress->bounds.height = 8;
        progress->animProgress = 0.0f;
    }
    return progress;
}

void hal_progress_set_value(HalWidget* progress, float value) {
    if (!progress || progress->type != HAL_WIDGET_PROGRESS) return;
    progress->animProgress = value < 0.0f ? 0.0f : (value > 1.0f ? 1.0f : value);
}

float hal_progress_get_value(HalWidget* progress) {
    if (!progress || progress->type != HAL_WIDGET_PROGRESS) return 0.0f;
    return progress->animProgress;
}

void hal_progress_set_indeterminate(HalWidget* progress, bool indeterminate) {
    if (!progress || progress->type != HAL_WIDGET_PROGRESS) return;
    progress->animating = indeterminate;
}
