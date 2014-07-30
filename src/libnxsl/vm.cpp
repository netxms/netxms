/* 
** NetXMS - Network Management System
** NetXMS Scripting Language Interpreter
** Copyright (C) 2003-2013 Victor Kirhenshtein
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
** File: vm.cpp
**
**/

#include "libnxsl.h"
#include <netxms-regex.h>

/**
 * Constants
 */
#define MAX_ERROR_NUMBER         32
#define CONTROL_STACK_LIMIT      32768

/**
 * Command mnemonics
 */
extern const char *g_nxslCommandMnemonic[];

/**
 * Error texts
 */
static const TCHAR *s_runtimeErrorMessage[MAX_ERROR_NUMBER] =
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
	_T("Named parameter required"),
	_T("Function or operation argument is not an iterator"),
	_T("Statistical data for given instance is not collected yet"),
	_T("Requested statistical parameter does not exist"),
	_T("Unknown object's method"),
   _T("Constant not defined"),
   _T("Execution aborted")
};

/**
 * Determine operation data type
 */
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

/**
 * Constructor
 */
NXSL_VM::NXSL_VM(NXSL_Environment *env)
{
   m_instructionSet = NULL;
   m_cp = INVALID_ADDRESS;
   m_dataStack = NULL;
   m_codeStack = NULL;
   m_catchStack = NULL;
   m_errorCode = 0;
   m_errorLine = 0;
   m_errorText = NULL;
   m_pConstants = new NXSL_VariableSystem(true);
   m_pGlobals = new NXSL_VariableSystem(false);
   m_pLocals = NULL;
   m_functions = NULL;
   m_modules = new ObjectArray<NXSL_Module>(4, 4, true);
   m_dwSubLevel = 0;    // Level of current subroutine
   m_env = (env != NULL) ? env : new NXSL_Environment;
   m_pRetValue = NULL;
	m_userData = NULL;

	srand((unsigned int)time(NULL));
}

/**
 * Destructor
 */
NXSL_VM::~NXSL_VM()
{
   delete m_instructionSet;

   delete m_dataStack;
   delete m_codeStack;
   delete m_catchStack;

   delete m_pConstants;
   delete m_pGlobals;
   delete m_pLocals;

   delete m_env;
   delete m_pRetValue;

   delete m_functions;
   delete m_modules;

   safe_free(m_errorText);
}

/**
 * Load program
 */
bool NXSL_VM::load(NXSL_Program *program)
{
   bool success = true;

   delete m_instructionSet;
   delete m_functions;
   delete m_modules;

   int i;

   // Copy instructions
   m_instructionSet = new ObjectArray<NXSL_Instruction>(program->m_instructionSet->size(), 32, true);
   for(i = 0; i < program->m_instructionSet->size(); i++)
      m_instructionSet->add(new NXSL_Instruction(program->m_instructionSet->get(i)));

   // Copy function information
   m_functions = new ObjectArray<NXSL_Function>(program->m_functions->size(), 8, true);
   for(i = 0; i < program->m_functions->size(); i++)
      m_functions->add(new NXSL_Function(program->m_functions->get(i)));

   // Set constants
   m_pConstants->clear();
   for(i = 0; i < (int)program->m_constants->getSize(); i++)
   {
      m_pConstants->create(program->m_constants->getKeyByIndex(i), new NXSL_Value(program->m_constants->getValueByIndex(i)));
   }

   // Load modules
   m_modules = new ObjectArray<NXSL_Module>(4, 4, true);
   for(i = 0; i < program->m_requiredModules.getSize(); i++)
   {
      if (!m_env->loadModule(this, program->m_requiredModules.getValue(i)))
      {
         error(NXSL_ERR_MODULE_NOT_FOUND);
         success = false;
         break;
      }
   }

   return success;
}

/**
 * Run program
 * Returns true on success and false on error
 */
