/* 
** NetXMS - Network Management System
** NetXMS Foundation Library
** Copyright (C) 2003, 2004, 2005, 2006, 2007 Victor Kirhenshtein
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
** File: string.cpp
**
**/

#include "libnetxms.h"


//
// Static members
//

const int String::npos = -1;


//
// Constructor
//

String::String()
{
   m_dwBufSize = 1;
   m_pszBuffer = NULL;
}


//
// Destructor
//

String::~String()
{
   safe_free(m_pszBuffer);
}


//
// Operator =
//

const String& String::operator =(TCHAR *pszStr)
{
   safe_free(m_pszBuffer);
   m_pszBuffer = _tcsdup(pszStr);
   m_dwBufSize = (DWORD)_tcslen(pszStr) + 1;
   return *this;
}


//
// Operator +=
//

const String&  String::operator +=(TCHAR *pszStr)
{
   DWORD dwLen;

   dwLen = (DWORD)_tcslen(pszStr);
   m_pszBuffer = (TCHAR *)realloc(m_pszBuffer, (m_dwBufSize + dwLen) * sizeof(TCHAR));
   _tcscpy(&m_pszBuffer[m_dwBufSize - 1], pszStr);
   m_dwBufSize += dwLen;
   return *this;
}


//
// Add formatted string to the end of buffer
//

void String::AddFormattedString(TCHAR *pszFormat, ...)
{
   int nLen;
   va_list args;
   TCHAR *pszBuffer;

   nLen = (int)_tcslen(pszFormat) + NumChars(pszFormat, _T('%')) * 1024;
   pszBuffer = (TCHAR *)malloc(nLen * sizeof(TCHAR));
   va_start(args, pszFormat);
   _vsntprintf(pszBuffer, nLen, pszFormat, args);
   va_end(args);
   *this += pszBuffer;
   free(pszBuffer);
}


//
// Add formatted string to the end of buffer
//

void String::AddString(TCHAR *pStr, DWORD dwSize)
{
   m_pszBuffer = (TCHAR *)realloc(m_pszBuffer, (m_dwBufSize + dwSize) * sizeof(TCHAR));
   memcpy(&m_pszBuffer[m_dwBufSize - 1], pStr, dwSize * sizeof(TCHAR));
   m_dwBufSize += dwSize;
	m_pszBuffer[m_dwBufSize - 1] = 0;
}


//
// Escape given character
//

void String::EscapeCharacter(int ch, int esc)
{
   int nCount;
   DWORD i;

   if (m_pszBuffer == NULL)
      return;

   nCount = NumChars(m_pszBuffer, ch);
   if (nCount == 0)
      return;

   m_dwBufSize += nCount;
   m_pszBuffer = (TCHAR *)realloc(m_pszBuffer, m_dwBufSize * sizeof(TCHAR));
   for(i = 0; m_pszBuffer[i] != 0; i++)
   {
      if (m_pszBuffer[i] == ch)
      {
         memmove(&m_pszBuffer[i + 1], &m_pszBuffer[i], (m_dwBufSize - i - 1) * sizeof(TCHAR));
         m_pszBuffer[i] = esc;
         i++;
      }
   }
}


//
// Set dynamically allocated string as a new buffer
//

void String::SetBuffer(TCHAR *pszBuffer)
{
   safe_free(m_pszBuffer);
   m_pszBuffer = pszBuffer;
   m_dwBufSize = (m_pszBuffer != NULL) ? (DWORD)_tcslen(m_pszBuffer) + 1 : 1;
}


//
// Translate given substring
//

void String::Translate(TCHAR *pszSrc, TCHAR *pszDst)
{
   DWORD i, dwLenSrc, dwLenDst, dwDelta;

   if (m_pszBuffer == NULL)
      return;

   dwLenSrc = (DWORD)_tcslen(pszSrc);
   dwLenDst = (DWORD)_tcslen(pszDst);

   if (m_dwBufSize <= dwLenSrc)
      return;
   
   for(i = 0; i < m_dwBufSize - dwLenSrc; i++)
   {
      if (!memcmp(pszSrc, &m_pszBuffer[i], dwLenSrc * sizeof(TCHAR)))
      {
         if (dwLenSrc == dwLenDst)
         {
            memcpy(&m_pszBuffer[i], pszDst, dwLenDst * sizeof(TCHAR));
            i += dwLenDst - 1;
         }
         else if (dwLenSrc > dwLenDst)
         {
            memcpy(&m_pszBuffer[i], pszDst, dwLenDst * sizeof(TCHAR));
            i += dwLenDst;
            dwDelta = dwLenSrc - dwLenDst;
            m_dwBufSize -= dwDelta;
            memmove(&m_pszBuffer[i], &m_pszBuffer[i + dwDelta], (m_dwBufSize - i) * sizeof(TCHAR));
            i--;
         }
         else
         {
            dwDelta = dwLenDst - dwLenSrc;
            m_pszBuffer = (TCHAR *)realloc(m_pszBuffer, (m_dwBufSize + dwDelta) * sizeof(TCHAR));
            memmove(&m_pszBuffer[i + dwLenDst], &m_pszBuffer[i + dwLenSrc], (m_dwBufSize - i - dwLenSrc) * sizeof(TCHAR));
            m_dwBufSize += dwDelta;
            memcpy(&m_pszBuffer[i], pszDst, dwLenDst * sizeof(TCHAR));
            i += dwLenDst - 1;
         }
      }
   }
}


//
// Extract substring into buffer
//

TCHAR *String::SubStr(int nStart, int nLen, TCHAR *pszBuffer)
{
	int nCount;
	TCHAR *pszOut;

	if ((nStart < (int)m_dwBufSize - 1) && (nStart >= 0))
	{
		if (nLen == -1)
		{
			nCount = (int)m_dwBufSize - nStart - 1;
		}
		else
		{
			nCount = min(nLen, (int)m_dwBufSize - nStart - 1);
		}
		pszOut = (pszBuffer != NULL) ? pszBuffer : (TCHAR *)malloc((nCount + 1) * sizeof(TCHAR));
		memcpy(pszOut, &m_pszBuffer[nStart], nCount * sizeof(TCHAR));
		pszOut[nCount] = 0;
	}
	else
	{
		pszOut = (pszBuffer != NULL) ? pszBuffer : (TCHAR *)malloc(sizeof(TCHAR));
		*pszOut = 0;
	}
	return pszOut;
}


//
// Find substring in a string
//

int String::Find(TCHAR *pszStr, int nStart)
{
	TCHAR *p;

	if ((nStart >= (int)m_dwBufSize - 1) || (nStart < 0))
		return npos;

	p = _tcsstr(&m_pszBuffer[nStart], pszStr);
	return (p != NULL) ? (((char *)p - (char *)m_pszBuffer) / sizeof(TCHAR)) : npos;
}
