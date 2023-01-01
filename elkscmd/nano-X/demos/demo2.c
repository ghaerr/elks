#include <stdio.h>
#include <stdlib.h>
#include "nano-X.h"

int main()
{
	GR_WINDOW_ID 	window;
	GR_EVENT 	event;

	if (GrOpen() < 0) {
		exit(1);
	}

	window = GrNewWindow(GR_ROOT_WINDOW_ID, 20, 20, 100, 60, 4, WHITE, BLUE);

	GrMapWindow(window);

	while(1)
		GrCheckNextEvent(&event);

	GrClose();
}

