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
** File: instruction.cpp
**
**/

#include "libnxsl.h"


//
// Constructors
//

NXSL_Instruction::NXSL_Instruction(int nLine, int nOpCode)
{
   m_nOpCode = nOpCode;
   m_nSourceLine = nLine;
   m_nStackItems = 0;
}

NXSL_Instruction::NXSL_Instruction(int nLine, int nOpCode, NXSL_Value *pValue)
{
   m_nOpCode = nOpCode;
   m_nSourceLine = nLine;
   m_operand.m_pConstant = pValue;
   m_nStackItems = 0;
}

NXSL_Instruction::NXSL_Instruction(int nLine, int nOpCode, char *pszString)
{
   m_nOpCode = nOpCode;
   m_nSourceLine = nLine;
#ifdef UNICODE
	m_operand.m_pszString = WideStringFromUTF8String(pszString);
	free(pszString);
#else
   m_operand.m_pszString = pszString;
#endif
   m_nStackItems = 0;
}

NXSL_Instruction::NXSL_Instruction(int nLine, int nOpCode, char *pszString, int nStackItems)
{
   m_nOpCode = nOpCode;
   m_nSourceLine = nLine;
#ifdef UNICODE
	m_operand.m_pszString = WideStringFromUTF8String(pszString);
	free(pszString);
#else
   m_operand.m_pszString = pszString;
#endif
   m_nStackItems = nStackItems;
}

NXSL_Instruction::NXSL_Instruction(int nLine, int nOpCode, DWORD dwAddr)
{
   m_nOpCode = nOpCode;
   m_nSourceLine = nLine;
   m_operand.m_dwAddr = dwAddr;
}

NXSL_Instruction::NXSL_Instruction(int nLine, int nOpCode, int nStackItems)
{
   m_nOpCode = nOpCode;
   m_nSourceLine = nLine;
   m_nStackItems = nStackItems;
}

NXSL_Instruction::NXSL_Instruction(NXSL_Instruction *pSrc)
{
   m_nOpCode = pSrc->m_nOpCode;
   m_nSourceLine = pSrc->m_nSourceLine;
   m_nStackItems = pSrc->m_nStackItems;
   switch(m_nOpCode)
   {
      case OPCODE_PUSH_CONSTANT:
         m_operand.m_pConstant = new NXSL_Value(pSrc->m_operand.m_pConstant);
         break;
      case OPCODE_PUSH_VARIABLE:
      case OPCODE_SET:
      case OPCODE_CALL_EXTERNAL:
      case OPCODE_BIND:
      case OPCODE_ARRAY:
      case OPCODE_INC:
      case OPCODE_DEC:
      case OPCODE_INCP:
      case OPCODE_DECP:
      case OPCODE_GET_ATTRIBUTE:
      case OPCODE_SET_ATTRIBUTE:
		case OPCODE_NAME:
         m_operand.m_pszString = _tcsdup(pSrc->m_operand.m_pszString);
         break;
      default:
         m_operand.m_dwAddr = pSrc->m_operand.m_dwAddr;
         break;
   }
}


//
// Destructor
//

NXSL_Instruction::~NXSL_Instruction()
{
   switch(m_nOpCode)
   {
      case OPCODE_PUSH_VARIABLE:
      case OPCODE_CALL_EXTERNAL:
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
         safe_free(m_operand.m_pszString);
         break;
      case OPCODE_PUSH_CONSTANT:
         delete m_operand.m_pConstant;
         break;
      default:
         break;
   }
}
