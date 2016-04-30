/* 
** NetXMS - Network Management System
** SNMP support library
** Copyright (C) 2003-2013 Victor Kirhenshtein
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
   m_value = NULL;
   m_textValue = NULL;
}

/**
 * Copy constructor
 */
SNMP_ObjectId::SNMP_ObjectId(SNMP_ObjectId *src)
{
   m_length = src->m_length;
   m_value = (UINT32 *)nx_memdup(src->m_value, sizeof(UINT32) * m_length);
   if (src->m_textValue != NULL)
   {
      m_textValue = _tcsdup(src->m_textValue);
   }
   else
   {
      m_textValue = NULL;
      convertToText();
   }
}

/**
 * Create OID from existing binary value
 */
SNMP_ObjectId::SNMP_ObjectId(size_t length, const UINT32 *value)
{
   m_length = (UINT32)length;
   m_value = (UINT32 *)nx_memdup(value, sizeof(UINT32) * length);
   m_textValue = NULL;
   convertToText();
}

/**
 * SNMP_ObjectId destructor
 */
SNMP_ObjectId::~SNMP_ObjectId()
{
   safe_free(m_value);
   safe_free(m_textValue);
}

/**
 * Convert binary representation to text
 */
void SNMP_ObjectId::convertToText()
{
   m_textValue = (TCHAR *)realloc(m_textValue, sizeof(TCHAR) * (m_length * 6 + 1));
   SNMPConvertOIDToText(m_length, m_value, m_textValue, m_length * 6 + 1);
}

/**
 * Compare this OID with another
 */
int SNMP_ObjectId::compare(const TCHAR *pszOid)
{
   UINT32 dwBuffer[MAX_OID_LEN];
   size_t length = SNMPParseOID(pszOid, dwBuffer, MAX_OID_LEN);
   if (length == 0)
      return OID_ERROR;
   return compare(dwBuffer, length);
}

/**
 * Compare this OID to another. Possible return values are:
 *    OID_EQUAL     both OIDs are equal
 *    OID_SHORTER   this OID is shorter than given, but common parts are equal
 *    OID_LONGER    this OID is longer than given, but common parts are equal
 *    OID_PRECEDING this OID preceding given OID (less than given OID)
 *    OID_FOLLOWING this OID following given OID (greater than given OID)
 */
int SNMP_ObjectId::compare(const UINT32 *oid, size_t length)
{
   if ((oid == NULL) || (length == 0) || (m_value == NULL))
      return OID_ERROR;

   size_t stop = min(length, m_length);
   for(size_t i = 0; i < stop; i++)
   {
      if (m_value[i] != oid[i])
         return (m_value[i] < oid[i]) ? OID_PRECEDING : OID_FOLLOWING;
   }

   return (length == m_length) ? OID_EQUAL : 
            ((length < m_length) ? OID_SHORTER : OID_LONGER);
}

/**
 * Compare this OID to another
 */
int SNMP_ObjectId::compare(SNMP_ObjectId *oid)
{
	if (oid == NULL)
      return OID_ERROR;
	return compare(oid->getValue(), oid->getLength());
}

/**
 * Set new value
 */
void SNMP_ObjectId::setValue(UINT32 *value, size_t length)
{
   safe_free(m_value);
   m_length = (UINT32)length;
   m_value = (UINT32 *)nx_memdup(value, sizeof(UINT32) * length);
   convertToText();
}

/**
 * Extend value by one subid
 *
 * @param subId sub-identifier to add
 */
void SNMP_ObjectId::extend(UINT32 subId)
{
   m_value = (UINT32 *)realloc(m_value, sizeof(UINT32) * (m_length + 1));
   m_value[m_length++] = subId;
   convertToText();
}
