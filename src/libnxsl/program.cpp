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
** File: program.cpp
**
**/

#include "libnxsl.h"
#include <netxms-regex.h>


//
// Constants
//

#define MAX_ERROR_NUMBER         26
#define CONTROL_STACK_LIMIT      32768


//
// Command mnemonics
//

static const char *m_szCommandMnemonic[] =
{
   "NOP", "RET", "JMP", "CALL", "CALL",
   "PUSH", "PUSH", "EXIT", "POP", "SET",
   "ADD", "SUB", "MUL", "DIV", "REM",
   "EQ", "NE", "LT", "LE", "GT", "GE",
   "BITAND", "BITOR", "BITXOR",
   "AND", "OR", "LSHIFT", "RSHIFT",
   "NRET", "JZ", "PRINT", "CONCAT",
   "BIND", "INC", "DEC", "NEG", "NOT",
   "BITNOT", "CAST", "AGET", "INCP", "DECP",
   "JNZ", "LIKE", "ILIKE", "MATCH",
   "IMATCH", "CASE", "ARRAY", "EGET",
	"ESET", "ASET", "NAME", "FOREACH", "NEXT"
};


//
// Error texts
//

static const TCHAR *m_szErrorMessage[MAX_ERROR_NUMBER] =
{
	_T("Data stack underflow"),
	_T("Control stack underflow"),
	_T("Condition value is not a number"),
	_T("Bad arithmetic conversion"),
	_T("Invalid operation with NULL value"),
	_T("Internal error"),
	_T("main() function not presented"),
	_T("Control stack overflow"),
	_T("Divide by zero"),
	_T("Invalid operation with real numbers"),
	_T("Function not found"),
	_T("Invalid number of function's arguments"),
	_T("Cannot do automatic type cast"),
	_T("Function or operation argument is not an object"),
	_T("Unknown object's attribute"),
	_T("Requested module not found or cannot be loaded"),
	_T("Argument is not of string type and cannot be converted to string"),
	_T("Invalid regular expression"),
	_T("Function or operation argument is not a whole number"),
	_T("Invalid operation on object"),
	_T("Bad (or incompatible) object class"),
	_T("Variable already exist"),
	_T("Array index is not an integer"),
	_T("Attempt to use array element access operation on non-array"),
	_T("Cannot assign to a variable that is constant"),
	_T("Named parameter required")
};


//
// Determine operation data type
//

static int SelectResultType(int nType1, int nType2, int nOp)
{
   int nType;

   if (nOp == OPCODE_DIV)
   {
      nType = NXSL_DT_REAL;
   }
   else
   {
      if ((nType1 == NXSL_DT_REAL) || (nType2 == NXSL_DT_REAL))
      {
         if ((nOp == OPCODE_REM) || (nOp == OPCODE_LSHIFT) ||
             (nOp == OPCODE_RSHIFT) || (nOp == OPCODE_BIT_AND) ||
             (nOp == OPCODE_BIT_OR) || (nOp == OPCODE_BIT_XOR))
         {
            nType = NXSL_DT_NULL;   // Error
         }
         else
         {
            nType = NXSL_DT_REAL;
         }
      }
      else
      {
         if (((nType1 >= NXSL_DT_UINT32) && (nType2 < NXSL_DT_UINT32)) ||
             ((nType1 < NXSL_DT_UINT32) && (nType2 >= NXSL_DT_UINT32)))
         {
            // One operand signed, other unsigned, convert both to signed
            if (nType1 >= NXSL_DT_UINT32)
               nType1 -= 2;
            else if (nType2 >= NXSL_DT_UINT32)
               nType2 -= 2;
         }
         nType = max(nType1, nType2);
      }
   }
   return nType;
}


//
// Constructor
//

NXSL_Program::NXSL_Program()
{
   m_ppInstructionSet = NULL;
   m_dwCodeSize = 0;
   m_dwCurrPos = INVALID_ADDRESS;
   m_pDataStack = NULL;
   m_pCodeStack = NULL;
   m_nErrorCode = 0;
   m_pszErrorText = NULL;
   m_pConstants = new NXSL_VariableSystem(true);
   m_pGlobals = new NXSL_VariableSystem(false);
   m_pLocals = NULL;
   m_dwNumFunctions = 0;
   m_pFunctionList = NULL;
   m_dwSubLevel = 0;    // Level of current subroutine
   m_pEnv = NULL;
   m_pRetValue = NULL;
   m_dwNumModules = 0;
   m_pModuleList = NULL;
   m_dwNumPreloads = 0;
   m_ppszPreloadList = NULL;

	srand((unsigned int)time(NULL));
}


//
// Destructor
//

NXSL_Program::~NXSL_Program()
{
   DWORD i;

   for(i = 0; i < m_dwCodeSize; i++)
      delete m_ppInstructionSet[i];
   safe_free(m_ppInstructionSet);
   
   delete m_pDataStack;
   delete m_pCodeStack;

   delete m_pConstants;
   delete m_pGlobals;
   delete m_pLocals;

   delete m_pEnv;
   delete m_pRetValue;

   safe_free(m_pFunctionList);
   safe_free(m_pModuleList);

   for(i = 0; i < m_dwNumPreloads; i++)
      safe_free(m_ppszPreloadList[i]);
   safe_free(m_ppszPreloadList);

   safe_free(m_pszErrorText);
}


//
// Add new instruction to set
//

void NXSL_Program::addInstruction(NXSL_Instruction *pInstruction)
{
   m_ppInstructionSet = (NXSL_Instruction **)realloc(m_ppInstructionSet,
         sizeof(NXSL_Instruction *) * (m_dwCodeSize + 1));
   m_ppInstructionSet[m_dwCodeSize++] = pInstruction;
}


//
// Resolve last jump with INVALID_ADDRESS to current address
//

void NXSL_Program::resolveLastJump(int nOpCode)
{
   DWORD i;

   for(i = m_dwCodeSize; i > 0;)
   {
      i--;
      if ((m_ppInstructionSet[i]->m_nOpCode == nOpCode) &&
          (m_ppInstructionSet[i]->m_operand.m_dwAddr == INVALID_ADDRESS))
      {
         m_ppInstructionSet[i]->m_operand.m_dwAddr = m_dwCodeSize;
         break;
      }
   }
}


//
// Create jump at given address replacing another instruction (usually NOP)
//

void NXSL_Program::createJumpAt(DWORD dwOpAddr, DWORD dwJumpAddr)
{
	int nLine;

	if (dwOpAddr >= m_dwCodeSize)
		return;

	nLine = m_ppInstructionSet[dwOpAddr]->m_nSourceLine;
	delete m_ppInstructionSet[dwOpAddr];
	m_ppInstructionSet[dwOpAddr] = new NXSL_Instruction(nLine, OPCODE_JMP, dwJumpAddr);
}


