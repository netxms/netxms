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
** File: power_domain.cpp
**
**/

#include "nxcore.h"

/**
 * Power domain type names (index = PowerDomainType value)
 */
static const char *s_domainTypeNames[] = { "GRID_ENTRY", "GENERATOR", "UPS", "PDU", "BUSWAY", "OTHER" };

/**
 * Get symbolic name of power domain type
 */
const char *PowerDomainTypeName(PowerDomainType type)
{
   return s_domainTypeNames[PowerDomainTypeFromInt(type)];
}

/**
 * Convert symbolic name to power domain type. Returns false if name is not recognized.
 */
bool PowerDomainTypeFromName(const char *name, PowerDomainType *type)
{
   for(int i = 0; i <= POWER_DOMAIN_OTHER; i++)
   {
      if (!stricmp(name, s_domainTypeNames[i]))
      {
         *type = static_cast<PowerDomainType>(i);
         return true;
      }
   }
   return false;
}

/**
 * Read power domain type from JSON value: either symbolic name or integer. Returns false if value is invalid.
 */
static bool PowerDomainTypeFromJson(json_t *value, PowerDomainType *type)
{
   if (json_is_string(value))
      return PowerDomainTypeFromName(json_string_value(value), type);
   if (json_is_integer(value))
   {
      json_int_t n = json_integer_value(value);
      if ((n < POWER_DOMAIN_GRID_ENTRY) || (n > POWER_DOMAIN_OTHER))
         return false;
      *type = static_cast<PowerDomainType>(n);
      return true;
   }
   return false;
}

/**
 * Default constructor
 */
PowerDomain::PowerDomain() : super()
{
   m_domainType = POWER_DOMAIN_OTHER;
   m_feedTag[0] = 0;
   m_ratedPower = 0;
}

/**
 * Constructor from NXCP message (object creation request)
 */
PowerDomain::PowerDomain(const TCHAR *name, const NXCPMessage& request) : super(name)
{
   m_domainType = PowerDomainTypeFromInt(request.getFieldAsInt32(VID_DOMAIN_TYPE));
   request.getFieldAsString(VID_FEED_TAG, m_feedTag, 16);
   m_ratedPower = request.getFieldAsInt32(VID_RATED_POWER);
}

/**
 * Constructor from JSON definition (REST API)
 */
PowerDomain::PowerDomain(const TCHAR *name, json_t *json) : super(name)
{
   m_domainType = POWER_DOMAIN_OTHER;
   json_t *value = json_object_get(json, "domainType");
   if (value != nullptr)
      PowerDomainTypeFromJson(value, &m_domainType);
   _tcslcpy(m_feedTag, json_object_get_string_t(json, "feedTag", _T("")), 16);
   m_ratedPower = json_object_get_int32(json, "ratedPower");
}

/**
 * Load from database
 */
