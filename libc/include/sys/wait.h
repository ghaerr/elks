#ifndef	_SYS_WAIT_H
#define	_SYS_WAIT_H

/* Bits in the third argument to `waitpid'.  */
#define	WNOHANG		1	/* Don't block waiting.  */
#define	WUNTRACED	2	/* Report status of stopped children.  */

/* Everything extant so far uses these same bits.  */
/* If WIFEXITED(STATUS), the low-order 8 bits of the status.  */
#define	WEXITSTATUS(status)	(((status) & 0xff00) >> 8)

/* If WIFSIGNALED(STATUS), the terminating signal.  */
#define	WTERMSIG(status)	((status) & 0x7f)

/* If WIFSTOPPED(STATUS), the signal that stopped the child.  */
#define	WSTOPSIG(status)	WEXITSTATUS(status)

/* Nonzero if STATUS indicates normal termination.  */
#define WIFEXITED(status)	(((status) & 0xff) == 0)

/* Nonzero if STATUS indicates termination by a signal.  */
#define WIFSIGNALED(status)	(((unsigned int)((status)-1) & 0xFFFF) < 0xFF)

/* Nonzero if STATUS indicates the child is stopped.  */
#define	WIFSTOPPED(status)	(((status) & 0xff) == 0x7f)

/* Nonzero if STATUS indicates the child dumped core.  */
#define	WCOREDUMP(status)	((status) & 0200)

/* Macros for constructing status values.  */
#define	W_EXITCODE(ret, sig)	((ret) << 8 | (sig))
#define	W_STOPCODE(sig)	((sig) << 8 | 0x7f)

/* Wait for a child to die.  When one does, put its status in *STAT_LOC
   and return its process ID.  For errors, return (pid_t) -1.  */
extern pid_t wait (int * stat_loc);

/* Special values for the PID argument to `waitpid' and `wait4'.  */
#define	WAIT_ANY	(-1)	/* Any process.  */
#define	WAIT_MYPGRP	0	/* Any process in my process group.  */

/* Wait for a child matching PID to die.
   If PID is greater than 0, match any process whose process ID is PID.
   If PID is (pid_t) -1, match any process.
   If PID is (pid_t) 0, match any process with the
   same process group as the current process.
   If PID is less than -1, match any process whose
   process group is the absolute value of PID.
   If the WNOHANG bit is set in OPTIONS, and that child
   is not already dead, return (pid_t) 0.  If successful,
   return PID and store the dead child's status in STAT_LOC.
   Return (pid_t) -1 for errors.  If the WUNTRACED bit is
   set in OPTIONS, return status for stopped children; otherwise don't.  */

pid_t waitpid(pid_t __pid, int *__stat_loc, int __options);

/* This being here makes the prototypes valid whether or not
   we have already included <sys/resource.h> to define `struct rusage'.  */
struct rusage;

/* Wait for a child to exit.  When one does, put its status in *STAT_LOC and
   return its process ID.  For errors return (pid_t) -1.  If USAGE is not
   nil, store information about the child's resource usage there.  If the
   WUNTRACED bit is set in OPTIONS, return status for stopped children;
   otherwise don't.  */
pid_t wait3(int * __stat_loc, int __options, struct rusage * __usage);

/* PID is like waitpid.  Other args are like wait3.  */
pid_t wait4(pid_t __pid, int * __stat_loc, int __options, struct rusage *__usage);

#endif /* sys/wait.h  */
