#ifndef CONFIG_H
#define CONFIG_H

/* compile time options*/
#define CSLIP		1	/* compile in CSLIP support*/

/* turn these on for ELKS debugging*/
#define DEBUG_TCP	0
#define DEBUG_IP	0
#define DEBUG_ARP	0
#define DEBUG_ETH	0
#define DEBUG_CSLIP	0

/* leave these off for now - too much info*/
#define DEBUG_TCPDEV	0		/* tcpdev_ routines*/
#define DEBUG_MEM	0		/* debug memory allocations*/
#define DEBUG_CB	0		/* dump control blocks*/

#if DEBUG_TCP
#define debug_tcp	printf
#else
#define debug_tcp(...)
#endif

#if DEBUG_IP
#define debug_ip	printf
#else
#define debug_ip(...)
#endif

#if DEBUG_ARP
#define debug_arp	printf
#else
#define debug_arp(...)
#endif

#if DEBUG_CSLIP
#define debug_cslip	printf
#else
#define debug_cslip(...)
#endif

#if DEBUG_TCPDEV
#define debug_tcpdev	printf
#else
#define debug_tcpdev(...)
#endif

#if DEBUG_MEM
#define debug_mem	printf
#else
#define debug_mem(...)
#endif

#endif
