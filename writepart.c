/* write partition table to MBR boot sector - TEMP CODE*/
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

typedef unsigned long sector_t;
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

struct partition part;
struct partition *p = &part;
unsigned short bootmagic = 0xAA55;

int main(int argc, char **argv)
{
	if (argc != 2) {
		printf("Usage %s <MBR HD image\n", argv[0]);
		exit(1);
	}
	int fd = open(argv[1], O_RDWR);
	if (fd < 0) perror(argv[1]);
	lseek(fd, (long)510-64, SEEK_SET);
	read(fd, &part, sizeof(part));
	lseek(fd, (long)510-64, SEEK_SET);

#ifdef MSDOS622
	p->end_head = 16;
	p->end_sector = 63;
#endif

#ifdef MINIX
#define CYL	63
#define HEAD	16
#define SECT	63
	p->boot_ind = 0x80;
	p->head = 0;
	p->sector = 2;		/* next sector after MBR, non-standard*/
	p->cyl = 0;
	p->sys_ind = 0x80;	/* MINIX*/
	p->end_head = HEAD;
	p->end_sector = SECT | ((CYL >> 2) & 0xc0);
	p->end_cyl = CYL & 0xff;
	p->start_sect = 1;		/* zero-relative start sector here*/
	p->start_sect_hi = 0;
	p->nr_sects = CYL*HEAD*SECT;
	p->nr_sects_hi = 0;
#endif

	write(fd, &part, sizeof(part));
	lseek(fd, (long)510, SEEK_SET);
	write(fd, &bootmagic, sizeof(bootmagic));
	close(fd);
}
