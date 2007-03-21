/*
 * parse_impl.cxx
 *
 * Parser Implementation
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

#include "libtpt_conf.h"
#include "libtpt_parse_impl.h"
#include "libtpt_funcs.h"
#include <algorithm>
#include <sstream>
#include <iostream>
#include <cstdio>

namespace TPT {

const char* toktypestr(const Token<>& tok);


void Parser_Impl::installfuncs()
{
	funcs["sum"] = func_sum;
	funcs["avg"] = func_avg;
	funcs["length"] = func_length;
	funcs["substr"] = func_substr;
	funcs["uc"] = func_uc;
	funcs["lc"] = func_lc;
	funcs["concat"] = func_concat;
	funcs["eval"] = func_concat;
	funcs["lpad"] = func_lpad;
	funcs["rpad"] = func_rpad;
	funcs["repeat"] = func_repeat;
}


bool Parser_Impl::pass1(std::ostream* os)
{
	parse_main(os);

	return !errlist.empty();
}


void Parser_Impl::parse_main(std::ostream* os)
{
	Token<> tok(lex.getloosetoken());
	while (tok.type != token_eof) {
		// Read a loosely defined token for outer pass
		switch (tok.type) {
		case token_eof:
			break;
		default:
			parse_dotoken(os, tok);
			break;
		}
		tok = lex.getloosetoken();
	}
}


void Parser_Impl::ignore_block()
{
	// Just get the block and don't do anything with it.
	if (lex.ignoreblock())
		recorderror("Error, expected { block }");
}

/*
 * Parse a brace enclosed {} block.  This call is used for macros and
 * if/else statements.
 *
 */
void Parser_Impl::parse_block(std::ostream* os)
{
	// The count guard ensures that level is incremented when this level is
	// entered and decremented when this level is left.
	cntguard<unsigned> gc(level);
	Token<> tok(lex.getstricttoken());
	// A block absolutely must start with an open brace '{'
	if (tok.type != token_openbrace)
		recorderror("Expected open brace '{'", &tok);

	do {
		// Read a loosely defined token for outer pass
		tok = lex.getloosetoken();
		switch (tok.type) {
		// Close brace (}) is end of block
		case token_closebrace:
			break;
		case token_eof:
			recorderror("Unexpected end of file");
			break;
		default:
			parse_dotoken(os, tok);
			break;
		}
	} while ( (tok.type != token_eof) && (tok.type != token_closebrace) );
}


/*
 * Add special handling to a loop block to support @next and @last
 * calls.
 *
 * @return	true on end of block or @next;
 * @return	false on @last
 *
 */
bool Parser_Impl::parse_loopblock(std::ostream* os)
{
	cntguard<unsigned> gc(level), glc(looplevel);
	loop_cmd = loop_ign;
	Token<> tok(lex.getstricttoken());
	if (tok.type != token_openbrace)
		recorderror("Expected open brace '{'", &tok);

	do {
		// Read a loosely defined token for outer pass
		tok = lex.getloosetoken();
		switch (tok.type) {
		case token_next:
			return true;
		case token_last:
			return false;
		// Close brace (}) is end of block
		case token_closebrace:
			break;
		case token_eof:
			recorderror("Unexpected end of file");
			break;
		default:
			parse_dotoken(os, tok);
			break;
		}
		if (loop_cmd == loop_next)
		{
			loop_cmd = loop_ign;
			return true;
		}
		if (loop_cmd == loop_last)
		{
			loop_cmd = loop_ign;
			return false;
		}
	} while ( (tok.type != token_eof) && (tok.type != token_closebrace) );
	return true;
}


/*
 * Handle any tokens not handled by the calling function
 *
 */
