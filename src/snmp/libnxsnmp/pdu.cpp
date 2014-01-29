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
** File: pdu.cpp
**
**/

#include "libnxsnmp.h"

#ifdef _WITH_ENCRYPTION
#ifndef OPENSSL_NO_DES
#include <openssl/des.h>
#endif
#ifndef OPENSSL_NO_AES
#include <openssl/aes.h>
#endif
#endif

/**
 * PLaceholder for message hash
 */
static BYTE m_hashPlaceholder[12] = { 0xAB, 0xCD, 0xEF, 0xFF, 0x00, 0x11, 0x22, 0x33, 0xBB, 0xAA, 0x99, 0x88 };

/**
 * PDU type to command translation
 */
static struct
{
   UINT32 dwType;
   int iVersion;
   UINT32 dwCommand;
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

/**
 * SNMP_PDU default constructor
 */
SNMP_PDU::SNMP_PDU()
{
   m_dwVersion = SNMP_VERSION_1;
   m_dwCommand = SNMP_INVALID_PDU;
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
	m_contextEngineIdLen = 0;
	m_contextName[0] = 0;
	m_msgMaxSize = SNMP_DEFAULT_MSG_MAX_SIZE;
	m_authObject = NULL;
	m_reportable = true;
}

/**
 * Create request PDU
 */
SNMP_PDU::SNMP_PDU(UINT32 dwCommand, UINT32 dwRqId, UINT32 dwVersion)
{
   m_dwVersion = dwVersion;
   m_dwCommand = dwCommand;
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
	m_contextEngineIdLen = 0;
	m_contextName[0] = 0;
	m_msgMaxSize = SNMP_DEFAULT_MSG_MAX_SIZE;
	m_authObject = NULL;
	m_reportable = true;
}

/**
 * SNMP_PDU destructor
 */
SNMP_PDU::~SNMP_PDU()
{
   UINT32 i;

   delete m_pEnterprise;
   for(i = 0; i < m_dwNumVariables; i++)
      delete m_ppVarList[i];
   safe_free(m_ppVarList);
	safe_free(m_authObject);
}

/**
 * Parse single variable binding
 */
BOOL SNMP_PDU::parseVariable(BYTE *pData, UINT32 dwVarLength)
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

/**
 * Parse variable bindings
 */
BOOL SNMP_PDU::parseVarBinds(BYTE *pData, UINT32 dwPDULength)
{
   BYTE *pbCurrPos;
   UINT32 dwType, dwLength, dwBindingLength, dwIdLength;

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

/**
 * Parse generic PDU content
 */
BOOL SNMP_PDU::parsePduContent(BYTE *pData, UINT32 dwPDULength)
{
   UINT32 dwType, dwLength ,dwIdLength;
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

/**
 * Parse version 1 TRAP PDU
 */
BOOL SNMP_PDU::parseTrapPDU(BYTE *pData, UINT32 dwPDULength)
{
   UINT32 dwType, dwLength, dwIdLength;
   BYTE *pbCurrPos = pData;
   SNMP_OID *oid;
   UINT32 dwBuffer;
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
         static UINT32 pdwStdOid[6][10] =
         {
            { 1, 3, 6, 1, 6, 3, 1, 1, 5, 1 },   // cold start
            { 1, 3, 6, 1, 6, 3, 1, 1, 5, 2 },   // warm start
            { 1, 3, 6, 1, 6, 3, 1, 1, 5, 3 },   // link down
            { 1, 3, 6, 1, 6, 3, 1, 1, 5, 4 },   // link up
            { 1, 3, 6, 1, 6, 3, 1, 1, 5, 5 },   // authentication failure
            { 1, 3, 6, 1, 6, 3, 1, 1, 5, 6 }    // EGP neighbor loss (obsolete)
         };

         // For standard trap types, create standard V2 Enterprise ID
         m_pEnterprise->setValue(pdwStdOid[m_iTrapType], 10);
      }
      else
      {
         m_pEnterprise->extend(0);
         m_pEnterprise->extend(m_iSpecificTrap);
      }
   }

   return bResult;
}

/**
 * Parse version 2 TRAP or INFORM-REQUEST PDU
 */
BOOL SNMP_PDU::parseTrap2PDU(BYTE *pData, UINT32 dwPDULength)
{
   BOOL bResult;
   static UINT32 pdwStdTrapPrefix[9] = { 1, 3, 6, 1, 6, 3, 1, 1, 5 };

   bResult = parsePduContent(pData, dwPDULength);
   if (bResult)
   {
      bResult = FALSE;
      if (m_dwNumVariables >= 2)
      {
         if (m_ppVarList[1]->GetType() == ASN_OBJECT_ID)
         {
            m_pEnterprise = new SNMP_ObjectId(
               m_ppVarList[1]->GetValueLength() / sizeof(UINT32), 
               (UINT32 *)m_ppVarList[1]->GetValue());
            bResult = TRUE;
         }
      }

      // Set V1 trap type and specific trap type fields
      if (bResult)
      {
         if ((m_pEnterprise->compare(pdwStdTrapPrefix, 9) == OID_SHORTER) &&
             (m_pEnterprise->getLength() == 10))
         {
            m_iTrapType = m_pEnterprise->getValue()[9];
            m_iSpecificTrap = 0;
         }
         else
         {
            m_iTrapType = 6;
            m_iSpecificTrap = m_pEnterprise->getValue()[m_pEnterprise->getLength() - 1];
         }
      }
   }
   return bResult;
}

/**
 * Parse version 3 header
 */
BOOL SNMP_PDU::parseV3Header(BYTE *header, UINT32 headerLength)
{
	UINT32 type, length, idLength, remLength = headerLength;
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
	m_reportable = (flags & SNMP_REPORTABLE_FLAG) ? true : false;
	m_flags = flags;
   currPos += length;
   remLength -= length + idLength;

   // Security model
   if (!BER_DecodeIdentifier(currPos, remLength, &type, &length, &currPos, &idLength))
      return FALSE;
   if (type != ASN_INTEGER)
      return FALSE;   // Should be of integer type
	UINT32 securityModel;
   if (!BER_DecodeContent(type, currPos, length, (BYTE *)&securityModel))
      return FALSE;   // Error parsing content
   currPos += length;
   remLength -= length + idLength;
	m_securityModel = (int)securityModel;

	return TRUE;
}

/**
 * Parse V3 USM security parameters
 */
BOOL SNMP_PDU::parseV3SecurityUsm(BYTE *data, UINT32 dataLength)
{
	UINT32 type, length, idLength, remLength = dataLength;
	UINT32 engineBoots, engineTime;
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

	m_authoritativeEngine = SNMP_Engine(engineId, engineIdLen, engineBoots, engineTime);

	// User name
   if (!BER_DecodeIdentifier(currPos, remLength, &type, &length, &currPos, &idLength))
      return FALSE;
   if (type != ASN_OCTET_STRING)
      return FALSE;
	m_authObject = (char *)malloc(length + 1);
	if (!BER_DecodeContent(type, currPos, length, (BYTE *)m_authObject))
	{
		free(m_authObject);
		m_authObject = NULL;
		return FALSE;
	}
	m_authObject[length] = 0;
   currPos += length;
   remLength -= length + idLength;

	// Message signature
   if (!BER_DecodeIdentifier(currPos, remLength, &type, &length, &currPos, &idLength))
      return FALSE;
   if (type != ASN_OCTET_STRING)
      return FALSE;
	memcpy(m_signature, currPos, min(length, 12));
	memset(currPos, 0, min(length, 12));	// Replace with 0 to generate correct hash in validate method
   currPos += length;
   remLength -= length + idLength;

	// Encryption salt
   if (!BER_DecodeIdentifier(currPos, remLength, &type, &length, &currPos, &idLength))
      return FALSE;
   if (type != ASN_OCTET_STRING)
      return FALSE;
	memcpy(m_salt, currPos, min(length, 8));
   currPos += length;
   remLength -= length + idLength;

	return TRUE;
}

/**
 * Parse V3 scoped PDU
 */
BOOL SNMP_PDU::parseV3ScopedPdu(BYTE *data, UINT32 dataLength)
{
	UINT32 type, length, idLength, remLength = dataLength;
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

/**
 * Parse PDU
 */
BOOL SNMP_PDU::parsePdu(BYTE *pdu, UINT32 pduLength)
{
	BYTE *content;
	UINT32 length, idLength, type;
	BOOL success;

   success = BER_DecodeIdentifier(pdu, pduLength, &type, &length, &content, &idLength);
   if (success)
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
            success = parseTrap2PDU(content, length);
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

/**
 * Validate V3 signed message
 */
BOOL SNMP_PDU::validateSignedMessage(BYTE *msg, UINT32 msgLen, SNMP_SecurityContext *securityContext)
{
	BYTE k1[64], k2[64], hash[20], *buffer;
	int i;

	if (securityContext == NULL)
		return FALSE;	// Unable to validate message without security context

	switch(securityContext->getAuthMethod())
	{
		case SNMP_AUTH_MD5:
			// Create K1 and K2
			memcpy(k1, securityContext->getAuthKeyMD5(), 16);
			memset(&k1[16], 0, 48);
			memcpy(k2, k1, 64);
			for(i = 0; i < 64; i++)
			{
				k1[i] ^= 0x36;
				k2[i] ^= 0x5C;
			}

			// Calculate first hash (step 3)
			buffer = (BYTE *)malloc(msgLen + 64);
			memcpy(buffer, k1, 64);
			memcpy(&buffer[64], msg, msgLen);
			CalculateMD5Hash(buffer, msgLen + 64, hash);

			// Calculate second hash
			memcpy(buffer, k2, 64);
			memcpy(&buffer[64], hash, 16);
			CalculateMD5Hash(buffer, 80, hash);
			free(buffer);
			break;
		case SNMP_AUTH_SHA1:
			// Create K1 and K2
			memcpy(k1, securityContext->getAuthKeySHA1(), 20);
			memset(&k1[20], 0, 44);
			memcpy(k2, k1, 64);
			for(i = 0; i < 64; i++)
			{
				k1[i] ^= 0x36;
				k2[i] ^= 0x5C;
			}

			// Calculate first hash (step 3)
			buffer = (BYTE *)malloc(msgLen + 64);
			memcpy(buffer, k1, 64);
			memcpy(&buffer[64], msg, msgLen);
			CalculateSHA1Hash(buffer, msgLen + 64, hash);

			// Calculate second hash
			memcpy(buffer, k2, 64);
			memcpy(&buffer[64], hash, 20);
			CalculateSHA1Hash(buffer, 84, hash);
			free(buffer);
			break;
		default:
			break;
	}

	// Computed hash should match message signature
	return !memcmp(m_signature, hash, 12);
}

/**
 * Decrypt data in packet
 */
BOOL SNMP_PDU::decryptData(BYTE *data, UINT32 length, BYTE *decryptedData, SNMP_SecurityContext *securityContext)
{
#ifdef _WITH_ENCRYPTION
	if (securityContext == NULL)
		return FALSE;	// Cannot decrypt message without valid security context

	if (securityContext->getPrivMethod() == SNMP_ENCRYPT_DES)
	{
#ifndef OPENSSL_NO_DES
		if (length % 8 != 0)
			return FALSE;	// Encrypted data length must be an integral multiple of 8

		DES_cblock key;
		DES_key_schedule schedule;
		memcpy(&key, securityContext->getPrivKey(), 8);
		DES_set_key_unchecked(&key, &schedule);

		DES_cblock iv;
		memcpy(&iv, securityContext->getPrivKey() + 8, 8);
		for(int i = 0; i < 8; i++)
			iv[i] ^= m_salt[i];

		DES_ncbc_encrypt(data, decryptedData, length, &schedule, &iv, DES_DECRYPT);
#else
		return FALSE;  // Compiled without DES support
#endif
	}
	else if (securityContext->getPrivMethod() == SNMP_ENCRYPT_AES)
	{
#ifndef OPENSSL_NO_AES
		AES_KEY key;
		AES_set_encrypt_key(securityContext->getPrivKey(), 128, &key);

		BYTE iv[16];
		UINT32 boots, engTime;
		// Use auth. engine from current PDU if possible
		if ((m_authoritativeEngine.getIdLen() > 0) && (m_authoritativeEngine.getBoots() > 0))
		{
			boots = htonl(m_authoritativeEngine.getBoots());
			engTime = htonl(m_authoritativeEngine.getTime());
		}
		else
		{
			boots = htonl((UINT32)securityContext->getAuthoritativeEngine().getBoots());
			engTime = htonl((UINT32)securityContext->getAuthoritativeEngine().getTime());
		}
		memcpy(iv, &boots, 4);
		memcpy(&iv[4], &engTime, 4);
		memcpy(&iv[8], m_salt, 8);

		int num = 0;
		AES_cfb128_encrypt(data, decryptedData, length, &key, iv, &num, AES_DECRYPT);
#else
		return FALSE;  // Compiled without AES support
#endif
	}
	else
	{
		return FALSE;
	}
	return TRUE;
#else
	return FALSE;	// No encryption support
#endif
}

/**
 * Create PDU from packet
 */
BOOL SNMP_PDU::parse(BYTE *pRawData, UINT32 dwRawLength, SNMP_SecurityContext *securityContext, bool engineIdAutoupdate)
{
   BYTE *pbCurrPos;
   UINT32 dwType, dwLength, dwPacketLength, dwIdLength;
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

		if (m_securityModel == SNMP_SECURITY_MODEL_USM)
		{
			if (!parseV3SecurityUsm(pbCurrPos, dwLength))
				return FALSE;

			if (engineIdAutoupdate && (m_authoritativeEngine.getIdLen() > 0) && (securityContext != NULL))
			{
				securityContext->setAuthoritativeEngine(m_authoritativeEngine);
			}

			if (m_flags & SNMP_AUTH_FLAG)
			{
				if (!validateSignedMessage(pRawData, dwRawLength, securityContext))
					return FALSE;
			}
		}

		pbCurrPos += dwLength;
		dwPacketLength -= dwLength + dwIdLength;

		// Decrypt scoped PDU if needed
		if ((m_securityModel == SNMP_SECURITY_MODEL_USM) && (m_flags & SNMP_PRIV_FLAG))
		{
			BYTE *scopedPduStart = pbCurrPos;

			if (!BER_DecodeIdentifier(pbCurrPos, dwPacketLength, &dwType, &dwLength, &pbCurrPos, &dwIdLength))
				return FALSE;
			if (dwType != ASN_OCTET_STRING)
				return FALSE;   // Should be encoded as octet string

			BYTE *decryptedPdu = (BYTE *)malloc(dwLength);
			if (!decryptData(pbCurrPos, dwLength, decryptedPdu, securityContext))
			{
				free(decryptedPdu);
				return FALSE;
			}

			pbCurrPos = scopedPduStart;
			memcpy(pbCurrPos, decryptedPdu, dwLength);
			free(decryptedPdu);
		}

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
		m_authObject = (char *)malloc(dwLength + 1);
		if (!BER_DecodeContent(dwType, pbCurrPos, dwLength, (BYTE *)m_authObject))
		{
			free(m_authObject);
			m_authObject = NULL;
			return FALSE;   // Error parsing content of version field
		}
		m_authObject[dwLength] = 0;
		pbCurrPos += dwLength;
		dwPacketLength -= dwLength + dwIdLength;

		bResult = parsePdu(pbCurrPos, dwLength);
	}

   return bResult;
}

/**
 * Create packet from PDU
 */
UINT32 SNMP_PDU::encode(BYTE **ppBuffer, SNMP_SecurityContext *securityContext)
{
   UINT32 i, dwBufferSize, dwBytes, dwVarBindsSize, dwPDUType, dwPDUSize, dwPacketSize, dwValue;
   BYTE *pbCurrPos, *pBlock, *pVarBinds, *pPacket;

	// Replace context name if defined in security context
	if (securityContext->getContextName() != NULL)
	{
		strncpy(m_contextName, securityContext->getContextName(), SNMP_MAX_CONTEXT_NAME);
		m_contextName[SNMP_MAX_CONTEXT_NAME - 1] = 0;
	}

   // Estimate required buffer size and allocate it
   for(dwBufferSize = 1024, i = 0; i < m_dwNumVariables; i++)
      dwBufferSize += m_ppVarList[i]->GetValueLength() + m_ppVarList[i]->GetName()->getLength() * 4 + 16;
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
      if (((m_dwVersion == (UINT32)PDU_type_to_command[i].iVersion) ||
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
            dwBytes = BER_Encode(ASN_OBJECT_ID, (BYTE *)m_pEnterprise->getValue(),
                                 m_pEnterprise->getLength() * sizeof(UINT32),
                                 pbCurrPos, dwBufferSize - dwPDUSize);
            dwPDUSize += dwBytes;
            pbCurrPos += dwBytes;

            dwBytes = BER_Encode(ASN_IP_ADDR, (BYTE *)&m_dwAgentAddr, sizeof(UINT32), 
                                 pbCurrPos, dwBufferSize - dwPDUSize);
            dwPDUSize += dwBytes;
            pbCurrPos += dwBytes;

            dwValue = (UINT32)m_iTrapType;
            dwBytes = BER_Encode(ASN_INTEGER, (BYTE *)&dwValue, sizeof(UINT32), 
                                 pbCurrPos, dwBufferSize - dwPDUSize);
            dwPDUSize += dwBytes;
            pbCurrPos += dwBytes;

            dwValue = (UINT32)m_iSpecificTrap;
            dwBytes = BER_Encode(ASN_INTEGER, (BYTE *)&dwValue, sizeof(UINT32), 
                                 pbCurrPos, dwBufferSize - dwPDUSize);
            dwPDUSize += dwBytes;
            pbCurrPos += dwBytes;

            dwBytes = BER_Encode(ASN_INTEGER, (BYTE *)&m_dwTimeStamp, sizeof(UINT32), 
                                 pbCurrPos, dwBufferSize - dwPDUSize);
            dwPDUSize += dwBytes;
            pbCurrPos += dwBytes;
            break;
         default:
            dwBytes = BER_Encode(ASN_INTEGER, (BYTE *)&m_dwRqId, sizeof(UINT32), 
                                 pbCurrPos, dwBufferSize - dwPDUSize);
            dwPDUSize += dwBytes;
            pbCurrPos += dwBytes;

            dwBytes = BER_Encode(ASN_INTEGER, (BYTE *)&m_dwErrorCode, sizeof(UINT32), 
                                 pbCurrPos, dwBufferSize - dwPDUSize);
            dwPDUSize += dwBytes;
            pbCurrPos += dwBytes;

            dwBytes = BER_Encode(ASN_INTEGER, (BYTE *)&m_dwErrorIndex, sizeof(UINT32), 
                                 pbCurrPos, dwBufferSize - dwPDUSize);
            dwPDUSize += dwBytes;
            pbCurrPos += dwBytes;
            break;
      }

      // Encode varbinds into PDU
		if ((m_dwVersion != SNMP_VERSION_3) || ((securityContext != NULL) && (securityContext->getAuthoritativeEngine().getIdLen() != 0)))
		{
			dwBytes = BER_Encode(ASN_SEQUENCE, pVarBinds, dwVarBindsSize, 
										pbCurrPos, dwBufferSize - dwPDUSize);
		}
		else
		{
			// Do not encode varbinds into engine id discovery message
			dwBytes = BER_Encode(ASN_SEQUENCE, NULL, 0, pbCurrPos, dwBufferSize - dwPDUSize);
		}
      dwPDUSize += dwBytes;

      // Encode packet header
      pbCurrPos = pPacket;
      dwPacketSize = 0;

      dwBytes = BER_Encode(ASN_INTEGER, (BYTE *)&m_dwVersion, sizeof(UINT32), 
                           pbCurrPos, dwBufferSize);
      dwPacketSize += dwBytes;
      pbCurrPos += dwBytes;

		if (m_dwVersion == SNMP_VERSION_3)
		{
			// Generate encryption salt if packet has to be encrypted
			if (securityContext->needEncryption())
			{
				QWORD temp = htonq(GetCurrentTimeMs());
				memcpy(m_salt, &temp, 8);
			}

			dwBytes = encodeV3Header(pbCurrPos, dwBufferSize - dwPacketSize, securityContext);
			dwPacketSize += dwBytes;
			pbCurrPos += dwBytes;

			dwBytes = encodeV3SecurityParameters(pbCurrPos, dwBufferSize - dwPacketSize, securityContext);
			dwPacketSize += dwBytes;
			pbCurrPos += dwBytes;

			dwBytes = encodeV3ScopedPDU(dwPDUType, pBlock, dwPDUSize, pbCurrPos, dwBufferSize - dwPacketSize);
			if (securityContext->needEncryption())
			{
#ifdef _WITH_ENCRYPTION
				if (securityContext->getPrivMethod() == SNMP_ENCRYPT_DES)
				{
#ifndef OPENSSL_NO_DES
					UINT32 encSize = (dwBytes % 8 == 0) ? dwBytes : (dwBytes + (8 - (dwBytes % 8)));
					BYTE *encryptedPdu = (BYTE *)malloc(encSize);

					DES_cblock key;
					DES_key_schedule schedule;
					memcpy(&key, securityContext->getPrivKey(), 8);
					DES_set_key_unchecked(&key, &schedule);

					DES_cblock iv;
					memcpy(&iv, securityContext->getPrivKey() + 8, 8);
					for(int i = 0; i < 8; i++)
						iv[i] ^= m_salt[i];

					DES_ncbc_encrypt(pbCurrPos, encryptedPdu, dwBytes, &schedule, &iv, DES_ENCRYPT);
					dwBytes = BER_Encode(ASN_OCTET_STRING, encryptedPdu, encSize, pbCurrPos, dwBufferSize - dwPacketSize);
					free(encryptedPdu);
#else
					dwBytes = 0;	// Error - no DES support
					goto cleanup;
#endif
				}
				else if (securityContext->getPrivMethod() == SNMP_ENCRYPT_AES)
				{
#ifndef OPENSSL_NO_AES
					AES_KEY key;
					AES_set_encrypt_key(securityContext->getPrivKey(), 128, &key);

					BYTE iv[16];
					UINT32 boots = htonl((UINT32)securityContext->getAuthoritativeEngine().getBoots());
					UINT32 engTime = htonl((UINT32)securityContext->getAuthoritativeEngine().getTime());
					memcpy(iv, &boots, 4);
					memcpy(&iv[4], &engTime, 4);
					memcpy(&iv[8], m_salt, 8);

					BYTE *encryptedPdu = (BYTE *)malloc(dwBytes);
					int num = 0;
					AES_cfb128_encrypt(pbCurrPos, encryptedPdu, dwBytes, &key, iv, &num, AES_ENCRYPT);
					dwBytes = BER_Encode(ASN_OCTET_STRING, encryptedPdu, dwBytes, pbCurrPos, dwBufferSize - dwPacketSize);
					free(encryptedPdu);
#else
					dwBytes = 0;	// Error - no AES support
					goto cleanup;
#endif
				}
				else
				{
					dwBytes = 0;	// Error - unsupported method
					goto cleanup;
				}
#else
				dwBytes = 0;	// Error
				goto cleanup;
#endif
			}
			dwPacketSize += dwBytes;
		}
		else
		{
			dwBytes = BER_Encode(ASN_OCTET_STRING, (BYTE *)securityContext->getCommunity(),
										(UINT32)strlen(securityContext->getCommunity()), pbCurrPos,
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

		// Sign message
		if ((m_dwVersion == SNMP_VERSION_3) && securityContext->needAuthentication())
		{
			signMessage(*ppBuffer, dwBytes, securityContext);
		}
   }
   else
   {
      dwBytes = 0;   // Error
   }

cleanup:
   free(pPacket);
   free(pBlock);
   free(pVarBinds);
   return dwBytes;
}

/**
 * Encode version 3 header
 */
UINT32 SNMP_PDU::encodeV3Header(BYTE *buffer, UINT32 bufferSize, SNMP_SecurityContext *securityContext)
{
	BYTE header[256];
	UINT32 bytes, securityModel = securityContext->getSecurityModel();

	BYTE flags = m_reportable ? SNMP_REPORTABLE_FLAG : 0;
	if (securityContext->getAuthoritativeEngine().getIdLen() != 0)
	{
		if (securityContext->needAuthentication())
		{
			flags |= SNMP_AUTH_FLAG;
			if (securityContext->needEncryption())
			{
				flags |= SNMP_PRIV_FLAG;
			}
		}
	}

	bytes = BER_Encode(ASN_INTEGER, (BYTE *)&m_msgId, sizeof(UINT32), header, 256);
	bytes += BER_Encode(ASN_INTEGER, (BYTE *)&m_msgMaxSize, sizeof(UINT32), &header[bytes], 256 - bytes);
	bytes += BER_Encode(ASN_OCTET_STRING, &flags, 1, &header[bytes], 256 - bytes);
	bytes += BER_Encode(ASN_INTEGER, (BYTE *)&securityModel, sizeof(UINT32), &header[bytes], 256 - bytes);
	return BER_Encode(ASN_SEQUENCE, header, bytes, buffer, bufferSize);
}

/**
 * Encode version 3 security parameters
 */
UINT32 SNMP_PDU::encodeV3SecurityParameters(BYTE *buffer, UINT32 bufferSize, SNMP_SecurityContext *securityContext)
{
	UINT32 bytes;

	if ((securityContext != NULL) && (securityContext->getSecurityModel() == SNMP_SECURITY_MODEL_USM))
	{
   	BYTE securityParameters[1024], sequence[1040];

	   UINT32 engineBoots = securityContext->getAuthoritativeEngine().getBoots();
	   UINT32 engineTime = securityContext->getAuthoritativeEngine().getTime();

		bytes = BER_Encode(ASN_OCTET_STRING, securityContext->getAuthoritativeEngine().getId(),
		                   securityContext->getAuthoritativeEngine().getIdLen(), securityParameters, 1024);
		bytes += BER_Encode(ASN_INTEGER, (BYTE *)&engineBoots, sizeof(UINT32), &securityParameters[bytes], 1024 - bytes);
		bytes += BER_Encode(ASN_INTEGER, (BYTE *)&engineTime, sizeof(UINT32), &securityParameters[bytes], 1024 - bytes);

		// Don't send user and auth/priv parameters in engine id discovery message
		if (securityContext->getAuthoritativeEngine().getIdLen() != 0)
		{
			bytes += BER_Encode(ASN_OCTET_STRING, (BYTE *)securityContext->getUser(), (UINT32)strlen(securityContext->getUser()), &securityParameters[bytes], 1024 - bytes);

			// Authentication parameters
			if (securityContext->needAuthentication())
			{
				// Add placeholder for message hash
				bytes += BER_Encode(ASN_OCTET_STRING, m_hashPlaceholder, 12, &securityParameters[bytes], 1024 - bytes);
			}
			else
			{
				bytes += BER_Encode(ASN_OCTET_STRING, NULL, 0, &securityParameters[bytes], 1024 - bytes);
			}

			// Privacy parameters
			if (securityContext->needEncryption())
			{
				bytes += BER_Encode(ASN_OCTET_STRING, m_salt, 8, &securityParameters[bytes], 1024 - bytes);
			}
			else
			{
				bytes += BER_Encode(ASN_OCTET_STRING, NULL, 0, &securityParameters[bytes], 1024 - bytes);
			}
		}
		else
		{
			// Empty strings in place of user name, auth parameters and privacy parameters
			bytes += BER_Encode(ASN_OCTET_STRING, NULL, 0, &securityParameters[bytes], 1024 - bytes);
			bytes += BER_Encode(ASN_OCTET_STRING, NULL, 0, &securityParameters[bytes], 1024 - bytes);
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

/**
 * Encode versionj 3 scoped PDU
 */
UINT32 SNMP_PDU::encodeV3ScopedPDU(UINT32 pduType, BYTE *pdu, UINT32 pduSize, BYTE *buffer, UINT32 bufferSize)
{
	UINT32 spduLen = pduSize + SNMP_MAX_CONTEXT_NAME + SNMP_MAX_ENGINEID_LEN + 32;
	BYTE *spdu = (BYTE *)malloc(spduLen);
	UINT32 bytes;

	bytes = BER_Encode(ASN_OCTET_STRING, m_contextEngineId, (UINT32)m_contextEngineIdLen, spdu, spduLen);
	bytes += BER_Encode(ASN_OCTET_STRING, (BYTE *)m_contextName, (UINT32)strlen(m_contextName), &spdu[bytes], spduLen - bytes);
	bytes += BER_Encode(pduType, pdu, pduSize, &spdu[bytes], spduLen - bytes);
	
	// Wrap scoped PDU into SEQUENCE
	bytes = BER_Encode(ASN_SEQUENCE, spdu, bytes, buffer, bufferSize);
	free(spdu);
	return bytes;
}

/**
 * Sign message
 *
 * Algorithm described in RFC:
 *   1) The msgAuthenticationParameters field is set to the serialization,
 *      according to the rules in [RFC3417], of an OCTET STRING containing
 *      12 zero octets.
 *   2) From the secret authKey, two keys K1 and K2 are derived:
 *      a) extend the authKey to 64 octets by appending 48 zero octets;
 *         save it as extendedAuthKey
 *      b) obtain IPAD by replicating the octet 0x36 64 times;
 *      c) obtain K1 by XORing extendedAuthKey with IPAD;
 *      d) obtain OPAD by replicating the octet 0x5C 64 times;
 *      e) obtain K2 by XORing extendedAuthKey with OPAD.
 *   3) Prepend K1 to the wholeMsg and calculate MD5 digest over it
 *      according to [RFC1321].
 *   4) Prepend K2 to the result of the step 4 and calculate MD5 digest
 *      over it according to [RFC1321].  Take the first 12 octets of the
 *      final digest - this is Message Authentication Code (MAC).
 *   5) Replace the msgAuthenticationParameters field with MAC obtained in
 *      the step 4.
 */
void SNMP_PDU::signMessage(BYTE *msg, UINT32 msgLen, SNMP_SecurityContext *securityContext)
{
	int i, hashPos;

	// Find placeholder for hash
	for(hashPos = 0; hashPos < (int)msgLen - 12; hashPos++)
		if (!memcmp(&msg[hashPos], m_hashPlaceholder, 12))
			break;

	// Fill hash placeholder with zeroes
	memset(&msg[hashPos], 0, 12);

	BYTE k1[64], k2[64], hash[20], *buffer;
	switch(securityContext->getAuthMethod())
	{
		case SNMP_AUTH_MD5:
			// Create K1 and K2
			memcpy(k1, securityContext->getAuthKeyMD5(), 16);
			memset(&k1[16], 0, 48);
			memcpy(k2, k1, 64);
			for(i = 0; i < 64; i++)
			{
				k1[i] ^= 0x36;
				k2[i] ^= 0x5C;
			}

			// Calculate first hash (step 3)
			buffer = (BYTE *)malloc(msgLen + 64);
			memcpy(buffer, k1, 64);
			memcpy(&buffer[64], msg, msgLen);
			CalculateMD5Hash(buffer, msgLen + 64, hash);

			// Calculate second hash
			memcpy(buffer, k2, 64);
			memcpy(&buffer[64], hash, 16);
			CalculateMD5Hash(buffer, 80, hash);
			free(buffer);
			break;
		case SNMP_AUTH_SHA1:
			// Create K1 and K2
			memcpy(k1, securityContext->getAuthKeySHA1(), 20);
			memset(&k1[20], 0, 44);
			memcpy(k2, k1, 64);
			for(i = 0; i < 64; i++)
			{
				k1[i] ^= 0x36;
				k2[i] ^= 0x5C;
			}

			// Calculate first hash (step 3)
			buffer = (BYTE *)malloc(msgLen + 64);
			memcpy(buffer, k1, 64);
			memcpy(&buffer[64], msg, msgLen);
			CalculateSHA1Hash(buffer, msgLen + 64, hash);

			// Calculate second hash
			memcpy(buffer, k2, 64);
			memcpy(&buffer[64], hash, 20);
			CalculateSHA1Hash(buffer, 84, hash);
			free(buffer);
			break;
		default:
			break;
	}

	// Update message hash
	memcpy(&msg[hashPos], hash, 12);
}

/**
 * Bind variable to PDU
 */
void SNMP_PDU::bindVariable(SNMP_Variable *pVar)
{
   m_ppVarList = (SNMP_Variable **)realloc(m_ppVarList, sizeof(SNMP_Variable *) * (m_dwNumVariables + 1));
   m_ppVarList[m_dwNumVariables] = pVar;
   m_dwNumVariables++;
}

/**
 * Set context engine ID
 */
void SNMP_PDU::setContextEngineId(BYTE *id, int len)
{
	m_contextEngineIdLen = min(len, SNMP_MAX_ENGINEID_LEN);
	memcpy(m_contextEngineId, id, m_contextEngineIdLen);
}

/**
 * Set context engine ID
 */
void SNMP_PDU::setContextEngineId(const char *id)
{
	m_contextEngineIdLen = min((int)strlen(id), SNMP_MAX_ENGINEID_LEN);
	memcpy(m_contextEngineId, id, m_contextEngineIdLen);
}

/**
 * Set context name
 */
void SNMP_PDU::setContextName(const char *name)
{
	strncpy(m_contextName, name, SNMP_MAX_CONTEXT_NAME);
	m_contextName[SNMP_MAX_CONTEXT_NAME - 1] = 0;
}
