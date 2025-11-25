%option noyywrap
%option reentrant
%option bison-bridge
%option never-interactive
%option stack

%x COMMENT
%x FEXPR
%x FSTR
%x FSTR2
%x IMPORT
%x STR
%x STR2
%x TXT

nl				\r\n|\r|\n
whitespace	[[:blank:]\t]+
identifier	[A-Za-z_\$][A-Za-z_\$0-9]*
compound_ident	{identifier}(::{identifier})+
wildcard_ident	{identifier}(::{identifier})*::\*
integer		([0-9]+|0x[0-9A-Fa-f]+)
uint32		{integer}U
int64		{integer}L
uint64		{integer}(UL|LU)
real		[0-9]+\.[0-9]+
escape		\\[bnrt0\"\\]

%{

#ifdef _WIN32
#define YY_NO_UNISTD_H
#pragma warning(disable : 4244 4267)
#define _CRT_SECURE_NO_WARNINGS
#endif

#define YY_EXTRA_TYPE	NXSL_Lexer*
#define YY_INPUT(buf, result, maxSize) { result = yyextra->lexerInput(buf, maxSize); }

#define YY_NO_UNPUT

#include "libnxsl.h"
#include "parser.tab.hpp"

static void LexerError(const char *pszMsg, yyscan_t scanner);

// prevent flex debug output on stdout
#undef ECHO
#define ECHO

#define LEXER_ERROR	255

static inline int64_t StringToInt64(const char *s)
{
	if (s[0] == '-')
		return ((s[1] == '0') && (s[2] == 'x')) ? -strtoll(&s[3], nullptr, 16) : strtoll(s, nullptr, 10);
   return ((s[0] == '0') && (s[1] == 'x')) ? strtoll(&s[2], nullptr, 16) : strtoll(s, nullptr, 10);
}

static inline uint64_t StringToUInt64(const char *s)
{
   return ((s[0] == '0') && (s[1] == 'x')) ? strtoull(&s[2], nullptr, 16) : strtoull(s, nullptr, 10);
}

static inline uint32_t StringToUInt32(const char *s)
{
   return ((s[0] == '0') && (s[1] == 'x')) ? strtoul(&s[2], nullptr, 16) : strtoul(s, nullptr, 10);
}

%}

%%

<INITIAL,FEXPR>"@meta"     return T_META;
<INITIAL,FEXPR>"@optional" return T_OPTIONAL;
<INITIAL,FEXPR>"abort"     return T_ABORT;
<INITIAL,FEXPR>"and"       return T_AND;
<INITIAL,FEXPR>"array"     return T_ARRAY;
<INITIAL,FEXPR>"boolean"   return T_TYPE_BOOLEAN;
<INITIAL,FEXPR>"break"     return T_BREAK;
<INITIAL,FEXPR>"case"      return T_CASE;
<INITIAL,FEXPR>"catch"     return T_CATCH;
<INITIAL,FEXPR>"const"     return T_CONST;
<INITIAL,FEXPR>"continue"  return T_CONTINUE;
<INITIAL,FEXPR>"default"   return T_DEFAULT;
<INITIAL,FEXPR>"do"        return T_DO;
<INITIAL,FEXPR>"else"      return T_ELSE;
<INITIAL,FEXPR>"exit"      return T_EXIT;
<INITIAL,FEXPR>"FALSE"     return T_FALSE;
<INITIAL,FEXPR>"false"     return T_FALSE;
<INITIAL,FEXPR>"for"       return T_FOR;
<INITIAL,FEXPR>"foreach"   return T_FOREACH;
<INITIAL,FEXPR>"function"  return T_FUNCTION;
<INITIAL,FEXPR>"global"    return T_GLOBAL;
<INITIAL,FEXPR>"has"       return T_HAS;
<INITIAL,FEXPR>"if"        return T_IF;
<INITIAL,FEXPR>"ilike"     return T_ILIKE;
<INITIAL,FEXPR>"imatch"    return T_IMATCH;
<INITIAL,FEXPR>"import"    { BEGIN IMPORT; return T_IMPORT; }
<INITIAL,FEXPR>"in"        return T_IN;
<INITIAL,FEXPR>"int32"     return T_TYPE_INT32;
<INITIAL,FEXPR>"int64"     return T_TYPE_INT64;
<INITIAL,FEXPR>"like"      return T_LIKE;
<INITIAL,FEXPR>"local"     return T_LOCAL;
<INITIAL,FEXPR>"match"     return T_MATCH;
<INITIAL,FEXPR>"new"       return T_NEW;
<INITIAL,FEXPR>"not"       return T_NOT;
<INITIAL,FEXPR>"NULL"      return T_NULL;
<INITIAL,FEXPR>"null"      return T_NULL;
<INITIAL,FEXPR>"or"        return T_OR;
<INITIAL,FEXPR>"real"      return T_TYPE_REAL;
<INITIAL,FEXPR>"return"    return T_RETURN;
<INITIAL,FEXPR>"select"    return T_SELECT;
<INITIAL,FEXPR>"string"    return T_TYPE_STRING;
<INITIAL,FEXPR>"sub"       return yyextra->isConverterMode() ? (int)T_SUB : (int)T_FUNCTION;
<INITIAL,FEXPR>"switch"    return T_SWITCH;
<INITIAL,FEXPR>"TRUE"      return T_TRUE;
<INITIAL,FEXPR>"true"      return T_TRUE;
<INITIAL,FEXPR>"try"       return T_TRY;
<INITIAL,FEXPR>"uint32"    return T_TYPE_UINT32;
<INITIAL,FEXPR>"uint64"    return T_TYPE_UINT64;
<INITIAL,FEXPR>"use"       return yyextra->isConverterMode() ? (int)T_USE : (int)T_IMPORT;
<INITIAL,FEXPR>"when"      return T_WHEN;
<INITIAL,FEXPR>"while"     return T_WHILE;
<INITIAL,FEXPR>"with"      return T_WITH;

<IMPORT>"as"			return T_AS;
<IMPORT>"from"			return T_FROM;
<IMPORT>","				return ',';
<IMPORT>";"				{ BEGIN INITIAL; return ';'; }

<INITIAL,FEXPR,IMPORT>{identifier}	{ strlcpy(yylval->valIdentifier.v, yytext, MAX_IDENTIFIER_LENGTH); return T_IDENTIFIER; }
<INITIAL,FEXPR,IMPORT>{compound_ident} { strlcpy(yylval->valIdentifier.v, yytext, MAX_IDENTIFIER_LENGTH); return T_COMPOUND_IDENTIFIER; }
<INITIAL,FEXPR,IMPORT>{wildcard_ident} { strlcpy(yylval->valIdentifier.v, yytext, MAX_IDENTIFIER_LENGTH); return T_WILDCARD_IDENTIFIER; }
<INITIAL,FEXPR>{integer}	{
				int64_t nVal = StringToInt64(yytext);
				if ((nVal < -2147483647) || (nVal > 2147483647))
				{
					yylval->valInt64 = nVal;
					return T_INT64;
				}
				else
				{
					yylval->valInt32 = static_cast<int32_t>(nVal);
					return T_INT32;
				}
			}
<INITIAL,FEXPR>{uint32} { yylval->valUInt32 = StringToUInt32(yytext); return T_UINT32; }
<INITIAL,FEXPR>{int64}  { yylval->valInt64 = StringToInt64(yytext); return T_INT64; }
<INITIAL,FEXPR>{uint64} { yylval->valUInt64 = StringToUInt64(yytext); return T_UINT64; }
<INITIAL,FEXPR>{real}   { yylval->valReal = strtod(yytext, nullptr); return T_REAL; }

<INITIAL,FEXPR>"=="	return T_EQ;
<INITIAL,FEXPR>"!="	return T_NE;
<INITIAL,FEXPR>"<="	return T_LE;
<INITIAL,FEXPR>">="	return T_GE;
<INITIAL,FEXPR>"<<"	return T_LSHIFT;
<INITIAL,FEXPR>">>"	return T_RSHIFT;
<INITIAL,FEXPR>"&&"	return T_AND;
<INITIAL,FEXPR>"||"	return T_OR;
<INITIAL,FEXPR>"++"	return T_INC;
<INITIAL,FEXPR>"--"	return T_DEC;
<INITIAL,FEXPR>"?."	return T_SAFEREF;
<INITIAL,FEXPR>"->"	return yyextra->isConverterMode() ? (int)T_ARROW_REF : (int)T_REF;
<INITIAL,FEXPR>"~="	return T_MATCH;
<INITIAL,FEXPR>"+="	return T_ASSIGN_ADD;
<INITIAL,FEXPR>"-="	return T_ASSIGN_SUB;
<INITIAL,FEXPR>"*="	return T_ASSIGN_MUL;
<INITIAL,FEXPR>"/="	return T_ASSIGN_DIV;
<INITIAL,FEXPR>"\\="	return T_ASSIGN_IDIV;
<INITIAL,FEXPR>"%="	return T_ASSIGN_REM;
<INITIAL,FEXPR>"..="	return T_ASSIGN_CONCAT;
<INITIAL,FEXPR>".="	return yyextra->isConverterMode() ? (int)T_V4_ASSIGN_CONCAT : (int)T_ASSIGN_CONCAT;
<INITIAL,FEXPR>"&="	return T_ASSIGN_AND;
<INITIAL,FEXPR>"|="	return T_ASSIGN_OR;
<INITIAL,FEXPR>"^="	return T_ASSIGN_XOR;
<INITIAL,FEXPR>"..."	return T_RANGE;
<INITIAL,FEXPR>".."	return T_CONCAT;
<INITIAL,FEXPR>"."	return yyextra->isConverterMode() ? '.' : T_REF;
<INITIAL,FEXPR>"("	return '(';
<INITIAL,FEXPR>")"	return ')';
<INITIAL,FEXPR>";"	return ';';
<INITIAL,FEXPR>":"	return ':';
<INITIAL,FEXPR>"+"	return '+';
<INITIAL,FEXPR>"-"	return '-';
<INITIAL,FEXPR>"*"	return '*';
<INITIAL,FEXPR>"/"	return '/';
<INITIAL,FEXPR>"%"	return '%';
<INITIAL,FEXPR>"="	return '=';
<INITIAL,FEXPR>"!"	return T_NOT;
<INITIAL,FEXPR>"&"	return '&';
<INITIAL,FEXPR>"|"	return '|';
<INITIAL,FEXPR>"^"	return '^';
<INITIAL,FEXPR>"~"	return '~';
<INITIAL,FEXPR>","	return ',';
<INITIAL,FEXPR>"<"	return '<';
<INITIAL,FEXPR>">"	return '>';
<INITIAL,FEXPR>"?"	return '?';
<INITIAL,FEXPR>"["	return '[';
<INITIAL,FEXPR>"]"	return ']';
<INITIAL,FEXPR>"@"	return '@';
<INITIAL,FEXPR>"#"	return '#';
<INITIAL,FEXPR>"\\"	return T_IDIV;

<INITIAL>"{"         return '{';
<INITIAL>"}"         return '}';

<FEXPR>"}"           { yyextra->m_stringSize = 0; BEGIN FSTR; }
<FEXPR>\'            { yyextra->m_stringSize = 0; BEGIN FSTR2; }
<FEXPR>[Ff]\"        {
         LexerError("Format strings cannot be nested", yyscanner);
         return LEXER_ERROR;
      }
