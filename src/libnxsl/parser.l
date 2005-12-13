%option noyywrap
%option yyclass="NXSL_Lexer"

%x COMMENT
%x STR

whitespace	[[:blank:]\t\r]+
identifier	[A-Za-z_][A-Za-z_0-9]*
integer		(0x)?[0-9]+
escape		\\[bnrt\"]

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
<INITIAL>{integer}	{ m_plval->valInt = strtoul(yytext, NULL, 0); return T_INTEGER; }

<INITIAL>"=="	return T_EQ;
<INITIAL>"!="	return T_NE;
<INITIAL>"<="	return T_LE;
<INITIAL>">="	return T_GE;
<INITIAL>"<<"	return T_LSHIFT;
<INITIAL>">>"	return T_RSHIFT;
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
<INITIAL>"^"	return '^';
<INITIAL>"~"	return '~';
<INITIAL>","	return ',';
<INITIAL>"<"	return '<';
<INITIAL>">"	return '>';

<INITIAL>"/*"	{ m_nCommentLevel = 1; BEGIN COMMENT; }
<INITIAL>\"	{ m_nStrSize = 0; BEGIN STR; }

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
