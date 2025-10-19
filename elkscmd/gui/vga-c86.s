; Routines to draw pixels and lines for EGA/VGA 16 color 4 planes modes.
; High speed version for C86 using AS86 assembly
; Supports  the following EGA and VGA 16 color modes:
;       640x480 16 color (mode 0x12)
;       640x350 16 color (mode 0x10)
;
; The algorithms for some of these routines are taken from the book:
; Programmer's Guide to PC and PS/2 Video Systems by Richard Wilton.
;
; Copyright (c) 1999 Greg Haerr <greg@censoft.com>
; Copyright (c) 1991 David I. Bell
; Permission is granted to use, distribute, or modify this source,
; provided that this copyright notice remains intact.
;
; 1 Feb 2025 Greg Haerr rewritten for ELKS AS86
;
        use16   86
        .text
BYTESPERLN  = 80                ; number of bytes in scan line (640/8)
arg1        = 4                 ; small model

;
; void vga_init(void)
;
; C version:
;   set_enable_sr(0xff);
;   set_op(0);
;   set_write_mode(0);
;
    .global _vga_init
_vga_init:
        mov     dx, #0x03ce     ; graphics controller port address
        mov     ax, #0xff01     ; set enable set/reset register 1 mask FF
        out     dx, ax
        mov     ax, #0x0003     ; data rotate register 3 NOP 0
        out     dx, ax
        mov     ax, #0x0005     ; set graphics mode register 5 write mode 0
        out     dx, ax          ; [load value 0 into mode register 5]
        ret

;
; Draw an individual pixel.
; void vga_drawpixel(int x, int y, int color);
;
; C version:
;   static unsigned char mask[8] = { 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01 };
;   set_color(c);
;   set_mask(mask[x&7]);
;
        .global  _vga_drawpixel
_vga_drawpixel:
        push    bp
        mov     bp, sp

        mov     cx, arg1[bp]    ; CX := x
        mov     bx, cx          ; BX := x
        mov     ax, arg1+2[bp]  ; AX := y

        shl     ax, 1           ; AX := [y * 80] (= y*64 + y*16)
        shl     ax, 1
        shl     ax, 1
        shl     ax, 1
        mov     dx, ax
        shl     ax, 1
        shl     ax, 1
        add     ax, dx

        and     cl, #7          ; CL := x & 7
        xor     cl, #7          ; CL := 7 - (x & 7)
        mov     ch, #1          ; CH := 1 << (7 - (x & 7))
        shl     ch, cl          ; CH is mask

        shr     bx, 1           ; BX := x / 8
        shr     bx, 1
        shr     bx, 1
        add     bx, ax          ; BX := [y * BYTESPERLN] + [x / 8]

        mov     dx, #0x03ce     ; graphics controller port address
        xor     ax, ax          ; set color register 0
        mov     ah, arg1+4[bp]  ; color pixel value
        out     dx, ax

        mov     al, #8          ; set bit mask register 8
        mov     ah, ch          ; [load bit mask into register 8]
        out     dx, ax

        mov     cx, ds
        mov     ax, #0xA000     ; DS := EGA buffer segment address
        mov     ds, ax
        or      [bx], al        ; quick rmw to set pixel
        mov     ds, cx          ; restore registers and return

        pop     bp
        ret

;
; Draw a horizontal line from x1,y to x2,y including final point
; void vga_drawhine(int x1, int x2, int y, int color);
;
; C version:
;   set_color(c);
;   char far *dst = EGA_BASE + x1 / 8 + y * BYTESPERLN;
;   if ((x1 >> 3) == (x2 >> 3)) {
;       set_mask((0xff >> (x1 & 7)) & (0xff << (7 - (x2 & 7))));
;       *dst |= 1;
;   } else {
;       set_mask(0xff >> (x1 & 7));
;       *dst++ |= 1;
;       set_mask(0xff);
;       last = EGA_BASE + x2 / 8 + y * BYTESPERLN;
;       while (dst < last)
;           *dst++ |= 1;
;       set_mask(0xff << (7 - x2 & 7));
;       *dst |= 1;
;   }

x1      = arg1          ; first X coordinate
x2      = arg1 + 2      ; second X coordinate
y       = arg1 + 4      ; second Y coordinate
color   = arg1 + 6      ; pixel value

        .global _vga_drawhline
