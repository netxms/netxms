/* 
** NetXMS - Network Management System
** NetXMS Scripting Language Interpreter
** Copyright (C) 2003-2018 Victor Kirhenshtein
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
int F_abs(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm);
int F_ceil(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm);
int F_chr(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_classof(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm);
int F_d2x(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm);
int F_exit(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm);
int F_exp(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm);
int F_gethostbyaddr(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm);
int F_gethostbyname(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm);
int F_floor(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm);
int F_format(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm);
int F_gmtime(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm);
int F_index(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm);
int F_inList(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm);
int F_left(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm);
int F_length(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm);
int F_localtime(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm);
int F_log(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm);
int F_log10(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm);
int F_lower(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm);
int F_ltrim(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm);
int F_max(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm);
int F_md5(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm);
int F_min(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm);
int F_mktime(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm);
int F_ord(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_pow(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm);
int F_random(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm);
int F_right(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm);
int F_rindex(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm);
int F_round(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm);
int F_rtrim(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm);
int F_sha1(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm);
int F_sha256(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm);
int F_sleep(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm);
int F_strftime(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm);
int F_substr(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm);
int F_sys(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm);
int F_Table(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_tcpConnector(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm);
int F_time(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm);
int F_TIME(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm);
int F_trace(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm);
int F_trim(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm);
int F_typeof(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm);
int F_udpConnector(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm);
int F_upper(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm);
int F_x2d(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_AddrInRange(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm);
int F_AddrInSubnet(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm);
int F_ArrayToString(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_GeoLocation(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_InetAddress(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_ReadPersistentStorage(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_SecondsToUptime(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm);
int F_SplitString(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_WritePersistentStorage(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);

int F_CopyFile(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_CreateDirectory(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_DeleteFile(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_FileAccess(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_OpenFile(int argc, NXSL_Value **argv, NXSL_Value **ppResult, NXSL_VM *vm);
int F_RemoveDirectory(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
int F_RenameFile(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);

int S_max(const NXSL_Identifier& name, NXSL_Value *options, int argc, NXSL_Value **argv, int *selection, NXSL_VM *vm);
int S_min(const NXSL_Identifier& name, NXSL_Value *options, int argc, NXSL_Value **argv, int *selection, NXSL_VM *vm);

/**
 * Default built-in function list
 */
static NXSL_ExtFunction s_builtinFunctions[] =
{
   { "__new@GeoLocation", F_GeoLocation, -1 },
   { "__new@InetAddress", F_InetAddress, -1 },
   { "__new@Table", F_Table, 0 },
   { "__new@TIME", F_TIME, 0 },
	{ "_exit", F_exit, -1 },
   { "abs", F_abs, 1 },
   { "ceil", F_ceil, 1 },
   { "chr", F_chr, 1 },
   { "classof", F_classof, 1 },
	{ "d2x", F_d2x, -1 },
   { "exp", F_exp, 1 },
   { "gethostbyaddr", F_gethostbyaddr, 1 },
   { "gethostbyname", F_gethostbyname, -1 },
   { "floor", F_floor, 1 },
   { "format", F_format, -1 },
   { "gmtime", F_gmtime, -1 },
   { "index", F_index, -1 },
   { "inList", F_inList, 3 },
   { "left", F_left, -1 },
   { "length", F_length, 1 },
   { "localtime", F_localtime, -1 },
   { "log", F_log, 1 },
   { "log10", F_log10, 1 },
   { "lower", F_lower, 1 },
	{ "ltrim", F_ltrim, 1 },
   { "max", F_max, -1 },
   { "md5", F_md5, 1 },
   { "min", F_min, -1 },
   { "mktime", F_mktime, 1 },
   { "ord", F_ord, 1 },
   { "pow", F_pow, 2 },
   { "random", F_random, 2 },
   { "right", F_right, -1 },
   { "rindex", F_rindex, -1 },
   { "round", F_round, -1 },
	{ "rtrim", F_rtrim, 1 },
	{ "sha1", F_sha1, 1 },
	{ "sha256", F_sha256, 1 },
	{ "sleep", F_sleep, 1 },
	{ "strftime", F_strftime, -1 },
	{ "substr", F_substr, -1 },
	{ "sys", F_sys, 1 },
	{ "time", F_time, 0 },
   { "trace", F_trace, 2 },
	{ "trim", F_trim, 1 },
   { "typeof", F_typeof, 1 },
   { "upper", F_upper, 1 },
   { "x2d", F_x2d, 1 },
   { "AddrInRange", F_AddrInRange, 3 },
   { "AddrInSubnet", F_AddrInSubnet, 3 },
   { "ArrayToString", F_ArrayToString, 2 },
   { "ReadPersistentStorage", F_ReadPersistentStorage, 1 },
	{ "SecondsToUptime", F_SecondsToUptime, 1 },
   { "SplitString", F_SplitString, 2 },
	{ "TCPConnector", F_tcpConnector, 2 },
	{ "UDPConnector", F_udpConnector, 2 },
   { "WritePersistentStorage", F_WritePersistentStorage, 2 }
};

