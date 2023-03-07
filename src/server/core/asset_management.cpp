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

#define DEBUG_TAG _T("am")
#define AUDIT_AM_ATTRIBUTE _T("ASSERT_MANAGEMENT")


enum class AMDataType
{
   String = 0,
   Integer = 1,
   Number = 2,
   Boolean = 3,
   Enum = 4,
   MacAddress = 5,
   IPAddress = 6,
   UUID = 7,
   ObjectReference = 8
};

enum class AMSystemType
{
   None = 0,
   Serial = 1,
   IPAddress = 2,
   MacAddress = 3,
   Vendor = 4,
   Model = 5

};

/**
 * Asset management attribute
 */
class AssetManagementAttribute
{
private:
   TCHAR *m_name;
   TCHAR *m_displayName;
   AMDataType m_dataType;
   bool m_isMandatory;
   bool m_isUnique;
   TCHAR *m_autofillScriptSource;
   NXSL_Program *m_autofillScript;
   int32_t m_rangeMin;
   int32_t m_rangeMax;
   AMSystemType m_systemType;
   StringMap m_enumMapping;

   bool saveToDatabase();
   void setScript(TCHAR *script);
   AssetManagementAttribute(TCHAR *name);

public:
   static AssetManagementAttribute *create(const NXCPMessage &msg);

   AssetManagementAttribute(DB_RESULT result, int row);
   ~AssetManagementAttribute();
   void addEnumMapping(DB_RESULT result);
   void fillMessage(NXCPMessage *msg, uint32_t baseId);
   void update(const NXCPMessage &msg);
   bool deleteFromDatabase();

   json_t *toJson();

   const TCHAR *getName() const { return m_name; }
   AMDataType getDataType() const { return m_dataType; }
   bool isMandotory() const { return m_isMandatory; }
   bool isUnique() const { return m_isUnique; }
   int32_t getMinRange() { return m_rangeMin; }
   int32_t getMaxRange() { return m_rangeMax; }

   bool isEnumValid(const TCHAR *value);
   bool isRangeSet() { return m_rangeMin != 0 && m_rangeMax != 0; }
};

/**
 * Create asset management attribute form database.
 * Fields:
 * attr_name,display_name,data_type,is_mandatory,is_unique,autofill_script,range_min,range_max,sys_type
 */
AssetManagementAttribute::AssetManagementAttribute(DB_RESULT result, int row) : m_enumMapping()
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

AssetManagementAttribute::AssetManagementAttribute(TCHAR *name) : m_enumMapping()
{
   m_name = name;
   m_displayName = nullptr;
   m_dataType = AMDataType::String;
   m_isMandatory = false;
   m_isUnique = false;
   m_autofillScriptSource = nullptr;
   m_autofillScript = nullptr;
   m_rangeMin = 0;
   m_rangeMax = 0;
   m_systemType = AMSystemType::None;
}


AssetManagementAttribute *AssetManagementAttribute::create(const NXCPMessage &msg)
{
   AssetManagementAttribute *conf = new AssetManagementAttribute(msg.getFieldAsString(VID_NAME));
   conf->update(msg);
   return conf;
}

/**
 * Add enum value to description mapping
 */
void AssetManagementAttribute::addEnumMapping(DB_RESULT result)
{
   int rowCount = DBGetNumRows(result);
   for (int i = 0; i < rowCount; i++)
   {
      m_enumMapping.setPreallocated(DBGetField(result, i, 1, nullptr, 0), DBGetField(result, i, 2, nullptr, 0));
   }
}

/**
 * Asset management attribute distructor
 */
AssetManagementAttribute::~AssetManagementAttribute()
{
   MemFree(m_autofillScriptSource);
   MemFree(m_name);
   MemFree(m_displayName);
   delete m_autofillScript;
}

/**
 * Fill message with attribute data
 */
void AssetManagementAttribute::fillMessage(NXCPMessage *msg, uint32_t baseId)
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
   msg->setField(baseId++, m_enumMapping.size());
   for(KeyValuePair<const TCHAR> *Attribute : m_enumMapping)
   {
      msg->setField(baseId++, Attribute->key);
      msg->setField(baseId++, Attribute->value);
   }
}

