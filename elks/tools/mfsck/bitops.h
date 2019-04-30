/*
 * Copyright (C) 2005 - Alejandro Liu Ly <alejandro_liu@hotmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef __i386__

#define bitop(name,op) \
static inline int name(char * addr,unsigned int nr) \
{ \
int __res; \
__asm__ __volatile__("bt" op " %1,%2; adcl $0,%0" \
:"=g" (__res) \
:"r" (nr),"m" (*(addr)),"0" (0)); \
return __res; \
}

bitop(bit,"")
bitop(setbit,"s")
bitop(clrbit,"r")

#elif defined(__mc68000__)

#define bitop(name,op) \
static inline int name (char *addr, unsigned int nr) \
{ \
       char __res; \
       __asm__ __volatile__("bf" op " %2@{%1:#1}; sne %0" \
                            : "=d" (__res) \
                            : "d" (nr ^ 15), "a" (addr)); \
       return __res != 0; \
}

bitop (bit, "tst")
bitop (setbit, "set")
bitop (clrbit, "clr")

#else
static inline int bit(char * addr,unsigned int nr) 
{
  return (addr[nr >> 3] & (1<<(nr & 7))) != 0;
}

static inline int setbit(char * addr,unsigned int nr)
{
  int __res = bit(addr, nr);
  addr[nr >> 3] |= (1<<(nr & 7));
  return __res != 0; \
}

static inline int clrbit(char * addr,unsigned int nr)
{
  int __res = bit(addr, nr);
  addr[nr >> 3] &= ~(1<<(nr & 7));
  return __res != 0;
}

#endif

