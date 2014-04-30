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
** File: engine.cpp
**
**/

#include "libnxsnmp.h"


//
// Constructors
//

SNMP_Engine::SNMP_Engine()
{
	m_idLen = 0;
	m_engineBoots = 0;
	m_engineTime = 0;
}

SNMP_Engine::SNMP_Engine(BYTE *id, size_t idLen, int engineBoots, int engineTime)
{
	m_idLen = min(idLen, SNMP_MAX_ENGINEID_LEN);
	memcpy(m_id, id, m_idLen);
	m_engineBoots = engineBoots;
	m_engineTime = engineTime;
}

SNMP_Engine::SNMP_Engine(SNMP_Engine *src)
{
	m_idLen = src->m_idLen;
	memcpy(m_id, src->m_id, m_idLen);
	m_engineBoots = src->m_engineBoots;
	m_engineTime = src->m_engineTime;
}

/**
 * Destructor
 */
SNMP_Engine::~SNMP_Engine()
{
}
