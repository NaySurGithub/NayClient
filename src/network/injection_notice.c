#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>
#include <string.h>
#include <wchar.h>

#include "nay/network/injection_notice.h"

#define TEXT_PACKET_ID 0x09u
#define MAX_PACKET_ID 0x140u
#define PLAYER_LIST_HANDLER_INDEX 66u
#define PACKET_HANDLER_OFFSET 0x20u
#define TEXT_TYPE_OFFSET 0xA0u
#define TEXT_STRING_OFFSET 0xA8u

typedef struct nay_shared_packet {
    void *packet;
    void *control;
} nay_shared_packet;

typedef void *(__fastcall *nay_text_factory_fn)(nay_shared_packet *result);
typedef void (__fastcall *nay_receive_fn)(
    void *handler,
    void *network_identifier,
    void *net_event_callback,
    const nay_shared_packet *packet
);

typedef struct nay_handler_hook {
    void *instance;
    void **original_vtable;
    void *replacement_vtable[2];
    nay_receive_fn original_receive;
} nay_handler_hook;

static const unsigned char g_factory_pattern[] = {
    0x56, 0x57, 0x53, 0x48, 0x81, 0xEC, 0xF0, 0x00,
    0x00, 0x00, 0x48, 0x89, 0xCE, 0x8B, 0x05
};

static nay_text_factory_fn g_text_factory;
static nay_receive_fn g_text_receive;
static void *g_text_handler;
static void *g_handler_base;
static void *g_network_identifier;
static void *g_net_event_callback;
static nay_handler_hook g_hooks[MAX_PACKET_ID + 1];
static volatile LONG g_notice_sent;
static volatile LONG g_notice_running;
static HANDLE g_notice_event;
static nay_packet_observer g_packet_observer;

static void restore_handler_hooks(bool keep_observer);

static bool executable(const void *pointer)
{
    MEMORY_BASIC_INFORMATION info;
    DWORD protect;

    if (!pointer || !VirtualQuery(pointer, &info, sizeof(info))) return false;
    protect = info.Protect & 0xFFu;
    return info.State == MEM_COMMIT && !(info.Protect & PAGE_GUARD)
        && (protect == PAGE_EXECUTE || protect == PAGE_EXECUTE_READ
            || protect == PAGE_EXECUTE_READWRITE
            || protect == PAGE_EXECUTE_WRITECOPY);
}

static bool readable(const void *pointer, SIZE_T size)
{
    MEMORY_BASIC_INFORMATION info;
    uintptr_t begin = (uintptr_t)pointer;
    uintptr_t end;

    if (!pointer || !size || begin > UINTPTR_MAX - size
        || !VirtualQuery(pointer, &info, sizeof(info)))
        return false;
    end = (uintptr_t)info.BaseAddress + info.RegionSize;
    return info.State == MEM_COMMIT
        && !(info.Protect & (PAGE_GUARD | PAGE_NOACCESS))
        && begin + size <= end;
}

static bool valid_callback(const void *callback)
{
    void *vtable;

    if (!readable(callback, sizeof(void *))) return false;
    vtable = *(void **)callback;
    return readable(vtable, sizeof(void *)) && executable(*(void **)vtable);
}

static bool module_text(unsigned char **begin, size_t *size)
{
    HMODULE module = GetModuleHandleW(L"Minecraft.Windows.exe");
    IMAGE_DOS_HEADER *dos;
    IMAGE_NT_HEADERS64 *nt;
    IMAGE_SECTION_HEADER *section;
    unsigned i;

    if (!module || !begin || !size) return false;
    dos = (IMAGE_DOS_HEADER *)module;
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) return false;
    nt = (IMAGE_NT_HEADERS64 *)((unsigned char *)module + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE) return false;
    section = IMAGE_FIRST_SECTION(nt);
    for (i = 0; i < nt->FileHeader.NumberOfSections; ++i) {
        if ((section[i].Characteristics & IMAGE_SCN_MEM_EXECUTE) != 0) {
            *begin = (unsigned char *)module + section[i].VirtualAddress;
            *size = section[i].Misc.VirtualSize;
            return true;
        }
    }
    return false;
}

