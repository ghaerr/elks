#ifndef LX86_LINUXMT_UTSNAME_H
#define LX86_LINUXMT_UTSNAME_H

struct utsname {
    char sysname[16];
    char nodename[80];
    char release[16];
    char version[48];
    char machine[16];
};

extern struct utsname system_utsname;

#endif
