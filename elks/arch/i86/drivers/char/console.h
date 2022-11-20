/* console routine forward definitions*/

void Console_conin(unsigned char Key);
void Console_conout(dev_t dev, int Ch);
extern struct tty ttys[];

void kbd_init(void);
extern char kbd_name[];

void soundp(unsigned);
void nosound(void);
void bell(void);

/* for direct and bios consoles only*/
void Console_set_vc(int N);
