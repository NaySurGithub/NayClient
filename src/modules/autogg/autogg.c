#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>
#include <string.h>
#include "nay/modules/autogg/autogg.h"

#define SIG_SEND_CHAT \
    "48 89 5C 24 ?? 55 56 57 41 54 41 55 41 56 41 57 48 8D AC 24 ?? ?? ?? ?? " \
    "48 81 EC ?? ?? ?? ?? 48 8B 05 ?? ?? ?? ?? 48 33 C4 48 89 85 ?? ?? ?? ?? " \
    "4C 8B EA 4C 8B F9 48 8B 49"
#define SIG_CLIENT_PTR   "48 8D 0D ?? ?? ?? ?? 89 1C B9"
#define SIG_AWARD_KILL   "40 55 53 56 57 41 56 48 8D 6C 24 ?? 48 81 EC ?? ?? ?? ??"
#define LOCALPLAYER_OFF  ((uintptr_t)0x218u)
#define STOLEN_BYTES     14u

typedef void (*send_chat_fn)(void *ctx, const char *message);
typedef int64_t (*award_kill_fn)(void *self, void *victim, int amount);

static send_chat_fn   g_send_chat;
static uintptr_t     *g_client_instance_ptr;
static void          *g_hook_target;
static unsigned char  g_original[32];
static void          *g_trampoline;
static char           g_message[64] = "gg";
static volatile LONG  g_sending;
static ULONGLONG      g_last_gg;

/* signature */
static SIZE_T parse_ida(const char *ida, unsigned char *pattern, unsigned char *mask, SIZE_T cap)
{
    SIZE_T count = 0;
    while (*ida && count < cap) {
        if (*ida == ' ') { ++ida; continue; }
        if (*ida == '?') {
            pattern[count] = 0; mask[count] = 0; ++count;
            while (*ida == '?') ++ida;
        } else {
            unsigned value = 0; int digits = 0;
            while (digits < 2 && ((*ida >= '0' && *ida <= '9') ||
                   (*ida >= 'A' && *ida <= 'F') || (*ida >= 'a' && *ida <= 'f'))) {
                char c = *ida++;
                value = (value << 4) |
                    (unsigned)(c <= '9' ? c - '0' : (c | 0x20) - 'a' + 10);
                ++digits;
            }
            pattern[count] = (unsigned char)value; mask[count] = 1; ++count;
        }
    }
    return count;
}

static unsigned char *scan_module(const unsigned char *pattern, const unsigned char *mask, SIZE_T len)
{
    HMODULE module = GetModuleHandleW(L"Minecraft.Windows.exe");
    IMAGE_DOS_HEADER *dos = (IMAGE_DOS_HEADER *)module;
    IMAGE_NT_HEADERS *nt;
    MEMORY_BASIC_INFORMATION info;
    unsigned char *base = (unsigned char *)module;
    SIZE_T size, offset = 0, index, inner;
    if (!module || dos->e_magic != IMAGE_DOS_SIGNATURE) return NULL;
    nt = (IMAGE_NT_HEADERS *)(base + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE) return NULL;
    size = nt->OptionalHeader.SizeOfImage;
    while (offset < size && VirtualQuery(base + offset, &info, sizeof(info))) {
        SIZE_T begin = (SIZE_T)((unsigned char *)info.BaseAddress - base);
        SIZE_T end = begin + info.RegionSize;
        if (end > size) end = size;
        if (info.State == MEM_COMMIT && (info.Protect & 0xF0)) {
            for (index = begin; index + len <= end; ++index) {
                for (inner = 0; inner < len; ++inner)
                    if (mask[inner] && base[index + inner] != pattern[inner]) break;
                if (inner == len) return base + index;
            }
        }
        if (end <= offset) break;
        offset = end;
    }
    return NULL;
}

static unsigned char *find_signature(const char *ida)
{
    unsigned char pattern[256], mask[256];
    SIZE_T len = parse_ida(ida, pattern, mask, sizeof(pattern));
    return len ? scan_module(pattern, mask, len) : NULL;
}

static uintptr_t *resolve_rip(unsigned char *lea_site, SIZE_T disp_offset)
{
    int32_t disp;
    if (!lea_site) return NULL;
    memcpy(&disp, lea_site + disp_offset, sizeof(disp));
    return (uintptr_t *)(lea_site + disp_offset + 4 + disp);
}

/* hook */
static void write_abs_jmp(unsigned char *at, void *destination)
{
    at[0] = 0xFF; at[1] = 0x25;
    memset(at + 2, 0, 4);
    memcpy(at + 6, &destination, sizeof(destination));
}

