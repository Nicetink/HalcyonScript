/*
 * HalForms - Code Editor Component
 * Professional code editor with syntax highlighting, line numbers, etc.
 * Uses RichEdit control with custom syntax highlighting
 */

#include "halforms.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <richedit.h>

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

/* RichEdit module handle */
static HMODULE g_richeditModule = NULL;
static bool g_richeditLoaded = false;

/* ============================================
   RichEdit Initialization
   ============================================ */

static bool halforms_load_richedit(void) {
    if (g_richeditLoaded) return true;
    
    /* Try to load RichEdit DLL */
    g_richeditModule = LoadLibraryA("Msftedit.dll");
    if (!g_richeditModule) {
        g_richeditModule = LoadLibraryA("Riched20.dll");
    }
    
    if (g_richeditModule) {
        g_richeditLoaded = true;
        return true;
    }
    
    return false;
}

/* ============================================
   Code Editor Colors
   ============================================ */

typedef struct {
    COLORREF background;
    COLORREF foreground;
    COLORREF lineNumbers;
    COLORREF keywords;
    COLORREF strings;
    COLORREF comments;
    COLORREF numbers;
    COLORREF operators;
    COLORREF functions;
    COLORREF types;
    COLORREF selection;
    COLORREF currentLine;
    COLORREF matchingBrace;
} HalCodeEditorTheme;

static HalCodeEditorTheme g_darkTheme = {
    0x1E1E1E,   /* background */
    0xD4D4D4,   /* foreground */
    0x858585,   /* lineNumbers */
    0x569CD6,   /* keywords */
    0xCE9178,   /* strings */
    0x6A9955,   /* comments */
    0xB5CEA8,   /* numbers */
    0xD4D4D4,   /* operators */
    0xDCDCAA,   /* functions */
    0x4EC9B0,   /* types */
    0x264F78,   /* selection */
    0x2D2D2D,   /* currentLine */
    0x0D6678    /* matchingBrace */
};

static HalCodeEditorTheme g_lightTheme = {
    0xFFFFFF,   /* background */
    0x000000,   /* foreground */
    0x237893,   /* lineNumbers */
    0x0000FF,   /* keywords */
    0xA31515,   /* strings */
    0x008000,   /* comments */
    0x098658,   /* numbers */
    0x000000,   /* operators */
    0x795E26,   /* functions */
    0x267F99,   /* types */
    0xADD6FF,   /* selection */
    0xFFFBCC,   /* currentLine */
    0xDBE0CC    /* matchingBrace */
};

/* ============================================
   HalcyonScript Keywords
   ============================================ */

static const char* g_halcyonKeywords[] = {
    "create", "window", "button", "label", "input", "textarea", "checkbox",
    "listbox", "dropdown", "slider", "progress", "panel", "treeview",
    "tabcontrol", "codeeditor", "menu", "toolbar", "statusbar",
    "when", "clicked", "changed", "started", "checked", "closed", "resized",
    "keydown", "keyup", "if", "else", "elseif", "elif", "while", "for",
    "from", "to", "step", "break", "continue", "func", "function", "return",
    "import", "export", "class", "new", "this", "extends", "var", "const",
    "let", "global", "true", "false", "null", "and", "or", "not",
    "print", "log", "debug", "alert", "set", "get", "show", "hide",
    "close", "open", "start", "stop", "timer", "interval", "async",
    "await", "try", "catch", "throw", "finally", NULL
};

/* Check if word is a keyword */
static bool is_keyword(const char* word) {
    for (int i = 0; g_halcyonKeywords[i] != NULL; i++) {
        if (strcmp(word, g_halcyonKeywords[i]) == 0) {
            return true;
        }
    }
    return false;
}

/* ============================================
   Code Editor Implementation
   ============================================ */

