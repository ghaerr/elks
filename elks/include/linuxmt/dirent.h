#ifndef __LINUXMT_DIRENT_H
#define __LINUXMT_DIRENT_H

#include <linuxmt/types.h>
#include <linuxmt/limits.h>

struct dirent {
    ino_t           d_ino;
    off_t           d_offset;
    unsigned short  d_namlen;
    char            d_name[MAXNAMLEN+1];
};

#endif
