/* $Id: files.c,v 1.348.2.18 2007/04/21 20:33:56 dolorous Exp $ */
/**************************************************************************
 *   files.c                                                              *
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

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <utime.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>
#include <pwd.h>

/* Add an entry to the openfile openfilestruct.  This should only be
 * called from open_buffer(). */
void make_new_buffer(void)
{
    /* If there are no entries in openfile, make the first one and
     * move to it. */
    if (openfile == NULL) {
	openfile = make_new_opennode();
	splice_opennode(openfile, openfile, openfile);
    /* Otherwise, make a new entry for openfile, splice it in after
     * the current entry, and move to it. */
    } else {
	splice_opennode(openfile, make_new_opennode(), openfile->next);
	openfile = openfile->next;
    }

    /* Initialize the new buffer. */
    initialize_buffer();
}

/* Initialize the current entry of the openfile openfilestruct. */
void initialize_buffer(void)
{
    assert(openfile != NULL);

    openfile->filename = mallocstrcpy(NULL, "");

    initialize_buffer_text();

    openfile->current_x = 0;
    openfile->placewewant = 0;
    openfile->current_y = 0;

    openfile->modified = FALSE;
#ifndef NANO_TINY
    openfile->mark_set = FALSE;

    openfile->mark_begin = NULL;
    openfile->mark_begin_x = 0;

    openfile->fmt = NIX_FILE;

    openfile->current_stat = NULL;
#endif
#ifdef ENABLE_COLOR
    openfile->colorstrings = NULL;
#endif
}

/* Initialize the text of the current entry of the openfile
 * openfilestruct. */
void initialize_buffer_text(void)
{
    assert(openfile != NULL);

    openfile->fileage = make_new_node(NULL);
    openfile->fileage->data = mallocstrcpy(NULL, "");

    openfile->filebot = openfile->fileage;
    openfile->edittop = openfile->fileage;
    openfile->current = openfile->fileage;

    openfile->totsize = 0;
}

/* If it's not "", filename is a file to open.  We make a new buffer, if
 * necessary, and then open and read the file, if applicable. */
void open_buffer(const char *filename)
{
    bool new_buffer = (openfile == NULL
#ifdef ENABLE_MULTIBUFFER
	 || ISSET(MULTIBUFFER)
#endif
	);
	/* Whether we load into this buffer or a new one. */
    FILE *f;
    int rc;
	/* rc == -2 means that we have a new file.  -1 means that the
	 * open() failed.  0 means that the open() succeeded. */

    assert(filename != NULL);

#ifndef DISABLE_OPERATINGDIR
    if (check_operating_dir(filename, FALSE)) {
	statusbar(_("Can't insert file from outside of %s"),
		operating_dir);
	return;
    }
#endif

    /* If the filename isn't blank, open the file.  Otherwise, treat it
     * as a new file. */
    rc = (filename[0] != '\0') ? open_file(filename, new_buffer, &f) :
	-2;

    /* If we're loading into a new buffer, add a new entry to
     * openfile. */
    if (new_buffer)
	make_new_buffer();

    /* If we have a file, and we're loading into a new buffer, update
     * the filename. */
    if (rc != -1 && new_buffer)
	openfile->filename = mallocstrcpy(openfile->filename, filename);

    /* If we have a non-new file, read it in.  Then, if the buffer has
     * no stat, update the stat, if applicable. */
    if (rc == 0) {
	read_file(f, filename);
#ifndef NANO_TINY
	if (openfile->current_stat == NULL) {
	    openfile->current_stat =
		(struct stat *)nmalloc(sizeof(struct stat));
	    stat(filename, openfile->current_stat);
	}
#endif
    }

    /* If we have a file, and we're loading into a new buffer, move back
     * to the beginning of the first line of the buffer. */
    if (rc != -1 && new_buffer) {
	openfile->current = openfile->fileage;
	openfile->current_x = 0;
	openfile->placewewant = 0;
    }

#ifdef ENABLE_COLOR
    /* If we're loading into a new buffer, update the colors to account
     * for it, if applicable. */
    if (new_buffer)
	color_update();
#endif
}

#ifndef DISABLE_SPELLER
/* If it's not "", filename is a file to open.  We blow away the text of
 * the current buffer, and then open and read the file, if
 * applicable.  Note that we skip the operating directory test when
 * doing this. */
void replace_buffer(const char *filename)
{
    FILE *f;
    int rc;
	/* rc == -2 means that we have a new file.  -1 means that the
	 * open() failed.  0 means that the open() succeeded. */

    assert(filename != NULL);

    /* If the filename isn't blank, open the file.  Otherwise, treat it
     * as a new file. */
    rc = (filename[0] != '\0') ? open_file(filename, TRUE, &f) : -2;

    /* Reinitialize the text of the current buffer. */
    free_filestruct(openfile->fileage);
    initialize_buffer_text();

    /* If we have a non-new file, read it in. */
    if (rc == 0)
	read_file(f, filename);

    /* Move back to the beginning of the first line of the buffer. */
    openfile->current = openfile->fileage;
    openfile->current_x = 0;
    openfile->placewewant = 0;
}
#endif /* !DISABLE_SPELLER */

/* Update the screen to account for the current buffer. */
void display_buffer(void)
{
    /* Update the titlebar, since the filename may have changed. */
    titlebar(NULL);

#ifdef ENABLE_COLOR
    /* Make sure we're using the buffer's associated colors, if
     * applicable. */
    color_init();
#endif

    /* Update the edit window. */
    edit_refresh();
}

#ifdef ENABLE_MULTIBUFFER
/* Switch to the next file buffer if next_buf is TRUE.  Otherwise,
 * switch to the previous file buffer. */
void switch_to_prevnext_buffer(bool next_buf)
{
    assert(openfile != NULL);

    /* If only one file buffer is open, indicate it on the statusbar and
     * get out. */
    if (openfile == openfile->next) {
	statusbar(_("No more open file buffers"));
	return;
    }

    /* Switch to the next or previous file buffer, depending on the
     * value of next_buf. */
    openfile = next_buf ? openfile->next : openfile->prev;

#ifdef DEBUG
    fprintf(stderr, "filename is %s\n", openfile->filename);
#endif

    /* Update the screen to account for the current buffer. */
    display_buffer();

    /* Indicate the switch on the statusbar. */
    statusbar(_("Switched to %s"),
	((openfile->filename[0] == '\0') ? _("New Buffer") :
	openfile->filename));

#ifdef DEBUG
    dump_filestruct(openfile->current);
#endif
}

/* Switch to the previous entry in the openfile filebuffer. */
void switch_to_prev_buffer_void(void)
{
    switch_to_prevnext_buffer(FALSE);
}

/* Switch to the next entry in the openfile filebuffer. */
void switch_to_next_buffer_void(void)
{
    switch_to_prevnext_buffer(TRUE);
}

/* Delete an entry from the openfile filebuffer, and switch to the one
 * after it.  Return TRUE on success, or FALSE if there are no more open
 * file buffers. */
bool close_buffer(void)
{
    assert(openfile != NULL);

    /* If only one file buffer is open, get out. */
    if (openfile == openfile->next)
	return FALSE;

    /* Switch to the next file buffer. */
    switch_to_next_buffer_void();

    /* Close the file buffer we had open before. */
    unlink_opennode(openfile->prev);

    display_main_list();

    return TRUE;
}
#endif /* ENABLE_MULTIBUFFER */

/* We make a new line of text from buf.  buf is length buf_len.  If
 * first_line_ins is TRUE, then we put the new line at the top of the
 * file.  Otherwise, we assume prevnode is the last line of the file,
 * and put our line after prevnode. */
filestruct *read_line(char *buf, filestruct *prevnode, bool
	*first_line_ins, size_t buf_len)
{
    filestruct *fileptr = (filestruct *)nmalloc(sizeof(filestruct));

    /* Convert nulls to newlines.  buf_len is the string's real
     * length. */
    unsunder(buf, buf_len);

    assert(openfile->fileage != NULL && strlen(buf) == buf_len);

    fileptr->data = mallocstrcpy(NULL, buf);

#ifndef NANO_TINY
    /* If it's a DOS file ("\r\n"), and file conversion isn't disabled,
     * strip the '\r' part from fileptr->data. */
    if (!ISSET(NO_CONVERT) && buf_len > 0 && buf[buf_len - 1] == '\r')
	fileptr->data[buf_len - 1] = '\0';
#endif

    if (*first_line_ins) {
	/* Special case: We're inserting with the cursor on the first
	 * line. */
	fileptr->prev = NULL;
	fileptr->next = openfile->fileage;
	fileptr->lineno = 1;
	if (*first_line_ins) {
	    *first_line_ins = FALSE;
	    /* If we're inserting into the first line of the file, then
	     * we want to make sure that our edit buffer stays on the
	     * first line and that fileage stays up to date. */
	    openfile->edittop = fileptr;
	} else
	    openfile->filebot = fileptr;
	openfile->fileage = fileptr;
    } else {
	assert(prevnode != NULL);

	fileptr->prev = prevnode;
	fileptr->next = NULL;
	fileptr->lineno = prevnode->lineno + 1;
	prevnode->next = fileptr;
    }

    return fileptr;
}

/* Read an open file into the current buffer.  f should be set to the
 * open file, and filename should be set to the name of the file. */
