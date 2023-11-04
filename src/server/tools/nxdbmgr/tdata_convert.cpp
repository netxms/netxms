/*
** nxdbmgr - NetXMS database manager
** Copyright (C) 2003-2023 Victor Kirhenshtein
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
**/

#include "nxdbmgr.h"

/**
 * Read table object from tdata_* tables
 */
static Table *ReadTable(UINT64 recordId, UINT32 tableId, UINT32 objectId)
{
   TCHAR query[1024];
   _sntprintf(query, 1024,
       _T("SELECT r.row_id,w.column_id,n.column_name,c.flags,c.display_name,w.value FROM tdata_records_%d r ")
       _T("INNER JOIN tdata_rows_%d w ON w.row_id = r.row_id ")
       _T("INNER JOIN dct_column_names n ON n.column_id=w.column_id ")
       _T("LEFT OUTER JOIN dc_table_columns c ON c.table_id=%d AND c.column_name=n.column_name ")
       _T("WHERE r.record_id=") UINT64_FMT _T(" ")
       _T("ORDER BY r.row_id"), objectId, objectId, tableId, recordId);
   DB_RESULT hResult = DBSelect(g_dbHandle, query);
   if (hResult == NULL)
      return NULL;

   int count = DBGetNumRows(hResult);
   Table *table = NULL;
   if (count > 0)
   {
      table = new Table();
      UINT64 currRowId = 0;
      for(int i = 0; i < count; i++)
      {
         TCHAR columnName[MAX_COLUMN_NAME];
         DBGetField(hResult, i, 2, columnName, MAX_COLUMN_NAME);
         int columnIndex = table->getColumnIndex(columnName);
         if (columnIndex == -1)
         {
            TCHAR displayName[256];
            DBGetField(hResult, i, 4, displayName, 256);
            UINT16 flags = (UINT16)DBGetFieldULong(hResult, i, 3);
            columnIndex = table->addColumn(columnName, TCF_GET_DATA_TYPE(flags), displayName, (flags & TCF_INSTANCE_COLUMN) ? true : false);
         }

         UINT64 rowId = DBGetFieldUInt64(hResult, i, 0);
         if (rowId != currRowId)
         {
            currRowId = rowId;
            table->addRow();
         }

         TCHAR value[MAX_RESULT_LENGTH];
         DBGetField(hResult, i, 5, value, MAX_RESULT_LENGTH);
         table->set(columnIndex, value);
      }
   }
   DBFreeResult(hResult);
   return table;
}

/**
 * Convert tdata table for given object
 */
