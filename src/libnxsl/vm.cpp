/* 
** NetXMS - Network Management System
** NetXMS Scripting Language Interpreter
** Copyright (C) 2003-2018 Victor Kirhenshtein
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
**/

#include "libnxsl.h"
#include <netxms-regex.h>

/**
 * Constants
 */
#define MAX_ERROR_NUMBER         38
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
   _T("Execution aborted"),
	_T("Attempt to use hash map element access operation on non hash map"),
   _T("Function or operation argument is not a container"),
   _T("Hash map key is not a string"),
   _T("Selector not found"),
   _T("Object constructor not found"),
   _T("Invalid number of object constructor's arguments")
};

/**
 * Get error message for given error code
 */
static const TCHAR *GetErrorMessage(int error)
{
   return ((error > 0) && (error <= MAX_ERROR_NUMBER)) ? s_runtimeErrorMessage[error - 1] : _T("Unknown error code");
}

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
         nType = std::max(nType1, nType2);
      }
   }
   return nType;
}

/**
 * Constructor
 */
NXSL_VM::NXSL_VM(NXSL_Environment *env, NXSL_Storage *storage) : NXSL_ValueManager()
{
   m_instructionSet = NULL;
   m_cp = INVALID_ADDRESS;
   m_dataStack = NULL;
   m_codeStack = NULL;
   m_catchStack = NULL;
   m_errorCode = 0;
   m_errorLine = 0;
   m_errorText = NULL;
   m_constants = new NXSL_VariableSystem(this, true);
   m_globals = new NXSL_VariableSystem(this, false);
   m_locals = NULL;
   m_functions = NULL;
   m_modules = new ObjectArray<NXSL_Module>(4, 4, true);
   m_dwSubLevel = 0;    // Level of current subroutine
   m_env = (env != NULL) ? env : new NXSL_Environment;
   m_pRetValue = NULL;
	m_userData = NULL;
	m_nBindPos = 0;
	if (storage != NULL)
	{
      m_localStorage = NULL;
	   m_storage = storage;
	}
	else
	{
      m_localStorage = new NXSL_LocalStorage(this);
      m_storage = m_localStorage;
	}
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

   delete m_constants;
   delete m_globals;
   delete m_locals;

   delete m_localStorage;

   delete m_env;
   destroyValue(m_pRetValue);

   delete m_functions;
   delete m_modules;

   free(m_errorText);
}

/**
 * Constant creation callback
 */
EnumerationCallbackResult NXSL_VM::createConstantsCallback(const void *key, const void *value, void *data)
{
   static_cast<NXSL_VM*>(data)->m_constants->create(*static_cast<const NXSL_Identifier*>(key),
            static_cast<NXSL_VM*>(data)->createValue(static_cast<const NXSL_Value*>(value)));
   return _CONTINUE;
}

/**
 * Load program
 */
