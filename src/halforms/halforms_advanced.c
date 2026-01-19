/*
 * HalForms - Advanced Controls Implementation
 *
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

/* Load RichEdit library for code editor */
static HMODULE g_richEditLib = NULL;

static void halforms_load_richedit(void) {
    if (!g_richEditLib) {
        g_richEditLib = LoadLibraryA("Msftedit.dll");
        if (!g_richEditLib) {
            g_richEditLib = LoadLibraryA("Riched20.dll");
        }
    }
}

/* ============================================
   Code Editor Implementation
   ============================================ */

HalCodeEditor* halcodeeditor_create(HalForm* parent, int x, int y, int w, int h) {
    if (!parent) return NULL;
    
    halforms_load_richedit();
    
    HalCodeEditor* editor = (HalCodeEditor*)calloc(1, sizeof(HalCodeEditor));
    if (!editor) return NULL;
    
    editor->base.type = HALCTRL_RICHTEXTBOX;
    editor->base.x = x;
    editor->base.y = y;
    editor->base.width = w;
    editor->base.height = h;
    editor->base.visible = true;
    editor->base.enabled = true;
    
    /* Default settings */
    editor->lineNumbers = true;
    editor->tabSize = 4;
    editor->autoIndent = true;
    editor->bracketMatching = true;
    editor->bgColor = RGB(30, 30, 30);
    editor->textColor = RGB(220, 220, 220);
    editor->lineNumColor = RGB(100, 100, 100);
    editor->keywordColor = RGB(86, 156, 214);
    editor->stringColor = RGB(206, 145, 120);
    editor->commentColor = RGB(106, 153, 85);
    editor->numberColor = RGB(181, 206, 168);
    
    /* Create RichEdit control */
    const char* className = g_richEditLib ? "RICHEDIT50W" : "RichEdit20A";
    
    editor->hwnd = CreateWindowExA(
        WS_EX_CLIENTEDGE,
        className,
        "",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL |
        ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL | ES_WANTRETURN,
        x, y, w, h,
        parent->hwnd,
        (HMENU)(LONG_PTR)g_halforms.nextControlId++,
        g_halforms.hInstance,
        NULL
    );
    
    if (!editor->hwnd) {
        free(editor);
        return NULL;
    }
    
    editor->base.hwnd = editor->hwnd;
    
    /* Set monospace font */
    HFONT monoFont = CreateFontA(
        -14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, FIXED_PITCH | FF_MODERN, "Consolas"
    );
    SendMessage(editor->hwnd, WM_SETFONT, (WPARAM)monoFont, TRUE);
    
    /* Set colors */
    SendMessage(editor->hwnd, EM_SETBKGNDCOLOR, 0, editor->bgColor);
    
    /* Set text color */
    CHARFORMAT2A cf = {0};
    cf.cbSize = sizeof(cf);
    cf.dwMask = CFM_COLOR;
    cf.crTextColor = editor->textColor;
    SendMessageA(editor->hwnd, EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&cf);
    
    /* Enable auto URL detection */
    SendMessage(editor->hwnd, EM_AUTOURLDETECT, TRUE, 0);
    
    /* Set tab stops (4 characters) */
    int tabStop = editor->tabSize * 4;
    SendMessage(editor->hwnd, EM_SETTABSTOPS, 1, (LPARAM)&tabStop);
    
    return editor;
}

void halcodeeditor_set_text(HalCodeEditor* editor, const char* text) {
    if (!editor) return;
    SetWindowTextA(editor->hwnd, text ? text : "");
}

char* halcodeeditor_get_text(HalCodeEditor* editor) {
    if (!editor) return NULL;
    
    int len = GetWindowTextLengthA(editor->hwnd);
    char* text = (char*)malloc(len + 1);
    GetWindowTextA(editor->hwnd, text, len + 1);
    return text;
}

void halcodeeditor_set_language(HalCodeEditor* editor, const char* language) {
    if (!editor) return;
    free(editor->language);
    editor->language = language ? _strdup(language) : NULL;
    /* Syntax highlighting would be applied here */
}

void halcodeeditor_goto_line(HalCodeEditor* editor, int line) {
    if (!editor || line < 1) return;
    
    int charIndex = (int)SendMessage(editor->hwnd, EM_LINEINDEX, line - 1, 0);
    SendMessage(editor->hwnd, EM_SETSEL, charIndex, charIndex);
    SendMessage(editor->hwnd, EM_SCROLLCARET, 0, 0);
}

