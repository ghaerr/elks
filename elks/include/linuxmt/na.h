#ifndef __LINUXMT_NA_H__
#define __LINUXMT_NA_H__

struct sockaddr_na {
	unsigned short sun_family;	/* AF_NANO */
	int sun_no;			/* address number */
};

#endif /* _LINUX_NA_H */
