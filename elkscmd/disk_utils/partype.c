/* partype v1.1.0 Partition type locator
 * Copyright (C) 2002, Riley H Williams G7GOD,
 * Released under the GNU General Public Licence, version 2 only.
 *
 * This program searches the partition table of the primary hard drive for
 * a partition with a particular hex code for the partition type. It only
 * searches the four primary partitions, not logical partitions.
 *
 * If it finds any partitions of the specified type, it displays the name
 * of the lowest numbered partition on stdout.
 *
 * On completion, it returns one of the following error codes:
 *
 *	  0	No partition with the specified type was found.
 *
 *	  1	Partition /dev/hda1 has the specified type.
 *	  2	Partition /dev/hda2 has the specified type.
 *	  3	Partition /dev/hda3 has the specified type.
 *	  4	Partition /dev/hda4 has the specified type.
 *
 *	 65	Partition /dev/dhda1 has the specified type.
 *	 66	Partition /dev/dhda2 has the specified type.
 *	 67	Partition /dev/dhda3 has the specified type.
 *	 68	Partition /dev/dhda4 has the specified type.
 *
 *	129	Partition /dev/sda1 has the specified type.
 *	130	Partition /dev/sda2 has the specified type.
 *	131	Partition /dev/sda3 has the specified type.
 *	132	Partition /dev/sda4 has the specified type.
 *
 *	251	The raw drive is not seekable.
 *	252	Neither /dev/dhda nor /dev/hda is available.
 *	253	The minimum size specified was larger than the
 *		maximum size specified.
 *	254	An invalid partition type was specified.
 *	255	Usage message displayed.
 *
 * Note that the error code only indicates the FIRST partition in the
 * sequence listed above that is of the relevant type.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    unsigned char	head;
    unsigned char	sector;
    unsigned char	cylinder;
} chs;

typedef struct {
    unsigned char	active;
    chs			start;
    unsigned char	partype;
    chs			finish;
    unsigned long int	system;
    unsigned long int	size;
} partentry;

typedef union {
    char c[64];
    partentry e[4];
} partable;

unsigned char digit(char ch)
{
    if (ch >= '0' && ch <= '9')
	return ch - '0';
    else
	return 0;
}

unsigned char xdigit(char ch)
{
    if (ch >= '0' && ch <= '9')
	return ch - '0';
    else if (ch >= 'A' && ch <= 'F')
	return ch + 10 - 'A';
    else if (ch >= 'a' && ch <= 'f')
	return ch + 10 - 'a';
    else
	return 0;
}

unsigned long int decimal(char *ptr)
{
    unsigned long result = 0;

    while (*ptr) {
	result *= 10;
	result += digit(*ptr++);
    }
    return result;
}

unsigned long int hex(char *ptr)
{
    unsigned long result = 0;

    while (*ptr) {
	result *= 16;
	result += xdigit(*ptr++);
    }
    return result;
}

int main(int argc,char **argv)
{
    partable table;
    FILE *fp;
    char drive[16];
    unsigned long min = 100UL, max = 16383UL, size;
    unsigned char n, partition = 0;

    if (argc != 2 && argc != 4) {
	fprintf(stderr, "Usage: %s <type> [ <min> <max> ]\n\n", *argv);
	fprintf(stderr, "where  <type>  is the hexadecimal partition type required,\n"
			"       <min>   is the minimum acceptable partition size in Kilobytes,\n"
			"  and  <max>   is the maximum acceptable partition size in Kilobytes.\n\n");
	fprintf(stderr, "By defaults, partitions between %luk and %luk in size are valid.\n",
			min, max);
	exit(255);
    }
    partition = hex(argv[1]);
    if (!partition) {
	fprintf(stderr, "ERROR 1: Invalid partition type: %02X (%s)\n",
		partition, argv[1]);
	exit(254);
    }
    if (argc == 4) {
	min = decimal(argv[2]);
	max = decimal(argv[3]);
	if (min > max) {
	    fprintf(stderr, "ERROR 2: size range %lu to %lu is invalid.\n",
			    min, max);
	    exit(253);
	}
    }
    strcpy(drive, "/dev/hda");
    if ((fp = fopen(drive,"rb")) == NULL) {
	strcpy(drive, "/dev/dhda");
	if ((fp = fopen(drive,"rb")) == NULL) {
	    strcpy(drive, "/dev/shda");
	    if ((fp = fopen(drive,"rb")) == NULL) {
		fprintf(stderr, "ERROR 3: Can't open raw drive.\n");
		fprintf(stderr, "         Searched /dev/hda - /dev/dhda - /dev/sda only.\n");
		exit(252);
	    }
	}
    }
    if (fseek(fp,0x1be,SEEK_SET)) {
	fprintf(stderr, "ERROR 4: Can't seek to partition table in /dev/hda\n"
			"         ");
	perror("Reason");
	exit(251);
    }
    for (n=0; n<64; n++)
	table.c[n] = fgetc(fp);
    fclose(fp);
    for (n=0; n<4; n++) {
	size = table.e[n].size;
	if (table.e[n].partype == partition && min <= size && size <= max)
	    break;
    }
    if (n < 4) {
	printf("%s%d\n",drive,n+1);
	switch (drive[5]) {
	    case 's':
		n += 64;
	    case 'h':
		n += 64;
	    case 'b':
		break;
	}
    }
    exit(n);
}
