/*
 * HalcyonScript - Lexer implementation
 */

#include "lexer.h"
#include <string.h>
#include <ctype.h>

/* Keyword lookup table */
typedef struct {
    const char* keyword;
    HcsTokenType type;
} KeywordEntry;

static KeywordEntry keywords[] = {
    /* UI Controls */
    {"create", HCS_TOK_CREATE}, {"window", HCS_TOK_WINDOW}, {"button", HCS_TOK_BUTTON},
    {"label", HCS_TOK_LABEL}, {"input", HCS_TOK_INPUT}, {"listbox", HCS_TOK_LISTBOX},
    {"checkbox", HCS_TOK_CHECKBOX}, {"image", HCS_TOK_IMAGE}, {"panel", HCS_TOK_PANEL},
    {"textarea", HCS_TOK_TEXTAREA}, {"dropdown", HCS_TOK_DROPDOWN}, {"combobox", HCS_TOK_DROPDOWN},
    {"slider", HCS_TOK_SLIDER}, {"trackbar", HCS_TOK_SLIDER}, {"progress", HCS_TOK_PROGRESS},
    {"progressbar", HCS_TOK_PROGRESS}, {"tab", HCS_TOK_TAB}, {"tabs", HCS_TOK_TABS},
    {"tabcontrol", HCS_TOK_TABS}, {"menu", HCS_TOK_MENU}, {"menuitem", HCS_TOK_MENUITEM},
    {"toolbar", HCS_TOK_TOOLBAR}, {"statusbar", HCS_TOK_STATUSBAR}, {"treeview", HCS_TOK_TREEVIEW},
    {"tree", HCS_TOK_TREEVIEW}, {"table", HCS_TOK_TABLE}, {"grid", HCS_TOK_TABLE},
    {"canvas", HCS_TOK_CANVAS}, {"splitter", HCS_TOK_SPLITTER}, {"tooltip", HCS_TOK_TOOLTIP},
    {"dialog", HCS_TOK_DIALOG},
    
    /* Events */
    {"when", HCS_TOK_WHEN}, {"clicked", HCS_TOK_CLICKED}, {"click", HCS_TOK_CLICKED},
    {"changed", HCS_TOK_CHANGED}, {"change", HCS_TOK_CHANGED}, {"started", HCS_TOK_STARTED},
    {"checked", HCS_TOK_CHECKED}, {"closed", HCS_TOK_CLOSED}, {"resized", HCS_TOK_RESIZED},
    {"resize", HCS_TOK_RESIZED}, {"keydown", HCS_TOK_KEYDOWN}, {"keyup", HCS_TOK_KEYUP},
    {"keypress", HCS_TOK_KEYPRESS}, {"mousemove", HCS_TOK_MOUSEMOVE},
    {"mousedown", HCS_TOK_MOUSEDOWN}, {"mouseup", HCS_TOK_MOUSEUP},
    {"doubleclick", HCS_TOK_DOUBLECLICK}, {"dblclick", HCS_TOK_DOUBLECLICK},
    {"rightclick", HCS_TOK_RIGHTCLICK}, {"focus", HCS_TOK_FOCUS}, {"blur", HCS_TOK_BLUR},
    {"scroll", HCS_TOK_SCROLL}, {"drag", HCS_TOK_DRAG}, {"drop", HCS_TOK_DROP},
    {"timer", HCS_TOK_TIMER}, {"tick", HCS_TOK_TICK}, {"selected", HCS_TOK_SELECTED},
    {"hover", HCS_TOK_HOVER},
    
    /* Actions */
    {"show", HCS_TOK_SHOW}, {"hide", HCS_TOK_HIDE}, {"close", HCS_TOK_CLOSE},
    {"open", HCS_TOK_OPEN}, {"minimize", HCS_TOK_MINIMIZE}, {"maximize", HCS_TOK_MAXIMIZE},
    {"restore", HCS_TOK_RESTORE}, {"set", HCS_TOK_SET}, {"get", HCS_TOK_GET},
    {"add", HCS_TOK_ADD}, {"remove", HCS_TOK_REMOVE}, {"clear", HCS_TOK_CLEAR},
    {"insert", HCS_TOK_INSERT}, {"update", HCS_TOK_UPDATE}, {"enable", HCS_TOK_ENABLE},
    {"disable", HCS_TOK_DISABLE}, {"select", HCS_TOK_SELECT}, {"deselect", HCS_TOK_DESELECT},
    {"play", HCS_TOK_PLAY}, {"pause", HCS_TOK_PAUSE}, {"stop", HCS_TOK_STOP},
    {"resume", HCS_TOK_RESUME}, {"seek", HCS_TOK_SEEK}, {"load", HCS_TOK_LOAD},
    {"start", HCS_TOK_START}, {"interval", HCS_TOK_INTERVAL}, {"timeout", HCS_TOK_TIMEOUT},
    
    /* Control flow */
    {"if", HCS_TOK_IF}, {"else", HCS_TOK_ELSE}, {"elseif", HCS_TOK_ELSEIF},
    {"elif", HCS_TOK_ELSEIF}, {"while", HCS_TOK_WHILE}, {"for", HCS_TOK_FOR},
    {"from", HCS_TOK_FROM}, {"to", HCS_TOK_TO}, {"step", HCS_TOK_STEP},
    {"break", HCS_TOK_BREAK}, {"continue", HCS_TOK_CONTINUE}, {"func", HCS_TOK_FUNC},
    {"function", HCS_TOK_FUNC}, {"return", HCS_TOK_RETURN}, {"import", HCS_TOK_IMPORT},
    {"export", HCS_TOK_EXPORT}, {"class", HCS_TOK_CLASS}, {"new", HCS_TOK_NEW},
    {"this", HCS_TOK_THIS}, {"extends", HCS_TOK_EXTENDS}, {"switch", HCS_TOK_SWITCH},
    {"case", HCS_TOK_CASE}, {"default", HCS_TOK_DEFAULT}, {"in", HCS_TOK_IN},
    
    /* Variables */
    {"var", HCS_TOK_VAR}, {"const", HCS_TOK_CONST}, {"let", HCS_TOK_LET},
    {"global", HCS_TOK_GLOBAL}, {"true", HCS_TOK_TRUE}, {"false", HCS_TOK_FALSE},
    {"null", HCS_TOK_NULL}, {"none", HCS_TOK_NULL}, {"and", HCS_TOK_AND},
    {"or", HCS_TOK_OR}, {"not", HCS_TOK_NOT}, {"is", HCS_TOK_IS}, {"as", HCS_TOK_AS},
    
    /* I/O */
    {"print", HCS_TOK_PRINT}, {"log", HCS_TOK_LOG}, {"debug", HCS_TOK_DEBUG},
    {"alert", HCS_TOK_ALERT}, {"confirm", HCS_TOK_CONFIRM}, {"prompt", HCS_TOK_PROMPT},
    {"read", HCS_TOK_READ}, {"write", HCS_TOK_WRITE}, {"append", HCS_TOK_APPEND},
    {"delete", HCS_TOK_DELETE}, {"exists", HCS_TOK_EXISTS}, {"copy", HCS_TOK_COPY},
    {"move", HCS_TOK_MOVE}, {"mkdir", HCS_TOK_MKDIR}, {"listdir", HCS_TOK_LISTDIR},
    {"http", HCS_TOK_HTTP}, {"fetch", HCS_TOK_FETCH}, {"request", HCS_TOK_REQUEST},
    {"wait", HCS_TOK_WAIT}, {"async", HCS_TOK_ASYNC}, {"await", HCS_TOK_AWAIT},
    {"parallel", HCS_TOK_PARALLEL}, {"try", HCS_TOK_TRY}, {"catch", HCS_TOK_CATCH},
    {"throw", HCS_TOK_THROW}, {"finally", HCS_TOK_FINALLY},
    
    /* Data */
    {"json", HCS_TOK_JSON}, {"parse", HCS_TOK_PARSE}, {"stringify", HCS_TOK_STRINGIFY},
    {"encode", HCS_TOK_ENCODE}, {"decode", HCS_TOK_DECODE}, {"regex", HCS_TOK_REGEX},
    {"match", HCS_TOK_MATCH}, {"test", HCS_TOK_TEST}, {"search", HCS_TOK_SEARCH},
    
    /* System */
    {"run", HCS_TOK_RUN}, {"exec", HCS_TOK_EXEC}, {"shell", HCS_TOK_SHELL},
    {"exit", HCS_TOK_EXIT}, {"env", HCS_TOK_ENV}, {"clipboard", HCS_TOK_CLIPBOARD},
    {"notify", HCS_TOK_NOTIFY}, {"beep", HCS_TOK_BEEP},
    
    {NULL, HCS_TOK_UNKNOWN}
};