void Parser_Impl::parse_dotoken(std::ostream* os, Token<> tok)
{
	switch (tok.type)
	{
	// Quit on end of file.
	case token_eof:
		recorderror("Unexpected end of file");
		break;
	// Ignore join lines (i.e. backslash (\) last on line)
	// and comments.
	case token_comment:
	case token_joinline:
		break;
	// Just output whitespace and text.
	case token_whitespace:
	case token_text:
	case token_escape:
		if (os) *os << tok.value;
		break;
	// Expand symbols.
	case token_id:
		{
			std::string val;
			if (!symbols.get(tok.value, val))
				if (os) *os << val;
		}
		break;
	// Process an if statement.
	case token_if:
		parse_if(os);
		break;
	// loop through an array.
	case token_foreach:
		parse_foreach(os);
		break;
	// loop while expression is true.
	case token_while:
		parse_while(os);
		break;
	// Include another file.
	case token_include:
		parse_include(os);
		break;
	// Include a raw text file.
	case token_includetext:
		parse_includetext(os);
		break;
	case token_using:
		parse_using();
		break;
	// Set a variable.
	case token_set:
		parse_set();
		break;
	case token_setif:
		parse_setif();
		break;
	case token_unset:
		parse_unset();
		break;
	case token_keys:
		parse_keys();
		break;
	case token_push:
		parse_push();
		break;
	case token_pop:
		parse_pop();
		break;
	// Define a macro
	case token_macro:
		parse_macro();
		break;
	// Display a random number.
	case token_rand:
		tok = parse_rand();
		if (os) *os << tok.value;
		break;
	// Check if a variable is empty, but why would this be raw.
	case token_empty:
		tok = parse_empty();
		if (os) *os << tok.value;
		break;
	// Get size of array variable
	case token_size:
		tok = parse_size();
		if (os) *os << tok.value;
		break;
	// Compare two strings
	case token_compare:
		tok = parse_compare();
		if (os) *os << tok.value;
		break;
	// Check if symbol is array
	case token_isarray:
		tok = parse_isarray();
		if (os) *os << tok.value;
		break;
	// Check if symbol is hash
	case token_ishash:
		tok = parse_ishash();
		if (os) *os << tok.value;
		break;
	// Check if symbol is scalar
	case token_isscalar:
		tok = parse_isscalar();
		if (os) *os << tok.value;
		break;
	// Call a user defined macro
	case token_usermacro:
		if (isuserfunc(tok.value))
			userfunc(tok.value, os);
		else
			user_macro(tok.value, os);
		break;
	case token_next:
		if (looplevel > 0)
			loop_cmd = loop_next;
		else
			recorderror("Syntax error", &tok);
		break;
	case token_last:
		if (looplevel > 0)
			loop_cmd = loop_last;
		else
			recorderror("Syntax error", &tok);
		break;
	// Syntax errors should be hard to create at this point.
	default:
		recorderror("Syntax error", &tok);
		break;
	}
}


/*
 * Get the next strict token.
 * @return	false on success;
 * @return	true on end of file
 *
 */
bool Parser_Impl::getnextstrict(Token<>& target)
{
	target = lex.getstricttoken();
	return (target.type == token_eof);
}

void Parser_Impl::recorderror(const std::string& desc, const Token<>* neartoken)
{
	char buf[32];
	if (neartoken)
		std::sprintf(buf, "%u", neartoken->lineno);
	else
		std::sprintf(buf, "%u", lex.getlineno());
	std::string errstr(desc);
	errstr+= " at line ";
	errstr+= buf;
	if (neartoken)
	{
		errstr+= " near <";
		errstr+= toktypestr(*neartoken);
		errstr+= "> '";
		errstr+= neartoken->value;
		errstr+= "'";
	}
	errlist.push_back(errstr);
}


/*
 * Get a parenthesis enclosed, comma delimeted parameter list.
 *
 * @param	pl			Reference to List to receive parameters.
 * @return	false on success;
 * @return	true on failure
 *
 */
bool Parser_Impl::getparamlist(Object& pl)
{
	pl = Object::type_array;
	// Expect opening parenthesis
	Token<> tok(lex.getstricttoken());
	if (tok.type != token_openparen)
	{
		recorderror("Syntax error, parameters must be enclosed in "
			"parenthesis", &tok);
		return true;
	}

	// tok holds the current token and nexttok holds the next token
	// returned by the rd parser.
	tok = lex.getstricttoken();
	while ((tok.type != token_eof) && (tok.type != token_closeparen))
	{
		Object obj(tok);
		Object nextobj = parse_level0(obj);
		pl.array().push_back(new Object(obj));

		if (nextobj.gettype() != Object::type_token)
		{
			recorderror("Syntax error, expected comma or close parenthesis", &tok);
			return true;
		}
		tok = nextobj.token();
		// The next token should be a comma or a close paren
		if (tok.type == token_closeparen)
			break;
		else if (tok.type != token_comma)
		{
			recorderror("Syntax error, expected comma or close parenthesis", &tok);
			return true;
		}
		tok = lex.getstricttoken();
	}

	return false;
}


