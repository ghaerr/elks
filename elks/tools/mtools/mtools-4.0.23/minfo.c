/*  Copyright 1997-2003,2006,2007,2009 Alain Knaff.
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
 * mlabel.c
 * Make an MSDOS volume label
 */

#include "sysincludes.h"
#include "msdos.h"
#include "mainloop.h"
#include "vfat.h"
#include "mtools.h"
#include "nameclash.h"

static void usage(int ret) NORETURN;
static void usage(int ret)
{
	fprintf(stderr, 
		"Mtools version %s, dated %s\n", mversion, mdate);
	fprintf(stderr, 
		"Usage: %s [-v] drive\n", progname);
	exit(ret);
}


static void displayInfosector(Stream_t *Stream, union bootsector *boot)
{
	InfoSector_t *infosec;

	if(WORD(ext.fat32.infoSector) == MAX16)
		return;

	infosec = (InfoSector_t *) safe_malloc(WORD(secsiz));
	force_read(Stream, (char *) infosec, 
			   (mt_off_t) WORD(secsiz) * WORD(ext.fat32.infoSector),
			   WORD(secsiz));
	printf("\nInfosector:\n");
	printf("signature=0x%08x\n", _DWORD(infosec->signature1));
	if(_DWORD(infosec->count) != MAX32)
		printf("free clusters=%u\n", _DWORD(infosec->count));
	if(_DWORD(infosec->pos) != MAX32)
		printf("last allocated cluster=%u\n", _DWORD(infosec->pos));
}


static void displayBPB(Stream_t *Stream, union bootsector *boot) {
	struct label_blk_t *labelBlock;

	printf("bootsector information\n");
	printf("======================\n");
	printf("banner:\"%.8s\"\n", boot->boot.banner);
	printf("sector size: %d bytes\n", WORD(secsiz));
	printf("cluster size: %d sectors\n", boot->boot.clsiz);
	printf("reserved (boot) sectors: %d\n", WORD(nrsvsect));
	printf("fats: %d\n", boot->boot.nfat);
	printf("max available root directory slots: %d\n", 
	       WORD(dirents));
	printf("small size: %d sectors\n", WORD(psect));
	printf("media descriptor byte: 0x%x\n", boot->boot.descr);
	printf("sectors per fat: %d\n", WORD(fatlen));
	printf("sectors per track: %d\n", WORD(nsect));
	printf("heads: %d\n", WORD(nheads));
	printf("hidden sectors: %d\n", DWORD(nhs));
	printf("big size: %d sectors\n", DWORD(bigsect));

	if(WORD(fatlen)) {
		labelBlock = &boot->boot.ext.old.labelBlock;
	} else {
		labelBlock = &boot->boot.ext.fat32.labelBlock;
	}

	if(has_BPB4) {
		printf("physical drive id: 0x%x\n", 
		       labelBlock->physdrive);
		printf("reserved=0x%x\n", 
		       labelBlock->reserved);
		printf("dos4=0x%x\n", 
		       labelBlock->dos4);
		printf("serial number: %08X\n", 
		       _DWORD(labelBlock->serial));
		printf("disk label=\"%11.11s\"\n", 
		       labelBlock->label);
		printf("disk type=\"%8.8s\"\n", 
		       labelBlock->fat_type);
	}
		
	if(!WORD(fatlen)){
		printf("Big fatlen=%u\n",
		       DWORD(ext.fat32.bigFat));
		printf("Extended flags=0x%04x\n",
		       WORD(ext.fat32.extFlags));
		printf("FS version=0x%04x\n",
		       WORD(ext.fat32.fsVersion));
		printf("rootCluster=%u\n",
		       DWORD(ext.fat32.rootCluster));
		if(WORD(ext.fat32.infoSector) != MAX16)
			printf("infoSector location=%d\n",
			       WORD(ext.fat32.infoSector));
		if(WORD(ext.fat32.backupBoot) != MAX16)
			printf("backup boot sector=%d\n",
			       WORD(ext.fat32.backupBoot));
		displayInfosector(Stream, boot);
	}
}

