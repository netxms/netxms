/* 
** NetXMS - Network Management System
** NetXMS Scripting Language Interpreter
** Copyright (C) 2003-2024 Victor Kirhenshtein
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
   "NRET", "JZ", "IDIV", "CONCAT",
   "BIND", "INC", "DEC", "NEG", "NOT",
   "BITNOT", "CAST", "AGET", "INCP", "DECP",
   "JNZ", "LIKE", "ILIKE", "MATCH",
   "IMATCH", "CASE", "ARRAY", "EGET",
	"ESET", "ASET", "NAME", "FOREACH", "NEXT",
	"GLOBAL", "GARRAY", "JZP", "JNZP", "APPEND",
	"AGETS", "CALL", "CASE", "EINC", "EDEC",
   "EINCP", "EDECP", "ABORT", "CATCH", "PUSH",
   "SETHM", "NEWARR", "NEWHM", "CPOP",
   "SREAD", "SWRITE", "SELECT", "PUSHCP",
   "SINC", "SINCP", "SDEC", "SDECP", "EPEEK",
   "PUSH", "SET", "CALL", "INC", "DEC",
   "INCP", "DECP", "IN", "PUSH", "SET",
   "UPDATE", "CLREXPR", "RANGE", "CASELT",
   "CASELT", "CASEGT", "CASEGT", "PUSH",
   "PUSH", "PUSH", "PUSH", "PUSH", "PUSH",
   "PUSH", "PUSH", "SPREAD", "ARGV", "APPEND",
   "FSTR", "CALL", "CALLS", "HASBITS", "LOCAL"
};

/**
 * Constructor
 */
NXSL_ProgramBuilder::NXSL_ProgramBuilder(NXSL_Environment *env) : NXSL_ValueManager(), m_instructionSet(256, 256),
         m_requiredModules(0, 8), m_constants(this, Ownership::True), m_functions(64, 64)
{
   m_environment = env;
   m_expressionVariables = nullptr;
   m_numFStringElements = 0;
}

/**
 * Destructor
 */
NXSL_ProgramBuilder::~NXSL_ProgramBuilder()
{
   for(int i = 0; i < m_instructionSet.size(); i++)
      m_instructionSet.get(i)->dispose(this);
   delete m_expressionVariables;
}

/**
 * Add new constant. Value should be created by same program builder.
 */
bool NXSL_ProgramBuilder::addConstant(const NXSL_Identifier& name, NXSL_Value *value)
{
   bool success = false;
   if (!m_constants.contains(name))
   {
      m_constants.set(name, value);
      success = true;
   }
   else
   {
      destroyValue(value);
   }
   return success;
}

/**
 * Get value of named constant
 */
NXSL_Value *NXSL_ProgramBuilder::getConstantValue(const NXSL_Identifier& name)
{
   NXSL_Value *v = m_constants.get(name);
   if (v != nullptr)
      return createValue(v);
   return (m_environment != nullptr) ? m_environment->getConstantValue(name, this) : nullptr;
}

/**
 * Enable expression variables
 */
void NXSL_ProgramBuilder::enableExpressionVariables()
{
   m_expressionVariables = new StructArray<NXSL_IdentifierLocation>(16, 16);
}

/**
 * Disable expression variables
 */
void NXSL_ProgramBuilder::disableExpressionVariables(int line)
{
   addInstruction(line, OPCODE_JZ_PEEK, INVALID_ADDRESS);
   for(int i = 0; i < m_expressionVariables->size(); i++)
   {
      const NXSL_IdentifierLocation *l = m_expressionVariables->get(i);
      addInstruction(line, OPCODE_UPDATE_EXPRVAR, l->m_identifier, 0, l->m_addr);
      addInstruction(line, OPCODE_SET_EXPRVAR, l->m_identifier, 1);
   }
   delete_and_null(m_expressionVariables);
   resolveLastJump(OPCODE_JZ_PEEK);
   addInstruction(line, OPCODE_CLEAR_EXPRVARS);
}

/**
 * Register expression variable
 */
void NXSL_ProgramBuilder::registerExpressionVariable(const NXSL_Identifier& identifier)
{
   if (m_expressionVariables != nullptr)
      m_expressionVariables->add(NXSL_IdentifierLocation(identifier, m_instructionSet.size()));
   m_currentMetadataPrefix = identifier;
}

/**
 * Get address of expression variable code block. Will return
 * INVALID_ADDRESS if given variable is not registered as expression variable.
 */