//
// Add new function to defined functions list
// Will use first free address if dwAddr == INVALID_ADDRESS
// Name must be in UTF-8
//

BOOL NXSL_Program::addFunction(const char *pszName, DWORD dwAddr, char *pszError)
{
   DWORD i;

   // Check for duplicate function names
#ifdef UNICODE
	WCHAR *pwszName = WideStringFromUTF8String(pszName);
#endif
   for(i = 0; i < m_dwNumFunctions; i++)
#ifdef UNICODE
      if (!wcscmp(m_pFunctionList[i].m_szName, pwszName))
#else
      if (!strcmp(m_pFunctionList[i].m_szName, pszName))
#endif
      {
         sprintf(pszError, "Duplicate function name: \"%s\"", pszName);
#ifdef UNICODE
			free(pwszName);
#endif
         return FALSE;
      }
   m_dwNumFunctions++;
   m_pFunctionList = (NXSL_Function *)realloc(m_pFunctionList, sizeof(NXSL_Function) * m_dwNumFunctions);
#ifdef UNICODE
   nx_strncpy(m_pFunctionList[i].m_szName, pwszName, MAX_FUNCTION_NAME);
	free(pwszName);
#else
   nx_strncpy(m_pFunctionList[i].m_szName, pszName, MAX_FUNCTION_NAME);
#endif
   m_pFunctionList[i].m_dwAddr = (dwAddr == INVALID_ADDRESS) ? m_dwCodeSize : dwAddr;
   return TRUE;
}


//
// Add preload information
//

void NXSL_Program::addPreload(char *pszName)
{
   m_ppszPreloadList = (TCHAR **)realloc(m_ppszPreloadList, sizeof(TCHAR *) * (m_dwNumPreloads + 1));
#ifdef UNICODE
	m_ppszPreloadList[m_dwNumPreloads] = WideStringFromUTF8String(pszName);
	free(pszName);
#else
   m_ppszPreloadList[m_dwNumPreloads] = pszName;
#endif
   m_dwNumPreloads++;
}


//
// resolve local functions
//

void NXSL_Program::resolveFunctions()
{
   DWORD i, j;

   for(i = 0; i < m_dwCodeSize; i++)
   {
      if (m_ppInstructionSet[i]->m_nOpCode == OPCODE_CALL_EXTERNAL)
      {
         for(j = 0; j < m_dwNumFunctions; j++)
         {
            if (!_tcscmp(m_pFunctionList[j].m_szName,
                        m_ppInstructionSet[i]->m_operand.m_pszString))
            {
               free(m_ppInstructionSet[i]->m_operand.m_pszString);
               m_ppInstructionSet[i]->m_operand.m_dwAddr = m_pFunctionList[j].m_dwAddr;
               m_ppInstructionSet[i]->m_nOpCode = OPCODE_CALL;
               break;
            }
         }
      }
   }
}


//
// Dump program to file (as text)
//

void NXSL_Program::dump(FILE *pFile)
{
   DWORD i;

   for(i = 0; i < m_dwCodeSize; i++)
   {
      fprintf(pFile, "%04X  %-6s  ", i,
              m_szCommandMnemonic[m_ppInstructionSet[i]->m_nOpCode]);
      switch(m_ppInstructionSet[i]->m_nOpCode)
      {
         case OPCODE_CALL_EXTERNAL:
            _ftprintf(pFile, _T("%s, %d\n"), m_ppInstructionSet[i]->m_operand.m_pszString,
                      m_ppInstructionSet[i]->m_nStackItems);
            break;
         case OPCODE_CALL:
            _ftprintf(pFile, _T("%04X, %d\n"), m_ppInstructionSet[i]->m_operand.m_dwAddr,
                      m_ppInstructionSet[i]->m_nStackItems);
            break;
         case OPCODE_JMP:
         case OPCODE_JZ:
         case OPCODE_JNZ:
            fprintf(pFile, "%04X\n", m_ppInstructionSet[i]->m_operand.m_dwAddr);
            break;
         case OPCODE_PUSH_VARIABLE:
         case OPCODE_SET:
         case OPCODE_BIND:
         case OPCODE_ARRAY:
         case OPCODE_INC:
         case OPCODE_DEC:
         case OPCODE_INCP:
         case OPCODE_DECP:
         case OPCODE_GET_ATTRIBUTE:
         case OPCODE_SET_ATTRIBUTE:
			case OPCODE_NAME:
            _ftprintf(pFile, _T("%s\n"), m_ppInstructionSet[i]->m_operand.m_pszString);
            break;
         case OPCODE_PUSH_CONSTANT:
			case OPCODE_CASE:
            if (m_ppInstructionSet[i]->m_operand.m_pConstant->isNull())
               fprintf(pFile, "<null>\n");
            else
               _ftprintf(pFile, _T("\"%s\"\n"), m_ppInstructionSet[i]->m_operand.m_pConstant->getValueAsCString());
            break;
         case OPCODE_POP:
            fprintf(pFile, "%d\n", m_ppInstructionSet[i]->m_nStackItems);
            break;
         case OPCODE_CAST:
            _ftprintf(pFile, _T("[%s]\n"), g_szTypeNames[m_ppInstructionSet[i]->m_nStackItems]);
            break;
         default:
            fprintf(pFile, "\n");
            break;
      }
   }
}


//
// Report error
//

void NXSL_Program::error(int nError)
{
   TCHAR szBuffer[1024];

   safe_free(m_pszErrorText);
   _sntprintf(szBuffer, 1024, _T("Error %d in line %d: %s"), nError,
              (m_dwCurrPos == INVALID_ADDRESS) ? 0 : m_ppInstructionSet[m_dwCurrPos]->m_nSourceLine,
              ((nError > 0) && (nError <= MAX_ERROR_NUMBER)) ? m_szErrorMessage[nError - 1] : _T("Unknown error code"));
   m_pszErrorText = _tcsdup(szBuffer);
   m_dwCurrPos = INVALID_ADDRESS;
}


//
// Run program
// Returns 0 on success and -1 on error
//

