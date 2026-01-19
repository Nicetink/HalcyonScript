/*
 * HalForms - Menu and Toolbar Implementation
 * Modern Windows-style menus with proper submenu support
 */

#include "halforms.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uxtheme.h>

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

static int g_nextMenuId = 10000;

/* Menu item registry for event handling */
typedef struct {
    int id;
    char* handlerName;
    HalEventHandler onClick;
    void* userData;
} MenuItemRegistry;

static MenuItemRegistry* g_menuRegistry = NULL;
static int g_menuRegistryCount = 0;
static int g_menuRegistryCapacity = 0;

/* Register menu item for event handling */
static void register_menu_item(int id, const char* handlerName, HalEventHandler onClick, void* userData) {
    if (g_menuRegistryCount >= g_menuRegistryCapacity) {
        g_menuRegistryCapacity = g_menuRegistryCapacity == 0 ? 64 : g_menuRegistryCapacity * 2;
        g_menuRegistry = (MenuItemRegistry*)realloc(g_menuRegistry, 
            g_menuRegistryCapacity * sizeof(MenuItemRegistry));
    }
    
    g_menuRegistry[g_menuRegistryCount].id = id;
    g_menuRegistry[g_menuRegistryCount].handlerName = handlerName ? _strdup(handlerName) : NULL;
    g_menuRegistry[g_menuRegistryCount].onClick = onClick;
    g_menuRegistry[g_menuRegistryCount].userData = userData;
    g_menuRegistryCount++;
}

/* External function to register menu item from runtime */
void register_menu_item_ext(int id, const char* handlerName) {
    register_menu_item(id, handlerName, NULL, NULL);
}

/* Find menu item by ID */
MenuItemRegistry* find_menu_item(int id) {
    for (int i = 0; i < g_menuRegistryCount; i++) {
        if (g_menuRegistry[i].id == id) {
            return &g_menuRegistry[i];
        }
    }
    return NULL;
}

/* Get handler name for menu item */
const char* halforms_get_menu_handler(int menuId) {
    MenuItemRegistry* item = find_menu_item(menuId);
    return item ? item->handlerName : NULL;
}

/* ============================================
   Menu Functions
   ============================================ */

HalMenu* halmenu_create(void) {
    HalMenu* menu = (HalMenu*)calloc(1, sizeof(HalMenu));
    if (!menu) return NULL;
    
    menu->hmenu = CreateMenu();
    menu->items = NULL;
    menu->itemCount = 0;
    
    return menu;
}

void halmenu_destroy(HalMenu* menu) {
    if (!menu) return;
    
    for (int i = 0; i < menu->itemCount; i++) {
        if (menu->items[i]) {
            free(menu->items[i]->text);
            free(menu->items[i]->shortcut);
            for (int j = 0; j < menu->items[i]->subItemCount; j++) {
                free(menu->items[i]->subItems[j]->text);
                free(menu->items[i]->subItems[j]->shortcut);
                free(menu->items[i]->subItems[j]);
            }
            free(menu->items[i]->subItems);
            free(menu->items[i]);
        }
    }
    free(menu->items);
    
    if (menu->hmenu) {
        DestroyMenu(menu->hmenu);
    }
    
    free(menu);
}

HalMenuItem* halmenu_add_item(HalMenu* menu, const char* text, HalEventHandler onClick) {
    if (!menu) return NULL;
    
    HalMenuItem* item = (HalMenuItem*)calloc(1, sizeof(HalMenuItem));
    if (!item) return NULL;
    
    item->id = g_nextMenuId++;
    item->text = _strdup(text);
    item->enabled = true;
    item->onClick = onClick;
    
    /* Check if separator */
    if (strcmp(text, "-") == 0) {
        AppendMenuA(menu->hmenu, MF_SEPARATOR, 0, NULL);
    } else {
        AppendMenuA(menu->hmenu, MF_STRING, item->id, text);
        /* Don't register here - let runtime handle it with handler name */
    }
    
    menu->items = (HalMenuItem**)realloc(menu->items, 
        (menu->itemCount + 1) * sizeof(HalMenuItem*));
    menu->items[menu->itemCount++] = item;
    
    return item;
}

HalMenuItem* halmenu_add_submenu(HalMenu* menu, const char* text) {
    if (!menu) return NULL;
    
    HalMenuItem* item = (HalMenuItem*)calloc(1, sizeof(HalMenuItem));
    if (!item) return NULL;
    
    item->hmenu = CreatePopupMenu();
    item->text = _strdup(text);
    item->enabled = true;
    item->id = g_nextMenuId++;
    
    AppendMenuA(menu->hmenu, MF_POPUP, (UINT_PTR)item->hmenu, text);
    
    menu->items = (HalMenuItem**)realloc(menu->items, 
        (menu->itemCount + 1) * sizeof(HalMenuItem*));
    menu->items[menu->itemCount++] = item;
    
    return item;
}

