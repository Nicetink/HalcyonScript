/*
 * HalForms - Layout Controls (SplitContainer, DockPanel)
 * 
 */

#include "halforms.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* External globals */
extern struct {
    bool initialized;
    HINSTANCE hInstance;
    HFONT defaultFont;
    HalForm* mainForm;
    HalForm** forms;
    int formCount;
    int formCapacity;
    int nextControlId;
    bool running;
} g_halforms;

/* ============================================
   SplitContainer Implementation
   ============================================ */

typedef struct HalSplitContainerData {
    HWND panel1;
    HWND panel2;
    int splitterPos;
    int splitterWidth;
    bool horizontal;
    bool dragging;
    int minSize1;
    int minSize2;
    HCURSOR hCursor;
} HalSplitContainerData;

static LRESULT CALLBACK SplitContainerProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

static void halforms_register_splitcontainer(void) {
    static bool registered = false;
    if (registered) return;
    
    WNDCLASSEXA wc = {0};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = SplitContainerProc;
    wc.hInstance = g_halforms.hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName = "HalFormsSplitContainer";
    wc.cbWndExtra = sizeof(void*);
    
    RegisterClassExA(&wc);
    registered = true;
}

HalSplitter* halsplitter_create(HalForm* parent, bool horizontal, int x, int y, int w, int h) {
    if (!parent) return NULL;
    
    halforms_register_splitcontainer();
    
    HalSplitter* splitter = (HalSplitter*)calloc(1, sizeof(HalSplitter));
    if (!splitter) return NULL;
    
    splitter->base.type = HALCTRL_PANEL;
    splitter->base.x = x;
    splitter->base.y = y;
    splitter->base.width = w;
    splitter->base.height = h;
    splitter->base.visible = true;
    splitter->base.enabled = true;
    splitter->base.parent = parent;
    splitter->horizontal = horizontal;
    splitter->splitterWidth = 5;
    splitter->minSize1 = 50;
    splitter->minSize2 = 50;
    
    HalSplitContainerData* data = (HalSplitContainerData*)calloc(1, sizeof(HalSplitContainerData));
    data->horizontal = horizontal;
    data->splitterWidth = 5;
    data->minSize1 = 50;
    data->minSize2 = 50;
    data->splitterPos = horizontal ? h / 2 : w / 2;
    data->hCursor = LoadCursor(NULL, horizontal ? IDC_SIZENS : IDC_SIZEWE);
    
    splitter->base.hwnd = CreateWindowExA(
        0, "HalFormsSplitContainer", "",
        WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN,
        x, y, w, h,
        parent->hwnd,
        (HMENU)(LONG_PTR)g_halforms.nextControlId++,
        g_halforms.hInstance,
        NULL
    );
    
    if (!splitter->base.hwnd) {
        free(data);
        free(splitter);
        return NULL;
    }
    
    SetWindowLongPtrA(splitter->base.hwnd, 0, (LONG_PTR)data);
    
    /* Create two child panels */
    int panel1W = horizontal ? w : data->splitterPos;
    int panel1H = horizontal ? data->splitterPos : h;
    int panel2X = horizontal ? 0 : data->splitterPos + data->splitterWidth;
    int panel2Y = horizontal ? data->splitterPos + data->splitterWidth : 0;
    int panel2W = horizontal ? w : w - panel2X;
    int panel2H = horizontal ? h - panel2Y : h;
    
    data->panel1 = CreateWindowExA(
        0, "STATIC", "",
        WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN,
        0, 0, panel1W, panel1H,
        splitter->base.hwnd, NULL, g_halforms.hInstance, NULL
    );
    
    data->panel2 = CreateWindowExA(
        0, "STATIC", "",
        WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN,
        panel2X, panel2Y, panel2W, panel2H,
        splitter->base.hwnd, NULL, g_halforms.hInstance, NULL
    );
    
    /* Add to parent */
    if (parent->base.childCount >= parent->base.childCapacity) {
        parent->base.childCapacity = parent->base.childCapacity == 0 ? 16 : parent->base.childCapacity * 2;
        parent->base.children = (HalControl**)realloc(parent->base.children,
            parent->base.childCapacity * sizeof(HalControl*));
    }
    parent->base.children[parent->base.childCount++] = (HalControl*)splitter;
    
    return splitter;
}

