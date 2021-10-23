#ifndef CONFIG_H
#define CONFIG_H

/* compile time options*/
#define CSLIP			1	/* compile in CSLIP support*/
#define SEND_RST_ON_CLOSE	0	/* send RST instead of FIN on close*/
#define SEND_RST_ON_REFUSED_PKT	0	/* send RST on unknown TCP packets*/

/* turn these on for ELKS debugging*/
#define USE_DEBUG_EVENT 1	/* use CTRLP to toggle debug output*/
#define DEBUG_STARTDEF	0	/* default startup debug display*/
#define DEBUG_TCP	1
#define DEBUG_CLOSE	1
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
#define DPRINTF		dprintf
#else
#define DPRINTF		printf
#define dprintf(...)
#endif

#if DEBUG_TCP
#define debug_tcp	DPRINTF
#else
#define debug_tcp(...)
#endif

#if DEBUG_CLOSE
#define debug_close	DPRINTF
#else
#define debug_close(...)
#endif

#if DEBUG_IP
#define debug_ip	DPRINTF
#else
#define debug_ip(...)
#endif

#if DEBUG_ARP
#define debug_arp	DPRINTF
#else
#define debug_arp(...)
#endif

#if DEBUG_CSLIP
#define debug_cslip	DPRINTF
#else
#define debug_cslip(...)
#endif

#if DEBUG_TCPDEV
#define debug_tcpdev	DPRINTF
#else
#define debug_tcpdev(...)
#endif

#if DEBUG_MEM
#define debug_mem	DPRINTF
#else
#define debug_mem(...)
#endif

#endif
