/*
 * HalGUI - Native Win32 Widgets
 * 
 * Uses native Win32 controls for full functionality (textarea, etc.)
 */

#include "halgui.h"
#include <commctrl.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ============================================
   Native Textarea Widget
   ============================================ */

typedef struct {
    HWND hwnd;          // Native Win32 EDIT control
    char* text;         // Text buffer
    int textCapacity;
    bool multiline;
    HBRUSH bgBrush;     // Background brush for theming
    COLORREF bgColor;   // Background color
    COLORREF textColor; // Text color
} HalTextareaData;

static WNDPROC g_originalEditProc = NULL;
static HalWidget* g_focusedTextarea = NULL;

// Subclass proc to handle events
static LRESULT CALLBACK TextareaSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    HalWidget* widget = (HalWidget*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    
    switch (msg) {
        case WM_SETFOCUS:
            g_focusedTextarea = widget;
            break;
        case WM_KILLFOCUS:
            if (g_focusedTextarea == widget) {
                g_focusedTextarea = NULL;
            }
            break;
    }
    
    return CallWindowProc(g_originalEditProc, hwnd, msg, wParam, lParam);
}

// Apply theme colors to textarea
void hal_textarea_set_colors(HalWidget* textarea, COLORREF bgColor, COLORREF textColor) {
    if (!textarea || textarea->type != HAL_WIDGET_TEXTAREA) return;
    HalTextareaData* data = (HalTextareaData*)textarea->data;
    if (!data) return;
    
    // Delete old brush
    if (data->bgBrush) {
        DeleteObject(data->bgBrush);
    }
    
    data->bgColor = bgColor;
    data->textColor = textColor;
    data->bgBrush = CreateSolidBrush(bgColor);
    
    // Force redraw
    if (data->hwnd) {
        InvalidateRect(data->hwnd, NULL, TRUE);
    }
}

// Get background brush for WM_CTLCOLOREDIT
HBRUSH hal_textarea_get_brush(HalWidget* textarea, HDC hdc) {
    if (!textarea || textarea->type != HAL_WIDGET_TEXTAREA) return NULL;
    HalTextareaData* data = (HalTextareaData*)textarea->data;
    if (!data) return NULL;
    
    if (data->bgBrush) {
        SetBkColor(hdc, data->bgColor);
        SetTextColor(hdc, data->textColor);
        return data->bgBrush;
    }
    return NULL;
}

HalWidget* hal_textarea_create(HalWidget* parent, const char* text) {
    HalWidget* textarea = (HalWidget*)calloc(1, sizeof(HalWidget));
    if (!textarea) return NULL;
    
    textarea->type = HAL_WIDGET_TEXTAREA;
    textarea->visible = true;
    textarea->enabled = true;
    textarea->opacity = 1.0f;
    
    HalTextareaData* data = (HalTextareaData*)calloc(1, sizeof(HalTextareaData));
    data->text = text ? _strdup(text) : _strdup("");
    data->textCapacity = 65536;
    data->multiline = true;
    textarea->data = data;
    
    textarea->bounds.width = 300;
    textarea->bounds.height = 200;
    
    if (parent) {
        textarea->parent = parent;
        if (parent->childCount < HAL_MAX_CHILDREN) {
            parent->children[parent->childCount++] = textarea;
        }
    }
    
    return textarea;
}

// Create the native HWND for textarea (called when window is shown)
void hal_textarea_create_native(HalWidget* textarea, HWND parentHwnd, float dpiScale) {
    if (!textarea || textarea->type != HAL_WIDGET_TEXTAREA) return;
    HalTextareaData* data = (HalTextareaData*)textarea->data;
    if (!data || data->hwnd) return;  // Already created
    
    int x = (int)(textarea->bounds.x * dpiScale);
    int y = (int)(textarea->bounds.y * dpiScale);
    int w = (int)(textarea->bounds.width * dpiScale);
    int h = (int)(textarea->bounds.height * dpiScale);
    
    DWORD style = WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL |
                  ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL | ES_WANTRETURN;
    
    data->hwnd = CreateWindowExW(
        0,  // No extended style - cleaner look
        L"EDIT",
        L"",
        style,
        x, y, w, h,
        parentHwnd,
        NULL,
        GetModuleHandle(NULL),
        NULL
    );
    
    if (data->hwnd) {
        // Set font
        HFONT font = CreateFontW(
            (int)(14 * dpiScale), 0, 0, 0,
            FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
            CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
            FIXED_PITCH | FF_MODERN,
            L"Consolas"
        );
        SendMessage(data->hwnd, WM_SETFONT, (WPARAM)font, TRUE);
        
        // Set initial text
        if (data->text && data->text[0]) {
            SetWindowTextA(data->hwnd, data->text);
        }
        
        // Store widget pointer for subclass
        SetWindowLongPtr(data->hwnd, GWLP_USERDATA, (LONG_PTR)textarea);
        
        // Subclass for event handling
        g_originalEditProc = (WNDPROC)SetWindowLongPtr(data->hwnd, GWLP_WNDPROC, (LONG_PTR)TextareaSubclassProc);
    }
}

