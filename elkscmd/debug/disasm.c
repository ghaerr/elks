/*
 * (Standalone) single-file tiny 8086 disassembler
 *
 * Jan 2022 Greg Haerr
 * Inspired by Andrew Jenner's 8086 simulator 86sim.cpp
 */
#include <stdio.h>
#include <string.h>
#include "disasm.h"

typedef unsigned char Byte;
typedef unsigned short int Word;
typedef unsigned int DWord;
typedef int bool;
enum { false = 0, true };

/* shared variables only if linked with sim.c */
bool wordSize;
bool sourceIsRM;
Byte opcode;
int f_asmout;
int f_outcol;

static Word startIP;
static Word startCS;
static Word startDS;

static Byte d_modRM;
//static int segOver;

/* user-defined external functions */
//extern Byte readByte(Word offset, int seg);	/* read next instruction byte */

static void decode();
static int (*fetchbyte)(int, int);

static Word d_fetchByte()
{
    Byte b = fetchbyte(startCS, startIP++);
	//Byte b = readByte(startIP++, 1);      /* for 86sim, seg =1 for CS */
	//if (!f_asmout) printf("%02x ", b);
    f_outcol++;
    return b;
}

int disasm(int cs, int ip, int (*nextbyte)(int, int), int ds)
{
	startCS = cs;
	startIP = (int)ip;
	fetchbyte = nextbyte;
    startDS = ds;
	f_outcol = 0;
	//if (!f_asmout) printf("%04hx:%04hx  ", cs, ip);
	decode();
	return startIP;
}

static const char *wordregs[] = {
	"%ax", "%cx", "%dx", "%bx", "%sp", "%bp", "%si", "%di"};
static const char *byteregs[] = {
	"%al", "%cl", "%dl", "%bl", "%ah", "%ch", "%dh", "%bh"};
static const char *segregs[] = { "%es", "%cs", "%ss", "%ds"};

