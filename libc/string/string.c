/* Copyright (C) 1995,1996 Robert de Bath <rdebath@cix.compulink.co.uk>
 * This file is part of the Linux-8086 C library and is distributed
 * under the GNU Library General Public License.
 */

#include <string.h>
#include <malloc.h>

#ifdef __AS386_16__
#if __FIRST_ARG_IN_AX__
#define BCC_AX_ASM	/* BCC Assembler that can cope with arg in AX  */
#else
#define BCC_AX_ASM
#define BCC_ASM		/* Use 16 bit BCC assembler */
#endif

#define PARANOID	/* Include extra code for cld and ES register */
#endif

/* This is a basic string package; it includes the most used functions

   strlen strcat strcpy strcmp strncat strncpy strncmp strchr strrchr strdup
   memcpy memccpy memchr memset memcmp memmove

   These functions are in seperate files.
    strpbrk.o strsep.o strstr.o strtok.o strcspn.o
    strspn.o strcasecmp.o strncasecmp.o
 */

/********************** Function strlen ************************************/

size_t strlen(const char * str)
{
#ifdef BCC_AX_ASM
#asm
#if !__FIRST_ARG_IN_AX__
  mov	bx,sp
#endif
  push	di

#ifdef PARANOID
  push	es
  push	ds	! Im not sure if this is needed, so just in case.
  pop	es
  cld
#endif
		! This is almost the same as memchr, but it can
		! stay as a special.

#if __FIRST_ARG_IN_AX__
  mov	di,ax
#else
  mov	di,[bx+2]
#endif
  mov	cx,#-1
  xor	ax,ax
  repne
  scasb
  not	cx
  dec	cx
  mov	ax,cx

#ifdef PARANOID
  pop	es
#endif
  pop	di
#endasm
#else
   register char * p =(char *) str;
   while(*p) p++;
   return p-str;
#endif  /* ifdef BCC_AX_ASM */
}

/********************** Function strcat ************************************/

char * strcat(char *d, const char * s)
{
   (void) strcpy(d+strlen(d), s);
   return d;
}

/********************** Function strcpy ************************************/

char * strcpy(d, s)
char *d;
const char * s;
{
   /* This is probably the quickest on an 8086 but a CPU with a cache will
    * prefer to do this in one pass */
   return memcpy(d, s, strlen(s)+1);
}

/********************** Function strcmp ************************************/

int strcmp(const char *d, const char * s)
{
  /* There are a number of ways to do this and it really does depend on the
     types of strings given as to which is better, nevertheless the Glib
     method is quite reasonable so we'll take that */

#ifdef BCC_AX_ASM
#asm
  mov	bx,sp
  push	di
  push	si

#ifdef PARANOID
  push	es
  push	ds	; Im not sure if this is needed, so just in case.
  pop	es
  cld
#endif

#if __FIRST_ARG_IN_AX__
  mov	di,ax		; dest
  mov	si,[bx+2]	; source
#else
  mov	di,[bx+2]	; dest
  mov	si,[bx+4]	; source
#endif
sc_1:
  lodsb
  scasb
  jne	sc_2		; If bytes are diff skip out.
  testb	al,al
  jne	sc_1		; If this byte in str1 is nul the strings are equal
  xor	ax,ax		; so return zero
  jmp	sc_3
sc_2:
  cmc
  sbb	ax,ax		; Collect correct val (-1,1).
  orb	al,#1
sc_3:

#ifdef PARANOID
  pop	es
#endif
  pop	si
  pop	di
#endasm
#else /* ifdef BCC_AX_ASM */
   register char *s1=(char *)d, *s2=(char *)s, c1,c2;
   while((c1= *s1++) == (c2= *s2++) && c1 );
   return c1 - c2;
#endif /* ifdef BCC_AX_ASM */
}

/********************** Function strncat ************************************/

char * strncat(char *d, char *s, size_t l)
{
   register char *s1=d+strlen(d), *s2;
   
   s2 = memchr(s, l, 0);
   if( s2 )
      memcpy(s1, s, s2-s+1);
   else
   {
      memcpy(s1, s, l);
      s1[l] = '\0';
   }
   return d;
}

//-----------------------------------------------------------------------------

char * strncpy (char * d, char * s, size_t l)
{
   register char *s1=d, *s2=s;
   while(l > 0)
   {
      l--;
      if( (*s1++ = *s2++) == '\0')
         break;
   }

   /* This _is_ correct strncpy is supposed to zap */
   for(; l>0; l--) *s1++ = '\0';
   return d;
}

//-----------------------------------------------------------------------------

