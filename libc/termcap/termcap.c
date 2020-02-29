/* Work-alike for termcap, plus extra features.
   Copyright (C) 1985, 1986, 1993 Free Software Foundation, Inc.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; see the file COPYING.  If not, write to
the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.  */

/* Emacs config.h may rename various library functions such as malloc.  */
#ifdef HAVE_CONFIG_H
#include "config.h"
#else /* not HAVE_CONFIG_H */

#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>

#ifdef __linux__
#undef STDC_HEADERS
#define STDC_HEADERS
#define HAVE_UNISTD_H
#define HAVE_SYS_IOCTL_H
#else	      
#if defined(HAVE_STRING_H) || defined(STDC_HEADERS)
#define bcopy(s, d, n) memcpy ((d), (s), (n))
#endif
#endif

#ifdef HAVE_SYS_IOCTL_H
#include <stdio.h>
#include <sys/ioctl.h>
#endif

#endif /* not HAVE_CONFIG_H */

#ifndef NULL
#define NULL (char *) 0
#endif

/* BUFSIZE is the initial size allocated for the buffer
   for reading the termcap file.
   It is not a limit.
   Make it large normally for speed.
   Make it variable when debugging, so can exercise
   increasing the space dynamically.  */

#ifndef BUFSIZE
#ifdef DEBUG
#define BUFSIZE bufsize

int bufsize = 128;
#else
#define BUFSIZE 2048
#endif
#endif

#ifdef TIOCGWINSZ
#define ADJUST_WIN_EXTENT
#endif

#ifndef emacs
void
termcap_memory_out ()
{
  write (2, "virtual memory exhausted\n", 25);
  exit (1);
}

char *
termcap_xmalloc (size)
     unsigned size;
{
  register char *tem = malloc (size);

  if (!tem)
    termcap_memory_out ();
  return tem;
}

char *
termcap_xrealloc (ptr, size)
     char *ptr;
     unsigned size;
{
  register char *tem = realloc (ptr, size);

  if (!tem)
    termcap_memory_out ();
  return tem;
}
#endif /* not emacs */

/* Looking up capabilities in the entry already found.  */

/* The pointer to the data made by tgetent is left here
   for tgetnum, tgetflag and tgetstr to find.  */
char *termcap_term_entry;

char *termcap_tgetst1 ();

/* Search entry BP for capability CAP.
   Return a pointer to the capability (in BP) if found,
   0 if not found.  */

char *
termcap_find_capability (bp, cap)
     register char *bp, *cap;
{
  for (; *bp; bp++)
    if (bp[0] == ':'
	&& bp[1] == cap[0]
	&& bp[2] == cap[1])
      return &bp[4];
  return NULL;
}

int
tgetnum (cap)
     char *cap;
{
  register char *ptr = termcap_find_capability (termcap_term_entry, cap);
  if (!ptr || ptr[-1] != '#')
    return -1;
  return atoi (ptr);
}

int
tgetflag (cap)
     char *cap;
{
  register char *ptr = termcap_find_capability (termcap_term_entry, cap);
  return ptr && ptr[-1] == ':';
}

/* Look up a string-valued capability CAP.
   If AREA is non-null, it points to a pointer to a block in which
   to store the string.  That pointer is advanced over the space used.
   If AREA is null, space is allocated with `malloc'.  */

char *
tgetstr (cap, area)
     char *cap;
     char **area;
{
  register char *ptr = termcap_find_capability (termcap_term_entry, cap);
  if (!ptr || (ptr[-1] != '=' && ptr[-1] != '~'))
    return NULL;
  return termcap_tgetst1 (ptr, area);
}

/* Table, indexed by a character in range 0100 to 0140 with 0100 subtracted,
   gives meaning of character following \, or a space if no special meaning.
   Eight characters per line within the string.  */

static char esctab[]
  = " \007\010  \033\014 \
      \012 \
  \015 \011 \013 \
        ";

/* PTR points to a string value inside a termcap entry.
   Copy that value, processing \ and ^ abbreviations,
   into the block that *AREA points to,
   or to newly allocated storage if AREA is NULL.
   Return the address to which we copied the value,
   or NULL if PTR is NULL.  */

char *
termcap_tgetst1 (ptr, area)
     char *ptr;
     char **area;
{
  register char *p, *r;
  register int c;
  register int size;
  char *ret;
  register int c1;

  if (!ptr)
    return NULL;

  /* `ret' gets address of where to store the string.  */
  if (!area)
    {
      /* Compute size of block needed (may overestimate).  */
      p = ptr;
      while ((c = *p++) && c != ':' && c != '\n')
	;
      ret = (char *) termcap_xmalloc (p - ptr + 1);
    }
  else
    ret = *area;

  /* Copy the string value, stopping at null or colon.
     Also process ^ and \ abbreviations.  */
  p = ptr;
  r = ret;
  while ((c = *p++) && c != ':' && c != '\n')
    {
      if (c == '^')
	c = *p++ & 037;
      else if (c == '\\')
	{
	  c = *p++;
	  if (c >= '0' && c <= '7')
	    {
	      c -= '0';
	      size = 0;

	      while (++size < 3 && (c1 = *p) >= '0' && c1 <= '7')
		{
		  c *= 8;
		  c += c1 - '0';
		  p++;
		}
	    }
	  else if (c >= 0100 && c < 0200)
	    {
	      c1 = esctab[(c & ~040) - 0100];
	      if (c1 != ' ')
		c = c1;
	    }
	}
      *r++ = c;
    }
  *r = '\0';
  /* Update *AREA.  */
  if (area)
    *area = r + 1;
  return ret;
}

