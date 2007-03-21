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
#include "libtpt_symbols_impl.h"
#include <algorithm>
#include <sstream>
#include <iostream>

namespace TPT {


void Parser_Impl::parse_macro()
{
	Token<> tok;
	if (level > 0)
	{
		recorderror("Macro may not be defined in sub-block");
		return;
	}
	if (getnextstrict(tok))
		return;
	if (tok.type != token_openparen)
	{
		recorderror("Expected macro declaration");
		return;
	}
	if (getnextstrict(tok))
		return;
	if (tok.type != token_id)
	{
		recorderror("Macro requires name parameter");
		return;
	}
	Macro newmacro;
	std::string name(tok.value);

	tok = lex.getstricttoken();
	while (tok.type == token_comma)
	{
		tok = lex.getstricttoken();
		if (tok.type != token_id)
		{
			recorderror("Syntax error, expected identifier", &tok);
			return;
		}
		if ((tok.value.find('{') != std::string::npos) ||
				(tok.value.find('[') != std::string::npos))
		{
			recorderror("Syntax error, invalid parameter name", &tok);
			return;
		}
		newmacro.params.push_back(tok.value);
		tok = lex.getstricttoken();

		if (tok.type == token_closeparen)
			break;
		// The next token should be a comma or a close paren
		if (tok.type != token_comma)
		{
			recorderror("Syntax error, expected comma or close parenthesis", &tok);
			return;
		}
	}
	if (tok.type == token_eof)
	{
		recorderror("Unexpected end of file");
		return;
	}
	else if (tok.type != token_closeparen)
	{
		recorderror("Expected close parenthesis", &tok);
		return;
	}

	if (lex.getblock(newmacro.body, newmacro.lineno))
	{
		recorderror("Expected macro body {}");
		return;
	}
	macros[name] = newmacro;
}


void Parser_Impl::user_macro(const std::string& name, std::ostream* os)
{
	std::string id(name.substr(1));
	Object params;
	if (getparamlist(params))
		return;
	Object::ArrayType& pl = params.array();

	// The id does not include the prefix @.
	MacroList::const_iterator it(macros.find(id));
	if (it == macros.end())
	{
		recorderror("Undefined macro: " + name);
		return;
	}

	const Macro& mac = (*it).second;
	// While processing parameters, preserve existing symbols
	// if there is a name collision.
	Object saveobj(Object::type_hash);
	// savehash holds a temporary copy of the original symbols.
	Object::HashType& savehash = saveobj.hash();
	Symbols_Impl& symimp = *symbols.imp;

	ParamList::const_iterator pit(mac.params.begin()),
		pend(mac.params.end());
	Object::PtrType objptr;
	unsigned i = 0;
	for (; pit != pend; ++pit)
	{
		// Save any global IDs with conflicting identifiers.
		if (!symimp.getobjectforget((*pit), symimp.symbols, objptr)
				&& objptr.get())
			savehash[*pit] = objptr;

		if (i >= pl.size())
			symimp.symbols.hash()[*pit] = new Object("");
		else
			symimp.symbols.hash()[*pit] = new Object(*pl[i++].get());
	}

	// Call the macro
	Buffer newbuf(mac.body.c_str(), mac.body.size()+1);
	Parser_Impl imp(newbuf, symbols, macros, funcs, inclist);
	imp.lex.setlineno(mac.lineno);
	imp.parse_block(os);

	// Pop saved symbols off the stack
	Object::HashType::iterator hit;
	pit = mac.params.begin();
	for (; pit != pend; ++pit)
	{
		hit = savehash.find((*pit));
		if (hit != savehash.end())
			symimp.symbols.hash()[(*pit)] = (*hit).second;
		else
		{
			Object::HashType::iterator temp(symimp.symbols.hash().find(*pit));
			if (temp != symimp.symbols.hash().end())
				symimp.symbols.hash().erase(temp);
		}
	}
}


bool Parser_Impl::isuserfunc(const std::string& name)
{
	if (name[0] == '@')
	{
		FunctionList::const_iterator it(funcs.find(name.substr(1)));
		return (it != funcs.end());
	} else {
		FunctionList::const_iterator it(funcs.find(name));
		return (it != funcs.end());
	}
}


void Parser_Impl::userfunc(const std::string& name, std::ostream* os)
{
	Object params;
	if (getparamlist(params))
		return;
	FunctionList::const_iterator it(funcs.find(name.substr(1)));
	if (it == funcs.end())
		return;

	bool (*func)(std::ostream&, Object&);
	func = it->second;
	// Do not trust user callbacks to behave.
	try {
		// Call the user defined function.
		if (func(*os, params))
		{
			std::string errstr("Error reported by function ");
			errstr+= name;
			errstr+= "()";
			recorderror(errstr);
		}
	} catch (const std::exception& e) {
		std::string errstr("EXCEPTION caused by function ");
		errstr+= name;
		errstr+= "():";
		errstr+= e.what();
		recorderror(errstr);
	} catch (...) {
		std::string errstr("EXCEPTION caused by function ");
		errstr+= name;
		errstr+= "()";
		recorderror(errstr);
	}
}

} // end namespace TPT
