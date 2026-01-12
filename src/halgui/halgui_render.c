/*
 * HalGUI - Anti-Aliased Rendering Engine
 * 
 * Uses GDI+ for smooth anti-aliased graphics
 * Features:
 * - Smooth rounded corners without jaggies
 * - Anti-aliased text
 * - Soft shadows
 * - Material Design 3 inspired
 */

#include "halgui.h"
#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// GDI+ Flat API - minimal declarations for C
// Using void* to avoid C++ headers
typedef void* GpGraphics;
typedef void* GpBrush;
typedef void* GpSolidFill;
typedef void* GpPen;
typedef void* GpPath;
typedef void* GpFont;
typedef void* GpFontFamily;
typedef void* GpStringFormat;
typedef int GpStatus;
typedef DWORD ARGB;

typedef struct { float X, Y, Width, Height; } RectF;

// GDI+ enums as integers
#define SmoothingModeAntiAlias 4
#define TextRenderingHintClearTypeGridFit 5
#define UnitPixel 2
#define FillModeWinding 1
#define StringAlignmentNear 0
#define StringAlignmentCenter 1
#define StringAlignmentFar 2

// GDI+ startup structure
typedef struct {
    UINT32 GdiplusVersion;
    void* DebugEventCallback;
    BOOL SuppressBackgroundThread;
    BOOL SuppressExternalCodecs;
} GdiplusStartupInputC;

// GDI+ function imports - using LoadLibrary for runtime linking
typedef GpStatus (WINAPI *PFN_GdiplusStartup)(ULONG_PTR*, const GdiplusStartupInputC*, void*);
typedef void (WINAPI *PFN_GdiplusShutdown)(ULONG_PTR);
typedef GpStatus (WINAPI *PFN_GdipCreateFromHDC)(HDC, GpGraphics*);
typedef GpStatus (WINAPI *PFN_GdipDeleteGraphics)(GpGraphics);
typedef GpStatus (WINAPI *PFN_GdipSetSmoothingMode)(GpGraphics, int);
typedef GpStatus (WINAPI *PFN_GdipSetTextRenderingHint)(GpGraphics, int);
typedef GpStatus (WINAPI *PFN_GdipGraphicsClear)(GpGraphics, ARGB);
typedef GpStatus (WINAPI *PFN_GdipCreateSolidFill)(ARGB, GpSolidFill*);
typedef GpStatus (WINAPI *PFN_GdipDeleteBrush)(GpBrush);
typedef GpStatus (WINAPI *PFN_GdipCreatePen1)(ARGB, float, int, GpPen*);
typedef GpStatus (WINAPI *PFN_GdipDeletePen)(GpPen);
typedef GpStatus (WINAPI *PFN_GdipCreatePath)(int, GpPath*);
typedef GpStatus (WINAPI *PFN_GdipDeletePath)(GpPath);
typedef GpStatus (WINAPI *PFN_GdipResetPath)(GpPath);
typedef GpStatus (WINAPI *PFN_GdipStartPathFigure)(GpPath);
typedef GpStatus (WINAPI *PFN_GdipAddPathArc)(GpPath, float, float, float, float, float, float);
typedef GpStatus (WINAPI *PFN_GdipAddPathLine)(GpPath, float, float, float, float);
typedef GpStatus (WINAPI *PFN_GdipClosePathFigure)(GpPath);
typedef GpStatus (WINAPI *PFN_GdipFillPath)(GpGraphics, GpBrush, GpPath);
typedef GpStatus (WINAPI *PFN_GdipDrawPath)(GpGraphics, GpPen, GpPath);
typedef GpStatus (WINAPI *PFN_GdipDrawLine)(GpGraphics, GpPen, float, float, float, float);
typedef GpStatus (WINAPI *PFN_GdipCreateFontFamilyFromName)(const WCHAR*, void*, GpFontFamily*);
typedef GpStatus (WINAPI *PFN_GdipDeleteFontFamily)(GpFontFamily);
typedef GpStatus (WINAPI *PFN_GdipCreateFont)(GpFontFamily, float, int, int, GpFont*);
typedef GpStatus (WINAPI *PFN_GdipDeleteFont)(GpFont);
typedef GpStatus (WINAPI *PFN_GdipCreateStringFormat)(int, LANGID, GpStringFormat*);
typedef GpStatus (WINAPI *PFN_GdipDeleteStringFormat)(GpStringFormat);
typedef GpStatus (WINAPI *PFN_GdipSetStringFormatAlign)(GpStringFormat, int);
typedef GpStatus (WINAPI *PFN_GdipSetStringFormatLineAlign)(GpStringFormat, int);
typedef GpStatus (WINAPI *PFN_GdipDrawString)(GpGraphics, const WCHAR*, int, GpFont, const RectF*, GpStringFormat, GpBrush);

