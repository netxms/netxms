/*
** NetXMS - Network Management System
** Copyright (C) 2003-2025 Victor Kirhenshtein
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
** File: tools.cpp
**
**/

#ifdef _WIN32
#define _CRT_NONSTDC_NO_WARNINGS
#endif

#include "libnetxms.h"
#include <stdarg.h>
#include <nms_threads.h>
#include <netxms-regex.h>
#include <nxstat.h>
#include <nxcpapi.h>
#include <nxproc.h>

#ifdef _WIN32
#include <psapi.h>
#include <wintrust.h>
#include <accctrl.h>
#include <aclapi.h>
#include <Softpub.h>
#else
#include <pwd.h>
#endif

#if !defined(_WIN32)
#include <sys/time.h>
#include <signal.h>
#endif

#if HAVE_SYS_UTSNAME_H
#include <sys/utsname.h>
#endif

#if HAVE_POLL_H
#include <poll.h>
#endif

#if HAVE_MALLOC_H && !WITH_JEMALLOC
#include <malloc.h>
#endif

#if HAVE_LOCALE_H
#include <locale.h>
#endif

#ifdef _WIN32
HRESULT (WINAPI *imp_SetThreadDescription)(HANDLE, PCWSTR) = NULL;
#endif

void LibCURLCleanup();

/**
 * Process shutdown condition
 */
static Condition s_shutdownCondition(true);

/**
 * Process shutdown flag
 */
static bool s_shutdownFlag = false;

/**
 * Client application indicator
 */
static bool s_isClientApp = false;

/**
 * Original console codepage
 */
#ifdef _WIN32
UINT s_originalConsoleCodePage = 0;
#endif

/**
 * Process exit handler
 */
static void OnProcessExit()
{
   s_shutdownFlag = true;
   s_shutdownCondition.set();
   SubProcessExecutor::shutdown();
   LibCURLCleanup();
#ifdef _WIN32
   SetConsoleOutputCP(s_originalConsoleCodePage);
#endif
}

#ifdef _WIN32

/**
 * Flag for core dump on CRT error
 */
static bool s_coreDumpOnCRTError = false;

/**
 * Last CRT error report
 */
static time_t s_lastCRTErrorReport = 0;

/**
 * Enable/disable process fatal exit on CRT error
 */
void LIBNETXMS_EXPORTABLE EnableFatalExitOnCRTError(bool enable)
{
   s_coreDumpOnCRTError = enable;
}

/**
 * Invalid parameter handler
 */
static void InvalidParameterHandler(const wchar_t *expression, const wchar_t *function, const wchar_t *file, unsigned int line, uintptr_t reserved)
{
#ifdef _DEBUG
   nxlog_write_tag(NXLOG_ERROR, _T("crt"), _T("CRT error: function %s in %s:%u (%s)"), function, file, line, expression);
   if (IsDebuggerPresent())
      DebugBreak();
   else
      FatalExit(99);
#else
   time_t now = time(nullptr);
   if (now - s_lastCRTErrorReport > 600)
   {
      nxlog_write_tag(NXLOG_ERROR, _T("crt"), _T("CRT error detected (IsDebuggerPresent=%s, CoreDumpOnCRTError=%s)"), IsDebuggerPresent() ? _T("true") : _T("false"), s_coreDumpOnCRTError ? _T("true") : _T("false"));
      s_lastCRTErrorReport = now;
   }
   if (IsDebuggerPresent())
   {
      DebugBreak();
   }
   else if (s_coreDumpOnCRTError)
   {
      char *p = nullptr;
      *p = 'X';
   }

#endif
}

#endif

/**
 * Common initialization for any NetXMS process
 *
 * @param commandLineTool set to true for command line tool initialization
 */
void LIBNETXMS_EXPORTABLE InitNetXMSProcess(bool commandLineTool, bool isClientApp)
{
   InitThreadLibrary();
   s_isClientApp = isClientApp;

   // Set locale to C. It shouldn't be needed, according to
   // documentation, but I've seen the cases when agent formats
   // floating point numbers by sprintf inserting comma in place
   // of a dot, as set by system's regional settings.
#if HAVE_SETLOCALE
   setlocale(LC_NUMERIC, "C");
#endif

   // Set default code page according to system settings and update LC_CTYPE
#ifndef _WIN32
   const char *locale = getenv("LC_CTYPE");
   if (locale == nullptr)
      locale = getenv("LC_ALL");
   if (locale == nullptr)
      locale = getenv("LANG");
   if (locale != nullptr)
   {
#if defined(UNICODE) && HAVE_SETLOCALE
      setlocale(LC_CTYPE, locale);
#endif

      const char *cp = strchr(locale, '.');
      if (cp != nullptr)
         SetDefaultCodepage(cp + 1);
   }
#endif

   json_set_alloc_funcs(MemAlloc, MemFree);

#ifdef NETXMS_MEMORY_DEBUG
   InitMemoryDebugger();
#endif

#ifdef _WIN32
   _set_invalid_parameter_handler(InvalidParameterHandler);
   _CrtSetReportMode(_CRT_ASSERT, 0);
   SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
   imp_SetThreadDescription = reinterpret_cast<HRESULT (WINAPI *)(HANDLE, PCWSTR)>(GetProcAddress(LoadLibrary(_T("kernel32.dll")), "SetThreadDescription"));
#endif

#if defined(__sun) || defined(_AIX) || defined(__hpux)
   signal(SIGPIPE, SIG_IGN);
   signal(SIGHUP, SIG_IGN);
   signal(SIGQUIT, SIG_IGN);
   signal(SIGUSR1, SIG_IGN);
   signal(SIGUSR2, SIG_IGN);
#endif

#ifndef _WIN32
   BlockAllSignals(true, commandLineTool);
#endif

   srand(static_cast<unsigned int>(time(nullptr)));

#ifdef _WIN32
   s_originalConsoleCodePage = GetConsoleOutputCP();
   SetConsoleOutputCP(CP_UTF8);
#endif

   atexit(OnProcessExit);
}

/**
 * Initiate shutdown of NetXMS process
 */
void LIBNETXMS_EXPORTABLE InitiateProcessShutdown()
{
   s_shutdownFlag = true;
   s_shutdownCondition.set();
   ThreadSleepMs(10);  // Yield CPU
}

/**
 * Sleep for specified number of seconds or until system shutdown arrives
 * Function will return TRUE if shutdown event occurs
 *
 * @param seconds seconds to sleep
 * @return true if server is shutting down
 */
bool LIBNETXMS_EXPORTABLE SleepAndCheckForShutdown(uint32_t seconds)
{
   return s_shutdownCondition.wait((seconds != INFINITE) ? seconds * 1000 : INFINITE);
}

/**
 * Sleep for specified number of milliseconds or until system shutdown arrives
 * Function will return TRUE if shutdown event occurs
 *
 * @param milliseconds milliseconds to sleep
 * @return true if server is shutting down
 */
bool LIBNETXMS_EXPORTABLE SleepAndCheckForShutdownEx(uint32_t milliseconds)
{
   return s_shutdownCondition.wait(milliseconds);
}

/**
 * Get condition object to wait for shutdown
 */
Condition LIBNETXMS_EXPORTABLE *GetShutdownConditionObject()
{
   return &s_shutdownCondition;
}

/**
 * Check if process shutdown is in progress
 */
bool LIBNETXMS_EXPORTABLE IsShutdownInProgress()
{
   return s_shutdownFlag;
}

#ifdef IMPLEMENT_SECURE_ZERO_MEMORY

/**
 * Internal implementation of SecureZeroMemory
 */
void LIBNETXMS_EXPORTABLE SecureZeroMemory(void *mem, size_t count)
{
   memset(mem, 0, count);
}

#endif

/**
 * Convert IP address from binary form (host bytes order) to string
 */
TCHAR LIBNETXMS_EXPORTABLE *IpToStr(uint32_t ipAddr, TCHAR *buffer)
{
   _sntprintf(buffer, 16, _T("%d.%d.%d.%d"), (int)(ipAddr >> 24), (int)((ipAddr >> 16) & 255),
      (int)((ipAddr >> 8) & 255), (int)(ipAddr & 255));
   return buffer;
}

#ifdef UNICODE

char LIBNETXMS_EXPORTABLE *IpToStrA(uint32_t ipAddr, char *buffer)
{
   snprintf(buffer, 16, "%d.%d.%d.%d", (int)(ipAddr >> 24), (int)((ipAddr >> 16) & 255),
      (int)((ipAddr >> 8) & 255), (int)(ipAddr & 255));
   return buffer;
}

#endif

/**
 * Universal IPv4/IPv6 to string converter
 */
TCHAR LIBNETXMS_EXPORTABLE *SockaddrToStr(struct sockaddr *addr, TCHAR *buffer)
{
	switch(addr->sa_family)
	{
		case AF_INET:
			return IpToStr(ntohl(((struct sockaddr_in *)addr)->sin_addr.s_addr), buffer);
		case AF_INET6:
			return Ip6ToStr(((struct sockaddr_in6 *)addr)->sin6_addr.s6_addr, buffer);
		default:
			buffer[0] = 0;
			return buffer;
	}
}

/**
 * Convert IPv6 address from binary form to string
 */
