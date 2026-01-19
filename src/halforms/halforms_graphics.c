/*
 * HalForms Graphics - Drawing and Canvas Support
 * GDI+ based graphics for custom drawing
 */

#include "halforms.h"
#include <windows.h>
#include <gdiplus.h>
#include <stdio.h>

/* GDI+ initialization */
static ULONG_PTR g_gdiplusToken = 0;
static bool g_gdiplusInit = false;

bool halforms_graphics_init(void) {
    if (g_gdiplusInit) return true;
    
    GdiplusStartupInput input = {0};
    input.GdiplusVersion = 1;
    
    if (GdiplusStartup(&g_gdiplusToken, &input, NULL) == Ok) {
        g_gdiplusInit = true;
        return true;
    }
    return false;
}

void halforms_graphics_shutdown(void) {
    if (g_gdiplusInit) {
        GdiplusShutdown(g_gdiplusToken);
        g_gdiplusInit = false;
    }
}

/* ============================================
   Canvas Control for Custom Drawing
   ============================================ */

typedef struct {
    HalControl base;
    HDC memDC;
    HBITMAP memBitmap;
    HBITMAP oldBitmap;
    int bufferWidth;
    int bufferHeight;
    COLORREF backgroundColor;
} HalCanvas;

static LRESULT CALLBACK CanvasWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    HalCanvas* canvas = (HalCanvas*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    
    switch (msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            if (canvas && canvas->memDC) {
                BitBlt(hdc, 0, 0, canvas->bufferWidth, canvas->bufferHeight,
                       canvas->memDC, 0, 0, SRCCOPY);
            }
            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_ERASEBKGND:
            return 1; /* Prevent flicker */
        case WM_SIZE: {
            if (canvas) {
                int newW = LOWORD(lParam);
                int newH = HIWORD(lParam);
                if (newW > 0 && newH > 0 && (newW != canvas->bufferWidth || newH != canvas->bufferHeight)) {
                    /* Resize buffer */
                    HDC hdc = GetDC(hwnd);
                    HBITMAP newBitmap = CreateCompatibleBitmap(hdc, newW, newH);
                    HDC newDC = CreateCompatibleDC(hdc);
                    HBITMAP oldNew = (HBITMAP)SelectObject(newDC, newBitmap);
                    
                    /* Copy old content */
                    if (canvas->memDC) {
                        BitBlt(newDC, 0, 0, canvas->bufferWidth, canvas->bufferHeight,
                               canvas->memDC, 0, 0, SRCCOPY);
                        SelectObject(canvas->memDC, canvas->oldBitmap);
                        DeleteObject(canvas->memBitmap);
                        DeleteDC(canvas->memDC);
                    }
                    
                    canvas->memDC = newDC;
                    canvas->memBitmap = newBitmap;
                    canvas->oldBitmap = oldNew;
                    canvas->bufferWidth = newW;
                    canvas->bufferHeight = newH;
                    
                    ReleaseDC(hwnd, hdc);
                }
            }
            return 0;
        }
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

HalCanvas* halcanvas_create(HalForm* parent, int x, int y, int w, int h) {
    static bool classRegistered = false;
    if (!classRegistered) {
        WNDCLASSEXW wc = {sizeof(WNDCLASSEXW)};
        wc.lpfnWndProc = CanvasWndProc;
        wc.hInstance = GetModuleHandle(NULL);
        wc.lpszClassName = L"HalFormsCanvas";
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        RegisterClassExW(&wc);
        classRegistered = true;
    }
    
    HalCanvas* canvas = (HalCanvas*)calloc(1, sizeof(HalCanvas));
    if (!canvas) return NULL;
    
    canvas->base.hwnd = CreateWindowExW(WS_EX_CLIENTEDGE, L"HalFormsCanvas", L"",
        WS_CHILD | WS_VISIBLE,
        x, y, w, h, parent->base.hwnd, NULL, GetModuleHandle(NULL), NULL);
    
    SetWindowLongPtr(canvas->base.hwnd, GWLP_USERDATA, (LONG_PTR)canvas);
    
    /* Create double buffer */
    HDC hdc = GetDC(canvas->base.hwnd);
    canvas->memDC = CreateCompatibleDC(hdc);
    canvas->memBitmap = CreateCompatibleBitmap(hdc, w, h);
    canvas->oldBitmap = (HBITMAP)SelectObject(canvas->memDC, canvas->memBitmap);
    canvas->bufferWidth = w;
    canvas->bufferHeight = h;
    canvas->backgroundColor = RGB(255, 255, 255);
    
    /* Clear to white */
    RECT rc = {0, 0, w, h};
    HBRUSH brush = CreateSolidBrush(canvas->backgroundColor);
    FillRect(canvas->memDC, &rc, brush);
    DeleteObject(brush);
    
    ReleaseDC(canvas->base.hwnd, hdc);
    
    canvas->base.x = x;
    canvas->base.y = y;
    canvas->base.width = w;
    canvas->base.height = h;
    canvas->base.visible = true;
    canvas->base.enabled = true;
    
    return canvas;
}

void halcanvas_clear(HalCanvas* canvas, COLORREF color) {
    if (!canvas || !canvas->memDC) return;
    RECT rc = {0, 0, canvas->bufferWidth, canvas->bufferHeight};
    HBRUSH brush = CreateSolidBrush(color);
    FillRect(canvas->memDC, &rc, brush);
    DeleteObject(brush);
    canvas->backgroundColor = color;
    InvalidateRect(canvas->base.hwnd, NULL, FALSE);
}

void halcanvas_draw_line(HalCanvas* canvas, int x1, int y1, int x2, int y2, COLORREF color, int width) {
    if (!canvas || !canvas->memDC) return;
    HPEN pen = CreatePen(PS_SOLID, width, color);
    HPEN oldPen = (HPEN)SelectObject(canvas->memDC, pen);
    MoveToEx(canvas->memDC, x1, y1, NULL);
    LineTo(canvas->memDC, x2, y2);
    SelectObject(canvas->memDC, oldPen);
    DeleteObject(pen);
    InvalidateRect(canvas->base.hwnd, NULL, FALSE);
}

void halcanvas_draw_rect(HalCanvas* canvas, int x, int y, int w, int h, COLORREF color, int width, bool fill) {
    if (!canvas || !canvas->memDC) return;
    HPEN pen = CreatePen(PS_SOLID, width, color);
    HPEN oldPen = (HPEN)SelectObject(canvas->memDC, pen);
    
    if (fill) {
        HBRUSH brush = CreateSolidBrush(color);
        HBRUSH oldBrush = (HBRUSH)SelectObject(canvas->memDC, brush);
        Rectangle(canvas->memDC, x, y, x + w, y + h);
        SelectObject(canvas->memDC, oldBrush);
        DeleteObject(brush);
    } else {
        HBRUSH oldBrush = (HBRUSH)SelectObject(canvas->memDC, GetStockObject(NULL_BRUSH));
        Rectangle(canvas->memDC, x, y, x + w, y + h);
        SelectObject(canvas->memDC, oldBrush);
    }
    
    SelectObject(canvas->memDC, oldPen);
    DeleteObject(pen);
    InvalidateRect(canvas->base.hwnd, NULL, FALSE);
}

void halcanvas_draw_ellipse(HalCanvas* canvas, int x, int y, int w, int h, COLORREF color, int width, bool fill) {
    if (!canvas || !canvas->memDC) return;
    HPEN pen = CreatePen(PS_SOLID, width, color);
    HPEN oldPen = (HPEN)SelectObject(canvas->memDC, pen);
    
    if (fill) {
        HBRUSH brush = CreateSolidBrush(color);
        HBRUSH oldBrush = (HBRUSH)SelectObject(canvas->memDC, brush);
        Ellipse(canvas->memDC, x, y, x + w, y + h);
        SelectObject(canvas->memDC, oldBrush);
        DeleteObject(brush);
    } else {
        HBRUSH oldBrush = (HBRUSH)SelectObject(canvas->memDC, GetStockObject(NULL_BRUSH));
        Ellipse(canvas->memDC, x, y, x + w, y + h);
        SelectObject(canvas->memDC, oldBrush);
    }
    
    SelectObject(canvas->memDC, oldPen);
    DeleteObject(pen);
    InvalidateRect(canvas->base.hwnd, NULL, FALSE);
}

void halcanvas_draw_text(HalCanvas* canvas, const char* text, int x, int y, COLORREF color, const char* fontName, int fontSize, bool bold) {
    if (!canvas || !canvas->memDC || !text) return;
    
    wchar_t wtext[1024], wfont[64];
    MultiByteToWideChar(CP_UTF8, 0, text, -1, wtext, 1024);
    MultiByteToWideChar(CP_UTF8, 0, fontName ? fontName : "Segoe UI", -1, wfont, 64);
    
    HFONT font = CreateFontW(fontSize, 0, 0, 0, bold ? FW_BOLD : FW_NORMAL,
        FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, wfont);
    
    HFONT oldFont = (HFONT)SelectObject(canvas->memDC, font);
    SetTextColor(canvas->memDC, color);
    SetBkMode(canvas->memDC, TRANSPARENT);
    TextOutW(canvas->memDC, x, y, wtext, (int)wcslen(wtext));
    SelectObject(canvas->memDC, oldFont);
    DeleteObject(font);
    
    InvalidateRect(canvas->base.hwnd, NULL, FALSE);
}

void halcanvas_draw_polygon(HalCanvas* canvas, POINT* points, int count, COLORREF color, int width, bool fill) {
    if (!canvas || !canvas->memDC || !points || count < 3) return;
    
    HPEN pen = CreatePen(PS_SOLID, width, color);
    HPEN oldPen = (HPEN)SelectObject(canvas->memDC, pen);
    
    if (fill) {
        HBRUSH brush = CreateSolidBrush(color);
        HBRUSH oldBrush = (HBRUSH)SelectObject(canvas->memDC, brush);
        Polygon(canvas->memDC, points, count);
        SelectObject(canvas->memDC, oldBrush);
        DeleteObject(brush);
    } else {
        HBRUSH oldBrush = (HBRUSH)SelectObject(canvas->memDC, GetStockObject(NULL_BRUSH));
        Polygon(canvas->memDC, points, count);
        SelectObject(canvas->memDC, oldBrush);
    }
    
    SelectObject(canvas->memDC, oldPen);
    DeleteObject(pen);
    InvalidateRect(canvas->base.hwnd, NULL, FALSE);
}

void halcanvas_draw_arc(HalCanvas* canvas, int x, int y, int w, int h, int startAngle, int sweepAngle, COLORREF color, int width) {
    if (!canvas || !canvas->memDC) return;
    
    HPEN pen = CreatePen(PS_SOLID, width, color);
    HPEN oldPen = (HPEN)SelectObject(canvas->memDC, pen);
    
    /* Convert angles to radians and calculate start/end points */
    double startRad = startAngle * 3.14159265 / 180.0;
    double endRad = (startAngle + sweepAngle) * 3.14159265 / 180.0;
    
    int cx = x + w / 2;
    int cy = y + h / 2;
    int rx = w / 2;
    int ry = h / 2;
    
    int x1 = cx + (int)(rx * cos(startRad));
    int y1 = cy - (int)(ry * sin(startRad));
    int x2 = cx + (int)(rx * cos(endRad));
    int y2 = cy - (int)(ry * sin(endRad));
    
    Arc(canvas->memDC, x, y, x + w, y + h, x1, y1, x2, y2);
    
    SelectObject(canvas->memDC, oldPen);
    DeleteObject(pen);
    InvalidateRect(canvas->base.hwnd, NULL, FALSE);
}

void halcanvas_draw_rounded_rect(HalCanvas* canvas, int x, int y, int w, int h, int radius, COLORREF color, int width, bool fill) {
    if (!canvas || !canvas->memDC) return;
    
    HPEN pen = CreatePen(PS_SOLID, width, color);
    HPEN oldPen = (HPEN)SelectObject(canvas->memDC, pen);
    
    if (fill) {
        HBRUSH brush = CreateSolidBrush(color);
        HBRUSH oldBrush = (HBRUSH)SelectObject(canvas->memDC, brush);
        RoundRect(canvas->memDC, x, y, x + w, y + h, radius * 2, radius * 2);
        SelectObject(canvas->memDC, oldBrush);
        DeleteObject(brush);
    } else {
        HBRUSH oldBrush = (HBRUSH)SelectObject(canvas->memDC, GetStockObject(NULL_BRUSH));
        RoundRect(canvas->memDC, x, y, x + w, y + h, radius * 2, radius * 2);
        SelectObject(canvas->memDC, oldBrush);
    }
    
    SelectObject(canvas->memDC, oldPen);
    DeleteObject(pen);
    InvalidateRect(canvas->base.hwnd, NULL, FALSE);
}

void halcanvas_draw_gradient(HalCanvas* canvas, int x, int y, int w, int h, COLORREF color1, COLORREF color2, bool horizontal) {
    if (!canvas || !canvas->memDC) return;
    
    TRIVERTEX vertices[2];
    GRADIENT_RECT gRect;
    
    vertices[0].x = x;
    vertices[0].y = y;
    vertices[0].Red = GetRValue(color1) << 8;
    vertices[0].Green = GetGValue(color1) << 8;
    vertices[0].Blue = GetBValue(color1) << 8;
    vertices[0].Alpha = 0xFF00;
    
    vertices[1].x = x + w;
    vertices[1].y = y + h;
    vertices[1].Red = GetRValue(color2) << 8;
    vertices[1].Green = GetGValue(color2) << 8;
    vertices[1].Blue = GetBValue(color2) << 8;
    vertices[1].Alpha = 0xFF00;
    
    gRect.UpperLeft = 0;
    gRect.LowerRight = 1;
    
    GradientFill(canvas->memDC, vertices, 2, &gRect, 1, 
                 horizontal ? GRADIENT_FILL_RECT_H : GRADIENT_FILL_RECT_V);
    
    InvalidateRect(canvas->base.hwnd, NULL, FALSE);
}

void halcanvas_set_pixel(HalCanvas* canvas, int x, int y, COLORREF color) {
    if (!canvas || !canvas->memDC) return;
    SetPixel(canvas->memDC, x, y, color);
}

COLORREF halcanvas_get_pixel(HalCanvas* canvas, int x, int y) {
    if (!canvas || !canvas->memDC) return 0;
    return GetPixel(canvas->memDC, x, y);
}

void halcanvas_refresh(HalCanvas* canvas) {
    if (!canvas) return;
    InvalidateRect(canvas->base.hwnd, NULL, FALSE);
    UpdateWindow(canvas->base.hwnd);
}

void halcanvas_destroy(HalCanvas* canvas) {
    if (!canvas) return;
    if (canvas->memDC) {
        SelectObject(canvas->memDC, canvas->oldBitmap);
        DeleteObject(canvas->memBitmap);
        DeleteDC(canvas->memDC);
    }
    if (canvas->base.hwnd) {
        DestroyWindow(canvas->base.hwnd);
    }
    free(canvas);
}