void halcodeeditor_set_selection(HalCodeEditor* editor, int start, int end) {
    if (!editor) return;
    SendMessage(editor->hwnd, EM_SETSEL, start, end);
}

void halcodeeditor_insert_text(HalCodeEditor* editor, const char* text) {
    if (!editor || !text) return;
    SendMessageA(editor->hwnd, EM_REPLACESEL, TRUE, (LPARAM)text);
}

/* ============================================
   Tree View Implementation
   ============================================ */

HalTreeView* haltreeview_create(HalForm* parent, int x, int y, int w, int h) {
    if (!parent) return NULL;
    
    HalTreeView* tree = (HalTreeView*)calloc(1, sizeof(HalTreeView));
    if (!tree) return NULL;
    
    tree->base.type = HALCTRL_CUSTOM;
    tree->base.x = x;
    tree->base.y = y;
    tree->base.width = w;
    tree->base.height = h;
    tree->base.visible = true;
    tree->base.enabled = true;
    
    tree->hwnd = CreateWindowExA(
        WS_EX_CLIENTEDGE,
        WC_TREEVIEWA,
        "",
        WS_CHILD | WS_VISIBLE | WS_BORDER |
        TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS | TVS_SHOWSELALWAYS,
        x, y, w, h,
        parent->hwnd,
        (HMENU)(LONG_PTR)g_halforms.nextControlId++,
        g_halforms.hInstance,
        NULL
    );
    
    if (!tree->hwnd) {
        free(tree);
        return NULL;
    }
    
    tree->base.hwnd = tree->hwnd;
    SendMessage(tree->hwnd, WM_SETFONT, (WPARAM)g_halforms.defaultFont, TRUE);
    
    return tree;
}

HTREEITEM haltreeview_add_node(HalTreeView* tree, HTREEITEM parent, const char* text, int imageIndex) {
    if (!tree) return NULL;
    
    TVINSERTSTRUCTA tvis = {0};
    tvis.hParent = parent ? parent : TVI_ROOT;
    tvis.hInsertAfter = TVI_LAST;
    tvis.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
    tvis.item.pszText = (LPSTR)text;
    tvis.item.iImage = imageIndex;
    tvis.item.iSelectedImage = imageIndex;
    
    return (HTREEITEM)SendMessageA(tree->hwnd, TVM_INSERTITEMA, 0, (LPARAM)&tvis);
}

void haltreeview_remove_node(HalTreeView* tree, HTREEITEM node) {
    if (!tree || !node) return;
    SendMessage(tree->hwnd, TVM_DELETEITEM, 0, (LPARAM)node);
}

void haltreeview_clear(HalTreeView* tree) {
    if (!tree) return;
    SendMessage(tree->hwnd, TVM_DELETEITEM, 0, (LPARAM)TVI_ROOT);
}

void haltreeview_expand_node(HalTreeView* tree, HTREEITEM node) {
    if (!tree || !node) return;
    SendMessage(tree->hwnd, TVM_EXPAND, TVE_EXPAND, (LPARAM)node);
}

void haltreeview_collapse_node(HalTreeView* tree, HTREEITEM node) {
    if (!tree || !node) return;
    SendMessage(tree->hwnd, TVM_EXPAND, TVE_COLLAPSE, (LPARAM)node);
}

HTREEITEM haltreeview_get_selected(HalTreeView* tree) {
    if (!tree) return NULL;
    return (HTREEITEM)SendMessage(tree->hwnd, TVM_GETNEXTITEM, TVGN_CARET, 0);
}

/* ============================================
   List View Implementation
   ============================================ */

