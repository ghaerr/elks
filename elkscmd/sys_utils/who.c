#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <utmp.h>

void main(argc, argv)
int argc;
char ** argv;
{
	register struct utmp * entry;
	char * timestr;

	setutent();
	while ((entry = getutent()) != NULL) {
		if (entry->ut_type != USER_PROCESS)
			continue;
		timestr = ctime(&entry->ut_time);
		timestr[strlen(timestr) - 1] = '\0';
		printf("%s	tty%c%c	%s %s\n", entry->ut_user,
					entry->ut_id[0],
					entry->ut_id[1] ? entry->ut_id[1] : 0,
					timestr,
					entry->ut_host);
	}
	exit(0);
}
