/*
 * HalGUI Extended Widgets
 * Copyright (C) 2026 KAInaps
 * 
 * Advanced widgets for modern applications:
 * - TreeView (hierarchical data)
 * - DataGrid (table with sorting/filtering)
 * - Chart (line, bar, pie charts)
 * - Calendar (date picker)
 * - ColorPicker (color selection)
 * - FileTree (file system browser)
 * - RichTextEditor (formatted text)
 * - Notification (toast messages)
 * - ContextMenu (right-click menu)
 * - Tooltip (hover hints)
 */

#include "halgui.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

/* External rendering functions from halgui_render.c */
extern void hal_render_rounded_rect(void* graphics, float x, float y, float w, float h, float radius, uint32_t color, bool fill);
extern void hal_render_text(void* graphics, const char* text, float x, float y, float w, float h, uint32_t color, int align);
extern void hal_render_line(void* graphics, float x1, float y1, float x2, float y2, uint32_t color, float width);
extern void* hal_get_graphics_from_hdc(HDC hdc);
extern void hal_release_graphics(void* graphics);

/* Get current theme */
extern HalTheme* hal_get_theme(void);

/* ============================================
   TreeView Widget
   ============================================ */

typedef struct HalTreeNode {
    char* text;
    void* data;
    bool expanded;
    bool selected;
    struct HalTreeNode* parent;
    struct HalTreeNode** children;
    int childCount;
    int childCapacity;
} HalTreeNode;

typedef struct {
    HalTreeNode* root;
    HalTreeNode* selected;
    int itemHeight;
    int indentWidth;
    bool showLines;
    bool showIcons;
} HalTreeViewData;

HalWidget* hal_treeview_create(HalWidget* parent) {
    HalWidget* tree = (HalWidget*)calloc(1, sizeof(HalWidget));
    tree->type = HAL_WIDGET_LIST;  // Reuse list type
    tree->parent = parent;
    tree->visible = true;
    tree->enabled = true;
    tree->opacity = 1.0f;
    
    HalTreeViewData* data = (HalTreeViewData*)calloc(1, sizeof(HalTreeViewData));
    data->root = (HalTreeNode*)calloc(1, sizeof(HalTreeNode));
    data->root->text = strdup("Root");
    data->itemHeight = 24;
    data->indentWidth = 20;
    data->showLines = true;
    data->showIcons = false;
    tree->data = data;
    
    if (parent) {
        hal_widget_add_child(parent, tree);
    }
    
    return tree;
}

HalTreeNode* hal_treeview_add_node(HalWidget* tree, HalTreeNode* parent, const char* text, void* userData) {
    if (!tree || tree->type != HAL_WIDGET_LIST) return NULL;
    
    HalTreeViewData* data = (HalTreeViewData*)tree->data;
    if (!parent) parent = data->root;
    
    HalTreeNode* node = (HalTreeNode*)calloc(1, sizeof(HalTreeNode));
    node->text = strdup(text);
    node->data = userData;
    node->parent = parent;
    node->expanded = false;
    node->selected = false;
    
    // Add to parent
    if (parent->childCount >= parent->childCapacity) {
        parent->childCapacity = parent->childCapacity == 0 ? 8 : parent->childCapacity * 2;
        parent->children = (HalTreeNode**)realloc(parent->children, 
                                                  parent->childCapacity * sizeof(HalTreeNode*));
    }
    parent->children[parent->childCount++] = node;
    
    hal_widget_invalidate(tree);
    return node;
}

void hal_treeview_expand(HalWidget* tree, HalTreeNode* node) {
    if (!tree || !node) return;
    node->expanded = true;
    hal_widget_invalidate(tree);
}

void hal_treeview_collapse(HalWidget* tree, HalTreeNode* node) {
    if (!tree || !node) return;
    node->expanded = false;
    hal_widget_invalidate(tree);
}

HalTreeNode* hal_treeview_get_selected(HalWidget* tree) {
    if (!tree) return NULL;
    HalTreeViewData* data = (HalTreeViewData*)tree->data;
    return data->selected;
}