// Global GDI+ state
static HMODULE g_gdiplus = NULL;
static ULONG_PTR g_gdiplusToken = 0;
static bool g_gdiplusInitialized = false;

// Function pointers
static PFN_GdiplusStartup pfnGdiplusStartup;
static PFN_GdiplusShutdown pfnGdiplusShutdown;
static PFN_GdipCreateFromHDC pfnGdipCreateFromHDC;
static PFN_GdipDeleteGraphics pfnGdipDeleteGraphics;
static PFN_GdipSetSmoothingMode pfnGdipSetSmoothingMode;
static PFN_GdipSetTextRenderingHint pfnGdipSetTextRenderingHint;
static PFN_GdipGraphicsClear pfnGdipGraphicsClear;
static PFN_GdipCreateSolidFill pfnGdipCreateSolidFill;
static PFN_GdipDeleteBrush pfnGdipDeleteBrush;
static PFN_GdipCreatePen1 pfnGdipCreatePen1;
static PFN_GdipDeletePen pfnGdipDeletePen;
static PFN_GdipCreatePath pfnGdipCreatePath;
static PFN_GdipDeletePath pfnGdipDeletePath;
static PFN_GdipResetPath pfnGdipResetPath;
static PFN_GdipStartPathFigure pfnGdipStartPathFigure;
static PFN_GdipAddPathArc pfnGdipAddPathArc;
static PFN_GdipAddPathLine pfnGdipAddPathLine;
static PFN_GdipClosePathFigure pfnGdipClosePathFigure;
static PFN_GdipFillPath pfnGdipFillPath;
static PFN_GdipDrawPath pfnGdipDrawPath;
static PFN_GdipDrawLine pfnGdipDrawLine;
static PFN_GdipCreateFontFamilyFromName pfnGdipCreateFontFamilyFromName;
static PFN_GdipDeleteFontFamily pfnGdipDeleteFontFamily;
static PFN_GdipCreateFont pfnGdipCreateFont;
static PFN_GdipDeleteFont pfnGdipDeleteFont;
static PFN_GdipCreateStringFormat pfnGdipCreateStringFormat;
static PFN_GdipDeleteStringFormat pfnGdipDeleteStringFormat;
static PFN_GdipSetStringFormatAlign pfnGdipSetStringFormatAlign;
static PFN_GdipSetStringFormatLineAlign pfnGdipSetStringFormatLineAlign;
static PFN_GdipDrawString pfnGdipDrawString;


