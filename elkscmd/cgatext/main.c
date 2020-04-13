/*
2020.04.13	Marcin.Laszewski@gmail.com	CGATEXT Demo
*/

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
}
