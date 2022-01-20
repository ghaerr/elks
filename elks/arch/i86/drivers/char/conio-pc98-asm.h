/* conio functions for PC-98 */

void early_putchar(int c);
int bios_getchar();
int bios_getarrow();
int bios_getroll();
void bios_getinit();
int read_tvram_x();
void write_tvram_x(int tvram_x);
void clear_tvram();
void cursor_on();
void cursor_off();
void cursor_set(int cursor);
