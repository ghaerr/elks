/* A Bison parser, made by GNU Bison 3.0.4.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

#ifndef YY_YY_ASM_TAB_H_INCLUDED
# define YY_YY_ASM_TAB_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 1
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    INCLUDE = 258,
    IF = 259,
    DEFINED = 260,
    MACRO = 261,
    MACRO_STRING = 262,
    ORG = 263,
    ERROR = 264,
    ECHO1 = 265,
    INCBIN = 266,
    INCLEN = 267,
    INCWORD = 268,
    RES = 269,
    WORD = 270,
    BYTE = 271,
    LDA = 272,
    LDX = 273,
    LDY = 274,
    STA = 275,
    STX = 276,
    STY = 277,
    AND = 278,
    ORA = 279,
    EOR = 280,
    ADC = 281,
    SBC = 282,
    CMP = 283,
    CPX = 284,
    CPY = 285,
    TSX = 286,
    TXS = 287,
    PHA = 288,
    PLA = 289,
    PHP = 290,
    PLP = 291,
    SEI = 292,
    CLI = 293,
    NOP = 294,
    TYA = 295,
    TAY = 296,
    TXA = 297,
    TAX = 298,
    CLC = 299,
    SEC = 300,
    RTS = 301,
    CLV = 302,
    CLD = 303,
    SED = 304,
    JSR = 305,
    JMP = 306,
    BEQ = 307,
    BNE = 308,
    BCC = 309,
    BCS = 310,
    BPL = 311,
    BMI = 312,
    BVC = 313,
    BVS = 314,
    INX = 315,
    DEX = 316,
    INY = 317,
    DEY = 318,
    INC = 319,
    DEC = 320,
    LSR = 321,
    ASL = 322,
    ROR = 323,
    ROL = 324,
    BIT = 325,
    SYMBOL = 326,
    STRING = 327,
    LAND = 328,
    LOR = 329,
    LNOT = 330,
    LPAREN = 331,
    RPAREN = 332,
    COMMA = 333,
    COLON = 334,
    X = 335,
    Y = 336,
    HASH = 337,
    PLUS = 338,
    MINUS = 339,
    MULT = 340,
    DIV = 341,
    MOD = 342,
    LT = 343,
    GT = 344,
    EQ = 345,
    NEQ = 346,
    ASSIGN = 347,
    GUESS = 348,
    NUMBER = 349,
    vNEG = 350,
    LABEL = 351
  };
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED

union YYSTYPE
{
#line 148 "asm.y" /* yacc.c:1909  */

    i32 num;
    char *str;
    struct atom *atom;
    struct expr *expr;

#line 158 "asm.tab.h" /* yacc.c:1909  */
};

typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;

int yyparse (void);

#endif /* !YY_YY_ASM_TAB_H_INCLUDED  */
