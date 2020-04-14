/*
2020.04.13	Marcin.Laszewski@gmail.com	CGATEXT Demo
*/

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <cgatext.h>

void cgatext_puts(unsigned addr, unsigned short attr, char const *str)
{
	while (*str) {
		cgatext_put_cell(addr, cgatext_cell_attr_c(attr, *str));
		str++;
		addr += cgatext_cell_SIZE;
	}
}

#define	cgatext_puts_center(attr, str) \
	cgatext_puts(cgatext_offset((cgatext_COLS - strlen(str)) >> 1, cgatext_ROWS >> 1), attr, str)

#define	rand_c(first_c, last_c)	((first_c) + (rand() % ((last_c) - (first_c) + 1)))

static unsigned
buf_puts(unsigned short * buf, unsigned char fg, unsigned char bg, char const * text)
{
	unsigned n = 0;

	while(*text)
	{
		*buf++ = cgatext_cell(fg, bg, *text++);
		n += cgatext_cell_SIZE;
	}

	return n;
}

int main(void)
{
	cgatext_clear();
	cgatext_puts_center(cgatext_cell_color(cgatext_color_WHITE, cgatext_color_BLACK),
		"<<< CGATEXT Demo >>>");
	sleep(2);

	{
		char c;

		for (c = 0; c < cgatext_attr_MAX; c++)
		{
			cgatext_fill(cgatext_cell(c, cgatext_color_BLACK, c < 10 ? c + '0' : (c - 10) + 'A'));
			sleep(1);
		}
	}

	{
		struct
		{
			char	first;
			char	last;
			unsigned char	fg;
			unsigned char	bg;
		} chtab[] = {
			{' ', ' ', cgatext_color_MAX, cgatext_color_MAX				},
			{'0', '9', cgatext_attr_MAX,	cgatext_color_BLACK + 1	},
			{'A', 'Z', cgatext_attr_MAX,	cgatext_color_MAX				},
		};

		unsigned i;
		unsigned long j;

		for (i = 0; i < 3; i++)
			for (j = 0; j < 2000000; j++)
				cgatext_put_cell_xy(
					rand() % cgatext_COLS,
					rand() % cgatext_ROWS,
					cgatext_cell(
						rand() % chtab[i].fg,
						rand() % chtab[i].bg,
						rand_c(chtab[i].first, chtab[i].last)
					)
				);
	}

	{
		unsigned x, y;

		char const *color[cgatext_color_MAX] = {
			[cgatext_color_BLACK]		= "Black",
			[cgatext_color_RED]			= "Red",
			[cgatext_color_GREEN]		= "Green",
			[cgatext_color_YELLOW] 	= "Yellow",
			[cgatext_color_BLUE]		= "Blue",
			[cgatext_color_CYAN]		= "Cyan",
			[cgatext_color_MAGENTA]	= "Magenta",
			[cgatext_color_WHITE]		= "White",
		};

		for (y = 0; y < cgatext_attr_MAX; y++)
			for (x = 0; x < cgatext_color_MAX; x++)
			{
				char text[11];

				sprintf(text, "%-10s",
					(x && y)
					? "ELKS"
					: color[(x ? x : y) % cgatext_color_MAX]);
				cgatext_puts(cgatext_offset(10 * x, y), cgatext_cell_color(y, x), text);
			}
	}

	sleep(1);

	{
		char const * devname = "/dev/cgatext";

		int f = open(devname, O_RDWR);

		if(f < 0)
		{
			perror(devname);
			return 1;
		}

		{
			unsigned short buf[50];
			off_t i;

			/* Clear screen */
			*buf = cgatext_cell(cgatext_color_YELLOW | cgatext_attr_BOLD, cgatext_color_BLUE, ' ');
			while(write(f, buf, sizeof *buf) > 0);

			{
				unsigned n;

				n = buf_puts(buf,
							cgatext_color_CYAN | cgatext_attr_BOLD,
							cgatext_color_BLUE,
							"--- /dev/cgatext ---");
				lseek(f, cgatext_offset((cgatext_COLS - 19) >> 1, cgatext_ROWS >> 1), SEEK_SET);
				write(f, buf, n);
			}

			sleep(3);

			{
				unsigned short c;

				for(i = 0; i < 2000000; i++)
				{
					c = cgatext_cell(
								rand() % cgatext_attr_MAX,
								rand() % cgatext_color_MAX,
								'0' + (rand() % 10));
					lseek(f,
						cgatext_offset(rand() % cgatext_COLS, rand() % cgatext_ROWS),
						SEEK_SET);
					write(f, &c, sizeof c);
				}
			}

			lseek(f, 0, SEEK_SET);
			memset(buf, 0, sizeof(buf));
			while(write(f, buf, sizeof buf) > 0);

			lseek(f, 0, SEEK_SET);

			{
				unsigned char fg = cgatext_color_BLACK;

				for(;;)
				{
					unsigned n = buf_puts(buf, fg, cgatext_color_BLACK, "ELKS");

					if(write(f, buf, n) != n)
						break;

					if(++fg >= cgatext_attr_MAX)
						fg = cgatext_color_BLACK;

					lseek(f, cgatext_cell_SIZE, SEEK_CUR);
				}
			}

			{
				unsigned n;

				n = buf_puts(buf,
							cgatext_color_GREEN | cgatext_attr_BOLD,
							cgatext_color_BLACK,
							" -THE END- ");
				lseek(f, n, SEEK_END);
				write(f, buf, n);
			}
		}

		if(close(f) < 0)
		{
			perror("close(cgatext)");
			return 2;
		}
	}

	return 0;
}
