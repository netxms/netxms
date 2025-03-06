/*
** NetXMS - Network Management System
** NetXMS Scripting Language Interpreter
** Copyright (C) 2003-2025 Victor Kirhenshtein
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
** File: main.cpp
**
**/

#include "libnxsl.h"

/**
 * Interface to compiler
 */
NXSL_Program LIBNXSL_EXPORTABLE *NXSLCompile(const TCHAR *source, TCHAR *errorMessage, size_t errorMessageLen, int *errorLine, NXSL_Environment *env)
{
   NXSL_Compiler compiler;
   NXSL_Program *pResult = compiler.compile(source, env);
   if (pResult == nullptr)
   {
      if (errorMessage != nullptr)
         _tcslcpy(errorMessage, compiler.getErrorText(), errorMessageLen);
      if (errorLine != nullptr)
         *errorLine = compiler.getErrorLineNumber();
   }
   return pResult;
}

/**
 * Compile script and create VM
 */
NXSL_VM LIBNXSL_EXPORTABLE *NXSLCompileAndCreateVM(const TCHAR *source, TCHAR *errorMessage, size_t errorMessageLen, NXSL_Environment *env)
{
   NXSL_Program *p = NXSLCompile(source, errorMessage, errorMessageLen, nullptr, env);
   if (p == nullptr)
   {
      delete env;
      return nullptr;
   }

   NXSL_VM *vm = new NXSL_VM(env);
   if (!vm->load(p))
   {
      if (errorMessage != nullptr)
      {
         _tcslcpy(errorMessage, vm->getErrorText(), errorMessageLen);
      }
      delete vm;
      vm = nullptr;
   }
   delete p;
   return vm;
}

/**
 * Load NXSL source file into memory
 */
TCHAR LIBNXSL_EXPORTABLE *NXSLLoadFile(const TCHAR *fileName)
{
   char *content = LoadFileAsUTF8String(fileName);
   if (content == nullptr)
      return nullptr;
#ifdef UNICODE
	WCHAR *wContent = WideStringFromUTF8String(content);
	MemFree(content);
	return wContent;
#else
   return content;
#endif
}

/**
 * Get text for script VM handle failure reason
 */
const TCHAR *ScriptVMHandle::failureReasonText() const
{
   switch (m_failureReason)
   {
      case ScriptVMFailureReason::SUCCESS:
         return _T("Success");
      case ScriptVMFailureReason::SCRIPT_NOT_FOUND:
         return _T("Script not found");
      case ScriptVMFailureReason::SCRIPT_IS_EMPTY:
         return _T("Script is empty");
      case ScriptVMFailureReason::SCRIPT_LOAD_ERROR:
         return _T("Script load error");
      case ScriptVMFailureReason::SCRIPT_VALIDATION_ERROR:
         return _T("Script validation error");
   }
   return _T("Unknown");
}

#ifdef _WIN32

/**
 * DLL entry point
 */
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
   if (dwReason == DLL_PROCESS_ATTACH)
      DisableThreadLibraryCalls(hInstance);
   return TRUE;
}

#endif   /* _WIN32 */
