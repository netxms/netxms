%{

#pragma warning(disable : 4065 4102)

#define YYERROR_VERBOSE
#define YYDEBUG			1

#include <nms_common.h>
#include "libnxsl.h"
#include "parser.tab.hpp"

void yyerror(NXSL_Lexer *pLexer, NXSL_Compiler *pCompiler,
             NXSL_Program *pScript, char *pszText);
int yylex(YYSTYPE *lvalp, NXSL_Lexer *pLexer);

%}

%expect		1
%pure-parser
%lex-param	{NXSL_Lexer *pLexer}
%parse-param	{NXSL_Lexer *pLexer}
%parse-param	{NXSL_Compiler *pCompiler}
%parse-param	{NXSL_Program *pScript}

%union
{
	int valInt;
	char *valStr;
	double valReal;
	NXSL_Value *pConstant;
	NXSL_Instruction *pInstruction;
}

%token T_ELSE
%token T_EXIT
%token T_IF
%token T_NULL
%token T_PRINT
%token T_RETURN
%token T_SUB

%token <valStr> T_IDENTIFIER
%token <valInt> T_INTEGER
%token <valStr> T_STRING
%token <valReal> T_REAL

%right '='
%left '.'
%left T_OR
%left T_AND
%left '|'
%left '^'
%left '&'
%left T_NE T_EQ
%left '<' T_LE '>' T_GE
%left T_LSHIFT T_RSHIFT
%left '+' '-'
%left '*' '/' '%'
%right T_INC T_DEC '!' '~' NEGATE
%left T_POST_INC T_POST_DEC

%type <pConstant> Constant
%type <valStr> FunctionName
%type <valInt> ParameterList
%type <pInstruction> SimpleStatementKeyword

%start Script


%%

Script:
	Module
|	Expression
{
	char szErrorText[256];

	// Add implicit return
	pScript->AddInstruction(new NXSL_Instruction(pLexer->GetCurrLine(), OPCODE_RETURN));

	// Add implicit main() function
	if (!pScript->AddFunction("main", 0, szErrorText))
	{
		pCompiler->Error(szErrorText);
		pLexer->SetErrorState();
	}
}
;

Module:
	ModuleComponent Module
|	ModuleComponent
;

ModuleComponent:
	Function
;

Function:
	T_SUB FunctionName 
	{
		char szErrorText[256];

		if (!pScript->AddFunction($2, INVALID_ADDRESS, szErrorText))
		{
			pCompiler->Error(szErrorText);
			pLexer->SetErrorState();
		}
		free($2);
	}
	ParameterDeclaration Block
{
	pScript->AddInstruction(new NXSL_Instruction(pLexer->GetCurrLine(), OPCODE_RET_NULL));
}
;

ParameterDeclaration:
	IdentifierList ')'
|	')'
;

IdentifierList:
	T_IDENTIFIER 
	{
		pScript->AddInstruction(new NXSL_Instruction(pLexer->GetCurrLine(), OPCODE_BIND, $1));
	}
	',' IdentifierList
|	T_IDENTIFIER
{
	pScript->AddInstruction(new NXSL_Instruction(pLexer->GetCurrLine(), OPCODE_BIND, $1));
}
;

Block:
	'{' StatementList '}'
|	'{' '}'
{
	pScript->AddInstruction(new NXSL_Instruction(pLexer->GetCurrLine(), OPCODE_NOP));
}
;

StatementList:
	StatementOrBlock StatementList
|	StatementOrBlock
;	

StatementOrBlock:
	Statement
|	Block
;

Statement:
	Expression ';'
{
	pScript->AddInstruction(new NXSL_Instruction(pLexer->GetCurrLine(), OPCODE_POP, (int)1));
}
|	BuiltinStatement
|	';'
{
	pScript->AddInstruction(new NXSL_Instruction(pLexer->GetCurrLine(), OPCODE_NOP));
}
;

Expression:
	'(' Expression ')'