/**
 * Update confiruation form message
 */
void AssetManagementAttribute::update(const NXCPMessage &msg)
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

   m_enumMapping.clear();
   uint32_t base = VID_AM_ENUM_MAP_BASE;
   int count = msg.getFieldAsUInt32(VID_ENUM_COUNT);
   for (int i = 0; i < count; i++)
   {
      m_enumMapping.setPreallocated(msg.getFieldAsString(base++), msg.getFieldAsString(base++));
   }

   saveToDatabase();
}

/**
 * Save to database
 */
bool AssetManagementAttribute::saveToDatabase()
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
      stmt = DBPrepare(db, _T("INSERT INTO am_enum_values (attr_name,attr_value,display_name) VALUES (?,?,?)"), m_enumMapping.size() > 1);
      if (stmt != nullptr)
      {
         DBBind(stmt, 1, DB_SQLTYPE_VARCHAR, m_name, DB_BIND_STATIC);
         for(KeyValuePair<const TCHAR> *Attribute : m_enumMapping)
         {
            DBBind(stmt, 2, DB_SQLTYPE_VARCHAR, Attribute->key, DB_BIND_STATIC);
            DBBind(stmt, 3, DB_SQLTYPE_VARCHAR, Attribute->value, DB_BIND_STATIC);
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
bool AssetManagementAttribute::deleteFromDatabase()
{
   bool success = false;
   DB_HANDLE db = DBConnectionPoolAcquireConnection();
   DBBegin(db);

   success = ExecuteQueryOnObject(db, m_name, _T("DELETE FROM am_attributes WHERE attr_name=?"));

   if (success)
   {
      success = ExecuteQueryOnObject(db, m_name, _T("DELETE FROM am_enum_values WHERE attr_name=?"));
   }

   if (success)
   {
      success = ExecuteQueryOnObject(db, m_name, _T("DELETE FROM am_object_data WHERE attr_name=?"));
   }

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
void AssetManagementAttribute::setScript(TCHAR *script)
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
            ReportScriptError(_T("AssetManagementAttribute"), nullptr, 0, error, _T("AMAutoFill::%s"), m_name);
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
json_t *AssetManagementAttribute::toJson()
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
   return root;
}


bool AssetManagementAttribute::isEnumValid(const TCHAR *value)
{
   return m_enumMapping.contains(value);
}

static StringObjectMap<AssetManagementAttribute> s_amSchema(Ownership::True);
static RWLock s_amAttributeLock;

/***************************************************
 * Asset management attribute instance implementation
 ***************************************************/

/**
 * Asset management attribute target constructor
 */
Asset::Asset(NetObj *_this) : m_amMutexProperties(MutexType::FAST), m_instances()
{
   m_this = _this;
   _this->m_asAsset = this;
}

/**
 * Assert management attribute target modify attribute instances form message
 */
void Asset::modifyFromMessage(const NXCPMessage& msg)
{
   internalLock();
   if (msg.isFieldExist(VID_AM_DATA_BASE))
   {
      m_instances = StringMap(msg, VID_AM_DATA_BASE, VID_AM_COUNT);
   }
   internalUnlock();
}

/**
 * Assert management attribute target modify attribute instance form message
 */
std::pair<uint32_t, String> Asset::modifyAassetData(const TCHAR *name, const TCHAR *value)
{
   std::pair<uint32_t, String> result = validateInput(name, value);
   if (result.first == RCC_SUCCESS)
   {
      internalLock();
      m_instances.set(name, value);
      internalUnlock();
      m_this->setModified(MODIFY_AM_INSTANCES, true);
   }

   return result;
}

/**
 * Assert management attribute target delete attribute instance form message
 */
uint32_t Asset::deleteAassetData(const TCHAR *name)
{
   uint32_t result = RCC_SUCCESS;
   s_amAttributeLock.readLock();
   bool canBeDeleted = true;
   AssetManagementAttribute *attr = s_amSchema.get(name);
   if (attr != nullptr)
   {
      canBeDeleted = !attr->isMandotory();
   }
   s_amAttributeLock.unlock();
   if (canBeDeleted)
   {
      internalLock();
      m_instances.remove(name);
      internalUnlock();
      m_this->setModified(MODIFY_AM_INSTANCES, true);
   }
   else
   {
      result = RCC_ATTRIBUTE_REQUIRED;
   }
   return result;
}

/**
 * Assert management attribute target fill message
 */
void Asset::getAassetData(NXCPMessage* msg) const
{
   internalLock();
   m_instances.fillMessage(msg, VID_AM_DATA_BASE, VID_AM_COUNT);
   internalUnlock();
}

bool Asset::loadFromDatabase(DB_HANDLE db, uint32_t objectId)
{
   bool success = false;
   DB_STATEMENT stmt = DBPrepare(db, _T("SELECT attr_name,attr_value FROM am_object_data WHERE object_id=?"));
   if (stmt != nullptr)
   {
      DBBind(stmt, 1, DB_SQLTYPE_INTEGER, objectId);
      DB_RESULT result = DBSelectPrepared(stmt);
      if (result != nullptr)
      {
         int numRows = DBGetNumRows(result);
         for (int i = 0; i < numRows; i++)
         {
            m_instances.setPreallocated(DBGetField(result, i, 0, nullptr, 0), DBGetField(result, i, 1, nullptr, 0));
         }
         DBFreeResult(result);
         success = true;
      }
      DBFreeStatement(stmt);
   }
   return success;
}

bool Asset::saveToDatabase(DB_HANDLE db)
{
   internalLock();
   bool success = m_this->executeQueryOnObject(db, _T("DELETE FROM am_object_data WHERE object_id=?"));
   if (success)
   {
      DB_STATEMENT stmt = DBPrepare(db, _T("INSERT INTO am_object_data (object_id,attr_name,attr_value) VALUES (?,?,?)"), m_instances.size() > 1);
      if (stmt != nullptr)
      {
         DBBind(stmt, 1, DB_SQLTYPE_INTEGER, m_this->getId());
         for (KeyValuePair<const TCHAR> *instance : m_instances)
         {
            DBBind(stmt, 2, DB_SQLTYPE_VARCHAR, instance->key, DB_BIND_STATIC);
            DBBind(stmt, 3, DB_SQLTYPE_VARCHAR, instance->value, DB_BIND_STATIC);
            if (!DBExecute(stmt))
            {
               success = false;
               break;
            }
         }
         DBFreeStatement(stmt);
      }
   }
   internalUnlock();
   return success;
}

bool Asset::deleteFromDatabase(DB_HANDLE db)
{
   return m_this->executeQueryOnObject(db, _T("DELETE FROM am_object_data WHERE object_id=?"));
}

/**
 * Delete assert management attribute by name from in memory cache
 */
void Asset::deleteItemImMemory(const TCHAR *name)
{
   internalLock();
   m_instances.remove(name);
   internalUnlock();
}

void Asset::assetToJson(json_t *root)
{
   json_t *amAttributeInstances = json_array();
   internalLock();
   for(KeyValuePair<const TCHAR> *instance : m_instances)
   {
      json_t *attrInstance = json_object();
      json_object_set_new(attrInstance, "name", json_string_t(instance->key));
      json_object_set_new(attrInstance, "value", json_string_t(instance->value));
      json_array_append_new(amAttributeInstances, attrInstance);
   }
   internalUnlock();
   json_object_set_new(root, "assetManagementAttributeInstances", amAttributeInstances);
}

/**
 * Validate value
 */
std::pair<uint32_t, String> Asset::validateInput(const TCHAR *name, const TCHAR *value)
{
   StringBuffer resultText;
   s_amAttributeLock.readLock();
   AssetManagementAttribute *assetAttribute = s_amSchema.get(name);
   if (assetAttribute == nullptr)
   {
      return std::pair<uint32_t, String>(RCC_INVALID_OBJECT_ID, _T("No such asset attribute"));
   }

   //Check value
   switch (assetAttribute->getDataType())
   {
      case AMDataType::String:
         if (assetAttribute->isRangeSet())
         {
            uint32_t len = _tcslen(value);
            if (len < assetAttribute->getMinRange())
            {
               resultText = _T("String too short");
            }
            if (len > assetAttribute->getMaxRange())
            {
               resultText = _T("String too long");
            }
         }
         break;
      case AMDataType::Enum:
      {
         if (!assetAttribute->isEnumValid(value))
         {
            resultText = _T("Invalid enum value");
         }
         break;
      }
      case AMDataType::Integer:
      {
         TCHAR *error;
         uint32_t number = _tcstol(value, &error, 0);
         if (*error != 0)
         {
            resultText = _T("Not an integer");
         }
         if (assetAttribute->isRangeSet())
         {
            if(number < assetAttribute->getMinRange())
            {
               resultText = _T("Integer too small");
            }
            if(number > assetAttribute->getMaxRange())
            {
               resultText = _T("Integer too big");
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
               resultText = _T("Number too small");
            }
            if(number > assetAttribute->getMaxRange())
            {
               resultText = _T("Number too big");
            }
         }
         break;
      }
      case AMDataType::Boolean:
      {
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
         if (mac.equals(MacAddress::ZERO))
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
         uint32_t objectId = _tcstoll(value, &error, 0);
         if (*error != 0)
         {
            resultText = _T("Invalid object reference a number");
         }
         shared_ptr<NetObj> object = FindObjectById(objectId);
         if (object == nullptr)
         {
            resultText = _T("Object id not found");
         }
         break;
      }
   }
   bool shouldBeUnique = assetAttribute->isUnique();
   s_amAttributeLock.unlock();

   if (shouldBeUnique)
   {
      unique_ptr<SharedObjectArray<NetObj>> objects = g_idxObjectById.getObjects();
      for (int i = 0; i < objects->size(); i++)
      {
         shared_ptr<NetObj> object = objects->getShared(i);
         if (object->isAsset())
         {
            if(object->getAsAsset()->isAssetValueEqual(name, value))
            {
               resultText.append(_T("Duplicate value. Original value comes from: "));
               resultText.append(object->getName());
            }
         }
      }
   }

   return std::pair<uint32_t, String>(resultText.isEmpty() ? RCC_SUCCESS : RCC_VALIDATION_ERROR, resultText);
}

bool Asset::isAssetValueEqual(const TCHAR *name, const TCHAR *value)
{
   bool equal = false;
   internalLock();
   const TCHAR *objectValue = m_instances.get(name);
   if (objectValue != nullptr)
      equal = !_tcscmp(objectValue, value);
   internalUnlock();
   return equal;
}

/***************************************************
 * Asset management attribute function
 ***************************************************/


/**
 * Load all asset management attribute
 */
void LoadAMFromDatabase()
{
   DB_HANDLE db = DBConnectionPoolAcquireConnection();
   DB_RESULT result = DBSelect(db, _T("SELECT attr_name,display_name,data_type,is_mandatory,is_unique,autofill_script,"
         "range_min,range_max,sys_type FROM am_attributes"));

   DB_STATEMENT stmt = DBPrepare(db, _T("SELECT attr_value,display_name FROM am_enum_values WHERE attr_name=?"), true);
   if (result != nullptr && stmt != nullptr)
   {
      int numRows = DBGetNumRows(result);
      for (int i = 0; i < numRows; i++)
      {
         AssetManagementAttribute *conf = new AssetManagementAttribute(result, i);
         s_amSchema.set(conf->getName(), conf);

         DBBind(stmt, 1, DB_SQLTYPE_VARCHAR, conf->getName(), DB_BIND_STATIC);
         DB_RESULT mappingResult = DBSelectPrepared(stmt);
         if (mappingResult != nullptr)
         {
            conf->addEnumMapping(mappingResult);
            DBFreeResult(mappingResult);
         }
      }
   }
   DBFreeResult(result);
   DBFreeStatement(stmt);
   DBConnectionPoolReleaseConnection(db);
   nxlog_debug_tag(DEBUG_TAG, 2, _T("%d asset management attribute  loaded"), s_amSchema.size());
}

/**
 * Fill message with asset management attribute
 */
void AMFillMessage(NXCPMessage *msg)
{
   s_amAttributeLock.readLock();
   uint32_t base = VID_AM_LIST_BASE;

   msg->setField(VID_AM_COUNT, s_amSchema.size());
   for (KeyValuePair<AssetManagementAttribute> *Attribute : s_amSchema)
   {
      Attribute->value->fillMessage(msg, base);
      base += 100;
   }
   s_amAttributeLock.unlock();
}

/**
 * Create asset management attribute
 */
uint32_t AMCreateAttribute(const NXCPMessage &msg, const ClientSession &session)
{
   uint32_t result;
   s_amAttributeLock.writeLock();
   SharedString name = msg.getFieldAsSharedString(VID_NAME, 64);

   if (s_amSchema.get(name) == nullptr)
   {
      AssetManagementAttribute *attribute = AssetManagementAttribute::create(msg);
      s_amSchema.set(name, attribute);

      json_t *attrData = attribute->toJson();
      session.writeAuditLogWithValues(AUDIT_AM_ATTRIBUTE, true, 0,  nullptr, attrData,
            _T("Asset management attribute %s created"), static_cast<const TCHAR *>(name));
      json_decref(attrData);

      NXCPMessage notificationMessage(CMD_UPDATE_ASSET_MGMT_ATTRIBUTE, 0);
      attribute->fillMessage(&notificationMessage, VID_AM_LIST_BASE);
      NotifyClientSessions(notificationMessage);

      result = RCC_SUCCESS;
   }
   else
   {
      result = RCC_ALREADY_EXIST;
   }
   s_amAttributeLock.unlock();
   return result;
}

/**
 * Update asset management attribute
 */
uint32_t AMUpdateAttribute(const NXCPMessage &msg, const ClientSession &session)
{
   uint32_t result;
   s_amAttributeLock.writeLock();
   SharedString name = msg.getFieldAsSharedString(VID_NAME, 64);
   AssetManagementAttribute *attribute = s_amSchema.get(name);
   if (attribute != nullptr)
   {
      json_t *oldAttrData = attribute->toJson();
      attribute->update(msg);
      json_t *newAttrData = attribute->toJson();

      session.writeAuditLogWithValues(AUDIT_AM_ATTRIBUTE, true, 0,  oldAttrData, newAttrData,
            _T("Asset management attribute %s update"), static_cast<const TCHAR *>(name));
      json_decref(oldAttrData);
      json_decref(newAttrData);

      NXCPMessage notificationMessage(CMD_UPDATE_ASSET_MGMT_ATTRIBUTE, 0);
      attribute->fillMessage(&notificationMessage, VID_AM_LIST_BASE);
      NotifyClientSessions(notificationMessage);

      result = RCC_SUCCESS;
   }
   else
   {
      result = RCC_INVALID_OBJECT_ID;
   }
   s_amAttributeLock.unlock();
   return result;
}


/**
 * Delete asset management attribute
 */
uint32_t AMDeleteAttribute(const NXCPMessage &msg, const ClientSession &session)
{
   uint32_t result;
   s_amAttributeLock.writeLock();
   SharedString name = msg.getFieldAsSharedString(VID_NAME, 64);
   AssetManagementAttribute *attribute = s_amSchema.get(name);
   if (attribute != nullptr)
   {

      json_t *oldAttrData = attribute->toJson();
      session.writeAuditLogWithValues(AUDIT_AM_ATTRIBUTE, true, 0, oldAttrData, nullptr,
            _T("Asset management attribute %s delete"), static_cast<const TCHAR *>(name));
      json_decref(oldAttrData);

      result = RCC_SUCCESS;

      bool success = attribute->deleteFromDatabase();
      s_amSchema.remove(name);

      unique_ptr<SharedObjectArray<NetObj>> objects = g_idxObjectById.getObjects();
      for (int i = 0; i < objects->size(); i++)
      {
         shared_ptr<NetObj> object = objects->getShared(i);
         if (object->isAsset())
           object->getAsAsset()->deleteItemImMemory(name);
      }

      NXCPMessage notificationMessage(CMD_DELETE_ASSET_MGMT_ATTRIBUTE, 0);
      notificationMessage.setField(VID_NAME, name);
      NotifyClientSessions(notificationMessage);

      result = success ? RCC_SUCCESS : RCC_DB_FAILURE;
   }
   else
   {
      result = RCC_INVALID_OBJECT_ID;
   }
   s_amAttributeLock.unlock();
   return result;
}