HalListView* hallistview_create(HalForm* parent, int x, int y, int w, int h) {
    if (!parent) return NULL;
    
    HalListView* list = (HalListView*)calloc(1, sizeof(HalListView));
    if (!list) return NULL;
    
    list->base.type = HALCTRL_CUSTOM;
    list->base.x = x;
    list->base.y = y;
    list->base.width = w;
    list->base.height = h;
    list->base.visible = true;
    list->base.enabled = true;
    list->viewStyle = HALVIEW_DETAILS;
    
    list->hwnd = CreateWindowExA(
        WS_EX_CLIENTEDGE,
        WC_LISTVIEWA,
        "",
        WS_CHILD | WS_VISIBLE | WS_BORDER |
        LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SINGLESEL,
        x, y, w, h,
        parent->hwnd,
        (HMENU)(LONG_PTR)g_halforms.nextControlId++,
        g_halforms.hInstance,
        NULL
    );
    
    if (!list->hwnd) {
        free(list);
        return NULL;
    }
    
    list->base.hwnd = list->hwnd;
    SendMessage(list->hwnd, WM_SETFONT, (WPARAM)g_halforms.defaultFont, TRUE);
    
    /* Enable full row select and grid lines */
    ListView_SetExtendedListViewStyle(list->hwnd, 
        LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);
    
    return list;
}

void hallistview_set_view(HalListView* list, HalListViewStyle style) {
    if (!list) return;
    
    list->viewStyle = style;
    
    DWORD viewStyle = LV_VIEW_DETAILS;
    switch (style) {
        case HALVIEW_ICON: viewStyle = LV_VIEW_ICON; break;
        case HALVIEW_SMALLICON: viewStyle = LV_VIEW_SMALLICON; break;
        case HALVIEW_LIST: viewStyle = LV_VIEW_LIST; break;
        case HALVIEW_TILE: viewStyle = LV_VIEW_TILE; break;
        default: viewStyle = LV_VIEW_DETAILS; break;
    }
    
    ListView_SetView(list->hwnd, viewStyle);
}

void hallistview_add_column(HalListView* list, const char* text, int width) {
    if (!list) return;
    
    LVCOLUMNA col = {0};
    col.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
    col.pszText = (LPSTR)text;
    col.cx = width;
    
    int colCount = Header_GetItemCount(ListView_GetHeader(list->hwnd));
    col.iSubItem = colCount;
    
    ListView_InsertColumn(list->hwnd, colCount, &col);
}

int hallistview_add_item(HalListView* list, const char* text, int imageIndex) {
    if (!list) return -1;
    
    LVITEMA item = {0};
    item.mask = LVIF_TEXT | LVIF_IMAGE;
    item.iItem = ListView_GetItemCount(list->hwnd);
    item.pszText = (LPSTR)text;
    item.iImage = imageIndex;
    
    return ListView_InsertItem(list->hwnd, &item);
}

void hallistview_set_subitem(HalListView* list, int item, int subitem, const char* text) {
    if (!list) return;
    
    LVITEMA lvi = {0};
    lvi.mask = LVIF_TEXT;
    lvi.iItem = item;
    lvi.iSubItem = subitem;
    lvi.pszText = (LPSTR)text;
    
    ListView_SetItem(list->hwnd, &lvi);
}

void hallistview_clear(HalListView* list) {
    if (!list) return;
    ListView_DeleteAllItems(list->hwnd);
}

int hallistview_get_selected(HalListView* list) {
    if (!list) return -1;
    return ListView_GetNextItem(list->hwnd, -1, LVNI_SELECTED);
}

/* ============================================
   Tab Control Implementation
   ============================================ */

HalTabControl* haltabcontrol_create(HalForm* parent, int x, int y, int w, int h) {
    if (!parent) return NULL;
    
    HalTabControl* tabs = (HalTabControl*)calloc(1, sizeof(HalTabControl));
    if (!tabs) return NULL;
    
    tabs->base.type = HALCTRL_CUSTOM;
    tabs->base.x = x;
    tabs->base.y = y;
    tabs->base.width = w;
    tabs->base.height = h;
    tabs->base.visible = true;
    tabs->base.enabled = true;
    
    tabs->hwnd = CreateWindowExA(
        0,
        WC_TABCONTROLA,
        "",
        WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | TCS_TABS,
        x, y, w, h,
        parent->hwnd,
        (HMENU)(LONG_PTR)g_halforms.nextControlId++,
        g_halforms.hInstance,
        NULL
    );
    
    if (!tabs->hwnd) {
        free(tabs);
        return NULL;
    }
    
    tabs->base.hwnd = tabs->hwnd;
    SendMessage(tabs->hwnd, WM_SETFONT, (WPARAM)g_halforms.defaultFont, TRUE);
    
    return tabs;
}

