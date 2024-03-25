/*
 *	Protocol initialiser table.
 */

#include <linuxmt/config.h>
#include <linuxmt/net.h>
#include <linuxmt/fs.h>

extern void inet_proto_init();
extern void unix_proto_init();
extern void nano_proto_init();

struct net_proto protocols[] = {
#ifdef	CONFIG_UNIX
    {"UNIX", unix_proto_init},
#endif
#ifdef	CONFIG_NANO
    {"NANO", nano_proto_init},
#endif
#ifdef	CONFIG_INET
    {"INET", inet_proto_init},
#endif
    {NULL, NULL}
};
