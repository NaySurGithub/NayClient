#ifndef NAY_MINECRAFT_VERSION_H
#define NAY_MINECRAFT_VERSION_H

#include <stdbool.h>
#include <stdint.h>

typedef struct nay_version {
    uint16_t major;
    uint16_t minor;
    uint16_t build;
    uint16_t revision;
} nay_version;

bool nay_minecraft_windows_version(nay_version *version);
bool nay_minecraft_version_supported(const nay_version *version);
void nay_show_unsupported_version_popup(const nay_version *version);

#endif
