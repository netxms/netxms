/* 
** NetXMS - Network Management System
** NetXMS Scripting Language Interpreter
** Copyright (C) 2003-2026 Victor Kirhenshtein
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
NXSL_Lexer::NXSL_Lexer(NXSL_Compiler *compiler, const char *sourceCode)
{
	m_sourceCode = MemCopyStringA(sourceCode);
   m_sourceSize = strlen(m_sourceCode);
   m_converterMode = false;
   m_currLine = 1;
   m_sourcePos = 0;
   m_compiler = compiler;
   m_commentLevel = 0;
   m_stringSize = 0;
   m_lastToken = 0;
   m_lastColonIsLabel = false;
   NXSL_BracketContext *root = m_brackets.addPlaceholder();
   root->openingToken = 0;
   root->pendingTernary = 0;
}

/**
 * Check if opening brace at current position starts a block (as opposed to
 * hash map initializer). Decision is based on previously returned token: brace
 * opens a block only where the grammar cannot expect an operand.
 */
bool NXSL_Lexer::isBlockBraceExpected() const
{
   switch(m_lastToken)
   {
      case 0:     // start of script
      case ';':
      case '{':
      case '}':
      case ')':   // if/while/for/foreach/switch/function header, select options
      case T_ELSE:
      case T_DO:
      case T_TRY:
      case T_CATCH:
      case T_IDENTIFIER:   // select statement without options
         return true;
      case ':':   // case/when/default label, unless ternary or hash map value
         return m_lastColonIsLabel;
      default:
         return false;
   }
}

/**
 * Post-process token returned by scanner: track bracket nesting and rewrite
 * hash map initializer brace into T_LBRACE_MAP. Returns token to be passed to parser.
 */
int NXSL_Lexer::processToken(int token)
{
   NXSL_BracketContext *context = m_brackets.get(m_brackets.size() - 1);
   switch(token)
   {
      case '{':
         if (m_lastToken == '%')   // deprecated %{ } form, parsed as separate '%' and '{' tokens
         {
            context = m_brackets.addPlaceholder();
            context->openingToken = T_LBRACE_MAP;
         }
         else
         {
            if (!isBlockBraceExpected())
               token = T_LBRACE_MAP;
            context = m_brackets.addPlaceholder();
            context->openingToken = token;
         }
         context->pendingTernary = 0;
         break;
      case '(':
      case '[':
         context = m_brackets.addPlaceholder();
         context->openingToken = token;
         context->pendingTernary = 0;
         break;
      case ')':
      case ']':
      case '}':
         if (m_brackets.size() > 1)
            m_brackets.remove(m_brackets.size() - 1);
         break;
      case '?':
         context->pendingTernary++;
         break;
      case ':':
         if (context->pendingTernary > 0)
         {
            context->pendingTernary--;
            m_lastColonIsLabel = false;
         }
         else
         {
            m_lastColonIsLabel = (context->openingToken == '{') || (context->openingToken == 0);
         }
         break;
      default:
         break;
   }
   m_lastToken = token;
   return token;
}

/**
 * Alternative input method
 */
size_t NXSL_Lexer::lexerInput(char *buffer, size_t maxSize)
{
   size_t bytes;
   if (m_sourcePos < m_sourceSize)
   {
      bytes = m_converterMode ? 1 : std::min(maxSize, m_sourceSize - m_sourcePos);
      memcpy(buffer, &m_sourceCode[m_sourcePos], bytes);
      m_sourcePos += bytes;
   }
   else
   {
      bytes = 0;   // EOF
   }
   return bytes;
}

/**
 * Report error
 */
void NXSL_Lexer::error(const char *message)
{
	m_compiler->error(message);
}
