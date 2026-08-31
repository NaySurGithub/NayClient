#ifndef NAY_PLAYERNOTIFIER_H
#define NAY_PLAYERNOTIFIER_H

#include "nay/modules/module/module.h"

#define NAY_PLAYERNOTIFIER_MAX_PLAYERS 256u

typedef struct nay_playernotifier_entry {
    unsigned char uuid[16];
    char name[65];
    bool used;
} nay_playernotifier_entry;

typedef struct nay_playernotifier {
    nay_module module;
    bool initialized;
    nay_playernotifier_entry players[NAY_PLAYERNOTIFIER_MAX_PLAYERS];
} nay_playernotifier;

void nay_playernotifier_init(nay_playernotifier *self, bool enabled);
bool nay_playernotifier_toggle(nay_playernotifier *self);
void nay_playernotifier_on_packet(unsigned handler_index, const void *packet);

#endif
