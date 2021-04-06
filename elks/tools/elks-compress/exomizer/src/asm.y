/*
 * Copyright (c) 2005 Magnus Lind.
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

%{
#include "int.h"
#include "parse.h"
#include "vec.h"
#include "buf.h"
#include "log.h"
#include <stdio.h>
#define YYERROR_VERBOSE

static struct vec asm_atoms[1];

/* prototypes to silence compiler warnings */
int yylex(void);
void yyerror(const char *s);

%}

%expect 5

%token INCLUDE
%token IF
%token DEFINED
%token MACRO
%token <str> MACRO_STRING
%token ORG
%token ERROR
%token ECHO1
%token INCBIN
%token INCLEN
%token INCWORD
%token RES
%token WORD
%token BYTE

%token LDA
%token LDX
%token LDY
%token STA
%token STX
%token STY
%token AND
%token ORA
%token EOR
%token ADC
%token SBC
%token CMP
%token CPX
%token CPY

%token TSX
%token TXS
%token PHA
%token PLA
%token PHP
%token PLP
%token SEI
%token CLI
%token NOP
%token TYA
%token TAY
%token TXA
%token TAX
%token CLC
%token SEC
%token RTS
%token CLV
%token CLD
%token SED

%token JSR
%token JMP
%token BEQ
%token BNE
%token BCC
%token BCS
%token BPL
%token BMI
%token BVC
%token BVS
%token INX
%token DEX
%token INY
%token DEY
%token INC
%token DEC

%token LSR
%token ASL
%token ROR
%token ROL
%token BIT

%token <str> SYMBOL
%token <str> STRING

%token LAND
%token LOR
%token LNOT

%token LPAREN
%token RPAREN
%token COMMA
%token COLON
%token X
%token Y
%token HASH
%token PLUS
%token MINUS
%token MULT
%token DIV
%token MOD
%token LT
%token GT
%token EQ
%token NEQ
%token ASSIGN
%token GUESS
%token <num> NUMBER

%union
{
    i32 num;
    char *str;
    struct atom *atom;
    struct expr *expr;
}

%right ASSIGN
%right GUESS
%left LOR
%left LAND
%nonassoc LT GT EQ
%left MINUS PLUS
%left MULT DIV MOD
%left vNEG
%left LNOT

%token <str> LABEL

%type <expr> lexpr
%type <expr> expr
%type <atom> op;
%type <atom> atom;
%type <atom> exprs;

%type <expr> am_im;
%type <expr> am_a;
%type <expr> am_ax;
%type <expr> am_ay;
%type <expr> am_zp;
%type <expr> am_zpx;
%type <expr> am_zpy;
%type <expr> am_ix;
%type <expr> am_iy;

%%

stmts:	stmts stmt | stmt;
stmt:	LABEL COLON { new_label($1); } |
	SYMBOL ASSIGN expr { new_symbol_expr($1, $3); } |
	SYMBOL GUESS expr { new_symbol_expr_guess($1, $3); } |
        IF LPAREN lexpr RPAREN { push_if_state($3); } |
        ORG LPAREN expr RPAREN { set_org($3); } |
        ERROR LPAREN STRING RPAREN { asm_error($3); } |
        ECHO1 LPAREN STRING RPAREN { asm_echo($3, NULL); } |
        ECHO1 LPAREN STRING COMMA exprs RPAREN { asm_echo($3, $5); } |
        INCLUDE LPAREN STRING RPAREN { asm_include($3); } |
        MACRO LPAREN STRING RPAREN { push_macro_state($3); } |
        atom { vec_push(asm_atoms, &$1); } |
        MACRO_STRING { macro_append($1); };

