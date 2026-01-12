/*
 * HalcyonScript - Parser implementation
 */

#include "parser.h"
#include <string.h>

#define CURRENT() (p->pos < p->token_count ? p->tokens[p->pos] : NULL)
#define PEEK(n) (p->pos + n < p->token_count ? p->tokens[p->pos + n] : NULL)
#define ADVANCE() (p->pos++)
#define IS_END() (p->pos >= p->token_count || CURRENT()->type == HCS_TOK_EOF)
#define CHECK(t) (CURRENT() && CURRENT()->type == (t))
#define PREV() (p->pos > 0 ? p->tokens[p->pos - 1] : NULL)

static HcsAstNode* parse_statement(HcsParser* p);
static HcsAstNode* parse_expression(HcsParser* p);

static bool match(HcsParser* p, HcsTokenType type) {
    if (CHECK(type)) { ADVANCE(); return true; }
    return false;
}

static HcsToken* expect(HcsParser* p, HcsTokenType type, const char* msg) {
    if (CHECK(type)) { HcsToken* t = CURRENT(); ADVANCE(); return t; }
    fprintf(stderr, "Error line %d: Expected %s\n", CURRENT() ? CURRENT()->line : 0, msg);
    return NULL;
}

static char* expect_id(HcsParser* p, const char* ctx) {
    if (CHECK(HCS_TOK_IDENTIFIER)) { char* n = strdup(CURRENT()->value); ADVANCE(); return n; }
    fprintf(stderr, "Error line %d: Expected identifier for %s\n", CURRENT() ? CURRENT()->line : 0, ctx);
    return strdup("_err_");
}

static void skip_semi(HcsParser* p) { while (match(p, HCS_TOK_SEMICOLON) || match(p, HCS_TOK_NEWLINE)) {} }

HcsParser* parser_create(HcsToken** tokens, int token_count) {
    HcsParser* p = (HcsParser*)malloc(sizeof(HcsParser));
    HcsToken** filtered = (HcsToken**)malloc(sizeof(HcsToken*) * token_count);
    int fc = 0;
    for (int i = 0; i < token_count; i++) {
        if (tokens[i]->type != HCS_TOK_NEWLINE) filtered[fc++] = tokens[i];
    }
    p->tokens = filtered;
    p->token_count = fc;
    p->pos = 0;
    return p;
}

void parser_free(HcsParser* p) { free(p->tokens); free(p); }

/* Expression parsing */
static HcsAstNode* parse_primary(HcsParser* p) {
    int line = CURRENT() ? CURRENT()->line : 0;
    
    if (match(p, HCS_TOK_NUMBER)) {
        HcsAstNode* n = ast_create(HCS_AST_NUMBER, line);
        n->data.number_value = atof(PREV()->value);
        return n;
    }
    if (match(p, HCS_TOK_STRING)) {
        HcsAstNode* n = ast_create(HCS_AST_STRING, line);
        n->data.string_value = strdup(PREV()->value);
        return n;
    }
    if (match(p, HCS_TOK_TRUE)) { HcsAstNode* n = ast_create(HCS_AST_BOOL, line); n->data.bool_value = true; return n; }
    if (match(p, HCS_TOK_FALSE)) { HcsAstNode* n = ast_create(HCS_AST_BOOL, line); n->data.bool_value = false; return n; }
    if (match(p, HCS_TOK_NULL)) { return ast_create(HCS_AST_NULL, line); }
    
    if (match(p, HCS_TOK_LBRACKET)) {
        HcsAstNode* n = ast_create(HCS_AST_ARRAY, line);
        ast_list_init(&n->data.array.elements);
        while (!CHECK(HCS_TOK_RBRACKET) && !IS_END()) {
            ast_list_add(&n->data.array.elements, parse_expression(p));
            match(p, HCS_TOK_COMMA);
        }
        expect(p, HCS_TOK_RBRACKET, "]");
        return n;
    }
    
    if (match(p, HCS_TOK_LPAREN)) {
        HcsAstNode* e = parse_expression(p);
        expect(p, HCS_TOK_RPAREN, ")");
        return e;
    }
    
    if (match(p, HCS_TOK_IDENTIFIER)) {
        char* name = strdup(PREV()->value);
        if (match(p, HCS_TOK_LPAREN)) {
            HcsAstNode* n = ast_create(HCS_AST_FUNC_CALL_EXPR, line);
            n->data.func_call.name = name;
            ast_list_init(&n->data.func_call.args);
            while (!CHECK(HCS_TOK_RPAREN) && !IS_END()) {
                ast_list_add(&n->data.func_call.args, parse_expression(p));
                if (!CHECK(HCS_TOK_RPAREN)) expect(p, HCS_TOK_COMMA, ",");
            }
            expect(p, HCS_TOK_RPAREN, ")");
            return n;
        }
        HcsAstNode* n = ast_create(HCS_AST_VAR_REF, line);
        n->data.var_ref.var_name = name;
        return n;
    }
    
    ADVANCE();
    return ast_create(HCS_AST_NULL, line);
}

