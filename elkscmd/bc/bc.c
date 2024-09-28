/* A Bison parser, made by GNU Bison 2.3.  */

/* Skeleton implementation for Bison's Yacc-like parsers in C

   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.  */

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

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.3"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Using locations.  */
#define YYLSP_NEEDED 0



/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     NEWLINE = 258,
     AND = 259,
     OR = 260,
     NOT = 261,
     STRING = 262,
     NAME = 263,
     NUMBER = 264,
     MUL_OP = 265,
     ASSIGN_OP = 266,
     REL_OP = 267,
     INCR_DECR = 268,
     Define = 269,
     Break = 270,
     Quit = 271,
     Length = 272,
     Return = 273,
     For = 274,
     If = 275,
     While = 276,
     Sqrt = 277,
     Else = 278,
     Scale = 279,
     Ibase = 280,
     Obase = 281,
     Auto = 282,
     Read = 283,
     Warranty = 284,
     Halt = 285,
     Last = 286,
     Continue = 287,
     Print = 288,
     Limits = 289,
     UNARY_MINUS = 290
   };
#endif
/* Tokens.  */
#define NEWLINE 258
#define AND 259
#define OR 260
#define NOT 261
#define STRING 262
#define NAME 263
#define NUMBER 264
#define MUL_OP 265
#define ASSIGN_OP 266
#define REL_OP 267
#define INCR_DECR 268
#define Define 269
#define Break 270
#define Quit 271
#define Length 272
#define Return 273
#define For 274
#define If 275
#define While 276
#define Sqrt 277
#define Else 278
#define Scale 279
#define Ibase 280
#define Obase 281
#define Auto 282
#define Read 283
#define Warranty 284
#define Halt 285
#define Last 286
#define Continue 287
#define Print 288
#define Limits 289
#define UNARY_MINUS 290




/* Copy the first part of user declarations.  */
#line 1 "bc.y"

/* bc.y: The grammar for a POSIX compatable bc processor with some
         extensions to the language. */

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
#include "global.h"
#include "proto.h"


/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* Enabling the token table.  */
#ifndef YYTOKEN_TABLE
# define YYTOKEN_TABLE 0
#endif

#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
#line 38 "bc.y"
{
	char	 *s_value;
	char	  c_value;
	int	  i_value;
	arg_list *a_value;
       }
/* Line 193 of yacc.c.  */
#line 208 "y.tab.c"
	YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 216 of yacc.c.  */
#line 221 "y.tab.c"

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#elif (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
typedef signed char yytype_int8;
#else
typedef short int yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(e) ((void) (e))
#else
# define YYUSE(e) /* empty */
#endif

/* Identity function, used to suppress warnings about constant conditions.  */
#ifndef lint
# define YYID(n) (n)
#else
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static int
YYID (int i)
#else
static int
YYID (i)
    int i;
#endif
{
  return i;
}
#endif

#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#     ifndef _STDLIB_H
#      define _STDLIB_H 1
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (YYID (0))
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined _STDLIB_H \
       && ! ((defined YYMALLOC || defined malloc) \
	     && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef _STDLIB_H
#    define _STDLIB_H 1
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined _STDLIB_H && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
	 || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss;
  YYSTYPE yyvs;
  };

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  YYSIZE_T yyi;				\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (YYID (0))
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack)					\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack, Stack, yysize);				\
	Stack = &yyptr->Stack;						\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (YYID (0))

