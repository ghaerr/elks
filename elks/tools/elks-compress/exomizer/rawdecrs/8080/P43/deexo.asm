;Exomizer3 Intel 8080 decoder version by Ivan Gorodetsky
;Based on Exomizer2 Z80 decoder by Metalbrain, but now it is nearly completely rewritten
;Compression algorithm by Magnus Lind
;input: 	hl=compressed data start
;			de=uncompressed destination start
;			you may change exo_mapbasebits to point to any 256-byte aligned free buffer
;v1.1 - 2019-10-07
;compress with <raw -P43> options (without #DEFINE BACKWARD_DECOMPRESS)
;264 bytes - reusable
;258 bytes - single-use
;
;compress with <raw -P43 -b> options (with #DEFINE BACKWARD_DECOMPRESS)
;259 bytes - reusable
;253 bytes - single-use
;
;Compile with The Telemark Assembler (TASM) 3.2

exo_mapbasebits		.equ	0


;#DEFINE BACKWARD_DECOMPRESS
;#DEFINE REUSABLE

#IFNDEF BACKWARD_DECOMPRESS

.DEFINE NEXT_HL inx h
.DEFINE NEXT_DE inx d
.DEFINE ADD_OFFSET mov a,e\ sub l\ mov l,a\ mov a,d\ sbb h\ mov h,a

#ELSE

.DEFINE NEXT_HL dcx h
.DEFINE NEXT_DE dcx d
.DEFINE ADD_OFFSET dad d

#ENDIF


deexo:

#IFDEF REUSABLE
			mvi a,80h
			sta	exo_getbit+1
#ENDIF

			push d
			xra a
			mov c,a
exo_initbits:
			sui 48
			jnc exo_initbits
			adi 48
			jnz exo_node1
			lxi d,1
exo_node1:
			mvi b,4
			push d
			call exo_getbits_
			xra a
			ora e
			pop d
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
			pop	d
			xra a
			mov b,a
			mov c,a
#IFDEF REUSABLE
			inr a
#ENDIF
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
			call exo_getbit
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
			mvi e,0
exo_gettingbits:
			call exo_getbit
			mov a,e\ adc a\ mov e,a
			dcr	b
			jnz	exo_gettingbits
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
exo_getbit:
			mvi a,80h
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

			.end