static HcsAstNode* parse_postfix(HcsParser* p) {
    HcsAstNode* e = parse_primary(p);
    while (true) {
        if (match(p, HCS_TOK_LBRACKET)) {
            HcsAstNode* n = ast_create(HCS_AST_ARRAY_ACCESS, e->line);
            n->data.array_access.array = e;
            n->data.array_access.index = parse_expression(p);
            expect(p, HCS_TOK_RBRACKET, "]");
            e = n;
        } else if (match(p, HCS_TOK_DOT)) {
            /* Property/method name can be a keyword like 'read', 'write', 'text', 'create', 'open', etc. */
            char* prop = NULL;
            // Accept ANY token with a value as property/method name (including keywords)
            if (CURRENT() && CURRENT()->value) {
                prop = strdup(CURRENT()->value);
                ADVANCE();
            } else {
                prop = strdup("_err_");
            }
            if (match(p, HCS_TOK_LPAREN)) {
                HcsAstNode* n = ast_create(HCS_AST_METHOD_CALL, e->line);
                n->data.method_call.object = e;
                n->data.method_call.method = prop;
                ast_list_init(&n->data.method_call.args);
                while (!CHECK(HCS_TOK_RPAREN) && !IS_END()) {
                    ast_list_add(&n->data.method_call.args, parse_expression(p));
                    if (!CHECK(HCS_TOK_RPAREN)) expect(p, HCS_TOK_COMMA, ",");
                }
                expect(p, HCS_TOK_RPAREN, ")");
                e = n;
            } else {
                HcsAstNode* n = ast_create(HCS_AST_PROPERTY_ACCESS, e->line);
                n->data.prop_access.object = e;
                n->data.prop_access.property = prop;
                e = n;
            }
        } else break;
    }
    return e;
}

static HcsAstNode* parse_unary(HcsParser* p) {
    if (match(p, HCS_TOK_NOT) || match(p, HCS_TOK_MINUS)) {
        HcsAstNode* n = ast_create(HCS_AST_UNARY_EXPR, PREV()->line);
        n->data.unary.op = strdup(PREV()->value);
        n->data.unary.operand = parse_unary(p);
        n->data.unary.is_prefix = true;
        return n;
    }
    return parse_postfix(p);
}

static HcsAstNode* parse_mult(HcsParser* p) {
    HcsAstNode* l = parse_unary(p);
    while (match(p, HCS_TOK_MULTIPLY) || match(p, HCS_TOK_DIVIDE) || match(p, HCS_TOK_MODULO)) {
        HcsAstNode* n = ast_create(HCS_AST_BINARY_EXPR, PREV()->line);
        n->data.binary.left = l;
        n->data.binary.op = strdup(PREV()->value);
        n->data.binary.right = parse_unary(p);
        l = n;
    }
    return l;
}

static HcsAstNode* parse_add(HcsParser* p) {
    HcsAstNode* l = parse_mult(p);
    while (match(p, HCS_TOK_PLUS) || match(p, HCS_TOK_MINUS)) {
        HcsAstNode* n = ast_create(HCS_AST_BINARY_EXPR, PREV()->line);
        n->data.binary.left = l;
        n->data.binary.op = strdup(PREV()->value);
        n->data.binary.right = parse_mult(p);
        l = n;
    }
    return l;
}

