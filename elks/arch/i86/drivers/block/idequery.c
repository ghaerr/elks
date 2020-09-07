
/*
 * Query the IDE/ATA drive for physical data
 * Added by HS - sep-2020
 *
 * ELKS: ported from directhd.c 
 */

#include <linuxmt/kernel.h>
#include <arch/io.h>
#include <linuxmt/heap.h>
#include <arch/ports.h>

#define IDE_DRIVE_ID	0xec	/* IDE command to get access to drive data */
#define IDE_STATUS	7	/* get drive status */
#define IDE_ERROR	1

#define IDE_DEBUG	1	/* IDE probe debugging */

/* For IDE/ATA access */
#define STATUS(port) inb_p(port + IDE_STATUS)
#define ERROR(port) inb_p(port + IDE_ERROR)
#define WAITING(port) (STATUS(port) & 0x80) == 0x80

static int io_ports[2] = { HD1_PORT, HD2_PORT };	/* physical port addresses */

/* FIXME: should be moved to a header file */
struct drive_infot {		/* CHS per drive*/
    int cylinders;
    int sectors;
    int heads;
    int fdtype;			/* floppy fd_types[] index  or -1 if hd */
};

void insw(word_t port, word_t *buffer, int count) {

    while (--count) {
   	*buffer++ = inw(port);
    }
}

void out_hd(word_t drive, word_t cmd)
{
    word_t port;

    port = io_ports[drive / 2];

    outb(0xff, ++port);		/* Feature set, should not matter */
    outb_p(0, ++port);
    outb_p(0, ++port);
    outb_p(0, ++port);
    outb_p(0, ++port);
    outb_p(0xA0 | ((drive % 2) << 4), ++port);
    outb_p(cmd, ++port);

    /* wait for ready */
    while (WAITING(port-7));
}

#if 0
void out_hd(word_t drive, word_t nsect, word_t sect,
	    word_t head, word_t cyl, word_t cmd)
{
    word_t port;

    port = io_ports[drive / 2];

    outb(0xff, ++port);		/* the supposedly correct value for WPCOM on IDE */
    outb_p(nsect, ++port);
    outb_p(sect, ++port);
    outb_p(cyl, ++port);
    outb_p(cyl >> 8, ++port);
    outb_p(0xA0 | ((drive % 2) << 4) | head, ++port);
    outb_p(cmd, ++port);

}
#endif

#if IDE_DEBUG
static void dump_ide(word_t *buffer, int size) {
	int counter = 0;

	while(--size) {
		printk("%04X ", *buffer++);
		if (++counter == 16) {
			printk("\n");
			counter = 0;
		}
	}
	printk("\n");
}
#endif

/* Collect useful info from IDE/ATA drives via the IDE ID cmd.
 * Don't call this function if arch < 5.
 * Could detect ATAPI drives here (extreme head count).
 */

static word_t ide_buffer[256];
int get_ide_data(int drive, struct drive_infot *drive_info) {

	int port = io_ports[drive / 2];
	int retval = 0;

#if IDE_DEBUG
	printk("get_ide_data: drive %i at 0x%3X\n", drive, port);
#endif
	//word_t *ide_buffer = (word_t *)heap_alloc(512, 0);

	out_hd(drive, IDE_DRIVE_ID);
	//out_hd(drive, 0, 0, 0, 0, IDE_DRIVE_ID);

	while (1) {
	    if ((STATUS(port) & 1) == 1) {
		/* error - drive not found or non-ide */
#if IDE_DEBUG
		printk("bd%s: drive at port 0x%x not found\n", 'a'+drive, port);
#endif
		retval = -1;
		break;
	    }

	    insw(drive, ide_buffer, 512/2);	/* read - word size */

	    /*
	     * Sanity check: Head, cyl and sector values must be other than
	     * 0 and buffer has to contain valid data (first entry in buffer
	     * must be != 0).
	     *
	     * Using 'current' CHS values from the ID data (words 54, 55 & 56), 
	     * considered more reliable than 'default' CHS values (words 1, 3 & 6).
	     * The difference is that some devices have selectable translation modes,
	     * which is reflected in 'current' values. ELKS does not use translation 
	     * modes, so they are always the same.
	     * Check for large # of heads to detect ATPI CDROMs.
	     * 							HS sep2020
	     */
	    if ((ide_buffer[54] < 34096) && (*ide_buffer != 0)	/* this is the real sanity check */
	    	&& (ide_buffer[54] != 0) && (ide_buffer[55] != 0)
	    	&& (ide_buffer[56] != 0)) {
#if IDE_DEBUG
	    	ide_buffer[20] = 0;
	    	printk("IDE default CHS: %d/%d/%d serial %s\n", ide_buffer[1], ide_buffer[3], ide_buffer[6],
			&ide_buffer[10]);
#endif
	    	drive_info->cylinders = ide_buffer[54];
	    	drive_info->heads = ide_buffer[55];
	    	drive_info->sectors = ide_buffer[56];
#if IDE_DEBUG
	    	printk("bd%c: %d heads, %d cylinders, %d sectors\n", 'a'+drive,
		   drive_info->heads, drive_info->cylinders, drive_info->sectors);
#endif
	    } else {
		retval = -2;
		printk("bd%c: Error in IDE device data.\n", 'a'+drive);
#if IDE_DEBUG
		dump_ide(ide_buffer, 256);
#endif
	    }
	    break;
	}

	//heap_free(ide_buffer);

	return retval;
}