<FEXPR>\"            {
         LexerError("Double-quoted strings within format string are forbidden", yyscanner);
         return LEXER_ERROR;
      }

<INITIAL,IMPORT>"/*" { yyextra->m_commentLevel = 1; yy_push_state(COMMENT, yyscanner); }
<INITIAL,IMPORT>"\"\"\"" { yyextra->m_text.clear(); yy_push_state(TXT, yyscanner); }
<INITIAL>[Ff]\"      { yyextra->m_stringSize = 0; BEGIN FSTR; return T_FSTRING_BEGIN; }
<INITIAL>\"          { yyextra->m_stringSize = 0; BEGIN STR; }
<INITIAL>\'          { yyextra->m_stringSize = 0; BEGIN STR2; }

<IMPORT>[Ff]\"       { LexerError("Unexpected string constant in import section", yyscanner); return LEXER_ERROR; }
<IMPORT>\"           { LexerError("Unexpected string constant in import section", yyscanner); return LEXER_ERROR; }
<IMPORT>\'           { LexerError("Unexpected string constant in import section", yyscanner); return LEXER_ERROR; }

<INITIAL,IMPORT>"//".*	{ if (yyextra->m_converterMode) return ' '; }

<INITIAL,IMPORT>{nl}				yyextra->m_currLine++;
<INITIAL,IMPORT>{whitespace}	;
<INITIAL,IMPORT>.	{
			char szBuffer[128];
#if defined(_WIN32) && (_MSC_VER >= 1400)
			_snprintf_s(szBuffer, 128, _TRUNCATE, "Invalid character '%c' (0x%02X)", *yytext, *yytext);
#else
			snprintf(szBuffer, 128, "Invalid character '%c' (0x%02X)", *yytext, *yytext);
#endif
			LexerError(szBuffer, yyscanner);
			return LEXER_ERROR;
		}