static HcsAstNode* parse_comp(HcsParser* p) {
    HcsAstNode* l = parse_add(p);
    while (match(p, HCS_TOK_GREATER) || match(p, HCS_TOK_LESS) || 
           match(p, HCS_TOK_GREATER_EQ) || match(p, HCS_TOK_LESS_EQ)) {
        HcsAstNode* n = ast_create(HCS_AST_BINARY_EXPR, PREV()->line);
        n->data.binary.left = l;
        n->data.binary.op = strdup(PREV()->value);
        n->data.binary.right = parse_add(p);
        l = n;
    }
    return l;
}

static HcsAstNode* parse_eq(HcsParser* p) {
    HcsAstNode* l = parse_comp(p);
    while (match(p, HCS_TOK_EQUAL) || match(p, HCS_TOK_NOT_EQUAL)) {
        HcsAstNode* n = ast_create(HCS_AST_BINARY_EXPR, PREV()->line);
        n->data.binary.left = l;
        n->data.binary.op = strdup(PREV()->value);
        n->data.binary.right = parse_comp(p);
        l = n;
    }
    return l;
}

static HcsAstNode* parse_and(HcsParser* p) {
    HcsAstNode* l = parse_eq(p);
    while (match(p, HCS_TOK_AND)) {
        HcsAstNode* n = ast_create(HCS_AST_BINARY_EXPR, PREV()->line);
        n->data.binary.left = l;
        n->data.binary.op = strdup("&&");
        n->data.binary.right = parse_eq(p);
        l = n;
    }
    return l;
}

static HcsAstNode* parse_or(HcsParser* p) {
    HcsAstNode* l = parse_and(p);
    while (match(p, HCS_TOK_OR)) {
        HcsAstNode* n = ast_create(HCS_AST_BINARY_EXPR, PREV()->line);
        n->data.binary.left = l;
        n->data.binary.op = strdup("||");
        n->data.binary.right = parse_and(p);
        l = n;
    }
    return l;
}

static HcsAstNode* parse_expression(HcsParser* p) { return parse_or(p); }

static HcsAstList parse_block(HcsParser* p) {
    HcsAstList list; ast_list_init(&list);
    while (!CHECK(HCS_TOK_RBRACE) && !IS_END()) {
        HcsAstNode* s = parse_statement(p);
        if (s) ast_list_add(&list, s);
    }
    expect(p, HCS_TOK_RBRACE, "}");
    return list;
}

static void parse_props(HcsParser* p, HcsPropertyList* props) {
    while (!CHECK(HCS_TOK_RBRACE) && !IS_END()) {
        char* name = expect_id(p, "prop");
        expect(p, HCS_TOK_COLON, ":");
        prop_list_add(props, name, parse_expression(p));
        free(name);
        match(p, HCS_TOK_COMMA);
    }
    expect(p, HCS_TOK_RBRACE, "}");
}

static HcsAstNode* parse_create_window(HcsParser* p) {
    int line = CURRENT() ? CURRENT()->line : 0;
    HcsAstNode* n = ast_create(HCS_AST_CREATE_WINDOW, line);
    n->data.create_window.name = expect_id(p, "window name");
    n->data.create_window.title = NULL;
    n->data.create_window.width = 800;
    n->data.create_window.height = 600;
    prop_list_init(&n->data.create_window.properties);
    if (match(p, HCS_TOK_STRING)) n->data.create_window.title = strdup(PREV()->value);
    if (match(p, HCS_TOK_NUMBER)) { n->data.create_window.width = (int)atof(PREV()->value);
        if (match(p, HCS_TOK_NUMBER)) n->data.create_window.height = (int)atof(PREV()->value); }
    if (match(p, HCS_TOK_LBRACE)) parse_props(p, &n->data.create_window.properties);
    return n;
}

