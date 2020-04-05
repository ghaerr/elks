/*
 * setboot - modify ELKS boot block parameters
 *
 * 7 Feb 2020 Greg Haerr <greg@censoft.com>
 *
 * Usage: setboot <image> [-F] [-{B,P}<sectors>,<heads>[,<tracks>]] [<input_boot_sector>]
 *
 *	setboot writes image after optionally reading an input boot sector and
 *		optionally modifying boot sector disk parameters passed as parameters.
 *
 *	Examples:
 *		setboot <image> -B9,2
 * 			-> set 9 sectors 2 heads on boot sector or bootable disk <image> file
 *		setboot <image> -B18,2 <bootsector>
 *			-> read <bootsector>, set 18 sectors 2 heads, write <image>
 *		setboot <image> -P63,16,63 <bootsector>
 *			-> read <bootsector>, write partition table, write <image>
 *		setboot <image> -F <bootsector>
 *			-> read <bootsector>, copy skipping FAT BPB to output <image>
 *
 *	Currently only writes ELKS sector_max and head_max values.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/stat.h>

#define SECT_SIZE 512

/* See bootblocks/minix.map for the offsets */
#define ELKS_BPB_NumTracks	0x1F7		/* offset of number of tracks (word)*/
#define ELKS_BPB_SecPerTrk	0x1F9		/* offset of sectors per track (byte)*/
#define ELKS_BPB_NumHeads	0x1FA		/* offset of number of heads (byte)*/

/* FAT BPB start and end offsets*/
#define FATBPB_START	11				/* start at bytes per sector*/
#define FATBPB_END		61				/* through end of file system type*/

static unsigned int SecPerTrk, NumHeads, NumTracks;

struct partition
{
    unsigned char boot_ind;	/* 0x80 - active */
    unsigned char head;		/* starting head */
    unsigned char sector;	/* starting sector */
    unsigned char cyl;		/* starting cylinder */
    unsigned char sys_ind;	/* What partition type */
    unsigned char end_head;	/* end head */
    unsigned char end_sector;	/* end sector */
    unsigned char end_cyl;	/* end cylinder */
    unsigned short start_sect;	/* starting sector counting from 0 */
    unsigned short start_sect_hi;
    unsigned short nr_sects;		/* nr of sectors in partition */
    unsigned short nr_sects_hi;
};
#define PARTITION_START		0x01be	/* offset of partition table in MBR*/

/* write partition table in buffer*/
static void writePartition(unsigned char *buf)
{
	struct partition *p = (struct partition *)&buf[PARTITION_START];
	unsigned long nr_sects = ((long)NumTracks) * NumHeads * SecPerTrk;

	/* create partition #1 at sector 2 using max CHS as size*/
	p->boot_ind = 0x80;
#if 1
	p->head = 1;
	p->sector = 1;				/* next cylinder after MBR, standard*/
	p->cyl = 0;
	p->start_sect = NumTracks;	/* zero-relative start sector here*/
#else
	p->head = 0;
	p->sector = 2;				/* next sector after MBR, non-standard*/
	p->cyl = 0;
	p->start_sect = 1;			/* zero-relative start sector here*/
#endif
	p->sys_ind = 0x80;			/* ELKS, Old Minix*/
	p->end_head = NumHeads;
	p->end_sector = SecPerTrk | ((NumTracks >> 2) & 0xc0);
	p->end_cyl = NumTracks & 0xff;
	p->start_sect_hi = 0;
	p->nr_sects = nr_sects & 0xffff;
	p->nr_sects_hi = nr_sects >> 16;
	buf[510] = 0x55;
	buf[511] = 0xAA;
}

