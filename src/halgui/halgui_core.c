/*
 * HalGUI - Core Implementation
 * 
 * Window management, event loop, widget base
 */

#include "halgui.h"
#include "../halgui_runtime.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ============================================
   Global State
   ============================================ */

static struct {
    bool initialized;
    bool running;
    HalTheme* currentTheme;
    HalWindow* windows[64];
    int windowCount;
    HINSTANCE hInstance;
    HFONT defaultFont;
    float dpiScale;
} g_hal = {0};

/* ============================================
   Forward Declarations
   ============================================ */

static LRESULT CALLBACK hal_window_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static void hal_dispatch_event(HalWidget* widget, HalEvent* event);
static void hal_dispatch_mouse_event(HalWindow* window, HalEvent* event);

/* External render functions (defined in halgui_render.c) */
extern void hal_render_window(HalWindow* window);
extern void hal_render_widget(HalWindow* window, HalWidget* widget, HDC hdc);
extern bool hal_gdiplus_init(void);
extern void hal_gdiplus_shutdown(void);

/* ============================================
   Initialization
   ============================================ */

bool hal_init(void) {
    if (g_hal.initialized) return true;
    
    g_hal.hInstance = GetModuleHandle(NULL);
    
    // Enable DPI awareness for proper title bar rendering
    // Try SetProcessDpiAwarenessContext first (Windows 10 1703+)
    typedef BOOL (WINAPI *PFN_SetProcessDpiAwarenessContext)(DPI_AWARENESS_CONTEXT);
    HMODULE hUser32 = GetModuleHandleW(L"user32.dll");
    if (hUser32) {
        PFN_SetProcessDpiAwarenessContext pfnSetDpiContext = 
            (PFN_SetProcessDpiAwarenessContext)GetProcAddress(hUser32, "SetProcessDpiAwarenessContext");
        if (pfnSetDpiContext) {
            pfnSetDpiContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
        } else {
            // Fallback to SetProcessDPIAware (Windows Vista+)
            typedef BOOL (WINAPI *PFN_SetProcessDPIAware)(void);
            PFN_SetProcessDPIAware pfnSetDPIAware = 
                (PFN_SetProcessDPIAware)GetProcAddress(hUser32, "SetProcessDPIAware");
            if (pfnSetDPIAware) {
                pfnSetDPIAware();
            }
        }
    }
    
    // Initialize GDI+ for anti-aliased rendering
    if (!hal_gdiplus_init()) {
        return false;
    }
    
    // Register window class
    WNDCLASSEXW wc = {0};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wc.lpfnWndProc = hal_window_proc;
    wc.hInstance = g_hal.hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = NULL;  // We handle painting
    wc.lpszClassName = L"HalGUIWindow";
    
    // Try to load icon from embedded resources first (for built applications)
    HICON hIcon = LoadIconW(g_hal.hInstance, MAKEINTRESOURCEW(1));  // IDI_APPICON = 1
    
    if (!hIcon) {
        // Fallback: Load default icon from logo directory
        wchar_t iconPath[MAX_PATH];
        GetModuleFileNameW(NULL, iconPath, MAX_PATH);
        wchar_t* lastSlash = wcsrchr(iconPath, L'\\');
        if (lastSlash) {
            *(lastSlash + 1) = L'\0';
            wcscat_s(iconPath, MAX_PATH, L"logo\\halcyon.ico");
        }
        
        // Try to load icon from file
        hIcon = (HICON)LoadImageW(NULL, iconPath, IMAGE_ICON, 0, 0, LR_LOADFROMFILE | LR_DEFAULTSIZE);
        
        if (!hIcon) {
            // Try alternative path (relative to executable - for dist folder)
            GetModuleFileNameW(NULL, iconPath, MAX_PATH);
            lastSlash = wcsrchr(iconPath, L'\\');
            if (lastSlash) {
                *(lastSlash + 1) = L'\0';
                wcscat_s(iconPath, MAX_PATH, L"halcyon.ico");
            }
            hIcon = (HICON)LoadImageW(NULL, iconPath, IMAGE_ICON, 0, 0, LR_LOADFROMFILE | LR_DEFAULTSIZE);
        }
        
        if (!hIcon) {
            // Try parent directory (for development)
            GetModuleFileNameW(NULL, iconPath, MAX_PATH);
            lastSlash = wcsrchr(iconPath, L'\\');
            if (lastSlash) {
                *(lastSlash + 1) = L'\0';
                wcscat_s(iconPath, MAX_PATH, L"..\\logo\\halcyon.ico");
            }
            hIcon = (HICON)LoadImageW(NULL, iconPath, IMAGE_ICON, 0, 0, LR_LOADFROMFILE | LR_DEFAULTSIZE);
        }
    }
    
    wc.hIcon = hIcon ? hIcon : LoadIcon(NULL, IDI_APPLICATION);
    wc.hIconSm = hIcon ? hIcon : LoadIcon(NULL, IDI_APPLICATION);
    
    if (!RegisterClassExW(&wc)) {
        return false;
    }
    
    // Get DPI scale
    HDC hdc = GetDC(NULL);
    g_hal.dpiScale = GetDeviceCaps(hdc, LOGPIXELSX) / 96.0f;
    ReleaseDC(NULL, hdc);
    
    // Create default font
    g_hal.defaultFont = CreateFontW(
        (int)(14 * g_hal.dpiScale), 0, 0, 0,
        FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE,
        L"Segoe UI"
    );
    
    // Set default theme
    g_hal.currentTheme = &HAL_THEME_DARK;
    
    g_hal.initialized = true;
    return true;
}

