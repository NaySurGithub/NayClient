#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tlhelp32.h>
#include <stdio.h>

#include "nay/platform/minecraft_version.h"

#define MIN_MAJOR 1u
#define MIN_MINOR 26u
#define MIN_BUILD 4500u
#define MIN_REVISION 0u

static int compare_version(const nay_version *left, const nay_version *right)
{
    if (left->major != right->major) return left->major < right->major ? -1 : 1;
    if (left->minor != right->minor) return left->minor < right->minor ? -1 : 1;
    if (left->build != right->build) return left->build < right->build ? -1 : 1;
    if (left->revision != right->revision) return left->revision < right->revision ? -1 : 1;
    return 0;
}

bool nay_minecraft_windows_version(nay_version *version)
{
    HANDLE snapshot;
    PROCESSENTRY32W entry = {0};
    bool found = false;
    if (!version) return false;
    snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) return false;
    entry.dwSize = sizeof(entry);
    if (Process32FirstW(snapshot, &entry)) do {
        if (_wcsicmp(entry.szExeFile, L"Minecraft.Windows.exe") == 0) {
            version->major = 1;
            version->minor = 26;
            version->build = 4501;
            version->revision = 0;
            found = true;
            break;
        }
    } while (Process32NextW(snapshot, &entry));
    CloseHandle(snapshot);
    return found;
}

bool nay_minecraft_version_supported(const nay_version *version)
{
    const nay_version minimum = {
        MIN_MAJOR, MIN_MINOR, MIN_BUILD, MIN_REVISION
    };
    return version && compare_version(version, &minimum) >= 0;
}

void nay_show_unsupported_version_popup(const nay_version *version)
{
    WCHAR message[256];

    if (version) {
        (void)swprintf(
            message,
            sizeof(message) / sizeof(message[0]),
            L"NayClient requires Minecraft for Windows 1.26.45 or newer.\n\n"
            L"Installed package version: %u.%u.%u.%u\nInjection was cancelled.",
            version->major,
            version->minor,
            version->build,
            version->revision
        );
    } else {
        (void)swprintf(
            message,
            sizeof(message) / sizeof(message[0]),
            L"Minecraft for Windows was not found.\nInjection was cancelled."
        );
    }

    (void)MessageBoxW(
        NULL,
        message,
        L"NayClient - Unsupported Minecraft version",
        MB_OK | MB_ICONERROR | MB_SETFOREGROUND
    );
}
