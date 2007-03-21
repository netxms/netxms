/*
 * parse_impl_rd.cxx
 *
 * The recursive descent part of the parser.  Since all variables
 * are treated as strings, numeric operations are expensive.
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
#include "libtpt_funcs.h"
#include <algorithm>
#include <sstream>
#include <iostream>

namespace TPT {


/*
 * The recursive descent parser will attempt to parse an expression
 * until it reaches a token it does not recognize.  The final token
 * will usually be a comma ',', or close parenthesis ')'.
 *
 */

// Level 0: Get things rolling
Object Parser_Impl::parse_level0(Object& left)
{
	if (left.gettype() != Object::type_token)
	{
		recorderror("Error, expected token");
		return left;
	}
	if (left.token().type == token_eof)
	{
		recorderror("Unexpected end of file");
		return left;
	}

	Object right = parse_level1(left);

	return right;
}

// Level 1: && || ^^
Object Parser_Impl::parse_level1(Object& left)
{
	Object op = parse_level2(left);
	if (left.gettype() != Object::type_scalar)
		return op;
	while ((op.gettype() == Object::type_token) &&
		(op.token().type == token_operator) &&
		((op.token().value == "&&") ||
		(op.token().value == "||") ||
		(op.token().value == "^^")))
	{
		Object right = lex.getstricttoken();
		// we have left and operation, but no right
		Object nextop = parse_level2(right);
		int64_t lwork = str2num(left.scalar().c_str()),
			rwork = str2num(right.scalar().c_str());
		if (op.token().value == "&&")
			lwork = lwork && rwork;
		else if (op.token().value == "||")
			lwork = lwork || rwork;
		else if (op.token().value == "^^")
			lwork = !lwork ^ !rwork;
		num2str(lwork, left.scalar());
		// read the next operator/terminator
		op = nextop;
	}

	return op;
}

// Level 2: == != < > <= >=
Object Parser_Impl::parse_level2(Object& left)
{
	Object op = parse_level3(left);
	if (left.gettype() != Object::type_scalar)
		return op;
	while ((op.gettype() == Object::type_token) &&
		(op.token().type == token_relop))
	{
		Object right = lex.getstricttoken();
		// we have left and operation, but no right
		Object nextop = parse_level3(right);
		int64_t lwork = str2num(left.scalar().c_str()),
			rwork = str2num(right.scalar().c_str());
		if (op.token().value == "==")
			lwork = lwork == rwork;
		else if (op.token().value == "!=")
			lwork = lwork != rwork;
		else if (op.token().value == "<")
			lwork = lwork < rwork;
		else if (op.token().value == ">")
			lwork = lwork > rwork;
		else if (op.token().value == "<=")
			lwork = lwork <= rwork;
		else if (op.token().value == ">=")
			lwork = lwork >= rwork;
		num2str(lwork, left.scalar());
		// read the next operator/terminator
		op = nextop;
	}
	return op;
}

// Level 3: + -
Object Parser_Impl::parse_level3(Object& left)
{
	Object op = parse_level4(left);
	if (left.gettype() != Object::type_scalar)
		return op;
	while ((op.gettype() == Object::type_token) &&
		(op.token().type == token_operator) &&
		((op.token().value[0] == '+') ||
		(op.token().value[0] == '-')))
	{
		Object right = lex.getstricttoken();
		// we have left and operation, but no right
		Object nextop = parse_level4(right);
		int64_t lwork = str2num(left.scalar().c_str()),
			rwork = str2num(right.scalar().c_str());
		switch (op.token().value[0])
		{
		case '+':
			lwork+= rwork;
			break;
		case '-':
			lwork-= rwork;
			break;
		}
		num2str(lwork, left.scalar());
		// read the next operator/terminator
		op = nextop;
	}
	return op;
}

// Level 4: * / %
Object Parser_Impl::parse_level4(Object& left)
{
	Object op = parse_level5(left);
	if (left.gettype() != Object::type_scalar)
		return op;
	while ((op.gettype() == Object::type_token) &&
		(op.token().type == token_operator) &&
		((op.token().value[0] == '*') ||
		(op.token().value[0] == '/') ||
		(op.token().value[0] == '%')))
	{
		Object right = lex.getstricttoken();
		// we have left and operation, but no right
		Object nextop = parse_level5(right);
		int64_t lwork = str2num(left.scalar().c_str()),
			rwork = str2num(right.scalar().c_str());
		switch (op.token().value[0])
		{
		case '*':
			lwork*= rwork;
			break;
		case '/':
			lwork/= rwork;
			break;
		case '%':
			lwork%= rwork;
			break;
		}
		num2str(lwork, left.scalar());
		// read the next operator/terminator
		op = nextop;
	}
	return op;
}

// Level 5: + - ! (unary operators)
Object Parser_Impl::parse_level5(Object& left)
{
	Object right;
	if ((left.token().type == token_operator) &&
		((left.token().value[0] == '+') ||
		(left.token().value[0] == '-') ||
		(left.token().value[0] == '!')))
	{
		Object temp = lex.getstricttoken();
		right = parse_level6(temp);
		if (temp.gettype() != Object::type_scalar)
			return temp;
		int64_t work = str2num(temp.scalar().c_str());
		if (left.token().value[0] == '!')
			work = !work;
		else if (left.token().value[0] == '-')
			work = -work;
		// else + ignore (forces string to 0)
		left = Object::type_scalar;
		num2str(work, left.scalar());
	}
	else
		right = parse_level6(left);
	return right;
}

// Level 6: ( )
Object Parser_Impl::parse_level6(Object& left)
{
	Object right = parse_level7(left);
	if ((left.gettype() == Object::type_token) &&
		(left.token().type == token_openparen))
	{
		left = right;
		right = parse_level0(left);
		if (right.token().type != token_closeparen)
			recorderror("Syntax error, expected )");
		else
			// get token after close paren
			right = lex.getstricttoken();
	}
	return right;
}

// Level 7: literals $id @macro
Object Parser_Impl::parse_level7(Object& left)
{
	switch (left.token().type) {
	case token_id:
		// create copy of left.value so key is not overwritten
		{
			Object::PtrType objptr;
			if (symbols.imp->getobjectforget(left.token().value,
				symbols.imp->symbols, objptr))
			{
				// Object does not exist
				left = Object::type_scalar; // empty string
			}
			else
				left = (*objptr.get());
		}
		break;
	case token_usermacro:
		{
			std::stringstream tempstr;
			if (isuserfunc(left.token().value))
				userfunc(left.token().value, &tempstr);
			else
				user_macro(left.token().value, &tempstr);
			left = tempstr.str();
		}
		break;

	case token_compare:
		left = parse_compare().value;
		break;
	case token_empty:
		left = parse_empty().value;
		break;
	case token_isarray:
		left = parse_isarray().value;
		break;
	case token_ishash:
		left = parse_ishash().value;
		break;
	case token_isscalar:
		left = parse_isscalar().value;
		break;
	case token_rand:
		left = parse_rand().value;
		break;
	case token_size:
		left = parse_size().value;
		break;

	case token_integer:
	case token_string:
		{
			// Must use temp to convert left to scalar
			std::string temp(left.token().value);
			left = temp;
		}
		break;
	default:
		// This may be a unary operator or parenthesis
		break;
	}
	// Return next available token
	return lex.getstricttoken();
}

} // end namespace TPT