/* ============================================
   DataGrid Widget
   ============================================ */

typedef struct {
    char* name;
    int width;
    HalAlignment align;
} HalGridColumn;

typedef struct {
    char** cells;
    void* data;
    bool selected;
} HalGridRow;

typedef struct {
    HalGridColumn* columns;
    int columnCount;
    HalGridRow* rows;
    int rowCount;
    int rowCapacity;
    int headerHeight;
    int rowHeight;
    int sortColumn;
    bool sortAscending;
    bool showGrid;
    bool alternateRows;
} HalDataGridData;

HalWidget* hal_datagrid_create(HalWidget* parent) {
    HalWidget* grid = (HalWidget*)calloc(1, sizeof(HalWidget));
    grid->type = HAL_WIDGET_LIST;
    grid->parent = parent;
    grid->visible = true;
    grid->enabled = true;
    grid->opacity = 1.0f;
    
    HalDataGridData* data = (HalDataGridData*)calloc(1, sizeof(HalDataGridData));
    data->headerHeight = 32;
    data->rowHeight = 28;
    data->sortColumn = -1;
    data->sortAscending = true;
    data->showGrid = true;
    data->alternateRows = true;
    grid->data = data;
    
    if (parent) {
        hal_widget_add_child(parent, grid);
    }
    
    return grid;
}

void hal_datagrid_add_column(HalWidget* grid, const char* name, int width, HalAlignment align) {
    if (!grid) return;
    
    HalDataGridData* data = (HalDataGridData*)grid->data;
    data->columns = (HalGridColumn*)realloc(data->columns, 
                                            (data->columnCount + 1) * sizeof(HalGridColumn));
    
    HalGridColumn* col = &data->columns[data->columnCount];
    col->name = strdup(name);
    col->width = width;
    col->align = align;
    data->columnCount++;
    
    hal_widget_invalidate(grid);
}

void hal_datagrid_add_row(HalWidget* grid, const char** cells, void* userData) {
    if (!grid) return;
    
    HalDataGridData* data = (HalDataGridData*)grid->data;
    
    if (data->rowCount >= data->rowCapacity) {
        data->rowCapacity = data->rowCapacity == 0 ? 16 : data->rowCapacity * 2;
        data->rows = (HalGridRow*)realloc(data->rows, data->rowCapacity * sizeof(HalGridRow));
    }
    
    HalGridRow* row = &data->rows[data->rowCount];
    row->cells = (char**)calloc(data->columnCount, sizeof(char*));
    for (int i = 0; i < data->columnCount; i++) {
        row->cells[i] = strdup(cells[i]);
    }
    row->data = userData;
    row->selected = false;
    data->rowCount++;
    
    hal_widget_invalidate(grid);
}

void hal_datagrid_clear(HalWidget* grid) {
    if (!grid) return;
    
    HalDataGridData* data = (HalDataGridData*)grid->data;
    for (int i = 0; i < data->rowCount; i++) {
        for (int j = 0; j < data->columnCount; j++) {
            free(data->rows[i].cells[j]);
        }
        free(data->rows[i].cells);
    }
    free(data->rows);
    data->rows = NULL;
    data->rowCount = 0;
    data->rowCapacity = 0;
    
    hal_widget_invalidate(grid);
}

/* ============================================
   Chart Widget
   ============================================ */

typedef enum {
    HAL_CHART_LINE,
    HAL_CHART_BAR,
    HAL_CHART_PIE,
    HAL_CHART_AREA,
    HAL_CHART_SCATTER
} HalChartType;

typedef struct {
    char* label;
    float* values;
    int valueCount;
    HalColor color;
} HalChartSeries;

typedef struct {
    HalChartType type;
    char* title;
    char* xLabel;
    char* yLabel;
    HalChartSeries* series;
    int seriesCount;
    char** xLabels;
    int xLabelCount;
    float minValue;
    float maxValue;
    bool showLegend;
    bool showGrid;
    bool animated;
} HalChartData;

