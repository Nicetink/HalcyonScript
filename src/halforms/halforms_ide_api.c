/*
 * HalForms - IDE-specific API for HalcyonScript Runtime
 * High-level functions for building IDEs
 */

#include "halforms.h"
#include "../runtime.h"
#include <stdio.h>
#include <string.h>
#include <shlobj.h>

/* External declarations */
extern void halforms_rt_add_widget(const char* name, void* control, int type);
extern void* halforms_rt_find_widget(const char* name);
extern HalForm* halforms_rt_get_main_form(void);

/* Process management */
extern char* halprocess_exec(const char* command, const char* workingDir, int* exitCode);

/* Clipboard */
extern char* halforms_clipboard_get_text(void);
extern bool halforms_clipboard_set_text(const char* text);

/* Code editor */
extern HalCodeEditor* halcodeeditor_create(HalForm* parent, int x, int y, int w, int h);
extern void halcodeeditor_set_text(HalCodeEditor* editor, const char* text);
extern char* halcodeeditor_get_text(HalCodeEditor* editor);
extern void halcodeeditor_set_language(HalCodeEditor* editor, const char* language);
extern void halcodeeditor_undo(HalCodeEditor* editor);
extern void halcodeeditor_redo(HalCodeEditor* editor);
extern void halcodeeditor_cut(HalCodeEditor* editor);
extern void halcodeeditor_copy(HalCodeEditor* editor);
extern void halcodeeditor_paste(HalCodeEditor* editor);
extern void halcodeeditor_select_all(HalCodeEditor* editor);
extern int halcodeeditor_find(HalCodeEditor* editor, const char* text, bool matchCase, bool wholeWord, bool forward);
extern int halcodeeditor_replace_all(HalCodeEditor* editor, const char* findText, const char* replaceText, bool matchCase);
extern int halcodeeditor_get_current_line(HalCodeEditor* editor);
extern int halcodeeditor_get_current_column(HalCodeEditor* editor);
extern int halcodeeditor_get_line_count(HalCodeEditor* editor);
extern bool halcodeeditor_is_modified(HalCodeEditor* editor);
extern void halcodeeditor_set_modified(HalCodeEditor* editor, bool modified);
extern void halcodeeditor_goto_line(HalCodeEditor* editor, int line);
extern void halcodeeditor_set_dark_mode(HalCodeEditor* editor, bool dark);

/* Splitter */
extern HalSplitter* halsplitter_create(HalForm* parent, bool horizontal, int x, int y, int w, int h);
extern void halsplitter_set_position(HalSplitter* splitter, int position);
extern HWND halsplitter_get_panel1(HalSplitter* splitter);
extern HWND halsplitter_get_panel2(HalSplitter* splitter);

/* Control type identifiers */
#define HALFORMS_TYPE_CODEEDITOR    16
#define HALFORMS_TYPE_SPLITTER      23
#define HALFORMS_TYPE_PROCESS       24

/* ============================================
   IDE API - Code Editor Functions
   ============================================ */

HcsValue* halforms_ide_create_editor(HcsRuntime* rt, const char* name, int x, int y, int w, int h) {
    HalForm* mainForm = halforms_rt_get_main_form();
    if (!mainForm) return value_bool(false);
    
    HalCodeEditor* editor = halcodeeditor_create(mainForm, x, y, w, h);
    if (editor) {
        halforms_rt_add_widget(name, editor, HALFORMS_TYPE_CODEEDITOR);
        return value_bool(true);
    }
    return value_bool(false);
}

HcsValue* halforms_ide_editor_get_text(const char* name) {
    void* widget = halforms_rt_find_widget(name);
    if (!widget) return value_string("");
    
    HalCodeEditor* editor = (HalCodeEditor*)widget;
    char* text = halcodeeditor_get_text(editor);
    HcsValue* result = value_string(text ? text : "");
    free(text);
    return result;
}

HcsValue* halforms_ide_editor_set_text(const char* name, const char* text) {
    void* widget = halforms_rt_find_widget(name);
    if (!widget) return value_bool(false);
    
    HalCodeEditor* editor = (HalCodeEditor*)widget;
    halcodeeditor_set_text(editor, text);
    return value_bool(true);
}

