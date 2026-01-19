/*
 * HalForms - Extended Runtime Functions
 * IDE-specific functions for HalcyonScript
 */

#include "halforms.h"
#include "../runtime.h"
#include "../value.h"
#include <stdio.h>
#include <string.h>

/* External declarations from halforms_runtime.c */
extern void halforms_rt_add_widget(const char* name, void* control, int type);
extern void* halforms_rt_find_widget_ptr(const char* name);

/* External declarations from halforms_ide_api.c */
extern HcsValue* halforms_ide_create_editor(HcsRuntime* rt, const char* name, int x, int y, int w, int h);
extern HcsValue* halforms_ide_editor_get_text(const char* name);
extern HcsValue* halforms_ide_editor_set_text(const char* name, const char* text);
extern HcsValue* halforms_ide_editor_undo(const char* name);
extern HcsValue* halforms_ide_editor_redo(const char* name);
extern HcsValue* halforms_ide_editor_cut(const char* name);
extern HcsValue* halforms_ide_editor_copy(const char* name);
extern HcsValue* halforms_ide_editor_paste(const char* name);
extern HcsValue* halforms_ide_editor_select_all(const char* name);
extern HcsValue* halforms_ide_editor_find(const char* name, const char* text, bool matchCase, bool forward);
extern HcsValue* halforms_ide_editor_replace_all(const char* name, const char* find, const char* replace, bool matchCase);
extern HcsValue* halforms_ide_editor_get_cursor(const char* name);
extern HcsValue* halforms_ide_editor_goto_line(const char* name, int line);
extern HcsValue* halforms_ide_editor_is_modified(const char* name);
extern HcsValue* halforms_ide_editor_set_language(const char* name, const char* language);
extern HcsValue* halforms_ide_exec(const char* command, const char* workingDir);
extern HcsValue* halforms_ide_clipboard_get(void);
extern HcsValue* halforms_ide_clipboard_set(const char* text);
extern HcsValue* halforms_ide_read_file(const char* path);
extern HcsValue* halforms_ide_write_file(const char* path, const char* content);
extern HcsValue* halforms_ide_file_exists(const char* path);
extern HcsValue* halforms_ide_is_directory(const char* path);
extern HcsValue* halforms_ide_list_directory(const char* path);
extern HcsValue* halforms_ide_get_filename(const char* path);
extern HcsValue* halforms_ide_get_directory(const char* path);
extern HcsValue* halforms_ide_get_extension(const char* path);

/* ============================================
   HalForms.IDE.* Runtime Functions
   ============================================ */

