/* 
** NetXMS - Network Management System
** Copyright (C) 2005, 2006 Victor Kirhenshtein
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
** $module: nxsl_classes.h
**
**/

#ifndef _nxsl_classes_h_
#define _nxsl_classes_h_


//
// Data types
//

#define NXSL_DT_NULL       0
#define NXSL_DT_STRING     1
#define NXSL_DT_REAL       2
#define NXSL_DT_INT32      3
#define NXSL_DT_INT64      4
#define NXSL_DT_UINT32     5
#define NXSL_DT_UINT64     6


//
// Simple stack class
//

class LIBNXSL_EXPORTABLE NXSL_Stack
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

class LIBNXSL_EXPORTABLE NXSL_Value
{
protected:
   DWORD m_dwStrLen;
   char *m_pszValStr;
   BYTE m_nDataType;
   BYTE m_bStringIsValid;
   union
   {
      LONG nInt32;
      DWORD uInt32;
      INT64 nInt64;
      QWORD uInt64;
      double dReal;
   } m_number;

   void UpdateNumber(void);
   void UpdateString(void);

public:
   NXSL_Value(void);
   NXSL_Value(NXSL_Value *);
   NXSL_Value(LONG nValue);
   NXSL_Value(INT64 nValue);
   NXSL_Value(DWORD uValue);
   NXSL_Value(QWORD uValue);
   NXSL_Value(double dValue);
   NXSL_Value(char *pszValue);
   ~NXSL_Value();

   void Set(LONG nValue);

   BOOL Convert(int nDataType);
   int DataType(void) { return m_nDataType; }

   BOOL IsNull(void) { return (m_nDataType == NXSL_DT_NULL); }
   BOOL IsNumeric(void) { return (m_nDataType > NXSL_DT_STRING); }
   BOOL IsReal(void) { return (m_nDataType == NXSL_DT_REAL); }
   BOOL IsInteger(void) { return (m_nDataType > NXSL_DT_REAL); }
   BOOL IsUnsigned(void) { return (m_nDataType >= NXSL_DT_UINT32); }
   BOOL IsZero(void);
   BOOL IsNonZero(void);

   char *GetValueAsString(DWORD *pdwLen);
   char *GetValueAsCString(void);
   LONG GetValueAsInt32(void);
   DWORD GetValueAsUInt32(void);
   INT64 GetValueAsInt64(void);
   QWORD GetValueAsUInt64(void);
   double GetValueAsReal(void);

   void Concatenate(char *pszString, DWORD dwLen);
   
   void Increment(void);
   void Decrement(void);
   void Negate(void);
   void BitNot(void);

   void Add(NXSL_Value *pVal);
   void Sub(NXSL_Value *pVal);
   void Mul(NXSL_Value *pVal);
   void Div(NXSL_Value *pVal);
   void Rem(NXSL_Value *pVal);
   void BitAnd(NXSL_Value *pVal);
   void BitOr(NXSL_Value *pVal);
   void BitXor(NXSL_Value *pVal);
   void LShift(int nBits);
   void RShift(int nBits);

   BOOL EQ(NXSL_Value *pVal);
   BOOL LT(NXSL_Value *pVal);
   BOOL LE(NXSL_Value *pVal);
   BOOL GT(NXSL_Value *pVal);
   BOOL GE(NXSL_Value *pVal);
};


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

struct NXSL_ExtFunction
{
   char m_szName[MAX_FUNCTION_NAME];
   int (* m_pfHandler)(int argc, NXSL_Value **argv, NXSL_Value **ppResult);
   int m_iNumArgs;   // Number of arguments or -1 fo variable number
};


//
// Environment for NXSL program
//

class LIBNXSL_EXPORTABLE NXSL_Environment
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
// Runtime variable information
//

class LIBNXSL_EXPORTABLE NXSL_Variable
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

class LIBNXSL_EXPORTABLE NXSL_VariableSystem
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

class LIBNXSL_EXPORTABLE NXSL_Instruction
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

class LIBNXSL_EXPORTABLE NXSL_Program
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


#endif
