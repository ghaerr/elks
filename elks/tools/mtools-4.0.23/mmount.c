/*  Copyright 1994,1996-2002,2005-2007,2009 Alain Knaff.
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
 * Mount an MSDOS disk
 *
 * written by:
 *
 * Alain L. Knaff			
 * alain@knaff.lu
 *
 */

#include "sysincludes.h"
#include "msdos.h"
#include "mtools.h"

#ifdef OS_linux
#include <sys/wait.h>
#include "mainloop.h"
#include "fs.h"

void mmount(int argc, char **argv, int type UNUSEDP) NORETURN;
void mmount(int argc, char **argv, int type UNUSEDP)
{
	char drive;
	int pid;
	int status;
	struct device dev;
	char name[EXPAND_BUF];
	int media;
	union bootsector boot;
	Stream_t *Stream;
	
	if (argc<2 || !argv[1][0]  || argv[1][1] != ':' || argv[1][2]){
		fprintf(stderr,"Usage: %s -V drive:\n", argv[0]);
		exit(1);
	}
	drive = ch_toupper(argv[1][0]);
	Stream= find_device(drive, O_RDONLY, &dev, &boot, name, &media, 0, NULL);
	if(!Stream)
		exit(1);
	FREE(&Stream);

	destroy_privs();

	if ( dev.partition ) {
		char part_name[4];
		sprintf(part_name, "%d", dev.partition %1000);
		strcat(name, part_name); 
	}

	/* and finally mount it */
	switch((pid=fork())){
	case -1:
		fprintf(stderr,"fork failed\n");
		exit(1);
	case 0:
		close(2);
		open("/dev/null", O_RDWR | O_BINARY | O_LARGEFILE);
		argv[1] = strdup("mount");
		if ( argc > 2 )
			execvp("mount", argv + 1 );
		else
			execlp("mount", "mount", name, NULL);
		perror("exec mount");
		exit(1);
	default:
		while ( wait(&status) != pid );
	}	
	if ( WEXITSTATUS(status) == 0 )
		exit(0);
	argv[0] = strdup("mount");
	argv[1] = strdup("-r");
	if(!argv[0] || !argv[1]){
		printOom();
		exit(1);
	}
	if ( argc > 2 )
		execvp("mount", argv);
	else
		execlp("mount", "mount","-r", name, NULL);
	exit(1);
}

#else /* linux */

#include "msdos.h"

void mmount(int argc, char **argv, int type)
{
  fprintf(stderr,"This command is only available for LINUX \n");
  exit(1);
}
#endif /* linux */

