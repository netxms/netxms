/*
** NetXMS - Network Management System
** Copyright (C) 2003-2024 Victor Kirhenshtein
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

#define DEBUG_TAG _T("dc.summarytable")

/**
 * Modify DCI summary table. Will create new table if id is 0.
 *
 * @return RCC ready to be sent to client
 */
uint32_t ModifySummaryTable(const NXCPMessage& msg, uint32_t *newId)
{
   uint32_t id = msg.getFieldAsUInt32(VID_SUMMARY_TABLE_ID);
   if (id == 0)
   {
      id = CreateUniqueId(IDG_DCI_SUMMARY_TABLE);
   }
   *newId = id;

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   bool isNew = !IsDatabaseRecordExist(hdb, _T("dci_summary_tables"), _T("id"), id);
   DB_STATEMENT hStmt;
   if (isNew)
   {
      hStmt = DBPrepare(hdb, _T("INSERT INTO dci_summary_tables (menu_path,title,node_filter,flags,columns,table_dci_name,id,guid) VALUES (?,?,?,?,?,?,?,?)"));
   }
   else
   {
      hStmt = DBPrepare(hdb, _T("UPDATE dci_summary_tables SET menu_path=?,title=?,node_filter=?,flags=?,columns=?,table_dci_name=? WHERE id=?"));
   }

   uint32_t rcc;
   if (hStmt != nullptr)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, msg.getFieldAsString(VID_MENU_PATH), DB_BIND_DYNAMIC);
      DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, msg.getFieldAsString(VID_TITLE), DB_BIND_DYNAMIC);
      DBBind(hStmt, 3, DB_SQLTYPE_TEXT, msg.getFieldAsString(VID_FILTER), DB_BIND_DYNAMIC);
      DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, msg.getFieldAsUInt32(VID_FLAGS));
      DBBind(hStmt, 5, DB_SQLTYPE_TEXT, msg.getFieldAsString(VID_COLUMNS), DB_BIND_DYNAMIC);
      DBBind(hStmt, 6, DB_SQLTYPE_VARCHAR, msg.getFieldAsString(VID_DCI_NAME), DB_BIND_DYNAMIC);
      DBBind(hStmt, 7, DB_SQLTYPE_INTEGER, id);

      if (isNew)
      {
         DBBind(hStmt, 8, DB_SQLTYPE_VARCHAR, uuid::generate());
      }

      rcc = DBExecute(hStmt) ? RCC_SUCCESS : RCC_DB_FAILURE;
      if (rcc == RCC_SUCCESS)
         NotifyClientSessions(NX_NOTIFY_DCISUMTBL_CHANGED, (UINT32)id);

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
uint32_t NXCORE_EXPORTABLE DeleteSummaryTable(uint32_t tableId)
{
   uint32_t rcc;
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   if (ExecuteQueryOnObject(hdb, tableId, _T("DELETE FROM dci_summary_tables WHERE id=?")))
   {
      rcc = RCC_SUCCESS;
      NotifyClientSessions(NX_NOTIFY_DCISUMTBL_DELETED, tableId);
   }
   else
   {
      rcc = RCC_DB_FAILURE;
   }
   DBConnectionPoolReleaseConnection(hdb);
   return rcc;
}

/**
 * Create column definition from NXCP message
 */
SummaryTableColumn::SummaryTableColumn(const NXCPMessage& msg, uint32_t baseId)
{
   msg.getFieldAsString(baseId, m_name, MAX_DB_STRING);
   msg.getFieldAsString(baseId + 1, m_dciName, MAX_PARAM_NAME);
   m_flags = msg.getFieldAsUInt32(baseId + 2);
   if (msg.isFieldExist(baseId + 3))
      msg.getFieldAsString(baseId + 3, m_separator, 16);
   else
      _tcscpy(m_separator, _T(";"));
}

/**
 * Create column definition from JSON document
 */
SummaryTableColumn::SummaryTableColumn(json_t *json)
{
   utf8_to_tchar(json_object_get_string_utf8(json, "name", ""), -1, m_name, MAX_DB_STRING);
   utf8_to_tchar(json_object_get_string_utf8(json, "dciName", ""), -1, m_dciName, MAX_PARAM_NAME);
   m_flags = json_object_get_uint32(json, "flags", 0);
   utf8_to_tchar(json_object_get_string_utf8(json, "separator", ";"), -1, m_separator, 16);
}

/**
 * Create column definition from configuration string
 */
