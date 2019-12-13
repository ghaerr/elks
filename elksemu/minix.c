#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/vm86.h>
#include <sys/times.h>
#include <utime.h>
#include <termios.h>
#include <time.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <dirent.h>
#include "elks.h"

#ifdef DEBUG
#define dbprintf(x) db_printf x
#else
#define dbprintf(x)
#endif

static char * minix_names[] = {
   "0", "EXIT", "FORK", "READ", "WRITE", "OPEN", "CLOSE", "WAIT",
   "CREAT", "LINK", "UNLINK", "WAITPID", "CHDIR", "TIME", "MKNOD",
   "CHMOD", "CHOWN", "BRK", "STAT", "LSEEK", "GETPID", "MOUNT",
   "UMOUNT", "SETUID", "GETUID", "STIME", "PTRACE", "ALARM", "FSTAT",
   "PAUSE", "UTIME", "31", "32", "ACCESS", "34", "35", "SYNC", "KILL",
   "RENAME", "MKDIR", "RMDIR", "DUP", "PIPE", "TIMES", "44", "45",
   "SETGID", "GETGID", "SIGNAL", "49", "50", "51", "52", "53", "IOCTL",
   "FCNTL", "56", "57", "58", "EXEC", "UMASK", "CHROOT", "SETSID",
   "GETPGRP", "KSIG", "UNPAUSE", "66", "REVIVE", "TASK_REPLY", "69",
   "70", "SIGACTION", "SIGSUSPEND", "SIGPENDING", "SIGPROCMASK",
   "SIGRETURN", "REBOOT", "77"

   };

void
minix_syscall()
{
   static char *nm[4] = {"?", "send", "receive", "sendrec"};
   char   tsks[10], syss[10];

   int   sr  = (unsigned short) elks_cpu.regs.ecx;
   int   tsk = (unsigned short) elks_cpu.regs.eax;
   int   sys = ELKS_PEEK(short, (unsigned short) elks_cpu.regs.ebx + 2);

   if (sr < 0 || sr > 3) sr = 0;
   switch(tsk)
   {
   case 0:  strcpy(tsks, "MM"); break;
   case 1:  strcpy(tsks, "FS"); break;
   default: sprintf(tsks, "task(%d)", tsk);
   }
   if( sys > 0 && sys < 77 )
      strcpy(syss, minix_names[sys]);
   else
      sprintf(syss, "%d", sys);

   fprintf(stderr, "Minix syscall %s(%s,&{%d,%s,...})\n", nm[sr], tsks, getpid(), syss);
   exit(99);
}
