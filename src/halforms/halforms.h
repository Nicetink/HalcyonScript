/*
 * HalForms - Windows Forms-like UI Framework for HalcyonScript
 * 
 * Native Win32 controls for Windows XP/7/10/11 compatibility
 * Professional desktop applications with menus, toolbars, and dialogs
 */

#ifndef HALFORMS_H
#define HALFORMS_H

#include <windows.h>
#include <commctrl.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================
   Forward Declarations
   ============================================ */

typedef struct HalForm HalForm;
typedef struct HalControl HalControl;
typedef struct HalMenu HalMenu;
typedef struct HalMenuItem HalMenuItem;
typedef struct HalToolbar HalToolbar;
typedef struct HalStatusBar HalStatusBar;
typedef struct HalDockPanel HalDockPanel;
typedef struct HalSplitter HalSplitter;
typedef struct HalTabControl HalTabControl;
typedef struct HalTreeView HalTreeView;
typedef struct HalListView HalListView;
typedef struct HalPropertyGrid HalPropertyGrid;
typedef struct HalCodeEditor HalCodeEditor;
typedef struct HalTimeline HalTimeline;

/* ============================================
   Enumerations
   ============================================ */

typedef enum {
    HALFORM_NORMAL,
    HALFORM_DIALOG,
    HALFORM_TOOL,
    HALFORM_MDI_PARENT,
    HALFORM_MDI_CHILD
} HalFormStyle;

typedef enum {
    HALCTRL_LABEL,
    HALCTRL_BUTTON,
    HALCTRL_TEXTBOX,
    HALCTRL_CHECKBOX,
    HALCTRL_RADIOBUTTON,
    HALCTRL_COMBOBOX,
    HALCTRL_LISTBOX,
    HALCTRL_GROUPBOX,
    HALCTRL_PANEL,
    HALCTRL_PICTUREBOX,
    HALCTRL_PROGRESSBAR,
    HALCTRL_TRACKBAR,
    HALCTRL_NUMERICUPDOWN,
    HALCTRL_DATETIMEPICKER,
    HALCTRL_MONTHCALENDAR,
    HALCTRL_RICHTEXTBOX,
    HALCTRL_WEBBROWSER,
    HALCTRL_CUSTOM
} HalControlType;

typedef enum {
    HALDOCK_NONE,
    HALDOCK_TOP,
    HALDOCK_BOTTOM,
    HALDOCK_LEFT,
    HALDOCK_RIGHT,
    HALDOCK_FILL
} HalDockStyle;

typedef enum {
    HALANCHOR_NONE = 0,
    HALANCHOR_TOP = 1,
    HALANCHOR_BOTTOM = 2,
    HALANCHOR_LEFT = 4,
    HALANCHOR_RIGHT = 8
} HalAnchorStyle;

typedef enum {
    HALVIEW_ICON,
    HALVIEW_SMALLICON,
    HALVIEW_LIST,
    HALVIEW_DETAILS,
    HALVIEW_TILE
} HalListViewStyle;

/* ============================================
   Event Types
   ============================================ */

typedef enum {
    HALEVENT_CLICK,
    HALEVENT_DOUBLECLICK,
    HALEVENT_MOUSEDOWN,
    HALEVENT_MOUSEUP,
    HALEVENT_MOUSEMOVE,
    HALEVENT_MOUSEENTER,
    HALEVENT_MOUSELEAVE,
    HALEVENT_KEYDOWN,
    HALEVENT_KEYUP,
    HALEVENT_KEYPRESS,
    HALEVENT_TEXTCHANGED,
    HALEVENT_SELECTIONCHANGED,
    HALEVENT_VALUECHANGED,
    HALEVENT_RESIZE,
    HALEVENT_PAINT,
    HALEVENT_LOAD,
    HALEVENT_CLOSING,
    HALEVENT_CLOSED,
    HALEVENT_DRAGENTER,
    HALEVENT_DRAGOVER,
    HALEVENT_DRAGLEAVE,
    HALEVENT_DROP
} HalEventType;

