; 26 Jul 92
; Copyright (c) 1999 Greg Haerr <greg@censoft.com>
; asm.h - all asm*.s TC/MC/Aztec assembly include file
;
; 7/26/92 v6.3 TSC support 
; 11/17/92 original version
;
; #define __LARGE__, __MEDIUM__, __SMALL__ for model
; #define AZTEC, TURBOC, or MSC for C Compiler
;
; Routines included:
;
;	include asm.h	- start the assembly file
;	.header		- create the code and data segs
;	.cseg		- start the code segment
;	.cend		- end the code segment
;	.dseg		- start the data segment
;	.dend		- end the data segment
;	.dsym name,type	- declare global C data (type=word,byte)
;	.cextp name	- reference an external C procedure, use model as type
;	.cextrn name,type- reference external C variable and type(word,byte)
;	.cproc name	- declare a C-accessible procedure, use model as type
;	.cendp name	- end C procedure
;	.center		- enter C procedure (set up BP etc)
;	.cexit		- exit C procedure (restore BP etc)

;
; Check Model Symbol
IFNDEF __LARGE__
  IFNDEF __MEDIUM__
    IFNDEF __SMALL__
	%OUT You must supply a model symbol, __LARGE__, __SMALL__, or __MEDIUM__
    ENDIF
  ENDIF
ENDIF
;
; Check Compiler Symbol
IFDEF AZTEC
	include asmaz.h
ELSE
  IFDEF TURBOC
	include asmtc.h
  ELSE
    IFDEF MSC
    	include asmmsc.h
    ELSE
      IFDEF TSC
      	include asmtsc.h
      ELSE
    	%OUT You must supply a Compiler symbol, AZTEC, TURBOC, MSC or TSC.
    	.ERR
      ENDIF
    ENDIF
  ENDIF
ENDIF
;
; Define other global symbols
IFDEF __LARGE__
LPROG	equ	1
LDATA	equ	1
ifdef AZTEC
PROCP	equ	far
else
PROCPTR	equ	far ptr
endif
ENDIF

IFDEF __MEDIUM__
LPROG	equ	1
LDATA	equ	0
ifdef AZTEC
PROCP	equ	far
else
PROCPTR	equ	far ptr
endif
ENDIF

IFDEF __SMALL__
LPROG	equ	0
LDATA	equ	0
ifdef AZTEC
PROCP	equ	near
else
PROCPTR	equ	near ptr
endif
ENDIF
;
