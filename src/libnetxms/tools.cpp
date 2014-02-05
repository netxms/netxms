/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2013 Victor Kirhenshtein
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

#include "libnetxms.h"
#include <stdarg.h>
#include <nms_agent.h>
#include <nms_threads.h>
#include <netxms-regex.h>

#ifdef _WIN32
#include <psapi.h>
#define read	_read
#define close	_close
#define wcsicmp _wcsicmp
#endif

#if !defined(_WIN32) && !defined(UNDER_CE)
#include <sys/time.h>
#endif

#if HAVE_SYS_UTSNAME_H
#include <sys/utsname.h>
#endif

#if HAVE_POLL_H
#include <poll.h>
#endif

#ifdef _WIN32
# ifndef __GNUC__
#  define EPOCHFILETIME (116444736000000000i64)
# else
#  define EPOCHFILETIME (116444736000000000LL)
# endif
#endif

/**
 * Calculate number of bits in netmask (in host byte order)
 */
int LIBNETXMS_EXPORTABLE BitsInMask(UINT32 dwMask)
{
   int bits;
   UINT32 dwTemp;

   for(bits = 0, dwTemp = dwMask; dwTemp != 0; bits++, dwTemp <<= 1);
   return bits;
}

/**
 * Convert IP address from binary form (host bytes order) to string
 */
TCHAR LIBNETXMS_EXPORTABLE *IpToStr(UINT32 dwAddr, TCHAR *szBuffer)
{
   static TCHAR szInternalBuffer[32];
   TCHAR *szBufPtr;

   szBufPtr = (szBuffer == NULL) ? szInternalBuffer : szBuffer;
   _sntprintf(szBufPtr, 32, _T("%d.%d.%d.%d"), (int)(dwAddr >> 24), (int)((dwAddr >> 16) & 255),
              (int)((dwAddr >> 8) & 255), (int)(dwAddr & 255));
   return szBufPtr;
}

#ifdef UNICODE

char LIBNETXMS_EXPORTABLE *IpToStrA(UINT32 dwAddr, char *szBuffer)
{
   static char szInternalBuffer[32];
   char *szBufPtr;

   szBufPtr = (szBuffer == NULL) ? szInternalBuffer : szBuffer;
   snprintf(szBufPtr, 32, "%d.%d.%d.%d", (int)(dwAddr >> 24), (int)((dwAddr >> 16) & 255),
            (int)((dwAddr >> 8) & 255), (int)(dwAddr & 255));
   return szBufPtr;
}

#endif


//
// Universal IPv4/IPv6 to string converter
//

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
TCHAR LIBNETXMS_EXPORTABLE *Ip6ToStr(BYTE *addr, TCHAR *buffer)
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
			while((*curr == 0) && (i < 8));
			i--;
			hasNulls = true;
		}
	}
	*out = 0;
   return bufPtr;
}

/**
 * Duplicate memory block
 */
void LIBNETXMS_EXPORTABLE *nx_memdup(const void *pData, UINT32 dwSize)
{
   void *pNewData;

   pNewData = malloc(dwSize);
   memcpy(pNewData, pData, dwSize);
   return pNewData;
}


//
// Swap two memory blocks
//

void LIBNETXMS_EXPORTABLE nx_memswap(void *pBlock1, void *pBlock2, UINT32 dwSize)
{
   void *pTemp;

   pTemp = malloc(dwSize);
   memcpy(pTemp, pBlock1, dwSize);
   memcpy(pBlock1, pBlock2, dwSize);
   memcpy(pBlock2, pTemp, dwSize);
   free(pTemp);
}


//
// Copy string
//

#if defined(_WIN32) && defined(USE_WIN32_HEAP)

char LIBNETXMS_EXPORTABLE *nx_strdup(const char *src)
{
	char *newStr = (char *)malloc(strlen(src) + 1);
	strcpy(newStr, src);
	return newStr;
}

WCHAR LIBNETXMS_EXPORTABLE *nx_wcsdup(const WCHAR *src)
{
	WCHAR *newStr = (WCHAR *)malloc((wcslen(src) + 1) * sizeof(WCHAR));
	wcscpy(newStr, src);
	return newStr;
}

#endif

#if !HAVE_WCSDUP && !defined(_WIN32)

WCHAR LIBNETXMS_EXPORTABLE *wcsdup(const WCHAR *src)
{
	return (WCHAR *)nx_memdup(src, (wcslen(src) + 1) * sizeof(WCHAR));
}

#endif

/**
 * Compare pattern with possible ? characters with given text
 */
static bool CompareTextBlocks(const TCHAR *pattern, const TCHAR *text, size_t size)
{
	const TCHAR *p = pattern;
	const TCHAR *t = text;
	for(size_t i = size; i > 0; i--, p++, t++)
	{
		if (*p == _T('?'))
			continue;
		if (*p != *t)
			return false;
	}
	return true;
}