typedef struct {
    HalEventType type;
    HalControl* sender;
    int mouseX, mouseY;
    int button;
    int keyCode;
    wchar_t character;
    bool handled;
    bool cancel;
    void* data;
} HalFormEvent;

typedef void (*HalEventHandler)(HalControl* sender, HalFormEvent* event, void* userData);

/* ============================================
   Structures
   ============================================ */

struct HalControl {
    HalControlType type;
    HWND hwnd;
    HalForm* parent;
    char* name;
    char* text;
    int x, y, width, height;
    bool visible;
    bool enabled;
    HalDockStyle dock;
    HalAnchorStyle anchor;
    int tag;
    void* userData;
    
    /* Event handlers */
    HalEventHandler onClick;
    HalEventHandler onDoubleClick;
    HalEventHandler onTextChanged;
    HalEventHandler onKeyDown;
    HalEventHandler onMouseDown;
    void* eventUserData;
    
    /* Child controls (for containers) */
    HalControl** children;
    int childCount;
    int childCapacity;
};

struct HalForm {
    HalControl base;
    HalFormStyle style;
    HWND hwnd;
    HalMenu* menu;
    HalToolbar* toolbar;
    HalStatusBar* statusBar;
    bool maximizeBox;
    bool minimizeBox;
    bool showInTaskbar;
    bool topMost;
    int minWidth, minHeight;
    int maxWidth, maxHeight;
    
    /* MDI support */
    HalForm* mdiParent;
    HalForm** mdiChildren;
    int mdiChildCount;
    
    /* Events */
    HalEventHandler onLoad;
    HalEventHandler onClosing;
    HalEventHandler onClosed;
    HalEventHandler onResize;
};

struct HalMenu {
    HMENU hmenu;
    HalMenuItem** items;
    int itemCount;
};

struct HalMenuItem {
    HMENU hmenu;
    int id;
    char* text;
    char* shortcut;
    bool checked;
    bool enabled;
    HalMenuItem** subItems;
    int subItemCount;
    HalEventHandler onClick;
    void* userData;
};

struct HalToolbar {
    HWND hwnd;
    HIMAGELIST imageList;
    int buttonCount;
};

struct HalStatusBar {
    HWND hwnd;
    int partCount;
    int* partWidths;
};

/* ============================================
   Advanced Controls
   ============================================ */

/* Code Editor with syntax highlighting */
struct HalCodeEditor {
    HalControl base;
    HWND hwnd;
    char* language;
    bool lineNumbers;
    bool wordWrap;
    int tabSize;
    bool autoIndent;
    bool bracketMatching;
    COLORREF bgColor;
    COLORREF textColor;
    COLORREF lineNumColor;
    COLORREF keywordColor;
    COLORREF stringColor;
    COLORREF commentColor;
    COLORREF numberColor;
};

/* Tree View for hierarchical data */
struct HalTreeView {
    HalControl base;
    HWND hwnd;
    HIMAGELIST imageList;
    bool checkBoxes;
    bool dragDrop;
    bool editLabels;
    HalEventHandler onNodeSelect;
    HalEventHandler onNodeExpand;
    HalEventHandler onNodeCollapse;
};

/* List View for data display */
struct HalListView {
    HalControl base;
    HWND hwnd;
    HalListViewStyle viewStyle;
    HIMAGELIST largeImageList;
    HIMAGELIST smallImageList;
    bool multiSelect;
    bool gridLines;
    bool fullRowSelect;
    HalEventHandler onItemSelect;
    HalEventHandler onItemActivate;
};

/* Property Grid for settings */
struct HalPropertyGrid {
    HalControl base;
    HWND hwnd;
    bool categorized;
    bool helpVisible;
    HalEventHandler onPropertyChanged;
};

