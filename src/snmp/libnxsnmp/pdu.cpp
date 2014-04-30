/* 
** NetXMS - Network Management System
** SNMP support library
** Copyright (C) 2003-2014 Victor Kirhenshtein
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
   m_version = SNMP_VERSION_1;
   m_command = SNMP_INVALID_PDU;
   m_variables = new ObjectArray<SNMP_Variable>(0, 16, true);
   m_pEnterprise = NULL;
   m_dwErrorCode = 0;
   m_dwErrorIndex = 0;
   m_dwRqId = 0;
	m_msgId = 0;
	m_flags = 0;
   m_trapType = 0;
   m_specificTrap = 0;
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
   m_version = dwVersion;
   m_command = dwCommand;
   m_variables = new ObjectArray<SNMP_Variable>(0, 16, true);
   m_pEnterprise = NULL;
   m_dwErrorCode = 0;
   m_dwErrorIndex = 0;
   m_dwRqId = dwRqId;
	m_msgId = dwRqId;
	m_flags = 0;
   m_trapType = 0;
   m_specificTrap = 0;
	m_contextEngineIdLen = 0;
	m_contextName[0] = 0;
	m_msgMaxSize = SNMP_DEFAULT_MSG_MAX_SIZE;
	m_authObject = NULL;
	m_reportable = true;
}

/**
 * Copy destructor
 */
SNMP_PDU::SNMP_PDU(SNMP_PDU *src)
{
   m_version = src->m_version;
   m_command = src->m_command;
   m_variables = new ObjectArray<SNMP_Variable>(0, 16, true);
   for(int i = 0; i < src->m_variables->size(); i++)
      m_variables->add(new SNMP_Variable(src->m_variables->get(i)));
   m_pEnterprise = (src->m_pEnterprise != NULL) ? new SNMP_ObjectId(src->m_pEnterprise) : NULL;
   m_dwErrorCode = src->m_dwErrorCode;
   m_dwErrorIndex = src->m_dwErrorIndex;
   m_dwRqId = src->m_dwRqId;
	m_msgId = src->m_msgId;
	m_flags = src->m_flags;
   m_trapType = src->m_trapType;
   m_specificTrap = src->m_specificTrap;
	m_contextEngineIdLen = src->m_contextEngineIdLen;
	strcpy(m_contextName, src->m_contextName);
	m_msgMaxSize = src->m_msgMaxSize;
   m_authObject = (src->m_authObject != NULL) ? strdup(src->m_authObject) : NULL;
	m_reportable = src->m_reportable;
}

/**
 * SNMP_PDU destructor
 */
SNMP_PDU::~SNMP_PDU()
{
   delete m_pEnterprise;
   delete m_variables;
	safe_free(m_authObject);
}

/**
 * Parse single variable binding
 */
bool SNMP_PDU::parseVariable(BYTE *pData, size_t varLength)
{
   SNMP_Variable *var;
   bool success = true;

   var = new SNMP_Variable;
   if (var->parse(pData, varLength))
   {
      bindVariable(var);
   }
   else
   {
      delete var;
      success = false;
   }
   return success;
}

/**
 * Parse variable bindings
 */
bool SNMP_PDU::parseVarBinds(BYTE *pData, size_t pduLength)
{
   BYTE *pbCurrPos;
   UINT32 dwType;
   size_t dwLength, dwBindingLength, idLength;

   // Varbind section should be a SEQUENCE
   if (!BER_DecodeIdentifier(pData, pduLength, &dwType, &dwBindingLength, &pbCurrPos, &idLength))
      return false;
   if (dwType != ASN_SEQUENCE)
      return false;

   while(dwBindingLength > 0)
   {
      if (!BER_DecodeIdentifier(pbCurrPos, pduLength, &dwType, &dwLength, &pbCurrPos, &idLength))
         return false;
      if (dwType != ASN_SEQUENCE)
         return false;  // Every binding is a sequence
      if (dwLength > dwBindingLength)
         return false;     // Invalid length

      if (!parseVariable(pbCurrPos, dwLength))
         return false;
      dwBindingLength -= dwLength + idLength;
      pbCurrPos += dwLength;
   }

   return true;
}

/**
 * Parse generic PDU content
 */