int NXSL_Program::run(NXSL_Environment *pEnv, DWORD argc, NXSL_Value **argv,
                      NXSL_VariableSystem *pUserLocals, NXSL_VariableSystem **ppGlobals,
							 NXSL_VariableSystem *pConstants, const TCHAR *entryPoint)
{
   DWORD i, dwOrigCodeSize, dwOrigNumFn;
   NXSL_VariableSystem *pSavedGlobals, *pSavedConstants = NULL;
   NXSL_Value *pValue;
   TCHAR szBuffer[32];

   // Save original code size and number of functions
   dwOrigCodeSize = m_dwCodeSize;
   dwOrigNumFn = m_dwNumFunctions;

   // Delete previous return value
   delete_and_null(m_pRetValue);

   // Use provided environment or create default
   if (pEnv != NULL)
      m_pEnv = pEnv;
   else
      m_pEnv = new NXSL_Environment;

   // Create stacks
   m_pDataStack = new NXSL_Stack;
   m_pCodeStack = new NXSL_Stack;

   // Create local variable system for main() and bind arguments
   m_pLocals = (pUserLocals == NULL) ? new NXSL_VariableSystem : pUserLocals;
   for(i = 0; i < argc; i++)
   {
      _sntprintf(szBuffer, 32, _T("$%d"), i + 1);
      m_pLocals->create(szBuffer, argv[i]);
   }

   // Preserve original global variables and constants
   pSavedGlobals = new NXSL_VariableSystem(m_pGlobals);
	if (pConstants != NULL)
	{
		pSavedConstants = new NXSL_VariableSystem(m_pConstants);
		m_pConstants->merge(pConstants);
	}

   // Preload modules
   for(i = 0; i < m_dwNumPreloads; i++)
   {
      if (!m_pEnv->useModule(this, m_ppszPreloadList[i]))
      {
         error(NXSL_ERR_MODULE_NOT_FOUND);
         break;
      }
   }

   // Locate entry point and run
   if (i == m_dwNumPreloads)
   {
		if (entryPoint != NULL)
		{
			for(i = 0; i < m_dwNumFunctions; i++)
				if (!_tcscmp(m_pFunctionList[i].m_szName, entryPoint))
					break;
		}
		else
		{
			for(i = 0; i < m_dwNumFunctions; i++)
				if (!_tcscmp(m_pFunctionList[i].m_szName, _T("main")))
					break;

			// No explicit main(), search for implicit
			if (i == m_dwNumFunctions)
			{
				for(i = 0; i < m_dwNumFunctions; i++)
					if (!_tcscmp(m_pFunctionList[i].m_szName, _T("$main")))
						break;
			}
		}

      if (i < m_dwNumFunctions)
      {
         m_dwCurrPos = m_pFunctionList[i].m_dwAddr;
         while(m_dwCurrPos < m_dwCodeSize)
            execute();
         if (m_dwCurrPos != INVALID_ADDRESS)
            m_pRetValue = (NXSL_Value *)m_pDataStack->pop();
      }
      else
      {
         error(NXSL_ERR_NO_MAIN);
      }
   }

   // Restore global variables
   if (ppGlobals == NULL)
	   delete m_pGlobals;
	else
		*ppGlobals = m_pGlobals;
   m_pGlobals = pSavedGlobals;

	// Restore constants
	if (pSavedConstants != NULL)
	{
		delete m_pConstants;
		m_pConstants = pSavedConstants;
	}

   // Cleanup
   while((pValue = (NXSL_Value *)m_pDataStack->pop()) != NULL)
      delete pValue;
   while(m_dwSubLevel > 0)
   {
      m_dwSubLevel--;
      delete (NXSL_VariableSystem *)m_pCodeStack->pop();
      m_pCodeStack->pop();
   }
   delete_and_null(m_pEnv);
   delete_and_null(m_pLocals);
   delete_and_null(m_pDataStack);
   delete_and_null(m_pCodeStack);
   safe_free(m_pModuleList);
   m_pModuleList = NULL;
   m_dwNumModules = 0;

   // Restore original code size and number of functions
   for(i = dwOrigCodeSize; i < m_dwCodeSize; i++)
      delete m_ppInstructionSet[i];
   m_dwCodeSize = dwOrigCodeSize;
   m_dwNumFunctions = dwOrigNumFn;

   return (m_dwCurrPos == INVALID_ADDRESS) ? -1 : 0;
}


//
// Set global variale
//

void NXSL_Program::setGlobalVariable(const TCHAR *pszName, NXSL_Value *pValue)
{
   NXSL_Variable *pVar;

	pVar = m_pGlobals->find(pszName);
   if (pVar == NULL)
		pVar = m_pGlobals->create(pszName, pValue);
	else
		pVar->setValue(pValue);
}


//
// Find variable or create if does not exist
//

NXSL_Variable *NXSL_Program::findOrCreateVariable(const TCHAR *pszName)
{
   NXSL_Variable *pVar;

   pVar = m_pConstants->find(pszName);
   if (pVar == NULL)
   {
      pVar = m_pGlobals->find(pszName);
      if (pVar == NULL)
      {
         pVar = m_pLocals->find(pszName);
         if (pVar == NULL)
         {
            pVar = m_pLocals->create(pszName);
         }
      }
   }
   return pVar;
}


//
// Create variable if it does not exist, otherwise return NULL
//

NXSL_Variable *NXSL_Program::createVariable(const TCHAR *pszName)
{
   NXSL_Variable *pVar = NULL;

   if (m_pConstants->find(pszName) == NULL)
   {
      if (m_pGlobals->find(pszName) == NULL)
      {
         if (m_pLocals->find(pszName) == NULL)
         {
            pVar = m_pLocals->create(pszName);
         }
      }
   }
   return pVar;
}


//
// Execute single instruction
//

