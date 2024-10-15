#ifndef MTOOLS_FSP_H
#define MTOOLS_FSP_H

/*  Copyright 1996-1999,2001-2003,2008,2009 Alain Knaff.
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
#include "stream.h"
#include "msdos.h"
#include "fs.h"

typedef enum fatAccessMode_t {
	FAT_ACCESS_READ,
	FAT_ACCESS_WRITE
} fatAccessMode_t;

typedef struct Fs_t {
	Class_t *Class;
	int refs;
	Stream_t *Next;
	Stream_t *Buffer;
	
	int serialized;
	unsigned long serial_number;
	unsigned int cluster_size;
	unsigned int sector_size;
	int fat_error;

	unsigned int (*fat_decode)(struct Fs_t *This, unsigned int num);
	void (*fat_encode)(struct Fs_t *This, unsigned int num,
			   unsigned int code);

	Stream_t *Direct;
	int fat_dirty;
	unsigned int fat_start;
	unsigned int fat_len;

	unsigned int num_fat;
	unsigned int end_fat;
	unsigned int last_fat;
	int fat_bits; /* must be signed, because we use negative values
		       * for special purposes */
	struct FatMap_t *FatMap;

	unsigned int dir_start;
	unsigned int dir_len;
	unsigned int clus_start;

	unsigned int num_clus;
	char drive; /* for error messages */

	/* fat 32 */
	unsigned int primaryFat;
	unsigned int writeAllFats;
	unsigned int rootCluster;
	unsigned int infoSectorLoc;
	unsigned int last; /* last sector allocated, or MAX32 if unknown */
	unsigned int freeSpace; /* free space, or MAX32 if unknown */
	int preallocatedClusters;

	int lastFatSectorNr;
	unsigned char *lastFatSectorData;
	fatAccessMode_t lastFatAccessMode;
	int sectorMask;
	int sectorShift;

	doscp_t *cp;
} Fs_t;

int fs_free(Stream_t *Stream);

void set_fat12(Fs_t *Fs);
void set_fat16(Fs_t *Fs);
void set_fat32(Fs_t *Fs);
unsigned int get_next_free_cluster(Fs_t *Fs, unsigned int last);
unsigned int fatDecode(Fs_t *This, unsigned int pos);
void fatAppend(Fs_t *This, unsigned int pos, unsigned int newpos);
void fatDeallocate(Fs_t *This, unsigned int pos);
void fatAllocate(Fs_t *This, unsigned int pos, unsigned int value);
void fatEncode(Fs_t *This, unsigned int pos, unsigned int value);

int fat_read(Fs_t *This, union bootsector *boot,
			 size_t tot_sectors, int nodups);
void fat_write(Fs_t *This);
int zero_fat(Fs_t *Fs, int media_descriptor);
extern Class_t FsClass;
int fsPreallocateClusters(Fs_t *Fs, long);
Fs_t *getFs(Stream_t *Stream);


#endif
