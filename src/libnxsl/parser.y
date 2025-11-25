%{

#ifdef _WIN32
#pragma warning(disable : 4065 4102)
#define _CRT_SECURE_NO_WARNINGS
#endif

#define YYERROR_VERBOSE
#define YYINCLUDED_STDLIB_H
#define YYDEBUG			1

#include <nms_common.h>
#include "libnxsl.h"
#include "parser.tab.hpp"

void yyerror(yyscan_t scanner, NXSL_Lexer *lexer, NXSL_Compiler *compiler, NXSL_ProgramBuilder *builder, const char *text);
int yylex(YYSTYPE *lvalp, yyscan_t scanner);

%}

%expect		1
%pure-parser
%lex-param		{yyscan_t scanner}
%parse-param	{yyscan_t scanner}
%parse-param	{NXSL_Lexer *lexer}
%parse-param	{NXSL_Compiler *compiler}
%parse-param	{NXSL_ProgramBuilder *builder}

%union
{
	int32_t valInt32;
	uint32_t valUInt32;
	int64_t valInt64;
	uint64_t valUInt64;
	char *valStr;
	identifier_t valIdentifier;
	double valReal;
	NXSL_Value *constant;
	SimpleStatementData simpleStatement;
	import_symbol_t importSymbol;
}

%token T_ABORT
%token T_ARRAY
%token T_AS
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
%token T_FROM
%token T_FSTRING_BEGIN
%token T_FUNCTION
%token T_GLOBAL
%token T_HAS
%token T_IDIV
%token T_IF
%token T_IMPORT
%token T_LOCAL
%token T_META
%token T_NEW
%token T_NULL
%token T_OPTIONAL
%token T_RANGE
%token T_RETURN
%token T_SELECT
%token T_SWITCH
%token T_TRUE
%token T_TRY
%token T_TYPE_BOOLEAN
%token T_TYPE_INT32
%token T_TYPE_INT64
%token T_TYPE_REAL
%token T_TYPE_STRING
%token T_TYPE_UINT32
%token T_TYPE_UINT64
%token T_WHEN
%token T_WHILE
%token T_WITH

%token <valIdentifier> T_COMPOUND_IDENTIFIER
%token <valIdentifier> T_IDENTIFIER
%token <valIdentifier> T_WILDCARD_IDENTIFIER
%token <valStr> T_STRING
%token <valStr> T_FSTRING
%token <valStr> T_FSTRING_END
%token <valInt32> T_INT32
%token <valUInt32> T_UINT32
%token <valInt64> T_INT64
%token <valUInt64> T_UINT64
%token <valReal> T_REAL

%right '=' T_ASSIGN_ADD T_ASSIGN_SUB T_ASSIGN_MUL T_ASSIGN_DIV T_ASSIGN_IDIV T_ASSIGN_REM T_ASSIGN_CONCAT T_ASSIGN_AND T_ASSIGN_OR T_ASSIGN_XOR
%right ':'
%left '?'
%left T_CONCAT
%left T_HAS
%left T_OR
%left T_AND
%left '|'
%left '^'
%left '&'
%left T_NE T_EQ T_LIKE T_ILIKE T_MATCH T_IMATCH T_IN
%left '<' T_LE '>' T_GE
%left T_LSHIFT T_RSHIFT
%left '+' '-'
%left '*' '/' T_IDIV '%'
%right T_INC T_DEC T_NOT '~' NEGATE
%left T_REF T_SAFEREF '@'
%left T_POST_INC T_POST_DEC '[' ']'
%left T_RANGE

%type <constant> Constant
%type <valIdentifier> AnyIdentifier
%type <valIdentifier> FunctionName
%type <valIdentifier> GlobalVariableDeclarationStart
%type <valIdentifier> ModuleName
%type <valIdentifier> OptionalFunctionName
%type <valInt32> BuiltinType
%type <valInt32> ParameterList
%type <valInt32> SelectList
%type <simpleStatement> SimpleStatementKeyword
%type <importSymbol> ImportSymbol

%destructor { MemFree($$); } <valStr>
%destructor { builder->destroyValue($$); } <constant>

%start Script


%%

Script:
	MetadataHeader ScriptBody
;

MetadataHeader:
	MetadataStatement MetadataHeader
|
;

ScriptBody:
	ModuleComponent ModuleBody
{
	char szErrorText[256];

	// Add implicit main() function
	if (!builder->addFunction("$main", 0, szErrorText))
	{
		compiler->error(szErrorText);
		YYERROR;
	}
	
	// Implicit return
	builder->addInstruction(lexer->getCurrLine(), OPCODE_RET_NULL);
}
|	ExpressionStatement
{
	char szErrorText[256];

	// Add implicit return
	builder->addInstruction(lexer->getCurrLine(), OPCODE_RETURN);

	// Add implicit main() function
	if (!builder->addFunction("$main", 0, szErrorText))
	{
		compiler->error(szErrorText);
		YYERROR;
	}
}
|
{
	char szErrorText[256];

	// Add implicit return
	builder->addInstruction(lexer->getCurrLine(), OPCODE_RET_NULL);

	// Add implicit main() function
	if (!builder->addFunction("$main", 0, szErrorText))
	{
		compiler->error(szErrorText);
		YYERROR;
	}
}
;

ModuleBody:
	ModuleBodyComponent ModuleBody
|
;

ModuleBodyComponent:
	ModuleComponent
|	MetadataStatement
;

ModuleComponent:
	ConstStatement
