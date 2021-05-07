/* $Id: cut.c,v 1.61 2007/01/01 05:15:32 dolorous Exp $ */
/**************************************************************************
 *   cut.c                                                                *
 *                                                                        *
 *   Copyright (C) 1999, 2000, 2001, 2002, 2003, 2004 Chris Allegretta    *
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

#include <string.h>
#include <stdio.h>

static bool keep_cutbuffer = FALSE;
	/* Should we keep the contents of the cutbuffer? */
static filestruct *cutbottom = NULL;
	/* Pointer to the end of the cutbuffer. */

/* Indicate that we should no longer keep the contents of the
 * cutbuffer. */
void cutbuffer_reset(void)
{
    keep_cutbuffer = FALSE;
}

/* If we aren't on the last line of the file, move all the text of the
 * current line, plus the newline at the end, into the cutbuffer.  If we
 * are, move all of the text of the current line into the cutbuffer.  In
 * both cases, set the current place we want to the beginning of the
 * current line. */
void cut_line(void)
{
    if (openfile->current != openfile->filebot)
	move_to_filestruct(&cutbuffer, &cutbottom, openfile->current, 0,
		openfile->current->next, 0);
    else
	move_to_filestruct(&cutbuffer, &cutbottom, openfile->current, 0,
		openfile->current, strlen(openfile->current->data));
    openfile->placewewant = 0;
}

#ifndef NANO_TINY
/* Move all currently marked text into the cutbuffer, and set the
 * current place we want to where the text used to start. */
void cut_marked(void)
{
    filestruct *top, *bot;
    size_t top_x, bot_x;

    mark_order((const filestruct **)&top, &top_x,
	(const filestruct **)&bot, &bot_x, NULL);

    move_to_filestruct(&cutbuffer, &cutbottom, top, top_x, bot, bot_x);
    openfile->placewewant = xplustabs();
}

/* If we aren't at the end of the current line, move all the text from
 * the current cursor position to the end of the current line, not
 * counting the newline at the end, into the cutbuffer.  If we are, and
 * we're not on the last line of the file, move the newline at the end
 * into the cutbuffer, and set the current place we want to where the
 * newline used to be. */
void cut_to_eol(void)
{
    size_t data_len = strlen(openfile->current->data);

    assert(openfile->current_x <= data_len);

    if (openfile->current_x < data_len)
	/* If we're not at the end of the line, move all the text from
	 * the current position up to it, not counting the newline at
	 * the end, into the cutbuffer. */
	move_to_filestruct(&cutbuffer, &cutbottom, openfile->current,
		openfile->current_x, openfile->current, data_len);
    else if (openfile->current != openfile->filebot) {
	/* If we're at the end of the line, and it isn't the last line
	 * of the file, move all the text from the current position up
	 * to the beginning of the next line, i.e. the newline at the
	 * end, into the cutbuffer. */
	move_to_filestruct(&cutbuffer, &cutbottom, openfile->current,
		openfile->current_x, openfile->current->next, 0);
	openfile->placewewant = xplustabs();
    }
}

/* Move all the text from the current cursor position to the end of the
 * file into the cutbuffer. */
void cut_to_eof(void)
{
    move_to_filestruct(&cutbuffer, &cutbottom, openfile->current,
	openfile->current_x, openfile->filebot,
	strlen(openfile->filebot->data));
}
#endif /* !NANO_TINY */

/* Move text from the current filestruct into the cutbuffer.  If
 * copy_text is TRUE, copy the text back into the filestruct afterward.
 * If cut_till_end is TRUE, move all text from the current cursor
 * position to the end of the file into the cutbuffer. */