void NXSL_Program::execute()
{
   NXSL_Instruction *cp;
   NXSL_Value *pValue;
   NXSL_Variable *pVar;
   NXSL_ExtFunction *pFunc;
   DWORD dwNext = m_dwCurrPos + 1;
   TCHAR szBuffer[256];
   int i, nRet;

   cp = m_ppInstructionSet[m_dwCurrPos];
   switch(cp->m_nOpCode)
   {
      case OPCODE_PUSH_CONSTANT:
         m_pDataStack->push(new NXSL_Value(cp->m_operand.m_pConstant));
         break;
      case OPCODE_PUSH_VARIABLE:
         pVar = findOrCreateVariable(cp->m_operand.m_pszString);
         m_pDataStack->push(new NXSL_Value(pVar->getValue()));
         break;
      case OPCODE_SET:
         pVar = findOrCreateVariable(cp->m_operand.m_pszString);
			if (!pVar->isConstant())
			{
				pValue = (NXSL_Value *)m_pDataStack->peek();
				if (pValue != NULL)
				{
					if (pValue->isArray())
					{
						pVar->setValue(new NXSL_Value(new NXSL_Array(pValue->getValueAsArray())));
					}
					else
					{
						pVar->setValue(new NXSL_Value(pValue));
					}
				}
				else
				{
					error(NXSL_ERR_DATA_STACK_UNDERFLOW);
				}
			}
			else
			{
				error(NXSL_ERR_ASSIGNMENT_TO_CONSTANT);
			}
         break;
		case OPCODE_ARRAY:
         pVar = createVariable(cp->m_operand.m_pszString);
			if (pVar != NULL)
			{
				pVar->setValue(new NXSL_Value(new NXSL_Array));
			}
			else
			{
				error(NXSL_ERR_VARIABLE_ALREADY_EXIST);
			}
			break;
		case OPCODE_SET_ELEMENT:	// Set array element; stack should contain: array index value (top)
			pValue = (NXSL_Value *)m_pDataStack->pop();
			if (pValue != NULL)
			{
				NXSL_Value *array, *index;

				index = (NXSL_Value *)m_pDataStack->pop();
				array = (NXSL_Value *)m_pDataStack->pop();
				if ((index != NULL) && (array != NULL))
				{
					if (!index->isInteger())
					{
						error(NXSL_ERR_INDEX_NOT_INTEGER);
					}
					else if (!array->isArray())
					{
						error(NXSL_ERR_NOT_ARRAY);
					}
					else
					{
						if (pValue->isArray())
						{
							array->getValueAsArray()->set(index->getValueAsInt32(), new NXSL_Value(new NXSL_Array(pValue->getValueAsArray())));
						}
						else
						{
							array->getValueAsArray()->set(index->getValueAsInt32(), new NXSL_Value(pValue));
						}
						m_pDataStack->push(pValue);
						pValue = NULL;		// Prevent deletion
					}
				}
				else
				{
					error(NXSL_ERR_DATA_STACK_UNDERFLOW);
				}
				delete index;
				delete array;
				delete pValue;
			}
         else
         {
            error(NXSL_ERR_DATA_STACK_UNDERFLOW);
         }
			break;
		case OPCODE_GET_ELEMENT:	// Get array element; stack should contain: array index (top)
			pValue = (NXSL_Value *)m_pDataStack->pop();
			if (pValue != NULL)
			{
				NXSL_Value *array;

				array = (NXSL_Value *)m_pDataStack->pop();
				if (array != NULL)
				{
					if (array->isArray())
					{
						if (pValue->isInteger())
						{
							NXSL_Value *element = array->getValueAsArray()->get(pValue->getValueAsInt32());
							m_pDataStack->push((element != NULL) ? new NXSL_Value(element) : new NXSL_Value);
						}
						else
						{
							error(NXSL_ERR_INDEX_NOT_INTEGER);
						}
					}
					else
					{
						error(NXSL_ERR_NOT_ARRAY);
					}
					delete array;
				}
				else
				{
					error(NXSL_ERR_DATA_STACK_UNDERFLOW);
				}
				delete pValue;
			}
         else
         {
            error(NXSL_ERR_DATA_STACK_UNDERFLOW);
         }
			break;
      case OPCODE_CAST:
         pValue = (NXSL_Value *)m_pDataStack->peek();
         if (pValue != NULL)
         {
            if (!pValue->convert(cp->m_nStackItems))
            {
               error(NXSL_ERR_TYPE_CAST);
            }
         }
         else
         {
            error(NXSL_ERR_DATA_STACK_UNDERFLOW);
         }
         break;
		case OPCODE_NAME:
         pValue = (NXSL_Value *)m_pDataStack->peek();
         if (pValue != NULL)
         {
				pValue->setName(cp->m_operand.m_pszString);
         }
         else
         {
            error(NXSL_ERR_DATA_STACK_UNDERFLOW);
         }
			break;
      case OPCODE_POP:
         for(i = 0; i < cp->m_nStackItems; i++)
            delete (NXSL_Value *)m_pDataStack->pop();
         break;
      case OPCODE_JMP:
         dwNext = cp->m_operand.m_dwAddr;
         break;
      case OPCODE_JZ:
      case OPCODE_JNZ:
         pValue = (NXSL_Value *)m_pDataStack->pop();
         if (pValue != NULL)
         {
            if (pValue->isNumeric())
            {
               if (cp->m_nOpCode == OPCODE_JZ ? pValue->isZero() : pValue->isNonZero())
                  dwNext = cp->m_operand.m_dwAddr;
            }
            else
            {
               error(3);
            }
            delete pValue;
         }
         else
         {
            error(NXSL_ERR_DATA_STACK_UNDERFLOW);
         }
         break;
      case OPCODE_CALL:
         dwNext = cp->m_operand.m_dwAddr;
         callFunction(cp->m_nStackItems);
         break;
      case OPCODE_CALL_EXTERNAL:
         pFunc = m_pEnv->findFunction(cp->m_operand.m_pszString);
         if (pFunc != NULL)
         {
            if ((cp->m_nStackItems == pFunc->m_iNumArgs) ||
                (pFunc->m_iNumArgs == -1))
            {
               if (m_pDataStack->getSize() >= cp->m_nStackItems)
               {
                  nRet = pFunc->m_pfHandler(cp->m_nStackItems,
                                            (NXSL_Value **)m_pDataStack->peekList(cp->m_nStackItems),
                                            &pValue, this);
                  if (nRet == 0)
                  {
                     for(i = 0; i < cp->m_nStackItems; i++)
                        delete (NXSL_Value *)m_pDataStack->pop();
                     m_pDataStack->push(pValue);
                  }
                  else if (nRet == NXSL_STOP_SCRIPT_EXECUTION)
						{
                     m_pDataStack->push(pValue);
			            dwNext = m_dwCodeSize;
						}
						else
                  {
                     // Execution error inside function
                     error(nRet);
                  }
               }
               else
               {
                  error(NXSL_ERR_DATA_STACK_UNDERFLOW);
               }
            }
            else
            {
               error(NXSL_ERR_INVALID_ARGUMENT_COUNT);
            }
         }
         else
         {
            DWORD dwAddr;

            dwAddr = getFunctionAddress(cp->m_operand.m_pszString);
            if (dwAddr != INVALID_ADDRESS)
            {
               dwNext = dwAddr;
               callFunction(cp->m_nStackItems);
            }
            else
            {
               error(NXSL_ERR_NO_FUNCTION);
            }
         }
         break;
      case OPCODE_RET_NULL:
         m_pDataStack->push(new NXSL_Value);
      case OPCODE_RETURN:
         if (m_dwSubLevel > 0)
         {
            m_dwSubLevel--;
            delete m_pLocals;
            m_pLocals = (NXSL_VariableSystem *)m_pCodeStack->pop();
            dwNext = CAST_FROM_POINTER(m_pCodeStack->pop(), DWORD);
         }
         else
         {
            // Return from main(), terminate program
            dwNext = m_dwCodeSize;
         }
         break;
      case OPCODE_BIND:
         _sntprintf(szBuffer, 256, _T("$%d"), m_nBindPos++);
         pVar = m_pLocals->find(szBuffer);
         pValue = (pVar != NULL) ? new NXSL_Value(pVar->getValue()) : new NXSL_Value;
         pVar = m_pLocals->find(cp->m_operand.m_pszString);
         if (pVar == NULL)
            m_pLocals->create(cp->m_operand.m_pszString, pValue);
         else
            pVar->setValue(pValue);
         break;
      case OPCODE_PRINT:
         pValue = (NXSL_Value *)m_pDataStack->pop();
         if (pValue != NULL)
         {
            const TCHAR *pszText;
            DWORD dwLen;

            if (m_pEnv->getStdOut() != NULL)
            {
               pszText = pValue->getValueAsString(&dwLen);
               if (pszText != NULL)
					{
#ifdef UNICODE
						int reqLen = WideCharToMultiByte(CP_ACP, WC_DEFAULTCHAR | WC_COMPOSITECHECK, pszText, dwLen, NULL, 0, NULL, NULL);
						char *mbText = (char *)malloc(reqLen);
						WideCharToMultiByte(CP_ACP, WC_DEFAULTCHAR | WC_COMPOSITECHECK, pszText, dwLen, mbText, reqLen, NULL, NULL);
                  fwrite(mbText, reqLen, 1, m_pEnv->getStdOut());
						free(mbText);
#else
                  fwrite(pszText, dwLen, 1, m_pEnv->getStdOut());
#endif
					}
               else
					{
                  fputs("(null)", m_pEnv->getStdOut());
					}
            }
            delete pValue;
         }
         else
         {
            error(NXSL_ERR_DATA_STACK_UNDERFLOW);
         }
         break;
      case OPCODE_EXIT:
			if (m_pDataStack->getSize() > 0)
         {
            dwNext = m_dwCodeSize;
         }
         else
         {
            error(NXSL_ERR_DATA_STACK_UNDERFLOW);
         }
         break;
      case OPCODE_ADD:
      case OPCODE_SUB:
      case OPCODE_MUL:
      case OPCODE_DIV:
      case OPCODE_REM:
      case OPCODE_CONCAT:
      case OPCODE_LIKE:
      case OPCODE_ILIKE:
      case OPCODE_MATCH:
      case OPCODE_IMATCH:
      case OPCODE_EQ:
      case OPCODE_NE:
      case OPCODE_LT:
      case OPCODE_LE:
      case OPCODE_GT:
      case OPCODE_GE:
      case OPCODE_AND:
      case OPCODE_OR:
      case OPCODE_BIT_AND:
      case OPCODE_BIT_OR:
      case OPCODE_BIT_XOR:
      case OPCODE_LSHIFT:
      case OPCODE_RSHIFT:
		case OPCODE_CASE:
         doBinaryOperation(cp->m_nOpCode);
         break;
      case OPCODE_NEG:
      case OPCODE_NOT:
      case OPCODE_BIT_NOT:
         doUnaryOperation(cp->m_nOpCode);
         break;
      case OPCODE_INC:  // Post increment/decrement
      case OPCODE_DEC:
         pVar = findOrCreateVariable(cp->m_operand.m_pszString);
         pValue = pVar->getValue();
         if (pValue->isNumeric())
         {
            m_pDataStack->push(new NXSL_Value(pValue));
            if (cp->m_nOpCode == OPCODE_INC)
               pValue->increment();
            else
               pValue->decrement();
         }
         else
         {
            error(NXSL_ERR_NOT_NUMBER);
         }
         break;
      case OPCODE_INCP: // Pre increment/decrement
      case OPCODE_DECP:
         pVar = findOrCreateVariable(cp->m_operand.m_pszString);
         pValue = pVar->getValue();
         if (pValue->isNumeric())
         {
            if (cp->m_nOpCode == OPCODE_INC)
               pValue->increment();
            else
               pValue->decrement();
            m_pDataStack->push(new NXSL_Value(pValue));
         }
         else
         {
            error(NXSL_ERR_NOT_NUMBER);
         }
         break;
      case OPCODE_GET_ATTRIBUTE:
         pValue = (NXSL_Value *)m_pDataStack->pop();
         if (pValue != NULL)
         {
            if (pValue->getDataType() == NXSL_DT_OBJECT)
            {
               NXSL_Object *pObj;
               NXSL_Value *pAttr;

               pObj = pValue->getValueAsObject();
               if (pObj != NULL)
               {
                  pAttr = pObj->getClass()->getAttr(pObj, cp->m_operand.m_pszString);
                  if (pAttr != NULL)
                  {
                     m_pDataStack->push(pAttr);
                  }
                  else
                  {
                     error(NXSL_ERR_NO_SUCH_ATTRIBUTE);
                  }
               }
               else
               {
                  error(NXSL_ERR_INTERNAL);
               }
            }
            else
            {
               error(NXSL_ERR_NOT_OBJECT);
            }
            delete pValue;
         }
         else
         {
            error(NXSL_ERR_DATA_STACK_UNDERFLOW);
         }
         break;
      case OPCODE_SET_ATTRIBUTE:
         pValue = (NXSL_Value *)m_pDataStack->pop();
         if (pValue != NULL)
         {
				NXSL_Value *pReference = (NXSL_Value *)m_pDataStack->pop();
				if (pReference != NULL)
				{
					if (pReference->getDataType() == NXSL_DT_OBJECT)
					{
						NXSL_Object *pObj;

						pObj = pReference->getValueAsObject();
						if (pObj != NULL)
						{
							if (pObj->getClass()->setAttr(pObj, cp->m_operand.m_pszString, pValue))
							{
								m_pDataStack->push(pValue);
								pValue = NULL;
							}
							else
							{
								error(NXSL_ERR_NO_SUCH_ATTRIBUTE);
							}
						}
						else
						{
							error(NXSL_ERR_INTERNAL);
						}
					}
					else
					{
						error(NXSL_ERR_NOT_OBJECT);
					}
					delete pReference;
				}
				else
				{
					error(NXSL_ERR_DATA_STACK_UNDERFLOW);
				}
				delete pValue;
         }
         else
         {
            error(NXSL_ERR_DATA_STACK_UNDERFLOW);
         }
         break;
		case OPCODE_FOREACH:
			nRet = NXSL_Iterator::createIterator(m_pDataStack);
			if (nRet != 0)
			{
				error(nRet);
			}
			break;
		case OPCODE_NEXT:
			pValue = (NXSL_Value *)m_pDataStack->peek();
			if (pValue != NULL)
			{
				if (pValue->isIterator())
				{
					NXSL_Iterator *it = pValue->getValueAsIterator();
					NXSL_Value *next = it->next();
					m_pDataStack->push(new NXSL_Value((LONG)((next != NULL) ? 1 : 0)));
					NXSL_Variable *var = findOrCreateVariable(it->getVariableName());
					if (!var->isConstant())
					{
						var->setValue((next != NULL) ? new NXSL_Value(next) : new NXSL_Value);
					}
					else
					{
						error(NXSL_ERR_ASSIGNMENT_TO_CONSTANT);
					}
				}
				else
				{
	            error(NXSL_ERR_NOT_ITERATOR);
				}
			}
			else
			{
            error(NXSL_ERR_DATA_STACK_UNDERFLOW);
			}
			break;
      default:
         break;
   }

   if (m_dwCurrPos != INVALID_ADDRESS)
      m_dwCurrPos = dwNext;
}


