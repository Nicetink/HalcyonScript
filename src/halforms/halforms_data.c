/*
 * HalForms Data Controls
 * DataGridView, Chart, and data binding support
 */

#include "halforms.h"
#include <commctrl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* ============================================
   DataGridView - Enhanced ListView with editing
   ============================================ */

typedef struct {
    HalControl base;
    int columnCount;
    int rowCount;
    bool editable;
    bool showGridLines;
    bool fullRowSelect;
    char** columnNames;
} HalDataGrid;

HalDataGrid* haldatagrid_create(HalForm* parent, int x, int y, int w, int h) {
    HalDataGrid* grid = (HalDataGrid*)calloc(1, sizeof(HalDataGrid));
    if (!grid) return NULL;
    
    grid->base.hwnd = CreateWindowExW(WS_EX_CLIENTEDGE, WC_LISTVIEWW, L"",
        WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SHOWSELALWAYS,
        x, y, w, h, parent->base.hwnd, NULL, GetModuleHandle(NULL), NULL);
    
    /* Enable extended styles */
    ListView_SetExtendedListViewStyle(grid->base.hwnd, 
        LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);
    
    grid->base.x = x;
    grid->base.y = y;
    grid->base.width = w;
    grid->base.height = h;
    grid->base.visible = true;
    grid->base.enabled = true;
    grid->showGridLines = true;
    grid->fullRowSelect = true;
    
    return grid;
}

void haldatagrid_add_column(HalDataGrid* grid, const char* name, int width, int align) {
    if (!grid) return;
    
    LVCOLUMNW col = {0};
    col.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_FMT;
    col.cx = width;
    
    switch (align) {
        case 1: col.fmt = LVCFMT_CENTER; break;
        case 2: col.fmt = LVCFMT_RIGHT; break;
        default: col.fmt = LVCFMT_LEFT; break;
    }
    
    wchar_t wname[256];
    MultiByteToWideChar(CP_UTF8, 0, name, -1, wname, 256);
    col.pszText = wname;
    
    ListView_InsertColumn(grid->base.hwnd, grid->columnCount, &col);
    grid->columnCount++;
}

int haldatagrid_add_row(HalDataGrid* grid, const char** values, int valueCount) {
    if (!grid || !values) return -1;
    
    LVITEMW item = {0};
    item.mask = LVIF_TEXT;
    item.iItem = grid->rowCount;
    
    wchar_t wtext[1024];
    MultiByteToWideChar(CP_UTF8, 0, values[0], -1, wtext, 1024);
    item.pszText = wtext;
    
    int idx = ListView_InsertItem(grid->base.hwnd, &item);
    
    /* Set sub-items */
    for (int i = 1; i < valueCount && i < grid->columnCount; i++) {
        wchar_t wsubtext[1024];
        MultiByteToWideChar(CP_UTF8, 0, values[i], -1, wsubtext, 1024);
        LVITEMW subitem = {0};
        subitem.mask = LVIF_TEXT;
        subitem.iItem = idx;
        subitem.iSubItem = i;
        subitem.pszText = wsubtext;
        SendMessageW(grid->base.hwnd, LVM_SETITEMTEXTW, idx, (LPARAM)&subitem);
    }
    
    grid->rowCount++;
    return idx;
}

void haldatagrid_set_cell(HalDataGrid* grid, int row, int col, const char* value) {
    if (!grid || !value) return;
    wchar_t wtext[1024];
    MultiByteToWideChar(CP_UTF8, 0, value, -1, wtext, 1024);
    LVITEMW item = {0};
    item.mask = LVIF_TEXT;
    item.iItem = row;
    item.iSubItem = col;
    item.pszText = wtext;
    SendMessageW(grid->base.hwnd, LVM_SETITEMTEXTW, row, (LPARAM)&item);
}

char* haldatagrid_get_cell(HalDataGrid* grid, int row, int col) {
    if (!grid) return NULL;
    wchar_t wtext[1024] = {0};
    LVITEMW item = {0};
    item.mask = LVIF_TEXT;
    item.iItem = row;
    item.iSubItem = col;
    item.pszText = wtext;
    item.cchTextMax = 1024;
    SendMessageW(grid->base.hwnd, LVM_GETITEMTEXTW, row, (LPARAM)&item);
    
    int len = WideCharToMultiByte(CP_UTF8, 0, wtext, -1, NULL, 0, NULL, NULL);
    char* result = (char*)malloc(len);
    WideCharToMultiByte(CP_UTF8, 0, wtext, -1, result, len, NULL, NULL);
    return result;
}