TCHAR LIBNETXMS_EXPORTABLE *Ip6ToStr(const BYTE *addr, TCHAR *buffer)
{
	if (!memcmp(addr, "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 16))
	{
		_tcscpy(buffer, _T("::"));
		return buffer;
	}

	TCHAR *out = buffer;
	const uint16_t *curr = reinterpret_cast<const uint16_t*>(addr);
	bool hasNulls = false;
	for(int i = 0; i < 8; i++)
	{
	   uint16_t value = ntohs(*curr);
		if ((value != 0) || hasNulls)
		{
			if (out != buffer)
				*out++ = _T(':');
			_sntprintf(out, 5, _T("%x"), value);
			out = buffer + _tcslen(buffer);
			curr++;
		}
		else
		{
			*out++ = _T(':');
			do
			{
				i++;
				curr++;
			}
			while((*curr == 0) && (i < 7));
         if ((i == 7) && (*curr == 0))
         {
   			*out++ = _T(':');
            break;
         }
			i--;
			hasNulls = true;
		}
	}
	*out = 0;
   return buffer;
}

#ifdef UNICODE

/**
 * Convert IPv6 address from binary form to string
 */
char LIBNETXMS_EXPORTABLE *Ip6ToStrA(const BYTE *addr, char *buffer)
{
   if (!memcmp(addr, "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 16))
   {
      strcpy(buffer, "::");
      return buffer;
   }

   char *out = buffer;
   const uint16_t *curr = reinterpret_cast<const uint16_t*>(addr);
   bool hasNulls = false;
   for(int i = 0; i < 8; i++)
   {
      uint16_t value = ntohs(*curr);
      if ((value != 0) || hasNulls)
      {
         if (out != buffer)
            *out++ = ':';
         snprintf(out, 5, "%x", value);
         out = buffer + strlen(buffer);
         curr++;
      }
      else
      {
         *out++ = ':';
         do
         {
            i++;
            curr++;
         }
         while((*curr == 0) && (i < 8));
         if (i == 8)
         {
            *out++ = ':';
            break;
         }
         i--;
         hasNulls = true;
      }
   }
   *out = 0;
   return buffer;
}

#endif

/**
 * Checksum routine for Internet Protocol family headers (C Version)
 *
 * Author -
 * Mike Muuss
 * U. S. Army Ballistic Research Laboratory
 * December, 1983
 */
uint16_t LIBNETXMS_EXPORTABLE CalculateIPChecksum(const void *data, size_t len)
{
   size_t nleft = len;
   uint32_t sum = 0;
   const BYTE *curr = static_cast<const BYTE*>(data);

   /*
    *  Our algorithm is simple, using a 32 bit accumulator (sum),
    *  we add sequential 16 bit words to it, and at the end, fold
    *  back all the carry bits from the top 16 bits into the lower
    *  16 bits.
    */
   while(nleft > 1)
   {
      sum += ((uint16_t)(*curr << 8) | (uint16_t)(*(curr + 1)));
      curr += 2;
      nleft -= 2;
   }

   /* mop up an odd byte, if necessary */
   if (nleft == 1)
      sum += (uint16_t)(*curr);

   /*
    * add back carry outs from top 16 bits to low 16 bits
    */
   while(sum >> 16)
      sum = (sum >> 16) + (sum & 0xffff);   /* add hi 16 to low 16 */
   return htons((uint16_t)(~sum));
}

/**
 * Character comparator - UNICODE/case sensitive
 */
static inline bool _ccw(WCHAR c1, WCHAR c2)
{
   return c1 == c2;
}

/**
 * Character comparator - UNICODE/case insensitive
 */
static inline bool _ccwi(WCHAR c1, WCHAR c2)
{
   return towupper(c1) == towupper(c2);
}

/**
 * Character comparator - non-UNICODE/case sensitive
 */
static inline bool _cca(char c1, char c2)
{
   return c1 == c2;
}

/**
 * Character comparator - non-UNICODE/case insensitive
 */
static inline bool _ccai(char c1, char c2)
{
   return toupper(c1) == toupper(c2);
}

/**
 * Compare pattern with possible ? characters with given text
 */
template<typename T, bool (*Compare)(T, T)>
static inline bool CompareTextBlocks(const T *pattern, const T *text, size_t size)
{
	const T *p = pattern;
	const T *t = text;
	for(size_t i = size; i > 0; i--, p++, t++)
	{
		if (*p == '?')
			continue;
		if (!Compare(*p, *t))
			return false;
	}
	return true;
}

/**
 * Match string against pattern with * and ? metasymbols - implementation
 */
template<typename T, bool (*Compare)(T, T), size_t (*Length)(const T*)>
static inline bool MatchStringEngine(const T *pattern, const T *string)
{
   const T *SPtr, *MPtr, *BPtr, *EPtr;
   size_t bsize;
   bool finishScan;

   SPtr = string;
   MPtr = pattern;

   while(*MPtr != 0)
   {
      switch(*MPtr)
      {
         case '?':
            if (*SPtr != 0)
            {
               SPtr++;
               MPtr++;
            }
            else
            {
               return false;
            }
            break;
         case '*':
            while(*MPtr == '*')
               MPtr++;
            if (*MPtr == 0)
               return true;
            while(*MPtr == '?')      // Handle "*?" case
            {
               if (*SPtr != 0)
                  SPtr++;
               else
                  return false;
               MPtr++;
            }
            BPtr = MPtr;           // Text block begins here
            while((*MPtr != 0) && (*MPtr != '*'))
               MPtr++;     // Find the end of text block
            // Try to find rightmost matching block
            bsize = (size_t)(MPtr - BPtr);
            if (bsize == 0)
               break;   // * immediately after ?
            EPtr = nullptr;
            finishScan = false;
            do
            {
               while(1)
               {
                  while((*SPtr != 0) && !Compare(*SPtr, *BPtr))
                     SPtr++;
                  if (Length(SPtr) < bsize)
                  {
                     if (EPtr == nullptr)
                     {
                        return false;  // Length of remained text less than remaining pattern
                     }
                     else
                     {
                        SPtr = EPtr;   // Revert back to point after last match
                        finishScan = true;
                        break;
                     }
                  }
                  if (CompareTextBlocks<T, Compare>(BPtr, SPtr, bsize))
                     break;
                  SPtr++;
               }
               if (!finishScan)
               {
                  EPtr = SPtr + bsize;   // Remember point after last match
                  SPtr++;    // continue scan at next character
               }
            }
            while(!finishScan);
            break;
         default:
            if (*SPtr == 0)
               return false;
            if (Compare(*MPtr, *SPtr))
            {
               SPtr++;
               MPtr++;
            }
            else
            {
               return false;
            }
            break;
      }
   }

   return *SPtr == 0;
}

/**
 * Match string against pattern with * and ? metasymbols
 *
 * @param pattern pattern to match against
 * @param str string to match
 * @param matchCase set to true for case-sensetive match
 * @return true if string matches given pattern
 */
bool LIBNETXMS_EXPORTABLE MatchStringA(const char *pattern, const char *str, bool matchCase)
{
   if (matchCase)
      return MatchStringEngine<char, _cca, strlen>(pattern, str);
   else
      return MatchStringEngine<char, _ccai, strlen>(pattern, str);
}

/**
 * Match string against pattern with * and ? metasymbols
 *
 * @param pattern pattern to match against
 * @param str string to match
 * @param matchCase set to true for case-sensetive match
 * @return true if string matches given pattern
 */
bool LIBNETXMS_EXPORTABLE MatchStringW(const WCHAR *pattern, const WCHAR *str, bool matchCase)
{
   if (matchCase)
      return MatchStringEngine<WCHAR, _ccw, wcslen>(pattern, str);
   else
      return MatchStringEngine<WCHAR, _ccwi, wcslen>(pattern, str);
}

/**
 * Strip whitespaces and tabs off the string (multibyte version)
 *
 * @param str string to trim
 * @return str for convenience
 */
char LIBNETXMS_EXPORTABLE *TrimA(char *str)
{
   if (str == nullptr)
      return nullptr;

   int i;
   for(i = 0; (str[i] != 0) && isspace(str[i]); i++);
   if (i > 0)
      memmove(str, &str[i], strlen(&str[i]) + 1);
   for(i = (int)strlen(str) - 1; (i >= 0) && isspace(str[i]); i--);
   str[i + 1] = 0;
   return str;
}

/**
 * Strip whitespaces and tabs off the string (wide character version)
 *
 * @param str string to trim
 * @return str for convenience
 */
WCHAR LIBNETXMS_EXPORTABLE *TrimW(WCHAR *str)
{
   if (str == nullptr)
      return nullptr;

   int i;
   for(i = 0; (str[i] != 0) && iswspace(str[i]); i++);
   if (i > 0)
      memmove(str, &str[i], (wcslen(&str[i]) + 1) * sizeof(WCHAR));
   for(i = (int)wcslen(str) - 1; (i >= 0) && iswspace(str[i]); i--);
   str[i + 1] = 0;
   return str;
}

/**
 * Remove trailing CR/LF or LF from string (multibyte version)
 */
void LIBNETXMS_EXPORTABLE RemoveTrailingCRLFA(char *str)
{
	if (*str == 0)
		return;

	char *p = str + strlen(str) - 1;
	if (*p == '\n')
		p--;
	if ((p >= str) && (*p == '\r'))
		p--;
	*(p + 1) = 0;
}

/**
 * Remove trailing CR/LF or LF from string (UNICODE version)
 */
void LIBNETXMS_EXPORTABLE RemoveTrailingCRLFW(WCHAR *str)
{
	if (*str == 0)
		return;

	WCHAR *p = str + wcslen(str) - 1;
	if (*p == L'\n')
		p--;
	if ((p >= str) && (*p == L'\r'))
		p--;
	*(p + 1) = 0;
}

/**
 * Swap bytes in INT16 array or UCS-2 string
 * Length -1 causes stop at first 0 value
 */
void LIBNETXMS_EXPORTABLE bswap_array_16(uint16_t *v, int len)
{
   if (len < 0)
   {
      for (uint16_t *p = v; *p != 0; p++)
         *p = bswap_16(*p);
   }
   else
   {
      int count = 0;
      for (uint16_t *p = v; count < len; p++, count++)
         *p = bswap_16(*p);
   }
}

/**
 * Swap bytes in INT32 array or UCS-4 string
 * Length -1 causes stop at first 0 value
 */
void LIBNETXMS_EXPORTABLE bswap_array_32(uint32_t *v, int len)
{
   if (len < 0)
   {
      for (uint32_t *p = v; *p != 0; p++)
         *p = bswap_32(*p);
   }
   else
   {
      int count = 0;
      for (uint32_t *p = v; count < len; p++, count++)
         *p = bswap_32(*p);
   }
}

#ifndef _WIN32

/**
 * strupr() implementation for non-windows platforms
 */
void LIBNETXMS_EXPORTABLE __strupr(char *in)
{
   char *p = in;

   if (in == NULL)
   {
      return;
   }

   for (; *p != 0; p++)
   {
      // TODO: check/set locale
      *p = toupper(*p);
   }
}

#if defined(UNICODE_UCS2) || defined(UNICODE_UCS4)

/**
 * wcsupr() implementation for non-Windows platforms
 */
void LIBNETXMS_EXPORTABLE __wcsupr(WCHAR *in)
{
   if (in == NULL)
      return;

   WCHAR *p = in;
   for (; *p != 0; p++)
   {
      // TODO: check/set locale
#if HAVE_TOWUPPER
      *p = towupper(*p);
#else
      if (*p < 256)
      {
         *p = (WCHAR)toupper(*p);
      }
#endif
   }
}

#endif

#endif

/**
 * Shorten file path for display by inserting ... if it is longer than max length
 */
String LIBNETXMS_EXPORTABLE ShortenFilePathForDisplay(const TCHAR *path, size_t maxLen)
{
   size_t len = _tcslen(path);
   if (len <= maxLen)
      return String(path);

   if (maxLen < 4)
      return String(_T("..."));

   StringBuffer dp;
   const TCHAR *s = _tcsrchr(path, FS_PATH_SEPARATOR_CHAR);
   if ((s == nullptr) || (len - (s - path) >= maxLen - 3))
   {
      dp.append(_T("..."));
      dp.append(&path[len - (maxLen - 3)]);
   }
   else
   {
      size_t slen = len - (s - path);  // suffix length
      dp.append(path, maxLen - slen - 3);
      dp.append(_T("..."));
      dp.append(s);
   }
   return dp;
}

/**
 * Expand file name. Source and destination may point to same location.
 * Can be used strftime format specifiers and external commands enclosed in ``
 */
const TCHAR LIBNETXMS_EXPORTABLE *ExpandFileName(const TCHAR *name, TCHAR *buffer, size_t bufSize, bool allowShellCommands)
{
	time_t t;
	struct tm *ltm;
#if HAVE_LOCALTIME_R
	struct tm tmBuff;
#endif
	TCHAR temp[MAX_PATH], command[1024];
	size_t outpos = 0;

	t = time(nullptr);
#if HAVE_LOCALTIME_R
	ltm = localtime_r(&t, &tmBuff);
#else
	ltm = localtime(&t);
#endif
   if (_tcsftime(temp, MAX_PATH, name, ltm) <= 0)
   {
      _tcslcpy(temp, name, MAX_PATH);
   }

	for(int i = 0; (temp[i] != 0) && (outpos < bufSize - 1); i++)
	{
		if (temp[i] == _T('`') && allowShellCommands)
		{
			int j = ++i;
			while((temp[j] != _T('`')) && (temp[j] != 0))
				j++;
			int len = std::min(j - i, 1023);
			memcpy(command, &temp[i], len * sizeof(TCHAR));
			command[len] = 0;

			OutputCapturingProcessExecutor executor(command);
			if (executor.execute() && executor.waitForCompletion(5000))
			{
				char result[1024];
				strlcpy(result, executor.getOutput(), 1024);
            char *lf = strchr(result, '\r');
            if (lf != nullptr)
               *lf = 0;

            lf = strchr(result, '\n');
            if (lf != nullptr)
               *lf = 0;

            len = (int)std::min(strlen(result), bufSize - outpos - 1);
#ifdef UNICODE
            len = mb_to_wchar(result, len, &buffer[outpos], len + 1);
#else
            memcpy(&buffer[outpos], result, len);
#endif
            outpos += len;
			}

			i = j;
		}
      else if (temp[i] == _T('$') && temp[i + 1] == _T('{'))
      {
         i += 2;
			int j = i;
			while((temp[j] != _T('}')) && (temp[j] != 0))
				j++;

			int len = std::min(j - i, 1023);

         TCHAR varName[256];
         memcpy(varName, &temp[i], len * sizeof(TCHAR));
         varName[len] = 0;

         String varValue = GetEnvironmentVariableEx(varName);
         if (!varValue.isEmpty())
         {
            len = (int)std::min(varValue.length(), bufSize - outpos - 1);
            memcpy(&buffer[outpos], varValue.cstr(), len * sizeof(TCHAR));
         }
         else
         {
            len = 0;
         }

         outpos += len;
         i = j;
      }
      else
		{
			buffer[outpos++] = temp[i];
		}
	}

	buffer[outpos] = 0;
	return buffer;
}

/**
 * Create directory and all parent directories as needed (like mkdir -p)
 */
bool LIBNETXMS_EXPORTABLE CreateDirectoryTree(const TCHAR *path)
{
   NX_STAT_STRUCT st;
   TCHAR *previous = MemCopyString(path);
   TCHAR *ptr = _tcsrchr(previous, FS_PATH_SEPARATOR_CHAR);
   bool success = false;
   if (ptr != nullptr)
   {
      *ptr = 0;
      if (CALL_STAT_FOLLOW_SYMLINK(previous, &st) != 0)
      {
         success = CreateDirectoryTree(previous);
         if (success)
         {
            success = (CALL_STAT_FOLLOW_SYMLINK(previous, &st) == 0);
         }
      }
      else
      {
         if (S_ISDIR(st.st_mode))
         {
            success = true;
         }
      }
   }
   else
   {
      success = true;
      st.st_mode = 0700;
   }
   MemFree(previous);

   if (success)
   {
#ifdef _WIN32
      success = CreateDirectory(path, nullptr);
#else
      success = (_tmkdir(path, st.st_mode) == 0);
#endif /* _WIN32 */
   }

   return success;
}

/**
 * Delete given directory and all files and sub-directories
 */
bool LIBNETXMS_EXPORTABLE DeleteDirectoryTree(const TCHAR *path)
{
   TCHAR epath[MAX_PATH];
   _tcslcpy(epath, path, MAX_PATH);
   size_t rootPathLen = _tcslen(epath);
   if (rootPathLen >= MAX_PATH - 2)
      return false;  // Path is too long
   epath[rootPathLen++] = FS_PATH_SEPARATOR_CHAR;

   _TDIR *dir = _topendir(path);
   if (dir == nullptr)
      return false;

   bool success = true;
   while(success)
   {
      struct _tdirent *e = _treaddir(dir);
      if (e == nullptr)
         break;

      if (!_tcscmp(e->d_name, _T(".")) || !_tcscmp(e->d_name, _T("..")))
         continue;

      _tcslcpy(&epath[rootPathLen], e->d_name, MAX_PATH - rootPathLen);

#if HAVE_DIRENT_D_TYPE
      bool isDir = (e->d_type == DT_DIR);
#else
      NX_STAT_STRUCT st;
      if (CALL_STAT(epath, &st) != 0)
      {
         success = false;
         break;
      }
      bool isDir = S_ISDIR(st.st_mode);
#endif

      if (isDir)
      {
         success = DeleteDirectoryTree(epath);
      }
      else
      {
#ifdef _WIN32
         success = DeleteFile(epath);
#else
         success = (_tremove(epath) == 0);
#endif
      }
   }

   _tclosedir(dir);

   if (success)
   {
#ifdef _WIN32
      success = RemoveDirectory(path);
#else
      success = (_trmdir(path) == 0);
#endif
   }

   return success;
}

/**
 * Set last modification time to file
 */
bool SetLastModificationTime(TCHAR *fileName, time_t lastModDate)
{
   bool success = false;
#ifdef _WIN32
   struct __utimbuf64 ut;
#else
   struct utimbuf ut;
#endif // _WIN32
   ut.actime = lastModDate;
   ut.modtime = lastModDate;
#ifdef _WIN32
   success = _wutime64(fileName, &ut) == 0;
#else
#ifdef UNICODE
   char mbname[MAX_PATH];
   WideCharToMultiByteSysLocale(fileName, mbname, MAX_PATH);
   success = utime(mbname, &ut) == 0;
#else
   success = utime(fileName, &ut) == 0;
#endif
#endif
   return success;
}

/**
 * Format timestamp as dd.mm.yyyy HH:MM:SS.
 * Provided buffer should be at least 21 characters long.
 */
TCHAR LIBNETXMS_EXPORTABLE *FormatTimestamp(time_t t, TCHAR *buffer)
{
   if (t != 0)
   {
#if HAVE_LOCALTIME_R
      struct tm ltmBuffer;
      struct tm *loc = localtime_r(&t, &ltmBuffer);
#else
      struct tm *loc = localtime(&t);
#endif
      _tcsftime(buffer, 21, _T("%d.%b.%Y %H:%M:%S"), loc);
   }
   else
   {
      _tcscpy(buffer, _T("never"));
   }
   return buffer;
}

/**
 * Format timestamp in milliseconds as yyyy-mm-dd HH:MM:SS.nnn.
 * Provided buffer should be at least 25 characters long.
 */
TCHAR LIBNETXMS_EXPORTABLE *FormatTimestampMs(int64_t timestamp, TCHAR *buffer)
{
   if (timestamp != 0)
   {
      time_t t = static_cast<time_t>(timestamp / 1000);
#if HAVE_LOCALTIME_R
      struct tm ltmBuffer;
      struct tm *loc = localtime_r(&t, &ltmBuffer);
#else
      struct tm *loc = localtime(&t);
#endif
      _tcsftime(buffer, 21, _T("%Y-%m-%d %H:%M:%S."), loc);
      _sntprintf(&buffer[20], 5, _T("%03d"), static_cast<int>(timestamp % 1000));
   }
   else
   {
      _tcscpy(buffer, _T("never"));
   }
   return buffer;
}

/**
 * Format timestamp in ISO 8601 format
 */
std::string LIBNETXMS_EXPORTABLE FormatISO8601Timestamp(time_t t)
{
   struct tm utcTime;
#if HAVE_GMTIME_R
   gmtime_r(&t, &utcTime);
#else
   memcpy(&utcTime, gmtime(&t), sizeof(struct tm));
#endif
   char text[64];
   strftime(text, 64, "%Y-%m-%dT%H:%M:%SZ", &utcTime);
   return std::string(text);
}

/**
 * Format milliseconds timestamp in ISO 8601 format
 */
std::string LIBNETXMS_EXPORTABLE FormatISO8601TimestampMs(int64_t t)
{
   time_t seconds = static_cast<time_t>(t / 1000);
   struct tm utcTime;
#if HAVE_GMTIME_R
   gmtime_r(&seconds, &utcTime);
#else
   memcpy(&utcTime, gmtime(&seconds), sizeof(struct tm));
#endif
   char text[64];
   strftime(text, 64, "%Y-%m-%dT%H:%M:%S.", &utcTime);
   snprintf(&text[20], 44, "%03dZ", static_cast<int>(t % 1000));
   return std::string(text);
}

/**
 * Get local system time zone. Returns pointer to buffer for convenience.
 */
TCHAR LIBNETXMS_EXPORTABLE *GetSystemTimeZone(TCHAR *buffer, size_t size, bool withName, bool forceFullOffset)
{
#if defined(_WIN32)
   TIME_ZONE_INFORMATION tz;
   DWORD tzType = GetTimeZoneInformation(&tz);

   LONG effectiveBias;
   switch(tzType)
   {
      case TIME_ZONE_ID_STANDARD:
         effectiveBias = tz.Bias + tz.StandardBias;
         break;
      case TIME_ZONE_ID_DAYLIGHT:
         effectiveBias = tz.Bias + tz.DaylightBias;
         break;
      case TIME_ZONE_ID_UNKNOWN:
         effectiveBias = tz.Bias;
         break;
      default:    // error
         effectiveBias = 0;
         break;
   }
   LONG minutes = abs(effectiveBias) % 60;

   if (withName)
   {
      WCHAR wst[4], wdt[8], *curr;
      int i;

      // Create 3 letter abbreviation for standard name
      for(i = 0, curr = tz.StandardName; (*curr != 0) && (i < 3); curr++)
         if (iswupper(*curr))
            wst[i++] = *curr;
      while(i < 3)
         wst[i++] = L'X';
      wst[i] = 0;

      // Create abbreviation for DST name
      for(i = 0, curr = tz.DaylightName; (*curr != 0) && (i < 7); curr++)
         if (iswupper(*curr))
            wdt[i++] = *curr;
      while(i < 3)
         wdt[i++] = L'X';
      wdt[i] = 0;

      if ((minutes != 0) || forceFullOffset)
      {
#ifdef UNICODE
         swprintf(buffer, size, L"%s%c%02d:%02d%s", wst, (effectiveBias > 0) ? '-' : '+', abs(effectiveBias) / 60, minutes, (tz.DaylightBias != 0) ? wdt : L"");
#else
         snprintf(buffer, size, "%S%c%02d:%02d%S", wst, (effectiveBias > 0) ? '-' : '+', abs(effectiveBias) / 60, minutes, (tz.DaylightBias != 0) ? wdt : L"");
#endif
      }
      else
      {
#ifdef UNICODE
         swprintf(buffer, size, L"%s%c%02d%s", wst, (effectiveBias > 0) ? '-' : '+', abs(effectiveBias) / 60, (tz.DaylightBias != 0) ? wdt : L"");
#else
         snprintf(buffer, size, "%S%c%02d%S", wst, (effectiveBias > 0) ? '-' : '+', abs(effectiveBias) / 60, (tz.DaylightBias != 0) ? wdt : L"");
#endif
      }
   }
   else
   {
      if ((minutes != 0) || forceFullOffset)
      {
         _sntprintf(buffer, size, _T("%c%02d:%02d"), (effectiveBias > 0) ? _T('-') : _T('+'), abs(effectiveBias) / 60, minutes);
      }
      else
      {
         _sntprintf(buffer, size, _T("%c%02d"), (effectiveBias > 0) ? _T('-') : _T('+'), abs(effectiveBias) / 60);
      }
   }

#else /* not Windows */

   time_t t = time(nullptr);

#if HAVE_TM_GMTOFF
#if HAVE_LOCALTIME_R
   struct tm tmbuff;
   struct tm *loc = localtime_r(&t, &tmbuff);
#else
   struct tm *loc = localtime(&t);
#endif
   int gmtOffset = loc->tm_gmtoff / 3600;
   int gmtOffsetMinutes = (loc->tm_gmtoff % 3600) / 60;
#else /* no tm_gmtoff */
#if HAVE_GMTIME_R
   struct tm tmbuff;
   struct tm *gmt = gmtime_r(&t, &tmbuff);
#else
   struct tm *gmt = gmtime(&t);
#endif
   gmt->tm_isdst = -1;
   time_t gt = mktime(gmt);
   int gmtOffset = (int)((t - gt) / 3600);
   int gmtOffsetMinutes = ((t - gt) % 3600) / 60;
#endif   /* HAVE_TM_GMTOFF */

   if (withName)
   {
      if ((gmtOffsetMinutes != 0) || forceFullOffset)
      {
#ifdef UNICODE
         swprintf(buffer, size, L"%hs%hc%02d:%02d%hs", tzname[0], (gmtOffset >= 0) ? '+' : '-', abs(gmtOffset), abs(gmtOffsetMinutes), (tzname[1] != nullptr) ? tzname[1] : "");
#else
         snprintf(buffer, size, "%s%c%02d:%02d%s", tzname[0], (gmtOffset >= 0) ? '+' : '-', abs(gmtOffset), abs(gmtOffsetMinutes), (tzname[1] != nullptr) ? tzname[1] : "");
#endif
      }
      else
      {
#ifdef UNICODE
         swprintf(buffer, size, L"%hs%hc%02d%hs", tzname[0], (gmtOffset >= 0) ? '+' : '-', abs(gmtOffset), (tzname[1] != nullptr) ? tzname[1] : "");
#else
         snprintf(buffer, size, "%s%c%02d%s", tzname[0], (gmtOffset >= 0) ? '+' : '-', abs(gmtOffset), (tzname[1] != nullptr) ? tzname[1] : "");
#endif
      }
   }
   else
   {
      if ((gmtOffsetMinutes != 0) || forceFullOffset)
         _sntprintf(buffer, size, _T("%c%02d:%02d"), (gmtOffset >= 0) ? _T('+') : _T('-'), abs(gmtOffset), abs(gmtOffsetMinutes));
      else
         _sntprintf(buffer, size, _T("%c%02d"), (gmtOffset >= 0) ? _T('+') : _T('-'), abs(gmtOffset));
   }
#endif   /* _WIN32 */

   return buffer;
}

/**
 * Extract word from line (UNICODE version). Extracted word will be placed in buffer.
 * Returns pointer to the next word or to the null character if end
 * of line reached.
 */
const WCHAR LIBNETXMS_EXPORTABLE *ExtractWordW(const WCHAR *line, WCHAR *buffer, int index)
{
   const WCHAR *ptr = line;
	WCHAR *bptr;

   // Skip initial spaces
   while((*ptr == L' ') || (*ptr == L'\t'))
      ptr++;

   while(index-- > 0)
   {
      // Skip word
      while((*ptr != L' ') && (*ptr != L'\t') && (*ptr != 0))
         ptr++;

      // Skip spaces
      while((*ptr == L' ') || (*ptr == L'\t'))
         ptr++;
   }

   // Copy word to buffer
   for(bptr = buffer; (*ptr != L' ') && (*ptr != L'\t') && (*ptr != 0); ptr++, bptr++)
      *bptr = *ptr;
   *bptr = 0;
   return ptr;
}

/**
 * Extract word from line (multibyte version). Extracted word will be placed in buffer.
 * Returns pointer to the next word or to the null character if end
 * of line reached.
 */
const char LIBNETXMS_EXPORTABLE *ExtractWordA(const char *line, char *buffer, int index)
{
   const char *ptr = line;
	char *bptr;

	// Skip initial spaces
   while((*ptr == ' ') || (*ptr == '\t'))
      ptr++;

	while(index-- > 0)
	{
	   // Skip word
	   while((*ptr != ' ') && (*ptr != '\t') && (*ptr != 0))
	      ptr++;

	   // Skip spaces
	   while((*ptr == ' ') || (*ptr == '\t'))
	      ptr++;
	}

   // Copy word to buffer
   for(bptr = buffer; (*ptr != ' ') && (*ptr != '\t') && (*ptr != 0); ptr++, bptr++)
      *bptr = *ptr;
   *bptr = 0;
   return ptr;
}

#if defined(_WIN32)

/**
 * Get system error string by call to FormatMessage (Windows only)
 */
TCHAR LIBNETXMS_EXPORTABLE *GetSystemErrorText(uint32_t error, TCHAR *buffer, size_t size)
{
   TCHAR *msgBuf;
   if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
         FORMAT_MESSAGE_FROM_SYSTEM |
         FORMAT_MESSAGE_IGNORE_INSERTS,
         NULL, error,
         MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
         (LPTSTR)&msgBuf, 0, nullptr) > 0)
   {
      msgBuf[_tcscspn(msgBuf, _T("\r\n"))] = 0;
      _tcslcpy(buffer, msgBuf, size);
      LocalFree(msgBuf);
   }
   else
   {
      _sntprintf(buffer, size, _T("No description for error code 0x%08X"), error);
   }
   return buffer;
}

/**
 * Get system error string by call to FormatMessage (Windows only)
 */
String LIBNETXMS_EXPORTABLE GetSystemErrorText(uint32_t error)
{
   TCHAR *msgBuf;
   if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
         FORMAT_MESSAGE_FROM_SYSTEM |
         FORMAT_MESSAGE_IGNORE_INSERTS,
         NULL, error,
         MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
         (LPTSTR)&msgBuf, 0, nullptr) > 0)
   {
      msgBuf[_tcscspn(msgBuf, _T("\r\n"))] = 0;
      String text(msgBuf);
      LocalFree(msgBuf);
      return text;
   }

   TCHAR buffer[128];
   _sntprintf(buffer, 128, _T("No description for error code 0x%08X"), error);
   return String(buffer);
}

#endif

/**
 * Get socket error text by error code
 * @param errorCode error code
 * @param buffer buffer to store result
 * @param size size of buffer
 * @return pointer to buffer with error text
 */
TCHAR LIBNETXMS_EXPORTABLE *GetSocketErrorText(uint32_t errorCode, TCHAR *buffer, size_t size)
{
#ifdef _WIN32
   _sntprintf(buffer, size, _T("%u "), errorCode);
   size_t len = _tcslen(buffer);
   GetSystemErrorText(errorCode, &buffer[len], size - len);
#else
   _sntprintf(buffer, size, _T("%u "), errorCode);
   size_t len = _tcslen(buffer);
#if HAVE_STRERROR_R
   _tcserror_r(errorCode, &buffer[len], size - len);
#else
   _tcslcpy(&buffer[len], _tcserror(errorCode), size - len);
#endif
#endif
   return buffer;
}

/**
 * Get last socket error as text
 */
TCHAR LIBNETXMS_EXPORTABLE *GetLastSocketErrorText(TCHAR *buffer, size_t size)
{
#ifdef _WIN32
   return GetSocketErrorText(WSAGetLastError(), buffer, size);
#else
   return GetSocketErrorText(errno, buffer, size);
#endif
}

