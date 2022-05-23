#ifndef __LINUXMT_NETSTAT_H
#define __LINUXMT_NETSTAT_H

#include <linuxmt/types.h>

struct netif_stat {
	__u16 rx_errors;	/* Receive errors, flagged by NIC */
	__u16 rq_errors;	/* Receive queue errors (in 8bit interfaces) */
	__u16 tx_errors;	/* Transmit errors, flagged by NIC */
	__u16 oflow_errors;	/* Receive buffer overflow interrupts */
	__u16 if_status;	/* Interface status flags */
	int   oflow_keep;	/* # of packets to keep if overflow */
	char  mac_addr[6];	/* Current MAC address */
};

/* status flags for if_status */

#define	NETIF_IS_8BIT	1
#define NETIF_FORCE_4K	2
#define NETIF_IS_OPEN	4
#define NETIF_IS_QEMU	8





#endif