static void SplitContainer_UpdateLayout(HWND hwnd) {
    HalSplitContainerData* data = (HalSplitContainerData*)GetWindowLongPtrA(hwnd, 0);
    if (!data) return;
    
    RECT rc;
    GetClientRect(hwnd, &rc);
    int w = rc.right;
    int h = rc.bottom;
    
    if (data->horizontal) {
        /* Horizontal split (top/bottom) */
        SetWindowPos(data->panel1, NULL, 0, 0, w, data->splitterPos, SWP_NOZORDER);
        SetWindowPos(data->panel2, NULL, 0, data->splitterPos + data->splitterWidth, 
            w, h - data->splitterPos - data->splitterWidth, SWP_NOZORDER);
    } else {
        /* Vertical split (left/right) */
        SetWindowPos(data->panel1, NULL, 0, 0, data->splitterPos, h, SWP_NOZORDER);
        SetWindowPos(data->panel2, NULL, data->splitterPos + data->splitterWidth, 0,
            w - data->splitterPos - data->splitterWidth, h, SWP_NOZORDER);
    }
}

static LRESULT CALLBACK SplitContainerProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    HalSplitContainerData* data = (HalSplitContainerData*)GetWindowLongPtrA(hwnd, 0);
    
    switch (msg) {
        case WM_SIZE:
            if (data) SplitContainer_UpdateLayout(hwnd);
            return 0;
            
        case WM_MOUSEMOVE: {
            if (!data) break;
            int x = LOWORD(lParam);
            int y = HIWORD(lParam);
            
            RECT rc;
            GetClientRect(hwnd, &rc);
            
            /* Check if mouse is over splitter */
            bool overSplitter = false;
            if (data->horizontal) {
                overSplitter = (y >= data->splitterPos && y <= data->splitterPos + data->splitterWidth);
            } else {
                overSplitter = (x >= data->splitterPos && x <= data->splitterPos + data->splitterWidth);
            }
            
            if (overSplitter || data->dragging) {
                SetCursor(data->hCursor);
            }
            
            if (data->dragging) {
                int newPos = data->horizontal ? y : x;
                int maxPos = (data->horizontal ? rc.bottom : rc.right) - data->splitterWidth - data->minSize2;
                
                if (newPos < data->minSize1) newPos = data->minSize1;
                if (newPos > maxPos) newPos = maxPos;
                
                data->splitterPos = newPos;
                SplitContainer_UpdateLayout(hwnd);
            }
            return 0;
        }
        
        case WM_LBUTTONDOWN: {
            if (!data) break;
            int x = LOWORD(lParam);
            int y = HIWORD(lParam);
            
            bool overSplitter = false;
            if (data->horizontal) {
                overSplitter = (y >= data->splitterPos && y <= data->splitterPos + data->splitterWidth);
            } else {
                overSplitter = (x >= data->splitterPos && x <= data->splitterPos + data->splitterWidth);
            }
            
            if (overSplitter) {
                data->dragging = true;
                SetCapture(hwnd);
            }
            return 0;
        }
        
        case WM_LBUTTONUP:
            if (data && data->dragging) {
                data->dragging = false;
                ReleaseCapture();
            }
            return 0;
            
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            
            if (data) {
                RECT rc;
                GetClientRect(hwnd, &rc);
                
                /* Draw splitter bar */
                RECT splitterRect;
                if (data->horizontal) {
                    splitterRect.left = 0;
                    splitterRect.top = data->splitterPos;
                    splitterRect.right = rc.right;
                    splitterRect.bottom = data->splitterPos + data->splitterWidth;
                } else {
                    splitterRect.left = data->splitterPos;
                    splitterRect.top = 0;
                    splitterRect.right = data->splitterPos + data->splitterWidth;
                    splitterRect.bottom = rc.bottom;
                }
                
                FillRect(hdc, &splitterRect, (HBRUSH)(COLOR_3DFACE + 1));
            }
            
            EndPaint(hwnd, &ps);
            return 0;
        }
        
        case WM_DESTROY:
            if (data) free(data);
            return 0;
    }
    
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

void halsplitter_set_position(HalSplitter* splitter, int position) {
    if (!splitter || !splitter->base.hwnd) return;
    
    HalSplitContainerData* data = (HalSplitContainerData*)GetWindowLongPtrA(splitter->base.hwnd, 0);
    if (data) {
        data->splitterPos = position;
        SplitContainer_UpdateLayout(splitter->base.hwnd);
    }
}

int halsplitter_get_position(HalSplitter* splitter) {
    if (!splitter || !splitter->base.hwnd) return 0;
    
    HalSplitContainerData* data = (HalSplitContainerData*)GetWindowLongPtrA(splitter->base.hwnd, 0);
    return data ? data->splitterPos : 0;
}

