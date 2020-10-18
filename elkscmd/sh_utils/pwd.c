#include <unistd.h>
#include <string.h>

int main(int ac, char **av)
{
	char wd[255];
	
	if (getcwd(wd,255) == NULL) {
		write(STDOUT_FILENO, "Cannot get current directory\n", 28);
		return 1;
	}
	strcat(wd, "\n");
	write(STDOUT_FILENO, wd, strlen(wd));
	return 0;
}
