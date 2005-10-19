/* 
** NetXMS - Network Management System
** SNMP support library
** Copyright (C) 2003, 2004, 2005 Victor Kirhenshtein
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
// PDU type to command translation
//

static struct
{
   DWORD dwType;
   int iVersion;
   DWORD dwCommand;
} PDU_type_to_command[] = 
{
   { ASN_TRAP_V1_PDU, SNMP_VERSION_1, SNMP_TRAP },
   { ASN_TRAP_V2_PDU, SNMP_VERSION_2C, SNMP_TRAP },
   { ASN_TRAP_V1_PDU, SNMP_VERSION_1, SNMP_TRAP },
   { ASN_GET_REQUEST_PDU, -1, SNMP_GET_REQUEST },
   { ASN_GET_NEXT_REQUEST_PDU, -1, SNMP_GET_NEXT_REQUEST },
   { ASN_SET_REQUEST_PDU, -1, SNMP_SET_REQUEST },
   { ASN_GET_RESPONSE_PDU, -1, SNMP_GET_RESPONSE },
   { 0, -1, 0 }
};


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
   m_dwErrorCode = 0;
   m_dwErrorIndex = 0;
   m_dwRqId = 0;
   m_iTrapType = 0;
   m_iSpecificTrap = 0;
}


//
// Create request PDU
//

SNMP_PDU::SNMP_PDU(DWORD dwCommand, char *pszCommunity, DWORD dwRqId, DWORD dwVersion)
{
   m_dwVersion = dwVersion;
   m_dwCommand = dwCommand;
   m_pszCommunity = strdup(CHECK_NULL(pszCommunity));
   m_dwNumVariables = 0;
   m_ppVarList = NULL;
   m_pEnterprise = NULL;
   m_dwErrorCode = 0;
   m_dwErrorIndex = 0;
   m_dwRqId = dwRqId;
   m_iTrapType = 0;
   m_iSpecificTrap = 0;
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
      BindVariable(pVar);
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
// Parse version 1 TRAP PDU
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

   if (bResult && (m_iTrapType < 6))
   {
      static DWORD pdwStdOid[6][10] =
      {
         { 1, 3, 6, 1, 6, 3, 1, 1, 5, 1 },   // cold start
         { 1, 3, 6, 1, 6, 3, 1, 1, 5, 2 },   // warm start
         { 1, 3, 6, 1, 6, 3, 1, 1, 5, 3 },   // link down
         { 1, 3, 6, 1, 6, 3, 1, 1, 5, 4 },   // link up
         { 1, 3, 6, 1, 6, 3, 1, 1, 5, 5 },   // authentication failure
         { 1, 3, 6, 1, 6, 3, 1, 1, 5, 6 }    // EGP neighbor loss (obsolete)
      };

      // For standard trap types, create standard V2 Enterprise ID
      m_pEnterprise->SetValue(pdwStdOid[m_iTrapType], 10);
   }
   else
   {
      m_pEnterprise->Extend(0);
      m_pEnterprise->Extend(m_iSpecificTrap);
   }

   return bResult;
}


//
// Parse version 2 TRAP PDU
//

BOOL SNMP_PDU::ParseTrap2PDU(BYTE *pData, DWORD dwPDULength)
{
   BOOL bResult;
   static DWORD pdwStdTrapPrefix[9] = { 1, 3, 6, 1, 6, 3, 1, 1, 5 };

   bResult = ParsePDU(pData, dwPDULength);
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

      // Set V1 trap type and specific trap type fields
      if (bResult)
      {
         if ((m_pEnterprise->Compare(pdwStdTrapPrefix, 9) == OID_SHORTER) &&
             (m_pEnterprise->Length() == 10))
         {
            m_iTrapType = m_pEnterprise->GetValue()[9];
            m_iSpecificTrap = 0;
         }
         else
         {
            m_iTrapType = 6;
            m_iSpecificTrap = m_pEnterprise->GetValue()[m_pEnterprise->Length() - 1];
         }
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
            bResult = ParseTrap2PDU(pbCurrPos, dwLength);
            break;
         case ASN_GET_REQUEST_PDU:
            m_dwCommand = SNMP_GET_REQUEST;
            bResult = ParsePDU(pbCurrPos, dwLength);
            break;
         case ASN_GET_NEXT_REQUEST_PDU:
            m_dwCommand = SNMP_GET_NEXT_REQUEST;
            bResult = ParsePDU(pbCurrPos, dwLength);
            break;
         case ASN_GET_RESPONSE_PDU:
            m_dwCommand = SNMP_GET_RESPONSE;
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


//
// Create packet from PDU
//

DWORD SNMP_PDU::Encode(BYTE **ppBuffer)
{
   DWORD i, dwBufferSize, dwBytes, dwVarBindsSize, dwPDUType, dwPDUSize, dwPacketSize, dwValue;
   BYTE *pbCurrPos, *pBlock, *pVarBinds, *pPacket;

   // Estimate required buffer size and allocate it
   for(dwBufferSize = 256, i = 0; i < m_dwNumVariables; i++)
      dwBufferSize += m_ppVarList[i]->GetValueLength() + m_ppVarList[i]->GetName()->Length() * 4 + 16;
   pBlock = (BYTE *)malloc(dwBufferSize);
   pVarBinds = (BYTE *)malloc(dwBufferSize);
   pPacket = (BYTE *)malloc(dwBufferSize);

   // Encode variables
   for(i = 0, dwVarBindsSize = 0, pbCurrPos = pVarBinds; i < m_dwNumVariables; i++)
   {
      dwBytes = m_ppVarList[i]->Encode(pbCurrPos, dwBufferSize - dwVarBindsSize);
      pbCurrPos += dwBytes;
      dwVarBindsSize += dwBytes;
   }

   // Determine PDU type
   for(i = 0; PDU_type_to_command[i].dwType != 0; i++)
      if (((m_dwVersion == (DWORD)PDU_type_to_command[i].iVersion) ||
           (PDU_type_to_command[i].iVersion == -1)) &&
          (PDU_type_to_command[i].dwCommand == m_dwCommand))
      {
         dwPDUType = PDU_type_to_command[i].dwType;
         break;
      }

   // Encode PDU header
   if (dwPDUType != 0)
   {
      pbCurrPos = pBlock;
      dwPDUSize = 0;
      switch(dwPDUType)
      {
         case ASN_TRAP_V1_PDU:
            dwBytes = BER_Encode(ASN_OBJECT_ID, (BYTE *)m_pEnterprise->GetValue(),
                                 m_pEnterprise->Length() * sizeof(DWORD),
                                 pbCurrPos, dwBufferSize - dwPDUSize);
            dwPDUSize += dwBytes;
            pbCurrPos += dwBytes;

            dwBytes = BER_Encode(ASN_IP_ADDR, (BYTE *)&m_dwAgentAddr, sizeof(DWORD), 
                                 pbCurrPos, dwBufferSize - dwPDUSize);
            dwPDUSize += dwBytes;
            pbCurrPos += dwBytes;

            dwValue = (DWORD)m_iTrapType;
            dwBytes = BER_Encode(ASN_INTEGER, (BYTE *)&dwValue, sizeof(DWORD), 
                                 pbCurrPos, dwBufferSize - dwPDUSize);
            dwPDUSize += dwBytes;
            pbCurrPos += dwBytes;

            dwValue = (DWORD)m_iSpecificTrap;
            dwBytes = BER_Encode(ASN_INTEGER, (BYTE *)&dwValue, sizeof(DWORD), 
                                 pbCurrPos, dwBufferSize - dwPDUSize);
            dwPDUSize += dwBytes;
            pbCurrPos += dwBytes;

            dwBytes = BER_Encode(ASN_INTEGER, (BYTE *)&m_dwTimeStamp, sizeof(DWORD), 
                                 pbCurrPos, dwBufferSize - dwPDUSize);
            dwPDUSize += dwBytes;
            pbCurrPos += dwBytes;
            break;
         default:
            dwBytes = BER_Encode(ASN_INTEGER, (BYTE *)&m_dwRqId, sizeof(DWORD), 
                                 pbCurrPos, dwBufferSize - dwPDUSize);
            dwPDUSize += dwBytes;
            pbCurrPos += dwBytes;

            dwBytes = BER_Encode(ASN_INTEGER, (BYTE *)&m_dwErrorCode, sizeof(DWORD), 
                                 pbCurrPos, dwBufferSize - dwPDUSize);
            dwPDUSize += dwBytes;
            pbCurrPos += dwBytes;

            dwBytes = BER_Encode(ASN_INTEGER, (BYTE *)&m_dwErrorIndex, sizeof(DWORD), 
                                 pbCurrPos, dwBufferSize - dwPDUSize);
            dwPDUSize += dwBytes;
            pbCurrPos += dwBytes;
            break;
      }

      // Encode varbinds into PDU
      dwBytes = BER_Encode(ASN_SEQUENCE, pVarBinds, dwVarBindsSize, 
                           pbCurrPos, dwBufferSize - dwPDUSize);
      dwPDUSize += dwBytes;

      // Encode packet header
      pbCurrPos = pPacket;
      dwPacketSize = 0;

      dwBytes = BER_Encode(ASN_INTEGER, (BYTE *)&m_dwVersion, sizeof(DWORD), 
                           pbCurrPos, dwBufferSize);
      dwPacketSize += dwBytes;
      pbCurrPos += dwBytes;

      dwBytes = BER_Encode(ASN_OCTET_STRING, (BYTE *)m_pszCommunity, strlen(m_pszCommunity),
                           pbCurrPos, dwBufferSize - dwPacketSize);
      dwPacketSize += dwBytes;
      pbCurrPos += dwBytes;

      // Encode PDU into packet
      dwBytes = BER_Encode(dwPDUType, pBlock, dwPDUSize, pbCurrPos, dwBufferSize - dwPacketSize);
      dwPacketSize += dwBytes;

      // And final step: allocate buffer for entire datagramm and wrap packet
      // into SEQUENCE
      *ppBuffer = (BYTE *)malloc(dwPacketSize + 6);
      dwBytes = BER_Encode(ASN_SEQUENCE, pPacket, dwPacketSize, *ppBuffer, dwPacketSize + 6);
   }
   else
   {
      dwBytes = 0;   // Error
   }

   free(pPacket);
   free(pBlock);
   free(pVarBinds);
   return dwBytes;
}


//
// Bind variable to PDU
//

void SNMP_PDU::BindVariable(SNMP_Variable *pVar)
{
   m_ppVarList = (SNMP_Variable **)realloc(m_ppVarList, sizeof(SNMP_Variable *) * (m_dwNumVariables + 1));
   m_ppVarList[m_dwNumVariables] = pVar;
   m_dwNumVariables++;
}
