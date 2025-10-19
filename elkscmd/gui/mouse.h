#ifndef MOUSE_H
#define MOUSE_H

/* mouse.h */

/* Microsoft, PC/Logitch and PS/2 mouse decoding */
int open_mouse(void);
int read_mouse(int *dx, int *dy, int *dw, int *bp);
void close_mouse(void);

typedef unsigned short MWIMAGEBITS; /* bitmap image unit size*/

/* software cursor structure*/
struct cursor {
    int         width;          /* cursor width in pixels*/
    int         height;         /* cursor height in pixels*/
    int         hotx;           /* relative x pos of hot spot*/
    int         hoty;           /* relative y pos of hot spot*/
    int         fgcolor;        /* foreground hardware pixel value*/
    int         bgcolor;        /* background hardware pixel value*/
    MWIMAGEBITS *image;         /* cursor image bits*/
    MWIMAGEBITS *mask;          /* cursor mask bits*/
};

/* cursor display handling routines */
void initcursor(void);
void movecursor(int newx, int newy);
void setcursor(struct cursor *pcursor);
int showcursor(void);
int hidecursor(void);
void checkcursor(int x1,int y1,int x2,int y2);
void erasecursor(void);
void fixcursor(void);

/* Ascii cursor definition helper macros */
#define _       ((unsigned) 0)          /* off bits */
#define X       ((unsigned) 1)          /* on bits */
#define mask(a,b,c,d,e,f,g,h)                                                           \
   ((((((((((((((                                                                       \
   (a * 2) + b) * 2) + c) * 2) + d) * 2) + e) * 2) + f) * 2) + g) * 2) + h) << 8)

#define MASK(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p)                                           \
    (((((((((((((((((((((((((((((                                                       \
    (a * 2) + b) * 2) + c) * 2) + d) * 2) + e) * 2) + f) * 2) + g) * 2) + h) * 2)       \
            + i) * 2) + j) * 2) + k) * 2) + l) * 2) + m) * 2) + n) * 2) + o) * 2) + p)

extern struct cursor cursor_lg;
extern struct cursor cursor_sm;
#endif
