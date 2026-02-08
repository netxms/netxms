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
** File: pdu.cpp
**
**/

#include "libnxsnmp.h"
#include <nxcrypto.h>

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
static BYTE s_hashPlaceholder[48] =
{
   0xAB, 0xCD, 0xEF, 0xFF, 0x00, 0x11, 0x22, 0x33, 0xBB, 0xAA, 0x99, 0x88,
   0xAB, 0xCD, 0xEF, 0xFF, 0x00, 0x11, 0x22, 0x33, 0xBB, 0xAA, 0x99, 0x88,
   0xAB, 0xCD, 0xEF, 0xFF, 0x00, 0x11, 0x22, 0x33, 0xBB, 0xAA, 0x99, 0x88,
   0xAB, 0xCD, 0xEF, 0xFF, 0x00, 0x11, 0x22, 0x33, 0xBB, 0xAA, 0x99, 0x88
};

/**
 * PDU type to command translation
 */
static struct
{
   UINT32 dwType;
   int iVersion;
   UINT32 dwCommand;
} s_pduTypeToCommand[] = 
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
SNMP_PDU::SNMP_PDU() : m_variables(0, 16, Ownership::True)
{
   m_version = SNMP_VERSION_1;
   m_command = SNMP_INVALID_PDU;
   m_errorCode = SNMP_PDU_ERR_SUCCESS;
   m_errorIndex = 0;
   m_requestId = 0;
	m_msgId = 0;
	m_flags = 0;
   m_trapType = 0;
   m_specificTrap = 0;
	m_contextEngineIdLen = 0;
	m_contextName[0] = 0;
	m_msgMaxSize = SNMP_DEFAULT_MSG_MAX_SIZE;
	m_authObject = nullptr;
	m_reportable = true;
   m_securityModel = SNMP_SECURITY_MODEL_V1;
	m_dwAgentAddr = 0;
	m_timestamp = 0;
	m_signatureOffset = 0;
}

/**
 * Create request PDU
 */
SNMP_PDU::SNMP_PDU(SNMP_Command command, uint32_t requestId, SNMP_Version version) : m_variables(0, 16, Ownership::True)
{
   m_version = version;
   m_command = command;
   m_errorCode = SNMP_PDU_ERR_SUCCESS;
   m_errorIndex = 0;
   m_requestId = requestId;
	m_msgId = requestId;
	m_flags = 0;
   m_trapType = 0;
   m_specificTrap = 0;
	m_contextEngineIdLen = 0;
	m_contextName[0] = 0;
	m_msgMaxSize = SNMP_DEFAULT_MSG_MAX_SIZE;
	m_authObject = nullptr;
	m_reportable = true;
   m_securityModel = (m_version == SNMP_VERSION_1) ? SNMP_SECURITY_MODEL_V1 : ((m_version == SNMP_VERSION_2C) ? SNMP_SECURITY_MODEL_V2C : SNMP_SECURITY_MODEL_USM);
   m_dwAgentAddr = 0;
   m_timestamp = 0;
   m_signatureOffset = 0;
}

/**
 * Create trap or inform request PDU
 */
SNMP_PDU::SNMP_PDU(SNMP_Command command, SNMP_Version version, const SNMP_ObjectId& trapId, uint32_t sysUpTime, uint32_t requestId) : m_variables(16, 16, Ownership::True)
{
   m_version = version;
   m_command = command;
   m_errorCode = SNMP_PDU_ERR_SUCCESS;
   m_errorIndex = 0;
   m_requestId = requestId;
   m_msgId = requestId;
   m_flags = 0;
   m_contextEngineIdLen = 0;
   m_contextName[0] = 0;
   m_msgMaxSize = SNMP_DEFAULT_MSG_MAX_SIZE;
   m_authObject = nullptr;
   m_reportable = true;
   m_securityModel = (m_version == SNMP_VERSION_1) ? SNMP_SECURITY_MODEL_V1 : ((m_version == SNMP_VERSION_2C) ? SNMP_SECURITY_MODEL_V2C : SNMP_SECURITY_MODEL_USM);
   m_dwAgentAddr = 0;
   m_timestamp = 0;
   m_signatureOffset = 0;

   setTrapId(trapId);
   if (version != SNMP_VERSION_1)
   {
      // V2 TRAP PDU - add uptime and OID varbinds
      SNMP_Variable *v = new SNMP_Variable({ 1, 3, 6, 1, 2, 1, 1, 3, 0 });
      v->setValueFromUInt32(ASN_TIMETICKS, sysUpTime);
      m_variables.add(v);

      v = new SNMP_Variable({ 1, 3, 6, 1, 6, 3, 1, 1, 4, 1, 0 });
      v->setValueFromObjectId(ASN_OBJECT_ID, trapId);
      m_variables.add(v);
   }
}

/**
 * Copy constructor
 */