SummaryTableColumn::SummaryTableColumn(TCHAR *configStr)
{
   TCHAR *ptr = _tcsstr(configStr, _T("^#^"));
   if (ptr != nullptr)
   {
      *ptr = 0;
      ptr += 3;
      TCHAR *opt = _tcsstr(ptr, _T("^#^"));
      if (opt != nullptr)
      {
         *opt = 0;
         opt += 3;
         TCHAR *sep = _tcsstr(opt, _T("^#^"));
         if (sep != nullptr)
         {
            *sep = 0;
            sep += 3;
            _tcslcpy(m_separator, sep, 16);
         }
         else
         {
            _tcscpy(m_separator, _T(";"));
         }
         m_flags = _tcstoul(opt, nullptr, 10);
      }
      else
      {
         m_flags = 0;
      }
      _tcslcpy(m_dciName, ptr, MAX_PARAM_NAME);
   }
   else
   {
      _tcslcpy(m_dciName, configStr, MAX_PARAM_NAME);
      m_flags = 0;
   }
   _tcslcpy(m_name, configStr, MAX_DB_STRING);
}

/**
 * Create export record for column
 */
void SummaryTableColumn::createExportRecord(TextFileWriter& xml, uint32_t id) const
{
   xml.appendUtf8String("\t\t\t\t<column id=\"");
   xml.append(id);
   xml.appendUtf8String("\">\n\t\t\t\t\t<name>");
   xml.append(EscapeStringForXML2(m_name));
   xml.appendUtf8String("</name>\n\t\t\t\t\t<dci>");
   xml.append(EscapeStringForXML2(m_dciName));
   xml.appendUtf8String("</dci>\n\t\t\t\t\t<flags>");
   xml.append(m_flags);
   xml.appendUtf8String("</flags>\n\t\t\t\t\t<separator>");
   xml.append(m_separator);
   xml.appendUtf8String("</separator>\n\t\t\t\t</column>\n");
}

/**
 * Create ad-hoc summary table definition from NXCP message
 */
SummaryTable::SummaryTable(const NXCPMessage& msg) : m_columns(msg.getFieldAsInt32(VID_NUM_COLUMNS), 16, Ownership::True)
{
   m_id = 0;
   m_guid = uuid::generate();
   m_title[0] = 0;
   m_menuPath[0] = 0;
   m_flags = msg.getFieldAsUInt32(VID_FLAGS);
   m_filterSource = nullptr;
   m_filter = nullptr;
   m_aggregationFunction = (AggregationFunction)msg.getFieldAsInt16(VID_FUNCTION);
   m_periodStart = msg.getFieldAsTime(VID_TIME_FROM);
   m_periodEnd = msg.getFieldAsTime(VID_TIME_TO);
   msg.getFieldAsString(VID_DCI_NAME, m_tableDciName, MAX_PARAM_NAME);

   uint32_t id = VID_COLUMN_INFO_BASE;
   int count = msg.getFieldAsInt32(VID_NUM_COLUMNS);
   for(int i = 0; i < count; i++)
   {
      m_columns.add(new SummaryTableColumn(msg, id));
      id += 10;
   }
}

/**
 * Create ad-hoc summary table definition from JSON object
 */
SummaryTable::SummaryTable(json_t *json) : m_columns(16, 16, Ownership::True)
{
   m_id = 0;
   m_guid = uuid::generate();
   m_title[0] = 0;
   m_menuPath[0] = 0;
   m_flags = json_object_get_uint32(json, "flags", 0);
   m_filterSource = nullptr;
   m_filter = nullptr;
   m_aggregationFunction = (AggregationFunction)json_object_get_int32(json, "function", DCI_AGG_LAST);
   m_periodStart = json_object_get_time(json, "periodStart");
   m_periodEnd = json_object_get_time(json, "periodEnd");
   utf8_to_tchar(json_object_get_string_utf8(json, "dciName", ""), -1, m_tableDciName, MAX_PARAM_NAME);

   json_t *columns = json_object_get(json, "columns");
   if (json_is_array(columns))
   {
      size_t i;
      json_t *column;
      json_array_foreach(columns, i, column)
      {
         if (json_is_object(column))
            m_columns.add(new SummaryTableColumn(column));
      }
   }
}

/**
 * Create summary table definition from DB data
 */
