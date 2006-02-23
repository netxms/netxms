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
// Global data
//

char *g_szTypeNames[] = { "null", "object", "string", "real", "int32",
                          "int64", "uint32", "uint64" };



//
// Type of value
//

int F_typeof(int argc, NXSL_Value **argv, NXSL_Value **ppResult)
{
   *ppResult = new NXSL_Value(g_szTypeNames[argv[0]->DataType()]);
   return 0;
}


//
// Absolute value
//

int F_abs(int argc, NXSL_Value **argv, NXSL_Value **ppResult)
{
   int nRet;

   if (argv[0]->IsNumeric())
   {
      if (argv[0]->IsReal())
      {
         *ppResult = new NXSL_Value(fabs(argv[0]->GetValueAsReal()));
      }
      else
      {
         *ppResult = new NXSL_Value(argv[0]);
         if (!argv[0]->IsUnsigned())
            if ((*ppResult)->GetValueAsInt64() < 0)
               (*ppResult)->Negate();
      }
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


//
// Convert string to uppercase
//

int F_upper(int argc, NXSL_Value **argv, NXSL_Value **ppResult)
{
   int nRet;
   DWORD i, dwLen;
   char *pStr;

   if (argv[0]->IsString())
   {
      *ppResult = new NXSL_Value(argv[0]);
      pStr = (*ppResult)->GetValueAsString(&dwLen);
      for(i = 0; i < dwLen; i++, pStr++)
         *pStr = toupper(*pStr);
      nRet = 0;
   }
   else
   {
      nRet = NXSL_ERR_NOT_STRING;
   }
   return nRet;
}


//
// Convert string to lowercase
//

int F_lower(int argc, NXSL_Value **argv, NXSL_Value **ppResult)
{
   int nRet;
   DWORD i, dwLen;
   char *pStr;

   if (argv[0]->IsString())
   {
      *ppResult = new NXSL_Value(argv[0]);
      pStr = (*ppResult)->GetValueAsString(&dwLen);
      for(i = 0; i < dwLen; i++, pStr++)
         *pStr = tolower(*pStr);
      nRet = 0;
   }
   else
   {
      nRet = NXSL_ERR_NOT_STRING;
   }
   return nRet;
}


//
// String length
//

int F_length(int argc, NXSL_Value **argv, NXSL_Value **ppResult)
{
   int nRet;
   DWORD dwLen;

   if (argv[0]->IsString())
   {
      argv[0]->GetValueAsString(&dwLen);
      *ppResult = new NXSL_Value(dwLen);
      nRet = 0;
   }
   else
   {
      nRet = NXSL_ERR_NOT_STRING;
   }
   return nRet;
}


//
// Check if IP address is within given range
//

int F_AddrInRange(int argc, NXSL_Value **argv, NXSL_Value **ppResult)
{
   int nRet;
   DWORD dwAddr, dwStart, dwEnd;

   if (argv[0]->IsString() && argv[1]->IsString() && argv[2]->IsString())
   {
      dwAddr = ntohl(inet_addr(argv[0]->GetValueAsCString()));
      dwStart = ntohl(inet_addr(argv[1]->GetValueAsCString()));
      dwEnd = ntohl(inet_addr(argv[2]->GetValueAsCString()));
      *ppResult = new NXSL_Value((LONG)(((dwAddr >= dwStart) && (dwAddr <= dwEnd)) ? 1 : 0));
      nRet = 0;
   }
   else
   {
      nRet = NXSL_ERR_NOT_STRING;
   }
   return nRet;
}


//
// Check if IP address is within given subnet
//

int F_AddrInSubnet(int argc, NXSL_Value **argv, NXSL_Value **ppResult)
{
   int nRet;
   DWORD dwAddr, dwSubnet, dwMask;

   if (argv[0]->IsString() && argv[1]->IsString() && argv[2]->IsString())
   {
      dwAddr = ntohl(inet_addr(argv[0]->GetValueAsCString()));
      dwSubnet = ntohl(inet_addr(argv[1]->GetValueAsCString()));
      dwMask = ntohl(inet_addr(argv[2]->GetValueAsCString()));
      *ppResult = new NXSL_Value((LONG)(((dwAddr & dwMask) == dwSubnet) ? 1 : 0));
      nRet = 0;
   }
   else
   {
      nRet = NXSL_ERR_NOT_STRING;
   }
   return nRet;
}
