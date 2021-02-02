/*
** NetXMS - Network Management System
** Copyright (C) 2003-2020 Victor Kirhenshtein
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
#include <Softpub.h>
#endif

#if !defined(_WIN32) && !defined(UNDER_CE)
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
static CONDITION s_shutdownCondition = INVALID_CONDITION_HANDLE;

/**
 * Process shutdown flag
 */
static bool s_shutdownFlag = false;

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
   SubProcessExecutor::shutdown();
   MsgWaitQueue::shutdown();
   ConditionDestroy(s_shutdownCondition);
   LibCURLCleanup();
#ifdef _WIN32
   SetConsoleOutputCP(s_originalConsoleCodePage);
#endif
}

#ifdef _WIN32

/**
 * Invalid parameter handler
 */
static void InvalidParameterHandler(const wchar_t *expression, const wchar_t *function, const wchar_t *file, unsigned int line, uintptr_t reserved)
{
#ifdef _DEBUG
   nxlog_write(NXLOG_ERROR, _T("CRT error: function %s in %s:%u (%s)"), function, file, line, expression);
   if (IsDebuggerPresent())
      DebugBreak();
   else
      FatalExit(99);
#else
   nxlog_write(NXLOG_ERROR, _T("CRT error detected"));
   if (IsDebuggerPresent())
      DebugBreak();
#endif
}

#endif

/**
 * Common initialization for any NetXMS process
 *
 * @param commandLineTool set to true for command line tool initialization
 */
void LIBNETXMS_EXPORTABLE InitNetXMSProcess(bool commandLineTool)
{
   InitThreadLibrary();

   s_shutdownCondition = ConditionCreate(true);

   // Set locale to C. It shouldn't be needed, according to
   // documentation, but I've seen the cases when agent formats
   // floating point numbers by sprintf inserting comma in place
   // of a dot, as set by system's regional settings.
#if HAVE_SETLOCALE
   setlocale(LC_NUMERIC, "C");
#if defined(UNICODE) && !defined(_WIN32)
   const char *locale = getenv("LC_CTYPE");
   if (locale == NULL)
      locale = getenv("LC_ALL");
   if (locale == NULL)
      locale = getenv("LANG");
   if (locale != NULL)
      setlocale(LC_CTYPE, locale);
#endif
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

   srand(static_cast<unsigned int>(time(NULL)));

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
   ConditionSet(s_shutdownCondition);
}

/**
 * Sleep for specified number of seconds or until system shutdown arrives
 * Function will return TRUE if shutdown event occurs
 *
 * @param seconds seconds to sleep
 * @return true if server is shutting down
 */
bool LIBNETXMS_EXPORTABLE SleepAndCheckForShutdown(UINT32 seconds)
{
   return ConditionWait(s_shutdownCondition, (seconds != INFINITE) ? seconds * 1000 : INFINITE);
}

/**
 * Sleep for specified number of milliseconds or until system shutdown arrives
 * Function will return TRUE if shutdown event occurs
 *
 * @param milliseconds milliseconds to sleep
 * @return true if server is shutting down
 */
bool LIBNETXMS_EXPORTABLE SleepAndCheckForShutdownEx(UINT32 milliseconds)
{
   return ConditionWait(s_shutdownCondition, milliseconds);
}

/**
 * Get condition object to wait for shutdown
 */
CONDITION LIBNETXMS_EXPORTABLE GetShutdownConditionObject()
{
   return s_shutdownCondition;
}

/**
 * Check if process shutdown is in progress
 */
bool LIBNETXMS_EXPORTABLE IsShutdownInProgress()
{
   return s_shutdownFlag;
}

/**
 * Calculate number of bits in IPv4 netmask (in host byte order)
 */
int LIBNETXMS_EXPORTABLE BitsInMask(UINT32 mask)
{
   int bits;
   for(bits = 0; mask != 0; bits++, mask <<= 1);
   return bits;
}

/**
 * Calculate number of bits in IP netmask (in network byte order)
 */
int LIBNETXMS_EXPORTABLE BitsInMask(const BYTE *mask, size_t size)
{
   int bits = 0;
   for(size_t i = 0; i < size; i++, bits += 8)
   {
      BYTE byte = mask[i];
      if (byte != 0xFF)
      {
         for(; byte != 0; bits++, byte <<= 1);
         break;
      }
   }
   return bits;
}

/**
 * Convert IP address from binary form (host bytes order) to string
 */
TCHAR LIBNETXMS_EXPORTABLE *IpToStr(UINT32 dwAddr, TCHAR *szBuffer)
{
   static TCHAR szInternalBuffer[16];
   TCHAR *szBufPtr;

   szBufPtr = (szBuffer == NULL) ? szInternalBuffer : szBuffer;
   _sntprintf(szBufPtr, 16, _T("%d.%d.%d.%d"), (int)(dwAddr >> 24), (int)((dwAddr >> 16) & 255),
              (int)((dwAddr >> 8) & 255), (int)(dwAddr & 255));
   return szBufPtr;
}

#ifdef UNICODE

