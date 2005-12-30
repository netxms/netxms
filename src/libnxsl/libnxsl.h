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

union YYSTYPE;


//
// Various defines
//

#define INVALID_ADDRESS    ((DWORD)0xFFFFFFFF)
#define MAX_STRING_SIZE    8192
#define MAX_FUNCTION_NAME  64


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


//
// Value flags
//

#define VALUE_IS_NULL         ((DWORD)0x0001)
#define VALUE_STRING_IS_VALID ((DWORD)0x0002)
#define VALUE_IS_NUMERIC      ((DWORD)0x0004)
#define VALUE_IS_REAL         ((DWORD)0x0008)


//
// Function structure
//

struct NXSL_Function
{
   char m_szName[MAX_FUNCTION_NAME];
   DWORD m_dwAddr;
};


//
// External function structure
//

class NXSL_Value;

struct NXSL_ExtFunction
{
   char m_szName[MAX_FUNCTION_NAME];
   int (* m_pfHandler)(int argc, NXSL_Value **argv, NXSL_Value **ppResult);
   int m_iNumArgs;   // Number of arguments or -1 fo variable number
};


//
// Environment for NXSL program
//

class NXSL_Environment
{
private:
   DWORD m_dwNumFunctions;
   NXSL_ExtFunction *m_pFunctionList;

   FILE *m_pStdIn;
   FILE *m_pStdOut;

public:
   NXSL_Environment();
   ~NXSL_Environment();

   void SetIO(FILE *pIn, FILE *pOut) { m_pStdIn = pIn; m_pStdOut = pOut; }
   FILE *GetStdIn(void) { return m_pStdIn; }
   FILE *GetStdOut(void) { return m_pStdOut; }

   NXSL_ExtFunction *FindFunction(char *pszName);
};


//
// Simple stack class
//

class NXSL_Stack
{
private:
   int m_nStackSize;
   int m_nStackPos;
   void **m_ppData;

public:
   NXSL_Stack(void);
   ~NXSL_Stack();

   void Push(void *pData);
   void *Pop(void);
   void *Peek(void);
   void **PeekList(int nLevel) { return &m_ppData[m_nStackPos - nLevel]; }

   int Size(void) { return m_nStackPos; }
};


//
// Variable or constant value
//

class NXSL_Value
{
protected:
   DWORD m_dwFlags;
   DWORD m_dwStrLen;
   char *m_pszValStr;
   int m_iValInt;
   double m_dValFloat;

   void UpdateNumbers(void);
   void UpdateString(void);

public:
   NXSL_Value(void);
   NXSL_Value(NXSL_Value *);
   NXSL_Value(int nValue);
   NXSL_Value(double dValue);
   NXSL_Value(char *pszValue);
   ~NXSL_Value();

   BOOL IsNull(void) { return (m_dwFlags & VALUE_IS_NULL) ? TRUE : FALSE; }
   BOOL IsNumeric(void) { return (m_dwFlags & VALUE_IS_NUMERIC) ? TRUE : FALSE; }
   BOOL IsReal(void) { return (m_dwFlags & VALUE_IS_REAL) ? TRUE : FALSE; }
   char *GetValueAsString(DWORD *pdwLen);
   char *GetValueAsCString(void);
   int GetValueAsInt(void);
   double GetValueAsReal(void);

   void Concatenate(char *pszString, DWORD dwLen);
   void Increment(void);
   void Decrement(void);
   void Negate(void);
   void Set(int nValue);
};


//
// Runtime variable information
//

class NXSL_Variable
{
protected:
   TCHAR *m_pszName;
   NXSL_Value *m_pValue;

public:
   NXSL_Variable(TCHAR *pszName);
   NXSL_Variable(TCHAR *pszName, NXSL_Value *pValue);
   NXSL_Variable(NXSL_Variable *pSrc);
   ~NXSL_Variable();

   TCHAR *Name(void) { return m_pszName; }
   NXSL_Value *Value(void) { return m_pValue; }
   void Set(NXSL_Value *pValue);
};


//
// Varable system
//

class NXSL_VariableSystem
{
protected:
   DWORD m_dwNumVariables;
   NXSL_Variable **m_ppVariableList;

public:
   NXSL_VariableSystem(void);
   NXSL_VariableSystem(NXSL_VariableSystem *pSrc);
   ~NXSL_VariableSystem();

   NXSL_Variable *Find(TCHAR *pszName);
   NXSL_Variable *Create(TCHAR *pszName, NXSL_Value *pValue = NULL);
};


//
// Single execution instruction
//

class NXSL_Instruction
{
   friend class NXSL_Program;

protected:
   int m_nOpCode;
   union
   {
      NXSL_Value *m_pConstant;
      char *m_pszString;
      DWORD m_dwAddr;
   } m_operand;
   int m_nStackItems;
   int m_nSourceLine;

public:
   NXSL_Instruction(int nLine, int nOpCode);
   NXSL_Instruction(int nLine, int nOpCode, NXSL_Value *pValue);
   NXSL_Instruction(int nLine, int nOpCode, char *pszString);
   NXSL_Instruction(int nLine, int nOpCode, char *pszString, int nStackItems);
   NXSL_Instruction(int nLine, int nOpCode, DWORD dwAddr);
   NXSL_Instruction(int nLine, int nOpCode, int nStackItems);
   ~NXSL_Instruction();
};


//
// Class representing compiled NXSL program
//

class NXSL_Program
{
protected:
   NXSL_Environment *m_pEnv;

   NXSL_Instruction **m_ppInstructionSet;
   DWORD m_dwCodeSize;
   DWORD m_dwCurrPos;

   DWORD m_dwSubLevel;
   NXSL_Stack *m_pDataStack;
   NXSL_Stack *m_pCodeStack;
   int m_nBindPos;

   NXSL_VariableSystem *m_pConstants;
   NXSL_VariableSystem *m_pGlobals;
   NXSL_VariableSystem *m_pLocals;

   DWORD m_dwNumFunctions;
   NXSL_Function *m_pFunctionList;

   int m_nErrorCode;
   TCHAR *m_pszErrorText;

   void Execute(void);
   void DoUnaryOperation(int nOpCode);
   void DoBinaryOperation(int nOpCode);
   void Error(int nError);

   NXSL_Variable *FindOrCreateVariable(TCHAR *pszName);

public:
   NXSL_Program(void);
   ~NXSL_Program();

   BOOL AddFunction(char *pszName, DWORD dwAddr, char *pszError);
   void ResolveFunctions(void);
   void AddInstruction(NXSL_Instruction *pInstruction);
   void ResolveLastJump(int nOpCode);

   int Run(NXSL_Environment *pEnv);

   void Dump(FILE *pFile);
   TCHAR *GetErrorText(void) { return m_pszErrorText; }
};


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

public:
   NXSL_Compiler(void);
   ~NXSL_Compiler();

   NXSL_Program *Compile(TCHAR *pszSourceCode);
   void Error(const char *pszMsg);

   TCHAR *GetErrorText(void) { return CHECK_NULL(m_pszErrorText); }
};


#endif
