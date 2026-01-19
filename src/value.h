/*
 * HalcyonScript - Value types
 */

#ifndef VALUE_H
#define VALUE_H

#include "../include/halcyon.h"

typedef struct HcsValue HcsValue;
typedef struct HcsValueArray HcsValueArray;
typedef struct HcsValueObject HcsValueObject;

struct HcsValueArray {
    HcsValue** items;
    int count;
    int capacity;
};

struct HcsValueObject {
    char** keys;
    HcsValue** values;
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
        HcsValueObject object;
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

/* Object functions */
HcsValue* value_object(void);
void value_object_set(HcsValue* obj, const char* key, HcsValue* value);
HcsValue* value_object_get(HcsValue* obj, const char* key);
bool value_object_has(HcsValue* obj, const char* key);
void value_object_delete(HcsValue* obj, const char* key);
char** value_object_keys(HcsValue* obj, int* count);
int value_object_size(HcsValue* obj);

/* String utility functions */
HcsValue* value_string_concat(HcsValue* a, HcsValue* b);
HcsValue* value_string_substring(HcsValue* str, int start, int end);
HcsValue* value_string_split(HcsValue* str, const char* delimiter);
HcsValue* value_string_trim(HcsValue* str);
HcsValue* value_string_upper(HcsValue* str);
HcsValue* value_string_lower(HcsValue* str);
HcsValue* value_string_replace(HcsValue* str, const char* find, const char* replace);
int value_string_indexof(HcsValue* str, const char* search);
bool value_string_startswith(HcsValue* str, const char* prefix);
bool value_string_endswith(HcsValue* str, const char* suffix);
bool value_string_contains(HcsValue* str, const char* search);

/* Array utility functions */
HcsValue* value_array_join(HcsValue* arr, const char* separator);
HcsValue* value_array_slice(HcsValue* arr, int start, int end);
HcsValue* value_array_reverse(HcsValue* arr);
HcsValue* value_array_find(HcsValue* arr, HcsValue* value);
int value_array_indexof(HcsValue* arr, HcsValue* value);
void value_array_remove(HcsValue* arr, int index);
void value_array_insert(HcsValue* arr, int index, HcsValue* value);
void value_array_clear(HcsValue* arr);

#endif
