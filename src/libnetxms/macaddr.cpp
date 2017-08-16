/*
** NetXMS - Network Management System
** Utility Library
** Copyright (C) 2003-2017 Raden Solutions
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
         toStringInternal(buffer, _T('.'));
         break;
      case MAC_ADDR_BYTEPAIR_DOT_SEPARATED:
         toStringInternal(buffer, _T('.'), true);
         break;
   }
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
      if(!bytePair || (i % 2 == 0))
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

   int stringSize;
   switch(notation)
   {
      case MAC_ADDR_FLAT_STRING:
         stringSize = m_length * 2;
         break;
      case MAC_ADDR_COLON_SEPARATED:
      case MAC_ADDR_HYPHEN_SEPARATED:
      case MAC_ADDR_DOT_SEPARATED:
         stringSize = m_length * 2 + m_length/2; //-1 separator +1 for
         break;
      case MAC_ADDR_BYTEPAIR_DOT_SEPARATED:
      case MAC_ADDR_BYTEPAIR_COLON_SEPARATED:
         stringSize = m_length * 2 + m_length/2; //-1 separator +1 for
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
   if (str == NULL || strlen(str) > 16)
      return MacAddress();

   BYTE buffer[16];
   size_t size = StrToBinA(str, buffer, strlen(str));

   return MacAddress(buffer, size);
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
