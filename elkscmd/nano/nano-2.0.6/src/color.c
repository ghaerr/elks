/* $Id: color.c,v 1.42 2007/01/01 05:15:32 dolorous Exp $ */
/**************************************************************************
 *   color.c                                                              *
 *                                                                        *
 *   Copyright (C) 2001, 2002, 2003, 2004 Chris Allegretta                *
 *   Copyright (C) 2005, 2006, 2007 David Lawrence Ramsey                 *
 *   This program is free software; you can redistribute it and/or modify *
 *   it under the terms of the GNU General Public License as published by *
 *   the Free Software Foundation; either version 2, or (at your option)  *
 *   any later version.                                                   *
 *                                                                        *
 *   This program is distributed in the hope that it will be useful, but  *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU    *
 *   General Public License for more details.                             *
 *                                                                        *
 *   You should have received a copy of the GNU General Public License    *
 *   along with this program; if not, write to the Free Software          *
 *   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA            *
 *   02110-1301, USA.                                                     *
 *                                                                        *
 **************************************************************************/

#include "proto.h"

#include <stdio.h>
#include <string.h>

#ifdef ENABLE_COLOR

/* For each syntax list entry, go through the list of colors and assign
 * the color pairs. */
void set_colorpairs(void)
{
    const syntaxtype *this_syntax = syntaxes;

    for (; this_syntax != NULL; this_syntax = this_syntax->next) {
	colortype *this_color = this_syntax->color;
	int color_pair = 1;

	for (; this_color != NULL; this_color = this_color->next) {
	    const colortype *beforenow = this_syntax->color;

	    for (; beforenow != this_color &&
		(beforenow->fg != this_color->fg ||
		beforenow->bg != this_color->bg ||
		beforenow->bright != this_color->bright);
		beforenow = beforenow->next)
		;

	    if (beforenow != this_color)
		this_color->pairnum = beforenow->pairnum;
	    else {
		this_color->pairnum = color_pair;
		color_pair++;
	    }
	}
    }
}

/* Initialize the color information. */
void color_init(void)
{
    assert(openfile != NULL);

    if (has_colors()) {
	const colortype *tmpcolor;
#ifdef HAVE_USE_DEFAULT_COLORS
	bool defok;
#endif

	start_color();

#ifdef HAVE_USE_DEFAULT_COLORS
	/* Use the default colors, if available. */
	defok = (use_default_colors() != ERR);
#endif

	for (tmpcolor = openfile->colorstrings; tmpcolor != NULL;
		tmpcolor = tmpcolor->next) {
	    short foreground = tmpcolor->fg, background = tmpcolor->bg;
	    if (foreground == -1) {
#ifdef HAVE_USE_DEFAULT_COLORS
		if (!defok)
#endif
		    foreground = COLOR_WHITE;
	    }

	    if (background == -1) {
#ifdef HAVE_USE_DEFAULT_COLORS
		if (!defok)
#endif
		    background = COLOR_BLACK;
	    }

	    init_pair(tmpcolor->pairnum, foreground, background);

#ifdef DEBUG
	    fprintf(stderr, "init_pair(): fg = %hd, bg = %hd\n", tmpcolor->fg, tmpcolor->bg);
#endif
	}
    }
}

/* Update the color information based on the current filename. */
void color_update(void)
{
    const syntaxtype *tmpsyntax;
    colortype *tmpcolor, *defcolor = NULL;

    assert(openfile != NULL);

    openfile->colorstrings = NULL;

    /* If we specified a syntax override string, use it. */
    if (syntaxstr != NULL) {
	/* If the syntax override is "none", it's the same as not having
	 * a syntax at all, so get out. */
	if (strcmp(syntaxstr, "none") == 0)
	    return;

	for (tmpsyntax = syntaxes; tmpsyntax != NULL;
		tmpsyntax = tmpsyntax->next) {
	    if (strcmp(tmpsyntax->desc, syntaxstr) == 0)
		openfile->colorstrings = tmpsyntax->color;

	    if (openfile->colorstrings != NULL)
		break;
	}
    }

    /* If we didn't specify a syntax override string, or if we did and
     * there was no syntax by that name, get the syntax based on the
     * file extension. */
    if (openfile->colorstrings == NULL) {
	for (tmpsyntax = syntaxes; tmpsyntax != NULL;
		tmpsyntax = tmpsyntax->next) {
	    exttype *e;

	    /* If this is the default syntax, it has no associated
	     * extensions, which we've checked for elsewhere.  Skip over
	     * it here, but keep track of its color regexes. */
	    if (strcmp(tmpsyntax->desc, "default") == 0) {
		defcolor = tmpsyntax->color;
		continue;
	    }

	    for (e = tmpsyntax->extensions; e != NULL; e = e->next) {
		bool not_compiled = (e->ext == NULL);

		/* e->ext_regex has already been checked for validity
		 * elsewhere.  Compile its specified regex if we haven't
		 * already. */
		if (not_compiled) {
		    e->ext = (regex_t *)nmalloc(sizeof(regex_t));
		    regcomp(e->ext, e->ext_regex, REG_EXTENDED);
		}

		/* Set colorstrings if we matched the extension
		 * regex. */
		if (regexec(e->ext, openfile->filename, 0, NULL,
			0) == 0)
		    openfile->colorstrings = tmpsyntax->color;

		if (openfile->colorstrings != NULL)
		    break;

		/* Decompile e->ext_regex's specified regex if we aren't
		 * going to use it. */
		if (not_compiled) {
		    regfree(e->ext);
		    free(e->ext);
		    e->ext = NULL;
		}
	    }
	}
    }

    /* If we didn't get a syntax based on the file extension, and we
     * have a default syntax, use it. */
    if (openfile->colorstrings == NULL && defcolor != NULL)
	openfile->colorstrings = defcolor;

    for (tmpcolor = openfile->colorstrings; tmpcolor != NULL;
	tmpcolor = tmpcolor->next) {
	/* tmpcolor->start_regex and tmpcolor->end_regex have already
	 * been checked for validity elsewhere.  Compile their specified
	 * regexes if we haven't already. */
	if (tmpcolor->start == NULL) {
	    tmpcolor->start = (regex_t *)nmalloc(sizeof(regex_t));
	    regcomp(tmpcolor->start, tmpcolor->start_regex,
		REG_EXTENDED | (tmpcolor->icase ? REG_ICASE : 0));
	}

	if (tmpcolor->end_regex != NULL && tmpcolor->end == NULL) {
	    tmpcolor->end = (regex_t *)nmalloc(sizeof(regex_t));
	    regcomp(tmpcolor->end, tmpcolor->end_regex,
		REG_EXTENDED | (tmpcolor->icase ? REG_ICASE : 0));
	}
    }
}

#endif /* ENABLE_COLOR */
