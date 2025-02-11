#ifndef __LINUXMT_UN_H
#define __LINUXMT_UN_H

#define UNIX_PATH_MAX   16          /* keep size of unix_proto_data small, incl/NUL */

struct sockaddr_un {
    unsigned short sun_family;		/* AF_UNIX */
    char sun_path[UNIX_PATH_MAX];	/* pathname */
};

#endif
