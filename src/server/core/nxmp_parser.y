%{

#pragma warning(disable : 4065 4102)

#define YYERROR_VERBOSE
#define YYINCLUDED_STDLIB_H
#define YYDEBUG			1

#include "nxcore.h"
#include "nxmp_parser.h"

void yyerror(yyscan_t scanner, NXMP_Lexer *pLexer, NXMP_Parser *pParser, NXMP_Data *pData, const char *pszText);
int yylex(YYSTYPE *lvalp, yyscan_t scanner);

%}

%pure-parser
%lex-param		{yyscan_t scanner}
%parse-param	{yyscan_t scanner}
%parse-param	{NXMP_Lexer *pLexer}
%parse-param	{NXMP_Parser *pParser}
%parse-param	{NXMP_Data *pData}

%union
{
	char *valStr;
	int valInt;
}

%token T_NXMP
%token T_DEFAULTS
%token T_EVENTS
%token T_EVENT
%token T_TEMPLATES
%token T_TEMPLATE
%token T_DCI_LIST
%token T_DCI
%token T_SCHEDULE
%token T_THRESHOLDS
%token T_THRESHOLD
%token T_RULES
%token T_RULE
%token T_SNMP_TRAPS
%token T_TRAP
%token T_PARAMETERS

%token <valStr> T_IDENTIFIER
%token <valStr> T_OID
%token <valInt> T_POS
%token <valStr> T_STRING
%token <valStr> T_INT
%token <valStr> T_UINT
%token <valStr> T_REAL

%type <valStr> Value

%start ManagementPack


%%

ManagementPack:
	SectionList
;

SectionList:
	Section SectionList
|
;

Section:
	T_NXMP '{' NXMPBody '}'
|	T_EVENTS '{' EventList '}'
|	T_TEMPLATES '{' TemplateList '}'
|	T_SNMP_TRAPS '{' TrapList '}'
|	T_RULES '{' RulesList '}'
;

NXMPBody:
	VariableOrDefaults NXMPBody
|
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
|
;

Event:
	T_EVENT T_IDENTIFIER { pData->NewEvent($2); free($2); } '{' VariableList '}' { pData->CloseEvent(); }
;

TemplateList:
	Template TemplateList
|
;

Template:
	T_TEMPLATE T_IDENTIFIER { pData->NewTemplate($2); free($2); } '{' TemplateBody '}' { pData->CloseTemplate(); }
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
|
;

DCI:
	T_DCI { pData->NewDCI(); } '{' DCIBody '}' { pData->CloseDCI(); }
;

DCIBody:
	DCIElement DCIBody
|	DCIElement
;

DCIElement:
	Variable
|	Thresholds
|	Schedule
;

Thresholds:
	T_THRESHOLDS '{' ThresholdsList '}'
;

ThresholdsList:
	Threshold ThresholdsList
|
;

Threshold:
	T_THRESHOLD { pData->NewThreshold(); } '{' VariableList '}' { pData->CloseThreshold(); }
;

Schedule:
	T_SCHEDULE '{' ScheduleList '}'
;

ScheduleList:
	T_STRING ';' { pData->AddDCISchedule($1); free($1); } ScheduleList
|
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

TrapList:
	Trap TrapList
|
;

Trap:
	T_TRAP T_OID { pData->NewTrap($2); free($2); } '{' TrapBody '}' { pData->CloseTrap(); }
;

TrapBody:
	TrapElement TrapBody
|
;

TrapElement:
	Variable
|	TrapParameters
;

TrapParameters:
	T_PARAMETERS '{' TrapParamList '}'
;

TrapParamList:
	TrapParam TrapParamList
|
;

TrapParam:
	T_OID '=' Value ';'
{
	pData->AddTrapParam($1, 0, $3);
	free($1);
	free($3);
}
|	T_POS '=' Value ';'
{
	pData->AddTrapParam(NULL, $1, $3);
	free($3);
}
;

VariableList:
	Variable VariableList
|
;

Variable:
	T_IDENTIFIER '=' Value ';'
{
	pData->ParseVariable($1, $3);
	free($1);
	free($3);
}
;

Value:
	T_IDENTIFIER
|	T_STRING
|	T_REAL
|	T_INT
|	T_UINT
;
