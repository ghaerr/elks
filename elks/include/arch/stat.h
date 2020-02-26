#ifndef LX86_ARCH_STAT_H
#define LX86_ARCH_STAT_H

#include <linuxmt/types.h>

struct stat {
    dev_t	st_dev;
    u_ino_t	st_ino;
    mode_t	st_mode;
    nlink_t	st_nlink;
    uid_t	st_uid;
    gid_t	st_gid;
    dev_t	st_rdev;
    off_t	st_size;
    time_t	st_atime;
    time_t	st_mtime;
    time_t	st_ctime;
};

#endif
