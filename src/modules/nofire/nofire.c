#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>
#include <string.h>
#include "nay/modules/nofire/nofire.h"

static const unsigned char pattern[] = {
    0x4C,0x8B,0x75,0x18,0x49,0x8B,0x06,0x48,0x8B,0x80,0x78,0x01,0,0,
    0x4C,0x89,0xF1,0xFF,0x15,0,0,0,0,0x84,0xC0,0x0F,0x84,0,0,0,0
};
static const unsigned char mask[] = {
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,1,1,1,1,0,0,0,0
};

static unsigned char *locate_branch(void)
{
    HMODULE module = GetModuleHandleW(L"Minecraft.Windows.exe");
    IMAGE_DOS_HEADER *dos = (IMAGE_DOS_HEADER *)module;
    IMAGE_NT_HEADERS *nt;
    MEMORY_BASIC_INFORMATION info;
    SIZE_T index, inner, size, offset = 0;
    unsigned char *base = (unsigned char *)module;
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
                if (inner == sizeof(pattern)) {
                    unsigned char *branch = base + index + 25;
                    int32_t displacement;
                    memcpy(&displacement, branch + 2, sizeof(displacement));
                    if (branch[0] == 0x0F && branch[1] == 0x84
                        && branch + 6 + displacement > branch) return branch;
                }
            }
        }
        if (end <= offset) break;
        offset = end;
    }
    return NULL;
}

static bool write_bytes(void *address, const void *bytes, SIZE_T size)
{
    DWORD old_protect, ignored;
    if (!VirtualProtect(address, size, PAGE_EXECUTE_READWRITE, &old_protect)) return false;
    memcpy(address, bytes, size);
    FlushInstructionCache(GetCurrentProcess(), address, size);
    return VirtualProtect(address, size, old_protect, &ignored) != FALSE;
}

static bool enable(nay_module *base)
{
    nay_nofire *self = (nay_nofire *)base->context;
    unsigned char patch[6] = {0xE9,0,0,0,0,0x90};
    int32_t old_displacement, new_displacement;
    self->branch = locate_branch();
    if (!self->branch) return false;
    memcpy(self->original, self->branch, 6);
    memcpy(&old_displacement, self->branch + 2, 4);
    new_displacement = old_displacement + 1;
    memcpy(patch + 1, &new_displacement, 4);
    self->patched = write_bytes(self->branch, patch, sizeof(patch));
    return self->patched;
}

static bool disable(nay_module *base)
{
    nay_nofire *self = (nay_nofire *)base->context;
    if (self->patched && !write_bytes(self->branch, self->original, 6)) return false;
    self->patched = false;
    self->branch = NULL;
    return true;
}

void nay_nofire_init(nay_nofire *self, bool enabled)
{
    if (!self) return;
    memset(self, 0, sizeof(*self));
    nay_module_init(&self->module, "nofire", self, enable, disable, NULL);
    if (enabled) (void)nay_module_set_enabled(&self->module, true);
}

bool nay_nofire_toggle(nay_nofire *self)
{
    return self && nay_module_toggle(&self->module);
}
