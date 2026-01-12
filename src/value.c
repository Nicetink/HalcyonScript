/*
 * HalcyonScript - Value implementation
 */

#include "value.h"
#include <stdio.h>

HcsValue* value_null(void) {
    HcsValue* v = (HcsValue*)calloc(1, sizeof(HcsValue));
    v->type = HCS_VAL_NULL; v->ref_count = 1;
    return v;
}

HcsValue* value_number(double n) {
    HcsValue* v = (HcsValue*)calloc(1, sizeof(HcsValue));
    v->type = HCS_VAL_NUMBER; v->data.number = n; v->ref_count = 1;
    return v;
}

HcsValue* value_string(const char* s) {
    HcsValue* v = (HcsValue*)calloc(1, sizeof(HcsValue));
    v->type = HCS_VAL_STRING; v->data.string = s ? strdup(s) : strdup(""); v->ref_count = 1;
    return v;
}

HcsValue* value_bool(bool b) {
    HcsValue* v = (HcsValue*)calloc(1, sizeof(HcsValue));
    v->type = HCS_VAL_BOOL; v->data.boolean = b; v->ref_count = 1;
    return v;
}

HcsValue* value_array(void) {
    HcsValue* v = (HcsValue*)calloc(1, sizeof(HcsValue));
    v->type = HCS_VAL_ARRAY; v->ref_count = 1;
    return v;
}

HcsValue* value_copy(HcsValue* v) {
    if (!v) return value_null();
    switch (v->type) {
        case HCS_VAL_NULL: return value_null();
        case HCS_VAL_NUMBER: return value_number(v->data.number);
        case HCS_VAL_STRING: return value_string(v->data.string);
        case HCS_VAL_BOOL: return value_bool(v->data.boolean);
        case HCS_VAL_ARRAY: {
            HcsValue* arr = value_array();
            for (int i = 0; i < v->data.array.count; i++)
                value_array_push(arr, value_copy(v->data.array.items[i]));
            return arr;
        }
        default: return value_null();
    }
}

void value_retain(HcsValue* v) { if (v) v->ref_count++; }
void value_release(HcsValue* v) { if (v && --v->ref_count <= 0) value_free(v); }

void value_free(HcsValue* v) {
    if (!v) return;
    if (v->type == HCS_VAL_STRING) free(v->data.string);
    else if (v->type == HCS_VAL_ARRAY) {
        for (int i = 0; i < v->data.array.count; i++) value_release(v->data.array.items[i]);
        free(v->data.array.items);
    }
    free(v);
}

void value_array_push(HcsValue* arr, HcsValue* item) {
    if (!arr || arr->type != HCS_VAL_ARRAY) return;
    if (arr->data.array.count >= arr->data.array.capacity) {
        int nc = arr->data.array.capacity == 0 ? 8 : arr->data.array.capacity * 2;
        arr->data.array.items = (HcsValue**)realloc(arr->data.array.items, sizeof(HcsValue*) * nc);
        arr->data.array.capacity = nc;
    }
    arr->data.array.items[arr->data.array.count++] = item;
}

HcsValue* value_array_get(HcsValue* arr, int i) {
    if (!arr || arr->type != HCS_VAL_ARRAY || i < 0 || i >= arr->data.array.count) return value_null();
    return arr->data.array.items[i];
}

void value_array_set(HcsValue* arr, int i, HcsValue* item) {
    if (!arr || arr->type != HCS_VAL_ARRAY || i < 0) return;
    while (i >= arr->data.array.count) value_array_push(arr, value_null());
    value_release(arr->data.array.items[i]);
    arr->data.array.items[i] = item;
}

int value_array_length(HcsValue* arr) {
    return (arr && arr->type == HCS_VAL_ARRAY) ? arr->data.array.count : 0;
}

double value_to_number(HcsValue* v) {
    if (!v) return 0;
    if (v->type == HCS_VAL_NUMBER) return v->data.number;
    if (v->type == HCS_VAL_STRING) return atof(v->data.string);
    if (v->type == HCS_VAL_BOOL) return v->data.boolean ? 1 : 0;
    return 0;
}

char* value_to_string(HcsValue* v) {
    if (!v) return strdup("null");
    char buf[64];
    switch (v->type) {
        case HCS_VAL_NULL: return strdup("null");
        case HCS_VAL_NUMBER:
            if (v->data.number == (int)v->data.number) sprintf(buf, "%d", (int)v->data.number);
            else sprintf(buf, "%g", v->data.number);
            return strdup(buf);
        case HCS_VAL_STRING: return strdup(v->data.string);
        case HCS_VAL_BOOL: return strdup(v->data.boolean ? "true" : "false");
        default: return strdup("undefined");
    }
}

bool value_to_bool(HcsValue* v) { return value_is_truthy(v); }

bool value_is_truthy(HcsValue* v) {
    if (!v) return false;
    switch (v->type) {
        case HCS_VAL_NULL: return false;
        case HCS_VAL_NUMBER: return v->data.number != 0;
        case HCS_VAL_STRING: return v->data.string && strlen(v->data.string) > 0;
        case HCS_VAL_BOOL: return v->data.boolean;
        case HCS_VAL_ARRAY: return v->data.array.count > 0;
        default: return false;
    }
}

bool value_equals(HcsValue* a, HcsValue* b) {
    if (!a && !b) return true;
    if (!a || !b) return false;
    if (a->type != b->type) return false;
    switch (a->type) {
        case HCS_VAL_NULL: return true;
        case HCS_VAL_NUMBER: return a->data.number == b->data.number;
        case HCS_VAL_STRING: return strcmp(a->data.string, b->data.string) == 0;
        case HCS_VAL_BOOL: return a->data.boolean == b->data.boolean;
        default: return a == b;
    }
}
