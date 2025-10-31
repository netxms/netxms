/* 
** NetXMS - Network Management System
** SNMP support library
** Copyright (C) 2003-2025 Victor Kirhenshtein
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
 * Create new OID from base OID and single element suffix
 */
SNMP_ObjectId::SNMP_ObjectId(const SNMP_ObjectId &base, uint32_t suffix)
{
   m_length = base.m_length + 1;
   m_value = (m_length <= SNMP_OID_INTERNAL_BUFFER_SIZE) ? m_internalBuffer : MemAllocArrayNoInit<uint32_t>(m_length);
   memcpy(m_value, base.m_value, base.m_length * sizeof(uint32_t));
   m_value[m_length - 1] = suffix;
}

/**
 * Create new OID from base OID and suffix of given length
 */
SNMP_ObjectId::SNMP_ObjectId(const SNMP_ObjectId &base, uint32_t *suffix, size_t length)
{
   m_length = base.m_length + length;
   m_value = (m_length <= SNMP_OID_INTERNAL_BUFFER_SIZE) ? m_internalBuffer : MemAllocArrayNoInit<uint32_t>(m_length);
   memcpy(m_value, base.m_value, base.m_length * sizeof(uint32_t));
   memcpy(&m_value[base.m_length], suffix, length * sizeof(uint32_t));
}

/**
 * Copy assignment operator
 */
SNMP_ObjectId& SNMP_ObjectId::operator =(const SNMP_ObjectId &src)
{
   if (&src == this)
      return *this;
   if (m_value != m_internalBuffer)
      MemFree(m_value);
   m_length = src.m_length;
   if (m_length <= SNMP_OID_INTERNAL_BUFFER_SIZE)
   {
      m_value = m_internalBuffer;
      memcpy(m_value, src.m_value, m_length * sizeof(uint32_t));
   }
   else
   {
      m_value = MemAllocArrayNoInit<uint32_t>(m_length);
   }
   return *this;
}

/**
 * Move assignment operator
 */
SNMP_ObjectId& SNMP_ObjectId::operator =(SNMP_ObjectId&& src)
{
   if (&src == this)
      return *this;
   if (m_value != m_internalBuffer)
      MemFree(m_value);
   m_length = src.m_length;
   if (src.m_value == src.m_internalBuffer)
   {
      m_value = m_internalBuffer;
      memcpy(m_value, src.m_value, m_length * sizeof(uint32_t));
   }
   else
   {
      m_value = src.m_value;
   }
   src.m_length = 0;
   src.m_value = src.m_internalBuffer;
   return *this;
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
 * Set new value
 */
void SNMP_ObjectId::setValue(const uint32_t *value, size_t length)
{
   if (m_value != m_internalBuffer)
      MemFree(m_value);
   m_length = length;
   if (m_length <= SNMP_OID_INTERNAL_BUFFER_SIZE)
   {
      m_value = m_internalBuffer;
      memcpy(m_value, value, m_length * sizeof(uint32_t));
   }
   else
   {
      m_value = MemCopyArray(value, length);
   }
}

/**
 * Extend value by one subid
 *
 * @param subId sub-identifier to add
 */
void SNMP_ObjectId::extend(uint32_t subId)
{
   if ((m_value == m_internalBuffer) && (m_length < SNMP_OID_INTERNAL_BUFFER_SIZE))
   {
      m_value[m_length++] = subId;
   }
   else if (m_value == m_internalBuffer)
   {
      uint32_t *newValue = MemAllocArrayNoInit<uint32_t>(m_length + 1);
      memcpy(newValue, m_value, m_length * sizeof(uint32_t));
      newValue[m_length++] = subId;
      m_value = newValue;
   }
   else
   {
      m_value = MemReallocArray(m_value, m_length + 1);
      m_value[m_length++] = subId;
   }
}

/**
 * Extend value by multiple subids
 *
 * @param subId sub-identifier to add
 * @param length length of sub-identifier to add
 */
void SNMP_ObjectId::extend(const uint32_t *subId, size_t length)
{
   if (m_value == m_internalBuffer)
   {
      if ((m_length + length > SNMP_OID_INTERNAL_BUFFER_SIZE))
      {
         uint32_t *newValue = MemAllocArrayNoInit<uint32_t>(m_length + length);
         memcpy(newValue, m_value, m_length * sizeof(uint32_t));
         m_value = newValue;
      }
   }
   else
   {
      m_value = MemReallocArray(m_value, m_length + length);
      memcpy(&m_value[m_length], subId, length * sizeof(uint32_t));
   }
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