void halmenu_add_separator(HalMenu* menu) {
    if (!menu) return;
    AppendMenuA(menu->hmenu, MF_SEPARATOR, 0, NULL);
}

/* Add item to submenu */
HalMenuItem* halmenuitem_add_item_ex(HalMenuItem* parent, const char* text, const char* handlerName) {
    if (!parent || !parent->hmenu) return NULL;
    
    HalMenuItem* item = (HalMenuItem*)calloc(1, sizeof(HalMenuItem));
    if (!item) return NULL;
    
    item->id = g_nextMenuId++;
    item->text = _strdup(text);
    item->enabled = true;
    
    /* Check if separator */
    if (strcmp(text, "-") == 0) {
        AppendMenuA(parent->hmenu, MF_SEPARATOR, 0, NULL);
    } else {
        AppendMenuA(parent->hmenu, MF_STRING, item->id, text);
        register_menu_item(item->id, handlerName, NULL, NULL);
    }
    
    parent->subItems = (HalMenuItem**)realloc(parent->subItems, 
        (parent->subItemCount + 1) * sizeof(HalMenuItem*));
    parent->subItems[parent->subItemCount++] = item;
    
    return item;
}

void halmenuitem_add_item(HalMenuItem* parent, const char* text, HalEventHandler onClick) {
    if (!parent || !parent->hmenu) return;
    
    HalMenuItem* item = (HalMenuItem*)calloc(1, sizeof(HalMenuItem));
    if (!item) return;
    
    item->id = g_nextMenuId++;
    item->text = _strdup(text);
    item->enabled = true;
    item->onClick = onClick;
    
    if (strcmp(text, "-") == 0) {
        AppendMenuA(parent->hmenu, MF_SEPARATOR, 0, NULL);
    } else {
        AppendMenuA(parent->hmenu, MF_STRING, item->id, text);
        register_menu_item(item->id, NULL, onClick, NULL);
    }
    
    parent->subItems = (HalMenuItem**)realloc(parent->subItems, 
        (parent->subItemCount + 1) * sizeof(HalMenuItem*));
    parent->subItems[parent->subItemCount++] = item;
}

/* Create nested submenu */
HalMenuItem* halmenuitem_add_submenu(HalMenuItem* parent, const char* text) {
    if (!parent || !parent->hmenu) return NULL;
    
    HalMenuItem* item = (HalMenuItem*)calloc(1, sizeof(HalMenuItem));
    if (!item) return NULL;
    
    item->hmenu = CreatePopupMenu();
    item->text = _strdup(text);
    item->enabled = true;
    item->id = g_nextMenuId++;
    
    AppendMenuA(parent->hmenu, MF_POPUP, (UINT_PTR)item->hmenu, text);
    
    parent->subItems = (HalMenuItem**)realloc(parent->subItems, 
        (parent->subItemCount + 1) * sizeof(HalMenuItem*));
    parent->subItems[parent->subItemCount++] = item;
    
    return item;
}

void halmenuitem_set_shortcut(HalMenuItem* item, const char* shortcut) {
    if (!item) return;
    free(item->shortcut);
    item->shortcut = shortcut ? _strdup(shortcut) : NULL;
}

void halmenuitem_set_checked(HalMenuItem* item, bool checked) {
    if (!item) return;
    item->checked = checked;
}

void halmenuitem_set_enabled(HalMenuItem* item, bool enabled) {
    if (!item) return;
    item->enabled = enabled;
}

/* ============================================
   Context Menu Functions
   ============================================ */

HalMenu* halmenu_create_popup(void) {
    HalMenu* menu = (HalMenu*)calloc(1, sizeof(HalMenu));
    if (!menu) return NULL;
    
    menu->hmenu = CreatePopupMenu();
    return menu;
}

void halmenu_show_popup(HalMenu* menu, HalForm* form, int x, int y) {
    if (!menu || !form) return;
    
    POINT pt = {x, y};
    ClientToScreen(form->hwnd, &pt);
    
    TrackPopupMenu(menu->hmenu, TPM_LEFTALIGN | TPM_TOPALIGN,
        pt.x, pt.y, 0, form->hwnd, NULL);
}

/* ============================================
   Toolbar Functions
   ============================================ */

