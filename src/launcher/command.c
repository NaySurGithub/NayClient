#include "nay/launcher/command.h"
#include "nay/launcher/console.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

static void trim(char *text)
{
    char *start = text;
    size_t length;

    while (*start && isspace((unsigned char)*start)) ++start;
    if (start != text) memmove(text, start, strlen(start) + 1);

    length = strlen(text);
    while (length && isspace((unsigned char)text[length - 1])) {
        text[--length] = '\0';
    }
}

static void print_help(void)
{
    puts(NAY_COLOR_LIGHT_RED "\n  commands" NAY_COLOR_RESET);
    puts("    help          show available commands");
    puts("    status        show client and module state");
    puts("    fullbright    toggle Fullbright");
    puts("    nofire        toggle fire overlay");
    puts("    nohurtcam     toggle hurt camera shake");
    puts("    clear         clear the terminal");
    puts("    quit          disable modules and unload\n");
}

void nay_command_loop(nay_launcher_state *state)
{
    char command[256];

    if (!state) return;
    state->running = true;

    while (state->running) {
        nay_console_prompt();

        if (!fgets(command, sizeof(command), stdin)) break;
        trim(command);

        if (!strcmp(command, "help")) {
            print_help();
        } else if (!strcmp(command, "status")) {
            fputs(NAY_COLOR_LIGHT_RED "\n  client" NAY_COLOR_RESET "\n", stdout);
            printf(
                "    minecraft     %s\n    injection     %s\n    fullbright    %s%s" NAY_COLOR_RESET
                "\n    nofire        %s%s" NAY_COLOR_RESET
                "\n    nohurtcam     %s%s" NAY_COLOR_RESET "\n\n",
                state->version_supported ? "supported" : "unsupported",
                state->injection_ready ? "ready" : "unavailable",
                state->fullbright_enabled ? NAY_COLOR_GREEN : NAY_COLOR_RED,
                state->fullbright_enabled ? "enabled" : "disabled",
                state->nofire_enabled ? NAY_COLOR_GREEN : NAY_COLOR_RED,
                state->nofire_enabled ? "enabled" : "disabled",
                state->nohurtcam_enabled ? NAY_COLOR_GREEN : NAY_COLOR_RED,
                state->nohurtcam_enabled ? "enabled" : "disabled"
            );
        } else if (!strcmp(command, "fullbright")) {
            if (!nay_injection_toggle_fullbright(&state->injection)) {
                puts("Fullbright failed: start Minecraft or run NayClient with sufficient rights.");
            } else {
                state->fullbright_enabled = !state->fullbright_enabled;
                printf("  " NAY_COLOR_LIGHT_RED "fullbright" NAY_COLOR_RESET "  %s\n",
                    state->fullbright_enabled ? "enabled" : "disabled");
            }
        } else if (!strcmp(command, "nofire")) {
            if (!nay_injection_toggle_nofire(&state->injection)) {
                puts("NoFire failed.");
            } else {
                state->nofire_enabled = !state->nofire_enabled;
                printf("  " NAY_COLOR_LIGHT_RED "nofire" NAY_COLOR_RESET "  %s\n",
                    state->nofire_enabled ? "enabled" : "disabled");
            }
        } else if (!strcmp(command, "nohurtcam")) {
            if (!nay_injection_toggle_nohurtcam(&state->injection)) {
                puts("NoHurtCam failed.");
            } else {
                state->nohurtcam_enabled = !state->nohurtcam_enabled;
                printf("  " NAY_COLOR_LIGHT_RED "nohurtcam" NAY_COLOR_RESET "  %s\n",
                    state->nohurtcam_enabled ? "enabled" : "disabled");
            }
        } else if (!strcmp(command, "clear")) {
            nay_console_clear();
        } else if (!strcmp(command, "quit") || !strcmp(command, "exit")) {
            state->running = false;
        } else if (command[0] != '\0') {
            printf(NAY_COLOR_RED "  unknown command: %s" NAY_COLOR_RESET "\n", command);
        }
    }
    nay_injection_close(&state->injection);
}
