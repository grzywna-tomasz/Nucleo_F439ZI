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
#include "lwip/sockets.h"
#include "lwip/netdb.h"

#define SERVER_IP_BYTE_1    (192U)
#define SERVER_IP_BYTE_2    (168U)
#define SERVER_IP_BYTE_3    (100U)
#define SERVER_IP_BYTE_4    (10U)

#define SERVER_MASK_BYTE_1    (255U)
#define SERVER_MASK_BYTE_2    (255U)
#define SERVER_MASK_BYTE_3    (255U)
#define SERVER_MASK_BYTE_4    (0U)

#define SERVER_GATEWAY_BYTE_1    (0U)
#define SERVER_GATEWAY_BYTE_2    (0U)
#define SERVER_GATEWAY_BYTE_3    (0U)
#define SERVER_GATEWAY_BYTE_4    (0U)

static void Ethernet_Link_Periodic_Handle(struct netif *netif);

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

void Lwip_UdpTask(void *pvParameters)
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
    /* This initializes TCP IP and call lwip_init which initialize the UDP part as well */
    tcpip_init(NULL, NULL);

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
    err_t ret_val = ERR_MEM;
    if (message == NULL || message_length == 0)
    {
        return ERR_ARG;
    }

    p = pbuf_alloc(PBUF_TRANSPORT, message_length, PBUF_RAM);

    if (p != NULL)
    {
        memcpy(p->payload, message, message_length);
        ret_val = udp_send(Lwip_UdpClient, p);
    }

    pbuf_free(p);

    return ret_val;
}

#define TCP_PORT 5000
#define RX_BUFFER_SIZE 512

void Lwip_TcpIpTask(void *pvParameters)
{
    vTaskDelay(pdMS_TO_TICKS(2000));
    /* Create File Descriptors (Unix naming for IDs) */
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    char rx_buffer[RX_BUFFER_SIZE];

    /* Create socket */
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0)
    {
        while(1);
    }

    /* Bind */
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(TCP_PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        close(server_fd);
         while(1);
    }

    /* Listen */
    if (listen(server_fd, 0) < 0)
    {
        close(server_fd);
        while(1);
    }

    Lwip_SendUdp("TCP server listening\n", strlen("TCP server listening\n"));

    while (1)
    {
        /* Accept client connection */
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd < 0)
        {
            continue;
        }

        Lwip_SendUdp("Client connected", strlen("Client connected"));

        while (1)
        {
            int len = recv(client_fd, rx_buffer, RX_BUFFER_SIZE - 1, 0);

            if (len <= 0)
            {
                Lwip_SendUdp("Client disconnected", strlen("Client disconnected"));
                break;
            }

            rx_buffer[len] = '\0';

            Lwip_SendUdp(rx_buffer, len);

            /* Echo back */
            send(client_fd, rx_buffer, len, 0);
        }

        /* Close client */
        close(client_fd);
    }
}