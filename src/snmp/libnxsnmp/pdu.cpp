/* 
** NetXMS - Network Management System
** SNMP support library
** Copyright (C) 2003-2009 Victor Kirhenshtein
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
** File: pdu.cpp
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
   { ASN_TRAP_V2_PDU, SNMP_VERSION_3, SNMP_TRAP },
   { ASN_GET_REQUEST_PDU, -1, SNMP_GET_REQUEST },
   { ASN_GET_NEXT_REQUEST_PDU, -1, SNMP_GET_NEXT_REQUEST },
   { ASN_SET_REQUEST_PDU, -1, SNMP_SET_REQUEST },
   { ASN_RESPONSE_PDU, -1, SNMP_RESPONSE },
   { ASN_REPORT_PDU, -1, SNMP_REPORT },
   { ASN_INFORM_REQUEST_PDU, -1, SNMP_INFORM_REQUEST },
   { 0, -1, 0 }
};


//
// SNMP_PDU default constructor
//

SNMP_PDU::SNMP_PDU()
{
   m_dwVersion = SNMP_VERSION_1;
   m_dwCommand = SNMP_INVALID_PDU;
   m_security = new SNMP_SecurityContext;
   m_dwNumVariables = 0;
   m_ppVarList = NULL;
   m_pEnterprise = NULL;
   m_dwErrorCode = 0;
   m_dwErrorIndex = 0;
   m_dwRqId = 0;
	m_msgId = 0;
	m_flags = 0;
   m_iTrapType = 0;
   m_iSpecificTrap = 0;
	m_authoritativeEngine = NULL;
	m_contextEngineIdLen = 0;
	m_contextName[0] = 0;
	m_msgMaxSize = SNMP_DEFAULT_MSG_MAX_SIZE;
}


//
// Create request PDU
//

SNMP_PDU::SNMP_PDU(DWORD dwCommand, char *pszCommunity, DWORD dwRqId, DWORD dwVersion)
{
   m_dwVersion = dwVersion;
   m_dwCommand = dwCommand;
   m_security = new SNMP_SecurityContext(pszCommunity);
   m_dwNumVariables = 0;
   m_ppVarList = NULL;
   m_pEnterprise = NULL;
   m_dwErrorCode = 0;
   m_dwErrorIndex = 0;
   m_dwRqId = dwRqId;
	m_msgId = dwRqId;
	m_flags = 0;
   m_iTrapType = 0;
   m_iSpecificTrap = 0;
	m_authoritativeEngine = NULL;
	m_contextEngineIdLen = 0;
	m_contextName[0] = 0;
	m_msgMaxSize = SNMP_DEFAULT_MSG_MAX_SIZE;
}


//
// Create request PDU
//

SNMP_PDU::SNMP_PDU(DWORD dwCommand, SNMP_SecurityContext *security, DWORD dwRqId, DWORD dwVersion)
{
   m_dwVersion = SNMP_VERSION_3;
   m_dwCommand = dwCommand;
   m_security = security;
   m_dwNumVariables = 0;
   m_ppVarList = NULL;
   m_pEnterprise = NULL;
   m_dwErrorCode = 0;
   m_dwErrorIndex = 0;
   m_dwRqId = dwRqId;
	m_msgId = dwRqId;
	m_flags = 0;
   m_iTrapType = 0;
   m_iSpecificTrap = 0;
	m_authoritativeEngine = NULL;
	m_contextEngineIdLen = 0;
	m_contextName[0] = 0;
	m_msgMaxSize = SNMP_DEFAULT_MSG_MAX_SIZE;
}


//
// SNMP_PDU destructor
//

SNMP_PDU::~SNMP_PDU()
{
   DWORD i;

   delete m_security;
	delete m_authoritativeEngine;
   delete m_pEnterprise;
   for(i = 0; i < m_dwNumVariables; i++)
      delete m_ppVarList[i];
   safe_free(m_ppVarList);
}


//
// Parse single variable binding
//

BOOL SNMP_PDU::parseVariable(BYTE *pData, DWORD dwVarLength)
{
   SNMP_Variable *var;
   BOOL success = TRUE;

   var = new SNMP_Variable;
   if (var->Parse(pData, dwVarLength))
   {
      bindVariable(var);
   }
   else
   {
      delete var;
      success = FALSE;
   }
   return success;
}


//
// Parse variable bindings
//

BOOL SNMP_PDU::parseVarBinds(BYTE *pData, DWORD dwPDULength)
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

      if (!parseVariable(pbCurrPos, dwLength))
         return FALSE;
      dwBindingLength -= dwLength + dwIdLength;
      pbCurrPos += dwLength;
   }

   return TRUE;
}


//
// Parse generic PDU content
//

BOOL SNMP_PDU::parsePduContent(BYTE *pData, DWORD dwPDULength)
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
      bResult = parseVarBinds(pbCurrPos, dwPDULength);

   return bResult;
}


//
// Parse version 1 TRAP PDU
//

BOOL SNMP_PDU::parseTrapPDU(BYTE *pData, DWORD dwPDULength)
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
      bResult = parseVarBinds(pbCurrPos, dwPDULength);

   if (bResult)
   {
      if (m_iTrapType < 6)
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
   }

   return bResult;
}


//
// Parse version 2 TRAP PDU
//

BOOL SNMP_PDU::parseTrap2PDU(BYTE *pData, DWORD dwPDULength)
{
   BOOL bResult;
   static DWORD pdwStdTrapPrefix[9] = { 1, 3, 6, 1, 6, 3, 1, 1, 5 };

   bResult = parsePduContent(pData, dwPDULength);
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
// Parse version 3 header
//

BOOL SNMP_PDU::parseV3Header(BYTE *header, DWORD headerLength)
{
	DWORD type, length, idLength, remLength = headerLength;
	BYTE *currPos = header;

   // Message id
   if (!BER_DecodeIdentifier(currPos, remLength, &type, &length, &currPos, &idLength))
      return FALSE;
   if (type != ASN_INTEGER)
      return FALSE;   // Should be of integer type
   if (!BER_DecodeContent(type, currPos, length, (BYTE *)&m_msgId))
      return FALSE;   // Error parsing content
   currPos += length;
   remLength -= length + idLength;

   // Message max size
   if (!BER_DecodeIdentifier(currPos, remLength, &type, &length, &currPos, &idLength))
      return FALSE;
   if (type != ASN_INTEGER)
      return FALSE;   // Should be of integer type
   if (!BER_DecodeContent(type, currPos, length, (BYTE *)&m_msgMaxSize))
      return FALSE;   // Error parsing content
   currPos += length;
   remLength -= length + idLength;

   // Message flags
   if (!BER_DecodeIdentifier(currPos, remLength, &type, &length, &currPos, &idLength))
      return FALSE;
   if ((type != ASN_OCTET_STRING) || (length != 1))
      return FALSE;
	BYTE flags;
   if (!BER_DecodeContent(type, currPos, length, &flags))
      return FALSE;   // Error parsing content
	m_flags = flags;
   currPos += length;
   remLength -= length + idLength;

   // Security model
   if (!BER_DecodeIdentifier(currPos, remLength, &type, &length, &currPos, &idLength))
      return FALSE;
   if (type != ASN_INTEGER)
      return FALSE;   // Should be of integer type
	DWORD securityModel;
   if (!BER_DecodeContent(type, currPos, length, (BYTE *)&securityModel))
      return FALSE;   // Error parsing content
   currPos += length;
   remLength -= length + idLength;

	delete m_security;
	m_security = new SNMP_SecurityContext(securityModel);

	return TRUE;
}


//
// Parse V3 USM security parameters
//

BOOL SNMP_PDU::parseV3SecurityUsm(BYTE *data, DWORD dataLength)
{
	DWORD type, length, idLength, remLength = dataLength;
	DWORD engineBoots, engineTime;
	BYTE *currPos = data;
	BYTE engineId[SNMP_MAX_ENGINEID_LEN];
	int engineIdLen;

	// Should be sequence
   if (!BER_DecodeIdentifier(currPos, remLength, &type, &length, &currPos, &idLength))
      return FALSE;
   if (type != ASN_SEQUENCE)
      return FALSE;
   remLength = length;

	// Engine ID
   if (!BER_DecodeIdentifier(currPos, remLength, &type, &length, &currPos, &idLength))
      return FALSE;
   if (type != ASN_OCTET_STRING)
      return FALSE;
	engineIdLen = length;
   if (!BER_DecodeContent(type, currPos, length, engineId))
      return FALSE;   // Error parsing content
   currPos += length;
   remLength -= length + idLength;

	// engine boots
   if (!BER_DecodeIdentifier(currPos, remLength, &type, &length, &currPos, &idLength))
      return FALSE;
   if (type != ASN_INTEGER)
      return FALSE;
   if (!BER_DecodeContent(type, currPos, length, (BYTE *)&engineBoots))
      return FALSE;   // Error parsing content
   currPos += length;
   remLength -= length + idLength;

	// engine time
   if (!BER_DecodeIdentifier(currPos, remLength, &type, &length, &currPos, &idLength))
      return FALSE;
   if (type != ASN_INTEGER)
      return FALSE;
   if (!BER_DecodeContent(type, currPos, length, (BYTE *)&engineTime))
      return FALSE;   // Error parsing content
   currPos += length;
   remLength -= length + idLength;

	setAuthoritativeEngine(new SNMP_Engine(engineId, engineIdLen, engineBoots, engineTime));

	return TRUE;
}


//
// Parse V3 scoped PDU
//

BOOL SNMP_PDU::parseV3ScopedPdu(BYTE *data, DWORD dataLength)
{
	DWORD type, length, idLength, remLength = dataLength;
	BYTE *currPos = data;

   // Context engine ID
   if (!BER_DecodeIdentifier(currPos, remLength, &type, &length, &currPos, &idLength))
      return FALSE;
   if ((type != ASN_OCTET_STRING) || (length > SNMP_MAX_ENGINEID_LEN))
      return FALSE;
	m_contextEngineIdLen = length;
   if (!BER_DecodeContent(type, currPos, length, m_contextEngineId))
      return FALSE;   // Error parsing content
   currPos += length;
   remLength -= length + idLength;
	
   // Context name
   if (!BER_DecodeIdentifier(currPos, remLength, &type, &length, &currPos, &idLength))
      return FALSE;
   if ((type != ASN_OCTET_STRING) || (length >= SNMP_MAX_CONTEXT_NAME))
      return FALSE;
   if (!BER_DecodeContent(type, currPos, length, (BYTE *)m_contextName))
      return FALSE;   // Error parsing content
	m_contextName[length] = 0;
   currPos += length;
   remLength -= length + idLength;
	
	return parsePdu(currPos, remLength);
}


//
// Parse PDU
//

BOOL SNMP_PDU::parsePdu(BYTE *pdu, DWORD pduLength)
{
	BYTE *content;
	DWORD length, idLength, type;
	BOOL success;

   if (success = BER_DecodeIdentifier(pdu, pduLength, &type, &length, &content, &idLength))
   {
      switch(type)
      {
         case ASN_TRAP_V1_PDU:
            m_dwCommand = SNMP_TRAP;
            success = parseTrapPDU(content, length);
            break;
         case ASN_TRAP_V2_PDU:
            m_dwCommand = SNMP_TRAP;
            success = parseTrap2PDU(content, length);
            break;
         case ASN_GET_REQUEST_PDU:
            m_dwCommand = SNMP_GET_REQUEST;
            success = parsePduContent(content, length);
            break;
         case ASN_GET_NEXT_REQUEST_PDU:
            m_dwCommand = SNMP_GET_NEXT_REQUEST;
            success = parsePduContent(content, length);
            break;
         case ASN_RESPONSE_PDU:
            m_dwCommand = SNMP_RESPONSE;
            success = parsePduContent(content, length);
            break;
         case ASN_SET_REQUEST_PDU:
            m_dwCommand = SNMP_SET_REQUEST;
            success = parsePduContent(content, length);
            break;
         case ASN_INFORM_REQUEST_PDU:
            m_dwCommand = SNMP_INFORM_REQUEST;
            success = parsePduContent(content, length);
            break;
         case ASN_REPORT_PDU:
            m_dwCommand = SNMP_REPORT;
            success = parsePduContent(content, length);
            break;
         default:
				success = FALSE;
            break;
      }
   }
	return success;
}


//
// Create PDU from packet
//

BOOL SNMP_PDU::parse(BYTE *pRawData, DWORD dwRawLength)
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
   if ((m_dwVersion != SNMP_VERSION_1) && (m_dwVersion != SNMP_VERSION_2C) && ((m_dwVersion != SNMP_VERSION_3)))
      return FALSE;   // Unsupported SNMP version

	if (m_dwVersion == SNMP_VERSION_3)
	{
		// V3 header
		if (!BER_DecodeIdentifier(pbCurrPos, dwPacketLength, &dwType, &dwLength, &pbCurrPos, &dwIdLength))
			return FALSE;
		if (dwType != ASN_SEQUENCE)
			return FALSE;   // Should be sequence
		
		// We don't need BER_DecodeContent because sequence does not need any special decoding
		if (!parseV3Header(pbCurrPos, dwLength))
			return FALSE;
		pbCurrPos += dwLength;
		dwPacketLength -= dwLength + dwIdLength;

		// Security parameters
		if (!BER_DecodeIdentifier(pbCurrPos, dwPacketLength, &dwType, &dwLength, &pbCurrPos, &dwIdLength))
			return FALSE;
		if (dwType != ASN_OCTET_STRING)
			return FALSE;   // Should be octet string

		if (m_security->getSecurityModel() == SNMP_SECURITY_MODEL_USM)
		{
			if (!parseV3SecurityUsm(pbCurrPos, dwLength))
				return FALSE;
		}

		pbCurrPos += dwLength;
		dwPacketLength -= dwLength + dwIdLength;

		// Scoped PDU
		if (!BER_DecodeIdentifier(pbCurrPos, dwPacketLength, &dwType, &dwLength, &pbCurrPos, &dwIdLength))
			return FALSE;
		if (dwType != ASN_SEQUENCE)
			return FALSE;   // Should be sequence
		bResult = parseV3ScopedPdu(pbCurrPos, dwLength);
	}
	else
	{
		// Community string
		if (!BER_DecodeIdentifier(pbCurrPos, dwPacketLength, &dwType, &dwLength, &pbCurrPos, &dwIdLength))
			return FALSE;
		if (dwType != ASN_OCTET_STRING)
			return FALSE;   // Community field should be of string type
		char *community = (char *)malloc(dwLength + 1);
		if (!BER_DecodeContent(dwType, pbCurrPos, dwLength, (BYTE *)community))
		{
			free(community);
			return FALSE;   // Error parsing content of version field
		}
		community[dwLength] = 0;
		delete m_security;
		m_security = new SNMP_SecurityContext(community);
		free(community);
		pbCurrPos += dwLength;
		dwPacketLength -= dwLength + dwIdLength;

		bResult = parsePdu(pbCurrPos, dwLength);
	}

   return bResult;
}


//
// Create packet from PDU
//

DWORD SNMP_PDU::encode(BYTE **ppBuffer)
{
   DWORD i, dwBufferSize, dwBytes, dwVarBindsSize, dwPDUType, dwPDUSize, dwPacketSize, dwValue;
   BYTE *pbCurrPos, *pBlock, *pVarBinds, *pPacket;

   // Estimate required buffer size and allocate it
   for(dwBufferSize = 1024, i = 0; i < m_dwNumVariables; i++)
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

		if (m_dwVersion == SNMP_VERSION_3)
		{
			if (m_authoritativeEngine == NULL)
				m_authoritativeEngine = new SNMP_Engine;

			dwBytes = encodeV3Header(pbCurrPos, dwBufferSize - dwPacketSize);
			dwPacketSize += dwBytes;
			pbCurrPos += dwBytes;

			dwBytes = encodeV3SecurityParameters(pbCurrPos, dwBufferSize - dwPacketSize);
			dwPacketSize += dwBytes;
			pbCurrPos += dwBytes;

			dwBytes = encodeV3ScopedPDU(dwPDUType, pBlock, dwPDUSize, pbCurrPos, dwBufferSize - dwPacketSize);
			dwPacketSize += dwBytes;
		}
		else
		{
			dwBytes = BER_Encode(ASN_OCTET_STRING, (BYTE *)m_security->getCommunity(),
										(DWORD)strlen(m_security->getCommunity()), pbCurrPos,
										dwBufferSize - dwPacketSize);
			dwPacketSize += dwBytes;
			pbCurrPos += dwBytes;

			// Encode PDU into packet
			dwBytes = BER_Encode(dwPDUType, pBlock, dwPDUSize, pbCurrPos, dwBufferSize - dwPacketSize);
			dwPacketSize += dwBytes;
		}

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
// Encode version 3 header
//

DWORD SNMP_PDU::encodeV3Header(BYTE *buffer, DWORD bufferSize)
{
	BYTE header[256];
	DWORD bytes, securityModel = m_security->getSecurityModel();

	bytes = BER_Encode(ASN_INTEGER, (BYTE *)&m_dwRqId, sizeof(DWORD), header, 256);
	bytes += BER_Encode(ASN_INTEGER, (BYTE *)&m_msgMaxSize, sizeof(DWORD), &header[bytes], 256 - bytes);
	bytes += BER_Encode(ASN_OCTET_STRING, &m_flags, 1, &header[bytes], 256 - bytes);
	bytes += BER_Encode(ASN_INTEGER, (BYTE *)&securityModel, sizeof(DWORD), &header[bytes], 256 - bytes);
	return BER_Encode(ASN_SEQUENCE, header, bytes, buffer, bufferSize);
}


//
// Encode version 3 security parameters
//

DWORD SNMP_PDU::encodeV3SecurityParameters(BYTE *buffer, DWORD bufferSize)
{
	BYTE securityParameters[1024], sequence[1040];
	DWORD bytes, engineBoots = m_authoritativeEngine->getBoots(), engineTime = m_authoritativeEngine->getTime();

	if ((m_security != NULL) && (m_security->getSecurityModel() == SNMP_SECURITY_MODEL_USM))
	{
		bytes = BER_Encode(ASN_OCTET_STRING, m_authoritativeEngine->getId(), m_authoritativeEngine->getIdLen(), securityParameters, 1024);
		bytes += BER_Encode(ASN_INTEGER, (BYTE *)&engineBoots, sizeof(DWORD), &securityParameters[bytes], 1024 - bytes);
		bytes += BER_Encode(ASN_INTEGER, (BYTE *)&engineTime, sizeof(DWORD), &securityParameters[bytes], 1024 - bytes);
		bytes += BER_Encode(ASN_OCTET_STRING, (BYTE *)m_security->getUser(), strlen(m_security->getUser()), &securityParameters[bytes], 1024 - bytes);

		// Authentication parameters
		if (m_flags & SNMP_AUTH_FLAG)
		{
			/* TODO: implement authentication */
		}
		else
		{
			bytes += BER_Encode(ASN_OCTET_STRING, NULL, 0, &securityParameters[bytes], 1024 - bytes);
		}

		// Privacy parameters
		if (m_flags & SNMP_PRIV_FLAG)
		{
			/* TODO: implement encryption */
		}
		else
		{
			bytes += BER_Encode(ASN_OCTET_STRING, NULL, 0, &securityParameters[bytes], 1024 - bytes);
		}

		// Wrap into sequence
		bytes = BER_Encode(ASN_SEQUENCE, securityParameters, bytes, sequence, 1040);

		// Wrap sequence into octet string
		bytes = BER_Encode(ASN_OCTET_STRING, sequence, bytes, buffer, bufferSize);
	}
	else
	{
		bytes = BER_Encode(ASN_OCTET_STRING, NULL, 0, buffer, bufferSize);
	}
	return bytes;
}