void read_file(FILE *f, const char *filename)
{
    size_t num_lines = 0;
	/* The number of lines in the file. */
    size_t len = 0;
	/* The length of the current line of the file. */
    size_t i = 0;
	/* The position in the current line of the file. */
    size_t bufx = MAX_BUF_SIZE;
	/* The size of each chunk of the file that we read. */
    char input = '\0';
	/* The current input character. */
    char *buf;
	/* The buffer where we store chunks of the file. */
    filestruct *fileptr = openfile->current;
	/* The current line of the file. */
    bool first_line_ins = FALSE;
	/* Whether we're inserting with the cursor on the first line. */
    int input_int;
	/* The current value we read from the file, whether an input
	 * character or EOF. */
#ifndef NANO_TINY
    int format = 0;
	/* 0 = *nix, 1 = DOS, 2 = Mac, 3 = both DOS and Mac. */
#endif

    assert(openfile->fileage != NULL && openfile->current != NULL);

    buf = charalloc(bufx);
    buf[0] = '\0';

    if (openfile->current == openfile->fileage)
	first_line_ins = TRUE;
    else
	fileptr = openfile->current->prev;

    /* Read the entire file into the filestruct. */
    while ((input_int = getc(f)) != EOF) {
	input = (char)input_int;

	/* If it's a *nix file ("\n") or a DOS file ("\r\n"), and file
	 * conversion isn't disabled, handle it! */
	if (input == '\n') {
#ifndef NANO_TINY
	    /* If there's a '\r' before the '\n', set format to DOS if
	     * we currently think this is a *nix file, or to both if we
	     * currently think it's a Mac file. */
	    if (!ISSET(NO_CONVERT) && i > 0 && buf[i - 1] == '\r' &&
		(format == 0 || format == 2))
		format++;
#endif

	    /* Read in the line properly. */
	    fileptr = read_line(buf, fileptr, &first_line_ins, len);

	    /* Reset the line length in preparation for the next
	     * line. */
	    len = 0;

	    num_lines++;
	    buf[0] = '\0';
	    i = 0;
#ifndef NANO_TINY
	/* If it's a Mac file ('\r' without '\n'), and file conversion
	 * isn't disabled, handle it! */
	} else if (!ISSET(NO_CONVERT) && i > 0 && buf[i - 1] == '\r') {
	    /* If we currently think the file is a *nix file, set format
	     * to Mac.  If we currently think the file is a DOS file,
	     * set format to both DOS and Mac. */
	    if (format == 0 || format == 1)
		format += 2;

	    /* Read in the line properly. */
	    fileptr = read_line(buf, fileptr, &first_line_ins, len);

	    /* Reset the line length in preparation for the next line.
	     * Since we've already read in the next character, reset it
	     * to 1 instead of 0. */
	    len = 1;

	    num_lines++;
	    buf[0] = input;
	    buf[1] = '\0';
	    i = 1;
#endif
	} else {
	    /* Calculate the total length of the line.  It might have
	     * nulls in it, so we can't just use strlen() here. */
	    len++;

	    /* Now we allocate a bigger buffer MAX_BUF_SIZE characters
	     * at a time.  If we allocate a lot of space for one line,
	     * we may indeed have to use a buffer this big later on, so
	     * we don't decrease it at all.  We do free it at the end,
	     * though. */
	    if (i >= bufx - 1) {
		bufx += MAX_BUF_SIZE;
		buf = charealloc(buf, bufx);
	    }

	    buf[i] = input;
	    buf[i + 1] = '\0';
	    i++;
	}
    }

    /* Perhaps this could use some better handling. */
    if (ferror(f))
	nperror(filename);
    fclose(f);

#ifndef NANO_TINY
    /* If file conversion isn't disabled and the last character in this
     * file is '\r', read it in properly as a Mac format line. */
    if (len == 0 && !ISSET(NO_CONVERT) && input == '\r') {
	len = 1;

	buf[0] = input;
	buf[1] = '\0';
    }
#endif

    /* Did we not get a newline and still have stuff to do? */
    if (len > 0) {
#ifndef NANO_TINY
	/* If file conversion isn't disabled and the last character in
	 * this file is '\r', set format to Mac if we currently think
	 * the file is a *nix file, or to both DOS and Mac if we
	 * currently think the file is a DOS file. */
	if (!ISSET(NO_CONVERT) && buf[len - 1] == '\r' &&
		(format == 0 || format == 1))
	    format += 2;
#endif

	/* Read in the last line properly. */
	fileptr = read_line(buf, fileptr, &first_line_ins, len);
	num_lines++;
    }

    free(buf);

    /* If we didn't get a file and we don't already have one, open a
     * blank buffer. */
    if (fileptr == NULL)
	open_buffer("");

    /* Attach the file we got to the filestruct.  If we got a file of
     * zero bytes, don't do anything. */
    if (num_lines > 0) {
	/* If the file we got doesn't end in a newline, tack its last
	 * line onto the beginning of the line at current. */
	if (len > 0) {
	    size_t current_len = strlen(openfile->current->data);

	    /* Adjust the current x-coordinate to compensate for the
	     * change in the current line. */
	    if (num_lines == 1)
		openfile->current_x += len;
	    else
		openfile->current_x = len;

	    /* Tack the text at fileptr onto the beginning of the text
	     * at current. */
	    openfile->current->data =
		charealloc(openfile->current->data, len +
		current_len + 1);
	    charmove(openfile->current->data + len,
		openfile->current->data, current_len + 1);
	    strncpy(openfile->current->data, fileptr->data, len);

	    /* Don't destroy fileage, edittop, or filebot! */
	    if (fileptr == openfile->fileage)
		openfile->fileage = openfile->current;
	    if (fileptr == openfile->edittop)
		openfile->edittop = openfile->current;
	    if (fileptr == openfile->filebot)
		openfile->filebot = openfile->current;

	    /* Move fileptr back one line and blow away the old fileptr,
	     * since its text has been saved. */
	    fileptr = fileptr->prev;
	    if (fileptr != NULL) {
		if (fileptr->next != NULL)
		    free(fileptr->next);
	    }
	}

	/* Attach the line at current after the line at fileptr. */
	if (fileptr != NULL) {
	    fileptr->next = openfile->current;
	    openfile->current->prev = fileptr;
	}

	/* Renumber starting with the last line of the file we
	 * inserted. */
	renumber(openfile->current);
    }

    openfile->totsize += get_totsize(openfile->fileage,
	openfile->filebot);

    /* If the NO_NEWLINES flag isn't set, and text has been added to
     * the magicline (i.e. a file that doesn't end in a newline has been
     * inserted at the end of the current buffer), add a new magicline,
     * and move the current line down to it. */
    if (!ISSET(NO_NEWLINES) && openfile->filebot->data[0] != '\0') {
	new_magicline();
	openfile->current = openfile->filebot;
	openfile->current_x = 0;
    }

    /* Set the current place we want to the end of the last line of the
     * file we inserted. */
    openfile->placewewant = xplustabs();

#ifndef NANO_TINY
    if (format == 3)
	statusbar(
		P_("Read %lu line (Converted from DOS and Mac format)",
		"Read %lu lines (Converted from DOS and Mac format)",
		(unsigned long)num_lines), (unsigned long)num_lines);
    else if (format == 2) {
	openfile->fmt = MAC_FILE;
	statusbar(P_("Read %lu line (Converted from Mac format)",
		"Read %lu lines (Converted from Mac format)",
		(unsigned long)num_lines), (unsigned long)num_lines);
    } else if (format == 1) {
	openfile->fmt = DOS_FILE;
	statusbar(P_("Read %lu line (Converted from DOS format)",
		"Read %lu lines (Converted from DOS format)",
		(unsigned long)num_lines), (unsigned long)num_lines);
    } else
#endif
	statusbar(P_("Read %lu line", "Read %lu lines",
		(unsigned long)num_lines), (unsigned long)num_lines);
}

/* Open the file (and decide if it exists).  If newfie is TRUE, display
 * "New File" if the file is missing.  Otherwise, say "[filename] not
 * found".
 *
 * Return -2 if we say "New File", -1 if the file isn't opened, and 0
 * otherwise.  The file might still have an error while reading with a 0
 * return value.  *f is set to the opened file. */
int open_file(const char *filename, bool newfie, FILE **f)
{
    struct stat fileinfo;
    int fd;
    char *full_filename;

    assert(filename != NULL && f != NULL);

    /* Get the specified file's full path. */
    full_filename = get_full_path(filename);

    if (full_filename == NULL)
	full_filename = mallocstrcpy(NULL, filename);

    if (stat(full_filename, &fileinfo) == -1) {
	if (newfie) {
	    statusbar(_("New File"));
	    return -2;
	}
	statusbar(_("\"%s\" not found"), filename);
	beep();
	return -1;
    } else if (S_ISDIR(fileinfo.st_mode) || S_ISCHR(fileinfo.st_mode) ||
	S_ISBLK(fileinfo.st_mode)) {
	/* Don't open directories, character files, or block files.
	 * Sorry, /dev/sndstat! */
	statusbar(S_ISDIR(fileinfo.st_mode) ?
		_("\"%s\" is a directory") :
		_("\"%s\" is a device file"), filename);
	beep();
	return -1;
    } else if ((fd = open(full_filename, O_RDONLY)) == -1) {
	statusbar(_("Error reading %s: %s"), filename,
		strerror(errno));
	beep();
 	return -1;
     } else {
	/* The file is A-OK.  Open it. */
	*f = fdopen(fd, "rb");

	if (*f == NULL) {
	    statusbar(_("Error reading %s: %s"), filename,
		strerror(errno));
	    beep();
	    close(fd);
	} else
	    statusbar(_("Reading File"));
    }

    free(full_filename);

    return 0;
}

/* This function will return the name of the first available extension
 * of a filename (starting with [name][suffix], then [name][suffix].1,
 * etc.).  Memory is allocated for the return value.  If no writable
 * extension exists, we return "". */
