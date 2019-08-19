/*
** NetXMS - Network Management System
** Utility Library
** Copyright (C) 2003-2019 Raden Solutions
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published
** by the Free Software Foundation; either version 3 of the License, or
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
** File: macaddr.cpp
**
**/

#include "libnetxms.h"
#include <nxcpapi.h>
#include <netxms-regex.h>

/**
 * Predefined invalid MAC address object
 */
const MacAddress LIBNETXMS_EXPORTABLE MacAddress::NONE;

/**
 * Predefined MAC address object 00:00:00:00:00:00
 */
const MacAddress LIBNETXMS_EXPORTABLE MacAddress::ZERO(MAC_ADDR_LENGTH);

/**
 * Returns true if MAC address is valid (has non-zero length and do not consists of all zeroes
 */
bool MacAddress::isValid() const
{
   for(size_t i = 0; i < m_length; i++)
      if (m_value[i] != 0)
         return true;
   return false;
}

/**
 * Returns true if it is the broadcast address
 */
bool MacAddress::isBroadcast() const
{
   if (m_length == 0)
      return false;
   for(size_t i = 0; i < m_length; i++)
      if (m_value[i] != 0xFF)
         return false;
   return true;
}

/**
 * Returns string representaiton of mac address
 */
TCHAR *MacAddress::toString(TCHAR *buffer, MacAddressNotation notation) const
{
   switch(notation)
   {
      case MacAddressNotation::FLAT_STRING:
         BinToStr(m_value, m_length, buffer);
         break;
      case MacAddressNotation::COLON_SEPARATED:
         toStringInternal(buffer, _T(':'));
         break;
      case MacAddressNotation::BYTEPAIR_COLON_SEPARATED:
         toStringInternal(buffer, _T(':'), true);
         break;
      case MacAddressNotation::HYPHEN_SEPARATED:
         toStringInternal(buffer, _T('-'));
         break;
      case MacAddressNotation::DOT_SEPARATED:
         toStringInternal3(buffer, _T('.'));
         break;
      case MacAddressNotation::BYTEPAIR_DOT_SEPARATED:
         toStringInternal(buffer, _T('.'), true);
         break;
      case MacAddressNotation::DECIMAL_DOT_SEPARATED:
         toStringInternalDecimal(buffer, _T('.'));
         break;
   }
   return buffer;
}

/**
 * Returns string representaiton of mac address
 */
String MacAddress::toString(MacAddressNotation notation) const
{
   if (m_length == 0)
      return String();

   TCHAR buffer[64]; // max_length(16) * 4
   return String(toString(buffer, notation));
}

/**
 * Internal method to string for inserting separator every third char
 */
TCHAR *MacAddress::toStringInternal3(TCHAR *buffer, const TCHAR separator) const
{
   TCHAR *curr = buffer;

   for(size_t i = 0; i < m_length; i++)
   {
      *curr++ = bin2hex(m_value[i] >> 4);
      if (((curr+1) - buffer) % 4 == 0)
         *curr++ = separator;
      *curr++ = bin2hex(m_value[i] & 15);
      if (((curr+1) - buffer) % 4 == 0)
         *curr++ = separator;
   }
   *(curr - 1) = 0;
   return buffer;
}

/**
 * Internal method to string
 */
TCHAR *MacAddress::toStringInternal(TCHAR *buffer, const TCHAR separator, bool bytePair) const
{
   TCHAR *curr = buffer;

   for(size_t i = 0; i < m_length; i++)
   {
      *curr++ = bin2hex(m_value[i] >> 4);
      *curr++ = bin2hex(m_value[i] & 15);
      if (!bytePair || (((i+1) % 2 == 0)))
         *curr++ = separator;
   }
   *(curr - 1) = 0;
   return buffer;
}

/**
 * Internal method to string
 */
TCHAR *MacAddress::toStringInternalDecimal(TCHAR *buffer, const TCHAR separator) const
{
   TCHAR *curr = buffer;

   for(size_t i = 0; i < m_length; i++)
   {
      if (i > 0)
         *curr++ = separator;
      _sntprintf(curr, 4, _T("%u"), static_cast<unsigned int>(m_value[i]));
      curr += _tcslen(curr);
   }
   return buffer;
}

/**
 * Parse string as MAC address
 */
MacAddress MacAddress::parse(const char *str)
{
   if (str == NULL || strlen(str) > 23)
      return MacAddress(MacAddress::ZERO);

   char exp1[254] = { "^([0-9a-fA-F]{2})[ :-]?"
                      "([0-9a-fA-F]{2})[ .:-]?"
                      "([0-9a-fA-F]{2})[ :-]?"
                      "([0-9a-fA-F]{2})[ .:-]?"
                      "([0-9a-fA-F]{2})?[ :-]?"
                      "([0-9a-fA-F]{2})?[ .:-]?"
                      "([0-9a-fA-F]{2})?[ :-]?"
                      "([0-9a-fA-F]{2})?$" };

   char exp2[128] = { "^([0-9a-fA-F]{3})\\."
                       "([0-9a-fA-F]{3})\\."
                       "([0-9a-fA-F]{3})\\."
                       "([0-9a-fA-F]{3})$" };

   String mac;
   const char *errptr;
   int erroffset;
   pcre *compRegex = pcre_compile(exp1, PCRE_EXTENDED, &errptr, &erroffset, NULL);
   if (compRegex != NULL)
   {
      int ovector[30];
      if (pcre_exec(compRegex, NULL, str, strlen(str), 0, 0, ovector, 30) == 9)
      {
         for(int i = 1; i <= 8; i++)
            mac.appendMBString(str + ovector[i * 2], (ovector[i * 2 + 1] - ovector[i * 2]), CP_ACP);
         pcre_free(compRegex);
      }
      else
      {
         pcre_free(compRegex);
         pcre *compRegex = pcre_compile(exp2, PCRE_EXTENDED, &errptr, &erroffset, NULL);
         if (compRegex != NULL)
         {
            if (pcre_exec(compRegex, NULL, str, strlen(str), 0, 0, ovector, 30) == 5)
            {
               for(int i = 1; i <= 4; i++)
                  mac.appendMBString(str + ovector[i * 2], (ovector[i * 2 + 1] - ovector[i * 2]), CP_ACP);
            }
            pcre_free(compRegex);
         }
      }
   }

   if (mac.length() > 0)
   {
      BYTE buffer[16];
      size_t size = StrToBin(mac, buffer, mac.length());
      return MacAddress(buffer, size);
   }

   return MacAddress(MacAddress::ZERO);
}

/**
 * Parse string as MAC address (WCHAR version)
 */
MacAddress MacAddress::parse(const WCHAR *str)
{
   char mb[256];
   WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR, str, -1, mb, 256, NULL, NULL);
   return parse(mb);
}
