/*
 *      frame.c
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


/* Shape attribute for draw it in the next box (center etc..)
 * [0]: +x
 * [1]: +y
 * [2]: What shape position choose for a perfect position in the box
 */
const int sattr[7][3] = {{0,2}, {-1,0}, {-1,1,1}, {-1,1}, {-1,1}, {0,1}, {0,1}};

void
frame_init(void)
{
     int i;

     /* Frame border */
     for(i = 0; i < FRAMEW + 1; ++i)
     {
          frame[0][i] = Border;
          frame[FRAMEH][i] = Border;
     }
     for(i = 0; i < FRAMEH; ++i)
     {
          frame[i][0] = Border;
          frame[i][1] = Border;
          frame[i][FRAMEW] = Border;
          frame[i][FRAMEW - 1] = Border;
     }

     frame_refresh();

     return;
}

void
frame_refresh(void)
{
     int i, j;

     /* Diff frames and print changes */
     for(i = 0; i < FRAMEH + 1; ++i) {
          for(j = 0; j < FRAMEW + 1; ++j) {
               if(frame[i][j] != frame_prev[i][j]) {
                    printxy(frame[i][j], i, j, " ");
                    frame_prev[i][j] = frame[i][j];
               }
          }
     }
     return;
}