HalToolbar* haltoolbar_create(HalForm* parent) {
    if (!parent) return NULL;
    
    HalToolbar* toolbar = (HalToolbar*)calloc(1, sizeof(HalToolbar));
    if (!toolbar) return NULL;
    
    toolbar->hwnd = CreateWindowExA(
        0, TOOLBARCLASSNAMEA, NULL,
        WS_CHILD | WS_VISIBLE | TBSTYLE_FLAT | TBSTYLE_TOOLTIPS | CCS_TOP,
        0, 0, 0, 0,
        parent->hwnd,
        NULL,
        g_halforms.hInstance,
        NULL
    );
    
    if (!toolbar->hwnd) {
        free(toolbar);
        return NULL;
    }
    
    SendMessage(toolbar->hwnd, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
    SendMessage(toolbar->hwnd, TB_SETBITMAPSIZE, 0, MAKELPARAM(16, 16));
    
    return toolbar;
}

void haltoolbar_add_button(HalToolbar* toolbar, int imageIndex, const char* tooltip, HalEventHandler onClick) {
    if (!toolbar) return;
    
    TBBUTTON button = {0};
    button.iBitmap = imageIndex;
    button.idCommand = g_nextMenuId++;
    button.fsState = TBSTATE_ENABLED;
    button.fsStyle = BTNS_BUTTON;
    button.iString = (INT_PTR)tooltip;
    
    SendMessage(toolbar->hwnd, TB_ADDBUTTONS, 1, (LPARAM)&button);
    toolbar->buttonCount++;
}

void haltoolbar_add_separator(HalToolbar* toolbar) {
    if (!toolbar) return;
    
    TBBUTTON button = {0};
    button.fsStyle = BTNS_SEP;
    
    SendMessage(toolbar->hwnd, TB_ADDBUTTONS, 1, (LPARAM)&button);
}

void haltoolbar_set_imagelist(HalToolbar* toolbar, const char* imagePath, int imageWidth) {
    if (!toolbar) return;
    
    HBITMAP hBitmap = (HBITMAP)LoadImageA(NULL, imagePath, IMAGE_BITMAP, 0, 0, 
        LR_LOADFROMFILE | LR_CREATEDIBSECTION);
    
    if (!hBitmap) return;
    
    BITMAP bm;
    GetObject(hBitmap, sizeof(bm), &bm);
    int imageCount = bm.bmWidth / imageWidth;
    
    toolbar->imageList = ImageList_Create(imageWidth, bm.bmHeight, 
        ILC_COLOR32 | ILC_MASK, imageCount, 0);
    
    if (toolbar->imageList) {
        ImageList_Add(toolbar->imageList, hBitmap, NULL);
        SendMessage(toolbar->hwnd, TB_SETIMAGELIST, 0, (LPARAM)toolbar->imageList);
    }
    
    DeleteObject(hBitmap);
}

/* ============================================
   StatusBar Functions
   ============================================ */

HalStatusBar* halstatusbar_create(HalForm* parent, int partCount) {
    if (!parent || partCount <= 0) return NULL;
    
    HalStatusBar* statusBar = (HalStatusBar*)calloc(1, sizeof(HalStatusBar));
    if (!statusBar) return NULL;
    
    statusBar->hwnd = CreateWindowExA(
        0, STATUSCLASSNAMEA, NULL,
        WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
        0, 0, 0, 0,
        parent->hwnd,
        NULL,
        g_halforms.hInstance,
        NULL
    );
    
    if (!statusBar->hwnd) {
        free(statusBar);
        return NULL;
    }
    
    statusBar->partCount = partCount;
    statusBar->partWidths = (int*)calloc(partCount, sizeof(int));
    
    RECT rect;
    GetClientRect(parent->hwnd, &rect);
    int partWidth = rect.right / partCount;
    
    int* widths = (int*)malloc(partCount * sizeof(int));
    for (int i = 0; i < partCount; i++) {
        widths[i] = (i + 1) * partWidth;
        statusBar->partWidths[i] = partWidth;
    }
    widths[partCount - 1] = -1;
    
    SendMessage(statusBar->hwnd, SB_SETPARTS, partCount, (LPARAM)widths);
    free(widths);
    
    return statusBar;
}

void halstatusbar_set_text(HalStatusBar* statusBar, int part, const char* text) {
    if (!statusBar || part < 0 || part >= statusBar->partCount) return;
    SendMessageA(statusBar->hwnd, SB_SETTEXTA, part, (LPARAM)text);
}

void halstatusbar_set_part_width(HalStatusBar* statusBar, int part, int width) {
    if (!statusBar || part < 0 || part >= statusBar->partCount) return;
    
    statusBar->partWidths[part] = width;
    
    int* widths = (int*)malloc(statusBar->partCount * sizeof(int));
    int pos = 0;
    for (int i = 0; i < statusBar->partCount; i++) {
        pos += statusBar->partWidths[i];
        widths[i] = pos;
    }
    widths[statusBar->partCount - 1] = -1;
    
    SendMessage(statusBar->hwnd, SB_SETPARTS, statusBar->partCount, (LPARAM)widths);
    free(widths);
}

void halform_set_toolbar(HalForm* form, HalToolbar* toolbar) {
    if (!form) return;
    form->toolbar = toolbar;
}

void halform_set_statusbar(HalForm* form, HalStatusBar* statusBar) {
    if (!form) return;
    form->statusBar = statusBar;
}
