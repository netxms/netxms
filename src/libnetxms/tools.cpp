/* $Id$ */
/* 
** NetXMS - Network Management System
** Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008 Victor Kirhenshtein
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
** File: tools.cpp
**
**/

#include "libnetxms.h"
#include <stdarg.h>
#include <nms_agent.h>
#include <nms_threads.h>

#ifdef _WIN32
#include <netxms-regex.h>
#else
#include <regex.h>
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


//
// Static data
//

static void (* m_pLogFunction)(int, TCHAR *) = NULL;
static void (* m_pTrapFunction1)(DWORD, int, TCHAR **) = NULL;
static void (* m_pTrapFunction2)(DWORD, const char *, va_list) = NULL;


//
// Calculate number of bits in netmask (in host byte order)
//

int LIBNETXMS_EXPORTABLE BitsInMask(DWORD dwMask)
{
   int bits;
   DWORD dwTemp;

   for(bits = 0, dwTemp = dwMask; dwTemp != 0; bits++, dwTemp <<= 1);
   return bits;
}


//
// Convert IP address from binary form (host bytes order) to string
//

TCHAR LIBNETXMS_EXPORTABLE *IpToStr(DWORD dwAddr, TCHAR *szBuffer)
{
   static TCHAR szInternalBuffer[32];
   TCHAR *szBufPtr;

   szBufPtr = szBuffer == NULL ? szInternalBuffer : szBuffer;
   _sntprintf(szBufPtr, 32, _T("%d.%d.%d.%d"), dwAddr >> 24, (dwAddr >> 16) & 255,
             (dwAddr >> 8) & 255, dwAddr & 255);
   return szBufPtr;
}


//
// Duplicate memory block
//

void LIBNETXMS_EXPORTABLE *nx_memdup(const void *pData, DWORD dwSize)
{
   void *pNewData;

   pNewData = malloc(dwSize);
   memcpy(pNewData, pData, dwSize);
   return pNewData;
}


//
// Swap two memory blocks
//

void LIBNETXMS_EXPORTABLE nx_memswap(void *pBlock1, void *pBlock2, DWORD dwSize)
{
   void *pTemp;

   pTemp = malloc(dwSize);
   memcpy(pTemp, pBlock1, dwSize);
   memcpy(pBlock1, pBlock2, dwSize);
   memcpy(pBlock2, pTemp, dwSize);
   free(pTemp);
}


//
// Match string against pattern with * and ? metasymbols
//

static BOOL MatchStringEngine(const TCHAR *pattern, const TCHAR *string)
{
   const TCHAR *SPtr, *MPtr, *BPtr, *EPtr;
   BOOL bFinishScan;

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
               return FALSE;
            break;
         case _T('*'):
            while(*MPtr == _T('*'))
               MPtr++;
            if (*MPtr == 0)
	            return TRUE;
            if (*MPtr == _T('?'))      // Handle "*?" case
            {
               if (*SPtr != 0)
                  SPtr++;
               else
                  return FALSE;
               break;
            }
            BPtr = MPtr;           // Text block begins here
            while((*MPtr != 0) && (*MPtr != _T('?')) && (*MPtr != _T('*')))
               MPtr++;     // Find the end of text block
            // Try to find rightmost matching block
            EPtr = NULL;
            bFinishScan = FALSE;
            do
            {
               while(1)
               {
                  while((*SPtr != 0) && (*SPtr != *BPtr))
                     SPtr++;
                  if (_tcslen(SPtr) < ((size_t)(MPtr - BPtr)) * sizeof(TCHAR))
                  {
                     if (EPtr == NULL)
                     {
                        return FALSE;  // Length of remained text less than remaining pattern
                     }
                     else
                     {
                        SPtr = EPtr;   // Revert back to last match
                        bFinishScan = TRUE;
                        break;
                     }
                  }
                  if (!memcmp(BPtr, SPtr, (MPtr - BPtr) * sizeof(TCHAR)))
                     break;
                  SPtr++;
               }
               if (!bFinishScan)
               {
                  SPtr += (MPtr - BPtr);   // Increment SPtr because we alredy match current fragment
                  EPtr = SPtr;   // Remember current point
               }
            }
            while(!bFinishScan);
            break;
         default:
            if (*MPtr == *SPtr)
            {
               SPtr++;
               MPtr++;
            }
            else
               return FALSE;
            break;
      }
   }

   return *SPtr == 0 ? TRUE : FALSE;
}