HWND halsplitter_get_panel1(HalSplitter* splitter) {
    if (!splitter || !splitter->base.hwnd) return NULL;
    HalSplitContainerData* data = (HalSplitContainerData*)GetWindowLongPtrA(splitter->base.hwnd, 0);
    return data ? data->panel1 : NULL;
}

HWND halsplitter_get_panel2(HalSplitter* splitter) {
    if (!splitter || !splitter->base.hwnd) return NULL;
    HalSplitContainerData* data = (HalSplitContainerData*)GetWindowLongPtrA(splitter->base.hwnd, 0);
    return data ? data->panel2 : NULL;
}

/* ============================================
   DockPanel Implementation
   ============================================ */

HalDockPanel* haldockpanel_create(HalForm* parent, int x, int y, int w, int h) {
    if (!parent) return NULL;
    
    HalDockPanel* dock = (HalDockPanel*)calloc(1, sizeof(HalDockPanel));
    if (!dock) return NULL;
    
    dock->base.type = HALCTRL_PANEL;
    dock->base.x = x;
    dock->base.y = y;
    dock->base.width = w;
    dock->base.height = h;
    dock->base.visible = true;
    dock->base.enabled = true;
    dock->base.parent = parent;
    dock->allowFloat = false;
    dock->allowClose = true;
    
    dock->base.hwnd = CreateWindowExA(
        0, "STATIC", "",
        WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN,
        x, y, w, h,
        parent->hwnd,
        (HMENU)(LONG_PTR)g_halforms.nextControlId++,
        g_halforms.hInstance,
        NULL
    );
    
    if (!dock->base.hwnd) {
        free(dock);
        return NULL;
    }
    
    /* Add to parent */
    if (parent->base.childCount >= parent->base.childCapacity) {
        parent->base.childCapacity = parent->base.childCapacity == 0 ? 16 : parent->base.childCapacity * 2;
        parent->base.children = (HalControl**)realloc(parent->base.children,
            parent->base.childCapacity * sizeof(HalControl*));
    }
    parent->base.children[parent->base.childCount++] = (HalControl*)dock;
    
    return dock;
}

void haldockpanel_set_content(HalDockPanel* dock, HalDockStyle position, HalControl* content, int size) {
    if (!dock || !content) return;
    
    switch (position) {
        case HALDOCK_TOP:
            dock->topPanel = content;
            break;
        case HALDOCK_BOTTOM:
            dock->bottomPanel = content;
            break;
        case HALDOCK_LEFT:
            dock->leftPanel = content;
            break;
        case HALDOCK_RIGHT:
            dock->rightPanel = content;
            break;
        case HALDOCK_FILL:
            dock->centerPanel = content;
            break;
        default:
            break;
    }
    
    /* Reparent control */
    SetParent(content->hwnd, dock->base.hwnd);
    
    /* Update layout */
    haldockpanel_update_layout(dock);
}

void haldockpanel_update_layout(HalDockPanel* dock) {
    if (!dock) return;
    
    RECT rc;
    GetClientRect(dock->base.hwnd, &rc);
    
    int left = 0, top = 0;
    int right = rc.right, bottom = rc.bottom;
    
    /* Top panel */
    if (dock->topPanel && dock->topPanel->visible) {
        SetWindowPos(dock->topPanel->hwnd, NULL, left, top, right - left, dock->topPanel->height, SWP_NOZORDER);
        top += dock->topPanel->height;
    }
    
    /* Bottom panel */
    if (dock->bottomPanel && dock->bottomPanel->visible) {
        SetWindowPos(dock->bottomPanel->hwnd, NULL, left, bottom - dock->bottomPanel->height, 
            right - left, dock->bottomPanel->height, SWP_NOZORDER);
        bottom -= dock->bottomPanel->height;
    }
    
    /* Left panel */
    if (dock->leftPanel && dock->leftPanel->visible) {
        SetWindowPos(dock->leftPanel->hwnd, NULL, left, top, dock->leftPanel->width, bottom - top, SWP_NOZORDER);
        left += dock->leftPanel->width;
    }
    
    /* Right panel */
    if (dock->rightPanel && dock->rightPanel->visible) {
        SetWindowPos(dock->rightPanel->hwnd, NULL, right - dock->rightPanel->width, top,
            dock->rightPanel->width, bottom - top, SWP_NOZORDER);
        right -= dock->rightPanel->width;
    }
    
    /* Center panel (fill) */
    if (dock->centerPanel && dock->centerPanel->visible) {
        SetWindowPos(dock->centerPanel->hwnd, NULL, left, top, right - left, bottom - top, SWP_NOZORDER);
    }
}
