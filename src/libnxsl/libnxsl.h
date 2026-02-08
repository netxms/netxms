/* 
** NetXMS - Network Management System
** NetXMS Scripting Language Interpreter
** Copyright (C) 2003-2022 Victor Kirhenshtein
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
** File: libnxsl.h
**
**/

#ifndef _libnxsl_h_
#define _libnxsl_h_

#include <nms_common.h>
#include <nms_util.h>
#include <nxcpapi.h>
#include <nxsl.h>
#include <nxqueue.h>

union YYSTYPE;
typedef void *yyscan_t;

//
// Various defines
//

#define MAX_STRING_SIZE    8192

/**
 * Instruction opcodes
 */
#define OPCODE_NOP            0
#define OPCODE_RETURN         1
#define OPCODE_JMP            2
#define OPCODE_CALL           3
#define OPCODE_CALL_EXTERNAL  4
#define OPCODE_PUSH_CONSTANT  5
#define OPCODE_PUSH_VARIABLE  6
#define OPCODE_EXIT           7
#define OPCODE_POP            8
#define OPCODE_SET            9
#define OPCODE_ADD            10
#define OPCODE_SUB            11
#define OPCODE_MUL            12
#define OPCODE_DIV            13
#define OPCODE_REM            14
#define OPCODE_EQ             15
#define OPCODE_NE             16
#define OPCODE_LT             17
#define OPCODE_LE             18
#define OPCODE_GT             19
#define OPCODE_GE             20
#define OPCODE_BIT_AND        21
#define OPCODE_BIT_OR         22
#define OPCODE_BIT_XOR        23
#define OPCODE_AND            24
#define OPCODE_OR             25
#define OPCODE_LSHIFT         26
#define OPCODE_RSHIFT         27
#define OPCODE_RET_NULL       28
#define OPCODE_JZ             29
#define OPCODE_IDIV           30
#define OPCODE_CONCAT         31
#define OPCODE_BIND           32
#define OPCODE_INC            33
#define OPCODE_DEC            34
#define OPCODE_NEG            35
#define OPCODE_NOT            36
#define OPCODE_BIT_NOT        37
#define OPCODE_CAST           38
#define OPCODE_GET_ATTRIBUTE  39
#define OPCODE_INCP           40
#define OPCODE_DECP           41
#define OPCODE_JNZ            42
#define OPCODE_LIKE           43
#define OPCODE_ILIKE          44
#define OPCODE_MATCH          45
#define OPCODE_IMATCH         46
#define OPCODE_CASE           47
#define OPCODE_ARRAY          48
#define OPCODE_GET_ELEMENT    49
#define OPCODE_SET_ELEMENT    50
#define OPCODE_SET_ATTRIBUTE  51
#define OPCODE_NAME           52
#define OPCODE_FOREACH        53
#define OPCODE_NEXT           54
#define OPCODE_GLOBAL         55
#define OPCODE_GLOBAL_ARRAY   56
#define OPCODE_JZ_PEEK        57
#define OPCODE_JNZ_PEEK       58
#define OPCODE_APPEND         59
#define OPCODE_SAFE_GET_ATTR  60
#define OPCODE_CALL_METHOD    61
#define OPCODE_CASE_CONST     62
#define OPCODE_INC_ELEMENT    63
#define OPCODE_DEC_ELEMENT    64
#define OPCODE_INCP_ELEMENT   65
#define OPCODE_DECP_ELEMENT   66
#define OPCODE_ABORT          67
#define OPCODE_CATCH          68
#define OPCODE_PUSH_CONSTREF  69
#define OPCODE_HASHMAP_SET    70
#define OPCODE_NEW_ARRAY      71
#define OPCODE_NEW_HASHMAP    72
#define OPCODE_CPOP           73
#define OPCODE_STORAGE_READ   74
#define OPCODE_STORAGE_WRITE  75
#define OPCODE_SELECT         76
#define OPCODE_PUSHCP         77
#define OPCODE_STORAGE_INC    78
#define OPCODE_STORAGE_INCP   79
#define OPCODE_STORAGE_DEC    80
#define OPCODE_STORAGE_DECP   81
#define OPCODE_PEEK_ELEMENT   82
#define OPCODE_PUSH_VARPTR    83
#define OPCODE_SET_VARPTR     84
#define OPCODE_CALL_EXTPTR    85
#define OPCODE_INC_VARPTR     86
#define OPCODE_DEC_VARPTR     87
#define OPCODE_INCP_VARPTR    88
#define OPCODE_DECP_VARPTR    89
#define OPCODE_IN             90
#define OPCODE_PUSH_EXPRVAR   91
#define OPCODE_SET_EXPRVAR    92
#define OPCODE_UPDATE_EXPRVAR 93
#define OPCODE_CLEAR_EXPRVARS 94
#define OPCODE_GET_RANGE      95
#define OPCODE_CASE_LT        96
#define OPCODE_CASE_CONST_LT  97
#define OPCODE_CASE_GT        98
#define OPCODE_CASE_CONST_GT  99
#define OPCODE_PUSH_PROPERTY  100
#define OPCODE_PUSH_INT32     101
#define OPCODE_PUSH_UINT32    102
#define OPCODE_PUSH_INT64     103
#define OPCODE_PUSH_UINT64    104
#define OPCODE_PUSH_TRUE      105
#define OPCODE_PUSH_FALSE     106
#define OPCODE_PUSH_NULL      107
#define OPCODE_SPREAD         108
#define OPCODE_ARGV           109
#define OPCODE_APPEND_ALL     110
#define OPCODE_FSTRING        111
#define OPCODE_CALL_INDIRECT  112
#define OPCODE_SAFE_CALL      113
#define OPCODE_HAS_BITS       114
#define OPCODE_LOCAL          115

