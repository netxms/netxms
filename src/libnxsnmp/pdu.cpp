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
** $module: pdu.cpp
**
**/

#include "libnxsnmp.h"


//
// SNMP_PDU default constructor
//

SNMP_PDU::SNMP_PDU()
{
   m_dwVersion = SNMP_VERSION_1;
   m_dwCommand = SNMP_INVALID_PDU;
   m_pszCommunity = NULL;
   m_dwNumVariables = 0;
   m_ppVarList = NULL;
   m_pEnterprise = NULL;
}


//
// SNMP_PDU destructor
//

SNMP_PDU::~SNMP_PDU()
{
   DWORD i;

   safe_free(m_pszCommunity);
   delete m_pEnterprise;
   for(i = 0; i < m_dwNumVariables; i++)
      delete m_ppVarList[i];
   safe_free(m_ppVarList);
}


//
// Parse TRAP PDU
//

BOOL SNMP_PDU::ParseTrapPDU(BYTE *pData, DWORD dwPDULength)
{
   DWORD dwType, dwLength;
   BYTE *pbCurrPos = pData;
   SNMP_OID *oid;
   BOOL bResult = FALSE;

   // Enterprise ID
   if (BER_DecodeIdentifier(pbCurrPos, dwPDULength, &dwType, &dwLength, &pbCurrPos))
   {
      if (dwType == ASN_OBJECT_ID)
      {
         oid = (SNMP_OID *)malloc(sizeof(SNMP_OID));
         memset(oid, 0, sizeof(SNMP_OID));
         if (BER_DecodeContent(dwType, pbCurrPos, dwLength, (BYTE *)oid))
         {
            m_pEnterprise = new SNMP_ObjectId(oid->dwLength, oid->pdwValue);
            dwPDULength -= dwLength;
            pbCurrPos += dwLength;

            bResult = TRUE;
         }
         safe_free(oid->pdwValue);
         free(oid);
      }
   }

   return bResult;
}


//
// Create PDU from packet
//

BOOL SNMP_PDU::Parse(BYTE *pRawData, DWORD dwRawLength)
{
   BYTE *pbCurrPos;
   DWORD dwType, dwLength, dwPacketLength;
   BOOL bResult = FALSE;

   // Packet start
   if (!BER_DecodeIdentifier(pRawData, dwRawLength, &dwType, &dwPacketLength, &pbCurrPos))
      return FALSE;
   if (dwType != ASN_SEQUENCE)
      return FALSE;   // Packet should start with SEQUENCE

   // Version
   if (!BER_DecodeIdentifier(pbCurrPos, dwPacketLength, &dwType, &dwLength, &pbCurrPos))
      return FALSE;
   if (dwType != ASN_INTEGER)
      return FALSE;   // Version field should be of integer type
   if (!BER_DecodeContent(dwType, pbCurrPos, dwLength, (BYTE *)&m_dwVersion))
      return FALSE;   // Error parsing content of version field
   pbCurrPos += dwLength;
   dwPacketLength -= dwLength;
   if ((m_dwVersion != SNMP_VERSION_1) && (m_dwVersion != SNMP_VERSION_2C))
      return FALSE;   // Unsupported SNMP version

   // Community string
   if (!BER_DecodeIdentifier(pbCurrPos, dwPacketLength, &dwType, &dwLength, &pbCurrPos))
      return FALSE;
   if (dwType != ASN_OCTET_STRING)
      return FALSE;   // Community field should be of string type
   m_pszCommunity = (char *)malloc(dwLength + 1);
   if (!BER_DecodeContent(dwType, pbCurrPos, dwLength, (BYTE *)m_pszCommunity))
      return FALSE;   // Error parsing content of version field
   m_pszCommunity[dwLength] = 0;
   pbCurrPos += dwLength;
   dwPacketLength -= dwLength;

   if (BER_DecodeIdentifier(pbCurrPos, dwPacketLength, &dwType, &dwLength, &pbCurrPos))
   {
      switch(dwType)
      {
         case ASN_TRAP_V1_PDU:
            m_dwCommand = SNMP_TRAP;
            bResult = ParseTrapPDU(pbCurrPos, dwLength);
            break;
         default:
            break;
      }
   }

   return bResult;
}