bool SNMP_PDU::parsePduContent(BYTE *pData, size_t pduLength)
{
   UINT32 dwType;
   size_t dwLength, idLength;
   BYTE *pbCurrPos = pData;
   bool bResult = false;

   // Request ID
   if (BER_DecodeIdentifier(pbCurrPos, pduLength, &dwType, &dwLength, &pbCurrPos, &idLength))
   {
      if ((dwType == ASN_INTEGER) &&
          BER_DecodeContent(dwType, pbCurrPos, dwLength, (BYTE *)&m_dwRqId))
      {
         pduLength -= dwLength + idLength;
         pbCurrPos += dwLength;
         bResult = true;
      }
   }

   // Error code
   if (bResult)
   {
      bResult = false;
      if (BER_DecodeIdentifier(pbCurrPos, pduLength, &dwType, &dwLength, &pbCurrPos, &idLength))
      {
         if ((dwType == ASN_INTEGER) &&
             BER_DecodeContent(dwType, pbCurrPos, dwLength, (BYTE *)&m_dwErrorCode))
         {
            pduLength -= dwLength + idLength;
            pbCurrPos += dwLength;
            bResult = true;
         }
      }
   }

   // Error index
   if (bResult)
   {
      bResult = false;
      if (BER_DecodeIdentifier(pbCurrPos, pduLength, &dwType, &dwLength, &pbCurrPos, &idLength))
      {
         if ((dwType == ASN_INTEGER) &&
             BER_DecodeContent(dwType, pbCurrPos, dwLength, (BYTE *)&m_dwErrorIndex))
         {
            pduLength -= dwLength + idLength;
            pbCurrPos += dwLength;
            bResult = true;
         }
      }
   }

   if (bResult)
      bResult = parseVarBinds(pbCurrPos, pduLength);

   return bResult;
}

/**
 * Parse version 1 TRAP PDU
 */
bool SNMP_PDU::parseTrapPDU(BYTE *pData, size_t pduLength)
{
   UINT32 dwType;
   size_t dwLength, idLength;
   BYTE *pbCurrPos = pData;
   SNMP_OID *oid;
   UINT32 dwBuffer;
   bool bResult = false;

   // Enterprise ID
   if (BER_DecodeIdentifier(pbCurrPos, pduLength, &dwType, &dwLength, &pbCurrPos, &idLength))
   {
      if (dwType == ASN_OBJECT_ID)
      {
         oid = (SNMP_OID *)malloc(sizeof(SNMP_OID));
         memset(oid, 0, sizeof(SNMP_OID));
         if (BER_DecodeContent(dwType, pbCurrPos, dwLength, (BYTE *)oid))
         {
            m_pEnterprise = new SNMP_ObjectId(oid->length, oid->value);
            pduLength -= dwLength + idLength;
            pbCurrPos += dwLength;

            bResult = true;
         }
         safe_free(oid->value);
         free(oid);
      }
   }

   // Agent's address
   if (bResult)
   {
      bResult = false;
      if (BER_DecodeIdentifier(pbCurrPos, pduLength, &dwType, &dwLength, &pbCurrPos, &idLength))
      {
         if ((dwType == ASN_IP_ADDR) && (dwLength == 4) &&
             BER_DecodeContent(dwType, pbCurrPos, dwLength, (BYTE *)&m_dwAgentAddr))
         {
            pduLength -= dwLength + idLength;
            pbCurrPos += dwLength;
            bResult = true;
         }
      }
   }

   // Generic trap type
   if (bResult)
   {
      bResult = false;
      if (BER_DecodeIdentifier(pbCurrPos, pduLength, &dwType, &dwLength, &pbCurrPos, &idLength))
      {
         if ((dwType == ASN_INTEGER) &&
             BER_DecodeContent(dwType, pbCurrPos, dwLength, (BYTE *)&dwBuffer))
         {
            pduLength -= dwLength + idLength;
            pbCurrPos += dwLength;
            m_trapType = (int)dwBuffer;
            bResult = true;
         }
      }
   }

   // Enterprise trap type
   if (bResult)
   {
      bResult = false;
      if (BER_DecodeIdentifier(pbCurrPos, pduLength, &dwType, &dwLength, &pbCurrPos, &idLength))
      {
         if ((dwType == ASN_INTEGER) &&
             BER_DecodeContent(dwType, pbCurrPos, dwLength, (BYTE *)&dwBuffer))
         {
            pduLength -= dwLength + idLength;
            pbCurrPos += dwLength;
            m_specificTrap = (int)dwBuffer;
            bResult = true;
         }
      }
   }

   // Timestamp
   if (bResult)
   {
      bResult = false;
      if (BER_DecodeIdentifier(pbCurrPos, pduLength, &dwType, &dwLength, &pbCurrPos, &idLength))
      {
         if ((dwType == ASN_TIMETICKS) &&
             BER_DecodeContent(dwType, pbCurrPos, dwLength, (BYTE *)&m_dwTimeStamp))
         {
            pduLength -= dwLength + idLength;
            pbCurrPos += dwLength;
            bResult = true;
         }
      }
   }

   if (bResult)
      bResult = parseVarBinds(pbCurrPos, pduLength);

   if (bResult)
   {
      if (m_trapType < 6)
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
         m_pEnterprise->setValue(pdwStdOid[m_trapType], 10);
      }
      else
      {
         m_pEnterprise->extend(0);
         m_pEnterprise->extend(m_specificTrap);
      }
   }

   return bResult;
}

