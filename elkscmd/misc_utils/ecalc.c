/*
 * ecalc - simple integer calculator for ELKS
 * 
 * Copyright (c) 2025 Benjamin Helle
 * SPDX-License-Identifier: GPL-2.0-or-later
 * 
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* version 1.2 */
/* notice: int is always signed 16 bit*/

/* define tokens*/
#define TOKEN_NULL  0
#define TOKEN_EOF   1
#define TOKEN_NUM   2
#define TOKEN_LPAR  3
#define TOKEN_RPAR  4
#define TOKEN_ANS   5
/* arithmetic operators */
#define TOKEN_ADD   10
#define TOKEN_SUB   11
#define TOKEN_MUL   12
#define TOKEN_DIV   13
#define TOKEN_EXP   14
/* logical operators */
#define TOKEN_AND   20
#define TOKEN_OR    21
#define TOKEN_XOR   22
#define TOKEN_NOT   23

char* src;
int token;
int num;
int result;

/* built in pow function to reduce size */
int ipow(int x, int y) {
  /* note: this could end up overflowing. to fix: use long */
  if (y < 0) return 0; /* can't do negative*/
  if (y == 0) return 1;
  int r = 1;
  while (y > 0) { /* keep multiplying in a loop while y > 0 */
    r *= x;
    y--;
  }
  return r; /* x^y */
}

void error(char *s) {
  fprintf(stderr, "error: %s\n", s);
  exit(1); /* exit on error */
}

void next_token(void) 
{
  while (*src == ' ') src++; /* skip whitespaces */

  if (*src == '\0') { /* end of input */
    token = TOKEN_EOF;
    return;
  }

  if (isdigit(*src)) { /* number */
    num = atoi(src);
    token = TOKEN_NUM;
    while (isdigit(*src)) src++; /* skip other digits */
    return;
  }

  if (strncmp(src, "ans", 3) == 0) { /* previous result */
    token = TOKEN_ANS;
    src += 3;
    return;
  }

  switch(*src) { /* operators and parentheses */
    case '+': token = TOKEN_ADD; break;
    case '-': token = TOKEN_SUB; break;
    case '*': token = TOKEN_MUL; break;
    case '/': token = TOKEN_DIV; break;
    case '(': token = TOKEN_LPAR; break;
    case ')': token = TOKEN_RPAR; break;
    case 'e': token = TOKEN_EXP; break;
    case '&': token = TOKEN_AND; break;
    case '|': token = TOKEN_OR; break;
    case '^': token = TOKEN_XOR; break;
    case '~': token = TOKEN_NOT; break;
    /* add more tokens here */
    default: token = TOKEN_NULL;
  }

  src++; 
}

/* forward declarations */
int parse_ans(void);
int parse_term(void);
int parse_factor(void);
int parse_expr(void);
int parse_exponent(void);
int parse_and(void);
int parse_or(void);
int parse_xor(void);
int parse_not(void);

/* bitwise operations */

/* this forms a predence tree, from lowest to highest precedence
 * OR, AND, XOR, ADD/SUB, MUL/DIV, EXP, UNARY(eg. NOT)
*/

int parse_and(void) {
  int val = parse_term();
  if (token == TOKEN_AND) {
    next_token();
    val &= parse_term();
  }
  return val;
}

int parse_xor(void) {
  int val = parse_and();
  if (token == TOKEN_XOR) {
    next_token();
    val ^= parse_and();
  }
  return val;
}

int parse_or(void) {
  int val = parse_xor();
  if (token == TOKEN_OR) {
    next_token();
    val |= parse_xor();
  }
  return val;
}


int parse_exponent(void) 
{
  int val = parse_factor();
  while (token == TOKEN_EXP) {
    next_token();
    int rs = parse_factor();
    val = ipow(val, rs);
  }
  return val;
}

int parse_factor(void)
{
  /* higher precedence */
  int val = 0;
  if (token == TOKEN_NUM) {
    val = num;
    next_token();
  } else if (token == TOKEN_LPAR) {
    next_token();
    val = parse_expr();
    if (token == TOKEN_RPAR) next_token();
    else error("expected ')'");
  } else if (token == TOKEN_ADD) {
    next_token();
    val = parse_factor();
  } else if (token == TOKEN_SUB) {
    next_token();
    val = -parse_factor();
  } else if (token == TOKEN_NOT) {
    next_token();
    val = ~parse_factor();
  } else if (token == TOKEN_ANS) {
    next_token();
    val = result;
  }
  else error("expected number or an expression."); /* error if nothing matches */
  return val;
}

int parse_term(void)
{
  int val = parse_exponent(); /* if not exponent, parse_exponent will return parse_factor */
  
  while (token == TOKEN_MUL || token == TOKEN_DIV) {
    int op = token;
    next_token();
    int rs = parse_exponent(); /* right side */
    if (op == TOKEN_MUL) val *= rs;
    else {
      if (rs == 0) error("division by zero"); /* check if zero to avoid dbz error */
      val /= rs;
    }
  }

  return val;
}

int parse_expr(void) 
{
  /* lowest precedence */
  int val = parse_or(); /* will check if logical operation, if not then return parse_term */
  while (token == TOKEN_ADD || token == TOKEN_SUB) {
    int op = token;
    next_token();
    int rs = parse_or();
    if (op == TOKEN_ADD) val += rs;
    else val -= rs;
  }

  return val;
}

int main(int argc, char **argv) 
{
  printf("ecalc 1.2\nSee the man page for help. Ctrl+C to exit\n");
  char line[0xFF];
  while (1) { /* main loop */
    printf("> ");
    if (fgets(line, sizeof(line), stdin) == NULL) break;
    
    src = line;
    next_token();

    result = parse_expr(); /* get result */

    printf("= %d\n", result); /* and print it */
  }

  return 0;
}
