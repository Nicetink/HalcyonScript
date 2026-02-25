/*
 * HalGUI - Standard GUI Framework for HalcyonScript
 * Version 1.0
 * 
 * Modern, themeable, hardware-accelerated GUI toolkit
 * Design Philosophy: Clean, Minimal, Elegant
 */

#ifndef HALGUI_H
#define HALGUI_H

#include <windows.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================
   Color System - HalGUI uses ARGB colors
   ============================================ */

typedef uint32_t HalColor;

#define HAL_RGBA(r, g, b, a) (((a) << 24) | ((r) << 16) | ((g) << 8) | (b))
#define HAL_RGB(r, g, b)     HAL_RGBA(r, g, b, 255)

#define HAL_GET_A(c) (((c) >> 24) & 0xFF)
#define HAL_GET_R(c) (((c) >> 16) & 0xFF)
#define HAL_GET_G(c) (((c) >> 8) & 0xFF)
#define HAL_GET_B(c) ((c) & 0xFF)

/* ============================================
   Theme System - HalGUI Design Language
   ============================================ */

typedef struct {
    // Primary colors
    HalColor primary;           // Main accent color
    HalColor primaryHover;      // Hover state
    HalColor primaryActive;     // Active/pressed state
    
    // Background colors
    HalColor background;        // Main background
    HalColor surface;           // Card/panel background
    HalColor surfaceHover;      // Hover state for surfaces
    HalColor elevated;          // Elevated elements (dropdowns, tooltips)
    
    // Text colors
    HalColor textPrimary;       // Main text
    HalColor textSecondary;     // Secondary/muted text
    HalColor textDisabled;      // Disabled text
    HalColor textOnPrimary;     // Text on primary color
    
    // Border colors
    HalColor border;            // Default border
    HalColor borderFocus;       // Focused element border
    HalColor borderError;       // Error state border
    
    // Status colors
    HalColor success;           // Success/positive
    HalColor warning;           // Warning/caution
    HalColor error;             // Error/danger
    HalColor info;              // Information
    
    // Shadows (alpha values for shadow layers)
    uint8_t shadowAlpha1;       // Light shadow
    uint8_t shadowAlpha2;       // Medium shadow
    uint8_t shadowAlpha3;       // Heavy shadow
    
    // Typography
    const char* fontFamily;
    int fontSizeSmall;
    int fontSizeNormal;
    int fontSizeLarge;
    int fontSizeTitle;
    
    // Spacing
    int spacingXS;              // 4px
    int spacingS;               // 8px
    int spacingM;               // 16px
    int spacingL;               // 24px
    int spacingXL;              // 32px
    
    // Border radius
    int radiusSmall;            // 4px
    int radiusNormal;           // 8px
    int radiusLarge;            // 12px
    int radiusRound;            // 9999px (pill shape)
    
    // Animation
    int animationFast;          // 100ms
    int animationNormal;        // 200ms
    int animationSlow;          // 300ms
    
} HalTheme;

/* Built-in themes */
extern HalTheme HAL_THEME_DARK;
extern HalTheme HAL_THEME_LIGHT;
extern HalTheme HAL_THEME_MIDNIGHT;
extern HalTheme HAL_THEME_OCEAN;
extern HalTheme HAL_THEME_TEAL;

/* ============================================
   Widget Base Types
   ============================================ */

typedef enum {
    HAL_WIDGET_WINDOW,
    HAL_WIDGET_PANEL,
    HAL_WIDGET_BUTTON,
    HAL_WIDGET_LABEL,
    HAL_WIDGET_INPUT,
    HAL_WIDGET_TEXTAREA,
    HAL_WIDGET_CHECKBOX,
    HAL_WIDGET_RADIO,
    HAL_WIDGET_SLIDER,
    HAL_WIDGET_PROGRESS,
    HAL_WIDGET_TOGGLE,          // Modern toggle switch
    HAL_WIDGET_DROPDOWN,
    HAL_WIDGET_LIST,
    HAL_WIDGET_TABS,
    HAL_WIDGET_MENU,
    HAL_WIDGET_TOOLBAR,
    HAL_WIDGET_STATUSBAR,
    HAL_WIDGET_CANVAS,
    HAL_WIDGET_IMAGE,
    HAL_WIDGET_SCROLLVIEW,
    HAL_WIDGET_SPLITVIEW,
    HAL_WIDGET_DIALOG,
    HAL_WIDGET_TOOLTIP,
    HAL_WIDGET_CUSTOM
} HalWidgetType;

