#ifndef __LINUXMT_DIRENT_H
#define __LINUXMT_DIRENT_H

#include <linuxmt/types.h>

struct dirent {
    u_ino_t d_ino;
    off_t d_offset;
    __u16 d_namlen;
    char d_name[255+1];
};

#endif
