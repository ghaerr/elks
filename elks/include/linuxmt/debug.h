/* linuxmt/include/linuxmt/debug.h for ELKS v. >=0.0.47
 * (C) 1997 Chad Page
 * 
 * This file contains the #defines to turn on and off various printk()-related
 * functions...
 */

#ifndef LX86_LINUXMT_DEBUG_H
#define LX86_LINUXMT_DEBUG_H

/* Found that strings were still included if debugging disabled so
 * re-organised so that each has a different macro depending on the number
 * of paramaters such that the parameters are not compiled in.
 *
 * Al Riddoch <ajr@ecs.soton.ac.uk> 14th Oct. 1997
 */

/* To enable debugging for any particular module, just include -DDEBUG
 * on the command line for that module. Note however that for the memory
 * management module, you will additionally need -DDEBUGMM included.
 *
 * Riley Williams <Riley@Williams.Name> 25 Apr 2002
 */

/* This switches which version of the kstack-tracker gets used */

/* Replaced by the 'true' kernel-strace */
#ifdef DEBUG
#define pstrace printk
#else
#define pstrace(_a)
#endif

/* This sets up a standard set of macros that can be used with any of the
 * files that make up the ELKS kernel. They can handle calls with up to 9
 * parameters after the format string.
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

#define debug(a)
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
