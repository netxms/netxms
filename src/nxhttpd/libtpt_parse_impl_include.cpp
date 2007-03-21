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
#include <algorithm>
#include <sstream>
#include <iostream>

namespace TPT {


void Parser_Impl::parse_include(std::ostream* os)
{
	Object params;
	if (getparamlist(params))
		return;
	Object::ArrayType& pl = params.array();

	if (pl.size() != 1)
	{
		recorderror("Error: @include takes exactly 1 parameter");
		return;
	}

	Object& obj = *pl[0].get();
	if (obj.gettype() != Object::type_scalar)
	{
		recorderror("Error: @include excpects string");
		return;
	}

	// Create another Impl which inherits symbols table
	// to process include
	if (!inclist.empty())
	{
		IncludeList::iterator it(inclist.begin()), end(inclist.end());
		for (; it != end; ++it)
		{
			std::string path(*it);
			path+= '/';
			path+= obj.scalar().c_str();
			Buffer buf(path.c_str());
			if (buf)
			{
				Parser_Impl incl(buf, symbols, macros, funcs, inclist);
				if (incl.pass1(os))
				{
					// copy incl's error list, if any
					ErrorList::iterator it(incl.errlist.begin()), end(incl.errlist.end());
					for (; it != end; ++it)
						errlist.push_back(*it);
				}
				return;
			}
		}
	}
	Buffer buf(obj.scalar().c_str());
	if (!buf)
	{
		recorderror("File Error: Could not read " + obj.scalar());
		return;
	}
	Parser_Impl incl(buf, symbols, macros, funcs, inclist);
	if (incl.pass1(os))
	{
		// copy incl's error list, if any
		ErrorList::iterator it(incl.errlist.begin()), end(incl.errlist.end());
		for (; it != end; ++it)
			errlist.push_back(*it);
	}
}


void Parser_Impl::parse_includetext(std::ostream* os)
{
	Object params;
	if (getparamlist(params))
		return;
	Object::ArrayType& pl = params.array();

	if (pl.size() != 1)
	{
		recorderror("Error: @includetext takes exactly 1 parameter");
		return;
	}

	Object& obj = *pl[0].get();
	if (obj.gettype() != Object::type_scalar)
	{
		recorderror("Error: @include excpects string");
		return;
	}

	// Create another Impl which inherits symbols table
	// to process include
	if (!inclist.empty())
	{
		IncludeList::iterator it(inclist.begin()), end(inclist.end());
		for (; it != end; ++it)
		{
			std::string path(*it);
			path+= '/';
			path+= obj.scalar().c_str();
			Buffer buf(path.c_str());
			if (buf)
			{
				while (buf)
					*os << buf.getnextchar();
				return;
			}
		}
	}
	Buffer buf(obj.scalar().c_str());
	if (!buf)
	{
		recorderror("File Error: Could not read " + obj.scalar());
		return;
	}
	while (buf)
		*os << buf.getnextchar();
}

} // end namespace TPT
