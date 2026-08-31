#ifndef NAY_INJECTION_NOTICE_H
#define NAY_INJECTION_NOTICE_H

#include <stdbool.h>

typedef void (*nay_packet_observer)(unsigned handler_index, const void *packet);

bool nay_injection_notice_install(void);
void nay_injection_notice_uninstall(void);
bool nay_injection_notice_sent(void);
bool nay_injection_notice_send_text(const char *text);
void nay_injection_notice_set_packet_observer(nay_packet_observer observer);

#endif
