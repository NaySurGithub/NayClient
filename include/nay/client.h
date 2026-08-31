#ifndef NAY_CLIENT_H
#define NAY_CLIENT_H

#include <stdbool.h>

#if defined(_WIN32)
#  if defined(NAYCLIENT_BUILD)
#    define NAY_API __declspec(dllexport)
#  else
#    define NAY_API __declspec(dllimport)
#  endif
#else
#  define NAY_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct nay_client_config {
    bool solo_only;
    bool fullbright_enabled;
    float fullbright_level;
} nay_client_config;

NAY_API bool nay_client_start(const nay_client_config *config);
NAY_API void nay_client_tick(void);
NAY_API void nay_client_stop(void);
NAY_API bool nay_client_is_running(void);
NAY_API bool nay_can_inject(void);
NAY_API void nay_fullbright_set_enabled(bool enabled);
NAY_API void nay_fullbright_set_level(float level);
NAY_API bool nay_client_bind_packet_sender(void *client_instance);
NAY_API bool nay_client_can_send_packets(void);
NAY_API bool nay_client_send_packet(void *packet);
NAY_API bool nay_client_send_packet_to_server(void *packet);

#ifdef __cplusplus
}
#endif

#endif