/* Dock Panel for flexible layouts */
struct HalDockPanel {
    HalControl base;
    HalControl* topPanel;
    HalControl* bottomPanel;
    HalControl* leftPanel;
    HalControl* rightPanel;
    HalControl* centerPanel;
    bool allowFloat;
    bool allowClose;
};

/* Splitter for resizable panels */
struct HalSplitter {
    HalControl base;
    bool horizontal;
    int splitterWidth;
    int minSize1, minSize2;
    HalControl* panel1;
    HalControl* panel2;
};

/* Tab Control for multi-page interfaces */
struct HalTabControl {
    HalControl base;
    HWND hwnd;
    bool closableTabs;
    bool draggableTabs;
    HIMAGELIST imageList;
    HalEventHandler onTabChanged;
    HalEventHandler onTabClosing;
};

/* Timeline for time-based data */
struct HalTimeline {
    HalControl base;
    HWND hwnd;
    double duration;
    double currentTime;
    double zoom;
    int trackCount;
    bool snapToGrid;
    double gridSize;
    HalEventHandler onTimeChanged;
    HalEventHandler onSelectionChanged;
};

/* ============================================
   Initialization
   ============================================ */

bool halforms_init(void);
void halforms_shutdown(void);
void halforms_run(void);
void halforms_exit(int code);

/* ============================================
   Form Functions
   ============================================ */

HalForm* halform_create(const char* title, int width, int height);
HalForm* halform_create_ex(const char* title, int x, int y, int width, int height, HalFormStyle style);
void halform_destroy(HalForm* form);
void halform_show(HalForm* form);
void halform_hide(HalForm* form);
void halform_close(HalForm* form);
void halform_center(HalForm* form);
void halform_maximize(HalForm* form);
void halform_minimize(HalForm* form);
void halform_restore(HalForm* form);
void halform_set_title(HalForm* form, const char* title);
void halform_set_icon(HalForm* form, const char* iconPath);
void halform_set_menu(HalForm* form, HalMenu* menu);
void halform_set_toolbar(HalForm* form, HalToolbar* toolbar);
void halform_set_statusbar(HalForm* form, HalStatusBar* statusBar);

/* ============================================
   Basic Controls
   ============================================ */

HalControl* halctrl_label(HalForm* parent, const char* text, int x, int y, int w, int h);
HalControl* halctrl_button(HalForm* parent, const char* text, int x, int y, int w, int h);
HalControl* halctrl_textbox(HalForm* parent, const char* text, int x, int y, int w, int h);
HalControl* halctrl_textbox_multiline(HalForm* parent, int x, int y, int w, int h);
HalControl* halctrl_checkbox(HalForm* parent, const char* text, int x, int y, int w, int h);
HalControl* halctrl_radiobutton(HalForm* parent, const char* text, int x, int y, int w, int h);
HalControl* halctrl_combobox(HalForm* parent, int x, int y, int w, int h);
HalControl* halctrl_listbox(HalForm* parent, int x, int y, int w, int h);
HalControl* halctrl_groupbox(HalForm* parent, const char* text, int x, int y, int w, int h);
HalControl* halctrl_panel(HalForm* parent, int x, int y, int w, int h);
HalControl* halctrl_progressbar(HalForm* parent, int x, int y, int w, int h);
HalControl* halctrl_trackbar(HalForm* parent, int x, int y, int w, int h, int min, int max);
HalControl* halctrl_numericupdown(HalForm* parent, int x, int y, int w, int h, int min, int max);

/* ============================================
   Control Properties
   ============================================ */

