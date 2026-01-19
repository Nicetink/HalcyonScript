/*
 * HalForms - Dialog Functions Implementation
 * Standard Windows dialogs: MessageBox, Open/Save File, Color, Font, etc.
 */

#include "halforms.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <commdlg.h>
#include <shlobj.h>

/* External globals */
extern struct {
    bool initialized;
    HINSTANCE hInstance;
    HFONT defaultFont;
    HalForm* mainForm;
    HalForm** forms;
    int formCount;
    int formCapacity;
    int nextControlId;
    bool running;
} g_halforms;

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

/* Helper function to convert Wide string to UTF-8 */
__attribute__((unused))
static char* wide_to_utf8(const wchar_t* wide) {
    if (!wide) return NULL;
    int len = WideCharToMultiByte(CP_UTF8, 0, wide, -1, NULL, 0, NULL, NULL);
    if (len == 0) return NULL;
    char* utf8 = (char*)malloc(len);
    if (!utf8) return NULL;
    WideCharToMultiByte(CP_UTF8, 0, wide, -1, utf8, len, NULL, NULL);
    return utf8;
}

/* ============================================
   Message Box
   ============================================ */

int halforms_msgbox(const char* text, const char* title, int buttons, int icon) {
    UINT type = 0;
    
    /* Button types */
    switch (buttons) {
        case 0: type |= MB_OK; break;
        case 1: type |= MB_OKCANCEL; break;
        case 2: type |= MB_YESNO; break;
        case 3: type |= MB_YESNOCANCEL; break;
        case 4: type |= MB_RETRYCANCEL; break;
        case 5: type |= MB_ABORTRETRYIGNORE; break;
        default: type |= MB_OK; break;
    }
    
    /* Icon types */
    switch (icon) {
        case 0: break; /* No icon */
        case 1: type |= MB_ICONINFORMATION; break;
        case 2: type |= MB_ICONWARNING; break;
        case 3: type |= MB_ICONERROR; break;
        case 4: type |= MB_ICONQUESTION; break;
        default: break;
    }
    
    HWND parent = g_halforms.mainForm ? g_halforms.mainForm->hwnd : NULL;
    
    /* Use Unicode version for proper UTF-8 support */
    wchar_t* wtext = utf8_to_wide(text);
    wchar_t* wtitle = utf8_to_wide(title);
    int result = MessageBoxW(parent, wtext ? wtext : L"", wtitle ? wtitle : L"Message", type);
    free(wtext);
    free(wtitle);
    
    /* Convert result to simple values */
    switch (result) {
        case IDOK: return 1;
        case IDCANCEL: return 0;
        case IDYES: return 1;
        case IDNO: return 0;
        case IDRETRY: return 2;
        case IDABORT: return 3;
        case IDIGNORE: return 4;
        default: return 0;
    }
}

/* ============================================
   Open File Dialog
   ============================================ */

char* halforms_open_file(const char* title, const char* filter) {
    char filename[MAX_PATH] = {0};
    
    OPENFILENAMEA ofn = {0};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = g_halforms.mainForm ? g_halforms.mainForm->hwnd : NULL;
    ofn.lpstrFilter = filter ? filter : "All Files (*.*)\0*.*\0";
    ofn.lpstrFile = filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = title ? title : "Open File";
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
    
    if (GetOpenFileNameA(&ofn)) {
        return _strdup(filename);
    }
    
    return NULL;
}

/* ============================================
   Save File Dialog
   ============================================ */

char* halforms_save_file(const char* title, const char* filter, const char* defaultName) {
    char filename[MAX_PATH] = {0};
    
    if (defaultName) {
        strncpy(filename, defaultName, MAX_PATH - 1);
    }
    
    OPENFILENAMEA ofn = {0};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = g_halforms.mainForm ? g_halforms.mainForm->hwnd : NULL;
    ofn.lpstrFilter = filter ? filter : "All Files (*.*)\0*.*\0";
    ofn.lpstrFile = filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = title ? title : "Save File";
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
    
    if (GetSaveFileNameA(&ofn)) {
        return _strdup(filename);
    }
    
    return NULL;
}

/* ============================================
   Browse Folder Dialog
   ============================================ */

/* Callback for folder browser */
static int CALLBACK BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData) {
    if (uMsg == BFFM_INITIALIZED && lpData) {
        SendMessage(hwnd, BFFM_SETSELECTION, TRUE, lpData);
    }
    return 0;
}

