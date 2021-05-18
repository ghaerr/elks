/*
2020.05.16 Marcin.Laszewski@gmail.com PORT Demo
*/

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define	LPT_PORT	0x378

static char const *dev_port = "/dev/port";

int main(void)
{
	int f;
	off_t l;
	ssize_t i;
	unsigned char value;

	f = open(dev_port, O_WRONLY);
	if (f < 0)
	{
		perror(dev_port);
		return 1;
	}

	for (value = 0; ; value++)
	{
		l = lseek(f, LPT_PORT, 0);
		if (l < 0)
		{
			perror("lseek");
			return 2;
		}

		if (l != LPT_PORT)
		{
			fprintf(stderr, "lseek: Incorrect return value: lseek(%d) = %ld",
				LPT_PORT, (long)l);

			return 3;
		}

		printf("write(0x%02X) = %u\n", LPT_PORT, value);
		i = write(f, &value, sizeof(value));
		if (i < 0)
		{
			perror("write()");
			return 4;
		}

		if (i != sizeof(value))
		{
			fprintf(stderr, "write: Incorrect return value: write(%d) = %d",
				sizeof(value), i);
		}

		/*sleep(1);*/
	}
}
