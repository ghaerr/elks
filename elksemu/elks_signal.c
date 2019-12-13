
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/vm86.h>
#include "elks.h" 

static int elks_sigtrap= -1;

void sig_trap(int signo)
{
   elks_cpu.regs.esp -= 2;
   ELKS_POKE(unsigned short, elks_cpu.regs.esp, signo);
   elks_cpu.regs.esp -= 2;
   ELKS_POKE(unsigned short, elks_cpu.regs.esp, elks_cpu.regs.eip);
   elks_cpu.regs.eip = elks_sigtrap;
}

int elks_signal(int bx,int cx,int dx,int di,int si)
{
   void (*oldsig)(int) = 0;
   if( bx < 0 || bx >= NSIG ) { errno = EINVAL; return -1; }
   if( cx == 0 )      oldsig = signal(bx, SIG_DFL);
   else if( cx == 1 ) oldsig = signal(bx, SIG_IGN);
   else 
   {
      elks_sigtrap = cx;
      oldsig = signal(bx, sig_trap);
   }
   if( oldsig == SIG_ERR) return -1;
   if( oldsig == SIG_DFL) return 0;
   if( oldsig == SIG_IGN) return 1;
   return 2;
}
