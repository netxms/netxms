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
** File: asset.cpp
**
**/

#include "nxcore.h"
#include <asset_management.h>

/**
 * Redefined status calculation for asset group
 */
void AssetGroup::calculateCompoundStatus(bool forcedRecalc)
{
   m_status = STATUS_NORMAL;
}

/**
 * Called by client session handler to check if threshold summary should be shown for this object.
 */
bool AssetGroup::showThresholdSummary() const
{
   return false;
}

/**
 * Asset object constructor
 */
Asset::Asset() : NetObj()
{
   m_linkedObjectId = 0;
   m_lastUpdateTimestamp = 0;
   m_lastUpdateUserId = 0;
   m_status = STATUS_NORMAL;
}

/**
 * Asset object constructor
 */
Asset::Asset(const TCHAR *name, const StringMap& properties) : NetObj(), m_properties(properties)
{
   _tcslcpy(m_name, name, MAX_OBJECT_NAME);
   m_linkedObjectId = 0;
   m_lastUpdateTimestamp = 0;
   m_lastUpdateUserId = 0;
   m_status = STATUS_NORMAL;
}

/**
 * Asset object destructor
 */
Asset::~Asset()
{
}

/**
 * Load asset object from database
 */
bool Asset::loadFromDatabase(DB_HANDLE hdb, uint32_t id, DB_STATEMENT *preparedStatements)
{
   m_id = id;

   if (!loadCommonProperties(hdb, preparedStatements))
      return false;

   DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT linked_object_id,last_update_timestamp,last_update_uid FROM assets WHERE id=?"));
   if (hStmt == nullptr)
      return false;

   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);

   bool success = false;
   DB_RESULT hResult = DBSelectPrepared(hStmt);
   if (hResult != nullptr)
   {
      if (DBGetNumRows(hResult) != 0)
      {
         m_linkedObjectId = DBGetFieldULong(hResult, 0, 0);
         m_lastUpdateTimestamp = DBGetFieldULong(hResult, 0, 1);
         m_lastUpdateUserId = DBGetFieldULong(hResult, 0, 2);
         success = true;
      }
      DBFreeResult(hResult);
   }
   DBFreeStatement(hStmt);

   if (success)
   {
      success = false;
      hStmt = DBPrepare(hdb, _T("SELECT attr_name,value FROM asset_properties WHERE asset_id=?"));
      if (hStmt != nullptr)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
         hResult = DBSelectPrepared(hStmt);
         if (hResult != nullptr)
         {
            int numRows = DBGetNumRows(hResult);
            for (int i = 0; i < numRows; i++)
            {
               m_properties.setPreallocated(DBGetField(hResult, i, 0, nullptr, 0), DBGetField(hResult, i, 1, nullptr, 0));
            }
            DBFreeResult(hResult);
            success = true;
         }
         DBFreeStatement(hStmt);
      }
   }

   return success;
}

/**
 * Save asset object to database
 */
bool Asset::saveToDatabase(DB_HANDLE hdb)
{
   bool success = super::saveToDatabase(hdb);
   if (success && (m_modified & MODIFY_ASSET_PROPERTIES))
   {
      static const TCHAR *columns[] = { _T("linked_object_id"), _T("last_update_timestamp"), _T("last_update_uid"), nullptr };

      DB_STATEMENT hStmt = DBPrepareMerge(hdb, _T("assets"), _T("id"), m_id, columns);
      if (hStmt != nullptr)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_linkedObjectId);
         DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(m_lastUpdateTimestamp));
         DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, m_lastUpdateUserId);
         DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, m_id);
         success = DBExecute(hStmt);
         DBFreeStatement(hStmt);
      }
      else
      {
         success = false;
      }

      if (success)
         success = executeQueryOnObject(hdb, _T("DELETE FROM asset_properties WHERE asset_id=?"));

      if (success)
      {
         hStmt = DBPrepare(hdb, _T("INSERT INTO asset_properties (asset_id,attr_name,value) VALUES (?,?,?)"), m_properties.size() > 1);
         if (hStmt != nullptr)
         {
            DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
            for (KeyValuePair<const TCHAR> *p : m_properties)
            {
               DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, p->key, DB_BIND_STATIC);
               DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, p->value, DB_BIND_STATIC);
               if (!DBExecute(hStmt))
               {
                  success = false;
                  break;
               }
            }
            DBFreeStatement(hStmt);
         }
         else
         {
            success = false;
         }
      }
   }

   return success;
}

