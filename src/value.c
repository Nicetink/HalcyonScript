/*
 * HalcyonScript © KAInaps 2026 
   Simple programming for creative minds 
   github.com/Nicetink/HalcyonScript
 */

#include "value.h"
#include <stdio.h>
#include <stdint.h>
#include <limits.h>

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
        case HCS_VAL_OBJECT: {
            HcsValue* obj = value_object();
            for (int i = 0; i < v->data.object.count; i++)
                value_object_set(obj, v->data.object.keys[i], value_copy(v->data.object.values[i]));
            return obj;
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
    else if (v->type == HCS_VAL_OBJECT) {
        for (int i = 0; i < v->data.object.count; i++) {
            free(v->data.object.keys[i]);
            value_release(v->data.object.values[i]);
        }
        free(v->data.object.keys);
        free(v->data.object.values);
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
        case HCS_VAL_ARRAY: {
            // Convert array to string representation: [1, 2, 3]
            if (v->data.array.count == 0) return strdup("[]");
            
            size_t total_len = 2; // for [ and ]
            char** item_strs = malloc(sizeof(char*) * v->data.array.count);
            if (!item_strs) return strdup("[]");
            
            // Check for integer overflow in total_len calculation
            for (int i = 0; i < v->data.array.count; i++) {
                item_strs[i] = value_to_string(v->data.array.items[i]);
                if (!item_strs[i]) {
                    // Cleanup on error
                    for (int j = 0; j < i; j++) free(item_strs[j]);
                    free(item_strs);
                    return strdup("[]");
                }
                
                size_t item_len = strlen(item_strs[i]);
                // Check for overflow
                if (total_len > SIZE_MAX - item_len - 2) {
                    // Cleanup on overflow
                    for (int j = 0; j <= i; j++) free(item_strs[j]);
                    free(item_strs);
                    return strdup("[...]"); // Truncated representation
                }
                
                total_len += item_len;
                if (i < v->data.array.count - 1) total_len += 2; // for ", "
            }
            
            char* result = malloc(total_len + 1);
            if (!result) {
                for (int i = 0; i < v->data.array.count; i++) free(item_strs[i]);
                free(item_strs);
                return strdup("[]");
            }
            
            strncpy(result, "[", total_len);
            result[1] = '\0'; // Ensure null termination
            
            for (int i = 0; i < v->data.array.count; i++) {
                strncat(result, item_strs[i], total_len - strlen(result) - 1);
                if (i < v->data.array.count - 1) {
                    strncat(result, ", ", total_len - strlen(result) - 1);
                }
                free(item_strs[i]);
            }
            strncat(result, "]", total_len - strlen(result) - 1);
            free(item_strs);
            return result;
        }
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

HcsValue* value_object(void) {
    HcsValue* v = (HcsValue*)calloc(1, sizeof(HcsValue));
    v->type = HCS_VAL_OBJECT;
    v->ref_count = 1;
    return v;
}

void value_object_set(HcsValue* obj, const char* key, HcsValue* value) {
    if (!obj || obj->type != HCS_VAL_OBJECT || !key) return;
    for (int i = 0; i < obj->data.object.count; i++) {
        if (strcmp(obj->data.object.keys[i], key) == 0) {
            value_release(obj->data.object.values[i]);
            obj->data.object.values[i] = value;
            return;
        }
    }
    if (obj->data.object.count >= obj->data.object.capacity) {
        int nc = obj->data.object.capacity == 0 ? 8 : obj->data.object.capacity * 2;
        obj->data.object.keys = (char**)realloc(obj->data.object.keys, sizeof(char*) * nc);
        obj->data.object.values = (HcsValue**)realloc(obj->data.object.values, sizeof(HcsValue*) * nc);
        obj->data.object.capacity = nc;
    }
    obj->data.object.keys[obj->data.object.count] = strdup(key);
    obj->data.object.values[obj->data.object.count] = value;
    obj->data.object.count++;
}

HcsValue* value_object_get(HcsValue* obj, const char* key) {
    if (!obj || obj->type != HCS_VAL_OBJECT || !key) return value_null();
    for (int i = 0; i < obj->data.object.count; i++) {
        if (strcmp(obj->data.object.keys[i], key) == 0) {
            value_retain(obj->data.object.values[i]);
            return obj->data.object.values[i];
        }
    }
    return value_null();
}

bool value_object_has(HcsValue* obj, const char* key) {
    if (!obj || obj->type != HCS_VAL_OBJECT || !key) return false;
    for (int i = 0; i < obj->data.object.count; i++) {
        if (strcmp(obj->data.object.keys[i], key) == 0) return true;
    }
    return false;
}

void value_object_delete(HcsValue* obj, const char* key) {
    if (!obj || obj->type != HCS_VAL_OBJECT || !key) return;
    for (int i = 0; i < obj->data.object.count; i++) {
        if (strcmp(obj->data.object.keys[i], key) == 0) {
            free(obj->data.object.keys[i]);
            value_release(obj->data.object.values[i]);
            for (int j = i; j < obj->data.object.count - 1; j++) {
                obj->data.object.keys[j] = obj->data.object.keys[j + 1];
                obj->data.object.values[j] = obj->data.object.values[j + 1];
            }
            obj->data.object.count--;
            return;
        }
    }
}

char** value_object_keys(HcsValue* obj, int* count) {
    if (!obj || obj->type != HCS_VAL_OBJECT) {
        if (count) *count = 0;
        return NULL;
    }
    if (count) *count = obj->data.object.count;
    return obj->data.object.keys;
}

int value_object_size(HcsValue* obj) {
    return (obj && obj->type == HCS_VAL_OBJECT) ? obj->data.object.count : 0;
}

HcsValue* value_string_concat(HcsValue* a, HcsValue* b) {
    char* sa = value_to_string(a);
    char* sb = value_to_string(b);
    char* result = (char*)malloc(strlen(sa) + strlen(sb) + 1);
    strcpy(result, sa);
    strcat(result, sb);
    HcsValue* v = value_string(result);
    free(sa); free(sb); free(result);
    return v;
}

HcsValue* value_string_substring(HcsValue* str, int start, int end) {
    if (!str || str->type != HCS_VAL_STRING) return value_string("");
    int len = (int)strlen(str->data.string);
    if (start < 0) start = 0;
    if (end < 0 || end > len) end = len;
    if (start >= end) return value_string("");
    int sublen = end - start;
    char* result = (char*)malloc(sublen + 1);
    strncpy(result, str->data.string + start, sublen);
    result[sublen] = '\0';
    HcsValue* v = value_string(result);
    free(result);
    return v;
}

HcsValue* value_string_split(HcsValue* str, const char* delimiter) {
    HcsValue* arr = value_array();
    if (!str || str->type != HCS_VAL_STRING || !delimiter) return arr;
    char* copy = strdup(str->data.string);
    char* token = strtok(copy, delimiter);
    while (token) {
        value_array_push(arr, value_string(token));
        token = strtok(NULL, delimiter);
    }
    free(copy);
    return arr;
}

HcsValue* value_string_trim(HcsValue* str) {
    if (!str || str->type != HCS_VAL_STRING) return value_string("");
    char* s = str->data.string;
    while (*s && (*s == ' ' || *s == '\t' || *s == '\n' || *s == '\r')) s++;
    int len = (int)strlen(s);
    while (len > 0 && (s[len-1] == ' ' || s[len-1] == '\t' || s[len-1] == '\n' || s[len-1] == '\r')) len--;
    char* result = (char*)malloc(len + 1);
    strncpy(result, s, len);
    result[len] = '\0';
    HcsValue* v = value_string(result);
    free(result);
    return v;
}

HcsValue* value_string_upper(HcsValue* str) {
    if (!str || str->type != HCS_VAL_STRING) return value_string("");
    char* result = strdup(str->data.string);
    for (int i = 0; result[i]; i++) {
        if (result[i] >= 'a' && result[i] <= 'z') result[i] -= 32;
    }
    HcsValue* v = value_string(result);
    free(result);
    return v;
}

HcsValue* value_string_lower(HcsValue* str) {
    if (!str || str->type != HCS_VAL_STRING) return value_string("");
    char* result = strdup(str->data.string);
    for (int i = 0; result[i]; i++) {
        if (result[i] >= 'A' && result[i] <= 'Z') result[i] += 32;
    }
    HcsValue* v = value_string(result);
    free(result);
    return v;
}

HcsValue* value_string_replace(HcsValue* str, const char* find, const char* replace) {
    if (!str || str->type != HCS_VAL_STRING || !find || !replace) return value_copy(str);
    char* s = str->data.string;
    int findLen = (int)strlen(find);
    int replaceLen = (int)strlen(replace);
    int count = 0;
    char* p = s;
    while ((p = strstr(p, find)) != NULL) { count++; p += findLen; }
    if (count == 0) return value_copy(str);
    int newLen = (int)strlen(s) + count * (replaceLen - findLen);
    char* result = (char*)malloc(newLen + 1);
    char* dst = result;
    p = s;
    while (*p) {
        if (strncmp(p, find, findLen) == 0) {
            strcpy(dst, replace);
            dst += replaceLen;
            p += findLen;
        } else {
            *dst++ = *p++;
        }
    }
    *dst = '\0';
    HcsValue* v = value_string(result);
    free(result);
    return v;
}

int value_string_indexof(HcsValue* str, const char* search) {
    if (!str || str->type != HCS_VAL_STRING || !search) return -1;
    char* p = strstr(str->data.string, search);
    return p ? (int)(p - str->data.string) : -1;
}

bool value_string_startswith(HcsValue* str, const char* prefix) {
    if (!str || str->type != HCS_VAL_STRING || !prefix) return false;
    return strncmp(str->data.string, prefix, strlen(prefix)) == 0;
}

bool value_string_endswith(HcsValue* str, const char* suffix) {
    if (!str || str->type != HCS_VAL_STRING || !suffix) return false;
    int slen = (int)strlen(str->data.string);
    int suflen = (int)strlen(suffix);
    if (suflen > slen) return false;
    return strcmp(str->data.string + slen - suflen, suffix) == 0;
}

bool value_string_contains(HcsValue* str, const char* search) {
    return value_string_indexof(str, search) >= 0;
}

HcsValue* value_array_join(HcsValue* arr, const char* separator) {
    if (!arr || arr->type != HCS_VAL_ARRAY) return value_string("");
    if (arr->data.array.count == 0) return value_string("");
    int sepLen = separator ? (int)strlen(separator) : 0;
    int totalLen = 0;
    char** strs = (char**)malloc(arr->data.array.count * sizeof(char*));
    for (int i = 0; i < arr->data.array.count; i++) {
        strs[i] = value_to_string(arr->data.array.items[i]);
        totalLen += (int)strlen(strs[i]);
        if (i > 0) totalLen += sepLen;
    }
    char* result = (char*)malloc(totalLen + 1);
    result[0] = '\0';
    for (int i = 0; i < arr->data.array.count; i++) {
        if (i > 0 && separator) strcat(result, separator);
        strcat(result, strs[i]);
        free(strs[i]);
    }
    free(strs);
    HcsValue* v = value_string(result);
    free(result);
    return v;
}

HcsValue* value_array_slice(HcsValue* arr, int start, int end) {
    HcsValue* result = value_array();
    if (!arr || arr->type != HCS_VAL_ARRAY) return result;
    int len = arr->data.array.count;
    if (start < 0) start = 0;
    if (end < 0 || end > len) end = len;
    for (int i = start; i < end; i++) {
        value_array_push(result, value_copy(arr->data.array.items[i]));
    }
    return result;
}

HcsValue* value_array_reverse(HcsValue* arr) {
    HcsValue* result = value_array();
    if (!arr || arr->type != HCS_VAL_ARRAY) return result;
    for (int i = arr->data.array.count - 1; i >= 0; i--) {
        value_array_push(result, value_copy(arr->data.array.items[i]));
    }
    return result;
}

int value_array_indexof(HcsValue* arr, HcsValue* value) {
    if (!arr || arr->type != HCS_VAL_ARRAY) return -1;
    for (int i = 0; i < arr->data.array.count; i++) {
        if (value_equals(arr->data.array.items[i], value)) return i;
    }
    return -1;
}

HcsValue* value_array_find(HcsValue* arr, HcsValue* value) {
    int idx = value_array_indexof(arr, value);
    if (idx >= 0) return value_copy(arr->data.array.items[idx]);
    return value_null();
}

void value_array_remove(HcsValue* arr, int index) {
    if (!arr || arr->type != HCS_VAL_ARRAY) return;
    if (index < 0 || index >= arr->data.array.count) return;
    value_release(arr->data.array.items[index]);
    for (int i = index; i < arr->data.array.count - 1; i++) {
        arr->data.array.items[i] = arr->data.array.items[i + 1];
    }
    arr->data.array.count--;
}

void value_array_insert(HcsValue* arr, int index, HcsValue* value) {
    if (!arr || arr->type != HCS_VAL_ARRAY) return;
    if (index < 0) index = 0;
    if (index > arr->data.array.count) index = arr->data.array.count;
    if (arr->data.array.count >= arr->data.array.capacity) {
        int nc = arr->data.array.capacity == 0 ? 8 : arr->data.array.capacity * 2;
        arr->data.array.items = (HcsValue**)realloc(arr->data.array.items, sizeof(HcsValue*) * nc);
        arr->data.array.capacity = nc;
    }
    for (int i = arr->data.array.count; i > index; i--) {
        arr->data.array.items[i] = arr->data.array.items[i - 1];
    }
    arr->data.array.items[index] = value;
    arr->data.array.count++;
}

void value_array_clear(HcsValue* arr) {
    if (!arr || arr->type != HCS_VAL_ARRAY) return;
    for (int i = 0; i < arr->data.array.count; i++) {
        value_release(arr->data.array.items[i]);
    }
    arr->data.array.count = 0;
}
