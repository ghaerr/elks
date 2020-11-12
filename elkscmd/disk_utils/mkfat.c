/*
 * mkfat - ELKS mkfs for FAT filesystems
 *  Can also be compiled and used on host system
 *
 * Usage: mkfat [-fat32] device size-in-blocks
 *
 * Nov 2020 Greg Haerr
 *  Using modified FAT File IO Library (see Copyright below)
 *  Added FAT12 support
 */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

#define FAT_TYPE_12             1
#define FAT_TYPE_16             2
#define FAT_TYPE_32             3

#define FAT_SECTOR_SIZE         512

typedef unsigned char uint8;
typedef unsigned short uint16;
#ifdef __ia16__
typedef unsigned long uint32;
#else
typedef unsigned int uint32;
#endif

/* Filesystem globals*/
uint8  sectors_per_cluster;
uint32 cluster_begin_lba;
uint32 rootdir_first_cluster;
uint32 rootdir_first_sector;
uint32 rootdir_sectors;
uint32 fat_begin_lba;
uint16 fs_info_sector;
uint32 lba_begin;
uint32 fat_sectors;
uint32 next_free_cluster;
uint16 root_entry_count;
uint16 reserved_sectors;
uint8  num_of_fats;
int    fat_type;
int    ofd;
uint32 blocks;
uint8  sector[FAT_SECTOR_SIZE];

struct sec_per_clus_table
{
    uint32  sectors;
    uint8   sectors_per_cluster;
};

struct sec_per_clus_table _cluster_size_table16[] =
{
    { 4084L,   1},   // 2MB - 512b
    { 32680L,  2},   // 16MB - 1K
    { 262144L, 4},   // 128MB - 2K
    { 524288L, 8},   // 256MB - 4K
    { 1048576L, 16}, // 512MB - 8K
    { 2097152L, 32}, // 1GB - 16K
    { 4194304L, 64}, // 2GB - 32K
    { 8388608L, 128},// 2GB - 64K [Warning only supported by Windows XP onwards]
    { 0 , 0 }        // Invalid
};

struct sec_per_clus_table _cluster_size_table32[] =
{
    { 532480L,   1},   // 260MB - 512b
    { 16777216L, 8},   // 8GB - 4K
    { 33554432L, 16},  // 16GB - 8K
    { 67108864L, 32},  // 32GB - 16K
    { 0xFFFFFFFFL, 64},// >32GB - 32K
    { 0 , 0 }          // Invalid
};

#define SET_32BIT_WORD(buffer, location, value) \
{ buffer[location+0] = (uint8)((value)&0xFF); \
  buffer[location+1] = (uint8)((value>>8)&0xFF); \
  buffer[location+2] = (uint8)((value>>16)&0xFF); \
  buffer[location+3] = (uint8)((value>>24)&0xFF); \
}

#define SET_16BIT_WORD(buffer, location, value) \
{ buffer[location+0] = (uint8)((value)&0xFF); \
  buffer[location+1] = (uint8)((value>>8)&0xFF); \
}

//-----------------------------------------------------------------------------
//                            FAT16/32 File IO Library
//                                    V2.6
//                              Ultra-Embedded.com
//                            Copyright 2003 - 2012
//
//                         Email: admin@ultra-embedded.com
//
//                                License: GPL
//   If you would like a version with a more permissive license for use in
//   closed source commercial applications please contact me for details.
//-----------------------------------------------------------------------------
//
// This file is part of FAT File IO Library.
//
// FAT File IO Library is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// FAT File IO Library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with FAT File IO Library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// fatfs_lba_of_cluster: This function converts a cluster number into a sector /
// LBA number.
//-----------------------------------------------------------------------------
uint32 fatfs_lba_of_cluster(uint32 Cluster_Number)
{
    if (fat_type == FAT_TYPE_32)
        return ((cluster_begin_lba + ((Cluster_Number-2)*sectors_per_cluster)));
    else
        return (cluster_begin_lba + (root_entry_count * 32 / FAT_SECTOR_SIZE) + ((Cluster_Number-2) * sectors_per_cluster));
}