void halctrl_set_text(HalControl* ctrl, const char* text);
char* halctrl_get_text(HalControl* ctrl);
void halctrl_set_visible(HalControl* ctrl, bool visible);
void halctrl_set_enabled(HalControl* ctrl, bool enabled);
void halctrl_set_bounds(HalControl* ctrl, int x, int y, int w, int h);
void halctrl_set_dock(HalControl* ctrl, HalDockStyle dock);
void halctrl_set_anchor(HalControl* ctrl, HalAnchorStyle anchor);
void halctrl_set_font(HalControl* ctrl, const char* fontName, int size, bool bold, bool italic);
void halctrl_set_colors(HalControl* ctrl, COLORREF foreground, COLORREF background);

/* Checkbox/RadioButton */
void halctrl_set_checked(HalControl* ctrl, bool checked);
bool halctrl_get_checked(HalControl* ctrl);

/* ComboBox/ListBox */
void halctrl_add_item(HalControl* ctrl, const char* item);
void halctrl_remove_item(HalControl* ctrl, int index);
void halctrl_clear_items(HalControl* ctrl);
int halctrl_get_selected_index(HalControl* ctrl);
void halctrl_set_selected_index(HalControl* ctrl, int index);
char* halctrl_get_selected_item(HalControl* ctrl);

/* ProgressBar/TrackBar */
void halctrl_set_value(HalControl* ctrl, int value);
int halctrl_get_value(HalControl* ctrl);
void halctrl_set_range(HalControl* ctrl, int min, int max);

/* ============================================
   Menu Functions
   ============================================ */

HalMenu* halmenu_create(void);
void halmenu_destroy(HalMenu* menu);
HalMenuItem* halmenu_add_item(HalMenu* menu, const char* text, HalEventHandler onClick);
HalMenuItem* halmenu_add_submenu(HalMenu* menu, const char* text);
void halmenu_add_separator(HalMenu* menu);
void halmenuitem_add_item(HalMenuItem* parent, const char* text, HalEventHandler onClick);
void halmenuitem_set_shortcut(HalMenuItem* item, const char* shortcut);
void halmenuitem_set_checked(HalMenuItem* item, bool checked);
void halmenuitem_set_enabled(HalMenuItem* item, bool enabled);

/* ============================================
   Toolbar Functions
   ============================================ */

HalToolbar* haltoolbar_create(HalForm* parent);
void haltoolbar_add_button(HalToolbar* toolbar, int imageIndex, const char* tooltip, HalEventHandler onClick);
void haltoolbar_add_separator(HalToolbar* toolbar);
void haltoolbar_set_imagelist(HalToolbar* toolbar, const char* imagePath, int imageWidth);

/* ============================================
   StatusBar Functions
   ============================================ */

HalStatusBar* halstatusbar_create(HalForm* parent, int partCount);
void halstatusbar_set_text(HalStatusBar* statusBar, int part, const char* text);
void halstatusbar_set_part_width(HalStatusBar* statusBar, int part, int width);

/* ============================================
   Specialized Controls
   ============================================ */

/* Code Editor */
HalCodeEditor* halcodeeditor_create(HalForm* parent, int x, int y, int w, int h);
void halcodeeditor_set_text(HalCodeEditor* editor, const char* text);
char* halcodeeditor_get_text(HalCodeEditor* editor);
void halcodeeditor_set_language(HalCodeEditor* editor, const char* language);
void halcodeeditor_goto_line(HalCodeEditor* editor, int line);
void halcodeeditor_set_selection(HalCodeEditor* editor, int start, int end);
void halcodeeditor_insert_text(HalCodeEditor* editor, const char* text);

/* Tree View */
HalTreeView* haltreeview_create(HalForm* parent, int x, int y, int w, int h);
HTREEITEM haltreeview_add_node(HalTreeView* tree, HTREEITEM parent, const char* text, int imageIndex);
void haltreeview_remove_node(HalTreeView* tree, HTREEITEM node);
void haltreeview_clear(HalTreeView* tree);
void haltreeview_expand_node(HalTreeView* tree, HTREEITEM node);
void haltreeview_collapse_node(HalTreeView* tree, HTREEITEM node);
HTREEITEM haltreeview_get_selected(HalTreeView* tree);