<STR,FSTR>{escape}	{
			if (yyextra->m_stringSize < MAX_STRING_SIZE)
			{
				switch(yytext[1])
				{
					case '"':
						yyextra->m_string[yyextra->m_stringSize++] = '"';
						break;
					case 'n':
						yyextra->m_string[yyextra->m_stringSize++] = '\n';
						break;
					case 'r':
						yyextra->m_string[yyextra->m_stringSize++] = '\r';
						break;
					case 'b':
						yyextra->m_string[yyextra->m_stringSize++] = '\b';
						break;
					case 't':
						yyextra->m_string[yyextra->m_stringSize++] = '\t';
						break;
					case '0':
						yyextra->m_string[yyextra->m_stringSize++] = 0;
						break;
					default:
						yyextra->m_string[yyextra->m_stringSize++] = yytext[1];
						break;
				}
			}
			else
			{
				LexerError("String constant is too long", yyscanner);
				return LEXER_ERROR;
			}
		}
<STR,FSTR>\\x[0-9A-Fa-f][0-9A-Fa-f]? {
			char *eptr;
			int nVal;

			nVal = strtol(&yytext[2], &eptr, 16);
			if (*eptr == 0)
			{
				if ((nVal >= 0) && (nVal <= 255))
				{
					yyextra->m_string[yyextra->m_stringSize++] = nVal;
				}
				else
				{
					LexerError("Character code must be in range 0x00 .. 0xFF", yyscanner);
					return LEXER_ERROR;
				}
			}
			else
			{
				LexerError("Invalid hex number", yyscanner);
				return LEXER_ERROR;
			}
		}