//-----------------------------------------------------------------------------
// fatfs_calc_cluster_size: Calculate what cluster size should be used
//-----------------------------------------------------------------------------
static uint8 fatfs_calc_cluster_size(uint32 sectors, int is_fat32)
{
    int i;

    if (!is_fat32)
    {
        if (fat_type == FAT_TYPE_12)
            return 1;
        for (i=0; _cluster_size_table16[i].sectors_per_cluster != 0;i++)
            if (sectors <= _cluster_size_table16[i].sectors)
                return _cluster_size_table16[i].sectors_per_cluster;
    }
    else
    {
        for (i=0; _cluster_size_table32[i].sectors_per_cluster != 0;i++)
            if (sectors <= _cluster_size_table32[i].sectors)
                return _cluster_size_table32[i].sectors_per_cluster;
    }

    return 0;
}
//-----------------------------------------------------------------------------
// fatfs_erase_sectors: Erase a number of sectors
//-----------------------------------------------------------------------------
static int fatfs_erase_sectors(uint32 lba, int count)
{
    int i;

    // Zero sector first
    memset(sector, 0, FAT_SECTOR_SIZE);

    for (i=0;i<count;i++) {
		lseek(ofd, (lba + i) * FAT_SECTOR_SIZE, SEEK_SET);
        if (write(ofd, sector, FAT_SECTOR_SIZE) != FAT_SECTOR_SIZE)
            return 0;
    }

    return 1;
}
//-----------------------------------------------------------------------------
// fatfs_create_boot_sector: Create the boot sector
//-----------------------------------------------------------------------------
static int fatfs_create_boot_sector(uint32 boot_sector_lba, uint32 vol_sectors, const char *name, int is_fat32)
{
    uint32 total_clusters;
    int i;

    // Zero sector initially
    memset(sector, 0, FAT_SECTOR_SIZE);

    // OEM Name & Jump Code
    sector[0] = 0xEB;
    sector[1] = 0x3C;
    sector[2] = 0x90;
    sector[3] = 0x4D;
    sector[4] = 0x53;
    sector[5] = 0x44;
    sector[6] = 0x4F;
    sector[7] = 0x53;
    sector[8] = 0x35;
    sector[9] = 0x2E;
    sector[10] = 0x30;

    // Bytes per sector
    sector[11] = (FAT_SECTOR_SIZE >> 0) & 0xFF;
    sector[12] = (FAT_SECTOR_SIZE >> 8) & 0xFF;

    // Get sectors per cluster size for the disk
    sectors_per_cluster = fatfs_calc_cluster_size(vol_sectors, is_fat32);
    if (!sectors_per_cluster)
        return 0; // Invalid disk size
    printf("Creating FAT%d, Cluster size %d\n",
        is_fat32? 32: (fat_type == FAT_TYPE_16? 16: 12), sectors_per_cluster);

    // Sectors per cluster
    sector[13] = sectors_per_cluster;

    // Reserved Sectors
    if (!is_fat32)
        reserved_sectors = 1;	//FIXME was 8
    else
        reserved_sectors = 32;
    sector[14] = (reserved_sectors >> 0) & 0xFF;
    sector[15] = (reserved_sectors >> 8) & 0xFF;

    // Number of FATS
    num_of_fats = 2;
    sector[16] = num_of_fats;

    // Max entries in root dir (FAT16 only)
    if (!is_fat32)
    {
        root_entry_count = 512;
        sector[17] = (root_entry_count >> 0) & 0xFF;
        sector[18] = (root_entry_count >> 8) & 0xFF;
    }
    else
    {
        root_entry_count = 0;
        sector[17] = 0;
        sector[18] = 0;
    }

    // [FAT16] Total sectors (use FAT32 count instead)
    sector[19] = 0x00;
    sector[20] = 0x00;

    // Media type
    sector[21] = 0xF8;


    // FAT16 BS Details
    if (!is_fat32)
    {
        // Count of sectors used by the FAT table (FAT16 only)
        total_clusters = (vol_sectors / sectors_per_cluster) + 1;
        fat_sectors = (total_clusters/(FAT_SECTOR_SIZE/2)) + 1;
        sector[22] = (uint8)((fat_sectors >> 0) & 0xFF);
        sector[23] = (uint8)((fat_sectors >> 8) & 0xFF);

        // Sectors per track
        sector[24] = 0x00;
        sector[25] = 0x00;

        // Heads
        sector[26] = 0x00;
        sector[27] = 0x00;

        // Hidden sectors
        sector[28] = 0x00;		//0x1C FIXME was 32
        sector[29] = 0x00;
        sector[30] = 0x00;
        sector[31] = 0x00;

        // Total sectors for this volume
        sector[32] = (uint8)((vol_sectors>>0)&0xFF);
        sector[33] = (uint8)((vol_sectors>>8)&0xFF);
        sector[34] = (uint8)((vol_sectors>>16)&0xFF);
        sector[35] = (uint8)((vol_sectors>>24)&0xFF);

        // Drive number
        sector[36] = 0x00;

        // Reserved
        sector[37] = 0x00;

        // Boot signature
        sector[38] = 0x29;

        // Volume ID
        sector[39] = 0x00;		//FIXME was 0x78563412
        sector[40] = 0x00;
        sector[41] = 0x00;
        sector[42] = 0x00;

        // Volume name
        for (i=0;i<11;i++)
        {
            if (i < (int)strlen(name))
                sector[i+43] = name[i];
            else
                sector[i+43] = ' ';
        }

        // File sys type
        sector[54] = 'F';
        sector[55] = 'A';
        sector[56] = 'T';
        sector[57] = '1';
        sector[58] = fat_type == FAT_TYPE_12? '2': '6';
        sector[59] = ' ';
        sector[60] = ' ';
        sector[61] = ' ';

        // Signature
        sector[510] = 0x55;
        sector[511] = 0xAA;
    }
    // FAT32 BS Details
    else
    {
        // Count of sectors used by the FAT table (FAT16 only)
        sector[22] = 0;
        sector[23] = 0;

        // Sectors per track (default)
        sector[24] = 0x3F;
        sector[25] = 0x00;

        // Heads (default)
        sector[26] = 0xFF;
        sector[27] = 0x00;

        // Hidden sectors
        sector[28] = 0x00;
        sector[29] = 0x00;
        sector[30] = 0x00;
        sector[31] = 0x00;

        // Total sectors for this volume
        sector[32] = (uint8)((vol_sectors>>0)&0xFF);
        sector[33] = (uint8)((vol_sectors>>8)&0xFF);
        sector[34] = (uint8)((vol_sectors>>16)&0xFF);
        sector[35] = (uint8)((vol_sectors>>24)&0xFF);

        total_clusters = (vol_sectors / sectors_per_cluster) + 1;
        fat_sectors = (total_clusters/(FAT_SECTOR_SIZE/4)) + 1;

        // BPB_FATSz32
        sector[36] = (uint8)((fat_sectors>>0)&0xFF);
        sector[37] = (uint8)((fat_sectors>>8)&0xFF);
        sector[38] = (uint8)((fat_sectors>>16)&0xFF);
        sector[39] = (uint8)((fat_sectors>>24)&0xFF);

        // BPB_ExtFlags
        sector[40] = 0;
        sector[41] = 0;

        // BPB_FSVer
        sector[42] = 0;
        sector[43] = 0;

        // BPB_RootClus
        sector[44] = (uint8)((rootdir_first_cluster>>0)&0xFF);
        sector[45] = (uint8)((rootdir_first_cluster>>8)&0xFF);
        sector[46] = (uint8)((rootdir_first_cluster>>16)&0xFF);
        sector[47] = (uint8)((rootdir_first_cluster>>24)&0xFF);

        // BPB_FSInfo
        sector[48] = (uint8)((fs_info_sector>>0)&0xFF);
        sector[49] = (uint8)((fs_info_sector>>8)&0xFF);

        // BPB_BkBootSec
        sector[50] = 6;
        sector[51] = 0;

        // Drive number
        sector[64] = 0x00;

        // Boot signature
        sector[66] = 0x29;

        // Volume ID
        sector[67] = 0x00;		//FIXME was 0x78563412
        sector[68] = 0x00;
        sector[69] = 0x00;
        sector[70] = 0x00;

        // Volume name
        for (i=0;i<11;i++)
        {
            if (i < (int)strlen(name))
                sector[i+71] = name[i];
            else
                sector[i+71] = ' ';
        }

        // File sys type
        sector[82] = 'F';
        sector[83] = 'A';
        sector[84] = 'T';
        sector[85] = '3';
        sector[86] = '2';
        sector[87] = ' ';
        sector[88] = ' ';
        sector[89] = ' ';

        // Signature
        sector[510] = 0x55;
        sector[511] = 0xAA;
    }

    lseek(ofd, boot_sector_lba * FAT_SECTOR_SIZE, SEEK_SET);
    if (write(ofd, sector, FAT_SECTOR_SIZE) != FAT_SECTOR_SIZE)
        return 0;
    return 1;
}
//-----------------------------------------------------------------------------
// fatfs_create_fsinfo_sector: Create the FSInfo sector (FAT32)
//-----------------------------------------------------------------------------
static int fatfs_create_fsinfo_sector(uint32 sector_lba)
{
    // Zero sector initially
    memset(sector, 0, FAT_SECTOR_SIZE);

    // FSI_LeadSig
    sector[0] = 0x52;
    sector[1] = 0x52;
    sector[2] = 0x61;
    sector[3] = 0x41;

    // FSI_StrucSig
    sector[484] = 0x72;
    sector[485] = 0x72;
    sector[486] = 0x41;
    sector[487] = 0x61;

    // FSI_Free_Count
    sector[488] = 0xFF;
    sector[489] = 0xFF;
    sector[490] = 0xFF;
    sector[491] = 0xFF;

    // FSI_Nxt_Free
    sector[492] = 0xFF;
    sector[493] = 0xFF;
    sector[494] = 0xFF;
    sector[495] = 0xFF;

    // Signature
    sector[510] = 0x55;
    sector[511] = 0xAA;

    lseek(ofd, sector_lba * FAT_SECTOR_SIZE, SEEK_SET);
    if (write(ofd, sector, FAT_SECTOR_SIZE) != FAT_SECTOR_SIZE)
        return 0;
    return 1;
}
//-----------------------------------------------------------------------------
// fatfs_erase_fat: Erase FAT table using fs details in fs struct
//-----------------------------------------------------------------------------
static int fatfs_erase_fat(int is_fat32)
{
    uint32 i;

    // Zero sector initially
    memset(sector, 0, FAT_SECTOR_SIZE);

    // Initialise default allocate / reserved clusters
    if (!is_fat32)
    {
		if (fat_type == FAT_TYPE_12)
		{
		    sector[0] = 0xF0;
			sector[1] = 0xFF;
			sector[2] = 0xFF;
		}
		else
		{
            SET_16BIT_WORD(sector, 0, 0xFFF8);
            SET_16BIT_WORD(sector, 2, 0xFFFF);
		}
    }
    else
    {
        SET_32BIT_WORD(sector, 0, 0x0FFFFFF8);
        SET_32BIT_WORD(sector, 4, 0xFFFFFFFF);
        SET_32BIT_WORD(sector, 8, 0x0FFFFFFF);
    }

    lseek(ofd, fat_begin_lba * FAT_SECTOR_SIZE, SEEK_SET);
    if (write(ofd, sector, FAT_SECTOR_SIZE) != FAT_SECTOR_SIZE)
        return 0;

    // Zero remaining FAT sectors
    memset(sector, 0, FAT_SECTOR_SIZE);
    for (i=1;i<fat_sectors*num_of_fats;i++) {
        lseek(ofd, (fat_begin_lba + i) * FAT_SECTOR_SIZE, SEEK_SET);
        if (write(ofd, sector, FAT_SECTOR_SIZE) != FAT_SECTOR_SIZE)
            return 0;
    }
    return 1;
}
//-----------------------------------------------------------------------------
// fatfs_format_fat16: Format a FAT16 partition
//-----------------------------------------------------------------------------
int fatfs_format_fat16(uint32 volume_sectors, const char *name)
{
    next_free_cluster = 0; // Invalid

    // Volume is FAT12/FAT16
    fat_type = volume_sectors <= 4084? FAT_TYPE_12: FAT_TYPE_16;

    // Not valid for FAT12/FAT16
    fs_info_sector = 0;
    rootdir_first_cluster = 0;

    // Sector 0: Boot sector
    // NOTE: We don't need an MBR, it is a waste of a good sector!
    lba_begin = 0;
    if (!fatfs_create_boot_sector(lba_begin, volume_sectors, name, 0))
        return 0;

    // For FAT16 (which this may be), rootdir_first_cluster is actually rootdir_first_sector
    rootdir_first_sector = reserved_sectors + (num_of_fats * fat_sectors);
    rootdir_sectors = ((root_entry_count * 32) + (FAT_SECTOR_SIZE - 1)) / FAT_SECTOR_SIZE;

    // First FAT LBA address
    fat_begin_lba = lba_begin + reserved_sectors;

    // The address of the first data cluster on this volume
    cluster_begin_lba = fat_begin_lba + (num_of_fats * fat_sectors);

    // Initialise FAT sectors
    if (!fatfs_erase_fat(0))
        return 0;

    // Erase Root directory
    if (!fatfs_erase_sectors(lba_begin + rootdir_first_sector, rootdir_sectors))
        return 0;

    return 1;
}
//-----------------------------------------------------------------------------
// fatfs_format_fat32: Format a FAT32 partition
//-----------------------------------------------------------------------------
int fatfs_format_fat32(uint32 volume_sectors, const char *name)
{
    next_free_cluster = 0; // Invalid

    // Volume is FAT32
    fat_type = FAT_TYPE_32;

    // Basic defaults for normal FAT32 partitions
    fs_info_sector = 1;
    rootdir_first_cluster = 2;

    // Sector 0: Boot sector
    // NOTE: We don't need an MBR, it is a waste of a good sector!
    lba_begin = 0;
    if (!fatfs_create_boot_sector(lba_begin, volume_sectors, name, 1))
        return 0;

    // First FAT LBA address
    fat_begin_lba = lba_begin + reserved_sectors;

    // The address of the first data cluster on this volume
    cluster_begin_lba = fat_begin_lba + (num_of_fats * fat_sectors);

    // Initialise FSInfo sector
    if (!fatfs_create_fsinfo_sector(fs_info_sector))
        return 0;

    // Initialise FAT sectors
    if (!fatfs_erase_fat(1))
        return 0;

    // Erase Root directory
    if (!fatfs_erase_sectors(fatfs_lba_of_cluster(rootdir_first_cluster), sectors_per_cluster))
        return 0;

    return 1;
}
//-----------------------------------------------------------------------------
// fatfs_format: Format a partition with either FAT16 or FAT32 based on size
//-----------------------------------------------------------------------------
int fatfs_format(int force_fat32, uint32 volume_sectors, const char *name)
{
    // 2GB - 32K limit for safe behaviour for FAT16
    if (force_fat32 || volume_sectors > 4194304)
        return fatfs_format_fat32(volume_sectors, name);
    else
        return fatfs_format_fat16(volume_sectors, name);
}

int main(int ac, char **av)
{
	int opt_fat32;
	char *image;
	uint32 blocks;

	if (ac > 1 && !strcmp(av[1], "-fat32")) {
		opt_fat32 = 1;
		av++;
		ac--;
	}
	if (ac != 3) {
usage:
		printf("Usage: mkfat [-fat32] device size-in-blocks\n");
		exit(1);
	}
	image = av[1];
	blocks = atol(av[2]);
	if (blocks == 0)
		goto usage;
	if (opt_fat32 && blocks < 32763) {
		printf("Error: FAT32 requires miniumum 32763 blocks\n");
		exit(1);
	}

	ofd = open(image, O_RDWR);
	if (ofd < 0) {
		printf("Can't open %s\n", image);
		exit(1);
	}

	if (!fatfs_format(opt_fat32, blocks * 2, "ELKS")) {
		printf("Can't create FAT image on %s\n", image);
		sync();
		exit(1);
	}
	close(ofd);

	printf("Created filesystem on %s, %lu blocks\n", image, (long)blocks);
	sync();
	return 0;
}
