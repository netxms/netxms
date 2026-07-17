#define _CRT_SECURE_NO_WARNINGS

// Respect the target Windows version selected by the build system (the XP build sets
// _WIN32_WINNT=0x0501); default to Windows 7 otherwise. This file includes <windows.h>
// directly rather than nms_common.h, so it must establish the default itself.
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif

#include <winsock2.h>
#include <windows.h>
#include <psapi.h>
#include <dbghelp.h>
#include <tlhelp32.h>
#include <shellapi.h>
#include <stdio.h>
#include <tchar.h>
#include <time.h>
#include <string>
#include <nxcrashdump.h>
#include <netxms-version.h>

using std::wstring;

/**
 * Deflate given file
 */
bool DeflateFile(const wchar_t *inputFile);

/**
 * Log file name
 */
static TCHAR s_logFile[MAX_PATH];

/**
 * Current process ID
 */
static DWORD s_pid = 0;

/**
 * Write log record
 */
static void WriteLog(const TCHAR *format, ...)
{
   va_list args;
   va_start(args, format);
   FILE *fp = _tfopen(s_logFile, _T("a"));
   if (fp != nullptr)
   {
      time_t now = time(nullptr);
      TCHAR timestamp[64];
      _tcsftime(timestamp, 64, _T("%Y.%m.%d %H:%M:%S"), localtime(&now));
      _ftprintf(fp, _T("%s [%6u] "), timestamp, s_pid);
      _vftprintf(fp, format, args);
      _fputtc(_T('\n'), fp);
      fclose(fp);
   }
   va_end(args);
}

/**
 * Get Windows product name from version number
 */
static void WindowsProductNameFromVersion(OSVERSIONINFOEX *ver, TCHAR *buffer)
{
   switch (ver->dwMajorVersion)
   {
      case 5:
         switch (ver->dwMinorVersion)
         {
         case 0:
            _tcscpy(buffer, _T("2000"));
            break;
         case 1:
            _tcscpy(buffer, _T("XP"));
            break;
         case 2:
            _tcscpy(buffer, (GetSystemMetrics(SM_SERVERR2) != 0) ? _T("Server 2003 R2") : _T("Server 2003"));
            break;
         default:
            _sntprintf(buffer, 256, _T("NT %u.%u"), ver->dwMajorVersion, ver->dwMinorVersion);
            break;
         }
         break;
      case 6:
         switch (ver->dwMinorVersion)
         {
         case 0:
            _tcscpy(buffer, (ver->wProductType == VER_NT_WORKSTATION) ? _T("Vista") : _T("Server 2008"));
            break;
         case 1:
            _tcscpy(buffer, (ver->wProductType == VER_NT_WORKSTATION) ? _T("7") : _T("Server 2008 R2"));
            break;
         case 2:
            _tcscpy(buffer, (ver->wProductType == VER_NT_WORKSTATION) ? _T("8") : _T("Server 2012"));
            break;
         case 3:
            _tcscpy(buffer, (ver->wProductType == VER_NT_WORKSTATION) ? _T("8.1") : _T("Server 2012 R2"));
            break;
         default:
            _sntprintf(buffer, 256, _T("NT %d.%d"), ver->dwMajorVersion, ver->dwMinorVersion);
            break;
         }
         break;
      case 10:
         switch (ver->dwMinorVersion)
         {
         case 0:
            if (ver->wProductType == VER_NT_WORKSTATION)
            {
               _tcscpy(buffer, _T("10"));
            }
            else
            {
               // Useful topic regarding Windows Server 2016/2019 version detection:
               // https://techcommunity.microsoft.com/t5/Windows-Server-Insiders/Windows-Server-2019-version-info/td-p/234472
               if (ver->dwBuildNumber <= 14393)
                  _tcscpy(buffer, _T("Server 2016"));
               else if (ver->dwBuildNumber <= 17763)
                  _tcscpy(buffer, _T("Server 2019"));
               else
                  _sntprintf(buffer, 256, _T("Server %d.%d.%d"), ver->dwMajorVersion, ver->dwMinorVersion, ver->dwBuildNumber);
            }
            break;
         default:
            _sntprintf(buffer, 256, _T("NT %d.%d"), ver->dwMajorVersion, ver->dwMinorVersion);
            break;
         }
         break;
      default:
         _sntprintf(buffer, 256, _T("NT %d.%d"), ver->dwMajorVersion, ver->dwMinorVersion);
         break;
   }
}

