#ifndef NAY_INJECTOR_H
#define NAY_INJECTOR_H

#include <stdbool.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

typedef struct nay_injection {
    DWORD pid;
    HANDLE toggle_event;
    HANDLE nofire_event;
    HANDLE unload_event;
} nay_injection;

bool nay_inject_client(nay_injection *injection);
bool nay_injection_toggle_fullbright(nay_injection *injection);
bool nay_injection_toggle_nofire(nay_injection *injection);
void nay_injection_close(nay_injection *injection);

#endif
