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
** File: security.cpp
**
**/

#include "libnxsnmp.h"

/**
 * Default constructor for SNMP_SecurityContext
 */
SNMP_SecurityContext::SNMP_SecurityContext()
{
	m_securityModel = 0;
	m_authName = NULL;
	m_authPassword = NULL;
	m_privPassword = NULL;
	m_contextName = NULL;
	m_authMethod = SNMP_AUTH_NONE;
	m_privMethod = SNMP_ENCRYPT_NONE;
	memset(m_authKeyMD5, 0, 16);
	memset(m_authKeySHA1, 0, 20);
	memset(m_privKey, 0, 20);
}

/**
 * Create copy of given security context
 */
SNMP_SecurityContext::SNMP_SecurityContext(SNMP_SecurityContext *src)
{
	m_securityModel = src->m_securityModel;
	m_authName = (src->m_authName != NULL) ? strdup(src->m_authName) : NULL;
	m_authPassword = (src->m_authPassword != NULL) ? strdup(src->m_authPassword) : NULL;
	m_privPassword = (src->m_privPassword != NULL) ? strdup(src->m_privPassword) : NULL;
	m_contextName = (src->m_contextName != NULL) ? strdup(src->m_contextName) : NULL;
	m_authMethod = src->m_authMethod;
	m_privMethod = src->m_privMethod;
	memcpy(m_authKeyMD5, src->m_authKeyMD5, 16);
	memcpy(m_authKeySHA1, src->m_authKeySHA1, 20);
	memcpy(m_privKey, src->m_privKey, 20);
	m_authoritativeEngine = src->m_authoritativeEngine;
}

/**
 * Create V2C security context
 */
SNMP_SecurityContext::SNMP_SecurityContext(const char *community)
{
	m_securityModel = SNMP_SECURITY_MODEL_V2C;
	m_authName = strdup(CHECK_NULL_EX_A(community));
	m_authPassword = NULL;
	m_privPassword = NULL;
	m_contextName = NULL;
	m_authMethod = SNMP_AUTH_NONE;
	m_privMethod = SNMP_ENCRYPT_NONE;
	memset(m_authKeyMD5, 0, 16);
	memset(m_authKeySHA1, 0, 20);
	memset(m_privKey, 0, 20);
}

/**
 * Create authNoPriv V3 security context
 */
SNMP_SecurityContext::SNMP_SecurityContext(const char *user, const char *authPassword, int authMethod)
{
	m_securityModel = SNMP_SECURITY_MODEL_USM;
	m_authName = strdup(CHECK_NULL_EX_A(user));
	m_authPassword = strdup(CHECK_NULL_EX_A(authPassword));
	m_privPassword = NULL;
	m_contextName = NULL;
	m_authMethod = authMethod;
	m_privMethod = SNMP_ENCRYPT_NONE;
	recalculateKeys();
}

/**
 * Create authPriv V3 security context
 */
SNMP_SecurityContext::SNMP_SecurityContext(const char *user, const char *authPassword, const char *encryptionPassword,
														 int authMethod, int encryptionMethod)
{
	m_securityModel = SNMP_SECURITY_MODEL_USM;
	m_authName = strdup(CHECK_NULL_EX_A(user));
	m_authPassword = strdup(CHECK_NULL_EX_A(authPassword));
	m_privPassword = strdup(CHECK_NULL_EX_A(encryptionPassword));
	m_contextName = NULL;
	m_authMethod = authMethod;
	m_privMethod = encryptionMethod;
	recalculateKeys();
}

/**
 * Destructor for security context
 */
SNMP_SecurityContext::~SNMP_SecurityContext()
{
	safe_free(m_authName);
	safe_free(m_authPassword);
	safe_free(m_privPassword);
	safe_free(m_contextName);
}

/**
 * Set context name
 */
void SNMP_SecurityContext::setContextName(const TCHAR *name)
{
	safe_free(m_contextName);
	if (name != NULL)
	{
#ifdef UNICODE
		m_contextName = MBStringFromWideString(name);
#else
		m_contextName = strdup(name);
#endif
	}
	else
	{
		m_contextName = NULL;
	}
}

#ifdef UNICODE

/**
 * Set context name (multibyte string version)
 */
void SNMP_SecurityContext::setContextNameA(const char *name)
{
	safe_free(m_contextName);
	m_contextName = (name != NULL) ? strdup(name) : NULL;
}

#endif

/**
 * Set authentication name
 */
void SNMP_SecurityContext::setAuthName(const char *name)
{
	safe_free(m_authName);
	m_authName = strdup(CHECK_NULL_EX_A(name));
}

/**
 * Set authentication password
 */
void SNMP_SecurityContext::setAuthPassword(const char *password)
{
	safe_free(m_authPassword);
	m_authPassword = strdup(CHECK_NULL_EX_A(password));
	recalculateKeys();
}

/**
 * Set encryption password
 */
void SNMP_SecurityContext::setPrivPassword(const char *password)
{
	safe_free(m_privPassword);
	m_privPassword = strdup(CHECK_NULL_EX_A(password));
	recalculateKeys();
}

/**
 * Set authoritative engine ID
 */
void SNMP_SecurityContext::setAuthoritativeEngine(SNMP_Engine &engine)
{
	m_authoritativeEngine = engine;
	recalculateKeys();
}

/**
 * Recalculate keys
 */
void SNMP_SecurityContext::recalculateKeys()
{
	BYTE buffer[256];
	const char *authPassword = (m_authPassword != NULL) ? m_authPassword : "";
	const char *privPassword = (m_privPassword != NULL) ? m_privPassword : "";

	// MD5 auth key
	MD5HashForPattern((const BYTE *)authPassword, strlen(authPassword), 1048576, buffer);
	memcpy(&buffer[16], m_authoritativeEngine.getId(), m_authoritativeEngine.getIdLen());
	memcpy(&buffer[16 + m_authoritativeEngine.getIdLen()], buffer, 16);
	CalculateMD5Hash(buffer, m_authoritativeEngine.getIdLen() + 32, m_authKeyMD5);

	// SHA1 auth key
	SHA1HashForPattern((BYTE *)authPassword, strlen(authPassword), 1048576, buffer);
	memcpy(&buffer[20], m_authoritativeEngine.getId(), m_authoritativeEngine.getIdLen());
	memcpy(&buffer[20 + m_authoritativeEngine.getIdLen()], buffer, 20);
	CalculateSHA1Hash(buffer, m_authoritativeEngine.getIdLen() + 40, m_authKeySHA1);

	// Priv key
	if (m_authMethod == SNMP_AUTH_MD5)
	{
		MD5HashForPattern((const BYTE *)privPassword, strlen(privPassword), 1048576, buffer);
		memcpy(&buffer[16], m_authoritativeEngine.getId(), m_authoritativeEngine.getIdLen());
		memcpy(&buffer[16 + m_authoritativeEngine.getIdLen()], buffer, 16);
		CalculateMD5Hash(buffer, m_authoritativeEngine.getIdLen() + 32, m_privKey);
	}
	else
	{
		SHA1HashForPattern((BYTE *)privPassword, strlen(privPassword), 1048576, buffer);
		memcpy(&buffer[20], m_authoritativeEngine.getId(), m_authoritativeEngine.getIdLen());
		memcpy(&buffer[20 + m_authoritativeEngine.getIdLen()], buffer, 20);
		CalculateSHA1Hash(buffer, m_authoritativeEngine.getIdLen() + 40, m_privKey);
	}
}
