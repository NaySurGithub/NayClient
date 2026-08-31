#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "nay/modules/playernotifier/playernotifier.h"
#include "nay/network/injection_notice.h"

#define PLAYER_LIST_HANDLER_INDEX 66u
#define PLAYER_LIST_VECTOR_OFFSET 0x30u
#define PLAYER_LIST_ENTRY_SIZE 184u
#define PLAYER_LIST_VARIANT_TAG_OFFSET 176u
#define PLAYER_LIST_UUID_OFFSET 8u
#define PLAYER_LIST_NAME_OFFSET 64u

static nay_playernotifier *g_playernotifier;

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

static bool read_msvc_string(const void *value, char output[65])
{
    const unsigned char *string = value;
    const char *data;
    size_t length;
    size_t capacity;

    if (!readable(string, 32)) return false;
    length = *(const size_t *)(string + 16);
    capacity = *(const size_t *)(string + 24);
    if (!length || length > 64 || capacity < length) return false;
    data = capacity < 16 ? (const char *)string : *(const char *const *)string;
    if (!readable(data, length)) return false;
    memcpy(output, data, length);
    output[length] = '\0';
    return true;
}

static int find_uuid(nay_playernotifier *self, const unsigned char uuid[16])
{
    unsigned i;
    for (i = 0; i < NAY_PLAYERNOTIFIER_MAX_PLAYERS; ++i)
        if (self->players[i].used
            && memcmp(self->players[i].uuid, uuid, 16) == 0)
            return (int)i;
    return -1;
}

static int free_slot(nay_playernotifier *self)
{
    unsigned i;
    for (i = 0; i < NAY_PLAYERNOTIFIER_MAX_PLAYERS; ++i)
        if (!self->players[i].used) return (int)i;
    return -1;
}

static void notify(const char *name, bool joined)
{
    char message[192];
    _snprintf_s(
        message,
        sizeof(message),
        _TRUNCATE,
        joined
            ? "\xC2\xA7" "c[PlayerNotifier] " "\xC2\xA7" "f%s " "\xC2\xA7" "ajoined."
            : "\xC2\xA7" "c[PlayerNotifier] " "\xC2\xA7" "f%s " "\xC2\xA7" "cleft.",
        name
    );
    (void)nay_injection_notice_send_text(message);
}

static bool enable(nay_module *module)
{
    nay_playernotifier *self = module ? module->context : NULL;
    if (!self) return false;
    self->initialized = false;
    memset(self->players, 0, sizeof(self->players));
    return true;
}

static bool disable(nay_module *module)
{
    nay_playernotifier *self = module ? module->context : NULL;
    if (!self) return false;
    self->initialized = false;
    memset(self->players, 0, sizeof(self->players));
    return true;
}

void nay_playernotifier_init(nay_playernotifier *self, bool enabled)
{
    if (!self) return;
    memset(self, 0, sizeof(*self));
    nay_module_init(&self->module, "PlayerNotifier", self, enable, disable, NULL);
    g_playernotifier = self;
    (void)nay_module_set_enabled(&self->module, enabled);
}

bool nay_playernotifier_toggle(nay_playernotifier *self)
{
    return self && nay_module_toggle(&self->module);
}

void nay_playernotifier_on_packet(unsigned handler_index, const void *packet)
{
    nay_playernotifier *self = g_playernotifier;
    const unsigned char *begin;
    const unsigned char *end;
    const unsigned char *entry;
    size_t count;

    if (!self || !self->module.enabled
        || handler_index != PLAYER_LIST_HANDLER_INDEX
        || !readable((const unsigned char *)packet + PLAYER_LIST_VECTOR_OFFSET, 24))
        return;
    begin = *(const unsigned char *const *)((const unsigned char *)packet + PLAYER_LIST_VECTOR_OFFSET);
    end = *(const unsigned char *const *)((const unsigned char *)packet + PLAYER_LIST_VECTOR_OFFSET + 8);
    if (!begin || end < begin
        || ((size_t)(end - begin) % PLAYER_LIST_ENTRY_SIZE) != 0)
        return;
    count = (size_t)(end - begin) / PLAYER_LIST_ENTRY_SIZE;
    if (count > NAY_PLAYERNOTIFIER_MAX_PLAYERS
        || (count && !readable(begin, count * PLAYER_LIST_ENTRY_SIZE)))
        return;

    for (entry = begin; entry < end; entry += PLAYER_LIST_ENTRY_SIZE) {
        const unsigned char *uuid = entry + PLAYER_LIST_UUID_OFFSET;
        unsigned char tag = entry[PLAYER_LIST_VARIANT_TAG_OFFSET];
        int slot = find_uuid(self, uuid);

        if (tag == 1) {
            char name[65];
            if (!read_msvc_string(entry + PLAYER_LIST_NAME_OFFSET, name)) continue;
            if (slot < 0) slot = free_slot(self);
            if (slot < 0) continue;
            if (self->initialized && !self->players[slot].used) notify(name, true);
            memcpy(self->players[slot].uuid, uuid, 16);
            strcpy_s(self->players[slot].name, sizeof(self->players[slot].name), name);
            self->players[slot].used = true;
        } else if (tag == 0 && slot >= 0) {
            if (self->initialized) notify(self->players[slot].name, false);
            memset(&self->players[slot], 0, sizeof(self->players[slot]));
        }
    }
    self->initialized = true;
}
