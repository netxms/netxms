/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2025 Victor Kirhenshtein
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
#include <nms_users.h>

#define DEBUG_TAG _T("mtables")

/**
 * Create mapping table object from NXCP message
 */
MappingTable *MappingTable::createFromMessage(const NXCPMessage& msg)
{
	MappingTable *mt = new MappingTable(msg.getFieldAsInt32(VID_MAPPING_TABLE_ID),
	                                    msg.getFieldAsString(VID_NAME),
													msg.getFieldAsUInt32(VID_FLAGS),
													msg.getFieldAsString(VID_DESCRIPTION));

	int count = msg.getFieldAsInt32(VID_NUM_ELEMENTS);
	uint32_t fieldId = VID_ELEMENT_LIST_BASE;
	for(int i = 0; i < count; i++)
	{
		TCHAR key[64];
		msg.getFieldAsString(fieldId++, key, 64);
		if (mt->m_flags & MTF_NUMERIC_KEYS)
		{
			int32_t n = _tcstol(key, nullptr, 0);
			_sntprintf(key, 64, _T("%ld"), n);
		}
		TCHAR *value = msg.getFieldAsString(fieldId++);
		TCHAR *description = msg.getFieldAsString(fieldId++);
		mt->m_data.set(key, new MappingTableElement(value, description));
		fieldId += 7;
	}

	return mt;
}

/**
 * Create mapping table object from database
 */
MappingTable *MappingTable::createFromDatabase(DB_HANDLE hdb, uint32_t id)
{
	MappingTable *mt = nullptr;

	DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT name,flags,description FROM mapping_tables WHERE id=?"));
	if (hStmt != nullptr)
	{
		DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, id);
		DB_RESULT hResult = DBSelectPrepared(hStmt);
		if (hResult != nullptr)
		{
			if (DBGetNumRows(hResult) > 0)
			{
				mt = new MappingTable(id, 
					DBGetField(hResult, 0, 0, nullptr, 0),		// name
					DBGetFieldULong(hResult, 0, 1),			// flags
					DBGetField(hResult, 0, 2, nullptr, 0));	// description
			}
			DBFreeResult(hResult);
		}
		DBFreeStatement(hStmt);
	}

	if (mt != nullptr)
	{
		hStmt = DBPrepare(hdb, _T("SELECT md_key,md_value,description FROM mapping_data WHERE table_id=?"));
		if (hStmt != nullptr)
		{
			DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, id);
			DB_RESULT hResult = DBSelectPrepared(hStmt);
			if (hResult != nullptr)
			{
				int count = DBGetNumRows(hResult);
				for(int i = 0; i < count; i++)
				{
					TCHAR key[64];
					DBGetField(hResult, i, 0, key, 64);
					if (mt->m_flags & MTF_NUMERIC_KEYS)
					{
						int32_t n = _tcstol(key, NULL, 0);
						_sntprintf(key, 64, _T("%ld"), n);
					}
					mt->m_data.set(key, new MappingTableElement(DBGetField(hResult, i, 1, nullptr, 0), DBGetField(hResult, i, 2, nullptr, 0)));
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
MappingTable::MappingTable(int32_t id, TCHAR *name, uint32_t flags, TCHAR *description) : m_data(Ownership::True)
{
	m_id = id;
	m_name = name;
	m_flags = flags;
	m_description = description;
}

/**
 * Destructor
 */
MappingTable::~MappingTable()
{
	MemFree(m_name);
	MemFree(m_description);
}

/**
 * Fill NXCP message with mapping table's data
 */
void MappingTable::fillMessage(NXCPMessage *msg) const
{
	msg->setField(VID_MAPPING_TABLE_ID, m_id);
	msg->setField(VID_NAME, CHECK_NULL_EX(m_name));
	msg->setField(VID_FLAGS, m_flags);
	msg->setField(VID_DESCRIPTION, CHECK_NULL_EX(m_description));

	msg->setField(VID_NUM_ELEMENTS, m_data.size());
   uint32_t fieldId = VID_ELEMENT_LIST_BASE;
   m_data.forEach(
      [msg, &fieldId] (const TCHAR *key, const MappingTableElement *value) -> EnumerationCallbackResult
      {
         msg->setField(fieldId, key);
         msg->setField(fieldId + 1, value->getValue());
         msg->setField(fieldId + 2, value->getDescription());
         fieldId += 10;
         return _CONTINUE;
      });
}

/**
 * Save mapping table to database
 */
bool MappingTable::saveToDatabase() const
{
	DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

	if (!DBBegin(hdb))
	{
		DBConnectionPoolReleaseConnection(hdb);
		return false;
	}

	DB_STATEMENT hStmt;
	if (IsDatabaseRecordExist(hdb, _T("mapping_tables"), _T("id"), static_cast<uint32_t>(m_id)))
	{
		hStmt = DBPrepare(hdb, _T("UPDATE mapping_tables SET name=?,flags=?,description=? WHERE id=?"));
	}
	else
	{
		hStmt = DBPrepare(hdb, _T("INSERT INTO mapping_tables (name,flags,description,id) VALUES (?,?,?,?)"));
	}
	if (hStmt == nullptr)
		goto failure2;

	DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, m_name, DB_BIND_STATIC);
	DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, m_flags);
	DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, m_description, DB_BIND_STATIC);
	DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, m_id);

	if (!DBExecute(hStmt))
		goto failure;
	DBFreeStatement(hStmt);

	if (!ExecuteQueryOnObject(hdb, m_id, _T("DELETE FROM mapping_data WHERE table_id=?")))
		goto failure2;

	hStmt = DBPrepare(hdb, _T("INSERT INTO mapping_data (table_id,md_key,md_value,description) VALUES (?,?,?,?)"));
	if (hStmt == nullptr)
		goto failure2;

	DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
	if (m_data.forEach(
      [hStmt] (const TCHAR *key, const MappingTableElement *value) -> EnumerationCallbackResult
      {
         DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, key, DB_BIND_STATIC);
         DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, value->getValue(), DB_BIND_STATIC);
         DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, value->getDescription(), DB_BIND_STATIC);
         return DBExecute(hStmt) ? _CONTINUE : _STOP;
      }) == _STOP)
	{
      goto failure;
	}
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

	bool success = ExecuteQueryOnObject(hdb, m_id, _T("DELETE FROM mapping_tables WHERE id=?"));
	if (success)
	   success = ExecuteQueryOnObject(hdb, m_id, _T("DELETE FROM mapping_data WHERE table_id=?"));

	if (success)
		DBCommit(hdb);
	else
		DBRollback(hdb);

	DBConnectionPoolReleaseConnection(hdb);
	return success;
}