bool PowerDomain::loadFromDatabase(DB_HANDLE hdb, uint32_t id, DB_STATEMENT *preparedStatements)
{
   if (!super::loadFromDatabase(hdb, id, preparedStatements))
      return false;

   DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT domain_type,feed_tag,rated_power FROM power_domains WHERE id=?"));
   if (hStmt == nullptr)
      return false;

   bool success = false;
   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
   DB_RESULT hResult = DBSelectPrepared(hStmt);
   if (hResult != nullptr)
   {
      if (DBGetNumRows(hResult) > 0)
      {
         m_domainType = PowerDomainTypeFromInt(DBGetFieldLong(hResult, 0, 0));
         DBGetField(hResult, 0, 1, m_feedTag, 16);
         m_ratedPower = DBGetFieldLong(hResult, 0, 2);
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
bool PowerDomain::saveToDatabase(DB_HANDLE hdb)
{
   bool success = super::saveToDatabase(hdb);
   if (success && (m_modified & MODIFY_OTHER))
   {
      static const TCHAR *columns[] = { _T("domain_type"), _T("feed_tag"), _T("rated_power"), nullptr };
      DB_STATEMENT hStmt = DBPrepareMerge(hdb, _T("power_domains"), _T("id"), m_id, columns);
      if (hStmt != nullptr)
      {
         lockProperties();
         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, static_cast<int32_t>(m_domainType));
         DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, m_feedTag, DB_BIND_TRANSIENT);
         DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, m_ratedPower);
         DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, m_id);
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
bool PowerDomain::deleteFromDatabase(DB_HANDLE hdb)
{
   bool success = super::deleteFromDatabase(hdb);
   if (success)
      success = executeQueryOnObject(hdb, _T("DELETE FROM power_domains WHERE id=?"));
   return success;
}

/**
 * Fill message with object fields
 */
void PowerDomain::fillMessageLocked(NXCPMessage *msg, uint32_t userId)
{
   super::fillMessageLocked(msg, userId);
   msg->setField(VID_DOMAIN_TYPE, static_cast<int16_t>(m_domainType));
   msg->setField(VID_FEED_TAG, m_feedTag);
   msg->setField(VID_RATED_POWER, m_ratedPower);
}

/**
 * Modify object from message
 */
uint32_t PowerDomain::modifyFromMessageInternal(const NXCPMessage& msg, ClientSession *session)
{
   if (msg.isFieldExist(VID_DOMAIN_TYPE))
      m_domainType = PowerDomainTypeFromInt(msg.getFieldAsInt32(VID_DOMAIN_TYPE));
   if (msg.isFieldExist(VID_FEED_TAG))
      msg.getFieldAsString(VID_FEED_TAG, m_feedTag, 16);
   if (msg.isFieldExist(VID_RATED_POWER))
      m_ratedPower = msg.getFieldAsInt32(VID_RATED_POWER);
   return super::modifyFromMessageInternal(msg, session);
}

/**
 * Build "powerDomain" property group
 */
json_t *PowerDomain::powerDomainConfigToJson()
{
   json_t *group = json_object();
   lockProperties();
   json_object_set_new(group, "domainType", json_string(PowerDomainTypeName(m_domainType)));
   json_object_set_new(group, "feedTag", json_string_t(m_feedTag));
   json_object_set_new(group, "ratedPower", json_integer(m_ratedPower));
   unlockProperties();
   return group;
}

/**
 * Serialize object to JSON
 */
json_t *PowerDomain::toJson(bool includeSensitiveData)
{
   json_t *root = super::toJson(includeSensitiveData);
   json_object_set_new(root, "powerDomain", powerDomainConfigToJson());
   return root;
}

/**
 * Modify object from JSON document (WebAPI path). Handles the "powerDomain" property group;
 * all other fields are delegated to the base class implementation.
 */
uint32_t PowerDomain::modifyFromJSONInternal(json_t *json, GenericClientSession *session)
{
   json_t *group = json_object_get(json, "powerDomain");
   if (group != nullptr)
   {
      if (!json_is_object(group))
         return RCC_INVALID_ARGUMENT;

      json_t *value = json_object_get(group, "domainType");
      if ((value != nullptr) && !PowerDomainTypeFromJson(value, &m_domainType))
         return RCC_INVALID_ARGUMENT;

      value = json_object_get(group, "feedTag");
      if (value != nullptr)
      {
         if (json_is_string(value))
            _tcslcpy(m_feedTag, json_object_get_string_t(group, "feedTag", _T("")), 16);
         else if (json_is_null(value))
            m_feedTag[0] = 0;
         else
            return RCC_INVALID_ARGUMENT;
      }

      value = json_object_get(group, "ratedPower");
      if (value != nullptr)
      {
         if (!json_is_integer(value) || (json_integer_value(value) < 0))
            return RCC_INVALID_ARGUMENT;
         m_ratedPower = static_cast<int32_t>(json_integer_value(value));
      }
   }
   return super::modifyFromJSONInternal(json, session);
}

/**
 * Collect facilities reachable from given object through facility and power domain parents.
 * Facility objects are added to the set directly; power domain parents are followed recursively.
 * Object lists are locked internally, so this must not be called with parent list locked.
 */
static void CollectReachableFacilities(const NetObj& object, HashSet<uint32_t> *facilities, HashSet<uint32_t> *visited)
{
   if (visited->contains(object.getId()))
      return;
   visited->put(object.getId());

   unique_ptr<SharedObjectArray<NetObj>> parents = object.getParents();
   for(int i = 0; i < parents->size(); i++)
   {
      NetObj *parent = parents->get(i);
      if (parent->getObjectClass() == OBJECT_FACILITY)
         facilities->put(parent->getId());
      else if (parent->getObjectClass() == OBJECT_POWERDOMAIN)
         CollectReachableFacilities(*parent, facilities, visited);
   }
}

/**
 * Validate binding to given parent. Power domain may not belong to two different facilities,
 * neither directly nor through a chain of parent power domains.
 */
uint32_t PowerDomain::validateParent(const NetObj& parent) const
{
   if ((parent.getObjectClass() != OBJECT_FACILITY) && (parent.getObjectClass() != OBJECT_POWERDOMAIN))
      return RCC_SUCCESS;

   HashSet<uint32_t> facilities, visited;
   CollectReachableFacilities(*this, &facilities, &visited);
   if (parent.getObjectClass() == OBJECT_FACILITY)
      facilities.put(parent.getId());
   else
      CollectReachableFacilities(parent, &facilities, &visited);

   return (facilities.size() > 1) ? RCC_OBJECT_HIERARCHY_VIOLATION : RCC_SUCCESS;
}

/**
 * Create NXSL object for this object
 */
NXSL_Value *PowerDomain::createNXSLObject(NXSL_VM *vm)
{
   return vm->createValue(vm->createObject(&g_nxslPowerDomainClass, new shared_ptr<PowerDomain>(self())));
}
