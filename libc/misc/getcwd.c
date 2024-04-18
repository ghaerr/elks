#include <errno.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <stdio.h>

/*
 * These functions find the absolute path to the current working directory.
 *
 * They don't use malloc or large amounts of stack space.
 */

static char * path_buf;
static size_t path_size;
static dev_t root_dev;
static ino_t root_ino;
static struct stat st;

/* routine to find the step back down */
static char *
search_dir(dev_t this_dev, ino_t this_ino)
{
   DIR * dp;
   struct dirent * d;
   char * ptr;
   size_t slen;
   int slow_search = 0;

   if( stat(path_buf, &st) < 0 ) return NULL;
   if( this_dev != st.st_dev ) slow_search = 1;

   slen = strlen(path_buf);
   ptr = path_buf + slen -1;
   if( *ptr != '/' )
   {
      if( slen + 2 > path_size )
      {
        errno = ERANGE;
        return NULL;
      }
      strcpy(++ptr, "/");
      slen++;
   }
   slen++;

   dp = opendir(path_buf);
   if( dp == NULL ) return NULL;

   while( (d=readdir(dp)) != NULL )
   {
      if( slow_search || this_ino == d->d_ino )
      {
         if( slen + strlen(d->d_name) > path_size )
         {
            errno = ERANGE;
            return NULL;
         }
         strcpy(ptr+1, d->d_name);
         if( stat(path_buf, &st) < 0 )
            continue;
         if( st.st_ino == this_ino && st.st_dev == this_dev )
         {
            closedir(dp);
            return path_buf;
         }
      }
   }

   closedir(dp);
   errno = ENOENT;
   return NULL;
}

/* routine to go up the tree */
static char *
recurser(void)
{
   dev_t this_dev;
   ino_t this_ino;

   if( stat(path_buf, &st) < 0 ) return NULL;
   this_dev = st.st_dev;
   this_ino = st.st_ino;
   if( this_dev == root_dev && this_ino == root_ino )
   {
      strcpy(path_buf, "/");
      return path_buf;
   }
   if( strlen(path_buf) + 4 > path_size )
   {
      errno = ERANGE;
      return NULL;
   }
   strcat(path_buf, "/..");
   if( recurser() == NULL ) return NULL;

   return search_dir(this_dev, this_ino);
}

char *
getcwd(char *buf, int size)
{
   path_buf = buf;
   path_size = size;

   if( size < 3 )
   {
      errno = ERANGE;
      return NULL;
   }
   strcpy(path_buf, ".");

   if( stat("/", &st) < 0 ) return NULL;

   root_dev = st.st_dev;
   root_ino = st.st_ino;

   return recurser();
}