void haldatagrid_delete_row(HalDataGrid* grid, int row) {
    if (!grid) return;
    ListView_DeleteItem(grid->base.hwnd, row);
    grid->rowCount--;
}

void haldatagrid_clear(HalDataGrid* grid) {
    if (!grid) return;
    ListView_DeleteAllItems(grid->base.hwnd);
    grid->rowCount = 0;
}

int haldatagrid_get_selected_row(HalDataGrid* grid) {
    if (!grid) return -1;
    return ListView_GetNextItem(grid->base.hwnd, -1, LVNI_SELECTED);
}

void haldatagrid_select_row(HalDataGrid* grid, int row) {
    if (!grid) return;
    ListView_SetItemState(grid->base.hwnd, row, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
    ListView_EnsureVisible(grid->base.hwnd, row, FALSE);
}

void haldatagrid_set_row_color(HalDataGrid* grid, int row, COLORREF textColor, COLORREF bgColor) {
    /* Custom draw would be needed for this - simplified version */
    (void)grid; (void)row; (void)textColor; (void)bgColor;
}

void haldatagrid_auto_size_columns(HalDataGrid* grid) {
    if (!grid) return;
    for (int i = 0; i < grid->columnCount; i++) {
        ListView_SetColumnWidth(grid->base.hwnd, i, LVSCW_AUTOSIZE_USEHEADER);
    }
}

void haldatagrid_sort_column(HalDataGrid* grid, int col, bool ascending) {
    /* Would need custom compare function - placeholder */
    (void)grid; (void)col; (void)ascending;
}

/* ============================================
   Simple Chart Control
   ============================================ */

typedef enum {
    CHART_BAR,
    CHART_LINE,
    CHART_PIE,
    CHART_AREA
} HalChartType;

typedef struct {
    char* label;
    double value;
    COLORREF color;
} HalChartDataPoint;

typedef struct {
    HalControl base;
    HalChartType type;
    HalChartDataPoint* data;
    int dataCount;
    int dataCapacity;
    char* title;
    bool showLegend;
    bool showValues;
    COLORREF backgroundColor;
    COLORREF gridColor;
} HalChart;

static LRESULT CALLBACK ChartWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

HalChart* halchart_create(HalForm* parent, int x, int y, int w, int h, HalChartType type) {
    static bool classRegistered = false;
    if (!classRegistered) {
        WNDCLASSEXW wc = {sizeof(WNDCLASSEXW)};
        wc.lpfnWndProc = ChartWndProc;
        wc.hInstance = GetModuleHandle(NULL);
        wc.lpszClassName = L"HalFormsChart";
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        RegisterClassExW(&wc);
        classRegistered = true;
    }
    
    HalChart* chart = (HalChart*)calloc(1, sizeof(HalChart));
    if (!chart) return NULL;
    
    chart->base.hwnd = CreateWindowExW(WS_EX_CLIENTEDGE, L"HalFormsChart", L"",
        WS_CHILD | WS_VISIBLE,
        x, y, w, h, parent->base.hwnd, NULL, GetModuleHandle(NULL), NULL);
    
    SetWindowLongPtr(chart->base.hwnd, GWLP_USERDATA, (LONG_PTR)chart);
    
    chart->base.x = x;
    chart->base.y = y;
    chart->base.width = w;
    chart->base.height = h;
    chart->base.visible = true;
    chart->base.enabled = true;
    chart->type = type;
    chart->backgroundColor = RGB(255, 255, 255);
    chart->gridColor = RGB(220, 220, 220);
    chart->showLegend = true;
    chart->showValues = true;
    
    return chart;
}

void halchart_add_data(HalChart* chart, const char* label, double value, COLORREF color) {
    if (!chart) return;
    
    if (chart->dataCount >= chart->dataCapacity) {
        int newCap = chart->dataCapacity == 0 ? 16 : chart->dataCapacity * 2;
        chart->data = (HalChartDataPoint*)realloc(chart->data, sizeof(HalChartDataPoint) * newCap);
        chart->dataCapacity = newCap;
    }
    
    chart->data[chart->dataCount].label = _strdup(label);
    chart->data[chart->dataCount].value = value;
    chart->data[chart->dataCount].color = color;
    chart->dataCount++;
    
    InvalidateRect(chart->base.hwnd, NULL, TRUE);
}

void halchart_clear_data(HalChart* chart) {
    if (!chart) return;
    for (int i = 0; i < chart->dataCount; i++) {
        free(chart->data[i].label);
    }
    chart->dataCount = 0;
    InvalidateRect(chart->base.hwnd, NULL, TRUE);
}

void halchart_set_title(HalChart* chart, const char* title) {
    if (!chart) return;
    free(chart->title);
    chart->title = title ? _strdup(title) : NULL;
    InvalidateRect(chart->base.hwnd, NULL, TRUE);
}

static void DrawBarChart(HDC hdc, HalChart* chart, RECT* rc) {
    if (chart->dataCount == 0) return;
    
    int margin = 40;
    int chartX = rc->left + margin;
    int chartY = rc->top + margin;
    int chartW = rc->right - rc->left - margin * 2;
    int chartH = rc->bottom - rc->top - margin * 2;
    
    /* Find max value */
    double maxVal = 0;
    for (int i = 0; i < chart->dataCount; i++) {
        if (chart->data[i].value > maxVal) maxVal = chart->data[i].value;
    }
    if (maxVal == 0) maxVal = 1;
    
    /* Draw bars */
    int barWidth = chartW / chart->dataCount - 10;
    for (int i = 0; i < chart->dataCount; i++) {
        int barH = (int)(chartH * chart->data[i].value / maxVal);
        int barX = chartX + i * (barWidth + 10) + 5;
        int barY = chartY + chartH - barH;
        
        HBRUSH brush = CreateSolidBrush(chart->data[i].color);
        RECT barRect = {barX, barY, barX + barWidth, chartY + chartH};
        FillRect(hdc, &barRect, brush);
        DeleteObject(brush);
        
        /* Draw label */
        if (chart->data[i].label) {
            wchar_t wlabel[64];
            MultiByteToWideChar(CP_UTF8, 0, chart->data[i].label, -1, wlabel, 64);
            SetTextAlign(hdc, TA_CENTER);
            TextOutW(hdc, barX + barWidth / 2, chartY + chartH + 5, wlabel, (int)wcslen(wlabel));
        }
        
        /* Draw value */
        if (chart->showValues) {
            wchar_t wval[32];
            swprintf(wval, 32, L"%.1f", chart->data[i].value);
            TextOutW(hdc, barX + barWidth / 2, barY - 15, wval, (int)wcslen(wval));
        }
    }
}

static void DrawPieChart(HDC hdc, HalChart* chart, RECT* rc) {
    if (chart->dataCount == 0) return;
    
    int cx = (rc->left + rc->right) / 2;
    int cy = (rc->top + rc->bottom) / 2;
    int radius = min(rc->right - rc->left, rc->bottom - rc->top) / 2 - 40;
    
    /* Calculate total */
    double total = 0;
    for (int i = 0; i < chart->dataCount; i++) {
        total += chart->data[i].value;
    }
    if (total == 0) return;
    
    /* Draw pie slices */
    double startAngle = 0;
    for (int i = 0; i < chart->dataCount; i++) {
        double sweepAngle = 360.0 * chart->data[i].value / total;
        
        HBRUSH brush = CreateSolidBrush(chart->data[i].color);
        HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, brush);
        HPEN pen = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
        HPEN oldPen = (HPEN)SelectObject(hdc, pen);
        
        double startRad = startAngle * 3.14159265 / 180.0;
        double endRad = (startAngle + sweepAngle) * 3.14159265 / 180.0;
        
        int x1 = cx + (int)(radius * cos(startRad));
        int y1 = cy - (int)(radius * sin(startRad));
        int x2 = cx + (int)(radius * cos(endRad));
        int y2 = cy - (int)(radius * sin(endRad));
        
        Pie(hdc, cx - radius, cy - radius, cx + radius, cy + radius, x1, y1, x2, y2);
        
        SelectObject(hdc, oldBrush);
        SelectObject(hdc, oldPen);
        DeleteObject(brush);
        DeleteObject(pen);
        
        startAngle += sweepAngle;
    }
}