SNMP_PDU::SNMP_PDU(const SNMP_PDU& src) :  m_variables(src.m_variables.size(), 16, Ownership::True), m_trapId(src.m_trapId), m_codepage(src.m_codepage), m_authoritativeEngine(src.m_authoritativeEngine)
{
   m_version = src.m_version;
   m_command = src.m_command;
   m_errorCode = src.m_errorCode;
   m_errorIndex = src.m_errorIndex;
   m_requestId = src.m_requestId;
	m_msgId = src.m_msgId;
	m_flags = src.m_flags;
   m_trapType = src.m_trapType;
   m_specificTrap = src.m_specificTrap;
	m_contextEngineIdLen = src.m_contextEngineIdLen;
   memcpy(m_contextEngineId, src.m_contextEngineId, SNMP_MAX_ENGINEID_LEN);
	strcpy(m_contextName, src.m_contextName);
	m_msgMaxSize = src.m_msgMaxSize;
   m_authObject = MemCopyStringA(src.m_authObject);
	m_reportable = src.m_reportable;
   m_securityModel = src.m_securityModel;
   m_dwAgentAddr = 0;
   m_timestamp = 0;
   m_signatureOffset = src.m_signatureOffset;

   for(int i = 0; i < src.m_variables.size(); i++)
      m_variables.add(new SNMP_Variable(*src.m_variables.get(i)));
}

/**
 * SNMP_PDU destructor
 */
SNMP_PDU::~SNMP_PDU()
{
	MemFree(m_authObject);
}

/**
 * Parse single variable binding
 */
