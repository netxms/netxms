/* 
** NetXMS - Network Management System
** SNMP support library
** Copyright (C) 2003, 2004 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** $module: variable.cpp
**
**/

#include "libnxsnmp.h"


//
// SNMP_Variable default constructor
//

SNMP_Variable::SNMP_Variable()
{
   m_pName = NULL;
   m_pValue = NULL;
   m_dwType = ASN_NULL;
   m_dwValueLength = 0;
}


//
// SNMP_Variable destructor
//

SNMP_Variable::~SNMP_Variable()
{
   delete m_pName;
   safe_free(m_pValue);
}


//
// Parse variable record in PDU
//

BOOL SNMP_Variable::Parse(BYTE *pData, DWORD dwVarLength)
{
   BYTE *pbCurrPos;
   DWORD dwType, dwLength, dwIdLength;
   SNMP_OID *oid;
   BOOL bResult = FALSE;

   // Object ID
   if (!BER_DecodeIdentifier(pData, dwVarLength, &dwType, &dwLength, &pbCurrPos, &dwIdLength))
      return FALSE;
   if (dwType != ASN_OBJECT_ID)
      return FALSE;

   oid = (SNMP_OID *)malloc(sizeof(SNMP_OID));
   memset(oid, 0, sizeof(SNMP_OID));
   if (BER_DecodeContent(dwType, pbCurrPos, dwLength, (BYTE *)oid))
   {
      m_pName = new SNMP_ObjectId(oid->dwLength, oid->pdwValue);
      dwVarLength -= dwLength + dwIdLength;
      pbCurrPos += dwLength;
      bResult = TRUE;
   }
   safe_free(oid->pdwValue);
   free(oid);

   if (bResult)
   {
      bResult = FALSE;
      if (BER_DecodeIdentifier(pbCurrPos, dwVarLength, &m_dwType, &dwLength, &pbCurrPos, &dwIdLength))
      {
         switch(m_dwType)
         {
            case ASN_OBJECT_ID:
               oid = (SNMP_OID *)malloc(sizeof(SNMP_OID));
               memset(oid, 0, sizeof(SNMP_OID));
               if (BER_DecodeContent(m_dwType, pbCurrPos, dwLength, (BYTE *)oid))
               {
                  m_dwValueLength = oid->dwLength * sizeof(DWORD);
                  m_pValue = (BYTE *)oid->pdwValue;
                  bResult = TRUE;
               }
               else
               {
                  safe_free(oid->pdwValue);
               }
               free(oid);
               break;
            case ASN_INTEGER:
            case ASN_COUNTER32:
            case ASN_GAUGE32:
            case ASN_TIMETICKS:
            case ASN_UINTEGER32:
               m_pValue = (BYTE *)malloc(8);
               bResult = BER_DecodeContent(m_dwType, pbCurrPos, dwLength, m_pValue);
               break;
            default:
               m_dwValueLength = dwLength;
               m_pValue = (BYTE *)nx_memdup(pbCurrPos, dwLength);
               bResult = TRUE;
               break;
         }
      }
   }

   return bResult;
}


//
// Get value as unsigned integer
//

DWORD SNMP_Variable::GetValueAsUInt(void)
{
   DWORD dwValue;

   switch(m_dwType)
   {
      case ASN_INTEGER:
      case ASN_COUNTER32:
      case ASN_GAUGE32:
      case ASN_TIMETICKS:
      case ASN_UINTEGER32:
      case ASN_IP_ADDR:
         dwValue = *((DWORD *)m_pValue);
         break;
      default:
         dwValue = 0;
         break;
   }

   return dwValue;
}


//
// Get value as signed integer
//

long SNMP_Variable::GetValueAsInt(void)
{
   long iValue;

   switch(m_dwType)
   {
      case ASN_INTEGER:
      case ASN_COUNTER32:
      case ASN_GAUGE32:
      case ASN_TIMETICKS:
      case ASN_UINTEGER32:
      case ASN_IP_ADDR:
         iValue = *((long *)m_pValue);
         break;
      default:
         iValue = 0;
         break;
   }

   return iValue;
}


//
// Get value as string
//

TCHAR *SNMP_Variable::GetValueAsString(TCHAR *pszBuffer, DWORD dwBufferSize)
{
   return NULL;
}


//
// Get value as object id
//

SNMP_ObjectId *SNMP_Variable::GetValueAsObjectId(void)
{
   SNMP_ObjectId *oid = NULL;

   if (m_dwType == ASN_OBJECT_ID)
   {
      oid = new SNMP_ObjectId(m_dwValueLength / sizeof(DWORD), (DWORD *)m_pValue);
   }
   return oid;
}
