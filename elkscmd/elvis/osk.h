/*
 * OS9 stat : @(#)stat.h	1.2 87/19/12
 */
/* @(#)stat.h	6.1 */
/*
 * Structure of the result of stat
 */

#ifndef CLK_TCK
#include <time.h>
#endif

struct	stat
{
	int	st_dev;
	long	st_ino;
	unsigned short	st_mode;
	unsigned short	st_nlink;
	unsigned short	st_uid;
	unsigned short	st_gid;
	int	st_rdev;
	long	st_size;
	time_t	st_atime;
	time_t	st_mtime;
	time_t	st_ctime;
};