/**
 * I/O and file management functions
 */
static NXSL_ExtFunction s_ioFunctions[] =
{
   { "CopyFile", F_CopyFile, 2 },
   { "CreateDirectory", F_CreateDirectory, 1 },
   { "DeleteFile", F_DeleteFile, 1 },
   { "FileAccess", F_FileAccess, 2 },
   { "OpenFile", F_OpenFile, -1 },
   { "RemoveDirectory", F_RemoveDirectory, 1 },
   { "RenameFile", F_RenameFile, 2 }
};

/**
 * Default built-in selector list
 */
static NXSL_ExtSelector s_builtinSelectors[] =
{
   { "max", S_max },
   { "min", S_min }
};

/**
 * Constructor
 */
NXSL_Environment::NXSL_Environment()
{
   m_numFunctions = sizeof(s_builtinFunctions) / sizeof(NXSL_ExtFunction);
   m_functionsAllocated = std::max(m_numFunctions, 256);
   m_functions = (NXSL_ExtFunction *)malloc(m_functionsAllocated * sizeof(s_builtinFunctions));
   memcpy(m_functions, s_builtinFunctions, sizeof(s_builtinFunctions));

   m_numSelectors = sizeof(s_builtinSelectors) / sizeof(NXSL_ExtSelector);
   m_selectorsAllocated = std::max(m_numSelectors, 16);
   m_selectors = (NXSL_ExtSelector *)malloc(m_selectorsAllocated * sizeof(s_builtinSelectors));
   memcpy(m_selectors, s_builtinSelectors, sizeof(s_builtinSelectors));

   m_library = NULL;
}

/**
 * Destructor
 */
NXSL_Environment::~NXSL_Environment()
{
   free(m_functions);
   free(m_selectors);
}

/**
 * Find function by name
 */
NXSL_ExtFunction *NXSL_Environment::findFunction(const NXSL_Identifier& name)
{
   for(int i = 0; i < m_numFunctions; i++)
      if (!strcmp(m_functions[i].m_name, name.value))
         return &m_functions[i];
   return NULL;
}

/**
 * Register function set
 */
void NXSL_Environment::registerFunctionSet(int count, NXSL_ExtFunction *list)
{
   if (m_numFunctions + count > m_functionsAllocated)
   {
      m_functionsAllocated += std::max(count, 256);
      m_functions = (NXSL_ExtFunction *)realloc(m_functions, sizeof(NXSL_ExtFunction) * m_functionsAllocated);
   }
   memcpy(&m_functions[m_numFunctions], list, sizeof(NXSL_ExtFunction) * count);
   m_numFunctions += count;
}

/**
 * Register I/O and file management functions
 */
void NXSL_Environment::registerIOFunctions()
{
   registerFunctionSet(sizeof(s_ioFunctions) / sizeof(NXSL_ExtFunction), s_ioFunctions);
}

/**
 * Find selector by name
 */
NXSL_ExtSelector *NXSL_Environment::findSelector(const NXSL_Identifier& name)
{
   for(int i = 0; i < m_numSelectors; i++)
      if (!strcmp(m_selectors[i].m_name, name.value))
         return &m_selectors[i];
   return NULL;
}

/**
 * Register selector set
 */
void NXSL_Environment::registerSelectorSet(int count, NXSL_ExtSelector *list)
{
   if (m_numSelectors + count > m_selectorsAllocated)
   {
      m_selectorsAllocated += std::max(count, 256);
      m_selectors = (NXSL_ExtSelector *)realloc(m_selectors, sizeof(NXSL_ExtSelector) * m_selectorsAllocated);
   }
   memcpy(&m_selectors[m_numSelectors], list, sizeof(NXSL_ExtSelector) * count);
   m_numSelectors += count;
}

/**
 * Load module into VM
 */
bool NXSL_Environment::loadModule(NXSL_VM *vm, const NXSL_ModuleImport *importInfo)
{
   TCHAR *pData, szBuffer[MAX_PATH];
   UINT32 dwSize;
   NXSL_Program *pScript;
   bool bRet = false;

   // First, try to find module in library
   if (m_library != NULL)
   {
      pScript = m_library->findNxslProgram(importInfo->name);
      if (pScript != NULL)
      {
         vm->loadModule(pScript, importInfo);
         bRet = true;
      }
   }

   // If failed, try to load it from file
   if (!bRet)
   {
      _sntprintf(szBuffer, MAX_PATH, _T("%s.nxsl"), importInfo->name);
      pData = NXSLLoadFile(szBuffer, &dwSize);
      if (pData != NULL)
      {
         pScript = (NXSL_Program *)NXSLCompile(pData, NULL, 0, NULL);
         if (pScript != NULL)
         {
            vm->loadModule(pScript, importInfo);
            delete pScript;
            bRet = true;
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

/**
 * Additional VM configuration
 */
void NXSL_Environment::configureVM(NXSL_VM *vm)
{
}