HalWidget* hal_chart_create(HalWidget* parent, HalChartType type) {
    HalWidget* chart = (HalWidget*)calloc(1, sizeof(HalWidget));
    chart->type = HAL_WIDGET_CANVAS;
    chart->parent = parent;
    chart->visible = true;
    chart->enabled = true;
    chart->opacity = 1.0f;
    
    HalChartData* data = (HalChartData*)calloc(1, sizeof(HalChartData));
    data->type = type;
    data->showLegend = true;
    data->showGrid = true;
    data->animated = true;
    data->minValue = 0.0f;
    data->maxValue = 100.0f;
    chart->data = data;
    
    if (parent) {
        hal_widget_add_child(parent, chart);
    }
    
    return chart;
}

void hal_chart_add_series(HalWidget* chart, const char* label, float* values, int count, HalColor color) {
    if (!chart) return;
    
    HalChartData* data = (HalChartData*)chart->data;
    data->series = (HalChartSeries*)realloc(data->series, 
                                            (data->seriesCount + 1) * sizeof(HalChartSeries));
    
    HalChartSeries* series = &data->series[data->seriesCount];
    series->label = strdup(label);
    series->values = (float*)malloc(count * sizeof(float));
    memcpy(series->values, values, count * sizeof(float));
    series->valueCount = count;
    series->color = color;
    data->seriesCount++;
    
    hal_widget_invalidate(chart);
}

void hal_chart_set_title(HalWidget* chart, const char* title) {
    if (!chart) return;
    HalChartData* data = (HalChartData*)chart->data;
    free(data->title);
    data->title = strdup(title);
    hal_widget_invalidate(chart);
}

/* ============================================
   Calendar Widget
   ============================================ */

typedef struct {
    int year;
    int month;
    int day;              // Current day (for highlighting today)
    int selectedDay;      // Selected day
    int selectedMonth;    // Selected month
    int selectedYear;     // Selected year
    int firstDayOfWeek;   // 0=Sunday, 1=Monday
    bool showWeekNumbers;
    HalEventHandler onDateChange;  // Callback when date is selected
    void* onDateChangeUserData;
} HalCalendarData;

// Helper: Get first day of month (0=Sunday, 1=Monday, etc.)
static int getFirstDayOfMonth(int year, int month) {
    // Zeller's congruence algorithm
    int q = 1;  // First day of month
    int m = month;
    int y = year;
    
    if (m < 3) {
        m += 12;
        y--;
    }
    
    int k = y % 100;
    int j = y / 100;
    int h = (q + ((13 * (m + 1)) / 5) + k + (k / 4) + (j / 4) - (2 * j)) % 7;
    
    // Convert to 0=Sunday format
    int dayOfWeek = ((h + 6) % 7);
    return dayOfWeek;
}

// Helper: Get days in month
static int getDaysInMonth(int year, int month) {
    int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (month == 2 && ((year % 4 == 0 && year % 100 != 0) || year % 400 == 0)) {
        return 29;
    }
    return days[month - 1];
}

HalWidget* hal_calendar_create(HalWidget* parent) {
    HalWidget* cal = (HalWidget*)calloc(1, sizeof(HalWidget));
    cal->type = HAL_WIDGET_CUSTOM;
    cal->parent = parent;
    cal->visible = true;
    cal->enabled = true;
    cal->opacity = 1.0f;
    
    HalCalendarData* data = (HalCalendarData*)calloc(1, sizeof(HalCalendarData));
    
    // Get current date
    SYSTEMTIME st;
    GetLocalTime(&st);
    data->year = st.wYear;
    data->month = st.wMonth;
    data->day = st.wDay;
    data->selectedDay = st.wDay;
    data->selectedMonth = st.wMonth;
    data->selectedYear = st.wYear;
    data->firstDayOfWeek = 1;  // Monday
    data->showWeekNumbers = false;
    data->onDateChange = NULL;
    data->onDateChangeUserData = NULL;
    data->showWeekNumbers = false;
    
    cal->data = data;
    
    if (parent) {
        hal_widget_add_child(parent, cal);
    }
    
    return cal;
}