atom:	op { $$ = $1; } |
        RES LPAREN expr COMMA expr RPAREN { $$ = new_res($3, $5); } |
        WORD LPAREN exprs RPAREN { $$ = exprs_to_word_exprs($3); } |
        BYTE LPAREN exprs RPAREN { $$ = exprs_to_byte_exprs($3); } |
	INCBIN LPAREN STRING RPAREN {
            $$ = new_incbin($3, NULL, NULL); } |
        INCBIN LPAREN STRING COMMA expr RPAREN {
            $$ = new_incbin($3, $5, NULL); } |
        INCBIN LPAREN STRING COMMA expr COMMA expr RPAREN {
            $$ = new_incbin($3, $5, $7); };

exprs:	exprs COMMA expr { $$ = exprs_add($1, $3); } |
	expr { $$ = new_exprs($1); };

op:	LDA am_im  { $$ = new_op(0xA9, ATOM_TYPE_OP_ARG_UI8, $2); } |
	LDA am_zp  { $$ = new_op(0xA5, ATOM_TYPE_OP_ARG_U8,  $2); } |
	LDA am_zpx { $$ = new_op(0xB5, ATOM_TYPE_OP_ARG_U8, $2); } |
	LDA am_a   { $$ = new_op(0xAD, ATOM_TYPE_OP_ARG_U16, $2); } |
	LDA am_ax  { $$ = new_op(0xBD, ATOM_TYPE_OP_ARG_U16, $2); } |
	LDA am_ay  { $$ = new_op(0xB9, ATOM_TYPE_OP_ARG_U16, $2); } |
	LDA am_ix  { $$ = new_op(0xA1, ATOM_TYPE_OP_ARG_U8, $2); } |
	LDA am_iy  { $$ = new_op(0xB1, ATOM_TYPE_OP_ARG_U8, $2); };

op:	LDX am_im  { $$ = new_op(0xA2, ATOM_TYPE_OP_ARG_UI8, $2); } |
	LDX am_zp  { $$ = new_op(0xA6, ATOM_TYPE_OP_ARG_U8,  $2); } |
	LDX am_zpy { $$ = new_op(0xB6, ATOM_TYPE_OP_ARG_U8,  $2); } |
	LDX am_a   { $$ = new_op(0xAE, ATOM_TYPE_OP_ARG_U16, $2); } |
	LDX am_ay  { $$ = new_op(0xBE, ATOM_TYPE_OP_ARG_U16, $2); };

op:	LDY am_im  { $$ = new_op(0xA0, ATOM_TYPE_OP_ARG_UI8, $2); } |
	LDY am_zp  { $$ = new_op(0xA4, ATOM_TYPE_OP_ARG_U8,  $2); } |
	LDY am_zpx { $$ = new_op(0xB4, ATOM_TYPE_OP_ARG_U8,  $2); } |
	LDY am_a   { $$ = new_op(0xAC, ATOM_TYPE_OP_ARG_U16, $2); } |
	LDY am_ax  { $$ = new_op(0xBC, ATOM_TYPE_OP_ARG_U16, $2); };

op:	STA am_zp  { $$ = new_op(0x85, ATOM_TYPE_OP_ARG_U8,  $2); } |
	STA am_zpx { $$ = new_op(0x95, ATOM_TYPE_OP_ARG_U8,  $2); } |
	STA am_a   { $$ = new_op(0x8D, ATOM_TYPE_OP_ARG_U16, $2); } |
	STA am_ax  { $$ = new_op(0x9D, ATOM_TYPE_OP_ARG_U16, $2); } |
	STA am_ay  { $$ = new_op(0x99, ATOM_TYPE_OP_ARG_U16, $2); } |
	STA am_ix  { $$ = new_op(0x81, ATOM_TYPE_OP_ARG_U8,  $2); } |
	STA am_iy  { $$ = new_op(0x91, ATOM_TYPE_OP_ARG_U8,  $2); };

op:	STX am_zp  { $$ = new_op(0x86, ATOM_TYPE_OP_ARG_U8,  $2); } |
	STX am_zpy { $$ = new_op(0x96, ATOM_TYPE_OP_ARG_U8,  $2); } |
	STX am_a   { $$ = new_op(0x8e, ATOM_TYPE_OP_ARG_U16, $2); };

