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
// Parse single variable binding
//

BOOL SNMP_PDU::ParseVariable(BYTE *pData, DWORD dwVarLength)
{
   SNMP_Variable *pVar;
   BOOL bResult = TRUE;

   pVar = new SNMP_Variable;
   if (pVar->Parse(pData, dwVarLength))
   {
      m_ppVarList = (SNMP_Variable **)realloc(m_ppVarList, sizeof(SNMP_Variable *) * (m_dwNumVariables + 1));
      m_ppVarList[m_dwNumVariables] = pVar;
      m_dwNumVariables++;
   }
   else
   {
      delete pVar;
      bResult = FALSE;
   }
   return bResult;
}


//
// Parse variable bindings
//

BOOL SNMP_PDU::ParseVarBinds(BYTE *pData, DWORD dwPDULength)
{
   BYTE *pbCurrPos;
   DWORD dwType, dwLength, dwBindingLength, dwIdLength;

   // Varbind section should be a SEQUENCE
   if (!BER_DecodeIdentifier(pData, dwPDULength, &dwType, &dwBindingLength, &pbCurrPos, &dwIdLength))
      return FALSE;
   if (dwType != ASN_SEQUENCE)
      return FALSE;

   while(dwBindingLength > 0)
   {
      if (!BER_DecodeIdentifier(pbCurrPos, dwPDULength, &dwType, &dwLength, &pbCurrPos, &dwIdLength))
         return FALSE;
      if (dwType != ASN_SEQUENCE)
         return FALSE;  // Every binding is a sequence
      if (dwLength > dwBindingLength)
         return FALSE;     // Invalid length

      if (!ParseVariable(pbCurrPos, dwLength))
         return FALSE;
      dwBindingLength -= dwLength + dwIdLength;
      pbCurrPos += dwLength;
   }

   return TRUE;
}


//
// Parse generic PDU
//

BOOL SNMP_PDU::ParsePDU(BYTE *pData, DWORD dwPDULength)
{
   DWORD dwType, dwLength ,dwIdLength;
   BYTE *pbCurrPos = pData;
   BOOL bResult = FALSE;

   // Request ID
   if (BER_DecodeIdentifier(pbCurrPos, dwPDULength, &dwType, &dwLength, &pbCurrPos, &dwIdLength))
   {
      if ((dwType == ASN_INTEGER) &&
          BER_DecodeContent(dwType, pbCurrPos, dwLength, (BYTE *)&m_dwRqId))
      {
         dwPDULength -= dwLength + dwIdLength;
         pbCurrPos += dwLength;
         bResult = TRUE;
      }
   }

   // Error code
   if (bResult)
   {
      bResult = FALSE;
      if (BER_DecodeIdentifier(pbCurrPos, dwPDULength, &dwType, &dwLength, &pbCurrPos, &dwIdLength))
      {
         if ((dwType == ASN_INTEGER) &&
             BER_DecodeContent(dwType, pbCurrPos, dwLength, (BYTE *)&m_dwErrorCode))
         {
            dwPDULength -= dwLength + dwIdLength;
            pbCurrPos += dwLength;
            bResult = TRUE;
         }
      }
   }

   // Error index
   if (bResult)
   {
      bResult = FALSE;
      if (BER_DecodeIdentifier(pbCurrPos, dwPDULength, &dwType, &dwLength, &pbCurrPos, &dwIdLength))
      {
         if ((dwType == ASN_INTEGER) &&
             BER_DecodeContent(dwType, pbCurrPos, dwLength, (BYTE *)&m_dwErrorIndex))
         {
            dwPDULength -= dwLength + dwIdLength;
            pbCurrPos += dwLength;
            bResult = TRUE;
         }
      }
   }

   if (bResult)
      bResult = ParseVarBinds(pbCurrPos, dwPDULength);

   return bResult;
}


//
// Parse TRAP PDU
//

BOOL SNMP_PDU::ParseTrapPDU(BYTE *pData, DWORD dwPDULength)
{
   DWORD dwType, dwLength, dwIdLength;
   BYTE *pbCurrPos = pData;
   SNMP_OID *oid;
   DWORD dwBuffer;
   BOOL bResult = FALSE;

   // Enterprise ID
   if (BER_DecodeIdentifier(pbCurrPos, dwPDULength, &dwType, &dwLength, &pbCurrPos, &dwIdLength))
   {
      if (dwType == ASN_OBJECT_ID)
      {
         oid = (SNMP_OID *)malloc(sizeof(SNMP_OID));
         memset(oid, 0, sizeof(SNMP_OID));
         if (BER_DecodeContent(dwType, pbCurrPos, dwLength, (BYTE *)oid))
         {
            m_pEnterprise = new SNMP_ObjectId(oid->dwLength, oid->pdwValue);
            dwPDULength -= dwLength + dwIdLength;
            pbCurrPos += dwLength;

            bResult = TRUE;
         }
         safe_free(oid->pdwValue);
         free(oid);
      }
   }

   // Agent's address
   if (bResult)
   {
      bResult = FALSE;
      if (BER_DecodeIdentifier(pbCurrPos, dwPDULength, &dwType, &dwLength, &pbCurrPos, &dwIdLength))
      {
         if ((dwType == ASN_IP_ADDR) && (dwLength == 4) &&
             BER_DecodeContent(dwType, pbCurrPos, dwLength, (BYTE *)&m_dwAgentAddr))
         {
            dwPDULength -= dwLength + dwIdLength;
            pbCurrPos += dwLength;
            bResult = TRUE;
         }
      }
   }

   // Generic trap type
   if (bResult)
   {
      bResult = FALSE;
      if (BER_DecodeIdentifier(pbCurrPos, dwPDULength, &dwType, &dwLength, &pbCurrPos, &dwIdLength))
      {
         if ((dwType == ASN_INTEGER) &&
             BER_DecodeContent(dwType, pbCurrPos, dwLength, (BYTE *)&dwBuffer))
         {
            dwPDULength -= dwLength + dwIdLength;
            pbCurrPos += dwLength;
            m_iTrapType = (int)dwBuffer;
            bResult = TRUE;
         }
      }
   }

   // Enterprise trap type
   if (bResult)
   {
      bResult = FALSE;
      if (BER_DecodeIdentifier(pbCurrPos, dwPDULength, &dwType, &dwLength, &pbCurrPos, &dwIdLength))
      {
         if ((dwType == ASN_INTEGER) &&
             BER_DecodeContent(dwType, pbCurrPos, dwLength, (BYTE *)&dwBuffer))
         {
            dwPDULength -= dwLength + dwIdLength;
            pbCurrPos += dwLength;
            m_iSpecificTrap = (int)dwBuffer;
            bResult = TRUE;
         }
      }
   }

   // Timestamp
   if (bResult)
   {
      bResult = FALSE;
      if (BER_DecodeIdentifier(pbCurrPos, dwPDULength, &dwType, &dwLength, &pbCurrPos, &dwIdLength))
      {
         if ((dwType == ASN_TIMETICKS) &&
             BER_DecodeContent(dwType, pbCurrPos, dwLength, (BYTE *)&m_dwTimeStamp))
         {
            dwPDULength -= dwLength + dwIdLength;
            pbCurrPos += dwLength;
            bResult = TRUE;
         }
      }
   }

   if (bResult)
      bResult = ParseVarBinds(pbCurrPos, dwPDULength);

   return bResult;
}


