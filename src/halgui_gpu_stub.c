/*
 * HalGUI GPU Stub - placeholder for GPU functions when not using D3D11
 */

#include "runtime.h"

void halgui_gpu_init(HcsRuntime* rt, const char* title, int width, int height) {
    // Not implemented - use HalGUI instead
}

void halgui_gpu_shutdown(void) {}

void halgui_gpu_run(HcsRuntime* rt, HcsAstNode* render_callback) {}

HcsValue* halgui_gpu_draw_rect_fn(HcsRuntime* rt, HcsAstList* args) { return value_null(); }
HcsValue* halgui_gpu_draw_rounded_rect_fn(HcsRuntime* rt, HcsAstList* args) { return value_null(); }
HcsValue* halgui_gpu_draw_circle_fn(HcsRuntime* rt, HcsAstList* args) { return value_null(); }
HcsValue* halgui_gpu_draw_line_fn(HcsRuntime* rt, HcsAstList* args) { return value_null(); }
HcsValue* halgui_gpu_draw_gradient_fn(HcsRuntime* rt, HcsAstList* args) { return value_null(); }
HcsValue* halgui_gpu_draw_shadow_fn(HcsRuntime* rt, HcsAstList* args) { return value_null(); }
HcsValue* halgui_gpu_draw_text_fn(HcsRuntime* rt, HcsAstList* args) { return value_null(); }
HcsValue* halgui_gpu_get_width_fn(HcsRuntime* rt, HcsAstList* args) { return value_number(0); }
HcsValue* halgui_gpu_get_height_fn(HcsRuntime* rt, HcsAstList* args) { return value_number(0); }
HcsValue* halgui_gpu_set_vsync_fn(HcsRuntime* rt, HcsAstList* args) { return value_null(); }
HcsValue* halgui_gpu_rgb_fn(HcsRuntime* rt, HcsAstList* args) { return value_number(0); }