void hal_calendar_set_date(HalWidget* cal, int year, int month, int day) {
    if (!cal) return;
    HalCalendarData* data = (HalCalendarData*)cal->data;
    data->year = year;
    data->month = month;
    data->selectedDay = day;
    data->selectedMonth = month;
    data->selectedYear = year;
    hal_widget_invalidate(cal);
}

void hal_calendar_get_selected_date(HalWidget* cal, int* year, int* month, int* day) {
    if (!cal) return;
    HalCalendarData* data = (HalCalendarData*)cal->data;
    if (year) *year = data->selectedYear;
    if (month) *month = data->selectedMonth;
    if (day) *day = data->selectedDay;
}

void hal_calendar_next_month(HalWidget* cal) {
    if (!cal) return;
    HalCalendarData* data = (HalCalendarData*)cal->data;
    data->month++;
    if (data->month > 12) {
        data->month = 1;
        data->year++;
    }
    hal_widget_invalidate(cal);
}

void hal_calendar_prev_month(HalWidget* cal) {
    if (!cal) return;
    HalCalendarData* data = (HalCalendarData*)cal->data;
    data->month--;
    if (data->month < 1) {
        data->month = 12;
        data->year--;
    }
    hal_widget_invalidate(cal);
}

void hal_calendar_set_on_date_change(HalWidget* cal, HalEventHandler handler, void* userData) {
    if (!cal) return;
    HalCalendarData* data = (HalCalendarData*)cal->data;
    data->onDateChange = handler;
    data->onDateChangeUserData = userData;
}

// Handle mouse click on calendar
bool hal_calendar_handle_click(HalWidget* cal, int mouseX, int mouseY) {
    if (!cal || !cal->data) return false;
    
    HalCalendarData* data = (HalCalendarData*)cal->data;
    
    // Calculate cell dimensions
    int cellW = cal->bounds.width / 7;
    int cellH = 30;
    int headerHeight = 50;
    int dayNamesHeight = 30;
    int startY = cal->bounds.y + headerHeight + dayNamesHeight;
    
    // Check if click is in navigation buttons
    if (mouseY >= cal->bounds.y + 10 && mouseY <= cal->bounds.y + 40) {
        // Previous month button (left side)
        if (mouseX >= cal->bounds.x + 10 && mouseX <= cal->bounds.x + 40) {
            hal_calendar_prev_month(cal);
            return true;
        }
        // Next month button (right side)
        if (mouseX >= cal->bounds.x + cal->bounds.width - 40 && 
            mouseX <= cal->bounds.x + cal->bounds.width - 10) {
            hal_calendar_next_month(cal);
            return true;
        }
    }
    
    // Check if click is in day grid
    if (mouseY < startY) return false;
    
    int relX = mouseX - cal->bounds.x;
    int relY = mouseY - startY;
    
    if (relX < 0 || relX >= cal->bounds.width) return false;
    
    int col = relX / cellW;
    int row = relY / cellH;
    
    if (col < 0 || col >= 7 || row < 0 || row >= 6) return false;
    
    // Calculate which day was clicked
    int firstDay = getFirstDayOfMonth(data->year, data->month);
    int daysInMonth = getDaysInMonth(data->year, data->month);
    
    // Adjust for Monday start
    if (data->firstDayOfWeek == 1) {
        firstDay = (firstDay + 6) % 7;
    }
    
    int dayIndex = row * 7 + col;
    int day = dayIndex - firstDay + 1;
    
    if (day >= 1 && day <= daysInMonth) {
        data->selectedDay = day;
        data->selectedMonth = data->month;
        data->selectedYear = data->year;
        
        // Fire onChange event
        if (data->onDateChange) {
            HalEvent event = {0};
            event.type = HAL_EVENT_CHANGE;
            event.target = cal;
            data->onDateChange(cal, &event, data->onDateChangeUserData);
        }
        
        hal_widget_invalidate(cal);
        return true;
    }
    
    return false;
}
void hal_calendar_get_date(HalWidget* cal, int* year, int* month, int* day) {
    if (!cal) return;
    HalCalendarData* data = (HalCalendarData*)cal->data;
    if (year) *year = data->year;
    if (month) *month = data->month;
    if (day) *day = data->selectedDay;
}

