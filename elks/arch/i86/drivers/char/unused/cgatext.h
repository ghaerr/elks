#if !defined(CGATEXT_H)
#define	CGATEXT_H

#include <linuxmt/memory.h>

/* CGA */
#define	cgatext_COLS	80
#define	cgatext_ROWS	25
#define	cgatext_CELLS	(cgatext_COLS * cgatext_ROWS)
#define	cgatext_cell_SIZE	2

#define	cgatext_mem_SEG		0xB800
#define	cgatext_mem_SIZE	(cgatext_CELLS * cgatext_cell_SIZE)


/* Video cell addressing */
#define	cgatext_offset_col(n)			((n) * cgatext_cell_SIZE)
#define	cgatext_offset_row(n)			(cgatext_offset_col(cgatext_COLS) * (n))
#define	cgatext_offset(col, row)	(cgatext_offset_col(col) + cgatext_offset_row(row))

/* Video cell displaing */
#define	cgatext_put_cell(off, cell)					pokew(off, cgatext_mem_SEG, cell)
#define	cgatext_put_cell_xy(col, row, cell)	cgatext_put_cell(cgatext_offset(col, row), cell)

/* Color -> cell */
#define	cgatext_attr_offset(a)	((a) << 8)
#define	cgatext_attr_bg(a)	cgatext_attr_offset((a) << 4)
#define	cgatext_attr_fg(a)	cgatext_attr_offset((a) << 0)
#define	cgatext_attr_BOLD		cgatext_attr_fg(1 << 3)
#define	cgatext_attr_BLINK	cgatext_attr_bg(1 << 3)

/* Video cell values */
#define	cgatext_cell_c(c)												(c)
#define	cgatext_cell_color(color_fg, color_bg)	(cgatext_attr_fg(color_fg) | cgatext_attr_bg(color_bg))
#define	cgatext_cell_attr_c(attr, c)						((attr) | cgatext_cell_c(c))
#define	cgatext_cell(color_fg, color_bg, c)			cgatext_cell_attr_c(cgatext_cell_color(color_fg, color_bg), c)

/* Color definitions */
#define	cgatext_color_RGB(r, g, b)	(((b) << 0) | ((g) << 1) | ((r) << 2))
#define	cgatext_color_BLACK		cgatext_color_RGB(0, 0, 0)
#define	cgatext_color_RED			cgatext_color_RGB(1, 0, 0)
#define	cgatext_color_GREEN		cgatext_color_RGB(0, 1, 0)
#define	cgatext_color_YELLOW	cgatext_color_RGB(1, 1, 0)
#define	cgatext_color_BLUE		cgatext_color_RGB(0, 0, 1)
#define	cgatext_color_CYAN		cgatext_color_RGB(0, 1, 1)
#define	cgatext_color_MAGENTA	cgatext_color_RGB(1, 0, 1)
#define	cgatext_color_WHITE		cgatext_color_RGB(1, 1, 1)
#define	cgatext_color_MAX			(cgatext_color_WHITE + 1)
#define	cgatext_attr_MAX			(cgatext_color_MAX << 1)

/* Function & macros */
#define	cgatext_fill(cell)	fmemsetw(0, cgatext_mem_SEG, cell, cgatext_CELLS)
#define	cgatext_clear()			cgatext_fill(cgatext_cell(cgatext_color_WHITE, cgatext_color_BLACK, ' '))

#endif
