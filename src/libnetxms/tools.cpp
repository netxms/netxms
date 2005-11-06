/* 
** NetXMS - Network Management System
** Copyright (C) 2003, 2004, 2005 Victor Kirhenshtein
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
** $module: tools.cpp
**
**/

#include "libnetxms.h"
#include <stdarg.h>
#include <nms_agent.h>

#if !defined(_WIN32) && !defined(UNDER_CE)
#include <sys/time.h>
#endif

#if HAVE_SYS_UTSNAME_H
#include <sys/utsname.h>
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

static void (* m_pLogFunction)(int, TCHAR *);


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
   _stprintf(szBufPtr, _T("%d.%d.%d.%d"), dwAddr >> 24, (dwAddr >> 16) & 255,
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
   for(i=_tcslen(str)-1;(i>=0)&&((str[i]==_T(' '))||(str[i]==_T('\t')));i--);
   str[i+1]=0;
}


//
// Add string to enumeration result set
//

void LIBNETXMS_EXPORTABLE NxAddResultString(NETXMS_VALUES_LIST *pList, TCHAR *pszString)
{
	// FIXME
   pList->ppStringList = (TCHAR **)realloc(pList->ppStringList, sizeof(TCHAR *) * (pList->dwNumStrings + 1));
   pList->ppStringList[pList->dwNumStrings] = _tcsdup(pszString);
   pList->dwNumStrings++;
}


//
// Get arguments for parameters like name(arg1,...)
// Returns FALSE on processing error
//

BOOL LIBNETXMS_EXPORTABLE NxGetParameterArg(TCHAR *param, int index, TCHAR *arg, int maxSize)
{
   TCHAR *ptr1, *ptr2;
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

BOOL LIBNETXMS_EXPORTABLE IsValidObjectName(TCHAR *pszName)
{
   static TCHAR szValidCharacters[] = _T("0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_- @()./");
   return (pszName[0] != 0) && (_tcsspn(pszName, szValidCharacters) == _tcslen(pszName));
}


//
// Convert byte array to text representation
//

void LIBNETXMS_EXPORTABLE BinToStr(BYTE *pData, DWORD dwSize, TCHAR *pStr)
{
   DWORD i;
   TCHAR *pCurr;

   for(i = 0, pCurr = pStr; i < dwSize; i++)
   {
      *pCurr++ = bin2hex(pData[i] >> 4);
      *pCurr++ = bin2hex(pData[i] & 15);
   }
   *pCurr = 0;
}


//
// Convert string of hexadecimal digits to byte array
//

DWORD LIBNETXMS_EXPORTABLE StrToBin(TCHAR *pStr, BYTE *pData, DWORD dwSize)
{
   DWORD i;
   TCHAR *pCurr;

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
// Initialize loggin for subagents
//

void LIBNETXMS_EXPORTABLE InitSubAgentsLogger(void (* pFunc)(int, TCHAR *))
{
   m_pLogFunction = pFunc;
}


//
// Write message to agent's log
//

void LIBNETXMS_EXPORTABLE NxWriteAgentLog(int iLevel, TCHAR *pszFormat, ...)
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
// Translate string
// NOTE: replacement string shouldn't be longer than original
//

void LIBNETXMS_EXPORTABLE TranslateStr(TCHAR *pszString, TCHAR *pszSubStr, TCHAR *pszReplace)
{
   TCHAR *pszSrc, *pszDst;
   int iSrcLen, iRepLen;

   iSrcLen = _tcslen(pszSubStr);
   iRepLen = _tcslen(pszReplace);
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

QWORD LIBNETXMS_EXPORTABLE FileSize(TCHAR *pszFileName)
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
   if (stat(pszFileName, &fileInfo) == -1)
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

int LIBNETXMS_EXPORTABLE NxDCIDataTypeFromText(TCHAR *pszText)
{
   static TCHAR *m_pszValidTypes[] = { _T("INT"), _T("UINT"), _T("INT64"),
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

int LIBNETXMS_EXPORTABLE SendEx(int nSocket, const void *pBuff,
		                          size_t nSize, int nFlags)
{
	int nLeft = nSize;
	int nRet;

	do
	{
		nRet = send(nSocket, ((char *)pBuff) + (nSize - nLeft), nLeft, nFlags);
		if (nRet <= 0)
		{
			break;
		}
		nLeft -= nRet;
	} while (nLeft > 0);

	return nLeft == 0 ? nSize : nRet;
}


//
// Resolve host name to IP address
//

DWORD LIBNETXMS_EXPORTABLE ResolveHostName(TCHAR *pszName)
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
   }

   return dwAddr;
}


//
// Get OS name and version
//

void LIBNETXMS_EXPORTABLE GetOSVersionString(TCHAR *pszBuffer)
{
   *pszBuffer = 0;

#if defined(_WIN32)
   OSVERSIONINFO ver;

   ver.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
   GetVersionEx(&ver);
   switch(ver.dwPlatformId)
   {
      case VER_PLATFORM_WIN32_WINDOWS:
         _stprintf(pszBuffer, _T("Win%s"), ver.dwMinorVersion == 0 ? _T("95") :
                   (ver.dwMinorVersion == 10 ? _T("98") :
                   (ver.dwMinorVersion == 90 ? _T("Me") : _T("9x"))));
         break;
      case VER_PLATFORM_WIN32_NT:
         _stprintf(pszBuffer, _T("WinNT %d.%d"), ver.dwMajorVersion, ver.dwMinorVersion);
         break;
      case VER_PLATFORM_WIN32_CE:
         _stprintf(pszBuffer, _T("WinCE %d.%d"), ver.dwMajorVersion, ver.dwMinorVersion);
         break;
      default:
         _stprintf(pszBuffer, _T("WinX %d.%d"), ver.dwMajorVersion, ver.dwMinorVersion);
         break;
   }
#elif defined(_NETWARE)
   struct utsname un;

   uname(&un);
   _stprintf(pszBuffer, _T("NetWare %d.%d"), un.netware_major, un.netware_minor);
   if (un.servicepack > 0)
      _stprintf(&pszBuffer[_tcslen(pszBuffer)], _T(" sp%d"), un.servicepack);
#else
   struct utsname un;

   uname(&un);
   _stprintf(pszBuffer, _T("%s %s.%s"), un.sysname, un.version, un.release);
#endif
}


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
