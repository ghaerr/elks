#ifndef MTOOLS_MSDOS_H
#define MTOOLS_MSDOS_H

/*  Copyright 1986-1992 Emmet P. Gray.
 *  Copyright 1996-1998,2000-2003,2006,2007,2009 Alain Knaff.
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
 * msdos common header file
 */

#define MAX_SECTOR	8192   		/* largest sector size */
#define MDIR_SIZE	32		/* MSDOS directory entry size in bytes*/
#define MAX_CLUSTER	8192		/* largest cluster size */
#ifndef MAX_PATH
#define MAX_PATH	128		/* largest MSDOS path length */
#endif
#define MAX_DIR_SECS	64		/* largest directory (in sectors) */
#define MSECTOR_SIZE    msector_size

#define NEW		1
#define OLD		0

#define _WORD(x) ((uint16_t)((unsigned char)(x)[0] + (((unsigned char)(x)[1]) << 8)))
#define _DWORD(x) ((uint32_t)(_WORD(x) + (_WORD((x)+2) << 16)))

#define DELMARK ((char) 0xe5)
#define ENDMARK ((char) 0x00)

struct directory {
	char name[8];			/*  0 file name */
	char ext[3];			/*  8 file extension */
	unsigned char attr;		/* 11 attribute byte */
	unsigned char Case;		/* 12 case of short filename */
	unsigned char ctime_ms;		/* 13 creation time, milliseconds (?) */
	unsigned char ctime[2];		/* 14 creation time */
	unsigned char cdate[2];		/* 16 creation date */
	unsigned char adate[2];		/* 18 last access date */
	unsigned char startHi[2];	/* 20 start cluster, Hi */
	unsigned char time[2];		/* 22 time stamp */
	unsigned char date[2];		/* 24 date stamp */
	unsigned char start[2];		/* 26 starting cluster number */
	unsigned char size[4];		/* 28 size of the file */
};

#define EXTCASE 0x10
#define BASECASE 0x8

#define MAX16 0xffff
#define MAX32 0xffffffff
#define MAX_SIZE 0x7fffffff

#define FILE_SIZE(dir)  (_DWORD((dir)->size))
#define START(dir) (_WORD((dir)->start))
#define STARTHI(dir) (_WORD((dir)->startHi))

/* ASSUMPTION: long is at least 32 bits */
UNUSED(static __inline__ void set_dword(unsigned char *data, uint32_t value))
{
	data[3] = (value >> 24) & 0xff;
	data[2] = (value >> 16) & 0xff;
	data[1] = (value >>  8) & 0xff;
	data[0] = (value >>  0) & 0xff;
}


/* ASSUMPTION: short is at least 16 bits */
UNUSED(static __inline__ void set_word(unsigned char *data, unsigned short value))
{
	data[1] = (value >>  8) & 0xff;
	data[0] = (value >>  0) & 0xff;
}


/*
 *	    hi byte     |    low byte
 *	|7|6|5|4|3|2|1|0|7|6|5|4|3|2|1|0|
 *  | | | | | | | | | | | | | | | | |
 *  \   7 bits    /\4 bits/\ 5 bits /
 *     year +80     month     day
 */
#define	DOS_YEAR(dir) (((dir)->date[1] >> 1) + 1980)
#define	DOS_MONTH(dir) (((((dir)->date[1]&0x1) << 3) + ((dir)->date[0] >> 5)))
#define	DOS_DAY(dir) ((dir)->date[0] & 0x1f)

/*
 *	    hi byte     |    low byte
 *	|7|6|5|4|3|2|1|0|7|6|5|4|3|2|1|0|
 *      | | | | | | | | | | | | | | | | |
 *      \  5 bits /\  6 bits  /\ 5 bits /
 *         hour      minutes     sec*2
 */
#define	DOS_HOUR(dir) ((dir)->time[1] >> 3)
#define	DOS_MINUTE(dir) (((((dir)->time[1]&0x7) << 3) + ((dir)->time[0] >> 5)))
#define	DOS_SEC(dir) (((dir)->time[0] & 0x1f) * 2)


typedef struct InfoSector_t {
	unsigned char signature1[4];
	unsigned char filler1[0x1e0];
	unsigned char signature2[4];
	unsigned char count[4];
	unsigned char pos[4];
	unsigned char filler2[14];
	unsigned char signature3[2];
} InfoSector_t;

#define INFOSECT_SIGNATURE1 0x41615252
#define INFOSECT_SIGNATURE2 0x61417272


typedef struct label_blk_t {
	unsigned char physdrive;	/* 36 physical drive ? */
	unsigned char reserved;		/* 37 reserved */
	unsigned char dos4;		/* 38 dos > 4.0 diskette */
	unsigned char serial[4];       	/* 39 serial number */
	char label[11];			/* 43 disk label */
	char fat_type[8];		/* 54 FAT type */
} label_blk_t;

#define has_BPB4 (labelBlock->dos4 == 0x28 || labelBlock->dos4 == 0x29)

/* FAT32 specific info in the bootsector */
struct fat32_t {
	unsigned char bigFat[4];	/* 36 nb of sectors per FAT */
	unsigned char extFlags[2];     	/* 40 extension flags */
	unsigned char fsVersion[2];	/* 42 ? */
	unsigned char rootCluster[4];	/* 44 start cluster of root dir */
	unsigned char infoSector[2];	/* 48 changeable global info */
	unsigned char backupBoot[2];	/* 50 back up boot sector */
	unsigned char reserved[6];	/* 52 ? */
	unsigned char reserved2[6];	/* 52 ? */
	struct label_blk_t labelBlock;
}; /* ends at 58 */

