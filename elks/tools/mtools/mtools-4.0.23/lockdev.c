/*  Copyright 2005,2009,2018 Alain Knaff.
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
 *
 * Create an advisory lock on the device to prevent concurrent writes.
 * Uses either lockf, flock, or fcntl locking methods.  See the Makefile
 * and the Configure files for how to specify the proper method.
 */

#include "sysincludes.h"
#include "mtools.h"
#include "lockdev.h"

#if (defined HAVE_SIGACTION && defined HAVE_ALARM)
# define ALRM
#endif


#if (defined(HAVE_FLOCK) && defined (LOCK_EX) && (defined(LOCK_NB) || defined(ALRM)))
     
# ifdef ALRM
#  define USE_FLOCK_W
# else
#  define USE_FLOCK
# endif

#else /* FLOCK */

#if (defined(HAVE_LOCKF) && (defined(F_TLOCK) || defined(ALRM)))

# ifdef ALRM
#  define USE_LOCKF_W
# else
#  define USE_LOCKF
# endif
     
#else /* LOCKF */

#if (defined(F_SETLK) && defined(F_WRLCK))

# if (defined ALRM && defined F_SETLKW)
#  define USE_SETLK_W
# else
#  define USE_SETLK_W
# endif

#else

#endif /* FCNTL */
#endif /* LOCKF */
#endif /* FLOCK */

#if  defined(USE_FLOCK_W) || defined(USE_LOCKF_W) || defined (USE_SETLK_W)
static void alrm(int a UNUSEDP) {
}
#endif

int lock_dev(int fd, int mode, struct device *dev)
{
	unsigned int retries = 0;
	if(IS_NOLOCK(dev))
		return 0;

	while(1) {
		int ret=0;
#if defined(USE_FLOCK_W) || defined(USE_LOCKF_W) || defined (USE_SETLK_W)
		struct sigaction alrm_action, old_alrm_action;
		int old_alrm = alarm(0);
		memset(&alrm_action, 0, sizeof(alrm_action));
		alrm_action.sa_handler = alrm;
		alrm_action.sa_flags = 0;
		sigaction(SIGALRM, &alrm_action, &old_alrm_action);
		alarm(mtools_lock_timeout);
#endif

#ifdef USE_FLOCK
		ret = flock(fd, (mode ? LOCK_EX : LOCK_SH)|LOCK_NB);
#endif
		
#ifdef USE_FLOCK_W
		ret = flock(fd, (mode ? LOCK_EX : LOCK_SH));
#endif

#if (defined(USE_LOCKF) || defined(USE_LOCKF_W))
		if(mode)
# ifdef USE_LOCKF
			ret = lockf(fd, F_TLOCK, 0);
# else
			ret = lockf(fd, F_LOCK, 0);
# endif
		else
			ret = 0;
#endif
		
#if (defined(USE_SETLK) || defined(USE_SETLK_W))
		{
			struct flock flk;
			flk.l_type = mode ? F_WRLCK : F_RDLCK;
			flk.l_whence = 0;
			flk.l_start = 0L;
			flk.l_len = 0L;

# ifdef USE_SETLK_W
			ret = fcntl(fd, F_SETLKW, &flk);
# else
			ret = fcntl(fd, F_SETLK, &flk);
# endif
		}
#endif
		
#if defined(USE_FLOCK_W) || defined(USE_LOCKF_W) || defined (USE_SETLK_W)
		/* Cancel the alarm */
		sigaction(SIGALRM, &old_alrm_action, NULL);
		alarm(old_alrm);
#endif

		if(ret < 0) {
#if defined(USE_FLOCK_W) || defined(USE_LOCKF_W) || defined (USE_SETLK_W)
			/* ALARM fired ==> this means we are still locked */
			if(errno == EINTR) {
				return 1;
			}
#endif
			
			if(
#ifdef EWOULDBLOCK
				(errno != EWOULDBLOCK)
#else
				1
#endif
				&&
#ifdef EAGAIN
				(errno != EAGAIN)
#else
				1
#endif
				&&
#ifdef EINTR
				(errno != EINTR)
#else
				1
#endif
			) {
				/* Error other than simply being locked */
				return -1;
			}
			/* Locked ==> continue until timeout */
		} else /* no error => we got the lock! */
			return 0;

#ifdef HAVE_USLEEP
		if(retries++ < mtools_lock_timeout * 10)
			usleep(100000);
#else
		if(retries++ < mtools_lock_timeout)
			sleep(1);
#endif
		else
			/* waited for too long => give up */
			return 1;
	}
}
