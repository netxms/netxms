/* 
** NetXMS - Network Management System
** NetXMS Scripting Language Interpreter
** Copyright (C) 2003-2022 Victor Kirhenshtein
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
int yyparse(yyscan_t scanner, NXSL_Lexer*, NXSL_Compiler*, NXSL_ProgramBuilder*);
void yyset_extra(NXSL_Lexer *, yyscan_t);
int yylex_init(yyscan_t *);
int yylex_destroy(yyscan_t);

/**
 * Constructor
 */
NXSL_Compiler::NXSL_Compiler()
{
   m_errorLineNumber = 0;
   m_warnings = nullptr;
   m_lexer = nullptr;
   m_addrStack = new NXSL_Stack;
	m_breakStack = new NXSL_Stack;
   m_selectStack = new NXSL_Stack;
	m_idOpCode = 0;
	m_temporaryStackItems = 0;
}

/**
 * Destructor
 */
NXSL_Compiler::~NXSL_Compiler()
{
   delete m_lexer;
   delete m_addrStack;

   Queue *q;
   while((q = static_cast<Queue*>(m_breakStack->pop())) != nullptr)
      delete q;
   delete m_breakStack;

   while((q = static_cast<Queue*>(m_selectStack->pop())) != nullptr)
      delete q;
   delete m_selectStack;
}

/**
 * Error handler
 */
void NXSL_Compiler::error(const char *message)
{
   if (m_errorText.isEmpty())
   {
      m_errorLineNumber = m_lexer->getCurrLine();
      m_errorText.append(_T("Error in line "));
      m_errorText.append(m_errorLineNumber);
      m_errorText.append(_T(": "));
#ifdef UNICODE
      m_errorText.appendMBString(message);
#else
      m_errorText.append(message);
#endif
   }
}

/**
 * Add compilation warning
 */
void NXSL_Compiler::warning(const TCHAR *format, ...)
{
   TCHAR text[4096];
   va_list(args);
   va_start(args, format);
   _vsntprintf(text, 4096, format, args);
   va_end(args);
   m_warnings->add(new NXSL_CompilationWarning(m_lexer->getCurrLine(), text));
}

/**
 * Compile source code
 */
NXSL_Program *NXSL_Compiler::compile(const TCHAR *sourceCode, NXSL_Environment *env, ObjectArray<NXSL_CompilationWarning> *warnings)
{
   m_warnings = warnings;
   m_lexer = new NXSL_Lexer(this, sourceCode);

	yyscan_t scanner;
	yylex_init(&scanner);
	yyset_extra(m_lexer, scanner);

	NXSL_Program *code = nullptr;
   NXSL_ProgramBuilder builder(env);
   if (yyparse(scanner, m_lexer, this, &builder) == 0)
   {
      builder.resolveFunctions();
		builder.optimize();
		code = new NXSL_Program(&builder);
   }
	yylex_destroy(scanner);
   return code;
}

/**
 * yyerror() for parser
 */
void yyerror(yyscan_t scanner, NXSL_Lexer *lexer, NXSL_Compiler *compiler, NXSL_ProgramBuilder *builder, const char *text)
{
   compiler->error(text);
}

/**
 * Pop address
 */
uint32_t NXSL_Compiler::popAddr()
{
   void *addr = m_addrStack->pop();
   return addr ? CAST_FROM_POINTER(addr, uint32_t) : INVALID_ADDRESS;
}

/**
 * Peek address
 */
uint32_t NXSL_Compiler::peekAddr()
{
   void *addr = m_addrStack->peek();
   return (addr != nullptr) ? CAST_FROM_POINTER(addr, uint32_t) : INVALID_ADDRESS;
}

/**
 * Add "break" statement address
 */
void NXSL_Compiler::addBreakAddr(uint32_t addr)
{
	Queue *queue = static_cast<Queue*>(m_breakStack->peek());
	if (queue != nullptr)
	{
		queue->put(CAST_TO_POINTER(addr, void *));
	}
}

/**
 * Resolve all breal statements at current level
 */
void NXSL_Compiler::closeBreakLevel(NXSL_ProgramBuilder *pScript)
{
   Queue *queue = static_cast<Queue*>(m_breakStack->pop());
	if (queue != nullptr)
	{
	   void *addr;
		while((addr = queue->get()) != nullptr)
		{
		   uint32_t nxslAddr = CAST_FROM_POINTER(addr, uint32_t);
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
   delete static_cast<Queue*>(m_selectStack->pop());
}

/**
 * Push jump address for select
 */
void NXSL_Compiler::pushSelectJumpAddr(uint32_t addr)
{
   Queue *q = static_cast<Queue*>(m_selectStack->peek());
   if (q != nullptr)
   {
      q->put(CAST_TO_POINTER(addr, void *));
   }
}

/**
 * Pop jump address for select
 */
uint32_t NXSL_Compiler::popSelectJumpAddr()
{
   Queue *q = static_cast<Queue*>(m_selectStack->peek());
   if ((q == nullptr) || (q->size() == 0))
      return INVALID_ADDRESS;

   return CAST_FROM_POINTER(q->get(), UINT32);
}