HcsValue* halforms_rt_ide_call(HcsRuntime* rt, const char* func, HcsAstList* args) {
    /* Editor functions */
    if (strcmp(func, "editorGetText") == 0 && args->count >= 1) {
        HcsValue* name_v = runtime_eval(rt, args->items[0]);
        char* name = value_to_string(name_v);
        HcsValue* result = halforms_ide_editor_get_text(name);
        free(name);
        value_release(name_v);
        return result;
    }
    
    if (strcmp(func, "editorSetText") == 0 && args->count >= 2) {
        HcsValue* name_v = runtime_eval(rt, args->items[0]);
        HcsValue* text_v = runtime_eval(rt, args->items[1]);
        char* name = value_to_string(name_v);
        char* text = value_to_string(text_v);
        HcsValue* result = halforms_ide_editor_set_text(name, text);
        free(name); free(text);
        value_release(name_v); value_release(text_v);
        return result;
    }
    
    if (strcmp(func, "editorUndo") == 0 && args->count >= 1) {
        HcsValue* name_v = runtime_eval(rt, args->items[0]);
        char* name = value_to_string(name_v);
        HcsValue* result = halforms_ide_editor_undo(name);
        free(name);
        value_release(name_v);
        return result;
    }
    
    if (strcmp(func, "editorRedo") == 0 && args->count >= 1) {
        HcsValue* name_v = runtime_eval(rt, args->items[0]);
        char* name = value_to_string(name_v);
        HcsValue* result = halforms_ide_editor_redo(name);
        free(name);
        value_release(name_v);
        return result;
    }
    
    if (strcmp(func, "editorCut") == 0 && args->count >= 1) {
        HcsValue* name_v = runtime_eval(rt, args->items[0]);
        char* name = value_to_string(name_v);
        HcsValue* result = halforms_ide_editor_cut(name);
        free(name);
        value_release(name_v);
        return result;
    }
    
    if (strcmp(func, "editorCopy") == 0 && args->count >= 1) {
        HcsValue* name_v = runtime_eval(rt, args->items[0]);
        char* name = value_to_string(name_v);
        HcsValue* result = halforms_ide_editor_copy(name);
        free(name);
        value_release(name_v);
        return result;
    }
    
    if (strcmp(func, "editorPaste") == 0 && args->count >= 1) {
        HcsValue* name_v = runtime_eval(rt, args->items[0]);
        char* name = value_to_string(name_v);
        HcsValue* result = halforms_ide_editor_paste(name);
        free(name);
        value_release(name_v);
        return result;
    }
    
    if (strcmp(func, "editorSelectAll") == 0 && args->count >= 1) {
        HcsValue* name_v = runtime_eval(rt, args->items[0]);
        char* name = value_to_string(name_v);
        HcsValue* result = halforms_ide_editor_select_all(name);
        free(name);
        value_release(name_v);
        return result;
    }
    
    if (strcmp(func, "editorFind") == 0 && args->count >= 2) {
        HcsValue* name_v = runtime_eval(rt, args->items[0]);
        HcsValue* text_v = runtime_eval(rt, args->items[1]);
        char* name = value_to_string(name_v);
        char* text = value_to_string(text_v);
        bool matchCase = args->count > 2 ? value_to_bool(runtime_eval(rt, args->items[2])) : false;
        bool forward = args->count > 3 ? value_to_bool(runtime_eval(rt, args->items[3])) : true;
        HcsValue* result = halforms_ide_editor_find(name, text, matchCase, forward);
        free(name); free(text);
        value_release(name_v); value_release(text_v);
        return result;
    }
    
    if (strcmp(func, "editorReplaceAll") == 0 && args->count >= 3) {
        HcsValue* name_v = runtime_eval(rt, args->items[0]);
        HcsValue* find_v = runtime_eval(rt, args->items[1]);
        HcsValue* replace_v = runtime_eval(rt, args->items[2]);
        char* name = value_to_string(name_v);
        char* find = value_to_string(find_v);
        char* replace = value_to_string(replace_v);
        bool matchCase = args->count > 3 ? value_to_bool(runtime_eval(rt, args->items[3])) : false;
        HcsValue* result = halforms_ide_editor_replace_all(name, find, replace, matchCase);
        free(name); free(find); free(replace);
        value_release(name_v); value_release(find_v); value_release(replace_v);
        return result;
    }
    
    if (strcmp(func, "editorGetCursor") == 0 && args->count >= 1) {
        HcsValue* name_v = runtime_eval(rt, args->items[0]);
        char* name = value_to_string(name_v);
        HcsValue* result = halforms_ide_editor_get_cursor(name);
        free(name);
        value_release(name_v);
        return result;
    }
    
    if (strcmp(func, "editorGotoLine") == 0 && args->count >= 2) {
        HcsValue* name_v = runtime_eval(rt, args->items[0]);
        HcsValue* line_v = runtime_eval(rt, args->items[1]);
        char* name = value_to_string(name_v);
        int line = (int)value_to_number(line_v);
        HcsValue* result = halforms_ide_editor_goto_line(name, line);
        free(name);
        value_release(name_v); value_release(line_v);
        return result;
    }
    
    if (strcmp(func, "editorIsModified") == 0 && args->count >= 1) {
        HcsValue* name_v = runtime_eval(rt, args->items[0]);
        char* name = value_to_string(name_v);
        HcsValue* result = halforms_ide_editor_is_modified(name);
        free(name);
        value_release(name_v);
        return result;
    }
    
    if (strcmp(func, "editorSetLanguage") == 0 && args->count >= 2) {
        HcsValue* name_v = runtime_eval(rt, args->items[0]);
        HcsValue* lang_v = runtime_eval(rt, args->items[1]);
        char* name = value_to_string(name_v);
        char* lang = value_to_string(lang_v);
        HcsValue* result = halforms_ide_editor_set_language(name, lang);
        free(name); free(lang);
        value_release(name_v); value_release(lang_v);
        return result;
    }
    
    /* Process execution */
    if (strcmp(func, "exec") == 0 && args->count >= 1) {
        HcsValue* cmd_v = runtime_eval(rt, args->items[0]);
        char* cmd = value_to_string(cmd_v);
        char* workDir = NULL;
        if (args->count > 1) {
            HcsValue* dir_v = runtime_eval(rt, args->items[1]);
            workDir = value_to_string(dir_v);
            value_release(dir_v);
        }
        HcsValue* result = halforms_ide_exec(cmd, workDir);
        free(cmd);
        if (workDir) free(workDir);
        value_release(cmd_v);
        return result;
    }
    
    /* Clipboard */
    if (strcmp(func, "clipboardGet") == 0) {
        return halforms_ide_clipboard_get();
    }
    
    if (strcmp(func, "clipboardSet") == 0 && args->count >= 1) {
        HcsValue* text_v = runtime_eval(rt, args->items[0]);
        char* text = value_to_string(text_v);
        HcsValue* result = halforms_ide_clipboard_set(text);
        free(text);
        value_release(text_v);
        return result;
    }
    
    /* File operations */
    if (strcmp(func, "readFile") == 0 && args->count >= 1) {
        HcsValue* path_v = runtime_eval(rt, args->items[0]);
        char* path = value_to_string(path_v);
        HcsValue* result = halforms_ide_read_file(path);
        free(path);
        value_release(path_v);
        return result;
    }
    
    if (strcmp(func, "writeFile") == 0 && args->count >= 2) {
        HcsValue* path_v = runtime_eval(rt, args->items[0]);
        HcsValue* content_v = runtime_eval(rt, args->items[1]);
        char* path = value_to_string(path_v);
        char* content = value_to_string(content_v);
        HcsValue* result = halforms_ide_write_file(path, content);
        free(path); free(content);
        value_release(path_v); value_release(content_v);
        return result;
    }
    
    if (strcmp(func, "fileExists") == 0 && args->count >= 1) {
        HcsValue* path_v = runtime_eval(rt, args->items[0]);
        char* path = value_to_string(path_v);
        HcsValue* result = halforms_ide_file_exists(path);
        free(path);
        value_release(path_v);
        return result;
    }
    
    if (strcmp(func, "isDirectory") == 0 && args->count >= 1) {
        HcsValue* path_v = runtime_eval(rt, args->items[0]);
        char* path = value_to_string(path_v);
        HcsValue* result = halforms_ide_is_directory(path);
        free(path);
        value_release(path_v);
        return result;
    }
    
    if (strcmp(func, "listDirectory") == 0 && args->count >= 1) {
        HcsValue* path_v = runtime_eval(rt, args->items[0]);
        char* path = value_to_string(path_v);
        HcsValue* result = halforms_ide_list_directory(path);
        free(path);
        value_release(path_v);
        return result;
    }
    
    /* Path utilities */
    if (strcmp(func, "getFilename") == 0 && args->count >= 1) {
        HcsValue* path_v = runtime_eval(rt, args->items[0]);
        char* path = value_to_string(path_v);
        HcsValue* result = halforms_ide_get_filename(path);
        free(path);
        value_release(path_v);
        return result;
    }
    
    if (strcmp(func, "getDirectory") == 0 && args->count >= 1) {
        HcsValue* path_v = runtime_eval(rt, args->items[0]);
        char* path = value_to_string(path_v);
        HcsValue* result = halforms_ide_get_directory(path);
        free(path);
        value_release(path_v);
        return result;
    }
    
    if (strcmp(func, "getExtension") == 0 && args->count >= 1) {
        HcsValue* path_v = runtime_eval(rt, args->items[0]);
        char* path = value_to_string(path_v);
        HcsValue* result = halforms_ide_get_extension(path);
        free(path);
        value_release(path_v);
        return result;
    }
    
    return value_null();
}

