#ifndef	__PWD_H
#define	__PWD_H

#include <sys/types.h>
#include <features.h>
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


extern void setpwent __P ((void));
extern void endpwent __P ((void));
extern struct passwd * getpwent __P ((void));

int putpwent (const struct passwd * p, FILE * stream);
extern int getpw __P ((uid_t uid, char *buf));

extern struct passwd * fgetpwent __P ((FILE * file));

extern struct passwd * getpwuid __P ((__const uid_t));
extern struct passwd * getpwnam __P ((__const char *));

#ifdef __LIBC__
extern struct passwd * __getpwent __P ((__const int passwd_fd));
#endif

extern char *getpass(char *prompt);

#endif /* pwd.h  */