/**
 * Parse version 2 TRAP or INFORM-REQUEST PDU
 */
bool SNMP_PDU::parseTrap2PDU(BYTE *pData, size_t pduLength)
{
   bool bResult;
   static UINT32 pdwStdTrapPrefix[9] = { 1, 3, 6, 1, 6, 3, 1, 1, 5 };

   bResult = parsePduContent(pData, pduLength);
   if (bResult)
   {
      bResult = false;
      if (m_variables->size() >= 2)
      {
         SNMP_Variable *var = m_variables->get(1);
         if (var->getType() == ASN_OBJECT_ID)
         {
            m_pEnterprise = new SNMP_ObjectId(var->getValueLength() / sizeof(UINT32), (UINT32 *)var->getValue());
            bResult = true;
         }
      }

      // Set V1 trap type and specific trap type fields
      if (bResult)
      {
         if ((m_pEnterprise->compare(pdwStdTrapPrefix, 9) == OID_SHORTER) &&
             (m_pEnterprise->getLength() == 10))
         {
            m_trapType = m_pEnterprise->getValue()[9];
            m_specificTrap = 0;
         }
         else
         {
            m_trapType = 6;
            m_specificTrap = m_pEnterprise->getValue()[m_pEnterprise->getLength() - 1];
         }
      }
   }
   return bResult;
}

/**
 * Parse version 3 header
 */
bool SNMP_PDU::parseV3Header(BYTE *header, size_t headerLength)
{
	UINT32 type;
   size_t length, idLength, remLength = headerLength;
	BYTE *currPos = header;

   // Message id
   if (!BER_DecodeIdentifier(currPos, remLength, &type, &length, &currPos, &idLength))
      return false;
   if (type != ASN_INTEGER)
      return false;   // Should be of integer type
   if (!BER_DecodeContent(type, currPos, length, (BYTE *)&m_msgId))
      return false;   // Error parsing content
   currPos += length;
   remLength -= length + idLength;

   // Message max size
   if (!BER_DecodeIdentifier(currPos, remLength, &type, &length, &currPos, &idLength))
      return false;
   if (type != ASN_INTEGER)
      return false;   // Should be of integer type
   if (!BER_DecodeContent(type, currPos, length, (BYTE *)&m_msgMaxSize))
      return false;   // Error parsing content
   currPos += length;
   remLength -= length + idLength;

   // Message flags
   if (!BER_DecodeIdentifier(currPos, remLength, &type, &length, &currPos, &idLength))
      return false;
   if ((type != ASN_OCTET_STRING) || (length != 1))
      return false;
	BYTE flags;
   if (!BER_DecodeContent(type, currPos, length, &flags))
      return false;   // Error parsing content
	m_reportable = (flags & SNMP_REPORTABLE_FLAG) ? true : false;
	m_flags = flags;
   currPos += length;
   remLength -= length + idLength;

   // Security model
   if (!BER_DecodeIdentifier(currPos, remLength, &type, &length, &currPos, &idLength))
      return false;
   if (type != ASN_INTEGER)
      return false;   // Should be of integer type
	UINT32 securityModel;
   if (!BER_DecodeContent(type, currPos, length, (BYTE *)&securityModel))
      return false;   // Error parsing content
	m_securityModel = (int)securityModel;

	return true;
}

/**
 * Parse V3 USM security parameters
 */
