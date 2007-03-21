/*
 * lexical.h
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

#ifndef include_libtpt_lexical_h
#define include_libtpt_lexical_h

#include <libtpt/token.h>
#include <libtpt/buffer.h>

#include <iostream>

namespace TPT {

class Buffer;

class Lex {
public:
	Lex(Buffer& b) : buf_(b), lineno_(1), column_(1), ignoreindent_(false),
		ignoreblankline_(false) {}

	Token<> getloosetoken();
	Token<> getstricttoken();
	Token<> getspecialtoken();
	void unget(const Token<>& tok);
	unsigned getlineno() const;
	void setlineno(unsigned newline);
	unsigned long index() const;
	bool seek(unsigned long index);

	///! Extract an unparsed brace enclosed {} block
	bool getblock(std::string& block, unsigned& lineno);
	///! Ignore the next brace enclosed {} block
	bool ignoreblock();

	// Handle spaces and comments after braces {}
	void handlebraceignore(Token<>&);

	char safeget()
	{ if (!buf_) return 0; ++column_; return buf_.getnextchar(); }
	void safeunget()
	{ --column_; buf_.unget(); }
	Token<>::en checkreserved(const char* str);
	void buildidentifier(std::string& value);
	void buildnumber(std::string& value);
	void buildrawtext(std::string& value);
	void getwhitespace(std::string& value);

	void getidname(Token<>& t);
	void getclosedidname(Token<>& t);
	void getbracketexpr(Token<>& t);

	void getstring(Token<>& t);
	void getsqstring(Token<>& t);

	bool getreturn(char c, std::string& cr);
	bool ignorereturn(char c);

	void buildcomment(Token<>& t);
	void newline();

private:
	Buffer& buf_;
	unsigned lineno_;
	unsigned column_;
	bool ignoreindent_;
	bool ignoreblankline_;

	Lex();
};


} // end namespace TPT

#endif // include_libtpt_lexical_h
