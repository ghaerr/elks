/*
 * Query the IDE/ATA drive for physical data
 * Added by HS - sep-2020
 *
 * ELKS: ported from directhd.c 
 */

#include <linuxmt/config.h>
#include <linuxmt/init.h>
#include <linuxmt/kernel.h>
#include <linuxmt/heap.h>
#include <arch/ports.h>
#include <arch/io.h>
#include "blk.h"

#define IDE_DRIVE_ID	0xec	/* IDE command to get access to drive data */
#define IDE_STATUS	7	/* get drive status */
#define WAIT_READY      (30*HZ/100)     /* 30/100 sec = 300msec */

#define IDE_DEBUG	0	/* IDE probe debugging */

/* For IDE/ATA access */
#define STATUS(port)	inb_p(port + IDE_STATUS)
#define WAITING(port)	(STATUS(port) & 0x80) == 0x80

#if IDE_DEBUG
#define ide_debug   printk
#else
#define ide_debug(...)
#endif

static int io_ports[2] = { HD1_PORT, HD2_PORT };	/* physical port addresses */

static void INITPROC out_hd(int drive, word_t cmd)
{
    word_t port = io_ports[drive >> 1];

    port += 6;
    outb_p(0xA0 | ((drive & 1) << 4), port);
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

int INITPROC get_ide_data(int drive, struct drive_infot *drivep) {

	word_t port = io_ports[drive >> 1];
	int retval = -1, i;

	word_t *ide_buffer = (word_t *)heap_alloc(512, HEAP_TAG_DRVR);
	if (!ide_buffer) return -1;

	while (1) {
	    unsigned long timeout = jiffies + WAIT_READY;
	    out_hd(drive, IDE_DRIVE_ID);
	    while ((STATUS(port) & 0x80) == 0x80) {     /* wait 300ms until not busy */
		if (time_after(jiffies, timeout))
		    goto out;				/* timeout */
	    }

	    i = STATUS(port);
	    if (!i || (i & 1) == 1) {
		/* Error - drive not found or non-IDE.
		 * NOTE: If drive # is 2 (i.e. 2nd controller) is missing,
		 * there may be a 3rd (slave) drive.
		 */
		ide_debug("hd%c: drive at port 0x%x not found\n", 'a'+drive, port);
		break;
	    }
	    insw(port, kernel_ds, ide_buffer, 512/2);	/* read - word size */
	    /*
	     * Sanity check: Head, cyl and sector values must be other than
	     * 0 and buffer has to contain valid data (first entry in buffer
	     * must be != 0).
	     *
	     * Using 'actual' CHS values from the ID data (words 54, 55 & 56), 
	     * considered more reliable than 'default' CHS values (words 1, 3 & 6).
	     * The difference is that some devices have selectable translation modes,
	     * which is reflected in 'actual' values. ELKS does not use translation 
	     * modes, so they are always the same.
	     * Check for large # of cyls to detect (and skip) ATAPI CDROMs. 
	     * 							HS sep2020
	     */

	    /* this is the real sanity check */
	    if ((ide_buffer[1] < 34096) && (*ide_buffer != 0)
	    	&& (ide_buffer[1] != 0) && (ide_buffer[3] != 0) && (ide_buffer[3] < 256)
	    	&& (ide_buffer[6] != 0) && (ide_buffer[6] < 64)) {
#if IDE_DEBUG
	    	ide_buffer[20] = 0; /* String termination */
	    	printk(" IDE default CHS: %d/%d/%d serial %s\n", ide_buffer[1], ide_buffer[3],
			ide_buffer[6], &ide_buffer[10]);
#endif
		if (ide_buffer[53]&1) {	/* if set, the 'actual' data are valid */
					/* Old IDE drives don't support 'actual data' */
	    	    drivep->cylinders = ide_buffer[54];
	    	    drivep->heads = ide_buffer[55];
	    	    drivep->sectors = ide_buffer[56];
		} else {
	    	    drivep->cylinders = ide_buffer[1];
	    	    drivep->heads = ide_buffer[3];
	    	    drivep->sectors = ide_buffer[6];
		}
		ide_debug("hd%c: %d heads, %d cylinders, %d sectors\n", 'a'+drive,
		    drivep->heads, drivep->cylinders, drivep->sectors);
		retval = 0;
	    } else {
		retval = -2;
		ide_debug("hd%c: Error in IDE device data.\n", 'a'+drive);
	    }
	    break;
	}
out:
#if IDE_DEBUG
	if (retval != -1) dump_ide(ide_buffer, 128);
#endif
	heap_free(ide_buffer);

	return retval;
}