void hal_shutdown(void) {
    if (!g_hal.initialized) return;
    
    // Destroy all windows
    for (int i = 0; i < g_hal.windowCount; i++) {
        if (g_hal.windows[i]) {
            hal_window_destroy(g_hal.windows[i]);
        }
    }
    
    if (g_hal.defaultFont) {
        DeleteObject(g_hal.defaultFont);
    }
    
    UnregisterClassW(L"HalGUIWindow", g_hal.hInstance);
    
    // Shutdown GDI+
    hal_gdiplus_shutdown();
    
    g_hal.initialized = false;
}

void hal_set_theme(HalTheme* theme) {
    g_hal.currentTheme = theme ? theme : &HAL_THEME_DARK;
    
    // Invalidate all windows
    for (int i = 0; i < g_hal.windowCount; i++) {
        if (g_hal.windows[i]) {
            InvalidateRect(g_hal.windows[i]->hwnd, NULL, TRUE);
        }
    }
}

HalTheme* hal_get_theme(void) {
    return g_hal.currentTheme;
}

float hal_get_dpi_scale(void) {
    return g_hal.dpiScale;
}

/* ============================================
   Main Loop
   ============================================ */

void hal_run(void) {
    g_hal.running = true;
    
    MSG msg;
    while (g_hal.running) {
        /* Process all pending messages */
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                g_hal.running = false;
                break;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        
        /* Call update callback for periodic updates */
        halgui_process_updates();
        
        /* Small sleep to avoid 100% CPU usage */
        Sleep(16);  /* ~60 FPS */
    }
    
    /* Force process termination when main loop exits */
    ExitProcess(0);
}

void hal_quit(void) {
    g_hal.running = false;
    PostQuitMessage(0);
}

bool hal_process_events(void) {
    MSG msg;
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) {
            g_hal.running = false;
            return false;
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return g_hal.running;
}

/* ============================================
   Window Management
   ============================================ */

HalWindow* hal_window_create(const char* title, int width, int height) {
    return hal_window_create_ex(title, CW_USEDEFAULT, CW_USEDEFAULT, width, height, HAL_WINDOW_NORMAL);
}