#if (!HAVE_DAEMON || !HAVE_DECL_DAEMON) && !defined(_WIN32)

/**
 * daemon() implementation for systems which doesn't have one
 */
int LIBNETXMS_EXPORTABLE __daemon(int nochdir, int noclose)
{
   int pid;

   if ((pid = fork()) < 0)
      return -1;
   if (pid != 0)
      exit(0);                // Terminate parent

   setsid();

   if (!nochdir)
      chdir("/");

   if (!noclose)
   {
      fclose(stdin);          // don't need stdin, stdout, stderr
      fclose(stdout);
      fclose(stderr);
   }

   return 0;
}

#endif

/**
 * Check if given name is a valid object name
 */
BOOL LIBNETXMS_EXPORTABLE IsValidObjectName(const TCHAR *pszName, BOOL bExtendedChars)
{
   static TCHAR szValidCharacters[] = _T("0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_- @()./");
   static TCHAR szInvalidCharacters[] = _T("\x01\x02\x03\x04\x05\x06\x07")
	                                     _T("\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F")
													 _T("\x10\x11\x12\x13\x14\x15\x16\x17")
	                                     _T("\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F")
	                                     _T("|\"'*%#\\`;?<>=");

   return (pszName[0] != 0) && (bExtendedChars ? (_tcscspn(pszName, szInvalidCharacters) == _tcslen(pszName)) : (_tcsspn(pszName, szValidCharacters) == _tcslen(pszName)));
}

/**
 * Check if given name is a valid script name
 */
BOOL LIBNETXMS_EXPORTABLE IsValidScriptName(const TCHAR *pszName)
{
   static TCHAR szValidCharacters[] = _T("0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_:");
   return (pszName[0] != 0) && (!isdigit(pszName[0])) && (pszName[0] != ':') &&
          (_tcsspn(pszName, szValidCharacters) == _tcslen(pszName));
}

/**
 * Convert 6-byte MAC address to text representation
 */
TCHAR LIBNETXMS_EXPORTABLE *MACToStr(const uint8_t *data, TCHAR *str)
{
   return BinToStrEx(data, MAC_ADDR_LENGTH, str, _T(':'), 0);
}

/**
 * Convert byte array to text representation (common implementation)
 */
template<typename CharT, char (*Converter)(BYTE)>
static inline void BinToStrImpl(const void *data, size_t size, CharT *str, CharT separator, size_t padding)
{
   const BYTE *in = static_cast<const BYTE*>(data);
   CharT *out = str;
   for(size_t i = 0; i < size; i++, in++)
   {
      *out++ = Converter(*in >> 4);
      *out++ = Converter(*in & 15);
      if (separator != 0)
         *out++ = separator;
   }
   for(size_t i = 0; i < padding; i++)
   {
      *out++ = ' ';
      *out++ = ' ';
      if (separator != 0)
         *out++ = separator;
   }
   if (separator != 0)
      out--;
   *out = 0;
}

/**
 * Convert byte array to text representation (wide character version)
 */
WCHAR LIBNETXMS_EXPORTABLE *BinToStrExW(const void *data, size_t size, WCHAR *str, WCHAR separator, size_t padding)
{
   BinToStrImpl<WCHAR, bin2hexU>(data, size, str, separator, padding);
   return str;
}

/**
 * Convert byte array to text representation (multibyte character version)
 */
char LIBNETXMS_EXPORTABLE *BinToStrExA(const void *data, size_t size, char *str, char separator, size_t padding)
{
   BinToStrImpl<char, bin2hexU>(data, size, str, separator, padding);
   return str;
}

/**
 * Convert byte array to text representation (wide character version, lowercase output)
 */
WCHAR LIBNETXMS_EXPORTABLE *BinToStrExWL(const void *data, size_t size, WCHAR *str, WCHAR separator, size_t padding)
{
   BinToStrImpl<WCHAR, bin2hexL>(data, size, str, separator, padding);
   return str;
}

/**
 * Convert byte array to text representation (multibyte character version, lowercase output)
 */
char LIBNETXMS_EXPORTABLE *BinToStrExAL(const void *data, size_t size, char *str, char separator, size_t padding)
{
   BinToStrImpl<char, bin2hexL>(data, size, str, separator, padding);
   return str;
}

/**
 * Convert string of hexadecimal digits to byte array (wide character version)
 * Returns number of bytes written to destination
 */
size_t LIBNETXMS_EXPORTABLE StrToBinW(const WCHAR *pStr, BYTE *pData, size_t size)
{
   size_t i;
   const WCHAR *pCurr;

   memset(pData, 0, size);
   for(i = 0, pCurr = pStr; (i < size) && (*pCurr != 0); i++)
   {
      pData[i] = hex2bin(*pCurr) << 4;
      pCurr++;
		if (*pCurr != 0)
		{
			pData[i] |= hex2bin(*pCurr);
			pCurr++;
		}
   }
   return i;
}

/**
 * Convert string of hexadecimal digits to byte array (multibyte character version)
 * Returns number of bytes written to destination
 */
size_t LIBNETXMS_EXPORTABLE StrToBinA(const char *pStr, BYTE *pData, size_t size)
{
   size_t i;
   const char *pCurr;

   memset(pData, 0, size);
   for(i = 0, pCurr = pStr; (i < size) && (*pCurr != 0); i++)
   {
      pData[i] = hex2bin(*pCurr) << 4;
      pCurr++;
		if (*pCurr != 0)
		{
			pData[i] |= hex2bin(*pCurr);
			pCurr++;
		}
   }
   return i;
}

/**
 * Translate string
 * NOTE: replacement string shouldn't be longer than original
 */
void LIBNETXMS_EXPORTABLE TranslateStr(TCHAR *pszString, const TCHAR *pszSubStr, const TCHAR *pszReplace)
{
   TCHAR *pszSrc, *pszDst;
   int iSrcLen, iRepLen;

   iSrcLen = (int)_tcslen(pszSubStr);
   iRepLen = (int)_tcslen(pszReplace);
   for(pszSrc = pszString, pszDst = pszString; *pszSrc != 0;)
   {
      if (!_tcsncmp(pszSrc, pszSubStr, iSrcLen))
      {
         memcpy(pszDst, pszReplace, sizeof(TCHAR) * iRepLen);
         pszSrc += iSrcLen;
         pszDst += iRepLen;
      }
      else
      {
         *pszDst++ = *pszSrc++;
      }
   }
   *pszDst = 0;
}

/**
 * Get size of file in bytes
 */
uint64_t LIBNETXMS_EXPORTABLE FileSizeW(const wchar_t *fileName)
{
#if defined(_WIN32)
   WIN32_FIND_DATAW fd;
   HANDLE hFind = FindFirstFileW(fileName, &fd);
   if (hFind == INVALID_HANDLE_VALUE)
      return 0;
   FindClose(hFind);
   return (unsigned __int64)fd.nFileSizeLow + ((unsigned __int64)fd.nFileSizeHigh << 32);
#else
   NX_STAT_STRUCT st;
#ifdef UNICODE
   if (CALL_STAT_FOLLOW_SYMLINK(fileName, &st) != 0)
      return 0;
#else
   char mbname[MAX_PATH];
   WideCharToMultiByteSysLocale(fileName, mbname, MAX_PATH);
   if (CALL_STAT_FOLLOW_SYMLINK(mbname, &st) != 0)
      return 0;
#endif
   return static_cast<uint64_t>(st.st_size);
#endif
}

/**
 * Get size of file in bytes
 */
uint64_t LIBNETXMS_EXPORTABLE FileSizeA(const char *fileName)
{
#ifdef _WIN32
   WIN32_FIND_DATAA fd;
   HANDLE hFind = FindFirstFileA(fileName, &fd);
   if (hFind == INVALID_HANDLE_VALUE)
      return 0;
   FindClose(hFind);

   return (unsigned __int64)fd.nFileSizeLow + ((unsigned __int64)fd.nFileSizeHigh << 32);
#else
   NX_STAT_STRUCT st;
   if (CALL_STAT_FOLLOW_SYMLINK_A(fileName, &st) != 0)
      return 0;
   return static_cast<uint64_t>(st.st_size);
#endif
}

/**
 * Get pointer to clean file name (without path specification)
 */
const TCHAR LIBNETXMS_EXPORTABLE *GetCleanFileName(const TCHAR *fileName)
{
   const TCHAR *ptr = fileName + _tcslen(fileName);
   while((ptr >= fileName) && (*ptr != _T('/')) && (*ptr != _T('\\')) && (*ptr != _T(':')))
      ptr--;
   return (ptr + 1);
}

/**
 * Extended send() - send all data even if single call to send()
 * cannot handle them all
 */
ssize_t LIBNETXMS_EXPORTABLE SendEx(SOCKET hSocket, const void *data, size_t len, int flags, Mutex* mutex)
{
   ssize_t nLeft = len;
	int nRet;

	if (mutex != nullptr)
		mutex->lock();

	do
	{
retry:
#ifdef MSG_NOSIGNAL
		nRet = send(hSocket, ((char *)data) + (len - nLeft), static_cast<int>(nLeft), flags | MSG_NOSIGNAL);
#else
		nRet = send(hSocket, ((char *)data) + (len - nLeft), static_cast<int>(nLeft), flags);
#endif
		if (nRet <= 0)
		{
			if ((WSAGetLastError() == WSAEWOULDBLOCK)
#ifndef _WIN32
			    || (errno == EAGAIN)
#endif
			   )
			{
				// Wait until socket becomes available for writing
			   SocketPoller p(true);
			   p.add(hSocket);
				nRet = p.poll(60000);
#ifdef _WIN32
				if (nRet > 0)
#else
				if ((nRet > 0) || ((nRet == -1) && (errno == EINTR)))
#endif
					goto retry;
			}
			break;
		}
		nLeft -= nRet;
	} while (nLeft > 0);

	if (mutex != nullptr)
		mutex->unlock();

	return nLeft == 0 ? len : nRet;
}

/**
 * Extended recv() - receive data with timeout
 *
 * @param hSocket socket handle
 * @param data data buffer
 * @param len buffer length in bytes
 * @param flags flags to be passed to recv() call
 * @param timeout waiting timeout in milliseconds or INFINITE
 * @return number of bytes read on success, 0 if socket was closed, -1 on error, -2 on timeout
 */
ssize_t LIBNETXMS_EXPORTABLE RecvEx(SOCKET hSocket, void *data, size_t len, int flags, uint32_t timeout, SOCKET controlSocket)
{
   if (hSocket == INVALID_SOCKET)
      return -1;

   SocketPoller sp;
   sp.add(hSocket);
   sp.add(controlSocket);
   int rc = sp.poll(timeout);
   if (rc > 0)
   {
      if ((controlSocket != INVALID_SOCKET) && sp.isSet(controlSocket))
      {
         char data;
#ifdef _WIN32
         recv(controlSocket, &data, 1, 0);
#else
         _read(controlSocket, &data, 1);
#endif
         rc = 0;  // Cancel signal
      }
      else
      {
#ifdef _WIN32
         rc = recv(hSocket, static_cast<char*>(data), static_cast<int>(len), flags);
#else
         do
         {
            rc = recv(hSocket, static_cast<char*>(data), len, flags);
         } while((rc == -1) && (errno == EINTR));
#endif
      }
   }
   else
   {
      rc = -2;
   }
   return rc;
}

/**
 * Read exact number of bytes from socket
 */
bool LIBNETXMS_EXPORTABLE RecvAll(SOCKET s, void *buffer, size_t size, uint32_t timeout)
{
   size_t bytes = 0;
   char *pos = (char *)buffer;
   while(bytes < size)
   {
      ssize_t b = RecvEx(s, pos, size - bytes, 0, timeout);
      if (b <= 0)
         return false;
      bytes += b;
      pos += b;
   }
   return true;
}

/**
 * Connect with given timeout
 * Sets socket to non-blocking mode
 */
int LIBNETXMS_EXPORTABLE ConnectEx(SOCKET s, struct sockaddr *addr, int len, uint32_t timeout, bool *isTimeout)
{
	SetSocketNonBlocking(s);

	if (isTimeout != nullptr)
	   *isTimeout = false;

	int rc = connect(s, addr, len);
	if (rc == -1)
	{
		if ((WSAGetLastError() == WSAEWOULDBLOCK) || (WSAGetLastError() == WSAEINPROGRESS))
		{
#if HAVE_POLL
		   struct pollfd fds;
			fds.fd = s;
			fds.events = POLLOUT;
			fds.revents = 0;
			do
			{
				int64_t startTime = GetMonotonicClockTime();
				rc = poll(&fds, 1, timeout);
				if ((rc != -1) || (errno != EINTR))
					break;
				uint32_t elapsed = static_cast<uint32_t>(GetMonotonicClockTime() - startTime);
				timeout -= std::min(timeout, elapsed);
			} while(timeout > 0);

			if (rc > 0)
			{
            if (fds.revents & (POLLERR | POLLHUP | POLLNVAL))
            {
               rc = -1;
            }
            else if (fds.revents & POLLOUT)
            {
               rc = 0;
            }
            else
            {
               rc = -1;
            }
			}
			else if (rc == 0)	// timeout, return error
			{
				rc = -1;
			   if (isTimeout != nullptr)
			      *isTimeout = true;
			}
#else
			struct timeval tv;
			fd_set wrfs, exfs;

			FD_ZERO(&wrfs);
			FD_SET(s, &wrfs);

			FD_ZERO(&exfs);
			FD_SET(s, &exfs);

#ifdef _WIN32
			tv.tv_sec = timeout / 1000;
			tv.tv_usec = (timeout % 1000) * 1000;
			rc = select(SELECT_NFDS(s + 1), NULL, &wrfs, &exfs, &tv);
#else
			do
			{
				tv.tv_sec = timeout / 1000;
				tv.tv_usec = (timeout % 1000) * 1000;
				int64_t startTime = GetCurrentTimeMs();
				rc = select(SELECT_NFDS(s + 1), NULL, &wrfs, &exfs, &tv);
				if ((rc != -1) || (errno != EINTR))
					break;
				uint32_t elapsed = static_cast<uint32_t>(GetCurrentTimeMs() - startTime);
				timeout -= min(timeout, elapsed);
			} while(timeout > 0);
#endif
			if (rc > 0)
			{
				if (FD_ISSET(s, &exfs))
				{
#ifdef _WIN32
					int err, len = sizeof(int);
					if (getsockopt(s, SOL_SOCKET, SO_ERROR, (char *)&err, &len) == 0)
						WSASetLastError(err);
#endif
					rc = -1;
				}
				else
				{
					rc = 0;
				}
			}
			else if (rc == 0)	// timeout, return error
			{
				rc = -1;
#ifdef _WIN32
				WSASetLastError(WSAETIMEDOUT);
#endif
            if (isTimeout != nullptr)
               *isTimeout = true;
			}
#endif
		}
	}
	return rc;
}

/**
 * Connect to given host/port
 *
 * @return connected socket on success or INVALID_SOCKET on error
 */
SOCKET LIBNETXMS_EXPORTABLE ConnectToHost(const InetAddress& addr, uint16_t port, uint32_t timeout)
{
   SOCKET s = CreateSocket(addr.getFamily(), SOCK_STREAM, 0);
   if (s == INVALID_SOCKET)
      return INVALID_SOCKET;

   SockAddrBuffer saBuffer;
   struct sockaddr *sa = addr.fillSockAddr(&saBuffer, port);
   if (ConnectEx(s, sa, SA_LEN(sa), timeout) == -1)
   {
      closesocket(s);
      s = INVALID_SOCKET;
   }
   return s;
}

/**
 * Prepare UDP socket for sending data to given host/port
 *
 * @return configured socket on success or INVALID_SOCKET on error
 */
SOCKET LIBNETXMS_EXPORTABLE ConnectToHostUDP(const InetAddress& addr, uint16_t port)
{
   SOCKET s = CreateSocket(addr.getFamily(), SOCK_DGRAM, 0);
   if (s == INVALID_SOCKET)
      return INVALID_SOCKET;

   SockAddrBuffer saBuffer;
   struct sockaddr *sa = addr.fillSockAddr(&saBuffer, port);
   int rc = connect(s, sa, SA_LEN(sa));
   if (rc == -1)
   {
      closesocket(s);
      s = INVALID_SOCKET;
   }
   return s;
}

#ifndef VER_PLATFORM_WIN32_WINDOWS
#define VER_PLATFORM_WIN32_WINDOWS 1
#endif
#ifndef VER_PLATFORM_WIN32_CE
#define VER_PLATFORM_WIN32_CE 3
#endif
#ifndef SM_SERVERR2
#define SM_SERVERR2             89
#endif

/**
 * Get OS name and version
 */
void LIBNETXMS_EXPORTABLE GetOSVersionString(TCHAR *pszBuffer, int nBufSize)
{
   int nSize = nBufSize - 1;

   memset(pszBuffer, 0, nBufSize * sizeof(TCHAR));

#if defined(_WIN32)
   OSVERSIONINFO ver;

   ver.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
#pragma warning(push)
#pragma warning(disable : 4996)
   GetVersionEx(&ver);
#pragma warning(pop)
   switch(ver.dwPlatformId)
   {
      case VER_PLATFORM_WIN32_WINDOWS:
         _sntprintf(pszBuffer, nSize, _T("Win%s"), ver.dwMinorVersion == 0 ? _T("95") :
                    (ver.dwMinorVersion == 10 ? _T("98") :
                    (ver.dwMinorVersion == 90 ? _T("Me") : _T("9x"))));
         break;
      case VER_PLATFORM_WIN32_NT:
         _sntprintf(pszBuffer, nSize, _T("WinNT %d.%d"), ver.dwMajorVersion, ver.dwMinorVersion);
         break;
#ifdef VER_PLATFORM_WIN32_CE
      case VER_PLATFORM_WIN32_CE:
         _sntprintf(pszBuffer, nSize, _T("WinCE %d.%d"), ver.dwMajorVersion, ver.dwMinorVersion);
         break;
#endif
      default:
         _sntprintf(pszBuffer, nSize, _T("WinX %d.%d"), ver.dwMajorVersion, ver.dwMinorVersion);
         break;
   }
#else
   struct utsname un;

   uname(&un);
#ifdef UNICODE
   char buf[1024];
   snprintf(buf, 1024, "%s %s", un.sysname, un.release);
   MultiByteToWideCharSysLocale(buf, pszBuffer, nSize);
#else
   snprintf(pszBuffer, nSize, "%s %s", un.sysname, un.release);
#endif
#endif
}

#ifdef _WIN32

/**
 * Get Windows product name from version number
 */