void hal_textarea_set_text(HalWidget* textarea, const char* text) {
    if (!textarea || textarea->type != HAL_WIDGET_TEXTAREA) return;
    HalTextareaData* data = (HalTextareaData*)textarea->data;
    if (!data) return;
    
    free(data->text);
    data->text = text ? _strdup(text) : _strdup("");
    
    if (data->hwnd) {
        SetWindowTextA(data->hwnd, data->text);
    }
}

const char* hal_textarea_get_text(HalWidget* textarea) {
    if (!textarea || textarea->type != HAL_WIDGET_TEXTAREA) return NULL;
    HalTextareaData* data = (HalTextareaData*)textarea->data;
    if (!data) return NULL;
    
    // Sync from native control
    if (data->hwnd) {
        int len = GetWindowTextLengthA(data->hwnd);
        if (len >= data->textCapacity) {
            data->textCapacity = len + 1024;
            data->text = (char*)realloc(data->text, data->textCapacity);
        }
        GetWindowTextA(data->hwnd, data->text, data->textCapacity);
    }
    
    return data->text;
}

void hal_textarea_set_readonly(HalWidget* textarea, bool readonly) {
    if (!textarea || textarea->type != HAL_WIDGET_TEXTAREA) return;
    HalTextareaData* data = (HalTextareaData*)textarea->data;
    if (data && data->hwnd) {
        SendMessage(data->hwnd, EM_SETREADONLY, readonly ? TRUE : FALSE, 0);
    }
}

void hal_textarea_select_all(HalWidget* textarea) {
    if (!textarea || textarea->type != HAL_WIDGET_TEXTAREA) return;
    HalTextareaData* data = (HalTextareaData*)textarea->data;
    if (data && data->hwnd) {
        SendMessage(data->hwnd, EM_SETSEL, 0, -1);
    }
}

void hal_textarea_focus(HalWidget* textarea) {
    if (!textarea || textarea->type != HAL_WIDGET_TEXTAREA) return;
    HalTextareaData* data = (HalTextareaData*)textarea->data;
    if (data && data->hwnd) {
        SetFocus(data->hwnd);
    }
}

HWND hal_textarea_get_hwnd(HalWidget* textarea) {
    if (!textarea || textarea->type != HAL_WIDGET_TEXTAREA) return NULL;
    HalTextareaData* data = (HalTextareaData*)textarea->data;
    return data ? data->hwnd : NULL;
}

void hal_textarea_update_bounds(HalWidget* textarea, float dpiScale) {
    if (!textarea || textarea->type != HAL_WIDGET_TEXTAREA) return;
    HalTextareaData* data = (HalTextareaData*)textarea->data;
    if (!data || !data->hwnd) return;
    
    int x = (int)(textarea->bounds.x * dpiScale);
    int y = (int)(textarea->bounds.y * dpiScale);
    int w = (int)(textarea->bounds.width * dpiScale);
    int h = (int)(textarea->bounds.height * dpiScale);
    
    SetWindowPos(data->hwnd, NULL, x, y, w, h, SWP_NOZORDER);
}

void hal_textarea_destroy(HalWidget* textarea) {
    if (!textarea || textarea->type != HAL_WIDGET_TEXTAREA) return;
    HalTextareaData* data = (HalTextareaData*)textarea->data;
    if (data) {
        if (data->hwnd) {
            DestroyWindow(data->hwnd);
        }
        free(data->text);
        free(data);
    }
}

/* ============================================
   Native Input Widget (single line)
   Uses native Win32 EDIT control for proper text input
   ============================================ */

// Forward declaration of HalInputData from halgui_widgets.c
typedef struct {
    char* text;
    char* placeholder;
    bool isPassword;
    bool readonly;
    int cursorPos;
    int selectionStart;
    int selectionEnd;
    HWND hwnd;
    HBRUSH bgBrush;
    COLORREF bgColor;
    COLORREF textColor;
} HalInputData;

// Get HWND for input widget
HWND hal_input_get_hwnd(HalWidget* input) {
    if (!input || input->type != HAL_WIDGET_INPUT) return NULL;
    HalInputData* data = (HalInputData*)input->data;
    return data ? data->hwnd : NULL;
}

// Set colors for input widget
void hal_input_set_colors(HalWidget* input, COLORREF bgColor, COLORREF textColor) {
    if (!input || input->type != HAL_WIDGET_INPUT) return;
    HalInputData* data = (HalInputData*)input->data;
    if (!data) return;
    
    if (data->bgBrush) {
        DeleteObject(data->bgBrush);
    }
    
    data->bgColor = bgColor;
    data->textColor = textColor;
    data->bgBrush = CreateSolidBrush(bgColor);
    
    if (data->hwnd) {
        InvalidateRect(data->hwnd, NULL, TRUE);
    }
}

// Get background brush for WM_CTLCOLOREDIT
HBRUSH hal_input_get_brush(HalWidget* input, HDC hdc) {
    if (!input || input->type != HAL_WIDGET_INPUT) return NULL;
    HalInputData* data = (HalInputData*)input->data;
    if (!data) return NULL;
    
    if (data->bgBrush) {
        SetBkColor(hdc, data->bgColor);
        SetTextColor(hdc, data->textColor);
        return data->bgBrush;
    }
    return NULL;
}

