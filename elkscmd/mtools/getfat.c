/*
 * Get and decode a FAT (file allocation table) entry.  The FAT entries
 * are 1.5 bytes long and switch nibbles (.5 byte) according to whether
 * or not the entry starts on a byte boundary.  Returns the cluster 
 * number on success or -1 on failure.
 */

#include "msdos.h"

extern int fat_len;
extern unsigned char *fatbuf;

int
getfat(num)
int num;
{
/*
 *	|    byte n     |   byte n+1    |   byte n+2    |
 *	|0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|0|1|2|3|4|5|6|7|
 *	| | | | | | | | | | | | | | | | | | | | | | | | |
 *	|  n.0  |  n.5  | n+1.0 | n+1.5 | n+2.0 | n+2.5 |
 *	    \_____  \____   \______/________/_____   /
 *	      ____\______\________/   _____/  ____\_/
 *	     /     \      \          /       /     \
 *	| n+1.5 |  n.0  |  n.5  | n+2.0 | n+2.5 | n+1.0 |
 *	|      FAT entry k      |    FAT entry k+1      |
 */
	unsigned int fat_hi, fat_low, byte_1, byte_2;
	int start, fat;
					/* which bytes contain the entry */
	start = num * 3 / 2;
	if (start < 0 || start+1 > (fat_len * MSECSIZ))
		return(-1);

	byte_1 = *(fatbuf + start);
	byte_2 = *(fatbuf + start + 1);
					/* (odd) not on byte boundary */
	if (num % 2) {
		fat_hi = (byte_2 & 0xff) << 4;
		fat_low  = (byte_1 & 0xf0) >> 4;
	}
					/* (even) on byte boundary */
	else {
		fat_hi = (byte_2 & 0xf) << 8;
		fat_low  = byte_1 & 0xff;
	}
	fat = (fat_hi + fat_low) & 0xfff;
	return(fat);
}