#endif

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  2
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   667

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  47
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  33
/* YYNRULES -- Number of rules.  */
#define YYNRULES  98
/* YYNRULES -- Number of states.  */
#define YYNSTATES  167

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   290

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
      40,    41,     2,    35,    44,    36,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,    39,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    45,     2,    46,    37,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    42,     2,    43,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      38
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint16 yyprhs[] =
{
       0,     0,     3,     4,     7,    10,    12,    15,    16,    18,
      22,    25,    26,    28,    31,    35,    38,    42,    44,    47,
      49,    51,    53,    55,    57,    59,    61,    63,    65,    70,
      71,    72,    73,    74,    88,    89,    97,    98,    99,   107,
     111,   112,   116,   118,   122,   124,   126,   127,   128,   132,
     133,   146,   147,   149,   150,   154,   158,   160,   164,   168,
     174,   175,   177,   179,   183,   187,   193,   194,   196,   197,
     199,   200,   205,   206,   211,   212,   217,   220,   224,   228,
     232,   236,   240,   243,   245,   247,   251,   256,   259,   262,
     267,   272,   277,   281,   283,   288,   290,   292,   294
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int8 yyrhs[] =
{
      48,     0,    -1,    -1,    48,    49,    -1,    50,     3,    -1,
      66,    -1,     1,     3,    -1,    -1,    52,    -1,    50,    39,
      52,    -1,    50,    39,    -1,    -1,    52,    -1,    51,     3,
      -1,    51,     3,    52,    -1,    51,    39,    -1,    51,    39,
      53,    -1,    53,    -1,     1,    53,    -1,    29,    -1,    34,
      -1,    75,    -1,     7,    -1,    15,    -1,    32,    -1,    16,
      -1,    30,    -1,    18,    -1,    18,    40,    74,    41,    -1,
      -1,    -1,    -1,    -1,    19,    54,    40,    73,    39,    55,
      73,    39,    56,    73,    41,    57,    53,    -1,    -1,    20,
      40,    75,    41,    58,    53,    64,    -1,    -1,    -1,    21,
      59,    40,    75,    60,    41,    53,    -1,    42,    51,    43,
      -1,    -1,    33,    61,    62,    -1,    63,    -1,    63,    44,
      62,    -1,     7,    -1,    75,    -1,    -1,    -1,    23,    65,
      53,    -1,    -1,    14,     8,    40,    68,    41,    42,     3,
      69,    67,    51,     3,    43,    -1,    -1,    70,    -1,    -1,
      27,    70,     3,    -1,    27,    70,    39,    -1,     8,    -1,
       8,    45,    46,    -1,    70,    44,     8,    -1,    70,    44,
       8,    45,    46,    -1,    -1,    72,    -1,    75,    -1,     8,
      45,    46,    -1,    72,    44,    75,    -1,    72,    44,     8,
      45,    46,    -1,    -1,    75,    -1,    -1,    75,    -1,    -1,
      79,    11,    76,    75,    -1,    -1,    75,     4,    77,    75,
      -1,    -1,    75,     5,    78,    75,    -1,     6,    75,    -1,
      75,    12,    75,    -1,    75,    35,    75,    -1,    75,    36,
      75,    -1,    75,    10,    75,    -1,    75,    37,    75,    -1,
      36,    75,    -1,    79,    -1,     9,    -1,    40,    75,    41,
      -1,     8,    40,    71,    41,    -1,    13,    79,    -1,    79,
      13,    -1,    17,    40,    75,    41,    -1,    22,    40,    75,
      41,    -1,    24,    40,    75,    41,    -1,    28,    40,    41,
      -1,     8,    -1,     8,    45,    75,    46,    -1,    25,    -1,
      26,    -1,    24,    -1,    31,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   106,   106,   114,   116,   118,   120,   127,   128,   129,
     130,   133,   134,   135,   136,   137,   138,   140,   141,   144,
     146,   148,   157,   164,   174,   185,   187,   189,   191,   194,
     199,   210,   221,   193,   239,   238,   252,   258,   251,   270,
     273,   272,   276,   277,   279,   285,   288,   290,   289,   300,
     298,   318,   319,   322,   323,   325,   328,   330,   332,   334,
     338,   339,   341,   346,   352,   357,   365,   369,   372,   376,
     383,   382,   409,   408,   422,   421,   437,   443,   471,   476,
     481,   488,   493,   498,   507,   523,   525,   541,   560,   583,
     585,   587,   589,   595,   597,   602,   604,   606,   608
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "NEWLINE", "AND", "OR", "NOT", "STRING",
  "NAME", "NUMBER", "MUL_OP", "ASSIGN_OP", "REL_OP", "INCR_DECR", "Define",
  "Break", "Quit", "Length", "Return", "For", "If", "While", "Sqrt",
  "Else", "Scale", "Ibase", "Obase", "Auto", "Read", "Warranty", "Halt",
  "Last", "Continue", "Print", "Limits", "'+'", "'-'", "'^'",
  "UNARY_MINUS", "';'", "'('", "')'", "'{'", "'}'", "','", "'['", "']'",
  "$accept", "program", "input_item", "semicolon_list", "statement_list",
  "statement_or_error", "statement", "@1", "@2", "@3", "@4", "@5", "@6",
  "@7", "@8", "print_list", "print_element", "opt_else", "@9", "function",
  "@10", "opt_parameter_list", "opt_auto_define_list", "define_list",
  "opt_argument_list", "argument_list", "opt_expression",
  "return_expression", "expression", "@11", "@12", "@13",
  "named_expression", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,    43,    45,    94,   290,    59,
      40,    41,   123,   125,    44,    91,    93
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    47,    48,    48,    49,    49,    49,    50,    50,    50,
      50,    51,    51,    51,    51,    51,    51,    52,    52,    53,
      53,    53,    53,    53,    53,    53,    53,    53,    53,    54,
      55,    56,    57,    53,    58,    53,    59,    60,    53,    53,
      61,    53,    62,    62,    63,    63,    64,    65,    64,    67,
      66,    68,    68,    69,    69,    69,    70,    70,    70,    70,
      71,    71,    72,    72,    72,    72,    73,    73,    74,    74,
      76,    75,    77,    75,    78,    75,    75,    75,    75,    75,
      75,    75,    75,    75,    75,    75,    75,    75,    75,    75,
      75,    75,    75,    79,    79,    79,    79,    79,    79
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     0,     2,     2,     1,     2,     0,     1,     3,
       2,     0,     1,     2,     3,     2,     3,     1,     2,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     4,     0,
       0,     0,     0,    13,     0,     7,     0,     0,     7,     3,
       0,     3,     1,     3,     1,     1,     0,     0,     3,     0,
      12,     0,     1,     0,     3,     3,     1,     3,     3,     5,
       0,     1,     1,     3,     3,     5,     0,     1,     0,     1,
       0,     4,     0,     4,     0,     4,     2,     3,     3,     3,
       3,     3,     2,     1,     1,     3,     4,     2,     2,     4,
       4,     4,     3,     1,     4,     1,     1,     1,     1
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       2,     0,     1,     0,     0,    22,    93,    84,     0,     0,
      23,    25,     0,    27,    29,     0,    36,     0,    97,    95,
      96,     0,    19,    26,    98,    24,    40,    20,     0,     0,
       0,     3,     0,     8,    17,     5,    21,    83,     6,    18,
      76,    60,     0,    93,    97,    87,     0,     0,    68,     0,
       0,     0,     0,     0,     0,     0,    82,     0,     0,     0,
      12,     4,     0,    72,    74,     0,     0,     0,     0,     0,
      70,    88,    93,     0,    61,    62,     0,    51,     0,     0,
      69,    66,     0,     0,     0,     0,    92,    44,    41,    42,
      45,    85,     0,    15,    39,     9,     0,     0,    80,    77,
      78,    79,    81,     0,     0,    86,     0,    94,    56,     0,
      52,    89,    28,     0,    67,    34,    37,    90,    91,     0,
      14,    16,    73,    75,    71,    63,    93,    64,     0,     0,
       0,    30,     0,     0,    43,     0,    57,     0,    58,    66,
      46,     0,    65,    53,     0,     0,    47,    35,    38,     0,
      49,    59,    31,     0,     0,     0,    66,    48,    54,    55,
       0,     0,     0,    32,    50,     0,    33
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,     1,    31,    32,    59,    60,    34,    49,   139,   156,
     165,   132,    51,   133,    55,    88,    89,   147,   153,    35,
     155,   109,   150,   110,    73,    74,   113,    79,    36,   103,
      96,    97,    37
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -135
static const yytype_int16 yypact[] =
{
    -135,   158,  -135,   437,   585,  -135,   -25,  -135,    72,    23,
    -135,  -135,   -11,   -10,  -135,     7,  -135,    25,    26,  -135,
    -135,    27,  -135,  -135,  -135,  -135,  -135,  -135,   585,   585,
     198,  -135,    10,  -135,  -135,  -135,    79,     8,  -135,  -135,
     116,   606,   585,    17,  -135,  -135,    28,   585,   585,    35,
     585,    37,   585,   585,     3,   564,  -135,    -1,   527,    21,
    -135,  -135,   327,  -135,  -135,   585,   585,   585,   585,   585,
    -135,  -135,    34,    41,    42,    79,     2,    70,   470,    47,
      79,   585,   479,   585,   482,   491,  -135,  -135,  -135,    48,
      79,  -135,   241,   527,  -135,  -135,   585,   585,    57,    -9,
      13,    13,    57,   585,    96,  -135,   627,  -135,    50,    58,
      62,  -135,  -135,    73,    79,  -135,    79,  -135,  -135,   564,
    -135,  -135,   116,     6,    -9,  -135,    36,    79,    65,    75,
     111,  -135,   527,    84,  -135,   402,  -135,   127,    86,   585,
     110,   527,  -135,   107,    89,    98,  -135,  -135,  -135,    70,
    -135,  -135,  -135,   527,    14,   367,   585,  -135,  -135,  -135,
      22,    99,   284,  -135,  -135,   527,  -135
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -135,  -135,  -135,  -135,   -14,     1,    -3,  -135,  -135,  -135,
    -135,  -135,  -135,  -135,  -135,    29,  -135,  -135,  -135,  -135,
    -135,  -135,  -135,    -5,  -135,  -135,  -134,  -135,     4,  -135,
    -135,  -135,   137
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -14
static const yytype_int16 yytable[] =
{
      39,    65,    33,    63,    64,   145,    63,    64,    40,    65,
      63,    66,    65,    61,    66,    41,    65,   158,    66,    70,
      42,    71,   161,    65,    92,   162,    67,    68,    69,    47,
      48,    46,    56,    57,    67,    68,    69,    67,    68,    69,
      91,    67,    68,    69,    86,    75,    76,    50,   107,    62,
      69,    78,    80,   159,    82,    39,    84,    85,   130,    90,
      93,    93,    42,    95,    94,    52,    53,    54,    77,    98,
      99,   100,   101,   102,    41,    81,    41,    83,   108,   104,
      43,   135,   105,    63,    64,   114,   106,   116,   112,    65,
     121,    66,   119,   120,    69,   128,    44,    19,    20,   129,
     122,   123,     4,    24,     6,     7,   130,   124,    76,     8,
     127,   136,   131,    12,    67,    68,    69,   137,    17,   138,
      18,    19,    20,    90,    21,   141,    65,    24,    66,   140,
     143,   144,    28,   146,   149,   151,    29,   152,   148,    76,
     163,   160,   125,   114,   154,    45,     0,     0,   134,     0,
     157,    67,    68,    69,     0,     0,     0,     0,     2,     3,
     114,    -7,   166,   120,     4,     5,     6,     7,     0,     0,
       0,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,     0,    18,    19,    20,     0,    21,    22,    23,    24,
      25,    26,    27,     0,    28,     0,     0,    -7,    29,    58,
      30,   -11,     0,     0,     4,     5,     6,     7,     0,     0,
       0,     8,     0,    10,    11,    12,    13,    14,    15,    16,
      17,     0,    18,    19,    20,     0,    21,    22,    23,    24,
      25,    26,    27,     0,    28,     0,     0,   -11,    29,     0,
      30,   -11,    58,     0,   -13,     0,     0,     4,     5,     6,
       7,     0,     0,     0,     8,     0,    10,    11,    12,    13,
      14,    15,    16,    17,     0,    18,    19,    20,     0,    21,
      22,    23,    24,    25,    26,    27,     0,    28,     0,     0,
     -13,    29,     0,    30,   -13,    58,     0,   -13,     0,     0,
       4,     5,     6,     7,     0,     0,     0,     8,     0,    10,
      11,    12,    13,    14,    15,    16,    17,     0,    18,    19,
      20,     0,    21,    22,    23,    24,    25,    26,    27,     0,
      28,     0,     0,   -13,    29,     0,    30,   164,    58,     0,
     -10,     0,     0,     4,     5,     6,     7,     0,     0,     0,
       8,     0,    10,    11,    12,    13,    14,    15,    16,    17,
       0,    18,    19,    20,     0,    21,    22,    23,    24,    25,
      26,    27,     0,    28,     0,     0,   -10,    29,    58,    30,
     -11,     0,     0,     4,     5,     6,     7,     0,     0,     0,
       8,     0,    10,    11,    12,    13,    14,    15,    16,    17,
       0,    18,    19,    20,     0,    21,    22,    23,    24,    25,
      26,    27,     0,    28,     0,     0,   -11,    29,     4,    30,
       6,     7,     0,     0,     0,     8,     0,     0,     0,    12,
       0,     0,     0,     0,    17,     0,    18,    19,    20,     0,
      21,     0,     0,    24,     0,     0,     0,     0,    28,     0,
      38,     0,    29,     4,     5,     6,     7,     0,   142,     0,
       8,     0,    10,    11,    12,    13,    14,    15,    16,    17,
       0,    18,    19,    20,     0,    21,    22,    23,    24,    25,
      26,    27,     0,    28,    63,    64,     0,    29,     0,    30,
      65,     0,    66,    63,    64,     0,    63,    64,     0,    65,
       0,    66,    65,     0,    66,    63,    64,     0,     0,     0,
       0,    65,     0,    66,     0,    67,    68,    69,     0,     0,
       0,   111,     0,     0,    67,    68,    69,    67,    68,    69,
     115,     0,     0,   117,     0,     0,    67,    68,    69,     0,
       0,     0,   118,     4,     5,     6,     7,     0,     0,     0,
       8,     0,    10,    11,    12,    13,    14,    15,    16,    17,
       0,    18,    19,    20,     0,    21,    22,    23,    24,    25,
      26,    27,     0,    28,     0,     0,     0,    29,     0,    30,
       4,    87,     6,     7,     0,     0,     0,     8,     0,     0,
       0,    12,     0,     0,     0,     0,    17,     0,    18,    19,
      20,     4,    21,     6,     7,    24,     0,     0,     8,     0,
      28,     0,    12,     0,    29,     0,     0,    17,     0,    18,
      19,    20,     4,    21,    72,     7,    24,     0,     0,     8,
       0,    28,     0,    12,     0,    29,     0,     0,    17,     0,
      18,    19,    20,     4,    21,   126,     7,    24,     0,     0,
       8,     0,    28,     0,    12,     0,    29,     0,     0,    17,
       0,    18,    19,    20,     0,    21,     0,     0,    24,     0,
       0,     0,     0,    28,     0,     0,     0,    29
};

static const yytype_int16 yycheck[] =
{
       3,    10,     1,     4,     5,   139,     4,     5,     4,    10,
       4,    12,    10,     3,    12,    40,    10,     3,    12,    11,
      45,    13,   156,    10,     3,     3,    35,    36,    37,    40,
      40,     8,    28,    29,    35,    36,    37,    35,    36,    37,
      41,    35,    36,    37,    41,    41,    42,    40,    46,    39,
      37,    47,    48,    39,    50,    58,    52,    53,    44,    55,
      39,    39,    45,    62,    43,    40,    40,    40,    40,    65,
      66,    67,    68,    69,    40,    40,    40,    40,     8,    45,
       8,    45,    41,     4,     5,    81,    44,    83,    41,    10,
      93,    12,    44,    92,    37,    45,    24,    25,    26,    41,
      96,    97,     6,    31,     8,     9,    44,   103,   104,    13,
     106,    46,    39,    17,    35,    36,    37,    42,    22,     8,
      24,    25,    26,   119,    28,    41,    10,    31,    12,   132,
       3,    45,    36,    23,    27,    46,    40,    39,   141,   135,
      41,   155,    46,   139,   149,     8,    -1,    -1,   119,    -1,
     153,    35,    36,    37,    -1,    -1,    -1,    -1,     0,     1,
     156,     3,   165,   162,     6,     7,     8,     9,    -1,    -1,
      -1,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,    -1,    24,    25,    26,    -1,    28,    29,    30,    31,
      32,    33,    34,    -1,    36,    -1,    -1,    39,    40,     1,
      42,     3,    -1,    -1,     6,     7,     8,     9,    -1,    -1,
      -1,    13,    -1,    15,    16,    17,    18,    19,    20,    21,
      22,    -1,    24,    25,    26,    -1,    28,    29,    30,    31,
      32,    33,    34,    -1,    36,    -1,    -1,    39,    40,    -1,
      42,    43,     1,    -1,     3,    -1,    -1,     6,     7,     8,
       9,    -1,    -1,    -1,    13,    -1,    15,    16,    17,    18,
      19,    20,    21,    22,    -1,    24,    25,    26,    -1,    28,
      29,    30,    31,    32,    33,    34,    -1,    36,    -1,    -1,
      39,    40,    -1,    42,    43,     1,    -1,     3,    -1,    -1,
       6,     7,     8,     9,    -1,    -1,    -1,    13,    -1,    15,
      16,    17,    18,    19,    20,    21,    22,    -1,    24,    25,
      26,    -1,    28,    29,    30,    31,    32,    33,    34,    -1,
      36,    -1,    -1,    39,    40,    -1,    42,    43,     1,    -1,
       3,    -1,    -1,     6,     7,     8,     9,    -1,    -1,    -1,
      13,    -1,    15,    16,    17,    18,    19,    20,    21,    22,
      -1,    24,    25,    26,    -1,    28,    29,    30,    31,    32,
      33,    34,    -1,    36,    -1,    -1,    39,    40,     1,    42,
       3,    -1,    -1,     6,     7,     8,     9,    -1,    -1,    -1,
      13,    -1,    15,    16,    17,    18,    19,    20,    21,    22,
      -1,    24,    25,    26,    -1,    28,    29,    30,    31,    32,
      33,    34,    -1,    36,    -1,    -1,    39,    40,     6,    42,
       8,     9,    -1,    -1,    -1,    13,    -1,    -1,    -1,    17,
      -1,    -1,    -1,    -1,    22,    -1,    24,    25,    26,    -1,
      28,    -1,    -1,    31,    -1,    -1,    -1,    -1,    36,    -1,
       3,    -1,    40,     6,     7,     8,     9,    -1,    46,    -1,
      13,    -1,    15,    16,    17,    18,    19,    20,    21,    22,
      -1,    24,    25,    26,    -1,    28,    29,    30,    31,    32,
      33,    34,    -1,    36,     4,     5,    -1,    40,    -1,    42,
      10,    -1,    12,     4,     5,    -1,     4,     5,    -1,    10,
      -1,    12,    10,    -1,    12,     4,     5,    -1,    -1,    -1,
      -1,    10,    -1,    12,    -1,    35,    36,    37,    -1,    -1,
      -1,    41,    -1,    -1,    35,    36,    37,    35,    36,    37,
      41,    -1,    -1,    41,    -1,    -1,    35,    36,    37,    -1,
      -1,    -1,    41,     6,     7,     8,     9,    -1,    -1,    -1,
      13,    -1,    15,    16,    17,    18,    19,    20,    21,    22,
      -1,    24,    25,    26,    -1,    28,    29,    30,    31,    32,
      33,    34,    -1,    36,    -1,    -1,    -1,    40,    -1,    42,
       6,     7,     8,     9,    -1,    -1,    -1,    13,    -1,    -1,
      -1,    17,    -1,    -1,    -1,    -1,    22,    -1,    24,    25,
      26,     6,    28,     8,     9,    31,    -1,    -1,    13,    -1,
      36,    -1,    17,    -1,    40,    -1,    -1,    22,    -1,    24,
      25,    26,     6,    28,     8,     9,    31,    -1,    -1,    13,
      -1,    36,    -1,    17,    -1,    40,    -1,    -1,    22,    -1,
      24,    25,    26,     6,    28,     8,     9,    31,    -1,    -1,
      13,    -1,    36,    -1,    17,    -1,    40,    -1,    -1,    22,
      -1,    24,    25,    26,    -1,    28,    -1,    -1,    31,    -1,
      -1,    -1,    -1,    36,    -1,    -1,    -1,    40
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,    48,     0,     1,     6,     7,     8,     9,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    24,    25,
      26,    28,    29,    30,    31,    32,    33,    34,    36,    40,
      42,    49,    50,    52,    53,    66,    75,    79,     3,    53,
      75,    40,    45,     8,    24,    79,     8,    40,    40,    54,
      40,    59,    40,    40,    40,    61,    75,    75,     1,    51,
      52,     3,    39,     4,     5,    10,    12,    35,    36,    37,
      11,    13,     8,    71,    72,    75,    75,    40,    75,    74,
      75,    40,    75,    40,    75,    75,    41,     7,    62,    63,
      75,    41,     3,    39,    43,    52,    77,    78,    75,    75,
      75,    75,    75,    76,    45,    41,    44,    46,     8,    68,
      70,    41,    41,    73,    75,    41,    75,    41,    41,    44,
      52,    53,    75,    75,    75,    46,     8,    75,    45,    41,
      44,    39,    58,    60,    62,    45,    46,    42,     8,    55,
      53,    41,    46,     3,    45,    73,    23,    64,    53,    27,
      69,    46,    39,    65,    70,    67,    56,    53,     3,    39,
      51,    73,     3,    41,    43,    57,    53
};

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */

#define YYFAIL		goto yyerrlab

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      yytoken = YYTRANSLATE (yychar);				\
      YYPOPSTACK (1);						\
      goto yybackup;						\
    }								\
  else								\
    {								\
      yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (YYID (0))


#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)				\
    do									\
      if (YYID (N))                                                    \
	{								\
	  (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;	\
	  (Current).first_column = YYRHSLOC (Rhs, 1).first_column;	\
	  (Current).last_line    = YYRHSLOC (Rhs, N).last_line;		\
	  (Current).last_column  = YYRHSLOC (Rhs, N).last_column;	\
	}								\
      else								\
	{								\
	  (Current).first_line   = (Current).last_line   =		\
	    YYRHSLOC (Rhs, 0).last_line;				\
	  (Current).first_column = (Current).last_column =		\
	    YYRHSLOC (Rhs, 0).last_column;				\
	}								\
    while (YYID (0))
#endif


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL
#  define YY_LOCATION_PRINT(File, Loc)			\
     fprintf (File, "%d.%d-%d.%d",			\
	      (Loc).first_line, (Loc).first_column,	\
	      (Loc).last_line,  (Loc).last_column)
# else
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (YYLEX_PARAM)
#else
# define YYLEX yylex ()
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (YYID (0))

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)			  \
do {									  \
  if (yydebug)								  \
    {									  \
      YYFPRINTF (stderr, "%s ", Title);					  \
      yy_symbol_print (stderr,						  \
		  Type, Value); \
      YYFPRINTF (stderr, "\n");						  \
    }									  \
} while (YYID (0))


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_value_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# else
  YYUSE (yyoutput);
# endif
  switch (yytype)
    {
      default:
	break;
    }
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_stack_print (yytype_int16 *bottom, yytype_int16 *top)
#else
static void
yy_stack_print (bottom, top)
    yytype_int16 *bottom;
    yytype_int16 *top;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; bottom <= top; ++bottom)
    YYFPRINTF (stderr, " %d", *bottom);
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (YYID (0))


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_reduce_print (YYSTYPE *yyvsp, int yyrule)
#else
static void
yy_reduce_print (yyvsp, yyrule)
    YYSTYPE *yyvsp;
    int yyrule;
#endif
{
  int yynrhs = yyr2[yyrule];
  int yyi;
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
	     yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      fprintf (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr, yyrhs[yyprhs[yyrule] + yyi],
		       &(yyvsp[(yyi + 1) - (yynrhs)])
		       		       );
      fprintf (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (yyvsp, Rule); \
} while (YYID (0))

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif



#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static YYSIZE_T
yystrlen (const char *yystr)
#else
static YYSIZE_T
yystrlen (yystr)
    const char *yystr;
#endif
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static char *
yystpcpy (char *yydest, const char *yysrc)
#else
static char *
yystpcpy (yydest, yysrc)
    char *yydest;
    const char *yysrc;
#endif
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
	switch (*++yyp)
	  {
	  case '\'':
	  case ',':
	    goto do_not_strip_quotes;

	  case '\\':
	    if (*++yyp != '\\')
	      goto do_not_strip_quotes;
	    /* Fall through.  */
	  default:
	    if (yyres)
	      yyres[yyn] = *yyp;
	    yyn++;
	    break;

	  case '"':
	    if (yyres)
	      yyres[yyn] = '\0';
	    return yyn;
	  }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

/* Copy into YYRESULT an error message about the unexpected token
   YYCHAR while in state YYSTATE.  Return the number of bytes copied,
   including the terminating null byte.  If YYRESULT is null, do not
   copy anything; just return the number of bytes that would be
   copied.  As a special case, return 0 if an ordinary "syntax error"
   message will do.  Return YYSIZE_MAXIMUM if overflow occurs during
   size calculation.  */
static YYSIZE_T
yysyntax_error (char *yyresult, int yystate, int yychar)
{
  int yyn = yypact[yystate];

  if (! (YYPACT_NINF < yyn && yyn <= YYLAST))
    return 0;
  else
    {
      int yytype = YYTRANSLATE (yychar);
      YYSIZE_T yysize0 = yytnamerr (0, yytname[yytype]);
      YYSIZE_T yysize = yysize0;
      YYSIZE_T yysize1;
      int yysize_overflow = 0;
      enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
      char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
      int yyx;

# if 0
      /* This is so xgettext sees the translatable formats that are
	 constructed on the fly.  */
      YY_("syntax error, unexpected %s");
      YY_("syntax error, unexpected %s, expecting %s");
      YY_("syntax error, unexpected %s, expecting %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s");
      YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s");
# endif
      char *yyfmt;
      char const *yyf;
      static char const yyunexpected[] = "syntax error, unexpected %s";
      static char const yyexpecting[] = ", expecting %s";
      static char const yyor[] = " or %s";
      char yyformat[sizeof yyunexpected
		    + sizeof yyexpecting - 1
		    + ((YYERROR_VERBOSE_ARGS_MAXIMUM - 2)
		       * (sizeof yyor - 1))];
      char const *yyprefix = yyexpecting;

      /* Start YYX at -YYN if negative to avoid negative indexes in
	 YYCHECK.  */
      int yyxbegin = yyn < 0 ? -yyn : 0;

      /* Stay within bounds of both yycheck and yytname.  */
      int yychecklim = YYLAST - yyn + 1;
      int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
      int yycount = 1;

      yyarg[0] = yytname[yytype];
      yyfmt = yystpcpy (yyformat, yyunexpected);

      for (yyx = yyxbegin; yyx < yyxend; ++yyx)
	if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	  {
	    if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
	      {
		yycount = 1;
		yysize = yysize0;
		yyformat[sizeof yyunexpected - 1] = '\0';
		break;
	      }
	    yyarg[yycount++] = yytname[yyx];
	    yysize1 = yysize + yytnamerr (0, yytname[yyx]);
	    yysize_overflow |= (yysize1 < yysize);
	    yysize = yysize1;
	    yyfmt = yystpcpy (yyfmt, yyprefix);
	    yyprefix = yyor;
	  }

      yyf = YY_(yyformat);
      yysize1 = yysize + yystrlen (yyf);
      yysize_overflow |= (yysize1 < yysize);
      yysize = yysize1;

      if (yysize_overflow)
	return YYSIZE_MAXIMUM;

      if (yyresult)
	{
	  /* Avoid sprintf, as that infringes on the user's name space.
	     Don't have undefined behavior even if the translation
	     produced a string with the wrong number of "%s"s.  */
	  char *yyp = yyresult;
	  int yyi = 0;
	  while ((*yyp = *yyf) != '\0')
	    {
	      if (*yyp == '%' && yyf[1] == 's' && yyi < yycount)
		{
		  yyp += yytnamerr (yyp, yyarg[yyi++]);
		  yyf += 2;
		}
	      else
		{
		  yyp++;
		  yyf++;
		}
	    }
	}
      return yysize;
    }
}
#endif /* YYERROR_VERBOSE */


/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep)
#else
static void
yydestruct (yymsg, yytype, yyvaluep)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  YYUSE (yyvaluep);

  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  switch (yytype)
    {

      default:
	break;
    }
}


/* Prevent warnings from -Wmissing-prototypes.  */

#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int yyparse (void *YYPARSE_PARAM);
#else
int yyparse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int yyparse (void);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */



/* The look-ahead symbol.  */
int yychar;

/* The semantic value of the look-ahead symbol.  */
YYSTYPE yylval;

/* Number of syntax errors so far.  */
int yynerrs;



/*----------.
| yyparse.  |
`----------*/

#ifdef YYPARSE_PARAM
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void *YYPARSE_PARAM)
#else
int
yyparse (YYPARSE_PARAM)
    void *YYPARSE_PARAM;
