/*
 * HalcyonScript © KAInaps 2026 
   Simple programming for creative minds 
   github.com/Nicetink/HalcyonScript
 */

#include "runtime.h"
#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>
#include <objbase.h>

// Validate command to prevent injection attacks
static int validate_command(const char* cmd) {
    if (!cmd || strlen(cmd) == 0) return 0;
    
    // Check for dangerous characters and sequences
    const char* dangerous[] = {
        "&", "|", ";", "`", "$", "(", ")", "{", "}", 
        "<", ">", ">>", "<<", "&&", "||", "^", "%",
        "del ", "rmdir ", "format ", "shutdown ", "reboot ",
        "net ", "sc ", "reg ", "wmic ", "powershell",
        NULL
    };
    
    for (int i = 0; dangerous[i]; i++) {
        if (strstr(cmd, dangerous[i])) {
            return 0; // Dangerous command detected
        }
    }
    
    // Only allow whitelisted executables
    const char* allowed[] = {
        "notepad.exe", "calc.exe", "mspaint.exe", "explorer.exe",
        "cmd.exe /c dir", "cmd.exe /c type", "cmd.exe /c echo",
        NULL
    };
    
    for (int i = 0; allowed[i]; i++) {
        if (strncmp(cmd, allowed[i], strlen(allowed[i])) == 0) {
            return 1; // Command is whitelisted
        }
    }
    
    return 0; // Command not in whitelist
}

HcsValue* builtin_exec(int argc, HcsValue** args) {
    if (argc < 1 || args[0]->type != HCS_VAL_STRING) return value_number(-1);
    
    // Validate command for security
    if (!validate_command(args[0]->data.string)) {
        fprintf(stderr, "Security Error: Command not allowed: %s\n", args[0]->data.string);
        return value_number(-1);
    }
    
    STARTUPINFOA si = {sizeof(si)};
    PROCESS_INFORMATION pi;
    
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = (argc > 1 && args[1]->type == HCS_VAL_BOOL && args[1]->data.boolean) ? SW_SHOW : SW_HIDE;
    
    // Use safer approach - don't modify the original string
    size_t cmd_len = strlen(args[0]->data.string) + 1;
    char* cmd = malloc(cmd_len);
    if (!cmd) return value_number(-1);
    
    strncpy(cmd, args[0]->data.string, cmd_len - 1);
    cmd[cmd_len - 1] = '\0';
    
    if (!CreateProcessA(NULL, cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        free(cmd);
        return value_number(-1);
    }
    free(cmd);
    
    WaitForSingleObject(pi.hProcess, INFINITE);
    
    DWORD exitCode;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    
    return value_number(exitCode);
}

HcsValue* builtin_exec_async(int argc, HcsValue** args) {
    if (argc < 1 || args[0]->type != HCS_VAL_STRING) return value_bool(false);
    
    // Validate command for security
    if (!validate_command(args[0]->data.string)) {
        fprintf(stderr, "Security Error: Command not allowed: %s\n", args[0]->data.string);
        return value_bool(false);
    }
    
    STARTUPINFOA si = {sizeof(si)};
    PROCESS_INFORMATION pi;
    
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = (argc > 1 && args[1]->type == HCS_VAL_BOOL && args[1]->data.boolean) ? SW_SHOW : SW_HIDE;
    
    size_t cmd_len = strlen(args[0]->data.string) + 1;
    char* cmd = malloc(cmd_len);
    if (!cmd) return value_bool(false);
    
    strncpy(cmd, args[0]->data.string, cmd_len - 1);
    cmd[cmd_len - 1] = '\0';
    
    BOOL result = CreateProcessA(NULL, cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
    free(cmd);
    
    if (result) {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
    
    return value_bool(result);
}

HcsValue* builtin_shell_execute(int argc, HcsValue** args) {
    if (argc < 1 || args[0]->type != HCS_VAL_STRING) return value_bool(false);
    
    const char* verb = (argc > 1 && args[1]->type == HCS_VAL_STRING) ? args[1]->data.string : "open";
    const char* params = (argc > 2 && args[2]->type == HCS_VAL_STRING) ? args[2]->data.string : NULL;
    
    HINSTANCE result = ShellExecuteA(NULL, verb, args[0]->data.string, params, NULL, SW_SHOWNORMAL);
    return value_bool((INT_PTR)result > 32);
}

HcsValue* builtin_create_shortcut(int argc, HcsValue** args) {
    if (argc < 2 || args[0]->type != HCS_VAL_STRING || args[1]->type != HCS_VAL_STRING) {
        return value_bool(false);
    }
    
    CoInitialize(NULL);
    
    IShellLinkA* psl;
    HRESULT hr = CoCreateInstance(&CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, &IID_IShellLinkA, (void**)&psl);
    
    if (SUCCEEDED(hr)) {
        IPersistFile* ppf;
        
        psl->lpVtbl->SetPath(psl, args[1]->data.string);
        
        if (argc > 2 && args[2]->type == HCS_VAL_STRING) {
            psl->lpVtbl->SetArguments(psl, args[2]->data.string);
        }
        if (argc > 3 && args[3]->type == HCS_VAL_STRING) {
            psl->lpVtbl->SetDescription(psl, args[3]->data.string);
        }
        if (argc > 4 && args[4]->type == HCS_VAL_STRING) {
            psl->lpVtbl->SetWorkingDirectory(psl, args[4]->data.string);
        }
        
        hr = psl->lpVtbl->QueryInterface(psl, &IID_IPersistFile, (void**)&ppf);
        
        if (SUCCEEDED(hr)) {
            WCHAR wsz[MAX_PATH];
            MultiByteToWideChar(CP_ACP, 0, args[0]->data.string, -1, wsz, MAX_PATH);
            hr = ppf->lpVtbl->Save(ppf, wsz, TRUE);
            ppf->lpVtbl->Release(ppf);
        }
        
        psl->lpVtbl->Release(psl);
    }
    
    CoUninitialize();
    return value_bool(SUCCEEDED(hr));
}
