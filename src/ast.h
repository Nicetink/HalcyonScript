/*
 * HalcyonScript - AST definitions
 */

#ifndef AST_H
#define AST_H

#include "../include/halcyon.h"

/* Forward declarations */
typedef struct HcsAstNode HcsAstNode;
typedef struct HcsAstList HcsAstList;
typedef struct HcsProperty HcsProperty;
typedef struct HcsPropertyList HcsPropertyList;

/* Property for UI elements */
struct HcsProperty {
    char* name;
    HcsAstNode* value;
};

struct HcsPropertyList {
    HcsProperty* items;
    int count;
    int capacity;
};

/* List of AST nodes */
struct HcsAstList {
    HcsAstNode** items;
    int count;
    int capacity;
};

/* Main AST Node structure */
struct HcsAstNode {
    HcsAstNodeType type;
    int line;
    
    union {
        /* Literals */
        double number_value;
        char* string_value;
        bool bool_value;
        
        /* Variable reference */
        struct { char* var_name; } var_ref;
        
        /* Variable declaration */
        struct {
            char* name;
            HcsAstNode* init_value;
            bool is_const;
            bool is_global;
        } var_decl;
        
        /* Assignment */
        struct {
            char* var_name;
            char* op;
            HcsAstNode* value;
        } assignment;
        
        /* Array assignment */
        struct {
            char* array_name;
            HcsAstNode* index;
            HcsAstNode* value;
        } array_assign;
        
        /* Binary expression */
        struct {
            HcsAstNode* left;
            char* op;
            HcsAstNode* right;
        } binary;
        
        /* Unary expression */
        struct {
            char* op;
            HcsAstNode* operand;
            bool is_prefix;
        } unary;
        
        /* Ternary expression */
        struct {
            HcsAstNode* condition;
            HcsAstNode* true_val;
            HcsAstNode* false_val;
        } ternary;
        
        /* Array literal */
        struct { HcsAstList elements; } array;
        
        /* Object literal */
        struct { HcsPropertyList properties; } object;
        
        /* Array access */
        struct {
            HcsAstNode* array;
            HcsAstNode* index;
        } array_access;
        
        /* Property access */
        struct {
            HcsAstNode* object;
            char* property;
        } prop_access;
        
        /* Function declaration */
        struct {
            char* name;
            char** params;
            int param_count;
            HcsAstList body;
        } func_decl;
        
        /* Function call */
        struct {
            char* name;
            HcsAstList args;
        } func_call;
        
        /* Method call */
        struct {
            HcsAstNode* object;
            char* method;
            HcsAstList args;
        } method_call;
        
        /* Return statement */
        struct { HcsAstNode* value; } return_stmt;
        
        /* If statement */
        struct {
            HcsAstNode* condition;
            HcsAstList then_body;
            HcsAstList else_body;
        } if_stmt;
        
        /* While loop */
        struct {
            HcsAstNode* condition;
            HcsAstList body;
        } while_loop;
        
        /* For loop */
        struct {
            char* var_name;
            HcsAstNode* from;
            HcsAstNode* to;
            HcsAstNode* step;
            HcsAstList body;
        } for_loop;
        
        /* For-each loop */
        struct {
            char* var_name;
            HcsAstNode* collection;
            HcsAstList body;
        } foreach_loop;
        
        /* Create window */
        struct {
            char* name;
            char* title;
            int width;
            int height;
            HcsPropertyList properties;
        } create_window;
        
        /* Create control */
        struct {
            char* control_type;
            char* name;
            char* text;
            HcsPropertyList properties;
        } create_control;
        
        /* Create timer */
        struct {
            char* name;
            HcsAstNode* interval;
            bool auto_start;
        } create_timer;
        
        /* Event handler */
        struct {
            char* element_name;
            char* event_type;
            char** params;
            int param_count;
            HcsAstList body;
        } event_handler;
        
        /* Alert */
        struct {
            HcsAstNode* message;
            HcsAstNode* title;
        } alert;
        
        /* Print */
        struct { HcsAstList values; } print;
        
        /* Set property */
        struct {
            char* element_name;
            char* property;
            HcsAstNode* value;
        } set_prop;
        
        /* Get property */
        struct {
            char* element_name;
            char* property;
            char* result_var;
        } get_prop;
        
        /* Timer action */
        struct {
            char* action;
            char* timer_name;
        } timer_action;
        
        /* Window action */
        struct {
            char* action;
            char* window_name;
        } window_action;
        
        /* Control action */
        struct {
            char* action;
            char* control_name;
        } control_action;
        
        /* File operation */
        struct {
            char* operation;
            HcsAstNode* file_path;
            HcsAstNode* content;
            HcsAstNode* destination;
            char* result_var;
        } file_op;
        
        /* Wait */
        struct { HcsAstNode* duration; } wait;
        
        /* Shell */
        struct {
            HcsAstNode* command;
            char* result_var;
            bool do_wait;
        } shell;
        
        /* Import */
        struct {
            char* path;           /* File path to import */
            char* alias;          /* Optional alias (import "file" as name) */
            char** symbols;       /* Specific symbols to import (import { a, b } from "file") */
            int symbol_count;
            bool import_all;      /* import * from "file" */
        } import_stmt;
        
        /* Program (root) */
        struct { HcsAstList statements; } program;
    } data;
};

/* AST functions */
HcsAstNode* ast_create(HcsAstNodeType type, int line);
void ast_free(HcsAstNode* node);

/* HcsAstList functions */
void ast_list_init(HcsAstList* list);
void ast_list_add(HcsAstList* list, HcsAstNode* node);
void ast_list_free(HcsAstList* list);

/* HcsPropertyList functions */
void prop_list_init(HcsPropertyList* list);
void prop_list_add(HcsPropertyList* list, const char* name, HcsAstNode* value);
void prop_list_free(HcsPropertyList* list);

#endif /* AST_H */
