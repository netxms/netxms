/*
** NetXMS - Network Management System
** Copyright (C) 2003-2026 Raden Solutions
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: cmdline.cpp
**
** Unit tests for SplitCommandLine: splitting of action and object tool command
** lines into argument lists with space separators and single/double quoting.
**
**/

#include <nms_common.h>
#include <nms_util.h>
#include <nxcpapi.h>
#include <nms_core.h>
#include <testtools.h>

/**
 * Unquoted words separated by single spaces
 */
static void TestSimpleSplit()
{
   StartTest(_T("SplitCommandLine - simple"));

   StringList args = SplitCommandLine(L"cmd arg1 arg2");
   AssertEquals(args.size(), 3);
   AssertEquals(args.get(0), L"cmd");
   AssertEquals(args.get(1), L"arg1");
   AssertEquals(args.get(2), L"arg2");

   args = SplitCommandLine(L"cmd");
   AssertEquals(args.size(), 1);
   AssertEquals(args.get(0), L"cmd");

   EndTest();
}

/**
 * Empty input produces single empty element
 */
static void TestEmptyInput()
{
   StartTest(_T("SplitCommandLine - empty input"));

   StringList args = SplitCommandLine(L"");
   AssertEquals(args.size(), 1);
   AssertEquals(args.get(0), L"");

   // Input consisting only of spaces also produces single empty element
   args = SplitCommandLine(L"   ");
   AssertEquals(args.size(), 1);
   AssertEquals(args.get(0), L"");

   EndTest();
}

/**
 * Leading spaces are ignored and do not produce empty first element
 */
static void TestLeadingSpaces()
{
   StartTest(_T("SplitCommandLine - leading spaces"));

   StringList args = SplitCommandLine(L"   cmd arg");
   AssertEquals(args.size(), 2);
   AssertEquals(args.get(0), L"cmd");
   AssertEquals(args.get(1), L"arg");

   args = SplitCommandLine(L" \"quoted cmd\" arg");
   AssertEquals(args.size(), 2);
   AssertEquals(args.get(0), L"quoted cmd");
   AssertEquals(args.get(1), L"arg");

   EndTest();
}

/**
 * Runs of spaces between words are collapsed and trailing spaces are ignored
 */
static void TestExtraSpaces()
{
   StartTest(_T("SplitCommandLine - extra spaces"));

   StringList args = SplitCommandLine(L"cmd    arg1  arg2");
   AssertEquals(args.size(), 3);
   AssertEquals(args.get(0), L"cmd");
   AssertEquals(args.get(1), L"arg1");
   AssertEquals(args.get(2), L"arg2");

   args = SplitCommandLine(L"cmd arg   ");
   AssertEquals(args.size(), 2);
   AssertEquals(args.get(0), L"cmd");
   AssertEquals(args.get(1), L"arg");

   EndTest();
}

/**
 * Only space is a separator; tabs are part of the word
 */
static void TestTabNotSeparator()
{
   StartTest(_T("SplitCommandLine - tab is not separator"));

   StringList args = SplitCommandLine(L"cmd\targ");
   AssertEquals(args.size(), 1);
   AssertEquals(args.get(0), L"cmd\targ");

   EndTest();
}

/**
 * Double quoted strings keep spaces and are returned without quotes
 */
static void TestDoubleQuotes()
{
   StartTest(_T("SplitCommandLine - double quotes"));

   StringList args = SplitCommandLine(L"cmd \"hello world\" last");
   AssertEquals(args.size(), 3);
   AssertEquals(args.get(0), L"cmd");
   AssertEquals(args.get(1), L"hello world");
   AssertEquals(args.get(2), L"last");

   // Single quote inside double quoted string is literal
   args = SplitCommandLine(L"cmd \"it's here\"");
   AssertEquals(args.size(), 2);
   AssertEquals(args.get(1), L"it's here");

   EndTest();
}

/**
 * Single quoted strings keep spaces and are returned without quotes
 */
static void TestSingleQuotes()
{
   StartTest(_T("SplitCommandLine - single quotes"));

   StringList args = SplitCommandLine(L"cmd 'hello world' last");
   AssertEquals(args.size(), 3);
   AssertEquals(args.get(0), L"cmd");
   AssertEquals(args.get(1), L"hello world");
   AssertEquals(args.get(2), L"last");

   // Double quote inside single quoted string is literal
   args = SplitCommandLine(L"cmd 'say \"hi\" now'");
   AssertEquals(args.size(), 2);
   AssertEquals(args.get(1), L"say \"hi\" now");

   EndTest();
}

/**
 * Quoted section can start in the middle of a word and is concatenated with unquoted parts
 */
static void TestQuotesInsideWord()
{
   StartTest(_T("SplitCommandLine - quotes inside word"));

   StringList args = SplitCommandLine(L"cmd ab\"c d\"ef 'x'y\"z\"");
   AssertEquals(args.size(), 3);
   AssertEquals(args.get(0), L"cmd");
   AssertEquals(args.get(1), L"abc def");
   AssertEquals(args.get(2), L"xyz");

   EndTest();
}

/**
 * Empty quoted string produces empty argument
 */
static void TestEmptyQuotedArgument()
{
   StartTest(_T("SplitCommandLine - empty quoted argument"));

   StringList args = SplitCommandLine(L"cmd \"\" last");
   AssertEquals(args.size(), 3);
   AssertEquals(args.get(0), L"cmd");
   AssertEquals(args.get(1), L"");
   AssertEquals(args.get(2), L"last");

   args = SplitCommandLine(L"cmd ''");
   AssertEquals(args.size(), 2);
   AssertEquals(args.get(1), L"");

   EndTest();
}

/**
 * Unterminated quote consumes rest of the line as single argument
 */
static void TestUnterminatedQuote()
{
   StartTest(_T("SplitCommandLine - unterminated quote"));

   StringList args = SplitCommandLine(L"cmd \"abc def");
   AssertEquals(args.size(), 2);
   AssertEquals(args.get(0), L"cmd");
   AssertEquals(args.get(1), L"abc def");

   args = SplitCommandLine(L"cmd 'abc def");
   AssertEquals(args.size(), 2);
   AssertEquals(args.get(1), L"abc def");

   EndTest();
}

/**
 * Command line splitting tests
 */
void TestSplitCommandLine()
{
   TestSimpleSplit();
   TestEmptyInput();
   TestLeadingSpaces();
   TestExtraSpaces();
   TestTabNotSeparator();
   TestDoubleQuotes();
   TestSingleQuotes();
   TestQuotesInsideWord();
   TestEmptyQuotedArgument();
   TestUnterminatedQuote();
}
