/*
** NetXMS - Network Management System
** Generic webhook notification channel driver
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
** File: webhook_helpers.h
**
** Pure helper functions for the generic webhook driver: placeholder
** substitution (URL and JSON contexts), RFC 3986 percent-encoding, and
** comma-separated integer-list parsing.
**
** These helpers intentionally do NOT depend on libcurl - the unit-test
** target links them without libcurl, so URL-encoding is a self-contained
** RFC 3986 implementation rather than curl_easy_escape().
**
**/

#ifndef _webhook_helpers_h_
#define _webhook_helpers_h_

#include <nms_util.h>

/**
 * RFC 3986 percent-encode a string. The input is converted to UTF-8 first,
 * then encoded byte-by-byte: unreserved characters (A-Z a-z 0-9 - _ . ~)
 * pass through unchanged, everything else becomes %XX (uppercase hex).
 * No libcurl dependency.
 */
String UrlEncode(const TCHAR *s);

/**
 * Substitute ${recipient}, ${subject}, ${body} placeholders in the request
 * body template, replacing each with its JSON-escaped (EscapeStringForJSON)
 * value. Unknown tokens are left literal so user typos are visible.
 *
 * The template is the raw UTF-8 byte string loaded from disk
 * (LoadFileAsUTF8String). Substitution runs directly over those bytes -
 * placeholder names are pure ASCII, so scanning for the ASCII bytes "${...}"
 * inside a UTF-8 stream is safe regardless of any multi-byte sequences
 * elsewhere in the template. Replacement values are JSON-escaped and emitted
 * as UTF-8 bytes, so the result is ready for CURLOPT_POSTFIELDS without any
 * extra re-encoding round-trip.
 *
 * Returns a heap-allocated, null-terminated UTF-8 buffer; caller must MemFree.
 */
char *SubstitutePlaceholdersJSONUtf8(const char *tpl, const TCHAR *recipient, const TCHAR *subject, const TCHAR *body);

/**
 * Substitute ${recipient}, ${subject}, ${body} placeholders in a URL,
 * replacing each with its RFC 3986 percent-encoded (UrlEncode) value.
 * Unknown tokens are left literal.
 */
String SubstitutePlaceholdersURL(const TCHAR *url, const TCHAR *recipient, const TCHAR *subject, const TCHAR *body);

/**
 * Parse a comma-separated list of integers into the provided array.
 * Tolerates leading/trailing whitespace around entries. On any malformed
 * entry or empty input the array is cleared and false is returned.
 */
bool ParseIntList(const TCHAR *csv, IntegerArray<int> *out);

/**
 * Resolve a JSON Pointer (RFC 6901) against a parsed jansson tree and return
 * the referenced value as a string. The pointer is split on '/', with the
 * standard "~1" -> "/" and "~0" -> "~" token unescaping; object keys and
 * array indices are walked accordingly. A non-empty pointer that does not
 * start with '/' is malformed and yields an empty string.
 *
 * Value stringification: strings as-is, integers and reals formatted
 * numerically, booleans as "true"/"false", null as "null". Objects, arrays,
 * a missing path, or a null root all yield an empty string.
 *
 * Uses jansson (json_t) but not libcurl - the test target links jansson but
 * not libcurl, keeping this file libcurl-free as documented above.
 */
String JsonPathLookup(json_t *root, const TCHAR *pointer);

#endif
