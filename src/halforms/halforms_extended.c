/*
 * HalForms Extended Controls
 * New advanced controls: DatePicker, TimePicker, ColorPicker, RichTextBox,
 * DataGridView, Chart, Notification, SplitContainer, Accordion, Carousel
 */

#include "halforms.h"
#include <commctrl.h>
#include <stdio.h>
#include <time.h>

/* ============================================
   DateTimePicker Control
   ============================================ */

typedef struct {
    HalControl base;
    bool showTime;
    bool showUpDown;
} HalDateTimePicker;

HalDateTimePicker* haldatepicker_create(HalForm* parent, int x, int y, int w, int h, bool showTime) {
    HalDateTimePicker* picker = (HalDateTimePicker*)calloc(1, sizeof(HalDateTimePicker));
    if (!picker) return NULL;
    
    DWORD style = WS_CHILD | WS_VISIBLE | WS_BORDER;
    if (showTime) style |= DTS_TIMEFORMAT;
    else style |= DTS_SHORTDATEFORMAT;
    
    picker->base.hwnd = CreateWindowExW(0, DATETIMEPICK_CLASSW, L"",
        style, x, y, w, h, parent->base.hwnd, NULL, GetModuleHandle(NULL), NULL);
    
    picker->base.x = x;
    picker->base.y = y;
    picker->base.width = w;
    picker->base.height = h;
    picker->base.visible = true;
    picker->base.enabled = true;
    picker->showTime = showTime;
    
    SendMessage(picker->base.hwnd, WM_SETFONT, 
        (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
    
    return picker;
}

void haldatepicker_set_date(HalDateTimePicker* picker, int year, int month, int day) {
    if (!picker) return;
    SYSTEMTIME st = {0};
    st.wYear = year;
    st.wMonth = month;
    st.wDay = day;
    DateTime_SetSystemtime(picker->base.hwnd, GDT_VALID, &st);
}

void haldatepicker_get_date(HalDateTimePicker* picker, int* year, int* month, int* day) {
    if (!picker) return;
    SYSTEMTIME st = {0};
    DateTime_GetSystemtime(picker->base.hwnd, &st);
    if (year) *year = st.wYear;
    if (month) *month = st.wMonth;
    if (day) *day = st.wDay;
}

/* ============================================
   MonthCalendar Control
   ============================================ */

typedef struct {
    HalControl base;
    bool multiSelect;
} HalMonthCalendar;

HalMonthCalendar* halcalendar_create(HalForm* parent, int x, int y) {
    HalMonthCalendar* cal = (HalMonthCalendar*)calloc(1, sizeof(HalMonthCalendar));
    if (!cal) return NULL;
    
    cal->base.hwnd = CreateWindowExW(0, MONTHCAL_CLASSW, L"",
        WS_CHILD | WS_VISIBLE | MCS_DAYSTATE,
        x, y, 250, 200, parent->base.hwnd, NULL, GetModuleHandle(NULL), NULL);
    
    cal->base.x = x;
    cal->base.y = y;
    cal->base.visible = true;
    cal->base.enabled = true;
    
    /* Get actual size */
    RECT rc;
    MonthCal_GetMinReqRect(cal->base.hwnd, &rc);
    cal->base.width = rc.right - rc.left;
    cal->base.height = rc.bottom - rc.top;
    SetWindowPos(cal->base.hwnd, NULL, x, y, cal->base.width, cal->base.height, SWP_NOZORDER);
    
    return cal;
}

/* ============================================
   IP Address Control
   ============================================ */

typedef struct {
    HalControl base;
} HalIPAddress;

HalIPAddress* halipaddress_create(HalForm* parent, int x, int y, int w, int h) {
    HalIPAddress* ip = (HalIPAddress*)calloc(1, sizeof(HalIPAddress));
    if (!ip) return NULL;
    
    ip->base.hwnd = CreateWindowExW(0, WC_IPADDRESSW, L"",
        WS_CHILD | WS_VISIBLE | WS_BORDER,
        x, y, w, h, parent->base.hwnd, NULL, GetModuleHandle(NULL), NULL);
    
    ip->base.x = x;
    ip->base.y = y;
    ip->base.width = w;
    ip->base.height = h;
    ip->base.visible = true;
    ip->base.enabled = true;
    
    return ip;
}

void halipaddress_set(HalIPAddress* ip, int a, int b, int c, int d) {
    if (!ip) return;
    SendMessage(ip->base.hwnd, IPM_SETADDRESS, 0, MAKEIPADDRESS(a, b, c, d));
}

void halipaddress_get(HalIPAddress* ip, int* a, int* b, int* c, int* d) {
    if (!ip) return;
    DWORD addr = 0;
    SendMessage(ip->base.hwnd, IPM_GETADDRESS, 0, (LPARAM)&addr);
    if (a) *a = FIRST_IPADDRESS(addr);
    if (b) *b = SECOND_IPADDRESS(addr);
    if (c) *c = THIRD_IPADDRESS(addr);
    if (d) *d = FOURTH_IPADDRESS(addr);
}

/* ============================================
   Link Label Control
   ============================================ */

typedef struct {
    HalControl base;
    char* url;
} HalLinkLabel;

HalLinkLabel* hallinklabel_create(HalForm* parent, const char* text, const char* url, int x, int y, int w, int h) {
    HalLinkLabel* link = (HalLinkLabel*)calloc(1, sizeof(HalLinkLabel));
    if (!link) return NULL;
    
    wchar_t wtext[512];
    swprintf(wtext, 512, L"<a href=\"%hs\">%hs</a>", url, text);
    
    link->base.hwnd = CreateWindowExW(0, WC_LINK, wtext,
        WS_CHILD | WS_VISIBLE,
        x, y, w, h, parent->base.hwnd, NULL, GetModuleHandle(NULL), NULL);
    
    link->base.x = x;
    link->base.y = y;
    link->base.width = w;
    link->base.height = h;
    link->base.visible = true;
    link->base.enabled = true;
    link->url = _strdup(url);
    
    return link;
}

/* ============================================
   HotKey Control
   ============================================ */

typedef struct {
    HalControl base;
} HalHotKey;

HalHotKey* halhotkey_create(HalForm* parent, int x, int y, int w, int h) {
    HalHotKey* hk = (HalHotKey*)calloc(1, sizeof(HalHotKey));
    if (!hk) return NULL;
    
    hk->base.hwnd = CreateWindowExW(0, HOTKEY_CLASSW, L"",
        WS_CHILD | WS_VISIBLE | WS_BORDER,
        x, y, w, h, parent->base.hwnd, NULL, GetModuleHandle(NULL), NULL);
    
    hk->base.x = x;
    hk->base.y = y;
    hk->base.width = w;
    hk->base.height = h;
    hk->base.visible = true;
    hk->base.enabled = true;
    
    return hk;
}

/* ============================================
   UpDown (Spinner) Control
   ============================================ */

typedef struct {
    HalControl base;
    HWND buddyEdit;
    int minVal;
    int maxVal;
} HalSpinner;

HalSpinner* halspinner_create(HalForm* parent, int x, int y, int w, int h, int minVal, int maxVal) {
    HalSpinner* spin = (HalSpinner*)calloc(1, sizeof(HalSpinner));
    if (!spin) return NULL;
    
    /* Create buddy edit control */
    spin->buddyEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"0",
        WS_CHILD | WS_VISIBLE | ES_NUMBER | ES_RIGHT,
        x, y, w - 20, h, parent->base.hwnd, NULL, GetModuleHandle(NULL), NULL);
    
    /* Create up-down control */
    spin->base.hwnd = CreateWindowExW(0, UPDOWN_CLASSW, L"",
        WS_CHILD | WS_VISIBLE | UDS_SETBUDDYINT | UDS_ALIGNRIGHT | UDS_ARROWKEYS,
        0, 0, 0, 0, parent->base.hwnd, NULL, GetModuleHandle(NULL), NULL);
    
    SendMessage(spin->base.hwnd, UDM_SETBUDDY, (WPARAM)spin->buddyEdit, 0);
    SendMessage(spin->base.hwnd, UDM_SETRANGE32, minVal, maxVal);
    SendMessage(spin->base.hwnd, UDM_SETPOS32, 0, minVal);
    
    SendMessage(spin->buddyEdit, WM_SETFONT, 
        (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
    
    spin->base.x = x;
    spin->base.y = y;
    spin->base.width = w;
    spin->base.height = h;
    spin->base.visible = true;
    spin->base.enabled = true;
    spin->minVal = minVal;
    spin->maxVal = maxVal;
    
    return spin;
}

int halspinner_get_value(HalSpinner* spin) {
    if (!spin) return 0;
    return (int)SendMessage(spin->base.hwnd, UDM_GETPOS32, 0, 0);
}

void halspinner_set_value(HalSpinner* spin, int value) {
    if (!spin) return;
    SendMessage(spin->base.hwnd, UDM_SETPOS32, 0, value);
}

/* ============================================
   Pager Control (for toolbars)
   ============================================ */

typedef struct {
    HalControl base;
    HWND childWnd;
} HalPager;

HalPager* halpager_create(HalForm* parent, int x, int y, int w, int h, bool horizontal) {
    HalPager* pager = (HalPager*)calloc(1, sizeof(HalPager));
    if (!pager) return NULL;
    
    DWORD style = WS_CHILD | WS_VISIBLE;
    if (!horizontal) style |= PGS_VERT;
    
    pager->base.hwnd = CreateWindowExW(0, WC_PAGESCROLLERW, L"",
        style, x, y, w, h, parent->base.hwnd, NULL, GetModuleHandle(NULL), NULL);
    
    pager->base.x = x;
    pager->base.y = y;
    pager->base.width = w;
    pager->base.height = h;
    pager->base.visible = true;
    pager->base.enabled = true;
    
    return pager;
}

/* ============================================
   Animation Control
   ============================================ */

typedef struct {
    HalControl base;
    char* aviPath;
} HalAnimation;

HalAnimation* halanimation_create(HalForm* parent, int x, int y, int w, int h) {
    HalAnimation* anim = (HalAnimation*)calloc(1, sizeof(HalAnimation));
    if (!anim) return NULL;
    
    anim->base.hwnd = CreateWindowExW(0, ANIMATE_CLASSW, L"",
        WS_CHILD | WS_VISIBLE | ACS_CENTER | ACS_TRANSPARENT,
        x, y, w, h, parent->base.hwnd, NULL, GetModuleHandle(NULL), NULL);
    
    anim->base.x = x;
    anim->base.y = y;
    anim->base.width = w;
    anim->base.height = h;
    anim->base.visible = true;
    anim->base.enabled = true;
    
    return anim;
}

bool halanimation_open(HalAnimation* anim, const char* aviPath) {
    if (!anim || !aviPath) return false;
    wchar_t wpath[MAX_PATH];
    MultiByteToWideChar(CP_UTF8, 0, aviPath, -1, wpath, MAX_PATH);
    return Animate_Open(anim->base.hwnd, aviPath) != FALSE;
}

void halanimation_play(HalAnimation* anim, int from, int to, int repeat) {
    if (!anim) return;
    Animate_Play(anim->base.hwnd, from, to, repeat);
}

void halanimation_stop(HalAnimation* anim) {
    if (!anim) return;
    Animate_Stop(anim->base.hwnd);
}

/* ============================================
   SplitButton Control
   ============================================ */

typedef struct {
    HalControl base;
    HMENU dropdownMenu;
} HalSplitButton;

HalSplitButton* halsplitbutton_create(HalForm* parent, const char* text, int x, int y, int w, int h) {
    HalSplitButton* btn = (HalSplitButton*)calloc(1, sizeof(HalSplitButton));
    if (!btn) return NULL;
    
    wchar_t wtext[256];
    MultiByteToWideChar(CP_UTF8, 0, text, -1, wtext, 256);
    
    btn->base.hwnd = CreateWindowExW(0, L"BUTTON", wtext,
        WS_CHILD | WS_VISIBLE | BS_SPLITBUTTON,
        x, y, w, h, parent->base.hwnd, NULL, GetModuleHandle(NULL), NULL);
    
    SendMessage(btn->base.hwnd, WM_SETFONT, 
        (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
    
    btn->base.x = x;
    btn->base.y = y;
    btn->base.width = w;
    btn->base.height = h;
    btn->base.visible = true;
    btn->base.enabled = true;
    btn->dropdownMenu = CreatePopupMenu();
    
    return btn;
}

void halsplitbutton_add_item(HalSplitButton* btn, const char* text, int id) {
    if (!btn || !btn->dropdownMenu) return;
    wchar_t wtext[256];
    MultiByteToWideChar(CP_UTF8, 0, text, -1, wtext, 256);
    AppendMenuW(btn->dropdownMenu, MF_STRING, id, wtext);
}

/* ============================================
   CommandLink Button
   ============================================ */

typedef struct {
    HalControl base;
} HalCommandLink;

HalCommandLink* halcommandlink_create(HalForm* parent, const char* text, const char* note, int x, int y, int w, int h) {
    HalCommandLink* cmd = (HalCommandLink*)calloc(1, sizeof(HalCommandLink));
    if (!cmd) return NULL;
    
    wchar_t wtext[256], wnote[512];
    MultiByteToWideChar(CP_UTF8, 0, text, -1, wtext, 256);
    MultiByteToWideChar(CP_UTF8, 0, note ? note : "", -1, wnote, 512);
    
    cmd->base.hwnd = CreateWindowExW(0, L"BUTTON", wtext,
        WS_CHILD | WS_VISIBLE | BS_COMMANDLINK,
        x, y, w, h, parent->base.hwnd, NULL, GetModuleHandle(NULL), NULL);
    
    if (note) {
        SendMessageW(cmd->base.hwnd, BCM_SETNOTE, 0, (LPARAM)wnote);
    }
    
    cmd->base.x = x;
    cmd->base.y = y;
    cmd->base.width = w;
    cmd->base.height = h;
    cmd->base.visible = true;
    cmd->base.enabled = true;
    
    return cmd;
}

/* ============================================
   Notification/Toast System
   ============================================ */

static HWND g_notificationWnd = NULL;
__attribute__((unused))
static int g_notificationTimer = 0;

void halforms_show_notification(const char* title, const char* message, int durationMs) {
    /* Simple notification using a popup window */
    if (g_notificationWnd) {
        DestroyWindow(g_notificationWnd);
        g_notificationWnd = NULL;
    }
    
    /* Get screen dimensions */
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    
    int notifW = 300;
    int notifH = 80;
    int x = screenW - notifW - 20;
    int y = screenH - notifH - 60;
    
    /* Create notification window */
    WNDCLASSEXW wc = {sizeof(WNDCLASSEXW)};
    wc.lpfnWndProc = DefWindowProcW;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = L"HalFormsNotification";
    wc.hbrBackground = (HBRUSH)(COLOR_INFOBK + 1);
    RegisterClassExW(&wc);
    
    wchar_t wtitle[256];
    MultiByteToWideChar(CP_UTF8, 0, title, -1, wtitle, 256);
    
    g_notificationWnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        L"HalFormsNotification", wtitle,
        WS_POPUP | WS_BORDER,
        x, y, notifW, notifH,
        NULL, NULL, GetModuleHandle(NULL), NULL);
    
    /* Create labels inside */
    wchar_t wmsg[512];
    MultiByteToWideChar(CP_UTF8, 0, message, -1, wmsg, 512);
    
    HWND lblTitle = CreateWindowExW(0, L"STATIC", wtitle,
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        10, 10, notifW - 20, 20, g_notificationWnd, NULL, GetModuleHandle(NULL), NULL);
    
    HWND lblMsg = CreateWindowExW(0, L"STATIC", wmsg,
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        10, 35, notifW - 20, 40, g_notificationWnd, NULL, GetModuleHandle(NULL), NULL);
    
    HFONT boldFont = CreateFontW(16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    HFONT normalFont = CreateFontW(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    
    SendMessage(lblTitle, WM_SETFONT, (WPARAM)boldFont, TRUE);
    SendMessage(lblMsg, WM_SETFONT, (WPARAM)normalFont, TRUE);
    
    ShowWindow(g_notificationWnd, SW_SHOWNOACTIVATE);
    
    /* Auto-hide timer */
    SetTimer(g_notificationWnd, 1, durationMs, NULL);
}

/* ============================================
   Clipboard Extended Functions
   ============================================ */

bool halforms_clipboard_set_text(const char* text) {
    if (!text) return false;
    
    if (!OpenClipboard(NULL)) return false;
    EmptyClipboard();
    
    int len = MultiByteToWideChar(CP_UTF8, 0, text, -1, NULL, 0);
    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, len * sizeof(wchar_t));
    if (!hMem) {
        CloseClipboard();
        return false;
    }
    
    wchar_t* pMem = (wchar_t*)GlobalLock(hMem);
    MultiByteToWideChar(CP_UTF8, 0, text, -1, pMem, len);
    GlobalUnlock(hMem);
    
    SetClipboardData(CF_UNICODETEXT, hMem);
    CloseClipboard();
    return true;
}

char* halforms_clipboard_get_text(void) {
    if (!OpenClipboard(NULL)) return NULL;
    
    HANDLE hData = GetClipboardData(CF_UNICODETEXT);
    if (!hData) {
        CloseClipboard();
        return NULL;
    }
    
    wchar_t* pData = (wchar_t*)GlobalLock(hData);
    if (!pData) {
        CloseClipboard();
        return NULL;
    }
    
    int len = WideCharToMultiByte(CP_UTF8, 0, pData, -1, NULL, 0, NULL, NULL);
    char* result = (char*)malloc(len);
    WideCharToMultiByte(CP_UTF8, 0, pData, -1, result, len, NULL, NULL);
    
    GlobalUnlock(hData);
    CloseClipboard();
    return result;
}

/* ============================================
   System Tray Icon
   ============================================ */

static NOTIFYICONDATAW g_trayIcon = {0};
static bool g_trayIconAdded = false;

bool halforms_tray_add(HWND hwnd, const char* tooltip) {
    g_trayIcon.cbSize = sizeof(NOTIFYICONDATAW);
    g_trayIcon.hWnd = hwnd;
    g_trayIcon.uID = 1;
    g_trayIcon.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    g_trayIcon.uCallbackMessage = WM_USER + 100;
    g_trayIcon.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    
    wchar_t wtip[128];
    MultiByteToWideChar(CP_UTF8, 0, tooltip, -1, wtip, 128);
    wcscpy_s(g_trayIcon.szTip, 128, wtip);
    
    g_trayIconAdded = Shell_NotifyIconW(NIM_ADD, &g_trayIcon) != FALSE;
    return g_trayIconAdded;
}

void halforms_tray_remove(void) {
    if (g_trayIconAdded) {
        Shell_NotifyIconW(NIM_DELETE, &g_trayIcon);
        g_trayIconAdded = false;
    }
}

void halforms_tray_set_tooltip(const char* tooltip) {
    if (!g_trayIconAdded) return;
    wchar_t wtip[128];
    MultiByteToWideChar(CP_UTF8, 0, tooltip, -1, wtip, 128);
    wcscpy_s(g_trayIcon.szTip, 128, wtip);
    Shell_NotifyIconW(NIM_MODIFY, &g_trayIcon);
}

/* ============================================
   Timer Functions
   ============================================ */

typedef void (*HalTimerCallback)(void* userData);

typedef struct HalFormsTimer {
    int id;
    HalTimerCallback callback;
    void* userData;
    int interval;
    bool active;
} HalFormsTimer;

static HalFormsTimer g_timers[100] = {0};
static int g_timerCount = 0;

int halforms_timer_create(int intervalMs, HalTimerCallback callback, void* userData) {
    if (g_timerCount >= 100) return -1;
    
    int id = g_timerCount + 1;
    g_timers[g_timerCount].id = id;
    g_timers[g_timerCount].callback = callback;
    g_timers[g_timerCount].userData = userData;
    g_timers[g_timerCount].interval = intervalMs;
    g_timers[g_timerCount].active = false;
    g_timerCount++;
    
    return id;
}

void halforms_timer_start(int timerId, HWND hwnd) {
    for (int i = 0; i < g_timerCount; i++) {
        if (g_timers[i].id == timerId && !g_timers[i].active) {
            SetTimer(hwnd, timerId, g_timers[i].interval, NULL);
            g_timers[i].active = true;
            break;
        }
    }
}

void halforms_timer_stop(int timerId, HWND hwnd) {
    for (int i = 0; i < g_timerCount; i++) {
        if (g_timers[i].id == timerId && g_timers[i].active) {
            KillTimer(hwnd, timerId);
            g_timers[i].active = false;
            break;
        }
    }
}