static void DrawLineChart(HDC hdc, HalChart* chart, RECT* rc) {
    if (chart->dataCount < 2) return;
    
    int margin = 40;
    int chartX = rc->left + margin;
    int chartY = rc->top + margin;
    int chartW = rc->right - rc->left - margin * 2;
    int chartH = rc->bottom - rc->top - margin * 2;
    
    /* Find max value */
    double maxVal = 0;
    for (int i = 0; i < chart->dataCount; i++) {
        if (chart->data[i].value > maxVal) maxVal = chart->data[i].value;
    }
    if (maxVal == 0) maxVal = 1;
    
    /* Draw grid lines */
    HPEN gridPen = CreatePen(PS_DOT, 1, chart->gridColor);
    HPEN oldPen = (HPEN)SelectObject(hdc, gridPen);
    for (int i = 0; i <= 5; i++) {
        int y = chartY + chartH * i / 5;
        MoveToEx(hdc, chartX, y, NULL);
        LineTo(hdc, chartX + chartW, y);
    }
    SelectObject(hdc, oldPen);
    DeleteObject(gridPen);
    
    /* Draw line */
    HPEN linePen = CreatePen(PS_SOLID, 2, chart->data[0].color);
    oldPen = (HPEN)SelectObject(hdc, linePen);
    
    int stepX = chartW / (chart->dataCount - 1);
    for (int i = 0; i < chart->dataCount; i++) {
        int x = chartX + i * stepX;
        int y = chartY + chartH - (int)(chartH * chart->data[i].value / maxVal);
        
        if (i == 0) {
            MoveToEx(hdc, x, y, NULL);
        } else {
            LineTo(hdc, x, y);
        }
        
        /* Draw point */
        Ellipse(hdc, x - 4, y - 4, x + 4, y + 4);
    }
    
    SelectObject(hdc, oldPen);
    DeleteObject(linePen);
}

