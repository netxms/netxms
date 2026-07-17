/*
** NetXMS - Network Management System
** Copyright (C) 2003-2026 Raden Solutions
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
** File: crashdump.cpp
** Windows out-of-process crash dump handler. Installs an unhandled exception
** filter that hands the crash off to a separate nxcrashsrv process via shared
** memory, so the dump is generated from outside the crashing process.
**
**/

#include "libnetxms.h"
#include <nxproc.h>
#include <nxcrashdump.h>
#include <stdlib.h>
#include <intrin.h>

#if defined(_MSC_VER)
#pragma intrinsic(_ReturnAddress)
#endif

#define DEBUG_TAG _T("crash")

/**
 * Crash dump generator process
 */
static ProcessExecutor *s_crashServer = nullptr;

/**
 * Shared memory section and synchronization objects
 */
static HANDLE s_mapping = nullptr;
static CrashDumpRequest *s_request = nullptr;
static HANDLE s_eventRequest = nullptr;
static HANDLE s_eventDone = nullptr;

/**
 * Hand the crash off to the generator process and wait for the dump to be written.
 * Must be allocation-free and use minimal stack so it can run on stack overflow.
 */
static void RequestCrashDump(EXCEPTION_POINTERS *pInfo)
{
   if (s_request == nullptr)
      return;
   s_request->faultingThreadId = GetCurrentThreadId();
   s_request->exceptionPointers = static_cast<uint64_t>(reinterpret_cast<uintptr_t>(pInfo));
   SetEvent(s_eventRequest);
   WaitForSingleObject(s_eventDone, NXCRASH_DUMP_TIMEOUT);
}

/**
 * Unhandled exception filter
 */
static LONG WINAPI CrashExceptionFilter(EXCEPTION_POINTERS *pInfo)
{
   RequestCrashDump(pInfo);
   return EXCEPTION_EXECUTE_HANDLER;   // terminate the process after the dump
}

/**
 * Pure virtual call handler. There is no exception context, so capture the
 * current one and synthesize EXCEPTION_POINTERS for the generator.
 */
static void CrashPureCallHandler()
{
   CONTEXT context;
   RtlCaptureContext(&context);

   EXCEPTION_RECORD record;
   memset(&record, 0, sizeof(record));
   record.ExceptionCode = STATUS_NONCONTINUABLE_EXCEPTION;
   record.ExceptionFlags = EXCEPTION_NONCONTINUABLE;
#if defined(_MSC_VER)
   record.ExceptionAddress = reinterpret_cast<void*>(_ReturnAddress());
#else
   record.ExceptionAddress = __builtin_return_address(0);
#endif

   EXCEPTION_POINTERS pointers;
   pointers.ContextRecord = &context;
   pointers.ExceptionRecord = &record;

   RequestCrashDump(&pointers);
   ExitProcess(0xDEAD);
}

/**
 * Release all crash handler resources
 */
static void CleanupCrashHandler()
{
   delete_and_null(s_crashServer);
   if (s_request != nullptr)
   {
      UnmapViewOfFile(s_request);
      s_request = nullptr;
   }
   if (s_mapping != nullptr)
   {
      CloseHandle(s_mapping);
      s_mapping = nullptr;
   }
   if (s_eventRequest != nullptr)
   {
      CloseHandle(s_eventRequest);
      s_eventRequest = nullptr;
   }
   if (s_eventDone != nullptr)
   {
      CloseHandle(s_eventDone);
      s_eventDone = nullptr;
   }
}

/**
 * Start out-of-process crash handler. On success installs an unhandled exception
 * filter; on failure leaves the process without a handler and returns false.
 * Dump directory size limit is the maximum total size of dump files in the dump
 * directory - once it is reached, the generator writes crash info files only.
 * Zero means unlimited.
 */
