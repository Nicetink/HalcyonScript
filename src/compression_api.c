/*
 * HalcyonScript © KAInaps 2026 
   Simple programming for creative minds 
   github.com/Nicetink/HalcyonScript
 */

#include "runtime.h"
#include <windows.h>
#include <stdio.h>

HcsValue* builtin_compress_folder(int argc, HcsValue** args) {
    if (argc != 2 || args[0]->type != HCS_VAL_STRING || args[1]->type != HCS_VAL_STRING) {
        return value_bool(false);
    }
    
    char cmd[2048];
    snprintf(cmd, sizeof(cmd), "powershell -Command \"Compress-Archive -Path '%s\\*' -DestinationPath '%s' -Force\"",
             args[0]->data.string, args[1]->data.string);
    
    STARTUPINFOA si = {sizeof(si)};
    PROCESS_INFORMATION pi;
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    
    if (!CreateProcessA(NULL, cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        return value_bool(false);
    }
    
    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD exitCode;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    
    return value_bool(exitCode == 0);
}

HcsValue* builtin_extract_archive(int argc, HcsValue** args) {
    if (argc != 2 || args[0]->type != HCS_VAL_STRING || args[1]->type != HCS_VAL_STRING) {
        return value_bool(false);
    }
    
    char cmd[2048];
    snprintf(cmd, sizeof(cmd), "powershell -Command \"Expand-Archive -Path '%s' -DestinationPath '%s' -Force\"",
             args[0]->data.string, args[1]->data.string);
    
    STARTUPINFOA si = {sizeof(si)};
    PROCESS_INFORMATION pi;
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    
    if (!CreateProcessA(NULL, cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        return value_bool(false);
    }
    
    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD exitCode;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    
    return value_bool(exitCode == 0);
}
