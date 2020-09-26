#ifndef CONFIG_H
#define CONFIG_H

/* compile time options*/
#define CSLIP		1	/* compile in CSLIP support*/

/* turn these on for ELKS debugging*/
#define USE_DEBUG_EVENT 1	/* use CTRLP to toggle debug output*/
#define DEBUG_STARTDEF	0	/* default startup debug display*/
#define DEBUG_TCP	1
#define DEBUG_IP	0
#define DEBUG_ARP	0
#define DEBUG_ETH	0
#define DEBUG_CSLIP	0

/* leave these off for now - too much info*/
#define DEBUG_TCPDEV	0		/* tcpdev_ routines*/
#define DEBUG_MEM	0		/* debug memory allocations*/
#define DEBUG_CB	0		/* dump control blocks*/

#if USE_DEBUG_EVENT
void dprintf(const char *, ...);
#define PRINTF		dprintf
#else
#define PRINTF		printf
#define dprintf(...)
#endif

#if DEBUG_TCP
#define debug_tcp	PRINTF
#else
#define debug_tcp(...)
#endif

#if DEBUG_IP
#define debug_ip	PRINTF
#else
#define debug_ip(...)
#endif

#if DEBUG_ARP
#define debug_arp	PRINTF
#else
#define debug_arp(...)
#endif

#if DEBUG_CSLIP
#define debug_cslip	PRINTF
#else
#define debug_cslip(...)
#endif

#if DEBUG_TCPDEV
#define debug_tcpdev	PRINTF
#else
#define debug_tcpdev(...)
#endif

#if DEBUG_MEM
#define debug_mem	PRINTF
#else
#define debug_mem(...)
#endif

#endif
