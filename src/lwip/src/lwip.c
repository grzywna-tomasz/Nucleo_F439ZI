#include "FreeRTOS.h"
#include "task.h"
#include "stm32f4xx_hal.h"
#include <string.h>

#include "lwip/tcpip.h"
#include "lwip/prot/ethernet.h"
#include "lwip/etharp.h"
#include "lwip/tcpip.h"
#include "lwip/pbuf.h"
#include "lwip/netbuf.h"
#include "lwip/api.h"
#include "lwip/memp.h"
#include "lwip/udp.h"
#include "lwip/init.h"
#include "lwip/netif.h"
#include "ethernetif.h"
#include "netif/ethernet.h"

#define SERVER_IP_BYTE_1    (198U)
#define SERVER_IP_BYTE_2    (19U)
#define SERVER_IP_BYTE_3    (60U)
#define SERVER_IP_BYTE_4    (101U)

#define SERVER_MASK_BYTE_1    (255U)
#define SERVER_MASK_BYTE_2    (255U)
#define SERVER_MASK_BYTE_3    (252U)
#define SERVER_MASK_BYTE_4    (0U)

#define SERVER_GATEWAY_BYTE_1    (0U)
#define SERVER_GATEWAY_BYTE_2    (0U)
#define SERVER_GATEWAY_BYTE_3    (0U)
#define SERVER_GATEWAY_BYTE_4    (0U)

static void Ethernet_Link_Periodic_Handle(struct netif *netif);
static void ethernet_link_status_updated(struct netif *netif);

static struct udp_pcb *Lwip_UdpClient;
static struct netif Lwip_NetIf;
ip4_addr_t Lwip_ServerIp;
ip4_addr_t Lwip_NetMask;
ip4_addr_t Lwip_Gateway;
uint32_t EthernetLinkTimer;

static void Ethernet_Link_Periodic_Handle(struct netif *netif)
{
    /* Ethernet Link every 100ms */
    if (HAL_GetTick() - EthernetLinkTimer >= 100)
    {
        EthernetLinkTimer = HAL_GetTick();
        ethernet_link_check_state(netif);
    }
}

static void ethernet_link_status_updated(struct netif *netif)
{
    if (netif_is_up(netif))
    {
        // TODOTODO logging here
    }
    else /* netif is down */
    {
        // TODOTODO logging here
    }
}

void Lwip_Task(void *pvParameters)
{
    while(1)
    {
        // TODOTODO - this propably can be reworked with the interrupts
        ethernetif_input(&Lwip_NetIf);
    
        sys_check_timeouts();
    
        Ethernet_Link_Periodic_Handle(&Lwip_NetIf);
    
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

void Lwip_Init(void)
{
    lwip_init();

    /* IP addresses initialization without DHCP (IPv4) */
    IP4_ADDR(&Lwip_ServerIp, SERVER_IP_BYTE_1, SERVER_IP_BYTE_2, SERVER_IP_BYTE_3, SERVER_IP_BYTE_4);
    IP4_ADDR(&Lwip_NetMask, SERVER_MASK_BYTE_1, SERVER_MASK_BYTE_2, SERVER_MASK_BYTE_3, SERVER_MASK_BYTE_4);
    IP4_ADDR(&Lwip_Gateway, SERVER_GATEWAY_BYTE_1, SERVER_GATEWAY_BYTE_2, SERVER_GATEWAY_BYTE_3, SERVER_GATEWAY_BYTE_4);

    /* add the network interface (IPv4/IPv6) */
    netif_add(&Lwip_NetIf, &Lwip_ServerIp, &Lwip_NetMask, &Lwip_Gateway, NULL, &ethernetif_init, &ethernet_input);

    /* Registers the default network interface */
    netif_set_default(&Lwip_NetIf);

    /* Bring up the netowork interface*/
    netif_set_up(&Lwip_NetIf);

    /* Set the link callback function, this function is called on change of link status*/
    netif_set_link_callback(&Lwip_NetIf, ethernet_link_status_updated);
}

err_t Lwip_CreateUdpConnection(ip_addr_t ip_address, uint16_t port)
{
    Lwip_UdpClient = udp_new();
    
    if (Lwip_UdpClient == NULL)
    {
        return ERR_MEM;
    }

    return udp_connect(Lwip_UdpClient, &ip_address, port);
}

err_t Lwip_SendUdp(uint8_t message[], uint16_t message_length)
{
    struct pbuf *p;
    if (message == NULL || message_length == 0)
    {
        return ERR_ARG;
    }

    p = pbuf_alloc(PBUF_TRANSPORT, message_length, PBUF_RAM);

    if (p != NULL)
    {
        memcpy(p->payload, message, message_length);
        return udp_send(Lwip_UdpClient, p);
    }

    return ERR_MEM;
}
