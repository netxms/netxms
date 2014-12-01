/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2014 Victor Kirhenshtein
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
#define INVALID_ADDRESS    ((UINT32)0xFFFFFFFF)

/**
 * NXSL data types
 */
enum NXSL_DataTypes
{
   NXSL_DT_NULL       = 0,
   NXSL_DT_OBJECT     = 1,
   NXSL_DT_ARRAY      = 2,
   NXSL_DT_ITERATOR   = 3,
   NXSL_DT_STRING     = 4,
   NXSL_DT_REAL       = 5,
   NXSL_DT_INT32      = 6,
   NXSL_DT_INT64      = 7,
   NXSL_DT_UINT32     = 8,
   NXSL_DT_UINT64     = 9
};

/**
 * NXSL stack class
 */
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
   void *peekAt(int offset);
   void **peekList(int nLevel) { return &m_ppData[m_nStackPos - nLevel]; }

   int getSize() { return m_nStackPos; }
};

class NXSL_Value;
class NXSL_Object;
class NXSL_VM;

/**
 * External method structure
 */
struct NXSL_ExtMethod
{
   int (* handler)(NXSL_Object *object, int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
   int numArgs;   // Number of arguments or -1 for variable number
};

#define NXSL_METHOD_DEFINITION(name) \
   static int M_##name (NXSL_Object *object, int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)

#define NXSL_REGISTER_METHOD(name, argc) { \
      NXSL_ExtMethod *m = new NXSL_ExtMethod; \
      m->handler = M_##name; \
      m->numArgs = argc; \
      m_methods->set(_T(#name), m); \
   }

/**
 * Class representing NXSL class
 */
class LIBNXSL_EXPORTABLE NXSL_Class
{
protected:
   TCHAR m_name[MAX_CLASS_NAME];
   StringObjectMap<NXSL_ExtMethod> *m_methods;

public:
   NXSL_Class();
   virtual ~NXSL_Class();

   virtual NXSL_Value *getAttr(NXSL_Object *pObject, const TCHAR *pszAttr);
   virtual BOOL setAttr(NXSL_Object *pObject, const TCHAR *pszAttr, NXSL_Value *pValue);

   virtual int callMethod(const TCHAR *name, NXSL_Object *object, int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);

	virtual void onObjectDelete(NXSL_Object *object);

   const TCHAR *getName() { return m_name; }
};

/**
 * Class data - object and reference count
 */
struct __nxsl_class_data
{
	void *data;
	int refCount;
};

/**
 * Object instance
 */
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

/**
 * Array element
 */
struct NXSL_ArrayElement
{
	int index;
	NXSL_Value *value;
};

/**
 * NXSL array
 */
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

/**
 * Iterator for arrays
 */
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

/**
 * Variable or constant value
 */
class LIBNXSL_EXPORTABLE NXSL_Value
{
protected:
   UINT32 m_dwStrLen;
   TCHAR *m_pszValStr;
#ifdef UNICODE
	char *m_valueMBStr;	// value as MB string; NULL until first request
#endif
	TCHAR *m_name;
   BYTE m_nDataType;
   BYTE m_bStringIsValid;
   union
   {
      INT32 nInt32;
      UINT32 uInt32;
      INT64 nInt64;
      UINT64 uInt64;
      double dReal;
      NXSL_Object *pObject;
		NXSL_Array *pArray;
		NXSL_Iterator *pIterator;
   } m_value;

   void updateNumber();
   void updateString();

   void invalidateString()
   {
      safe_free_and_null(m_pszValStr);
#ifdef UNICODE
		safe_free_and_null(m_valueMBStr);
#endif
      m_bStringIsValid = FALSE;
   }

public:
   NXSL_Value();
   NXSL_Value(const NXSL_Value *);
   NXSL_Value(NXSL_Object *pObject);
   NXSL_Value(NXSL_Array *pArray);
   NXSL_Value(NXSL_Iterator *pIterator);
   NXSL_Value(INT32 nValue);
   NXSL_Value(INT64 nValue);
   NXSL_Value(UINT32 uValue);
   NXSL_Value(UINT64 uValue);
   NXSL_Value(double dValue);
   NXSL_Value(const TCHAR *pszValue);
   NXSL_Value(const TCHAR *pszValue, UINT32 dwLen);
#ifdef UNICODE
   NXSL_Value(const char *pszValue);
#endif
   ~NXSL_Value();

   void set(INT32 nValue);

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

   const TCHAR *getValueAsString(UINT32 *pdwLen);
   const TCHAR *getValueAsCString();
#ifdef UNICODE
   const char *getValueAsMBString();
#else
	const char *getValueAsMBString() { return getValueAsCString(); }
#endif
   INT32 getValueAsInt32();
   UINT32 getValueAsUInt32();
   INT64 getValueAsInt64();
   UINT64 getValueAsUInt64();
   double getValueAsReal();
   NXSL_Object *getValueAsObject() { return (m_nDataType == NXSL_DT_OBJECT) ? m_value.pObject : NULL; }
   NXSL_Array *getValueAsArray() { return (m_nDataType == NXSL_DT_ARRAY) ? m_value.pArray : NULL; }
   NXSL_Iterator *getValueAsIterator() { return (m_nDataType == NXSL_DT_ITERATOR) ? m_value.pIterator : NULL; }

   void concatenate(const TCHAR *pszString, UINT32 dwLen);
   
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

/**
 * NXSL function definition structure
 */
class NXSL_Function
{
public:
   TCHAR m_name[MAX_FUNCTION_NAME];
   UINT32 m_dwAddr;

   NXSL_Function() { m_name[0] = 0; m_dwAddr = INVALID_ADDRESS; }
   NXSL_Function(NXSL_Function *src) { nx_strncpy(m_name, src->m_name, MAX_FUNCTION_NAME); m_dwAddr = src->m_dwAddr; }
};

/**
 * External function structure
 */
struct NXSL_ExtFunction
{
   TCHAR m_name[MAX_FUNCTION_NAME];
   int (* m_pfHandler)(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm);
   int m_iNumArgs;   // Number of arguments or -1 for variable number
};

class NXSL_Library;

/**
 * Environment for NXSL program
 */
class LIBNXSL_EXPORTABLE NXSL_Environment
{
private:
   UINT32 m_dwNumFunctions;
   NXSL_ExtFunction *m_pFunctionList;

   NXSL_Library *m_pLibrary;

public:
   NXSL_Environment();
   ~NXSL_Environment();

	virtual void print(NXSL_Value *value);
	virtual void trace(int level, const TCHAR *text);

   void setLibrary(NXSL_Library *pLib) { m_pLibrary = pLib; }

   NXSL_ExtFunction *findFunction(const TCHAR *pszName);
   void registerFunctionSet(UINT32 dwNumFunctions, NXSL_ExtFunction *pList);

   bool loadModule(NXSL_VM *vm, const TCHAR *name);
};

/**
 * Runtime variable information
 */
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

/**
 * Varable system
 */
class LIBNXSL_EXPORTABLE NXSL_VariableSystem
{
protected:
   ObjectArray<NXSL_Variable> *m_variables;
	bool m_isConstant;

public:
   NXSL_VariableSystem(bool constant = false);
   NXSL_VariableSystem(NXSL_VariableSystem *src);
   ~NXSL_VariableSystem();

   NXSL_Variable *find(const TCHAR *pszName);
   NXSL_Variable *create(const TCHAR *pszName, NXSL_Value *pValue = NULL);
	void merge(NXSL_VariableSystem *src);
	void addAll(StringObjectMap<NXSL_Value> *src);
   void clear() { m_variables->clear(); }
	bool isConstant() { return m_isConstant; }
};

/**
 * Single execution instruction
 */
class LIBNXSL_EXPORTABLE NXSL_Instruction
{
   friend class NXSL_Program;
   friend class NXSL_VM;

protected:
   int m_nOpCode;
   union
   {
      NXSL_Value *m_pConstant;
      TCHAR *m_pszString;
      UINT32 m_dwAddr;
   } m_operand;
   int m_nStackItems;
   int m_nSourceLine;

public:
   NXSL_Instruction(int nLine, int nOpCode);
   NXSL_Instruction(int nLine, int nOpCode, NXSL_Value *pValue);
   NXSL_Instruction(int nLine, int nOpCode, char *pszString);
   NXSL_Instruction(int nLine, int nOpCode, char *pszString, int nStackItems);
   NXSL_Instruction(int nLine, int nOpCode, UINT32 dwAddr);
   NXSL_Instruction(int nLine, int nOpCode, int nStackItems);
   NXSL_Instruction(NXSL_Instruction *pSrc);
   ~NXSL_Instruction();
};

/**
 * NXSL module information
 */
struct NXSL_Module
{
   TCHAR m_name[MAX_PATH];
   UINT32 m_codeStart;
   int m_codeSize;
   int m_functionStart;
   int m_numFunctions;
};

/**
 * Compiled NXSL program
 */
class LIBNXSL_EXPORTABLE NXSL_Program
{
   friend class NXSL_VM;

protected:
   ObjectArray<NXSL_Instruction> *m_instructionSet;
   StringList m_requiredModules;
   StringObjectMap<NXSL_Value> *m_constants;
   ObjectArray<NXSL_Function> *m_functions;

	UINT32 getFinalJumpDestination(UINT32 dwAddr, int srcJump);

public:
   NXSL_Program();
   ~NXSL_Program();

   bool addFunction(const char *pszName, UINT32 dwAddr, char *pszError);
   void resolveFunctions();
   void addInstruction(NXSL_Instruction *pInstruction) { m_instructionSet->add(pInstruction); }
   void resolveLastJump(int nOpCode);
	void createJumpAt(UINT32 dwOpAddr, UINT32 dwJumpAddr);
   void addRequiredModule(char *pszName);
	void optimize();
	void removeInstructions(UINT32 start, int count);
   bool addConstant(const char *name, NXSL_Value *value);

   UINT32 getCodeSize() { return m_instructionSet->size(); }

   void dump(FILE *pFile);
};

/**
 * Script library
 */
class LIBNXSL_EXPORTABLE NXSL_Library
{
private:
   UINT32 m_dwNumScripts;
   NXSL_Program **m_ppScriptList;
   TCHAR **m_ppszNames;
   UINT32 *m_pdwIdList;
   MUTEX m_mutex;

   void deleteInternal(int nIndex);

public:
   NXSL_Library();
   ~NXSL_Library();

   void lock() { MutexLock(m_mutex); }
   void unlock() { MutexUnlock(m_mutex); }

   BOOL addScript(UINT32 dwId, const TCHAR *pszName, NXSL_Program *pScript);
   void deleteScript(const TCHAR *name);
   void deleteScript(UINT32 id);
   NXSL_Program *findScript(const TCHAR *name);
   NXSL_VM *createVM(const TCHAR *name, NXSL_Environment *env);

   void fillMessage(NXCPMessage *pMsg);
};

/**
 * Catch point information
 */
struct NXSL_CatchPoint
{
   UINT32 addr;
   UINT32 subLevel;
   int dataStackSize;
};

/**
 * NXSL virtual machine
 */
class LIBNXSL_EXPORTABLE NXSL_VM
{
private:
   static bool createConstantsCallback(const TCHAR *key, const void *value, void *data);

protected:
   NXSL_Environment *m_env;
	void *m_userData;

   ObjectArray<NXSL_Instruction> *m_instructionSet;
   UINT32 m_cp;

   UINT32 m_dwSubLevel;
   NXSL_Stack *m_dataStack;
   NXSL_Stack *m_codeStack;
   NXSL_Stack *m_catchStack;
   int m_nBindPos;

   NXSL_VariableSystem *m_constants;
   NXSL_VariableSystem *m_globals;
   NXSL_VariableSystem *m_locals;

   ObjectArray<NXSL_Function> *m_functions;
   ObjectArray<NXSL_Module> *m_modules;

   NXSL_Value *m_pRetValue;
   int m_errorCode;
   int m_errorLine;
   TCHAR *m_errorText;

   void execute();
   bool unwind();
   void callFunction(int nArgCount);
   void doUnaryOperation(int nOpCode);
   void doBinaryOperation(int nOpCode);
   void error(int nError);
   NXSL_Value *matchRegexp(NXSL_Value *pValue, NXSL_Value *pRegexp, BOOL bIgnoreCase);

   NXSL_Variable *findVariable(const TCHAR *pszName);
   NXSL_Variable *findOrCreateVariable(const TCHAR *pszName);
	NXSL_Variable *createVariable(const TCHAR *pszName);

   void relocateCode(UINT32 dwStartOffset, UINT32 dwLen, UINT32 dwShift);
   UINT32 getFunctionAddress(const TCHAR *pszName);

public:
   NXSL_VM(NXSL_Environment *env = NULL);
   ~NXSL_VM();

   void loadModule(NXSL_Program *module, const TCHAR *name);

	void setGlobalVariable(const TCHAR *pszName, NXSL_Value *pValue);
	NXSL_Variable *findGlobalVariable(const TCHAR *pszName) { return m_globals->find(pszName); }

   bool load(NXSL_Program *program);
   bool run(ObjectArray<NXSL_Value> *args, NXSL_VariableSystem *pUserLocals = NULL,
            NXSL_VariableSystem **ppGlobals = NULL, NXSL_VariableSystem *pConstants = NULL,
            const TCHAR *entryPoint = NULL);
   bool run(int argc, NXSL_Value **argv, NXSL_VariableSystem *pUserLocals = NULL,
            NXSL_VariableSystem **ppGlobals = NULL, NXSL_VariableSystem *pConstants = NULL,
            const TCHAR *entryPoint = NULL);
   bool run() { ObjectArray<NXSL_Value> args(1, 1, false); return run(&args); }

   UINT32 getCodeSize() { return m_instructionSet->size(); }

	void trace(int level, const TCHAR *text);
   void dump(FILE *pFile);
   int getErrorCode() { return m_errorCode; }
   int getErrorLine() { return m_errorLine; }
   const TCHAR *getErrorText() { return CHECK_NULL_EX(m_errorText); }
   NXSL_Value *getResult() { return m_pRetValue; }

	void *getUserData() { return m_userData; }
	void setUserData(void *data) { m_userData = data; }
};

/**
 * NXSL "TableColumn" class
 */
class LIBNXSL_EXPORTABLE NXSL_TableColumnClass : public NXSL_Class
{
public:
   NXSL_TableColumnClass();
   virtual ~NXSL_TableColumnClass();

   virtual NXSL_Value *getAttr(NXSL_Object *pObject, const TCHAR *pszAttr);
	virtual void onObjectDelete(NXSL_Object *object);
};

/**
 * NXSL "Table" class
 */
class LIBNXSL_EXPORTABLE NXSL_TableClass : public NXSL_Class
{
public:
   NXSL_TableClass();
   virtual ~NXSL_TableClass();

   virtual NXSL_Value *getAttr(NXSL_Object *pObject, const TCHAR *pszAttr);
	virtual void onObjectDelete(NXSL_Object *object);
};

/**
 * NXSL "StaticTable" class - table that is not deleted when ref count reaches 0
 */
class LIBNXSL_EXPORTABLE NXSL_StaticTableClass : public NXSL_TableClass
{
public:
   NXSL_StaticTableClass();
   virtual ~NXSL_StaticTableClass();

	virtual void onObjectDelete(NXSL_Object *object);
};

/**
 * NXSL "Connector" class
 */
class LIBNXSL_EXPORTABLE NXSL_ConnectorClass : public NXSL_Class
{
public:
   NXSL_ConnectorClass();
   virtual ~NXSL_ConnectorClass();

   virtual NXSL_Value *getAttr(NXSL_Object *pObject, const TCHAR *pszAttr);
	virtual void onObjectDelete(NXSL_Object *object);
};

/**
 * Class definition instances
 */
extern NXSL_TableClass LIBNXSL_EXPORTABLE g_nxslTableClass;
extern NXSL_StaticTableClass LIBNXSL_EXPORTABLE g_nxslStaticTableClass;
extern NXSL_TableColumnClass LIBNXSL_EXPORTABLE g_nxslTableColumnClass;
extern NXSL_ConnectorClass LIBNXSL_EXPORTABLE g_nxslConnectorClass;

#endif
