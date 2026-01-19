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

HcsValue* builtin_exec(int argc, HcsValue** args) {
    if (argc < 1 || args[0]->type != HCS_VAL_STRING) return value_number(-1);
    
    STARTUPINFOA si = {sizeof(si)};
    PROCESS_INFORMATION pi;
    
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = (argc > 1 && args[1]->type == HCS_VAL_BOOL && args[1]->data.boolean) ? SW_SHOW : SW_HIDE;
    
    char* cmd = strdup(args[0]->data.string);
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
    
    STARTUPINFOA si = {sizeof(si)};
    PROCESS_INFORMATION pi;
    
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = (argc > 1 && args[1]->type == HCS_VAL_BOOL && args[1]->data.boolean) ? SW_SHOW : SW_HIDE;
    
    char* cmd = strdup(args[0]->data.string);
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
