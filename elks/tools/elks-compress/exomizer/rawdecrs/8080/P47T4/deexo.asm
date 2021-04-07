;Exomizer3 Intel 8080 decoder version by Ivan Gorodetsky
;Based on Exomizer2 Z80 decoder by Metalbrain, but now it is nearly completely rewritten
;Compression algorithm by Magnus Lind
;input: 	hl=compressed data start
;			de=uncompressed destination start
;			you may change exo_mapbasebits to point to any 256-byte aligned free buffer
;
;v1.1 - 2019-10-28
;
;compress with <raw -P47 -T4> options (without #DEFINE BACKWARD_DECOMPRESS)
;294 bytes - reusable
;285 bytes - single-use
;compress with <raw -P47 -T4 -b> options (with #DEFINE BACKWARD_DECOMPRESS)
;289 bytes - reusable
;280 bytes - single-use
;
;Compile with The Telemark Assembler (TASM) 3.2

exo_mapbasebits		.equ	0FF00h

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

			.org 8000h

deexo:
#IFDEF REUSABLE
			mvi a,80h
			sta	exo_getbit+1
			rlc
			sta reuse_offset_state+1
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
			lxi b,0
exo_literalcopy1:
			mov a,m\ stax d\ NEXT_HL\ NEXT_DE
exo_mainloop_:
			stc
reuse_offset_state:
			mvi a,1
			adc a
exo_mainloop:
			sta reuse_offset_state+1
			lda	exo_getbit+1
			add a
			jnz $+6\ mov a,m\ NEXT_HL\ adc a
			sta	exo_getbit+1
			jc	exo_literalcopy1
			dcr c
exo_getindex:
			inr	c
			add a
			jnz $+6\ mov a,m\ NEXT_HL\ adc a
			jnc	exo_getindex
			sta	exo_getbit+1
			mov a,c
			cpi 16
			jc	exo_continue
			rz
			mov b,m
			NEXT_HL
			mov c,m
			NEXT_HL
			mov	a,m
			stax d
			NEXT_HL
			NEXT_DE
			dcx b
			mov a,b
			ora c
			jnz $-7
			jmp	exo_mainloop_
exo_continue:
			push d
			call exo_getpair
			lda reuse_offset_state+1
			ani 3
			jpe not_reuse_offset
			lda	exo_getbit+1
			add a
			jnz $+6\ mov a,m\ NEXT_HL\ adc a
			sta	exo_getbit+1
			jnc not_reuse_offset
			mov c,e
			mov b,d
			pop d
			push h
reuse_offset:
			lxi h,0
			jmp exo_matchcopy
not_reuse_offset:
			push d
			lxi	b,0230h
			dcr e
			jz	exo_goforit
			lxi	b,0420h
			dcr e
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
exo_matchcopy:
			ADD_OFFSET
			mov	a,m
			stax d
			NEXT_HL
			NEXT_DE
			dcx b
			mov a,b
			ora c
			jnz	$-7
			pop	h
			jmp	exo_mainloop
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


			.end