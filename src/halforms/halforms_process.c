/*
 * HalForms - Process Execution and System Integration
 * For running compilers, debuggers, and external tools
 */

#include "halforms.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <shlobj.h>

/* ============================================
   Process Execution
   ============================================ */

typedef struct HalProcess {
    HANDLE hProcess;
    HANDLE hThread;
    HANDLE hStdOutRead;
    HANDLE hStdOutWrite;
    HANDLE hStdInRead;
    HANDLE hStdInWrite;
    HANDLE hStdErrRead;
    HANDLE hStdErrWrite;
    DWORD processId;
    bool running;
    char* workingDir;
    char* command;
} HalProcess;

/* Create a new process */
HalProcess* halprocess_create(const char* command, const char* workingDir, bool redirectOutput) {
    HalProcess* proc = (HalProcess*)calloc(1, sizeof(HalProcess));
    if (!proc) return NULL;
    
    proc->command = _strdup(command);
    proc->workingDir = workingDir ? _strdup(workingDir) : NULL;
    
    if (redirectOutput) {
        SECURITY_ATTRIBUTES sa = {0};
        sa.nLength = sizeof(sa);
        sa.bInheritHandle = TRUE;
        
        /* Create pipes for stdout */
        if (!CreatePipe(&proc->hStdOutRead, &proc->hStdOutWrite, &sa, 0)) {
            free(proc->command);
            free(proc->workingDir);
            free(proc);
            return NULL;
        }
        SetHandleInformation(proc->hStdOutRead, HANDLE_FLAG_INHERIT, 0);
        
        /* Create pipes for stderr */
        if (!CreatePipe(&proc->hStdErrRead, &proc->hStdErrWrite, &sa, 0)) {
            CloseHandle(proc->hStdOutRead);
            CloseHandle(proc->hStdOutWrite);
            free(proc->command);
            free(proc->workingDir);
            free(proc);
            return NULL;
        }
        SetHandleInformation(proc->hStdErrRead, HANDLE_FLAG_INHERIT, 0);
        
        /* Create pipes for stdin */
        if (!CreatePipe(&proc->hStdInRead, &proc->hStdInWrite, &sa, 0)) {
            CloseHandle(proc->hStdOutRead);
            CloseHandle(proc->hStdOutWrite);
            CloseHandle(proc->hStdErrRead);
            CloseHandle(proc->hStdErrWrite);
            free(proc->command);
            free(proc->workingDir);
            free(proc);
            return NULL;
        }
        SetHandleInformation(proc->hStdInWrite, HANDLE_FLAG_INHERIT, 0);
    }
    
    return proc;
}

/* Start the process */
bool halprocess_start(HalProcess* proc) {
    if (!proc || proc->running) return false;
    
    STARTUPINFOA si = {0};
    PROCESS_INFORMATION pi = {0};
    
    si.cb = sizeof(si);
    
    if (proc->hStdOutWrite) {
        si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
        si.hStdOutput = proc->hStdOutWrite;
        si.hStdError = proc->hStdErrWrite;
        si.hStdInput = proc->hStdInRead;
        si.wShowWindow = SW_HIDE;
    }
    
    char* cmdLine = _strdup(proc->command);
    
    BOOL result = CreateProcessA(
        NULL,
        cmdLine,
        NULL,
        NULL,
        proc->hStdOutWrite ? TRUE : FALSE,
        CREATE_NO_WINDOW,
        NULL,
        proc->workingDir,
        &si,
        &pi
    );
    
    free(cmdLine);
    
    if (!result) {
        return false;
    }
    
    proc->hProcess = pi.hProcess;
    proc->hThread = pi.hThread;
    proc->processId = pi.dwProcessId;
    proc->running = true;
    
    /* Close write handles in parent process */
    if (proc->hStdOutWrite) {
        CloseHandle(proc->hStdOutWrite);
        proc->hStdOutWrite = NULL;
    }
    if (proc->hStdErrWrite) {
        CloseHandle(proc->hStdErrWrite);
        proc->hStdErrWrite = NULL;
    }
    if (proc->hStdInRead) {
        CloseHandle(proc->hStdInRead);
        proc->hStdInRead = NULL;
    }
    
    return true;
}

