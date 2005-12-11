%option noyywrap
%option yyclass="NXSL_Lexer"

%x COMMENT

whitespace	[[:blank:]\t]+
identifier	[A-Za-z_][A-Za-z_0-9]*
integer		[0-9]+

%{

#include "libnxsl.h"
#include "parser.tab.h"

%}

%%

<INITIAL>"else"		return T_ELSE;
<INITIAL>"if"		return T_IF;
<INITIAL>"return"	return T_RETURN;
<INITIAL>"sub"		return T_SUB;

<INITIAL>{identifier}	{ return T_IDENTIFIER; }
<INITIAL>{integer}	{ return T_INTEGER; }

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
<INITIAL>\n	m_nCurrLine++;
<INITIAL>{whitespace}	;

<INITIAL>"/*"	{ m_nCommentLevel = 1; BEGIN COMMENT; }
<COMMENT>"/*"	m_nCommentLevel++;
<COMMENT>"*/"	{ m_nCommentLevel--; if (m_nCommentLevel == 0) BEGIN INITIAL; }
<COMMENT>\n	m_nCurrLine++;
<COMMENT>.	;
