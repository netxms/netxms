/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2012 NetXMS Team
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
** File: mt.cpp
**
**/

#include "nxcore.h"
#include <netxms_mt.h>

/**
 * Create mapping table object from NXCP message
 */
MappingTable *MappingTable::createFromMessage(CSCPMessage *msg)
{
	MappingTable *mt = new MappingTable((LONG)msg->GetVariableLong(VID_MAPPING_TABLE_ID),
	                                    msg->GetVariableStr(VID_NAME), 
													msg->GetVariableLong(VID_FLAGS), 
													msg->GetVariableStr(VID_DESCRIPTION));

	int count = (int)msg->GetVariableLong(VID_NUM_ELEMENTS);
	DWORD varId = VID_ELEMENT_LIST_BASE;
	for(int i = 0; i < count; i++)
	{
		TCHAR key[64];
		msg->GetVariableStr(varId++, key, 64);
		if (mt->m_flags & MTF_NUMERIC_KEYS)
		{
			long n = _tcstol(key, NULL, 0);
			_sntprintf(key, 64, _T("%ld"), n);
		}
		TCHAR *value = msg->GetVariableStr(varId++);
		TCHAR *description = msg->GetVariableStr(varId++);
		mt->m_data->set(key, new MappingTableElement(value, description));
		varId += 7;
	}

	return mt;
}

/**
 * Create mapping table object from database
 */
MappingTable *MappingTable::createFromDatabase(LONG id)
{
	MappingTable *mt = NULL;

	DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

	DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT name,flags,description FROM mapping_tables WHERE id=?"));
	if (hStmt != NULL)
	{
		DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, id);
		DB_RESULT hResult = DBSelectPrepared(hStmt);
		if (hResult != NULL)
		{
			if (DBGetNumRows(hResult) > 0)
			{
				mt = new MappingTable(id, 
					DBGetField(hResult, 0, 0, NULL, 0),		// name
					DBGetFieldULong(hResult, 0, 1),			// flags
					DBGetField(hResult, 0, 2, NULL, 0));	// description
			}
			DBFreeResult(hResult);
		}
		DBFreeStatement(hStmt);
	}

	if (mt != NULL)
	{
		hStmt = DBPrepare(hdb, _T("SELECT md_key,md_value,description FROM mapping_data WHERE table_id=?"));
		if (hStmt != NULL)
		{
			DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, id);
			DB_RESULT hResult = DBSelectPrepared(hStmt);
			if (hResult != NULL)
			{
				int count = DBGetNumRows(hResult);
				for(int i = 0; i < count; i++)
				{
					TCHAR key[64];
					DBGetField(hResult, i, 0, key, 64);
					if (mt->m_flags & MTF_NUMERIC_KEYS)
					{
						long n = _tcstol(key, NULL, 0);
						_sntprintf(key, 64, _T("%ld"), n);
					}
					mt->m_data->set(key, new MappingTableElement(DBGetField(hResult, i, 1, NULL, 0), DBGetField(hResult, i, 2, NULL, 0)));
				}
				DBFreeResult(hResult);
			}
			DBFreeStatement(hStmt);
		}
	}

	DBConnectionPoolReleaseConnection(hdb);
	return mt;
}

/**
 * Internal constructor
 */
MappingTable::MappingTable(LONG id, TCHAR *name, DWORD flags, TCHAR *description)
{
	m_id = id;
	m_name = name;
	m_flags = flags;
	m_description = description;
	m_data = new StringObjectMap<MappingTableElement>(true);
}

/**
 * Destructor
 */
MappingTable::~MappingTable()
{
	safe_free(m_name);
	safe_free(m_description);
	delete m_data;
}

/**
 * Fill NXCP message with mapping table's data
 */
void MappingTable::fillMessage(CSCPMessage *msg)
{
	msg->SetVariable(VID_MAPPING_TABLE_ID, (DWORD)m_id);
	msg->SetVariable(VID_NAME, CHECK_NULL_EX(m_name));
	msg->SetVariable(VID_FLAGS, m_flags);
	msg->SetVariable(VID_DESCRIPTION, CHECK_NULL_EX(m_description));
	
	msg->SetVariable(VID_NUM_ELEMENTS, m_data->getSize());
	DWORD varId = VID_ELEMENT_LIST_BASE;
	for(DWORD i = 0; i < m_data->getSize(); i++)
	{
		msg->SetVariable(varId++, m_data->getKeyByIndex(i));
		MappingTableElement *e = m_data->getValueByIndex(i);
		msg->SetVariable(varId++, e->getValue());
		msg->SetVariable(varId++, e->getDescription());
		varId += 7;
	}
}

/**
 * Save mapping table to database
 */
bool MappingTable::saveToDatabase(DB_HANDLE hdb)
{
	if (!DBBegin(hdb))
		return false;

	DB_STATEMENT hStmt;
	if (IsDatabaseRecordExist(hdb, _T("mapping_tables"), _T("id"), (DWORD)m_id))
	{
		hStmt = DBPrepare(hdb, _T("UPDATE mapping_tables SET name=?,flags=?,description=? WHERE id=?"));
	}
	else
	{
		hStmt = DBPrepare(hdb, _T("INSERT INTO mapping_tables (name,flags,description,id) VALUES (?,?,?,?)"));
	}
	if (hStmt == NULL)
		goto failure2;

	DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, m_name, DB_BIND_STATIC);
	DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, m_flags);
	DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, m_description, DB_BIND_STATIC);
	DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, m_id);

	if (!DBExecute(hStmt))
		goto failure;
	DBFreeStatement(hStmt);

	hStmt = DBPrepare(hdb, _T("DELETE FROM mapping_data WHERE table_id=?"));
	if (hStmt == NULL)
		goto failure2;

	DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
	if (!DBExecute(hStmt))
		goto failure;
	DBFreeStatement(hStmt);

	hStmt = DBPrepare(hdb, _T("INSERT INTO mapping_data (table_id,md_key,md_value,description) VALUES (?,?,?,?)"));
	if (hStmt == NULL)
		goto failure2;

	for(DWORD i = 0; i < m_data->getSize(); i++)
	{
		MappingTableElement *e = m_data->getValueByIndex(i);
		DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
		DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, m_data->getKeyByIndex(i), DB_BIND_STATIC);
		DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, e->getValue(), DB_BIND_STATIC);
		DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, e->getDescription(), DB_BIND_STATIC);
		if (!DBExecute(hStmt))
			goto failure;
	}
	DBFreeStatement(hStmt);

	DBCommit(hdb);
	return true;

failure:
	DBFreeStatement(hStmt);

failure2:
	DBRollback(hdb);
	return false;
}

/**
 * Get value by key
 */
const TCHAR *MappingTable::get(const TCHAR *key)
{
	MappingTableElement *e = m_data->get(key);
	return (e != NULL) ? e->getValue() : NULL;
}

/**
 * Defined mapping tables
 */
static ObjectArray<MappingTable> s_mappingTables;
static RWLOCK s_mappingTablesLock;
