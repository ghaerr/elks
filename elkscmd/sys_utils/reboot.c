#include <stdio.h>
#include <unistd.h>
#include <errno.h>

int main(argc,argv)
int argc;
char ** argv;
{
	sync();
	if (umount("/")) {
		perror("reboot umount");
		return(errno);
	}
	sleep(3);
	if (reboot(0x1D1E,0xC0DE,0x0123)) {
		perror("reboot");
		return(errno);
	}
}
	
