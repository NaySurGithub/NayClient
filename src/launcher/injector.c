#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tlhelp32.h>
#include <stdio.h>
#include <wchar.h>

#include "nay/launcher/injector.h"

static DWORD minecraft_pid(void)
{
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    PROCESSENTRY32W entry = {0};
    DWORD pid = 0;
    if (snapshot == INVALID_HANDLE_VALUE) return 0;
    entry.dwSize = sizeof(entry);
    if (Process32FirstW(snapshot, &entry)) {
        do {
            if (_wcsicmp(entry.szExeFile, L"Minecraft.Windows.exe") == 0) {
                pid = entry.th32ProcessID;
                break;
            }
        } while (Process32NextW(snapshot, &entry));
    }
    CloseHandle(snapshot);
    return pid;
}

static bool dll_path(WCHAR path[MAX_PATH])
{
    DWORD length = GetModuleFileNameW(NULL, path, MAX_PATH);
    WCHAR *slash;
    if (!length || length >= MAX_PATH) return false;
    slash = wcsrchr(path, L'\\');
    if (!slash) return false;
    wcscpy_s(slash + 1, MAX_PATH - (size_t)(slash + 1 - path), L"nayclient.dll");
    return GetFileAttributesW(path) != INVALID_FILE_ATTRIBUTES;
}

bool nay_inject_client(nay_injection *injection)
{
    WCHAR path[MAX_PATH];
    WCHAR event_name[96];
    WCHAR unload_name[96];
    WCHAR nofire_name[96];
    HANDLE process = NULL;
    HANDLE thread = NULL;
    LPVOID remote_path = NULL;
    SIZE_T bytes;
    HMODULE kernel32;
    FARPROC load_library;
    DWORD wait_result;

    if (!injection || !dll_path(path)) return false;
    injection->pid = minecraft_pid();
    if (!injection->pid) return false;

    process = OpenProcess(
        PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION
            | PROCESS_VM_WRITE | PROCESS_VM_READ,
        FALSE,
        injection->pid
    );
    if (!process) return false;
    bytes = (wcslen(path) + 1) * sizeof(WCHAR);
    remote_path = VirtualAllocEx(process, NULL, bytes, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!remote_path) goto cleanup;
    if (!WriteProcessMemory(process, remote_path, path, bytes, NULL)) goto cleanup;
    kernel32 = GetModuleHandleW(L"kernel32.dll");
    load_library = kernel32 ? GetProcAddress(kernel32, "LoadLibraryW") : NULL;
    if (!load_library) goto cleanup;
    thread = CreateRemoteThread(
        process, NULL, 0, (LPTHREAD_START_ROUTINE)load_library, remote_path, 0, NULL
    );
    if (!thread) goto cleanup;
    wait_result = WaitForSingleObject(thread, 10000);
    if (wait_result != WAIT_OBJECT_0) goto cleanup;

    swprintf_s(event_name, 96, L"Local\\NayClient.Fullbright.Toggle.%lu", injection->pid);
    swprintf_s(unload_name, 96, L"Local\\NayClient.Unload.%lu", injection->pid);
    swprintf_s(nofire_name, 96, L"Local\\NayClient.NoFire.Toggle.%lu", injection->pid);
    for (unsigned attempt = 0; attempt < 50; ++attempt) {
        injection->toggle_event = OpenEventW(EVENT_MODIFY_STATE, FALSE, event_name);
        injection->unload_event = OpenEventW(EVENT_MODIFY_STATE, FALSE, unload_name);
        injection->nofire_event = OpenEventW(EVENT_MODIFY_STATE, FALSE, nofire_name);
        if (injection->toggle_event && injection->nofire_event && injection->unload_event) break;
        if (injection->toggle_event) CloseHandle(injection->toggle_event);
        if (injection->unload_event) CloseHandle(injection->unload_event);
        if (injection->nofire_event) CloseHandle(injection->nofire_event);
        injection->toggle_event = NULL;
        injection->unload_event = NULL;
        injection->nofire_event = NULL;
        Sleep(100);
    }

cleanup:
    if (thread) CloseHandle(thread);
    if (remote_path) VirtualFreeEx(process, remote_path, 0, MEM_RELEASE);
    if (process) CloseHandle(process);
    return injection->toggle_event && injection->nofire_event && injection->unload_event;
}

bool nay_injection_toggle_nofire(nay_injection *injection)
{
    return injection && injection->nofire_event && SetEvent(injection->nofire_event);
}

bool nay_injection_toggle_fullbright(nay_injection *injection)
{
    return injection && injection->toggle_event && SetEvent(injection->toggle_event);
}

void nay_injection_close(nay_injection *injection)
{
    if (!injection) return;
    if (injection->unload_event) SetEvent(injection->unload_event);
    if (injection->toggle_event) CloseHandle(injection->toggle_event);
    if (injection->nofire_event) CloseHandle(injection->nofire_event);
    if (injection->unload_event) CloseHandle(injection->unload_event);
    injection->toggle_event = NULL;
    injection->nofire_event = NULL;
    injection->unload_event = NULL;
    injection->pid = 0;
}
