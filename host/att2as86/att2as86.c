/*
Copyright (C) 2014 Juan Perez

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

/* gnu as to as86 source code converter */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_LINE	4096
#define MAX_OPERANDS	5

static char buffer[MAX_LINE];
static char *operands[MAX_OPERANDS];
static int sects[] = {'T', 'D', 'B', 'R',};
static char *sectmsg[] = {".text", ".data", ".bss", ".rom", ".undef",};


typedef struct {
	char	*att;
	char	*as86;
	int	type;
} instruction;
static int opcode = 0;
static int lineno = 1;
static char *imm_one = "#1";

static instruction mnemonics[] = {
	{".align",  ".align", 12},
	{".arch",   ".", 0},
	{".ascii",  ".ascii", 12},
	{".asciz",  ".asciz", 12},
	{".att_syntax",  ".", 0},
	{".byte",   ".byte", 12},
	{".bss",    ".bss", 12},
	{".code16", ".", 0},
	{".comm",   ".comm", 9},
	{".data",   ".data", 12},
	{".extern", ".globl", 12},
	{".global", "export", 12},
	{".hword",  ".word", 12},
	{".ident",  ".", 0},
	{".local",   ".", 0,},
	{".long",   ".long", 12,},
	{".p2align",".align", 7},
	{".section",".sect", 6},
	{".single", ".single", 12},
	{".size",   ".", 0},
	{".skip",   ".space", 8},
	{".string", ".asciz", 12},
	{".text",   ".text", 1},
	{".type",   ".", 0},
	{".word",   ".word", 1},
	{"aaa",     "aaa", 1},
	{"aad",     "aad", 1},
	{"aam",     "aam", 1},
	{"aas",     "aas", 1},
	{"adcb",    "adcb", 2},
	{"adcw",    "adc", 2},
	{"addb",    "addb", 2},
	{"addw",    "add", 2},
	{"andb",    "andb", 2},
	{"andw",    "and", 2},
	{"call",    "call", 5},
	{"cbtw",    "cbw", 1},
	{"clc",     "clc", 1},
	{"cld",     "cld", 1},
	{"cli",     "cli", 1},
	{"cmc",     "cmc", 1},
	{"cmpb",    "cmpb", 2},
	{"cmpsb",   "cmpsb", 1},
	{"cmpsw",   "cmpsw", 1},
	{"cmpw",    "cmp", 2},
	{"cwtd",    "cwd", 1},
	{"daa",     "daa", 1},
	{"das",     "das", 1},
	{"decb",    "decb", 1},
	{"decw",    "dec", 1},
	{"divb",    "divb", 1},
	{"divw",    "div", 1},
	{"fadd",    "fadd", 1},
	{"faddp",   "faddp", 1},
	{"faddps",  "faddp", 1},
	{"fadds",   "fadd", 1},
	{"fcom",    "fcom", 1},
	{"fcomp",   "fcomp", 1},
	{"fcomps",  "fcomp", 1},
	{"fcoms",   "fcom", 1},
	{"fdiv",    "fdiv", 1},
	{"fdivp",   "fdivp", 1},
	{"fdivr",   "fdivr", 1},
	{"fdivrp",  "fdivrp", 1},
	{"fdivrs",  "fdivrs", 1},
	{"fildl",   "fild", 1},
	{"fistl",   "fist", 1},
	{"fistpl",  "fistp", 1},
	{"fld",     "fld", 1},
	{"fldcw",   "fldcw", 1},
	{"fldenv",  "fldenv", 1},
	{"flds",    "fld", 1},
	{"fmul",    "fmul", 1},
	{"fmulp",   "fmulp", 1},
	{"fmulps",  "fmulp", 1},
	{"fmuls",   "fmul", 1},
	{"fnstcw",  "fnstcw", 1},
	{"fnstenv", "fnstenv", 1},
	{"fnstsw",  "fnstsw",  1},
	{"fstp",    "fstp",    1},
	{"fstps",   "fstp", 1},
	{"fsts",    "fst", 1},
	{"fsub",    "fsub", 1},
	{"fsubp",   "fsubp", 1},
	{"fsubps",  "fsubps", 1},
	{"fsubr",   "fsubr", 1},
	{"fsubrp",  "fsubrp", 1},
	{"fsubrs",  "fsubrs", 1},
	{"fsubs",   "fsubs", 1},
	{"fxch",    "fxch", 1},
	{"hlt",     "hlt", 1},
	{"idivb",   "idivb", 1},
	{"idivw",   "idiv", 1},
	{"imulb",   "imulb", 3},
	{"imulw",   "imul", 3},
	{"inb",     "inb", 2},
	{"incb",    "incb", 1},
	{"incw",    "inc", 1},
	{"int",     "int", 1},
	{"into",    "into", 1},
	{"inw",     "in", 2},
	{"iret",    "iret", 1},
	{"ja",      "bhi", 1},	/**/
	{"jae",     "bhis", 1},	/**/
	{"jb",      "blo", 1},	/**/
	{"jbe",     "blos", 1},	/**/
	{"jc",      "bcs", 1},	/**/
	{"jcxz",    "jcxz", 1},
	{"je",      "beq", 1},	/**/
	{"jg",      "bgt", 1},	/**/
	{"jge",     "bge", 1},	/**/
	{"jl",      "blt", 1},	/**/
	{"jle",     "ble", 1},	/**/
	{"jmp",     "br", 5},
	{"jna",     "blos", 1},	/**/
	{"jnae",    "blo", 1},	/**/
	{"jnb",     "bhis", 1},	/**/
	{"jnbe",    "bhi", 1},	/**/
	{"jnc",     "bcc", 1},	/**/
	{"jne",     "bne", 1},	/**/
	{"jng",     "ble", 1},	/**/
	{"jnge",    "blt", 1},	/**/
	{"jnl",     "bge", 1},	/**/
	{"jnle",    "bgt", 1},	/**/
	{"jno",     "bvc", 1},	/**/
	{"jnp",     "bpc", 1},	/**/
	{"jns",     "bpl", 1},	/**/
	{"jnz",     "bne", 1},	/**/
	{"jo",      "bvs", 1},	/**/
	{"jp",      "bps", 1},	/**/
	{"jpe",     "bps", 1},	/**/
	{"jpo",     "bpc", 1},	/**/
	{"js",      "bmi", 1},	/**/
	{"jz",      "beq", 1},	/**/
	{"lahf",    "lahf", 1},
	{"ldsw",    "lds", 2},
	{"leaw",    "lea", 2},
	{"lesw",    "les", 2},
	{"lodsb",   "lodsb", 1},
	{"lodsw",   "lodsw", 1},
	{"loop",    "loop", 1},
	{"loope",   "loope", 1},
	{"loopne",  "loopne", 1},
	{"loopnz",  "loopnz", 1},
	{"loopz",   "loopz", 1},
	{"movb",    "movb", 2},
	{"movsb",   "movsb", 1},
	{"movsw",   "movsw", 1},
	{"movw",    "mov", 2},
	{"mulb",    "mulb", 1},
	{"mulw",    "mul", 1},
	{"negb",    "negb", 1},
	{"negw",    "neg", 1},
	{"nop",     "nop", 1},
	{"notb",    "notb", 1},
	{"notw",    "not", 1},
	{"orb",     "orb", 2},
	{"orw",     "or", 2},
	{"outb",    "outb", 2},
	{"outw",    "out", 2},
	{"popa",    "popa", 1},
	{"popf",    "popf", 1},
	{"popfw",   "popf", 1},
	{"popw",    "pop", 1},
	{"pusha",   "pusha", 1},
	{"pushf",   "pushf", 1},
	{"pushfw",  "pushf", 1},
	{"pushw",   "push", 1},
	{"rclb",    "rclb", 4},
	{"rclw",    "rcl", 4},
	{"rcrb",    "rcrb", 4},
	{"rcrw",    "rcr", 4},
	{"rep",     "rep", 11},
	{"repe",    "repe", 11},
	{"repne",   "repne", 11},
	{"repnz",   "repnz", 11},
	{"repz",    "repz", 11},
	{"ret",     "ret", 1},
	{"rolb",    "rolb", 4},
	{"rolw",    "rol", 4},
	{"rorb",    "rorb", 4},
	{"rorw",    "ror", 4},
	{"sahf",    "sahf", 1},
	{"salb",    "salb", 4},
	{"salw",    "sal", 4},
	{"sarb",    "sarb", 4},
	{"sarw",    "sar", 4},
	{"sbbb",    "sbbb", 2},
	{"sbbw",    "sbb", 2},
	{"scasb",   "scasb", 1},
	{"scasw",   "scasw", 1},
	{"shlb",    "shlb", 4},
	{"shlw",    "shl", 4},
	{"shrb",    "shrb", 4},
	{"shrw",    "shr", 4},
	{"stc",     "stc", 1},
	{"std",     "std", 1},
	{"sti",     "sti", 1},
	{"stosb",   "stosb", 1},
	{"stosw",   "stosw", 1},
	{"subb",    "subb", 2},
	{"subw",    "sub", 2},
	{"testb",   "testb", 2},
	{"testw",   "test", 2},
	{"xchgb",   "xchgb", 1},
	{"xchgw",   "xchg", 1},
	{"xlat",    "xlatb", 11},
	{"xlatb",   "xlatb", 11},
	{"xorb",    "xorb", 2},
	{"xorw",    "xor", 2},
};