static HcsAstNode* parse_create_control(HcsParser* p, const char* type) {
    int line = CURRENT() ? CURRENT()->line : 0;
    HcsAstNode* n = ast_create(HCS_AST_CREATE_CONTROL, line);
    n->data.create_control.control_type = strdup(type);
    n->data.create_control.name = expect_id(p, "control name");
    n->data.create_control.text = NULL;
    prop_list_init(&n->data.create_control.properties);
    if (match(p, HCS_TOK_STRING)) n->data.create_control.text = strdup(PREV()->value);
    if (match(p, HCS_TOK_LBRACE)) {
        parse_props(p, &n->data.create_control.properties);
    } else {
        /* Support both x=10 and x:10 syntax for properties */
        while (CHECK(HCS_TOK_IDENTIFIER) && PEEK(1) && 
               (PEEK(1)->type == HCS_TOK_ASSIGN || PEEK(1)->type == HCS_TOK_COLON)) {
            char* pn = strdup(CURRENT()->value); ADVANCE(); ADVANCE();
            prop_list_add(&n->data.create_control.properties, pn, parse_expression(p));
            free(pn);
        }
    }
    return n;
}

static HcsAstNode* parse_create_timer(HcsParser* p) {
    int line = CURRENT() ? CURRENT()->line : 0;
    HcsAstNode* n = ast_create(HCS_AST_CREATE_TIMER, line);
    n->data.create_timer.name = expect_id(p, "timer name");
    n->data.create_timer.interval = parse_expression(p);
    n->data.create_timer.auto_start = false;
    if (CHECK(HCS_TOK_IDENTIFIER) && strcmp(CURRENT()->value, "autostart") == 0) {
        ADVANCE(); n->data.create_timer.auto_start = true;
    }
    return n;
}

static HcsAstNode* parse_create(HcsParser* p) {
    ADVANCE();
    HcsTokenType t = CURRENT()->type;
    ADVANCE();
    switch (t) {
        case HCS_TOK_WINDOW: return parse_create_window(p);
        case HCS_TOK_TIMER: return parse_create_timer(p);
        case HCS_TOK_BUTTON: return parse_create_control(p, "button");
        case HCS_TOK_LABEL: return parse_create_control(p, "label");
        case HCS_TOK_INPUT: return parse_create_control(p, "input");
        case HCS_TOK_TEXTAREA: return parse_create_control(p, "textarea");
        case HCS_TOK_CHECKBOX: return parse_create_control(p, "checkbox");
        case HCS_TOK_LISTBOX: return parse_create_control(p, "listbox");
        case HCS_TOK_DROPDOWN: return parse_create_control(p, "dropdown");
        case HCS_TOK_SLIDER: return parse_create_control(p, "slider");
        case HCS_TOK_PROGRESS: return parse_create_control(p, "progress");
        case HCS_TOK_PANEL: return parse_create_control(p, "panel");
        default: return parse_create_control(p, "unknown");
    }
}

static HcsAstNode* parse_event(HcsParser* p) {
    ADVANCE();
    int line = CURRENT() ? CURRENT()->line : 0;
    HcsAstNode* n = ast_create(HCS_AST_EVENT_HANDLER, line);
    n->data.event_handler.element_name = expect_id(p, "element");
    n->data.event_handler.event_type = strdup(CURRENT()->value);
    ADVANCE();
    n->data.event_handler.params = NULL;
    n->data.event_handler.param_count = 0;
    expect(p, HCS_TOK_LBRACE, "{");
    n->data.event_handler.body = parse_block(p);
    return n;
}

static HcsAstNode* parse_var(HcsParser* p, bool is_const, bool is_global) {
    ADVANCE();
    int line = CURRENT() ? CURRENT()->line : 0;
    HcsAstNode* n = ast_create(HCS_AST_VAR_DECL, line);
    n->data.var_decl.name = expect_id(p, "var name");
    n->data.var_decl.is_const = is_const;
    n->data.var_decl.is_global = is_global;
    n->data.var_decl.init_value = NULL;
    if (match(p, HCS_TOK_ASSIGN)) n->data.var_decl.init_value = parse_expression(p);
    return n;
}

static HcsAstNode* parse_func(HcsParser* p) {
    ADVANCE();
    int line = CURRENT() ? CURRENT()->line : 0;
    HcsAstNode* n = ast_create(HCS_AST_FUNC_DECL, line);
    n->data.func_decl.name = expect_id(p, "func name");
    expect(p, HCS_TOK_LPAREN, "(");
    char** params = (char**)malloc(sizeof(char*) * MAX_PARAMS);
    int cnt = 0;
    while (!CHECK(HCS_TOK_RPAREN) && !IS_END()) {
        params[cnt++] = expect_id(p, "param");
        if (!CHECK(HCS_TOK_RPAREN)) expect(p, HCS_TOK_COMMA, ",");
    }
    expect(p, HCS_TOK_RPAREN, ")");
    n->data.func_decl.params = params;
    n->data.func_decl.param_count = cnt;
    expect(p, HCS_TOK_LBRACE, "{");
    n->data.func_decl.body = parse_block(p);
    return n;
}