char* halforms_browse_folder(const char* title) {
    char path[MAX_PATH] = {0};
    
    BROWSEINFOA bi = {0};
    bi.hwndOwner = g_halforms.mainForm ? g_halforms.mainForm->hwnd : NULL;
    bi.lpszTitle = title ? title : "Select Folder";
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE | BIF_EDITBOX;
    bi.lpfn = BrowseCallbackProc;
    
    LPITEMIDLIST pidl = SHBrowseForFolderA(&bi);
    if (pidl) {
        if (SHGetPathFromIDListA(pidl, path)) {
            CoTaskMemFree(pidl);
            return _strdup(path);
        }
        CoTaskMemFree(pidl);
    }
    
    return NULL;
}

/* ============================================
   Color Dialog
   ============================================ */

static COLORREF g_customColors[16] = {0};

COLORREF halforms_color_dialog(COLORREF initial) {
    CHOOSECOLORA cc = {0};
    cc.lStructSize = sizeof(cc);
    cc.hwndOwner = g_halforms.mainForm ? g_halforms.mainForm->hwnd : NULL;
    cc.rgbResult = initial;
    cc.lpCustColors = g_customColors;
    cc.Flags = CC_FULLOPEN | CC_RGBINIT;
    
    if (ChooseColorA(&cc)) {
        return cc.rgbResult;
    }
    
    return initial;
}

/* ============================================
   Font Dialog
   ============================================ */

char* halforms_font_dialog(const char* initialFont, int initialSize) {
    LOGFONTA lf = {0};
    
    if (initialFont) {
        strncpy(lf.lfFaceName, initialFont, LF_FACESIZE - 1);
    }
    lf.lfHeight = -MulDiv(initialSize > 0 ? initialSize : 12, 
        GetDeviceCaps(GetDC(NULL), LOGPIXELSY), 72);
    
    CHOOSEFONTA cf = {0};
    cf.lStructSize = sizeof(cf);
    cf.hwndOwner = g_halforms.mainForm ? g_halforms.mainForm->hwnd : NULL;
    cf.lpLogFont = &lf;
    cf.Flags = CF_SCREENFONTS | CF_INITTOLOGFONTSTRUCT | CF_EFFECTS;
    
    if (ChooseFontA(&cf)) {
        /* Return font name and size as "FontName,Size" */
        char* result = (char*)malloc(LF_FACESIZE + 16);
        int size = MulDiv(-lf.lfHeight, 72, GetDeviceCaps(GetDC(NULL), LOGPIXELSY));
        snprintf(result, LF_FACESIZE + 16, "%s,%d", lf.lfFaceName, size);
        return result;
    }
    
    return NULL;
}

/* ============================================
   Input Dialog (Custom)
   ============================================ */

static char g_inputResult[1024] = {0};
static const char* g_inputPrompt = NULL;
static const char* g_inputDefault = NULL;

__attribute__((unused))
static INT_PTR CALLBACK InputDialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_INITDIALOG: {
            /* Center dialog */
            RECT rc, rcOwner;
            HWND owner = GetParent(hwnd);
            if (!owner) owner = GetDesktopWindow();
            GetWindowRect(owner, &rcOwner);
            GetWindowRect(hwnd, &rc);
            int x = rcOwner.left + (rcOwner.right - rcOwner.left - (rc.right - rc.left)) / 2;
            int y = rcOwner.top + (rcOwner.bottom - rcOwner.top - (rc.bottom - rc.top)) / 2;
            SetWindowPos(hwnd, NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
            
            /* Set prompt text */
            if (g_inputPrompt) {
                SetDlgItemTextA(hwnd, 101, g_inputPrompt);
            }
            
            /* Set default value */
            if (g_inputDefault) {
                SetDlgItemTextA(hwnd, 102, g_inputDefault);
            }
            
            return TRUE;
        }
        
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDOK:
                    GetDlgItemTextA(hwnd, 102, g_inputResult, sizeof(g_inputResult));
                    EndDialog(hwnd, IDOK);
                    return TRUE;
                case IDCANCEL:
                    g_inputResult[0] = '\0';
                    EndDialog(hwnd, IDCANCEL);
                    return TRUE;
            }
            break;
            
        case WM_CLOSE:
            EndDialog(hwnd, IDCANCEL);
            return TRUE;
    }
    return FALSE;
}

