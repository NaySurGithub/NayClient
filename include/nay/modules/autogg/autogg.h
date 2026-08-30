#ifndef NAY_AUTOGG_H
#define NAY_AUTOGG_H

#include <stdbool.h>
#include "nay/modules/module/module.h"

typedef struct nay_autogg {
    nay_module module;
    bool installed;
} nay_autogg;

void nay_autogg_init(nay_autogg *self, bool enabled);
bool nay_autogg_toggle(nay_autogg *self);
void nay_autogg_set_message(const char *text);

#endif
