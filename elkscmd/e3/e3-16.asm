;-------------------------------------------------------------------------
;
;  Copyright (C) 2002,03 Albrecht Kleine  <kleine@ak.sax.de>
;
;  This program is free software; you can redistribute it and/or
;  modify it under the terms of the GNU General Public License
;  as published by the Free Software Foundation; either
;  version 2 of the License, or (at your option) any later version.
;
;  This library is distributed in the hope that it will be useful,
;  but WITHOUT ANY WARRANTY; without even the implied warranty of
;  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
;  Library General Public License for more details.
;
;  You should have received a copy of the GNU Library General Public
;  License along with this library; if not, write to the Free Software
;  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
;
;--------------------------------------------------------------------------
;
; This is e3-16, a 16 bit derived work of 32 bit micro editor e3, ...
; ...running on ELKS (==Embeddable Linux Kernel Subset)  and DOS.
; Unlike e3 we are supporting WStar key bindings only.
; Unlike e3 "BAK-files" (*~) are not yet created.
;
TAB		equ 8
TABCHAR		equ 09h 		; ^I
SPACECHAR	equ ' '
CHANGED		equ '*'
UNCHANGED	equ SPACECHAR
NEWLINE		equ 0ah
errlen		equ 100
MAXERRNO	equ 30
ERRNOMEM	equ 12
ERRNOIO		equ 5
sBREITE    	equ 80			;cols
sHOEHE		equ 24			;rows
MAXLEN 		equ 0x7FFF
maxfilenamelen	equ 255
%ifdef ELKS
 %define LESSWRITEOPS
 stdin		equ 0
 stdout 	equ 1
 O_WRONLY_CREAT_TRUNC equ 1101q
 O_RDONLY	equ 0
 PERMS		equ 644q
%else	;--------------
 normfarbe	equ 07h
 kursorfarbe	equ 70h
 slinefarbe	equ 1eh			;yellow on blue
 blockfarbe	equ 15
 O_WRONLY_CREAT_TRUNC equ 0
%endif

section .text
bits 16

global _start
global _main				; for ELKS and the ld86 linker
_start:	
_main:
EXE_startcode:
%ifdef EXESTUB
..start:
%endif
;-------
%ifdef ELKS
	call InitBSS
	pop ax
	pop bx
	pop si				;si points to first arg
%else
%ifdef COM
	org 100h
%else
%ifdef EXESTUB
	mov ax,data
	mov es,ax
	push ax
%else
%ifdef EXE
	EXE_realstacksize equ 0x800
	org 0e0h
header_start:
;EXE header adapted from a NASM contribution by Yann Guidon <whygee_corp@hol.fr>
	db 'M','Z'			; EXE file signature
	dw EXE_allocsize % 512
	dw (EXE_allocsize + 511) / 512
	dw 0				; relocation information: none
	dw (header_end-header_start)/16 ; header size in paragraphs
	dw (EXE_absssize + EXE_realstacksize) / 16 ; min extra mem
	dw (EXE_absssize + EXE_realstacksize) / 16 ; max extra mem
	dw -10h				; Initial SS (before fixup)
	dw EXE_endbss + EXE_realstacksize ; 2k
	dw 0				; (no) Checksum
	dw 100h				; Initial IP - start just after the header
	dw -10h				; Initial CS (before fixup)
	dw 0				; file offset to relocation table: none
	dw 0,0,0			; (no overlay)
header_end:				; here we go... (@ org 100h)
%endif
%endif
%endif
	call InitBSS
	call GetArg
%ifdef EXESTUB
	pop ds
%endif
%endif
;-----------------------------------------------------------------------
;
; start with OUTER editor loop
;
ReStart:call NewFile
	jc E3exit
MainCharLoop:call DispNewScreen
	call RestoreStatusLine
	call HandleChar
	cmp byte [endedit],0
	je MainCharLoop
	xor si,si			;just like if no arg is present
	cmp byte [endedit],2
	je ReStart			;^KD repeat edit using another file
E3exit:	call KursorStatusLine
;-------
%ifdef ELKS
	mov bx,stdout   		;file handle
	mov cx,newline			;enter next line on terminal
	xor dx,dx
	inc dx				;mov dx,1
	push dx
	call WriteFile
	pop ax				;mov ax,1
	xor bx,bx			;return 0
	int 80h
%else
	mov ah,4ch
	int 21h
%endif
;----------------------------------------------------------------------
;
; MAIN function for processing keys
;
HandleChar:call ReadChar
	jz ExtAscii			;DOS version got ah=0 by int 16 for F-keys and cursor keys
	cmp al,19h			;^Y is the last
	ja NormChar
	mov bl,al
	add bl,jumps1
	jmp short CompJump2
NormChar:call CheckMode
	jnz OverWriteChar
	push ax
	xor ax,ax
	inc ax
	call InsertByte
	pop ax
	jc InsWriteEnd			;error: text buffer full
OverWriteChar:cld
	stosb
	mov byte [changed],CHANGED
InsWriteEnd:ret
;-------
;
; helper for HandleChar
;
CtrlKMenu:mov bx,Ktable
	mov cx,4b5eh			;^K
	jmp short Menu
CtrlQMenu:mov bx,Qtable
	mov cx,515eh			;^Q
Menu:	call MakeScanCode
	jc EndeRet			;if no valid scancode
ExtAscii:mov bl,ah			;don't use al (carries char e.g. TAB)
	sub bl,lowest			;= scan code first key in jumptab1
	jb EndeRet
	cmp bl,jumps1
	jae EndeRet
CompJump2:and bx,0ffh
	shl bx,1			;2*bx is due 2 byte per entry
;-------
	call [bx+jumptab1]
;-------
	cmp byte [numeriere],1		;after return from functions...
	jnz BZNret			;...decide whether count current line number
	mov word [linenr],0
	push di
	mov si,sot
	xchg si,di
BZNLoop:inc word [linenr]
	call LookForward
	inc di				;point to start of next line
%ifndef ELKS
	inc di				;for DOS one extra 
%endif
	cmp di,si
	jbe BZNLoop
	pop di
	mov byte [numeriere],0
BZNret:	ret
;-------
MakeScanCode:call WriteTwo		;bx expects xlat-table
	push bx
	call GetChar
	pop bx
	and al,01fh
	cmp al,26
	jnb exit
	xlatb
	mov ah,al			;returns pseudo "scancode"
	stc
exit:	cmc				;ok=nc
EndeRet:ret
;----------------------------------------------------------------------
;
; processing special keys: cursor, ins, del
;
KeyRet:	call CheckMode
	jnz  OvrRet
	call CountToLineBegin		;set si / returns ax
	inc si
	inc si
	xor bx,bx
	or ax,ax
	jz KeyRetNoIndent
	dec bx
KeyRetSrch:inc bx			;search non (SPACE or TABCHAR)
	cmp byte [si+bx],SPACECHAR
	je KeyRetSrch
	cmp byte [si+bx],TABCHAR
	je KeyRetSrch
KeyRetNoIndent:
	push si
	push bx			;ax is 0 or =indented chars
	call GoDown
	pop ax

	push ax
	inc ax				;1 extra for 0ah
%ifndef ELKS
	inc ax
%endif
	call InsertByte
	pop cx				;# blanks
	pop si				;where to copy
	jc SimpleRet
	inc word [linenr]
	cld
%ifdef ELKS
	mov al,NEWLINE
	stosb
%else
	mov ax,0a0dh
	stosw
%endif
	jcxz SimpleRet
	rep movsb			;copy upper line i.e. SPACES,TABS into next
SimpleRet:ret
OvrRet:	mov word [ch2linebeg],0
	jmp short DownRet
;-------
KeyDown:call CountColToLineBeginVis
DownRet:call GoDown
	call LookLineDown
	jmp short SetColumn
;-------
KeyUp:	call GoUp
	call CountColToLineBeginVis
	call LookLineUp
	jmp short SetColumn
;-------
KeyPgUp:call CountColToLineBeginVis
	call LookPageUp
	jmp short SetColumn
;-------
KeyPgDn:call CountColToLineBeginVis
	call LookPgDown			;1st char last line
;-------
SetColumn:mov cx,[ch2linebeg]		;maximal columns
	xor dx,dx			;counts visible columns i.e. expand TABs
	dec di
lod:	inc di
	cmp dx,cx			;from CountColToLineBeginVis
	jae fert
%ifdef ELKS
	cmp byte [di],NEWLINE		;don't go beyond line earlier line end
%else
	cmp byte [di],0dh
%endif
	jz fert
	cmp byte [di],TABCHAR
	jz isTab
	inc dx				;count columns
	jmp short lod
isTab:	call SpacesForTab
	add dl,ah
	cmp dx,cx			;this tab to far away right?
	jna lod				;no
fert:	ret
;-------
KeyHome:call CountToLineBegin
	sub di,ax
	ret
;-------
KeyEnd:	call CountToLineEnd
	add di,ax			;points to a 0ah char
	ret
;-------
KeyIns:	not byte [insstat]
	ret
;-------
KeyDell:call KeyLeft
	jz KeyDell2
KeyDel:	cmp di,bp
	jnb KeyLeftEnd
	mov ax,1			;delete one @ cursor
