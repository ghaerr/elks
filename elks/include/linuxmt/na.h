#ifndef _LINUXMT_NA_H
#define _LINUXMT_NA_H

#define NANO_PATH_MAX	108

struct sockaddr_na {
	unsigned short sun_family;	/* AF_NANO */
	int sun_no;			/* address number */
};

#endif /* _LINUX_NA_H */
