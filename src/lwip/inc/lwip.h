#include "std_types.h"
#include "lwip/ip_addr.h"
#include "lwip/err.h"

void Lwip_Init(void);
void Lwip_Task(void *pvParameters);
err_t Lwip_CreateUdpConnection(ip_addr_t ip_address, uint16_t port);
err_t Lwip_SendUdp(const char* data, uint16_t data_len);