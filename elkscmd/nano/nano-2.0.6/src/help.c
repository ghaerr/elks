/* $Id: help.c,v 1.58.2.1 2007/04/19 03:15:04 dolorous Exp $ */
/**************************************************************************
 *   help.c                                                               *
 *                                                                        *
 *   Copyright (C) 2000, 2001, 2002, 2003, 2004 Chris Allegretta          *
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
#include <ctype.h>

#ifndef DISABLE_HELP

static char *help_text = NULL;
	/* The text displayed in the help window. */

/* Our main help browser function.  refresh_func is the function we will
 * call to refresh the edit window. */
void do_help(void (*refresh_func)(void))
{
    int kbinput = ERR;
    bool meta_key, func_key, old_no_help = ISSET(NO_HELP);
    bool abort = FALSE;
	/* Whether we should abort the help browser. */
    size_t line = 0;
	/* The line number in help_text of the first displayed help
	 * line.  This variable is zero-based. */
    size_t last_line = 0;
	/* The line number in help_text of the last help line.  This
	 * variable is zero-based. */
#ifndef DISABLE_MOUSE
    const shortcut *oldshortcut = currshortcut;
	/* The current shortcut list. */
#endif
    const char *ptr;
	/* The current line of the help text. */
    size_t old_line = (size_t)-1;
	/* The line we were on before the current line. */

    curs_set(0);
    blank_edit();
    wattroff(bottomwin, reverse_attr);
    blank_statusbar();

    /* Set help_text as the string to display. */
    help_init();

    assert(help_text != NULL);

#ifndef DISABLE_MOUSE
    /* Set currshortcut to allow clicking on the help screen's shortcut
     * list, after help_init() is called. */
    currshortcut = help_list;
#endif

    if (ISSET(NO_HELP)) {
	/* Make sure that the help screen's shortcut list will actually
	 * be displayed. */
	UNSET(NO_HELP);
	window_init();
    }

    bottombars(help_list);
    wnoutrefresh(bottomwin);

    /* Get the last line of the help text. */
    ptr = help_text;

    for (; *ptr != '\0'; last_line++) {
	ptr += help_line_len(ptr);
	if (*ptr == '\n')
	    ptr++;
    }
    if (last_line > 0)
	last_line--;

    while (!abort) {
	size_t i;

	/* Display the help text if we don't have a key, or if the help
	 * text has moved. */
	if (kbinput == ERR || line != old_line) {
	    blank_edit();

	    ptr = help_text;

	    /* Calculate where in the text we should be, based on the
	     * page. */
	    for (i = 0; i < line; i++) {
		ptr += help_line_len(ptr);
		if (*ptr == '\n')
		    ptr++;
	    }

	    for (i = 0; i < editwinrows && *ptr != '\0'; i++) {
		size_t j = help_line_len(ptr);

		mvwaddnstr(edit, i, 0, ptr, j);
		ptr += j;
		if (*ptr == '\n')
		    ptr++;
	    }
	}

	wnoutrefresh(edit);

	old_line = line;

	kbinput = get_kbinput(edit, &meta_key, &func_key);
	parse_help_input(&kbinput, &meta_key, &func_key);

	switch (kbinput) {
#ifndef DISABLE_MOUSE
	    case KEY_MOUSE:
		{
		    int mouse_x, mouse_y;

		    get_mouseinput(&mouse_x, &mouse_y, TRUE);
		}
		break;
#endif
	    /* Redraw the screen. */
	    case NANO_REFRESH_KEY:
		total_redraw();
		break;
	    case NANO_PREVPAGE_KEY:
		if (line > editwinrows - 2)
		    line -= editwinrows - 2;
		else
		    line = 0;
		break;
	    case NANO_NEXTPAGE_KEY:
		if (line + (editwinrows - 1) < last_line)
		    line += editwinrows - 2;
		break;
	    case NANO_PREVLINE_KEY:
		if (line > 0)
		    line--;
		break;
	    case NANO_NEXTLINE_KEY:
		if (line + (editwinrows - 1) < last_line)
		    line++;
		break;
	    case NANO_FIRSTLINE_METAKEY:
		if (meta_key)
		    line = 0;
		break;
	    case NANO_LASTLINE_METAKEY:
		if (meta_key) {
		    if (line + (editwinrows - 1) < last_line)
			line = last_line - (editwinrows - 1);
		}
		break;
	    /* Abort the help browser. */
	    case NANO_EXIT_KEY:
		abort = TRUE;
		break;
	}
    }

#ifndef DISABLE_MOUSE
    currshortcut = oldshortcut;
#endif

    if (old_no_help) {
	blank_bottombars();
	wnoutrefresh(bottomwin);
	SET(NO_HELP);
	window_init();
    } else
	bottombars(currshortcut);

    curs_set(1);
    refresh_func();

    /* The help_init() at the beginning allocated help_text.  Since
     * help_text has now been written to the screen, we don't need it
     * anymore. */
    free(help_text);
    help_text = NULL;
}

