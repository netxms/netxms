/* 
** NetXMS - Network Management System
** Copyright (C) 2005, 2006, 2007 Victor Kirhenshtein
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
** File: seh.cpp
**
**/

#include "libnetxms.h"
#include <dbghelp.h>
#include "StackWalker.h"


//
// Customized StackWalker class
//

class NxStackWalker : public StackWalker
{
protected:
	virtual void OnOutput(LPCSTR pszText);
	virtual void OnSymInit(LPCSTR szSearchPath, DWORD symOptions, LPCSTR szUserName) { }
	virtual void OnDbgHelpErr(LPCSTR szFuncName, DWORD gle, DWORD64 addr) { }

public:
	NxStackWalker(int options, LPCSTR szSymPath, DWORD dwProcessId, HANDLE hProcess)
		: StackWalker(options, szSymPath, dwProcessId, hProcess) { }
};


//
// Static data
//

static void (*m_pfExceptionHandler)(EXCEPTION_POINTERS *) = NULL;
static void (*m_pfWriter)(char *pszText) = NULL;
static HANDLE m_hExceptionLock = INVALID_HANDLE_VALUE;


//
// Output for stack walker
//

void NxStackWalker::OnOutput(LPCSTR pszText)
{
	if (m_pfWriter != NULL)
	{
		m_pfWriter("  ");
		m_pfWriter((char *)pszText);
	}
	else
	{
		fputs("  ", stdout);
		fputs(pszText, stdout);
	}
}


//
// Initialize SEH functions
//

void SEHInit(void)
{
	m_hExceptionLock = CreateMutex(NULL, FALSE, NULL);
}


//
// Set exception handler
//

void LIBNETXMS_EXPORTABLE SetExceptionHandler(void (*pfHandler)(EXCEPTION_POINTERS *),
															 void (*pfWriter)(char *))
{
	m_pfExceptionHandler = pfHandler;
	m_pfWriter = pfWriter;
}


//
// Get exception name from code
//

TCHAR LIBNETXMS_EXPORTABLE *SEHExceptionName(DWORD code)
{
   static struct 
   {
      DWORD code;
      TCHAR *name;
   } exceptionList[] =
   {
      { EXCEPTION_ACCESS_VIOLATION, _T("Access violation") },
      { EXCEPTION_ARRAY_BOUNDS_EXCEEDED, _T("Array bounds exceeded") },
      { EXCEPTION_BREAKPOINT, _T("Breakpoint") },
      { EXCEPTION_DATATYPE_MISALIGNMENT, _T("Alignment error") },
      { EXCEPTION_FLT_DENORMAL_OPERAND, _T("FLT_DENORMAL_OPERAND") },
      { EXCEPTION_FLT_DIVIDE_BY_ZERO, _T("FLT_DIVIDE_BY_ZERO") },
      { EXCEPTION_FLT_INEXACT_RESULT, _T("FLT_INEXACT_RESULT") },
      { EXCEPTION_FLT_INVALID_OPERATION, _T("FLT_INVALID_OPERATION") },
      { EXCEPTION_FLT_OVERFLOW, _T("Floating point overflow") },
      { EXCEPTION_FLT_STACK_CHECK, _T("FLT_STACK_CHECK") },
      { EXCEPTION_FLT_UNDERFLOW, _T("Floating point underflow") },
      { EXCEPTION_ILLEGAL_INSTRUCTION, _T("Illegal instruction") },
      { EXCEPTION_IN_PAGE_ERROR, _T("Page error") },
      { EXCEPTION_INT_DIVIDE_BY_ZERO, _T("Divide by zero") },
      { EXCEPTION_INT_OVERFLOW, _T("Integer overflow") },
      { EXCEPTION_INVALID_DISPOSITION, _T("Invalid disposition") },
      { EXCEPTION_NONCONTINUABLE_EXCEPTION, _T("Non-continuable exception") },
      { EXCEPTION_PRIV_INSTRUCTION, _T("Priveledged instruction") },
      { EXCEPTION_SINGLE_STEP, _T("Single step") },
      { EXCEPTION_STACK_OVERFLOW, _T("Stack overflow") },
      { 0, NULL }
   };
   int i;

   for(i = 0; exceptionList[i].name != NULL; i++)
      if (code == exceptionList[i].code)
         return exceptionList[i].name;

   return _T("Unknown");
}


