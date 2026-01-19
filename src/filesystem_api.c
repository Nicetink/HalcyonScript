/*
 * HalcyonScript © KAInaps 2026 
   Simple programming for creative minds 
   github.com/Nicetink/HalcyonScript
 */

#include "runtime.h"
#include <windows.h>
#include <shlobj.h>
#include <stdio.h>
#include <sys/stat.h>

HcsValue* builtin_file_exists(int argc, HcsValue** args) {
    if (argc != 1 || args[0]->type != HCS_VAL_STRING) {
        return value_bool(false);
    }
    struct stat st;
    return value_bool(stat(args[0]->data.string, &st) == 0);
}

HcsValue* builtin_file_read(int argc, HcsValue** args) {
    if (argc != 1 || args[0]->type != HCS_VAL_STRING) return value_null();
    
    FILE* f = fopen(args[0]->data.string, "rb");
    if (!f) return value_null();
    
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    char* buffer = malloc(size + 1);
    fread(buffer, 1, size, f);
    buffer[size] = 0;
    fclose(f);
    
    HcsValue* result = value_string(buffer);
    free(buffer);
    return result;
}

HcsValue* builtin_file_write(int argc, HcsValue** args) {
    if (argc != 2 || args[0]->type != HCS_VAL_STRING || args[1]->type != HCS_VAL_STRING) {
        return value_bool(false);
    }
    
    FILE* f = fopen(args[0]->data.string, "wb");
    if (!f) return value_bool(false);
    
    fwrite(args[1]->data.string, 1, strlen(args[1]->data.string), f);
    fclose(f);
    return value_bool(true);
}

HcsValue* builtin_file_copy(int argc, HcsValue** args) {
    if (argc != 2 || args[0]->type != HCS_VAL_STRING || args[1]->type != HCS_VAL_STRING) {
        return value_bool(false);
    }
    return value_bool(CopyFileA(args[0]->data.string, args[1]->data.string, FALSE));
}

HcsValue* builtin_file_delete(int argc, HcsValue** args) {
    if (argc != 1 || args[0]->type != HCS_VAL_STRING) return value_bool(false);
    return value_bool(DeleteFileA(args[0]->data.string));
}

HcsValue* builtin_file_size(int argc, HcsValue** args) {
    if (argc != 1 || args[0]->type != HCS_VAL_STRING) return value_number(0);
    
    struct stat st;
    if (stat(args[0]->data.string, &st) != 0) return value_number(0);
    return value_number((double)st.st_size);
}

HcsValue* builtin_dir_create(int argc, HcsValue** args) {
    if (argc != 1 || args[0]->type != HCS_VAL_STRING) return value_bool(false);
    return value_bool(CreateDirectoryA(args[0]->data.string, NULL) || GetLastError() == ERROR_ALREADY_EXISTS);
}

HcsValue* builtin_dir_exists(int argc, HcsValue** args) {
    if (argc != 1 || args[0]->type != HCS_VAL_STRING) return value_bool(false);
    DWORD attr = GetFileAttributesA(args[0]->data.string);
    return value_bool(attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY));
}

HcsValue* builtin_dir_delete(int argc, HcsValue** args) {
    if (argc != 1 || args[0]->type != HCS_VAL_STRING) return value_bool(false);
    return value_bool(RemoveDirectoryA(args[0]->data.string));
}

HcsValue* builtin_dir_list(int argc, HcsValue** args) {
    if (argc != 1 || args[0]->type != HCS_VAL_STRING) return value_array();
    
    HcsValue* result = value_array();
    char pattern[MAX_PATH];
    snprintf(pattern, MAX_PATH, "%s\\*", args[0]->data.string);
    
    WIN32_FIND_DATAA fd;
    HANDLE hFind = FindFirstFileA(pattern, &fd);
    if (hFind == INVALID_HANDLE_VALUE) return result;
    
    do {
        if (strcmp(fd.cFileName, ".") != 0 && strcmp(fd.cFileName, "..") != 0) {
            value_array_push(result, value_string(fd.cFileName));
        }
    } while (FindNextFileA(hFind, &fd));
    
    FindClose(hFind);
    return result;
}

HcsValue* builtin_path_join(int argc, HcsValue** args) {
    if (argc < 1) return value_string("");
    
    char result[MAX_PATH] = "";
    for (int i = 0; i < argc; i++) {
        if (args[i]->type != HCS_VAL_STRING) continue;
        if (i > 0 && result[0]) strcat(result, "\\");
        strcat(result, args[i]->data.string);
    }
    return value_string(result);
}

HcsValue* builtin_path_basename(int argc, HcsValue** args) {
    if (argc != 1 || args[0]->type != HCS_VAL_STRING) return value_string("");
    
    char* path = args[0]->data.string;
    char* last = strrchr(path, '\\');
    if (!last) last = strrchr(path, '/');
    return value_string(last ? last + 1 : path);
}

HcsValue* builtin_path_dirname(int argc, HcsValue** args) {
    if (argc != 1 || args[0]->type != HCS_VAL_STRING) return value_string("");
    
    char* path = strdup(args[0]->data.string);
    char* last = strrchr(path, '\\');
    if (!last) last = strrchr(path, '/');
    if (last) *last = 0;
    
    HcsValue* result = value_string(path);
    free(path);
    return result;
}

HcsValue* builtin_get_temp_path(int argc, HcsValue** args) {
    char temp[MAX_PATH];
    GetTempPathA(MAX_PATH, temp);
    return value_string(temp);
}

HcsValue* builtin_get_program_files(int argc, HcsValue** args) {
    char path[MAX_PATH];
    SHGetFolderPathA(NULL, CSIDL_PROGRAM_FILES, NULL, 0, path);
    return value_string(path);
}

HcsValue* builtin_get_appdata(int argc, HcsValue** args) {
    char path[MAX_PATH];
    SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, path);
    return value_string(path);
}

HcsValue* builtin_get_desktop(int argc, HcsValue** args) {
    char path[MAX_PATH];
    SHGetFolderPathA(NULL, CSIDL_DESKTOP, NULL, 0, path);
    return value_string(path);
}

HcsValue* builtin_get_startmenu(int argc, HcsValue** args) {
    char path[MAX_PATH];
    SHGetFolderPathA(NULL, CSIDL_STARTMENU, NULL, 0, path);
    return value_string(path);
}
