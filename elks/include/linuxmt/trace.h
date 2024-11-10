#ifndef __LINUXMT_TRACE_H
#define __LINUXMT_TRACE_H

#include <linuxmt/config.h>

/*
 * Kernel tracing functions and their default settings
 *
 * By default, most functions (configured below) are included when
 * CONFIG_TRACE is set, which is very useful for stack overflow and
 * constistency checking when developing drivers for or porting the kernel.
 *
 * When CONFIG_TRACE is enabled, the following can be used in /bootopts:
 *  strace  - enable system call tracing
 *  kstack  - display max kernel stack usage per process
 */

#ifdef CONFIG_TRACE

/* calculate max kernel stack usage per system call, notify when near overflow */
#define CHECK_KSTACK

/* display each system call/return value along with max system stack usage */
#define CHECK_STRACE

/* check matched sleep/wait and idle task sleeps when writing/testing drivers */
#define CHECK_SCHED

/* check buffer and block I/O request system when writing/testing block drivers */
#define CHECK_BLOCKIO

/* integrity check application SS on interrupts from user mode */
#define CHECK_SS

/* check buffer and inode free counts, list inodes w/^N and buffers w/^O */
#define CHECK_FREECNTS

#endif /* CONFIG_TRACE */

/*
 * Additional options not requiring CONFIG_TRACE:
 *
 * DEBUG_CACHE      - floppy track cache tracing
 * IODELAY          - emulate floppy disk I/O delays in QEMU (set in bios.c)
 * TRACK_SPLIT_BLK  - read extra sector on track split block (set in bioshd.c)
 * SPLIT_BLK        - read extra sector on single split block (set in bioshd.c)
 * FULL_TRACK       - read full track on cache refill (set in bioshd.c)
 * BOOT_TIMER       - show startup time on boot (set in getty.c)
 */

/* internal flags for kernel */
#define TRACE_STRACE    0x01    /* system call tracing enabled */
#define TRACE_KSTACK    0x02    /* calculate kernel stack used per syscall/process */

#ifndef __ASSEMBLER__
extern int tracing;

void trace_begin(void);
void trace_end(unsigned int retval);
void check_tstack(void);
#endif

#endif
