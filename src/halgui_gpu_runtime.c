/*
 * HalcyonScript - HalGUI GPU Runtime Integration
 * 
 * Connects HalcyonScript to GPU-accelerated HalGUI renderer
 */

#include "runtime.h"
#include "halgui/halgui_d3d11.h"
#include <stdio.h>
#include <string.h>

/* ============================================
   GPU Runtime State
   ============================================ */

typedef struct {
    HWND window;
    bool initialized;
    bool running;
    HcsRuntime* hcs_runtime;
    uint32_t width;
    uint32_t height;
} HalGPURuntime;

static HalGPURuntime g_gpu_rt = {0};

/* ============================================
   Window Procedure
   ============================================ */

static LRESULT CALLBACK gpu_window_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CLOSE:
            g_gpu_rt.running = false;
            return 0;
            
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
            
        case WM_SIZE: {
            UINT width = LOWORD(lParam);
            UINT height = HIWORD(lParam);
            if (width > 0 && height > 0) {
                g_gpu_rt.width = width;
                g_gpu_rt.height = height;
                hal_gpu_resize(width, height);
            }
            return 0;
        }
        
        case WM_KEYDOWN:
            if (wParam == VK_ESCAPE) {
                g_gpu_rt.running = false;
            }
            return 0;
    }
    
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

/* ============================================
   Initialization
   ============================================ */

void halgui_gpu_init(HcsRuntime* rt, const char* title, int width, int height) {
    if (g_gpu_rt.initialized) return;
    
    g_gpu_rt.hcs_runtime = rt;
    g_gpu_rt.width = width;
    g_gpu_rt.height = height;
    
    // Register window class
    WNDCLASSEXW wc = {0};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = gpu_window_proc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = L"HalGUIGPU";
    
    if (!RegisterClassExW(&wc)) {
        fprintf(stderr, "Failed to register window class\n");
        return;
    }
    
    // Create window
    RECT rect = {0, 0, width, height};
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);
    
    // Convert title to wide string
    int titleLen = MultiByteToWideChar(CP_UTF8, 0, title, -1, NULL, 0);
    wchar_t* wTitle = (wchar_t*)malloc(titleLen * sizeof(wchar_t));
    MultiByteToWideChar(CP_UTF8, 0, title, -1, wTitle, titleLen);
    
    g_gpu_rt.window = CreateWindowExW(
        0, L"HalGUIGPU", wTitle,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        rect.right - rect.left, rect.bottom - rect.top,
        NULL, NULL, GetModuleHandle(NULL), NULL
    );
    
    free(wTitle);
    
    if (!g_gpu_rt.window) {
        fprintf(stderr, "Failed to create window\n");
        return;
    }
    
    // Initialize GPU renderer
    if (!hal_gpu_init(g_gpu_rt.window, width, height)) {
        fprintf(stderr, "Failed to initialize GPU renderer\n");
        DestroyWindow(g_gpu_rt.window);
        g_gpu_rt.window = NULL;
        return;
    }
    
    ShowWindow(g_gpu_rt.window, SW_SHOW);
    UpdateWindow(g_gpu_rt.window);
    
    g_gpu_rt.initialized = true;
    g_gpu_rt.running = true;
    
    printf("HalGUI GPU initialized: %dx%d\n", width, height);
}

void halgui_gpu_shutdown(void) {
    if (!g_gpu_rt.initialized) return;
    
    hal_gpu_shutdown();
    
    if (g_gpu_rt.window) {
        DestroyWindow(g_gpu_rt.window);
        g_gpu_rt.window = NULL;
    }
    
    UnregisterClassW(L"HalGUIGPU", GetModuleHandle(NULL));
    
    memset(&g_gpu_rt, 0, sizeof(g_gpu_rt));
}

/* ============================================
   Main Loop
   ============================================ */

