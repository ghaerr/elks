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
#else
#define printd_mfs(_a)
#endif

#if 0
#define printd_mfs1 printk 
#else
#define printd_mfs1(_a,_b)
#endif

#if 0
#define printd_mfs2 printk 
#else
#define printd_mfs2(_a,_b,_c)
#endif

#if 0
#define printd_mfs3 printk 
#else
#define printd_mfs3(_a,_b,_c,_d)
#endif

#if 0
#define printd_rfs printk 
#else
#define printd_rfs(_a)
#endif

#if 0
#define printd_exec printk
#else
#define printd_exec(_a)
#endif

#if 0
#define printd_exec1 printk
#else
#define printd_exec1(_a,_b)
#endif

#if 0
#define printd_exec2 printk
#else
#define printd_exec2(_a,_b,_c)
#endif

#if 0
#define printd_fsmkdir printk
#else
#define printd_fsmkdir(_a)
#endif

#if 0
#define printd_bufmap printk
#else
#define printd_bufmap(_a)
#endif

#if 0
#define printd_bufmap1 printk
#else
#define printd_bufmap1(_a,_b)
#endif

#if 0
#define printd_bufmap2 printk
#else
#define printd_bufmap2(_a,_b,_c)
#endif

#if 0
#define printd_bufmap3 printk
#else
#define printd_bufmap3(_a,_b,_c,_d)
#endif

#if 0
#define printd_fork printk
#else
#define printd_fork(_a)
#endif

#if 0
#define printd_chq printk
#else
#define printd_chq(_a)
#endif

#if 0
#define printd_chq3 printk
#else
#define printd_chq3(_a,_b,_c,_d)
#endif

#if 0
#define printd_chq5 printk
#else
#define printd_chq5(_a,_b,_c,_d,_e,_f)
#endif

#if 0
#define printd_chq6 printk
#else
#define printd_chq6(_a,_b,_c,_d,_e,_f,_g)
#endif

#if 0
#define printd_mount printk
#else
#define printd_mount(_a)
#endif

#if 0
#define printd_namei printk
#else
#define printd_namei(_a)
#endif

#if 0
#define printd_namei1 printk
#else
#define printd_namei1(_a,_b)
#endif

#if 0
#define printd_namei2 printk
#else
#define printd_namei2(_a,_b,_c)
#endif

#if 0
#define printd_iget printk
#else
#define printd_iget(_a)
#endif

#if 0
#define printd_iget1 printk
#else
#define printd_iget1(_a,_b)
#endif

#if 0
#define printd_iget3 printk
#else
#define printd_iget3(_a,_b,_c,_d)
#endif
/* This is really chatty, and not recommended for use on a 5150 :) */

#if 0
#define printd_mem printk
#else
#define printd_mem(_a)
#endif

#if 0
#define printd_mem1 printk
#else
#define printd_mem1(_a,_b)
#endif

#if 0
#define printd_rs printk
#else
#define printd_rs(_a)
#endif

#if 0
#define printd_rd printk
#else
#define printd_rd(_a)
#endif

#if 0
#define printd_rd1 printk
#else
#define printd_rd1(_a,_b)
#endif

#if 0
#define printd_rd2 printk
#else
#define printd_rd2(_a,_b,_c)
#endif

#if 0
#define printd_pipe printk
#else
#define printd_pipe(_a)
#endif

#if 0
#define printd2_pipe printk
#else
#define printd2_pipe(_a,_b,_c)
#endif