static bool ConvertTData(UINT32 id, int *skippedRecords)
{
   bool success = false;
   int total = 0x07FFFFFF;
   TCHAR query[256];
   _sntprintf(query, 256, _T("SELECT count(*) FROM tdata_temp_%d"), id);
   DB_RESULT hCountResult = DBSelect(g_dbHandle, query);
   if (hCountResult != NULL)
   {
      total = DBGetFieldLong(hCountResult, 0, 0);
      if (total <= 0)
         total = 0x07FFFFFF;
      DBFreeResult(hCountResult);
   }

   // Open second connection to database to allow unbuffered query in parallel with inserts
   DB_HANDLE hdb = ConnectToDatabase();
   if (hdb != nullptr)
   {
      _sntprintf(query, 256, _T("SELECT item_id,tdata_timestamp,record_id FROM tdata_temp_%d"), id);
      DB_UNBUFFERED_RESULT hResult = DBSelectUnbuffered(hdb, query);
      if (hResult != nullptr)
      {
         _sntprintf(query, 256, _T("INSERT INTO tdata_%d (item_id,tdata_timestamp,tdata_value) VALUES (?,?,?)"), id);
         DB_STATEMENT hStmt = DBPrepare(g_dbHandle, query);
         if (hStmt != nullptr)
         {
            success = true;
            int converted = 0;
            int skipped = 0;
            DBBegin(g_dbHandle);
            while(DBFetch(hResult))
            {
               UINT32 tableId = DBGetFieldULong(hResult, 0);
               UINT32 timestamp = DBGetFieldULong(hResult, 1);
               UINT64 recordId = DBGetFieldUInt64(hResult, 2);
               Table *value = ReadTable(recordId, tableId, id);
               if (value != NULL)
               {
                  DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, tableId);
                  DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, timestamp);
                  DBBind(hStmt, 3, DB_SQLTYPE_TEXT, DB_CTYPE_UTF8_STRING, value->toPackedXML(), DB_BIND_DYNAMIC);
                  if (!SQLExecute(hStmt))
                  {
                     delete value;
                     success = false;
                     break;
                  }
                  delete value;
               }
               else
               {
                  skipped++;
               }

               converted++;
               if (converted % 100 == 0)
               {
                  int pct = (converted * 100) / total;
                  if (pct > 100)
                     pct = 100;
                  WriteToTerminalEx(_T("\b\b\b\b%3d%%"), pct);
                  fflush(stdout);
                  DBCommit(g_dbHandle);
                  DBBegin(g_dbHandle);
               }
            }
            DBCommit(g_dbHandle);
            DBFreeStatement(hStmt);
            *skippedRecords = skipped;
         }
         DBFreeResult(hResult);
      }
      DBDisconnect(hdb);
   }

   if (success)
   {
      TCHAR query[256];
      _sntprintf(query, 256, _T("DROP TABLE tdata_rows_%d"), id);
      SQLQuery(query);
      _sntprintf(query, 256, _T("DROP TABLE tdata_records_%d"), id);
      SQLQuery(query);
      _sntprintf(query, 256, _T("DROP TABLE tdata_temp_%d"), id);
      SQLQuery(query);
   }

   return success;
}

/**
 * Check data tables for given object class
 */
static bool ConvertTDataForClass(const TCHAR *className, int stage)
{
   bool success = false;
   TCHAR query[1024];
   _sntprintf(query, 256, _T("SELECT id FROM %s"), className);
   DB_RESULT hResult = SQLSelect(query);
   if (hResult != NULL)
   {
      success = true;
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         uint32_t id = DBGetFieldULong(hResult, i, 0);
         if (stage == 1)
         {
            if (IsDataTableExist(_T("tdata_%u"), id))
            {
               TCHAR oldName[32], newName[32];
               _sntprintf(oldName, 32, _T("tdata_%d"), id);
               _sntprintf(newName, 32, _T("tdata_temp_%d"), id);
               CHK_EXEC_NO_SP(DBRenameTable(g_dbHandle, oldName, newName));
            }
            CreateTDataTable(id);
         }
         else if ((stage == 2) && IsDataTableExist(_T("tdata_temp_%u"), id))
         {
            WriteToTerminalEx(_T("Converting table \x1b[1mtdata_%d\x1b[0m:   0%%"), id);
            fflush(stdout);
            int skippedRecords = 0;
            if (ConvertTData(id, &skippedRecords))
            {
               if (skippedRecords == 0)
                  WriteToTerminalEx(_T("\b\b\b\b\x1b[32;1mdone\x1b[0m\n"));
               else
                  WriteToTerminalEx(_T("\b\b\b\b\x1b[33;1mdone with %d records skipped\x1b[0m\n"), skippedRecords);
            }
            else
            {
               WriteToTerminalEx(_T("\b\b\b\b\x1b[31;1mfailed\x1b[0m\n"));
               success = false;
               if (g_ignoreErrors)
                  continue;
               else
                  break;
            }
         }
      }
      DBFreeResult(hResult);
   }
   return success;
}

/**
 * Convert tdata tables into new format
 */
bool ConvertTDataTables(int stage)
{
   CHK_EXEC_NO_SP(ConvertTDataForClass(_T("nodes"), stage));
   CHK_EXEC_NO_SP(ConvertTDataForClass(_T("clusters"), stage));
   CHK_EXEC_NO_SP(ConvertTDataForClass(_T("mobile_devices"), stage));
   CHK_EXEC_NO_SP(ConvertTDataForClass(_T("access_points"), stage));
   CHK_EXEC_NO_SP(ConvertTDataForClass(_T("chassis"), stage));
   return true;
}
