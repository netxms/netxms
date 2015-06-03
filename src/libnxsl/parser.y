%{

#pragma warning(disable : 4065 4102)

#define YYERROR_VERBOSE
#define YYINCLUDED_STDLIB_H
#define YYDEBUG			1

#include <nms_common.h>
#include "libnxsl.h"
#include "parser.tab.hpp"

void yyerror(yyscan_t scanner, NXSL_Lexer *pLexer, NXSL_Compiler *pCompiler,
             NXSL_Program *pScript, const char *pszText);
int yylex(YYSTYPE *lvalp, yyscan_t scanner);

%}

%expect		1
%pure-parser
%lex-param		{yyscan_t scanner}
%parse-param	{yyscan_t scanner}
%parse-param	{NXSL_Lexer *pLexer}
%parse-param	{NXSL_Compiler *pCompiler}
%parse-param	{NXSL_Program *pScript}

%union
{
	INT32 valInt32;
	UINT32 valUInt32;
	INT64 valInt64;
	UINT64 valUInt64;
	char *valStr;
	double valReal;
	NXSL_Value *pConstant;
	NXSL_Instruction *pInstruction;
}

%token T_ABORT
%token T_ARRAY
%token T_BREAK
%token T_CASE
%token T_CATCH
%token T_CONST
%token T_CONTINUE
%token T_DEFAULT
%token T_DO
%token T_ELSE
%token T_EXIT
%token T_FALSE
%token T_FOR
%token T_FOREACH
%token T_GLOBAL
%token T_IF
%token T_NULL
%token T_PRINT
%token T_PRINTLN
%token T_RETURN
%token T_SUB
%token T_SWITCH
%token T_TRUE
%token T_TRY
%token T_TYPE_INT32
%token T_TYPE_INT64
%token T_TYPE_REAL
%token T_TYPE_STRING
%token T_TYPE_UINT32
%token T_TYPE_UINT64
%token T_USE
%token T_WHILE

%token <valStr> T_COMPOUND_IDENTIFIER
%token <valStr> T_IDENTIFIER
%token <valStr> T_STRING
%token <valInt32> T_INT32
%token <valUInt32> T_UINT32
%token <valInt64> T_INT64
%token <valUInt64> T_UINT64
%token <valReal> T_REAL

%right '=' T_ASSIGN_ADD T_ASSIGN_SUB T_ASSIGN_MUL T_ASSIGN_DIV T_ASSIGN_REM T_ASSIGN_CONCAT T_ASSIGN_AND T_ASSIGN_OR T_ASSIGN_XOR
%right ':'
%left '?'
%left '.'
%left T_OR
%left T_AND
%left '|'
%left '^'
%left '&'
%left T_NE T_EQ T_LIKE T_ILIKE T_MATCH T_IMATCH
%left '<' T_LE '>' T_GE
%left T_LSHIFT T_RSHIFT
%left '+' '-'
%left '*' '/' '%'
%right T_INC T_DEC '!' '~' NEGATE
%left T_REF '@'
%left T_POST_INC T_POST_DEC '[' ']'

%type <pConstant> Constant
%type <valStr> AnyIdentifier
%type <valStr> FunctionName
%type <valInt32> BuiltinType
%type <valInt32> ParameterList
%type <pInstruction> SimpleStatementKeyword

%destructor { safe_free($$); } <valStr>
%destructor { delete $$; } <pConstant>
%destructor { delete $$; } <pInstruction>

%start Script


%%

Script:
	Module
{
	char szErrorText[256];

	// Add implicit main() function
	if (!pScript->addFunction("$main", 0, szErrorText))
	{
		pCompiler->error(szErrorText);
		YYERROR;
	}
	
	// Implicit return
	pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_RET_NULL));
}
|	Expression
{
	char szErrorText[256];

	// Add implicit return
	pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_RETURN));

	// Add implicit main() function
	if (!pScript->addFunction("$main", 0, szErrorText))
	{
		pCompiler->error(szErrorText);
		YYERROR;
	}
}
|
{
	char szErrorText[256];

	// Add implicit return
	pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_RET_NULL));

	// Add implicit main() function
	if (!pScript->addFunction("$main", 0, szErrorText))
	{
		pCompiler->error(szErrorText);
		YYERROR;
	}
}
;

Module:
	ModuleComponent Module
|	ModuleComponent
;

ModuleComponent:
	ConstStatement
|	Function
|	StatementOrBlock
|	UseStatement
;

ConstStatement:
	T_CONST ConstList ';'
;

ConstList:
	ConstDefinition ',' ConstList