%ifndef ELKS
	cmp byte [di],0dh
	jnz KeyDell3
	inc ax
%endif
KeyDell3:jmp DeleteByte
KeyDell2:call CheckBOF			;cmp di,sot  	delete newline char
	jbe KeyLeftEnd
	dec word [linenr]
	dec di
%ifndef ELKS
	dec di
%endif
	jmp BisNeueZeile
;-------
KeyLeft:cmp byte [di-1],NEWLINE		;FIXME another check of BOF
	jz KeyLeftEnd			;jmp if at BOL
	dec di
KeyLeftEnd:ret
;-------
KeyRight:
%ifdef ELKS
	cmp byte [di],NEWLINE
%else
	cmp byte [di],0dh
%endif
	jz KeyRightEnd			;at right margin
	inc di
KeyRightEnd:ret
;-------
KeyCLeft3:call CheckBOF			;cmp di,sot  bzw sot-1
	jbe KeyCLEnd
	dec di
%ifndef ELKS
	dec di
%endif
KeyCtrlLeft:call KeyLeft
	jz KeyCLeft3
	cmp byte [di],2fh
	jbe KeyCtrlLeft
	cmp byte [di-1],2fh
	ja KeyCtrlLeft
KeyCLEnd:ret
;-------
KeyCRight3:call CheckEOF
	jae KeyCREnd
	inc di
KeyCtrlRight:call KeyRight
	jz KeyCRight3
	cmp byte [di],2fh
	jbe KeyCtrlRight
	cmp byte [di-1],2fh
	ja KeyCtrlRight
KeyCREnd:ret
;
; processing special keys from the Ctrl-Q menu
;
;-------
KeyCtrlQA:call AskForReplace
	jc CtrlQFEnd
	mov byte [bereitsges],2
CQACtrlL:push di
	call FindText
	jc CtrlQFNotFound
	mov ax,[suchlaenge]
	call DeleteByte
	mov ax,[repllaenge]
	call InsertByte
	mov si, replacetext
	call MoveBlock
	jmp short CQFFound
;-------
KeyCtrlQF:call AskForFind
	jc CtrlQFEnd
	mov byte [bereitsges],1
CQFCtrlL:push di
	call FindText
	jc CtrlQFNotFound
CQFFound:pop si				;dummy
CQFNum:	mov byte [numeriere],1
	ret
CtrlQFNotFound:pop di
CtrlQFEnd:ret
;-------
KeyCtrlQC:mov di,bp
	jmp short CQFNum
;-------
KeyCtrlQR:mov di,sot
	jmp short CQFNum
;-------
KeyCtrlQP:mov di,[veryold]
	jmp short CQFNum	
;-------
KeyCtrlL:mov al,[bereitsges]		;2^QA   1^QF   0else
	dec al
	jz CQFCtrlL
	dec al
	jz CQACtrlL
SimpleRet4:ret
;-------
KeyCtrlQB:mov ax,di
	mov di,[blockbegin]
CtrlQB2:or di,di			;exit of no marker set
	jnz CQFNum
	xchg di,ax			;old mov di,ax
	ret
;-------
KeyCtrlQK:xchg ax,di			;old mov ax,di
	mov di,[blockende]
	jmp short CtrlQB2
;-------
KeyCtrlQY:call CountToLineEnd
	jmp short CtrlTEnd1
;-------
KeyCtrlY:call CountToLineBegin
	sub di,ax        		;di at begin
	call CountToLineEnd
	call DeleteByteCheckMarker
	jmp short BisNeueZeile
;-------
KeyCtrlT:call CountToWordBegin
%ifdef ELKS
	cmp byte [di],NEWLINE
%else
	cmp byte [di],0dh
%endif
	jnz CtrlTEnd1
BisNeueZeile:call CheckEOF
	jz SimpleRet4
%ifdef ELKS
	mov ax,1			;0ah
%else
	mov ax,2			;0dh,0ah	
%endif
CtrlTEnd1:jmp DeleteByteCheckMarker
;-------
KeyCtrlQI:mov dx,asklineno
	call GetAsciiToInteger
	jbe CtrlQFEnd			;CY or ZR set
	mov di,sot
	call LookPD2
	jmp CheckENum
;----------------------------------------------------------------------
;
; processing special Keys from Ctrl-K menu
;
KeyCtrlKY:call CheckBlock
	jc SimpleRet3			;no block: no action
	mov ax,[blockende]
	mov di,[blockbegin]
	sub ax,si			;block length
	mov di,si			;begin
	call DeleteByte			;out cx:=0
	mov [blockende],cx
	mov [blockbegin],cx
	jmp CQFNum
;-------
KeyCtrlKH:xor byte [showblock],1 	;flip flop
SimpleRet3:ret
;-------
KeyCtrlKK:mov [blockende],di
	jmp short KCKB
;-------
KeyCtrlKW:call CheckBlock
	jc SimpleRet2   ;no action
	call SaveBlock
	jmp short CtrlKREnd
;-------
KeyCtrlKC:call CopyBlock
	jc SimpleRet2
CtrlKC2:mov [blockbegin],di
	add ax,di
	mov [blockende],ax
	ret
;-------
KeyCtrlKV:call CopyBlock
	jc SimpleRet2
	push di
	cmp di,[blockbegin]
	pushf
	mov di,[blockbegin]
	call DeleteByte
	popf
	pop di
	jb CtrlKC2
	mov [blockende],di
	sub di,ax
KeyCtrlKB:mov [blockbegin],di
KCKB:	mov byte [showblock],1
SimpleRet2:ret
;-------
KeyCtrlKR:call ReadBlock
	jc CtrlKREnd
	call KeyCtrlKB
	add cx,di
	mov [blockende],cx
CtrlKREnd:jmp RestKursPos
;-------
KeyCtrlKS:call SaveFile
	pushf				;(called by ^kd)
	call RestKursPos
	popf
	jc CtrlKSEnd
	mov byte [changed],UNCHANGED
CtrlKSEnd:ret
;-------
KeyCtrlKQ:cmp byte [changed],UNCHANGED
	jz CtrlKQ2
	mov dx, asksave
	call DE1
	call RestKursPos
	and al,0dfh
	cmp al,'N'			;confirm
	jnz KeyCtrlKX
CtrlKQ2:mov byte [endedit],1
	ret 
KeyCtrlKD:call KeyCtrlKS
	jc CtrlKSEnd
	mov byte [endedit],2
	ret
KeyCtrlKX:call KeyCtrlKS
	jc CtrlKSEnd
	inc byte [endedit]
KeyKXend:ret
;---------------------------------------------------------------------
;
; the general PAGE DISPLAY function: called after any pressed key
;
; side effect: sets 'columne' for RestoreStatusLine function (displays columne)
; variable kurspos: for placing the cursor at new position
; register bh counts lines
; register bl counts columns visible on screen (w/o left scrolled)
; register dx counts columns in text lines
; register cx screen line counter and helper for rep stos
; register si text index
; register di screen line buffer index
;
DispNewScreen:call GetEditScreenSize	;check changed tty size
	xor ax,ax
%ifdef ELKS
	mov byte[isbold],al
	mov byte[inverse],al
%endif
	mov [zloffset],ax
	mov [columne],ax
	mov [fileptr],di		;for seeking current cursor pos
	call CountColToLineBeginVis	;i.e. expanding TABs
	cmp ax,[columns]
	jb DispShortLine
	sub ax,[columns]
	inc ax
	mov [zloffset],ax
DispShortLine:call LookPgBegin 		;go on 1st char upper left on screen
	mov si,di			;si for reading chars from text
	mov cx,[lines]
	jcxz KeyKXend			;window appears too small
	cld
%ifndef ELKS
	dec si
%endif
	mov bh,0
	dec bh
DispNewLine:
%ifndef ELKS
	inc si
%endif
	inc bh				;new line
	mov di,screenline		;line display buffer
	xor dx,dx			;reset char counter
	mov bl,0			;reset screen column
%ifdef LESSWRITEOPS
	cmp cx,1			;not in status line
	jz DCL0
	call SetColor2			;set initial character color per each line
DCL0:
%endif
DispCharLoop:
	cmp si,[fileptr]		;display char @ cursor postion ?
	jnz DispCharL1
	cmp byte[tabcnt],0
	jnz DispCharL1
	mov [kurspos],bx
	mov byte [columne],bl
	mov ax,[zloffset]		;chars scrolled left hidden
	add [columne],ax
%ifdef ELKS
	stc
	call SetInverseStatus
	jnc DispEndLine
%else
	mov ah,kursorfarbe
	cmp byte [insstat],1
	jz DispEndLine
%endif
DispCharL1:call SetColor
;-------
DispEndLine:cmp si,bp
	ja FillLine			;we have passed EOF, so now fill rest of screen
	cmp byte[tabcnt],0
	jz ELZ
	dec byte[tabcnt]
	jmp short ELZ2
ELZ:	cmp si,bp
	jnz ELZ6
	inc si				;set si>bp will later trigger  "ja FillLine"
	jmp short ELZ2	
ELZ6:	lodsb
	cmp al,TABCHAR
	jnz ELZ3
	push ax				;preserve color attribute
	call SpacesForTab		;ah = space_up_to_next_tab location
	dec ah				;count out the tab char itself
	mov byte[tabcnt],ah
	pop ax