SummaryTable::SummaryTable(uint32_t id, DB_RESULT hResult) : m_columns(16, 16, Ownership::True)
{
   m_id = id;

   DBGetField(hResult, 0, 0, m_title, MAX_DB_STRING);
   m_flags = DBGetFieldULong(hResult, 0, 1);
   m_guid = DBGetFieldGUID(hResult, 0, 2);
   DBGetField(hResult, 0, 3, m_menuPath, MAX_DB_STRING);

   m_aggregationFunction = DCI_AGG_LAST;
   m_periodStart = 0;
   m_periodEnd = 0;

   // Filter script
   m_filterSource = DBGetField(hResult, 0, 4, nullptr, 0);
   if (m_filterSource != nullptr)
   {
      Trim(m_filterSource);
      if (*m_filterSource != 0)
      {
         NXSL_CompilationDiagnostic diag;
         m_filter = NXSLCompileAndCreateVM(m_filterSource, new NXSL_ServerEnv(), &diag);
         if (m_filter == nullptr)
         {
            nxlog_debug_tag(DEBUG_TAG, 4, _T("Error compiling filter script for DCI summary table (%s)"), diag.errorText.cstr());
         }
      }
      else
      {
         m_filter = nullptr;
      }
   }
   else
   {
      m_filter = nullptr;
   }

   // Columns
   TCHAR *config = DBGetField(hResult, 0, 5, nullptr, 0);
   if ((config != nullptr) && (*config != 0))
   {
      TCHAR *curr = config;
      while(curr != nullptr)
      {
         TCHAR *next = _tcsstr(curr, _T("^~^"));
         if (next != nullptr)
         {
            *next = 0;
            next += 3;
         }
         m_columns.add(new SummaryTableColumn(curr));
         curr = next;
      }
      MemFree(config);
   }
   DBGetField(hResult, 0, 6, m_tableDciName, MAX_PARAM_NAME);
}

/**
 * Load summary table object from database
 */
