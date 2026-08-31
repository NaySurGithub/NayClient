#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>
#include <string.h>
#include "nay/modules/nohurtcam/nohurtcam.h"

static const unsigned char pattern[] = {
    0x76,0x0E,0xF3,0x0F,0x5C,0x00,0xF3,0x0F,0x58,0x00,
    0x00,0x00,0x00,0x00,0xEB,0x08,0xF3,0x0F,0x58
};
static const unsigned char mask[] = {
    1,1,1,1,1,0,1,1,1,0,0,0,0,0,1,1,1,1,1
};

#define GUARD_OFFSET 0u
#define GUARD_ORIGINAL 0x76u
#define GUARD_PATCH 0xEBu

static unsigned char *locate_guard(void)
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
            for (index = begin; index + sizeof(pattern) <= end; ++index) {
                for (inner = 0; inner < sizeof(pattern); ++inner)
                    if (mask[inner] && base[index + inner] != pattern[inner]) break;
                if (inner == sizeof(pattern)) return base + index + GUARD_OFFSET;
            }
        }
        if (end <= offset) break;
        offset = end;
    }
    return NULL;
}

static bool write_byte(void *address, unsigned char value)
{
    DWORD old, ignored;
    if (!VirtualProtect(address, 1, PAGE_EXECUTE_READWRITE, &old)) return false;
    *(unsigned char *)address = value;
    FlushInstructionCache(GetCurrentProcess(), address, 1);
    return VirtualProtect(address, 1, old, &ignored) != FALSE;
}

static bool enable(nay_module *base)
{
    nay_nohurtcam *self = (nay_nohurtcam *)base->context;
    self->site = locate_guard();
    if (!self->site || *self->site != GUARD_ORIGINAL) return false;
    self->original = *self->site;
    self->patched = write_byte(self->site, GUARD_PATCH);
    return self->patched;
}

static bool disable(nay_module *base)
{
    nay_nohurtcam *self = (nay_nohurtcam *)base->context;
    if (self->patched && self->site && !write_byte(self->site, self->original)) return false;
    self->patched = false;
    self->site = NULL;
    return true;
}

void nay_nohurtcam_init(nay_nohurtcam *self, bool enabled)
{
    if (!self) return;
    memset(self, 0, sizeof(*self));
    nay_module_init(&self->module, "nohurtcam", self, enable, disable, NULL);
    if (enabled) (void)nay_module_set_enabled(&self->module, true);
}

bool nay_nohurtcam_toggle(nay_nohurtcam *self)
{
    return self && nay_module_toggle(&self->module);
}
