/* 
** NetXMS - Network Management System
** NetXMS Scripting Language Interpreter
** Copyright (C) 2003-2015 Victor Kirhenshtein
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
NXSL_Instruction::NXSL_Instruction(int line, INT16 opCode)
{
   m_opCode = opCode;
   m_sourceLine = line;
   m_stackItems = 0;
   m_addr2 = INVALID_ADDRESS;
}

/**
 * Create instruction with constant operand
 */
NXSL_Instruction::NXSL_Instruction(int line, INT16 opCode, NXSL_Value *value)
{
   m_opCode = opCode;
   m_sourceLine = line;
   m_operand.m_constant = value;
   m_stackItems = 0;
   m_addr2 = INVALID_ADDRESS;
}

/**
 * Create instruction with identifier operand.
 */
NXSL_Instruction::NXSL_Instruction(int line, INT16 opCode, char *identifier)
{
   m_opCode = opCode;
   m_sourceLine = line;
#ifdef UNICODE
	m_operand.m_pszString = WideStringFromUTF8String(identifier);
	free(identifier);
#else
   m_operand.m_pszString = identifier;
#endif
   m_stackItems = 0;
   m_addr2 = INVALID_ADDRESS;
}

/**
 * Create instruction with identifier operand, specific stack item count, and secondary address.
 */
NXSL_Instruction::NXSL_Instruction(int line, INT16 opCode, char *identifier, INT16 stackItems, UINT32 addr2)
{
   m_opCode = opCode;
   m_sourceLine = line;
#ifdef UNICODE
   m_operand.m_pszString = WideStringFromUTF8String(identifier);
   free(identifier);
#else
   m_operand.m_pszString = identifier;
#endif
   m_stackItems = stackItems;
   m_addr2 = addr2;
}

/**
 * Create instruction with address operand
 */
NXSL_Instruction::NXSL_Instruction(int line, INT16 opCode, UINT32 addr)
{
   m_opCode = opCode;
   m_sourceLine = line;
   m_operand.m_addr = addr;
   m_stackItems = 0;
   m_addr2 = INVALID_ADDRESS;
}

/**
 * Create instruction without operand and non-zero stack item count
 */
NXSL_Instruction::NXSL_Instruction(int line, INT16 opCode, INT16 stackItems)
{
   m_opCode = opCode;
   m_sourceLine = line;
   m_stackItems = stackItems;
   m_addr2 = INVALID_ADDRESS;
}

/**
 * Copy constructor
 */
NXSL_Instruction::NXSL_Instruction(NXSL_Instruction *src)
{
   m_opCode = src->m_opCode;
   m_sourceLine = src->m_sourceLine;
   m_stackItems = src->m_stackItems;
   switch(getOperandType())
   {
      case OP_TYPE_CONST:
         m_operand.m_constant = new NXSL_Value(src->m_operand.m_constant);
         break;
      case OP_TYPE_STRING:
         m_operand.m_pszString = _tcsdup(src->m_operand.m_pszString);
         break;
      default:
         m_operand.m_addr = src->m_operand.m_addr;
         break;
   }
   m_addr2 = src->m_addr2;
}

/**
 * Destructor
 */
NXSL_Instruction::~NXSL_Instruction()
{
   switch(getOperandType())
   {
      case OP_TYPE_STRING:
         MemFree(m_operand.m_pszString);
         break;
      case OP_TYPE_CONST:
         delete m_operand.m_constant;
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
   switch(m_opCode)
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
      case OPCODE_PUSH_EXPRVAR:
      case OPCODE_PUSH_VARIABLE:
      case OPCODE_SAFE_GET_ATTR:
      case OPCODE_SELECT:
      case OPCODE_SET:
      case OPCODE_SET_ATTRIBUTE:
      case OPCODE_SET_EXPRVAR:
      case OPCODE_UPDATE_EXPRVAR:
         return OP_TYPE_STRING;
		case OPCODE_CASE:
      case OPCODE_PUSH_CONSTANT:
         return OP_TYPE_CONST;
      case OPCODE_JMP:
      case OPCODE_CALL:
      case OPCODE_CATCH:
      case OPCODE_JZ:
      case OPCODE_JNZ:
      case OPCODE_JZ_PEEK:
      case OPCODE_JNZ_PEEK:
         return OP_TYPE_ADDR;
      default:
         return OP_TYPE_NONE;
   }
}
