#include <termios.h>

static struct termios orgtermios;

main()
{
	int	n;
	char	buf[256];

	setraw(0);
	for (;;) {
		n = read(0, buf, sizeof(buf));
		if (n > 0) {
			if (buf[0] == '!')
				break;
			write(1, buf, n);
		}
	}
	setcooked(0);
}

setraw(fd)
int fd;
{
	struct termios	termios;

	tcgetattr(fd, &orgtermios);

	termios = orgtermios;
	termios.c_iflag &= (IXON|IXOFF|IXANY|ISTRIP|IGNBRK);
	termios.c_oflag &= ~OPOST;
	termios.c_lflag &= ISIG;
	termios.c_cc[VINTR] = 3;
	termios.c_cc[VMIN] = 1;
	termios.c_cc[VTIME] = 0;
	tcsetattr(2, TCSANOW, &termios);
}

setcooked(fd)
int fd;
{
	tcsetattr(fd, TCSAFLUSH, &orgtermios);
}