HcsValue* halforms_ide_editor_undo(const char* name) {
    void* widget = halforms_rt_find_widget(name);
    if (!widget) return value_bool(false);
    
    halcodeeditor_undo((HalCodeEditor*)widget);
    return value_bool(true);
}

HcsValue* halforms_ide_editor_redo(const char* name) {
    void* widget = halforms_rt_find_widget(name);
    if (!widget) return value_bool(false);
    
    halcodeeditor_redo((HalCodeEditor*)widget);
    return value_bool(true);
}

HcsValue* halforms_ide_editor_cut(const char* name) {
    void* widget = halforms_rt_find_widget(name);
    if (!widget) return value_bool(false);
    
    halcodeeditor_cut((HalCodeEditor*)widget);
    return value_bool(true);
}

HcsValue* halforms_ide_editor_copy(const char* name) {
    void* widget = halforms_rt_find_widget(name);
    if (!widget) return value_bool(false);
    
    halcodeeditor_copy((HalCodeEditor*)widget);
    return value_bool(true);
}

HcsValue* halforms_ide_editor_paste(const char* name) {
    void* widget = halforms_rt_find_widget(name);
    if (!widget) return value_bool(false);
    
    halcodeeditor_paste((HalCodeEditor*)widget);
    return value_bool(true);
}

HcsValue* halforms_ide_editor_select_all(const char* name) {
    void* widget = halforms_rt_find_widget(name);
    if (!widget) return value_bool(false);
    
    halcodeeditor_select_all((HalCodeEditor*)widget);
    return value_bool(true);
}

HcsValue* halforms_ide_editor_find(const char* name, const char* text, bool matchCase, bool forward) {
    void* widget = halforms_rt_find_widget(name);
    if (!widget) return value_number(-1);
    
    int pos = halcodeeditor_find((HalCodeEditor*)widget, text, matchCase, false, forward);
    return value_number(pos);
}

HcsValue* halforms_ide_editor_replace_all(const char* name, const char* find, const char* replace, bool matchCase) {
    void* widget = halforms_rt_find_widget(name);
    if (!widget) return value_number(0);
    
    int count = halcodeeditor_replace_all((HalCodeEditor*)widget, find, replace, matchCase);
    return value_number(count);
}

HcsValue* halforms_ide_editor_get_cursor(const char* name) {
    void* widget = halforms_rt_find_widget(name);
    if (!widget) return value_null();
    
    HalCodeEditor* editor = (HalCodeEditor*)widget;
    
    /* Return object with line and column */
    HcsValue* result = value_object();
    value_object_set(result, "line", value_number(halcodeeditor_get_current_line(editor)));
    value_object_set(result, "column", value_number(halcodeeditor_get_current_column(editor)));
    return result;
}

HcsValue* halforms_ide_editor_goto_line(const char* name, int line) {
    void* widget = halforms_rt_find_widget(name);
    if (!widget) return value_bool(false);
    
    halcodeeditor_goto_line((HalCodeEditor*)widget, line);
    return value_bool(true);
}

HcsValue* halforms_ide_editor_is_modified(const char* name) {
    void* widget = halforms_rt_find_widget(name);
    if (!widget) return value_bool(false);
    
    return value_bool(halcodeeditor_is_modified((HalCodeEditor*)widget));
}

HcsValue* halforms_ide_editor_set_language(const char* name, const char* language) {
    void* widget = halforms_rt_find_widget(name);
    if (!widget) return value_bool(false);
    
    halcodeeditor_set_language((HalCodeEditor*)widget, language);
    return value_bool(true);
}

/* ============================================
   IDE API - Process Execution
   ============================================ */

HcsValue* halforms_ide_exec(const char* command, const char* workingDir) {
    int exitCode = 0;
    char* output = halprocess_exec(command, workingDir, &exitCode);
    
    HcsValue* result = value_object();
    value_object_set(result, "output", value_string(output ? output : ""));
    value_object_set(result, "exitCode", value_number(exitCode));
    
    free(output);
    return result;
}

HcsValue* halforms_ide_compile(const char* compiler, const char* sourceFile, const char* outputFile, const char* flags) {
    char command[4096];
    snprintf(command, sizeof(command), "%s %s -o %s %s", compiler, sourceFile, outputFile, flags ? flags : "");
    
    return halforms_ide_exec(command, NULL);
}

HcsValue* halforms_ide_run_program(const char* program, const char* args, const char* workingDir) {
    char command[4096];
    if (args && args[0]) {
        snprintf(command, sizeof(command), "\"%s\" %s", program, args);
    } else {
        snprintf(command, sizeof(command), "\"%s\"", program);
    }
    
    return halforms_ide_exec(command, workingDir);
}

/* ============================================
   IDE API - Clipboard
   ============================================ */

HcsValue* halforms_ide_clipboard_get(void) {
    char* text = halforms_clipboard_get_text();
    HcsValue* result = value_string(text ? text : "");
    free(text);
    return result;
}

HcsValue* halforms_ide_clipboard_set(const char* text) {
    return value_bool(halforms_clipboard_set_text(text));
}

/* ============================================
   IDE API - File Operations
   ============================================ */

HcsValue* halforms_ide_read_file(const char* path) {
    FILE* f = fopen(path, "rb");
    if (!f) return value_null();
    
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    char* content = (char*)malloc(size + 1);
    fread(content, 1, size, f);
    content[size] = '\0';
    fclose(f);
    
    HcsValue* result = value_string(content);
    free(content);
    return result;
}

HcsValue* halforms_ide_write_file(const char* path, const char* content) {
    FILE* f = fopen(path, "wb");
    if (!f) return value_bool(false);
    
    size_t len = strlen(content);
    size_t written = fwrite(content, 1, len, f);
    fclose(f);
    
    return value_bool(written == len);
}

HcsValue* halforms_ide_file_exists(const char* path) {
    DWORD attrs = GetFileAttributesA(path);
    return value_bool(attrs != INVALID_FILE_ATTRIBUTES);
}

HcsValue* halforms_ide_is_directory(const char* path) {
    DWORD attrs = GetFileAttributesA(path);
    return value_bool(attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY));
}

HcsValue* halforms_ide_list_directory(const char* path) {
    HcsValue* result = value_array();
    
    char searchPath[MAX_PATH];
    snprintf(searchPath, sizeof(searchPath), "%s\\*", path);
    
    WIN32_FIND_DATAA fd;
    HANDLE hFind = FindFirstFileA(searchPath, &fd);
    
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (strcmp(fd.cFileName, ".") != 0 && strcmp(fd.cFileName, "..") != 0) {
                HcsValue* entry = value_object();
                value_object_set(entry, "name", value_string(fd.cFileName));
                value_object_set(entry, "isDirectory", value_bool((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0));
                value_object_set(entry, "size", value_number((double)(((__int64)fd.nFileSizeHigh << 32) | fd.nFileSizeLow)));
                value_array_push(result, entry);
            }
        } while (FindNextFileA(hFind, &fd));
        FindClose(hFind);
    }
    
    return result;
}

HcsValue* halforms_ide_create_directory(const char* path) {
    return value_bool(CreateDirectoryA(path, NULL) || GetLastError() == ERROR_ALREADY_EXISTS);
}

HcsValue* halforms_ide_delete_file(const char* path) {
    return value_bool(DeleteFileA(path) != 0);
}

HcsValue* halforms_ide_delete_directory(const char* path) {
    return value_bool(RemoveDirectoryA(path) != 0);
}

HcsValue* halforms_ide_rename(const char* oldPath, const char* newPath) {
    return value_bool(MoveFileA(oldPath, newPath) != 0);
}

HcsValue* halforms_ide_copy_file(const char* src, const char* dst) {
    return value_bool(CopyFileA(src, dst, FALSE) != 0);
}

/* ============================================
   IDE API - Path Operations
   ============================================ */

HcsValue* halforms_ide_get_filename(const char* path) {
    const char* name = strrchr(path, '\\');
    if (!name) name = strrchr(path, '/');
    return value_string(name ? name + 1 : path);
}

HcsValue* halforms_ide_get_directory(const char* path) {
    char* copy = _strdup(path);
    char* sep = strrchr(copy, '\\');
    if (!sep) sep = strrchr(copy, '/');
    if (sep) *sep = '\0';
    HcsValue* result = value_string(copy);
    free(copy);
    return result;
}

HcsValue* halforms_ide_get_extension(const char* path) {
    const char* ext = strrchr(path, '.');
    return value_string(ext ? ext : "");
}

HcsValue* halforms_ide_join_path(const char* base, const char* relative) {
    char result[MAX_PATH];
    snprintf(result, sizeof(result), "%s\\%s", base, relative);
    return value_string(result);
}

HcsValue* halforms_ide_get_absolute_path(const char* path) {
    char result[MAX_PATH];
    if (GetFullPathNameA(path, sizeof(result), result, NULL)) {
        return value_string(result);
    }
    return value_string(path);
}

/* ============================================
   IDE API - Recent Files
   ============================================ */

static char** g_recentFiles = NULL;
static int g_recentFileCount = 0;
static int g_recentFileCapacity = 0;
static int g_maxRecentFiles = 10;

void halforms_ide_add_recent_file(const char* path) {
    /* Remove if already exists */
    for (int i = 0; i < g_recentFileCount; i++) {
        if (strcmp(g_recentFiles[i], path) == 0) {
            free(g_recentFiles[i]);
            for (int j = i; j < g_recentFileCount - 1; j++) {
                g_recentFiles[j] = g_recentFiles[j + 1];
            }
            g_recentFileCount--;
            break;
        }
    }
    
    /* Add to front */
    if (g_recentFileCount >= g_recentFileCapacity) {
        g_recentFileCapacity = g_recentFileCapacity == 0 ? 16 : g_recentFileCapacity * 2;
        g_recentFiles = (char**)realloc(g_recentFiles, g_recentFileCapacity * sizeof(char*));
    }
    
    /* Shift existing */
    for (int i = g_recentFileCount; i > 0; i--) {
        g_recentFiles[i] = g_recentFiles[i - 1];
    }
    g_recentFiles[0] = _strdup(path);
    g_recentFileCount++;
    
    /* Trim to max */
    while (g_recentFileCount > g_maxRecentFiles) {
        free(g_recentFiles[--g_recentFileCount]);
    }
}

HcsValue* halforms_ide_get_recent_files(void) {
    HcsValue* result = value_array();
    for (int i = 0; i < g_recentFileCount; i++) {
        value_array_push(result, value_string(g_recentFiles[i]));
    }
    return result;
}

/* ============================================
   IDE API - Settings
   ============================================ */

static char* g_settingsPath = NULL;

void halforms_ide_set_settings_path(const char* path) {
    free(g_settingsPath);
    g_settingsPath = _strdup(path);
}

HcsValue* halforms_ide_load_settings(void) {
    if (!g_settingsPath) return value_object();
    
    HcsValue* content = halforms_ide_read_file(g_settingsPath);
    if (content->type == HCS_VAL_NULL) {
        value_release(content);
        return value_object();
    }
    
    /* Simple key=value parser */
    HcsValue* settings = value_object();
    char* text = _strdup(content->data.string);
    value_release(content);
    
    char* line = strtok(text, "\n");
    while (line) {
        char* eq = strchr(line, '=');
        if (eq) {
            *eq = '\0';
            char* key = line;
            char* val = eq + 1;
            
            /* Trim whitespace */
            while (*key == ' ' || *key == '\t') key++;
            while (*val == ' ' || *val == '\t') val++;
            char* end = key + strlen(key) - 1;
            while (end > key && (*end == ' ' || *end == '\t' || *end == '\r')) *end-- = '\0';
            end = val + strlen(val) - 1;
            while (end > val && (*end == ' ' || *end == '\t' || *end == '\r')) *end-- = '\0';
            
            value_object_set(settings, key, value_string(val));
        }
        line = strtok(NULL, "\n");
    }
    
    free(text);
    return settings;
}

HcsValue* halforms_ide_save_settings(HcsValue* settings) {
    if (!g_settingsPath || settings->type != HCS_VAL_OBJECT) return value_bool(false);
    
    /* Build settings string */
    char buffer[65536];
    int pos = 0;
    
    /* Iterate object properties */
    for (int i = 0; i < settings->data.object.count; i++) {
        char* key = settings->data.object.keys[i];
        HcsValue* val = settings->data.object.values[i];
        char* valStr = value_to_string(val);
        pos += snprintf(buffer + pos, sizeof(buffer) - pos, "%s=%s\n", key, valStr);
        free(valStr);
    }
    
    return halforms_ide_write_file(g_settingsPath, buffer);
}
