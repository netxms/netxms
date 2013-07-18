/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2010 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published
** by the Free Software Foundation; either version 3 of the License, or
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
** File: seh.cpp
**
**/

#include "libnetxms.h"
#include <dbghelp.h>
#include "StackWalker.h"

#pragma warning(disable : 4482)


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

static BOOL (*m_pfExceptionHandler)(EXCEPTION_POINTERS *) = NULL;
static void (*m_pfWriter)(const TCHAR *pszText) = NULL;
static HANDLE m_hExceptionLock = INVALID_HANDLE_VALUE;
static TCHAR m_szBaseProcessName[64] = _T("netxms");
static TCHAR m_szDumpDir[MAX_PATH] = _T("C:\\");
static DWORD m_dwLogMessageCode = 0;
static BOOL m_printToScreen = FALSE;
static BOOL m_writeFullDump = FALSE;


//
// Output for stack walker
//

void NxStackWalker::OnOutput(LPCSTR pszText)
{
	if (m_pfWriter != NULL)
	{
		m_pfWriter(_T("  "));
#ifdef UNICODE
		WCHAR *wtext = WideStringFromMBString(pszText);
		m_pfWriter(wtext);
		free(wtext);
#else
		m_pfWriter(pszText);
#endif
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

/**
 * Set exception handler
 */
void LIBNETXMS_EXPORTABLE SetExceptionHandler(BOOL (*pfHandler)(EXCEPTION_POINTERS *),
															 void (*pfWriter)(const TCHAR *), const TCHAR *pszDumpDir,
															 const TCHAR *pszBaseProcessName, DWORD dwLogMsgCode,
															 BOOL writeFullDump, BOOL printToScreen)
{
	m_pfExceptionHandler = pfHandler;
	m_pfWriter = pfWriter;
	if (pszBaseProcessName != NULL)
		nx_strncpy(m_szBaseProcessName, pszBaseProcessName, 64);
	if (pszDumpDir != NULL)
		nx_strncpy(m_szDumpDir, pszDumpDir, MAX_PATH);
	m_dwLogMessageCode = dwLogMsgCode;
	m_writeFullDump = writeFullDump;
	m_printToScreen = printToScreen;
}

/**
 * Get exception name from code
 */
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

/**
 * Default exception handler for console applications
 */
BOOL LIBNETXMS_EXPORTABLE SEHDefaultConsoleHandler(EXCEPTION_POINTERS *pInfo)
{
	_tprintf(_T("EXCEPTION: %08X (%s) at %p\n"),
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
	return TRUE;
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
		return m_pfExceptionHandler(pInfo) ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH;
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
		free(pArg);
	}
	__except(___ExceptionHandler((EXCEPTION_POINTERS *)_exception_info()))
	{
		ExitProcess(99);
	}
	return THREAD_OK;
}


/*
 * Windows service exception handling
 * ****************************************************
 */


//
// Static data
//

static FILE *m_pExInfoFile = NULL;


//
// Writer for SEHShowCallStack()
//

void LIBNETXMS_EXPORTABLE SEHServiceExceptionDataWriter(const TCHAR *pszText)
{
	if (m_pExInfoFile != NULL)
		_fputts(pszText, m_pExInfoFile);
}


//
// Exception handler
//

BOOL LIBNETXMS_EXPORTABLE SEHServiceExceptionHandler(EXCEPTION_POINTERS *pInfo)
{
	TCHAR szWindowsVersion[256] = _T("ERROR"), szInfoFile[MAX_PATH], szDumpFile[MAX_PATH], szProcNameUppercase[64];
	HANDLE hFile;
	time_t t;
	MINIDUMP_EXCEPTION_INFORMATION mei;
   SYSTEM_INFO sysInfo;

	t = time(NULL);
	_tcscpy(szProcNameUppercase, m_szBaseProcessName);
	_tcsupr(szProcNameUppercase);

	// Create info file
	_sntprintf(szInfoFile, MAX_PATH, _T("%s\\%s-%d-%u.info"),
	           m_szDumpDir, m_szBaseProcessName, GetCurrentProcessId(), (DWORD)t);
	m_pExInfoFile = _tfopen(szInfoFile, _T("w"));
	if (m_pExInfoFile != NULL)
	{
		_ftprintf(m_pExInfoFile, _T("%s CRASH DUMP\n%s\n"), szProcNameUppercase, ctime(&t));
#ifdef _M_IX86
		_ftprintf(m_pExInfoFile, _T("EXCEPTION: %08X (%s) at %08X\n"),
#else
		_ftprintf(m_pExInfoFile, _T("EXCEPTION: %08X (%s) at %016I64X\n"),
#endif
		          pInfo->ExceptionRecord->ExceptionCode,
		          SEHExceptionName(pInfo->ExceptionRecord->ExceptionCode),
		          pInfo->ExceptionRecord->ExceptionAddress);

		// NetXMS and OS version
		GetWindowsVersionString(szWindowsVersion, 256);
		_ftprintf(m_pExInfoFile, _T("\nNetXMS Version: ") NETXMS_VERSION_STRING _T("\n")
		                         _T("OS Version: %s\n"), szWindowsVersion);

		// Processor architecture
		_ftprintf(m_pExInfoFile, _T("Processor architecture: "));
		GetSystemInfo(&sysInfo);
		switch(sysInfo.wProcessorArchitecture)
		{
			case PROCESSOR_ARCHITECTURE_INTEL:
				_ftprintf(m_pExInfoFile, _T("Intel x86\n"));
				break;
			case PROCESSOR_ARCHITECTURE_MIPS:
				_ftprintf(m_pExInfoFile, _T("MIPS\n"));
				break;
			case PROCESSOR_ARCHITECTURE_ALPHA:
				_ftprintf(m_pExInfoFile, _T("ALPHA\n"));
				break;
			case PROCESSOR_ARCHITECTURE_PPC:
				_ftprintf(m_pExInfoFile, _T("PowerPC\n"));
				break;
			case PROCESSOR_ARCHITECTURE_IA64:
				_ftprintf(m_pExInfoFile, _T("Intel IA-64\n"));
				break;
			case PROCESSOR_ARCHITECTURE_IA32_ON_WIN64:
				_ftprintf(m_pExInfoFile, _T("Intel x86 on Win64\n"));
				break;
			case PROCESSOR_ARCHITECTURE_AMD64:
				_ftprintf(m_pExInfoFile, _T("AMD64 (Intel EM64T)\n"));
				break;
			default:
				_ftprintf(m_pExInfoFile, _T("UNKNOWN\n"));
				break;
		}

#ifdef _X86_
		_ftprintf(m_pExInfoFile, _T("\nRegister information:\n")
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

		_ftprintf(m_pExInfoFile, _T("\nCall stack:\n"));
		SEHShowCallStack(pInfo->ContextRecord);

		fclose(m_pExInfoFile);
	}

	// Create minidump
	_sntprintf(szDumpFile, MAX_PATH, _T("%s\\%s-%d-%u.mdmp"),
	           m_szDumpDir, m_szBaseProcessName, GetCurrentProcessId(), (DWORD)t);
   hFile = CreateFile(szDumpFile, GENERIC_WRITE, 0, NULL,
                      CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
   if (hFile != INVALID_HANDLE_VALUE)
   {
		mei.ThreadId = GetCurrentThreadId();
		mei.ExceptionPointers = pInfo;
		mei.ClientPointers = FALSE;
      MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile,
		                  m_writeFullDump ? MiniDumpWithDataSegs : MiniDumpNormal, &mei, NULL, NULL);
      CloseHandle(hFile);
   }

	// Write event log
	nxlog_write(m_dwLogMessageCode, EVENTLOG_ERROR_TYPE, "xsxss",
               pInfo->ExceptionRecord->ExceptionCode,
               SEHExceptionName(pInfo->ExceptionRecord->ExceptionCode),
	            pInfo->ExceptionRecord->ExceptionAddress,
	            szInfoFile, szDumpFile);

	if (m_printToScreen)
	{
		_tprintf(_T("\n\n*************************************************************\n")
#ifdef _M_IX86
		         _T("EXCEPTION: %08X (%s) at %08X\nPROCESS TERMINATED"),
#else
		         _T("EXCEPTION: %08X (%s) at %016I64X\nPROCESS TERMINATED"),
#endif
               pInfo->ExceptionRecord->ExceptionCode,
               SEHExceptionName(pInfo->ExceptionRecord->ExceptionCode),
               pInfo->ExceptionRecord->ExceptionAddress);
	}

	return TRUE;	// Terminate process
}