/**
 * Get more specific Windows version string
 */
static bool GetWindowsVersionString(TCHAR *versionString, size_t size)
{
   OSVERSIONINFOEX ver;
   ver.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
#pragma warning(push)
#pragma warning(disable : 4996)
   if (!GetVersionEx(reinterpret_cast<OSVERSIONINFO*>(&ver)))
      return false;
#pragma warning(pop)

   TCHAR buffer[256];
   WindowsProductNameFromVersion(&ver, buffer);

   _sntprintf(versionString, size, _T("Windows %s Build %d %s"), buffer, ver.dwBuildNumber, ver.szCSDVersion);
   return true;
}

/**
 * Dump process modules
 */
static void DumpProcessModules(FILE *fp, HANDLE hProcess, HMODULE *modules, int count)
{
   _ftprintf(fp, _T("\nModules:\n"));
   for (int i = 0; i < count; i++)
   {
      TCHAR baseName[256];
      if (!GetModuleBaseName(hProcess, modules[i], baseName, 256))
         _tcscpy(baseName, _T("<unnamed>"));

      TCHAR fileName[MAX_PATH];
      if (!GetModuleFileNameEx(hProcess, modules[i], fileName, MAX_PATH))
        fileName[0] = 0;

      MODULEINFO info;
      if (!GetModuleInformation(hProcess, modules[i], &info, sizeof(info)))
         memset(&info, 0, sizeof(info));

      _ftprintf(fp, _T("   %-48s %p %10u %s\n"), baseName, info.lpBaseOfDll, info.SizeOfImage, fileName);
   }
}

/**
 * Get name of the module where given address is located
 */
static wstring GetModuleForAddress(HANDLE hProcess, const void *addr, HMODULE *modules, int count)
{
   for (int i = 0; i < count; i++)
   {
      MODULEINFO info;
      if (GetModuleInformation(hProcess, modules[i], &info, sizeof(info)))
      {
         if ((addr >= info.lpBaseOfDll) && (addr < (const void*)((const char*)info.lpBaseOfDll + info.SizeOfImage)))
         {
            TCHAR baseName[256];
            if (GetModuleBaseName(hProcess, modules[i], baseName, 256))
               return wstring(baseName);
            return wstring(L"<unknown>");
         }
      }
   }
   return wstring(L"<unknown>");
}

/**
 * Get exception name from code
 */
static const TCHAR *GetExceptionName(DWORD code)
{
   static struct
   {
      DWORD code;
      const TCHAR *name;
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
      { 0, nullptr }
   };
   int i;

   for (i = 0; exceptionList[i].name != nullptr; i++)
      if (code == exceptionList[i].code)
         return exceptionList[i].name;

   return _T("Unknown");
}

/**
 * Walk and dump the call stack of the faulting thread. The context to start from is
 * taken from the exception's CONTEXT record so the trace reflects the exact point of
 * failure. Symbol names and source lines are resolved if matching PDB files are
 * available; otherwise frames are reported as module+offset (still symbolizable later).
 */
