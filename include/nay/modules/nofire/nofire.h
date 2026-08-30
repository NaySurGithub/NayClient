#ifndef NAY_NOFIRE_H
#define NAY_NOFIRE_H
#include "nay/modules/module/module.h"
typedef struct nay_nofire {
    nay_module module;
    unsigned char *branch;
    unsigned char original[6];
    bool patched;
} nay_nofire;
void nay_nofire_init(nay_nofire *self, bool enabled);
bool nay_nofire_toggle(nay_nofire *self);
#endif