static bool hook_install(void *target, void *detour)
{
    DWORD old, ignored;
    unsigned char *bytes = (unsigned char *)target;
    unsigned char *tramp;
    if (!target || !detour || STOLEN_BYTES < 14 || STOLEN_BYTES > sizeof(g_original))
        return false;

    tramp = (unsigned char *)VirtualAlloc(NULL, STOLEN_BYTES + 14,
        MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!tramp) return false;

    memcpy(g_original, bytes, STOLEN_BYTES);
    memcpy(tramp, bytes, STOLEN_BYTES);
    write_abs_jmp(tramp + STOLEN_BYTES, bytes + STOLEN_BYTES);
    g_trampoline = tramp;

    if (!VirtualProtect(target, STOLEN_BYTES, PAGE_EXECUTE_READWRITE, &old)) {
        VirtualFree(tramp, 0, MEM_RELEASE); g_trampoline = NULL; return false;
    }
    write_abs_jmp(bytes, detour);
    if (STOLEN_BYTES > 14) memset(bytes + 14, 0x90, STOLEN_BYTES - 14);
    FlushInstructionCache(GetCurrentProcess(), target, STOLEN_BYTES);
    VirtualProtect(target, STOLEN_BYTES, old, &ignored);
    return true;
}

static void hook_remove(void)
{
    DWORD old, ignored;
    if (g_hook_target) {
        if (VirtualProtect(g_hook_target, STOLEN_BYTES, PAGE_EXECUTE_READWRITE, &old)) {
            memcpy(g_hook_target, g_original, STOLEN_BYTES);
            FlushInstructionCache(GetCurrentProcess(), g_hook_target, STOLEN_BYTES);
            VirtualProtect(g_hook_target, STOLEN_BYTES, old, &ignored);
        }
        g_hook_target = NULL;
    }
    if (g_trampoline) { VirtualFree(g_trampoline, 0, MEM_RELEASE); g_trampoline = NULL; }
}

/* core */
static void *local_player(void)
{
    if (!g_client_instance_ptr || !*g_client_instance_ptr) return NULL;
    return *(void **)(*g_client_instance_ptr + LOCALPLAYER_OFF);
}

static void send_gg(void)
{
    ULONGLONG now = GetTickCount64();
    if (!g_send_chat) return;
    if (now - g_last_gg < 1500) return;
    if (InterlockedExchange(&g_sending, 1)) return;
    g_last_gg = now;
    g_send_chat(*g_client_instance_ptr ? (void *)*g_client_instance_ptr : NULL, g_message);
    InterlockedExchange(&g_sending, 0);
}

#if defined(__GNUC__)
#  define NAY_MSABI __attribute__((ms_abi))
#else
#  define NAY_MSABI
#endif

static NAY_MSABI int64_t detour_award_kill(void *self, void *victim, int amount)
{
    award_kill_fn original = (award_kill_fn)g_trampoline;
    if (self && self == local_player()) send_gg();
    return original(self, victim, amount);
}

static bool resolve_all(void)
{
    unsigned char *chat, *client_lea, *kill;
    if (g_send_chat && g_client_instance_ptr && g_hook_target) return true;

    chat = find_signature(SIG_SEND_CHAT);
    client_lea = find_signature(SIG_CLIENT_PTR);
    kill = find_signature(SIG_AWARD_KILL);
    if (!chat || !client_lea || !kill) return false;

    g_send_chat = (send_chat_fn)chat;
    g_client_instance_ptr = resolve_rip(client_lea, 3);
    g_hook_target = kill;
    return g_send_chat && g_client_instance_ptr && g_hook_target;
}

/* module */
static bool enable(nay_module *base)
{
    nay_autogg *self = (nay_autogg *)base->context;
    if (!resolve_all()) return false;
    if (self->installed) return true;
    if (!hook_install(g_hook_target, (void *)detour_award_kill)) return false;
    self->installed = true;
    return true;
}

static bool disable(nay_module *base)
{
    nay_autogg *self = (nay_autogg *)base->context;
    if (self->installed) hook_remove();
    self->installed = false;
    return true;
}

void nay_autogg_init(nay_autogg *self, bool enabled)
{
    if (!self) return;
    memset(self, 0, sizeof(*self));
    nay_module_init(&self->module, "autogg", self, enable, disable, NULL);
    if (enabled) (void)nay_module_set_enabled(&self->module, true);
}

bool nay_autogg_toggle(nay_autogg *self)
{
    return self && nay_module_toggle(&self->module);
}

void nay_autogg_set_message(const char *text)
{
    if (!text) return;
    strncpy_s(g_message, sizeof(g_message), text, _TRUNCATE);
}
