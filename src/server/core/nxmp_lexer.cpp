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
** File: nxmp_lexer.cpp
**
**/

#include "nxcore.h"
#include "nxmp_parser.h"

#ifndef min
#define min(a,b) ((a) < (b) ? (a) : (b))
#endif


//
// Constructor for our lexer class
//

NXMP_Lexer::NXMP_Lexer(NXMP_Parser *pParser, TCHAR *pszSource)
{
#ifdef UNICODE
   m_nSourceSize = wcslen(pszCode);
   m_pszSource = (char *)malloc(m_nSourceSize + 1);
   WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR, pszSource,
                       -1, m_pszSource, m_nSourceSize + 1, NULL, NULL);
#else
   m_pszSource = strdup(pszSource);
   m_nSourceSize = (int)strlen(pszSource);
#endif
   m_nCurrLine = 1;
   m_nSourcePos = 0;
   m_pParser = pParser;
   m_bErrorState = FALSE;
}


//
// Destructor
//

NXMP_Lexer::~NXMP_Lexer()
{
   safe_free(m_pszSource);
}


//
// Alternative input method
//

int NXMP_Lexer::LexerInput(char *pBuffer, int nMaxSize)
{
   int nBytes;

   if (m_nSourcePos < m_nSourceSize)
   {
      nBytes = min(nMaxSize, m_nSourceSize - m_nSourcePos);
      memcpy(pBuffer, &m_pszSource[m_nSourcePos], nBytes);
      m_nSourcePos += nBytes;
   }
   else
   {
      nBytes = 0;   // EOF
   }   
   return nBytes;
}


//
// Overrided error handler
//

void NXMP_Lexer::Error(const char *pszMsg)
{
   m_pParser->Error(pszMsg);
}