/* Helper functions */
static char lexer_current(HcsLexer* l) {
    return l->position < l->length ? l->source[l->position] : '\0';
}

static char lexer_peek(HcsLexer* l) {
    return l->position + 1 < l->length ? l->source[l->position + 1] : '\0';
}

static void lexer_advance(HcsLexer* l) {
    l->position++;
    l->column++;
}

static int str_icmp(const char* a, const char* b) {
    while (*a && *b) {
        int ca = tolower((unsigned char)*a);
        int cb = tolower((unsigned char)*b);
        if (ca != cb) return ca - cb;
        a++; b++;
    }
    return tolower((unsigned char)*a) - tolower((unsigned char)*b);
}

static HcsTokenType lookup_keyword(const char* word) {
    for (int i = 0; keywords[i].keyword != NULL; i++) {
        if (str_icmp(word, keywords[i].keyword) == 0) {
            return keywords[i].type;
        }
    }
    return HCS_TOK_IDENTIFIER;
}

HcsLexer* lexer_create(const char* source) {
    HcsLexer* lexer = (HcsLexer*)malloc(sizeof(HcsLexer));
    if (!lexer) return NULL;
    
    lexer->source = source;
    lexer->position = 0;
    lexer->line = 1;
    lexer->column = 1;
    lexer->length = strlen(source);
    return lexer;
}

