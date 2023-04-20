/*
** NetXMS - Network Management System
** Copyright (C) 2022-2023 Raden Solutions
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
** File: maintenance.cpp
**
**/

#include "nxcore.h"

/**
 * Appends field list and conditions to query
 */
static void BuildFieldListAndCondition(StringBuffer* query, const SharedObjectArray<NetObj>& sources, time_t startTime, time_t endTime)
{
   //date_part('epoch',%s_timestamp)::int
   query->append(_T(" record_id,object_id,author,last_edited_by,description,"));
   if (g_dbSyntax == DB_SYNTAX_TSDB)
      query->append(_T("date_part('epoch',creation_time)::int,date_part('epoch',modification_time)::int"));
   else
      query->append(_T("creation_time,modification_time"));
   query->append(_T(" FROM maintenance_journal WHERE object_id IN ("));
   for (const shared_ptr<NetObj>& o : sources)
   {
      query->append(o->getId());
      query->append(_T(","));
   }
   query->shrink();
   query->append(_T(")"));

   if (startTime != 0)
   {
      query->append(_T(" AND creation_time>="));
      if (g_dbSyntax == DB_SYNTAX_TSDB)
         query->append(_T("to_timestamp("));
      query->append(static_cast<int64_t>(startTime));
      if (g_dbSyntax == DB_SYNTAX_TSDB)
         query->append(_T(")"));
   }
   if (endTime != 0)
   {
      query->append(_T(" AND creation_time<="));
      if (g_dbSyntax == DB_SYNTAX_TSDB)
         query->append(_T("to_timestamp("));
      query->append(static_cast<int64_t>(endTime));
      if (g_dbSyntax == DB_SYNTAX_TSDB)
         query->append(_T(")"));
   }

   query->append(_T(" ORDER BY record_id DESC"));
}

/**
 * Get all maintenance journal entries for the given object
 */
static bool ReadMaintenanceJournal(const SharedObjectArray<NetObj>& sources, uint32_t maxEntries, time_t startTime, time_t endTime, std::function<void (DB_RESULT)> callback)
{
   StringBuffer query;
   if (maxEntries == 0)
   {
      query.append(_T("SELECT "));
      BuildFieldListAndCondition(&query, sources, startTime, endTime);
   }
   else
   {
      switch (g_dbSyntax)
      {
         case DB_SYNTAX_MSSQL:
            query.append(_T("SELECT TOP "));
            query.append(maxEntries);
            BuildFieldListAndCondition(&query, sources, startTime, endTime);
            break;
         case DB_SYNTAX_ORACLE:
            query.append(_T("SELECT * FROM (SELECT "));
            BuildFieldListAndCondition(&query, sources, startTime, endTime);
            query.append(_T(") WHERE ROWNUM <= "));
            query.append(maxEntries);
            break;
         case DB_SYNTAX_DB2:
            query.append(_T("SELECT "));
            BuildFieldListAndCondition(&query, sources, startTime, endTime);
            query.append(_T(" FETCH FIRST "));
            query.append(maxEntries);
            query.append(_T(" ROWS ONLY"));
            break;
         default:
            query.append(_T("SELECT "));
            BuildFieldListAndCondition(&query, sources, startTime, endTime);
            query.append(_T(" LIMIT "));
            query.append(maxEntries);
            break;
      }
   }

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_RESULT hResult = DBSelect(hdb, query);
   if (hResult != nullptr)
   {
      callback(hResult);
      DBFreeResult(hResult);
   }
   DBConnectionPoolReleaseConnection(hdb);
   return hResult != nullptr;
}

/**
 * Read maintenance journal into NXCP message
 */
uint32_t ReadMaintenanceJournal(SharedObjectArray<NetObj>& sources, NXCPMessage *response, uint32_t maxEntries)
{
   return ReadMaintenanceJournal(sources, maxEntries, 0, 0, [response](DB_RESULT hResult) {
      int numRows = DBGetNumRows(hResult);
      response->setField(VID_NUM_ELEMENTS, numRows);

      uint32_t fieldId = VID_ELEMENT_LIST_BASE;
      for (int row = 0; row < numRows; row++, fieldId += 3)
      {
         response->setField(fieldId++, DBGetFieldULong(hResult, row, 0));          // id
         response->setField(fieldId++, DBGetFieldULong(hResult, row, 1));          // object_id
         response->setField(fieldId++, DBGetFieldULong(hResult, row, 2));          // author
         response->setField(fieldId++, DBGetFieldULong(hResult, row, 3));          // last_edited_by
         response->setField(fieldId++, DBGetFieldAsSharedString(hResult, row, 4)); // description
         response->setField(fieldId++, DBGetFieldULong(hResult, row, 5));          // creation_time
         response->setField(fieldId++, DBGetFieldULong(hResult, row, 6));          // modification_time
      }
   }) ? RCC_SUCCESS : RCC_DB_FAILURE;
}