//
// Perform binary operation on two operands from stack and push result to stack
//

void NXSL_Program::doBinaryOperation(int nOpCode)
{
   NXSL_Value *pVal1, *pVal2, *pRes = NULL;
   const TCHAR *pszText1, *pszText2;
   DWORD dwLen1, dwLen2;
   int nType;
   LONG nResult;

	if (nOpCode == OPCODE_CASE)
	{
		pVal1 = m_ppInstructionSet[m_dwCurrPos]->m_operand.m_pConstant;
		pVal2 = (NXSL_Value *)m_pDataStack->peek();
	}
	else
	{
		pVal2 = (NXSL_Value *)m_pDataStack->pop();
		pVal1 = (NXSL_Value *)m_pDataStack->pop();
	}

   if ((pVal1 != NULL) && (pVal2 != NULL))
   {
      if ((!pVal1->isNull() && !pVal2->isNull()) ||
          (nOpCode == OPCODE_EQ) || (nOpCode == OPCODE_NE) || (nOpCode == OPCODE_CASE))
      {
         if (pVal1->isNumeric() && pVal2->isNumeric() &&
             (nOpCode != OPCODE_CONCAT) && (nOpCode != OPCODE_LIKE))
         {
            nType = SelectResultType(pVal1->getDataType(), pVal2->getDataType(), nOpCode);
            if (nType != NXSL_DT_NULL)
            {
               if ((pVal1->convert(nType)) && (pVal2->convert(nType)))
               {
                  switch(nOpCode)
                  {
                     case OPCODE_ADD:
                        pRes = pVal1;
                        pRes->add(pVal2);
                        delete pVal2;
                        break;
                     case OPCODE_SUB:
                        pRes = pVal1;
                        pRes->sub(pVal2);
                        delete pVal2;
                        break;
                     case OPCODE_MUL:
                        pRes = pVal1;
                        pRes->mul(pVal2);
                        delete pVal2;
                        break;
                     case OPCODE_DIV:
                        pRes = pVal1;
                        pRes->div(pVal2);
                        delete pVal2;
                        break;
                     case OPCODE_REM:
                        pRes = pVal1;
                        pRes->rem(pVal2);
                        delete pVal2;
                        break;
                     case OPCODE_EQ:
                     case OPCODE_NE:
                        nResult = pVal1->EQ(pVal2);
                        delete pVal1;
                        delete pVal2;
                        pRes = new NXSL_Value((nOpCode == OPCODE_EQ) ? nResult : !nResult);
                        break;
                     case OPCODE_LT:
                        nResult = pVal1->LT(pVal2);
                        delete pVal1;
                        delete pVal2;
                        pRes = new NXSL_Value(nResult);
                        break;
                     case OPCODE_LE:
                        nResult = pVal1->LE(pVal2);
                        delete pVal1;
                        delete pVal2;
                        pRes = new NXSL_Value(nResult);
                        break;
                     case OPCODE_GT:
                        nResult = pVal1->GT(pVal2);
                        delete pVal1;
                        delete pVal2;
                        pRes = new NXSL_Value(nResult);
                        break;
                     case OPCODE_GE:
                        nResult = pVal1->GE(pVal2);
                        delete pVal1;
                        delete pVal2;
                        pRes = new NXSL_Value(nResult);
                        break;
                     case OPCODE_LSHIFT:
                        pRes = pVal1;
                        pRes->lshift(pVal2->getValueAsInt32());
                        delete pVal2;
                        break;
                     case OPCODE_RSHIFT:
                        pRes = pVal1;
                        pRes->rshift(pVal2->getValueAsInt32());
                        delete pVal2;
                        break;
                     case OPCODE_BIT_AND:
                        pRes = pVal1;
                        pRes->bitAnd(pVal2);
                        delete pVal2;
                        break;
                     case OPCODE_BIT_OR:
                        pRes = pVal1;
                        pRes->bitOr(pVal2);
                        delete pVal2;
                        break;
                     case OPCODE_BIT_XOR:
                        pRes = pVal1;
                        pRes->bitXor(pVal2);
                        delete pVal2;
                        break;
                     case OPCODE_AND:
                        nResult = (pVal1->isNonZero() && pVal2->isNonZero());
                        delete pVal1;
                        delete pVal2;
                        pRes = new NXSL_Value(nResult);
                        break;
                     case OPCODE_OR:
                        nResult = (pVal1->isNonZero() || pVal2->isNonZero());
                        delete pVal1;
                        delete pVal2;
                        pRes = new NXSL_Value(nResult);
                        break;
                     case OPCODE_CASE:
                        pRes = new NXSL_Value((LONG)pVal1->EQ(pVal2));
                        break;
                     default:
                        error(NXSL_ERR_INTERNAL);
                        break;
                  }
               }
               else
               {
                  error(NXSL_ERR_TYPE_CAST);
               }
            }
            else
            {
               error(NXSL_ERR_REAL_VALUE);
            }
         }
         else
         {
            switch(nOpCode)
            {
               case OPCODE_EQ:
               case OPCODE_NE:
					case OPCODE_CASE:
                  if (pVal1->isNull() && pVal2->isNull())
                  {
                     nResult = 1;
                  }
                  else if (pVal1->isNull() || pVal2->isNull())
                  {
                     nResult = 0;
                  }
                  else
                  {
                     pszText1 = pVal1->getValueAsString(&dwLen1);
                     pszText2 = pVal2->getValueAsString(&dwLen2);
                     if (dwLen1 == dwLen2)
                        nResult = !memcmp(pszText1, pszText2, dwLen1 * sizeof(TCHAR));
                     else
                        nResult = 0;
                  }
						if (nOpCode != OPCODE_CASE)
						{
							delete pVal1;
							delete pVal2;
						}
                  pRes = new NXSL_Value((nOpCode == OPCODE_NE) ? !nResult : nResult);
                  break;
               case OPCODE_CONCAT:
                  if (pVal1->isNull() || pVal2->isNull())
                  {
                     error(NXSL_ERR_NULL_VALUE);
                  }
                  else if (pVal1->isObject() || pVal2->isObject())
                  {
                     error(NXSL_ERR_INVALID_OBJECT_OPERATION);
                  }
                  else
                  {
                     pRes = pVal1;
                     pszText2 = pVal2->getValueAsString(&dwLen2);
                     pRes->concatenate(pszText2, dwLen2);
                     delete pVal2;
                  }
                  break;
               case OPCODE_LIKE:
               case OPCODE_ILIKE:
                  if (pVal1->isString() && pVal2->isString())
                  {
                     pRes = new NXSL_Value((LONG)MatchString(pVal2->getValueAsCString(),
                                                             pVal1->getValueAsCString(),
                                                             nOpCode == OPCODE_LIKE));
                     delete pVal1;
                     delete pVal2;
                  }
                  else
                  {
                     error(NXSL_ERR_NOT_STRING);
                  }
                  break;
               case OPCODE_MATCH:
               case OPCODE_IMATCH:
                  if (pVal1->isString() && pVal2->isString())
                  {
                     pRes = matchRegexp(pVal1, pVal2, nOpCode == OPCODE_IMATCH);
                     delete pVal1;
                     delete pVal2;
                  }
                  else
                  {
                     error(NXSL_ERR_NOT_STRING);
                  }
                  break;
               case OPCODE_ADD:
               case OPCODE_SUB:
               case OPCODE_MUL:
               case OPCODE_DIV:
               case OPCODE_REM:
               case OPCODE_LT:
               case OPCODE_LE:
               case OPCODE_GT:
               case OPCODE_GE:
               case OPCODE_AND:
               case OPCODE_OR:
               case OPCODE_BIT_AND:
               case OPCODE_BIT_OR:
               case OPCODE_BIT_XOR:
               case OPCODE_LSHIFT:
               case OPCODE_RSHIFT:
                  error(NXSL_ERR_NOT_NUMBER);
                  break;
               default:
                  error(NXSL_ERR_INTERNAL);
                  break;
            }
         }
      }
      else
      {
         error(NXSL_ERR_NULL_VALUE);
      }
   }
   else
   {
      error(NXSL_ERR_DATA_STACK_UNDERFLOW);
   }

   if (pRes != NULL)
      m_pDataStack->push(pRes);
}