int strncmp (const char * d, const char * s, size_t l)
{
#ifdef BCC_AX_ASM
#asm
  mov	bx,sp
  push	si
  push	di

#ifdef PARANOID
  push	es
  push	ds	! Im not sure if this is needed, so just in case.
  pop	es
  cld
#endif

#if __FIRST_ARG_IN_AX__
  mov	si,ax
  mov	di,[bx+2]
  mov	cx,[bx+4]
#else
  mov	si,[bx+2]	! Fetch
  mov	di,[bx+4]
  mov	cx,[bx+6]
#endif

  inc	cx
lp1:
  dec	cx
  je	lp2
  lodsb
  scasb
  jne	lp3
  testb	al,al
  jne	lp1
lp2:
  xor	ax,ax
  jmp	lp4
lp3:
  sbb	ax,ax
  or	al,#1
lp4:

#ifdef PARANOID
  pop	es
#endif
  pop	di
  pop	si
#endasm
#else
   register char c1=0, c2=0;
   while(l-- >0)
      if( (c1= *d++) != (c2= *s++) || c1 == '\0' )
         break;
   return c1-c2;
#endif
}

/********************** Function strchr ************************************/

char * strchr (const char * s, int c)
{
#ifdef BCC_AX_ASM
#asm
  mov	bx,sp
  push	si
#if __FIRST_ARG_IN_AX__
  mov	bx,[bx+2]
  mov	si,ax
#else
  mov	si,[bx+2]
  mov	bx,[bx+4]
#endif
  xor	ax,ax

#ifdef PARANOID
  cld
#endif

in_loop:
  lodsb
  cmp	al,bl
  jz	got_it
  or	al,al
  jnz	in_loop
  pop	si
  ret
got_it:
  lea	ax,[si-1]
  pop	si

#endasm
#else /* ifdef BCC_AX_ASM */
   register char ch;
   for(;;)
   {
     if( (ch= *s) == c ) return s;
     if( ch == 0 ) return 0;
     s++;
   }
#endif /* ifdef BCC_AX_ASM */
}

//-----------------------------------------------------------------------------

char * strrchr (const char * s, int c)
{
   register char * prev = 0;
   register char * p = s;
   /* For null it's just like strlen */
   if( c == '\0' ) return p+strlen(p);

   /* everything else just step along the string. */
   while( (p=strchr(p, c)) != 0 )
   {
      prev = p; p++;
   }
   return prev;
}

/********************** Function strdup ************************************/

char * strdup(s)
char * s;
{
   register size_t len;
   register char * p;

   len = strlen(s)+1;
   p = (char *) malloc(len);
   if(p) memcpy(p, s, len); /* Faster than strcpy */
   return p;
}

/********************** Function memcpy ************************************/

void *memcpy(void *d, const void *s, size_t l)
{
#ifdef BCC_AX_ASM
#asm
  mov	bx,sp
  push	di
  push	si

#ifdef PARANOID
  push	es
  push	ds	; Im not sure if this is needed, so just in case.
  pop	es
  cld
#endif

#if __FIRST_ARG_IN_AX__
  mov	di,ax		; dest
  mov	si,[bx+2]	; source
  mov	cx,[bx+4]	; count
#else
  mov	di,[bx+2]	; dest
  mov	si,[bx+4]	; source
  mov	cx,[bx+6]	; count

  mov	ax,di
#endif
  		; If di is odd we could mov 1 byte before doing word move
		; as this would speed the copy slightly but its probably
		; too rare to be worthwhile.
		; NB 8086 has no problem with mis-aligned access.

  shr	cx,#1	; Do this faster by doing a mov word
  rep
  movsw
  adc	cx,cx	; Retrieve the leftover 1 bit from cflag.
  rep
  movsb

#ifdef PARANOID
  pop	es
#endif
  pop	si
  pop	di
#endasm
#else /* ifdef BCC_AX_ASM */
   register char *s1=d, *s2=(char *)s;
   for( ; l>0; l--) *((unsigned char*)s1++) = *((unsigned char*)s2++);
   return d;
#endif /* ifdef BCC_AX_ASM */
}

/********************** Function memccpy ************************************/

#ifdef L_memccpy
void * memccpy(d, s, c, l)	/* Do we need a fast one ? */
void *s, *d;
int c;
size_t l;
{
   register char *s1=d, *s2=s;
   while(l-- > 0)
      if((*s1++ = *s2++) == c )
         return s1;
   return 0;
}
#endif

/********************** Function memchr ************************************/

void * memchr(const void * str, int c, size_t l)
{
#ifdef BCC_ASM
#asm
  mov	bx,sp
  push	di

#ifdef PARANOID
  push	es
  push	ds	; Im not sure if this is needed, so just in case.
  pop	es
  cld
#endif

  mov	di,[bx+2]
  mov	ax,[bx+4]
  mov	cx,[bx+6]
  test	cx,cx
  je	is_z	! Zero length, do not find.

  repne		! Scan
  scasb
  jne	is_z	! Not found, ret zero
  dec	di	! Adjust ptr
  mov	ax,di	! return
  jmp	xit
is_z:
  xor	ax,ax
xit:

#ifdef PARANOID
  pop	es
#endif
  pop	di
#endasm
#else /* ifdef BCC_ASM */
   register char *p=(char *)str;
   while(l-- > 0)
   {
      if(*p == c) return p;
      p++;
   }
   return 0;
#endif /* ifdef BCC_ASM */
}