/* ============================================
   Notification Widget (Toast)
   ============================================ */

typedef enum {
    HAL_NOTIFY_INFO,
    HAL_NOTIFY_SUCCESS,
    HAL_NOTIFY_WARNING,
    HAL_NOTIFY_ERROR
} HalNotifyType;

typedef struct {
    char* title;
    char* message;
    HalNotifyType type;
    int duration;  // milliseconds
    float progress;  // 0.0 - 1.0
    bool autoClose;
} HalNotificationData;

// Notification window procedure
static LRESULT CALLBACK NotificationWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    HalWidget* notif = (HalWidget*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
    
    switch (msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            
            // Create memory DC for double buffering
            HDC memDC = CreateCompatibleDC(hdc);
            RECT rect;
            GetClientRect(hwnd, &rect);
            HBITMAP memBitmap = CreateCompatibleBitmap(hdc, rect.right, rect.bottom);
            HBITMAP oldBitmap = (HBITMAP)SelectObject(memDC, memBitmap);
            
            // Render notification
            if (notif && notif->data) {
                extern void hal_notification_render(HalWidget* notif, HDC hdc, HalTheme* theme);
                extern HalTheme* hal_get_theme(void);
                
                // Adjust bounds for rendering
                notif->bounds.x = 0;
                notif->bounds.y = 0;
                notif->bounds.width = rect.right;
                notif->bounds.height = rect.bottom;
                
                hal_notification_render(notif, memDC, hal_get_theme());
            }
            
            // Copy to screen
            BitBlt(hdc, 0, 0, rect.right, rect.bottom, memDC, 0, 0, SRCCOPY);
            
            SelectObject(memDC, oldBitmap);
            DeleteObject(memBitmap);
            DeleteDC(memDC);
            EndPaint(hwnd, &ps);
            return 0;
        }
        
        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
            // Close on click
            DestroyWindow(hwnd);
            return 0;
            
        case WM_TIMER:
            // Auto-close timer
            DestroyWindow(hwnd);
            return 0;
            
        case WM_DESTROY:
            if (notif) {
                HalNotificationData* data = (HalNotificationData*)notif->data;
                if (data) {
                    free(data->title);
                    free(data->message);
                    free(data);
                }
                free(notif);
            }
            return 0;
    }
    
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

HalWidget* hal_notification_create(HalWindow* window, const char* title, const char* message, 
                                   HalNotifyType type, int durationMs) {
    // Register notification window class
    static bool classRegistered = false;
    if (!classRegistered) {
        WNDCLASSEXW wc = {0};
        wc.cbSize = sizeof(WNDCLASSEXW);
        wc.lpfnWndProc = NotificationWndProc;
        wc.hInstance = GetModuleHandle(NULL);
        wc.lpszClassName = L"HalNotification";
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = NULL;  // We'll paint everything
        wc.style = CS_HREDRAW | CS_VREDRAW;
        RegisterClassExW(&wc);
        classRegistered = true;
    }
    
    // Create notification data
    HalWidget* notif = (HalWidget*)calloc(1, sizeof(HalWidget));
    notif->type = HAL_WIDGET_DIALOG;
    notif->visible = true;
    notif->enabled = true;
    notif->opacity = 1.0f;
    
    HalNotificationData* data = (HalNotificationData*)calloc(1, sizeof(HalNotificationData));
    data->title = strdup(title);
    data->message = strdup(message);
    data->type = type;
    data->duration = durationMs;
    data->progress = 0.0f;
    data->autoClose = true;
    notif->data = data;
    
    // Calculate position (bottom-right of screen, near system tray)
    RECT workArea;
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &workArea, 0);  // Get work area (excludes taskbar)
    
    int notifWidth = 320;
    int notifHeight = 100;
    int x = workArea.right - notifWidth - 10;  // 10px from right edge
    int y = workArea.bottom - notifHeight - 10;  // 10px from bottom (above taskbar)
    
    notif->bounds.width = notifWidth;
    notif->bounds.height = notifHeight;
    notif->bounds.x = x;
    notif->bounds.y = y;
    
    // Create popup window
    HWND hwnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_LAYERED,  // Topmost, no taskbar, layered for transparency
        L"HalNotification",
        L"",
        WS_POPUP,
        x, y, notifWidth, notifHeight,
        window->hwnd,
        NULL,
        GetModuleHandle(NULL),
        NULL
    );
    
    if (hwnd) {
        // Set transparency
        SetLayeredWindowAttributes(hwnd, 0, 250, LWA_ALPHA);  // 98% opaque
        
        // Store notification data
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)notif);
        
        // Store HWND in widget for later access
        notif->hwnd = hwnd;
        
        // Set auto-close timer if needed
        if (data->autoClose && durationMs > 0) {
            SetTimer(hwnd, 1, durationMs, NULL);
        }
    }
    
    return notif;
}

