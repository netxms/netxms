/* A Bison parser, made by GNU Bison 2.1.  */

/* Skeleton parser for Yacc-like parsing with Bison,
   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.  */

/* As a special exception, when this file is copied by Bison into a
   Bison output file, you may use that output file without restriction.
   This special exception was added by the Free Software Foundation
   in version 1.24 of Bison.  */

/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     T_BREAK = 258,
     T_CONTINUE = 259,
     T_DO = 260,
     T_ELSE = 261,
     T_EXIT = 262,
     T_FOR = 263,
     T_IF = 264,
     T_NULL = 265,
     T_PRINT = 266,
     T_RETURN = 267,
     T_SUB = 268,
     T_TYPE_INT32 = 269,
     T_TYPE_INT64 = 270,
     T_TYPE_REAL = 271,
     T_TYPE_STRING = 272,
     T_TYPE_UINT32 = 273,
     T_TYPE_UINT64 = 274,
     T_USE = 275,
     T_WHILE = 276,
     T_COMPOUND_IDENTIFIER = 277,
     T_IDENTIFIER = 278,
     T_STRING = 279,
     T_INT32 = 280,
     T_UINT32 = 281,
     T_INT64 = 282,
     T_UINT64 = 283,
     T_REAL = 284,
     T_OR = 285,
     T_AND = 286,
     T_IMATCH = 287,
     T_MATCH = 288,
     T_ILIKE = 289,
     T_LIKE = 290,
     T_EQ = 291,
     T_NE = 292,
     T_GE = 293,
     T_LE = 294,
     T_RSHIFT = 295,
     T_LSHIFT = 296,
     NEGATE = 297,
     T_DEC = 298,
     T_INC = 299,
     T_REF = 300,
     T_POST_DEC = 301,
     T_POST_INC = 302
   };
#endif
/* Tokens.  */
#define T_BREAK 258
#define T_CONTINUE 259
#define T_DO 260
#define T_ELSE 261
#define T_EXIT 262
#define T_FOR 263
#define T_IF 264
#define T_NULL 265
#define T_PRINT 266
#define T_RETURN 267
#define T_SUB 268
#define T_TYPE_INT32 269
#define T_TYPE_INT64 270
#define T_TYPE_REAL 271
#define T_TYPE_STRING 272
#define T_TYPE_UINT32 273
#define T_TYPE_UINT64 274
#define T_USE 275
#define T_WHILE 276
#define T_COMPOUND_IDENTIFIER 277
#define T_IDENTIFIER 278
#define T_STRING 279
#define T_INT32 280
#define T_UINT32 281
#define T_INT64 282
#define T_UINT64 283
#define T_REAL 284
#define T_OR 285
#define T_AND 286
#define T_IMATCH 287
#define T_MATCH 288
#define T_ILIKE 289
#define T_LIKE 290
#define T_EQ 291
#define T_NE 292
#define T_GE 293
#define T_LE 294
#define T_RSHIFT 295
#define T_LSHIFT 296
#define NEGATE 297
#define T_DEC 298
#define T_INC 299
#define T_REF 300
#define T_POST_DEC 301
#define T_POST_INC 302




#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
#line 27 "parser.y"
typedef union YYSTYPE {
	LONG valInt32;
	DWORD valUInt32;
	INT64 valInt64;
	QWORD valUInt64;
	char *valStr;
	double valReal;
	NXSL_Value *pConstant;
	NXSL_Instruction *pInstruction;
} YYSTYPE;
/* Line 1447 of yacc.c.  */
#line 143 "parser.tab.hpp"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif





