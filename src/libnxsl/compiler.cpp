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
** File: compiler.cpp
**
**/

#include "libnxsl.h"

/**
 * Externals
 */
int yyparse(yyscan_t scanner, NXSL_Lexer *, NXSL_Compiler *, NXSL_Program *);
extern int yydebug;
void yyset_extra(NXSL_Lexer *, yyscan_t);
int yylex_init(yyscan_t *);
int yylex_destroy(yyscan_t);

/**
 * Constructor
 */
NXSL_Compiler::NXSL_Compiler()
{
   m_errorText = NULL;
   m_errorLineNumber = 0;
   m_lexer = NULL;
   m_addrStack = new NXSL_Stack;
	m_breakStack = new NXSL_Stack;
	m_idOpCode = 0;
}

/**
 * Destructor
 */
NXSL_Compiler::~NXSL_Compiler()
{
   safe_free(m_errorText);
   delete m_lexer;
   delete m_addrStack;
	delete m_breakStack;
}

/**
 * Error handler
 */
void NXSL_Compiler::error(const char *pszMsg)
{
   char szText[1024];

   if (m_errorText == NULL)
   {
      m_errorLineNumber = m_lexer->getCurrLine();
      snprintf(szText, 1024, "Error in line %d: %s", m_errorLineNumber, pszMsg);
#ifdef UNICODE
		m_errorText = WideStringFromMBString(szText);
#else
      m_errorText = strdup(szText);
#endif
   }
}

/**
 * Compile source code
 */
NXSL_Program *NXSL_Compiler::compile(const TCHAR *pszSourceCode)
{
   NXSL_Program *pResult;
	yyscan_t scanner;

   m_lexer = new NXSL_Lexer(this, pszSourceCode);
   pResult = new NXSL_Program;
	yylex_init(&scanner);
	yyset_extra(m_lexer, scanner);
   if (yyparse(scanner, m_lexer, this, pResult) == 0)
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

/**
 * yyerror() for parser
 */
void yyerror(yyscan_t scanner, NXSL_Lexer *pLexer, NXSL_Compiler *pCompiler,
             NXSL_Program *pScript, const char *pszText)
{
   pCompiler->error(pszText);
}

/**
 * Pop address
 */
UINT32 NXSL_Compiler::popAddr()
{
   void *pAddr;

   pAddr = m_addrStack->pop();
   return pAddr ? CAST_FROM_POINTER(pAddr, UINT32) : INVALID_ADDRESS;
}

/**
 * Peek address
 */
UINT32 NXSL_Compiler::peekAddr()
{
   void *pAddr;

   pAddr = m_addrStack->peek();
   return pAddr ? CAST_FROM_POINTER(pAddr, UINT32) : INVALID_ADDRESS;
}

/**
 * Add "break" statement address
 */
void NXSL_Compiler::addBreakAddr(UINT32 dwAddr)
{
	Queue *pQueue;

	pQueue = (Queue *)m_breakStack->peek();
	if (pQueue != NULL)
	{
		pQueue->put(CAST_TO_POINTER(dwAddr, void *));
	}
}

/**
 * Resolve all breal statements at current level
 */
void NXSL_Compiler::closeBreakLevel(NXSL_Program *pScript)
{
	Queue *pQueue;
	void *pAddr;
	UINT32 dwAddr;

	pQueue = (Queue *)m_breakStack->pop();
	if (pQueue != NULL)
	{
		while((pAddr = pQueue->get()) != NULL)
		{
			dwAddr = CAST_FROM_POINTER(pAddr, UINT32);
			pScript->createJumpAt(dwAddr, pScript->getCodeSize());
		}
		delete pQueue;
	}
}
