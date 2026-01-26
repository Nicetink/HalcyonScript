/*
 * HalGUI - Widget Implementation
 * 
 * All standard widgets: Button, Label, Input, etc.
 */

#include "halgui.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

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
    
    // Initialize original sizes to 0 (will be set when bounds are first set)
    widget->originalWidth = 0;
    widget->originalHeight = 0;
    
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
    
    // Save original sizes if not set yet
    if (widget->originalWidth == 0 && width > 0) {
        widget->originalWidth = width;
    }
    if (widget->originalHeight == 0 && height > 0) {
        widget->originalHeight = height;
    }
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
        // Apply layout immediately
        hal_widget_apply_layout(panel);
    }
}

void hal_panel_set_gap(HalWidget* panel, int gap) {
    if (panel && panel->type == HAL_WIDGET_PANEL) {
        panel->gap = gap;
        // Re-apply layout with new gap
        hal_widget_apply_layout(panel);
    }
}

/* General layout functions for any widget */
void hal_widget_set_layout(HalWidget* widget, HalLayoutType layout) {
    if (!widget) return;
    widget->layout = layout;
    // Apply layout immediately
    hal_widget_apply_layout(widget);
}

void hal_widget_set_gap(HalWidget* widget, int gap) {
    if (!widget) return;
    widget->gap = gap;
    // Re-apply layout with new gap
    hal_widget_apply_layout(widget);
}

void hal_widget_set_flex(HalWidget* widget, float flex) {
    if (!widget) return;
    widget->flex = flex;
    // Re-apply parent layout if it's flex layout
    if (widget->parent && widget->parent->layout == HAL_LAYOUT_FLEX) {
        hal_widget_apply_layout(widget->parent);
    }
}

/* ============================================
   Layout System - Automatic positioning
   ============================================ */

void hal_widget_apply_layout(HalWidget* widget) {
    if (!widget || widget->layout == HAL_LAYOUT_NONE) return;
    
    // Calculate content area (bounds minus padding)
    int contentX = widget->bounds.x + widget->padding.left;
    int contentY = widget->bounds.y + widget->padding.top;
    int contentW = widget->bounds.width - widget->padding.left - widget->padding.right;
    int contentH = widget->bounds.height - widget->padding.top - widget->padding.bottom;
    
    int currentX = contentX;
    int currentY = contentY;
    int gap = widget->gap > 0 ? widget->gap : 8; // Default gap 8px
    
    switch (widget->layout) {
        case HAL_LAYOUT_HORIZONTAL: {
            // Arrange children horizontally
            currentX = contentX;
            
            // First pass: restore original sizes and position elements
            for (int i = 0; i < widget->childCount; i++) {
                HalWidget* child = widget->children[i];
                if (!child || !child->visible) continue;
                
                // Restore original size if available
                if (child->originalWidth > 0) {
                    child->bounds.width = child->originalWidth;
                }
                if (child->originalHeight > 0) {
                    child->bounds.height = child->originalHeight;
                }
                
                // Apply margins
                currentX += child->margin.left;
                
                // Position child
                child->bounds.x = currentX;
                child->bounds.y = contentY + child->margin.top;
                
                // Move to next position
                currentX += child->bounds.width + child->margin.right + gap;
            }
            
            // Second pass: check bounds and resize if necessary
            for (int i = 0; i < widget->childCount; i++) {
                HalWidget* child = widget->children[i];
                if (!child || !child->visible) continue;
                
                // Check if child would exceed container bounds
                int childEndX = child->bounds.x + child->bounds.width + child->margin.right;
                int containerEndX = contentX + contentW;
                
                if (childEndX > containerEndX && contentW > 0) {
                    // Resize child to fit within container
                    int availableWidth = containerEndX - child->bounds.x - child->margin.right;
                    if (availableWidth > 10) { // Minimum width
                        child->bounds.width = availableWidth;
                    }
                }
            }
            
            // Third pass: apply alignment after positioning
            if (widget->alignH == HAL_ALIGN_CENTER || widget->alignH == HAL_ALIGN_RIGHT) {
                // Calculate total width used
                int totalUsedWidth = 0;
                for (int i = 0; i < widget->childCount; i++) {
                    HalWidget* child = widget->children[i];
                    if (!child || !child->visible) continue;
                    totalUsedWidth += child->bounds.width + child->margin.left + child->margin.right;
                    if (i < widget->childCount - 1) totalUsedWidth += gap;
                }
                
                if (totalUsedWidth < contentW) {
                    int offset = 0;
                    if (widget->alignH == HAL_ALIGN_CENTER) {
                        offset = (contentW - totalUsedWidth) / 2;
                    } else if (widget->alignH == HAL_ALIGN_RIGHT) {
                        offset = contentW - totalUsedWidth;
                    }
                    
                    // Shift all children by offset
                    for (int i = 0; i < widget->childCount; i++) {
                        HalWidget* child = widget->children[i];
                        if (!child || !child->visible) continue;
                        child->bounds.x += offset;
                    }
                }
            }
            break;
        }
        
        case HAL_LAYOUT_VERTICAL: {
            // Arrange children vertically
            currentY = contentY;
            
            // First pass: restore original sizes and position elements
            for (int i = 0; i < widget->childCount; i++) {
                HalWidget* child = widget->children[i];
                if (!child || !child->visible) continue;
                
                // Restore original size if available
                if (child->originalWidth > 0) {
                    child->bounds.width = child->originalWidth;
                }
                if (child->originalHeight > 0) {
                    child->bounds.height = child->originalHeight;
                }
                
                // Apply margins
                currentY += child->margin.top;
                
                // Position child
                child->bounds.x = contentX + child->margin.left;
                child->bounds.y = currentY;
                
                // Move to next position
                currentY += child->bounds.height + child->margin.bottom + gap;
            }
            
            // Second pass: check bounds and resize if necessary
            for (int i = 0; i < widget->childCount; i++) {
                HalWidget* child = widget->children[i];
                if (!child || !child->visible) continue;
                
                // Check if child would exceed container height bounds
                int childEndY = child->bounds.y + child->bounds.height + child->margin.bottom;
                int containerEndY = contentY + contentH;
                
                if (childEndY > containerEndY && contentH > 0) {
                    // Resize child to fit within container
                    int availableHeight = containerEndY - child->bounds.y - child->margin.bottom;
                    if (availableHeight > 10) { // Minimum height
                        child->bounds.height = availableHeight;
                    }
                }
                
                // Check if child would exceed container width bounds
                int childEndX = child->bounds.x + child->bounds.width;
                int containerEndX = contentX + contentW;
                
                if (childEndX > containerEndX && contentW > 0) {
                    // Resize child width to fit within container
                    int availableWidth = containerEndX - child->bounds.x;
                    if (availableWidth > 10) { // Minimum width
                        child->bounds.width = availableWidth;
                    }
                }
            }
            
            // Third pass: apply horizontal alignment
            if (widget->alignH == HAL_ALIGN_CENTER || widget->alignH == HAL_ALIGN_RIGHT) {
                for (int i = 0; i < widget->childCount; i++) {
                    HalWidget* child = widget->children[i];
                    if (!child || !child->visible) continue;
                    
                    int availableWidth = contentW - child->margin.left - child->margin.right;
                    if (child->bounds.width < availableWidth) {
                        int offset = 0;
                        if (widget->alignH == HAL_ALIGN_CENTER) {
                            offset = (availableWidth - child->bounds.width) / 2;
                        } else if (widget->alignH == HAL_ALIGN_RIGHT) {
                            offset = availableWidth - child->bounds.width;
                        }
                        child->bounds.x = contentX + child->margin.left + offset;
                    }
                }
            }
            break;
        }
        
        case HAL_LAYOUT_GRID: {
            // Improved grid layout with bounds checking
            int itemWidth = 120;  // Default item width
            int itemHeight = 40;  // Default item height
            
            // Calculate columns based on available width
            int columns = contentW > 0 ? (contentW + gap) / (itemWidth + gap) : 1;
            if (columns < 1) columns = 1;
            
            int col = 0;
            int row = 0;
            
            for (int i = 0; i < widget->childCount; i++) {
                HalWidget* child = widget->children[i];
                if (!child || !child->visible) continue;
                
                // Use child's actual size if specified, otherwise use defaults
                int childW = child->bounds.width > 0 ? child->bounds.width : itemWidth;
                int childH = child->bounds.height > 0 ? child->bounds.height : itemHeight;
                
                // Calculate position
                int childX = contentX + col * (childW + gap) + child->margin.left;
                int childY = contentY + row * (childH + gap) + child->margin.top;
                
                // Check bounds and adjust size if necessary
                int childEndX = childX + childW;
                int childEndY = childY + childH;
                int containerEndX = contentX + contentW;
                int containerEndY = contentY + contentH;
                
                if (childEndX > containerEndX && contentW > 0) {
                    childW = containerEndX - childX;
                    if (childW < 10) childW = 10; // Minimum width
                }
                
                if (childEndY > containerEndY && contentH > 0) {
                    childH = containerEndY - childY;
                    if (childH < 10) childH = 10; // Minimum height
                }
                
                // Position child
                child->bounds.x = childX;
                child->bounds.y = childY;
                child->bounds.width = childW;
                child->bounds.height = childH;
                
                // Move to next column
                col++;
                if (col >= columns) {
                    col = 0;
                    row++;
                }
            }
            break;
        }
        
        case HAL_LAYOUT_FLEX: {
            // Flexible layout with flex weights and bounds checking
            float totalFlex = 0.0f;
            int fixedSpace = 0;
            int visibleCount = 0;
            
            // Calculate total flex and fixed space
            for (int i = 0; i < widget->childCount; i++) {
                HalWidget* child = widget->children[i];
                if (!child || !child->visible) continue;
                
                visibleCount++;
                if (child->flex > 0.0f) {
                    totalFlex += child->flex;
                } else {
                    fixedSpace += child->bounds.width + child->margin.left + child->margin.right;
                }
            }
            
            // Calculate available space for flex items
            int gapSpace = visibleCount > 1 ? (visibleCount - 1) * gap : 0;
            int flexSpace = contentW - fixedSpace - gapSpace;
            if (flexSpace < 0) flexSpace = 0;
            
            // Arrange children
            currentX = contentX;
            for (int i = 0; i < widget->childCount; i++) {
                HalWidget* child = widget->children[i];
                if (!child || !child->visible) continue;
                
                currentX += child->margin.left;
                
                // Calculate width based on flex
                if (child->flex > 0.0f && totalFlex > 0.0f) {
                    int flexWidth = (int)(flexSpace * (child->flex / totalFlex));
                    child->bounds.width = flexWidth > 10 ? flexWidth : 10; // Minimum width
                }
                
                // Check if child would exceed container bounds
                int childEndX = currentX + child->bounds.width + child->margin.right;
                int containerEndX = contentX + contentW;
                
                if (childEndX > containerEndX && contentW > 0) {
                    // Resize child to fit within container
                    int availableWidth = containerEndX - currentX - child->margin.right;
                    if (availableWidth > 10) { // Minimum width
                        child->bounds.width = availableWidth;
                    }
                }
                
                // Check height bounds
                int childEndY = contentY + child->margin.top + child->bounds.height;
                int containerEndY = contentY + contentH;
                
                if (childEndY > containerEndY && contentH > 0) {
                    // Resize child height to fit within container
                    int availableHeight = containerEndY - contentY - child->margin.top;
                    if (availableHeight > 10) { // Minimum height
                        child->bounds.height = availableHeight;
                    }
                }
                
                child->bounds.x = currentX;
                child->bounds.y = contentY + child->margin.top;
                
                currentX += child->bounds.width + child->margin.right;
                if (i < widget->childCount - 1) currentX += gap;
            }
            break;
        }
        
        default:
            // HAL_LAYOUT_NONE or HAL_LAYOUT_ABSOLUTE - no automatic positioning
            break;
    }
    
    // Recursively apply layout to children
    for (int i = 0; i < widget->childCount; i++) {
        if (widget->children[i] && widget->children[i]->layout != HAL_LAYOUT_NONE) {
            hal_widget_apply_layout(widget->children[i]);
        }
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
        // Save original sizes
        button->originalWidth = 120;
        button->originalHeight = 36;
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
