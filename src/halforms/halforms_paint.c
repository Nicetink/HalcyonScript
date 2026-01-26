/*
 * HalForms Paint API - Advanced Drawing Tools for Paint-like Applications
 * Provides brushes, pens, fill tools, selection, layers, and image manipulation
 */

#include "halforms.h"
#include <windows.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

/* ============================================
   Paint Canvas Structure (internal)
   ============================================ */

struct PaintCanvas {
    HalControl base;
    HDC memDC;
    HBITMAP memBitmap;
    HBITMAP oldBitmap;
    int width;
    int height;
    
    /* Current tool settings */
    PaintToolType currentTool;
    COLORREF foreColor;
    COLORREF backColor;
    int brushSize;
    HalBrushType brushType;
    int transparency;
    
    /* Drawing state */
    bool isDrawing;
    int lastX, lastY;
    POINT* polygonPoints;
    int polygonCount;
    
    /* Selection */
    bool hasSelection;
    RECT selectionRect;
    HBITMAP selectionBitmap;
    
    /* Undo/Redo */
    HBITMAP* undoStack;
    int undoCount;
    int undoCapacity;
    int undoPosition;
    
    /* Layers */
    HBITMAP* layers;
    int layerCount;
    int activeLayer;
    
    /* Zoom */
    float zoom;
    int scrollX, scrollY;
};

/* ============================================
   Canvas Creation
   ============================================ */

static LRESULT CALLBACK PaintCanvasProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

PaintCanvas* paintcanvas_create(HalForm* parent, int x, int y, int w, int h) {
    static bool classRegistered = false;
    if (!classRegistered) {
        WNDCLASSEXW wc = {sizeof(WNDCLASSEXW)};
        wc.lpfnWndProc = PaintCanvasProc;
        wc.hInstance = GetModuleHandle(NULL);
        wc.lpszClassName = L"HalFormsPaintCanvas";
        wc.hCursor = LoadCursor(NULL, IDC_CROSS);
        wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
        wc.hbrBackground = NULL;  // No background - we paint everything ourselves
        RegisterClassExW(&wc);
        classRegistered = true;
    }
    
    PaintCanvas* canvas = (PaintCanvas*)calloc(1, sizeof(PaintCanvas));
    if (!canvas) return NULL;
    
    canvas->base.hwnd = CreateWindowExW(0, L"HalFormsPaintCanvas", L"",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP,
        x, y, w, h, parent->base.hwnd, NULL, GetModuleHandle(NULL), NULL);
    
    if (!canvas->base.hwnd) {
        OutputDebugStringA("ERROR: Failed to create Paint Canvas window!\n");
        free(canvas);
        return NULL;
    }
    
    char debug[256];
    sprintf(debug, "Paint Canvas created: hwnd=%p, parent=%p, pos=(%d,%d), size=(%dx%d)\n", 
            canvas->base.hwnd, parent->base.hwnd, x, y, w, h);
    OutputDebugStringA(debug);
    
    SetWindowLongPtr(canvas->base.hwnd, GWLP_USERDATA, (LONG_PTR)canvas);
    
    /* Create drawing surface */
    HDC hdc = GetDC(canvas->base.hwnd);
    canvas->memDC = CreateCompatibleDC(hdc);
    canvas->memBitmap = CreateCompatibleBitmap(hdc, 800, 600);
    canvas->oldBitmap = (HBITMAP)SelectObject(canvas->memDC, canvas->memBitmap);
    canvas->width = 800;
    canvas->height = 600;
    ReleaseDC(canvas->base.hwnd, hdc);
    
    if (!canvas->memDC || !canvas->memBitmap) {
        OutputDebugStringA("ERROR: Failed to create memDC or memBitmap!\n");
        printf("ERROR: Failed to create drawing surface!\n");
        fflush(stdout);
    } else {
        char debug[256];
        sprintf(debug, "Drawing surface created: memDC=%p, memBitmap=%p, size=%dx%d\n",
                canvas->memDC, canvas->memBitmap, canvas->width, canvas->height);
        OutputDebugStringA(debug);
        printf("PAINT: %s", debug);
        fflush(stdout);
    }
    
    /* Clear to white */
    RECT rc = {0, 0, canvas->width, canvas->height};
    FillRect(canvas->memDC, &rc, (HBRUSH)GetStockObject(WHITE_BRUSH));
    
    /* Draw test pattern to verify canvas is visible */
    HPEN testPen = CreatePen(PS_SOLID, 2, RGB(255, 0, 0));
    HPEN oldPen = (HPEN)SelectObject(canvas->memDC, testPen);
    MoveToEx(canvas->memDC, 0, 0, NULL);
    LineTo(canvas->memDC, 100, 100);
    MoveToEx(canvas->memDC, 100, 0, NULL);
    LineTo(canvas->memDC, 0, 100);
    SelectObject(canvas->memDC, oldPen);
    DeleteObject(testPen);
    
    /* Draw text */
    SetBkMode(canvas->memDC, TRANSPARENT);
    SetTextColor(canvas->memDC, RGB(0, 0, 255));
    TextOutA(canvas->memDC, 10, 110, "Paint Canvas Ready - Click to Draw!", 36);
    
    OutputDebugStringA("Test pattern drawn on canvas\n");
    
    /* Initialize settings */
    canvas->currentTool = PAINT_TOOL_BRUSH;
    canvas->foreColor = RGB(0, 0, 0);
    canvas->backColor = RGB(255, 255, 255);
    canvas->brushSize = 3;
    canvas->brushType = HALBRUSH_ROUND;
    canvas->transparency = 255;
    canvas->zoom = 1.0f;
    
    /* Initialize undo stack */
    canvas->undoCapacity = 20;
    canvas->undoStack = (HBITMAP*)calloc(canvas->undoCapacity, sizeof(HBITMAP));
    
    /* Initialize layers */
    canvas->layerCount = 1;
    canvas->layers = (HBITMAP*)calloc(10, sizeof(HBITMAP));
    canvas->activeLayer = 0;
    
    /* Force initial paint */
    InvalidateRect(canvas->base.hwnd, NULL, TRUE);
    UpdateWindow(canvas->base.hwnd);
    
    printf("PAINT: Canvas initialization complete, forced repaint\n");
    fflush(stdout);
    
    return canvas;
}

