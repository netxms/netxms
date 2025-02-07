#define _CRT_SECURE_NO_WARNINGS

#include <Windows.h>
#include <psapi.h>
#include <stdio.h>
#include <tchar.h>
#include <time.h>
#include <algorithm>
#include <client/windows/crash_generation/crash_generation_server.h>
#include <client/windows/crash_generation/client_info.h>
#include <netxms-version.h>

using namespace google_breakpad;

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
 * Get value of custom client info entry by name
 */
static wstring GetCustomClientInfoEntry(const ClientInfo *clientInfo, const WCHAR *name)
{
   CustomClientInfo info = clientInfo->GetCustomInfo();
   if (info.entries != nullptr)
   {
      for (size_t i = 0; i < info.count; i++)
      {
         if (!_wcsicmp(info.entries[i].name, name))
            return wstring(info.entries[i].value);
      }
   }
   return wstring();
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
 * Write crash info file
 */
static void WriteInfoFile(const TCHAR *fileName, const ClientInfo *clientInfo, const TCHAR *processName, time_t now)
{
   WriteLog(_T("Writing info file %s"), fileName);

   FILE *fp = _tfopen(fileName, _T("w"));
   if (fp == nullptr)
      return;

   HMODULE modules[4096];
   DWORD bytesNeeded;
   int moduleCount;
   if (EnumProcessModules(clientInfo->process_handle(), modules, sizeof(modules), &bytesNeeded))
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
   EXCEPTION_POINTERS *exceptionPointersRef = nullptr;
   if (clientInfo->GetClientExceptionInfo(&exceptionPointersRef))
   {
      SIZE_T bytesRead;
      EXCEPTION_POINTERS exceptionPointers;
      if (ReadProcessMemory(clientInfo->process_handle(), exceptionPointersRef, &exceptionPointers, sizeof(EXCEPTION_POINTERS), &bytesRead))
      {
         EXCEPTION_RECORD exceptionRecord;
         if (ReadProcessMemory(clientInfo->process_handle(), exceptionPointers.ExceptionRecord, &exceptionRecord, sizeof(EXCEPTION_RECORD), &bytesRead))
         {
#ifdef _M_IX86
            _ftprintf(fp, _T("EXCEPTION: %08X (%s) at %08X in %s\n"),
#else
            _ftprintf(fp, _T("EXCEPTION: %08X (%s) at %016I64X in %s\n"),
#endif
               exceptionRecord.ExceptionCode, GetExceptionName(exceptionRecord.ExceptionCode), (ULONG_PTR)exceptionRecord.ExceptionAddress,
               GetModuleForAddress(clientInfo->process_handle(), exceptionRecord.ExceptionAddress, modules, moduleCount).c_str());
         }
         else
         {
            _ftprintf(fp, _T("Cannot read exception record\n"));
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
      DumpProcessModules(fp, clientInfo->process_handle(), modules, moduleCount);

   fclose(fp);
}

/**
 * Called when client process requests crash dump generation
 */
static void DumpCallback(void* context, const ClientInfo* clientInfo, const std::wstring* dumpFile)
{
   WriteLog(_T("Received dump request from client"));
   WriteLog(_T("Original dump file: %s"), dumpFile->c_str());

   wstring processName = GetCustomClientInfoEntry(clientInfo, L"ProcessName");
   if (processName.empty())
      processName = _T("netxms-process");

   TCHAR timeText[64];
   time_t now = time(nullptr);
   _tcsftime(timeText, 64, _T("%Y%m%d-%H%M%S"), localtime(&now));

   wstring dumpPath = dumpFile->substr(0, dumpFile->find_last_of(L'\\'));

   TCHAR fileName[MAX_PATH];
   _sntprintf(fileName, MAX_PATH, _T("%s\\%s-crash-%s-%u.info"), dumpPath.c_str(), processName.c_str(), timeText, clientInfo->pid());
   std::transform(processName.begin(), processName.end(), processName.begin(), ::toupper);
   WriteInfoFile(fileName, clientInfo, processName.c_str(), now);

   // Rename dump file
   _tcscpy(&fileName[_tcslen(fileName) - 4], _T("mdmp"));
   wstring fullDumpFile = dumpFile->substr(0, dumpFile->find_last_of(L'.'));
   fullDumpFile.append(_T("-full.dmp"));
   if (_waccess(fullDumpFile.c_str(), 0) == 0)
   {
      _wrename(fullDumpFile.c_str(), fileName);
      _wremove(dumpFile->c_str());
      if (DeflateFile(fileName))
         _wremove(fileName);
   }
   else
   {
      _wrename(dumpFile->c_str(), fileName);
   }

   WriteLog(_T("Crash dump processing completed"));
   ExitProcess(0);
}

/**
 * Called when client process connects to server
 */
static void ClientConnectCallback(void* context, const ClientInfo* clientInfo)
{
   WriteLog(_T("Client process connected"));
}

/**
 * Called when client process exits
 */
static void ClientExitCallback(void* context, const ClientInfo* clientInfo)
{
   WriteLog(_T("Client process exited"));
   ExitProcess(0);
}

/**
 * Entry point
 */
int _tmain(int argc, TCHAR *argv[])
{
   if (argc < 3)
   {
      _tprintf(_T("*** Crash Server *** Required arguments msissing\n"));
      return 1;
   }

   s_pid = GetCurrentProcessId();
   _sntprintf(s_logFile, MAX_PATH, _T("%s\\nxcrashsrv.log"), argv[2]);
   WriteLog(_T("Crash server starting (pipe name %s)"), argv[1]);

   std::wstring pipeName(L"\\\\.\\pipe\\");
   pipeName.append(argv[1]);
   std::wstring dumpPath(argv[2]);
   CrashGenerationServer server(pipeName, nullptr, ClientConnectCallback, nullptr, DumpCallback, nullptr, ClientExitCallback, nullptr, nullptr, nullptr, true, &dumpPath);
   if (!server.Start())
   {
      WriteLog(_T("ERROR: cannot start server instance"));
      return 2;
   }

   WriteLog(_T("Waiting for requests"));
   while (true)
      Sleep(600000);

   return 0;
}