static void DumpThreadStack(FILE *fp, HANDLE hProcess, DWORD faultingThreadId, const CONTEXT *contextRecord, HMODULE *modules, int moduleCount)
{
   _ftprintf(fp, _T("\nFaulting thread ID: %u\nStack trace:\n"), faultingThreadId);

   HANDLE hThread = OpenThread(THREAD_GET_CONTEXT | THREAD_QUERY_INFORMATION, FALSE, faultingThreadId);
   if (hThread == nullptr)
   {
      _ftprintf(fp, _T("   Cannot open faulting thread (error %u)\n"), GetLastError());
      return;
   }

   SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_DEFERRED_LOADS | SYMOPT_UNDNAME | SYMOPT_FAIL_CRITICAL_ERRORS);
#if (_WIN32_WINNT >= 0x0600)
   if (!SymInitializeW(hProcess, nullptr, TRUE))
#else
   // Windows XP's dbghelp.dll (5.1) lacks the wide SymInitializeW; use the ANSI form.
   if (!SymInitialize(hProcess, nullptr, TRUE))
#endif
   {
      _ftprintf(fp, _T("   Cannot initialize symbol handler (error %u)\n"), GetLastError());
      CloseHandle(hThread);
      return;
   }

   // StackWalk64 modifies the context as it unwinds, so work on a private copy
   CONTEXT context;
   memcpy(&context, contextRecord, sizeof(CONTEXT));

   STACKFRAME64 frame;
   memset(&frame, 0, sizeof(frame));
   frame.AddrPC.Mode = AddrModeFlat;
   frame.AddrFrame.Mode = AddrModeFlat;
   frame.AddrStack.Mode = AddrModeFlat;
   DWORD machineType;
#if defined(_M_IX86)
   machineType = IMAGE_FILE_MACHINE_I386;
   frame.AddrPC.Offset = context.Eip;
   frame.AddrFrame.Offset = context.Ebp;
   frame.AddrStack.Offset = context.Esp;
#elif defined(_M_ARM64)
   machineType = IMAGE_FILE_MACHINE_ARM64;
   frame.AddrPC.Offset = context.Pc;
   frame.AddrFrame.Offset = context.Fp;
   frame.AddrStack.Offset = context.Sp;
#else
   machineType = IMAGE_FILE_MACHINE_AMD64;
   frame.AddrPC.Offset = context.Rip;
   frame.AddrFrame.Offset = context.Rbp;
   frame.AddrStack.Offset = context.Rsp;