|	Function
|	StatementOrBlock
|	ImportStatement
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
	if (!builder->addConstant($1, $3))
	{
		compiler->error("Constant already defined");
		builder->destroyValue($3);
		$3 = nullptr;
		YYERROR;
	}
	$3 = nullptr;
}
;

ImportStatement:
	T_IMPORT ModuleName ';'
{
	builder->addRequiredModule($2.v, lexer->getCurrLine(), false, true, false);
}
|	T_IMPORT ImportSymbolList T_FROM ModuleName ';'
{
	builder->finalizeSymbolImport($4.v);
	builder->addRequiredModule($4.v, lexer->getCurrLine(), false, false, false);
}
;

ImportSymbolList:
	ImportSymbol ',' ImportSymbolList
{
	builder->addImportSymbol($1);
}
|	ImportSymbol
{
	builder->addImportSymbol($1);
}
;

ImportSymbol:
	T_IDENTIFIER
{
	$$ = { $1, $1, lexer->getCurrLine() };
}
|	T_IDENTIFIER T_AS T_IDENTIFIER
{
	$$ = { $1, $3, lexer->getCurrLine() };
}
;

ModuleName:
	AnyIdentifier
|	T_WILDCARD_IDENTIFIER
;

AnyIdentifier:
	T_IDENTIFIER
|	T_COMPOUND_IDENTIFIER
;

Function:
	T_FUNCTION FunctionName 
	{
		char szErrorText[256];

		builder->addInstruction(lexer->getCurrLine(), OPCODE_JMP, INVALID_ADDRESS);

		if (!builder->addFunction($2, INVALID_ADDRESS, szErrorText))
		{
			compiler->error(szErrorText);
			YYERROR;
		}
		compiler->setIdentifierOperation(OPCODE_BIND);
	}
	ParameterDeclaration Block
	{
		builder->addInstruction(lexer->getCurrLine(), OPCODE_RET_NULL);
		builder->resolveLastJump(OPCODE_JMP);
	}
;

ParameterDeclaration:
	IdentifierList ')'
|	')'
;

IdentifierList:
	T_IDENTIFIER 
	{
		builder->addInstruction(lexer->getCurrLine(), compiler->getIdentifierOperation(), $1);
	}
	',' IdentifierList
|	T_IDENTIFIER
	{
		builder->addInstruction(lexer->getCurrLine(), compiler->getIdentifierOperation(), $1);
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
		builder->addInstruction(lexer->getCurrLine(), OPCODE_CATCH, INVALID_ADDRESS);
	} 
	Block T_CATCH 
	{
		builder->addInstruction(lexer->getCurrLine(), OPCODE_CPOP);
		builder->addInstruction(lexer->getCurrLine(), OPCODE_JMP, INVALID_ADDRESS);
		builder->resolveLastJump(OPCODE_CATCH);
	} 
	Block
	{
		builder->resolveLastJump(OPCODE_JMP);
	}
;

Statement:
	ExpressionStatement ';'
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_POP, static_cast<int16_t>(1));
}
|	BuiltinStatement
|	';'
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_NOP);
}
;

ExpressionStatement:
	T_WITH
{
	builder->enableExpressionVariables();
} 
	WithAssignmentList Expression
{
	builder->disableExpressionVariables(lexer->getCurrLine());
}
|	Expression
;

WithAssignmentList:
	WithAssignment ',' WithAssignmentList
|	WithAssignment
;

WithAssignment:
	T_IDENTIFIER
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_JMP, INVALID_ADDRESS);
	builder->registerExpressionVariable($1);
}
	Metadata '=' WithCalculationBlock
{
	builder->resolveLastJump(OPCODE_JMP);
}	
;

MetadataStatement:
    T_META '(' MetadataValues ')'
;

Metadata:
	'(' MetadataValues ')'
|
;

MetadataValues:
	MetadataValue ',' MetadataValues
|	MetadataValue
;

MetadataValue:
	T_IDENTIFIER '=' Constant
{
   TCHAR key[1024];
   if (builder->getCurrentMetadataPrefix().length > 0)
   {
#ifdef UNICODE
      size_t l = utf8_to_wchar(builder->getCurrentMetadataPrefix().value, -1, key, 1024);
	   key[l - 1] = L'.';
      utf8_to_wchar($1.v, -1, &key[l], 1024 - l);
#else
	   strlcpy(key, builder->getCurrentMetadataPrefix().value, 1024);
	   strlcat(key, ".", 1024);
	   strlcat(key, $1.v, 1024);
#endif
   }
   else
   {
#ifdef UNICODE
      utf8_to_wchar($1.v, -1, key, 1024);
#else
      strlcpy(key, $1.v, 1024);
#endif
   }
   builder->setMetadata(key, $3->getValueAsCString());
	builder->destroyValue($3);
	$3 = nullptr;
}
;

WithCalculationBlock:
	'{' StatementList '}'
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_RET_NULL);
}
|	'{' Expression '}'
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_RETURN);
}
;

Expression:
	'(' Expression ')'