/* set sector and head parameters in buffer*/
static void setSHparms(unsigned char *buf)
{
	buf[ELKS_BPB_SecPerTrk] = (unsigned char)SecPerTrk;
	buf[ELKS_BPB_NumHeads] = (unsigned char)NumHeads;
	if (NumTracks) {
		buf[ELKS_BPB_NumTracks] = (unsigned char)NumTracks;
		buf[ELKS_BPB_NumTracks+1] = (unsigned char)(NumTracks >> 8);
	}
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

static int numcommas(char *line)
{
	int count = 0;
	char *p;

	for (p=line; *p; )
		if (*p++ == ',')
			count++;
	return count;
}

int main(int argc,char **argv)
{
	FILE *ifp = NULL, *ofp;
	struct stat sb;
	int count, i;
	int opt_new_bootsect = 0, opt_updatebpb = 0;
	int opt_skipfatbpb = 0;
	int opt_writepartitiontable = 0;
	char *outfile, *infile = NULL;
	unsigned char blk[SECT_SIZE*2];
	unsigned char inblk[SECT_SIZE];

	if (argc != 3 && argc != 4 && argc != 5)
		fatalmsg("Usage: %s <image> [-F] [-{B,P}<sectors>,<heads>[,<tracks>]] [<input_boot_image>]\n", argv[0]);

	outfile = *++argv; argc--;

	while (argv[1] && argv[1][0] == '-') {
		if (argv[1][1] == 'B') {
			opt_updatebpb = 1;			/* BPB update specified*/
			if (numcommas(&argv[1][2]) == 2) {		/* tracks specified*/
					if (sscanf(&argv[1][2], "%d,%d,%d", &SecPerTrk, &NumHeads, &NumTracks) != 3)
						fatalmsg("Invalid -B<sectors>,<heads>,<tracks> option\n");
					printf("Updating BPB to %d sectors, %d heads, %d tracks\n",
						SecPerTrk, NumHeads, NumTracks);
			} else {
					if (sscanf(&argv[1][2], "%d,%d", &SecPerTrk, &NumHeads) != 2)
						fatalmsg("Invalid -B<sectors>,<heads> option\n");
					printf("Updating BPB to %d sectors, %d heads\n", SecPerTrk, NumHeads);
			}
			argv++;
			argc--;
		}
		else if (argv[1][1] == 'P') {
			opt_writepartitiontable = 1;
				if (sscanf(&argv[1][2], "%d,%d,%d", &SecPerTrk, &NumHeads, &NumTracks) != 3)
					fatalmsg("Invalid -P<sectors>,<heads>,<cylinders> option\n");
				printf("Creating partition table %d sectors, %d heads, %d cylinders\n",
					SecPerTrk, NumHeads, NumTracks);
			argv++;
			argc--;
		}
		else if (argv[1][1] == 'F') {
			opt_skipfatbpb = 1;			/* skip FAT BPB specified*/
			printf("Skipping FAT BPB\n");
			argv++;
			argc--;
		}
	}
	if (argc == 2) {			/* new boot sector specified*/
		infile = *++argv;
		opt_new_bootsect = 1;
	}

	if (opt_skipfatbpb && !opt_new_bootsect)
		fatalmsg("-F option requires input_boot_image\n");

	if (opt_writepartitiontable && !opt_new_bootsect)
		fatalmsg("-P option requires input_boot_image\n");

	if (opt_new_bootsect) {
		if (stat(infile,&sb)) die("stat(%s)",infile);
		if (!S_ISREG(sb.st_mode)) fatalmsg("%s: not a regular file\n",infile);
		if (opt_skipfatbpb && sb.st_size != SECT_SIZE)
			fatalmsg("%s: boot sector size not equal to %d bytes\n", infile, SECT_SIZE);
		else if (sb.st_size > SECT_SIZE * 2)
			fatalmsg("%s: Bad boot sector size: %d bytes\n", sb.st_size);

		ifp = fopen(infile,"rb");
		if (!ifp) die(infile);

		ofp = fopen(outfile,"r+b");
		if (!ofp) die(outfile);

		if (opt_skipfatbpb) {
			/* read boot block into temp buffer*/
			count = fread(inblk,1,SECT_SIZE,ifp);
			if (count != sb.st_size) die("fread(%s)", infile);

			/* read image boot sector into output buffer*/
			count = fread(blk,1,SECT_SIZE,ofp);
			if (count != sb.st_size) die("fread(%s)", infile);
			if (fseek(ofp, 0L, SEEK_SET) != 0)
				die("fseek(%s)", outfile);

			/* copy boot sector skipping FAT BPB*/
			for (i=0; i<SECT_SIZE; i++) {
				if (i < FATBPB_START || i > FATBPB_END)
					blk[i] = inblk[i];
			}
		} else {
			/* read boot block directly into output block*/
			count = fread(blk,1,SECT_SIZE*2,ifp);
			if (count != sb.st_size) die("fread(%s)", infile);

		}

		if (opt_writepartitiontable) {	/* create parititon table before writing*/
			writePartition(blk);
			count = 512;
		}
		if (opt_updatebpb)				/* update BPB before writing*/
			setSHparms(blk);

		if (blk[510] != 0x55 || blk[511] != 0xaa)
			fprintf(stderr, "Warning: '%s' may not be valid bootable sector\n", infile);

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