void halgui_gpu_run(HcsRuntime* rt, HcsAstNode* render_callback) {
    if (!g_gpu_rt.initialized || !g_gpu_rt.running) return;
    
    MSG msg;
    LARGE_INTEGER freq, lastTime, currentTime;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&lastTime);
    
    float time = 0.0f;
    
    while (g_gpu_rt.running) {
        // Process messages
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                g_gpu_rt.running = false;
                break;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        
        if (!g_gpu_rt.running) break;
        
        // Calculate delta time
        QueryPerformanceCounter(&currentTime);
        float deltaTime = (float)(currentTime.QuadPart - lastTime.QuadPart) / (float)freq.QuadPart;
        lastTime = currentTime;
        time += deltaTime;
        
        // Set time variable in runtime
        scope_set(rt->current_scope, "time", value_number(time), false);
        scope_set(rt->current_scope, "deltaTime", value_number(deltaTime), false);
        
        // Begin frame
        hal_gpu_begin_frame();
        
        // Execute render callback
        if (render_callback && render_callback->type == HCS_AST_FUNCTION) {
            HcsScope* funcScope = scope_create(rt->current_scope);
            HcsScope* oldScope = rt->current_scope;
            rt->current_scope = funcScope;
            
            for (int i = 0; i < render_callback->data.function.body.count; i++) {
                runtime_execute(rt, render_callback->data.function.body.items[i]);
                if (rt->should_return || rt->should_break) break;
            }
            
            rt->current_scope = oldScope;
            scope_free(funcScope);
        }
        
        // End frame
        hal_gpu_end_frame();
        hal_gpu_present();
        
        // Limit to ~60 FPS
        Sleep(1);
    }
}

/* ============================================
   Drawing Functions (HalcyonScript API)
   ============================================ */

HcsValue* halgui_gpu_draw_rect_fn(HcsRuntime* rt, HcsAstList* args) {
    if (args->count < 5) return value_null();
    
    HcsValue* vx = runtime_eval(rt, args->items[0]);
    HcsValue* vy = runtime_eval(rt, args->items[1]);
    HcsValue* vw = runtime_eval(rt, args->items[2]);
    HcsValue* vh = runtime_eval(rt, args->items[3]);
    HcsValue* vc = runtime_eval(rt, args->items[4]);
    
    float x = (float)value_to_number(vx);
    float y = (float)value_to_number(vy);
    float w = (float)value_to_number(vw);
    float h = (float)value_to_number(vh);
    uint32_t color = (uint32_t)value_to_number(vc);
    
    value_release(vx); value_release(vy); value_release(vw); value_release(vh); value_release(vc);
    
    hal_gpu_draw_rect(x, y, w, h, color);
    return value_null();
}

HcsValue* halgui_gpu_draw_rounded_rect_fn(HcsRuntime* rt, HcsAstList* args) {
    if (args->count < 6) return value_null();
    
    float x = (float)value_to_number(runtime_eval(rt, args->items[0]));
    float y = (float)value_to_number(runtime_eval(rt, args->items[1]));
    float w = (float)value_to_number(runtime_eval(rt, args->items[2]));
    float h = (float)value_to_number(runtime_eval(rt, args->items[3]));
    float radius = (float)value_to_number(runtime_eval(rt, args->items[4]));
    uint32_t color = (uint32_t)value_to_number(runtime_eval(rt, args->items[5]));
    
    hal_gpu_draw_rounded_rect(x, y, w, h, radius, color);
    return value_null();
}

HcsValue* halgui_gpu_draw_circle_fn(HcsRuntime* rt, HcsAstList* args) {
    if (args->count < 4) return value_null();
    
    float cx = (float)value_to_number(runtime_eval(rt, args->items[0]));
    float cy = (float)value_to_number(runtime_eval(rt, args->items[1]));
    float radius = (float)value_to_number(runtime_eval(rt, args->items[2]));
    uint32_t color = (uint32_t)value_to_number(runtime_eval(rt, args->items[3]));
    
    hal_gpu_draw_circle(cx, cy, radius, color);
    return value_null();
}

HcsValue* halgui_gpu_draw_line_fn(HcsRuntime* rt, HcsAstList* args) {
    if (args->count < 6) return value_null();
    
    float x1 = (float)value_to_number(runtime_eval(rt, args->items[0]));
    float y1 = (float)value_to_number(runtime_eval(rt, args->items[1]));
    float x2 = (float)value_to_number(runtime_eval(rt, args->items[2]));
    float y2 = (float)value_to_number(runtime_eval(rt, args->items[3]));
    float thickness = (float)value_to_number(runtime_eval(rt, args->items[4]));
    uint32_t color = (uint32_t)value_to_number(runtime_eval(rt, args->items[5]));
    
    hal_gpu_draw_line(x1, y1, x2, y2, thickness, color);
    return value_null();
}

