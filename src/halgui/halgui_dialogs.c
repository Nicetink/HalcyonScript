/*
 * HalGUI - Dialog Implementation
 * 
 * Message boxes, file dialogs, color picker
 */

#include "halgui.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <commdlg.h>
#include <shlobj.h>

/* ============================================
   Message Dialog
   ============================================ */

int hal_dialog_message(HalWindow* parent, const char* title, const char* message,
                       HalDialogButtons buttons, HalDialogIcon icon) {
    UINT type = 0;
    
    // Button type
    switch (buttons) {
        case HAL_DIALOG_OK:           type |= MB_OK; break;
        case HAL_DIALOG_OK_CANCEL:    type |= MB_OKCANCEL; break;
        case HAL_DIALOG_YES_NO:       type |= MB_YESNO; break;
        case HAL_DIALOG_YES_NO_CANCEL: type |= MB_YESNOCANCEL; break;
    }
    
    // Icon type
    switch (icon) {
        case HAL_DIALOG_INFO:     type |= MB_ICONINFORMATION; break;
        case HAL_DIALOG_WARNING:  type |= MB_ICONWARNING; break;
        case HAL_DIALOG_ERROR:    type |= MB_ICONERROR; break;
        case HAL_DIALOG_QUESTION: type |= MB_ICONQUESTION; break;
    }
    
    // Convert strings
    int titleLen = MultiByteToWideChar(CP_UTF8, 0, title, -1, NULL, 0);
    int msgLen = MultiByteToWideChar(CP_UTF8, 0, message, -1, NULL, 0);
    
    wchar_t* wideTitle = (wchar_t*)malloc(titleLen * sizeof(wchar_t));
    wchar_t* wideMsg = (wchar_t*)malloc(msgLen * sizeof(wchar_t));
    
    MultiByteToWideChar(CP_UTF8, 0, title, -1, wideTitle, titleLen);
    MultiByteToWideChar(CP_UTF8, 0, message, -1, wideMsg, msgLen);
    
    HWND hwndParent = parent ? parent->hwnd : NULL;
    int result = MessageBoxW(hwndParent, wideMsg, wideTitle, type);
    
    free(wideTitle);
    free(wideMsg);
    
    // Convert result to simple values
    switch (result) {
        case IDOK:     return 1;
        case IDCANCEL: return 0;
        case IDYES:    return 1;
        case IDNO:     return 2;
        default:       return 0;
    }
}

/* ============================================
   Input Dialog
   ============================================ */

// Simple input dialog using a custom window
char* hal_dialog_input(HalWindow* parent, const char* title, const char* prompt, const char* defaultValue) {
    // For simplicity, use a basic approach
    // In a full implementation, this would create a custom HalGUI dialog
    
    // Allocate result buffer
    char* result = (char*)malloc(256);
    if (!result) return NULL;
    
    if (defaultValue) {
        strncpy(result, defaultValue, 255);
        result[255] = '\0';
    } else {
        result[0] = '\0';
    }
    
    // TODO: Implement custom input dialog with HalGUI
    // For now, return default value
    return result;
}

/* ============================================
   File Dialogs
   ============================================ */

char* hal_dialog_open_file(HalWindow* parent, const char* title, const char* filter) {
    wchar_t filename[MAX_PATH] = {0};
    
    // Convert filter (format: "Description|*.ext|Description2|*.ext2||")
    wchar_t wideFilter[512] = {0};
    if (filter) {
        int filterLen = MultiByteToWideChar(CP_UTF8, 0, filter, -1, wideFilter, 512);
        // Replace | with \0
        for (int i = 0; i < filterLen; i++) {
            if (wideFilter[i] == L'|') wideFilter[i] = L'\0';
        }
    } else {
        wcscpy(wideFilter, L"All Files\0*.*\0\0");
    }
    
    wchar_t wideTitle[256] = {0};
    if (title) {
        MultiByteToWideChar(CP_UTF8, 0, title, -1, wideTitle, 256);
    }
    
    OPENFILENAMEW ofn = {0};
    ofn.lStructSize = sizeof(OPENFILENAMEW);
    ofn.hwndOwner = parent ? parent->hwnd : NULL;
    ofn.lpstrFilter = wideFilter;
    ofn.lpstrFile = filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = wideTitle[0] ? wideTitle : NULL;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    
    if (GetOpenFileNameW(&ofn)) {
        // Convert to UTF-8
        int len = WideCharToMultiByte(CP_UTF8, 0, filename, -1, NULL, 0, NULL, NULL);
        char* result = (char*)malloc(len);
        WideCharToMultiByte(CP_UTF8, 0, filename, -1, result, len, NULL, NULL);
        return result;
    }
    
    return NULL;
}

char* hal_dialog_save_file(HalWindow* parent, const char* title, const char* filter, const char* defaultName) {
    wchar_t filename[MAX_PATH] = {0};
    
    if (defaultName) {
        MultiByteToWideChar(CP_UTF8, 0, defaultName, -1, filename, MAX_PATH);
    }
    
    wchar_t wideFilter[512] = {0};
    if (filter) {
        int filterLen = MultiByteToWideChar(CP_UTF8, 0, filter, -1, wideFilter, 512);
        for (int i = 0; i < filterLen; i++) {
            if (wideFilter[i] == L'|') wideFilter[i] = L'\0';
        }
    } else {
        wcscpy(wideFilter, L"All Files\0*.*\0\0");
    }
    
    wchar_t wideTitle[256] = {0};
    if (title) {
        MultiByteToWideChar(CP_UTF8, 0, title, -1, wideTitle, 256);
    }
    
    OPENFILENAMEW ofn = {0};
    ofn.lStructSize = sizeof(OPENFILENAMEW);
    ofn.hwndOwner = parent ? parent->hwnd : NULL;
    ofn.lpstrFilter = wideFilter;
    ofn.lpstrFile = filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = wideTitle[0] ? wideTitle : NULL;
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;
    
    if (GetSaveFileNameW(&ofn)) {
        int len = WideCharToMultiByte(CP_UTF8, 0, filename, -1, NULL, 0, NULL, NULL);
        char* result = (char*)malloc(len);
        WideCharToMultiByte(CP_UTF8, 0, filename, -1, result, len, NULL, NULL);
        return result;
    }
    
    return NULL;
}

