#ifndef NAY_NOHURTCAM_H
#define NAY_NOHURTCAM_H

#include <stdbool.h>
#include "nay/modules/module/module.h"

typedef struct nay_nohurtcam {
    nay_module module;
    unsigned char *site;
    unsigned char original;
    bool patched;
} nay_nohurtcam;

void nay_nohurtcam_init(nay_nohurtcam *self, bool enabled);
bool nay_nohurtcam_toggle(nay_nohurtcam *self);

#endif
