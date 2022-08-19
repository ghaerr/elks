/* display the matrix! */
/* ported from https://github.com/clydeshaffer/matrix_DOS */
/* July 2022 Greg Haerr */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/select.h>
#include <time.h>
#include "unikey.h"
#include "runes.h"

#define SCREENWIDTH     80
#define SCREENHEIGHT    25
#define DELAY           75

#define far

#define charat(x,y) txtmem[(((y)<<4)+((y)<<6)+(x))<<1]
#define colorat(x,y) txtmem[((((y)<<4)+((y)<<6)+(x))<<1) + 1]

char far *txtmem;

#if 0
void write_string(char* s, int x, int y, int len, char color) {
    char far *ptr = &charat(x,y);
    while(len) {
        *ptr = *s;
        ptr++;
        s++;
        *ptr=color;
        ptr++;
        len--;
    }
}

void draw_box(int x, int y, int width, int height, char color) {
    int i;
    width--;
    height--;
    for(i = 1; i < width; i++) {
        charat(x+i, y) = 0xCD;
        charat(x+i, y+height) = 0xCD;
        colorat(x+i, y) = color;
        colorat(x+i, y+height) = color;
    }
    for(i = 1; i < height; i++) {
        charat(x, y+i) = 0xBA;
        charat(x+width, y+i) = 0xBA;
        colorat(x, y+i) = color;
        colorat(x+width, y+i) = color;
    }
    charat(x, y) = 0xC9;
    charat(x+width, y) = 0xBB;
    charat(x, y+height) = 0xC8;
    charat(x+width, y+height) = 0xBC;
    colorat(x, y) = color;
    colorat(x+width, y) = color;
    colorat(x, y+height) = color;
    colorat(x+width, y+height) = color;
}
#endif

void fill_box(int x, int y, int width, int height, char ch, char color) {
    int i, j;
    for(i = 0; i < width; i++) {
        for(j = 0; j < height; j++) {
            charat(x+i, y+j) = ch;
            colorat(x+i, y+j) = color;
        }
    }
}

#if 0
void paint_box(int x, int y, int width, int height, char color) {
    int i, j;
    for(i = 0; i < width; i++) {
        for(j = 0; j < height; j++) {
            colorat(x+i, y+j) = color;
        }
    }
}

void clear_box(int x, int y, int width, int height) {
    int i, j;
    for(i = 0; i < width; i++) {
        for(j = 0; j < height; j++) {
            charat(x+i, y+j) = 0;
        }
    }
}
#endif

#if 0
static const int green_colors[16] = {
    16,     /* 0,0,0    0% */
    22,     /* 0,95,0   18%*/
    2,      /* 0,128,0  25%*/
    28,     /* 0,135,0  26%*/
    34,     /* 0,175,0  34%*/
    40,     /* 0,215,0  42%*/
    41,     /* 0,215,95  42%*/
    65,     /* 95,125,95    45% */
    46,     /* 0,255,0  50%*/
    77,     /* 95,215,95 60% */
    83,     /* 95,255,95 68% */
    114,    /* 135,255,135 68% */
    120,    /* 125,255,125 76% */
    157,    /* 175,255,175 84% */
    194,    /* 215,255,215 92% */
    231     /* 255,255,255 */
};
#else
static const int fg256_green[16] = {
    //16, 23, 22, 22, 28, 28, 34, 34, 40, 40, 46, 46, 82, 82, 231
    //16, 23, 22, 29, 28, 34, 35, 40, 41, 42, 46, 48, 49, 50, 231
    //16, 22, 23, 28, 29, 34, 35, 36, 40, 41, 42, 46, 47, 48, 49, 231
    //16, 22, 23, 28, 29, 34, 35, 40, 41, 42, 46, 47, 48, 49, 50, 231
    //16, 22, 23, 28, 29, 2, 34, 35, 40, 41, 42, 46, 47, 48, 49, 231
    16, 22, 23, 28, 29, 30, 2, 34, 35, 40, 41, 42, 46, 47, 48, 231
    //16, 23, 22, 29, 28, 34, 35, 36, 43, 40, 41, 42, 46, 47, 48, 231
    //16, 22, 23, 28, 29, 34, 35, 36, 43, 42, 40, 41, 46, 47, 48, 231
    //16, 23, 22, 29, 28, 34, 35, 36, 43, 40, 41, 42, 46, 47, 48, 231
    //16, 22, 28, 29, 28, 34, 35, 36, 43, 40, 41, 42, 46, 47, 48, 231
};
#endif