static int next(char *token)
{
    int chr;
    char *buf;

    buf = token;
/*********** Skip whitespace and comments ***********/
    do {
	chr = getchar();
	if((chr == EOF) || (chr == '\n'))
	    return chr;
	if((chr == '#') || (chr == ';')) {
	    do {
		chr = getchar();
	    } while((chr != '\n') && (chr != EOF));
	    return chr;
	}
    } while((chr <= ' ') || (chr > '~'));
/******************** Get number ********************/
    if(isdigit(chr)) {
	do {
	    *token++ = chr;
	    chr = getchar();
	} while(isdigit(chr));
	*token = 0;
	ungetc(chr, stdin);
	return '0';
    }
/******* Get alphanumeric, labels and opcodes *******/
    if(chr == '.') {
	chr = getchar();
	if(isalpha(chr) || (chr == '.') || (chr == '_')) {
	    *token++ = '.';
	}
	else {
	    ungetc(chr, stdin);
	    return '.';
	}
    }
    if(isalpha(chr) || (chr == '.') || (chr == '_')) {
	do {
	    *token++ = chr;
	    chr = getchar();
	} while(isalnum(chr) || (chr == '.') || (chr == '_'));
	if(chr == ':') {
	    *token++ = chr;
	    *token = 0;
	    return 'L';
	}
	*token = 0;
	ungetc(chr, stdin);
	for(opcode = 0; opcode < sizeof(mnemonics)/sizeof(instruction); opcode++) {
	    if (!strcasecmp(buf, mnemonics[opcode].att))
		break;
	}
	if(opcode >=  sizeof(mnemonics)/sizeof(instruction))
	    return 'A';
	return 'I';
    }
/**************** Get literal string ****************/
    else if(chr == '\"') {
	*token++ = chr;
	do {
	    chr = getchar();
	    if((chr < ' ') && (chr > '~'))
		continue;
	    *token++ = chr;
	    if(chr == '\\') {
		chr = getchar();
		if((chr < ' ') && (chr > '~'))
		    continue;
		*token++ = chr;
		chr = '\\';
	    }
	} while((chr != '\"') && (chr != '\n') && (chr != EOF));
	*token = 0;
	return 'S';
    }
/******************* Get register *******************/
    else if(chr == '%') {
	chr = getchar();
	if(isalpha(chr)) {
	    do {
		*token++ = chr;
		chr = getchar();
	    } while(isalnum(chr));
	    *token = 0;
	    ungetc(chr, stdin);
	    return 'R';
	}
	ungetc(chr, stdin);
	return '%';
    }
    return chr;
}

