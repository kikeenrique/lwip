// lwIP configured for one job: TCP over IPv6 on a point-to-point tunnel,
// driven from one thread by the caller, with no operating-system layer.
#ifndef LWIP_SWIFTPM_LWIPOPTS_H
#define LWIP_SWIFTPM_LWIPOPTS_H

#define NO_SYS 1
#define SYS_LIGHTWEIGHT_PROT 0
#define LWIP_SOCKET 0
#define LWIP_NETCONN 0
#define LWIP_TIMERS 1

#define LWIP_IPV4 0
#define LWIP_IPV6 1
#define LWIP_TCP 1
#define LWIP_UDP 0
#define LWIP_RAW 0
#define LWIP_DNS 0
#define LWIP_ARP 0
#define LWIP_ETHERNET 0
#define LWIP_ICMP6 1
#define LWIP_IPV6_MLD 0
#define LWIP_IPV6_AUTOCONFIG 0
#define LWIP_IPV6_DUP_DETECT_ATTEMPTS 0
#define LWIP_IPV6_SEND_ROUTER_SOLICIT 0
#define LWIP_ND6_QUEUEING 0
#define LWIP_IPV6_FRAG 0
#define LWIP_IPV6_REASS 0
#define LWIP_NETIF_LOOPBACK 0
#define LWIP_HAVE_LOOPIF 0
#define LWIP_SINGLE_NETIF 1
#define LWIP_STATS 0
#define LWIP_NETIF_API 0

// The system allocator: a desktop process is not short of memory, and lwIP's pools
// would only add limits to tune.
#define MEM_LIBC_MALLOC 1
#define MEMP_MEM_MALLOC 1
#define MEM_ALIGNMENT 8

// The device's tunnel offers an MTU of 1280, the IPv6 minimum: 1280 less 40
// bytes of IPv6 header and 20 of TCP.
#define TCP_MSS 1220
#define TCP_WND (32 * TCP_MSS)
#define TCP_SND_BUF (32 * TCP_MSS)
#define TCP_SND_QUEUELEN (4 * TCP_SND_BUF / TCP_MSS)
#define TCP_QUEUE_OOSEQ 1
#define LWIP_TCP_KEEPALIVE 1
#define LWIP_TCP_SACK_OUT 0
#define MEMP_NUM_TCP_PCB 32
#define MEMP_NUM_TCP_SEG 256
#define PBUF_POOL_SIZE 64

#define LWIP_ERRNO_STDINCLUDE 1
#define LWIP_TIMEVAL_PRIVATE 0
#define LWIP_DEBUG 0

#endif
