/*
 * HalcyonScript - AST implementation
 */

#include "ast.h"

HcsAstNode* ast_create(HcsAstNodeType type, int line) {
    HcsAstNode* node = (HcsAstNode*)calloc(1, sizeof(HcsAstNode));
    if (!node) return NULL;
    node->type = type;
    node->line = line;
    return node;
}

void ast_list_init(HcsAstList* list) {
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}

void ast_list_add(HcsAstList* list, HcsAstNode* node) {
    if (list->count >= list->capacity) {
        int new_cap = list->capacity == 0 ? 8 : list->capacity * 2;
        list->items = (HcsAstNode**)realloc(list->items, sizeof(HcsAstNode*) * new_cap);
        list->capacity = new_cap;
    }
    list->items[list->count++] = node;
}

void ast_list_free(HcsAstList* list) {
    for (int i = 0; i < list->count; i++) {
        ast_free(list->items[i]);
    }
    free(list->items);
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}

void prop_list_init(HcsPropertyList* list) {
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}

void prop_list_add(HcsPropertyList* list, const char* name, HcsAstNode* value) {
    if (list->count >= list->capacity) {
        int new_cap = list->capacity == 0 ? 8 : list->capacity * 2;
        list->items = (HcsProperty*)realloc(list->items, sizeof(HcsProperty) * new_cap);
        list->capacity = new_cap;
    }
    list->items[list->count].name = strdup(name);
    list->items[list->count].value = value;
    list->count++;
}

void prop_list_free(HcsPropertyList* list) {
    for (int i = 0; i < list->count; i++) {
        free(list->items[i].name);
        ast_free(list->items[i].value);
    }
    free(list->items);
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}