/* ============================================
   String Method Implementations for Runtime
   ============================================ */

HcsValue* halforms_rt_string_method(HcsRuntime* rt, HcsValue* str, const char* method, HcsAstList* args) {
    if (strcmp(method, "length") == 0) {
        return value_number(str->data.string ? strlen(str->data.string) : 0);
    }
    
    if (strcmp(method, "upper") == 0 || strcmp(method, "toUpperCase") == 0) {
        return value_string_upper(str);
    }
    
    if (strcmp(method, "lower") == 0 || strcmp(method, "toLowerCase") == 0) {
        return value_string_lower(str);
    }
    
    if (strcmp(method, "trim") == 0) {
        return value_string_trim(str);
    }
    
    if (strcmp(method, "split") == 0 && args->count >= 1) {
        HcsValue* delim_v = runtime_eval(rt, args->items[0]);
        char* delim = value_to_string(delim_v);
        HcsValue* result = value_string_split(str, delim);
        free(delim);
        value_release(delim_v);
        return result;
    }
    
    if (strcmp(method, "substring") == 0 || strcmp(method, "substr") == 0) {
        int start = args->count > 0 ? (int)value_to_number(runtime_eval(rt, args->items[0])) : 0;
        int end = args->count > 1 ? (int)value_to_number(runtime_eval(rt, args->items[1])) : -1;
        return value_string_substring(str, start, end);
    }
    
    if (strcmp(method, "indexOf") == 0 && args->count >= 1) {
        HcsValue* search_v = runtime_eval(rt, args->items[0]);
        char* search = value_to_string(search_v);
        int result = value_string_indexof(str, search);
        free(search);
        value_release(search_v);
        return value_number(result);
    }
    
    if (strcmp(method, "contains") == 0 && args->count >= 1) {
        HcsValue* search_v = runtime_eval(rt, args->items[0]);
        char* search = value_to_string(search_v);
        bool result = value_string_contains(str, search);
        free(search);
        value_release(search_v);
        return value_bool(result);
    }
    
    if (strcmp(method, "startsWith") == 0 && args->count >= 1) {
        HcsValue* prefix_v = runtime_eval(rt, args->items[0]);
        char* prefix = value_to_string(prefix_v);
        bool result = value_string_startswith(str, prefix);
        free(prefix);
        value_release(prefix_v);
        return value_bool(result);
    }
    
    if (strcmp(method, "endsWith") == 0 && args->count >= 1) {
        HcsValue* suffix_v = runtime_eval(rt, args->items[0]);
        char* suffix = value_to_string(suffix_v);
        bool result = value_string_endswith(str, suffix);
        free(suffix);
        value_release(suffix_v);
        return value_bool(result);
    }
    
    if (strcmp(method, "replace") == 0 && args->count >= 2) {
        HcsValue* find_v = runtime_eval(rt, args->items[0]);
        HcsValue* replace_v = runtime_eval(rt, args->items[1]);
        char* find = value_to_string(find_v);
        char* replace = value_to_string(replace_v);
        HcsValue* result = value_string_replace(str, find, replace);
        free(find); free(replace);
        value_release(find_v); value_release(replace_v);
        return result;
    }
    
    return value_null();
}

