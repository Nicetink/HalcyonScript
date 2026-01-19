/*
 * HalcyonScript Application Launcher
 * 
 * This is a small launcher that:
 * 1. Checks if HalcyonScript is installed in the system
 * 2. Extracts embedded scripts to temp folder
 * 3. Runs HalcyonRT.exe with the extracted project
 */

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <shlobj.h>

/* Resource IDs */
#define IDR_APP_CONFIG  100
#define IDR_SCRIPTS     101

/* Find HalcyonRT.exe in system */
static char* find_halcyon(void) {
    static char path[MAX_PATH];
    
    /* Try PATH first */
    if (SearchPathA(NULL, "HalcyonRT.exe", NULL, MAX_PATH, path, NULL)) {
        return path;
    }
    
    /* Try common installation locations */
    const char* locations[] = {
        "C:\\Program Files\\HalcyonScript\\HalcyonRT.exe",
        "C:\\Program Files (x86)\\HalcyonScript\\HalcyonRT.exe",
        "C:\\HalcyonScript\\HalcyonRT.exe",
        NULL
    };
    
    for (int i = 0; locations[i]; i++) {
        if (GetFileAttributesA(locations[i]) != INVALID_FILE_ATTRIBUTES) {
            strcpy(path, locations[i]);
            return path;
        }
    }
    
    /* Try relative to exe */
    GetModuleFileNameA(NULL, path, MAX_PATH);
    char* last_sep = strrchr(path, '\\');
    if (last_sep) {
        strcpy(last_sep + 1, "HalcyonRT.exe");
        if (GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES) {
            return path;
        }
    }
    
    return NULL;
}

/* Get temp directory for this app */
static char* get_app_temp_dir(const char* app_name) {
    static char temp_dir[MAX_PATH];
    char temp_base[MAX_PATH];
    
    GetTempPathA(MAX_PATH, temp_base);
    snprintf(temp_dir, MAX_PATH, "%sHalcyonApps\\%s", temp_base, app_name);
    
    return temp_dir;
}

/* Create directory recursively */
static void create_dir_recursive(const char* path) {
    char tmp[MAX_PATH];
    strncpy(tmp, path, MAX_PATH - 1);
    tmp[MAX_PATH - 1] = '\0';
    
    for (char* p = tmp + 1; *p; p++) {
        if (*p == '\\' || *p == '/') {
            *p = '\0';
            CreateDirectoryA(tmp, NULL);
            *p = '\\';
        }
    }
    CreateDirectoryA(tmp, NULL);
}

/* Extract resource to file */
static BOOL extract_resource(HMODULE hModule, int resourceId, const char* outputPath) {
    HRSRC hRes = FindResourceA(hModule, MAKEINTRESOURCEA(resourceId), "HALCYON_DATA");
    if (!hRes) return FALSE;
    
    HGLOBAL hData = LoadResource(hModule, hRes);
    if (!hData) return FALSE;
    
    DWORD size = SizeofResource(hModule, hRes);
    void* data = LockResource(hData);
    if (!data) return FALSE;
    
    /* Create parent directory */
    char parent[MAX_PATH];
    strcpy(parent, outputPath);
    char* last_sep = strrchr(parent, '\\');
    if (last_sep) {
        *last_sep = '\0';
        create_dir_recursive(parent);
    }
    
    FILE* f = fopen(outputPath, "wb");
    if (!f) return FALSE;
    
    fwrite(data, 1, size, f);
    fclose(f);
    
    return TRUE;
}