void hal_notification_show(HalWidget* notif) {
    if (!notif || !notif->hwnd) return;
    
    // Show the popup window with animation
    ShowWindow(notif->hwnd, SW_SHOWNOACTIVATE);
    UpdateWindow(notif->hwnd);
    
    // Animate fade-in (optional - can be enhanced later)
    AnimateWindow(notif->hwnd, 200, AW_BLEND);
}

void hal_notification_close(HalWidget* notif) {
    if (!notif || !notif->hwnd) return;
    
    // Animate fade-out
    AnimateWindow(notif->hwnd, 150, AW_HIDE | AW_BLEND);
    DestroyWindow(notif->hwnd);
}

/* ============================================
   Context Menu Widget
   ============================================ */

typedef struct HalMenuItem {
    char* text;
    char* shortcut;
    bool separator;
    bool enabled;
    bool checked;
    HalEventHandler callback;
    void* userData;
    struct HalMenuItem** children;
    int childCount;
} HalMenuItem;

typedef struct {
    HalMenuItem* items;
    int itemCount;
    int itemCapacity;
    int selectedIndex;
    int itemHeight;
} HalContextMenuData;

HalWidget* hal_contextmenu_create(void) {
    HalWidget* menu = (HalWidget*)calloc(1, sizeof(HalWidget));
    menu->type = HAL_WIDGET_MENU;
    menu->visible = false;
    menu->enabled = true;
    menu->opacity = 1.0f;
    
    HalContextMenuData* data = (HalContextMenuData*)calloc(1, sizeof(HalContextMenuData));
    data->itemHeight = 28;
    data->selectedIndex = -1;
    menu->data = data;
    
    return menu;
}

void hal_contextmenu_add_item(HalWidget* menu, const char* text, HalEventHandler callback, void* userData) {
    if (!menu) return;
    
    HalContextMenuData* data = (HalContextMenuData*)menu->data;
    
    if (data->itemCount >= data->itemCapacity) {
        data->itemCapacity = data->itemCapacity == 0 ? 8 : data->itemCapacity * 2;
        data->items = (HalMenuItem*)realloc(data->items, data->itemCapacity * sizeof(HalMenuItem));
    }
    
    HalMenuItem* item = &data->items[data->itemCount];
    memset(item, 0, sizeof(HalMenuItem));
    item->text = strdup(text);
    item->enabled = true;
    item->callback = callback;
    item->userData = userData;
    data->itemCount++;
}

void hal_contextmenu_add_separator(HalWidget* menu) {
    if (!menu) return;
    
    HalContextMenuData* data = (HalContextMenuData*)menu->data;
    
    if (data->itemCount >= data->itemCapacity) {
        data->itemCapacity = data->itemCapacity == 0 ? 8 : data->itemCapacity * 2;
        data->items = (HalMenuItem*)realloc(data->items, data->itemCapacity * sizeof(HalMenuItem));
    }
    
    HalMenuItem* item = &data->items[data->itemCount];
    memset(item, 0, sizeof(HalMenuItem));
    item->separator = true;
    data->itemCount++;
}

