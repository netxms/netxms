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

/* Written by Richard Stallman by simplifying the original so called
   ``semantic'' parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.1"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 1

/* Using locations.  */
#define YYLSP_NEEDED 0



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




/* Copy the first part of user declarations.  */
#line 1 "parser.y"


#pragma warning(disable : 4065 4102)

#define YYERROR_VERBOSE
#define YYINCLUDED_STDLIB_H
#define YYDEBUG			1

#include <nms_common.h>
#include "libnxsl.h"
#include "parser.tab.hpp"

void yyerror(NXSL_Lexer *pLexer, NXSL_Compiler *pCompiler,
             NXSL_Program *pScript, char *pszText);
int yylex(YYSTYPE *lvalp, NXSL_Lexer *pLexer);



/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 1
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* Enabling the token table.  */
#ifndef YYTOKEN_TABLE
# define YYTOKEN_TABLE 0
#endif

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
/* Line 196 of yacc.c.  */
#line 208 "parser.tab.cpp"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 219 of yacc.c.  */
#line 220 "parser.tab.cpp"

#if ! defined (YYSIZE_T) && defined (__SIZE_TYPE__)
# define YYSIZE_T __SIZE_TYPE__
#endif
#if ! defined (YYSIZE_T) && defined (size_t)
# define YYSIZE_T size_t
#endif
#if ! defined (YYSIZE_T) && (defined (__STDC__) || defined (__cplusplus))
# include <stddef.h> /* INFRINGES ON USER NAME SPACE */
# define YYSIZE_T size_t
#endif
#if ! defined (YYSIZE_T)
# define YYSIZE_T unsigned int
#endif

#ifndef YY_
# if YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

#if ! defined (yyoverflow) || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if defined (__STDC__) || defined (__cplusplus)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#     define YYINCLUDED_STDLIB_H
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning. */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2005 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM ((YYSIZE_T) -1)
#  endif
#  ifdef __cplusplus
extern "C" {
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if (! defined (malloc) && ! defined (YYINCLUDED_STDLIB_H) \
	&& (defined (__STDC__) || defined (__cplusplus)))
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if (! defined (free) && ! defined (YYINCLUDED_STDLIB_H) \
	&& (defined (__STDC__) || defined (__cplusplus)))
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifdef __cplusplus
}
#  endif
# endif
#endif /* ! defined (yyoverflow) || YYERROR_VERBOSE */


#if (! defined (yyoverflow) \
     && (! defined (__cplusplus) \
	 || (defined (YYSTYPE_IS_TRIVIAL) && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  short int yyss;
  YYSTYPE yyvs;
  };

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (short int) + sizeof (YYSTYPE))			\
      + YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined (__GNUC__) && 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  YYSIZE_T yyi;				\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (0)
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack)					\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack, Stack, yysize);				\
	Stack = &yyptr->Stack;						\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (0)

#endif

#if defined (__STDC__) || defined (__cplusplus)
   typedef signed char yysigned_char;
#else
   typedef short int yysigned_char;
#endif

/* YYFINAL -- State number of the termination state. */
#define YYFINAL  50
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   718

/* YYNTOKENS -- Number of terminals. */
#define YYNTOKENS  68
/* YYNNTS -- Number of nonterminals. */
#define YYNNTS  36
/* YYNRULES -- Number of rules. */
#define YYNRULES  104
/* YYNRULES -- Number of states. */
#define YYNSTATES  172

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   302

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const unsigned char yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    54,     2,     2,     2,    53,    36,     2,
      67,    63,    51,    49,    64,    50,    31,    52,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,    62,
      43,    30,    44,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,    35,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    65,    34,    66,    55,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    32,    33,    37,    38,    39,
      40,    41,    42,    45,    46,    47,    48,    56,    57,    58,
      59,    60,    61
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const unsigned short int yyprhs[] =
{
       0,     0,     3,     5,     7,    10,    12,    14,    16,    20,
      22,    24,    25,    31,    34,    36,    37,    42,    44,    48,
      51,    54,    56,    58,    60,    63,    65,    67,    71,    75,
      79,    82,    85,    88,    91,    94,    97,   100,   104,   108,
     112,   116,   120,   124,   128,   132,   136,   140,   144,   148,
     152,   156,   160,   164,   168,   172,   176,   180,   184,   188,
     192,   194,   196,   198,   200,   202,   207,   209,   211,   213,
     215,   217,   219,   222,   224,   226,   228,   231,   234,   236,
     238,   240,   242,   243,   250,   252,   255,   256,   260,   261,
     262,   270,   271,   280,   284,   287,   291,   293,   296,   298,
     300,   302,   304,   306,   308
};