//
// Encode versionj 3 scoped PDU
//

DWORD SNMP_PDU::encodeV3ScopedPDU(DWORD pduType, BYTE *pdu, DWORD pduSize, BYTE *buffer, DWORD bufferSize)
{
	DWORD spduLen = pduSize + SNMP_MAX_CONTEXT_NAME + SNMP_MAX_ENGINEID_LEN + 32;
	BYTE *spdu = (BYTE *)malloc(spduLen);
	DWORD bytes;

	bytes = BER_Encode(ASN_OCTET_STRING, m_contextEngineId, (DWORD)m_contextEngineIdLen, spdu, spduLen);
	bytes += BER_Encode(ASN_OCTET_STRING, (BYTE *)m_contextName, strlen(m_contextName), &spdu[bytes], spduLen - bytes);
	bytes += BER_Encode(pduType, pdu, pduSize, &spdu[bytes], spduLen - bytes);
	
	// Wrap scoped PDU into SEQUENCE
	bytes = BER_Encode(ASN_SEQUENCE, spdu, bytes, buffer, bufferSize);
	free(spdu);
	return bytes;
}


//
// Bind variable to PDU
//

void SNMP_PDU::bindVariable(SNMP_Variable *pVar)
{
   m_ppVarList = (SNMP_Variable **)realloc(m_ppVarList, sizeof(SNMP_Variable *) * (m_dwNumVariables + 1));
   m_ppVarList[m_dwNumVariables] = pVar;
   m_dwNumVariables++;
}


//
// Set context ID
//

void SNMP_PDU::setContextEngineId(BYTE *id, int len)
{
	m_contextEngineIdLen = min(len, SNMP_MAX_ENGINEID_LEN);
	memcpy(m_contextEngineId, id, m_contextEngineIdLen);
}

void SNMP_PDU::setContextEngineId(const char *id)
{
	m_contextEngineIdLen = min(strlen(id), SNMP_MAX_ENGINEID_LEN);
	memcpy(m_contextEngineId, id, m_contextEngineIdLen);
}


//
// Set context name
//

void SNMP_PDU::setContextName(const char *name)
{
	strncpy(m_contextName, name, SNMP_MAX_CONTEXT_NAME);
	m_contextName[SNMP_MAX_CONTEXT_NAME - 1] = 0;
}