|	T_IDENTIFIER '=' Expression
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_SET, $1);
}
|	T_IDENTIFIER T_ASSIGN_ADD { builder->addPushVariableInstruction($1, lexer->getCurrLine()); } Expression
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_ADD);
	builder->addInstruction(lexer->getCurrLine(), OPCODE_SET, $1);
}
|	T_IDENTIFIER T_ASSIGN_SUB { builder->addPushVariableInstruction($1, lexer->getCurrLine()); } Expression
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_SUB);
	builder->addInstruction(lexer->getCurrLine(), OPCODE_SET, $1);
}
|	T_IDENTIFIER T_ASSIGN_MUL { builder->addPushVariableInstruction($1, lexer->getCurrLine()); } Expression
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_MUL);
	builder->addInstruction(lexer->getCurrLine(), OPCODE_SET, $1);
}
|	T_IDENTIFIER T_ASSIGN_DIV { builder->addPushVariableInstruction($1, lexer->getCurrLine()); } Expression
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_DIV);
	builder->addInstruction(lexer->getCurrLine(), OPCODE_SET, $1);
}
|	T_IDENTIFIER T_ASSIGN_IDIV { builder->addPushVariableInstruction($1, lexer->getCurrLine()); } Expression
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_IDIV);
	builder->addInstruction(lexer->getCurrLine(), OPCODE_SET, $1);
}
|	T_IDENTIFIER T_ASSIGN_REM { builder->addPushVariableInstruction($1, lexer->getCurrLine()); } Expression
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_REM);
	builder->addInstruction(lexer->getCurrLine(), OPCODE_SET, $1);
}
|	T_IDENTIFIER T_ASSIGN_CONCAT { builder->addPushVariableInstruction($1, lexer->getCurrLine()); } Expression
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_CONCAT);
	builder->addInstruction(lexer->getCurrLine(), OPCODE_SET, $1);
}
|	T_IDENTIFIER T_ASSIGN_AND { builder->addPushVariableInstruction($1, lexer->getCurrLine()); } Expression
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_BIT_AND);
	builder->addInstruction(lexer->getCurrLine(), OPCODE_SET, $1);
}
|	T_IDENTIFIER T_ASSIGN_OR { builder->addPushVariableInstruction($1, lexer->getCurrLine()); } Expression
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_BIT_OR);
	builder->addInstruction(lexer->getCurrLine(), OPCODE_SET, $1);
}
|	T_IDENTIFIER T_ASSIGN_XOR { builder->addPushVariableInstruction($1, lexer->getCurrLine()); } Expression
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_BIT_XOR);
	builder->addInstruction(lexer->getCurrLine(), OPCODE_SET, $1);
}
|	StorageItem T_ASSIGN_ADD { builder->addInstruction(lexer->getCurrLine(), OPCODE_STORAGE_READ, static_cast<int16_t>(1)); } Expression
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_ADD);
	builder->addInstruction(lexer->getCurrLine(), OPCODE_STORAGE_WRITE);
}
|	StorageItem T_ASSIGN_SUB { builder->addInstruction(lexer->getCurrLine(), OPCODE_STORAGE_READ, static_cast<int16_t>(1)); } Expression
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_SUB);
	builder->addInstruction(lexer->getCurrLine(), OPCODE_STORAGE_WRITE);
}
|	StorageItem T_ASSIGN_MUL { builder->addInstruction(lexer->getCurrLine(), OPCODE_STORAGE_READ, static_cast<int16_t>(1)); } Expression
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_MUL);
	builder->addInstruction(lexer->getCurrLine(), OPCODE_STORAGE_WRITE);
}
|	StorageItem T_ASSIGN_DIV { builder->addInstruction(lexer->getCurrLine(), OPCODE_STORAGE_READ, static_cast<int16_t>(1)); } Expression
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_DIV);
	builder->addInstruction(lexer->getCurrLine(), OPCODE_STORAGE_WRITE);
}
|	StorageItem T_ASSIGN_REM { builder->addInstruction(lexer->getCurrLine(), OPCODE_STORAGE_READ, static_cast<int16_t>(1)); } Expression
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_REM);
	builder->addInstruction(lexer->getCurrLine(), OPCODE_STORAGE_WRITE);
}
|	StorageItem T_ASSIGN_CONCAT { builder->addInstruction(lexer->getCurrLine(), OPCODE_STORAGE_READ, static_cast<int16_t>(1)); } Expression
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_CONCAT);
	builder->addInstruction(lexer->getCurrLine(), OPCODE_STORAGE_WRITE);
}
|	StorageItem T_ASSIGN_AND { builder->addInstruction(lexer->getCurrLine(), OPCODE_STORAGE_READ, static_cast<int16_t>(1)); } Expression
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_BIT_AND);
	builder->addInstruction(lexer->getCurrLine(), OPCODE_STORAGE_WRITE);
}
|	StorageItem T_ASSIGN_OR { builder->addInstruction(lexer->getCurrLine(), OPCODE_STORAGE_READ, static_cast<int16_t>(1)); } Expression
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_BIT_OR);
	builder->addInstruction(lexer->getCurrLine(), OPCODE_STORAGE_WRITE);
}
|	StorageItem T_ASSIGN_XOR { builder->addInstruction(lexer->getCurrLine(), OPCODE_STORAGE_READ, static_cast<int16_t>(1)); } Expression
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_BIT_XOR);
	builder->addInstruction(lexer->getCurrLine(), OPCODE_STORAGE_WRITE);
}
|	Expression '[' Expression ']' '=' Expression
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_SET_ELEMENT);
}
|	Expression '[' Expression ']' T_ASSIGN_ADD { builder->addInstruction(lexer->getCurrLine(), OPCODE_PEEK_ELEMENT); } Expression
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_ADD);
	builder->addInstruction(lexer->getCurrLine(), OPCODE_SET_ELEMENT);
}
|	Expression '[' Expression ']' T_ASSIGN_SUB { builder->addInstruction(lexer->getCurrLine(), OPCODE_PEEK_ELEMENT); } Expression
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_SUB);
	builder->addInstruction(lexer->getCurrLine(), OPCODE_SET_ELEMENT);
}
|	Expression '[' Expression ']' T_ASSIGN_MUL { builder->addInstruction(lexer->getCurrLine(), OPCODE_PEEK_ELEMENT); } Expression
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_MUL);
	builder->addInstruction(lexer->getCurrLine(), OPCODE_SET_ELEMENT);
}
|	Expression '[' Expression ']' T_ASSIGN_DIV { builder->addInstruction(lexer->getCurrLine(), OPCODE_PEEK_ELEMENT); } Expression
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_DIV);
	builder->addInstruction(lexer->getCurrLine(), OPCODE_SET_ELEMENT);
}
|	Expression '[' Expression ']' T_ASSIGN_CONCAT { builder->addInstruction(lexer->getCurrLine(), OPCODE_PEEK_ELEMENT); } Expression
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_CONCAT);
	builder->addInstruction(lexer->getCurrLine(), OPCODE_SET_ELEMENT);
}
|	Expression '[' Expression ']' T_ASSIGN_AND { builder->addInstruction(lexer->getCurrLine(), OPCODE_PEEK_ELEMENT); } Expression
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_BIT_AND);
	builder->addInstruction(lexer->getCurrLine(), OPCODE_SET_ELEMENT);
}
|	Expression '[' Expression ']' T_ASSIGN_OR { builder->addInstruction(lexer->getCurrLine(), OPCODE_PEEK_ELEMENT); } Expression
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_BIT_OR);
	builder->addInstruction(lexer->getCurrLine(), OPCODE_SET_ELEMENT);
}
|	Expression '[' Expression ']' T_ASSIGN_XOR { builder->addInstruction(lexer->getCurrLine(), OPCODE_PEEK_ELEMENT); } Expression
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_BIT_XOR);
	builder->addInstruction(lexer->getCurrLine(), OPCODE_SET_ELEMENT);
}
|	Expression '[' Expression ']'
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_GET_ELEMENT);
}
|	Expression '[' ExpressionOrNone ':' ExpressionOrNone ']'
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_GET_RANGE);
}
|	StorageItem '=' Expression
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_STORAGE_WRITE);
}
|	StorageItem
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_STORAGE_READ);
}
|	Expression T_REF T_IDENTIFIER '=' Expression
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_SET_ATTRIBUTE, $3);
}
|	Expression T_REF T_IDENTIFIER
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_GET_ATTRIBUTE, $3);
}
|	Expression T_SAFEREF T_IDENTIFIER
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_SAFE_GET_ATTR, $3);
}
|	Expression T_REF FunctionName { builder->addInstruction(lexer->getCurrLine(), OPCODE_ARGV); } ParameterList ')'
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_CALL_METHOD, $3, $5);
}
|	Expression T_SAFEREF FunctionName { builder->addInstruction(lexer->getCurrLine(), OPCODE_ARGV); } ParameterList ')'
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_SAFE_CALL, $3, $5);
}
|	Expression T_REF FunctionName ')'
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_CALL_METHOD, $3, 0);
}
|	Expression T_SAFEREF FunctionName ')'
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_SAFE_CALL, $3, 0);
}
|	T_IDENTIFIER '@' Expression
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_SAFE_GET_ATTR, $1);
	compiler->warning(_T("Operation \"@\" is deprecated (safe dereference operation \"?.\" should be used instead)"));	
}
|	'-' Expression		%prec NEGATE
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_NEG);
}
|	T_NOT Expression
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_NOT);
}
|	'~' Expression
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_BIT_NOT);
}
|	T_INC T_IDENTIFIER
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_INCP, $2);
}
|	T_DEC T_IDENTIFIER
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_DECP, $2);
}
|	T_IDENTIFIER T_INC	%prec T_POST_INC
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_INC, $1);
}
|	T_IDENTIFIER T_DEC	%prec T_POST_DEC
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_DEC, $1);
}
|	T_INC StorageItem
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_STORAGE_INCP);
}
|	T_DEC StorageItem
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_STORAGE_DECP);
}
|	StorageItem T_INC	%prec T_POST_INC
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_STORAGE_INC);
}
|	StorageItem T_DEC	%prec T_POST_DEC
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_STORAGE_DEC);
}
|	T_INC '(' Expression '[' Expression ']' ')'
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_INCP_ELEMENT);
}
|	T_DEC '(' Expression '[' Expression ']' ')'
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_DECP_ELEMENT);
}
|	Expression '[' Expression ']' T_INC	%prec T_POST_INC
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_INC_ELEMENT);
}
|	Expression '[' Expression ']' T_DEC	%prec T_POST_DEC
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_DEC_ELEMENT);
}
|	Expression '+' Expression	
{ 
	builder->addInstruction(lexer->getCurrLine(), OPCODE_ADD);
}
|	Expression '-' Expression
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_SUB);
}
|	Expression '*' Expression
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_MUL);
}
|	Expression '/' Expression
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_DIV);
}
|	Expression T_IDIV Expression
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_IDIV);
}
|	Expression '%' Expression
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_REM);
}
|	Expression T_LIKE Expression
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_LIKE);
}
|	Expression T_ILIKE Expression
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_ILIKE);
}
|	Expression T_MATCH Expression
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_MATCH);
}
|	Expression T_IMATCH Expression
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_IMATCH);
}
|	Expression T_IN Expression
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_IN);
}
|	Expression T_EQ Expression
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_EQ);
}
|	Expression T_NE Expression
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_NE);
}
|	Expression '<' Expression
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_LT);
}
|	Expression T_LE Expression
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_LE);
}
|	Expression '>' Expression
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_GT);
}
|	Expression T_GE Expression
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_GE);
}
|	Expression '&' Expression
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_BIT_AND);
}
|	Expression '|' Expression
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_BIT_OR);
}
|	Expression '^' Expression
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_BIT_XOR);
}
|	Expression T_AND { builder->addInstruction(lexer->getCurrLine(), OPCODE_JZ_PEEK, INVALID_ADDRESS); } Expression
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_AND);
	builder->resolveLastJump(OPCODE_JZ_PEEK);
}
|	Expression T_OR { builder->addInstruction(lexer->getCurrLine(), OPCODE_JNZ_PEEK, INVALID_ADDRESS); } Expression
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_OR);
	builder->resolveLastJump(OPCODE_JNZ_PEEK);
}
|	Expression T_LSHIFT Expression
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_LSHIFT);
}
|	Expression T_RSHIFT Expression
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_RSHIFT);
}
|	Expression T_CONCAT Expression
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_CONCAT);
}
|	Expression T_HAS Expression
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_HAS_BITS);
}
|	Expression '?'
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_JZ, INVALID_ADDRESS);
}
	Expression ':'
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_JMP, INVALID_ADDRESS);
	builder->resolveLastJump(OPCODE_JZ);
}	 
	Expression
{
	builder->resolveLastJump(OPCODE_JMP);
}
|	Operand
;