char *get_next_filename(const char *name, const char *suffix)
{
    static int ulmax_digits = -1;
    unsigned long i = 0;
    char *buf;
    size_t namelen, suffixlen;

    assert(name != NULL && suffix != NULL);

    if (ulmax_digits == -1)
	ulmax_digits = digits(ULONG_MAX);

    namelen = strlen(name);
    suffixlen = strlen(suffix);

    buf = charalloc(namelen + suffixlen + ulmax_digits + 2);
    sprintf(buf, "%s%s", name, suffix);

    while (TRUE) {
	struct stat fs;

	if (stat(buf, &fs) == -1)
	    return buf;
	if (i == ULONG_MAX)
	    break;

	i++;
	sprintf(buf + namelen + suffixlen, ".%lu", i);
    }

    /* We get here only if there is no possible save file.  Blank out
     * the filename to indicate this. */
    null_at(&buf, 0);

    return buf;
}

/* Insert a file into a new buffer if the MULTIBUFFER flag is set, or
 * into the current buffer if it isn't.  If execute is TRUE, insert the
 * output of an executed command instead of a file. */
void do_insertfile(
#ifndef NANO_TINY
	bool execute
#else
	void
#endif
	)
{
    int i;
    const char *msg;
    char *ans = mallocstrcpy(NULL, "");
	/* The last answer the user typed at the statusbar prompt. */
    filestruct *edittop_save = openfile->edittop;
    size_t current_x_save = openfile->current_x;
    ssize_t current_y_save = openfile->current_y;
    bool at_edittop = FALSE;
	/* Whether we're at the top of the edit window. */

    while (TRUE) {
#ifndef NANO_TINY
	if (execute) {
	    msg =
#ifdef ENABLE_MULTIBUFFER
		ISSET(MULTIBUFFER) ?
		_("Command to execute in new buffer [from %s] ") :
#endif
		_("Command to execute [from %s] ");
	} else {
#endif
	    msg =
#ifdef ENABLE_MULTIBUFFER
		ISSET(MULTIBUFFER) ?
		_("File to insert into new buffer [from %s] ") :
#endif
		_("File to insert [from %s] ");
#ifndef NANO_TINY
	}
#endif

	i = do_prompt(TRUE,
#ifndef DISABLE_TABCOMP
		TRUE,
#endif
#ifndef NANO_TINY
		execute ? extcmd_list :
#endif
		insertfile_list, ans,
#ifndef NANO_TINY
		NULL,
#endif
		edit_refresh, msg,
#ifndef DISABLE_OPERATINGDIR
		operating_dir != NULL && strcmp(operating_dir,
		".") != 0 ? operating_dir :
#endif
		"./");

	/* If we're in multibuffer mode and the filename or command is
	 * blank, open a new buffer instead of canceling.  If the
	 * filename or command begins with a newline (i.e. an encoded
	 * null), treat it as though it's blank. */
	if (i == -1 || ((i == -2 || answer[0] == '\n')
#ifdef ENABLE_MULTIBUFFER
		&& !ISSET(MULTIBUFFER)
#endif
		)) {
	    statusbar(_("Cancelled"));
	    break;
	} else {
	    size_t pww_save = openfile->placewewant;

	    ans = mallocstrcpy(ans, answer);

#ifndef NANO_TINY
#ifdef ENABLE_MULTIBUFFER
	    if (i == TOGGLE_MULTIBUFFER_KEY) {
		/* Don't allow toggling if we're in view mode. */
		if (!ISSET(VIEW_MODE))
		    TOGGLE(MULTIBUFFER);
		continue;
	    } else
#endif
	    if (i == NANO_TOOTHERINSERT_KEY) {
		execute = !execute;
		continue;
	    }
#ifndef DISABLE_BROWSER
	    else
#endif
#endif /* !NANO_TINY */

#ifndef DISABLE_BROWSER
	    if (i == NANO_TOFILES_KEY) {
		char *tmp = do_browse_from(answer);

		if (tmp == NULL)
		    continue;

		/* We have a file now.  Indicate this. */
		free(answer);
		answer = tmp;

		i = 0;
	    }
#endif

	    /* If we don't have a file yet, go back to the statusbar
	     * prompt. */
	    if (i != 0
#ifdef ENABLE_MULTIBUFFER
		&& (i != -2 || !ISSET(MULTIBUFFER))
#endif
		)
		continue;

#ifdef ENABLE_MULTIBUFFER
	    if (!ISSET(MULTIBUFFER)) {
#endif
		/* If we're not inserting into a new buffer, partition
		 * the filestruct so that it contains no text and hence
		 * looks like a new buffer, and keep track of whether
		 * the top of the partition is the top of the edit
		 * window. */
		filepart = partition_filestruct(openfile->current,
			openfile->current_x, openfile->current,
			openfile->current_x);
		at_edittop =
			(openfile->fileage == openfile->edittop);
#ifdef ENABLE_MULTIBUFFER
	    }
#endif

	    /* Convert newlines to nulls, just before we insert the file
	     * or execute the command. */
	    sunder(answer);
	    align(&answer);

#ifndef NANO_TINY
	    if (execute) {
#ifdef ENABLE_MULTIBUFFER
		if (ISSET(MULTIBUFFER))
		    /* Open a blank buffer. */
		    open_buffer("");
#endif

		/* Save the command's output in the current buffer. */
		execute_command(answer);

#ifdef ENABLE_MULTIBUFFER
		if (ISSET(MULTIBUFFER)) {
		    /* Move back to the beginning of the first line of
		     * the buffer. */
		    openfile->current = openfile->fileage;
		    openfile->current_x = 0;
		    openfile->placewewant = 0;
		}
#endif
	    } else {
#endif /* !NANO_TINY */
		/* Make sure the path to the file specified in answer is
		 * tilde-expanded. */
		answer = mallocstrassn(answer,
			real_dir_from_tilde(answer));

		/* Save the file specified in answer in the current
		 * buffer. */
		open_buffer(answer);
#ifndef NANO_TINY
	    }
#endif

#ifdef ENABLE_MULTIBUFFER
	    if (ISSET(MULTIBUFFER))
		/* Update the screen to account for the current
		 * buffer. */
		display_buffer();
	    else
#endif
	    {
		filestruct *top_save = openfile->fileage;

		/* If we were at the top of the edit window before, set
		 * the saved value of edittop to the new top of the edit
		 * window. */
		if (at_edittop)
		    edittop_save = openfile->fileage;

		/* Update the current x-coordinate to account for the
		 * number of characters inserted on the current line. */
		openfile->current_x = strlen(openfile->filebot->data);
		if (openfile->fileage == openfile->filebot)
		    openfile->current_x += current_x_save;

		/* Update the current y-coordinate to account for the
		 * number of lines inserted. */
		openfile->current_y += current_y_save;

		/* Unpartition the filestruct so that it contains all
		 * the text again.  Note that we've replaced the
		 * non-text originally in the partition with the text in
		 * the inserted file/executed command output. */
		unpartition_filestruct(&filepart);

		/* Renumber starting with the beginning line of the old
		 * partition. */
		renumber(top_save);

		/* Restore the old edittop. */
		openfile->edittop = edittop_save;

		/* Restore the old place we want. */
		openfile->placewewant = pww_save;

		/* Mark the file as modified. */
		set_modified();

		/* Update the screen. */
		edit_refresh();
	    }

	    break;
	}
    }

    free(ans);
}

/* Insert a file into a new buffer or the current buffer, depending on
 * whether the MULTIBUFFER flag is set.  If we're in view mode, only
 * allow inserting a file into a new buffer. */
void do_insertfile_void(void)
{
#ifdef ENABLE_MULTIBUFFER
    if (ISSET(VIEW_MODE) && !ISSET(MULTIBUFFER))
	statusbar(_("Key invalid in non-multibuffer mode"));
    else
#endif
	do_insertfile(
#ifndef NANO_TINY
		FALSE
#endif
		);

    display_main_list();
}

/* When passed "[relative path]" or "[relative path][filename]" in
 * origpath, return "[full path]" or "[full path][filename]" on success,
 * or NULL on error.  Do this if the file doesn't exist but the relative
 * path does, since the file could exist in memory but not yet on disk).
 * Don't do this if the relative path doesn't exist, since we won't be
 * able to go there. */
