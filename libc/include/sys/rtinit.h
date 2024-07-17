#ifndef __SYS_RTINIT_H
#define __SYS_RTINIT_H
/*
 * Runtime library initialization
 *
 * For ia16-elf-gcc and OpenWatcom C
 */

/*
 * Constructor/destructor priorities (0=highest, 255=lowest)
 * Constructors are run 0..255 in priority order before main().
 * Destructors are run 255..0 in reverse priority order at start of exit(),
 *   but are skipped if _exit() called directly.
 * Priorities <= 100 are reserved for library use.
 */
#define _INIT_PRI_STDIO     20      /* stdio stream setup/flush */
#define _INIT_PRI_FTRACE    24      /* --ftrace setup */
#define _INIT_PRI_ATEXIT    32      /* atexit fini before stdio close */
#define _INIT_PRI_PROGRAM   101     /* default program-init priority */

#ifdef __WATCOMC__
struct _rt_init {                   /* stored in XI/YI segments */
    void (*func)(void);
    unsigned char priority;
    unsigned char done;
};

/* XI/YI section start/end for constructor/destructors */
extern struct _rt_init _Start_XI;
extern struct _rt_init _End_XI;
extern struct _rt_init _Start_YI;
extern struct _rt_init _End_YI;

/* constructor/destructor functions called before main and after exit */
#pragma aux __InitRtns modify [ bx cx dx si di ]
#pragma aux __FiniRtns modify [ bx cx dx si di ]
extern void __InitRtns(void);
extern void __FiniRtns(void);

#endif

#endif
