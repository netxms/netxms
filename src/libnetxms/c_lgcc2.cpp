/*
** Allow gcc 2.x programs to link without libgcc and company
**
** Copyright (c) 2005 Victor Kirhenshtein
**
*/

#include <stdio.h>
#include <stdlib.h>

extern "C"
{
   void * __builtin_new(int memsize);
   void __builtin_delete(void *ptr);
}

void * __builtin_new(int memsize)
{
   return malloc(memsize);
}

void __builtin_delete(void *ptr)
{
   if (ptr != NULL)
      free(ptr);
}
