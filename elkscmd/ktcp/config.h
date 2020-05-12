#ifndef CONFIG_H
#define CONFIG_H

#include <autoconf.h>

#define DEBUG_TCP	0
#define DEBUG_IP	0

#if DEBUG_TCP
#define debug_tcp	printf
#else
#define debug_tcp(...)
#endif

#if DEBUG_IP
#define debug_ip	printf
#else
#define debug_ip(...)
#endif

#endif