HalWindow* hal_window_create_ex(const char* title, int x, int y, int width, int height, HalWindowStyle style) {
    if (!g_hal.initialized) {
        if (!hal_init()) return NULL;
    }
    
    HalWindow* window = (HalWindow*)calloc(1, sizeof(HalWindow));
    if (!window) return NULL;
    
    // Initialize base widget
    window->base.type = HAL_WIDGET_WINDOW;
    window->base.bounds.x = x;
    window->base.bounds.y = y;
    window->base.bounds.width = width;
    window->base.bounds.height = height;
    window->base.visible = false;
    window->base.enabled = true;
    window->base.opacity = 1.0f;
    window->base.layout = HAL_LAYOUT_NONE;
    
    // Window properties
    window->title = _strdup(title);
    window->style = style;
    window->resizable = true;
    window->maximizable = true;
    window->minimizable = true;
    window->closable = true;
    window->theme = g_hal.currentTheme;
    
    // Determine window style flags
    DWORD dwStyle = WS_OVERLAPPEDWINDOW;
    DWORD dwExStyle = WS_EX_APPWINDOW;
    
    if (style == HAL_WINDOW_FRAMELESS) {
        dwStyle = WS_POPUP | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;
    } else if (style == HAL_WINDOW_DIALOG) {
        dwStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU;
        window->resizable = false;
        window->maximizable = false;
        window->minimizable = false;
    } else if (style == HAL_WINDOW_POPUP) {
        dwStyle = WS_POPUP;
        dwExStyle = WS_EX_TOOLWINDOW;
    }
    
    // Scale dimensions
    int scaledWidth = (int)(width * g_hal.dpiScale);
    int scaledHeight = (int)(height * g_hal.dpiScale);
    
    // Adjust for window chrome
    RECT rect = {0, 0, scaledWidth, scaledHeight};
    AdjustWindowRectEx(&rect, dwStyle, FALSE, dwExStyle);
    
    // Convert title to wide string using UTF-8
    int titleLen = MultiByteToWideChar(CP_UTF8, 0, title, -1, NULL, 0);
    wchar_t* wideTitle = NULL;
    
    if (titleLen > 0) {
        wideTitle = (wchar_t*)malloc(titleLen * sizeof(wchar_t));
        MultiByteToWideChar(CP_UTF8, 0, title, -1, wideTitle, titleLen);
    } else {
        // Fallback: convert each byte to wchar
        titleLen = (int)strlen(title) + 1;
        wideTitle = (wchar_t*)malloc(titleLen * sizeof(wchar_t));
        for (int i = 0; i < titleLen; i++) {
            wideTitle[i] = (wchar_t)(unsigned char)title[i];
        }
    }
    
    // Create window
    window->hwnd = CreateWindowExW(
        dwExStyle,
        L"HalGUIWindow",
        wideTitle,
        dwStyle,
        x == CW_USEDEFAULT ? CW_USEDEFAULT : (int)(x * g_hal.dpiScale),
        y == CW_USEDEFAULT ? CW_USEDEFAULT : (int)(y * g_hal.dpiScale),
        rect.right - rect.left,
        rect.bottom - rect.top,
        NULL,
        NULL,
        g_hal.hInstance,
        window
    );
    
    free(wideTitle);
    
    if (!window->hwnd) {
        free(window->title);
        free(window);
        return NULL;
    }
    
    // Create back buffer for double buffering
    window->hdc = GetDC(window->hwnd);
    window->backBufferDC = CreateCompatibleDC(window->hdc);
    window->backBuffer = CreateCompatibleBitmap(window->hdc, scaledWidth, scaledHeight);
    SelectObject(window->backBufferDC, window->backBuffer);
    SelectObject(window->backBufferDC, g_hal.defaultFont);
    
    // Store window reference
    SetWindowLongPtr(window->hwnd, GWLP_USERDATA, (LONG_PTR)window);
    
    // Add to window list
    if (g_hal.windowCount < 64) {
        g_hal.windows[g_hal.windowCount++] = window;
    }
    
    return window;
}

void hal_window_destroy(HalWindow* window) {
    if (!window) return;
    
    // Remove from window list
    for (int i = 0; i < g_hal.windowCount; i++) {
        if (g_hal.windows[i] == window) {
            g_hal.windows[i] = g_hal.windows[--g_hal.windowCount];
            break;
        }
    }
    
    // Destroy children
    for (int i = 0; i < window->base.childCount; i++) {
        hal_widget_destroy(window->base.children[i]);
    }
    
    // Clean up GDI resources
    if (window->backBuffer) DeleteObject(window->backBuffer);
    if (window->backBufferDC) DeleteDC(window->backBufferDC);
    if (window->hdc) ReleaseDC(window->hwnd, window->hdc);
    
    // Destroy native window
    if (window->hwnd) DestroyWindow(window->hwnd);
    
    free(window->title);
    free(window);
}

void hal_window_show(HalWindow* window) {
    if (!window) return;
    
    // Initialize native widgets (textarea, etc.) before showing
    hal_init_native_widgets(window);
    
    window->base.visible = true;
    ShowWindow(window->hwnd, SW_SHOW);
    UpdateWindow(window->hwnd);
}

void hal_window_hide(HalWindow* window) {
    if (!window) return;
    window->base.visible = false;
    ShowWindow(window->hwnd, SW_HIDE);
}

void hal_window_close(HalWindow* window) {
    if (!window) return;
    PostMessage(window->hwnd, WM_CLOSE, 0, 0);
}

void hal_window_maximize(HalWindow* window) {
    if (!window) return;
    ShowWindow(window->hwnd, SW_MAXIMIZE);
    window->isMaximized = true;
}

