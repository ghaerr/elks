/* mou_tp.h */

#define TRANSFORMATION_UNITS_PER_PIXEL 4

typedef struct
{
	/*
	 * Coefficients for the transformation formulas:
	 *
	 *     m = (ax + by + c) / s
	 *     n = (dx + ey + f) / s
	 *
	 * These formulas will transform a device point (x, y) to a
	 * screen point (m, n) in fractional pixels.  The fraction
	 * is 1 / TRANSFORMATION_UNITS_PER_PIXEL.
	 */

	int a, b, c, d, e, f, s;
} TRANSFORMATION_COEFFICIENTS;
