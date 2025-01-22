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
** File: dci_recalc.cpp
**/

#include "nxcore.h"
#include <nxtask.h>

/**
 * Recalculate DCI values
 */
bool RecalculateDCIValues(DataCollectionTarget *object, DCItem *dci, BackgroundTask *task)
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   TCHAR query[256];
   if (g_flags & AF_SINGLE_TABLE_PERF_DATA)
   {
      if (g_dbSyntax == DB_SYNTAX_TSDB)
      {
         _sntprintf(query, 256, _T("SELECT date_part('epoch',idata_timestamp)::int,raw_value FROM idata_sc_%s WHERE node_id=%d AND item_id=%d ORDER BY idata_timestamp"),
                  DCObject::getStorageClassName(dci->getStorageClass()), object->getId(), dci->getId());
      }
      else
      {
         _sntprintf(query, 256, _T("SELECT idata_timestamp,raw_value FROM idata WHERE node_id=%d AND item_id=%d ORDER BY idata_timestamp"), object->getId(), dci->getId());
      }
   }
   else
   {
      _sntprintf(query, 256, _T("SELECT idata_timestamp,raw_value FROM idata_%d WHERE item_id=%d ORDER BY idata_timestamp"), object->getId(), dci->getId());
   }
   DB_RESULT hResult = DBSelect(hdb, query);
   if (hResult == nullptr)
   {
      DBConnectionPoolReleaseConnection(hdb);
      return false;
   }

   bool success = true;
   int count = DBGetNumRows(hResult);
   if (count > 0)
   {
      DB_STATEMENT hStmt;
      if (g_flags & AF_SINGLE_TABLE_PERF_DATA)
      {
         if (g_dbSyntax == DB_SYNTAX_TSDB)
         {
            TCHAR query[256];
            _sntprintf(query, 256, _T("UPDATE idata_sc_%s SET idata_value=? WHERE node_id=? AND item_id=? AND idata_timestamp=to_timestamp(?)"),
                     DCObject::getStorageClassName(dci->getStorageClass()));
            hStmt = DBPrepare(hdb, query);
         }
         else
         {
            hStmt = DBPrepare(hdb, _T("UPDATE idata SET idata_value=? WHERE node_id=? AND item_id=? AND idata_timestamp=?"));
         }
      }
      else
      {
         _sntprintf(query, 256, _T("UPDATE idata_%d SET idata_value=? WHERE item_id=? AND idata_timestamp=?"), object->getId());
         hStmt = DBPrepare(hdb, query);
      }
      if (hStmt != nullptr)
      {
         dci->prepareForRecalc();
         DBBegin(hdb);
         for(int i = 0; i < count; i++)
         {
            time_t timestamp = static_cast<time_t>(DBGetFieldInt64(hResult, i, 0));
            ItemValue value(hResult, i, 1, timestamp, false);
            dci->recalculateValue(value);

            int index = 1;
            DBBind(hStmt, index++, DB_SQLTYPE_VARCHAR, value.getString(), DB_BIND_STATIC);
            if (g_flags & AF_SINGLE_TABLE_PERF_DATA)
               DBBind(hStmt, index++, DB_SQLTYPE_INTEGER, object->getId());
            DBBind(hStmt, index++, DB_SQLTYPE_INTEGER, dci->getId());
            DBBind(hStmt, index++, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(value.getTimeStamp()));
            DBExecute(hStmt);

            if (i % 10 == 0)
            {
               task->markProgress(i * 100 / count);
            }

            if (i % 1000 == 0)
            {
               DBCommit(hdb);
               DBBegin(hdb);
            }
         }
         DBCommit(hdb);
         DBFreeStatement(hStmt);
      }
      else
      {
         success = false;
      }
   }

   DBFreeResult(hResult);
   DBConnectionPoolReleaseConnection(hdb);

   if (success)
   {
      object->reloadDCItemCache(dci->getId());
      task->markProgress(100);
   }
   return success;
}
