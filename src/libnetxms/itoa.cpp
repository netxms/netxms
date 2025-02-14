/*
 ** NetXMS - Network Management System
 ** NetXMS Foundation Library
 ** Copyright (C) 2003-2025 Raden Solutions
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
 ** File: itoa.cpp
 **
 **/

#include "libnetxms.h"

/**
 * Convert signed integer to string
 */
template<typename _Ti, typename _Ta> static inline _Ta *SignedIntegerToString(_Ti value, _Ta *str, int base)
{
   if (value == 0)
   {
      str[0] = '0';
      str[1] = 0;
      return str;
   }

   _Ta buffer[64];
   _Ta *p = str, *t = buffer;
   if (value < 0)
   {
      *p++ = '-';
      _Ti rem = -(value % base);
      *t++ = static_cast<_Ta>((rem < 10) ? (rem + '0') : (rem - 10 + 'a'));
      value /= -base;   // This will negate resulting value
   }

   while(value > 0)
   {
      _Ti rem = value % base;
      *t++ = static_cast<_Ta>((rem < 10) ? (rem + '0') : (rem - 10 + 'a'));
      value /= base;
   }

   t--;
   while(t >= buffer)
      *p++ = *t--;
   *p = 0;
   return str;
}

/**
 * Convert unsigned integer to string
 */
template<typename _Ti, typename _Ta> static inline _Ta *UnsignedIntegerToString(_Ti value, _Ta *str, int base)
{
   _Ta buffer[64];
   _Ta *p = str, *t = buffer;
   do
   {
      _Ti rem = value % base;
      *t++ = static_cast<_Ta>((rem < 10) ? (rem + '0') : (rem - 10 + 'a'));
      value = value / base;
   } while(value > 0);

   t--;
   while(t >= buffer)
      *p++ = *t--;
   *p = 0;
   return str;
}

/**
 * Convert 32 bit integer to string
 */
char LIBNETXMS_EXPORTABLE *IntegerToString(int32_t value, char *str, int base)
{
   return SignedIntegerToString(value, str, base);
}

/**
 * Convert 32 bit integer to wide string
 */
WCHAR LIBNETXMS_EXPORTABLE *IntegerToString(int32_t value, WCHAR *str, int base)
{
   return SignedIntegerToString(value, str, base);
}

/**
 * Convert unsigned 32 bit integer to string
 */
char LIBNETXMS_EXPORTABLE *IntegerToString(uint32_t value, char *str, int base)
{
   return UnsignedIntegerToString(value, str, base);
}

/**
 * Convert unsigned 32 bit integer to wide string
 */
WCHAR LIBNETXMS_EXPORTABLE *IntegerToString(uint32_t value, WCHAR *str, int base)
{
   return UnsignedIntegerToString(value, str, base);
}

/**
 * Convert 64 bit integer to string
 */
char LIBNETXMS_EXPORTABLE *IntegerToString(int64_t value, char *str, int base)
{
   return SignedIntegerToString(value, str, base);
}

/**
 * Convert 64 bit integer to wide string
 */
WCHAR LIBNETXMS_EXPORTABLE *IntegerToString(int64_t value, WCHAR *str, int base)
{
   return SignedIntegerToString(value, str, base);
}

/**
 * Convert unsigned 64 bit integer to string
 */
char LIBNETXMS_EXPORTABLE *IntegerToString(uint64_t value, char *str, int base)
{
   return UnsignedIntegerToString(value, str, base);
}

/**
 * Convert unsigned 64 bit integer to wide string
 */
WCHAR LIBNETXMS_EXPORTABLE *IntegerToString(uint64_t value, WCHAR *str, int base)
{
   return UnsignedIntegerToString(value, str, base);
}
