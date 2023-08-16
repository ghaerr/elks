#ifndef __LINUXMT_LIMITS_H
#define __LINUXMT_LIMITS_H

/* tunable parameters */
#define MAX_TASKS	16	/* Max # processes */

#ifdef CONFIG_ARCH_PC98
#define KSTACK_BYTES	740	/* Size of kernel stacks for PC-98 */
#else
#ifdef CONFIG_ASYNCIO
#define KSTACK_BYTES	700	/* Size of kernel stacks w/async I/O */
#else
#define KSTACK_BYTES	640	/* Size of kernel stacks w/sync I/O */
#endif
#endif

#define ISTACK_BYTES    512     /* Size of interrupt stack */

#define KSTACK_GUARD    100     /* bytes before CHECK_KSTACK overflow warning */

#define POLL_MAX	6	/* Maximum number of polled queues per process */

#define NR_ALARMS	5	/* Max number of simultaneous alarms system-wide */
#define NGROUPS		13	/* Supplementary groups */

#define PIPE_BUFSIZ	80	/* doesn't have to be power of two */

#define MAX_PACKET_ETH 1536	/* Max packet size, 6 blocks of 256 bytes */

#endif /* !__LINUXMT_LIMITS_H */