/* ============================================
   Undo/Redo System
   ============================================ */

void paintcanvas_save_state(PaintCanvas* canvas) {
    if (!canvas) return;
    
    /* Remove any redo states */
    for (int i = canvas->undoPosition; i < canvas->undoCount; i++) {
        if (canvas->undoStack[i]) {
            DeleteObject(canvas->undoStack[i]);
            canvas->undoStack[i] = NULL;
        }
    }
    
    /* Save current state */
    HDC hdc = GetDC(canvas->base.hwnd);
    HBITMAP snapshot = CreateCompatibleBitmap(hdc, canvas->width, canvas->height);
    HDC snapDC = CreateCompatibleDC(hdc);
    HBITMAP oldSnap = (HBITMAP)SelectObject(snapDC, snapshot);
    BitBlt(snapDC, 0, 0, canvas->width, canvas->height, canvas->memDC, 0, 0, SRCCOPY);
    SelectObject(snapDC, oldSnap);
    DeleteDC(snapDC);
    ReleaseDC(canvas->base.hwnd, hdc);
    
    /* Add to stack */
    if (canvas->undoCount >= canvas->undoCapacity) {
        /* Remove oldest */
        DeleteObject(canvas->undoStack[0]);
        memmove(canvas->undoStack, canvas->undoStack + 1, 
                (canvas->undoCapacity - 1) * sizeof(HBITMAP));
        canvas->undoCount--;
    }
    
    canvas->undoStack[canvas->undoCount++] = snapshot;
    canvas->undoPosition = canvas->undoCount;
}

void paintcanvas_undo(PaintCanvas* canvas) {
    if (!canvas || canvas->undoPosition <= 0) return;
    
    canvas->undoPosition--;
    HBITMAP state = canvas->undoStack[canvas->undoPosition];
    
    HDC snapDC = CreateCompatibleDC(canvas->memDC);
    HBITMAP oldSnap = (HBITMAP)SelectObject(snapDC, state);
    BitBlt(canvas->memDC, 0, 0, canvas->width, canvas->height, snapDC, 0, 0, SRCCOPY);
    SelectObject(snapDC, oldSnap);
    DeleteDC(snapDC);
    
    InvalidateRect(canvas->base.hwnd, NULL, FALSE);
}