/**
 * Match string against pattern with * and ? metasymbols - implementation
 */
static bool MatchStringEngine(const TCHAR *pattern, const TCHAR *string)
{
   const TCHAR *SPtr, *MPtr, *BPtr, *EPtr;
	size_t bsize;
   bool finishScan;

   SPtr = string;
   MPtr = pattern;

   while(*MPtr != 0)
   {
      switch(*MPtr)
      {
         case _T('?'):
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
         case _T('*'):
            while(*MPtr == _T('*'))
               MPtr++;
            if (*MPtr == 0)
	            return true;
            while(*MPtr == _T('?'))      // Handle "*?" case
            {
               if (*SPtr != 0)
                  SPtr++;
               else
                  return false;
					MPtr++;
            }
            BPtr = MPtr;           // Text block begins here
            while((*MPtr != 0) && (*MPtr != _T('*')))
               MPtr++;     // Find the end of text block
            // Try to find rightmost matching block
				bsize = (size_t)(MPtr - BPtr);
            EPtr = NULL;
            finishScan = false;
            do
            {
               while(1)
               {
                  while((*SPtr != 0) && (*SPtr != *BPtr))
                     SPtr++;
                  if (_tcslen(SPtr) < bsize)
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
                  if (CompareTextBlocks(BPtr, SPtr, bsize))
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
            if (*MPtr == *SPtr)
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
bool LIBNETXMS_EXPORTABLE MatchString(const TCHAR *pattern, const TCHAR *str, bool matchCase)
{
   if (matchCase)
      return MatchStringEngine(pattern, str);

   TCHAR *tp, *ts;
   bool bResult;

   tp = _tcsdup(pattern);
   ts = _tcsdup(str);
   _tcsupr(tp);
   _tcsupr(ts);
   bResult = MatchStringEngine(tp, ts);
   free(tp);
   free(ts);
   return bResult;
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
 * Strip whitespaces and tabs off the string
 */
void LIBNETXMS_EXPORTABLE Trim(TCHAR *str)
{
   int i;

   for(i = 0; (str[i] != 0) && _istspace(str[i]); i++);
   if (i > 0)
      memmove(str, &str[i], (_tcslen(&str[i]) + 1) * sizeof(TCHAR));
   for(i = (int)_tcslen(str) - 1; (i >= 0) && _istspace(str[i]); i--);
   str[i + 1] = 0;
}


//
// Remove trailing CR/LF or LF from string
//

void LIBNETXMS_EXPORTABLE RemoveTrailingCRLFA(char *str)
{
	if (*str == 0)
		return;

	char *p = str + strlen(str) - 1;
	if (*p == '\n')
		p--;
	if (*p == '\r')
		p--;
	*(p + 1) = 0;
}

void LIBNETXMS_EXPORTABLE RemoveTrailingCRLFW(WCHAR *str)
{
	if (*str == 0)
		return;

	WCHAR *p = str + wcslen(str) - 1;
	if (*p == L'\n')
		p--;
	if (*p == L'\r')
		p--;
	*(p + 1) = 0;
}

//
// Expand file name
// Can be used strftime placeholders and external commands enclosed in ``
//

const TCHAR LIBNETXMS_EXPORTABLE *ExpandFileName(const TCHAR *name, TCHAR *buffer, size_t bufSize)
{
	time_t t;
	struct tm *ltm;
#if HAVE_LOCALTIME_R
	struct tm tmBuff;
#endif
	TCHAR temp[8192], command[1024];
	size_t outpos = 0;

	t = time(NULL);
#if HAVE_LOCALTIME_R
	ltm = localtime_r(&t, &tmBuff);
#else
	ltm = localtime(&t);
#endif
	if (_tcsftime(temp, 8192, name, ltm) <= 0)
		return NULL;

	for(int i = 0; (temp[i] != 0) && (outpos < bufSize - 1); i++)
	{
		if (temp[i] == _T('`'))
		{
			int j = ++i;
			while((temp[j] != _T('`')) && (temp[j] != 0))
				j++;
			int len = min(j - i, 1023);
			memcpy(command, &temp[i], len * sizeof(TCHAR));
			command[len] = 0;

			FILE *p = _tpopen(command, _T("r"));
			if (p != NULL)
			{
				char result[1024];

				int rc = (int)fread(result, 1, 1023, p);
				pclose(p);

				if (rc > 0)
				{
					result[rc] = 0;
					char *lf = strchr(result, '\n');
					if (lf != NULL)
						*lf = 0;

					len = (int)min(strlen(result), bufSize - outpos - 1);
#ifdef UNICODE
					MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, result, len, &buffer[outpos], len);
#else
					memcpy(&buffer[outpos], result, len);
#endif
					outpos += len;
				}
			}

			i = j;
		}
      else if (temp[i] == _T('$') && temp[i+1] == _T('{'))
      {
         i += 2;
			int j = i;
			while((temp[j] != _T('}')) && (temp[j] != 0))
				j++;

			int len = min(j - i, 1023);

         TCHAR varName[256];
         memcpy(varName, &temp[i], len * sizeof(TCHAR));
         varName[len] = 0;

         TCHAR *result = _tgetenv(varName);
         if (result != NULL)
         {
            len = (int)min(_tcslen(result), bufSize - outpos - 1);
            memcpy(&buffer[outpos], result, len * sizeof(TCHAR));
         }
         else {
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
 * Get current time in milliseconds
 * Based on timeval.h by Wu Yongwei
 */
INT64 LIBNETXMS_EXPORTABLE GetCurrentTimeMs()
{
#ifdef _WIN32
   FILETIME ft;
   LARGE_INTEGER li;
   __int64 t;

   GetSystemTimeAsFileTime(&ft);
   li.LowPart  = ft.dwLowDateTime;
   li.HighPart = ft.dwHighDateTime;
   t = li.QuadPart;       // In 100-nanosecond intervals
   t -= EPOCHFILETIME;    // Offset to the Epoch time
   t /= 10000;            // Convert to milliseconds
#else
   struct timeval tv;
   INT64 t;

   gettimeofday(&tv, NULL);
   t = (INT64)tv.tv_sec * 1000 + (INT64)(tv.tv_usec / 1000);
#endif

   return t;
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
   *bptr=0;
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
   *bptr=0;
   return ptr;
}

#if defined(_WIN32)

/**
 * Get system error string by call to FormatMessage
 * (Windows only)
 */
TCHAR LIBNETXMS_EXPORTABLE *GetSystemErrorText(UINT32 dwError, TCHAR *pszBuffer, size_t iBufSize)
{
   TCHAR *msgBuf;

   if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
                     FORMAT_MESSAGE_FROM_SYSTEM | 
                     FORMAT_MESSAGE_IGNORE_INSERTS,
                     NULL, dwError,
                     MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                     (LPTSTR)&msgBuf, 0, NULL) > 0)
   {
      msgBuf[_tcscspn(msgBuf, _T("\r\n"))] = 0;
      nx_strncpy(pszBuffer, msgBuf, iBufSize);
      LocalFree(msgBuf);
   }
   else
   {
      _sntprintf(pszBuffer, iBufSize, _T("MSG 0x%08X - Unable to find message text"), dwError);
   }
   return pszBuffer;
}

#endif


//
// daemon() implementation for systems which doesn't have one
//

#if !(HAVE_DAEMON) && !defined(_NETWARE) && !defined(_WIN32)

int LIBNETXMS_EXPORTABLE daemon(int nochdir, int noclose)
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
TCHAR LIBNETXMS_EXPORTABLE *MACToStr(const BYTE *pData, TCHAR *pStr)
{
   UINT32 i;
   TCHAR *pCurr;

   for(i = 0, pCurr = pStr; i < 6; i++)
   {
      *pCurr++ = bin2hex(pData[i] >> 4);
      *pCurr++ = bin2hex(pData[i] & 15);
      *pCurr++ = _T(':');
   }
   *(pCurr - 1) = 0;
	return pStr;
}

/**
 * Convert byte array to text representation (wide character version)
 */
WCHAR LIBNETXMS_EXPORTABLE *BinToStrW(const BYTE *pData, UINT32 dwSize, WCHAR *pStr)
{
   UINT32 i;
   WCHAR *pCurr;

   for(i = 0, pCurr = pStr; i < dwSize; i++)
   {
      *pCurr++ = bin2hex(pData[i] >> 4);
      *pCurr++ = bin2hex(pData[i] & 15);
   }
   *pCurr = 0;
   return pStr;
}

/**
 * Convert byte array to text representation (multibyte character version)
 */
char LIBNETXMS_EXPORTABLE *BinToStrA(const BYTE *pData, UINT32 dwSize, char *pStr)
{
   UINT32 i;
   char *pCurr;

   for(i = 0, pCurr = pStr; i < dwSize; i++)
   {
      *pCurr++ = bin2hex(pData[i] >> 4);
      *pCurr++ = bin2hex(pData[i] & 15);
   }
   *pCurr = 0;
   return pStr;
}

/**
 * Convert string of hexadecimal digits to byte array (wide character version)
 * Returns number of bytes written to destination
 */
UINT32 LIBNETXMS_EXPORTABLE StrToBinW(const WCHAR *pStr, BYTE *pData, UINT32 dwSize)
{
   UINT32 i;
   const WCHAR *pCurr;

   memset(pData, 0, dwSize);
   for(i = 0, pCurr = pStr; (i < dwSize) && (*pCurr != 0); i++)
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
UINT32 LIBNETXMS_EXPORTABLE StrToBinA(const char *pStr, BYTE *pData, UINT32 dwSize)
{
   UINT32 i;
   const char *pCurr;

   memset(pData, 0, dwSize);
   for(i = 0, pCurr = pStr; (i < dwSize) && (*pCurr != 0); i++)
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


//
// Get size of file in bytes
//

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
int LIBNETXMS_EXPORTABLE SendEx(SOCKET hSocket, const void *data, size_t len, int flags, MUTEX mutex)
{
	int nLeft = (int)len;
	int nRet;

	if (mutex != NULL)
		MutexLock(mutex);

	do
	{
retry:
#ifdef MSG_NOSIGNAL
		nRet = send(hSocket, ((char *)data) + (len - nLeft), nLeft, flags | MSG_NOSIGNAL);
#else
		nRet = send(hSocket, ((char *)data) + (len - nLeft), nLeft, flags);
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
				struct timeval tv;
				fd_set wfds;

				tv.tv_sec = 60;
				tv.tv_usec = 0;
				FD_ZERO(&wfds);
				FD_SET(hSocket, &wfds);
				nRet = select(SELECT_NFDS(hSocket + 1), NULL, &wfds, NULL, &tv);
				if ((nRet > 0) || ((nRet == -1) && (errno == EINTR)))
					goto retry;
			}
			break;
		}
		nLeft -= nRet;
	} while (nLeft > 0);

	if (mutex != NULL)
		MutexUnlock(mutex);

	return nLeft == 0 ? (int)len : nRet;
}

/**
 * Extended recv() - receive data with timeout
 * 
 * @param hSocket socket handle
 * @param data data buffer
 * @param len buffer length in bytes
 * @param flags flags to be passed to recv() call
 * @param timeout waiting timeout in milliseconds
 * @return number of bytes read on success, 0 if socket was closed, -1 on error, -2 on timeout
 */
int LIBNETXMS_EXPORTABLE RecvEx(SOCKET hSocket, void *data, size_t len, int flags, UINT32 timeout)
{
   int iErr;
#if HAVE_POLL
   struct pollfd fds;
#else
   struct timeval tv;
   fd_set rdfs;
#endif
#ifndef _WIN32
   QWORD qwStartTime;
   UINT32 dwElapsed;
#endif

	// I've seen on Linux that poll() may hang if fds.fd == -1,
	// so we check this ourselves
	if (hSocket == INVALID_SOCKET)
		return -1;

   if (timeout != INFINITE)
   {
#if HAVE_POLL
      fds.fd = hSocket;
      fds.events = POLLIN;
      fds.revents = POLLIN;
      do
      {
         qwStartTime = GetCurrentTimeMs();
	      iErr = poll(&fds, 1, timeout);
         if ((iErr != -1) || (errno != EINTR))
            break;
         dwElapsed = GetCurrentTimeMs() - qwStartTime;
         timeout -= min(timeout, dwElapsed);
      } while(timeout > 0);
#else
	   FD_ZERO(&rdfs);
	   FD_SET(hSocket, &rdfs);
#ifdef _WIN32
      tv.tv_sec = timeout / 1000;
      tv.tv_usec = (timeout % 1000) * 1000;
      iErr = select(SELECT_NFDS(hSocket + 1), &rdfs, NULL, NULL, &tv);
#else
      do
      {
         tv.tv_sec = timeout / 1000;
         tv.tv_usec = (timeout % 1000) * 1000;
         qwStartTime = GetCurrentTimeMs();
         iErr = select(SELECT_NFDS(hSocket + 1), &rdfs, NULL, NULL, &tv);
         if ((iErr != -1) || (errno != EINTR))
            break;
         dwElapsed = GetCurrentTimeMs() - qwStartTime;
         timeout -= min(timeout, dwElapsed);
      } while(timeout > 0);
#endif
#endif
      if (iErr > 0)
      {
#ifdef _WIN32
         iErr = recv(hSocket, (char *)data, (int)len, flags);
#else
         do
         {
            iErr = recv(hSocket, (char *)data, len, flags);
         } while((iErr == -1) && (errno == EINTR));
#endif
      }
      else
      {
         iErr = -2;
      }
   }
   else
   {
#ifdef _WIN32
      iErr = recv(hSocket, (char *)data, (int)len, flags);
#else
      do
      {
         iErr = recv(hSocket, (char *)data, (int)len, flags);
      } while((iErr == -1) && (errno == EINTR));
#endif
   }

   return iErr;
}

/**
 * Connect with given timeout
 * Sets socket to non-blocking mode
 */
int LIBNETXMS_EXPORTABLE ConnectEx(SOCKET s, struct sockaddr *addr, int len, UINT32 timeout)
{
	SetSocketNonBlocking(s);

	int rc = connect(s, addr, len);
	if (rc == -1)
	{
		if ((WSAGetLastError() == WSAEWOULDBLOCK) || (WSAGetLastError() == WSAEINPROGRESS))
		{
#ifndef _WIN32
			QWORD qwStartTime;
			UINT32 dwElapsed;
#endif

#if HAVE_POLL
		   struct pollfd fds;

			fds.fd = s;
			fds.events = POLLOUT;
			fds.revents = POLLOUT;
			do
			{
				qwStartTime = GetCurrentTimeMs();
				rc = poll(&fds, 1, timeout);
				if ((rc != -1) || (errno != EINTR))
					break;
				dwElapsed = GetCurrentTimeMs() - qwStartTime;
				timeout -= min(timeout, dwElapsed);
			} while(timeout > 0);

			if (rc > 0)
			{
            if (fds.revents == POLLOUT)
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
				qwStartTime = GetCurrentTimeMs();
				rc = select(SELECT_NFDS(s + 1), NULL, &wrfs, &exfs, &tv);
				if ((rc != -1) || (errno != EINTR))
					break;
				dwElapsed = GetCurrentTimeMs() - qwStartTime;
				timeout -= min(timeout, dwElapsed);
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
			}
#endif
		}
	}
	return rc;
}

/**
 * Resolve host name to IP address
 *
 * @param pszName host name or IP address
 * @return IP address in network byte order
 */
UINT32 LIBNETXMS_EXPORTABLE ResolveHostName(const TCHAR *pszName)
{
   UINT32 dwAddr;
   struct hostent *hs;
#ifdef UNICODE
   char szBuffer[256];

   WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR, pszName, -1, szBuffer, 256, NULL, NULL);
   dwAddr = inet_addr(szBuffer);
#else
   dwAddr = inet_addr(pszName);
#endif
   if ((dwAddr == INADDR_NONE) || (dwAddr == INADDR_ANY))
   {
#ifdef UNICODE
      hs = gethostbyname(szBuffer);
#else
      hs = gethostbyname(pszName);
#endif
      if (hs != NULL)
      {
         memcpy(&dwAddr, hs->h_addr, sizeof(UINT32));
      }
      else
      {
         dwAddr = INADDR_NONE;
      }
   }

   return dwAddr;
}

#ifdef UNICODE

UINT32 LIBNETXMS_EXPORTABLE ResolveHostNameA(const char *pszName)
{
   UINT32 dwAddr;
   struct hostent *hs;
   dwAddr = inet_addr(pszName);
   if ((dwAddr == INADDR_NONE) || (dwAddr == INADDR_ANY))
   {
      hs = gethostbyname(pszName);
      if (hs != NULL)
      {
         memcpy(&dwAddr, hs->h_addr, sizeof(UINT32));
      }
      else
      {
         dwAddr = INADDR_NONE;
      }
   }

   return dwAddr;
}

#endif


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
   GetVersionEx(&ver);
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
#elif defined(_NETWARE)
   struct utsname un;

   uname(&un);
   _sntprintf(pszBuffer, nSize, _T("NetWare %d.%d"), un.netware_major, un.netware_minor);
   if (un.servicepack > 0)
   {
      int nLen = (int)_tcslen(pszBuffer);
      nSize -= nLen;
      if (nSize > 0)
         _sntprintf(&pszBuffer[nLen], nSize, _T(" sp%d"), un.servicepack);
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


//
// Get more specific Windows version string
//

#ifdef _WIN32

BOOL LIBNETXMS_EXPORTABLE GetWindowsVersionString(TCHAR *versionString, int strSize)
{
	OSVERSIONINFOEX ver;
	TCHAR buffer[256];

	ver.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	if (!GetVersionEx((OSVERSIONINFO *)&ver))
		return FALSE;

	switch(ver.dwMajorVersion)
	{
		case 5:
			switch(ver.dwMinorVersion)
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
					_sntprintf(buffer, 256, _T("NT %d.%d"), ver.dwMajorVersion, ver.dwMinorVersion);
					break;
			}
			break;
		case 6:
			switch(ver.dwMinorVersion)
			{
				case 0:
					_tcscpy(buffer, (ver.wProductType == VER_NT_WORKSTATION) ? _T("Vista") : _T("Server 2008"));
					break;
				case 1:
					_tcscpy(buffer, (ver.wProductType == VER_NT_WORKSTATION) ? _T("7") : _T("Server 2008 R2"));
					break;
				default:
					_sntprintf(buffer, 256, _T("NT %d.%d"), ver.dwMajorVersion, ver.dwMinorVersion);
					break;
			}
			break;
		default:
			_sntprintf(buffer, 256, _T("NT %d.%d"), ver.dwMajorVersion, ver.dwMinorVersion);
			break;
	}

	_sntprintf(versionString, strSize, _T("Windows %s Build %d %s"), buffer, ver.dwBuildNumber, ver.szCSDVersion);
	StrStrip(versionString);
	return TRUE;
}

#endif


//
// Count number of characters in string
//

int LIBNETXMS_EXPORTABLE NumCharsW(const WCHAR *pszStr, WCHAR ch)
{
   const WCHAR *p;
   int nCount;

   for(p = pszStr, nCount = 0; *p != 0; p++)
      if (*p == ch)
         nCount++;
   return nCount;
}

int LIBNETXMS_EXPORTABLE NumCharsA(const char *pszStr, char ch)
{
   const char *p;
   int nCount;

   for(p = pszStr, nCount = 0; *p != 0; p++)
      if (*p == ch)
         nCount++;
   return nCount;
}


//
// Match string against regexp
//

BOOL LIBNETXMS_EXPORTABLE RegexpMatchW(const WCHAR *pszStr, const WCHAR *pszExpr, BOOL bMatchCase)
{
   regex_t preg;
   BOOL bResult = FALSE;

	if (tre_regwcomp(&preg, pszExpr, bMatchCase ? REG_EXTENDED | REG_NOSUB : REG_EXTENDED | REG_NOSUB | REG_ICASE) == 0)
	{
		if (tre_regwexec(&preg, pszStr, 0, NULL, 0) == 0) // MATCH
			bResult = TRUE;
		regfree(&preg);
	}

   return bResult;
}

BOOL LIBNETXMS_EXPORTABLE RegexpMatchA(const char *pszStr, const char *pszExpr, BOOL bMatchCase)
{
   regex_t preg;
   BOOL bResult = FALSE;

	if (tre_regcomp(&preg, pszExpr, bMatchCase ? REG_EXTENDED | REG_NOSUB : REG_EXTENDED | REG_NOSUB | REG_ICASE) == 0)
	{
		if (tre_regexec(&preg, pszStr, 0, NULL, 0) == 0) // MATCH
			bResult = TRUE;
		regfree(&preg);
	}

   return bResult;
}


//
// Translate given code to text
//

const TCHAR LIBNETXMS_EXPORTABLE *CodeToText(int iCode, CODE_TO_TEXT *pTranslator, const TCHAR *pszDefaultText)
{
   int i;

   for(i = 0; pTranslator[i].text != NULL; i++)
      if (pTranslator[i].code == iCode)
         return pTranslator[i].text;
   return pszDefaultText;
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
long LIBNETXMS_EXPORTABLE ExtractNamedOptionValueAsIntW(const WCHAR *optString, const WCHAR *option, long defVal)
{
	WCHAR buffer[256], *eptr;
	long val;

	if (ExtractNamedOptionValueW(optString, option, buffer, 256))
	{
		val = wcstol(buffer, &eptr, 0);
		if (*eptr == 0)
			return val;
	}
	return defVal;
}

/**
 * Extract named option value as integer (multibyte version)
 */
long LIBNETXMS_EXPORTABLE ExtractNamedOptionValueAsIntA(const char *optString, const char *option, long defVal)
{
	char buffer[256], *eptr;
	long val;

	if (ExtractNamedOptionValueA(optString, option, buffer, 256))
	{
		val = strtol(buffer, &eptr, 0);
		if (*eptr == 0)
			return val;
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
	strings = (TCHAR **)malloc(sizeof(TCHAR *) * (*numStrings));
	for(int n = 0, i = 0; n < *numStrings; n++, i++)
	{
		int start = i;
		while((source[i] != sep) && (source[i] != 0))
			i++;
		int len = i - start;
		strings[n] = (TCHAR *)malloc(sizeof(TCHAR) * (len + 1));
		memcpy(strings[n], &source[start], len * sizeof(TCHAR));
		strings[n][len] = 0;
	}
	return strings;
}

/**
 * Decrypt password encrypted with nxencpassw
 */
BOOL LIBNETXMS_EXPORTABLE DecryptPassword(const TCHAR *login, const TCHAR *encryptedPasswd, TCHAR *decryptedPasswd)
{
	if (_tcslen(encryptedPasswd) != 44)
		return FALSE;

#ifdef UNICODE
	char *mbencrypted = MBStringFromWideString(encryptedPasswd);
	char *mblogin = MBStringFromWideString(login);
#else
	const char *mbencrypted = encryptedPasswd;
	const char *mblogin = login;
#endif

	BYTE encrypted[32], decrypted[32], key[16];
	size_t encSize = 32;
	base64_decode(mbencrypted, strlen(mbencrypted), (char *)encrypted, &encSize);
	if (encSize != 32)
		return FALSE;

	CalculateMD5Hash((BYTE *)mblogin, strlen(mblogin), key);
	ICEDecryptData(encrypted, 32, decrypted, key);
	decrypted[31] = 0;
	
#ifdef UNICODE
	MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, (char *)decrypted, -1, decryptedPasswd, 32);
	decryptedPasswd[31] = 0;
	free(mbencrypted);
	free(mblogin);
#else
	nx_strncpy(decryptedPasswd, (char *)decrypted, 32);
#endif

	return TRUE;
}

#ifndef UNDER_CE

/**
 * Load file content into memory
 */
static BYTE *LoadFileContent(int fd, UINT32 *pdwFileSize)
{
   int iBufPos, iNumBytes, iBytesRead;
   BYTE *pBuffer = NULL;
   struct stat fs;

   if (fstat(fd, &fs) != -1)
   {
      pBuffer = (BYTE *)malloc(fs.st_size + 1);
      if (pBuffer != NULL)
      {
         *pdwFileSize = fs.st_size;
         for(iBufPos = 0; iBufPos < fs.st_size; iBufPos += iBytesRead)
         {
            iNumBytes = min(16384, fs.st_size - iBufPos);
            if ((iBytesRead = read(fd, &pBuffer[iBufPos], iNumBytes)) < 0)
            {
               free(pBuffer);
               pBuffer = NULL;
               break;
            }
         }
         if (pBuffer != NULL)
            pBuffer[fs.st_size] = 0;
      }
   }
   close(fd);
   return pBuffer;
}

BYTE LIBNETXMS_EXPORTABLE *LoadFile(const TCHAR *pszFileName, UINT32 *pdwFileSize)
{
   int fd;
   BYTE *pBuffer = NULL;

   fd = _topen(pszFileName, O_RDONLY | O_BINARY);
   if (fd != -1)
   {
		pBuffer = LoadFileContent(fd, pdwFileSize);
   }
   return pBuffer;
}

#ifdef UNICODE

BYTE LIBNETXMS_EXPORTABLE *LoadFileA(const char *pszFileName, UINT32 *pdwFileSize)
{
   int fd;
   BYTE *pBuffer = NULL;

#ifdef _WIN32
   fd = _open(pszFileName, O_RDONLY | O_BINARY);
#else
   fd = open(pszFileName, O_RDONLY | O_BINARY);
#endif
   if (fd != -1)
   {
      pBuffer = LoadFileContent(fd, pdwFileSize);
   }
   return pBuffer;
}

#endif

#endif


//
// open/read/write for Windows CE
//

#ifdef UNDER_CE

int LIBNETXMS_EXPORTABLE _topen(TCHAR *pszName, int nFlags, ...)
{
   HANDLE hFile;
   UINT32 dwAccess, dwDisp;

   dwAccess = (nFlags & O_RDONLY) ? GENERIC_READ : 
                 (nFlags & O_WRONLY) ? GENERIC_WRITE : 
                    (nFlags & O_RDWR) ? (GENERIC_READ | GENERIC_WRITE) : 0;
   if ((nFlags & (O_CREAT | O_TRUNC)) == (O_CREAT | O_TRUNC))
      dwDisp = CREATE_ALWAYS;
   else if ((nFlags & (O_CREAT | O_EXCL)) == (O_CREAT | O_EXCL))
      dwDisp = CREATE_NEW;
   else if ((nFlags & O_CREAT) == O_CREAT)
      dwDisp = OPEN_ALWAYS;
   else if ((nFlags & O_TRUNC) == O_TRUNC)
      dwDisp = TRUNCATE_EXISTING;
   else
      dwDisp = OPEN_EXISTING;
   hFile = CreateFile(pszName, dwAccess, FILE_SHARE_READ, NULL, dwDisp,
                      FILE_ATTRIBUTE_NORMAL, NULL);
   return (hFile == INVALID_HANDLE_VALUE) ? -1 : (int)hFile;
}

int LIBNETXMS_EXPORTABLE read(int hFile, void *pBuffer, size_t nBytes)
{
   UINT32 dwBytes;

   if (ReadFile((HANDLE)hFile, pBuffer, nBytes, &dwBytes, NULL))
      return dwBytes;
   else
      return -1;
}

int LIBNETXMS_EXPORTABLE write(int hFile, void *pBuffer, size_t nBytes)
{
   UINT32 dwBytes;

   if (WriteFile((HANDLE)hFile, pBuffer, nBytes, &dwBytes, NULL))
      return dwBytes;
   else
      return -1;
}

#endif   /* UNDER_CE */


//
// Memory debugging functions
//

#ifdef NETXMS_MEMORY_DEBUG

#undef malloc
#undef realloc
#undef free

typedef struct
{
	void *pAddr;
	char szFile[MAX_PATH];
	int nLine;
	time_t nAllocTime;
	LONG nBytes;
} MEMORY_BLOCK;

static MEMORY_BLOCK *m_pBlockList = NULL;
static UINT32 m_dwNumBlocks = 0;
static MUTEX m_mutex;
static time_t m_nStartTime;

void InitMemoryDebugger(void)
{
	m_mutex = MutexCreate();
	m_nStartTime = time(NULL);
}

static void AddBlock(void *p, char *file, int line, size_t size)
{
	m_pBlockList = (MEMORY_BLOCK *)realloc(m_pBlockList, sizeof(MEMORY_BLOCK) * (m_dwNumBlocks + 1));
	m_pBlockList[m_dwNumBlocks].pAddr = p;
	strcpy(m_pBlockList[m_dwNumBlocks].szFile, file);
	m_pBlockList[m_dwNumBlocks].nLine = line;
	m_pBlockList[m_dwNumBlocks].nAllocTime = time(NULL);
	m_pBlockList[m_dwNumBlocks].nBytes = size;
	m_dwNumBlocks++;
}

static void DeleteBlock(void *ptr)
{
	UINT32 i;

	for(i = 0; i < m_dwNumBlocks; i++)
		if (m_pBlockList[i].pAddr == ptr)
		{
			m_dwNumBlocks--;
			memmove(&m_pBlockList[i], &m_pBlockList[i + 1], sizeof(MEMORY_BLOCK) * (m_dwNumBlocks - i));
			break;
		}
}

void *nx_malloc(size_t size, char *file, int line)
{
	void *p;

	p = malloc(size);
	MutexLock(m_mutex, INFINITE);
	AddBlock(p, file, line, size);
	MutexUnlock(m_mutex);
	return p;
}

void *nx_realloc(void *ptr, size_t size, char *file, int line)
{
	void *p;

	p = realloc(ptr, size);
	if (p != ptr)
	{
		MutexLock(m_mutex, INFINITE);
		DeleteBlock(ptr);
		AddBlock(p, file, line, size);
		MutexUnlock(m_mutex);
	}
	return p;
}

void nx_free(void *ptr, char *file, int line)
{
	free(ptr);
	MutexLock(m_mutex, INFINITE);
	DeleteBlock(ptr);
	MutexUnlock(m_mutex);
}

void PrintMemoryBlocks(void)
{
	UINT32 i;
	LONG nBytes;

	MutexLock(m_mutex, INFINITE);
	for(i = 0, nBytes = 0; i < m_dwNumBlocks; i++)
	{
		nBytes += m_pBlockList[i].nBytes;
		printf("%08X %d %s:%d (AGE: %d)\n", m_pBlockList[i].pAddr, m_pBlockList[i].nBytes, m_pBlockList[i].szFile, m_pBlockList[i].nLine, m_pBlockList[i].nAllocTime - m_nStartTime);
	}
	printf("%dK bytes (%d bytes) in %d blocks\n", nBytes / 1024, nBytes, m_dwNumBlocks);
	MutexUnlock(m_mutex);
}

#endif


//
// IPSO placeholders
//

#ifdef _IPSO

extern "C" void flockfile(FILE *fp)
{
}

extern "C" void funlockfile(FILE *fp)
{
}

extern "C" int __isthreaded = 1;

#endif


//
// Get memory consumed by current process
//

#ifdef _WIN32

INT64 LIBNETXMS_EXPORTABLE GetProcessRSS()
{
	PROCESS_MEMORY_COUNTERS pmc;

	if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc)))
		return pmc.WorkingSetSize;
	return 0;
}

#endif

/**
 * Apply terminal attributes to console - Win32 API specific
 */
#ifdef _WIN32

#define BG_MASK 0xF0
#define FG_MASK 0x0F

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
      WriteFile(out, mbText, (UINT32)strlen(mbText), &mode, NULL);
      free(mbText);
#else
      WriteFile(out, text, (UINT32)strlen(text), &mode, NULL);
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
	fputws(text, stdout);
#else
	char *mbtext = MBStringFromWideString(text);
	fputs(mbtext, stdout);
	free(mbtext);
#endif
#else
	fputs(text, stdout);
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

#ifndef _WIN32

/**
 * strcat_s implementation
 */
int LIBNETXMS_EXPORTABLE strcat_s(char *dst, size_t dstSize, const char *src)
{
	if (strlen(dst) + strlen(src) + 1 >= dstSize)
		return EINVAL;
	strcat(dst, src);
	return 0;
}

/**
 * wcscat_s implementation
 */
int LIBNETXMS_EXPORTABLE wcscat_s(WCHAR *dst, size_t dstSize, const WCHAR *src)
{
	if (wcslen(dst) + wcslen(src) + 1 >= dstSize)
		return EINVAL;
	wcscat(dst, src);
	return 0;
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
#if SAFE_FGETWS_WITH_POPEN
	return fgetws(buffer, len, f);
#else
	char *mbBuffer = (char *)alloca(len);
	char *s = fgets(mbBuffer, len, f);
	if (s == NULL)
		return NULL;
	mbBuffer[len - 1] = 0;
	MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, mbBuffer, -1, buffer, len);
	return buffer;
#endif
#else
	return fgets(buffer, len, f);
#endif
}