char *get_full_path(const char *origpath)
{
    struct stat fileinfo;
    char *d_here, *d_there, *d_there_file = NULL;
    const char *last_slash;
    bool path_only;

    if (origpath == NULL)
    	return NULL;

    /* Get the current directory.  If it doesn't exist, back up and try
     * again until we get a directory that does, and use that as the
     * current directory. */
    d_here = charalloc(PATH_MAX + 1);
    d_here = getcwd(d_here, PATH_MAX + 1);

    while (d_here == NULL) {
	if (chdir("..") == -1)
	    break;

	d_here = getcwd(d_here, PATH_MAX + 1);
    }

    /* If we succeeded, canonicalize it in d_here. */
    if (d_here != NULL) {
	align(&d_here);

	/* If the current directory isn't "/", tack a slash onto the end
	 * of it. */
	if (strcmp(d_here, "/") != 0) {
	    d_here = charealloc(d_here, strlen(d_here) + 2);
	    strcat(d_here, "/");
	}
    /* Otherwise, set d_here to "". */
    } else
	d_here = mallocstrcpy(NULL, "");

    d_there = real_dir_from_tilde(origpath);

    /* If stat()ing d_there fails, assume that d_there refers to a new
     * file that hasn't been saved to disk yet.  Set path_only to TRUE
     * if d_there refers to a directory, and FALSE otherwise. */
    path_only = (stat(d_there, &fileinfo) != -1 &&
	S_ISDIR(fileinfo.st_mode));

    /* If path_only is TRUE, make sure d_there ends in a slash. */
    if (path_only) {
	size_t d_there_len = strlen(d_there);

	if (d_there[d_there_len - 1] != '/') {
	    d_there = charealloc(d_there, d_there_len + 2);
	    strcat(d_there, "/");
	}
    }

    /* Search for the last slash in d_there. */
    last_slash = strrchr(d_there, '/');

    /* If we didn't find one, then make sure the answer is in the format
     * "d_here/d_there". */
    if (last_slash == NULL) {
	assert(!path_only);

	d_there_file = d_there;
	d_there = d_here;
    } else {
	/* If path_only is FALSE, then save the filename portion of the
	 * answer (everything after the last slash) in d_there_file. */
	if (!path_only)
	    d_there_file = mallocstrcpy(NULL, last_slash + 1);

	/* Remove the filename portion of the answer from d_there. */
	null_at(&d_there, last_slash - d_there + 1);

	/* Go to the path specified in d_there. */
	if (chdir(d_there) == -1) {
	    free(d_there);
	    d_there = NULL;
	} else {
	    free(d_there);

	    /* Get the full path. */
	    d_there = charalloc(PATH_MAX + 1);
	    d_there = getcwd(d_there, PATH_MAX + 1);

	    /* If we succeeded, canonicalize it in d_there. */
	    if (d_there != NULL) {
		align(&d_there);

		/* If the current directory isn't "/", tack a slash onto
		 * the end of it. */
		if (strcmp(d_there, "/") != 0) {
		    d_there = charealloc(d_there, strlen(d_there) + 2);
		    strcat(d_there, "/");
		}
	    } else
		/* Otherwise, set path_only to TRUE, so that we clean up
		 * correctly, free all allocated memory, and return
		 * NULL. */
		path_only = TRUE;

	    /* Finally, go back to the path specified in d_here,
	     * where we were before.  We don't check for a chdir()
	     * error, since we can do nothing if we get one. */
	    chdir(d_here);

	    /* Free d_here, since we're done using it. */
	    free(d_here);
	}
    }

    /* At this point, if path_only is FALSE and d_there isn't NULL,
     * d_there contains the path portion of the answer and d_there_file
     * contains the filename portion of the answer.  If this is the
     * case, tack the latter onto the end of the former.  d_there will
     * then contain the complete answer. */
    if (!path_only && d_there != NULL) {
	d_there = charealloc(d_there, strlen(d_there) +
		strlen(d_there_file) + 1);
	strcat(d_there, d_there_file);
    }

    /* Free d_there_file, since we're done using it. */
    if (d_there_file != NULL)
	free(d_there_file);

    return d_there;
}

/* Return the full version of path, as returned by get_full_path().  On
 * error, if path doesn't reference a directory, or if the directory
 * isn't writable, return NULL. */
char *check_writable_directory(const char *path)
{
    char *full_path = get_full_path(path);

    /* If get_full_path() fails, return NULL. */
    if (full_path == NULL)
	return NULL;

    /* If we can't write to path or path isn't a directory, return
     * NULL. */
    if (access(full_path, W_OK) != 0 ||
	full_path[strlen(full_path) - 1] != '/') {
	free(full_path);
	return NULL;
    }

    /* Otherwise, return the full path. */
    return full_path;
}

#ifdef ELKS
#define P_tmpdir	"/tmp/tmp.XXXXXX"

int mkstemp(char *template)
{
    char *p = mktemp(template);
    if (p)
        return creat(p, 0666);
    return -1;
}
#endif

/* This function calls mkstemp(($TMPDIR|P_tmpdir|/tmp/)"nano.XXXXXX").
 * On success, it returns the malloc()ed filename and corresponding FILE
 * stream, opened in "r+b" mode.  On error, it returns NULL for the
 * filename and leaves the FILE stream unchanged. */
char *safe_tempfile(FILE **f)
{
    char *full_tempdir = NULL;
    const char *tmpdir_env;
    int fd;
    mode_t original_umask = 0;

    assert(f != NULL);

    /* If $TMPDIR is set, set tempdir to it, run it through
     * get_full_path(), and save the result in full_tempdir.  Otherwise,
     * leave full_tempdir set to NULL. */
    tmpdir_env = getenv("TMPDIR");
    if (tmpdir_env != NULL)
	full_tempdir = check_writable_directory(tmpdir_env);

    /* If $TMPDIR is unset, empty, or not a writable directory, and
     * full_tempdir is NULL, try P_tmpdir instead. */
    if (full_tempdir == NULL)
	full_tempdir = check_writable_directory(P_tmpdir);

    /* if P_tmpdir is NULL, use /tmp. */
    if (full_tempdir == NULL)
	full_tempdir = mallocstrcpy(NULL, "/tmp/");

    full_tempdir = charealloc(full_tempdir, strlen(full_tempdir) + 12);
    strcat(full_tempdir, "nano.XXXXXX");

    original_umask = umask(0);
    umask(S_IRWXG | S_IRWXO);

    fd = mkstemp(full_tempdir);

    if (fd != -1)
	*f = fdopen(fd, "r+b");
    else {
	free(full_tempdir);
	full_tempdir = NULL;
    }

    umask(original_umask);

    return full_tempdir;
}

#ifndef DISABLE_OPERATINGDIR
/* Initialize full_operating_dir based on operating_dir. */
void init_operating_dir(void)
{
    assert(full_operating_dir == NULL);

    if (operating_dir == NULL)
	return;

    full_operating_dir = get_full_path(operating_dir);

    /* If get_full_path() failed or the operating directory is
     * inaccessible, unset operating_dir. */
    if (full_operating_dir == NULL || chdir(full_operating_dir) == -1) {
	free(full_operating_dir);
	full_operating_dir = NULL;
	free(operating_dir);
	operating_dir = NULL;
    }
}

/* Check to see if we're inside the operating directory.  Return FALSE
 * if we are, or TRUE otherwise.  If allow_tabcomp is TRUE, allow
 * incomplete names that would be matches for the operating directory,
 * so that tab completion will work. */
bool check_operating_dir(const char *currpath, bool allow_tabcomp)
{
    /* full_operating_dir is global for memory cleanup.  It should have
     * already been initialized by init_operating_dir().  Also, a
     * relative operating directory path will only be handled properly
     * if this is done. */

    char *fullpath;
    bool retval = FALSE;
    const char *whereami1, *whereami2 = NULL;

    /* If no operating directory is set, don't bother doing anything. */
    if (operating_dir == NULL)
	return FALSE;

    assert(full_operating_dir != NULL);

    fullpath = get_full_path(currpath);

    /* If fullpath is NULL, it means some directory in the path doesn't
     * exist or is unreadable.  If allow_tabcomp is FALSE, then currpath
     * is what the user typed somewhere.  We don't want to report a
     * non-existent directory as being outside the operating directory,
     * so we return FALSE.  If allow_tabcomp is TRUE, then currpath
     * exists, but is not executable.  So we say it isn't in the
     * operating directory. */
    if (fullpath == NULL)
	return allow_tabcomp;

    whereami1 = strstr(fullpath, full_operating_dir);
    if (allow_tabcomp)
	whereami2 = strstr(full_operating_dir, fullpath);

    /* If both searches failed, we're outside the operating directory.
     * Otherwise, check the search results.  If the full operating
     * directory path is not at the beginning of the full current path
     * (for normal usage) and vice versa (for tab completion, if we're
     * allowing it), we're outside the operating directory. */
    if (whereami1 != fullpath && whereami2 != full_operating_dir)
	retval = TRUE;
    free(fullpath);

    /* Otherwise, we're still inside it. */
    return retval;
}
#endif

#ifndef NANO_TINY
void init_backup_dir(void)
{
    char *full_backup_dir;

    if (backup_dir == NULL)
	return;

    full_backup_dir = get_full_path(backup_dir);

    /* If get_full_path() failed or the backup directory is
     * inaccessible, unset backup_dir. */
    if (full_backup_dir == NULL ||
	full_backup_dir[strlen(full_backup_dir) - 1] != '/') {
	free(full_backup_dir);
	free(backup_dir);
	backup_dir = NULL;
    } else {
	free(backup_dir);
	backup_dir = full_backup_dir;
    }
}
#endif

/* Read from inn, write to out.  We assume inn is opened for reading,
 * and out for writing.  We return 0 on success, -1 on read error, or -2
 * on write error. */
int copy_file(FILE *inn, FILE *out)
{
    char buf[BUFSIZ];
    size_t charsread;
    int retval = 0;

    assert(inn != NULL && out != NULL);

    do {
	charsread = fread(buf, sizeof(char), BUFSIZ, inn);
	if (charsread == 0 && ferror(inn)) {
	    retval = -1;
	    break;
	}
	if (fwrite(buf, sizeof(char), charsread, out) < charsread) {
	    retval = -2;
	    break;
	}
    } while (charsread > 0);

    if (fclose(inn) == EOF)
	retval = -1;
    if (fclose(out) == EOF)
	retval = -2;

    return retval;
}

/* Write a file out to disk.  If f_open isn't NULL, we assume that it is
 * a stream associated with the file, and we don't try to open it
 * ourselves.  If tmp is TRUE, we set the umask to disallow anyone else
 * from accessing the file, we don't set the filename to its name, and
 * we don't print out how many lines we wrote on the statusbar.
 *
 * tmp means we are writing a temporary file in a secure fashion.  We
 * use it when spell checking or dumping the file on an error.  If
 * append is APPEND, it means we are appending instead of overwriting.
 * If append is PREPEND, it means we are prepending instead of
 * overwriting.  If nonamechange is TRUE, we don't change the current
 * filename.  nonamechange is ignored if tmp is FALSE, we're appending,
 * or we're prepending.
 *
 * Return TRUE on success or FALSE on error. */
