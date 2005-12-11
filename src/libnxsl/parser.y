%{

#define YYERROR_VERBOSE
#define YYDEBUG			1

#include <nms_common.h>
#include "parser.tab.h"

void yyerror(void *pLexer, void *pCompiler, char *pszText);
int yylex(YYSTYPE *lvalp, void *pLexer);

%}

%expect		1
%pure-parser
%lex-param	{void *pLexer}
%parse-param	{void *pLexer}
%parse-param	{void *pCompiler}

%token T_ELSE
%token T_FUNCTION
%token T_IDENTIFIER
%token T_IF
%token T_INTEGER
%token T_RETURN
%token T_STRING
%token T_SUB

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
%right '='
%right T_INC T_DEC '!' '~'


%%

Script:
	Module
|	Expression
;

Module:
	ModuleComponent Module
|	ModuleComponent
;

ModuleComponent:
	Function
;

Function:
	T_SUB T_IDENTIFIER '(' ParameterDeclaration Block
;

ParameterDeclaration:
	IdentifierList ')'
|	')'
;

IdentifierList:
	T_IDENTIFIER ',' IdentifierList
|	T_IDENTIFIER
;

Block:
	'{' StatementList '}'
|	'{' '}'
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
|	BuiltinStatement
|	';'
;

Expression:
	'(' Expression ')'
|	T_IDENTIFIER '=' Expression
|	UnaryOperation Expression
|	Operand BinaryOperation Expression
{
printf("binary op: %d\n", $2);
}
|	Operand
;

Operand:
	FunctionCall
|	T_IDENTIFIER
|	Constant
;

/*PrefixOperation:
	IncrementOrDecrement
|
;

PostfixOperation:
	IncrementOrDecrement
|
;


IncrementOrDecrement:
	T_INC
|	T_DEC
;*/

UnaryOperation:
	'!'
|	'~'
|	'-'
;

BinaryOperation:
	'+' { $$ = 1; }
|	'-' { $$ = 3; }
|	'*' { $$ = 2; }
|	'/' { $$ = 4; }
|	'%'
|	T_EQ
|	T_NE
|	'<'
|	T_LE
|	'>'
|	T_GE
|	'&'
|	'|'
|	'^'
|	T_AND
|	T_OR
|	T_LSHIFT
|	T_RSHIFT
;

BuiltinStatement:
	ReturnStatement ';'
|	IfStatement
;

ReturnStatement:
	T_RETURN Expression
|	T_RETURN
;

IfStatement:
	T_IF '(' Expression ')' {printf("if body\n"); } IfBody
;

IfBody:
	StatementOrBlock
|	StatementOrBlock ElseStatement
;

ElseStatement:
	T_ELSE StatementOrBlock
;

FunctionCall:
	T_IDENTIFIER '(' ParameterList ')'
|	T_IDENTIFIER '(' ')'
;

ParameterList:
	Expression ',' ParameterList
|	Expression
;

Constant:
	T_STRING
|	T_INTEGER
;
