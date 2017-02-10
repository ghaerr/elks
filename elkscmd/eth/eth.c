
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>


#define BUF_LEN 256

static char echo_buf [BUF_LEN];

static char * cry = "Heee hooo somebody here ?";


int main ()
	{
	int ret;
	int fd = -1;

	while (1)
		{
		fd = open ("/dev/eth", O_RDWR);
		if (fd < 0)
			{
			perror ("open /dev/eth");
			ret = 1;
			break;
			}

		ret = write (fd, cry, strlen (cry));
		if (ret < 0)
			{
			perror ("write /dev/eth");
			ret = 1;
			break;
			}

		ret = read (fd, echo_buf, BUF_LEN);
		if (ret < 0)
			{
			perror ("read /dev/eth");
			ret = 1;
			break;
			}

		ret = strncmp (echo_buf, cry, strlen (cry));
		printf ("cry and echo are %s\n", ret ? "different" : "the same");
		break;
		}

	if (fd >= 0)
		{
		close (fd);
		}

	return ret;
	}
