;Exomizer3 Intel 8080 decoder version by Ivan Gorodetsky
;Based on Exomizer2 Z80 decoder by Metalbrain, but now it is nearly completely rewritten
;Compression algorithm by Magnus Lind
;
;1. Initialisation
;input: 	hl=compressed table start
;			call deexo_MakeTable
;2. Decompression
;input: 	hl=compressed data start
;			de=uncompressed destination start
;			you may change exo_mapbasebits to point to any 256-byte aligned free buffer
;			call deexo
;
;v1.1 - 2019-10-08
;compress with <raw -P43 -E> options (without #DEFINE BACKWARD_DECOMPRESS)
;279 bytes
;
;compress with <raw -P43 -E -b> options (with #DEFINE BACKWARD_DECOMPRESS)
;274 bytes
;
;Compile with The Telemark Assembler (TASM) 3.2

exo_mapbasebits		.equ	0

;#DEFINE BACKWARD_DECOMPRESS

.include "deexohead.asm"

#IFNDEF BACKWARD_DECOMPRESS

.DEFINE NEXT_HL inx h
.DEFINE NEXT_DE inx d
.DEFINE ADD_OFFSET mov a,e\ sub l\ mov l,a\ mov a,d\ sbb h\ mov h,a

#ELSE

.DEFINE NEXT_HL dcx h
.DEFINE NEXT_DE dcx d
.DEFINE ADD_OFFSET dad d

#ENDIF


deexo_MakeTable:
			xra a
			mov c,a
exo_initbits:
			sui 48
			jnc exo_initbits
			adi 48
			jnz exo_node1
			lxi d,1
exo_node1:
			rrc
			mov a,m
			jc LoNib
			rrc
			rrc
			rrc
			rrc
			.db 0FEh
LoNib:
			NEXT_HL
			ani 0Fh
			
			rrc
			push h
			mvi b,exo_mapbasebits>>8
			stax b
			jnc skipPlus8
			adi -128+8
skipPlus8:
			lxi h,1
			jz skip_exo_setbit
exo_setbit:
			dad	h
			dcr a
			jnz	exo_setbit
skip_exo_setbit:
			dad	d
			inr c
			mov a,e
			stax b
			inr c
			mov a,d
			stax b
			inr c
			xchg
			pop	h
			mov a,c
			cpi 52*3
			jnz	exo_initbits
			ret

deexo:
			mvi a,80h
			sta	exo_getbit+1
			xra a
			mov b,a
			mov c,a
			inr a
			jmp	exo_mainloop
not_reuse_offset:
			push d
			lxi	b,0230h
			dcx d
			mov a,d
			ora e
			jz	exo_goforit
			lxi	b,0420h
			dcx d
			mov a,d
			ora e
			jz	exo_goforit
exo_goforit_:
			mvi	c,10h
exo_goforit:
			call exo_getbits_

			mov a,c
			add e
			mov c,a
			call exo_getpair
			pop b
			xthl
			xchg
			shld reuse_offset+1
reuse_offset_:
			ADD_OFFSET
			call ldir
			pop	h
exo_mainloop:
			sta reuse_offset_state+1
			call exo_getbit_InrC
			jc	exo_literalcopy
			mvi	c,0FFh
exo_getindex:
			call exo_getbit_InrC
			jnc	exo_getindex
			mov a,c
			cpi 16
			jc	exo_continue
			rz
			mov b,m
			NEXT_HL
			mov c,m
			NEXT_HL
exo_literalcopy:
			call ldir
reuse_offset_state:
			mvi a,1
			stc
			adc a
			jmp	exo_mainloop
exo_continue:
			push d
			call exo_getpair
			lda reuse_offset_state+1
			ani 3
			jpe not_reuse_offset
			call exo_getbit_
			jnc not_reuse_offset
			mov c,e
			mov b,d
			pop d
			push h
reuse_offset:
			lxi h,0
			jmp reuse_offset_

exo_getpair:
			add a
			add c
			mov c,a
			mvi b,exo_mapbasebits>>8
			ldax b
			mov b,a
			mov e,a
			mov d,a
			ora a
			cnz exo_getbits

			inr c
			mvi b,exo_mapbasebits>>8
			ldax b
			inr c
			add e
			mov e,a
			ldax b
			adc d
			mov d,a
			ret

exo_getbits:
			jm GT7
exo_getbits_:
			xchg
exo_getbit:
			lxi h,0080h
			mov a,l
exo_gettingbits:
			add a
			jnz $+7 \ ldax d\ NEXT_DE\ mov l,a\ adc a
			dad h
			dcr	b
			jnz	exo_gettingbits
exo_gettingbitsexit:
			xchg
			sta exo_getbit+1
			mov e,d
			mov d,b
			ret
GT7:
			ani 7Fh
			mov b,a
			mov e,a
			cnz exo_getbits_
			mov d,e
			mov e,m
			NEXT_HL
			ret

exo_getbit_InrC:
			inr c
exo_getbit_:
			lda	exo_getbit+1
			add a
			sta	exo_getbit+1
			rnz
			mov a,m
			NEXT_HL
			adc a
			jmp $-7

ldir:
			mov	a,m
			stax d
			NEXT_HL
			NEXT_DE
			dcx b
			mov a,b
			ora c
			jnz $-7
			ret

packed:


			.end