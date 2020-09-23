/*
 * Query the IDE/ATA drive for physical data
 * Added by HS - sep-2020
 *
 * ELKS: ported from directhd.c 
 */

#include <linuxmt/init.h>
#include <linuxmt/kernel.h>
#include <linuxmt/heap.h>
#include <arch/ports.h>
#include <arch/io.h>
#include "blk.h"

#define IDE_DRIVE_ID	0xec	/* IDE command to get access to drive data */
#define IDE_STATUS	7	/* get drive status */
#define IDE_ERROR	1

#define IDE_DEBUG	0	/* IDE probe debugging */

/* For IDE/ATA access */
#define STATUS(port)	inb_p(port + IDE_STATUS)
#define ERROR(port)	inb_p(port + IDE_ERROR)
#define WAITING(port)	(STATUS(port) & 0x80) == 0x80

#if IDE_DEBUG
#define debug   printk
#else
#define debug(...)
#endif

static int io_ports[2] = { HD1_PORT, HD2_PORT };	/* physical port addresses */

static void INITPROC insw(word_t port, word_t *buffer, int count) {

    do {
   	*buffer++ = inw(port);
    } while (--count);
}

static void INITPROC out_hd(int drive, word_t cmd)
{
    word_t port = io_ports[drive >> 1];

    outb(0xff, ++port);		/* Feature set, should not matter */
    outb_p(0, ++port);
    outb_p(0, ++port);
    outb_p(0, ++port);
    outb_p(0, ++port);
    outb_p(0xA0 | ((drive & 1) << 4), ++port);
    outb_p(cmd, ++port);
}

#if IDE_DEBUG
static void INITPROC dump_ide(word_t *buffer, int size) {
	int counter = 0;

	do {
		printk("%04X ", *buffer++);
		if (++counter == 16) {
			printk("\n");
			counter = 0;
		}
	} while (--size);
	if (counter) printk("\n");
}
#endif

/* Collect useful info from IDE/ATA drives via the IDE ID cmd.
 * Don't call this function if arch < 5.
 * Could detect ATAPI drives here (extreme head count).
 */

int INITPROC get_ide_data(int drive, struct drive_infot *drive_info) {

	word_t port = io_ports[drive >> 1];
	int retval = 0;

	word_t *ide_buffer = (word_t *)heap_alloc(512, 0);

	while (1) {
	    out_hd(drive, IDE_DRIVE_ID);
    	    while (WAITING(port));

	    if ((STATUS(port) & 1) == 1) {
		/* Error - drive not found or non-IDE.
		 * If drive # is 2 it may actually be physical drive 3 -
		 * slave, not master. Take another round to check.
		 */
		if (drive == 2) {
		    drive++;
		    continue;
		}
		debug("bd%c: drive at port 0x%x not found\n", 'a'+drive, port);

		retval = -1;
		break;
	    }

	    insw(port, ide_buffer, 512/2);	/* read - word size */

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
	     * Check for large # of cyls to detect (and skip) ATAPI CDROMs. 
	     * 							HS sep2020
	     */

	    if ((ide_buffer[54] < 34096) && (*ide_buffer != 0)	/* this is the real sanity check */
	    	&& (ide_buffer[54] != 0) && (ide_buffer[55] != 0) && (ide_buffer[55] < 256)
	    	&& (ide_buffer[56] != 0) && (ide_buffer[56] < 64)) {
#if IDE_DEBUG
	    	ide_buffer[20] = 0; /* String termination */
	    	printk("IDE default CHS: %d/%d/%d serial %s\n", ide_buffer[1], ide_buffer[3], ide_buffer[6],
			&ide_buffer[10]);
#endif
	    	drive_info->cylinders = ide_buffer[54];
	    	drive_info->heads = ide_buffer[55];
	    	drive_info->sectors = ide_buffer[56];

		/* Note: If the 3rd drive is slave, not master, it will be reported as
		 * bdd, not bdc here. */
		debug("bd%c: %d heads, %d cylinders, %d sectors\n", 'a'+drive,
			drive_info->heads, drive_info->cylinders, drive_info->sectors);
	    } else {
		retval = -2;
#if IDE_DEBUG
		printk("bd%c: Error in IDE device data.\n", 'a'+drive);
		dump_ide(ide_buffer, 256);
#endif
	    }
	    break;
	}
	heap_free(ide_buffer);

	return retval;
}