BOOL LIBNETXMS_EXPORTABLE MatchString(const TCHAR *pattern,
									  const TCHAR *string,
									  BOOL matchCase)
{
   if (matchCase)
      return MatchStringEngine(pattern, string);
   else
   {
      TCHAR *tp, *ts;
      BOOL bResult;

      tp = _tcsdup(pattern);
      ts = _tcsdup(string);
      _tcsupr(tp);
      _tcsupr(ts);
      bResult = MatchStringEngine(tp, ts);
      free(tp);
      free(ts);
      return bResult;
   }
}


//
// Strip whitespaces and tabs off the string
//

void LIBNETXMS_EXPORTABLE StrStrip(TCHAR *str)
{
   int i;

   for(i=0;(str[i]!=0)&&((str[i]==_T(' '))||(str[i]==_T('\t')));i++);
   if (i>0)
      memmove(str,&str[i],(_tcslen(&str[i])+1) * sizeof(TCHAR));
   for(i=(int)_tcslen(str)-1;(i>=0)&&((str[i]==_T(' '))||(str[i]==_T('\t')));i--);
   str[i+1]=0;
}


//
// Add string to enumeration result set
//

void LIBNETXMS_EXPORTABLE NxAddResultString(NETXMS_VALUES_LIST *pList, const TCHAR *pszString)
{
	// FIXME
   pList->ppStringList = (TCHAR **)realloc(pList->ppStringList, sizeof(TCHAR *) * (pList->dwNumStrings + 1));
   pList->ppStringList[pList->dwNumStrings] = _tcsdup(pszString);
   pList->dwNumStrings++;
}


//
// Destroy dynamically created values list
//

void LIBNETXMS_EXPORTABLE NxDestroyValuesList(NETXMS_VALUES_LIST *pList)
{
	DWORD i;

	if (pList != NULL)
	{
		for(i = 0; i < pList->dwNumStrings; i++)
			safe_free(pList->ppStringList[i]);
		safe_free(pList->ppStringList);
		free(pList);
	}
}


//
// Get arguments for parameters like name(arg1,...)
// Returns FALSE on processing error
//

BOOL LIBNETXMS_EXPORTABLE NxGetParameterArg(const TCHAR *param, int index, TCHAR *arg, int maxSize)
{
   const TCHAR *ptr1, *ptr2;
   int state, currIndex, pos;
   BOOL bResult = TRUE;

   arg[0] = 0;    // Default is empty string
   ptr1 = _tcschr(param, _T('('));
   if (ptr1 == NULL)
      return TRUE;  // No arguments at all
   for(ptr2 = ptr1 + 1, currIndex = 1, state = 0, pos = 0; state != -1; ptr2++)
   {
      switch(state)
      {
         case 0:  // Normal
            switch(*ptr2)
            {
               case _T(')'):
                  if (currIndex == index)
                     arg[pos] = 0;
                  state = -1;    // Finish processing
                  break;
               case _T('"'):
                  state = 1;     // String
                  break;
               case _T('\''):        // String, type 2
                  state = 2;
                  break;
               case _T(','):
                  if (currIndex == index)
                  {
                     arg[pos] = 0;
                     state = -1;
                  }
                  else
                  {
                     currIndex++;
                  }
                  break;
               case 0:
                  state = -1;       // Finish processing
                  bResult = FALSE;  // Set error flag
                  break;
               default:
                  if ((currIndex == index) && (pos < maxSize - 1))
                     arg[pos++] = *ptr2;
            }
            break;
         case 1:  // String in ""
            switch(*ptr2)
            {
               case _T('"'):
                  state = 0;     // Normal
                  break;
/*               case _T('\\'):        // Escape
                  ptr2++;
                  if ((currIndex == index) && (pos < maxSize - 1))
                     arg[pos++] = *ptr2;
                  if (ptr2 == 0)    // Unexpected EOL
                  {
                     bResult = FALSE;
                     state = -1;
                  }
                  break;*/
               case 0:
                  state = -1;    // Finish processing
                  bResult = FALSE;  // Set error flag
                  break;
               default:
                  if ((currIndex == index) && (pos < maxSize - 1))
                     arg[pos++] = *ptr2;
            }
            break;
         case 2:  // String in ''
            switch(*ptr2)
            {
               case _T('\''):
                  state = 0;     // Normal
                  break;
/*               case _T('\\'):        // Escape
                  ptr2++;
                  if ((currIndex == index) && (pos < maxSize - 1))
                     arg[pos++] = *ptr2;
                  if (ptr2 == 0)    // Unexpected EOL
                  {
                     bResult = FALSE;
                     state = -1;
                  }
                  break;*/
               case 0:
                  state = -1;    // Finish processing
                  bResult = FALSE;  // Set error flag
                  break;
               default:
                  if ((currIndex == index) && (pos < maxSize - 1))
                     arg[pos++] = *ptr2;
            }
            break;
      }
   }

   if (bResult)
      StrStrip(arg);
   return bResult;
}