void do_cut_text(
#ifndef NANO_TINY
	bool copy_text, bool cut_till_end
#else
	void
#endif
	)
{
#ifndef NANO_TINY
    filestruct *cb_save = NULL;
	/* The current end of the cutbuffer, before we add text to
	 * it. */
    size_t cb_save_len = 0;
	/* The length of the string at the current end of the cutbuffer,
	 * before we add text to it.  */
    bool old_no_newlines = ISSET(NO_NEWLINES);
#endif

    assert(openfile->current != NULL && openfile->current->data != NULL);

    /* If keep_cutbuffer is FALSE and the cutbuffer isn't empty, blow
     * away the text in the cutbuffer. */
    if (!keep_cutbuffer && cutbuffer != NULL) {
	free_filestruct(cutbuffer);
	cutbuffer = NULL;
#ifdef DEBUG
	fprintf(stderr, "Blew away cutbuffer =)\n");
#endif
    }

#ifndef NANO_TINY
    if (copy_text) {
	if (cutbuffer != NULL) {
	    /* If the cutbuffer isn't empty, save where it currently
	     * ends.  This is where we'll add the new text. */
	    cb_save = cutbottom;
	    cb_save_len = strlen(cutbottom->data);
	}

	/* Set NO_NEWLINES to TRUE, so that we don't disturb the last
	 * line of the file when moving text to the cutbuffer. */
	SET(NO_NEWLINES);
    }
#endif

    /* Set keep_cutbuffer to TRUE, so that the text we're going to move
     * into the cutbuffer will be added to the text already in the
     * cutbuffer instead of replacing it. */
    keep_cutbuffer = TRUE;

#ifndef NANO_TINY
    if (cut_till_end) {
	/* If cut_till_end is TRUE, move all text up to the end of the
	 * file into the cutbuffer. */
	cut_to_eof();
    } else if (openfile->mark_set) {
	/* If the mark is on, move the marked text to the cutbuffer, and
	 * turn the mark off. */
	cut_marked();
	openfile->mark_set = FALSE;
    } else if (ISSET(CUT_TO_END))
	/* If the CUT_TO_END flag is set, move all text up to the end of
	 * the line into the cutbuffer. */
	cut_to_eol();
    else
#endif
	/* Move the entire line into the cutbuffer. */
	cut_line();

#ifndef NANO_TINY
    if (copy_text) {
	/* Copy the text in the cutbuffer, starting at its saved end if
	 * there is one, back into the filestruct.  This effectively
	 * uncuts the text we just cut without marking the file as
	 * modified. */
	if (cutbuffer != NULL) {
	    if (cb_save != NULL) {
		cb_save->data += cb_save_len;
		copy_from_filestruct(cb_save, cutbottom);
		cb_save->data -= cb_save_len;
	    } else
		copy_from_filestruct(cutbuffer, cutbottom);

	    /* Set the current place we want to where the text from the
	     * cutbuffer ends. */
	    openfile->placewewant = xplustabs();
	}

	/* Set NO_NEWLINES back to what it was before, since we're done
	 * disturbing the text. */
	if (!old_no_newlines)
	    UNSET(NO_NEWLINES);
    } else
#endif
	/* Leave the text in the cutbuffer, and mark the file as
	 * modified. */
	set_modified();

    /* Update the screen. */
    edit_refresh();

#ifdef DEBUG
    dump_filestruct(cutbuffer);
#endif
}

/* Move text from the current filestruct into the cutbuffer. */
void do_cut_text_void(void)
{
    do_cut_text(
#ifndef NANO_TINY
	FALSE, FALSE
#endif
	);
}

#ifndef NANO_TINY
/* Move text from the current filestruct into the cutbuffer, and copy it
 * back into the filestruct afterward. */
void do_copy_text(void)
{
    do_cut_text(TRUE, FALSE);
}

/* Cut from the current cursor position to the end of the file. */
void do_cut_till_end(void)
{
    do_cut_text(FALSE, TRUE);
}
#endif /* !NANO_TINY */

/* Copy text from the cutbuffer into the current filestruct. */
void do_uncut_text(void)
{
    assert(openfile->current != NULL && openfile->current->data != NULL);

    /* If the cutbuffer is empty, get out. */
    if (cutbuffer == NULL)
	return;

    /* Add a copy of the text in the cutbuffer to the current filestruct
     * at the current cursor position. */
    copy_from_filestruct(cutbuffer, cutbottom);

    /* Set the current place we want to where the text from the
     * cutbuffer ends. */
    openfile->placewewant = xplustabs();

    /* Mark the file as modified. */
    set_modified();

    /* Update the screen. */
    edit_refresh();

#ifdef DEBUG
    dump_filestruct_reverse();
#endif
}