/**
 * Delete asset object from database
 */
bool Asset::deleteFromDatabase(DB_HANDLE hdb)
{
   bool success = super::deleteFromDatabase(hdb);
   if (success)
      success = executeQueryOnObject(hdb, _T("DELETE FROM asset_properties WHERE asset_id=?"));
   if (success)
      success = executeQueryOnObject(hdb, _T("DELETE FROM assets WHERE id=?"));
   return success;
}

/**
 * Fill message with asset data
 */
void Asset::fillMessageLocked(NXCPMessage *msg, uint32_t userId)
{
   super::fillMessageLocked(msg, userId);
   msg->setField(VID_LINKED_OBJECT, m_linkedObjectId);
   msg->setFieldFromTime(VID_LAST_UPDATE_TIMESTAMP, m_lastUpdateTimestamp);
   msg->setField(VID_LAST_UPDATE_UID, m_lastUpdateUserId);
   m_properties.fillMessage(msg, VID_ASSET_PROPERTIES_BASE, VID_NUM_ASSET_PROPERTIES);
}

/**
 * Modify asset object from message
 */
uint32_t Asset::modifyFromMessageInternal(const NXCPMessage& msg, ClientSession *session)
{
   if (msg.isFieldExist(VID_LINKED_OBJECT))
      m_linkedObjectId = msg.getFieldAsUInt32(VID_LINKED_OBJECT);

   return super::modifyFromMessageInternal(msg, session);
}

/**
 * Create NXSL object for this object
 */
NXSL_Value *Asset::createNXSLObject(NXSL_VM *vm)
{
   return vm->createValue(vm->createObject(&g_nxslAssetClass, new shared_ptr<Asset>(self())));
}

/**
 * Serialize object to JSON
 */
json_t *Asset::toJson()
{
   json_t *root = super::toJson();

   lockProperties();
   json_object_set_new(root, "linkedObjectId", json_integer(m_linkedObjectId));
   json_object_set_new(root, "lastUpdateTimestamp", json_integer(m_lastUpdateTimestamp));
   json_object_set_new(root, "lastUpdateUserId", json_integer(m_lastUpdateUserId));

   json_t *properties = json_object();
   for(KeyValuePair<const TCHAR> *p : m_properties)
   {
      char name[256];
      tchar_to_utf8(p->key, -1, name, sizeof(name));
      json_object_set_new(properties, name, json_string_t(p->value));
   }
   json_object_set_new(root, "properties", properties);

   unlockProperties();
   return root;
}

/**
 * Redefined status calculation for asset
 */
void Asset::calculateCompoundStatus(bool forcedRecalc)
{
   m_status = STATUS_NORMAL;
}

/**
 * Prepare object for deletion. Method should return only
 * when object deletion is safe
 */
void Asset::prepareForDeletion()
{
   UnlinkAsset(this, nullptr);
}

/**
 * Get property
 */
SharedString Asset::getProperty(const TCHAR *attr)
{
   lockProperties();
   SharedString result = m_properties.get(attr);
   unlockProperties();
   return result;
}

/**
 * Set property
 */
std::pair<uint32_t, String> Asset::setProperty(const TCHAR *attr, const TCHAR *value, uint32_t userId)
{
   std::pair<uint32_t, String> result = ValidateAssetPropertyValue(attr, value);
   if (result.first == RCC_SUCCESS)
   {
      lockProperties();
      SharedString oldValue = m_properties.get(attr);
      m_properties.set(attr, value);
      if (oldValue.isNull() || _tcscmp(oldValue, value))
      {
         WriteAssetChangeLog(m_id, attr, oldValue.isNull() ? AssetOperation::Create : AssetOperation::Update, oldValue, value, userId, m_linkedObjectId);
         m_lastUpdateTimestamp = time(nullptr);
         m_lastUpdateUserId = userId;
         setModified(MODIFY_ASSET_PROPERTIES);
      }
      unlockProperties();
   }
   return result;
}