bool LIBNETXMS_EXPORTABLE StartCrashHandler(const TCHAR *processName, const TCHAR *dumpDir, bool fullDump, uint64_t dumpDirSizeLimit)
{
   uint32_t pid = GetCurrentProcessId();

   WCHAR mapName[64], reqName[64], doneName[64], readyName[64];
   BuildCrashDumpObjectName(mapName, L"Share", pid);
   BuildCrashDumpObjectName(reqName, L"Req", pid);
   BuildCrashDumpObjectName(doneName, L"Done", pid);
   BuildCrashDumpObjectName(readyName, L"Ready", pid);

   // Create shared memory section and fill static fields
   s_mapping = CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, sizeof(CrashDumpRequest), mapName);
   if (s_mapping == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 2, _T("Cannot create shared memory section for crash handler (error %u)"), GetLastError());
      return false;
   }
   s_request = static_cast<CrashDumpRequest*>(MapViewOfFile(s_mapping, FILE_MAP_WRITE, 0, 0, sizeof(CrashDumpRequest)));
   if (s_request == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 2, _T("Cannot map shared memory section for crash handler (error %u)"), GetLastError());
      CleanupCrashHandler();
      return false;
   }
   memset(s_request, 0, sizeof(CrashDumpRequest));
   s_request->protocolVersion = NXCRASH_PROTOCOL_VERSION;
   s_request->fullDump = fullDump ? 1 : 0;
#ifdef UNICODE
   wcslcpy(s_request->processName, processName, 64);
#else
   MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, processName, -1, s_request->processName, 64);
#endif

   // Create synchronization events (manual reset)
   HANDLE eventReady = CreateEventW(nullptr, TRUE, FALSE, readyName);
   s_eventRequest = CreateEventW(nullptr, TRUE, FALSE, reqName);
   s_eventDone = CreateEventW(nullptr, TRUE, FALSE, doneName);
   if ((eventReady == nullptr) || (s_eventRequest == nullptr) || (s_eventDone == nullptr))
   {
      nxlog_debug_tag(DEBUG_TAG, 2, _T("Cannot create synchronization objects for crash handler (error %u)"), GetLastError());
      if (eventReady != nullptr)
         CloseHandle(eventReady);
      CleanupCrashHandler();
      return false;
   }

   // Launch crash dump generator process
   TCHAR cmdLine[MAX_PATH + 128];
   _sntprintf(cmdLine, MAX_PATH + 128, _T("nxcrashsrv.exe %u \"%s\" ") UINT64_FMT, pid, dumpDir, dumpDirSizeLimit);
   s_crashServer = new ProcessExecutor(cmdLine, false);
   bool ready = false;
   if (s_crashServer->execute())
   {
      // Wait for the generator to signal that it is ready to receive requests
      ready = (WaitForSingleObject(eventReady, 2000) == WAIT_OBJECT_0);
      if (!ready)
         nxlog_debug_tag(DEBUG_TAG, 2, _T("Crash dump generator did not become ready in time"));
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 2, _T("Cannot start crash dump generator process"));
   }
   CloseHandle(eventReady);

   if (!ready)
   {
      CleanupCrashHandler();
      return false;
   }

   // Reserve stack space so the exception filter can run even on stack overflow
   // (SetThreadStackGuarantee is Vista+; on XP the crash filter runs without this guarantee)
#if (_WIN32_WINNT >= 0x0600)
   ULONG stackGuarantee = 65536;
   SetThreadStackGuarantee(&stackGuarantee);
#endif

   SetUnhandledExceptionFilter(CrashExceptionFilter);
   _set_purecall_handler(CrashPureCallHandler);

   nxlog_debug_tag(DEBUG_TAG, 2, _T("Crash handler started (process name %s, dump directory %s)"), processName, dumpDir);
   return true;
}

/**
 * Stop crash handler and terminate the generator process
 */
void LIBNETXMS_EXPORTABLE StopCrashHandler()
{
   SetUnhandledExceptionFilter(nullptr);
   CleanupCrashHandler();
}