|	ConstDefinition
;

ConstDefinition:
	T_IDENTIFIER '=' Constant
{
	if (!pScript->addConstant($1, $3))
	{
		pCompiler->error("Constant already defined");
		delete_and_null($3);
		YYERROR;
	}
	safe_free_and_null($1);
	$3 = NULL;
}
;

UseStatement:
	T_USE AnyIdentifier ';'
{
	pScript->addRequiredModule($2);
	$2 = NULL;
}
;

AnyIdentifier:
	T_IDENTIFIER
|	T_COMPOUND_IDENTIFIER
;

Function:
	T_SUB FunctionName 
	{
		char szErrorText[256];

		pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_JMP, INVALID_ADDRESS));
		
		if (!pScript->addFunction($2, INVALID_ADDRESS, szErrorText))
		{
			pCompiler->error(szErrorText);
			YYERROR;
		}
		safe_free_and_null($2);
		pCompiler->setIdentifierOperation(OPCODE_BIND);
	}
	ParameterDeclaration Block
	{
		pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_RET_NULL));
		pScript->resolveLastJump(OPCODE_JMP);
	}
;

ParameterDeclaration:
	IdentifierList ')'
|	')'
;

IdentifierList:
	T_IDENTIFIER 
	{
		pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), pCompiler->getIdentifierOperation(), $1));
		$1 = NULL;
	}
	',' IdentifierList
|	T_IDENTIFIER
	{
		pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), pCompiler->getIdentifierOperation(), $1));
		$1 = NULL;
	}
;

Block:
	'{' StatementList '}'
;

StatementList:
	StatementOrBlock StatementList
|
;

StatementOrBlock:
	Statement
|	Block
|	TryCatchBlock
;

TryCatchBlock:
	T_TRY 
	{
		pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_CATCH, INVALID_ADDRESS));
	} 
	Block T_CATCH 
	{
		pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_JMP, INVALID_ADDRESS));
		pScript->resolveLastJump(OPCODE_CATCH);
	} 
	Block
	{
		pScript->resolveLastJump(OPCODE_JMP);
	}
;

Statement:
	Expression ';'
{
	pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_POP, (short)1));
}
|	BuiltinStatement
|	';'
{
	pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_NOP));
}
;

Expression:
	'(' Expression ')'
