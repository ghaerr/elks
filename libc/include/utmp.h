#ifndef __UTMP_H
#define __UTMP_H

#include <sys/types.h>
#include <paths.h>
#include <time.h>

#define UT_UNKNOWN 0
#define UT_LINESIZE 12
#define UT_NAMESIZE 8
#define UT_HOSTSIZE 16

#define RUN_LVL 1
#define BOOT_TIME 2
#define NEW_TIME 3
#define OLD_TIME 4

#define INIT_PROCESS 5
#define LOGIN_PROCESS 6
#define USER_PROCESS 7
#define DEAD_PROCESS 8

struct utmp
{
  short   ut_type;                 /* type of login */
  pid_t   ut_pid;                  /* pid of login-process */
  char    ut_line[UT_LINESIZE];    /* devicename of tty -"/dev/", null-term */
  char    ut_id[2];                /* abbrev. ttyname, as 01, s1 etc. */
  time_t  ut_time;                 /* login time */
  char    ut_user[UT_NAMESIZE];    /* username, not null-term */
  char    ut_host[UT_HOSTSIZE];    /* hostname for remote login... */
  long    ut_addr;                 /* IP addr of remote host */

};

void             setutent(void);
void             utmpname(const char *);
struct utmp *    getutent(void);
struct utmp *    getutid(const struct utmp *);
struct utmp *    getutline(const struct utmp *);
struct utmp *    pututline(const struct utmp *);
void             endutent(void);

#ifdef __LIBC__
struct utmp *    __getutent(int);
#endif

#endif /* __UTMP_H */

