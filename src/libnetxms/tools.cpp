/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2011 Victor Kirhenshtein
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

   szBufPtr = (szBuffer == NULL) ? szInternalBuffer : szBuffer;
   _sntprintf(szBufPtr, 32, _T("%d.%d.%d.%d"), (int)(dwAddr >> 24), (int)((dwAddr >> 16) & 255),
              (int)((dwAddr >> 8) & 255), (int)(dwAddr & 255));
   return szBufPtr;
}

#ifdef UNICODE

char LIBNETXMS_EXPORTABLE *IpToStrA(DWORD dwAddr, char *szBuffer)
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


//
// Convert IPv6 address from binary form to string
//

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
            while(*MPtr == _T('?'))      // Handle "*?" case
            {
               if (*SPtr != 0)
                  SPtr++;
               else
                  return FALSE;
					MPtr++;
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
                  if (_tcslen(SPtr) < (size_t)(MPtr - BPtr))
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
// Strip whitespaces and tabs off the string
//

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
// Get current time in milliseconds
// Based on timeval.h by Wu Yongwei
//

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


//
// Extract word from line. Extracted word will be placed in buffer.
// Returns pointer to the next word or to the null character if end
// of line reached.
//

const TCHAR LIBNETXMS_EXPORTABLE *ExtractWord(const TCHAR *line, TCHAR *buffer)
{
   const TCHAR *ptr;
	TCHAR *bptr;

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

TCHAR LIBNETXMS_EXPORTABLE *GetSystemErrorText(DWORD dwError, TCHAR *pszBuffer, size_t iBufSize)
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

TCHAR LIBNETXMS_EXPORTABLE *MACToStr(const BYTE *pData, TCHAR *pStr)
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
	return pStr;
}


//
// Convert byte array to text representation
//

TCHAR LIBNETXMS_EXPORTABLE *BinToStr(const BYTE *pData, DWORD dwSize, TCHAR *pStr)
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

const TCHAR LIBNETXMS_EXPORTABLE *GetCleanFileName(const TCHAR *pszFileName)
{
   const TCHAR *ptr;

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

int LIBNETXMS_EXPORTABLE SendEx(SOCKET nSocket, const void *pBuff, size_t nSize, int nFlags, MUTEX mutex)
{
	int nLeft = (int)nSize;
	int nRet;

	if (mutex != NULL)
		MutexLock(mutex, INFINITE);

	do
	{
retry:
#ifdef MSG_NOSIGNAL
		nRet = send(nSocket, ((char *)pBuff) + (nSize - nLeft), nLeft, nFlags | MSG_NOSIGNAL);
#else
		nRet = send(nSocket, ((char *)pBuff) + (nSize - nLeft), nLeft, nFlags);
#endif
		if (nRet <= 0)
		{
			if (WSAGetLastError() == WSAEWOULDBLOCK)
			{
				// Wait until socket becomes available for writing
				struct timeval tv;
				fd_set wfds;

				tv.tv_sec = 60;
				tv.tv_usec = 0;
				FD_ZERO(&wfds);
				FD_SET(nSocket, &wfds);
				nRet = select(SELECT_NFDS(nSocket + 1), NULL, &wfds, NULL, &tv);
				if ((nRet > 0) || ((nRet == -1) && (errno == EINTR)))
					goto retry;
			}
			break;
		}
		nLeft -= nRet;
	} while (nLeft > 0);

	if (mutex != NULL)
		MutexUnlock(mutex);

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
// Connect with given timeout
// Sets socket to non-blocking mode
//

int LIBNETXMS_EXPORTABLE ConnectEx(SOCKET s, struct sockaddr *addr, int len, DWORD timeout)
{
	SetSocketNonBlocking(s);

	int rc = connect(s, addr, len);
	if (rc == -1)
	{
		if ((WSAGetLastError() == WSAEWOULDBLOCK) || (WSAGetLastError() == WSAEINPROGRESS))
		{
#ifndef _WIN32
			QWORD qwStartTime;
			DWORD dwElapsed;
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

#ifdef UNICODE

DWORD LIBNETXMS_EXPORTABLE ResolveHostNameA(const char *pszName)
{
   DWORD dwAddr;
   struct hostent *hs;
   dwAddr = inet_addr(pszName);
   if ((dwAddr == INADDR_NONE) || (dwAddr == INADDR_ANY))
   {
      hs = gethostbyname(pszName);
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

BOOL LIBNETXMS_EXPORTABLE RegexpMatch(const TCHAR *pszStr, const TCHAR *pszExpr, BOOL bMatchCase)
{
   regex_t preg;
   BOOL bResult = FALSE;

	if (_tregcomp(&preg, pszExpr, bMatchCase ? REG_EXTENDED | REG_NOSUB : REG_EXTENDED | REG_NOSUB | REG_ICASE) == 0)
	{
		if (_tregexec(&preg, pszStr, 0, NULL, 0) == 0) // MATCH
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
// Split string
//

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


//
// Decrypt password
//

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
	size_t encSize;
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


//
// Load file into memory
//

#ifndef UNDER_CE

static BYTE *LoadFileContent(int fd, DWORD *pdwFileSize)
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
			pBuffer[fs.st_size] = 0;
      }
   }
   close(fd);
   return pBuffer;
}

BYTE LIBNETXMS_EXPORTABLE *LoadFile(const TCHAR *pszFileName, DWORD *pdwFileSize)
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

BYTE LIBNETXMS_EXPORTABLE *LoadFileA(const char *pszFileName, DWORD *pdwFileSize)
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


//
// Write to terminal with support for ANSI color codes
//

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

void LIBNETXMS_EXPORTABLE WriteToTerminal(const TCHAR *text)
{
#ifdef _WIN32
	HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);

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
				WriteConsole(out, curr, (DWORD)(esc - curr - 1), &chars, NULL);

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
				WriteConsole(out, curr, (DWORD)(esc - curr), &chars, NULL);
			}
			curr = esc;
		}
		else
		{
			DWORD chars;
			WriteConsole(out, curr, (DWORD)_tcslen(curr), &chars, NULL);
			break;
		}
	}
#else
	fputs(text, stdout);
#endif
}

void LIBNETXMS_EXPORTABLE WriteToTerminalEx(const TCHAR *format, ...)
{
	TCHAR buffer[8192];
	va_list args;

	va_start(args, format);
	_vsntprintf(buffer, 8192, format, args);
	va_end(args);
	WriteToTerminal(buffer);
}


//
// RefCountObject implementation
//
// Auxilliary class for objects which counts references and
// destroys itself wheren reference count falls to 0
//

RefCountObject::RefCountObject()
{
	m_refCount = 1;
	m_mutex = MutexCreate();
}

RefCountObject::~RefCountObject()
{
	MutexDestroy(m_mutex);
}

void RefCountObject::incRefCount()
{
	MutexLock(m_mutex, INFINITE);
	m_refCount++;
	MutexUnlock(m_mutex);
}

void RefCountObject::decRefCount()
{
	MutexLock(m_mutex, INFINITE);
	m_refCount--;
	if (m_refCount == 0)
	{
		MutexUnlock(m_mutex);
		delete this;
	}
	else
	{
		MutexUnlock(m_mutex);
	}
}


//
// mkstemp() implementation for Windows

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
