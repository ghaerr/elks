#include <errno.h>
#include <signal.h>
/*
 * Signal handling routines.
 *
 * KERNEL INTERFACE:
 *   It is assumed the kernel will never give us a signal we haven't
 *   _explicitly_ asked for!
 *
 * The Kernel need only save space for _one_ function pointer
 * (to _signal_cbhandler) and must deal with SIG_DFL and SIG_IGN
 * in kernel space.
 *
 * When a signal is required to be sent the kernel will setup a specialized
 * user stack frame as described in elks/arch/i86/kernel/process.c in the
 * arch_setup_sighandler_stack() function. This will look like returning
 * from a interrupt normally except the C library kernel signal callback
 * routine will be called as _signal_cbhandler(sig), after which a normal
 * interrupt return will be executed.
 *
 * Not all C library routines are safe to call from a C signal handler,
 * as the mechanism can be reentrant.
 */

sighandler_t _sigtable[_NSIG];

#ifdef __GNUC__
#if defined __TINY__ || defined __SMALL__ || defined __COMPACT__
/*
 * If we are building libc for a near-code memory model (tiny, small, or
 * compact), then we will arrange for _signal_cbhandler( ) to be within the
 * same near code section as this module.
 */
extern __attribute__((near_section, __stdcall__))   __far void _signal_cbhandler(int);
#else
extern stdcall                                      __far void _signal_cbhandler(int);
#endif
#endif

#ifdef __WATCOMC__
extern void stdcall __far _signal_cbhandler(int);   /* kernel callback in ASM */
void __far _signal_wchandler(int sig)               /* callback handler calls this */
{
    _sigtable[sig-1](sig);
}
#endif

sighandler_t signal(int number, sighandler_t pointer)
{
    sighandler_t old_sig;
    int rv;
    if (number < 1 || number > _NSIG) { errno = EINVAL; return SIG_ERR; }

    if (pointer == SIG_DFL || pointer == SIG_IGN)
        rv = _signal(number, (__kern_sighandler_t) (unsigned long)pointer);
    else
        rv = _signal(number, (__kern_sighandler_t) _signal_cbhandler);
    if (rv < 0) return SIG_ERR;

    old_sig = _sigtable[number-1];
    _sigtable[number-1] = pointer;

    return old_sig;
}