op:	STY am_zp  { $$ = new_op(0x84, ATOM_TYPE_OP_ARG_U8,  $2); } |
	STY am_zpx { $$ = new_op(0x94, ATOM_TYPE_OP_ARG_U8,  $2); } |
	STY am_a   { $$ = new_op(0x8c, ATOM_TYPE_OP_ARG_U16, $2); };

op:	AND am_im  { $$ = new_op(0x29, ATOM_TYPE_OP_ARG_UI8, $2); } |
	AND am_zp  { $$ = new_op(0x25, ATOM_TYPE_OP_ARG_U8,  $2); } |
	AND am_zpx { $$ = new_op(0x35, ATOM_TYPE_OP_ARG_U8, $2); } |
	AND am_a   { $$ = new_op(0x2d, ATOM_TYPE_OP_ARG_U16, $2); } |
	AND am_ax  { $$ = new_op(0x3d, ATOM_TYPE_OP_ARG_U16, $2); } |
	AND am_ay  { $$ = new_op(0x39, ATOM_TYPE_OP_ARG_U16, $2); } |
	AND am_ix  { $$ = new_op(0x21, ATOM_TYPE_OP_ARG_U8, $2); } |
	AND am_iy  { $$ = new_op(0x31, ATOM_TYPE_OP_ARG_U8, $2); };

op:	ORA am_im  { $$ = new_op(0x09, ATOM_TYPE_OP_ARG_UI8, $2); } |
	ORA am_zp  { $$ = new_op(0x05, ATOM_TYPE_OP_ARG_U8,  $2); } |
	ORA am_zpx { $$ = new_op(0x15, ATOM_TYPE_OP_ARG_U8, $2); } |
	ORA am_a   { $$ = new_op(0x0d, ATOM_TYPE_OP_ARG_U16, $2); } |
	ORA am_ax  { $$ = new_op(0x1d, ATOM_TYPE_OP_ARG_U16, $2); } |
	ORA am_ay  { $$ = new_op(0x19, ATOM_TYPE_OP_ARG_U16, $2); } |
	ORA am_ix  { $$ = new_op(0x01, ATOM_TYPE_OP_ARG_U8, $2); } |
	ORA am_iy  { $$ = new_op(0x11, ATOM_TYPE_OP_ARG_U8, $2); };

op:	EOR am_im  { $$ = new_op(0x49, ATOM_TYPE_OP_ARG_UI8, $2); } |
	EOR am_zp  { $$ = new_op(0x45, ATOM_TYPE_OP_ARG_U8,  $2); } |
	EOR am_zpx { $$ = new_op(0x55, ATOM_TYPE_OP_ARG_U8, $2); } |
	EOR am_a   { $$ = new_op(0x4d, ATOM_TYPE_OP_ARG_U16, $2); } |
	EOR am_ax  { $$ = new_op(0x5d, ATOM_TYPE_OP_ARG_U16, $2); } |
	EOR am_ay  { $$ = new_op(0x59, ATOM_TYPE_OP_ARG_U16, $2); } |
	EOR am_ix  { $$ = new_op(0x41, ATOM_TYPE_OP_ARG_U8, $2); } |
	EOR am_iy  { $$ = new_op(0x51, ATOM_TYPE_OP_ARG_U8, $2); };

op:	ADC am_im  { $$ = new_op(0x69, ATOM_TYPE_OP_ARG_UI8, $2); } |
	ADC am_zp  { $$ = new_op(0x65, ATOM_TYPE_OP_ARG_U8,  $2); } |
	ADC am_zpx { $$ = new_op(0x75, ATOM_TYPE_OP_ARG_U8, $2); } |
	ADC am_a   { $$ = new_op(0x6D, ATOM_TYPE_OP_ARG_U16, $2); } |
	ADC am_ax  { $$ = new_op(0x7D, ATOM_TYPE_OP_ARG_U16, $2); } |
	ADC am_ay  { $$ = new_op(0x79, ATOM_TYPE_OP_ARG_U16, $2); } |
	ADC am_ix  { $$ = new_op(0x61, ATOM_TYPE_OP_ARG_U8, $2); } |
	ADC am_iy  { $$ = new_op(0x71, ATOM_TYPE_OP_ARG_U8, $2); };