void paintcanvas_redo(PaintCanvas* canvas) {
    if (!canvas || canvas->undoPosition >= canvas->undoCount) return;
    
    HBITMAP state = canvas->undoStack[canvas->undoPosition];
    canvas->undoPosition++;
    
    HDC snapDC = CreateCompatibleDC(canvas->memDC);
    HBITMAP oldSnap = (HBITMAP)SelectObject(snapDC, state);
    BitBlt(canvas->memDC, 0, 0, canvas->width, canvas->height, snapDC, 0, 0, SRCCOPY);
    SelectObject(snapDC, oldSnap);
    DeleteDC(snapDC);
    
    InvalidateRect(canvas->base.hwnd, NULL, FALSE);
}

/* ============================================
   Drawing Tools Implementation
   ============================================ */

void paintcanvas_draw_brush(PaintCanvas* canvas, int x, int y) {
    if (!canvas) return;
    
    HPEN pen = CreatePen(PS_SOLID, 1, canvas->foreColor);
    HBRUSH brush = CreateSolidBrush(canvas->foreColor);
    HPEN oldPen = (HPEN)SelectObject(canvas->memDC, pen);
    HBRUSH oldBrush = (HBRUSH)SelectObject(canvas->memDC, brush);
    
    switch (canvas->brushType) {
        case HALBRUSH_ROUND:
            Ellipse(canvas->memDC, x - canvas->brushSize, y - canvas->brushSize,
                    x + canvas->brushSize, y + canvas->brushSize);
            break;
        case HALBRUSH_SQUARE:
            Rectangle(canvas->memDC, x - canvas->brushSize, y - canvas->brushSize,
                      x + canvas->brushSize, y + canvas->brushSize);
            break;
        case HALBRUSH_SOFT: {
            /* Soft brush with alpha blending */
            for (int r = canvas->brushSize; r > 0; r--) {
                int alpha = (255 * r) / canvas->brushSize;
                COLORREF blendColor = RGB(
                    (GetRValue(canvas->foreColor) * alpha + 255 * (255 - alpha)) / 255,
                    (GetGValue(canvas->foreColor) * alpha + 255 * (255 - alpha)) / 255,
                    (GetBValue(canvas->foreColor) * alpha + 255 * (255 - alpha)) / 255
                );
                HBRUSH softBrush = CreateSolidBrush(blendColor);
                HBRUSH oldSoft = (HBRUSH)SelectObject(canvas->memDC, softBrush);
                Ellipse(canvas->memDC, x - r, y - r, x + r, y + r);
                SelectObject(canvas->memDC, oldSoft);
                DeleteObject(softBrush);
            }
            break;
        }
        case HALBRUSH_SPRAY: {
            /* Spray paint effect */
            for (int i = 0; i < canvas->brushSize * 5; i++) {
                int dx = (rand() % (canvas->brushSize * 2)) - canvas->brushSize;
                int dy = (rand() % (canvas->brushSize * 2)) - canvas->brushSize;
                if (dx * dx + dy * dy <= canvas->brushSize * canvas->brushSize) {
                    SetPixel(canvas->memDC, x + dx, y + dy, canvas->foreColor);
                }
            }
            break;
        }
        case HALBRUSH_CALLIGRAPHY: {
            /* Angled brush */
            POINT pts[4] = {
                {x - canvas->brushSize, y},
                {x, y - canvas->brushSize / 2},
                {x + canvas->brushSize, y},
                {x, y + canvas->brushSize / 2}
            };
            Polygon(canvas->memDC, pts, 4);
            break;
        }
    }
    
    SelectObject(canvas->memDC, oldPen);
    SelectObject(canvas->memDC, oldBrush);
    DeleteObject(pen);
    DeleteObject(brush);
}

void paintcanvas_draw_line_smooth(PaintCanvas* canvas, int x1, int y1, int x2, int y2) {
    /* Bresenham's line algorithm with brush */
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
    int sx = x1 < x2 ? 1 : -1;
    int sy = y1 < y2 ? 1 : -1;
    int err = dx - dy;
    
    int x = x1, y = y1;
    while (1) {
        paintcanvas_draw_brush(canvas, x, y);
        
        if (x == x2 && y == y2) break;
        
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x += sx;
        }
        if (e2 < dx) {
            err += dx;
            y += sy;
        }
    }
}