ELZ2:	mov al,SPACECHAR
ELZ3:	
%ifdef ELKS
	cmp al,NEWLINE
%else
	cmp al,0dh
%endif
	jz FillLine
	cmp al,SPACECHAR
	jae ELZ9			;simply ignore chars like carriage_return etc.
	mov al,'.'
ELZ9:	cmp al,7fh
	jne ELZ8
	mov al,'.'
ELZ8:	cmp bl,byte [columns]		;screen width
	jae DispEndLine			;continue reading line until end
	inc dx				;also count hidden chars (left margin)
KZA6:	cmp dx,[zloffset]
	jbe DispCharLoop		;load new char (but no display)
%ifdef ELKS
	stosB
	clc
	call SetInverseStatus
%else
	stosw
%endif
	inc bl				;counts displayed chars only
	jmp DispCharLoop
;-------
FillLine:push cx			;continue rest of line
	mov cx,[columns]		;width
	sub cl,bl
	mov al,SPACECHAR		;fill with blanks
	jcxz FillLine2
%ifdef ELKS
	cmp byte[inverse],1		;special cursor attribute?
%else
	cmp ah,kursorfarbe
%endif
	jnz FillLine1
%ifdef ELKS
	mov al,SPACECHAR
	stosB				;only 1st char with special attribute
	clc
	call SetInverseStatus
	dec cx				;one char less
	jz FillLine2
FillLine1:rep stosB			;store the rest blanks
%else
	stosw
	dec cx
	jz FillLine2
	mov ah,normfarbe
FillLine1:rep stosw
%endif
FillLine2:pop cx
%ifdef ELKS
	mov byte[di],0
%endif
	call ScreenLineShow
	dec cx
	jz XX1
	jmp DispNewLine
XX1:	call RestKursPos
	mov di,[fileptr]		;restore old value
	ret
;----------------------------------------------------------------------
InitVars:mov word [textX],0a0ah		;don't touch si!
	mov byte [changed],UNCHANGED
	xor ax,ax
	mov byte[bereitsges],al
	mov [blockbegin],ax
	mov [blockende],ax
	mov [endedit],al
	mov word[old], sot
	inc ax
	mov word [linenr],ax
	mov byte [showblock],al
	mov byte [insstat],al
	mov word [error],'ER'
	mov word [error+2],'RO'
	mov word [error+4],'R '
	mov word [error+6],'  '
%ifdef ELKS
	mov word [perms],PERMS
%endif
	cld
	ret
;----------------------------------------------------------------------
;
; STATUS LINE maintaining subroutines
; at first the writer of a complete line
;
RestoreStatusLine:			;important e.g. for asksave
	push ax
	push bx
	push cx
	push dx
	push si
	push di
	push bp
%ifdef ELKS
	mov cx,[columns]		;width
	push cx
	mov al,SPACECHAR		;first prepare the line buffer....
	mov di,screenline
	cld
	rep stosb
	mov al,0			;prepare ASCIIZ string
	stosb
	pop cx
	cmp cl,stdtxtlen+15+5+2		;this window is too small
	jb no_lineNr
	mov bl, byte [changed]
	mov byte[screenline+1],bl	;changed status
	mov bx,'IN'			;Insert
	cmp byte [insstat],1
	jz rrr1
	mov bx,'OV'			;Overwrite
rrr1:	mov [screenline+4],bx		;mode status
	mov di,screenline+stdtxtlen
	mov cx,[columns]
	sub cx,stdtxtlen+15+5		;space for other than filename
	mov si,filepath
rrr2:	lodsb
	or al,al
	jz raus
okay:	stosb
	loop rrr2
	jmp short wett
raus:	mov al,SPACECHAR
	stosb
	loop raus
wett:	mov di,screenline-15
	add di,[columns]
	js no_lineNr
	mov ax,[columne]
	inc ax				;start with 1
	call IntegerToAscii
	mov byte [di],':'		;delimiter ROW:COL
	dec di
	mov ax,[linenr]
	call IntegerToAscii
%else	;----------------------------------------------
	mov di,zeilenangabe		;make string
	mov cx,12
	mov al,SPACECHAR
	cld
	rep stosb
	mov di,zeilenangabe+8
	mov ax,[columne]
	inc ax				;start with 1
	call IntegerToAscii
	mov byte [di],':'		;delimiter ROW:COL
	dec di
	mov ax,[linenr]
	call IntegerToAscii
	cld
;-------
	mov cx,[columns]
	mov ah,slinefarbe
	mov al,SPACECHAR
	mov di,screenline
	cld
	rep stosw
	mov bl, byte [changed]
	mov byte[screenline+2],bl
	mov di,screenline+20
	mov cx,55
	mov si,filepath
rrr2:   lodsb
	stosw
	loop rrr2
	mov cx,10
	mov si,zeilenangabe
rrr3:   lodsb
	stosw
	loop rrr3
%endif
no_lineNr:call StatusLineShow		;now write all at once
	pop bp
	pop di
	pop si
	pop dx
	pop cx
	pop bx
	pop ax
	ret
;-------------------------------------------------------------------------
; this function does write the line buffer to screen i.e. terminal
; at begin a special entry point for writing the STATUS line below
;
StatusLineShow:xor cx,cx		;0 for last line
ScreenLineShow:				;expecting in cx screen line counted from 0
	push ax
	push bx
	push cx
	push dx
	push si
	push di
	push bp
%ifdef ELKS

%ifdef LESSWRITEOPS
	xor bx,bx			;flag
	mov ax,[columns]
	add ax,32			;estimated max ESC sequences extra bytes (i.e. boldlen*X)
	mul cl
	mov di,screenbuffer
	add di,ax
	cld
;	xor dx,dx			;counter
	mov si,screenline
sl_3:	lodsb
;	inc dx				;count message length to write
	cmp di,screenbuffer_end		;never read/write beyond buffer
	jnb sl5
	cmp al,[di]
	jz sl4
	mov [di],al
sl5:	inc bx				;set flag whether line need redrawing
sl4:	inc di
	or al,al
	jnz sl_3
	or bx,bx			;redraw ?
	jz NoWrite
%endif
	mov dh,byte [lines]
	sub dh,cl
	mov dl,0
	call sys_writeKP
	call sys_writeSL
	mov dx,[kurspos2]
	call sys_writeKP		;restore cursor pos
