/* 
** NetXMS - Network Management System
** SNMP support library
** Copyright (C) 2003-2023 Victor Kirhenshtein
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
** File: security.cpp
**
**/

#include "libnxsnmp.h"

/**
 * Default constructor for SNMP_SecurityContext
 */
SNMP_SecurityContext::SNMP_SecurityContext()
{
	m_securityModel = SNMP_SECURITY_MODEL_V2C;
	m_community = nullptr;
	m_userName = nullptr;
	m_authPassword = nullptr;
	m_privPassword = nullptr;
	m_contextName = nullptr;
	m_authMethod = SNMP_AUTH_NONE;
	m_privMethod = SNMP_ENCRYPT_NONE;
	memset(m_authKey, 0, sizeof(m_authKey));
	memset(m_privKey, 0, sizeof(m_privKey));
	m_validKeys = false;
}

/**
 * Create copy of given security context
 */
SNMP_SecurityContext::SNMP_SecurityContext(const SNMP_SecurityContext *src) : m_authoritativeEngine(src->m_authoritativeEngine), m_contextEngine(src->m_contextEngine)
{
	m_securityModel = src->m_securityModel;
	m_community = MemCopyStringA(src->m_community);
	m_userName = MemCopyStringA(src->m_userName);
	m_authPassword = MemCopyStringA(src->m_authPassword);
	m_privPassword = MemCopyStringA(src->m_privPassword);
	m_contextName = MemCopyStringA(src->m_contextName);
	m_authMethod = src->m_authMethod;
	m_privMethod = src->m_privMethod;
	memcpy(m_authKey, src->m_authKey, sizeof(m_authKey));
	memcpy(m_privKey, src->m_privKey, sizeof(m_privKey));
	m_validKeys = src->m_validKeys;
}

/**
 * Create V2C security context
 */
SNMP_SecurityContext::SNMP_SecurityContext(const char *community)
{
	m_securityModel = SNMP_SECURITY_MODEL_V2C;
	m_community = MemCopyStringA(CHECK_NULL_EX_A(community));
	m_userName = nullptr;
	m_authPassword = nullptr;
	m_privPassword = nullptr;
	m_contextName = nullptr;
	m_authMethod = SNMP_AUTH_NONE;
	m_privMethod = SNMP_ENCRYPT_NONE;
   memset(m_authKey, 0, sizeof(m_authKey));
   memset(m_privKey, 0, sizeof(m_privKey));
   m_validKeys = false;
}

/**
 * Create authNoPriv V3 security context
 */
SNMP_SecurityContext::SNMP_SecurityContext(const char *user, const char *authPassword, SNMP_AuthMethod authMethod)
{
	m_securityModel = SNMP_SECURITY_MODEL_USM;
	m_community = nullptr;
	m_userName = MemCopyStringA(CHECK_NULL_EX_A(user));
	m_authPassword = MemCopyStringA(CHECK_NULL_EX_A(authPassword));
	m_privPassword = nullptr;
	m_contextName = nullptr;
	m_authMethod = authMethod;
	m_privMethod = SNMP_ENCRYPT_NONE;
   memset(m_authKey, 0, sizeof(m_authKey));
   memset(m_privKey, 0, sizeof(m_privKey));
   m_validKeys = false;
}

/**
 * Create authPriv V3 security context
 */
SNMP_SecurityContext::SNMP_SecurityContext(const char *user, const char *authPassword, const char *encryptionPassword, SNMP_AuthMethod authMethod, SNMP_EncryptionMethod encryptionMethod)
{
	m_securityModel = SNMP_SECURITY_MODEL_USM;
   m_community = nullptr;
	m_userName = MemCopyStringA(CHECK_NULL_EX_A(user));
	m_authPassword = MemCopyStringA(CHECK_NULL_EX_A(authPassword));
	m_privPassword = MemCopyStringA(CHECK_NULL_EX_A(encryptionPassword));
	m_contextName = nullptr;
	m_authMethod = authMethod;
	m_privMethod = encryptionMethod;
   memset(m_authKey, 0, sizeof(m_authKey));
   memset(m_privKey, 0, sizeof(m_privKey));
   m_validKeys = false;
}

/**
 * Destructor for security context
 */