class NXSL_Compiler;

/**
 * Data type for import symbols
 */
struct import_symbol_t
{
   identifier_t name;
   identifier_t alias;
   int32_t lineNumber;
};

/**
 * Statement data used in parser
 */
struct SimpleStatementData
{
   int sourceLine;
   int16_t opCode;
};

/**
 * Instruction's operand type
 */
enum OperandType
{
   OP_TYPE_NONE = 0,
   OP_TYPE_ADDR = 1,
   OP_TYPE_IDENTIFIER = 2,
   OP_TYPE_CONST = 3,
   OP_TYPE_VARIABLE = 4,
   OP_TYPE_EXT_FUNCTION = 5,
   OP_TYPE_INT32 = 6,
   OP_TYPE_UINT32 = 7,
   OP_TYPE_INT64 = 8,
   OP_TYPE_UINT64 = 9
};

/**
 * Single execution instruction
 */
struct NXSL_Instruction
{
   int16_t m_opCode;
   int16_t m_stackItems;
   uint32_t m_addr2;   // Second address
   union
   {
      NXSL_Value *m_constant;
      NXSL_Identifier *m_identifier;
      NXSL_Variable *m_variable;
      const NXSL_ExtFunction *m_function;
      uint32_t m_addr;
      int32_t m_valueInt32;
      uint32_t m_valueUInt32;
      int64_t m_valueInt64;
      uint64_t m_valueUInt64;
   } m_operand;
   int32_t m_sourceLine;

   OperandType getOperandType() const;
   void copyFrom(const NXSL_Instruction *src, NXSL_ValueManager *vm);
   void dispose(NXSL_ValueManager *vm);
   void restoreVariableReference(NXSL_Identifier *identifier);
};

/**
 * NXSL program builder
 */
class NXSL_ProgramBuilder : public NXSL_ValueManager
{
   friend class NXSL_Program;

protected:
   StructArray<NXSL_Instruction> m_instructionSet;
   StructArray<NXSL_ModuleImport> m_requiredModules;
   NXSL_ValueHashMap<NXSL_Identifier> m_constants;
   StructArray<NXSL_FunctionImport> m_importedFunctions;
   StructArray<NXSL_Function> m_functions;
   StructArray<NXSL_IdentifierLocation> *m_expressionVariables;
   NXSL_Identifier m_currentMetadataPrefix;
   StringMap m_metadata;
   NXSL_Environment *m_environment;
   uint32_t m_numFStringElements;
   StructArray<import_symbol_t> m_importSymbols;