SummaryTable *SummaryTable::loadFromDB(uint32_t id, uint32_t *rcc)
{
   nxlog_debug_tag(DEBUG_TAG, 4, _T("Loading configuration for DCI summary table [%u]"), id);
   SummaryTable *table = nullptr;
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT title,flags,guid,menu_path,node_filter,columns,table_dci_name FROM dci_summary_tables WHERE id=?"));
   if (hStmt != nullptr)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, id);
      DB_RESULT hResult = DBSelectPrepared(hStmt);
      if (hResult != nullptr)
      {
         if (DBGetNumRows(hResult) > 0)
         {
            table = new SummaryTable(id, hResult);
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
   nxlog_debug_tag(DEBUG_TAG, 4, _T("SummaryTable::loadFromDB([%u]): object=%p, rcc=%u"), id, table, *rcc);
   return table;
}

/**
 * Destructor
 */
SummaryTable::~SummaryTable()
{
   delete m_filter;
   MemFree(m_filterSource);
}

/**
 * Pass node through filter
 */
bool SummaryTable::filter(const shared_ptr<DataCollectionTarget>& object)
{
   if (m_filter == nullptr)
      return true;   // no filtering

   bool result = true;
   SetupServerScriptVM(m_filter, object, shared_ptr<DCObjectInfo>());
   if (m_filter->run())
   {
      result = m_filter->getResult()->getValueAsBoolean();
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Error executing filter script for DCI summary table [%u]: %s"), m_id, m_filter->getErrorText());
   }
   return result;
}

/**
 * Create empty result table
 */
Table *SummaryTable::createEmptyResultTable() const
{
   Table *result = new Table();
   result->setTitle(m_title);
   result->setExtendedFormat(true);
   result->addColumn(_T("Node"), DCI_DT_STRING);
   if (m_flags & SUMMARY_TABLE_MULTI_INSTANCE)
      result->addColumn(_T("Instance"), DCI_DT_STRING);

   if (!(m_flags & SUMMARY_TABLE_TABLE_DCI_SOURCE))
   {
      for(int i = 0; i < m_columns.size(); i++)
      {
         result->addColumn(m_columns.get(i)->m_name, DCI_DT_STRING);
      }
   }
   return result;
}

/**
 * Create export record
 */
void SummaryTable::createExportRecord(TextFileWriter& xml) const
{
   xml.appendUtf8String("\t\t<table id=\"");
   xml.append(m_id);
   xml.appendUtf8String("\">\n\t\t\t<guid>");
   xml.append(m_guid);
   xml.appendUtf8String("</guid>\n\t\t\t<title>");
   xml.append(EscapeStringForXML2(m_title));
   xml.appendUtf8String("</title>\n\t\t\t<flags>");
   xml.append(m_flags);
   xml.appendUtf8String("</flags>\n\t\t\t<path>");
   xml.append(EscapeStringForXML2(m_menuPath));
   xml.appendUtf8String("</path>\n\t\t\t<filter>");
   xml.append(EscapeStringForXML2(m_filterSource));
   xml.appendUtf8String("</filter>\n\t\t\t<tableDci>");
   xml.append(EscapeStringForXML2(m_tableDciName));
   xml.appendUtf8String("</tableDci>\n\t\t\t<columns>\n");
   for(int i = 0; i < m_columns.size(); i++)
   {
      m_columns.get(i)->createExportRecord(xml, i + 1);
   }
   xml.appendUtf8String("\t\t\t</columns>\n\t\t</table>\n");
}

/**
 * Query summary table. If ad-hoc definition is provided it will be deleted by this function.
 */
Table NXCORE_EXPORTABLE *QuerySummaryTable(uint32_t tableId, SummaryTable *adHocDefinition, uint32_t baseObjectId, uint32_t userId, uint32_t *rcc)
{
   shared_ptr<NetObj> object = FindObjectById(baseObjectId);
   if (object == nullptr)
   {
      *rcc = RCC_INVALID_OBJECT_ID;
      return nullptr;
   }

   if (!object->checkAccessRights(userId, OBJECT_ACCESS_READ))
   {
      *rcc = RCC_ACCESS_DENIED;
      return nullptr;
   }

   if ((object->getObjectClass() != OBJECT_CONTAINER) && (object->getObjectClass() != OBJECT_COLLECTOR) &&
       (object->getObjectClass() != OBJECT_CLUSTER) && (object->getObjectClass() != OBJECT_SERVICEROOT) &&
       (object->getObjectClass() != OBJECT_SUBNET) && (object->getObjectClass() != OBJECT_ZONE) &&
       (object->getObjectClass() != OBJECT_NETWORK) && (object->getObjectClass() != OBJECT_WIRELESSDOMAIN))
   {
      *rcc = RCC_INCOMPATIBLE_OPERATION;
      return nullptr;
   }

   SummaryTable *dbTableDefinition, *tableDefinition;
   if (adHocDefinition == nullptr)
   {
      tableDefinition = dbTableDefinition = SummaryTable::loadFromDB(tableId, rcc);
      if (dbTableDefinition == nullptr)
         return nullptr;
   }
   else
   {
      dbTableDefinition = nullptr;
      tableDefinition = adHocDefinition;
   }

   unique_ptr<SharedObjectArray<NetObj>> childObjects = object->getAllChildren(true);
   Table *tableData = tableDefinition->createEmptyResultTable();
   for(int i = 0; i < childObjects->size(); i++)
   {
      NetObj *obj = childObjects->get(i);
      if (!obj->isDataCollectionTarget() || !obj->checkAccessRights(userId, OBJECT_ACCESS_READ))
         continue;

      shared_ptr<DataCollectionTarget> target = static_pointer_cast<DataCollectionTarget>(childObjects->getShared(i));
      if (tableDefinition->filter(target))
      {
         target->getDciValuesSummary(tableDefinition, tableData, userId);
      }
   }

   delete dbTableDefinition;
   return tableData;
}

/**
 * Create export record for summary table
 */
bool CreateSummaryTableExportRecord(uint32_t id, TextFileWriter& xml)
{
   uint32_t rcc;
   SummaryTable *t = SummaryTable::loadFromDB(id, &rcc);
   if (t == nullptr)
      return false;
   t->createExportRecord(xml);
   delete t;
   return true;
}

/**
 * Build column list
 */
static StringBuffer BuildColumnList(ConfigEntry *root)
{
   if (root == nullptr)
      return StringBuffer();

   StringBuffer s;
   unique_ptr<ObjectArray<ConfigEntry>> columns = root->getOrderedSubEntries(_T("column#*"));
   for(int i = 0; i < columns->size(); i++)
   {
      if (i > 0)
         s.append(_T("^~^"));

      ConfigEntry *c = columns->get(i);
      s.append(c->getSubEntryValue(_T("name")));
      s.append(_T("^#^"));
      s.append(c->getSubEntryValue(_T("dci")));
      s.append(_T("^#^"));
      s.append(c->getSubEntryValueAsUInt(_T("flags")));
      s.append(_T("^#^"));
      s.append(c->getSubEntryValue(_T("separator")));
   }
   return s;
}

/**
 * Import failure exit
 */
static bool ImportFailure(DB_HANDLE hdb, DB_STATEMENT hStmt, ImportContext *context)
{
   if (hStmt != nullptr)
      DBFreeStatement(hStmt);
   DBRollback(hdb);
   DBConnectionPoolReleaseConnection(hdb);
   context->log(NXLOG_ERROR, _T("ImportSummaryTable()"), _T("Database failure"));
   return false;
}

/**
 * Import summary table
 */
bool ImportSummaryTable(ConfigEntry *config, bool overwrite, ImportContext *context, bool nxslV5)
{
   const TCHAR *guid = config->getSubEntryValue(_T("guid"));
   if (guid == nullptr)
   {
      context->log(NXLOG_ERROR, _T("ImportSummaryTable()"), _T("Missing summary table GUID"));
      return false;
   }

   uuid_t temp;
   if (_uuid_parse(guid, temp) == -1)
   {
      context->log(NXLOG_ERROR, _T("ImportSummaryTable()"), _T("Summary table GUID %s is invalid"), guid);
      return false;
   }

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   // Step 1: find existing tool ID by GUID
   DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT id FROM dci_summary_tables WHERE guid=?"));
   if (hStmt == nullptr)
   {
      return ImportFailure(hdb, nullptr, context);
   }

   DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, guid, DB_BIND_STATIC);
   DB_RESULT hResult = DBSelectPrepared(hStmt);
   if (hResult == nullptr)
   {
      return ImportFailure(hdb, hStmt, context);
   }

   uint32_t id;
   if (DBGetNumRows(hResult) > 0)
   {
      id = DBGetFieldULong(hResult, 0, 0);
   }
   else
   {
      id = 0;
   }
   DBFreeResult(hResult);
   DBFreeStatement(hStmt);

   // Step 2: create or update summary table configuration record
   if (id == 0)
   {
      id = CreateUniqueId(IDG_DCI_SUMMARY_TABLE);
      hStmt = DBPrepare(hdb, _T("INSERT INTO dci_summary_tables (menu_path,title,node_filter,flags,columns,guid,id) VALUES (?,?,?,?,?,?,?)"));
   }
   else
   {
      if (!overwrite)
      {
         DBConnectionPoolReleaseConnection(hdb);
         return true;
      }
      hStmt = DBPrepare(hdb, _T("UPDATE dci_summary_tables SET menu_path=?,title=?,node_filter=?,flags=?,columns=?,guid=? WHERE id=?"));
   }
   if (hStmt == nullptr)
   {
      return ImportFailure(hdb, nullptr, context);
   }

   DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, config->getSubEntryValue(_T("path")), DB_BIND_STATIC);
   DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, config->getSubEntryValue(_T("title")), DB_BIND_STATIC);
   StringBuffer filter;
   if (nxslV5)
      filter = config->getSubEntryValue(_T("filter"));
   else
      filter = NXSLConvertToV5(config->getSubEntryValue(_T("filter"), 0, _T("")));
   DBBind(hStmt, 3, DB_SQLTYPE_TEXT, filter, DB_BIND_STATIC);
   DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, config->getSubEntryValueAsUInt(_T("flags")));
   DBBind(hStmt, 5, DB_SQLTYPE_TEXT, BuildColumnList(config->findEntry(_T("columns"))), DB_BIND_TRANSIENT);
   DBBind(hStmt, 6, DB_SQLTYPE_VARCHAR, guid, DB_BIND_STATIC);
   DBBind(hStmt, 7, DB_SQLTYPE_INTEGER, id);

   if (!DBExecute(hStmt))
   {
      return ImportFailure(hdb, hStmt, context);
   }

   NotifyClientSessions(NX_NOTIFY_DCISUMTBL_CHANGED, (UINT32)id);

   DBFreeStatement(hStmt);
   DBConnectionPoolReleaseConnection(hdb);
   return true;
}

/**
 * Get list of configured DCI summary tables
 */
json_t *GetSummaryTablesList()
{
   json_t *result = json_array();
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_RESULT hResult = DBSelect(hdb, _T("SELECT id,title FROM dci_summary_tables"));
   if (hResult != nullptr)
   {
      TCHAR buffer[256];
      int32_t count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         json_t *summatyTableEntry = json_object();
         json_object_set_new(summatyTableEntry, "id", json_integer(DBGetFieldULong(hResult, i, 0)));
         json_object_set_new(summatyTableEntry, "name", json_string_t(DBGetField(hResult, i, 1, buffer, 256)));
         json_array_append_new(result, summatyTableEntry);
      }
      DBFreeResult(hResult);
   }
   DBConnectionPoolReleaseConnection(hdb);
   return result;
}
