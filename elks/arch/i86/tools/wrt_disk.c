/* This tool copies a Minix Disk image to the 'B' Flash SSD. The original
 * assumed  that the disk is a 128K Flash and that the source file for the
 * image was LOC::M:\MINIX.DSK but this version draws both parameters from
 * a separate header file where they can be configured.
 *
 * It is rather slow at present but uses the SSD driver stuff so should 
 * pick up any improvements as that developes.
 */

#include "tools.h"

/* Select method to use - uncomment the following line to use a single long
 * int for the address, or comment it to use two short ints instead.
 */

#define LONG_INT
/****/

/* Pseudotypes we use internally */

#define DWORD	unsigned long int
#define WORD	unsigned short int
#define BYTE	unsigned char

/* Common values generated from the parameters */

#define BLOCKS	((WORD) ((DWORD) DISK_SIZE) / 64UL)
#define SIZE	(((DWORD) DISK_SIZE) * 1024UL)

/* Prototypes for local functions */

void open_file(void);
BYTE read_byte(void);
void close_file(void);

char filename[64];

void print_word(WORD data)
{
    LCD_WriteChar('0' + ((data & 0xF000) >> 12));
    LCD_WriteChar('0' + ((data & 0x0F00) >>  8));
    LCD_WriteChar('0' + ((data & 0x00F0) >>  4));
    LCD_WriteChar('0' +  (data & 0x000F)       );
}

void print_address(WORD high, WORD low)
{
    print_word(high);
    print_word(low);
    LCD_WriteChar(' ');
}

void error(char Pass, WORD data, WORD result)
{
    LCD_WriteChar('E');
    LCD_WriteChar('r');
    LCD_WriteChar('r');
    LCD_WriteChar('o');
    LCD_WriteChar('r');
    LCD_WriteChar(' ');
    LCD_WriteChar(Pass);
    LCD_WriteChar(' ');
    print_word(data);
    LCD_WriteChar('/');
    print_word(result);
    LCD_WriteChar(' ');

    /* Loop forever (requires hard reset) */
    while (1)
	/* Do nothing */;
}

void strcat(char *tgt, char *src)
{
    while (*tgt)
	tgt++;
    while (*src)
	*tgt++ = *src++;
    *tgt = *src;
}

/* NOTE: Each time this is run it erases the SSD */
int main(void)
{
#ifdef LONG_INT
    DWORD address;
#endif
    WORD high_address, low_address;
    BYTE data, result;
#ifndef LONG_INT
    BYTE not_done;
#endif

    /* Specify the filename to use */
    *filename = '\0';
    strcat(filename, "loc::m:\\");
    filename[5] = DRIVE;
    strcat(filename,FILENAME);

    /* Connect to SSD in slot 'B' (under 'enter') */
    ssd_open4(0x01);

    /* pre-program all bits to zero */
    LCD_Position(0, 0);
    LCD_WriteChar('P');
    LCD_WriteChar('r');
    LCD_WriteChar('e');
    LCD_WriteChar('P');
    LCD_WriteChar('r');
    LCD_WriteChar('o');
    LCD_WriteChar('g');

#ifdef LONG_INT

    for (address = 0; address < SIZE; address++) {
	high_address = (WORD) (address >> 16);
	low_address = (WORD) (address & 0xFFFF);
	if ((low_address & 0x1F) == 0) {
	    LCD_Position(0, 1);
	    print_address(high_address, low_address);
	}
	result = ssd_write4(high_address, low_address, 0);
	if (result) {
	    LCD_Position(0, 2);
	    error('1', data, result);
	}
    }

#else

    /* for block of 64K write data */
    for (high_address = 0; high_address < BLOCKS; high_address++) {
	low_address = 0;
	not_done = 1;

	while (not_done) {
	    if ((low_address & 0x1F) == 0) {
		LCD_Position(0, 1);
		print_address(high_address, low_address);
	    }

	    result = ssd_write4(high_address, low_address, 0);

	    if (result) {
		LCD_Position(0, 2);
		error('1', data, result);
	    }

	    low_address++;
	    if (low_address == 0)
		not_done = 0;
	}
    }

#endif

    /* need to erase each chip in the SSD */
    LCD_Position(0, 0);
    LCD_WriteChar('E');
    LCD_WriteChar('r');
    LCD_WriteChar('a');
    LCD_WriteChar('s');
    LCD_WriteChar('e');
    LCD_WriteChar(' ');
    LCD_WriteChar(' ');

    /* for block of 64K write data */
    for (high_address = 0; high_address < BLOCKS; high_address++) {
	LCD_Position(0, 1);
	print_address(high_address, 0);

	result = ssd_erase4(0);

	if (result) {
	    LCD_Position(0, 2);
	    error('2',high_address, result);
	}
    }

    open_file();

    /* lets create the disk image */
    LCD_Position(0, 0);
    LCD_WriteChar('P');
    LCD_WriteChar('r');
    LCD_WriteChar('o');
    LCD_WriteChar('g');
    LCD_WriteChar('r');
    LCD_WriteChar('a');
    LCD_WriteChar('m');

#ifdef LONG_INT

    for (address = 0; address < SIZE; address++) {
	high_address = (unsigned short int) (address >> 16);
	low_address = (unsigned short int) (address & 0xFFFF);
	if ((low_address & 0x1F) == 0) {
	    LCD_Position(0, 1);
	    print_address(high_address, low_address);
	}
	data = read_byte();
	result = ssd_write4(high_address, low_address, data);
	if (result != data) {
	    LCD_Position(0, 2);
	    error('3', data, result);
	}
    }

#else

    /* for block of 64K write data */
    for (high_address = 0; high_address < BLOCKS; high_address++) {
	low_address = 0;
	not_done = 1;

	while (not_done) {
	    data = read_byte();
	    if ((low_address & 0x1F) == 0) {
		LCD_Position(0, 1);
		print_address(high_address, low_address);
	    }

	    LCD_WriteChar(data);

	    result = ssd_write4(high_address, low_address, data);

	    if (result != data) {
		LCD_Position(0, 2);
		error('3', data, result);
	    }

	    low_address++;
	    if (low_address == 0)
		not_done = 0;
	}
    }

#endif

    close_file();

    LCD_Position(0, 2);
    LCD_WriteChar('D');
    LCD_WriteChar('o');
    LCD_WriteChar('n');
    LCD_WriteChar('e');
    LCD_WriteChar('.');

    /* Sleep forever - requires a hard reset */
    while (1)
	/* Do nothing */;

    return 0;
}
