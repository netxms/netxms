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
** $module: compiler.cpp
**
**/

#include "libnxsl.h"


//
// Externals
//

extern "C" int yyparse(void *, void *);
extern "C" int yydebug;


//
// Constructor
//

NXSL_Compiler::NXSL_Compiler(void)
{
   m_pszErrorText = NULL;
   m_pLexer = NULL;
}


//
// Destructor
//

NXSL_Compiler::~NXSL_Compiler()
{
   safe_free(m_pszErrorText);
   delete m_pLexer;
}


//
// Error handler
//

void NXSL_Compiler::Error(const char *pszMsg)
{
   char szText[1024];

   snprintf(szText, 1024, "Error in line %d: %s", m_pLexer->GetCurrLine(), pszMsg);
   safe_free(m_pszErrorText);
#ifdef UNICODE
   nLen = strlen(szText) + 1;
   m_pszErrorText = (WCHAR *)malloc(nLen * sizeof(WCHAR));
   MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, szText,
                       -1, m_pszErrorText, nLen, NULL, NULL);
#else
   m_pszErrorText = strdup(szText);
#endif
}


//
// Compile source code
//

NXSL_Program *NXSL_Compiler::Compile(TCHAR *pszSourceCode)
{
   NXSL_Program *pResult;

   m_pLexer = new NXSL_Lexer(this, pszSourceCode);
yydebug=1;
   if (yyparse(m_pLexer, this) == 0)
   {
      pResult = new NXSL_Program;
   }
   else
   {
      pResult = NULL;
   }
   return pResult;
}


//
// yyerror() for parser
//

extern "C" void yyerror(void *pLexer, void *pCompiler, char *pszText)
{
   ((NXSL_Compiler *)pCompiler)->Error(pszText);
}