// Create native HWND for input widget
void hal_input_create_native(HalWidget* input, HWND parentHwnd, float dpiScale) {
    if (!input || input->type != HAL_WIDGET_INPUT) return;
    HalInputData* data = (HalInputData*)input->data;
    if (!data) return;
    if (data->hwnd) return;  // Already created
    
    int x = (int)(input->bounds.x * dpiScale);
    int y = (int)(input->bounds.y * dpiScale);
    int w = (int)(input->bounds.width * dpiScale);
    int h = (int)(input->bounds.height * dpiScale);
    
    DWORD style = WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL;
    
    data->hwnd = CreateWindowExW(
        WS_EX_CLIENTEDGE,
        L"EDIT",
        L"",
        style,
        x, y, w, h,
        parentHwnd,
        NULL,
        GetModuleHandle(NULL),
        NULL
    );
    
    if (data->hwnd) {
        // Use Segoe UI for better Unicode support
        HFONT font = CreateFontW(
            (int)(14 * dpiScale), 0, 0, 0,
            FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
            CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE,
            L"Segoe UI"
        );
        SendMessage(data->hwnd, WM_SETFONT, (WPARAM)font, TRUE);
        
        // Set initial text if any
        if (data->text && data->text[0]) {
            int len = MultiByteToWideChar(CP_UTF8, 0, data->text, -1, NULL, 0);
            wchar_t* wText = (wchar_t*)malloc(len * sizeof(wchar_t));
            MultiByteToWideChar(CP_UTF8, 0, data->text, -1, wText, len);
            SetWindowTextW(data->hwnd, wText);
            free(wText);
        }
        
        // Set placeholder (cue banner)
        if (data->placeholder) {
            int len = MultiByteToWideChar(CP_UTF8, 0, data->placeholder, -1, NULL, 0);
            wchar_t* wPlaceholder = (wchar_t*)malloc(len * sizeof(wchar_t));
            MultiByteToWideChar(CP_UTF8, 0, data->placeholder, -1, wPlaceholder, len);
            SendMessage(data->hwnd, EM_SETCUEBANNER, TRUE, (LPARAM)wPlaceholder);
            free(wPlaceholder);
        }
        
        // Store widget pointer for event handling
        SetWindowLongPtr(data->hwnd, GWLP_USERDATA, (LONG_PTR)input);
    }
}

// Update input bounds when window resizes
void hal_input_update_bounds(HalWidget* input, float dpiScale) {
    if (!input || input->type != HAL_WIDGET_INPUT) return;
    HalInputData* data = (HalInputData*)input->data;
    if (!data || !data->hwnd) return;
    
    int x = (int)(input->bounds.x * dpiScale);
    int y = (int)(input->bounds.y * dpiScale);
    int w = (int)(input->bounds.width * dpiScale);
    int h = (int)(input->bounds.height * dpiScale);
    
    SetWindowPos(data->hwnd, NULL, x, y, w, h, SWP_NOZORDER);
}

/* ============================================
   Initialize native widgets after window creation
   ============================================ */

void hal_init_native_widgets(HalWindow* window) {
    if (!window || !window->hwnd) return;
    
    float dpiScale = hal_get_dpi_scale();
    HalTheme* theme = window->theme;
    
    // Iterate through all children and create native controls
    for (int i = 0; i < window->base.childCount; i++) {
        HalWidget* child = window->base.children[i];
        if (!child) continue;
        
        if (child->type == HAL_WIDGET_TEXTAREA) {
            hal_textarea_create_native(child, window->hwnd, dpiScale);
            
            // Apply theme colors to textarea
            if (theme) {
                COLORREF bgColor = RGB(HAL_GET_R(theme->surface), HAL_GET_G(theme->surface), HAL_GET_B(theme->surface));
                COLORREF textColor = RGB(HAL_GET_R(theme->textPrimary), HAL_GET_G(theme->textPrimary), HAL_GET_B(theme->textPrimary));
                hal_textarea_set_colors(child, bgColor, textColor);
            }
        }
        else if (child->type == HAL_WIDGET_INPUT) {
            hal_input_create_native(child, window->hwnd, dpiScale);
            
            // Apply theme colors to input
            if (theme) {
                COLORREF bgColor = RGB(HAL_GET_R(theme->surface), HAL_GET_G(theme->surface), HAL_GET_B(theme->surface));
                COLORREF textColor = RGB(HAL_GET_R(theme->textPrimary), HAL_GET_G(theme->textPrimary), HAL_GET_B(theme->textPrimary));
                hal_input_set_colors(child, bgColor, textColor);
            }
        }
    }
}

// Check if a widget uses native HWND
bool hal_widget_is_native(HalWidget* widget) {
    if (!widget) return false;
    return widget->type == HAL_WIDGET_TEXTAREA || widget->type == HAL_WIDGET_INPUT;
}
