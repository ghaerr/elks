#include <sys/time.h>

#include "vga_dev.h"

MODE gr_mode;

static void
psleep(int num)
{
	struct timeval tv;

	tv.tv_sec = 0;
	tv.tv_usec = num * 100;

	select(0, NULL, NULL, NULL, &tv);
}

main(
	int argc,
	char ** argv)
{
	struct _screendevice device;
	SCREENINFO sinfo;
	int i,j;

	HERC_open(&device);
	for (j = 0; j < 16; j++) {
		for (i = 0; i < 320; i++) {
			HERC_drawhline(&device, 0, 400, 20+i, j);
		}
		psleep(1);
	}

	for (j = 0; j < 16; j++) {
		for (i = 0; i < 400; i++) {
			HERC_drawvline(&device, 20+i, 150, 340, (j + 2) & 0xf);
		}
		psleep(1);
	}

#if 0
	for (i = 0; i < 600; i++) {
		for (j = 0; j < 200; j++) {
			HERC_drawpixel(&device, i, j+100, HERC_readpixel(&device, i, j));
		}
	}

	for (i = 0; i < 100; i++) {
		for (j = 0; j < 100; j++) {
			HERC_drawpixel(&device, i, j, i + j & 0xf);
		}
	}
	/***for (i = 100; i < 600; i += 100) {
		for (j = 100; j < 400; j += 100) {
			herc_blit(&device, i, j, 100, 100, &device, 0, 0, 1);
		}
	}***/
	psleep(10);
#endif

	HERC_getscreeninfo(&device, &sinfo);
	
	HERC_close(&device);

	printf("[%d %d], %ld cols\n", sinfo.cols, sinfo.rows, sinfo.ncolors);
}
	
	