/**
 * Delete property
 */
uint32_t Asset::deleteProperty(const TCHAR *name, uint32_t userId)
{
   uint32_t result;
   if (!IsMandatoryAssetProperty(name))
   {
      lockProperties();
      bool changed = m_properties.contains(name);
      if (changed)
      {
         SharedString oldValue = m_properties.get(name);
         m_properties.remove(name);
         WriteAssetChangeLog(m_id, name, AssetOperation::Delete, oldValue, nullptr, userId, m_linkedObjectId);
         m_lastUpdateTimestamp = time(nullptr);
         m_lastUpdateUserId = userId;
         setModified(MODIFY_ASSET_PROPERTIES);
         result = RCC_SUCCESS;
      }
      else
      {
         result = RCC_UNKNOWN_ATTRIBUTE;
      }
      unlockProperties();
   }
   else
   {
      result = RCC_MANDATORY_ATTRIBUTE;
   }
   return result;
}

/**
 * Delete cached property
 */
void Asset::deleteCachedProperty(const TCHAR *name)
{
   lockProperties();
   bool changed = m_properties.contains(name);
   if (changed)
      m_properties.remove(name);
   unlockProperties();
   if (changed)
   {
      setModified(MODIFY_RUNTIME);  // Will initiate notification of clients
   }
}

/**
 * Check if stored value of given property value is the same as provided one
 */
bool Asset::isSamePropertyValue(const TCHAR *name, const TCHAR *value) const
{
   bool equal = false;
   lockProperties();
   const TCHAR *currentValue = m_properties.get(name);
   if (currentValue != nullptr)
      equal = !_tcscmp(currentValue, value);
   unlockProperties();
   return equal;
}

/**
 * Get asset value as NXSL value
 */
NXSL_Value *Asset::getPropertyValueForNXSL(NXSL_VM *vm, const TCHAR *name) const
{
   bool isBoolean = IsBooleanAssetProperty(name);
   NXSL_Value *nxslValue = nullptr;

   lockProperties();
   const TCHAR *value = m_properties.get(name);
   if (value != nullptr)
   {
      if (isBoolean)
         nxslValue = vm->createValue(!_tcsicmp(value, _T("yes")) || !_tcsicmp(value, _T("true")) || !_tcsicmp(value, _T("on")) || (_tcstol(value, nullptr, 0) != 0));
      else
         nxslValue = vm->createValue(value);
   }
   else if (IsValidAssetPropertyName(name))
   {
      nxslValue = vm->createValue();
   }
   unlockProperties();

   return nxslValue;
}

/**
 * Dump properties to server console
 */
void Asset::dumpProperties(ServerConsole *console) const
{
   lockProperties();
   if (!m_properties.isEmpty())
   {
      console->print(_T("   Properties:\n"));
      for(KeyValuePair<const TCHAR> *p : m_properties)
         console->printf(_T("      %-24s = %s\n"), p->key, p->value);
   }
   else
   {
      console->print(_T("   No properties defined\n"));
   }
   unlockProperties();
}

/**
 * Update asset property
 */
