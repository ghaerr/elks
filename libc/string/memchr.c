#include <string.h>

#ifdef BCC_ASM
#asm
  mov   bx,sp
  push  di
  push  es
  push  ds
  pop   es
  cld
  mov   di,[bx+2]
  mov   ax,[bx+4]
  mov   cx,[bx+6]
  test  cx,cx
  je    is_z    ! Zero length, do not find.
  repne         ! Scan
  scasb
  jne   is_z    ! Not found, ret zero
  dec   di      ! Adjust ptr
  mov   ax,di   ! return
  jmp   xit
is_z:
  xor   ax,ax
xit:
  pop   es
  pop   di
  ret
#endif

void *
memchr(const void * str, int c, size_t l)
{
   unsigned char *p = (unsigned char *)str;
   while (l-- > 0) {
      if (*p == c) return p;
      p++;
   }
   return 0;
}
