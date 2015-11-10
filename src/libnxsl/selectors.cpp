/*
** NetXMS - Network Management System
** NetXMS Scripting Language Interpreter
** Copyright (C) 2003-2015 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation; either version 3 of the License, or
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
** File: selectors.cpp
**
**/

#include "libnxsl.h"

/**
 * Select max value
 */
int S_max(const TCHAR *name, NXSL_Value *options, int argc, NXSL_Value **argv, int *selection, NXSL_VM *vm)
{
   if (!argv[0]->isNumeric())
      return NXSL_ERR_NOT_NUMBER;

   double val = argv[0]->getValueAsReal();
   int index = 0;
   for(int i = 1; i < argc; i++)
   {
      if (!argv[0]->isNumeric())
         return NXSL_ERR_NOT_NUMBER;
      if (argv[i]->getValueAsReal() > val)
      {
         index = i;
         val = argv[i]->getValueAsReal();
      }
   }
   *selection = index;
   return 0;
}

/**
 * Select min value
 */
int S_min(const TCHAR *name, NXSL_Value *options, int argc, NXSL_Value **argv, int *selection, NXSL_VM *vm)
{
   if (!argv[0]->isNumeric())
      return NXSL_ERR_NOT_NUMBER;

   double val = argv[0]->getValueAsReal();
   int index = 0;
   for(int i = 1; i < argc; i++)
   {
      if (!argv[0]->isNumeric())
         return NXSL_ERR_NOT_NUMBER;
      if (argv[i]->getValueAsReal() < val)
      {
         index = i;
         val = argv[i]->getValueAsReal();
      }
   }
   *selection = index;
   return 0;
}
