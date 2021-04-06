/*
 * Copyright (c) 2002 - 2005 Magnus Lind.
 *
 * This software is provided 'as-is', without any express or implied warranty.
 * In no event will the authors be held liable for any damages arising from
 * the use of this software.
 *
 * Permission is granted to anyone to use this software, alter it and re-
 * distribute it freely for any non-commercial, non-profit purpose subject to
 * the following restrictions:
 *
 *   1. The origin of this software must not be misrepresented; you must not
 *   claim that you wrote the original software. If you use this software in a
 *   product, an acknowledgment in the product documentation would be
 *   appreciated but is not required.
 *
 *   2. Altered source versions must be plainly marked as such, and must not
 *   be misrepresented as being the original software.
 *
 *   3. This notice may not be removed or altered from any distribution.
 *
 *   4. The names of this software and/or it's copyright holders may not be
 *   used to endorse or promote products derived from this software without
 *   specific prior written permission.
 *
 */

/* scanner for a simple assembler */
%{
#include <stdio.h>
#include <string.h>
#include "int.h"
#include "buf.h"
#include "parse.h"
#include "asm.tab.h"

#define MAX_SRC_BUFFER_DEPTH 10
static YY_BUFFER_STATE src_buffers[MAX_SRC_BUFFER_DEPTH];
static int src_buffer_depth = 0;

static char *strdupped_get(char *text);
static int strdupped_cmp(const void *a, const void *b);
static void strdupped_free(void *a);

int num_lines = 1;
int push_state_skip = 0;
int push_state_init = 0;
int push_state_macro = 0;
struct vec strdupped[1];

%}

%x SKIP SKIP_ALL QUOTED_STRING SKIP_LINE MACROO

%option noyywrap
%option case-insensitive
%option stack

%%

	if(push_state_init)
	{push_state_init = 0; yy_push_state(INITIAL); }
	if(push_state_skip)
	{push_state_skip = 0; yy_push_state(SKIP); }
	if(push_state_macro)
	{push_state_macro = 0; yy_push_state(MACROO); }

\.if		return IF;
\.elif		BEGIN(SKIP_ALL);
\.else		BEGIN(SKIP);
\.endif		yy_pop_state();

\.include	return INCLUDE;
\.macro		return MACRO;

\.defined	return DEFINED;
\.org		return ORG;
\.error		return ERROR;
\.echo		return ECHO1;
\.incbin	return INCBIN;
\.inclen	return INCLEN;
\.incword	return INCWORD;
\.res		return RES;
\.word		return WORD;
\.byte		return BYTE;

\"		BEGIN(QUOTED_STRING);
\;		BEGIN(SKIP_LINE);

lda		return LDA;
ldx		return LDX;
ldy		return LDY;
sta		return STA;
stx		return STX;
sty		return STY;
and		return AND;
ora		return ORA;
eor		return EOR;
adc		return ADC;
sbc		return SBC;
cmp		return CMP;
cpx		return CPX;
cpy		return CPY;

tsx		return TSX;
txs		return TXS;
pha		return PHA;
pla		return PLA;
php		return PHP;
plp		return PLP;
sei		return SEI;
cli		return CLI;
nop		return NOP;
tya		return TYA;
tay		return TAY;
txa		return TXA;
tax		return TAX;
clc		return CLC;
sec		return SEC;
rts		return RTS;
clv		return CLV;
cld		return CLD;
sed		return SED;

jsr		return JSR;
jmp		return JMP;
beq		return BEQ;
bne		return BNE;
bcc		return BCC;
bcs		return BCS;
bpl		return BPL;
bmi		return BMI;
bvc		return BVC;
bvs		return BVS;
inx		return INX;
dex		return DEX;
iny		return INY;
dey		return DEY;
inc		return INC;
dec		return DEC;
lsr		return LSR;
asl		return ASL;
ror		return ROR;
rol		return ROL;
bit		return BIT;

[0-9]+		{ yylval.num = atoi(yytext); return NUMBER; }