bool SNMP_PDU::parseV3SecurityUsm(BYTE *data, size_t dataLength)
{
	UINT32 type;
   size_t length, idLength, engineIdLen, remLength = dataLength;
	UINT32 engineBoots, engineTime;
	BYTE *currPos = data;
	BYTE engineId[SNMP_MAX_ENGINEID_LEN];

	// Should be sequence
   if (!BER_DecodeIdentifier(currPos, remLength, &type, &length, &currPos, &idLength))
      return false;
   if (type != ASN_SEQUENCE)
      return false;
   remLength = length;

	// Engine ID
   if (!BER_DecodeIdentifier(currPos, remLength, &type, &length, &currPos, &idLength))
      return false;
   if (type != ASN_OCTET_STRING)
      return false;
	engineIdLen = length;
   if (!BER_DecodeContent(type, currPos, length, engineId))
      return false;   // Error parsing content
   currPos += length;
   remLength -= length + idLength;

	// engine boots
   if (!BER_DecodeIdentifier(currPos, remLength, &type, &length, &currPos, &idLength))
      return false;
   if (type != ASN_INTEGER)
      return false;
   if (!BER_DecodeContent(type, currPos, length, (BYTE *)&engineBoots))
      return false;   // Error parsing content
   currPos += length;
   remLength -= length + idLength;

	// engine time
   if (!BER_DecodeIdentifier(currPos, remLength, &type, &length, &currPos, &idLength))
      return false;
   if (type != ASN_INTEGER)
      return false;
   if (!BER_DecodeContent(type, currPos, length, (BYTE *)&engineTime))
      return false;   // Error parsing content
   currPos += length;
   remLength -= length + idLength;

	m_authoritativeEngine = SNMP_Engine(engineId, engineIdLen, engineBoots, engineTime);

	// User name
   if (!BER_DecodeIdentifier(currPos, remLength, &type, &length, &currPos, &idLength))
      return false;
   if (type != ASN_OCTET_STRING)
      return false;
	m_authObject = (char *)malloc(length + 1);
	if (!BER_DecodeContent(type, currPos, length, (BYTE *)m_authObject))
	{
		free(m_authObject);
		m_authObject = NULL;
		return false;
	}
	m_authObject[length] = 0;
   currPos += length;
   remLength -= length + idLength;

	// Message signature
   if (!BER_DecodeIdentifier(currPos, remLength, &type, &length, &currPos, &idLength))
      return false;
   if (type != ASN_OCTET_STRING)
      return false;
	memcpy(m_signature, currPos, min(length, 12));
	memset(currPos, 0, min(length, 12));	// Replace with 0 to generate correct hash in validate method
   currPos += length;
   remLength -= length + idLength;

	// Encryption salt
   if (!BER_DecodeIdentifier(currPos, remLength, &type, &length, &currPos, &idLength))
      return false;
   if (type != ASN_OCTET_STRING)
      return false;
	memcpy(m_salt, currPos, min(length, 8));

	return true;
}

/**
 * Parse V3 scoped PDU
 */
bool SNMP_PDU::parseV3ScopedPdu(BYTE *data, size_t dataLength)
{
	UINT32 type;
   size_t length, idLength, remLength = dataLength;
	BYTE *currPos = data;

   // Context engine ID
   if (!BER_DecodeIdentifier(currPos, remLength, &type, &length, &currPos, &idLength))
      return false;
   if ((type != ASN_OCTET_STRING) || (length > SNMP_MAX_ENGINEID_LEN))
      return false;
	m_contextEngineIdLen = length;
   if (!BER_DecodeContent(type, currPos, length, m_contextEngineId))
      return false;   // Error parsing content
   currPos += length;
   remLength -= length + idLength;
	
   // Context name
   if (!BER_DecodeIdentifier(currPos, remLength, &type, &length, &currPos, &idLength))
      return false;
   if ((type != ASN_OCTET_STRING) || (length >= SNMP_MAX_CONTEXT_NAME))
      return false;
   if (!BER_DecodeContent(type, currPos, length, (BYTE *)m_contextName))
      return false;   // Error parsing content
	m_contextName[length] = 0;
   currPos += length;
   remLength -= length + idLength;
	
	return parsePdu(currPos, remLength);
}

/**
 * Parse PDU
 */
