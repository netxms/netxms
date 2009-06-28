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
** File: security.cpp
**
**/

#include "libnxsnmp.h"


//
// Constructors for SNMP_SecurityContext
//

SNMP_SecurityContext::SNMP_SecurityContext()
{
	m_securityModel = 0;
	m_community = NULL;
	m_user = NULL;
	m_authPassword = NULL;
	m_encryptionPassword = NULL;
}

SNMP_SecurityContext::SNMP_SecurityContext(int securityModel)
{
	m_securityModel = securityModel;
	m_community = NULL;
	m_user = NULL;
	m_authPassword = NULL;
	m_encryptionPassword = NULL;
}

SNMP_SecurityContext::SNMP_SecurityContext(const char *community)
{
	m_securityModel = SNMP_SECURITY_MODEL_V2C;
	m_community = strdup(CHECK_NULL_A(community));
	m_user = NULL;
	m_authPassword = NULL;
	m_encryptionPassword = NULL;
}

SNMP_SecurityContext::SNMP_SecurityContext(const char *user, const char *authPassword, const char *encryptionPassword)
{
	m_securityModel = SNMP_SECURITY_MODEL_USM;
	m_community = NULL;
	m_user = strdup(CHECK_NULL_A(user));
	m_authPassword = strdup(CHECK_NULL_A(authPassword));
	m_encryptionPassword = strdup(CHECK_NULL_A(encryptionPassword));
}


//
// Destructor for security context
//

SNMP_SecurityContext::~SNMP_SecurityContext()
{
	safe_free(m_community);
	safe_free(m_user);
	safe_free(m_authPassword);
	safe_free(m_encryptionPassword);
}


//
// Set community
//

void SNMP_SecurityContext::setCommunity(const char *community)
{
	safe_free(m_community);
	m_community = strdup(CHECK_NULL_A(community));
}


//
// Set user
//

void SNMP_SecurityContext::setUser(const char *user)
{
	safe_free(m_user);
	m_user = strdup(CHECK_NULL_A(user));
}


//
// Set authentication password
//

void SNMP_SecurityContext::setAuthPassword(const char *authPassword)
{
	safe_free(m_authPassword);
	m_authPassword = strdup(CHECK_NULL_A(authPassword));
}


//
// Set encryption password
//

void SNMP_SecurityContext::setEncryptionPassword(const char *encryptionPassword)
{
	safe_free(m_encryptionPassword);
	m_encryptionPassword = strdup(CHECK_NULL_A(encryptionPassword));
}