void lexer_free(HcsLexer* lexer) {
    free(lexer);
}

static void skip_whitespace_and_comments(HcsLexer* l) {
    while (l->position < l->length) {
        char c = lexer_current(l);
        
        if (c == ' ' || c == '\t' || c == '\r') {
            lexer_advance(l);
            continue;
        }
        
        /* Single-line comment // or # */
        if ((c == '/' && lexer_peek(l) == '/') || c == '#') {
            while (l->position < l->length && lexer_current(l) != '\n') {
                lexer_advance(l);
            }
            continue;
        }
        
        /* Multi-line comment */
        if (c == '/' && lexer_peek(l) == '*') {
            lexer_advance(l); lexer_advance(l);
            while (l->position < l->length) {
                if (lexer_current(l) == '*' && lexer_peek(l) == '/') {
                    lexer_advance(l); lexer_advance(l);
                    break;
                }
                if (lexer_current(l) == '\n') {
                    l->line++;
                    l->column = 0;
                }
                lexer_advance(l);
            }
            continue;
        }
        
        break;
    }
}

static HcsToken* read_identifier(HcsLexer* l) {
    int start = l->position;
    int start_col = l->column;
    
    while (l->position < l->length && 
           (isalnum((unsigned char)lexer_current(l)) || lexer_current(l) == '_')) {
        lexer_advance(l);
    }
    
    int len = l->position - start;
    char* value = (char*)malloc(len + 1);
    strncpy(value, l->source + start, len);
    value[len] = '\0';
    
    HcsTokenType type = lookup_keyword(value);
    HcsToken* token = token_create(type, value, l->line, start_col);
    free(value);
    return token;
}

