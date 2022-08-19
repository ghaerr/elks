/* Ported to ELKS from mwtypes.h */

typedef unsigned int MWIMAGEBITS;			/* bitmap image unit size*/

/* builtin C-based proportional/fixed font structure*/
typedef struct {
	char			*name;			/* font name*/
	int			maxwidth;		/* max width in pixels*/
	unsigned int		height;			/* height in pixels*/
	int			ascent;			/* ascent (baseline) height*/
	int			firstchar;		/* first character in bitmap*/
	int			size;			/* font size in characters*/
	const MWIMAGEBITS 	*bits;			/* 16-bit right-padded bitmap data*/
	const unsigned int	*offset;		/* offsets into bitmap data*/
	const unsigned char	*width;			/* character widths or 0 if fixed*/
	int			defaultchar;		/* default char (not glyph index)*/
	int			bits_size;		/* # words of MWIMAGEBITS bits*/
} MWCFONT, *PMWCFONT;
