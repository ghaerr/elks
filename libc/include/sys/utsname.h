#ifndef _SYS_UTSNAME_H
#define _SYS_UTSNAME_H

#include <features.h>
#include <sys/types.h>
#include __SYSINC__(utsname.h)

int uname(struct utsname *name);

#endif
