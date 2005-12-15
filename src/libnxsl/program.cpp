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
** $module: program.cpp
**
**/

#include "libnxsl.h"


//
// Constants
//

#define MAX_ERROR_NUMBER   6


//
// Command mnemonics
//

static char *m_szCommandMnemonic[] =
{
   "NOP", "RET", "JMP", "CALL", "CALL",
   "PUSH", "PUSH", "EXIT", "POP", "SET",
   "ADD", "SUB", "MUL", "DIV", "REM",
   "EQ", "NE", "LT", "LE", "GT", "GE",
   "BITAND", "BITOR", "BITXOR",
   "AND", "OR", "LSHIFT", "RSHIFT",
   "NRET", "JZ", "PRINT", "CONCAT"
};


//
// Error texts
//

static TCHAR *m_szErrorMessage[MAX_ERROR_NUMBER] =
{
   _T("Data stack underflow"),
   _T("Control stack underflow"),
   _T("Condition value is not a number"),
   _T("Bad arithmetic conversion"),
   _T("Invalid operation with NULL value"),
   _T("Internal error")
};


//
// Constructor
//

NXSL_Program::NXSL_Program(void)
{
   m_ppInstructionSet = NULL;
   m_dwCodeSize = 0;
   m_pDataStack = NULL;
   m_pCodeStack = NULL;
   m_nErrorCode = 0;
   m_pszErrorText = NULL;
   m_pConstants = new NXSL_VariableSystem;
   m_pGlobals = new NXSL_VariableSystem;
   m_pLocals = NULL;
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
            fprintf(pFile, "%04X\n", m_ppInstructionSet[i]->m_operand.m_dwAddr);
            break;
         case OPCODE_PUSH_VARIABLE:
         case OPCODE_SET:
            fprintf(pFile, "%s\n", m_ppInstructionSet[i]->m_operand.m_pszString);
            break;
         case OPCODE_PUSH_CONSTANT:
            if (m_ppInstructionSet[i]->m_operand.m_pConstant->IsNull())
               fprintf(pFile, "<null>\n");
            else
               fprintf(pFile, "\"%s\"\n", 
                       m_ppInstructionSet[i]->m_operand.m_pConstant->GetValueAsString());
            break;
         case OPCODE_POP:
            fprintf(pFile, "%d\n", m_ppInstructionSet[i]->m_nStackItems);
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
   _sntprintf(szBuffer, 1024, _T("Error %d in line %d: %s"),
              nError, m_ppInstructionSet[m_dwCurrPos]->m_nSourceLine,
              ((nError > 0) && (nError <= MAX_ERROR_NUMBER)) ? m_szErrorMessage[nError - 1] : _T("Unknown error code"));
   m_pszErrorText = _tcsdup(szBuffer);
   m_dwCurrPos = INVALID_ADDRESS;
}


//
// Run program
//

int NXSL_Program::Run(void)
{
   // Create stacks
   m_pDataStack = new NXSL_Stack;
   m_pCodeStack = new NXSL_Stack;

   // Create local variable system for main()
   m_pLocals = new NXSL_VariableSystem;

   m_dwCurrPos = 0;
   while(m_dwCurrPos < m_dwCodeSize)
      Execute();

   return (m_dwCurrPos == INVALID_ADDRESS) ? -1 : 0;
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
   DWORD dwNext = m_dwCurrPos + 1;
   int i;

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
            Error(1);
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
         pValue = (NXSL_Value *)m_pDataStack->Pop();
         if (pValue != NULL)
         {
            if (pValue->IsNumeric())
            {
               if (pValue->GetValueAsInt() == 0)
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
            Error(1);
         }
         break;
      case OPCODE_PRINT:
         pValue = (NXSL_Value *)m_pDataStack->Pop();
         if (pValue != NULL)
         {
            fputs(pValue->GetValueAsString(), stdout);
            delete pValue;
         }
         else
         {
            Error(1);
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
            Error(1);
         }
         break;
      case OPCODE_ADD:
      case OPCODE_SUB:
      case OPCODE_MUL:
      case OPCODE_DIV:
      case OPCODE_CONCAT:
         DoBinaryOperation(cp->m_nOpCode);
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

   pVal2 = (NXSL_Value *)m_pDataStack->Pop();
   pVal1 = (NXSL_Value *)m_pDataStack->Pop();

   if ((pVal1 != NULL) && (pVal2 != NULL))
   {
      if ((!pVal1->IsNull() && !pVal2->IsNull()) ||
          (nOpCode == OPCODE_EQ) || (nOpCode == OPCODE_NE))
      {
         if (pVal1->IsNumeric() && pVal2->IsNumeric())
         {
            switch(nOpCode)
            {
               case OPCODE_CONCAT:
                  pRes = pVal1;
                  pRes->Concatenate(pVal2->GetValueAsString());
                  delete pVal2;
                  break;
               default:
                  Error(6);
                  break;
            }
         }
         else
         {
            switch(nOpCode)
            {
               case OPCODE_EQ:
                  break;
               case OPCODE_NE:
                  break;
               case OPCODE_CONCAT:
                  if (pVal1->IsNull() || pVal2->IsNull())
                  {
                     Error(5);
                  }
                  else
                  {
                     pRes = pVal1;
                     pRes->Concatenate(pVal2->GetValueAsString());
                     delete pVal2;
                  }
                  break;
               default:
                  Error(6);
                  break;
            }
         }
      }
      else
      {
         Error(5);
      }
   }
   else
   {
      Error(1);
   }

   if (pRes != NULL)
      m_pDataStack->Push(pRes);
}
