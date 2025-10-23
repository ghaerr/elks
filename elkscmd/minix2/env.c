/*	env 1.0 - Set environment for command		Author: Kees J. Bot
 *								17 Dec 1997
 */
#define nil 0
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#define outmsg(str) write(STDOUT_FILENO, str, sizeof(str) - 1)
#define outstr(str) write(STDOUT_FILENO, str, strlen(str))
#define errmsg(str) write(STDERR_FILENO, str, sizeof(str) - 1)
#define errstr(str) write(STDERR_FILENO, str, strlen(str))

int main(int argc, char **argv)
{
	int i;
	int iflag= 0;
	int aflag= 0;

	i= 1;
	while (i < argc && argv[i][0] == '-') {
		char *opt= argv[i++] + 1;

		if (opt[0] == '-' && opt[1] == 0) break;	/* -- */

		if (opt[0] == 0) iflag= 1;			/* - */

		while (*opt != 0) switch (*opt++) {
		case 'i':
			iflag= 1;	/* Clear environment. */
			break;
		case 'a':		/* Specify arg 0 separately. */
			aflag= 1;
			break;
		default:
			errmsg("Usage: env [-ia] [name=value] ... [utility [args ...]]\n");
			return 1;
		}
	}

	/* Clear the environment if -i. */
	if (iflag) *environ= nil;

	/* Set the new environment strings. */
	while (i < argc && strchr(argv[i], '=') != nil) {
		if (putenv(argv[i]) != 0) {
			errmsg("env: Setting '");
			errstr(argv[i]);
			errmsg("' failed\n");
			return 1;
		}
		i++;
	}

	if (i >= argc) {
		/* No utility given; print environment. */
		char **ep;

		for (ep= environ; *ep != nil; ep++) {
			outstr(*ep);
                        outmsg("\n");
                }
		return 0;
	} else {
		char *util, **args;
		int err;

		util= argv[i];
		args= argv + i;
		if (aflag) args++;
		(void) execvp(util, args);
		err= errno;
		errmsg("env: Can't execute ");
		errstr(util);
		errmsg("\n");
		return err == ENOENT ? 127 : 126;
	}
}
