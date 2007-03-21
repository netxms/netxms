/*
 * lexical.cxx
 *
 * Lexical Analyzer
 *
 * libtpt 1.30
 *
 */

/*
 * Copyright (C) 2002-2004 Isaac W. Foraker (isaac@tazthecat.net)
 * All Rights Reserved
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of the Author nor the names of its contributors
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/*
 * The lexical analyzer is responsible for converting input
 * text to tokens.
 *
 * Using the ChrSet<> and Token<> templates, it should be easy to
 * upgrade the lexical analyzer to handle wchar_t (wide character) types
 * as well as the basic char type.
 *
 */

#include "libtpt_conf.h"
#include "libtpt_lexical.h"
#include "libtpt/buffer.h"
#include <cstring>
#include <iostream>
#include <cctype>

namespace TPT {


unsigned Lex::getlineno() const
{
	return lineno_;
}

void Lex::setlineno(unsigned newline)
{
	lineno_ = newline;
}


Token<> Lex::getloosetoken()
{
	char c;

	if (!(c = safeget()))
	{
		Token<> t(column_, lineno_);
		t.type = token_eof;	
		return t;
	}

	// Check for #! on the first line, and ignore
	if ((lineno_ == 1) && (column_ == 2) && (c == '#'))
	{
		c = safeget();
		if (c == '!')
		{
			Token<> t(column_, lineno_);
			// Ignore this line
			while ( (c = safeget()) )
			{
				if ((c == '\n') || (c == '\n'))
				{
					getreturn(c, t.value);
					newline();
					break;
				}
			}
			t.type = token_comment;
			t.value.erase();
			return t;
		}
		else if (c)
			safeunget();
		c = '#';
	}

	switch (c) {
	// For simplicity, have the special token reader process these
	// symbols to avoid duplicate code.
	case ' ':
	case '\t':
	case '\n':
	case '\r':
	case '\\':	// escape character
	case '@':	// keyword, macro, or comment
	case '$':	// variable name
	case '{':	// start block
	case '}':	// end block
		safeunget();
		return getspecialtoken();
	default:
		break;
	}
	Token<> t(column_, lineno_);
	t.type = token_text;
	t.value = c;
	buildrawtext(t.value);

	return t;
}


Token<> Lex::getstricttoken()
{
	Token<> t(getspecialtoken());
	// skip white-spaces
	while ((t.type == token_whitespace) || (t.type == token_comment))
		t = getspecialtoken();

	return t;
}


Token<> Lex::getspecialtoken()
{
	Token<> t(column_, lineno_);
	t.type = token_error;
	unsigned char c;
	unsigned col = column_;

	if (!(c = safeget()))
	{
		t.type = token_eof;	
		return t;
	}

	// Check for #! on the first line, and ignore
	if ((lineno_ == 1) && (col == 1) && (c == '#'))
	{
		c = safeget();
		if (c == '!')
		{
			// Ignore this line
			while ( (c = safeget()) )
			{
				if ((c == '\r') || (c == '\n'))
				{
					getreturn(c, t.value);
					newline();
					break;
				}
			}
			t.type = token_comment;
			t.value.erase();
			return t;
		}
		else if (c)
			safeunget();
		c = '#';
	}

	t.value = c;

	switch (c) {
	case '{':
		t.type = token_openbrace;
		handlebraceignore(t);
		break;
	case '}':
		t.type = token_closebrace;
		handlebraceignore(t);
		break;
	case '(':
		t.type = token_openparen;
		break;
	case ')':
		t.type = token_closeparen;
		break;
	case ',':
		t.type = token_comma;
		break;
	case '+':
	case '-':
	case '*':
	case '/':
	case '%':
		t.type = token_operator;
		break;
	case '|':
	case '&':
	case '^':
		c = safeget();
		if (t.value[0] == c)
		{
			// as in ||, &&, ^^
			t.type = token_operator;
			t.value+= c;
		}
		else if (c) // error, no support for binary ops
			safeunget();
		break;
	// Handle whitespace
	case ' ':
	case '\t':
		t.type = token_whitespace;
		if ( (c = safeget()) )
		{
			while (std::isspace(c))
			{
				t.value+= c;
				c = safeget();
			}
			if (ignoreindent_ && col == 1)
				t.value.erase();
			if (c == '@')
			{
				// check for comment
				c = safeget();
				if (c == '#')
				{
					safeunget();
					safeunget();
					t = getspecialtoken();
				}
				// Check for truncate whitespace
				else if (c == '<')
					t.value.erase();
				else {
					if (c) safeunget();
					safeunget();
				}
			}
			else if (c)
				safeunget();
		}
		break;
	// Handle new lines
	case '\n':
	case '\r':
		t.type = token_whitespace;
		t.value.erase();
		getreturn(c, t.value);
		newline();
		if (ignoreblankline_ && col == 1)
			t.value.erase();
		break;
	case '"':	// quoted strings
		getstring(t);
		break;
	case '\'':
		getsqstring(t);
		break;
	case '\\':	// escape sequences
		c = safeget();
		t.value.erase();
		if (getreturn(c, t.value))
		{
			newline();
			t.type = token_joinline;
		}
		else
		{
			t.type = token_escape;
			t.value = c;
		}
		break;
	case '@':	// keyword or user macro
		c = safeget();
		t.value+= c;
		if (std::isalpha(c) || c == '_' || c == '.')
		{
			buildidentifier(t.value);
			t.type = checkreserved(&t.value[1]);
		}
		else if (c == '#') // this is a @# comment
			buildcomment(t);
		else if (c == '<')
		{
			// ignore
			t.type = token_whitespace;
			t.value.erase();
		}
		else if (c == '>')
		{
			// truncate space to the right
			t.type = token_whitespace;
			t.value.erase();
			while ( (c = safeget()) && std::isspace(c))
				;	// ignore
			if (c)
				safeunget();
		}
		else
		{
			if (c) safeunget();
			t.type = token_text;
			return t;
		}
		break;
	case '$':	// variable name
		getclosedidname(t);
		break;
	case '!':
		c = safeget();
		if (c == '=')
		{
			t.type = token_relop;
			t.value+= c;
		}
		else
		{
			t.type = token_operator;
			if (c) safeunget();
		}
		break;
	case '>':
	case '<':
	case '=':
		t.type = token_relop;
		c = safeget();
		if (c == '=')
			t.value+= c;
		else
			if (c) safeunget();
		break;
	default: t.type = token_error; break;
	}
	// Next, process tokens that are comprised of sets
	// Group digits together
	if (t.type == token_error)
	{
		if (std::isdigit(c))
		{
			t.type = token_integer;
			buildnumber(t.value);
		}
		else if (std::isalnum(c) || c == '_' || c == '.')
		{
			getidname(t);
		}
	}

	return t;
}


/*
 * Put one token back in the buffer.
 */
void Lex::unget(const Token<>& tok)
{
	register size_t i = tok.value.size();
	buf_.seek(buf_.offset() - i);
	column_-= i;
}


/*
 * Copy the next brace enclosed {} block of template into the specified string.
 *
 * The lineno_ variable is used to hold the actual starting line of the block.
 *
 */
bool Lex::getblock(std::string& block, unsigned& lineno)
{
	unsigned depth = 1;
	Token<> t(getloosetoken());
	size_t abortindex = buf_.offset();
	lineno = lineno_;

	// Ignore all whitespace before the opening brace;
	while (t.type == token_whitespace)
		t = getloosetoken();

	// If the current token is not an open brace then something is wrong, so
	// abort this function.
	if (t.type != token_openbrace)
	{
		// roll back parsing and abort
		buf_.seek(abortindex);
		lineno_ = lineno;
		return true;
	}
	lineno = lineno_;
	block = t.value;	// block should always start with open brace '{'

	// Iterate through the buffer until the close brace for this block is found
	do
	{
		t = getloosetoken();
		if (t.type == token_eof)
			return true;
		// Preserve escaped sequences
		else if (t.type == token_escape || t.type == token_joinline)
			block+= '\\';
		else if (t.type == token_openbrace)
			++depth;
		else if (t.type == token_closebrace)
			--depth;
		block+= t.value;
	} while (depth > 0);

	return false;
}

/*
 * Ignore the current block wrapped in braces { }
 *
 * lineno_ is used to hold the actual starting line of the block.
 *
 */
bool Lex::ignoreblock()
{
	unsigned depth = 1;
	Token<> t(getloosetoken());
	size_t abortindex = buf_.offset();
	unsigned lineno = t.lineno;	// preservice line number

	// Ignore all whitespace before the opening brace;
	while (t.type == token_whitespace)
		t = getloosetoken();

	// If the current token is not an open brace, then something is
	// wrong, so abort this function.
	if (t.type != token_openbrace)
	{
		// roll back parsing and abort
		buf_.seek(abortindex);
		lineno_ = lineno;
		return true;
	}

	// Iterate through the buffer until the close brace for this block is found
	do
	{
		t = getloosetoken();
		if (t.type == token_eof)
			return true;
		else if (t.type == token_openbrace)
			++depth;
		else if (t.type == token_closebrace)
			--depth;
	} while (depth > 0);

	return false;
}


unsigned long Lex::index() const
{
	return buf_.offset();
}

/*
 * @return true on failure;
 * @return false on success;
 *
 */
bool Lex::seek(unsigned long index)
{
	return buf_.seek(index);
}

/*
 * Handle whitespace and comment following brace
 */
void Lex::handlebraceignore(Token<>& t)
{
	// Ignore all whitespace after open brace
	getwhitespace(t.value);
	char c = *t.value.rbegin();	// Get last character in string
	// If the last character was not a <cr> or <lf>, then there may be a
	// comment to process
	if (c != '\r' && c != '\n')
	{
		// Now check for and ignore comments
		c = safeget();
		if (c == '@')
		{
			c = safeget();
			if (c == '#')
			{
				t.value+= "@#";
				// Get the comment and ignore it
				Token<> temp;
				buildcomment(temp);
				t.value+= temp.value;
				// Now remove newline following comment
				c = safeget();
				if (getreturn(c, temp.value))
				{
					t.value+= temp.value;
					newline();
				}
				else if (c)
					safeunget();
			}
			else
			{
				// If this was not a comment, put back up two characters
				if (c)
					safeunget();
				safeunget();
			}
		}
		// If this was not a function, put back character
		else if (c)
			safeunget();
	}
}

/*
 * Check a function token to see if it is a reserved word, assuming the
 * prepending @ is excluded.
 *
 */
Token<>::en Lex::checkreserved(const char* str)
{
	// Should not be in this function unless str[0] == '@'
	switch (*str)
	{
	case 'c':
		if (!std::strcmp(str, "compare"))	return token_compare;
		if (!std::strcmp(str, "comp"))		return token_compare;
		break;
	case 'e':
		if (!std::strcmp(str, "else"))		return token_else;
		if (!std::strcmp(str, "elsif"))		return token_elsif;
		if (!std::strcmp(str, "empty"))		return token_empty;
		break;
	case 'f':
		if (!std::strcmp(str, "foreach"))	return token_foreach;
		break;
	case 'i':
		if (!std::strcmp(str, "if"))		return token_if;
		if (!std::strcmp(str, "include"))	return token_include;
		if (!std::strcmp(str, "includetext"))	return token_includetext;
		if (!std::strcmp(str, "isarray"))	return token_isarray;
		if (!std::strcmp(str, "ishash"))	return token_ishash;
		if (!std::strcmp(str, "isscalar"))	return token_isscalar;
		if (!std::strcmp(str, "ignoreblankline"))
		{
			ignoreblankline_ = true;
			return token_comment;
		}
		else if (!std::strcmp(str, "ignoreindent"))
		{
			ignoreindent_ = true;
			return token_comment;
		}
		break;
	case 'k':
		if (!std::strcmp(str, "keys"))		return token_keys;
		break;
	case 'l':
		if (!std::strcmp(str, "last"))		return token_last;
		break;
	case 'm':
		if (!std::strcmp(str, "macro"))		return token_macro;
		break;
	case 'n':
		if (!std::strcmp(str, "next"))		return token_next;
		if (!std::strcmp(str, "noignoreindent"))
		{
			ignoreindent_ = false;
			return token_comment;
		}
		else if (!std::strcmp(str, "noignoreblankline"))
		{
			ignoreblankline_ = false;
			return token_comment;
		}
		break;
	case 'p':
		if (!std::strcmp(str, "pop"))		return token_pop;
		if (!std::strcmp(str, "push"))		return token_push;
		break;
	case 'r':
		if (!std::strcmp(str, "rand"))		return token_rand;
		break;
	case 's':
		if (!std::strcmp(str, "set"))		return token_set;
		if (!std::strcmp(str, "setif"))		return token_setif;
		if (!std::strcmp(str, "size"))		return token_size;
		if (!std::strcmp(str, "strcmp"))	return token_compare;
		break;
	case 't':
		if (!std::strcmp(str, "tpt_ignoreblankline"))
		{
			ignoreblankline_ = true;
			return token_comment;
		}
		else if (!std::strcmp(str, "tpt_ignoreindent"))
		{
			ignoreindent_ = true;
			return token_comment;
		}
		if (!std::strcmp(str, "tpt_noignoreindent"))
		{
			ignoreindent_ = false;
			return token_comment;
		}
		else if (!std::strcmp(str, "tpt_noignoreblankline"))
		{
			ignoreblankline_ = false;
			return token_comment;
		}
		break;
	case 'u':
		if (!std::strcmp(str, "unset"))		return token_unset;
		else if (!strcmp(str, "using"))		return token_using;
		break;
	case 'w':
		if (!std::strcmp(str, "while"))		return token_while;
		break;
	default:
		break;
	}
	return token_usermacro;
}


/*
 * Get an enclosed id token, assuming the prepending $ has
 * already been scanned.
 *
 */
void Lex::getclosedidname(Token<>& t)
{
	char c;

	// If next character is not a {, then this is not an id
	c = safeget();
	if (c != '{')
	{
		if (c) safeunget();
		t.type = token_text;
		return;
	}
	t.value+= c;

	// Use getidname instead of duplicating code
	getidname(t);
	if (t.type != token_id)
	{
		t.type = token_error;
		return;
	}
	// Get closing brace
	c = safeget();
	if (c == '}')
	{
		t.value+= c;
		t.type = token_id;
	}
	else
		t.type = token_error;
}


/*
 * Get an id token, assuming the first character has been scanned.
 *
 */
void Lex::getidname(Token<>& t)
{
	char c;

	t.type = token_id;
	while ( (t.type != token_error) && (c = safeget()) )
	{
		if (c == '$')
		{
			t.value+= c;
			getclosedidname(t);
			if (t.type != token_id)
			{
				t.type = token_error;
				return;
			}
		}
		else if (c == '[')
		{
			t.value+= c;
			getbracketexpr(t);
			c = safeget();
			if (c != ']')
				t.type = token_error;
			else
				t.value+= c;
		}
		else if (!std::isalnum(c) && c != '_' && c != '.')
		{
			safeunget();
			break;
		}
		else
			t.value+= c;
	}
}


void Lex::getbracketexpr(Token<>& t)
{
	// assume last char was '['
	char c = 0;
	while ( (t.type != token_error) && (c = safeget()) )
	{
		if (c == ']')
		{
			safeunget();
			return;
		}
		if (c == '$')
		{
			t.value+= c;
			getclosedidname(t);
			if (t.type != token_id)
			{
				t.type = token_error;
				return;
			}
		}
		else if (std::isalpha(c) || c == '_' || c == '.')
		{
			safeunget();
			getidname(t);
		}
		else
			t.value+= c;
	}
	if (!c)
		t.type = token_error;
	return;
}


/*
 * Get a string token, assuming the open quote has already been
 * scanned.
 *
 */
void Lex::getstring(Token<>& t)
{
	char c;
	t.type = token_string;
	t.value.erase();
	while ( (c = safeget()) )
	{
		if (c == '"')
			return;
		else if (c == '\\')
		{
			if ( !(c = safeget()) )
			{
				t.type = token_error;
				break;
			}
			switch (c)
			{
			case 'n':
				c = '\n';
				break;
			case 'r':
				c = '\r';
				break;
			case 't':
				c = '\t';
				break;
			case 'a':
				c = '\a';
				break;
			}
		}
		else if (c == '\n' || c == '\r')
		{
			safeunget();
			t.type = token_error;
			break;
		}
		t.value+= c;
	}
	if (!c)
		t.type = token_error;
}


/*
 * Get a string token, assuming open single quote has already been
 * scanned.
 *
 */
void Lex::getsqstring(Token<>& t)
{
	char c;
	t.type = token_string;
	t.value.erase();
	while ( (c = safeget()) )
	{
		if (c == '\'')
			return;
		else if (c == '\\')
		{
			if ( !(c = safeget()) )
			{
				t.type = token_error;
				break;
			}
			switch (c)
			{
			case 'n':
				c = '\n';
				break;
			case 'r':
				c = '\r';
				break;
			case 't':
				c = '\t';
				break;
			case 'a':
				c = '\a';
				break;
			}
		}
		else if (c == '\n' || c == '\r')
		{
			safeunget();
			t.type = token_error;
			break;
		}
		t.value+= c;
	}
	if (!c)
		t.type = token_error;
}


/*
 *
 */
void Lex::buildidentifier(std::string& value)
{
	while (char c = safeget())
	{
		if (std::isalnum(c) || c == '_' || c == '.')
			value+= c;
		else
		{
			safeunget();
			break;
		}
	}
}


/*
 *
 */
void Lex::buildnumber(std::string& value)
{
	while (char c = safeget())
	{
		if (std::isdigit(c))
			value+= c;
		else
		{
			safeunget();
			break;
		}
	}
}


/*
 *
 */
void Lex::buildrawtext(std::string& value)
{
	while (char c = safeget())
	{
		switch (c) {
		case '$':
		case '@':
		case '\\':
		case '{':
		case '}':
		case '\r':
		case '\n':
		case ' ':
		case '\t':
			safeunget();
			return;
		default:
			value+= c;
		}
	}
}


void Lex::getwhitespace(std::string& value)
{
	char c(safeget());
	if (!c)
		return;
	while (c == ' ' || c == '\t')
	{
		value+= c;
		c = safeget();
	}
	// eat a newline
	if (c && getreturn(c, value))
		newline();
	else
		safeunget();
}


/*
 * Build a return, keeping in mind that valid returns are \r, \n, or \r\n
 * depending on the system.
 */
bool Lex::getreturn(char c, std::string& cr)
{
	if (c == '\n')
	{
		cr+= c;
		return true;
	}
	else if (c == '\r')
	{
		cr+= c;
		c = safeget();
		if (c == '\n') cr+= c;
		else if (c) safeunget();
		return true;
	}
	return false;
}

/*
 * Same as make return except characters are discarded.
 */
bool Lex::ignorereturn(char c)
{
	if (c == '\n')
		return true;
	else if (c == '\r')
	{
		c = safeget();
		if (c && c != '\n')
			safeunget();
		return true;
	}
	return false;
}


void Lex::buildcomment(Token<>& t)
{
	char c;
	t.type = token_comment;

	while ( (c = safeget()) )
	{
		if (c == '\\')
		{
			// Special processing for \<cr>
			c = safeget();
			if (c == '\r' || c == '\n')
			{
				// Escaped <cr> is handled elsewhere
				safeunget();
				safeunget();
				break;
			}
			else if (c)
				safeunget();
		}
		else if (t.column == 1 && getreturn(c, t.value))
		{
			newline();
			break;
		}
		else if (c == '\r' || c == '\n')
		{
			safeunget();
			break;
		}
		t.value+= c;
	}
}


void Lex::newline()
{
	column_ = 1;
	++lineno_;
}


} // end namespace TPT
