/*
 * parse_impl_foreach.cxx
 *
 * Parse Foreach Implementation
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
#include <sstream>
#include <iostream>

namespace TPT {


void Parser_Impl::parse_foreach(std::ostream* os)
{
	std::string writeto(".");
	Object params;
	Token<> temp(lex.getstricttoken());
	bool stop = false;

	// Check for overide of default ID name
	if (temp.type == token_id)
		writeto = temp.value;
	else
		lex.unget(temp);

	// Get parameters
	if (getparamlist(params))
		return;
	Object::ArrayType& pl = params.array();

	// Get the Object Pointer to the symbol used to hold the values for
	// this foreach.
	Object::PtrType writeobj;
	if (symbols.imp->getobjectforset(writeto, symbols.imp->symbols, writeobj))
	{
		recorderror("Invalid identifier");
		ignore_block();
		return;
	}
	// Parse and process the parameter list
	size_t foreachstart = lex.index();
	unsigned lineno = lex.getlineno();

	Object::ArrayType::const_iterator pit(pl.begin()), pend(pl.end());
	for (; !stop && pit != pend; ++pit)
	{
		Object& obj = *(*pit).get();

		if (obj.gettype() == Object::type_array)
		{
			Object::ArrayType& temp = obj.array();
			Object::ArrayType::const_iterator it(temp.begin()), end(temp.end());
			for (; !stop && it != end; ++it)
			{
				// Set "writeto" object to object in this iterator
				*(writeobj.get()) = *(*it).get();
				if (!parse_loopblock(os))
					stop = true;
				lex.seek(foreachstart);
				lex.setlineno(lineno);
			}
		}
		else
		{
			if ((pl.size() == 1) && (obj.gettype() == Object::type_scalar) &&
				obj.scalar().empty())
			{
				break;
			}
			// Set "writeto" object to "obj"
			*(writeobj.get()) = obj;
			if (!parse_loopblock(os))
				stop = true;
			lex.seek(foreachstart);
			lex.setlineno(lineno);
		}
	}
	ignore_block();
}


} // end namespace TPT