/* ============================================
   Array Method Implementations for Runtime
   ============================================ */

HcsValue* halforms_rt_array_method(HcsRuntime* rt, HcsValue* arr, const char* method, HcsAstList* args) {
    if (strcmp(method, "length") == 0) {
        return value_number(value_array_length(arr));
    }
    
    if (strcmp(method, "push") == 0 && args->count >= 1) {
        HcsValue* item = runtime_eval(rt, args->items[0]);
        value_array_push(arr, item);
        return value_number(value_array_length(arr));
    }
    
    if (strcmp(method, "pop") == 0) {
        int len = value_array_length(arr);
        if (len > 0) {
            HcsValue* item = value_copy(value_array_get(arr, len - 1));
            value_array_remove(arr, len - 1);
            return item;
        }
        return value_null();
    }
    
    if (strcmp(method, "shift") == 0) {
        int len = value_array_length(arr);
        if (len > 0) {
            HcsValue* item = value_copy(value_array_get(arr, 0));
            value_array_remove(arr, 0);
            return item;
        }
        return value_null();
    }
    
    if (strcmp(method, "unshift") == 0 && args->count >= 1) {
        HcsValue* item = runtime_eval(rt, args->items[0]);
        value_array_insert(arr, 0, item);
        return value_number(value_array_length(arr));
    }
    
    if (strcmp(method, "join") == 0) {
        char* sep = args->count > 0 ? value_to_string(runtime_eval(rt, args->items[0])) : strdup(",");
        HcsValue* result = value_array_join(arr, sep);
        free(sep);
        return result;
    }
    
    if (strcmp(method, "slice") == 0) {
        int start = args->count > 0 ? (int)value_to_number(runtime_eval(rt, args->items[0])) : 0;
        int end = args->count > 1 ? (int)value_to_number(runtime_eval(rt, args->items[1])) : -1;
        return value_array_slice(arr, start, end);
    }
    
    if (strcmp(method, "reverse") == 0) {
        return value_array_reverse(arr);
    }
    
    if (strcmp(method, "indexOf") == 0 && args->count >= 1) {
        HcsValue* item = runtime_eval(rt, args->items[0]);
        int idx = value_array_indexof(arr, item);
        value_release(item);
        return value_number(idx);
    }
    
    if (strcmp(method, "includes") == 0 && args->count >= 1) {
        HcsValue* item = runtime_eval(rt, args->items[0]);
        int idx = value_array_indexof(arr, item);
        value_release(item);
        return value_bool(idx >= 0);
    }
    
    if (strcmp(method, "clear") == 0) {
        value_array_clear(arr);
        return value_null();
    }
    
    if (strcmp(method, "remove") == 0 && args->count >= 1) {
        int idx = (int)value_to_number(runtime_eval(rt, args->items[0]));
        value_array_remove(arr, idx);
        return value_null();
    }
    
    if (strcmp(method, "insert") == 0 && args->count >= 2) {
        int idx = (int)value_to_number(runtime_eval(rt, args->items[0]));
        HcsValue* item = runtime_eval(rt, args->items[1]);
        value_array_insert(arr, idx, item);
        return value_null();
    }
    
    return value_null();
}
