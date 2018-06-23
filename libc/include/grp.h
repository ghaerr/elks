#ifndef	__GRP_H
#define	__GRP_H

#include <sys/types.h>
#include <features.h>
#include <stdio.h>

/* The group structure */
struct group
{
  char *gr_name;		/* Group name.	*/
  char *gr_passwd;		/* Password.	*/
  gid_t gr_gid;			/* Group ID.	*/
  char **gr_mem;		/* Member list.	*/
};

extern void setgrent __P ((void));
extern void endgrent __P ((void));
extern struct group * getgrent __P ((void));

extern struct group * getgrgid __P ((__const gid_t gid));
extern struct group * getgrnam __P ((__const char * name));

extern struct group * fgetgrent __P ((FILE * file));

extern int setgroups __P ((size_t n, __const gid_t * groups));
extern int initgroups __P ((__const char * user, gid_t gid));


#ifdef __LIBC__
extern struct group * __getgrent __P ((int grp_fd));
#endif

#endif /* _GRP_H */



