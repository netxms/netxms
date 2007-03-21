/*
 * parse_impl_set.cxx
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

void Parser_Impl::parse_unset()
{
	std::string id;
	Object params;

	if (getidparamlist(id, params))
		return;
	Object::ArrayType& pl = params.array();

	if (!pl.empty())
		recorderror("Warning: @unset takes only an id parameter");

	symbols.unset(id);
}


/*
 * Set the value of the specified symbol
 */
void Parser_Impl::parse_set()
{
	std::string id;
	Object params;

	if (getidparamlist(id, params))
		return;
	Object::ArrayType& pl = params.array();

	if (pl.empty())
		symbols.set(id, "");
	else
	{
		Object::PtrType ptr;
		if (symbols.imp->getobjectforset(id,
			symbols.imp->symbols, ptr))
		{
			recorderror("Invalid symbol");
			return;
		}
		Object& obj = *ptr.get();
		if (pl.size() > 1)
		{
			// copy whole array
			obj = params;
		}
		else
		{
			// copy scalar
			obj = *pl[0].get();
		}
	}
}


/*
 * Only set the variable if it is empty.
 *
 */
void Parser_Impl::parse_setif()
{
	std::string id;
	Object params;

	if (getidparamlist(id, params))
		return;
	Object::ArrayType& pl = params.array();

	// Only set if symbol is empty
	if (!symbols.empty(id))
		return;

	if (pl.empty())
	{
		if (!symbols.empty(id))
			symbols.set(id, "");
	}
	else
	{
		Object::PtrType ptr;
		if (symbols.imp->getobjectforset(id,
			symbols.imp->symbols, ptr))
		{
			recorderror("Invalid symbol");
			return;
		}
		Object& obj = *ptr.get();
		if (pl.size() > 1)
		{
			// copy whole array
			obj = params;
		}
		else
		{
			// copy scalar
			obj = *pl[0].get();
		}
	}
}


/*
 * Copy hash keys into an array
 *
 */
void Parser_Impl::parse_keys()
{
	std::string id;
	Object params;

	if (getidparamlist(id, params))
		return;
	Object::ArrayType& pl = params.array();

	if (pl.size() != 1)
		recorderror("@keys takes two arguments");
	else
	{
		Object::PtrType ptr;
		if (symbols.imp->getobjectforset(id,
			symbols.imp->symbols, ptr))
		{
			recorderror("Invalid symbol");
			return;
		}
		Object& aobj = *ptr.get();
		aobj = Object::type_array;
		Object& hobj = *(pl[0].get());
		if (hobj.gettype() != Object::type_hash)
		{
			recorderror("Expected hash as second argument");
			return;
		}
		Object::HashType::iterator it(hobj.hash().begin()),
			end(hobj.hash().end());
		for (; it != end; ++it)
			aobj.array().push_back(new Object(it->first));
	}
}


/*
 * Push a variable onto an array.
 */
void Parser_Impl::parse_push()
{
	std::string id;
	Object params;

	if (getidparamlist(id, params))
		return;
	Object::ArrayType& pl = params.array();

	Object::PtrType ptr;
	if (symbols.imp->getobjectforset(id,
		symbols.imp->symbols, ptr))
	{
		recorderror("Invalid symbol");
		return;
	}
	Object& obj = *ptr.get();
	if (obj.gettype() != Object::type_array)
		obj = Object::type_array;
	Object::ArrayType& array = obj.array();
	if (pl.empty())
		array.push_back(new Object(""));
	else
	{
		if (pl.size() > 1)
		{
			// push whole array
			array.push_back(new Object(params));
		}
		else
		{
			// push scalar
			array.push_back(new Object(*pl[0].get()));
		}
	}
}

/*
 * Pop last object off array (second parameter) into symbol (first paramter)
 */
void Parser_Impl::parse_pop()
{
	std::vector< std::string > ids;
	Object params;

	if (getidlist(ids))
		return;

	if (ids.size() != 2)
	{
		recorderror("@pop takes destination and array parameters");
		return;
	}
	Object::PtrType varptr, arrayptr;
	if (symbols.imp->getobjectforset(ids[0], symbols.imp->symbols, varptr))
	{
		recorderror("Invalid symbol in first parameter");
		return;
	}
	if (symbols.imp->getobjectforset(ids[1], symbols.imp->symbols, arrayptr))
	{
		symbols.set(ids[0], "");
		recorderror("Invalid symbol in second parameter`");
		return;
	}
	Object& varobj = *varptr.get();
	Object& arrayobj = *arrayptr.get();
	if (arrayobj.gettype() != Object::type_array)
	{
		symbols.set(ids[0], "");
		recorderror("Second parameter must be an array");
		return;
	}
	Object::ArrayType& array = arrayobj.array();
	varobj = *(array[array.size()-1].get());
	array.pop_back();
}

} // end namespace TPT

