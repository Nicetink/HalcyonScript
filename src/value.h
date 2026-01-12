/*
 * HalcyonScript - Value types
 */

#ifndef VALUE_H
#define VALUE_H

#include "../include/halcyon.h"

typedef struct HcsValue HcsValue;
typedef struct HcsValueArray HcsValueArray;

struct HcsValueArray {
    HcsValue** items;
    int count;
    int capacity;
};

struct HcsValue {
    HcsValueType type;
    union {
        double number;
        char* string;
        bool boolean;
        HcsValueArray array;
    } data;
    int ref_count;
};

HcsValue* value_null(void);
HcsValue* value_number(double n);
HcsValue* value_string(const char* s);
HcsValue* value_bool(bool b);
HcsValue* value_array(void);
HcsValue* value_copy(HcsValue* v);
void value_free(HcsValue* v);
void value_retain(HcsValue* v);
void value_release(HcsValue* v);
void value_array_push(HcsValue* arr, HcsValue* item);
HcsValue* value_array_get(HcsValue* arr, int index);
void value_array_set(HcsValue* arr, int index, HcsValue* item);
int value_array_length(HcsValue* arr);
double value_to_number(HcsValue* v);
char* value_to_string(HcsValue* v);
bool value_to_bool(HcsValue* v);
bool value_is_truthy(HcsValue* v);
bool value_equals(HcsValue* a, HcsValue* b);

#endif