static Word d_fetchWord() { Word w = d_fetchByte(); w += d_fetchByte() << 8; return w; }
static int d_modRMReg() { return (d_modRM >> 3) & 7; }
static void outREG() {
	if (wordSize || opcode == 0xee || opcode == 0xec)	// OUT dx
		printf("%s", wordregs[(d_modRM >> 3) & 7]);
	else printf("%s", byteregs[(d_modRM >> 3) & 7]);
}
static void outSREG() {
	printf("%s", segregs[(d_modRM >> 3) & 3]);
}
static void outRM(Word w) {
	signed char b;
	static const char *basemodes[] = {
		"(%bx,%si)", "(%bx,%di)", "(%bp,%si)", "(%bp,%di)",
        "(%si)", "(%di)", "(%bp)", "(%bx)" };

    switch (d_modRM & 0xc0) {
        case 0x00:
            if ((d_modRM & 0xc7) == 6)
                printf("(%s)", getsymbol(startDS, w));
			else printf("%s", basemodes[d_modRM & 7]);
            break;
        case 0x40:
			b = (signed char) w;	/* signed */
			if (b < 0) printf("-0x%02x", -b);
			else printf("0x%02x", b);
			printf("%s", basemodes[d_modRM & 7]);
			break;
        case 0x80:
			printf("0x%04x%s", w, basemodes[d_modRM & 7]);
			break;
        case 0xc0:
			if (wordSize) printf("%s", wordregs[d_modRM & 7]);
			else printf("%s", byteregs[d_modRM & 7]);
			break;
    }
}
#define BW			0x0001	/* display byte/word instruction */
#define RDMOD		0x0002	/* read modRMReg byte */
#define OPS2		0x0004	/* display both REG & R/M operands */
#define RM			0x0008	/* display R/M operand */
#define SREG		0x0010	/* display SREG operand */
#define REGOP		0x0020	/* display REG operand from opcode */
#define IMM			0x0040	/* display immediate operand */
#define BYTE		0x0080	/* fetch unsigned byte immediate operand */
#define SBYTE		0x0100	/* fetch signed byte */
#define WORD		0x0200	/* fetch word, and/or display word not immediate operand */
#define DWORD		0x0400	/* fetch second word */
#define MEMWORD		0x0800	/* display word memory address */
#define ACC			0x1000	/* display accumulator */
#define JMP			0x2000	/* display jmp w/byte, word or dword operand */
#define SHIFTBY1	0x4000	/* display shift 1 operand */
#define SHIFTBYCL	0x8000	/* display shift CL operand */
void out_bw(int flags)
{
	int bw;

	if ((flags & (BW|OPS2|RM)) == 0)
		return;

	/* discard non- BW, IMM, inc/dec and non-direct addressing */
	bw = (flags == BW || (flags & IMM) || opcode == 0xfe || opcode == 0xff);
	if (!bw && ((flags & (OPS2|RM)) && (d_modRM & 0xc7) != 6))
		return;
	/* discard immediate to register */
	if ((flags & IMM) && ((flags & (OPS2|RM)) && ((d_modRM & 0xc0) == 0xc0)))
		return;
	/* discard register operands */
	if (flags & REGOP)
		return;
	putchar(wordSize? 'w': 'b');
}
static void outs(const char *str, int flags)
{
	Word w = 0;
    Word w2 = 0;
	signed char c = 0;

	if (flags & RDMOD)
		d_modRM = d_fetchByte();
	if (flags & (OPS2|RM)) {
		if (((d_modRM & 0xc7) == 6) || ((d_modRM & 0xc0) == 0x80))
			w = d_fetchWord();
		if ((d_modRM & 0xc0) == 0x40)
			w = d_fetchByte();
	}
	if ((flags & (IMM|BYTE|SBYTE|WORD)) == IMM)
		w2 = !wordSize ? d_fetchByte() : d_fetchWord();

	if (flags & (WORD|DWORD|MEMWORD))
		w2 = d_fetchWord();
	if (flags & BYTE)
		w2 = d_fetchByte();
	if (flags & SBYTE)
		w2 = c = d_fetchByte();
	if (flags & DWORD)
		w = d_fetchWord();

	while (f_outcol++ < 6)
		printf("   ");
    if (f_asmout && !strcmp(str, "???")) {
        printf(".byte 0x%02x\n", opcode);
        return;
    }
	printf("%s", str);
	if (flags & BW) out_bw(flags);
	if (flags != 0) putchar('\t');
#if 0
	if (segOver != -1) {
		printf("%s", segregs[segOver]);
		putchar(':');
	}
#endif
	if ((flags & (OPS2|SREG)) == OPS2) {
		if (sourceIsRM) outRM(w); else outREG();
		putchar(',');
		if (sourceIsRM) outREG(); else outRM(w);
	}
	if ((flags & (OPS2|SREG)) == (OPS2|SREG)) {
		wordSize = 1;
		if (sourceIsRM) outRM(w); else outSREG();
		putchar(',');
		if (sourceIsRM) outSREG(); else outRM(w);
	}
	if ((flags & (IMM|BYTE|ACC)) == (IMM|BYTE|ACC)) {	// IN, OUT imm
		if (!sourceIsRM) printf("$0x%x,%s", w2, wordSize? "%ax": "%al");
		else printf("%s,$0x%x", wordSize? "%ax": "%al", w2);
		flags = 0;
	}
	else if (flags & IMM) {
		printf("$0x%x", w2);
		if (flags != (flags & IMM))
			printf(",");
	}
	if (flags & SHIFTBY1) printf("$1,");
	if (flags & SHIFTBYCL) printf("%%cl,");
	if (flags & RM) outRM(w);
	if ((flags & (OPS2|SREG)) == SREG)  outSREG();
	if ((flags & ACC) && sourceIsRM == 0)
		printf("%s,", wordSize? wordregs[0]: byteregs[0]);
	if ((flags & MEMWORD) && sourceIsRM)
        printf("(%s),", getsymbol(startDS, w2));
	if ((flags & ACC) && sourceIsRM)
		printf("%s", wordSize? wordregs[0]: byteregs[0]);
	if ((flags & MEMWORD) && sourceIsRM == 0)
        printf("(%s)", getsymbol(startDS, w2));
	if (flags & REGOP) printf("%s", wordSize? wordregs[opcode & 7]: byteregs[opcode & 7]);
	if ((flags & (JMP|IMM|WORD)) == WORD) printf("%u", w2);
    if (flags & JMP) {
        if (flags & SBYTE) {
            if (f_asmout) printf(".%s%d // %04x", c>=0? "+": "", c+2, startIP+c);
            else printf("%04x", startIP + c);
        }
        if (flags & WORD) {
            int waddr = (startIP + w2) & 0xffff;
            if (opcode == 0xfe || opcode == 0xff) {
                printf("*%s", getsymbol(startCS, w2));
            } else printf("%s // %04x", getsymbol(startCS, waddr), waddr);
        }
        if (flags & DWORD) {
            printf("$%s,$%s", getsegsymbol(w), getsymbol(w, w2));
        }
    }
    printf("\n");
}

