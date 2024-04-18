#ifndef __SYS_SELECT_H
#define __SYS_SELECT_H

#include <sys/types.h>

struct timeval;

int select (int nfds, fd_set * restrict readfds,
	fd_set * restrict writefds, fd_set * restrict errorfds,
	struct timeval * restrict timeout);

#endif