//
// Get current time in milliseconds
// Based on timeval.h by Wu Yongwei
//

INT64 LIBNETXMS_EXPORTABLE GetCurrentTimeMs(void)
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
   t = (INT64)tv.tv_sec * 1000 + (INT64)(tv.tv_usec / 10000);
#endif

   return t;
}


//
// Extract word from line. Extracted word will be placed in buffer.
// Returns pointer to the next word or to the null character if end
// of line reached.
//

TCHAR LIBNETXMS_EXPORTABLE *ExtractWord(TCHAR *line, TCHAR *buffer)
{
   TCHAR *ptr,*bptr;

   for(ptr=line;(*ptr==_T(' '))||(*ptr==_T('\t'));ptr++);  // Skip initial spaces
   // Copy word to buffer
   for(bptr=buffer;(*ptr!=_T(' '))&&(*ptr!=_T('\t'))&&(*ptr!=0);ptr++,bptr++)
      *bptr=*ptr;
   *bptr=0;
   return ptr;
}


//
// Get system error string by call to FormatMessage
// (Windows only)
//

#if defined(_WIN32)

TCHAR LIBNETXMS_EXPORTABLE *GetSystemErrorText(DWORD dwError, TCHAR *pszBuffer, int iBufSize)
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
      _stprintf(pszBuffer, _T("MSG 0x%08X - Unable to find message text"), dwError);
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


//
// Check if given name is a valid object name
//

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


//
// Check if given name is a valid script name
//

BOOL LIBNETXMS_EXPORTABLE IsValidScriptName(const TCHAR *pszName)
{
   static TCHAR szValidCharacters[] = _T("0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_:");
   return (pszName[0] != 0) && (!isdigit(pszName[0])) && (pszName[0] != ':') &&
          (_tcsspn(pszName, szValidCharacters) == _tcslen(pszName));
}


//
// Convert 6-byte MAC address to text representation
//

void LIBNETXMS_EXPORTABLE MACToStr(BYTE *pData, TCHAR *pStr)
{
   DWORD i;
   TCHAR *pCurr;

   for(i = 0, pCurr = pStr; i < 6; i++)
   {
      *pCurr++ = bin2hex(pData[i] >> 4);
      *pCurr++ = bin2hex(pData[i] & 15);
      *pCurr++ = _T(':');
   }
   *(pCurr - 1) = 0;
}


//
// Convert byte array to text representation
//

TCHAR LIBNETXMS_EXPORTABLE *BinToStr(BYTE *pData, DWORD dwSize, TCHAR *pStr)
{
   DWORD i;
   TCHAR *pCurr;

   for(i = 0, pCurr = pStr; i < dwSize; i++)
   {
      *pCurr++ = bin2hex(pData[i] >> 4);
      *pCurr++ = bin2hex(pData[i] & 15);
   }
   *pCurr = 0;
   return pStr;
}


//
// Convert string of hexadecimal digits to byte array
//