static HcsAstNode* parse_if(HcsParser* p) {
    ADVANCE();
    int line = CURRENT() ? CURRENT()->line : 0;
    HcsAstNode* n = ast_create(HCS_AST_IF, line);
    n->data.if_stmt.condition = parse_expression(p);
    expect(p, HCS_TOK_LBRACE, "{");
    n->data.if_stmt.then_body = parse_block(p);
    ast_list_init(&n->data.if_stmt.else_body);
    while (match(p, HCS_TOK_ELSEIF) || (CHECK(HCS_TOK_ELSE) && PEEK(1) && PEEK(1)->type == HCS_TOK_IF)) {
        if (PREV()->type == HCS_TOK_ELSE) ADVANCE();
        HcsAstNode* elif = ast_create(HCS_AST_IF, CURRENT()->line);
        elif->data.if_stmt.condition = parse_expression(p);
        expect(p, HCS_TOK_LBRACE, "{");
        elif->data.if_stmt.then_body = parse_block(p);
        ast_list_init(&elif->data.if_stmt.else_body);
        ast_list_add(&n->data.if_stmt.else_body, elif);
    }
    if (match(p, HCS_TOK_ELSE)) {
        expect(p, HCS_TOK_LBRACE, "{");
        HcsAstList eb = parse_block(p);
        for (int i = 0; i < eb.count; i++) ast_list_add(&n->data.if_stmt.else_body, eb.items[i]);
        free(eb.items);
    }
    return n;
}

static HcsAstNode* parse_while(HcsParser* p) {
    ADVANCE();
    int line = CURRENT() ? CURRENT()->line : 0;
    HcsAstNode* n = ast_create(HCS_AST_WHILE, line);
    n->data.while_loop.condition = parse_expression(p);
    expect(p, HCS_TOK_LBRACE, "{");
    n->data.while_loop.body = parse_block(p);
    return n;
}

static HcsAstNode* parse_for(HcsParser* p) {
    ADVANCE();
    int line = CURRENT() ? CURRENT()->line : 0;
    char* var = expect_id(p, "loop var");
    if (match(p, HCS_TOK_IN)) {
        HcsAstNode* n = ast_create(HCS_AST_FOR_EACH, line);
        n->data.foreach_loop.var_name = var;
        n->data.foreach_loop.collection = parse_expression(p);
        expect(p, HCS_TOK_LBRACE, "{");
        n->data.foreach_loop.body = parse_block(p);
        return n;
    }
    expect(p, HCS_TOK_FROM, "from");
    HcsAstNode* n = ast_create(HCS_AST_FOR, line);
    n->data.for_loop.var_name = var;
    n->data.for_loop.from = parse_expression(p);
    expect(p, HCS_TOK_TO, "to");
    n->data.for_loop.to = parse_expression(p);
    n->data.for_loop.step = NULL;
    if (match(p, HCS_TOK_STEP)) n->data.for_loop.step = parse_expression(p);
    expect(p, HCS_TOK_LBRACE, "{");
    n->data.for_loop.body = parse_block(p);
    return n;
}

static HcsAstNode* parse_alert(HcsParser* p) {
    ADVANCE();
    int line = CURRENT() ? CURRENT()->line : 0;
    HcsAstNode* n = ast_create(HCS_AST_ALERT, line);
    n->data.alert.message = parse_expression(p);
    n->data.alert.title = NULL;
    if (match(p, HCS_TOK_COMMA)) n->data.alert.title = parse_expression(p);
    return n;
}

static HcsAstNode* parse_print(HcsParser* p) {
    ADVANCE();
    int line = CURRENT() ? CURRENT()->line : 0;
    HcsAstNode* n = ast_create(HCS_AST_PRINT, line);
    ast_list_init(&n->data.print.values);
    ast_list_add(&n->data.print.values, parse_expression(p));
    while (match(p, HCS_TOK_COMMA)) ast_list_add(&n->data.print.values, parse_expression(p));
    return n;
}

