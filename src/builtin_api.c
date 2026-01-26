/*
 * HalcyonScript © KAInaps 2026 
   Simple programming for creative minds 
   github.com/Nicetink/HalcyonScript
 */

#include "runtime.h"
#include "value.h"
#include <time.h>
#include <windows.h>

HcsValue* call_system_api(HcsRuntime* rt, const char* name, HcsAstList* args);

extern HcsValue* builtin_file_exists(int argc, HcsValue** args);
extern HcsValue* builtin_file_read(int argc, HcsValue** args);
extern HcsValue* builtin_file_write(int argc, HcsValue** args);
extern HcsValue* builtin_file_copy(int argc, HcsValue** args);
extern HcsValue* builtin_file_delete(int argc, HcsValue** args);
extern HcsValue* builtin_file_size(int argc, HcsValue** args);
extern HcsValue* builtin_dir_create(int argc, HcsValue** args);
extern HcsValue* builtin_dir_exists(int argc, HcsValue** args);
extern HcsValue* builtin_dir_delete(int argc, HcsValue** args);
extern HcsValue* builtin_dir_list(int argc, HcsValue** args);
extern HcsValue* builtin_path_join(int argc, HcsValue** args);
extern HcsValue* builtin_path_basename(int argc, HcsValue** args);
extern HcsValue* builtin_path_dirname(int argc, HcsValue** args);
extern HcsValue* builtin_get_temp_path(int argc, HcsValue** args);
extern HcsValue* builtin_get_program_files(int argc, HcsValue** args);
extern HcsValue* builtin_get_appdata(int argc, HcsValue** args);
extern HcsValue* builtin_get_desktop(int argc, HcsValue** args);
extern HcsValue* builtin_get_startmenu(int argc, HcsValue** args);

extern HcsValue* builtin_reg_write(int argc, HcsValue** args);
extern HcsValue* builtin_reg_read(int argc, HcsValue** args);
extern HcsValue* builtin_reg_delete(int argc, HcsValue** args);
extern HcsValue* builtin_reg_key_exists(int argc, HcsValue** args);

extern HcsValue* builtin_exec(int argc, HcsValue** args);
extern HcsValue* builtin_exec_async(int argc, HcsValue** args);
extern HcsValue* builtin_shell_execute(int argc, HcsValue** args);
extern HcsValue* builtin_create_shortcut(int argc, HcsValue** args);

extern HcsValue* builtin_compress_folder(int argc, HcsValue** args);
extern HcsValue* builtin_extract_archive(int argc, HcsValue** args);

extern HcsValue* builtin_console_init(int argc, HcsValue** args);
extern HcsValue* builtin_console_clear(int argc, HcsValue** args);
extern HcsValue* builtin_console_write(int argc, HcsValue** args);
extern HcsValue* builtin_console_writeln(int argc, HcsValue** args);
extern HcsValue* builtin_console_read(int argc, HcsValue** args);
extern HcsValue* builtin_console_readkey(int argc, HcsValue** args);
extern HcsValue* builtin_console_set_color(int argc, HcsValue** args);
extern HcsValue* builtin_console_reset_color(int argc, HcsValue** args);
extern HcsValue* builtin_console_set_title(int argc, HcsValue** args);
extern HcsValue* builtin_console_get_size(int argc, HcsValue** args);
extern HcsValue* builtin_console_set_cursor(int argc, HcsValue** args);
extern HcsValue* builtin_console_hide_cursor(int argc, HcsValue** args);
extern HcsValue* builtin_console_show_cursor(int argc, HcsValue** args);
extern HcsValue* builtin_console_beep(int argc, HcsValue** args);
extern HcsValue* builtin_sleep(int argc, HcsValue** args);

/* New functions */
extern HcsValue* builtin_string_split(int argc, HcsValue** args);
extern HcsValue* builtin_math_random(int argc, HcsValue** args);
extern HcsValue* builtin_tooltip_show(int argc, HcsValue** args);