_vga_drawhline:
        push    bp              ; setup stack frame and preserve registers
        mov     bp, sp
        push    si
        push    di
        push    es
        push    ds

        mov     dx, #0x03ce     ; Graphics Controller port address
        xor     al, al          ; set Set/Reset register 0 with color
        mov     ah, color[bp]
        out     dx, ax

        mov     ax, #0x0f01     ; AH := bit plane mask for Enable Set/Reset
        out     dx, ax          ; AL := Enable Set/Reset register 1

        mov     ax, y[bp]
        mov     bx, x1[bp]

        ; compute pixel address
        shl     ax, 1           ; AX := [row * 80] (= row*64 + y*16)
        shl     ax, 1
        shl     ax, 1
        shl     ax, 1
        mov     dx, ax
        shl     ax, 1
        shl     ax, 1
        add     ax, dx

        mov     cl, bl          ; save low order column bits
        shr     bx, 1           ; BX := [col / 8]
        shr     bx, 1
        shr     bx, 1
        add     bx, ax          ; BX := [row * BYTESPERLN] + [col / 8]
        and     cl, #7          ; CL := [col & 7]
        xor     cl, #7          ; CL := 7 - [col & 7]
        mov     ah, #1          ; AH := 1 << [7 - [col & 7]]    [mask]
        mov     dx, #0xA000     ; ES := EGA buffer segment address
        mov     es, dx          ; ES:BX -> video buffer
                                ; AH := bit mask
                                ; CL := number bits to shift left
        mov     di, bx          ; ES:DI -> buffer
        mov     dh, ah          ; DH := unshifted bit mask for left byte

        not     dh
        shl     dh, cl          ; DH := reverse bit mask for first byte
        not     dh              ; DH := bit mask for first byte

        mov     cx, x2[bp]
        and     cl, #7
        xor     cl, #7          ; CL := number of bits to shift left
        mov     dl, #0xff       ; DL := unshifted bit mask for right byte
        shl     dl, cl          ; DL := bit mask for last byte

        ; determine byte offset of first and last pixel in the line
        mov     ax, x2[bp]      ; AX := x2
        mov     bx, x1[bp]      ; BX := x1

        shr     ax, 1           ; AX := byte offset of X2
        shr     ax, 1
        shr     ax, 1
        shr     bx, 1           ; BX := byte offset of X1
        shr     bx, 1
        shr     bx, 1
        mov     cx, ax
        sub     cx, bx          ; CX := [number of bytes in line] - 1

        ; get Graphics Controller port address into DX
        mov     bx, dx          ; BH := bit mask for first byte
                                ; BL := bit mask for last byte
        mov     dx, #0x03ce     ; DX := Graphics Controller port
        mov     al, #8          ; AL := Bit mask Register number

        ; make video buffer addressable through DS:SI
        push    es
        pop     ds
        mov     si, di          ; DS:SI -> video buffer

        ; set pixels in leftmost byte of the line
        or      bh, bh
        js      L43             ; jump if byte-aligned [x1 is leftmost]

        or      cx, cx
        jnz     L42             ; jump if more than one byte in the line

        and     bl, bh          ; BL := bit mask for the line
        jmp     L44

L42:    mov     ah, bh          ; AH := bit mask for first byte
        out     dx, ax          ; update graphics controller

        movsb                   ; update bit planes
        dec     cx

        ; use a fast 8086 machine instruction to draw the remainder of the line
L43:    mov     ah, #0xff       ; AH := bit mask
        out     dx, ax          ; update Bit Mask register
        rep
        movsb                   ; update all pixels in the line

        ; set pixels in the rightmost byte of the line
L44:    mov     ah, bl          ; AH := bit mask for last byte
        out     dx, ax          ; update Graphics Controller
        movsb                   ; update bit planes

        pop     ds
        pop     es
        pop     di
        pop     si
        pop     bp
        ret

;
; Draw a vertical line from x,y1 to x,y2 including final point
; void vga_drawvline(int x, int y1, int y2, int color);
;
; C version:
;   set_color(c);
;   set_mask(mask[x&7]);
;   char far *dst = EGA_BASE + x / 8 + y1 * BYTESPERLN;
;   char far *last = EGA_BASE + x / 8 + y2 * BYTESPERLN;
;   while (dst < last) {
;       *dst |= 1;
;       dst += BYTESPERLN;
;   }
;

x       = arg1          ; first X coordinate
y1      = arg1 + 2      ; first Y coordinate
y2      = arg1 + 4      ; second Y coordinate
color   = arg1 + 6      ; pixel value

        .global  _vga_drawvline
_vga_drawvline:
        push    bp              ; setup stack frame and preserve registers
        mov     bp, sp
        push    ds

        mov     dx, #0x03ce     ; DX := Graphics Controller port address
        xor     al, al          ; set Set/Reset register 0 with color
        mov     ah, color[bp]
        out     dx, ax

        mov     ax, #0x0f01     ; AH := bit plane mask for Enable Set/Reset
        out     dx, ax          ; AL := Enable Set/Reset register number

        ; prepare to draw vertical line
        mov     ax, y1[bp]      ; AX := y1
        mov     cx, y2[bp]      ; BX := y2
        sub     cx, ax          ; CX := dy

