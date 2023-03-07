#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

void
abort(void)
{
   signal (SIGABRT, SIG_DFL);
   kill (getpid (), SIGABRT);  // first try
   pause ();  // system may just schedule
   signal (SIGKILL, SIG_DFL);
   kill (getpid (), SIGKILL);  // second try
   _exit (255);  // third try
}