static HcsAstNode* parse_set(HcsParser* p) {
    ADVANCE();
    int line = CURRENT() ? CURRENT()->line : 0;
    HcsAstNode* n = ast_create(HCS_AST_SET_PROPERTY, line);
    n->data.set_prop.element_name = expect_id(p, "element");
    expect(p, HCS_TOK_DOT, ".");
    /* Property name can be a keyword or identifier like 'text', 'value', 'visible', etc. */
    if (CURRENT() && CURRENT()->value) {
        n->data.set_prop.property = strdup(CURRENT()->value);
        ADVANCE();
    } else {
        fprintf(stderr, "Error line %d: Expected property name\n", line);
        n->data.set_prop.property = strdup("_err_");
    }
    expect(p, HCS_TOK_ASSIGN, "=");
    n->data.set_prop.value = parse_expression(p);
    return n;
}

static HcsAstNode* parse_get(HcsParser* p) {
    ADVANCE();
    int line = CURRENT() ? CURRENT()->line : 0;
    HcsAstNode* n = ast_create(HCS_AST_GET_PROPERTY, line);
    n->data.get_prop.element_name = expect_id(p, "element");
    expect(p, HCS_TOK_DOT, ".");
    /* Property name can be a keyword or identifier like 'text', 'value', 'checked', etc. */
    if (CURRENT() && CURRENT()->value) {
        n->data.get_prop.property = strdup(CURRENT()->value);
        ADVANCE();
    } else {
        fprintf(stderr, "Error line %d: Expected property name\n", line);
        n->data.get_prop.property = strdup("_err_");
    }
    expect(p, HCS_TOK_ARROW, "->");
    n->data.get_prop.result_var = expect_id(p, "result var");
    return n;
}

static HcsAstNode* parse_return(HcsParser* p) {
    ADVANCE();
    int line = CURRENT() ? CURRENT()->line : 0;
    HcsAstNode* n = ast_create(HCS_AST_RETURN, line);
    n->data.return_stmt.value = NULL;
    if (!CHECK(HCS_TOK_RBRACE) && !CHECK(HCS_TOK_SEMICOLON) && !IS_END())
        n->data.return_stmt.value = parse_expression(p);
    return n;
}

