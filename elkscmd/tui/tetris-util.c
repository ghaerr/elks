/*
 *      util.c
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

#include <sys/time.h>
#include "ttytetris.h"

void
clear_term(void)
{
     puts("\033[2J");

     return;
}

void
set_cursor(Bool b)
{
     printf("\033[?25%c", ((b) ? 'h' : 'l'));

     return;
}

void
set_color(int color)
{
     int bg = 0, fg = 0;

     switch(color)
     {
     default:
     case Black:   bg = 0;  break;
     case Blue:    bg = 44; break;
     case Red:     bg = 41; break;
     case Magenta: bg = 45; break;
     case White:   bg = 47; break;
     case Green:   bg = 42; break;
     case Cyan:    bg = 46; break;
     case Yellow:  bg = 43; break;
     case Border:  bg = 47; break;
     case Score:   fg = 37; bg = 49; break;
     }

     printf("\033[%d;%dm", fg, bg);

     return;
}

void
printxy(int color, int x, int y, char *str)
{
     set_color(color);
     printf("\033[%d;%dH%s", ++x, ++y, str);
     set_color(0);

     return;
}

int
nrand(int min, int max)
{
     return (rand() % (max - min + 1)) + min;
}

void
sig_handler(int sig)
{
     switch(sig)
     {
     case SIGTERM:
     case SIGINT:
     case SIGSEGV:
          running = False;
          break;
     case SIGALRM:
          signal(SIGALRM, sig_handler); /* ELKS requires signal() every signal */
          tv.it_value.tv_usec -= tv.it_value.tv_usec / 3000;
          setitimer(ITIMER_REAL, &tv, NULL);
          break;
     }

     return;
}