ExpressionOrNone:
	Expression
|
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_PUSH_CONSTANT, builder->createValue());
}
;

Operand:
	FunctionCall
|	TypeCast
|	ArrayInitializer
|	HashMapInitializer
|	New
|	T_IDENTIFIER
{
	builder->addPushVariableInstruction($1, lexer->getCurrLine());
}
|	T_COMPOUND_IDENTIFIER
{
   // Special case for VM properties
   if (!strcmp($1.v, "NXSL::Classes") || !strcmp($1.v, "NXSL::Functions"))
   {
      builder->addInstruction(lexer->getCurrLine(), OPCODE_PUSH_PROPERTY, $1);
   }
   else
   {
      NXSL_Value *constValue = builder->getConstantValue($1);
      if (constValue != nullptr)
      {
	      builder->addInstruction(lexer->getCurrLine(), OPCODE_PUSH_CONSTANT, constValue);
	   }
      else
      {
		   builder->addInstruction(lexer->getCurrLine(), OPCODE_PUSH_CONSTREF, $1);
	      builder->addRequiredModule($1.v, lexer->getCurrLine(), true, false, true);
		}
   }
}
|  FString
|	Constant
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_PUSH_CONSTANT, $1);
	$1 = nullptr;
}
;

TypeCast:
	BuiltinType '(' Expression ')'
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_CAST, static_cast<int16_t>($1));
}
;

