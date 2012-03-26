/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2012 Victor Kirhenshtein
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


//
// Copy constructor
//

DCTableColumn::DCTableColumn(const DCTableColumn *src)
{
	nx_strncpy(m_name, src->m_name, MAX_COLUMN_NAME);
	m_dataType = src->m_dataType;
	m_snmpOid = (src->m_snmpOid != NULL) ? new SNMP_ObjectId(src->m_snmpOid->getLength(), src->m_snmpOid->getValue()) : NULL;
	setTransformationScript(src->m_scriptSource);
}


//
// Create column object from NXCP message
//

DCTableColumn::DCTableColumn(CSCPMessage *msg, DWORD baseId)
{
	msg->GetVariableStr(baseId, m_name, MAX_COLUMN_NAME);
	m_dataType = (int)msg->GetVariableShort(baseId + 1);

	if (msg->IsVariableExist(baseId + 2))
	{
		TCHAR *s = msg->GetVariableStr(baseId + 2);
		setTransformationScript(s);
		safe_free(s);
	}
	else
	{
		setTransformationScript(NULL);
	}

	if (msg->IsVariableExist(baseId + 3))
	{
		DWORD oid[256];
		DWORD len = msg->GetVariableInt32Array(baseId + 3, 256, oid);
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


//
// Create column object from database result set
// Expected field order is following:
//    column_name,data_type,snmp_oid,transformation_script
//

DCTableColumn::DCTableColumn(DB_RESULT hResult, int row)
{
	DBGetField(hResult, row, 0, m_name, MAX_COLUMN_NAME);
	m_dataType = DBGetFieldLong(hResult, row, 1);

	TCHAR *s = DBGetField(hResult, row, 2, NULL, 0);
	setTransformationScript(s);
	safe_free(s);

	TCHAR oid[1024];
	oid[0] = 0;
	DBGetField(hResult, row, 3, oid, 1024);
	StrStrip(oid);
	if (oid[0] != 0)
	{
		DWORD oidBin[256];
		DWORD len = SNMPParseOID(oid, oidBin, 256);
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


//
// Destructor
//

DCTableColumn::~DCTableColumn()
{
	delete m_snmpOid;
	safe_free(m_scriptSource);
	delete m_script;
}


//
// Set new transformation script
//

void DCTableColumn::setTransformationScript(const TCHAR *script)
{
   safe_free(m_scriptSource);
   delete m_script;
   if (script != NULL)
   {
      m_scriptSource = _tcsdup(script);
      StrStrip(m_scriptSource);
      if (m_scriptSource[0] != 0)
      {
			/* TODO: add compilation error handling */
         m_script = (NXSL_Program *)NXSLCompile(m_scriptSource, NULL, 0);
      }
      else
      {
         m_script = NULL;
      }
   }
   else
   {
      m_scriptSource = NULL;
      m_script = NULL;
   }
}