/* Configure RichEdit for code editing */
static void halcodeeditor_configure(HalCodeEditor* editor, bool darkMode) {
    HWND hwnd = editor->hwnd;
    HalCodeEditorTheme* theme = darkMode ? &g_darkTheme : &g_lightTheme;
    
    /* Set background color */
    SendMessageA(hwnd, EM_SETBKGNDCOLOR, 0, theme->background);
    
    /* Set default text format */
    CHARFORMATA cf = {0};
    cf.cbSize = sizeof(cf);
    cf.dwMask = CFM_FACE | CFM_SIZE | CFM_COLOR;
    strcpy(cf.szFaceName, "Consolas");
    cf.yHeight = 11 * 20; /* twips (1/20 of a point) */
    cf.crTextColor = theme->foreground;
    SendMessageA(hwnd, EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&cf);
    
    /* Enable auto URL detection */
    SendMessageA(hwnd, EM_AUTOURLDETECT, TRUE, 0);
    
    /* Set event mask for notifications */
    SendMessageA(hwnd, EM_SETEVENTMASK, 0, ENM_CHANGE | ENM_SELCHANGE | ENM_KEYEVENTS);
}

/* ============================================
   Public API
   ============================================ */

HalCodeEditor* halcodeeditor_create(HalForm* parent, int x, int y, int w, int h) {
    if (!parent) return NULL;
    
    HalCodeEditor* editor = (HalCodeEditor*)calloc(1, sizeof(HalCodeEditor));
    if (!editor) return NULL;
    
    editor->base.type = HALCTRL_RICHTEXTBOX;
    editor->base.x = x;
    editor->base.y = y;
    editor->base.width = w;
    editor->base.height = h;
    editor->base.visible = true;
    editor->base.enabled = true;
    editor->base.parent = parent;
    
    /* Default settings */
    editor->lineNumbers = true;
    editor->wordWrap = false;
    editor->tabSize = 4;
    editor->autoIndent = true;
    editor->bracketMatching = true;
    editor->language = _strdup("halcyon");
    
    /* Load RichEdit */
    halforms_load_richedit();
    
    /* Create RichEdit control */
    editor->hwnd = CreateWindowExA(
        WS_EX_CLIENTEDGE, 
        "RICHEDIT50W",  /* RichEdit 4.1 class */
        "",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | WS_HSCROLL |
        ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL | ES_WANTRETURN | ES_NOHIDESEL,
        x, y, w, h,
        parent->hwnd,
        (HMENU)(LONG_PTR)g_halforms.nextControlId++,
        g_halforms.hInstance,
        NULL
    );
    
    /* Fallback to older RichEdit if needed */
    if (!editor->hwnd) {
        editor->hwnd = CreateWindowExA(
            WS_EX_CLIENTEDGE,
            "RichEdit20A",
            "",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | WS_HSCROLL |
            ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL | ES_WANTRETURN | ES_NOHIDESEL,
            x, y, w, h,
            parent->hwnd,
            (HMENU)(LONG_PTR)g_halforms.nextControlId++,
            g_halforms.hInstance,
            NULL
        );
    }
    
    if (!editor->hwnd) {
        free(editor->language);
        free(editor);
        return NULL;
    }
    
    editor->base.hwnd = editor->hwnd;
    
    /* Configure editor */
    halcodeeditor_configure(editor, true); /* Dark mode by default */
    
    /* Set text limit to large value */
    SendMessageA(editor->hwnd, EM_EXLIMITTEXT, 0, 0x7FFFFFFF);
    
    /* Add to parent */
    if (parent->base.childCount >= parent->base.childCapacity) {
        parent->base.childCapacity = parent->base.childCapacity == 0 ? 16 : parent->base.childCapacity * 2;
        parent->base.children = (HalControl**)realloc(parent->base.children,
            parent->base.childCapacity * sizeof(HalControl*));
    }
    parent->base.children[parent->base.childCount++] = (HalControl*)editor;
    
    return editor;
}

void halcodeeditor_set_text(HalCodeEditor* editor, const char* text) {
    if (!editor || !editor->hwnd) return;
    SetWindowTextA(editor->hwnd, text ? text : "");
}

char* halcodeeditor_get_text(HalCodeEditor* editor) {
    if (!editor || !editor->hwnd) return NULL;
    
    int len = GetWindowTextLengthA(editor->hwnd);
    char* text = (char*)malloc(len + 1);
    if (text) {
        GetWindowTextA(editor->hwnd, text, len + 1);
    }
    return text;
}

