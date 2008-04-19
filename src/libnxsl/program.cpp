/* 
** NetXMS - Network Management System
** NetXMS Scripting Language Interpreter
** Copyright (C) 2005, 2006, 2007 Victor Kirhenshtein
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
** File: program.cpp
**
**/

#include "libnxsl.h"
#ifdef _WIN32
#include <netxms-regex.h>
#else
#include <regex.h>
#endif


//
// Constants
//

#define MAX_ERROR_NUMBER         21
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
   "BITNOT", "CAST", "REF", "INCP", "DECP",
   "JNZ", "LIKE", "ILIKE", "MATCH",
   "IMATCH", "CASE"
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
	_T("Left argument of -> must be a reference to object"),
	_T("Unknown object's attribute"),
	_T("Requested module not found or cannot be loaded"),
	_T("Argument is not of string type and cannot be converted to string"),
	_T("Invalid regular expression"),
	_T("Function or operation argument is not a whole number"),
	_T("Invalid operation on object"),
	_T("Bad (or incompatible) object class")
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

NXSL_Program::NXSL_Program(void)
{
   m_ppInstructionSet = NULL;
   m_dwCodeSize = 0;
   m_dwCurrPos = INVALID_ADDRESS;
   m_pDataStack = NULL;
   m_pCodeStack = NULL;
   m_nErrorCode = 0;
   m_pszErrorText = NULL;
   m_pConstants = new NXSL_VariableSystem;
   m_pGlobals = new NXSL_VariableSystem;
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
}


//
// Destructor
//

NXSL_Program::~NXSL_Program(void)
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

void NXSL_Program::AddInstruction(NXSL_Instruction *pInstruction)
{
   m_ppInstructionSet = (NXSL_Instruction **)realloc(m_ppInstructionSet,
         sizeof(NXSL_Instruction *) * (m_dwCodeSize + 1));
   m_ppInstructionSet[m_dwCodeSize++] = pInstruction;
}


//
// Resolve last jump with INVALID_ADDRESS to current address
//

void NXSL_Program::ResolveLastJump(int nOpCode)
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

void NXSL_Program::CreateJumpAt(DWORD dwOpAddr, DWORD dwJumpAddr)
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
//

BOOL NXSL_Program::AddFunction(const char *pszName, DWORD dwAddr, char *pszError)
{
   DWORD i;

   // Check for duplicate function names
   for(i = 0; i < m_dwNumFunctions; i++)
      if (!strcmp(m_pFunctionList[i].m_szName, pszName))
      {
         sprintf(pszError, "Duplicate function name: \"%s\"", pszName);
         return FALSE;
      }
   m_dwNumFunctions++;
   m_pFunctionList = (NXSL_Function *)realloc(m_pFunctionList, sizeof(NXSL_Function) * m_dwNumFunctions);
   nx_strncpy(m_pFunctionList[i].m_szName, pszName, MAX_FUNCTION_NAME);
   m_pFunctionList[i].m_dwAddr = (dwAddr == INVALID_ADDRESS) ? m_dwCodeSize : dwAddr;
   return TRUE;
}


//
// Add preload information
//

void NXSL_Program::AddPreload(char *pszName)
{
   m_ppszPreloadList = (char **)realloc(m_ppszPreloadList, sizeof(char *) * (m_dwNumPreloads + 1));
   m_ppszPreloadList[m_dwNumPreloads] = pszName;
   m_dwNumPreloads++;
}


//
// resolve local functions
//