/**
 * Add new entry to the object maintenance journal
 */
bool AddMaintenanceJournalRecord(uint32_t objectId, uint32_t userId, const TCHAR *description)
{
   bool success = false;
   DB_HANDLE db = DBConnectionPoolAcquireConnection();
   DB_STATEMENT stmt = DBPrepare(db,
      (g_dbSyntax == DB_SYNTAX_TSDB) ?
         _T("INSERT INTO maintenance_journal (record_id, object_id, author, last_edited_by, description, creation_time, modification_time) VALUES (?, ?, ?, ?, ?, to_timestamp(?), to_timestamp(?))") :
         _T("INSERT INTO maintenance_journal (record_id, object_id, author, last_edited_by, description, creation_time, modification_time) VALUES (?, ?, ?, ?, ?, ?, ?)"));
   if (stmt != nullptr)
   {
      DBBind(stmt, 1, DB_SQLTYPE_INTEGER, CreateUniqueId(IDG_MAINTENANCE_JOURNAL));
      DBBind(stmt, 2, DB_SQLTYPE_INTEGER, objectId);
      DBBind(stmt, 3, DB_SQLTYPE_INTEGER, userId);
      DBBind(stmt, 4, DB_SQLTYPE_INTEGER, userId);
      DBBind(stmt, 5, DB_SQLTYPE_TEXT, description, DB_BIND_STATIC);
      uint32_t now = static_cast<uint32_t>(time(nullptr));
      DBBind(stmt, 6, DB_SQLTYPE_INTEGER, now); // creation time
      DBBind(stmt, 7, DB_SQLTYPE_INTEGER, now); // modification time
      if (DBExecute(stmt))
      {
         NotifyClientSessions(NX_NOTIFY_MAINTENANCE_JOURNAL_UPDATED, objectId);
         success = true;
      }
      DBFreeStatement(stmt);
   }
   DBConnectionPoolReleaseConnection(db);
   return success;
}

/**
 * Add new entry to the object maintenance journal
 */
uint32_t AddMaintenanceJournalRecord(const NXCPMessage& request, uint32_t userId)
{
   uint32_t objectId = request.getFieldAsUInt32(VID_OBJECT_ID);
   TCHAR *description  = request.getFieldAsString(VID_DESCRIPTION);
   uint32_t rcc = AddMaintenanceJournalRecord(objectId, userId, description) ? RCC_SUCCESS : RCC_DB_FAILURE;
   MemFree(description);
   return rcc;
}

/**
 * Edit given maintenance journal entry
 */
uint32_t UpdateMaintenanceJournalRecord(const NXCPMessage& request, uint32_t userId)
{
   uint32_t objectId = request.getFieldAsUInt32(VID_OBJECT_ID);
   uint32_t modificationTime = static_cast<uint32_t>(time(nullptr));

   DB_HANDLE db = DBConnectionPoolAcquireConnection();

   uint32_t rcc;
   DB_STATEMENT stmt = DBPrepare(db,
      (g_dbSyntax == DB_SYNTAX_TSDB) ?
      _T("UPDATE maintenance_journal SET last_edited_by=?, description=?, modification_time=to_timestamp(?) WHERE record_id=?") :
      _T("UPDATE maintenance_journal SET last_edited_by=?, description=?, modification_time=? WHERE record_id=?"));
   if (stmt != nullptr)
   {
      DBBind(stmt, 1, DB_SQLTYPE_INTEGER, userId); // last edited by
      DBBind(stmt, 2, DB_SQLTYPE_TEXT, request.getFieldAsString(VID_DESCRIPTION), DB_BIND_DYNAMIC);
      DBBind(stmt, 3, DB_SQLTYPE_INTEGER, modificationTime);
      DBBind(stmt, 4, DB_SQLTYPE_INTEGER, request.getFieldAsUInt32(VID_RECORD_ID));
      if (DBExecute(stmt))
      {
         rcc = RCC_SUCCESS;
         NotifyClientSessions(NX_NOTIFY_MAINTENANCE_JOURNAL_UPDATED, objectId);
      }
      else
      {
         rcc = RCC_DB_FAILURE;
      }
      DBFreeStatement(stmt);
   }
   else
   {
      rcc = RCC_DB_FAILURE;
   }

   DBConnectionPoolReleaseConnection(db);
   return rcc;
}

