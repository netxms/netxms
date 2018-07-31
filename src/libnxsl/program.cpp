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
static const char *s_nxslCommandMnemonic[] =
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
   "EINCP", "EDECP", "ABORT", "CATCH", "PUSH",
   "SETHM", "NEWARR", "NEWHM", "CPOP",
   "SREAD", "SWRITE", "SELECT", "PUSHCP",
   "SINC", "SINCP", "SDEC", "SDECP", "EPEEK",
   "PUSH", "SET", "CALL", "INC", "DEC",
   "INCP", "DECP", "IN", "PUSH", "SET",
   "UPDATE", "CLREXPR"
};

/**
 * Constructor
 */
NXSL_Program::NXSL_Program() : NXSL_ValueManager()
{
   m_instructionSet = new ObjectArray<NXSL_Instruction>(256, 256, true);
   m_constants = new NXSL_ValueHashMap<NXSL_Identifier>(this, true);
   m_functions = new ObjectArray<NXSL_Function>(16, 16, true);
   m_requiredModules = new ObjectArray<NXSL_ModuleImport>(4, 4, true);
   m_expressionVariables = NULL;
}

/**
 * Destructor
 */
NXSL_Program::~NXSL_Program()
{
   delete m_instructionSet;
   delete m_constants;
   delete m_functions;
   delete m_requiredModules;
   delete m_expressionVariables;
}

/**
 * Add new constant. Name expected to be dynamically allocated and
 * will be destroyed by NXSL_Program when no longer needed.
 */
bool NXSL_Program::addConstant(const NXSL_Identifier& name, NXSL_Value *value)
{
   bool success = false;
   if (!m_constants->contains(name))
   {
      m_constants->set(name, value);
      success = true;
   }
   return success;
}

/**
 * Enable expression variables
 */
void NXSL_Program::enableExpressionVariables()
{
   m_expressionVariables = new ObjectArray<NXSL_IdentifierLocation>(16, 16, true);
}

/**
 * Disable expression variables
 */
void NXSL_Program::disableExpressionVariables(int line)
{
   addInstruction(new NXSL_Instruction(this, line, OPCODE_JZ_PEEK, INVALID_ADDRESS));
   for(int i = 0; i < m_expressionVariables->size(); i++)
   {
      const NXSL_IdentifierLocation *l = m_expressionVariables->get(i);
      addInstruction(new NXSL_Instruction(this, line, OPCODE_UPDATE_EXPRVAR, l->m_identifier, 0, l->m_addr));
      addInstruction(new NXSL_Instruction(this, line, OPCODE_SET_EXPRVAR, l->m_identifier, 1));
   }
   delete_and_null(m_expressionVariables);
   resolveLastJump(OPCODE_JZ_PEEK);
   addInstruction(new NXSL_Instruction(this, line, OPCODE_CLEAR_EXPRVARS));
}

/**
 * Register expression variable
 */
void NXSL_Program::registerExpressionVariable(const NXSL_Identifier& identifier)
{
   if (m_expressionVariables != NULL)
      m_expressionVariables->add(new NXSL_IdentifierLocation(identifier, m_instructionSet->size()));
}

/**
 * Get address of expression variable code block. Will return
 * INVALID_ADDRESS if given variable is not registered as expression variable.
 */
UINT32 NXSL_Program::getExpressionVariableCodeBlock(const NXSL_Identifier& identifier)
{
   if (m_expressionVariables == NULL)
      return INVALID_ADDRESS;

   for(int i = 0; i < m_expressionVariables->size(); i++)
   {
      const NXSL_IdentifierLocation *l = m_expressionVariables->get(i);
      if (l->m_identifier.equals(identifier))
      {
         return l->m_addr;
      }
   }
   return INVALID_ADDRESS;
}

/**
 * Add "push variable" instruction
 */
void NXSL_Program::addPushVariableInstruction(const NXSL_Identifier& name, int line)
{
   UINT32 addr = getExpressionVariableCodeBlock(name);
   if (addr == INVALID_ADDRESS)
   {
      addInstruction(new NXSL_Instruction(this, line, OPCODE_PUSH_VARIABLE, name));
   }
   else
   {
      addInstruction(new NXSL_Instruction(this, line, OPCODE_PUSH_EXPRVAR, name, 0, addr));
      addInstruction(new NXSL_Instruction(this, line, OPCODE_SET_EXPRVAR, name));
   }
}

