/*  Copyright 1997,1999,2001,2002,2007,2009 Alain Knaff.
 *  This file is part of mtools.
 *
 *  Mtools is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Mtools is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Mtools.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "sysincludes.h"
#include "msdos.h"
#include "mtools.h"

int noPrivileges=0;

#ifdef OS_mingw32msvc
void reclaim_privs(void)
{
}

void drop_privs(void)
{
}

void destroy_privs(void)
{
}

uid_t get_real_uid(void)
{
	return 0;
}

void init_privs(void)
{
}

void closeExec(int fd)
{
}

#else
/*#define PRIV_DEBUG*/

#if 0
#undef HAVE_SETEUID
#define HAVE_SETRESUID
#include <asm/unistd.h>
int setresuid(int a, int b, int c)
{
	syscall(164, a, b, c);

}
#endif

static __inline__ void print_privs(const char *message UNUSEDP)
{
#ifdef PRIV_DEBUG
	/* for debugging purposes only */
	fprintf(stderr,"%s egid=%d rgid=%d\n", message, getegid(), getgid());
	fprintf(stderr,"%s euid=%d ruid=%d\n", message, geteuid(), getuid());
#endif
}


static gid_t rgid, egid;
static uid_t ruid, euid;

/* privilege management routines for SunOS and Solaris.  These are
 * needed in order to issue raw SCSI read/write ioctls.  Mtools drops
 * its privileges at the beginning, and reclaims them just for the
 * above-mentioned ioctl's.  Before popen(), exec() or system, it
 * drops its privileges completely, and issues a warning.
 */


/* group id handling is lots easier, as long as we don't use group 0.
 * If you want to use group id's, create a *new* group mtools or
 * floppy.  Chgrp any devices that you only want to be accessible to
 * mtools to this group, and give them the appropriate privs.  Make
 * sure this group doesn't own any other files: be aware that any user
 * with access to mtools may mformat these files!
 */


static __inline__ void Setuid(uid_t uid)
{
#if defined HAVE_SETEUID || defined HAVE_SETRESUID
	if(euid == 0) {
#ifdef HAVE_SETEUID
		seteuid(uid);
#else
		setresuid(ruid, uid, euid);
#endif
	} else
#endif
		setuid(uid);
}

/* In reclaim_privs and drop privs, we have to manipulate group privileges
 * when having no root privileges, else we might lose them */

void reclaim_privs(void)
{
	if(noPrivileges)
		return;
	setgid(egid);
	Setuid(euid);
	print_privs("after reclaim privs, both uids should be 0 ");
}

void drop_privs(void)
{
	Setuid(ruid);
	setgid(rgid);
	print_privs("after drop_privs, real should be 0, effective should not ");
}

void destroy_privs(void)
{

#if defined HAVE_SETEUID || defined HAVE_SETRESUID
	if(euid == 0) {
#ifdef HAVE_SETEUID
		setuid(0); /* get the necessary privs to drop real root id */
		setuid(ruid); /* this should be enough to get rid of the three
			       * ids */
		seteuid(ruid); /* for good measure... just in case we came
				* across a system which implemented sane
				* semantics instead of POSIXly broken
				* semantics for setuid */
#else
		setresuid(ruid, ruid, ruid);
#endif
	}
#endif

	/* we also destroy group privileges */
	drop_privs();

	/* saved set [ug]id will go away by itself on exec */

	print_privs("destroy_privs, no uid should be zero  ");
}


uid_t get_real_uid(void)
{
	return ruid;
}

void init_privs(void)
{
	euid = geteuid();
	ruid = getuid();
	egid = getegid();
	rgid = getgid();

#ifndef F_SETFD
	if(euid != ruid) {
		fprintf(stderr,
			"Setuid installation not supported on this platform\n");
		fprintf(stderr,
			"Missing F_SETFD");
		exit(1);
	}
#endif

	if(euid != ruid) {
		unsetenv("SOURCE_DATE_EPOCH");
	}
	if(euid == 0 && ruid != 0) {
#ifdef HAVE_SETEUID
		setuid(0); /* set real uid to 0 */
#else
#ifndef HAVE_SETRESUID
		/* on this machine, it is not possible to reversibly drop
		 * root privileges.  We print an error and quit */

		/* BEOS is no longer a special case, as both euid and ruid
		 * return 0, and thus we do not get any longer into this
		 * branch */
		fprintf(stderr,
			"Seteuid call not supported on this architecture.\n");
		fprintf(stderr,
			"Mtools cannot be installed setuid root.\n");
		fprintf(stderr,
			"However, it can be installed setuid to a non root");
		fprintf(stderr,
			"user or setgid to any id.\n");
		exit(1);
#endif
#endif
	}
	
	drop_privs();
	print_privs("after init, real should be 0, effective should not ");
}

void closeExec(int fd)
{
#ifdef F_SETFD
	fcntl(fd, F_SETFD, 1);
#endif
}
#endif
