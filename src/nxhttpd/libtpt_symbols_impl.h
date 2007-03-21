/*
 * symbols_impl.h
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

#ifndef include_libtpt_symbols_impl_h
#define include_libtpt_symbols_impl_h

#include <libtpt/object.h>
#include <libtpt/symbols.h>
#include "libtpt_conf.h"
#include <string>
#include <vector>
#include <map>

namespace TPT {

typedef SymbolArrayType SymbolValue;

struct Symbol_t {
	std::string	id;
	Object::PtrType value;

	Symbol_t() {}
	Symbol_t(const Symbol_t& x) : id(x.id), value(x.value) {}
	Symbol_t& operator=(const Symbol_t& x)
	{
		id = x.id;
		value = x.value;
		return *this;
	}
};

/*
 * The private implementation of Symbols.
 *
 */
class Symbols_Impl {
public:
	Symbols& parent;
	Object symbols;
	Object emptyobject;

	Symbols_Impl(Symbols& p) : parent(p), symbols(Object::type_hash), emptyobject("") {}
	Symbols_Impl(Symbols& p, const Object& obj) : parent(p), symbols(obj), emptyobject("") {}
	~Symbols_Impl() {};

	void copy(Object& table);

	Object& getobject(const SymbolKeyType& id, Object& table);
	bool setobject(const SymbolKeyType& id, const std::string& value,
		Object& table);
	bool setobject(const SymbolKeyType& id,
		const SymbolArrayType& value, Object& table);
	bool setobject(const SymbolKeyType& id,
		const SymbolHashType& value, Object& table);
	bool setobject(const SymbolKeyType& id,
		Object& value, Object& table);

	bool pushobject(const SymbolKeyType& id, const std::string& value,
		Object& table);
	bool pushobject(const SymbolKeyType& id,
		const SymbolArrayType& value, Object& table);
	bool pushobject(const SymbolKeyType& id,
		const SymbolHashType& value, Object& table);
	bool pushobject(const SymbolKeyType& id,
		Object& value, Object& table);

	bool getobjectforget(const SymbolKeyType& id, Object& table,
		Object::PtrType& rptr);
	bool getobjectforset(const SymbolKeyType& id, Object& table,
		Object::PtrType& rptr);
	size_t getarrayindex(const std::string& expr);

	size_t findclosebracket(const SymbolKeyType& id, size_t obracket);
	bool istextnumber(const char* str);
};



} // end namespace TPT

#endif // include_libtpt_symbols_impl_h
