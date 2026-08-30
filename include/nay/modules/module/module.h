#ifndef NAY_MODULE_H
#define NAY_MODULE_H
#include <stdbool.h>
typedef struct nay_module nay_module;
typedef bool (*nay_module_callback)(nay_module *module);
struct nay_module {
    const char *name;
    bool enabled;
    void *context;
    nay_module_callback on_enable;
    nay_module_callback on_disable;
    nay_module_callback on_tick;
};
void nay_module_init(nay_module *, const char *, void *, nay_module_callback, nay_module_callback, nay_module_callback);
bool nay_module_set_enabled(nay_module *module, bool enabled);
bool nay_module_toggle(nay_module *module);
bool nay_module_tick(nay_module *module);
void nay_module_shutdown(nay_module *module);
void nay_module_tick_all(void);
void nay_module_shutdown_all(void);
#endif