//
// Create PDU from packet
//

BOOL SNMP_PDU::Parse(BYTE *pRawData, DWORD dwRawLength)
{
   BYTE *pbCurrPos;
   DWORD dwType, dwLength, dwPacketLength, dwIdLength;
   BOOL bResult = FALSE;

   // Packet start
   if (!BER_DecodeIdentifier(pRawData, dwRawLength, &dwType, &dwPacketLength, &pbCurrPos, &dwIdLength))
      return FALSE;
   if (dwType != ASN_SEQUENCE)
      return FALSE;   // Packet should start with SEQUENCE

   // Version
   if (!BER_DecodeIdentifier(pbCurrPos, dwPacketLength, &dwType, &dwLength, &pbCurrPos, &dwIdLength))
      return FALSE;
   if (dwType != ASN_INTEGER)
      return FALSE;   // Version field should be of integer type
   if (!BER_DecodeContent(dwType, pbCurrPos, dwLength, (BYTE *)&m_dwVersion))
      return FALSE;   // Error parsing content of version field
   pbCurrPos += dwLength;
   dwPacketLength -= dwLength + dwIdLength;
   if ((m_dwVersion != SNMP_VERSION_1) && (m_dwVersion != SNMP_VERSION_2C))
      return FALSE;   // Unsupported SNMP version

   // Community string
   if (!BER_DecodeIdentifier(pbCurrPos, dwPacketLength, &dwType, &dwLength, &pbCurrPos, &dwIdLength))
      return FALSE;
   if (dwType != ASN_OCTET_STRING)
      return FALSE;   // Community field should be of string type
   m_pszCommunity = (char *)malloc(dwLength + 1);
   if (!BER_DecodeContent(dwType, pbCurrPos, dwLength, (BYTE *)m_pszCommunity))
      return FALSE;   // Error parsing content of version field
   m_pszCommunity[dwLength] = 0;
   pbCurrPos += dwLength;
   dwPacketLength -= dwLength + dwIdLength;

   if (BER_DecodeIdentifier(pbCurrPos, dwPacketLength, &dwType, &dwLength, &pbCurrPos, &dwIdLength))
   {
      switch(dwType)
      {
         case ASN_TRAP_V1_PDU:
            m_dwCommand = SNMP_TRAP;
            bResult = ParseTrapPDU(pbCurrPos, dwLength);
            break;
         case ASN_TRAP_V2_PDU:
            m_dwCommand = SNMP_TRAP;
            m_iTrapType = 6;
            m_iSpecificTrap = 0;
            bResult = ParsePDU(pbCurrPos, dwLength);
            if (bResult)
            {
               bResult = FALSE;
               if (m_dwNumVariables >= 2)
               {
                  if (m_ppVarList[1]->GetType() == ASN_OBJECT_ID)
                  {
                     m_pEnterprise = new SNMP_ObjectId(
                        m_ppVarList[1]->GetValueLength() / sizeof(DWORD), 
                        (DWORD *)m_ppVarList[1]->GetValue());
                     bResult = TRUE;
                  }
               }
            }
            break;
         case ASN_GET_REQUEST_PDU:
            m_dwCommand = SNMP_GET_REQUEST;
            bResult = ParsePDU(pbCurrPos, dwLength);
            break;
         case ASN_GET_NEXT_REQUEST_PDU:
            m_dwCommand = SNMP_GET_NEXT_REQUEST;
            bResult = ParsePDU(pbCurrPos, dwLength);
            break;
         case ASN_GET_RESPONCE_PDU:
            m_dwCommand = SNMP_GET_RESPONCE;
            bResult = ParsePDU(pbCurrPos, dwLength);
            break;
         case ASN_SET_REQUEST_PDU:
            m_dwCommand = SNMP_SET_REQUEST;
            bResult = ParsePDU(pbCurrPos, dwLength);
            break;
         default:
            break;
      }
   }

   return bResult;
}
