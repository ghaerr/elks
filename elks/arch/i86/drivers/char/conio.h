/* low level conio functions for BIOS Console and Headless Console*/

/* initialize console*/
void conio_init(void);

/*
 * Poll for console input available.
 * Return nonzero character received else 0 if none ready.
 */
int  conio_poll(void);

/* Output character to console*/
void conio_putc(byte_t ch);
