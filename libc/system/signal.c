
#include <errno.h>
#include <signal.h>

typedef __sighandler_t Sig;

extern int _signal (int, __sighandler_t);
extern Sig _syscall_signal ();

Sig _sigtable[_NSIG-1];

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
   if( number < 1 || number >= _NSIG ) { errno=EINVAL; return SIG_ERR; }

   if( pointer == SIG_DFL || pointer == SIG_IGN )
      rv = _signal(number, pointer);
   else
      rv = _signal(number, (__sighandler_t) _syscall_signal);

   if( rv < 0 ) return SIG_ERR;

   old_sig = _sigtable[number-1];
   _sigtable[number-1] = pointer;

   switch(rv)
   {
   case 0: return SIG_DFL;
   case 1: return SIG_IGN;
   return old_sig;
   }
}
