! 1 
! 1 # 1 "elkplan4.c"
! 1 
! 2 
! 3 
! 4 
! 5 
! 6 
! 7 
! 8 
! 9 # 1 "vga_dev.h" 1
! 1 
! 2 
! 3 
! 4 
! 5 
! 6 
! 7 
! 8 
! 9 
! 10 
! 11 
! 12 
! 13 
! 14 
! 15 
! 16 
! 17 
! 18 
! 19 
! 20 
! 21 
! 22 
! 23 
! 24 
! 25 
! 26 
! 27 
! 28 
! 29 
! 30 
! 31 
! 32 
! 33 
! 34 
! 35 
! 36 
! 37 
! 38 
! 39 
! 40 
! 41 
! 42 
! 43 
! 44 
! 45 
! 46 
! 47 
! 48 
! 49 
! 50 
! 51 
! 52 
! 53 
! 54 
! 55 
! 56 
! 57 
! 58 
! 59 
! 60 
! 61 
! 62 
! 63 
! 64 
! 65 
! 66 
! 67 
! 68 
! 69 
! 70 
! 71 
! 72 
! 73 
! 74 
! 75 
! 76 
! 77 
! 78 
! 79 
! 80 
! 81 
! 82 
! 83 
! 84 
! 85 
! 86 
! 87 
! 88 
! 89 
! 90 
! 91 
! 92 
! 93 
! 94 
! 95 
! 96 
! 97 
! 98 
! 99 
! 100 
! 101 
! 102 
! 103 
! 104 
! 105 
! 106 
! 107 
! 108 typedef unsigned char PIXELVAL;
! 109 
! 110 
! 111 
! 112 
! 113 
! 114 
! 115 
! 116 typedef int		COORD;		
! 117 typedef int		MODE;		
! 118 typedef unsigned long	COLORVAL;	
! 119 typedef unsigned int	BUTTON;		
! 120 typedef unsigned int	MODIFIER;	
! 121 typedef int		FONTID;		
! 122 typedef unsigned short	IMAGEBITS;	
! 123 
! 124 
! 125 
! 126 
! 127 typedef int		BOOL;		
! 128 
! 129 
! 130 typedef unsigned char 	UCHAR;		
! 131 
! 132 
! 133 
! 134 
! 135 
! 136 
! 137 
! 138 
! 139 
! 140 
! 141 
! 142 
! 143 
! 144 typedef struct {
! 145 	COORD 	 rows;		
! 146 	COORD 	 cols;		
! 147 	int 	 xdpcm;		
! 148 	int 	 ydpcm;		
! 149 	int	 planes;	
! 150 	int	 bpp;		
! 151 	long	 ncolors;	
! 152 	int 	 fonts;		
! 153 	BUTTON 	 buttons;	
! 154 	MODIFIER modifiers;	
! 155 	int	 pixtype;	
! 156 } SCREENINFO, *PSCREENINFO;
! 157 
! 158 
! 159 typedef struct {
! 160 	int 	font;		
! 161 	int 	height;		
! 162 	int 	maxwidth;	
! 163 	int 	baseline;	
! 164 	BOOL	fixed;		
! 165 	UCHAR	widths[256];	
! 166 } FONTINFO, *PFONTINFO;
! 167 
! 168 
! 169 typedef struct {
! 170 	char *		name;		
! 171 	int		maxwidth;	
! 172 	int		height;		
! 173 	int		firstchar;	
! 174 	int		size;		
! 175 	IMAGEBITS *	bits;		
! 176 	unsigned short *offset;		
! 177 	unsigned char *	width;		
! 178 					
! 179 } FONT, *PFONT;
! 180 
! 181 
! 182 typedef struct {
! 183 	int		width;			
! 184 	int		height;			
! 185 	COORD		hotx;			
! 186 	COORD		hoty;			
! 187 	COLORVAL	fgcolor;		
! 188 	COLORVAL	bgcolor;		
! 189 	IMAGEBITS	image[16];	
! 190 	IMAGEBITS	mask[16];	
! 191 } SWCURSOR, *PSWCURSOR;
! 192 
! 193 
! 194 typedef struct {
! 195 	UCHAR	r;
! 196 	UCHAR	g;
! 197 	UCHAR	b;
! 198 } RGBENTRY;
! 199 
! 200 
! 201 
! 202 
! 203 typedef struct {
! 204 	COORD dstx, dsty, dstw, dsth, dst_linelen;
! 205 	COORD srcx, srcy, src_linelen;
! 206 	void *pixels, *misc;
! 207 	PIXELVAL bg_color, fg_color;
! 208 	int gr_usebg;
! 209 } driver_gc_t;
! 210 
! 211 
! 212 
! 213 
! 214 
! 215 
! 216 
! 217 
! 218 
! 219 
! 220 
! 221 
! 222 
! 223 
! 224 
! 225 
! 226 
! 227 typedef struct _screendevice *PSD;
! 228 typedef struct _screendevice {
! 229 	COORD	xres;		
! 230 	COORD	yres;		
! 231 	COORD	xvirtres;	
! 232 	COORD	yvirtres;	
! 233 	int	planes;		
! 234 	int	bpp;		
! 235 	int	linelen;	
! 236 				
! 237 	int	size;		
! 238 	long	ncolors;	
! 239 	int	pixtype;	
! 240 	int	flags;		
! 241 	void *	addr;		
! 242 
! 243 	int	(*Open)();
! 244 	void	(*Close)();
! 245 	void	(*GetScreenInfo)();
! 246 	void	(*SetPalette)();
! 247 	void	(*DrawPixel)();
! 248 	PIXELVAL(*ReadPixel)();
! 249 	void	(*DrawHorzLine)();
! 250 	void	(*DrawVertLine)();
! 251 	void	(*FillRect)();
! 252 	
! 253 
! 254 	BOOL	(*GetFontInfo)();
! 255 	void	(*GetTextSize)(
! 256 );
! 257 	void	(*GetTextBits)(
! 258 );
! 259 	void	(*Blit)(
! 260 );
! 261 	void	(*PreSelec
! 261 t)();
! 262 	void	(*DrawArea)();
! 263 } SCREENDEVICE;
! 264 
! 265 
! 266 
! 267 
! 268 
! 269 
! 270 
! 271 
! 272 
! 273 typedef struct _mousedevice {
! 274 	int	(*Open)();
! 275 	void	(*Close)();
! 276 	BUTTON	(*GetButtonInfo)();
! 277 	void	(*GetDefaultAccel)();
! 278 	int	(*Read)();
! 279 	int	(*Poll)();		
! 280 } MOUSEDEVICE;
! 281 
! 282 
! 283 typedef struct _kbddevice {
! 284 	int  (*Open)();
! 285 	void (*Close)();
! 286 	void (*GetModifierInfo)();
! 287 	int  (*Read)();
! 288 	int  (*Poll)();		
! 289 } KBDDEVICE;
! 290 
! 291 
! 292 typedef struct {
! 293 	COORD 	x;		
! 294 	COORD 	y;		
! 295 } XYPOINT;
! 296 
! 297 
! 298 typedef struct {
! 299 	COORD 	x;		
! 300 	COORD 	y;		
! 301 	COORD 	width;		
! 302 	COORD 	height;		
! 303 } CLIPRECT;
! 304 
! 305 
! 306 
! 307 
! 308 
! 309 
! 310 
! 311 
! 312 
! 313 
! 314 typedef struct {
! 315 	COORD	left;
! 316 	COORD	top;
! 317 	COORD	right;
! 318 	COORD	bottom;
! 319 } RECT;
! 320 
! 321 
! 322 typedef struct {
! 323 	int	size;		
! 324 	int	numRects;	
! 325 	int	type; 		
! 326 	RECT *	rects;		
! 327 	RECT	extents;	
! 328 } CLIPREGION;
! 329 
! 330 
! 331 
! 332 
! 333 
! 334 
! 335 
! 336 
! 337 
! 338 
! 339 
! 340 
! 341 
! 342 
! 343 
! 344 
! 345 
! 346 
! 347 
! 348 
! 349 
! 350 
! 351 
! 352 
! 353 
! 354 
! 355 
! 356 
! 357 
! 358 
! 359 
! 360 
! 361 
! 362 
! 363 
! 364 
! 365 
! 366 
! 367 
! 368 
! 369 
! 370 
! 371 
! 372 
! 373 
! 374 
! 375 
! 376 
! 377 
! 378 
! 379 
! 380 
! 381 
! 382 
! 383 
! 384 
! 385 
! 386 
! 387 
! 388 
! 389 
! 390 
! 391 
! 392 
! 393 
! 394 
! 395 
! 396 
! 397 
! 398 
! 399 
! 400 
! 401 
! 402 
! 403 
! 404 
! 405 
! 406 
! 407 
! 408 
! 409 
! 410 
! 411 
! 412 
! 413 
! 414 
! 415 
! 416 
! 417 
! 418 
! 419 
! 420 
! 421 
! 422 
! 423 
! 424 
! 425 
! 426 
! 427 
! 428 
! 429 
! 430 
! 431 
! 432 
! 433 
! 434 
! 435 
! 436 
! 437 
! 438 
! 439 
! 440 
! 441 
! 442 
! 443 
! 444 
! 445 
! 446 
! 447 
! 448 
! 449 
! 450 
! 451 
! 452 
! 453 
! 454 
! 455 
! 456 
! 457 
! 458 
! 459 
! 460 
! 461 
! 462 
! 463 
! 464 
! 465 typedef struct {
! 466 	int		width;		
! 467 	int		height;		
! 468 	int		planes;		
! 469 	int		bpp;		
! 470 	int		compression;	
! 471 	int		palsize;	
! 472 	RGBENTRY *	palette;	
! 473 	UCHAR *		imagebits;	
! 474 } IMAGEHDR, *PIMAGEHDR;
! 475 
! 476 
! 477 
! 478 
! 479 
! 480 
! 481 
! 482 
! 483 int	GdOpenScreen();
! 484 void	GdCloseScreen();
! 485 MODE	GdSetMode();
! 486 BOOL	GdSetUseBackground();
! 487 PIXELVAL GdSetForeground();
! 488 PIXELVAL GdSetBackground();
! 489 FONTID	GdSetFont();
! 490 void	GdResetPalette();
! 491 void	GdSetPalette();
! 492 int	GdGetPalette();
! 493 PIXELVAL GdFindColor();
! 494 PIXELVAL GdFindNearestColor();
! 495 void	GdGetScreenInfo();
! 496 BOOL	GdGetFontInfo();
! 497 void	GdPoint();
! 498 void	GdLine();
! 499 void	GdRect();
! 500 void	GdFillRect();
! 501 void	GdGetTextSize();
! 502 void	GdText(
! 503 );
! 504 void	GdBitmap(
! 505 );
! 506 BOOL	GdColorInPalette();
! 507 void	GdMakePaletteConversionTable(
! 508 );
! 509 void	GdDrawImage();
! 510 void	GdEllipse();
! 511 void	GdFillEllipse();
! 512 void	GdPoly();
! 513 void	GdFillPoly();
! 514 void	GdReadArea(
! 515 );
! 516 void	GdArea(
! 517 );
! 518 void	GdCopyArea(
! 519 );
! 520 void	GdBlit(
! 521 );
! 522 extern SCREENDEVICE scrdev;
! 523 
! 524 
! 525 void 	GdSetClipRects();
! 526 BOOL	GdClipPoint();
! 527 int	GdClipArea();
! 528 extern COORD clipminx, clipminy, clipmaxx, clipmaxy;
! 529 
! 530 
! 531 BOOL GdPtInRegion();
! 532 BOOL GdRectInRegion();
! 533 CLIPREGION *GdAllocClipRegion();
! 534 void GdDestroyClipRegion();
! 535 void GdUnionRectWithRegion();
! 536 void GdCopyRegion();
! 537 void GdIntersectRegion();
! 538 void Gd
! 538 UnionRegion();
! 539 void GdSubtractRegion();
! 540 void GdXorRegion();
! 541 
! 542 
! 543 int	GdOpenMouse();
! 544 void	GdCloseMouse();
! 545 void	GdGetButtonInfo();
! 546 void	GdRestrictMouse(
! 547 );
! 548 void	GdSetAccelMouse();
! 549 void	GdMoveMouse();
! 550 int	GdReadMouse();
! 551 void	GdMoveCursor();
! 552 void	GdSetCursor();
! 553 int 	GdShowCursor();
! 554 int 	GdHideCursor();
! 555 void	GdCheckCursor();
! 556 void 	GdFixCursor();
! 557 extern MOUSEDEVICE mousedev;
! 558 
! 559 
! 560 int  	GdOpenKeyboard();
! 561 void 	GdCloseKeyboard();
! 562 void 	GdGetModifierInfo();
! 563 int  	GdReadKeyboard();
! 564 extern KBDDEVICE kbddev;
! 565 
! 566 
! 567 void GdJPEG (
! 568 );
! 569 void GdBMP (
! 570 );
! 571 
! 572 
! 573 
! 574 
! 575 
! 576 
! 577 
! 578 
! 579 
! 580 
! 581 
! 582 
! 583 
! 584 
! 585 
! 586 
! 587 
! 588 
! 589 # 9 "elkplan4.c" 2
! 9 
! 10 # 1 "vgaplan4.h" 1
! 1 
! 2 
! 3 
! 4 
! 5 
! 6 
! 7 
! 8 
! 9 
! 10 
! 11 
! 12 
! 13 
! 14 
! 15 
! 16 
! 17 
! 18 
! 19 
! 20 
! 21 
! 22 
! 23 
! 24 
! 25 
! 26 
! 27 
! 28 
! 29 
! 30 
! 31 
! 32 
! 33 
! 34 
! 35 
! 36 
! 37 
! 38 
! 39 
! 40 
! 41 
! 42 
! 43 
! 44 
! 45 
! 46 
! 47 
! 48 
! 49 
! 50 
! 51 
! 52 
! 53 
! 54 
! 55 
! 56 
! 57 
! 58 
! 59 
! 60 
! 61 
! 62 
! 63 
! 64 
! 65 
! 66 
! 67 
! 68 
! 69 
! 70 
! 71 
! 72 typedef unsigned long	FARADDR;
! 73 
! 74 
! 75 extern unsigned char GETBYTE_FP();
! 76 
! 77 
! 78 extern void PUTBYTE_FP();
! 79 
! 80 
! 81 extern void RMW_FP();
! 82 
! 83 
! 84 extern void ORBYTE_FP();
! 85 
! 86 
! 87 extern void ANDBYTE_FP();
! 88 
! 89 
! 90 
! 91 
! 92 
! 93 
! 94 
! 95 
! 96 
! 97 
! 98 
! 99 extern int  inportb();
! 100 extern void outportb();
! 101 extern void outport();
! 102 
! 103 
! 104 
! 105 
! 106 
! 107 
! 108 
! 109 
! 110 
! 111 
! 112 
! 113 
! 114 
! 115 
! 116 
! 117 
! 118 
! 119 
! 120 
! 121 
! 122 
! 123 
! 124 
! 125 
! 126 
! 127 
! 128 
! 129 
! 130 
! 131 
! 132 
! 133 
! 134 
! 135 
! 136 
! 137 
! 138 
! 139 FARADDR		int10();
! 140 
! 141 
! 142 
! 143 
! 144 int		ega_init();
! 145 void 		ega_drawpixel();
! 146 PIXELVAL 	ega_readpixel();
! 147 void		ega_drawhorzline(
! 148 );
! 149 void		ega_drawvertline(
! 150 );
! 151 void	 	ega_blit(
! 152 );
! 153 
! 154 
! 155 void		ega_hwinit();
! 156 void		ega_hwterm();
! 157 
! 158 
! 159 
! 160 void 	 mempl4_drawpixel();
! 161 PIXELVAL mempl4_readpixel();
! 162 void 	 mempl4_drawhorzline();
! 163 void 	 mempl4_drawvertline();
! 164 void 	 mempl4_to_mempl4_blit(
! 165 );
! 166 void 	 vga_to_mempl4_blit(
! 167 );
! 168 void 	 mempl4_to_vga_blit(
! 169 );
! 170 void 	 vga_to_vga_blit(
! 171 );
! 172 
! 173 
! 174 
! 175 
! 176 
! 177 
! 178 
! 179 
! 180 
! 181 
! 182 
! 183 
! 184 
! 185 
! 186 
! 187 
! 188 
! 189 
! 190 
! 191 
! 192 
! 193 
! 194 
! 195 
! 196 
! 197 
! 198 
! 199 
! 200 
! 201 
! 202 
! 203 
! 204 # 10 "elkplan4.c" 2
! 10 
! 11 
! 12 
! 13 
! 14 
! 15 
! 16 
! 17 
! 18 extern MODE gr_mode;	
! 19 
! 20 static unsigned char mode_table[3 + 1] = {
.data
_mode_table:
! 21   0x00, 0x18, 0x10, 0x08
.byte	0
.byte	$18
.byte	$10
! 22 };
.byte	8
! 23 
! 24 int
! 25 ega_init(psd)
! 26 # 25 "elkplan4.c"
! 25 PSD psd;
.text
export	_ega_init
_ega_init:
! 26 {
! 27 	
! 28 	psd->addr = ((FARADDR)(((unsigned long)(0xa000)<< 16)| (unsigned)(0)));
push	bp
mov	bp,sp
push	di
push	si
mov	bx,4[bp]
xor	ax,ax
mov	$18[bx],ax
! 29 	psd->linelen = 80;
mov	bx,4[bp]
mov	ax,*$50
mov	$C[bx],ax
! 30 
! 31 	
! 32 	{ outportb(0x3ce,1); outportb(0x3cf,0x0f); };
mov	ax,*1
push	ax
mov	ax,#$3CE
push	ax
call	_outportb
add	sp,*4
mov	ax,*$F
push	ax
mov	ax,#$3CF
push	ax
call	_outportb
add	sp,*4
! 33 	{ outportb(0x3ce,3); outportb(0x3cf,0); };
mov	ax,*3
push	ax
mov	ax,#$3CE
push	ax
call	_outportb
add	sp,*4
xor	ax,ax
push	ax
mov	ax,#$3CF
push	ax
call	_outportb
add	sp,*4
! 34 	{ outportb(0x3ce,5); outportb(0x3cf,0); };
mov	ax,*5
push	ax
mov	ax,#$3CE
push	ax
call	_outportb
add	sp,*4
xor	ax,ax
push	ax
mov	ax,#$3CF
push	ax
call	_outportb
add	sp,*4
! 35 
! 36 	return 1;
mov	ax,*1
pop	si
pop	di
pop	bp
ret
! 37 }
! 38 
! 39 
! 40 
! 41 
! 42 
! 43 
! 44 
! 45 
! 46 
! 47 
! 48 
! 49 void
! 50 ega_drawhorzline(psd,x1,x2,y,color)
! 51 # 50 "elkplan4.c"
! 50 PSD psd;
export	_ega_drawhorzline
_ega_drawhorzline:
! 51 # 50 "elkplan4.c"
! 50 int x1;
! 51 # 50 "elkplan4.c"
! 50 int x2;
! 51 # 50 "elkplan4.c"
! 50 int y;
! 51 # 50 "elkplan4.c"
! 50 int color;
! 51 {
! 52 #asm
!BCC_ASM
_ega_drawhorzline.color	set	$A
_ega_drawhorzline.psd	set	2
_ega_drawhorzline.y	set	8
_ega_drawhorzline.x1	set	4
_ega_drawhorzline.x2	set	6
	push	bp		; setup stack frame and preserve registers
	mov	bp, sp
	push	si
	push	di
	push	es

	dec	[bp+8]		; dec x2 - don't draw final point
	; configure the graphics controller

	mov	dx, #$03ce	; DX := Graphics Controller port address
	
	mov	al, #3		; set data rotate register
	lea	bx, _mode_table
	add	bx, _gr_mode
	mov	ah, [bx]
	out	dx, ax

	mov 	ah, [bp+12]	; pixel value
	xor	al, al		; Set/Reset register number (0)
	out	dx, ax

	mov	ax, #$0f01	; AH := bit plane mask for Enable Set/Reset
	out	dx, ax		; AL := Enable Set/Reset register number

	push	ds		; preserve DS

	mov	ax, [bp+10]	; y
	mov	bx, [bp+6]	; x1

	; compute pixel address
	mov	dx, #80 ; AX := [row * 80]
	mul	dx
	mov 	cl, bl		; save low order column bits
	shr	bx, #1		; BX := [col / 8]
	shr	bx, #1
	shr	bx, #1
	add	bx, ax		; BX := [row * 80] + [col / 8]
	and	cl, #$07	; CL := [col % 8]
	xor	cl, #$07	; CL := 7 - [col % 8]
	mov 	ah, #$01	; AH := 1 << [7 - [col % 8]]	[mask]
	mov	dx, #$0a000	; ES := EGA buffer segment address
	mov	es, dx
				; AH := bit mask
				; ES:BX -> video buffer
				; CL := number bits to shift left
	mov	di, bx		; ES:DI -> buffer
	mov 	dh, ah		; DH := unshifted bit mask for left byte

	not	dh
	shl	dh, cl		; DH := reverse bit mask for first byte
	not	dh		; DH := bit mask for first byte

	mov	cx, [bp+8]	; x2
	and	cl, #7
	xor	cl, #7		; CL := number of bits to shift left
	mov 	dl, #$0ff	; DL := unshifted bit mask for right byte
	shl	dl, cl		; DL := bit mask for last byte

	; determine byte offset of first and last pixel in the line

	mov	ax, [bp+8]	; AX := x2
	mov	bx, [bp+6]	; BX := x1

	mov 	cl, #3		; bits to convert pixels to bytes

	shr	ax, cl		; AX := byte offset of X2
	shr	bx, cl		; BX := byte offset of X1
	mov	cx, ax
	sub	cx, bx		; CX := [number of bytes in line] - 1

	; get Graphics Controller port address into DX

	mov	bx, dx		; BH := bit mask for first byte
				; BL := bit mask for last byte
	mov	dx, #$03ce	; DX := Graphics Controller port
	mov 	al, #8		; AL := Bit mask Register number

	; make video buffer addressable through DS:SI

	push	es
	pop	ds
	mov	si, di		; DS:SI -> video buffer

	; set pixels in leftmost byte of the line

	or	bh, bh
	js	L43		; jump if byte-aligned [x1 is leftmost]

	or	cx, cx
	jnz	L42		; jump if more than one byte in the line

	and	bl, bh		; BL := bit mask for the line
	jmp near L44

L42:	mov 	ah, bh		; AH := bit mask for first byte
	out	dx, ax		; update graphics controller

	movsb			; update bit planes
	dec	cx

	; use a fast 8086 machine instruction to draw the remainder of the line

L43:	mov 	ah, #$0ff	; AH := bit mask
	out	dx, ax		; update Bit Mask register
	rep 
	movsb			; update all pixels in the line

	; set pixels in the rightmost byte of the line

L44:	mov 	ah, bl		; AH := bit mask for last byte
	out	dx, ax		; update Graphics Controller
	movsb			; update bit planes

	pop	ds		; restore ds

	; restore default Graphics Controller state and return to caller
	;;xor	ax, ax		; AH := 0, AL := 0
	;;out	dx, ax		; restore Set/Reset register
	;;inc	ax		; AH := 0, AL := 1
	;;out	dx, ax		; restore Enable Set/Reset register
	;;mov	ax, #$0ff08	; AH := 0xff, AL := 0
	;;out	dx, ax		; restore Bit Mask register

	pop	es
	pop	di
	pop	si
	pop	bp
!BCC_ENDASM
! 181 }
ret
! 182 
! 183 
! 184 
! 185 
! 186 
! 187 
! 188 
! 189 
! 190 
! 191 
! 192 
! 193 
! 194 void
! 195 ega_drawvertline(psd,x,y1,y2,color)
! 196 # 195 "elkplan4.c"
! 195 PSD psd;
export	_ega_drawvertline
_ega_drawvertline:
! 196 # 195 "elkplan4.c"
! 195 int x;
! 196 # 195 "elkplan4.c"
! 195 int y1;
! 196 # 195 "elkplan4.c"
! 195 int y2;
! 196 # 195 "elkplan4.c"
! 195 int color;
! 196 {
! 197 #asm
!BCC_ASM
_ega_drawvertline.color	set	$A
_ega_drawvertline.y2	set	8
_ega_drawvertline.psd	set	2
_ega_drawvertline.y1	set	6
_ega_drawvertline.x	set	4
	push	bp		; setup stack frame and preserve registers
	mov	bp, sp
	push	ds

	dec	[bp+10]		; dec y2 - don't draw final point
	; configure the graphics controller

	mov	dx, #$03ce	; DX := Graphics Controller port address

	mov	al, #3		; set data rotate register
	lea	bx, _mode_table
	add	bx, _gr_mode
	mov	ah, [bx]
	out	dx, ax

	mov 	ah, [bp+12]	; color pixel value
	xor	al, al		; Set/Reset register number (0)
	out	dx, ax

	mov	ax, #$0f01	; AH := bit plane mask for Enable Set/Reset
	out	dx, ax		; AL := Enable Set/Reset register number

	; prepare to draw vertical line

	mov	ax, [bp+8]	; AX := y1
	mov	cx, [bp+10]	; BX := y2
	;;mov	cx, bx
	sub	cx, ax		; CX := dy
	;;jge	L311		; jump if dy >= 0
	;;neg	cx		; force dy >= 0
	;;mov	ax, bx		; AX := y2

L311:	inc	cx		; CX := number of pixels to draw
	mov	bx, [bp+6]	; BX := x
	push	cx		; save register

	; compute pixel address
	push	dx
	mov	dx, #80 ; AX := [row * 80]
	mul	dx
	mov 	cl, bl		; save low order column bits
	shr	bx, #1		; BX := [col / 8]
	shr	bx, #1
	shr	bx, #1
	add	bx, ax		; BX := [row * 80] + [col / 8]
	and	cl, #$07	; CL := [col % 8]
	xor	cl, #$07	; CL := 7 - [col % 8]
	mov 	ah, #$01	; AH := 1 << [7 - [col % 8]]	[mask]
	mov	dx, #$0a000	; DS := EGA buffer segment address
	mov	ds, dx
	pop	dx
				; AH := bit mask
				; DS:BX -> video buffer
				; CL := number bits to shift left

	; set up Graphics controller

	shl	ah, cl		; AH := bit mask in proper position
	mov 	al, #$08	; AL := Bit Mask register number
	out	dx, ax

	pop	cx		; restore register

	; draw the line

	mov	dx, #80 ; increment for video buffer
L1111:	or	[bx], al	; set pixel
	add	bx, dx		; increment to next line
	loop	L1111

	; restore default Graphics Controller state and return to caller
	;;xor	ax, ax		; AH := 0, AL := 0
	;;out	dx, ax		; restore Set/Reset register
	;;inc	ax		; AH := 0, AL := 1
	;;out	dx, ax		; restore Enable Set/Reset register
	;;mov	ax, #$0ff08	; AH := 0xff, AL := 0
	;;out	dx, ax		; restore Bit Mask register

	pop	ds
	pop	bp
!BCC_ENDASM
! 279 }
ret
! 280 
! 281 
! 282 
! 283 
! 284 
! 285 
! 286 void
! 287 ega_drawpixel(psd,x,y,color)
! 288 # 287 "elkplan4.c"
! 287 PSD psd;
export	_ega_drawpixel
_ega_drawpixel:
! 288 # 287 "elkplan4.c"
! 287 int x;
! 288 # 287 "elkplan4.c"
! 287 int y;
! 288 # 287 "elkplan4.c"
! 287 int color;
! 288 {
! 289 #asm
!BCC_ASM
_ega_drawpixel.color	set	8
_ega_drawpixel.psd	set	2
_ega_drawpixel.y	set	6
_ega_drawpixel.x	set	4
	push	bp
	mov	bp, sp

	mov	dx, #$03ce	; graphics controller port address
	mov	al, #3		; set data rotate register
	lea	bx, _mode_table
	add	bx, _gr_mode
	mov	ah, [bx]
	out	dx, ax

	mov	cx, [bp+6]	; ECX := x
	mov	ax, [bp+8]	; EAX := y

	mov	dx, #80 ; AX := [y * 80]
	mul	dx

	mov	bx, cx		; BX := [x / 8]
	shr	bx, #1
	shr	bx, #1
	shr	bx, #1

	add	bx, ax		; BX := [y * 80] + [x / 8]

	and	cl, #$07	; CL := [x % 8]
	xor	cl, #$07	; CL := 7 - [x % 8]
	mov 	ch, #$01	; CH := 1 << [7 - [x % 8]]	[mask]
	shl	ch, cl

	mov	dx, #$03ce	; graphics controller port address

	;;required for old code
	mov	ax, #$0205	; select write mode 2
	out	dx, ax		; [load value 2 into mode register 5]

	; new code
	;;xor	ax,ax		; set color register 0
	;;mov	ah,[bp+10]	; color pixel value
	;;out	dx,ax

	; original code
	mov 	al, #$08	; set the bit mask register
	mov 	ah, ch		; [load bit mask into register 8]
	out	dx, ax

	push	ds
	mov	ax, #$0a000	; DS := EGA buffer segment address
	mov	ds, ax

	; new code
	;;or	[bx],al		; quick rmw to set pixel

	;;the following fails under 1 without cli/sti
	;;using ES works though.  Code changed to use single
	;;rmw above rather than write mode 2, but the
	;;reason for this failure is still unknown...
	;;cli
	mov 	al, [bx]	; dummy read to latch bit planes
	mov	al, [bp+10]	; pixel value
	mov 	[bx], al	; write pixel back to bit planes
	;;sti

	pop	ds		; restore registers and return

	mov	ax, #$0005	; restore default write mode 0
	out	dx, ax		; [load value 0 into mode register 5]

	;;mov	ax, #$0ff08	; restore default bit mask
	;;out	dx, ax		; [load value ff into register 8]

	pop	bp
!BCC_ENDASM
! 361 }
ret
! 362 
! 363 
! 364 
! 365 
! 366 
! 367 
! 368 PIXELVAL
! 369 ega_readpixel(psd,x,y)
! 370 # 369 "elkplan4.c"
! 369 PSD psd;
export	_ega_readpixel
_ega_readpixel:
! 370 # 369 "elkplan4.c"
! 369 int x;
! 370 # 369 "elkplan4.c"
! 369 int y;
! 370 {
! 371 #asm
!BCC_ASM
_ega_readpixel.psd	set	2
_ega_readpixel.y	set	6
_ega_readpixel.x	set	4
	push	bp
	mov	bp, sp
	push	si
	push	ds

	mov	ax, [bp+8]	; EAX := y
	mov	bx, [bp+6]	; EBX := x
	mov	dx, #80 ; AX := [y * 80]
	mul	dx

	mov 	cl, bl		; save low order column bits
	shr	bx, #1		; BX := [x / 8]
	shr	bx, #1
	shr	bx, #1

	add	bx, ax		; BX := [y * 80] + [x / 8]

	and	cl, #$07	; CL := [x % 8]
	xor 	cl, #$07	; CL := 7 - [x % 8]

	mov	dx, #$0a000	; DS := EGA buffer segment address
	mov	ds, dx

	mov 	ch, #$01	; CH := 1 << [7 - [col % 8]]  [mask]
	shl	ch, cl		; CH := bit mask in proper position

	mov	si, bx		; DS:SI -> region buffer byte
	xor	bl, bl		; BL is used to accumulate the pixel value

	mov	dx, #$03ce	; DX := Graphics Controller port
	mov	ax, #$0304	; AH := initial bit plane number
				; AL := Read Map Select register number

L112:	out	dx, ax		; select bit plane
	mov 	bh, [si]	; BH := byte from current bit plane
	and	bh, ch		; mask one bit
	neg	bh		; bit 7 of BH := 1 if masked bit = 1
				; bit 7 of BH := 0 if masked bit = 0
	rol	bx, #1		; bit 0 of BL := next bit from pixel value
	dec	ah		; AH := next bit plane number
	jge	L112

	xor	ax, ax		; AL := pixel value
	mov 	al, bl

	pop	ds
	pop	si
	pop	bp	
!BCC_ENDASM
! 421 }
ret
! 422 
! 423 void
! 424 ega_blit(dstpsd,dstx,dsty,w,h,
! 425 srcpsd,srcx,srcy,op)
! 426 # 424 "elkplan4.c"
! 424 PSD dstpsd;
export	_ega_blit
_ega_blit:
! 425 # 424 "elkplan4.c"
! 424 COORD dstx;
! 425 # 424 "elkplan4.c"
! 424 COORD dsty;
! 425 # 424 "elkplan4.c"
! 424 COORD w;
! 425 # 424 "elkplan4.c"
! 424 COORD h;
! 425 PSD srcpsd;
! 426 # 425 "elkplan4.c"
! 425 COORD srcx;
! 426 # 425 "elkplan4.c"
! 425 COORD srcy;
! 426 # 425 "elkplan4.c"
! 425 int op;
! 426 {
! 427 
! 428 	BOOL	srcvga, dstvga;
! 429 
! 430 	
! 431 	srcvga = srcpsd->flags & 0x01;
push	bp
mov	bp,sp
push	di
push	si
add	sp,*-4
mov	bx,$E[bp]
mov	al,$16[bx]
and	al,*1
xor	ah,ah
mov	-6[bp],ax
! 432 	dstvga = dstpsd->flags & 0x01;
mov	bx,4[bp]
mov	al,$16[bx]
and	al,*1
xor	ah,ah
mov	-8[bp],ax
! 433 
! 434 	if (srcvga) {
mov	ax,-6[bp]
test	ax,ax
je  	.1
.2:
! 435 		if (dstvga)
mov	ax,-8[bp]
test	ax,ax
je  	.3
.4:
! 436 			vga_to_vga_blit(dstpsd,dstx,dsty,w,h,
! 437 srcpsd,srcx,srcy,op);
push	$14[bp]
push	$12[bp]
push	$10[bp]
push	$E[bp]
push	$C[bp]
push	$A[bp]
push	8[bp]
push	6[bp]
push	4[bp]
call	_vga_to_vga_blit
add	sp,*$12
! 438 		else
! 439 			vga_to_mempl4_blit(dstpsd,dstx,dsty,w,h,
jmp .5
.3:
! 440 srcpsd,srcx,srcy,op);
push	$14[bp]
push	$12[bp]
push	$10[bp]
push	$E[bp]
push	$C[bp]
push	$A[bp]
push	8[bp]
push	6[bp]
push	4[bp]
call	_vga_to_mempl4_blit
add	sp,*$12
! 441 	} else {
.5:
jmp .6
.1:
! 442 		if (dstvga)
mov	ax,-8[bp]
test	ax,ax
je  	.7
.8:
! 443 			mempl4_to_vga_blit(dstpsd,dstx,dsty,w,h,
! 444 srcpsd,srcx,srcy,op);
push	$14[bp]
push	$12[bp]
push	$10[bp]
push	$E[bp]
push	$C[bp]
push	$A[bp]
push	8[bp]
push	6[bp]
push	4[bp]
call	_mempl4_to_vga_blit
add	sp,*$12
! 445 		else
! 446 			mempl4_to_mempl4_blit(dstpsd,dstx,dsty,w,h,
jmp .9
.7:
! 447 srcpsd,srcx,srcy,op);
push	$14[bp]
push	$12[bp]
push	$10[bp]
push	$E[bp]
push	$C[bp]
push	$A[bp]
push	8[bp]
push	6[bp]
push	4[bp]
call	_mempl4_to_mempl4_blit
add	sp,*$12
! 448 	}
.9:
! 449 
! 450 }
.6:
add	sp,*4
pop	si
pop	di
pop	bp
ret
! 451 
.data
.bss

! 0 errors detected
