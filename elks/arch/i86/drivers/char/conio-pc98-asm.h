/* conio functions for PC-98 */

/* put character on text vram */
void early_putchar(int c);

/* get character data from keyboard */
int bios_getchar(void);

/* get arrow key data */
int bios_getarrow(void);

/* get roll key data */
int bios_getroll(void);

/* initialize keyboard interface */
void bios_getinit(void);

/* read text vram address */
int read_tvram_x(void);

/* write text vram address */
void write_tvram_x(int tvram_x);

/* clear text vram */
void clear_tvram(void);

/* cursor on */
void cursor_on(void);

/* cursor off */
void cursor_off(void);

/* set cursor address */
void cursor_set(int cursor);
