#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <paths.h>

static void
tryrun(const char *pname, char **argv, char **envp)
{
   struct stat st;

   if( stat(pname, &st) < 0 ) return;
   if( !S_ISREG(st.st_mode) ) return;

   execve(pname, argv, envp);
}

int
execvpe(const char *fname, char **argv, char **envp)
{
   char *path;
   char *pname = (char *)fname;
   int besterr = ENOENT;
   int flen, plen;
   char * bp = sbrk(0);

   if( !strchr(fname, '/') && (path = getenv("PATH")) != 0 )
   {
      flen = strlen(fname)+2;

      for(;path;)
      {
         if( *path == ':' || *path == '\0' )
	 {
	    tryrun(fname, argv, envp);
	    if( errno == EACCES ) besterr = EACCES;
	    if( errno == ENOMEM || errno == ENOEXEC ) goto out;
	    if( *path ) path++; else break;
	 }
	 else
	 {
	    char * p = strchr(path, ':');
	    if(p) *p = '\0';
	    plen = strlen(path);
	    pname = sbrk(plen+flen);
	    if ((int)pname == -1) {
		errno = ENOMEM;
		goto out;
	    }

	    strcpy(pname, path);
	    strcat(pname, "/");
	    strcat(pname, fname);

	    tryrun(pname, argv, envp);
	    if( errno == EACCES ) besterr = EACCES;
	    if( errno == ENOMEM || errno == E2BIG || errno == ENOEXEC ) goto out;

	    brk((char *)pname);
	    pname = (char *)fname;
	    if(p) *p++ = ':';
	    path=p;
	 }
      }
   }

   tryrun(pname, argv, envp);
out:
   brk(bp);
   if( errno == ENOENT || errno == 0 ) errno = besterr;
   return -1;
}
