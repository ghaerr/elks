#include <linuxmt/compiler-generated.h>
#include <linuxmt/config.h>
#include <linuxmt/types.h>
#include <linuxmt/utsname.h>

struct utsname system_utsname=
{
	UTS_SYSNAME,
	UTS_NODENAME,
	UTS_RELEASE,
	UTS_VERSION,
	UTS_MACHINE
};
