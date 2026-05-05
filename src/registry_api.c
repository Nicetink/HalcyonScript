/*
 * HalcyonScript © KAInaps 2026 
   Simple programming for creative minds 
   github.com/Nicetink/HalcyonScript
 */

#include "runtime.h"
#include <windows.h>

// Validate registry key path to prevent unauthorized access
static int validate_registry_path(const char* root, const char* key_path) {
    if (!root || !key_path) return 0;
    
    // Only allow HKCU for security
    if (strcmp(root, "HKCU") != 0) {
        return 0; // Block HKLM, HKCR, HKU access
    }
    
    // Whitelist allowed key paths under HKCU
    const char* allowed_paths[] = {
        "Software\\HalcyonScript\\",
        "Software\\MyApp\\",
        "Environment",
        NULL
    };
    
    for (int i = 0; allowed_paths[i]; i++) {
        if (strncmp(key_path, allowed_paths[i], strlen(allowed_paths[i])) == 0) {
            return 1;
        }
    }
    
    return 0; // Path not in whitelist
}

static HKEY get_root_key(const char* root) {
    if (strcmp(root, "HKLM") == 0) return HKEY_LOCAL_MACHINE;
    if (strcmp(root, "HKCU") == 0) return HKEY_CURRENT_USER;
    if (strcmp(root, "HKCR") == 0) return HKEY_CLASSES_ROOT;
    if (strcmp(root, "HKU") == 0) return HKEY_USERS;
    return HKEY_CURRENT_USER;
}

HcsValue* builtin_reg_write(int argc, HcsValue** args) {
    if (argc != 4) return value_bool(false);
    if (args[0]->type != HCS_VAL_STRING || args[1]->type != HCS_VAL_STRING || 
        args[2]->type != HCS_VAL_STRING) return value_bool(false);
    
    // Validate registry access
    if (!validate_registry_path(args[0]->data.string, args[1]->data.string)) {
        fprintf(stderr, "Security Error: Registry access denied: %s\\%s\n", 
                args[0]->data.string, args[1]->data.string);
        return value_bool(false);
    }
    
    HKEY root = get_root_key(args[0]->data.string);
    HKEY key;
    
    if (RegCreateKeyExA(root, args[1]->data.string, 0, NULL, 0, KEY_WRITE, NULL, &key, NULL) != ERROR_SUCCESS) {
        return value_bool(false);
    }
    
    LONG result;
    if (args[3]->type == HCS_VAL_STRING) {
        result = RegSetValueExA(key, args[2]->data.string, 0, REG_SZ, 
                               (BYTE*)args[3]->data.string, strlen(args[3]->data.string) + 1);
    } else if (args[3]->type == HCS_VAL_NUMBER) {
        DWORD val = (DWORD)args[3]->data.number;
        result = RegSetValueExA(key, args[2]->data.string, 0, REG_DWORD, (BYTE*)&val, sizeof(DWORD));
    } else {
        RegCloseKey(key);
        return value_bool(false);
    }
    
    RegCloseKey(key);
    return value_bool(result == ERROR_SUCCESS);
}

HcsValue* builtin_reg_read(int argc, HcsValue** args) {
    if (argc != 3 || args[0]->type != HCS_VAL_STRING || args[1]->type != HCS_VAL_STRING || 
        args[2]->type != HCS_VAL_STRING) return value_null();
    
    // Validate registry access
    if (!validate_registry_path(args[0]->data.string, args[1]->data.string)) {
        fprintf(stderr, "Security Error: Registry access denied: %s\\%s\n", 
                args[0]->data.string, args[1]->data.string);
        return value_null();
    }
    
    HKEY root = get_root_key(args[0]->data.string);
    HKEY key;
    
    if (RegOpenKeyExA(root, args[1]->data.string, 0, KEY_READ, &key) != ERROR_SUCCESS) {
        return value_null();
    }
    
    DWORD type, size = 1024;
    char buffer[1024];
    
    if (RegQueryValueExA(key, args[2]->data.string, NULL, &type, (BYTE*)buffer, &size) != ERROR_SUCCESS) {
        RegCloseKey(key);
        return value_null();
    }
    
    RegCloseKey(key);
    
    if (type == REG_SZ) return value_string(buffer);
    if (type == REG_DWORD) return value_number(*(DWORD*)buffer);
    return value_null();
}

HcsValue* builtin_reg_delete(int argc, HcsValue** args) {
    if (argc != 3 || args[0]->type != HCS_VAL_STRING || args[1]->type != HCS_VAL_STRING || 
        args[2]->type != HCS_VAL_STRING) return value_bool(false);
    
    HKEY root = get_root_key(args[0]->data.string);
    HKEY key;
    
    if (RegOpenKeyExA(root, args[1]->data.string, 0, KEY_WRITE, &key) != ERROR_SUCCESS) {
        return value_bool(false);
    }
    
    LONG result = RegDeleteValueA(key, args[2]->data.string);
    RegCloseKey(key);
    return value_bool(result == ERROR_SUCCESS);
}

HcsValue* builtin_reg_key_exists(int argc, HcsValue** args) {
    if (argc != 2 || args[0]->type != HCS_VAL_STRING || args[1]->type != HCS_VAL_STRING) {
        return value_bool(false);
    }
    
    HKEY root = get_root_key(args[0]->data.string);
    HKEY key;
    
    if (RegOpenKeyExA(root, args[1]->data.string, 0, KEY_READ, &key) == ERROR_SUCCESS) {
        RegCloseKey(key);
        return value_bool(true);
    }
    return value_bool(false);
}
