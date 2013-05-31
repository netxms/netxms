/* 
** NetXMS - Network Management System
** NetXMS Scripting Language Interpreter
** Copyright (C) 2003-2013 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation; either version 3 of the License, or
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
** File: lexer.cpp
**
**/

#include "libnxsl.h"
#include "parser.tab.hpp"

/**
 * Constructor for our lexer class
 */
NXSL_Lexer::NXSL_Lexer(NXSL_Compiler *pCompiler, const TCHAR *pszCode)
{
#ifdef UNICODE
	m_pszSourceCode = UTF8StringFromWideString(pszCode);
#else
   m_pszSourceCode = strdup(pszCode);
#endif
   m_nSourceSize = (int)strlen(m_pszSourceCode);
   m_nCurrLine = 1;
   m_nSourcePos = 0;
   m_pCompiler = pCompiler;
   m_bErrorState = FALSE;
}

/**
 * Destructor
 */
NXSL_Lexer::~NXSL_Lexer()
{
   safe_free(m_pszSourceCode);
}

/**
 * Alternative input method
 */
int NXSL_Lexer::lexerInput(char *pBuffer, int nMaxSize)
{
   int nBytes;

   if (m_nSourcePos < m_nSourceSize)
   {
      nBytes = min(nMaxSize, m_nSourceSize - m_nSourcePos);
      memcpy(pBuffer, &m_pszSourceCode[m_nSourcePos], nBytes);
      m_nSourcePos += nBytes;
   }
   else
   {
      nBytes = 0;   // EOF
   }
   return nBytes;
}


//
// Report error
//

void NXSL_Lexer::error(const char *pszText)
{
	m_pCompiler->error(pszText);
}