/**
 * Convert to JSON format
 */
json_t *MappingTable::toJson() const
{
   json_t *root = json_object();

   json_object_set_new(root, "name", json_string_t(m_name));
   json_object_set_new(root, "description", json_string_t(m_description));
   json_object_set_new(root, "flags", json_integer(m_flags));

   json_t *elements = json_array();
   m_data.forEach(
      [elements] (const TCHAR *key, const MappingTableElement *value) -> EnumerationCallbackResult
      {
         json_t *e = json_object();
         json_object_set_new(e, "key", json_string_t(key));
         json_object_set_new(e, "value", json_string_t(value->getValue()));
         json_object_set_new(e, "description", json_string_t(value->getDescription()));
         json_array_append_new(elements, e);
         return _CONTINUE;
      });
   json_object_set_new(root, "elements", elements);

   return root;
}

/**
 * Get all keys as NXSL array
 */
NXSL_Value *MappingTable::getKeysForNXSL(NXSL_VM *vm) const
{
   NXSL_Array *keys = new NXSL_Array(vm);
   m_data.forEach(
      [keys, vm] (const TCHAR *key, const MappingTableElement *value) -> EnumerationCallbackResult
      {
         keys->append(vm->createValue(key));
         return _CONTINUE;
      });
   return vm->createValue(keys);
}

/**
 * Defined mapping tables
 */
static ObjectArray<MappingTable> s_mappingTables(0, 16, Ownership::True);
static RWLock s_mappingTablesLock;

/**
 * Init mapping tables
 */
