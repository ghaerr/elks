/*
 * setboot - modify ELKS boot block parameters
 *
 * 7 Feb 2020 Greg Haerr <greg@censoft.com>
 *
 * Usage: setboot <image> [-B<sectors>,<heads>] [<input_boot_sector>]
 *
 *	setboot writes image after optionally reading an input boot sector and
 *		optionally modifying boot sector disk parameters passed as parameters.
 *
 *	Examples:
 *		setboot <image> -B9,2
 * 			-> set 9 sectors 2 heads on boot sector or bootable disk <image> file
 *		setboot <image> -B18,2 <bootsector>
 *			-> read <bootsector>, set 18 sectors 2 heads, write <image>
 *
 *	Currently only writes ELKS sector_max and head_max values.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/stat.h>

#define SECT_SIZE 512

/* See bootblocks/minix.map for the offsets */
#define ELKS_BPB_SecPerTrk	0x1F9		/* offset of sectors per track (byte)*/
#define ELKS_BPB_NumHeads	0x1FA		/* offset of number of heads (byte)*/

static int SecPerTrk, NumHeads;

/* set sector and head parameters in buffer*/
static void setSHparms(unsigned char *buf)
{
	buf[ELKS_BPB_SecPerTrk] = (unsigned char)SecPerTrk;
	buf[ELKS_BPB_NumHeads] = (unsigned char)NumHeads;
}

/* Print an error message and die*/
static void fatalmsg(const char *s,...)
{
	va_list p;
	va_start(p,s);
	vfprintf(stderr,s,p);
	va_end(p);
	putc('\n',stderr);  
	exit(-1);
}

/* Like fatalmsg but also show the errno message*/
static void die(const char *s,...)
{
	va_list p;
	va_start(p,s);
	vfprintf(stderr,s,p);
	va_end(p);
	putc(':',stderr);
	putc(' ',stderr);
	perror(NULL);
	putc('\n',stderr);
	exit(1);
}

int main(int argc,char **argv)
{
	FILE *ifp = NULL, *ofp;
	struct stat sb;
	int count;
	int opt_new_bootsect = 0, opt_updatebpb = 0;
	char *outfile, *infile = NULL;
	unsigned char blk[SECT_SIZE];

	if (argc != 3 && argc != 4)
		fatalmsg("Usage: %s <image> [-B<sectors>,<heads>] [<input_boot_image>]\n", argv[0]);

	outfile = *++argv; argc--;

	if (argv[1][0] == '-' && argv[1][1] == 'B') {
		opt_updatebpb = 1;			/* BPB update specified*/
		if (sscanf(&argv[1][2], "%d,%d", &SecPerTrk, &NumHeads) != 2)
			fatalmsg("Invalid -B<sectors>,<heads> option\n");
		printf("Updating BPB to %d sectors, %d heads\n", SecPerTrk, NumHeads);
		argv++;
		argc--;
	}
	if (argc == 2) {			/* new boot sector specified*/
		infile = *++argv;
		opt_new_bootsect = 1;
	}

	if (opt_new_bootsect) {
		if (stat(infile,&sb)) die("stat(%s)",infile);
		if (!S_ISREG(sb.st_mode)) fatalmsg("%s: not a regular file\n",infile);
		if (sb.st_size != SECT_SIZE)
				fatalmsg("%s: boot sector size not equal to %d bytes\n", infile, SECT_SIZE);

		ifp = fopen(infile,"rb");
		if (!ifp) die(infile);

		ofp = fopen(outfile,"r+b");
		if (!ofp) die(outfile);

		count = fread(blk,1,SECT_SIZE,ifp);
		if (count != sb.st_size) die("fread(%s)", infile);

		if (blk[510] != 0x55 || blk[511] != 0xaa)
		fprintf(stderr, "Warning: '%s' may not be valid bootable sector\n", infile);

		if (opt_updatebpb)		/* update BPB before writing*/
				setSHparms(blk);
		if (fwrite(blk,1,count,ofp) != count) die("fwrite(%s)", infile);
			fclose(ofp);
			fclose(ifp);
		} else {			/* perform BPB update only on existing boot block*/
			ofp = fopen(outfile, "r+b");
			if (!ofp) die(outfile);

			count = fread(blk,1,SECT_SIZE,ofp);
			if (count != 512) die("fread(%s)", outfile);
			setSHparms(blk);
			if (fseek(ofp, 0L, SEEK_SET) != 0)
			die("fseek(%s)", outfile);
			if (fwrite(blk,1,SECT_SIZE,ofp) != SECT_SIZE) die("fwrite(%s)", outfile);
			fclose(ofp);
		}
	return 0;
}