int haltabcontrol_add_tab(HalTabControl* tabs, const char* text, int imageIndex) {
    if (!tabs) return -1;
    
    TCITEMA item = {0};
    item.mask = TCIF_TEXT | TCIF_IMAGE;
    item.pszText = (LPSTR)text;
    item.iImage = imageIndex;
    
    int index = TabCtrl_GetItemCount(tabs->hwnd);
    TabCtrl_InsertItem(tabs->hwnd, index, &item);
    
    return index;
}

void haltabcontrol_remove_tab(HalTabControl* tabs, int index) {
    if (!tabs) return;
    TabCtrl_DeleteItem(tabs->hwnd, index);
}

void haltabcontrol_set_selected(HalTabControl* tabs, int index) {
    if (!tabs) return;
    TabCtrl_SetCurSel(tabs->hwnd, index);
}

int haltabcontrol_get_selected(HalTabControl* tabs) {
    if (!tabs) return -1;
    return TabCtrl_GetCurSel(tabs->hwnd);
}

HalControl* haltabcontrol_get_panel(HalTabControl* tabs, int index) {
    /* Tab panels would be managed separately */
    return NULL;
}

/* ============================================
   Splitter Implementation
   ============================================ */

HalSplitter* halsplitter_create(HalForm* parent, bool horizontal, int x, int y, int w, int h) {
    if (!parent) return NULL;
    
    HalSplitter* splitter = (HalSplitter*)calloc(1, sizeof(HalSplitter));
    if (!splitter) return NULL;
    
    splitter->base.type = HALCTRL_CUSTOM;
    splitter->base.x = x;
    splitter->base.y = y;
    splitter->base.width = w;
    splitter->base.height = h;
    splitter->base.visible = true;
    splitter->base.enabled = true;
    splitter->horizontal = horizontal;
    splitter->splitterWidth = 5;
    splitter->minSize1 = 50;
    splitter->minSize2 = 50;
    
    /* Create splitter bar as a static control */
    splitter->base.hwnd = CreateWindowExA(
        0,
        "STATIC",
        "",
        WS_CHILD | WS_VISIBLE | SS_ETCHEDFRAME,
        x, y, w, h,
        parent->hwnd,
        (HMENU)(LONG_PTR)g_halforms.nextControlId++,
        g_halforms.hInstance,
        NULL
    );
    
    return splitter;
}

void halsplitter_set_position(HalSplitter* splitter, int position) {
    if (!splitter) return;
    /* Would update panel sizes based on position */
}

int halsplitter_get_position(HalSplitter* splitter) {
    if (!splitter) return 0;
    return splitter->horizontal ? splitter->base.y : splitter->base.x;
}

/* ============================================
   Property Grid Implementation (Simplified)
   ============================================ */