|	T_IDENTIFIER '=' Expression
{
	pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_SET, $1));
	$1 = NULL;
}
|	T_IDENTIFIER T_ASSIGN_ADD { pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_PUSH_VARIABLE, strdup($1))); } Expression
{
	pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_ADD));
	pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_SET, $1));
	$1 = NULL;
}
|	T_IDENTIFIER T_ASSIGN_SUB { pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_PUSH_VARIABLE, strdup($1))); } Expression
{
	pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_SUB));
	pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_SET, $1));
	$1 = NULL;
}
|	T_IDENTIFIER T_ASSIGN_MUL { pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_PUSH_VARIABLE, strdup($1))); } Expression
{
	pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_MUL));
	pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_SET, $1));
	$1 = NULL;
}
|	T_IDENTIFIER T_ASSIGN_DIV { pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_PUSH_VARIABLE, strdup($1))); } Expression
{
	pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_DIV));
	pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_SET, $1));
	$1 = NULL;
}
|	T_IDENTIFIER T_ASSIGN_REM { pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_PUSH_VARIABLE, strdup($1))); } Expression
{
	pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_REM));
	pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_SET, $1));
	$1 = NULL;
}
|	T_IDENTIFIER T_ASSIGN_CONCAT { pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_PUSH_VARIABLE, strdup($1))); } Expression
{
	pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_CONCAT));
	pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_SET, $1));
	$1 = NULL;
}
|	T_IDENTIFIER T_ASSIGN_AND { pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_PUSH_VARIABLE, strdup($1))); } Expression
{
	pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_BIT_AND));
	pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_SET, $1));
	$1 = NULL;
}
|	T_IDENTIFIER T_ASSIGN_OR { pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_PUSH_VARIABLE, strdup($1))); } Expression
{
	pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_BIT_OR));
	pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_SET, $1));
	$1 = NULL;
}
|	T_IDENTIFIER T_ASSIGN_XOR { pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_PUSH_VARIABLE, strdup($1))); } Expression
{
	pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_BIT_XOR));
	pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_SET, $1));
	$1 = NULL;
}
|	Expression '[' Expression ']' '=' Expression
{
	pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_SET_ELEMENT));
}
|	Expression '[' Expression ']'
{
	pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_GET_ELEMENT));
}
|	Expression T_REF T_IDENTIFIER '=' Expression
{
	pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_SET_ATTRIBUTE, $3));
	$3 = NULL;
}
|	Expression T_REF T_IDENTIFIER
{
	pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_GET_ATTRIBUTE, $3));
	$3 = NULL;
}
|	Expression T_REF FunctionName ParameterList ')'
{
	pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_CALL_METHOD, $3, $4));
	$3 = NULL;
}
|	Expression T_REF FunctionName ')'
{
	pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_CALL_METHOD, $3, 0));
	$3 = NULL;
}
|	T_IDENTIFIER '@' Expression
{
	pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_SAFE_GET_ATTR, $1));
	$1 = NULL;
}
|	'-' Expression		%prec NEGATE
{
	pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_NEG));
}
|	'!' Expression
{
	pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_NOT));
}
|	'~' Expression
{
	pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_BIT_NOT));
}
|	T_INC T_IDENTIFIER
{
	pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_INCP, $2));
	$2 = NULL;
}
|	T_DEC T_IDENTIFIER
{
	pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_DECP, $2));
	$2 = NULL;
}
|	T_IDENTIFIER T_INC	%prec T_POST_INC
{
	pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_INC, $1));
	$1 = NULL;
}
|	T_IDENTIFIER T_DEC	%prec T_POST_DEC
{
	pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_DEC, $1));
	$1 = NULL;
}
/*
|	T_INC Expression '[' Expression ']'
{
	pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_INCP_ELEMENT));
}
|	T_DEC Expression '[' Expression ']'
{
	pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_DECP_ELEMENT));
}
*/
|	Expression '[' Expression ']' T_INC	%prec T_POST_INC
{
	pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_INC_ELEMENT));
}
|	Expression '[' Expression ']' T_DEC	%prec T_POST_DEC
{
	pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_DEC_ELEMENT));
}
|	Expression '+' Expression	
{ 
	pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_ADD));
}
|	Expression '-' Expression
{
	pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_SUB));
}
|	Expression '*' Expression
{
	pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_MUL));
}
|	Expression '/' Expression
{
	pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_DIV));
}
|	Expression '%' Expression
{
	pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_REM));
}
|	Expression T_LIKE Expression
{
	pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_LIKE));
}
|	Expression T_ILIKE Expression
{
	pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_ILIKE));
}
|	Expression T_MATCH Expression
{
	pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_MATCH));
}
|	Expression T_IMATCH Expression
{
	pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_IMATCH));
}
|	Expression T_EQ Expression
{
	pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_EQ));
}
|	Expression T_NE Expression
{
	pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_NE));
}
|	Expression '<' Expression
{
	pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_LT));
}
|	Expression T_LE Expression
{
	pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_LE));
}
|	Expression '>' Expression
{
	pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_GT));
}
|	Expression T_GE Expression
{
	pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_GE));
}
|	Expression '&' Expression
{
	pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_BIT_AND));
}
|	Expression '|' Expression
{
	pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_BIT_OR));
}
|	Expression '^' Expression
{
	pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_BIT_XOR));
}
|	Expression T_AND { pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_JZ_PEEK, INVALID_ADDRESS)); } Expression
{
	pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_AND));
	pScript->resolveLastJump(OPCODE_JZ_PEEK);
}
|	Expression T_OR { pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_JNZ_PEEK, INVALID_ADDRESS)); } Expression
{
	pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_OR));
	pScript->resolveLastJump(OPCODE_JNZ_PEEK);
}
|	Expression T_LSHIFT Expression
{
	pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_LSHIFT));
}
|	Expression T_RSHIFT Expression
{
	pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_RSHIFT));
}
|	Expression '.' Expression
{
	pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_CONCAT));
}
|	Expression '?'
{
	pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_JZ, INVALID_ADDRESS));
}
	Expression ':'
{
	pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_JMP, INVALID_ADDRESS));
	pScript->resolveLastJump(OPCODE_JZ);
}	 
	Expression
{
	pScript->resolveLastJump(OPCODE_JMP);
}
|	Operand
;

Operand:
	FunctionCall
|	TypeCast
|	ArrayInitializer
|	T_IDENTIFIER
{
	pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_PUSH_VARIABLE, $1));
	$1 = NULL;
}
|	Constant
{
	pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_PUSH_CONSTANT, $1));
	$1 = NULL;
}
;

TypeCast:
	BuiltinType '(' Expression ')'
{
	pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_CAST, (short)$1));
}
;

