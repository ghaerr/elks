#ifndef	__DIRENT_H
#define	__DIRENT_H

#include <features.h>
#include <sys/types.h>
#include __SYSINC__(dirent.h)

/* Directory stream type from opendir().  */
typedef struct {
    int dd_fd;                  /* file descriptor */
    int dd_loc;                 /* offset in buffer UNUSED */
    int dd_size;                /* # of valid entries in buffer UNUSED */
    struct dirent *dd_buf;      /* -> directory buffer */
} DIR;

DIR *opendir (const char *dname);
int closedir(DIR * dirp);
struct dirent *readdir(DIR * dirp);
int _readdir(int fd, struct dirent *buf, int count);    /* syscall */
void rewinddir(DIR * dirp);
void seekdir(DIR * dirp, off_t pos);
off_t telldir(DIR * dirp);

/* Scan the directory DIR, calling SELECT on each directory entry.
   Entries for which SELECT returns nonzero are individually malloc'd,
   sorted using qsort with CMP, and collected in a malloc'd array in
   *NAMELIST.  Returns the number of entries selected, or -1 on error.  */
typedef int(*__dir_select_fn_t) (const struct dirent *);

typedef int (*__dir_compar_fn_t) (
                const struct dirent * const *,
                const struct dirent * const *
                );

int scandir(const char *__dir,
             struct dirent ***__namelist,
             __dir_select_fn_t __select,
             __dir_compar_fn_t __compar);

#endif /* dirent.h  */
