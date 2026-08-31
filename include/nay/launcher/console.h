#ifndef NAY_CONSOLE_H
#define NAY_CONSOLE_H

#define NAY_COLOR_RESET "\x1b[0m"
#define NAY_COLOR_RED "\x1b[38;5;196m"
#define NAY_COLOR_LIGHT_RED "\x1b[38;5;196m"
#define NAY_COLOR_DARK_RED "\x1b[38;5;196m"
#define NAY_COLOR_GREY "\x1b[38;5;245m"
#define NAY_COLOR_DIM "\x1b[38;5;240m"
#define NAY_COLOR_GREEN "\x1b[38;5;77m"

void nay_console_init(void);
void nay_console_banner(void);
void nay_console_prompt(void);
void nay_console_clear(void);

#endif