bool write_file(const char *name, FILE *f_open, bool tmp, append_type
	append, bool nonamechange)
{
    bool retval = FALSE;
	/* Instead of returning in this function, you should always
	 * set retval and then goto cleanup_and_exit. */
    size_t lineswritten = 0;
    const filestruct *fileptr = openfile->fileage;
    int fd;
	/* The file descriptor we use. */
    mode_t original_umask = 0;
	/* Our umask, from when nano started. */
    bool realexists;
	/* The result of stat().  TRUE if the file exists, FALSE
	 * otherwise.  If name is a link that points nowhere, realexists
	 * is FALSE. */
    struct stat st;
	/* The status fields filled in by stat(). */
    bool anyexists;
	/* The result of lstat().  The same as realexists, unless name
	 * is a link. */
    struct stat lst;
	/* The status fields filled in by lstat(). */
    char *realname;
	/* name after tilde expansion. */
    FILE *f = NULL;
	/* The actual file, realname, we are writing to. */
    char *tempname = NULL;
	/* The temp file name we write to on prepend. */

    assert(name != NULL);

    if (name[0] == '\0')
	return -1;

    if (f_open != NULL)
	f = f_open;

    if (!tmp)
	titlebar(NULL);

    realname = real_dir_from_tilde(name);

#ifndef DISABLE_OPERATINGDIR
    /* If we're writing a temporary file, we're probably going outside
     * the operating directory, so skip the operating directory test. */
    if (!tmp && check_operating_dir(realname, FALSE)) {
	statusbar(_("Can't write outside of %s"), operating_dir);
	goto cleanup_and_exit;
    }
#endif

    anyexists = (lstat(realname, &lst) != -1);

    /* If the temp file exists and isn't already open, give up. */
    if (tmp && anyexists && f_open == NULL)
	goto cleanup_and_exit;

    /* If NOFOLLOW_SYMLINKS is set, it doesn't make sense to prepend or
     * append to a symlink.  Here we warn about the contradiction. */
    if (ISSET(NOFOLLOW_SYMLINKS) && anyexists && S_ISLNK(lst.st_mode)) {
	statusbar(
		_("Cannot prepend or append to a symlink with --nofollow set"));
	goto cleanup_and_exit;
    }

    /* Save the state of the file at the end of the symlink (if there is
     * one). */
    realexists = (stat(realname, &st) != -1);

#ifndef NANO_TINY
    /* We backup only if the backup toggle is set, the file isn't
     * temporary, and the file already exists.  Furthermore, if we
     * aren't appending, prepending, or writing a selection, we backup
     * only if the file has not been modified by someone else since nano
     * opened it. */
    if (ISSET(BACKUP_FILE) && !tmp && realexists && ((append !=
	OVERWRITE || openfile->mark_set) ||
	openfile->current_stat->st_mtime == st.st_mtime)) {
	FILE *backup_file;
	char *backupname;
	struct utimbuf filetime;
	int copy_status;

	/* Save the original file's access and modification times. */
	filetime.actime = openfile->current_stat->st_atime;
	filetime.modtime = openfile->current_stat->st_mtime;

	if (f_open == NULL) {
	    /* Open the original file to copy to the backup. */
	    f = fopen(realname, "rb");

	    if (f == NULL) {
		statusbar(_("Error reading %s: %s"), realname,
			strerror(errno));
		beep();
		/* If we can't read from the original file, go on, since
		 * only saving the original file is better than saving
		 * nothing. */
		goto skip_backup;
	    }
	}

	/* If backup_dir is set, we set backupname to
	 * backup_dir/backupname~[.number], where backupname is the
	 * canonicalized absolute pathname of realname with every '/'
	 * replaced with a '!'.  This means that /home/foo/file is
	 * backed up in backup_dir/!home!foo!file~[.number]. */
	if (backup_dir != NULL) {
	    char *backuptemp = get_full_path(realname);

	    if (backuptemp == NULL)
		/* If get_full_path() failed, we don't have a
		 * canonicalized absolute pathname, so just use the
		 * filename portion of the pathname.  We use tail() so
		 * that e.g. ../backupname will be backed up in
		 * backupdir/backupname~ instead of
		 * backupdir/../backupname~. */
		backuptemp = mallocstrcpy(NULL, tail(realname));
	    else {
		size_t i = 0;

		for (; backuptemp[i] != '\0'; i++) {
		    if (backuptemp[i] == '/')
			backuptemp[i] = '!';
		}
	    }

	    backupname = charalloc(strlen(backup_dir) +
		strlen(backuptemp) + 1);
	    sprintf(backupname, "%s%s", backup_dir, backuptemp);
	    free(backuptemp);
	    backuptemp = get_next_filename(backupname, "~");
	    if (backuptemp[0] == '\0') {
		statusbar(_("Error writing %s: %s"), backupname,
		    _("Too many backup files?"));
		free(backuptemp);
		free(backupname);
		/* If we can't write to the backup, go on, since only
		 * saving the original file is better than saving
		 * nothing. */
		goto skip_backup;
	    } else {
		free(backupname);
		backupname = backuptemp;
	    }
	} else {
	    backupname = charalloc(strlen(realname) + 2);
	    sprintf(backupname, "%s~", realname);
	}

	/* Open the destination backup file.  Before we write to it, we
	 * set its permissions, so no unauthorized person can read it as
	 * we write. */
	backup_file = fopen(backupname, "wb");

	if (backup_file == NULL || chmod(backupname,
		openfile->current_stat->st_mode) == -1) {
	    statusbar(_("Error writing %s: %s"), backupname,
		strerror(errno));
	    free(backupname);
	    if (backup_file != NULL)
		fclose(backup_file);
	    /* If we can't write to the backup, go on, since only saving
	     * the original file is better than saving nothing. */
	    goto skip_backup;
	}

#ifdef DEBUG
	fprintf(stderr, "Backing up %s to %s\n", realname, backupname);
#endif

	/* Copy the file. */
	copy_status = copy_file(f, backup_file);

	/* And set its metadata. */
	if (copy_status != 0 || chown(backupname,
		openfile->current_stat->st_uid,
		openfile->current_stat->st_gid) == -1 ||
		utime(backupname, &filetime) == -1) {
	    if (copy_status == -1) {
		statusbar(_("Error reading %s: %s"), realname,
			strerror(errno));
		beep();
	    } else
		statusbar(_("Error writing %s: %s"), backupname,
			strerror(errno));
	    /* If we can't read from or write to the backup, go on,
	     * since only saving the original file is better than saving
	     * nothing. */
	}

	free(backupname);
    }

  skip_backup:
#endif /* !NANO_TINY */

    /* If NOFOLLOW_SYMLINKS is set and the file is a link, we aren't
     * doing prepend or append.  So we delete the link first, and just
     * overwrite. */
    if (ISSET(NOFOLLOW_SYMLINKS) && anyexists && S_ISLNK(lst.st_mode) &&
	unlink(realname) == -1) {
	statusbar(_("Error writing %s: %s"), realname, strerror(errno));
	goto cleanup_and_exit;
    }

    if (f_open == NULL) {
	original_umask = umask(0);

	/* If we create a temp file, we don't let anyone else access it.
	 * We create a temp file if tmp is TRUE. */
	if (tmp)
	    umask(S_IRWXG | S_IRWXO);
	else
	    umask(original_umask);
    }

    /* If we're prepending, copy the file to a temp file. */
    if (append == PREPEND) {
	int fd_source;
	FILE *f_source = NULL;

	if (f == NULL) {
	    f = fopen(realname, "rb");

	    if (f == NULL) {
		statusbar(_("Error reading %s: %s"), realname,
			strerror(errno));
		beep();
		goto cleanup_and_exit;
	    }
	}

	tempname = safe_tempfile(&f);

	if (tempname == NULL) {
	    statusbar(_("Error writing temp file: %s"),
		strerror(errno));
	    goto cleanup_and_exit;
	}

	if (f_open == NULL) {
	    fd_source = open(realname, O_RDONLY | O_CREAT);

	    if (fd_source != -1) {
		f_source = fdopen(fd_source, "rb");
		if (f_source == NULL) {
		    statusbar(_("Error reading %s: %s"), realname,
			strerror(errno));
		    beep();
		    close(fd_source);
		    fclose(f);
		    unlink(tempname);
		    goto cleanup_and_exit;
		}
	    }
	}

	if (copy_file(f_source, f) != 0) {
	    statusbar(_("Error writing %s: %s"), tempname,
		strerror(errno));
	    unlink(tempname);
	    goto cleanup_and_exit;
	}
    }

    if (f_open == NULL) {
	/* Now open the file in place.  Use O_EXCL if tmp is TRUE.  This
	 * is copied from joe, because wiggy says so *shrug*. */
	fd = open(realname, O_WRONLY | O_CREAT | ((append == APPEND) ?
		O_APPEND : (tmp ? O_EXCL : O_TRUNC)), S_IRUSR |
		S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);

	/* Set the umask back to the user's original value. */
	umask(original_umask);

	/* If we couldn't open the file, give up. */
	if (fd == -1) {
	    statusbar(_("Error writing %s: %s"), realname,
		strerror(errno));

	    /* tempname has been set only if we're prepending. */
	    if (tempname != NULL)
		unlink(tempname);
	    goto cleanup_and_exit;
	}

	f = fdopen(fd, (append == APPEND) ? "ab" : "wb");

	if (f == NULL) {
	    statusbar(_("Error writing %s: %s"), realname,
		strerror(errno));
	    close(fd);
	    goto cleanup_and_exit;
	}
    }

    /* There might not be a magicline.  There won't be when writing out
     * a selection. */
    assert(openfile->fileage != NULL && openfile->filebot != NULL);

    while (fileptr != NULL) {
	size_t data_len = strlen(fileptr->data), size;

	/* Convert newlines to nulls, just before we write to disk. */
	sunder(fileptr->data);

	size = fwrite(fileptr->data, sizeof(char), data_len, f);

	/* Convert nulls to newlines.  data_len is the string's real
	 * length. */
	unsunder(fileptr->data, data_len);

	if (size < data_len) {
	    statusbar(_("Error writing %s: %s"), realname,
		strerror(errno));
	    fclose(f);
	    goto cleanup_and_exit;
	}

	/* If we're on the last line of the file, don't write a newline
	 * character after it.  If the last line of the file is blank,
	 * this means that zero bytes are written, in which case we
	 * don't count the last line in the total lines written. */
	if (fileptr == openfile->filebot) {
	    if (fileptr->data[0] == '\0')
		lineswritten--;
	} else {
#ifndef NANO_TINY
	    if (openfile->fmt == DOS_FILE || openfile->fmt ==
		MAC_FILE) {
		if (putc('\r', f) == EOF) {
		    statusbar(_("Error writing %s: %s"), realname,
			strerror(errno));
		    fclose(f);
		    goto cleanup_and_exit;
		}
	    }

	    if (openfile->fmt != MAC_FILE) {
#endif
		if (putc('\n', f) == EOF) {
		    statusbar(_("Error writing %s: %s"), realname,
			strerror(errno));
		    fclose(f);
		    goto cleanup_and_exit;
		}
#ifndef NANO_TINY
	    }
#endif
	}

	fileptr = fileptr->next;
	lineswritten++;
    }

    /* If we're prepending, open the temp file, and append it to f. */
    if (append == PREPEND) {
	int fd_source;
	FILE *f_source = NULL;

	fd_source = open(tempname, O_RDONLY | O_CREAT);

	if (fd_source != -1) {
	    f_source = fdopen(fd_source, "rb");
	    if (f_source == NULL)
		close(fd_source);
	}

	if (f_source == NULL) {
	    statusbar(_("Error reading %s: %s"), tempname,
		strerror(errno));
	    beep();
	    fclose(f);
	    goto cleanup_and_exit;
	}

	if (copy_file(f_source, f) == -1 || unlink(tempname) == -1) {
	    statusbar(_("Error writing %s: %s"), realname,
		strerror(errno));
	    goto cleanup_and_exit;
	}
    } else
	fclose(f);

    if (!tmp && append == OVERWRITE) {
	if (!nonamechange) {
	    openfile->filename = mallocstrcpy(openfile->filename,
		realname);
#ifdef ENABLE_COLOR
	    /* We might have changed the filename, so update the colors
	     * to account for it, and then make sure we're using
	     * them. */
	    color_update();
	    color_init();

	    /* If color syntaxes are available and turned on, we need to
	     * call edit_refresh(). */
	    if (openfile->colorstrings != NULL &&
		!ISSET(NO_COLOR_SYNTAX))
		edit_refresh();
#endif
	}

#ifndef NANO_TINY
	/* Update current_stat to reference the file as it is now. */
	if (openfile->current_stat == NULL)
	    openfile->current_stat =
		(struct stat *)nmalloc(sizeof(struct stat));
	stat(realname, openfile->current_stat);
#endif

	statusbar(P_("Wrote %lu line", "Wrote %lu lines",
		(unsigned long)lineswritten),
		(unsigned long)lineswritten);
	openfile->modified = FALSE;
	titlebar(NULL);
    }

    retval = TRUE;

  cleanup_and_exit:
    free(realname);
    if (tempname != NULL)
	free(tempname);

    return retval;
}