void ast_free(HcsAstNode* node) {
    if (!node) return;
    
    switch (node->type) {
        case HCS_AST_STRING:
            free(node->data.string_value);
            break;
            
        case HCS_AST_VAR_REF:
            free(node->data.var_ref.var_name);
            break;
            
        case HCS_AST_VAR_DECL:
            free(node->data.var_decl.name);
            ast_free(node->data.var_decl.init_value);
            break;
            
        case HCS_AST_ASSIGNMENT:
            free(node->data.assignment.var_name);
            free(node->data.assignment.op);
            ast_free(node->data.assignment.value);
            break;
            
        case HCS_AST_ARRAY_ASSIGNMENT:
            free(node->data.array_assign.array_name);
            ast_free(node->data.array_assign.index);
            ast_free(node->data.array_assign.value);
            break;
            
        case HCS_AST_BINARY_EXPR:
            ast_free(node->data.binary.left);
            free(node->data.binary.op);
            ast_free(node->data.binary.right);
            break;
            
        case HCS_AST_UNARY_EXPR:
            free(node->data.unary.op);
            ast_free(node->data.unary.operand);
            break;
            
        case HCS_AST_TERNARY_EXPR:
            ast_free(node->data.ternary.condition);
            ast_free(node->data.ternary.true_val);
            ast_free(node->data.ternary.false_val);
            break;
            
        case HCS_AST_ARRAY:
            ast_list_free(&node->data.array.elements);
            break;
            
        case HCS_AST_OBJECT:
            prop_list_free(&node->data.object.properties);
            break;
            
        case HCS_AST_ARRAY_ACCESS:
            ast_free(node->data.array_access.array);
            ast_free(node->data.array_access.index);
            break;
            
        case HCS_AST_PROPERTY_ACCESS:
            ast_free(node->data.prop_access.object);
            free(node->data.prop_access.property);
            break;
            
        case HCS_AST_FUNC_DECL:
            free(node->data.func_decl.name);
            for (int i = 0; i < node->data.func_decl.param_count; i++) {
                free(node->data.func_decl.params[i]);
            }
            free(node->data.func_decl.params);
            ast_list_free(&node->data.func_decl.body);
            break;
            
        case HCS_AST_FUNC_CALL:
        case HCS_AST_FUNC_CALL_EXPR:
            free(node->data.func_call.name);
            ast_list_free(&node->data.func_call.args);
            break;
            
        case HCS_AST_METHOD_CALL:
            ast_free(node->data.method_call.object);
            free(node->data.method_call.method);
            ast_list_free(&node->data.method_call.args);
            break;
            
        case HCS_AST_RETURN:
            ast_free(node->data.return_stmt.value);
            break;
            
        case HCS_AST_IF:
            ast_free(node->data.if_stmt.condition);
            ast_list_free(&node->data.if_stmt.then_body);
            ast_list_free(&node->data.if_stmt.else_body);
            break;
            
        case HCS_AST_WHILE:
            ast_free(node->data.while_loop.condition);
            ast_list_free(&node->data.while_loop.body);
            break;
            
        case HCS_AST_FOR:
            free(node->data.for_loop.var_name);
            ast_free(node->data.for_loop.from);
            ast_free(node->data.for_loop.to);
            ast_free(node->data.for_loop.step);
            ast_list_free(&node->data.for_loop.body);
            break;
            
        case HCS_AST_FOR_EACH:
            free(node->data.foreach_loop.var_name);
            ast_free(node->data.foreach_loop.collection);
            ast_list_free(&node->data.foreach_loop.body);
            break;
            
        case HCS_AST_CREATE_WINDOW:
            free(node->data.create_window.name);
            free(node->data.create_window.title);
            prop_list_free(&node->data.create_window.properties);
            break;
            
        case HCS_AST_CREATE_CONTROL:
            free(node->data.create_control.control_type);
            free(node->data.create_control.name);
            free(node->data.create_control.text);
            prop_list_free(&node->data.create_control.properties);
            break;
            
        case HCS_AST_CREATE_TIMER:
            free(node->data.create_timer.name);
            ast_free(node->data.create_timer.interval);
            break;
            
        case HCS_AST_EVENT_HANDLER:
            free(node->data.event_handler.element_name);
            free(node->data.event_handler.event_type);
            for (int i = 0; i < node->data.event_handler.param_count; i++) {
                free(node->data.event_handler.params[i]);
            }
            free(node->data.event_handler.params);
            ast_list_free(&node->data.event_handler.body);
            break;
            
        case HCS_AST_ALERT:
            ast_free(node->data.alert.message);
            ast_free(node->data.alert.title);
            break;
            
        case HCS_AST_PRINT:
            ast_list_free(&node->data.print.values);
            break;
            
        case HCS_AST_SET_PROPERTY:
            free(node->data.set_prop.element_name);
            free(node->data.set_prop.property);
            ast_free(node->data.set_prop.value);
            break;
            
        case HCS_AST_GET_PROPERTY:
            free(node->data.get_prop.element_name);
            free(node->data.get_prop.property);
            free(node->data.get_prop.result_var);
            break;
            
        case HCS_AST_FILE_OP:
            free(node->data.file_op.operation);
            ast_free(node->data.file_op.file_path);
            ast_free(node->data.file_op.content);
            ast_free(node->data.file_op.destination);
            free(node->data.file_op.result_var);
            break;
            
        case HCS_AST_WAIT:
            ast_free(node->data.wait.duration);
            break;
            
        case HCS_AST_SHELL:
            ast_free(node->data.shell.command);
            free(node->data.shell.result_var);
            break;
            
        case HCS_AST_IMPORT:
            free(node->data.import_stmt.path);
            free(node->data.import_stmt.alias);
            for (int i = 0; i < node->data.import_stmt.symbol_count; i++) {
                free(node->data.import_stmt.symbols[i]);
            }
            free(node->data.import_stmt.symbols);
            break;
            
        case HCS_AST_PROGRAM:
            ast_list_free(&node->data.program.statements);
            break;
            
        default:
            break;
    }
    
    free(node);
}