/* List View */
HalListView* hallistview_create(HalForm* parent, int x, int y, int w, int h);
void hallistview_set_view(HalListView* list, HalListViewStyle style);
void hallistview_add_column(HalListView* list, const char* text, int width);
int hallistview_add_item(HalListView* list, const char* text, int imageIndex);
void hallistview_set_subitem(HalListView* list, int item, int subitem, const char* text);
void hallistview_clear(HalListView* list);
int hallistview_get_selected(HalListView* list);

/* Property Grid */
HalPropertyGrid* halpropertygrid_create(HalForm* parent, int x, int y, int w, int h);
void halpropertygrid_add_category(HalPropertyGrid* grid, const char* name);
void halpropertygrid_add_string(HalPropertyGrid* grid, const char* name, const char* value);
void halpropertygrid_add_number(HalPropertyGrid* grid, const char* name, double value);
void halpropertygrid_add_bool(HalPropertyGrid* grid, const char* name, bool value);
void halpropertygrid_add_color(HalPropertyGrid* grid, const char* name, COLORREF value);
void halpropertygrid_add_enum(HalPropertyGrid* grid, const char* name, const char** options, int count, int selected);

/* Tab Control */
HalTabControl* haltabcontrol_create(HalForm* parent, int x, int y, int w, int h);
int haltabcontrol_add_tab(HalTabControl* tabs, const char* text, int imageIndex);
void haltabcontrol_remove_tab(HalTabControl* tabs, int index);
void haltabcontrol_set_selected(HalTabControl* tabs, int index);
int haltabcontrol_get_selected(HalTabControl* tabs);
HalControl* haltabcontrol_get_panel(HalTabControl* tabs, int index);

/* Splitter */
HalSplitter* halsplitter_create(HalForm* parent, bool horizontal, int x, int y, int w, int h);
void halsplitter_set_position(HalSplitter* splitter, int position);
int halsplitter_get_position(HalSplitter* splitter);

/* Timeline (for DAW/Animation) */
HalTimeline* haltimeline_create(HalForm* parent, int x, int y, int w, int h);
void haltimeline_set_duration(HalTimeline* timeline, double seconds);
void haltimeline_set_time(HalTimeline* timeline, double seconds);
double haltimeline_get_time(HalTimeline* timeline);
void haltimeline_add_track(HalTimeline* timeline, const char* name);
void haltimeline_add_keyframe(HalTimeline* timeline, int track, double time, void* data);

/* ============================================
   Extended Code Editor Functions
   ============================================ */

void halcodeeditor_undo(HalCodeEditor* editor);
void halcodeeditor_redo(HalCodeEditor* editor);
bool halcodeeditor_can_undo(HalCodeEditor* editor);
bool halcodeeditor_can_redo(HalCodeEditor* editor);
void halcodeeditor_cut(HalCodeEditor* editor);
void halcodeeditor_copy(HalCodeEditor* editor);
void halcodeeditor_paste(HalCodeEditor* editor);
void halcodeeditor_select_all(HalCodeEditor* editor);
int halcodeeditor_get_line_count(HalCodeEditor* editor);
int halcodeeditor_get_current_line(HalCodeEditor* editor);
int halcodeeditor_get_current_column(HalCodeEditor* editor);
void halcodeeditor_set_readonly(HalCodeEditor* editor, bool readonly);
void halcodeeditor_set_modified(HalCodeEditor* editor, bool modified);
bool halcodeeditor_is_modified(HalCodeEditor* editor);
int halcodeeditor_find(HalCodeEditor* editor, const char* text, bool matchCase, bool wholeWord, bool forward);
int halcodeeditor_replace(HalCodeEditor* editor, const char* findText, const char* replaceText, bool matchCase);
int halcodeeditor_replace_all(HalCodeEditor* editor, const char* findText, const char* replaceText, bool matchCase);
void halcodeeditor_toggle_bookmark(HalCodeEditor* editor, int line);
void halcodeeditor_goto_next_bookmark(HalCodeEditor* editor);
void halcodeeditor_fold_all(HalCodeEditor* editor);
void halcodeeditor_unfold_all(HalCodeEditor* editor);
void halcodeeditor_set_dark_mode(HalCodeEditor* editor, bool dark);

