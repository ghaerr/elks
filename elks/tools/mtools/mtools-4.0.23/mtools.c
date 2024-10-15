/*  Copyright 1996-2004,2007-2010 Alain Knaff.
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
 */

#include "sysincludes.h"
#include "msdos.h"
#include "mtools.h"
#include "partition.h"
#include "vfat.h"

const char *progname;

static const struct dispatch {
	const char *cmd;
	void (*fn)(int, char **, int);
	int type;
} dispatch[] = {
	{"mattrib",mattrib, 0},
	{"mbadblocks",mbadblocks, 0},
	{"mcat",mcat, 0},
	{"mcd",mcd, 0},
	{"mclasserase",mclasserase, 0},
	{"mcopy",mcopy, 0},
	{"mdel",mdel, 0},
	{"mdeltree",mdel, 2},
	{"mdir",mdir, 0},
	{"mdoctorfat",mdoctorfat, 0},
	{"mdu",mdu, 0},
	{"mformat",mformat, 0},
	{"minfo", minfo, 0},
	{"mlabel",mlabel, 0},
	{"mmd",mmd, 0},
	{"mmount",mmount, 0},
	{"mpartition",mpartition, 0},
	{"mrd",mdel, 1},
	{"mread",mcopy, 0},
	{"mmove",mmove, 0},
	{"mren",mmove, 1},
	{"mshowfat", mshowfat, 0},
	{"mshortname", mshortname, 0},
	{"mtoolstest", mtoolstest, 0},
	{"mtype",mcopy, 1},
	{"mwrite",mcopy, 0},
	{"mzip", mzip, 0}
};
#define NDISPATCH (sizeof dispatch / sizeof dispatch[0])

int main(int argc,char **argv)
{
	unsigned int i;
	const char *name;

#ifdef HAVE_SETLOCALE
	char *locale;
	locale=setlocale(LC_ALL, "");
	if(locale == NULL || !strcmp(locale, "C"))
		setlocale(LC_ALL, "en_US");
#endif

	init_privs();
#ifdef __EMX__
	_wildcard(&argc,&argv);
#endif

/*#define PRIV_TEST*/

#ifdef PRIV_TEST
	{ 
		int euid;
		char command[100];
	
		printf("INIT: %d %d\n", getuid(), geteuid());
		drop_privs();
		printf("DROP: %d %d\n", getuid(), geteuid());
		reclaim_privs();
		printf("RECLAIM: %d %d\n", getuid(), geteuid());
		euid = geteuid();
		if(argc & 1) {
			drop_privs();
			printf("DROP: %d %d\n", getuid(), geteuid());
		}
		if(!((argc-1) & 2)) {
			destroy_privs();
			printf("DESTROY: %d %d\n", getuid(), geteuid());
		}
		sprintf(command, "a.out %d", euid);
		system(command);
		return 1;
	}
#endif


#ifdef __EMX__
       _wildcard(&argc,&argv);
#endif 


	/* check whether the compiler lays out structures in a sane way */
	if(sizeof(struct partition) != 16 ||
	   sizeof(struct directory) != 32 ||
	   sizeof(struct vfat_subentry) !=32) {
		fprintf(stderr,"Mtools has not been correctly compiled\n");
		fprintf(stderr,"Recompile it using a more recent compiler\n");
		return 137;
	}

#ifdef __EMX__
       argv[0] = _getname(argv[0]); _remext(argv[0]); name = argv[0];
#else
#ifdef OS_mingw32msvc
	_stripexe(argv[0]);
#endif
	name = _basename(argv[0]);
#endif
	progname = argv[0];

	/* this allows the different tools to be called as "mtools -c <command>"
	** where <command> is mdir, mdel, mcopy etcetera
	** Mainly done for the BeOS, which doesn't support links yet.
	*/

	if(argc >= 3 && 
	   !strcmp(argv[1], "-c") &&
	   !strcmp(name, "mtools")) {
		argc-=2;
		argv+=2;
		name = argv[0];
	}



	/* print the version */
	if(argc >= 2 && 
	   (strcmp(argv[1], "-V") == 0 || strcmp(argv[1], "--version") ==0)) {
		printf("%s (GNU mtools) %s\n", 
		       name, mversion);
		printf("configured with the following options: ");
#ifdef USE_XDF
		printf("enable-xdf ");
#else
		printf("disable-xdf ");
#endif
#ifdef USING_VOLD
		printf("enable-vold ");
#else
		printf("disable-vold ");
#endif
#ifdef USING_NEW_VOLD
		printf("enable-new-vold ");
#else
		printf("disable-new-vold ");
#endif
#ifdef DEBUG
		printf("enable-debug ");
#else
		printf("disable-debug ");
#endif
#ifdef USE_RAWTERM
		printf("enable-raw-term ");
#else
		printf("disable-raw-term ");
#endif
		printf("\n");
		return 0;
	}

	read_config();
	setup_signal();
	for (i = 0; i < NDISPATCH; i++) {
		if (!strcmp(name,dispatch[i].cmd))
			dispatch[i].fn(argc, argv, dispatch[i].type);
	}
	if (strcmp(name,"mtools"))
		fprintf(stderr,"Unknown mtools command '%s'\n",name);
	fprintf(stderr,"Supported commands:");
	for (i = 0; i < NDISPATCH; i++) {
		if (i%8 == 0) putc('\n', stderr);
		else fprintf(stderr, ", ");
		fprintf(stderr, "%s", dispatch[i].cmd);
	}
	putc('\n', stderr);

	return 1;
}

int helpFlag(int argc, char **argv) {
	return (argc > 1 && !strcmp(argv[1], "--help"));
}