HcsValue* call_system_api(HcsRuntime* rt, const char* name, HcsAstList* args) {
    HcsValue** arg_values = NULL;
    int arg_count = 0;
    
    if (args && args->count > 0) {
        arg_values = (HcsValue**)malloc(sizeof(HcsValue*) * args->count);
        for (int i = 0; i < args->count; i++) {
            arg_values[i] = eval_expression(rt, args->items[i]);
        }
        arg_count = args->count;
    }
    
    HcsValue* result = NULL;
    
    if (strncmp(name, "File.", 5) == 0) {
        const char* func = name + 5;
        if (strcmp(func, "exists") == 0) result = builtin_file_exists(arg_count, arg_values);
        else if (strcmp(func, "read") == 0) result = builtin_file_read(arg_count, arg_values);
        else if (strcmp(func, "write") == 0) result = builtin_file_write(arg_count, arg_values);
        else if (strcmp(func, "copy") == 0) result = builtin_file_copy(arg_count, arg_values);
        else if (strcmp(func, "delete") == 0) result = builtin_file_delete(arg_count, arg_values);
        else if (strcmp(func, "size") == 0) result = builtin_file_size(arg_count, arg_values);
    }
    else if (strncmp(name, "Dir.", 4) == 0) {
        const char* func = name + 4;
        if (strcmp(func, "create") == 0) result = builtin_dir_create(arg_count, arg_values);
        else if (strcmp(func, "exists") == 0) result = builtin_dir_exists(arg_count, arg_values);
        else if (strcmp(func, "delete") == 0) result = builtin_dir_delete(arg_count, arg_values);
        else if (strcmp(func, "list") == 0) result = builtin_dir_list(arg_count, arg_values);
    }
    else if (strncmp(name, "Path.", 5) == 0) {
        const char* func = name + 5;
        if (strcmp(func, "join") == 0) result = builtin_path_join(arg_count, arg_values);
        else if (strcmp(func, "basename") == 0) result = builtin_path_basename(arg_count, arg_values);
        else if (strcmp(func, "dirname") == 0) result = builtin_path_dirname(arg_count, arg_values);
    }
    else if (strncmp(name, "Sys.", 4) == 0) {
        const char* func = name + 4;
        if (strcmp(func, "getTempPath") == 0) result = builtin_get_temp_path(arg_count, arg_values);
        else if (strcmp(func, "getProgramFiles") == 0) result = builtin_get_program_files(arg_count, arg_values);
        else if (strcmp(func, "getAppData") == 0) result = builtin_get_appdata(arg_count, arg_values);
        else if (strcmp(func, "getDesktop") == 0) result = builtin_get_desktop(arg_count, arg_values);
        else if (strcmp(func, "getStartMenu") == 0) result = builtin_get_startmenu(arg_count, arg_values);
        else if (strcmp(func, "exec") == 0) result = builtin_exec(arg_count, arg_values);
        else if (strcmp(func, "execAsync") == 0) result = builtin_exec_async(arg_count, arg_values);
        else if (strcmp(func, "shellExecute") == 0) result = builtin_shell_execute(arg_count, arg_values);
        else if (strcmp(func, "createShortcut") == 0) result = builtin_create_shortcut(arg_count, arg_values);
        else if (strcmp(func, "sleep") == 0) result = builtin_sleep(arg_count, arg_values);
    }
    else if (strncmp(name, "Registry.", 9) == 0) {
        const char* func = name + 9;
        if (strcmp(func, "write") == 0) result = builtin_reg_write(arg_count, arg_values);
        else if (strcmp(func, "read") == 0) result = builtin_reg_read(arg_count, arg_values);
        else if (strcmp(func, "delete") == 0) result = builtin_reg_delete(arg_count, arg_values);
        else if (strcmp(func, "keyExists") == 0) result = builtin_reg_key_exists(arg_count, arg_values);
    }
    else if (strncmp(name, "Archive.", 8) == 0) {
        const char* func = name + 8;
        if (strcmp(func, "compress") == 0) result = builtin_compress_folder(arg_count, arg_values);
        else if (strcmp(func, "extract") == 0) result = builtin_extract_archive(arg_count, arg_values);
    }
    else if (strncmp(name, "Console.", 8) == 0) {
        const char* func = name + 8;
        if (strcmp(func, "init") == 0) result = builtin_console_init(arg_count, arg_values);
        else if (strcmp(func, "clear") == 0) result = builtin_console_clear(arg_count, arg_values);
        else if (strcmp(func, "write") == 0) result = builtin_console_write(arg_count, arg_values);
        else if (strcmp(func, "writeln") == 0) result = builtin_console_writeln(arg_count, arg_values);
        else if (strcmp(func, "read") == 0) result = builtin_console_read(arg_count, arg_values);
        else if (strcmp(func, "readKey") == 0) result = builtin_console_readkey(arg_count, arg_values);
        else if (strcmp(func, "setColor") == 0) result = builtin_console_set_color(arg_count, arg_values);
        else if (strcmp(func, "resetColor") == 0) result = builtin_console_reset_color(arg_count, arg_values);
        else if (strcmp(func, "setTitle") == 0) result = builtin_console_set_title(arg_count, arg_values);
        else if (strcmp(func, "getSize") == 0) result = builtin_console_get_size(arg_count, arg_values);
        else if (strcmp(func, "setCursor") == 0) result = builtin_console_set_cursor(arg_count, arg_values);
        else if (strcmp(func, "hideCursor") == 0) result = builtin_console_hide_cursor(arg_count, arg_values);
        else if (strcmp(func, "showCursor") == 0) result = builtin_console_show_cursor(arg_count, arg_values);
        else if (strcmp(func, "beep") == 0) result = builtin_console_beep(arg_count, arg_values);
    }
    else if (strncmp(name, "String.", 7) == 0) {
        const char* func = name + 7;
        if (strcmp(func, "split") == 0) result = builtin_string_split(arg_count, arg_values);
    }
    else if (strncmp(name, "Math.", 5) == 0) {
        const char* func = name + 5;
        if (strcmp(func, "random") == 0) result = builtin_math_random(arg_count, arg_values);
    }
    else if (strncmp(name, "Tooltip.", 8) == 0) {
        const char* func = name + 8;
        if (strcmp(func, "show") == 0) result = builtin_tooltip_show(arg_count, arg_values);
    }
    
    if (arg_values) {
        for (int i = 0; i < arg_count; i++) {
            value_release(arg_values[i]);
        }
        free(arg_values);
    }
    
    return result;
}


