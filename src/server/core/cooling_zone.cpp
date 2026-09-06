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
** File: cooling_zone.cpp
**
**/

#include "nxcore.h"

/**
 * Cooling zone type names (index = CoolingZoneType value)
 */
static const char *s_zoneTypeNames[] = { "PLANT", "ZONE", "OTHER" };

/**
 * Get symbolic name of cooling zone type
 */
const char *CoolingZoneTypeName(CoolingZoneType type)
{
   return s_zoneTypeNames[CoolingZoneTypeFromInt(type)];
}

/**
 * Convert symbolic name to cooling zone type. Returns false if name is not recognized.
 */
bool CoolingZoneTypeFromName(const char *name, CoolingZoneType *type)
{
   for(int i = 0; i <= COOLING_ZONE_OTHER; i++)
   {
      if (!stricmp(name, s_zoneTypeNames[i]))
      {
         *type = static_cast<CoolingZoneType>(i);
         return true;
      }
   }
   return false;
}

/**
 * Read cooling zone type from JSON value: either symbolic name or integer. Returns false if value is invalid.
 */
static bool CoolingZoneTypeFromJson(json_t *value, CoolingZoneType *type)
{
   if (json_is_string(value))
      return CoolingZoneTypeFromName(json_string_value(value), type);
   if (json_is_integer(value))
   {
      json_int_t n = json_integer_value(value);
      if ((n < COOLING_ZONE_PLANT) || (n > COOLING_ZONE_OTHER))
         return false;
      *type = static_cast<CoolingZoneType>(n);
      return true;
   }
   return false;
}

/**
 * Default constructor
 */
CoolingZone::CoolingZone() : super()
{
   m_zoneType = COOLING_ZONE_OTHER;
   m_ratedCapacity = 0;
}

/**
 * Constructor from NXCP message (object creation request)
 */
CoolingZone::CoolingZone(const TCHAR *name, const NXCPMessage& request) : super(name)
{
   m_zoneType = CoolingZoneTypeFromInt(request.getFieldAsInt32(VID_ZONE_TYPE));
   m_ratedCapacity = request.getFieldAsInt32(VID_RATED_CAPACITY);
}

/**
 * Constructor from JSON definition (REST API)
 */
CoolingZone::CoolingZone(const TCHAR *name, json_t *json) : super(name)
{
   m_zoneType = COOLING_ZONE_OTHER;
   json_t *value = json_object_get(json, "zoneType");
   if (value != nullptr)
      CoolingZoneTypeFromJson(value, &m_zoneType);
   m_ratedCapacity = json_object_get_int32(json, "ratedCapacity");
}

/**
 * Load from database
 */
bool CoolingZone::loadFromDatabase(DB_HANDLE hdb, uint32_t id, DB_STATEMENT *preparedStatements)
{
   if (!super::loadFromDatabase(hdb, id, preparedStatements))
      return false;

   DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT zone_type,rated_capacity FROM cooling_zones WHERE id=?"));
   if (hStmt == nullptr)
      return false;

   bool success = false;
   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
   DB_RESULT hResult = DBSelectPrepared(hStmt);
   if (hResult != nullptr)
   {
      if (DBGetNumRows(hResult) > 0)
      {
         m_zoneType = CoolingZoneTypeFromInt(DBGetFieldLong(hResult, 0, 0));
         m_ratedCapacity = DBGetFieldLong(hResult, 0, 1);
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
bool CoolingZone::saveToDatabase(DB_HANDLE hdb)
{
   bool success = super::saveToDatabase(hdb);
   if (success && (m_modified & MODIFY_OTHER))
   {
      static const TCHAR *columns[] = { _T("zone_type"), _T("rated_capacity"), nullptr };
      DB_STATEMENT hStmt = DBPrepareMerge(hdb, _T("cooling_zones"), _T("id"), m_id, columns);
      if (hStmt != nullptr)
      {
         lockProperties();
         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, static_cast<int32_t>(m_zoneType));
         DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, m_ratedCapacity);
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
bool CoolingZone::deleteFromDatabase(DB_HANDLE hdb)
{
   bool success = super::deleteFromDatabase(hdb);
   if (success)
      success = executeQueryOnObject(hdb, _T("DELETE FROM cooling_zones WHERE id=?"));
   return success;
}

/**
 * Fill message with object fields
 */
void CoolingZone::fillMessageLocked(NXCPMessage *msg, uint32_t userId)
{
   super::fillMessageLocked(msg, userId);
   msg->setField(VID_ZONE_TYPE, static_cast<int16_t>(m_zoneType));
   msg->setField(VID_RATED_CAPACITY, m_ratedCapacity);
}

/**
 * Modify object from message
 */
uint32_t CoolingZone::modifyFromMessageInternal(const NXCPMessage& msg, ClientSession *session)
{
   if (msg.isFieldExist(VID_ZONE_TYPE))
      m_zoneType = CoolingZoneTypeFromInt(msg.getFieldAsInt32(VID_ZONE_TYPE));
   if (msg.isFieldExist(VID_RATED_CAPACITY))
      m_ratedCapacity = msg.getFieldAsInt32(VID_RATED_CAPACITY);
   return super::modifyFromMessageInternal(msg, session);
}

/**
 * Build "coolingZone" property group
 */
json_t *CoolingZone::coolingZoneConfigToJson()
{
   json_t *group = json_object();
   lockProperties();
   json_object_set_new(group, "zoneType", json_string(CoolingZoneTypeName(m_zoneType)));
   json_object_set_new(group, "ratedCapacity", json_integer(m_ratedCapacity));
   unlockProperties();
   return group;
}

/**
 * Serialize object to JSON
 */
json_t *CoolingZone::toJson(bool includeSensitiveData)
{
   json_t *root = super::toJson(includeSensitiveData);
   json_object_set_new(root, "coolingZone", coolingZoneConfigToJson());
   return root;
}

/**
 * Modify object from JSON document (WebAPI path). Handles the "coolingZone" property group;
 * all other fields are delegated to the base class implementation.
 */
uint32_t CoolingZone::modifyFromJSONInternal(json_t *json, GenericClientSession *session)
{
   json_t *group = json_object_get(json, "coolingZone");
   if (group != nullptr)
   {
      if (!json_is_object(group))
         return RCC_INVALID_ARGUMENT;

      json_t *value = json_object_get(group, "zoneType");
      if ((value != nullptr) && !CoolingZoneTypeFromJson(value, &m_zoneType))
         return RCC_INVALID_ARGUMENT;

      value = json_object_get(group, "ratedCapacity");
      if (value != nullptr)
      {
         if (!json_is_integer(value) || (json_integer_value(value) < 0))
            return RCC_INVALID_ARGUMENT;
         m_ratedCapacity = static_cast<int32_t>(json_integer_value(value));
      }
   }
   return super::modifyFromJSONInternal(json, session);
}

/**
 * Create NXSL object for this object
 */
NXSL_Value *CoolingZone::createNXSLObject(NXSL_VM *vm)
{
   return vm->createValue(vm->createObject(&g_nxslCoolingZoneClass, new shared_ptr<CoolingZone>(self())));
}