bool hal_gdiplus_init(void) {
    if (g_gdiplusInitialized) return true;
    
    g_gdiplus = LoadLibraryW(L"gdiplus.dll");
    if (!g_gdiplus) return false;
    
    pfnGdiplusStartup = (PFN_GdiplusStartup)GetProcAddress(g_gdiplus, "GdiplusStartup");
    pfnGdiplusShutdown = (PFN_GdiplusShutdown)GetProcAddress(g_gdiplus, "GdiplusShutdown");
    pfnGdipCreateFromHDC = (PFN_GdipCreateFromHDC)GetProcAddress(g_gdiplus, "GdipCreateFromHDC");
    pfnGdipDeleteGraphics = (PFN_GdipDeleteGraphics)GetProcAddress(g_gdiplus, "GdipDeleteGraphics");
    pfnGdipSetSmoothingMode = (PFN_GdipSetSmoothingMode)GetProcAddress(g_gdiplus, "GdipSetSmoothingMode");
    pfnGdipSetTextRenderingHint = (PFN_GdipSetTextRenderingHint)GetProcAddress(g_gdiplus, "GdipSetTextRenderingHint");
    pfnGdipGraphicsClear = (PFN_GdipGraphicsClear)GetProcAddress(g_gdiplus, "GdipGraphicsClear");
    pfnGdipCreateSolidFill = (PFN_GdipCreateSolidFill)GetProcAddress(g_gdiplus, "GdipCreateSolidFill");
    pfnGdipDeleteBrush = (PFN_GdipDeleteBrush)GetProcAddress(g_gdiplus, "GdipDeleteBrush");
    pfnGdipCreatePen1 = (PFN_GdipCreatePen1)GetProcAddress(g_gdiplus, "GdipCreatePen1");
    pfnGdipDeletePen = (PFN_GdipDeletePen)GetProcAddress(g_gdiplus, "GdipDeletePen");
    pfnGdipCreatePath = (PFN_GdipCreatePath)GetProcAddress(g_gdiplus, "GdipCreatePath");
    pfnGdipDeletePath = (PFN_GdipDeletePath)GetProcAddress(g_gdiplus, "GdipDeletePath");
    pfnGdipResetPath = (PFN_GdipResetPath)GetProcAddress(g_gdiplus, "GdipResetPath");
    pfnGdipStartPathFigure = (PFN_GdipStartPathFigure)GetProcAddress(g_gdiplus, "GdipStartPathFigure");
    pfnGdipAddPathArc = (PFN_GdipAddPathArc)GetProcAddress(g_gdiplus, "GdipAddPathArc");
    pfnGdipAddPathLine = (PFN_GdipAddPathLine)GetProcAddress(g_gdiplus, "GdipAddPathLine");
    pfnGdipClosePathFigure = (PFN_GdipClosePathFigure)GetProcAddress(g_gdiplus, "GdipClosePathFigure");
    pfnGdipFillPath = (PFN_GdipFillPath)GetProcAddress(g_gdiplus, "GdipFillPath");
    pfnGdipDrawPath = (PFN_GdipDrawPath)GetProcAddress(g_gdiplus, "GdipDrawPath");
    pfnGdipDrawLine = (PFN_GdipDrawLine)GetProcAddress(g_gdiplus, "GdipDrawLine");
    pfnGdipCreateFontFamilyFromName = (PFN_GdipCreateFontFamilyFromName)GetProcAddress(g_gdiplus, "GdipCreateFontFamilyFromName");
    pfnGdipDeleteFontFamily = (PFN_GdipDeleteFontFamily)GetProcAddress(g_gdiplus, "GdipDeleteFontFamily");
    pfnGdipCreateFont = (PFN_GdipCreateFont)GetProcAddress(g_gdiplus, "GdipCreateFont");
    pfnGdipDeleteFont = (PFN_GdipDeleteFont)GetProcAddress(g_gdiplus, "GdipDeleteFont");
    pfnGdipCreateStringFormat = (PFN_GdipCreateStringFormat)GetProcAddress(g_gdiplus, "GdipCreateStringFormat");
    pfnGdipDeleteStringFormat = (PFN_GdipDeleteStringFormat)GetProcAddress(g_gdiplus, "GdipDeleteStringFormat");
    pfnGdipSetStringFormatAlign = (PFN_GdipSetStringFormatAlign)GetProcAddress(g_gdiplus, "GdipSetStringFormatAlign");
    pfnGdipSetStringFormatLineAlign = (PFN_GdipSetStringFormatLineAlign)GetProcAddress(g_gdiplus, "GdipSetStringFormatLineAlign");
    pfnGdipDrawString = (PFN_GdipDrawString)GetProcAddress(g_gdiplus, "GdipDrawString");
    
    if (!pfnGdiplusStartup) { FreeLibrary(g_gdiplus); return false; }
    
    GdiplusStartupInputC input = {0};
    input.GdiplusVersion = 1;
    
    if (pfnGdiplusStartup(&g_gdiplusToken, &input, NULL) == 0) {
        g_gdiplusInitialized = true;
        return true;
    }
    
    FreeLibrary(g_gdiplus);
    return false;
}