//
// Perform unary operation on operand from the stack and push result back to stack
//

void NXSL_Program::doUnaryOperation(int nOpCode)
{
   NXSL_Value *pVal;

   pVal = (NXSL_Value *)m_pDataStack->peek();
   if (pVal != NULL)
   {
      if (pVal->isNumeric())
      {
         switch(nOpCode)
         {
            case OPCODE_NEG:
               pVal->negate();
               break;
            case OPCODE_NOT:
               if (!pVal->isReal())
               {
                  pVal->set((LONG)pVal->isZero());
               }
               else
               {
                  error(NXSL_ERR_REAL_VALUE);
               }
               break;
            case OPCODE_BIT_NOT:
               if (!pVal->isReal())
               {
                  pVal->bitNot();
               }
               else
               {
                  error(NXSL_ERR_REAL_VALUE);
               }
               break;
            default:
               error(NXSL_ERR_INTERNAL);
               break;
         }
      }
      else
      {
         error(NXSL_ERR_NOT_NUMBER);
      }
   }
   else
   {
      error(NXSL_ERR_DATA_STACK_UNDERFLOW);
   }
}


//
// Relocate code block
//

void NXSL_Program::relocateCode(DWORD dwStart, DWORD dwLen, DWORD dwShift)
{
   DWORD i, dwLast;

   dwLast = min(dwStart + dwLen, m_dwCodeSize);
   for(i = dwStart; i < dwLast; i++)
      if ((m_ppInstructionSet[i]->m_nOpCode == OPCODE_JMP) ||
          (m_ppInstructionSet[i]->m_nOpCode == OPCODE_JZ) ||
          (m_ppInstructionSet[i]->m_nOpCode == OPCODE_JNZ) ||
          (m_ppInstructionSet[i]->m_nOpCode == OPCODE_CALL))
      {
         m_ppInstructionSet[i]->m_operand.m_dwAddr += dwShift;
      }
}