uint32_t NXSL_ProgramBuilder::getExpressionVariableCodeBlock(const NXSL_Identifier& identifier)
{
   if (m_expressionVariables == nullptr)
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
void NXSL_ProgramBuilder::addPushVariableInstruction(const NXSL_Identifier& name, int line)
{
   uint32_t addr = getExpressionVariableCodeBlock(name);
   if (addr == INVALID_ADDRESS)
   {
      addInstruction(line, OPCODE_PUSH_VARIABLE, name);
   }
   else
   {
      addInstruction(line, OPCODE_PUSH_EXPRVAR, name, 0, addr);
      addInstruction(line, OPCODE_SET_EXPRVAR, name);
   }
}

/**
 * Resolve last jump with INVALID_ADDRESS to current address
 */
void NXSL_ProgramBuilder::resolveLastJump(int opcode, int offset)
{
   for(int i = m_instructionSet.size(); i > 0;)
   {
      i--;
      NXSL_Instruction *instr = m_instructionSet.get(i);
      if ((instr->m_opCode == opcode) &&
          (instr->m_operand.m_addr == INVALID_ADDRESS))
      {
         instr->m_operand.m_addr = m_instructionSet.size() + offset;
         break;
      }
   }
}

/**
 * Create jump at given address replacing another instruction (usually NOP)
 */
void NXSL_ProgramBuilder::createJumpAt(uint32_t opAddr, uint32_t jumpAddr)
{
	if (opAddr >= (uint32_t)m_instructionSet.size())
		return;

	NXSL_Instruction *i = m_instructionSet.get(opAddr);
	i->dispose(this);
	i->m_opCode = OPCODE_JMP;
	i->m_operand.m_addr = jumpAddr;
}

/**
 * Add new function to defined functions list
 * Will use first free address if dwAddr == INVALID_ADDRESS
 * Name must be in UTF-8
 */
bool NXSL_ProgramBuilder::addFunction(const NXSL_Identifier& name, uint32_t addr, char *errorText)
{
   // Check for duplicate function names
   for(int i = 0; i < m_functions.size(); i++)
      if (name.equals(m_functions.get(i)->m_name))
      {
         sprintf(errorText, "Duplicate function name: \"%s\"", name.value);
         return false;
      }
   NXSL_Function f(name, (addr == INVALID_ADDRESS) ? m_instructionSet.size() : addr);
   m_functions.add(f);
   return true;
}

/**
 * Add required module name (must be dynamically allocated).
 */
void NXSL_ProgramBuilder::addRequiredModule(const char *name, int lineNumber, bool removeLastElement, bool fullImport, bool optional)
{
#ifdef UNICODE
   WCHAR mname[MAX_PATH];
   size_t cch = utf8_to_wchar(name, -1, mname, MAX_PATH - 1);
#else
   char mname[MAX_PATH];
   size_t cch = utf8_to_mb(name, -1, mname, MAX_PATH);
#endif
   mname[cch] = 0;

   if (removeLastElement)
   {
      TCHAR *s = mname;
      TCHAR *p = _tcsstr(s, _T("::"));
      while(p != nullptr)
      {
         s = p;
         p = _tcsstr(s + 2, _T("::"));
      }
      *s = 0;
   }

   // Check if module already added
   for(int i = 0; i < m_requiredModules.size(); i++)
   {
      NXSL_ModuleImport *module = m_requiredModules.get(i);
      if (!_tcscmp(module->name, mname))
      {
         if ((module->flags & MODULE_IMPORT_OPTIONAL) && !optional)
            module->flags &= ~MODULE_IMPORT_OPTIONAL;
         if (!(module->flags & MODULE_IMPORT_FULL) && fullImport)
            module->flags |= MODULE_IMPORT_FULL;
         return;
      }
   }

   NXSL_ModuleImport *module = m_requiredModules.addPlaceholder();
   _tcslcpy(module->name, mname, MAX_IDENTIFIER_LENGTH);
   module->lineNumber = lineNumber;
   module->flags =
            (fullImport ? MODULE_IMPORT_FULL : 0) |
            (optional ? MODULE_IMPORT_OPTIONAL : 0) |
            ((mname[_tcslen(mname) - 1] == '*') ? MODULE_IMPORT_WILDCARD : 0);
}

/**
 * Finalize import
 */
void NXSL_ProgramBuilder::finalizeSymbolImport(const char *module)
{
   if (m_importSymbols.isEmpty())
      return;

   NXSL_FunctionImport f(module);
   for(int i = 0; i < m_importSymbols.size(); i++)
   {
      import_symbol_t *s = m_importSymbols.get(i);
      f.name = s->name;
      f.alias = s->alias;
      f.lineNumber = s->lineNumber;
      m_importedFunctions.add(f);
   }

   m_importSymbols.clear();
}

/**
 * Resolve local functions
 */
void NXSL_ProgramBuilder::resolveFunctions()
{
   for(int i = 0; i < m_instructionSet.size(); i++)
   {
      NXSL_Instruction *instr = m_instructionSet.get(i);
      if (instr->m_opCode == OPCODE_CALL_EXTERNAL)
      {
         for(int j = 0; j < m_functions.size(); j++)
         {
            NXSL_Function *f = m_functions.get(j);
            if (instr->m_operand.m_identifier->equals(f->m_name))
            {
               destroyIdentifier(instr->m_operand.m_identifier);
               instr->m_operand.m_addr = f->m_addr;
               instr->m_opCode = OPCODE_CALL;
               break;
            }
         }
      }
   }
}

/**
 * Dump single instruction to file (as text)
 */
void NXSL_ProgramBuilder::dump(FILE *fp, uint32_t addr, const NXSL_Instruction& instruction)
{
   _ftprintf(fp, _T("%04X  %04X  %-6hs  "), addr, instruction.m_opCode, s_nxslCommandMnemonic[instruction.m_opCode]);
   switch(instruction.m_opCode)
   {
      case OPCODE_CALL_EXTERNAL:
         if (instruction.m_addr2 == OPTIONAL_FUNCTION_CALL)
            _ftprintf(fp, _T("[optional] %hs, %d\n"), instruction.m_operand.m_identifier->value, instruction.m_stackItems);
         else
            _ftprintf(fp, _T("%hs, %d\n"), instruction.m_operand.m_identifier->value, instruction.m_stackItems);
         break;
      case OPCODE_GLOBAL:
      case OPCODE_LOCAL:
      case OPCODE_SELECT:
         _ftprintf(fp, _T("%hs, %d\n"), instruction.m_operand.m_identifier->value, instruction.m_stackItems);
         break;
      case OPCODE_CALL:
         _ftprintf(fp, _T("%04X, %d\n"), instruction.m_operand.m_addr, instruction.m_stackItems);
         break;
      case OPCODE_CALL_EXTPTR:
         _ftprintf(fp, _T("%hs, %d\n"), instruction.m_operand.m_function->m_name.value, instruction.m_stackItems);
         break;
      case OPCODE_CALL_INDIRECT:
         _ftprintf(fp, _T("(%d)\n"), instruction.m_stackItems);
         break;
      case OPCODE_CALL_METHOD:
      case OPCODE_SAFE_CALL:
         _ftprintf(fp, _T("@%hs, %d\n"), instruction.m_operand.m_identifier->value, instruction.m_stackItems);
         break;
      case OPCODE_CATCH:
      case OPCODE_JMP:
      case OPCODE_JZ:
      case OPCODE_JNZ:
      case OPCODE_JZ_PEEK:
      case OPCODE_JNZ_PEEK:
         _ftprintf(fp, _T("%04X\n"), instruction.m_operand.m_addr);
         break;
      case OPCODE_PUSH_CONSTREF:
      case OPCODE_PUSH_VARIABLE:
      case OPCODE_PUSH_PROPERTY:
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
      case OPCODE_CASE_CONST_LT:
      case OPCODE_CASE_CONST_GT:
         _ftprintf(fp, _T("%hs\n"), instruction.m_operand.m_identifier->value);
         break;
      case OPCODE_SET:
         _ftprintf(fp, _T("%hs, %d\n"), instruction.m_operand.m_identifier->value, instruction.m_stackItems);
         break;
      case OPCODE_SET_EXPRVAR:
         _ftprintf(fp, _T("(%hs), %d\n"), instruction.m_operand.m_identifier->value, instruction.m_stackItems);
         break;
      case OPCODE_PUSH_EXPRVAR:
         _ftprintf(fp, _T("(%hs), %04X\n"), instruction.m_operand.m_identifier->value, instruction.m_addr2);
         break;
      case OPCODE_UPDATE_EXPRVAR:
         _ftprintf(fp, _T("(%hs), %04X\n"), instruction.m_operand.m_identifier->value, instruction.m_addr2);
         break;
      case OPCODE_PUSH_VARPTR:
         _ftprintf(fp, _T("%hs\n"), instruction.m_operand.m_variable->getName().value);
         break;
      case OPCODE_SET_VARPTR:
         _ftprintf(fp, _T("%hs, %d\n"), instruction.m_operand.m_variable->getName().value, instruction.m_stackItems);
         break;
      case OPCODE_PUSH_CONSTANT:
      case OPCODE_CASE:
      case OPCODE_CASE_LT:
      case OPCODE_CASE_GT:
         if (instruction.m_operand.m_constant->isNull())
            _ftprintf(fp, _T("<null>\n"));
         else if (instruction.m_operand.m_constant->isArray())
            _ftprintf(fp, _T("<array>\n"));
         else if (instruction.m_operand.m_constant->isHashMap())
            _ftprintf(fp, _T("<hash map>\n"));
         else
            _ftprintf(fp, _T("\"%s\"\n"), instruction.m_operand.m_constant->getValueAsCString());
         break;
      case OPCODE_FSTRING:
      case OPCODE_POP:
      case OPCODE_PUSHCP:
      case OPCODE_STORAGE_READ:
      case OPCODE_SET_ELEMENT:
         _ftprintf(fp, _T("%d\n"), instruction.m_stackItems);
         break;
      case OPCODE_CAST:
         _ftprintf(fp, _T("[%s]\n"), g_szTypeNames[instruction.m_stackItems]);
         break;
      case OPCODE_PUSH_NULL:
         _ftprintf(fp, _T("null\n"));
         break;
      case OPCODE_PUSH_TRUE:
         _ftprintf(fp, _T("true\n"));
         break;
      case OPCODE_PUSH_FALSE:
         _ftprintf(fp, _T("false\n"));
         break;
      case OPCODE_PUSH_INT32:
         _ftprintf(fp, _T("%d\n"), instruction.m_operand.m_valueInt32);
         break;
      case OPCODE_PUSH_INT64:
         _ftprintf(fp, INT64_FMT _T("L\n"), instruction.m_operand.m_valueInt64);
         break;
      case OPCODE_PUSH_UINT32:
         _ftprintf(fp, _T("%uU\n"), instruction.m_operand.m_valueUInt32);
         break;
      case OPCODE_PUSH_UINT64:
         _ftprintf(fp, UINT64_FMT _T("UL\n"), instruction.m_operand.m_valueUInt64);
         break;
      case OPCODE_APPEND:
         _ftprintf(fp, _T("ELEMENT\n"));
         break;
      case OPCODE_APPEND_ALL:
         _ftprintf(fp, _T("ALL\n"));
         break;
      default:
         _ftprintf(fp, _T("\n"));
         break;
   }
}

/**
 * Dump program to file (as text)
 */
void NXSL_ProgramBuilder::dump(FILE *fp, const StructArray<NXSL_Instruction>& instructionSet)
{
   for(int i = 0; i < instructionSet.size(); i++)
      dump(fp, i, *instructionSet.get(i));
}

/**
 * Get final jump destination from a jump chain
 */
uint32_t NXSL_ProgramBuilder::getFinalJumpDestination(uint32_t addr, int srcJump)
{
   NXSL_Instruction *instr = m_instructionSet.get(addr);
	if ((instr->m_opCode == OPCODE_JMP) || (instr->m_opCode == srcJump))
		return getFinalJumpDestination(instr->m_operand.m_addr, srcJump);
	return addr;
}

/**
 * Check if constant represented as NXSL_Value object can be converted to numeric representation
 */
template<typename T> static inline bool ValidateConstantConversion(T intValue, const TCHAR *format, const TCHAR *strValue)
{
   TCHAR buffer[32];
   _sntprintf(buffer, 32, format, intValue);
   return !_tcscmp(buffer, strValue);
}

/**
 * Optimize compiled program
 */
void NXSL_ProgramBuilder::optimize()
{
	int i;

	// Convert push constant followed by NEG to single push constant
	// Convert push integer and boolean constants to special push instructions
	for(i = 0; (m_instructionSet.size() > 1) && (i < m_instructionSet.size() - 1); i++)
	{
      NXSL_Instruction *instr = m_instructionSet.get(i);
		if (instr->m_opCode != OPCODE_PUSH_CONSTANT)
		   continue;

		if ((m_instructionSet.get(i + 1)->m_opCode == OPCODE_NEG) &&
			 instr->m_operand.m_constant->isNumeric() &&
			 !instr->m_operand.m_constant->isUnsigned())
		{
			instr->m_operand.m_constant->negate();
			removeInstructions(i + 1, 1);
		}

		switch(instr->m_operand.m_constant->getDataType())
		{
		   case NXSL_DT_BOOLEAN:
		      instr->m_opCode = instr->m_operand.m_constant->isTrue() ? OPCODE_PUSH_TRUE : OPCODE_PUSH_FALSE;
            destroyValue(instr->m_operand.m_constant);
            break;
         case NXSL_DT_NULL:
            instr->m_opCode = OPCODE_PUSH_NULL;
            destroyValue(instr->m_operand.m_constant);
            break;
         case NXSL_DT_INT32:
            if (ValidateConstantConversion(instr->m_operand.m_constant->getValueAsInt32(), _T("%d"), instr->m_operand.m_constant->getValueAsCString()))
            {
               instr->m_opCode = OPCODE_PUSH_INT32;
               int32_t i32 = instr->m_operand.m_constant->getValueAsInt32();
               destroyValue(instr->m_operand.m_constant);
               instr->m_operand.m_valueInt32 = i32;
            }
            break;
         case NXSL_DT_INT64:
            if (ValidateConstantConversion(instr->m_operand.m_constant->getValueAsInt64(), INT64_FMT, instr->m_operand.m_constant->getValueAsCString()))
            {
               instr->m_opCode = OPCODE_PUSH_INT64;
               int64_t i64 = instr->m_operand.m_constant->getValueAsInt64();
               destroyValue(instr->m_operand.m_constant);
               instr->m_operand.m_valueInt64 = i64;
            }
            break;
         case NXSL_DT_UINT32:
            if (ValidateConstantConversion(instr->m_operand.m_constant->getValueAsUInt32(), _T("%u"), instr->m_operand.m_constant->getValueAsCString()))
            {
               instr->m_opCode = OPCODE_PUSH_UINT32;
               uint32_t u32 = instr->m_operand.m_constant->getValueAsUInt32();
               destroyValue(instr->m_operand.m_constant);
               instr->m_operand.m_valueInt32 = u32;
            }
            break;
         case NXSL_DT_UINT64:
            if (ValidateConstantConversion(instr->m_operand.m_constant->getValueAsUInt64(), UINT64_FMT, instr->m_operand.m_constant->getValueAsCString()))
            {
               instr->m_opCode = OPCODE_PUSH_UINT64;
               uint64_t u64 = instr->m_operand.m_constant->getValueAsUInt64();
               destroyValue(instr->m_operand.m_constant);
               instr->m_operand.m_valueUInt64 = u64;
            }
            break;
		   default:
		      break;
		}
	}

	// Convert jumps to address beyond code end to NRETs
	for(i = 0; i < m_instructionSet.size(); i++)
	{
      NXSL_Instruction *instr = m_instructionSet.get(i);
		if ((instr->m_opCode == OPCODE_JMP) && (instr->m_operand.m_addr >= static_cast<uint32_t>(m_instructionSet.size())))
		{
			instr->m_opCode = OPCODE_RET_NULL;
		}
	}

	// Fix destination address for JZP/JNZP jumps
	for(i = 0; i < m_instructionSet.size(); i++)
	{
      NXSL_Instruction *instr = m_instructionSet.get(i);
		if (((instr->m_opCode == OPCODE_JZ_PEEK) &&
			  (m_instructionSet.get(instr->m_operand.m_addr)->m_opCode == OPCODE_JNZ_PEEK)) ||
		    ((instr->m_opCode == OPCODE_JNZ_PEEK) &&
			  (m_instructionSet.get(instr->m_operand.m_addr)->m_opCode == OPCODE_JZ_PEEK)))
		{
			instr->m_operand.m_addr++;
		}
	}

	// Convert jump chains to single jump
	for(i = 0; i < m_instructionSet.size(); i++)
	{
      NXSL_Instruction *instr = m_instructionSet.get(i);
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
	for(i = 0; i < m_instructionSet.size(); i++)
	{
      NXSL_Instruction *instr = m_instructionSet.get(i);
		if (((instr->m_opCode == OPCODE_JMP) ||
			  (instr->m_opCode == OPCODE_JZ_PEEK) ||
			  (instr->m_opCode == OPCODE_JNZ_PEEK)) &&
			 (instr->m_operand.m_addr == static_cast<uint32_t>(i + 1)))
		{
			removeInstructions(i, 1);
			i--;
		}
	}

   // Convert SET/ESET followed by POP 1 to SET/ESET with POP
   for(i = 0; (m_instructionSet.size() > 1) && (i < m_instructionSet.size() - 1); i++)
   {
      NXSL_Instruction *instr = m_instructionSet.get(i);
      if (((instr->m_opCode == OPCODE_SET) || (instr->m_opCode == OPCODE_SET_ELEMENT)) &&
          (instr->m_stackItems == 0) &&
          (m_instructionSet.get(i + 1)->m_opCode == OPCODE_POP) &&
          (m_instructionSet.get(i + 1)->m_stackItems == 1))
      {
         instr->m_stackItems = 1;
         removeInstructions(i + 1, 1);
      }
   }
}

/**
 * Remove one or more instructions starting at given position.
 *
 * @param start start offset
 * @param count number of instructions to remove
 */
void NXSL_ProgramBuilder::removeInstructions(uint32_t start, int count)
{
	if ((count <= 0) || (start + (uint32_t)count >= (uint32_t)m_instructionSet.size()))
		return;

   int i;
	for(i = 0; i < count; i++)
	{
      m_instructionSet.get(start)->dispose(this);
      m_instructionSet.remove(start);
	}

	// Change jump destination addresses
	for(i = 0; i < m_instructionSet.size(); i++)
	{
      NXSL_Instruction *instr = m_instructionSet.get(i);
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
		if ((instr->m_addr2 != INVALID_ADDRESS) && (instr->m_addr2 != OPTIONAL_FUNCTION_CALL) && (instr->m_addr2 > start))
		{
         instr->m_addr2 -= count;
		}
	}

	// Update function table
   for(i = 0; i < m_functions.size(); i++)
   {
      NXSL_Function *f = m_functions.get(i);
      if (f->m_addr > start)
      {
         f->m_addr -= count;
      }
   }
}

/**
 * Get list of required module names
 */
StringList *NXSL_ProgramBuilder::getRequiredModules() const
{
   StringList *modules = new StringList();
   for(int i = 0; i < m_requiredModules.size(); i++)
      modules->add(m_requiredModules.get(i)->name);
   return modules;
}

/**
 * Get estimated memory usage
 */
uint64_t NXSL_ProgramBuilder::getMemoryUsage() const
{
   uint64_t mem = NXSL_ValueManager::getMemoryUsage();
   mem += m_instructionSet.size() * sizeof(NXSL_Instruction) + sizeof(m_instructionSet);
   mem += m_requiredModules.size() * sizeof(NXSL_ModuleImport) + sizeof(m_requiredModules);
   mem += m_functions.size() * sizeof(NXSL_Function) + sizeof(m_functions);
   if (m_expressionVariables != nullptr)
      mem += m_expressionVariables->size() * sizeof(NXSL_IdentifierLocation);
   return mem;
}

/**
 * Create empty compiled script
 */
NXSL_Program::NXSL_Program(size_t valueRegionSize, size_t identifierRegionSize) : NXSL_ValueManager(valueRegionSize, identifierRegionSize),
         m_instructionSet(0, 256), m_requiredModules(0, 16), m_constants(this, Ownership::True), m_functions(0, 64)
{
}

/**
 * Constant copy callback
 */
EnumerationCallbackResult CopyConstantsCallback(const void *key, void *value, void *data)
{
   static_cast<NXSL_ValueHashMap<NXSL_Identifier>*>(data)->set(*static_cast<const NXSL_Identifier*>(key),
            static_cast<NXSL_ValueHashMap<NXSL_Identifier>*>(data)->vm()->createValue(static_cast<NXSL_Value*>(value)));
   return _CONTINUE;
}

/**
 * Create compiled script object from code builder
 */
NXSL_Program::NXSL_Program(NXSL_ProgramBuilder *builder) :
         NXSL_ValueManager(builder->m_values.getElementCount(), builder->m_identifiers.getElementCount()),
         m_instructionSet(builder->m_instructionSet.size(), 256),
         m_requiredModules(builder->m_requiredModules),
         m_importedFunctions(builder->m_importedFunctions),
         m_constants(this, Ownership::True),
         m_functions(builder->m_functions),
         m_metadata(builder->m_metadata)
{
   for(int i = 0; i < builder->m_instructionSet.size(); i++)
      m_instructionSet.addPlaceholder()->copyFrom(builder->m_instructionSet.get(i), this);
   builder->m_constants.forEach(CopyConstantsCallback, &m_constants);
}

/**
 * Destructor
 */
NXSL_Program::~NXSL_Program()
{
   for(int i = 0; i < m_instructionSet.size(); i++)
      m_instructionSet.get(i)->dispose(this);
}

/**
 * Check if this program is empty
 */
bool NXSL_Program::isEmpty() const
{
   return m_instructionSet.isEmpty() || ((m_instructionSet.size() == 1) && (m_instructionSet.get(0)->m_opCode == OPCODE_RET_NULL));
}

/**
 * Get list of required module names
 */
StringList *NXSL_Program::getRequiredModules(bool withFlags) const
{
   StringList *modules = new StringList();
   if (withFlags)
   {
      for(int i = 0; i < m_requiredModules.size(); i++)
      {
         NXSL_ModuleImport *m = m_requiredModules.get(i);
         if (m->flags != 0)
         {
            StringBuffer s(m->name);
            s.append(_T(" ["));
            if (m->flags & MODULE_IMPORT_FULL)
               s.append(_T("full"));
            if (m->flags & MODULE_IMPORT_OPTIONAL)
            {
               if (!s.endsWith(_T("[")))
                  s.append(_T(", "));
               s.append(_T("optional"));
            }
            if (m->flags & MODULE_IMPORT_WILDCARD)
            {
               if (!s.endsWith(_T("[")))
                  s.append(_T(", "));
               s.append(_T("wildcard"));
            }
            s.append(_T("]"));
            modules->add(s);
         }
         else
         {
            modules->add(m->name);
         }
      }
   }
   else
   {
      for(int i = 0; i < m_requiredModules.size(); i++)
         modules->add(m_requiredModules.get(i)->name);
   }
   return modules;
}

/**
 * Get estimated memory usage
 */
uint64_t NXSL_Program::getMemoryUsage() const
{
   uint64_t mem = NXSL_ValueManager::getMemoryUsage();
   mem += m_instructionSet.size() * sizeof(NXSL_Instruction) + sizeof(m_instructionSet);
   mem += m_requiredModules.size() * sizeof(NXSL_ModuleImport) + sizeof(m_requiredModules);
   mem += m_functions.size() * sizeof(NXSL_Function) + sizeof(m_functions);
   return mem;
}

/**
 * Dump program code
 */
void NXSL_Program::dump(FILE *fp) const
{
   NXSL_ProgramBuilder::dump(fp, m_instructionSet);
}

/**
 * Serialize compiled script
 */
void NXSL_Program::serialize(ByteStream& s) const
{
   ObjectRefArray<NXSL_Value> constants(64, 64);

   NXSL_FileHeader header;
   memset(&header, 0, sizeof(header));
   memcpy(header.magic, "NXSL", 4);
   header.version = NXSL_BIN_FORMAT_VERSION;
   header.valueRegionSizeHint = htonl(static_cast<uint32_t>(m_values.getElementCount()));
   header.identifierRegionSizeHint = htonl(static_cast<uint32_t>(m_identifiers.getElementCount()));
   s.write(&header, sizeof(header));

   // Serialize instructions
   header.codeSectionOffset = htonl((UINT32)s.pos());
   int i;
   for(i = 0; i < m_instructionSet.size(); i++)
   {
      NXSL_Instruction *instr = m_instructionSet.get(i);
      s.writeB(instr->m_opCode);
      s.writeB(instr->m_stackItems);
      s.writeB(instr->m_sourceLine);
      switch(instr->getOperandType())
      {
         case OP_TYPE_ADDR:
            s.writeB(instr->m_operand.m_addr);
            break;
         case OP_TYPE_CONST:
            {
               int32_t idx = -1;
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

               s.writeB(idx);
            }
            break;
         case OP_TYPE_IDENTIFIER:
            s.write(instr->m_operand.m_identifier->length);
            s.write(instr->m_operand.m_identifier->value, instr->m_operand.m_identifier->length);
            break;
         case OP_TYPE_INT32:
            s.writeB(instr->m_operand.m_valueInt32);
            break;
         case OP_TYPE_INT64:
            s.writeB(instr->m_operand.m_valueInt64);
            break;
         case OP_TYPE_UINT32:
            s.writeB(instr->m_operand.m_valueUInt32);
            break;
         case OP_TYPE_UINT64:
            s.writeB(instr->m_operand.m_valueUInt64);
            break;
         default:
            break;
      }
   }

   // write constants section
   header.constSectionOffset = htonl((UINT32)s.pos());
   for(i = 0; i < constants.size(); i++)
   {
      constants.get(i)->serialize(s);
   }

   // write required modules list
   header.moduleSectionOffset = htonl((UINT32)s.pos());
   for(i = 0; i < m_requiredModules.size(); i++)
   {
      NXSL_ModuleImport *module = m_requiredModules.get(i);
      s.writeString(module->name, "UTF-8", -1, true, false);
      s.writeB(static_cast<int32_t>(module->lineNumber));
      s.write(module->flags);
   }

   // write function list
   header.functionSectionOffset = htonl((UINT32)s.pos());
   for(i = 0; i < m_functions.size(); i++)
   {
      NXSL_Function *f = m_functions.get(i);
      s.writeString(f->m_name.value, -1, true, false);
      s.writeB(f->m_addr);
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
      _tcslcpy(errMsg, _T("Binary file is too small"), errMsgSize);
      return nullptr;  // Too small
   }
   if (memcmp(header.magic, "NXSL", 4) || (header.version != NXSL_BIN_FORMAT_VERSION))
   {
      _tcslcpy(errMsg, _T("Binary file header is invalid"), errMsgSize);
      return nullptr;  // invalid header
   }

   header.codeSectionOffset = ntohl(header.codeSectionOffset);
   header.constSectionOffset = ntohl(header.constSectionOffset);
   header.functionSectionOffset = ntohl(header.functionSectionOffset);
   header.moduleSectionOffset = ntohl(header.moduleSectionOffset);
   header.valueRegionSizeHint = ntohl(header.valueRegionSizeHint);
   header.identifierRegionSizeHint = ntohl(header.identifierRegionSizeHint);

   NXSL_Program *p = new NXSL_Program(MAX(header.valueRegionSizeHint, 4), MAX(header.identifierRegionSizeHint, 4));

   // Load constants
   ObjectRefArray<NXSL_Value> constants(64, 64);
   s.seek(header.constSectionOffset);
   while(!s.eos() && (s.pos() < header.moduleSectionOffset))
   {
      NXSL_Value *v = NXSL_Value::load(p, s);
      if (v == nullptr)
      {
         _tcslcpy(errMsg, _T("Binary file read error (constants section)"), errMsgSize);
         goto failure;   // read error
      }
      constants.add(v);
   }   

   // Load code
   s.seek(header.codeSectionOffset);
   while(!s.eos() && (s.pos() < header.constSectionOffset))
   {
      int16_t opcode = s.readInt16B();
      int16_t stackItems = s.readInt16B();
      int32_t line = s.readInt32B();
      NXSL_Instruction *instr = p->m_instructionSet.addPlaceholder();
      instr->m_sourceLine = line;
      instr->m_opCode = opcode;
      instr->m_stackItems = stackItems;
      switch(instr->getOperandType())
      {
         case OP_TYPE_ADDR:
            instr->m_operand.m_addr = s.readUInt32B();
            break;
         case OP_TYPE_CONST:
            {
               int32_t idx = s.readInt32B();
               NXSL_Value *v = constants.get(idx);
               if (v == nullptr)
               {
                  _sntprintf(errMsg, errMsgSize, _T("Binary file read error (instruction %04X)"), p->m_instructionSet.size());
                  delete instr;
                  goto failure;
               }
               instr->m_operand.m_constant = p->createValue(v);
            }
            break;
         case OP_TYPE_IDENTIFIER:
            instr->m_operand.m_identifier = p->createIdentifier();
            instr->m_operand.m_identifier->length = s.readByte();
            if ((instr->m_operand.m_identifier->length == 0) || (instr->m_operand.m_identifier->length > MAX_IDENTIFIER_LENGTH))
            {
               _sntprintf(errMsg, errMsgSize, _T("Binary file read error (instruction %04X)"), p->m_instructionSet.size());
               delete instr;
               goto failure;
            }
            s.read(instr->m_operand.m_identifier->value, instr->m_operand.m_identifier->length);
            break;
         case OP_TYPE_INT32:
            instr->m_operand.m_valueInt32 = s.readInt32B();
            break;
         case OP_TYPE_INT64:
            instr->m_operand.m_valueInt64 = s.readInt64B();
            break;
         case OP_TYPE_UINT32:
            instr->m_operand.m_valueUInt32 = s.readUInt32B();
            break;
         case OP_TYPE_UINT64:
            instr->m_operand.m_valueUInt64 = s.readUInt64B();
            break;
         default: 
            break;
      }
   }

   // Load module list
   s.seek(header.moduleSectionOffset);
   while(!s.eos() && (s.pos() < header.functionSectionOffset))
   {
      TCHAR *name = s.readPStringW("UTF-8");
      if (name == nullptr)
      {
         _tcslcpy(errMsg, _T("Binary file read error (modules section)"), errMsgSize);
         goto failure;
      }

      NXSL_ModuleImport *module = new NXSL_ModuleImport();
      _tcslcpy(module->name, name, MAX_IDENTIFIER_LENGTH);
      MemFree(name);
      module->lineNumber = s.readInt32B();
      module->flags = s.readByte();

      p->m_requiredModules.add(module);
   }

   // Load function list
   s.seek(header.functionSectionOffset);
   while(!s.eos())
   {
      char *name = s.readPStringA();
      if (name == nullptr)
      {
         _tcslcpy(errMsg, _T("Binary file read error (functions section)"), errMsgSize);
         goto failure;
      }
      p->m_functions.add(NXSL_Function(name, s.readUInt32B()));
      MemFree(name);
   }

   for(int i = 0; i < constants.size(); i++)
      p->destroyValue(constants.get(i));

   return p;

failure:
   for(int i = 0; i < constants.size(); i++)
      p->destroyValue(constants.get(i));

   delete p;
   return nullptr;
}