void hal_gdiplus_shutdown(void) {
    if (g_gdiplusInitialized && pfnGdiplusShutdown) {
        pfnGdiplusShutdown(g_gdiplusToken);
    }
    if (g_gdiplus) FreeLibrary(g_gdiplus);
    g_gdiplusInitialized = false;
}

static ARGB hal_to_argb(HalColor c) {
    return (HAL_GET_A(c) << 24) | (HAL_GET_R(c) << 16) | (HAL_GET_G(c) << 8) | HAL_GET_B(c);
}

static HalColor hal_darken(HalColor c, float amount) {
    int r = (int)(HAL_GET_R(c) * (1 - amount));
    int g = (int)(HAL_GET_G(c) * (1 - amount));
    int b = (int)(HAL_GET_B(c) * (1 - amount));
    return HAL_RGBA(r < 0 ? 0 : r, g < 0 ? 0 : g, b < 0 ? 0 : b, HAL_GET_A(c));
}


// Create rounded rectangle path
static void hal_create_rounded_rect_path(GpPath path, float x, float y, float w, float h, float r) {
    if (!path) return;
    pfnGdipResetPath(path);
    
    if (r <= 0) {
        pfnGdipAddPathLine(path, x, y, x + w, y);
        pfnGdipAddPathLine(path, x + w, y, x + w, y + h);
        pfnGdipAddPathLine(path, x + w, y + h, x, y + h);
        pfnGdipAddPathLine(path, x, y + h, x, y);
        pfnGdipClosePathFigure(path);
        return;
    }
    
    float d = r * 2;
    if (d > w) d = w;
    if (d > h) d = h;
    r = d / 2;
    
    pfnGdipStartPathFigure(path);
    pfnGdipAddPathArc(path, x, y, d, d, 180, 90);
    pfnGdipAddPathArc(path, x + w - d, y, d, d, 270, 90);
    pfnGdipAddPathArc(path, x + w - d, y + h - d, d, d, 0, 90);
    pfnGdipAddPathArc(path, x, y + h - d, d, d, 90, 90);
    pfnGdipClosePathFigure(path);
}

static void hal_draw_rounded_rect_aa(GpGraphics g, float x, float y, float w, float h, float r,
                                      HalColor fillColor, HalColor borderColor, float borderWidth) {
    if (w <= 0 || h <= 0 || !g) return;
    
    GpPath path = NULL;
    pfnGdipCreatePath(FillModeWinding, &path);
    if (!path) return;
    
    hal_create_rounded_rect_path(path, x, y, w, h, r);
    
    if (HAL_GET_A(fillColor) > 0) {
        GpSolidFill brush = NULL;
        pfnGdipCreateSolidFill(hal_to_argb(fillColor), &brush);
        if (brush) {
            pfnGdipFillPath(g, brush, path);
            pfnGdipDeleteBrush(brush);
        }
    }
    
    if (borderWidth > 0 && HAL_GET_A(borderColor) > 0) {
        GpPen pen = NULL;
        pfnGdipCreatePen1(hal_to_argb(borderColor), borderWidth, UnitPixel, &pen);
        if (pen) {
            pfnGdipDrawPath(g, pen, path);
            pfnGdipDeletePen(pen);
        }
    }
    
    pfnGdipDeletePath(path);
}

static void hal_draw_soft_shadow_aa(GpGraphics g, float x, float y, float w, float h, float r,
                                     int elevation, uint8_t baseAlpha) {
    if (elevation <= 0 || !g) return;
    
    int layers = 6;
    float maxSpread = (float)elevation + 3;
    
    for (int i = layers; i >= 1; i--) {
        float t = (float)i / layers;
        float spread = maxSpread * t;
        float offsetY = elevation * t * 0.6f;
        float alpha = (1.0f - t) * (1.0f - t);
        uint8_t layerAlpha = (uint8_t)(baseAlpha * alpha * 0.4f);
        if (layerAlpha < 2) continue;
        
        HalColor shadowColor = HAL_RGBA(0, 0, 0, layerAlpha);
        hal_draw_rounded_rect_aa(g, x - spread/2, y + offsetY, w + spread, h + spread/2, r + spread/3, shadowColor, HAL_RGBA(0,0,0,0), 0);
    }
}


