/*
 * drivers/mou_tp.c
 *
 * Touch-panel driver
 *
 * Designed for for use with the Linux VR project touch-panel kernel driver.
 *
 * Copyright (C) 1999 Bradley D. LaRonde <brad@ltc.com>
 * Portions Copyright (c) 1999 Greg Haerr <greg@censoft.com>
 * Portions Copyright (c) 1991 David I. Bell
 *
 * Permission is granted to use, distribute, or modify this source,
 * provided that this copyright notice remains intact.
 *
 * This program is free software; you may redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <sys/ioctl.h>
#include <linux/tpanel.h>
#include "../device.h"
#include "mou_tp.h"

/* file descriptor for touch panel */
static int pd_fd;

/*
 * Enable absolute coordinate transformation.
 * Normally this should be left at 1.
 * To disable transformation, set it to 0 before calling GsMain().
 * This is done by the pointer calibration utility since it's used to produce the pointer
 * calibration data file.
 */
int enable_pointing_coordinate_transform = 1;

static TRANSFORMATION_COEFFICIENTS tc;

int GetPointerCalibrationData()
{
	/*
	 * Read the calibration data from the calibration file.
	 * Calibration file format is seven coefficients separated by spaces.
	 */

	/* Get pointer calibration data from this file */
	const char cal_filename[] = "/etc/pointercal";

	int items;

	FILE* f = fopen(cal_filename, "r");
	if ( f == NULL )
	{
		fprintf(stderr, "Error %d opening pointer calibration file %s.\n",
			errno, cal_filename);
		return -1;
	}

	items = fscanf(f, "%d %d %d %d %d %d %d",
		&tc.a, &tc.b, &tc.c, &tc.d, &tc.e, &tc.f, &tc.s);
	if ( items != 7 )
	{
		fprintf(stderr, "Improperly formatted pointer calibration file %s.\n",
			cal_filename);
		return -1;
	}

#if TEST
		printf("a=%d b=%d c=%d d=%d e=%d f=%d s=%d\n",
			tc.a, tc.b, tc.c, tc.d, tc.e, tc.f, tc.s);
#endif

	return 0;
}

inline XYPOINT DeviceToScreen(XYPOINT p)
{
	/*
	 * Transform device coordinates to screen coordinates.
	 * Take a point p in device coordinates and return the corresponding
	 * point in screen coodinates.
	 * This can scale, translate, rotate and/or skew, based on the coefficients
	 * calculated above based on the list of screen vs. device coordinates.
	 */

	static XYPOINT prev;
	/* set slop at 3/4 pixel */
	const short slop = TRANSFORMATION_UNITS_PER_PIXEL * 3 / 4;
	XYPOINT new, out;

	/* transform */
	new.x = (tc.a * p.x + tc.b * p.y + tc.c) / tc.s;
	new.y = (tc.d * p.x + tc.e * p.y + tc.f) / tc.s;

	/* hysteresis (thanks to John Siau) */
	if ( abs(new.x - prev.x) >= slop )
		out.x = (new.x | 0x3) ^ 0x3;
	else
		out.x = prev.x;

	if ( abs(new.y - prev.y) >= slop )
		out.y = (new.y | 0x3) ^ 0x3;
	else
		out.y = prev.y;

	prev = out;

	return out;
}