//
// Use external module
//

void NXSL_Program::useModule(NXSL_Program *pModule, const TCHAR *pszName)
{
   DWORD i, j, dwStart;

   // Check if module already loaded
   for(i = 0; i < m_dwNumModules; i++)
      if (!_tcsicmp(pszName, m_pModuleList[i].m_szName))
         return;  // Already loaded

   // Add code from module
   dwStart = m_dwCodeSize;
   m_dwCodeSize += pModule->m_dwCodeSize;
   m_ppInstructionSet = (NXSL_Instruction **)realloc(m_ppInstructionSet,
         sizeof(NXSL_Instruction *) * m_dwCodeSize);
   for(i = dwStart, j = 0; i < m_dwCodeSize; i++, j++)
      m_ppInstructionSet[i] = new NXSL_Instruction(pModule->m_ppInstructionSet[j]);
   relocateCode(dwStart, pModule->m_dwCodeSize, dwStart);
   
   // Add function names from module
   m_pFunctionList = (NXSL_Function *)realloc(m_pFunctionList,
         sizeof(NXSL_Function) * (m_dwNumFunctions + pModule->m_dwNumFunctions));
   memcpy(&m_pFunctionList[m_dwNumFunctions], pModule->m_pFunctionList,
          sizeof(NXSL_Function) * pModule->m_dwNumFunctions);
   for(i = m_dwNumFunctions, j = 0; j < pModule->m_dwNumFunctions; i++, j++)
      m_pFunctionList[i].m_dwAddr += dwStart;

   // Register module as loaded
   m_pModuleList = (NXSL_Module *)malloc(sizeof(NXSL_Module) * (m_dwNumModules + 1));
   nx_strncpy(m_pModuleList[m_dwNumModules].m_szName, pszName, MAX_PATH);
   m_pModuleList[m_dwNumModules].m_dwCodeStart = dwStart;
   m_pModuleList[m_dwNumModules].m_dwCodeSize = pModule->m_dwCodeSize;
   m_pModuleList[m_dwNumModules].m_dwFunctionStart = m_dwNumFunctions;
   m_pModuleList[m_dwNumModules].m_dwNumFunctions = pModule->m_dwNumFunctions;
   m_dwNumModules++;

   m_dwNumFunctions += pModule->m_dwNumFunctions;
}


