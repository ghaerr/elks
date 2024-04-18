#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <termcap.h>
#include <unistd.h>
#include <paths.h>

/* Finding the termcap entry in the termcap data base.  */

/* Look up a string-valued capability CAP.
   If AREA is non-null, it points to a pointer to a block in which
   to store the string.  That pointer is advanced over the space used.
   If AREA is null, space is allocated with `malloc'.  */


#ifdef TIOCGWINSZ
#	define ADJUST_WIN_EXTENT
#endif

#if defined(ADJUST_WIN_EXTENT)
#	include <sys/ioctl.h>
#endif

#include "t.h"

/* BUFSIZE is the initial size allocated for the buffer
   for reading the termcap file.
   It is not a limit.
   Make it large normally for speed.
   Make it variable when debugging, so can exercise
   increasing the space dynamically.  */
#ifndef BUFSIZE
#	ifdef DEBUG
#		define BUFSIZE bufsize
static	int bufsize = 128;
#	else
#		define BUFSIZE 2048
#	endif
#endif

/* Make sure that the buffer <- BUFP contains a full line
   of the file open on FD, starting at the place BUFP->ptr
   points to.  Can read more of the file, discard stuff before
   BUFP->ptr, or make the buffer bigger.

   Return the pointer to after the newline ending the line,
   or to the end of the file, if there is no newline to end it.

   Can also merge on continuation lines.  If APPEND_END is
   non-null, it points past the newline of a line that is
   continued; we add another line onto it and regard the whole
   thing as one line.  The caller decides when a line is continued.  */
static char *
gobble_line (int fd, register struct termcap_buffer *bufp, char *append_end)
{
  register char *end;
  register int nread;
  register char *buf = bufp->beg;
  register char *tem;

  if (!append_end)
    append_end = bufp->ptr;

  while (1)
    {
      end = append_end;
      while (*end && *end != '\n') end++;
      if (*end)
        break;
      if (bufp->ateof)
	return buf + bufp->full;
      if (bufp->ptr == buf)
	{
	  if (bufp->full == bufp->size)
	    {
	      bufp->size *= 2;
	      /* Add 1 to size to ensure room for terminating null.  */
	      tem = (char *) termcap_xrealloc (buf, bufp->size + 1);
	      bufp->ptr = (bufp->ptr - buf) + tem;
	      append_end = (append_end - buf) + tem;
	      bufp->beg = buf = tem;
	    }
	}
      else
	{
	  append_end -= bufp->ptr - buf;
	  bcopy (bufp->ptr, buf, bufp->full -= bufp->ptr - buf);
	  bufp->ptr = buf;
	}
      if (!(nread = read (fd, buf + bufp->full, bufp->size - bufp->full)))
	bufp->ateof = 1;
      bufp->full += nread;
      buf[bufp->full] = '\0';
    }
  return end + 1;
}

static int
compare_contin(register const char *str1, register const char *str2)
{
  register int c1, c2;
  while (1)
    {
      c1 = *str1++;
      c2 = *str2++;
      while (c1 == '\\' && *str1 == '\n')
	{
	  str1++;
	  while ((c1 = *str1++) == ' ' || c1 == '\t');
	}
      if (c2 == '\0')
	{
	  /* End of type being looked up.  */
	  if (c1 == '|' || c1 == ':')
	    /* If end of name in data base, we win.  */
	    return 0;
	  else
	    return 1;
        }
      else if (c1 != c2)
	return 1;
    }
}

/* Return nonzero if NAME is one of the names specified
   by termcap entry LINE.  */
static int
name_match(const char *line, const char *name)
{
  register const char *tem;

  if (!compare_contin (line, name))
    return 1;
  /* This line starts an entry.  Is it the right one?  */
  for (tem = line; *tem && *tem != '\n' && *tem != ':'; tem++)
    if (*tem == '|' && !compare_contin (tem + 1, name))
      return 1;

  return 0;
}

/* Given file open on FD and buffer BUFP,
   scan the file from the beginning until a line is found
   that starts the entry for terminal type STR.
   Return 1 if successful, with that line in BUFP,
   or 0 if no entry is found in the file.  */
