/* partype v1.0.0 Partition type locator
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
 *	  1	Partition /dev/bda1 has the specified type.
 *	  2	Partition /dev/bda2 has the specified type.
 *	  3	Partition /dev/bda3 has the specified type.
 *	  4	Partition /dev/bda4 has the specified type.
 *
 *	 65	Partition /dev/hda1 has the specified type.
 *	 66	Partition /dev/hda2 has the specified type.
 *	 67	Partition /dev/hda3 has the specified type.
 *	 68	Partition /dev/hda4 has the specified type.
 *
 *	129	Partition /dev/sda1 has the specified type.
 *	130	Partition /dev/sda2 has the specified type.
 *	131	Partition /dev/sda3 has the specified type.
 *	132	Partition /dev/sda4 has the specified type.
 *
 *	252	The raw drive is not seekable.
 *	253	Neither /dev/hda nor /dev/bda is available.
 *	254	An invalid partition type was specified.
 *	255	Usage message displayed.
 *
 * Note that the error code only indicates the FIRST partition in the
 * sequence listed above that is of the relevant type.
 */

#include <stdio.h>

unsigned char digit(char ch)
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

int main(int argc,char **argv)
{
    unsigned char table[64];
    FILE *fp;
    char *ptr = argv[1], *drive = "/dev/bda";
    unsigned char n, partition = 0, result = 0;

    if (argc != 2) {
	fprintf(stderr, "Usage: %s <type>\n\n", *argv);
	fprintf(stderr, "Where <type> is the hexadecimal partition type required\n");
	exit(255);
    }
    while (*ptr) {
	partition *= 16;
	partition += digit(*ptr++);
    }
    if (!partition) {
	fprintf(stderr, "ERROR 1: Invalid partition type: %02X (%s)\n",
		partition, argv[1]);
	exit(254);
    }
    
    if ((fp = fopen(drive,"rb")) == NULL) {
	drive[5] = 'h';
	if ((fp = fopen(drive,"rb")) == NULL) {
	    drive[5] = 's';
	    if ((fp = fopen(drive,"rb")) == NULL) {
		fprintf(stderr, "ERROR 2: Can't open raw drive.\n");
		fprintf(stderr, "         Searched /dev/hda - /dev/sda - /dev/bda only.\n");
		exit(253);
	    }
	}
    }
    if (fseek(fp,0x1c0,SEEK_SET)) {
	fprintf(stderr, "ERROR 3: Can't seek to partition table in /dev/bda\n");
	exit(252);
    }
    for (n=0; n<64; n++)
	table[n] = fgetc(fp);
    for (n=4; n; n--)
	if (table[16*n-14] == partition)
	    result = n;
    if (result) {
	printf("%s%d\n",drive,result);
	switch (drive[5]) {
	    case 's':
		result += 64;
	    case 'h':
		result += 64;
	    case 'b':
		break;
	}
    }
    exit(result);
}
