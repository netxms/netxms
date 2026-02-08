/*
** NetXMS - Network Management System
** Utility Library
** Copyright (C) 2003-2022 Raden Solutions
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
 * Returns string representation of MAC address (ASCII)
 */
char *MacAddress::toStringA(char *buffer, MacAddressNotation notation) const
{
   if (m_length == 0)
   {
      *buffer = 0;
      return buffer;
   }

   switch(notation)
   {
      case MacAddressNotation::FLAT_STRING:
         BinToStrA(m_value, m_length, buffer);
         break;
      case MacAddressNotation::COLON_SEPARATED:
         toStringInternalA(buffer, ':');
         break;
      case MacAddressNotation::BYTEPAIR_COLON_SEPARATED:
         toStringInternalA(buffer, ':', true);
         break;
      case MacAddressNotation::HYPHEN_SEPARATED:
         toStringInternalA(buffer, '-');
         break;
      case MacAddressNotation::DOT_SEPARATED:
         toStringInternal3A(buffer, '.');
         break;
      case MacAddressNotation::BYTEPAIR_DOT_SEPARATED:
         toStringInternalA(buffer, '.', true);
         break;
      case MacAddressNotation::DECIMAL_DOT_SEPARATED:
         toStringInternalDecimalA(buffer, '.');
         break;
   }
   return buffer;
}

/**
 * Returns string representation of MAC address (wide char)
 */
wchar_t *MacAddress::toStringW(wchar_t *buffer, MacAddressNotation notation) const
{
   if (m_length == 0)
   {
      *buffer = 0;
      return buffer;
   }

   switch(notation)
   {
      case MacAddressNotation::FLAT_STRING:
         BinToStrW(m_value, m_length, buffer);
         break;
      case MacAddressNotation::COLON_SEPARATED:
         toStringInternalW(buffer, L':');
         break;
      case MacAddressNotation::BYTEPAIR_COLON_SEPARATED:
         toStringInternalW(buffer, L':', true);
         break;
      case MacAddressNotation::HYPHEN_SEPARATED:
         toStringInternalW(buffer, L'-');
         break;
      case MacAddressNotation::DOT_SEPARATED:
         toStringInternal3W(buffer, L'.');
         break;
      case MacAddressNotation::BYTEPAIR_DOT_SEPARATED:
         toStringInternalW(buffer, L'.', true);
         break;
      case MacAddressNotation::DECIMAL_DOT_SEPARATED:
         toStringInternalDecimalW(buffer, L'.');
         break;
   }
   return buffer;
}

/**
 * Returns string representation of MAC address
 */
String MacAddress::toString(MacAddressNotation notation) const
{
   if (m_length == 0)
      return String();

   TCHAR buffer[32]; // max_length(8) * 4
   return String(toString(buffer, notation));
}

/**
 * Returns string representation of MAC address for inserting separator every third char (wide char)
 */
