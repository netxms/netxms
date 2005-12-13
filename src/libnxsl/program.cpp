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
// Command mnemonics
//

static char *m_pszCommandMnemonic[] =
{
   "NOP", "RET", "JMP", "CALL", "CALL",
   "PUSH", "PUSH", "EXIT", "POP", "SET",
   "ADD", "SUB", "MUL", "DIV", "REM",
   "EQ", "NE", "LT", "LE", "GT", "GE",
   "BITAND", "BITOR", "BITXOR",
   "AND", "OR", "LSHIFT", "RSHIFT",
   "NRET", "JZ", "PRINT"
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
   m_pszErrorText = NULL;
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
          (m_ppInstructionSet[i]->operand.m_dwAddr == INVALID_ADDRESS))
      {
         m_ppInstructionSet[i]->operand.m_dwAddr = m_dwCodeSize;
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
              m_pszCommandMnemonic[m_ppInstructionSet[i]->m_nOpCode]);
      switch(m_ppInstructionSet[i]->m_nOpCode)
      {
         case OPCODE_CALL_EXTERNAL:
            fprintf(pFile, "%s, %d\n", m_ppInstructionSet[i]->operand.m_pszString,
                    m_ppInstructionSet[i]->m_nStackItems);
            break;
         case OPCODE_CALL:
            fprintf(pFile, "%04X, %d\n", m_ppInstructionSet[i]->operand.m_dwAddr,
                    m_ppInstructionSet[i]->m_nStackItems);
            break;
         case OPCODE_JMP:
         case OPCODE_JZ:
            fprintf(pFile, "%04X\n", m_ppInstructionSet[i]->operand.m_dwAddr);
            break;
         case OPCODE_PUSH_VARIABLE:
         case OPCODE_SET:
            fprintf(pFile, "%s\n", m_ppInstructionSet[i]->operand.m_pszString);
            break;
         case OPCODE_PUSH_CONSTANT:
            if (m_ppInstructionSet[i]->operand.m_pConstant->IsNull())
               fprintf(pFile, "<null>\n");
            else
               fprintf(pFile, "\"%s\"\n", 
                       m_ppInstructionSet[i]->operand.m_pConstant->GetValueAsString());
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

void NXSL_Program::Error(TCHAR *pszFormat, ...)
{
   va_list args;
   TCHAR szBuffer[1024];

   safe_free(m_pszErrorText);
   va_start(args, pszFormat);
   _vsntprintf(szBuffer, 1024, pszFormat, args);
   va_end(args);
   m_pszErrorText = _tcsdup(szBuffer);
}


//
// Run program
//

int NXSL_Program::Run(void)
{
   // Create stacks
   m_pDataStack = new NXSL_Stack;
   m_pCodeStack = new NXSL_Stack;

   m_dwCurrPos = 0;
   while(m_dwCurrPos < m_dwCodeSize)
      Execute();

   return (m_dwCurrPos == INVALID_ADDRESS) ? -1 : 0;
}


//
// Execute single instruction
//

void NXSL_Program::Execute(void)
{
   NXSL_Instruction *cp;
   NXSL_Value *pValue;
   DWORD dwNext = m_dwCurrPos + 1;
   int i;

   cp = m_ppInstructionSet[m_dwCurrPos];
//printf("Execute: %s\n", m_pszCommandMnemonic[cp->m_nOpCode]);
   switch(cp->m_nOpCode)
   {
      case OPCODE_PUSH_CONSTANT:
         m_pDataStack->Push(cp->operand.m_pConstant);
         break;
      case OPCODE_POP:
         for(i = 0; i < cp->m_nStackItems; i++)
            delete (NXSL_Value *)m_pDataStack->Pop();
         break;
      case OPCODE_JMP:
         dwNext = cp->operand.m_dwAddr;
         break;
      case OPCODE_JZ:
         pValue = (NXSL_Value *)m_pDataStack->Pop();
         if (pValue != NULL)
         {
            if (pValue->GetValueAsInt() == 0)
               dwNext = cp->operand.m_dwAddr;
         }
         break;
      case OPCODE_PRINT:
         pValue = (NXSL_Value *)m_pDataStack->Pop();
         if (pValue != NULL)
         {
            fputs(pValue->GetValueAsString(), stdout);
         }
         else
         {
            Error("Data stack underflow");
            dwNext = INVALID_ADDRESS;
         }
         break;
      case OPCODE_EXIT:
         pValue = (NXSL_Value *)m_pDataStack->Pop();
         if (pValue != NULL)
         {
            dwNext = m_dwCodeSize;
         }
         else
         {
            Error("Data stack underflow");
            dwNext = INVALID_ADDRESS;
         }
         break;
      default:
         break;
   }
   m_dwCurrPos = dwNext;
}