void paintcanvas_flood_fill(PaintCanvas* canvas, int x, int y, COLORREF fillColor) {
    if (!canvas || x < 0 || y < 0 || x >= canvas->width || y >= canvas->height) return;
    
    COLORREF targetColor = GetPixel(canvas->memDC, x, y);
    if (targetColor == fillColor) return;
    
    /* Simple flood fill using stack */
    typedef struct { int x, y; } Point;
    Point* stack = (Point*)malloc(canvas->width * canvas->height * sizeof(Point));
    int stackSize = 0;
    
    stack[stackSize++] = (Point){x, y};
    
    while (stackSize > 0) {
        Point p = stack[--stackSize];
        
        if (p.x < 0 || p.y < 0 || p.x >= canvas->width || p.y >= canvas->height) continue;
        if (GetPixel(canvas->memDC, p.x, p.y) != targetColor) continue;
        
        SetPixel(canvas->memDC, p.x, p.y, fillColor);
        
        if (stackSize < canvas->width * canvas->height - 4) {
            stack[stackSize++] = (Point){p.x + 1, p.y};
            stack[stackSize++] = (Point){p.x - 1, p.y};
            stack[stackSize++] = (Point){p.x, p.y + 1};
            stack[stackSize++] = (Point){p.x, p.y - 1};
        }
    }
    
    free(stack);
}

/* ============================================
   Selection Tools
   ============================================ */

void paintcanvas_select_rect(PaintCanvas* canvas, int x1, int y1, int x2, int y2) {
    if (!canvas) return;
    
    canvas->hasSelection = true;
    canvas->selectionRect.left = min(x1, x2);
    canvas->selectionRect.top = min(y1, y2);
    canvas->selectionRect.right = max(x1, x2);
    canvas->selectionRect.bottom = max(y1, y2);
}

void paintcanvas_copy_selection(PaintCanvas* canvas) {
    if (!canvas || !canvas->hasSelection) return;
    
    int w = canvas->selectionRect.right - canvas->selectionRect.left;
    int h = canvas->selectionRect.bottom - canvas->selectionRect.top;
    
    HDC hdc = GetDC(canvas->base.hwnd);
    if (canvas->selectionBitmap) DeleteObject(canvas->selectionBitmap);
    canvas->selectionBitmap = CreateCompatibleBitmap(hdc, w, h);
    
    HDC selDC = CreateCompatibleDC(hdc);
    HBITMAP oldSel = (HBITMAP)SelectObject(selDC, canvas->selectionBitmap);
    BitBlt(selDC, 0, 0, w, h, canvas->memDC, 
           canvas->selectionRect.left, canvas->selectionRect.top, SRCCOPY);
    SelectObject(selDC, oldSel);
    DeleteDC(selDC);
    ReleaseDC(canvas->base.hwnd, hdc);
}

void paintcanvas_paste_selection(PaintCanvas* canvas, int x, int y) {
    if (!canvas || !canvas->selectionBitmap) return;
    
    BITMAP bm;
    GetObject(canvas->selectionBitmap, sizeof(BITMAP), &bm);
    
    HDC selDC = CreateCompatibleDC(canvas->memDC);
    HBITMAP oldSel = (HBITMAP)SelectObject(selDC, canvas->selectionBitmap);
    BitBlt(canvas->memDC, x, y, bm.bmWidth, bm.bmHeight, selDC, 0, 0, SRCCOPY);
    SelectObject(selDC, oldSel);
    DeleteDC(selDC);
    
    InvalidateRect(canvas->base.hwnd, NULL, FALSE);
}

void paintcanvas_delete_selection(PaintCanvas* canvas) {
    if (!canvas || !canvas->hasSelection) return;
    
    RECT rc = canvas->selectionRect;
    FillRect(canvas->memDC, &rc, (HBRUSH)GetStockObject(WHITE_BRUSH));
    canvas->hasSelection = false;
    InvalidateRect(canvas->base.hwnd, NULL, FALSE);
}

/* ============================================
   Image Filters
   ============================================ */

