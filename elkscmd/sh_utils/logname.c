#include <unistd.h>

void
main ()
{
	char *cp;

	cp = getlogin ();
	if (cp)
	{
		write(STDOUT_FILENO,cp,strlen(cp));
		write(STDOUT_FILENO,"\n",1);
		exit (0);
	}
	exit (1);
}
