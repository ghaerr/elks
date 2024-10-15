/*  Copyright 1996-2002,2005,2007,2009,2011 Alain Knaff.
 *  This file is part of mtools.
 *
 *  Mtools is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Mtools is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Mtools.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Miscellaneous routines.
 */

#include "sysincludes.h"
#include "msdos.h"
#include "stream.h"
#include "vfat.h"
#include "mtools.h"


void printOom(void)
{
	fprintf(stderr, "Out of memory error");
}

char *get_homedir(void)
{
#ifndef OS_mingw32msvc
	struct passwd *pw;
	uid_t uid;
	char *homedir;
	char *username;
	
	homedir = getenv ("HOME");    
	/* 
	 * first we call getlogin. 
	 * There might be several accounts sharing one uid 
	 */
	if ( homedir )
		return homedir;
	
	pw = 0;
	
	username = getenv("LOGNAME");
	if ( !username )
		username = getlogin();
	if ( username )
		pw = getpwnam( username);
  
	if ( pw == 0 ){
		/* if we can't getlogin, look up the pwent by uid */
		uid = geteuid();
		pw = getpwuid(uid);
	}
	
	/* we might still get no entry */
	if ( pw )
		return pw->pw_dir;
	return 0;
#else
	return getenv("HOME");
#endif
}


static void get_mcwd_file_name(char *file)
{
	char *mcwd_path;
	const char *homedir;

	mcwd_path = getenv("MCWD");
	if (mcwd_path == NULL || *mcwd_path == '\0'){
		homedir= get_homedir();
		if(!homedir)
			homedir="/tmp";
		strncpy(file, homedir, MAXPATHLEN-6);
		file[MAXPATHLEN-6]='\0';
		strcat( file, "/.mcwd");
	} else {
		strncpy(file, mcwd_path, MAXPATHLEN);
		file[MAXPATHLEN]='\0';
	}
}

void unlink_mcwd(void)
{
	char file[MAXPATHLEN+1];
	get_mcwd_file_name(file);
	unlink(file);
}

FILE *open_mcwd(const char *mode)
{
	struct MT_STAT sbuf;
	char file[MAXPATHLEN+1];
	time_t now;
	
	get_mcwd_file_name(file);
	if (*mode == 'r'){
		if (MT_STAT(file, &sbuf) < 0)
			return NULL;
		/*
		 * Ignore the info, if the file is more than 6 hours old
		 */
		getTimeNow(&now);
		if (now - sbuf.st_mtime > 6 * 60 * 60) {
			fprintf(stderr,
				"Warning: \"%s\" is out of date, removing it\n",
				file);
			unlink(file);
			return NULL;
		}
	}
	
	return  fopen(file, mode);
}
	


void *safe_malloc(size_t size)
{
	void *p;

	p = malloc(size);
	if(!p){
		printOom();
		exit(1);
	}
	return p;
}

void print_sector(const char *message, unsigned char *data, int size)
{
	int col;
	int row;

	printf("%s:\n", message);
	
	for(row = 0; row * 16 < size; row++){
		printf("%03x  ", row * 16);
		for(col = 0; col < 16; col++)			
			printf("%02x ", data [row*16+col]);
		for(col = 0; col < 16; col++) {
			if(isprint(data [row*16+col]))
				printf("%c", data [row*16+col]);
			else
				printf(".");
		}
		printf("\n");
	}
}

#if (SIZEOF_TIME_T > SIZEOF_LONG) && defined (HAVE_STRTOLL)
# define STRTOTIME strtoll
#else
# define STRTOTIME strtol
#endif

time_t getTimeNow(time_t *now)
{
	static int haveTime = 0;
	static time_t sharedNow;

	if(!haveTime) {
		const char *source_date_epoch = getenv("SOURCE_DATE_EPOCH");
		if (source_date_epoch) {
			char *endptr;
			errno = 0;
			time_t epoch =
				STRTOTIME(source_date_epoch, &endptr, 10);

			if (endptr == source_date_epoch)
				fprintf(stderr,
					"SOURCE_DATE_EPOCH \"%s\" invalid\n",
					source_date_epoch);
			else if (errno != 0)
				fprintf(stderr,
					"SOURCE_DATE_EPOCH: strtoll: %s: %s\n",
					strerror(errno), source_date_epoch);
			else if (*endptr != '\0')
				fprintf(stderr,
					"SOURCE_DATE_EPOCH has trailing garbage \"%s\"\n",
					endptr);
			else {
				sharedNow = epoch;
				haveTime = 1;
			}
		}
	}
	
	if(!haveTime) {
		time(&sharedNow);
		haveTime = 1;
	}
	if(now)
		*now = sharedNow;
	return sharedNow;
}

/* Convert a string to an offset. The string should be a number,
   optionally followed by S (sectors), K (K-Bytes), M (Megabytes), G
   (Gigabytes) */
off_t str_to_offset(char *str) {
	char s, *endp = NULL;
	off_t ofs;

	ofs = strtol(str, &endp, 0);
	if (ofs <= 0)
		return 0; /* invalid or missing offset */
	s = *endp++;
	if (s) {   /* trailing char, see if it is a size specifier */
		if (s == 's' || s == 'S')       /* sector */
			ofs <<= 9;
		else if (s == 'k' || s == 'K')  /* kb */
			ofs <<= 10;
		else if (s == 'm' || s == 'M')  /* Mb */
			ofs <<= 20;
		else if (s == 'g' || s == 'G')  /* Gb */
			ofs <<= 30;
		else
			return 0;      /* invalid character */
		if (*endp)
			return 0;      /* extra char, invalid */
	}
	return ofs;
}

#if 0

#undef free
#undef malloc

static int total=0;

void myfree(void *ptr)
{
	int *size = ((int *) ptr)-1;
	total -= *size;
	fprintf(stderr, "freeing %d bytes at %p total allocated=%d\n",
		*size, ptr, total);
	free(size);
}

void *mymalloc(size_t size)
{
	int *ptr;
	ptr = (int *)malloc(size+sizeof(int));
	if(!ptr)
		return 0;
	*ptr = size;
	ptr++;
	total += size;
	fprintf(stderr, "allocating %d bytes at %p total allocated=%d\n",
		size, ptr, total);
	return (void *) ptr;
}

void *mycalloc(size_t nmemb, size_t size)
{
	void *ptr = mymalloc(nmemb * size);
	if(!ptr)
		return 0;
	memset(ptr, 0, size);
	return ptr;
}

void *myrealloc(void *ptr, size_t size)
{
	int oldsize = ((int *)ptr) [-1];
	void *new = mymalloc(size);
	if(!new)
		return 0;
	memcpy(new, ptr, oldsize);
	myfree(ptr);
	return new;
}

char *mystrdup(char *src)
{
	char *dest;
	dest = mymalloc(strlen(src)+1);
	if(!dest)
		return 0;
	strcpy(dest, src);
	return dest;
}

#endif
