#ifndef CONFIG_H
#define CONFIG_H

/* compile time options*/
#define CSLIP		0	/* compile in CSLIP support*/

/* turn these on for ELKS debugging*/
#define USE_DEBUG_EVENT 1	/* use CTRLP to toggle debug output*/
#define DEBUG_STARTDEF	0	/* default startup debug display*/
#define DEBUG_TCP	0	/* TCP ops*/
#define DEBUG_TUNE	1	/* tuning options*/
#define DEBUG_TCPDATA	0	/* TCP data packets*/
#define DEBUG_RETRANS	0	/* TCP retransmissions*/
#define DEBUG_CLOSE	1	/* TCP close ops*/
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

#if DEBUG_TUNE
#define debug_tune	DPRINTF
#else
#define debug_tune(...)
#endif

#if DEBUG_TCPDATA
#define debug_tcpdata	DPRINTF
#else
#define debug_tcpdata(...)
#endif

#if DEBUG_RETRANS
#define debug_retrans	DPRINTF
#else
#define debug_retrans(...)
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
