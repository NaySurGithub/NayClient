#include "nay/client.h"

#include "nay/modules/fullbright/fullbright.h"
#include "nay/modules/nofire/nofire.h"
#include "nay/modules/module/module.h"
#include "nay/platform/minecraft_version.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>

typedef struct nay_client_state {
    bool running;
    bool solo_only;
    nay_fullbright fullbright;
    nay_nofire nofire;
} nay_client_state;

static nay_client_state g_client;
static HANDLE g_stop_event;
static HANDLE g_toggle_event;
static HANDLE g_nofire_event;
static HANDLE g_unload_event;
static HMODULE g_instance;

static DWORD WINAPI nay_worker(LPVOID unused)
{
    WCHAR event_name[96];
    WCHAR unload_name[96];
    WCHAR nofire_name[96];
    nay_client_config config = {true, false, 9999.0f};
    (void)unused;
    swprintf_s(
        event_name, 96, L"Local\\NayClient.Fullbright.Toggle.%lu", GetCurrentProcessId()
    );
    g_toggle_event = CreateEventW(NULL, FALSE, FALSE, event_name);
    swprintf_s(nofire_name, 96, L"Local\\NayClient.NoFire.Toggle.%lu", GetCurrentProcessId());
    g_nofire_event = CreateEventW(NULL, FALSE, FALSE, nofire_name);
    swprintf_s(unload_name, 96, L"Local\\NayClient.Unload.%lu", GetCurrentProcessId());
    g_unload_event = CreateEventW(NULL, FALSE, FALSE, unload_name);
    g_stop_event = CreateEventW(NULL, TRUE, FALSE, NULL);
    if (!g_toggle_event || !g_nofire_event || !g_unload_event || !g_stop_event || !nay_client_start(&config)) return 1;
    for (;;) {
        HANDLE events[4] = {g_stop_event, g_toggle_event, g_nofire_event, g_unload_event};
        DWORD result = WaitForMultipleObjects(4, events, FALSE, INFINITE);
        if (result == WAIT_OBJECT_0) break;
        if (result == WAIT_OBJECT_0 + 1) {
            (void)nay_fullbright_toggle(&g_client.fullbright);
        }
        if (result == WAIT_OBJECT_0 + 2) {
            (void)nay_nofire_toggle(&g_client.nofire);
        }
        if (result == WAIT_OBJECT_0 + 3) {
            nay_client_stop();
            CloseHandle(g_toggle_event);
            CloseHandle(g_nofire_event);
            CloseHandle(g_unload_event);
            CloseHandle(g_stop_event);
            g_toggle_event = NULL;
            g_nofire_event = NULL;
            g_unload_event = NULL;
            g_stop_event = NULL;
            FreeLibraryAndExitThread(g_instance, 0);
        }
    }
    nay_client_stop();
    return 0;
}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved)
{
    (void)reserved;
    if (reason == DLL_PROCESS_ATTACH) {
        g_instance = instance;
        DisableThreadLibraryCalls(instance);
        if (!CreateThread(NULL, 0, nay_worker, NULL, 0, NULL)) return FALSE;
    } else if (reason == DLL_PROCESS_DETACH && g_stop_event) {
        SetEvent(g_stop_event);
    }
    return TRUE;
}

bool nay_client_start(const nay_client_config *config)
{
    if (!config || g_client.running || !config->solo_only) return false;

    g_client.solo_only = true;
    nay_fullbright_init(
        &g_client.fullbright,
        config->fullbright_enabled,
        config->fullbright_level
    );
    nay_nofire_init(&g_client.nofire, false);
    g_client.running = true;
    return true;
}

void nay_client_tick(void)
{
    if (g_client.running) nay_module_tick_all();
}

void nay_client_stop(void)
{
    if (!g_client.running) return;
    nay_module_shutdown_all();
    g_client.running = false;
}

bool nay_client_is_running(void)
{
    return g_client.running;
}

bool nay_can_inject(void)
{
    nay_version version;

    if (!nay_minecraft_windows_version(&version)) {
        nay_show_unsupported_version_popup(NULL);
        return false;
    }

    if (!nay_minecraft_version_supported(&version)) {
        nay_show_unsupported_version_popup(&version);
        return false;
    }

    return true;
}

void nay_fullbright_set_enabled(bool enabled)
{
    if (g_client.running) nay_fullbright_enable(&g_client.fullbright, enabled);
}

void nay_fullbright_set_level(float level)
{
    if (g_client.running) nay_fullbright_level(&g_client.fullbright, level);
}
