#ifndef lint
/*static char yysccsid[] = "from: @(#)yaccpar	1.9 (Berkeley) 02/21/93";*/
static char yyrcsid[] = "$Id$";
#endif
#define YYBYACC 1
#define YYMAJOR 1
#define YYMINOR 9
#define yyclearin (yychar=(-1))
#define yyerrok (yyerrflag=0)
#define YYRECOVERING (yyerrflag!=0)
#define YYPREFIX "yy"
#line 2 "sbc.y"
/* sbc.y: A POSIX bc processor written for minix with no extensions.  */
 
/*  This file is part of bc written for MINIX.
    Copyright (C) 1991, 1992 Free Software Foundation, Inc.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License , or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; see the file COPYING.  If not, write to
    the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.

    You may contact the author by:
       e-mail:  phil@cs.wwu.edu
      us-mail:  Philip A. Nelson
                Computer Science Department, 9062
                Western Washington University
                Bellingham, WA 98226-9062
       
*************************************************************************/

#include "bcdefs.h"
#include "global.h"     /* To get the global variables. */
#include "proto.h"
#line 37 "sbc.y"
typedef union {
	char *s_value;
	char  c_value;
	int   i_value;
	arg_list *a_value;
       } YYSTYPE;
#line 52 "y.tab.c"
#define NEWLINE 257
#define AND 258
#define OR 259
#define NOT 260
#define STRING 261
#define NAME 262
#define NUMBER 263
#define MUL_OP 264
#define ASSIGN_OP 265
#define REL_OP 266
#define INCR_DECR 267
#define Define 268
#define Break 269
#define Quit 270
#define Length 271
#define Return 272
#define For 273
#define If 274
#define While 275
#define Sqrt 276
#define Else 277
#define Scale 278
#define Ibase 279
#define Obase 280
#define Auto 281
#define Read 282
#define Warranty 283
#define Halt 284
#define Last 285
#define Continue 286
#define Print 287
#define Limits 288
#define UNARY_MINUS 289
#define YYERRCODE 256
short yylhs[] = {                                        -1,
    0,    0,   10,   10,   10,   11,   11,   11,   11,   12,
   12,   12,   12,   12,   12,   13,   13,   14,   14,   14,
   14,   14,   14,   14,   17,   18,   19,   20,   14,   21,
   14,   22,   23,   14,   14,   24,   15,    4,    4,    5,
    5,    6,    6,    6,    7,    7,    7,    7,    8,    8,
    9,    9,   16,   16,    3,    3,   25,    1,    1,    1,
    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
    1,    1,    2,    2,    2,    2,    2,
};
short yylen[] = {                                         2,
    0,    2,    2,    1,    2,    0,    1,    3,    2,    0,
    1,    2,    3,    2,    3,    1,    2,    1,    1,    1,
    1,    1,    1,    4,    0,    0,    0,    0,   13,    0,
    6,    0,    0,    7,    3,    0,   12,    0,    1,    1,
    3,    0,    3,    3,    1,    3,    3,    5,    0,    1,
    1,    3,    1,    3,    0,    1,    0,    4,    3,    3,
    3,    3,    2,    1,    1,    3,    4,    2,    2,    4,
    4,    4,    1,    4,    1,    1,    1,
};
short yydefred[] = {                                      1,
    0,    0,   20,    0,   65,    0,    0,   21,   22,    0,
    0,   25,    0,   32,    0,    0,   75,   76,   18,    0,
    0,    0,    0,    0,    2,    0,    7,   16,    4,    5,
   17,    0,    0,    0,   77,   68,    0,    0,    0,    0,
    0,    0,    0,    0,   63,    0,    0,   11,    0,    0,
    0,    0,   57,   69,    3,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
   66,    0,    0,   35,    0,    0,    0,    0,    0,    0,
    8,   67,    0,   74,    0,    0,   39,    0,   70,   24,
    0,    0,   30,   33,   71,   72,   13,   15,    0,    0,
    0,    0,    0,   26,    0,    0,    0,   46,    0,    0,
    0,   31,    0,    0,    0,    0,   34,    0,   36,   48,
   27,    0,    0,    0,    0,   43,   44,    0,    0,    0,
    0,    0,   28,   37,    0,   29,
};
short yydgoto[] = {                                       1,
   23,   24,   64,   86,   87,  119,   88,   58,   59,   25,
   26,   47,   27,   28,   29,   67,   40,  111,  125,  135,
  106,   42,  107,  124,   79,
};
short yysindex[] = {                                      0,
  -40,    8,    0,  -21,    0, -221, -218,    0,    0,    6,
   16,    0,   25,    0,   31,   32,    0,    0,    0,   98,
   98,   79,   60, -230,    0,  -58,    0,    0,    0,    0,
    0,   98,   98,  -59,    0,    0,   42,   98,   98,   45,
   98,   52,   98,   98,    0,  -39,  -56,    0,   98,   98,
   98,   98,    0,    0,    0,   33,   60,   20,   53,   46,
 -176,  -27,   60,   49,   98,  106,   61,   98,  -20,    4,
    0,   79,   79,    0,   10,  -84,  -84,   10,   98,   79,
    0,    0,   98,    0,   15,   67,    0,   66,    0,    0,
   50,   98,    0,    0,    0,    0,    0,    0,   60,   60,
   18,  -11, -149,    0,   60,   79,   73,    0, -140,   30,
   98,    0,   79, -159,   34,   64,    0, -137,    0,    0,
    0,   15,  -37,   79,   98,    0,    0, -136,  -51,   21,
   30,   56,    0,    0,   79,    0,
};
short yyrindex[] = {                                      0,
  -47,    0,    0,  124,    0,    0,    0,    0,    0,    0,
  -50,    0,    0,    0,    0,  323,    0,    0,    0,    0,
    0,  -44,   -9,  342,    0,    0,    0,    0,    0,    0,
    0,   88,    0,  352,    0,    0,    0,    0,   89,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,  -42,  -14,    0,   91,    0,
   92,    0,   94,    0,    0,   -8,    0,    0,    0,    0,
    0,   -7,    9,    0,  149,   -5,  381,  364,    0,    0,
    0,    0,    0,    0,  -10,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,   35,   36,
    0,    0,    0,    0,    1,    0,    0,    0,    0,   43,
    0,    0,    0,  -16,    0,    0,    0,    0,    0,    0,
    0,  -33,    0,   40,    0,    0,    0,    0,    0,    0,
  -31,   41,    0,    0,    0,    0,
};
short yygindex[] = {                                      0,
  449,  130,    0,    0,    0,    0,   19,    0,    0,    0,
    0,   17,   86,  378,    0,  -48,    0,    0,    0,    0,
    0,    0,    0,    0,    0,
};
#define YYTABLESIZE 647
short yytable[] = {                                      21,
   56,   71,   73,   50,   20,   51,  128,   73,   23,   52,
   45,    6,   47,   89,   10,   50,    9,   51,   32,   94,
   95,  127,   50,   42,   51,   45,   51,   47,   42,   51,
   40,   33,   53,   45,   53,   59,   54,   59,   59,   59,
   34,   54,   42,   37,   96,   38,   50,   21,   51,   19,
   53,   12,   20,   59,   52,   39,   35,   17,   18,   54,
   82,  133,  116,   50,   41,   51,   52,   14,   74,   33,
   43,   44,   21,   52,   23,   58,   52,   20,   58,   52,
   10,   61,   22,   41,   65,   85,   47,   59,   50,   90,
   51,   68,   50,   58,   51,   21,   83,   52,   10,   12,
   20,   93,   50,   52,   51,  101,   42,  102,  104,  103,
  108,  109,  110,  113,   52,   19,  114,   12,   21,   59,
  115,  118,  121,   20,  122,  131,  120,   58,   49,   55,
   22,   50,   38,   14,   56,   36,  123,   21,   84,   52,
  129,   81,   20,   52,    0,    0,    0,    0,   50,    0,
   51,    0,    0,   52,    0,   22,    0,    0,    0,   58,
    0,    0,    0,    0,   73,    0,   73,   73,   73,    0,
    0,    0,    0,    0,    0,    0,    0,    0,   22,   49,
  134,    0,   73,    0,    0,    0,    0,    0,    0,   61,
    0,   61,   61,   61,    0,    0,    0,    0,   55,   52,
   72,   22,    0,    0,    0,  132,   23,   61,    0,    6,
    0,    0,   10,    0,    9,    2,   73,   73,    0,  126,
    3,    4,    5,   45,   49,   47,    6,    7,    8,    9,
   10,   11,   12,   13,   14,   15,   49,   16,   17,   18,
   42,   61,   19,   49,   42,   42,   42,   19,   73,   12,
   42,   59,   42,   42,   42,   42,   42,   42,   42,   42,
   59,   42,   42,   42,   30,   14,   42,   49,    3,    4,
    5,    0,    0,   61,    6,    0,    8,    9,   10,   11,
   12,   13,   14,   15,   49,   16,   17,   18,   80,    0,
   19,   58,    0,    3,    4,    5,   10,   12,    0,    6,
   58,    8,    9,   10,   11,   12,   13,   14,   15,   49,
   16,   17,   18,   49,    0,   19,    3,    4,    5,    0,
    0,    0,    6,   49,    8,    9,   10,   11,   12,   13,
   14,   15,    0,   16,   17,   18,    0,    0,   19,    3,
    4,    5,    0,    0,    0,    6,    0,    8,    9,   10,
   11,   12,   13,   14,   15,    0,   16,   17,   18,    4,
    5,   19,    0,   77,    6,   77,   77,   77,   10,   49,
    0,   92,    0,   15,    0,   16,   17,   18,    0,   31,
   73,   77,   64,    0,   64,   64,   64,   73,   73,   73,
   73,    0,   73,    0,   73,   73,   73,    0,    0,   48,
   64,    0,    0,    0,   62,   61,   62,   62,   62,    0,
   73,    0,   61,    0,   61,   77,   77,    0,    0,    0,
    0,   60,   62,   60,   60,   60,    0,    0,    0,    0,
    0,    0,    0,    0,   64,   64,    0,    0,    0,   60,
    0,    0,    0,    0,   73,   73,    0,   77,    0,   97,
   98,    0,    0,    0,    0,    0,   62,   31,    0,    0,
    0,    0,    0,    0,    0,    0,   64,    0,   45,   46,
    0,    0,    0,   60,    0,    0,   73,    0,    0,    0,
   57,   60,    0,  112,    0,    0,   62,   63,   62,   66,
  117,   69,   70,    0,    0,    0,    0,   75,   76,   77,
   78,   48,    0,    0,    0,   60,    0,    0,    0,   97,
    0,    0,  136,   91,    0,    0,   66,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,   99,    0,    0,
    0,  100,    0,    0,    0,    0,    0,    0,    0,    0,
  105,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,   66,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,  130,    0,    0,    0,    0,    0,   77,
    0,    0,    0,    0,    0,    0,   77,   77,   77,   77,
    0,    0,    0,    0,    0,    0,    0,    0,   64,    0,
    0,    0,    0,    0,    0,   64,    0,   64,   73,    0,
    0,    0,    0,    0,    0,   73,    0,   73,    0,    0,
   62,    0,    0,    0,    0,    0,    0,   62,    0,   62,
    0,    0,    0,    0,    0,    0,    0,   60,    0,    0,
    0,    0,    0,    0,    0,    0,   60,
};
short yycheck[] = {                                      40,
   59,   41,   59,   43,   45,   45,   44,   59,   59,   94,
   44,   59,   44,   41,   59,   43,   59,   45,   40,   68,
   41,   59,   43,   40,   45,   59,   41,   59,   45,   44,
   41,   91,   41,   44,  265,   41,  267,   43,   44,   45,
  262,   41,   59,  262,   41,   40,   43,   40,   45,   59,
   59,   59,   45,   59,   94,   40,  278,  279,  280,   59,
   41,   41,  111,   43,   40,   45,   94,   59,  125,   91,
   40,   40,   40,   94,  125,   41,   41,   45,   44,   44,
  125,   40,  123,   41,   40,  262,   44,   93,   43,   41,
   45,   40,   43,   59,   45,   40,   44,   94,   59,   59,
   45,   41,   43,   94,   45,   91,  123,   41,   59,   44,
   93,  123,  262,   41,   94,  125,  257,  125,   40,  125,
   91,  281,   59,   45,  262,  262,   93,   93,   41,   41,
  123,   41,   41,  125,   41,    6,  118,   40,   93,   94,
  124,   56,   45,   94,   -1,   -1,   -1,   -1,   43,   -1,
   45,   -1,   -1,   94,   -1,  123,   -1,   -1,   -1,  125,
   -1,   -1,   -1,   -1,   41,   -1,   43,   44,   45,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,  123,  264,
  125,   -1,   59,   -1,   -1,   -1,   -1,   -1,   -1,   41,
   -1,   43,   44,   45,   -1,   -1,   -1,   -1,  257,   94,
  257,  123,   -1,   -1,   -1,  257,  257,   59,   -1,  257,
   -1,   -1,  257,   -1,  257,  256,   93,   94,   -1,  257,
  261,  262,  263,  257,  264,  257,  267,  268,  269,  270,
  271,  272,  273,  274,  275,  276,  264,  278,  279,  280,
  257,   93,  283,  264,  261,  262,  263,  257,  125,  257,
  267,  257,  269,  270,  271,  272,  273,  274,  275,  276,
  266,  278,  279,  280,  257,  257,  283,  264,  261,  262,
  263,   -1,   -1,  125,  267,   -1,  269,  270,  271,  272,
  273,  274,  275,  276,  264,  278,  279,  280,  256,   -1,
  283,  257,   -1,  261,  262,  263,  257,  257,   -1,  267,
  266,  269,  270,  271,  272,  273,  274,  275,  276,  264,
  278,  279,  280,  264,   -1,  283,  261,  262,  263,   -1,
   -1,   -1,  267,  264,  269,  270,  271,  272,  273,  274,
  275,  276,   -1,  278,  279,  280,   -1,   -1,  283,  261,
  262,  263,   -1,   -1,   -1,  267,   -1,  269,  270,  271,
  272,  273,  274,  275,  276,   -1,  278,  279,  280,  262,
  263,  283,   -1,   41,  267,   43,   44,   45,  271,  264,
   -1,  266,   -1,  276,   -1,  278,  279,  280,   -1,    2,
  257,   59,   41,   -1,   43,   44,   45,  264,  265,  266,
  267,   -1,   41,   -1,   43,   44,   45,   -1,   -1,   22,
   59,   -1,   -1,   -1,   41,  257,   43,   44,   45,   -1,
   59,   -1,  264,   -1,  266,   93,   94,   -1,   -1,   -1,
   -1,   41,   59,   43,   44,   45,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   93,   94,   -1,   -1,   -1,   59,
   -1,   -1,   -1,   -1,   93,   94,   -1,  125,   -1,   72,
   73,   -1,   -1,   -1,   -1,   -1,   93,   80,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,  125,   -1,   20,   21,
   -1,   -1,   -1,   93,   -1,   -1,  125,   -1,   -1,   -1,
   32,   33,   -1,  106,   -1,   -1,   38,   39,  125,   41,
  113,   43,   44,   -1,   -1,   -1,   -1,   49,   50,   51,
   52,  124,   -1,   -1,   -1,  125,   -1,   -1,   -1,  132,
   -1,   -1,  135,   65,   -1,   -1,   68,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   79,   -1,   -1,
   -1,   83,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   92,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,  111,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,  125,   -1,   -1,   -1,   -1,   -1,  257,
   -1,   -1,   -1,   -1,   -1,   -1,  264,  265,  266,  267,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,  257,   -1,
   -1,   -1,   -1,   -1,   -1,  264,   -1,  266,  257,   -1,
   -1,   -1,   -1,   -1,   -1,  264,   -1,  266,   -1,   -1,
  257,   -1,   -1,   -1,   -1,   -1,   -1,  264,   -1,  266,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,  257,   -1,   -1,
   -1,   -1,   -1,   -1,   -1,   -1,  266,
};
#define YYFINAL 1
#ifndef YYDEBUG
#define YYDEBUG 0
#endif
#define YYMAXTOKEN 289
#if YYDEBUG
char *yyname[] = {
"end-of-file",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,"'('","')'",0,"'+'","','","'-'",0,0,0,0,0,0,0,0,0,0,0,0,0,"';'",0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"'['",0,"']'","'^'",0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"'{'",0,"'}'",0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,"NEWLINE","AND","OR","NOT","STRING","NAME","NUMBER","MUL_OP",
"ASSIGN_OP","REL_OP","INCR_DECR","Define","Break","Quit","Length","Return",
"For","If","While","Sqrt","Else","Scale","Ibase","Obase","Auto","Read",
"Warranty","Halt","Last","Continue","Print","Limits","UNARY_MINUS",
};
char *yyrule[] = {
"$accept : program",
"program :",
"program : program input_item",
"input_item : semicolon_list NEWLINE",
"input_item : function",
"input_item : error NEWLINE",
"semicolon_list :",
"semicolon_list : statement_or_error",
"semicolon_list : semicolon_list ';' statement_or_error",
"semicolon_list : semicolon_list ';'",
"statement_list :",
"statement_list : statement",
"statement_list : statement_list NEWLINE",
"statement_list : statement_list NEWLINE statement",
"statement_list : statement_list ';'",
"statement_list : statement_list ';' statement",
"statement_or_error : statement",
"statement_or_error : error statement",
"statement : Warranty",
"statement : expression",
"statement : STRING",
"statement : Break",
"statement : Quit",
"statement : Return",
"statement : Return '(' return_expression ')'",
"$$1 :",
"$$2 :",
"$$3 :",
"$$4 :",
"statement : For $$1 '(' expression ';' $$2 relational_expression ';' $$3 expression ')' $$4 statement",
"$$5 :",
"statement : If '(' relational_expression ')' $$5 statement",
"$$6 :",
"$$7 :",
"statement : While $$6 '(' relational_expression $$7 ')' statement",
"statement : '{' statement_list '}'",
"$$8 :",
"function : Define NAME '(' opt_parameter_list ')' '{' NEWLINE opt_auto_define_list $$8 statement_list NEWLINE '}'",
"opt_parameter_list :",
"opt_parameter_list : parameter_list",
"parameter_list : NAME",
"parameter_list : define_list ',' NAME",
"opt_auto_define_list :",
"opt_auto_define_list : Auto define_list NEWLINE",
"opt_auto_define_list : Auto define_list ';'",
"define_list : NAME",
"define_list : NAME '[' ']'",
"define_list : define_list ',' NAME",
"define_list : define_list ',' NAME '[' ']'",
"opt_argument_list :",
"opt_argument_list : argument_list",
"argument_list : expression",
"argument_list : argument_list ',' expression",
"relational_expression : expression",
"relational_expression : expression REL_OP expression",
"return_expression :",
"return_expression : expression",
"$$9 :",
"expression : named_expression ASSIGN_OP $$9 expression",
"expression : expression '+' expression",
"expression : expression '-' expression",
"expression : expression MUL_OP expression",
"expression : expression '^' expression",
"expression : '-' expression",
"expression : named_expression",
"expression : NUMBER",
"expression : '(' expression ')'",
"expression : NAME '(' opt_argument_list ')'",
"expression : INCR_DECR named_expression",
"expression : named_expression INCR_DECR",
"expression : Length '(' expression ')'",
"expression : Sqrt '(' expression ')'",
"expression : Scale '(' expression ')'",
"named_expression : NAME",
"named_expression : NAME '[' expression ']'",
"named_expression : Ibase",
"named_expression : Obase",
"named_expression : Scale",
};
#endif
#ifdef YYSTACKSIZE
#undef YYMAXDEPTH
#define YYMAXDEPTH YYSTACKSIZE
#else
#ifdef YYMAXDEPTH
#define YYSTACKSIZE YYMAXDEPTH
#else
#define YYSTACKSIZE 500
#define YYMAXDEPTH 500
#endif
#endif
int yydebug;
int yynerrs;
int yyerrflag;
int yychar;
short *yyssp;
YYSTYPE *yyvsp;
YYSTYPE yyval;
YYSTYPE yylval;
short yyss[YYSTACKSIZE];
YYSTYPE yyvs[YYSTACKSIZE];
#define yystacksize YYSTACKSIZE
#define YYABORT goto yyabort
#define YYREJECT goto yyabort
#define YYACCEPT goto yyaccept
#define YYERROR goto yyerrlab
int
#if defined(__STDC__)
yyparse(void)
#else
yyparse()
#endif
{
    register int yym, yyn, yystate;
#if YYDEBUG
    register char *yys;
    extern char *getenv();

    if (yys = getenv("YYDEBUG"))
    {
        yyn = *yys;
        if (yyn >= '0' && yyn <= '9')
            yydebug = yyn - '0';
    }
#endif

    yynerrs = 0;
    yyerrflag = 0;
    yychar = (-1);

    yyssp = yyss;
    yyvsp = yyvs;
    *yyssp = yystate = 0;

yyloop:
    if ((yyn = yydefred[yystate]) != 0) goto yyreduce;
    if (yychar < 0)
    {
        if ((yychar = yylex()) < 0) yychar = 0;
#if YYDEBUG
        if (yydebug)
        {
            yys = 0;
            if (yychar <= YYMAXTOKEN) yys = yyname[yychar];
            if (!yys) yys = "illegal-symbol";
            printf("%sdebug: state %d, reading %d (%s)\n",
                    YYPREFIX, yystate, yychar, yys);
        }
#endif
    }
    if ((yyn = yysindex[yystate]) && (yyn += yychar) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == yychar)
    {
#if YYDEBUG
        if (yydebug)
            printf("%sdebug: state %d, shifting to state %d\n",
                    YYPREFIX, yystate, yytable[yyn]);
#endif
        if (yyssp >= yyss + yystacksize - 1)
        {
            goto yyoverflow;
        }
        *++yyssp = yystate = yytable[yyn];
        *++yyvsp = yylval;
        yychar = (-1);
        if (yyerrflag > 0)  --yyerrflag;
        goto yyloop;
    }
    if ((yyn = yyrindex[yystate]) && (yyn += yychar) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == yychar)
    {
        yyn = yytable[yyn];
        goto yyreduce;
    }
    if (yyerrflag) goto yyinrecovery;
    yyerror("syntax error");
#ifdef lint
    goto yyerrlab;
#endif
yyerrlab:
    ++yynerrs;
yyinrecovery:
    if (yyerrflag < 3)
    {
        yyerrflag = 3;
        for (;;)
        {
            if ((yyn = yysindex[*yyssp]) && (yyn += YYERRCODE) >= 0 &&
                    yyn <= YYTABLESIZE && yycheck[yyn] == YYERRCODE)
            {
#if YYDEBUG
                if (yydebug)
                    printf("%sdebug: state %d, error recovery shifting\
 to state %d\n", YYPREFIX, *yyssp, yytable[yyn]);
#endif
                if (yyssp >= yyss + yystacksize - 1)
                {
                    goto yyoverflow;
                }
                *++yyssp = yystate = yytable[yyn];
                *++yyvsp = yylval;
                goto yyloop;
            }
            else
            {
#if YYDEBUG
                if (yydebug)
                    printf("%sdebug: error recovery discarding state %d\n",
                            YYPREFIX, *yyssp);
#endif
                if (yyssp <= yyss) goto yyabort;
                --yyssp;
                --yyvsp;
            }
        }
    }
    else
    {
        if (yychar == 0) goto yyabort;
#if YYDEBUG
        if (yydebug)
        {
            yys = 0;
            if (yychar <= YYMAXTOKEN) yys = yyname[yychar];
            if (!yys) yys = "illegal-symbol";
            printf("%sdebug: state %d, error recovery discards token %d (%s)\n",
                    YYPREFIX, yystate, yychar, yys);
        }
#endif
        yychar = (-1);
        goto yyloop;
    }
yyreduce:
#if YYDEBUG
    if (yydebug)
        printf("%sdebug: state %d, reducing by rule %d (%s)\n",
                YYPREFIX, yystate, yyn, yyrule[yyn]);
#endif
    yym = yylen[yyn];
    yyval = yyvsp[1-yym];
    switch (yyn)
    {
case 1:
#line 82 "sbc.y"
{
			      yyval.i_value = 0;
			      std_only = TRUE;
			      if (interactive)
				{
				  printf ("s%s\n", BC_VERSION);
				  welcome();
				}
			    }
break;
case 3:
#line 94 "sbc.y"
{ run_code(); }
break;
case 4:
#line 96 "sbc.y"
{ run_code(); }
break;
case 5:
#line 98 "sbc.y"
{
			      yyerrok; 
			      init_gen() ;
			    }
break;
case 6:
#line 104 "sbc.y"
{ yyval.i_value = 0; }
break;
case 10:
#line 110 "sbc.y"
{ yyval.i_value = 0; }
break;
case 17:
#line 119 "sbc.y"
{ yyval.i_value = yyvsp[0].i_value; }
break;
case 18:
#line 122 "sbc.y"
{ warranty("s"); }
break;
case 19:
#line 124 "sbc.y"
{
			      if (yyvsp[0].i_value & 1)
				generate ("W");
			      else
				generate ("p");
			    }
break;
case 20:
#line 131 "sbc.y"
{
			      yyval.i_value = 0;
			      generate ("w");
			      generate (yyvsp[0].s_value);
			      free (yyvsp[0].s_value);
			    }
break;
case 21:
#line 138 "sbc.y"
{
			      if (break_label == 0)
				yyerror ("Break outside a for/while");
			      else
				{
				  sprintf (genstr, "J%1d:", break_label);
				  generate (genstr);
				}
			    }
break;
case 22:
#line 148 "sbc.y"
{ exit(0); }
break;
case 23:
#line 150 "sbc.y"
{ generate ("0R"); }
break;
case 24:
#line 152 "sbc.y"
{ generate ("R"); }
break;
case 25:
#line 154 "sbc.y"
{
			      yyvsp[0].i_value = break_label; 
			      break_label = next_label++;
			    }
break;
case 26:
#line 159 "sbc.y"
{
			      yyvsp[-1].i_value = next_label++;
			      sprintf (genstr, "pN%1d:", yyvsp[-1].i_value);
			      generate (genstr);
			    }
break;
case 27:
#line 165 "sbc.y"
{
			      yyvsp[-1].i_value = next_label++;
			      sprintf (genstr, "B%1d:J%1d:", yyvsp[-1].i_value, break_label);
			      generate (genstr);
			      yyval.i_value = next_label++;
			      sprintf (genstr, "N%1d:", yyval.i_value);
			      generate (genstr);
			    }
break;
case 28:
#line 174 "sbc.y"
{
			      sprintf (genstr, "pJ%1d:N%1d:", yyvsp[-7].i_value, yyvsp[-4].i_value);
			      generate (genstr);
			    }
break;
case 29:
#line 179 "sbc.y"
{
			      sprintf (genstr, "J%1d:N%1d:", yyvsp[-4].i_value,
				       break_label);
			      generate (genstr);
			      break_label = yyvsp[-12].i_value;
			    }
break;
case 30:
#line 186 "sbc.y"
{
			      yyvsp[-1].i_value = next_label++;
			      sprintf (genstr, "Z%1d:", yyvsp[-1].i_value);
			      generate (genstr);
			    }
break;
case 31:
#line 192 "sbc.y"
{
			      sprintf (genstr, "N%1d:", yyvsp[-3].i_value); 
			      generate (genstr);
			    }
break;
case 32:
#line 197 "sbc.y"
{
			      yyvsp[0].i_value = next_label++;
			      sprintf (genstr, "N%1d:", yyvsp[0].i_value);
			      generate (genstr);
			    }
break;
case 33:
#line 203 "sbc.y"
{
			      yyvsp[0].i_value = break_label; 
			      break_label = next_label++;
			      sprintf (genstr, "Z%1d:", break_label);
			      generate (genstr);
			    }
break;
case 34:
#line 210 "sbc.y"
{
			      sprintf (genstr, "J%1d:N%1d:", yyvsp[-6].i_value, break_label);
			      generate (genstr);
			      break_label = yyvsp[-3].i_value;
			    }
break;
case 35:
#line 216 "sbc.y"
{ yyval.i_value = 0; }
break;
case 36:
#line 220 "sbc.y"
{
			      check_params (yyvsp[-4].a_value,yyvsp[0].a_value);
			      sprintf (genstr, "F%d,%s.%s[", lookup(yyvsp[-6].s_value,FUNCT),
				       arg_str (yyvsp[-4].a_value,TRUE), arg_str (yyvsp[0].a_value,TRUE));
			      generate (genstr);
			      free_args (yyvsp[-4].a_value);
			      free_args (yyvsp[0].a_value);
			      yyvsp[-7].i_value = next_label;
			      next_label = 0;
			    }
break;
case 37:
#line 231 "sbc.y"
{
			      generate ("0R]");
			      next_label = yyvsp[-11].i_value;
			    }
break;
case 38:
#line 237 "sbc.y"
{ yyval.a_value = NULL; }
break;
case 40:
#line 241 "sbc.y"
{ yyval.a_value = nextarg (NULL, lookup(yyvsp[0].s_value,SIMPLE)); }
break;
case 41:
#line 243 "sbc.y"
{ yyval.a_value = nextarg (yyvsp[-2].a_value, lookup(yyvsp[0].s_value,SIMPLE)); }
break;
case 42:
#line 246 "sbc.y"
{ yyval.a_value = NULL; }
break;
case 43:
#line 248 "sbc.y"
{ yyval.a_value = yyvsp[-1].a_value; }
break;
case 44:
#line 250 "sbc.y"
{ yyval.a_value = yyvsp[-1].a_value; }
break;
case 45:
#line 253 "sbc.y"
{ yyval.a_value = nextarg (NULL, lookup(yyvsp[0].s_value,SIMPLE)); }
break;
case 46:
#line 255 "sbc.y"
{ yyval.a_value = nextarg (NULL, lookup(yyvsp[-2].s_value,ARRAY)); }
break;
case 47:
#line 257 "sbc.y"
{ yyval.a_value = nextarg (yyvsp[-2].a_value, lookup(yyvsp[0].s_value,SIMPLE)); }
break;
case 48:
#line 259 "sbc.y"
{ yyval.a_value = nextarg (yyvsp[-4].a_value, lookup(yyvsp[-2].s_value,ARRAY)); }
break;
case 49:
#line 262 "sbc.y"
{ yyval.a_value = NULL; }
break;
case 51:
#line 266 "sbc.y"
{ yyval.a_value = nextarg (NULL,0); }
break;
case 52:
#line 268 "sbc.y"
{ yyval.a_value = nextarg (yyvsp[-2].a_value,0); }
break;
case 53:
#line 271 "sbc.y"
{ yyval.i_value = 0; }
break;
case 54:
#line 273 "sbc.y"
{
			      yyval.i_value = 0;
			      switch (*(yyvsp[-1].s_value))
				{
				case '=':
				  generate ("=");
				  break;
				case '!':
				  generate ("#");
				  break;
				case '<':
				  if (yyvsp[-1].s_value[1] == '=')
				    generate ("{");
				  else
				    generate ("<");
				  break;
				case '>':
				  if (yyvsp[-1].s_value[1] == '=')
				    generate ("}");
				  else
				    generate (">");
				  break;
				}
			    }
break;
case 55:
#line 299 "sbc.y"
{
			      yyval.i_value = 0;
			      generate ("0");
			    }
break;
case 57:
#line 306 "sbc.y"
{
			      if (yyvsp[0].c_value != '=')
				{
				  if (yyvsp[-1].i_value < 0)
				    sprintf (genstr, "DL%d:", -yyvsp[-1].i_value);
				  else
				    sprintf (genstr, "l%d:", yyvsp[-1].i_value);
				  generate (genstr);
				}
			    }
break;
case 58:
#line 317 "sbc.y"
{
			      yyval.i_value = 0;
			      if (yyvsp[-2].c_value != '=')
				{
				  sprintf (genstr, "%c", yyvsp[-2].c_value);
				  generate (genstr);
				}
			      if (yyvsp[-3].i_value < 0)
				sprintf (genstr, "S%d:", -yyvsp[-3].i_value);
			      else
				sprintf (genstr, "s%d:", yyvsp[-3].i_value);
			      generate (genstr);
			    }
break;
case 59:
#line 331 "sbc.y"
{ generate ("+"); }
break;
case 60:
#line 333 "sbc.y"
{ generate ("-"); }
break;
case 61:
#line 335 "sbc.y"
{
			      genstr[0] = yyvsp[-1].c_value;
			      genstr[1] = 0;
			      generate (genstr);
			    }
break;
case 62:
#line 341 "sbc.y"
{ generate ("^"); }
break;
case 63:
#line 343 "sbc.y"
{ generate ("n"); yyval.i_value = 1;}
break;
case 64:
#line 345 "sbc.y"
{
			      yyval.i_value = 1;
			      if (yyvsp[0].i_value < 0)
				sprintf (genstr, "L%d:", -yyvsp[0].i_value);
			      else
				sprintf (genstr, "l%d:", yyvsp[0].i_value);
			      generate (genstr);
			    }
break;
case 65:
#line 354 "sbc.y"
{
			      int len = strlen(yyvsp[0].s_value);
			      yyval.i_value = 1;
			      if (len == 1 && *yyvsp[0].s_value == '0')
				generate ("0");
			      else
				{
				  if (len == 1 && *yyvsp[0].s_value == '1')
				    generate ("1");
				  else
				    {
				      generate ("K");
				      generate (yyvsp[0].s_value);
				      generate (":");
				    }
				  free (yyvsp[0].s_value);
				}
			    }
break;
case 66:
#line 373 "sbc.y"
{ yyval.i_value = 1; }
break;
case 67:
#line 375 "sbc.y"
{
			      yyval.i_value = 1;
			      if (yyvsp[-1].a_value != NULL)
				{ 
				  sprintf (genstr, "C%d,%s:", lookup(yyvsp[-3].s_value,FUNCT),
					   arg_str (yyvsp[-1].a_value,FALSE));
				  free_args (yyvsp[-1].a_value);
				}
			      else
				  sprintf (genstr, "C%d:", lookup(yyvsp[-3].s_value,FUNCT));
			      generate (genstr);
			    }
break;
case 68:
#line 388 "sbc.y"
{
			      yyval.i_value = 1;
			      if (yyvsp[0].i_value < 0)
				{
				  if (yyvsp[-1].c_value == '+')
				    sprintf (genstr, "DA%d:L%d:", -yyvsp[0].i_value, -yyvsp[0].i_value);
				  else
				    sprintf (genstr, "DM%d:L%d:", -yyvsp[0].i_value, -yyvsp[0].i_value);
				}
			      else
				{
				  if (yyvsp[-1].c_value == '+')
				    sprintf (genstr, "i%d:l%d:", yyvsp[0].i_value, yyvsp[0].i_value);
				  else
				    sprintf (genstr, "d%d:l%d:", yyvsp[0].i_value, yyvsp[0].i_value);
				}
			      generate (genstr);
			    }
break;
case 69:
#line 407 "sbc.y"
{
			      yyval.i_value = 1;
			      if (yyvsp[-1].i_value < 0)
				{
				  sprintf (genstr, "DL%d:x", -yyvsp[-1].i_value);
				  generate (genstr); 
				  if (yyvsp[0].c_value == '+')
				    sprintf (genstr, "A%d:", -yyvsp[-1].i_value);
				  else
				    sprintf (genstr, "M%d:", -yyvsp[-1].i_value);
				}
			      else
				{
				  sprintf (genstr, "l%d:", yyvsp[-1].i_value);
				  generate (genstr);
				  if (yyvsp[0].c_value == '+')
				    sprintf (genstr, "i%d:", yyvsp[-1].i_value);
				  else
				    sprintf (genstr, "d%d:", yyvsp[-1].i_value);
				}
			      generate (genstr);
			    }
break;
case 70:
#line 430 "sbc.y"
{ generate ("cL"); yyval.i_value = 1;}
break;
case 71:
#line 432 "sbc.y"
{ generate ("cR"); yyval.i_value = 1;}
break;
case 72:
#line 434 "sbc.y"
{ generate ("cS"); yyval.i_value = 1;}
break;
case 73:
#line 437 "sbc.y"
{ yyval.i_value = lookup(yyvsp[0].s_value,SIMPLE); }
break;
case 74:
#line 439 "sbc.y"
{ yyval.i_value = lookup(yyvsp[-3].s_value,ARRAY); }
break;
case 75:
#line 441 "sbc.y"
{ yyval.i_value = 0; }
break;
case 76:
#line 443 "sbc.y"
{ yyval.i_value = 1; }
break;
case 77:
#line 445 "sbc.y"
{ yyval.i_value = 2; }
break;
#line 1025 "y.tab.c"
    }
    yyssp -= yym;
    yystate = *yyssp;
    yyvsp -= yym;
    yym = yylhs[yyn];
    if (yystate == 0 && yym == 0)
    {
#if YYDEBUG
        if (yydebug)
            printf("%sdebug: after reduction, shifting from state 0 to\
 state %d\n", YYPREFIX, YYFINAL);
#endif
        yystate = YYFINAL;
        *++yyssp = YYFINAL;
        *++yyvsp = yyval;
        if (yychar < 0)
        {
            if ((yychar = yylex()) < 0) yychar = 0;
#if YYDEBUG
            if (yydebug)
            {
                yys = 0;
                if (yychar <= YYMAXTOKEN) yys = yyname[yychar];
                if (!yys) yys = "illegal-symbol";
                printf("%sdebug: state %d, reading %d (%s)\n",
                        YYPREFIX, YYFINAL, yychar, yys);
            }
#endif
        }
        if (yychar == 0) goto yyaccept;
        goto yyloop;
    }
    if ((yyn = yygindex[yym]) && (yyn += yystate) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == yystate)
        yystate = yytable[yyn];
    else
        yystate = yydgoto[yym];
#if YYDEBUG
    if (yydebug)
        printf("%sdebug: after reduction, shifting from state %d \
to state %d\n", YYPREFIX, *yyssp, yystate);
#endif
    if (yyssp >= yyss + yystacksize - 1)
    {
        goto yyoverflow;
    }
    *++yyssp = yystate;
    *++yyvsp = yyval;
    goto yyloop;
yyoverflow:
    yyerror("yacc stack overflow");
yyabort:
    return (1);
yyaccept:
    return (0);
}
