/*
 * func_misc.cxx
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
#include <iostream>

namespace TPT {

/*
 * Reinventing the wheel?  Not all libraries support these functions
 *
 */
int64_t str2num(const char* str)
{
	int64_t value = 0;
	bool isneg(false);

	if (*str == '-')
	{
		++str;
		isneg = true;
	}
	while (*str)
	{
		value*= 10;
		value+= (*str) - '0';
		++str;
	}
	if (isneg)
		value = -value;
	return value;
}


void num2str(int64_t value, std::string& str)
{
	char buf[32];

	str.erase();
	if (!value)
		str = "0";
	else
	{
		int i=0;
		if (value < 0)
		{
			str = '-';
			value = -value;
		}
		while (value)
		{
			buf[i++] = static_cast<char>(value % 10) + '0';
			value/= 10;
		}
		do {
			--i;
			str+= buf[i];
		} while (i);
	}
}


} // end namespace TPT