/*
 * Get a parenthesis enclosed, comma delimeted id and parameter list.
 *
 * @param	id			Reference to String to receive id.
 * @param	pl			Reference to List to receive parameters.
 * @return	false on success;
 * @return	true on failure
 *
 */
bool Parser_Impl::getidparamlist(std::string& id,  Object& pl)
{
	pl = Object::type_array;
	// Expect opening parenthesis
	Token<> tok(lex.getstricttoken());
	if (tok.type != token_openparen)
	{
		recorderror("Syntax error, parameters must be enclosed in "
			"parenthesis", &tok);
		return true;
	}

	// tok holds the current token and nexttok holds the next token
	// returned by the rd parser.
	tok = lex.getstricttoken();
	if (tok.type != token_id)
	{
		recorderror("Syntax error, expected id", &tok);
		return true;
	}
	id = tok.value;
	tok = lex.getstricttoken();
	if (tok.type == token_comma)
		tok = lex.getstricttoken();

	while ((tok.type != token_eof) && (tok.type != token_closeparen))
	{
		Object obj(tok);
		Object nextobj = parse_level0(obj);
		pl.array().push_back(new Object(obj));

		if (nextobj.gettype() != Object::type_token)
		{
			recorderror("Syntax error, expected comma or close parenthesis", &tok);
			return true;
		}
		tok = nextobj.token();
		// The next token should be a comma or a close paren
		if (tok.type == token_closeparen)
			break;
		else if (tok.type != token_comma)
		{
			recorderror("Syntax error, expected comma or close parenthesis", &tok);
			return true;
		}
		tok = lex.getstricttoken();
	}

	return false;
}


/*
 * Get a parenthesis enclosed, comma delimeted list of ids.
 *
 * @param	id			Reference to String to receive id.
 * @param	pl			Reference to List to receive parameters.
 * @return	false on success;
 * @return	true on failure
 *
 */
bool Parser_Impl::getidlist(std::vector< std::string >& ids)
{
	ids.clear();
	// Expect opening parenthesis
	Token<> tok(lex.getstricttoken());
	if (tok.type != token_openparen)
	{
		recorderror("Syntax error, parameters must be enclosed in "
			"parenthesis", &tok);
		return true;
	}


	tok = lex.getstricttoken();
	while ((tok.type != token_eof) && (tok.type != token_closeparen))
	{
		// tok holds the current token and nexttok holds the next token
		// returned by the rd parser.
		if (tok.type != token_id)
		{
			recorderror("Syntax error, expected id", &tok);
			return true;
		}
		ids.push_back(tok.value);
		tok = lex.getstricttoken();
		// The next token should be a comma or a close paren
		if (tok.type == token_closeparen)
			break;
		else if (tok.type != token_comma)
		{
			recorderror("Syntax error, expected comma or close parenthesis", &tok);
			return true;
		}
		tok = lex.getstricttoken();
	}

	return false;
}

const char* toktypestr(const Token<>& tok)
{
	switch(tok.type) {
	case token_error:
		return "error";
	case token_eof:
		return "eof";
	case token_id:
		return "id";
	case token_usermacro:
		return "usermacro";
	case token_integer:
		return "integer";
	case token_string:
		return "string";
	case token_text:
		return "text";
	case token_comment:
		return "comment";
	case token_whitespace:
		return "whitespace";
	case token_joinline:
		return "joinline";
	case token_escape:
		return "escape";
	case token_openbrace:
		return "openbrace";
	case token_closebrace:
		return "closebrace";
	case token_openparen:
		return "openparen";
	case token_closeparen:
		return "closeparen";
	case token_comma:
		return "comma";
	case token_quote:
		return "quote";
	case token_operator:
		return "operator";
	case token_relop:
		return "relop";
	case token_include:
		return "include";
	case token_includetext:
		return "includetext";
	case token_using:
		return "using";
	case token_set:
		return "set";
	case token_setif:
		return "setif";
	case token_keys:
		return "keys";
	case token_macro:
		return "macro";
	case token_foreach:
		return "foreach";
	case token_while:
		return "while";
	case token_next:
		return "next";
	case token_last:
		return "last";
	case token_if:
		return "if";
	case token_elsif:
		return "elsif";
	case token_else:
		return "else";
	case token_empty:
		return "empty";
	case token_rand:
		return "rand";
	case token_size:
		return "size";
	default:
		return "undefined";
	}
}


} // end namespace TPT
