#ifndef CONFIG_H
#define CONFIG_H

//#include <autoconf.h>
#undef CONFIG_CSLIP

//#define ARP_WAIT_KLUGE	/* force loop on arp reply*/

/* turn these on for ELKS debugging*/
#define DEBUG_TCP	0
#define DEBUG_IP	0
#define DEBUG_ARP	0

/* leave these off for now - too much info*/
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

#if DEBUG_MEM
#define debug_mem	printf
#else
#define debug_mem(...)
#endif

#endif
