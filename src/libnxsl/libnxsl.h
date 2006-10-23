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
#include <nms_util.h>
#include <nxcpapi.h>
#include <nxsl.h>
#include <FlexLexer.h>

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif
#ifndef max
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif

union YYSTYPE;


//
// Various defines
//

#define INVALID_ADDRESS    ((DWORD)0xFFFFFFFF)
#define MAX_STRING_SIZE    8192


//
// Instruction opcodes
//

#define OPCODE_NOP            0
#define OPCODE_RETURN         1
#define OPCODE_JMP            2
#define OPCODE_CALL           3
#define OPCODE_CALL_EXTERNAL  4
#define OPCODE_PUSH_CONSTANT  5
#define OPCODE_PUSH_VARIABLE  6
#define OPCODE_EXIT           7
#define OPCODE_POP            8
#define OPCODE_SET            9
#define OPCODE_ADD            10
#define OPCODE_SUB            11
#define OPCODE_MUL            12
#define OPCODE_DIV            13
#define OPCODE_REM            14
#define OPCODE_EQ             15
#define OPCODE_NE             16
#define OPCODE_LT             17
#define OPCODE_LE             18
#define OPCODE_GT             19
#define OPCODE_GE             20
#define OPCODE_BIT_AND        21
#define OPCODE_BIT_OR         22
#define OPCODE_BIT_XOR        23
#define OPCODE_AND            24
#define OPCODE_OR             25
#define OPCODE_LSHIFT         26
#define OPCODE_RSHIFT         27
#define OPCODE_RET_NULL       28
#define OPCODE_JZ             29
#define OPCODE_PRINT          30
#define OPCODE_CONCAT         31
#define OPCODE_BIND           32
#define OPCODE_INC            33
#define OPCODE_DEC            34
#define OPCODE_NEG            35
#define OPCODE_NOT            36
#define OPCODE_BIT_NOT        37
#define OPCODE_CAST           38
#define OPCODE_REFERENCE      39
#define OPCODE_INCP           40
#define OPCODE_DECP           41
#define OPCODE_JNZ            42
#define OPCODE_LIKE           43
#define OPCODE_ILIKE          44
#define OPCODE_MATCH          45
#define OPCODE_IMATCH         46


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
   int m_nStrSize;
   char m_szStr[MAX_STRING_SIZE];
   YYSTYPE *m_plval;
   BOOL m_bErrorState;

	virtual int LexerInput(char *pBuffer, int nMaxSize);
   virtual void LexerError(const char *pszMsg);

public:
	NXSL_Lexer(NXSL_Compiler *pCompiler, TCHAR *pszCode);
	virtual ~NXSL_Lexer();

   virtual int yylex(void);
   void SetLvalPtr(YYSTYPE *plval) { m_plval = plval; }

   int GetCurrLine(void) { return m_nCurrLine; }

   void SetErrorState(void) { m_bErrorState = TRUE; }
   BOOL IsErrorState(void) { return m_bErrorState; }
};


//
// Compiler class
//

class NXSL_Compiler
{
protected:
   TCHAR *m_pszErrorText;
   NXSL_Lexer *m_pLexer;
   NXSL_Stack *m_pAddrStack;

public:
   NXSL_Compiler(void);
   ~NXSL_Compiler();

   NXSL_Program *Compile(TCHAR *pszSourceCode);
   void Error(const char *pszMsg);

   TCHAR *GetErrorText(void) { return CHECK_NULL(m_pszErrorText); }

   void PushAddr(DWORD dwAddr) { m_pAddrStack->Push(CAST_TO_POINTER(dwAddr, void *)); }
   DWORD PopAddr(void);
   DWORD PeekAddr(void);
};


//
// Global variables
//

extern char *g_szTypeNames[];


#endif