static void hal_draw_text_aa(GpGraphics g, const char* text, float x, float y, float w, float h,
                              HalColor color, HalAlignment alignH, HalAlignment alignV, float fontSize) {
    if (!text || !*text || !g) return;
    
    int len = MultiByteToWideChar(CP_UTF8, 0, text, -1, NULL, 0);
    wchar_t* wideText = (wchar_t*)malloc(len * sizeof(wchar_t));
    if (!wideText) return;
    MultiByteToWideChar(CP_UTF8, 0, text, -1, wideText, len);
    
    GpFontFamily family = NULL;
    GpFont font = NULL;
    pfnGdipCreateFontFamilyFromName(L"Segoe UI", NULL, &family);
    if (!family) pfnGdipCreateFontFamilyFromName(L"Arial", NULL, &family);
    if (family) pfnGdipCreateFont(family, fontSize, 0, UnitPixel, &font);
    
    GpStringFormat format = NULL;
    pfnGdipCreateStringFormat(0, 0, &format);
    
    if (format) {
        int hAlign = StringAlignmentNear;
        int vAlign = StringAlignmentNear;
        if (alignH == HAL_ALIGN_CENTER) hAlign = StringAlignmentCenter;
        else if (alignH == HAL_ALIGN_RIGHT) hAlign = StringAlignmentFar;
        if (alignV == HAL_ALIGN_MIDDLE) vAlign = StringAlignmentCenter;
        else if (alignV == HAL_ALIGN_BOTTOM) vAlign = StringAlignmentFar;
        pfnGdipSetStringFormatAlign(format, hAlign);
        pfnGdipSetStringFormatLineAlign(format, vAlign);
    }
    
    GpSolidFill brush = NULL;
    pfnGdipCreateSolidFill(hal_to_argb(color), &brush);
    
    if (font && brush) {
        RectF rect = {x, y, w, h};
        pfnGdipDrawString(g, wideText, -1, font, &rect, format, brush);
    }
    
    if (brush) pfnGdipDeleteBrush(brush);
    if (format) pfnGdipDeleteStringFormat(format);
    if (font) pfnGdipDeleteFont(font);
    if (family) pfnGdipDeleteFontFamily(family);
    free(wideText);
}

/* Widget Rendering */
void hal_render_widget_offset(HalWindow* window, HalWidget* widget, GpGraphics g, int offsetX, int offsetY);

static void hal_render_button_aa(HalWindow* window, HalWidget* widget, GpGraphics g, int offsetX, int offsetY) {
    HalTheme* theme = window->theme;
    float scale = hal_get_dpi_scale();
    float x = (widget->bounds.x + offsetX) * scale;
    float y = (widget->bounds.y + offsetY) * scale;
    float w = widget->bounds.width * scale;
    float h = widget->bounds.height * scale;
    float r = 12 * scale;
    
    HalColor bgColor, textColor;
    int elevation = 3;
    
    if (widget->state & HAL_STATE_DISABLED) { bgColor = theme->surface; textColor = theme->textDisabled; elevation = 0; }
    else if (widget->state & HAL_STATE_ACTIVE) { bgColor = theme->primaryActive; textColor = theme->textOnPrimary; elevation = 1; }
    else if (widget->state & HAL_STATE_HOVER) { bgColor = theme->primaryHover; textColor = theme->textOnPrimary; elevation = 5; }
    else { bgColor = theme->primary; textColor = theme->textOnPrimary; elevation = 3; }
    
    if (widget->bgColor) bgColor = *widget->bgColor;
    if (widget->fgColor) textColor = *widget->fgColor;
    
    hal_draw_soft_shadow_aa(g, x, y, w, h, r, elevation, theme->shadowAlpha2);
    hal_draw_rounded_rect_aa(g, x, y, w, h, r, bgColor, HAL_RGBA(0,0,0,0), 0);
    
    const char* text = (const char*)widget->data;
    if (text) hal_draw_text_aa(g, text, x + 16*scale, y, w - 32*scale, h, textColor, HAL_ALIGN_CENTER, HAL_ALIGN_MIDDLE, 14*scale);
}

