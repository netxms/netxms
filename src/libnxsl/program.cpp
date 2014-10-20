/* 
** NetXMS - Network Management System
** NetXMS Scripting Language Interpreter
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
** File: program.cpp
**
**/

#include "libnxsl.h"
#include <netxms-regex.h>

/**
 * Constants
 */
#define MAX_ERROR_NUMBER         31
#define CONTROL_STACK_LIMIT      32768

/**
 * Command mnemonics
 */
const char *g_nxslCommandMnemonic[] =
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
	"ESET", "ASET", "NAME", "FOREACH", "NEXT",
	"GLOBAL", "GARRAY", "JZP", "JNZP", "ADDARR",
	"AGETS", "CALL", "CASE", "EINC", "EDEC",
   "EINCP", "EDECP", "ABORT", "CATCH"
};

/**
 * Constructor
 */
NXSL_Program::NXSL_Program()
{
   m_instructionSet = new ObjectArray<NXSL_Instruction>(32, 32, true);
   m_constants = new StringObjectMap<NXSL_Value>(true);
   m_functions = new ObjectArray<NXSL_Function>(16, 16, true);
}

/**
 * Destructor
 */
NXSL_Program::~NXSL_Program()
{
   delete m_instructionSet;
   delete m_constants;
   delete m_functions;
}

/**
 * Add new constant. Name expected to be dynamically allocated and
 * will be destroyed by NXSL_Program when no longer needed.
 */
bool NXSL_Program::addConstant(const char *name, NXSL_Value *value)
{
   bool success = false;
#ifdef UNICODE
   WCHAR *tname = WideStringFromUTF8String(name);
#else
   const char *tname = name;
#endif
   if (m_constants->get(tname) == NULL)
   {
      m_constants->set(tname, value);
      success = true;
   }
#ifdef UNICODE
   free(tname);
#endif
   return success;
}

/**
 * Resolve last jump with INVALID_ADDRESS to current address
 */
void NXSL_Program::resolveLastJump(int nOpCode)
{
   for(int i = m_instructionSet->size(); i > 0;)
   {
      i--;
      NXSL_Instruction *instr = m_instructionSet->get(i);
      if ((instr->m_nOpCode == nOpCode) &&
          (instr->m_operand.m_dwAddr == INVALID_ADDRESS))
      {
         instr->m_operand.m_dwAddr = m_instructionSet->size();
         break;
      }
   }
}

/**
 * Create jump at given address replacing another instruction (usually NOP)
 */
void NXSL_Program::createJumpAt(UINT32 dwOpAddr, UINT32 dwJumpAddr)
{
	if (dwOpAddr >= (UINT32)m_instructionSet->size())
		return;

	int nLine = m_instructionSet->get(dwOpAddr)->m_nSourceLine;
   m_instructionSet->set(dwOpAddr, new NXSL_Instruction(nLine, OPCODE_JMP, dwJumpAddr));
}

/**
 * Add new function to defined functions list
 * Will use first free address if dwAddr == INVALID_ADDRESS
 * Name must be in UTF-8
 */
bool NXSL_Program::addFunction(const char *pszName, UINT32 dwAddr, char *pszError)
{
   // Check for duplicate function names
#ifdef UNICODE
	WCHAR *pwszName = WideStringFromUTF8String(pszName);
#endif
   for(int i = 0; i < m_functions->size(); i++)
#ifdef UNICODE
      if (!wcscmp(m_functions->get(i)->m_name, pwszName))
#else
      if (!strcmp(m_functions->get(i)->m_name, pszName))
#endif
      {
         sprintf(pszError, "Duplicate function name: \"%s\"", pszName);
#ifdef UNICODE
			free(pwszName);
#endif
         return false;
      }
   NXSL_Function *f = new NXSL_Function;
#ifdef UNICODE
   nx_strncpy(f->m_name, pwszName, MAX_FUNCTION_NAME);
	free(pwszName);
#else
   nx_strncpy(f->m_name, pszName, MAX_FUNCTION_NAME);
#endif
   f->m_dwAddr = (dwAddr == INVALID_ADDRESS) ? m_instructionSet->size() : dwAddr;
   m_functions->add(f);
   return true;
}

/**
 * Add required module name (must be dynamically allocated).
 */
void NXSL_Program::addRequiredModule(char *name)
{
#ifdef UNICODE
   m_requiredModules.addPreallocated(WideStringFromUTF8String(name));
	free(name);
#else
   m_requiredModules.addPreallocated(name);
#endif
}

/**
 * resolve local functions
 */
void NXSL_Program::resolveFunctions()
{
   for(int i = 0; i < m_instructionSet->size(); i++)
   {
      NXSL_Instruction *instr = m_instructionSet->get(i);
      if (instr->m_nOpCode == OPCODE_CALL_EXTERNAL)
      {
         for(int j = 0; j < m_functions->size(); j++)
         {
            NXSL_Function *f = m_functions->get(j);
            if (!_tcscmp(f->m_name, instr->m_operand.m_pszString))
            {
               free(instr->m_operand.m_pszString);
               instr->m_operand.m_dwAddr = f->m_dwAddr;
               instr->m_nOpCode = OPCODE_CALL;
               break;
            }
         }
      }
   }
}

/**
 * Dump program to file (as text)
 */