void minfo(int argc, char **argv, int type UNUSEDP) NORETURN;
void minfo(int argc, char **argv, int type UNUSEDP)
{
	union bootsector boot;

	char name[EXPAND_BUF];
	int media;
	int haveBPB;
	int size_code;
	int i;
	struct device dev;
	char drive;
	int verbose=0;
	int c;
	Stream_t *Stream;
	int have_drive = 0;

	unsigned long sect_per_track;

	char *imgFile=NULL;
	
	if(helpFlag(argc, argv))
		usage(0);
	while ((c = getopt(argc, argv, "i:vh")) != EOF) {
		switch (c) {
			case 'i':
				set_cmd_line_image(optarg);
				imgFile=optarg;
				break;
			case 'v':
				verbose = 1;
				break;
			case 'h':
				usage(0);
			default:
				usage(1);
		}
	}

	for(;optind <= argc; optind++) {
		if(optind == argc) {
			if(have_drive)
				break;
			drive = get_default_drive();
		} else {
			if(!argv[optind][0] || argv[optind][1] != ':')
				usage(1);
			drive = ch_toupper(argv[optind][0]);
		}
		have_drive = 1;

		if(! (Stream = find_device(drive, O_RDONLY, &dev, &boot, 
					   name, &media, 0, NULL)))
			exit(1);

		haveBPB = media >= 0x100;
		media = media & 0xff;
		
		printf("device information:\n");
		printf("===================\n");
		printf("filename=\"%s\"\n", name);
		printf("sectors per track: %d\n", dev.sectors);
		printf("heads: %d\n", dev.heads);
		printf("cylinders: %d\n\n", dev.tracks);
		printf("media byte: %02x\n\n", media & 0xff);

		sect_per_track = dev.sectors * dev.heads;
		if(sect_per_track != 0) {
			unsigned int hidden;
			unsigned long tot_sectors;
			int tracks_match=0;
			printf("mformat command line: mformat ");

			if(haveBPB) {
				int sector_size;
				tot_sectors = DWORD_S(bigsect);
				SET_INT(tot_sectors, WORD_S(psect));
				sector_size = WORD_S(secsiz);
				size_code=2;
				for(i=0; i<7; i++) {
					if(sector_size == 128 << i) {
						size_code = i;
						break;
					}
				}
				if(media == 0xf0)
					hidden = DWORD_S(nhs);
				else
					hidden = 0;
			} else {
				tot_sectors = dev.tracks * sect_per_track;
				size_code=2;
				hidden = 0;
			}

			if(tot_sectors ==
			   dev.tracks * sect_per_track - hidden % sect_per_track) {
				tracks_match=1;
				printf("-t %d ", dev.tracks);
			} else {
				printf("-T %ld ", tot_sectors);
			}
			printf ("-h %d -s %d ", dev.heads, dev.sectors);
			if(haveBPB && (hidden || !tracks_match))
				printf("-H %d ", hidden);
			if(size_code != 2)
				printf("-S %d ",size_code);
			if(imgFile != NULL)
				printf("-i \"%s\" ", imgFile);
			printf("%c:\n", ch_tolower(drive));
			printf("\n");
		}

		if(haveBPB || verbose)
			displayBPB(Stream, &boot);

		if(verbose) {
			int size;
			unsigned char *buf;

			printf("\n");
			size = WORD_S(secsiz);
			
			buf = (unsigned char *) malloc(size);
			if(!buf) {
				fprintf(stderr, "Out of memory error\n");
				exit(1);
			}

			size = READS(Stream, buf, (mt_off_t) 0, size);
			if(size < 0) {
				perror("read boot sector");
				exit(1);
			}

			print_sector("Boot sector hexdump", buf, size);
		}
	}
	FREE(&Stream);
	exit(0);
}
