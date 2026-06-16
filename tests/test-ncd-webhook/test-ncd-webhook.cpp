/*
** NetXMS - Network Management System
** Unit tests for the generic webhook notification channel driver helpers
** Copyright (C) 2014-2026 Raden Solutions
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation; either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: test-ncd-webhook.cpp
**
**/

#include <nms_common.h>
#include <nms_util.h>
#include <testtools.h>
#include <netxms-version.h>
#include <webhook_helpers.h>

NETXMS_EXECUTABLE_HEADER(test-ncd-webhook)

/**
 * é (U+00E9) as a TCHAR string - used for UTF-8 round-trip checks
 */
static const TCHAR s_eacute[2] = { static_cast<TCHAR>(0x00E9), 0 };

/**
 * Test JSON-context placeholder substitution (UTF-8 request-body path)
 */
static void TestSubstituteJSONUtf8()
{
   StartTest(_T("SubstitutePlaceholdersJSON (UTF-8)"));

   char *r;

   // all three placeholders
   r = SubstitutePlaceholdersJSONUtf8("{\"to\":\"${recipient}\",\"s\":\"${subject}\",\"m\":\"${body}\"}",
            _T("+123"), _T("hi"), _T("world"));
   AssertEquals(r, "{\"to\":\"+123\",\"s\":\"hi\",\"m\":\"world\"}");
   MemFree(r);

   // repeated placeholder, and adjacent placeholders with no separator
   r = SubstitutePlaceholdersJSONUtf8("${body}-${body}", _T(""), _T(""), _T("x"));
   AssertEquals(r, "x-x");
   MemFree(r);
   r = SubstitutePlaceholdersJSONUtf8("${recipient}${body}", _T("R"), _T(""), _T("B"));
   AssertEquals(r, "RB");
   MemFree(r);

   // missing placeholder value -> empty substitution
   r = SubstitutePlaceholdersJSONUtf8("a${subject}b", _T(""), _T(""), _T(""));
   AssertEquals(r, "ab");
   MemFree(r);

   // empty (non-null) template -> empty string
   r = SubstitutePlaceholdersJSONUtf8("", _T("r"), _T("s"), _T("b"));
   AssertEquals(r, "");
   MemFree(r);

   // null template -> empty string
   r = SubstitutePlaceholdersJSONUtf8(static_cast<const char*>(nullptr), _T("r"), _T("s"), _T("b"));
   AssertEquals(r, "");
   MemFree(r);

   // JSON escaping of replacement value: quote, backslash, newline, CR, tab
   r = SubstitutePlaceholdersJSONUtf8("{\"m\":\"${body}\"}", _T(""), _T(""), _T("a\"b\\c\nd\re\tf"));
   AssertEquals(r, "{\"m\":\"a\\\"b\\\\c\\nd\\re\\tf\"}");
   MemFree(r);

   // UTF-8 bytes already in the template are preserved verbatim
   r = SubstitutePlaceholdersJSONUtf8("caf\xC3\xA9 ${body}", _T(""), _T(""), _T("x"));
   AssertEquals(r, "caf\xC3\xA9 x");
   MemFree(r);

   // UTF-8 in the replacement value (é -> C3 A9)
   r = SubstitutePlaceholdersJSONUtf8("[${subject}]", _T(""), s_eacute, _T(""));
   AssertEquals(r, "[\xC3\xA9]");
   MemFree(r);

   // unknown token left literal
   r = SubstitutePlaceholdersJSONUtf8("a${xyz}b", _T("r"), _T("s"), _T("b"));
   AssertEquals(r, "a${xyz}b");
   MemFree(r);

   // token that only shares a prefix with a known name is unknown (exact-length
   // match, not prefix match) - guards against a regression to prefix matching
   r = SubstitutePlaceholdersJSONUtf8("${bod}-${bodyX}", _T("r"), _T("s"), _T("B"));
   AssertEquals(r, "${bod}-${bodyX}");
   MemFree(r);

   // empty token name left literal
   r = SubstitutePlaceholdersJSONUtf8("a${}b", _T("r"), _T("s"), _T("b"));
   AssertEquals(r, "a${}b");
   MemFree(r);

   // unterminated placeholder copied literally
   r = SubstitutePlaceholdersJSONUtf8("abc${body", _T("r"), _T("s"), _T("b"));
   AssertEquals(r, "abc${body");
   MemFree(r);

   EndTest();
}

