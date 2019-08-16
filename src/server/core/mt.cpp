/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2019 Victor Kirhenshtein
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
MappingTable *MappingTable::createFromMessage(NXCPMessage *msg)
{
	MappingTable *mt = new MappingTable((LONG)msg->getFieldAsUInt32(VID_MAPPING_TABLE_ID),
	                                    msg->getFieldAsString(VID_NAME), 
													msg->getFieldAsUInt32(VID_FLAGS), 
													msg->getFieldAsString(VID_DESCRIPTION));

	int count = (int)msg->getFieldAsUInt32(VID_NUM_ELEMENTS);
	UINT32 varId = VID_ELEMENT_LIST_BASE;
	for(int i = 0; i < count; i++)
	{
		TCHAR key[64];
		msg->getFieldAsString(varId++, key, 64);
		if (mt->m_flags & MTF_NUMERIC_KEYS)
		{
			long n = _tcstol(key, NULL, 0);
			_sntprintf(key, 64, _T("%ld"), n);
		}
		TCHAR *value = msg->getFieldAsString(varId++);
		TCHAR *description = msg->getFieldAsString(varId++);
		mt->m_data->set(key, new MappingTableElement(value, description));
		varId += 7;
	}

	return mt;
}

/**
 * Create mapping table object from database
 */
MappingTable *MappingTable::createFromDatabase(DB_HANDLE hdb, LONG id)
{
	MappingTable *mt = NULL;

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

	return mt;
}

/**
 * Internal constructor
 */
MappingTable::MappingTable(LONG id, TCHAR *name, UINT32 flags, TCHAR *description)
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
	MemFree(m_name);
	MemFree(m_description);
	delete m_data;
}

/**
 * Callback for setting mapping table elements in NXCP message
 */
static EnumerationCallbackResult FillMessageCallback(const TCHAR *key,
         const MappingTableElement *value, std::pair<NXCPMessage*, UINT32> *context)
{
	context->first->setField(context->second, key);
	context->first->setField(context->second + 1, value->getValue());
	context->first->setField(context->second + 2, value->getDescription());
	context->second += 10;
   return _CONTINUE;
}

/**
 * Fill NXCP message with mapping table's data
 */
void MappingTable::fillMessage(NXCPMessage *msg)
{
	msg->setField(VID_MAPPING_TABLE_ID, (UINT32)m_id);
	msg->setField(VID_NAME, CHECK_NULL_EX(m_name));
	msg->setField(VID_FLAGS, m_flags);
	msg->setField(VID_DESCRIPTION, CHECK_NULL_EX(m_description));
	
	msg->setField(VID_NUM_ELEMENTS, (UINT32)m_data->size());
   std::pair<NXCPMessage*, UINT32> data(msg, VID_ELEMENT_LIST_BASE);
   m_data->forEach(FillMessageCallback, &data);
}

/**
 * Callback for saving elements into database
 */
static EnumerationCallbackResult SaveElementCallback(const TCHAR *key, const MappingTableElement *value, db_statement_t *hStmt)
{
	DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, key, DB_BIND_STATIC);
	DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, value->getValue(), DB_BIND_STATIC);
	DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, value->getDescription(), DB_BIND_STATIC);
   return DBExecute(hStmt) ? _CONTINUE : _STOP;
}

/**
 * Save mapping table to database
 */
bool MappingTable::saveToDatabase()
{
	DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

	if (!DBBegin(hdb))
	{
		DBConnectionPoolReleaseConnection(hdb);
		return false;
	}

	DB_STATEMENT hStmt;
	if (IsDatabaseRecordExist(hdb, _T("mapping_tables"), _T("id"), (UINT32)m_id))
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

	DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
   if (m_data->forEach(SaveElementCallback, hStmt) == _STOP)
      goto failure;
	DBFreeStatement(hStmt);

	DBCommit(hdb);
	DBConnectionPoolReleaseConnection(hdb);
	return true;

failure:
	DBFreeStatement(hStmt);

failure2:
	DBRollback(hdb);
	DBConnectionPoolReleaseConnection(hdb);
	return false;
}

/**
 * Delete from database
 */
