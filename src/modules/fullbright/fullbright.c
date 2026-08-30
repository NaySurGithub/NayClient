#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>
#include <string.h>
#include "nay/modules/fullbright/fullbright.h"

#define GAMMA_VTABLE_RVA ((uintptr_t)0xE764080u)
#define GAMMA_ID 0x35u
#define VALUE_OFFSET 0x18u
#define META_ID_OFFSET 0x1CCu
static float *gamma_value;
static bool readable(const void *pointer, SIZE_T size)
{
    MEMORY_BASIC_INFORMATION info;
    uintptr_t begin = (uintptr_t)pointer;
    uintptr_t end;
    if (!pointer || !VirtualQuery(pointer, &info, sizeof(info))) return false;
    end = (uintptr_t)info.BaseAddress + info.RegionSize;
    return info.State == MEM_COMMIT && !(info.Protect & (PAGE_GUARD | PAGE_NOACCESS))
        && begin <= UINTPTR_MAX - size && begin + size <= end;
}
static bool has_text(const unsigned char *data, SIZE_T size, const char *text)
{
    SIZE_T length = strlen(text);
    SIZE_T index;
    for (index = 0; index + length <= size; ++index)
        if (!memcmp(data + index, text, length)) return true;
    return false;
}
static bool valid_option(uintptr_t object, uintptr_t vtable)
{
    uintptr_t metadata;
    if (!readable((void *)object, 0x28) || *(uintptr_t *)object != vtable) return false;
    metadata = *(uintptr_t *)(object + 8);
    if (!readable((void *)metadata, 0x2A0)) return false;
    if (*(uint32_t *)(metadata + META_ID_OFFSET) != GAMMA_ID) return false;
    return has_text((unsigned char *)metadata, 0x2A0, "gamma")
        && has_text((unsigned char *)metadata, 0x2A0, "gfx_gamma")
        && has_text((unsigned char *)metadata, 0x2A0, "options.gamma");
}

static bool locate_gamma(void)
{
    SYSTEM_INFO system;
    MEMORY_BASIC_INFORMATION info;
    uintptr_t address;
    uintptr_t vtable = (uintptr_t)GetModuleHandleW(L"Minecraft.Windows.exe") + GAMMA_VTABLE_RVA;
    if (gamma_value && readable(gamma_value, sizeof(*gamma_value))) return true;
    GetSystemInfo(&system);
    address = (uintptr_t)system.lpMinimumApplicationAddress;
    while (address < (uintptr_t)system.lpMaximumApplicationAddress
        && VirtualQuery((void *)address, &info, sizeof(info))) {
        uintptr_t begin = (uintptr_t)info.BaseAddress;
        uintptr_t end = begin + info.RegionSize;
        if (info.State == MEM_COMMIT && !(info.Protect & (PAGE_GUARD | PAGE_NOACCESS))) {
            uintptr_t cursor;
            for (cursor = begin; cursor + sizeof(uintptr_t) <= end; cursor += sizeof(uintptr_t)) {
                if (*(uintptr_t *)cursor == vtable && valid_option(cursor, vtable)) {
                    gamma_value = (float *)(cursor + VALUE_OFFSET);
                    return true;
                }
            }
        }
        if (end <= address) break;
        address = end;
    }
    return false;
}

static float clamp(float value)
{
    if (value < 0.0f) return 0.0f;
    return value;
}

static bool enable(nay_module *base)
{
    nay_fullbright *self = (nay_fullbright *)base->context;
    if (!locate_gamma()) return false;
    self->previous_level = *gamma_value;
    *gamma_value = 9999.0f;
    self->applied = true;
    return true;
}

static bool disable(nay_module *base)
{
    nay_fullbright *self = (nay_fullbright *)base->context;
    if (self->applied && locate_gamma()) *gamma_value = self->previous_level;
    self->applied = false;
    return true;
}

static bool tick(nay_module *base)
{
    (void)base;
    if (!locate_gamma()) return false;
    *gamma_value = 9999.0f;
    return true;
}

void nay_fullbright_init(nay_fullbright *self, bool enabled, float level)
{
    if (!self) return;
    (void)level;
    self->level = 9999.0f; self->previous_level = 0.0f; self->applied = false;
    nay_module_init(&self->module, "fullbright", self, enable, disable, tick);
    if (enabled) (void)nay_module_set_enabled(&self->module, true);
}
void nay_fullbright_update(nay_fullbright *self) { if (self) (void)nay_module_tick(&self->module); }
void nay_fullbright_shutdown(nay_fullbright *self) { if (self) nay_module_shutdown(&self->module); }
void nay_fullbright_enable(nay_fullbright *self, bool value) { if (self) (void)nay_module_set_enabled(&self->module, value); }
void nay_fullbright_level(nay_fullbright *self, float value) { if (self) self->level = clamp(value); }
bool nay_fullbright_toggle(nay_fullbright *self) { return self && nay_module_toggle(&self->module); }
bool nay_fullbright_enabled(const nay_fullbright *self) { return self && self->module.enabled; }
