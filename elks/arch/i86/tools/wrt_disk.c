/* This tool copies a Minix Disk image to the 'B' Flash SSD, it assumes
 * that the disk is a 128K Flash and the source file for the image is
 * LOC::M:\MINIX.DSK
 * It is rather slow at present but uses the SSD driver stuff so should 
 * pick up any improvements as that developes.
 */

/* NOTE: Each time this is run it erases the SSD */

#define BLOCKS 2 /* set for 128K SSD, 1 Block = 64K */

main()
{
	unsigned int high_address, low_address;
	unsigned char data, result, not_done;

	/* Connect to SSD in slot 'B' (under 'enter') */
	ssd_open4(0x01);
	
	/* pre-program all bits to zero */
	LCD_Position(0,0);
	LCD_WriteChar('P');
	LCD_WriteChar('r');
	LCD_WriteChar('e');
	LCD_WriteChar('P');
	LCD_WriteChar('r');
	LCD_WriteChar('o');
	LCD_WriteChar('g');

	/* for block of 64K write data */
	for (high_address = 0; high_address < BLOCKS ; high_address ++) {
		low_address = 0;
		not_done = 1;

		while (not_done) {
			if ( (low_address & 0x1F) == 0) {
				LCD_Position(0,1);
				print_address(high_address, low_address);
			}

			result = ssd_write4(high_address, low_address, 0);

			if (result != data) {
				LCD_Position(0,2);
				error(data, result);
			}

			low_address ++;
			if (low_address == 0) not_done = 0;	
		}
	}
	

	/* need to erase each chip in the SSD */
	LCD_Position(0,0);
	LCD_WriteChar('E');
	LCD_WriteChar('r');
	LCD_WriteChar('a');
	LCD_WriteChar('s');
	LCD_WriteChar('e');
	LCD_WriteChar(' ');
	LCD_WriteChar(' ');
	
	/* for block of 64K write data */
	for (high_address = 0; high_address < BLOCKS ; high_address ++) {
		LCD_Position(0,1);
		print_address(high_address, 0);

		result = ssd_erase4(0);

		if ( result ) {
			LCD_Position(0,2);
			error(high_address, result);
		}
	}
	
	open_file();

	/* lets create the disk image */
	LCD_Position(0,0);
	LCD_WriteChar('P');
	LCD_WriteChar('r');
	LCD_WriteChar('o');
	LCD_WriteChar('g');
	LCD_WriteChar('r');
	LCD_WriteChar('a');
	LCD_WriteChar('m');

	/* for block of 64K write data */
	for (high_address = 0; high_address < BLOCKS ; high_address ++) {
		low_address = 0;
		not_done = 1;

		while (not_done) {
			data = read_byte();
			if ( (low_address & 0x1F) == 0) {
				LCD_Position(0,1);
				print_address(high_address, low_address);
			}

			LCD_WriteChar(data);

			result = ssd_write4(high_address, low_address, data);

			if (result != data) {
				LCD_Position(0,2);
				error(data, result);
			}

			low_address ++;
			if (low_address == 0) not_done = 0;	
		}
	}

	close_file();

	LCD_Position(0,2);
	LCD_WriteChar('D');
	LCD_WriteChar('o');
	LCD_WriteChar('n');
	LCD_WriteChar('e');
	LCD_WriteChar('.');

	/* Sleep for every - requires a hard reset */
	while(1);

	return(0);
}

int print_address(high, low)
int high;
int low;
{
	/* Print out the address at the current location */
	LCD_WriteChar( '0' + ((high & 0xF000) >> 12));
	LCD_WriteChar( '0' + ((high & 0x0F00) >> 8));
	LCD_WriteChar( '0' + ((high & 0x00F0) >> 4));
	LCD_WriteChar( '0' + (high & 0x000F) );
	
	LCD_WriteChar( '0' + ((low & 0xF000) >> 12));
	LCD_WriteChar( '0' + ((low & 0x0F00) >> 8));
	LCD_WriteChar( '0' + ((low & 0x00F0) >> 4));
	LCD_WriteChar( '0' + (low & 0x000F) );

	LCD_WriteChar( ' ' );
}

int error(data, result)
int data;
int result;
{
	LCD_WriteChar('E');
	LCD_WriteChar('r');
	LCD_WriteChar('r');
	LCD_WriteChar('o');
	LCD_WriteChar('r');

	LCD_WriteChar(' ');
	print_address(data, result);

	while (1);
}

