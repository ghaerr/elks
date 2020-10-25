/*
 * fdtest - A program to test floppy disk I/O speed in user space
 */
#include <stdio.h>
#include <time.h>
#include <linuxmt/bioshd.h>
#include <linuxmt/biosparm.h>
#include <linuxmt/memory.h>

struct biosparms bdt;

/* Useful defines for accessing the above structure. */
#define CARRY_SET (bdt.fl & 0x1)
#define BD_IRQ bdt.irq
#define BD_AX bdt.ax
#define BD_BX bdt.bx
#define BD_CX bdt.cx
#define BD_DX bdt.dx
#define BD_SI bdt.si
#define BD_DI bdt.di
#define BD_ES bdt.es
#define BD_FL bdt.fl

#define MAX		18		/* # sectors per track*/
#define SECSIZE		512		/* sector size*/

unsigned char user_buf[MAX * SECSIZE];
unsigned char kernel_buf[MAX * SECSIZE];

void read_disk(unsigned short drive, unsigned short cylinder, unsigned short head,
	unsigned short sector, unsigned short count)
{
	if (count > MAX) count = MAX;

	/* BIOS disk read into "kernel" buffer*/
	BD_AX = BIOSHD_READ | count;
	BD_BX = _FP_OFF(kernel_buf);	/* note: 64k DMA boundary ignored here*/
	BD_ES = _FP_SEG(kernel_buf);
	BD_CX = (unsigned short) ((cylinder << 8) | ((cylinder >> 2) & 0xc0) | sector);
	BD_DX = (head << 8) | drive;
	if (call_bios(&bdt))
		printf("READ ERROR at CHS %d,%d,%d\n", cylinder, head, sector);

	/* then memcpy to "user" buffer*/
	fmemcpyw(user_buf, _FP_SEG(user_buf), kernel_buf, _FP_SEG(kernel_buf),
		(count * SECSIZE) >> 1);
}

int main(int ac, char **av)
{
	unsigned short drive, cylinder, head, sector;
	unsigned short start_dma_page, end_dma_page;
	unsigned long normalized_seg;
	int i;
	time_t t;

	/* setup starting CHS*/
	drive = 0;		/* 0-1*/
	cylinder = 3;		/* 1-79*/
	head = 0;		/* 0-1*/
	sector = 1;		/* 1-18 on 1.44M floppies*/

	/* dma start/end pages must be equal to ensure on 64k DMA boundary for INT 13h*/
	start_dma_page = (_FP_SEG(kernel_buf) + ((__u16) kernel_buf >> 4)) >> 12;
	end_dma_page = (_FP_SEG(kernel_buf) + ((__u16) (kernel_buf + MAX * 512 - 1) >> 4)) >> 12;
	normalized_seg = (((__u32)_FP_SEG(kernel_buf) + (_FP_OFF(kernel_buf) >> 4)) << 16) +
		(_FP_OFF(kernel_buf) & 15);
	printf("dma start page %x, dma end page %x, buffer at %x:%x - ",
		start_dma_page, end_dma_page, _FP_SEG(normalized_seg), _FP_OFF(normalized_seg));
	printf(start_dma_page == end_dma_page? "OK\n": "ERROR\n");

	printf("Reading %d sectors individually\n", MAX);
	t = time(NULL);
	for (i=0; i < MAX; i++)
		read_disk(drive, cylinder, head, sector+i, 1);
	printf("%ld secs\n", time(NULL) - t);
	
	printf("Reading %d sectors as %d 1024-byte blocks\n", MAX, MAX/2);
	t = time(NULL);
	for (i=0; i < MAX; i += 2)
		read_disk(drive, cylinder, head, sector+i, 2);
	printf("%ld secs\n", time(NULL) - t);
	
	printf("Reading %d sectors at once\n", MAX);
	t = time(NULL);
	read_disk(drive, cylinder, head, sector, MAX);
	printf("%ld secs\n", time(NULL) - t);
}