void InitMappingTables()
{
	DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
	DB_RESULT hResult = DBSelect(hdb, _T("SELECT id FROM mapping_tables"));
	if (hResult == nullptr)
	{
	   DBConnectionPoolReleaseConnection(hdb);
		return;
	}

	int count = DBGetNumRows(hResult);
	for(int i = 0; i < count; i++)
	{
		MappingTable *mt = MappingTable::createFromDatabase(hdb, DBGetFieldLong(hResult, i, 0));
		if (mt != nullptr)
			s_mappingTables.add(mt);
	}
	DBFreeResult(hResult);
	DBConnectionPoolReleaseConnection(hdb);
	nxlog_debug_tag(DEBUG_TAG, 2, _T("%d mapping tables loaded"), s_mappingTables.size());
}

/**
 * Callback for client notification
 */
static void NotifyClients(ClientSession *session, std::pair<uint32_t, uint32_t> *context)
{
	session->notify(context->first, context->second);
}

/**
 * Create/update mapping table. If table ID is 0, new table will be created,
 * otherwise existing table with same ID updated.
 *
 * @param msg NXCP message with table's data
 * @return RCC
 */
uint32_t UpdateMappingTable(const NXCPMessage& msg, uint32_t *newId, ClientSession *session)
{
	uint32_t rcc;
	MappingTable *mt = MappingTable::createFromMessage(msg);
	s_mappingTablesLock.writeLock();
	if (mt->getId() != 0)
	{
		rcc = RCC_INVALID_MAPPING_TABLE_ID;
		for(int i = 0; i < s_mappingTables.size(); i++)
		{
			if (s_mappingTables.get(i)->getId() == mt->getId())
			{
				if (mt->saveToDatabase())
				{
				   json_t *oldValue = s_mappingTables.get(i)->toJson();
				   json_t *newValue = mt->toJson();
               session->writeAuditLogWithValues(AUDIT_SYSCFG, true, 0, oldValue, newValue, _T("Mapping table %s [%u] updated"), mt->getName(), mt->getId());
               json_decref(oldValue);
               json_decref(newValue);

					s_mappingTables.set(i, mt);
					*newId = mt->getId();
					rcc = RCC_SUCCESS;

					nxlog_debug_tag(DEBUG_TAG, 4, _T("Mapping table \"%s\" [%d] updated"), mt->getName(), mt->getId());
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

         json_t *newValue = mt->toJson();
         session->writeAuditLogWithValues(AUDIT_SYSCFG, true, 0, nullptr, newValue, _T("Mapping table %s [%u] created"), mt->getName(), mt->getId());
         json_decref(newValue);

			nxlog_debug_tag(DEBUG_TAG, 4, _T("Created new mapping table \"%s\" [%d]"), mt->getName(), mt->getId());
		}
		else
		{
			rcc = RCC_DB_FAILURE;
		}
	}

	std::pair<uint32_t, uint32_t> context(NX_NOTIFY_MAPTBL_CHANGED, mt->getId());

	s_mappingTablesLock.unlock();

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
uint32_t DeleteMappingTable(uint32_t id, ClientSession *session)
{
   uint32_t rcc = RCC_INVALID_MAPPING_TABLE_ID;
	s_mappingTablesLock.writeLock();
	for(int i = 0; i < s_mappingTables.size(); i++)
	{
		MappingTable *mt = s_mappingTables.get(i);
		if (mt->getId() == id)
		{
			if (mt->deleteFromDatabase())
			{
			   json_t *json = mt->toJson();
			   session->writeAuditLogWithValues(AUDIT_SYSCFG, true, 0, json, nullptr, _T("Mapping table \"%s\" [%d] deleted"), mt->getName(), id);
			   json_decref(json);

			   wchar_t username[MAX_USER_NAME];
            nxlog_debug_tag(DEBUG_TAG, 4, _T("Mapping table \"%s\" [%d] deleted by user %s"), mt->getName(), id, ResolveUserId(session->getUserId(), username, true));

				s_mappingTables.remove(i);
				rcc = RCC_SUCCESS;
			}
			else
			{
				rcc = RCC_DB_FAILURE;
			}
			break;
		}
	}
	s_mappingTablesLock.unlock();
	if (rcc == RCC_SUCCESS)
	{
	   std::pair<uint32_t, uint32_t> context(NX_NOTIFY_MAPTBL_DELETED, id);
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
uint32_t GetMappingTable(uint32_t id, NXCPMessage *msg)
{
   uint32_t rcc = RCC_INVALID_MAPPING_TABLE_ID;
	s_mappingTablesLock.readLock();
	for(int i = 0; i < s_mappingTables.size(); i++)
	{
		if (s_mappingTables.get(i)->getId() == id)
		{
			s_mappingTables.get(i)->fillMessage(msg);
			rcc = RCC_SUCCESS;
			break;
		}
	}
	s_mappingTablesLock.unlock();
	return rcc;
}

/**
 * List all mapping tables
 *
 * @param msg NXCP mesage to fill
 * @return RCC
 */
uint32_t ListMappingTables(NXCPMessage *msg)
{
	uint32_t fieldId = VID_ELEMENT_LIST_BASE;
	s_mappingTablesLock.readLock();
	msg->setField(VID_NUM_ELEMENTS, static_cast<uint32_t>(s_mappingTables.size()));
	for(int i = 0; i < s_mappingTables.size(); i++)
	{
		MappingTable *mt = s_mappingTables.get(i);
		msg->setField(fieldId++, mt->getId());
		msg->setField(fieldId++, mt->getName());
		msg->setField(fieldId++, mt->getDescription());
		msg->setField(fieldId++, mt->getFlags());
		fieldId += 6;
	}
	s_mappingTablesLock.unlock();
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

	uint32_t tableId = (argv[0]->isInteger()) ? argv[0]->getValueAsUInt32() : 0;
	const TCHAR *value = nullptr;
	s_mappingTablesLock.readLock();
	for(int i = 0; i < s_mappingTables.size(); i++)
	{
		MappingTable *mt = s_mappingTables.get(i);
		if (((tableId > 0) && (mt->getId() == tableId)) || !_tcsicmp(argv[0]->getValueAsCString(), mt->getName()))
		{
			value = mt->get(argv[1]->getValueAsCString());
			break;
		}
	}
	*result = (value != nullptr) ? vm->createValue(value) : ((argc == 3) ? vm->createValue(argv[2]) : vm->createValue());
	s_mappingTablesLock.unlock();

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

   uint32_t tableId = (argv[0]->isInteger()) ? argv[0]->getValueAsUInt32() : 0;
   MappingTable *mt = nullptr;
   s_mappingTablesLock.readLock();
   for(int i = 0; i < s_mappingTables.size(); i++)
   {
      MappingTable *t = s_mappingTables.get(i);
      if (((tableId > 0) && (t->getId() == tableId)) || !_tcsicmp(argv[0]->getValueAsCString(), t->getName()))
      {
         mt = t;
         break;
      }
   }

   if (mt != nullptr)
   {
      StringBuffer mappedList;
      for(int i = 0; i < count; i++)
      {
         if (i > 0)
            mappedList.append(argv[2]->getValueAsCString());

         const TCHAR *v = mt->get(strings[i]);
         mappedList.append((v != nullptr) ? v : ((argc == 4) ? argv[3]->getValueAsCString() : _T("")));
         MemFree(strings[i]);
      }
      *result = vm->createValue(mappedList);
   }
   else
   {
      // mapping table not found, return original value
      *result = vm->createValue(argv[0]);
      for(int i = 0; i < count; i++)
         MemFree(strings[i]);
   }
   s_mappingTablesLock.unlock();

   MemFree(strings);
   return 0;
}

/**
 * NXSL API: function GetMappingTableKeys
 * Format: GetMappingTableKeys(table)
 * Returns: all keys from given mapping table
 * Table can be referenced by name or ID
 */
int F_GetMappingTableKeys(int argc, NXSL_Value **argv, NXSL_Value **result, NXSL_VM *vm)
{
   if (!argv[0]->isString())
      return NXSL_ERR_NOT_STRING;

   uint32_t tableId = (argv[0]->isInteger()) ? argv[0]->getValueAsUInt32() : 0;
   NXSL_Value *value = nullptr;
   s_mappingTablesLock.readLock();
   for(int i = 0; i < s_mappingTables.size(); i++)
   {
      MappingTable *mt = s_mappingTables.get(i);
      if (((tableId > 0) && (mt->getId() == tableId)) || !_tcsicmp(argv[0]->getValueAsCString(), mt->getName()))
      {
         value = mt->getKeysForNXSL(vm);
         break;
      }
   }
   *result = (value != nullptr) ? value : vm->createValue();
   s_mappingTablesLock.unlock();

   return 0;
}
