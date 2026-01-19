/*
 * HalForms - Basic Controls Implementation
 * Standard Windows Forms controls
 */

#include "halforms.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
   Helper Functions
   ============================================ */

static void halctrl_add_to_form(HalForm* form, HalControl* ctrl) {
    if (!form || !ctrl) return;
    
    if (form->base.childCount >= form->base.childCapacity) {
        form->base.childCapacity = form->base.childCapacity == 0 ? 16 : form->base.childCapacity * 2;
        form->base.children = (HalControl**)realloc(form->base.children, 
            form->base.childCapacity * sizeof(HalControl*));
    }
    form->base.children[form->base.childCount++] = ctrl;
    ctrl->parent = form;
}

static HalControl* halctrl_create_base(HalForm* parent, HalControlType type, 
    const char* className, const char* text, DWORD style, DWORD exStyle,
    int x, int y, int w, int h) {
    
    HalControl* ctrl = (HalControl*)calloc(1, sizeof(HalControl));
    if (!ctrl) return NULL;
    
    ctrl->type = type;
    ctrl->text = text ? _strdup(text) : NULL;
    ctrl->x = x;
    ctrl->y = y;
    ctrl->width = w;
    ctrl->height = h;
    ctrl->visible = true;
    ctrl->enabled = true;
    ctrl->dock = HALDOCK_NONE;
    ctrl->anchor = HALANCHOR_TOP | HALANCHOR_LEFT;
    
    int id = g_halforms.nextControlId++;
    
    /* Convert to wide strings for Unicode support */
    wchar_t* wClassName = utf8_to_wide(className);
    wchar_t* wText = utf8_to_wide(text);
    
    ctrl->hwnd = CreateWindowExW(
        exStyle,
        wClassName ? wClassName : L"STATIC",
        wText ? wText : L"",
        style | WS_CHILD | WS_VISIBLE,
        x, y, w, h,
        parent->hwnd,
        (HMENU)(LONG_PTR)id,
        g_halforms.hInstance,
        NULL
    );
    
    free(wClassName);
    free(wText);
    
    if (!ctrl->hwnd) {
        free(ctrl->text);
        free(ctrl);
        return NULL;
    }
    
    /* Set default font */
    SendMessage(ctrl->hwnd, WM_SETFONT, (WPARAM)g_halforms.defaultFont, TRUE);
    
    halctrl_add_to_form(parent, ctrl);
    return ctrl;
}

/* ============================================
   Basic Controls
   ============================================ */

HalControl* halctrl_label(HalForm* parent, const char* text, int x, int y, int w, int h) {
    return halctrl_create_base(parent, HALCTRL_LABEL, "STATIC", text,
        SS_LEFT, 0, x, y, w, h);
}

HalControl* halctrl_button(HalForm* parent, const char* text, int x, int y, int w, int h) {
    return halctrl_create_base(parent, HALCTRL_BUTTON, "BUTTON", text,
        BS_PUSHBUTTON | BS_TEXT, 0, x, y, w, h);
}

HalControl* halctrl_textbox(HalForm* parent, const char* text, int x, int y, int w, int h) {
    return halctrl_create_base(parent, HALCTRL_TEXTBOX, "EDIT", text,
        ES_LEFT | ES_AUTOHSCROLL | WS_BORDER | WS_TABSTOP, 
        WS_EX_CLIENTEDGE, x, y, w, h);
}

HalControl* halctrl_textbox_multiline(HalForm* parent, int x, int y, int w, int h) {
    return halctrl_create_base(parent, HALCTRL_TEXTBOX, "EDIT", "",
        ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | ES_WANTRETURN | 
        WS_BORDER | WS_TABSTOP | WS_VSCROLL,
        WS_EX_CLIENTEDGE, x, y, w, h);
}

HalControl* halctrl_checkbox(HalForm* parent, const char* text, int x, int y, int w, int h) {
    return halctrl_create_base(parent, HALCTRL_CHECKBOX, "BUTTON", text,
        BS_AUTOCHECKBOX | WS_TABSTOP, 0, x, y, w, h);
}

HalControl* halctrl_radiobutton(HalForm* parent, const char* text, int x, int y, int w, int h) {
    return halctrl_create_base(parent, HALCTRL_RADIOBUTTON, "BUTTON", text,
        BS_AUTORADIOBUTTON | WS_TABSTOP, 0, x, y, w, h);
}

