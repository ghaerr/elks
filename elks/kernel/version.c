#include <linuxmt/types.h>
#include <linuxmt/utsname.h>
#include <linuxmt/version.h>
#include <linuxmt/config.h>
#include <linuxmt/compile.h>

struct utsname system_utsname=
{
	UTS_SYSNAME,
	UTS_NODENAME,
	UTS_RELEASE,
	UTS_VERSION,
	UTS_MACHINE
};