#ifndef NANO_TINY
/* Write a marked selection from a file out to disk.  Return TRUE on
 * success or FALSE on error. */
bool write_marked_file(const char *name, FILE *f_open, bool tmp,
	append_type append)
{
    bool retval;
    bool old_modified = openfile->modified;
	/* write_file() unsets the modified flag. */
    bool added_magicline = FALSE;
	/* Whether we added a magicline after filebot. */
    filestruct *top, *bot;
    size_t top_x, bot_x;

    assert(openfile->mark_set);

    /* Partition the filestruct so that it contains only the marked
     * text. */
    mark_order((const filestruct **)&top, &top_x,
	(const filestruct **)&bot, &bot_x, NULL);
    filepart = partition_filestruct(top, top_x, bot, bot_x);

    /* Handle the magicline if the NO_NEWLINES flag isn't set.  If the
     * line at filebot is blank, treat it as the magicline and hence the
     * end of the file.  Otherwise, add a magicline and treat it as the
     * end of the file. */
    if (!ISSET(NO_NEWLINES) &&
	(added_magicline = (openfile->filebot->data[0] != '\0')))
	new_magicline();

    retval = write_file(name, f_open, tmp, append, TRUE);

    /* If the NO_NEWLINES flag isn't set, and we added a magicline,
     * remove it now. */
    if (!ISSET(NO_NEWLINES) && added_magicline)
	remove_magicline();

    /* Unpartition the filestruct so that it contains all the text
     * again. */
    unpartition_filestruct(&filepart);

    if (old_modified)
	set_modified();

    return retval;
}
#endif /* !NANO_TINY */

/* Write the current file to disk.  If the mark is on, write the current
 * marked selection to disk.  If exiting is TRUE, write the file to disk
 * regardless of whether the mark is on, and without prompting if the
 * TEMP_FILE flag is set.  Return TRUE on success or FALSE on error. */
bool do_writeout(bool exiting)
{
    int i;
    append_type append = OVERWRITE;
    char *ans;
	/* The last answer the user typed at the statusbar prompt. */
#ifdef NANO_EXTRA
    static bool did_credits = FALSE;
#endif
    bool retval = FALSE;

    currshortcut = writefile_list;

    if (exiting && openfile->filename[0] != '\0' && ISSET(TEMP_FILE)) {
	retval = write_file(openfile->filename, NULL, FALSE, OVERWRITE,
		FALSE);

	/* Write succeeded. */
	if (retval)
	    return retval;
    }

    ans = mallocstrcpy(NULL,
#ifndef NANO_TINY
	(!exiting && openfile->mark_set) ? "" :
#endif
	openfile->filename);

    while (TRUE) {
	const char *msg;
#ifndef NANO_TINY
	const char *formatstr, *backupstr;

	formatstr = (openfile->fmt == DOS_FILE) ?
		_(" [DOS Format]") : (openfile->fmt == MAC_FILE) ?
		_(" [Mac Format]") : "";

	backupstr = ISSET(BACKUP_FILE) ? _(" [Backup]") : "";

	/* If we're using restricted mode, don't display the "Write
	 * Selection to File" prompt.  This function is disabled, since
	 * it allows reading from or writing to files not specified on
	 * the command line. */
	if (!ISSET(RESTRICTED) && !exiting && openfile->mark_set)
	    msg = (append == PREPEND) ?
		_("Prepend Selection to File") : (append == APPEND) ?
		_("Append Selection to File") :
		_("Write Selection to File");
	else
#endif /* !NANO_TINY */
	    msg = (append == PREPEND) ? _("File Name to Prepend to") :
		(append == APPEND) ? _("File Name to Append to") :
		_("File Name to Write");

	/* If we're using restricted mode, the filename isn't blank,
	 * and we're at the "Write File" prompt, disable tab
	 * completion. */
	i = do_prompt(!ISSET(RESTRICTED) ||
		openfile->filename[0] == '\0',
#ifndef DISABLE_TABCOMP
		TRUE,
#endif
		writefile_list, ans,
#ifndef NANO_TINY
		NULL,
#endif
		edit_refresh, "%s%s%s", msg,
#ifndef NANO_TINY
		formatstr, backupstr
#else
		"", ""
#endif
		);

	/* If the filename or command begins with a newline (i.e. an
	 * encoded null), treat it as though it's blank. */
	if (i < 0 || answer[0] == '\n') {
	    statusbar(_("Cancelled"));
	    retval = FALSE;
	    break;
	} else {
	    ans = mallocstrcpy(ans, answer);

#ifndef DISABLE_BROWSER
	    if (i == NANO_TOFILES_KEY) {
		char *tmp = do_browse_from(answer);

		if (tmp == NULL)
		    continue;

		/* We have a file now.  Indicate this. */
		free(answer);
		answer = tmp;
	    } else
#endif /* !DISABLE_BROWSER */
#ifndef NANO_TINY
	    if (i == TOGGLE_DOS_KEY) {
		openfile->fmt = (openfile->fmt == DOS_FILE) ? NIX_FILE :
			DOS_FILE;
		continue;
	    } else if (i == TOGGLE_MAC_KEY) {
		openfile->fmt = (openfile->fmt == MAC_FILE) ? NIX_FILE :
			MAC_FILE;
		continue;
	    } else if (i == TOGGLE_BACKUP_KEY) {
		TOGGLE(BACKUP_FILE);
		continue;
	    } else
#endif /* !NANO_TINY */
	    if (i == NANO_PREPEND_KEY) {
		append = (append == PREPEND) ? OVERWRITE : PREPEND;
		continue;
	    } else if (i == NANO_APPEND_KEY) {
		append = (append == APPEND) ? OVERWRITE : APPEND;
		continue;
	    }

#ifdef DEBUG
	    fprintf(stderr, "filename is %s\n", answer);
#endif

#ifdef NANO_EXTRA
	    /* If the current file has been modified, we've pressed
	     * Ctrl-X at the edit window to exit, we've pressed "y" at
	     * the "Save modified buffer" prompt to save, we've entered
	     * "zzy" as the filename to save under (hence "xyzzy"), and
	     * this is the first time we've done this, show an Easter
	     * egg.  Display the credits. */
	    if (!did_credits && exiting && !ISSET(TEMP_FILE) &&
		strcasecmp(answer, "zzy") == 0) {
		do_credits();
		did_credits = TRUE;
		retval = FALSE;
		break;
	    }
#endif

	    if (append == OVERWRITE) {
		size_t answer_len = strlen(answer);
		bool name_exists, do_warning;
		char *full_answer, *full_filename;
		struct stat st;

		/* Convert newlines to nulls, just before we get the
		 * full path. */
		sunder(answer);

		full_answer = get_full_path(answer);
		full_filename = get_full_path(openfile->filename);
		name_exists = (stat((full_answer == NULL) ? answer :
			full_answer, &st) != -1);
		if (openfile->filename[0] == '\0')
		    do_warning = name_exists;
		else
		    do_warning = (strcmp((full_answer == NULL) ?
			answer : full_answer, (full_filename == NULL) ?
			openfile->filename : full_filename) != 0);

		/* Convert nulls to newlines.  answer_len is the
		 * string's real length. */
		unsunder(answer, answer_len);

		if (full_filename != NULL)
		    free(full_filename);
		if (full_answer != NULL)
		    free(full_answer);

		if (do_warning) {
		    /* If we're using restricted mode, we aren't allowed
		     * to overwrite an existing file with the current
		     * file.  We also aren't allowed to change the name
		     * of the current file if it has one, because that
		     * would allow reading from or writing to files not
		     * specified on the command line. */
		    if (ISSET(RESTRICTED))
			continue;

		    if (name_exists) {
			i = do_yesno_prompt(FALSE,
				_("File exists, OVERWRITE ? "));
			if (i == 0 || i == -1)
			    continue;
		    } else
#ifndef NANO_TINY
		    if (exiting || !openfile->mark_set)
#endif
		    {
			i = do_yesno_prompt(FALSE,
				_("Save file under DIFFERENT NAME ? "));
			if (i == 0 || i == -1)
			    continue;
		    }
		}
	    }

	    /* Convert newlines to nulls, just before we save the
	     * file. */
	    sunder(answer);
	    align(&answer);

	    /* Here's where we allow the selected text to be written to
	     * a separate file.  If we're using restricted mode, this
	     * function is disabled, since it allows reading from or
	     * writing to files not specified on the command line. */
	    retval =
#ifndef NANO_TINY
		(!ISSET(RESTRICTED) && !exiting && openfile->mark_set) ?
		write_marked_file(answer, NULL, FALSE, append) :
#endif
		write_file(answer, NULL, FALSE, append, FALSE);

	    break;
	}
    }

    free(ans);

    return retval;
}