void halcodeeditor_set_language(HalCodeEditor* editor, const char* language) {
    if (!editor) return;
    
    free(editor->language);
    editor->language = _strdup(language ? language : "text");
}

void halcodeeditor_goto_line(HalCodeEditor* editor, int line) {
    if (!editor || !editor->hwnd || line < 1) return;
    
    int charIndex = (int)SendMessageA(editor->hwnd, EM_LINEINDEX, line - 1, 0);
    if (charIndex >= 0) {
        SendMessageA(editor->hwnd, EM_SETSEL, charIndex, charIndex);
        SendMessageA(editor->hwnd, EM_SCROLLCARET, 0, 0);
    }
}

void halcodeeditor_set_selection(HalCodeEditor* editor, int start, int end) {
    if (!editor || !editor->hwnd) return;
    SendMessageA(editor->hwnd, EM_SETSEL, start, end);
}

void halcodeeditor_insert_text(HalCodeEditor* editor, const char* text) {
    if (!editor || !editor->hwnd || !text) return;
    SendMessageA(editor->hwnd, EM_REPLACESEL, TRUE, (LPARAM)text);
}

/* ============================================
   Extended Code Editor Functions
   ============================================ */

void halcodeeditor_undo(HalCodeEditor* editor) {
    if (!editor || !editor->hwnd) return;
    SendMessageA(editor->hwnd, EM_UNDO, 0, 0);
}

void halcodeeditor_redo(HalCodeEditor* editor) {
    if (!editor || !editor->hwnd) return;
    SendMessageA(editor->hwnd, EM_REDO, 0, 0);
}

bool halcodeeditor_can_undo(HalCodeEditor* editor) {
    if (!editor || !editor->hwnd) return false;
    return SendMessageA(editor->hwnd, EM_CANUNDO, 0, 0) != 0;
}

bool halcodeeditor_can_redo(HalCodeEditor* editor) {
    if (!editor || !editor->hwnd) return false;
    return SendMessageA(editor->hwnd, EM_CANREDO, 0, 0) != 0;
}

void halcodeeditor_cut(HalCodeEditor* editor) {
    if (!editor || !editor->hwnd) return;
    SendMessageA(editor->hwnd, WM_CUT, 0, 0);
}

void halcodeeditor_copy(HalCodeEditor* editor) {
    if (!editor || !editor->hwnd) return;
    SendMessageA(editor->hwnd, WM_COPY, 0, 0);
}

void halcodeeditor_paste(HalCodeEditor* editor) {
    if (!editor || !editor->hwnd) return;
    SendMessageA(editor->hwnd, WM_PASTE, 0, 0);
}

void halcodeeditor_select_all(HalCodeEditor* editor) {
    if (!editor || !editor->hwnd) return;
    SendMessageA(editor->hwnd, EM_SETSEL, 0, -1);
}

int halcodeeditor_get_line_count(HalCodeEditor* editor) {
    if (!editor || !editor->hwnd) return 0;
    return (int)SendMessageA(editor->hwnd, EM_GETLINECOUNT, 0, 0);
}

int halcodeeditor_get_current_line(HalCodeEditor* editor) {
    if (!editor || !editor->hwnd) return 0;
    
    DWORD start = 0;
    SendMessageA(editor->hwnd, EM_GETSEL, (WPARAM)&start, 0);
    return (int)SendMessageA(editor->hwnd, EM_LINEFROMCHAR, start, 0) + 1;
}

int halcodeeditor_get_current_column(HalCodeEditor* editor) {
    if (!editor || !editor->hwnd) return 0;
    
    DWORD start = 0;
    SendMessageA(editor->hwnd, EM_GETSEL, (WPARAM)&start, 0);
    int line = (int)SendMessageA(editor->hwnd, EM_LINEFROMCHAR, start, 0);
    int lineStart = (int)SendMessageA(editor->hwnd, EM_LINEINDEX, line, 0);
    return (int)(start - lineStart) + 1;
}