/* ============================================
   SplitContainer / Layout Controls
   ============================================ */

HWND halsplitter_get_panel1(HalSplitter* splitter);
HWND halsplitter_get_panel2(HalSplitter* splitter);

/* Dock Panel */
HalDockPanel* haldockpanel_create(HalForm* parent, int x, int y, int w, int h);
void haldockpanel_set_content(HalDockPanel* dock, HalDockStyle position, HalControl* content, int size);
void haldockpanel_update_layout(HalDockPanel* dock);

/* ============================================
   Process Execution
   ============================================ */

typedef struct HalProcess HalProcess;

HalProcess* halprocess_create(const char* command, const char* workingDir, bool redirectOutput);
bool halprocess_start(HalProcess* proc);
char* halprocess_read_output(HalProcess* proc, int maxBytes);
char* halprocess_read_error(HalProcess* proc, int maxBytes);
bool halprocess_write_input(HalProcess* proc, const char* data);
bool halprocess_is_running(HalProcess* proc);
int halprocess_get_exit_code(HalProcess* proc);
bool halprocess_wait(HalProcess* proc, int timeoutMs);
bool halprocess_kill(HalProcess* proc);
void halprocess_destroy(HalProcess* proc);
char* halprocess_exec(const char* command, const char* workingDir, int* exitCode);

/* ============================================
   Clipboard Functions
   ============================================ */

char* halforms_clipboard_get_text(void);
bool halforms_clipboard_set_text(const char* text);

/* ============================================
   System Functions
   ============================================ */

char* halforms_getenv(const char* name);
bool halforms_setenv(const char* name, const char* value);
char* halforms_getcwd(void);
bool halforms_chdir(const char* path);
char* halforms_get_temp_dir(void);
char* halforms_get_home_dir(void);
char* halforms_get_appdata_dir(void);
bool halforms_shell_open(const char* path);
bool halforms_shell_show_in_folder(const char* path);

/* ============================================
   Timer Functions
   ============================================ */

typedef struct HalTimer HalTimer;

HalTimer* haltimer_create(int intervalMs, void (*callback)(void*), void* userData);
void haltimer_start(HalTimer* timer);
void haltimer_stop(HalTimer* timer);
void haltimer_destroy(HalTimer* timer);

/* ============================================
   Dialogs
   ============================================ */

int halforms_msgbox(const char* text, const char* title, int buttons, int icon);
char* halforms_open_file(const char* title, const char* filter);
char* halforms_save_file(const char* title, const char* filter, const char* defaultName);
char* halforms_browse_folder(const char* title);
COLORREF halforms_color_dialog(COLORREF initial);
char* halforms_font_dialog(const char* initialFont, int initialSize);
char* halforms_input_dialog(const char* title, const char* prompt, const char* defaultValue);

/* ============================================
   Event Handling
   ============================================ */

void halctrl_on_click(HalControl* ctrl, HalEventHandler handler, void* userData);
void halctrl_on_doubleclick(HalControl* ctrl, HalEventHandler handler, void* userData);
void halctrl_on_textchanged(HalControl* ctrl, HalEventHandler handler, void* userData);
void halctrl_on_keydown(HalControl* ctrl, HalEventHandler handler, void* userData);
void halctrl_on_mousedown(HalControl* ctrl, HalEventHandler handler, void* userData);

void halform_on_load(HalForm* form, HalEventHandler handler, void* userData);
void halform_on_closing(HalForm* form, HalEventHandler handler, void* userData);
void halform_on_resize(HalForm* form, HalEventHandler handler, void* userData);