HalControl* halctrl_combobox(HalForm* parent, int x, int y, int w, int h) {
    return halctrl_create_base(parent, HALCTRL_COMBOBOX, "COMBOBOX", "",
        CBS_DROPDOWNLIST | CBS_HASSTRINGS | WS_TABSTOP | WS_VSCROLL,
        0, x, y, w, h);
}

HalControl* halctrl_listbox(HalForm* parent, int x, int y, int w, int h) {
    return halctrl_create_base(parent, HALCTRL_LISTBOX, "LISTBOX", "",
        LBS_STANDARD | LBS_HASSTRINGS | WS_TABSTOP | WS_VSCROLL,
        WS_EX_CLIENTEDGE, x, y, w, h);
}

HalControl* halctrl_groupbox(HalForm* parent, const char* text, int x, int y, int w, int h) {
    return halctrl_create_base(parent, HALCTRL_GROUPBOX, "BUTTON", text,
        BS_GROUPBOX, 0, x, y, w, h);
}

HalControl* halctrl_panel(HalForm* parent, int x, int y, int w, int h) {
    return halctrl_create_base(parent, HALCTRL_PANEL, "STATIC", "",
        SS_ETCHEDFRAME, 0, x, y, w, h);
}

HalControl* halctrl_progressbar(HalForm* parent, int x, int y, int w, int h) {
    return halctrl_create_base(parent, HALCTRL_PROGRESSBAR, "msctls_progress32", "",
        PBS_SMOOTH, 0, x, y, w, h);
}

HalControl* halctrl_trackbar(HalForm* parent, int x, int y, int w, int h, int min, int max) {
    HalControl* ctrl = halctrl_create_base(parent, HALCTRL_TRACKBAR, "msctls_trackbar32", "",
        TBS_AUTOTICKS | TBS_HORZ | WS_TABSTOP, 0, x, y, w, h);
    
    if (ctrl) {
        SendMessage(ctrl->hwnd, TBM_SETRANGE, TRUE, MAKELPARAM(min, max));
    }
    return ctrl;
}

HalControl* halctrl_numericupdown(HalForm* parent, int x, int y, int w, int h, int min, int max) {
    /* Create edit control first */
    HalControl* edit = halctrl_create_base(parent, HALCTRL_TEXTBOX, "EDIT", "0",
        ES_NUMBER | ES_LEFT | WS_BORDER | WS_TABSTOP,
        WS_EX_CLIENTEDGE, x, y, w - 20, h);
    
    if (!edit) return NULL;
    
    /* Create up-down control */
    HWND updown = CreateWindowExA(
        0, "msctls_updown32", "",
        UDS_ALIGNRIGHT | UDS_ARROWKEYS | UDS_SETBUDDYINT | WS_CHILD | WS_VISIBLE,
        0, 0, 0, 0,
        parent->hwnd,
        NULL,
        g_halforms.hInstance,
        NULL
    );
    
    if (updown) {
        SendMessage(updown, UDM_SETBUDDY, (WPARAM)edit->hwnd, 0);
        SendMessage(updown, UDM_SETRANGE32, min, max);
    }
    
    edit->type = HALCTRL_NUMERICUPDOWN;
    return edit;
}

/* ============================================
   Control Properties
   ============================================ */

void halctrl_set_text(HalControl* ctrl, const char* text) {
    if (!ctrl) return;
    free(ctrl->text);
    ctrl->text = text ? _strdup(text) : NULL;
    wchar_t* wtext = utf8_to_wide(text);
    SetWindowTextW(ctrl->hwnd, wtext ? wtext : L"");
    free(wtext);
}

char* halctrl_get_text(HalControl* ctrl) {
    if (!ctrl) return NULL;
    
    int len = GetWindowTextLengthW(ctrl->hwnd);
    wchar_t* wtext = (wchar_t*)malloc((len + 1) * sizeof(wchar_t));
    GetWindowTextW(ctrl->hwnd, wtext, len + 1);
    char* text = wide_to_utf8(wtext);
    free(wtext);
    return text;
}

