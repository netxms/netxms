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
   m_dwLength = 0;
   m_pdwValue = NULL;
   m_pszTextValue = NULL;
}

/**
 * Create OID from existing binary value
 */
SNMP_ObjectId::SNMP_ObjectId(UINT32 dwLength, const UINT32 *pdwValue)
{
   m_dwLength = dwLength;
   m_pdwValue = (UINT32 *)nx_memdup(pdwValue, sizeof(UINT32) * dwLength);
   m_pszTextValue = NULL;
   convertToText();
}

/**
 * SNMP_ObjectId destructor
 */
SNMP_ObjectId::~SNMP_ObjectId()
{
   safe_free(m_pdwValue);
   safe_free(m_pszTextValue);
}

/**
 * Convert binary representation to text
 */
void SNMP_ObjectId::convertToText()
{
   m_pszTextValue = (TCHAR *)realloc(m_pszTextValue, sizeof(TCHAR) * (m_dwLength * 6 + 1));
   SNMPConvertOIDToText(m_dwLength, m_pdwValue, m_pszTextValue, m_dwLength * 6 + 1);
}

/**
 * Compare this OID with another
 */
int SNMP_ObjectId::compare(const TCHAR *pszOid)
{
   UINT32 dwBuffer[MAX_OID_LEN], dwLength;

   dwLength = SNMPParseOID(pszOid, dwBuffer, MAX_OID_LEN);
   if (dwLength == 0)
      return OID_ERROR;
   return compare(dwBuffer, dwLength);
}

/**
 * Compare this OID to another
 */
int SNMP_ObjectId::compare(const UINT32 *pdwOid, UINT32 dwLen)
{
   if ((pdwOid == NULL) || (dwLen == 0) || (m_pdwValue == NULL))
      return OID_ERROR;

   if (memcmp(m_pdwValue, pdwOid, min(dwLen, m_dwLength) * sizeof(UINT32)))
      return OID_NOT_EQUAL;

   return (dwLen == m_dwLength) ? OID_EQUAL : 
            ((dwLen < m_dwLength) ? OID_SHORTER : OID_LONGER);
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
void SNMP_ObjectId::setValue(UINT32 *pdwValue, UINT32 dwLength)
{
   safe_free(m_pdwValue);
   m_dwLength = dwLength;
   m_pdwValue = (UINT32 *)nx_memdup(pdwValue, sizeof(UINT32) * dwLength);
   convertToText();
}

/**
 * Extend value by one subid
 *
 * @param subId sub-identifier to add
 */
void SNMP_ObjectId::extend(UINT32 subId)
{
   m_pdwValue = (UINT32 *)realloc(m_pdwValue, sizeof(UINT32) * (m_dwLength + 1));
   m_pdwValue[m_dwLength++] = subId;
   convertToText();
}
