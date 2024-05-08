#ifndef __SYS_SYSCTL_H
#define __SYS_SYSCTL_H

#include <features.h>
#include __SYSINC__(sysctl.h)

int sysctl(int op, char *name, int *value);

#endif