bool MappingTable::deleteFromDatabase()
{
	DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

	if (!DBBegin(hdb))
	{
		DBConnectionPoolReleaseConnection(hdb);
		return false;
	}

	BOOL success = FALSE;

	DB_STATEMENT hStmt = DBPrepare(hdb, _T("DELETE FROM mapping_tables WHERE id=?"));
	if (hStmt != NULL)
	{
		DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
		success = DBExecute(hStmt);
		DBFreeStatement(hStmt);
	}

	if (success)
	{
		success = FALSE;
		DB_STATEMENT hStmt = DBPrepare(hdb, _T("DELETE FROM mapping_data WHERE table_id=?"));
		if (hStmt != NULL)
		{
			DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
			success = DBExecute(hStmt);
			DBFreeStatement(hStmt);
		}
	}

	if (success)
		DBCommit(hdb);
	else
		DBRollback(hdb);

	DBConnectionPoolReleaseConnection(hdb);
	return success ? true : false;
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
static ObjectArray<MappingTable> s_mappingTables(0, 16, true);
static RWLOCK s_mappingTablesLock;

/**
 * Init mapping tables
 */
void InitMappingTables()
{
	s_mappingTablesLock = RWLockCreate();

	DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
	DB_RESULT hResult = DBSelect(hdb, _T("SELECT id FROM mapping_tables"));
	if (hResult == NULL)
	{
	   DBConnectionPoolReleaseConnection(hdb);
		return;
	}

	int count = DBGetNumRows(hResult);
	for(int i = 0; i < count; i++)
	{
		MappingTable *mt = MappingTable::createFromDatabase(hdb, DBGetFieldLong(hResult, i, 0));
		if (mt != NULL)
			s_mappingTables.add(mt);
	}
	DBFreeResult(hResult);
	DBConnectionPoolReleaseConnection(hdb);
	DbgPrintf(2, _T("%d mapping tables loaded"), s_mappingTables.size());
}

/**
 * Callback for client notification
 */
static void NotifyClients(ClientSession *session, std::pair<UINT32, UINT32> *context)
{
	session->notify(context->first, context->second);
}

/**
 * Create/update mapping table. If table ID is 0, new table will be craeted, 
 * otherwise existing table with same ID updated.
 *
 * @param msg NXCP message with table's data
 * @return RCC
 */
UINT32 UpdateMappingTable(NXCPMessage *msg, LONG *newId)
{
	UINT32 rcc;
	MappingTable *mt = MappingTable::createFromMessage(msg);
	RWLockWriteLock(s_mappingTablesLock, INFINITE);
	if (mt->getId() != 0)
	{
		rcc = RCC_INVALID_MAPPING_TABLE_ID;
		for(int i = 0; i < s_mappingTables.size(); i++)
		{
			if (s_mappingTables.get(i)->getId() == mt->getId())
			{
				if (mt->saveToDatabase())
				{
					s_mappingTables.set(i, mt);
					*newId = mt->getId();
					rcc = RCC_SUCCESS;
					DbgPrintf(4, _T("Mapping table updated, name=\"%s\", id=%d"), mt->getName(), mt->getId());
				}
				else
				{
					rcc = RCC_DB_FAILURE;
				}
				break;
			}
		}
	}
	else
	{
		mt->createUniqueId();
		if (mt->saveToDatabase())
		{
			s_mappingTables.add(mt);
			*newId = mt->getId();
			rcc = RCC_SUCCESS;
			DbgPrintf(4, _T("New mapping table added, name=\"%s\", id=%d"), mt->getName(), mt->getId());
		}
		else
		{
			rcc = RCC_DB_FAILURE;
		}
	}

	std::pair<UINT32, UINT32> context(NX_NOTIFY_MAPTBL_CHANGED, mt->getId());

	RWLockUnlock(s_mappingTablesLock);

	if (rcc == RCC_SUCCESS)
	{
		EnumerateClientSessions(NotifyClients, &context);
	}
	else
	{
		delete mt;
	}

	return rcc;
}

/**
 * Delete mapping table
 *
 * @param id mapping table ID
 * @return RCC
 */
UINT32 DeleteMappingTable(LONG id)
{
	UINT32 rcc = RCC_INVALID_MAPPING_TABLE_ID;
	RWLockWriteLock(s_mappingTablesLock, INFINITE);
	for(int i = 0; i < s_mappingTables.size(); i++)
	{
		MappingTable *mt = s_mappingTables.get(i);
		if (mt->getId() == id)
		{
			if (mt->deleteFromDatabase())
			{
				s_mappingTables.remove(i);
				rcc = RCC_SUCCESS;
				DbgPrintf(4, _T("Mapping table deleted, id=%d"), id);
			}
			else
			{
				rcc = RCC_DB_FAILURE;
			}
			break;
		}
	}
	RWLockUnlock(s_mappingTablesLock);
	if (rcc == RCC_SUCCESS)
	{
	   std::pair<UINT32, UINT32> context(NX_NOTIFY_MAPTBL_DELETED, id);
		EnumerateClientSessions(NotifyClients, &context);
	}
	return rcc;
}

/**
 * Get single mapping table
 *
 * @param id mapping table ID
 * @param msg NXCP message to fill
 * @return RCC
 */
UINT32 GetMappingTable(LONG id, NXCPMessage *msg)
{
	UINT32 rcc = RCC_INVALID_MAPPING_TABLE_ID;
	RWLockReadLock(s_mappingTablesLock, INFINITE);
	for(int i = 0; i < s_mappingTables.size(); i++)
	{
		if (s_mappingTables.get(i)->getId() == id)
		{
			s_mappingTables.get(i)->fillMessage(msg);
			rcc = RCC_SUCCESS;
			break;
		}
	}
	RWLockUnlock(s_mappingTablesLock);
	return rcc;
}

/**
 * List all mapping tables
 *
 * @param msg NXCP mesage to fill
 * @return RCC
 */
UINT32 ListMappingTables(NXCPMessage *msg)
{
	UINT32 varId = VID_ELEMENT_LIST_BASE;
	RWLockReadLock(s_mappingTablesLock, INFINITE);
	msg->setField(VID_NUM_ELEMENTS, (UINT32)s_mappingTables.size());
	for(int i = 0; i < s_mappingTables.size(); i++)
	{
		MappingTable *mt = s_mappingTables.get(i);
		msg->setField(varId++, (UINT32)mt->getId());
		msg->setField(varId++, mt->getName());
		msg->setField(varId++, mt->getDescription());
		msg->setField(varId++, mt->getFlags());
		varId += 6;
	}
	RWLockUnlock(s_mappingTablesLock);
	return RCC_SUCCESS;
}

/**
 * NXSL API: function map
 * Format: map(table, key, [default])
 * Returns: mapped value if key found; otherwise default value or null if default value is not provided
 * Table can be referenced by name or ID
 */
int F_map(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
	if ((argc < 2) || (argc > 3))
		return NXSL_ERR_INVALID_ARGUMENT_COUNT;

	if (!argv[0]->isString() || !argv[1]->isString())
		return NXSL_ERR_NOT_STRING;

	LONG tableId = (argv[0]->isInteger()) ? argv[0]->getValueAsInt32() : 0;
	const TCHAR *value = NULL;
	RWLockReadLock(s_mappingTablesLock, INFINITE);
	for(int i = 0; i < s_mappingTables.size(); i++)
	{
		MappingTable *mt = s_mappingTables.get(i);
		if (((tableId > 0) && (mt->getId() == tableId)) || !_tcsicmp(argv[0]->getValueAsCString(), mt->getName()))
		{
			value = mt->get(argv[1]->getValueAsCString());
			break;
		}
	}
	*result = (value != NULL) ? vm->createValue(value) : ((argc == 3) ? vm->createValue(argv[2]) : vm->createValue());
	RWLockUnlock(s_mappingTablesLock);

	return 0;
}

/**
 * NXSL API: function mapList
 * Format: mapList(table, list, separator, [default])
 * Returns: list of mapped values; for each element mapped value if key found, otherwise default value or empty string if default value is not provided
 * Table can be referenced by name or ID
 */
int F_mapList(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   if ((argc < 3) || (argc > 4))
      return NXSL_ERR_INVALID_ARGUMENT_COUNT;

   if (!argv[0]->isString() || !argv[1]->isString() || !argv[2]->isString())
      return NXSL_ERR_NOT_STRING;

   if ((argc == 4) && !argv[3]->isString())
      return NXSL_ERR_NOT_STRING;

   int count;
   TCHAR **strings = SplitString(argv[1]->getValueAsCString(), argv[2]->getValueAsCString()[0], &count);

   LONG tableId = (argv[0]->isInteger()) ? argv[0]->getValueAsInt32() : 0;
   MappingTable *mt = NULL;
   RWLockReadLock(s_mappingTablesLock, INFINITE);
   for(int i = 0; i < s_mappingTables.size(); i++)
   {
      MappingTable *t = s_mappingTables.get(i);
      if (((tableId > 0) && (t->getId() == tableId)) || !_tcsicmp(argv[0]->getValueAsCString(), t->getName()))
      {
         mt = t;
         break;
      }
   }

   if (mt != NULL)
   {
      String mappedList;
      for(int i = 0; i < count; i++)
      {
         if (i > 0)
            mappedList.append(argv[2]->getValueAsCString());

         const TCHAR *v = mt->get(strings[i]);
         mappedList.append((v != NULL) ? v : ((argc == 4) ? argv[3]->getValueAsCString() : _T("")));
         free(strings[i]);
      }
      *result = vm->createValue(mappedList);
   }
   else
   {
      // mapping table not found, return original value
      *result = vm->createValue(argv[0]);
      for(int i = 0; i < count; i++)
         free(strings[i]);
   }
   RWLockUnlock(s_mappingTablesLock);

   free(strings);
   return 0;
}