DWORD LIBNETXMS_EXPORTABLE StrToBin(const TCHAR *pStr, BYTE *pData, DWORD dwSize)
{
   DWORD i;
   const TCHAR *pCurr;

   memset(pData, 0, dwSize);
   for(i = 0, pCurr = pStr; (i < dwSize) && (*pCurr != 0); i++)
   {
      pData[i] = hex2bin(*pCurr) << 4;
      pCurr++;
      pData[i] |= hex2bin(*pCurr);
      pCurr++;
   }
   return i;
}


//
// Initialize logging for subagents
//

void LIBNETXMS_EXPORTABLE InitSubAgentsLogger(void (* pFunc)(int, TCHAR *))
{
   m_pLogFunction = pFunc;
}


//
// Write message to agent's log
//

void LIBNETXMS_EXPORTABLE NxWriteAgentLog(int iLevel, const TCHAR *pszFormat, ...)
{
   TCHAR szBuffer[4096];
   va_list args;

   if (m_pLogFunction != NULL)
   {
      va_start(args, pszFormat);
      _vsntprintf(szBuffer, 4096, pszFormat, args);
      va_end(args);
      m_pLogFunction(iLevel, szBuffer);
   }
}


//
// Initialize trap sending functions for subagent
//

void LIBNETXMS_EXPORTABLE InitSubAgentsTrapSender(void (* pFunc1)(DWORD, int, TCHAR **),
                                                  void (* pFunc2)(DWORD, const char *, va_list))
{
   m_pTrapFunction1 = pFunc1;
   m_pTrapFunction2 = pFunc2;
}


//
// Send trap from agent to server
//

void LIBNETXMS_EXPORTABLE NxSendTrap(DWORD dwEvent, const char *pszFormat, ...)
{
   va_list args;

   if (m_pTrapFunction2 != NULL)
   {
      va_start(args, pszFormat);
      m_pTrapFunction2(dwEvent, pszFormat, args);
      va_end(args);
   }
}

void LIBNETXMS_EXPORTABLE NxSendTrap2(DWORD dwEvent, int nCount, TCHAR **ppszArgList)
{
   if (m_pTrapFunction1 != NULL)
      m_pTrapFunction1(dwEvent, nCount, ppszArgList);
}


//
// Translate string
// NOTE: replacement string shouldn't be longer than original
//

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

QWORD LIBNETXMS_EXPORTABLE FileSize(const TCHAR *pszFileName)
{
#ifdef _WIN32
   HANDLE hFind;
   WIN32_FIND_DATA fd;
#else
   struct stat fileInfo;
#endif

#ifdef _WIN32
   hFind = FindFirstFile(pszFileName, &fd);
   if (hFind == INVALID_HANDLE_VALUE)
      return 0;
   FindClose(hFind);

   return (unsigned __int64)fd.nFileSizeLow + ((unsigned __int64)fd.nFileSizeHigh << 32);
#else
   if (_tstat(pszFileName, &fileInfo) == -1)
      return 0;

   return (QWORD)fileInfo.st_size;
#endif
}


//
// Get pointer to clean file name (without path specification)
//

TCHAR LIBNETXMS_EXPORTABLE *GetCleanFileName(TCHAR *pszFileName)
{
   TCHAR *ptr;

   ptr = pszFileName + _tcslen(pszFileName);
   while((ptr >= pszFileName) && (*ptr != _T('/')) && (*ptr != _T('\\')) && (*ptr != _T(':')))
      ptr--;
   return (ptr + 1);
}


//
// Translate DCI data type from text form to code
//

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


//
// Extended send() - send all data even if single call to send()
// cannot handle them all
//

int LIBNETXMS_EXPORTABLE SendEx(SOCKET nSocket, const void *pBuff,
		                          size_t nSize, int nFlags)
{
	int nLeft = (int)nSize;
	int nRet;

	do
	{
retry:
		nRet = send(nSocket, ((char *)pBuff) + (nSize - nLeft), nLeft, nFlags);
		if (nRet <= 0)
		{
			if (WSAGetLastError() == WSAEWOULDBLOCK)
			{
				// retry operation
				goto retry;
			}
			break;
		}
		nLeft -= nRet;
	} while (nLeft > 0);

	return nLeft == 0 ? (int)nSize : nRet;
}


