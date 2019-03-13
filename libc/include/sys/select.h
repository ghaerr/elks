#pragma once

int select (int nfds, fd_set * restrict readfds,
	fd_set * restrict writefds, fd_set * restrict errorfds,
	struct timeval * restrict timeout);
