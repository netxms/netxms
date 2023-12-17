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
** File: lexer.cpp
**
**/

#include "libnxsl.h"
#include "parser.tab.hpp"

/**
 * Constructor for our lexer class
 */
NXSL_Lexer::NXSL_Lexer(NXSL_Compiler *compiler, const TCHAR *sourceCode)
{
#ifdef UNICODE
	m_sourceCode = UTF8StringFromWideString(sourceCode);
#else
	m_sourceCode = UTF8StringFromMBString(sourceCode);
#endif
   m_sourceSize = strlen(m_sourceCode);
   m_converterMode = false;
   m_currLine = 1;
   m_sourcePos = 0;
   m_compiler = compiler;
   m_commentLevel = 0;
   m_stringSize = 0;
}

/**
 * Destructor
 */
NXSL_Lexer::~NXSL_Lexer()
{
   MemFree(m_sourceCode);
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
