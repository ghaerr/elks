/*
 * ecalc - simple integer calculator for ELKS
 * 
 * Copyright (c) 2025 Benjamin Helle
 * SPDX-License-Identifier: GPL-2.0-or-later
 * 
*/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

/* version 1.0 */

/* define tokens*/
#define TOKEN_NULL  0
#define TOKEN_EOF   1
#define TOKEN_NUM   2
#define TOKEN_LPAR  3
#define TOKEN_RPAR  4
#define TOKEN_ADD   5
#define TOKEN_SUB   6
#define TOKEN_MUL   7
#define TOKEN_DIV   8

char* src;
int token;
int num;

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

  switch(*src) { /* operators and parentheses */
    case '+': token = TOKEN_ADD; break;
    case '-': token = TOKEN_SUB; break;
    case '*': token = TOKEN_MUL; break;
    case '/': token = TOKEN_DIV; break;
    case '(': token = TOKEN_LPAR; break;
    case ')': token = TOKEN_RPAR; break;
    /* add more tokens here */
    default: token = TOKEN_NULL;
  }

  src++; 
}

/* forward declarations */
int parse_term(void);
int parse_factor(void);
int parse_expr(void);

int parse_factor(void)
{
  int val = 0;
  if (token == TOKEN_NUM) {
    val = num;
    next_token();
  } else if (token == TOKEN_LPAR) {
    next_token();
    val = parse_expr();
    if (token == TOKEN_RPAR) next_token();
    else error("expected ')'\n");
  } else if (token == TOKEN_ADD) {
    next_token();
    val = parse_factor();
  } else if (token == TOKEN_SUB) {
    next_token();
    val = -parse_factor();
  } else error("expected number or '(', ')', '+', '-', '*' or '/'\n"); /* error if nothing matches */
  return val;
}

int parse_term(void)
{
  int val = parse_factor();
  
  while (token == TOKEN_MUL || token == TOKEN_DIV) {
    int op = token;
    next_token();
    int rs = parse_factor(); /* right side */
    if (op == TOKEN_MUL) val *= rs;
    else {
      if (rs == 0 || val == 0) error("division by zero"); /* check if zero */
      val /= rs;
    }
  }

  return val;
}

int parse_expr(void) 
{
  int val = parse_term();
  while (token == TOKEN_ADD || token == TOKEN_SUB) {
    int op = token;
    next_token();
    int rs = parse_term();
    if (op == TOKEN_ADD) val += rs;
    else val -= rs;
  }

  return val;
}

int main(int argc, char **argv) 
{
  printf("ecalc 1.0 - simple integer calculator for ELKS\nAvailable operators: +, -, *, /\n");
  char line[0xFF];
  while (1) { /* main loop */
    printf("> ");
    if (fgets(line, sizeof(line), stdin) == NULL) break;
    
    src = line;
    next_token();

    int result = parse_expr(); /* get result */

    printf("= %d\n", result); /* and print it */
  }

  return 0;
}