static int flush_line(void)
{
    int chr;

    do {
	chr = getchar();
    } while((chr != '\n') && (chr != EOF));
    return chr;
}

static int flush_opr(void)
{
    int chr;

    do {
	chr = getchar();
    } while((chr != '\n') && (chr != EOF) && (chr != ','));
    ungetc(chr, stdin);
    return chr;
}

static int mov_opr(char *ptr)
{
    int chr;

    do {
	chr = getchar();
	*ptr++ = chr;
    } while((chr != '\n') && (chr != EOF) && (chr != ','));
    *(ptr - 1) = 0;
    ungetc(chr, stdin);
    return 'A';
}

static int line(void)
{
    int nops, chr, i;
    char *bufptr;

    bufptr = buffer;

    /* Discard empty lines */
    do {
	chr = next(bufptr);
	if(chr == '\n')
	    lineno++;
    } while(chr == '\n');

    if(chr == EOF)
	return chr;

    /* Every label goes in its own line */
    if(chr == 'L') {
	printf("%s\n", bufptr);
	return chr;
    }
    i = opcode;
/*********** Process invalid instructions ***********/
    if((chr != 'I') || (mnemonics[opcode].type == 0)) {
	if(chr != 'I')
	    fprintf(stderr, "!!ERROR: Line %d: Unknown %s\n", lineno, bufptr);
	flush_line();
	lineno++;
	return '\n';
    }
    printf("\t%s\t", mnemonics[opcode].as86);
/************ Process .SECTION directive ************/
    if(mnemonics[opcode].type == 6) {
	chr = next(bufptr);
	for(nops = 0; nops < 4; nops++)
	    if(toupper((int)(*(bufptr + 1))) == sects[nops])
		break;
	printf("%s\n", sectmsg[nops]);
	flush_line();
	lineno++;
	return '\n';
    }
/************ Process .P4ALIGN directive ************/
    if(mnemonics[opcode].type == 7) {
	next(bufptr);
	chr = atoi(bufptr);
	printf("%d\n", (1 << chr));
	flush_line();
	lineno++;
	return '\n';
    }

/************* Process .SKIP directive **************/
    if(mnemonics[opcode].type == 8) {
	next(bufptr);
	printf("%s\n", bufptr);
	flush_line();
	lineno++;
	return '\n';
    }

/************* Process .COMM directive **************/
    if(mnemonics[opcode].type == 9) {
	next(bufptr);
	printf("%s,", bufptr);
	next(bufptr); next(bufptr);
	printf(" %s\n", bufptr);
	flush_line();
	lineno++;
	return '\n';
    }
/********** Process indirect call and jmp ***********/
    if(mnemonics[opcode].type == 5) {
	do {
	    chr = getchar();
	    if((chr == EOF) || (chr == '\n'))
		return chr;
	    if(chr == '*')
		break;
	} while((chr <= ' ') || (chr > '~'));
	if(chr != '*')
	    ungetc(chr, stdin);
    }

    nops = 0;
    do {
	operands[nops] = bufptr;
	do {
	    chr = next(bufptr);
	    switch(chr) {
		case 'R':
		    flush_opr();
		    break;
		case '$':
		    *bufptr = '#';
		    chr = mov_opr(bufptr + 1);
		    break;
		case '(':
		    *bufptr = '(';
		    chr = next(bufptr + 1);
		    if(chr == 'R') {
			*bufptr = '[';
			do {
			    bufptr = bufptr + (strlen(bufptr) + 1);
			    *(bufptr - 1) = ' ';
			    chr = next(bufptr);
			    if(chr != ')') {
				*bufptr++ = '+';
				next(bufptr);
			    }
			} while(chr != ')');
			chr = ']';
		    }
		    else {
			bufptr++;
		    }
		default:
		    if(!isalnum(chr)) {
			*bufptr = chr;
			*(bufptr+1) = 0;
		    }
	    }
	    bufptr = bufptr + (strlen(bufptr) + 1);
	    *(bufptr - 1) = ' ';
	} while((chr != '\n') && (chr != EOF) && (chr != ','));
	bufptr--;
	*(bufptr - 1) = 0;
	nops++;
    } while((chr != '\n') && (chr != EOF));
    opcode = i;
    switch(mnemonics[opcode].type) {
	case 11:
	    nops = 0;
	    break;
	case 4:
	    if(nops < 2) {
		operands[1] = imm_one;
		nops++;
		break;
	    }
	case 3:
	    if(nops >= 3) {
		bufptr = operands[0];
		operands[0] = operands[2];
		operands[2] = bufptr;
		break;
	    }
	case 2:
	    if(nops >= 2) {
		bufptr = operands[0];
		operands[0] = operands[1];
		operands[1] = bufptr;
	    }
	case 1:
	default:
	    break;
    }
    i = 0;
    while(nops--) {
	if(strlen(operands[i]))
	    printf("%s", operands[i]);
	if(nops)
	    printf(",");
	i++;
    };
    printf("\n");
    lineno++;
    return chr;
}

int main (int argc, char **argv)
{
/*char buffer[MAX_LINE];
char *operands[MAX_OPERANDS];*/
    int chr;

    do {
	chr = line();
    } while(chr != EOF);
    return 0;
}
