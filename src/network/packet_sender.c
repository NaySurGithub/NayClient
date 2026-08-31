#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>
#include <string.h>

#include "nay/network/packet_sender.h"

typedef void (__fastcall *nay_send_packet_fn)(void *sender, void *packet);

static bool readable(const void *pointer, SIZE_T size)
{
    MEMORY_BASIC_INFORMATION info;
    uintptr_t begin = (uintptr_t)pointer;
    uintptr_t end;

    if (!pointer || size == 0 || begin > UINTPTR_MAX - size) return false;
    if (!VirtualQuery(pointer, &info, sizeof(info))) return false;
    end = (uintptr_t)info.BaseAddress + info.RegionSize;
    return info.State == MEM_COMMIT
        && !(info.Protect & (PAGE_GUARD | PAGE_NOACCESS))
        && begin + size <= end;
}

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

static void *read_pointer(const void *address)
{
    void *value = NULL;
    if (!readable(address, sizeof(value))) return NULL;
    memcpy(&value, address, sizeof(value));
    return value;
}

static void *resolve_sender(void *client_instance, size_t offset)
{
    uintptr_t address;

    if (!client_instance || (uintptr_t)client_instance > UINTPTR_MAX - offset)
        return NULL;
    address = (uintptr_t)client_instance + offset;
    return read_pointer((const void *)address);
}

static nay_send_packet_fn resolve_method(void *sender, unsigned index)
{
    void *vtable;
    void *method;

    if (!readable(sender, sizeof(void *))) return NULL;
    vtable = read_pointer(sender);
    if (!vtable || index > (UINTPTR_MAX / sizeof(void *))) return NULL;
    method = read_pointer((const unsigned char *)vtable + index * sizeof(void *));
    return executable(method) ? (nay_send_packet_fn)method : NULL;
}

static bool valid_packet(void *packet)
{
    void *vtable;
    void *virtual_method;

    if (!readable(packet, sizeof(void *))) return false;
    vtable = read_pointer(packet);
    if (!vtable) return false;
    virtual_method = read_pointer(vtable);
    return executable(virtual_method);
}

static bool dispatch(nay_packet_sender *self, void *packet, unsigned index)
{
    nay_send_packet_fn method;
    void *current;

    if (!self || !self->client_instance || !valid_packet(packet)) return false;
    current = resolve_sender(self->client_instance, self->sender_offset);
    if (!current) {
        self->sender = NULL;
        return false;
    }
    self->sender = current;
    method = resolve_method(current, index);
    if (!method) return false;
    method(current, packet);
    return true;
}

void nay_packet_sender_init(nay_packet_sender *self, size_t sender_offset)
{
    if (!self) return;
    memset(self, 0, sizeof(*self));
    self->sender_offset = sender_offset;
}

bool nay_packet_sender_bind(nay_packet_sender *self, void *client_instance)
{
    void *sender;

    if (!self || !client_instance || !readable(client_instance, sizeof(void *)))
        return false;
    sender = resolve_sender(client_instance, self->sender_offset);
    if (!sender || !resolve_method(sender, NAY_PACKET_SEND_VTABLE_INDEX)
        || !resolve_method(sender, NAY_PACKET_SEND_TO_SERVER_VTABLE_INDEX))
        return false;
    self->client_instance = client_instance;
    self->sender = sender;
    return true;
}

void nay_packet_sender_reset(nay_packet_sender *self)
{
    if (!self) return;
    self->client_instance = NULL;
    self->sender = NULL;
}

bool nay_packet_sender_ready(const nay_packet_sender *self)
{
    void *sender;

    if (!self || !self->client_instance) return false;
    sender = resolve_sender(self->client_instance, self->sender_offset);
    return sender && resolve_method(sender, NAY_PACKET_SEND_VTABLE_INDEX)
        && resolve_method(sender, NAY_PACKET_SEND_TO_SERVER_VTABLE_INDEX);
}

bool nay_packet_sender_send(nay_packet_sender *self, void *packet)
{
    return dispatch(self, packet, NAY_PACKET_SEND_VTABLE_INDEX);
}

bool nay_packet_sender_send_to_server(nay_packet_sender *self, void *packet)
{
    return dispatch(self, packet, NAY_PACKET_SEND_TO_SERVER_VTABLE_INDEX);
}
