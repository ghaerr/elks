/*
 * Simplistic mapping between Linux process ids and ELKS process ids.  We
 * kind of need this because ELKS pids should never exceed 0x7FFF, while
 * Linux pids --- at least on x86-64 --- can and do.
 */

#include <search.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include "elks.h"

#define MAX_ELKS_PID	0x7FFF

static pid_t linux_pid[MAX_ELKS_PID + 1U];

void elks_pid_init(void)
{
	unsigned h;
	memset(linux_pid, 0, sizeof linux_pid);
}

static int16_t hash(pid_t lpid)
{
	/* Hash a Linux pid value to an integer in [2 ... MAX_ELKS_PID].  We
	   try to map Linux pid {lpid} to ELKS pid {hash(lpid)} as far as
	   possible. */
	return (unsigned long)(lpid - 2) % (MAX_ELKS_PID - 1) + 2;
}

pid_t elks_to_linux_pid(int16_t epid)
{
	pid_t lpid;
	/* Pids 0 and 1 are special, as are negative pids. */
	if (epid <= 1)
		return epid;
	/* If no Linux pid is known for this ELKS pid, assume that it is
	   referring to a Linux pid.  Otherwise, use the known Linux pid.  */
	lpid = linux_pid[epid];
	if (!lpid)
		linux_pid[epid] = lpid = epid;
	/* In any case, test if the Linux process referred to by the Linux
	   pid actually exists...  */
	if (kill(lpid, 0) == -1)
		fprintf(stderr, "Linux pid %ld (for ELKS pid %d) "
				"no longer exists!\n", (long)lpid, (int)epid);
	return lpid;
}

int16_t linux_to_elks_pid(pid_t lpid)
{
	unsigned i;
	int16_t h, epid;
	if (lpid <= 1)
		return lpid;
	h = hash(lpid);
	if (linux_pid[h] == lpid)
		return h;
	/* Not at the expected hash slot.  Do a linear probe until we find
	   lpid, or we find an empty slot, whichever comes first.  */
	epid = h;
	while (linux_pid[epid] != 0) {
		if (linux_pid[epid] == lpid)
			return epid;
		if (epid != MAX_ELKS_PID)
			++epid;
		else
			epid = 2;
		if (epid == h) {
			fprintf(stderr, "Cannot map Linux pid %ld to an "
					"ELKS pid!\n", (long)lpid);
			exit(255);
		}
	}
	linux_pid[epid] = lpid;
	return epid;
}
