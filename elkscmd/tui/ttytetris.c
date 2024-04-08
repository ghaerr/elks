/*
 *      tetris.c TTY-TETRIS Main file.
 *      Copyright © 2008 Martin Duquesnoy <xorg62@gmail.com>
 *      All rights reserved.
 *
 *      Redistribution and use in source and binary forms, with or without
 *      modification, are permitted provided that the following conditions are
 *      met:
 *
 *      * Redistributions of source code must retain the above copyright
 *        notice, this list of conditions and the following disclaimer.
 *      * Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following disclaimer
 *        in the documentation and/or other materials provided with the
 *        distribution.
 *      * Neither the name of the  nor the names of its
 *        contributors may be used to endorse or promote products derived from
 *        this software without specific prior written permission.
 *
 *      THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *      "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *      LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *      A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *      OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *      SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *      LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *      DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *      THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *      (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *      OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "ttytetris.h"

/* Functions */
void
init(void)
{
     struct sigaction siga;
     struct termios term;

     /* Clean term */
     clear_term();
     set_cursor(False);

     /* Make rand() really random :) */
     srand(time(0)&0xFFFF);

     /* Init variables */
     score = lines = 0;
     running = True;
     current.y = (FRAMEW / 2) - 1;
     current.num = nrand(0, 6);
     current.next = nrand(0, 6);

     /* Score */
     printxy(0, FRAMEH_NB + 2, FRAMEW + 3, "Score:");
     printxy(0, FRAMEH_NB + 3, FRAMEW + 3, "Lines:");
     DRAW_SCORE();

     /* Init signal */
     sigemptyset(&siga.sa_mask);
     siga.sa_flags = 0;
     siga.sa_handler = sig_handler;
     sigaction(SIGALRM, &siga, NULL);
     sigaction(SIGTERM, &siga, NULL);
     sigaction(SIGINT,  &siga, NULL);
     sigaction(SIGSEGV, &siga, NULL);

     /* Init timer */
     tv.it_value.tv_usec = TIMING;
     sig_handler(SIGALRM);

     /* Init terminal (for non blocking & noecho getchar(); */
     tcgetattr(STDIN_FILENO, &term);
     tcgetattr(STDIN_FILENO, &back_attr);
     term.c_lflag &= ~(ICANON|ECHO);
     tcsetattr(STDIN_FILENO, TCSANOW, &term);

     return;
}

int xgetchar()
{
    int c = getchar();
    clearerr(stdin);    /* ELKS permanently returns EOF after SIGALRM read */
    return c;
}

void
get_key_event(void)
{
     int c = xgetchar();

     if(c > 0)
          --current.x;

     switch(c)
     {
     case KEY_MOVE_LEFT:            shape_move(-EXP_FACT);              break;
     case KEY_MOVE_RIGHT:           shape_move(EXP_FACT);               break;
     case KEY_CHANGE_POSITION_NEXT: shape_set_position(N_POS);          break;
     case KEY_CHANGE_POSITION_PREV: shape_set_position(P_POS);          break;
     case KEY_DROP_SHAPE:           shape_drop();                       break;
     case KEY_SPEED:                ++current.x; ++score; DRAW_SCORE(); break;
     case KEY_PAUSE:                while(xgetchar() != KEY_PAUSE);     break;
     case KEY_QUIT:                 running = False;                    break;
     }

     return;
}

void
arrange_score(int l)
{
     /* Standard score count */
     switch(l)
     {
     case 1: score += 40;   break; /* One line */
     case 2: score += 100;  break; /* Two lines */
     case 3: score += 300;  break; /* Three lines */
     case 4: score += 1200; break; /* Four lines */
     }

     lines += l;

     DRAW_SCORE();

     return;
}

void
check_plain_line(void)
{
     int i, j, k, f, c = 0, nl = 0;

     for(i = 1; i < FRAMEH; ++i)
     {
          for(j = 1; j < FRAMEW; ++j)
               if(frame[i][j] == 0)
                    ++c;
          if(!c)
          {
               ++nl;
               for(k = i - 1; k > 1; --k)
                    for(f = 1; f < FRAMEW; ++f)
                         frame[k + 1][f] = frame[k][f];
          }
          c = 0;
     }
     arrange_score(nl);
     frame_refresh();

     return;
}

int
check_possible_pos(int x, int y)
{
     int i, j, c = 0;

     for(i = 0; i < 4; ++i)
          for(j = 0; j < EXP_FACT; ++j)
               if(frame[x + shapes[current.num][current.pos][i][0]]
                  [y + shapes[current.num][current.pos][i][1] * EXP_FACT + j] != 0)
                    ++c;

     return c;
}

void
quit(void)
{
     frame_refresh(); /* Redraw a last time the frame */
     set_cursor(True);
     tcsetattr(0, TCSANOW, &back_attr);
     printf("\nBye! Your score: %d\n", score);

     return;
}

int
main(int argc, char **argv)
{
     init();
     frame_init();
     frame_nextbox_init();;

     current.last_move = False;

     while(running)
     {
          get_key_event();
          shape_set();
          frame_refresh();
          shape_go_down();
     }

     quit();

     return 0;
}