static HcsAstNode* parse_id_stmt(HcsParser* p) {
    char* name = strdup(CURRENT()->value);
    int line = CURRENT()->line;
    ADVANCE();
    if (match(p, HCS_TOK_ASSIGN) || match(p, HCS_TOK_PLUS_ASSIGN) || 
        match(p, HCS_TOK_MINUS_ASSIGN) || match(p, HCS_TOK_MUL_ASSIGN) || match(p, HCS_TOK_DIV_ASSIGN)) {
        HcsAstNode* n = ast_create(HCS_AST_ASSIGNMENT, line);
        n->data.assignment.var_name = name;
        n->data.assignment.op = strdup(PREV()->value);
        n->data.assignment.value = parse_expression(p);
        return n;
    }
    if (match(p, HCS_TOK_LBRACKET)) {
        HcsAstNode* idx = parse_expression(p);
        expect(p, HCS_TOK_RBRACKET, "]");
        expect(p, HCS_TOK_ASSIGN, "=");
        HcsAstNode* n = ast_create(HCS_AST_ARRAY_ASSIGNMENT, line);
        n->data.array_assign.array_name = name;
        n->data.array_assign.index = idx;
        n->data.array_assign.value = parse_expression(p);
        return n;
    }
    if (match(p, HCS_TOK_INCREMENT)) {
        HcsAstNode* n = ast_create(HCS_AST_ASSIGNMENT, line);
        n->data.assignment.var_name = name;
        n->data.assignment.op = strdup("+=");
        HcsAstNode* one = ast_create(HCS_AST_NUMBER, line);
        one->data.number_value = 1;
        n->data.assignment.value = one;
        return n;
    }
    if (match(p, HCS_TOK_DECREMENT)) {
        HcsAstNode* n = ast_create(HCS_AST_ASSIGNMENT, line);
        n->data.assignment.var_name = name;
        n->data.assignment.op = strdup("-=");
        HcsAstNode* one = ast_create(HCS_AST_NUMBER, line);
        one->data.number_value = 1;
        n->data.assignment.value = one;
        return n;
    }
    if (match(p, HCS_TOK_LPAREN)) {
        HcsAstNode* n = ast_create(HCS_AST_FUNC_CALL, line);
        n->data.func_call.name = name;
        ast_list_init(&n->data.func_call.args);
        while (!CHECK(HCS_TOK_RPAREN) && !IS_END()) {
            ast_list_add(&n->data.func_call.args, parse_expression(p));
            if (!CHECK(HCS_TOK_RPAREN)) expect(p, HCS_TOK_COMMA, ",");
        }
        expect(p, HCS_TOK_RPAREN, ")");
        return n;
    }
    /* Handle method calls like HalGUI.init(), HalGUI.setTheme("dark"), HalGUI.run(), etc. */
    if (match(p, HCS_TOK_DOT)) {
        /* Method name can be a keyword like 'run', 'init', 'open', 'create', etc. */
        char* method = NULL;
        
        // Accept ANY token with a value as method name (including keywords)
        if (CURRENT() && CURRENT()->value) {
            method = strdup(CURRENT()->value);
            ADVANCE();
        } else {
            fprintf(stderr, "Error line %d: Expected method name\n", line);
            method = strdup("_err_");
        }
        if (match(p, HCS_TOK_LPAREN)) {
            /* Build full function name: "HalGUI.init" */
            char* full_name = malloc(strlen(name) + strlen(method) + 2);
            sprintf(full_name, "%s.%s", name, method);
            free(name);
            free(method);
            
            HcsAstNode* n = ast_create(HCS_AST_FUNC_CALL, line);
            n->data.func_call.name = full_name;
            ast_list_init(&n->data.func_call.args);
            while (!CHECK(HCS_TOK_RPAREN) && !IS_END()) {
                ast_list_add(&n->data.func_call.args, parse_expression(p));
                if (!CHECK(HCS_TOK_RPAREN)) expect(p, HCS_TOK_COMMA, ",");
            }
            expect(p, HCS_TOK_RPAREN, ")");
            return n;
        }
        free(method);
    }
    free(name);
    return NULL;
}

static HcsAstNode* parse_start(HcsParser* p) {
    ADVANCE();
    int line = CURRENT() ? CURRENT()->line : 0;
    HcsAstNode* n = ast_create(HCS_AST_TIMER_ACTION, line);
    n->data.timer_action.action = strdup("start");
    n->data.timer_action.timer_name = expect_id(p, "timer");
    return n;
}

static HcsAstNode* parse_stop(HcsParser* p) {
    ADVANCE();
    int line = CURRENT() ? CURRENT()->line : 0;
    HcsAstNode* n = ast_create(HCS_AST_TIMER_ACTION, line);
    n->data.timer_action.action = strdup("stop");
    n->data.timer_action.timer_name = expect_id(p, "timer");
    return n;
}

/*
 * Parse import statement
 * Syntax:
 *   import "file.hcs"
 *   import "file.hcs" as alias
 *   import { func1, func2 } from "file.hcs"
 *   import * from "file.hcs"
 */
