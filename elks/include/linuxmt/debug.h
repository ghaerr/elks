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

#ifdef DEBUG
#define printd_mfs printk
#define printd_mfs1 printk
#define printd_mfs2 printk
#define printd_mfs3 printk
#else
#define printd_mfs(_a)
#define printd_mfs1(_a,_b)
#define printd_mfs2(_a,_b,_c)
#define printd_mfs3(_a,_b,_c,_d)
#endif

#ifdef DEBUG
#define printd_mm printk 
#define printd_mm1 printk 
#define printd_mm2 printk 
#define printd_mm3 printk 
#else
#define printd_mm(_a)
#define printd_mm1(_a,_b)
#define printd_mm2(_a,_b,_c)
#define printd_mm3(_a,_b,_c,_d)
#endif

#ifdef DEBUG
#define printd_rfs printk 
#else
#define printd_rfs(_a)
#endif

#ifdef DEBUG
#define printd_sig printk
#define printd_sig1 printk
#define printd_sig2 printk
#define printd_sig4 printk
#define printd_sig5 printk
#else
#define printd_sig(_a)
#define printd_sig1(_a,_b)
#define printd_sig2(_a,_b,_c)
#define printd_sig4(_a,_b,_c,_d,_e)
#define printd_sig5(_a,_b,_c,_d,_e,_f)
#endif

#ifdef DEBUG
#define printd_exec printk
#define printd_exec1 printk
#define printd_exec2 printk
#else
#define printd_exec(_a)
#define printd_exec1(_a,_b)
#define printd_exec2(_a,_b,_c)
#endif

#ifdef DEBUG
#define printd_fsmkdir printk
#define printd_fsmkdir1 printk
#else
#define printd_fsmkdir(_a)
#define printd_fsmkdir1(_a,_b)
#endif

#ifdef DEBUG
#define printd_bufmap printk
#define printd_bufmap1 printk
#define printd_bufmap2 printk
#define printd_bufmap3 printk
#else
#define printd_bufmap(_a)
#define printd_bufmap1(_a,_b)
#define printd_bufmap2(_a,_b,_c)
#define printd_bufmap3(_a,_b,_c,_d)
#endif

#ifdef DEBUG
#define printd_fork printk
#else
#define printd_fork(_a)
#endif

#ifdef DEBUG
#define printd_chq printk
#define printd_chq3 printk
#define printd_chq5 printk
#define printd_chq6 printk
#else
#define printd_chq(_a)
#define printd_chq3(_a,_b,_c,_d)
#define printd_chq5(_a,_b,_c,_d,_e,_f)
#define printd_chq6(_a,_b,_c,_d,_e,_f,_g)
#endif

#ifdef DEBUG
#define printd_mount printk
#else
#define printd_mount(_a)
#endif

#ifdef DEBUG
#define printd_namei printk
#define printd_namei1 printk
#define printd_namei2 printk
#else
#define printd_namei(_a)
#define printd_namei1(_a,_b)
#define printd_namei2(_a,_b,_c)
#endif

#ifdef DEBUG
#define printd_iget printk
#define printd_iget1 printk
#define printd_iget3 printk
#else
#define printd_iget(_a)
#define printd_iget1(_a,_b)
#define printd_iget3(_a,_b,_c,_d)
#endif

/* This is really chatty, and not recommended for use on a 5150 :)
 * As a result, it requires DEBUGMM to be defined as well as DEBUG.
 */

#ifdef DEBUG
#ifdef DEBUGMM
#define printd_mem printk
#define printd_mem1 printk
#endif
#else
#define printd_mem(_a)
#define printd_mem1(_a,_b)
#endif

#ifdef DEBUG
#define printd_rs printk
#define printd_rs1 printk
#else
#define printd_rs(_a)
#define printd_rs1(_a,_b)
#endif

#ifdef DEBUG
#define printd_td printk
#define printd_td1 printk
#define printd_td2 printk
#else
#define printd_td(_a)
#define printd_td1(_a,_b)
#define printd_td2(_a,_b,_c)
#endif

#ifdef DEBUG
#define printd_inet printk
#define printd_inet1 printk
#define printd_inet2 printk
#define printd_inet3 printk
#else
#define printd_inet(_a)
#define printd_inet1(_a,_b)
#define printd_inet2(_a,_b,_c)
#define printd_inet3(_a,_b,_c,_d)
#endif

#ifdef DEBUG
#define printd_rd printk
#define printd_rd1 printk
#define printd_rd2 printk
#else
#define printd_rd(_a)
#define printd_rd1(_a,_b)
#define printd_rd2(_a,_b,_c)
#endif

#ifdef DEBUG
#define printd_pipe printk
#define printd2_pipe printk
#else
#define printd_pipe(_a)
#define printd2_pipe(_a,_b,_c)
#endif

#endif
