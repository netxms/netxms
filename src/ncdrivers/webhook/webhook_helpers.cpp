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
** File: webhook_helpers.cpp
**
** Pure (libcurl-free) helper functions for the generic webhook driver.
**
**/

#include "webhook_helpers.h"

/**
 * RFC 3986 unreserved characters (ASCII): A-Z a-z 0-9 - _ . ~
 */
static inline bool IsUnreserved(unsigned char c)
{
   return ((c >= 'A') && (c <= 'Z')) ||
          ((c >= 'a') && (c <= 'z')) ||
          ((c >= '0') && (c <= '9')) ||
          (c == '-') || (c == '_') || (c == '.') || (c == '~');
}

/**
 * RFC 3986 percent-encoding. Convert the TCHAR input to UTF-8, then encode
 * byte-by-byte. Unreserved bytes pass through, everything else becomes %XX.
 */
String UrlEncode(const TCHAR *s)
{
   StringBuffer out;
   if ((s == nullptr) || (*s == 0))
      return out;

   char *utf8 = UTF8StringFromTString(s);
   static const char hex[] = "0123456789ABCDEF";
   for(const unsigned char *p = reinterpret_cast<const unsigned char*>(utf8); *p != 0; p++)
   {
      if (IsUnreserved(*p))
      {
         out.append(static_cast<TCHAR>(*p));
      }
      else
      {
         TCHAR enc[3] = { _T('%'), static_cast<TCHAR>(hex[*p >> 4]), static_cast<TCHAR>(hex[*p & 0x0F]) };
         out.append(enc, 3);
      }
   }
   MemFree(utf8);
   return out;
}

/**
 * Token names recognized in templates. All ASCII.
 */
#define TOKEN_RECIPIENT  "recipient"
#define TOKEN_SUBJECT    "subject"
#define TOKEN_BODY       "body"

/**
 * Generic TCHAR-template substitution. Linear scan for "${", read token up to
 * "}", emit the matching pre-escaped replacement. Unknown tokens (and an
 * unterminated "${" at end of input) are copied literally so user typos stay
 * visible.
 */
static String SubstituteTemplate(const TCHAR *tpl, const TCHAR *recipientRepl, const TCHAR *subjectRepl, const TCHAR *bodyRepl)
{
   StringBuffer out;
   if (tpl == nullptr)
      return out;

   const TCHAR *p = tpl;
   while(*p != 0)
   {
      if ((p[0] == _T('$')) && (p[1] == _T('{')))
      {
         const TCHAR *end = _tcschr(p + 2, _T('}'));
         if (end == nullptr)
         {
            out.append(p);   // unterminated placeholder - copy rest literally
            break;
         }

         size_t tokenLen = static_cast<size_t>(end - (p + 2));
         if ((tokenLen == 9) && !_tcsncmp(p + 2, _T("recipient"), 9))
            out.append(recipientRepl);
         else if ((tokenLen == 7) && !_tcsncmp(p + 2, _T("subject"), 7))
            out.append(subjectRepl);
         else if ((tokenLen == 4) && !_tcsncmp(p + 2, _T("body"), 4))
            out.append(bodyRepl);
         else
            out.append(p, static_cast<size_t>(end + 1 - p));   // unknown token - literal

         p = end + 1;
      }
      else
      {
         out.append(*p);
         p++;
      }
   }
   return out;
}

/**
 * Substitute placeholders with percent-encoded values.
 */
String SubstitutePlaceholdersURL(const TCHAR *url, const TCHAR *recipient, const TCHAR *subject, const TCHAR *body)
{
   return SubstituteTemplate(url,
            UrlEncode(recipient),
            UrlEncode(subject),
            UrlEncode(body));
}

/**
 * UTF-8 request-body substitution. Operates on the raw UTF-8 template bytes
 * (as loaded from disk) so the result needs no re-encoding before being fed
 * to CURLOPT_POSTFIELDS. Placeholder names are ASCII, so a byte-level scan
 * for "${...}" is safe inside an arbitrary UTF-8 stream. Replacement values
 * are JSON-escaped and converted to UTF-8 bytes.
 *
 * Returns a heap-allocated, null-terminated UTF-8 buffer (caller MemFree).
 */
char *SubstitutePlaceholdersJSONUtf8(const char *tpl, const TCHAR *recipient, const TCHAR *subject, const TCHAR *body)
{
   if (tpl == nullptr)
      return MemCopyStringA("");

   char *recipientRepl = EscapeStringForJSON(recipient).getUTF8String();
   char *subjectRepl = EscapeStringForJSON(subject).getUTF8String();
   char *bodyRepl = EscapeStringForJSON(body).getUTF8String();

   ByteStream out(strlen(tpl) + 64);
   const char *p = tpl;
   while(*p != 0)
   {
      if ((p[0] == '$') && (p[1] == '{'))
      {
         const char *end = strchr(p + 2, '}');
         if (end == nullptr)
         {
            out.write(p, strlen(p));   // unterminated placeholder - copy rest literally
            break;
         }

         size_t tokenLen = static_cast<size_t>(end - (p + 2));
         const char *repl = nullptr;
         if ((tokenLen == 9) && !memcmp(p + 2, TOKEN_RECIPIENT, 9))
            repl = recipientRepl;
         else if ((tokenLen == 7) && !memcmp(p + 2, TOKEN_SUBJECT, 7))
            repl = subjectRepl;
         else if ((tokenLen == 4) && !memcmp(p + 2, TOKEN_BODY, 4))
            repl = bodyRepl;

         if (repl != nullptr)
            out.write(repl, strlen(repl));
         else
            out.write(p, static_cast<size_t>(end + 1 - p));   // unknown token - literal

         p = end + 1;
      }
      else
      {
         out.write(*p);
         p++;
      }
   }

   MemFree(recipientRepl);
   MemFree(subjectRepl);
   MemFree(bodyRepl);

   size_t len = out.size();
   char *result = MemAllocArray<char>(len + 1);
   if (len > 0)
      memcpy(result, out.buffer(), len);
   result[len] = 0;
   return result;
}