static LRESULT CALLBACK ChartWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    HalChart* chart = (HalChart*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    
    switch (msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            
            RECT rc;
            GetClientRect(hwnd, &rc);
            
            /* Fill background */
            HBRUSH bgBrush = CreateSolidBrush(chart ? chart->backgroundColor : RGB(255, 255, 255));
            FillRect(hdc, &rc, bgBrush);
            DeleteObject(bgBrush);
            
            if (chart) {
                /* Draw title */
                if (chart->title) {
                    wchar_t wtitle[256];
                    MultiByteToWideChar(CP_UTF8, 0, chart->title, -1, wtitle, 256);
                    HFONT font = CreateFontW(18, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                        CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
                    HFONT oldFont = (HFONT)SelectObject(hdc, font);
                    SetTextAlign(hdc, TA_CENTER);
                    TextOutW(hdc, (rc.left + rc.right) / 2, 10, wtitle, (int)wcslen(wtitle));
                    SelectObject(hdc, oldFont);
                    DeleteObject(font);
                }
                
                /* Draw chart based on type */
                SetBkMode(hdc, TRANSPARENT);
                switch (chart->type) {
                    case CHART_BAR: DrawBarChart(hdc, chart, &rc); break;
                    case CHART_PIE: DrawPieChart(hdc, chart, &rc); break;
                    case CHART_LINE: DrawLineChart(hdc, chart, &rc); break;
                    default: break;
                }
            }
            
            EndPaint(hwnd, &ps);
            return 0;
        }
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

void halchart_refresh(HalChart* chart) {
    if (!chart) return;
    InvalidateRect(chart->base.hwnd, NULL, TRUE);
}

/* ============================================
   JSON Data Binding Helper
   ============================================ */

typedef struct {
    char* key;
    char* value;
} HalKeyValue;

typedef struct {
    HalKeyValue* items;
    int count;
    int capacity;
} HalDataObject;

HalDataObject* haldata_create(void) {
    HalDataObject* obj = (HalDataObject*)calloc(1, sizeof(HalDataObject));
    return obj;
}

void haldata_set(HalDataObject* obj, const char* key, const char* value) {
    if (!obj || !key) return;
    
    /* Check if key exists */
    for (int i = 0; i < obj->count; i++) {
        if (strcmp(obj->items[i].key, key) == 0) {
            free(obj->items[i].value);
            obj->items[i].value = value ? _strdup(value) : NULL;
            return;
        }
    }
    
    /* Add new key */
    if (obj->count >= obj->capacity) {
        int newCap = obj->capacity == 0 ? 16 : obj->capacity * 2;
        obj->items = (HalKeyValue*)realloc(obj->items, sizeof(HalKeyValue) * newCap);
        obj->capacity = newCap;
    }
    
    obj->items[obj->count].key = _strdup(key);
    obj->items[obj->count].value = value ? _strdup(value) : NULL;
    obj->count++;
}

const char* haldata_get(HalDataObject* obj, const char* key) {
    if (!obj || !key) return NULL;
    for (int i = 0; i < obj->count; i++) {
        if (strcmp(obj->items[i].key, key) == 0) {
            return obj->items[i].value;
        }
    }
    return NULL;
}

void haldata_free(HalDataObject* obj) {
    if (!obj) return;
    for (int i = 0; i < obj->count; i++) {
        free(obj->items[i].key);
        free(obj->items[i].value);
    }
    free(obj->items);
    free(obj);
}
