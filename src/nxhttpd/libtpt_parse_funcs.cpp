/*
 * parse_funcs.cxx
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

#include <cstdlib>
#include <ctime>
#include <cctype>
#include <cstring>

namespace TPT {


/*
 * The random number algorithm is the KISS algorithm which I found on
 * the Internet years ago.  I don't know who to give credit to, so if
 * you know, please tell me.  I use it because it's fast and works
 * well.  (Def: KISS = Keep it simple stupid)
 *
 * The idea is to use several different pseudorandom number algorithms
 * to generate a good random looking composite.
 *
 */
void Parser_Impl::srandom32()
{
	// Seeds always start with the time!
	kiss_seed = time(NULL);
	// Then we mix the time with the process id.
	kiss_seed = (kiss_seed << 16) ^ kiss_seed ^ (getpid()<<1);
	kiss_x = kiss_seed | 1;
	kiss_y = kiss_seed | 2;
	kiss_z = kiss_seed | 4;
	kiss_w = kiss_seed | 8;
	kiss_carry = 0;
}

unsigned int Parser_Impl::random32()
{
	if (!isseeded)
	{
		srandom32();
		isseeded = true;
	}

	kiss_x*=69069;	//	kiss_x = kiss_x * 69069 + 1;
	++kiss_x;
	kiss_y ^= kiss_y << 13;
	kiss_y ^= kiss_y >> 17;
	kiss_y ^= kiss_y << 5;
	kiss_k = (kiss_z >> 2) + (kiss_w >> 3) + (kiss_carry >> 1);
	kiss_m = kiss_w + kiss_w + kiss_z + kiss_carry;
	kiss_z = kiss_w;
	kiss_w = kiss_m;
	kiss_carry = kiss_k >> 30;
	return kiss_x + kiss_y + kiss_w;
}


/*
 * Generate a random 32-bit integer token
 *
 */
Token<> Parser_Impl::parse_rand()
{
	Object params;
	Token<> result;
	result.type = token_integer;
	if (getparamlist(params))
		return result;
	int64_t lwork = 0xFFFFFFFF;
	Object::ArrayType& pl = params.array();

	if (!pl.empty())
	{
		Object& obj = *pl[0].get();
		if (obj.gettype() != Object::type_scalar)
			recorderror("Error: Expected scalar expression");
		else
			lwork = str2num(obj.scalar().c_str());
		if (pl.size() > 1)
			recorderror("Warning: @rand takes zero or one arguments");
	}
	lwork = random32() % lwork;
	num2str(lwork, result.value);

	return result;
}


/*
 * Return a true token (i.e. "1") if parameter token is empty, or false
 * token (i.e. "0") if parameter token is not empty.
 *
 * Note: "" is empty, but "0" is not empty, therefore, only string
 * tokens can be empty.
 *
 */
Token<> Parser_Impl::parse_empty()
{
	Object params;
	Token<> result;
	result.type = token_integer;
	if (getparamlist(params))
		return result;
	Object::ArrayType& pl = params.array();

	if (pl.empty())
	{
		// use default parameter
		result.value = "1";
	}
	else
	{
		Object& obj = *pl[0].get();
		if (obj.gettype() != Object::type_scalar)
			recorderror("Error: Expected scalar expression");
		else
			result.value = (obj.scalar().empty()) ? "1" : "0";
		if (pl.size() > 1)
			recorderror("Warning: @empty takes one argument");
	}
	return result;
}


/*
 *
 */
Token<> Parser_Impl::parse_size()
{
	Object params;
	Token<> result;
	result.type = token_integer;
	if (getparamlist(params))
		return result;
	Object::ArrayType& pl = params.array();
	size_t size=0;

	Object::ArrayType::const_iterator it(pl.begin()), end(pl.end());
	for (; it != end; ++it)
	{
		Object& obj = *(*it).get();
		if (obj.gettype() == Object::type_array)
			size+= obj.array().size();
		else if (obj.gettype() == Object::type_hash)
			size+= obj.hash().size() * 2;
		else if (obj.gettype() == Object::type_scalar)
			size+= !obj.scalar().empty();
	}
	num2str(size, result.value);

	return result;
}


/*
 * Compare two strings
 *
 */
Token<> Parser_Impl::parse_compare()
{
	Object params;
	Token<> result;
	result.type = token_integer;
	if (getparamlist(params))
		return result;
	Object::ArrayType& pl = params.array();

	if (pl.size() < 2)
	{
		recorderror("Error: @compare takes two parameters");
		if (pl.size() == 1)
			result.value = "-1";
		return result;
	}
	if (pl.size() > 2)
		recorderror("Warning: @compare ignoring extra parameter(s)");

	Object& obj0 = *pl[0].get();
	Object& obj1 = *pl[1].get();
	if ((obj0.gettype() != Object::type_scalar) ||
		(obj1.gettype() != Object::type_scalar))
	{
		recorderror("Error: Expected scalar expression");
	}
	else
	{
		int lwork = std::strcmp(obj0.scalar().c_str(), obj1.scalar().c_str());
		num2str(lwork, result.value);
	}
	
	return result;
}

Token<> Parser_Impl::parse_isarray()
{
	Object params;
	Token<> result;
	result.type = token_integer;
	if (getparamlist(params))
		return result;
	Object::ArrayType& pl = params.array();

	if (pl.empty())
	{
		// use default parameter
		result.value = "0";
	}
	else
	{
		Object& obj = *pl[0].get();
		result.value = (obj.gettype() == Object::type_array)
			? "1" : "0";
		if (pl.size() > 1)
			recorderror("Warning: @isarray ignoring extra parameters");
	}
	return result;
}

Token<> Parser_Impl::parse_ishash()
{
	Object params;
	Token<> result;
	result.type = token_integer;
	if (getparamlist(params))
		return result;
	Object::ArrayType& pl = params.array();

	if (pl.empty())
	{
		// use default parameter
		result.value = "0";
	}
	else
	{
		Object& obj = *pl[0].get();
		result.value = (obj.gettype() == Object::type_hash)
			? "1" : "0";
		if (pl.size() > 1)
			recorderror("Warning: @ishash ignoring extra parameters");
	}
	return result;
}

Token<> Parser_Impl::parse_isscalar()
{
	Object params;
	Token<> result;
	result.type = token_integer;
	if (getparamlist(params))
		return result;
	Object::ArrayType& pl = params.array();

	if (pl.empty())
	{
		// use default parameter
		result.value = "0";
	}
	else
	{
		Object& obj = *pl[0].get();
		result.value = (obj.gettype() == Object::type_scalar)
			? "1" : "0";
		if (pl.size() > 1)
			recorderror("Warning: @isscalar ignoring extra parameters");
	}
	return result;
}



} // end namespace TPT