<STR,FSTR>\\u[0-9A-Fa-f]{4} {
			char *eptr;
			UCS2CHAR cp = (UCS2CHAR)strtoul(&yytext[2], &eptr, 16);
			if (*eptr == 0)
			{
				char utf8cp[4];
				size_t bytes = ucs2_to_utf8(&cp, 1, utf8cp, 4);
				if (yyextra->m_stringSize + bytes <= MAX_STRING_SIZE)
				{
					memcpy(&yyextra->m_string[yyextra->m_stringSize], utf8cp, bytes);
					yyextra->m_stringSize += bytes;
				}
				else
				{
					LexerError("String constant is too long", yyscanner);
					return LEXER_ERROR;
				}
			}
			else
			{
				LexerError("Invalid hex number", yyscanner);
				return LEXER_ERROR;
			}
		}
<STR>\"		{
			BEGIN INITIAL;
			yyextra->m_string[yyextra->m_stringSize] = 0;
			yylval->valStr = MemCopyStringA(yyextra->m_string);
			return T_STRING;
		}
<FSTR>"\\{"		{
			if (yyextra->m_stringSize < MAX_STRING_SIZE)
			{
				yyextra->m_string[yyextra->m_stringSize++] = '{';
			}
			else
			{
				LexerError("String constant is too long", yyscanner);
				return LEXER_ERROR;
			}

      }