void LIBNETXMS_EXPORTABLE WindowsProductNameFromVersion(OSVERSIONINFOEX *ver, TCHAR *buffer)
{
   switch(ver->dwMajorVersion)
   {
      case 5:
         switch(ver->dwMinorVersion)
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
         switch(ver->dwMinorVersion)
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
         switch(ver->dwMinorVersion)
         {
            case 0:
               if (ver->wProductType == VER_NT_WORKSTATION)
               {
                  _tcscpy(buffer, (ver->dwBuildNumber < 22000) ? _T("10") : _T("11"));
               }
               else
               {
                  // Useful topic regarding Windows Server 2016/2019 version detection:
                  // https://techcommunity.microsoft.com/t5/Windows-Server-Insiders/Windows-Server-2019-version-info/td-p/234472
                  if (ver->dwBuildNumber <= 14393)
                     _tcscpy(buffer, _T("Server 2016"));
                  else if (ver->dwBuildNumber <= 17763)
                     _tcscpy(buffer, _T("Server 2019"));
                  else if (ver->dwBuildNumber <= 20348)
                     _tcscpy(buffer, _T("Server 2022"));
                  else if (ver->dwBuildNumber <= 26212)
                     _tcscpy(buffer, _T("Server 2025"));
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
bool LIBNETXMS_EXPORTABLE GetWindowsVersionString(TCHAR *versionString, size_t size)
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
   Trim(versionString);
   return true;
}

#endif

/**
 * Count number of characters in UNICODE string
 */
int LIBNETXMS_EXPORTABLE NumCharsW(const WCHAR *str, WCHAR ch)
{
   int count = 0;
   for(const WCHAR *p = str; *p != 0; p++)
      if (*p == ch)
         count++;
   return count;
}

/**
 * Count number of characters in non-UNICODE string
 */
int LIBNETXMS_EXPORTABLE NumCharsA(const char *str, char ch)
{
   int count = 0;
   for(const char *p = str; *p != 0; p++)
      if (*p == ch)
         count++;
   return count;
}

/**
 * Match string against regexp (UNICODE version)
 */
bool LIBNETXMS_EXPORTABLE RegexpMatchW(const WCHAR *str, const WCHAR *expr, bool matchCase)
{
   bool result = false;
   const char *errptr;
   int erroffset;
   PCREW *preg = _pcre_compile_w(reinterpret_cast<const PCRE_WCHAR*>(expr), matchCase ? PCRE_COMMON_FLAGS_W : PCRE_COMMON_FLAGS_W | PCRE_CASELESS, &errptr, &erroffset, nullptr);
   if (preg != nullptr)
   {
      int ovector[60];
      if (_pcre_exec_w(preg, nullptr, reinterpret_cast<const PCRE_WCHAR*>(str), static_cast<int>(wcslen(str)), 0, 0, ovector, 60) >= 0) // MATCH
         result = true;
      _pcre_free_w(preg);
   }
   return result;
}

/**
 * Match string against regexp (multibyte version)
 */
bool LIBNETXMS_EXPORTABLE RegexpMatchA(const char *str, const char *expr, bool matchCase)
{
   bool result = false;
   const char *errptr;
   int erroffset;
   pcre *preg = pcre_compile(expr, matchCase ? PCRE_COMMON_FLAGS_A : PCRE_COMMON_FLAGS_A | PCRE_CASELESS, &errptr, &erroffset, nullptr);
   if (preg != nullptr)
   {
      int ovector[60];
      if (pcre_exec(preg, nullptr, str, static_cast<int>(strlen(str)), 0, 0, ovector, 60) >= 0) // MATCH
         result = true;
      pcre_free(preg);
   }
   return result;
}

/**
 * Translate given code to text
 */
const TCHAR LIBNETXMS_EXPORTABLE *CodeToText(int32_t code, CodeLookupElement *lookupTable, const TCHAR *defaultText)
{
   for(int i = 0; lookupTable[i].text != NULL; i++)
      if (lookupTable[i].code == code)
         return lookupTable[i].text;
   return defaultText;
}

/**
 * Translate code to text
 */
int LIBNETXMS_EXPORTABLE CodeFromText(const TCHAR *text, CodeLookupElement *lookupTable, int32_t defaultCode)
{
   for(int i = 0; lookupTable[i].text != NULL; i++)
      if (!_tcsicmp(text, lookupTable[i].text))
         return lookupTable[i].code;
   return defaultCode;
}

/**
 * Extract option value from string of form option=value;option=value;... (UNICODE version)
 */
bool LIBNETXMS_EXPORTABLE ExtractNamedOptionValueW(const WCHAR *optString, const WCHAR *option, WCHAR *buffer, size_t bufSize)
{
	int state;
	size_t pos;
	const WCHAR *curr, *start;
	WCHAR temp[256];

	for(curr = start = optString, pos = 0, state = 0; *curr != 0; curr++)
	{
		switch(*curr)
		{
			case L';':		// Next option
				if (state == 1)
				{
					buffer[pos] = 0;
					TrimW(buffer);
					return true;
				}
				state = 0;
				start = curr + 1;
				break;
			case L'=':
				if (state == 0)
				{
					memcpy(temp, start, (curr - start) * sizeof(WCHAR));
					temp[curr - start] = 0;
					TrimW(temp);
					if (!wcsicmp(option, temp))
						state = 1;
					else
						state = 2;
				}
				else if ((state == 1) && (pos < bufSize - 1))
				{
					buffer[pos++] = L'=';
				}
				break;
			default:
				if ((state == 1) && (pos < bufSize - 1))
					buffer[pos++] = *curr;
				break;
		}
	}

	if (state == 1)
	{
		buffer[pos] = 0;
		TrimW(buffer);
		return true;
	}

	return false;
}

/**
 * Extract option value from string of form option=value;option=value;... (multibyte version)
 */
bool LIBNETXMS_EXPORTABLE ExtractNamedOptionValueA(const char *optString, const char *option, char *buffer, size_t bufSize)
{
	int state;
	size_t pos;
	const char *curr, *start;
	char temp[256];

	for(curr = start = optString, pos = 0, state = 0; *curr != 0; curr++)
	{
		switch(*curr)
		{
			case ';':		// Next option
				if (state == 1)
				{
					buffer[pos] = 0;
					TrimA(buffer);
					return true;
				}
				state = 0;
				start = curr + 1;
				break;
			case '=':
				if (state == 0)
				{
					memcpy(temp, start, curr - start);
					temp[curr - start] = 0;
					TrimA(temp);
					if (!stricmp(option, temp))
						state = 1;
					else
						state = 2;
				}
				else if ((state == 1) && (pos < bufSize - 1))
				{
					buffer[pos++] = '=';
				}
				break;
			default:
				if ((state == 1) && (pos < bufSize - 1))
					buffer[pos++] = *curr;
				break;
		}
	}

	if (state == 1)
	{
		buffer[pos] = 0;
		TrimA(buffer);
		return true;
	}

	return false;
}

/**
 * Extract named option value as boolean (UNICODE version)
 */
bool LIBNETXMS_EXPORTABLE ExtractNamedOptionValueAsBoolW(const WCHAR *optString, const WCHAR *option, bool defVal)
{
	WCHAR buffer[256];
	if (ExtractNamedOptionValueW(optString, option, buffer, 256))
		return !wcsicmp(buffer, L"yes") || !wcsicmp(buffer, L"true");
	return defVal;
}

/**
 * Extract named option value as boolean (multibyte version)
 */
bool LIBNETXMS_EXPORTABLE ExtractNamedOptionValueAsBoolA(const char *optString, const char *option, bool defVal)
{
	char buffer[256];
	if (ExtractNamedOptionValueA(optString, option, buffer, 256))
		return !stricmp(buffer, "yes") || !stricmp(buffer, "true");
	return defVal;
}

/**
 * Extract named option value as integer (UNICODE version)
 */
int32_t LIBNETXMS_EXPORTABLE ExtractNamedOptionValueAsIntW(const WCHAR *optString, const WCHAR *option, int32_t defVal)
{
	WCHAR buffer[256];
	if (ExtractNamedOptionValueW(optString, option, buffer, 256))
	{
	   WCHAR *eptr;
	   int32_t val = wcstol(buffer, &eptr, 0);
		if (*eptr == 0)
			return val;
	}
	return defVal;
}

/**
 * Extract named option value as integer (multibyte version)
 */
int32_t LIBNETXMS_EXPORTABLE ExtractNamedOptionValueAsIntA(const char *optString, const char *option, int32_t defVal)
{
	char buffer[256];
	if (ExtractNamedOptionValueA(optString, option, buffer, 256))
	{
	   char *eptr;
	   int32_t val = strtol(buffer, &eptr, 0);
		if (*eptr == 0)
			return val;
	}
	return defVal;
}

/**
 * Extract named option value as unsigned integer (UNICODE version)
 */
uint32_t LIBNETXMS_EXPORTABLE ExtractNamedOptionValueAsUIntW(const WCHAR *optString, const WCHAR *option, uint32_t defVal)
{
   WCHAR buffer[256];
   if (ExtractNamedOptionValueW(optString, option, buffer, 256))
   {
      WCHAR *eptr;
      uint32_t val = wcstoul(buffer, &eptr, 0);
      if (*eptr == 0)
         return val;
   }
   return defVal;
}

/**
 * Extract named option value as unsigned integer (multibyte version)
 */
uint32_t LIBNETXMS_EXPORTABLE ExtractNamedOptionValueAsUIntA(const char *optString, const char *option, uint32_t defVal)
{
   char buffer[256];
   if (ExtractNamedOptionValueA(optString, option, buffer, 256))
   {
      char *eptr;
      uint32_t val = strtoul(buffer, &eptr, 0);
      if (*eptr == 0)
         return val;
   }
   return defVal;
}

/**
 * Extract named option value as 64 bit unsigned integer (UNICODE version)
 */
uint64_t LIBNETXMS_EXPORTABLE ExtractNamedOptionValueAsUInt64W(const WCHAR *optString, const WCHAR *option, uint64_t defVal)
{
   WCHAR buffer[256];
   if (ExtractNamedOptionValueW(optString, option, buffer, 256))
   {
      WCHAR *eptr;
      uint64_t val = wcstoull(buffer, &eptr, 0);
      if (*eptr == 0)
         return val;
   }
   return defVal;
}

/**
 * Extract named option value as 64 bit unsigned integer (multibyte version)
 */
uint64_t LIBNETXMS_EXPORTABLE ExtractNamedOptionValueAsUInt64A(const char *optString, const char *option, uint64_t defVal)
{
   char buffer[256];
   if (ExtractNamedOptionValueA(optString, option, buffer, 256))
   {
      char *eptr;
      uint64_t val = strtoull(buffer, &eptr, 0);
      if (*eptr == 0)
         return val;
   }
   return defVal;
}

/**
 * Extract named option value as GUID (UNICODE version)
 */
uuid LIBNETXMS_EXPORTABLE ExtractNamedOptionValueAsGUIDW(const WCHAR *optString, const WCHAR *option, const uuid& defVal)
{
   WCHAR buffer[256];
   if (ExtractNamedOptionValueW(optString, option, buffer, 256))
   {
#ifdef UNICODE
      return uuid::parse(buffer);
#else
      char mbbuffer[256];
      wchar_to_mb(buffer, -1, mbbuffer, 256);
      return uuid::parse(mbbuffer);
#endif
   }
   return defVal;
}

/**
 * Extract named option value as GUID (multibyte version)
 */
uuid LIBNETXMS_EXPORTABLE ExtractNamedOptionValueAsGUIDA(const char *optString, const char *option, const uuid& defVal)
{
   char buffer[256];
   if (ExtractNamedOptionValueA(optString, option, buffer, 256))
   {
#ifdef UNICODE
      WCHAR wcbuffer[256];
      mb_to_wchar(buffer, -1, wcbuffer, 256);
      return uuid::parse(wcbuffer);
#else
      return uuid::parse(buffer);
#endif
   }
   return defVal;
}

/**
 * Split string
 */
TCHAR LIBNETXMS_EXPORTABLE **SplitString(const TCHAR *source, TCHAR sep, int *numStrings, bool mergeSeparators)
{
	TCHAR **strings;
	if (mergeSeparators)
   {
	   // Count number of non-empty strings
	   int count = 0;
	   for(const TCHAR *p = source; *p != 0; p++)
	   {
	      if (*p == sep)
         {
            count++;
            while(*(p + 1) == sep)
               p++;
         }
	   }
	   *numStrings = count + 1;
   }
	else
	{
	   *numStrings = NumChars(source, sep) + 1;
	}
	strings = MemAllocArray<TCHAR*>(*numStrings);
	for(int n = 0, i = 0; n < *numStrings; n++, i++)
	{
		int start = i;
		while((source[i] != sep) && (source[i] != 0))
			i++;
		int len = i - start;
		strings[n] = MemAllocString(len + 1);
		memcpy(strings[n], &source[start], len * sizeof(TCHAR));
		strings[n][len] = 0;
		if (mergeSeparators)
      {
         while(source[i + 1] == sep)
            i++;
      }
	}
	return strings;
}

/**
 * Get step size for "%" and "/" crontab cases
 */
static int GetStepSize(TCHAR *str)
{
  int step = 0;
  if (str != NULL)
  {
    *str = 0;
    str++;
    step = *str == _T('\0') ? 1 : _tcstol(str, NULL, 10);
  }

  if (step <= 0)
  {
    step = 1;
  }

  return step;
}

/**
 * Get last day of current month
 */
int LIBNETXMS_EXPORTABLE GetLastMonthDay(struct tm *currTime)
{
   switch(currTime->tm_mon)
   {
      case 1:  // February
         if (((currTime->tm_year % 4) == 0) && (((currTime->tm_year % 100) != 0) || (((currTime->tm_year + 1900) % 400) == 0)))
            return 29;
         return 28;
      case 0:  // January
      case 2:  // March
      case 4:  // May
      case 6:  // July
      case 7:  // August
      case 9:  // October
      case 11: // December
         return 31;
      default:
         return 30;
   }
}

/**
 * Match schedule element
 * NOTE: We assume that pattern can be modified during processing
 */
bool LIBNETXMS_EXPORTABLE MatchScheduleElement(TCHAR *pszPattern, int nValue, int maxValue, struct tm *localTime, time_t currTime, bool checkSeconds)
{
   TCHAR *ptr, *curr;
   int nStep, nCurr, nPrev;
   bool bRun = true, bRange = false;

   // Check for "last" pattern
   if (*pszPattern == _T('L'))
      return nValue == maxValue;

	// Check if time() step was specified (% - special syntax)
	ptr = _tcschr(pszPattern, _T('%'));
	if (checkSeconds && ptr != nullptr)
		return (currTime % GetStepSize(ptr)) != 0;

   // Check if step was specified
   ptr = _tcschr(pszPattern, _T('/'));
   nStep = GetStepSize(ptr);

   if (*pszPattern == _T('*'))
      goto check_step;

   for(curr = pszPattern; bRun; curr = ptr + 1)
   {
      for(ptr = curr; (*ptr != 0) && (*ptr != '-') && (*ptr != ','); ptr++);
      switch(*ptr)
      {
         case '-':
            if (bRange)
               return false;  // Form like 1-2-3 is invalid
            bRange = true;
            *ptr = 0;
            nPrev = _tcstol(curr, nullptr, 10);
            break;
         case 'L':  // special case for last day of week in a month (like 5L - last Friday)
            if (bRange || (localTime == nullptr))
               return false;  // Range with L is not supported; nL form supported only for day of week
            *ptr = 0;
            nCurr = _tcstol(curr, nullptr, 10);
            if ((nValue == nCurr) && (localTime->tm_mday + 7 > GetLastMonthDay(localTime)))
               return true;
            ptr++;
            if (*ptr != ',')
               bRun = false;
            break;
         case 0:
            bRun = false;
            /* no break */
         case ',':
            *ptr = 0;
            nCurr = _tcstol(curr, nullptr, 10);
            if (bRange)
            {
               if ((nValue >= nPrev) && (nValue <= nCurr))
                  goto check_step;
               bRange = false;
            }
            else
            {
               if (nValue == nCurr)
                  return true;
            }
            break;
      }
   }

   return false;

check_step:
   return (nValue % nStep) == 0;
}

/**
 * Match schedule to current time
 */
bool LIBNETXMS_EXPORTABLE MatchSchedule(const TCHAR *schedule, bool *withSeconds, struct tm *currTime, time_t now)
{
   TCHAR szValue[256];

   // Minute
   const TCHAR *pszCurr = ExtractWord(schedule, szValue);
   if (!MatchScheduleElement(szValue, currTime->tm_min, 59, currTime, now, false))
      return false;

   // Hour
   pszCurr = ExtractWord(pszCurr, szValue);
   if (!MatchScheduleElement(szValue, currTime->tm_hour, 23, currTime, now, false))
      return false;

   // Day of month
   pszCurr = ExtractWord(pszCurr, szValue);
   if (!MatchScheduleElement(szValue, currTime->tm_mday, GetLastMonthDay(currTime), currTime, now, false))
      return false;

   // Month
   pszCurr = ExtractWord(pszCurr, szValue);
   if (!MatchScheduleElement(szValue, currTime->tm_mon + 1, 12, currTime, now, false))
      return false;

   // Day of week
   pszCurr = ExtractWord(pszCurr, szValue);
   for(int i = 0; szValue[i] != 0; i++)
      if (szValue[i] == _T('7'))
         szValue[i] = _T('0');
   if (!MatchScheduleElement(szValue, currTime->tm_wday, 6, currTime, now, false))
      return false;

   // Seconds
   szValue[0] = _T('\0');
   ExtractWord(pszCurr, szValue);
   if (szValue[0] != _T('\0'))
   {
      if (withSeconds != nullptr)
         *withSeconds = true;
      return MatchScheduleElement(szValue, currTime->tm_sec, 59, currTime, now, true);
   }

   return true;
}

/**
 * Failure handler for DecryptPasswordW
 */
static inline bool DecryptPasswordFailW(const WCHAR *encryptedPasswd, WCHAR *decryptedPasswd, size_t bufferLenght)
{
   if (decryptedPasswd != encryptedPasswd)
      wcslcpy(decryptedPasswd, encryptedPasswd, bufferLenght);
   return false;
}

/**
 * Decrypt password encrypted with nxencpassw.
 * In case when it was not possible to decrypt password as the decrypted password will be set the original one.
 * The buffer length for encryptedPasswd and decryptedPasswd should be the same.
 */
bool LIBNETXMS_EXPORTABLE DecryptPasswordW(const WCHAR *login, const WCHAR *encryptedPasswd, WCHAR *decryptedPasswd, size_t bufferLenght)
{
   //check that length is correct
   size_t plen = wcslen(encryptedPasswd);
	if ((plen != 44) && (plen != 88))
      return DecryptPasswordFailW(encryptedPasswd, decryptedPasswd, bufferLenght);

   // check that password contains only allowed symbols
   size_t invalidSymbolIndex = wcsspn(encryptedPasswd, L"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/");
   if ((invalidSymbolIndex < plen - 2) || ((invalidSymbolIndex != plen) && ((encryptedPasswd[invalidSymbolIndex] != L'=') || ((invalidSymbolIndex == plen - 2) && (encryptedPasswd[plen - 1] != L'=')))))
      return DecryptPasswordFailW(encryptedPasswd, decryptedPasswd, bufferLenght);

	char *mbencrypted = MBStringFromWideStringSysLocale(encryptedPasswd);
	char *mblogin = MBStringFromWideStringSysLocale(login);

	BYTE encrypted[64], decrypted[64], key[16];
	size_t encSize = ((plen == 44) ? 32 : 64);
	base64_decode(mbencrypted, strlen(mbencrypted), (char *)encrypted, &encSize);
	if (encSize != ((plen == 44) ? 32 : 64))
      return DecryptPasswordFailW(encryptedPasswd, decryptedPasswd, bufferLenght);

	CalculateMD5Hash((BYTE *)mblogin, strlen(mblogin), key);
	ICEDecryptData(encrypted, encSize, decrypted, key);
	decrypted[encSize - 1] = 0;

	MultiByteToWideCharSysLocale((char *)decrypted, decryptedPasswd, bufferLenght);
	decryptedPasswd[bufferLenght - 1] = 0;
	MemFree(mbencrypted);
	MemFree(mblogin);
	SecureZeroMemory(decrypted, sizeof(decrypted));

	return true;
}

/**
 * Failure handler for DecryptPasswordA
 */
static inline bool DecryptPasswordFailA(const char *encryptedPasswd, char *decryptedPasswd, size_t bufferLenght)
{
   if (decryptedPasswd != encryptedPasswd)
      strlcpy(decryptedPasswd, encryptedPasswd, bufferLenght);
   return false;
}

/**
 * Decrypt password encrypted with nxencpassw.
 * In case when it was not possible to decrypt password as the decrypted password will be set the original one.
 * The buffer length for encryptedPasswd and decryptedPasswd should be the same.
 */
bool LIBNETXMS_EXPORTABLE DecryptPasswordA(const char *login, const char *encryptedPasswd, char *decryptedPasswd, size_t bufferLenght)
{
   // check that lenght is correct
   size_t plen = strlen(encryptedPasswd);
   if ((plen != 44) && (plen != 88))
      return DecryptPasswordFailA(encryptedPasswd, decryptedPasswd, bufferLenght);

   // check that password contains only allowed symbols
   size_t invalidSymbolIndex = strspn(encryptedPasswd, "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/");
   if ((invalidSymbolIndex < plen - 2) || ((invalidSymbolIndex != plen) && ((encryptedPasswd[invalidSymbolIndex] != '=') || ((invalidSymbolIndex == plen - 2) && (encryptedPasswd[plen - 1] != '=')))))
      return DecryptPasswordFailA(encryptedPasswd, decryptedPasswd, bufferLenght);

   BYTE encrypted[64], decrypted[64], key[16];
   size_t encSize = ((plen == 44) ? 32 : 64);
   base64_decode(encryptedPasswd, strlen(encryptedPasswd), (char *)encrypted, &encSize);
   if (encSize != ((plen == 44) ? 32 : 64))
      return DecryptPasswordFailA(encryptedPasswd, decryptedPasswd, bufferLenght);

   CalculateMD5Hash((BYTE *)login, strlen(login), key);
   ICEDecryptData(encrypted, encSize, decrypted, key);
   decrypted[encSize - 1] = 0;

   strlcpy(decryptedPasswd, (char *)decrypted, bufferLenght);
   SecureZeroMemory(decrypted, sizeof(decrypted));
   return true;
}

/**
 * Scan file for specific data block
 */
bool LIBNETXMS_EXPORTABLE ScanFile(const TCHAR *fileName, const void *data, size_t size)
{
   int fh = _topen(fileName, O_RDONLY);
   if (fh == -1)
      return false;

   bool found = false;
   char buffer[8192];
   size_t offset = 0;
   while(true)
   {
      ssize_t bytes = _read(fh, &buffer[offset], 8192 - offset);
      if (bytes <= 0)
         break;

      if (bytes < static_cast<ssize_t>(size))
         break;

      if (memmem(buffer, bytes, data, size) != nullptr)
      {
         found = true;
         break;
      }

      memmove(buffer, &buffer[bytes - size + 1], size - 1);
      offset = size - 1;
   }

   _close(fh);
   return found;
}

/**
 * Load file content into memory (internal implementation)
 */
static BYTE *LoadFileContent(int fd, size_t *fileSize, bool kernelFS, bool stdInput)
{
   size_t size;
   if (stdInput)
   {
      size = 16384;
   }
   else
   {
      NX_STAT_STRUCT fs;
      if (NX_FSTAT(fd, &fs) == -1)
         return nullptr;

      size = static_cast<size_t>(fs.st_size);
#ifndef _WIN32
      if (kernelFS && (size == 0))
      {
          size = 16384;
      }
#endif
   }

   BYTE *buffer = static_cast<BYTE*>(MemAlloc(size + 1));
   if (buffer != nullptr)
   {
      *fileSize = size;
      int bytesRead = 0;
      for(size_t bufPos = 0; bufPos < size; bufPos += bytesRead)
      {
         size_t numBytes = std::min(static_cast<size_t>(16384), size - bufPos);
         if ((bytesRead = _read(fd, &buffer[bufPos], (int)numBytes)) < 0)
         {
            MemFreeAndNull(buffer);
            break;
         }
         // On Linux stat() for /proc files may report size larger than actual
         // so we check here for premature end of file
         if (bytesRead == 0)
         {
            buffer[bufPos] = 0;
            *fileSize = bufPos;
            break;
         }
#ifndef _WIN32
         if (kernelFS && (bufPos + bytesRead == size))
         {
            // Assume that file is larger than expected
            size += 16384;
            buffer = MemRealloc(buffer, size + 1);
         }
#endif
      }
      if (buffer != nullptr)
         buffer[size] = 0;
   }
   if (!stdInput)
      _close(fd);
   return buffer;
}

/**
 * Load file content into memory
 */
BYTE LIBNETXMS_EXPORTABLE *LoadFile(const TCHAR *fileName, size_t *fileSize)
{
   BYTE *buffer = nullptr;
   int fd = (fileName != nullptr) ? _topen(fileName, O_RDONLY | O_BINARY) : fileno(stdin);
   if (fd != -1)
   {
#ifdef _WIN32
      buffer = LoadFileContent(fd, fileSize, false, fileName == nullptr);
#else
      buffer = LoadFileContent(fd, fileSize, (fileName != nullptr) && !_tcsncmp(fileName, _T("/proc/"), 6), fileName == nullptr);
#endif
   }
   return buffer;
}

#ifdef UNICODE

/**
 * Load file content into memory
 */
BYTE LIBNETXMS_EXPORTABLE *LoadFileA(const char *fileName, size_t *fileSize)
{
   BYTE *buffer = nullptr;
   int fd = (fileName != nullptr) ? _open(fileName, O_RDONLY | O_BINARY) : fileno(stdin);
   if (fd != -1)
   {
#ifdef _WIN32
      buffer = LoadFileContent(fd, fileSize, false, fileName == nullptr);
#else
      buffer = LoadFileContent(fd, fileSize, (fileName != nullptr) && !strncmp(fileName, "/proc/", 6), fileName == nullptr);
#endif
   }
   return buffer;
}

#endif

/**
 * Load file into memory as string (with null byte at the end)
 */
char LIBNETXMS_EXPORTABLE *LoadFileAsUTF8String(const TCHAR *fileName)
{
   BYTE *buffer = nullptr;
   int fd = (fileName != nullptr) ? _topen(fileName, O_RDONLY | O_BINARY) : fileno(stdin);
   if (fd != -1)
   {
      size_t size;
#ifdef _WIN32
      buffer = LoadFileContent(fd, &size, false, fileName == nullptr);
#else
      buffer = LoadFileContent(fd, &size, (fileName != nullptr) && !_tcsncmp(fileName, _T("/proc/"), 6), fileName == nullptr);
#endif
   }
   return reinterpret_cast<char*>(buffer);
}

/**
 * Read first line from given file into buffer
 */
bool LIBNETXMS_EXPORTABLE ReadLineFromFileA(const char *path, char *buffer, size_t size)
{
   FILE *hFile = fopen(path, "r");
   if (hFile == nullptr)
      return false;

   bool success;
   if (fgets(buffer, size, hFile) != nullptr)
   {
      char *nl = strchr(buffer, '\n');
      if (nl != nullptr)
         *nl = 0;
      success = true;
   }
   else
   {
      success = false;
   }
   fclose(hFile);
   return success;
}

/**
 * Read first line from given file into buffer
 */
bool LIBNETXMS_EXPORTABLE ReadLineFromFileW(const WCHAR *path, WCHAR *buffer, size_t size)
{
   FILE *hFile = _wfopen(path, L"r");
   if (hFile == nullptr)
      return false;

   bool success;
   Buffer<char, 1024> mbBuffer(size);
   if (fgets(mbBuffer, size, hFile) != nullptr)
   {
      char *nl = strchr(mbBuffer, '\n');
      if (nl != nullptr)
         *nl = 0;
      MultiByteToWideCharSysLocale(mbBuffer, buffer, size);
      success = true;
   }
   else
   {
      success = false;
   }
   fclose(hFile);
   return success;
}

/**
 * Read 32 bit integer from file
 */
bool LIBNETXMS_EXPORTABLE ReadInt32FromFileA(const char *path, int32_t *value)
{
   char buffer[256];
   if (!ReadLineFromFileA(path, buffer, sizeof(buffer)))
      return false;

   char *eptr;
   *value = strtol(buffer, &eptr, 0);
   return *eptr == 0;
}

/**
 * Read 32 bit integer from file
 */
bool LIBNETXMS_EXPORTABLE ReadInt32FromFileW(const WCHAR *path, int32_t *value)
{
   WCHAR buffer[256];
   if (!ReadLineFromFileW(path, buffer, sizeof(buffer)))
      return false;

   WCHAR *eptr;
   *value = wcstol(buffer, &eptr, 0);
   return *eptr == 0;
}

/**
 * Read 64 bit unsigned integer from file
 */
bool LIBNETXMS_EXPORTABLE ReadUInt64FromFileA(const char *path, uint64_t *value)
{
   char buffer[256];
   if (!ReadLineFromFileA(path, buffer, sizeof(buffer)))
      return false;

   char *eptr;
   *value = strtoull(buffer, &eptr, 0);
   return *eptr == 0;
}

/**
 * Read 64 bit unsigned integer from file
 */
bool LIBNETXMS_EXPORTABLE ReadUInt64FromFileW(const WCHAR *path, uint64_t *value)
{
   WCHAR buffer[256];
   if (!ReadLineFromFileW(path, buffer, sizeof(buffer)))
      return false;

   WCHAR *eptr;
   *value = wcstoull(buffer, &eptr, 0);
   return *eptr == 0;
}

/**
 * Read double value from file
 */
bool LIBNETXMS_EXPORTABLE ReadDoubleFromFileA(const char *path, double *value)
{
   char buffer[256];
   if (!ReadLineFromFileA(path, buffer, sizeof(buffer) / sizeof(WCHAR)))
      return false;

   char *eptr;
   *value = strtod(buffer, &eptr);
   return *eptr == 0;
}

/**
 * Read double value from file
 */
bool LIBNETXMS_EXPORTABLE ReadDoubleFromFileW(const WCHAR *path, double *value)
{
   WCHAR buffer[256];
   if (!ReadLineFromFileW(path, buffer, sizeof(buffer) / sizeof(WCHAR)))
      return false;

   WCHAR *eptr;
   *value = wcstod(buffer, &eptr);
   return *eptr == 0;
}

/**
 * Save file atomically (do not replace existing file until all data is written)
 */
SaveFileStatus LIBNETXMS_EXPORTABLE SaveFile(const TCHAR *fileName, const void *data, size_t size, bool binary, bool removeCR)
{
   TCHAR tempFileName[MAX_PATH];
   _tcslcpy(tempFileName, fileName, MAX_PATH - 6);
   _tcslcat(tempFileName, _T(".part"), MAX_PATH);
   int hFile = _topen(tempFileName, O_CREAT | O_TRUNC | O_WRONLY | (binary ? O_BINARY : 0), 0644);
   if (hFile == -1)
      return SaveFileStatus::OPEN_ERROR;

   SaveFileStatus status = SaveFileStatus::SUCCESS;
   if (size > 0)
   {
      if (removeCR)
      {
         const BYTE *curr = static_cast<const BYTE*>(data);
         size_t pos = 0;
         while(pos < size)
         {
            auto next = curr;
            while((*next != 0x0D) && (pos < size))
            {
               next++;
               pos++;
            }
            int blockSize = static_cast<int>(next - curr);
            if (_write(hFile, curr, blockSize) != blockSize)
            {
               status = SaveFileStatus::WRITE_ERROR;
               break;
            }
            while((*next == 0x0D) && (pos < size))
            {
               next++;
               pos++;
            }
            curr = next;
         }
      }
      else
      {
         if (_write(hFile, data, static_cast<unsigned int>(size)) != static_cast<ssize_t>(size))
            status = SaveFileStatus::WRITE_ERROR;
      }
   }
   _close(hFile);

   if (status == SaveFileStatus::SUCCESS)
   {
#ifdef _WIN32
      if (!MoveFileEx(tempFileName, fileName, MOVEFILE_REPLACE_EXISTING))
#else
      if (_trename(tempFileName, fileName) != 0)
#endif
      {
         status = SaveFileStatus::RENAME_ERROR;
         _tremove(tempFileName);
      }
   }
   else
   {
      _tremove(tempFileName);
   }

   return status;
}

#ifdef _WIN32

/**
 * Get memory consumed by current process
 */
int64_t LIBNETXMS_EXPORTABLE GetProcessRSS()
{
	PROCESS_MEMORY_COUNTERS pmc;

	if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc)))
		return pmc.WorkingSetSize;
	return 0;
}

