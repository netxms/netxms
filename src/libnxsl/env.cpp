/* 
** NetXMS - Network Management System
** NetXMS Scripting Language Interpreter
** Copyright (C) 2003-2013 Victor Kirhenshtein
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
** File: env.cpp
**
**/

#include "libnxsl.h"

/**
 * Externals
 */
int F_abs(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_Program *program);
int F_ceil(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_Program *program);
int F_classof(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_Program *program);
int F_d2x(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_Program *program);
int F_exit(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_Program *program);
int F_exp(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_Program *program);
int F_floor(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_Program *program);
int F_format(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_Program *program);
int F_gmtime(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_Program *program);
int F_index(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_Program *program);
int F_left(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_Program *program);
int F_length(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_Program *program);
int F_localtime(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_Program *program);
int F_log(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_Program *program);
int F_log10(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_Program *program);
int F_lower(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_Program *program);
int F_ltrim(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_Program *program);
int F_max(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_Program *program);
int F_min(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_Program *program);
int F_pow(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_Program *program);
int F_random(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_Program *program);
int F_right(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_Program *program);
int F_rindex(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_Program *program);
int F_round(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_Program *program);
int F_rtrim(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_Program *program);
int F_sleep(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_Program *program);
int F_strftime(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_Program *program);
int F_substr(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_Program *program);
int F_sys(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_Program *program);
int F_time(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_Program *program);
int F_trace(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_Program *program);
int F_trim(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_Program *program);
int F_typeof(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_Program *program);
int F_upper(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_Program *program);
int F_AddrInRange(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_Program *program);
int F_AddrInSubnet(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_Program *program);
int F_SecondsToUptime(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_Program *program);
int F_tcpConnector(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_Program *program);
int F_udpConnector(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_Program *program);

/**
 * Default built-in function list
 */
static NXSL_ExtFunction m_builtinFunctions[] =
{
	{ _T("_exit"), F_exit, -1 },
   { _T("abs"), F_abs, 1 },
   { _T("ceil"), F_ceil, 1 },
   { _T("classof"), F_classof, 1 },
	{ _T("d2x"), F_d2x, -1 },
   { _T("exp"), F_exp, 1 },
   { _T("floor"), F_floor, 1 },
   { _T("format"), F_format, -1 },
   { _T("gmtime"), F_gmtime, -1 },
   { _T("index"), F_index, -1 },
   { _T("left"), F_left, -1 },
   { _T("length"), F_length, 1 },
   { _T("localtime"), F_localtime, -1 },
   { _T("log"), F_log, 1 },
   { _T("log10"), F_log10, 1 },
   { _T("lower"), F_lower, 1 },
	{ _T("ltrim"), F_ltrim, 1 },
   { _T("max"), F_max, -1 },
   { _T("min"), F_min, -1 },
   { _T("pow"), F_pow, 2 },
   { _T("random"), F_random, 2 },
   { _T("right"), F_right, -1 },
   { _T("rindex"), F_rindex, -1 },
   { _T("round"), F_round, -1 },
	{ _T("rtrim"), F_rtrim, 1 },
	{ _T("sleep"), F_sleep, 1 },
	{ _T("strftime"), F_strftime, -1 },
	{ _T("substr"), F_substr, -1 },
	{ _T("sys"), F_sys, 1 },
	{ _T("time"), F_time, 0 },
   { _T("trace"), F_trace, 2 },
	{ _T("trim"), F_trim, 1 },
   { _T("typeof"), F_typeof, 1 },
   { _T("upper"), F_upper, 1 },
   { _T("AddrInRange"), F_AddrInRange, 3 },
   { _T("AddrInSubnet"), F_AddrInSubnet, 3 },
	{ _T("SecondsToUptime"), F_SecondsToUptime, 1 },
	{ _T("TCPConnector"), F_tcpConnector, 2 },
	{ _T("UDPConnector"), F_udpConnector, 2 },
};

/**
 * Constructor
 */
NXSL_Environment::NXSL_Environment()
{
   m_dwNumFunctions = sizeof(m_builtinFunctions) / sizeof(NXSL_ExtFunction);
   m_pFunctionList = (NXSL_ExtFunction *)nx_memdup(m_builtinFunctions, sizeof(m_builtinFunctions));
   m_pLibrary = NULL;
}

/**
 * Destructor
 */
NXSL_Environment::~NXSL_Environment()
{
   safe_free(m_pFunctionList);
}

/**
 * Find function by name
 */
NXSL_ExtFunction *NXSL_Environment::findFunction(const TCHAR *pszName)
{
   DWORD i;

   for(i = 0; i < m_dwNumFunctions; i++)
      if (!_tcscmp(m_pFunctionList[i].m_szName, pszName))
         return &m_pFunctionList[i];
   return NULL;
}

/**
 * Register function set
 */
void NXSL_Environment::registerFunctionSet(DWORD dwNumFunctions, NXSL_ExtFunction *pList)
{
   m_pFunctionList = (NXSL_ExtFunction *)realloc(m_pFunctionList, sizeof(NXSL_ExtFunction) * (m_dwNumFunctions + dwNumFunctions));
   memcpy(&m_pFunctionList[m_dwNumFunctions], pList, sizeof(NXSL_ExtFunction) * dwNumFunctions);
   m_dwNumFunctions += dwNumFunctions;
}

/**
 * Find module by name
 */
BOOL NXSL_Environment::useModule(NXSL_Program *pMain, const TCHAR *pszName)
{
   TCHAR *pData, szBuffer[MAX_PATH];
   DWORD dwSize;
   NXSL_Program *pScript;
   BOOL bRet = FALSE;

   // First, try to find module in library
   if (m_pLibrary != NULL)
   {
      pScript = m_pLibrary->findScript(pszName);
      if (pScript != NULL)
      {
         pMain->useModule(pScript, pszName);
         bRet = TRUE;
      }
   }

   // If failed, try to load it from file
   if (!bRet)
   {
      _sntprintf(szBuffer, MAX_PATH, _T("%s.nxsl"), pszName);
      pData = NXSLLoadFile(szBuffer, &dwSize);
      if (pData != NULL)
      {
         pScript = (NXSL_Program *)NXSLCompile(pData, NULL, 0);
         if (pScript != NULL)
         {
            pMain->useModule(pScript, pszName);
            delete pScript;
            bRet = TRUE;
         }
         free(pData);
      }
   }

   return bRet;
}

/**
 * Write trace message
 * Default implementation does nothing
 */
void NXSL_Environment::trace(int level, const TCHAR *text)
{
}

/**
 * Print text to standard output
 * Default implementation writes value as string to stdout, if it is set
 */
void NXSL_Environment::print(NXSL_Value *value)
{
   const TCHAR *text = value->getValueAsCString();
   if (text != NULL)
	{
      WriteToTerminal(text);
	}
   else
	{
      WriteToTerminal(_T("(null)"));
	}
}
