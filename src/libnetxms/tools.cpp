/* 
** NetXMS - Network Management System
** Copyright (C) 2003, 2004 Victor Kirhenshtein
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

#if !defined(_WIN32) && !defined(UNDER_CE)
# include <sys/time.h>
#endif

#ifdef _WIN32
# ifndef __GNUC__
#  define EPOCHFILETIME (116444736000000000i64)
# else
#  define EPOCHFILETIME (116444736000000000LL)
# endif
#endif


//
// Calculate number of bits in netmask
//

int LIBNETXMS_EXPORTABLE BitsInMask(DWORD dwMask)
{
   int bits;
   DWORD dwTemp;

   for(bits = 0, dwTemp = ntohl(dwMask); dwTemp != 0; bits++, dwTemp <<= 1);
   return bits;
}


//
// Convert IP address from binary form (network bytes order) to string
//

TCHAR LIBNETXMS_EXPORTABLE *IpToStr(DWORD dwAddr, TCHAR *szBuffer)
{
   static TCHAR szInternalBuffer[32];
   TCHAR *szBufPtr;

   szBufPtr = szBuffer == NULL ? szInternalBuffer : szBuffer;
#if WORDS_BIGENDIAN
   _stprintf(szBufPtr, _T("%ld.%ld.%ld.%ld"), dwAddr >> 24, (dwAddr >> 16) & 255,
           (dwAddr >> 8) & 255, dwAddr & 255);
#else
   _stprintf(szBufPtr, _T("%ld.%ld.%ld.%ld"), dwAddr & 255, (dwAddr >> 8) & 255,
           (dwAddr >> 16) & 255, dwAddr >> 24);
#endif
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
// Match string against pattern with * and ? metasymbols
//

static BOOL MatchStringEngine(const TCHAR *pattern, const TCHAR *string)
{
   const TCHAR *SPtr,*MPtr,*BPtr;

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
            if (*MPtr==0)
	            return TRUE;
            if (*MPtr==_T('?'))      // Handle "*?" case
            {
               if (*SPtr!=0)
                  SPtr++;
               else
                  return FALSE;
               break;
            }
            BPtr=MPtr;           // Text block begins here
            while((*MPtr!=0)&&(*MPtr!=_T('?'))&&(*MPtr!=_T('*')))
               MPtr++;     // Find the end of text block
            while(1)
            {
               while((*SPtr!=0)&&(*SPtr!=*BPtr))
                  SPtr++;
               if (_tcslen(SPtr)<((size_t)(MPtr-BPtr)) * sizeof(TCHAR))
                  return FALSE;  // Length of remained text less than remaining pattern
               if (!memcmp(BPtr,SPtr,(MPtr-BPtr) * sizeof(TCHAR)))
                  break;
               SPtr++;
            }
            SPtr+=(MPtr-BPtr);   // Increment SPtr because we alredy match current fragment
            break;
         default:
            if (*MPtr==*SPtr)
            {
               SPtr++;
               MPtr++;
            }
            else
               return FALSE;
            break;
      }
   }

   return *SPtr==0 ? TRUE : FALSE;
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
               case _T('\\'):        // Escape
                  ptr2++;
                  if ((currIndex == index) && (pos < maxSize - 1))
                     arg[pos++] = *ptr2;
                  if (ptr2 == 0)    // Unexpected EOL
                  {
                     bResult = FALSE;
                     state = -1;
                  }
                  break;
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
               case _T('\\'):        // Escape
                  ptr2++;
                  if ((currIndex == index) && (pos < maxSize - 1))
                     arg[pos++] = *ptr2;
                  if (ptr2 == 0)    // Unexpected EOL
                  {
                     bResult = FALSE;
                     state = -1;
                  }
                  break;
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
      _tcsncpy(pszBuffer, msgBuf, iBufSize);
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

#if !(HAVE_DAEMON) && !defined(_NETWARE)

int LIBNETXMS_EXPORTABLE daemon(int nochdir, int noclose)
{
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

void LIBNETXMS_EXPORTABLE BinToStr(BYTE *pData, DWORD dwSize, char *pStr)
{
   DWORD i;
   char *pCurr;

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

DWORD LIBNETXMS_EXPORTABLE StrToBin(char *pStr, BYTE *pData, DWORD dwSize)
{
   DWORD i;
   char *pCurr;

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
