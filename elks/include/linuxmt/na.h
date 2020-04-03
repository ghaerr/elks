#ifndef __LINUXMT_NA_H
#define __LINUXMT_NA_H

struct sockaddr_na {
    unsigned short sun_family;	/* AF_NANO */
    int sun_no;			/* address number */
};

#endif
