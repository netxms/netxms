/* 
** NetXMS - Network Management System
** NetXMS Scripting Language Interpreter
** Copyright (C) 2003-2011 Victor Kirhenshtein
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
** File: libnxsl.h
**
**/

#ifndef _libnxsl_h_
#define _libnxsl_h_

#include <nms_common.h>
#include <nms_util.h>
#include <nxcpapi.h>
#include <nxsl.h>
#include <nxqueue.h>

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif
#ifndef max
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif

union YYSTYPE;
typedef void *yyscan_t;


//
// Various defines
//

#define MAX_STRING_SIZE    8192

/**
 * Instruction opcodes
 */
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
#define OPCODE_GET_ATTRIBUTE  39
#define OPCODE_INCP           40
#define OPCODE_DECP           41
#define OPCODE_JNZ            42
#define OPCODE_LIKE           43
#define OPCODE_ILIKE          44
#define OPCODE_MATCH          45
#define OPCODE_IMATCH         46
#define OPCODE_CASE           47
#define OPCODE_ARRAY          48
#define OPCODE_GET_ELEMENT    49
#define OPCODE_SET_ELEMENT    50
#define OPCODE_SET_ATTRIBUTE  51
#define OPCODE_NAME           52
#define OPCODE_FOREACH        53
#define OPCODE_NEXT           54
#define OPCODE_GLOBAL         55
#define OPCODE_GLOBAL_ARRAY   56
#define OPCODE_JZ_PEEK        57
#define OPCODE_JNZ_PEEK       58
#define OPCODE_ADD_TO_ARRAY   59
#define OPCODE_SAFE_GET_ATTR  60
#define OPCODE_CALL_METHOD    61
#define OPCODE_CASE_CONST     62
#define OPCODE_INC_ELEMENT    63
#define OPCODE_DEC_ELEMENT    64
#define OPCODE_INCP_ELEMENT   65
#define OPCODE_DECP_ELEMENT   66
#define OPCODE_ABORT          67
#define OPCODE_CATCH          68
#define OPCODE_PUSH_CONSTREF  69
#define OPCODE_HASHMAP_SET    70

class NXSL_Compiler;

/**
 * Modified lexer class
 */
class NXSL_Lexer
{
	friend int yylex(YYSTYPE *, yyscan_t);

protected:
   int m_nSourceSize;
   int m_nSourcePos;
   char *m_pszSourceCode;
   NXSL_Compiler *m_pCompiler;

   int m_nCurrLine;
   int m_nCommentLevel;
   int m_nStrSize;
   char m_szStr[MAX_STRING_SIZE];

public:
	NXSL_Lexer(NXSL_Compiler *pCompiler, const TCHAR *pszCode);
	virtual ~NXSL_Lexer();

	int lexerInput(char *pBuffer, int nMaxSize);

	int getCurrLine() { return m_nCurrLine; }
	void error(const char *pszText);
};

/**
 * Compiler class
 */
class NXSL_Compiler
{
protected:
   TCHAR *m_errorText;
   int m_errorLineNumber;
   NXSL_Lexer *m_lexer;
   NXSL_Stack *m_addrStack;
	NXSL_Stack *m_breakStack;
	int m_idOpCode;

public:
   NXSL_Compiler();
   ~NXSL_Compiler();

   NXSL_Program *compile(const TCHAR *pszSourceCode);
   void error(const char *pszMsg);

   const TCHAR *getErrorText() { return CHECK_NULL(m_errorText); }
   int getErrorLineNumber() { return m_errorLineNumber; }

   void pushAddr(UINT32 dwAddr) { m_addrStack->push(CAST_TO_POINTER(dwAddr, void *)); }
   UINT32 popAddr();
   UINT32 peekAddr();

	void addBreakAddr(UINT32 dwAddr);
	void closeBreakLevel(NXSL_Program *pScript);
	BOOL canUseBreak() { return m_breakStack->getSize() > 0; }
	void newBreakLevel() { m_breakStack->push(new Queue); }

	void setIdentifierOperation(int opcode) { m_idOpCode = opcode; }
	int getIdentifierOperation() { return m_idOpCode; }
};


//
// Global variables
//

extern const TCHAR *g_szTypeNames[];


#endif