/* Read output from process (non-blocking) */
char* halprocess_read_output(HalProcess* proc, int maxBytes) {
    if (!proc || !proc->hStdOutRead) return NULL;
    
    DWORD available = 0;
    if (!PeekNamedPipe(proc->hStdOutRead, NULL, 0, NULL, &available, NULL) || available == 0) {
        return NULL;
    }
    
    int toRead = (maxBytes > 0 && (int)available > maxBytes) ? maxBytes : (int)available;
    char* buffer = (char*)malloc(toRead + 1);
    
    DWORD bytesRead = 0;
    if (!ReadFile(proc->hStdOutRead, buffer, toRead, &bytesRead, NULL)) {
        free(buffer);
        return NULL;
    }
    
    buffer[bytesRead] = '\0';
    return buffer;
}

/* Read error output from process (non-blocking) */
char* halprocess_read_error(HalProcess* proc, int maxBytes) {
    if (!proc || !proc->hStdErrRead) return NULL;
    
    DWORD available = 0;
    if (!PeekNamedPipe(proc->hStdErrRead, NULL, 0, NULL, &available, NULL) || available == 0) {
        return NULL;
    }
    
    int toRead = (maxBytes > 0 && (int)available > maxBytes) ? maxBytes : (int)available;
    char* buffer = (char*)malloc(toRead + 1);
    
    DWORD bytesRead = 0;
    if (!ReadFile(proc->hStdErrRead, buffer, toRead, &bytesRead, NULL)) {
        free(buffer);
        return NULL;
    }
    
    buffer[bytesRead] = '\0';
    return buffer;
}

/* Write to process stdin */
bool halprocess_write_input(HalProcess* proc, const char* data) {
    if (!proc || !proc->hStdInWrite || !data) return false;
    
    DWORD bytesWritten = 0;
    return WriteFile(proc->hStdInWrite, data, (DWORD)strlen(data), &bytesWritten, NULL) != 0;
}

/* Check if process is still running */
bool halprocess_is_running(HalProcess* proc) {
    if (!proc || !proc->hProcess) return false;
    
    DWORD exitCode;
    if (GetExitCodeProcess(proc->hProcess, &exitCode)) {
        proc->running = (exitCode == STILL_ACTIVE);
    }
    return proc->running;
}

/* Get exit code */
int halprocess_get_exit_code(HalProcess* proc) {
    if (!proc || !proc->hProcess) return -1;
    
    DWORD exitCode = 0;
    GetExitCodeProcess(proc->hProcess, &exitCode);
    return (int)exitCode;
}

/* Wait for process to complete */
bool halprocess_wait(HalProcess* proc, int timeoutMs) {
    if (!proc || !proc->hProcess) return false;
    
    DWORD result = WaitForSingleObject(proc->hProcess, timeoutMs < 0 ? INFINITE : (DWORD)timeoutMs);
    return result == WAIT_OBJECT_0;
}

/* Kill the process */
bool halprocess_kill(HalProcess* proc) {
    if (!proc || !proc->hProcess) return false;
    
    if (TerminateProcess(proc->hProcess, 1)) {
        proc->running = false;
        return true;
    }
    return false;
}

/* Destroy process handle */
void halprocess_destroy(HalProcess* proc) {
    if (!proc) return;
    
    if (proc->running) {
        halprocess_kill(proc);
    }
    
    if (proc->hProcess) CloseHandle(proc->hProcess);
    if (proc->hThread) CloseHandle(proc->hThread);
    if (proc->hStdOutRead) CloseHandle(proc->hStdOutRead);
    if (proc->hStdErrRead) CloseHandle(proc->hStdErrRead);
    if (proc->hStdInWrite) CloseHandle(proc->hStdInWrite);
    
    free(proc->command);
    free(proc->workingDir);
    free(proc);
}

