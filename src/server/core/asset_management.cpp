/*
** NetXMS - Network Management System
** Copyright (C) 2003-2025 Raden Solutions
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
** File: asset_management.cpp
**
**/

#include "nxcore.h"
#include <asset_management.h>

/**
 * Create asset attribute from database.
 * Fields:
 * attr_name,display_name,data_type,is_mandatory,is_unique,is_hidden,autofill_script,range_min,range_max,sys_type
 */
AssetAttribute::AssetAttribute(DB_RESULT result, int row)
{
   m_name = DBGetField(result, row, 0, nullptr, 0);
   m_displayName = DBGetField(result, row, 1, nullptr, 0);
   m_dataType = static_cast<AMDataType>(DBGetFieldLong(result, row, 2));
   m_isMandatory = DBGetFieldLong(result, row, 3) ? true : false;
   m_isUnique = DBGetFieldLong(result, row, 4) ? true : false;
   m_isHidden = DBGetFieldLong(result, row, 5) ? true : false;
   m_autofillScriptSource = nullptr;
   m_autofillScript = nullptr;
   setScript(DBGetField(result, row, 6, nullptr, 0));
   m_rangeMin = DBGetFieldLong(result, row, 7);
   m_rangeMax = DBGetFieldLong(result, row, 8);
   m_systemType = static_cast<AMSystemType>(DBGetFieldLong(result, row, 9));
}

/**
 * Create asset attribute from scratch
 */
AssetAttribute::AssetAttribute(const NXCPMessage &msg) : m_enumValues(msg, VID_AM_ENUM_MAP_BASE, VID_ENUM_COUNT)
{
   m_name = msg.getFieldAsString(VID_NAME);
   m_displayName = msg.getFieldAsString(VID_DISPLAY_NAME);
   m_dataType = static_cast<AMDataType>(msg.getFieldAsUInt32(VID_DATA_TYPE));
   m_isMandatory = msg.getFieldAsBoolean(VID_IS_MANDATORY);
   m_isUnique = msg.getFieldAsBoolean(VID_IS_UNIQUE);
   m_isHidden = msg.getFieldAsBoolean(VID_IS_HIDDEN);
   m_autofillScriptSource = nullptr;
   m_autofillScript = nullptr;
   setScript(msg.getFieldAsString(VID_SCRIPT));
   m_rangeMin = msg.getFieldAsUInt32(VID_RANGE_MIN);
   m_rangeMax = msg.getFieldAsUInt32(VID_RANGE_MAX);
   m_systemType = static_cast<AMSystemType>(msg.getFieldAsUInt32(VID_SYSTEM_TYPE));
}

/**
 * Create asset attribute from config entry
 */
AssetAttribute::AssetAttribute(const wchar_t *name, const ConfigEntry& entry, bool nxslV5)
{
   m_name = MemCopyStringW(name);
   m_displayName = MemCopyStringW(entry.getSubEntryValue(_T("displayName")));
   m_dataType = static_cast<AMDataType>(entry.getSubEntryValueAsInt(_T("dataType"), 0 , 0));
   m_isMandatory = entry.getSubEntryValueAsBoolean(_T("mandatory"), 0, false);
   m_isUnique = entry.getSubEntryValueAsBoolean(_T("unique"), 0, false);
   m_isHidden = entry.getSubEntryValueAsBoolean(_T("hidden"), 0, false);
   m_autofillScriptSource = nullptr;
   m_autofillScript = nullptr;

   TCHAR *script;
   if (nxslV5)
      script = MemCopyString(entry.getSubEntryValue(_T("script")));
   else
   {
      StringBuffer output = NXSLConvertToV5(entry.getSubEntryValue(_T("script"), 0, _T("")));
      script = MemCopyString(output);
   }
   setScript(script);

   m_rangeMin = entry.getSubEntryValueAsInt(_T("rangeMin"), 0 , 0);
   m_rangeMax = entry.getSubEntryValueAsInt(_T("rangeMax"), 0 , 0);
   m_systemType = static_cast<AMSystemType>(entry.getSubEntryValueAsInt(_T("systemType"), 0 , 0));

   ConfigEntry *enumValuesRoot = entry.findEntry(_T("enumValues"));
   if (enumValuesRoot != nullptr)
   {
      unique_ptr<ObjectArray<ConfigEntry>> enumValues = enumValuesRoot->getSubEntries(_T("enumValue"));
      for(int i = 0; i < enumValues->size(); i++)
      {
         const ConfigEntry *config = enumValues->get(i);
         m_enumValues.set(config->getSubEntryValue(_T("name")), config->getSubEntryValue(_T("value")));
      }
   }
}

/**
 * Asset attribute destructor
 */
AssetAttribute::~AssetAttribute()
{
   delete m_autofillScript;
   MemFree(m_autofillScriptSource);
   MemFree(m_name);
   MemFree(m_displayName);
}

/**
 * Load enum values from database
 */
void AssetAttribute::loadEnumValues(DB_RESULT result)
{
   int rowCount = DBGetNumRows(result);
   for (int i = 0; i < rowCount; i++)
   {
      m_enumValues.setPreallocated(DBGetField(result, i, 0, nullptr, 0), DBGetField(result, i, 1, nullptr, 0));
   }
}

/**
 * Fill message with attribute data
 */
void AssetAttribute::fillMessage(NXCPMessage *msg, uint32_t baseId)
{
   msg->setField(baseId++, m_name);
   msg->setField(baseId++, m_displayName);
   msg->setField(baseId++, static_cast<uint32_t>(m_dataType));
   msg->setField(baseId++, m_isMandatory);
   msg->setField(baseId++, m_isUnique);
   msg->setField(baseId++, m_isHidden);
   msg->setField(baseId++, m_autofillScriptSource);
   msg->setField(baseId++, m_rangeMin);
   msg->setField(baseId++, m_rangeMax);
   msg->setField(baseId++, static_cast<uint32_t>(m_systemType));
   m_enumValues.fillMessage(msg, baseId + 1, baseId);
}

/**
 * Update attribute form message
 */
void AssetAttribute::updateFromMessage(const NXCPMessage &msg)
{
   MemFree(m_displayName);
   m_displayName = msg.getFieldAsString(VID_DISPLAY_NAME);
   m_dataType = static_cast<AMDataType>(msg.getFieldAsUInt32(VID_DATA_TYPE));
   m_isMandatory = msg.getFieldAsBoolean(VID_IS_MANDATORY);
   m_isUnique = msg.getFieldAsBoolean(VID_IS_UNIQUE);
   m_isHidden = msg.getFieldAsBoolean(VID_IS_HIDDEN);
   setScript(msg.getFieldAsString(VID_SCRIPT));
   m_rangeMin = msg.getFieldAsUInt32(VID_RANGE_MIN);
   m_rangeMax = msg.getFieldAsUInt32(VID_RANGE_MAX);
   m_systemType = static_cast<AMSystemType>(msg.getFieldAsUInt32(VID_SYSTEM_TYPE));

   m_enumValues.clear();
   m_enumValues.addAllFromMessage(msg, VID_AM_ENUM_MAP_BASE, VID_ENUM_COUNT);

   saveToDatabase();
}