SNMP_SecurityContext::~SNMP_SecurityContext()
{
   MemFree(m_community);
	MemFree(m_userName);
	MemFree(m_authPassword);
	MemFree(m_privPassword);
	MemFree(m_contextName);
}

/**
 * Set security model
 */
void SNMP_SecurityContext::setSecurityModel(SNMP_SecurityModel model)
{
   if (m_securityModel == model)
      return;

   if (model == SNMP_SECURITY_MODEL_USM)
   {
      // switch to USM, use community as user name
      MemFree(m_userName);
      m_userName = MemCopyStringA(m_community);
      MemFreeAndNull(m_community);
   }
   else if (m_securityModel == SNMP_SECURITY_MODEL_USM)
   {
      // switch to community, use user name as community
      MemFree(m_community);
      m_community = MemCopyStringA(m_userName);
      MemFreeAndNull(m_userName);
   }
   m_securityModel = model;
   m_validKeys = false;
}

/**
 * Set authentication password
 */
void SNMP_SecurityContext::setAuthPassword(const char *password)
{
   if ((m_authPassword != nullptr) && !strcmp(CHECK_NULL_EX_A(password), m_authPassword))
      return;
	MemFree(m_authPassword);
	m_authPassword = MemCopyStringA(CHECK_NULL_EX_A(password));
   m_validKeys = false;
}

/**
 * Set encryption password
 */
void SNMP_SecurityContext::setPrivPassword(const char *password)
{
   if ((m_privPassword != nullptr) && !strcmp(CHECK_NULL_EX_A(password), m_privPassword))
      return;
	MemFree(m_privPassword);
	m_privPassword = MemCopyStringA(CHECK_NULL_EX_A(password));
   m_validKeys = false;
}

/**
 * Set authentication method
 */
void SNMP_SecurityContext::setAuthMethod(SNMP_AuthMethod method)
{
   if (m_authMethod == method)
      return;
   m_authMethod = method;
   m_validKeys = false;
}

/**
 * Set privacy method
 */
void SNMP_SecurityContext::setPrivMethod(SNMP_EncryptionMethod method)
{
   if (m_privMethod == method)
      return;
   m_privMethod = method;
   m_validKeys = false;
}

/**
 * Set authoritative engine ID
 */
void SNMP_SecurityContext::setAuthoritativeEngine(const SNMP_Engine &engine)
{
   if (m_authoritativeEngine.equals(engine))
   {
      // Do not invalidate keys if engine ID was not changed and only update boot count and time
      m_authoritativeEngine.setBootsAndTime(engine);
   }
   else
   {
      m_authoritativeEngine = engine;
      m_validKeys = false;
   }
}

/**
 * Generate user key from password using provided hash function
 */
template<void (*__HashForPattern)(const void*, size_t, size_t, BYTE*), void (*__Hash)(const void*, size_t, BYTE*), size_t __hashSize>
static inline void GenerateUserKey(const void *password, size_t passwordLen, const SNMP_Engine& authoritativeEngine, BYTE *key)
{
   BYTE buffer[1024];
   __HashForPattern(password, passwordLen, 1048576, buffer);
   memcpy(&buffer[__hashSize], authoritativeEngine.getId(), authoritativeEngine.getIdLen());
   memcpy(&buffer[__hashSize + authoritativeEngine.getIdLen()], buffer, __hashSize);
   __Hash(buffer, authoritativeEngine.getIdLen() + __hashSize * 2, key);
}

/**
 * Recalculate keys
 */