#endif
#else /* ! YYPARSE_PARAM */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void)
#else
int
yyparse ()

#endif
#endif
{
  
  int yystate;
  int yyn;
  int yyresult;
  /* Number of tokens to shift before error messages enabled.  */
  int yyerrstatus;
  /* Look-ahead token as an internal (translated) token number.  */
  int yytoken = 0;
#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

  /* Three stacks and their tools:
     `yyss': related to states,
     `yyvs': related to semantic values,
     `yyls': related to locations.

     Refer to the stacks thru separate pointers, to allow yyoverflow
     to reallocate them elsewhere.  */

  /* The state stack.  */
  yytype_int16 yyssa[YYINITDEPTH];
  yytype_int16 *yyss = yyssa;
  yytype_int16 *yyssp;

  /* The semantic value stack.  */
  YYSTYPE yyvsa[YYINITDEPTH];
  YYSTYPE *yyvs = yyvsa;
  YYSTYPE *yyvsp;



#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  YYSIZE_T yystacksize = YYINITDEPTH;

  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;


  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY;		/* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  yyssp = yyss;
  yyvsp = yyvs;

  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack.  Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	yytype_int16 *yyss1 = yyss;


	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow (YY_("memory exhausted"),
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),

		    &yystacksize);

	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	yytype_int16 *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyexhaustedlab;
	YYSTACK_RELOCATE (yyss);
	YYSTACK_RELOCATE (yyvs);

#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;


      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

  /* Do appropriate processing given the current state.  Read a
     look-ahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to look-ahead token.  */
  yyn = yypact[yystate];
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a look-ahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid look-ahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yyn == 0 || yyn == YYTABLE_NINF)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the look-ahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  yystate = yyn;
  *++yyvsp = yylval;

  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 2:
#line 106 "bc.y"
    {
			      (yyval.i_value) = 0;
			      if (interactive)
				{
				  printf ("%s\n", BC_VERSION);
				  welcome ();
				}
			    }
    break;

  case 4:
#line 117 "bc.y"
    { run_code (); }
    break;

  case 5:
#line 119 "bc.y"
    { run_code (); }
    break;

  case 6:
#line 121 "bc.y"
    {
			      yyerrok;
			      init_gen ();
			    }
    break;

  case 7:
#line 127 "bc.y"
    { (yyval.i_value) = 0; }
    break;

  case 11:
#line 133 "bc.y"
    { (yyval.i_value) = 0; }
    break;

  case 18:
#line 142 "bc.y"
    { (yyval.i_value) = (yyvsp[(2) - (2)].i_value); }
    break;

  case 19:
#line 145 "bc.y"
    { warranty (""); }
    break;

  case 20:
#line 147 "bc.y"
    { limits (); }
    break;

  case 21:
#line 149 "bc.y"
    {
			      if ((yyvsp[(1) - (1)].i_value) & 2)
				warn ("comparison in expression");
			      if ((yyvsp[(1) - (1)].i_value) & 1)
				generate ("W");
			      else 
				generate ("p");
			    }
    break;

  case 22:
#line 158 "bc.y"
    {
			      (yyval.i_value) = 0;
			      generate ("w");
			      generate ((yyvsp[(1) - (1)].s_value));
			      free ((yyvsp[(1) - (1)].s_value));
			    }
    break;

  case 23:
#line 165 "bc.y"
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

  case 24:
#line 175 "bc.y"
    {
			      warn ("Continue statement");
			      if (continue_label == 0)
				yyerror ("Continue outside a for");
			      else
				{
				  sprintf (genstr, "J%1d:", continue_label);
				  generate (genstr);
				}
			    }
    break;

  case 25:
#line 186 "bc.y"
    { exit (0); }
    break;

  case 26:
#line 188 "bc.y"
    { generate ("h"); }
    break;

  case 27:
#line 190 "bc.y"
    { generate ("0R"); }
    break;

  case 28:
#line 192 "bc.y"
    { generate ("R"); }
    break;

  case 29:
#line 194 "bc.y"
    {
			      (yyvsp[(1) - (1)].i_value) = break_label; 
			      break_label = next_label++;
			    }
    break;

  case 30:
#line 199 "bc.y"
    {
			      if ((yyvsp[(4) - (5)].i_value) > 1)
				warn ("Comparison in first for expression");
			      (yyvsp[(4) - (5)].i_value) = next_label++;
			      if ((yyvsp[(4) - (5)].i_value) < 0)
				sprintf (genstr, "N%1d:", (yyvsp[(4) - (5)].i_value));
			      else
				sprintf (genstr, "pN%1d:", (yyvsp[(4) - (5)].i_value));
			      generate (genstr);
			    }
    break;

  case 31:
#line 210 "bc.y"
    {
			      if ((yyvsp[(7) - (8)].i_value) < 0) generate ("1");
			      (yyvsp[(7) - (8)].i_value) = next_label++;
			      sprintf (genstr, "B%1d:J%1d:", (yyvsp[(7) - (8)].i_value), break_label);
			      generate (genstr);
			      (yyval.i_value) = continue_label;
			      continue_label = next_label++;
			      sprintf (genstr, "N%1d:", continue_label);
			      generate (genstr);
			    }
    break;

  case 32:
#line 221 "bc.y"
    {
			      if ((yyvsp[(10) - (11)].i_value) > 1)
				warn ("Comparison in third for expression");
			      if ((yyvsp[(10) - (11)].i_value) < 0)
				sprintf (genstr, "J%1d:N%1d:", (yyvsp[(4) - (11)].i_value), (yyvsp[(7) - (11)].i_value));
			      else
				sprintf (genstr, "pJ%1d:N%1d:", (yyvsp[(4) - (11)].i_value), (yyvsp[(7) - (11)].i_value));
			      generate (genstr);
			    }
    break;

  case 33:
#line 231 "bc.y"
    {
			      sprintf (genstr, "J%1d:N%1d:",
				       continue_label, break_label);
			      generate (genstr);
			      break_label = (yyvsp[(1) - (13)].i_value);
			      continue_label = (yyvsp[(9) - (13)].i_value);
			    }
    break;

  case 34:
#line 239 "bc.y"
    {
			      (yyvsp[(3) - (4)].i_value) = if_label;
			      if_label = next_label++;
			      sprintf (genstr, "Z%1d:", if_label);
			      generate (genstr);
			    }
    break;

  case 35:
#line 246 "bc.y"
    {
			      sprintf (genstr, "N%1d:", if_label); 
			      generate (genstr);
			      if_label = (yyvsp[(3) - (7)].i_value);
			    }
    break;

  case 36:
#line 252 "bc.y"
    {
			      (yyvsp[(1) - (1)].i_value) = next_label++;
			      sprintf (genstr, "N%1d:", (yyvsp[(1) - (1)].i_value));
			      generate (genstr);
			    }
    break;

  case 37:
#line 258 "bc.y"
    {
			      (yyvsp[(4) - (4)].i_value) = break_label; 
			      break_label = next_label++;
			      sprintf (genstr, "Z%1d:", break_label);
			      generate (genstr);
			    }
    break;

  case 38:
#line 265 "bc.y"
    {
			      sprintf (genstr, "J%1d:N%1d:", (yyvsp[(1) - (7)].i_value), break_label);
			      generate (genstr);
			      break_label = (yyvsp[(4) - (7)].i_value);
			    }
    break;

  case 39:
#line 271 "bc.y"
    { (yyval.i_value) = 0; }
    break;

  case 40:
#line 273 "bc.y"
    {  warn ("print statement"); }
    break;

  case 44:
#line 280 "bc.y"
    {
			      generate ("O");
			      generate ((yyvsp[(1) - (1)].s_value));
			      free ((yyvsp[(1) - (1)].s_value));
			    }
    break;

  case 45:
#line 286 "bc.y"
    { generate ("P"); }
    break;

  case 47:
#line 290 "bc.y"
    {
			      warn ("else clause in if statement");
			      (yyvsp[(1) - (1)].i_value) = next_label++;
			      sprintf (genstr, "J%d:N%1d:", (yyvsp[(1) - (1)].i_value), if_label); 
			      generate (genstr);
			      if_label = (yyvsp[(1) - (1)].i_value);
			    }
    break;

  case 49:
#line 300 "bc.y"
    {
			      /* Check auto list against parameter list? */
			      check_params ((yyvsp[(4) - (8)].a_value),(yyvsp[(8) - (8)].a_value));
			      sprintf (genstr, "F%d,%s.%s[", lookup((yyvsp[(2) - (8)].s_value),FUNCT), 
				       arg_str ((yyvsp[(4) - (8)].a_value),TRUE), arg_str ((yyvsp[(8) - (8)].a_value),TRUE));
			      generate (genstr);
			      free_args ((yyvsp[(4) - (8)].a_value));
			      free_args ((yyvsp[(8) - (8)].a_value));
			      (yyvsp[(1) - (8)].i_value) = next_label;
			      next_label = 0;
			    }
    break;

  case 50:
#line 312 "bc.y"
    {
			      generate ("0R]");
			      next_label = (yyvsp[(1) - (12)].i_value);
			    }
    break;

  case 51:
#line 318 "bc.y"
    { (yyval.a_value) = NULL; }
    break;

  case 53:
#line 322 "bc.y"
    { (yyval.a_value) = NULL; }
    break;

  case 54:
#line 324 "bc.y"
    { (yyval.a_value) = (yyvsp[(2) - (3)].a_value); }
    break;

  case 55:
#line 326 "bc.y"
    { (yyval.a_value) = (yyvsp[(2) - (3)].a_value); }
    break;

  case 56:
#line 329 "bc.y"
    { (yyval.a_value) = nextarg (NULL, lookup ((yyvsp[(1) - (1)].s_value),SIMPLE)); }
    break;

  case 57:
#line 331 "bc.y"
    { (yyval.a_value) = nextarg (NULL, lookup ((yyvsp[(1) - (3)].s_value),ARRAY)); }
    break;

  case 58:
#line 333 "bc.y"
    { (yyval.a_value) = nextarg ((yyvsp[(1) - (3)].a_value), lookup ((yyvsp[(3) - (3)].s_value),SIMPLE)); }
    break;

  case 59:
#line 335 "bc.y"
    { (yyval.a_value) = nextarg ((yyvsp[(1) - (5)].a_value), lookup ((yyvsp[(3) - (5)].s_value),ARRAY)); }
    break;

  case 60:
#line 338 "bc.y"
    { (yyval.a_value) = NULL; }
    break;

  case 62:
#line 342 "bc.y"
    {
			      if ((yyvsp[(1) - (1)].i_value) > 1) warn ("comparison in argument");
			      (yyval.a_value) = nextarg (NULL,0);
			    }
    break;

  case 63:
#line 347 "bc.y"
    {
			      sprintf (genstr, "K%d:", -lookup ((yyvsp[(1) - (3)].s_value),ARRAY));
			      generate (genstr);
			      (yyval.a_value) = nextarg (NULL,1);
			    }
    break;

  case 64:
#line 353 "bc.y"
    {
			      if ((yyvsp[(3) - (3)].i_value) > 1) warn ("comparison in argument");
			      (yyval.a_value) = nextarg ((yyvsp[(1) - (3)].a_value),0);
			    }
    break;

  case 65:
#line 358 "bc.y"
    {
			      sprintf (genstr, "K%d:", -lookup ((yyvsp[(3) - (5)].s_value),ARRAY));
			      generate (genstr);
			      (yyval.a_value) = nextarg ((yyvsp[(1) - (5)].a_value),1);
			    }
    break;

  case 66:
#line 365 "bc.y"
    {
			      (yyval.i_value) = -1;
			      warn ("Missing expression in for statement");
			    }
    break;

  case 68:
#line 372 "bc.y"
    {
			      (yyval.i_value) = 0;
			      generate ("0");
			    }
    break;

  case 69:
#line 377 "bc.y"
    {
			      if ((yyvsp[(1) - (1)].i_value) > 1)
				warn ("comparison in return expresion");
			    }
    break;

  case 70:
#line 383 "bc.y"
    {
			      if ((yyvsp[(2) - (2)].c_value) != '=')
				{
				  if ((yyvsp[(1) - (2)].i_value) < 0)
				    sprintf (genstr, "DL%d:", -(yyvsp[(1) - (2)].i_value));
				  else
				    sprintf (genstr, "l%d:", (yyvsp[(1) - (2)].i_value));
				  generate (genstr);
				}
			    }
    break;

  case 71:
#line 394 "bc.y"
    {
			      if ((yyvsp[(4) - (4)].i_value) > 1) warn("comparison in assignment");
			      if ((yyvsp[(2) - (4)].c_value) != '=')
				{
				  sprintf (genstr, "%c", (yyvsp[(2) - (4)].c_value));
				  generate (genstr);
				}
			      if ((yyvsp[(1) - (4)].i_value) < 0)
				sprintf (genstr, "S%d:", -(yyvsp[(1) - (4)].i_value));
			      else
				sprintf (genstr, "s%d:", (yyvsp[(1) - (4)].i_value));
			      generate (genstr);
			      (yyval.i_value) = 0;
			    }
    break;

  case 72:
#line 409 "bc.y"
    {
			      warn("&& operator");
			      (yyvsp[(2) - (2)].i_value) = next_label++;
			      sprintf (genstr, "DZ%d:p", (yyvsp[(2) - (2)].i_value));
			      generate (genstr);
			    }
    break;

  case 73:
#line 416 "bc.y"
    {
			      sprintf (genstr, "DZ%d:p1N%d:", (yyvsp[(2) - (4)].i_value), (yyvsp[(2) - (4)].i_value));
			      generate (genstr);
			      (yyval.i_value) = (yyvsp[(1) - (4)].i_value) | (yyvsp[(4) - (4)].i_value);
			    }
    break;

  case 74:
#line 422 "bc.y"
    {
			      warn("|| operator");
			      (yyvsp[(2) - (2)].i_value) = next_label++;
			      sprintf (genstr, "B%d:", (yyvsp[(2) - (2)].i_value));
			      generate (genstr);
			    }
    break;

  case 75:
#line 429 "bc.y"
    {
			      int tmplab;
			      tmplab = next_label++;
			      sprintf (genstr, "B%d:0J%d:N%d:1N%d:",
				       (yyvsp[(2) - (4)].i_value), tmplab, (yyvsp[(2) - (4)].i_value), tmplab);
			      generate (genstr);
			      (yyval.i_value) = (yyvsp[(1) - (4)].i_value) | (yyvsp[(4) - (4)].i_value);
			    }
    break;

  case 76:
#line 438 "bc.y"
    {
			      (yyval.i_value) = (yyvsp[(2) - (2)].i_value);
			      warn("! operator");
			      generate ("!");
			    }
    break;

  case 77:
#line 444 "bc.y"
    {
			      (yyval.i_value) = 3;
			      switch (*((yyvsp[(2) - (3)].s_value)))
				{
				case '=':
				  generate ("=");
				  break;

				case '!':
				  generate ("#");
				  break;

				case '<':
				  if ((yyvsp[(2) - (3)].s_value)[1] == '=')
				    generate ("{");
				  else
				    generate ("<");
				  break;

				case '>':
				  if ((yyvsp[(2) - (3)].s_value)[1] == '=')
				    generate ("}");
				  else
				    generate (">");
				  break;
				}
			    }
    break;

  case 78:
#line 472 "bc.y"
    {
			      generate ("+");
			      (yyval.i_value) = (yyvsp[(1) - (3)].i_value) | (yyvsp[(3) - (3)].i_value);
			    }
    break;

  case 79:
#line 477 "bc.y"
    {
			      generate ("-");
			      (yyval.i_value) = (yyvsp[(1) - (3)].i_value) | (yyvsp[(3) - (3)].i_value);
			    }
    break;

  case 80:
#line 482 "bc.y"
    {
			      genstr[0] = (yyvsp[(2) - (3)].c_value);
			      genstr[1] = 0;
			      generate (genstr);
			      (yyval.i_value) = (yyvsp[(1) - (3)].i_value) | (yyvsp[(3) - (3)].i_value);
			    }
    break;

  case 81:
#line 489 "bc.y"
    {
			      generate ("^");
			      (yyval.i_value) = (yyvsp[(1) - (3)].i_value) | (yyvsp[(3) - (3)].i_value);
			    }
    break;

  case 82:
#line 494 "bc.y"
    {
			      generate ("n");
			      (yyval.i_value) = (yyvsp[(2) - (2)].i_value);
			    }
    break;

  case 83:
#line 499 "bc.y"
    {
			      (yyval.i_value) = 1;
			      if ((yyvsp[(1) - (1)].i_value) < 0)
				sprintf (genstr, "L%d:", -(yyvsp[(1) - (1)].i_value));
			      else
				sprintf (genstr, "l%d:", (yyvsp[(1) - (1)].i_value));
			      generate (genstr);
			    }
    break;

  case 84:
#line 508 "bc.y"
    {
			      int len = strlen((yyvsp[(1) - (1)].s_value));
			      (yyval.i_value) = 1;
			      if (len == 1 && *(yyvsp[(1) - (1)].s_value) == '0')
				generate ("0");
			      else if (len == 1 && *(yyvsp[(1) - (1)].s_value) == '1')
				generate ("1");
			      else
				{
				  generate ("K");
				  generate ((yyvsp[(1) - (1)].s_value));
				  generate (":");
				}
			      free ((yyvsp[(1) - (1)].s_value));
			    }
    break;

  case 85:
#line 524 "bc.y"
    { (yyval.i_value) = (yyvsp[(2) - (3)].i_value) | 1; }
    break;

  case 86:
#line 526 "bc.y"
    {
			      (yyval.i_value) = 1;
			      if ((yyvsp[(3) - (4)].a_value) != NULL)
				{ 
				  sprintf (genstr, "C%d,%s:",
					   lookup ((yyvsp[(1) - (4)].s_value),FUNCT),
					   arg_str ((yyvsp[(3) - (4)].a_value),FALSE));
				  free_args ((yyvsp[(3) - (4)].a_value));
				}
			      else
				{
				  sprintf (genstr, "C%d:", lookup ((yyvsp[(1) - (4)].s_value),FUNCT));
				}
			      generate (genstr);
			    }
    break;

  case 87:
#line 542 "bc.y"
    {
			      (yyval.i_value) = 1;
			      if ((yyvsp[(2) - (2)].i_value) < 0)
				{
				  if ((yyvsp[(1) - (2)].c_value) == '+')
				    sprintf (genstr, "DA%d:L%d:", -(yyvsp[(2) - (2)].i_value), -(yyvsp[(2) - (2)].i_value));
				  else
				    sprintf (genstr, "DM%d:L%d:", -(yyvsp[(2) - (2)].i_value), -(yyvsp[(2) - (2)].i_value));
				}
			      else
				{
				  if ((yyvsp[(1) - (2)].c_value) == '+')
				    sprintf (genstr, "i%d:l%d:", (yyvsp[(2) - (2)].i_value), (yyvsp[(2) - (2)].i_value));
				  else
				    sprintf (genstr, "d%d:l%d:", (yyvsp[(2) - (2)].i_value), (yyvsp[(2) - (2)].i_value));
				}
			      generate (genstr);
			    }
    break;

  case 88:
#line 561 "bc.y"
    {
			      (yyval.i_value) = 1;
			      if ((yyvsp[(1) - (2)].i_value) < 0)
				{
				  sprintf (genstr, "DL%d:x", -(yyvsp[(1) - (2)].i_value));
				  generate (genstr); 
				  if ((yyvsp[(2) - (2)].c_value) == '+')
				    sprintf (genstr, "A%d:", -(yyvsp[(1) - (2)].i_value));
				  else
				      sprintf (genstr, "M%d:", -(yyvsp[(1) - (2)].i_value));
				}
			      else
				{
				  sprintf (genstr, "l%d:", (yyvsp[(1) - (2)].i_value));
				  generate (genstr);
				  if ((yyvsp[(2) - (2)].c_value) == '+')
				    sprintf (genstr, "i%d:", (yyvsp[(1) - (2)].i_value));
				  else
				    sprintf (genstr, "d%d:", (yyvsp[(1) - (2)].i_value));
				}
			      generate (genstr);
			    }
    break;

  case 89:
#line 584 "bc.y"
    { generate ("cL"); (yyval.i_value) = 1;}
    break;

  case 90:
#line 586 "bc.y"
    { generate ("cR"); (yyval.i_value) = 1;}
    break;

  case 91:
#line 588 "bc.y"
    { generate ("cS"); (yyval.i_value) = 1;}
    break;

  case 92:
#line 590 "bc.y"
    {
			      warn ("read function");
			      generate ("cI"); (yyval.i_value) = 1;
			    }
    break;

  case 93:
#line 596 "bc.y"
    { (yyval.i_value) = lookup((yyvsp[(1) - (1)].s_value),SIMPLE); }
    break;

  case 94:
#line 598 "bc.y"
    {
			      if ((yyvsp[(3) - (4)].i_value) > 1) warn("comparison in subscript");
			      (yyval.i_value) = lookup((yyvsp[(1) - (4)].s_value),ARRAY);
			    }
    break;

  case 95:
#line 603 "bc.y"
    { (yyval.i_value) = 0; }
    break;

  case 96:
#line 605 "bc.y"
    { (yyval.i_value) = 1; }
    break;

  case 97:
#line 607 "bc.y"
    { (yyval.i_value) = 2; }
    break;

  case 98:
#line 609 "bc.y"
    { (yyval.i_value) = 3; }
    break;


/* Line 1267 of yacc.c.  */
#line 2376 "y.tab.c"
      default: break;
    }
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;


  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
#else
      {
	YYSIZE_T yysize = yysyntax_error (0, yystate, yychar);
	if (yymsg_alloc < yysize && yymsg_alloc < YYSTACK_ALLOC_MAXIMUM)
	  {
	    YYSIZE_T yyalloc = 2 * yysize;
	    if (! (yysize <= yyalloc && yyalloc <= YYSTACK_ALLOC_MAXIMUM))
	      yyalloc = YYSTACK_ALLOC_MAXIMUM;
	    if (yymsg != yymsgbuf)
	      YYSTACK_FREE (yymsg);
	    yymsg = (char *) YYSTACK_ALLOC (yyalloc);
	    if (yymsg)
	      yymsg_alloc = yyalloc;
	    else
	      {
		yymsg = yymsgbuf;
		yymsg_alloc = sizeof yymsgbuf;
	      }
	  }

	if (0 < yysize && yysize <= yymsg_alloc)
	  {
	    (void) yysyntax_error (yymsg, yystate, yychar);
	    yyerror (yymsg);
	  }
	else
	  {
	    yyerror (YY_("syntax error"));
	    if (yysize != 0)
	      goto yyexhaustedlab;
	  }
      }
#endif
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse look-ahead token after an
	 error, discard it.  */

      if (yychar <= YYEOF)
	{
	  /* Return failure if at end of input.  */
	  if (yychar == YYEOF)
	    YYABORT;
	}
      else
	{
	  yydestruct ("Error: discarding",
		      yytoken, &yylval);
	  yychar = YYEMPTY;
	}
    }

  /* Else will try to reuse look-ahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  /* Do not reclaim the symbols of the rule which action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (yyn != YYPACT_NINF)
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
	    {
	      yyn = yytable[yyn];
	      if (0 < yyn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
	YYABORT;


      yydestruct ("Error: popping",
		  yystos[yystate], yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  *++yyvsp = yylval;


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#ifndef yyoverflow
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEOF && yychar != YYEMPTY)
     yydestruct ("Cleanup: discarding lookahead",
		 yytoken, &yylval);
  /* Do not reclaim the symbols of the rule which action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  /* Make sure YYID is used.  */
  return YYID (yyresult);
}


#line 611 "bc.y"