bool NXSL_VM::run(UINT32 argc, NXSL_Value **argv,
                  NXSL_VariableSystem *pUserLocals, NXSL_VariableSystem **ppGlobals,
                  NXSL_VariableSystem *pConstants, const TCHAR *entryPoint)
{
   UINT32 i;
   NXSL_VariableSystem *pSavedGlobals, *pSavedConstants = NULL;
   NXSL_Value *pValue;
   TCHAR szBuffer[32];

	m_cp = INVALID_ADDRESS;

   // Delete previous return value
   delete_and_null(m_pRetValue);

   // Create stacks
   m_dataStack = new NXSL_Stack;
   m_codeStack = new NXSL_Stack;
   m_catchStack = new NXSL_Stack;

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

   // Locate entry point and run
   UINT32 entryAddr = INVALID_ADDRESS;
	if (entryPoint != NULL)
	{
      entryAddr = getFunctionAddress(entryPoint);
	}
	else
	{
      entryAddr = getFunctionAddress(_T("main"));

		// No explicit main(), search for implicit
		if (entryAddr == INVALID_ADDRESS)
		{
         entryAddr = getFunctionAddress(_T("$main"));
		}
	}

   if (entryAddr != INVALID_ADDRESS)
   {
      m_cp = entryAddr;
resume:
      while(m_cp < (UINT32)m_instructionSet->size())
         execute();
      if (m_cp != INVALID_ADDRESS)
      {
         m_pRetValue = (NXSL_Value *)m_dataStack->pop();
      }
      else if (m_catchStack->getSize() > 0)
      {
         if (unwind())
         {
            setGlobalVariable(_T("$errorcode"), new NXSL_Value(m_errorCode));
            setGlobalVariable(_T("$errorline"), new NXSL_Value(m_errorLine));
            setGlobalVariable(_T("$errortext"), new NXSL_Value(m_errorText));
            goto resume;
         }
      }
   }
   else
   {
      error(NXSL_ERR_NO_MAIN);
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
   while((pValue = (NXSL_Value *)m_dataStack->pop()) != NULL)
      delete pValue;
   
   while(m_dwSubLevel > 0)
   {
      m_dwSubLevel--;
      delete (NXSL_VariableSystem *)m_codeStack->pop();
      m_codeStack->pop();
   }
   
   NXSL_CatchPoint *p;
   while((p = (NXSL_CatchPoint *)m_catchStack->pop()) != NULL)
      delete p;
   
   delete_and_null(m_pLocals);
   delete_and_null(m_dataStack);
   delete_and_null(m_codeStack);
   delete_and_null(m_catchStack);

   return (m_cp != INVALID_ADDRESS);
}

/**
 * Unwind stack to nearest catch
 */
bool NXSL_VM::unwind()
{
   NXSL_CatchPoint *p = (NXSL_CatchPoint *)m_catchStack->pop();
   if (p == NULL)
      return false;

   while(m_dwSubLevel > p->subLevel)
   {
      m_dwSubLevel--;
      delete m_pLocals;
      m_pLocals = (NXSL_VariableSystem *)m_codeStack->pop();
      m_codeStack->pop();
   }

   while(m_dataStack->getSize() > p->dataStackSize)
      delete (NXSL_Value *)m_dataStack->pop();

   m_cp = p->addr;
   delete p;
   return true;
}

/**
 * Set global variale
 */
void NXSL_VM::setGlobalVariable(const TCHAR *pszName, NXSL_Value *pValue)
{
   NXSL_Variable *pVar;

	pVar = m_pGlobals->find(pszName);
   if (pVar == NULL)
		m_pGlobals->create(pszName, pValue);
	else
		pVar->setValue(pValue);
}

/**
 * Find variable
 */
NXSL_Variable *NXSL_VM::findVariable(const TCHAR *pszName)
{
   NXSL_Variable *pVar;

   pVar = m_pConstants->find(pszName);
   if (pVar == NULL)
   {
      pVar = m_pGlobals->find(pszName);
      if (pVar == NULL)
      {
         pVar = m_pLocals->find(pszName);
      }
   }
   return pVar;
}

/**
 * Find variable or create if does not exist
 */
NXSL_Variable *NXSL_VM::findOrCreateVariable(const TCHAR *pszName)
{
   NXSL_Variable *pVar = findVariable(pszName);
   if (pVar == NULL)
   {
      pVar = m_pLocals->create(pszName);
   }
   return pVar;
}

/**
 * Create variable if it does not exist, otherwise return NULL
 */
NXSL_Variable *NXSL_VM::createVariable(const TCHAR *pszName)
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

/**
 * Execute single instruction
 */
void NXSL_VM::execute()
{
   NXSL_Instruction *cp;
   NXSL_Value *pValue;
   NXSL_Variable *pVar;
   NXSL_ExtFunction *pFunc;
   UINT32 dwNext = m_cp + 1;
   TCHAR szBuffer[256];
   int i, nRet;

   cp = m_instructionSet->get(m_cp);
   switch(cp->m_nOpCode)
   {
      case OPCODE_PUSH_CONSTANT:
         if (cp->m_operand.m_pConstant->isArray())
            m_dataStack->push(new NXSL_Value(new NXSL_Array(cp->m_operand.m_pConstant->getValueAsArray())));
         else
            m_dataStack->push(new NXSL_Value(cp->m_operand.m_pConstant));
         break;
      case OPCODE_PUSH_VARIABLE:
         pVar = findOrCreateVariable(cp->m_operand.m_pszString);
         m_dataStack->push(new NXSL_Value(pVar->getValue()));
         break;
      case OPCODE_SET:
         pVar = findOrCreateVariable(cp->m_operand.m_pszString);
			if (!pVar->isConstant())
			{
				pValue = (NXSL_Value *)m_dataStack->peek();
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
			// Check if variable already exist
			pVar = findVariable(cp->m_operand.m_pszString);
			if (pVar != NULL)
			{
				// only raise error if variable with given name already exist
				// and is not an array
				if (!pVar->getValue()->isArray())
				{
					error(NXSL_ERR_VARIABLE_ALREADY_EXIST);
				}
			}
			else
			{
				pVar = createVariable(cp->m_operand.m_pszString);
				if (pVar != NULL)
				{
					pVar->setValue(new NXSL_Value(new NXSL_Array));
				}
				else
				{
					error(NXSL_ERR_VARIABLE_ALREADY_EXIST);
				}
			}
			break;
		case OPCODE_GLOBAL_ARRAY:
			// Check if variable already exist
			pVar = m_pGlobals->find(cp->m_operand.m_pszString);
			if (pVar == NULL)
			{
				// raise error if variable with given name already exist and is not global
				if (findVariable(cp->m_operand.m_pszString) != NULL)
				{
					error(NXSL_ERR_VARIABLE_ALREADY_EXIST);
				}
				else
				{
					m_pGlobals->create(cp->m_operand.m_pszString, new NXSL_Value(new NXSL_Array));
				}
			}
			else
			{
				if (!pVar->getValue()->isArray())
				{
					error(NXSL_ERR_VARIABLE_ALREADY_EXIST);
				}
			}
			break;
		case OPCODE_GLOBAL:
			// Check if variable already exist
			pVar = m_pGlobals->find(cp->m_operand.m_pszString);
			if (pVar == NULL)
			{
				// raise error if variable with given name already exist and is not global
				if (findVariable(cp->m_operand.m_pszString) != NULL)
				{
					error(NXSL_ERR_VARIABLE_ALREADY_EXIST);
				}
				else
				{
					if (cp->m_nStackItems > 0)	// with initialization
					{
						pValue = (NXSL_Value *)m_dataStack->pop();
						if (pValue != NULL)
						{
							m_pGlobals->create(cp->m_operand.m_pszString, pValue);
						}
						else
						{
			            error(NXSL_ERR_DATA_STACK_UNDERFLOW);
						}
					}
					else
					{
						m_pGlobals->create(cp->m_operand.m_pszString, new NXSL_Value);
					}
				}
			}
			break;
		case OPCODE_SET_ELEMENT:	// Set array element; stack should contain: array index value (top)
			pValue = (NXSL_Value *)m_dataStack->pop();
			if (pValue != NULL)
			{
				NXSL_Value *array, *index;

				index = (NXSL_Value *)m_dataStack->pop();
				array = (NXSL_Value *)m_dataStack->pop();
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
						m_dataStack->push(pValue);
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
		case OPCODE_INC_ELEMENT:	// Get array element and increment; stack should contain: array index (top)
		case OPCODE_DEC_ELEMENT:	// Get array element and decrement; stack should contain: array index (top)
		case OPCODE_INCP_ELEMENT:	// Increment array element and get; stack should contain: array index (top)
		case OPCODE_DECP_ELEMENT:	// Decrement array element and get; stack should contain: array index (top)
			pValue = (NXSL_Value *)m_dataStack->pop();
			if (pValue != NULL)
			{
				NXSL_Value *array;

				array = (NXSL_Value *)m_dataStack->pop();
				if (array != NULL)
				{
					if (array->isArray())
					{
						if (pValue->isInteger())
						{
                     int index = pValue->getValueAsInt32();
							NXSL_Value *element = array->getValueAsArray()->get(index);

                     if (cp->m_nOpCode == OPCODE_INCP_ELEMENT)
                     {
                        if (element->isNumeric())
                        {
                           element->increment();
                        }
                        else
                        {
                           error(NXSL_ERR_NOT_NUMBER);
                        }
                     }
                     else if (cp->m_nOpCode == OPCODE_DECP_ELEMENT)
                     {
                        if (element->isNumeric())
                        {
                           element->decrement();
                        }
                        else
                        {
                           error(NXSL_ERR_NOT_NUMBER);
                        }
                     }

                     m_dataStack->push((element != NULL) ? new NXSL_Value(element) : new NXSL_Value);

                     if (cp->m_nOpCode == OPCODE_INC_ELEMENT)
                     {
                        if (element->isNumeric())
                        {
                           element->increment();
                        }
                        else
                        {
                           error(NXSL_ERR_NOT_NUMBER);
                        }
                     }
                     else if (cp->m_nOpCode == OPCODE_DEC_ELEMENT)
                     {
                        if (element->isNumeric())
                        {
                           element->decrement();
                        }
                        else
                        {
                           error(NXSL_ERR_NOT_NUMBER);
                        }
                     }
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
		case OPCODE_ADD_TO_ARRAY:  // add element on stack top to array; stack should contain: array new_value (top)
         pValue = (NXSL_Value *)m_dataStack->pop();
         if (pValue != NULL)
         {
            NXSL_Value *array;

            array = (NXSL_Value *)m_dataStack->peek();
            if (array != NULL)
            {
               if (array->isArray())
               {
                  int index = array->getValueAsArray()->size();
                  array->getValueAsArray()->set(index, pValue);
                  pValue = NULL;    // Prevent deletion
               }
               else
               {
                  error(NXSL_ERR_NOT_ARRAY);
               }
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
         pValue = (NXSL_Value *)m_dataStack->peek();
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
         pValue = (NXSL_Value *)m_dataStack->peek();
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
            delete (NXSL_Value *)m_dataStack->pop();
         break;
      case OPCODE_JMP:
         dwNext = cp->m_operand.m_dwAddr;
         break;
      case OPCODE_JZ:
      case OPCODE_JNZ:
         pValue = (NXSL_Value *)m_dataStack->pop();
         if (pValue != NULL)
         {
            if (pValue->isNumeric())
            {
               if (cp->m_nOpCode == OPCODE_JZ ? pValue->isZero() : pValue->isNonZero())
                  dwNext = cp->m_operand.m_dwAddr;
            }
            else
            {
               error(NXSL_ERR_BAD_CONDITION);
            }
            delete pValue;
         }
         else
         {
            error(NXSL_ERR_DATA_STACK_UNDERFLOW);
         }
         break;
      case OPCODE_JZ_PEEK:
      case OPCODE_JNZ_PEEK:
			pValue = (NXSL_Value *)m_dataStack->peek();
         if (pValue != NULL)
         {
            if (pValue->isNumeric())
            {
					if (cp->m_nOpCode == OPCODE_JZ_PEEK ? pValue->isZero() : pValue->isNonZero())
                  dwNext = cp->m_operand.m_dwAddr;
            }
            else
            {
               error(NXSL_ERR_BAD_CONDITION);
            }
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
         pFunc = m_env->findFunction(cp->m_operand.m_pszString);
         if (pFunc != NULL)
         {
            if ((cp->m_nStackItems == pFunc->m_iNumArgs) ||
                (pFunc->m_iNumArgs == -1))
            {
               if (m_dataStack->getSize() >= cp->m_nStackItems)
               {
                  nRet = pFunc->m_pfHandler(cp->m_nStackItems,
                                            (NXSL_Value **)m_dataStack->peekList(cp->m_nStackItems),
                                            &pValue, this);
                  if (nRet == 0)
                  {
                     for(i = 0; i < cp->m_nStackItems; i++)
                        delete (NXSL_Value *)m_dataStack->pop();
                     m_dataStack->push(pValue);
                  }
                  else if (nRet == NXSL_STOP_SCRIPT_EXECUTION)
						{
                     m_dataStack->push(pValue);
			            dwNext = m_instructionSet->size();
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
            UINT32 dwAddr;

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
      case OPCODE_CALL_METHOD:
         pValue = (NXSL_Value *)m_dataStack->peekAt(cp->m_nStackItems + 1);
         if (pValue != NULL)
         {
            if (pValue->getDataType() == NXSL_DT_OBJECT)
            {
               NXSL_Object *pObj;
               NXSL_Value *pResult;

               pObj = pValue->getValueAsObject();
               if (pObj != NULL)
               {
                  nRet = pObj->getClass()->callMethod(cp->m_operand.m_pszString, pObj, cp->m_nStackItems,
                                                      (NXSL_Value **)m_dataStack->peekList(cp->m_nStackItems),
                                                      &pResult, this);
                  if (nRet == 0)
                  {
                     for(i = 0; i < cp->m_nStackItems + 1; i++)
                        delete (NXSL_Value *)m_dataStack->pop();
                     m_dataStack->push(pResult);
                  }
                  else if (nRet == NXSL_STOP_SCRIPT_EXECUTION)
					   {
                     m_dataStack->push(pResult);
		               dwNext = m_instructionSet->size();
					   }
					   else
                  {
                     // Execution error inside method
                     error(nRet);
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
         }
         else
         {
            error(NXSL_ERR_DATA_STACK_UNDERFLOW);
         }
         break;
      case OPCODE_RET_NULL:
         m_dataStack->push(new NXSL_Value);
      case OPCODE_RETURN:
         if (m_dwSubLevel > 0)
         {
            m_dwSubLevel--;
            delete m_pLocals;
            m_pLocals = (NXSL_VariableSystem *)m_codeStack->pop();
            dwNext = CAST_FROM_POINTER(m_codeStack->pop(), UINT32);
         }
         else
         {
            // Return from main(), terminate program
            dwNext = m_instructionSet->size();
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
         pValue = (NXSL_Value *)m_dataStack->pop();
         if (pValue != NULL)
         {
				m_env->print(pValue);
            delete pValue;
         }
         else
         {
            error(NXSL_ERR_DATA_STACK_UNDERFLOW);
         }
         break;
      case OPCODE_EXIT:
			if (m_dataStack->getSize() > 0)
         {
            dwNext = m_instructionSet->size();
         }
         else
         {
            error(NXSL_ERR_DATA_STACK_UNDERFLOW);
         }
         break;
      case OPCODE_ABORT:
			if (m_dataStack->getSize() > 0)
         {
            pValue = (NXSL_Value *)m_dataStack->pop();
            if (pValue->isInteger())
            {
               error(pValue->getValueAsInt32());
            }
            else if (pValue->isNull())
            {
               error(NXSL_ERR_EXECUTION_ABORTED);
            }
            else
            {
               error(NXSL_ERR_NOT_INTEGER);
            }
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
      case OPCODE_CASE_CONST:
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
            m_dataStack->push(new NXSL_Value(pValue));
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
            if (cp->m_nOpCode == OPCODE_INCP)
               pValue->increment();
            else
               pValue->decrement();
            m_dataStack->push(new NXSL_Value(pValue));
         }
         else
         {
            error(NXSL_ERR_NOT_NUMBER);
         }
         break;
      case OPCODE_GET_ATTRIBUTE:
		case OPCODE_SAFE_GET_ATTR:
         pValue = (NXSL_Value *)m_dataStack->pop();
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
                     m_dataStack->push(pAttr);
                  }
                  else
                  {
							if (cp->m_nOpCode == OPCODE_SAFE_GET_ATTR)
							{
	                     m_dataStack->push(new NXSL_Value);
							}
							else
							{
								error(NXSL_ERR_NO_SUCH_ATTRIBUTE);
							}
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
         pValue = (NXSL_Value *)m_dataStack->pop();
         if (pValue != NULL)
         {
				NXSL_Value *pReference = (NXSL_Value *)m_dataStack->pop();
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
								m_dataStack->push(pValue);
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
			nRet = NXSL_Iterator::createIterator(m_dataStack);
			if (nRet != 0)
			{
				error(nRet);
			}
			break;
		case OPCODE_NEXT:
			pValue = (NXSL_Value *)m_dataStack->peek();
			if (pValue != NULL)
			{
				if (pValue->isIterator())
				{
					NXSL_Iterator *it = pValue->getValueAsIterator();
					NXSL_Value *next = it->next();
					m_dataStack->push(new NXSL_Value((LONG)((next != NULL) ? 1 : 0)));
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
      case OPCODE_CATCH:
         {
            NXSL_CatchPoint *p = new NXSL_CatchPoint;
            p->addr = cp->m_operand.m_dwAddr;
            p->dataStackSize = m_dataStack->getSize();
            p->subLevel = m_dwSubLevel;
            m_catchStack->push(p);
         }
         break;
      default:
         break;
   }

   if (m_cp != INVALID_ADDRESS)
      m_cp = dwNext;
}

/**
 * Perform binary operation on two operands from stack and push result to stack
 */
void NXSL_VM::doBinaryOperation(int nOpCode)
{
   NXSL_Value *pVal1, *pVal2, *pRes = NULL;
   NXSL_Variable *var;
   const TCHAR *pszText1, *pszText2;
   UINT32 dwLen1, dwLen2;
   int nType;
   LONG nResult;
   bool dynamicValues = false;

   switch(nOpCode)
   {
      case OPCODE_CASE:
		   pVal1 = m_instructionSet->get(m_cp)->m_operand.m_pConstant;
		   pVal2 = (NXSL_Value *)m_dataStack->peek();
         break;
      case OPCODE_CASE_CONST:
         var = m_pConstants->find(m_instructionSet->get(m_cp)->m_operand.m_pszString);
         if (var != NULL)
         {
            pVal1 = var->getValue();
         }
         else
         {
            error(NXSL_ERR_NO_SUCH_CONSTANT);
            return;
         }
		   pVal2 = (NXSL_Value *)m_dataStack->peek();
         break;
      default:
		   pVal2 = (NXSL_Value *)m_dataStack->pop();
		   pVal1 = (NXSL_Value *)m_dataStack->pop();
         dynamicValues = true;
         break;
   }

   if ((pVal1 != NULL) && (pVal2 != NULL))
   {
      if ((!pVal1->isNull() && !pVal2->isNull()) ||
          (nOpCode == OPCODE_EQ) || (nOpCode == OPCODE_NE) || (nOpCode == OPCODE_CASE) || (nOpCode == OPCODE_CASE_CONST) || (nOpCode == OPCODE_CONCAT))
      {
         if (pVal1->isNumeric() && pVal2->isNumeric() &&
             (nOpCode != OPCODE_CONCAT) && 
             (nOpCode != OPCODE_LIKE) && (nOpCode != OPCODE_ILIKE) &&
             (nOpCode != OPCODE_MATCH) && (nOpCode != OPCODE_IMATCH))
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
                        pVal1 = NULL;
                        break;
                     case OPCODE_SUB:
                        pRes = pVal1;
                        pRes->sub(pVal2);
                        pVal1 = NULL;
                        break;
                     case OPCODE_MUL:
                        pRes = pVal1;
                        pRes->mul(pVal2);
                        pVal1 = NULL;
                        break;
                     case OPCODE_DIV:
                        pRes = pVal1;
                        pRes->div(pVal2);
                        pVal1 = NULL;
                        break;
                     case OPCODE_REM:
                        pRes = pVal1;
                        pRes->rem(pVal2);
                        pVal1 = NULL;
                        break;
                     case OPCODE_EQ:
                     case OPCODE_NE:
                        nResult = pVal1->EQ(pVal2);
                        pRes = new NXSL_Value((nOpCode == OPCODE_EQ) ? nResult : !nResult);
                        break;
                     case OPCODE_LT:
                        nResult = pVal1->LT(pVal2);
                        pRes = new NXSL_Value(nResult);
                        break;
                     case OPCODE_LE:
                        nResult = pVal1->LE(pVal2);
                        pRes = new NXSL_Value(nResult);
                        break;
                     case OPCODE_GT:
                        nResult = pVal1->GT(pVal2);
                        pRes = new NXSL_Value(nResult);
                        break;
                     case OPCODE_GE:
                        nResult = pVal1->GE(pVal2);
                        pRes = new NXSL_Value(nResult);
                        break;
                     case OPCODE_LSHIFT:
                        pRes = pVal1;
                        pRes->lshift(pVal2->getValueAsInt32());
                        pVal1 = NULL;
                        break;
                     case OPCODE_RSHIFT:
                        pRes = pVal1;
                        pRes->rshift(pVal2->getValueAsInt32());
                        pVal1 = NULL;
                        break;
                     case OPCODE_BIT_AND:
                        pRes = pVal1;
                        pRes->bitAnd(pVal2);
                        pVal1 = NULL;
                        break;
                     case OPCODE_BIT_OR:
                        pRes = pVal1;
                        pRes->bitOr(pVal2);
                        pVal1 = NULL;
                        break;
                     case OPCODE_BIT_XOR:
                        pRes = pVal1;
                        pRes->bitXor(pVal2);
                        pVal1 = NULL;
                        break;
                     case OPCODE_AND:
                        nResult = (pVal1->isNonZero() && pVal2->isNonZero());
                        pRes = new NXSL_Value(nResult);
                        break;
                     case OPCODE_OR:
                        nResult = (pVal1->isNonZero() || pVal2->isNonZero());
                        pRes = new NXSL_Value(nResult);
                        break;
                     case OPCODE_CASE:
                     case OPCODE_CASE_CONST:
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
					case OPCODE_CASE_CONST:
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
                  pRes = new NXSL_Value((nOpCode == OPCODE_NE) ? !nResult : nResult);
                  break;
               case OPCODE_CONCAT:
                  if (pVal1->isString())
                  {
                     pRes = pVal1;
                     pVal1 = NULL;
                  }
                  else
                  {
                     pszText1 = pVal2->getValueAsString(&dwLen1);
                     pRes = new NXSL_Value(pszText1, dwLen1);
                  }
                  pszText2 = pVal2->getValueAsString(&dwLen2);
                  pRes->concatenate(pszText2, dwLen2);
                  break;
               case OPCODE_LIKE:
               case OPCODE_ILIKE:
                  if (pVal1->isString() && pVal2->isString())
                  {
                     pRes = new NXSL_Value((LONG)MatchString(pVal2->getValueAsCString(),
                                                             pVal1->getValueAsCString(),
                                                             nOpCode == OPCODE_LIKE));
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

   if (dynamicValues)
   {
      delete pVal1;
      delete pVal2;
   }

   if (pRes != NULL)
      m_dataStack->push(pRes);
}

/**
 * Perform unary operation on operand from the stack and push result back to stack
 */
void NXSL_VM::doUnaryOperation(int nOpCode)
{
   NXSL_Value *pVal;

   pVal = (NXSL_Value *)m_dataStack->peek();
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

/**
 * Relocate code block
 */
void NXSL_VM::relocateCode(UINT32 dwStart, UINT32 dwLen, UINT32 dwShift)
{
   UINT32 dwLast = min(dwStart + dwLen, (UINT32)m_instructionSet->size());
   for(UINT32 i = dwStart; i < dwLast; i++)
	{
      NXSL_Instruction *instr = m_instructionSet->get(i);
      if ((instr->m_nOpCode == OPCODE_JMP) ||
          (instr->m_nOpCode == OPCODE_JZ) ||
          (instr->m_nOpCode == OPCODE_JNZ) ||
          (instr->m_nOpCode == OPCODE_JZ_PEEK) ||
          (instr->m_nOpCode == OPCODE_JNZ_PEEK) ||
          (instr->m_nOpCode == OPCODE_CALL))
      {
         instr->m_operand.m_dwAddr += dwShift;
      }
	}
}

/**
 * Use external module
 */
void NXSL_VM::loadModule(NXSL_Program *module, const TCHAR *name)
{
   int i;

   // Check if module already loaded
   for(i = 0; i < m_modules->size(); i++)
      if (!_tcsicmp(name, m_modules->get(i)->m_name))
         return;  // Already loaded

   // Add code from module
   int start = m_instructionSet->size();
   for(i = 0; i < module->m_instructionSet->size(); i++)
      m_instructionSet->add(new NXSL_Instruction(module->m_instructionSet->get(i)));
   relocateCode(start, module->m_instructionSet->size(), start);
   
   // Add function names from module
   for(i = 0; i < module->m_functions->size(); i++)
   {
      NXSL_Function *f = new NXSL_Function(module->m_functions->get(i));
      f->m_dwAddr += (UINT32)start;
      m_functions->add(f);
   }

   // Add constants from module
   m_pConstants->addAll(module->m_constants);

   // Register module as loaded
   NXSL_Module *m = new NXSL_Module;
   nx_strncpy(m->m_name, name, MAX_PATH);
   m->m_codeStart = (UINT32)start;
   m->m_codeSize = module->m_instructionSet->size();
   m->m_functionStart = m_functions->size() - module->m_functions->size();
   m->m_numFunctions = module->m_functions->size();
   m_modules->add(m);
}

/**
 * Call function at given address
 */
void NXSL_VM::callFunction(int nArgCount)
{
   int i;
   NXSL_Value *pValue;
   TCHAR szBuffer[256];

   if (m_dwSubLevel < CONTROL_STACK_LIMIT)
   {
      m_dwSubLevel++;
      m_codeStack->push(CAST_TO_POINTER(m_cp + 1, void *));
      m_codeStack->push(m_pLocals);
      m_pLocals = new NXSL_VariableSystem;
      m_nBindPos = 1;

      // Bind arguments
      for(i = nArgCount; i > 0; i--)
      {
         pValue = (NXSL_Value *)m_dataStack->pop();
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

/**
 * Find function address by name
 */
UINT32 NXSL_VM::getFunctionAddress(const TCHAR *pszName)
{
   for(int i = 0; i < m_functions->size(); i++)
   {
      NXSL_Function *f = m_functions->get(i);
      if (!_tcscmp(f->m_szName, pszName))
         return f->m_dwAddr;
   }
   return INVALID_ADDRESS;
}

/**
 * Match regular expression
 */
NXSL_Value *NXSL_VM::matchRegexp(NXSL_Value *pValue, NXSL_Value *pRegexp, BOOL bIgnoreCase)
{
   regex_t preg;
   regmatch_t fields[256];
   NXSL_Value *pResult;
   NXSL_Variable *pVar;
   const TCHAR *regExp, *value;
	TCHAR szName[16];
	UINT32 regExpLen, valueLen;
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

/**
 * Trace
 */
void NXSL_VM::trace(int level, const TCHAR *text)
{
	if (m_env != NULL)
		m_env->trace(level, text);
}

/**
 * Dump VM to file (as text)
 */
void NXSL_VM::dump(FILE *pFile)
{
   for(int i = 0; i < m_instructionSet->size(); i++)
   {
      NXSL_Instruction *instr = m_instructionSet->get(i);
      _ftprintf(pFile, _T("%04X  %-6hs  "), i, g_nxslCommandMnemonic[instr->m_nOpCode]);
      switch(instr->m_nOpCode)
      {
         case OPCODE_CALL_EXTERNAL:
         case OPCODE_GLOBAL:
            _ftprintf(pFile, _T("%s, %d\n"), instr->m_operand.m_pszString, instr->m_nStackItems);
            break;
         case OPCODE_CALL:
            _ftprintf(pFile, _T("%04X, %d\n"), instr->m_operand.m_dwAddr, instr->m_nStackItems);
            break;
         case OPCODE_CALL_METHOD:
            _ftprintf(pFile, _T("@%s, %d\n"), instr->m_operand.m_pszString, instr->m_nStackItems);
            break;
         case OPCODE_JMP:
         case OPCODE_JZ:
         case OPCODE_JNZ:
         case OPCODE_JZ_PEEK:
         case OPCODE_JNZ_PEEK:
            fprintf(pFile, "%04X\n", instr->m_operand.m_dwAddr);
            break;
         case OPCODE_PUSH_VARIABLE:
         case OPCODE_SET:
         case OPCODE_BIND:
         case OPCODE_ARRAY:
         case OPCODE_GLOBAL_ARRAY:
         case OPCODE_INC:
         case OPCODE_DEC:
         case OPCODE_INCP:
         case OPCODE_DECP:
         case OPCODE_INC_ELEMENT:
         case OPCODE_DEC_ELEMENT:
         case OPCODE_INCP_ELEMENT:
         case OPCODE_DECP_ELEMENT:
			case OPCODE_SAFE_GET_ATTR:
         case OPCODE_GET_ATTRIBUTE:
         case OPCODE_SET_ATTRIBUTE:
			case OPCODE_NAME:
         case OPCODE_CASE_CONST:
            _ftprintf(pFile, _T("%s\n"), instr->m_operand.m_pszString);
            break;
         case OPCODE_PUSH_CONSTANT:
			case OPCODE_CASE:
            if (instr->m_operand.m_pConstant->isNull())
               fprintf(pFile, "<null>\n");
            else if (instr->m_operand.m_pConstant->isArray())
               fprintf(pFile, "<array>\n");
            else
               _ftprintf(pFile, _T("\"%s\"\n"), instr->m_operand.m_pConstant->getValueAsCString());
            break;
         case OPCODE_POP:
            fprintf(pFile, "%d\n", instr->m_nStackItems);
            break;
         case OPCODE_CAST:
            _ftprintf(pFile, _T("[%s]\n"), g_szTypeNames[instr->m_nStackItems]);
            break;
         default:
            fprintf(pFile, "\n");
            break;
      }
   }
}

/**
 * Report error
 */
void NXSL_VM::error(int nError)
{
   TCHAR szBuffer[1024];

   m_errorCode = nError;
   m_errorLine = (m_cp == INVALID_ADDRESS) ? 0 : m_instructionSet->get(m_cp)->m_nSourceLine;
   safe_free(m_errorText);
   _sntprintf(szBuffer, 1024, _T("Error %d in line %d: %s"), nError, m_errorLine,
              ((nError > 0) && (nError <= MAX_ERROR_NUMBER)) ? s_runtimeErrorMessage[nError - 1] : _T("Unknown error code"));
   m_errorText = _tcsdup(szBuffer);
   m_cp = INVALID_ADDRESS;
}