static HcsAstNode* parse_import(HcsParser* p) {
    ADVANCE(); /* consume 'import' */
    int line = PREV()->line;
    
    HcsAstNode* n = ast_create(HCS_AST_IMPORT, line);
    n->data.import_stmt.path = NULL;
    n->data.import_stmt.alias = NULL;
    n->data.import_stmt.symbols = NULL;
    n->data.import_stmt.symbol_count = 0;
    n->data.import_stmt.import_all = false;
    
    /* Check for { symbols } or * */
    if (CHECK(HCS_TOK_LBRACE)) {
        /* import { a, b, c } from "file" */
        ADVANCE();
        char* symbols[64];
        int count = 0;
        
        while (!CHECK(HCS_TOK_RBRACE) && !IS_END() && count < 64) {
            if (CHECK(HCS_TOK_IDENTIFIER)) {
                symbols[count++] = strdup(CURRENT()->value);
                ADVANCE();
            }
            if (CHECK(HCS_TOK_COMMA)) ADVANCE();
        }
        expect(p, HCS_TOK_RBRACE, "}");
        
        /* Expect 'from' keyword */
        if (CHECK(HCS_TOK_FROM)) {
            ADVANCE();
        }
        
        n->data.import_stmt.symbols = malloc(sizeof(char*) * count);
        for (int i = 0; i < count; i++) {
            n->data.import_stmt.symbols[i] = symbols[i];
        }
        n->data.import_stmt.symbol_count = count;
    }
    else if (CHECK(HCS_TOK_MULTIPLY)) {
        /* import * from "file" */
        ADVANCE();
        n->data.import_stmt.import_all = true;
        
        if (CHECK(HCS_TOK_FROM)) {
            ADVANCE();
        }
    }
    
    /* Get file path */
    if (CHECK(HCS_TOK_STRING)) {
        n->data.import_stmt.path = strdup(CURRENT()->value);
        ADVANCE();
    } else {
        fprintf(stderr, "Error line %d: Expected file path in import\n", line);
        n->data.import_stmt.path = strdup("");
    }
    
    /* Check for 'as alias' */
    if (CHECK(HCS_TOK_AS)) {
        ADVANCE();
        if (CHECK(HCS_TOK_IDENTIFIER)) {
            n->data.import_stmt.alias = strdup(CURRENT()->value);
            ADVANCE();
        }
    }
    
    return n;
}

static HcsAstNode* parse_statement(HcsParser* p) {
    skip_semi(p);
    if (IS_END()) return NULL;
    
    switch (CURRENT()->type) {
        case HCS_TOK_CREATE: return parse_create(p);
        case HCS_TOK_WHEN: return parse_event(p);
        case HCS_TOK_IMPORT: return parse_import(p);
        case HCS_TOK_VAR:
        case HCS_TOK_LET: return parse_var(p, false, false);
        case HCS_TOK_CONST: return parse_var(p, true, false);
        case HCS_TOK_GLOBAL: {
            // Support both "global var x" and "global x" syntax
            ADVANCE();
            if (CHECK(HCS_TOK_VAR) || CHECK(HCS_TOK_LET)) {
                ADVANCE();
            }
            // Now parse as a var declaration
            int line = CURRENT() ? CURRENT()->line : 0;
            HcsAstNode* n = ast_create(HCS_AST_VAR_DECL, line);
            n->data.var_decl.name = expect_id(p, "var name");
            n->data.var_decl.is_const = false;
            n->data.var_decl.is_global = true;
            n->data.var_decl.init_value = NULL;
            if (match(p, HCS_TOK_ASSIGN)) n->data.var_decl.init_value = parse_expression(p);
            return n;
        }
        case HCS_TOK_FUNC: return parse_func(p);
        case HCS_TOK_IF: return parse_if(p);
        case HCS_TOK_WHILE: return parse_while(p);
        case HCS_TOK_FOR: return parse_for(p);
        case HCS_TOK_ALERT: return parse_alert(p);
        case HCS_TOK_PRINT:
        case HCS_TOK_LOG:
        case HCS_TOK_DEBUG: return parse_print(p);
        case HCS_TOK_SET: return parse_set(p);
        case HCS_TOK_GET: return parse_get(p);
        case HCS_TOK_RETURN: return parse_return(p);
        case HCS_TOK_BREAK: ADVANCE(); return ast_create(HCS_AST_BREAK, PREV()->line);
        case HCS_TOK_CONTINUE: ADVANCE(); return ast_create(HCS_AST_CONTINUE, PREV()->line);
        case HCS_TOK_START: return parse_start(p);
        case HCS_TOK_STOP: return parse_stop(p);
        case HCS_TOK_IDENTIFIER: return parse_id_stmt(p);
        default: ADVANCE(); return NULL;
    }
}

HcsAstNode* parser_parse(HcsParser* p) {
    HcsAstNode* prog = ast_create(HCS_AST_PROGRAM, 0);
    ast_list_init(&prog->data.program.statements);
    while (!IS_END()) {
        HcsAstNode* s = parse_statement(p);
        if (s) ast_list_add(&prog->data.program.statements, s);
    }
    return prog;
}
