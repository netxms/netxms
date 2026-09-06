/*
** NetXMS - Network Management System
** Copyright (C) 2026 Raden Solutions
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
** File: facility.cpp
**
**/

#include "nxcore.h"

#define DEFAULT_SETTLEMENT_LAG 5

/**
 * Default constructor
 */
Facility::Facility() : super()
{
   m_settlementLag = DEFAULT_SETTLEMENT_LAG;
}

/**
 * Constructor from NXCP message (object creation request)
 */
Facility::Facility(const TCHAR *name, const NXCPMessage& request) : super(name)
{
   m_settlementLag = request.isFieldExist(VID_SETTLEMENT_LAG) ? request.getFieldAsInt32(VID_SETTLEMENT_LAG) : DEFAULT_SETTLEMENT_LAG;
   m_providerId = request.getFieldAsSharedString(VID_PROVIDER_ID, 63);
}

/**
 * Constructor from JSON definition (REST API)
 */
Facility::Facility(const TCHAR *name, json_t *json) : super(name)
{
   m_settlementLag = json_object_get_int32(json, "settlementLag", DEFAULT_SETTLEMENT_LAG);
   m_providerId = json_object_get_string_t(json, "providerId", _T(""));
}

/**
 * Load from database
 */
bool Facility::loadFromDatabase(DB_HANDLE hdb, uint32_t id, DB_STATEMENT *preparedStatements)
{
   if (!super::loadFromDatabase(hdb, id, preparedStatements))
      return false;

   DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT settlement_lag,provider_id FROM facilities WHERE id=?"));
   if (hStmt == nullptr)
      return false;

   bool success = false;
   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
   DB_RESULT hResult = DBSelectPrepared(hStmt);
   if (hResult != nullptr)
   {
      if (DBGetNumRows(hResult) > 0)
      {
         m_settlementLag = DBGetFieldLong(hResult, 0, 0);
         m_providerId = DBGetFieldAsSharedString(hResult, 0, 1);
         success = true;
      }
      DBFreeResult(hResult);
   }
   DBFreeStatement(hStmt);
   return success;
}

/**
 * Save to database
 */
bool Facility::saveToDatabase(DB_HANDLE hdb)
{
   bool success = super::saveToDatabase(hdb);
   if (success && (m_modified & MODIFY_OTHER))
   {
      static const TCHAR *columns[] = { _T("settlement_lag"), _T("provider_id"), nullptr };
      DB_STATEMENT hStmt = DBPrepareMerge(hdb, _T("facilities"), _T("id"), m_id, columns);
      if (hStmt != nullptr)
      {
         lockProperties();
         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_settlementLag);
         DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, m_providerId, DB_BIND_TRANSIENT, 63);
         DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, m_id);
         unlockProperties();
         success = DBExecute(hStmt);
         DBFreeStatement(hStmt);
      }
      else
      {
         success = false;
      }
   }
   return success;
}

/**
 * Delete from database
 */
bool Facility::deleteFromDatabase(DB_HANDLE hdb)
{
   bool success = super::deleteFromDatabase(hdb);
   if (success)
      success = executeQueryOnObject(hdb, _T("DELETE FROM facilities WHERE id=?"));
   return success;
}

/**
 * Fill message with object fields
 */
void Facility::fillMessageLocked(NXCPMessage *msg, uint32_t userId)
{
   super::fillMessageLocked(msg, userId);
   msg->setField(VID_SETTLEMENT_LAG, m_settlementLag);
   msg->setField(VID_PROVIDER_ID, m_providerId);
}

/**
 * Modify object from message
 */
uint32_t Facility::modifyFromMessageInternal(const NXCPMessage& msg, ClientSession *session)
{
   if (msg.isFieldExist(VID_SETTLEMENT_LAG))
      m_settlementLag = msg.getFieldAsInt32(VID_SETTLEMENT_LAG);
   if (msg.isFieldExist(VID_PROVIDER_ID))
      m_providerId = msg.getFieldAsSharedString(VID_PROVIDER_ID, 63);
   return super::modifyFromMessageInternal(msg, session);
}

/**
 * Build "facility" property group (settlement lag and provider selection)
 */
json_t *Facility::facilityConfigToJson()
{
   json_t *group = json_object();
   lockProperties();
   json_object_set_new(group, "settlementLag", json_integer(m_settlementLag));
   json_object_set_new(group, "providerId", json_string_t(m_providerId));
   json_object_set_new(group, "engineEnabled", json_boolean(!m_providerId.isEmpty()));
   unlockProperties();
   return group;
}

/**
 * Serialize object to JSON
 */
json_t *Facility::toJson(bool includeSensitiveData)
{
   json_t *root = super::toJson(includeSensitiveData);
   json_object_set_new(root, "facility", facilityConfigToJson());
   return root;
}

/**
 * Modify object from JSON document (WebAPI path). Handles the "facility" property group;
 * all other fields are delegated to the base class implementation.
 */
uint32_t Facility::modifyFromJSONInternal(json_t *json, GenericClientSession *session)
{
   json_t *group = json_object_get(json, "facility");
   if (group != nullptr)
   {
      if (!json_is_object(group))
         return RCC_INVALID_ARGUMENT;

      json_t *value = json_object_get(group, "settlementLag");
      if (value != nullptr)
      {
         if (!json_is_integer(value) || (json_integer_value(value) < 0))
            return RCC_INVALID_ARGUMENT;
         m_settlementLag = static_cast<int32_t>(json_integer_value(value));
      }

      value = json_object_get(group, "providerId");
      if (value != nullptr)
      {
         if (json_is_string(value))
            m_providerId = json_object_get_string_t(group, "providerId", _T(""));
         else if (json_is_null(value))
            m_providerId = nullptr;
         else
            return RCC_INVALID_ARGUMENT;
      }
   }
   return super::modifyFromJSONInternal(json, session);
}

/**
 * Create NXSL object for this object
 */
NXSL_Value *Facility::createNXSLObject(NXSL_VM *vm)
{
   return vm->createValue(vm->createObject(&g_nxslFacilityClass, new shared_ptr<Facility>(self())));
}
