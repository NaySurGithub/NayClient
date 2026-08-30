#include "nay/modules/module/module.h"
#define NAY_MAX_MODULES 256u
static nay_module *modules[NAY_MAX_MODULES];
static unsigned module_count;

void nay_module_init(nay_module *m, const char *name, void *context, nay_module_callback enable, nay_module_callback disable, nay_module_callback tick)
{
    if (!m) return;
    m->name = name; m->enabled = false; m->context = context;
    m->on_enable = enable; m->on_disable = disable; m->on_tick = tick;
    if (module_count < NAY_MAX_MODULES) modules[module_count++] = m;
}
bool nay_module_set_enabled(nay_module *m, bool enabled)
{
    nay_module_callback callback;
    if (!m) return false;
    if (m->enabled == enabled) return true;
    callback = enabled ? m->on_enable : m->on_disable;
    if (callback && !callback(m)) return false;
    m->enabled = enabled;
    return true;
}
bool nay_module_toggle(nay_module *m) { return m && nay_module_set_enabled(m, !m->enabled); }
bool nay_module_tick(nay_module *m) { return m && m->enabled && (!m->on_tick || m->on_tick(m)); }
void nay_module_shutdown(nay_module *m) { if (m) (void)nay_module_set_enabled(m, false); }
void nay_module_tick_all(void)
{
    unsigned index;
    for (index = 0; index < module_count; ++index) (void)nay_module_tick(modules[index]);
}
void nay_module_shutdown_all(void)
{
    while (module_count) nay_module_shutdown(modules[--module_count]);
}