typedef enum {
    HAL_STATE_NORMAL = 0,
    HAL_STATE_HOVER = 1,
    HAL_STATE_ACTIVE = 2,
    HAL_STATE_FOCUSED = 4,
    HAL_STATE_DISABLED = 8,
    HAL_STATE_CHECKED = 16,
    HAL_STATE_SELECTED = 32
} HalWidgetState;

typedef enum {
    HAL_ALIGN_LEFT,
    HAL_ALIGN_CENTER,
    HAL_ALIGN_RIGHT,
    HAL_ALIGN_TOP,
    HAL_ALIGN_MIDDLE,
    HAL_ALIGN_BOTTOM
} HalAlignment;

typedef enum {
    HAL_LAYOUT_NONE,
    HAL_LAYOUT_HORIZONTAL,
    HAL_LAYOUT_VERTICAL,
    HAL_LAYOUT_GRID,
    HAL_LAYOUT_FLEX,
    HAL_LAYOUT_ABSOLUTE,
    HAL_LAYOUT_RESPONSIVE     // Auto-resize with window
} HalLayoutType;

/* Layout anchors for responsive layout */
typedef enum {
    HAL_ANCHOR_NONE = 0,
    HAL_ANCHOR_LEFT = 1,
    HAL_ANCHOR_TOP = 2,
    HAL_ANCHOR_RIGHT = 4,
    HAL_ANCHOR_BOTTOM = 8,
    HAL_ANCHOR_ALL = 15
} HalAnchor;

/* Layout constraints for responsive widgets */
typedef struct {
    float leftPercent;      // 0.0-1.0, position as % of parent width
    float topPercent;       // 0.0-1.0, position as % of parent height
    float widthPercent;     // 0.0-1.0, width as % of parent width
    float heightPercent;    // 0.0-1.0, height as % of parent height
    int leftMargin;         // Fixed margin from left
    int topMargin;          // Fixed margin from top
    int rightMargin;        // Fixed margin from right
    int bottomMargin;       // Fixed margin from bottom
    HalAnchor anchors;      // Which edges to anchor
    bool usePercent;        // Use percentage-based layout
} HalLayoutConstraints;

/* Forward declarations */
typedef struct HalWidget HalWidget;
typedef struct HalWindow HalWindow;
typedef struct HalEvent HalEvent;

/* ============================================
   Event System
   ============================================ */

typedef enum {
    HAL_EVENT_NONE,
    HAL_EVENT_CLICK,
    HAL_EVENT_DOUBLE_CLICK,
    HAL_EVENT_MOUSE_DOWN,
    HAL_EVENT_MOUSE_UP,
    HAL_EVENT_MOUSE_MOVE,
    HAL_EVENT_MOUSE_ENTER,
    HAL_EVENT_MOUSE_LEAVE,
    HAL_EVENT_MOUSE_WHEEL,
    HAL_EVENT_KEY_DOWN,
    HAL_EVENT_KEY_UP,
    HAL_EVENT_KEY_PRESS,
    HAL_EVENT_FOCUS,
    HAL_EVENT_BLUR,
    HAL_EVENT_CHANGE,
    HAL_EVENT_INPUT,
    HAL_EVENT_SUBMIT,
    HAL_EVENT_RESIZE,
    HAL_EVENT_CLOSE,
    HAL_EVENT_PAINT,
    HAL_EVENT_TIMER,
    HAL_EVENT_DRAG_START,
    HAL_EVENT_DRAG,
    HAL_EVENT_DRAG_END,
    HAL_EVENT_DROP
} HalEventType;

struct HalEvent {
    HalEventType type;
    HalWidget* target;
    HalWidget* currentTarget;
    
    // Mouse data
    int mouseX;
    int mouseY;
    int deltaX;
    int deltaY;
    int button;         // 0=left, 1=middle, 2=right
    