ArrayInitializer:
	'['
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_NEW_ARRAY);
}
	ArrayElements ']'
|	'[' ']'
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_NEW_ARRAY);
}
|	'%' '('
{
	compiler->warning(_T("Array initialization with \"%%()\" is deprecated, use \"[]\" instead"));
	builder->addInstruction(lexer->getCurrLine(), OPCODE_NEW_ARRAY);
}
	ArrayElements ')'
|	'%' '(' ')'
{
	compiler->warning(_T("Array initialization with \"%%()\" is deprecated, use \"[]\" instead"));
	builder->addInstruction(lexer->getCurrLine(), OPCODE_NEW_ARRAY);
}
;

ArrayElements:
	ArrayElement ',' ArrayElements
|	ArrayElement
;

ArrayElement:
	Expression 
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_APPEND);
}
|	T_RANGE Expression 
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_APPEND_ALL);
}
;

HashMapInitializer:
	'%' '{'
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_NEW_HASHMAP);
}
	HashMapElements '}'
|	'%' '{' '}'
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_NEW_HASHMAP);
}
;

HashMapElements:
	HashMapElement ',' HashMapElements
|	HashMapElement
;

HashMapElement:
	Expression ':' Expression
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_HASHMAP_SET);
}

BuiltinType:
	T_TYPE_BOOLEAN
{
	$$ = NXSL_DT_BOOLEAN;
}
|	T_TYPE_INT32
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
|	IfStatement
|	DoStatement
|	WhileStatement
|	ForStatement
|	ForEachStatement
|	SwitchStatement
|	SelectStatement
|	ArrayStatement
|	GlobalStatement
|	LocalStatement
|	T_BREAK ';'
{
	if (compiler->canUseBreak())
	{
		builder->addInstruction(lexer->getCurrLine(), OPCODE_NOP);
		compiler->addBreakAddr(builder->getCodeSize() - 1);
	}
	else
	{
		compiler->error("\"break\" statement can be used only within loops, \"switch\", and \"select\" statements");
		YYERROR;
	}
}
|	T_CONTINUE ';'
{
	uint32_t addr = compiler->peekAddr();
	if (addr != INVALID_ADDRESS)
	{
		builder->addInstruction(lexer->getCurrLine(), OPCODE_JMP, addr);
	}
	else
	{
		compiler->error("\"continue\" statement can be used only within loops");
		YYERROR;
	}
}
;