   uint32_t getFinalJumpDestination(uint32_t addr, int srcJump);
   uint32_t getExpressionVariableCodeBlock(const NXSL_Identifier& identifier);

   NXSL_Instruction *addInstructionPlaceholder(int line, int16_t opCode)
   {
      NXSL_Instruction *i = m_instructionSet.addPlaceholder();
      memset(i, 0, sizeof(NXSL_Instruction));
      i->m_sourceLine = line;
      i->m_opCode = opCode;
      i->m_addr2 = INVALID_ADDRESS;
      return i;
   }

public:
   NXSL_ProgramBuilder(NXSL_Environment *env);
   virtual ~NXSL_ProgramBuilder();

   void addInstruction(int line, int16_t opCode)
   {
      addInstructionPlaceholder(line, opCode);
   }
   void addInstruction(int line, int16_t opCode, NXSL_Value *value)
   {
      addInstructionPlaceholder(line, opCode)->m_operand.m_constant = value;
   }
   void addInstruction(int line, int16_t opCode, const NXSL_Identifier& identifier)
   {
      addInstructionPlaceholder(line, opCode)->m_operand.m_identifier = createIdentifier(identifier);
   }
   void addInstruction(int line, int16_t opCode, const NXSL_Identifier& identifier, int16_t stackItems, uint32_t addr2 = INVALID_ADDRESS)
   {
      NXSL_Instruction *i = addInstructionPlaceholder(line, opCode);
      i->m_operand.m_identifier = createIdentifier(identifier);
      i->m_stackItems = stackItems;
      i->m_addr2 = addr2;
   }
   void addInstruction(int line, int16_t opCode, uint32_t addr)
   {
      addInstructionPlaceholder(line, opCode)->m_operand.m_addr = addr;
   }
   void addInstruction(int line, int16_t opCode, int16_t stackItems)
   {
      addInstructionPlaceholder(line, opCode)->m_stackItems = stackItems;
   }

   bool addFunction(const NXSL_Identifier& name, uint32_t addr, char *errorText);
   void resolveFunctions();
   void addPushVariableInstruction(const NXSL_Identifier& name, int line);
   void resolveLastJump(int opcode, int offset = 0);
   void createJumpAt(uint32_t opAddr, uint32_t jumpAddr);
   void addRequiredModule(const char *name, int lineNumber, bool removeLastElement, bool fullImport, bool optional);
   void optimize();
   void removeInstructions(uint32_t start, int count);
   bool addConstant(const NXSL_Identifier& name, NXSL_Value *value);
   NXSL_Value *getConstantValue(const NXSL_Identifier& name);
   void enableExpressionVariables();
   void disableExpressionVariables(int line);
   void registerExpressionVariable(const NXSL_Identifier& identifier);
   void setMetadata(const TCHAR *key, const TCHAR *value) { m_metadata.set(key, value); }
   void setCurrentMetadataPrefix(const NXSL_Identifier& prefix) { m_currentMetadataPrefix = prefix; }

   uint32_t getCodeSize() const { return m_instructionSet.size(); }
   bool isEmpty() const { return m_instructionSet.isEmpty() || ((m_instructionSet.size() == 1) && (m_instructionSet.get(0)->m_opCode == 28)); }
   StringList *getRequiredModules() const;
   const NXSL_Identifier& getCurrentMetadataPrefix() const { return m_currentMetadataPrefix; }
   NXSL_Environment *getEnvironment() { return m_environment; }

   void startFString() { m_numFStringElements = 0; }
   void incFStringElements() { m_numFStringElements++; }
   uint32_t getFStringElementCount() { return m_numFStringElements; }