/* Write the current file to disk.  If the mark is on, write the current
 * marked selection to disk. */
void do_writeout_void(void)
{
    do_writeout(FALSE);
    display_main_list();
}

/* Return a malloc()ed string containing the actual directory, used to
 * convert ~user/ and ~/ notation. */
char *real_dir_from_tilde(const char *buf)
{
    char *retval;

    assert(buf != NULL);

    if (buf[0] == '~') {
	size_t i = 1;
	char *tilde_dir;

	/* Figure out how much of the string we need to compare. */
	for (; buf[i] != '/' && buf[i] != '\0'; i++)
	    ;

	/* Get the home directory. */
	if (i == 1) {
	    get_homedir();
	    tilde_dir = mallocstrcpy(NULL, homedir);
	} else {
	    const struct passwd *userdata;

	    tilde_dir = mallocstrncpy(NULL, buf, i + 1);
	    tilde_dir[i] = '\0';

	    do {
		userdata = getpwent();
	    } while (userdata != NULL && strcmp(userdata->pw_name,
		tilde_dir + 1) != 0);
	    endpwent();
	    if (userdata != NULL)
		tilde_dir = mallocstrcpy(tilde_dir, userdata->pw_dir);
	}

	retval = charalloc(strlen(tilde_dir) + strlen(buf + i) + 1);
	sprintf(retval, "%s%s", tilde_dir, buf + i);

	free(tilde_dir);
    } else
	retval = mallocstrcpy(NULL, buf);

    return retval;
}

#if !defined(DISABLE_TABCOMP) || !defined(DISABLE_BROWSER)
/* Our sort routine for file listings.  Sort alphabetically and
 * case-insensitively, and sort directories before filenames. */
int diralphasort(const void *va, const void *vb)
{
    struct stat fileinfo;
    const char *a = *(const char *const *)va;
    const char *b = *(const char *const *)vb;
    bool aisdir = stat(a, &fileinfo) != -1 && S_ISDIR(fileinfo.st_mode);
    bool bisdir = stat(b, &fileinfo) != -1 && S_ISDIR(fileinfo.st_mode);

    if (aisdir && !bisdir)
	return -1;
    if (!aisdir && bisdir)
	return 1;

    /* Standard function brain damage: We should be sorting
     * alphabetically and case-insensitively according to the current
     * locale, but there's no standard strcasecoll() function, so we
     * have to use multibyte strcasecmp() instead, */
    return mbstrcasecmp(a, b);
}

/* Free the memory allocated for array, which should contain len
 * elements. */
void free_chararray(char **array, size_t len)
{
    assert(array != NULL);

    for (; len > 0; len--)
	free(array[len - 1]);
    free(array);
}
#endif

#ifndef DISABLE_TABCOMP
/* Is the given path a directory? */
bool is_dir(const char *buf)
{
    char *dirptr;
    struct stat fileinfo;
    bool retval;

    assert(buf != NULL);

    dirptr = real_dir_from_tilde(buf);

    retval = (stat(dirptr, &fileinfo) != -1 &&
	S_ISDIR(fileinfo.st_mode));

    free(dirptr);

    return retval;
}

/* These functions, username_tab_completion(), cwd_tab_completion()
 * (originally exe_n_cwd_tab_completion()), and input_tab(), were
 * adapted from busybox 0.46 (cmdedit.c).  Here is the notice from that
 * file:
 *
 * Termios command line History and Editting, originally
 * intended for NetBSD sh (ash)
 * Copyright (c) 1999
 *      Main code:            Adam Rogoyski <rogoyski@cs.utexas.edu>
 *      Etc:                  Dave Cinege <dcinege@psychosis.com>
 *  Majorly adjusted/re-written for busybox:
 *                            Erik Andersen <andersee@debian.org>
 *
 * You may use this code as you wish, so long as the original author(s)
 * are attributed in any redistributions of the source code.
 * This code is 'as is' with no warranty.
 * This code may safely be consumed by a BSD or GPL license. */

/* We consider the first buflen characters of buf for ~username tab
 * completion. */
char **username_tab_completion(const char *buf, size_t *num_matches,
	size_t buflen)
{
    char **matches = NULL;
    const struct passwd *userdata;

    assert(buf != NULL && num_matches != NULL && buflen > 0);

    *num_matches = 0;

    while ((userdata = getpwent()) != NULL) {
	if (strncmp(userdata->pw_name, buf + 1, buflen - 1) == 0) {
	    /* Cool, found a match.  Add it to the list.  This makes a
	     * lot more sense to me (Chris) this way... */

#ifndef DISABLE_OPERATINGDIR
	    /* ...unless the match exists outside the operating
	     * directory, in which case just go to the next match. */
	    if (check_operating_dir(userdata->pw_dir, TRUE))
		continue;
#endif

	    matches = (char **)nrealloc(matches, (*num_matches + 1) *
		sizeof(char *));
	    matches[*num_matches] =
		charalloc(strlen(userdata->pw_name) + 2);
	    sprintf(matches[*num_matches], "~%s", userdata->pw_name);
	    ++(*num_matches);
	}
    }
    endpwent();

    return matches;
}

/* We consider the first buflen characters of buf for filename tab
 * completion. */
char **cwd_tab_completion(const char *buf, bool allow_files, size_t
	*num_matches, size_t buflen)
{
    char *dirname = mallocstrcpy(NULL, buf), *filename;
#ifndef DISABLE_OPERATINGDIR
    size_t dirnamelen;
#endif
    size_t filenamelen;
    char **matches = NULL;
    DIR *dir;
    const struct dirent *nextdir;

    assert(dirname != NULL && num_matches != NULL && buflen >= 0);

    *num_matches = 0;
    null_at(&dirname, buflen);

    /* Okie, if there's a / in the buffer, strip out the directory
     * part. */
    filename = strrchr(dirname, '/');
    if (filename != NULL) {
	char *tmpdirname = filename + 1;

	filename = mallocstrcpy(NULL, tmpdirname);
	*tmpdirname = '\0';
	tmpdirname = dirname;
	dirname = real_dir_from_tilde(dirname);
	free(tmpdirname);
    } else {
	filename = dirname;
	dirname = mallocstrcpy(NULL, "./");
    }

    assert(dirname[strlen(dirname) - 1] == '/');

    dir = opendir(dirname);

    if (dir == NULL) {
	/* Don't print an error, just shut up and return. */
	beep();
	free(filename);
	free(dirname);
	return NULL;
    }

#ifndef DISABLE_OPERATINGDIR
    dirnamelen = strlen(dirname);
#endif
    filenamelen = strlen(filename);

    while ((nextdir = readdir(dir)) != NULL) {
	bool skip_match = FALSE;

#ifdef DEBUG
	fprintf(stderr, "Comparing \'%s\'\n", nextdir->d_name);
#endif
	/* See if this matches. */
	if (strncmp(nextdir->d_name, filename, filenamelen) == 0 &&
		(*filename == '.' || (strcmp(nextdir->d_name, ".") !=
		0 && strcmp(nextdir->d_name, "..") != 0))) {
	    /* Cool, found a match.  Add it to the list.  This makes a
	     * lot more sense to me (Chris) this way... */

	    char *tmp = charalloc(strlen(dirname) +
		strlen(nextdir->d_name) + 1);
	    sprintf(tmp, "%s%s", dirname, nextdir->d_name);

#ifndef DISABLE_OPERATINGDIR
	    /* ...unless the match exists outside the operating
	     * directory, in which case just go to the next match. */
	    if (check_operating_dir(tmp, TRUE))
		skip_match = TRUE;
#endif

	    /* ...or unless the match isn't a directory and allow_files
	     * isn't set, in which case just go to the next match. */
	    if (!allow_files && !is_dir(tmp))
		skip_match = TRUE;

	    free(tmp);

	    if (skip_match)
		continue;

	    matches = (char **)nrealloc(matches, (*num_matches + 1) *
		sizeof(char *));
	    matches[*num_matches] = mallocstrcpy(NULL, nextdir->d_name);
	    ++(*num_matches);
	}
    }

    closedir(dir);
    free(dirname);
    free(filename);

    return matches;
}