op:	SBC am_im  { $$ = new_op(0xe9, ATOM_TYPE_OP_ARG_UI8, $2); } |
	SBC am_zp  { $$ = new_op(0xe5, ATOM_TYPE_OP_ARG_U8,  $2); } |
	SBC am_zpx { $$ = new_op(0xf5, ATOM_TYPE_OP_ARG_U8, $2); } |
	SBC am_a   { $$ = new_op(0xeD, ATOM_TYPE_OP_ARG_U16, $2); } |
	SBC am_ax  { $$ = new_op(0xfD, ATOM_TYPE_OP_ARG_U16, $2); } |
	SBC am_ay  { $$ = new_op(0xf9, ATOM_TYPE_OP_ARG_U16, $2); } |
	SBC am_ix  { $$ = new_op(0xe1, ATOM_TYPE_OP_ARG_U8, $2); } |
	SBC am_iy  { $$ = new_op(0xf1, ATOM_TYPE_OP_ARG_U8, $2); };

op:	CMP am_im  { $$ = new_op(0xc9, ATOM_TYPE_OP_ARG_UI8, $2); } |
	CMP am_zp  { $$ = new_op(0xc5, ATOM_TYPE_OP_ARG_U8,  $2); } |
	CMP am_zpx { $$ = new_op(0xd5, ATOM_TYPE_OP_ARG_U8, $2); } |
	CMP am_a   { $$ = new_op(0xcD, ATOM_TYPE_OP_ARG_U16, $2); } |
	CMP am_ax  { $$ = new_op(0xdD, ATOM_TYPE_OP_ARG_U16, $2); } |
	CMP am_ay  { $$ = new_op(0xd9, ATOM_TYPE_OP_ARG_U16, $2); } |
	CMP am_ix  { $$ = new_op(0xc1, ATOM_TYPE_OP_ARG_U8, $2); } |
	CMP am_iy  { $$ = new_op(0xd1, ATOM_TYPE_OP_ARG_U8, $2); };

op:	CPX am_im { $$ = new_op(0xe0, ATOM_TYPE_OP_ARG_U8, $2); } |
	CPX am_zp { $$ = new_op(0xe4, ATOM_TYPE_OP_ARG_U8, $2); } |
	CPX am_a  { $$ = new_op(0xec, ATOM_TYPE_OP_ARG_U16, $2); } |
	CPY am_im { $$ = new_op(0xc0, ATOM_TYPE_OP_ARG_U8, $2); } |
	CPY am_zp { $$ = new_op(0xc4, ATOM_TYPE_OP_ARG_U8, $2); } |
	CPY am_a  { $$ = new_op(0xcc, ATOM_TYPE_OP_ARG_U16, $2); };

op:	TXS { $$ = new_op0(0x9A); } |
	TSX { $$ = new_op0(0xBA); } |
	PHA { $$ = new_op0(0x48); } |
	PLA { $$ = new_op0(0x68); } |
	PHP { $$ = new_op0(0x08); } |
	PLP { $$ = new_op0(0x28); } |
	SEI { $$ = new_op0(0x78); } |
	CLI { $$ = new_op0(0x58); } |
	NOP { $$ = new_op0(0xea); } |
	TYA { $$ = new_op0(0x98); } |
	TAY { $$ = new_op0(0xa8); } |
	TXA { $$ = new_op0(0x8a); } |
	TAX { $$ = new_op0(0xaa); } |
	CLC { $$ = new_op0(0x18); } |
	SEC { $$ = new_op0(0x38); } |
	RTS { $$ = new_op0(0x60); } |
	CLV { $$ = new_op0(0xb8); } |
	CLD { $$ = new_op0(0xd8); } |
	SED { $$ = new_op0(0xf0); };