bool NXSL_VM::load(const NXSL_Program *program)
{
   bool success = true;

   delete m_instructionSet;
   delete m_functions;
   delete m_modules;

   int i;

   // Copy instructions
   m_instructionSet = new ObjectArray<NXSL_Instruction>(program->m_instructionSet->size(), 32, true);
   for(i = 0; i < program->m_instructionSet->size(); i++)
      m_instructionSet->add(new NXSL_Instruction(this, program->m_instructionSet->get(i)));

   // Copy function information
   m_functions = new ObjectArray<NXSL_Function>(program->m_functions->size(), 8, true);
   for(i = 0; i < program->m_functions->size(); i++)
      m_functions->add(new NXSL_Function(program->m_functions->get(i)));

   // Set constants
   m_constants->clear();
   program->m_constants->forEach(createConstantsCallback, this);
   m_constants->create("NXSL::build", createValue(NETXMS_VERSION_BUILD_STRING));
   m_constants->create("NXSL::version", createValue(NETXMS_VERSION_STRING));

   // Load modules
   m_modules = new ObjectArray<NXSL_Module>(4, 4, true);
   for(i = 0; i < program->m_requiredModules->size(); i++)
   {
      const NXSL_ModuleImport *importInfo = program->m_requiredModules->get(i);
      if (!m_env->loadModule(this, importInfo))
      {
         error(NXSL_ERR_MODULE_NOT_FOUND, importInfo->lineNumber);
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
bool NXSL_VM::run(int argc, NXSL_Value **argv, NXSL_VariableSystem **ppGlobals,
                  NXSL_VariableSystem *pConstants, const char *entryPoint)
{
   ObjectRefArray<NXSL_Value> args(argc, 8);
   for(int i = 0; i < argc; i++)
      args.add(argv[i]);
   return run(&args, ppGlobals, pConstants, entryPoint);
}

/**
 * Run program
 * Returns true on success and false on error
 */
bool NXSL_VM::run(ObjectRefArray<NXSL_Value> *args, NXSL_VariableSystem **ppGlobals,
                  NXSL_VariableSystem *pConstants, const char *entryPoint)
{
	m_cp = INVALID_ADDRESS;

   // Delete previous return value
	destroyValue(m_pRetValue);
	m_pRetValue = NULL;

   // Create stacks
   m_dataStack = new NXSL_Stack;
   m_codeStack = new NXSL_Stack;
   m_catchStack = new NXSL_Stack;

   // Create local variable system for main() and bind arguments
   m_locals = new NXSL_VariableSystem(this);
   for(int i = 0; i < args->size(); i++)
   {
      char name[32];
      snprintf(name, 32, "$%d", i + 1);
      m_locals->create(name, args->get(i));
   }

   // Preserve original global variables and constants
   NXSL_VariableSystem *pSavedGlobals = new NXSL_VariableSystem(this, m_globals);
   NXSL_VariableSystem *pSavedConstants = new NXSL_VariableSystem(this, m_constants);
   if (pConstants != NULL)
      m_constants->merge(pConstants);

	m_env->configureVM(this);

   // Locate entry point and run
   UINT32 entryAddr = INVALID_ADDRESS;
	if (entryPoint != NULL)
	{
      entryAddr = getFunctionAddress(entryPoint);
	}
	else
	{
      entryAddr = getFunctionAddress("main");

		// No explicit main(), search for implicit
		if (entryAddr == INVALID_ADDRESS)
		{
         entryAddr = getFunctionAddress("$main");
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
            setGlobalVariable("$errorcode", createValue(m_errorCode));
            setGlobalVariable("$errorline", createValue(m_errorLine));
            setGlobalVariable("$errormsg", createValue(GetErrorMessage(m_errorCode)));
            setGlobalVariable("$errortext", createValue(m_errorText));
            goto resume;
         }
      }
   }
   else
   {
      error(NXSL_ERR_NO_MAIN);
   }

   // Restore instructions replaced to direct variable pointers
   m_locals->restoreVariableReferences(m_instructionSet);
   m_globals->restoreVariableReferences(m_instructionSet);
   m_constants->restoreVariableReferences(m_instructionSet);

   // Restore global variables
   if (ppGlobals == NULL)
	   delete m_globals;
	else
		*ppGlobals = m_globals;
   m_globals = pSavedGlobals;

	// Restore constants
	if (pSavedConstants != NULL)
	{
		delete m_constants;
		m_constants = pSavedConstants;
	}

   // Cleanup
	NXSL_Value *v;
   while((v = (NXSL_Value *)m_dataStack->pop()) != NULL)
      destroyValue(v);
   
   while(m_dwSubLevel > 0)
   {
      m_dwSubLevel--;
      delete (NXSL_VariableSystem *)m_codeStack->pop();
      m_codeStack->pop();
   }
   
   NXSL_CatchPoint *p;
   while((p = (NXSL_CatchPoint *)m_catchStack->pop()) != NULL)
      delete p;
   
   delete_and_null(m_locals);
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
      m_locals->restoreVariableReferences(m_instructionSet);
      delete m_locals;
      m_locals = (NXSL_VariableSystem *)m_codeStack->pop();
      m_codeStack->pop();
   }

   while(m_dataStack->getSize() > p->dataStackSize)
      destroyValue((NXSL_Value *)m_dataStack->pop());

   m_cp = p->addr;
   delete p;
   return true;
}

/**
 * Add constant to VM
 */
bool NXSL_VM::addConstant(const NXSL_Identifier& name, NXSL_Value *value)
{
   if (m_constants->find(name) != NULL)
   {
      destroyValue(value);
      return false;  // not added
   }
   m_constants->create(name, value);
   return true;
}

/**
 * Set global variale
 */
void NXSL_VM::setGlobalVariable(const NXSL_Identifier& name, NXSL_Value *pValue)
{
   NXSL_Variable *pVar = m_globals->find(name);
   if (pVar == NULL)
		m_globals->create(name, pValue);
	else
		pVar->setValue(pValue);
}

/**
 * Find variable
 */
NXSL_Variable *NXSL_VM::findVariable(const NXSL_Identifier& name, NXSL_VariableSystem **vs)
{
   NXSL_Variable *var = m_constants->find(name);
   if (var == NULL)
   {
      var = m_globals->find(name);
      if (var == NULL)
      {
         var = m_locals->find(name);
         if (vs != NULL)
            *vs = m_locals;
      }
      else if (vs != NULL)
      {
         *vs = m_globals;
      }
   }
   else if (vs != NULL)
   {
      *vs = m_constants;
   }
   return var;
}

/**
 * Find variable or create if does not exist
 */
NXSL_Variable *NXSL_VM::findOrCreateVariable(const NXSL_Identifier& name, NXSL_VariableSystem **vs)
{
   NXSL_Variable *var = findVariable(name, vs);
   if (var == NULL)
   {
      var = m_locals->create(name);
      if (vs != NULL)
         *vs = m_locals;
   }
   return var;
}

/**
 * Create variable if it does not exist, otherwise return NULL
 */
NXSL_Variable *NXSL_VM::createVariable(const NXSL_Identifier& name)
{
   NXSL_Variable *pVar = NULL;

   if (m_constants->find(name) == NULL)
   {
      if (m_globals->find(name) == NULL)
      {
         if (m_locals->find(name) == NULL)
         {
            pVar = m_locals->create(name);
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
   char varName[MAX_IDENTIFIER_LENGTH];
   int i, nRet;
   bool constructor;
   NXSL_VariableSystem *vs;

   cp = m_instructionSet->get(m_cp);
   switch(cp->m_nOpCode)
   {
      case OPCODE_PUSH_CONSTANT:
         m_dataStack->push(createValue(cp->m_operand.m_pConstant));
         break;
      case OPCODE_PUSH_VARIABLE:
         pVar = findOrCreateVariable(*cp->m_operand.m_identifier, &vs);
         m_dataStack->push(createValue(pVar->getValue()));
         // convert to direct variable access without name lookup
         if (vs->createVariableReferenceRestorePoint(m_cp, cp->m_operand.m_identifier))
         {
            cp->m_nOpCode = OPCODE_PUSH_VARPTR;
            cp->m_operand.m_variable = pVar;
         }
         break;
      case OPCODE_PUSH_VARPTR:
         m_dataStack->push(createValue(cp->m_operand.m_variable->getValue()));
         break;
      case OPCODE_PUSH_CONSTREF:
         pVar = m_constants->find(*cp->m_operand.m_identifier);
         if (pVar != NULL)
         {
            m_dataStack->push(createValue(pVar->getValue()));
            // convert to direct value access without name lookup
            if (m_constants->createVariableReferenceRestorePoint(m_cp, cp->m_operand.m_identifier))
            {
               cp->m_nOpCode = OPCODE_PUSH_VARPTR;
               cp->m_operand.m_variable = pVar;
            }
         }
         else
         {
            m_dataStack->push(createValue());
         }
         break;
      case OPCODE_NEW_ARRAY:
         m_dataStack->push(createValue(new NXSL_Array(this)));
         break;
      case OPCODE_NEW_HASHMAP:
         m_dataStack->push(createValue(new NXSL_HashMap(this)));
         break;
      case OPCODE_SET:
         pVar = findOrCreateVariable(*cp->m_operand.m_identifier, &vs);
			if (!pVar->isConstant())
			{
				pValue = (NXSL_Value *)m_dataStack->peek();
				if (pValue != NULL)
				{
					pVar->setValue(createValue(pValue));
               // convert to direct variable access without name lookup
		         if (vs->createVariableReferenceRestorePoint(m_cp, cp->m_operand.m_identifier))
		         {
                  cp->m_nOpCode = OPCODE_SET_VARPTR;
                  cp->m_operand.m_variable = pVar;
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
      case OPCODE_SET_VARPTR:
         pValue = (NXSL_Value *)m_dataStack->peek();
         if (pValue != NULL)
         {
            cp->m_operand.m_variable->setValue(createValue(pValue));
         }
         else
         {
            error(NXSL_ERR_DATA_STACK_UNDERFLOW);
         }
         break;
		case OPCODE_ARRAY:
			// Check if variable already exist
			pVar = findVariable(*cp->m_operand.m_identifier);
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
				pVar = createVariable(*cp->m_operand.m_identifier);
				if (pVar != NULL)
				{
					pVar->setValue(createValue(new NXSL_Array(this)));
				}
				else
				{
					error(NXSL_ERR_VARIABLE_ALREADY_EXIST);
				}
			}
			break;
		case OPCODE_GLOBAL_ARRAY:
			// Check if variable already exist
			pVar = m_globals->find(*cp->m_operand.m_identifier);
			if (pVar == NULL)
			{
				// raise error if variable with given name already exist and is not global
				if (findVariable(*cp->m_operand.m_identifier) != NULL)
				{
					error(NXSL_ERR_VARIABLE_ALREADY_EXIST);
				}
				else
				{
					m_globals->create(*cp->m_operand.m_identifier, createValue(new NXSL_Array(this)));
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
			pVar = m_globals->find(*cp->m_operand.m_identifier);
			if (pVar == NULL)
			{
				// raise error if variable with given name already exist and is not global
				if (findVariable(*cp->m_operand.m_identifier) != NULL)
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
							m_globals->create(*cp->m_operand.m_identifier, pValue);
						}
						else
						{
			            error(NXSL_ERR_DATA_STACK_UNDERFLOW);
						}
					}
					else
					{
						m_globals->create(*cp->m_operand.m_identifier, createValue());
					}
				}
			}
			break;
		case OPCODE_SET_ELEMENT:	// Set array or map element; stack should contain: array index value (top) / hashmap key value (top)
			pValue = (NXSL_Value *)m_dataStack->pop();
			if (pValue != NULL)
			{
				NXSL_Value *key = (NXSL_Value *)m_dataStack->pop();
				NXSL_Value *container = (NXSL_Value *)m_dataStack->pop();
				if ((key != NULL) && (container != NULL))
				{
               bool success;
               if (container->isArray())
               {
                  success = setArrayElement(container, key, pValue);
               }
               else if (container->isHashMap())
               {
                  success = setHashMapElement(container, key, pValue);
               }
               else
               {
						error(NXSL_ERR_NOT_CONTAINER);
                  success = false;
               }
               if (success)
               {
		            m_dataStack->push(pValue);
		            pValue = NULL;		// Prevent deletion
               }
				}
				else
				{
					error(NXSL_ERR_DATA_STACK_UNDERFLOW);
				}
				destroyValue(key);
				destroyValue(container);
				destroyValue(pValue);
			}
         else
         {
            error(NXSL_ERR_DATA_STACK_UNDERFLOW);
         }
			break;
		case OPCODE_GET_ELEMENT:	// Get array or map element; stack should contain: array index (top) (or hashmap key (top))
		case OPCODE_INC_ELEMENT:	// Get array or map  element and increment; stack should contain: array index (top)
		case OPCODE_DEC_ELEMENT:	// Get array or map  element and decrement; stack should contain: array index (top)
		case OPCODE_INCP_ELEMENT:	// Increment array or map  element and get; stack should contain: array index (top)
		case OPCODE_DECP_ELEMENT:	// Decrement array or map  element and get; stack should contain: array index (top)
			pValue = (NXSL_Value *)m_dataStack->pop();
			if (pValue != NULL)
			{
				NXSL_Value *container = (NXSL_Value *)m_dataStack->pop();
				if (container != NULL)
				{
					if (container->isArray())
					{
                  getOrUpdateArrayElement(cp->m_nOpCode, container, pValue);
					}
               else if (container->isHashMap())
               {
                  getOrUpdateHashMapElement(cp->m_nOpCode, container, pValue);
               }
					else
					{
						error(NXSL_ERR_NOT_CONTAINER);
					}
					destroyValue(container);
				}
				else
				{
					error(NXSL_ERR_DATA_STACK_UNDERFLOW);
				}
				destroyValue(pValue);
			}
         else
         {
            error(NXSL_ERR_DATA_STACK_UNDERFLOW);
         }
			break;
      case OPCODE_PEEK_ELEMENT:   // Get array or map element keeping array and index on stack; stack should contain: array index (top) (or hashmap key (top))
         pValue = (NXSL_Value *)m_dataStack->peek();
         if (pValue != NULL)
         {
            NXSL_Value *container = (NXSL_Value *)m_dataStack->peekAt(2);
            if (container != NULL)
            {
               if (container->isArray())
               {
                  getOrUpdateArrayElement(cp->m_nOpCode, container, pValue);
               }
               else if (container->isHashMap())
               {
                  getOrUpdateHashMapElement(cp->m_nOpCode, container, pValue);
               }
               else
               {
                  error(NXSL_ERR_NOT_CONTAINER);
               }
            }
            else
            {
               error(NXSL_ERR_DATA_STACK_UNDERFLOW);
            }
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
            NXSL_Value *array = (NXSL_Value *)m_dataStack->peek();
            if (array != NULL)
            {
               if (array->isArray())
               {
                  array->copyOnWrite();
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
            destroyValue(pValue);
         }
         else
         {
            error(NXSL_ERR_DATA_STACK_UNDERFLOW);
         }
         break;
		case OPCODE_HASHMAP_SET:  // set hash map entry from elements on stack top; stack should contain: hashmap key value (top)
         pValue = (NXSL_Value *)m_dataStack->pop();
         if (pValue != NULL)
         {
            NXSL_Value *key = (NXSL_Value *)m_dataStack->pop();
            if (key != NULL)
            {
               NXSL_Value *hashMap = (NXSL_Value *)m_dataStack->peek();
               if (hashMap != NULL)
               {
                  if (hashMap->isHashMap())
                  {
                     if (key->isString())
                     {
                        hashMap->getValueAsHashMap()->set(key->getValueAsCString(), pValue);
                        pValue = NULL;    // Prevent deletion
                     }
                     else
                     {
                        error(NXSL_ERR_KEY_NOT_STRING);
                     }
                  }
                  else
                  {
                     error(NXSL_ERR_NOT_HASHMAP);
                  }
               }
               else
               {
                  error(NXSL_ERR_DATA_STACK_UNDERFLOW);
               }
               destroyValue(key);
            }
            else
            {
               error(NXSL_ERR_DATA_STACK_UNDERFLOW);
            }
            destroyValue(pValue);
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
				pValue->setName(cp->m_operand.m_identifier->value);
         }
         else
         {
            error(NXSL_ERR_DATA_STACK_UNDERFLOW);
         }
			break;
      case OPCODE_POP:
         for(i = 0; i < cp->m_nStackItems; i++)
            destroyValue((NXSL_Value *)m_dataStack->pop());
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
            destroyValue(pValue);
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
         pFunc = m_env->findFunction(*cp->m_operand.m_identifier);
         if (pFunc != NULL)
         {
            // convert to direct call using pointer
            cp->m_nOpCode = OPCODE_CALL_EXTPTR;
            delete cp->m_operand.m_identifier;
            cp->m_operand.m_function = pFunc;

            if (callExternalFunction(pFunc, cp->m_nStackItems))
               dwNext = m_instructionSet->size();
         }
         else
         {
            UINT32 addr = getFunctionAddress(*cp->m_operand.m_identifier);
            if (addr != INVALID_ADDRESS)
            {
               // convert to CALL
               cp->m_nOpCode = OPCODE_CALL;
               delete cp->m_operand.m_identifier;
               cp->m_operand.m_dwAddr = addr;

               dwNext = addr;
               callFunction(cp->m_nStackItems);
            }
            else
            {
               constructor = !strncmp(cp->m_operand.m_identifier->value, "__new@", 6);
               error(constructor ? NXSL_ERR_NO_OBJECT_CONSTRUCTOR : NXSL_ERR_NO_FUNCTION);
            }
         }
         break;
      case OPCODE_CALL_EXTPTR:
         if (callExternalFunction(cp->m_operand.m_function, cp->m_nStackItems))
            dwNext = m_instructionSet->size();
         break;
      case OPCODE_CALL_METHOD:
         pValue = (NXSL_Value *)m_dataStack->peekAt(cp->m_nStackItems + 1);
         if (pValue != NULL)
         {
            if (pValue->getDataType() == NXSL_DT_OBJECT)
            {
               NXSL_Object *object = pValue->getValueAsObject();
               if (object != NULL)
               {
                  NXSL_Value *pResult;
                  nRet = object->getClass()->callMethod(*cp->m_operand.m_identifier, object, cp->m_nStackItems,
                                                        (NXSL_Value **)m_dataStack->peekList(cp->m_nStackItems),
                                                        &pResult, this);
                  if (nRet == 0)
                  {
                     for(i = 0; i < cp->m_nStackItems + 1; i++)
                        destroyValue((NXSL_Value *)m_dataStack->pop());
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
            else if (pValue->getDataType() == NXSL_DT_ARRAY)
            {
               NXSL_Array *array = pValue->getValueAsArray();
               NXSL_Value *result;
               nRet = array->callMethod(*cp->m_operand.m_identifier, cp->m_nStackItems,
                                        (NXSL_Value **)m_dataStack->peekList(cp->m_nStackItems),
                                        &result, this);
               if (nRet == 0)
               {
                  for(i = 0; i < cp->m_nStackItems + 1; i++)
                     destroyValue((NXSL_Value *)m_dataStack->pop());
                  m_dataStack->push(result);
               }
               else
               {
                  // Execution error inside method
                  error(nRet);
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
         m_dataStack->push(createValue());
         /* no break */
      case OPCODE_RETURN:
         if (m_dwSubLevel > 0)
         {
            m_dwSubLevel--;
            m_locals->restoreVariableReferences(m_instructionSet);
            delete m_locals;
            m_locals = (NXSL_VariableSystem *)m_codeStack->pop();
            dwNext = CAST_FROM_POINTER(m_codeStack->pop(), UINT32);
         }
         else
         {
            // Return from main(), terminate program
            dwNext = m_instructionSet->size();
         }
         break;
      case OPCODE_BIND:
         snprintf(varName, MAX_IDENTIFIER_LENGTH, "$%d", m_nBindPos++);
         pVar = m_locals->find(varName);
         pValue = (pVar != NULL) ? createValue(pVar->getValue()) : createValue();
         pVar = m_locals->find(*cp->m_operand.m_identifier);
         if (pVar == NULL)
            m_locals->create(*cp->m_operand.m_identifier, pValue);
         else
            pVar->setValue(pValue);
         break;
      case OPCODE_PRINT:
         pValue = (NXSL_Value *)m_dataStack->pop();
         if (pValue != NULL)
         {
				m_env->print(pValue);
            destroyValue(pValue);
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
            destroyValue(pValue);
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
      case OPCODE_IN:
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
         pVar = findOrCreateVariable(*cp->m_operand.m_identifier, &vs);
         pValue = pVar->getValue();
         if (pValue->isNumeric())
         {
            m_dataStack->push(createValue(pValue));
            if (cp->m_nOpCode == OPCODE_INC)
               pValue->increment();
            else
               pValue->decrement();

            // Convert to direct variable access
            if (vs->createVariableReferenceRestorePoint(m_cp, cp->m_operand.m_identifier))
            {
               cp->m_nOpCode = (cp->m_nOpCode == OPCODE_INC) ? OPCODE_INC_VARPTR : OPCODE_DEC_VARPTR;
               cp->m_operand.m_variable = pVar;
            }
         }
         else
         {
            error(NXSL_ERR_NOT_NUMBER);
         }
         break;
      case OPCODE_INC_VARPTR:  // Post increment/decrement
      case OPCODE_DEC_VARPTR:
         pValue = cp->m_operand.m_variable->getValue();
         if (pValue->isNumeric())
         {
            m_dataStack->push(createValue(pValue));
            if (cp->m_nOpCode == OPCODE_INC_VARPTR)
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
         pVar = findOrCreateVariable(*cp->m_operand.m_identifier, &vs);
         pValue = pVar->getValue();
         if (pValue->isNumeric())
         {
            if (cp->m_nOpCode == OPCODE_INCP)
               pValue->increment();
            else
               pValue->decrement();
            m_dataStack->push(createValue(pValue));

            // Convert to direct variable access
            if (vs->createVariableReferenceRestorePoint(m_cp, cp->m_operand.m_identifier))
            {
               cp->m_nOpCode = (cp->m_nOpCode == OPCODE_INCP) ? OPCODE_INCP_VARPTR : OPCODE_DECP_VARPTR;
               cp->m_operand.m_variable = pVar;
            }
         }
         else
         {
            error(NXSL_ERR_NOT_NUMBER);
         }
         break;
      case OPCODE_INCP_VARPTR: // Pre increment/decrement
      case OPCODE_DECP_VARPTR:
         pValue = cp->m_operand.m_variable->getValue();
         if (pValue->isNumeric())
         {
            if (cp->m_nOpCode == OPCODE_INCP_VARPTR)
               pValue->increment();
            else
               pValue->decrement();
            m_dataStack->push(createValue(pValue));
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
                  pAttr = pObj->getClass()->getAttr(pObj, cp->m_operand.m_identifier->value);
                  if (pAttr != NULL)
                  {
                     m_dataStack->push(pAttr);
                  }
                  else
                  {
							if (cp->m_nOpCode == OPCODE_SAFE_GET_ATTR)
							{
	                     m_dataStack->push(createValue());
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
            else if (pValue->getDataType() == NXSL_DT_ARRAY)
            {
               getArrayAttribute(pValue->getValueAsArray(), cp->m_operand.m_identifier->value, cp->m_nOpCode == OPCODE_SAFE_GET_ATTR);
            }
            else if (pValue->getDataType() == NXSL_DT_HASHMAP)
            {
               getHashMapAttribute(pValue->getValueAsHashMap(), cp->m_operand.m_identifier->value, cp->m_nOpCode == OPCODE_SAFE_GET_ATTR);
            }
            else
            {
               error(NXSL_ERR_NOT_OBJECT);
            }
            destroyValue(pValue);
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
						NXSL_Object *pObj = pReference->getValueAsObject();
						if (pObj != NULL)
						{
							if (pObj->getClass()->setAttr(pObj, cp->m_operand.m_identifier->value, pValue))
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
					destroyValue(pReference);
				}
				else
				{
					error(NXSL_ERR_DATA_STACK_UNDERFLOW);
				}
				destroyValue(pValue);
         }
         else
         {
            error(NXSL_ERR_DATA_STACK_UNDERFLOW);
         }
         break;
		case OPCODE_FOREACH:
			nRet = NXSL_Iterator::createIterator(this, m_dataStack);
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
					m_dataStack->push(createValue((LONG)((next != NULL) ? 1 : 0)));
					NXSL_Variable *var = findOrCreateVariable(it->getVariableName());
					if (!var->isConstant())
					{
						var->setValue((next != NULL) ? createValue(next) : createValue());
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
      case OPCODE_CPOP:
         {
            NXSL_CatchPoint *p = (NXSL_CatchPoint *)m_catchStack->pop();
            delete p;
         }
         break;
      case OPCODE_STORAGE_WRITE:   // Write to storage; stack should contain: name value (top)
         pValue = (NXSL_Value *)m_dataStack->pop();
         if (pValue != NULL)
         {
            NXSL_Value *name = (NXSL_Value *)m_dataStack->pop();
            if (name != NULL)
            {
               if (name->isString())
               {
                  m_storage->write(name->getValueAsCString(), createValue(pValue));
                  m_dataStack->push(pValue);
                  pValue = NULL;    // Prevent deletion
               }
               else
               {
                  error(NXSL_ERR_NOT_STRING);
               }
               destroyValue(name);
            }
            else
            {
               error(NXSL_ERR_DATA_STACK_UNDERFLOW);
            }
            destroyValue(pValue);
         }
         else
         {
            error(NXSL_ERR_DATA_STACK_UNDERFLOW);
         }
         break;
      case OPCODE_STORAGE_READ:   // Read from storage; stack should contain item name on top
         pValue = (cp->m_nStackItems > 0) ? (NXSL_Value *)m_dataStack->peek() : (NXSL_Value *)m_dataStack->pop();
         if (pValue != NULL)
         {
            if (pValue->isString())
            {
               m_dataStack->push(m_storage->read(pValue->getValueAsCString(), this));
            }
            else
            {
               error(NXSL_ERR_NOT_STRING);
            }
            if (cp->m_nStackItems == 0)
               destroyValue(pValue);
         }
         else
         {
            error(NXSL_ERR_DATA_STACK_UNDERFLOW);
         }
         break;
      case OPCODE_STORAGE_INC:  // Post increment/decrement for storage item
      case OPCODE_STORAGE_DEC:
         pValue = (NXSL_Value *)m_dataStack->pop();
         if (pValue != NULL)
         {
            if (pValue->isString())
            {
               NXSL_Value *sval = m_storage->read(pValue->getValueAsCString(), this);
               if (sval->isNumeric())
               {
                  m_dataStack->push(createValue(sval));
                  if (cp->m_nOpCode == OPCODE_STORAGE_INC)
                     sval->increment();
                  else
                     sval->decrement();
                  m_storage->write(pValue->getValueAsCString(), sval);
               }
               else
               {
                  error(NXSL_ERR_NOT_NUMBER);
                  destroyValue(sval);
               }
            }
            else
            {
               error(NXSL_ERR_NOT_STRING);
            }
            destroyValue(pValue);
         }
         else
         {
            error(NXSL_ERR_DATA_STACK_UNDERFLOW);
         }
         break;
      case OPCODE_STORAGE_INCP: // Pre increment/decrement for storage item
      case OPCODE_STORAGE_DECP:
         pValue = (NXSL_Value *)m_dataStack->pop();
         if (pValue != NULL)
         {
            if (pValue->isString())
            {
               NXSL_Value *sval = m_storage->read(pValue->getValueAsCString(), this);
               if (sval->isNumeric())
               {
                  if (cp->m_nOpCode == OPCODE_STORAGE_INCP)
                     sval->increment();
                  else
                     sval->decrement();
                  m_dataStack->push(createValue(sval));
                  m_storage->write(pValue->getValueAsCString(), sval);
               }
               else
               {
                  error(NXSL_ERR_NOT_NUMBER);
                  destroyValue(sval);
               }
            }
            else
            {
               error(NXSL_ERR_NOT_STRING);
            }
            destroyValue(pValue);
         }
         else
         {
            error(NXSL_ERR_DATA_STACK_UNDERFLOW);
         }
         break;
      case OPCODE_PUSHCP:
         m_dataStack->push(createValue((INT32)m_cp + cp->m_nStackItems));
         break;
      case OPCODE_SELECT:
         dwNext = callSelector(*cp->m_operand.m_identifier, cp->m_nStackItems);
         break;
      default:
         break;
   }

   if (m_cp != INVALID_ADDRESS)
      m_cp = dwNext;
}

/**
 * Set array element
 */
bool NXSL_VM::setArrayElement(NXSL_Value *array, NXSL_Value *index, NXSL_Value *value)
{
   bool success;
	if (index->isInteger())
	{
      // copy on write
      array->copyOnWrite();
		array->getValueAsArray()->set(index->getValueAsInt32(), createValue(value));
      success = true;
	}
   else
	{
		error(NXSL_ERR_INDEX_NOT_INTEGER);
      success = false;
	}
   return success;
}

/**
 * Get or update array element
 */
void NXSL_VM::getOrUpdateArrayElement(int opcode, NXSL_Value *array, NXSL_Value *index)
{
	if (index->isInteger())
	{
      if ((opcode != OPCODE_GET_ELEMENT) && (opcode != OPCODE_PEEK_ELEMENT))
         array->copyOnWrite();
		NXSL_Value *element = array->getValueAsArray()->get(index->getValueAsInt32());

      if (opcode == OPCODE_INCP_ELEMENT)
      {
         if ((element != NULL) && element->isNumeric())
         {
            element->increment();
         }
         else
         {
            error(NXSL_ERR_NOT_NUMBER);
         }
      }
      else if (opcode == OPCODE_DECP_ELEMENT)
      {
         if ((element != NULL) && element->isNumeric())
         {
            element->decrement();
         }
         else
         {
            error(NXSL_ERR_NOT_NUMBER);
         }
      }

      m_dataStack->push((element != NULL) ? createValue(element) : createValue());

      if (opcode == OPCODE_INC_ELEMENT)
      {
         if ((element != NULL) && element->isNumeric())
         {
            element->increment();
         }
         else
         {
            error(NXSL_ERR_NOT_NUMBER);
         }
      }
      else if (opcode == OPCODE_DEC_ELEMENT)
      {
         if ((element != NULL) && element->isNumeric())
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

/**
 * Set hash map element
 */
bool NXSL_VM::setHashMapElement(NXSL_Value *hashMap, NXSL_Value *key, NXSL_Value *value)
{
   bool success;
	if (key->isString())
	{
      hashMap->copyOnWrite();
		hashMap->getValueAsHashMap()->set(key->getValueAsCString(), createValue(value));
      success = true;
	}
   else
	{
		error(NXSL_ERR_KEY_NOT_STRING);
      success = false;
	}
   return success;
}

/**
 * Get or update hash map element
 */
void NXSL_VM::getOrUpdateHashMapElement(int opcode, NXSL_Value *hashMap, NXSL_Value *key)
{
	if (key->isString())
	{
      if ((opcode != OPCODE_GET_ELEMENT) && (opcode != OPCODE_PEEK_ELEMENT))
         hashMap->copyOnWrite();
		NXSL_Value *element = hashMap->getValueAsHashMap()->get(key->getValueAsCString());

      if (opcode == OPCODE_INCP_ELEMENT)
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
      else if (opcode == OPCODE_DECP_ELEMENT)
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

      m_dataStack->push((element != NULL) ? createValue(element) : createValue());

      if (opcode == OPCODE_INC_ELEMENT)
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
      else if (opcode == OPCODE_DEC_ELEMENT)
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
      error(NXSL_ERR_KEY_NOT_STRING);
	}
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
         var = m_constants->find(*(m_instructionSet->get(m_cp)->m_operand.m_identifier));
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
          (!pVal2->isNull() && (nOpCode == OPCODE_IN)) ||
          (nOpCode == OPCODE_EQ) || (nOpCode == OPCODE_NE) || (nOpCode == OPCODE_CASE) || (nOpCode == OPCODE_CASE_CONST) || (nOpCode == OPCODE_CONCAT))
      {
         if (pVal1->isNumeric() && pVal2->isNumeric() &&
             (nOpCode != OPCODE_CONCAT) && (nOpCode != OPCODE_IN) &&
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
                        pRes = createValue((nOpCode == OPCODE_EQ) ? nResult : !nResult);
                        break;
                     case OPCODE_LT:
                        nResult = pVal1->LT(pVal2);
                        pRes = createValue(nResult);
                        break;
                     case OPCODE_LE:
                        nResult = pVal1->LE(pVal2);
                        pRes = createValue(nResult);
                        break;
                     case OPCODE_GT:
                        nResult = pVal1->GT(pVal2);
                        pRes = createValue(nResult);
                        break;
                     case OPCODE_GE:
                        nResult = pVal1->GE(pVal2);
                        pRes = createValue(nResult);
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
                        pRes = createValue(nResult);
                        break;
                     case OPCODE_OR:
                        nResult = (pVal1->isNonZero() || pVal2->isNonZero());
                        pRes = createValue(nResult);
                        break;
                     case OPCODE_CASE:
                     case OPCODE_CASE_CONST:
                        pRes = createValue((LONG)pVal1->EQ(pVal2));
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
                  pRes = createValue((nOpCode == OPCODE_NE) ? !nResult : nResult);
                  break;
               case OPCODE_CONCAT:
                  if (pVal1->isString())
                  {
                     pRes = pVal1;
                     pVal1 = NULL;
                  }
                  else
                  {
                     pRes = createValue(_T(""));
                  }
                  pszText2 = pVal2->getValueAsString(&dwLen2);
                  pRes->concatenate(pszText2, dwLen2);
                  break;
               case OPCODE_LIKE:
               case OPCODE_ILIKE:
                  if (pVal1->isString() && pVal2->isString())
                  {
                     pRes = createValue((LONG)MatchString(pVal2->getValueAsCString(),
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
               case OPCODE_IN:
                  if (pVal2->isArray())
                  {
                     pRes = createValue(pVal2->getValueAsArray()->contains(pVal1));
                  }
                  else
                  {
                     error(NXSL_ERR_NOT_ARRAY);
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
      destroyValue(pVal1);
      destroyValue(pVal2);
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
   UINT32 dwLast = std::min(dwStart + dwLen, (UINT32)m_instructionSet->size());
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
void NXSL_VM::loadModule(NXSL_Program *module, const NXSL_ModuleImport *importInfo)
{
   int i;

   // Check if module already loaded
   for(i = 0; i < m_modules->size(); i++)
      if (!_tcsicmp(importInfo->name, m_modules->get(i)->m_name))
         return;  // Already loaded

   // Add code from module
   int start = m_instructionSet->size();
   for(i = 0; i < module->m_instructionSet->size(); i++)
      m_instructionSet->add(new NXSL_Instruction(this, module->m_instructionSet->get(i)));
   relocateCode(start, module->m_instructionSet->size(), start);
   
   // Add function names from module
   for(i = 0; i < module->m_functions->size(); i++)
   {
      NXSL_Function *f = new NXSL_Function(module->m_functions->get(i));
      f->m_dwAddr += (UINT32)start;
      m_functions->add(f);
   }

   // Add constants from module
   m_constants->addAll(module->m_constants);

   // Register module as loaded
   NXSL_Module *m = new NXSL_Module;
   nx_strncpy(m->m_name, importInfo->name, MAX_PATH);
   m->m_codeStart = (UINT32)start;
   m->m_codeSize = module->m_instructionSet->size();
   m->m_functionStart = m_functions->size() - module->m_functions->size();
   m->m_numFunctions = module->m_functions->size();
   m_modules->add(m);
}

/**
 * Call external function
 */
bool NXSL_VM::callExternalFunction(NXSL_ExtFunction *function, int stackItems)
{
   bool stopExecution = false;
   bool constructor = !strncmp(function->m_name, "__new@", 6);
   if ((stackItems == function->m_iNumArgs) || (function->m_iNumArgs == -1))
   {
      if (m_dataStack->getSize() >= stackItems)
      {
         NXSL_Value *result = NULL;
         int ret = function->m_pfHandler(stackItems,
                  (NXSL_Value **)m_dataStack->peekList(stackItems), &result, this);
         if (ret == 0)
         {
            for(int i = 0; i < stackItems; i++)
               destroyValue((NXSL_Value *)m_dataStack->pop());
            m_dataStack->push(result);
         }
         else if (ret == NXSL_STOP_SCRIPT_EXECUTION)
         {
            m_dataStack->push(result);
            stopExecution = true;
         }
         else
         {
            // Execution error inside function
            error(ret);
         }
      }
      else
      {
         error(NXSL_ERR_DATA_STACK_UNDERFLOW);
      }
   }
   else
   {
      error(constructor ? NXSL_ERR_INVALID_OC_ARG_COUNT : NXSL_ERR_INVALID_ARGUMENT_COUNT);
   }
   return stopExecution;
}

/**
 * Call function at given address
 */
void NXSL_VM::callFunction(int nArgCount)
{
   int i;
   NXSL_Value *pValue;
   char varName[MAX_IDENTIFIER_LENGTH];

   if (m_dwSubLevel < CONTROL_STACK_LIMIT)
   {
      m_dwSubLevel++;
      m_codeStack->push(CAST_TO_POINTER(m_cp + 1, void *));
      m_codeStack->push(m_locals);
      m_locals->restoreVariableReferences(m_instructionSet);
      m_locals = new NXSL_VariableSystem(this);
      m_nBindPos = 1;

      // Bind arguments
      for(i = nArgCount; i > 0; i--)
      {
         pValue = (NXSL_Value *)m_dataStack->pop();
         if (pValue != NULL)
         {
            snprintf(varName, MAX_IDENTIFIER_LENGTH, "$%d", i);
            m_locals->create(varName, pValue);
				if (pValue->getName() != NULL)
				{
					// Named parameter
					snprintf(varName, MAX_IDENTIFIER_LENGTH, "$%s", pValue->getName());
	            m_locals->create(varName, createValue(pValue));
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
UINT32 NXSL_VM::getFunctionAddress(const NXSL_Identifier& name)
{
   for(int i = 0; i < m_functions->size(); i++)
   {
      NXSL_Function *f = m_functions->get(i);
      if (name.equals(f->m_name))
         return f->m_dwAddr;
   }
   return INVALID_ADDRESS;
}

/**
 * Call selector
 */
UINT32 NXSL_VM::callSelector(const NXSL_Identifier& name, int numElements)
{
   NXSL_ExtSelector *selector = m_env->findSelector(name);
   if (selector == NULL)
   {
      error(NXSL_ERR_NO_SELECTOR);
      return 0;
   }

   int err, selection = -1;
   UINT32 addr = 0;
   NXSL_Value *options = NULL;
#if HAVE_ALLOCA
   UINT32 *addrList = (UINT32 *)alloca(sizeof(UINT32) * numElements);
#else
   UINT32 *addrList = (UINT32 *)malloc(sizeof(UINT32) * numElements);
#endif
   NXSL_Value **valueList = (NXSL_Value **)alloca(sizeof(NXSL_Value *) * numElements);
   memset(valueList, 0, sizeof(NXSL_Value *) * numElements);

   for(int i = numElements - 1; i >= 0; i--)
   {
      NXSL_Value *v = (NXSL_Value *)m_dataStack->pop();
      if (v == NULL)
      {
         error(NXSL_ERR_DATA_STACK_UNDERFLOW);
         goto cleanup;
      }

      if (!v->isInteger())
      {
         destroyValue(v);
         error(NXSL_ERR_INTERNAL);
         goto cleanup;
      }
      addrList[i] = v->getValueAsUInt32();
      destroyValue(v);

      valueList[i] = (NXSL_Value *)m_dataStack->pop();
      if (valueList[i] == NULL)
      {
         error(NXSL_ERR_DATA_STACK_UNDERFLOW);
         goto cleanup;
      }
   }

   options = (NXSL_Value *)m_dataStack->pop();
   if (options == NULL)
   {
      error(NXSL_ERR_DATA_STACK_UNDERFLOW);
      goto cleanup;
   }

   err = selector->m_handler(name, options, numElements, valueList, &selection, this);
   if (err == NXSL_ERR_SUCCESS)
   {
      if ((selection >= 0) && (selection < numElements))
      {
         addr = addrList[selection];
      }
      else
      {
         addr = m_cp + 1;
      }
   }
   else
   {
      error(err);
   }

cleanup:
   for(int j = 0; j < numElements; j++)
      destroyValue(valueList[j]);
   destroyValue(options);

#if !HAVE_ALLOCA
   free(addrList);
#endif

   return addr;
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
	char varName[16];
	UINT32 regExpLen, valueLen;
   int i;

	regExp = pRegexp->getValueAsString(&regExpLen);
   if (_tregncomp(&preg, regExp, regExpLen, bIgnoreCase ? REG_EXTENDED | REG_ICASE : REG_EXTENDED) == 0)
   {
		value = pValue->getValueAsString(&valueLen);
      if (_tregnexec(&preg, value, valueLen, 256, fields, 0) == 0)
      {
         pResult = createValue((LONG)1);
         for(i = 1; (i < 256) && (fields[i].rm_so != -1); i++)
         {
            snprintf(varName, 16, "$%d", i);
            pVar = m_locals->find(varName);
            if (pVar == NULL)
               m_locals->create(varName, createValue(pValue->getValueAsCString() + fields[i].rm_so, fields[i].rm_eo - fields[i].rm_so));
            else
               pVar->setValue(createValue(pValue->getValueAsCString() + fields[i].rm_so, fields[i].rm_eo - fields[i].rm_so));
         }
      }
      else
      {
         pResult = createValue((LONG)0);
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
            _ftprintf(pFile, _T("%hs, %d\n"), instr->m_operand.m_identifier->value, instr->m_nStackItems);
            break;
         case OPCODE_CALL:
            _ftprintf(pFile, _T("%04X, %d\n"), instr->m_operand.m_dwAddr, instr->m_nStackItems);
            break;
         case OPCODE_CALL_METHOD:
            _ftprintf(pFile, _T("@%hs, %d\n"), instr->m_operand.m_identifier->value, instr->m_nStackItems);
            break;
         case OPCODE_JMP:
         case OPCODE_JZ:
         case OPCODE_JNZ:
         case OPCODE_JZ_PEEK:
         case OPCODE_JNZ_PEEK:
            _ftprintf(pFile, _T("%04X\n"), instr->m_operand.m_dwAddr);
            break;
         case OPCODE_PUSH_CONSTREF:
         case OPCODE_PUSH_VARIABLE:
         case OPCODE_SET:
         case OPCODE_BIND:
         case OPCODE_ARRAY:
         case OPCODE_GLOBAL_ARRAY:
         case OPCODE_INC:
         case OPCODE_DEC:
         case OPCODE_INCP:
         case OPCODE_DECP:
			case OPCODE_SAFE_GET_ATTR:
         case OPCODE_GET_ATTRIBUTE:
         case OPCODE_SET_ATTRIBUTE:
			case OPCODE_NAME:
         case OPCODE_CASE_CONST:
            _ftprintf(pFile, _T("%hs\n"), instr->m_operand.m_identifier->value);
            break;
         case OPCODE_PUSH_CONSTANT:
			case OPCODE_CASE:
            if (instr->m_operand.m_pConstant->isNull())
               _ftprintf(pFile, _T("<null>\n"));
            else if (instr->m_operand.m_pConstant->isArray())
               _ftprintf(pFile, _T("<array>\n"));
            else if (instr->m_operand.m_pConstant->isHashMap())
               _ftprintf(pFile, _T("<hash map>\n"));
            else
               _ftprintf(pFile, _T("\"%s\"\n"), instr->m_operand.m_pConstant->getValueAsCString());
            break;
         case OPCODE_POP:
            _ftprintf(pFile, _T("%d\n"), instr->m_nStackItems);
            break;
         case OPCODE_CAST:
            _ftprintf(pFile, _T("[%s]\n"), g_szTypeNames[instr->m_nStackItems]);
            break;
         default:
            _ftprintf(pFile, _T("\n"));
            break;
      }
   }
}

/**
 * Report error
 */
void NXSL_VM::error(int errorCode, int sourceLine)
{
   TCHAR szBuffer[1024];

   m_errorCode = errorCode;
   m_errorLine = (sourceLine == -1) ? ((m_cp == INVALID_ADDRESS) ? 0 : m_instructionSet->get(m_cp)->m_nSourceLine) : sourceLine;
   safe_free(m_errorText);
   _sntprintf(szBuffer, 1024, _T("Error %d in line %d: %s"), errorCode, m_errorLine, GetErrorMessage(errorCode));
   m_errorText = _tcsdup(szBuffer);
   m_cp = INVALID_ADDRESS;
}

/**
 * Set persistent storage. Passing NULL will switch VM to local storage.
 */
void NXSL_VM::setStorage(NXSL_Storage *storage)
{
   if (storage != NULL)
   {
      m_storage = storage;
   }
   else
   {
      if (m_localStorage == NULL)
         m_localStorage = new NXSL_LocalStorage(this);
      m_storage = m_localStorage;
   }
}

/**
 * Get array's attribute
 */
void NXSL_VM::getArrayAttribute(NXSL_Array *a, const char *attribute, bool safe)
{
   if (!strcmp(attribute, "maxIndex"))
   {
      m_dataStack->push((a->size() > 0) ? createValue((INT32)a->getMaxIndex()) : createValue());
   }
   else if (!strcmp(attribute, "minIndex"))
   {
      m_dataStack->push((a->size() > 0) ? createValue((INT32)a->getMinIndex()) : createValue());
   }
   else if (!strcmp(attribute, "size"))
   {
      m_dataStack->push(createValue((INT32)a->size()));
   }
   else
   {
      if (safe)
         m_dataStack->push(createValue());
      else
         error(NXSL_ERR_NO_SUCH_ATTRIBUTE);
   }
}

/**
 * Get hash map's attribute
 */
void NXSL_VM::getHashMapAttribute(NXSL_HashMap *m, const char *attribute, bool safe)
{
   if (!strcmp(attribute, "keys"))
   {
      m_dataStack->push(m->getKeys());
   }
   else if (!strcmp(attribute, "size"))
   {
      m_dataStack->push(createValue((INT32)m->size()));
   }
   else if (!strcmp(attribute, "values"))
   {
      m_dataStack->push(m->getValues());
   }
   else
   {
      if (safe)
         m_dataStack->push(createValue());
      else
         error(NXSL_ERR_NO_SUCH_ATTRIBUTE);
   }
}
