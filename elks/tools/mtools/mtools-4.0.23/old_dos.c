#include "sysincludes.h"
#include "mtools.h"

static struct OldDos_t old_dos[]={
{   40,  9,  1, 4, 1, 2, 0xfc }, /*  180 KB */
{   40,  9,  2, 7, 2, 2, 0xfd }, /*  360 KB */
{   40,  8,  1, 4, 1, 1, 0xfe }, /*  160 KB */
{   40,  8,  2, 7, 2, 1, 0xff }, /*  320 KB */
{   80,  9,  2, 7, 2, 3, 0xf9 }, /*  720 KB */
{   80, 15,  2,14, 1, 7, 0xf9 }, /* 1200 KB */
{   80, 18,  2,14, 1, 9, 0xf0 }, /* 1440 KB */
{   80, 36,  2,15, 2, 9, 0xf0 }, /* 2880 KB */

/* Source: https://en.wikipedia.org/w/index.php?title=File_Allocation_Table&oldid=560606333#Exceptions : */
/* https://www.win.tue.nl/~aeb/linux/fs/fat/fat-1.html */
{   80,  8,  2, 7, 2, 2, 0xfb }, /* 640 KB */
{   80,  8,  1, 7, 2, 2, 0xfa }, /* 320 KB */
{   80,  9,  1, 7, 2, 2, 0xf8 }, /* 360 KB */
};

/**
 * Get Old Dos parameters for a filesystem of size KBytes (assuming
 * 512 byte sectors), i.e. number of sectors is double the size
 */
struct OldDos_t *getOldDosBySize(size_t size) {
	size_t i;
	size = size * 2;
	for(i=0; i < sizeof(old_dos) / sizeof(old_dos[0]); i++){
		if (old_dos[i].sectors *
		    old_dos[i].tracks *
		    old_dos[i].heads == size)
			return &old_dos[i];
	}
	return NULL;
}

struct OldDos_t *getOldDosByMedia(int media) {
	size_t i;
	for(i=0; i < sizeof(old_dos) / sizeof(old_dos[0]); i++){
		if (old_dos[i].media == media)
			return &old_dos[i];
	}
	fprintf(stderr, "Unknown media type %02x\n", media);
	return NULL;
}

struct OldDos_t *getOldDosByParams(unsigned int tracks,
				   unsigned int heads,
				   unsigned int sectors,
				   unsigned int dir_len,
				   unsigned int cluster_size) {
	size_t i;
	for(i=0; i < sizeof(old_dos) / sizeof(old_dos[0]); i++){
		if (sectors == old_dos[i].sectors &&
		    tracks == old_dos[i].tracks &&
		    heads == old_dos[i].heads &&
		    (dir_len == 0 || dir_len == old_dos[i].dir_len) &&
		    (cluster_size == 0 ||
		     cluster_size == old_dos[i].cluster_size)) {
			return &old_dos[i];
		}
	}
	return NULL;
}

int setDeviceFromOldDos(int media, struct device *dev) {
	struct OldDos_t *params=getOldDosByMedia(media);
	if(params == NULL)
		return -1;
	dev->heads = params->heads;
	dev->tracks = params->tracks;
	dev->sectors = params->sectors;
	dev->ssize = 0x80;
	dev->use_2m = ~1u;
	return 0;
}
