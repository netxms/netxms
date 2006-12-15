%{

#pragma warning(disable : 4065 4102)

#define YYERROR_VERBOSE
#define YYINCLUDED_STDLIB_H
#define YYDEBUG			1

#include "nxcore.h"
#include "nxmp_parser.h"

void yyerror(NXMP_Lexer *pLexer, NXMP_Parser *pParser, NXMP_Data *pData, char *pszText);
int yylex(YYSTYPE *lvalp, NXMP_Lexer *pLexer);

%}

%expect		1
%pure-parser
%parse-param	{NXMP_Lexer *pLexer}
%lex-param		{NXMP_Lexer *pLexer}
%parse-param	{NXMP_Parser *pParser}
%parse-param	{NXMP_Data *pData}

%union
{
	LONG valInt;
	DWORD valUInt;
	char *valStr;
	double valReal;
}

%token T_NXMP
%token T_DEFAULTS
%token T_EVENTS
%token T_EVENT
%token T_TEMPLATES
%token T_TEMPLATE
%token T_DCI_LIST
%token T_DCI
%token T_THRESHOLDS
%token T_THRESHOLD
%token T_RULES
%token T_RULE

%token <valStr> T_IDENTIFIER
%token <valStr> T_STRING
%token <valInt> T_INT
%token <valUInt> T_UINT
%token <valReal> T_REAL

%start ManagementPack


%%

ManagementPack:
	SectionList
;

SectionList:
	Section SectionList
|	Section
;

Section:
	T_NXMP '{' NXMPBody '}'
|	T_EVENTS '{' EventList '}'
|	T_TEMPLATES '{' TemplateList '}'
|	T_RULES '{' RulesList '}'
;

NXMPBody:
	VariableOrDefaults NXMPBody
|	VariableOrDefaults
;

VariableOrDefaults:
	Variable
|	Defaults
;

Defaults:
	T_DEFAULTS '{' VariableList '}'
;

EventList:
	Event EventList
|	Event
;

Event:
	T_EVENT T_IDENTIFIER { pData->NewEvent($2); } '{' VariableList '}' { pData->CloseEvent(); }
;

TemplateList:
	Template TemplateList
|	Template
;

Template:
	T_TEMPLATE T_IDENTIFIER '{' TemplateBody '}'
;

TemplateBody:
	TemplateElement TemplateBody
|	TemplateElement
;

TemplateElement:
	T_DCI_LIST '{' DCIList '}'
;

DCIList:
	DCI DCIList
|	DCI
;

DCI:
	T_DCI '{' DCIBody '}'
;

DCIBody:
	DCIElement DCIBody
|	DCIElement
;

DCIElement:
	Variable
|	Thresholds
;

Thresholds:
	T_THRESHOLDS '{' ThresholdsList '}'
;

ThresholdsList:
	Threshold ThresholdsList
|
;

Threshold:
	VariableList
;

RulesList:
	Rule RulesList
|
;

Rule:
	T_RULE '{' RuleBody '}'
;

RuleBody:
	VariableList
;

VariableList:
	Variable VariableList
|	Variable
;

Variable:
	T_IDENTIFIER '=' Value ';'
{
}
;

Value:
	T_IDENTIFIER
|	T_STRING
|	T_REAL
|	T_INT
|	T_UINT
;