void NXSL_Program::dump(FILE *pFile)
{
   for(int i = 0; i < m_instructionSet->size(); i++)
   {
      NXSL_Instruction *instr = m_instructionSet->get(i);
      _ftprintf(pFile, _T("%04X  %-6hs  "), i, g_nxslCommandMnemonic[instr->m_nOpCode]);
      switch(instr->m_nOpCode)
      {
         case OPCODE_CALL_EXTERNAL:
         case OPCODE_GLOBAL:
            _ftprintf(pFile, _T("%s, %d\n"), instr->m_operand.m_pszString,
                      instr->m_nStackItems);
            break;
         case OPCODE_CALL:
            _ftprintf(pFile, _T("%04X, %d\n"), instr->m_operand.m_dwAddr,
                      instr->m_nStackItems);
            break;
         case OPCODE_CALL_METHOD:
            _ftprintf(pFile, _T("@%s, %d\n"), instr->m_operand.m_pszString,
                      instr->m_nStackItems);
            break;
         case OPCODE_CATCH:
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
 * Get final jump destination from a jump chain
 */
UINT32 NXSL_Program::getFinalJumpDestination(UINT32 dwAddr, int srcJump)
{
   NXSL_Instruction *instr = m_instructionSet->get(dwAddr);
	if ((instr->m_nOpCode == OPCODE_JMP) || (instr->m_nOpCode == srcJump))
		return getFinalJumpDestination(instr->m_operand.m_dwAddr, srcJump);
	return dwAddr;
}

/**
 * Optimize compiled program
 */
void NXSL_Program::optimize()
{
	int i;

	// Convert push constant followed by NEG to single push constant
	for(i = 0; (m_instructionSet->size() > 1) && (i < m_instructionSet->size() - 1); i++)
	{
      NXSL_Instruction *instr = m_instructionSet->get(i);
		if ((instr->m_nOpCode == OPCODE_PUSH_CONSTANT) &&
		    (m_instructionSet->get(i + 1)->m_nOpCode == OPCODE_NEG) &&
			 instr->m_operand.m_pConstant->isNumeric() &&
			 !instr->m_operand.m_pConstant->isUnsigned())
		{
			instr->m_operand.m_pConstant->negate();
			removeInstructions(i + 1, 1);
		}
	}

	// Convert jumps to address beyond code end to NRETs
	for(i = 0; i < m_instructionSet->size(); i++)
	{
      NXSL_Instruction *instr = m_instructionSet->get(i);
		if ((instr->m_nOpCode == OPCODE_JMP) && (instr->m_operand.m_dwAddr >= (UINT32)m_instructionSet->size()))
		{
			instr->m_nOpCode = OPCODE_RET_NULL;
		}
	}

	// Fix destination address for JZP/JNZP jumps
	for(i = 0; i < m_instructionSet->size(); i++)
	{
      NXSL_Instruction *instr = m_instructionSet->get(i);
		if (((instr->m_nOpCode == OPCODE_JZ_PEEK) && 
			  (m_instructionSet->get(instr->m_operand.m_dwAddr)->m_nOpCode == OPCODE_JNZ_PEEK)) ||
		    ((instr->m_nOpCode == OPCODE_JNZ_PEEK) && 
			  (m_instructionSet->get(instr->m_operand.m_dwAddr)->m_nOpCode == OPCODE_JZ_PEEK)))
		{
			instr->m_operand.m_dwAddr++;
		}
	}

	// Convert jump chains to single jump
	for(i = 0; i < m_instructionSet->size(); i++)
	{
      NXSL_Instruction *instr = m_instructionSet->get(i);
		if ((instr->m_nOpCode == OPCODE_JMP) ||
			 (instr->m_nOpCode == OPCODE_JZ) ||
			 (instr->m_nOpCode == OPCODE_JNZ))
		{
			instr->m_operand.m_dwAddr = getFinalJumpDestination(instr->m_operand.m_dwAddr, -1);
		}
		else if ((instr->m_nOpCode == OPCODE_JZ_PEEK) ||
			      (instr->m_nOpCode == OPCODE_JNZ_PEEK))
		{
			instr->m_operand.m_dwAddr = getFinalJumpDestination(instr->m_operand.m_dwAddr, instr->m_nOpCode);
		}
	}

	// Remove jumps to next instruction
	for(i = 0; i < m_instructionSet->size(); i++)
	{
      NXSL_Instruction *instr = m_instructionSet->get(i);
		if (((instr->m_nOpCode == OPCODE_JMP) ||
			  (instr->m_nOpCode == OPCODE_JZ_PEEK) ||
			  (instr->m_nOpCode == OPCODE_JNZ_PEEK)) &&
			 (instr->m_operand.m_dwAddr == i + 1))
		{
			removeInstructions(i, 1);
			i--;
		}
	}
}

/**
 * Remove one or more instructions starting at given position.
 *
 * @param start start offset
 * @param count number of instructions to remove
 */
void NXSL_Program::removeInstructions(UINT32 start, int count)
{
	if ((count <= 0) || (start + (UINT32)count >= (UINT32)m_instructionSet->size()))
		return;

   int i;
	for(i = 0; i < count; i++)
      m_instructionSet->remove(start);

	// Change jump destination addresses
	for(i = 0; i < m_instructionSet->size(); i++)
	{
      NXSL_Instruction *instr = m_instructionSet->get(i);
		if (((instr->m_nOpCode == OPCODE_JMP) ||
		     (instr->m_nOpCode == OPCODE_JZ) ||
		     (instr->m_nOpCode == OPCODE_JNZ) ||
		     (instr->m_nOpCode == OPCODE_JZ_PEEK) ||
		     (instr->m_nOpCode == OPCODE_JNZ_PEEK) ||
		     (instr->m_nOpCode == OPCODE_CALL)) &&
		    (instr->m_operand.m_dwAddr > start))
		{
         instr->m_operand.m_dwAddr -= count;
		}
	}
}
