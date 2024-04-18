#ifndef	__GRP_H
#define	__GRP_H

#include <sys/types.h>

/* The group structure */
struct group
{
  char *gr_name;		/* Group name.	*/
  char *gr_passwd;		/* Password.	*/
  gid_t gr_gid;			/* Group ID.	*/
  char **gr_mem;		/* Member list.	*/
};

void setgrent(void);
void endgrent(void);
struct group * getgrent(void);
struct group * getgrgid(const gid_t gid);
struct group * getgrnam(const char * name);

#ifdef __LIBC__
struct group * __getgrent(int grp_fd);
#endif

#endif /* __GRP_H */
