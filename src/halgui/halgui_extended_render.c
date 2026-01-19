/*
 * HalGUI Extended Widgets - Rendering Implementation
 * Copyright (C) 2026 KAInaps
 * 
 * Implements actual rendering for extended widgets
 */

#include "halgui.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

/* Full structure definitions from halgui_extended.c */
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

typedef struct {
    char* name;
    int width;
    int align;
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

typedef struct {
    int year;
    int month;
    int day;
    int selectedDay;
    int selectedMonth;
    int selectedYear;
    int firstDayOfWeek;
    bool showWeekNumbers;
    void* onDateChange;
    void* onDateChangeUserData;
} HalCalendarData;

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
    int duration;
    float progress;
    bool autoClose;
} HalNotificationData;

/* ============================================
   TreeView Rendering
   ============================================ */

static void render_tree_node(HDC hdc, HalTreeNode* node, int* y, int indent, HalTheme* theme) {
    if (!node) return;
    
    int x = indent * 20;
    
    // Draw expand/collapse icon if has children
    if (node->childCount > 0) {
        RECT iconRect = {x, *y + 4, x + 16, *y + 20};
        const char* icon = node->expanded ? "▼" : "▶";
        SetTextColor(hdc, RGB(HAL_GET_R(theme->textSecondary), 
                              HAL_GET_G(theme->textSecondary), 
                              HAL_GET_B(theme->textSecondary)));
        DrawTextA(hdc, icon, -1, &iconRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    }
    
    // Draw node text
    RECT textRect = {x + 20, *y, x + 400, *y + 24};
    if (node->selected) {
        HBRUSH brush = CreateSolidBrush(RGB(HAL_GET_R(theme->primary), 
                                            HAL_GET_G(theme->primary), 
                                            HAL_GET_B(theme->primary)));
        FillRect(hdc, &textRect, brush);
        DeleteObject(brush);
        SetTextColor(hdc, RGB(HAL_GET_R(theme->textOnPrimary), 
                              HAL_GET_G(theme->textOnPrimary), 
                              HAL_GET_B(theme->textOnPrimary)));
    } else {
        SetTextColor(hdc, RGB(HAL_GET_R(theme->textPrimary), 
                              HAL_GET_G(theme->textPrimary), 
                              HAL_GET_B(theme->textPrimary)));
    }
    
    DrawTextA(hdc, node->text, -1, &textRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    *y += 24;
    
    // Draw children if expanded
    if (node->expanded) {
        for (int i = 0; i < node->childCount; i++) {
            render_tree_node(hdc, node->children[i], y, indent + 1, theme);
        }
    }
}

void hal_treeview_render(HalWidget* tree, HDC hdc, HalTheme* theme) {
    if (!tree || !tree->data) return;
    
    HalTreeViewData* data = (HalTreeViewData*)tree->data;
    
    // Background
    HBRUSH bgBrush = CreateSolidBrush(RGB(HAL_GET_R(theme->surface), 
                                          HAL_GET_G(theme->surface), 
                                          HAL_GET_B(theme->surface)));
    RECT rect = {tree->bounds.x, tree->bounds.y, 
                 tree->bounds.x + tree->bounds.width, 
                 tree->bounds.y + tree->bounds.height};
    FillRect(hdc, &rect, bgBrush);
    DeleteObject(bgBrush);
    
    // Render tree nodes
    int y = tree->bounds.y + 5;
    if (data->root) {
        for (int i = 0; i < data->root->childCount; i++) {
            render_tree_node(hdc, data->root->children[i], &y, 0, theme);
        }
    }
}

/* ============================================
   DataGrid Rendering
   ============================================ */

void hal_datagrid_render(HalWidget* grid, HDC hdc, HalTheme* theme) {
    if (!grid || !grid->data) return;
    
    HalDataGridData* data = (HalDataGridData*)grid->data;
    
    int x = grid->bounds.x;
    int y = grid->bounds.y;
    
    // Background
    HBRUSH bgBrush = CreateSolidBrush(RGB(HAL_GET_R(theme->surface), 
                                          HAL_GET_G(theme->surface), 
                                          HAL_GET_B(theme->surface)));
    RECT rect = {x, y, x + grid->bounds.width, y + grid->bounds.height};
    FillRect(hdc, &rect, bgBrush);
    DeleteObject(bgBrush);
    
    // Draw header
    HBRUSH headerBrush = CreateSolidBrush(RGB(HAL_GET_R(theme->elevated), 
                                              HAL_GET_G(theme->elevated), 
                                              HAL_GET_B(theme->elevated)));
    RECT headerRect = {x, y, x + grid->bounds.width, y + data->headerHeight};
    FillRect(hdc, &headerRect, headerBrush);
    DeleteObject(headerBrush);
    
    SetTextColor(hdc, RGB(HAL_GET_R(theme->textPrimary), 
                          HAL_GET_G(theme->textPrimary), 
                          HAL_GET_B(theme->textPrimary)));
    SetBkMode(hdc, TRANSPARENT);
    
    int colX = x;
    for (int i = 0; i < data->columnCount; i++) {
        RECT colRect = {colX, y, colX + data->columns[i].width, y + data->headerHeight};
        DrawTextA(hdc, data->columns[i].name, -1, &colRect, 
                 DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        
        // Draw column separator
        if (data->showGrid) {
            HPEN pen = CreatePen(PS_SOLID, 1, RGB(HAL_GET_R(theme->border), 
                                                   HAL_GET_G(theme->border), 
                                                   HAL_GET_B(theme->border)));
            HPEN oldPen = (HPEN)SelectObject(hdc, pen);
            MoveToEx(hdc, colX + data->columns[i].width, y, NULL);
            LineTo(hdc, colX + data->columns[i].width, y + data->headerHeight);
            SelectObject(hdc, oldPen);
            DeleteObject(pen);
        }
        
        colX += data->columns[i].width;
    }
    
    // Draw rows
    y += data->headerHeight;
    for (int row = 0; row < data->rowCount; row++) {
        // Alternate row background
        if (data->alternateRows && row % 2 == 1) {
            HBRUSH altBrush = CreateSolidBrush(RGB(HAL_GET_R(theme->background), 
                                                    HAL_GET_G(theme->background), 
                                                    HAL_GET_B(theme->background)));
            RECT rowRect = {x, y, x + grid->bounds.width, y + data->rowHeight};
            FillRect(hdc, &rowRect, altBrush);
            DeleteObject(altBrush);
        }
        
        // Selected row
        if (data->rows[row].selected) {
            HBRUSH selBrush = CreateSolidBrush(RGB(HAL_GET_R(theme->primaryHover), 
                                                    HAL_GET_G(theme->primaryHover), 
                                                    HAL_GET_B(theme->primaryHover)));
            RECT rowRect = {x, y, x + grid->bounds.width, y + data->rowHeight};
            FillRect(hdc, &rowRect, selBrush);
            DeleteObject(selBrush);
        }
        
        // Draw cells
        colX = x;
        for (int col = 0; col < data->columnCount; col++) {
            RECT cellRect = {colX + 5, y, colX + data->columns[col].width - 5, y + data->rowHeight};
            DrawTextA(hdc, data->rows[row].cells[col], -1, &cellRect, 
                     DT_LEFT | DT_VCENTER | DT_SINGLELINE);
            
            // Draw grid lines
            if (data->showGrid) {
                HPEN pen = CreatePen(PS_SOLID, 1, RGB(HAL_GET_R(theme->border), 
                                                       HAL_GET_G(theme->border), 
                                                       HAL_GET_B(theme->border)));
                HPEN oldPen = (HPEN)SelectObject(hdc, pen);
                MoveToEx(hdc, colX + data->columns[col].width, y, NULL);
                LineTo(hdc, colX + data->columns[col].width, y + data->rowHeight);
                SelectObject(hdc, oldPen);
                DeleteObject(pen);
            }
            
            colX += data->columns[col].width;
        }
        
        // Draw horizontal grid line
        if (data->showGrid) {
            HPEN pen = CreatePen(PS_SOLID, 1, RGB(HAL_GET_R(theme->border), 
                                                   HAL_GET_G(theme->border), 
                                                   HAL_GET_B(theme->border)));
            HPEN oldPen = (HPEN)SelectObject(hdc, pen);
            MoveToEx(hdc, x, y + data->rowHeight, NULL);
            LineTo(hdc, x + grid->bounds.width, y + data->rowHeight);
            SelectObject(hdc, oldPen);
            DeleteObject(pen);
        }
        
        y += data->rowHeight;
    }
}

/* ============================================
   Chart Rendering
   ============================================ */

void hal_chart_render(HalWidget* chart, HDC hdc, HalTheme* theme) {
    if (!chart || !chart->data) return;
    
    HalChartData* data = (HalChartData*)chart->data;
    
    // Background
    HBRUSH bgBrush = CreateSolidBrush(RGB(HAL_GET_R(theme->surface), 
                                          HAL_GET_G(theme->surface), 
                                          HAL_GET_B(theme->surface)));
    RECT rect = {chart->bounds.x, chart->bounds.y, 
                 chart->bounds.x + chart->bounds.width, 
                 chart->bounds.y + chart->bounds.height};
    FillRect(hdc, &rect, bgBrush);
    DeleteObject(bgBrush);
    
    // Draw title
    if (data->title) {
        SetTextColor(hdc, RGB(HAL_GET_R(theme->textPrimary), 
                              HAL_GET_G(theme->textPrimary), 
                              HAL_GET_B(theme->textPrimary)));
        SetBkMode(hdc, TRANSPARENT);
        RECT titleRect = {chart->bounds.x, chart->bounds.y + 10, 
                         chart->bounds.x + chart->bounds.width, chart->bounds.y + 40};
        DrawTextA(hdc, data->title, -1, &titleRect, DT_CENTER | DT_SINGLELINE);
    }
    
    // Chart area
    int chartX = chart->bounds.x + 50;
    int chartY = chart->bounds.y + 60;
    int chartW = chart->bounds.width - 100;
    int chartH = chart->bounds.height - 120;
    
    // Draw grid
    if (data->showGrid) {
        HPEN gridPen = CreatePen(PS_DOT, 1, RGB(HAL_GET_R(theme->border), 
                                                HAL_GET_G(theme->border), 
                                                HAL_GET_B(theme->border)));
        HPEN oldPen = (HPEN)SelectObject(hdc, gridPen);
        
        for (int i = 0; i <= 5; i++) {
            int y = chartY + (chartH * i / 5);
            MoveToEx(hdc, chartX, y, NULL);
            LineTo(hdc, chartX + chartW, y);
        }
        
        SelectObject(hdc, oldPen);
        DeleteObject(gridPen);
    }
    
    // Draw axes
    HPEN axisPen = CreatePen(PS_SOLID, 2, RGB(HAL_GET_R(theme->textPrimary), 
                                              HAL_GET_G(theme->textPrimary), 
                                              HAL_GET_B(theme->textPrimary)));
    HPEN oldPen = (HPEN)SelectObject(hdc, axisPen);
    
    MoveToEx(hdc, chartX, chartY, NULL);
    LineTo(hdc, chartX, chartY + chartH);
    LineTo(hdc, chartX + chartW, chartY + chartH);
    
    SelectObject(hdc, oldPen);
    DeleteObject(axisPen);
    
    // Draw series
    if (data->type == HAL_CHART_LINE) {
        for (int s = 0; s < data->seriesCount; s++) {
            HalChartSeries* series = &data->series[s];
            if (series->valueCount < 2) continue;
            
            HPEN seriesPen = CreatePen(PS_SOLID, 2, RGB(HAL_GET_R(series->color), 
                                                        HAL_GET_G(series->color), 
                                                        HAL_GET_B(series->color)));
            oldPen = (HPEN)SelectObject(hdc, seriesPen);
            
            float xStep = (float)chartW / (series->valueCount - 1);
            float yScale = chartH / (data->maxValue - data->minValue);
            
            for (int i = 0; i < series->valueCount - 1; i++) {
                int x1 = chartX + (int)(i * xStep);
                int y1 = chartY + chartH - (int)((series->values[i] - data->minValue) * yScale);
                int x2 = chartX + (int)((i + 1) * xStep);
                int y2 = chartY + chartH - (int)((series->values[i + 1] - data->minValue) * yScale);
                
                MoveToEx(hdc, x1, y1, NULL);
                LineTo(hdc, x2, y2);
            }
            
            SelectObject(hdc, oldPen);
            DeleteObject(seriesPen);
        }
    } else if (data->type == HAL_CHART_BAR) {
        for (int s = 0; s < data->seriesCount; s++) {
            HalChartSeries* series = &data->series[s];
            
            HBRUSH barBrush = CreateSolidBrush(RGB(HAL_GET_R(series->color), 
                                                   HAL_GET_G(series->color), 
                                                   HAL_GET_B(series->color)));
            
            float barWidth = (float)chartW / (series->valueCount * data->seriesCount + series->valueCount);
            float yScale = chartH / (data->maxValue - data->minValue);
            
            for (int i = 0; i < series->valueCount; i++) {
                int x = chartX + (int)((i * (data->seriesCount + 1) + s) * barWidth);
                int h = (int)((series->values[i] - data->minValue) * yScale);
                int y = chartY + chartH - h;
                
                RECT barRect = {x, y, x + (int)barWidth, chartY + chartH};
                FillRect(hdc, &barRect, barBrush);
            }
            
            DeleteObject(barBrush);
        }
    }
    
    // Draw legend
    if (data->showLegend && data->seriesCount > 0) {
        int legendX = chart->bounds.x + chart->bounds.width - 150;
        int legendY = chart->bounds.y + 60;
        
        for (int s = 0; s < data->seriesCount; s++) {
            // Color box
            HBRUSH legendBrush = CreateSolidBrush(RGB(HAL_GET_R(data->series[s].color), 
                                                      HAL_GET_G(data->series[s].color), 
                                                      HAL_GET_B(data->series[s].color)));
            RECT colorRect = {legendX, legendY + s * 25, legendX + 15, legendY + s * 25 + 15};
            FillRect(hdc, &colorRect, legendBrush);
            DeleteObject(legendBrush);
            
            // Label
            RECT labelRect = {legendX + 20, legendY + s * 25, legendX + 140, legendY + s * 25 + 20};
            DrawTextA(hdc, data->series[s].label, -1, &labelRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        }
    }
}

/* ============================================
   Calendar Rendering
   ============================================ */

static const char* monthNames[] = {
    "January", "February", "March", "April", "May", "June",
    "July", "August", "September", "October", "November", "December"
};

static const char* dayNames[] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};

// Helper: Get first day of month (0=Sunday, 1=Monday, etc.)
static int getFirstDayOfMonth_render(int year, int month) {
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
static int getDaysInMonth_render(int year, int month) {
    int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (month == 2 && ((year % 4 == 0 && year % 100 != 0) || year % 400 == 0)) {
        return 29;
    }
    return days[month - 1];
}

void hal_calendar_render(HalWidget* cal, HDC hdc, HalTheme* theme) {
    if (!cal || !cal->data) return;
    
    HalCalendarData* data = (HalCalendarData*)cal->data;
    
    // Background
    HBRUSH bgBrush = CreateSolidBrush(RGB(HAL_GET_R(theme->surface), 
                                          HAL_GET_G(theme->surface), 
                                          HAL_GET_B(theme->surface)));
    RECT rect = {cal->bounds.x, cal->bounds.y, 
                 cal->bounds.x + cal->bounds.width, 
                 cal->bounds.y + cal->bounds.height};
    FillRect(hdc, &rect, bgBrush);
    DeleteObject(bgBrush);
    
    SetTextColor(hdc, RGB(HAL_GET_R(theme->textPrimary), 
                          HAL_GET_G(theme->textPrimary), 
                          HAL_GET_B(theme->textPrimary)));
    SetBkMode(hdc, TRANSPARENT);
    
    // Header with month/year and navigation buttons
    char header[64];
    sprintf(header, "%s %d", monthNames[data->month - 1], data->year);
    
    // Draw navigation buttons
    HBRUSH btnBrush = CreateSolidBrush(RGB(HAL_GET_R(theme->primary), 
                                            HAL_GET_G(theme->primary), 
                                            HAL_GET_B(theme->primary)));
    
    // Previous month button
    RECT prevBtn = {cal->bounds.x + 10, cal->bounds.y + 10, 
                    cal->bounds.x + 40, cal->bounds.y + 40};
    FillRect(hdc, &prevBtn, btnBrush);
    SetTextColor(hdc, RGB(255, 255, 255));
    DrawTextA(hdc, "<", -1, &prevBtn, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    
    // Next month button
    RECT nextBtn = {cal->bounds.x + cal->bounds.width - 40, cal->bounds.y + 10, 
                    cal->bounds.x + cal->bounds.width - 10, cal->bounds.y + 40};
    FillRect(hdc, &nextBtn, btnBrush);
    DrawTextA(hdc, ">", -1, &nextBtn, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    
    DeleteObject(btnBrush);
    
    // Month/Year title
    SetTextColor(hdc, RGB(HAL_GET_R(theme->textPrimary), 
                          HAL_GET_G(theme->textPrimary), 
                          HAL_GET_B(theme->textPrimary)));
    RECT headerRect = {cal->bounds.x + 50, cal->bounds.y + 10, 
                      cal->bounds.x + cal->bounds.width - 50, cal->bounds.y + 40};
    DrawTextA(hdc, header, -1, &headerRect, DT_CENTER | DT_SINGLELINE);
    
    // Day names
    int cellW = cal->bounds.width / 7;
    int cellH = 30;
    int y = cal->bounds.y + 50;
    
    // Draw day names background
    HBRUSH dayNamesBrush = CreateSolidBrush(RGB(HAL_GET_R(theme->elevated), 
                                                 HAL_GET_G(theme->elevated), 
                                                 HAL_GET_B(theme->elevated)));
    RECT dayNamesRect = {cal->bounds.x, y, cal->bounds.x + cal->bounds.width, y + cellH};
    FillRect(hdc, &dayNamesRect, dayNamesBrush);
    DeleteObject(dayNamesBrush);
    
    for (int i = 0; i < 7; i++) {
        RECT dayRect = {cal->bounds.x + i * cellW, y, 
                       cal->bounds.x + (i + 1) * cellW, y + cellH};
        DrawTextA(hdc, dayNames[i], -1, &dayRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }
    
    // Days grid
    y += cellH;
    int daysInMonth = getDaysInMonth_render(data->year, data->month);
    int firstDay = getFirstDayOfMonth_render(data->year, data->month);
    
    // Adjust for Monday start
    if (data->firstDayOfWeek == 1) {
        firstDay = (firstDay + 6) % 7;
    }
    
    // Get current date for highlighting
    SYSTEMTIME st;
    GetLocalTime(&st);
    bool isCurrentMonth = (st.wYear == data->year && st.wMonth == data->month);
    
    int day = 1;
    for (int week = 0; week < 6 && day <= daysInMonth; week++) {
        for (int dow = 0; dow < 7; dow++) {
            RECT dayRect = {cal->bounds.x + dow * cellW, y, 
                           cal->bounds.x + (dow + 1) * cellW, y + cellH};
            
            // Check if this cell should have a day
            int dayIndex = week * 7 + dow;
            if (dayIndex >= firstDay && day <= daysInMonth) {
                char dayStr[12];  // Increased buffer size to avoid truncation warning
                snprintf(dayStr, sizeof(dayStr), "%d", day);
                
                // Highlight selected day
                if (day == data->selectedDay && 
                    data->month == data->selectedMonth && 
                    data->year == data->selectedYear) {
                    HBRUSH selBrush = CreateSolidBrush(RGB(HAL_GET_R(theme->primary), 
                                                           HAL_GET_G(theme->primary), 
                                                           HAL_GET_B(theme->primary)));
                    FillRect(hdc, &dayRect, selBrush);
                    DeleteObject(selBrush);
                    SetTextColor(hdc, RGB(255, 255, 255));
                }
                // Highlight today
                else if (isCurrentMonth && day == st.wDay) {
                    HBRUSH todayBrush = CreateSolidBrush(RGB(HAL_GET_R(theme->primaryHover), 
                                                             HAL_GET_G(theme->primaryHover), 
                                                             HAL_GET_B(theme->primaryHover)));
                    FillRect(hdc, &dayRect, todayBrush);
                    DeleteObject(todayBrush);
                    SetTextColor(hdc, RGB(255, 255, 255));
                }
                else {
                    SetTextColor(hdc, RGB(HAL_GET_R(theme->textPrimary), 
                                          HAL_GET_G(theme->textPrimary), 
                                          HAL_GET_B(theme->textPrimary)));
                }
                
                DrawTextA(hdc, dayStr, -1, &dayRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
                day++;
            }
        }
        y += cellH;
    }
    
    // Draw grid lines
    HPEN gridPen = CreatePen(PS_SOLID, 1, RGB(HAL_GET_R(theme->border), 
                                              HAL_GET_G(theme->border), 
                                              HAL_GET_B(theme->border)));
    HPEN oldPen = (HPEN)SelectObject(hdc, gridPen);
    
    // Vertical lines
    for (int i = 0; i <= 7; i++) {
        int x = cal->bounds.x + i * cellW;
        MoveToEx(hdc, x, cal->bounds.y + 50, NULL);
        LineTo(hdc, x, cal->bounds.y + 50 + 7 * cellH);
    }
    
    // Horizontal lines
    for (int i = 0; i <= 7; i++) {
        int lineY = cal->bounds.y + 50 + i * cellH;
        MoveToEx(hdc, cal->bounds.x, lineY, NULL);
        LineTo(hdc, cal->bounds.x + cal->bounds.width, lineY);
    }
    
    SelectObject(hdc, oldPen);
    DeleteObject(gridPen);
}

/* ============================================
   Notification Rendering
   ============================================ */

void hal_notification_render(HalWidget* notif, HDC hdc, HalTheme* theme) {
    if (!notif || !notif->data) return;
    
    HalNotificationData* data = (HalNotificationData*)notif->data;
    
    // Background color based on type
    HalColor bgColor;
    HalColor borderColor;
    switch (data->type) {
        case HAL_NOTIFY_SUCCESS: 
            bgColor = HAL_RGB(34, 197, 94);  // Green
            borderColor = HAL_RGB(22, 163, 74);
            break;
        case HAL_NOTIFY_WARNING: 
            bgColor = HAL_RGB(251, 191, 36);  // Yellow
            borderColor = HAL_RGB(245, 158, 11);
            break;
        case HAL_NOTIFY_ERROR: 
            bgColor = HAL_RGB(239, 68, 68);  // Red
            borderColor = HAL_RGB(220, 38, 38);
            break;
        default: 
            bgColor = HAL_RGB(59, 130, 246);  // Blue
            borderColor = HAL_RGB(37, 99, 235);
            break;
    }
    
    // Draw background
    HBRUSH bgBrush = CreateSolidBrush(RGB(HAL_GET_R(bgColor), 
                                          HAL_GET_G(bgColor), 
                                          HAL_GET_B(bgColor)));
    RECT rect = {notif->bounds.x, notif->bounds.y, 
                 notif->bounds.x + notif->bounds.width, 
                 notif->bounds.y + notif->bounds.height};
    FillRect(hdc, &rect, bgBrush);
    DeleteObject(bgBrush);
    
    // Draw border
    HPEN borderPen = CreatePen(PS_SOLID, 2, RGB(HAL_GET_R(borderColor), 
                                                HAL_GET_G(borderColor), 
                                                HAL_GET_B(borderColor)));
    HPEN oldPen = (HPEN)SelectObject(hdc, borderPen);
    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
    Rectangle(hdc, rect.left, rect.top, rect.right, rect.bottom);
    SelectObject(hdc, oldPen);
    SelectObject(hdc, oldBrush);
    DeleteObject(borderPen);
    
    // Setup text rendering
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(255, 255, 255));  // White text
    
    // Draw title (bold)
    HFONT boldFont = CreateFontA(16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, 
                                 DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, 
                                 CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
    HFONT oldFont = (HFONT)SelectObject(hdc, boldFont);
    
    RECT titleRect = {notif->bounds.x + 15, notif->bounds.y + 10, 
                     notif->bounds.x + notif->bounds.width - 15, notif->bounds.y + 30};
    DrawTextA(hdc, data->title, -1, &titleRect, DT_LEFT | DT_SINGLELINE | DT_VCENTER);
    
    SelectObject(hdc, oldFont);
    DeleteObject(boldFont);
    
    // Draw message (normal)
    HFONT normalFont = CreateFontA(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, 
                                   DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, 
                                   CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
    oldFont = (HFONT)SelectObject(hdc, normalFont);
    
    RECT msgRect = {notif->bounds.x + 15, notif->bounds.y + 35, 
                   notif->bounds.x + notif->bounds.width - 15, notif->bounds.y + notif->bounds.height - 10};
    DrawTextA(hdc, data->message, -1, &msgRect, DT_LEFT | DT_WORDBREAK);
    
    SelectObject(hdc, oldFont);
    DeleteObject(normalFont);
}