char* hal_dialog_select_folder(HalWindow* parent, const char* title) {
    wchar_t path[MAX_PATH] = {0};
    
    wchar_t wideTitle[256] = {0};
    if (title) {
        MultiByteToWideChar(CP_UTF8, 0, title, -1, wideTitle, 256);
    }
    
    BROWSEINFOW bi = {0};
    bi.hwndOwner = parent ? parent->hwnd : NULL;
    bi.lpszTitle = wideTitle[0] ? wideTitle : NULL;
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
    
    LPITEMIDLIST pidl = SHBrowseForFolderW(&bi);
    if (pidl) {
        if (SHGetPathFromIDListW(pidl, path)) {
            CoTaskMemFree(pidl);
            
            int len = WideCharToMultiByte(CP_UTF8, 0, path, -1, NULL, 0, NULL, NULL);
            char* result = (char*)malloc(len);
            WideCharToMultiByte(CP_UTF8, 0, path, -1, result, len, NULL, NULL);
            return result;
        }
        CoTaskMemFree(pidl);
    }
    
    return NULL;
}

/* ============================================
   Color Picker
   ============================================ */

HalColor hal_dialog_color_picker(HalWindow* parent, HalColor initialColor) {
    static COLORREF customColors[16] = {0};
    
    CHOOSECOLORW cc = {0};
    cc.lStructSize = sizeof(CHOOSECOLORW);
    cc.hwndOwner = parent ? parent->hwnd : NULL;
    cc.rgbResult = RGB(HAL_GET_R(initialColor), HAL_GET_G(initialColor), HAL_GET_B(initialColor));
    cc.lpCustColors = customColors;
    cc.Flags = CC_FULLOPEN | CC_RGBINIT;
    
    if (ChooseColorW(&cc)) {
        return HAL_RGB(GetRValue(cc.rgbResult), GetGValue(cc.rgbResult), GetBValue(cc.rgbResult));
    }
    
    return initialColor;
}

/* ============================================
   Clipboard
   ============================================ */

void hal_clipboard_set_text(const char* text) {
    if (!text) return;
    
    int len = MultiByteToWideChar(CP_UTF8, 0, text, -1, NULL, 0);
    
    if (OpenClipboard(NULL)) {
        EmptyClipboard();
        
        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, len * sizeof(wchar_t));
        if (hMem) {
            wchar_t* pMem = (wchar_t*)GlobalLock(hMem);
            MultiByteToWideChar(CP_UTF8, 0, text, -1, pMem, len);
            GlobalUnlock(hMem);
            
            SetClipboardData(CF_UNICODETEXT, hMem);
        }
        
        CloseClipboard();
    }
}

char* hal_clipboard_get_text(void) {
    char* result = NULL;
    
    if (OpenClipboard(NULL)) {
        HANDLE hData = GetClipboardData(CF_UNICODETEXT);
        if (hData) {
            wchar_t* pData = (wchar_t*)GlobalLock(hData);
            if (pData) {
                int len = WideCharToMultiByte(CP_UTF8, 0, pData, -1, NULL, 0, NULL, NULL);
                result = (char*)malloc(len);
                WideCharToMultiByte(CP_UTF8, 0, pData, -1, result, len, NULL, NULL);
                GlobalUnlock(hData);
            }
        }
        CloseClipboard();
    }
    
    return result;
}

bool hal_clipboard_has_text(void) {
    return IsClipboardFormatAvailable(CF_UNICODETEXT) != 0 || 
           IsClipboardFormatAvailable(CF_TEXT) != 0;
}

/* ============================================
   Timers
   ============================================ */

typedef struct {
    HalEventHandler callback;
    void* userData;
} HalTimerData;

static HalTimerData g_timers[64] = {0};
static int g_timerCount = 0;

static void CALLBACK hal_timer_proc(HWND hwnd, UINT msg, UINT_PTR idEvent, DWORD dwTime) {
    int id = (int)idEvent - 1;
    if (id >= 0 && id < g_timerCount && g_timers[id].callback) {
        HalEvent event = {0};
        event.type = HAL_EVENT_TIMER;
        event.data = (void*)(intptr_t)id;
        g_timers[id].callback(NULL, &event, g_timers[id].userData);
    }
}

int hal_timer_create(int intervalMs, HalEventHandler callback, void* userData) {
    if (g_timerCount >= 64) return -1;
    
    int id = g_timerCount++;
    g_timers[id].callback = callback;
    g_timers[id].userData = userData;
    
    SetTimer(NULL, id + 1, intervalMs, hal_timer_proc);
    
    return id;
}

void hal_timer_destroy(int timerId) {
    if (timerId >= 0 && timerId < g_timerCount) {
        KillTimer(NULL, timerId + 1);
        g_timers[timerId].callback = NULL;
        g_timers[timerId].userData = NULL;
    }
}
