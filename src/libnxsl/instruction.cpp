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
** File: instruction.cpp
**
**/

#include "libnxsl.h"

/**
 * Create instruction without operand
 */
NXSL_Instruction::NXSL_Instruction(int nLine, short nOpCode)
{
   m_nOpCode = nOpCode;
   m_nSourceLine = nLine;
   m_nStackItems = 0;
}

/**
 * Create instruction with constant operand
 */
NXSL_Instruction::NXSL_Instruction(int nLine, short nOpCode, NXSL_Value *pValue)
{
   m_nOpCode = nOpCode;
   m_nSourceLine = nLine;
   m_operand.m_pConstant = pValue;
   m_nStackItems = 0;
}

/**
 * Create instruction with string operand.
 * String must be dynamically allocated.
 */
NXSL_Instruction::NXSL_Instruction(int nLine, short nOpCode, const NXSL_Identifier& identifier)
{
   m_nOpCode = nOpCode;
   m_nSourceLine = nLine;
   m_operand.m_identifier = new NXSL_Identifier(identifier);
   m_nStackItems = 0;
}

/**
 * Create instruction with string operand and non-zero stack item count.
 * String must be dynamically allocated.
 */
NXSL_Instruction::NXSL_Instruction(int nLine, short nOpCode, const NXSL_Identifier& identifier, short nStackItems)
{
   m_nOpCode = nOpCode;
   m_nSourceLine = nLine;
   m_operand.m_identifier = new NXSL_Identifier(identifier);
   m_nStackItems = nStackItems;
}

/**
 * Create instruction with address operand
 */
NXSL_Instruction::NXSL_Instruction(int nLine, short nOpCode, UINT32 dwAddr)
{
   m_nOpCode = nOpCode;
   m_nSourceLine = nLine;
   m_operand.m_dwAddr = dwAddr;
   m_nStackItems = 0;
}

/**
 * Create instruction without operand and non-zero stack item count
 */
NXSL_Instruction::NXSL_Instruction(int nLine, short nOpCode, short nStackItems)
{
   m_nOpCode = nOpCode;
   m_nSourceLine = nLine;
   m_nStackItems = nStackItems;
}

/**
 * Copy constructor
 */
NXSL_Instruction::NXSL_Instruction(NXSL_Instruction *pSrc)
{
   m_nOpCode = pSrc->m_nOpCode;
   m_nSourceLine = pSrc->m_nSourceLine;
   m_nStackItems = pSrc->m_nStackItems;
   switch(getOperandType())
   {
		case OP_TYPE_CONST:
         m_operand.m_pConstant = new NXSL_Value(pSrc->m_operand.m_pConstant);
         break;
      case OP_TYPE_IDENTIFIER:
         m_operand.m_identifier = new NXSL_Identifier(*pSrc->m_operand.m_identifier);
         break;
      default:
         m_operand.m_dwAddr = pSrc->m_operand.m_dwAddr;
         break;
   }
}

/**
 * Destructor
 */
NXSL_Instruction::~NXSL_Instruction()
{
   switch(getOperandType())
   {
      case OP_TYPE_IDENTIFIER:
         delete m_operand.m_identifier;
         break;
      case OP_TYPE_CONST:
         delete m_operand.m_pConstant;
         break;
      default:
         break;
   }
}

/**
 * Get operand type for instruction
 */
OperandType NXSL_Instruction::getOperandType()
{
   switch(m_nOpCode)
   {
      case OPCODE_ARRAY:
      case OPCODE_BIND:
      case OPCODE_CALL_EXTERNAL:
      case OPCODE_CALL_METHOD:
      case OPCODE_CASE_CONST:
      case OPCODE_DEC:
      case OPCODE_DECP:
      case OPCODE_GET_ATTRIBUTE:
      case OPCODE_GLOBAL:
      case OPCODE_GLOBAL_ARRAY:
      case OPCODE_INC:
      case OPCODE_INCP:
		case OPCODE_NAME:
      case OPCODE_PUSH_CONSTREF:
      case OPCODE_PUSH_VARIABLE:
      case OPCODE_SAFE_GET_ATTR:
      case OPCODE_SELECT:
      case OPCODE_SET:
      case OPCODE_SET_ATTRIBUTE:
         return OP_TYPE_IDENTIFIER;
		case OPCODE_CASE:
      case OPCODE_PUSH_CONSTANT:
         return OP_TYPE_CONST;
      case OPCODE_PUSH_VARPTR:
      case OPCODE_SET_VARPTR:
      case OPCODE_INC_VARPTR:
      case OPCODE_DEC_VARPTR:
      case OPCODE_INCP_VARPTR:
      case OPCODE_DECP_VARPTR:
         return OP_TYPE_VARIABLE;
      case OPCODE_JMP:
      case OPCODE_CALL:
      case OPCODE_JZ:
      case OPCODE_JNZ:
      case OPCODE_JZ_PEEK:
      case OPCODE_JNZ_PEEK:
      case OPCODE_CATCH:
         return OP_TYPE_ADDR;
      case OPCODE_CALL_EXTPTR:
         return OP_TYPE_EXT_FUNCTION;
      default:
         return OP_TYPE_NONE;
   }
}

/**
 * Restore variable reference
 */
void NXSL_Instruction::restoreVariableReference(NXSL_Identifier *identifier)
{
   switch(m_nOpCode)
   {
      case OPCODE_PUSH_VARPTR:
         m_nOpCode = OPCODE_PUSH_VARIABLE;
         break;
      case OPCODE_SET_VARPTR:
         m_nOpCode = OPCODE_SET;
         break;
      case OPCODE_INC_VARPTR:
         m_nOpCode = OPCODE_INC;
         break;
      case OPCODE_DEC_VARPTR:
         m_nOpCode = OPCODE_DEC;
         break;
      case OPCODE_INCP_VARPTR:
         m_nOpCode = OPCODE_INCP;
         break;
      case OPCODE_DECP_VARPTR:
         m_nOpCode = OPCODE_DECP;
         break;
      default:
         break;
   }
   m_operand.m_identifier = identifier;
}
