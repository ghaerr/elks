#pragma once

struct timeval;

int select (int __nfds, fd_set * restrict __readfds,
	fd_set * restrict __writefds, fd_set * restrict __errorfds,
	struct timeval * restrict __timeout);