op:	JSR am_a   { $$ = new_op(0x20, ATOM_TYPE_OP_ARG_U16, $2); } |
	JMP am_a   { $$ = new_op(0x4c, ATOM_TYPE_OP_ARG_U16, $2); } |
	BEQ am_a   { $$ = new_op(0xf0, ATOM_TYPE_OP_ARG_I8,  $2); } |
	BNE am_a   { $$ = new_op(0xd0, ATOM_TYPE_OP_ARG_I8,  $2); } |
	BCC am_a   { $$ = new_op(0x90, ATOM_TYPE_OP_ARG_I8,  $2); } |
	BCS am_a   { $$ = new_op(0xb0, ATOM_TYPE_OP_ARG_I8,  $2); } |
	BPL am_a   { $$ = new_op(0x10, ATOM_TYPE_OP_ARG_I8,  $2); } |
	BMI am_a   { $$ = new_op(0x30, ATOM_TYPE_OP_ARG_I8,  $2); } |
	BVC am_a   { $$ = new_op(0x50, ATOM_TYPE_OP_ARG_I8,  $2); } |
	BVS am_a   { $$ = new_op(0x70, ATOM_TYPE_OP_ARG_I8,  $2); };

op:	INX { $$ = new_op0(0xe8); } |
	DEX { $$ = new_op0(0xca); } |
	INY { $$ = new_op0(0xc8); } |
	DEY { $$ = new_op0(0x88); };

op:	INC am_zp  { $$ = new_op(0xe6, ATOM_TYPE_OP_ARG_U8, $2); } |
	INC am_zpx { $$ = new_op(0xf6, ATOM_TYPE_OP_ARG_U8, $2); } |
	INC am_a   { $$ = new_op(0xee, ATOM_TYPE_OP_ARG_U16, $2); } |
        INC am_ax  { $$ = new_op(0xfe, ATOM_TYPE_OP_ARG_U16, $2); };

op:	DEC am_zp  { $$ = new_op(0xc6, ATOM_TYPE_OP_ARG_U8, $2); } |
	DEC am_zpx { $$ = new_op(0xd6, ATOM_TYPE_OP_ARG_U8, $2); } |
	DEC am_a   { $$ = new_op(0xce, ATOM_TYPE_OP_ARG_U16, $2); } |
        DEC am_ax  { $$ = new_op(0xde, ATOM_TYPE_OP_ARG_U16, $2); };

op:	LSR        { $$ = new_op0(0x4a); } |
	LSR am_zp  { $$ = new_op(0x46, ATOM_TYPE_OP_ARG_U8, $2); } |
	LSR am_zpx { $$ = new_op(0x56, ATOM_TYPE_OP_ARG_U8, $2); } |
	LSR am_a   { $$ = new_op(0x4e, ATOM_TYPE_OP_ARG_U16, $2); } |
	LSR am_ax  { $$ = new_op(0x5e, ATOM_TYPE_OP_ARG_U16, $2); };

op:	ASL        { $$ = new_op0(0x0a); } |
	ASL am_zp  { $$ = new_op(0x06, ATOM_TYPE_OP_ARG_U8, $2); } |
	ASL am_zpx { $$ = new_op(0x16, ATOM_TYPE_OP_ARG_U8, $2); } |
	ASL am_a   { $$ = new_op(0x0e, ATOM_TYPE_OP_ARG_U16, $2); } |
	ASL am_ax  { $$ = new_op(0x1e, ATOM_TYPE_OP_ARG_U16, $2); };

op:	ROR        { $$ = new_op0(0x6a); } |
	ROR am_zp  { $$ = new_op(0x66, ATOM_TYPE_OP_ARG_U8, $2); } |
	ROR am_zpx { $$ = new_op(0x76, ATOM_TYPE_OP_ARG_U8, $2); } |
	ROR am_a   { $$ = new_op(0x6e, ATOM_TYPE_OP_ARG_U16, $2); } |
	ROR am_ax  { $$ = new_op(0x7e, ATOM_TYPE_OP_ARG_U16, $2); };