/**
 * Test the shared byte-level placeholder scan with pre-escaped replacements
 * (the path the URL substitution takes, exercised here without libcurl).
 */
static void TestSubstitutePlaceholders()
{
   StartTest(_T("SubstitutePlaceholders"));

   char *r;

   // all three placeholders replaced verbatim with their pre-escaped values
   r = SubstitutePlaceholders("/${recipient}/${subject}/${body}", "R", "S", "B");
   AssertEquals(r, "/R/S/B");
   MemFree(r);

   // a null replacement leaves its token literal
   r = SubstitutePlaceholders("${recipient}-${subject}", "R", nullptr, "B");
   AssertEquals(r, "R-${subject}");
   MemFree(r);

   // unknown token left literal
   r = SubstitutePlaceholders("a${xyz}b", "R", "S", "B");
   AssertEquals(r, "a${xyz}b");
   MemFree(r);

   // unterminated placeholder copied literally
   r = SubstitutePlaceholders("abc${body", "R", "S", "B");
   AssertEquals(r, "abc${body");
   MemFree(r);

   // null template yields empty string
   r = SubstitutePlaceholders(nullptr, "R", "S", "B");
   AssertEquals(r, "");
   MemFree(r);

   EndTest();
}

/**
 * Test comma-separated integer list parsing
 */
static void TestParseIntList()
{
   StartTest(_T("ParseIntList"));

   IntegerArray<int> a;

   AssertFalse(ParseIntList(nullptr, &a));
   AssertTrue(a.isEmpty());

   AssertFalse(ParseIntList(_T(""), &a));
   AssertTrue(a.isEmpty());

   AssertTrue(ParseIntList(_T("200"), &a));
   AssertEquals(a.size(), 1);
   AssertEquals(a.get(0), 200);

   AssertTrue(ParseIntList(_T("200,201,202,204"), &a));
   AssertEquals(a.size(), 4);
   AssertEquals(a.get(0), 200);
   AssertEquals(a.get(1), 201);
   AssertEquals(a.get(2), 202);
   AssertEquals(a.get(3), 204);

   // whitespace tolerance
   AssertTrue(ParseIntList(_T(" 429 , 502 ,503 "), &a));
   AssertEquals(a.size(), 3);
   AssertEquals(a.get(0), 429);
   AssertEquals(a.get(1), 502);
   AssertEquals(a.get(2), 503);

   // whitespace-only input -> false, empty
   AssertFalse(ParseIntList(_T("   "), &a));
   AssertTrue(a.isEmpty());

   // a trailing comma is tolerated (the preceding value still parses)
   AssertTrue(ParseIntList(_T("200,"), &a));
   AssertEquals(a.size(), 1);
   AssertEquals(a.get(0), 200);

   // a leading or empty-between-commas token is malformed -> false, cleared
   AssertFalse(ParseIntList(_T(",200"), &a));
   AssertTrue(a.isEmpty());
   AssertFalse(ParseIntList(_T("200,,201"), &a));
   AssertTrue(a.isEmpty());

   // malformed entry -> false, array cleared
   AssertFalse(ParseIntList(_T("200,abc"), &a));
   AssertTrue(a.isEmpty());

   AssertFalse(ParseIntList(_T("20x0"), &a));
   AssertTrue(a.isEmpty());

   EndTest();
}

/**
 * main()
 */
int main(int argc, char *argv[])
{
   InitNetXMSProcess(true);

   TestSubstituteJSONUtf8();
   TestSubstitutePlaceholders();
   TestParseIntList();

   InitiateProcessShutdown();
   return 0;
}