/* ============================================
   New Built-in Functions
   ============================================ */

/* String.split(str, delimiter) - Split string into array */
HcsValue* builtin_string_split(int argc, HcsValue** args) {
    if (argc < 2) return value_null();
    
    const char* str = value_to_string(args[0]);
    const char* delim = value_to_string(args[1]);
    
    if (!str || !delim || strlen(delim) == 0) {
        return value_null();
    }
    
    /* Create array to hold results */
    HcsValue* arr = value_array();
    
    /* Make a copy of the string since strtok modifies it */
    char* str_copy = strdup(str);
    char* token = strtok(str_copy, delim);
    
    while (token != NULL) {
        value_array_push(arr, value_string(token));
        token = strtok(NULL, delim);
    }
    
    free(str_copy);
    return arr;
}

/* Math.random(min, max) - Generate random number between min and max */
HcsValue* builtin_math_random(int argc, HcsValue** args) {
    static bool seeded = false;
    if (!seeded) {
        srand((unsigned int)time(NULL));
        seeded = true;
    }
    
    if (argc < 2) {
        /* No arguments - return random float between 0 and 1 */
        double r = (double)rand() / (double)RAND_MAX;
        return value_number(r);
    }
    
    double min = value_to_number(args[0]);
    double max = value_to_number(args[1]);
    
    if (min > max) {
        double temp = min;
        min = max;
        max = temp;
    }
    
    /* Generate random number in range */
    double range = max - min;
    double r = min + (range * ((double)rand() / (double)RAND_MAX));
    
    return value_number(r);
}

/* Tooltip.show(text, x, y) - Show tooltip at position */
HcsValue* builtin_tooltip_show(int argc, HcsValue** args) {
    if (argc < 3) return value_bool(false);
    
    const char* text = value_to_string(args[0]);
    int x = (int)value_to_number(args[1]);
    int y = (int)value_to_number(args[2]);
    
    if (!text) return value_bool(false);
    
    /* Create a tooltip window */
    HWND hwndTip = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        "STATIC",
        text,
        WS_POPUP | WS_BORDER | SS_CENTER,
        x, y, 200, 30,
        NULL, NULL, GetModuleHandle(NULL), NULL
    );
    
    if (!hwndTip) return value_bool(false);
    
    /* Set background color to light yellow */
    SetClassLongPtr(hwndTip, GCLP_HBRBACKGROUND, (LONG_PTR)CreateSolidBrush(RGB(255, 255, 200)));
    
    /* Show the tooltip */
    ShowWindow(hwndTip, SW_SHOWNOACTIVATE);
    UpdateWindow(hwndTip);
    
    /* Auto-hide after 3 seconds */
    SetTimer(hwndTip, 1, 3000, NULL);
    
    return value_bool(true);
}
