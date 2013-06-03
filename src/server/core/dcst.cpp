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
** File: dcst.cpp
**
**/

#include "nxcore.h"

/**
 * Modify DCI summary table. Will create new table if id is 0.
 *
 * @return RCC ready to be sent to client
 */
DWORD ModifySummaryTable(CSCPMessage *msg, LONG *newId)
{
   LONG id = msg->GetVariableLong(VID_SUMMARY_TABLE_ID);
   if (id == 0)
   {
      id = CreateUniqueId(IDG_DCI_SUMMARY_TABLE);
   }
   *newId = id;

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   
   DB_STATEMENT hStmt;
   if (IsDatabaseRecordExist(hdb, _T("dci_summary_tables"), _T("id"), (DWORD)id))
   {
      hStmt = DBPrepare(hdb, _T("UPDATE dci_summary_tables SET menu_path=?,title=?,node_filter=?,flags=?,columns=? WHERE id=?"));
   }
   else
   {
      hStmt = DBPrepare(hdb, _T("INSERT INTO dci_summary_tables (menu_path,title,node_filter,flags,columns,id) VALUES (?,?,?,?,?,?)"));
   }

   DWORD rcc;
   if (hStmt != NULL)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, msg->GetVariableStr(VID_MENU_PATH), DB_BIND_DYNAMIC);
      DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, msg->GetVariableStr(VID_TITLE), DB_BIND_DYNAMIC);
      DBBind(hStmt, 3, DB_SQLTYPE_TEXT, msg->GetVariableStr(VID_FILTER), DB_BIND_DYNAMIC);
      DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, msg->GetVariableLong(VID_FLAGS));
      DBBind(hStmt, 5, DB_SQLTYPE_TEXT, msg->GetVariableStr(VID_COLUMNS), DB_BIND_DYNAMIC);
      DBBind(hStmt, 6, DB_SQLTYPE_INTEGER, id);

      rcc = DBExecute(hStmt) ? RCC_SUCCESS : RCC_DB_FAILURE;
      if (rcc == RCC_SUCCESS)
         NotifyClientSessions(NX_NOTIFY_DCISUMTBL_CHANGED, (DWORD)id);

      DBFreeStatement(hStmt);
   }
   else
   {
      rcc = RCC_DB_FAILURE;
   }

   DBConnectionPoolReleaseConnection(hdb);
   return rcc;
}

/**
 * Delete DCI summary table
 */
DWORD DeleteSummaryTable(LONG tableId)
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DWORD rcc = RCC_DB_FAILURE;
   DB_STATEMENT hStmt = DBPrepare(hdb, _T("DELETE FROM dci_summary_tables WHERE id=?"));
   if (hStmt != NULL)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, tableId);
      if (DBExecute(hStmt))
      {
         rcc = RCC_SUCCESS;
         NotifyClientSessions(NX_NOTIFY_DCISUMTBL_DELETED, (DWORD)tableId);
      }
      DBFreeStatement(hStmt);
   }
   DBConnectionPoolReleaseConnection(hdb);
   return rcc;
}

/**
 * Create column definition from configuration string
 */
SummaryTableColumn::SummaryTableColumn(TCHAR *configStr)
{
   TCHAR *ptr = _tcsstr(configStr, _T("^#^"));
   if (ptr != NULL)
   {
      *ptr = 0;
      ptr += 3;
      TCHAR *opt = _tcsstr(ptr, _T("^#^"));
      if (opt != NULL)
      {
         *opt = 0;
         opt += 3;
         m_flags = _tcstoul(opt, NULL, 10);
      }
      else
      {
         m_flags = 0;
      }
      nx_strncpy(m_dciName, ptr, MAX_PARAM_NAME);
   }
   else
   {
      nx_strncpy(m_dciName, configStr, MAX_PARAM_NAME);
      m_flags = 0;
   }
   nx_strncpy(m_name, configStr, MAX_DB_STRING);
}

/**
 * Destructor
 */
SummaryTable::~SummaryTable()
{
   delete m_columns;
   delete m_filter;
}

/**
 * Create object from DB data
 */
SummaryTable::SummaryTable(DB_RESULT hResult)
{
   DBGetField(hResult, 0, 0, m_title, MAX_DB_STRING);
   m_flags = DBGetFieldULong(hResult, 0, 1);

   // Filter script
   TCHAR *filterSource = DBGetField(hResult, 0, 2, NULL, 0);
   if (filterSource != NULL)
   {
      StrStrip(filterSource);
      if (*filterSource != 0)
      {
         TCHAR errorText[1024];
         m_filter = NXSLCompile(filterSource, errorText, 1024);
         if (m_filter == NULL)
         {
            DbgPrintf(4, _T("Error compiling filter script for DCI summary table: %s"), errorText);
         }
      }
      else
      {
         m_filter = NULL;
      }
      free(filterSource);
   }
   else
   {
      m_filter = NULL;
   }

   // Columns
   m_columns = new ObjectArray<SummaryTableColumn>(16, 16, true);
   TCHAR *config = DBGetField(hResult, 0, 3, NULL, 0);
   if ((config != NULL) && (*config != 0))
   {
      TCHAR *curr = config;
      while(curr != NULL)
      {
         TCHAR *next = _tcsstr(curr, _T("^~^"));
         if (next != NULL)
         {
            *next = 0;
            next += 3;
         }
         m_columns->add(new SummaryTableColumn(curr));
         curr = next;
      }
      free(config);
   }
}

