/*  Copyright 1997,1999,2001-2004,2007,2009 Alain Knaff.
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
 * Do filename expansion with the shell.
 */

#include "sysincludes.h"
#include "mtools.h"

void precmd(struct device *dev)
{
#ifndef OS_mingw32msvc
	int status;
	pid_t pid;

	if(!dev || !dev->precmd)
		return;
	
	switch((pid=fork())){
		case -1:
			perror("Could not fork");
			exit(1);
		case 0: /* the son */
			execl("/bin/sh", "sh", "-c", dev->precmd, (char *)NULL);
			break;
		default:
			wait(&status);
			break;
	}
#endif
}
		