    // Keyboard data
    int keyCode;
    int scanCode;
    wchar_t character;
    bool ctrlKey;
    bool shiftKey;
    bool altKey;
    
    // Misc
    void* data;
    bool handled;
    bool propagate;
};

typedef void (*HalEventHandler)(HalWidget* widget, HalEvent* event, void* userData);

/* ============================================
   Geometry Types
   ============================================ */

typedef struct {
    int x, y;
} HalPoint;

typedef struct {
    int width, height;
} HalSize;

typedef struct {
    int x, y, width, height;
} HalRect;

typedef struct {
    int top, right, bottom, left;
} HalInsets;

/* ============================================
   Widget Structure
   ============================================ */

#define HAL_MAX_CHILDREN 256
#define HAL_MAX_EVENT_HANDLERS 16

typedef struct {
    HalEventType type;
    HalEventHandler handler;
    void* userData;
} HalEventBinding;

struct HalWidget {
    HalWidgetType type;
    char* id;
    char* className;
    
    // Geometry
    HalRect bounds;
    HalRect contentBounds;
    HalInsets padding;
    HalInsets margin;
    int minWidth, minHeight;
    int maxWidth, maxHeight;
    
    // Original sizes for responsive layout
    int originalWidth, originalHeight;
    
    // State
    uint32_t state;
    bool visible;
    bool enabled;
    float opacity;
    
    // Hierarchy
    HalWidget* parent;
    HalWidget* children[HAL_MAX_CHILDREN];
    int childCount;
    
    // Layout
    HalLayoutType layout;
    HalAlignment alignH;
    HalAlignment alignV;
    int gap;
    float flex;
    
    // Style overrides
    HalColor* bgColor;
    HalColor* fgColor;
    HalColor* borderColor;
    int* borderWidth;
    int* borderRadius;
    
    // Events
    HalEventBinding events[HAL_MAX_EVENT_HANDLERS];
    int eventCount;
    
    // Custom data
    void* data;
    void* nativeHandle;
    HWND hwnd;  // For popup windows (notifications, tooltips, etc.)
    
    // Animation state
    float animProgress;
    int animDuration;
    bool animating;
    
    // Responsive layout
    HalLayoutConstraints constraints;
    HalRect initialBounds;      // Original bounds for percentage calculation
    bool hasConstraints;
};

/* ============================================
   Window Structure
   ============================================ */

typedef enum {
    HAL_WINDOW_NORMAL,
    HAL_WINDOW_FRAMELESS,
    HAL_WINDOW_DIALOG,
    HAL_WINDOW_POPUP,
    HAL_WINDOW_TOOLTIP
} HalWindowStyle;

struct HalWindow {
    HalWidget base;
    
    char* title;
    HalWindowStyle style;
    bool resizable;
    bool maximizable;
    bool minimizable;
    bool closable;
    bool modal;
    bool topmost;
    
    // Native
    HWND hwnd;
    HDC hdc;
    HBITMAP backBuffer;
    HDC backBufferDC;
    
    // Theme
    HalTheme* theme;
    
    // State
    bool isMaximized;
    bool isMinimized;
    bool isFullscreen;
    HalRect restoreBounds;
};

/* ============================================
   Core API
   ============================================ */

// Initialization
bool hal_init(void);
void hal_shutdown(void);
void hal_set_theme(HalTheme* theme);
HalTheme* hal_get_theme(void);

// Main loop
void hal_run(void);
void hal_quit(void);
bool hal_process_events(void);

// Window management
HalWindow* hal_window_create(const char* title, int width, int height);
HalWindow* hal_window_create_ex(const char* title, int x, int y, int width, int height, HalWindowStyle style);
void hal_window_destroy(HalWindow* window);
void hal_window_show(HalWindow* window);
void hal_window_hide(HalWindow* window);
void hal_window_close(HalWindow* window);
void hal_window_maximize(HalWindow* window);
void hal_window_minimize(HalWindow* window);
void hal_window_restore(HalWindow* window);
void hal_window_set_title(HalWindow* window, const char* title);
void hal_window_set_icon(HalWindow* window, const char* iconPath);
void hal_window_center(HalWindow* window);
void hal_window_set_theme(HalWindow* window, HalTheme* theme);

