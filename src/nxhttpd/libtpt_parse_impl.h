/*
 * parse_impl.h
 *
 * Parser Implementation
 *
 * libtpt 1.30
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
 */

#ifndef include_libtpt_parse_impl_h
#define include_libtpt_parse_impl_h

#include <libtpt/object.h>
#include <libtpt/tpttypes.h>
#include "libtpt_conf.h"
#include "libtpt_lexical.h"
#include "libtpt_macro.h"
#include <libtpt/parse.h>

namespace TPT {

enum loop_control { loop_ign = 0, loop_next, loop_last };

typedef std::vector< std::string > IncludeList;

class Parser_Impl {
public:
	Buffer* allocbuf;
	Lex lex;
	unsigned level;	// block level
	unsigned looplevel;
	Symbols localsymmap;
	Symbols& symbols;	// reference to whatever symbol map is in use
	MacroList localmacros;
	MacroList& macros;	// reference to whatever macro list is in use
	FunctionList localfuncs;
	FunctionList& funcs;
	ErrorList errlist;
	IncludeList localinclist;
	IncludeList& inclist;
	bool isseeded;
	loop_control loop_cmd;

	// kiss_vars are used for pseudo-random number generation
	unsigned kiss_x;
	unsigned kiss_y;
	unsigned kiss_z;
	unsigned kiss_w;
	unsigned kiss_carry;
	unsigned kiss_k;
	unsigned kiss_m;
	unsigned kiss_seed;

	Parser_Impl(Buffer& buf) : allocbuf(0), lex(buf), level(0),
		looplevel(0), symbols(localsymmap), macros(localmacros),
		funcs(localfuncs), inclist(localinclist), isseeded(false)
	{ installfuncs(); }

	Parser_Impl(Buffer& buf, Symbols& sm) : allocbuf(0), lex(buf),
		level(0), looplevel(0), symbols(sm),
		macros(localmacros), funcs(localfuncs), inclist(localinclist),
		isseeded(false)
	{ installfuncs(); }

	Parser_Impl(const char* filename) : allocbuf(new Buffer(filename)),
		lex(*allocbuf), level(0), looplevel(0), symbols(localsymmap),
		macros(localmacros), funcs(localfuncs), inclist(localinclist),
		isseeded(false)
	{ installfuncs(); }

	Parser_Impl(const char* filename, Symbols& sm) : 
		allocbuf(new Buffer(filename)), lex(*allocbuf), level(0),
		looplevel(0), symbols(sm),
		macros(localmacros), funcs(localfuncs), inclist(localinclist),
		isseeded(false)
	{ installfuncs(); }

	Parser_Impl(const char* buffer, unsigned long size) :
		allocbuf(new Buffer(buffer, size)), lex(*allocbuf), level(0),
		looplevel(0), symbols(localsymmap), macros(localmacros),
		funcs(localfuncs), inclist(localinclist), isseeded(false)
	{ installfuncs(); }

	Parser_Impl(const char* buffer, unsigned long size, Symbols& sm) :
		allocbuf(new Buffer(buffer, size)), lex(*allocbuf), level(0),
		looplevel(0), symbols(sm),
		macros(localmacros), funcs(localfuncs), inclist(localinclist),
		isseeded(false)
	{ installfuncs(); }

	Parser_Impl(Buffer& buf, Symbols& sm, MacroList& ml, FunctionList& fns,
			IncludeList& il) :
		allocbuf(0), lex(buf), level(0), looplevel(0), symbols(sm), macros(ml),
		funcs(fns), inclist(il), isseeded(false)
	{ }
	
	~Parser_Impl() { if (allocbuf) delete allocbuf; }

	void installfuncs();
	
	void recorderror(const std::string& desc, const Token<>* neartoken=0);
	bool getnextparam(std::string& value);
	bool getnextstrict(Token<>& target);
	bool getparamlist(Object& pl);
	bool getidparamlist(std::string& id, Object& pl);
	bool getidlist(std::vector< std::string >& ids);

	bool pass1(std::ostream* os);

	Object parse_level0(Object& left);
	Object parse_level1(Object& left);
	Object parse_level2(Object& left);
	Object parse_level3(Object& left);
	Object parse_level4(Object& left);
	Object parse_level5(Object& left);
	Object parse_level6(Object& left);
	Object parse_level7(Object& left);

	void srandom32();
	unsigned int random32();
	Token<> parse_rand();		// generate pseudorandom number

	Token<> parse_empty();		// check emptiness of symbol
	Token<> parse_size();		// get size of array
	Token<> parse_compare();	// Compare two strings

	Token<> parse_sum();		// get sum of list
	Token<> parse_avg();

	Token<> parse_isarray();	// check if array
	Token<> parse_ishash();		// check if hash
	Token<> parse_isscalar();	// check if scalar

	void parse_main(std::ostream* os);
	void parse_block(std::ostream* os);
	bool parse_loopblock(std::ostream* os);
	void parse_dotoken(std::ostream* os, Token<> tok);
	void ignore_block();

	void parse_include(std::ostream* os);
	void parse_includetext(std::ostream* os);
	void parse_using();
	void parse_if(std::ostream* os);
	bool parse_ifexpr(std::ostream* os);
	void parse_foreach(std::ostream* os);
	void parse_while(std::ostream* os);
	bool parse_whileexpr(std::ostream* os);
	void parse_set();
	void parse_setif();
	void parse_unset();
	void parse_push();
	void parse_pop();
	void parse_keys();

	bool isuserfunc(const std::string& name);
	void userfunc(const std::string& name, std::ostream* os);
	
	void parse_macro();
	void user_macro(const std::string& name, std::ostream* os);
};

template<typename T>
struct cntguard {
	T& refcnt;
	cntguard(T& counter) : refcnt(counter) { ++refcnt; }
	~cntguard() { --refcnt; }
};

} // end namespace TPT

#endif // include_libtpt_parse_impl_h