SimpleStatement:
	SimpleStatementKeyword Expression
{
	builder->addInstruction($1.sourceLine, $1.opCode);
}
|	SimpleStatementKeyword
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_PUSH_CONSTANT, builder->createValue());
	builder->addInstruction($1.sourceLine, $1.opCode);
}
;

SimpleStatementKeyword:
	T_ABORT
{
	$$.sourceLine = lexer->getCurrLine();
	$$.opCode = OPCODE_ABORT;
}
|	T_EXIT
{
	$$.sourceLine = lexer->getCurrLine();
	$$.opCode = OPCODE_EXIT;
}
|	T_RETURN
{
   if (compiler->getTemporaryStackItems() > 0)
   {
	   builder->addInstruction(lexer->getCurrLine(), OPCODE_POP, static_cast<int16_t>(compiler->getTemporaryStackItems()));
   }
	$$.sourceLine = lexer->getCurrLine();
	$$.opCode = OPCODE_RETURN;
}
;

IfStatement:
	T_IF '(' Expression ')' 
	{
		builder->addInstruction(lexer->getCurrLine(), OPCODE_JZ, INVALID_ADDRESS);
	} 
	IfBody
;

IfBody:
	StatementOrBlock
{
	builder->resolveLastJump(OPCODE_JZ);
}
|	StatementOrBlock ElseStatement
{
	builder->resolveLastJump(OPCODE_JMP);
}
;

ElseStatement:
	T_ELSE
	{
		builder->addInstruction(lexer->getCurrLine(), OPCODE_JMP, INVALID_ADDRESS);
		builder->resolveLastJump(OPCODE_JZ);
	}
	StatementOrBlock
;

ForStatement:
	T_FOR '(' Expression ';'
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_POP, static_cast<int16_t>(1));
	compiler->pushAddr(builder->getCodeSize());
}
	Expression ';'
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_JZ, INVALID_ADDRESS);
	builder->addInstruction(lexer->getCurrLine(), OPCODE_JMP, INVALID_ADDRESS);
	compiler->pushAddr(builder->getCodeSize());
}
	Expression ')'
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_POP, static_cast<int16_t>(1));
	uint32_t addrPart3 = compiler->popAddr();
	builder->addInstruction(lexer->getCurrLine(), OPCODE_JMP, compiler->popAddr());
	compiler->pushAddr(addrPart3);
	compiler->newBreakLevel();
	builder->resolveLastJump(OPCODE_JMP);
}	
	StatementOrBlock
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_JMP, compiler->popAddr());
	builder->resolveLastJump(OPCODE_JZ);
	compiler->closeBreakLevel(builder);
}
;

ForEachStatement:
	ForEach { compiler->incTemporaryStackItems(); } ForEachBody { compiler->decTemporaryStackItems(); }
;

ForEach:
	T_FOREACH '(' T_IDENTIFIER ':'
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_PUSH_CONSTANT, builder->createValue($3.v));
}
|
	T_FOR '(' T_IDENTIFIER ':'
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_PUSH_CONSTANT, builder->createValue($3.v));
}
;

ForEachBody:
	Expression ')'
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_FOREACH);
	compiler->pushAddr(builder->getCodeSize());
	compiler->newBreakLevel();
	builder->addInstruction(lexer->getCurrLine(), OPCODE_NEXT);
	builder->addInstruction(lexer->getCurrLine(), OPCODE_JZ, INVALID_ADDRESS);
}
	StatementOrBlock
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_JMP, compiler->popAddr());
	builder->resolveLastJump(OPCODE_JZ);
	compiler->closeBreakLevel(builder);
	builder->addInstruction(lexer->getCurrLine(), OPCODE_POP, static_cast<int16_t>(1));
}
;

WhileStatement:
	T_WHILE
{
	compiler->pushAddr(builder->getCodeSize());
	compiler->newBreakLevel();
}
	'(' Expression ')'
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_JZ, INVALID_ADDRESS);
}
	StatementOrBlock
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_JMP, compiler->popAddr());
	builder->resolveLastJump(OPCODE_JZ);
	compiler->closeBreakLevel(builder);
}
;

DoStatement:
	T_DO
{
	compiler->pushAddr(builder->getCodeSize());
	compiler->newBreakLevel();
}
	StatementOrBlock T_WHILE '(' Expression ')' ';'
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_JNZ, compiler->popAddr());
	compiler->closeBreakLevel(builder);
}
;

SwitchStatement:
	T_SWITCH
{ 
	compiler->newBreakLevel();
}
	'(' Expression ')'
{
	compiler->incTemporaryStackItems();
}
	'{' CaseList Default '}'
{
	compiler->closeBreakLevel(builder);
	compiler->decTemporaryStackItems();
	builder->addInstruction(lexer->getCurrLine(), OPCODE_POP, static_cast<int16_t>(1));
}
;

CaseList:
	Case
{ 
	builder->addInstruction(lexer->getCurrLine(), OPCODE_JMP, builder->getCodeSize() + 5);
	builder->resolveLastJump(OPCODE_JZ);
}
	CaseList
|	RangeCase
{ 
	builder->addInstruction(lexer->getCurrLine(), OPCODE_JMP, builder->getCodeSize() + 5);
	builder->resolveLastJump(OPCODE_JNZ);
	builder->resolveLastJump(OPCODE_JNZ);
}
	CaseList
|	Case
{
	builder->resolveLastJump(OPCODE_JZ);
}
|	RangeCase
{
	builder->resolveLastJump(OPCODE_JNZ);
	builder->resolveLastJump(OPCODE_JNZ);
}
;

