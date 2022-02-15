#include <errno.h>
#include <signal.h>

typedef __sighandler_t Sig;

extern int _signal (int, __kern_sighandler_t);
#if defined __TINY__ || defined __SMALL__ || defined __COMPACT__
/*
 * If we are building libc for a near-code memory model (tiny, small, or
 * compact), then we will arrange for _syscall_signal( ) to be within the
 * same near code section as this module.
 */
extern __attribute__((near_section, stdcall))
#else
extern __attribute__((stdcall))
#endif
    __far void _syscall_signal (int);

Sig _sigtable[_NSIG];

/*
 * Signal handler.
 *
 */

/*
 * KERNEL INTERFACE:
 *   It is assumed the kernel will never give us a signal we haven't
 *   _explicitly_ asked for!
 *
 * The Kernel need only save space for _one_ function pointer
 * (to _syscall_signal) and must deal with SIG_DFL and SIG_IGN
 * in kernel space.
 *
 * When a signal is required the kernel must set all the registers as if
 * returning from a interrupt normally then push the number of the signal
 * to be generated, push the current pc value, then set the pc to the
 * address of the '_syscall_signal' function.
 */

Sig signal(int number, Sig pointer)
{
   Sig old_sig;
   int rv;
   if( number < 1 || number > _NSIG ) { errno=EINVAL; return SIG_ERR; }

#pragma GCC diagnostic ignored "-Wpointer-to-int-cast"
   if( pointer == SIG_DFL || pointer == SIG_IGN )
      rv = _signal(number, (__kern_sighandler_t) (long) (int) pointer);
   else
      rv = _signal(number, (__kern_sighandler_t) _syscall_signal);

   if( rv < 0 ) return SIG_ERR;

   old_sig = _sigtable[number-1];
   _sigtable[number-1] = pointer;

   return old_sig;
}
