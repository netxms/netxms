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

#endif /* __sun */