void SNMP_SecurityContext::recalculateKeys()
{
   if ((m_securityModel != SNMP_SECURITY_MODEL_USM) || m_validKeys)
      return;  // no need to recalculate keys

	const char *authPassword = (m_authPassword != nullptr) ? m_authPassword : "";
	const char *privPassword = (m_privPassword != nullptr) ? m_privPassword : "";

	switch(m_authMethod)
	{
	   case SNMP_AUTH_MD5:
	      GenerateUserKey<MD5HashForPattern, CalculateMD5Hash, MD5_DIGEST_SIZE>(authPassword, strlen(authPassword), m_authoritativeEngine, m_authKey);
         GenerateUserKey<MD5HashForPattern, CalculateMD5Hash, MD5_DIGEST_SIZE>(privPassword, strlen(privPassword), m_authoritativeEngine, m_privKey);
         if ((m_privMethod == SNMP_ENCRYPT_AES_192) || (m_privMethod== SNMP_ENCRYPT_AES_256))
         {
            GenerateUserKey<MD5HashForPattern, CalculateMD5Hash, MD5_DIGEST_SIZE>(m_privKey, 16, m_authoritativeEngine, m_privKey + 16);
         }
	      break;
      case SNMP_AUTH_SHA1:
         GenerateUserKey<SHA1HashForPattern, CalculateSHA1Hash, SHA1_DIGEST_SIZE>(authPassword, strlen(authPassword), m_authoritativeEngine, m_authKey);
         GenerateUserKey<SHA1HashForPattern, CalculateSHA1Hash, SHA1_DIGEST_SIZE>(privPassword, strlen(privPassword), m_authoritativeEngine, m_privKey);
         if ((m_privMethod == SNMP_ENCRYPT_AES_192) || (m_privMethod== SNMP_ENCRYPT_AES_256))
         {
            BYTE privKeyPart2[20];
            GenerateUserKey<MD5HashForPattern, CalculateSHA1Hash, MD5_DIGEST_SIZE>(m_privKey, 20, m_authoritativeEngine, privKeyPart2);
            memcpy(m_privKey + 20, privKeyPart2, 12);
         }
         break;
      case SNMP_AUTH_SHA224:
         GenerateUserKey<SHA224HashForPattern, CalculateSHA224Hash, SHA224_DIGEST_SIZE>(authPassword, strlen(authPassword), m_authoritativeEngine, m_authKey);
         GenerateUserKey<SHA224HashForPattern, CalculateSHA224Hash, SHA224_DIGEST_SIZE>(privPassword, strlen(privPassword), m_authoritativeEngine, m_privKey);
         if (m_privMethod== SNMP_ENCRYPT_AES_256)
         {
            BYTE privKeyPart2[28];
            GenerateUserKey<MD5HashForPattern, CalculateSHA224Hash, MD5_DIGEST_SIZE>(m_privKey, 20, m_authoritativeEngine, privKeyPart2);
            memcpy(m_privKey + 28, privKeyPart2, 4);
         }
         break;
      case SNMP_AUTH_SHA256:
         GenerateUserKey<SHA256HashForPattern, CalculateSHA256Hash, SHA256_DIGEST_SIZE>(authPassword, strlen(authPassword), m_authoritativeEngine, m_authKey);
         GenerateUserKey<SHA256HashForPattern, CalculateSHA256Hash, SHA256_DIGEST_SIZE>(privPassword, strlen(privPassword), m_authoritativeEngine, m_privKey);
         break;
      case SNMP_AUTH_SHA384:
         GenerateUserKey<SHA384HashForPattern, CalculateSHA384Hash, SHA384_DIGEST_SIZE>(authPassword, strlen(authPassword), m_authoritativeEngine, m_authKey);
         GenerateUserKey<SHA384HashForPattern, CalculateSHA384Hash, SHA384_DIGEST_SIZE>(privPassword, strlen(privPassword), m_authoritativeEngine, m_privKey);
         break;
      case SNMP_AUTH_SHA512:
         GenerateUserKey<SHA512HashForPattern, CalculateSHA512Hash, SHA512_DIGEST_SIZE>(authPassword, strlen(authPassword), m_authoritativeEngine, m_authKey);
         GenerateUserKey<SHA512HashForPattern, CalculateSHA512Hash, SHA512_DIGEST_SIZE>(privPassword, strlen(privPassword), m_authoritativeEngine, m_privKey);
         break;
      default:
         break;
	}

	m_validKeys = true;
}

/**
 * Serialize to JSON
 */
json_t *SNMP_SecurityContext::toJson() const
{
   json_t *root = json_object();
   json_object_set_new(root, "securityModel", json_integer(m_securityModel));
   json_object_set_new(root, "community", json_string_a(m_community));
   json_object_set_new(root, "userName", json_string_a(m_userName));
   json_object_set_new(root, "authPassword", json_string_a(m_authPassword));
   json_object_set_new(root, "privPassword", json_string_a(m_privPassword));
   json_object_set_new(root, "contextName", json_string_a(m_contextName));
   json_object_set_new(root, "authMethod", json_integer(m_authMethod));
   json_object_set_new(root, "privMethod", json_integer(m_privMethod));
   return root;
}
