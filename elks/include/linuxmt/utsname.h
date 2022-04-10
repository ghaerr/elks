#ifndef __LINUXMT_UTSNAME_H
#define __LINUXMT_UTSNAME_H

struct utsname {
    char sysname[8];
    char nodename[16];
    char release[12];
    char version[48];
    char machine[16];
};

#ifdef __KERNEL__
extern struct utsname system_utsname;
#endif

#endif