/* ============================================
   Widget Creation API
   ============================================ */

// Panel - Container widget
HalWidget* hal_panel_create(HalWidget* parent);
void hal_panel_set_layout(HalWidget* panel, HalLayoutType layout);
void hal_panel_set_gap(HalWidget* panel, int gap);
void hal_widget_apply_layout(HalWidget* widget);

// General layout functions
void hal_widget_set_layout(HalWidget* widget, HalLayoutType layout);
void hal_widget_set_gap(HalWidget* widget, int gap);
void hal_widget_set_flex(HalWidget* widget, float flex);

// Button
HalWidget* hal_button_create(HalWidget* parent, const char* text);
void hal_button_set_text(HalWidget* button, const char* text);
const char* hal_button_get_text(HalWidget* button);
void hal_button_set_icon(HalWidget* button, const char* iconPath);

// Label
HalWidget* hal_label_create(HalWidget* parent, const char* text);
void hal_label_set_text(HalWidget* label, const char* text);
const char* hal_label_get_text(HalWidget* label);
void hal_label_set_align(HalWidget* label, HalAlignment align);

// Input (single-line text) - Uses Win32 EDIT control
HalWidget* hal_input_create(HalWidget* parent, const char* placeholder);
void hal_input_set_text(HalWidget* input, const char* text);
const char* hal_input_get_text(HalWidget* input);
void hal_input_set_placeholder(HalWidget* input, const char* placeholder);
void hal_input_set_password(HalWidget* input, bool isPassword);
void hal_input_set_readonly(HalWidget* input, bool readonly);
void hal_input_create_native(HalWidget* input, HWND parentHwnd, float dpiScale);
void hal_input_update_bounds(HalWidget* input, float dpiScale);
void hal_input_set_colors(HalWidget* input, COLORREF bgColor, COLORREF textColor);
HBRUSH hal_input_get_brush(HalWidget* input, HDC hdc);
HWND hal_input_get_hwnd(HalWidget* input);

// TextArea (multi-line text) - Uses Win32 EDIT control
HalWidget* hal_textarea_create(HalWidget* parent, const char* text);
void hal_textarea_set_text(HalWidget* textarea, const char* text);
const char* hal_textarea_get_text(HalWidget* textarea);
void hal_textarea_set_readonly(HalWidget* textarea, bool readonly);
void hal_textarea_select_all(HalWidget* textarea);
void hal_textarea_focus(HalWidget* textarea);
void hal_textarea_create_native(HalWidget* textarea, HWND parentHwnd, float dpiScale);
void hal_textarea_update_bounds(HalWidget* textarea, float dpiScale);
void hal_textarea_destroy(HalWidget* textarea);
void hal_textarea_set_colors(HalWidget* textarea, COLORREF bgColor, COLORREF textColor);
HBRUSH hal_textarea_get_brush(HalWidget* textarea, HDC hdc);
HWND hal_textarea_get_hwnd(HalWidget* textarea);
void hal_init_native_widgets(HalWindow* window);
bool hal_widget_is_native(HalWidget* widget);
float hal_get_dpi_scale(void);

// Checkbox
HalWidget* hal_checkbox_create(HalWidget* parent, const char* label);
void hal_checkbox_set_checked(HalWidget* checkbox, bool checked);
bool hal_checkbox_is_checked(HalWidget* checkbox);

// Toggle Switch (modern alternative to checkbox)
HalWidget* hal_toggle_create(HalWidget* parent, const char* label);
void hal_toggle_set_checked(HalWidget* toggle, bool checked);
bool hal_toggle_is_checked(HalWidget* toggle);

// Radio button
HalWidget* hal_radio_create(HalWidget* parent, const char* label, const char* group);
void hal_radio_set_checked(HalWidget* radio, bool checked);
bool hal_radio_is_checked(HalWidget* radio);

// Slider
HalWidget* hal_slider_create(HalWidget* parent, int min, int max, int value);
void hal_slider_set_value(HalWidget* slider, int value);
int hal_slider_get_value(HalWidget* slider);
void hal_slider_set_range(HalWidget* slider, int min, int max);