/* YYRHS -- A `-1'-separated list of the rules' RHS. */
static const yysigned_char yyrhs[] =
{
      69,     0,    -1,    70,    -1,    83,    -1,    71,    70,    -1,
      71,    -1,    74,    -1,    72,    -1,    20,    73,    62,    -1,
      23,    -1,    22,    -1,    -1,    13,   102,    75,    76,    79,
      -1,    77,    63,    -1,    63,    -1,    -1,    23,    78,    64,
      77,    -1,    23,    -1,    65,    80,    66,    -1,    65,    66,
      -1,    81,    80,    -1,    81,    -1,    82,    -1,    79,    -1,
      83,    62,    -1,    87,    -1,    62,    -1,    67,    83,    63,
      -1,    23,    30,    83,    -1,    83,    59,    23,    -1,    50,
      83,    -1,    54,    83,    -1,    55,    83,    -1,    58,    23,
      -1,    57,    23,    -1,    23,    58,    -1,    23,    57,    -1,
      83,    49,    83,    -1,    83,    50,    83,    -1,    83,    51,
      83,    -1,    83,    52,    83,    -1,    83,    53,    83,    -1,
      83,    40,    83,    -1,    83,    39,    83,    -1,    83,    38,
      83,    -1,    83,    37,    83,    -1,    83,    41,    83,    -1,
      83,    42,    83,    -1,    83,    43,    83,    -1,    83,    46,
      83,    -1,    83,    44,    83,    -1,    83,    45,    83,    -1,
      83,    36,    83,    -1,    83,    34,    83,    -1,    83,    35,
      83,    -1,    83,    33,    83,    -1,    83,    32,    83,    -1,
      83,    48,    83,    -1,    83,    47,    83,    -1,    83,    31,
      83,    -1,    84,    -1,   100,    -1,    85,    -1,    23,    -1,
     103,    -1,    86,    67,    83,    63,    -1,    14,    -1,    15,
      -1,    18,    -1,    19,    -1,    16,    -1,    17,    -1,    88,
      62,    -1,    90,    -1,    98,    -1,    95,    -1,     4,    62,
      -1,    89,    83,    -1,    89,    -1,     7,    -1,    12,    -1,
      11,    -1,    -1,     9,    67,    83,    63,    91,    92,    -1,
      81,    -1,    81,    93,    -1,    -1,     6,    94,    81,    -1,
      -1,    -1,    21,    96,    67,    83,    63,    97,    81,    -1,
      -1,     5,    99,    81,    21,    67,    83,    63,    62,    -1,
     102,   101,    63,    -1,   102,    63,    -1,    83,    64,   101,
      -1,    83,    -1,    23,    67,    -1,    24,    -1,    25,    -1,
      26,    -1,    27,    -1,    28,    -1,    29,    -1,    10,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const unsigned short int yyrline[] =
{
       0,    95,    95,    96,   113,   114,   118,   119,   123,   130,
     131,   136,   135,   153,   154,   159,   158,   163,   170,   171,
     178,   179,   183,   184,   188,   192,   193,   200,   201,   205,
     209,   213,   217,   221,   225,   229,   233,   237,   241,   245,
     249,   253,   257,   261,   265,   269,   273,   277,   281,   285,
     289,   293,   297,   301,   305,   309,   313,   317,   321,   325,
     329,   333,   334,   335,   339,   346,   354,   358,   362,   366,
     370,   374,   381,   382,   383,   384,   385,   401,   405,   413,
     417,   421,   429,   428,   436,   440,   448,   447,   457,   461,
     456,   474,   473,   485,   489,   496,   497,   501,   508,   513,
     517,   521,   525,   529,   533
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals. */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "T_BREAK", "T_CONTINUE", "T_DO",
  "T_ELSE", "T_EXIT", "T_FOR", "T_IF", "T_NULL", "T_PRINT", "T_RETURN",
  "T_SUB", "T_TYPE_INT32", "T_TYPE_INT64", "T_TYPE_REAL", "T_TYPE_STRING",
  "T_TYPE_UINT32", "T_TYPE_UINT64", "T_USE", "T_WHILE",
  "T_COMPOUND_IDENTIFIER", "T_IDENTIFIER", "T_STRING", "T_INT32",
  "T_UINT32", "T_INT64", "T_UINT64", "T_REAL", "'='", "'.'", "T_OR",
  "T_AND", "'|'", "'^'", "'&'", "T_IMATCH", "T_MATCH", "T_ILIKE", "T_LIKE",
  "T_EQ", "T_NE", "'<'", "'>'", "T_GE", "T_LE", "T_RSHIFT", "T_LSHIFT",
  "'+'", "'-'", "'*'", "'/'", "'%'", "'!'", "'~'", "NEGATE", "T_DEC",
  "T_INC", "T_REF", "T_POST_DEC", "T_POST_INC", "';'", "')'", "','", "'{'",
  "'}'", "'('", "$accept", "Script", "Module", "ModuleComponent",
  "UseStatement", "AnyIdentifier", "Function", "@1",
  "ParameterDeclaration", "IdentifierList", "@2", "Block", "StatementList",
  "StatementOrBlock", "Statement", "Expression", "Operand", "TypeCast",
  "BuiltinType", "BuiltinStatement", "SimpleStatement",
  "SimpleStatementKeyword", "IfStatement", "@3", "IfBody", "ElseStatement",
  "@4", "WhileStatement", "@5", "@6", "DoStatement", "@7", "FunctionCall",
  "ParameterList", "FunctionName", "Constant", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const unsigned short int yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
      61,    46,   285,   286,   124,    94,    38,   287,   288,   289,
     290,   291,   292,    60,    62,   293,   294,   295,   296,    43,
      45,    42,    47,    37,    33,   126,   297,   298,   299,   300,
     301,   302,    59,    41,    44,   123,   125,    40
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const unsigned char yyr1[] =
{
       0,    68,    69,    69,    70,    70,    71,    71,    72,    73,
      73,    75,    74,    76,    76,    78,    77,    77,    79,    79,
      80,    80,    81,    81,    82,    82,    82,    83,    83,    83,
      83,    83,    83,    83,    83,    83,    83,    83,    83,    83,
      83,    83,    83,    83,    83,    83,    83,    83,    83,    83,
      83,    83,    83,    83,    83,    83,    83,    83,    83,    83,
      83,    84,    84,    84,    84,    85,    86,    86,    86,    86,
      86,    86,    87,    87,    87,    87,    87,    88,    88,    89,
      89,    89,    91,    90,    92,    92,    94,    93,    96,    97,
      95,    99,    98,   100,   100,   101,   101,   102,   103,   103,
     103,   103,   103,   103,   103
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const unsigned char yyr2[] =
{
       0,     2,     1,     1,     2,     1,     1,     1,     3,     1,
       1,     0,     5,     2,     1,     0,     4,     1,     3,     2,
       2,     1,     1,     1,     2,     1,     1,     3,     3,     3,
       2,     2,     2,     2,     2,     2,     2,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       1,     1,     1,     1,     1,     4,     1,     1,     1,     1,
       1,     1,     2,     1,     1,     1,     2,     2,     1,     1,
       1,     1,     0,     6,     1,     2,     0,     3,     0,     0,
       7,     0,     8,     3,     2,     3,     1,     2,     1,     1,
       1,     1,     1,     1,     1
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const unsigned char yydefact[] =
{
       0,   104,     0,    66,    67,    70,    71,    68,    69,     0,
      63,    98,    99,   100,   101,   102,   103,     0,     0,     0,
       0,     0,     0,     0,     2,     5,     7,     6,     3,    60,
      62,     0,    61,     0,    64,     0,    11,    10,     9,     0,
       0,    36,    35,    97,    30,    31,    32,    34,    33,     0,
       1,     4,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    94,    96,     0,
       0,     8,    28,    27,    59,    56,    55,    53,    54,    52,
      45,    44,    43,    42,    46,    47,    48,    50,    51,    49,
      58,    57,    37,    38,    39,    40,    41,    29,     0,     0,
      93,    15,    14,     0,     0,    65,    95,     0,     0,    12,
      13,     0,     0,    91,    79,     0,    81,    80,    88,    26,
      19,    23,     0,    21,    22,     0,    25,     0,    78,    73,
      75,    74,    16,    76,     0,     0,     0,    18,    20,    24,
      72,    77,     0,     0,     0,     0,    82,     0,     0,     0,
      89,     0,    84,    83,     0,     0,    86,    85,    90,    92,
       0,    87
};

/* YYDEFGOTO[NTERM-NUM]. */
static const short int yydefgoto[] =
{
      -1,    23,    24,    25,    26,    39,    27,    80,   113,   114,
     117,   131,   132,   133,   134,   135,    29,    30,    31,   136,
     137,   138,   139,   159,   163,   167,   170,   140,   146,   164,
     141,   144,    32,    79,    33,    34
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -140
static const short int yypact[] =
{
     172,  -140,   -17,  -140,  -140,  -140,  -140,  -140,  -140,   -19,
     -28,  -140,  -140,  -140,  -140,  -140,  -140,   238,   238,   238,
     -16,     1,   238,     8,  -140,     3,  -140,  -140,   506,  -140,
    -140,   -41,  -140,   218,  -140,   -40,  -140,  -140,  -140,   -34,
     238,  -140,  -140,  -140,   -27,   -27,   -27,  -140,  -140,   309,
    -140,  -140,   238,   238,   238,   238,   238,   238,   238,   238,
     238,   238,   238,   238,   238,   238,   238,   238,   238,   238,
     238,   238,   238,   238,   238,    19,   238,  -140,   275,   -20,
     -22,  -140,   506,  -140,   534,   561,   587,   612,   636,   659,
      67,    67,    67,    67,    67,    67,   -38,   -38,   -38,   -38,
     -15,   -15,    91,    91,   -27,   -27,   -27,  -140,   342,   238,
    -140,   -18,  -140,   -14,   -13,  -140,  -140,    11,    74,  -140,
    -140,    23,    15,  -140,  -140,    13,  -140,  -140,  -140,  -140,
    -140,  -140,    16,   152,  -140,   474,  -140,    25,   238,  -140,
    -140,  -140,  -140,  -140,   152,   238,    27,  -140,  -140,  -140,
    -140,   506,    26,   375,   238,    29,  -140,   408,   238,   152,
    -140,   441,    42,  -140,   152,    43,  -140,  -140,  -140,  -140,
     152,  -140
};

/* YYPGOTO[NTERM-NUM].  */
static const short int yypgoto[] =
{
    -140,  -140,    24,  -140,  -140,  -140,  -140,  -140,  -140,     2,
    -140,    -9,   -26,  -139,  -140,     0,  -140,  -140,  -140,  -140,
    -140,  -140,  -140,  -140,  -140,  -140,  -140,  -140,  -140,  -140,
    -140,  -140,  -140,    -3,   106,  -140
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -18
static const short int yytable[] =
{
      28,   111,    40,    37,    38,   152,    35,    47,    50,    68,
      69,    70,    71,    72,    73,    74,     2,    44,    45,    46,
     162,    75,    49,     9,    48,   168,    76,    43,    81,    41,
      42,   171,    75,    78,    70,    71,    72,    73,    74,    43,
      82,   112,   107,   110,    75,   -17,   111,   155,   166,    51,
     120,   118,    84,    85,    86,    87,    88,    89,    90,    91,
      92,    93,    94,    95,    96,    97,    98,    99,   100,   101,
     102,   103,   104,   105,   106,   121,   108,   143,   122,   123,
     145,   124,   147,   125,     1,   126,   127,   150,     3,     4,
       5,     6,     7,     8,   154,   128,   158,    10,    11,    12,
      13,    14,    15,    16,   119,   169,   116,   148,    36,    78,
      64,    65,    66,    67,    68,    69,    70,    71,    72,    73,
      74,     0,     0,   142,    17,     0,    75,     0,    18,    19,
       0,    20,    21,     0,     0,     0,   129,     0,   151,   118,
     130,    22,    72,    73,    74,   153,     0,     0,     0,     0,
      75,     0,     0,     0,   157,     0,   122,   123,   161,   124,
       0,   125,     1,   126,   127,     0,     3,     4,     5,     6,
       7,     8,     0,   128,     0,    10,    11,    12,    13,    14,
      15,    16,     1,     0,     0,     2,     3,     4,     5,     6,
       7,     8,     9,     0,     0,    10,    11,    12,    13,    14,
      15,    16,    17,     0,     0,     0,    18,    19,     0,    20,
      21,     0,     0,     0,   129,     0,     0,   118,     0,    22,
       0,     0,    17,     0,     0,     0,    18,    19,     1,    20,
      21,     0,     3,     4,     5,     6,     7,     8,     0,    22,
       0,    10,    11,    12,    13,    14,    15,    16,     1,     0,
       0,     0,     3,     4,     5,     6,     7,     8,     0,     0,
       0,    10,    11,    12,    13,    14,    15,    16,    17,     0,
       0,     0,    18,    19,     0,    20,    21,     0,     0,     0,
       0,    77,     0,     0,     0,    22,     0,     0,    17,     0,
       0,     0,    18,    19,     0,    20,    21,     0,     0,     0,
       0,     0,     0,     0,     0,    22,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,     0,
       0,     0,     0,     0,    75,     0,     0,     0,     0,   109,
      52,    53,    54,    55,    56,    57,    58,    59,    60,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    71,
      72,    73,    74,     0,     0,     0,     0,     0,    75,     0,
       0,     0,    83,    52,    53,    54,    55,    56,    57,    58,
      59,    60,    61,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    71,    72,    73,    74,     0,     0,     0,     0,
       0,    75,     0,     0,     0,   115,    52,    53,    54,    55,
      56,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,     0,
       0,     0,     0,     0,    75,     0,     0,     0,   156,    52,
      53,    54,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      73,    74,     0,     0,     0,     0,     0,    75,     0,     0,
       0,   160,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    71,    72,    73,    74,     0,     0,     0,     0,     0,
      75,     0,     0,     0,   165,    52,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    71,    72,    73,    74,     0,     0,
       0,     0,     0,    75,     0,     0,   149,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
       0,     0,     0,     0,     0,    75,    53,    54,    55,    56,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    71,    72,    73,    74,     0,     0,
       0,     0,     0,    75,    54,    55,    56,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    71,    72,    73,    74,     0,     0,     0,     0,     0,
      75,    55,    56,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    71,    72,    73,
      74,     0,     0,     0,     0,     0,    75,    56,    57,    58,
      59,    60,    61,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    71,    72,    73,    74,     0,     0,     0,     0,
       0,    75,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
       0,     0,     0,     0,     0,    75,    58,    59,    60,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    71,
      72,    73,    74,     0,     0,     0,     0,     0,    75
};

static const short int yycheck[] =
{
       0,    23,    30,    22,    23,   144,    23,    23,     0,    47,
      48,    49,    50,    51,    52,    53,    13,    17,    18,    19,
     159,    59,    22,    20,    23,   164,    67,    67,    62,    57,
      58,   170,    59,    33,    49,    50,    51,    52,    53,    67,
      40,    63,    23,    63,    59,    63,    23,    21,     6,    25,
      63,    65,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    71,    72,    73,    74,    64,    76,    62,     4,     5,
      67,     7,    66,     9,    10,    11,    12,    62,    14,    15,
      16,    17,    18,    19,    67,    21,    67,    23,    24,    25,
      26,    27,    28,    29,   113,    62,   109,   133,     2,   109,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    -1,    -1,   121,    50,    -1,    59,    -1,    54,    55,
      -1,    57,    58,    -1,    -1,    -1,    62,    -1,   138,    65,
      66,    67,    51,    52,    53,   145,    -1,    -1,    -1,    -1,
      59,    -1,    -1,    -1,   154,    -1,     4,     5,   158,     7,
      -1,     9,    10,    11,    12,    -1,    14,    15,    16,    17,
      18,    19,    -1,    21,    -1,    23,    24,    25,    26,    27,
      28,    29,    10,    -1,    -1,    13,    14,    15,    16,    17,
      18,    19,    20,    -1,    -1,    23,    24,    25,    26,    27,
      28,    29,    50,    -1,    -1,    -1,    54,    55,    -1,    57,
      58,    -1,    -1,    -1,    62,    -1,    -1,    65,    -1,    67,
      -1,    -1,    50,    -1,    -1,    -1,    54,    55,    10,    57,
      58,    -1,    14,    15,    16,    17,    18,    19,    -1,    67,
      -1,    23,    24,    25,    26,    27,    28,    29,    10,    -1,
      -1,    -1,    14,    15,    16,    17,    18,    19,    -1,    -1,
      -1,    23,    24,    25,    26,    27,    28,    29,    50,    -1,
      -1,    -1,    54,    55,    -1,    57,    58,    -1,    -1,    -1,
      -1,    63,    -1,    -1,    -1,    67,    -1,    -1,    50,    -1,
      -1,    -1,    54,    55,    -1,    57,    58,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    67,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    -1,
      -1,    -1,    -1,    -1,    59,    -1,    -1,    -1,    -1,    64,
      31,    32,    33,    34,    35,    36,    37,    38,    39,    40,
      41,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    -1,    -1,    -1,    -1,    -1,    59,    -1,
      -1,    -1,    63,    31,    32,    33,    34,    35,    36,    37,
      38,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    -1,    -1,    -1,    -1,
      -1,    59,    -1,    -1,    -1,    63,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    -1,
      -1,    -1,    -1,    -1,    59,    -1,    -1,    -1,    63,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    41,
      42,    43,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    -1,    -1,    -1,    -1,    -1,    59,    -1,    -1,
      -1,    63,    31,    32,    33,    34,    35,    36,    37,    38,
      39,    40,    41,    42,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    53,    -1,    -1,    -1,    -1,    -1,
      59,    -1,    -1,    -1,    63,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    40,    41,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    -1,    -1,
      -1,    -1,    -1,    59,    -1,    -1,    62,    31,    32,    33,
      34,    35,    36,    37,    38,    39,    40,    41,    42,    43,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      -1,    -1,    -1,    -1,    -1,    59,    32,    33,    34,    35,
      36,    37,    38,    39,    40,    41,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    -1,    -1,
      -1,    -1,    -1,    59,    33,    34,    35,    36,    37,    38,
      39,    40,    41,    42,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    53,    -1,    -1,    -1,    -1,    -1,
      59,    34,    35,    36,    37,    38,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    -1,    -1,    -1,    -1,    -1,    59,    35,    36,    37,
      38,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    -1,    -1,    -1,    -1,
      -1,    59,    36,    37,    38,    39,    40,    41,    42,    43,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      -1,    -1,    -1,    -1,    -1,    59,    37,    38,    39,    40,
      41,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    -1,    -1,    -1,    -1,    -1,    59
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const unsigned char yystos[] =
{
       0,    10,    13,    14,    15,    16,    17,    18,    19,    20,
      23,    24,    25,    26,    27,    28,    29,    50,    54,    55,
      57,    58,    67,    69,    70,    71,    72,    74,    83,    84,
      85,    86,   100,   102,   103,    23,   102,    22,    23,    73,
      30,    57,    58,    67,    83,    83,    83,    23,    23,    83,
       0,    70,    31,    32,    33,    34,    35,    36,    37,    38,
      39,    40,    41,    42,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    53,    59,    67,    63,    83,   101,
      75,    62,    83,    63,    83,    83,    83,    83,    83,    83,
      83,    83,    83,    83,    83,    83,    83,    83,    83,    83,
      83,    83,    83,    83,    83,    83,    83,    23,    83,    64,
      63,    23,    63,    76,    77,    63,   101,    78,    65,    79,
      63,    64,     4,     5,     7,     9,    11,    12,    21,    62,
      66,    79,    80,    81,    82,    83,    87,    88,    89,    90,
      95,    98,    77,    62,    99,    67,    96,    66,    80,    62,
      62,    83,    81,    83,    67,    21,    63,    83,    67,    91,
      63,    83,    81,    92,    97,    63,     6,    93,    81,    62,
      94,    81
};

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */

#define YYFAIL		goto yyerrlab

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      yytoken = YYTRANSLATE (yychar);				\
      YYPOPSTACK;						\
      goto yybackup;						\
    }								\
  else								\
    {								\
      yyerror (pLexer, pCompiler, pScript, YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (0)


#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)				\
    do									\
      if (N)								\
	{								\
	  (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;	\
	  (Current).first_column = YYRHSLOC (Rhs, 1).first_column;	\
	  (Current).last_line    = YYRHSLOC (Rhs, N).last_line;		\
	  (Current).last_column  = YYRHSLOC (Rhs, N).last_column;	\
	}								\
      else								\
	{								\
	  (Current).first_line   = (Current).last_line   =		\
	    YYRHSLOC (Rhs, 0).last_line;				\
	  (Current).first_column = (Current).last_column =		\
	    YYRHSLOC (Rhs, 0).last_column;				\
	}								\
    while (0)
#endif


/* YY_LOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

#ifndef YY_LOCATION_PRINT
# if YYLTYPE_IS_TRIVIAL
#  define YY_LOCATION_PRINT(File, Loc)			\
     fprintf (File, "%d.%d-%d.%d",			\
              (Loc).first_line, (Loc).first_column,	\
              (Loc).last_line,  (Loc).last_column)
# else
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (&yylval, YYLEX_PARAM)
#else
# define YYLEX yylex (&yylval, pLexer)
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (0)

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)		\
do {								\
  if (yydebug)							\
    {								\
      YYFPRINTF (stderr, "%s ", Title);				\
      yysymprint (stderr,					\
                  Type, Value);	\
      YYFPRINTF (stderr, "\n");					\
    }								\
} while (0)

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yy_stack_print (short int *bottom, short int *top)
#else
static void
yy_stack_print (bottom, top)
    short int *bottom;
    short int *top;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (/* Nothing. */; bottom <= top; ++bottom)
    YYFPRINTF (stderr, " %d", *bottom);
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yy_reduce_print (int yyrule)
#else
static void
yy_reduce_print (yyrule)
    int yyrule;
#endif
{
  int yyi;
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu), ",
             yyrule - 1, yylno);
  /* Print the symbols being reduced, and their result.  */
  for (yyi = yyprhs[yyrule]; 0 <= yyrhs[yyi]; yyi++)
    YYFPRINTF (stderr, "%s ", yytname[yyrhs[yyi]]);
  YYFPRINTF (stderr, "-> %s\n", yytname[yyr1[yyrule]]);
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (Rule);		\
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif



#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined (__GLIBC__) && defined (_STRING_H)
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
static YYSIZE_T
#   if defined (__STDC__) || defined (__cplusplus)
yystrlen (const char *yystr)
#   else
yystrlen (yystr)
     const char *yystr;
#   endif
{
  const char *yys = yystr;

  while (*yys++ != '\0')
    continue;

  return yys - yystr - 1;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined (__GLIBC__) && defined (_STRING_H) && defined (_GNU_SOURCE)
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
#   if defined (__STDC__) || defined (__cplusplus)
yystpcpy (char *yydest, const char *yysrc)
#   else
yystpcpy (yydest, yysrc)
     char *yydest;
     const char *yysrc;
#   endif
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      size_t yyn = 0;
      char const *yyp = yystr;

      for (;;)
	switch (*++yyp)
	  {
	  case '\'':
	  case ',':
	    goto do_not_strip_quotes;

	  case '\\':
	    if (*++yyp != '\\')
	      goto do_not_strip_quotes;
	    /* Fall through.  */
	  default:
	    if (yyres)
	      yyres[yyn] = *yyp;
	    yyn++;
	    break;

	  case '"':
	    if (yyres)
	      yyres[yyn] = '\0';
	    return yyn;
	  }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

#endif /* YYERROR_VERBOSE */



#if YYDEBUG
/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yysymprint (FILE *yyoutput, int yytype, YYSTYPE *yyvaluep)
#else
static void
yysymprint (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  /* Pacify ``unused variable'' warnings.  */
  (void) yyvaluep;

  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);


# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# endif
  switch (yytype)
    {
      default:
        break;
    }
  YYFPRINTF (yyoutput, ")");
}

#endif /* ! YYDEBUG */
/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep)
#else
static void
yydestruct (yymsg, yytype, yyvaluep)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  /* Pacify ``unused variable'' warnings.  */
  (void) yyvaluep;

  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  switch (yytype)
    {

      default:
        break;
    }
}


/* Prevent warnings from -Wmissing-prototypes.  */

#ifdef YYPARSE_PARAM
# if defined (__STDC__) || defined (__cplusplus)
int yyparse (void *YYPARSE_PARAM);
# else
int yyparse ();
# endif
#else /* ! YYPARSE_PARAM */
#if defined (__STDC__) || defined (__cplusplus)
int yyparse (NXSL_Lexer *pLexer, NXSL_Compiler *pCompiler, NXSL_Program *pScript);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */






/*----------.
| yyparse.  |
`----------*/

#ifdef YYPARSE_PARAM
# if defined (__STDC__) || defined (__cplusplus)
int yyparse (void *YYPARSE_PARAM)
# else
int yyparse (YYPARSE_PARAM)
  void *YYPARSE_PARAM;
# endif
#else /* ! YYPARSE_PARAM */
#if defined (__STDC__) || defined (__cplusplus)
int
yyparse (NXSL_Lexer *pLexer, NXSL_Compiler *pCompiler, NXSL_Program *pScript)
#else
int
yyparse (pLexer, pCompiler, pScript)
    NXSL_Lexer *pLexer;
    NXSL_Compiler *pCompiler;
    NXSL_Program *pScript;
#endif
#endif
{
  /* The look-ahead symbol.  */
int yychar;

/* The semantic value of the look-ahead symbol.  */
YYSTYPE yylval;

/* Number of syntax errors so far.  */
int yynerrs;

  int yystate;
  int yyn;
  int yyresult;
  /* Number of tokens to shift before error messages enabled.  */
  int yyerrstatus;
  /* Look-ahead token as an internal (translated) token number.  */
  int yytoken = 0;

  /* Three stacks and their tools:
     `yyss': related to states,
     `yyvs': related to semantic values,
     `yyls': related to locations.

     Refer to the stacks thru separate pointers, to allow yyoverflow
     to reallocate them elsewhere.  */

  /* The state stack.  */
  short int yyssa[YYINITDEPTH];
  short int *yyss = yyssa;
  short int *yyssp;

  /* The semantic value stack.  */
  YYSTYPE yyvsa[YYINITDEPTH];
  YYSTYPE *yyvs = yyvsa;
  YYSTYPE *yyvsp;



#define YYPOPSTACK   (yyvsp--, yyssp--)

  YYSIZE_T yystacksize = YYINITDEPTH;

  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;


  /* When reducing, the number of symbols on the RHS of the reduced
     rule.  */
  int yylen;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY;		/* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  yyssp = yyss;
  yyvsp = yyvs;

  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed. so pushing a state here evens the stacks.
     */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack. Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	short int *yyss1 = yyss;


	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow (YY_("memory exhausted"),
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),

		    &yystacksize);

	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	short int *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyexhaustedlab;
	YYSTACK_RELOCATE (yyss);
	YYSTACK_RELOCATE (yyvs);

#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;


      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

/* Do appropriate processing given the current state.  */
/* Read a look-ahead token if we need one and don't already have one.  */
/* yyresume: */

  /* First try to decide what to do without reference to look-ahead token.  */

  yyn = yypact[yystate];
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a look-ahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid look-ahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yyn == 0 || yyn == YYTABLE_NINF)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Shift the look-ahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the token being shifted unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  *++yyvsp = yylval;


  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  yystate = yyn;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 3:
#line 97 "parser.y"
    {
	char szErrorText[256];

	// Add implicit return
	pScript->AddInstruction(new NXSL_Instruction(pLexer->GetCurrLine(), OPCODE_RETURN));

	// Add implicit main() function
	if (!pScript->AddFunction("main", 0, szErrorText))
	{
		pCompiler->Error(szErrorText);
		pLexer->SetErrorState();
	}
}
    break;

  case 8:
#line 124 "parser.y"
    {
	pScript->AddPreload((yyvsp[-1].valStr));
}
    break;

  case 11:
#line 136 "parser.y"
    {
		char szErrorText[256];

		if (!pScript->AddFunction((yyvsp[0].valStr), INVALID_ADDRESS, szErrorText))
		{
			pCompiler->Error(szErrorText);
			pLexer->SetErrorState();
		}
		free((yyvsp[0].valStr));
	}
    break;

  case 12:
#line 147 "parser.y"
    {
	pScript->AddInstruction(new NXSL_Instruction(pLexer->GetCurrLine(), OPCODE_RET_NULL));
}
    break;

  case 15:
#line 159 "parser.y"
    {
		pScript->AddInstruction(new NXSL_Instruction(pLexer->GetCurrLine(), OPCODE_BIND, (yyvsp[0].valStr)));
	}
    break;

  case 17:
#line 164 "parser.y"
    {
	pScript->AddInstruction(new NXSL_Instruction(pLexer->GetCurrLine(), OPCODE_BIND, (yyvsp[0].valStr)));
}
    break;

  case 19:
#line 172 "parser.y"
    {
	pScript->AddInstruction(new NXSL_Instruction(pLexer->GetCurrLine(), OPCODE_NOP));
}
    break;

  case 24:
#line 189 "parser.y"
    {
	pScript->AddInstruction(new NXSL_Instruction(pLexer->GetCurrLine(), OPCODE_POP, (int)1));
}
    break;

  case 26:
#line 194 "parser.y"
    {
	pScript->AddInstruction(new NXSL_Instruction(pLexer->GetCurrLine(), OPCODE_NOP));
}
    break;

  case 28:
#line 202 "parser.y"
    {
	pScript->AddInstruction(new NXSL_Instruction(pLexer->GetCurrLine(), OPCODE_SET, (yyvsp[-2].valStr)));
}
    break;

  case 29:
#line 206 "parser.y"
    {
	pScript->AddInstruction(new NXSL_Instruction(pLexer->GetCurrLine(), OPCODE_REFERENCE, (yyvsp[0].valStr)));
}
    break;

  case 30:
#line 210 "parser.y"
    {
	pScript->AddInstruction(new NXSL_Instruction(pLexer->GetCurrLine(), OPCODE_NEG));
}
    break;

  case 31:
#line 214 "parser.y"
    {
	pScript->AddInstruction(new NXSL_Instruction(pLexer->GetCurrLine(), OPCODE_NOT));
}
    break;

  case 32:
#line 218 "parser.y"
    {
	pScript->AddInstruction(new NXSL_Instruction(pLexer->GetCurrLine(), OPCODE_BIT_NOT));
}
    break;

  case 33:
#line 222 "parser.y"
    {
	pScript->AddInstruction(new NXSL_Instruction(pLexer->GetCurrLine(), OPCODE_INCP, (yyvsp[0].valStr)));
}
    break;

  case 34:
#line 226 "parser.y"
    {
	pScript->AddInstruction(new NXSL_Instruction(pLexer->GetCurrLine(), OPCODE_DECP, (yyvsp[0].valStr)));
}
    break;

  case 35:
#line 230 "parser.y"
    {
	pScript->AddInstruction(new NXSL_Instruction(pLexer->GetCurrLine(), OPCODE_INC, (yyvsp[-1].valStr)));
}
    break;

  case 36:
#line 234 "parser.y"
    {
	pScript->AddInstruction(new NXSL_Instruction(pLexer->GetCurrLine(), OPCODE_DEC, (yyvsp[-1].valStr)));
}
    break;

  case 37:
#line 238 "parser.y"
    { 
	pScript->AddInstruction(new NXSL_Instruction(pLexer->GetCurrLine(), OPCODE_ADD));
}
    break;

  case 38:
#line 242 "parser.y"
    {
	pScript->AddInstruction(new NXSL_Instruction(pLexer->GetCurrLine(), OPCODE_SUB));
}
    break;

  case 39:
#line 246 "parser.y"
    {
	pScript->AddInstruction(new NXSL_Instruction(pLexer->GetCurrLine(), OPCODE_MUL));
}
    break;

  case 40:
#line 250 "parser.y"
    {
	pScript->AddInstruction(new NXSL_Instruction(pLexer->GetCurrLine(), OPCODE_DIV));
}
    break;

  case 41:
#line 254 "parser.y"
    {
	pScript->AddInstruction(new NXSL_Instruction(pLexer->GetCurrLine(), OPCODE_REM));
}
    break;

  case 42:
#line 258 "parser.y"
    {
	pScript->AddInstruction(new NXSL_Instruction(pLexer->GetCurrLine(), OPCODE_LIKE));
}
    break;

  case 43:
#line 262 "parser.y"
    {
	pScript->AddInstruction(new NXSL_Instruction(pLexer->GetCurrLine(), OPCODE_ILIKE));
}
    break;

  case 44:
#line 266 "parser.y"
    {
	pScript->AddInstruction(new NXSL_Instruction(pLexer->GetCurrLine(), OPCODE_MATCH));
}
    break;

  case 45:
#line 270 "parser.y"
    {
	pScript->AddInstruction(new NXSL_Instruction(pLexer->GetCurrLine(), OPCODE_IMATCH));
}
    break;

  case 46:
#line 274 "parser.y"
    {
	pScript->AddInstruction(new NXSL_Instruction(pLexer->GetCurrLine(), OPCODE_EQ));
}
    break;

  case 47:
#line 278 "parser.y"
    {
	pScript->AddInstruction(new NXSL_Instruction(pLexer->GetCurrLine(), OPCODE_NE));
}
    break;

  case 48:
#line 282 "parser.y"
    {
	pScript->AddInstruction(new NXSL_Instruction(pLexer->GetCurrLine(), OPCODE_LT));
}
    break;

  case 49:
#line 286 "parser.y"
    {
	pScript->AddInstruction(new NXSL_Instruction(pLexer->GetCurrLine(), OPCODE_LE));
}
    break;

  case 50:
#line 290 "parser.y"
    {
	pScript->AddInstruction(new NXSL_Instruction(pLexer->GetCurrLine(), OPCODE_GT));
}
    break;

  case 51:
#line 294 "parser.y"
    {
	pScript->AddInstruction(new NXSL_Instruction(pLexer->GetCurrLine(), OPCODE_GE));
}
    break;

  case 52:
#line 298 "parser.y"
    {
	pScript->AddInstruction(new NXSL_Instruction(pLexer->GetCurrLine(), OPCODE_BIT_AND));
}
    break;

  case 53:
#line 302 "parser.y"
    {
	pScript->AddInstruction(new NXSL_Instruction(pLexer->GetCurrLine(), OPCODE_BIT_OR));
}
    break;

  case 54:
#line 306 "parser.y"
    {
	pScript->AddInstruction(new NXSL_Instruction(pLexer->GetCurrLine(), OPCODE_BIT_XOR));
}
    break;

  case 55:
#line 310 "parser.y"
    {
	pScript->AddInstruction(new NXSL_Instruction(pLexer->GetCurrLine(), OPCODE_AND));
}
    break;

  case 56:
#line 314 "parser.y"
    {
	pScript->AddInstruction(new NXSL_Instruction(pLexer->GetCurrLine(), OPCODE_OR));
}
    break;

  case 57:
#line 318 "parser.y"
    {
	pScript->AddInstruction(new NXSL_Instruction(pLexer->GetCurrLine(), OPCODE_LSHIFT));
}
    break;

  case 58:
#line 322 "parser.y"
    {
	pScript->AddInstruction(new NXSL_Instruction(pLexer->GetCurrLine(), OPCODE_RSHIFT));
}
    break;

  case 59:
#line 326 "parser.y"
    {
	pScript->AddInstruction(new NXSL_Instruction(pLexer->GetCurrLine(), OPCODE_CONCAT));
}
    break;

  case 63:
#line 336 "parser.y"
    {
	pScript->AddInstruction(new NXSL_Instruction(pLexer->GetCurrLine(), OPCODE_PUSH_VARIABLE, (yyvsp[0].valStr)));
}
    break;

  case 64:
#line 340 "parser.y"
    {
	pScript->AddInstruction(new NXSL_Instruction(pLexer->GetCurrLine(), OPCODE_PUSH_CONSTANT, (yyvsp[0].pConstant)));
}
    break;

  case 65:
#line 347 "parser.y"
    {
	pScript->AddInstruction(new NXSL_Instruction(pLexer->GetCurrLine(),
				OPCODE_CAST, (int)(yyvsp[-3].valInt32)));
}
    break;

  case 66:
#line 355 "parser.y"
    {
	(yyval.valInt32) = NXSL_DT_INT32;
}
    break;

  case 67:
#line 359 "parser.y"
    {
	(yyval.valInt32) = NXSL_DT_INT64;
}
    break;

  case 68:
#line 363 "parser.y"
    {
	(yyval.valInt32) = NXSL_DT_INT32;
}
    break;

  case 69:
#line 367 "parser.y"
    {
	(yyval.valInt32) = NXSL_DT_UINT64;
}
    break;

  case 70:
#line 371 "parser.y"
    {
	(yyval.valInt32) = NXSL_DT_REAL;
}
    break;

  case 71:
#line 375 "parser.y"
    {
	(yyval.valInt32) = NXSL_DT_STRING;
}
    break;

  case 76:
#line 386 "parser.y"
    {
	DWORD dwAddr = pCompiler->PeekAddr();
	if (dwAddr != INVALID_ADDRESS)
	{
		pScript->AddInstruction(new NXSL_Instruction(pLexer->GetCurrLine(),
					OPCODE_JMP, dwAddr));
	}
	else
	{
		pCompiler->Error("\"continue\" statement must be within loop");
	}
}
    break;

  case 77:
#line 402 "parser.y"
    {
	pScript->AddInstruction((yyvsp[-1].pInstruction));
}
    break;

  case 78:
#line 406 "parser.y"
    {
	pScript->AddInstruction(new NXSL_Instruction(pLexer->GetCurrLine(), OPCODE_PUSH_CONSTANT, new NXSL_Value));
	pScript->AddInstruction((yyvsp[0].pInstruction));
}
    break;

  case 79:
#line 414 "parser.y"
    {
	(yyval.pInstruction) = new NXSL_Instruction(pLexer->GetCurrLine(), OPCODE_EXIT);
}
    break;

  case 80:
#line 418 "parser.y"
    {
	(yyval.pInstruction) = new NXSL_Instruction(pLexer->GetCurrLine(), OPCODE_RETURN);
}
    break;

  case 81:
#line 422 "parser.y"
    {
	(yyval.pInstruction) = new NXSL_Instruction(pLexer->GetCurrLine(), OPCODE_PRINT);
}
    break;

  case 82:
#line 429 "parser.y"
    {
		pScript->AddInstruction(new NXSL_Instruction(pLexer->GetCurrLine(), OPCODE_JZ, INVALID_ADDRESS));
	}
    break;

  case 84:
#line 437 "parser.y"
    {
	pScript->ResolveLastJump(OPCODE_JZ);
}
    break;

  case 85:
#line 441 "parser.y"
    {
	pScript->ResolveLastJump(OPCODE_JMP);
}
    break;

  case 86:
#line 448 "parser.y"
    {
		pScript->AddInstruction(new NXSL_Instruction(pLexer->GetCurrLine(), OPCODE_JMP, INVALID_ADDRESS));
		pScript->ResolveLastJump(OPCODE_JZ);
	}
    break;

  case 88:
#line 457 "parser.y"
    {
		pCompiler->PushAddr(pScript->CodeSize());
	}
    break;

  case 89:
#line 461 "parser.y"
    {
		pScript->AddInstruction(new NXSL_Instruction(pLexer->GetCurrLine(), OPCODE_JZ, INVALID_ADDRESS));
	}
    break;

  case 90:
#line 465 "parser.y"
    {
	pScript->AddInstruction(new NXSL_Instruction(pLexer->GetCurrLine(), 
				OPCODE_JMP, pCompiler->PopAddr()));
	pScript->ResolveLastJump(OPCODE_JZ);
}
    break;

  case 91:
#line 474 "parser.y"
    {
		pCompiler->PushAddr(pScript->CodeSize());
	}
    break;

  case 92:
#line 478 "parser.y"
    {
	pScript->AddInstruction(new NXSL_Instruction(pLexer->GetCurrLine(),
				OPCODE_JNZ, pCompiler->PopAddr()));
}
    break;

  case 93:
#line 486 "parser.y"
    {
	pScript->AddInstruction(new NXSL_Instruction(pLexer->GetCurrLine(), OPCODE_CALL_EXTERNAL, (yyvsp[-2].valStr), (yyvsp[-1].valInt32)));
}
    break;

  case 94:
#line 490 "parser.y"
    {
	pScript->AddInstruction(new NXSL_Instruction(pLexer->GetCurrLine(), OPCODE_CALL_EXTERNAL, (yyvsp[-1].valStr), 0));
}
    break;

  case 95:
#line 496 "parser.y"
    { (yyval.valInt32) = (yyvsp[0].valInt32) + 1; }
    break;

  case 96:
#line 497 "parser.y"
    { (yyval.valInt32) = 1; }
    break;

  case 97:
#line 502 "parser.y"
    {
	(yyval.valStr) = (yyvsp[-1].valStr);
}
    break;

  case 98:
#line 509 "parser.y"
    {
	(yyval.pConstant) = new NXSL_Value((yyvsp[0].valStr));
	free((yyvsp[0].valStr));
}
    break;

  case 99:
#line 514 "parser.y"
    {
	(yyval.pConstant) = new NXSL_Value((yyvsp[0].valInt32));
}
    break;

  case 100:
#line 518 "parser.y"
    {
	(yyval.pConstant) = new NXSL_Value((yyvsp[0].valUInt32));
}
    break;

  case 101:
#line 522 "parser.y"
    {
	(yyval.pConstant) = new NXSL_Value((yyvsp[0].valInt64));
}
    break;

  case 102:
#line 526 "parser.y"
    {
	(yyval.pConstant) = new NXSL_Value((yyvsp[0].valUInt64));
}
    break;

  case 103:
#line 530 "parser.y"
    {
	(yyval.pConstant) = new NXSL_Value((yyvsp[0].valReal));
}
    break;

  case 104:
#line 534 "parser.y"
    {
	(yyval.pConstant) = new NXSL_Value;
}
    break;


      default: break;
    }

/* Line 1126 of yacc.c.  */
#line 2059 "parser.tab.cpp"

  yyvsp -= yylen;
  yyssp -= yylen;


  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;


  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if YYERROR_VERBOSE
      yyn = yypact[yystate];

      if (YYPACT_NINF < yyn && yyn < YYLAST)
	{
	  int yytype = YYTRANSLATE (yychar);
	  YYSIZE_T yysize0 = yytnamerr (0, yytname[yytype]);
	  YYSIZE_T yysize = yysize0;
	  YYSIZE_T yysize1;
	  int yysize_overflow = 0;
	  char *yymsg = 0;
#	  define YYERROR_VERBOSE_ARGS_MAXIMUM 5
	  char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
	  int yyx;

#if 0
	  /* This is so xgettext sees the translatable formats that are
	     constructed on the fly.  */
	  YY_("syntax error, unexpected %s");
	  YY_("syntax error, unexpected %s, expecting %s");
	  YY_("syntax error, unexpected %s, expecting %s or %s");
	  YY_("syntax error, unexpected %s, expecting %s or %s or %s");
	  YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s");
#endif
	  char *yyfmt;
	  char const *yyf;
	  static char const yyunexpected[] = "syntax error, unexpected %s";
	  static char const yyexpecting[] = ", expecting %s";
	  static char const yyor[] = " or %s";
	  char yyformat[sizeof yyunexpected
			+ sizeof yyexpecting - 1
			+ ((YYERROR_VERBOSE_ARGS_MAXIMUM - 2)
			   * (sizeof yyor - 1))];
	  char const *yyprefix = yyexpecting;

	  /* Start YYX at -YYN if negative to avoid negative indexes in
	     YYCHECK.  */
	  int yyxbegin = yyn < 0 ? -yyn : 0;

	  /* Stay within bounds of both yycheck and yytname.  */
	  int yychecklim = YYLAST - yyn;
	  int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
	  int yycount = 1;

	  yyarg[0] = yytname[yytype];
	  yyfmt = yystpcpy (yyformat, yyunexpected);

	  for (yyx = yyxbegin; yyx < yyxend; ++yyx)
	    if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	      {
		if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
		  {
		    yycount = 1;
		    yysize = yysize0;
		    yyformat[sizeof yyunexpected - 1] = '\0';
		    break;
		  }
		yyarg[yycount++] = yytname[yyx];
		yysize1 = yysize + yytnamerr (0, yytname[yyx]);
		yysize_overflow |= yysize1 < yysize;
		yysize = yysize1;
		yyfmt = yystpcpy (yyfmt, yyprefix);
		yyprefix = yyor;
	      }

	  yyf = YY_(yyformat);
	  yysize1 = yysize + yystrlen (yyf);
	  yysize_overflow |= yysize1 < yysize;
	  yysize = yysize1;

	  if (!yysize_overflow && yysize <= YYSTACK_ALLOC_MAXIMUM)
	    yymsg = (char *) YYSTACK_ALLOC (yysize);
	  if (yymsg)
	    {
	      /* Avoid sprintf, as that infringes on the user's name space.
		 Don't have undefined behavior even if the translation
		 produced a string with the wrong number of "%s"s.  */
	      char *yyp = yymsg;
	      int yyi = 0;
	      while ((*yyp = *yyf))
		{
		  if (*yyp == '%' && yyf[1] == 's' && yyi < yycount)
		    {
		      yyp += yytnamerr (yyp, yyarg[yyi++]);
		      yyf += 2;
		    }
		  else
		    {
		      yyp++;
		      yyf++;
		    }
		}
	      yyerror (pLexer, pCompiler, pScript, yymsg);
	      YYSTACK_FREE (yymsg);
	    }
	  else
	    {
	      yyerror (pLexer, pCompiler, pScript, YY_("syntax error"));
	      goto yyexhaustedlab;
	    }
	}
      else
#endif /* YYERROR_VERBOSE */
	yyerror (pLexer, pCompiler, pScript, YY_("syntax error"));
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse look-ahead token after an
	 error, discard it.  */

      if (yychar <= YYEOF)
        {
	  /* Return failure if at end of input.  */
	  if (yychar == YYEOF)
	    YYABORT;
        }
      else
	{
	  yydestruct ("Error: discarding", yytoken, &yylval);
	  yychar = YYEMPTY;
	}
    }

  /* Else will try to reuse look-ahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (0)
     goto yyerrorlab;

yyvsp -= yylen;
  yyssp -= yylen;
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (yyn != YYPACT_NINF)
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
	    {
	      yyn = yytable[yyn];
	      if (0 < yyn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
	YYABORT;


      yydestruct ("Error: popping", yystos[yystate], yyvsp);
      YYPOPSTACK;
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  *++yyvsp = yylval;


  /* Shift the error token. */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#ifndef yyoverflow
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (pLexer, pCompiler, pScript, YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEOF && yychar != YYEMPTY)
     yydestruct ("Cleanup: discarding lookahead",
		 yytoken, &yylval);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp);
      YYPOPSTACK;
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
  return yyresult;
}



