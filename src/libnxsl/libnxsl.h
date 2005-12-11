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
** $module: libnxsl.h
**
**/

#ifndef _libnxsl_h_
#define _libnxsl_h_

#include <nms_common.h>
#include <nms_threads.h>
#include <nms_util.h>
#include <nxsl.h>
#include <FlexLexer.h>


//
// Modified lexer class
//

class NXSL_Compiler;

class NXSL_Lexer : public yyFlexLexer
{
protected:
   int m_nSourceSize;
   int m_nSourcePos;
   char *m_pszSourceCode;
   NXSL_Compiler *m_pCompiler;

   int m_nCurrLine;
   int m_nCommentLevel;

	virtual int LexerInput(char *pBuffer, int nMaxSize);
   virtual void LexerError(const char *pszMsg);

public:
	NXSL_Lexer(NXSL_Compiler *pCompiler, TCHAR *pszCode);
	virtual ~NXSL_Lexer();

   virtual int yylex(void);

   int GetCurrLine(void) { return m_nCurrLine; }
};


//
// Compiler class
//

class NXSL_Compiler
{
protected:
   TCHAR *m_pszErrorText;
   NXSL_Lexer *m_pLexer;

public:
   NXSL_Compiler(void);
   ~NXSL_Compiler();

   NXSL_Program *Compile(TCHAR *pszSourceCode);
   void Error(const char *pszMsg);

   TCHAR *GetErrorText(void) { return CHECK_NULL(m_pszErrorText); }
};

#endif
