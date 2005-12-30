/* 
** NetXMS - Network Management System
** NetXMS Scripting Language Interpreter
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
** $module: functions.cpp
**
**/

#include "libnxsl.h"
#include <math.h>


//
// Absolute value
//

int F_abs(int argc, NXSL_Value **argv, NXSL_Value **ppResult)
{
   int nRet;

   if (argv[0]->IsNumeric())
   {
      if (argv[0]->IsReal())
         *ppResult = new NXSL_Value(fabs(argv[0]->GetValueAsReal()));
      else
         *ppResult = new NXSL_Value(abs(argv[0]->GetValueAsInt()));
      nRet = 0;
   }
   else
   {
      nRet = NXSL_ERR_NOT_NUMBER;
   }
   return nRet;
}


//
// Calculates x raised to the power of y
//

int F_pow(int argc, NXSL_Value **argv, NXSL_Value **ppResult)
{
   int nRet;

   if ((argv[0]->IsNumeric()) && (argv[1]->IsNumeric()))
   {
      *ppResult = new NXSL_Value(pow(argv[0]->GetValueAsReal(), argv[1]->GetValueAsReal()));
      nRet = 0;
   }
   else
   {
      nRet = NXSL_ERR_NOT_NUMBER;
   }
   return nRet;
}
