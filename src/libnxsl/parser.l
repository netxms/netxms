%option noyywrap
%option reentrant
%option bison-bridge
%option never-interactive

%x COMMENT
%x STR

whitespace	[[:blank:]\t\r]+
identifier	[A-Za-z_\$][A-Za-z_\$0-9]*
compound_ident	{identifier}(::{identifier})+
integer		([0-9]+|0x[0-9A-Fa-f]+)
uint32		{integer}U
int64		{integer}L
uint64		{integer}(UL|LU)
real		[0-9]+\.[0-9]+
escape		\\[bnrt0\"\\]

%{

#ifdef _WIN32
#pragma warning(disable : 4244 4267)
#endif

#define YY_EXTRA_TYPE	NXSL_Lexer*
#define YY_INPUT(buf, result, maxSize) { result = yyextra->lexerInput(buf, maxSize); }

#include "libnxsl.h"
#include "parser.tab.hpp"

static void LexerError(const char *pszMsg, yyscan_t scanner);

%}

%%

<INITIAL>"abort"    return T_ABORT;
<INITIAL>"array"    return T_ARRAY;
<INITIAL>"break"    return T_BREAK;
<INITIAL>"case"     return T_CASE;
<INITIAL>"const"    return T_CONST;
<INITIAL>"continue" return T_CONTINUE;
<INITIAL>"default"  return T_DEFAULT;
<INITIAL>"do"       return T_DO;
<INITIAL>"else"     return T_ELSE;
<INITIAL>"exit"     return T_EXIT;
<INITIAL>"FALSE"    return T_FALSE;
<INITIAL>"false"    return T_FALSE;
<INITIAL>"for"      return T_FOR;
<INITIAL>"foreach"  return T_FOREACH;
<INITIAL>"global"   return T_GLOBAL;
<INITIAL>"if"       return T_IF;
<INITIAL>"ilike"    return T_ILIKE;
<INITIAL>"imatch"   return T_IMATCH;
<INITIAL>"int32"    return T_TYPE_INT32;
<INITIAL>"int64"    return T_TYPE_INT64;
<INITIAL>"like"     return T_LIKE;
<INITIAL>"match"    return T_MATCH;
<INITIAL>"NULL"     return T_NULL;
<INITIAL>"null"     return T_NULL;
<INITIAL>"print"    return T_PRINT;
<INITIAL>"println"  return T_PRINTLN;
<INITIAL>"real"     return T_TYPE_REAL;
<INITIAL>"return"   return T_RETURN;
<INITIAL>"string"   return T_TYPE_STRING;
<INITIAL>"sub"      return T_SUB;
<INITIAL>"switch"   return T_SWITCH;
<INITIAL>"TRUE"     return T_TRUE;
<INITIAL>"true"     return T_TRUE;
<INITIAL>"uint32"   return T_TYPE_UINT32;
<INITIAL>"uint64"   return T_TYPE_UINT64;
<INITIAL>"use"      return T_USE;
<INITIAL>"while"    return T_WHILE;

<INITIAL>{identifier}	{ yylval->valStr = strdup(yytext); return T_IDENTIFIER; }
<INITIAL>{compound_ident} { yylval->valStr = strdup(yytext); return T_COMPOUND_IDENTIFIER; }
<INITIAL>{integer}	{
				INT64 nVal;

				nVal = strtoll(yytext, NULL, 0);
				if ((nVal < -2147483647) || (nVal > 2147483647))
				{
					yylval->valInt64 = nVal;
					return T_INT64;
				}
				else
				{
					yylval->valInt32 = (LONG)nVal;
					return T_INT32;
				}
			}
<INITIAL>{uint32}	{ yylval->valUInt32 = strtoul(yytext, NULL, 0); return T_UINT32; }
<INITIAL>{int64}	{ yylval->valInt64 = strtoll(yytext, NULL, 0); return T_INT64; }
<INITIAL>{uint64}	{ yylval->valUInt64 = strtoull(yytext, NULL, 0); return T_UINT64; }
<INITIAL>{real}		{ yylval->valReal = strtod(yytext, NULL); return T_REAL; }

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
<INITIAL>"->"	return T_REF;
<INITIAL>"~="	return T_MATCH;
<INITIAL>"+="	return T_ASSIGN_ADD;
<INITIAL>"-="	return T_ASSIGN_SUB;
<INITIAL>"*="	return T_ASSIGN_MUL;
<INITIAL>"/="	return T_ASSIGN_DIV;
<INITIAL>"%="	return T_ASSIGN_REM;
<INITIAL>".="	return T_ASSIGN_CONCAT;
<INITIAL>"&="	return T_ASSIGN_AND;
<INITIAL>"|="	return T_ASSIGN_OR;
<INITIAL>"^="	return T_ASSIGN_XOR;
<INITIAL>"."	return '.';
<INITIAL>"{"	return '{';
<INITIAL>"}"	return '}';
<INITIAL>"("	return '(';
<INITIAL>")"	return ')';
<INITIAL>";"	return ';';
<INITIAL>":"	return ':';
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
<INITIAL>"?"	return '?';
<INITIAL>"["	return '[';
<INITIAL>"]"	return ']';
<INITIAL>"@"	return '@';

<INITIAL>"/*"	{ yyextra->m_nCommentLevel = 1; BEGIN COMMENT; }
<INITIAL>\"	{ yyextra->m_nStrSize = 0; BEGIN STR; }

<INITIAL>"//".*	;

<INITIAL>\n	yyextra->m_nCurrLine++;
<INITIAL>{whitespace}	;
<INITIAL>.	{
			char szBuffer[128];
#if defined(_WIN32) && (_MSC_VER >= 1400)
			_snprintf_s(szBuffer, 128, _TRUNCATE, "Invalid character '%c' (0x%02X)", *yytext, *yytext);
#else
			snprintf(szBuffer, 128, "Invalid character '%c' (0x%02X)", *yytext, *yytext);
#endif
			LexerError(szBuffer, yyscanner);
			return -1;
		}

<STR>{escape}	{		
			if (yyextra->m_nStrSize < MAX_STRING_SIZE)
			{
				switch(yytext[1])
				{
					case '"':
						yyextra->m_szStr[yyextra->m_nStrSize++] = '"';
						break;
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
				LexerError("String constant is too long", yyscanner);
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
					LexerError("Character code must be in range 0x00 .. 0xFF", yyscanner);
				}
			}
			else
			{
				LexerError("Invalid hex number", yyscanner);
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
			LexerError("Newline in string constant", yyscanner);
			return -1;
		}
<STR>.		{
			if (yyextra->m_nStrSize < MAX_STRING_SIZE)
			{
				yyextra->m_szStr[yyextra->m_nStrSize++] = *yytext;
			}
			else
			{
				LexerError("String constant is too long", yyscanner);
				return -1;
			}
		}

<COMMENT>"/*"	yyextra->m_nCommentLevel++;
<COMMENT>"*/"	{ yyextra->m_nCommentLevel--; if (yyextra->m_nCommentLevel == 0) BEGIN INITIAL; }
<COMMENT>\n	yyextra->m_nCurrLine++;
<COMMENT>.	;

%%

static void LexerError(const char *pszMsg, yyscan_t scanner)
{
   yyget_extra(scanner)->error(pszMsg);
}
