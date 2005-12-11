/* 
** NetXMS - Network Management System
** NetXMS Scripting Language Interpreter
** Copyright (C) 2005 Victor Kirhenshtein
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
** $module: lexer.cpp
**
**/

#include "libnxsl.h"
#include "parser.tab.h"


//
// Constructor for our lexer class
//

NXSL_Lexer::NXSL_Lexer(NXSL_Compiler *pCompiler, TCHAR *pszCode)
           :yyFlexLexer(NULL, NULL)
{
#ifdef UNICODE
   m_nSourceSize = wcslen(pszCode);
   m_pszSourceCode = (char *)malloc(m_nSourceSize + 1);
   WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR, pszCode,
                       -1, m_pszSourceCode, m_nSourceSize + 1, NULL, NULL);
#else
   m_pszSourceCode = strdup(pszCode);
   m_nSourceSize = strlen(pszCode);
#endif
   m_nCurrLine = 1;
   m_nSourcePos = 0;
   m_pCompiler = pCompiler;
}


//
// Destructor
//

NXSL_Lexer::~NXSL_Lexer()
{
   safe_free(m_pszSourceCode);
}


//
// Alternative input method
//

int NXSL_Lexer::LexerInput(char *pBuffer, int nMaxSize)
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
// Overrided error handler
//

void NXSL_Lexer::LexerError(const char *pszMsg)
{
   m_pCompiler->Error(pszMsg);
}


//
// yylex() for Bison
//

extern "C" int yylex(YYSTYPE *lvalp, void *pLexer)
{
   return ((NXSL_Lexer *)pLexer)->yylex();
}