typedef struct oldboot_t {
	struct label_blk_t labelBlock;
	unsigned char res_2m;		/* 62 reserved by 2M */
	unsigned char CheckSum;		/* 63 2M checksum (not used) */
	unsigned char fmt_2mf;		/* 64 2MF format version */
	unsigned char wt;		/* 65 1 if write track after format */
	unsigned char rate_0;		/* 66 data transfer rate on track 0 */
	unsigned char rate_any;		/* 67 data transfer rate on track<>0 */
	unsigned char BootP[2];		/* 68 offset to boot program */
	unsigned char Infp0[2];		/* 70 T1: information for track 0 */
	unsigned char InfpX[2];		/* 72 T2: information for track<>0 */
	unsigned char InfTm[2];		/* 74 T3: track sectors size table */
	unsigned char DateF[2];		/* 76 Format date */
	unsigned char TimeF[2];		/* 78 Format time */
	unsigned char junk[1024 - 80];	/* 80 remaining data */
} oldboot_t;

struct bootsector_s {
	unsigned char jump[3];		/* 0  Jump to boot code */
	char banner[8] NONULLTERM; 	/* 3  OEM name & version */
	unsigned char secsiz[2];	/* 11 Bytes per sector hopefully 512 */
	unsigned char clsiz;    	/* 13 Cluster size in sectors */
	unsigned char nrsvsect[2];	/* 14 Number of reserved (boot) sectors */
	unsigned char nfat;		/* 16 Number of FAT tables hopefully 2 */
	unsigned char dirents[2];	/* 17 Number of directory slots */
	unsigned char psect[2]; 	/* 19 Total sectors on disk */
	unsigned char descr;		/* 21 Media descriptor=first byte of FAT */
	unsigned char fatlen[2];	/* 22 Sectors in FAT */
	unsigned char nsect[2];		/* 24 Sectors/track */
	unsigned char nheads[2];	/* 26 Heads */
	unsigned char nhs[4];		/* 28 number of hidden sectors */
	unsigned char bigsect[4];	/* 32 big total sectors */

	union {
		struct fat32_t fat32;
		struct oldboot_t old;
	} ext;
};

#define MAX_BOOT 4096

union bootsector {
	unsigned char bytes[MAX_BOOT];
	char characters[MAX_BOOT];
	struct bootsector_s boot;
};

#define CHAR(x) (boot->x[0])
#define WORD(x) (_WORD(boot->boot.x))
#define DWORD(x) (_DWORD(boot->boot.x))

#define WORD_S(x) (_WORD(boot.boot.x))
#define DWORD_S(x) (_DWORD(boot.boot.x))

#define OFFSET(x) (((char *) (boot->x)) - ((char *)(boot->jump)))

/* max FAT12/FAT16 sizes, according to
   
 https://staff.washington.edu/dittrich/misc/fatgen103.pdf
 https://download.microsoft.com/download/1/6/1/161ba512-40e2-4cc9-843a-923143f3456c/fatgen103.doc

 interestingly enough, another Microsoft document 
 [http://support.microsoft.com/default.aspx?scid=kb%3ben-us%3b67321]
 gives different values, but the first seems to be more sure about
 itself, so we believe that one ;-)
*/
#define FAT12 0x0ff5 /* max. number + 1 of clusters described by a 12 bit FAT */
#define FAT16 0xfff5 /* max number + 1 of clusters for a 16 bit FAT */

#define ATTR_ARCHIVE 0x20
#define ATTR_DIR 0x10
#define ATTR_LABEL 0x8
#define ATTR_SYSTEM 0x4
#define ATTR_HIDDEN 0x2
#define ATTR_READONLY 0x1

#define HAS_BIT(entry,x) ((entry)->dir.attr & (x))

#define IS_ARCHIVE(entry) (HAS_BIT((entry),ATTR_ARCHIVE))
#define IS_DIR(entry) (HAS_BIT((entry),ATTR_DIR))
#define IS_LABEL(entry) (HAS_BIT((entry),ATTR_LABEL))
#define IS_SYSTEM(entry) (HAS_BIT((entry),ATTR_SYSTEM))
#define IS_HIDDEN(entry) (HAS_BIT((entry),ATTR_HIDDEN))
#define IS_READONLY(entry) (HAS_BIT((entry),ATTR_READONLY))


#define MAX_BYTES_PER_CLUSTER (32*1024)
/* Experimentally, it turns out that DOS only accepts cluster sizes
 * which are powers of two, and less than 128 sectors (else it gets a
 * divide overflow) */


#define FAT_SIZE(bits, sec_siz, clusters) \
	((((clusters)+2) * ((bits)/4) - 1) / 2 / (sec_siz) + 1)

#define NEEDED_FAT_SIZE(x) FAT_SIZE((x)->fat_bits, (x)->sector_size, \
				    (x)->num_clus)

/* disk size taken by FAT and clusters */
#define DISK_SIZE(bits, sec_siz, clusters, n, cluster_size) \
	((n) * FAT_SIZE(bits, sec_siz, clusters) + \
	 (clusters) * (cluster_size))

#define TOTAL_DISK_SIZE(bits, sec_siz, clusters, n, cluster_size) \
	(DISK_SIZE(bits, sec_siz, clusters, n, cluster_size) + 2)
/* approx. total disk size: assume 1 boot sector and one directory sector */

extern const char *mversion;
extern const char *mdate;
extern const char *mformat_banner;

extern char *Version;
extern char *Date;


int init(char drive, int mode);

#define MT_READ 1
#define MT_WRITE 2

#endif