/**
 * Resolve last jump with INVALID_ADDRESS to current address
 */
void NXSL_Program::resolveLastJump(int opcode, int offset)
{
   for(int i = m_instructionSet->size(); i > 0;)
   {
      i--;
      NXSL_Instruction *instr = m_instructionSet->get(i);
      if ((instr->m_opCode == opcode) &&
          (instr->m_operand.m_addr == INVALID_ADDRESS))
      {
         instr->m_operand.m_addr = m_instructionSet->size() + offset;
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

	int nLine = m_instructionSet->get(dwOpAddr)->m_sourceLine;
   m_instructionSet->set(dwOpAddr, new NXSL_Instruction(this, nLine, OPCODE_JMP, dwJumpAddr));
}

/**
 * Add new function to defined functions list
 * Will use first free address if dwAddr == INVALID_ADDRESS
 * Name must be in UTF-8
 */
bool NXSL_Program::addFunction(const NXSL_Identifier& name, UINT32 dwAddr, char *pszError)
{
   // Check for duplicate function names
   for(int i = 0; i < m_functions->size(); i++)
      if (name.equals(m_functions->get(i)->m_name))
      {
         sprintf(pszError, "Duplicate function name: \"%s\"", name.value);
         return false;
      }
   NXSL_Function *f = new NXSL_Function;
	f->m_name = name;
   f->m_dwAddr = (dwAddr == INVALID_ADDRESS) ? m_instructionSet->size() : dwAddr;
   m_functions->add(f);
   return true;
}

/**
 * Add required module name (must be dynamically allocated).
 */
void NXSL_Program::addRequiredModule(const char *name, int lineNumber)
{
   NXSL_ModuleImport *module = new NXSL_ModuleImport();
#ifdef UNICODE
   MultiByteToWideChar(CP_UTF8, 0, name, -1, module->name, MAX_PATH - 1);
#else
   nx_strncpy(module->name, name, MAX_PATH);
#endif
   module->lineNumber = lineNumber;
   m_requiredModules->add(module);
}

/**
 * resolve local functions
 */
void NXSL_Program::resolveFunctions()
{
   for(int i = 0; i < m_instructionSet->size(); i++)
   {
      NXSL_Instruction *instr = m_instructionSet->get(i);
      if (instr->m_opCode == OPCODE_CALL_EXTERNAL)
      {
         for(int j = 0; j < m_functions->size(); j++)
         {
            NXSL_Function *f = m_functions->get(j);
            if (instr->m_operand.m_identifier->equals(f->m_name))
            {
               delete instr->m_operand.m_identifier;
               instr->m_operand.m_addr = f->m_dwAddr;
               instr->m_opCode = OPCODE_CALL;
               break;
            }
         }
      }
   }
}

/**
 * Dump program to file (as text)
 */
void NXSL_Program::dump(FILE *fp, const ObjectArray<NXSL_Instruction> *instructionSet)
{
   for(int i = 0; i < instructionSet->size(); i++)
   {
      const NXSL_Instruction *instr = instructionSet->get(i);
      _ftprintf(fp, _T("%04X  %04X  %-6hs  "), i, instr->m_opCode, s_nxslCommandMnemonic[instr->m_opCode]);
      switch(instr->m_opCode)
      {
         case OPCODE_CALL_EXTERNAL:
         case OPCODE_GLOBAL:
         case OPCODE_SELECT:
            _ftprintf(fp, _T("%hs, %d\n"), instr->m_operand.m_identifier->value, instr->m_stackItems);
            break;
         case OPCODE_CALL:
            _ftprintf(fp, _T("%04X, %d\n"), instr->m_operand.m_addr, instr->m_stackItems);
            break;
         case OPCODE_CALL_METHOD:
            _ftprintf(fp, _T("@%hs, %d\n"), instr->m_operand.m_identifier->value, instr->m_stackItems);
            break;
         case OPCODE_CALL_EXTPTR:
            _ftprintf(fp, _T("%hs, %d\n"), instr->m_operand.m_function->m_name, instr->m_stackItems);
            break;
         case OPCODE_CATCH:
         case OPCODE_JMP:
         case OPCODE_JZ:
         case OPCODE_JNZ:
         case OPCODE_JZ_PEEK:
         case OPCODE_JNZ_PEEK:
            _ftprintf(fp, _T("%04X\n"), instr->m_operand.m_addr);
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
            _ftprintf(fp, _T("%hs\n"), instr->m_operand.m_identifier->value);
            break;
         case OPCODE_SET_EXPRVAR:
            _ftprintf(fp, _T("(%hs), %d\n"), instr->m_operand.m_identifier->value, instr->m_stackItems);
            break;
         case OPCODE_PUSH_EXPRVAR:
            _ftprintf(fp, _T("(%hs), %04X\n"), instr->m_operand.m_identifier->value, instr->m_addr2);
            break;
         case OPCODE_UPDATE_EXPRVAR:
            _ftprintf(fp, _T("(%hs)\n"), instr->m_operand.m_identifier->value);
            break;
         case OPCODE_PUSH_VARPTR:
         case OPCODE_SET_VARPTR:
            _ftprintf(fp, _T("%hs\n"), instr->m_operand.m_variable->getName().value);
            break;
         case OPCODE_PUSH_CONSTANT:
			case OPCODE_CASE:
            if (instr->m_operand.m_constant->isNull())
               _ftprintf(fp, _T("<null>\n"));
            else if (instr->m_operand.m_constant->isArray())
               _ftprintf(fp, _T("<array>\n"));
            else if (instr->m_operand.m_constant->isHashMap())
               _ftprintf(fp, _T("<hash map>\n"));
            else
               _ftprintf(fp, _T("\"%s\"\n"), instr->m_operand.m_constant->getValueAsCString());
            break;
         case OPCODE_POP:
         case OPCODE_PUSHCP:
         case OPCODE_STORAGE_READ:
            _ftprintf(fp, _T("%d\n"), instr->m_stackItems);
            break;
         case OPCODE_CAST:
            _ftprintf(fp, _T("[%s]\n"), g_szTypeNames[instr->m_stackItems]);
            break;
         default:
            _ftprintf(fp, _T("\n"));
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
	if ((instr->m_opCode == OPCODE_JMP) || (instr->m_opCode == srcJump))
		return getFinalJumpDestination(instr->m_operand.m_addr, srcJump);
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
		if ((instr->m_opCode == OPCODE_PUSH_CONSTANT) &&
		    (m_instructionSet->get(i + 1)->m_opCode == OPCODE_NEG) &&
			 instr->m_operand.m_constant->isNumeric() &&
			 !instr->m_operand.m_constant->isUnsigned())
		{
			instr->m_operand.m_constant->negate();
			removeInstructions(i + 1, 1);
		}
	}

	// Convert jumps to address beyond code end to NRETs
	for(i = 0; i < m_instructionSet->size(); i++)
	{
      NXSL_Instruction *instr = m_instructionSet->get(i);
		if ((instr->m_opCode == OPCODE_JMP) && (instr->m_operand.m_addr >= (UINT32)m_instructionSet->size()))
		{
			instr->m_opCode = OPCODE_RET_NULL;
		}
	}

	// Fix destination address for JZP/JNZP jumps
	for(i = 0; i < m_instructionSet->size(); i++)
	{
      NXSL_Instruction *instr = m_instructionSet->get(i);
		if (((instr->m_opCode == OPCODE_JZ_PEEK) &&
			  (m_instructionSet->get(instr->m_operand.m_addr)->m_opCode == OPCODE_JNZ_PEEK)) ||
		    ((instr->m_opCode == OPCODE_JNZ_PEEK) &&
			  (m_instructionSet->get(instr->m_operand.m_addr)->m_opCode == OPCODE_JZ_PEEK)))
		{
			instr->m_operand.m_addr++;
		}
	}

	// Convert jump chains to single jump
	for(i = 0; i < m_instructionSet->size(); i++)
	{
      NXSL_Instruction *instr = m_instructionSet->get(i);
		if ((instr->m_opCode == OPCODE_JMP) ||
			 (instr->m_opCode == OPCODE_JZ) ||
			 (instr->m_opCode == OPCODE_JNZ))
		{
			instr->m_operand.m_addr = getFinalJumpDestination(instr->m_operand.m_addr, -1);
		}
		else if ((instr->m_opCode == OPCODE_JZ_PEEK) ||
			      (instr->m_opCode == OPCODE_JNZ_PEEK))
		{
			instr->m_operand.m_addr = getFinalJumpDestination(instr->m_operand.m_addr, instr->m_opCode);
		}
	}

	// Remove jumps to next instruction
	for(i = 0; i < m_instructionSet->size(); i++)
	{
      NXSL_Instruction *instr = m_instructionSet->get(i);
		if (((instr->m_opCode == OPCODE_JMP) ||
			  (instr->m_opCode == OPCODE_JZ_PEEK) ||
			  (instr->m_opCode == OPCODE_JNZ_PEEK)) &&
			 (instr->m_operand.m_addr == i + 1))
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
		if (((instr->m_opCode == OPCODE_JMP) ||
		     (instr->m_opCode == OPCODE_JZ) ||
		     (instr->m_opCode == OPCODE_JNZ) ||
		     (instr->m_opCode == OPCODE_JZ_PEEK) ||
		     (instr->m_opCode == OPCODE_JNZ_PEEK) ||
           (instr->m_opCode == OPCODE_CATCH) ||
		     (instr->m_opCode == OPCODE_CALL)) &&
		    (instr->m_operand.m_addr > start))
		{
         instr->m_operand.m_addr -= count;
		}
		if ((instr->m_addr2 != INVALID_ADDRESS) && (instr->m_addr2 > start))
		{
         instr->m_addr2 -= count;
		}
	}
}

/**
 * Serialize compiled script
 */
void NXSL_Program::serialize(ByteStream& s)
{
   StringList strings;
   ObjectRefArray<NXSL_Value> constants(64, 64);

   NXSL_FileHeader header;
   memset(&header, 0, sizeof(header));
   memcpy(header.magic, "NXSL", 4);
   header.version = NXSL_BIN_FORMAT_VERSION;
   s.write(&header, sizeof(header));

   // Serialize instructions
   header.codeSectionOffset = htonl((UINT32)s.pos());
   int i;
   for(i = 0; i < m_instructionSet->size(); i++)
   {
      NXSL_Instruction *instr = m_instructionSet->get(i);
      s.write(instr->m_opCode);
      s.write(instr->m_stackItems);
      s.write(instr->m_sourceLine);
      switch(instr->getOperandType())
      {
         case OP_TYPE_ADDR:
            s.write(instr->m_operand.m_addr);
            break;
         case OP_TYPE_IDENTIFIER:
            break;
            /*
         case OP_TYPE_STRING:
            {
               INT32 idx = strings.indexOf(instr->m_operand.m_pszString);
               if (idx == -1)
               {
                  idx = strings.size();
                  strings.add(instr->m_operand.m_pszString);
               }
               s.write(idx);
            }
            */
            break;
         case OP_TYPE_CONST:
            {
               INT32 idx = -1;
               for(int i = 0; i < constants.size(); i++)
               {
                  if (constants.get(i)->equals(instr->m_operand.m_constant))
                  {
                     idx = i;
                     break;
                  }
               }

               if (idx == -1)
               {
                  idx = constants.size();
                  constants.add(instr->m_operand.m_constant);
               }

               s.write(idx);
            }
            break;
         default:
            break;
      }
   }

   // write strings section
   header.stringSectionOffset = htonl((UINT32)s.pos());
   for(i = 0; i < strings.size(); i++)
   {
      s.writeString(strings.get(i));
   }

   // write constants section
   header.constSectionOffset = htonl((UINT32)s.pos());
   for(i = 0; i < constants.size(); i++)
   {
      constants.get(i)->serialize(s);
   }

   // write required modules list
   header.moduleSectionOffset = htonl((UINT32)s.pos());
   for(i = 0; i < m_requiredModules->size(); i++)
   {
      NXSL_ModuleImport *module = m_requiredModules->get(i);
      s.writeString(module->name);
      s.write((INT32)module->lineNumber);
   }

   // write function list
   header.functionSectionOffset = htonl((UINT32)s.pos());
   for(i = 0; i < m_functions->size(); i++)
   {
      NXSL_Function *f = m_functions->get(i);
      s.writeStringUtf8(f->m_name.value);
      s.write(f->m_dwAddr);
   }

   // update header
   s.seek(0);
   s.write(&header, sizeof(header));
}

/**
 * Load from serialized form
 */
NXSL_Program *NXSL_Program::load(ByteStream& s, TCHAR *errMsg, size_t errMsgSize)
{
   NXSL_FileHeader header;
   if (s.read(&header, sizeof(NXSL_FileHeader)) != sizeof(NXSL_FileHeader))
   {
      nx_strncpy(errMsg, _T("Binary file is too small"), errMsgSize);
      return NULL;  // Too small
   }
   if (memcmp(header.magic, "NXSL", 4) || (header.version != NXSL_BIN_FORMAT_VERSION))
   {
      nx_strncpy(errMsg, _T("Binary file header is invalid"), errMsgSize);
      return NULL;  // invalid header
   }

   header.codeSectionOffset = ntohl(header.codeSectionOffset);
   header.constSectionOffset = ntohl(header.constSectionOffset);
   header.functionSectionOffset = ntohl(header.functionSectionOffset);
   header.moduleSectionOffset = ntohl(header.moduleSectionOffset);
   header.stringSectionOffset = ntohl(header.stringSectionOffset);

   // Load strings
   StringList strings;
   s.seek(header.stringSectionOffset);
   while(!s.eos() && (s.pos() < header.constSectionOffset))
   {
      TCHAR *str = s.readString();
      if (str == NULL)
      {
         nx_strncpy(errMsg, _T("Binary file read error (strings section)"), errMsgSize);
         return NULL;   // read error
      }
      strings.addPreallocated(str);
   }

   NXSL_Program *p = new NXSL_Program();

   // Load constants
   ObjectRefArray<NXSL_Value> constants(64, 64);
   s.seek(header.constSectionOffset);
   while(!s.eos() && (s.pos() < header.moduleSectionOffset))
   {
      NXSL_Value *v = NXSL_Value::load(p, s);
      if (v == NULL)
      {
         nx_strncpy(errMsg, _T("Binary file read error (constants section)"), errMsgSize);
         goto failure;   // read error
      }
      constants.add(v);
   }   

   // Load code
   s.seek(header.codeSectionOffset);
   while(!s.eos() && (s.pos() < header.stringSectionOffset))
   {
      INT16 opcode = s.readInt16();
      INT16 si = s.readInt16();
      INT32 line = s.readInt32();
      NXSL_Instruction *instr = new NXSL_Instruction(p, line, opcode, si);
      switch(instr->getOperandType())
      {
         case OP_TYPE_ADDR:
            instr->m_operand.m_addr = s.readUInt32();
            break;
            /*
         case OP_TYPE_STRING:
            {
               INT32 idx = s.readInt32();
               const TCHAR *str = strings.get(idx);
               if (str == NULL)
               {
                  _sntprintf(errMsg, errMsgSize, _T("Binary file read error (instruction %04X)"), p->m_instructionSet->size());
                  delete instr;
                  goto failure;
               }
               instr->m_operand.m_pszString = _tcsdup(str);
            }
            break;
            */
         case OP_TYPE_CONST:
            {
               INT32 idx = s.readInt32();
               NXSL_Value *v = constants.get(idx);
               if (v == NULL)
               {
                  _sntprintf(errMsg, errMsgSize, _T("Binary file read error (instruction %04X)"), p->m_instructionSet->size());
                  delete instr;
                  goto failure;
               }
               instr->m_operand.m_constant = p->createValue(v);
            }
            break;
         default:
            break;
      }
      p->m_instructionSet->add(instr);
   }

   // Load module list
   s.seek(header.moduleSectionOffset);
   while(!s.eos() && (s.pos() < header.functionSectionOffset))
   {
      TCHAR *name = s.readString();
      if (name == NULL)
      {
         nx_strncpy(errMsg, _T("Binary file read error (modules section)"), errMsgSize);
         goto failure;
      }

      NXSL_ModuleImport *module = new NXSL_ModuleImport();
      nx_strncpy(module->name, name, MAX_PATH);
      free(name);
      module->lineNumber = s.readInt32();

      p->m_requiredModules->add(module);
   }

   // Load function list
   s.seek(header.functionSectionOffset);
   while(!s.eos())
   {
      char *name = s.readStringUtf8();
      if (name == NULL)
      {
         nx_strncpy(errMsg, _T("Binary file read error (functions section)"), errMsgSize);
         goto failure;
      }
      p->m_functions->add(new NXSL_Function(name, s.readUInt32()));
      free(name);
   }

   for(int i = 0; i < constants.size(); i++)
      p->destroyValue(constants.get(i));

   return p;

failure:
   for(int i = 0; i < constants.size(); i++)
      p->destroyValue(constants.get(i));

   delete p;
   return NULL;
}
