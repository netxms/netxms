/* 
** NetXMS - Network Management System
** NetXMS MIB compiler
** Copyright (C) 2005 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** $module: mibparse.cpp
**
**/

#include "mibparse.h"


//
// Create dynamic array
//

DynArray *da_create(void)
{
   DynArray *pArray;

   pArray = (DynArray *)malloc(sizeof(DynArray));
   if (pArray != NULL)
   {
      pArray->nSize = 0;
      pArray->ppData = NULL;
   }
   return pArray;
}


//
// Add element to array
//

void da_add(DynArray *pArray, void *pElement)
{
   if (pArray != NULL)
   {
      pArray->ppData = (void **)realloc(pArray->ppData, sizeof(void *) * (pArray->nSize + 1));
      pArray->ppData[pArray->nSize++] = pElement;
   }
}


//
// Get element by index
//

void *da_get(DynArray *pArray, int nIndex)
{
   if (pArray != NULL)
   {
      if (nIndex < pArray->nSize)
         return pArray->ppData[nIndex];
   }
   return NULL;
}


//
// Get array size
//

int da_size(DynArray *pArray)
{
   return (pArray != NULL) ? pArray->nSize : 0;
}


//
// Destroy array
//

void da_destroy(DynArray *pArray)
{
   if (pArray != NULL)
   {
      safe_free(pArray->ppData);
      free(pArray);
   }
}


//
// Join one array to another
//

void da_join(DynArray *pArray, DynArray *pSource)
{
   if ((pArray != NULL) && (pSource != NULL))
   {
      pArray->ppData = (void **)realloc(pArray->ppData, sizeof(void *) * (pArray->nSize + pSource->nSize));
      memcpy(&pArray->ppData[pArray->nSize], pSource->ppData, sizeof(void *) * pSource->nSize);
      pArray->nSize += pSource->nSize;
   }
}
