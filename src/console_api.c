/*
 * HalcyonScript © KAInaps 2026 
   Simple programming for creative minds 
   github.com/Nicetink/HalcyonScript
 */

#include "runtime.h"
#include <windows.h>
#include <stdio.h>
#include <conio.h>

static HANDLE hConsole = NULL;
static WORD defaultAttributes = 0;

HcsValue* builtin_console_init(int argc, HcsValue** args) {
    if (!hConsole) {
        AllocConsole();
        hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        GetConsoleScreenBufferInfo(hConsole, &csbi);
        defaultAttributes = csbi.wAttributes;
    }
    return value_bool(true);
}

HcsValue* builtin_console_clear(int argc, HcsValue** args) {
    system("cls");
    return value_null();
}

HcsValue* builtin_console_write(int argc, HcsValue** args) {
    for (int i = 0; i < argc; i++) {
        if (args[i]->type == HCS_VAL_STRING) {
            printf("%s", args[i]->data.string);
        } else if (args[i]->type == HCS_VAL_NUMBER) {
            printf("%g", args[i]->data.number);
        } else if (args[i]->type == HCS_VAL_BOOL) {
            printf("%s", args[i]->data.boolean ? "true" : "false");
        }
    }
    return value_null();
}

HcsValue* builtin_console_writeln(int argc, HcsValue** args) {
    builtin_console_write(argc, args);
    printf("\n");
    return value_null();
}

HcsValue* builtin_console_read(int argc, HcsValue** args) {
    char buffer[4096];
    if (fgets(buffer, sizeof(buffer), stdin)) {
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len-1] == '\n') buffer[len-1] = 0;
        return value_string(buffer);
    }
    return value_string("");
}

HcsValue* builtin_console_readkey(int argc, HcsValue** args) {
    int ch = _getch();
    char str[2] = {(char)ch, 0};
    return value_string(str);
}

HcsValue* builtin_console_set_color(int argc, HcsValue** args) {
    if (argc != 1 || args[0]->type != HCS_VAL_STRING) return value_null();
    if (!hConsole) builtin_console_init(0, NULL);
    
    const char* color = args[0]->data.string;
    WORD attr = defaultAttributes;
    
    if (strcmp(color, "black") == 0) attr = 0;
    else if (strcmp(color, "blue") == 0) attr = FOREGROUND_BLUE;
    else if (strcmp(color, "green") == 0) attr = FOREGROUND_GREEN;
    else if (strcmp(color, "cyan") == 0) attr = FOREGROUND_BLUE | FOREGROUND_GREEN;
    else if (strcmp(color, "red") == 0) attr = FOREGROUND_RED;
    else if (strcmp(color, "magenta") == 0) attr = FOREGROUND_RED | FOREGROUND_BLUE;
    else if (strcmp(color, "yellow") == 0) attr = FOREGROUND_RED | FOREGROUND_GREEN;
    else if (strcmp(color, "white") == 0) attr = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
    else if (strcmp(color, "gray") == 0) attr = FOREGROUND_INTENSITY;
    else if (strcmp(color, "bright_blue") == 0) attr = FOREGROUND_BLUE | FOREGROUND_INTENSITY;
    else if (strcmp(color, "bright_green") == 0) attr = FOREGROUND_GREEN | FOREGROUND_INTENSITY;
    else if (strcmp(color, "bright_cyan") == 0) attr = FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
    else if (strcmp(color, "bright_red") == 0) attr = FOREGROUND_RED | FOREGROUND_INTENSITY;
    else if (strcmp(color, "bright_magenta") == 0) attr = FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
    else if (strcmp(color, "bright_yellow") == 0) attr = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
    else if (strcmp(color, "bright_white") == 0) attr = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
    
    SetConsoleTextAttribute(hConsole, attr);
    return value_null();
}

HcsValue* builtin_console_reset_color(int argc, HcsValue** args) {
    if (!hConsole) builtin_console_init(0, NULL);
    SetConsoleTextAttribute(hConsole, defaultAttributes);
    return value_null();
}

HcsValue* builtin_console_set_title(int argc, HcsValue** args) {
    if (argc != 1 || args[0]->type != HCS_VAL_STRING) return value_null();
    SetConsoleTitleA(args[0]->data.string);
    return value_null();
}

HcsValue* builtin_console_get_size(int argc, HcsValue** args) {
    if (!hConsole) builtin_console_init(0, NULL);
    
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hConsole, &csbi);
    
    HcsValue* result = value_object();
    value_object_set(result, "width", value_number(csbi.srWindow.Right - csbi.srWindow.Left + 1));
    value_object_set(result, "height", value_number(csbi.srWindow.Bottom - csbi.srWindow.Top + 1));
    return result;
}

HcsValue* builtin_console_set_cursor(int argc, HcsValue** args) {
    if (argc != 2 || args[0]->type != HCS_VAL_NUMBER || args[1]->type != HCS_VAL_NUMBER) {
        return value_null();
    }
    if (!hConsole) builtin_console_init(0, NULL);
    
    COORD coord;
    coord.X = (SHORT)args[0]->data.number;
    coord.Y = (SHORT)args[1]->data.number;
    SetConsoleCursorPosition(hConsole, coord);
    return value_null();
}

HcsValue* builtin_console_hide_cursor(int argc, HcsValue** args) {
    if (!hConsole) builtin_console_init(0, NULL);
    
    CONSOLE_CURSOR_INFO cci;
    GetConsoleCursorInfo(hConsole, &cci);
    cci.bVisible = FALSE;
    SetConsoleCursorInfo(hConsole, &cci);
    return value_null();
}

HcsValue* builtin_console_show_cursor(int argc, HcsValue** args) {
    if (!hConsole) builtin_console_init(0, NULL);
    
    CONSOLE_CURSOR_INFO cci;
    GetConsoleCursorInfo(hConsole, &cci);
    cci.bVisible = TRUE;
    SetConsoleCursorInfo(hConsole, &cci);
    return value_null();
}

HcsValue* builtin_console_beep(int argc, HcsValue** args) {
    int freq = 800;
    int duration = 200;
    
    if (argc > 0 && args[0]->type == HCS_VAL_NUMBER) freq = (int)args[0]->data.number;
    if (argc > 1 && args[1]->type == HCS_VAL_NUMBER) duration = (int)args[1]->data.number;
    
    Beep(freq, duration);
    return value_null();
}

HcsValue* builtin_sleep(int argc, HcsValue** args) {
    if (argc != 1 || args[0]->type != HCS_VAL_NUMBER) return value_null();
    Sleep((DWORD)args[0]->data.number);
    return value_null();
}