HcsValue* halgui_gpu_draw_gradient_fn(HcsRuntime* rt, HcsAstList* args) {
    if (args->count < 7) return value_null();
    
    float x = (float)value_to_number(runtime_eval(rt, args->items[0]));
    float y = (float)value_to_number(runtime_eval(rt, args->items[1]));
    float w = (float)value_to_number(runtime_eval(rt, args->items[2]));
    float h = (float)value_to_number(runtime_eval(rt, args->items[3]));
    uint32_t c1 = (uint32_t)value_to_number(runtime_eval(rt, args->items[4]));
    uint32_t c2 = (uint32_t)value_to_number(runtime_eval(rt, args->items[5]));
    bool vertical = value_is_truthy(runtime_eval(rt, args->items[6]));
    
    hal_gpu_draw_gradient(x, y, w, h, c1, c2, vertical);
    return value_null();
}

HcsValue* halgui_gpu_draw_shadow_fn(HcsRuntime* rt, HcsAstList* args) {
    if (args->count < 6) return value_null();
    
    float x = (float)value_to_number(runtime_eval(rt, args->items[0]));
    float y = (float)value_to_number(runtime_eval(rt, args->items[1]));
    float w = (float)value_to_number(runtime_eval(rt, args->items[2]));
    float h = (float)value_to_number(runtime_eval(rt, args->items[3]));
    float radius = (float)value_to_number(runtime_eval(rt, args->items[4]));
    int elevation = (int)value_to_number(runtime_eval(rt, args->items[5]));
    
    hal_gpu_draw_shadow(x, y, w, h, radius, (HalElevationLevel)elevation, 0xFF000000);
    return value_null();
}

HcsValue* halgui_gpu_draw_text_fn(HcsRuntime* rt, HcsAstList* args) {
    if (args->count < 5) return value_null();
    
    char* text = value_to_string(runtime_eval(rt, args->items[0]));
    float x = (float)value_to_number(runtime_eval(rt, args->items[1]));
    float y = (float)value_to_number(runtime_eval(rt, args->items[2]));
    float size = (float)value_to_number(runtime_eval(rt, args->items[3]));
    uint32_t color = (uint32_t)value_to_number(runtime_eval(rt, args->items[4]));
    
    hal_gpu_draw_text(text, x, y, size, color);
    free(text);
    return value_null();
}

/* ============================================
   Utility Functions
   ============================================ */

HcsValue* halgui_gpu_get_width_fn(HcsRuntime* rt, HcsAstList* args) {
    (void)rt; (void)args;
    return value_number(g_gpu_rt.width);
}

HcsValue* halgui_gpu_get_height_fn(HcsRuntime* rt, HcsAstList* args) {
    (void)rt; (void)args;
    return value_number(g_gpu_rt.height);
}

HcsValue* halgui_gpu_set_vsync_fn(HcsRuntime* rt, HcsAstList* args) {
    if (args->count < 1) return value_null();
    bool enabled = value_is_truthy(runtime_eval(rt, args->items[0]));
    hal_gpu_set_vsync(enabled);
    return value_null();
}

/* ============================================
   Color Helpers
   ============================================ */

HcsValue* halgui_gpu_rgb_fn(HcsRuntime* rt, HcsAstList* args) {
    if (args->count < 3) return value_number(0xFF000000);
    
    int r = (int)value_to_number(runtime_eval(rt, args->items[0]));
    int g = (int)value_to_number(runtime_eval(rt, args->items[1]));
    int b = (int)value_to_number(runtime_eval(rt, args->items[2]));
    int a = args->count >= 4 ? (int)value_to_number(runtime_eval(rt, args->items[3])) : 255;
    
    uint32_t color = ((a & 0xFF) << 24) | ((r & 0xFF) << 16) | ((g & 0xFF) << 8) | (b & 0xFF);
    return value_number(color);
}
