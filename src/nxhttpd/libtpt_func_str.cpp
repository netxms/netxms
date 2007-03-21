/*
 * func_str.cxx
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

#include "libtpt_funcs.h"
#include <libtpt/object.h>
#include <cstring>

namespace TPT {

/*
 * Concatenate a series of string together.  Integer tokens will be
 * treated as strings.
 *
 */
bool func_concat(std::ostream& os, TPT::Object& params)
{
	Object::ArrayType& pl = params.array();
	bool iserr = false;

	if (!pl.empty())
	{
		Object::ArrayType::const_iterator it(pl.begin()), end(pl.end());
		for (; it != end; ++it)
		{	   
			Object& obj = *(*it).get();
			if (obj.gettype() == Object::type_scalar)
				os << obj.scalar();
			else
				iserr = true;
		}	   
	}

	return iserr;
}


/*
 * Get length of string.
 *
 */
bool func_length(std::ostream& os, Object& params)
{
	Object::ArrayType& pl = params.array();
	bool iserr = false;
	int64_t lwork=0;
	std::string result;

	if (pl.size() != 1)
		iserr = true;
	else
	{   
		Object& obj = *pl[0].get();
		if (obj.gettype() == Object::type_scalar)
			lwork = obj.scalar().size();
		else
			iserr = true;
	}
	num2str(lwork, result);
	os << result;
	return iserr;
}

/*
 * Return a substring of a token.
 *
 * Param1 is string
 * Param2 is start
 * Param3 is end (optional)
 *
 */
bool func_substr(std::ostream& os, Object& params)
{
	Object::ArrayType& pl = params.array();
	unsigned int start=0, end=std::string::npos;
	bool iserr = false;

	if (!pl.empty())
	{   
		if (pl.size() == 1)
			return true;
		else
		{   
			Object& obj1 = *pl[1].get();
			if (obj1.gettype() != Object::type_scalar)
				return true;
			start = std::atoi(obj1.scalar().c_str());
			if (pl.size() >= 3)
			{   
				Object& obj2 = *pl[2].get();
				if (obj2.gettype() != Object::type_scalar)
					return true;
				end = std::atoi(obj2.scalar().c_str());
			}
			if (pl.size() > 3)
				iserr = true;
		}
		Object& obj = *pl[0].get();
		if (obj.gettype() != Object::type_scalar)
			return true;
		os << obj.scalar().substr(start,end);
	}
	else
		return true;
	
	return iserr;
}


/*
 * Convert a string to uppercase
 *
 */
bool func_uc(std::ostream& os, Object& params)
{
	Object::ArrayType& pl = params.array();
	bool iserr = false;

	if (pl.empty())
		return true;
	if (pl.size() > 1)
		iserr = true;
	Object& obj = *pl[0].get();
	if (obj.gettype() != Object::type_scalar)
		return true;
	std::string result(obj.scalar());
	std::string::iterator it(result.begin()),
		end(result.end());
	for (; it != end; ++it)
		(*it) = std::toupper((*it));
	
	os << result;
	return iserr;
}


/*
 * Convert a string to lowercase
 *
 */
bool func_lc(std::ostream& os, Object& params)
{
	Object::ArrayType& pl = params.array();
	bool iserr = false;

	if (pl.empty())
		return true;
	if (pl.size() > 1)
		iserr = true;
	Object& obj = *pl[0].get();
	if (obj.gettype() != Object::type_scalar)
		return true;
	std::string result(obj.scalar());
	std::string::iterator it(result.begin()),
		end(result.end());
	for (; it != end; ++it)
		(*it) = std::tolower((*it));
	
	os << result;
	return iserr;
}


/*
 * Pad a string on the left with spaces to fit specified width.
 */
bool func_lpad(std::ostream& os, Object& params)
{
	Object::ArrayType& pl = params.array();

	if (pl.empty() || pl.size() != 2)
		return true;
	Object& strobj = *pl[0].get(),
		numobj = *pl[1].get();
	if (strobj.gettype() != Object::type_scalar ||
			numobj.gettype() != Object::type_scalar)
		return true;
	std::string& str = strobj.scalar();
	unsigned width = std::atoi(numobj.scalar().c_str());
	if (str.length() >= width)
	{
		os << str;
		return false;
	}
	unsigned spacecnt = width - str.length();
	std::string spaces(" ");
	while (--spacecnt)
		spaces+= ' '; 
	os << spaces << str;
	return false;
}

/*
 * Pad a string on the right with spaces to fit specified width.
 */
bool func_rpad(std::ostream& os, Object& params)
{
	Object::ArrayType& pl = params.array();

	if (pl.empty() || pl.size() != 2)
		return true;
	Object& strobj = *pl[0].get(),
		numobj = *pl[1].get();
	if (strobj.gettype() != Object::type_scalar ||
			numobj.gettype() != Object::type_scalar)
		return true;
	std::string& str = strobj.scalar();
	unsigned width = std::atoi(numobj.scalar().c_str());
	if (str.length() >= width)
	{
		os << str;
		return false;
	}
	unsigned spacecnt = width - str.length();
	std::string spaces(" ");
	while (--spacecnt)
		spaces+= ' '; 
	os << str << spaces;
	return false;
}

/*
 * Repeat the specified text n times.
 */
bool func_repeat(std::ostream& os, Object& params)
{
	Object::ArrayType& pl = params.array();

	if (pl.empty() || pl.size() != 2)
		return true;
	Object& strobj = *pl[0].get(),
		numobj = *pl[1].get();
	if (strobj.gettype() != Object::type_scalar ||
			numobj.gettype() != Object::type_scalar)
		return true;
	std::string& str = strobj.scalar();
	int n = std::atoi(numobj.scalar().c_str());
	while (n-- > 0)
		os << str;
	return false;
}



} // end namespacr TPT
