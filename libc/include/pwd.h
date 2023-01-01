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


void setpwent(void);
void endpwent(void);
struct passwd * getpwent(void);

int putpwent (const struct passwd * p, FILE * stream);
int getpw(uid_t uid, char *buf);

struct passwd * fgetpwent(FILE * file);

struct passwd * getpwuid(const uid_t);
struct passwd * getpwnam(const char *);

#ifdef __LIBC__
struct passwd * __getpwent(const int passwd_fd);
#endif

char *getpass(char *prompt);

#endif /* pwd.h  */



