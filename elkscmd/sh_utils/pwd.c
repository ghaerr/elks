#include <unistd.h>
#include <string.h>

void main ()
{
	char wd[255];
	int i;
	
	getcwd(wd,255);
	i = strlen(wd);
	write(STDOUT_FILENO,wd,i);
	write(STDOUT_FILENO,"\n",1);
}