//
// Extended recv() - receive data with timeout
//

int LIBNETXMS_EXPORTABLE RecvEx(SOCKET nSocket, const void *pBuff,
                                size_t nSize, int nFlags, DWORD dwTimeout)
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
   DWORD dwElapsed;
#endif

	// I've seen on Linux that poll() may hang if fds.fd == -1,
	// so we check this ourselves
	if (nSocket == -1)
		return -1;

   if (dwTimeout != INFINITE)
   {
#if HAVE_POLL
      fds.fd = nSocket;
      fds.events = POLLIN;
      fds.revents = POLLIN;
      do
      {
         qwStartTime = GetCurrentTimeMs();
	      iErr = poll(&fds, 1, dwTimeout);
         if ((iErr != -1) || (errno != EINTR))
            break;
         dwElapsed = GetCurrentTimeMs() - qwStartTime;
         dwTimeout -= min(dwTimeout, dwElapsed);
      } while(dwTimeout > 0);
#else
	   FD_ZERO(&rdfs);
	   FD_SET(nSocket, &rdfs);
#ifdef _WIN32
      tv.tv_sec = dwTimeout / 1000;
      tv.tv_usec = (dwTimeout % 1000) * 1000;
      iErr = select(SELECT_NFDS(nSocket + 1), &rdfs, NULL, NULL, &tv);
#else
      do
      {
         tv.tv_sec = dwTimeout / 1000;
         tv.tv_usec = (dwTimeout % 1000) * 1000;
         qwStartTime = GetCurrentTimeMs();
         iErr = select(SELECT_NFDS(nSocket + 1), &rdfs, NULL, NULL, &tv);
         if ((iErr != -1) || (errno != EINTR))
            break;
         dwElapsed = GetCurrentTimeMs() - qwStartTime;
         dwTimeout -= min(dwTimeout, dwElapsed);
      } while(dwTimeout > 0);
#endif
#endif
      if (iErr > 0)
      {
#ifdef _WIN32
         iErr = recv(nSocket, (char *)pBuff, (int)nSize, nFlags);
#else
         do
         {
            iErr = recv(nSocket, (char *)pBuff, nSize, nFlags);
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
      iErr = recv(nSocket, (char *)pBuff, (int)nSize, nFlags);
#else
      do
      {
         iErr = recv(nSocket, (char *)pBuff, nSize, nFlags);
      } while((iErr == -1) && (errno == EINTR));
#endif
   }

   return iErr;
}


//
// Resolve host name to IP address
//

DWORD LIBNETXMS_EXPORTABLE ResolveHostName(const TCHAR *pszName)
{
   DWORD dwAddr;
   struct hostent *hs;
#ifdef UNICODE
   char szBuffer[256];

   WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR,
                       pszName, -1, szBuffer, 256, NULL, NULL);
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
         memcpy(&dwAddr, hs->h_addr, sizeof(DWORD));
      }
      else
      {
         dwAddr = INADDR_NONE;
      }
   }

   return dwAddr;
}


#ifndef VER_PLATFORM_WIN32_WINDOWS
#define VER_PLATFORM_WIN32_WINDOWS 1
#endif
#ifndef VER_PLATFORM_WIN32_CE
#define VER_PLATFORM_WIN32_CE 3
#endif

//
// Get OS name and version
//

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
// Count number of characters in string
//

int LIBNETXMS_EXPORTABLE NumChars(const TCHAR *pszStr, int ch)
{
   const TCHAR *p;
   int nCount;

   for(p = pszStr, nCount = 0; *p != 0; p++)
      if (*p == ch)
         nCount++;
   return nCount;
}


//
// Match string against regexp
//