void halcodeeditor_set_readonly(HalCodeEditor* editor, bool readonly) {
    if (!editor || !editor->hwnd) return;
    SendMessageA(editor->hwnd, EM_SETREADONLY, readonly, 0);
}

void halcodeeditor_set_modified(HalCodeEditor* editor, bool modified) {
    if (!editor || !editor->hwnd) return;
    SendMessageA(editor->hwnd, EM_SETMODIFY, modified, 0);
}

bool halcodeeditor_is_modified(HalCodeEditor* editor) {
    if (!editor || !editor->hwnd) return false;
    return SendMessageA(editor->hwnd, EM_GETMODIFY, 0, 0) != 0;
}

/* Find and Replace */
int halcodeeditor_find(HalCodeEditor* editor, const char* text, bool matchCase, bool wholeWord, bool forward) {
    if (!editor || !editor->hwnd || !text || !text[0]) return -1;
    
    FINDTEXTA ft = {0};
    DWORD flags = forward ? FR_DOWN : 0;
    if (matchCase) flags |= FR_MATCHCASE;
    if (wholeWord) flags |= FR_WHOLEWORD;
    
    DWORD start = 0, end = 0;
    SendMessageA(editor->hwnd, EM_GETSEL, (WPARAM)&start, (LPARAM)&end);
    
    int textLen = GetWindowTextLengthA(editor->hwnd);
    
    if (forward) {
        ft.chrg.cpMin = end;
        ft.chrg.cpMax = textLen;
    } else {
        ft.chrg.cpMin = start;
        ft.chrg.cpMax = 0;
    }
    ft.lpstrText = (char*)text;
    
    int pos = (int)SendMessageA(editor->hwnd, EM_FINDTEXT, flags, (LPARAM)&ft);
    
    if (pos >= 0) {
        SendMessageA(editor->hwnd, EM_SETSEL, pos, pos + (int)strlen(text));
        SendMessageA(editor->hwnd, EM_SCROLLCARET, 0, 0);
    }
    
    return pos;
}

int halcodeeditor_replace(HalCodeEditor* editor, const char* findText, const char* replaceText, bool matchCase) {
    if (!editor || !editor->hwnd || !findText || !replaceText) return 0;
    
    DWORD start = 0, end = 0;
    SendMessageA(editor->hwnd, EM_GETSEL, (WPARAM)&start, (LPARAM)&end);
    
    if (start != end) {
        SendMessageA(editor->hwnd, EM_REPLACESEL, TRUE, (LPARAM)replaceText);
        return 1;
    }
    
    return 0;
}

int halcodeeditor_replace_all(HalCodeEditor* editor, const char* findText, const char* replaceText, bool matchCase) {
    if (!editor || !editor->hwnd || !findText || !replaceText || !findText[0]) return 0;
    
    int count = 0;
    int findLen = (int)strlen(findText);
    
    /* Start from beginning */
    SendMessageA(editor->hwnd, EM_SETSEL, 0, 0);
    
    while (halcodeeditor_find(editor, findText, matchCase, false, true) >= 0) {
        SendMessageA(editor->hwnd, EM_REPLACESEL, TRUE, (LPARAM)replaceText);
        count++;
        
        /* Safety limit */
        if (count > 100000) break;
    }
    
    return count;
}

/* Bookmarks - stored in editor struct */
void halcodeeditor_toggle_bookmark(HalCodeEditor* editor, int line) {
    /* Bookmarks would need custom implementation */
    /* For now, this is a placeholder */
    (void)editor;
    (void)line;
}

void halcodeeditor_goto_next_bookmark(HalCodeEditor* editor) {
    /* Placeholder */
    (void)editor;
}

/* Folding - not supported in RichEdit */
void halcodeeditor_fold_all(HalCodeEditor* editor) {
    (void)editor;
}

void halcodeeditor_unfold_all(HalCodeEditor* editor) {
    (void)editor;
}

/* Theme */
void halcodeeditor_set_dark_mode(HalCodeEditor* editor, bool dark) {
    if (!editor || !editor->hwnd) return;
    halcodeeditor_configure(editor, dark);
    InvalidateRect(editor->hwnd, NULL, TRUE);
}