static void decode(void)
{
	static const char *alunames[8] = {
		"add", "or", "adc", "sbb", "and", "sub", "xor", "cmp"
	};

	int flags;
#if 0
    bool prefix = false;
    //do {
        //if (!repeating) {
            if (!prefix) {
                segOver = -1;
                //rep = 0;
            }
            prefix = false;
            //opcode = fetchByte();
        //}
nextopcode:
#endif
		opcode = d_fetchByte();
        //if (rep != 0 && (opcode < 0xa4 || opcode >= 0xb0 || opcode == 0xa8 ||
            //opcode == 0xa9))
            //runtimeError("REP prefix with non-string instruction");
        wordSize = ((opcode & 1) != 0);
        sourceIsRM = ((opcode & 2) != 0);
        int operation = (opcode >> 3) & 7;
        switch (opcode) {
            case 0x00: case 0x01: case 0x02: case 0x03:
            case 0x08: case 0x09: case 0x0a: case 0x0b:
            case 0x10: case 0x11: case 0x12: case 0x13:
            case 0x18: case 0x19: case 0x1a: case 0x1b:
            case 0x20: case 0x21: case 0x22: case 0x23:
            case 0x28: case 0x29: case 0x2a: case 0x2b:
            case 0x30: case 0x31: case 0x32: case 0x33:
            case 0x38: case 0x39: case 0x3a: case 0x3b:  // alu rmv,rmv
				outs(alunames[(opcode >> 3) & 7], BW|RDMOD|OPS2);
				break;
            case 0x04: case 0x05: case 0x0c: case 0x0d:
            case 0x14: case 0x15: case 0x1c: case 0x1d:
            case 0x24: case 0x25: case 0x2c: case 0x2d:
            case 0x34: case 0x35: case 0x3c: case 0x3d:  // alu accum,i
				sourceIsRM = 1;	// acc dest
				//outs(alunames[(opcode >> 3) & 7], BW|RDMOD|IMM|ACC);
				outs(alunames[(opcode >> 3) & 7], BW|IMM|ACC);
				break;
            case 0x06: case 0x0e: case 0x16: case 0x1e:  // PUSH segreg
				d_modRM = opcode;
				outs("push", SREG);
                break;
            case 0x07: case 0x17: case 0x1f:  // POP segreg
				d_modRM = opcode;
				outs("pop", SREG);
                break;
            case 0x26: case 0x2e: case 0x36: case 0x3e:  // segment override
                {
#if 1
                static const char *segprefix[] = { "es", "cs", "ss", "ds"};
                outs(segprefix[operation - 4], 0);
                break;
#else
                segOver = operation - 4;
                prefix = true;
                goto nextopcode;
#endif
                }
            case 0x27: case 0x2f:  // DA
				outs(opcode == 0x27? "daa": "das", 0);
                break;
            case 0x37: case 0x3f:  // AA
				outs(opcode == 0x37? "aaa": "aas", 0);
                break;
            case 0x40: case 0x41: case 0x42: case 0x43:
            case 0x44: case 0x45: case 0x46: case 0x47:
            case 0x48: case 0x49: case 0x4a: case 0x4b:
            case 0x4c: case 0x4d: case 0x4e: case 0x4f:  // incdec rw
				wordSize = 1;
				outs((opcode & 8)? "dec": "inc", REGOP);
				break;
            case 0x50: case 0x51: case 0x52: case 0x53:
            case 0x54: case 0x55: case 0x56: case 0x57:  // PUSH rw
				wordSize = 1;
				outs("push", REGOP);
                break;
            case 0x58: case 0x59: case 0x5a: case 0x5b:
            case 0x5c: case 0x5d: case 0x5e: case 0x5f:  // POP rw
				wordSize = 1;
				outs("pop", REGOP);
                break;
            case 0x60: case 0x61: case 0x62: case 0x63:
            case 0x64: case 0x65: case 0x66: case 0x67:
            case 0x68: case 0x69: case 0x6a: case 0x6b:
            case 0x6c: case 0x6d: case 0x6e: case 0x6f:
            case 0xc0: case 0xc1: case 0xc8: case 0xc9:  // invalid
			case 0xf1:
            case 0xd8: case 0xd9: case 0xda: case 0xdb:
            case 0xdc: case 0xdd: case 0xde: case 0xdf:  // escape
			case 0x0f:  // POP CS
				outs("???", 0);
                break;
            case 0x70: case 0x71: case 0x72: case 0x73:
            case 0x74: case 0x75: case 0x76: case 0x77:
            case 0x78: case 0x79: case 0x7a: case 0x7b:
            case 0x7c: case 0x7d: case 0x7e: case 0x7f:  // Jcond cb
				{
				static const char *jumpnames[] = {
					"jo ", "jno", "jb ", "jae", "je ", "jne", "jbe", "ja ",
					"js ", "jns", "jp", "jnp","jl ", "jge", "jle", "jg " };
				outs(jumpnames[opcode & 0x0f], JMP|SBYTE);
				}
                break;
            case 0x80: case 0x81: case 0x82: case 0x83:  // alu rmv,iv
				d_modRM = d_fetchByte();
				flags = BW|IMM|RM;
				if (opcode == 0x81) flags |= WORD;
				else if (opcode == 0x83) flags |= SBYTE;
				else flags |= BYTE;
				outs(alunames[d_modRMReg()], flags);
                break;
            case 0x84: case 0x85:  // TEST rmv,rv
				sourceIsRM = 0;
				outs("test", BW|RDMOD|OPS2);
                break;
            case 0x86: case 0x87:  // XCHG rmv,rv
				sourceIsRM = 0;
				outs("xchg", BW|RDMOD|OPS2);
                break;
            case 0x88: case 0x89:  // MOV rmv,rv
				sourceIsRM = 0;
				outs("mov", BW|RDMOD|OPS2);
                break;
            case 0x8a: case 0x8b:  // MOV rv,rmv
				sourceIsRM = 1;
				outs("mov", BW|RDMOD|OPS2);
                break;
            case 0x8c:  // MOV rmw,segreg
				sourceIsRM = 0;
				outs("mov", RDMOD|OPS2|SREG);
				break;
            case 0x8d:  // LEA
				sourceIsRM = 1;
				outs("lea", RDMOD|OPS2);
                //if (!useMemory) runtimeError("LEA needs a memory address");
                break;
            case 0x8e:  // MOV segreg,rmw
				sourceIsRM = 1;
				outs("mov", RDMOD|OPS2|SREG);
                break;
            case 0x8f:  // POP rmw
				outs("pop", RDMOD|RM);
                break;
            case 0x90: case 0x91: case 0x92: case 0x93:
            case 0x94: case 0x95: case 0x96: case 0x97:  // XCHG AX,rw
				wordSize = 1;
				sourceIsRM = 0;	// acc src
				outs("xchg", ACC|REGOP);
                break;
            case 0x98:  // CBTW
                outs("cbtw", 0);
                break;
            case 0x99:  // CWTD
                outs("cwtd", 0);
                break;
            case 0x9a:  // CALL cp
				outs("lcall", JMP|DWORD);
                break;
            case 0x9b:  // WAIT
				outs("fwait", 0);
				break;
            case 0x9c:  // PUSHF
                outs("pushf", 0);
                break;
            case 0x9d:  // POPF
                outs("popf", 0);
                break;
            case 0x9e:  // SAHF
                outs("sahf", 0);
                break;
            case 0x9f:  // LAHF
                outs("lahf", 0);
                break;
            case 0xa0:  // MOVB accum,xv
				wordSize = 0;
				sourceIsRM = 1;	// acc dest
				outs("mov", BW|MEMWORD|ACC);
				break;
			case 0xa1:  // MOVW accum,xv
				wordSize = 1;
				sourceIsRM = 1;	// acc dest
				outs("mov", BW|MEMWORD|ACC);
                break;
            case 0xa2:  // MOVB xv,accum
				wordSize = 0;
				sourceIsRM = 0;	// acc src
				outs("mov", BW|ACC|MEMWORD);
				break;
			case 0xa3:  // MOVW xv,accum
				wordSize = 1;
				sourceIsRM = 0;	// acc src
				outs("mov", BW|ACC|MEMWORD);
                break;
            case 0xa4: case 0xa5:  // MOVSv
				outs("movs", BW);
                break;
            case 0xa6: case 0xa7:  // CMPSv
				outs("cmps", BW);
                break;
			case 0xa8: case 0xa9:  // TEST accum,iv
				sourceIsRM = 1;	// acc dest
				outs("test", BW|IMM|ACC);
                break;
            case 0xaa: case 0xab:  // STOSv
				outs("stos", BW);
                break;
            case 0xac: case 0xad:  // LODSv
				outs("lods", BW);
                break;
            case 0xae: case 0xaf:  // SCASv
				outs("scas", BW);
                break;
            case 0xb0: case 0xb1: case 0xb2: case 0xb3:
            case 0xb4: case 0xb5: case 0xb6: case 0xb7:
				wordSize = 0;
				outs("mov", BW|IMM|REGOP);
                break;
            case 0xb8: case 0xb9: case 0xba: case 0xbb:
            case 0xbc: case 0xbd: case 0xbe: case 0xbf:  // MOV rv,iv
				wordSize = 1;
				outs("mov", BW|IMM|REGOP);
                break;
            case 0xc2: case 0xc3:  // RET
                outs("ret", !wordSize? WORD: 0);    //FIXME should display $WORD
				break;
			case 0xca: case 0xcb:  // RETF
                outs("lret", !wordSize? WORD: 0);   //FIXME should display $WORD
                break;
            case 0xc4: case 0xc5:  // LES/LDS
				//if (!useMemory) runtimeError("This instruction needs a memory address");
				wordSize = 1;
				sourceIsRM = 1;
				outs((opcode & 1)? "lds": "les", RDMOD|OPS2);
                break;
            case 0xc6: case 0xc7:  // MOV rmv,iv
				outs("mov", BW|RDMOD|IMM|RM);
                break;
            case 0xcc:  // INT 3
				outs("int3", 0);
				break;
            case 0xcd:
				wordSize = 0;
				outs("int", IMM);
                break;
			case 0xce:  // INTO
				outs("into", 0);
				break;
            case 0xcf:  // IRET
				outs("iret", 0);
                break;
            case 0xd0: case 0xd1: case 0xd2: case 0xd3:  // rot rmv,n
				{
				static const char *rotates[] = {
					"rol", "ror", "rcl", "rcr", "shl", "shr", "shl", "sar" };
				d_modRM = d_fetchByte();
				flags = BW|RM;
				if (opcode & 2) flags |= SHIFTBYCL;
				else flags |= SHIFTBY1;
				outs(rotates[d_modRMReg()], flags);
				}
                break;
            case 0xd4:  // AAM
				outs("aam", 0);     //FIXME should display $BYTE
                break;
            case 0xd5:  // AAD
				outs("aad", 0);     //FIXME should display $BYTE
                break;
            case 0xd6:  // SALC (undocumented)
				outs("salc", 0);
                break;
            case 0xd7:  // XLATB
				outs("xlatb", 0);   //FIXME xlat %ds:(%bx)?
                break;
            case 0xe0: case 0xe1: case 0xe2:  // LOOPc cb
				outs(opcode == 0xe0? "loopne":
                     opcode == 0xe1? "loope": "loop", JMP|SBYTE);
                break;
            case 0xe3:  // JCXZ cb
				outs("jcxz", JMP|SBYTE);
                break;
            case 0xe8:  // CALL cw
				outs("call", JMP|WORD);
                break;
            case 0xe9:  // JMP cw
				outs("jmp", JMP|WORD);
                break;
            case 0xea:  // JMP cp
				outs("ljmp", JMP|DWORD);
                break;
            case 0xeb:  // JMP cb
				outs("jmp", JMP|SBYTE);
                break;
            case 0xe4: case 0xe5: case 0xe6: case 0xe7:  // IN, OUT ib
				outs((opcode & 2)? "out": "in ", IMM|BYTE|ACC);
				break;
            case 0xec: case 0xed: case 0xee: case 0xef:  // IN, OUT dx
				d_modRM = 0xd0;
				outs((opcode & 2)? "out": "in ", OPS2);
                break;
			case 0xf0:  // LOCK
				outs("lock", 0);
				break;
            case 0xf2:  // REPNZ
				//prefix = true;
				outs("repnz ", 0);
				break;
			case 0xf3:  // REPZ
				//prefix = true;
				outs("repz ", 0);
				break;
			case 0xf4:  // HLT
				outs("hlt", 0);
				break;
            case 0xf5:  // CMC
                outs("cmc", 0);
                break;
            case 0xf6: case 0xf7:  // math rmv
				d_modRM = d_fetchByte();
                //data = readEA();
                switch (d_modRMReg()) {
                    case 0: case 1:  // TEST rmv,iv
						outs("test", BW|IMM|RM);
                        break;
                    case 2:  // NOT iv
						outs("not", BW|RM);     //FIXME f6=notb, f7=notw
                        break;
                    case 3:  // NEG iv
						outs("neg", BW|RM);
                        break;
                    case 4: case 5:  // MUL rmv, IMUL rmv
						outs(d_modRMReg() == 4? "mul": "imul", BW|RM);
						break;
                    case 6: case 7:  // DIV rmv, IDIV rmv
						outs(d_modRMReg() == 4? "div": "idiv", BW|RM);
						break;
                }
                break;
            case 0xf8: case 0xf9:  // STC/CLC
                outs(wordSize? "stc": "clc", 0);
                break;
            case 0xfa: case 0xfb:  // STI/CLI
                outs(wordSize? "sti": "cli", 0);
                break;
            case 0xfc: case 0xfd:  // STD/CLD
                outs(wordSize? "std": "cld", 0);
                break;
            case 0xfe: case 0xff:  // misc
				d_modRM = d_fetchByte();
                if ((!wordSize && d_modRMReg() >= 2 && d_modRMReg() <= 6) ||
                    d_modRMReg() == 7) {
					outs("???", 0);
                } else switch (d_modRMReg()) {
                    case 0:  // inc rmv
						outs("inc", BW|RM);
						break;
					case 1:  // dec rmv
						outs("dec", BW|RM);
                        break;
                    case 2:  // CALL rmv
						outs("call", RM);
                        break;
                    case 3:  // CALL mp
						outs("lcallw", JMP|WORD);
                        break;
                    case 4:  // JMP rmw
						outs("jmp", RM);
                        break;
                    case 5:  // JMP mp
						outs("ljmpw", JMP|WORD);
                        break;
                    case 6:  // PUSH rmw
						outs("push", RM);
                        break;
                }
                break;
        }
    //} while (prefix == true);
}
