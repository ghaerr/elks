/* linuxmt/include/linuxmt/debug.h for ELKS v. >=0.0.47
 * (C) 1997 Chad Page
 * 
 * This file contains the #defines to turn on and off various printk()-related
 * functions...
 */

/*
 * Found that strings were still included if debugging disabled so re-organised
 * so that each has a different macro depending on the no. of paramaters so
 * that the parameters are not compiled in.
 *
 * Al (ajr@ecs.soton.ac.uk) 14th Oct. 1997
 */

/* This switches which version of the kstack-tracker gets used */

/* Replaced by the 'true' kernel-strace */
#if 0
#define pstrace printk
#else
#define pstrace(_a)
#endif

#if 0
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

#if 0
#define printd_rfs printk 
#else
#define printd_rfs(_a)
#endif

#if 0
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

#if 0
#define printd_exec printk
#define printd_exec1 printk
#define printd_exec2 printk
#else
#define printd_exec(_a)
#define printd_exec1(_a,_b)
#define printd_exec2(_a,_b,_c)
#endif

#if 0
#define printd_fsmkdir printk
#else
#define printd_fsmkdir(_a)
#endif

#if 0
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

#if 0
#define printd_fork printk
#else
#define printd_fork(_a)
#endif

#if 0
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

#if 0
#define printd_mount printk
#else
#define printd_mount(_a)
#endif

#if 0
#define printd_namei printk
#define printd_namei1 printk
#define printd_namei2 printk
#else
#define printd_namei(_a)
#define printd_namei1(_a,_b)
#define printd_namei2(_a,_b,_c)
#endif

#if 0
#define printd_iget printk
#define printd_iget1 printk
#define printd_iget3 printk
#else
#define printd_iget(_a)
#define printd_iget1(_a,_b)
#define printd_iget3(_a,_b,_c,_d)
#endif

/* This is really chatty, and not recommended for use on a 5150 :) */

#if 0
#define printd_mem printk
#define printd_mem1 printk
#else
#define printd_mem(_a)
#define printd_mem1(_a,_b)
#endif

#if 0
#define printd_rs printk
#define printd_rs1 printk
#else
#define printd_rs(_a)
#define printd_rs1(_a,_b)
#endif

#if 0
#define printd_rd printk
#define printd_rd1 printk
#define printd_rd2 printk
#else
#define printd_rd(_a)
#define printd_rd1(_a,_b)
#define printd_rd2(_a,_b,_c)
#endif

#if 0
#define printd_pipe printk
#define printd2_pipe printk
#else
#define printd_pipe(_a)
#define printd2_pipe(_a,_b,_c)
#endif
