/*
 * HalForms - Core Implementation
 * Windows Forms-like UI Framework for HalcyonScript
 * Compatible with Windows XP/7/10/11
 */

#include "halforms.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Note: Link with comctl32.lib, gdi32.lib, user32.lib - handled in build script */

/* Helper function to convert UTF-8 string to Wide string */
static wchar_t* utf8_to_wide(const char* utf8) {
    if (!utf8) return NULL;
    int len = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, NULL, 0);
    if (len == 0) return NULL;
    wchar_t* wide = (wchar_t*)malloc(len * sizeof(wchar_t));
    if (!wide) return NULL;
    MultiByteToWideChar(CP_UTF8, 0, utf8, -1, wide, len);
    return wide;
}

/* ============================================
   Global State
   ============================================ */

struct {
    bool initialized;
    HINSTANCE hInstance;
    HFONT defaultFont;
    HalForm* mainForm;
    HalForm** forms;
    int formCount;
    int formCapacity;
    int nextControlId;
    bool running;
} g_halforms = {0};

/* ============================================
   Forward Declarations
   ============================================ */

static LRESULT CALLBACK HalFormsWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static void halforms_register_class(void);
static void halforms_apply_layout(HalForm* form);
static HalControl* halforms_find_control_by_hwnd(HalForm* form, HWND hwnd);

/* ============================================
   Initialization
   ============================================ */

bool halforms_init(void) {
    if (g_halforms.initialized) return true;
    
    g_halforms.hInstance = GetModuleHandle(NULL);
    g_halforms.nextControlId = 1000;
    
    /* Enable modern visual styles (Windows XP and later) */
    INITCOMMONCONTROLSEX icc = {0};
    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_WIN95_CLASSES | ICC_BAR_CLASSES | ICC_LISTVIEW_CLASSES | 
                ICC_TREEVIEW_CLASSES | ICC_TAB_CLASSES | ICC_PROGRESS_CLASS |
                ICC_UPDOWN_CLASS | ICC_DATE_CLASSES | ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&icc);
    
    /* Register window class */
    halforms_register_class();
    
    /* Create default font - use Segoe UI for modern Windows look */
    const char* fontName = "Segoe UI";
    int fontSize = 9;
    
    /* Check Windows version for font selection */
    OSVERSIONINFO osvi = {0};
    osvi.dwOSVersionInfoSize = sizeof(osvi);
    if (GetVersionEx(&osvi)) {
        if (osvi.dwMajorVersion < 6) {
            /* Windows XP - use Tahoma */
            fontName = "Tahoma";
            fontSize = 8;
        }
    }
    
    g_halforms.defaultFont = CreateFontA(
        -MulDiv(fontSize, GetDeviceCaps(GetDC(NULL), LOGPIXELSY), 72),
        0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, fontName
    );
    
    /* Allocate form array */
    g_halforms.formCapacity = 16;
    g_halforms.forms = (HalForm**)calloc(g_halforms.formCapacity, sizeof(HalForm*));
    
    g_halforms.initialized = true;
    return true;
}

void halforms_shutdown(void) {
    if (!g_halforms.initialized) return;
    
    /* Destroy all forms */
    for (int i = 0; i < g_halforms.formCount; i++) {
        if (g_halforms.forms[i]) {
            halform_destroy(g_halforms.forms[i]);
        }
    }
    free(g_halforms.forms);
    
    if (g_halforms.defaultFont) {
        DeleteObject(g_halforms.defaultFont);
    }
    
    UnregisterClassW(L"HalFormsWindow", g_halforms.hInstance);
    
    g_halforms.initialized = false;
}

static void halforms_register_class(void) {
    WNDCLASSEXW wc = {0};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wc.lpfnWndProc = HalFormsWndProc;
    wc.hInstance = g_halforms.hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName = L"HalFormsWindow";
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
    
    RegisterClassExW(&wc);
}

/* ============================================
   Main Loop
   ============================================ */