Case:
	T_CASE Constant
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_CASE, $2);
	builder->addInstruction(lexer->getCurrLine(), OPCODE_JZ, INVALID_ADDRESS);
	builder->addInstruction(lexer->getCurrLine(), OPCODE_NOP);	// Needed to match number of instructions in case and range case
	builder->addInstruction(lexer->getCurrLine(), OPCODE_NOP);
	$2 = nullptr;
} 
	':' StatementList
|	T_CASE AnyIdentifier
{
   NXSL_Value *constValue = builder->getConstantValue($2);
   if (constValue != nullptr)
      builder->addInstruction(lexer->getCurrLine(), OPCODE_CASE, constValue);
   else
      builder->addInstruction(lexer->getCurrLine(), OPCODE_CASE_CONST, $2);
   builder->addInstruction(lexer->getCurrLine(), OPCODE_JZ, INVALID_ADDRESS);
	builder->addInstruction(lexer->getCurrLine(), OPCODE_NOP);	// Needed to match number of instructions in case and range case
	builder->addInstruction(lexer->getCurrLine(), OPCODE_NOP);
} 
	':' StatementList
;

RangeCase:
	T_CASE CaseRangeLeft
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_JNZ, INVALID_ADDRESS);
}
T_RANGE CaseRangeRight 
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_JNZ, INVALID_ADDRESS);
}
	':' StatementList
;

CaseRangeLeft:
	Constant
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_CASE_LT, $1);
	$1 = nullptr;
}
|	AnyIdentifier
{
   NXSL_Value *constValue = builder->getConstantValue($1);
   if (constValue != nullptr)
      builder->addInstruction(lexer->getCurrLine(), OPCODE_CASE_LT, constValue);
   else
      builder->addInstruction(lexer->getCurrLine(), OPCODE_CASE_CONST_LT, $1);
}
;

CaseRangeRight:
	Constant
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_CASE_GT, $1);
	$1 = nullptr;
}
|	AnyIdentifier
{
   NXSL_Value *constValue = builder->getConstantValue($1);
   if (constValue != nullptr)
      builder->addInstruction(lexer->getCurrLine(), OPCODE_CASE_GT, constValue);
   else
      builder->addInstruction(lexer->getCurrLine(), OPCODE_CASE_CONST_GT, $1);
}
;

Default:
	T_DEFAULT ':' StatementList
|
;

SelectStatement:
	T_SELECT
{ 
	compiler->newBreakLevel();
	compiler->newSelectLevel();
}
	T_IDENTIFIER SelectOptions	'{' SelectList '}'
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_SELECT, $3, $6);
	compiler->closeBreakLevel(builder);

	uint32_t addr = compiler->popSelectJumpAddr();
	if (addr != INVALID_ADDRESS)
	{
		builder->createJumpAt(addr, builder->getCodeSize());
	}
	compiler->closeSelectLevel();
}
;

SelectOptions:
	'(' Expression ')'
|
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_PUSH_CONSTANT, builder->createValue());
}
;

SelectList:
	SelectEntry	SelectList
{ 
	$$ = $2 + 1; 
}
|	SelectEntry
{
	$$ = 1;
}
;

SelectEntry:
	T_WHEN
{
} 
	Expression
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_PUSHCP, static_cast<int16_t>(2));
	builder->addInstruction(lexer->getCurrLine(), OPCODE_JMP, INVALID_ADDRESS);
	uint32_t addr = compiler->popSelectJumpAddr();
	if (addr != INVALID_ADDRESS)
	{
		builder->createJumpAt(addr, builder->getCodeSize());
	}
} 
	':' StatementList
{
	compiler->pushSelectJumpAddr(builder->getCodeSize());
	builder->addInstruction(lexer->getCurrLine(), OPCODE_NOP);
	builder->resolveLastJump(OPCODE_JMP);
}
;

ArrayStatement:
	T_ARRAY { compiler->setIdentifierOperation(OPCODE_ARRAY); } IdentifierList ';'
|	T_GLOBAL T_ARRAY { compiler->setIdentifierOperation(OPCODE_GLOBAL_ARRAY); } IdentifierList ';'
;

GlobalStatement:
	T_GLOBAL GlobalVariableList ';'
;

GlobalVariableList:
	GlobalVariableDeclaration ',' GlobalVariableList
|	GlobalVariableDeclaration
;

GlobalVariableDeclaration:
	GlobalVariableDeclarationStart
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_GLOBAL, $1, 0);
}
|	GlobalVariableDeclarationStart '=' Expression
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_GLOBAL, $1, 1);
}
;

GlobalVariableDeclarationStart:
	T_IDENTIFIER { builder->setCurrentMetadataPrefix($1); } Metadata
{
	$$ = $1;
}
;

LocalStatement:
	T_LOCAL LocalVariableList ';'
;

LocalVariableList:
	LocalVariableDeclaration ',' LocalVariableList
|	LocalVariableDeclaration
;

LocalVariableDeclaration:
	T_IDENTIFIER
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_LOCAL, $1, 0);
}
|	T_IDENTIFIER '=' Expression
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_LOCAL, $1, 1);
}
;