char LIBNETXMS_EXPORTABLE *IpToStrA(UINT32 dwAddr, char *szBuffer)
{
   static char szInternalBuffer[16];
   char *szBufPtr;

   szBufPtr = (szBuffer == NULL) ? szInternalBuffer : szBuffer;
   snprintf(szBufPtr, 16, "%d.%d.%d.%d", (int)(dwAddr >> 24), (int)((dwAddr >> 16) & 255),
            (int)((dwAddr >> 8) & 255), (int)(dwAddr & 255));
   return szBufPtr;
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
   static TCHAR internalBuffer[64];
   TCHAR *bufPtr = (buffer == NULL) ? internalBuffer : buffer;

	if (!memcmp(addr, "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 16))
	{
		_tcscpy(bufPtr, _T("::"));
		return bufPtr;
	}

	TCHAR *out = bufPtr;
	WORD *curr = (WORD *)addr;
	bool hasNulls = false;
	for(int i = 0; i < 8; i++)
	{
		WORD value = ntohs(*curr);
		if ((value != 0) || hasNulls)
		{
			if (out != bufPtr)
				*out++ = _T(':');
			_sntprintf(out, 5, _T("%x"), value);
			out = bufPtr + _tcslen(bufPtr);
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
   return bufPtr;
}

#ifdef UNICODE

/**
 * Convert IPv6 address from binary form to string
 */
char LIBNETXMS_EXPORTABLE *Ip6ToStrA(const BYTE *addr, char *buffer)
{
   static char internalBuffer[64];
   char *bufPtr = (buffer == NULL) ? internalBuffer : buffer;

   if (!memcmp(addr, "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 16))
   {
      strcpy(bufPtr, "::");
      return bufPtr;
   }

   char *out = bufPtr;
   WORD *curr = (WORD *)addr;
   bool hasNulls = false;
   for(int i = 0; i < 8; i++)
   {
      WORD value = ntohs(*curr);
      if ((value != 0) || hasNulls)
      {
         if (out != bufPtr)
            *out++ = ':';
         snprintf(out, 5, "%x", value);
         out = bufPtr + strlen(bufPtr);
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
   return bufPtr;
}

#endif

/**
 * Duplicate memory block
 */
LIBNETXMS_EXPORTABLE void *MemCopyBlock(const void *data, size_t size)
{
   void *newData = MemAlloc(size);
   memcpy(newData, data, size);
   return newData;
}

/**
 * Swap two memory blocks
 */
void LIBNETXMS_EXPORTABLE nx_memswap(void *block1, void *block2, size_t size)
{
   void *temp;

   temp = MemAlloc(size);
   memcpy(temp, block1, size);
   memcpy(block1, block2, size);
   memcpy(block2, temp, size);
   MemFree(temp);
}

/**
 * Character comparator - UNICODE/case sensitive
 */
inline bool _ccw(WCHAR c1, WCHAR c2)
{
   return c1 == c2;
}

/**
 * Character comparator - UNICODE/case insensitive
 */
inline bool _ccwi(WCHAR c1, WCHAR c2)
{
   return towupper(c1) == towupper(c2);
}

/**
 * Character comparator - non-UNICODE/case sensitive
 */
inline bool _cca(char c1, char c2)
{
   return c1 == c2;
}

/**
 * Character comparator - non-UNICODE/case insensitive
 */
inline bool _ccai(char c1, char c2)
{
   return toupper(c1) == toupper(c2);
}

/**
 * Compare pattern with possible ? characters with given text
 */
template<typename T, bool (*Compare)(T, T)> bool CompareTextBlocks(const T *pattern, const T *text, size_t size)
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
template<typename T, bool (*Compare)(T, T), size_t (*Length)(const T*)> bool MatchStringEngine(const T *pattern, const T *string)
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
            EPtr = NULL;
            finishScan = false;
            do
            {
               while(1)
               {
                  while((*SPtr != 0) && !Compare(*SPtr, *BPtr))
                     SPtr++;
                  if (Length(SPtr) < bsize)
                  {
                     if (EPtr == NULL)
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
 * Strip whitespaces and tabs off the string
 */
void LIBNETXMS_EXPORTABLE StrStripA(char *str)
{
   int i;

   for(i = 0; (str[i]!=0) && ((str[i] == ' ') || (str[i] == '\t')); i++);
   if (i > 0)
      memmove(str, &str[i], strlen(&str[i]) + 1);
   for(i = (int)strlen(str) - 1; (i >= 0) && ((str[i] == ' ') || (str[i] == '\t')); i--);
   str[i + 1] = 0;
}

/**
 * Strip whitespaces and tabs off the string
 */
void LIBNETXMS_EXPORTABLE StrStripW(WCHAR *str)
{
   int i;

   for(i = 0; (str[i]!=0) && ((str[i] == L' ') || (str[i] == L'\t')); i++);
   if (i > 0)
      memmove(str, &str[i], (wcslen(&str[i]) + 1) * sizeof(WCHAR));
   for(i = (int)wcslen(str) - 1; (i >= 0) && ((str[i] == L' ') || (str[i] == L'\t')); i--);
   str[i + 1] = 0;
}

/**
 * Strip whitespaces and tabs off the string.
 *
 * @param str string to trim
 * @return str for convenience
 */
TCHAR LIBNETXMS_EXPORTABLE *Trim(TCHAR *str)
{
   if (str == NULL)
      return NULL;

   int i;
   for(i = 0; (str[i] != 0) && _istspace(str[i]); i++);
   if (i > 0)
      memmove(str, &str[i], (_tcslen(&str[i]) + 1) * sizeof(TCHAR));
   for(i = (int)_tcslen(str) - 1; (i >= 0) && _istspace(str[i]); i--);
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

	t = time(NULL);
#if HAVE_LOCALTIME_R
	ltm = localtime_r(&t, &tmBuff);
#else
	ltm = localtime(&t);
#endif
   if (_tcsftime(temp, MAX_PATH, name, ltm) <= 0)
   {
      if (name != buffer)
         _tcslcpy(buffer, name, bufSize);
      return NULL;
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

			FILE *p = _tpopen(command, _T("r"));
			if (p != NULL)
			{
				char result[1024];
				int rc = (int)fread(result, 1, 1023, p);
				_pclose(p);

				if (rc > 0)
				{
					result[rc] = 0;
               char *lf = strchr(result, '\r');
               if (lf != NULL)
                  *lf = 0;

					lf = strchr(result, '\n');
					if (lf != NULL)
						*lf = 0;

					len = (int)std::min(strlen(result), bufSize - outpos - 1);
#ifdef UNICODE
					len = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, result, len, &buffer[outpos], len + 1);
#else
					memcpy(&buffer[outpos], result, len);
#endif
					outpos += len;
				}
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
 * Create folder
 */
bool LIBNETXMS_EXPORTABLE CreateFolder(const TCHAR *directory)
{
   NX_STAT_STRUCT st;
   TCHAR *previous = MemCopyString(directory);
   TCHAR *ptr = _tcsrchr(previous, FS_PATH_SEPARATOR_CHAR);
   bool result = false;
   if (ptr != nullptr)
   {
      *ptr = 0;
      if (CALL_STAT(previous, &st) != 0)
      {
         result = CreateFolder(previous);
         if (result)
         {
            result = (CALL_STAT(previous, &st) == 0);
         }
      }
      else
      {
         if (S_ISDIR(st.st_mode))
         {
            result = true;
         }
      }
   }
   else
   {
      result = true;
      st.st_mode = 0700;
   }
   MemFree(previous);

   if (result)
   {
#ifdef _WIN32
      result = CreateDirectory(directory, nullptr);
#else
      result = (_tmkdir(directory, st.st_mode) == 0);
#endif /* _WIN32 */
   }

   return result;
}

/**
 * Set last modification time to file
 */
bool SetLastModificationTime(TCHAR *fileName, time_t lastModDate)
{
   bool success = false;
#ifdef _WIN32
   struct _utimbuf ut;
#else
   struct utimbuf ut;
#endif // _WIN32
   ut.actime = lastModDate;
   ut.modtime = lastModDate;
   success = _tutime(fileName, &ut) == 0;
   return success;
}

/**
 * Get current time in milliseconds
 * Based on timeval.h by Wu Yongwei
 */
int64_t LIBNETXMS_EXPORTABLE GetCurrentTimeMs()
{
#ifdef _WIN32
   FILETIME ft;
   GetSystemTimeAsFileTime(&ft);

   LARGE_INTEGER li;
   li.LowPart  = ft.dwLowDateTime;
   li.HighPart = ft.dwHighDateTime;
   int64_t t = li.QuadPart;       // In 100-nanosecond intervals
   t -= EPOCHFILETIME;    // Offset to the Epoch time
   t /= 10000;            // Convert to milliseconds
#else
   struct timeval tv;
   gettimeofday(&tv, NULL);
   int64_t t = (int64_t)tv.tv_sec * 1000 + (int64_t)(tv.tv_usec / 1000);
#endif

   return t;
}

/**
 * Format timestamp as dd.mm.yy HH:MM:SS.
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
 * Format timestamp as dd.mm.yy HH:MM:SS.
 */
String LIBNETXMS_EXPORTABLE FormatTimestamp(time_t t)
{
   TCHAR buffer[64];
   return String(FormatTimestamp(t, buffer));
}

/**
 * Extract word from line (UNICODE version). Extracted word will be placed in buffer.
 * Returns pointer to the next word or to the null character if end
 * of line reached.
 */
const WCHAR LIBNETXMS_EXPORTABLE *ExtractWordW(const WCHAR *line, WCHAR *buffer)
{
   const WCHAR *ptr;
	WCHAR *bptr;

   for(ptr = line; (*ptr == L' ') || (*ptr == L'\t'); ptr++);  // Skip initial spaces
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
const char LIBNETXMS_EXPORTABLE *ExtractWordA(const char *line, char *buffer)
{
   const char *ptr;
	char *bptr;

   for(ptr = line; (*ptr == ' ') || (*ptr == '\t'); ptr++);  // Skip initial spaces
   // Copy word to buffer
   for(bptr = buffer; (*ptr != ' ') && (*ptr != '\t') && (*ptr != 0); ptr++, bptr++)
      *bptr = *ptr;
   *bptr = 0;
   return ptr;
}

#if defined(_WIN32)

/**
 * Get system error string by call to FormatMessage
 * (Windows only)
 */
TCHAR LIBNETXMS_EXPORTABLE *GetSystemErrorText(UINT32 error, TCHAR *buffer, size_t size)
{
   TCHAR *msgBuf;

   if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                     FORMAT_MESSAGE_FROM_SYSTEM |
                     FORMAT_MESSAGE_IGNORE_INSERTS,
                     NULL, error,
                     MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                     (LPTSTR)&msgBuf, 0, NULL) > 0)
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

#endif

/**
 * Get last socket error as text
 */
TCHAR LIBNETXMS_EXPORTABLE *GetLastSocketErrorText(TCHAR *buffer, size_t size)
{
#ifdef _WIN32
   DWORD error = WSAGetLastError();
   _sntprintf(buffer, size, _T("%u "), error);
   size_t len = _tcslen(buffer);
   GetSystemErrorText(error, &buffer[len], size - len);
#else
   _sntprintf(buffer, size, _T("%u "), errno);
   size_t len = _tcslen(buffer);
#if HAVE_STRERROR_R
   _tcserror_r(errno, &buffer[len], size - len);
#else
   _tcslcpy(&buffer[len], _tcserror(errno), size - len);
#endif
#endif
   return buffer;
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
 * Convert byte array to text representation (wide character version)
 */
WCHAR LIBNETXMS_EXPORTABLE *BinToStrW(const void *data, size_t size, WCHAR *str)
{
   return BinToStrExW(data, size, str, 0, 0);
}

/**
 * Convert byte array to text representation (wide character version)
 */
WCHAR LIBNETXMS_EXPORTABLE *BinToStrExW(const void *data, size_t size, WCHAR *str, WCHAR separator, size_t padding)
{
   const BYTE *in = (const BYTE *)data;
   WCHAR *out = str;
   for(size_t i = 0; i < size; i++, in++)
   {
      *out++ = bin2hex(*in >> 4);
      *out++ = bin2hex(*in & 15);
      if (separator != 0)
         *out++ = separator;
   }
   for(size_t i = 0; i < padding; i++)
   {
      *out++ = L' ';
      *out++ = L' ';
      if (separator != 0)
         *out++ = separator;
   }
   if (separator != 0)
      out--;
   *out = 0;
   return str;
}

/**
 * Convert byte array to text representation (multibyte character version)
 */
char LIBNETXMS_EXPORTABLE *BinToStrA(const void *data, size_t size, char *str)
{
   return BinToStrExA(data, size, str, 0, 0);
}

/**
 * Convert byte array to text representation (multibyte character version)
 */
char LIBNETXMS_EXPORTABLE *BinToStrExA(const void *data, size_t size, char *str, char separator, size_t padding)
{
   const BYTE *in = (const BYTE *)data;
   char *out = str;
   for(size_t i = 0; i < size; i++, in++)
   {
      *out++ = bin2hex(*in >> 4);
      *out++ = bin2hex(*in & 15);
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
QWORD LIBNETXMS_EXPORTABLE FileSizeW(const WCHAR *pszFileName)
{
#ifdef _WIN32
   HANDLE hFind;
   WIN32_FIND_DATAW fd;
#else
   struct stat fileInfo;
#endif

#ifdef _WIN32
   hFind = FindFirstFileW(pszFileName, &fd);
   if (hFind == INVALID_HANDLE_VALUE)
      return 0;
   FindClose(hFind);

   return (unsigned __int64)fd.nFileSizeLow + ((unsigned __int64)fd.nFileSizeHigh << 32);
#else
   if (wstat(pszFileName, &fileInfo) == -1)
      return 0;

   return (QWORD)fileInfo.st_size;
#endif
}

/**
 * Get size of file in bytes
 */
QWORD LIBNETXMS_EXPORTABLE FileSizeA(const char *pszFileName)
{
#ifdef _WIN32
   HANDLE hFind;
   WIN32_FIND_DATAA fd;
#else
   struct stat fileInfo;
#endif

#ifdef _WIN32
   hFind = FindFirstFileA(pszFileName, &fd);
   if (hFind == INVALID_HANDLE_VALUE)
      return 0;
   FindClose(hFind);

   return (unsigned __int64)fd.nFileSizeLow + ((unsigned __int64)fd.nFileSizeHigh << 32);
#else
   if (stat(pszFileName, &fileInfo) == -1)
      return 0;

   return (QWORD)fileInfo.st_size;
#endif
}

/**
 * Get pointer to clean file name (without path specification)
 */
const TCHAR LIBNETXMS_EXPORTABLE *GetCleanFileName(const TCHAR *pszFileName)
{
   const TCHAR *ptr;

   ptr = pszFileName + _tcslen(pszFileName);
   while((ptr >= pszFileName) && (*ptr != _T('/')) && (*ptr != _T('\\')) && (*ptr != _T(':')))
      ptr--;
   return (ptr + 1);
}

/**
 * Translate DCI data type from text form to code
 */
int LIBNETXMS_EXPORTABLE NxDCIDataTypeFromText(const TCHAR *pszText)
{
   static const TCHAR *m_pszValidTypes[] = { _T("INT"), _T("UINT"), _T("INT64"),
                                             _T("UINT64"), _T("STRING"),
                                             _T("FLOAT"), NULL };
   int i;

   for(i = 0; m_pszValidTypes[i] != NULL; i++)
      if (!_tcsicmp(pszText, m_pszValidTypes[i]))
         return i;
   return -1;     // Invalid data type
}

/**
 * Extended send() - send all data even if single call to send()
 * cannot handle them all
 */
ssize_t LIBNETXMS_EXPORTABLE SendEx(SOCKET hSocket, const void *data, size_t len, int flags, MUTEX mutex)
{
   ssize_t nLeft = len;
	int nRet;

	if (mutex != INVALID_MUTEX_HANDLE)
		MutexLock(mutex);

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

	if (mutex != INVALID_MUTEX_HANDLE)
		MutexUnlock(mutex);

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
ssize_t LIBNETXMS_EXPORTABLE RecvEx(SOCKET hSocket, void *data, size_t len, int flags, UINT32 timeout, SOCKET controlSocket)
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
bool RecvAll(SOCKET s, void *buffer, size_t size, UINT32 timeout)
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
int LIBNETXMS_EXPORTABLE ConnectEx(SOCKET s, struct sockaddr *addr, int len, UINT32 timeout, bool *isTimeout)
{
	SetSocketNonBlocking(s);

	if (isTimeout != NULL)
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
				INT64 startTime = GetCurrentTimeMs();
				rc = poll(&fds, 1, timeout);
				if ((rc != -1) || (errno != EINTR))
					break;
				UINT32 elapsed = (UINT32)(GetCurrentTimeMs() - startTime);
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
			   if (isTimeout != NULL)
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
				INT64 startTime = GetCurrentTimeMs();
				rc = select(SELECT_NFDS(s + 1), NULL, &wrfs, &exfs, &tv);
				if ((rc != -1) || (errno != EINTR))
					break;
				UINT32 elapsed = (UINT32)(GetCurrentTimeMs() - startTime);
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
            if (isTimeout != NULL)
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
   MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, buf, -1, pszBuffer, nSize);
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
	StrStrip(versionString);
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
   PCREW *preg = _pcre_compile_w(reinterpret_cast<const PCRE_WCHAR*>(expr), matchCase ? PCRE_COMMON_FLAGS_W : PCRE_COMMON_FLAGS_W | PCRE_CASELESS, &errptr, &erroffset, NULL);
   if (preg != NULL)
   {
      int ovector[60];
      if (_pcre_exec_w(preg, NULL, reinterpret_cast<const PCRE_WCHAR*>(str), static_cast<int>(wcslen(str)), 0, 0, ovector, 60) >= 0) // MATCH
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
   pcre *preg = pcre_compile(expr, matchCase ? PCRE_COMMON_FLAGS_A : PCRE_COMMON_FLAGS_A | PCRE_CASELESS, &errptr, &erroffset, NULL);
   if (preg != NULL)
   {
      int ovector[60];
      if (pcre_exec(preg, NULL, str, static_cast<int>(strlen(str)), 0, 0, ovector, 60) >= 0) // MATCH
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
bool LIBNETXMS_EXPORTABLE ExtractNamedOptionValueW(const WCHAR *optString, const WCHAR *option, WCHAR *buffer, int bufSize)
{
	int state, pos;
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
					StrStripW(buffer);
					return true;
				}
				state = 0;
				start = curr + 1;
				break;
			case L'=':
				if (state == 0)
				{
					wcsncpy(temp, start, curr - start);
					temp[curr - start] = 0;
					StrStripW(temp);
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
		StrStripW(buffer);
		return true;
	}

	return false;
}

/**
 * Extract option value from string of form option=value;option=value;... (multibyte version)
 */
bool LIBNETXMS_EXPORTABLE ExtractNamedOptionValueA(const char *optString, const char *option, char *buffer, int bufSize)
{
	int state, pos;
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
					StrStripA(buffer);
					return true;
				}
				state = 0;
				start = curr + 1;
				break;
			case '=':
				if (state == 0)
				{
					strncpy(temp, start, curr - start);
					temp[curr - start] = 0;
					StrStripA(temp);
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
		StrStripA(buffer);
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
TCHAR LIBNETXMS_EXPORTABLE **SplitString(const TCHAR *source, TCHAR sep, int *numStrings)
{
	TCHAR **strings;

	*numStrings = NumChars(source, sep) + 1;
	strings = (TCHAR **)MemAlloc(sizeof(TCHAR *) * (*numStrings));
	for(int n = 0, i = 0; n < *numStrings; n++, i++)
	{
		int start = i;
		while((source[i] != sep) && (source[i] != 0))
			i++;
		int len = i - start;
		strings[n] = (TCHAR *)MemAlloc(sizeof(TCHAR) * (len + 1));
		memcpy(strings[n], &source[start], len * sizeof(TCHAR));
		strings[n][len] = 0;
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
	if (checkSeconds && ptr != NULL)
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
            nPrev = _tcstol(curr, NULL, 10);
            break;
         case 'L':  // special case for last day of week in a month (like 5L - last Friday)
            if (bRange || (localTime == NULL))
               return false;  // Range with L is not supported; nL form supported only for day of week
            *ptr = 0;
            nCurr = _tcstol(curr, NULL, 10);
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
            nCurr = _tcstol(curr, NULL, 10);
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
inline bool DecryptPasswordFailW(const WCHAR *encryptedPasswd, WCHAR *decryptedPasswd, size_t bufferLenght)
{
   if (decryptedPasswd != encryptedPasswd)
      wcsncpy(decryptedPasswd, encryptedPasswd, bufferLenght);
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

	MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, (char *)decrypted, -1, decryptedPasswd, (int)bufferLenght);
	decryptedPasswd[bufferLenght - 1] = 0;
	MemFree(mbencrypted);
	MemFree(mblogin);

	return true;
}

/**
 * Failure handler for DecryptPasswordA
 */
inline bool DecryptPasswordFailA(const char *encryptedPasswd, char *decryptedPasswd, size_t bufferLenght)
{
   if (decryptedPasswd != encryptedPasswd)
      strncpy(decryptedPasswd, encryptedPasswd, bufferLenght);
   return false;
}

/**
 * Decrypt password encrypted with nxencpassw.
 * In case when it was not possible to decrypt password as the decrypted password will be set the original one.
 * The buffer length for encryptedPasswd and decryptedPasswd should be the same.
 */
bool LIBNETXMS_EXPORTABLE DecryptPasswordA(const char *login, const char *encryptedPasswd, char *decryptedPasswd, size_t bufferLenght)
{
   //check that lenght is correct
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
   return true;
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
               *next++;
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
               *next++;
               pos++;
            }
            curr = next;
         }
      }
      else
      {
         if (_write(hFile, data, static_cast<unsigned int>(size)) != size)
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
INT64 LIBNETXMS_EXPORTABLE GetProcessRSS()
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

#if defined(UNICODE) && !defined(_WIN32) && HAVE_FPUTWS
#define WRT_UNICODE  1
#define WRT_TCHAR    WCHAR
#define _wrt_tcschr  wcschr
#define _wrt_tcslen  wcslen
#else
#define WRT_TCHAR    char
#define _wrt_tcschr  strchr
#define _wrt_tcslen  strlen
#endif

#if WRT_UNICODE
static void fputwsl(const WCHAR *text, const WCHAR *stop, FILE *out)
{
   const WCHAR *p = text;
   while(p < stop)
   {
      fputwc(*p, out);
      p++;
   }
}
#endif

/**
 * Write text with escape sequences to file
 */
static void WriteRedirectedTerminalOutput(const WRT_TCHAR *text)
{
   const WRT_TCHAR *curr = text;
   while(*curr != 0)
   {
      const WRT_TCHAR *esc = _wrt_tcschr(curr, 27);	// Find ESC
      if (esc != NULL)
      {
         esc++;
         if (*esc == '[')
         {
            // write everything up to ESC char
#if WRT_UNICODE
            fputwsl(curr, esc - 1, stdout);
#else
            fwrite(curr, sizeof(WRT_TCHAR), esc - curr - 1, stdout);
#endif
            esc++;
            while((*esc != 0) && (*esc != _T('m')))
               esc++;
            if (*esc == 0)
               break;
            esc++;
         }
         else
         {
#if WRT_UNICODE
            fputwsl(curr, esc, stdout);
#else
            fwrite(curr, sizeof(WRT_TCHAR), esc - curr, stdout);
#endif
         }
         curr = esc;
      }
      else
      {
#if WRT_UNICODE
         fputws(curr, stdout);
#else
         fputs(curr, stdout);
#endif
         break;
      }
   }
}

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
      char *mbText = MBStringFromWideString(text);
      WriteRedirectedTerminalOutput(mbText);
      MemFree(mbText);
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
				WriteConsole(out, curr, (UINT32)(esc - curr), &chars, NULL);
			}
			curr = esc;
		}
		else
		{
			DWORD chars;
			WriteConsole(out, curr, (UINT32)_tcslen(curr), &chars, NULL);
			break;
		}
	}
#else
#ifdef UNICODE
#if HAVE_FPUTWS
   if (isatty(fileno(stdout)))
	   fputws(text, stdout);
   else
      WriteRedirectedTerminalOutput(text);
#else
	char *mbtext = MBStringFromWideStringSysLocale(text);
   if (isatty(fileno(stdout)))
      fputs(mbtext, stdout);
   else
      WriteRedirectedTerminalOutput(mbtext);
   MemFree(mbtext);
#endif
#else
   if (isatty(fileno(stdout)))
      fputs(text, stdout);
   else
      WriteRedirectedTerminalOutput(text);
#endif
#endif
}

/**
 * Write to terminal with support for ANSI color codes
 */
void LIBNETXMS_EXPORTABLE WriteToTerminalEx(const TCHAR *format, ...)
{
	TCHAR buffer[8192];
	va_list args;

	va_start(args, format);
	_vsntprintf(buffer, 8192, format, args);
	va_end(args);
	WriteToTerminal(buffer);
}

/**
 * mkstemp() implementation for Windows
 */
#ifdef _WIN32

int LIBNETXMS_EXPORTABLE mkstemp(char *tmpl)
{
	char *name = _mktemp(tmpl);
	if (name == NULL)
		return -1;
	return _open(name, O_RDWR | O_BINARY | O_CREAT | O_EXCL| _O_SHORT_LIVED, _S_IREAD | _S_IWRITE);
}

int LIBNETXMS_EXPORTABLE wmkstemp(WCHAR *tmpl)
{
	WCHAR *name = _wmktemp(tmpl);
	if (name == NULL)
		return -1;
	return _wopen(name, O_RDWR | O_BINARY | O_CREAT | O_EXCL| _O_SHORT_LIVED, _S_IREAD | _S_IWRITE);
}

#endif

/**
 * Destructor for RefCountObject
 */
RefCountObject::~RefCountObject()
{
}

/**
 * Safe _fgetts implementation which will work
 * with handles opened by popen
 */
TCHAR LIBNETXMS_EXPORTABLE *safe_fgetts(TCHAR *buffer, int len, FILE *f)
{
#ifdef UNICODE
#if SAFE_FGETWS_WITH_POPEN && !defined(_WIN32)
	return fgetws(buffer, len, f);
#else
	char *mbBuffer = static_cast<char*>(MemAllocLocal(len));
	char *s = fgets(mbBuffer, len, f);
	if (s == nullptr)
	{
	   MemFreeLocal(mbBuffer);
		return nullptr;
	}
	mbBuffer[len - 1] = 0;
#if HAVE_MBSTOWCS
	mbstowcs(buffer, mbBuffer, len);
#elif defined(_WIN32)
   utf8_to_wchar(mbBuffer, -1, buffer, len);
#else
	mb_to_wchar(mbBuffer, -1, buffer, len);
#endif
   MemFreeLocal(mbBuffer);
	return buffer;
#endif
#else
	return fgets(buffer, len, f);
#endif
}

#if !HAVE_STRLWR && !defined(_WIN32)

/**
 * Convert UNICODE string to lowercase
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
   WideCharToMultiByte(CP_ACP, WC_DEFAULTCHAR | WC_COMPOSITECHECK, format, -1, mfmt, flen, NULL, NULL);
#else
   char *mbuf = (char *)MemAlloc(bufsize);
   char *mfmt = MBStringFromWideString(format);
#endif
   size_t rc = strftime(mbuf, bufsize, mfmt, t);
   if (rc > 0)
   {
      MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, mbuf, -1, buffer, (int)bufsize);
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

#if !HAVE__ITOA && !defined(_WIN32)

/**
 * _itoa() implementation
 */
char LIBNETXMS_EXPORTABLE *_itoa(int value, char *str, int base)
{
   char *p = str;
   if (value < 0)
   {
      *p++ = '-';
      value = -value;
   }

   char buffer[64];
   char *t = buffer;
   do
   {
      int rem = value % base;
      *t++ = (rem < 10) ? (rem + '0') : (rem - 10 + 'a');
      value = value / base;
   } while(value > 0);

   t--;
   while(t >= buffer)
      *p++ = *t--;
   *p = 0;
   return str;
}

#endif

#if !HAVE__ITOA && !defined(_WIN32)

/**
 * _itow() implementation
 */
WCHAR LIBNETXMS_EXPORTABLE *_itow(int value, WCHAR *str, int base)
{
   WCHAR *p = str;
   if (value < 0)
   {
      *p++ = '-';
      value = -value;
   }

   WCHAR buffer[64];
   WCHAR *t = buffer;
   do
   {
      int rem = value % base;
      *t++ = (rem < 10) ? (rem + '0') : (rem - 10 + 'a');
      value = value / base;
   } while(value > 0);

   t--;
   while(t >= buffer)
      *p++ = *t--;
   *p = 0;
   return str;
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
 * Parse timestamp (should be in form YYMMDDhhmmss or YYYYMMDDhhmmss), local time
 * If timestamp string is invalid returns default value
 */
time_t LIBNETXMS_EXPORTABLE ParseDateTimeA(const char *text, time_t defaultValue)
{
	int len = (int)strlen(text);
	if ((len != 12) && (len != 14))
		return defaultValue;

	struct tm t;
	char buffer[16], *curr;

	strncpy(buffer, text, 16);
	curr = &buffer[len - 2];

	memset(&t, 0, sizeof(struct tm));
	t.tm_isdst = -1;

	t.tm_sec = strtol(curr, NULL, 10);
	*curr = 0;
	curr -= 2;

	t.tm_min = strtol(curr, NULL, 10);
	*curr = 0;
	curr -= 2;

	t.tm_hour = strtol(curr, NULL, 10);
	*curr = 0;
	curr -= 2;

	t.tm_mday = strtol(curr, NULL, 10);
	*curr = 0;
	curr -= 2;

	t.tm_mon = strtol(curr, NULL, 10) - 1;
	*curr = 0;

	if (len == 12)
	{
		curr -= 2;
		t.tm_year = strtol(curr, NULL, 10) + 100;	// Assuming XXI century
	}
	else
	{
		curr -= 4;
		t.tm_year = strtol(curr, NULL, 10) - 1900;
	}

	return mktime(&t);
}

/**
 * Parse timestamp (should be in form YYMMDDhhmmss or YYYYMMDDhhmmss), local time
 * If timestamp string is invalid returns default value
 * (UNICODE version)
 */
time_t LIBNETXMS_EXPORTABLE ParseDateTimeW(const WCHAR *text, time_t defaultValue)
{
   char buffer[16];
   WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR, text, -1, buffer, 16, NULL, NULL);
   buffer[15] = 0;
   return ParseDateTimeA(buffer, defaultValue);
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
   HKEY hKey;
   bool found = false;
   if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("Software\\NetXMS\\Server"), 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
   {
      DWORD size = MAX_PATH * sizeof(TCHAR);
      found = (RegQueryValueEx(hKey, _T("InstallPath"), NULL, NULL, (BYTE *)installPath, &size) == ERROR_SUCCESS);
      RegCloseKey(hKey);
   }

   if (!found)
   {
      if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("Software\\NetXMS\\Agent"), 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
      {
         DWORD size = MAX_PATH * sizeof(TCHAR);
         found = (RegQueryValueEx(hKey, _T("InstallPath"), NULL, NULL, (BYTE *)installPath, &size) == ERROR_SUCCESS);
         RegCloseKey(hKey);
      }
   }

   if (!found && (GetModuleFileName(NULL, installPath, MAX_PATH) > 0))
   {
      TCHAR *p = _tcsrchr(installPath, _T('\\'));
      if (p != NULL)
      {
         *p = 0;
         p = _tcsrchr(installPath, _T('\\'));
         if (p != NULL)
         {
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
   char *buffer = NULL;
   size_t size = 0;
   FILE *f = open_memstream(&buffer, &size);
   if (f == NULL)
      return NULL;
#if WITH_JEMALLOC
   malloc_stats_print(jemalloc_stats_cb, f, NULL);
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
   return _tcsdup(_T("No heap information API available"));
#endif
}

#ifdef WITH_JEMALLOC

/**
 * Get jemalloc stat
 */
static INT64 GetJemallocStat(const char *name)
{
   size_t value;
   size_t size = sizeof(value);
   if (mallctl(name, &value, &size, NULL, 0) == 0)
      return value;
   return -1;
}

#endif

/**
 * Get amount of memory allocated by process on heap
 */
INT64 LIBNETXMS_EXPORTABLE GetAllocatedHeapMemory()
{
#ifdef WITH_JEMALLOC
   return GetJemallocStat("stats.allocated");
#endif
   return -1;
}

/**
 * Get amount of memory in active heap pages
 */
INT64 LIBNETXMS_EXPORTABLE GetActiveHeapMemory()
{
#ifdef WITH_JEMALLOC
   return GetJemallocStat("stats.active");
#endif
   return -1;
}

/**
 * Get amount of memory mapped for heap
 */
INT64 LIBNETXMS_EXPORTABLE GetMappedHeapMemory()
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
   if (prompt != NULL)
   {
      _tprintf(_T("%s"), prompt);
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
   if (_fgetts(buffer, (int)bufferSize, stdin) != NULL)
   {
      TCHAR *nl = _tcschr(buffer, _T('\n'));
      if (nl != NULL)
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

   _tprintf(_T("\n"));
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
      return NULL;
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
         return NULL;

      bool found = false;
      for(struct addrinfo *p = info; p != NULL; p = p->ai_next)
      {
         if (p->ai_canonname != NULL && strchr(p->ai_canonname, '.') != NULL)
         {
#ifdef UNICODE
            MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, p->ai_canonname, -1, buffer, size);
#else
            strncpy(buffer, p->ai_canonname, size);
#endif
            found = true;
            break;
         }
      }

      if (!found && (info != NULL))
      {
         // Use first available name as last resort
#ifdef UNICODE
         MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, info->ai_canonname, -1, buffer, size);
#else
         strncpy(buffer, info->ai_canonname, size);
#endif
         found = true;
      }

      freeaddrinfo(info);
      if (!found)
         return NULL;
#else    /* HAVE_GETADDRINFO */
      struct hostent *h = gethostbyname(hostname);
      if (h == NULL)
         return NULL;
#ifdef UNICODE
      MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, h->h_name, -1, buffer, size);
#else
      strncpy(buffer, h->h_name, size);
#endif
#endif   /* HAVE_GETADDRINFO */
   }
   else
   {
      // some systems return FQDN in gethostname call
      char *p = strchr(hostname, '.');
      if (p != NULL)
         *p = 0;
#ifdef UNICODE
      MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, hostname, -1, buffer, size);
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
static BOOL CopyFileInternal(const TCHAR *src, const TCHAR *dst, int mode)
{
   int oldFile = _topen(src, O_RDONLY | O_BINARY);
   if (oldFile == -1)
      return FALSE;

   int newFile = _topen(dst, O_CREAT | O_WRONLY | O_BINARY, mode); // should be copied with the same access rights
   if (newFile == -1)
   {
      _close(oldFile);
      return FALSE;
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
         return FALSE;
      }
   }

   _close(oldFile);
   _close(newFile);
   return TRUE;
}

#endif

/**
 * Copy file/folder
 */
bool LIBNETXMS_EXPORTABLE CopyFileOrDirectory(const TCHAR *oldName, const TCHAR *newName)
{
   NX_STAT_STRUCT st;
   if (CALL_STAT(oldName, &st) != 0)
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
   if (CALL_STAT(oldName, &st) != 0)
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
   WINTRUST_FILE_INFO fi;
   fi.cbStruct = sizeof(WINTRUST_FILE_INFO);
#ifdef UNICODE
   fi.pcwszFilePath = file;
#else
   WCHAR wfile[MAX_PATH];
   MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, file, -1, wfile, MAX_PATH);
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
   const TCHAR *v = _tgetenv(var);
   if (v != nullptr)
      _tcslcpy(buffer, v, size);
   else
      buffer[0] = 0;
   return _tcslen(buffer);
}

#endif

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
   const TCHAR *value = _tgetenv(var);
   return (value != nullptr) ? String(value) : String();
#endif
}
