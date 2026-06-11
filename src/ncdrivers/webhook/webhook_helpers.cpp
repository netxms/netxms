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
 * Token names recognized in templates. All ASCII.
 */
#define TOKEN_RECIPIENT  "recipient"
#define TOKEN_SUBJECT    "subject"
#define TOKEN_BODY       "body"

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