   void addImportSymbol(const import_symbol_t& s) { m_importSymbols.add(s); }
   void finalizeSymbolImport(const char *module);

   virtual uint64_t getMemoryUsage() const override;

   void dump(FILE *fp) const { dump(fp, m_instructionSet); }
   static void dump(FILE *fp, const StructArray<NXSL_Instruction>& instructionSet);
   static void dump(FILE *fp, uint32_t addr, const NXSL_Instruction& instruction);
};

/**
 * Modified lexer class
 */
class NXSL_Lexer
{
	friend int yylex(YYSTYPE *, yyscan_t);

protected:
   size_t m_sourceSize;
   size_t m_sourcePos;
   char *m_sourceCode;
   NXSL_Compiler *m_compiler;
   bool m_converterMode;

   int m_currLine;
   int m_commentLevel;
   size_t m_stringSize;
   char m_string[MAX_STRING_SIZE];
   ByteStream m_text;

public:
	NXSL_Lexer(NXSL_Compiler *compiler, const TCHAR *sourceCode);
	~NXSL_Lexer();

	size_t lexerInput(char *buffer, size_t maxSize);

	void setConverterMode(bool mode) { m_converterMode = mode; }
	bool isConverterMode() const { return m_converterMode; }
	const char *getSourceArPos(int pos) const { return m_sourceCode + pos; }
   const char *getSource() const { return m_sourceCode; }

	int getCurrLine() const { return m_currLine; }
	size_t getSourcePos() const { return m_sourcePos; }

	void error(const char *message);
};

/**
 * Compiler class
 */
class NXSL_Compiler
{
protected:
   StringBuffer m_errorText;
   int m_errorLineNumber;
   ObjectArray<NXSL_CompilationWarning> *m_warnings;
   NXSL_Lexer *m_lexer;
   NXSL_Stack *m_addrStack;
	NXSL_Stack *m_breakStack;
	NXSL_Stack *m_selectStack;
	int m_idOpCode;
	int m_temporaryStackItems;

public:
   NXSL_Compiler();
   ~NXSL_Compiler();

   NXSL_Program *compile(const TCHAR *sourceCode, NXSL_Environment *env, ObjectArray<NXSL_CompilationWarning> *warnings);
   void error(const char *message);
   void warning(const TCHAR *format, ...);

   StringBuffer convertToV5(const TCHAR *sourceCode);

   const TCHAR *getErrorText() const { return m_errorText.cstr(); }
   int getErrorLineNumber() const { return m_errorLineNumber; }

   void pushAddr(uint32_t addr) { m_addrStack->push(CAST_TO_POINTER(addr, void *)); }
   uint32_t popAddr();
   uint32_t peekAddr();

	void addBreakAddr(uint32_t addr);
	void closeBreakLevel(NXSL_ProgramBuilder *pScript);
	bool canUseBreak() { return m_breakStack->getPosition() > 0; }
	void newBreakLevel() { m_breakStack->push(new Queue); }

	void newSelectLevel() { m_selectStack->push(new Queue); }
   void closeSelectLevel();
   void pushSelectJumpAddr(uint32_t addr);
   uint32_t popSelectJumpAddr();

   void incTemporaryStackItems() { m_temporaryStackItems++; }
   void decTemporaryStackItems() { m_temporaryStackItems--; }
   int getTemporaryStackItems() const { return m_temporaryStackItems; }

	void setIdentifierOperation(int opcode) { m_idOpCode = opcode; }
	int getIdentifierOperation() { return m_idOpCode; }
};

/**
 * Class registry
 */
struct NXSL_ClassRegistry
{
   size_t size;
   size_t allocated;
   NXSL_Class **classes;
};

/**
 * Additional tokens for converter
 */
enum ConverterTokens
{
   T_ARROW_REF = 30001,
   T_SUB = 30002,
   T_USE = 30003,
   T_V4_ASSIGN_CONCAT = 30004
};

//
// Global variables
//

extern const TCHAR *g_szTypeNames[];


#endif
