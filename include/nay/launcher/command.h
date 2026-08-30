#ifndef NAY_COMMAND_H
#define NAY_COMMAND_H

#include <stdbool.h>
#include "nay/launcher/injector.h"

typedef struct nay_launcher_state {
    bool running;
    bool version_supported;
    bool injection_ready;
    bool fullbright_enabled;
    bool nofire_enabled;
    nay_injection injection;
} nay_launcher_state;

void nay_command_loop(nay_launcher_state *state);

#endif