#define BG_MASK 0xF0
#define FG_MASK 0x0F

/**
 * Apply terminal attributes to console - Win32 API specific
 */
static WORD ApplyTerminalAttribute(HANDLE out, WORD currAttr, long code)
{
	WORD attr = currAttr;
	switch(code)
	{
		case 0:	// reset attribute
			attr = 0x07;
			break;
		case 1:	// bold/bright
			attr = currAttr | FOREGROUND_INTENSITY;
			break;
		case 22:	// normal color
			attr = currAttr & ~FOREGROUND_INTENSITY;
			break;
		case 30:	// black foreground
			attr = (currAttr & BG_MASK);
			break;
		case 31:	// red foreground
			attr = (currAttr & BG_MASK) | 0x04;
			break;
		case 32:	// green foreground
			attr = (currAttr & BG_MASK) | 0x02;
			break;
		case 33:	// yellow foreground
			attr = (currAttr & BG_MASK) | 0x06;
			break;
		case 34:	// blue foreground
			attr = (currAttr & BG_MASK) | 0x01;
			break;
		case 35:	// magenta foreground
			attr = (currAttr & BG_MASK) | 0x05;
			break;
		case 36:	// cyan foreground
			attr = (currAttr & BG_MASK) | 0x03;
			break;
		case 37:	// white (gray) foreground
			attr = (currAttr & BG_MASK) | 0x07;
			break;
		case 40:	// black background
			attr = (currAttr & FG_MASK);
			break;
		case 41:	// red background
			attr = (currAttr & FG_MASK) | 0x40;
			break;
		case 42:	// green background
			attr = (currAttr & FG_MASK) | 0x20;
			break;
		case 43:	// yellow background
			attr = (currAttr & FG_MASK) | 0x60;
			break;
		case 44:	// blue background
			attr = (currAttr & FG_MASK) | 0x10;
			break;
		case 45:	// magenta background
			attr = (currAttr & FG_MASK) | 0x50;
			break;
		case 46:	// cyan background
			attr = (currAttr & FG_MASK) | 0x30;
			break;
		case 47:	// white (gray) background
			attr = (currAttr & FG_MASK) | 0x70;
			break;
		default:
			break;
	}
	SetConsoleTextAttribute(out, attr);
	return attr;
}

#endif

#if defined(UNICODE) && (defined(_WIN32) || HAVE_FPUTWS)

/**
 * Write part of wide character string to output stream
 */
static void fputwsl(const WCHAR *text, const WCHAR *stop, FILE *out)
{
   const WCHAR *p = text;
   while(p < stop)
   {
      fputwc(*p, out);
      p++;
   }
}

/**
 * Write text with escape sequences to file
 */
static void WriteRedirectedTerminalOutputW(const WCHAR *text)
{
   const WCHAR *curr = text;
   while(*curr != 0)
   {
      const WCHAR *esc = wcschr(curr, 27);	// Find ESC
      if (esc != nullptr)
      {
         esc++;
         if (*esc == '[')
         {
            // write everything up to ESC char
            fputwsl(curr, esc - 1, stdout);
            esc++;
            while((*esc != 0) && (*esc != _T('m')))
               esc++;
            if (*esc == 0)
               break;
            esc++;
         }
         else
         {
            fputwsl(curr, esc, stdout);
         }
         curr = esc;
      }
      else
      {
         fputws(curr, stdout);
         break;
      }
   }
}

#endif

#if !defined(UNICODE) || (!HAVE_FPUTWS && !defined(_WIN32)) || HAVE_FWIDE

/**
 * Write text with escape sequences to file
 */
static void WriteRedirectedTerminalOutputA(const char *text)
{
   const char *curr = text;
   while(*curr != 0)
   {
      const char *esc = strchr(curr, 27);   // Find ESC
      if (esc != nullptr)
      {
         esc++;
         if (*esc == '[')
         {
            // write everything up to ESC char
            fwrite(curr, 1, esc - curr - 1, stdout);
            esc++;
            while((*esc != 0) && (*esc != 'm'))
               esc++;
            if (*esc == 0)
               break;
            esc++;
         }
         else
         {
            fwrite(curr, 1, esc - curr, stdout);
         }
         curr = esc;
      }
      else
      {
         fputs(curr, stdout);
         break;
      }
   }
}

#endif

/**
 * Write to terminal with support for ANSI color codes
 */
void LIBNETXMS_EXPORTABLE WriteToTerminal(const TCHAR *text)
{
#ifdef _WIN32
	HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);

   DWORD mode;
   if (!GetConsoleMode(out, &mode))
   {
      // Assume output is redirected
#ifdef UNICODE
      WriteRedirectedTerminalOutputW(text);
#else
      WriteRedirectedTerminalOutput(text);
#endif
      return;
   }

	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(out, &csbi);

	const TCHAR *curr = text;
	while(*curr != 0)
	{
		const TCHAR *esc = _tcschr(curr, 27);	// Find ESC
		if (esc != NULL)
		{
			esc++;
			if (*esc == _T('['))
			{
				// write everything up to ESC char
				DWORD chars;
				WriteConsole(out, curr, (UINT32)(esc - curr - 1), &chars, NULL);

				esc++;

				TCHAR code[64];
				int pos = 0;
				while((*esc != 0) && (*esc != _T('m')))
				{
					if (*esc == _T(';'))
					{
						code[pos] = 0;
						csbi.wAttributes = ApplyTerminalAttribute(out, csbi.wAttributes, _tcstol(code, NULL, 10));
						pos = 0;
					}
					else
					{
						if (pos < 63)
							code[pos++] = *esc;
					}
					esc++;
				}
            if (*esc == 0)
               break;
				if (pos > 0)
				{
					code[pos] = 0;
					csbi.wAttributes = ApplyTerminalAttribute(out, csbi.wAttributes, _tcstol(code, NULL, 10));
				}
				esc++;
			}
			else
			{
				DWORD chars;
				WriteConsole(out, curr, (UINT32)(esc - curr), &chars, nullptr);
			}
			curr = esc;
		}
		else
		{
			DWORD chars;
			WriteConsole(out, curr, (UINT32)_tcslen(curr), &chars, nullptr);
			break;
		}
	}
#else
#ifdef UNICODE
#if HAVE_FPUTWS
   if (isatty(fileno(stdout)))
   {
#if HAVE_FWIDE
      if (fwide(stdout, 0) < 0)
      {
         char *mbtext = MBStringFromWideStringSysLocale(text);
         fputs(mbtext, stdout);
         MemFree(mbtext);
      }
      else
#endif
	   fputws(text, stdout);
   }
   else
   {
#if HAVE_FWIDE
      if (fwide(stdout, 0) < 0)
      {
         char *mbtext = MBStringFromWideStringSysLocale(text);
         WriteRedirectedTerminalOutputA(mbtext);
         MemFree(mbtext);
      }
      else
#endif
      WriteRedirectedTerminalOutputW(text);
   }
#else
	char *mbtext = MBStringFromWideStringSysLocale(text);
   if (isatty(fileno(stdout)))
      fputs(mbtext, stdout);
   else
      WriteRedirectedTerminalOutputA(mbtext);
   MemFree(mbtext);
#endif
#else
   if (isatty(fileno(stdout)))
      fputs(text, stdout);
   else
      WriteRedirectedTerminalOutputA(text);
#endif
#endif
}

/**
 * Write to terminal with support for ANSI color codes
 */
void LIBNETXMS_EXPORTABLE WriteToTerminalEx(const TCHAR *format, ...)
{
	va_list args;
	va_start(args, format);
   TCHAR buffer[8192];
	_vsntprintf(buffer, 8192, format, args);
	va_end(args);
	buffer[8191] = 0;
	WriteToTerminal(buffer);
}

#if !HAVE_STRLWR && !defined(_WIN32)

/**
 * Convert non-UNICODE string to lowercase
 */
char LIBNETXMS_EXPORTABLE *strlwr(char *str)
{
   for(char *p = str; *p != 0; p++)
   {
      *p = tolower(*p);
   }
   return str;
}

#endif

#if !HAVE_WCSLWR && !defined(_WIN32)

/**
 * Convert UNICODE string to lowercase
 */
WCHAR LIBNETXMS_EXPORTABLE *wcslwr(WCHAR *str)
{
   for(WCHAR *p = str; *p != 0; p++)
   {
      *p = towlower(*p);
   }
   return str;
}

#endif

#if !defined(_WIN32) && (!HAVE_WCSFTIME || !WORKING_WCSFTIME)

/**
 * wide char version of strftime
 */
size_t LIBNETXMS_EXPORTABLE nx_wcsftime(WCHAR *buffer, size_t bufsize, const WCHAR *format, const struct tm *t)
{
#if HAVE_ALLOCA
   char *mbuf = (char *)alloca(bufsize);
   size_t flen = wcslen(format) + 1;
   char *mfmt = (char *)alloca(flen);
   WideCharToMultiByteSysLocale(format, mfmt, flen);
#else
   char *mbuf = (char *)MemAlloc(bufsize);
   char *mfmt = MBStringFromWideString(format);
#endif
   size_t rc = strftime(mbuf, bufsize, mfmt, t);
   if (rc > 0)
   {
      MultiByteToWideCharSysLocale(mbuf, buffer, (int)bufsize);
      buffer[bufsize - 1] = 0;
   }
   else
   {
      buffer[0] = 0;
   }
#if !HAVE_ALLOCA
   MemFree(mbuf);
   MemFree(mfmt);
#endif
   return rc;
}

#endif

/**
 * Get sleep time until specific time
 */
int LIBNETXMS_EXPORTABLE GetSleepTime(int hour, int minute, int second)
{
   time_t now = time(NULL);

   struct tm localTime;
#if HAVE_LOCALTIME_R
   localtime_r(&now, &localTime);
#else
   memcpy(&localTime, localtime(&now), sizeof(struct tm));
#endif

   int target = hour * 3600 + minute * 60 + second;
   int curr = localTime.tm_hour * 3600 + localTime.tm_min * 60 + localTime.tm_sec;
   return (target >= curr) ? target - curr : 86400 - (curr - target);
}