BOOL LIBNETXMS_EXPORTABLE RegexpMatch(TCHAR *pszStr, TCHAR *pszExpr, BOOL bMatchCase)
{
#ifdef UNICODE
   regex_t preg;
	char *mbStr, *mbExpr;
   BOOL bResult = FALSE;

	mbStr = MBStringFromWideString(pszStr);
	mbExpr = MBStringFromWideString(pszExpr);
	if (regcomp(&preg, mbExpr, bMatchCase ? REG_EXTENDED | REG_NOSUB : REG_EXTENDED | REG_NOSUB | REG_ICASE) == 0)
	{
		if (regexec(&preg, mbStr, 0, NULL, 0) == 0) // MATCH
			bResult = TRUE;
		regfree(&preg);
	}
	free(mbStr);
	free(mbExpr);

   return bResult;
#else
   regex_t preg;
   BOOL bResult = FALSE;

	if (regcomp(&preg, pszExpr, bMatchCase ? REG_EXTENDED | REG_NOSUB : REG_EXTENDED | REG_NOSUB | REG_ICASE) == 0)
	{
		if (regexec(&preg, pszStr, 0, NULL, 0) == 0) // MATCH
			bResult = TRUE;
		regfree(&preg);
	}

   return bResult;
#endif
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


//
// Extract option value from string of form option=value;option=value;...
//

BOOL LIBNETXMS_EXPORTABLE ExtractNamedOptionValue(const TCHAR *optString, const TCHAR *option, TCHAR *buffer, int bufSize)
{
	int state, pos;
	const TCHAR *curr, *start;
	TCHAR temp[256];

	for(curr = start = optString, pos = 0, state = 0; *curr != 0; curr++)
	{
		switch(*curr)
		{
			case _T(';'):		// Next option
				if (state == 1)
				{
					buffer[pos] = 0;
					StrStrip(buffer);
					return TRUE;
				}
				state = 0;
				start = curr + 1;
				break;
			case _T('='):
				if (state == 0)
				{
					_tcsncpy(temp, start, curr - start);
					temp[curr - start] = 0;
					StrStrip(temp);
					if (!_tcsicmp(option, temp))
						state = 1;
					else
						state = 2;
				}
				else if ((state == 1) && (pos < bufSize - 1))
				{
					buffer[pos++] = _T('=');
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
		StrStrip(buffer);
		return TRUE;
	}
	
	return FALSE;
}

BOOL LIBNETXMS_EXPORTABLE ExtractNamedOptionValueAsBool(const TCHAR *optString, const TCHAR *option, BOOL defVal)
{
	TCHAR buffer[256];
	
	if (ExtractNamedOptionValue(optString, option, buffer, 256))
	{
		if (!_tcsicmp(buffer, _T("yes")) || !_tcsicmp(buffer, _T("true")))
			return TRUE;
		return FALSE;
	}
	return defVal;
}

long LIBNETXMS_EXPORTABLE ExtractNamedOptionValueAsInt(const TCHAR *optString, const TCHAR *option, long defVal)
{
	TCHAR buffer[256], *eptr;
	long val;

	if (ExtractNamedOptionValue(optString, option, buffer, 256))
	{
		val = _tcstol(buffer, &eptr, 0);
		if (*eptr == 0)
			return val;
	}
	return defVal;
}


//
// Load file into memory
//

#ifndef UNDER_CE

BYTE LIBNETXMS_EXPORTABLE *LoadFile(const TCHAR *pszFileName, DWORD *pdwFileSize)
{
   int fd, iBufPos, iNumBytes, iBytesRead;
   BYTE *pBuffer = NULL;
   struct stat fs;

   fd = _topen(pszFileName, O_RDONLY | O_BINARY);
   if (fd != -1)
   {
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
         }
      }
      close(fd);
   }
   return pBuffer;
}

#endif


//
// open/read/write for Windows CE
//

#ifdef UNDER_CE

int LIBNETXMS_EXPORTABLE _topen(TCHAR *pszName, int nFlags, ...)
{
   HANDLE hFile;
   DWORD dwAccess, dwDisp;

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
   DWORD dwBytes;

   if (ReadFile((HANDLE)hFile, pBuffer, nBytes, &dwBytes, NULL))
      return dwBytes;
   else
      return -1;
}

int LIBNETXMS_EXPORTABLE write(int hFile, void *pBuffer, size_t nBytes)
{
   DWORD dwBytes;

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
static DWORD m_dwNumBlocks = 0;
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
	DWORD i;

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
	DWORD i;
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