void paintcanvas_invert_colors(PaintCanvas* canvas) {
    if (!canvas) return;
    
    for (int y = 0; y < canvas->height; y++) {
        for (int x = 0; x < canvas->width; x++) {
            COLORREF c = GetPixel(canvas->memDC, x, y);
            COLORREF inverted = RGB(255 - GetRValue(c), 255 - GetGValue(c), 255 - GetBValue(c));
            SetPixel(canvas->memDC, x, y, inverted);
        }
    }
    InvalidateRect(canvas->base.hwnd, NULL, FALSE);
}

void paintcanvas_grayscale(PaintCanvas* canvas) {
    if (!canvas) return;
    
    for (int y = 0; y < canvas->height; y++) {
        for (int x = 0; x < canvas->width; x++) {
            COLORREF c = GetPixel(canvas->memDC, x, y);
            int gray = (GetRValue(c) * 30 + GetGValue(c) * 59 + GetBValue(c) * 11) / 100;
            SetPixel(canvas->memDC, x, y, RGB(gray, gray, gray));
        }
    }
    InvalidateRect(canvas->base.hwnd, NULL, FALSE);
}

void paintcanvas_adjust_brightness(PaintCanvas* canvas, int amount) {
    if (!canvas) return;
    
    for (int y = 0; y < canvas->height; y++) {
        for (int x = 0; x < canvas->width; x++) {
            COLORREF c = GetPixel(canvas->memDC, x, y);
            int r = min(255, max(0, GetRValue(c) + amount));
            int g = min(255, max(0, GetGValue(c) + amount));
            int b = min(255, max(0, GetBValue(c) + amount));
            SetPixel(canvas->memDC, x, y, RGB(r, g, b));
        }
    }
    InvalidateRect(canvas->base.hwnd, NULL, FALSE);
}

void paintcanvas_blur(PaintCanvas* canvas, int radius) {
    if (!canvas || radius < 1) return;
    
    /* Simple box blur */
    HDC tempDC = CreateCompatibleDC(canvas->memDC);
    HBITMAP tempBitmap = CreateCompatibleBitmap(canvas->memDC, canvas->width, canvas->height);
    HBITMAP oldTemp = (HBITMAP)SelectObject(tempDC, tempBitmap);
    
    for (int y = 0; y < canvas->height; y++) {
        for (int x = 0; x < canvas->width; x++) {
            int r = 0, g = 0, b = 0, count = 0;
            
            for (int dy = -radius; dy <= radius; dy++) {
                for (int dx = -radius; dx <= radius; dx++) {
                    int nx = x + dx, ny = y + dy;
                    if (nx >= 0 && ny >= 0 && nx < canvas->width && ny < canvas->height) {
                        COLORREF c = GetPixel(canvas->memDC, nx, ny);
                        r += GetRValue(c);
                        g += GetGValue(c);
                        b += GetBValue(c);
                        count++;
                    }
                }
            }
            
            SetPixel(tempDC, x, y, RGB(r / count, g / count, b / count));
        }
    }
    
    BitBlt(canvas->memDC, 0, 0, canvas->width, canvas->height, tempDC, 0, 0, SRCCOPY);
    SelectObject(tempDC, oldTemp);
    DeleteObject(tempBitmap);
    DeleteDC(tempDC);
    
    InvalidateRect(canvas->base.hwnd, NULL, FALSE);
}

/* ============================================
   Tool Settings
   ============================================ */

void paintcanvas_set_tool(PaintCanvas* canvas, PaintToolType tool) {
    if (!canvas) return;
    canvas->currentTool = tool;
}

void paintcanvas_set_forecolor(PaintCanvas* canvas, COLORREF color) {
    if (!canvas) return;
    canvas->foreColor = color;
}

void paintcanvas_set_backcolor(PaintCanvas* canvas, COLORREF color) {
    if (!canvas) return;
    canvas->backColor = color;
}

void paintcanvas_set_brush_size(PaintCanvas* canvas, int size) {
    if (!canvas) return;
    canvas->brushSize = max(1, min(100, size));
}

void paintcanvas_set_brush_type(PaintCanvas* canvas, HalBrushType type) {
    if (!canvas) return;
    canvas->brushType = type;
}

COLORREF paintcanvas_get_pixel_color(PaintCanvas* canvas, int x, int y) {
    if (!canvas) return 0;
    return GetPixel(canvas->memDC, x, y);
}

/* ============================================
   File Operations
   ============================================ */