/* ============================================
   Utility Functions
   ============================================ */

void halforms_set_dpi_aware(bool aware);
void halforms_enable_visual_styles(void);
HFONT halforms_create_font(const char* name, int size, bool bold, bool italic);
HICON halforms_load_icon(const char* path);
HBITMAP halforms_load_bitmap(const char* path);

/* ============================================
   Paint Canvas - Advanced Drawing API
   ============================================ */

typedef struct PaintCanvas PaintCanvas;

typedef enum {
    PAINT_TOOL_PENCIL,
    PAINT_TOOL_BRUSH,
    PAINT_TOOL_ERASER,
    PAINT_TOOL_FILL,
    PAINT_TOOL_PICKER,
    PAINT_TOOL_LINE,
    PAINT_TOOL_RECT,
    PAINT_TOOL_ELLIPSE,
    PAINT_TOOL_POLYGON,
    PAINT_TOOL_TEXT,
    PAINT_TOOL_SELECT_RECT,
    PAINT_TOOL_SELECT_LASSO,
    PAINT_TOOL_SPRAY,
    PAINT_TOOL_BLUR,
    PAINT_TOOL_SHARPEN
} PaintToolType;

typedef enum {
    HALBRUSH_ROUND,
    HALBRUSH_SQUARE,
    HALBRUSH_SOFT,
    HALBRUSH_SPRAY,
    HALBRUSH_CALLIGRAPHY
} HalBrushType;

/* Canvas creation and management */
PaintCanvas* paintcanvas_create(HalForm* parent, int x, int y, int w, int h);
void paintcanvas_destroy(PaintCanvas* canvas);
void paintcanvas_clear(PaintCanvas* canvas, COLORREF color);

/* Tool settings */
void paintcanvas_set_tool(PaintCanvas* canvas, PaintToolType tool);
void paintcanvas_set_forecolor(PaintCanvas* canvas, COLORREF color);
void paintcanvas_set_backcolor(PaintCanvas* canvas, COLORREF color);
void paintcanvas_set_brush_size(PaintCanvas* canvas, int size);
void paintcanvas_set_brush_type(PaintCanvas* canvas, HalBrushType type);
COLORREF paintcanvas_get_pixel_color(PaintCanvas* canvas, int x, int y);

/* Drawing operations */
void paintcanvas_draw_brush(PaintCanvas* canvas, int x, int y);
void paintcanvas_draw_line_smooth(PaintCanvas* canvas, int x1, int y1, int x2, int y2);
void paintcanvas_flood_fill(PaintCanvas* canvas, int x, int y, COLORREF fillColor);

/* Selection tools */
void paintcanvas_select_rect(PaintCanvas* canvas, int x1, int y1, int x2, int y2);
void paintcanvas_copy_selection(PaintCanvas* canvas);
void paintcanvas_paste_selection(PaintCanvas* canvas, int x, int y);
void paintcanvas_delete_selection(PaintCanvas* canvas);

/* Undo/Redo */
void paintcanvas_save_state(PaintCanvas* canvas);
void paintcanvas_undo(PaintCanvas* canvas);
void paintcanvas_redo(PaintCanvas* canvas);

/* Image filters */
void paintcanvas_invert_colors(PaintCanvas* canvas);
void paintcanvas_grayscale(PaintCanvas* canvas);
void paintcanvas_adjust_brightness(PaintCanvas* canvas, int amount);
void paintcanvas_blur(PaintCanvas* canvas, int radius);

/* File operations */
bool paintcanvas_save_bmp(PaintCanvas* canvas, const char* filename);
bool paintcanvas_load_bmp(PaintCanvas* canvas, const char* filename);

/* Runtime widget management */
void halforms_rt_add_widget(const char* name, void* control, int type);

#ifdef __cplusplus
}
#endif

#endif /* HALFORMS_H */
