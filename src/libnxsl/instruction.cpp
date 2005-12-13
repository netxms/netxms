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
** $module: instruction.cpp
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
   m_operand.m_pszString = pszString;
   m_nStackItems = 0;
}

NXSL_Instruction::NXSL_Instruction(int nLine, int nOpCode, char *pszString, int nStackItems)
{
   m_nOpCode = nOpCode;
   m_nSourceLine = nLine;
   m_operand.m_pszString = pszString;
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
         safe_free(m_operand.m_pszString);
         break;
      case OPCODE_PUSH_CONSTANT:
         delete m_operand.m_pConstant;
         break;
      default:
         break;
   }
}