L311:   inc     cx              ; CX := number of pixels to draw
        mov     bx, x[bp]       ; BX := x
        push    cx              ; save register

        ; compute pixel address
        shl     ax, 1           ; AX := [row * 80] (= row*64 + row*16)
        shl     ax, 1
        shl     ax, 1
        shl     ax, 1
        mov     dx, ax
        shl     ax, 1
        shl     ax, 1
        add     ax, dx

        mov     cl, bl          ; save low order column bits
        shr     bx, 1           ; BX := [col / 8]
        shr     bx, 1
        shr     bx, 1
        add     bx, ax          ; BX := [row * BYTESPERLN] + [col / 8]
        and     cl, #7          ; CL := [col & 7]
        xor     cl, #7          ; CL := 7 - [col & 7]
        mov     ah, #1          ; AH := 1 << [7 - [col & 7]]    [mask]
        mov     dx, #0x0A000    ; DS := EGA buffer segment address
        mov     ds, dx          ; DS:BX -> video buffer
                                ; AH := bit mask
                                ; CL := number bits to shift left
        ; set up Graphics controller
        mov     dx, #0x03ce     ; DX := Graphics Controller port address
        shl     ah, cl          ; AH := bit mask in proper position
        mov     al, #8          ; AL := Bit Mask register number 8
        out     dx, ax

        pop     cx              ; restore number of pixels to draw

        ; draw the line
        mov     dx, #BYTESPERLN ; increment for video buffer
L1111:  or      [bx], al        ; set pixel
        add     bx, dx          ; increment to next line
        loop    L1111

        pop     ds
        pop     bp
        ret

;
; Read the value of an individual pixel.
; int vga_readpixel(int x, int y);
;
        .global _vga_readpixel
_vga_readpixel:
        push    bp
        mov     bp, sp
        push    si
        push    ds

        mov     ax, arg1+2[bp]  ; AX := y
        mov     bx, arg1[bp]    ; BX := x
        shl     ax, 1           ; AX := [y * 80] (= y*64 + y*16)
        shl     ax, 1
        shl     ax, 1
        shl     ax, 1
        mov     dx, ax
        shl     ax, 1
        shl     ax, 1
        add     ax, dx

        mov     cl, bl          ; save low order column bits
        shr     bx, 1           ; BX := [x / 8]
        shr     bx, 1
        shr     bx, 1
        add     bx, ax          ; BX := [y * BYTESPERLN] + [x / 8]

        and     cl, #7          ; CL := [x & 7]
        xor     cl, #7          ; CL := 7 - [x & 7]

        mov     dx, #0xA000     ; DS := EGA buffer segment address
        mov     ds, dx

        mov     ch, #1          ; CH := 1 << [7 - [col & 7]]
        shl     ch, cl          ; CH := bit mask in proper position

        mov     si, bx          ; DS:SI -> region buffer byte
        xor     bl, bl          ; BL is used to accumulate the pixel value

        mov     dx, #0x03ce     ; DX := Graphics Controller port
        mov     ax, #0x0304     ; AH := initial bit plane number 3
                                ; AL := Read Map Select register number 4

L112:   out     dx, ax          ; select bit plane
        mov     bh, [si]        ; BH := byte from current bit plane
        and     bh, ch          ; mask one bit
        neg     bh              ; bit 7 of BH := 1 if masked bit = 1
                                ; bit 7 of BH := 0 if masked bit = 0
        rol     bx, #1          ; bit 0 of BL := next bit from pixel value
        dec     ah              ; AH := next bit plane number
        jge     L112

        xor     ax, ax          ; AL := pixel value
        mov     al, bl

        pop     ds
        pop     si
        pop     bp      
        ret

; void fdstmemcpy (word_t dst_off, seg_t dst_seg, char *src_off, word_t count)
; Copy near source to far destination
; segment parameter after offset to allow LES from the stack
; NOTE: currently saves SI, DI and ES - may not be required

ARG0    = 2
ARG1    = 4
ARG2    = 6
ARG3    = 8

        .global _fdstmemcpy
_fdstmemcpy:
        mov    ax,si
        mov    dx,di
        mov    bx,es
        mov    si,sp

        mov    cx,ARG3[si]      ; byte count
        les    di,ARG0[si]      ; far destination pointer
        mov    si,ARG2[si]      ; far source pointer
        cld
        shr    cx,1             ; copy words
        rep
        movsw
        rcl    cx,1             ; then possibly final byte
        rep
        movsb

        mov    es,bx
        mov    di,dx
        mov    si,ax
        ret