bool SNMP_PDU::parseVariable(const BYTE *data, size_t varLength)
{
   bool success;
   auto var = new SNMP_Variable;
   if (var->decode(data, varLength))
   {
      bindVariable(var);
      success = true;
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
bool SNMP_PDU::parseVarBinds(const BYTE *pduData, size_t pduLength)
{
   const BYTE *currPos;
   uint32_t dataType;
   size_t length, bindingLength, idLength;

   // Varbind section should be a SEQUENCE
   if (!BER_DecodeIdentifier(pduData, pduLength, &dataType, &bindingLength, &currPos, &idLength))
      return false;
   if (dataType != ASN_SEQUENCE)
      return false;

   while(bindingLength > 0)
   {
      if (!BER_DecodeIdentifier(currPos, pduLength, &dataType, &length, &currPos, &idLength))
         return false;
      if (dataType != ASN_SEQUENCE)
         return false;  // Every binding is a sequence
      if (length > bindingLength)
         return false;     // Invalid length

      if (!parseVariable(currPos, length))
         return false;
      bindingLength -= length + idLength;
      currPos += length;
   }

   return true;
}

/**
 * Parse generic PDU content
 */
bool SNMP_PDU::parsePduContent(const BYTE *pData, size_t pduLength)
{
   UINT32 dwType;
   size_t dwLength, idLength;
   const BYTE *pbCurrPos = pData;
   bool bResult = false;

   // Request ID
   if (BER_DecodeIdentifier(pbCurrPos, pduLength, &dwType, &dwLength, &pbCurrPos, &idLength))
   {
      if ((dwType == ASN_INTEGER) &&
          BER_DecodeContent(dwType, pbCurrPos, dwLength, (BYTE *)&m_requestId))
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
             BER_DecodeContent(dwType, pbCurrPos, dwLength, (BYTE *)&m_errorCode))
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
             BER_DecodeContent(dwType, pbCurrPos, dwLength, (BYTE *)&m_errorIndex))
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
bool SNMP_PDU::parseTrapPDU(const BYTE *pData, size_t pduLength)
{
   uint32_t dwType;
   size_t dwLength, idLength;
   const BYTE *pbCurrPos = pData;
   uint32_t dwBuffer;
   bool bResult = false;

   // Enterprise ID
   if (BER_DecodeIdentifier(pbCurrPos, pduLength, &dwType, &dwLength, &pbCurrPos, &idLength))
   {
      if (dwType == ASN_OBJECT_ID)
      {
         SNMP_OID oid;
         if (BER_DecodeContent(dwType, pbCurrPos, dwLength, (BYTE *)&oid))
         {
            m_trapId.setValue(oid.value, oid.length);
            pduLength -= dwLength + idLength;
            pbCurrPos += dwLength;
            bResult = true;
            if (oid.value != oid.internalBuffer)
            {
               MemFree(oid.value);
            }
         }
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
             BER_DecodeContent(dwType, pbCurrPos, dwLength, (BYTE *)&m_timestamp))
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
         static uint32_t pdwStdOid[6][10] =
         {
            { 1, 3, 6, 1, 6, 3, 1, 1, 5, 1 },   // cold start
            { 1, 3, 6, 1, 6, 3, 1, 1, 5, 2 },   // warm start
            { 1, 3, 6, 1, 6, 3, 1, 1, 5, 3 },   // link down
            { 1, 3, 6, 1, 6, 3, 1, 1, 5, 4 },   // link up
            { 1, 3, 6, 1, 6, 3, 1, 1, 5, 5 },   // authentication failure
            { 1, 3, 6, 1, 6, 3, 1, 1, 5, 6 }    // EGP neighbor loss (obsolete)
         };

         // For standard trap types, create standard V2 Enterprise ID
         m_trapId.setValue(pdwStdOid[m_trapType], 10);
      }
      else
      {
         m_trapId.extend(0);
         m_trapId.extend(m_specificTrap);
      }
   }

   return bResult;
}

/**
 * Parse version 2 TRAP or INFORM-REQUEST PDU
 */
bool SNMP_PDU::parseTrap2PDU(const BYTE *pData, size_t pduLength)
{
   bool bResult;

   bResult = parsePduContent(pData, pduLength);
   if (bResult)
   {
      bResult = false;
      if (m_variables.size() >= 2)
      {
         SNMP_Variable *var = m_variables.get(1);
         if (var->getType() == ASN_OBJECT_ID)
         {
            setTrapId(reinterpret_cast<const uint32_t*>(var->getValue()), var->getValueLength() / sizeof(uint32_t));
            bResult = true;
         }
      }
   }
   return bResult;
}

/**
 * Parse version 3 header
 */
bool SNMP_PDU::parseV3Header(const BYTE *header, size_t headerLength)
{
	UINT32 type;
   size_t length, idLength, remLength = headerLength;
	const BYTE *currPos = header;

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
	uint32_t securityModel;
   if (!BER_DecodeContent(type, currPos, length, (BYTE *)&securityModel))
      return false;   // Error parsing content
	m_securityModel = static_cast<SNMP_SecurityModel>(securityModel);

	return true;
}

/**
 * Parse V3 USM security parameters
 */
bool SNMP_PDU::parseV3SecurityUsm(const BYTE *data, size_t dataLength, const BYTE *rawMsg)
{
	uint32_t type;
   size_t length, idLength, engineIdLen, remLength = dataLength;
   uint32_t engineBoots, engineTime;
	const BYTE *currPos = data;
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
	m_authObject = MemAllocStringA(length + 1);
	if (!BER_DecodeContent(type, currPos, length, (BYTE *)m_authObject))
	{
		MemFreeAndNull(m_authObject);
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
	memcpy(m_signature, currPos, std::min(length, sizeof(m_signature)));
	m_signatureOffset = currPos - rawMsg;
   currPos += length;
   remLength -= length + idLength;

	// Encryption salt
   if (!BER_DecodeIdentifier(currPos, remLength, &type, &length, &currPos, &idLength))
      return false;
   if (type != ASN_OCTET_STRING)
      return false;
	memcpy(m_salt, currPos, MIN(length, 8));

	return true;
}

/**
 * Parse V3 scoped PDU
 */
bool SNMP_PDU::parseV3ScopedPdu(const BYTE *data, size_t dataLength)
{
	UINT32 type;
   size_t length, idLength, remLength = dataLength;
	const BYTE *currPos = data;

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
bool SNMP_PDU::parsePdu(const BYTE *pdu, size_t pduLength)
{
	const BYTE *content;
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
 * Get AES key length depending on encryption method
 */
static inline int AESKeyLengthFromMethod(SNMP_EncryptionMethod method)
{
   switch (method)
   {
      case SNMP_ENCRYPT_AES_128:
         return 128;
      case SNMP_ENCRYPT_AES_192:
         return 192;
      case SNMP_ENCRYPT_AES_256:
         return 256;
      default:
         return 128;
   }
}

/**
 * Decrypt data in packet
 */
bool SNMP_PDU::decryptData(const BYTE *data, size_t length, BYTE *decryptedData, SNMP_SecurityContext *securityContext)
{
#ifdef _WITH_ENCRYPTION
	if (securityContext == nullptr)
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
	else if ((securityContext->getPrivMethod() == SNMP_ENCRYPT_AES_128) ||
	      (securityContext->getPrivMethod() == SNMP_ENCRYPT_AES_192) ||
	      (securityContext->getPrivMethod() == SNMP_ENCRYPT_AES_256))
	{
#ifndef OPENSSL_NO_AES
		AES_KEY key;
		AES_set_encrypt_key(securityContext->getPrivKey(), AESKeyLengthFromMethod(securityContext->getPrivMethod()), &key);

		BYTE iv[16];
		uint32_t boots, engineTime;
		// Use auth. engine from current PDU if possible
		if (m_authoritativeEngine.getIdLen() > 0)
		{
			boots = htonl(m_authoritativeEngine.getBoots());
			engineTime = htonl(m_authoritativeEngine.getTime());
		}
		else
		{
			boots = htonl(securityContext->getAuthoritativeEngine().getBoots());
			engineTime = htonl(securityContext->getAuthoritativeEngine().getAdjustedTime());
		}
		memcpy(iv, &boots, 4);
		memcpy(&iv[4], &engineTime, 4);
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
bool SNMP_PDU::parse(const BYTE *rawData, size_t rawLength, SNMP_SecurityContext *securityContext, bool engineIdAutoupdate)
{
   const BYTE *pbCurrPos;
   uint32_t type;
   size_t dwLength, dwPacketLength, idLength;
   bool bResult = false;

   // Packet start
   if (!BER_DecodeIdentifier(rawData, rawLength, &type, &dwPacketLength, &pbCurrPos, &idLength))
      return false;
   if (type != ASN_SEQUENCE)
      return false;   // Packet should start with SEQUENCE

   // Version
   if (!BER_DecodeIdentifier(pbCurrPos, dwPacketLength, &type, &dwLength, &pbCurrPos, &idLength))
      return false;
   if (type != ASN_INTEGER)
      return false;   // Version field should be of integer type
   uint32_t version;
   if (!BER_DecodeContent(type, pbCurrPos, dwLength, (BYTE *)&version))
      return false;   // Error parsing content of version field
   pbCurrPos += dwLength;
   dwPacketLength -= dwLength + idLength;
   if ((version != SNMP_VERSION_1) && (version != SNMP_VERSION_2C) && ((version != SNMP_VERSION_3)))
      return false;   // Unsupported SNMP version
   m_version = static_cast<SNMP_Version>(version);

	if (m_version == SNMP_VERSION_3)
	{
		// V3 header
		if (!BER_DecodeIdentifier(pbCurrPos, dwPacketLength, &type, &dwLength, &pbCurrPos, &idLength))
			return false;
		if (type != ASN_SEQUENCE)
			return false;   // Should be sequence
		
		// We don't need BER_DecodeContent because sequence does not need any special decoding
		if (!parseV3Header(pbCurrPos, dwLength))
			return false;
		pbCurrPos += dwLength;
		dwPacketLength -= dwLength + idLength;

		// Security parameters
		if (!BER_DecodeIdentifier(pbCurrPos, dwPacketLength, &type, &dwLength, &pbCurrPos, &idLength))
			return false;
		if (type != ASN_OCTET_STRING)
			return false;   // Should be octet string

		if (m_securityModel == SNMP_SECURITY_MODEL_USM)
		{
			if (!parseV3SecurityUsm(pbCurrPos, dwLength, rawData))
				return false;

			if (engineIdAutoupdate && (m_authoritativeEngine.getIdLen() > 0) && (securityContext != nullptr))
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
		BYTE *decryptedPdu = NULL;
		size_t decryptedPduLength = 0;
		if ((m_securityModel == SNMP_SECURITY_MODEL_USM) && (m_flags & SNMP_PRIV_FLAG))
		{
			if (!BER_DecodeIdentifier(pbCurrPos, dwPacketLength, &type, &dwLength, &pbCurrPos, &idLength))
				return false;
			if (type != ASN_OCTET_STRING)
				return false;   // Should be encoded as octet string

			decryptedPduLength = dwLength;
			decryptedPdu = static_cast<BYTE*>(SNMP_MemAlloc(decryptedPduLength));
			if (!decryptData(pbCurrPos, dwLength, decryptedPdu, securityContext))
			{
				SNMP_MemFree(decryptedPdu, decryptedPduLength);
				return false;
			}

			pbCurrPos = decryptedPdu;
		}

		// Scoped PDU
		if (!BER_DecodeIdentifier(pbCurrPos, dwPacketLength, &type, &dwLength, &pbCurrPos, &idLength))
		{
         SNMP_MemFree(decryptedPdu, decryptedPduLength);
			return false;
		}
		if (type != ASN_SEQUENCE)
		{
         SNMP_MemFree(decryptedPdu, decryptedPduLength);
			return false;   // Should be sequence
		}
		bResult = parseV3ScopedPdu(pbCurrPos, dwLength);
      SNMP_MemFree(decryptedPdu, decryptedPduLength);
	}
	else
	{
		// Community string
		if (!BER_DecodeIdentifier(pbCurrPos, dwPacketLength, &type, &dwLength, &pbCurrPos, &idLength))
			return false;
		if (type != ASN_OCTET_STRING)
			return false;   // Community field should be of string type
		m_authObject = MemAllocStringA(dwLength + 1);
		if (!BER_DecodeContent(type, pbCurrPos, dwLength, (BYTE *)m_authObject))
		{
			MemFreeAndNull(m_authObject);
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
size_t SNMP_PDU::encode(SNMP_PDUBuffer *outBuffer, SNMP_SecurityContext *securityContext)
{
   size_t bytes, varBindsSize, pduSize, packetSize;

	// Replace context name if defined in security context
	if (securityContext->getContextName() != nullptr)
		strlcpy(m_contextName, securityContext->getContextName(), SNMP_MAX_CONTEXT_NAME);

   // Estimate required buffer size and allocate it
	size_t bufferSize = 1024;
   for(int i = 0; i < m_variables.size(); i++)
   {
      SNMP_Variable *var = m_variables.get(i);
      bufferSize += var->getValueLength() + var->getName().length() * 4 + 16;
   }
   BYTE *block = static_cast<BYTE*>(SNMP_MemAlloc(bufferSize));
   BYTE *varBinds = static_cast<BYTE*>(SNMP_MemAlloc(bufferSize));
   BYTE *packet = static_cast<BYTE*>(SNMP_MemAlloc(bufferSize));

   // Encode variables
   varBindsSize = 0;
   BYTE *currPos = varBinds;
   for(int i = 0; i < m_variables.size(); i++)
   {
      SNMP_Variable *var = m_variables.get(i);
      bytes = var->encode(currPos, bufferSize - varBindsSize);
      currPos += bytes;
      varBindsSize += bytes;
   }

   // Determine PDU type
   uint32_t pduType = 0;
   for(int i = 0; s_pduTypeToCommand[i].dwType != 0; i++)
      if (((m_version == static_cast<SNMP_Version>(s_pduTypeToCommand[i].iVersion)) ||
           (s_pduTypeToCommand[i].iVersion == -1)) &&
          (s_pduTypeToCommand[i].dwCommand == m_command))
      {
         pduType = s_pduTypeToCommand[i].dwType;
         break;
      }

   // Encode PDU header
   if (pduType != 0)
   {
      currPos = block;
      pduSize = 0;
      uint32_t intValue;
      switch(pduType)
      {
         case ASN_TRAP_V1_PDU:
            bytes = BER_Encode(ASN_OBJECT_ID, reinterpret_cast<const BYTE*>(m_trapId.value()),
                                 m_trapId.length() * sizeof(uint32_t),
                                 currPos, bufferSize - pduSize);
            pduSize += bytes;
            currPos += bytes;

            bytes = BER_Encode(ASN_IP_ADDR, (BYTE *)&m_dwAgentAddr, sizeof(uint32_t),
                                 currPos, bufferSize - pduSize);
            pduSize += bytes;
            currPos += bytes;

            intValue = static_cast<uint32_t>(m_trapType);
            bytes = BER_Encode(ASN_INTEGER, (BYTE *)&intValue, sizeof(uint32_t),
                                 currPos, bufferSize - pduSize);
            pduSize += bytes;
            currPos += bytes;

            intValue = static_cast<uint32_t>(m_specificTrap);
            bytes = BER_Encode(ASN_INTEGER, (BYTE *)&intValue, sizeof(uint32_t),
                                 currPos, bufferSize - pduSize);
            pduSize += bytes;
            currPos += bytes;

            bytes = BER_Encode(ASN_INTEGER, (BYTE *)&m_timestamp, sizeof(uint32_t),
                                 currPos, bufferSize - pduSize);
            pduSize += bytes;
            currPos += bytes;
            break;
         default:
            bytes = BER_Encode(ASN_INTEGER, (BYTE *)&m_requestId, sizeof(uint32_t),
                                 currPos, bufferSize - pduSize);
            pduSize += bytes;
            currPos += bytes;

            bytes = BER_Encode(ASN_INTEGER, (BYTE *)&m_errorCode, sizeof(uint32_t),
                                 currPos, bufferSize - pduSize);
            pduSize += bytes;
            currPos += bytes;

            bytes = BER_Encode(ASN_INTEGER, (BYTE *)&m_errorIndex, sizeof(uint32_t),
                                 currPos, bufferSize - pduSize);
            pduSize += bytes;
            currPos += bytes;
            break;
      }

      // Encode varbinds into PDU
		if ((m_version != SNMP_VERSION_3) || ((securityContext != nullptr) && (securityContext->getAuthoritativeEngine().getIdLen() != 0)))
		{
			bytes = BER_Encode(ASN_SEQUENCE, varBinds, varBindsSize, currPos, bufferSize - pduSize);
		}
		else
		{
			// Do not encode varbinds into engine id discovery message
			bytes = BER_Encode(ASN_SEQUENCE, nullptr, 0, currPos, bufferSize - pduSize);
		}
      pduSize += bytes;

      // Encode packet header
      currPos = packet;
      packetSize = 0;

      uint32_t version = static_cast<uint32_t>(m_version);
      bytes = BER_Encode(ASN_INTEGER, (BYTE *)&version, sizeof(uint32_t), currPos, bufferSize);
      packetSize += bytes;
      currPos += bytes;

		if (m_version == SNMP_VERSION_3)
		{
			// Generate encryption salt if packet has to be encrypted
			if (securityContext->needEncryption())
			{
				uint64_t temp = htonq(GetCurrentTimeMs());
				memcpy(m_salt, &temp, 8);
			}

			bytes = encodeV3Header(currPos, bufferSize - packetSize, securityContext);
			packetSize += bytes;
			currPos += bytes;

			bytes = encodeV3SecurityParameters(currPos, bufferSize - packetSize, securityContext);
			packetSize += bytes;
			currPos += bytes;

			bytes = encodeV3ScopedPDU(pduType, block, pduSize, currPos, bufferSize - packetSize);
			if (securityContext->needEncryption())
			{
#ifdef _WITH_ENCRYPTION
				if (securityContext->getPrivMethod() == SNMP_ENCRYPT_DES)
				{
#ifndef OPENSSL_NO_DES
					size_t encSize = (bytes % 8 == 0) ? bytes : (bytes + (8 - (bytes % 8)));
					BYTE *encryptedPdu = static_cast<BYTE*>(SNMP_MemAlloc(encSize));

					DES_cblock key;
					DES_key_schedule schedule;
					memcpy(&key, securityContext->getPrivKey(), 8);
					DES_set_key_unchecked(&key, &schedule);

					DES_cblock iv;
					memcpy(&iv, securityContext->getPrivKey() + 8, 8);
					for(int i = 0; i < 8; i++)
						iv[i] ^= m_salt[i];

					DES_ncbc_encrypt(currPos, encryptedPdu, (long)bytes, &schedule, &iv, DES_ENCRYPT);
					bytes = BER_Encode(ASN_OCTET_STRING, encryptedPdu, encSize, currPos, bufferSize - packetSize);
					SNMP_MemFree(encryptedPdu, encSize);
#else
					bytes = 0;	// Error - no DES support
					goto cleanup;
#endif
				}
				else if ((securityContext->getPrivMethod() == SNMP_ENCRYPT_AES_128) || (securityContext->getPrivMethod() == SNMP_ENCRYPT_AES_192) ||
			         (securityContext->getPrivMethod() == SNMP_ENCRYPT_AES_256))
				{
#ifndef OPENSSL_NO_AES
					AES_KEY key;
					AES_set_encrypt_key(securityContext->getPrivKey(), AESKeyLengthFromMethod(securityContext->getPrivMethod()), &key);

					BYTE iv[16];
					uint32_t boots = htonl(securityContext->getAuthoritativeEngine().getBoots());
					uint32_t engineTime = htonl(securityContext->getAuthoritativeEngine().getAdjustedTime());
					memcpy(iv, &boots, 4);
					memcpy(&iv[4], &engineTime, 4);
					memcpy(&iv[8], m_salt, 8);

					BYTE *encryptedPdu = static_cast<BYTE*>(SNMP_MemAlloc(bytes));
					int num = 0;
					AES_cfb128_encrypt(currPos, encryptedPdu, bytes, &key, iv, &num, AES_ENCRYPT);
					bytes = BER_Encode(ASN_OCTET_STRING, encryptedPdu, bytes, currPos, bufferSize - packetSize);
					SNMP_MemFree(encryptedPdu, bytes);
#else
					bytes = 0;	// Error - no AES support
					goto cleanup;
#endif
				}
				else
				{
					bytes = 0;	// Error - unsupported method
					goto cleanup;
				}
#else
				bytes = 0;	// Error
				goto cleanup;
#endif
			}
			packetSize += bytes;
		}
		else
		{
			bytes = BER_Encode(ASN_OCTET_STRING, (BYTE *)securityContext->getCommunity(),
			         strlen(securityContext->getCommunity()), currPos, bufferSize - packetSize);
			packetSize += bytes;
			currPos += bytes;

			// Encode PDU into packet
			bytes = BER_Encode(pduType, block, pduSize, currPos, bufferSize - packetSize);
			packetSize += bytes;
		}

      // And final step: allocate buffer for entire datagramm and wrap packet into SEQUENCE
		outBuffer->realloc(packetSize + 6);
      bytes = BER_Encode(ASN_SEQUENCE, packet, packetSize, outBuffer->buffer(), packetSize + 6);

		// Sign message
		if ((m_version == SNMP_VERSION_3) && securityContext->needAuthentication())
		{
			signMessage(outBuffer->buffer(), bytes, securityContext);
		}
   }
   else
   {
      bytes = 0;   // Error
   }

cleanup:
   SNMP_MemFree(packet, bufferSize);
   SNMP_MemFree(block, bufferSize);
   SNMP_MemFree(varBinds, bufferSize);
   return bytes;
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
	size_t bytes = BER_Encode(ASN_INTEGER, (BYTE *)&m_msgId, sizeof(uint32_t), header, 256);
	bytes += BER_Encode(ASN_INTEGER, (BYTE *)&m_msgMaxSize, sizeof(uint32_t), &header[bytes], 256 - bytes);
	bytes += BER_Encode(ASN_OCTET_STRING, &flags, 1, &header[bytes], 256 - bytes);
	uint32_t securityModel = static_cast<uint32_t>(securityContext->getSecurityModel());
	bytes += BER_Encode(ASN_INTEGER, (BYTE *)&securityModel, sizeof(uint32_t), &header[bytes], 256 - bytes);
	return BER_Encode(ASN_SEQUENCE, header, bytes, buffer, bufferSize);
}

/**
 * Encode version 3 security parameters
 */
size_t SNMP_PDU::encodeV3SecurityParameters(BYTE *buffer, size_t bufferSize, SNMP_SecurityContext *securityContext)
{
	size_t bytes;

	if ((securityContext != nullptr) && (securityContext->getSecurityModel() == SNMP_SECURITY_MODEL_USM))
	{
   	BYTE securityParameters[1024], sequence[1040];

   	uint32_t engineBoots = securityContext->getAuthoritativeEngine().getBoots();
   	uint32_t engineTime = securityContext->getAuthoritativeEngine().getAdjustedTime();

		bytes = BER_Encode(ASN_OCTET_STRING, securityContext->getAuthoritativeEngine().getId(),
		         securityContext->getAuthoritativeEngine().getIdLen(), securityParameters, 1024);
		bytes += BER_Encode(ASN_INTEGER, (BYTE *)&engineBoots, sizeof(uint32_t), &securityParameters[bytes], 1024 - bytes);
		bytes += BER_Encode(ASN_INTEGER, (BYTE *)&engineTime, sizeof(uint32_t), &securityParameters[bytes], 1024 - bytes);

		// Don't send user and auth/priv parameters in engine id discovery message
		if (securityContext->getAuthoritativeEngine().getIdLen() != 0)
		{
			bytes += BER_Encode(ASN_OCTET_STRING, (BYTE *)securityContext->getUserName(), strlen(securityContext->getUserName()), &securityParameters[bytes], 1024 - bytes);

			// Authentication parameters
			if (securityContext->needAuthentication())
			{
				// Add placeholder for message hash
				bytes += BER_Encode(ASN_OCTET_STRING, s_hashPlaceholder, securityContext->getSignatureSize(), &securityParameters[bytes], 1024 - bytes);
			}
			else
			{
				bytes += BER_Encode(ASN_OCTET_STRING, nullptr, 0, &securityParameters[bytes], 1024 - bytes);
			}

			// Privacy parameters
			if (securityContext->needEncryption())
			{
				bytes += BER_Encode(ASN_OCTET_STRING, m_salt, 8, &securityParameters[bytes], 1024 - bytes);
			}
			else
			{
				bytes += BER_Encode(ASN_OCTET_STRING, nullptr, 0, &securityParameters[bytes], 1024 - bytes);
			}
		}
		else
		{
			// Empty strings in place of user name, auth parameters and privacy parameters
			bytes += BER_Encode(ASN_OCTET_STRING, nullptr, 0, &securityParameters[bytes], 1024 - bytes);
			bytes += BER_Encode(ASN_OCTET_STRING, nullptr, 0, &securityParameters[bytes], 1024 - bytes);
			bytes += BER_Encode(ASN_OCTET_STRING, nullptr, 0, &securityParameters[bytes], 1024 - bytes);
		}

		// Wrap into sequence
		bytes = BER_Encode(ASN_SEQUENCE, securityParameters, bytes, sequence, 1040);

		// Wrap sequence into octet string
		bytes = BER_Encode(ASN_OCTET_STRING, sequence, bytes, buffer, bufferSize);
	}
	else
	{
		bytes = BER_Encode(ASN_OCTET_STRING, nullptr, 0, buffer, bufferSize);
	}
	return bytes;
}

/**
 * Encode versionj 3 scoped PDU
 */
size_t SNMP_PDU::encodeV3ScopedPDU(uint32_t pduType, BYTE *pdu, size_t pduSize, BYTE *buffer, size_t bufferSize)
{
	size_t spduLen = pduSize + SNMP_MAX_CONTEXT_NAME + SNMP_MAX_ENGINEID_LEN + 32;
	BYTE *spdu = static_cast<BYTE*>(SNMP_MemAlloc(spduLen));
	size_t bytes;

	bytes = BER_Encode(ASN_OCTET_STRING, m_contextEngineId, m_contextEngineIdLen, spdu, spduLen);
	bytes += BER_Encode(ASN_OCTET_STRING, (BYTE *)m_contextName, strlen(m_contextName), &spdu[bytes], spduLen - bytes);
	bytes += BER_Encode(pduType, pdu, pduSize, &spdu[bytes], spduLen - bytes);
	
	// Wrap scoped PDU into SEQUENCE
	bytes = BER_Encode(ASN_SEQUENCE, spdu, bytes, buffer, bufferSize);
	SNMP_MemFree(spdu, spduLen);
	return bytes;
}

/**
 * Message hash calculation for SNMP_PDU::signMessage and SNMP_PDU::validateSignedMessage using provided hash function
 */
template<typename C, void (*__Init)(C*), void (*__Update)(C*, const void*, size_t), void (*__Final)(C*, unsigned char*), size_t __hashSize, size_t __blockSize>
static inline void CalculateMessageHash(const BYTE *msg, size_t msgLen, size_t signatureOffset,
         size_t signatureSize, SNMP_SecurityContext *securityContext, BYTE *hash)
{
   static char zeroSignature[] = "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00";

   BYTE k1[__blockSize], k2[__blockSize];

   // Create K1 and K2
   memcpy(k1, securityContext->getAuthKey(), __hashSize);
   memset(&k1[__hashSize], 0, __blockSize - __hashSize);
   memcpy(k2, k1, __blockSize);
   for(size_t i = 0; i < __blockSize; i++)
   {
      k1[i] ^= 0x36;
      k2[i] ^= 0x5C;
   }

   // Calculate first hash (step 3)
   C context;
   __Init(&context);
   __Update(&context, k1, __blockSize);
   __Update(&context, msg, signatureOffset);
   __Update(&context, zeroSignature, signatureSize); // Replace signature with 0 to generate correct hash
   if (msgLen > signatureOffset + signatureSize)
      __Update(&context, msg + signatureOffset + signatureSize, msgLen - signatureOffset - signatureSize);
   __Final(&context, hash);

   // Calculate second hash
   __Init(&context);
   __Update(&context, k2, __blockSize);
   __Update(&context, hash, __hashSize);
   __Final(&context, hash);
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
   size_t signatureSize = securityContext->getSignatureSize();

	// Find placeholder for hash
   size_t hashPos;
	for(hashPos = 0; hashPos < msgLen - signatureSize; hashPos++)
		if (!memcmp(&msg[hashPos], s_hashPlaceholder, signatureSize))
			break;

	// Calculate message hash
	BYTE hash[64];
   switch(securityContext->getAuthMethod())
   {
      case SNMP_AUTH_MD5:
         CalculateMessageHash<MD5_STATE, MD5Init, MD5Update, MD5Final, MD5_DIGEST_SIZE, 64>(msg, msgLen, hashPos, signatureSize, securityContext, hash);
         break;
      case SNMP_AUTH_SHA1:
         CalculateMessageHash<SHA1_STATE, SHA1Init, SHA1Update, SHA1Final, SHA1_DIGEST_SIZE, 64>(msg, msgLen, hashPos, signatureSize, securityContext, hash);
         break;
      case SNMP_AUTH_SHA224:
         CalculateMessageHash<SHA224_STATE, SHA224Init, SHA224Update, SHA224Final, SHA224_DIGEST_SIZE, 64>(msg, msgLen, hashPos, signatureSize, securityContext, hash);
         break;
      case SNMP_AUTH_SHA256:
         CalculateMessageHash<SHA256_STATE, SHA256Init, SHA256Update, SHA256Final, SHA256_DIGEST_SIZE, 64>(msg, msgLen, hashPos, signatureSize, securityContext, hash);
         break;
      case SNMP_AUTH_SHA384:
         CalculateMessageHash<SHA384_STATE, SHA384Init, SHA384Update, SHA384Final, SHA384_DIGEST_SIZE, 128>(msg, msgLen, hashPos, signatureSize, securityContext, hash);
         break;
      case SNMP_AUTH_SHA512:
         CalculateMessageHash<SHA512_STATE, SHA512Init, SHA512Update, SHA512Final, SHA512_DIGEST_SIZE, 128>(msg, msgLen, hashPos, signatureSize, securityContext, hash);
         break;
      default:
         break;
   }

	// Update message hash
	memcpy(&msg[hashPos], hash, signatureSize);
}

/**
 * Validate V3 signed message
 */
bool SNMP_PDU::validateSignedMessage(const BYTE *msg, size_t msgLen, SNMP_SecurityContext *securityContext)
{
   if (securityContext == nullptr)
      return false;  // Unable to validate message without security context

   size_t signatureSize = securityContext->getSignatureSize();
   if (m_signatureOffset + signatureSize > msgLen)
      return false;  // malformed message

   // Calculate message hash
   BYTE hash[64];
   switch(securityContext->getAuthMethod())
   {
      case SNMP_AUTH_MD5:
         CalculateMessageHash<MD5_STATE, MD5Init, MD5Update, MD5Final, MD5_DIGEST_SIZE, 64>(msg, msgLen, m_signatureOffset, signatureSize, securityContext, hash);
         break;
      case SNMP_AUTH_SHA1:
         CalculateMessageHash<SHA1_STATE, SHA1Init, SHA1Update, SHA1Final, SHA1_DIGEST_SIZE, 64>(msg, msgLen, m_signatureOffset, signatureSize, securityContext, hash);
         break;
      case SNMP_AUTH_SHA224:
         CalculateMessageHash<SHA224_STATE, SHA224Init, SHA224Update, SHA224Final, SHA224_DIGEST_SIZE, 64>(msg, msgLen, m_signatureOffset, signatureSize, securityContext, hash);
         break;
      case SNMP_AUTH_SHA256:
         CalculateMessageHash<SHA256_STATE, SHA256Init, SHA256Update, SHA256Final, SHA256_DIGEST_SIZE, 64>(msg, msgLen, m_signatureOffset, signatureSize, securityContext, hash);
         break;
      case SNMP_AUTH_SHA384:
         CalculateMessageHash<SHA384_STATE, SHA384Init, SHA384Update, SHA384Final, SHA384_DIGEST_SIZE, 128>(msg, msgLen, m_signatureOffset, signatureSize, securityContext, hash);
         break;
      case SNMP_AUTH_SHA512:
         CalculateMessageHash<SHA512_STATE, SHA512Init, SHA512Update, SHA512Final, SHA512_DIGEST_SIZE, 128>(msg, msgLen, m_signatureOffset, signatureSize, securityContext, hash);
         break;
      default:
	 return false;
   }

   // Computed hash should match message signature
   return !memcmp(m_signature, hash, signatureSize);
}

/**
 * Set context engine ID
 */
void SNMP_PDU::setContextEngineId(const BYTE *id, size_t len)
{
	m_contextEngineIdLen = std::min(len, SNMP_MAX_ENGINEID_LEN);
	memcpy(m_contextEngineId, id, m_contextEngineIdLen);
}

/**
 * Set context engine ID
 */
void SNMP_PDU::setContextEngineId(const char *id)
{
	m_contextEngineIdLen = std::min(strlen(id), SNMP_MAX_ENGINEID_LEN);
	memcpy(m_contextEngineId, id, m_contextEngineIdLen);
}

/**
 * Set trap ID
 */
void SNMP_PDU::setTrapId(const uint32_t *value, size_t length)
{
   m_trapId.setValue(value, length);

   // Set V1 trap type and specific trap type fields
   static uint32_t standardTrapPrefix[9] = { 1, 3, 6, 1, 6, 3, 1, 1, 5 };
   if ((m_trapId.compare(standardTrapPrefix, 9) == OID_LONGER) && (m_trapId.length() == 10))
   {
      m_trapType = m_trapId.value()[9];
      m_specificTrap = 0;
   }
   else
   {
      m_trapType = 6;
      m_specificTrap = m_trapId.value()[m_trapId.length() - 1];
   }
}

/**
 * Set codepage for PDU
 */
void SNMP_PDU::setCodepage(const SNMP_Codepage& codepage)
{
   m_codepage = codepage;
   for(int i = 0; i < m_variables.size(); i++)
      m_variables.get(i)->setCodepage(m_codepage);
}

/**
 * Unlink variables
 */
void SNMP_PDU::unlinkVariables()
{
   m_variables.setOwner(Ownership::False);
   m_variables.clear();
   m_variables.setOwner(Ownership::True);
}
