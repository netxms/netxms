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
** File: dci_recalc.cpp
**
**/

#include "nxcore.h"

/**
 * Recalculation job constructor
 */
DCIRecalculationJob::DCIRecalculationJob(DataCollectionTarget *object, DCItem *dci, UINT32 userId)
                    : ServerJob(_T("DCI_RECALC"), _T("Recalculate DCI values"), object->getId(), userId, false)
{
   m_object = object;
   m_object->incRefCount();
   m_dci = new DCItem(dci, true);
   m_cancelled = false;

   TCHAR buffer[1024];
   _sntprintf(buffer, 1024, _T("Recalculate values for DCI \"%s\" on %s"), dci->getDescription(), object->getName());
   setDescription(buffer);
}

/**
 * Recalculation job destructor
 */
DCIRecalculationJob::~DCIRecalculationJob()
{
   m_object->decRefCount();
   delete m_dci;
}

/**
 * Run job
 */
ServerJobResult DCIRecalculationJob::run()
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   TCHAR query[256];
   if (g_flags & AF_SINGLE_TABLE_PERF_DATA)
      _sntprintf(query, 256, _T("SELECT idata_timestamp,raw_value FROM idata WHERE node_id=%d AND item_id=%d ORDER BY idata_timestamp"), m_object->getId(), m_dci->getId());
   else
      _sntprintf(query, 256, _T("SELECT idata_timestamp,raw_value FROM idata_%d WHERE item_id=%d ORDER BY idata_timestamp"), m_object->getId(), m_dci->getId());
   DB_RESULT hResult = DBSelect(hdb, query);
   if (hResult == NULL)
   {
      DBConnectionPoolReleaseConnection(hdb);
      return JOB_RESULT_FAILED;
   }

   bool success = true;
   int count = DBGetNumRows(hResult);
   if (count > 0)
   {
      DB_STATEMENT hStmt;
      if (g_flags & AF_SINGLE_TABLE_PERF_DATA)
      {
         hStmt = DBPrepare(hdb, _T("UPDATE idata SET idata_value=? WHERE node_id=? AND item_id=? AND idata_timestamp=?"));
      }
      else
      {
         _sntprintf(query, 256, _T("UPDATE idata_%d SET idata_value=? WHERE item_id=? AND idata_timestamp=?"), m_object->getId());
         hStmt = DBPrepare(hdb, query);
      }
      if (hStmt != NULL)
      {
         m_dci->prepareForRecalc();
         DBBegin(hdb);
         for(int i = 0; (i < count) && !m_cancelled; i++)
         {
            time_t timestamp = static_cast<time_t>(DBGetFieldInt64(hResult, i, 0));
            TCHAR data[MAX_RESULT_LENGTH];
            DBGetField(hResult, i, 1, data, MAX_RESULT_LENGTH);
            ItemValue value(data, timestamp);
            m_dci->recalculateValue(value);

            int index = 1;
            DBBind(hStmt, index++, DB_SQLTYPE_VARCHAR, value.getString(), DB_BIND_STATIC);
            if (g_flags & AF_SINGLE_TABLE_PERF_DATA)
               DBBind(hStmt, index++, DB_SQLTYPE_INTEGER, m_object->getId());
            DBBind(hStmt, index++, DB_SQLTYPE_INTEGER, m_dci->getId());
            DBBind(hStmt, index++, DB_SQLTYPE_INTEGER, (UINT32)value.getTimeStamp());
            DBExecute(hStmt);

            if (i % 10 == 0)
            {
               markProgress(i * 100 / count);
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
      m_object->reloadDCItemCache(m_dci->getId());
      markProgress(100);
   }
   return success ? JOB_RESULT_SUCCESS : JOB_RESULT_FAILED;
}

/**
 * Cancel job
 */
bool DCIRecalculationJob::onCancel()
{
   m_cancelled = true;
   return true;
}
