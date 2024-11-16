#include <string.h>

#ifdef BCC_ASM
  mov   bx,sp
  push  si
  push  di
  push  es
  push  ds
  pop   es
  cld
  mov   si,[bx+2]       ! Fetch
  mov   di,[bx+4]
  mov   cx,[bx+6]
  inc   cx
lp1:
  dec   cx
  je    lp2
  lodsb
  scasb
  jne   lp3
  testb al,al
  jne   lp1
lp2:
  xor   ax,ax
  jmp   lp4
lp3:
  sbb   ax,ax
  or    al,#1
lp4:
  pop   es
  pop   di
  pop   si
  ret
#endif

int
strncmp(const char * d, const char * s, size_t l)
{
   char c1 = 0, c2 = 0;
   while (l-- > 0) {
      if ((c1 = *d++) != (c2 = *s++) || c1 == '\0')
         break;
   }
   return c1 - c2;
}