/**
 * Parse timestamp (should be in form YYMMDDhhmmss or YYYYMMDDhhmmss), local or UTC time
 * If timestamp string is invalid returns default value
 */
time_t LIBNETXMS_EXPORTABLE ParseDateTimeA(const char *text, time_t defaultValue, bool utc)
{
	size_t len = strlen(text);
	if ((len != 12) && (len != 14))
		return defaultValue;

	struct tm t;
	char buffer[16], *curr;

	memcpy(buffer, text, len + 1);
	curr = &buffer[len - 2];

	memset(&t, 0, sizeof(struct tm));
	t.tm_isdst = utc ? 0 : -1;

	// Disable incorrect warning, probably caused by gcc bug 106757
	// https://gcc.gnu.org/bugzilla/show_bug.cgi?id=106757
#if __GNUC__ == 13
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-overflow"
#endif

	t.tm_sec = strtol(curr, nullptr, 10);
	*curr = 0;
	curr -= 2;

	t.tm_min = strtol(curr, nullptr, 10);
	*curr = 0;
	curr -= 2;

	t.tm_hour = strtol(curr, nullptr, 10);
	*curr = 0;
	curr -= 2;

	t.tm_mday = strtol(curr, nullptr, 10);
	*curr = 0;
	curr -= 2;

	t.tm_mon = strtol(curr, nullptr, 10) - 1;
	*curr = 0;

	if (len == 12)
	{
		curr -= 2;
		t.tm_year = strtol(curr, nullptr, 10) + 100;	// Assuming XXI century
	}
	else
	{
		curr -= 4;
		t.tm_year = strtol(curr, nullptr, 10) - 1900;
	}

#if __GNUC__ == 13
#pragma GCC diagnostic pop
#endif

	return utc ? timegm(&t) : mktime(&t);
}

/**
 * Parse timestamp (should be in form YYMMDDhhmmss or YYYYMMDDhhmmss), local time
 * If timestamp string is invalid returns default value
 * (UNICODE version)
 */
time_t LIBNETXMS_EXPORTABLE ParseDateTimeW(const WCHAR *text, time_t defaultValue, bool utc)
{
   char buffer[16];
   wchar_to_mb(text, -1, buffer, 16);
   buffer[15] = 0;
   return ParseDateTimeA(buffer, defaultValue, utc);
}

/**
 * Data directory override
 */
static TCHAR *s_dataDirectory = nullptr;

/**
 * Set NetXMS data directory (can be used if default location is overridden by process).
 * This function is not thread safe and should be called from main thread before other
 * threads will call GetNetXMSDirectory.
 */
void LIBNETXMS_EXPORTABLE SetNetXMSDataDirectory(const TCHAR *dir)
{
   MemFree(s_dataDirectory);
   s_dataDirectory = MemCopyString(dir);
}

#ifdef _WIN32

/**
 * Get installation path from registry
 */
static inline bool GetInstallationPath(const TCHAR *prefix, TCHAR *installPath)
{
   TCHAR keyName[128] = _T("Software\\NetXMS\\");
   _tcscat(keyName, prefix);

   HKEY hKey;
   bool found = false;
   if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, keyName, 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
   {
      DWORD size = MAX_PATH * sizeof(TCHAR);
      found = (RegQueryValueEx(hKey, _T("InstallPath"), nullptr, nullptr, (BYTE *)installPath, &size) == ERROR_SUCCESS);
      RegCloseKey(hKey);
   }
   return found;
}

#endif

/**
 * Get NetXMS directory
 */
void LIBNETXMS_EXPORTABLE GetNetXMSDirectory(nxDirectoryType type, TCHAR *dir)
{
   if ((type == nxDirData) && (s_dataDirectory != nullptr))
   {
      _tcslcpy(dir, s_dataDirectory, MAX_PATH);
      return;
   }

   *dir = 0;

   String homeDir = GetEnvironmentVariableEx(_T("NETXMS_HOME"));
   if (!homeDir.isEmpty())
   {
#ifdef _WIN32
      switch(type)
      {
         case nxDirBin:
            _sntprintf(dir, MAX_PATH, _T("%s\\bin"), homeDir.cstr());
            break;
         case nxDirData:
            _sntprintf(dir, MAX_PATH, _T("%s\\var"), homeDir.cstr());
            break;
         case nxDirEtc:
            _sntprintf(dir, MAX_PATH, _T("%s\\etc"), homeDir.cstr());
            break;
         case nxDirLib:
            _sntprintf(dir, MAX_PATH, _T("%s\\lib"), homeDir.cstr());
            break;
         case nxDirShare:
            _sntprintf(dir, MAX_PATH, _T("%s\\share"), homeDir.cstr());
            break;
         default:
            _tcslcpy(dir, homeDir, MAX_PATH);
            break;
      }
#else
      switch(type)
      {
         case nxDirBin:
            _sntprintf(dir, MAX_PATH, _T("%s/bin"), homeDir.cstr());
            break;
         case nxDirData:
            _sntprintf(dir, MAX_PATH, _T("%s/var/lib/netxms"), homeDir.cstr());
            break;
         case nxDirEtc:
            _sntprintf(dir, MAX_PATH, _T("%s/etc"), homeDir.cstr());
            break;
         case nxDirLib:
            _sntprintf(dir, MAX_PATH, _T("%s/lib/netxms"), homeDir.cstr());
            break;
         case nxDirShare:
            _sntprintf(dir, MAX_PATH, _T("%s/share/netxms"), homeDir.cstr());
            break;
         default:
            _tcslcpy(dir, homeDir, MAX_PATH);
            break;
      }
#endif
      return;
   }

#ifdef _WIN32
   TCHAR installPath[MAX_PATH] = _T("");
   bool found = GetInstallationPath(s_isClientApp ? _T("Client") : _T("Server"), installPath);
   if (!found)
      found = GetInstallationPath(_T("Agent"), installPath);
   if (!found && s_isClientApp)
      found = GetInstallationPath(_T("Server"), installPath);

   if (!found && (GetModuleFileName(nullptr, installPath, MAX_PATH) > 0))
   {
      TCHAR *p = _tcsrchr(installPath, _T('\\'));
      if (p != nullptr)
      {
         *p = 0;
         p = _tcsrchr(installPath, _T('\\'));
         if (p != nullptr)
         {
            // If last path element is not \bin consider that app binary is installed directly to base
            if (!_tcsicmp(p, _T("\\bin")))
               *p = 0;
            found = true;
         }
      }
   }

   if (!found)
   {
      _tcscpy(installPath, _T("C:\\NetXMS"));
   }

   switch(type)
   {
      case nxDirBin:
         _sntprintf(dir, MAX_PATH, _T("%s\\bin"), installPath);
         break;
      case nxDirData:
         _sntprintf(dir, MAX_PATH, _T("%s\\var"), installPath);
         break;
      case nxDirEtc:
         _sntprintf(dir, MAX_PATH, _T("%s\\etc"), installPath);
         break;
      case nxDirLib:
         _sntprintf(dir, MAX_PATH, _T("%s\\lib"), installPath);
         break;
      case nxDirShare:
         _sntprintf(dir, MAX_PATH, _T("%s\\share"), installPath);
         break;
      default:
         _tcslcpy(dir, installPath, MAX_PATH);
         break;
   }
#else
   switch(type)
   {
      case nxDirBin:
#ifdef BINDIR
         _tcscpy(dir, BINDIR);
#else
         _tcscpy(dir, _T("/usr/bin"));
#endif
         break;
      case nxDirData:
#ifdef STATEDIR
         _tcscpy(dir, STATEDIR);
#else
         _tcscpy(dir, _T("/var/lib/netxms"));
#endif
         break;
      case nxDirEtc:
#ifdef SYSCONFDIR
         _tcscpy(dir, SYSCONFDIR);
#else
         _tcscpy(dir, _T("/etc"));
#endif
         break;
      case nxDirLib:
#ifdef PKGLIBDIR
         _tcscpy(dir, PKGLIBDIR);
#else
         _tcscpy(dir, _T("/usr/lib/netxms"));
#endif
         break;
      case nxDirShare:
#ifdef DATADIR
         _tcscpy(dir, DATADIR);
#else
         _tcscpy(dir, _T("/usr/share/netxms"));
#endif
         break;
      default:
         _tcscpy(dir, _T("/usr"));
         break;
   }
#endif
}

#if WITH_JEMALLOC

/**
 * Callback for jemalloc's malloc_stats_print
 */
static void jemalloc_stats_cb(void *arg, const char *text)
{
   fwrite(text, 1, strlen(text), (FILE *)arg);
}

#endif

/**
 * Get heap information using system-specific functions (if available)
 */
TCHAR LIBNETXMS_EXPORTABLE *GetHeapInfo()
{
#if WITH_JEMALLOC || HAVE_MALLOC_INFO
   char *buffer = nullptr;
   size_t size = 0;
   FILE *f = open_memstream(&buffer, &size);
   if (f == nullptr)
      return nullptr;
#if WITH_JEMALLOC
   malloc_stats_print(jemalloc_stats_cb, f, nullptr);
#else
   malloc_info(0, f);
#endif
   fclose(f);
#ifdef UNICODE
   WCHAR *wtext = WideStringFromMBString(buffer);
   MemFree(buffer);
   return wtext;
#else
   return buffer;
#endif
#else
   return MemCopyString(_T("No heap information API available"));
#endif
}

#ifdef WITH_JEMALLOC

/**
 * Get jemalloc stat
 */
static int64_t GetJemallocStat(const char *name)
{
   size_t value;
   size_t size = sizeof(value);
   if (mallctl(name, &value, &size, nullptr, 0) == 0)
      return value;
   return -1;
}

#endif

/**
 * Get amount of memory allocated by process on heap
 */
int64_t LIBNETXMS_EXPORTABLE GetAllocatedHeapMemory()
{
#ifdef WITH_JEMALLOC
   return GetJemallocStat("stats.allocated");
#endif
   return -1;
}

/**
 * Get amount of memory in active heap pages
 */
int64_t LIBNETXMS_EXPORTABLE GetActiveHeapMemory()
{
#ifdef WITH_JEMALLOC
   return GetJemallocStat("stats.active");
#endif
   return -1;
}

/**
 * Get amount of memory mapped for heap
 */
int64_t LIBNETXMS_EXPORTABLE GetMappedHeapMemory()
{
#ifdef WITH_JEMALLOC
   return GetJemallocStat("stats.mapped");
#endif
   return -1;
}

/**
 * Escape string for JSON
 */
String LIBNETXMS_EXPORTABLE EscapeStringForJSON(const TCHAR *s)
{
   StringBuffer js;
   if (s == NULL)
      return js;
   for(const TCHAR *p = s; *p != 0; p++)
   {
      if (*p == _T('\r'))
      {
         js.append(_T("\\r"));
      }
      else if (*p == _T('\n'))
      {
         js.append(_T("\\n"));
      }
      else if (*p == _T('\t'))
      {
         js.append(_T("\\t"));
      }
      else
      {
         if (*p == _T('"') || *p == _T('\\'))
            js.append(_T('\\'));
         js.append(*p);
      }
   }
   return js;
}

/**
 * Convert JSON string in ISO 8601 format or integer to UNIX timestamp
 */
time_t LIBNETXMS_EXPORTABLE json_object_get_time(json_t *object, const char *tag, time_t defval)
{
   json_t *e = json_object_get(object, tag);
   if (json_is_integer(e))
      return static_cast<time_t>(json_integer_value(e)); // Assume that integer value represents UNIX timestamp

   if (!json_is_string(e))
      return defval;

   struct tm t;
   if (strptime(json_string_value(e), "%Y-%m-%dT%H:%M:%SZ", &t) == nullptr)
      return defval;

   return timegm(&t);
}

/**
 * Get JSON object property as UUID
 */
uuid LIBNETXMS_EXPORTABLE json_object_get_uuid(json_t *object, const char *tag)
{
   json_t *value = json_object_get(object, tag);
   return json_is_string(value) ? uuid::parseA(json_string_value(value)) : uuid::NULL_UUID;
}

/**
 * Get element from object by path (separated by /)
 */
json_t LIBNETXMS_EXPORTABLE *json_object_get_by_path_a(json_t *root, const char *path)
{
   if ((path[0] == 0) || ((path[0] == '/') && (path[1] == 0)))
      return root;   // points to root object itself

   if (*path == '/')
      path++;

   json_t *object = root;
   char name[128];
   const char *separator = nullptr;
   do
   {
      separator = strchr(path, '/');
      if (separator != nullptr)
      {
         size_t len = separator - path;
         if (len > 127)
            len = 127;
         memcpy(name, path, len);
         name[len] = 0;
         object = json_object_get(object, name);
         path = separator + 1;
      }
      else
      {
         object = json_object_get(object, path); // last element in path
      }
   } while ((separator != nullptr) && (*path != 0) && (object != nullptr));
   return object;
}

/**
 * Escape string for agent parameter
 */
String LIBNETXMS_EXPORTABLE EscapeStringForAgent(const TCHAR *s)
{
   StringBuffer out;
   if (s == NULL)
      return out;
   for(const TCHAR *p = s; *p != 0; p++)
   {
      if (*p == _T('"'))
         out.append(_T('"'));
      out.append(*p);
   }
   return out;
}

/**
 * Parse command line into argument list considering single and double quotes
 */
StringList LIBNETXMS_EXPORTABLE *ParseCommandLine(const TCHAR *cmdline)
{
   StringList *args = new StringList();

   TCHAR *temp = _tcsdup(cmdline);
   int state = 0;

   TCHAR *curr = temp;
   while(*curr == ' ')
      curr++;

   if (*curr != 0)
   {
      int len = (int)_tcslen(temp);
      for(int i = (int)(curr - temp); i < len; i++)
      {
         switch(temp[i])
         {
            case ' ':
               if (state == 0)
               {
                  temp[i] = 0;
                  args->add(curr);
                  while(temp[i + 1] == ' ')
                     i++;
                  curr = &temp[i + 1];
               }
               break;
            case '"':
               if (state == 2)
                  break;   // within single quoted string
               if (state == 0)
               {
                  state = 1;
               }
               else
               {
                  state = 0;
               }
               memmove(&temp[i], &temp[i + 1], (len - i) * sizeof(TCHAR));
               i--;
               break;
            case '\'':
               if (state == 1)
                  break;   // within double quoted string
               if (state == 0)
               {
                  state = 2;
               }
               else
               {
                  state = 0;
               }
               memmove(&temp[i], &temp[i + 1], (len - i) * sizeof(TCHAR));
               i--;
               break;
            default:
               break;
         }
      }

      if (*curr != 0)
         args->add(curr);
   }
   MemFree(temp);
   return args;
}

/**
 * Read password from terminal
 */
bool LIBNETXMS_EXPORTABLE ReadPassword(const TCHAR *prompt, TCHAR *buffer, size_t bufferSize)
{
   if (prompt != nullptr)
   {
      WriteToTerminal(prompt);
      fflush(stdout);
   }

   /* Turn echoing off and fail if we can’t. */
#ifdef WIN32
   HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE); 
   DWORD mode;
   GetConsoleMode(hStdin, &mode);

   mode &= ~ENABLE_ECHO_INPUT;
   SetConsoleMode(hStdin, mode);
#else
   struct termios ts;
   if (tcgetattr(fileno(stdin), &ts) != 0)
      return false;

   ts.c_lflag &= ~ECHO;
   if (tcsetattr(fileno(stdin), TCSAFLUSH, &ts) != 0)
      return false;
#endif

   /* Read the password. */
   if (_fgetts(buffer, (int)bufferSize, stdin) != nullptr)
   {
      TCHAR *nl = _tcschr(buffer, _T('\n'));
      if (nl != nullptr)
         *nl = 0;
   }

   /* Restore terminal. */
#ifdef _WIN32
   mode |= ENABLE_ECHO_INPUT;
   SetConsoleMode(hStdin, mode);
#else
   ts.c_lflag |= ECHO;
   tcsetattr(fileno(stdin), TCSAFLUSH, &ts);
#endif

   WriteToTerminal(_T("\n"));
   return true;
}

/**
 * Get system name
 */
TCHAR LIBNETXMS_EXPORTABLE *GetLocalHostName(TCHAR *buffer, size_t size, bool fqdn)
{
   *buffer = 0;
#ifdef _WIN32
   DWORD s = (DWORD)size;
   return GetComputerNameEx(fqdn ? ComputerNamePhysicalDnsFullyQualified : ComputerNamePhysicalDnsHostname, buffer, &s) ? buffer : NULL;
#else    /* _WIN32 */
   char hostname[256];
   if (gethostname(hostname, 256) != 0)
      return nullptr;
   if (fqdn)
   {
#if HAVE_GETADDRINFO
      struct addrinfo hints;
      memset(&hints, 0, sizeof hints);
      hints.ai_family = AF_UNSPEC; /*either IPV4 or IPV6*/
      hints.ai_socktype = SOCK_STREAM;
      hints.ai_flags = AI_CANONNAME;

      struct addrinfo *info;
      if (getaddrinfo(hostname, "http", &hints, &info) != 0)
         return nullptr;

      bool found = false;
      for(struct addrinfo *p = info; p != nullptr; p = p->ai_next)
      {
         if ((p->ai_canonname != nullptr) && (strchr(p->ai_canonname, '.') != nullptr))
         {
#ifdef UNICODE
            MultiByteToWideCharSysLocale(p->ai_canonname, buffer, size);
#else
            strncpy(buffer, p->ai_canonname, size);
#endif
            found = true;
            break;
         }
      }

      if (!found && (info != nullptr))
      {
         // Use first available name as last resort
#ifdef UNICODE
         MultiByteToWideCharSysLocale(info->ai_canonname, buffer, size);
#else
         strlcpy(buffer, info->ai_canonname, size);
#endif
         found = true;
      }

      freeaddrinfo(info);
      if (!found)
         return nullptr;
#else    /* HAVE_GETADDRINFO */
      struct hostent *h = gethostbyname(hostname);
      if (h == nullptr)
         return nullptr;
#ifdef UNICODE
      MultiByteToWideCharSysLocale(h->h_name, buffer, size);
#else
      strncpy(buffer, h->h_name, size);
#endif
#endif   /* HAVE_GETADDRINFO */
   }
   else
   {
      // some systems return FQDN in gethostname call
      char *p = strchr(hostname, '.');
      if (p != nullptr)
         *p = 0;
#ifdef UNICODE
      MultiByteToWideCharSysLocale(hostname, buffer, size);
#else
      strncpy(buffer, hostname, size);
#endif
   }
   buffer[size - 1] = 0;
   return buffer;
#endif   /* _WIN32 */
}

#ifdef _WIN32

/**
 * stat() implementaion for Windows
 */
int LIBNETXMS_EXPORTABLE _statw32(const WCHAR *file, struct _stati64 *st)
{
   size_t l = _tcslen(file);

   // Special handling for root directory
   if (((l == 2) && (file[1] == L':')) ||
       ((l == 3) && (file[1] == L':') && (file[2] == L'\\')))
   {
      if (l == 2)
      {
         WCHAR temp[4];
         temp[0] = file[0];
         temp[1] = L':';
         temp[2] = L'\\';
         temp[3] = 0;
         return _wstati64(temp, st);
      }
      return _wstati64(file, st);
   }

   WCHAR *fn;
   if ((l > 1) && (file[l - 1] == L'\\'))
   {
      fn = (WCHAR *)alloca(l * sizeof(TCHAR));
      memcpy(fn, file, (l - 1) * sizeof(TCHAR));
      fn[l - 1] = 0;
   }
   else
   {
      fn = const_cast<WCHAR*>(file);
   }

   WIN32_FIND_DATA fd;
   HANDLE h = FindFirstFile(fn, &fd);
   if (h == INVALID_HANDLE_VALUE)
   {
      _set_errno((GetLastError() == ERROR_FILE_NOT_FOUND) ? ENOENT : EIO);
      return -1;
   }

   memset(st, 0, sizeof(struct _stati64));
   st->st_mode = _S_IREAD;
   if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_READONLY))
      st->st_mode |= _S_IWRITE;
   if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
      st->st_mode |= _S_IFDIR;
   else if (fd.dwFileAttributes & FILE_ATTRIBUTE_DEVICE)
      st->st_mode |= _S_IFCHR;
   else
      st->st_mode |= _S_IFREG;

   st->st_size = (static_cast<INT64>(fd.nFileSizeHigh) << 32) + fd.nFileSizeLow;
   st->st_atime = FileTimeToUnixTime(fd.ftLastAccessTime);
   st->st_ctime = FileTimeToUnixTime(fd.ftCreationTime);
   st->st_mtime = FileTimeToUnixTime(fd.ftLastWriteTime);

   FindClose(h);
   return 0;
}