$[0-9a-f]+	{ yylval.num = strtol(yytext + 1, NULL, 16); return NUMBER; }

\<		return LT;
\>		return GT;
==		return EQ;
!=		return NEQ;
!		return LNOT;
\&\&		return LAND;
\|\|		return LOR;

\(		return LPAREN;
\)		return RPAREN;
\,		return COMMA;
\:		return COLON;
\#		return HASH;
\+		return PLUS;
\-		return MINUS;
\*		return MULT;
\/		return DIV;
\%		return MOD;

\=		return ASSIGN;
\?=		return GUESS;

x		return X;
y		return Y;

[a-z][_a-z0-9]*/:	{ yylval.str = strdupped_get(yytext); return LABEL; }
[a-z][_a-z0-9]*	{ yylval.str = strdupped_get(yytext); return SYMBOL; }

\r\n|\n		++num_lines;

[ \t]		/* eat whitespace */

.		printf("unknown character found %s\n", yytext);

<SKIP>\.if		yy_push_state(SKIP_ALL);
<SKIP>\.elif		{ yy_pop_state(); return IF; }
<SKIP>\.else		BEGIN(INITIAL);
<SKIP>\.endif		yy_pop_state();
<SKIP>\r\n|\n		++num_lines;
<SKIP>.

<MACROO>\.endmacro	yy_pop_state();
<MACROO>\.+		{ yylval.str = yytext; return MACRO_STRING; }
<MACROO>[^\.]+		{ yylval.str = yytext; return MACRO_STRING; }

<SKIP_ALL>\.if		yy_push_state(SKIP_ALL);
<SKIP_ALL>\.endif	yy_pop_state();
<SKIP_ALL>\r\n|\n	++num_lines;
<SKIP_ALL>.

<QUOTED_STRING>[^\"]*	{
    /* multi-line string with correct line count */
    char *p = strdupped_get(yytext);
    yylval.str = p;
    while (*p != '\0')
    {
        if (*p++ == '\n')
        {
            ++num_lines;
        }
    }
    return STRING;
}
<QUOTED_STRING>\"	BEGIN(INITIAL);

<SKIP_LINE>\r\n|\n	{ ++num_lines; BEGIN(INITIAL); }
<SKIP_LINE>.

<<EOF>>	{
	    if(--src_buffer_depth == 0)
	    {
		yyterminate();
	    }
	    else
	    {
		yy_delete_buffer(YY_CURRENT_BUFFER);
		yy_switch_to_buffer(src_buffers[src_buffer_depth]);
	    }
	}

%%

void scanner_init(void)
{
    vec_init(strdupped, sizeof(char*));
}

void asm_src_buffer_push(struct buf *buffer)
{
    if(src_buffer_depth == MAX_SRC_BUFFER_DEPTH)
    {
	fprintf(stderr, "source buffers nested too deep\n");
	exit(1);
    }
    src_buffers[src_buffer_depth++] = YY_CURRENT_BUFFER;
    yy_scan_bytes(buf_data(buffer), buf_size(buffer));
}

static char *strdupped_get(char *text)
{
    char **pp;
    /*printf("get \"%s\" => ", text);*/
    if(vec_insert_uniq(strdupped, strdupped_cmp, &text, (void*)&pp))
    {
        /* replace the pointer to <text> since <text> will be reused */
        *pp = strdup(text);
    }
    /*printf("%p\n", *pp);*/
    return *pp;
}

static void strdupped_free(void *a)
{
    char *b = *(char**)a;
    /*printf("free => %p \"%s\"\n", b, b);*/
    free(b);
}

static int strdupped_cmp(const void *a, const void *b)
{
    char *c = *(char**)a;
    char *d = *(char**)b;

    return strcmp(c, d);
}

void scanner_free(void)
{
    vec_free(strdupped, strdupped_free);
}


void silence_warnings_about_unused_functions(void)
{
    yyunput(0, NULL);
    input();
    yy_top_state();
}
