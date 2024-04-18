#ifndef __LINUXMT_DEBUG_H
#define __LINUXMT_DEBUG_H

/* linuxmt/include/linuxmt/debug.h for ELKS v. >=0.0.47
 * (C) 1997 Chad Page
 * Al Riddoch <ajr@ecs.soton.ac.uk> 14th Oct. 1997
 * 
 * This file contains the #defines to turn on and off various printk()-related
 * functions...
 */

/*
 * Kernel debug options, set =1 to turn on. Works across multiple files.
 */
#define DEBUG_EVENT     1               /* generate debug events on CTRLN-CTRLP*/
#define DEBUG_STARTDEF  1               /* default startup debug display*/
#define DEBUG_BIOS      0               /* BIOS driver*/
#define DEBUG_BLK       0               /* block i/o*/
#define DEBUG_ETH       0               /* ethernet*/
#define DEBUG_FAT       0               /* FAT filesystem*/
#define DEBUG_FILE      0               /* sys open and file i/o*/
#define DEBUG_LP        0               /* parallel port */
#define DEBUG_MAP       0               /* L1 mapping */
#define DEBUG_MM        0               /* mem char device*/
#define DEBUG_NET       0               /* networking*/
#define DEBUG_SCHED     0               /* scheduler/wait*/
#define DEBUG_SIG       0               /* signals*/
#define DEBUG_SUP       0               /* superblock, mount, umount*/
#define DEBUG_TTY       0               /* tty driver*/
#define DEBUG_TUNE      0               /* tunable debug statements*/
#define DEBUG_WAIT      0               /* wait, exit*/

#if DEBUG_EVENT
void dprintk(const char *, ...);        /* printk when debugging on*/
void debug_event(int evnum);            /* generate debug event*/
void debug_setcallback(int evnum, void (*cbfunc)()); /* callback on debug event*/
#define PRINTK          dprintk
#else
#define PRINTK          printk
#define dprintk(...)
#define debug_event(evnum)
#define debug_setcallback(evnum,cbfunc)
#endif

#if DEBUG_BIOS
#define debug_bios      PRINTK
#else
#define debug_bios(...)
#endif

#if DEBUG_BLK
#define debug_blk       PRINTK
#else
#define debug_blk(...)
#endif

#if DEBUG_ETH
#define debug_eth   PRINTK
#else
#define debug_eth(...)
#endif

#if DEBUG_FAT
#define debug_fat       PRINTK
#else
#define debug_fat(...)
#endif

#if DEBUG_FILE
#define debug_file      PRINTK
#else
#define debug_file(...)
#endif

#if DEBUG_LP
#define debug_lp      PRINTK
#else
#define debug_lp(...)
#endif

#if DEBUG_MM
#define debugmem        PRINTK
#else
#define debugmem(...)
#endif

#if DEBUG_MAP
#define debug_map       PRINTK
#else
#define debug_map(...)
#endif

#if DEBUG_NET
#define debug_net       PRINTK
#else
#define debug_net(...)
#endif

#if DEBUG_SCHED
#define debug_sched     printk
#else
#define debug_sched(...)
#endif

#if DEBUG_SIG
#define debug_sig       PRINTK
#else
#define debug_sig(...)
#endif

#if DEBUG_SUP
#define debug_sup       PRINTK
#else
#define debug_sup(...)
#endif

#if DEBUG_TTY
#define debug_tty       PRINTK
#else
#define debug_tty(...)
#endif

#if DEBUG_TUNE
#define debug_tune      PRINTK
#else
#define debug_tune(...)
#endif

#if DEBUG_WAIT
#define debug_wait      PRINTK
#else
#define debug_wait(...)
#endif

/* Old debug mechanism - enables debug() macro.
 *
 * To enable debugging for any particular module, just include -DDEBUG=1
 * on the command line or #define DEBUG 1 in the module.
 *
 * Riley Williams <Riley@Williams.Name> 25 Apr 2002
 */

#if DEBUG
#       define  debug(...)      PRINTK(__VA_ARGS__)
#else
#       define  debug(...)
#endif

#endif
