/* 
** NetXMS - Network Management System
** NetXMS Scripting Language Interpreter
** Copyright (C) 2005, 2006, 2007, 2008 Victor Kirhenshtein
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
** File: env.cpp
**
**/

#include "libnxsl.h"


//
// Externals
//

int F_abs(int argc, NXSL_Value **argv, NXSL_Value **ppResult);
int F_classof(int argc, NXSL_Value **argv, NXSL_Value **ppResult);
int F_d2x(int argc, NXSL_Value **argv, NXSL_Value **ppResult);
int F_gmtime(int argc, NXSL_Value **argv, NXSL_Value **ppResult);
int F_length(int argc, NXSL_Value **argv, NXSL_Value **ppResult);
int F_localtime(int argc, NXSL_Value **argv, NXSL_Value **ppResult);
int F_lower(int argc, NXSL_Value **argv, NXSL_Value **ppResult);
int F_max(int argc, NXSL_Value **argv, NXSL_Value **ppResult);
int F_min(int argc, NXSL_Value **argv, NXSL_Value **ppResult);
int F_pow(int argc, NXSL_Value **argv, NXSL_Value **ppResult);
int F_strftime(int argc, NXSL_Value **argv, NXSL_Value **ppResult);
int F_substr(int argc, NXSL_Value **argv, NXSL_Value **ppResult);
int F_time(int argc, NXSL_Value **argv, NXSL_Value **ppResult);
int F_typeof(int argc, NXSL_Value **argv, NXSL_Value **ppResult);
int F_upper(int argc, NXSL_Value **argv, NXSL_Value **ppResult);
int F_AddrInRange(int argc, NXSL_Value **argv, NXSL_Value **ppResult);
int F_AddrInSubnet(int argc, NXSL_Value **argv, NXSL_Value **ppResult);
int F_SecondsToUptime(int argc, NXSL_Value **argv, NXSL_Value **ppResult);


//
// Default built-in function list
//

static NXSL_ExtFunction m_builtinFunctions[] =
{
   { "abs", F_abs, 1 },
   { "classof", F_classof, 1 },
	{ "d2x", F_d2x, -1 },
   { "gmtime", F_gmtime, -1 },
   { "length", F_length, 1 },
   { "localtime", F_localtime, -1 },
   { "lower", F_lower, 1 },
   { "max", F_max, -1 },
   { "min", F_min, -1 },
   { "pow", F_pow, 2 },
	{ "strftime", F_strftime, -1 },
	{ "substr", F_substr, -1 },
	{ "time", F_time, 0 },
   { "typeof", F_typeof, 1 },
   { "upper", F_upper, 1 },
   { "AddrInRange", F_AddrInRange, 3 },
   { "AddrInSubnet", F_AddrInSubnet, 3 },
	{ "SecondsToUptime", F_SecondsToUptime, 1 }
};


//
// Constructor
//

NXSL_Environment::NXSL_Environment()
{
   m_dwNumFunctions = sizeof(m_builtinFunctions) / sizeof(NXSL_ExtFunction);
   m_pFunctionList = (NXSL_ExtFunction *)nx_memdup(m_builtinFunctions, sizeof(m_builtinFunctions));
   m_pStdIn = NULL;
   m_pStdOut = NULL;
   m_pLibrary = NULL;
}


//
// Destructor
//

NXSL_Environment::~NXSL_Environment()
{
   safe_free(m_pFunctionList);
}


//
// Find function by name
//

NXSL_ExtFunction *NXSL_Environment::FindFunction(char *pszName)
{
   DWORD i;

   for(i = 0; i < m_dwNumFunctions; i++)
      if (!strcmp(m_pFunctionList[i].m_szName, pszName))
         return &m_pFunctionList[i];
   return NULL;
}


//
// Register function set
//

void NXSL_Environment::RegisterFunctionSet(DWORD dwNumFunctions, NXSL_ExtFunction *pList)
{
   m_pFunctionList = (NXSL_ExtFunction *)realloc(m_pFunctionList, sizeof(NXSL_ExtFunction) * (m_dwNumFunctions + dwNumFunctions));
   memcpy(&m_pFunctionList[m_dwNumFunctions], pList, sizeof(NXSL_ExtFunction) * dwNumFunctions);
   m_dwNumFunctions += dwNumFunctions;
}


//
// Find module by name
//

BOOL NXSL_Environment::UseModule(NXSL_Program *pMain, TCHAR *pszName)
{
   TCHAR *pData, szBuffer[MAX_PATH];
   DWORD dwSize;
   NXSL_Program *pScript;
   BOOL bRet = FALSE;

   // First, try to find module in library
   if (m_pLibrary != NULL)
   {
      pScript = m_pLibrary->FindScript(pszName);
      if (pScript != NULL)
      {
         pMain->UseModule(pScript, pszName);
         bRet = TRUE;
      }
   }

   // If failed, try to load it from file
   if (!bRet)
   {
      _sntprintf(szBuffer, MAX_PATH, "%s.nxsl", pszName);
      pData = NXSLLoadFile(szBuffer, &dwSize);
      if (pData != NULL)
      {
         pScript = (NXSL_Program *)NXSLCompile(pData, NULL, 0);
         if (pScript != NULL)
         {
            pMain->UseModule(pScript, pszName);
            delete pScript;
            bRet = TRUE;
         }
         free(pData);
      }
   }

   return bRet;
}