static void *find_unique(const unsigned char *pattern, size_t pattern_size)
{
    unsigned char *begin;
    size_t size;
    size_t i;
    void *match = NULL;

    if (!module_text(&begin, &size) || pattern_size > size) return NULL;
    for (i = 0; i <= size - pattern_size; ++i) {
        if (memcmp(begin + i, pattern, pattern_size) == 0) {
            if (match) return NULL;
            match = begin + i;
        }
    }
    return match;
}

static bool write_pointer(void *address, void *value)
{
    DWORD old_protect;
    DWORD ignored;

    if (!VirtualProtect(address, sizeof(value), PAGE_READWRITE, &old_protect))
        return false;
    memcpy(address, &value, sizeof(value));
    FlushInstructionCache(GetCurrentProcess(), address, sizeof(value));
    return VirtualProtect(address, sizeof(value), old_protect, &ignored) != FALSE;
}

static bool set_packet_string(void *address, const char *text)
{
    size_t length = strlen(text);
    unsigned char *string = (unsigned char *)address;
    char *storage;

    memset(string, 0, 32);
    if (length <= 15) {
        memcpy(string, text, length);
        *(size_t *)(string + 24) = 15;
    } else {
        storage = HeapAlloc(GetProcessHeap(), 0, length + 1);
        if (!storage) return false;
        memcpy(storage, text, length + 1);
        *(char **)string = storage;
        *(size_t *)(string + 24) = length;
    }
    *(size_t *)(string + 16) = length;
    return true;
}

static bool dispatch_text(
    void *network_identifier,
    void *net_event_callback,
    const char *text
)
{
    nay_shared_packet packet = {0};

    g_text_factory(&packet);
    if (packet.packet && packet.control) {
        *(unsigned char *)((unsigned char *)packet.packet + TEXT_TYPE_OFFSET) = 6;
        if (!set_packet_string(
                (unsigned char *)packet.packet + TEXT_STRING_OFFSET,
                text
            )) {
            return false;
        }
        g_text_receive(
            g_text_handler,
            network_identifier,
            net_event_callback,
            &packet
        );
        /* A single native packet is intentionally retained: Minecraft's
           custom shared_ptr allocator cannot be released by the DLL CRT. */
        return true;
    }
    return false;
}

static void send_notice(
    void *network_identifier,
    void *net_event_callback
)
{
    if (InterlockedCompareExchange(&g_notice_running, 1, 0) != 0) return;
    if (InterlockedCompareExchange(&g_notice_sent, 0, 0) != 0) {
        InterlockedExchange(&g_notice_running, 0);
        return;
    }

    if (dispatch_text(
            network_identifier,
            net_event_callback,
            "\xC2\xA7" "c[NayClient] "
            "\xC2\xA7" "7Successfully injected!"
        )) {
        g_network_identifier = network_identifier;
        g_net_event_callback = net_event_callback;
        InterlockedExchange(&g_notice_sent, 1);
        if (g_notice_event) SetEvent(g_notice_event);
        restore_handler_hooks(true);
    }
    InterlockedExchange(&g_notice_running, 0);
}

static void __fastcall receive_hook(
    void *handler,
    void *network_identifier,
    void *net_event_callback,
    const nay_shared_packet *packet
)
{
    nay_receive_fn original = NULL;
    uintptr_t delta;
    unsigned index = MAX_PACKET_ID + 1;

    if ((uintptr_t)handler >= (uintptr_t)g_handler_base) {
        delta = (uintptr_t)handler - (uintptr_t)g_handler_base;
        index = (unsigned)(delta / sizeof(void *));
        if ((delta % sizeof(void *)) == 0 && index <= MAX_PACKET_ID
            && g_hooks[index].instance == handler)
            original = g_hooks[index].original_receive;
    }
    if (!original) return;

    original(handler, network_identifier, net_event_callback, packet);
    if (g_packet_observer && packet && packet->packet)
        g_packet_observer(index, packet->packet);
    send_notice(network_identifier, net_event_callback);
}