static void hal_render_label_aa(HalWindow* window, HalWidget* widget, GpGraphics g, int offsetX, int offsetY) {
    HalTheme* theme = window->theme;
    float scale = hal_get_dpi_scale();
    float x = (widget->bounds.x + offsetX) * scale;
    float y = (widget->bounds.y + offsetY) * scale;
    float w = widget->bounds.width * scale;
    float h = widget->bounds.height * scale;
    HalColor textColor = widget->fgColor ? *widget->fgColor : theme->textPrimary;
    const char* text = (const char*)widget->data;
    if (text) hal_draw_text_aa(g, text, x, y, w, h, textColor, widget->alignH, widget->alignV, 14*scale);
}

static void hal_render_checkbox_aa(HalWindow* window, HalWidget* widget, GpGraphics g, int offsetX, int offsetY) {
    HalTheme* theme = window->theme;
    float scale = hal_get_dpi_scale();
    float x = (widget->bounds.x + offsetX) * scale;
    float y = (widget->bounds.y + offsetY) * scale;
    float h = widget->bounds.height * scale;
    float boxSize = 20 * scale, r = 5 * scale;
    float boxY = y + (h - boxSize) / 2;
    bool checked = (widget->state & HAL_STATE_CHECKED) != 0;
    
    HalColor bgColor = checked ? theme->primary : theme->surface;
    HalColor borderColor = checked ? theme->primary : theme->border;
    if (widget->state & HAL_STATE_HOVER) { if (checked) bgColor = theme->primaryHover; else borderColor = theme->textSecondary; }
    
    hal_draw_rounded_rect_aa(g, x, boxY, boxSize, boxSize, r, bgColor, borderColor, checked ? 0 : 2*scale);
    
    if (checked) {
        GpPen pen = NULL;
        pfnGdipCreatePen1(hal_to_argb(theme->textOnPrimary), 2.5f * scale, UnitPixel, &pen);
        if (pen) {
            float cx = x + boxSize/2, cy = boxY + boxSize/2, s = boxSize/4;
            pfnGdipDrawLine(g, pen, cx - s, cy, cx - s/3, cy + s);
            pfnGdipDrawLine(g, pen, cx - s/3, cy + s, cx + s, cy - s + 2);
            pfnGdipDeletePen(pen);
        }
    }
    
    const char* text = (const char*)widget->data;
    if (text) hal_draw_text_aa(g, text, x + boxSize + 10*scale, y, widget->bounds.width*scale - boxSize - 10*scale, h, theme->textPrimary, HAL_ALIGN_LEFT, HAL_ALIGN_MIDDLE, 14*scale);
}

static void hal_render_toggle_aa(HalWindow* window, HalWidget* widget, GpGraphics g, int offsetX, int offsetY) {
    HalTheme* theme = window->theme;
    float scale = hal_get_dpi_scale();
    float x = (widget->bounds.x + offsetX) * scale;
    float y = (widget->bounds.y + offsetY) * scale;
    float h = widget->bounds.height * scale;
    float trackW = 50*scale, trackH = 26*scale, thumbSize = 22*scale, thumbPad = 2*scale;
    float trackY = y + (h - trackH) / 2;
    bool checked = (widget->state & HAL_STATE_CHECKED) != 0;
    
    HalColor trackColor = checked ? theme->primary : theme->surfaceHover;
    if (widget->state & HAL_STATE_HOVER) trackColor = checked ? theme->primaryHover : hal_darken(theme->surfaceHover, 0.05f);
    
    hal_draw_rounded_rect_aa(g, x, trackY, trackW, trackH, trackH/2, trackColor, checked ? HAL_RGBA(0,0,0,0) : theme->border, checked ? 0 : 1*scale);
    
    float thumbX = checked ? x + trackW - thumbSize - thumbPad : x + thumbPad;
    float thumbY = trackY + thumbPad;
    hal_draw_soft_shadow_aa(g, thumbX, thumbY, thumbSize, thumbSize, thumbSize/2, 2, 25);
    hal_draw_rounded_rect_aa(g, thumbX, thumbY, thumbSize, thumbSize, thumbSize/2, HAL_RGB(255,255,255), HAL_RGBA(0,0,0,0), 0);
    
    const char* text = (const char*)widget->data;
    if (text) hal_draw_text_aa(g, text, x + trackW + 12*scale, y, widget->bounds.width*scale - trackW - 12*scale, h, theme->textPrimary, HAL_ALIGN_LEFT, HAL_ALIGN_MIDDLE, 14*scale);
}