%else
%ifdef DO_NOT_USE_AX_1302		;very slow (but running on some very old 1980's PC)
	mov si,screenline
	cld
	mov dh,[lines]
	sub dh,cl
	mov dl,0
no1302:	call sys_writeKP
	lodsw
	mov bl,ah
	mov bh,0
	mov ah,09h
	mov cx,1
	int 10h
	mov al,[columns]
	dec al
	cmp dl,al
	inc dl
	jb no1302
	mov dx,[kurspos2]
	call sys_writeKP		;restore cursor pos
%else
	mov ax,1302h
	mov bx,0
	mov dh,[lines]
	sub dh,cl
	mov dl,0
	mov cx,[columns]
	mov bp,screenline
	int 10h
%endif
%endif
NoWrite:
	pop bp
	pop di
	pop si
	pop dx
	pop cx
	pop bx
	pop ax
	ret
;-----------------------------------------------------------------------
; write an answer prompt into status line
; (with and without re-initialisation)
; expecting dx points to ASCIIZ-string
;
WriteMess9MachRand:
	call InitStatusLine
WriteMess9:
%ifdef ELKS
	push ax
	push bx
	push cx
	push dx
	push si
	push di
	push bp
	mov di,screenline
	mov si,dx
	cld
WriteMLoop:lodsb
	or al,al
	jz WriteMEnd
	cmp al,0ah			;for error messages
	jz WriteMEnd
	stosb
	jmp short WriteMLoop
WriteMEnd:call StatusLineShow
	pop bp
	pop di
	pop si
	pop dx
	pop cx
	pop bx
	pop ax
%else	;---------------------
	push si
	push di
	mov si,dx
	cld
	mov di,screenline
	mov ah,slinefarbe
WriteMLoop:lodsb
	or al,al
	jz WriteMEnd
	cmp al,0dh
	jz WriteMEnd
	stosw 
	jmp short WriteMLoop
WriteMEnd:call StatusLineShow
	pop di
	pop si
%endif
	call KursorStatusLine
	ret
;-------
; another way: write 2 letters in ch/cl to status line
; called by MakeScanCode for showing ^K and ^Q status (lower left)
;
WriteTwo:push di
%ifdef ELKS
	mov word[screenline],cx
%else
	mov di,screenline
	mov al,cl
	mov ah,slinefarbe
	cld
	stosw
	mov al,ch
	stosw
%endif
	call StatusLineShow		;write the line on last screen line
	pop di
	ret
;--------------------------------------------------------------------
; a helper for other status line functions:
; simply init an empty line  
;
InitStatusLine:push di
	push ax
	push cx
	mov di,screenline
	cld
	mov al,SPACECHAR
	mov cx,[columns]
%ifdef ELKS
	cld
	rep stosb
%else
	mov ah,slinefarbe
	rep stosw
%endif
	pop cx
	pop ax
	pop di
	ret
;-----------------------------------------------------------------------
;
; getting INPUT from terminal
; at first read a whole string until <enter> pressed,
; follwed by handling reading one char alone
;
%ifdef ELKS
; expecting buffer in cx
; expecting count byte in dx
InputString:call sys_writeSLColors1			
	push cx
	mov bx,stdin			;file desc
	call ReadFile
	pop cx		
	stc
	js ISRet
	dec ax				;0ah
	push bx
	mov bx,ax
	add bx,cx
	mov byte[bx],0			;make asciz string
	pop bx
	cmp ax,1			;set cy flag if empty string
ISRet:	pushf
	call sys_writeSLColors0		;FIXME should flush stdin: read until empty buffer
	popf		
	ret
%else	;----------------------------------
InputString:mov dx,cx			;ELKS register style
	xor cx,cx			;char counter 
	push di
	mov di,dx
	cld
GetNameLoop:call GetChar
	cmp ah,4bh			;left
	jz GetNameDelete
	cmp ah,4dh
	jz GetNameOldChar
	or al,al
	jz GetNameLoop
	cmp al,7
	jz GetNameLoop			;no beep
	cmp al,0dh
	jz GetNameEnd
	cmp al,1bh
	stc
	jz GetNameErr
	cmp al,8
	jnz GetNameChar
GetNameDelete:mov al,8
	dec cx
	jS GetNameNoToDel
	call xDispChar
	dec di
	mov al,0
	call xDispChar
	mov al,8
	call xDispChar
	jmp short GetNameLoop
GetNameOldChar:mov al,[di]
	or al,al
	jz GetNameLoop
GetNameChar:stosb
	call xDispChar
GetNameNoToDel:inc cx
	cmp cl,maxfilenamelen
	jnc GetNameEnd
	jmp short GetNameLoop
GetNameEnd:mov byte [di],0
	mov ax,di
	sub ax,dx			;ret ax=lge
	cmp ax,1			;clc
GetNameErr:pop di
	ret
;-------
xDispChar:push bx			;char in al
	mov ah,0eh
	mov bx,0111b			;page bh,0
	int 10h
	pop bx
	ret
%endif
;-----------------------------------------------------------------------
%ifdef ELKS
;
; GetChar returns ZERO flag for non ASCII (checked in HandleChar)
; FIXME: e3-32 bit does the IOctlTerminal only at start/end of editor...
; ...but here we are calling IOctlTerminal each time (==useless overhead)
; (see diff e3 release 0.1 -> 0.2)
;
ReadChar:mov ax,di
	xchg ax,[old] ;fuer ^QP
	mov [veryold],ax
GetChar:mov cx, 0x5401			;TCGETS asm/ioctls.h
	mov dx,termios
	call IOctlTerminal
	call SaveTermStruc
	push bx
	mov bx,dx
	and byte [bx+12],(~2 & ~1 & ~8)	;icanon off, isig (^C) off, echo off
	and byte [bx+ 1],(~4)		;ixon off	was:and word [bx+ 0],(~400h)
	pop bx
	mov cx, 0x5402			;TCSETS asm/ioctls.h
	call IOctlTerminal		;dx is termios pointer
readloop:call ReadOneChar
	cmp al,7FH
	jne No7F			; special case: remap DEL to Ctrl-H
	mov al,8
No7F:	cmp al,27 			; ESC ?
	jnz ready_2
	call ReadOneChar		;e.g.  [ for ELKS vt100
	mov bl,48h			;48h up - the lowest
	cmp al,'A'
	jz ready
	add bl,3			;4Bh left
	cmp al,'D'		
	jz ready
	add bl,2			;4Dh right
	cmp al,'C'
	jz ready
	add bl,3			;50h down
	cmp al,'B'
	jz ready
	jmp short ready_2
;-------
ready:	xor ax,ax
	mov ah,bl
ready_2:push ax
	mov cx,0x5402			;TCSETS asm/ioctls.h
	mov dx,orig
	call IOctlTerminal		; restore termios settings
	pop ax
	or al,al			; was similar DOS version (via BIOS int 16h)
	ret
;-------
SaveTermStruc:push di
	mov si,termios
	mov di,orig
	mov cx,termios_size
	cld
	rep movsb
	pop di
	ret
;-------
; called by ReadChar/GetChar
;
ReadOneChar:mov bx,stdin		;file desc
	mov cx,read_b 			;pointer to buf
	xor dx,dx
	inc dx				;mov dx,1  (length)
	call ReadFile
	mov ax,[read_b]
	ret
%else   ;-------
ReadChar:mov ax,di
	xchg ax,[old]			;for ^QP
	mov [veryold],ax
	call GetChar
	or al,al
	ret
;-------
GetChar:mov ah,0
	int 16h
	ret
%endif
;----------------------------------------------------------------------
%ifdef ELKS
;
; helper subroutine called by DispNewScreen
;
SetInverseStatus:
	push si				; returns zero flag
	push cx
	mov cx,boldlen
	jnc SIS1
	cmp byte [insstat],1
	stc
	jnz SIS4
	mov byte[inverse],1
	mov si,reversevideoX
	rep movsb
	jmp short SIS3
SIS1:	cmp byte[inverse],1
	jnz SIS3
	mov byte[inverse],0
	mov byte[isbold],0
SIS_5:	mov si,bold0
	rep movsb
SIS3:	clc
SIS4:	pop cx
	pop si
	ret
%endif
;-------
; another helper subroutine called by DispNewScreen
;
SetColor:
%ifdef ELKS
	call IsShowBlock
	push si				;expects cy flag:bold /  nc:normal
	push cx
	mov cx,boldlen
	jnc SFEsc1
	cmp byte [isbold],1		;never set bold if it is already bold
	jz SFEsc2
SCEsc_2:mov si,bold1
	rep movsb
	mov byte [isbold],1
	jmp short SFEsc2
SFEsc1:	cmp byte [isbold],0		;ditto
	jz SFEsc2
	mov si,bold0
	rep movsb
	mov byte [isbold],0
SFEsc2:	pop cx
	pop si
	ret
%ifdef LESSWRITEOPS
SetColor2:push si
	push cx
	call IsShowBlock
	mov cx,boldlen
	jnc SIS_5
	jmp short SCEsc_2
%endif
%else	;---------------------------------
	mov ah,normfarbe
	cmp byte[showblock],0
	je SetColorEnde
	cmp word [blockbegin],0
	je SetColorEnde
	cmp [blockbegin],si
	ja SetColorEnde
	cmp si,[blockende]
	jnb SetColorEnde
	mov ah,blockfarbe
SetColorEnde:ret
%endif
;-----------------------------------------------------------------------
;
; LOWER LEVEL screen acces function (main +2 helpers)   (ELKS only)
;
%ifdef ELKS
sys_writeSL:push cx
	or cx,cx
	jnz sl1
	call sys_writeSLColors1		;special for status line (cx==0)
sl1:    push si
	cld
	xor dx,dx
	mov si,screenline
sl3:	lodsb
	inc dx				;count message length to write
	or al,al
	jnz sl3
	dec dx				;one too much
	pop si
	mov bx,stdout			;first argument: file desc (stdout)
	mov cx,screenline		;second argument: pointer to message to write
	call WriteFile
	pop cx
	or cx,cx
	jnz sl2
	call sys_writeSLColors0
sl2:	ret
;-------
sys_writeSLColors1:
	push ax
	push bx
	push cx
	push dx
	push si
	push di
	push bp
	mov cx,screencolors1		;set bold yellow on blue
	jmp short SwSL
;-------
sys_writeSLColors0:
	push ax
	push bx
	push cx
	push dx
	push si
	push di
	push bp
	mov cx,screencolors0		;reset to b/w
SwSL:	mov bx,stdout
	mov dx,scolorslen
	call WriteFile
	pop bp
	pop di
	pop si
	pop dx
	pop cx
	pop bx
	pop ax
	ret	       
%endif
;----------------------------------------------------------------------
;
; L O O K functions
; search special text locations and set register di
;
%ifdef ELKS
LookBackward:				;set di to 1 before EOL (0Ah) i.e., 2 before start of next line
	push cx	
	push bx
	xor bx,bx
	cmp byte[di-1],NEWLINE		;at BOL ?
	jz LBa3
	cmp byte[di],NEWLINE		;at EOL ?
	jnz LBa1
	dec di				;at EOL ? start search 1 char earlier
	inc bx				;increase counter
LBa1:	mov cx,9999
	mov al,NEWLINE
	std
	repne scasb
	mov ax,9997
	sub ax,cx
	add ax,bx
	pop bx
	pop cx
	jmp short CheckBOF
;-------
LBa3:	xor ax,ax
	pop bx
	pop cx
	dec di
	dec di
	jmp short CheckBOF
%else
LookBackward:push cx
	mov cx,9999
	mov al,0ah
	std
	repne scasb
	mov ax,9997
	sub ax,cx
	pop cx
	jmp short CheckBOF
%endif
LookForward:push cx
	mov cx,9999
%ifdef ELKS
	mov al,NEWLINE
%else
	mov al,0dh
%endif
	cld
	repne scasb
	mov ax,9998
	sub ax,cx
	pop cx
	dec di
CheckEOF:cmp di,bp			;ptr is eof-ptr?      
	jnz CheckEnd			;Z flag if eof             
	jmp short CheckENum
CheckBOF:cmp di,sot
	ja CheckEnd
CheckENum:mov byte [numeriere],1	;if bof
CheckEnd:ret
;-------
LookPgBegin:mov dx,[kurspos2]		;called by DispNewScreen to get sync with 1st char on screen
	mov cl,dh			;called by KeyCtrlQE  (go upper left)
	mov ch,0
	inc cl
	jmp short LookPU2
;-------
LookLineUp:mov cx,2			;2 lines: THIS line and line BEFORE
	dec word [linenr]
	jmp short LookPU2
;-------
LookLineDown:mov cx,2			;2 lines: THIS and NEXT line
	inc word [linenr]
	jmp short LookPD2
;-------
LookPageUp:mov cx,[lines]
	sub [linenr],cx 
	inc word [linenr]		;PgUp,PgDown one line less
LookPU2:call LookBackward
	jb LookPUEnd			;if BOF
%ifdef ELKS
	inc di
%endif
	loop LookPU2			;after loop di points to char left of 0ah
%ifdef ELKS
	dec di
%endif
LookPUEnd:inc di   
	inc di				;now points to 1st char on screen or line
	ret
;-------
LookPgDown:mov cx,[lines]
	add [linenr],cx
	dec word [linenr]
LookPD2:call LookForward
	jz LookPDEnd			;(jmp if EOF)
%ifndef ELKS
	inc di
%endif
	inc di				;1st char next line
	loop LookPD2
%ifndef ELKS
	dec di
%endif
	dec di				;last char last line
LookPDEnd:sub di,ax			;1st char last line
	ret
;----------------------------------------------------------------------
;
; some more CHECK functions
;
CheckBlock:cmp byte [showblock],1	;returns CY if error else ok: NC
	jc CheckBlockEnd
	mov si,[blockende]
	cmp si, sot
	jb CheckBlockEnd
	mov si,[blockbegin]		;side effect si points to block begin
	cmp si, sot
	jb CheckBlockEnd
	cmp [blockende],si		;^KK > ^KB ..OK if above!
CheckBlockEnd:ret
;-------
CheckImBlock:cmp [blockbegin],di	;^KB mark > di ?
	ja CImBlockEnd			;OK
	cmp di,[blockende]		;di > ^KK
CImBlockEnd:ret	          		;output:cy fehler / nc ok inside block
;-------
CheckMode:
%ifdef ELKS
	cmp byte [di],NEWLINE		;checks for INSERT status
%else
	cmp byte [di],0dh
%endif
	jz ChModeEnd
	cmp byte [insstat],1
ChModeEnd:ret				;Z flag for ins-mode
;-------
; a special case called by DeleteByteCheckMarker
;
CheckMarker:				;dx is blockbegin (^KB) 
					;bx is deleate area end --- di delete area start
	cmp di,dx			;delete area start < ^KB marker ?
	ja CMEnd			;no
	cmp bx,dx			;yes, but delete area end > ^KB ?
	jl CMEnd			;no
	mov dx,di			;yes so block start (^KB) to delete area start
CMEnd:	ret
;----------------------------------------------------------------------
;
; C O U N T  functions
; to return number of chars up to some place
; (all of them are wrappers of Look....functions anyway)
;
CountToLineEnd:push di
	call LookForward
	pop di
	ret				;ax=chars up to line end
;-------
CountColToLineBeginVis:			;counts columns represented by chars in ax
	call CountToLineBegin		;i.e. EXPAND any TAB chars found
	push si
	xor dx,dx
	mov si,di			;startpoint	
	sub si,ax			;to bol
	dec si
CCV1:	inc si
	cmp si,di
	jae CCVend
	cmp byte [si],TABCHAR
	jz CCVTab
	inc dx				;count visible chars
	jmp short CCV1
CCVTab:	call SpacesForTab		;return space_up_to_next_tab in ah
	add dl,ah			;FIXME: now using 8 bits only
	jmp short CCV1
CCVend:  mov [ch2linebeg],dx		;ch2linebeg: interface to Key... functions
	mov ax,dx			;ax: interface to DispNewScreen
	pop si
	ret
;-------
CountToLineBegin:push di		;output ax=chars up there
	call LookBackward
	mov si,di			;side effect: set di to 1st char in line
	pop di
	ret
;-------
CountToWordBegin:			;output ax=chars up there
	mov si,di
CountNLoop:inc si
%ifdef ELKS
	cmp byte [si],NEWLINE
%else
	cmp byte [si],0dh
%endif
	jz fertig2
	cmp byte [si],SPACECHAR		;below SPACE includes tab chars
	jbe CountNLoop
	cmp byte [si-1],2fh
	ja CountNLoop 
fertig2:mov ax,si
	sub ax,di			;maybe =0
	ret
;---------------------------------------------------------------------
;
; some CURSOR control functions
;
GoUp:	mov al,0
	mov ah,-1
	jmp short UpDown
GoDown:	mov al,byte [lines]
	dec al
	mov ah,1
UpDown:	mov dx,[kurspos2]		;former was call getkurspos
	cmp dh,al
	jz Goret
	add dh,ah			;ONLY here we change curent line of cursor
	jmp short SetKursPos 
Goret:	ret       
;-------
; set cursor to some desired places
;
KursorStatusLine:mov dh,[lines]
	mov dl,stdtxtlen
	jmp short SetKursPos
RestKursPos:mov dx,[kurspos]
SetKursPos:mov [kurspos2],dx		;saves reading cursor pos   (0,0)
%ifndef ELKS
sys_writeKP:
	push ax
	mov ah,2
	mov bh,0
	int 10h
	pop ax
	ret
%else	;---------------------------------------------------------------
sys_writeKP:
	push ax
	push bx
	push cx
	push dx
	push si
	push di
	push bp
	call make_KPstr
	mov bx,stdout			;file handle (stdout)
	mov cx,setkp 			;second argument: pointer to message to write
	mov dx,setkplen			;third argument: message length
	call WriteFile
	pop bp
	pop di
	pop si
	pop dx
	pop cx
	pop bx
	pop ax
	ret
;-------
; make ESC sequence appropriate to most important terminals
;
make_KPstr:
	inc dl				;expecting cursor pos in dh/dl (0,0)
	inc dh 				;both line (dh) col (dl) are counted now from 1
	cld
	mov di,setkp			;build cursor control esc string db 27,'[000;000H'
	mov ax,0x5B1B			;line starts at setkp+2, col starts at setkp+6
	stosw
	mov ax,'00'
	stosw
	mov ax,'0;'			;init memory
	stosw
	mov ax,'00'
	stosw
	mov ax,'0H'			;init memory
	stosw
	mov di,setkp+1+3		;line end
	xor ax,ax
	mov al,dh			;DH=line
	push dx
	call IntegerToAscii		;make number string
	pop dx
	cld
	mov di,setkp+1+3+4		;column end
	xor ax,ax
	mov al,dl			;DL=col
	jmp IntegerToAscii	
%endif
;-----------------------------------------------------------------------
;
; functions for INSERTING, COPYING and DELETING chars in text
;
InsertByte:or ax,ax			;input: ax = #bytes ,  di = ptr
	jz Ins3
	mov byte [changed],CHANGED
	mov cx,MAXLEN			;max_len+offset-eofptr=freespace(cx)
	add cx,sot
	sub cx,bp
	cmp cx,ax			;cmp freespace - newbytes  ;>= 0 ok/ NC  <0 bad / CY
	jnc SpaceAva
	mov word[errno],ERRNOMEM
	call DosError
	call RestKursPos
	stc
	ret
SpaceAva:push di
	mov si,bp			;end of text
	mov cx,bp
	add cx,ax
	sub cx,di			;space count
	mov di,bp
	add di,ax
	std
	rep movsB
	pop di
;-------
	add bp,ax
	cmp di,[blockende]
	ja Ins1
	add [blockende],ax
Ins1:	cmp di,[blockbegin]
	ja Ins2
        add [blockbegin],ax
Ins2:	clc
Ins3:	ret				;output:nc=ok / cy=bad
;-------
CopyBlock:call CheckBlock		;copy block, called by ^KC, ^KV
	jc MoveBlEnd
	call CheckImBlock
	jc MoveBlEnd
	mov ax,[blockende]
	sub ax,si			;block len
	call InsertByte
	jc MoveBlEnd
	mov si,[blockbegin]
MoveBlock:push di			;input : si=^KB di=current
	mov cx,ax
	cld
	rep movsb
	pop di
	clc				;nocarry->ok
MoveBlEnd:ret
;-------
DeleteByteCheckMarker:			;di points to begin
	mov bx,di
	add bx,ax
	mov dx,[blockbegin]
	call CheckMarker
	mov [blockbegin],dx
	mov dx,[blockende]
	call CheckMarker
	mov [blockende],dx
DeleteByte:or ax,ax			;input in ax
	jz DeleteByteEnd
	mov byte [changed],CHANGED
	push di
	push si
	mov cx,bp			;end
	sub cx,di
	mov si,di
	add si,ax
	sub cx,ax
	add cx,3
	shr cx,1
	cld
	rep movsW
	pop si
	pop di
	sub bp,ax
	cmp di,[blockende]
	jae Del1
	sub [blockende],ax
Del1:	cmp di,[blockbegin]
	jae DeleteByteEnd
	sub [blockbegin],ax
DeleteByteEnd:ret
;---------------------------------------------------------------------
; read a file name for block operations
; expecting message text ptr in dx
;
getBlockName:
	push ax
	push bx
	push cx
	push dx
	push si
	push di
	push bp
	call WriteMess9MachRand
	mov cx,blockpath
	mov dx,maxfilenamelen
	call InputString		;cy if empty string
	pushf
	call RestKursPos
	popf
	pop bp
	pop di
	pop si
	pop dx
	pop cx
	pop bx
	pop ax
	ret
;----------------------------------------------------------------------
;
; functions reading/writing  text or blocks  from/into  files
;
NewFile:call InitVars
	call DelEditScreen
	or si,si
	jz noarg
	cld
	mov di,filepath
ccc:	lodsb
	stosb
	or al,al
	jnz ccc
	jmp short GetFile
;-------
noarg:	mov dx, filename
	call WriteMess9MachRand
	mov cx,filepath
	mov dx,maxfilenamelen
	call InputString
	jc NFEnd2			;empty string not allowed here
;-------
GetFile:mov bx,filepath
	xor cx,cx			;i.e O_RDONLY
	call OpenFile
	mov di,sot
	mov bp,di
	mov bx,ax			;file descriptor
	js NewFileEnd	
OldFile:
%ifdef SYS_FSTAT
	call Fstat			;kernel returns error 38
	js DosEjmp0
	mov ax,[fstatbuf+8]		;better define some structure
	and ax,777q
	mov [perms],ax
%endif
;-------
	mov dx,MAXLEN
	mov cx,di			;sot
	call ReadFile
	mov dx,ax			;bytes read
	js DosEjmp0			;DosError
	call CloseFile
	js DosEjmp0			;DosError
;-------
	mov word [errno],ERRNOMEM
	cmp dx,MAXLEN			;MAXLEN read amount is too much
	jnz XX2
	jmp DosError
XX2:
;-------
	mov bp,sot			;eof_ptr=filesize+start_of_text
	add bp,dx
NewFileEnd:
%ifdef ELKS
	mov byte [ds:bp],NEWLINE	;eof-marker
%else
	mov word [ds:bp],0a0dh
%endif
	clc
NFEnd2:	ret
;-------
;  save file (called by ^KS,^KX)
;
SaveFile:cmp byte [changed],UNCHANGED
	jz SaveFile3			;no changes: nothing to save
	mov dx,filesave
	call WriteMess9
	mov cx,O_WRONLY_CREAT_TRUNC
	mov bx,filepath
%ifdef ELKS
	mov dx,[perms]
	call OpenFile
%else
	call CreateFile
%endif
DosEjmp0:js DosEjmp			;DosError
	mov cx,sot			;cx=bof
	mov dx,bp			;eof
SaveFile2:sub dx,cx			;dx=filesize= eof-bof
	mov bx,ax			;file descriptor
	call WriteFile
	js DosEjmp			;DosError
	mov word[errno],ERRNOIO		;just in case of....
	cmp ax,dx			;all written?
	jz XX4
	jmp DosError
XX4:	call CloseFile
	js DosEjmp			;DosError
SaveFile3:ret
;------------------------------
;  save block (called by ^KW)
;
SaveBlock:mov dx,blocksave
	call getBlockName
	jnc XX3
	jmp DE2
XX3:	mov cx,O_WRONLY_CREAT_TRUNC
	mov bx,blockpath
%ifdef ELKS
	mov dx,[perms]
	call OpenFile
%else
	call CreateFile
%endif
	js DosEjmp			;DosError
	mov cx,si			;= block begin
	mov dx,[blockende]
	jmp short SaveFile2
;-------
; read a block into buffer (by ^KR)
;
ReadBlock:mov dx,blockread
	call getBlockName
	jnc XX5
	jmp DE2
XX5:	xor cx,cx			;i.e O_RDONLY
	mov bx,blockpath
	call OpenFile
DosEjmp:js DosError
	mov bx,ax			;file desc
	mov dx,2
	call SeekFile			;end
	js DosError
	push dx
	push ax
	xor dx,dx
	call SeekFile			;home
	pop ax
	pop dx
	js DosError
	or dx,dx
	mov word [errno],ERRNOMEM
	jnz DosError
	push ax
	call InsertByte
	pop dx				;file size
	jc DosError
	mov cx,di			;^offset akt ptr
	call ReadFile
	js preDosError			;to delete inserted bytes (# in dx)
	mov cx,ax    			;bytes read
	call CloseFile
	js DosError
	mov word[errno],ERRNOIO		;just in case of....
	cmp dx,cx			;all read?
	jnz DosError
ReadBlock2:jmp NewFileEnd
;------------------------------------------------------------
;
; Error handler
;
preDosError:mov ax,dx			;count bytes
	call DeleteByte			;delete space reserved for insertation
DosError:push di
	mov di,error+8			;where to store ASCII value of errno
	mov ax,[errno]
	push ax
	call IntegerToAscii		;TODO: print a string instead of errno value
	pop cx
	cmp cx,MAXERRNO
	ja DE0
	mov di,errmsgs
	call LookPD2			;look message x in line number x
	mov si,di
	mov di,error+9
	mov ax,' :'
	stosw
	mov cx,80			;max strlen / compare errlen equ 100
	rep movsb
DE0:	mov dx,error
	pop di
DE1:	call WriteMess9
	call GetChar
DE2:	call RestoreStatusLine
	stc				;error status
	ret
;----------------------------------------------------------------------
;
; some GENERAL helper functions
;
IntegerToAscii:
	mov cx,10
	std
	mov bx,ax			;bx=quotient
Connum1:mov ax,bx
	sub dx,dx
	div cx
	mov bx,ax			;save quotient (new low word)
	mov al,dl
	call Hexnibble
	or bx,bx
	jne Connum1
	cld
	ret
Hexnibble:and al,0fh
	add al,'0'	
	cmp al,':'
	jb noHex
	add al,7			;(should never be due cx==10)
noHex:	stosb
	ret
;-------
;
; GetAsciiToInteger reads integer value from keyboard (only > 0)
;
GetAsciiToInteger:call WriteMess9MachRand
	mov cx,blockpath
	mov dx,maxfilenamelen
	call InputString
	call AskFor_Ex			;repair status line & set cursor pos / preserve flags
	mov ax,0			;preserve flags
	mov cx,ax
	mov bl,9			;bl == base-1
	jc AIexit2
	mov si,blockpath
	cld
AIload:	lodsb
	sub al,'0'
	js AIexit
	cmp al,bl
	ja AIexit
	mov dx,cx	
	shl cx,3			;* 8
	add cx,dx			;+ 1
	add cx,dx			;+ 1
	add cx,ax			;+digit
	jmp short AIload
AIexit:	or cx,cx			;ret ecx
AIexit2:ret				;CY or ZR if error
;-------
;
; expects curent column in dx
; returns # spaces up to next tabulated location in AH
;
SpacesForTab:push cx
	mov ax,dx
	mov cl,TAB
	div cl
	neg ah				;ah = modulo division
	add ah,TAB			;TAB - pos % TAB
	pop cx
	ret
;-------
; a little helper for SetColor* functions
;
IsShowBlock:cmp byte [showblock],0
	je SBlock
	cmp word [blockbegin],0
	je SBlock
	cmp [blockbegin],si
	ja SBlock
	cmp si,[blockende]
	jb SB_ret
SBlock:	clc
SB_ret:	ret
;-------
GetEditScreenSize:
	mov al,sHOEHE-1
	mov byte [lines],al
	mov al,sBREITE
	mov byte [columns],al		;columns > 255 are ignored...
	ret
;-------
DelEditScreen:push si
	push bp
	mov di,help
	mov bp,di			;end
	add bp,help_ws_size
	call DispNewScreen
	pop bp
	pop si
	ret
;-------
InitBSS:mov cx,EXE_absssize		;init bss
	mov di,EXE_startbss
	cld 
	xor ax,ax
	rep stosb
	mov word [es:textX],0a0ah	;es: due EXESTUB version
	ret
;-------
%ifndef ELKS
GetArg:	mov si,80h			;point to params
	mov cl,[si]			;get number of chars
	xor ch,ch			;make it a word
	inc si				;point to first char
	add si,cx			;point to just after last char
	mov byte [si],0			;make into an ASCIIZ string
	sub si,cx			;get back ptr to first char
	cld
	jcxz no_filename		;if no file name, then get one
	mov dx,cx
del_spaces:lodsb
	cmp al,SPACECHAR
	jne found_letter		;exit loop if al not space
	loop del_spaces
found_letter:dec si			;backup to first ascii char
	cmp byte [si],SPACECHAR
	jz no_filename
	ret
no_filename:xor si,si
	ret
%endif
;----------------------------------------------------------------------
;
; FIND/REPLACE related stuff
;
AskForReplace:mov dx, askreplace1
	call WriteMess9MachRand
	mov cx,suchtext
	mov dx,maxfilenamelen	
	call InputString
	jc AskFor_Ex
	mov [suchlaenge],ax
	mov dx,askreplace2
	call WriteMess9MachRand
	mov cx,replacetext
	mov dx,maxfilenamelen	
	call InputString
	mov [repllaenge],ax
	jc AskFor_Ex
	jmp short GetOptions
AskForFind:mov dx,askfind
	call WriteMess9MachRand
	mov cx,suchtext
	mov dx,maxfilenamelen
	call InputString
	mov [repllaenge],ax
	jc AskFor_Ex
GetOptions:mov dx,optiontext
	call WriteMess9MachRand
	mov cx,optbuffer
	mov dx,optslen
	call InputString		; empty string is allowd for std options...
	call ParseOptions		; ...(set in ParseOptions)
	clc
AskFor_Ex:pushf
	call RestoreStatusLine
	call RestKursPos
	popf
	ret
;-------
; check string for 2 possible options
; 
ParseOptions:push si
	cld
	mov si,optbuffer
	mov word[vorwarts],1
	mov byte[grossklein],0dfh
Scan1:	lodsb
	and al,5fh
	cmp al,'C'
	jnz notCopt
	xor byte[grossklein],20h	;result 0dfh,   2*C is 20h again -->not U option
notCopt:cmp al,'B'
	jnz notBopt
	neg word[vorwarts]		;similar 2*B is backward twice i.e. forward
notBopt:or al,al
	jnz Scan1
	pop si
	ret
;-------
; the find subroutine itself
;
find2:	mov bx,di
find3:	lodsb
	or al,al			;=end?
	jz found
	cmp al,41h
	jb find7
	and al,ch
find7:	inc di
	mov cl,byte [di]
	cmp cl,41h
	jb find10
	and cl,ch
find10:	cmp al,cl
	jz find3
	mov di,bx
FindText:mov dx,[vorwarts]		;+1 or -1
	mov ch,[grossklein]		;ff or df
	mov si,suchtext
	cld
	lodsb
	cmp al,41h
	jb find1
	and al,ch
find1:	add di,dx			;+1/-1
	mov cl,byte [di]
	cmp cl,41h
	jb find6
	and cl,ch
find6:	cmp al,cl
	je find2
	cmp di,bp
	ja notfound
	cmp di,sot
	jnb find1
notfound:stc
	ret
found:	mov di,bx
	clc				;di points after location
	ret
;----------------------------------------------------------------------
;
; INTERFACE to OS kernel
;
%ifdef ELKS
ReadFile:mov ax,3			;(3==sys_read) ;return read byte ax
	jmp short IntCall		;bx file / cx buffer / dx count byte
;-------
WriteFile:mov ax,4			;(4==sys_write)
	jmp short IntCall
;-------
OpenFile:mov ax,5
	jmp short IntCall		;cx mode / bx path / dx permissions (if create)
;-------
CloseFile:
	push bx
	push cx
	push dx
	push si
	push di
	push bp
	mov ax,6			;bx is file desc
	int 80h
	pop bp
	pop di
	pop si
	pop dx
	pop cx
	pop bx
	xor ax,ax			;always return "NO_ERROR" 
	ret
;-------
SeekFile:xor cx,cx			;offset
	mov word [seekhelp],cx		;due special ELKS kernel interface
	mov word [seekhelp+2],cx	;due special ELKS kernel interface
	mov cx,seekhelp			;2nd arg is a ptr to long, not a long
	mov ax,19			;system call number (lseek)
	push bx
	int 0x80			;bx file / dx method
	pop bx
	or ax,ax
	js IC2	
	mov ax,word [seekhelp]		;seek result: file size. 32 bit Linux is different here!
	mov dx,word [seekhelp+2]
	ret
;-------
IntCall:int 0x80			;bx file / dx method
IC2:	neg ax
	mov [errno],ax
	neg ax				;set flags also
	ret		
;-------
IOctlTerminal:mov bx,stdin		;expects dx termios or winsize structure ptr
	mov ax,54			;54 == the ioctl syscall no.
	int 80h				;cx TCSETS,TCGETS,TIOCGWINSZ
	ret
;-------
%ifdef SYS_FSTAT
Fstat:	mov cx,fstatbuf
	mov al,108			;currently  (April 2002) one gets
	jmp short IntCall		;error code  38:  Function not implemented
%endif
%else	;---------------
;
; ******beside int 21h we have also BIOS calls:
; **** 	mov ax,1302h	int 10h
; ****	mov ah,0eh	int 10h
; ****	mov ah,2	int 10h
; ****  mov ah,0	int 16h
; *****************************
;
OpenFile:xchg bx,dx			;elks register style
	mov ax,3d02h			;r/w  input bx=^path
Intcall:int 21h				;=ax file
	jnc NoErr
	mov [errno],ax
	mov ax,-1
NoErr:	test ax,ax			;set sign flag
	ret
CreateFile:xor dx,dx
	xchg bx,dx			;elks style
	xor cx,cx			;input bx=^path
	mov ah,3ch
	jmp short Intcall
ReadFile:mov ah,3fh
	jmp short WFile
WriteFile:mov ah,40h
WFile:	push dx
	xchg dx,cx
	int 21h	
	jnc NoErr2
	mov [errno],ax
	mov ax,-1
NoErr2:	test ax,ax			;set sign flag
	pop dx
	ret
CloseFile:mov ah,3eh			;path in bx
	jmp short Intcall
SeekFile:mov al,dl			;ELKS register style
	mov ah,42h
	xor dx,dx
	xor cx,cx
	jmp short Intcall
%endif
EXE_endcode:
;
;----------------------------------------------------------------------
;
section .data
bits 16
EXE_startdata:
;
; CONSTANT DATA AREA
;
Ktable	db 45h	;^K@	xlatb table for making pseudo-scancode
	db 45h	;^ka	45h points to an an offset in jumptab1
	db 41h	;^kb	41h for example points to KeyCtrlKB function offset
	db 43h	;^kc
	db 5dh	;^kd
	db 45h	;^ke	45h means SimpleRet i.e. 'do nothing'
	db 45h	;^kf
	db 45h	;^kg
	db 57h	;^kh
	db 45h	;^ki
	db 45h	;^kj
	db 42h	;^kk
	db 45h	;^kl
	db 45h	;^km
	db 45h	;^kn
	db 45h	;^ko
	db 45h	;^kp
	db 46h	;^kq
	db 3dh	;^kr	;not yet for ELKS
	db 5ch	;^ks
	db 45h	;^kt
	db 45h	;^ku
	db 56h	;^kv
	db 3eh	;^kw
	db 44h	;^kx
	db 4eh	;^ky
Qtable	db 45h	;^q@	ditto for ^Q menu
	db 54h	;^qa
	db 5ah	;^qb
	db 58h	;^qc 
	db 4fh	;^qd
	db 45h	;^qe	
	db 55h	;^qf
	db 45h	;^qg	
	db 45h	;^qh
	db 4Ah	;^qi
	db 45h	;^qj
	db 5bh	;^qk
	db 45h	;^ql
	db 45h	;^qm
	db 45h	;^qn
	db 45h	;^qo
	db 4ch	;^qp
	db 45h	;^qq
	db 59h	;^qr
	db 47h	;^qs			
	db 45h	;^qt
	db 45h	;^qu
	db 45h	;^qv
	db 45h	;^qw
	db 45h	;^qx
	db 40h	;^qy
size equ 2	;(byte per entry)
jumptab1:	; The associated key values originaly were BIOS scan codes...
		;  ... now using terminal device this does have less sense, so I altered some 
		;  ... special cases, like ^PageUp (was 84h, but extends the table too much)
		;  ... to some places shortly after 5dh (i.e. shift F10).
		; Using terminals the F-keys are not supported on ELKS (but DOS only). 
lowest 	equ 3bh	
	dw SimpleRet		;3bh
	dw KeyCtrlL		;3ch  ^L   F2 (ditto)
	dw KeyCtrlKR		;3dh  ^KR  F3 (etc)
	dw KeyCtrlKW		;3eh  ^KW
	dw KeyCtrlT		;3fh  ^T
	dw KeyCtrlQY		;40h  ^QY
	dw KeyCtrlKB		;41h  ^KB
	dw KeyCtrlKK		;42h  ^KK
	dw KeyCtrlKC		;43h  ^KC
	dw KeyCtrlKX		;44h  ^KX  F10
	dw SimpleRet		;45h       F11
	dw KeyCtrlKQ		;46h       F12
	dw KeyHome		;47h
	dw KeyUp		;48h
	dw KeyPgUp		;49h
	dw KeyCtrlQI		;4Ah
	dw KeyLeft		;4bh
	dw KeyCtrlQP		;(5 no num lock) 
	dw KeyRight		;4dh
	dw KeyCtrlKY		;(+)  ^KY
	dw KeyEnd		;4fh
	dw KeyDown		;50H
	dw KeyPgDn		;51h
	dw KeyIns		;52H
	dw KeyDel		;53H
	dw KeyCtrlQA		;54h ^QA sF1
	dw KeyCtrlQF		;55h ^QF sF2
	dw KeyCtrlKV		;56h ^KV
	dw KeyCtrlKH		;57h
	dw KeyCtrlQC		;58h
	dw KeyCtrlQR		;59h
	dw KeyCtrlQB		;5Ah ^QB
	dw KeyCtrlQK		;5Bh ^QK  sF8
	dw KeyCtrlKS		;5ch ^KS  sF9
	dw KeyCtrlKD		;5dh ^KD  sF10
jumps1 equ ($-jumptab1) / size
jumptab3 dw SimpleRet		;^@
	dw KeyCtrlLeft		;^a
	dw SimpleRet		;^b
	dw KeyPgDn		;^c
	dw KeyRight		;^d
	dw KeyUp		;^e
	dw KeyCtrlRight		;^f
	dw KeyDel		;^g 7
	dw KeyDell		;^h 8   DEL (7fh is translated to this)
	dw NormChar		;^i 9
	dw KeyRet		;^j = 0ah
	dw CtrlKMenu		;^k b
	dw KeyCtrlL		;^l c
	dw KeyRet		;^m 0dh
	dw SimpleRet		;^n e
	dw SimpleRet		;^o f
	dw CtrlQMenu		;^p 10	;^P like ^Q 
	dw CtrlQMenu		;^q 11
	dw KeyPgUp		;^r 12
	dw KeyLeft		;^s 13
	dw KeyCtrlT		;^t 14
	dw SimpleRet		;^u 15
	dw KeyIns		;^v 16
	dw SimpleRet		;^w 17
	dw KeyDown		;^x 18
	dw KeyCtrlY		;^y 19
;-------
optiontext	db 'OPT? C/B ',0
filename	db 'FILENAME:',0
filesave	db '   SAVE: ',0
asksave		db 'SAVE? Y/n',0
blockread	db '^KR NAME:',0
blocksave	db '^KW NAME:',0
asklineno	db 'GO LINE :',0
askfind		db '^QF FIND:',0
askreplace1	db '^QA REPL:',0
askreplace2	db '^QA WITH:',0
stdtxtlen	equ filesave-filename

%ifdef ELKS
 screencolors0	db 27,'[40m',27,'[37m'
 bold0		db 27,'[0m'		;reset to b/w
 screencolors1	db 27,'[41m',27,'[36m'	;yellow on blue
 reversevideoX:
 bold1:		db 27,'[1m'		;bold
 scolorslen	equ $-screencolors1
 boldlen	equ $-bold1		;take care length of bold0 == length of bold1
%endif

;-------
%macro LD 0
 %ifdef ELKS
  db 10
 %else
  db 13,10
 %endif
%endmacro
;-------
errmsgs:
%ifdef ELKS
db "Op not permitted"			;1
LD
db "No such file|directory"		;2
LD
LD					;3
LD					;4
db "Input/output"			;5
LD
db "No such device"			;6
LD
LD					;7
LD					;8
db "Bad file descriptor"		;9
LD
LD					;10
LD					;11
db "Cannot allocate memory"		;12
LD
db "Permission denied"			;13
LD
LD					;14
LD					;15
db "Device or resource busy"		;16
LD
LD					;17
LD					;18
db "No such device"			;19
LD
LD					;20
db "Is a directory"			;21
LD
db "Invalid argument"			;22
LD
db "Too many open files"		;23
LD
db "Too many open files"		;24
LD
db "Inappropriate ioctl"		;25
LD
db "Text file busy"			;26
LD
db "File too large"			;27
LD
db "No space left on device"		;28
LD
db "Illegal seek"			;29
LD
db "R/O file system"			;30
LD
%else
db "Op not permitted"			;1
LD
db "No such file|directory"		;2
LD
db "Path not found"			;3
LD
db "Too much open files"		;4
LD
db "Access denied"			;5
LD
LD					;6
LD					;7
LD					;8
LD					;9
LD					;10
LD					;11
db "Cannot allocate memory"		;12
LD
LD					;13
LD					;14
db "Invalid drive"
LD					;15
LD					;16
LD					;17
LD					;18
db "R/O file system"			;19
LD
LD					;20
db "Drive not ready"			;21
LD
db "Invalid argument"			;22
LD
LD					;23
LD					;24
db "Illegal seek"			;25
LD
LD					;26
LD					;27
LD					;28
db "Write"				;29
LD
db "Read"				;30
LD
%endif
;-----------------------------------------------------------------------
newline:
db 10
help:
db "MicroEditor e3/16bit v0.3 GPL (C) 2002,03 A.Kleine <kleine@ak.sax.de>"
LD
db "Enter filename or leave with RETURN"
LD
LD
db "Files:	^KR Insert	^KS Save	^KX Save&Exit	^KQ Abort&Exit"
LD
db "	^KD Save&Load"		
; ^KR not yet ready on ELKS
LD
LD
db "Blocks:	^KB Start	^KK End		^KC Copy	^KY Del"
LD
db "	^KV Move	^KW Write"
LD
LD
db "Search:	^QF Find	^L  Repeat	^QA Srch&Repl"
LD
LD
db "Move:	^E  Up		^X  Down	^S  Left	^D  Right"
LD
db "	^R  Page Up	^C  Page Dn	^F  Next Word	^A  Prev Word"
LD
LD
db "Quick-	^QS Home	^QD End		^QR BOF		^QC EOF"
LD
db "-Move:	^QB Blk Begin	^QK Blk End	^F  Next Word	^A  Prev Word"
LD
db "	^QI Go Line#"
LD
LD
db "Delete:	^T  Word	^Y  Line	^H  Left	^G  Chr"
LD
db "	^QY Line End"
LD
help_ws_size equ $-help
LD
%ifndef EXESTUB
EXE_enddata:
;-----------------------------------------------------------------------
;
section .bss
bits 16
%endif
EXE_startbss:
;
%ifdef ELKS
 screenline_len	equ 256+4*scolorslen	;max possible columns + 4 color ESC seq per line
%else	;--------------
 screenline_len	equ sBREITE * 2		;2 byte per char
%endif

%ifdef ELKS
 termios_size	equ 60
 termios	resb termios_size
 orig		resb termios_size
 setkplen	equ 10
 setkp		resb setkplen		;to store cursor ESC seq like  db 27,'[000;000H'
 read_b		resw 1			;buffer for GetChar
 isbold		resw 1			;control of bold display of ws-blocks
 inverse	resw 1
 seekhelp	resd 1			;syscall helper (for seeking more than 16 bit range)
 perms		resw 1
%ifdef SYS_FSTAT
 fstatbuf	resb 64			;prepared for later
%endif
%else		;-----
 zeilenangabe	resb 12			;buffer for showlinenum
%endif
%ifdef LESSWRITEOPS
 screenbuffer_size equ 62*(160+32)	;estimated 62 lines 160 columns, 32 byte ESC seq (ca.12k)
 screenbuffer_dwords equ screenbuffer_size/4
 screenbuffer resb screenbuffer_size
 screenbuffer_end equ $			;If you really have higher screen resolution,
%endif
errno		resw 1			;used similar libc, but not excactly equal
error		resb errlen		;reserved space for string: 'ERROR xxx:tttteeeexxxxtttt',0
columne		resw 1			;helper for display of current column
zloffset	resw 1			;helper: chars scrolled out at left border
fileptr		resw 1			;helper for temp storage of current pos in file
tabcnt		resw 1			;internal helper byte in DispNewScreen() only
kurspos		resw 1			;cursor position set by DispNewScreen()
kurspos2	resw 1			;cursor position set by other functions
insstat		resw 1
endedit		resw 1			;byte controls program exit
changed		resw 1			;status byte: (UN)CHANGED
linenr		resw 1			;current line
showblock	resw 1			;helper for ^KH
blockbegin	resw 1
blockende	resw 1
bereitsges	resw 1			;byte used for ^L
suchlaenge	resw 1			;helper for ^QA,^QF
repllaenge	resw 1
vorwarts	resw 1
grossklein	resw 1			;helper byte for ^QF,^QA
old		resw 1			;helper for ^QP
veryold		resw 1			;ditto
ch2linebeg	resw 1			;helper keeping cursor pos max at EOL (up/dn keys)
numeriere	resw 1			;byte controls re-numeration
lines		resw 1			;equ 23 or similar i.e. screen lines-2 (status-,unused line)
columns		resw 1			;equ 80 or similar word (using only LSB)
filepath	resb maxfilenamelen+1
blockpath	resb maxfilenamelen+1
replacetext	resb maxfilenamelen+1
suchtext	resb maxfilenamelen+1
optbuffer	resb maxfilenamelen+1	;buffer for search/replace options and for ^QI
optslen		equ $-optbuffer
screenline	resb screenline_len	;buffer for displaying a text line
textX		resb MAXLEN
sot 		equ (textX+1)		;start-of-text

alignb 4
EXE_endbss:
	EXE_absssize equ (EXE_endbss-EXE_startbss+3) & (~3)
%ifdef EXE
	EXE_acodesize equ (EXE_endcode-EXE_startcode+3) & (~3)
	EXE_datasize equ EXE_enddata-EXE_startdata
	EXE_allocsize equ EXE_acodesize + EXE_datasize +100h
%endif

%ifndef ELKS
%ifdef EXESTUB
bits 16
section .stack stack
	resb 0x800
%endif
%endif