void halctrl_set_visible(HalControl* ctrl, bool visible) {
    if (!ctrl) return;
    ctrl->visible = visible;
    ShowWindow(ctrl->hwnd, visible ? SW_SHOW : SW_HIDE);
}

void halctrl_set_enabled(HalControl* ctrl, bool enabled) {
    if (!ctrl) return;
    ctrl->enabled = enabled;
    EnableWindow(ctrl->hwnd, enabled);
}

void halctrl_set_bounds(HalControl* ctrl, int x, int y, int w, int h) {
    if (!ctrl) return;
    ctrl->x = x;
    ctrl->y = y;
    ctrl->width = w;
    ctrl->height = h;
    SetWindowPos(ctrl->hwnd, NULL, x, y, w, h, SWP_NOZORDER);
}

void halctrl_set_dock(HalControl* ctrl, HalDockStyle dock) {
    if (ctrl) ctrl->dock = dock;
}

void halctrl_set_anchor(HalControl* ctrl, HalAnchorStyle anchor) {
    if (ctrl) ctrl->anchor = anchor;
}

void halctrl_set_font(HalControl* ctrl, const char* fontName, int size, bool bold, bool italic) {
    if (!ctrl) return;
    
    HFONT font = halforms_create_font(fontName, size, bold, italic);
    if (font) {
        SendMessage(ctrl->hwnd, WM_SETFONT, (WPARAM)font, TRUE);
    }
}

void halctrl_set_colors(HalControl* ctrl, COLORREF foreground, COLORREF background) {
    /* Note: Requires custom drawing for most controls */
    if (!ctrl) return;
    InvalidateRect(ctrl->hwnd, NULL, TRUE);
}

/* Checkbox/RadioButton */
void halctrl_set_checked(HalControl* ctrl, bool checked) {
    if (!ctrl) return;
    SendMessage(ctrl->hwnd, BM_SETCHECK, checked ? BST_CHECKED : BST_UNCHECKED, 0);
}

bool halctrl_get_checked(HalControl* ctrl) {
    if (!ctrl) return false;
    return SendMessage(ctrl->hwnd, BM_GETCHECK, 0, 0) == BST_CHECKED;
}

/* ComboBox/ListBox */
void halctrl_add_item(HalControl* ctrl, const char* item) {
    if (!ctrl || !item) return;
    
    if (ctrl->type == HALCTRL_COMBOBOX) {
        SendMessageA(ctrl->hwnd, CB_ADDSTRING, 0, (LPARAM)item);
    } else if (ctrl->type == HALCTRL_LISTBOX) {
        SendMessageA(ctrl->hwnd, LB_ADDSTRING, 0, (LPARAM)item);
    }
}

void halctrl_remove_item(HalControl* ctrl, int index) {
    if (!ctrl) return;
    
    if (ctrl->type == HALCTRL_COMBOBOX) {
        SendMessage(ctrl->hwnd, CB_DELETESTRING, index, 0);
    } else if (ctrl->type == HALCTRL_LISTBOX) {
        SendMessage(ctrl->hwnd, LB_DELETESTRING, index, 0);
    }
}

void halctrl_clear_items(HalControl* ctrl) {
    if (!ctrl) return;
    
    if (ctrl->type == HALCTRL_COMBOBOX) {
        SendMessage(ctrl->hwnd, CB_RESETCONTENT, 0, 0);
    } else if (ctrl->type == HALCTRL_LISTBOX) {
        SendMessage(ctrl->hwnd, LB_RESETCONTENT, 0, 0);
    }
}

int halctrl_get_selected_index(HalControl* ctrl) {
    if (!ctrl) return -1;
    
    if (ctrl->type == HALCTRL_COMBOBOX) {
        return (int)SendMessage(ctrl->hwnd, CB_GETCURSEL, 0, 0);
    } else if (ctrl->type == HALCTRL_LISTBOX) {
        return (int)SendMessage(ctrl->hwnd, LB_GETCURSEL, 0, 0);
    }
    return -1;
}

void halctrl_set_selected_index(HalControl* ctrl, int index) {
    if (!ctrl) return;
    
    if (ctrl->type == HALCTRL_COMBOBOX) {
        SendMessage(ctrl->hwnd, CB_SETCURSEL, index, 0);
    } else if (ctrl->type == HALCTRL_LISTBOX) {
        SendMessage(ctrl->hwnd, LB_SETCURSEL, index, 0);
    }
}