static HcsToken* read_number(HcsLexer* l) {
    int start = l->position;
    int start_col = l->column;
    
    while (l->position < l->length && 
           (isdigit((unsigned char)lexer_current(l)) || lexer_current(l) == '.')) {
        lexer_advance(l);
    }
    
    int len = l->position - start;
    char* value = (char*)malloc(len + 1);
    strncpy(value, l->source + start, len);
    value[len] = '\0';
    
    HcsToken* token = token_create(HCS_TOK_NUMBER, value, l->line, start_col);
    free(value);
    return token;
}

static HcsToken* read_string(HcsLexer* l, char quote) {
    int start_col = l->column;
    lexer_advance(l); /* Skip opening quote */
    
    char* buffer = (char*)malloc(MAX_STRING_LEN);
    int buf_pos = 0;
    
    while (l->position < l->length && lexer_current(l) != quote) {
        if (lexer_current(l) == '\\' && l->position + 1 < l->length) {
            lexer_advance(l);
            char escaped = lexer_current(l);
            switch (escaped) {
                case 'n': buffer[buf_pos++] = '\n'; break;
                case 't': buffer[buf_pos++] = '\t'; break;
                case 'r': buffer[buf_pos++] = '\r'; break;
                case '\\': buffer[buf_pos++] = '\\'; break;
                case '"': buffer[buf_pos++] = '"'; break;
                case '\'': buffer[buf_pos++] = '\''; break;
                default: buffer[buf_pos++] = escaped; break;
            }
        } else {
            buffer[buf_pos++] = lexer_current(l);
        }
        lexer_advance(l);
    }
    buffer[buf_pos] = '\0';
    
    if (l->position < l->length) {
        lexer_advance(l); /* Skip closing quote */
    }
    
    HcsToken* token = token_create(HCS_TOK_STRING, buffer, l->line, start_col);
    free(buffer);
    return token;
}