wchar_t *MacAddress::toStringInternal3W(wchar_t *buffer, wchar_t separator) const
{
   wchar_t *curr = buffer;
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
 * Returns string representation of MAC address for inserting separator every third char (ASCII)
 */
char *MacAddress::toStringInternal3A(char *buffer, char separator) const
{
   char *curr = buffer;
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
 * Returns string representation of MAC address (wide char)
 */
wchar_t *MacAddress::toStringInternalW(wchar_t *buffer, wchar_t separator, bool bytePair) const
{
   wchar_t *curr = buffer;
   for(size_t i = 0; i < m_length; i++)
   {
      *curr++ = bin2hex(m_value[i] >> 4);
      *curr++ = bin2hex(m_value[i] & 15);
      if (!bytePair || (((i + 1) % 2 == 0)))
         *curr++ = separator;
   }
   *(curr - 1) = 0;
   return buffer;
}

/**
 * Returns string representation of MAC address (ASCII)
 */
char *MacAddress::toStringInternalA(char *buffer, char separator, bool bytePair) const
{
   char *curr = buffer;
   for(size_t i = 0; i < m_length; i++)
   {
      *curr++ = bin2hex(m_value[i] >> 4);
      *curr++ = bin2hex(m_value[i] & 15);
      if (!bytePair || (((i + 1) % 2 == 0)))
         *curr++ = separator;
   }
   *(curr - 1) = 0;
   return buffer;
}

/**
 * Returns string representation of MAC address in decimal format (wide char)
 */
wchar_t *MacAddress::toStringInternalDecimalW(wchar_t *buffer, wchar_t separator) const
{
   wchar_t *curr = buffer;
   for(size_t i = 0; i < m_length; i++)
   {
      if (i > 0)
         *curr++ = separator;
      IntegerToString(m_value[i], curr);
      curr += wcslen(curr);
   }
   return buffer;
}

/**
 * Returns string representation of MAC address in decimal format (ASCII)
 */
char *MacAddress::toStringInternalDecimalA(char *buffer, char separator) const
{
   char *curr = buffer;
   for(size_t i = 0; i < m_length; i++)
   {
      if (i > 0)
         *curr++ = separator;
      IntegerToString(m_value[i], curr);
      curr += strlen(curr);
   }
   return buffer;
}

/**
 * Parse string as MAC address
 */
MacAddress MacAddress::parse(const char *str, bool partialMac)
{
   if (str == NULL || strlen(str) > 23)
      return MacAddress(MacAddress::ZERO);

   char exp1[254] = { "^([0-9a-fA-F]{2})[ :-]?"
                      "([0-9a-fA-F]{2})[ .:-]?"
                      "([0-9a-fA-F]{2})?[ :-]?"
                      "([0-9a-fA-F]{2})?[ .:-]?"
                      "([0-9a-fA-F]{2})?[ :-]?"
                      "([0-9a-fA-F]{2})?[ .:-]?"
                      "([0-9a-fA-F]{2})?[ :-]?"
                      "([0-9a-fA-F]{2})?$" };

   char exp2[256] = { "^([0-9a-fA-F]{3})\\."
                       "([0-9a-fA-F]{3})\\."
                       "([0-9a-fA-F]{3})\\."
                       "([0-9a-fA-F]{3})$|"
                      "^([0-9a-fA-F]{3})\\."
                       "([0-9a-fA-F]{3})?$" };

   StringBuffer mac;
   const char *errptr;
   int erroffset;
   pcre *compRegex = pcre_compile(exp1, PCRE_COMMON_FLAGS_A, &errptr, &erroffset, nullptr);
   if (compRegex != nullptr)
   {
      int ovector[30];
      int cgcount = pcre_exec(compRegex, nullptr, str, static_cast<int>(strlen(str)), 0, 0, ovector, 30);
      if (cgcount >= 7 || (partialMac && cgcount >= 3)) // at least 6 elements for full and 2 for partial
      {
         for(int i = 1; i < cgcount; i++)
            mac.appendMBString(str + ovector[i * 2], (ovector[i * 2 + 1] - ovector[i * 2]));
         pcre_free(compRegex);
      }
      else
      {
         pcre_free(compRegex);
         pcre *compRegex = pcre_compile(exp2, PCRE_COMMON_FLAGS_A, &errptr, &erroffset, nullptr);
         if (compRegex != NULL)
         {
            cgcount = pcre_exec(compRegex, nullptr, str, static_cast<int>(strlen(str)), 0, 0, ovector, 30);
            if (cgcount == 5 || (partialMac && cgcount >= 3))
            {
               for(int i = 1; i < cgcount; i++)
                  mac.appendMBString(str + ovector[i * 2], (ovector[i * 2 + 1] - ovector[i * 2]));
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
 * Parse string as MAC address (wide char version)
 */
MacAddress MacAddress::parse(const wchar_t *str, bool partialMac)
{
   char mb[256];
   wchar_to_mb(str, -1, mb, 256);
   return parse(mb, partialMac);
}