void hal_window_minimize(HalWindow* window) {
    if (!window) return;
    ShowWindow(window->hwnd, SW_MINIMIZE);
    window->isMinimized = true;
}

void hal_window_restore(HalWindow* window) {
    if (!window) return;
    ShowWindow(window->hwnd, SW_RESTORE);
    window->isMaximized = false;
    window->isMinimized = false;
}

void hal_window_set_title(HalWindow* window, const char* title) {
    if (!window) return;
    
    free(window->title);
    window->title = _strdup(title);
    
    // Convert title to wide string using UTF-8
    int titleLen = MultiByteToWideChar(CP_UTF8, 0, title, -1, NULL, 0);
    wchar_t* wideTitle = NULL;
    
    if (titleLen > 0) {
        wideTitle = (wchar_t*)malloc(titleLen * sizeof(wchar_t));
        MultiByteToWideChar(CP_UTF8, 0, title, -1, wideTitle, titleLen);
    } else {
        titleLen = (int)strlen(title) + 1;
        wideTitle = (wchar_t*)malloc(titleLen * sizeof(wchar_t));
        for (int i = 0; i < titleLen; i++) {
            wideTitle[i] = (wchar_t)(unsigned char)title[i];
        }
    }
    
    SetWindowTextW(window->hwnd, wideTitle);
    free(wideTitle);
}

