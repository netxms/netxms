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
** Pure helper functions for the generic webhook driver: JSON-context
** placeholder substitution and comma-separated integer-list parsing.
** Kept libcurl-free so they can be unit-tested in isolation.
**
**/

#ifndef _webhook_helpers_h_
#define _webhook_helpers_h_

#include <nms_util.h>

/**
 * Substitute ${recipient}, ${subject}, ${body} placeholders in a UTF-8
 * template, replacing each with the corresponding pre-escaped UTF-8
 * replacement string. Unknown tokens are left literal so user typos are
 * visible. A null replacement leaves its token literal too.
 *
 * This is the shared byte-level scan; callers supply replacements escaped for
 * their target context (JSON-escaped for bodies, percent-encoded for URLs).
 *
 * Returns a heap-allocated, null-terminated UTF-8 buffer; caller must MemFree.
 */
char *SubstitutePlaceholders(const char *tpl, const char *recipientRepl, const char *subjectRepl, const char *bodyRepl);

/**
 * Substitute ${recipient}, ${subject}, ${body} placeholders in the request
 * body template, replacing each with its JSON-escaped value. Unknown tokens
 * are left literal so user typos are visible. A null value is treated as an
 * empty string.
 *
 * The template is the raw UTF-8 byte string loaded from disk
 * (LoadFileAsUTF8String) and the values are UTF-8. Substitution runs directly
 * over those bytes - placeholder names and JSON escapes are pure ASCII, so
 * scanning and escaping byte-by-byte is safe regardless of any multi-byte
 * sequences in the data, and the result is ready for CURLOPT_POSTFIELDS
 * without any re-encoding round-trip.
 *
 * Returns a heap-allocated, null-terminated UTF-8 buffer; caller must MemFree.
 */
char *SubstitutePlaceholdersJSONUtf8(const char *tpl, const char *recipient, const char *subject, const char *body);

/**
 * Parse a comma-separated list of integers into the provided array.
 * Tolerates leading/trailing whitespace around entries. On any malformed
 * entry or empty input the array is cleared and false is returned.
 */
bool ParseIntList(const TCHAR *csv, IntegerArray<int> *out);

#endif