/**
 * Maintenance journal record
 */
struct MaintenanceJournalRecord
{
   uint32_t recordId;
   uint32_t objectId;
   uint32_t authorId;
   uint32_t editorId;
   time_t creationTime;
   time_t modificationTime;
   TCHAR *description;

   ~MaintenanceJournalRecord()
   {
      MemFree(description);
   }
};

/**
 * NXSL "MaintenanceJournalRecord" class
 */
NXSL_MaintenanceJournalRecordClass::NXSL_MaintenanceJournalRecordClass()
{
   setName(_T("MaintenanceJournalRecord"));
}

/**
 * NXSL class MaintenanceJournalRecord: get attribute
 */
NXSL_Value *NXSL_MaintenanceJournalRecordClass::getAttr(NXSL_Object *object, const NXSL_Identifier& attr)
{
   NXSL_Value *value = NXSL_Class::getAttr(object, attr);
   if (value != nullptr)
      return value;

   NXSL_VM *vm = object->vm();
   MaintenanceJournalRecord *record = static_cast<MaintenanceJournalRecord*>(object->getData());

   if (NXSL_COMPARE_ATTRIBUTE_NAME("authorId"))
   {
      value = vm->createValue(record->authorId);
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("creationTime"))
   {
      value = vm->createValue(static_cast<int64_t>(record->creationTime));
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("description"))
   {
      value = vm->createValue(record->description);
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("editorId"))
   {
      value = vm->createValue(record->editorId);
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("modificationTime"))
   {
      value = vm->createValue(static_cast<int64_t>(record->modificationTime));
   }
   else if (NXSL_COMPARE_ATTRIBUTE_NAME("objectId"))
   {
      value = vm->createValue(record->objectId);
   }
   return value;
}

/**
 * Object destruction handler for MaintenanceJournalRecord
 */
void NXSL_MaintenanceJournalRecordClass::onObjectDelete(NXSL_Object *object)
{
   delete static_cast<MaintenanceJournalRecord*>(object->getData());
}

/**
 * NXSL class MaintenanceJournalRecord: convert to string
 */
void NXSL_MaintenanceJournalRecordClass::toString(StringBuffer *sb, NXSL_Object *object)
{
   MaintenanceJournalRecord *record = static_cast<MaintenanceJournalRecord*>(object->getData());
   sb->append(_T("MaintenanceJournalRecord { authorId="));
   sb->append(record->authorId);
   sb->append(_T(", creationTime="));
   sb->append(static_cast<int64_t>(record->creationTime));
   sb->append(_T(", editorId="));
   sb->append(record->editorId);
   sb->append(_T(", modificationTime="));
   sb->append(static_cast<int64_t>(record->modificationTime));
   sb->append(_T(", objectId="));
   sb->append(record->objectId);
   sb->append(_T(", description=\""));
   sb->append(record->description);
   sb->append(_T("\" }"));
}

/**
 * Read maintenance journal for use in NXSL
 */
NXSL_Value *ReadMaintenanceJournal(const shared_ptr<NetObj>& object, NXSL_VM *vm, time_t startTime, time_t endTime)
{
   SharedObjectArray<NetObj> sources;
   sources.add(object);
   NXSL_Array *array = new NXSL_Array(vm);
   bool success = ReadMaintenanceJournal(sources, 0, startTime, endTime, [array, vm](DB_RESULT hResult) {
      int numRows = DBGetNumRows(hResult);
      for (int row = 0; row < numRows; row++)
      {
         auto record = new MaintenanceJournalRecord;
         record->recordId = DBGetFieldULong(hResult, row, 0);
         record->objectId = DBGetFieldULong(hResult, row, 1);
         record->authorId = DBGetFieldULong(hResult, row, 2);
         record->editorId = DBGetFieldULong(hResult, row, 3);
         record->description = DBGetField(hResult, row, 4, nullptr, 0);
         record->creationTime = DBGetFieldULong(hResult, row, 5);
         record->modificationTime = DBGetFieldULong(hResult, row, 6);
         array->append(vm->createValue(vm->createObject(&g_nxslMaintenanceJournalRecordClass, record)));
      }
   });

   if (success)
      return vm->createValue(array);

   delete array;
   return vm->createValue();
}
