/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2013 Victor Kirhenshtein
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
** File: dctcolumn.cpp
**
**/

#include "nxcore.h"

/**
 * Copy constructor
 */
DCTableColumn::DCTableColumn(const DCTableColumn *src)
{
	nx_strncpy(m_name, src->m_name, MAX_COLUMN_NAME);
	m_flags = src->m_flags;
	m_snmpOid = (src->m_snmpOid != NULL) ? new SNMP_ObjectId(src->m_snmpOid->getLength(), src->m_snmpOid->getValue()) : NULL;
   m_displayName = (src->m_displayName != NULL) ? _tcsdup(src->m_displayName) : NULL;
}

/**
 * Create column object from NXCP message
 */
DCTableColumn::DCTableColumn(CSCPMessage *msg, UINT32 baseId)
{
	msg->GetVariableStr(baseId, m_name, MAX_COLUMN_NAME);
	m_flags = msg->GetVariableShort(baseId + 1);
   m_displayName = msg->GetVariableStr(baseId + 3);

   if (msg->IsVariableExist(baseId + 2))
	{
		UINT32 oid[256];
		UINT32 len = msg->GetVariableInt32Array(baseId + 2, 256, oid);
		if (len > 0)
		{
			m_snmpOid = new SNMP_ObjectId(len, oid);
		}
		else
		{
			m_snmpOid = NULL;
		}
	}
	else
	{
		m_snmpOid = NULL;
	}
}

/**
 * Create column object from database result set
 * Expected field order is following:
 *    column_name,flags,snmp_oid,display_name
 */
DCTableColumn::DCTableColumn(DB_RESULT hResult, int row)
{
	DBGetField(hResult, row, 0, m_name, MAX_COLUMN_NAME);
	m_flags = (UINT16)DBGetFieldULong(hResult, row, 1);
   m_displayName = DBGetField(hResult, row, 3, NULL, 0);

	TCHAR oid[1024];
	oid[0] = 0;
	DBGetField(hResult, row, 2, oid, 1024);
	StrStrip(oid);
	if (oid[0] != 0)
	{
		UINT32 oidBin[256];
		UINT32 len = SNMPParseOID(oid, oidBin, 256);
		if (len > 0)
		{
			m_snmpOid = new SNMP_ObjectId(len, oidBin);
		}
		else
		{
			m_snmpOid = NULL;
		}
	}
	else
	{
		m_snmpOid = NULL;
	}
}

/**
 * Create from NXMP record
 */
DCTableColumn::DCTableColumn(ConfigEntry *e)
{
   nx_strncpy(m_name, e->getSubEntryValue(_T("name"), 0, _T("")), MAX_COLUMN_NAME);
   m_flags = (UINT16)e->getSubEntryValueUInt(_T("flags"));
   m_displayName = _tcsdup(e->getSubEntryValue(_T("displayName"), 0, _T("")));

   const TCHAR *oid = e->getSubEntryValue(_T("snmpOid"));
   if ((oid != NULL) && (*oid != 0))
   {
		UINT32 oidBin[256];
		UINT32 len = SNMPParseOID(oid, oidBin, 256);
		if (len > 0)
		{
			m_snmpOid = new SNMP_ObjectId(len, oidBin);
		}
		else
		{
			m_snmpOid = NULL;
		}
   }
	else
	{
		m_snmpOid = NULL;
	}
}

/**
 * Destructor
 */
DCTableColumn::~DCTableColumn()
{
	delete m_snmpOid;
   safe_free(m_displayName);
}

/**
 * Create NXMP record
 */
void DCTableColumn::createNXMPRecord(String &str, int id)
{
   str.addFormattedString(_T("\t\t\t\t\t\t<column id=\"%d\">\n")
                          _T("\t\t\t\t\t\t\t<name>%s</name>\n")
                          _T("\t\t\t\t\t\t\t<displayName>%s</displayName>\n")
                          _T("\t\t\t\t\t\t\t<snmpOid>%s</snmpOid>\n")
                          _T("\t\t\t\t\t\t\t<flags>%d</flags>\n")
                          _T("\t\t\t\t\t\t</column>\n"),
								  id, (const TCHAR *)EscapeStringForXML2(m_name),
								  (const TCHAR *)EscapeStringForXML2(CHECK_NULL_EX(m_displayName)),
                          (m_snmpOid != NULL) ? m_snmpOid->getValueAsText() : _T(""), (int)m_flags);
}