/* ============================================
   Simple Process Execution (blocking)
   ============================================ */

/* Execute command and return output */
char* halprocess_exec(const char* command, const char* workingDir, int* exitCode) {
    HalProcess* proc = halprocess_create(command, workingDir, true);
    if (!proc) return NULL;
    
    if (!halprocess_start(proc)) {
        halprocess_destroy(proc);
        return NULL;
    }
    
    /* Wait for completion */
    halprocess_wait(proc, -1);
    
    /* Read all output */
    char* output = NULL;
    int outputLen = 0;
    int outputCap = 0;
    
    char* chunk;
    while ((chunk = halprocess_read_output(proc, 4096)) != NULL) {
        int chunkLen = (int)strlen(chunk);
        if (outputLen + chunkLen >= outputCap) {
            outputCap = outputCap == 0 ? 4096 : outputCap * 2;
            if (outputLen + chunkLen >= outputCap) outputCap = outputLen + chunkLen + 1;
            output = (char*)realloc(output, outputCap);
        }
        memcpy(output + outputLen, chunk, chunkLen);
        outputLen += chunkLen;
        free(chunk);
    }
    
    /* Also read stderr */
    while ((chunk = halprocess_read_error(proc, 4096)) != NULL) {
        int chunkLen = (int)strlen(chunk);
        if (outputLen + chunkLen >= outputCap) {
            outputCap = outputCap == 0 ? 4096 : outputCap * 2;
            if (outputLen + chunkLen >= outputCap) outputCap = outputLen + chunkLen + 1;
            output = (char*)realloc(output, outputCap);
        }
        memcpy(output + outputLen, chunk, chunkLen);
        outputLen += chunkLen;
        free(chunk);
    }
    
    if (output) output[outputLen] = '\0';
    
    if (exitCode) {
        *exitCode = halprocess_get_exit_code(proc);
    }
    
    halprocess_destroy(proc);
    return output ? output : _strdup("");
}

/* ============================================
   Clipboard Functions
   ============================================ */

char* halforms_clipboard_get_text(void) {
    if (!OpenClipboard(NULL)) return NULL;
    
    HANDLE hData = GetClipboardData(CF_UNICODETEXT);
    if (!hData) {
        CloseClipboard();
        return NULL;
    }
    
    wchar_t* wtext = (wchar_t*)GlobalLock(hData);
    if (!wtext) {
        CloseClipboard();
        return NULL;
    }
    
    /* Convert to UTF-8 */
    int len = WideCharToMultiByte(CP_UTF8, 0, wtext, -1, NULL, 0, NULL, NULL);
    char* text = (char*)malloc(len);
    WideCharToMultiByte(CP_UTF8, 0, wtext, -1, text, len, NULL, NULL);
    
    GlobalUnlock(hData);
    CloseClipboard();
    
    return text;
}

bool halforms_clipboard_set_text(const char* text) {
    if (!text) return false;
    
    /* Convert to wide string */
    int wlen = MultiByteToWideChar(CP_UTF8, 0, text, -1, NULL, 0);
    
    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, wlen * sizeof(wchar_t));
    if (!hMem) return false;
    
    wchar_t* wtext = (wchar_t*)GlobalLock(hMem);
    MultiByteToWideChar(CP_UTF8, 0, text, -1, wtext, wlen);
    GlobalUnlock(hMem);
    
    if (!OpenClipboard(NULL)) {
        GlobalFree(hMem);
        return false;
    }
    
    EmptyClipboard();
    SetClipboardData(CF_UNICODETEXT, hMem);
    CloseClipboard();
    
    return true;
}

/* ============================================
   System Functions
   ============================================ */

/* Get environment variable */
char* halforms_getenv(const char* name) {
    char buffer[32768];
    DWORD len = GetEnvironmentVariableA(name, buffer, sizeof(buffer));
    if (len == 0 || len >= sizeof(buffer)) return NULL;
    return _strdup(buffer);
}

