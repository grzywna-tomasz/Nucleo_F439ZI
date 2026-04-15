#ifndef TRANSPORT_LWIP_TCP_H
#define TRANSPORT_LWIP_TCP_H
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void * TransportLwipTcp_Init(uint16_t port, TaskHandle_t task_to_notify);

#ifdef __cplusplus
}
#endif

#endif /* TRANSPORT_LWIP_TCP_H */
 