static int PD_Open(MOUSEDEVICE *pmd)
{
 	/*
	 * open up the touch-panel device.
	 * Return the fd if successful, or negative if unsuccessful.
	 */

	struct scanparam s;
	int settle_upper_limit;
	int result;

	pd_fd = open("/dev/tpanel", O_NONBLOCK);
	if (pd_fd < 0) {
		fprintf(stderr, "Error %d opening touch panel\n", errno);
		return -1;
	}

	/* set interval to 5000us (200Hz) */
	s.interval = 5000;
	/*
	 * Upper limit on settle time is approximately (scan_interval / 5) - 60
	 * (5 conversions and some fixed overhead)
	 * The opmtimal value is the lowest that doesn't cause significant distortion.
	 * 50% of upper limit works well on my Clio.  25% gets into distortion.
	 */
	settle_upper_limit = (s.interval / 5) - 60;
	s.settletime = settle_upper_limit * 50 / 100;
	result = ioctl(pd_fd, TPSETSCANPARM, &s);
	if ( result < 0 )
		fprintf(stderr, "Error %d, result %d setting scan parameters.\n", result, errno);

	if (enable_pointing_coordinate_transform)
	{
		if (GetPointerCalibrationData() < 0)
		{
			close(pd_fd);
			return -1;
		}
	}

	return pd_fd;
}

static void PD_Close(void)
{
 	/* Close the touch panel device. */
	if (pd_fd > 0)
		close(pd_fd);
	pd_fd = 0;
}

static BUTTON PD_GetButtonInfo(void)
{
 	/* get "mouse" buttons supported */
	return LBUTTON;
}

static void PD_GetDefaultAccel(int *pscale,int *pthresh)
{
	/*
	 * Get default mouse acceleration settings
	 * This doesn't make sense for a touch panel.
	 * Just return something inconspicuous for now.
	 */
	*pscale = 3;
	*pthresh = 5;
}