void hal_contextmenu_show(HalWidget* menu, int x, int y) {
    if (!menu) return;
    menu->bounds.x = x;
    menu->bounds.y = y;
    menu->visible = true;
    hal_widget_invalidate(menu);
}

/* ============================================
   Tooltip Widget
   ============================================ */

typedef struct {
    char* text;
    int delay;  // milliseconds before showing
    int duration;  // milliseconds to show (0 = infinite)
    HalWidget* target;
} HalTooltipData;

HalWidget* hal_tooltip_create(HalWidget* target, const char* text) {
    HalWidget* tooltip = (HalWidget*)calloc(1, sizeof(HalWidget));
    tooltip->type = HAL_WIDGET_TOOLTIP;
    tooltip->visible = false;
    tooltip->enabled = true;
    tooltip->opacity = 0.95f;
    
    HalTooltipData* data = (HalTooltipData*)calloc(1, sizeof(HalTooltipData));
    data->text = strdup(text);
    data->delay = 500;  // 500ms delay
    data->duration = 0;  // Show until mouse leaves
    data->target = target;
    tooltip->data = data;
    
    return tooltip;
}

void hal_tooltip_set_text(HalWidget* tooltip, const char* text) {
    if (!tooltip) return;
    HalTooltipData* data = (HalTooltipData*)tooltip->data;
    free(data->text);
    data->text = strdup(text);
}

void hal_tooltip_show(HalWidget* tooltip, int x, int y) {
    if (!tooltip) return;
    tooltip->bounds.x = x + 10;
    tooltip->bounds.y = y + 20;
    tooltip->visible = true;
    hal_widget_invalidate(tooltip);
}

void hal_tooltip_hide(HalWidget* tooltip) {
    if (!tooltip) return;
    tooltip->visible = false;
    hal_widget_invalidate(tooltip);
}

/* ============================================
   Rich Text Editor Widget
   ============================================ */

typedef enum {
    HAL_TEXT_BOLD = 1,
    HAL_TEXT_ITALIC = 2,
    HAL_TEXT_UNDERLINE = 4,
    HAL_TEXT_STRIKETHROUGH = 8
} HalTextStyle;

typedef struct {
    char* text;
    int fontSize;
    HalColor textColor;
    HalColor bgColor;
    uint32_t style;  // Combination of HalTextStyle flags
    bool readonly;
    int cursorPos;
    int selectionStart;
    int selectionEnd;
} HalRichTextData;

HalWidget* hal_richtexteditor_create(HalWidget* parent) {
    HalWidget* editor = (HalWidget*)calloc(1, sizeof(HalWidget));
    editor->type = HAL_WIDGET_TEXTAREA;
    editor->parent = parent;
    editor->visible = true;
    editor->enabled = true;
    editor->opacity = 1.0f;
    
    HalRichTextData* data = (HalRichTextData*)calloc(1, sizeof(HalRichTextData));
    data->text = strdup("");
    data->fontSize = 12;
    data->textColor = HAL_RGB(0, 0, 0);
    data->bgColor = HAL_RGB(255, 255, 255);
    data->style = 0;
    data->readonly = false;
    data->cursorPos = 0;
    data->selectionStart = -1;
    data->selectionEnd = -1;
    editor->data = data;
    
    if (parent) {
        hal_widget_add_child(parent, editor);
    }
    
    return editor;
}

void hal_richtexteditor_set_style(HalWidget* editor, uint32_t style) {
    if (!editor) return;
    HalRichTextData* data = (HalRichTextData*)editor->data;
    data->style = style;
    hal_widget_invalidate(editor);
}

void hal_richtexteditor_set_font_size(HalWidget* editor, int size) {
    if (!editor) return;
    HalRichTextData* data = (HalRichTextData*)editor->data;
    data->fontSize = size;
    hal_widget_invalidate(editor);
}

void hal_richtexteditor_set_text_color(HalWidget* editor, HalColor color) {
    if (!editor) return;
    HalRichTextData* data = (HalRichTextData*)editor->data;
    data->textColor = color;
    hal_widget_invalidate(editor);
}