<FSTR>"{"		{
			BEGIN FEXPR;
			yyextra->m_string[yyextra->m_stringSize] = 0;
			yylval->valStr = MemCopyStringA(yyextra->m_string);
			return T_FSTRING;
		}
<FSTR>\"		{
			BEGIN INITIAL;
			yyextra->m_string[yyextra->m_stringSize] = 0;
			yylval->valStr = MemCopyStringA(yyextra->m_string);
			return T_FSTRING_END;
		}
<STR,FSTR>{nl}		{
			LexerError("Newline in string constant", yyscanner);
			return LEXER_ERROR;
		}
<STR,FSTR>.		{
			if (yyextra->m_stringSize < MAX_STRING_SIZE)
			{
				yyextra->m_string[yyextra->m_stringSize++] = *yytext;
			}
			else
			{
				LexerError("String constant is too long", yyscanner);
				return LEXER_ERROR;
			}
		}

<STR2>\'		{
			BEGIN INITIAL;
			yyextra->m_string[yyextra->m_stringSize] = 0;
			yylval->valStr = MemCopyStringA(yyextra->m_string);
			return T_STRING;
		}
<FSTR2>\'	{
			BEGIN FEXPR;
			yyextra->m_string[yyextra->m_stringSize] = 0;
			yylval->valStr = MemCopyStringA(yyextra->m_string);
			return T_STRING;
		}
<STR2,FSTR2>{nl}	{
			LexerError("Newline in string constant", yyscanner);
			return LEXER_ERROR;
		}
<STR2,FSTR2>.		{
			if (yyextra->m_stringSize < MAX_STRING_SIZE)
			{
				yyextra->m_string[yyextra->m_stringSize++] = *yytext;
			}
			else
			{
				LexerError("String constant is too long", yyscanner);
				return LEXER_ERROR;
			}
		}

<TXT>"\"\"\""		{
			yy_pop_state(yyscanner);
			yyextra->m_text.write(static_cast<char>(0));
			yylval->valStr = MemCopyStringA(reinterpret_cast<const char*>(yyextra->m_text.buffer()));
			return T_STRING;
		}
<TXT>{nl}	{ yyextra->m_text.write('\n'); yyextra->m_currLine++; }
<TXT><<EOF>> {
			LexerError("Unterminated text block", yyscanner);
			return LEXER_ERROR;
		}
<TXT>.		{ yyextra->m_text.write(*yytext); }

<COMMENT>"/*"	yyextra->m_commentLevel++;
<COMMENT>"*/"	{
			yyextra->m_commentLevel--;
			if (yyextra->m_commentLevel == 0)
			{
				yy_pop_state(yyscanner);
				if (yyextra->m_converterMode)
					return ' ';
			}
		}
<COMMENT>{nl}	yyextra->m_currLine++;
<COMMENT><<EOF>> {
			LexerError("Unterminated comment", yyscanner);
			return LEXER_ERROR;
		}
<COMMENT>.	;

%%

static void LexerError(const char *message, yyscan_t scanner)
{
   yyget_extra(scanner)->error(message);
}