#include <unistd.h>
#include <string.h>
#include <pwd.h>
#include <sys/types.h>

int
main(int ac, char **av)
{
	register struct passwd * upw = getpwuid(getuid());

	if (upw) {
		write(STDOUT_FILENO,upw->pw_name,strlen(upw->pw_name));
		write(STDOUT_FILENO,"\n",1);
		return 0;
	}
	return 1;
}
