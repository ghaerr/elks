/* test - test conditions		Author: Erik Baalbergen */

/* Test-	version 7-like test(1).
 *
 * Grammar:	expr	::= bexpr | bexpr "-o" expr ;
 *		bexpr	::= primary | primary "-a" bexpr ;
 *		primary	::= unary-operator operand
 *			| operand binary-operator operand
 *			| operand
 *			| "(" expr ")"
 *			| "!" expr
 *			;
 *		unary-operator ::= "-r"|"-w"|"-f"|"-d"|"-s"|"-t"|"-z"|"-n"|"-x";
 *		binary-operator ::= "="|"!="|"-eq"|"-ne"|"-ge"|"-gt"|"-le"|"-lt";
 *		operand ::= <any legal UNIX file name>
 *
 * Author:	Erik Baalbergen	erikb@cs.vu.nl
 *
 * History:	Fix Bert Reuling #571 (03/09/89 FVK)	bert@kyber.UUCP
 *
 * 		Add Jeroen van der Pluijm  09/25/89 jeroen@minixug.nluug.nl
 *		    Enabled linking to /usr/bin/[ so you cn use structures
 *		    like:
 *		 	   if [ -f ./test.c ]
 *		    and so on. Also added checking of argv[0] to see if it
 *		    is '[' and the last argument if it is ']'.
 *
 *		Mod Fred van Kempen 09/27/89 waltje@minixug.nluug.nl
 *		    Adapted source to MINIX Style Sheet.
 *
 *		Mod Bruce Evans 09/27/89 evans@ditsyda.oz.au
 *		    Allow whitespace in numbers.
 *		    Deleted bloat from single use of stdio (fprintf).
 *
 *		Mod Paul Wood 10/10/91 paul@max.uk.mugnet.org
 *		    Added support for -x option.
 *
 *		Add Kees J. Bot 02/19/92 kjb@cs.vu.nl
 *		    Binary operator 'f1 -newer f2'.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifndef S_ISLNK
#define lstat		stat
#define S_ISLNK(mode)	(0)
#endif

#define EOI	0
#define FILRD	1
#define FILWR	2
#define FILIF	3
#define FILID	4
#define FILGZ	5
#define FILTT	6
#define STZER	7
#define STNZE	8
#define STEQL	9
#define STNEQ	10
#define INTEQ	11
#define INTNE	12
#define INTGE	13
#define INTGT	14
#define INTLE	15
#define INTLT	16
#define UNEGN	17
#define BAND	18
#define BOR	19
#define LPAREN	20
#define RPAREN	21
#define OPERAND	22
#define FILXQ	23
#define FILNW	24
#define FILIC	25
#define FILIB	26
#define FILIP	27
#define FILIL	28

#define UNOP	1
#define BINOP	2
#define BUNOP	3
#define BBINOP	4
#define PAREN	5

struct op {
  char *op_text;
  short op_num, op_type;
} ops[] = {
  {	"-r",	FILRD,	UNOP	},
  {	"-w",	FILWR,	UNOP	},
  {	"-f",	FILIF,	UNOP	},
  {	"-d",	FILID,	UNOP	},
  {	"-s",	FILGZ,	UNOP	},
  {	"-t",	FILTT,	UNOP	},
  {	"-z",	STZER,	UNOP	},
  {	"-n",	STNZE,	UNOP	},
  {	"=",	STEQL,	BINOP	},
  {	"!=",	STNEQ,	BINOP	},
  {	"-eq",	INTEQ,	BINOP	},
  {	"-ne",	INTNE,	BINOP	},
  {	"-ge",	INTGE,	BINOP	},
  {	"-gt",	INTGT,	BINOP	},
  {	"-le",	INTLE,	BINOP	},
  {	"-lt",	INTLT,	BINOP	},
  {	"!",	UNEGN,	BUNOP	},
  {	"-a",	BAND,	BBINOP	},
  {	"-o",	BOR,	BBINOP	},
  {	"(",	LPAREN,	PAREN	},
  {	")",	RPAREN,	PAREN	},
  {	"-x",	FILXQ,	UNOP	},
  {	"-newer",FILNW,	BINOP	},
  {	"-c",	FILIC,	UNOP	},
  {	"-b",	FILIB,	UNOP	},
  {	"-p",	FILIP,	UNOP	},
  {	"-h",	FILIL,	UNOP	},
  {	"-L",	FILIL,	UNOP	},
  {	0,	0,	0	}
};

#define _PROTOTYPE(a,b)	

char **ip;
char *prog;
struct op *ip_op;

_PROTOTYPE(int main, (int argc, char **argv));
_PROTOTYPE(void syntax, (void));
_PROTOTYPE(long num, (char *s));
_PROTOTYPE(int filstat, (char *nm, int mode));
_PROTOTYPE(int newer, (char *fil1, char *fil2));
_PROTOTYPE(int lex, (char *s));
_PROTOTYPE(int primary, (int n));
_PROTOTYPE(int bexpr, (int n));
_PROTOTYPE(int expr, (int n));

void syntax()
{
  write(2, prog, strlen(prog));
  write(2, ": syntax error\n", 15);
  exit(1);
}


long num(s)
register char *s;
{
  long l = 0;
  long sign = 1;

  while (*s == ' ' || *s == '\t') ++s;
  if (*s == '\0') syntax();
  if (*s == '-') {
	sign = -1;
	s++;
  }
  while (*s >= '0' && *s <= '9') l = l * 10 + *s++ - '0';
  while (*s == ' ' || *s == '\t') ++s;
  if (*s != '\0') syntax();
  return(sign * l);
}


int filstat(nm, mode)
char *nm;
int mode;
{
  struct stat s;

  switch (mode) {
      case FILRD:
	return(access(nm, 4) == 0);
      case FILWR:
	return(access(nm, 2) == 0);
      case FILIF:
	return(stat(nm, &s) == 0 && S_ISREG(s.st_mode));
      case FILID:
	return(stat(nm, &s) == 0 && S_ISDIR(s.st_mode));
      case FILGZ:
	return(stat(nm, &s) == 0 && (s.st_size > 0L));
      case FILTT:
	return(isatty((int) num(nm)));
      case FILXQ:
	return(access(nm, 1) == 0);
      case FILIC:
	return(stat(nm, &s) == 0 && S_ISCHR(s.st_mode));
      case FILIB:
	return(stat(nm, &s) == 0 && S_ISBLK(s.st_mode));
      case FILIP:
	return(stat(nm, &s) == 0 && S_ISFIFO(s.st_mode));
      case FILIL:
	return(lstat(nm, &s) == 0 && S_ISLNK(s.st_mode));
  }
  return(0);
}


int newer(fil1, fil2)
char *fil1, *fil2;
/* True iff fil1 is not older (!) then fil2. */
{
  struct stat s1, s2;

  if (stat(fil1, &s1) != 0) return 0;
  if (stat(fil2, &s2) != 0) return 1;
  return s1.st_mtime >= s2.st_mtime;
}