char* halforms_input_dialog(const char* title, const char* prompt, const char* defaultValue) {
    g_inputPrompt = prompt;
    g_inputDefault = defaultValue;
    g_inputResult[0] = '\0';
    
    /* Create dialog template in memory */
    /* This is a simplified approach - in production, use a resource or proper template */
    
    /* For now, use a simple MessageBox-based approach or create window manually */
    HWND parent = g_halforms.mainForm ? g_halforms.mainForm->hwnd : NULL;
    
    /* Create a simple input window */
    HWND dlg = CreateWindowExA(
        WS_EX_DLGMODALFRAME | WS_EX_TOPMOST,
        "STATIC",
        title ? title : "Input",
        WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, 300, 130,
        parent, NULL, g_halforms.hInstance, NULL
    );
    
    if (!dlg) return NULL;
    
    /* Add controls */
    CreateWindowExA(0, "STATIC", prompt ? prompt : "Enter value:",
        WS_CHILD | WS_VISIBLE, 10, 10, 280, 20, dlg, NULL, g_halforms.hInstance, NULL);
    
    HWND edit = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", defaultValue ? defaultValue : "",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
        10, 35, 280, 25, dlg, (HMENU)102, g_halforms.hInstance, NULL);
    
    CreateWindowExA(0, "BUTTON", "OK",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
        120, 70, 80, 25, dlg, (HMENU)IDOK, g_halforms.hInstance, NULL);
    
    CreateWindowExA(0, "BUTTON", "Cancel",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP,
        210, 70, 80, 25, dlg, (HMENU)IDCANCEL, g_halforms.hInstance, NULL);
    
    /* Center dialog */
    RECT rc;
    GetWindowRect(dlg, &rc);
    int w = rc.right - rc.left;
    int h = rc.bottom - rc.top;
    int x = (GetSystemMetrics(SM_CXSCREEN) - w) / 2;
    int y = (GetSystemMetrics(SM_CYSCREEN) - h) / 2;
    SetWindowPos(dlg, HWND_TOP, x, y, 0, 0, SWP_NOSIZE);
    
    SetFocus(edit);
    
    /* Simple message loop for modal dialog */
    MSG msg;
    bool done = false;
    bool ok = false;
    
    while (!done && GetMessage(&msg, NULL, 0, 0)) {
        if (msg.message == WM_KEYDOWN && msg.wParam == VK_RETURN) {
            GetWindowTextA(edit, g_inputResult, sizeof(g_inputResult));
            ok = true;
            done = true;
        } else if (msg.message == WM_KEYDOWN && msg.wParam == VK_ESCAPE) {
            done = true;
        } else if (msg.hwnd == dlg || IsChild(dlg, msg.hwnd)) {
            if (msg.message == WM_COMMAND) {
                if (LOWORD(msg.wParam) == IDOK) {
                    GetWindowTextA(edit, g_inputResult, sizeof(g_inputResult));
                    ok = true;
                    done = true;
                } else if (LOWORD(msg.wParam) == IDCANCEL) {
                    done = true;
                }
            }
        }
        
        if (!done) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    
    DestroyWindow(dlg);
    
    return ok ? _strdup(g_inputResult) : NULL;
}

/* ============================================
   Print Dialog
   ============================================ */

bool halforms_print_dialog(void) {
    PRINTDLGA pd = {0};
    pd.lStructSize = sizeof(pd);
    pd.hwndOwner = g_halforms.mainForm ? g_halforms.mainForm->hwnd : NULL;
    pd.Flags = PD_RETURNDC | PD_NOSELECTION;
    
    if (PrintDlgA(&pd)) {
        if (pd.hDC) {
            DeleteDC(pd.hDC);
        }
        return true;
    }
    
    return false;
}

/* ============================================
   Find/Replace Dialog (Simplified)
   ============================================ */

static HWND g_findDialog = NULL;
static char g_findText[256] = {0};
static char g_replaceText[256] = {0};

typedef void (*HalFindCallback)(const char* findText, const char* replaceText, bool findNext, bool replace, bool replaceAll);
static HalFindCallback g_findCallback = NULL;

void halforms_find_dialog(HalForm* parent, HalFindCallback callback) {
    g_findCallback = callback;
    
    FINDREPLACEA fr = {0};
    fr.lStructSize = sizeof(fr);
    fr.hwndOwner = parent ? parent->hwnd : NULL;
    fr.lpstrFindWhat = g_findText;
    fr.wFindWhatLen = sizeof(g_findText);
    fr.Flags = FR_DOWN;
    
    g_findDialog = FindTextA(&fr);
}