bool SNMP_PDU::parsePdu(BYTE *pdu, size_t pduLength)
{
	BYTE *content;
	size_t length, idLength;
   UINT32 type;
	bool success;

   success = BER_DecodeIdentifier(pdu, pduLength, &type, &length, &content, &idLength);
   if (success)
   {
      switch(type)
      {
         case ASN_TRAP_V1_PDU:
            m_command = SNMP_TRAP;
            success = parseTrapPDU(content, length);
            break;
         case ASN_TRAP_V2_PDU:
            m_command = SNMP_TRAP;
            success = parseTrap2PDU(content, length);
            break;
         case ASN_GET_REQUEST_PDU:
            m_command = SNMP_GET_REQUEST;
            success = parsePduContent(content, length);
            break;
         case ASN_GET_NEXT_REQUEST_PDU:
            m_command = SNMP_GET_NEXT_REQUEST;
            success = parsePduContent(content, length);
            break;
         case ASN_RESPONSE_PDU:
            m_command = SNMP_RESPONSE;
            success = parsePduContent(content, length);
            break;
         case ASN_SET_REQUEST_PDU:
            m_command = SNMP_SET_REQUEST;
            success = parsePduContent(content, length);
            break;
         case ASN_INFORM_REQUEST_PDU:
            m_command = SNMP_INFORM_REQUEST;
            success = parseTrap2PDU(content, length);
            break;
         case ASN_REPORT_PDU:
            m_command = SNMP_REPORT;
            success = parsePduContent(content, length);
            break;
         default:
				success = false;
            break;
      }
   }
	return success;
}

/**
 * Validate V3 signed message
 */