/**
 * Load summary table object from database
 */
SummaryTable *SummaryTable::loadFromDB(LONG id, DWORD *rcc)
{
   DbgPrintf(4, _T("Loading configuration for DCI summary table %d"), id);
   SummaryTable *table = NULL;
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT title,flags,node_filter,columns FROM dci_summary_tables WHERE id=?"));
   if (hStmt != NULL)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, id);
      DB_RESULT hResult = DBSelectPrepared(hStmt);
      if (hResult != NULL)
      {
         if (DBGetNumRows(hResult) > 0)
         {
            table = new SummaryTable(hResult);
            *rcc = RCC_SUCCESS;
         }
         else
         {
            *rcc = RCC_INVALID_SUMMARY_TABLE_ID;
         }
         DBFreeResult(hResult);
      }
      else
      {
         *rcc = RCC_DB_FAILURE;
      }
      DBFreeStatement(hStmt);
   }
   else
   {
      *rcc = RCC_DB_FAILURE;
   }
   DBConnectionPoolReleaseConnection(hdb);
   DbgPrintf(4, _T("SummaryTable::loadFromDB(%d): object=%p, rcc=%d"), id, table, (int)*rcc);
   return table;
}

/**
 * Pass node through filter
 */
bool SummaryTable::filter(DataCollectionTarget *object)
{
   if (m_filter == NULL)
      return true;   // no filtering

   bool result = true;
   NXSL_ServerEnv *env = new NXSL_ServerEnv;
   m_filter->setGlobalVariable(_T("$object"), new NXSL_Value(new NXSL_Object(&g_nxslNetObjClass, object)));
   if (object->Type() == OBJECT_NODE)
      m_filter->setGlobalVariable(_T("$node"), new NXSL_Value(new NXSL_Object(&g_nxslNodeClass, object)));
   if (m_filter->run(env) == 0)
   {
      NXSL_Value *value = m_filter->getResult();
      if (value != NULL)
      {
         result = value->getValueAsInt32() ? true : false;
      }
      else
      {
         DbgPrintf(4, _T("Error executing filter script for DCI summary table: %s"), m_filter->getErrorText());
      }
   }
   return result;
}

/**
 * Create empty result table
 */
Table *SummaryTable::createEmptyResultTable()
{
   Table *result = new Table();
   result->setTitle(m_title);
   result->addColumn(_T("Node"), DCI_DT_STRING);
   for(int i = 0; i < m_columns->size(); i++)
   {
      result->addColumn(m_columns->get(i)->m_name, DCI_DT_STRING);
   }
   return result;
}

/**
 * Query summary table
 */
Table *QuerySummaryTable(LONG tableId, DWORD baseObjectId, DWORD userId, DWORD *rcc)
{
   NetObj *object = FindObjectById(baseObjectId);
   if (object == NULL)
   {
      *rcc = RCC_INVALID_OBJECT_ID;
      return NULL;
   }
   if (!object->checkAccessRights(userId, OBJECT_ACCESS_READ))
   {
      *rcc = RCC_ACCESS_DENIED;
      return NULL;
   }
   if ((object->Type() != OBJECT_CONTAINER) && (object->Type() != OBJECT_CLUSTER) &&
       (object->Type() != OBJECT_SERVICEROOT) && (object->Type() != OBJECT_SUBNET) &&
       (object->Type() != OBJECT_ZONE) && (object->Type() != OBJECT_NETWORK))
   {
      *rcc = RCC_INCOMPATIBLE_OPERATION;
      return NULL;
   }

   SummaryTable *tableDefinition = SummaryTable::loadFromDB(tableId, rcc);
   if (tableDefinition == NULL)
      return NULL;

   ObjectArray<NetObj> *childObjects = object->getFullChildList(true);
   Table *tableData = tableDefinition->createEmptyResultTable();

   for(int i = 0; i < childObjects->size(); i++)
   {
      if (((childObjects->get(i)->Type() != OBJECT_NODE) && (childObjects->get(i)->Type() != OBJECT_MOBILEDEVICE)) || 
          !childObjects->get(i)->checkAccessRights(userId, OBJECT_ACCESS_READ))
         continue;

      DataCollectionTarget *dct = (DataCollectionTarget *)childObjects->get(i);
      if (tableDefinition->filter(dct))
      {
         dct->getLastValuesSummary(tableDefinition, tableData);
      }
   }

   delete childObjects;
   delete tableDefinition;

   return tableData;
}