bool paintcanvas_save_bmp(PaintCanvas* canvas, const char* filename) {
    if (!canvas || !filename) return false;
    
    BITMAP bm;
    GetObject(canvas->memBitmap, sizeof(BITMAP), &bm);
    
    BITMAPFILEHEADER bfh = {0};
    BITMAPINFOHEADER bih = {0};
    
    bih.biSize = sizeof(BITMAPINFOHEADER);
    bih.biWidth = bm.bmWidth;
    bih.biHeight = bm.bmHeight;
    bih.biPlanes = 1;
    bih.biBitCount = 24;
    bih.biCompression = BI_RGB;
    
    int rowSize = ((bm.bmWidth * 3 + 3) & ~3);
    bih.biSizeImage = rowSize * bm.bmHeight;
    
    bfh.bfType = 0x4D42;
    bfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    bfh.bfSize = bfh.bfOffBits + bih.biSizeImage;
    
    FILE* f = fopen(filename, "wb");
    if (!f) return false;
    
    fwrite(&bfh, sizeof(BITMAPFILEHEADER), 1, f);
    fwrite(&bih, sizeof(BITMAPINFOHEADER), 1, f);
    
    BYTE* pixels = (BYTE*)malloc(bih.biSizeImage);
    GetDIBits(canvas->memDC, canvas->memBitmap, 0, bm.bmHeight, pixels,
              (BITMAPINFO*)&bih, DIB_RGB_COLORS);
    fwrite(pixels, bih.biSizeImage, 1, f);
    free(pixels);
    
    fclose(f);
    return true;
}

bool paintcanvas_load_bmp(PaintCanvas* canvas, const char* filename) {
    if (!canvas || !filename) return false;
    
    wchar_t wpath[MAX_PATH];
    MultiByteToWideChar(CP_UTF8, 0, filename, -1, wpath, MAX_PATH);
    
    HBITMAP hBitmap = (HBITMAP)LoadImageW(NULL, wpath, IMAGE_BITMAP, 0, 0,
                                          LR_LOADFROMFILE | LR_CREATEDIBSECTION);
    if (!hBitmap) return false;
    
    /* Replace current bitmap */
    SelectObject(canvas->memDC, canvas->oldBitmap);
    DeleteObject(canvas->memBitmap);
    canvas->memBitmap = hBitmap;
    canvas->oldBitmap = (HBITMAP)SelectObject(canvas->memDC, canvas->memBitmap);
    
    BITMAP bm;
    GetObject(canvas->memBitmap, sizeof(BITMAP), &bm);
    canvas->width = bm.bmWidth;
    canvas->height = bm.bmHeight;
    
    InvalidateRect(canvas->base.hwnd, NULL, FALSE);
    return true;
}

void paintcanvas_clear(PaintCanvas* canvas, COLORREF color) {
    if (!canvas) return;
    RECT rc = {0, 0, canvas->width, canvas->height};
    HBRUSH brush = CreateSolidBrush(color);
    FillRect(canvas->memDC, &rc, brush);
    DeleteObject(brush);
    InvalidateRect(canvas->base.hwnd, NULL, FALSE);
}

/* ============================================
   Window Procedure
   ============================================ */