bool SNMP_PDU::validateSignedMessage(BYTE *msg, size_t msgLen, SNMP_SecurityContext *securityContext)
{
	BYTE k1[64], k2[64], hash[20], *buffer;
	int i;

	if (securityContext == NULL)
		return false;	// Unable to validate message without security context

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
bool SNMP_PDU::decryptData(BYTE *data, size_t length, BYTE *decryptedData, SNMP_SecurityContext *securityContext)
{
#ifdef _WITH_ENCRYPTION
	if (securityContext == NULL)
		return false;	// Cannot decrypt message without valid security context

	if (securityContext->getPrivMethod() == SNMP_ENCRYPT_DES)
	{
#ifndef OPENSSL_NO_DES
		if (length % 8 != 0)
			return false;	// Encrypted data length must be an integral multiple of 8

		DES_cblock key;
		DES_key_schedule schedule;
		memcpy(&key, securityContext->getPrivKey(), 8);
		DES_set_key_unchecked(&key, &schedule);

		DES_cblock iv;
		memcpy(&iv, securityContext->getPrivKey() + 8, 8);
		for(int i = 0; i < 8; i++)
			iv[i] ^= m_salt[i];

		DES_ncbc_encrypt(data, decryptedData, (long)length, &schedule, &iv, DES_DECRYPT);
#else
		return false;  // Compiled without DES support
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
		return false;  // Compiled without AES support
#endif
	}
	else
	{
		return false;
	}
	return true;
#else
	return false;	// No encryption support
#endif
}

/**
 * Create PDU from packet
 */
bool SNMP_PDU::parse(BYTE *rawData, size_t rawLength, SNMP_SecurityContext *securityContext, bool engineIdAutoupdate)
{
   BYTE *pbCurrPos;
   UINT32 dwType;
   size_t dwLength, dwPacketLength, idLength;
   bool bResult = false;

   // Packet start
   if (!BER_DecodeIdentifier(rawData, rawLength, &dwType, &dwPacketLength, &pbCurrPos, &idLength))
      return false;
   if (dwType != ASN_SEQUENCE)
      return false;   // Packet should start with SEQUENCE

   // Version
   if (!BER_DecodeIdentifier(pbCurrPos, dwPacketLength, &dwType, &dwLength, &pbCurrPos, &idLength))
      return false;
   if (dwType != ASN_INTEGER)
      return false;   // Version field should be of integer type
   if (!BER_DecodeContent(dwType, pbCurrPos, dwLength, (BYTE *)&m_version))
      return false;   // Error parsing content of version field
   pbCurrPos += dwLength;
   dwPacketLength -= dwLength + idLength;
   if ((m_version != SNMP_VERSION_1) && (m_version != SNMP_VERSION_2C) && ((m_version != SNMP_VERSION_3)))
      return false;   // Unsupported SNMP version

	if (m_version == SNMP_VERSION_3)
	{
		// V3 header
		if (!BER_DecodeIdentifier(pbCurrPos, dwPacketLength, &dwType, &dwLength, &pbCurrPos, &idLength))
			return false;
		if (dwType != ASN_SEQUENCE)
			return false;   // Should be sequence
		
		// We don't need BER_DecodeContent because sequence does not need any special decoding
		if (!parseV3Header(pbCurrPos, dwLength))
			return false;
		pbCurrPos += dwLength;
		dwPacketLength -= dwLength + idLength;

		// Security parameters
		if (!BER_DecodeIdentifier(pbCurrPos, dwPacketLength, &dwType, &dwLength, &pbCurrPos, &idLength))
			return false;
		if (dwType != ASN_OCTET_STRING)
			return false;   // Should be octet string

		if (m_securityModel == SNMP_SECURITY_MODEL_USM)
		{
			if (!parseV3SecurityUsm(pbCurrPos, dwLength))
				return false;

			if (engineIdAutoupdate && (m_authoritativeEngine.getIdLen() > 0) && (securityContext != NULL))
			{
				securityContext->setAuthoritativeEngine(m_authoritativeEngine);
			}

			if (m_flags & SNMP_AUTH_FLAG)
			{
				if (!validateSignedMessage(rawData, rawLength, securityContext))
					return false;
			}
		}

		pbCurrPos += dwLength;
		dwPacketLength -= dwLength + idLength;

		// Decrypt scoped PDU if needed
		if ((m_securityModel == SNMP_SECURITY_MODEL_USM) && (m_flags & SNMP_PRIV_FLAG))
		{
			BYTE *scopedPduStart = pbCurrPos;

			if (!BER_DecodeIdentifier(pbCurrPos, dwPacketLength, &dwType, &dwLength, &pbCurrPos, &idLength))
				return false;
			if (dwType != ASN_OCTET_STRING)
				return false;   // Should be encoded as octet string

			BYTE *decryptedPdu = (BYTE *)malloc(dwLength);
			if (!decryptData(pbCurrPos, dwLength, decryptedPdu, securityContext))
			{
				free(decryptedPdu);
				return false;
			}

			pbCurrPos = scopedPduStart;
			memcpy(pbCurrPos, decryptedPdu, dwLength);
			free(decryptedPdu);
		}

		// Scoped PDU
		if (!BER_DecodeIdentifier(pbCurrPos, dwPacketLength, &dwType, &dwLength, &pbCurrPos, &idLength))
			return false;
		if (dwType != ASN_SEQUENCE)
			return false;   // Should be sequence
		bResult = parseV3ScopedPdu(pbCurrPos, dwLength);
	}
	else
	{
		// Community string
		if (!BER_DecodeIdentifier(pbCurrPos, dwPacketLength, &dwType, &dwLength, &pbCurrPos, &idLength))
			return false;
		if (dwType != ASN_OCTET_STRING)
			return false;   // Community field should be of string type
		m_authObject = (char *)malloc(dwLength + 1);
		if (!BER_DecodeContent(dwType, pbCurrPos, dwLength, (BYTE *)m_authObject))
		{
			free(m_authObject);
			m_authObject = NULL;
			return false;   // Error parsing content of version field
		}
		m_authObject[dwLength] = 0;
		pbCurrPos += dwLength;
		dwPacketLength -= dwLength + idLength;

		bResult = parsePdu(pbCurrPos, dwLength);
	}

   return bResult;
}

/**
 * Create packet from PDU
 */
size_t SNMP_PDU::encode(BYTE **ppBuffer, SNMP_SecurityContext *securityContext)
{
   int i;
   UINT32 dwValue, dwPDUType;
   size_t dwBufferSize, dwBytes, dwVarBindsSize, dwPDUSize, dwPacketSize;
   BYTE *pbCurrPos, *pBlock, *pVarBinds, *pPacket;

	// Replace context name if defined in security context
	if (securityContext->getContextName() != NULL)
	{
		strncpy(m_contextName, securityContext->getContextName(), SNMP_MAX_CONTEXT_NAME);
		m_contextName[SNMP_MAX_CONTEXT_NAME - 1] = 0;
	}

   // Estimate required buffer size and allocate it
   for(dwBufferSize = 1024, i = 0; i < m_variables->size(); i++)
   {
      SNMP_Variable *var = m_variables->get(i);
      dwBufferSize += var->getValueLength() + var->getName()->getLength() * 4 + 16;
   }
   pBlock = (BYTE *)malloc(dwBufferSize);
   pVarBinds = (BYTE *)malloc(dwBufferSize);
   pPacket = (BYTE *)malloc(dwBufferSize);

   // Encode variables
   for(i = 0, dwVarBindsSize = 0, pbCurrPos = pVarBinds; i < m_variables->size(); i++)
   {
      SNMP_Variable *var = m_variables->get(i);
      dwBytes = var->encode(pbCurrPos, dwBufferSize - dwVarBindsSize);
      pbCurrPos += dwBytes;
      dwVarBindsSize += dwBytes;
   }

   // Determine PDU type
   for(i = 0; PDU_type_to_command[i].dwType != 0; i++)
      if (((m_version == (UINT32)PDU_type_to_command[i].iVersion) ||
           (PDU_type_to_command[i].iVersion == -1)) &&
          (PDU_type_to_command[i].dwCommand == m_command))
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

            dwValue = (UINT32)m_trapType;
            dwBytes = BER_Encode(ASN_INTEGER, (BYTE *)&dwValue, sizeof(UINT32), 
                                 pbCurrPos, dwBufferSize - dwPDUSize);
            dwPDUSize += dwBytes;
            pbCurrPos += dwBytes;

            dwValue = (UINT32)m_specificTrap;
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
		if ((m_version != SNMP_VERSION_3) || ((securityContext != NULL) && (securityContext->getAuthoritativeEngine().getIdLen() != 0)))
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

      dwBytes = BER_Encode(ASN_INTEGER, (BYTE *)&m_version, sizeof(UINT32), 
                           pbCurrPos, dwBufferSize);
      dwPacketSize += dwBytes;
      pbCurrPos += dwBytes;

		if (m_version == SNMP_VERSION_3)
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
					size_t encSize = (dwBytes % 8 == 0) ? dwBytes : (dwBytes + (8 - (dwBytes % 8)));
					BYTE *encryptedPdu = (BYTE *)malloc(encSize);

					DES_cblock key;
					DES_key_schedule schedule;
					memcpy(&key, securityContext->getPrivKey(), 8);
					DES_set_key_unchecked(&key, &schedule);

					DES_cblock iv;
					memcpy(&iv, securityContext->getPrivKey() + 8, 8);
					for(int i = 0; i < 8; i++)
						iv[i] ^= m_salt[i];

					DES_ncbc_encrypt(pbCurrPos, encryptedPdu, (long)dwBytes, &schedule, &iv, DES_ENCRYPT);
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
		if ((m_version == SNMP_VERSION_3) && securityContext->needAuthentication())
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
size_t SNMP_PDU::encodeV3Header(BYTE *buffer, size_t bufferSize, SNMP_SecurityContext *securityContext)
{
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

	BYTE header[256];
	size_t bytes = BER_Encode(ASN_INTEGER, (BYTE *)&m_msgId, sizeof(UINT32), header, 256);
	bytes += BER_Encode(ASN_INTEGER, (BYTE *)&m_msgMaxSize, sizeof(UINT32), &header[bytes], 256 - bytes);
	bytes += BER_Encode(ASN_OCTET_STRING, &flags, 1, &header[bytes], 256 - bytes);
   UINT32 securityModel = securityContext->getSecurityModel();
	bytes += BER_Encode(ASN_INTEGER, (BYTE *)&securityModel, sizeof(UINT32), &header[bytes], 256 - bytes);
	return BER_Encode(ASN_SEQUENCE, header, bytes, buffer, bufferSize);
}

/**
 * Encode version 3 security parameters
 */
size_t SNMP_PDU::encodeV3SecurityParameters(BYTE *buffer, size_t bufferSize, SNMP_SecurityContext *securityContext)
{
	size_t bytes;

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
size_t SNMP_PDU::encodeV3ScopedPDU(UINT32 pduType, BYTE *pdu, size_t pduSize, BYTE *buffer, size_t bufferSize)
{
	size_t spduLen = pduSize + SNMP_MAX_CONTEXT_NAME + SNMP_MAX_ENGINEID_LEN + 32;
	BYTE *spdu = (BYTE *)malloc(spduLen);
	size_t bytes;

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
void SNMP_PDU::signMessage(BYTE *msg, size_t msgLen, SNMP_SecurityContext *securityContext)
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
   m_variables->add(pVar);
}

/**
 * Set context engine ID
 */
void SNMP_PDU::setContextEngineId(BYTE *id, size_t len)
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
