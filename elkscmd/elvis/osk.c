/* osk.c */

/* ------------------------------------------------------------------- *
 |
 | OS9Lib:  stat(), fstat()
 |
 |
 |     Copyright (c) 1988 by Wolfgang Ocker, Puchheim,
 |                           Ulli Dessauer, Germering and
 |                           Reimer Mellin, Muenchen
 |                           (W-Germany)
 |
 |  This  programm can  be  copied and  distributed freely  for any
 |  non-commercial  purposes.   It can only  be  incorporated  into
 |  commercial software with the written permission of the authors.
 |
 |  If you should modify this program, the authors would appreciate
 |  a notice about the changes. Please send a (context) diff or the
 |  complete source to:
 |
 |  address:     Wolfgang Ocker
 |               Lochhauserstrasse 35a
 |               D-8039 Puchheim
 |               West Germany
 |
 |  e-mail:      weo@altger.UUCP, ud@altger.UUCP, ram@altger.UUCP
 |               pyramid!tmpmbx!recco!weo
 |               pyramid!tmpmbx!nitmar!ud
 |               pyramid!tmpmbx!ramsys!ram
 |
 * ----------------------------------------------------------------- */

#ifdef OSK

#define PATCHLEVEL 1

#ifndef VIREC
#include <stdio.h>
#include "osk.h"
#include <modes.h>
#include <errno.h>
#endif
#include <signal.h>
#include <sgstat.h>
#include <sg_codes.h>
#include <direct.h>

/*
 * f s t a t
 */
int fstat(fd, buff)
  int         fd;
  struct stat *buff;
{
  struct fildes ftmp;
  struct tm     ttmp;
  struct _sgr   fopt;

  if (_gs_gfd(fd, &ftmp, 16) < 0) /* 16 insteat of sizeof(struct fildes)   */
    return(-1);                   /* used due to a bug in stupid os9net */

  if (_gs_opt(fd, &fopt) < 0)
    return(-1);

  ttmp.tm_year  = (int) ftmp.fd_date[0];
  ttmp.tm_mon   = (int) ftmp.fd_date[1] - 1;
  ttmp.tm_mday  = (int) ftmp.fd_date[2];	
  ttmp.tm_hour  = (int) ftmp.fd_date[3];
  ttmp.tm_min   = (int) ftmp.fd_date[4];
  ttmp.tm_sec   = 0;
  ttmp.tm_isdst = -1;

  buff->st_atime = buff->st_mtime = mktime(&ttmp);

  ttmp.tm_year  = (int) ftmp.fd_dcr[0];
  ttmp.tm_mon   = (int) ftmp.fd_dcr[1] - 1;
  ttmp.tm_mday  = (int) ftmp.fd_dcr[2];	
  ttmp.tm_hour  = ttmp.tm_min = ttmp.tm_sec = 0;
  ttmp.tm_isdst = -1;
  
  buff->st_ctime = mktime(&ttmp);

  memcpy(&(buff->st_size), ftmp.fd_fsize, sizeof(long));  /* misalignment! */
  buff->st_uid   = ftmp.fd_own[1];
  buff->st_gid   = ftmp.fd_own[0];
  buff->st_mode  = ftmp.fd_att;
  buff->st_nlink = ftmp.fd_link;

  buff->st_ino   = fopt._sgr_fdpsn;
  buff->st_dev   = fopt._sgr_dvt;

  return(0);
}

/*
 * s t a t
 */	
int stat(filename, buff)
  char        *filename;
  struct stat *buff;
{
  register int i, ret;

  if ((i = open(filename, S_IREAD)) < 0)
    if ((i = open(filename, S_IFDIR | S_IREAD)) < 0)
      return(-1);

  ret = fstat(i, buff);
  close(i);

  return(ret);
}

/*
	unix library functions mist in OSK
	Author: Peter Reinig
*/

/* NOTE: this version of link() is only capable of renaming files, not true
 * UNIX-style linking.  That's okay, though, because elvis only uses it for
 * renaming.
 */
link(from,to)
char *from,*to;
{
	char *buffer;
	int status;
	char *malloc();

	if ((buffer = malloc(strlen(from) + strlen(to) + 12)) == NULL)
		return -1;
	sprintf(buffer,"rename %s %s\n",from,to);
	status = system(buffer);
	free(buffer);
	return status;
}

typedef (*procref)();
#define MAX_SIGNAL 10

extern exit();

static int (*sig_table[MAX_SIGNAL])();
static int _sig_install = 0;

sig_handler(sig)
int sig;
{
	if ((int) sig_table[sig] > MAX_SIGNAL)
		sig_table[sig](sig);
}

procref signal(sig,func)
int sig;
int (*func)();
{
	int i, (*sav)();

	if (!_sig_install) {
		for (i=0; i < MAX_SIGNAL; i++)
			sig_table[i] = exit;
		_sig_install = 1;
		intercept(sig_handler);
	}	
	sav = sig_table[sig];
	switch ((int) func) {
		case SIG_DFL : sig_table[sig] = exit;
					   break;
		case SIG_IGN : sig_table[sig] = 0;
					   break;
		default      : sig_table[sig] = func;
					   break;
	}
	return sav;
}

perror(str)
char *str;
{
	static int path = 0;
	if (!path && (path = open("/dd/sys/Errmsg", S_IREAD)) == -1) {
		fprintf(stderr,"Can\'t open error message file\n");
		path = 0;
	}
	if (str && *str) {
		fprintf(stderr,"%s: ",str);
		fflush(stderr);
	}
	prerr(path,(short) errno);
}

isatty(fd)
int fd;
{
	struct sgbuf buffer;
	char type;

	_gs_opt(fd,&buffer);
	type = buffer.sg_class;
	if (type == DT_SCF)
		return 1;
	else
		return 0;
}
#endif /* OSK */
