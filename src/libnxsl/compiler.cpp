/* 
** NetXMS - Network Management System
** NetXMS Scripting Language Interpreter
** Copyright (C) 2003-2018 Victor Kirhenshtein
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
   m_selectStack = new NXSL_Stack;
	m_idOpCode = 0;
}

/**
 * Destructor
 */
NXSL_Compiler::~NXSL_Compiler()
{
   free(m_errorText);
   delete m_lexer;
   delete m_addrStack;

   Queue *q;
   while((q = static_cast<Queue*>(m_breakStack->pop())) != NULL)
      delete q;
   delete m_breakStack;

   while((q = static_cast<Queue*>(m_selectStack->pop())) != NULL)
      delete q;
   delete m_selectStack;
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
   void *addr = m_addrStack->pop();
   return addr ? CAST_FROM_POINTER(addr, UINT32) : INVALID_ADDRESS;
}

/**
 * Peek address
 */
UINT32 NXSL_Compiler::peekAddr()
{
   void *addr = m_addrStack->peek();
   return addr ? CAST_FROM_POINTER(addr, UINT32) : INVALID_ADDRESS;
}

/**
 * Add "break" statement address
 */
void NXSL_Compiler::addBreakAddr(UINT32 dwAddr)
{
	Queue *queue = static_cast<Queue*>(m_breakStack->peek());
	if (queue != NULL)
	{
		queue->put(CAST_TO_POINTER(dwAddr, void *));
	}
}

/**
 * Resolve all breal statements at current level
 */
void NXSL_Compiler::closeBreakLevel(NXSL_Program *pScript)
{
   Queue *queue = static_cast<Queue*>(m_breakStack->pop());
	if (queue != NULL)
	{
	   void *addr;
		while((addr = queue->get()) != NULL)
		{
			UINT32 nxslAddr = CAST_FROM_POINTER(addr, UINT32);
			pScript->createJumpAt(nxslAddr, pScript->getCodeSize());
		}
		delete queue;
	}
}

/**
 * Close select level
 */
void NXSL_Compiler::closeSelectLevel()
{
   delete (Queue *)m_selectStack->pop();
}

/**
 * Push jump address for select
 */
void NXSL_Compiler::pushSelectJumpAddr(UINT32 addr)
{
   Queue *q = (Queue *)m_selectStack->peek();
   if (q != NULL)
   {
      q->put(CAST_TO_POINTER(addr, void *));
   }
}

/**
 * Pop jump address for select
 */
UINT32 NXSL_Compiler::popSelectJumpAddr()
{
   Queue *q = (Queue *)m_selectStack->peek();
   if ((q == NULL) || (q->size() == 0))
      return INVALID_ADDRESS;

   return CAST_FROM_POINTER(q->get(), UINT32);
}