static HcsToken* read_operator(HcsLexer* l) {
    int start_col = l->column;
    char c = lexer_current(l);
    lexer_advance(l);
    
    switch (c) {
        case '+':
            if (lexer_current(l) == '+') { lexer_advance(l); return token_create(HCS_TOK_INCREMENT, "++", l->line, start_col); }
            if (lexer_current(l) == '=') { lexer_advance(l); return token_create(HCS_TOK_PLUS_ASSIGN, "+=", l->line, start_col); }
            return token_create(HCS_TOK_PLUS, "+", l->line, start_col);
        case '-':
            if (lexer_current(l) == '-') { lexer_advance(l); return token_create(HCS_TOK_DECREMENT, "--", l->line, start_col); }
            if (lexer_current(l) == '=') { lexer_advance(l); return token_create(HCS_TOK_MINUS_ASSIGN, "-=", l->line, start_col); }
            if (lexer_current(l) == '>') { lexer_advance(l); return token_create(HCS_TOK_ARROW, "->", l->line, start_col); }
            return token_create(HCS_TOK_MINUS, "-", l->line, start_col);
        case '*':
            if (lexer_current(l) == '*') { lexer_advance(l); return token_create(HCS_TOK_POWER, "**", l->line, start_col); }
            if (lexer_current(l) == '=') { lexer_advance(l); return token_create(HCS_TOK_MUL_ASSIGN, "*=", l->line, start_col); }
            return token_create(HCS_TOK_MULTIPLY, "*", l->line, start_col);
        case '/':
            if (lexer_current(l) == '=') { lexer_advance(l); return token_create(HCS_TOK_DIV_ASSIGN, "/=", l->line, start_col); }
            return token_create(HCS_TOK_DIVIDE, "/", l->line, start_col);
        case '%':
            return token_create(HCS_TOK_MODULO, "%", l->line, start_col);
        case '=':
            if (lexer_current(l) == '=') { lexer_advance(l); return token_create(HCS_TOK_EQUAL, "==", l->line, start_col); }
            if (lexer_current(l) == '>') { lexer_advance(l); return token_create(HCS_TOK_ARROW, "=>", l->line, start_col); }
            return token_create(HCS_TOK_ASSIGN, "=", l->line, start_col);
        case '!':
            if (lexer_current(l) == '=') { lexer_advance(l); return token_create(HCS_TOK_NOT_EQUAL, "!=", l->line, start_col); }
            return token_create(HCS_TOK_NOT, "!", l->line, start_col);
        case '>':
            if (lexer_current(l) == '=') { lexer_advance(l); return token_create(HCS_TOK_GREATER_EQ, ">=", l->line, start_col); }
            return token_create(HCS_TOK_GREATER, ">", l->line, start_col);
        case '<':
            if (lexer_current(l) == '=') { lexer_advance(l); return token_create(HCS_TOK_LESS_EQ, "<=", l->line, start_col); }
            return token_create(HCS_TOK_LESS, "<", l->line, start_col);
        case '&':
            if (lexer_current(l) == '&') { lexer_advance(l); return token_create(HCS_TOK_AND, "&&", l->line, start_col); }
            return token_create(HCS_TOK_UNKNOWN, "&", l->line, start_col);
        case '|':
            if (lexer_current(l) == '|') { lexer_advance(l); return token_create(HCS_TOK_OR, "||", l->line, start_col); }
            return token_create(HCS_TOK_UNKNOWN, "|", l->line, start_col);
        case '(': return token_create(HCS_TOK_LPAREN, "(", l->line, start_col);
        case ')': return token_create(HCS_TOK_RPAREN, ")", l->line, start_col);
        case '{': return token_create(HCS_TOK_LBRACE, "{", l->line, start_col);
        case '}': return token_create(HCS_TOK_RBRACE, "}", l->line, start_col);
        case '[': return token_create(HCS_TOK_LBRACKET, "[", l->line, start_col);
        case ']': return token_create(HCS_TOK_RBRACKET, "]", l->line, start_col);
        case ',': return token_create(HCS_TOK_COMMA, ",", l->line, start_col);
        case '.': return token_create(HCS_TOK_DOT, ".", l->line, start_col);
        case ':': return token_create(HCS_TOK_COLON, ":", l->line, start_col);
        case ';': return token_create(HCS_TOK_SEMICOLON, ";", l->line, start_col);
        case '?': return token_create(HCS_TOK_QUESTION, "?", l->line, start_col);
        default: {
            char buf[2] = {c, '\0'};
            return token_create(HCS_TOK_UNKNOWN, buf, l->line, start_col);
        }
    }
}

HcsToken** lexer_tokenize(HcsLexer* l, int* token_count) {
    HcsToken** tokens = (HcsToken**)malloc(sizeof(HcsToken*) * MAX_TOKENS);
    int count = 0;
    
    while (l->position < l->length) {
        skip_whitespace_and_comments(l);
        if (l->position >= l->length) break;
        
        char c = lexer_current(l);
        HcsToken* token = NULL;
        
        if (c == '\n') {
            token = token_create(HCS_TOK_NEWLINE, "\\n", l->line, l->column);
            lexer_advance(l);
            l->line++;
            l->column = 1;
        }
        else if (isalpha((unsigned char)c) || c == '_') {
            token = read_identifier(l);
        }
        else if (isdigit((unsigned char)c)) {
            token = read_number(l);
        }
        else if (c == '"' || c == '\'') {
            token = read_string(l, c);
        }
        else {
            token = read_operator(l);
        }
        
        if (token) {
            tokens[count++] = token;
            if (count >= MAX_TOKENS - 1) break;
        }
    }
    
    tokens[count++] = token_create(HCS_TOK_EOF, "", l->line, l->column);
    *token_count = count;
    return tokens;
}

void lexer_free_tokens(HcsToken** tokens, int count) {
    for (int i = 0; i < count; i++) {
        token_free(tokens[i]);
    }
    free(tokens);
}
