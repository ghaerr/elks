// Fast 32-bit combined divide and modulo routine
//
// unsigned long __divmod(unsigned long val, unsigned int *baserem)
//  Unsigned divide 32-bits by 16-bits
//      Store denominator in *baserem before calling
//      Returns 32-bit quotient in DX:AX and remainder in *baserem
//
//  Designed for a fast replacement of the following code which calls __udivsi3/__umodsi3:
//      unsigned int rem, base;
//      rem = val % base;
//      val = val / base;
//  New code:
//      rem = base;
//      val = __divmod(val, &rem);
//
//  inspired by OpenWatcom ltoa.c __uldiv routine
//  13 Sep 2024 Greg Haerr

#define NUMLO   2
#define NUMHI   4
#define ADDR    6

        .arch   i8086, nojumps
        .code16
        .text

        .global __divmod
__divmod:
        mov     %sp,%bx
        mov     NUMLO(%bx),%ax
        mov     NUMHI(%bx),%dx
        mov     ADDR(%bx),%bx

//  divides DX:AX / [BX]
//  returns DX:AX with remainder in [BX]

        xor     %cx,%cx             // temp CX = 0
        cmp     (%bx),%dx           // is upper 16 bits numerator less than denominator
        jb      1f                  // yes - only one DIV needed
        xchg    %dx,%ax             // AX = upper numerator, DX = lower numerator
        xchg    %dx,%cx             // DX = 0, CX = lower numerator
        divw    (%bx)               // AX = upper numerator / base, DX = remainder
        xchg    %cx,%ax             // AX = lower numerator, CX = high quotient
1:      divw    (%bx)               // AX = lower numerator / base, DX = remainder
        mov     %dx,(%bx)           // store remainder
        mov     %cx,%dx             // DX = high quotient, AX = low quotient
        ret
