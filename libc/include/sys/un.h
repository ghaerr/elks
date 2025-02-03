#ifndef __SYS_UN_H
#define __SYS_UN_H

#include <features.h>
#include __SYSINC__(un.h)

#define SUN_LEN(ptr)  (offsetof(struct sockaddr_un, sun_path) + strlen((ptr)->sun_path))

#endif
