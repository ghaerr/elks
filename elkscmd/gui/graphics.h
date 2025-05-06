/* graphics.h */

#define PALMODE             0           /* =1 for 320x200 palette mode graphics */

/* supported graphics modes in graphics_open */
#define VGA_640x350x16      0x10        /* 640x350 16 color/4bpp */
#define VGA_640x480x16      0x12        /* 640x480 16 color/4bpp */
#define PAL_320x200x256     0x13        /* 320x200 256 color/8bpp */
#define TEXT_MODE           0x03        /* 80x25 text mode */

extern int SCREENWIDTH;                 /* set by graphics_open */
extern int SCREENHEIGHT;

/* start/stop graphics mode */
int graphics_open(int mode);
void graphics_close(void);
int draw_bmp(char *path, int x, int y);
int save_bmp(char *pathname);

#if defined(__C86__) && PALMODE             /* use C pal_ routines */
#define drawpixel(x,y,c)        pal_drawpixel(x,y,c)
#define drawhline(x1,x2,y,c)    pal_drawhline(x1,x2,y,c)
#define drawvline(x,y1,y2,c)    pal_drawvline(x,y1,y2,c)
#define readpixel(x,y)          pal_readpixel(x,y)
#elif defined(__C86__) || defined(__ia16__)   /* use ASM vga_ routines */
#define drawpixel(x,y,c)        vga_drawpixel(x,y,c)
#define drawhline(x1,x2,y,c)    vga_drawhline(x1,x2,y,c)
#define drawvline(x,y1,y2,c)    vga_drawvline(x,y1,y2,c)
#define readpixel(x,y)          vga_readpixel(x,y)
unsigned short cmp8(int x, int y, int target_color);
#else
void drawpixel(int x,int y, int color);
void drawhline(int x1, int x2, int y, int c);
void drawvline(int x, int y1, int y2, int c);
int readpixel(int x, int y);
#endif
void fillrect(int x1, int y1, int x2, int y2, int c);

/* VGA 16 color, 4bpp routines implemented in ASM in vga-ia16.S and vga-c86.s */
void vga_init(void);
void vga_drawpixel(int x, int y, int c);
void vga_drawhline(int x1, int x2, int y, int c);
void vga_drawvline(int x, int y1, int y2, int c);
int  vga_readpixel(int x, int y);
void vga_drawcursor(int x, int y, int height, unsigned short *mask);

/* ia16 only fast scanline blit */
void vga_drawscanline(unsigned char *colors, int x, int y, int length);

/* PAL 256 color 8bpp routines */
void pal_drawpixel(int x, int y, int color);
void pal_drawhline(int x1, int x2, int y, int c);
void pal_drawvline(int x, int y1, int y2, int c);
int  pal_readpixel(int x, int y);

void fdstmemcpy(unsigned dst_off, unsigned dst_seg, void *src_off, unsigned count);

unsigned int strtoi(const char *s, int base);

/* hardware pixel values (VGA 4bpp) */
#define BLACK   0
#define GRAY    8
#define WHITE   15