/* Do tab completion.  place refers to how much the statusbar cursor
 * position should be advanced.  refresh_func is the function we will
 * call to refresh the edit window. */
char *input_tab(char *buf, bool allow_files, size_t *place, bool
	*lastwastab, void (*refresh_func)(void), bool *list)
{
    size_t num_matches = 0;
    char **matches = NULL;

    assert(buf != NULL && place != NULL && *place <= strlen(buf) && lastwastab != NULL && refresh_func != NULL && list != NULL);

    *list = FALSE;

    /* If the word starts with `~' and there is no slash in the word,
     * then try completing this word as a username. */
    if (*place > 0 && buf[0] == '~') {
	const char *bob = strchr(buf, '/');

	if (bob == NULL || bob >= buf + *place)
	    matches = username_tab_completion(buf, &num_matches,
		*place);
    }

    /* Match against files relative to the current working directory. */
    if (matches == NULL)
	matches = cwd_tab_completion(buf, allow_files, &num_matches,
		*place);

    if (num_matches == 0)
	beep();
    else {
	size_t match, common_len = 0;
	char *mzero;
	const char *lastslash = revstrstr(buf, "/", buf + *place);
	size_t lastslash_len = (lastslash == NULL) ? 0 :
		lastslash - buf + 1;
	char *match1_mb = charalloc(mb_cur_max() + 1);
	char *match2_mb = charalloc(mb_cur_max() + 1);
	int match1_mb_len, match2_mb_len;

	while (TRUE) {
	    for (match = 1; match < num_matches; match++) {
		/* Get the number of single-byte characters that all the
		 * matches have in common. */
		match1_mb_len = parse_mbchar(matches[0] + common_len,
			match1_mb, NULL);
		match2_mb_len = parse_mbchar(matches[match] +
			common_len, match2_mb, NULL);
		match1_mb[match1_mb_len] = '\0';
		match2_mb[match2_mb_len] = '\0';
		if (strcmp(match1_mb, match2_mb) != 0)
		    break;
	    }

	    if (match < num_matches || matches[0][common_len] == '\0')
		break;

	    common_len += parse_mbchar(buf + common_len, NULL, NULL);
	}

	free(match1_mb);
	free(match2_mb);

	mzero = charalloc(lastslash_len + common_len + 1);

	strncpy(mzero, buf, lastslash_len);
	strncpy(mzero + lastslash_len, matches[0], common_len);

	common_len += lastslash_len;
	mzero[common_len] = '\0';

	assert(common_len >= *place);

	if (num_matches == 1 && is_dir(mzero)) {
	    mzero[common_len++] = '/';

	    assert(common_len > *place);
	}

	if (num_matches > 1 && (common_len != *place || !*lastwastab))
	    beep();

	/* If there is more of a match to display on the statusbar, show
	 * it.  We reset lastwastab to FALSE: it requires pressing Tab
	 * twice in succession with no statusbar changes to see a match
	 * list. */
	if (common_len != *place) {
	    size_t buf_len = strlen(buf);

	    *lastwastab = FALSE;
	    buf = charealloc(buf, common_len + buf_len - *place + 1);
	    charmove(buf + common_len, buf + *place, buf_len -
		*place + 1);
	    strncpy(buf, mzero, common_len);
	    *place = common_len;
	} else if (!*lastwastab || num_matches < 2)
	    *lastwastab = TRUE;
	else {
	    int longest_name = 0, columns, editline = 0;

	    /* Now we show a list of the available choices. */
	    assert(num_matches > 1);

	    /* Sort the list. */
	    qsort(matches, num_matches, sizeof(char *), diralphasort);

	    for (match = 0; match < num_matches; match++) {
		common_len = strnlenpt(matches[match], COLS - 1);

		if (common_len > COLS - 1) {
		    longest_name = COLS - 1;
		    break;
		}

		if (common_len > longest_name)
		    longest_name = common_len;
	    }

	    assert(longest_name <= COLS - 1);

	    /* Each column will be (longest_name + 2) columns wide, i.e.
	     * two spaces between columns, except that there will be
	     * only one space after the last column. */
	    columns = (COLS + 1) / (longest_name + 2);

	    /* Blank the edit window, and print the matches out
	     * there. */
	    blank_edit();
	    wmove(edit, 0, 0);

	    /* Disable el cursor. */
	    curs_set(0);

	    for (match = 0; match < num_matches; match++) {
		char *disp;

		wmove(edit, editline, (longest_name + 2) *
			(match % columns));

		if (match % columns == 0 &&
			editline == editwinrows - 1 &&
			num_matches - match > columns) {
		    waddstr(edit, _("(more)"));
		    break;
		}

		disp = display_string(matches[match], 0, longest_name,
			FALSE);
		waddstr(edit, disp);
		free(disp);

		if ((match + 1) % columns == 0)
		    editline++;
	    }

	    wnoutrefresh(edit);
	    *list = TRUE;
	}

	free(mzero);
    }

    free_chararray(matches, num_matches);

    /* Only refresh the edit window if we don't have a list of filename
     * matches on it. */
    if (!*list)
	refresh_func();

    /* Enable el cursor. */
    curs_set(1);

    return buf;
}
#endif /* !DISABLE_TABCOMP */

/* Only print the last part of a path.  Isn't there a shell command for
 * this? */
const char *tail(const char *foo)
{
    const char *tmp = strrchr(foo, '/');

    if (tmp == NULL)
	tmp = foo;
    else
	tmp++;

    return tmp;
}

#if !defined(NANO_TINY) && defined(ENABLE_NANORC)
/* Return $HOME/.nano_history, or NULL if we can't find the home
 * directory.  The string is dynamically allocated, and should be
 * freed. */
char *histfilename(void)
{
    char *nanohist = NULL;

    if (homedir != NULL) {
	size_t homelen = strlen(homedir);

	nanohist = charalloc(homelen + 15);
	strcpy(nanohist, homedir);
	strcpy(nanohist + homelen, "/.nano_history");
    }

    return nanohist;
}

/* Load histories from ~/.nano_history. */
void load_history(void)
{
    char *nanohist = histfilename();

    /* Assume do_rcfile() has reported a missing home directory. */
    if (nanohist != NULL) {
	FILE *hist = fopen(nanohist, "rb");

	if (hist == NULL) {
	    if (errno != ENOENT) {
		/* Don't save history when we quit. */
		UNSET(HISTORYLOG);
		rcfile_error(N_("Error reading %s: %s"), nanohist,
			strerror(errno));
		fprintf(stderr,
			_("\nPress Enter to continue starting nano.\n"));
		while (getchar() != '\n')
		    ;
	    }
	} else {
	    /* Load a history list (first the search history, then the
	     * replace history) from the oldest entry to the newest.
	     * Assume the last history entry is a blank line. */
	    filestruct **history = &search_history;
	    char *line = NULL;
	    size_t buflen = 0;
	    ssize_t read;

	    while ((read = getline(&line, &buflen, hist)) >= 0) {
		if (read > 0 && line[read - 1] == '\n') {
		    read--;
		    line[read] = '\0';
		}
		if (read > 0) {
		    unsunder(line, read);
		    update_history(history, line);
		} else
		    history = &replace_history;
	    }

	    fclose(hist);
	    free(line);
	}
	free(nanohist);
    }
}

/* Write the lines of a history list, starting with the line at h, to
 * the open file at hist.  Return TRUE if the write succeeded, and FALSE
 * otherwise. */
bool writehist(FILE *hist, filestruct *h)
{
    filestruct *p;

    /* Write a history list from the oldest entry to the newest.  Assume
     * the last history entry is a blank line. */
    for (p = h; p != NULL; p = p->next) {
	size_t p_len = strlen(p->data);

	sunder(p->data);

	if (fwrite(p->data, sizeof(char), p_len, hist) < p_len ||
		putc('\n', hist) == EOF)
	    return FALSE;
    }

    return TRUE;
}

/* Save histories to ~/.nano_history. */
void save_history(void)
{
    char *nanohist;

    /* Don't save unchanged or empty histories. */
    if (!history_has_changed() || (searchbot->lineno == 1 &&
	replacebot->lineno == 1))
	return;

    nanohist = histfilename();

    if (nanohist != NULL) {
	FILE *hist = fopen(nanohist, "wb");

	if (hist == NULL)
	    rcfile_error(N_("Error writing %s: %s"), nanohist,
		strerror(errno));
	else {
	    /* Make sure no one else can read from or write to the
	     * history file. */
	    chmod(nanohist, S_IRUSR | S_IWUSR);

	    if (!writehist(hist, searchage) || !writehist(hist,
		replaceage))
		rcfile_error(N_("Error writing %s: %s"), nanohist,
			strerror(errno));

	    fclose(hist);
	}

	free(nanohist);
    }
}
#endif /* !NANO_TINY && ENABLE_NANORC */