/* Start the help browser for the edit window. */
void do_help_void(void)
{
    do_help(&edit_refresh);
}

#ifndef DISABLE_BROWSER
/* Start the help browser for the file browser. */
void do_browser_help(void)
{
    do_help(&browser_refresh);
}
#endif

/* This function allocates help_text, and stores the help string in it.
 * help_text should be NULL initially. */
void help_init(void)
{
    size_t allocsize = 0;	/* Space needed for help_text. */
    const char *htx[3];		/* Untranslated help message.  We break
				 * it up into three chunks in case the
				 * full string is too long for the
				 * compiler to handle. */
    char *ptr;
    const shortcut *s;
#ifndef NANO_TINY
    const toggle *t;
#ifdef ENABLE_NANORC
    bool old_whitespace = ISSET(WHITESPACE_DISPLAY);

    UNSET(WHITESPACE_DISPLAY);
#endif
#endif

    /* First, set up the initial help text for the current function. */
    if (currshortcut == whereis_list || currshortcut == replace_list ||
	currshortcut == replace_list_2) {
	htx[0] = N_("Search Command Help Text\n\n "
		"Enter the words or characters you would like to "
		"search for, and then press Enter.  If there is a "
		"match for the text you entered, the screen will be "
		"updated to the location of the nearest match for the "
		"search string.\n\n The previous search string will be "
		"shown in brackets after the search prompt.  Hitting "
		"Enter without entering any text will perform the "
		"previous search.  ");
	htx[1] = N_("If you have selected text with the mark and then "
		"search to replace, only matches in the selected text "
		"will be replaced.\n\n The following function keys are "
		"available in Search mode:\n\n");
	htx[2] = NULL;
    } else if (currshortcut == gotoline_list) {
	htx[0] = N_("Go To Line Help Text\n\n "
		"Enter the line number that you wish to go to and hit "
		"Enter.  If there are fewer lines of text than the "
		"number you entered, you will be brought to the last "
		"line of the file.\n\n The following function keys are "
		"available in Go To Line mode:\n\n");
	htx[1] = NULL;
	htx[2] = NULL;
    } else if (currshortcut == insertfile_list) {
	htx[0] = N_("Insert File Help Text\n\n "
		"Type in the name of a file to be inserted into the "
		"current file buffer at the current cursor "
		"location.\n\n If you have compiled nano with multiple "
		"file buffer support, and enable multiple file buffers "
		"with the -F or --multibuffer command line flags, the "
		"Meta-F toggle, or a nanorc file, inserting a file "
		"will cause it to be loaded into a separate buffer "
		"(use Meta-< and > to switch between file buffers).  ");
	htx[1] = N_("If you need another blank buffer, do not enter "
		"any filename, or type in a nonexistent filename at "
		"the prompt and press Enter.\n\n The following "
		"function keys are available in Insert File mode:\n\n");
	htx[2] = NULL;
    } else if (currshortcut == writefile_list) {
	htx[0] = N_("Write File Help Text\n\n "
		"Type the name that you wish to save the current file "
		"as and press Enter to save the file.\n\n If you have "
		"selected text with the mark, you will be prompted to "
		"save only the selected portion to a separate file.  To "
		"reduce the chance of overwriting the current file with "
		"just a portion of it, the current filename is not the "
		"default in this mode.\n\n The following function keys "
		"are available in Write File mode:\n\n");
	htx[1] = NULL;
	htx[2] = NULL;
    }
#ifndef DISABLE_BROWSER
    else if (currshortcut == browser_list) {
	htx[0] = N_("File Browser Help Text\n\n "
		"The file browser is used to visually browse the "
		"directory structure to select a file for reading "
		"or writing.  You may use the arrow keys or Page Up/"
		"Down to browse through the files, and S or Enter to "
		"choose the selected file or enter the selected "
		"directory.  To move up one level, select the "
		"directory called \"..\" at the top of the file "
		"list.\n\n The following function keys are available "
		"in the file browser:\n\n");
	htx[1] = NULL;
	htx[2] = NULL;
    } else if (currshortcut == whereis_file_list) {
	htx[0] = N_("Browser Search Command Help Text\n\n "
		"Enter the words or characters you would like to "
		"search for, and then press Enter.  If there is a "
		"match for the text you entered, the screen will be "
		"updated to the location of the nearest match for the "
		"search string.\n\n The previous search string will be "
		"shown in brackets after the search prompt.  Hitting "
		"Enter without entering any text will perform the "
		"previous search.\n\n");
	htx[1] = N_(" The following function keys are available in "
		"Browser Search mode:\n\n");
	htx[2] = NULL;
    } else if (currshortcut == gotodir_list) {
	htx[0] = N_("Browser Go To Directory Help Text\n\n "
		"Enter the name of the directory you would like to "
		"browse to.\n\n If tab completion has not been "
		"disabled, you can use the Tab key to (attempt to) "
		"automatically complete the directory name.\n\n The "
		"following function keys are available in Browser Go "
		"To Directory mode:\n\n");
	htx[1] = NULL;
	htx[2] = NULL;
    }
#endif /* !DISABLE_BROWSER */
#ifndef DISABLE_SPELLER
    else if (currshortcut == spell_list) {
	htx[0] = N_("Spell Check Help Text\n\n "
		"The spell checker checks the spelling of all text in "
		"the current file.  When an unknown word is "
		"encountered, it is highlighted and a replacement can "
		"be edited.  It will then prompt to replace every "
		"instance of the given misspelled word in the current "
		"file, or, if you have selected text with the mark, in "
		"the selected text.\n\n The following function keys "
		"are available in Spell Check mode:\n\n");
	htx[1] = NULL;
	htx[2] = NULL;
    }
#endif /* !DISABLE_SPELLER */
#ifndef NANO_TINY
    else if (currshortcut == extcmd_list) {
	htx[0] = N_("Execute Command Help Text\n\n "
		"This mode allows you to insert the output of a "
		"command run by the shell into the current buffer (or "
		"a new buffer in multiple file buffer mode). If you "
		"need another blank buffer, do not enter any "
		"command.\n\n The following function keys are "
		"available in Execute Command mode:\n\n");
	htx[1] = NULL;
	htx[2] = NULL;
    }
#endif /* !NANO_TINY */
    else {
	/* Default to the main help list. */
	htx[0] = N_("Main nano help text\n\n "
		"The nano editor is designed to emulate the "
		"functionality and ease-of-use of the UW Pico text "
		"editor.  There are four main sections of the editor.  "
		"The top line shows the program version, the current "
		"filename being edited, and whether or not the file "
		"has been modified.  Next is the main editor window "
		"showing the file being edited.  The status line is "
		"the third line from the bottom and shows important "
		"messages.  ");
	htx[1] = N_("The bottom two lines show the most commonly used "
		"shortcuts in the editor.\n\n The notation for "
		"shortcuts is as follows: Control-key sequences are "
		"notated with a caret (^) symbol and can be entered "
		"either by using the Control (Ctrl) key or pressing "
		"the Escape (Esc) key twice.  Escape-key sequences are "
		"notated with the Meta (M-) symbol and can be entered "
		"using either the Esc, Alt, or Meta key depending on "
		"your keyboard setup.  ");
	htx[2] = N_("Also, pressing Esc twice and then typing a "
		"three-digit decimal number from 000 to 255 will enter "
		"the character with the corresponding value.  The "
		"following keystrokes are available in the main editor "
		"window.  Alternative keys are shown in "
		"parentheses:\n\n");
    }

    htx[0] = _(htx[0]);
    if (htx[1] != NULL)
	htx[1] = _(htx[1]);
    if (htx[2] != NULL)
	htx[2] = _(htx[2]);

    allocsize += strlen(htx[0]);
    if (htx[1] != NULL)
	allocsize += strlen(htx[1]);
    if (htx[2] != NULL)
	allocsize += strlen(htx[2]);

    /* Count the shortcut help text.  Each entry has up to three keys,
     * which fill 24 columns, plus translated text, plus one or two
     * \n's. */
	for (s = currshortcut; s != NULL; s = s->next)
	    allocsize += (24 * mb_cur_max()) + strlen(s->help) + 2;

#ifndef NANO_TINY
    /* If we're on the main list, we also count the toggle help text.
     * Each entry has "M-%c\t\t\t", which fills 24 columns, plus a
     * space, plus translated text, plus one or two '\n's. */
    if (currshortcut == main_list) {
	size_t endis_len = strlen(_("enable/disable"));

	for (t = toggles; t != NULL; t = t->next)
	    allocsize += strlen(t->desc) + endis_len + 9;
    }
#endif

    /* help_text has been freed and set to NULL unless the user resized
     * while in the help screen. */
    if (help_text != NULL)
	free(help_text);

    /* Allocate space for the help text. */
    help_text = charalloc(allocsize + 1);

    /* Now add the text we want. */
    strcpy(help_text, htx[0]);
    if (htx[1] != NULL)
	strcat(help_text, htx[1]);
    if (htx[2] != NULL)
	strcat(help_text, htx[2]);

    ptr = help_text + strlen(help_text);

    /* Now add our shortcut info.  Assume that each shortcut has, at the
     * very least, an equivalent control key, an equivalent primary meta
     * key sequence, or both.  Also assume that the meta key values are
     * not control characters.  We can display a maximum of three
     * shortcut entries. */
    for (s = currshortcut; s != NULL; s = s->next) {
	int entries = 0;

	/* Control key. */
	if (s->ctrlval != NANO_NO_KEY) {
	    entries++;
	    /* Yucky sentinel values that we can't handle a better
	     * way. */
	    if (s->ctrlval == NANO_CONTROL_SPACE) {
		char *space_ptr = display_string(_("Space"), 0, 14,
			FALSE);

		if (s->funcval == NANO_NO_KEY && (s->metaval ==
			NANO_NO_KEY || s->miscval == NANO_NO_KEY)) {
		    /* If we're here, we have at least two entries worth
		     * of blank space.  If this entry takes up more than
		     * one entry's worth of space, use two to display
		     * it. */
		    if (mbstrlen(space_ptr) > 6)
			entries++;
		} else
		    /* Otherwise, truncate it so that it takes up only
		     * one entry's worth of space. */
		    space_ptr[6] = '\0';

		ptr += sprintf(ptr, "^%s", space_ptr);

		free(space_ptr);
	    } else if (s->ctrlval == NANO_CONTROL_8)
		ptr += sprintf(ptr, "^?");
	    /* Normal values. */
	    else
		ptr += sprintf(ptr, "^%c", s->ctrlval + 64);
	    *(ptr++) = '\t';
	}

	/* Function key. */
	if (s->funcval != NANO_NO_KEY) {
	    entries++;
	    /* If this is the first entry, put it in the middle. */
	    if (entries == 1) {
		entries++;
		*(ptr++) = '\t';
	    }
	    ptr += sprintf(ptr, "(F%d)", s->funcval - KEY_F0);
	    *(ptr++) = '\t';
	}

	/* Primary meta key sequence.  If it's the first entry, don't
	 * put parentheses around it. */
	if (s->metaval != NANO_NO_KEY) {
	    entries++;
	    /* If this is the last entry, put it at the end. */
	    if (entries == 2 && s->miscval == NANO_NO_KEY) {
		entries++;
		*(ptr++) = '\t';
	    }
	    /* Yucky sentinel values that we can't handle a better
	     * way. */
	    if (s->metaval == NANO_META_SPACE && entries == 1) {
		char *space_ptr = display_string(_("Space"), 0, 13,
			FALSE);

		/* If we're here, we have at least two entries worth of
		 * blank space.  If this entry takes up more than one
		 * entry's worth of space, use two to display it. */
		if (mbstrlen(space_ptr) > 5)
		    entries++;

		ptr += sprintf(ptr, "M-%s", space_ptr);

		free(space_ptr);
	    } else
		/* Normal values. */
		ptr += sprintf(ptr, (entries == 1) ? "M-%c" : "(M-%c)",
			toupper(s->metaval));
	    *(ptr++) = '\t';
	}

	/* Miscellaneous meta key sequence. */
	if (entries < 3 && s->miscval != NANO_NO_KEY) {
	    entries++;
	    /* If this is the last entry, put it at the end. */
	    if (entries == 2) {
		entries++;
		*(ptr++) = '\t';
	    }
	    ptr += sprintf(ptr, "(M-%c)", toupper(s->miscval));
	    *(ptr++) = '\t';
	}

	/* If this entry isn't blank, make sure all the help text starts
	 * at the same place. */
	if (s->ctrlval != NANO_NO_KEY || s->funcval != NANO_NO_KEY ||
		s->metaval != NANO_NO_KEY || s->miscval !=
		NANO_NO_KEY) {
	    while (entries < 3) {
		entries++;
		*(ptr++) = '\t';
	    }
	}

	/* The shortcut's help text. */
	ptr += sprintf(ptr, "%s\n", s->help);

	if (s->blank_after)
	    ptr += sprintf(ptr, "\n");
    }

#ifndef NANO_TINY
    /* And the toggles... */
    if (currshortcut == main_list) {
	for (t = toggles; t != NULL; t = t->next) {
	    ptr += sprintf(ptr, "M-%c\t\t\t%s %s\n",
		toupper(t->val), t->desc, _("enable/disable"));

	    if (t->blank_after)
		ptr += sprintf(ptr, "\n");
	}
    }

#ifdef ENABLE_NANORC
    if (old_whitespace)
	SET(WHITESPACE_DISPLAY);
#endif
#endif

    /* If all went well, we didn't overwrite the allocated space for
     * help_text. */
    assert(strlen(help_text) <= allocsize + 1);
}

