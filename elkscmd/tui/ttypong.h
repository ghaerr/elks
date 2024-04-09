#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "unikey.h"
#include "curses.h"

/* Options */
#define FH 20
#define FW 60
#define RACKETL 3
#define TIMEDELAY 40

/* Const */
#define BALL   1
#define WALL   2
#define RACKET 3
#define OUT    4

/* Macros */
#define GX ((COLS / 2) - (FW / 2))
#define GY ((LINES / 2) - (FH / 2)) - 1
#define COL(x)    attron(COLOR_PAIR(x))
#define UNCOL(x)  attroff(COLOR_PAIR(x))

/* Struct */
typedef struct
{
     int running;
     int frame[FH + 1][FW + 1];

     struct
     {
          /* Ball position */
          int x, y;

          /* Incrementer (Oo ?) */
          int a, b;
     } ball;

     struct
     {
          int y, iay;
     } racket;

} Pong;

/* Prototypes */
void init_curses(void);
void init_frame(void);
void key_event(void);
void manage_ball(void);
void racket_move(int y);
void ia_racket(void);

Pong *pong;