/* Parse embedded manifest to get file list */
static int extract_all_files(HMODULE hModule, const char* temp_dir) {
    /* Extract app.halproj (resource 100) */
    char config_path[MAX_PATH];
    snprintf(config_path, MAX_PATH, "%s\\app.halproj", temp_dir);
    
    if (!extract_resource(hModule, IDR_APP_CONFIG, config_path)) {
        return 0;
    }
    
    /* Extract scripts bundle (resource 101) */
    HRSRC hRes = FindResourceA(hModule, MAKEINTRESOURCEA(IDR_SCRIPTS), "HALCYON_DATA");
    if (!hRes) return 0;
    
    HGLOBAL hData = LoadResource(hModule, hRes);
    if (!hData) return 0;
    
    DWORD size = SizeofResource(hModule, hRes);
    char* data = (char*)LockResource(hData);
    if (!data) return 0;
    
    /* Parse bundle format: [filename_len:4][filename][content_len:4][content]... */
    char* ptr = data;
    char* end = data + size;
    int file_count = 0;
    
    while (ptr < end) {
        /* Read filename length */
        if (ptr + 4 > end) break;
        DWORD name_len = *(DWORD*)ptr;
        ptr += 4;
        
        if (name_len == 0 || name_len > 1024) break;
        if (ptr + name_len > end) break;
        
        /* Read filename */
        char filename[1024];
        memcpy(filename, ptr, name_len);
        filename[name_len] = '\0';
        ptr += name_len;
        
        /* Read content length */
        if (ptr + 4 > end) break;
        DWORD content_len = *(DWORD*)ptr;
        ptr += 4;
        
        if (ptr + content_len > end) break;
        
        /* Write file */
        char file_path[MAX_PATH];
        snprintf(file_path, MAX_PATH, "%s\\scripts\\%s", temp_dir, filename);
        
        /* Create parent directory */
        char parent[MAX_PATH];
        strcpy(parent, file_path);
        char* last_sep = strrchr(parent, '\\');
        if (last_sep) {
            *last_sep = '\0';
            create_dir_recursive(parent);
        }
        
        FILE* f = fopen(file_path, "wb");
        if (f) {
            fwrite(ptr, 1, content_len, f);
            fclose(f);
            file_count++;
        }
        
        ptr += content_len;
    }
    
    return file_count;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    (void)hPrevInstance;
    (void)lpCmdLine;
    (void)nCmdShow;
    
    /* Find HalcyonRT.exe */
    char* halcyon_path = find_halcyon();
    if (!halcyon_path) {
        MessageBoxA(NULL, 
            "HalcyonScript is not installed on this system.\n\n"
            "Please install HalcyonScript from:\n"
            "https://github.com/KAInaps/HalcyonScript\n\n"
            "Or add HalcyonRT.exe to your system PATH.",
            "HalcyonScript Required",
            MB_OK | MB_ICONERROR);
        return 1;
    }
    
    /* Get app name from exe filename */
    char exe_path[MAX_PATH];
    GetModuleFileNameA(NULL, exe_path, MAX_PATH);
    char* exe_name = strrchr(exe_path, '\\');
    exe_name = exe_name ? exe_name + 1 : exe_path;
    
    char app_name[256];
    strcpy(app_name, exe_name);
    char* dot = strrchr(app_name, '.');
    if (dot) *dot = '\0';
    
    /* Get temp directory */
    char* temp_dir = get_app_temp_dir(app_name);
    create_dir_recursive(temp_dir);
    
    /* Extract files */
    int file_count = extract_all_files(hInstance, temp_dir);
    if (file_count == 0) {
        MessageBoxA(NULL,
            "Failed to extract application files.\n"
            "The application may be corrupted.",
            "Error",
            MB_OK | MB_ICONERROR);
        return 1;
    }
    
    /* Build command line */
    char cmd_line[MAX_PATH * 2];
    char config_path[MAX_PATH];
    snprintf(config_path, MAX_PATH, "%s\\app.halproj", temp_dir);
    snprintf(cmd_line, sizeof(cmd_line), "\"%s\" \"%s\"", halcyon_path, config_path);
    
    /* Start HalcyonRT.exe */
    STARTUPINFOA si = {0};
    PROCESS_INFORMATION pi = {0};
    si.cb = sizeof(si);
    
    /* Set working directory to temp dir */
    if (!CreateProcessA(NULL, cmd_line, NULL, NULL, FALSE, 0, NULL, temp_dir, &si, &pi)) {
        char error_msg[512];
        snprintf(error_msg, sizeof(error_msg),
            "Failed to start HalcyonScript.\n\n"
            "Command: %s\n"
            "Error code: %lu",
            cmd_line, GetLastError());
        MessageBoxA(NULL, error_msg, "Error", MB_OK | MB_ICONERROR);
        return 1;
    }
    
    /* Wait for process to finish */
    WaitForSingleObject(pi.hProcess, INFINITE);
    
    DWORD exit_code = 0;
    GetExitCodeProcess(pi.hProcess, &exit_code);
    
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    
    return (int)exit_code;
}