//-----------------------------------------------------------------------------

void * memset (void * str, int c, size_t l)
{
#ifdef BCC_AX_ASM
#asm
  mov	bx,sp
  push	di

#ifdef PARANOID
  push	es
  push	ds	; Im not sure if this is needed, so just in case.
  pop	es
  cld
#endif

#if __FIRST_ARG_IN_AX__
  mov	di,ax		; Fetch
  mov	ax,[bx+2]
  mov	cx,[bx+4]
#else
  mov	di,[bx+2]	; Fetch
  mov	ax,[bx+4]
  mov	cx,[bx+6]
#endif

; How much difference does this alignment make ?
; I don`t think it`s significant cause most will already be aligned.

;  test	cx,cx		; Zero size - skip
;  je	xit
;
;  test	di,#1		; Line it up
;  je	s_1
;  stosb
;  dec	cx
;s_1:

  mov	ah,al		; Replicate byte
  shr	cx,#1		; Do this faster by doing a sto word
  rep			; Bzzzzz ...
  stosw
  adc	cx,cx		; Retrieve the leftover 1 bit from cflag.

  rep			; ... z
  stosb

xit:
  mov	ax,[bx+2]
#ifdef PARANOID
  pop	es
#endif
  pop	di
#endasm
#else /* ifdef BCC_AX_ASM */
   register char *s1=str;
   while(l-->0) *s1++ = c;
   return str;
#endif /* ifdef BCC_AX_ASM */
}

/********************** Function memcmp ************************************/

int memcmp(const void *s, const void *d, size_t l)
{
#ifdef BCC_ASM
#asm
  mov	bx,sp
  push	di
  push	si

#ifdef PARANOID
  push	es
  push	ds	! Im not sure if this is needed, so just in case.
  pop	es
  cld
#endif

  mov	si,[bx+2]	! Fetch
  mov	di,[bx+4]
  mov	cx,[bx+6]
  xor	ax,ax

  rep			! Bzzzzz
  cmpsb
  je	xit		! All the same!
  sbb	ax,ax
  sbb	ax,#-1		! choose +/-1
xit:
#ifdef PARANOID
  pop	es
#endif
  pop	si
  pop	di
#endasm
#else /* ifdef BCC_ASM */
   register const char *s1=d, *s2=s;
   register char c1=0, c2=0;
   while(l-- > 0)
      if( (c1= *s1++) != (c2= *s2++) )
         break;
   return c1-c2;
#endif /* ifdef BCC_ASM */
}

/********************** Function memmove ************************************/

void *
memmove(d, s, l)
void *d, *s;
size_t l;
{
   register char *s1=d, *s2=s;
   /* This bit of sneakyness c/o Glibc, it assumes the test is unsigned */
   if( s1-s2 >= l ) return memcpy(d,s,l);

   /* This reverse copy only used if we absolutly have to */
   s1+=l; s2+=l;
   while(l-- >0)
      *(--s1) = *(--s2);
   return d;
}

/********************** Function movedata ***********************************/

#ifdef L_movedata

/* NB There isn't any C version of this function ... */

#ifdef BCC_AX_ASM
void
__movedata(srcseg, srcoff, destseg, destoff, len)
unsigned int srcseg, srcoff, destseg, destoff, len;
{
#asm
  push	bp
  mov	bp,sp
  push	si
  push	di
  push	ds
#ifdef PARANOID
  push	es
  cld
#endif

  ! sti			! Are we _really_ paranoid ?

#if !__FIRST_ARG_IN_AX__
  mov	ds,[bp+4]	! Careful, [bp+xx] is SS based.
  mov	si,[bp+6]
  mov	es,[bp+8]
  mov	di,[bp+10]
  mov	cx,[bp+12]
#else
  mov	ds,ax
  mov	si,[bp+4]
  mov	es,[bp+6]
  mov	di,[bp+8]
  mov	cx,[bp+10]
#endif

  ; Would it me a good idea to normalise the pointers ?
  ; How about allowing for overlapping moves ?

  shr	cx,#1	; Do this faster by doing a mov word
  rep
  movsw
  adc	cx,cx	; Retrieve the leftover 1 bit from cflag.
  rep
  movsb

  ! cli			! Are we _really_ paranoid ?
  
#ifdef PARANOID
  pop	es
#endif
  pop	ds
  pop	di
  pop	si
  pop	bp
#endasm
}
#endif

#endif

/********************** THE END ********************************************/

