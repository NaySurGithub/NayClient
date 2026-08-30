#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>

#include "nay/launcher/command.h"
#include "nay/launcher/console.h"
#include "nay/platform/minecraft_version.h"

int main(void)
{
    nay_launcher_state state = {0};
    nay_version version;

    SetConsoleTitleW(L"NayClient");
    nay_console_init();
    nay_console_banner();

    if (!nay_minecraft_windows_version(&version)) {
        nay_show_unsupported_version_popup(NULL);
        return 1;
    }

    state.version_supported = nay_minecraft_version_supported(&version);
    if (!state.version_supported) {
        nay_show_unsupported_version_popup(&version);
        return 1;
    }

    printf(NAY_COLOR_DIM "  minecraft  " NAY_COLOR_RESET "%u.%u.%u.%u\n",
        version.major,
        version.minor,
        version.build,
        version.revision
    );
    state.injection_ready = nay_inject_client(&state.injection);
    printf(NAY_COLOR_DIM "  injection  " NAY_COLOR_RESET "%s%s" NAY_COLOR_RESET "\n\n",
        state.injection_ready ? NAY_COLOR_GREEN : NAY_COLOR_RED,
        state.injection_ready ? "ready" : "failed");
    if (!state.injection_ready) return 1;
    puts(NAY_COLOR_DIM "  type 'help' for commands\n" NAY_COLOR_RESET);

    nay_command_loop(&state);
    return 0;
}
