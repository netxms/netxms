/*
** NetXMS - Network Management System
** Utility Library
** Copyright (C) 2003-2018 Raden Solutions
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
 * Returns true if it is the multicast address
 */
bool MacAddress::isMulticast() const
{
   return (m_length == 6) ? (m_value[0] & 0x01) != 0 : false;
}

/**
 * Returns true if it is the broadcast address
 */
bool MacAddress::isBroadcast() const
{
   return (m_length == 6) ? !memcmp(m_value, "\xFF\xFF\xFF\xFF\xFF\xFF", 6) : false;
}

/**
 * Returns true if addrese are equals
 */
bool MacAddress::equals(const MacAddress &a) const
{
   if(a.length() == m_length)
   {
      return !memcmp(m_value, a.value(), m_length);
   }
   return false;
}

/**
 * Returns string representaiton of mac address
 */
TCHAR *MacAddress::toString(TCHAR *buffer, MacAddressNotation notation) const
{
   switch(notation)
   {
      case MAC_ADDR_FLAT_STRING:
         BinToStr(m_value, m_length, buffer);
         break;
      case MAC_ADDR_COLON_SEPARATED:
         toStringInternal(buffer, _T(':'));
         break;
      case MAC_ADDR_BYTEPAIR_COLON_SEPARATED:
         toStringInternal(buffer, _T(':'), true);
         break;
      case MAC_ADDR_HYPHEN_SEPARATED:
         toStringInternal(buffer, _T('-'));
         break;
      case MAC_ADDR_DOT_SEPARATED:
         toStringInternal3(buffer, _T('.'));
         break;
      case MAC_ADDR_BYTEPAIR_DOT_SEPARATED:
         toStringInternal(buffer, _T('.'), true);
         break;
   }
   return buffer;
}

/**
 * Internal method to string for inserting separator every third char
 */
TCHAR *MacAddress::toStringInternal3(TCHAR *buffer, const TCHAR separator) const
{
   TCHAR *curr = buffer;

   for(int i = 0; i < m_length; i++)
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

   for(int i = 0; i < m_length; i++)
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
 * Returns string representaiton of mac address
 */
String MacAddress::toString(MacAddressNotation notation) const
{
   if (m_length == 0)
      return String();

   size_t stringSize;
   switch(notation)
   {
      case MAC_ADDR_FLAT_STRING:
         stringSize = m_length * 2 + 1;
         break;
      case MAC_ADDR_COLON_SEPARATED:
      case MAC_ADDR_HYPHEN_SEPARATED:
      case MAC_ADDR_DOT_SEPARATED:
         stringSize = m_length * 2 + m_length; //-1 separator +1 for 0 termination
         break;
      case MAC_ADDR_BYTEPAIR_DOT_SEPARATED:
      case MAC_ADDR_BYTEPAIR_COLON_SEPARATED:
         stringSize = m_length * 2 + m_length / 2; //-1 separator +1 for 0 termination
         break;
   }
   TCHAR *buf = (TCHAR *)malloc(stringSize * sizeof(TCHAR));
   String str(toString(buf, notation));
   free(buf);
   return str;
}

/**
 * Parse string as MAC address
 */
MacAddress MacAddress::parse(const char *str)
{
   if (str == NULL || strlen(str) > 23)
      return MacAddress();

   regex_t compRegex;
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
   if (tre_regcomp(&compRegex, exp1, REG_EXTENDED) == 0)
   {
      regmatch_t match[9];
      if (tre_regexec(&compRegex, str, 9, match, 0) == 0)
      {
         for(int i = 1; i < 9; i++)
            mac.appendMBString(str+match[i].rm_so, (match[i].rm_eo - match[i].rm_so), CP_ACP);
      }
      else
      {
         regfree(&compRegex);
         if (tre_regcomp(&compRegex, exp2, REG_EXTENDED) == 0)
         {
            regmatch_t match[5];
            if (tre_regexec(&compRegex, str, 5, match, 0) == 0)
            {
               for(int i = 1; i < 5; i++)
                  mac.appendMBString(str+match[i].rm_so, (match[i].rm_eo - match[i].rm_so), CP_ACP);
            }
         }
      }
      regfree(&compRegex);
   }

   if (mac.length() > 0)
   {
      BYTE buffer[16];
      size_t size = StrToBin(mac, buffer, mac.length());
      return MacAddress(buffer, size);
   }

   return MacAddress();
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