//
// Call function at given address
//

void NXSL_Program::callFunction(int nArgCount)
{
   int i;
   NXSL_Value *pValue;
   TCHAR szBuffer[256];

   if (m_dwSubLevel < CONTROL_STACK_LIMIT)
   {
      m_dwSubLevel++;
      m_pCodeStack->push(CAST_TO_POINTER(m_dwCurrPos + 1, void *));
      m_pCodeStack->push(m_pLocals);
      m_pLocals = new NXSL_VariableSystem;
      m_nBindPos = 1;

      // Bind arguments
      for(i = nArgCount; i > 0; i--)
      {
         pValue = (NXSL_Value *)m_pDataStack->pop();
         if (pValue != NULL)
         {
            _sntprintf(szBuffer, 256, _T("$%d"), i);
            m_pLocals->create(szBuffer, pValue);
				if (pValue->getName() != NULL)
				{
					// Named parameter
					_sntprintf(szBuffer, 255, _T("$%s"), pValue->getName());
					szBuffer[255] = 0;
	            m_pLocals->create(szBuffer, new NXSL_Value(pValue));
				}
         }
         else
         {
            error(NXSL_ERR_DATA_STACK_UNDERFLOW);
            break;
         }
      }
   }
   else
   {
      error(NXSL_ERR_CONTROL_STACK_OVERFLOW);
   }
}


//
// Find function address by name
//

DWORD NXSL_Program::getFunctionAddress(const TCHAR *pszName)
{
   DWORD i;

   for(i = 0; i < m_dwNumFunctions; i++)
      if (!_tcscmp(m_pFunctionList[i].m_szName, pszName))
         return m_pFunctionList[i].m_dwAddr;
   return INVALID_ADDRESS;
}


//
// Match regular expression
//

NXSL_Value *NXSL_Program::matchRegexp(NXSL_Value *pValue, NXSL_Value *pRegexp, BOOL bIgnoreCase)
{
   regex_t preg;
   regmatch_t fields[256];
   NXSL_Value *pResult;
   NXSL_Variable *pVar;
   const TCHAR *regExp, *value;
	TCHAR szName[16];
	DWORD regExpLen, valueLen;
   int i;

	regExp = pRegexp->getValueAsString(&regExpLen);
   if (_tregncomp(&preg, regExp, regExpLen, bIgnoreCase ? REG_EXTENDED | REG_ICASE : REG_EXTENDED) == 0)
   {
		value = pValue->getValueAsString(&valueLen);
      if (_tregnexec(&preg, value, valueLen, 256, fields, 0) == 0)
      {
         pResult = new NXSL_Value((LONG)1);
         for(i = 1; (i < 256) && (fields[i].rm_so != -1); i++)
         {
            _sntprintf(szName, 16, _T("$%d"), i);
            pVar = m_pLocals->find(szName);
            if (pVar == NULL)
               m_pLocals->create(szName, new NXSL_Value(pValue->getValueAsCString() + fields[i].rm_so, fields[i].rm_eo - fields[i].rm_so));
            else
               pVar->setValue(new NXSL_Value(pValue->getValueAsCString() + fields[i].rm_so, fields[i].rm_eo - fields[i].rm_so));
         }
      }
      else
      {
         pResult = new NXSL_Value((LONG)0);
      }
      regfree(&preg);
   }
   else
   {
      error(NXSL_ERR_REGEXP_ERROR);
      pResult = NULL;
   }
   return pResult;
}


//
// Get final jump destination from a jump chain
//

DWORD NXSL_Program::getFinalJumpDestination(DWORD dwAddr)
{
	return (m_ppInstructionSet[dwAddr]->m_nOpCode == OPCODE_JMP) ? getFinalJumpDestination(m_ppInstructionSet[dwAddr]->m_operand.m_dwAddr) : dwAddr;
}


//
// Optimize compiled program
//

void NXSL_Program::optimize()
{
	DWORD i, j;

	// Convert jumps to address beyond code end to NRETs
	for(i = 0; i < m_dwCodeSize; i++)
	{
		if ((m_ppInstructionSet[i]->m_nOpCode == OPCODE_JMP) &&
		    (m_ppInstructionSet[i]->m_operand.m_dwAddr >= m_dwCodeSize))
		{
			m_ppInstructionSet[i]->m_nOpCode = OPCODE_RET_NULL;
		}
	}

	// Convert jump chains to single jump
	for(i = 0; i < m_dwCodeSize; i++)
	{
		if ((m_ppInstructionSet[i]->m_nOpCode == OPCODE_JMP) ||
			 (m_ppInstructionSet[i]->m_nOpCode == OPCODE_JZ) ||
			 (m_ppInstructionSet[i]->m_nOpCode == OPCODE_JNZ))
		{
			m_ppInstructionSet[i]->m_operand.m_dwAddr = getFinalJumpDestination(m_ppInstructionSet[i]->m_operand.m_dwAddr);
		}
	}

	// Remove jumps to next instruction
	for(i = 0; i < m_dwCodeSize; i++)
	{
		if (((m_ppInstructionSet[i]->m_nOpCode == OPCODE_JMP) ||
			  (m_ppInstructionSet[i]->m_nOpCode == OPCODE_JZ) ||
			  (m_ppInstructionSet[i]->m_nOpCode == OPCODE_JNZ)) &&
			 (m_ppInstructionSet[i]->m_operand.m_dwAddr == i + 1))
		{
			delete m_ppInstructionSet[i];
			m_dwCodeSize--;
			memmove(&m_ppInstructionSet[i], &m_ppInstructionSet[i + 1], sizeof(NXSL_Instruction *) * (m_dwCodeSize - i));

			// Change jump destination addresses
			for(j = 0; j < m_dwCodeSize; j++)
			{
				if (((m_ppInstructionSet[j]->m_nOpCode == OPCODE_JMP) ||
 				     (m_ppInstructionSet[j]->m_nOpCode == OPCODE_JZ) ||
				     (m_ppInstructionSet[j]->m_nOpCode == OPCODE_JNZ) ||
				     (m_ppInstructionSet[j]->m_nOpCode == OPCODE_CALL)) &&
				    (m_ppInstructionSet[j]->m_operand.m_dwAddr > i))
				{
		         m_ppInstructionSet[j]->m_operand.m_dwAddr--;
				}
			}

			i--;
		}
	}
}


//
// Trace
//

void NXSL_Program::trace(int level, const TCHAR *text)
{
	if (m_pEnv != NULL)
		m_pEnv->trace(level, text);
}