op:	ROL        { $$ = new_op0(0x2a); } |
	ROL am_zp  { $$ = new_op(0x26, ATOM_TYPE_OP_ARG_U8, $2); } |
	ROL am_zpx { $$ = new_op(0x36, ATOM_TYPE_OP_ARG_U8, $2); } |
	ROL am_a   { $$ = new_op(0x2e, ATOM_TYPE_OP_ARG_U16, $2); } |
	ROL am_ax  { $$ = new_op(0x3e, ATOM_TYPE_OP_ARG_U16, $2); };

op:	BIT am_zp  { $$ = new_op(0x24, ATOM_TYPE_OP_ARG_U8, $2); } |
	BIT am_a   { $$ = new_op(0x2c, ATOM_TYPE_OP_ARG_U16, $2); };

am_im:	HASH expr { $$ = $2; };
am_a:	expr { $$ = $1; };
am_ax:	expr COMMA X { $$ = $1; };
am_ay:	expr COMMA Y { $$ = $1; };
am_zp:	LT expr { $$ = $2; };
am_zpx:	LT expr COMMA X { $$ = $2; };
am_zpy:	LT expr COMMA Y { $$ = $2; };
am_ix:	LPAREN expr COMMA X RPAREN { $$ = $2; };
am_iy:	LPAREN expr RPAREN COMMA Y { $$ = $2; };

expr:	expr PLUS expr        { $$ = new_expr_op2(PLUS, $1, $3); } |
	expr MINUS expr       { $$ = new_expr_op2(MINUS, $1, $3); } |
	expr MULT expr        { $$ = new_expr_op2(MULT, $1, $3); } |
	expr DIV expr         { $$ = new_expr_op2(DIV, $1, $3); } |
	expr MOD expr         { $$ = new_expr_op2(MOD, $1, $3); } |
	MINUS expr %prec vNEG { $$ = new_expr_op1(vNEG, $2); } |
	LPAREN expr RPAREN { $$ = $2; } |
        INCLEN LPAREN STRING RPAREN { $$ = new_expr_inclen($3); } |
        INCWORD LPAREN STRING COMMA expr RPAREN {
            $$ = new_expr_incword($3, $5); } |
	NUMBER { $$ = new_expr_number($1); } |
	SYMBOL { $$ = new_expr_symref($1); };

lexpr:	lexpr LOR lexpr     { $$ = new_expr_op2(LOR, $1, $3); } |
	lexpr LAND lexpr    { $$ = new_expr_op2(LAND, $1, $3); } |
        LNOT lexpr          { $$ = new_expr_op1(LNOT, $2); } |
	LPAREN lexpr RPAREN { $$ = $2; } |
	expr LT expr        { $$ = new_expr_op2(LT, $1, $3); } |
	expr GT expr        { $$ = new_expr_op2(GT, $1, $3); } |
	expr EQ expr        { $$ = new_expr_op2(EQ, $1, $3); } |
	expr NEQ expr       { $$ = new_expr_op2(NEQ, $1, $3); };

lexpr:  DEFINED LPAREN SYMBOL RPAREN        { $$ = new_is_defined($3); };

%%

void yyerror (const char *s)
{
    fprintf (stderr, "line %d, %s\n", num_lines, s);
}

void asm_set_source(struct buf *buffer);

int assembleSinglePass(struct buf *source, struct buf *dest)
{
    int val;

    yydebug = 0;
    asm_src_buffer_push(source);
    vec_init(asm_atoms, sizeof(struct atom*));
    val = yyparse();
    if(val == 0)
    {
        output_atoms(dest, asm_atoms);
    }
    vec_free(asm_atoms, NULL);
    return val;
}
