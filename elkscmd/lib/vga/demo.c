#include <sys/time.h>

#include "vga_dev.h"

MODE gr_mode;

static void
psleep(int num)
{
	struct timeval tv;

	tv.tv_sec = num;
	tv.tv_usec = 0;

	select(0, NULL, NULL, NULL, &tv);
}

main(
	int argc,
	char ** argv)
{
	struct _screendevice device;
	SCREENINFO sinfo;
	int i,j;

	VGA_open(&device);
	for (j = 0; j < 16; j++) {
		for (i = 0; i < 400; i++) {
			VGA_drawhline(&device, 0, 400, 20+i, j);
		}
		psleep(1);
	}

	for (j = 0; j < 16; j++) {
		for (i = 0; i < 400; i++) {
			VGA_drawvline(&device, 20+i, 150, 350, (j + 2) & 0xf);
		}
		psleep(1);
	}

	for (i = 0; i < 600; i++) {
		for (j = 0; j < 200; j++) {
			VGA_drawpixel(&device, i, j+200,
					VGA_readpixel(&device, i, j));
		}
	}

	for (i = 0; i < 100; i++) {
		for (j = 0; j < 100; j++) {
			VGA_drawpixel(&device, i, j, i + j & 0xf);
		}
	}
	for (i = 100; i < 600; i += 100) {
		for (j = 100; j < 400; j += 100) {
			ega_blit(&device, i, j, 100, 100, &device, 0, 0, 1);
		}
	}
	psleep(10);

	VGA_getscreeninfo(&device, &sinfo);
	
	VGA_close(&device);

	printf("[%d %d], %ld cols\n", sinfo.cols, sinfo.rows, sinfo.ncolors);
}
	
	
