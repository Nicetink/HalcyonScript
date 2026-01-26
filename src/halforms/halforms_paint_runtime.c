/*
 * HalForms Paint API - HalcyonScript Runtime Bindings
 * Exposes Paint Canvas functions to HalcyonScript
 */

#include "halforms.h"
#include "../runtime.h"
#include "../value.h"
#include <stdio.h>
#include <string.h>

/* Forward declarations */
typedef struct PaintCanvas PaintCanvas;

/* Widget storage for paint canvases */
static struct {
    char* name;
    PaintCanvas* canvas;
} g_paintCanvases[50];
static int g_paintCanvasCount = 0;

static void add_paint_canvas(const char* name, PaintCanvas* canvas) {
    if (g_paintCanvasCount < 50) {
        g_paintCanvases[g_paintCanvasCount].name = _strdup(name);
        g_paintCanvases[g_paintCanvasCount].canvas = canvas;
        g_paintCanvasCount++;
    }
}

static PaintCanvas* find_paint_canvas(const char* name) {
    for (int i = 0; i < g_paintCanvasCount; i++) {
        if (strcmp(g_paintCanvases[i].name, name) == 0) {
            return g_paintCanvases[i].canvas;
        }
    }
    return NULL;
}

/* ============================================
   HalcyonScript API Functions
   ============================================ */

/* paintcanvas_create(parent, name, x, y, w, h) */
HcsValue* hcs_paintcanvas_create(HcsRuntime* rt, HcsValue** args, int argc) {
    if (argc < 6) return value_null();
    
    (void)value_to_string(args[0]); /* parentName - unused but kept for API compatibility */
    const char* name = value_to_string(args[1]);
    int x = (int)value_to_number(args[2]);
    int y = (int)value_to_number(args[3]);
    int w = (int)value_to_number(args[4]);
    int h = (int)value_to_number(args[5]);
    
    /* Get parent form */
    extern HalForm* halforms_rt_get_main_form(void);
    extern void halforms_rt_add_widget(const char* name, void* control, int type);
    HalForm* parent = halforms_rt_get_main_form();
    if (!parent) return value_bool(false);
    
    PaintCanvas* canvas = paintcanvas_create(parent, x, y, w, h);
    if (canvas) {
        add_paint_canvas(name, canvas);
        /* Register as widget - use type 100 for paint canvas */
        halforms_rt_add_widget(name, canvas, 100);
        return value_bool(true);
    }
    return value_bool(false);
}

/* paintcanvas_set_tool(name, tool) */
HcsValue* hcs_paintcanvas_set_tool(HcsRuntime* rt, HcsValue** args, int argc) {
    if (argc < 2) return value_null();
    
    const char* name = value_to_string(args[0]);
    const char* toolName = value_to_string(args[1]);
    
    PaintCanvas* canvas = find_paint_canvas(name);
    if (!canvas) return value_bool(false);
    
    PaintToolType tool = PAINT_TOOL_BRUSH;
    if (strcmp(toolName, "pencil") == 0) tool = PAINT_TOOL_PENCIL;
    else if (strcmp(toolName, "brush") == 0) tool = PAINT_TOOL_BRUSH;
    else if (strcmp(toolName, "eraser") == 0) tool = PAINT_TOOL_ERASER;
    else if (strcmp(toolName, "fill") == 0) tool = PAINT_TOOL_FILL;
    else if (strcmp(toolName, "picker") == 0) tool = PAINT_TOOL_PICKER;
    else if (strcmp(toolName, "line") == 0) tool = PAINT_TOOL_LINE;
    else if (strcmp(toolName, "rect") == 0) tool = PAINT_TOOL_RECT;
    else if (strcmp(toolName, "ellipse") == 0) tool = PAINT_TOOL_ELLIPSE;
    else if (strcmp(toolName, "polygon") == 0) tool = PAINT_TOOL_POLYGON;
    else if (strcmp(toolName, "text") == 0) tool = PAINT_TOOL_TEXT;
    else if (strcmp(toolName, "select") == 0) tool = PAINT_TOOL_SELECT_RECT;
    else if (strcmp(toolName, "spray") == 0) tool = PAINT_TOOL_SPRAY;
    
    paintcanvas_set_tool(canvas, tool);
    return value_bool(true);
}

/* paintcanvas_set_color(name, r, g, b) */
HcsValue* hcs_paintcanvas_set_color(HcsRuntime* rt, HcsValue** args, int argc) {
    if (argc < 4) return value_null();
    
    const char* name = value_to_string(args[0]);
    int r = (int)value_to_number(args[1]);
    int g = (int)value_to_number(args[2]);
    int b = (int)value_to_number(args[3]);
    
    PaintCanvas* canvas = find_paint_canvas(name);
    if (!canvas) return value_bool(false);
    
    paintcanvas_set_forecolor(canvas, RGB(r, g, b));
    return value_bool(true);
}

/* paintcanvas_set_brush_size(name, size) */
HcsValue* hcs_paintcanvas_set_brush_size(HcsRuntime* rt, HcsValue** args, int argc) {
    if (argc < 2) return value_null();
    
    const char* name = value_to_string(args[0]);
    int size = (int)value_to_number(args[1]);
    
    PaintCanvas* canvas = find_paint_canvas(name);
    if (!canvas) return value_bool(false);
    
    paintcanvas_set_brush_size(canvas, size);
    return value_bool(true);
}