ArrayInitializer:
	'%' '(' 
{
	pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_PUSH_CONSTANT, new NXSL_Value(new NXSL_Array)));
}
	ArrayElements ')'
|	'%' '(' ')'
{
	pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_PUSH_CONSTANT, new NXSL_Value(new NXSL_Array)));
}
;

ArrayElements:
	Expression 
{
	pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_ADD_TO_ARRAY));
}
	',' ArrayElements
|	Expression
{
	pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_ADD_TO_ARRAY));
}
;

BuiltinType:
	T_TYPE_INT32
{
	$$ = NXSL_DT_INT32;
}
|	T_TYPE_INT64
{
	$$ = NXSL_DT_INT64;
}
|	T_TYPE_UINT32
{
	$$ = NXSL_DT_UINT32;
}
|	T_TYPE_UINT64
{
	$$ = NXSL_DT_UINT64;
}
|	T_TYPE_REAL
{
	$$ = NXSL_DT_REAL;
}
|	T_TYPE_STRING
{
	$$ = NXSL_DT_STRING;
}
;

BuiltinStatement:
	SimpleStatement ';'
|	PrintlnStatement
|	IfStatement
|	DoStatement
|	WhileStatement
|	ForStatement
|	ForEachStatement
|	SwitchStatement
|	ArrayStatement
|	GlobalStatement
|	T_BREAK ';'
{
	if (pCompiler->canUseBreak())
	{
		pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_NOP));
		pCompiler->addBreakAddr(pScript->getCodeSize() - 1);
	}
	else
	{
		pCompiler->error("\"break\" statement can be used only within loops and \"switch\" statements");
		YYERROR;
	}
}
|	T_CONTINUE ';'
{
	UINT32 dwAddr = pCompiler->peekAddr();
	if (dwAddr != INVALID_ADDRESS)
	{
		pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_JMP, dwAddr));
	}
	else
	{
		pCompiler->error("\"continue\" statement can be used only within loops");
		YYERROR;
	}
}
;

SimpleStatement:
	SimpleStatementKeyword Expression
{
	pScript->addInstruction($1);
	$1 = NULL;
}
|	SimpleStatementKeyword
{
	pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_PUSH_CONSTANT, new NXSL_Value));
	pScript->addInstruction($1);
	$1 = NULL;
}
;

SimpleStatementKeyword:
	T_ABORT
{
	$$ = new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_ABORT);
}
|	T_EXIT
{
	$$ = new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_EXIT);
}
|	T_RETURN
{
	$$ = new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_RETURN);
}
|	T_PRINT
{
	$$ = new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_PRINT);
}
;

PrintlnStatement:
	T_PRINTLN Expression ';'
{
	pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_PUSH_CONSTANT, new NXSL_Value(_T("\n"))));
	pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_CONCAT));
	pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_PRINT));
}
|	T_PRINTLN ';'
{
	pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_PUSH_CONSTANT, new NXSL_Value(_T("\n"))));
	pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_PRINT));
}
;

IfStatement:
	T_IF '(' Expression ')' 
	{
		pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_JZ, INVALID_ADDRESS));
	} 
	IfBody
;

IfBody:
	StatementOrBlock
{
	pScript->resolveLastJump(OPCODE_JZ);
}
|	StatementOrBlock ElseStatement
{
	pScript->resolveLastJump(OPCODE_JMP);
}
;

ElseStatement:
	T_ELSE
	{
		pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_JMP, INVALID_ADDRESS));
		pScript->resolveLastJump(OPCODE_JZ);
	}
	StatementOrBlock
;

ForStatement:
	T_FOR '(' Expression ';'
{
	pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_POP, (short)1));
	pCompiler->pushAddr(pScript->getCodeSize());
}
	Expression ';'
{
	pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_JZ, INVALID_ADDRESS));
	pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_JMP, INVALID_ADDRESS));
	pCompiler->pushAddr(pScript->getCodeSize());
}
	Expression ')'
{
	pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_POP, (short)1));
	UINT32 addrPart3 = pCompiler->popAddr();
	pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_JMP, pCompiler->popAddr()));
	pCompiler->pushAddr(addrPart3);
	pCompiler->newBreakLevel();
	pScript->resolveLastJump(OPCODE_JMP);
}	
	StatementOrBlock
{
	pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_JMP, pCompiler->popAddr()));
	pScript->resolveLastJump(OPCODE_JZ);
	pCompiler->closeBreakLevel(pScript);
}
;