/**
 * Save to database
 */
bool AssetAttribute::saveToDatabase() const
{
   bool success = false;
   DB_HANDLE db = DBConnectionPoolAcquireConnection();
   DB_STATEMENT stmt;
   DBBegin(db);
   if (IsDatabaseRecordExist(db, _T("am_attributes"), _T("attr_name"), m_name))
   {
      stmt = DBPrepare(db, _T("UPDATE am_attributes SET display_name=?,data_type=?,is_mandatory=?,is_unique=?,is_hidden=?,autofill_script=?,range_min=?,range_max=?,sys_type=? WHERE attr_name=?"));
   }
   else
   {
      stmt = DBPrepare(db, _T("INSERT INTO am_attributes (display_name,data_type,is_mandatory,is_unique,is_hidden,autofill_script,range_min,range_max,sys_type,attr_name) VALUES (?,?,?,?,?,?,?,?,?,?)"));
   }
   if (stmt != nullptr)
   {
      DBBind(stmt, 1, DB_SQLTYPE_VARCHAR, m_displayName, DB_BIND_STATIC);
      DBBind(stmt, 2, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(m_dataType));
      DBBind(stmt, 3, DB_SQLTYPE_VARCHAR, m_isMandatory ? _T("1") : _T("0"), DB_BIND_STATIC);
      DBBind(stmt, 4, DB_SQLTYPE_VARCHAR, m_isUnique ? _T("1") : _T("0"), DB_BIND_STATIC);
      DBBind(stmt, 5, DB_SQLTYPE_VARCHAR, m_isHidden ? _T("1") : _T("0"), DB_BIND_STATIC);
      DBBind(stmt, 6, DB_SQLTYPE_TEXT, m_autofillScriptSource, DB_BIND_STATIC);
      DBBind(stmt, 7, DB_SQLTYPE_INTEGER, m_rangeMin);
      DBBind(stmt, 8, DB_SQLTYPE_INTEGER, m_rangeMax);
      DBBind(stmt, 9, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(m_systemType));
      DBBind(stmt, 10, DB_SQLTYPE_VARCHAR, m_name, DB_BIND_STATIC);

      success = DBExecute(stmt);
      DBFreeStatement(stmt);
   }

   if (success)
   {
      success = ExecuteQueryOnObject(db, m_name, _T("DELETE FROM am_enum_values WHERE attr_name=?"));
   }

   if (success)
   {
      stmt = DBPrepare(db, _T("INSERT INTO am_enum_values (attr_name,value,display_name) VALUES (?,?,?)"), m_enumValues.size() > 1);
      if (stmt != nullptr)
      {
         DBBind(stmt, 1, DB_SQLTYPE_VARCHAR, m_name, DB_BIND_STATIC);
         for(KeyValuePair<const TCHAR> *v : m_enumValues)
         {
            DBBind(stmt, 2, DB_SQLTYPE_VARCHAR, v->key, DB_BIND_STATIC);
            DBBind(stmt, 3, DB_SQLTYPE_VARCHAR, v->value, DB_BIND_STATIC);
            if (!DBExecute(stmt))
            {
               success = false;
               break;
            }
         }
         DBFreeStatement(stmt);
      }
   }

   if (success)
      DBCommit(db);
   else
      DBRollback(db);

   DBConnectionPoolReleaseConnection(db);
   return success;
}

/**
 * Delete attribute form database
 */
bool AssetAttribute::deleteFromDatabase()
{
   DB_HANDLE db = DBConnectionPoolAcquireConnection();
   bool success = DBBegin(db);

   if (success)
      success = ExecuteQueryOnObject(db, m_name, _T("DELETE FROM am_attributes WHERE attr_name=?"));
   if (success)
      success = ExecuteQueryOnObject(db, m_name, _T("DELETE FROM am_enum_values WHERE attr_name=?"));
   if (success)
      success = ExecuteQueryOnObject(db, m_name, _T("DELETE FROM asset_properties WHERE attr_name=?"));

   if (success)
      DBCommit(db);
   else
      DBRollback(db);

   DBConnectionPoolReleaseConnection(db);
   return success;
}

/**
 * Update script
 */
void AssetAttribute::setScript(TCHAR *script)
{
   MemFree(m_autofillScriptSource);
   delete m_autofillScript;
   m_autofillScriptSource = Trim(script);

   if (m_autofillScriptSource != nullptr)
   {
      if (m_autofillScriptSource[0] != 0)
      {
         m_autofillScript = CompileServerScript(m_autofillScriptSource, SCRIPT_CONTEXT_ASSET_MGMT, nullptr, 0, _T("AMAutoFill::%s"), m_name);
      }
      else
      {
         MemFree(m_autofillScriptSource);
         m_autofillScriptSource = nullptr;
         m_autofillScript = nullptr;
      }
   }
   else
   {
      m_autofillScript = nullptr;
   }
}

/**
 * Get objects JSNON representation
 */
json_t *AssetAttribute::toJson() const
{
   json_t *root = json_object();
   json_object_set_new(root, "name", json_string_t(m_name));
   json_object_set_new(root, "displayName", json_string_t(m_displayName));
   json_object_set_new(root, "dataType", json_integer(static_cast<uint32_t>(m_dataType)));
   json_object_set_new(root, "isMandatory", json_boolean(m_isMandatory));
   json_object_set_new(root, "isUnique", json_boolean(m_isUnique));
   json_object_set_new(root, "autofillScript", json_string_t(m_autofillScriptSource));
   json_object_set_new(root, "rangeMin", json_integer(m_rangeMin));
   json_object_set_new(root, "rangeMax", json_integer(m_rangeMax));
   json_object_set_new(root, "systemType", json_integer(static_cast<uint32_t>(m_systemType)));
   json_object_set_new(root, "enumMap", m_enumValues.toJson());
   return root;
}

/**
 * Create asset attribute entry in export XML document
 */
