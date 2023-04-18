/*
** NetXMS - Network Management System
** Copyright (C) 2003-2023 Raden Solutions
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
 * attr_name,display_name,data_type,is_mandatory,is_unique,autofill_script,range_min,range_max,sys_type
 */
AssetAttribute::AssetAttribute(DB_RESULT result, int row)
{
   m_name = DBGetField(result, row, 0, nullptr, 0);
   m_displayName = DBGetField(result, row, 1, nullptr, 0);
   m_dataType = static_cast<AMDataType>(DBGetFieldLong(result, row, 2));
   m_isMandatory = DBGetFieldLong(result, row, 3) ? true : false;
   m_isUnique = DBGetFieldLong(result, row, 4) ? true : false;
   m_autofillScriptSource = nullptr;
   m_autofillScript = nullptr;
   setScript(DBGetField(result, row, 5, nullptr, 0));
   m_rangeMin = DBGetFieldLong(result, row, 6);
   m_rangeMax = DBGetFieldLong(result, row, 7);
   m_systemType = static_cast<AMSystemType>(DBGetFieldLong(result, row, 8));
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
AssetAttribute::AssetAttribute(const TCHAR *name, const ConfigEntry& entry)
{
   m_name = MemCopyString(name);
   m_displayName = MemCopyString(entry.getSubEntryValue(_T("displayName")));
   m_dataType = static_cast<AMDataType>(entry.getSubEntryValueAsInt(_T("dataType"), 0 , 0));
   m_isMandatory = entry.getSubEntryValueAsBoolean(_T("isMandatory"), 0, false);
   m_isUnique = entry.getSubEntryValueAsBoolean(_T("isUnique"), 0, false);
   m_autofillScriptSource = nullptr;
   m_autofillScript = nullptr;
   setScript(MemCopyString(entry.getSubEntryValue(_T("script"))));
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
      stmt = DBPrepare(db, _T("UPDATE am_attributes SET display_name=?,data_type=?,is_mandatory=?,is_unique=?,autofill_script=?,range_min=?,range_max=?,sys_type=? WHERE attr_name=?"));
   }
   else
   {
      stmt = DBPrepare(db, _T("INSERT INTO am_attributes (display_name,data_type,is_mandatory,is_unique,autofill_script,range_min,range_max,sys_type,attr_name) VALUES (?,?,?,?,?,?,?,?,?)"));
   }
   if (stmt != nullptr)
   {
      DBBind(stmt, 1, DB_SQLTYPE_VARCHAR, m_displayName, DB_BIND_STATIC);
      DBBind(stmt, 2, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(m_dataType));
      DBBind(stmt, 3, DB_SQLTYPE_VARCHAR, m_isMandatory ? _T("1") : _T("0"), DB_BIND_STATIC);
      DBBind(stmt, 4, DB_SQLTYPE_VARCHAR, m_isUnique ? _T("1") : _T("0"), DB_BIND_STATIC);
      DBBind(stmt, 5, DB_SQLTYPE_TEXT, m_autofillScriptSource, DB_BIND_STATIC);
      DBBind(stmt, 6, DB_SQLTYPE_INTEGER, m_rangeMin);
      DBBind(stmt, 7, DB_SQLTYPE_INTEGER, m_rangeMax);
      DBBind(stmt, 8, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(m_systemType));
      DBBind(stmt, 9, DB_SQLTYPE_VARCHAR, m_name, DB_BIND_STATIC);

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
         TCHAR error[256];
         NXSL_ServerEnv env;
         m_autofillScript = NXSLCompile(m_autofillScriptSource, error, 256, nullptr, &env);
         if (m_autofillScript == nullptr)
         {
            ReportScriptError(_T("AssetAttribute"), nullptr, 0, error, _T("AMAutoFill::%s"), m_name);
         }
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
void AssetAttribute::createExportRecord(StringBuffer &xml)
{
   xml.append(_T("\t\t<attribute>\n\t\t\t<name>"));
   xml.append(m_name);
   xml.append(_T("</name>\n\t\t\t<displayName>"));
   xml.append(m_displayName);
   xml.append(_T("</displayName>\n\t\t\t<dataType>"));
   xml.append(static_cast<int32_t>(m_dataType));
   xml.append(_T("</dataType>\n\t\t\t<isMandatory>"));
   xml.append(m_isMandatory ? 1 : 0);
   xml.append(_T("</isMandatory>\n\t\t\t<isUnique>"));
   xml.append(m_isUnique ? 1 : 0);
   xml.append(_T("</isUnique>\n\t\t\t<script>"));
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

   DB_RESULT result = DBSelect(db, _T("SELECT attr_name,display_name,data_type,is_mandatory,is_unique,autofill_script,range_min,range_max,sys_type FROM am_attributes"));
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

      result = RCC_SUCCESS;

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
unique_ptr<ObjectArray<AssetPropertyAutofillContext>> PrepareAssetPropertyAutofill(const shared_ptr<Asset>& asset)
{
   auto contexts = make_unique<ObjectArray<AssetPropertyAutofillContext>>(0, 16, Ownership::True);
   s_schemaLock.readLock();
   for (KeyValuePair<AssetAttribute> *a : s_schema)
   {
      if (!a->value->hasScript())
         continue;

      NXSL_VM *vm = CreateServerScriptVM(a->value->getScript(), asset);
      if (vm != nullptr)
      {
         contexts->add(new AssetPropertyAutofillContext(a->key, a->value->getDataType(), vm));
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG_ASSET_MGMT, 4, _T("PrepareAssetPropertyAutofill(%s [%u]): Script load failed"), asset->getName(), asset->getId());
         ReportScriptError(SCRIPT_CONTEXT_ASSET_MGMT, asset.get(), 0, _T("Script load failed"), _T("AssetAttribute::%s::autoFill"), a->key);
      }
   }
   s_schemaLock.unlock();
   return contexts;
}

/**
 * Link asset to object
 */
void LinkAsset(const shared_ptr<Asset>& asset, const shared_ptr<NetObj>& object, ClientSession *session)
{
   if (asset->getLinkedObjectId() == object->getId())
      return;

   UnlinkAsset(asset, session);

   asset->setLinkedObjectId(object->getId());
   object->setAssetId(asset->getId());
   session->writeAuditLog(AUDIT_OBJECTS, true, asset->getId(), _T("Asset linked with object %s [%u]"), object->getName(), object->getId());
   session->writeAuditLog(AUDIT_OBJECTS, true, object->getId(), _T("Object linked with asset %s [%u]"), asset->getName(), asset->getId());
}

/**
 * Unlink asset from object it is currently linked to
 */
void UnlinkAsset(const shared_ptr<Asset>& asset, ClientSession *session)
{
   if (asset->getLinkedObjectId() == 0)
      return;

   shared_ptr<NetObj> object = FindObjectById(asset->getLinkedObjectId());
   if (object != nullptr)
   {
      object->setAssetId(0);
      session->writeAuditLog(AUDIT_OBJECTS, true, object->getId(), _T("Link with asset %s [%u] removed"), asset->getName(), asset->getId());
      session->writeAuditLog(AUDIT_OBJECTS, true, asset->getId(), _T("Link with object %s [%u] removed"), object->getName(), object->getId());
   }
}

/**
 * Export asset management schema
 */
void ExportAssetManagementSchema(StringBuffer &xml, const StringList &attributeNames)
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
void ImportAssetManagementSchema(const ConfigEntry& root, bool overwrite)
{
   s_schemaLock.writeLock();

   unique_ptr<ObjectArray<ConfigEntry>> assetAttrDef = root.getSubEntries(_T("attribute"));
   for(int i = 0; i < assetAttrDef->size(); i++)
   {
      const ConfigEntry *config = assetAttrDef->get(i);
      const TCHAR *name = config->getSubEntryValue(_T("name"));
      if (name == nullptr)
      {
         nxlog_debug_tag(_T("import"), 4, _T("ImportAssetManagementSchema: no name specified"));
         continue;
      }
      else if (!RegexpMatch(name, _T("^[A-Za-z$_][A-Za-z0-9$_]*$"), true))
      {
         nxlog_debug_tag(_T("import"), 4, _T("ImportAssetManagementSchema: invalid name format"));
         continue;
      }

      AssetAttribute *attribute = s_schema.get(name);
      if (attribute == nullptr || overwrite)
      {
         if (attribute == nullptr)
         {
            nxlog_debug_tag(_T("import"), 4, _T("ImportAssetManagementSchema: asset attribute \"%s\" created"), name);
         }
         else
         {
            nxlog_debug_tag(_T("import"), 4, _T("ImportAssetManagementSchema: found existing asset attribute \"%s\" (overwrite)"), name);
         }

         attribute = new AssetAttribute(name, *config);
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
         nxlog_debug_tag(_T("import"), 4, _T("ImportAssetManagementSchema: found existing asset attribute \"%s\" (skipping)"), name);
      }
   }

   s_schemaLock.unlock();
}