ForEachStatement:
	T_FOREACH '(' T_IDENTIFIER 
{
	pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_PUSH_CONSTANT, new NXSL_Value($3)));
	safe_free_and_null($3);
}
	':' Expression ')'
{
	pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_FOREACH));
	pCompiler->pushAddr(pScript->getCodeSize());
	pCompiler->newBreakLevel();
	pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_NEXT));
	pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_JZ, INVALID_ADDRESS));
}
	StatementOrBlock
{
	pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_JMP, pCompiler->popAddr()));
	pScript->resolveLastJump(OPCODE_JZ);
	pCompiler->closeBreakLevel(pScript);
	pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_POP, (short)1));
}
;

WhileStatement:
	T_WHILE
{
	pCompiler->pushAddr(pScript->getCodeSize());
	pCompiler->newBreakLevel();
}
	'(' Expression ')'
{
	pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_JZ, INVALID_ADDRESS));
}
	StatementOrBlock
{
	pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_JMP, pCompiler->popAddr()));
	pScript->resolveLastJump(OPCODE_JZ);
	pCompiler->closeBreakLevel(pScript);
}
;

DoStatement:
	T_DO
{
	pCompiler->pushAddr(pScript->getCodeSize());
	pCompiler->newBreakLevel();
}
	StatementOrBlock T_WHILE '(' Expression ')' ';'
{
	pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(),
				OPCODE_JNZ, pCompiler->popAddr()));
	pCompiler->closeBreakLevel(pScript);
}
;

SwitchStatement:
	T_SWITCH
{ 
	pCompiler->newBreakLevel();
}
	'(' Expression ')' '{' CaseList Default '}'
{
	pCompiler->closeBreakLevel(pScript);
	pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_POP, (short)1));
}
;

CaseList:
	Case
{ 
	pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_JMP, pScript->getCodeSize() + 3));
	pScript->resolveLastJump(OPCODE_JZ);
}
	CaseList
|	Case
{
	pScript->resolveLastJump(OPCODE_JZ);
}
;

Case:
	T_CASE Constant
{
	pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_CASE, $2));
	pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_JZ, INVALID_ADDRESS));
	$2 = NULL;
} 
	':' StatementList
|	T_CASE T_IDENTIFIER
{
	pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_CASE_CONST, $2));
	pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_JZ, INVALID_ADDRESS));
	$2 = NULL;
}
	':' StatementList
;

Default:
	T_DEFAULT ':' StatementList
|
;

ArrayStatement:
	T_ARRAY { pCompiler->setIdentifierOperation(OPCODE_ARRAY); } IdentifierList ';'
|	T_GLOBAL T_ARRAY { pCompiler->setIdentifierOperation(OPCODE_GLOBAL_ARRAY); } IdentifierList ';'
;

GlobalStatement:
	T_GLOBAL GlobalVariableList ';'
;

GlobalVariableList:
	GlobalVariableDeclaration ',' GlobalVariableList
|	GlobalVariableDeclaration
;

GlobalVariableDeclaration:
	T_IDENTIFIER
{
	pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_GLOBAL, $1, 0));
	$1 = NULL;
}
|	T_IDENTIFIER '=' Expression
{
	pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_GLOBAL, $1, 1));
	$1 = NULL;
}
;

FunctionCall:
	FunctionName ParameterList ')'
{
	pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_CALL_EXTERNAL, $1, $2));
	$1 = NULL;
}
|	FunctionName ')'
{
	pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_CALL_EXTERNAL, $1, 0));
	$1 = NULL;
}
;

ParameterList:
	Parameter ',' ParameterList { $$ = $3 + 1; }
|	Parameter { $$ = 1; }
;

Parameter:
	Expression
|	T_IDENTIFIER ':' Expression
{
	pScript->addInstruction(new NXSL_Instruction(pLexer->getCurrLine(), OPCODE_NAME, $1));
	$1 = NULL;
}
;

FunctionName:
	T_IDENTIFIER '('
{
	$$ = $1;
}
;

Constant:
	T_STRING
{
	$$ = new NXSL_Value($1);
	safe_free_and_null($1);
}
|	T_INT32
{
	$$ = new NXSL_Value($1);
}
|	T_UINT32
{
	$$ = new NXSL_Value($1);
}
|	T_INT64
{
	$$ = new NXSL_Value($1);
}
|	T_UINT64
{
	$$ = new NXSL_Value($1);
}
|	T_REAL
{
	$$ = new NXSL_Value($1);
}
|	T_TRUE
{
	$$ = new NXSL_Value((LONG)1);
}
|	T_FALSE
{
	$$ = new NXSL_Value((LONG)0);
}
|	T_NULL
{
	$$ = new NXSL_Value;
}
;