char* halctrl_get_selected_item(HalControl* ctrl) {
    if (!ctrl) return NULL;
    
    int index = halctrl_get_selected_index(ctrl);
    if (index < 0) return NULL;
    
    int len = 0;
    if (ctrl->type == HALCTRL_COMBOBOX) {
        len = (int)SendMessage(ctrl->hwnd, CB_GETLBTEXTLEN, index, 0);
    } else if (ctrl->type == HALCTRL_LISTBOX) {
        len = (int)SendMessage(ctrl->hwnd, LB_GETTEXTLEN, index, 0);
    }
    
    if (len <= 0) return NULL;
    
    char* text = (char*)malloc(len + 1);
    if (ctrl->type == HALCTRL_COMBOBOX) {
        SendMessageA(ctrl->hwnd, CB_GETLBTEXT, index, (LPARAM)text);
    } else if (ctrl->type == HALCTRL_LISTBOX) {
        SendMessageA(ctrl->hwnd, LB_GETTEXT, index, (LPARAM)text);
    }
    
    return text;
}

/* ProgressBar/TrackBar */
void halctrl_set_value(HalControl* ctrl, int value) {
    if (!ctrl) return;
    
    if (ctrl->type == HALCTRL_PROGRESSBAR) {
        SendMessage(ctrl->hwnd, PBM_SETPOS, value, 0);
    } else if (ctrl->type == HALCTRL_TRACKBAR) {
        SendMessage(ctrl->hwnd, TBM_SETPOS, TRUE, value);
    }
}

int halctrl_get_value(HalControl* ctrl) {
    if (!ctrl) return 0;
    
    if (ctrl->type == HALCTRL_PROGRESSBAR) {
        return (int)SendMessage(ctrl->hwnd, PBM_GETPOS, 0, 0);
    } else if (ctrl->type == HALCTRL_TRACKBAR) {
        return (int)SendMessage(ctrl->hwnd, TBM_GETPOS, 0, 0);
    }
    return 0;
}

void halctrl_set_range(HalControl* ctrl, int min, int max) {
    if (!ctrl) return;
    
    if (ctrl->type == HALCTRL_PROGRESSBAR) {
        SendMessage(ctrl->hwnd, PBM_SETRANGE32, min, max);
    } else if (ctrl->type == HALCTRL_TRACKBAR) {
        SendMessage(ctrl->hwnd, TBM_SETRANGE, TRUE, MAKELPARAM(min, max));
    }
}

/* ============================================
   Event Handling
   ============================================ */

void halctrl_on_click(HalControl* ctrl, HalEventHandler handler, void* userData) {
    if (!ctrl) return;
    ctrl->onClick = handler;
    ctrl->eventUserData = userData;
}

void halctrl_on_doubleclick(HalControl* ctrl, HalEventHandler handler, void* userData) {
    if (!ctrl) return;
    ctrl->onDoubleClick = handler;
    ctrl->eventUserData = userData;
}

void halctrl_on_textchanged(HalControl* ctrl, HalEventHandler handler, void* userData) {
    if (!ctrl) return;
    ctrl->onTextChanged = handler;
    ctrl->eventUserData = userData;
}

void halctrl_on_keydown(HalControl* ctrl, HalEventHandler handler, void* userData) {
    if (!ctrl) return;
    ctrl->onKeyDown = handler;
    ctrl->eventUserData = userData;
}

void halctrl_on_mousedown(HalControl* ctrl, HalEventHandler handler, void* userData) {
    if (!ctrl) return;
    ctrl->onMouseDown = handler;
    ctrl->eventUserData = userData;
}

void halform_on_load(HalForm* form, HalEventHandler handler, void* userData) {
    if (!form) return;
    form->onLoad = handler;
    form->base.eventUserData = userData;
}

void halform_on_closing(HalForm* form, HalEventHandler handler, void* userData) {
    if (!form) return;
    form->onClosing = handler;
    form->base.eventUserData = userData;
}

void halform_on_resize(HalForm* form, HalEventHandler handler, void* userData) {
    if (!form) return;
    form->onResize = handler;
    form->base.eventUserData = userData;
}