/* Determine the shortcut key corresponding to the values of kbinput
 * (the key itself), meta_key (whether the key is a meta sequence), and
 * func_key (whether the key is a function key), if any.  In the
 * process, convert certain non-shortcut keys into their corresponding
 * shortcut keys. */
void parse_help_input(int *kbinput, bool *meta_key, bool *func_key)
{
    get_shortcut(help_list, kbinput, meta_key, func_key);

    if (!*meta_key) {
	switch (*kbinput) {
	    /* For consistency with the file browser. */
	    case ' ':
		*kbinput = NANO_NEXTPAGE_KEY;
		break;
	    case '-':
		*kbinput = NANO_PREVPAGE_KEY;
		break;
	    /* Cancel is equivalent to Exit here. */
	    case NANO_CANCEL_KEY:
	    case 'E':
	    case 'e':
		*kbinput = NANO_EXIT_KEY;
		break;
	}
    }
}

/* Calculate the next line of help_text, starting at ptr. */
size_t help_line_len(const char *ptr)
{
    int help_cols = (COLS > 24) ? COLS - 1 : 24;

    /* Try to break the line at (COLS - 1) columns if we have more than
     * 24 columns, and at 24 columns otherwise. */
    ssize_t wrap_loc = break_line(ptr, help_cols, TRUE);
    size_t retval = (wrap_loc < 0) ? 0 : wrap_loc;
    size_t retval_save = retval;

    /* Get the length of the entire line up to a null or a newline. */
    while (*(ptr + retval) != '\0' && *(ptr + retval) != '\n')
	retval += move_mbright(ptr + retval, 0);

    /* If the entire line doesn't go more than one column beyond where
     * we tried to break it, we should display it as-is.  Otherwise, we
     * should display it only up to the break. */
    if (strnlenpt(ptr, retval) > help_cols + 1)
	retval = retval_save;

    return retval;
}

#endif /* !DISABLE_HELP */
