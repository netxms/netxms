/* 
** Project X - Network Management System
** Copyright (C) 2003 Victor Kirhenshtein
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

#ifndef _WIN32
#include <sys/time.h>
#endif

#ifdef _WIN32
#ifndef __GNUC__
#define EPOCHFILETIME (116444736000000000i64)
#else
#define EPOCHFILETIME (116444736000000000LL)
#endif
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

char LIBNETXMS_EXPORTABLE *IpToStr(DWORD dwAddr, char *szBuffer)
{
   static char szInternalBuffer[32];
   char *szBufPtr;

   szBufPtr = szBuffer == NULL ? szInternalBuffer : szBuffer;
   sprintf(szBufPtr, "%ld.%ld.%ld.%ld", dwAddr & 255, (dwAddr >> 8) & 255,
           (dwAddr >> 16) & 255, dwAddr >> 24);
   return szBufPtr;
}


//
// Our memory allocation wrappers
// We need them primary because of Windows CLIB behavior:
// every module has it's own memory block list, so if memory block
// was allocated in DLL by malloc(), call to free() for this block in
// main program will cause crash.
//

void LIBNETXMS_EXPORTABLE *MemAlloc(DWORD dwSize)
{
   return malloc(dwSize);
}

void LIBNETXMS_EXPORTABLE *MemReAlloc(void *pBlock, DWORD dwNewSize)
{
   return realloc(pBlock, dwNewSize);
}

void LIBNETXMS_EXPORTABLE MemFree(void *pBlock)
{
   if (pBlock != NULL)
      free(pBlock);
}


//
// Duplicate memory block
//

void LIBNETXMS_EXPORTABLE *nx_memdup(const void *pData, DWORD dwSize)
{
   void *pNewData;

   pNewData = MemAlloc(dwSize);
   memcpy(pNewData, pData, dwSize);
   return pNewData;
}


//
// Duplicate string using MemAlloc()
//

char LIBNETXMS_EXPORTABLE *nx_strdup(const char *pSrc)
{
   char *pDest;

   pDest = (char *)MemAlloc(strlen(pSrc) + 1);
   strcpy(pDest, pSrc);
   return pDest;
}


//
// Match string against pattern with * and ? metasymbols
//

static BOOL MatchStringEngine(const char *pattern, const char *string)
{
   const char *SPtr,*MPtr,*BPtr;

   SPtr = string;
   MPtr = pattern;

   while(*MPtr!=0)
   {
      switch(*MPtr)
      {
         case '?':
            if (*SPtr!=0)
            {
               SPtr++;
               MPtr++;
            }
            else
               return FALSE;
            break;
         case '*':
            while(*MPtr=='*')
               MPtr++;
            if (*MPtr==0)
	            return TRUE;
            if (*MPtr=='?')      // Handle "*?" case
            {
               if (*SPtr!=0)
                  SPtr++;
               else
                  return FALSE;
               break;
            }
            BPtr=MPtr;           // Text block begins here
            while((*MPtr!=0)&&(*MPtr!='?')&&(*MPtr!='*'))
               MPtr++;     // Find the end of text block
            while(1)
            {
               while((*SPtr!=0)&&(*SPtr!=*BPtr))
                  SPtr++;
               if (strlen(SPtr)<(size_t)(MPtr-BPtr))
                  return FALSE;  // Length of remained text less than remaining pattern
               if (!memcmp(BPtr,SPtr,MPtr-BPtr))
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

BOOL LIBNETXMS_EXPORTABLE MatchString(const char *pattern, const char *string, BOOL matchCase)
{
   if (matchCase)
      return MatchStringEngine(pattern, string);
   else
   {
      char *tp, *ts;
      BOOL bResult;

      tp = strdup(pattern);
      ts = strdup(string);
      strupr(tp);
      strupr(ts);
      bResult = MatchStringEngine(tp, ts);
      free(tp);
      free(ts);
      return bResult;
   }
}


//
// Strip whitespaces and tabs off the string
//

void LIBNETXMS_EXPORTABLE StrStrip(char *str)
{
   int i;

   for(i=0;(str[i]!=0)&&((str[i]==' ')||(str[i]=='\t'));i++);
   if (i>0)
      memmove(str,&str[i],strlen(&str[i])+1);
   for(i=strlen(str)-1;(i>=0)&&((str[i]==' ')||(str[i]=='\t'));i--);
   str[i+1]=0;
}


//
// Get arguments for parameters like name(arg1,...)
// Returns FALSE on processing error
//

BOOL LIBNETXMS_EXPORTABLE GetParameterArg(char *param, int index, char *arg, int maxSize)
{
   char *ptr1, *ptr2;
   int state, currIndex, pos;
   BOOL bResult = TRUE;

   arg[0] = 0;    // Default is empty string
   ptr1 = strchr(param,'(');
   if (ptr1 == NULL)
      return TRUE;  // No arguments at all
   for(ptr2 = ptr1 + 1, currIndex = 1, state = 0, pos = 0; state != -1; ptr2++)
   {
      switch(state)
      {
         case 0:  // Normal
            switch(*ptr2)
            {
               case ')':
                  if (currIndex == index)
                     arg[pos] = 0;
                  state = -1;    // Finish processing
                  break;
               case '"':
                  state = 1;     // String
                  break;
               case '\'':        // String, type 2
                  state = 2;
                  break;
               case ',':
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
               case '"':
                  state = 0;     // Normal
                  break;
               case '\\':        // Escape
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
               case '\'':
                  state = 0;     // Normal
                  break;
               case '\\':        // Escape
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