/* Outputting a string with padding.  */

#ifdef __linux__
speed_t ospeed;
#else
short ospeed;
#endif
/* If OSPEED is 0, we use this as the actual baud rate.  */
int tputs_baud_rate;
char PC;

/* Actual baud rate if positive;
   - baud rate / 100 if negative.  */

static short speeds[] =
  {
#ifdef VMS
    0, 50, 75, 110, 134, 150, -3, -6, -12, -18,
    -20, -24, -36, -48, -72, -96, -192
#else /* not VMS */
    0, 50, 75, 110, 135, 150, -2, -3, -6, -12,
    -18, -24, -48, -96, -192, -384
#endif /* not VMS */
  };

void
tputs (str, nlines, outfun)
     register char *str;
     int nlines;
     register int (*outfun) ();
{
  register int padcount = 0;
  register int speed;

#ifdef emacs
  extern baud_rate;
  speed = baud_rate;
#else
  if (ospeed == 0)
    speed = tputs_baud_rate;
  else
    speed = speeds[ospeed];
#endif

  if (!str)
    return;

  while (*str >= '0' && *str <= '9')
    {
      padcount += *str++ - '0';
      padcount *= 10;
    }
  if (*str == '.')
    {
      str++;
      padcount += *str++ - '0';
    }
  if (*str == '*')
    {
      str++;
      padcount *= nlines;
    }
  while (*str)
    (*outfun) (*str++);

  /* padcount is now in units of tenths of msec.  */
  padcount *= speeds[ospeed];
  padcount += 500;
  padcount /= 1000;
  if (speeds[ospeed] < 0)
    padcount = -padcount;
  else
    {
      padcount += 50;
      padcount /= 100;
    }

  while (padcount-- > 0)
    (*outfun) (PC);
}

/* Finding the termcap entry in the termcap data base.  */

struct termcap_buffer
  {
    char *beg;
    int size;
    char *ptr;
    int ateof;
    int full;
  };

/* Forward declarations of static functions.  */

static int scan_file ();
static char *gobble_line ();
static int compare_contin ();
static int name_match ();

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

#ifdef VMS

#include <rmsdef.h>
#include <fab.h>
#include <nam.h>

static int
valid_filename_p (fn)
     char *fn;
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

#else /* !VMS */

#define valid_filename_p(fn) (*(fn) == '/')

#endif /* !VMS */

/* Find the termcap entry data for terminal type NAME
   and store it in the block that BP points to.
   Record its address for future use.

   If BP is null, space is dynamically allocated.

   Return -1 if there is some difficulty accessing the data base
   of terminal types,
   0 if the data base is accessible but the type NAME is not defined
   in it, and some other value otherwise.  */

int
tgetent (bp, name)
     char *bp, *name;
{
  register char *termcap_name;
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

  if (termcap_name && !filep && !strcmp (name, getenv ("TERM")))
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
    termcap_name = "/etc/termcap";
#endif

  /* Here we know we must search a file and termcap_name has its name.  */

  fd = open (termcap_name, 0, 0);
  if (fd < 0)
    return -1;

  buf.size = BUFSIZE;
  /* Add 1 to size to ensure room for terminating null.  */
  buf.beg = (char *) termcap_xmalloc (buf.size + 1);
  term = indirect ? indirect : name;

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

/* Given file open on FD and buffer BUFP,
   scan the file from the beginning until a line is found
   that starts the entry for terminal type STR.
   Return 1 if successful, with that line in BUFP,
   or 0 if no entry is found in the file.  */

static int
scan_file (str, fd, bufp)
     char *str;
     int fd;
     register struct termcap_buffer *bufp;
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

/* Return nonzero if NAME is one of the names specified
   by termcap entry LINE.  */

static int
name_match (line, name)
     char *line, *name;
{
  register char *tem;

  if (!compare_contin (line, name))
    return 1;
  /* This line starts an entry.  Is it the right one?  */
  for (tem = line; *tem && *tem != '\n' && *tem != ':'; tem++)
    if (*tem == '|' && !compare_contin (tem + 1, name))
      return 1;

  return 0;
}

static int
compare_contin (str1, str2)
     register char *str1, *str2;
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
gobble_line (fd, bufp, append_end)
     int fd;
     register struct termcap_buffer *bufp;
     char *append_end;
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

#ifdef TEST

#ifdef NULL
#undef NULL
#endif

#include <stdio.h>

main (argc, argv)
     int argc;
     char **argv;
{
  char *term;
  char *buf;

  term = argv[1];
  printf ("TERM: %s\n", term);

  buf = (char *) tgetent (0, term);
  if ((int) buf <= 0)
    {
      printf ("No entry.\n");
      return 0;
    }

  printf ("Entry: %s\n", buf);

  tprint ("cm");
  tprint ("AL");

  printf ("co: %d\n", tgetnum ("co"));
  printf ("am: %d\n", tgetflag ("am"));
}

tprint (cap)
     char *cap;
{
  char *x = tgetstr (cap, 0);
  register char *y;

  printf ("%s: ", cap);
  if (x)
    {
      for (y = x; *y; y++)
	if (*y <= ' ' || *y == 0177)
	  printf ("\\%0o", *y);
	else
	  putchar (*y);
      free (x);
    }
  else
    printf ("none");
  putchar ('\n');
}

#endif /* TEST */

