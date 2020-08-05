#ifndef __LINUXMT_DEBUG_H
#define __LINUXMT_DEBUG_H

/* linuxmt/include/linuxmt/debug.h for ELKS v. >=0.0.47
 * (C) 1997 Chad Page
 * 
 * This file contains the #defines to turn on and off various printk()-related
 * functions...
 */

/* Found that strings were still included if debugging disabled so
 * re-organised so that each has a different macro depending on the number
 * of paramaters such that the parameters are not compiled in.
 *
 * Al Riddoch <ajr@ecs.soton.ac.uk> 14th Oct. 1997
 */

/* This switches which version of the kstack-tracker gets used */

/* Replaced by the 'true' kernel-strace */
#ifdef DEBUG
#define pstrace printk
#else
#define pstrace(_a)
#endif

/*
 * Kernel debug options, set =1 to turn on. Works across multiple files.
 */
#define DEBUG_BLK	0		/* block i/o*/
#define DEBUG_FAT	0		/* FAT filesystem*/
#define DEBUG_FILE	0		/* sys open and file i/o*/
#define DEBUG_NET	0		/* networking*/
#define DEBUG_SCHED	0		/* scheduler/wait*/
#define DEBUG_SIG	0		/* signals*/
#define DEBUG_SUP	0		/* superblock, mount, umount*/
#define DEBUG_TTY	0		/* tty driver*/
#define DEBUG_WAIT	0		/* wait, exit*/

#if DEBUG_BLK
#define debug_blk	printk
#else
#define debug_blk(...)
#endif

#if DEBUG_FAT
#define debug_fat	printk
#else
#define debug_fat(...)
#endif

#if DEBUG_FILE
#define debug_file	printk
#else
#define debug_file(...)
#endif

#if DEBUG_NET
#define debug_net	printk
#else
#define debug_net(...)
#endif

#if DEBUG_SCHED
#define debug_sched	printk
#else
#define debug_sched(...)
#endif

#if DEBUG_SIG
#define debug_sig	printk
#else
#define debug_sig(...)
#endif

#if DEBUG_SUP
#define debug_sup	printk
#else
#define debug_sup(...)
#endif

#if DEBUG_TTY
#define debug_tty	printk
#else
#define debug_tty(...)
#endif

#if DEBUG_WAIT
#define debug_wait	printk
#else
#define debug_wait(...)
#endif

/* Old debug mechanism - deprecated.
 * This sets up a standard set of macros that can be used with any of the
 * files that make up the ELKS kernel. They can handle calls with up to 9
 * parameters after the format string.
 *
 * To enable debugging for any particular module, just include -DDEBUG
 * on the command line for that module. Note however that for the memory
 * management module, you will additionally need -DDEBUGMM included.
 *
 * Riley Williams <Riley@Williams.Name> 25 Apr 2002
 */

#ifdef DEBUG

#define debug					printk
#define debug1					printk
#define debug2					printk
#define debug3					printk
#define debug4					printk
#define debug5					printk
#define debug6					printk
#define debug7					printk
#define debug8					printk
#define debug9					printk

#else

#define debug(a,...)
#define debug1(a,b)
#define debug2(a,b,c)
#define debug3(a,b,c,d)
#define debug4(a,b,c,d,e)
#define debug5(a,b,c,d,e,f)
#define debug6(a,b,c,d,e,f,g)
#define debug7(a,b,c,d,e,f,g,h)
#define debug8(a,b,c,d,e,f,g,h,i)
#define debug9(a,b,c,d,e,f,g,h,i,j)

#endif

/* This is really chatty, and not recommended for use on a 5150 :)
 * As a result, it requires DEBUGMM to be defined as well as DEBUG.
 * This definition uses the above definitions for simplicity.
 */

#ifdef DEBUGMM

#define debugmem				debug
#define debugmem1				debug1
#define debugmem2				debug2
#define debugmem3				debug3

#else

#define debugmem(_a)
#define debugmem1(_a,_b)
#define debugmem2(_a,_b,_c)
#define debugmem3(_a,_b,_c,_d)

#endif

#endif