/* paintcanvas_set_brush_type(name, type) */
HcsValue* hcs_paintcanvas_set_brush_type(HcsRuntime* rt, HcsValue** args, int argc) {
    if (argc < 2) return value_null();
    
    const char* name = value_to_string(args[0]);
    const char* typeName = value_to_string(args[1]);
    
    PaintCanvas* canvas = find_paint_canvas(name);
    if (!canvas) return value_bool(false);
    
    HalBrushType type = HALBRUSH_ROUND;
    if (strcmp(typeName, "round") == 0) type = HALBRUSH_ROUND;
    else if (strcmp(typeName, "square") == 0) type = HALBRUSH_SQUARE;
    else if (strcmp(typeName, "soft") == 0) type = HALBRUSH_SOFT;
    else if (strcmp(typeName, "spray") == 0) type = HALBRUSH_SPRAY;
    else if (strcmp(typeName, "calligraphy") == 0) type = HALBRUSH_CALLIGRAPHY;
    
    paintcanvas_set_brush_type(canvas, type);
    return value_bool(true);
}

/* paintcanvas_clear(name, r, g, b) */
HcsValue* hcs_paintcanvas_clear(HcsRuntime* rt, HcsValue** args, int argc) {
    if (argc < 4) return value_null();
    
    const char* name = value_to_string(args[0]);
    int r = (int)value_to_number(args[1]);
    int g = (int)value_to_number(args[2]);
    int b = (int)value_to_number(args[3]);
    
    PaintCanvas* canvas = find_paint_canvas(name);
    if (!canvas) return value_bool(false);
    
    paintcanvas_clear(canvas, RGB(r, g, b));
    return value_bool(true);
}

/* paintcanvas_undo(name) */
HcsValue* hcs_paintcanvas_undo(HcsRuntime* rt, HcsValue** args, int argc) {
    if (argc < 1) return value_null();
    
    const char* name = value_to_string(args[0]);
    PaintCanvas* canvas = find_paint_canvas(name);
    if (!canvas) return value_bool(false);
    
    paintcanvas_undo(canvas);
    return value_bool(true);
}

/* paintcanvas_redo(name) */
HcsValue* hcs_paintcanvas_redo(HcsRuntime* rt, HcsValue** args, int argc) {
    if (argc < 1) return value_null();
    
    const char* name = value_to_string(args[0]);
    PaintCanvas* canvas = find_paint_canvas(name);
    if (!canvas) return value_bool(false);
    
    paintcanvas_redo(canvas);
    return value_bool(true);
}

/* paintcanvas_save(name, filename) */
HcsValue* hcs_paintcanvas_save(HcsRuntime* rt, HcsValue** args, int argc) {
    if (argc < 2) return value_null();
    
    const char* name = value_to_string(args[0]);
    const char* filename = value_to_string(args[1]);
    
    PaintCanvas* canvas = find_paint_canvas(name);
    if (!canvas) return value_bool(false);
    
    bool result = paintcanvas_save_bmp(canvas, filename);
    return value_bool(result);
}

/* paintcanvas_load(name, filename) */
HcsValue* hcs_paintcanvas_load(HcsRuntime* rt, HcsValue** args, int argc) {
    if (argc < 2) return value_null();
    
    const char* name = value_to_string(args[0]);
    const char* filename = value_to_string(args[1]);
    
    PaintCanvas* canvas = find_paint_canvas(name);
    if (!canvas) return value_bool(false);
    
    bool result = paintcanvas_load_bmp(canvas, filename);
    return value_bool(result);
}

/* paintcanvas_invert(name) */
HcsValue* hcs_paintcanvas_invert(HcsRuntime* rt, HcsValue** args, int argc) {
    if (argc < 1) return value_null();
    
    const char* name = value_to_string(args[0]);
    PaintCanvas* canvas = find_paint_canvas(name);
    if (!canvas) return value_bool(false);
    
    paintcanvas_invert_colors(canvas);
    return value_bool(true);
}

/* paintcanvas_grayscale(name) */
HcsValue* hcs_paintcanvas_grayscale(HcsRuntime* rt, HcsValue** args, int argc) {
    if (argc < 1) return value_null();
    
    const char* name = value_to_string(args[0]);
    PaintCanvas* canvas = find_paint_canvas(name);
    if (!canvas) return value_bool(false);
    
    paintcanvas_grayscale(canvas);
    return value_bool(true);
}

/* paintcanvas_brightness(name, amount) */
HcsValue* hcs_paintcanvas_brightness(HcsRuntime* rt, HcsValue** args, int argc) {
    if (argc < 2) return value_null();
    
    const char* name = value_to_string(args[0]);
    int amount = (int)value_to_number(args[1]);
    
    PaintCanvas* canvas = find_paint_canvas(name);
    if (!canvas) return value_bool(false);
    
    paintcanvas_adjust_brightness(canvas, amount);
    return value_bool(true);
}

/* paintcanvas_blur(name, radius) */
HcsValue* hcs_paintcanvas_blur(HcsRuntime* rt, HcsValue** args, int argc) {
    if (argc < 2) return value_null();
    
    const char* name = value_to_string(args[0]);
    int radius = (int)value_to_number(args[1]);
    
    PaintCanvas* canvas = find_paint_canvas(name);
    if (!canvas) return value_bool(false);
    
    paintcanvas_blur(canvas, radius);
    return value_bool(true);
}

/* ============================================
   Registration Function - Not used, functions called directly from runtime.c
   ============================================ */