void NXSL_Program::ResolveFunctions(void)
{
   DWORD i, j;

   for(i = 0; i < m_dwCodeSize; i++)
   {
      if (m_ppInstructionSet[i]->m_nOpCode == OPCODE_CALL_EXTERNAL)
      {
         for(j = 0; j < m_dwNumFunctions; j++)
         {
            if (!strcmp(m_pFunctionList[j].m_szName,
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

void NXSL_Program::Dump(FILE *pFile)
{
   DWORD i;

   for(i = 0; i < m_dwCodeSize; i++)
   {
      fprintf(pFile, "%04X  %-6s  ", i,
              m_szCommandMnemonic[m_ppInstructionSet[i]->m_nOpCode]);
      switch(m_ppInstructionSet[i]->m_nOpCode)
      {
         case OPCODE_CALL_EXTERNAL:
            fprintf(pFile, "%s, %d\n", m_ppInstructionSet[i]->m_operand.m_pszString,
                    m_ppInstructionSet[i]->m_nStackItems);
            break;
         case OPCODE_CALL:
            fprintf(pFile, "%04X, %d\n", m_ppInstructionSet[i]->m_operand.m_dwAddr,
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
         case OPCODE_INC:
         case OPCODE_DEC:
         case OPCODE_INCP:
         case OPCODE_DECP:
         case OPCODE_REFERENCE:
            fprintf(pFile, "%s\n", m_ppInstructionSet[i]->m_operand.m_pszString);
            break;
         case OPCODE_PUSH_CONSTANT:
			case OPCODE_CASE:
            if (m_ppInstructionSet[i]->m_operand.m_pConstant->IsNull())
               fprintf(pFile, "<null>\n");
            else
               fprintf(pFile, "\"%s\"\n", 
                       m_ppInstructionSet[i]->m_operand.m_pConstant->GetValueAsCString());
            break;
         case OPCODE_POP:
            fprintf(pFile, "%d\n", m_ppInstructionSet[i]->m_nStackItems);
            break;
         case OPCODE_CAST:
            fprintf(pFile, "[%s]\n", g_szTypeNames[m_ppInstructionSet[i]->m_nStackItems]);
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

void NXSL_Program::Error(int nError)
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

int NXSL_Program::Run(NXSL_Environment *pEnv, DWORD argc, NXSL_Value **argv,
                      NXSL_VariableSystem *pUserLocals, NXSL_VariableSystem **ppGlobals)
{
   DWORD i, dwOrigCodeSize, dwOrigNumFn;
   NXSL_VariableSystem *pSavedGlobals;
   NXSL_Value *pValue;
   char szBuffer[32];

   // Save original code size and number of functions
   dwOrigCodeSize = m_dwCodeSize;
   dwOrigNumFn = m_dwNumFunctions;

   // Delete previous return value
   delete m_pRetValue;
   m_pRetValue = NULL;

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
      sprintf(szBuffer, "$%d", i + 1);
      m_pLocals->Create(szBuffer, argv[i]);
   }

   // Preserve original global variables
   pSavedGlobals = new NXSL_VariableSystem(m_pGlobals);

   // Preload modules
   for(i = 0; i < m_dwNumPreloads; i++)
   {
      if (!pEnv->UseModule(this, m_ppszPreloadList[i]))
      {
         Error(NXSL_ERR_MODULE_NOT_FOUND);
         break;
      }
   }

   // Locate main() and run
   if (i == m_dwNumPreloads)
   {
      for(i = 0; i < m_dwNumFunctions; i++)
         if (!strcmp(m_pFunctionList[i].m_szName, "main"))
            break;
      if (i < m_dwNumFunctions)
      {
         m_dwCurrPos = m_pFunctionList[i].m_dwAddr;
         while(m_dwCurrPos < m_dwCodeSize)
            Execute();
         if (m_dwCurrPos != INVALID_ADDRESS)
            m_pRetValue = (NXSL_Value *)m_pDataStack->Pop();
      }
      else
      {
         Error(NXSL_ERR_NO_MAIN);
      }
   }

   // Restore global variables
   if (ppGlobals == NULL)
	   delete m_pGlobals;
	else
		*ppGlobals = m_pGlobals;
   m_pGlobals = pSavedGlobals;

   // Cleanup
   while((pValue = (NXSL_Value *)m_pDataStack->Pop()) != NULL)
      delete pValue;
   while(m_dwSubLevel > 0)
   {
      m_dwSubLevel--;
      delete (NXSL_VariableSystem *)m_pCodeStack->Pop();
      m_pCodeStack->Pop();
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

void NXSL_Program::SetGlobalVariable(const TCHAR *pszName, NXSL_Value *pValue)
{
   NXSL_Variable *pVar;

	pVar = m_pGlobals->Find(pszName);
   if (pVar == NULL)
		pVar = m_pGlobals->Create(pszName, pValue);
	else
		pVar->Set(pValue);
}


//
// Find variable or create if does not exist
//

NXSL_Variable *NXSL_Program::FindOrCreateVariable(TCHAR *pszName)
{
   NXSL_Variable *pVar;

   pVar = m_pConstants->Find(pszName);
   if (pVar == NULL)
   {
      pVar = m_pGlobals->Find(pszName);
      if (pVar == NULL)
      {
         pVar = m_pLocals->Find(pszName);
         if (pVar == NULL)
         {
            pVar = m_pLocals->Create(pszName);
         }
      }
   }
   return pVar;
}


//
// Execute single instruction
//

void NXSL_Program::Execute(void)
{
   NXSL_Instruction *cp;
   NXSL_Value *pValue;
   NXSL_Variable *pVar;
   NXSL_ExtFunction *pFunc;
   DWORD dwNext = m_dwCurrPos + 1;
   char szBuffer[256];
   int i, nRet;

   cp = m_ppInstructionSet[m_dwCurrPos];
   switch(cp->m_nOpCode)
   {
      case OPCODE_PUSH_CONSTANT:
         m_pDataStack->Push(new NXSL_Value(cp->m_operand.m_pConstant));
         break;
      case OPCODE_PUSH_VARIABLE:
         pVar = FindOrCreateVariable(cp->m_operand.m_pszString);
         m_pDataStack->Push(new NXSL_Value(pVar->Value()));
         break;
      case OPCODE_SET:
         pVar = FindOrCreateVariable(cp->m_operand.m_pszString);
         pValue = (NXSL_Value *)m_pDataStack->Peek();
         if (pValue != NULL)
         {
            pVar->Set(new NXSL_Value(pValue));
         }
         else
         {
            Error(NXSL_ERR_DATA_STACK_UNDERFLOW);
         }
         break;
      case OPCODE_CAST:
         pValue = (NXSL_Value *)m_pDataStack->Peek();
         if (pValue != NULL)
         {
            if (!pValue->Convert(cp->m_nStackItems))
            {
               Error(NXSL_ERR_TYPE_CAST);
            }
         }
         else
         {
            Error(NXSL_ERR_DATA_STACK_UNDERFLOW);
         }
         break;
      case OPCODE_POP:
         for(i = 0; i < cp->m_nStackItems; i++)
            delete (NXSL_Value *)m_pDataStack->Pop();
         break;
      case OPCODE_JMP:
         dwNext = cp->m_operand.m_dwAddr;
         break;
      case OPCODE_JZ:
      case OPCODE_JNZ:
         pValue = (NXSL_Value *)m_pDataStack->Pop();
         if (pValue != NULL)
         {
            if (pValue->IsNumeric())
            {
               if (cp->m_nOpCode == OPCODE_JZ ? pValue->IsZero() : pValue->IsNonZero())
                  dwNext = cp->m_operand.m_dwAddr;
            }
            else
            {
               Error(3);
            }
            delete pValue;
         }
         else
         {
            Error(NXSL_ERR_DATA_STACK_UNDERFLOW);
         }
         break;
      case OPCODE_CALL:
         dwNext = cp->m_operand.m_dwAddr;
         CallFunction(cp->m_nStackItems);
         break;
      case OPCODE_CALL_EXTERNAL:
         pFunc = m_pEnv->FindFunction(cp->m_operand.m_pszString);
         if (pFunc != NULL)
         {
            if ((cp->m_nStackItems == pFunc->m_iNumArgs) ||
                (pFunc->m_iNumArgs == -1))
            {
               if (m_pDataStack->Size() >= cp->m_nStackItems)
               {
                  nRet = pFunc->m_pfHandler(cp->m_nStackItems,
                                            (NXSL_Value **)m_pDataStack->PeekList(cp->m_nStackItems),
                                            &pValue);
                  if (nRet == 0)
                  {
                     for(i = 0; i < cp->m_nStackItems; i++)
                        m_pDataStack->Pop();
                     m_pDataStack->Push(pValue);
                  }
                  else
                  {
                     // Execution error inside function
                     Error(nRet);
                  }
               }
               else
               {
                  Error(NXSL_ERR_DATA_STACK_UNDERFLOW);
               }
            }
            else
            {
               Error(NXSL_ERR_INVALID_ARGUMENT_COUNT);
            }
         }
         else
         {
            DWORD dwAddr;

            dwAddr = GetFunctionAddress(cp->m_operand.m_pszString);
            if (dwAddr != INVALID_ADDRESS)
            {
               dwNext = dwAddr;
               CallFunction(cp->m_nStackItems);
            }
            else
            {
               Error(NXSL_ERR_NO_FUNCTION);
            }
         }
         break;
      case OPCODE_RET_NULL:
         m_pDataStack->Push(new NXSL_Value);
      case OPCODE_RETURN:
         if (m_dwSubLevel > 0)
         {
            m_dwSubLevel--;
            delete m_pLocals;
            m_pLocals = (NXSL_VariableSystem *)m_pCodeStack->Pop();
            dwNext = CAST_FROM_POINTER(m_pCodeStack->Pop(), DWORD);
         }
         else
         {
            // Return from main(), terminate program
            dwNext = m_dwCodeSize;
         }
         break;
      case OPCODE_BIND:
         sprintf(szBuffer, "$%d", m_nBindPos++);
         pVar = m_pLocals->Find(szBuffer);
         pValue = (pVar != NULL) ? new NXSL_Value(pVar->Value()) : new NXSL_Value;
         pVar = m_pLocals->Find(cp->m_operand.m_pszString);
         if (pVar == NULL)
            m_pLocals->Create(cp->m_operand.m_pszString, pValue);
         else
            pVar->Set(pValue);
         break;
      case OPCODE_PRINT:
         pValue = (NXSL_Value *)m_pDataStack->Pop();
         if (pValue != NULL)
         {
            char *pszText;
            DWORD dwLen;

            if (m_pEnv->GetStdOut() != NULL)
            {
               pszText = pValue->GetValueAsString(&dwLen);
               if (pszText != NULL)
                  fwrite(pszText, dwLen, 1, m_pEnv->GetStdOut());
               else
                  fputs("(null)", m_pEnv->GetStdOut());
            }
            delete pValue;
         }
         else
         {
            Error(NXSL_ERR_DATA_STACK_UNDERFLOW);
         }
         break;
      case OPCODE_EXIT:
         pValue = (NXSL_Value *)m_pDataStack->Pop();
         if (pValue != NULL)
         {
            dwNext = m_dwCodeSize;
            delete pValue;
         }
         else
         {
            Error(NXSL_ERR_DATA_STACK_UNDERFLOW);
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
         DoBinaryOperation(cp->m_nOpCode);
         break;
      case OPCODE_NEG:
      case OPCODE_NOT:
      case OPCODE_BIT_NOT:
         DoUnaryOperation(cp->m_nOpCode);
         break;
      case OPCODE_INC:  // Post increment/decrement
      case OPCODE_DEC:
         pVar = FindOrCreateVariable(cp->m_operand.m_pszString);
         pValue = pVar->Value();
         if (pValue->IsNumeric())
         {
            m_pDataStack->Push(new NXSL_Value(pValue));
            if (cp->m_nOpCode == OPCODE_INC)
               pValue->Increment();
            else
               pValue->Decrement();
         }
         else
         {
            Error(NXSL_ERR_NOT_NUMBER);
         }
         break;
      case OPCODE_INCP: // Pre increment/decrement
      case OPCODE_DECP:
         pVar = FindOrCreateVariable(cp->m_operand.m_pszString);
         pValue = pVar->Value();
         if (pValue->IsNumeric())
         {
            if (cp->m_nOpCode == OPCODE_INC)
               pValue->Increment();
            else
               pValue->Decrement();
            m_pDataStack->Push(new NXSL_Value(pValue));
         }
         else
         {
            Error(NXSL_ERR_NOT_NUMBER);
         }
         break;
      case OPCODE_REFERENCE:
         pValue = (NXSL_Value *)m_pDataStack->Pop();
         if (pValue != NULL)
         {
            if (pValue->DataType() == NXSL_DT_OBJECT)
            {
               NXSL_Object *pObj;
               NXSL_Value *pAttr;

               pObj = pValue->GetValueAsObject();
               if (pObj != NULL)
               {
                  pAttr = pObj->Class()->GetAttr(pObj, cp->m_operand.m_pszString);
                  if (pAttr != NULL)
                  {
                     m_pDataStack->Push(pAttr);
                  }
                  else
                  {
                     Error(NXSL_ERR_NO_SUCH_ATTRIBUTE);
                  }
               }
               else
               {
                  Error(NXSL_ERR_INTERNAL);
               }
            }
            else
            {
               Error(NXSL_ERR_NOT_OBJECT);
            }
            delete pValue;
         }
         else
         {
            Error(NXSL_ERR_DATA_STACK_UNDERFLOW);
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

void NXSL_Program::DoBinaryOperation(int nOpCode)
{
   NXSL_Value *pVal1, *pVal2, *pRes = NULL;
   char *pszText1, *pszText2;
   DWORD dwLen1, dwLen2;
   int nType;
   LONG nResult;

	if (nOpCode == OPCODE_CASE)
	{
		pVal1 = m_ppInstructionSet[m_dwCurrPos]->m_operand.m_pConstant;
		pVal2 = (NXSL_Value *)m_pDataStack->Peek();
	}
	else
	{
		pVal2 = (NXSL_Value *)m_pDataStack->Pop();
		pVal1 = (NXSL_Value *)m_pDataStack->Pop();
	}

   if ((pVal1 != NULL) && (pVal2 != NULL))
   {
      if ((!pVal1->IsNull() && !pVal2->IsNull()) ||
          (nOpCode == OPCODE_EQ) || (nOpCode == OPCODE_NE) || (nOpCode == OPCODE_CASE))
      {
         if (pVal1->IsNumeric() && pVal2->IsNumeric() &&
             (nOpCode != OPCODE_CONCAT) && (nOpCode != OPCODE_LIKE))
         {
            nType = SelectResultType(pVal1->DataType(), pVal2->DataType(), nOpCode);
            if (nType != NXSL_DT_NULL)
            {
               if ((pVal1->Convert(nType)) && (pVal2->Convert(nType)))
               {
                  switch(nOpCode)
                  {
                     case OPCODE_ADD:
                        pRes = pVal1;
                        pRes->Add(pVal2);
                        delete pVal2;
                        break;
                     case OPCODE_SUB:
                        pRes = pVal1;
                        pRes->Sub(pVal2);
                        delete pVal2;
                        break;
                     case OPCODE_MUL:
                        pRes = pVal1;
                        pRes->Mul(pVal2);
                        delete pVal2;
                        break;
                     case OPCODE_DIV:
                        pRes = pVal1;
                        pRes->Div(pVal2);
                        delete pVal2;
                        break;
                     case OPCODE_REM:
                        pRes = pVal1;
                        pRes->Rem(pVal2);
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
                        pRes->LShift(pVal2->GetValueAsInt32());
                        delete pVal2;
                        break;
                     case OPCODE_RSHIFT:
                        pRes = pVal1;
                        pRes->RShift(pVal2->GetValueAsInt32());
                        delete pVal2;
                        break;
                     case OPCODE_BIT_AND:
                        pRes = pVal1;
                        pRes->BitAnd(pVal2);
                        delete pVal2;
                        break;
                     case OPCODE_BIT_OR:
                        pRes = pVal1;
                        pRes->BitOr(pVal2);
                        delete pVal2;
                        break;
                     case OPCODE_BIT_XOR:
                        pRes = pVal1;
                        pRes->BitXor(pVal2);
                        delete pVal2;
                        break;
                     case OPCODE_AND:
                        nResult = (pVal1->IsNonZero() && pVal2->IsNonZero());
                        delete pVal1;
                        delete pVal2;
                        pRes = new NXSL_Value(nResult);
                        break;
                     case OPCODE_OR:
                        nResult = (pVal1->IsNonZero() || pVal2->IsNonZero());
                        delete pVal1;
                        delete pVal2;
                        pRes = new NXSL_Value(nResult);
                        break;
                     case OPCODE_CASE:
                        pRes = new NXSL_Value((LONG)pVal1->EQ(pVal2));
                        break;
                     default:
                        Error(NXSL_ERR_INTERNAL);
                        break;
                  }
               }
               else
               {
                  Error(NXSL_ERR_TYPE_CAST);
               }
            }
            else
            {
               Error(NXSL_ERR_REAL_VALUE);
            }
         }
         else
         {
            switch(nOpCode)
            {
               case OPCODE_EQ:
               case OPCODE_NE:
					case OPCODE_CASE:
                  if (pVal1->IsNull() && pVal2->IsNull())
                  {
                     nResult = 1;
                  }
                  else if (pVal1->IsNull() || pVal2->IsNull())
                  {
                     nResult = 0;
                  }
                  else
                  {
                     pszText1 = pVal1->GetValueAsString(&dwLen1);
                     pszText2 = pVal2->GetValueAsString(&dwLen2);
                     if (dwLen1 == dwLen2)
                        nResult = !memcmp(pszText1, pszText2, dwLen1);
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
                  if (pVal1->IsNull() || pVal2->IsNull())
                  {
                     Error(NXSL_ERR_NULL_VALUE);
                  }
                  else if (pVal1->IsObject() || pVal2->IsObject())
                  {
                     Error(NXSL_ERR_INVALID_OBJECT_OPERATION);
                  }
                  else
                  {
                     pRes = pVal1;
                     pszText2 = pVal2->GetValueAsString(&dwLen2);
                     pRes->Concatenate(pszText2, dwLen2);
                     delete pVal2;
                  }
                  break;
               case OPCODE_LIKE:
               case OPCODE_ILIKE:
                  if (pVal1->IsString() && pVal2->IsString())
                  {
                     pRes = new NXSL_Value((LONG)MatchString(pVal2->GetValueAsCString(),
                                                             pVal1->GetValueAsCString(),
                                                             nOpCode == OPCODE_LIKE));
                     delete pVal1;
                     delete pVal2;
                  }
                  else
                  {
                     Error(NXSL_ERR_NOT_STRING);
                  }
                  break;
               case OPCODE_MATCH:
               case OPCODE_IMATCH:
                  if (pVal1->IsString() && pVal2->IsString())
                  {
                     pRes = MatchRegexp(pVal1, pVal2, nOpCode == OPCODE_IMATCH);
                     delete pVal1;
                     delete pVal2;
                  }
                  else
                  {
                     Error(NXSL_ERR_NOT_STRING);
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
                  Error(NXSL_ERR_NOT_NUMBER);
                  break;
               default:
                  Error(NXSL_ERR_INTERNAL);
                  break;
            }
         }
      }
      else
      {
         Error(NXSL_ERR_NULL_VALUE);
      }
   }
   else
   {
      Error(NXSL_ERR_DATA_STACK_UNDERFLOW);
   }

   if (pRes != NULL)
      m_pDataStack->Push(pRes);
}


//
// Perform unary operation on operand from the stack and push result back to stack
//

void NXSL_Program::DoUnaryOperation(int nOpCode)
{
   NXSL_Value *pVal;

   pVal = (NXSL_Value *)m_pDataStack->Peek();
   if (pVal != NULL)
   {
      if (pVal->IsNumeric())
      {
         switch(nOpCode)
         {
            case OPCODE_NEG:
               pVal->Negate();
               break;
            case OPCODE_NOT:
               if (!pVal->IsReal())
               {
                  pVal->Set((LONG)pVal->IsZero());
               }
               else
               {
                  Error(NXSL_ERR_REAL_VALUE);
               }
               break;
            case OPCODE_BIT_NOT:
               if (!pVal->IsReal())
               {
                  pVal->BitNot();
               }
               else
               {
                  Error(NXSL_ERR_REAL_VALUE);
               }
               break;
            default:
               Error(NXSL_ERR_INTERNAL);
               break;
         }
      }
      else
      {
         Error(NXSL_ERR_NOT_NUMBER);
      }
   }
   else
   {
      Error(NXSL_ERR_DATA_STACK_UNDERFLOW);
   }
}


//
// Relocate code block
//

void NXSL_Program::RelocateCode(DWORD dwStart, DWORD dwLen, DWORD dwShift)
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

void NXSL_Program::UseModule(NXSL_Program *pModule, const char *pszName)
{
   DWORD i, j, dwStart;

   // Check if module already loaded
   for(i = 0; i < m_dwNumModules; i++)
      if (!stricmp(pszName, m_pModuleList[i].m_szName))
         return;  // Already loaded

   // Add code from module
   dwStart = m_dwCodeSize;
   m_dwCodeSize += pModule->m_dwCodeSize;
   m_ppInstructionSet = (NXSL_Instruction **)realloc(m_ppInstructionSet,
         sizeof(NXSL_Instruction *) * m_dwCodeSize);
   for(i = dwStart, j = 0; i < m_dwCodeSize; i++, j++)
      m_ppInstructionSet[i] = new NXSL_Instruction(pModule->m_ppInstructionSet[j]);
   RelocateCode(dwStart, pModule->m_dwCodeSize, dwStart);
   
   // Add function names from module
   m_pFunctionList = (NXSL_Function *)realloc(m_pFunctionList,
         sizeof(NXSL_Function) * (m_dwNumFunctions + pModule->m_dwNumFunctions));
   memcpy(&m_pFunctionList[m_dwNumFunctions], pModule->m_pFunctionList,
          sizeof(NXSL_Function) * pModule->m_dwNumFunctions);
   for(i = m_dwNumFunctions, j = 0; j < pModule->m_dwNumFunctions; i++, j++)
      m_pFunctionList[i].m_dwAddr += dwStart;

   // Register module as loaded
   m_pModuleList = (NXSL_Module *)malloc(sizeof(NXSL_Module) * (m_dwNumModules + 1));
   strncpy(m_pModuleList[m_dwNumModules].m_szName, pszName, MAX_PATH);
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

void NXSL_Program::CallFunction(int nArgCount)
{
   int i;
   NXSL_Value *pValue;
   char szBuffer[32];

   if (m_dwSubLevel < CONTROL_STACK_LIMIT)
   {
      m_dwSubLevel++;
      m_pCodeStack->Push((void *)(m_dwCurrPos + 1));
      m_pCodeStack->Push(m_pLocals);
      m_pLocals = new NXSL_VariableSystem;
      m_nBindPos = 1;

      // Bind arguments
      for(i = nArgCount; i > 0; i--)
      {
         pValue = (NXSL_Value *)m_pDataStack->Pop();
         if (pValue != NULL)
         {
            sprintf(szBuffer, "$%d", i);
            m_pLocals->Create(szBuffer, pValue);
         }
         else
         {
            Error(NXSL_ERR_DATA_STACK_UNDERFLOW);
            break;
         }
      }
   }
   else
   {
      Error(NXSL_ERR_CONTROL_STACK_OVERFLOW);
   }
}


//
// Find function address by name
//

DWORD NXSL_Program::GetFunctionAddress(char *pszName)
{
   DWORD i;

   for(i = 0; i < m_dwNumFunctions; i++)
      if (!strcmp(m_pFunctionList[i].m_szName, pszName))
         return m_pFunctionList[i].m_dwAddr;
   return INVALID_ADDRESS;
}


//
// Match regular expression
//

NXSL_Value *NXSL_Program::MatchRegexp(NXSL_Value *pValue, NXSL_Value *pRegexp,
                                      BOOL bIgnoreCase)
{
   regex_t preg;
   regmatch_t fields[256];
   NXSL_Value *pResult;
   NXSL_Variable *pVar;
   TCHAR szName[16];
   int i;

   if (regcomp(&preg, pRegexp->GetValueAsCString(),
               bIgnoreCase ? REG_EXTENDED | REG_ICASE : REG_EXTENDED) == 0)
   {
      if (regexec(&preg, pValue->GetValueAsCString(), 256, fields, 0) == 0)
      {
         pResult = new NXSL_Value((LONG)1);
         for(i = 1; (i < 256) && (fields[i].rm_so != -1); i++)
         {
            _stprintf(szName, _T("$%d"), i);
            pVar = m_pLocals->Find(szName);
            if (pVar == NULL)
               m_pLocals->Create(szName, new NXSL_Value(pValue->GetValueAsCString() + fields[i].rm_so, fields[i].rm_eo - fields[i].rm_so));
            else
               pVar->Set(new NXSL_Value(pValue->GetValueAsCString() + fields[i].rm_so, fields[i].rm_eo - fields[i].rm_so));
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
      Error(NXSL_ERR_REGEXP_ERROR);
      pResult = NULL;
   }
   return pResult;
}


//
// Get final jump destination from a jump chain
//

DWORD NXSL_Program::FinalJumpDestination(DWORD dwAddr)
{
	return (m_ppInstructionSet[dwAddr]->m_nOpCode == OPCODE_JMP) ? FinalJumpDestination(m_ppInstructionSet[dwAddr]->m_operand.m_dwAddr) : dwAddr;
}


//
// Optimize compiled program
//

void NXSL_Program::Optimize(void)
{
	DWORD i, j;

	// Convert jump chains to single jump
	for(i = 0; i < m_dwCodeSize; i++)
	{
		if ((m_ppInstructionSet[i]->m_nOpCode == OPCODE_JMP) ||
			 (m_ppInstructionSet[i]->m_nOpCode == OPCODE_JZ) ||
			 (m_ppInstructionSet[i]->m_nOpCode == OPCODE_JNZ))
		{
			m_ppInstructionSet[i]->m_operand.m_dwAddr = FinalJumpDestination(m_ppInstructionSet[i]->m_operand.m_dwAddr);
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