static int
scan_file(const char * str, int fd, register struct termcap_buffer *bufp)
{
  register char *end;

  bufp->ptr = bufp->beg;
  bufp->full = 0;
  bufp->ateof = 0;
  *bufp->ptr = '\0';

  lseek (fd, 0L, 0);

  while (!bufp->ateof)
    {
      /* Read a line into the buffer.  */
      end = NULL;
      do
	{
	  /* if it is continued, append another line to it,
	     until a non-continued line ends.  */
	  end = gobble_line (fd, bufp, end);
	}
      while (!bufp->ateof && end[-2] == '\\');

      if (*bufp->ptr != '#'
	  && name_match (bufp->ptr, str))
	return 1;

      /* Discard the line just processed.  */
      bufp->ptr = end;
    }
  return 0;
}


#ifdef VMS
#	include <rmsdef.h>
#	include <fab.h>
#	include <nam.h>

static int
valid_filename_p(const char *fn)
{
  struct FAB fab = cc$rms_fab;
  struct NAM nam = cc$rms_nam;
  char esa[NAM$C_MAXRSS];

  fab.fab$l_fna = fn;
  fab.fab$b_fns = strlen(fn);
  fab.fab$l_nam = &nam;
  fab.fab$l_fop = FAB$M_NAM;

  nam.nam$l_esa = esa;
  nam.nam$b_ess = sizeof esa;

  return SYS$PARSE(&fab, 0, 0) == RMS$_NORMAL;
}
#else
#	define valid_filename_p(fn) (*(fn) == '/')
#endif

#ifdef ADJUST_WIN_EXTENT
#ifdef TIOCGWINSZ
static int
get_win_extent(li, co)
int *li, *co;
{
  struct winsize ws;

  /* Some TIOCGWINSZ may be broken. Make sure ws.ws_row and
   * ws.ws_col are not zero.
   */
  if (ioctl(0, TIOCGWINSZ, &ws) != 0 || !ws.ws_row || !ws.ws_col)
    return -1;
  *li = ws.ws_row;
  *co = ws.ws_col;
  return 0;
}
#endif /* TIOCGWINSZ */

static int
adjust_win_extent(bpp, howalloc, li, co)
char **bpp;
int howalloc;  /* 0 must do in place, 1 must use malloc, 2 must use realloc */
int li, co;
{
  int licolen, o_len, t, colon;
  char *licobuf, *s;

  if (li < 0 || co < 0)
    return 0;
  for (s = *bpp, colon = -1; *s; ++s)
    if (*s == ':' && colon < 0)
      colon = s - *bpp;
  o_len = s - *bpp;
  licolen = 11;
  for (t = li; (t /= 10) > 0; ++licolen);
  for (t = co; (t /= 10) > 0; ++licolen);

  licobuf = termcap_xmalloc(licolen + 1);
  sprintf(licobuf, ":li#%d:co#%d:", li, co);

  if (howalloc == 0)
  {
    bcopy(*bpp + colon, *bpp + colon + licolen, o_len - colon + 1);
    bcopy(licobuf, *bpp + colon, licolen);
  }
  else if (howalloc == 1)
  {
    char *newbp;

    newbp = termcap_xmalloc(o_len + licolen + 1);
    bcopy(*bpp, newbp, colon);
    bcopy(licobuf, newbp + colon, licolen);
    strcpy(newbp + colon + licolen, *bpp + colon);
    *bpp = newbp;
  }
  else /* (howalloc == 2) */
  {
    char *newbp;

    newbp = termcap_xrealloc(*bpp, o_len + licolen + 1);
    bcopy(newbp + colon, newbp + colon + licolen, o_len - colon + 1);
    bcopy(licobuf, newbp + colon, licolen);
    *bpp = newbp;
  }

  free(licobuf);
  return 1;
}
#endif /* ADJUST_WIN_EXTENT */

/* Find the termcap entry data for terminal type NAME
   and store it in the block that BP points to.
   Record its address for future use.

   If BP is null, space is dynamically allocated.

   Return -1 if there is some difficulty accessing the data base
   of terminal types,
   0 if the data base is accessible but the type NAME is not defined
   in it, and some other value otherwise.  */

