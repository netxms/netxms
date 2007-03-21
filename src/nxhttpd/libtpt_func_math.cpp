/*
 * func_math.cxx
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
#include <algorithm>
#include <sstream>
#include <iostream>

#include <cstdlib>
#include <cctype>
#include <cstring>

namespace TPT {

bool func_sum(std::ostream& os, TPT::Object& params)
{
	Object::ArrayType& pl = params.array();
	int64_t lwork = 0;
	bool iserr = false;

	Object::ArrayType::const_iterator it(pl.begin()), end(pl.end());
	for (; it != end; ++it)
	{
		Object& obj = *(*it).get();
		if (obj.gettype() == Object::type_array)
		{
			Object::ArrayType& temp = obj.array();
			Object::ArrayType::const_iterator ait(temp.begin()), aend(temp.end());
			for (; ait != aend; ++ait)
			{
				Object& subobj = *(*it).get();
				if (subobj.gettype() == Object::type_scalar)
					lwork+= str2num(subobj.scalar().c_str());
			}
		}
		else if (obj.gettype() == Object::type_scalar)
			lwork+= str2num(obj.scalar().c_str());
		else
			iserr = true;
	}
	std::string result;
	num2str(lwork, result);
	os << result;

	return iserr;
}


bool func_avg(std::ostream& os, TPT::Object& params)
{
	Object::ArrayType& pl = params.array();
	std::string result;
	int64_t lwork = 0, count = 0;
	bool iserr = true;
	
	Object::ArrayType::const_iterator it(pl.begin()), end(pl.end());
	for (; it != end; ++it)
	{
		Object& obj = *(*it).get();
		if (obj.gettype() == Object::type_array)
		{
			Object::ArrayType& temp = obj.array();
			Object::ArrayType::const_iterator ait(temp.begin()), aend(temp.end());
			for (; ait != aend; ++ait)
			{
				Object& subobj = *(*it).get();
				if (subobj.gettype() == Object::type_scalar)
				{
					lwork+= str2num(subobj.scalar().c_str());
					++count;
				}
			}
		}
		else if (obj.gettype() == Object::type_scalar)
		{
			lwork+= str2num(obj.scalar().c_str());
			++count;
		}
		else
			iserr = true;
	}
	count = !count ? 1 : count;
	lwork/= count;
	num2str(lwork, result);
	os << result;

	return iserr;
}


} // end namespace TPT