void hal_window_center(HalWindow* window) {
    if (!window) return;
    
    RECT rect;
    GetWindowRect(window->hwnd, &rect);
    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;
    
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    
    int x = (screenWidth - width) / 2;
    int y = (screenHeight - height) / 2;
    
    SetWindowPos(window->hwnd, NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

void hal_window_set_theme(HalWindow* window, HalTheme* theme) {
    if (!window) return;
    window->theme = theme ? theme : g_hal.currentTheme;
    InvalidateRect(window->hwnd, NULL, TRUE);
}

/* ============================================
   Window Procedure
   ============================================ */

static LRESULT CALLBACK hal_window_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    HalWindow* window = (HalWindow*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    
    switch (msg) {
        case WM_CREATE: {
            CREATESTRUCT* cs = (CREATESTRUCT*)lParam;
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)cs->lpCreateParams);
            return 0;
        }
        
        case WM_PAINT: {
            if (window) {
                PAINTSTRUCT ps;
                BeginPaint(hwnd, &ps);
                hal_render_window(window);
                
                // Blit back buffer to screen
                RECT rect;
                GetClientRect(hwnd, &rect);
                BitBlt(ps.hdc, 0, 0, rect.right, rect.bottom,
                       window->backBufferDC, 0, 0, SRCCOPY);
                
                EndPaint(hwnd, &ps);
            }
            return 0;
        }
        
        case WM_SIZE: {
            if (window) {
                int width = LOWORD(lParam);
                int height = HIWORD(lParam);
                
                window->base.bounds.width = (int)(width / g_hal.dpiScale);
                window->base.bounds.height = (int)(height / g_hal.dpiScale);
                
                // Recreate back buffer
                if (window->backBuffer) DeleteObject(window->backBuffer);
                window->backBuffer = CreateCompatibleBitmap(window->hdc, width, height);
                SelectObject(window->backBufferDC, window->backBuffer);
                
                // Apply responsive layout to all children
                hal_window_apply_layout(window);
                
                // Dispatch resize event
                HalEvent event = {0};
                event.type = HAL_EVENT_RESIZE;
                event.target = (HalWidget*)window;
                hal_dispatch_event((HalWidget*)window, &event);
                
                InvalidateRect(hwnd, NULL, FALSE);
            }
            return 0;
        }
        
        case WM_CLOSE: {
            if (window) {
                HalEvent event = {0};
                event.type = HAL_EVENT_CLOSE;
                event.target = (HalWidget*)window;
                event.propagate = true;
                hal_dispatch_event((HalWidget*)window, &event);
                
                if (!event.handled) {
                    hal_window_destroy(window);
                    
                    // Quit if no more windows
                    if (g_hal.windowCount == 0) {
                        hal_quit();
                    }
                }
            }
            return 0;
        }
        
        case WM_DESTROY: {
            return 0;
        }
        
        case WM_ERASEBKGND: {
            return 1;  // We handle background painting
        }
        
        case WM_MOUSEMOVE:
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
        case WM_LBUTTONDBLCLK: {
            if (window) {
                HalEvent event = {0};
                // Use raw pixel coordinates - widgets store logical coords, 
                // but we compare against scaled rendering positions
                int rawX = (short)LOWORD(lParam);
                int rawY = (short)HIWORD(lParam);
                
                // Convert to logical coordinates (same as widget bounds)
                event.mouseX = (int)(rawX / g_hal.dpiScale);
                event.mouseY = (int)(rawY / g_hal.dpiScale);
                event.ctrlKey = (wParam & MK_CONTROL) != 0;
                event.shiftKey = (wParam & MK_SHIFT) != 0;
                
                switch (msg) {
                    case WM_MOUSEMOVE:     event.type = HAL_EVENT_MOUSE_MOVE; break;
                    case WM_LBUTTONDOWN:   event.type = HAL_EVENT_MOUSE_DOWN; event.button = 0; break;
                    case WM_LBUTTONUP:     event.type = HAL_EVENT_MOUSE_UP; event.button = 0; break;
                    case WM_RBUTTONDOWN:   event.type = HAL_EVENT_MOUSE_DOWN; event.button = 2; break;
                    case WM_RBUTTONUP:     event.type = HAL_EVENT_MOUSE_UP; event.button = 2; break;
                    case WM_LBUTTONDBLCLK: event.type = HAL_EVENT_DOUBLE_CLICK; break;
                }
                
                hal_dispatch_mouse_event(window, &event);
            }
            return 0;
        }
        
        case WM_KEYDOWN:
        case WM_KEYUP: {
            if (window) {
                HalEvent event = {0};
                event.type = (msg == WM_KEYDOWN) ? HAL_EVENT_KEY_DOWN : HAL_EVENT_KEY_UP;
                event.keyCode = (int)wParam;
                event.ctrlKey = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
                event.shiftKey = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
                event.altKey = (GetKeyState(VK_MENU) & 0x8000) != 0;
                event.target = (HalWidget*)window;
                event.propagate = true;
                hal_dispatch_event((HalWidget*)window, &event);
            }
            return 0;
        }
        
        case WM_CHAR: {
            if (window) {
                HalEvent event = {0};
                event.type = HAL_EVENT_KEY_PRESS;
                event.character = (wchar_t)wParam;
                event.target = (HalWidget*)window;
                event.propagate = true;
                hal_dispatch_event((HalWidget*)window, &event);
            }
            return 0;
        }
        
        case WM_CTLCOLOREDIT: {
            // Handle textarea and input theming
            if (window) {
                HDC hdcEdit = (HDC)wParam;
                HWND hwndEdit = (HWND)lParam;
                
                // Find the widget that owns this HWND
                for (int i = 0; i < window->base.childCount; i++) {
                    HalWidget* child = window->base.children[i];
                    if (!child) continue;
                    
                    if (child->type == HAL_WIDGET_TEXTAREA) {
                        HWND textareaHwnd = hal_textarea_get_hwnd(child);
                        if (textareaHwnd == hwndEdit) {
                            HBRUSH brush = hal_textarea_get_brush(child, hdcEdit);
                            if (brush) {
                                return (LRESULT)brush;
                            }
                        }
                    }
                    else if (child->type == HAL_WIDGET_INPUT) {
                        HWND inputHwnd = hal_input_get_hwnd(child);
                        if (inputHwnd == hwndEdit) {
                            HBRUSH brush = hal_input_get_brush(child, hdcEdit);
                            if (brush) {
                                return (LRESULT)brush;
                            }
                        }
                    }
                }
            }
            break;
        }
    }
    
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

/* ============================================
   Hit Testing
   ============================================ */

static bool hal_point_in_rect(int px, int py, int rx, int ry, int rw, int rh) {
    return px >= rx && px < rx + rw && py >= ry && py < ry + rh;
}

// Simple hit test - check direct children of window only (no nesting offset issues)
static HalWidget* hal_hit_test_simple(HalWindow* window, int x, int y) {
    if (!window) return NULL;
    
    // Check children in reverse order (top-most first)
    for (int i = window->base.childCount - 1; i >= 0; i--) {
        HalWidget* child = window->base.children[i];
        if (!child || !child->visible) continue;
        
        // Check if point is inside this widget
        if (hal_point_in_rect(x, y, child->bounds.x, child->bounds.y, 
                              child->bounds.width, child->bounds.height)) {
            return child;
        }
    }
    
    return NULL;
}

/* ============================================
   Event Dispatch
   ============================================ */

static void hal_dispatch_event(HalWidget* widget, HalEvent* event) {
    if (!widget || !event) return;
    
    event->currentTarget = widget;
    
    // Call registered handlers
    for (int i = 0; i < widget->eventCount; i++) {
        if (widget->events[i].type == event->type) {
            widget->events[i].handler(widget, event, widget->events[i].userData);
            if (event->handled) return;
        }
    }
    
    // Propagate to parent if not handled
    if (!event->handled && widget->parent) {
        hal_dispatch_event(widget->parent, event);
    }
}

static HalWidget* g_activeWidget = NULL;  // Track which widget was pressed
HalWidget* g_draggingSlider = NULL; // Track slider being dragged (non-static for external access)

// Helper function to update slider value from mouse position
static void hal_update_slider_value(HalWidget* slider, int mouseX, HalWindow* window) {
    if (!slider || slider->type != HAL_WIDGET_SLIDER) return;
    
    // Calculate relative position within slider bounds
    int sliderX = slider->bounds.x;
    int sliderW = slider->bounds.width;
    
    // Clamp mouse position to slider bounds
    float relX = (float)(mouseX - sliderX) / (float)sliderW;
    if (relX < 0.0f) relX = 0.0f;
    if (relX > 1.0f) relX = 1.0f;
    
    // Update slider value
    slider->animProgress = relX;
    
    // Update the actual value in slider data
    typedef struct { int min; int max; int value; } HalSliderData;
    HalSliderData* data = (HalSliderData*)slider->data;
    if (data) {
        int newValue = data->min + (int)(relX * (data->max - data->min));
        if (newValue != data->value) {
            data->value = newValue;
            
            // Fire change event
            HalEvent changeEvent = {0};
            changeEvent.type = HAL_EVENT_CHANGE;
            changeEvent.target = slider;
            changeEvent.currentTarget = slider;
            changeEvent.propagate = true;
            hal_dispatch_event(slider, &changeEvent);
        }
    }
    
    // Invalidate slider area
    float scale = hal_get_dpi_scale();
    RECT rect = {
        (LONG)(slider->bounds.x * scale),
        (LONG)(slider->bounds.y * scale),
        (LONG)((slider->bounds.x + slider->bounds.width) * scale),
        (LONG)((slider->bounds.y + slider->bounds.height) * scale)
    };
    InvalidateRect(window->hwnd, &rect, FALSE);
}

static void hal_dispatch_mouse_event(HalWindow* window, HalEvent* event) {
    if (!window) return;
    
    // If dragging a slider, continue tracking it even if mouse moves outside
    if (g_draggingSlider && event->type == HAL_EVENT_MOUSE_MOVE) {
        hal_update_slider_value(g_draggingSlider, event->mouseX, window);
        return;
    }
    
    // Find widget under mouse using simple hit test
    HalWidget* target = hal_hit_test_simple(window, event->mouseX, event->mouseY);
    
    if (!target) {
        target = (HalWidget*)window;
    }
    
    event->target = target;
    event->currentTarget = target;
    event->propagate = true;
    
    // Handle mouse events
    if (event->type == HAL_EVENT_MOUSE_DOWN) {
        // Store active widget for click detection
        g_activeWidget = target;
        target->state |= HAL_STATE_ACTIVE;
        
        // Handle slider drag start
        if (target->type == HAL_WIDGET_SLIDER) {
            g_draggingSlider = target;
            SetCapture(window->hwnd);  // Capture mouse for dragging
            hal_update_slider_value(target, event->mouseX, window);
        }
        
        // Only invalidate the specific widget area
        float scale = hal_get_dpi_scale();
        RECT rect = {
            (LONG)(target->bounds.x * scale),
            (LONG)(target->bounds.y * scale),
            (LONG)((target->bounds.x + target->bounds.width) * scale),
            (LONG)((target->bounds.y + target->bounds.height) * scale)
        };
        InvalidateRect(window->hwnd, &rect, FALSE);
    }
    else if (event->type == HAL_EVENT_MOUSE_UP) {
        // Handle slider drag end
        if (g_draggingSlider) {
            ReleaseCapture();  // Release mouse capture
            g_draggingSlider = NULL;
        }
        
        // Clear active state
        if (g_activeWidget) {
            g_activeWidget->state &= ~HAL_STATE_ACTIVE;
            // Only invalidate the specific widget area
            float scale = hal_get_dpi_scale();
            RECT rect = {
                (LONG)(g_activeWidget->bounds.x * scale),
                (LONG)(g_activeWidget->bounds.y * scale),
                (LONG)((g_activeWidget->bounds.x + g_activeWidget->bounds.width) * scale),
                (LONG)((g_activeWidget->bounds.y + g_activeWidget->bounds.height) * scale)
            };
            InvalidateRect(window->hwnd, &rect, FALSE);
        }
        
        // Fire click event only if mouse up on same widget as mouse down
        // But NOT for sliders (they use change event instead)
        if (target == g_activeWidget && g_activeWidget != NULL && 
            target->type != HAL_WIDGET_SLIDER) {
            // Toggle checkbox/toggle state BEFORE firing click event
            if (target->type == HAL_WIDGET_CHECKBOX || target->type == HAL_WIDGET_TOGGLE) {
                if (target->state & HAL_STATE_CHECKED) {
                    target->state &= ~HAL_STATE_CHECKED;
                } else {
                    target->state |= HAL_STATE_CHECKED;
                }
            }
            
            // Fire click event
            HalEvent clickEvent = *event;
            clickEvent.type = HAL_EVENT_CLICK;
            clickEvent.target = target;
            clickEvent.currentTarget = target;
            clickEvent.propagate = true;
            hal_dispatch_event(target, &clickEvent);
        }
        
        g_activeWidget = NULL;
    }
    else if (event->type == HAL_EVENT_MOUSE_MOVE) {
        // Update hover state
        static HalWidget* lastHover = NULL;
        
        if (lastHover && lastHover != target) {
            lastHover->state &= ~HAL_STATE_HOVER;
            // Only invalidate the specific widget area, not the whole window
            float scale = hal_get_dpi_scale();
            RECT rect = {
                (LONG)(lastHover->bounds.x * scale),
                (LONG)(lastHover->bounds.y * scale),
                (LONG)((lastHover->bounds.x + lastHover->bounds.width) * scale),
                (LONG)((lastHover->bounds.y + lastHover->bounds.height) * scale)
            };
            InvalidateRect(window->hwnd, &rect, FALSE);
        }
        
        if (target && target != (HalWidget*)window) {
            target->state |= HAL_STATE_HOVER;
            lastHover = target;
            // Only invalidate the specific widget area, not the whole window
            float scale = hal_get_dpi_scale();
            RECT rect = {
                (LONG)(target->bounds.x * scale),
                (LONG)(target->bounds.y * scale),
                (LONG)((target->bounds.x + target->bounds.width) * scale),
                (LONG)((target->bounds.y + target->bounds.height) * scale)
            };
            InvalidateRect(window->hwnd, &rect, FALSE);
        } else {
            lastHover = NULL;
        }
    }
    
    hal_dispatch_event(target, event);
}

void hal_widget_on(HalWidget* widget, HalEventType type, HalEventHandler handler, void* userData) {
    if (!widget || !handler) return;
    if (widget->eventCount >= HAL_MAX_EVENT_HANDLERS) return;
    
    widget->events[widget->eventCount].type = type;
    widget->events[widget->eventCount].handler = handler;
    widget->events[widget->eventCount].userData = userData;
    widget->eventCount++;
}

void hal_widget_off(HalWidget* widget, HalEventType type, HalEventHandler handler) {
    if (!widget) return;
    
    for (int i = 0; i < widget->eventCount; i++) {
        if (widget->events[i].type == type && widget->events[i].handler == handler) {
            // Remove by shifting
            for (int j = i; j < widget->eventCount - 1; j++) {
                widget->events[j] = widget->events[j + 1];
            }
            widget->eventCount--;
            return;
        }
    }
}

/* ============================================
   Screen Info
   ============================================ */

HalSize hal_screen_get_size(void) {
    HalSize size;
    size.width = GetSystemMetrics(SM_CXSCREEN);
    size.height = GetSystemMetrics(SM_CYSCREEN);
    return size;
}

HalRect hal_screen_get_work_area(void) {
    RECT rect;
    SystemParametersInfo(SPI_GETWORKAREA, 0, &rect, 0);
    
    HalRect result;
    result.x = rect.left;
    result.y = rect.top;
    result.width = rect.right - rect.left;
    result.height = rect.bottom - rect.top;
    return result;
}

/* ============================================
   Responsive Layout System
   ============================================ */

void hal_widget_set_constraints(HalWidget* widget, float leftPct, float topPct, float widthPct, float heightPct) {
    if (!widget) return;
    
    // Store initial bounds if not set
    if (!widget->hasConstraints) {
        widget->initialBounds = widget->bounds;
    }
    
    widget->constraints.leftPercent = leftPct;
    widget->constraints.topPercent = topPct;
    widget->constraints.widthPercent = widthPct;
    widget->constraints.heightPercent = heightPct;
    widget->constraints.usePercent = true;
    widget->hasConstraints = true;
}

void hal_widget_set_anchors(HalWidget* widget, HalAnchor anchors) {
    if (!widget) return;
    
    // Store initial bounds if not set
    if (!widget->hasConstraints) {
        widget->initialBounds = widget->bounds;
    }
    
    widget->constraints.anchors = anchors;
    widget->hasConstraints = true;
}

void hal_widget_set_margins(HalWidget* widget, int left, int top, int right, int bottom) {
    if (!widget) return;
    
    widget->constraints.leftMargin = left;
    widget->constraints.topMargin = top;
    widget->constraints.rightMargin = right;
    widget->constraints.bottomMargin = bottom;
}

static void hal_apply_widget_constraints(HalWidget* widget, int parentWidth, int parentHeight, int origParentWidth, int origParentHeight) {
    if (!widget || !widget->hasConstraints) return;
    
    HalLayoutConstraints* c = &widget->constraints;
    HalRect* initial = &widget->initialBounds;
    
    if (c->usePercent) {
        // Percentage-based layout
        widget->bounds.x = (int)(parentWidth * c->leftPercent) + c->leftMargin;
        widget->bounds.y = (int)(parentHeight * c->topPercent) + c->topMargin;
        widget->bounds.width = (int)(parentWidth * c->widthPercent) - c->leftMargin - c->rightMargin;
        widget->bounds.height = (int)(parentHeight * c->heightPercent) - c->topMargin - c->bottomMargin;
    } else if (c->anchors != HAL_ANCHOR_NONE) {
        // Anchor-based layout
        int deltaW = parentWidth - origParentWidth;
        int deltaH = parentHeight - origParentHeight;
        
        // Calculate new position and size based on anchors
        int newX = initial->x;
        int newY = initial->y;
        int newW = initial->width;
        int newH = initial->height;
        
        if (c->anchors & HAL_ANCHOR_LEFT) {
            // Left edge stays fixed
        } else if (c->anchors & HAL_ANCHOR_RIGHT) {
            // Right edge stays fixed, move left edge
            newX = initial->x + deltaW;
        }
        
        if (c->anchors & HAL_ANCHOR_TOP) {
            // Top edge stays fixed
        } else if (c->anchors & HAL_ANCHOR_BOTTOM) {
            // Bottom edge stays fixed, move top edge
            newY = initial->y + deltaH;
        }
        
        // If both left and right are anchored, stretch width
        if ((c->anchors & HAL_ANCHOR_LEFT) && (c->anchors & HAL_ANCHOR_RIGHT)) {
            newW = initial->width + deltaW;
        }
        
        // If both top and bottom are anchored, stretch height
        if ((c->anchors & HAL_ANCHOR_TOP) && (c->anchors & HAL_ANCHOR_BOTTOM)) {
            newH = initial->height + deltaH;
        }
        
        widget->bounds.x = newX;
        widget->bounds.y = newY;
        widget->bounds.width = newW > 10 ? newW : 10;
        widget->bounds.height = newH > 10 ? newH : 10;
    }
    
    // Update native widget bounds if needed
    if (widget->type == HAL_WIDGET_TEXTAREA && widget->nativeHandle) {
        hal_textarea_update_bounds(widget, g_hal.dpiScale);
    }
}

void hal_window_apply_layout(HalWindow* window) {
    if (!window) return;
    
    int parentWidth = window->base.bounds.width;
    int parentHeight = window->base.bounds.height;
    
    // Use stored original window size for responsive layout
    int origWidth = window->restoreBounds.width;
    int origHeight = window->restoreBounds.height;
    
    // If not set, use current size
    if (origWidth == 0) origWidth = parentWidth;
    if (origHeight == 0) origHeight = parentHeight;
    
    for (int i = 0; i < window->base.childCount; i++) {
        HalWidget* child = window->base.children[i];
        if (child && child->hasConstraints) {
            hal_apply_widget_constraints(child, parentWidth, parentHeight, origWidth, origHeight);
        }
    }
}

void hal_window_enable_responsive(HalWindow* window, bool enable) {
    if (!window) return;
    
    // Store initial bounds for all children
    for (int i = 0; i < window->base.childCount; i++) {
        HalWidget* child = window->base.children[i];
        if (child && !child->hasConstraints) {
            child->initialBounds = child->bounds;
            // Default: anchor all edges (stretch with window)
            child->constraints.anchors = HAL_ANCHOR_ALL;
            child->hasConstraints = enable;
        }
    }
}