int
tgetent(char *bp, const char *name)
{
  char *termcap_name, *term_name;
  register int fd;
  struct termcap_buffer buf;
  register char *bp1;
  char *bp2;
  char *term;
  int malloc_size = 0;
  register int c;
  char *tcenv;			/* TERMCAP value, if it contains :tc=.  */
  char *indirect = NULL;	/* Terminal type in :tc= in TERMCAP value.  */
  int filep;
#ifdef ADJUST_WIN_EXTENT
  int li, co;			/* #lines and columns on this tty */
  
  if (get_win_extent(&li, &co) != 0)
     li = co = -1;
#endif /* ADJUST_WIN_EXTENT */

  termcap_name = getenv ("TERMCAP");
  if (termcap_name && *termcap_name == '\0')
    termcap_name = NULL;

  filep = termcap_name && valid_filename_p (termcap_name);

  /* If termcap_name is non-null and starts with / (in the un*x case, that is),
     it is a file name to use instead of /etc/termcap.
     If it is non-null and does not start with /,
     it is the entry itself, but only if
     the name the caller requested matches the TERM variable.  */

  if (termcap_name && !filep && (term_name = getenv("TERM")) && !strcmp (name, term_name))
    {
      indirect = termcap_tgetst1 (termcap_find_capability (termcap_name, "tc"), (char **) 0);
      if (!indirect)
	{
	  if (!bp)
	  {
	      bp = termcap_name;
#ifdef ADJUST_WIN_EXTENT
	      if (adjust_win_extent(&bp, 1, li, co))
		malloc_size = 1;	/* force return of bp */
#endif /* ADJUST_WIN_EXTENT */
	  }
	  else
	  {
	    strcpy (bp, termcap_name);
#ifdef ADJUST_WIN_EXTENT
	    adjust_win_extent(&bp, 0, li, co);
#endif /* ADJUST_WIN_EXTENT */
	  }
	  goto ret;
	}
      else
	{			/* It has tc=.  Need to read /etc/termcap.  */
	  tcenv = termcap_name;
 	  termcap_name = NULL;
	}
    }

  if (!termcap_name || !filep)
#ifdef VMS
    termcap_name = "emacs_library:[etc]termcap.dat";
#else
    termcap_name = _PATH_TERMCAP;
#endif

  /* Here we know we must search a file and termcap_name has its name.  */

  fd = open (termcap_name, 0, 0);
  if (fd < 0)
    return -1;

  buf.size = BUFSIZE;
  /* Add 1 to size to ensure room for terminating null.  */
  buf.beg = (char *) termcap_xmalloc (buf.size + 1);
  term = indirect ? indirect : (char *)name;

  if (!bp)
    {
      malloc_size = indirect ? strlen (tcenv) + 1 : buf.size;
      bp = (char *) termcap_xmalloc (malloc_size);
    }
  bp1 = bp;

  if (indirect)
    /* Copy the data from the environment variable.  */
    {
      strcpy (bp, tcenv);
      bp1 += strlen (tcenv);
    }

  while (term)
    {
      /* Scan the file, reading it via buf, till find start of main entry.  */
      if (scan_file (term, fd, &buf) == 0)
	{
	  close (fd);
	  free (buf.beg);
	  if (malloc_size)
	    free (bp);
	  return 0;
	}

      /* Free old `term' if appropriate.  */
      if (term != name)
	free (term);

      /* If BP is malloc'd by us, make sure it is big enough.  */
      if (malloc_size)
	{
	  malloc_size = bp1 - bp + buf.size;
	  termcap_name = (char *) termcap_xrealloc (bp, malloc_size);
	  bp1 += termcap_name - bp;
	  bp = termcap_name;
	}

      bp2 = bp1;

      /* Copy the line of the entry from buf into bp.  */
      termcap_name = buf.ptr;
      while ((*bp1++ = c = *termcap_name++) && c != '\n')
	/* Drop out any \ newline sequence.  */
	if (c == '\\' && *termcap_name == '\n')
	  {
	    bp1--;
	    termcap_name++;
	  }
      *bp1 = '\0';

      /* Does this entry refer to another terminal type's entry?
	 If something is found, copy it into heap and null-terminate it.  */
      term = termcap_tgetst1 (termcap_find_capability (bp2, "tc"), (char **) 0);
    }

  close (fd);
  free (buf.beg);

  if (malloc_size)
    bp = (char *) termcap_xrealloc (bp, bp1 - bp + 1);
#ifdef ADJUST_WIN_EXTENT
  adjust_win_extent(&bp, malloc_size ? 2 : 0, li, co);
#endif /* ADJUST_WIN_EXTENT */

 ret:
  termcap_term_entry = bp;
  if (malloc_size)
    return (int) bp;
  return 1;
}
