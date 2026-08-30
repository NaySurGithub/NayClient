#ifndef NAY_FULLBRIGHT_H
#define NAY_FULLBRIGHT_H

#include <stdbool.h>
#include "nay/modules/module/module.h"

typedef struct nay_fullbright {
    nay_module module;
    float level;
    float previous_level;
    bool applied;
} nay_fullbright;

void nay_fullbright_init(nay_fullbright *module, bool enabled, float level);
void nay_fullbright_update(nay_fullbright *module);
void nay_fullbright_shutdown(nay_fullbright *module);
void nay_fullbright_enable(nay_fullbright *module, bool enabled);
void nay_fullbright_level(nay_fullbright *module, float level);
bool nay_fullbright_toggle(nay_fullbright *module);
bool nay_fullbright_enabled(const nay_fullbright *module);

#endif