#endif   /* _WIN32 */

#ifndef _WIN32

/**
 * Copy file
 */
static bool CopyFileInternal(const TCHAR *src, const TCHAR *dst, int mode, bool append = false)
{
   int oldFile = _topen(src, O_RDONLY | O_BINARY);
   if (oldFile == -1)
      return false;

   int appendMode = append ? O_APPEND : 0;
   int newFile = _topen(dst, O_CREAT | O_WRONLY | O_BINARY | appendMode, mode); // should be copied with the same access rights
   if (newFile == -1)
   {
      _close(oldFile);
      return false;
   }

   int in, out;
   BYTE buffer[16384];
   while ((in = _read(oldFile, buffer, sizeof(buffer))) > 0)
   {
      out = _write(newFile, buffer, in);
      if (out != in)
      {
         _close(oldFile);
         _close(newFile);
         return false;
      }
   }

   _close(oldFile);
   _close(newFile);
   return true;
}

#endif

/**
 * Merge files
 */
bool LIBNETXMS_EXPORTABLE MergeFiles(const TCHAR *source, const TCHAR *destination)
{
   bool success = false;
#ifdef _WIN32
   HANDLE sourceFile = CreateFile(source, FILE_READ_DATA, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (sourceFile == INVALID_HANDLE_VALUE)
        return false;

    HANDLE destinationFile = CreateFile(destination, FILE_APPEND_DATA, 0, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (destinationFile == INVALID_HANDLE_VALUE)
    {
       CloseHandle(sourceFile);
       return false;
    }

    BYTE buffer[4096];
    DWORD bytesRead = 0;
    DWORD bytesWritten = 0;
    ReadFile(sourceFile, buffer, 4096, &bytesRead, nullptr);
    while (bytesRead > 0)
    {
       success = WriteFile( destinationFile, buffer, bytesRead, &bytesWritten, nullptr);
       if (!success)
          break;
       ReadFile(sourceFile, buffer, 4096, &bytesRead, nullptr);
    }

    CloseHandle(sourceFile);
    CloseHandle(destinationFile);
#else
   success = CopyFileInternal(source, destination, S_IRUSR | S_IWUSR, true);
#endif
   return success;
}

/**
 * Counts files in given directory considering the filter
 * @param dir Directory path.
 * @param filter File name filter. Can be 'nullptr' for no filtering.
 * @return Returns -1 if file reading fails. Otherwise returns file count.
 */
int LIBNETXMS_EXPORTABLE CountFilesInDirectoryA(const char *path, bool (*filter)(const struct dirent *))
{
   DIR *dir = opendir(path);
   if (dir == nullptr)
      return -1;

   int i = 0;
   struct dirent *d;
   while ((d = readdir(dir)) != nullptr)
   {
      if (!strcmp(d->d_name, ".") || !strcmp(d->d_name, ".."))
         continue;
      if ((filter == nullptr) || filter(d))
      {
         i++;
      }
   }
   closedir(dir);
   return i;
}

/**
 * Counts files in given directory considering the filter (wide character version).
 *
 * @param dir Directory path.
 * @param filter File name filter. Can be 'nullptr' for no filtering.
 * @return Returns -1 if file reading fails. Otherwise returns file count.
 */
int LIBNETXMS_EXPORTABLE CountFilesInDirectoryW(const WCHAR *path, bool (*filter)(const struct dirent_w *))
{
   DIRW *dir = wopendir(path);
   if (dir == nullptr)
      return -1;

   int i = 0;
   struct dirent_w *d;
   while ((d = wreaddir(dir)) != nullptr)
   {
      if (!wcscmp(d->d_name, L".") || !wcscmp(d->d_name, L".."))
         continue;
      if ((filter == nullptr) || filter(d))
      {
         i++;
      }
   }
   wclosedir(dir);
   return i;
}

/**
 * Copy file/folder
 */
bool LIBNETXMS_EXPORTABLE CopyFileOrDirectory(const TCHAR *oldName, const TCHAR *newName)
{
   NX_STAT_STRUCT st;
   if (CALL_STAT_FOLLOW_SYMLINK(oldName, &st) != 0)
      return false;

   if (!S_ISDIR(st.st_mode))
   {
#ifdef _WIN32
      return CopyFile(oldName, newName, FALSE);
#else
      return CopyFileInternal(oldName, newName, st.st_mode);
#endif
   }

#ifdef _WIN32
   if (!CreateDirectory(newName, nullptr))
      return false;
#else
   if (_tmkdir(newName, st.st_mode) != 0)
      return false;
#endif

   _TDIR *dir = _topendir(oldName);
   if (dir == nullptr)
      return false;

   struct _tdirent *d;
   while ((d = _treaddir(dir)) != nullptr)
   {
      if (!_tcscmp(d->d_name, _T(".")) || !_tcscmp(d->d_name, _T("..")))
         continue;

      TCHAR nextNewName[MAX_PATH];
      _tcscpy(nextNewName, newName);
      _tcscat(nextNewName, FS_PATH_SEPARATOR);
      _tcscat(nextNewName, d->d_name);

      TCHAR nextOldaName[MAX_PATH];
      _tcscpy(nextOldaName, oldName);
      _tcscat(nextOldaName, FS_PATH_SEPARATOR);
      _tcscat(nextOldaName, d->d_name);

      CopyFileOrDirectory(nextOldaName, nextNewName);
   }

   _tclosedir(dir);
   return true;
}

/**
 * Move file/folder
 */
bool LIBNETXMS_EXPORTABLE MoveFileOrDirectory(const TCHAR *oldName, const TCHAR *newName)
{
#ifdef _WIN32
   if (MoveFileEx(oldName, newName, MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING))
      return true;
#else
   if (_trename(oldName, newName) == 0)
      return true;
#endif

   NX_STAT_STRUCT st;
   if (CALL_STAT_FOLLOW_SYMLINK(oldName, &st) != 0)
      return false;

   if (S_ISDIR(st.st_mode))
   {
#ifdef _WIN32
      CreateDirectory(newName, nullptr);
#else
      _tmkdir(newName, st.st_mode);
#endif
      _TDIR *dir = _topendir(oldName);
      if (dir != nullptr)
      {
         struct _tdirent *d;
         while((d = _treaddir(dir)) != nullptr)
         {
            if (!_tcscmp(d->d_name, _T(".")) || !_tcscmp(d->d_name, _T("..")))
               continue;
            
            TCHAR nextNewName[MAX_PATH];
            _tcscpy(nextNewName, newName);
            _tcscat(nextNewName, FS_PATH_SEPARATOR);
            _tcscat(nextNewName, d->d_name);

            TCHAR nextOldaName[MAX_PATH];
            _tcscpy(nextOldaName, oldName);
            _tcscat(nextOldaName, FS_PATH_SEPARATOR);
            _tcscat(nextOldaName, d->d_name);

            MoveFileOrDirectory(nextOldaName, nextNewName);
         }
         _tclosedir(dir);
      }
      _trmdir(oldName);
   }
   else
   {
#ifdef _WIN32
      if (!CopyFile(oldName, newName, FALSE))
         return false;
#else
      if (!CopyFileInternal(oldName, newName, st.st_mode))
         return false;
#endif
      _tremove(oldName);
   }
   return true;
}

/**
 * Verify file signature
 */
bool LIBNETXMS_EXPORTABLE VerifyFileSignature(const TCHAR *file)
{
#ifdef _WIN32
   HKEY hKey;
   if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("Software\\NetXMS"), 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
   {
      DWORD value = 0;
      DWORD size = sizeof(DWORD);
      if (RegQueryValueEx(hKey, _T("DisableFileSignatureVerification"), nullptr, nullptr, reinterpret_cast<BYTE*>(&value), &size) == ERROR_SUCCESS)
      {
         if (value != 0)
            return true;
      }
      RegCloseKey(hKey);
   }

   WINTRUST_FILE_INFO fi;
   fi.cbStruct = sizeof(WINTRUST_FILE_INFO);
#ifdef UNICODE
   fi.pcwszFilePath = file;
#else
   WCHAR wfile[MAX_PATH];
   MultiByteToWideCharSysLocale(file, wfile, MAX_PATH);
   fi.pcwszFilePath = wfile;
#endif
   fi.hFile = NULL;
   fi.pgKnownSubject = NULL;

   GUID action = WINTRUST_ACTION_GENERIC_VERIFY_V2;

   WINTRUST_DATA wd;
   memset(&wd, 0, sizeof(WINTRUST_DATA));
   wd.cbStruct = sizeof(WINTRUST_DATA);
   wd.dwUIChoice = WTD_UI_NONE;
   wd.fdwRevocationChecks = WTD_REVOKE_NONE;
   wd.dwUnionChoice = WTD_CHOICE_FILE;
   wd.dwStateAction = WTD_STATEACTION_VERIFY;
   wd.pFile = &fi;

   LONG status = WinVerifyTrust((HWND)INVALID_HANDLE_VALUE, &action, &wd);

   wd.dwStateAction = WTD_STATEACTION_CLOSE;
   WinVerifyTrust((HWND)INVALID_HANDLE_VALUE, &action, &wd);

   return status == 0;
#else
   return false;
#endif
}

/**
 * Do TCP "ping"
 */
TcpPingResult LIBNETXMS_EXPORTABLE TcpPing(const InetAddress& addr, UINT16 port, UINT32 timeout)
{
   SOCKET s = CreateSocket(addr.getFamily(), SOCK_STREAM, 0);
   if (s == INVALID_SOCKET)
      return TCP_PING_SOCKET_ERROR;

   TcpPingResult result;
   bool isTimeout;
   SockAddrBuffer sb;
   addr.fillSockAddr(&sb, port);
   if (ConnectEx(s, reinterpret_cast<struct sockaddr*>(&sb), SA_LEN(reinterpret_cast<struct sockaddr*>(&sb)), timeout, &isTimeout) == 0)
   {
      result = TCP_PING_SUCCESS;
      shutdown(s, SHUT_RDWR);
   }
   else
   {
      if (isTimeout)
      {
         result = TCP_PING_TIMEOUT;
      }
      else
      {
#ifdef _WIN32
         result = (WSAGetLastError() == WSAECONNREFUSED) ? TCP_PING_REJECT : TCP_PING_SOCKET_ERROR;
#else
         unsigned int err;
#if GETSOCKOPT_USES_SOCKLEN_T
         socklen_t len = sizeof(int);
#else
         unsigned int len = sizeof(int);
#endif
         if (getsockopt(s, SOL_SOCKET, SO_ERROR, &err, &len) == 0)
         {
            result = (err == ECONNREFUSED) ? TCP_PING_REJECT : TCP_PING_SOCKET_ERROR;
         }
         else
         {
            result = TCP_PING_SOCKET_ERROR;
         }
#endif
      }
   }

   closesocket(s);
   return result;
}

#ifndef _WIN32

/**
 * Unset environment variable
 */
static BOOL UnsetEnvironmentVariable(const TCHAR *var)
{
#if HAVE_UNSETENV

#ifdef UNICODE
   char *mbenv = MBStringFromWideStringSysLocale(var);
   BOOL result = (unsetenv(mbenv) == 0);
   MemFree(mbenv);
   return result;
#else
   return unsetenv(var) == 0;
#endif

#else //HAVE_UNSETENV

   size_t len = _tcslen(var) + 1;
   TCHAR *env = MemAllocString(len);
   _sntprintf(env, len, _T("%s"), var);
#ifdef UNICODE
   char *mbenv = MBStringFromWideStringSysLocale(env);
   MemFree(env);
   return putenv(mbenv) == 0;
#else
   return putenv(env) == 0;
#endif

#endif //HAVE_UNSETENV
}

/**
 * SetEnvironmentVariable implementation for non-Windows environments
 */
BOOL LIBNETXMS_EXPORTABLE SetEnvironmentVariable(const TCHAR *var, const TCHAR *value)
{
   if (value == nullptr)
      return UnsetEnvironmentVariable(var);

#if HAVE_SETENV

#ifdef UNICODE
   char *mbenv = MBStringFromWideStringSysLocale(var);
   char *mvalue = MBStringFromWideStringSysLocale(value);
   BOOL result = setenv(mbenv, mvalue, 1) == 0;
   MemFree(mbenv);
   MemFree(mvalue);
   return result;
#else
   return setenv(var, value, 1) == 0;
#endif

#else //HAVE_SETENV

   size_t len = _tcslen(var) + _tcslen(value) + 2;
   TCHAR *env = MemAllocString(len);
   _sntprintf(env, len, _T("%s=%s"), var, value);
#ifdef UNICODE
   char *mbenv = MBStringFromWideStringSysLocale(env);
   MemFree(env);
   return putenv(mbenv) == 0;
#else
   return putenv(env) == 0;
#endif

#endif //HAVE_SETENV
}

/**
 * GetEnvironmentVariable implementation for non-Windows environments
 */
DWORD LIBNETXMS_EXPORTABLE GetEnvironmentVariable(const TCHAR *var, TCHAR *buffer, DWORD size)
{
#ifdef UNICODE
   char mbvar[256];
   WideCharToMultiByteSysLocale(var, mbvar, 256);
   const char *v = getenv(mbvar);
   if (v != nullptr)
   {
      MultiByteToWideCharSysLocale(v, buffer, size);
      buffer[size - 1] = 0;
   }
   else
   {
      buffer[0] = 0;
   }
   return wcslen(buffer);
#else
   const char *v = getenv(var);
   if (v != nullptr)
      strlcpy(buffer, v, size);
   else
      buffer[0] = 0;
   return strlen(buffer);
#endif
}

#endif   /* not _WIN32 */

/**
 * Get environment variable as string object. Returns empty
 * string if variable is not found.
 */
String LIBNETXMS_EXPORTABLE GetEnvironmentVariableEx(const TCHAR *var)
{
#ifdef _WIN32
   TCHAR buffer[2048];
   DWORD size = GetEnvironmentVariable(var, buffer, 2048);
   if (size < 2048)
      return (size > 0) ? String(buffer) : String();
   // Buffer is not large enough
   TCHAR *mbuffer = MemAllocString(size);
   GetEnvironmentVariable(var, mbuffer, size);
   String value = String(mbuffer);
   MemFree(mbuffer);
   return value;
#else
#ifdef UNICODE
   char mbvar[256];
   WideCharToMultiByteSysLocale(var, mbvar, 256);
   const char *value = getenv(mbvar);
#else
   const char *value = getenv(var);
#endif
   if (value == nullptr)
      return String();
#ifdef UNICODE
   size_t len = strlen(value) + 1;
   Buffer<wchar_t, 1024> wvalue(len);
   MultiByteToWideCharSysLocale(value, wvalue, len);
   return String(wvalue);
#else
   return String(value);
#endif
#endif
}

/**
 * Get file owner information. Returns provided buffer on success and null on failure.
 */
TCHAR LIBNETXMS_EXPORTABLE *GetFileOwner(const TCHAR *file, TCHAR *buffer, size_t size)
{
   buffer[0] = 0;

#ifdef _WIN32
   HANDLE hFile = CreateFile(file, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
   if (hFile == INVALID_HANDLE_VALUE)
      return nullptr;

   // Get the owner SID of the file.
   PSID owner = nullptr;
   PSECURITY_DESCRIPTOR sd = nullptr;
   if (GetSecurityInfo(hFile, SE_FILE_OBJECT, OWNER_SECURITY_INFORMATION, &owner, NULL, NULL, NULL, &sd) != ERROR_SUCCESS)
   {
      CloseHandle(hFile);
      return nullptr;
   }
   CloseHandle(hFile);

   // Resolve account and domain name
   TCHAR acctName[256], domainName[256];
   DWORD acctNameSize = 256, domainNameSize = 256;
   SID_NAME_USE use = SidTypeUnknown;
   if (!LookupAccountSid(nullptr, owner, acctName, &acctNameSize, domainName, &domainNameSize, &use))
      return nullptr;

   _tcslcpy(buffer, domainName, size);
   _tcslcat(buffer, _T("\\"), size);
   _tcslcat(buffer, acctName, size);
#else
   NX_STAT_STRUCT st;
   if (CALL_STAT_FOLLOW_SYMLINK(file, &st) != 0)
      return nullptr;

#if HAVE_GETPWUID_R
   struct passwd *pw, pwbuf;
   char pwtxt[4096];
   getpwuid_r(st.st_uid, &pwbuf, pwtxt, 4096, &pw);
#else
   struct passwd *pw = getpwuid(st.st_uid);
#endif
   if (pw != nullptr)
   {
#ifdef UNICODE
      mb_to_wchar(pw->pw_name, -1, buffer, size);
#else
      strlcpy(buffer, pw->pw_name, size);
#endif
   }
   else
   {
      _sntprintf(buffer, size, _T("[%lu]"), (unsigned long)st.st_uid);
   }
#endif // _WIN32

   return buffer;
}

/**
 * CSS color names
 */
static struct
{
   const TCHAR *name;
   Color value;
} s_cssColorNames[] = {
   { _T("aliceblue"), Color(0xf0, 0xf8, 0xff) },
   { _T("antiquewhite"), Color(0xfa, 0xeb, 0xd7) },
   { _T("aqua"), Color(0x00, 0xff, 0xff) },
   { _T("aquamarine"), Color(0x7f, 0xff, 0xd4) },
   { _T("azure"), Color(0xf0, 0xff, 0xff) },
   { _T("beige"), Color(0xf5, 0xf5, 0xdc) },
   { _T("bisque"), Color(0xff, 0xe4, 0xc4) },
   { _T("black"), Color(0x00, 0x00, 0x00) },
   { _T("blanchedalmond"), Color(0xff, 0xeb, 0xcd) },
   { _T("blue"), Color(0x00, 0x00, 0xff) },
   { _T("blueviolet"), Color(0x8a, 0x2b, 0xe2) },
   { _T("brown"), Color(0xa5, 0x2a, 0x2a) },
   { _T("burlywood"), Color(0xde, 0xb8, 0x87) },
   { _T("cadetblue"), Color(0x5f, 0x9e, 0xa0) },
   { _T("chartreuse"), Color(0x7f, 0xff, 0x00) },
   { _T("chocolate"), Color(0xd2, 0x69, 0x1e) },
   { _T("coral"), Color(0xff, 0x7f, 0x50) },
   { _T("cornflowerblue"), Color(0x64, 0x95, 0xed) },
   { _T("cornsilk"), Color(0xff, 0xf8, 0xdc) },
   { _T("crimson"), Color(0xdc, 0x14, 0x3c) },
   { _T("cyan"), Color(0x00, 0xff, 0xff) },
   { _T("darkblue"), Color(0x00, 0x00, 0x8b) },
   { _T("darkcyan"), Color(0x00, 0x8b, 0x8b) },
   { _T("darkgoldenrod"), Color(0xb8, 0x86, 0x0b) },
   { _T("darkgray"), Color(0xa9, 0xa9, 0xa9) },
   { _T("darkgrey"), Color(0xa9, 0xa9, 0xa9) },
   { _T("darkgreen"), Color(0x00, 0x64, 0x00) },
   { _T("darkkhaki"), Color(0xbd, 0xb7, 0x6b) },
   { _T("darkmagenta"), Color(0x8b, 0x00, 0x8b) },
   { _T("darkolivegreen"), Color(0x55, 0x6b, 0x2f) },
   { _T("darkorange"), Color(0xff, 0x8c, 0x00) },
   { _T("darkorchid"), Color(0x99, 0x32, 0xcc) },
   { _T("darkred"), Color(0x8b, 0x00, 0x00) },
   { _T("darksalmon"), Color(0xe9, 0x96, 0x7a) },
   { _T("darkseagreen"), Color(0x8f, 0xbc, 0x8f) },
   { _T("darkslateblue"), Color(0x48, 0x3d, 0x8b) },
   { _T("darkslategray"), Color(0x2f, 0x4f, 0x4f) },
   { _T("darkslategrey"), Color(0x2f, 0x4f, 0x4f) },
   { _T("darkturquoise"), Color(0x00, 0xce, 0xd1) },
   { _T("darkviolet"), Color(0x94, 0x00, 0xd3) },
   { _T("deeppink"), Color(0xff, 0x14, 0x93) },
   { _T("deepskyblue"), Color(0x00, 0xbf, 0xff) },
   { _T("dimgray"), Color(0x69, 0x69, 0x69) },
   { _T("dimgrey"), Color(0x69, 0x69, 0x69) },
   { _T("dodgerblue"), Color(0x1e, 0x90, 0xff) },
   { _T("firebrick"), Color(0xb2, 0x22, 0x22) },
   { _T("floralwhite"), Color(0xff, 0xfa, 0xf0) },
   { _T("forestgreen"), Color(0x22, 0x8b, 0x22) },
   { _T("fuchsia"), Color(0xff, 0x00, 0xff) },
   { _T("gainsboro"), Color(0xdc, 0xdc, 0xdc) },
   { _T("ghostwhite"), Color(0xf8, 0xf8, 0xff) },
   { _T("gold"), Color(0xff, 0xd7, 0x00) },
   { _T("goldenrod"), Color(0xda, 0xa5, 0x20) },
   { _T("gray"), Color(0x80, 0x80, 0x80) },
   { _T("grey"), Color(0x80, 0x80, 0x80) },
   { _T("green"), Color(0x00, 0x80, 0x00) },
   { _T("greenyellow"), Color(0xad, 0xff, 0x2f) },
   { _T("honeydew"), Color(0xf0, 0xff, 0xf0) },
   { _T("hotpink"), Color(0xff, 0x69, 0xb4) },
   { _T("indianred"), Color(0xcd, 0x5c, 0x5c) },
   { _T("indigo"), Color(0x4b, 0x00, 0x82) },
   { _T("ivory"), Color(0xff, 0xff, 0xf0) },
   { _T("khaki"), Color(0xf0, 0xe6, 0x8c) },
   { _T("lavender"), Color(0xe6, 0xe6, 0xfa) },
   { _T("lavenderblush"), Color(0xff, 0xf0, 0xf5) },
   { _T("lawngreen"), Color(0x7c, 0xfc, 0x00) },
   { _T("lemonchiffon"), Color(0xff, 0xfa, 0xcd) },
   { _T("lightblue"), Color(0xad, 0xd8, 0xe6) },
   { _T("lightcoral"), Color(0xf0, 0x80, 0x80) },
   { _T("lightcyan"), Color(0xe0, 0xff, 0xff) },
   { _T("lightgoldenrodyellow"), Color(0xfa, 0xfa, 0xd2) },
   { _T("lightgray"), Color(0xd3, 0xd3, 0xd3) },
   { _T("lightgrey"), Color(0xd3, 0xd3, 0xd3) },
   { _T("lightgreen"), Color(0x90, 0xee, 0x90) },
   { _T("lightpink"), Color(0xff, 0xb6, 0xc1) },
   { _T("lightsalmon"), Color(0xff, 0xa0, 0x7a) },
   { _T("lightseagreen"), Color(0x20, 0xb2, 0xaa) },
   { _T("lightskyblue"), Color(0x87, 0xce, 0xfa) },
   { _T("lightslategray"), Color(0x77, 0x88, 0x99) },
   { _T("lightslategrey"), Color(0x77, 0x88, 0x99) },
   { _T("lightsteelblue"), Color(0xb0, 0xc4, 0xde) },
   { _T("lightyellow"), Color(0xff, 0xff, 0xe0) },
   { _T("lime"), Color(0x00, 0xff, 0x00) },
   { _T("limegreen"), Color(0x32, 0xcd, 0x32) },
   { _T("linen"), Color(0xfa, 0xf0, 0xe6) },
   { _T("magenta"), Color(0xff, 0x00, 0xff) },
   { _T("maroon"), Color(0x80, 0x00, 0x00) },
   { _T("mediumaquamarine"), Color(0x66, 0xcd, 0xaa) },
   { _T("mediumblue"), Color(0x00, 0x00, 0xcd) },
   { _T("mediumorchid"), Color(0xba, 0x55, 0xd3) },
   { _T("mediumpurple"), Color(0x93, 0x70, 0xdb) },
   { _T("mediumseagreen"), Color(0x3c, 0xb3, 0x71) },
   { _T("mediumslateblue"), Color(0x7b, 0x68, 0xee) },
   { _T("mediumspringgreen"), Color(0x00, 0xfa, 0x9a) },
   { _T("mediumturquoise"), Color(0x48, 0xd1, 0xcc) },
   { _T("mediumvioletred"), Color(0xc7, 0x15, 0x85) },
   { _T("midnightblue"), Color(0x19, 0x19, 0x70) },
   { _T("mintcream"), Color(0xf5, 0xff, 0xfa) },
   { _T("mistyrose"), Color(0xff, 0xe4, 0xe1) },
   { _T("moccasin"), Color(0xff, 0xe4, 0xb5) },
   { _T("navajowhite"), Color(0xff, 0xde, 0xad) },
   { _T("navy"), Color(0x00, 0x00, 0x80) },
   { _T("oldlace"), Color(0xfd, 0xf5, 0xe6) },
   { _T("olive"), Color(0x80, 0x80, 0x00) },
   { _T("olivedrab"), Color(0x6b, 0x8e, 0x23) },
   { _T("orange"), Color(0xff, 0xa5, 0x00) },
   { _T("orangered"), Color(0xff, 0x45, 0x00) },
   { _T("orchid"), Color(0xda, 0x70, 0xd6) },
   { _T("palegoldenrod"), Color(0xee, 0xe8, 0xaa) },
   { _T("palegreen"), Color(0x98, 0xfb, 0x98) },
   { _T("paleturquoise"), Color(0xaf, 0xee, 0xee) },
   { _T("palevioletred"), Color(0xdb, 0x70, 0x93) },
   { _T("papayawhip"), Color(0xff, 0xef, 0xd5) },
   { _T("peachpuff"), Color(0xff, 0xda, 0xb9) },
   { _T("peru"), Color(0xcd, 0x85, 0x3f) },
   { _T("pink"), Color(0xff, 0xc0, 0xcb) },
   { _T("plum"), Color(0xdd, 0xa0, 0xdd) },
   { _T("powderblue"), Color(0xb0, 0xe0, 0xe6) },
   { _T("purple"), Color(0x80, 0x00, 0x80) },
   { _T("rebeccapurple"), Color(0x66, 0x33, 0x99) },
   { _T("red"), Color(0xff, 0x00, 0x00) },
   { _T("rosybrown"), Color(0xbc, 0x8f, 0x8f) },
   { _T("royalblue"), Color(0x41, 0x69, 0xe1) },
   { _T("saddlebrown"), Color(0x8b, 0x45, 0x13) },
   { _T("salmon"), Color(0xfa, 0x80, 0x72) },
   { _T("sandybrown"), Color(0xf4, 0xa4, 0x60) },
   { _T("seagreen"), Color(0x2e, 0x8b, 0x57) },
   { _T("seashell"), Color(0xff, 0xf5, 0xee) },
   { _T("sienna"), Color(0xa0, 0x52, 0x2d) },
   { _T("silver"), Color(0xc0, 0xc0, 0xc0) },
   { _T("skyblue"), Color(0x87, 0xce, 0xeb) },
   { _T("slateblue"), Color(0x6a, 0x5a, 0xcd) },
   { _T("slategray"), Color(0x70, 0x80, 0x90) },
   { _T("slategrey"), Color(0x70, 0x80, 0x90) },
   { _T("snow"), Color(0xff, 0xfa, 0xfa) },
   { _T("springgreen"), Color(0x00, 0xff, 0x7f) },
   { _T("steelblue"), Color(0x46, 0x82, 0xb4) },
   { _T("tan"), Color(0xd2, 0xb4, 0x8c) },
   { _T("teal"), Color(0x00, 0x80, 0x80) },
   { _T("thistle"), Color(0xd8, 0xbf, 0xd8) },
   { _T("tomato"), Color(0xff, 0x63, 0x47) },
   { _T("turquoise"), Color(0x40, 0xe0, 0xd0) },
   { _T("violet"), Color(0xee, 0x82, 0xee) },
   { _T("wheat"), Color(0xf5, 0xde, 0xb3) },
   { _T("white"), Color(0xff, 0xff, 0xff) },
   { _T("whitesmoke"), Color(0xf5, 0xf5, 0xf5) },
   { _T("yellow"), Color(0xff, 0xff, 0x00) },
   { _T("yellowgreen"), Color(0x9a, 0xcd, 0x32) },
   { nullptr, Color() }
};

/**
 * Parse color definition in CSS compatible format
 */
Color Color::parseCSS(const TCHAR *css)
{
   // #NNNNNN
   if (*css == '#')
   {
      uint32_t v = _tcstoul(&css[1], nullptr, 16) & 0x00FFFFFF;
      return Color(v >> 16, (v >> 8) & 0xFF, v & 0xFF);
   }

   // 0xNNNNNN
   if (!_tcsncmp(css, _T("0x"), 2))
   {
      uint32_t v = _tcstoul(&css[2], nullptr, 16) & 0x00FFFFFF;
      return Color(v >> 16, (v >> 8) & 0xFF, v & 0xFF);
   }

   // rgb(NN, NN, NN)
   if (!_tcsnicmp(css, _T("rgb("), 4))
   {
      BYTE red = 0, green = 0, blue = 0;

      int count;
      TCHAR **parts = SplitString(&css[4], _T(','), &count);
      if (count == 3)
      {
         Trim(parts[0]);
         Trim(parts[1]);
         TCHAR *p = _tcschr(parts[2], _T(')'));
         if (p != nullptr)
            *p = 0;
         Trim(parts[2]);

         red = static_cast<BYTE>(_tcstoul(parts[0], nullptr, 0));
         green = static_cast<BYTE>(_tcstoul(parts[1], nullptr, 0));
         blue = static_cast<BYTE>(_tcstoul(parts[2], nullptr, 0));
      }

      for(int i = 0; i < count; i++)
         MemFree(parts[i]);
      MemFree(parts);

      return Color(red, green, blue);
   }

   for(int i = 0; s_cssColorNames[i].name != nullptr; i++)
      if (!_tcsicmp(css, s_cssColorNames[i].name))
         return s_cssColorNames[i].value;

   return Color();
}

/**
 * Convert volor object to CSS definition
 */
String Color::toCSS(bool alwaysUseHex) const
{
   if (!alwaysUseHex)
   {
      for(int i = 0; s_cssColorNames[i].name != nullptr; i++)
         if (equals(s_cssColorNames[i].value))
            return String(s_cssColorNames[i].name);
   }

   TCHAR buffer[16];
   _sntprintf(buffer, 16, _T("#%02x%02x%02x"), red, green, blue);
   return String(buffer);
}

/**
 * Encode string for URL
 */
char LIBNETXMS_EXPORTABLE *URLEncode(const char *src, char *dst, size_t size)
{
   const char *s = src;
   char *d = dst;
   size_t count = 0;
   while((*s != 0) && (count < size - 1))
   {
      char c = *s++;
      if (isalnum(c) || (c == '-') || (c == '_') || (c == '.') || (c == '~'))
      {
         *d++ = c;
         count++;
      }
      else
      {
         if (count >= size - 3)
            break;
         *d++ = '%';
         *d++ = bin2hex(static_cast<BYTE>(c) >> 4);
         *d++ = bin2hex(static_cast<BYTE>(c) & 15);
         count += 3;
      }
   }
   *d = 0;
   return dst;
}

/**
 * Seconds to uptime
 */
String LIBNETXMS_EXPORTABLE SecondsToUptime(uint64_t arg, bool withSeconds)
{
   uint32_t d, h, n;
   d = (UINT32)(arg / 86400);
   arg -= d * 86400;
   h = (UINT32)(arg / 3600);
   arg -= h * 3600;
   n = (UINT32)(arg / 60);
   arg -= n * 60;
   if (arg >= 30 && !withSeconds)
   {
       n++;
       if (n == 60)
       {
          n = 0;
          h++;
          if (h == 24)
          {
             h = 0;
             d++;
          }
       }
   }

   StringBuffer sb;
   if (withSeconds)
      sb.appendFormattedString(_T("%u days, %2u:%02u:%02u"), d, h, n, arg);
   else
      sb.appendFormattedString(_T("%u days, %2u:%02u"), d, h, n);
   return sb;
}

static double DECIMAL_MULTIPLIERS[] = { 1L, 1000L, 1000000L, 1000000000L, 1000000000000L, 1000000000000000L };
static double DECIMAL_MULTIPLIERS_SMALL[] = { 0.000000000000001, 0.000000000001, 0.000000001, 0.000001, 0.001, 1 };
static double BINARY_MULTIPLIERS[] = { 1L, 0x400L, 0x100000L, 0x40000000L, 0x10000000000L, 0x4000000000000L }; //TODO: s_decimalMultipliers
static const TCHAR *SUFFIX[] = { _T(""), _T(" k"), _T(" M"), _T(" G"), _T(" T"), _T(" P") };
static const TCHAR *BINARY_SUFFIX[] = { _T(""), _T(" Ki"), _T(" Mi"), _T(" Gi"), _T(" Ti"), _T(" Pi") };
static const TCHAR *SUFFIX_SMALL[] = { _T(" f"), _T(" p"), _T(" n"), _T(" μ"), _T(" m"), _T("") };

/**
 * Convert number to short form using decadic unit prefixes
 * If precision is < 0 then absolute value is used to determine number of digits after decimal separator and non-significant zeroes are removed
 */
String LIBNETXMS_EXPORTABLE FormatNumber(double n, bool useBinaryMultipliers, int multiplierPower, int precision, const TCHAR *unit)
{
   bool isSmallNumber = ((n > -0.01) && (n < 0.01) && n != 0 && multiplierPower <= 0) || multiplierPower < 0;
   const double *multipliers = isSmallNumber ? DECIMAL_MULTIPLIERS_SMALL : useBinaryMultipliers ? BINARY_MULTIPLIERS : DECIMAL_MULTIPLIERS;

   int i = 0;
   if (multiplierPower != 0)
   {
      if (isSmallNumber)
         multiplierPower = 5 + multiplierPower;
      i = MIN(multiplierPower, 5);
   }
   else
   {
      for(i = 5; i >= 0; i--)
      {
         if ((n >= multipliers[i]) || (n <= -multipliers[i]))
            break;
      }
   }

   TCHAR out[128];
   const TCHAR *suffix;
   if (i >= 0)
   {
      suffix = isSmallNumber ? SUFFIX_SMALL[i] : (useBinaryMultipliers ? BINARY_SUFFIX[i] : SUFFIX[i]);
      _sntprintf(out, 128, _T("%.*f"), abs(precision), n / multipliers[i]);
   }
   else
   {
      suffix = nullptr;
      _sntprintf(out, 128, _T("%.*f"), abs(precision), n);
   }

   if (precision < 0)
   {
      TCHAR *p = &out[_tcslen(out) - 1];
      while(*p == '0')
         p--;

      if (_istdigit(*p))
         *(p + 1) = 0;
      else
         *p = 0;
   }

   if (suffix != nullptr)
      _tcslcat(out, suffix, 128);

   if (unit != nullptr)
   {
      if ((suffix == nullptr) || (*suffix == 0))
         _tcslcat(out, _T(" "), 128);
      _tcslcat(out, unit, 128);
   }

   return String(out);
}

/**
 * List of units that should be exempt from multiplication
 */
static const TCHAR *s_unitsWithoutMultipliers[] = { _T("%"), _T("°C"), _T("°F"), _T("dbm") };

/**
 * Format DCI value based on Unit and value.
 */
String LIBNETXMS_EXPORTABLE FormatDCIValue(const TCHAR *unitName, const TCHAR *value)
{
   if (value == nullptr || *value == 0)
      return String();

   bool useUnits = true;
   bool useMultiplier = true;
   StringBuffer units = unitName;
   bool useBinaryPrefixes = units.containsIgnoreCase(_T(" (IEC)"));
   units.replace(_T(" (IEC)"), _T(""));
   units.replace(_T(" (Metric)"), _T(""));

   for (size_t i = 0; useMultiplier && !units.isEmpty() && i < sizeof(s_unitsWithoutMultipliers) / sizeof(TCHAR*); i++)
   {
      if (!_tcscmp(s_unitsWithoutMultipliers[i], units))
      {
         useMultiplier = false;
         break;
      }
   }

   if (useUnits && !_tcsicmp(unitName, _T("Uptime")))
   {
      TCHAR *eptr;
      uint64_t time = _tcstoull(value, &eptr, 10);
      return (*eptr != 0) ? String(value) : SecondsToUptime(time, true);
   }

   if (useUnits && !_tcsicmp(unitName, _T("Epoch time")))
   {
      TCHAR *eptr;
      time_t time = static_cast<time_t>(_tcstoll(value, &eptr, 10));
      return (*eptr != 0) ? String(value) : FormatTimestamp(time);
   }

   TCHAR *eptr;
   double inVal = _tcstod(value, &eptr);
   if (*eptr != 0)
      return String(value);

   StringBuffer result;
   TCHAR prefixSymbol[8] = _T("");
   if (useMultiplier)
   {
      result.append(FormatNumber(inVal, useBinaryPrefixes, 0, 2));
   }
   else
   {
      result.append(value);
   }

   if (useUnits && !units.isEmpty())
   {
      if (*prefixSymbol == 0)
         result.append(_T(" "));
      result.append(units);
   }

   return result;
}

/**
 * Calculate Levenshtein distance between two strings
 * @return Edit distance (number of single-character edits required to transform one string into another)
 */
size_t LIBNETXMS_EXPORTABLE CalculateLevenshteinDistance(const TCHAR *s1, size_t len1, const TCHAR *s2, size_t len2, bool ignoreCase)
{
   // If one string is empty, distance is the length of the other
   if (len1 == 0)
      return len2;
   if (len2 == 0)
      return len1;

   // Create distance matrix
   std::vector<std::vector<size_t>> matrix(len1 + 1, std::vector<size_t>(len2 + 1));

   // Initialize first row and column
   for (size_t i = 0; i <= len1; i++)
      matrix[i][0] = i;
   for (size_t j = 0; j <= len2; j++)
      matrix[0][j] = j;

   // Fill the matrix
   for (size_t i = 1; i <= len1; i++)
   {
      for (size_t j = 1; j <= len2; j++)
      {
         size_t cost = ((s1[i - 1] == s2[j - 1]) || (ignoreCase && (_totupper(s1[i - 1]) == _totupper(s2[j - 1])))) ? 0 : 1;
         matrix[i][j] = std::min({
            matrix[i - 1][j] + 1,      // deletion
            matrix[i][j - 1] + 1,      // insertion
            matrix[i - 1][j - 1] + cost // substitution
         });
      }
   }

   return matrix[len1][len2];
}

/**
 * Calculate string similarity (1.0 - exact match, 0.0 - completely different)
 * @param s1 first string
 * @param s2 second string
 * @param ignoreCase true to ignore case during comparison
 * @return similarity value
 */
double LIBNETXMS_EXPORTABLE CalculateStringSimilarity(const TCHAR *s1, const TCHAR *s2, bool ignoreCase)
{
   size_t len1 = _tcslen(s1);
   size_t len2 = _tcslen(s2);
   if (len1 == 0 && len2 == 0)
      return 1.0;

   size_t editDistance = CalculateLevenshteinDistance(s1, len1, s2, len2, ignoreCase);
   return 1.0 - (static_cast<double>(editDistance) / std::max(len1, len2));
}

/**
 * Fuzzy string comparison
 * @param s1 first string
 * @param s2 second string
 * @param threshold matching threshold (0.0 - exact match, 1.0 - any strings match)
 * @return true if strings match according to specified threshold
 */
bool LIBNETXMS_EXPORTABLE FuzzyMatchStrings(const TCHAR *s1, const TCHAR *s2, double threshold)
{
   if (threshold >= 1.0)
      return _tcscmp(s1, s2) == 0;

   if (threshold <= 0.0)
      return true;

   return CalculateStringSimilarity(s1, s2, false) >= threshold;
}

/**
 * Fuzzy string comparison ignoring case
 * @param s1 first string
 * @param s2 second string
 * @param threshold matching threshold (0.0 - exact match, 1.0 - any strings match)
 * @return true if strings match according to specified threshold
 */
bool LIBNETXMS_EXPORTABLE FuzzyMatchStringsIgnoreCase(const TCHAR *s1, const TCHAR *s2, double threshold)
{
   if (threshold >= 1.0)
      return _tcsicmp(s1, s2) == 0;

   if (threshold <= 0.0)
      return true;

   return CalculateStringSimilarity(s1, s2, true) >= threshold;
}