/* Set environment variable */
bool halforms_setenv(const char* name, const char* value) {
    return SetEnvironmentVariableA(name, value) != 0;
}

/* Get current directory */
char* halforms_getcwd(void) {
    char buffer[MAX_PATH];
    DWORD len = GetCurrentDirectoryA(sizeof(buffer), buffer);
    if (len == 0 || len >= sizeof(buffer)) return NULL;
    return _strdup(buffer);
}

/* Set current directory */
bool halforms_chdir(const char* path) {
    return SetCurrentDirectoryA(path) != 0;
}

/* Get temp directory */
char* halforms_get_temp_dir(void) {
    char buffer[MAX_PATH];
    DWORD len = GetTempPathA(sizeof(buffer), buffer);
    if (len == 0 || len >= sizeof(buffer)) return NULL;
    return _strdup(buffer);
}

/* Get user home directory */
char* halforms_get_home_dir(void) {
    char buffer[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_PROFILE, NULL, 0, buffer))) {
        return _strdup(buffer);
    }
    return halforms_getenv("USERPROFILE");
}

/* Get application data directory */
char* halforms_get_appdata_dir(void) {
    char buffer[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, buffer))) {
        return _strdup(buffer);
    }
    return halforms_getenv("APPDATA");
}

/* Open file/URL with default application */
bool halforms_shell_open(const char* path) {
    HINSTANCE result = ShellExecuteA(NULL, "open", path, NULL, NULL, SW_SHOWNORMAL);
    return (INT_PTR)result > 32;
}

/* Show file in explorer */
bool halforms_shell_show_in_folder(const char* path) {
    char command[MAX_PATH + 32];
    snprintf(command, sizeof(command), "/select,\"%s\"", path);
    HINSTANCE result = ShellExecuteA(NULL, "open", "explorer.exe", command, NULL, SW_SHOWNORMAL);
    return (INT_PTR)result > 32;
}

/* ============================================
   Timer Functions
   ============================================ */

typedef struct HalTimer {
    UINT_PTR id;
    int interval;
    bool running;
    void (*callback)(void* userData);
    void* userData;
} HalTimer;

static HalTimer* g_timers = NULL;
static int g_timerCount = 0;
static int g_timerCapacity = 0;

static void CALLBACK TimerProc(HWND hwnd, UINT msg, UINT_PTR id, DWORD time) {
    for (int i = 0; i < g_timerCount; i++) {
        if (g_timers[i].id == id && g_timers[i].callback) {
            g_timers[i].callback(g_timers[i].userData);
            break;
        }
    }
}

HalTimer* haltimer_create(int intervalMs, void (*callback)(void*), void* userData) {
    if (g_timerCount >= g_timerCapacity) {
        g_timerCapacity = g_timerCapacity == 0 ? 16 : g_timerCapacity * 2;
        g_timers = (HalTimer*)realloc(g_timers, g_timerCapacity * sizeof(HalTimer));
    }
    
    HalTimer* timer = &g_timers[g_timerCount++];
    timer->interval = intervalMs;
    timer->callback = callback;
    timer->userData = userData;
    timer->running = false;
    timer->id = 0;
    
    return timer;
}

void haltimer_start(HalTimer* timer) {
    if (!timer || timer->running) return;
    
    timer->id = SetTimer(NULL, 0, timer->interval, TimerProc);
    timer->running = (timer->id != 0);
}

void haltimer_stop(HalTimer* timer) {
    if (!timer || !timer->running) return;
    
    KillTimer(NULL, timer->id);
    timer->running = false;
    timer->id = 0;
}

void haltimer_destroy(HalTimer* timer) {
    if (!timer) return;
    
    haltimer_stop(timer);
    
    /* Remove from array */
    for (int i = 0; i < g_timerCount; i++) {
        if (&g_timers[i] == timer) {
            g_timers[i] = g_timers[--g_timerCount];
            break;
        }
    }
}