void AssetAttribute::createExportRecord(TextFileWriter& xml)
{
   xml.append(_T("\t\t<attribute>\n\t\t\t<name>"));
   xml.append(m_name);
   xml.append(_T("</name>\n\t\t\t<displayName>"));
   xml.append(m_displayName);
   xml.append(_T("</displayName>\n\t\t\t<dataType>"));
   xml.append(static_cast<int32_t>(m_dataType));
   xml.append(_T("</dataType>\n\t\t\t<mandatory>"));
   xml.append(m_isMandatory);
   xml.append(_T("</mandatory>\n\t\t\t<unique>"));
   xml.append(m_isUnique);
   xml.append(_T("</unique>\n\t\t\t<hidden>"));
   xml.append(m_isHidden);
   xml.append(_T("</hidden>\n\t\t\t<script>"));
   xml.append(m_autofillScriptSource);
   xml.append(_T("</script>\n\t\t\t<rangeMin>"));
   xml.append(m_rangeMin);
   xml.append(_T("</rangeMin>\n\t\t\t<rangeMax>"));
   xml.append(m_rangeMax);
   xml.append(_T("</rangeMax>\n\t\t\t<systemType>"));
   xml.append(static_cast<int32_t>(m_systemType));
   xml.append(_T("</systemType>\n"));

   if (m_enumValues.size() > 0)
   {
      xml.append(_T("\t\t\t<enumValues>\n"));
      for(KeyValuePair<const TCHAR> *v : m_enumValues)
      {
         xml.append(_T("\t\t\t\t<enumValue>\n\t\t\t\t\t<name>"));
         xml.append(v->key);
         xml.append(_T("</name>\n\t\t\t\t\t<value>"));
         xml.append(v->value);
         xml.append(_T("</value>\n"));
         xml.append(_T("\t\t\t\t</enumValue>\n"));
      }
      xml.append(_T("\t\t\t</enumValues>\n"));
   }

   xml.append(_T("\t\t</attribute>\n"));
}

/**
 * Change log record ID
 */
static VolatileCounter64 s_assetChangeLogId = 0;

/**
 * Get last used asset change log record ID
 */
uint64_t GetLastAssetChangeLogId()
{
   return s_assetChangeLogId;
}

/**
 * Initialize asset change log ID
 */
static void InitAssetChangeLogId()
{
   int64_t id = ConfigReadInt64(_T("LastAssetChangeLogRecordId"), 0);
   if (id > s_assetChangeLogId)
      s_assetChangeLogId = id;

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_RESULT hResult = DBSelect(hdb, _T("SELECT max(record_id) FROM asset_change_log"));
   if (hResult != nullptr)
   {
      if (DBGetNumRows(hResult) > 0)
         s_assetChangeLogId = std::max(DBGetFieldInt64(hResult, 0, 0), static_cast<int64_t>(s_assetChangeLogId));
      DBFreeResult(hResult);
   }
   DBConnectionPoolReleaseConnection(hdb);
}

/**
 * Asset data to be logged
 */
struct AssetChangeLogData
{
   uint32_t assetId;
   AssetOperation operation;
   time_t timestamp;
   String attributeName;
   String oldValue;
   String newValue;
   uint32_t userId;
   uint32_t objectId;

   AssetChangeLogData(uint32_t assetId, const TCHAR *attributeName, AssetOperation operation,
         const TCHAR *oldValue, const TCHAR *newValue, uint32_t userId, uint32_t objectId);
};

/**
 * Constructor
 */
AssetChangeLogData::AssetChangeLogData(uint32_t assetId, const TCHAR *attributeName, AssetOperation operation,
      const TCHAR *oldValue, const TCHAR *newValue, uint32_t userId, uint32_t objectId) : attributeName(attributeName), oldValue(oldValue), newValue(newValue)
{
   this->assetId = assetId;
   this->operation = operation;
   this->timestamp = time(nullptr);
   this->userId = userId;
   this->objectId = objectId;
}

/**
 * Saves operation with asset to log
 */
static void WriteAssetChangeLogWriter(AssetChangeLogData *data)
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_STATEMENT hStmt = DBPrepare(hdb,
         g_dbSyntax == DB_SYNTAX_TSDB ?
            _T("INSERT INTO asset_change_log (record_id,operation_timestamp,asset_id,attribute_name,operation,old_value,new_value,user_id,linked_object_id) VALUES (?,to_timestamp(?),?,?,?,?,?,?,?)") :
            _T("INSERT INTO asset_change_log (record_id,operation_timestamp,asset_id,attribute_name,operation,old_value,new_value,user_id,linked_object_id) VALUES (?,?,?,?,?,?,?,?,?)"));
   if (hStmt != nullptr)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_BIGINT, InterlockedIncrement64(&s_assetChangeLogId));
      DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(data->timestamp));
      DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, data->assetId);
      DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, data->attributeName, DB_BIND_STATIC, 63);
      DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(data->operation));
      DBBind(hStmt, 6, DB_SQLTYPE_VARCHAR, data->oldValue, DB_BIND_STATIC, 2000);
      DBBind(hStmt, 7, DB_SQLTYPE_VARCHAR, data->newValue, DB_BIND_STATIC, 2000);
      DBBind(hStmt, 8, DB_SQLTYPE_INTEGER, data->userId);
      DBBind(hStmt, 9, DB_SQLTYPE_INTEGER, data->objectId);
      DBExecute(hStmt);
      DBFreeStatement(hStmt);
   }
   DBConnectionPoolReleaseConnection(hdb);
   delete data;
}

/**
 * Write asset change log
 */
void WriteAssetChangeLog(uint32_t assetId, const TCHAR *attributeName, AssetOperation operation, const TCHAR *oldValue, const TCHAR *newValue, uint32_t userId, uint32_t objectId)
{
   AssetChangeLogData *data = new AssetChangeLogData(assetId, attributeName, operation, oldValue, newValue, userId, objectId);
   ThreadPoolExecuteSerialized(g_mainThreadPool, _T("AssetChangeLogWriter"), WriteAssetChangeLogWriter, data);
}

/**
 * Asset management schema
 */
static StringObjectMap<AssetAttribute> s_schema(Ownership::True);

/**
 * Asset management schema lock
 */
static RWLock s_schemaLock;

/**
 * Load asset management schema from database
 */
void LoadAssetManagementSchema()
{
   DB_HANDLE db = DBConnectionPoolAcquireConnection();

   DB_RESULT result = DBSelect(db, _T("SELECT attr_name,display_name,data_type,is_mandatory,is_unique,is_hidden,autofill_script,range_min,range_max,sys_type FROM am_attributes"));
   if (result != nullptr)
   {
      DB_STATEMENT stmt = DBPrepare(db, _T("SELECT value,display_name FROM am_enum_values WHERE attr_name=?"), true);
      if (stmt != nullptr)
      {
         int numRows = DBGetNumRows(result);
         for (int i = 0; i < numRows; i++)
         {
            AssetAttribute *a = new AssetAttribute(result, i);
            if (a->getDataType() == AMDataType::Enum)
            {
               DBBind(stmt, 1, DB_SQLTYPE_VARCHAR, a->getName(), DB_BIND_STATIC);
               DB_RESULT mappingResult = DBSelectPrepared(stmt);
               if (mappingResult != nullptr)
               {
                  a->loadEnumValues(mappingResult);
                  DBFreeResult(mappingResult);
               }
            }
            s_schema.set(a->getName(), a);
         }
         DBFreeStatement(stmt);
      }
      DBFreeResult(result);
   }

   DBConnectionPoolReleaseConnection(db);
   nxlog_debug_tag(DEBUG_TAG_ASSET_MGMT, 2, _T("%d asset attributes loaded"), s_schema.size());

   InitAssetChangeLogId();
}

/**
 * Fill message with asset management schema
 */
void AssetManagementSchemaToMessage(NXCPMessage *msg)
{
   s_schemaLock.readLock();
   msg->setField(VID_NUM_ASSET_ATTRIBUTES, s_schema.size());
   uint32_t fieldId = VID_AM_ATTRIBUTES_BASE;
   for (KeyValuePair<AssetAttribute> *a : s_schema)
   {
      a->value->fillMessage(msg, fieldId);
      fieldId += 256;
   }
   s_schemaLock.unlock();
}

/**
 * Create asset management attribute
 */
uint32_t CreateAssetAttribute(const NXCPMessage& msg, const ClientSession& session)
{
   uint32_t result;
   s_schemaLock.writeLock();
   SharedString name = msg.getFieldAsSharedString(VID_NAME, 64);

   if (RegexpMatch(name, _T("^[A-Za-z$_][A-Za-z0-9$_]*$"), true))
   {
      if (s_schema.get(name) == nullptr)
      {
         AssetAttribute *attribute = new AssetAttribute(msg);
         if (attribute->saveToDatabase())
         {
            s_schema.set(name, attribute);

            json_t *attrData = attribute->toJson();
            session.writeAuditLogWithValues(AUDIT_SYSCFG, true, 0,  nullptr, attrData, _T("Asset management attribute \"%s\" created"), name.cstr());
            json_decref(attrData);

            NXCPMessage notificationMessage(CMD_UPDATE_ASSET_ATTRIBUTE, 0);
            attribute->fillMessage(&notificationMessage, VID_AM_ATTRIBUTES_BASE);
            NotifyClientSessions(notificationMessage);

            result = RCC_SUCCESS;
         }
         else
         {
            delete attribute;
            result = RCC_DB_FAILURE;
         }
      }
      else
      {
         result = RCC_ATTRIBUTE_ALREADY_EXISTS;
      }
   }
   else
   {
      result = RCC_INVALID_OBJECT_NAME;
   }
   s_schemaLock.unlock();
   return result;
}

/**
 * Update asset management attribute
 */
uint32_t UpdateAssetAttribute(const NXCPMessage& msg, const ClientSession& session)
{
   uint32_t result;
   s_schemaLock.writeLock();
   SharedString name = msg.getFieldAsSharedString(VID_NAME, 64);
   AssetAttribute *attribute = s_schema.get(name);
   if (attribute != nullptr)
   {
      json_t *oldAttrData = attribute->toJson();
      attribute->updateFromMessage(msg);
      json_t *newAttrData = attribute->toJson();

      session.writeAuditLogWithValues(AUDIT_SYSCFG, true, 0,  oldAttrData, newAttrData, _T("Asset attribute \"%s\" updated"), name.cstr());
      json_decref(oldAttrData);
      json_decref(newAttrData);

      NXCPMessage notificationMessage(CMD_UPDATE_ASSET_ATTRIBUTE, 0);
      attribute->fillMessage(&notificationMessage, VID_AM_ATTRIBUTES_BASE);
      NotifyClientSessions(notificationMessage);

      result = RCC_SUCCESS;
   }
   else
   {
      result = RCC_UNKNOWN_ATTRIBUTE;
   }
   s_schemaLock.unlock();
   return result;
}

/**
 * Delete asset attribute
 */
uint32_t DeleteAssetAttribute(const NXCPMessage &msg, const ClientSession &session)
{
   uint32_t result;
   s_schemaLock.writeLock();
   SharedString name = msg.getFieldAsSharedString(VID_NAME, 64);
   AssetAttribute *attribute = s_schema.get(name);
   if (attribute != nullptr)
   {
      json_t *oldAttrData = attribute->toJson();
      session.writeAuditLogWithValues(AUDIT_SYSCFG, true, 0, oldAttrData, nullptr, _T("Asset attribute \"%s\" deleted"), name.cstr());
      json_decref(oldAttrData);

      bool success = attribute->deleteFromDatabase();
      s_schema.remove(name);

      unique_ptr<SharedObjectArray<NetObj>> objects = g_idxObjectById.getObjects(OBJECT_ASSET);
      for (int i = 0; i < objects->size(); i++)
      {
         static_cast<Asset*>(objects->get(i))->deleteCachedProperty(name);
      }

      NXCPMessage notificationMessage(CMD_DELETE_ASSET_ATTRIBUTE, 0);
      notificationMessage.setField(VID_NAME, name);
      NotifyClientSessions(notificationMessage);

      result = success ? RCC_SUCCESS : RCC_DB_FAILURE;
   }
   else
   {
      result = RCC_UNKNOWN_ATTRIBUTE;
   }
   s_schemaLock.unlock();
   return result;
}

/**
 * Validate date
 */
bool isValidDate(uint32_t formatedDate)
{
   int year = formatedDate / 10000 - 1900;
   if (year > 0)
      return false;

   int day = formatedDate % 100;
   int month = (formatedDate % 10000) / 100 - 1;

   struct tm tmp;
   memset(&tmp, 0, sizeof(struct tm));
   tmp.tm_year = year;
   tmp.tm_mon = month;
   tmp.tm_mday = day;
   tmp.tm_hour = 12; // to prevent unexpected results when DST changes
   tmp.tm_isdst = -1;

   time_t t = mktime(&tmp);
#if HAVE_LOCALTIME_R
   struct tm tmBuff;
   struct tm *ltm = localtime_r(&t, &tmBuff);
#else
   struct tm *ltm = localtime(&t);
#endif

   return (day == ltm->tm_mday) && (month == ltm->tm_mon) && (year == ltm->tm_year);
}

/**
 * Validate value for asset property
 */
std::pair<uint32_t, String> ValidateAssetPropertyValue(const TCHAR *name, const TCHAR *value)
{
   s_schemaLock.readLock();
   AssetAttribute *assetAttribute = s_schema.get(name);
   if (assetAttribute == nullptr)
   {
      s_schemaLock.unlock();
      return std::pair<uint32_t, String>(RCC_UNKNOWN_ATTRIBUTE, _T("No such attribute"));
   }

   StringBuffer resultText;
   switch (assetAttribute->getDataType())
   {
      case AMDataType::String:
         if (assetAttribute->isRangeSet())
         {
            int32_t len = static_cast<int32_t>(_tcslen(value));
            if (len < assetAttribute->getMinRange())
            {
               resultText = _T("String is too short");
            }
            if (len > assetAttribute->getMaxRange())
            {
               resultText = _T("String is too long");
            }
         }
         break;
      case AMDataType::Enum:
      {
         if (!assetAttribute->isValidEnumValue(value))
         {
            resultText = _T("Invalid enum value");
         }
         break;
      }
      case AMDataType::Integer:
      {
         TCHAR *error;
         int32_t number = _tcstol(value, &error, 0);
         if (*error != 0)
         {
            resultText = _T("Not an integer");
         }
         if (assetAttribute->isRangeSet())
         {
            if(number < assetAttribute->getMinRange())
            {
               resultText = _T("Integer is too small");
            }
            if(number > assetAttribute->getMaxRange())
            {
               resultText = _T("Integer is too big");
            }
         }
         break;
      }
      case AMDataType::Number:
      {
         TCHAR *error;;
         double number = _tcstod(value, &error);
         if (*error != 0)
         {
            resultText = _T("Not a number");
         }
         if (assetAttribute->isRangeSet())
         {
            if(number < assetAttribute->getMinRange())
            {
               resultText = _T("Number is too small");
            }
            if(number > assetAttribute->getMaxRange())
            {
               resultText = _T("Number is too big");
            }
         }
         break;
      }
      case AMDataType::Boolean:
      {
         if (!_tcsicmp(value, _T("yes")) || !_tcsicmp(value, _T("true")) || !_tcsicmp(value, _T("on"))
              || !_tcsicmp(value, _T("no")) || !_tcsicmp(value, _T("false")) || !_tcsicmp(value, _T("off")))
            break;

         TCHAR *error;
         _tcstol(value, &error, 0);
         if (*error != 0)
         {
            resultText = _T("Not a boolean value");
         }
         break;
      }
      case AMDataType::MacAddress:
      {
         MacAddress mac = MacAddress::parse(value);
         if (mac.isNull())
         {
            resultText = _T("Invalid MAC address format");
         }
         break;
      }
      case AMDataType::IPAddress:
      {
         InetAddress ipAddr = InetAddress::parse(value);
         if (!ipAddr.isValid())
         {
            resultText = _T("Invalid IP address format");
         }
         break;
      }
      case AMDataType::UUID:
      {
         uuid guid = uuid::parse(value);
         if (guid.isNull())
         {
            resultText = _T("Invalid UUID format");
         }
         break;
      }
      case AMDataType::ObjectReference:
      {
         TCHAR *error;
         uint32_t objectId = _tcstoul(value, &error, 0);
         if ((objectId != 0) && (*error == 0))
         {
            shared_ptr<NetObj> object = FindObjectById(objectId);
            if (object == nullptr)
            {
               resultText = _T("Referenced object not found");
            }
         }
         else
         {
            resultText = _T("Invalid object reference");
         }
         break;
      }
      case AMDataType::Date:
      {
         TCHAR *error;
         uint32_t date = _tcstoul(value, &error, 0);
         if ((date != 0) && (*error == 0))
         {
            if (isValidDate(date))
            {
               resultText = _T("Invalid date");
            }
         }
         else
         {
            resultText = _T("Invalid date format");
         }
         break;
      }
   }
   bool shouldBeUnique = assetAttribute->isUnique();
   s_schemaLock.unlock();

   if (shouldBeUnique && resultText.isEmpty())
   {
      unique_ptr<SharedObjectArray<NetObj>> objects = g_idxObjectById.getObjects(OBJECT_ASSET);
      for (int i = 0; i < objects->size(); i++)
      {
         auto asset = static_cast<Asset*>(objects->get(i));
         if (asset->isSamePropertyValue(name, value))
         {
            resultText.append(_T("Unique constraint violation (same value already set on "));
            resultText.append(asset->getName());
            resultText.append(_T(")"));
         }
      }
   }

   return std::pair<uint32_t, String>(resultText.isEmpty() ? RCC_SUCCESS : RCC_VALIDATION_ERROR, resultText);
}

/**
 * Get display name of given asset attribute
 */
String GetAssetAttributeDisplayName(const TCHAR *name)
{
   s_schemaLock.readLock();
   AssetAttribute *attribute = s_schema.get(name);
   String displayName((attribute != nullptr) ? attribute->getActualDisplayName() : name);
   s_schemaLock.unlock();
   return displayName;
}

/**
 * Check if given asset property is mandatory
 */
bool IsMandatoryAssetProperty(const TCHAR *name)
{
   s_schemaLock.readLock();
   AssetAttribute *attr = s_schema.get(name);
   bool mandatory = (attr != nullptr) && attr->isMandatory();
   s_schemaLock.unlock();
   return mandatory;
}

/**
 * Check if given asset property is boolean
 */
bool IsBooleanAssetProperty(const TCHAR *name)
{
   s_schemaLock.readLock();
   AssetAttribute *attr = s_schema.get(name);
   bool isBoolean = (attr != nullptr) && (attr->getDataType() == AMDataType::Boolean);
   s_schemaLock.unlock();
   return isBoolean;
}

/**
 * Check if given asset property name is valid
 */
bool IsValidAssetPropertyName(const TCHAR *name)
{
   s_schemaLock.readLock();
   bool valid = s_schema.contains(name);
   s_schemaLock.unlock();
   return valid;
}

/**
 * Get all asset attribute names
 */
unique_ptr<StringSet> GetAssetAttributeNames(bool mandatoryOnly)
{
   unique_ptr<StringSet> names = make_unique<StringSet>();
   s_schemaLock.readLock();
   for (KeyValuePair<AssetAttribute> *a : s_schema)
   {
      if (!mandatoryOnly || a->value->isMandatory())
         names->add(a->key);
   }
   s_schemaLock.unlock();
   return names;
}

/**
 * Prepare contexts for asset property autofill
 */
unique_ptr<ObjectArray<AssetPropertyAutofillContext>> PrepareAssetPropertyAutofill(const Asset& asset, const shared_ptr<NetObj>& linkedObject)
{
   auto contexts = make_unique<ObjectArray<AssetPropertyAutofillContext>>(0, 16, Ownership::True);
   s_schemaLock.readLock();
   for (KeyValuePair<AssetAttribute> *a : s_schema)
   {
      if (a->value->getSystemType() != AMSystemType::None)
      {
         SharedString newValue;
         switch (a->value->getSystemType())
         {
            case AMSystemType::MacAddress:
            case AMSystemType::None:
            case AMSystemType::Serial:
               // Those system types are not intended to be autofilled
               break;
            case AMSystemType::IPAddress:
               if (linkedObject->getObjectClass() == OBJECT_NODE && static_pointer_cast<Node>(linkedObject)->getIpAddress().isValid())
               {
                  newValue = SharedString(static_pointer_cast<Node>(linkedObject)->getIpAddress().toString());
               }
               break;
            case AMSystemType::Model:
               if (linkedObject->getObjectClass() == OBJECT_NODE)
               {
                  newValue = static_pointer_cast<Node>(linkedObject)->getProductName();
               }
               if (linkedObject->getObjectClass() == OBJECT_MOBILEDEVICE)
               {
                  newValue = static_pointer_cast<MobileDevice>(linkedObject)->getModel();
               }
               if (linkedObject->getObjectClass() == OBJECT_ACCESSPOINT)
               {
                  newValue = static_pointer_cast<AccessPoint>(linkedObject)->getModel();
               }
               break;
            case AMSystemType::Vendor:
               if (linkedObject->getObjectClass() == OBJECT_NODE)
               {
                  newValue = static_pointer_cast<Node>(linkedObject)->getVendor();
               }
               if (linkedObject->getObjectClass() == OBJECT_MOBILEDEVICE)
               {
                  newValue = static_pointer_cast<MobileDevice>(linkedObject)->getVendor();
               }
               if (linkedObject->getObjectClass() == OBJECT_ACCESSPOINT)
               {
                  newValue = static_pointer_cast<AccessPoint>(linkedObject)->getVendor();
               }
               if (linkedObject->getObjectClass() == OBJECT_SENSOR)
               {
                  newValue = static_pointer_cast<Sensor>(linkedObject)->getVendor();
               }
               break;
         }

         if (!newValue.isNull())
            contexts->add(new AssetPropertyAutofillContext(a->key, a->value->getDataType(), nullptr, nullptr, newValue, a->value->getSystemType()));
         continue;
      }

      if (!a->value->hasScript())
         continue;

      NXSL_VM *vm = CreateServerScriptVM(a->value->getScript(), linkedObject);
      if (vm != nullptr)
      {
         contexts->add(new AssetPropertyAutofillContext(a->key, a->value->getDataType(), a->value->getEnumValues(), vm, nullptr, a->value->getSystemType()));
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG_ASSET_MGMT, 4, _T("PrepareAssetPropertyAutofill(%s [%u]): Script load failed"), asset.getName(), asset.getId());
         ReportScriptError(SCRIPT_CONTEXT_ASSET_MGMT, &asset, 0, _T("Script load failed"), _T("AssetAttribute::%s::autoFill"), a->key);
      }
   }
   s_schemaLock.unlock();
   return contexts;
}

/**
 * Get serial number for object
 */
static inline SharedString GetSerialNumber(const NetObj& object)
{
   switch(object.getObjectClass())
   {
      case OBJECT_ACCESSPOINT:
         return SharedString(static_cast<const AccessPoint&>(object).getSerialNumber());
      case OBJECT_NODE:
         return static_cast<const Node&>(object).getSerialNumber();
      case OBJECT_SENSOR:
         return SharedString(static_cast<const Sensor&>(object).getSerialNumber());
   }
   return SharedString();
}

/**
 * Get MAC address for object
 */
static inline MacAddress GetMacAddress(const NetObj& object)
{
   switch(object.getObjectClass())
   {
      case OBJECT_ACCESSPOINT:
         return static_cast<const AccessPoint&>(object).getMacAddress();
      case OBJECT_NODE:
         return static_cast<const Node&>(object).getPrimaryMacAddress();
      case OBJECT_SENSOR:
         return static_cast<const Sensor&>(object).getMacAddress();
   }
   return MacAddress();
}

/**
 * Update identification fields for asset (serial number and MAC address)
 */
std::pair<uint32_t, String> UpdateAssetIdentification(Asset *asset, NetObj *object, uint32_t userId)
{
   TCHAR serialAttributeName[MAX_OBJECT_NAME] = _T("");
   TCHAR macAttributeName[MAX_OBJECT_NAME] = _T("");
   s_schemaLock.readLock();
   for (KeyValuePair<AssetAttribute> *a : s_schema)
   {
      if (a->value->getSystemType() == AMSystemType::Serial)
         _tcslcpy(serialAttributeName, a->key, MAX_OBJECT_NAME);
      if (a->value->getSystemType() == AMSystemType::MacAddress)
         _tcslcpy(macAttributeName, a->key, MAX_OBJECT_NAME);
      if ((serialAttributeName[0] != 0) && (macAttributeName[0] != 0))
         break;
   }
   s_schemaLock.unlock();

   std::pair<uint32_t, String> result;

   SharedString serialNumber = GetSerialNumber(*object);
   if ((serialAttributeName[0] != 0) && !serialNumber.isEmpty())
   {
      return asset->setProperty(serialAttributeName, serialNumber, userId);
   }

   MacAddress macAddress = GetMacAddress(*object);
   if ((macAttributeName[0] != 0) &&  macAddress.isValid())
   {
      return asset->setProperty(macAttributeName, macAddress.toString(), userId);
   }

   return std::pair<uint32_t, String>(RCC_SUCCESS, _T(""));
}

/**
 * Link asset to object
 */
void LinkAsset(Asset *asset, NetObj *object, ClientSession *session)
{
   if (asset->getLinkedObjectId() == object->getId())
      return;

   UnlinkAsset(asset, session);

   if (object->getAssetId() != 0)
   {
      shared_ptr<Asset> oldAsset = static_pointer_cast<Asset>(FindObjectById(object->getAssetId(), OBJECT_ASSET));
      if (oldAsset != nullptr)
      {
         UnlinkAsset(oldAsset.get(), session);
      }
   }

   asset->setLinkedObjectId(object->getId());
   object->setAssetId(asset->getId());
   WriteAssetChangeLog(asset->getId(), nullptr, AssetOperation::Link, nullptr, nullptr, (session != nullptr) ? session->getUserId() : 0, object->getId());
   EventBuilder(EVENT_ASSET_LINK, object->getId()).
                  param(_T("assetId"), asset->getId()).
                  param(_T("assetName"), asset->getName()).post();
   if (session != nullptr)
   {
      session->writeAuditLog(AUDIT_OBJECTS, true, asset->getId(), _T("Asset linked with object %s [%u]"), object->getName(), object->getId());
      session->writeAuditLog(AUDIT_OBJECTS, true, object->getId(), _T("Object linked with asset %s [%u]"), asset->getName(), asset->getId());
   }
}

/**
 * Unlink asset from object it is currently linked to
 */
void UnlinkAsset(Asset *asset, ClientSession *session)
{
   if (asset->getLinkedObjectId() == 0)
      return;

   shared_ptr<NetObj> object = FindObjectById(asset->getLinkedObjectId());
   if (object != nullptr)
   {
      asset->setLinkedObjectId(0);
      object->setAssetId(0);
      WriteAssetChangeLog(asset->getId(), nullptr, AssetOperation::Unlink, nullptr, nullptr, (session != nullptr) ? session->getUserId() : 0, object->getId());
      EventBuilder(EVENT_ASSET_UNLINK, object->getId()).
                     param(_T("assetId"), asset->getId()).
                     param(_T("assetName"), asset->getName()).post();
      if (session != nullptr)
      {
         session->writeAuditLog(AUDIT_OBJECTS, true, object->getId(), _T("Link with asset %s [%u] removed"), asset->getName(), asset->getId());
         session->writeAuditLog(AUDIT_OBJECTS, true, asset->getId(), _T("Link with object %s [%u] removed"), object->getName(), object->getId());
      }
   }
}

/**
 * Update asset linking by provided attribute and value
 */
static void UpdateAssetLinkageInternal(NetObj *object, const TCHAR *attributeName, const TCHAR *objectValue, const TCHAR *attrTypeName, bool matchByMacAllowed)
{
   // Check that attached asset still match
   if (object->getAssetId() != 0)
   {
      shared_ptr<Asset> asset = static_pointer_cast<Asset>(FindObjectById(object->getAssetId(), OBJECT_ASSET));
      if (asset != nullptr)
      {
         if (asset->isSamePropertyValue(attributeName, objectValue))
         {
            nxlog_debug_tag(DEBUG_TAG_ASSET_MGMT_LINK, 6, _T("UpdateAssetLinkage(%s [%u]: %s \"%s\" match with currently linked asset \"%s\" [%u]"),
                  object->getName(), object->getId(), attrTypeName, objectValue, asset->getName(), asset->getId());
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG_ASSET_MGMT_LINK, 6, _T("UpdateAssetLinkage(%s [%u]: %s \"%s\" does not match with currently linked asset \"%s\" [%u]"),
                  object->getName(), object->getId(), attrTypeName, objectValue, asset->getName(), asset->getId());
            UnlinkAsset(asset.get(), nullptr);
            nxlog_debug_tag(DEBUG_TAG_ASSET_MGMT_LINK, 6, _T("UpdateAssetLinkage(%s [%u]: unlinked from asset %s [%u]"), object->getName(), object->getId(), asset->getName(), asset->getId());
            object->sendPollerMsg(POLLER_WARNING _T("   Unlinked from asset \"%s\"\r\n"), asset->getName());
         }
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG_ASSET_MGMT_LINK, 6, _T("UpdateAssetLinkage(%s [%u]: object linked to non-existing asset [%u]"), object->getName(), object->getId(), object->getAssetId());
         object->setAssetId(0);
      }
   }

   // Check if object can be linked to another asset (it may have been unlinked on previous step)
   if (object->getAssetId() == 0)
   {
      shared_ptr<Asset> asset = FindAssetByPropertyValue(attributeName, objectValue);
      if (asset != nullptr)
      {
         nxlog_debug_tag(DEBUG_TAG_ASSET_MGMT_LINK, 6, _T("UpdateAssetLinkage(%s [%u]: asset %s [%u] matches %s \"%s\""),
               object->getName(), object->getId(), asset->getName(), asset->getId(), attrTypeName, objectValue);
         if (asset->getLinkedObjectId() == 0)
         {
            LinkAsset(asset.get(), object, nullptr);
            nxlog_debug_tag(DEBUG_TAG_ASSET_MGMT_LINK, 6, _T("UpdateAssetLinkage(%s [%u]: linked to asset %s [%u]"), object->getName(), object->getId(), asset->getName(), asset->getId());
            object->sendPollerMsg(POLLER_INFO _T("   Linked to asset \"%s\"\r\n"), asset->getName());
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG_ASSET_MGMT_LINK, 6, _T("UpdateAssetLinkage(%s [%u]: asset %s [%u] already linked to another object"), object->getName(), object->getId(), asset->getName(), asset->getId());
            shared_ptr<NetObj> otherObject = FindObjectById(asset->getLinkedObjectId());
            if (otherObject != nullptr)
            {
               object->sendPollerMsg(POLLER_WARNING _T("   Asset \"%s\" is a match but already linked to another object \"%s\"\r\n"), asset->getName(), otherObject->getName());

               // Attempt to update other object linkage
               UpdateAssetLinkage(otherObject.get(), matchByMacAllowed);
               if (otherObject->getAssetId() == 0)
               {
                  LinkAsset(asset.get(), object, nullptr);
                  nxlog_debug_tag(DEBUG_TAG_ASSET_MGMT_LINK, 6, _T("UpdateAssetLinkage(%s [%u]: linked to asset %s [%u]"), object->getName(), object->getId(), asset->getName(), asset->getId());
                  object->sendPollerMsg(POLLER_INFO _T("   Linked to asset \"%s\"\r\n"), asset->getName());
               }
               else
               {
                  nxlog_debug_tag(DEBUG_TAG_ASSET_MGMT_LINK, 6, _T("UpdateAssetLinkage(%s [%u]: asset linkage conflict with object %s [%u]"), object->getName(), object->getId(), otherObject->getName(), otherObject->getId());
                  EventBuilder(EVENT_ASSET_LINK_CONFLICT, object->getId()).
                        param(_T("assetId"), asset->getId()).
                        param(_T("assetName"), asset->getName()).
                        param(_T("currentNodeId"), otherObject->getId()).
                        param(_T("currentNodeName"), otherObject->getName()).post();
                  object->sendPollerMsg(POLLER_WARNING _T("   Cannot be linked to asset \"%s\" because of conflict\r\n"), asset->getName());
               }
            }
            else
            {
               nxlog_debug_tag(DEBUG_TAG_ASSET_MGMT_LINK, 6, _T("UpdateAssetLinkage(%s [%u]: asset %s [%u] linked to non-existing object"), object->getName(), object->getId(), asset->getName(), asset->getId());
               asset->setLinkedObjectId(0);
               LinkAsset(asset.get(), object, nullptr);
               nxlog_debug_tag(DEBUG_TAG_ASSET_MGMT_LINK, 6, _T("UpdateAssetLinkage(%s [%u]: linked to asset %s [%u]"), object->getName(), object->getId(), asset->getName(), asset->getId());
               object->sendPollerMsg(POLLER_INFO _T("   Linked to asset \"%s\"\r\n"), asset->getName());
            }
         }
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG_ASSET_MGMT_LINK, 6, _T("UpdateAssetLinkage(%s [%u]: %s \"%s\" does not match any asset"), object->getName(), object->getId(), attrTypeName, objectValue);
      }
   }
}

/**
 * Check asset link to node/sensor/access point/etc. and update link if necessary
 */
void UpdateAssetLinkage(NetObj *object, bool matchByMacAllowed)
{
   TCHAR serialAttributeName[MAX_OBJECT_NAME] = _T("");
   TCHAR macAttributeName[MAX_OBJECT_NAME] = _T("");
   s_schemaLock.readLock();
   for (KeyValuePair<AssetAttribute> *a : s_schema)
   {
      if (a->value->getSystemType() == AMSystemType::Serial)
         _tcslcpy(serialAttributeName, a->key, MAX_OBJECT_NAME);
      if (a->value->getSystemType() == AMSystemType::MacAddress)
         _tcslcpy(macAttributeName, a->key, MAX_OBJECT_NAME);
      if ((serialAttributeName[0] != 0) && (macAttributeName[0] != 0))
         break;
   }
   s_schemaLock.unlock();

   if ((serialAttributeName[0] == 0) && (macAttributeName[0] == 0))
   {
      nxlog_debug_tag(DEBUG_TAG_ASSET_MGMT_LINK, 6, _T("UpdateAssetLinkage(%s [%u]): neither serial number nor MAC address attributes are defined in schema"), object->getName(), object->getId());
      return;
   }

   object->sendPollerMsg(_T("Checking asset linkage\r\n"));

   if (serialAttributeName[0] != 0)
      nxlog_debug_tag(DEBUG_TAG_ASSET_MGMT_LINK, 6, _T("UpdateAssetLinkage(%s [%u]): using serial number attribute \"%s\""), object->getName(), object->getId(), serialAttributeName);
   else
      nxlog_debug_tag(DEBUG_TAG_ASSET_MGMT_LINK, 6, _T("UpdateAssetLinkage(%s [%u]): serial number attribute not defined in schema"), object->getName(), object->getId());
   if (macAttributeName[0] != 0)
      nxlog_debug_tag(DEBUG_TAG_ASSET_MGMT_LINK, 6, _T("UpdateAssetLinkage(%s [%u]): using MAC address attribute \"%s\""), object->getName(), object->getId(), macAttributeName);
   else
      nxlog_debug_tag(DEBUG_TAG_ASSET_MGMT_LINK, 6, _T("UpdateAssetLinkage(%s [%u]): MAC address attribute not defined in schema"), object->getName(), object->getId());

   SharedString serialNumber = GetSerialNumber(*object);
   MacAddress macAddress = GetMacAddress(*object);
   if (serialNumber.isNull() || serialNumber.isEmpty())
   {
      nxlog_debug_tag(DEBUG_TAG_ASSET_MGMT_LINK, 6, _T("UpdateAssetLinkage(%s [%u]): cannot get serial number from object"), object->getName(), object->getId());
      if (!macAddress.isNull())
      {
         if (!matchByMacAllowed)
         {
            shared_ptr<Asset> asset = static_pointer_cast<Asset>(FindObjectById(object->getAssetId(), OBJECT_ASSET));
            if (asset != nullptr)
            {
               nxlog_debug_tag(DEBUG_TAG_ASSET_MGMT_LINK, 6, _T("UpdateAssetLinkage(%s [%u]): unlink asset matched by MAC to this object, but by serial to other object"), object->getName(), object->getId());
               UnlinkAsset(asset.get(), nullptr);
               nxlog_debug_tag(DEBUG_TAG_ASSET_MGMT_LINK, 6, _T("UpdateAssetLinkage(%s [%u]): unlinked from asset %s [%u]"), object->getName(), object->getId(), asset->getName(), asset->getId());
               object->sendPollerMsg(POLLER_WARNING _T("   Unlinked from asset \"%s\"\r\n"), asset->getName());
            }
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG_ASSET_MGMT_LINK, 6, _T("UpdateAssetLinkage(%s [%u]: match will be done by MAC address"), object->getName(), object->getId());
            UpdateAssetLinkageInternal(object, macAttributeName, macAddress.toString(), _T("MAC address"), true);
         }
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG_ASSET_MGMT_LINK, 6, _T("UpdateAssetLinkage(%s [%u]: cannot get MAC address from object"), object->getName(), object->getId());
      }
   }
   else
   {
      UpdateAssetLinkageInternal(object, serialAttributeName, serialNumber, _T("serial number"), false);
   }
}

/**
 * Export asset management schema
 */
void ExportAssetManagementSchema(TextFileWriter& xml, const StringList& attributeNames)
{
   s_schemaLock.readLock();
   for (int i = 0; i < attributeNames.size(); i++)
   {
      AssetAttribute *attribute = s_schema.get(attributeNames.get(i));
      if (attribute != nullptr)
      {
         attribute->createExportRecord(xml);
      }
   }
   s_schemaLock.unlock();
}

/**
 * Import asset attributes
 */
void ImportAssetManagementSchema(const ConfigEntry& root, bool overwrite, ImportContext *context, bool nxslV5)
{
   s_schemaLock.writeLock();

   unique_ptr<ObjectArray<ConfigEntry>> assetAttrDef = root.getSubEntries(_T("attribute"));
   for(int i = 0; i < assetAttrDef->size(); i++)
   {
      const ConfigEntry *config = assetAttrDef->get(i);
      const TCHAR *name = config->getSubEntryValue(_T("name"));
      if (name == nullptr)
      {
         context->log(NXLOG_ERROR, _T("ImportAssetManagementSchema()"), _T("No name specified"));
         continue;
      }
      else if (!RegexpMatch(name, _T("^[A-Za-z$_][A-Za-z0-9$_]*$"), true))
      {
         context->log(NXLOG_ERROR, _T("ImportAssetManagementSchema()"), _T("Invalid name format"));
         continue;
      }

      AssetAttribute *attribute = s_schema.get(name);
      if (attribute == nullptr || overwrite)
      {
         if (attribute == nullptr)
         {
            context->log(NXLOG_INFO, _T("ImportAssetManagementSchema()"), _T("Asset attribute \"%s\" created"), name);
         }
         else
         {
            context->log(NXLOG_INFO, _T("ImportAssetManagementSchema()"), _T("Found existing asset attribute \"%s\" (overwrite)"), name);
         }

         attribute = new AssetAttribute(name, *config, nxslV5);
         if (attribute->saveToDatabase())
         {
            s_schema.set(name, attribute);

            NXCPMessage notificationMessage(CMD_UPDATE_ASSET_ATTRIBUTE, 0);
            attribute->fillMessage(&notificationMessage, VID_AM_ATTRIBUTES_BASE);
            NotifyClientSessions(notificationMessage);
         }
      }
      else
      {
         context->log(NXLOG_INFO, _T("ImportAssetManagementSchema()"), _T("Found existing asset attribute \"%s\" (skipping)"), name);
      }
   }

   s_schemaLock.unlock();
}