HalPropertyGrid* halpropertygrid_create(HalForm* parent, int x, int y, int w, int h) {
    if (!parent) return NULL;
    
    HalPropertyGrid* grid = (HalPropertyGrid*)calloc(1, sizeof(HalPropertyGrid));
    if (!grid) return NULL;
    
    grid->base.type = HALCTRL_CUSTOM;
    grid->base.x = x;
    grid->base.y = y;
    grid->base.width = w;
    grid->base.height = h;
    grid->base.visible = true;
    grid->base.enabled = true;
    grid->categorized = true;
    grid->helpVisible = true;
    
    /* Use ListView as base for property grid */
    grid->hwnd = CreateWindowExA(
        WS_EX_CLIENTEDGE,
        WC_LISTVIEWA,
        "",
        WS_CHILD | WS_VISIBLE | WS_BORDER | LVS_REPORT | LVS_SINGLESEL | LVS_NOCOLUMNHEADER,
        x, y, w, h,
        parent->hwnd,
        (HMENU)(LONG_PTR)g_halforms.nextControlId++,
        g_halforms.hInstance,
        NULL
    );
    
    if (!grid->hwnd) {
        free(grid);
        return NULL;
    }
    
    grid->base.hwnd = grid->hwnd;
    SendMessage(grid->hwnd, WM_SETFONT, (WPARAM)g_halforms.defaultFont, TRUE);
    
    ListView_SetExtendedListViewStyle(grid->hwnd, 
        LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
    
    /* Add columns: Name and Value */
    LVCOLUMNA col = {0};
    col.mask = LVCF_TEXT | LVCF_WIDTH;
    col.pszText = "Property";
    col.cx = w / 2;
    ListView_InsertColumn(grid->hwnd, 0, &col);
    
    col.pszText = "Value";
    col.cx = w / 2;
    ListView_InsertColumn(grid->hwnd, 1, &col);
    
    return grid;
}

void halpropertygrid_add_category(HalPropertyGrid* grid, const char* name) {
    if (!grid) return;
    
    LVITEMA item = {0};
    item.mask = LVIF_TEXT;
    item.iItem = ListView_GetItemCount(grid->hwnd);
    item.pszText = (LPSTR)name;
    
    ListView_InsertItem(grid->hwnd, &item);
}

void halpropertygrid_add_string(HalPropertyGrid* grid, const char* name, const char* value) {
    if (!grid) return;
    
    int index = ListView_GetItemCount(grid->hwnd);
    
    LVITEMA item = {0};
    item.mask = LVIF_TEXT;
    item.iItem = index;
    item.pszText = (LPSTR)name;
    ListView_InsertItem(grid->hwnd, &item);
    
    item.iSubItem = 1;
    item.pszText = (LPSTR)value;
    ListView_SetItem(grid->hwnd, &item);
}

void halpropertygrid_add_number(HalPropertyGrid* grid, const char* name, double value) {
    char buf[64];
    snprintf(buf, sizeof(buf), "%.2f", value);
    halpropertygrid_add_string(grid, name, buf);
}

void halpropertygrid_add_bool(HalPropertyGrid* grid, const char* name, bool value) {
    halpropertygrid_add_string(grid, name, value ? "True" : "False");
}

void halpropertygrid_add_color(HalPropertyGrid* grid, const char* name, COLORREF value) {
    char buf[32];
    snprintf(buf, sizeof(buf), "#%02X%02X%02X", 
        GetRValue(value), GetGValue(value), GetBValue(value));
    halpropertygrid_add_string(grid, name, buf);
}

void halpropertygrid_add_enum(HalPropertyGrid* grid, const char* name, 
    const char** options, int count, int selected) {
    if (!grid || !options || selected < 0 || selected >= count) return;
    halpropertygrid_add_string(grid, name, options[selected]);
}

/* ============================================
   Timeline Implementation (Simplified)
   ============================================ */

HalTimeline* haltimeline_create(HalForm* parent, int x, int y, int w, int h) {
    if (!parent) return NULL;
    
    HalTimeline* timeline = (HalTimeline*)calloc(1, sizeof(HalTimeline));
    if (!timeline) return NULL;
    
    timeline->base.type = HALCTRL_CUSTOM;
    timeline->base.x = x;
    timeline->base.y = y;
    timeline->base.width = w;
    timeline->base.height = h;
    timeline->base.visible = true;
    timeline->base.enabled = true;
    timeline->duration = 60.0;
    timeline->zoom = 1.0;
    timeline->snapToGrid = true;
    timeline->gridSize = 1.0;
    
    /* Create custom timeline control using static + owner draw */
    timeline->hwnd = CreateWindowExA(
        WS_EX_CLIENTEDGE,
        "STATIC",
        "",
        WS_CHILD | WS_VISIBLE | SS_OWNERDRAW,
        x, y, w, h,
        parent->hwnd,
        (HMENU)(LONG_PTR)g_halforms.nextControlId++,
        g_halforms.hInstance,
        NULL
    );
    
    if (!timeline->hwnd) {
        free(timeline);
        return NULL;
    }
    
    timeline->base.hwnd = timeline->hwnd;
    
    return timeline;
}

void haltimeline_set_duration(HalTimeline* timeline, double seconds) {
    if (timeline) timeline->duration = seconds;
}

void haltimeline_set_time(HalTimeline* timeline, double seconds) {
    if (timeline) timeline->currentTime = seconds;
}

double haltimeline_get_time(HalTimeline* timeline) {
    return timeline ? timeline->currentTime : 0.0;
}

void haltimeline_add_track(HalTimeline* timeline, const char* name) {
    if (timeline) timeline->trackCount++;
}

void haltimeline_add_keyframe(HalTimeline* timeline, int track, double time, void* data) {
    /* Keyframe storage would be implemented here */
}
