/* 
** NetXMS - Network Management System
** SNMP support library
** Copyright (C) 2003-2023 Victor Kirhenshtein
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
** File: oid.cpp
**
**/

#include "libnxsnmp.h"

/**
 * Create empty OID with length 0
 */
SNMP_ObjectId::SNMP_ObjectId()
{
   m_length = 0;
   m_value = nullptr;
}

/**
 * Copy constructor
 */
SNMP_ObjectId::SNMP_ObjectId(const SNMP_ObjectId &src)
{
   m_length = src.m_length;
   m_value = MemCopyArray(src.m_value, m_length);
}

/**
 * Create new OID from base OID and single element suffix
 */
SNMP_ObjectId::SNMP_ObjectId(const SNMP_ObjectId &base, uint32_t suffix)
{
   m_length = base.m_length + 1;
   m_value = MemAllocArrayNoInit<uint32_t>(m_length);
   memcpy(m_value, base.m_value, base.m_length * sizeof(uint32_t));
   m_value[m_length - 1] = suffix;
}

/**
 * Create new OID from base OID and suffix of given length
 */
SNMP_ObjectId::SNMP_ObjectId(const SNMP_ObjectId &base, uint32_t *suffix, size_t length)
{
   m_length = base.m_length + length;
   m_value = MemAllocArrayNoInit<uint32_t>(m_length);
   memcpy(m_value, base.m_value, base.m_length * sizeof(uint32_t));
   memcpy(&m_value[base.m_length], suffix, length * sizeof(uint32_t));
}

/**
 * Create OID from existing binary value
 */
SNMP_ObjectId::SNMP_ObjectId(const uint32_t *value, size_t length)
{
   m_length = length;
   m_value = (length > 0) ? MemCopyArray(value, length) : nullptr;
}

/**
 * SNMP_ObjectId destructor
 */
SNMP_ObjectId::~SNMP_ObjectId()
{
   MemFree(m_value);
}

/**
 * Operator =
 */
SNMP_ObjectId& SNMP_ObjectId::operator =(const SNMP_ObjectId &src)
{
   if (&src == this)
      return *this;
   MemFree(m_value);
   m_length = src.m_length;
   m_value = MemCopyArray(src.m_value, m_length);
   return *this;
}

/**
 * Get OID value as text
 */
String SNMP_ObjectId::toString() const
{
   TCHAR buffer[MAX_OID_LEN * 5];
   SnmpConvertOIDToText(m_length, m_value, buffer, MAX_OID_LEN * 5);
   return String(buffer);
}

/**
 * Get OID value as text
 */
TCHAR *SNMP_ObjectId::toString(TCHAR *buffer, size_t bufferSize) const
{
   SnmpConvertOIDToText(m_length, m_value, buffer, bufferSize);
   return buffer;
}

/**
 * Compare this OID with another
 */
int SNMP_ObjectId::compare(const TCHAR *oid) const
{
   uint32_t buffer[MAX_OID_LEN];
   size_t length = SnmpParseOID(oid, buffer, MAX_OID_LEN);
   if (length == 0)
      return OID_ERROR;
   return compare(buffer, length);
}

/**
 * Compare this OID to another. Possible return values are:
 *    OID_EQUAL     both OIDs are equal
 *    OID_SHORTER   this OID is shorter than given, but common parts are equal
 *    OID_LONGER    this OID is longer than given, but common parts are equal
 *    OID_PRECEDING this OID preceding given OID (less than given OID)
 *    OID_FOLLOWING this OID following given OID (greater than given OID)
 */
int SNMP_ObjectId::compare(const uint32_t *oid, size_t length) const
{
   if ((oid == nullptr) || (length == 0) || (m_value == nullptr))
      return OID_ERROR;

   size_t stop = std::min(length, m_length);
   for(size_t i = 0; i < stop; i++)
   {
      if (m_value[i] != oid[i])
         return (m_value[i] < oid[i]) ? OID_PRECEDING : OID_FOLLOWING;
   }

   return (length == m_length) ? OID_EQUAL : ((length < m_length) ? OID_LONGER : OID_SHORTER);
}

/**
 * Compare this OID to another
 */
int SNMP_ObjectId::compare(const SNMP_ObjectId& oid) const
{
	return compare(oid.value(), oid.length());
}

/**
 * Set new value
 */
void SNMP_ObjectId::setValue(const uint32_t *value, size_t length)
{
   MemFree(m_value);
   m_length = length;
   m_value = MemCopyArray(value, length);
}

/**
 * Extend value by one subid
 *
 * @param subId sub-identifier to add
 */
void SNMP_ObjectId::extend(uint32_t subId)
{
   m_value = MemReallocArray(m_value, m_length + 1);
   m_value[m_length++] = subId;
}

/**
 * Extend value by multiple subids
 *
 * @param subId sub-identifier to add
 * @param length length of sub-identifier to add
 */
void SNMP_ObjectId::extend(const uint32_t *subId, size_t length)
{
   m_value = MemReallocArray(m_value, m_length + length);
   memcpy(&m_value[m_length], subId, length * sizeof(uint32_t));
   m_length += length;
}

/**
 * Truncate value by given number of sub-identifiers
 *
 * @param count number of sub-identifiers to remove
 */
void SNMP_ObjectId::truncate(size_t count)
{
   if (count < m_length)
      m_length -= count;
   else
      m_length = 0;
}

/**
 * Parse OID
 */
SNMP_ObjectId SNMP_ObjectId::parse(const TCHAR *oid)
{
   uint32_t buffer[MAX_OID_LEN];
   size_t length = SnmpParseOID(oid, buffer, MAX_OID_LEN);
   return SNMP_ObjectId(buffer, length);
}
