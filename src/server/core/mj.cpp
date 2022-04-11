/*
** NetXMS - Network Management System
** Copyright (C) 2022 Raden Solutions
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
 * Appends "* FROM maintenance_journal WHERE object_id IN [sources] ORDER BY id DESC" to query
 */
static void buildSelectQueryMiddlePart(StringBuffer* query, SharedObjectArray<NetObj>& sources)
{
   query->append(_T(" record_id, object_id, author, last_edited_by, description, creation_time, modification_time FROM maintenance_journal WHERE object_id IN ("));
   query->append(sources.get(0)->getId());

   if (sources.size() > 1)
   {
      for (shared_ptr<NetObj> childObj : sources)
      {
         query->append(_T(","));
         query->append(childObj->getId());
      }
   }

   query->append(_T(") ORDER BY record_id DESC"));
}

/**
 * Get all maintenance journal entries for the given object
 */
uint32_t MaintenanceJournalRead(SharedObjectArray<NetObj>& sources, NXCPMessage* response, uint32_t maxEntries)
{
   DB_HANDLE db = DBConnectionPoolAcquireConnection();

   StringBuffer query;
   if (maxEntries == 0)
   {
      query.append(_T("SELECT "));
      buildSelectQueryMiddlePart(&query, sources);
   }
   else
   {
      switch (g_dbSyntax)
      {
         case DB_SYNTAX_MSSQL:
            query.append(_T("SELECT TOP "));
            query.append(maxEntries);
            buildSelectQueryMiddlePart(&query, sources);
            break;
         case DB_SYNTAX_ORACLE:
            query.append(_T("SELECT * FROM (SELECT "));
            buildSelectQueryMiddlePart(&query, sources);
            query.append(_T(") WHERE ROWNUM <= "));
            query.append(maxEntries);
            break;
         case DB_SYNTAX_DB2:
            query.append(_T("SELECT "));
            buildSelectQueryMiddlePart(&query, sources);
            query.append(_T(" FETCH FIRST "));
            query.append(maxEntries);
            query.append(_T(" ROWS ONLY"));
            break;
         default:
            query.append(_T("SELECT "));
            buildSelectQueryMiddlePart(&query, sources);
            query.append(_T(" LIMIT "));
            query.append(maxEntries);
            break;
      }
   }

   uint32_t rcc;
   DB_RESULT result = DBSelect(db, query);
   if (result != nullptr)
   {
      int numRows = DBGetNumRows(result);
      response->setField(VID_NUM_ELEMENTS, numRows);

      uint32_t base = VID_ELEMENT_LIST_BASE;
      for (int row = 0; row < numRows; row++, base += 10)
      {
         response->setField(base, DBGetFieldULong(result, row, 0));              // id
         response->setField(base + 1, DBGetFieldULong(result, row, 1));          // object_id
         response->setField(base + 2, DBGetFieldULong(result, row, 2));          // author
         response->setField(base + 3, DBGetFieldULong(result, row, 3));          // last_edited_by
         response->setField(base + 4, DBGetFieldAsSharedString(result, row, 4)); // description
         response->setField(base + 5, DBGetFieldULong(result, row, 5));          // creation_time
         response->setField(base + 6, DBGetFieldULong(result, row, 6));          // modification_time
      }
      DBFreeResult(result);

      rcc = RCC_SUCCESS;
   }
   else
   {
      rcc = RCC_DB_FAILURE;
   }

   DBConnectionPoolReleaseConnection(db);
   return rcc;
}

/**
 * Create new entry in given the object maintenance journal
 */
uint32_t MaintenanceJournalCreate(const NXCPMessage& request, uint32_t userId)
{
   uint32_t objectId = request.getFieldAsUInt32(VID_OBJECT_ID);

   uint32_t rcc;
   DB_HANDLE db = DBConnectionPoolAcquireConnection();
   DB_STATEMENT stmt = DBPrepare(db, _T("INSERT INTO maintenance_journal (record_id, object_id, author, last_edited_by, description, creation_time, modification_time) VALUES (?, ?, ?, ?, ?, ?, ?)"));
   if (stmt != nullptr)
   {
      DBBind(stmt, 1, DB_SQLTYPE_INTEGER, CreateUniqueId(IDG_MAINTENANCE_JOURNAL));                 // id
      DBBind(stmt, 2, DB_SQLTYPE_INTEGER, objectId);                                                // source object id
      DBBind(stmt, 3, DB_SQLTYPE_INTEGER, userId);                                                  // author
      DBBind(stmt, 4, DB_SQLTYPE_INTEGER, userId);                                                  // last edited by
      DBBind(stmt, 5, DB_SQLTYPE_TEXT, request.getFieldAsString(VID_DESCRIPTION), DB_BIND_DYNAMIC); // description
      uint32_t now = static_cast<uint32_t>(time(nullptr));
      DBBind(stmt, 6, DB_SQLTYPE_INTEGER, now); // creation time
      DBBind(stmt, 7, DB_SQLTYPE_INTEGER, now); // modification time
      if (DBExecute(stmt))
      {
         NotifyClientSessions(NX_NOTIFY_MAINTENANCE_JOURNAL_UPDATED, objectId);
         rcc = RCC_SUCCESS;
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
 * Edit given maintenance journal entry
 */
uint32_t MaintenanceJournalEdit(const NXCPMessage& request, uint32_t userId)
{
   uint32_t objectId = request.getFieldAsUInt32(VID_OBJECT_ID);
   uint32_t modificationTime = static_cast<uint32_t>(time(nullptr));

   DB_HANDLE db = DBConnectionPoolAcquireConnection();

   uint32_t rcc;
   DB_STATEMENT stmt = DBPrepare(db, _T("UPDATE maintenance_journal SET last_edited_by=?, description=?, modification_time=? WHERE record_id=?"));
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
