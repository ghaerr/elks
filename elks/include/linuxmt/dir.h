#ifndef LX86_ARCH_DIR_H
#define LX86_ARCH_DIR_H

#include <linuxmt/types.h>

struct dirent {
    u_ino_t d_ino;
    off_t d_offset;
    __u16 d_namlen;
    char d_name[255+1];
};

#endif
