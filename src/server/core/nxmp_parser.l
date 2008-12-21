%option noyywrap
%option reentrant
%option bison-bridge
%option never-interactive

%x COMMENT
%x STR

whitespace	[[:blank:]\t\r]+
ident		[A-Za-z_\$\@\-][A-Za-z_\$\@\-0-9]*
int			\-?(0x)?[0-9]+
uint		{int}U
real		\-?[0-9]+\.[0-9]+
oid			(\.[0-9]+)+
pos			POS\:[0-9]+
escape		\\[A-Za-z0-9\'\"\\]

%{

#define YY_EXTRA_TYPE	NXMP_Lexer*
#define YY_INPUT(buf, result, maxSize) { result = yyextra->LexerInput(buf, maxSize); }
#define YY_DECL int yylex(YYSTYPE *yylval_param, yyscan_t yyscanner, NXMP_Parser *dummy)

#include "nxcore.h"
#include "nxmp_parser.h"

#ifdef _WIN32
//#pragma warning(disable: 4244)
#endif

#define LexerError(s) yyextra->Error(s)

%}

%%

<INITIAL>"@NXMP"		return T_NXMP;
<INITIAL>"@DEFAULTS"	return T_DEFAULTS;
<INITIAL>"@EVENTS"		return T_EVENTS;
<INITIAL>"@EVENT"		return T_EVENT;
<INITIAL>"@TEMPLATES"	return T_TEMPLATES;
<INITIAL>"@TEMPLATE"	return T_TEMPLATE;
<INITIAL>"@DCI_LIST"	return T_DCI_LIST;
<INITIAL>"@DCI"			return T_DCI;
<INITIAL>"@SCHEDULE"	return T_SCHEDULE;
<INITIAL>"@THRESHOLDS"	return T_THRESHOLDS;
<INITIAL>"@THRESHOLD"	return T_THRESHOLD;
<INITIAL>"@RULES"		return T_RULES;
<INITIAL>"@RULE"		return T_RULE;
<INITIAL>"@SNMP_TRAPS"	return T_SNMP_TRAPS;
<INITIAL>"@TRAP"		return T_TRAP;
<INITIAL>"@PARAMETERS"	return T_PARAMETERS;

<INITIAL>{ident}	{ yylval->valStr = strdup(yytext); return T_IDENTIFIER; }
<INITIAL>{int}		{ yylval->valStr = strdup(yytext); return T_INT; }
<INITIAL>{uint}		{ yylval->valStr = strdup(yytext); return T_UINT; }
<INITIAL>{real}		{ yylval->valStr = strdup(yytext); return T_REAL; }
<INITIAL>{oid}		{ yylval->valStr = strdup(yytext); return T_OID; }
<INITIAL>{pos}		{ yylval->valInt = strtol(&yytext[4], NULL, 10); return T_POS; }

<INITIAL>";"	return ';';
<INITIAL>"="	return '=';
<INITIAL>"{"	return '{';
<INITIAL>"}"	return '}';

<INITIAL>"/*"	{ yyextra->m_nCommentLevel = 1; BEGIN COMMENT; }
<INITIAL>\"		{ yyextra->m_nStrSize = 0; BEGIN STR; }

<INITIAL>"//".*$	;
<INITIAL>"#".*$		;

<INITIAL>\n	yyextra->m_nCurrLine++;
<INITIAL>{whitespace}	;
<INITIAL>.	{
			char szBuffer[128]; 
			sprintf(szBuffer, "Invalid character '%c' (0x%02X)", *yytext, *yytext);
			LexerError(szBuffer);
			return -1;
		}

<STR>{escape}	{		
			if (yyextra->m_nStrSize < MAX_STRING_SIZE)
			{
				switch(yytext[1])
				{
					case 'n':
						yyextra->m_szStr[yyextra->m_nStrSize++] = '\n';
						break;
					case 'r':
						yyextra->m_szStr[yyextra->m_nStrSize++] = '\r';
						break;
					case 'b':
						yyextra->m_szStr[yyextra->m_nStrSize++] = '\b';
						break;
					case 't':
						yyextra->m_szStr[yyextra->m_nStrSize++] = '\t';
						break;
					case '0':
						yyextra->m_szStr[yyextra->m_nStrSize++] = 0;
						break;
					default:
						yyextra->m_szStr[yyextra->m_nStrSize++] = yytext[1];
						break;
				}
			}
			else
			{
				LexerError("String constant is too long");
				return -1;
			}
		}
<STR>\\x[0-9A-Fa-f][0-9A-Fa-f]? {
			char *eptr;
			int nVal;

			nVal = strtol(&yytext[2], &eptr, 16);
			if (*eptr == 0)
			{
				if ((nVal >= 0) && (nVal <= 255))
				{
					yyextra->m_szStr[yyextra->m_nStrSize++] = nVal;
				}
				else
				{
					LexerError("Character code must be in range 0x00 .. 0xFF");
				}
			}
			else
			{
				LexerError("Invalid hex number");
			}
		}
<STR>\"		{
			BEGIN INITIAL;
			yyextra->m_szStr[yyextra->m_nStrSize] = 0;
			yylval->valStr = strdup(yyextra->m_szStr);
			return T_STRING;
		}
<STR>\r		;
<STR>\n		{
			LexerError("Newline in string constant");
			return -1;
		}
<STR>.		{
			if (yyextra->m_nStrSize < MAX_STRING_SIZE)
			{
				yyextra->m_szStr[yyextra->m_nStrSize++] = *yytext;
			}
			else
			{
				LexerError("String constant is too long");
				return -1;
			}
		}

<COMMENT>"/*"	yyextra->m_nCommentLevel++;
<COMMENT>"*/"	{ yyextra->m_nCommentLevel--; if (yyextra->m_nCommentLevel == 0) BEGIN INITIAL; }
<COMMENT>\n	yyextra->m_nCurrLine++;
<COMMENT>.	;