static bool install_hook(nay_handler_hook *hook, void *instance)
{
    void **vtable;

    if (!hook || !readable(instance, sizeof(void *))) return false;
    vtable = *(void ***)instance;
    if (!readable(vtable, 2 * sizeof(void *))
        || !executable(vtable[0]) || !executable(vtable[1])) return false;
    hook->instance = instance;
    hook->original_vtable = vtable;
    hook->original_receive = (nay_receive_fn)vtable[1];
    hook->replacement_vtable[0] = vtable[0];
    hook->replacement_vtable[1] = receive_hook;
    if (!write_pointer(instance, hook->replacement_vtable)) {
        memset(hook, 0, sizeof(*hook));
        return false;
    }
    return true;
}

static void uninstall_hook(nay_handler_hook *hook)
{
    if (!hook || !hook->instance || !hook->original_vtable) return;
    (void)write_pointer(hook->instance, hook->original_vtable);
    memset(hook, 0, sizeof(*hook));
}

static void restore_handler_hooks(bool keep_observer)
{
    unsigned i;
    for (i = 0; i < sizeof(g_hooks) / sizeof(g_hooks[0]); ++i) {
        if (keep_observer && g_packet_observer
            && i == PLAYER_LIST_HANDLER_INDEX)
            continue;
        uninstall_hook(&g_hooks[i]);
    }
}

bool nay_injection_notice_install(void)
{
    nay_shared_packet packet = {0};
    WCHAR event_name[96];
    unsigned installed = 0;
    unsigned i;

    if (g_text_factory) return true;
    g_text_factory = (nay_text_factory_fn)find_unique(
        g_factory_pattern,
        sizeof(g_factory_pattern)
    );
    if (!g_text_factory) return false;
    swprintf_s(
        event_name,
        96,
        L"Local\\NayClient.InjectionNotice.%lu",
        GetCurrentProcessId()
    );
    g_notice_event = CreateEventW(NULL, TRUE, FALSE, event_name);
    if (!g_notice_event) goto fail;

    g_text_factory(&packet);
    if (!packet.packet || !packet.control) goto fail;
    g_text_handler = *(void **)((unsigned char *)packet.packet + PACKET_HANDLER_OFFSET);
    if (!g_text_handler) goto fail;
    g_text_receive = (nay_receive_fn)(*(void ***)g_text_handler)[1];
    if (!executable((void *)g_text_receive)) goto fail;

    g_handler_base = (unsigned char *)g_text_handler
        - TEXT_PACKET_ID * sizeof(void *);
    for (i = 0; i <= MAX_PACKET_ID; ++i) {
        uintptr_t handler_address = (uintptr_t)g_handler_base
            + i * sizeof(void *);
        if (install_hook(&g_hooks[i], (void *)handler_address)) ++installed;
    }
    if (!installed) goto fail;
    return true;

fail:
    nay_injection_notice_uninstall();
    return false;
}

void nay_injection_notice_uninstall(void)
{
    restore_handler_hooks(false);
    if (g_text_factory && g_text_receive && g_text_handler
        && valid_callback(g_net_event_callback)) {
        (void)dispatch_text(
            g_network_identifier,
            g_net_event_callback,
            "\xC2\xA7" "c[NayClient] "
            "\xC2\xA7" "7Successfully unloaded!"
        );
    }
    g_text_factory = NULL;
    g_text_receive = NULL;
    g_text_handler = NULL;
    g_handler_base = NULL;
    g_network_identifier = NULL;
    g_net_event_callback = NULL;
    if (g_notice_event) CloseHandle(g_notice_event);
    g_notice_event = NULL;
    InterlockedExchange(&g_notice_running, 0);
}

bool nay_injection_notice_sent(void)
{
    return InterlockedCompareExchange(&g_notice_sent, 0, 0) != 0;
}

bool nay_injection_notice_send_text(const char *text)
{
    return text && g_text_factory && g_text_receive && g_text_handler
        && valid_callback(g_net_event_callback)
        && dispatch_text(g_network_identifier, g_net_event_callback, text);
}

void nay_injection_notice_set_packet_observer(nay_packet_observer observer)
{
    g_packet_observer = observer;
}
