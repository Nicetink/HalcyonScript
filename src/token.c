/*
 * HalcyonScript - Token implementation
 */

#include "token.h"

HcsToken* token_create(HcsTokenType type, const char* value, int line, int column) {
    HcsToken* token = (HcsToken*)malloc(sizeof(HcsToken));
    if (!token) return NULL;
    
    token->type = type;
    token->value = value ? strdup(value) : NULL;
    token->line = line;
    token->column = column;
    return token;
}

void token_free(HcsToken* token) {
    if (token) {
        free(token->value);
        free(token);
    }
}

const char* token_type_name(HcsTokenType type) {
    static const char* names[] = {
        "CREATE", "WINDOW", "BUTTON", "LABEL", "INPUT", "LISTBOX",
        "CHECKBOX", "IMAGE", "PANEL", "TEXTAREA", "DROPDOWN", "SLIDER",
        "PROGRESS", "TAB", "TABS", "MENU", "MENUITEM", "TOOLBAR",
        "STATUSBAR", "TREEVIEW", "TABLE", "CANVAS", "SPLITTER",
        "SCROLLBAR", "TOOLTIP", "DIALOG", "CALENDAR",
        "WHEN", "CLICKED", "CHANGED", "STARTED", "CHECKED", "CLOSED",
        "RESIZED", "KEYDOWN", "KEYUP", "KEYPRESS", "MOUSEMOVE",
        "MOUSEDOWN", "MOUSEUP", "DOUBLECLICK", "RIGHTCLICK", "FOCUS",
        "BLUR", "SCROLL", "DRAG", "DROP", "TIMER", "TICK",
        "SELECTED", "HOVER",
        "SHOW", "HIDE", "CLOSE", "OPEN", "MINIMIZE", "MAXIMIZE",
        "RESTORE", "SET", "GET", "ADD", "REMOVE", "CLEAR", "INSERT",
        "UPDATE", "ENABLE", "DISABLE", "FOCUS_ACTION", "SELECT",
        "DESELECT", "PLAY", "PAUSE", "STOP", "RESUME", "SEEK",
        "LOAD", "START", "INTERVAL", "TIMEOUT",
        "IF", "ELSE", "ELSEIF", "WHILE", "FOR", "FROM", "TO",
        "STEP", "BREAK", "CONTINUE", "FUNC", "RETURN", "IMPORT",
        "EXPORT", "CLASS", "NEW", "THIS", "EXTENDS", "SWITCH",
        "CASE", "DEFAULT", "IN",
        "VAR", "CONST", "LET", "GLOBAL", "TRUE", "FALSE", "NULL",
        "AND", "OR", "NOT", "IS", "AS",
        "PRINT", "ALERT", "CONFIRM", "PROMPT", "LOG", "DEBUG",
        "READ", "WRITE", "APPEND", "DELETE", "EXISTS", "COPY",
        "MOVE", "MKDIR", "LISTDIR", "HTTP", "FETCH", "REQUEST",
        "RESPONSE", "WAIT", "ASYNC", "AWAIT", "PARALLEL",
        "TRY", "CATCH", "THROW", "FINALLY",
        "JSON", "PARSE", "STRINGIFY", "ENCODE", "DECODE",
        "REGEX", "MATCH", "TEST", "SEARCH",
        "RUN", "EXEC", "SHELL", "EXIT", "ENV", "CLIPBOARD",
        "NOTIFY", "BEEP",
        "NUMBER", "STRING", "IDENTIFIER",
        "PLUS", "MINUS", "MULTIPLY", "DIVIDE", "MODULO", "POWER",
        "EQUAL", "NOT_EQUAL", "GREATER", "LESS", "GREATER_EQ", "LESS_EQ",
        "ASSIGN", "PLUS_ASSIGN", "MINUS_ASSIGN", "MUL_ASSIGN", "DIV_ASSIGN",
        "INCREMENT", "DECREMENT",
        "LPAREN", "RPAREN", "LBRACE", "RBRACE", "LBRACKET", "RBRACKET",
        "COMMA", "DOT", "COLON", "SEMICOLON", "ARROW", "QUESTION",
        "NEWLINE", "EOF", "UNKNOWN"
    };
    if (type >= 0 && type <= HCS_TOK_UNKNOWN) {
        return names[type];
    }
    return "UNKNOWN";
}
