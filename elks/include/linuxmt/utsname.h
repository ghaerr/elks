#ifndef __LINUXMT_UTSNAME_H__
#define __LINUXMT_UTSNAME_H__

struct utsname {
	char sysname[9];
	char nodename[65];
	char release[15];
	char version[40];
	char machine[9];
};

extern struct utsname system_utsname;

#endif
