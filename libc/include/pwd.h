#ifndef	__PWD_H
#define	__PWD_H

#include <sys/types.h>
#include <stdio.h>

/* The passwd structure.  */
struct passwd
{
  char *pw_name;		/* Username.  */
  char *pw_passwd;		/* Password.  */
  uid_t pw_uid;			/* User ID.  */
  gid_t pw_gid;			/* Group ID.  */
  char *pw_gecos;		/* Real name.  */
  char *pw_dir;			/* Home directory.  */
  char *pw_shell;		/* Shell program.  */
};

void setpwent(void);
void endpwent(void);
struct passwd * getpwent(void);
struct passwd * getpwuid(uid_t);
struct passwd * getpwnam(const char *);
int putpwent (const struct passwd * p, FILE * stream);

#ifdef __LIBC__
struct passwd * __getpwent(int passwd_fd);
#endif

char *getpass(char *prompt);

#endif /* __PWD_H */
