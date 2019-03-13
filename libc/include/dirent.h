
#ifndef	__DIRENT_H
#define	__DIRENT_H

#include <features.h>
#include <sys/types.h>

#ifndef	MAXNAMLEN
#define	MAXNAMLEN	255
#endif

/* Directory stream type.  */
typedef struct {
  int dd_fd;			/* file descriptor */
  int dd_loc;			/* offset in buffer */
  int dd_size;			/* # of valid entries in buffer */
  struct dirent *dd_buf;	/* -> directory buffer */
} DIR;				/* stream data from opendir() */

typedef int (*__dir_select_fn_t) __P ((__const struct dirent *));

typedef int (*__dir_compar_fn_t) __P ((
                __const struct dirent * __const *,
                __const struct dirent * __const *
                ));

struct dirent {
	long		d_ino;
	off_t		d_off;
	unsigned short	d_reclen;
	char		d_name[MAXNAMLEN+1];
};

extern DIR *opendir __P ((__const char *__name));
extern int closedir __P ((DIR * __dirp));
extern struct dirent *readdir __P ((DIR * __dirp));
extern void rewinddir __P ((DIR * __dirp));

extern void seekdir __P ((DIR * __dirp, off_t __pos));
extern off_t telldir __P ((DIR * __dirp));

/* Scan the directory DIR, calling SELECT on each directory entry.
   Entries for which SELECT returns nonzero are individually malloc'd,
   sorted using qsort with CMP, and collected in a malloc'd array in
   *NAMELIST.  Returns the number of entries selected, or -1 on error.  */
extern int scandir __P ((__const char *__dir,
			 struct dirent ***__namelist,
			 __dir_select_fn_t __select,
			 __dir_compar_fn_t __compar));

#endif /* dirent.h  */

