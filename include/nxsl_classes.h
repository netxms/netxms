/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2010 Victor Kirhenshtein
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
** File: nxsl_classes.h
**
**/

#ifndef _nxsl_classes_h_
#define _nxsl_classes_h_

#include <nms_threads.h>
#include <nms_util.h>
#include <nxcpapi.h>


//
// Constants
//

#define MAX_CLASS_NAME     64


//
// Data types
//

#define NXSL_DT_NULL       0
#define NXSL_DT_OBJECT     1
#define NXSL_DT_ARRAY      2
#define NXSL_DT_ITERATOR   3
#define NXSL_DT_STRING     4
#define NXSL_DT_REAL       5
#define NXSL_DT_INT32      6
#define NXSL_DT_INT64      7
#define NXSL_DT_UINT32     8
#define NXSL_DT_UINT64     9


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
   NXSL_Stack();
   ~NXSL_Stack();

   void push(void *pData);
   void *pop();
   void *peek();
   void **peekList(int nLevel) { return &m_ppData[m_nStackPos - nLevel]; }

   int getSize() { return m_nStackPos; }
};


//
// Class representing NXSL class
//

class NXSL_Value;
class NXSL_Object;

class LIBNXSL_EXPORTABLE NXSL_Class
{
protected:
   TCHAR m_szName[MAX_CLASS_NAME];

public:
   NXSL_Class();
   virtual ~NXSL_Class();

   virtual NXSL_Value *getAttr(NXSL_Object *pObject, const TCHAR *pszAttr);
   virtual BOOL setAttr(NXSL_Object *pObject, const TCHAR *pszAttr, NXSL_Value *pValue);

	virtual void onObjectDelete(NXSL_Object *object);

   const TCHAR *getName() { return m_szName; }
};


//
// Object instance
//

struct __nxsl_class_data
{
	void *data;
	int refCount;
};

class LIBNXSL_EXPORTABLE NXSL_Object
{
private:
   NXSL_Class *m_class;
   __nxsl_class_data *m_data;

public:
   NXSL_Object(NXSL_Object *pObject);
   NXSL_Object(NXSL_Class *pClass, void *pData);
   ~NXSL_Object();

   NXSL_Class *getClass() { return m_class; }
	void *getData() { return m_data->data; }
};


//
// Array
//

struct NXSL_ArrayElement
{
	int index;
	NXSL_Value *value;
};

class LIBNXSL_EXPORTABLE NXSL_Array
{
private:
	int m_refCount;
	int m_size;
	int m_allocated;
	NXSL_ArrayElement *m_data;

public:
	NXSL_Array();
	NXSL_Array(NXSL_Array *src);
	~NXSL_Array();

	void incRefCount() { m_refCount++; }
	void decRefCount() { m_refCount--; }
	BOOL isUnused() { return m_refCount < 1; }

	void set(int index, NXSL_Value *value);
	NXSL_Value *get(int index);
	NXSL_Value *getByPosition(int position);

	int size() { return m_size; }
};


//
// Iterator for arrays
//

class LIBNXSL_EXPORTABLE NXSL_Iterator
{
private:
	int m_refCount;
	TCHAR *m_variable;
	NXSL_Array *m_array;
	int m_position;

public:
	NXSL_Iterator(const TCHAR *variable, NXSL_Array *array);
	~NXSL_Iterator();

	const TCHAR *getVariableName() { return m_variable; }

	void incRefCount() { m_refCount++; }
	void decRefCount() { m_refCount--; }
	BOOL isUnused() { return m_refCount < 1; }

	NXSL_Value *next();

	static int createIterator(NXSL_Stack *stack);
};


//
// Variable or constant value
//

class LIBNXSL_EXPORTABLE NXSL_Value
{
protected:
   DWORD m_dwStrLen;
   TCHAR *m_pszValStr;
	TCHAR *m_name;
   BYTE m_nDataType;
   BYTE m_bStringIsValid;
   union
   {
      LONG nInt32;
      DWORD uInt32;
      INT64 nInt64;
      QWORD uInt64;
      double dReal;
      NXSL_Object *pObject;
		NXSL_Array *pArray;
		NXSL_Iterator *pIterator;
   } m_value;

   void updateNumber();
   void updateString();

   void invalidateString()
   {
      safe_free(m_pszValStr);
      m_pszValStr = NULL;
      m_bStringIsValid = FALSE;
   }

public:
   NXSL_Value();
   NXSL_Value(const NXSL_Value *);
   NXSL_Value(NXSL_Object *pObject);
   NXSL_Value(NXSL_Array *pArray);
   NXSL_Value(NXSL_Iterator *pIterator);
   NXSL_Value(LONG nValue);
   NXSL_Value(INT64 nValue);
   NXSL_Value(DWORD uValue);
   NXSL_Value(QWORD uValue);
   NXSL_Value(double dValue);
   NXSL_Value(const TCHAR *pszValue);
   NXSL_Value(const TCHAR *pszValue, DWORD dwLen);
#ifdef UNICODE
   NXSL_Value(const char *pszValue);
#endif
   ~NXSL_Value();