static void hal_render_slider_aa(HalWindow* window, HalWidget* widget, GpGraphics g, int offsetX, int offsetY) {
    HalTheme* theme = window->theme;
    float scale = hal_get_dpi_scale();
    float x = (widget->bounds.x + offsetX) * scale;
    float y = (widget->bounds.y + offsetY) * scale;
    float w = widget->bounds.width * scale;
    float h = widget->bounds.height * scale;
    float trackH = 4*scale, thumbSize = 18*scale;
    float trackY = y + (h - trackH) / 2;
    float value = widget->animProgress;
    
    hal_draw_rounded_rect_aa(g, x, trackY, w, trackH, trackH/2, theme->surfaceHover, HAL_RGBA(0,0,0,0), 0);
    float filledW = w * value;
    if (filledW > 0) hal_draw_rounded_rect_aa(g, x, trackY, filledW, trackH, trackH/2, theme->primary, HAL_RGBA(0,0,0,0), 0);
    
    float thumbX = x + filledW - thumbSize/2;
    if (thumbX < x) thumbX = x;
    if (thumbX > x + w - thumbSize) thumbX = x + w - thumbSize;
    float thumbY = y + (h - thumbSize) / 2;
    
    HalColor thumbColor = theme->primary;
    if (widget->state & HAL_STATE_HOVER) thumbColor = theme->primaryHover;
    if (widget->state & HAL_STATE_ACTIVE) thumbColor = theme->primaryActive;
    
    hal_draw_soft_shadow_aa(g, thumbX, thumbY, thumbSize, thumbSize, thumbSize/2, 3, 30);
    hal_draw_rounded_rect_aa(g, thumbX, thumbY, thumbSize, thumbSize, thumbSize/2, thumbColor, HAL_RGB(255,255,255), 2*scale);
}

static void hal_render_progress_aa(HalWindow* window, HalWidget* widget, GpGraphics g, int offsetX, int offsetY) {
    HalTheme* theme = window->theme;
    float scale = hal_get_dpi_scale();
    float x = (widget->bounds.x + offsetX) * scale;
    float y = (widget->bounds.y + offsetY) * scale;
    float w = widget->bounds.width * scale;
    float h = widget->bounds.height * scale;
    float r = h / 2;
    float value = widget->animProgress;
    
    hal_draw_rounded_rect_aa(g, x, y, w, h, r, theme->surfaceHover, HAL_RGBA(0,0,0,0), 0);
    float filledW = w * value;
    if (filledW > r * 2) hal_draw_rounded_rect_aa(g, x, y, filledW, h, r, theme->primary, HAL_RGBA(0,0,0,0), 0);
    else if (filledW > 0) hal_draw_rounded_rect_aa(g, x, y, h, h, r, theme->primary, HAL_RGBA(0,0,0,0), 0);
}