/**
 * Parse a comma-separated list of integers into the provided array.
 * Tolerates leading/trailing whitespace around entries. On any malformed
 * entry or empty input the array is cleared and false is returned.
 */
bool ParseIntList(const TCHAR *csv, IntegerArray<int> *out)
{
   out->clear();
   if (csv == nullptr)
      return false;

   const TCHAR *p = csv;
   while(*p != 0)
   {
      while((*p == _T(' ')) || (*p == _T('\t')))
         p++;
      if (*p == 0)
         break;

      TCHAR *eptr;
      long value = _tcstol(p, &eptr, 10);
      if (eptr == p)
      {
         out->clear();
         return false;
      }

      while((*eptr == _T(' ')) || (*eptr == _T('\t')))
         eptr++;
      if ((*eptr != 0) && (*eptr != _T(',')))
      {
         out->clear();
         return false;
      }

      out->add(static_cast<int>(value));
      if (*eptr == _T(','))
         eptr++;
      p = eptr;
   }
   return !out->isEmpty();
}

/**
 * Resolve a JSON Pointer (RFC 6901) and stringify the referenced value.
 * Operates on the UTF-8 form of the pointer (object keys are UTF-8 and
 * placeholder/pointer tokens are scanned byte-wise, which is safe because
 * the only structural bytes - '/', '~' - are ASCII).
 */
String JsonPathLookup(json_t *root, const TCHAR *pointer)
{
   if ((root == nullptr) || (pointer == nullptr))
      return String();

   char *path = UTF8StringFromTString(pointer);
   json_t *node = root;

   if (path[0] != 0)
   {
      if (path[0] != '/')   // RFC 6901: a non-empty pointer must start with '/'
      {
         MemFree(path);
         return String();
      }

      const char *p = path + 1;
      while(true)
      {
         char token[256];
         size_t ti = 0;
         while((*p != 0) && (*p != '/') && (ti < sizeof(token) - 1))
         {
            if ((*p == '~') && (*(p + 1) == '1'))
            {
               token[ti++] = '/';
               p += 2;
            }
            else if ((*p == '~') && (*(p + 1) == '0'))
            {
               token[ti++] = '~';
               p += 2;
            }
            else
            {
               token[ti++] = *p++;
            }
         }
         token[ti] = 0;

         // Fail closed if a segment exceeds the token buffer: a silently
         // truncated key must not be matched against a (possibly different)
         // JSON key, and the leftover bytes must not be reparsed as a new
         // segment. An over-long pointer segment is treated as "no match".
         if ((*p != 0) && (*p != '/'))
         {
            MemFree(path);
            return String();
         }

         if (json_is_object(node))
         {
            node = json_object_get(node, token);
         }
         else if (json_is_array(node))
         {
            char *eptr;
            long index = strtol(token, &eptr, 10);
            node = ((token[0] != 0) && (*eptr == 0) && (index >= 0)) ? json_array_get(node, static_cast<size_t>(index)) : nullptr;
         }
         else
         {
            node = nullptr;   // cannot descend into a scalar
         }

         if ((node == nullptr) || (*p == 0))
            break;
         p++;   // skip the '/' separator
      }
   }

   MemFree(path);

   if (node == nullptr)
      return String();

   if (json_is_string(node))
      return String(TStringFromUTF8String(json_string_value(node)), -1, Ownership::True);
   if (json_is_integer(node))
   {
      TCHAR buffer[32];
      _sntprintf(buffer, 32, INT64_FMT, static_cast<int64_t>(json_integer_value(node)));
      return String(buffer);
   }
   if (json_is_real(node))
   {
      // Serialize through jansson rather than _sntprintf("%f", ...): "%f"
      // forces exactly six decimal places (1.5 -> "1.500000") and honours the
      // C runtime LC_NUMERIC locale (decimal separator could be ','), both of
      // which would spuriously fail the comparison against SuccessValue.
      // jansson's serializer is locale-independent (it forces '.') and emits
      // a compact %g-style form (1.5 -> "1.5", 2.0 -> "2.0").
      char *s = json_dumps(node, JSON_ENCODE_ANY | JSON_REAL_PRECISION(15));
      if (s == nullptr)
         return String();
      TCHAR buffer[64];
      _sntprintf(buffer, 64, _T("%hs"), s);
      MemFree(s);
      return String(buffer);
   }
   if (json_is_true(node))
      return String(_T("true"));
   if (json_is_false(node))
      return String(_T("false"));
   if (json_is_null(node))
      return String(_T("null"));

   return String();   // objects / arrays have no scalar string form
}