   void set(LONG nValue);

	void setName(const TCHAR *name) { safe_free(m_name); m_name = _tcsdup(name); }
	const TCHAR *getName() { return m_name; }

   bool convert(int nDataType);
   int getDataType() { return m_nDataType; }

   bool isNull() { return (m_nDataType == NXSL_DT_NULL); }
   bool isObject() { return (m_nDataType == NXSL_DT_OBJECT); }
	bool isObject(const TCHAR *className);
   bool isArray() { return (m_nDataType == NXSL_DT_ARRAY); }
   bool isIterator() { return (m_nDataType == NXSL_DT_ITERATOR); }
   bool isString() { return (m_nDataType >= NXSL_DT_STRING); }
   bool isNumeric() { return (m_nDataType > NXSL_DT_STRING); }
   bool isReal() { return (m_nDataType == NXSL_DT_REAL); }
   bool isInteger() { return (m_nDataType > NXSL_DT_REAL); }
   bool isUnsigned() { return (m_nDataType >= NXSL_DT_UINT32); }
   bool isZero();
   bool isNonZero();

   const TCHAR *getValueAsString(DWORD *pdwLen);
   const TCHAR *getValueAsCString();
   LONG getValueAsInt32();
   DWORD getValueAsUInt32();
   INT64 getValueAsInt64();
   QWORD getValueAsUInt64();
   double getValueAsReal();
   NXSL_Object *getValueAsObject() { return (m_nDataType == NXSL_DT_OBJECT) ? m_value.pObject : NULL; }
   NXSL_Array *getValueAsArray() { return (m_nDataType == NXSL_DT_ARRAY) ? m_value.pArray : NULL; }
   NXSL_Iterator *getValueAsIterator() { return (m_nDataType == NXSL_DT_ITERATOR) ? m_value.pIterator : NULL; }

   void concatenate(const TCHAR *pszString, DWORD dwLen);
   
   void increment();
   void decrement();
   void negate();
   void bitNot();

   void add(NXSL_Value *pVal);
   void sub(NXSL_Value *pVal);
   void mul(NXSL_Value *pVal);
   void div(NXSL_Value *pVal);
   void rem(NXSL_Value *pVal);
   void bitAnd(NXSL_Value *pVal);
   void bitOr(NXSL_Value *pVal);
   void bitXor(NXSL_Value *pVal);
   void lshift(int nBits);
   void rshift(int nBits);

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
   TCHAR m_szName[MAX_FUNCTION_NAME];
   DWORD m_dwAddr;
};


//
// External function structure
//

class NXSL_Program;

struct NXSL_ExtFunction
{
   TCHAR m_szName[MAX_FUNCTION_NAME];
   int (* m_pfHandler)(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_Program *program);
   int m_iNumArgs;   // Number of arguments or -1 for variable number
};


//
// Environment for NXSL program
//

class NXSL_Library;

class LIBNXSL_EXPORTABLE NXSL_Environment
{
private:
   DWORD m_dwNumFunctions;
   NXSL_ExtFunction *m_pFunctionList;

   NXSL_Library *m_pLibrary;

   FILE *m_pStdIn;
   FILE *m_pStdOut;

public:
   NXSL_Environment();
   ~NXSL_Environment();

	virtual void trace(int level, const TCHAR *text);

   void setIO(FILE *pIn, FILE *pOut) { m_pStdIn = pIn; m_pStdOut = pOut; }
   FILE *getStdIn() { return m_pStdIn; }
   FILE *getStdOut() { return m_pStdOut; }

   void setLibrary(NXSL_Library *pLib) { m_pLibrary = pLib; }

   NXSL_ExtFunction *findFunction(const TCHAR *pszName);
   void registerFunctionSet(DWORD dwNumFunctions, NXSL_ExtFunction *pList);

   BOOL useModule(NXSL_Program *main, const TCHAR *name);
};


//
// Runtime variable information
//

class LIBNXSL_EXPORTABLE NXSL_Variable
{
protected:
   TCHAR *m_pszName;
   NXSL_Value *m_pValue;
	bool m_isConstant;

public:
   NXSL_Variable(const TCHAR *pszName);
   NXSL_Variable(const TCHAR *pszName, NXSL_Value *pValue, bool constant = false);
   NXSL_Variable(NXSL_Variable *pSrc);
   ~NXSL_Variable();

   const TCHAR *getName() { return m_pszName; }
   NXSL_Value *getValue() { return m_pValue; }
   void setValue(NXSL_Value *pValue);
	bool isConstant() { return m_isConstant; }
};


