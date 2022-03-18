#ifndef __LINUXMT_LIMITS_H
#define __LINUXMT_LIMITS_H

#define MAX_TASKS	16	/* Max # processes */

#ifdef CONFIG_ARCH_PC98
#define KSTACK_BYTES	740	/* Size of kernel stacks */
#else
#define KSTACK_BYTES	640	/* Size of kernel stacks */
#endif

#define POLL_MAX	6	/* Maximum number of polled queues per process */

#define NR_ALARMS	5	/* Max number of simultaneous alarms system-wide */
#define NGROUPS		13	/* Supplementary groups */

#define MAX_PACKET_ETH 1536	/* Max packet size, 6 blocks of 256 bytes */

#endif /* !__LINUXMT_LIMITS_H */
