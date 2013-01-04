/* 
** NetXMS - Network Management System
** NetXMS Scripting Language Interpreter
** Copyright (C) 2003-2010 Victor Kirhenshtein
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
** File: compiler.cpp
**
**/

#include "libnxsl.h"


//
// Externals
//

int yyparse(yyscan_t scanner, NXSL_Lexer *, NXSL_Compiler *, NXSL_Program *);
extern int yydebug;
void yyset_extra(NXSL_Lexer *, yyscan_t);
int yylex_init(yyscan_t *);
int yylex_destroy(yyscan_t);


//
// Constructor
//

NXSL_Compiler::NXSL_Compiler()
{
   m_pszErrorText = NULL;
   m_pLexer = NULL;
   m_pAddrStack = new NXSL_Stack;
	m_pBreakStack = new NXSL_Stack;
}


//
// Destructor
//

NXSL_Compiler::~NXSL_Compiler()
{
   safe_free(m_pszErrorText);
   delete m_pLexer;
   delete m_pAddrStack;
	delete m_pBreakStack;
}


//
// Error handler
//

void NXSL_Compiler::error(const char *pszMsg)
{
   char szText[1024];

   if (m_pszErrorText == NULL)
   {
      snprintf(szText, 1024, "Error in line %d: %s", m_pLexer->getCurrLine(), pszMsg);
#ifdef UNICODE
		m_pszErrorText = WideStringFromMBString(pszMsg);
#else
      m_pszErrorText = strdup(szText);
#endif
   }
}


//
// Compile source code
//

NXSL_Program *NXSL_Compiler::compile(const TCHAR *pszSourceCode)
{
   NXSL_Program *pResult;
	yyscan_t scanner;

   m_pLexer = new NXSL_Lexer(this, pszSourceCode);
   pResult = new NXSL_Program;
	yylex_init(&scanner);
	yyset_extra(m_pLexer, scanner);
   if (yyparse(scanner, m_pLexer, this, pResult) == 0)
   {
      pResult->resolveFunctions();
		pResult->optimize();
   }
   else
   {
      delete pResult;
      pResult = NULL;
   }
	yylex_destroy(scanner);
   return pResult;
}


//
// yyerror() for parser
//

void yyerror(yyscan_t scanner, NXSL_Lexer *pLexer, NXSL_Compiler *pCompiler,
             NXSL_Program *pScript, const char *pszText)
{
   pCompiler->error(pszText);
}


//
// Pop address
//

DWORD NXSL_Compiler::popAddr()
{
   void *pAddr;

   pAddr = m_pAddrStack->pop();
   return pAddr ? CAST_FROM_POINTER(pAddr, DWORD) : INVALID_ADDRESS;
}


//
// Peek address
//

DWORD NXSL_Compiler::peekAddr()
{
   void *pAddr;

   pAddr = m_pAddrStack->peek();
   return pAddr ? CAST_FROM_POINTER(pAddr, DWORD) : INVALID_ADDRESS;
}


//
// Add "break" statement address
//

void NXSL_Compiler::addBreakAddr(DWORD dwAddr)
{
	Queue *pQueue;

	pQueue = (Queue *)m_pBreakStack->peek();
	if (pQueue != NULL)
	{
		pQueue->Put(CAST_TO_POINTER(dwAddr, void *));
	}
}


//
// Resolve all breal statements at current level
//

void NXSL_Compiler::closeBreakLevel(NXSL_Program *pScript)
{
	Queue *pQueue;
	void *pAddr;
	DWORD dwAddr;

	pQueue = (Queue *)m_pBreakStack->pop();
	if (pQueue != NULL)
	{
		while((pAddr = pQueue->Get()) != NULL)
		{
			dwAddr = CAST_FROM_POINTER(pAddr, DWORD);
			pScript->createJumpAt(dwAddr, pScript->getCodeSize());
		}
		delete pQueue;
	}
}
