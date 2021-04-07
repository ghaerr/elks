; Exomizer2 algorithm, backward with litterals for 6809
; by Fool-DupleX, PrehisTO and Sam from PULS (www.pulsdemos.com)
; edouard@forler.ch
; This routine decrunches data compressed with Exomizer2 in raw mode, backward
; with litterals.
; This routine was developed and tested on a Thomson MO5 in July 2011.

; Please direct any questions about this decruncher to edouard@forler.ch

; The Exomizer2 decruncher starts here.
; call with a JSR exo2 or equivalent.
;
; Input    : U = pointer to last byte of compressed data
;            Y = pointer to last byte of output buffer
; Output   : Y = pointer to first byte of decompressed data
; Modifies : Y.
;
; All registers are preserved except Y.
; This code self modifies and cannot be run in ROM.
; This code must be contained within a single page (makes use of DP), but may
; be located anywhere in RAM.

exo2    pshs    u,y,x,dp,d,cc           ; Save context
        tfr     pc,d                    ; Set direct page
        tfr     a,dp
        lda     ,u                      ; Save first byte of data
        sta     <bitbuf+1
        leay    biba,pcr                ; Set ptr to bits and base table

        clrb
nxt     clra
        pshs    a,b
        bitb    #$0f                    ; if (i&15==0)
        bne     skp
        ldx     #$0001                  ; b2 = 1

skp     ldb     #4                      ; Fetch 4 bits
        bsr     getbits
        stb     ,y+                     ; bits[i] = b1
        comb                            ; CC=1
roll    rol     ,s
        rola
        incb
        bmi     roll
        ldb     ,s
        stx     ,y++                    ; base[i] = b2
        leax    d,x                     ; b2 += accu1
        puls    a,b
        incb
        cmpb    #52                     ; 52 times ?
        bne     nxt

go      ldy     6,s                     ; Y = ptr to output
mloop   ldb     #1                      ; for(;;)
        bsr     getbits                 ; Fetch 1 bit
        bne     cpy                     ; is 1 ?
        stb     <idx+1                  ; B always 0 here
        fcb     $8c                     ; (CMPX) to skip first iteration
rbl     inc     <idx+1                  ; Compute index
        incb
        bsr     getbits
        beq     rbl

idx     ldb     #$00                    ; Self-modified code
        cmpb    #$10                    ; index = 16 ?
        beq     endr
        blo     coffs                   ; index < 16 ?
        decb                            ; index = 17
        bsr     getbits                 ; Get size

cpy     tfr     d,x                     ; Copy litteral
cpyl    lda     ,-u
        sta     ,-y
        leax    -1,x
        bne     cpyl
        bra     mloop

coffs   bsr     cook                    ; Compute length
        pshs    d
        leax    <tab1,pcr
        cmpd    #$03
        bhs     scof
        abx
scof    ldb     ,x
        bsr     getbits
        addb    3,x
        bsr     cook
        std     <offs+2
        puls    x

cpy2    leay    -1,y                    ; Copy non litteral
offs    lda     $1234,y                 ; Self-modified code
        sta     ,y
        leax    -1,x
        bne     cpy2
        bra     mloop

endr    sty     6,s                     ; End
        puls    cc,d,dp,x,y,u,pc        ; Restore context and set Y

; getbits  : get 0 to 16 bits from input stream
; Input    : B = bit count, U points to input buffer
; Output   : D = bits
; Modifies : D,U.

getbits clr     ,-s                     ; Clear local bits
        clr     ,-s
bitbuf  lda     #$01                    ; Self-modified code
        bra     get3
get1    lda     ,-u
get2    rora
        beq     get1                    ; Bit buffer = 1 ?
        rol     1,s
        rol     ,s
get3    decb
        bpl     get2
        sta     <bitbuf+1               ; Save buffer
        ldd     ,s++                    ; Retrieve bits
        rts

; cook     : computes base[index] + readbits(&in, bits[index])
; Input    : B = index
; Output   : D = base[index] + readbits(&in, bits[index])
; Modifies : D,X,U.

cook    leax    biba,pcr
        abx                             ; bits+base = 3 bytes
        aslb                            ; times 2
        abx
        ldb     ,x                      ; fetch bits[index]
        bsr     getbits                 ; readbits
        addd    1,x                     ; add base[index]
        rts

; values used in the switch (index)
tab1    fcb     4,2,4
tab2    fcb     16,48,32

biba    rmb     156                     ; Bits and base are interleaved
