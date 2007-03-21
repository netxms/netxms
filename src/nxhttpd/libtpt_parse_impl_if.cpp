/*
 * parse_impl_if.cxx
 *
 * Parse If Implementation
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

namespace TPT {


/*
 *
 */
bool Parser_Impl::parse_ifexpr(std::ostream* os)
{
	Object params;
	if (getparamlist(params))
		return false;
	Object::ArrayType& pl = params.array();

	if (pl.empty())
	{
		recorderror("Syntax error, expected expression");
		return false;
	}
	else if (pl.size() > 1)
		recorderror("Warning: extra parameters ignored");

	Object& obj = *pl[0].get();

	if (obj.gettype() != Object::type_scalar)
	{
		recorderror("Error: Excpected scalar expression");
		return false;
	}

	int64_t lwork;
	lwork = str2num(obj.scalar().c_str());	// lwork = result of if expression

	if (lwork)	// if true, then TPT if was true
		parse_block(os);
	else
		ignore_block();

	return !!lwork;
}

void Parser_Impl::parse_if(std::ostream* os)
{

	// Go ahead and parse the @if expression
	bool done = parse_ifexpr(os);

	// Add a couple more variables
	size_t saveindex = lex.index();
	Token<> tok(lex.getstricttoken());
	unsigned savelineno = tok.lineno;

	while (tok.type == token_elsif)
	{
		if (done)
		{
			Object pl;
			getparamlist(pl);
			ignore_block();
		}
		else
			done = parse_ifexpr(os);
		saveindex = lex.index();
		tok = lex.getstricttoken();
		savelineno = tok.lineno;
	}

	if (tok.type == token_else)
	{
		if (done)
			ignore_block();
		else
			parse_block(os);
	}
	else
	{
		// unget token
		lex.seek(saveindex);
		lex.setlineno(savelineno);
	}
}

} // end namespace TPT
