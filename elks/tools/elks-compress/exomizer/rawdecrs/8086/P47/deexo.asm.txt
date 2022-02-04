;Exomizer3 Intel 8088/86 decoder version by Ivan Gorodetsky
;Compression algorithm by Magnus Lind
;
;Memory model - Tiny
;input: 	si=compressed data start
;			di=uncompressed destination start
;			you may change exo_mapbasebits to point to any 256-byte aligned free buffer
;
;v1.1 - 2019-10-28
;
;compress with <raw -P47> options (with BACKWARD equ 0)
;compress with <raw -P47 -b> options (with BACKWARD equ 1)
;212 bytes
;
;Compile with flat assembler (FASM)


BACKWARD equ 0

macro SetDir {
 if BACKWARD eq 1
  std
 else
  cld
 end if
}

macro NextSI {
 if BACKWARD eq 1
  dec si
 else
  inc si
 end if
}

macro AddOffset {
 if BACKWARD eq 1
  add si,bp
 else
  sub si,bp
 end if
}

macro GetBit {
 add al,al
 jnz $+5
 lodsb
 adc al,al
}

exo_table equ 0

deexo:
		mov ax,0180h
		mov bx,exo_table
		xor ch,ch
		SetDir
		push di
exo_initable:
		test bl,63
		jnz exo_node1
		mov di,1
exo_node1:
		mov cl,4
		call exo_getbits
		mov cl,dl
		ror cl,1
		mov [bx],cl
		jnc exo_skipPlus8
		add cl,-128+8
exo_skipPlus8:
		mov bp,1
		shl bp,cl
		inc bx
		inc bx
		mov [bx],di
		inc bx
		inc bx
		add di,bp
		cmp bl,52*4
		jnz exo_initable
		pop di
		xor cx,cx
exo_literalcopy1:
		movsb
		stc
exo_mainloop:
		adc ah,ah
		GetBit
		jc exo_literalcopy1
		dec cx
exo_getindex:
		inc cx
		GetBit
		jnc exo_getindex
		cmp cl,16
		jc exo_continue
		jz exo_ret
		mov ch,[si]
		NextSI
		mov cl,[si]
		NextSI
exo_literalcopy:
		rep movsb
		stc
		jmp exo_mainloop
exo_continue:
		mov bl,cl
		call exo_getpair
		mov cl,ah
		and cl,3
		dec cl
		jnz not_reuse_offset
		GetBit
		jnc not_reuse_offset
		mov cx,dx
		jmp reuse_offset
not_reuse_offset:
		push dx
		mov cl,02h
		mov bl,30h
		dec dx
		jz exo_gogetbits
		mov cl,04h
		mov bl,20h
		dec dx
		jz exo_gogetbits
		mov bl,10h
exo_gogetbits:
		call exo_getbits
		add bl,dl
		call exo_getpair
		mov bp,dx
		pop cx
reuse_offset:
		mov dx,si
		mov si,di
		AddOffset
		rep movsb
		mov si,dx
		jmp exo_mainloop

exo_getpair:
		add bl,bl
		add bl,bl
		mov cl,[bx]
		call exo_getbits
		inc bx
		inc bx
		add dx,[bx]
		ret

exo_getbits:
		xor dx,dx
		test cl,cl
		jz exo_ret
		jns exo_gettingbits
		and cl,7Fh
		jz GT7z
		call exo_gettingbits
		mov dh,dl
GT7z:
		mov dl,[si]
		NextSI
		ret
exo_gettingbits:
		GetBit
		adc dx,dx
		loop exo_gettingbits
exo_ret:
		ret


