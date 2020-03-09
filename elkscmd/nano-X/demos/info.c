/*
 * Display the results of various GrGetsomethingInfo() calls.
 */
#include "nano-X.h"

int main()
{
	GR_SCREEN_INFO si;
	GR_FONT_INFO fi;
	GR_GC_INFO gi;
	int fonts, x, y;

	GrOpen();
	GrGetScreenInfo(&si);

	printf("rows = %d\n", si.rows);
	printf("cols = %d\n", si.cols);
	printf("bpp = %d\n", si.bpp);
	printf("planes = %d\n", si.planes);
	printf("ncolors = %d\n", si.ncolors);
	printf("buttons = 0x%x\n", si.buttons);
	printf("modifiers = 0x%x\n", si.modifiers);
	printf("fonts = %d\n", si.fonts);

	for (fonts = 0; fonts < si.fonts; fonts++) {
		if (!GrGetFontInfo(fonts, &fi)) {
			printf("\nfont = %d\n", fi.font);
			printf("height = %d\n", fi.height);
			printf("maxwidth = %d\n", fi.maxwidth);
			printf("baseline = %d\n", fi.baseline);
			printf("fixed = %s\n", fi.fixed ? "TRUE" : "FALSE");
			printf("widths =\n");
			for (y = 0; y != 3; y++) {
				for (x = 0; x != 7; x++)
					printf("%2d", fi.widths[x * y]);
				printf("\n");
			}
		}
	}

	GrClose();
}