//
// Varable system
//

class LIBNXSL_EXPORTABLE NXSL_VariableSystem
{
protected:
   DWORD m_dwNumVariables;
   NXSL_Variable **m_ppVariableList;
	bool m_isConstant;

public:
   NXSL_VariableSystem(bool constant = false);
   NXSL_VariableSystem(NXSL_VariableSystem *pSrc);
   ~NXSL_VariableSystem();

   NXSL_Variable *find(const TCHAR *pszName);
   NXSL_Variable *create(const TCHAR *pszName, NXSL_Value *pValue = NULL);
	void merge(NXSL_VariableSystem *src);
	bool isConstant() { return m_isConstant; }
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
      TCHAR *m_pszString;
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
   NXSL_Instruction(NXSL_Instruction *pSrc);
   ~NXSL_Instruction();
};


//
// Used module information
//

struct NXSL_Module
{
   TCHAR m_szName[MAX_PATH];
   DWORD m_dwCodeStart;
   DWORD m_dwCodeSize;
   DWORD m_dwFunctionStart;
   DWORD m_dwNumFunctions;
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

   DWORD m_dwNumPreloads;
   TCHAR **m_ppszPreloadList;

   DWORD m_dwSubLevel;
   NXSL_Stack *m_pDataStack;
   NXSL_Stack *m_pCodeStack;
   int m_nBindPos;

   NXSL_VariableSystem *m_pConstants;
   NXSL_VariableSystem *m_pGlobals;
   NXSL_VariableSystem *m_pLocals;

   DWORD m_dwNumFunctions;
   NXSL_Function *m_pFunctionList;

   DWORD m_dwNumModules;
   NXSL_Module *m_pModuleList;

   NXSL_Value *m_pRetValue;
   int m_nErrorCode;
   TCHAR *m_pszErrorText;

   void execute();
   void callFunction(int nArgCount);
   void doUnaryOperation(int nOpCode);
   void doBinaryOperation(int nOpCode);
   void error(int nError);
   NXSL_Value *matchRegexp(NXSL_Value *pValue, NXSL_Value *pRegexp, BOOL bIgnoreCase);

   NXSL_Variable *findVariable(const TCHAR *pszName);
   NXSL_Variable *findOrCreateVariable(const TCHAR *pszName);
	NXSL_Variable *createVariable(const TCHAR *pszName);

   DWORD getFunctionAddress(const TCHAR *pszName);
   void relocateCode(DWORD dwStartOffset, DWORD dwLen, DWORD dwShift);
	DWORD getFinalJumpDestination(DWORD dwAddr);

public:
   NXSL_Program();
   ~NXSL_Program();

   BOOL addFunction(const char *pszName, DWORD dwAddr, char *pszError);
   void resolveFunctions();
   void addInstruction(NXSL_Instruction *pInstruction);
   void resolveLastJump(int nOpCode);
	void createJumpAt(DWORD dwOpAddr, DWORD dwJumpAddr);
   void addPreload(char *pszName);
   void useModule(NXSL_Program *pModule, const TCHAR *pszName);
	void optimize();

	void setGlobalVariable(const TCHAR *pszName, NXSL_Value *pValue);
	NXSL_Variable *findGlobalVariable(const TCHAR *pszName) { return m_pGlobals->find(pszName); }

   int run(NXSL_Environment *pEnv = NULL, DWORD argc = 0,
           NXSL_Value **argv = NULL, NXSL_VariableSystem *pUserLocals = NULL,
           NXSL_VariableSystem **ppGlobals = NULL, NXSL_VariableSystem *pConstants = NULL,
			  const TCHAR *entryPoint = NULL);

   DWORD getCodeSize() { return m_dwCodeSize; }

	void trace(int level, const TCHAR *text);
   void dump(FILE *pFile);
   const TCHAR *getErrorText() { return CHECK_NULL_EX(m_pszErrorText); }
   NXSL_Value *getResult() { return m_pRetValue; }
};


//
// Script library
//

class LIBNXSL_EXPORTABLE NXSL_Library
{
private:
   DWORD m_dwNumScripts;
   NXSL_Program **m_ppScriptList;
   TCHAR **m_ppszNames;
   DWORD *m_pdwIdList;
   MUTEX m_mutex;

   void deleteInternal(int nIndex);

public:
   NXSL_Library();
   ~NXSL_Library();

   void lock() { MutexLock(m_mutex); }
   void unlock() { MutexUnlock(m_mutex); }

   BOOL addScript(DWORD dwId, const TCHAR *pszName, NXSL_Program *pScript);
   void deleteScript(const TCHAR *pszName);
   void deleteScript(DWORD dwId);
   NXSL_Program *findScript(const TCHAR *pszName);

   void fillMessage(CSCPMessage *pMsg);
};


#endif