#endif

   for (int i = 0; i < 256; i++)
   {
      if (!StackWalk64(machineType, hProcess, hThread, &frame, &context, nullptr,
                       SymFunctionTableAccess64, SymGetModuleBase64, nullptr))
         break;
      if (frame.AddrPC.Offset == 0)
         break;

      DWORD64 address = frame.AddrPC.Offset;
      wstring moduleName = GetModuleForAddress(hProcess, reinterpret_cast<void*>(static_cast<uintptr_t>(address)), modules, moduleCount);

#if (_WIN32_WINNT >= 0x0600)
      BYTE symbolBuffer[sizeof(SYMBOL_INFOW) + 256 * sizeof(WCHAR)];
      SYMBOL_INFOW *symbol = reinterpret_cast<SYMBOL_INFOW*>(symbolBuffer);
      memset(symbol, 0, sizeof(SYMBOL_INFOW));
      symbol->SizeOfStruct = sizeof(SYMBOL_INFOW);
      symbol->MaxNameLen = 255;

      DWORD64 displacement = 0;
      if (SymFromAddrW(hProcess, address, &displacement, symbol))
      {
         IMAGEHLP_LINEW64 line;
         memset(&line, 0, sizeof(line));
         line.SizeOfStruct = sizeof(IMAGEHLP_LINEW64);
         DWORD lineDisplacement = 0;
         if (SymGetLineFromAddrW64(hProcess, address, &lineDisplacement, &line))
         {
            _ftprintf(fp, _T("   %2d  %s!%s+0x%I64x  (%s:%u)  [0x%016I64x]\n"),
               i, moduleName.c_str(), symbol->Name, displacement, line.FileName, line.LineNumber, address);
         }
         else
         {
            _ftprintf(fp, _T("   %2d  %s!%s+0x%I64x  [0x%016I64x]\n"),
               i, moduleName.c_str(), symbol->Name, displacement, address);
         }
      }
      else
      {
         DWORD64 moduleBase = SymGetModuleBase64(hProcess, address);
         _ftprintf(fp, _T("   %2d  %s+0x%I64x  [0x%016I64x]\n"),
            i, moduleName.c_str(), (moduleBase != 0) ? (address - moduleBase) : 0, address);
      }
#else
      // Windows XP's dbghelp.dll lacks the wide symbol API (SymFromAddrW /
      // SymGetLineFromAddrW64) but provides the ANSI equivalents; use them and print
      // the narrow (char*) names with %hs.
      BYTE symbolBuffer[sizeof(SYMBOL_INFO) + 256];
      SYMBOL_INFO *symbol = reinterpret_cast<SYMBOL_INFO*>(symbolBuffer);
      memset(symbol, 0, sizeof(SYMBOL_INFO));
      symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
      symbol->MaxNameLen = 255;

      DWORD64 displacement = 0;
      if (SymFromAddr(hProcess, address, &displacement, symbol))
      {
         IMAGEHLP_LINE64 line;
         memset(&line, 0, sizeof(line));
         line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
         DWORD lineDisplacement = 0;
         if (SymGetLineFromAddr64(hProcess, address, &lineDisplacement, &line))
         {
            _ftprintf(fp, _T("   %2d  %s!%hs+0x%I64x  (%hs:%u)  [0x%016I64x]\n"),
               i, moduleName.c_str(), symbol->Name, displacement, line.FileName, line.LineNumber, address);
         }
         else
         {
            _ftprintf(fp, _T("   %2d  %s!%hs+0x%I64x  [0x%016I64x]\n"),
               i, moduleName.c_str(), symbol->Name, displacement, address);
         }
      }
      else
      {
         DWORD64 moduleBase = SymGetModuleBase64(hProcess, address);
         _ftprintf(fp, _T("   %2d  %s+0x%I64x  [0x%016I64x]\n"),
            i, moduleName.c_str(), (moduleBase != 0) ? (address - moduleBase) : 0, address);
      }
#endif
   }

   SymCleanup(hProcess);
   CloseHandle(hThread);
}

/**
 * Write crash info file
 */
