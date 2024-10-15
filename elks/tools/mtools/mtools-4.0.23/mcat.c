/*  Copyright 1999-2003,2007,2009 Alain Knaff.
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
 * mcat.c
 * Same thing as cat /dev/fd0 or cat file >/dev/fd0
 * Something, that isn't possible with floppyd anymore.
 */

#include "sysincludes.h"
#include "msdos.h"
#include "mtools.h"
#include "mainloop.h"
#include "fsP.h"
#include "xdf_io.h"
#include "floppyd_io.h"
#include "plain_io.h"

static void usage(void) NORETURN;
static void usage(void) 
{
	fprintf(stderr, "Mtools version %s, dated %s\n", 
		mversion, mdate);
	fprintf(stderr, "Usage: mcat [-V] [-w] device\n");
	fprintf(stderr, "       -w write on device else read\n");
	exit(1);
}

#ifdef __CYGWIN__
#define BUF_SIZE 512u
#else
#define BUF_SIZE 16000u
#endif

static size_t bufLen(size_t blocksize, mt_size_t totalSize, mt_off_t address)
{
	if(totalSize == 0)
		return blocksize;
	if(address + blocksize > totalSize)
		return totalSize - address;
	return blocksize;
}

void mcat(int argc, char **argv, int type UNUSEDP) NORETURN;
void mcat(int argc, char **argv, int type UNUSEDP)
{
	struct device *dev;
	struct device out_dev;
	char drive, name[EXPAND_BUF];
        char errmsg[200];
        Stream_t *Stream;
	char buf[BUF_SIZE];

	mt_off_t address = 0;

	char mode = O_RDONLY;
	int optindex = 1;
	size_t len;

	noPrivileges = 1;

	if (argc < 2) {
		usage();
	}

	if (argv[1][0] == '-') {
		if (argv[1][1] != 'w') {
			usage();
		}
		mode = O_WRONLY;
		optindex++;
	}

	if (argc - optindex < 1)
		usage();


	if (!argv[optindex][0] || argv[optindex][1] != ':' 
	    || argv[optindex][2]) {
		usage();
	}

        drive = ch_toupper(argv[optindex][0]);

        /* check out a drive whose letter and parameters match */       
        sprintf(errmsg, "Drive '%c:' not supported", drive);    
        Stream = NULL;
        for (dev=devices; dev->name; dev++) {
                FREE(&Stream);
                if (dev->drive != drive)
                        continue;
                out_dev = *dev;
                expand(dev->name,name);
#ifdef USING_NEW_VOLD
                strcpy(name, getVoldName(dev, name));
#endif

                Stream = 0;
#ifdef USE_XDF
                Stream = XdfOpen(&out_dev, name, mode, errmsg, 0);
				if(Stream)
                        out_dev.use_2m = 0x7f;

#endif

#ifdef USE_FLOPPYD
                if(!Stream)
                        Stream = FloppydOpen(&out_dev, name, 
					     mode, errmsg, NULL);
#endif


                if (!Stream)
                        Stream = SimpleFileOpen(&out_dev, dev, name, mode,
						errmsg, 0, 1, 0);

                if( !Stream)
                        continue;
                break;
        }

        /* print error msg if needed */ 
        if ( dev->drive == 0 ){
                FREE(&Stream);
                fprintf(stderr,"%s\n",errmsg);
                exit(1);
        }


	if (mode == O_WRONLY) {
		mt_size_t size=0;
		size = out_dev.sectors * out_dev.heads * out_dev.tracks;
		size *= 512;
		while ((len = fread(buf, 1,
				    bufLen(BUF_SIZE, size, address),
				    stdin)) > 0) {			
			int r = WRITES(Stream, buf, address, len);
			fprintf(stderr, "Wrote to %d\n", (int) address);
			if(r < 0)
				break;
			address += len;
		}
	} else {
		while ((len = READS(Stream, buf, address, BUF_SIZE)) > 0) {
			fwrite(buf, 1, len, stdout);
			address += len;
		}
	}

	FREE(&Stream);
	exit(0);
}
