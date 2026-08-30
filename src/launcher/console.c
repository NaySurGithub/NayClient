#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include "nay/launcher/console.h"

static const char *logo[] = {
    "███╗   ██╗ █████╗ ██╗   ██╗ ██████╗██╗     ██╗███████╗███╗   ██╗████████╗",
    "████╗  ██║██╔══██╗╚██╗ ██╔╝██╔════╝██║     ██║██╔════╝████╗  ██║╚══██╔══╝",
    "██╔██╗ ██║███████║ ╚████╔╝ ██║     ██║     ██║█████╗  ██╔██╗ ██║   ██║   ",
    "██║╚██╗██║██╔══██║  ╚██╔╝  ██║     ██║     ██║██╔══╝  ██║╚██╗██║   ██║   ",
    "██║ ╚████║██║  ██║   ██║   ╚██████╗███████╗██║███████╗██║ ╚████║   ██║   ",
    "╚═╝  ╚═══╝╚═╝  ╚═╝   ╚═╝    ╚═════╝╚══════╝╚═╝╚══════╝╚═╝  ╚═══╝   ╚═╝   "
};

void nay_console_init(void)
{
    HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode;
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    if (output != INVALID_HANDLE_VALUE && GetConsoleMode(output, &mode))
        SetConsoleMode(output, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
}

void nay_console_banner(void)
{
    unsigned index;
    fputs("\n" NAY_COLOR_LIGHT_RED, stdout);
    for (index = 0; index < sizeof(logo) / sizeof(logo[0]); ++index)
        printf("  %s\n", logo[index]);
    fputs(NAY_COLOR_RESET "\n  " NAY_COLOR_DIM, stdout);
    fputs("NayClient v0.1.0  ·  Minecraft Bedrock internal client", stdout);
    fputs(NAY_COLOR_RESET "\n\n", stdout);
}

void nay_console_prompt(void)
{
    fputs(NAY_COLOR_RED "nayclient" NAY_COLOR_GREY " > " NAY_COLOR_RESET, stdout);
    fflush(stdout);
}

void nay_console_clear(void)
{
    fputs("\x1b[2J\x1b[H", stdout);
    nay_console_banner();
}