static void WriteInfoFile(const TCHAR *fileName, HANDLE hProcess, uint64_t exceptionPointers, DWORD faultingThreadId, const TCHAR *processName, time_t now)
{
   WriteLog(_T("Writing info file %s"), fileName);

   FILE *fp = _tfopen(fileName, _T("w"));
   if (fp == nullptr)
      return;

   HMODULE modules[4096];
   DWORD bytesNeeded;
   int moduleCount;
   if (EnumProcessModules(hProcess, modules, sizeof(modules), &bytesNeeded))
   {
      moduleCount = static_cast<int>(bytesNeeded / sizeof(HMODULE));
   }
   else
   {
      WriteLog(_T("Cannot get list of modules from crashed process (error %u)"), GetLastError());
      moduleCount = 0;
   }

   TCHAR timeText[32];
   struct tm* ltm = localtime(&now);
   _tcsftime(timeText, 32, _T("%Y-%m-%d %H:%M:%S %Z"), ltm);
   _ftprintf(fp, _T("%s CRASH DUMP\n%s\n"), processName, timeText);
   if (exceptionPointers != 0)
   {
      SIZE_T bytesRead;
      EXCEPTION_POINTERS ep;
      if (ReadProcessMemory(hProcess, reinterpret_cast<void*>(static_cast<uintptr_t>(exceptionPointers)), &ep, sizeof(EXCEPTION_POINTERS), &bytesRead))
      {
         EXCEPTION_RECORD exceptionRecord;
         if (ReadProcessMemory(hProcess, ep.ExceptionRecord, &exceptionRecord, sizeof(EXCEPTION_RECORD), &bytesRead))
         {
#ifdef _M_IX86
            _ftprintf(fp, _T("EXCEPTION: %08X (%s) at %08X in %s\n"),
#else
            _ftprintf(fp, _T("EXCEPTION: %08X (%s) at %016I64X in %s\n"),
#endif
               exceptionRecord.ExceptionCode, GetExceptionName(exceptionRecord.ExceptionCode), (ULONG_PTR)exceptionRecord.ExceptionAddress,
               GetModuleForAddress(hProcess, exceptionRecord.ExceptionAddress, modules, moduleCount).c_str());
         }
         else
         {
            _ftprintf(fp, _T("Cannot read exception record\n"));
         }

         // Walk the faulting thread's stack starting from the exception context
         CONTEXT context;
         if (ReadProcessMemory(hProcess, ep.ContextRecord, &context, sizeof(CONTEXT), &bytesRead))
         {
            DumpThreadStack(fp, hProcess, faultingThreadId, &context, modules, moduleCount);
         }
         else
         {
            _ftprintf(fp, _T("Cannot read thread context for stack trace\n"));
         }
      }
      else
      {
         _ftprintf(fp, _T("Cannot get exception pointers\n"));
      }
   }
   else
   {
      _ftprintf(fp, _T("Cannot get exception pointers\n"));
   }

   // NetXMS and OS version
   TCHAR windowsVersion[256];
   GetWindowsVersionString(windowsVersion, 256);
   _ftprintf(fp,
      _T("\nNetXMS Version: ") NETXMS_VERSION_STRING
      _T("\nNetXMS Build Tag: ") NETXMS_BUILD_TAG
      _T("\nOS Version: %s\n"), windowsVersion);

   // Processor architecture
   _ftprintf(fp, _T("Processor architecture: "));
   SYSTEM_INFO sysInfo;
   GetSystemInfo(&sysInfo);
   switch (sysInfo.wProcessorArchitecture)
   {
      case PROCESSOR_ARCHITECTURE_INTEL:
         _ftprintf(fp, _T("Intel x86\n"));
         break;
      case PROCESSOR_ARCHITECTURE_MIPS:
         _ftprintf(fp, _T("MIPS\n"));
         break;
      case PROCESSOR_ARCHITECTURE_ALPHA:
         _ftprintf(fp, _T("ALPHA\n"));
         break;
      case PROCESSOR_ARCHITECTURE_PPC:
         _ftprintf(fp, _T("PowerPC\n"));
         break;
      case PROCESSOR_ARCHITECTURE_IA64:
         _ftprintf(fp, _T("Intel IA-64\n"));
         break;
      case PROCESSOR_ARCHITECTURE_IA32_ON_WIN64:
         _ftprintf(fp, _T("Intel x86 on Win64\n"));
         break;
      case PROCESSOR_ARCHITECTURE_AMD64:
         _ftprintf(fp, _T("AMD64 (Intel EM64T)\n"));
         break;
      default:
         _ftprintf(fp, _T("UNKNOWN\n"));
         break;
   }

   if (moduleCount > 0)
      DumpProcessModules(fp, hProcess, modules, moduleCount);

   fclose(fp);
}

/**
 * Suspend all threads of the target process except the faulting one. The faulting
 * thread is parked in the crashing process's exception filter waiting for the dump
 * to complete, so it must stay runnable; suspending the rest keeps process memory
 * stable while the dump is written.
 */
static void SuspendOtherThreads(DWORD pid, DWORD faultingThreadId)
{
   HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
   if (snapshot == INVALID_HANDLE_VALUE)
   {
      WriteLog(_T("Cannot create thread snapshot (error %u)"), GetLastError());
      return;
   }

   THREADENTRY32 te;
   te.dwSize = sizeof(te);
   if (Thread32First(snapshot, &te))
   {
      do
      {
         if ((te.th32OwnerProcessID == pid) && (te.th32ThreadID != faultingThreadId))
         {
            HANDLE hThread = OpenThread(THREAD_SUSPEND_RESUME, FALSE, te.th32ThreadID);
            if (hThread != nullptr)
            {
               SuspendThread(hThread);
               CloseHandle(hThread);
            }
         }
         te.dwSize = sizeof(te);
      } while (Thread32Next(snapshot, &te));
   }
   CloseHandle(snapshot);
}

/**
 * Generate minidump and info file for crashed process
 */
static void GenerateDump(DWORD pid, HANDLE hProcess, const CrashDumpRequest *request, const TCHAR *dumpDir)
{
   WCHAR processName[64];
   wcsncpy(processName, (request->processName[0] != 0) ? request->processName : L"netxms-process", 64);
   processName[63] = 0;

   time_t now = time(nullptr);
   TCHAR timeText[64];
   _tcsftime(timeText, 64, _T("%Y%m%d-%H%M%S"), localtime(&now));

   TCHAR baseName[MAX_PATH];
   _sntprintf(baseName, MAX_PATH, _T("%s\\%s-crash-%s-%u"), dumpDir, processName, timeText, pid);

   TCHAR dumpFile[MAX_PATH];
   _sntprintf(dumpFile, MAX_PATH, _T("%s.mdmp"), baseName);

   // Keep process memory stable while the dump is written
   SuspendOtherThreads(pid, request->faultingThreadId);

   HANDLE hFile = CreateFile(dumpFile, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
   if (hFile != INVALID_HANDLE_VALUE)
   {
      MINIDUMP_EXCEPTION_INFORMATION mei;
      mei.ThreadId = request->faultingThreadId;
      mei.ExceptionPointers = reinterpret_cast<PEXCEPTION_POINTERS>(static_cast<uintptr_t>(request->exceptionPointers));
      mei.ClientPointers = TRUE;

      static const char *comments = "Version=" NETXMS_VERSION_STRING_A "; BuildTag=" NETXMS_BUILD_TAG_A;
      MINIDUMP_USER_STREAM us;
      us.Type = CommentStreamA;
      us.Buffer = (void*)comments;
      us.BufferSize = static_cast<ULONG>(strlen(comments) + 1);

      MINIDUMP_USER_STREAM_INFORMATION usi;
      usi.UserStreamCount = 1;
      usi.UserStreamArray = &us;

      MINIDUMP_TYPE dumpType = static_cast<MINIDUMP_TYPE>(
         ((request->fullDump != 0) ? MiniDumpWithFullMemory : MiniDumpNormal) | MiniDumpWithHandleData | MiniDumpWithProcessThreadData);

      if (MiniDumpWriteDump(hProcess, pid, hFile, dumpType, &mei, &usi, nullptr))
      {
         CloseHandle(hFile);
         WriteLog(_T("Minidump written to %s"), dumpFile);
      }
      else
      {
         CloseHandle(hFile);
         _wremove(dumpFile);
         WriteLog(_T("MiniDumpWriteDump failed (error %u)"), GetLastError());
      }
   }
   else
   {
      WriteLog(_T("Cannot create dump file %s (error %u)"), dumpFile, GetLastError());
   }

   // Write accompanying info file (process name in upper case for the title)
   TCHAR infoFile[MAX_PATH];
   _sntprintf(infoFile, MAX_PATH, _T("%s.info"), baseName);
   WCHAR titleName[64];
   wcsncpy(titleName, processName, 64);
   titleName[63] = 0;
   CharUpperW(titleName);
   WriteInfoFile(infoFile, hProcess, request->exceptionPointers, request->faultingThreadId, titleName, now);

   // Compress dump file
   if (_waccess(dumpFile, 0) == 0)
   {
      if (DeflateFile(dumpFile))
         _wremove(dumpFile);
      else
         WriteLog(_T("Cannot compress dump file %s (keeping uncompressed file)"), dumpFile);
   }
}

/**
 * Entry point. Built as a GUI subsystem application (WinMain entry) so that no
 * console window is ever allocated when launched by a service that has no console
 * of its own. The crash server communicates only through its log file and the
 * shared memory section, so it never needs a console.
 */
int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR cmdLine, int cmdShow)
{
   int argc;
   WCHAR **argv = CommandLineToArgvW(GetCommandLineW(), &argc);
   if ((argv == nullptr) || (argc < 3))
      return 1;

   s_pid = GetCurrentProcessId();
   DWORD targetPid = _tcstoul(argv[1], nullptr, 10);
   const TCHAR *dumpDir = argv[2];
   _sntprintf(s_logFile, MAX_PATH, _T("%s\\nxcrashsrv.log"), dumpDir);
   WriteLog(_T("Crash server starting (target PID %u)"), targetPid);

   HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, targetPid);
   if (hProcess == nullptr)
   {
      WriteLog(_T("ERROR: cannot open target process %u (error %u)"), targetPid, GetLastError());
      return 2;
   }

   // Open shared memory section and synchronization objects created by the client
   WCHAR mapName[64], reqName[64], doneName[64], readyName[64];
   BuildCrashDumpObjectName(mapName, L"Share", targetPid);
   BuildCrashDumpObjectName(reqName, L"Req", targetPid);
   BuildCrashDumpObjectName(doneName, L"Done", targetPid);
   BuildCrashDumpObjectName(readyName, L"Ready", targetPid);

   HANDLE mapping = OpenFileMappingW(FILE_MAP_READ, FALSE, mapName);
   if (mapping == nullptr)
   {
      WriteLog(_T("ERROR: cannot open shared memory section (error %u)"), GetLastError());
      return 3;
   }
   CrashDumpRequest *request = static_cast<CrashDumpRequest*>(MapViewOfFile(mapping, FILE_MAP_READ, 0, 0, sizeof(CrashDumpRequest)));
   if (request == nullptr)
   {
      WriteLog(_T("ERROR: cannot map shared memory section (error %u)"), GetLastError());
      return 3;
   }
   if (request->protocolVersion != NXCRASH_PROTOCOL_VERSION)
   {
      WriteLog(_T("ERROR: protocol version mismatch (client %u, expected %u)"), request->protocolVersion, NXCRASH_PROTOCOL_VERSION);
      return 4;
   }

   HANDLE eventRequest = OpenEventW(SYNCHRONIZE, FALSE, reqName);
   HANDLE eventDone = OpenEventW(EVENT_MODIFY_STATE, FALSE, doneName);
   HANDLE eventReady = OpenEventW(EVENT_MODIFY_STATE, FALSE, readyName);
   if ((eventRequest == nullptr) || (eventDone == nullptr) || (eventReady == nullptr))
   {
      WriteLog(_T("ERROR: cannot open synchronization objects (error %u)"), GetLastError());
      return 5;
   }

   // Signal client that we are ready to receive requests
   SetEvent(eventReady);
   WriteLog(_T("Waiting for crash dump request"));

   // Wait for a crash request or for the target process to exit
   HANDLE waitHandles[2] = { eventRequest, hProcess };
   DWORD rc = WaitForMultipleObjects(2, waitHandles, FALSE, INFINITE);
   if (rc == WAIT_OBJECT_0)
   {
      WriteLog(_T("Crash dump request received"));
      GenerateDump(targetPid, hProcess, request, dumpDir);
      SetEvent(eventDone);
      WriteLog(_T("Crash dump processing completed"));
   }
   else if (rc == WAIT_OBJECT_0 + 1)
   {
      WriteLog(_T("Target process exited without crash, shutting down"));
   }
   else
   {
      WriteLog(_T("ERROR: wait failed (error %u)"), GetLastError());
   }

   return 0;
}