static void hal_render_panel_aa(HalWindow* window, HalWidget* widget, GpGraphics g, int offsetX, int offsetY) {
    HalTheme* theme = window->theme;
    float scale = hal_get_dpi_scale();
    float x = (widget->bounds.x + offsetX) * scale;
    float y = (widget->bounds.y + offsetY) * scale;
    float w = widget->bounds.width * scale;
    float h = widget->bounds.height * scale;
    float r = widget->borderRadius ? (*widget->borderRadius * scale) : (16 * scale);
    
    HalColor bgColor = widget->bgColor ? *widget->bgColor : theme->surface;
    HalColor borderColor = widget->borderColor ? *widget->borderColor : theme->border;
    float borderW = widget->borderWidth ? (*widget->borderWidth * scale) : (1 * scale);
    
    int elevation = (int)(widget->opacity * 6);
    if (elevation > 0) hal_draw_soft_shadow_aa(g, x, y, w, h, r, elevation, theme->shadowAlpha2);
    hal_draw_rounded_rect_aa(g, x, y, w, h, r, bgColor, borderColor, borderW);
    
    int newOffsetX = offsetX + widget->bounds.x;
    int newOffsetY = offsetY + widget->bounds.y;
    for (int i = 0; i < widget->childCount; i++) hal_render_widget_offset(window, widget->children[i], g, newOffsetX, newOffsetY);
}


void hal_render_widget_offset(HalWindow* window, HalWidget* widget, GpGraphics g, int offsetX, int offsetY) {
    if (!widget || !widget->visible) return;
    
    // Skip rendering for native widgets (they render themselves)
    if (widget->type == HAL_WIDGET_TEXTAREA) {
        return;  // Native EDIT control handles its own rendering
    }
    if (widget->type == HAL_WIDGET_INPUT) {
        return;  // Native EDIT control handles its own rendering
    }
    
    switch (widget->type) {
        case HAL_WIDGET_PANEL: hal_render_panel_aa(window, widget, g, offsetX, offsetY); break;
        case HAL_WIDGET_BUTTON: hal_render_button_aa(window, widget, g, offsetX, offsetY); break;
        case HAL_WIDGET_LABEL: hal_render_label_aa(window, widget, g, offsetX, offsetY); break;
        case HAL_WIDGET_CHECKBOX:
        case HAL_WIDGET_RADIO: hal_render_checkbox_aa(window, widget, g, offsetX, offsetY); break;
        case HAL_WIDGET_TOGGLE: hal_render_toggle_aa(window, widget, g, offsetX, offsetY); break;
        case HAL_WIDGET_SLIDER: hal_render_slider_aa(window, widget, g, offsetX, offsetY); break;
        case HAL_WIDGET_PROGRESS: hal_render_progress_aa(window, widget, g, offsetX, offsetY); break;
        default:
            for (int i = 0; i < widget->childCount; i++) hal_render_widget_offset(window, widget->children[i], g, offsetX, offsetY);
            break;
    }
}

void hal_render_widget(HalWindow* window, HalWidget* widget, HDC hdc) {
    if (!g_gdiplusInitialized) return;
    GpGraphics g = NULL;
    pfnGdipCreateFromHDC(hdc, &g);
    if (g) {
        pfnGdipSetSmoothingMode(g, SmoothingModeAntiAlias);
        pfnGdipSetTextRenderingHint(g, TextRenderingHintClearTypeGridFit);
        hal_render_widget_offset(window, widget, g, 0, 0);
        pfnGdipDeleteGraphics(g);
    }
}

void hal_render_window(HalWindow* window) {
    if (!window) return;
    if (!g_gdiplusInitialized) hal_gdiplus_init();
    if (!g_gdiplusInitialized) return;
    
    HalTheme* theme = window->theme;
    HDC hdc = window->backBufferDC;
    
    GpGraphics g = NULL;
    pfnGdipCreateFromHDC(hdc, &g);
    if (!g) return;
    
    pfnGdipSetSmoothingMode(g, SmoothingModeAntiAlias);
    pfnGdipSetTextRenderingHint(g, TextRenderingHintClearTypeGridFit);
    pfnGdipGraphicsClear(g, hal_to_argb(theme->background));
    
    for (int i = 0; i < window->base.childCount; i++) hal_render_widget_offset(window, window->base.children[i], g, 0, 0);
    
    pfnGdipDeleteGraphics(g);
}