// Progress bar
HalWidget* hal_progress_create(HalWidget* parent);
void hal_progress_set_value(HalWidget* progress, float value);  // 0.0 - 1.0
float hal_progress_get_value(HalWidget* progress);
void hal_progress_set_indeterminate(HalWidget* progress, bool indeterminate);

// Dropdown/ComboBox
HalWidget* hal_dropdown_create(HalWidget* parent);
void hal_dropdown_add_item(HalWidget* dropdown, const char* text, void* data);
void hal_dropdown_remove_item(HalWidget* dropdown, int index);
void hal_dropdown_clear(HalWidget* dropdown);
int hal_dropdown_get_selected(HalWidget* dropdown);
void hal_dropdown_set_selected(HalWidget* dropdown, int index);

// List
HalWidget* hal_list_create(HalWidget* parent);
void hal_list_add_item(HalWidget* list, const char* text, void* data);
void hal_list_remove_item(HalWidget* list, int index);
void hal_list_clear(HalWidget* list);
int hal_list_get_selected(HalWidget* list);
void hal_list_set_selected(HalWidget* list, int index);

// Tabs
HalWidget* hal_tabs_create(HalWidget* parent);
HalWidget* hal_tabs_add_tab(HalWidget* tabs, const char* title);
void hal_tabs_remove_tab(HalWidget* tabs, int index);
int hal_tabs_get_active(HalWidget* tabs);
void hal_tabs_set_active(HalWidget* tabs, int index);

// Canvas (for custom drawing)
HalWidget* hal_canvas_create(HalWidget* parent, int width, int height);
void hal_canvas_clear(HalWidget* canvas, HalColor color);
void hal_canvas_draw_rect(HalWidget* canvas, HalRect rect, HalColor color);
void hal_canvas_fill_rect(HalWidget* canvas, HalRect rect, HalColor color);
void hal_canvas_draw_line(HalWidget* canvas, int x1, int y1, int x2, int y2, HalColor color);
void hal_canvas_draw_circle(HalWidget* canvas, int cx, int cy, int radius, HalColor color);
void hal_canvas_fill_circle(HalWidget* canvas, int cx, int cy, int radius, HalColor color);
void hal_canvas_draw_text(HalWidget* canvas, int x, int y, const char* text, HalColor color);

// Calendar
HalWidget* hal_calendar_create(HalWidget* parent);
void hal_calendar_set_date(HalWidget* cal, int year, int month, int day);
void hal_calendar_get_selected_date(HalWidget* cal, int* year, int* month, int* day);
void hal_calendar_next_month(HalWidget* cal);
void hal_calendar_prev_month(HalWidget* cal);
bool hal_calendar_handle_click(HalWidget* cal, int mouseX, int mouseY);

// Image
HalWidget* hal_image_create(HalWidget* parent, const char* path);
void hal_image_set_source(HalWidget* image, const char* path);
void hal_image_set_scale_mode(HalWidget* image, int mode);  // 0=fit, 1=fill, 2=stretch

// ScrollView
HalWidget* hal_scrollview_create(HalWidget* parent);
void hal_scrollview_scroll_to(HalWidget* scrollview, int x, int y);

// SplitView
HalWidget* hal_splitview_create(HalWidget* parent, bool horizontal);
void hal_splitview_set_ratio(HalWidget* splitview, float ratio);

/* ============================================
   Widget Common API
   ============================================ */

// Geometry
void hal_widget_set_bounds(HalWidget* widget, int x, int y, int width, int height);
void hal_widget_set_position(HalWidget* widget, int x, int y);
void hal_widget_set_size(HalWidget* widget, int width, int height);
HalRect hal_widget_get_bounds(HalWidget* widget);
void hal_widget_set_padding(HalWidget* widget, int top, int right, int bottom, int left);
void hal_widget_set_margin(HalWidget* widget, int top, int right, int bottom, int left);