static const int fg16_green[16] = {
    30, 37, 32, 32, 32, 32, 32, 32, 32, 32, 92, 92, 92, 93, 93, 97
   //0,  7,  2,  2,  2,  2,  2,  2,  2,  2, 10, 10, 10, 14, 14, 15
};

#if 0
/* set up a 16-shade greenscale palette */
void green_palette() {
    unsigned char g, gstep;
    int i;
    outp(0x03c8, 0);

    g = 0x00;
    gstep = 0x3F >> 4;
    for(i = 0; i < 6; i++) {
        putGVal(g);
        g += gstep;
    }
    outp(0x03c8, 20);
    putGVal(g);
    g += gstep;
    outp(0x03c8, 7);
    putGVal(g);
    g += gstep;
    outp(0x03c8, 56);
    for(i = 0; i < 7; i ++) {
        putGVal(g);
        g += gstep;
    }
    /* head of the trail is bright white rather than green */
    outp(0x03c9, 0x3F);
    outp(0x03c9, 0x3F);
    outp(0x03c9, 0x3F);
}
#endif

int is_whitespace(unsigned char c) {
	return ((c == 0) || (c == 32) || (c == 255));
}

char rnd_printable() {
    //return 0xdb;
	unsigned char rndchr = (((unsigned char) rand()) % 253) + 1;
	if(rndchr >= 32){
		rndchr++;
	}
	return rndchr;
}

void step(int spawn) {
	int row, col;
	unsigned char currentColor;
	unsigned char aboveColor;
	unsigned char aboveChar;
	/* Fade every cell's color by 1 */
	for(row = 0; row < SCREENHEIGHT; row++) {
	 for(col = 0; col < SCREENWIDTH; col++) {
		currentColor = 0x0F & colorat(col, row);
		if(currentColor > 0) {
			colorat(col, row) = currentColor - 1;
		}
	 }
	}
	/* continue the motion of any cell that was maximally bright last frame */
	for(row = 1; row < SCREENHEIGHT; row++) {
	 for(col = 0; col < SCREENWIDTH; col++) {
		currentColor = 0x0F & colorat(col, row);
		aboveColor = 0x0F & colorat(col, row-1);
		aboveChar = charat(col, row-1);
		if((aboveColor == 0x0E) && !is_whitespace(aboveChar)) {
			colorat(col, row) = 0x0F;
			charat(col, row) = rnd_printable();
		}
	 }
	}
	/* start new trails at some of the fully darkend top row cells */
	row = 0;
	for(col = 0; col < SCREENWIDTH; col++) {
		currentColor = colorat(col, row);
		if(currentColor == 0 && ((rand() % 100) >= spawn)) {
			colorat(col, row) = 0x0F;
			charat(col, row) = rnd_printable();
		}
	}
}

void delay(long msecs)
{
    fd_set fdset;
    struct timeval tv;

    FD_ZERO(&fdset);
    tv.tv_sec = 0;
    tv.tv_usec = msecs * 1000;
    select(0, &fdset, NULL, NULL, &tv);
}

static int stop;

static void onintr(int sig)
{
    stop++;
    signal(SIGINT, onintr);     /* only required for ELKS */
}

int main(int argc, char** argv)
{
    int i;

    tty_init(CatchISig|ExitLastLine|FullBuffer);
    signal(SIGINT, onintr);

    txtmem = tty_allocate_screen(SCREENWIDTH, SCREENHEIGHT);
    tty_setfgpalette(fg16_green, fg256_green);

	//paint_box(0,0,80,25,0x0F);
	//paint_box(0,0,80,1, 0x00);
    while (!stop) {
		step(95);
        tty_output_screen(1);
        delay(DELAY);
	}

	/* step through the simulation some more with new spawns disabled,
		to allow remaining trails to finish their arc */
	for(i = 0; i < 36; i ++) {
		step(100);
        tty_output_screen(1);
        delay(DELAY);
        if (stop > 1)
            break;
	}
    if (stop < 2)
	    fill_box(0, 0, SCREENWIDTH, SCREENHEIGHT, ' ', 0x07);
    tty_output_screen(1);
	return 0;
}
