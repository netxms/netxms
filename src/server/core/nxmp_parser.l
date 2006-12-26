%option noyywrap
%option yyclass="NXMP_Lexer"

%x COMMENT
%x STR

whitespace	[[:blank:]\t\r]+
ident		[A-Za-z_\$][A-Za-z_\$0-9]*
int			\-?(0x)?[0-9]+
uint		{int}U
real		\-?[0-9]+\.[0-9]+
oid			(\.[0-9]+)+
pos			POS\:[0-9]+
escape		\\[bnrt0\"]

%{

// dirtiest hack i've ever done
#undef yyFlexLexer
#include "nxcore.h"
#include "nxmp_parser.h"

#ifdef _WIN32
#pragma warning(disable: 4244)
#define isatty __declspec(dllimport) _isatty
#endif

%}

%%

<INITIAL>"NXMP"			return T_NXMP;
<INITIAL>"DEFAULTS"		return T_DEFAULTS;
<INITIAL>"EVENTS"		return T_EVENTS;
<INITIAL>"EVENT"		return T_EVENT;
<INITIAL>"TEMPLATES"	return T_TEMPLATES;
<INITIAL>"TEMPLATE"		return T_TEMPLATE;
<INITIAL>"DCI_LIST"		return T_DCI_LIST;
<INITIAL>"DCI"			return T_DCI;
<INITIAL>"THRESHOLDS"	return T_THRESHOLDS;
<INITIAL>"THRESHOLD"	return T_THRESHOLD;
<INITIAL>"RULES"		return T_RULES;
<INITIAL>"RULE"			return T_RULE;
<INITIAL>"SNMP_TRAPS"	return T_SNMP_TRAPS;
<INITIAL>"TRAP"			return T_TRAP;
<INITIAL>"PARAMETERS"	return T_PARAMETERS;

<INITIAL>{ident}	{ m_plval->valStr = strdup(yytext); return T_IDENTIFIER; }
<INITIAL>{int}		{ m_plval->valStr = strdup(yytext); return T_INT; }
<INITIAL>{uint}		{ m_plval->valStr = strdup(yytext); return T_UINT; }
<INITIAL>{real}		{ m_plval->valStr = strdup(yytext); return T_REAL; }
<INITIAL>{oid}		{ m_plval->valStr = strdup(yytext); return T_OID; }
<INITIAL>{pos}		{ m_plval->valInt = strtol(yytext, NULL, 10); return T_POS; }

<INITIAL>";"	return ';';
<INITIAL>"="	return '=';
<INITIAL>"{"	return '{';
<INITIAL>"}"	return '}';

<INITIAL>"/*"	{ m_nCommentLevel = 1; BEGIN COMMENT; }
<INITIAL>\"		{ m_nStrSize = 0; BEGIN STR; }

<INITIAL>"//".*$	;
<INITIAL>"#".*$		;

<INITIAL>\n	m_nCurrLine++;
<INITIAL>{whitespace}	;
<INITIAL>.	{
			char szBuffer[128]; 
			sprintf(szBuffer, "Invalid character '%c' (0x%02X)", *yytext, *yytext);
			LexerError(szBuffer);
			return -1;
		}

<STR>{escape}	{		
			if (m_nStrSize < MAX_STRING_SIZE)
			{
				switch(yytext[1])
				{
					case '"':
						m_szStr[m_nStrSize++] = '"';
						break;
					case 'n':
						m_szStr[m_nStrSize++] = '\n';
						break;
					case 'r':
						m_szStr[m_nStrSize++] = '\r';
						break;
					case 'b':
						m_szStr[m_nStrSize++] = '\b';
						break;
					case 't':
						m_szStr[m_nStrSize++] = '\t';
						break;
					case '0':
						m_szStr[m_nStrSize++] = 0;
						break;
					default:
						m_szStr[m_nStrSize++] = yytext[1];
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
					m_szStr[m_nStrSize++] = nVal;
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
			m_szStr[m_nStrSize] = 0;
			m_plval->valStr = strdup(m_szStr);
			return T_STRING;
		}
<STR>\r		;
<STR>\n		{
			LexerError("Newline in string constant");
			return -1;
		}
<STR>.		{
			if (m_nStrSize < MAX_STRING_SIZE)
			{
				m_szStr[m_nStrSize++] = *yytext;
			}
			else
			{
				LexerError("String constant is too long");
				return -1;
			}
		}

<COMMENT>"/*"	m_nCommentLevel++;
<COMMENT>"*/"	{ m_nCommentLevel--; if (m_nCommentLevel == 0) BEGIN INITIAL; }
<COMMENT>\n	m_nCurrLine++;
<COMMENT>.	;
