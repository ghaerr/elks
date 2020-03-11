/* utent.c <ndf@linux.mit.edu> */
/* Let it be known that this is very possibly the worst standard ever.  HP-UX
   does one thing, someone else does another, linux another... If anyone
   actually has the standard, please send it to me.

   Note that because of the way this stupid stupid standard works, you
   have to call endutent() to close the file even if you've not called
   setutent -- getutid and family use the same file descriptor. */

#include <unistd.h>
#include <fcntl.h>
#include <paths.h>
#include <errno.h>
#include <string.h>
#include <utmp.h>

static const char * ut_name=_PATH_UTMP;

static int ut_fd=-1;

struct utmp *
__getutent(int utmp_fd)
{
  static struct utmp utmp;
  if (read(utmp_fd, (char *) &utmp, sizeof(struct utmp))!=sizeof(struct utmp))
    return NULL;
  return &utmp;
}

void
setutent(void)
{
  if (ut_fd!=-1)
    close(ut_fd);
  if (((ut_fd=open(ut_name, O_RDWR))<0) && ((ut_fd=open(ut_name, O_RDONLY))<0))
    {
	/* I don't really think this should output error messages */
	/* The accepted way to disable logging is to remove the utmp file */
/*      perror("setutent: Can't open utmp file"); */
      ut_fd=-1;
    }
}

void
endutent(void)
{
  if (ut_fd!=-1)
    close(ut_fd);
  ut_fd=-1;
}

struct utmp *
getutent(void)
{
  if (ut_fd==-1)
    setutent();
  if (ut_fd==-1)
    return NULL;
  return __getutent(ut_fd);
}

struct utmp *
getutid(struct utmp * utmp_entry)
{
  struct utmp * utmp;

  if (ut_fd==-1)
    setutent();
  if (ut_fd==-1)
    return NULL;

  while ((utmp=__getutent(ut_fd))!=NULL)
    {
      if ((utmp_entry->ut_type==RUN_LVL   ||
	   utmp_entry->ut_type==BOOT_TIME ||
	   utmp_entry->ut_type==NEW_TIME  ||
	   utmp_entry->ut_type==OLD_TIME) &&
	  utmp->ut_type==utmp_entry->ut_type)
	return utmp;
      if ((utmp_entry->ut_type==INIT_PROCESS ||
	   utmp_entry->ut_type==DEAD_PROCESS ||
	   utmp_entry->ut_type==LOGIN_PROCESS ||
	   utmp_entry->ut_type==USER_PROCESS) &&
	  !strncmp(utmp->ut_id, utmp_entry->ut_id, 2))
	return utmp;
    }

  return NULL;
}

struct utmp *
getutline(struct utmp * utmp_entry)
{
  struct utmp * utmp;

  if (ut_fd==-1)
    setutent();
  if (ut_fd==-1)
    return NULL;

#if 0 /* This is driving me nuts.  It's not an implementation problem -
	 it's a matter of how things _SHOULD_ behave.  Groan. */
  lseek(ut_fd, SEEK_CUR, -sizeof(struct utmp));
#endif

  while ((utmp=__getutent(ut_fd))!=NULL)
    {
      if ((utmp->ut_type==USER_PROCESS  ||
	   utmp->ut_type==LOGIN_PROCESS) &&
	  !strcmp(utmp->ut_line, utmp_entry->ut_line))
	return utmp;
    }

  return NULL;
}

struct utmp *
pututline(struct utmp * utmp_entry)
{
  struct utmp * ut;

  if (ut_fd==-1)
    setutent();
  if (ut_fd==-1)
    return NULL;

  /* Ignore the return value.  That way, if they've already positioned
     the file pointer where they want it, everything will work out. */
/*  (void) lseek(ut_fd, (off_t) -sizeof(struct utmp), SEEK_CUR); */
  /* getutid relies on the file having been rewound if we are to find the
     entry. */
  (void) lseek(ut_fd, (off_t) 0, SEEK_SET);

  if ((ut=getutid(utmp_entry))!=NULL)
    {
      lseek(ut_fd, (off_t) -sizeof(struct utmp), SEEK_CUR);
      if (write(ut_fd, (char *) utmp_entry, sizeof(struct utmp))
	  != sizeof(struct utmp)) {
	return NULL;
      }
    }
  else
    {
      lseek(ut_fd, (off_t) 0, SEEK_END);
      if (write(ut_fd, (char *) utmp_entry, sizeof(struct utmp))
	  != sizeof(struct utmp)) {
	return NULL;
      }
    }

  return utmp_entry;
}

void
utmpname(const char * new_ut_name)
{
  if (new_ut_name!=NULL)
    ut_name=new_ut_name;

  if (ut_fd!=-1)
    close(ut_fd);
}


