#ifndef LX86_LINUXMT_UN_H
#define LX86_LINUXMT_UN_H

#define UNIX_PATH_MAX	108

struct sockaddr_un {
    unsigned short sun_family;		/* AF_UNIX */
    char sun_path[UNIX_PATH_MAX];	/* pathname */
};

#endif