void halforms_replace_dialog(HalForm* parent, HalFindCallback callback) {
    g_findCallback = callback;
    
    FINDREPLACEA fr = {0};
    fr.lStructSize = sizeof(fr);
    fr.hwndOwner = parent ? parent->hwnd : NULL;
    fr.lpstrFindWhat = g_findText;
    fr.wFindWhatLen = sizeof(g_findText);
    fr.lpstrReplaceWith = g_replaceText;
    fr.wReplaceWithLen = sizeof(g_replaceText);
    fr.Flags = FR_DOWN;
    
    g_findDialog = ReplaceTextA(&fr);
}

/* ============================================
   Progress Dialog
   ============================================ */

typedef struct {
    HWND hwnd;
    HWND progressBar;
    HWND label;
    bool cancelled;
} HalProgressDialog;

static HalProgressDialog* g_progressDialog = NULL;

__attribute__((unused))
static LRESULT CALLBACK ProgressDialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_COMMAND:
            if (LOWORD(wParam) == IDCANCEL && g_progressDialog) {
                g_progressDialog->cancelled = true;
            }
            break;
        case WM_CLOSE:
            if (g_progressDialog) {
                g_progressDialog->cancelled = true;
            }
            return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

void* halforms_progress_start(const char* title, const char* message, bool canCancel) {
    HalProgressDialog* dlg = (HalProgressDialog*)calloc(1, sizeof(HalProgressDialog));
    if (!dlg) return NULL;
    
    HWND parent = g_halforms.mainForm ? g_halforms.mainForm->hwnd : NULL;
    
    dlg->hwnd = CreateWindowExA(
        WS_EX_DLGMODALFRAME | WS_EX_TOPMOST,
        "HalFormsWindow",
        title ? title : "Progress",
        WS_POPUP | WS_CAPTION | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, 350, 120,
        parent, NULL, g_halforms.hInstance, NULL
    );
    
    if (!dlg->hwnd) {
        free(dlg);
        return NULL;
    }
    
    /* Add label */
    dlg->label = CreateWindowExA(0, "STATIC", message ? message : "Please wait...",
        WS_CHILD | WS_VISIBLE, 10, 10, 330, 20, dlg->hwnd, NULL, g_halforms.hInstance, NULL);
    
    /* Add progress bar */
    dlg->progressBar = CreateWindowExA(0, PROGRESS_CLASSA, "",
        WS_CHILD | WS_VISIBLE | PBS_SMOOTH,
        10, 35, 330, 25, dlg->hwnd, NULL, g_halforms.hInstance, NULL);
    
    SendMessage(dlg->progressBar, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
    
    /* Add cancel button if allowed */
    if (canCancel) {
        CreateWindowExA(0, "BUTTON", "Cancel",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP,
            135, 70, 80, 25, dlg->hwnd, (HMENU)IDCANCEL, g_halforms.hInstance, NULL);
    }
    
    /* Center dialog */
    RECT rc;
    GetWindowRect(dlg->hwnd, &rc);
    int w = rc.right - rc.left;
    int h = rc.bottom - rc.top;
    int x = (GetSystemMetrics(SM_CXSCREEN) - w) / 2;
    int y = (GetSystemMetrics(SM_CYSCREEN) - h) / 2;
    SetWindowPos(dlg->hwnd, HWND_TOP, x, y, 0, 0, SWP_NOSIZE);
    
    g_progressDialog = dlg;
    return dlg;
}

void halforms_progress_update(void* handle, int percent, const char* message) {
    HalProgressDialog* dlg = (HalProgressDialog*)handle;
    if (!dlg) return;
    
    SendMessage(dlg->progressBar, PBM_SETPOS, percent, 0);
    
    if (message) {
        SetWindowTextA(dlg->label, message);
    }
    
    /* Process messages to keep UI responsive */
    MSG msg;
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

bool halforms_progress_cancelled(void* handle) {
    HalProgressDialog* dlg = (HalProgressDialog*)handle;
    return dlg ? dlg->cancelled : true;
}

void halforms_progress_end(void* handle) {
    HalProgressDialog* dlg = (HalProgressDialog*)handle;
    if (!dlg) return;
    
    if (dlg->hwnd) {
        DestroyWindow(dlg->hwnd);
    }
    
    if (g_progressDialog == dlg) {
        g_progressDialog = NULL;
    }
    
    free(dlg);
}