void halforms_run(void) {
    g_halforms.running = true;
    
    MSG msg;
    while (g_halforms.running && GetMessage(&msg, NULL, 0, 0)) {
        /* Handle MDI accelerators if needed */
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

void halforms_exit(int code) {
    g_halforms.running = false;
    PostQuitMessage(code);
}

/* ============================================
   Form Functions
   ============================================ */

HalForm* halform_create(const char* title, int width, int height) {
    return halform_create_ex(title, CW_USEDEFAULT, CW_USEDEFAULT, width, height, HALFORM_NORMAL);
}

HalForm* halform_create_ex(const char* title, int x, int y, int width, int height, HalFormStyle style) {
    if (!g_halforms.initialized) {
        if (!halforms_init()) return NULL;
    }
    
    HalForm* form = (HalForm*)calloc(1, sizeof(HalForm));
    if (!form) return NULL;
    
    form->base.type = HALCTRL_PANEL;
    form->base.name = _strdup(title);
    form->base.text = _strdup(title);
    form->base.x = x;
    form->base.y = y;
    form->base.width = width;
    form->base.height = height;
    form->base.visible = false;
    form->base.enabled = true;
    form->style = style;
    form->maximizeBox = true;
    form->minimizeBox = true;
    form->showInTaskbar = true;
    
    /* Determine window style */
    DWORD dwStyle = WS_OVERLAPPEDWINDOW;
    DWORD dwExStyle = 0;
    
    switch (style) {
        case HALFORM_DIALOG:
            dwStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME;
            dwExStyle = WS_EX_DLGMODALFRAME;
            break;
        case HALFORM_TOOL:
            dwStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME;
            dwExStyle = WS_EX_TOOLWINDOW;
            break;
        case HALFORM_MDI_PARENT:
            dwStyle = WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN;
            break;
        case HALFORM_MDI_CHILD:
            dwStyle = WS_CHILD | WS_VISIBLE | WS_OVERLAPPEDWINDOW;
            break;
        default:
            break;
    }
    
    /* Adjust window rect for client area */
    RECT rect = {0, 0, width, height};
    AdjustWindowRectEx(&rect, dwStyle, FALSE, dwExStyle);
    
    /* Convert title to wide string for Unicode support */
    wchar_t* wtitle = utf8_to_wide(title);
    
    /* Create window using Unicode version */
    form->hwnd = CreateWindowExW(
        dwExStyle,
        L"HalFormsWindow",
        wtitle ? wtitle : L"HalForms",
        dwStyle,
        x, y,
        rect.right - rect.left,
        rect.bottom - rect.top,
        NULL,
        NULL,
        g_halforms.hInstance,
        form
    );
    
    free(wtitle);
    
    if (!form->hwnd) {
        free(form->base.name);
        free(form->base.text);
        free(form);
        return NULL;
    }
    
    form->base.hwnd = form->hwnd;
    
    /* Set default font for form */
    SendMessage(form->hwnd, WM_SETFONT, (WPARAM)g_halforms.defaultFont, TRUE);
    
    /* Store form reference */
    SetWindowLongPtr(form->hwnd, GWLP_USERDATA, (LONG_PTR)form);
    
    /* Add to form list */
    if (g_halforms.formCount >= g_halforms.formCapacity) {
        g_halforms.formCapacity *= 2;
        g_halforms.forms = (HalForm**)realloc(g_halforms.forms, 
            g_halforms.formCapacity * sizeof(HalForm*));
    }
    g_halforms.forms[g_halforms.formCount++] = form;
    
    /* Set as main form if first */
    if (!g_halforms.mainForm) {
        g_halforms.mainForm = form;
    }
    
    return form;
}

void halform_destroy(HalForm* form) {
    if (!form) return;
    
    /* Destroy child controls */
    for (int i = 0; i < form->base.childCount; i++) {
        if (form->base.children[i]) {
            if (form->base.children[i]->hwnd) {
                DestroyWindow(form->base.children[i]->hwnd);
            }
            free(form->base.children[i]->name);
            free(form->base.children[i]->text);
            free(form->base.children[i]);
        }
    }
    free(form->base.children);
    
    /* Destroy menu */
    if (form->menu) {
        halmenu_destroy(form->menu);
    }
    
    /* Destroy window */
    if (form->hwnd) {
        DestroyWindow(form->hwnd);
    }
    
    /* Remove from form list */
    for (int i = 0; i < g_halforms.formCount; i++) {
        if (g_halforms.forms[i] == form) {
            g_halforms.forms[i] = g_halforms.forms[--g_halforms.formCount];
            break;
        }
    }
    
    free(form->base.name);
    free(form->base.text);
    free(form);
}

void halform_show(HalForm* form) {
    if (!form) return;
    form->base.visible = true;
    ShowWindow(form->hwnd, SW_SHOW);
    UpdateWindow(form->hwnd);
}

void halform_hide(HalForm* form) {
    if (!form) return;
    form->base.visible = false;
    ShowWindow(form->hwnd, SW_HIDE);
}

void halform_close(HalForm* form) {
    if (!form) return;
    
    /* Fire closing event */
    if (form->onClosing) {
        HalFormEvent event = {0};
        event.type = HALEVENT_CLOSING;
        event.sender = (HalControl*)form;
        form->onClosing((HalControl*)form, &event, form->base.eventUserData);
        if (event.cancel) return;
    }
    
    /* Close the form */
    if (form == g_halforms.mainForm) {
        halforms_exit(0);
    } else {
        halform_destroy(form);
    }
}

void halform_center(HalForm* form) {
    if (!form) return;
    
    RECT rect;
    GetWindowRect(form->hwnd, &rect);
    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;
    
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    
    int x = (screenW - width) / 2;
    int y = (screenH - height) / 2;
    
    SetWindowPos(form->hwnd, NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

void halform_maximize(HalForm* form) {
    if (form) ShowWindow(form->hwnd, SW_MAXIMIZE);
}

void halform_minimize(HalForm* form) {
    if (form) ShowWindow(form->hwnd, SW_MINIMIZE);
}

void halform_restore(HalForm* form) {
    if (form) ShowWindow(form->hwnd, SW_RESTORE);
}

void halform_set_title(HalForm* form, const char* title) {
    if (!form) return;
    free(form->base.text);
    form->base.text = _strdup(title);
    wchar_t* wtitle = utf8_to_wide(title);
    SetWindowTextW(form->hwnd, wtitle ? wtitle : L"");
    free(wtitle);
}

void halform_set_icon(HalForm* form, const char* iconPath) {
    if (!form) return;
    
    HICON hIcon = (HICON)LoadImageA(NULL, iconPath, IMAGE_ICON, 0, 0, 
        LR_LOADFROMFILE | LR_DEFAULTSIZE);
    
    if (hIcon) {
        SendMessage(form->hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
        SendMessage(form->hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
    }
}

void halform_set_menu(HalForm* form, HalMenu* menu) {
    if (!form || !menu) return;
    form->menu = menu;
    SetMenu(form->hwnd, menu->hmenu);
}

/* ============================================
   Window Procedure
   ============================================ */

static LRESULT CALLBACK HalFormsWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    HalForm* form = (HalForm*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    
    switch (msg) {
        case WM_CREATE: {
            CREATESTRUCT* cs = (CREATESTRUCT*)lParam;
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)cs->lpCreateParams);
            return 0;
        }
        
        case WM_CLOSE: {
            if (form) {
                halform_close(form);
                return 0;
            }
            break;
        }
        
        case WM_DESTROY: {
            if (form == g_halforms.mainForm) {
                PostQuitMessage(0);
            }
            return 0;
        }
        
        case WM_SIZE: {
            if (form) {
                form->base.width = LOWORD(lParam);
                form->base.height = HIWORD(lParam);
                halforms_apply_layout(form);
                
                if (form->onResize) {
                    HalFormEvent event = {0};
                    event.type = HALEVENT_RESIZE;
                    event.sender = (HalControl*)form;
                    form->onResize((HalControl*)form, &event, form->base.eventUserData);
                }
            }
            return 0;
        }
        
        case WM_COMMAND: {
            if (form) {
                int id = LOWORD(wParam);
                int code = HIWORD(wParam);
                HWND ctrlHwnd = (HWND)lParam;
                
                /* Debug output */
                char debug[256];
                snprintf(debug, sizeof(debug), "WM_COMMAND: id=%d, code=%d, lParam=%lld\n", id, code, (long long)lParam);
                OutputDebugStringA(debug);
                
                /* Find control */
                HalControl* ctrl = halforms_find_control_by_hwnd(form, ctrlHwnd);
                
                if (ctrl) {
                    sprintf(debug, "Found control, onClick=%p\n", ctrl->onClick);
                    OutputDebugStringA(debug);
                    
                    HalFormEvent event = {0};
                    event.sender = ctrl;
                    
                    switch (code) {
                        case BN_CLICKED:
                            event.type = HALEVENT_CLICK;
                            if (ctrl->onClick) {
                                OutputDebugStringA("Calling onClick handler\n");
                                ctrl->onClick(ctrl, &event, ctrl->eventUserData);
                            } else {
                                OutputDebugStringA("No onClick handler\n");
                            }
                            break;
                        case EN_CHANGE:
                            event.type = HALEVENT_TEXTCHANGED;
                            if (ctrl->onTextChanged) {
                                ctrl->onTextChanged(ctrl, &event, ctrl->eventUserData);
                            }
                            break;
                        case CBN_SELCHANGE:
                            event.type = HALEVENT_SELECTIONCHANGED;
                            if (ctrl->onTextChanged) {
                                ctrl->onTextChanged(ctrl, &event, ctrl->eventUserData);
                            }
                            break;
                    }
                } else {
                    sprintf(debug, "Control not found for hwnd=%p\n", ctrlHwnd);
                    OutputDebugStringA(debug);
                }
                
                /* Handle menu commands (lParam == 0 means menu item) */
                if (lParam == 0 && id >= 10000) {
                    /* This is a menu command - dispatch to runtime */
                    extern const char* halforms_get_menu_handler(int menuId);
                    extern void halforms_rt_dispatch_menu_event(const char* handlerName);
                    
                    const char* handler = halforms_get_menu_handler(id);
                    if (handler) {
                        halforms_rt_dispatch_menu_event(handler);
                    }
                }
            }
            return 0;
        }
        
        case WM_NOTIFY: {
            /* Handle tree view, list view, tab control notifications */
            return 0;
        }
        
        case WM_CTLCOLORSTATIC:
        case WM_CTLCOLORBTN:
        case WM_CTLCOLOREDIT:
        case WM_CTLCOLORLISTBOX: {
            /* Allow custom colors for controls */
            HDC hdc = (HDC)wParam;
            SetBkMode(hdc, TRANSPARENT);
            return (LRESULT)GetSysColorBrush(COLOR_BTNFACE);
        }
    }
    
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

/* ============================================
   Helper Functions
   ============================================ */

static HalControl* halforms_find_control_by_hwnd(HalForm* form, HWND hwnd) {
    if (!form) return NULL;
    
    for (int i = 0; i < form->base.childCount; i++) {
        if (form->base.children[i] && form->base.children[i]->hwnd == hwnd) {
            return form->base.children[i];
        }
    }
    return NULL;
}

static void halforms_apply_layout(HalForm* form) {
    if (!form) return;
    
    RECT clientRect;
    GetClientRect(form->hwnd, &clientRect);
    int clientW = clientRect.right;
    int clientH = clientRect.bottom;
    
    /* Apply dock and anchor styles */
    for (int i = 0; i < form->base.childCount; i++) {
        HalControl* ctrl = form->base.children[i];
        if (!ctrl) continue;
        
        int x = ctrl->x;
        int y = ctrl->y;
        int w = ctrl->width;
        int h = ctrl->height;
        
        /* Apply dock style */
        switch (ctrl->dock) {
            case HALDOCK_TOP:
                x = 0; y = 0; w = clientW;
                break;
            case HALDOCK_BOTTOM:
                x = 0; y = clientH - h; w = clientW;
                break;
            case HALDOCK_LEFT:
                x = 0; y = 0; h = clientH;
                break;
            case HALDOCK_RIGHT:
                x = clientW - w; y = 0; h = clientH;
                break;
            case HALDOCK_FILL:
                x = 0; y = 0; w = clientW; h = clientH;
                break;
            default:
                /* Apply anchor style */
                if (ctrl->anchor & HALANCHOR_RIGHT) {
                    if (ctrl->anchor & HALANCHOR_LEFT) {
                        w = clientW - ctrl->x - (form->base.width - ctrl->x - ctrl->width);
                    } else {
                        x = clientW - (form->base.width - ctrl->x);
                    }
                }
                if (ctrl->anchor & HALANCHOR_BOTTOM) {
                    if (ctrl->anchor & HALANCHOR_TOP) {
                        h = clientH - ctrl->y - (form->base.height - ctrl->y - ctrl->height);
                    } else {
                        y = clientH - (form->base.height - ctrl->y);
                    }
                }
                break;
        }
        
        SetWindowPos(ctrl->hwnd, NULL, x, y, w, h, SWP_NOZORDER);
    }
}

/* ============================================
   Utility Functions
   ============================================ */

void halforms_enable_visual_styles(void) {
    /* Enable XP visual styles via manifest or runtime */
    /* This is typically done via application manifest */
}

HFONT halforms_create_font(const char* name, int size, bool bold, bool italic) {
    return CreateFontA(
        -MulDiv(size, GetDeviceCaps(GetDC(NULL), LOGPIXELSY), 72),
        0, 0, 0,
        bold ? FW_BOLD : FW_NORMAL,
        italic, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, name
    );
}

HICON halforms_load_icon(const char* path) {
    return (HICON)LoadImageA(NULL, path, IMAGE_ICON, 0, 0, 
        LR_LOADFROMFILE | LR_DEFAULTSIZE);
}

HBITMAP halforms_load_bitmap(const char* path) {
    return (HBITMAP)LoadImageA(NULL, path, IMAGE_BITMAP, 0, 0, 
        LR_LOADFROMFILE);
}