static int PD_Read(COORD *px, COORD *py, COORD *pz, BUTTON *pb)
{
	/*
	 * Read the tpanel state and position.
         * Returns the position data in x, y, and button data in b.
	 * Returns -1 on error.
	 * Returns 0 if no new data is available.
	 * Returns 1 if position data is relative (i.e. mice).
	 * Returns 2 if position data is absolute (i.e. touch panels).
	 * Returns 3 if position data is not available, but button data is.
	 * This routine does not block.
	 *
	 * Unlike a mouse, this driver returns absolute postions, not deltas.
	 */

	/* If z is below this value, ignore the data. */
	const int low_z_limit = 900;

	/*
	 * I do some error masking by tossing out really wild data points.
	 * Lower data_change_limit value means pointer get's "left behind" more easily.
	 * Higher value means less errors caught.
	 * The right setting of this value is just slightly higher than the number of
	 * units traversed per sample during a "quick" stroke.
	 */
	const int data_change_limit = 100;
	static int have_last_data = 0;
	static int last_data_x = 0;
	static int last_data_y = 0;

	/*
	 * Thanks to John Siau <jsiau@benchmarkmedia.com> for help with the noise filter.
	 * I use an infinite impulse response low-pass filter on the data to filter out
	 * high-frequency noise.  Results look better than a finite impulse response filter.
	 * If I understand it right, the nice thing is that the noise now acts as a dither
	 * signal that effectively increases the resolution of the a/d converter by a few bits
	 * and drops the noise level by about 10db.
	 * Please don't quote me on those db numbers however. :-)
	 * The end result is that the pointer goes from very fuzzy to much more steady.
	 * Hysteresis really calms it down in the end (elsewhere).
	 *
	 * iir_shift_bits effectively sets the number of samples used by the filter
	 * (number of samlpes is 2^iir_shift_bits).
	 * Lower iir_width means less pointer lag, higher iir_width means steadier pointer.
	 */
	const int iir_shift_bits = 3;
	const int iir_sample_depth = (1 << iir_shift_bits);
	static int iir_accum_x = 0;
	static int iir_accum_y = 0;
	static int iir_accum_z = 0;
	static int iir_count = 0;

	int data_x, data_y, data_z;

	/* read a data point */
	short data[6];
	int bytes_read;
	bytes_read = read(pd_fd, data, sizeof(data));
	if (bytes_read != sizeof(data)) {
		if (errno == EINTR || errno == EAGAIN)
			return 0;
		return -1;
	}

	/* did we lose any data? */
	if ( (data[0] & 0x2000) )
		fprintf(stderr, "Lost touch panel data\n");

	/* do we only have contact state data (no position data)? */
	if ( (data[0] & 0x8000) == 0 )
	{
		/* is it a pen-release? */
		if ( (data[0] & 0x4000) == 0 )
		{
			/* reset the limiter */
			have_last_data = 0;

			/* reset the filter */
			iir_count = 0;
			iir_accum_x = 0;
			iir_accum_y = 0;
			iir_accum_z = 0;

			/* return the pen (button) state only, */
			/* indicating that the pen is up (no buttons are down) */
			*pb = 0;
			return 3;
		}

		/* ignore pen-down since we don't know where it is */
		return 0;
	}

	/* we have position data */

	/*
	 * Combine the complementary panel readings into one value (except z)
	 * This effectively doubles the sampling freqency, reducing noise by approx 3db.
	 * Again, please don't quote the 3db figure.
	 * I think it also cancels out changes in the overall resistance of the panel
	 * such as may be caused by changes in panel temperature.
	 */
	data_x = data[2] - data[1];
	data_y = data[4] - data[3];
	data_z = data[5];

	/* isn't z big enough for valid position data? */
	if ( data_z <= low_z_limit )
		return 0;

	/* has the position changed more than we will allow? */
	if ( have_last_data )
		if ( (abs(data_x - last_data_x) > data_change_limit)
			|| ( abs(data_y - last_data_y) > data_change_limit ) )
			return 0;

	/* save last position */
	last_data_x = data_x;
	last_data_y = data_y;
	have_last_data = 1;

	/* is filter ready? */
	if ( iir_count == iir_sample_depth )
	{
		/* make room for new sample */
		iir_accum_x -= iir_accum_x >> iir_shift_bits;
		iir_accum_y -= iir_accum_y >> iir_shift_bits;
		iir_accum_z -= iir_accum_z >> iir_shift_bits;

		/* feed new sample to filter */
		iir_accum_x += data_x;
		iir_accum_y += data_y;
		iir_accum_z += data_z;

		/* transformation enabled? */
		if (enable_pointing_coordinate_transform)
		{
			/* transform x,y to screen coords */
			XYPOINT transformed = {iir_accum_x, iir_accum_y};
			transformed = DeviceToScreen(transformed);
			/*
			 * HACK: move this from quarter pixels to whole pixels for now
			 * at least until I decide on the right interface to get the
			 * quarter-pixel data up to the next layer.
			 */
			*px = transformed.x >> 2;
			*py = transformed.y >> 2;
		}
		else
		{
			/* return untransformed coords (for calibration) */
			*px = iir_accum_x;
			*py = iir_accum_y;
		}

		/* return filtered pressure */
		*pb = LBUTTON;
		*pz = iir_accum_z;

#ifdef TEST
		printf("In: %hd, %hd, %hd  Filtered: %d %d %d  Out: %d, %d, %d\n",
			data_x, data_y, data_z, iir_accum_x, iir_accum_y, iir_accum_z, *px, *py, *pz);
#endif
		return 2;
	}

	/* prime the filter */
	iir_accum_x += data_x;
	iir_accum_y += data_y;
	iir_accum_z += data_z;
	iir_count += 1;

	return 0;
}

MOUSEDEVICE mousedev = {
	PD_Open,
	PD_Close,
	PD_GetButtonInfo,
	PD_GetDefaultAccel,
	PD_Read,
	NULL
};

#ifdef TEST
int main()
{
	COORD 	x, y, z;
	BUTTON	b;
	int result;

	enable_pointing_coordinate_transform = 1;

	printf("Opening touch panel...\n");

	if((result=PD_Open(0)) < 0)
		printf("Error %d, result %d opening touch-panel\n", errno, result);

	printf("Reading touch panel...\n");

	while(1) {
		result = PD_Read(&x, &y, &z, &b);
		if( result > 0) {
			/* printf("%d,%d,%d,%d,%d\n", result, x, y, z, b); */
		}
	}
}
#endif