//
// Default exception handler for console applications
//

void LIBNETXMS_EXPORTABLE SEHDefaultConsoleHandler(EXCEPTION_POINTERS *pInfo)
{
	_tprintf(_T("EXCEPTION: %08X (%s) at %08X\n"),
            pInfo->ExceptionRecord->ExceptionCode,
				SEHExceptionName(pInfo->ExceptionRecord->ExceptionCode),
				pInfo->ExceptionRecord->ExceptionAddress);
#ifdef _X86_
   _tprintf(_T("\nRegister information:\n")
				_T("  eax=%08X  ebx=%08X  ecx=%08X  edx=%08X\n")
            _T("  esi=%08X  edi=%08X  ebp=%08X  esp=%08X\n")
            _T("  cs=%04X  ds=%04X  es=%04X  ss=%04X  fs=%04X  gs=%04X  flags=%08X\n"),
            pInfo->ContextRecord->Eax, pInfo->ContextRecord->Ebx,
				pInfo->ContextRecord->Ecx, pInfo->ContextRecord->Edx,
				pInfo->ContextRecord->Esi, pInfo->ContextRecord->Edi,
            pInfo->ContextRecord->Ebp, pInfo->ContextRecord->Esp,
            pInfo->ContextRecord->SegCs, pInfo->ContextRecord->SegDs,
				pInfo->ContextRecord->SegEs, pInfo->ContextRecord->SegSs,
				pInfo->ContextRecord->SegFs, pInfo->ContextRecord->SegGs,
            pInfo->ContextRecord->EFlags);
#endif

	_tprintf(_T("\nCall stack:\n"));
	NxStackWalker sw(NxStackWalker::StackWalkOptions::OptionsAll, NULL, GetCurrentProcessId(), GetCurrentProcess());
	sw.ShowCallstack(GetCurrentThread(), pInfo->ContextRecord);

	_tprintf(_T("\nPROCESS TERMINATED\n"));
}


//
// Show call stack
//

void LIBNETXMS_EXPORTABLE SEHShowCallStack(CONTEXT *pCtx)
{
	NxStackWalker sw(NxStackWalker::StackWalkOptions::OptionsAll, NULL, GetCurrentProcessId(), GetCurrentProcess());
	sw.ShowCallstack(GetCurrentThread(), pCtx);
}


//
// Exception handler
//

int LIBNETXMS_EXPORTABLE ___ExceptionHandler(EXCEPTION_POINTERS *pInfo)
{
	if ((m_pfExceptionHandler != NULL) &&
		 (pInfo->ExceptionRecord->ExceptionCode != EXCEPTION_BREAKPOINT) &&
		 (pInfo->ExceptionRecord->ExceptionCode != EXCEPTION_SINGLE_STEP))
	{
		// Only one exception handler can be executed at a time
		// We will never release mutex because __except block
		// will call ExitProcess anyway and we want to log only first
		// exception in case of multiple simultaneous exceptions
		WaitForSingleObject(m_hExceptionLock, INFINITE);
		m_pfExceptionHandler(pInfo);
		return EXCEPTION_EXECUTE_HANDLER;
	}
	return EXCEPTION_CONTINUE_SEARCH;
}


//
// Starter for threads
//

THREAD_RESULT LIBNETXMS_EXPORTABLE THREAD_CALL SEHThreadStarter(void *pArg)
{
	__try
	{
		((THREAD_START_DATA *)pArg)->start_address(((THREAD_START_DATA *)pArg)->args);
	}
	__except(___ExceptionHandler((EXCEPTION_POINTERS *)_exception_info()))
	{
		ExitProcess(99);
	}
	return THREAD_OK;
}