|	T_IDENTIFIER '=' Expression
{
	pScript->AddInstruction(new NXSL_Instruction(pLexer->GetCurrLine(), OPCODE_SET, $1));
}
|	'-' Expression		%prec NEGATE
{
	pScript->AddInstruction(new NXSL_Instruction(pLexer->GetCurrLine(), OPCODE_NEG));
}
|	'!' Expression
{
	pScript->AddInstruction(new NXSL_Instruction(pLexer->GetCurrLine(), OPCODE_NOT));
}
|	'~' Expression
{
	pScript->AddInstruction(new NXSL_Instruction(pLexer->GetCurrLine(), OPCODE_BIT_NOT));
}
|	T_INC T_IDENTIFIER
{
	pScript->AddInstruction(new NXSL_Instruction(pLexer->GetCurrLine(), OPCODE_INC, $2));
}
|	T_DEC T_IDENTIFIER
{
	pScript->AddInstruction(new NXSL_Instruction(pLexer->GetCurrLine(), OPCODE_DEC, $2));
}
|	T_IDENTIFIER T_INC	%prec T_POST_INC
{
	pScript->AddInstruction(new NXSL_Instruction(pLexer->GetCurrLine(), OPCODE_INC, $1));
}
|	T_IDENTIFIER T_DEC	%prec T_POST_DEC
{
	pScript->AddInstruction(new NXSL_Instruction(pLexer->GetCurrLine(), OPCODE_DEC, $1));
}
|	Expression '+' Expression	
{ 
	pScript->AddInstruction(new NXSL_Instruction(pLexer->GetCurrLine(), OPCODE_ADD));
}
|	Expression '-' Expression
{
	pScript->AddInstruction(new NXSL_Instruction(pLexer->GetCurrLine(), OPCODE_SUB));
}
|	Expression '*' Expression
{
	pScript->AddInstruction(new NXSL_Instruction(pLexer->GetCurrLine(), OPCODE_MUL));
}
|	Expression '/' Expression
{
	pScript->AddInstruction(new NXSL_Instruction(pLexer->GetCurrLine(), OPCODE_DIV));
}
|	Expression '%' Expression
{
	pScript->AddInstruction(new NXSL_Instruction(pLexer->GetCurrLine(), OPCODE_REM));
}
|	Expression T_EQ Expression
{
	pScript->AddInstruction(new NXSL_Instruction(pLexer->GetCurrLine(), OPCODE_EQ));
}
|	Expression T_NE Expression
{
	pScript->AddInstruction(new NXSL_Instruction(pLexer->GetCurrLine(), OPCODE_NE));
}
|	Expression '<' Expression
{
	pScript->AddInstruction(new NXSL_Instruction(pLexer->GetCurrLine(), OPCODE_LT));
}
|	Expression T_LE Expression
{
	pScript->AddInstruction(new NXSL_Instruction(pLexer->GetCurrLine(), OPCODE_LE));
}
|	Expression '>' Expression
{
	pScript->AddInstruction(new NXSL_Instruction(pLexer->GetCurrLine(), OPCODE_GT));
}
|	Expression T_GE Expression
{
	pScript->AddInstruction(new NXSL_Instruction(pLexer->GetCurrLine(), OPCODE_GE));
}
|	Expression '&' Expression
{
	pScript->AddInstruction(new NXSL_Instruction(pLexer->GetCurrLine(), OPCODE_BIT_AND));
}
|	Expression '|' Expression
{
	pScript->AddInstruction(new NXSL_Instruction(pLexer->GetCurrLine(), OPCODE_BIT_OR));
}
|	Expression '^' Expression
{
	pScript->AddInstruction(new NXSL_Instruction(pLexer->GetCurrLine(), OPCODE_BIT_XOR));
}
|	Expression T_AND Expression
{
	pScript->AddInstruction(new NXSL_Instruction(pLexer->GetCurrLine(), OPCODE_AND));
}
|	Expression T_OR Expression
{
	pScript->AddInstruction(new NXSL_Instruction(pLexer->GetCurrLine(), OPCODE_OR));
}
|	Expression T_LSHIFT Expression
{
	pScript->AddInstruction(new NXSL_Instruction(pLexer->GetCurrLine(), OPCODE_LSHIFT));
}
|	Expression T_RSHIFT Expression
{
	pScript->AddInstruction(new NXSL_Instruction(pLexer->GetCurrLine(), OPCODE_RSHIFT));
}
|	Expression '.' Expression
{
	pScript->AddInstruction(new NXSL_Instruction(pLexer->GetCurrLine(), OPCODE_CONCAT));
}
|	Operand
;

Operand:
	FunctionCall
|	T_IDENTIFIER
{
	pScript->AddInstruction(new NXSL_Instruction(pLexer->GetCurrLine(), OPCODE_PUSH_VARIABLE, $1));
}
|	Constant
{
	pScript->AddInstruction(new NXSL_Instruction(pLexer->GetCurrLine(), OPCODE_PUSH_CONSTANT, $1));
}
;

BuiltinStatement:
	SimpleStatement ';'
|	IfStatement
;

SimpleStatement:
	SimpleStatementKeyword Expression
{
	pScript->AddInstruction($1);
}
|	SimpleStatementKeyword
{
	pScript->AddInstruction(new NXSL_Instruction(pLexer->GetCurrLine(), OPCODE_PUSH_CONSTANT, new NXSL_Value));
	pScript->AddInstruction($1);
}
;

SimpleStatementKeyword:
	T_EXIT
{
	$$ = new NXSL_Instruction(pLexer->GetCurrLine(), OPCODE_EXIT);
}
|	T_RETURN
{
	$$ = new NXSL_Instruction(pLexer->GetCurrLine(), OPCODE_RETURN);
}
|	T_PRINT
{
	$$ = new NXSL_Instruction(pLexer->GetCurrLine(), OPCODE_PRINT);
}
;

IfStatement:
	T_IF '(' Expression ')' 
	{ 
		pScript->AddInstruction(new NXSL_Instruction(pLexer->GetCurrLine(), OPCODE_JZ, INVALID_ADDRESS));
	} 
	IfBody
;

IfBody:
	StatementOrBlock
{
	pScript->ResolveLastJump(OPCODE_JZ);
}
|	StatementOrBlock ElseStatement
{
	pScript->ResolveLastJump(OPCODE_JMP);
}
;

ElseStatement:
	T_ELSE
	{
		pScript->AddInstruction(new NXSL_Instruction(pLexer->GetCurrLine(), OPCODE_JMP, INVALID_ADDRESS));
		pScript->ResolveLastJump(OPCODE_JZ);
	}
	StatementOrBlock
;

FunctionCall:
	FunctionName ParameterList ')'
{
	pScript->AddInstruction(new NXSL_Instruction(pLexer->GetCurrLine(), OPCODE_CALL_EXTERNAL, $1, $2));
}
|	FunctionName ')'
{
	pScript->AddInstruction(new NXSL_Instruction(pLexer->GetCurrLine(), OPCODE_CALL_EXTERNAL, $1, 0));
}
;

ParameterList:
	Expression ',' ParameterList { $$ = $3 + 1; }
|	Expression { $$ = 1; }
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
	free($1);
}
|	T_INTEGER
{
	$$ = new NXSL_Value($1);
}
|	T_REAL
{
	$$ = new NXSL_Value($1);
}
|	T_NULL
{
	$$ = new NXSL_Value;
}
;