int lex(s)
register char *s;
{
  register struct op *op = ops;

  if (s == 0) return EOI;
  while (op->op_text) {
	if (strcmp(s, op->op_text) == 0) {
		ip_op = op;
		return(op->op_num);
	}
	op++;
  }
  ip_op = (struct op *) 0;
  return(OPERAND);
}


int primary(n)
int n;
{
  register char *opnd1, *opnd2;
  int res;

  if (n == EOI) syntax();
  if (n == UNEGN) return (!expr(lex(*++ip)));
  if (n == LPAREN) {
	res = expr(lex(*++ip));
	if (lex(*++ip) != RPAREN) syntax();
	return(res);
  }
  if (n == OPERAND) {
	opnd1 = *ip;
	(void) lex(*++ip);
	if (ip_op && ip_op->op_type == BINOP) {
		struct op *op = ip_op;

		if ((opnd2 = *++ip) == (char *) 0) syntax();

		switch (op->op_num) {
		    case STEQL:
			return(strcmp(opnd1, opnd2) == 0);
		    case STNEQ:
			return(strcmp(opnd1, opnd2) != 0);
		    case INTEQ:
			return(num(opnd1) == num(opnd2));
		    case INTNE:
			return(num(opnd1) != num(opnd2));
		    case INTGE:
			return(num(opnd1) >= num(opnd2));
		    case INTGT:
			return(num(opnd1) > num(opnd2));
		    case INTLE:
			return(num(opnd1) <= num(opnd2));
		    case INTLT:
			return(num(opnd1) < num(opnd2));
		    case FILNW:
			return(newer(opnd1, opnd2));
		}
	}
	ip--;
	return(strlen(opnd1) > 0);
  }

  /* Unary expression */
  if (ip_op->op_type != UNOP || *++ip == 0) syntax();
  if (n == STZER) return (strlen(*ip) == 0);
  if (n == STNZE) return (strlen(*ip) != 0);
  return(filstat(*ip, n));
}


int bexpr(n)
int n;
{
  int res;

  if (n == EOI) syntax();
  res = primary(n);
  if (lex(*++ip) == BAND) return(bexpr(lex(*++ip)) && res);
  ip--;
  return(res);
}


int expr(n)
int n;
{
  int res;

  if (n == EOI) syntax();
  res = bexpr(n);
  if (lex(*++ip) == BOR) return(expr(lex(*++ip)) || res);
  ip--;
  return(res);
}


int main(argc, argv)
int argc;
char **argv;
{
  if (argv[0][0] == '[' && argv[argc - 1][0] != ']') {
	write(2, "test: ] missing\n", 16);
	exit(1);
  }
  if (argv[0][0] == '[') argc--;

  if (argc == 1) exit(1);
  prog = argv[0];
  ip = &argv[1];
  return(!expr(lex(*ip)));
}
