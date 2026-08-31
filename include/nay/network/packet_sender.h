#ifndef NAY_PACKET_SENDER_H
#define NAY_PACKET_SENDER_H

#include <stdbool.h>
#include <stddef.h>

#define NAY_PACKET_SENDER_OFFSET_1_26 ((size_t)0x1C8u)
#define NAY_PACKET_SEND_VTABLE_INDEX 1u
#define NAY_PACKET_SEND_TO_SERVER_VTABLE_INDEX 2u

typedef struct nay_packet_sender {
    void *client_instance;
    void *sender;
    size_t sender_offset;
} nay_packet_sender;

void nay_packet_sender_init(nay_packet_sender *self, size_t sender_offset);
bool nay_packet_sender_bind(nay_packet_sender *self, void *client_instance);
void nay_packet_sender_reset(nay_packet_sender *self);
bool nay_packet_sender_ready(const nay_packet_sender *self);
bool nay_packet_sender_send(nay_packet_sender *self, void *packet);
bool nay_packet_sender_send_to_server(nay_packet_sender *self, void *packet);

#endif
