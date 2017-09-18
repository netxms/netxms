/* 
** NetXMS - Network Management System
** NetXMS Foundation Library
** Copyright (C) 2003-2013 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published
** by the Free Software Foundation; either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: solaris9_atomic.c
**
**/

#include "libnetxms.h"

#ifdef __sun

#if !HAVE_ATOMIC_INC_32_NV

volatile uint32_t solaris9_atomic_inc32(volatile uint32_t *v)
{
   volatile uint32_t t1, t2;
   asm(
      "       ld       [%2], %0;"
      "1:     add      %0, 1, %1;"
      "       cas      [%2], %0, %1;"
      "       cmp      %0, %1;"
      "       bne,a,pn %icc, 1b;"
      "       mov      %1, %0;"
         : "=&r" (t1), "=&r" (t2) : "r" (v) : "memory");
   return t1 + 1;
}

#endif

#if !HAVE_ATOMIC_DEC_32_NV

volatile uint32_t solaris9_atomic_dec32(volatile uint32_t *v)
{
   volatile uint32_t t1, t2;
   asm(
      "       ld       [%2], %0;"
      "1:     sub      %0, 1, %1;"
      "       cas      [%2], %0, %1;"
      "       cmp      %0, %1;"
      "       bne,a,pn %icc,1b;"
      "       mov      %1, %0;"
         : "=&r" (t1), "=&r" (t2) : "r" (v) : "memory");
   return t1 + 1;
}

#endif

#if !HAVE_ATOMIC_SWAP_PTR

void *solaris9_atomic_swap_ptr(volatile *void *target, void *value)
{
   volatile void *t;
   asm(
      "       ld       [%2], %0;"
      "1:     cas      [%2], %0, %1;"
      "       cmp      %0, %1;"
      "       bne,a,pn %icc,1b;"
      "       mov      %1, %0;"
         : "=&r" (value), "=&r" (t) : "r" (target) : "memory");
   return t;
}

#endif

#endif /* __sun */