static LRESULT CALLBACK PaintCanvasProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    PaintCanvas* canvas = (PaintCanvas*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    
    /* Debug: log all messages */
    if (msg == WM_LBUTTONDOWN || msg == WM_MOUSEMOVE || msg == WM_LBUTTONUP) {
        char debug[256];
        sprintf(debug, "PaintCanvasProc: msg=%d, canvas=%p\n", msg, canvas);
        OutputDebugStringA(debug);
    }
    
    switch (msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            if (canvas && canvas->memDC) {
                printf("PAINT: WM_PAINT called, BitBlt from memDC=%p to hdc=%p, size=%dx%d\n",
                       canvas->memDC, hdc, canvas->width, canvas->height);
                fflush(stdout);
                
                BOOL result = BitBlt(hdc, 0, 0, canvas->width, canvas->height,
                       canvas->memDC, 0, 0, SRCCOPY);
                
                if (!result) {
                    printf("PAINT: ERROR - BitBlt failed! Error=%lu\n", (unsigned long)GetLastError());
                    fflush(stdout);
                }
                
                /* Draw selection rectangle */
                if (canvas->hasSelection) {
                    HPEN pen = CreatePen(PS_DOT, 1, RGB(0, 0, 0));
                    HPEN oldPen = (HPEN)SelectObject(hdc, pen);
                    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
                    Rectangle(hdc, canvas->selectionRect.left, canvas->selectionRect.top,
                             canvas->selectionRect.right, canvas->selectionRect.bottom);
                    SelectObject(hdc, oldPen);
                    SelectObject(hdc, oldBrush);
                    DeleteObject(pen);
                }
            } else {
                printf("PAINT: WM_PAINT called but canvas=%p or memDC=%p is NULL!\n", 
                       canvas, canvas ? canvas->memDC : NULL);
                fflush(stdout);
            }
            EndPaint(hwnd, &ps);
            return 0;
        }
        
        case WM_LBUTTONDOWN: {
            if (canvas) {
                canvas->isDrawing = true;
                canvas->lastX = LOWORD(lParam);
                canvas->lastY = HIWORD(lParam);
                
                /* Debug output to console AND debug string */
                char debug[256];
                sprintf(debug, "Mouse down at %d,%d, tool=%d\n", canvas->lastX, canvas->lastY, canvas->currentTool);
                OutputDebugStringA(debug);
                printf("PAINT: %s", debug);  // Also print to console
                fflush(stdout);
                
                if (canvas->currentTool == PAINT_TOOL_FILL) {
                    paintcanvas_save_state(canvas);
                    paintcanvas_flood_fill(canvas, canvas->lastX, canvas->lastY, canvas->foreColor);
                    InvalidateRect(hwnd, NULL, FALSE);
                } else if (canvas->currentTool == PAINT_TOOL_PICKER) {
                    canvas->foreColor = GetPixel(canvas->memDC, canvas->lastX, canvas->lastY);
                } else {
                    paintcanvas_save_state(canvas);
                }
                
                SetCapture(hwnd);
            }
            return 0;
        }
        
        case WM_MOUSEMOVE: {
            if (canvas && canvas->isDrawing) {
                int x = LOWORD(lParam);
                int y = HIWORD(lParam);
                
                printf("PAINT: Mouse move to %d,%d\n", x, y);
                fflush(stdout);
                
                if (canvas->currentTool == PAINT_TOOL_BRUSH || 
                    canvas->currentTool == PAINT_TOOL_PENCIL) {
                    paintcanvas_draw_line_smooth(canvas, canvas->lastX, canvas->lastY, x, y);
                    InvalidateRect(hwnd, NULL, FALSE);
                } else if (canvas->currentTool == PAINT_TOOL_ERASER) {
                    COLORREF oldColor = canvas->foreColor;
                    canvas->foreColor = canvas->backColor;
                    paintcanvas_draw_line_smooth(canvas, canvas->lastX, canvas->lastY, x, y);
                    canvas->foreColor = oldColor;
                    InvalidateRect(hwnd, NULL, FALSE);
                }
                
                canvas->lastX = x;
                canvas->lastY = y;
            }
            return 0;
        }
        
        case WM_LBUTTONUP: {
            if (canvas && canvas->isDrawing) {
                canvas->isDrawing = false;
                ReleaseCapture();
            }
            return 0;
        }
        
        case WM_ERASEBKGND:
            return 1;
    }
    
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

void paintcanvas_destroy(PaintCanvas* canvas) {
    if (!canvas) return;
    
    /* Clean up undo stack */
    for (int i = 0; i < canvas->undoCount; i++) {
        if (canvas->undoStack[i]) DeleteObject(canvas->undoStack[i]);
    }
    free(canvas->undoStack);
    
    /* Clean up layers */
    for (int i = 0; i < canvas->layerCount; i++) {
        if (canvas->layers[i]) DeleteObject(canvas->layers[i]);
    }
    free(canvas->layers);
    
    if (canvas->selectionBitmap) DeleteObject(canvas->selectionBitmap);
    if (canvas->polygonPoints) free(canvas->polygonPoints);
    
    if (canvas->memDC) {
        SelectObject(canvas->memDC, canvas->oldBitmap);
        DeleteObject(canvas->memBitmap);
        DeleteDC(canvas->memDC);
    }
    
    if (canvas->base.hwnd) DestroyWindow(canvas->base.hwnd);
    free(canvas);
}
