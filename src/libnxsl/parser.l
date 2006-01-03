%option noyywrap
%option yyclass="NXSL_Lexer"

%x COMMENT
%x STR

whitespace	[[:blank:]\t\r]+
identifier	[A-Za-z_\$][A-Za-z_\$0-9]*
integer		\-?(0x)?[0-9]+
uint32		{integer}U
int64		{integer}L
uint64		{integer}(UL|LU)
real		\-?[0-9]+\.[0-9]+
escape		\\[bnrt0\"]

%{

#include "libnxsl.h"
#include "parser.tab.hpp"

#ifdef _WIN32
#define isatty __declspec(dllimport) _isatty
#endif

%}

%%

<INITIAL>"else"		return T_ELSE;
<INITIAL>"exit"		return T_EXIT;
<INITIAL>"if"		return T_IF;
<INITIAL>"NULL"		return T_NULL;
<INITIAL>"print"	return T_PRINT;
<INITIAL>"return"	return T_RETURN;
<INITIAL>"sub"		return T_SUB;

<INITIAL>{identifier}	{ m_plval->valStr = strdup(yytext); return T_IDENTIFIER; }
<INITIAL>{integer}	{
				INT64 nVal;

				nVal = strtoll(yytext, NULL, 0);
				if ((nVal < -2147483647) || (nVal > 2147483647))
				{
					m_plval->valInt64 = nVal;
					return T_INT64;
				}
				else
				{
					m_plval->valInt32 = (LONG)nVal;
					return T_INT32;
				}
			}
<INITIAL>{uint32}	{ m_plval->valUInt32 = strtoul(yytext, NULL, 0); return T_UINT32; }
<INITIAL>{int64}	{ m_plval->valInt64 = strtoll(yytext, NULL, 0); return T_INT64; }
<INITIAL>{uint64}	{ m_plval->valUInt64 = strtoull(yytext, NULL, 0); return T_UINT64; }
<INITIAL>{real}		{ m_plval->valReal = strtod(yytext, NULL); return T_REAL; }

<INITIAL>"=="	return T_EQ;
<INITIAL>"!="	return T_NE;
<INITIAL>"<="	return T_LE;
<INITIAL>">="	return T_GE;
<INITIAL>"<<"	return T_LSHIFT;
<INITIAL>">>"	return T_RSHIFT;
<INITIAL>"&&"	return T_AND;
<INITIAL>"||"	return T_OR;
<INITIAL>"++"	return T_INC;
<INITIAL>"--"	return T_DEC;
<INITIAL>"."	return '.';
<INITIAL>"{"	return '{';
<INITIAL>"}"	return '}';
<INITIAL>"("	return '(';
<INITIAL>")"	return ')';
<INITIAL>";"	return ';';
<INITIAL>"+"	return '+';
<INITIAL>"-"	return '-';
<INITIAL>"*"	return '*';
<INITIAL>"/"	return '/';
<INITIAL>"%"	return '%';
<INITIAL>"="	return '=';
<INITIAL>"!"	return '!';
<INITIAL>"&"	return '&';
<INITIAL>"|"	return '|';
<INITIAL>"^"	return '^';
<INITIAL>"~"	return '~';
<INITIAL>","	return ',';
<INITIAL>"<"	return '<';
<INITIAL>">"	return '>';

<INITIAL>"/*"	{ m_nCommentLevel = 1; BEGIN COMMENT; }
<INITIAL>\"	{ m_nStrSize = 0; BEGIN STR; }

<INITIAL>"//".*$	;

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
