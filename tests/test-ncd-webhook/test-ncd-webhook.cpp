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
 * Test RFC 3986 percent-encoding
 */
static void TestUrlEncode()
{
   StartTest(_T("UrlEncode"));

   AssertEquals(UrlEncode(nullptr).cstr(), _T(""));
   AssertEquals(UrlEncode(_T("")).cstr(), _T(""));

   // unreserved set passes through unchanged
   AssertEquals(UrlEncode(_T("ABCxyz0189-_.~")).cstr(), _T("ABCxyz0189-_.~"));

   // reserved ASCII gets encoded
   AssertEquals(UrlEncode(_T("/")).cstr(), _T("%2F"));
   AssertEquals(UrlEncode(_T("?")).cstr(), _T("%3F"));
   AssertEquals(UrlEncode(_T("&")).cstr(), _T("%26"));
   AssertEquals(UrlEncode(_T("=")).cstr(), _T("%3D"));
   AssertEquals(UrlEncode(_T("+")).cstr(), _T("%2B"));
   AssertEquals(UrlEncode(_T(" ")).cstr(), _T("%20"));
   AssertEquals(UrlEncode(_T("a b/c")).cstr(), _T("a%20b%2Fc"));

   // a literal '%' must be encoded (no double-encoding bug)
   AssertEquals(UrlEncode(_T("100%")).cstr(), _T("100%25"));

   // UTF-8 multi-byte: é -> %C3%A9
   AssertEquals(UrlEncode(s_eacute).cstr(), _T("%C3%A9"));

   // multi-byte interleaved with ASCII and a space: a éb -> a%20%C3%A9b
   static const TCHAR mixed[5] = { _T('a'), _T(' '), static_cast<TCHAR>(0x00E9), _T('b'), 0 };
   AssertEquals(UrlEncode(mixed).cstr(), _T("a%20%C3%A9b"));

   EndTest();
}

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
 * Test URL-context placeholder substitution
 */
static void TestSubstituteURL()
{
   StartTest(_T("SubstitutePlaceholdersURL"));

   // special chars get percent-encoded
   AssertEquals(
      SubstitutePlaceholdersURL(_T("https://x/api?to=${recipient}&m=${body}"),
            _T("+1 2/3"), _T(""), _T("a&b")).cstr(),
      _T("https://x/api?to=%2B1%202%2F3&m=a%26b"));

   // unreserved set passes through unchanged
   AssertEquals(SubstitutePlaceholdersURL(_T("u/${recipient}"), _T("Abc-1._~"), _T(""), _T("")).cstr(),
            _T("u/Abc-1._~"));

   // UTF-8 recipient encoded
   AssertEquals(SubstitutePlaceholdersURL(_T("${recipient}"), s_eacute, _T(""), _T("")).cstr(), _T("%C3%A9"));

   // unknown token literal, missing value empty
   AssertEquals(SubstitutePlaceholdersURL(_T("${foo}-${subject}"), _T("r"), _T(""), _T("")).cstr(), _T("${foo}-"));

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
 * Test JSON Pointer lookup and value stringification
 */
static void TestJsonPathLookup()
{
   StartTest(_T("JsonPathLookup"));

   // null root -> empty
   AssertEquals(JsonPathLookup(nullptr, _T("/x")).cstr(), _T(""));

   json_error_t error;
   json_t *root = json_loads(
      "{\"ErrorCode\":0,\"status\":\"ok\",\"nested\":{\"deep\":{\"value\":\"hit\"}},"
      "\"arr\":[10,20,30],\"flag\":true,\"off\":false,\"nul\":null,\"obj\":{},"
      "\"a/b\":\"slash\",\"ti~ld\":\"tilde\",\"pi\":1.5,\"whole\":2.0,"
      "\"frac\":0.1}", 0, &error);
   AssertNotNull(root);

   // integer value (A2A ErrorCode == 0)
   AssertEquals(JsonPathLookup(root, _T("/ErrorCode")).cstr(), _T("0"));

   // string value
   AssertEquals(JsonPathLookup(root, _T("/status")).cstr(), _T("ok"));

   // nested object lookup
   AssertEquals(JsonPathLookup(root, _T("/nested/deep/value")).cstr(), _T("hit"));

   // array index
   AssertEquals(JsonPathLookup(root, _T("/arr/1")).cstr(), _T("20"));

   // boolean / null
   AssertEquals(JsonPathLookup(root, _T("/flag")).cstr(), _T("true"));
   AssertEquals(JsonPathLookup(root, _T("/off")).cstr(), _T("false"));
   AssertEquals(JsonPathLookup(root, _T("/nul")).cstr(), _T("null"));

   // real value - compact, locale-independent (no forced six decimals)
   AssertEquals(JsonPathLookup(root, _T("/pi")).cstr(), _T("1.5"));
   AssertEquals(JsonPathLookup(root, _T("/whole")).cstr(), _T("2.0"));
   AssertEquals(JsonPathLookup(root, _T("/frac")).cstr(), _T("0.1"));

   // missing path -> empty
   AssertEquals(JsonPathLookup(root, _T("/missing")).cstr(), _T(""));
   AssertEquals(JsonPathLookup(root, _T("/nested/nope")).cstr(), _T(""));

   // object / array node has no scalar form -> empty
   AssertEquals(JsonPathLookup(root, _T("/obj")).cstr(), _T(""));
   AssertEquals(JsonPathLookup(root, _T("/arr")).cstr(), _T(""));

   // attempt to descend into a scalar -> empty
   AssertEquals(JsonPathLookup(root, _T("/status/x")).cstr(), _T(""));

   // out-of-range / non-numeric / negative array index -> empty
   AssertEquals(JsonPathLookup(root, _T("/arr/9")).cstr(), _T(""));
   AssertEquals(JsonPathLookup(root, _T("/arr/x")).cstr(), _T(""));
   AssertEquals(JsonPathLookup(root, _T("/arr/-1")).cstr(), _T(""));

   // a pointer segment longer than the internal token buffer fails closed
   // (no truncated-key match, no reparsing of the leftover bytes)
   TCHAR longPointer[400];
   longPointer[0] = _T('/');
   for(int i = 1; i < 350; i++)
      longPointer[i] = _T('a');
   longPointer[350] = 0;
   AssertEquals(JsonPathLookup(root, longPointer).cstr(), _T(""));

   // RFC 6901 token unescaping: ~1 -> '/', ~0 -> '~'
   AssertEquals(JsonPathLookup(root, _T("/a~1b")).cstr(), _T("slash"));
   AssertEquals(JsonPathLookup(root, _T("/ti~0ld")).cstr(), _T("tilde"));

   // malformed pointer (no leading '/') -> empty
   AssertEquals(JsonPathLookup(root, _T("ErrorCode")).cstr(), _T(""));

   // empty pointer points at the whole document (object) -> empty
   AssertEquals(JsonPathLookup(root, _T("")).cstr(), _T(""));

   json_decref(root);

   EndTest();
}

/**
 * main()
 */
int main(int argc, char *argv[])
{
   InitNetXMSProcess(true);

   TestUrlEncode();
   TestSubstituteJSONUtf8();
   TestSubstituteURL();
   TestParseIntList();
   TestJsonPathLookup();

   InitiateProcessShutdown();
   return 0;
}