static inline void UpdateProperty(const AssetPropertyAutofillContext *context, Asset *asset, const shared_ptr<NetObj>& object, const TCHAR *newValue)
{
   SharedString oldValue = asset->getProperty(context->name);
   std::pair<uint32_t, String> result = asset->setProperty(context->name, newValue, 0);
   if (result.first == RCC_SUCCESS)
   {
      nxlog_debug_tag(DEBUG_TAG_ASSET_MGMT, 5, _T("Asset::autoFillProperties(%s [%u]): asset property \"%s\" set to \"%s\""), asset->getName(), asset->getId(), context->name, newValue);
      object->sendPollerMsg(_T("   Asset property \"%s\" set to \"%s\"\r\n"), context->name, newValue);
   }
   else if (result.first != RCC_UNKNOWN_ATTRIBUTE)
   {
      nxlog_debug_tag(DEBUG_TAG_ASSET_MGMT, 5, _T("Asset::autoFillProperties(%s [%u]): automatic update of asset property \"%s\" with value \"%s\" failed (%s)"),
            asset->getName(), asset->getId(), context->name, newValue, static_cast<const TCHAR*>(result.second));
      object->sendPollerMsg(POLLER_WARNING _T("   Automatic update of asset property \"%s\" with value \"%s\" failed (%s)\r\n"), context->name, newValue, static_cast<const TCHAR*>(result.second));
      EventBuilder(EVENT_ASSET_AUTO_UPDATE_FAILED, *object)
         .param(_T("name"), context->name)
         .param(_T("displayName"), GetAssetAttributeDisplayName(context->name))
         .param(_T("dataType"), static_cast<int32_t>(context->dataType))
         .param(_T("currValue"), oldValue)
         .param(_T("newValue"), newValue)
         .param(_T("reason"), result.second)
         .post();
   }
}

/**
 * Fill asset properties with autofill scripts
 */
void Asset::autoFillProperties()
{
   nxlog_debug_tag(DEBUG_TAG_ASSET_MGMT, 6, _T("Asset::autoFillProperties(%s [%u]): start asset auto fill"), m_name, m_id);

   shared_ptr<NetObj> object = FindObjectById(m_linkedObjectId);
   if (object == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG_ASSET_MGMT, 6, _T("Asset::autoFillProperties(%s [%u]): asset auto fill aborted - linked object [%u] not found"), m_name, m_id, m_linkedObjectId);
      return;
   }

   unique_ptr<ObjectArray<AssetPropertyAutofillContext>> autoFillContexts = PrepareAssetPropertyAutofill(*this, object);

   lockProperties();
   StringMap propertiesCopy(m_properties);
   unlockProperties();

   for (AssetPropertyAutofillContext *context : *autoFillContexts)
   {
      if (context->vm != nullptr)
      {
         NXSL_VM *vm = context->vm;
         vm->setGlobalVariable("$asset", createNXSLObject(vm));
         vm->setGlobalVariable("$name", vm->createValue(context->name));
         vm->setGlobalVariable("$value", vm->createValue(propertiesCopy.get(context->name)));
         if (context->enumValues != nullptr)
         {
            vm->setGlobalVariable("$enumValues", vm->createValue(new NXSL_Array(vm, *context->enumValues)));
         }
         if (vm->run())
         {
            NXSL_Value *result = vm->getResult();
            if (!result->isNull())
            {
               UpdateProperty(context, this, object, result->getValueAsCString());
            }
            else
            {
               nxlog_debug_tag(DEBUG_TAG_ASSET_MGMT, 6, _T("Asset::autoFillProperties(%s [%u]): autofill script for attribute \"%s\" returned null, no changes will be made"), m_name, m_id, context->name);
            }
         }
         else
         {
            if (vm->getErrorCode() == NXSL_ERR_EXECUTION_ABORTED)
            {
               nxlog_debug_tag(DEBUG_TAG_ASSET_MGMT, 6, _T("Asset::autoFillProperties(%s [%u]): automatic update of asset property \"%s\" aborted"), m_name, m_id, context->name);
            }
            else
            {
               nxlog_debug_tag(DEBUG_TAG_ASSET_MGMT, 6, _T("Asset::autoFillProperties(%s [%u]): automatic update of asset property \"%s\" failed (%s)"),
                     m_name, m_id, context->name, vm->getErrorText());
               ReportScriptError(SCRIPT_CONTEXT_ASSET_MGMT, this, 0, vm->getErrorText(), _T("AssetAttribute::%s::autoFill"), context->name);
            }
         }
      }
      else
      {
         SharedString oldValue = getProperty(context->name);
         if (oldValue.isEmpty() || context->systemType == AMSystemType::IPAddress)
            UpdateProperty(context, this, object, context->newValue);
      }
   }
   nxlog_debug_tag(DEBUG_TAG_ASSET_MGMT, 6, _T("Asset::autoFillProperties(%s [%u]): asset auto fill completed"), m_name, m_id);
}