// Visibility & State
void hal_widget_show(HalWidget* widget);
void hal_widget_hide(HalWidget* widget);
void hal_widget_enable(HalWidget* widget);
void hal_widget_disable(HalWidget* widget);
void hal_widget_focus(HalWidget* widget);
bool hal_widget_is_visible(HalWidget* widget);
bool hal_widget_is_enabled(HalWidget* widget);
bool hal_widget_has_focus(HalWidget* widget);

// Style
void hal_widget_set_background(HalWidget* widget, HalColor color);
void hal_widget_set_foreground(HalWidget* widget, HalColor color);
void hal_widget_set_border(HalWidget* widget, int width, HalColor color);
void hal_widget_set_border_radius(HalWidget* widget, int radius);
void hal_widget_set_opacity(HalWidget* widget, float opacity);

// Layout
void hal_widget_set_align(HalWidget* widget, HalAlignment h, HalAlignment v);

// Responsive layout
void hal_widget_set_constraints(HalWidget* widget, float leftPct, float topPct, float widthPct, float heightPct);
void hal_widget_set_anchors(HalWidget* widget, HalAnchor anchors);
void hal_widget_set_margins(HalWidget* widget, int left, int top, int right, int bottom);
void hal_window_enable_responsive(HalWindow* window, bool enable);
void hal_window_apply_layout(HalWindow* window);

// Events
void hal_widget_on(HalWidget* widget, HalEventType type, HalEventHandler handler, void* userData);
void hal_widget_off(HalWidget* widget, HalEventType type, HalEventHandler handler);

// Hierarchy
void hal_widget_add_child(HalWidget* parent, HalWidget* child);
void hal_widget_remove_child(HalWidget* parent, HalWidget* child);
HalWidget* hal_widget_find_by_id(HalWidget* root, const char* id);

// Destruction
void hal_widget_destroy(HalWidget* widget);

// Refresh
void hal_widget_invalidate(HalWidget* widget);
void hal_widget_update(HalWidget* widget);

/* ============================================
   Dialogs
   ============================================ */

typedef enum {
    HAL_DIALOG_OK,
    HAL_DIALOG_OK_CANCEL,
    HAL_DIALOG_YES_NO,
    HAL_DIALOG_YES_NO_CANCEL
} HalDialogButtons;

typedef enum {
    HAL_DIALOG_INFO,
    HAL_DIALOG_WARNING,
    HAL_DIALOG_ERROR,
    HAL_DIALOG_QUESTION
} HalDialogIcon;

int hal_dialog_message(HalWindow* parent, const char* title, const char* message, 
                       HalDialogButtons buttons, HalDialogIcon icon);
char* hal_dialog_input(HalWindow* parent, const char* title, const char* prompt, const char* defaultValue);
char* hal_dialog_open_file(HalWindow* parent, const char* title, const char* filter);
char* hal_dialog_save_file(HalWindow* parent, const char* title, const char* filter, const char* defaultName);
char* hal_dialog_select_folder(HalWindow* parent, const char* title);
HalColor hal_dialog_color_picker(HalWindow* parent, HalColor initialColor);

/* ============================================
   Utility Functions
   ============================================ */

// Timers
int hal_timer_create(int intervalMs, HalEventHandler callback, void* userData);
void hal_timer_destroy(int timerId);

// Clipboard
void hal_clipboard_set_text(const char* text);
char* hal_clipboard_get_text(void);
bool hal_clipboard_has_text(void);

// Drag & Drop
void hal_widget_enable_drag(HalWidget* widget, bool enable);
void hal_widget_enable_drop(HalWidget* widget, bool enable);
void hal_widget_set_drag_data(HalWidget* widget, const char* data);
char* hal_widget_get_drag_data(HalWidget* widget);

// Clipboard
void hal_clipboard_set_text(const char* text);
char* hal_clipboard_get_text(void);
bool hal_clipboard_has_text(void);

// Screen info
HalSize hal_screen_get_size(void);
HalRect hal_screen_get_work_area(void);

// DPI
float hal_get_dpi_scale(void);

/* ============================================
   Internal State (for cross-module access)
   ============================================ */

// Track slider being dragged (used by audio update logic)
extern HalWidget* g_draggingSlider;

#ifdef __cplusplus
}
#endif

#endif /* HALGUI_H */
