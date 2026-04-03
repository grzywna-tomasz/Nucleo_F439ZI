#include "FreeRTOSConfig.h"

#define NO_SYS                          0
#define SYS_LIGHTWEIGHT_PROT            1
#define LWIP_NETCONN                    1
#define LWIP_SOCKET                     1

#define TCPIP_THREAD_NAME               "tcpip"
#define TCPIP_THREAD_STACKSIZE          1024
#define TCPIP_THREAD_PRIO               (configMAX_PRIORITIES - 1)
#define DEFAULT_THREAD_STACKSIZE        512
#define DEFAULT_THREAD_PRIO             (configMAX_PRIORITIES - 2)
#define LWIP_TCPIP_CORE_LOCKING         1
#define LWIP_NETIF_HOSTNAME             1

/* Size of TCP/IP thread message queue */
#define TCPIP_MBOX_SIZE                 16
/* Queue size for incomming TCP data per connection */
#define DEFAULT_TCP_RECVMBOX_SIZE       8
/* Queue for pending incoming TCP connections */
#define DEFAULT_TCP_ACCEPTMBOX_SIZE     8
/* Queue for received UDP packets */
#define DEFAULT_UDP_RECVMBOX_SIZE       8
/* Queue for RAW API packets. TODOTODO try to set it to 0 */
#define DEFAULT_RAW_RECVMBOX_SIZE       8

#define LWIP_TCP                        1
#define LWIP_UDP                        1
#define TCP_TTL                         255
#define TCP_QUEUE_OOSEQ                 1
/* Max segment size per TCP packet - 1460 is standard */
#define TCP_MSS                         1460
/* Total send buffer size */
#define TCP_SND_BUF                     (2 * TCP_MSS)
/* TTCP receive window (how much data you can receive before ACK) */
#define TCP_WND                         (2 * TCP_MSS)
#define LWIP_TCP_KEEPALIVE              1
#define LWIP_RAW                        1
/* Number of RAW API connections. TODOTODO try set it to 0 */
#define MEMP_NUM_RAW_PCB                2

#define LWIP_ICMP                       1
#define LWIP_ARP                        1
/* Number of cached IP-MAC mappings. TODO - even through I am sending to one device, take into consideration Gateway, breadcast, other temporary trafic */
#define ARP_TABLE_SIZE                  4
#define ARP_MAXAGE                      300
/* Max packets queued while resolving address (resolving the address - sending ARP request and awaiting their response) */
#define ARP_QUEUE_LEN                   2
/* Max retries before dropping unresolved ARP */
#define ARP_MAXPENDING                  2
#define ARP_QUEUEING                    1
#define ETH_PAD_SIZE                    0
#define LWIP_IPV4                       1
#define LWIP_IPV6                       0
/* IP forwarding option. Usefull for routers */
#define IP_FORWARD                      0
/* Socket reuse option */
#define SO_REUSE                        0

#define LWIP_DHCP                       0
#define LWIP_DNS                        0
#define LWIP_AUTOIP                     0
#define LWIP_ETHERNET                   1

#define CHECKSUM_GEN_IP                 1
#define CHECKSUM_GEN_UDP                1
#define CHECKSUM_GEN_TCP                1
#define CHECKSUM_GEN_ICMP               1
#define CHECKSUM_CHECK_IP               1
#define CHECKSUM_CHECK_UDP              1
#define CHECKSUM_CHECK_TCP              1
#define CHECKSUM_CHECK_ICMP             1

#define MEM_ALIGNMENT                   4
/* Heap size for LWIP TODO - try to decrement it */
#define MEM_SIZE                        (8 * 1024)
/* Number of pbuf structures (packet descriptors) TODOTODO check what it is */
#define MEMP_NUM_PBUF                   16
/* Number of active TCP connections 2 should be enough for 1 TCP connection (1 active and 1 TIME_WAIT/closing state) */
#define MEMP_NUM_TCP_PCB                4
/* Number of queued TCP segments */
#define MEMP_NUM_TCP_SEG                8
/* Number of simultaneous lwIP timers */
#define MEMP_NUM_SYS_TIMEOUT            4

/* Number of packet buffers */
#define PBUF_POOL_SIZE                  16
/* Size for each PBUF. 1520 fitt full eth frame */
#define PBUF_POOL_BUFSIZE               1520
#define PBUF_LINK_HLEN                  14

#define LWIP_TIMERS                     1
#define LWIP_TIMERS_CUSTOM              0

/* Those options are used with stats_display(). TODO Consider configuring those */
#define LWIP_STATS                      0
#define LWIP_STATS_DISPLAY              0
#define LWIP_DEBUG                      0
#define ETHARP_DEBUG                    LWIP_DBG_OFF
#define NETIF_DEBUG                     LWIP_DBG_OFF
#define PBUF_DEBUG                      LWIP_DBG_OFF
#define TCP_DEBUG                       LWIP_DBG_OFF
#define DHCP_DEBUG                      LWIP_DBG_OFF

#define LWIP_NETIF_LINK_CALLBACK    1

/* todotodo 
Create a mallock wrapper to catch all alocation failures
monitor lwip_stats.pbuf.err lwip_stats.pbuf.drop for PBUF_POOL_SIZE too small
monitor lwip_stats.etharp.memerr lwip_stats.etharp.drop for ARP_TABLE_SIZE too small and ARP_QUEUE_LEN too small
monitor lwip_stats.tcp.memerr
lwip_stats.tcp.drop for TCP issues MEMP_NUM_TCP_SEG too small TCP_WND mismatch MEM_SIZE too small 
monitor lwip_stats.mem.err for memory errors */