New:
	T_NEW T_IDENTIFIER
{
	char fname[256];
	snprintf(fname, 256, "__new@%s", $2.v); 
	builder->addInstruction(lexer->getCurrLine(), OPCODE_CALL_EXTERNAL, fname, 0);
}
|	T_NEW T_IDENTIFIER '(' { builder->addInstruction(lexer->getCurrLine(), OPCODE_ARGV); } ParameterList ')'
{
	char fname[256];
	snprintf(fname, 256, "__new@%s", $2.v);
	builder->addInstruction(lexer->getCurrLine(), OPCODE_CALL_EXTERNAL, fname, $5);
}
|	T_NEW T_IDENTIFIER '(' ')'
{
	char fname[256];
	snprintf(fname, 256, "__new@%s", $2.v); 
	builder->addInstruction(lexer->getCurrLine(), OPCODE_CALL_EXTERNAL, fname, 0);
}
;

FunctionCall:
	FunctionName { builder->addInstruction(lexer->getCurrLine(), OPCODE_ARGV); } ParameterList ')'
{
	if (!strcmp($1.v, "__invoke"))
	{
		builder->addInstruction(lexer->getCurrLine(), OPCODE_CALL_INDIRECT, static_cast<int16_t>($3 - 1));
	}
	else
	{
		builder->addInstruction(lexer->getCurrLine(), OPCODE_CALL_EXTERNAL, $1, $3);
		if (builder->getEnvironment()->isDeprecatedFunction($1))
			compiler->warning(_T("Function \"%hs\" is deprecated"), $1.v);
	}
}
|	FunctionName ')'
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_CALL_EXTERNAL, $1, 0);
	if (builder->getEnvironment()->isDeprecatedFunction($1))
		compiler->warning(_T("Function \"%hs\" is deprecated"), $1.v);
}
|	OptionalFunctionName { builder->addInstruction(lexer->getCurrLine(), OPCODE_ARGV); } ParameterList ')'
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_CALL_EXTERNAL, $1, $3, OPTIONAL_FUNCTION_CALL);
	if (builder->getEnvironment()->isDeprecatedFunction($1))
		compiler->warning(_T("Function \"%hs\" is deprecated"), $1.v);
}
|	OptionalFunctionName ')'
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_CALL_EXTERNAL, $1, 0, OPTIONAL_FUNCTION_CALL);
	if (builder->getEnvironment()->isDeprecatedFunction($1))
		compiler->warning(_T("Function \"%hs\" is deprecated"), $1.v);
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
	builder->addInstruction(lexer->getCurrLine(), OPCODE_NAME, $1);
}
|	T_RANGE Expression
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_SPREAD);
}
;

FunctionName:
	T_IDENTIFIER '('
{
	$$ = $1;
}
|	T_COMPOUND_IDENTIFIER '('
{
	$$ = $1;
	builder->addRequiredModule($1.v, lexer->getCurrLine(), true, false, false);
}
;

OptionalFunctionName:
	T_OPTIONAL T_IDENTIFIER '('
{
	$$ = $2;
}
|	T_OPTIONAL T_COMPOUND_IDENTIFIER '('
{
	$$ = $2;
	builder->addRequiredModule($2.v, lexer->getCurrLine(), true, false, true);
}
;

StorageItem:
	'#' T_IDENTIFIER
{
	builder->addInstruction(lexer->getCurrLine(), OPCODE_PUSH_CONSTANT, builder->createValue($2.v));
}
|	'#' '(' Expression ')'
;

FString:
   T_FSTRING_BEGIN
{
   builder->startFString();   
}
   FStringElements
;

FStringElements:
   T_FSTRING
{
   if (*$1 != 0)
   {
      builder->incFStringElements();
#ifdef UNICODE
	   builder->addInstruction(lexer->getCurrLine(), OPCODE_PUSH_CONSTANT, builder->createValue($1));
#else
	   char *mbString = MBStringFromUTF8String($1);
	   builder->addInstruction(lexer->getCurrLine(), OPCODE_PUSH_CONSTANT, builder->createValue(mbString));
	   MemFree(mbString);
#endif
   }
	MemFreeAndNull($1);
}
   Expression { builder->incFStringElements(); } FStringElements
|  T_FSTRING_END
{
   if (*$1 != 0)
   {
      builder->incFStringElements();
#ifdef UNICODE
   	builder->addInstruction(lexer->getCurrLine(), OPCODE_PUSH_CONSTANT, builder->createValue($1));
#else
   	char *mbString = MBStringFromUTF8String($1);
	   builder->addInstruction(lexer->getCurrLine(), OPCODE_PUSH_CONSTANT, builder->createValue(mbString));
	   MemFree(mbString);
#endif
   }
	MemFreeAndNull($1);
   uint32_t numElements = builder->getFStringElementCount();
   if (numElements > 1)
   {
	   builder->addInstruction(lexer->getCurrLine(), OPCODE_FSTRING, static_cast<int16_t>(numElements));
   }
}
;

Constant:
	T_STRING
{
#ifdef UNICODE
	$$ = builder->createValue($1);
#else
	char *mbString = MBStringFromUTF8String($1);
	$$ = builder->createValue(mbString);
	MemFree(mbString);
#endif
	MemFreeAndNull($1);
}
|	T_INT32
{
	$$ = builder->createValue($1);
}
|	T_UINT32
{
	$$ = builder->createValue($1);
}
|	T_INT64
{
	$$ = builder->createValue($1);
}
|	T_UINT64
{
	$$ = builder->createValue($1);
}
|	T_REAL
{
	$$ = builder->createValue($1);
}
|	T_TRUE
{
	$$ = builder->createValue(true);
}
|	T_FALSE
{
	$$ = builder->createValue(false);
}
|	T_NULL
{
	$$ = builder->createValue();
}
;
