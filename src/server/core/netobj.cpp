/*
** NetXMS - Network Management System
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
** File: netobj.cpp
**
**/

#include "nxcore.h"
#include <netxms-version.h>
#include <asset_management.h>

#define DEBUG_TAG_OBJECT_RELATIONS  _T("obj.relations")
#define DEBUG_TAG_OBJECT_LIFECYCLE  _T("obj.lifecycle")

/**
 * Class names
 */
static const WCHAR *s_classNameW[]=
   {
      L"Generic", L"Subnet", L"Node", L"Interface",
      L"Network", L"Container", L"Zone", L"ServiceRoot",
      L"Template", L"TemplateGroup", L"TemplateRoot",
      L"NetworkService", L"VPNConnector", L"Condition",
      L"Cluster", L"BusinessServiceProto", L"Asset",
      L"AssetGroup", L"AssetRoot", L"NetworkMapRoot",
      L"NetworkMapGroup", L"NetworkMap", L"DashboardRoot",
      L"Dashboard", L"ReportRoot", L"ReportGroup", L"Report",
      L"BusinessServiceRoot", L"BusinessService", L"NodeLink",
      L"ServiceCheck", L"MobileDevice", L"Rack", L"AccessPoint",
      L"AgentPolicyLogParser", L"Chassis", L"DashboardGroup",
      L"Sensor"
   };
static const char *s_classNameA[]=
   {
      "Generic", "Subnet", "Node", "Interface",
      "Network", "Container", "Zone", "ServiceRoot",
      "Template", "TemplateGroup", "TemplateRoot",
      "NetworkService", "VPNConnector", "Condition",
      "Cluster", "BusinessServiceProto", "Asset",
      "AssetGroup", "AssetRoot", "NetworkMapRoot",
      "NetworkMapGroup", "NetworkMap", "DashboardRoot",
      "Dashboard", "ReportRoot", "ReportGroup", "Report",
      "BusinessServiceRoot", "BusinessService", "NodeLink",
      "ServiceCheck", "MobileDevice", "Rack", "AccessPoint",
      "AgentPolicyLogParser", "Chassis", "DashboardGroup",
      "Sensor"
   };

/**
 * Default constructor
 */
NetObj::NetObj() : NObject(), m_mutexProperties(MutexType::FAST), m_dashboards(0, 8), m_urls(0, 8, Ownership::True), m_mutexACL(MutexType::FAST), m_moduleDataLock(MutexType::FAST), m_mutexResponsibleUsers(MutexType::FAST)
{
   m_status = STATUS_UNKNOWN;
   m_savedStatus = STATUS_UNKNOWN;
   m_comments = nullptr;
   m_commentsSource = nullptr;
   m_modified = 0;
   m_isDeleted = false;
   m_isDeleteInitiated = false;
   m_isHidden = false;
   m_isSystem = false;
   m_maintenanceEventId = 0;
   m_maintenanceInitiator = 0;
   m_inheritAccessRights = true;
   m_trustedObjects = nullptr;
   m_pollRequestor = nullptr;
   m_pollRequestId = 0;
   m_statusCalcAlg = SA_CALCULATE_DEFAULT;
   m_statusPropAlg = SA_PROPAGATE_DEFAULT;
   m_fixedStatus = STATUS_WARNING;
   m_statusShift = 0;
   m_statusSingleThreshold = 75;
   m_timestamp = 0;
   for(int i = 0; i < 4; i++)
   {
      m_statusTranslation[i] = i + 1;
      m_statusThresholds[i] = 80 - i * 20;
   }
   m_drilldownObjectId = 0;
   m_moduleData = nullptr;
   m_primaryZoneProxyId = 0;
   m_backupZoneProxyId = 0;
   m_state = 0;
   m_savedState = 0;
   m_stateBeforeMaintenance = 0;
   m_runtimeFlags = 0;
   m_flags = 0;
   m_responsibleUsers = nullptr;
   m_creationTime = 0;
   m_categoryId = 0;
   m_assetId = 0;
   m_asPollable = nullptr;
}

/**
 * Destructor
 */
NetObj::~NetObj()
{
   if (!(g_flags & AF_SHUTDOWN))
      nxlog_debug_tag(DEBUG_TAG_OBJECT_LIFECYCLE, 7, _T("Called destructor for object %s [%u]"), m_name, m_id);
   delete m_trustedObjects;
   delete m_moduleData;
   delete m_responsibleUsers;
}

/**
 * Get class name for this object
 */
const WCHAR *NetObj::getObjectClassNameW() const
{
   return getObjectClassNameW(getObjectClass());
}

/**
 * Get class name for this object
 */
const char *NetObj::getObjectClassNameA() const
{
   return getObjectClassNameA(getObjectClass());
}

/**
 * Get class name for given class ID
 */
const WCHAR *NetObj::getObjectClassNameW(int objectClass)
{
   return ((objectClass >= 0) && (objectClass < static_cast<int>(sizeof(s_classNameW) / sizeof(const WCHAR*)))) ? s_classNameW[objectClass] : L"Custom";
}

/**
 * Get class name for given class ID
 */
const char *NetObj::getObjectClassNameA(int objectClass)
{
   return ((objectClass >= 0) && (objectClass < static_cast<int>(sizeof(s_classNameA) / sizeof(const char*)))) ? s_classNameA[objectClass] : "Custom";
}

/**
 * Get object class ID by name
 */
int NetObj::getObjectClassByNameW(const WCHAR *name)
{
   for(int i = 0; i < static_cast<int>(sizeof(s_classNameW) / sizeof(const WCHAR*)); i++)
      if (!wcsicmp(name, s_classNameW[i]))
         return i;
   return OBJECT_GENERIC;
}

/**
 * Get object class ID by name
 */
int NetObj::getObjectClassByNameA(const char *name)
{
   for(int i = 0; i < static_cast<int>(sizeof(s_classNameA) / sizeof(const char*)); i++)
      if (!stricmp(name, s_classNameA[i]))
         return i;
   return OBJECT_GENERIC;
}

/**
 * Create object from database data
 */
bool NetObj::loadFromDatabase(DB_HANDLE hdb, UINT32 dwId)
{
   return false;     // Abstract objects cannot be loaded from database
}

/**
 * Link related objects after loading from database
 */
void NetObj::linkObjects()
{
}

/**
 * Callback for saving custom attribute in database
 */
static EnumerationCallbackResult SaveAttributeCallback(const TCHAR *key, const CustomAttribute *value, DB_STATEMENT hStmt)
{
   if ((value->sourceObject != 0) && !(value->flags & CAF_REDEFINED)) // do not save inherited attributes
      return _CONTINUE;

   DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, key, DB_BIND_STATIC);
   DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, value->value, DB_BIND_STATIC);
   DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, value->flags);
   return DBExecute(hStmt) ? _CONTINUE : _STOP;
}

/**
 * Save object to database
 */
bool NetObj::saveToDatabase(DB_HANDLE hdb)
{
   // Save custom attributes
   bool success = true;
   if (m_modified & MODIFY_CUSTOM_ATTRIBUTES)
   {
      success = executeQueryOnObject(hdb, _T("DELETE FROM object_custom_attributes WHERE object_id=?"));
      if (success)
      {
         DB_STATEMENT hStmt = DBPrepare(hdb, _T("INSERT INTO object_custom_attributes (object_id,attr_name,attr_value,flags) VALUES (?,?,?,?)"), true);
         if (hStmt != nullptr)
         {
            DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
            success = (forEachCustomAttribute(SaveAttributeCallback, hStmt) == _CONTINUE);
            DBFreeStatement(hStmt);
         }
         else
         {
            success = false;
         }
      }
   }

   if (success)
      success = saveACLToDB(hdb);

   if (success && (m_modified & MODIFY_POLL_TIMES) && isPollable())
   {
      success = getAsPollable()->saveToDatabase(hdb);
   }

   if (!success)
      return false;

   if (!(m_modified & MODIFY_COMMON_PROPERTIES))
      return saveModuleData(hdb);

   static const TCHAR *columns[] = {
      _T("name"), _T("status"), _T("is_deleted"), _T("inherit_access_rights"), _T("last_modified"), _T("status_calc_alg"),
      _T("status_prop_alg"), _T("status_fixed_val"), _T("status_shift"), _T("status_translation"), _T("status_single_threshold"),
      _T("status_thresholds"), _T("comments"), _T("is_system"), _T("location_type"), _T("latitude"), _T("longitude"),
      _T("location_accuracy"), _T("location_timestamp"), _T("guid"), _T("map_image"), _T("drilldown_object_id"), _T("country"),
      _T("region"), _T("city"), _T("district"), _T("street_address"), _T("postcode"), _T("maint_event_id"),
      _T("state_before_maint"), _T("state"), _T("flags"), _T("creation_time"), _T("maint_initiator"), _T("alias"),
      _T("name_on_map"), _T("category"), _T("comments_source"), _T("asset_id"), nullptr
   };

   DB_STATEMENT hStmt = DBPrepareMerge(hdb, _T("object_properties"), _T("object_id"), m_id, columns);
   if (hStmt == nullptr)
      return false;

   lockProperties();

   TCHAR szTranslation[16], szThresholds[16], lat[32], lon[32];
   for(int i = 0, j = 0; i < 4; i++, j += 2)
   {
      _sntprintf(&szTranslation[j], 16 - j, _T("%02X"), (BYTE)m_statusTranslation[i]);
      _sntprintf(&szThresholds[j], 16 - j, _T("%02X"), (BYTE)m_statusThresholds[i]);
   }
   _sntprintf(lat, 32, _T("%f"), m_geoLocation.getLatitude());
   _sntprintf(lon, 32, _T("%f"), m_geoLocation.getLongitude());

   DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, m_name, DB_BIND_STATIC);
   DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, (LONG)m_status);
   DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, (LONG)(m_isDeleted ? 1 : 0));
   DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, (LONG)(m_inheritAccessRights ? 1 : 0));
   DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, (LONG)m_timestamp);
   DBBind(hStmt, 6, DB_SQLTYPE_INTEGER, (LONG)m_statusCalcAlg);
   DBBind(hStmt, 7, DB_SQLTYPE_INTEGER, (LONG)m_statusPropAlg);
   DBBind(hStmt, 8, DB_SQLTYPE_INTEGER, (LONG)m_fixedStatus);
   DBBind(hStmt, 9, DB_SQLTYPE_INTEGER, (LONG)m_statusShift);
   DBBind(hStmt, 10, DB_SQLTYPE_VARCHAR, szTranslation, DB_BIND_STATIC);
   DBBind(hStmt, 11, DB_SQLTYPE_INTEGER, (LONG)m_statusSingleThreshold);
   DBBind(hStmt, 12, DB_SQLTYPE_VARCHAR, szThresholds, DB_BIND_STATIC);
   DBBind(hStmt, 13, DB_SQLTYPE_VARCHAR, m_comments, DB_BIND_STATIC);
   DBBind(hStmt, 14, DB_SQLTYPE_INTEGER, (LONG)(m_isSystem ? 1 : 0));
   DBBind(hStmt, 15, DB_SQLTYPE_INTEGER, (LONG)m_geoLocation.getType());
   DBBind(hStmt, 16, DB_SQLTYPE_VARCHAR, lat, DB_BIND_STATIC);
   DBBind(hStmt, 17, DB_SQLTYPE_VARCHAR, lon, DB_BIND_STATIC);
   DBBind(hStmt, 18, DB_SQLTYPE_INTEGER, (LONG)m_geoLocation.getAccuracy());
   DBBind(hStmt, 19, DB_SQLTYPE_INTEGER, (UINT32)m_geoLocation.getTimestamp());
   DBBind(hStmt, 20, DB_SQLTYPE_VARCHAR, m_guid);
   DBBind(hStmt, 21, DB_SQLTYPE_VARCHAR, m_mapImage);
   DBBind(hStmt, 22, DB_SQLTYPE_INTEGER, m_drilldownObjectId);
   DBBind(hStmt, 23, DB_SQLTYPE_VARCHAR, m_postalAddress.getCountry(), DB_BIND_STATIC);
   DBBind(hStmt, 24, DB_SQLTYPE_VARCHAR, m_postalAddress.getRegion(), DB_BIND_STATIC);
   DBBind(hStmt, 25, DB_SQLTYPE_VARCHAR, m_postalAddress.getCity(), DB_BIND_STATIC);
   DBBind(hStmt, 26, DB_SQLTYPE_VARCHAR, m_postalAddress.getDistrict(), DB_BIND_STATIC);
   DBBind(hStmt, 27, DB_SQLTYPE_VARCHAR, m_postalAddress.getStreetAddress(), DB_BIND_STATIC);
   DBBind(hStmt, 28, DB_SQLTYPE_VARCHAR, m_postalAddress.getPostCode(), DB_BIND_STATIC);
   DBBind(hStmt, 29, DB_SQLTYPE_BIGINT, m_maintenanceEventId);
   DBBind(hStmt, 30, DB_SQLTYPE_INTEGER, m_stateBeforeMaintenance);
   DBBind(hStmt, 31, DB_SQLTYPE_INTEGER, m_state);
   DBBind(hStmt, 32, DB_SQLTYPE_INTEGER, m_flags);
   DBBind(hStmt, 33, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(m_creationTime));
   DBBind(hStmt, 34, DB_SQLTYPE_INTEGER, m_maintenanceInitiator);
   DBBind(hStmt, 35, DB_SQLTYPE_VARCHAR, m_alias, DB_BIND_STATIC);
   DBBind(hStmt, 36, DB_SQLTYPE_VARCHAR, m_nameOnMap, DB_BIND_STATIC);
   DBBind(hStmt, 37, DB_SQLTYPE_INTEGER, m_categoryId);
   DBBind(hStmt, 38, DB_SQLTYPE_TEXT, m_commentsSource, DB_BIND_STATIC);
   DBBind(hStmt, 39, DB_SQLTYPE_INTEGER, m_assetId);
   DBBind(hStmt, 40, DB_SQLTYPE_INTEGER, m_id);

   success = DBExecute(hStmt);
   DBFreeStatement(hStmt);
   if (success)
   {
      m_savedStatus = m_status;
      m_savedState = m_state;
   }

   // Save dashboard associations
   if (success && (m_modified & MODIFY_DASHBOARD_LIST))
   {
      success = ExecuteQueryOnObject(hdb, m_id, _T("DELETE FROM dashboard_associations WHERE object_id=?"));
      if (success && !m_dashboards.isEmpty())
      {
         hStmt = DBPrepare(hdb, _T("INSERT INTO dashboard_associations (object_id,dashboard_id) VALUES (?,?)"), true);
         if (hStmt != nullptr)
         {
            DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
            for(int i = 0; (i < m_dashboards.size()) && success; i++)
            {
               DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, m_dashboards.get(i));
               success = DBExecute(hStmt);
            }
            DBFreeStatement(hStmt);
         }
         else
         {
            success = false;
         }
      }
   }

   // Save URL associations
   if (success)
   {
      success = ExecuteQueryOnObject(hdb, m_id, _T("DELETE FROM object_urls WHERE object_id=?"));
      if (success && !m_urls.isEmpty())
      {
         hStmt = DBPrepare(hdb, _T("INSERT INTO object_urls (object_id,url_id,url,description) VALUES (?,?,?,?)"), true);
         if (hStmt != nullptr)
         {
            DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
            for(int i = 0; (i < m_urls.size()) && success; i++)
            {
               const ObjectUrl *url = m_urls.get(i);
               DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, url->getId());
               DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, url->getUrl(), DB_BIND_STATIC);
               DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, url->getDescription(), DB_BIND_STATIC);
               success = DBExecute(hStmt);
            }
            DBFreeStatement(hStmt);
         }
         else
         {
            success = false;
         }
      }
   }

   unlockProperties();

   if (success)
      success = saveTrustedObjects(hdb);

   // Save responsible users
   if (success)
   {
      success = executeQueryOnObject(hdb, _T("DELETE FROM responsible_users WHERE object_id=?"));
      lockResponsibleUsersList();
      if (success && (m_responsibleUsers != nullptr) && !m_responsibleUsers->isEmpty())
      {
         hStmt = DBPrepare(hdb, _T("INSERT INTO responsible_users (object_id,user_id,tag) VALUES (?,?,?)"), m_responsibleUsers->size() > 1);
         if (hStmt != nullptr)
         {
            DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
            for(int i = 0; (i < m_responsibleUsers->size()) && success; i++)
            {
               ResponsibleUser *r = m_responsibleUsers->get(i);
               DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, r->userId);
               DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, r->tag, DB_BIND_STATIC);
               success = DBExecute(hStmt);
            }
            DBFreeStatement(hStmt);
         }
         else
         {
            success = false;
         }
      }
      unlockResponsibleUsersList();
   }

   return success ? saveModuleData(hdb) : false;
}

/**
 * Callback for saving module data in database
 */
static EnumerationCallbackResult SaveModuleDataCallback(const TCHAR *key, const ModuleData *value, std::pair<uint32_t, DB_HANDLE> *context)
{
   return const_cast<ModuleData*>(value)->saveToDatabase(context->second, context->first) ? _CONTINUE : _STOP;
}

/**
 * Save module data to database
 */
bool NetObj::saveModuleData(DB_HANDLE hdb)
{
   if (!(m_modified & MODIFY_MODULE_DATA))
      return true;

   bool success = true;
   m_moduleDataLock.lock();
   if (m_moduleData != nullptr)
   {
      std::pair<uint32_t, DB_HANDLE> context(m_id, hdb);
      success = (m_moduleData->forEach(SaveModuleDataCallback, &context) == _CONTINUE);
   }
   m_moduleDataLock.unlock();
   return success;
}

/**
 * Callback for deleting module data from database
 */
static EnumerationCallbackResult SaveModuleRuntimeDataCallback(const TCHAR *key, const ModuleData *value, std::pair<uint32_t, DB_HANDLE> *context)
{
   return const_cast<ModuleData*>(value)->saveRuntimeData(context->second, context->first) ? _CONTINUE : _STOP;
}

/**
 * Save runtime data to database. Called only on server shutdown to save
 * less important but frequently changing runtime data when it is not feasible
 * to mark object as modified on each change of such data.
 */
bool NetObj::saveRuntimeData(DB_HANDLE hdb)
{
   bool success;
   if ((m_status != m_savedStatus) || (m_state != m_savedState))
   {
      DB_STATEMENT hStmt = DBPrepare(hdb, _T("UPDATE object_properties SET status=?,state=? WHERE object_id=?"));
      if (hStmt == nullptr)
         return false;

      DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_status);
      DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, m_state);
      DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, m_id);
      success = DBExecute(hStmt);
      DBFreeStatement(hStmt);

      if (success)
      {
         m_savedStatus = m_status;
         m_savedState = m_state;
      }
   }
   else
   {
      success = true;
   }

   // Save module runtime data
   m_moduleDataLock.lock();
   if (success && (m_moduleData != nullptr))
   {
      std::pair<uint32_t, DB_HANDLE> context(m_id, hdb);
      success = (m_moduleData->forEach(SaveModuleRuntimeDataCallback, &context) == _CONTINUE);
   }
   m_moduleDataLock.unlock();

   return success;
}

/**
 * Callback for deleting module data from database
 */
static EnumerationCallbackResult DeleteModuleDataCallback(const TCHAR *key, const ModuleData *value, std::pair<uint32_t, DB_HANDLE> *context)
{
   return const_cast<ModuleData*>(value)->deleteFromDatabase(context->second, context->first) ? _CONTINUE : _STOP;
}

/**
 * Delete object from database
 */
bool NetObj::deleteFromDatabase(DB_HANDLE hdb)
{
   // Delete ACL
   bool success = executeQueryOnObject(hdb, _T("DELETE FROM acl WHERE object_id=?"));
   if (success)
      success = executeQueryOnObject(hdb, _T("DELETE FROM object_properties WHERE object_id=?"));
   if (success)
      success = executeQueryOnObject(hdb, _T("DELETE FROM object_custom_attributes WHERE object_id=?"));
   if (success)
      success = executeQueryOnObject(hdb, _T("DELETE FROM object_urls WHERE object_id=?"));
   if (success)
      success = executeQueryOnObject(hdb, _T("DELETE FROM trusted_objects WHERE object_id=?"));

   // Delete events
   if (success && (g_dbSyntax != DB_SYNTAX_TSDB) && ConfigReadBoolean(_T("Events.DeleteEventsOfDeletedObject"), true))
   {
      success = executeQueryOnObject(hdb, _T("DELETE FROM event_log WHERE event_source=?"));
   }

   // Delete alarms
   if (success && ConfigReadBoolean(_T("Alarms.DeleteAlarmsOfDeletedObject"), true))
      success = DeleteObjectAlarms(m_id, hdb);

   if (success && isGeoLocationHistoryTableExists(hdb))
   {
      TCHAR query[256];
      _sntprintf(query, 256, _T("DROP TABLE gps_history_%u"), m_id);
      success = DBQuery(hdb, query);
   }

   // Delete module data
   if (success && (m_moduleData != nullptr))
   {
      std::pair<uint32_t, DB_HANDLE> context(m_id, hdb);
      success = (m_moduleData->forEach(DeleteModuleDataCallback, &context) == _CONTINUE);
   }

   if (success)
      success = executeQueryOnObject(hdb, _T("DELETE FROM responsible_users WHERE object_id=?"));

   if (success && isPollable())
      success = executeQueryOnObject(hdb, _T("DELETE FROM pollable_objects WHERE id=?"));

   if (success && (g_dbSyntax != DB_SYNTAX_TSDB))
      success = executeQueryOnObject(hdb, _T("DELETE FROM maintenance_journal WHERE object_id=?"));

   return success;
}

/**
 * Load common object properties from database
 */
bool NetObj::loadCommonProperties(DB_HANDLE hdb)
{
   bool success = false;

   // Load access options
   DB_STATEMENT hStmt = DBPrepare(hdb,
                                  _T("SELECT name,status,is_deleted,inherit_access_rights,last_modified,status_calc_alg,")
                                  _T("status_prop_alg,status_fixed_val,status_shift,status_translation,status_single_threshold,")
                                  _T("status_thresholds,comments,is_system,location_type,latitude,longitude,location_accuracy,")
                                  _T("location_timestamp,guid,map_image,drilldown_object_id,country,region,city,district,street_address,")
                                  _T("postcode,maint_event_id,state_before_maint,maint_initiator,state,flags,creation_time,alias,")
                                  _T("name_on_map,category,comments_source,asset_id FROM object_properties WHERE object_id=?"));
   if (hStmt != nullptr)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
      DB_RESULT hResult = DBSelectPrepared(hStmt);
      if (hResult != nullptr)
      {
			if (DBGetNumRows(hResult) > 0)
			{
				DBGetField(hResult, 0, 0, m_name, MAX_OBJECT_NAME);
				m_status = m_savedStatus = DBGetFieldLong(hResult, 0, 1);
				m_isDeleted = DBGetFieldLong(hResult, 0, 2) ? true : false;
				m_inheritAccessRights = DBGetFieldLong(hResult, 0, 3) ? true : false;
				m_timestamp = (time_t)DBGetFieldULong(hResult, 0, 4);
				m_statusCalcAlg = DBGetFieldLong(hResult, 0, 5);
				m_statusPropAlg = DBGetFieldLong(hResult, 0, 6);
				m_fixedStatus = DBGetFieldLong(hResult, 0, 7);
				m_statusShift = DBGetFieldLong(hResult, 0, 8);
				DBGetFieldByteArray(hResult, 0, 9, m_statusTranslation, 4, STATUS_WARNING);
				m_statusSingleThreshold = DBGetFieldLong(hResult, 0, 10);
            DBGetFieldByteArray(hResult, 0, 11, m_statusThresholds, 4, 50);
            m_comments = DBGetFieldAsSharedString(hResult, 0, 12);
            m_isSystem = DBGetFieldLong(hResult, 0, 13) ? true : false;

            int locType = DBGetFieldLong(hResult, 0, 14);
            if (locType != GL_UNSET)
				{
					TCHAR lat[32], lon[32];

					DBGetField(hResult, 0, 15, lat, 32);
					DBGetField(hResult, 0, 16, lon, 32);
					m_geoLocation = GeoLocation(locType, lat, lon, DBGetFieldLong(hResult, 0, 17), DBGetFieldULong(hResult, 0, 18));
				}
				else
				{
					m_geoLocation = GeoLocation();
				}

				m_guid = DBGetFieldGUID(hResult, 0, 19);
				m_mapImage = DBGetFieldGUID(hResult, 0, 20);
				m_drilldownObjectId = DBGetFieldULong(hResult, 0, 21);

            TCHAR buffer[256];
            m_postalAddress.setCountry(DBGetField(hResult, 0, 22, buffer, 64));
            m_postalAddress.setRegion(DBGetField(hResult, 0, 23, buffer, 64));
            m_postalAddress.setCity(DBGetField(hResult, 0, 24, buffer, 64));
            m_postalAddress.setDistrict(DBGetField(hResult, 0, 25, buffer, 64));
            m_postalAddress.setStreetAddress(DBGetField(hResult, 0, 26, buffer, 256));
            m_postalAddress.setPostCode(DBGetField(hResult, 0, 27, buffer, 32));

            m_maintenanceEventId = DBGetFieldUInt64(hResult, 0, 28);
            m_stateBeforeMaintenance = DBGetFieldULong(hResult, 0, 29);
            m_maintenanceInitiator = DBGetFieldULong(hResult, 0, 30);

            m_state = m_savedState = DBGetFieldULong(hResult, 0, 31);
            m_runtimeFlags = 0;
            m_flags = DBGetFieldULong(hResult, 0, 32);
            m_creationTime = static_cast<time_t>(DBGetFieldULong(hResult, 0, 33));
            m_alias = DBGetFieldAsSharedString(hResult, 0, 34);
            m_nameOnMap = DBGetFieldAsSharedString(hResult, 0, 35);
            m_categoryId = DBGetFieldULong(hResult, 0, 36);
            m_commentsSource = DBGetFieldAsSharedString(hResult, 0, 37);
            m_assetId = DBGetFieldULong(hResult, 0, 38);
            success = true;
         }
         DBFreeResult(hResult);
      }
      DBFreeStatement(hStmt);
   }

   // Load custom attributes
   if (success)
   {
      hStmt = DBPrepare(hdb, _T("SELECT attr_name,attr_value,flags FROM object_custom_attributes WHERE object_id=?"));
		if (hStmt != nullptr)
		{
			DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
			DB_RESULT hResult = DBSelectPrepared(hStmt);
			if (hResult != nullptr)
			{
			   setCustomAttributesFromDatabase(hResult);
				DBFreeResult(hResult);
			}
			else
			{
				success = false;
			}
			DBFreeStatement(hStmt);
		}
		else
		{
			success = false;
		}
   }

   // Load associated dashboards
   if (success)
   {
      hStmt = DBPrepare(hdb, _T("SELECT dashboard_id FROM dashboard_associations WHERE object_id=?"));
      if (hStmt != nullptr)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
         DB_RESULT hResult = DBSelectPrepared(hStmt);
         if (hResult != nullptr)
         {
            int count = DBGetNumRows(hResult);
            for(int i = 0; i < count; i++)
            {
               m_dashboards.add(DBGetFieldULong(hResult, i, 0));
            }
            DBFreeResult(hResult);
         }
         else
         {
            success = false;
         }
         DBFreeStatement(hStmt);
      }
      else
      {
         success = false;
      }
   }

   // Load associated URLs
   if (success)
   {
      hStmt = DBPrepare(hdb, _T("SELECT url_id,url,description FROM object_urls WHERE object_id=?"));
      if (hStmt != nullptr)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
         DB_RESULT hResult = DBSelectPrepared(hStmt);
         if (hResult != nullptr)
         {
            int count = DBGetNumRows(hResult);
            for(int i = 0; i < count; i++)
            {
               m_urls.add(new ObjectUrl(hResult, i));
            }
            DBFreeResult(hResult);
         }
         else
         {
            success = false;
         }
         DBFreeStatement(hStmt);
      }
      else
      {
         success = false;
      }
   }

	if (success)
		success = loadTrustedObjects(hdb);

	if (success)
	{
      success = false;
	   hStmt = DBPrepare(hdb, _T("SELECT user_id,tag FROM responsible_users WHERE object_id=?"));
	   if (hStmt != nullptr)
	   {
	      DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
	      DB_RESULT hResult = DBSelectPrepared(hStmt);
	      if (hResult != nullptr)
	      {
	         success = true;
	         int numRows = DBGetNumRows(hResult);
	         if (numRows > 0)
	         {
	            m_responsibleUsers = new StructArray<ResponsibleUser>(numRows, 16);
               for(int i = 0; i < numRows; i++)
               {
                  ResponsibleUser *r = m_responsibleUsers->addPlaceholder();
                  r->userId = DBGetFieldULong(hResult, i, 0);
                  DBGetField(hResult, i, 1, r->tag, MAX_RESPONSIBLE_USER_TAG_LEN);
               }
	         }
	         DBFreeResult(hResult);
	      }
	      DBFreeStatement(hStmt);
	   }
	}

	if (!success)
		nxlog_debug_tag(DEBUG_TAG_OBJECT_LIFECYCLE, 4, _T("NetObj::loadCommonProperties() failed for object %s [%ld] class=%d"), m_name, (long)m_id, getObjectClass());

   return success;
}

/**
 * Method called on custom attribute change
 */
void NetObj::onCustomAttributeChange()
{
   markAsModified(MODIFY_CUSTOM_ATTRIBUTES);
}

/**
 * Add reference to the new child object
 */
void NetObj::addChild(const shared_ptr<NetObj>& object)
{
   super::addChild(object);
	markAsModified(MODIFY_RELATIONS);
	nxlog_debug_tag(DEBUG_TAG_OBJECT_RELATIONS, 7, _T("NetObj::addChild: this=%s [%d]; object=%s [%d]"), m_name, m_id, object->m_name, object->m_id);
}

/**
 * Add reference to parent object
 */
void NetObj::addParent(const shared_ptr<NetObj>& object)
{
   super::addParent(object);
	markAsModified(MODIFY_RELATIONS);
	nxlog_debug_tag(DEBUG_TAG_OBJECT_RELATIONS, 7, _T("NetObj::addParent: this=%s [%d]; object=%s [%d]"), m_name, m_id, object->m_name, object->m_id);
}

/**
 * Delete reference to child object
 */
void NetObj::deleteChild(const NetObj& object)
{
   nxlog_debug_tag(DEBUG_TAG_OBJECT_RELATIONS, 7, _T("NetObj::deleteChild: this=%s [%u]; object=%s [%u]"), m_name, m_id, object.getName(), object.getId());
   super::deleteChild(object.getId());
	markAsModified(MODIFY_RELATIONS);
}

/**
 * Delete reference to parent object
 */
void NetObj::deleteParent(const NetObj& object)
{
   nxlog_debug_tag(DEBUG_TAG_OBJECT_RELATIONS, 7, _T("NetObj::deleteParent: this=%s [%u]; object=%s [%u]"), m_name, m_id, object.getName(), object.getId());
   super::deleteParent(object.getId());
	markAsModified(MODIFY_RELATIONS);
}

/**
 * Walker callback to call OnObjectDelete for each active object
 */
void NetObj::onObjectDeleteCallback(NetObj *object, NetObj *context)
{
	if ((object->getId() != context->getId()) && !object->isDeleted())
		object->onObjectDelete(*context);
}

/**
 * Prepare object for deletion - remove all references, etc.
 *
 * @param initiator pointer to parent object which causes recursive deletion or NULL
 */
void NetObj::deleteObject(NetObj *initiator)
{
   nxlog_debug_tag(DEBUG_TAG_OBJECT_LIFECYCLE, 4, _T("Deleting object %d [%s]"), m_id, m_name);

	// Prevent object change propagation until it's marked as deleted
	// (to prevent the object's incorrect appearance in GUI)
	lockProperties();
	if (m_isDeleteInitiated)
	{
	   nxlog_debug_tag(DEBUG_TAG_OBJECT_LIFECYCLE, 4, _T("Attempt to call NetObj::deleteObject() multiple times for object %d [%s]"), m_id, m_name);
	   unlockProperties();
	   return;
	}
	m_isDeleteInitiated = true;
   m_isHidden = true;
	unlockProperties();

	// Notify modules about object deletion
   CALL_ALL_MODULES(pfPreObjectDelete, (this));

   prepareForDeletion();

   nxlog_debug_tag(DEBUG_TAG_OBJECT_LIFECYCLE, 5, _T("NetObj::deleteObject(): deleting object %d from indexes"), m_id);
   NetObjDeleteFromIndexes(*this);

   // Delete references to this object from child objects
   nxlog_debug_tag(DEBUG_TAG_OBJECT_RELATIONS, 5, _T("NetObj::deleteObject(): clearing child list for object %d"), m_id);
   SharedObjectArray<NetObj> *deleteList = nullptr;
   SharedObjectArray<NetObj> *detachList = nullptr;
   writeLockChildList();
   for(int i = 0; i < getChildList().size(); i++)
   {
      shared_ptr<NetObj> o = getChildList().getShared(i);
      if (o->getParentCount() == 1)
      {
         // last parent, delete object
         if (deleteList == nullptr)
            deleteList = new SharedObjectArray<NetObj>();
			deleteList->add(o);
      }
      else
      {
         if (detachList == nullptr)
            detachList = new SharedObjectArray<NetObj>();
         detachList->add(o);
      }
   }
   clearChildList();
   unlockChildList();

   if (detachList != nullptr)
   {
      for(int i = 0; i < detachList->size(); i++)
      {
         NetObj *obj = detachList->get(i);
         nxlog_debug_tag(DEBUG_TAG_OBJECT_RELATIONS, 5, _T("NetObj::deleteObject(): calling deleteParent() on %s [%d]"), obj->getName(), obj->getId());
         obj->deleteParent(*this);
      }
      delete detachList;
   }

   // Remove references to this object from parent objects
   nxlog_debug_tag(DEBUG_TAG_OBJECT_RELATIONS, 5, _T("NetObj::deleteObject(): clearing parent list for object %d"), m_id);
   SharedObjectArray<NetObj> *recalcList = nullptr;
   writeLockParentList();
   for(int i = 0; i < getParentList().size(); i++)
   {
      // If parent is deletion initiator then this object already
      // removed from parent's child list
      shared_ptr<NetObj> o = getParentList().getShared(i);
      if (o.get() != initiator)
      {
         o->deleteChild(*this);
         if ((o->getObjectClass() == OBJECT_SUBNET) && (g_flags & AF_DELETE_EMPTY_SUBNETS) && (o->getChildCount() == 0))
         {
            if (deleteList == nullptr)
               deleteList = new SharedObjectArray<NetObj>();
            deleteList->add(o);
         }
         else
         {
            if (recalcList == nullptr)
               recalcList = new SharedObjectArray<NetObj>();
            recalcList->add(o);
         }
      }
   }
   clearParentList();
   unlockParentList();

   // Delete orphaned child objects and empty subnets
   if (deleteList != nullptr)
   {
      for(int i = 0; i < deleteList->size(); i++)
      {
         NetObj *obj = deleteList->get(i);
         nxlog_debug_tag(DEBUG_TAG_OBJECT_RELATIONS, 5, _T("NetObj::deleteObject(): calling deleteObject() on %s [%d]"), obj->getName(), obj->getId());
         obj->deleteObject(this);
      }
      delete deleteList;
   }

   // Recalculate statuses of parent objects
   if (recalcList != nullptr)
   {
      for(int i = 0; i < recalcList->size(); i++)
      {
         recalcList->get(i)->calculateCompoundStatus();
      }
      delete recalcList;
   }

   lockProperties();
   m_isHidden = false;
   m_isDeleted = true;
   setModified(MODIFY_ALL);
   unlockProperties();

   // Notify all other objects about object deletion
   nxlog_debug_tag(DEBUG_TAG_OBJECT_LIFECYCLE, 5, _T("NetObj::deleteObject(%s [%d]): calling onObjectDelete()"), m_name, m_id);
	g_idxObjectById.forEach(onObjectDeleteCallback, this);

	nxlog_debug_tag(DEBUG_TAG_OBJECT_LIFECYCLE, 4, _T("Object %d deleted successfully"), m_id);
}

/**
 * Default handler for object deletion notification
 */
void NetObj::onObjectDelete(const NetObj& object)
{
   lockProperties();
   if (m_trustedObjects != nullptr)
   {
      int index = m_trustedObjects->indexOf(object.getId());
      if (index != -1)
      {
         nxlog_debug_tag(DEBUG_TAG_OBJECT_RELATIONS, 5, _T("NetObj::onObjectDelete(%s [%u]): deleted object %s [%u] was listed as trusted object"), m_name, m_id, object.getName(), object.getId());
         m_trustedObjects->remove(index);
         if (m_trustedObjects->isEmpty())
         {
            delete_and_null(m_trustedObjects);
         }
         setModified(MODIFY_COMMON_PROPERTIES);
      }
   }
   unlockProperties();
}

/**
 * Destroy partially loaded object
 */
void NetObj::destroy()
{
   // Delete references to this object from child objects
   for(int i = 0; i < getChildList().size(); i++)
   {
      NetObj *o = getChildList().get(i);
      o->deleteParent(*this);
      if (o->getParentCount() == 0)
      {
         // last parent, delete object
         o->destroy();
      }
   }

   // Remove references to this object from parent objects
   for(int i = 0; i < getParentList().size(); i++)
   {
      getParentList().get(i)->deleteChild(*this);
   }
}

/**
 * Get most critical additional status from any object specific function or component. Default implementation always return STATUS_UNKNOWN.
 */
int NetObj::getAdditionalMostCriticalStatus()
{
   return STATUS_UNKNOWN;
}

/**
 * Calculate status for compound object based on children status
 */
void NetObj::calculateCompoundStatus(bool forcedRecalc)
{
   if (m_status == STATUS_UNMANAGED)
      return;

   int mostCriticalAlarm = GetMostCriticalStatusForObject(m_id);
   int mostCriticalStatus, i, count, iStatusAlg;
   int nSingleThreshold, *pnThresholds;
   int nRating[5], iChildStatus, nThresholds[4];

   lockProperties();
   if (m_statusCalcAlg == SA_CALCULATE_DEFAULT)
   {
      iStatusAlg = GetDefaultStatusCalculation(&nSingleThreshold, &pnThresholds);
   }
   else
   {
      iStatusAlg = m_statusCalcAlg;
      nSingleThreshold = m_statusSingleThreshold;
      pnThresholds = m_statusThresholds;
   }
   if (iStatusAlg == SA_CALCULATE_SINGLE_THRESHOLD)
   {
      for(i = 0; i < 4; i++)
         nThresholds[i] = nSingleThreshold;
      pnThresholds = nThresholds;
   }
   unlockProperties();

   int newStatus;
   switch(iStatusAlg)
   {
      case SA_CALCULATE_MOST_CRITICAL:
         readLockChildList();
         for(i = 0, count = 0, mostCriticalStatus = -1; i < getChildList().size(); i++)
         {
            iChildStatus = getChildList().get(i)->getPropagatedStatus();
            if ((iChildStatus < STATUS_UNKNOWN) &&
                (iChildStatus > mostCriticalStatus))
            {
               mostCriticalStatus = iChildStatus;
               count++;
            }
         }
         newStatus = (count > 0) ? mostCriticalStatus : STATUS_UNKNOWN;
         unlockChildList();
         break;
      case SA_CALCULATE_SINGLE_THRESHOLD:
      case SA_CALCULATE_MULTIPLE_THRESHOLDS:
         // Step 1: calculate severity raitings
         memset(nRating, 0, sizeof(int) * 5);
         readLockChildList();
         for(i = 0, count = 0; i < getChildList().size(); i++)
         {
            iChildStatus = getChildList().get(i)->getPropagatedStatus();
            if (iChildStatus < STATUS_UNKNOWN)
            {
               while(iChildStatus >= 0)
                  nRating[iChildStatus--]++;
               count++;
            }
         }
         unlockChildList();

         // Step 2: check what severity rating is above threshold
         if (count > 0)
         {
            for(i = 4; i > 0; i--)
               if (nRating[i] * 100 / count >= pnThresholds[i - 1])
                  break;
            newStatus = i;
         }
         else
         {
            newStatus = STATUS_UNKNOWN;
         }
         break;
      default:
         newStatus = STATUS_UNKNOWN;
         break;
   }

   // If alarms exist for object, apply alarm severity to object's status
   if (mostCriticalAlarm != STATUS_UNKNOWN)
   {
      if (newStatus == STATUS_UNKNOWN)
      {
         newStatus = mostCriticalAlarm;
      }
      else
      {
         newStatus = std::max(newStatus, mostCriticalAlarm);
      }
   }

   // Apply any additional most critical status
   int mostCriticalAdditional = getAdditionalMostCriticalStatus();
   if (mostCriticalAdditional != STATUS_UNKNOWN)
   {
      if (newStatus == STATUS_UNKNOWN)
      {
         newStatus = mostCriticalAdditional;
      }
      else
      {
         newStatus = std::max(newStatus, mostCriticalAdditional);
      }
   }

   // Query loaded modules for object status
   ENUMERATE_MODULES(pfCalculateObjectStatus)
   {
      int moduleStatus = CURRENT_MODULE.pfCalculateObjectStatus(this);
      if (moduleStatus != STATUS_UNKNOWN)
      {
         if (newStatus == STATUS_UNKNOWN)
         {
            newStatus = moduleStatus;
         }
         else
         {
            newStatus = std::max(m_status, moduleStatus);
         }
      }
   }

   bool updateParents = forcedRecalc;
   lockProperties();
   if (newStatus != m_status)
   {
      m_status = newStatus;
      setModified(MODIFY_RUNTIME);  // only notify clients
      updateParents = true;
   }
   unlockProperties();

   // Cause parent object(s) to recalculate it's status
   if (updateParents)
   {
      readLockParentList();
      for(i = 0; i < getParentList().size(); i++)
         getParentList().get(i)->calculateCompoundStatus();
      unlockParentList();
   }
}

/**
 * Load ACL from database
 */
bool NetObj::loadACLFromDB(DB_HANDLE hdb)
{
   bool success = false;

	DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT user_id,access_rights FROM acl WHERE object_id=?"));
	if (hStmt != NULL)
	{
		DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
		DB_RESULT hResult = DBSelectPrepared(hStmt);
		if (hResult != NULL)
		{
			int count = DBGetNumRows(hResult);
			for(int i = 0; i < count; i++)
				m_accessList.addElement(DBGetFieldULong(hResult, i, 0), DBGetFieldULong(hResult, i, 1));
			DBFreeResult(hResult);
			success = true;
		}
		DBFreeStatement(hStmt);
	}
   return success;
}

/**
 * Handler for ACL elements enumeration
 */
static void EnumerationHandler(uint32_t userId, uint32_t accessRights, std::pair<uint32_t, DB_HANDLE> *context)
{
   TCHAR query[256];
   _sntprintf(query, sizeof(query) / sizeof(TCHAR), _T("INSERT INTO acl (object_id,user_id,access_rights) VALUES (%u,%u,%u)"),
            context->first, userId, accessRights);
   DBQuery(context->second, query);
}

/**
 * Save ACL to database
 */
bool NetObj::saveACLToDB(DB_HANDLE hdb)
{
   if (!(m_modified & MODIFY_ACCESS_LIST))
      return true;

   bool success = executeQueryOnObject(hdb, _T("DELETE FROM acl WHERE object_id=?"));
   if (success)
   {
      std::pair<uint32_t, DB_HANDLE> context(m_id, hdb);
      lockACL();
      m_accessList.enumerateElements(EnumerationHandler, &context);
      unlockACL();
   }
   return success;
}

/**
 * Callback for sending module data in NXCP message
 */
static EnumerationCallbackResult SendModuleDataCallback(const TCHAR *key, const ModuleData *value, std::pair<uint32_t, NXCPMessage*> *context)
{
   context->second->setField(context->first, key);
   const_cast<ModuleData*>(value)->fillMessage(context->second, context->first + 1);
   context->first += 0x100000;
   return _CONTINUE;
}

/**
 * Callback to fill message with custom attributes
 */
static EnumerationCallbackResult CustomAttributeFillMessage(const TCHAR *key, const CustomAttribute *attr, std::pair<NXCPMessage *, UINT32 *> *data)
{
   data->first->setField((*data->second)++, key);
   data->first->setField((*data->second)++, attr->value);
   data->first->setField((*data->second)++, attr->flags);
   data->first->setField((*data->second)++, attr->sourceObject);
   return _CONTINUE;
}

/**
 * Fill NXCP message with object's data
 * Object's properties are locked when this method is called. Method should not do any other locks.
 * Data required other locks should be filled in fillMessageInternalStage2().
 */
void NetObj::fillMessageInternal(NXCPMessage *msg, uint32_t userId)
{
   msg->setField(VID_OBJECT_CLASS, static_cast<uint16_t>(getObjectClass()));
   msg->setField(VID_OBJECT_ID, m_id);
	msg->setField(VID_GUID, m_guid);
   msg->setField(VID_OBJECT_NAME, m_name);
   msg->setField(VID_ALIAS, m_alias);
   msg->setField(VID_NAME_ON_MAP, m_nameOnMap);
   msg->setField(VID_OBJECT_STATUS, (WORD)m_status);
   msg->setField(VID_IS_DELETED, (WORD)(m_isDeleted ? 1 : 0));
   msg->setField(VID_IS_SYSTEM, (INT16)(m_isSystem ? 1 : 0));
   msg->setField(VID_MAINTENANCE_MODE, (INT16)(m_maintenanceEventId ? 1 : 0));
   msg->setField(VID_MAINTENANCE_INITIATOR, m_maintenanceInitiator);
   msg->setField(VID_FLAGS, m_flags);
   msg->setField(VID_PRIMARY_ZONE_PROXY_ID, m_primaryZoneProxyId);
   msg->setField(VID_BACKUP_ZONE_PROXY_ID, m_backupZoneProxyId);
   msg->setField(VID_INHERIT_RIGHTS, m_inheritAccessRights);
   msg->setField(VID_STATUS_CALCULATION_ALG, (WORD)m_statusCalcAlg);
   msg->setField(VID_STATUS_PROPAGATION_ALG, (WORD)m_statusPropAlg);
   msg->setField(VID_FIXED_STATUS, (WORD)m_fixedStatus);
   msg->setField(VID_STATUS_SHIFT, (WORD)m_statusShift);
   msg->setField(VID_STATUS_TRANSLATION_1, (WORD)m_statusTranslation[0]);
   msg->setField(VID_STATUS_TRANSLATION_2, (WORD)m_statusTranslation[1]);
   msg->setField(VID_STATUS_TRANSLATION_3, (WORD)m_statusTranslation[2]);
   msg->setField(VID_STATUS_TRANSLATION_4, (WORD)m_statusTranslation[3]);
   msg->setField(VID_STATUS_SINGLE_THRESHOLD, (WORD)m_statusSingleThreshold);
   msg->setField(VID_STATUS_THRESHOLD_1, (WORD)m_statusThresholds[0]);
   msg->setField(VID_STATUS_THRESHOLD_2, (WORD)m_statusThresholds[1]);
   msg->setField(VID_STATUS_THRESHOLD_3, (WORD)m_statusThresholds[2]);
   msg->setField(VID_STATUS_THRESHOLD_4, (WORD)m_statusThresholds[3]);
   msg->setField(VID_COMMENTS, CHECK_NULL_EX(m_comments));
   msg->setField(VID_COMMENTS_SOURCE, CHECK_NULL_EX(m_commentsSource));
   msg->setField(VID_IMAGE, m_mapImage);
   msg->setField(VID_DRILL_DOWN_OBJECT_ID, m_drilldownObjectId);
   msg->setField(VID_CATEGORY_ID, m_categoryId);
   msg->setField(VID_ASSET_ID, m_assetId);
	msg->setFieldFromTime(VID_CREATION_TIME, m_creationTime);
	if ((m_trustedObjects != nullptr) && !m_trustedObjects->isEmpty())
	{
		msg->setFieldFromInt32Array(VID_TRUSTED_OBJECTS, m_trustedObjects);
	}
   msg->setFieldFromInt32Array(VID_DASHBOARDS, m_dashboards);

   uint32_t base = VID_CUSTOM_ATTRIBUTES_BASE;
   std::pair<NXCPMessage *, UINT32 *> data(msg, &base);
   forEachCustomAttribute(CustomAttributeFillMessage, &data);
   msg->setField(VID_NUM_CUSTOM_ATTRIBUTES, getCustomAttributeSize());

	m_geoLocation.fillMessage(*msg);

   msg->setField(VID_COUNTRY, m_postalAddress.getCountry());
   msg->setField(VID_REGION, m_postalAddress.getRegion());
   msg->setField(VID_CITY, m_postalAddress.getCity());
   msg->setField(VID_DISTRICT, m_postalAddress.getDistrict());
   msg->setField(VID_STREET_ADDRESS, m_postalAddress.getStreetAddress());
   msg->setField(VID_POSTCODE, m_postalAddress.getPostCode());

   msg->setField(VID_NUM_URLS, static_cast<uint32_t>(m_urls.size()));
   uint32_t fieldId = VID_URL_LIST_BASE;
   for(int i = 0; i < m_urls.size(); i++)
   {
      const ObjectUrl *url = m_urls.get(i);
      msg->setField(fieldId++, url->getId());
      msg->setField(fieldId++, url->getUrl());
      msg->setField(fieldId++, url->getDescription());
      fieldId += 7;
   }
}

/**
 * Fill NXCP message with object's data - stage 2
 * Object's properties are not locked when this method is called. Should be
 * used only to fill data where properties lock is not enough (like data
 * collection configuration).
 */
void NetObj::fillMessageInternalStage2(NXCPMessage *msg, uint32_t userId)
{
   m_moduleDataLock.lock();
   if (m_moduleData != nullptr)
   {
      msg->setField(VID_MODULE_DATA_COUNT, static_cast<uint16_t>(m_moduleData->size()));
      std::pair<uint32_t, NXCPMessage*> context(VID_MODULE_DATA_BASE, msg);
      m_moduleData->forEach(SendModuleDataCallback, &context);
   }
   else
   {
      msg->setField(VID_MODULE_DATA_COUNT, static_cast<uint16_t>(0));
   }
   m_moduleDataLock.unlock();
}

/**
 * Fill NXCP message with object's data
 */
void NetObj::fillMessage(NXCPMessage *msg, uint32_t userId)
{
   lockProperties();
   fillMessageInternal(msg, userId);
   unlockProperties();
   fillMessageInternalStage2(msg, userId);

   lockACL();
   m_accessList.fillMessage(msg);
   unlockACL();

   uint32_t dwId;
   int i;

   readLockParentList();
   msg->setField(VID_PARENT_CNT, getParentList().size());
   for(i = 0, dwId = VID_PARENT_ID_BASE; i < getParentList().size(); i++, dwId++)
      msg->setField(dwId, getParentList().get(i)->getId());
   unlockParentList();

   readLockChildList();
   msg->setField(VID_CHILD_CNT, getChildList().size());
   for(i = 0, dwId = VID_CHILD_ID_BASE; i < getChildList().size(); i++, dwId++)
      msg->setField(dwId, getChildList().get(i)->getId());
   unlockChildList();

   lockResponsibleUsersList();
   if (m_responsibleUsers != nullptr)
   {
      msg->setField(VID_RESPONSIBLE_USERS_COUNT, m_responsibleUsers->size());
      uint32_t fieldId = VID_RESPONSIBLE_USERS_BASE;
      for(int i = 0; i < m_responsibleUsers->size(); i++)
      {
         ResponsibleUser *r = m_responsibleUsers->get(i);
         msg->setField(fieldId++, r->userId);
         msg->setField(fieldId++, r->tag);
         fieldId += 8;
      }
   }
   else
   {
      msg->setField(VID_RESPONSIBLE_USERS_COUNT, static_cast<uint32_t>(0));
   }
   unlockResponsibleUsersList();

   if (isPollable())
      getAsPollable()->pollStateToMessage(msg);
}

/**
 * Handler for EnumerateSessions()
 */
static void BroadcastObjectChange(ClientSession *session, NetObj *object)
{
   session->onObjectChange(object->self());
}

/**
 * Mark object as modified and put on client's notification queue
 */
void NetObj::setModified(uint32_t flags, bool notify)
{
   if (g_modificationsLocked)
      return;

   InterlockedOr(&m_modified, flags);
   m_timestamp = time(nullptr);

   // Send event to all connected clients
   if (notify && !m_isHidden && !m_isSystem)
   {
      nxlog_debug_tag(_T("obj.notify"), 7, _T("Sending object change notification for %s [%u] (flags=0x%08X)"), m_name, m_id, flags);
      EnumerateClientSessions(BroadcastObjectChange, this);
   }
}

/**
 * Modify object from NXCP message - common wrapper
 */
uint32_t NetObj::modifyFromMessage(const NXCPMessage& msg)
{
   lockProperties();
   uint32_t rcc = modifyFromMessageInternal(msg);
   unlockProperties();
   if (rcc == RCC_SUCCESS)
      rcc = modifyFromMessageInternalStage2(msg);
   markAsModified(MODIFY_ALL);
   return rcc;
}

/**
 * Update flags field
 */
void NetObj::updateFlags(uint32_t flags, uint32_t mask)
{
   m_flags &= ~mask;
   m_flags |= (flags & mask);
}

/**
 * Modify object from NXCP message
 */
uint32_t NetObj::modifyFromMessageInternal(const NXCPMessage& msg)
{
   // Change object's name
   if (msg.isFieldExist(VID_OBJECT_NAME))
   {
      msg.getFieldAsString(VID_OBJECT_NAME, m_name, MAX_OBJECT_NAME);

      // Cleanup
      for(int i = 0; m_name[i] != 0; i++)
      {
#ifdef UNICODE
         if (m_name[i] < 0x20)
            m_name[i] = L' ';
#else
         if (reinterpret_cast<BYTE*>(m_name)[i] < 0x20)
            m_name[i] = ' ';
#endif
      }
   }

   if (msg.isFieldExist(VID_ALIAS))
      m_alias = msg.getFieldAsSharedString(VID_ALIAS);

   // Change flags
   if (msg.isFieldExist(VID_FLAGS))
   {
      updateFlags(msg.getFieldAsUInt32(VID_FLAGS), msg.isFieldExist(VID_FLAGS_MASK) ? msg.getFieldAsUInt32(VID_FLAGS_MASK) : UINT_MAX);
   }

   if (msg.isFieldExist(VID_NAME_ON_MAP))
      m_nameOnMap = msg.getFieldAsSharedString(VID_NAME_ON_MAP);

   // Change object's status calculation/propagation algorithms
   if (msg.isFieldExist(VID_STATUS_CALCULATION_ALG))
   {
      m_statusCalcAlg = msg.getFieldAsInt16(VID_STATUS_CALCULATION_ALG);
      m_statusPropAlg = msg.getFieldAsInt16(VID_STATUS_PROPAGATION_ALG);
      m_fixedStatus = msg.getFieldAsInt16(VID_FIXED_STATUS);
      m_statusShift = msg.getFieldAsInt16(VID_STATUS_SHIFT);
      m_statusTranslation[0] = msg.getFieldAsInt16(VID_STATUS_TRANSLATION_1);
      m_statusTranslation[1] = msg.getFieldAsInt16(VID_STATUS_TRANSLATION_2);
      m_statusTranslation[2] = msg.getFieldAsInt16(VID_STATUS_TRANSLATION_3);
      m_statusTranslation[3] = msg.getFieldAsInt16(VID_STATUS_TRANSLATION_4);
      m_statusSingleThreshold = msg.getFieldAsInt16(VID_STATUS_SINGLE_THRESHOLD);
      m_statusThresholds[0] = msg.getFieldAsInt16(VID_STATUS_THRESHOLD_1);
      m_statusThresholds[1] = msg.getFieldAsInt16(VID_STATUS_THRESHOLD_2);
      m_statusThresholds[2] = msg.getFieldAsInt16(VID_STATUS_THRESHOLD_3);
      m_statusThresholds[3] = msg.getFieldAsInt16(VID_STATUS_THRESHOLD_4);
   }

	// Change image
	if (msg.isFieldExist(VID_IMAGE))
		m_mapImage = msg.getFieldAsGUID(VID_IMAGE);

	// Object category
	if (msg.isFieldExist(VID_CATEGORY_ID))
	   m_categoryId = msg.getFieldAsUInt32(VID_CATEGORY_ID);

   // Change object's ACL
   if (msg.isFieldExist(VID_ACL_SIZE))
   {
      lockACL();
      m_inheritAccessRights = msg.getFieldAsBoolean(VID_INHERIT_RIGHTS);
      m_accessList.deleteAll();
      int count = msg.getFieldAsUInt32(VID_ACL_SIZE);
      for(int i = 0; i < count; i++)
         m_accessList.addElement(msg.getFieldAsUInt32(VID_ACL_USER_BASE + i), msg.getFieldAsUInt32(VID_ACL_RIGHTS_BASE + i));
      unlockACL();
   }

	// Change trusted nodes list
   if (msg.isFieldExist(VID_TRUSTED_OBJECTS))
   {
      size_t size;
      msg.getBinaryFieldPtr(VID_TRUSTED_OBJECTS, &size);
      if (size > 0)
      {
         if (m_trustedObjects == nullptr)
            m_trustedObjects = new IntegerArray<uint32_t>();
         msg.getFieldAsInt32Array(VID_TRUSTED_OBJECTS, m_trustedObjects);
      }
      else
      {
         delete_and_null(m_trustedObjects);
      }
   }

	// Change geolocation
	if (msg.isFieldExist(VID_GEOLOCATION_TYPE))
	{
		m_geoLocation = GeoLocation(msg);
		TCHAR key[32];
		_sntprintf(key, 32, _T("GLupd-%u"), m_id);
		ThreadPoolExecuteSerialized(g_mainThreadPool, key, self(), &NetObj::updateGeoLocationHistory, m_geoLocation);
	}

	if (msg.isFieldExist(VID_DRILL_DOWN_OBJECT_ID))
	{
	   m_drilldownObjectId = msg.getFieldAsUInt32(VID_DRILL_DOWN_OBJECT_ID);
	}

   if (msg.isFieldExist(VID_COUNTRY))
   {
      TCHAR buffer[64];
      msg.getFieldAsString(VID_COUNTRY, buffer, 64);
      m_postalAddress.setCountry(buffer);
   }

   if (msg.isFieldExist(VID_REGION))
   {
      TCHAR buffer[64];
      msg.getFieldAsString(VID_REGION, buffer, 64);
      m_postalAddress.setRegion(buffer);
   }

   if (msg.isFieldExist(VID_CITY))
   {
      TCHAR buffer[64];
      msg.getFieldAsString(VID_CITY, buffer, 64);
      m_postalAddress.setCity(buffer);
   }

   if (msg.isFieldExist(VID_DISTRICT))
   {
      TCHAR buffer[64];
      msg.getFieldAsString(VID_DISTRICT, buffer, 64);
      m_postalAddress.setDistrict(buffer);
   }

   if (msg.isFieldExist(VID_STREET_ADDRESS))
   {
      TCHAR buffer[256];
      msg.getFieldAsString(VID_STREET_ADDRESS, buffer, 256);
      m_postalAddress.setStreetAddress(buffer);
   }

   if (msg.isFieldExist(VID_POSTCODE))
   {
      TCHAR buffer[32];
      msg.getFieldAsString(VID_POSTCODE, buffer, 32);
      m_postalAddress.setPostCode(buffer);
   }

   // Change dashboard list
   if (msg.isFieldExist(VID_DASHBOARDS))
   {
      msg.getFieldAsInt32Array(VID_DASHBOARDS, &m_dashboards);
   }

   // Update URL list
   if (msg.isFieldExist(VID_NUM_URLS))
   {
      m_urls.clear();
      int count = msg.getFieldAsInt32(VID_NUM_URLS);
      uint32_t fieldId = VID_URL_LIST_BASE;
      for(int i = 0; i < count; i++)
      {
         m_urls.add(new ObjectUrl(msg, fieldId));
         fieldId += 10;
      }
   }

   return RCC_SUCCESS;
}

/**
 * Modify object from NXCP message - stage 2
 * Object's properties are not locked when this method is called. Should be
 * used when more direct control over locks is needed (for example when
 * code should lock parent or children list without holding property lock).
 */
uint32_t NetObj::modifyFromMessageInternalStage2(const NXCPMessage& msg)
{
   // Change custom attributes
   if (msg.isFieldExist(VID_NUM_CUSTOM_ATTRIBUTES))
   {
      setCustomAttributesFromMessage(msg);
   }

   int count = msg.getFieldAsInt32(VID_RESPONSIBLE_USERS_COUNT);
   lockResponsibleUsersList();
   if (count > 0)
   {
      if (m_responsibleUsers == nullptr)
         m_responsibleUsers = new StructArray<ResponsibleUser>(count, 16);
      else
         m_responsibleUsers->clear();

      uint32_t fieldId = VID_RESPONSIBLE_USERS_BASE;
      for(int i = 0; i < count; i++)
      {
         ResponsibleUser *r = m_responsibleUsers->addPlaceholder();
         r->userId = msg.getFieldAsUInt32(fieldId++);
         msg.getFieldAsString(fieldId++, r->tag, MAX_RESPONSIBLE_USER_TAG_LEN);
         fieldId += 8;
      }
   }
   else
   {
      delete_and_null(m_responsibleUsers);
   }
   unlockResponsibleUsersList();

   return RCC_SUCCESS;
}

/**
 * Post-modify hook
 */
void NetObj::postModify()
{
   calculateCompoundStatus(true);
}

/**
 * Get rights to object for specific user
 *
 * @param userId user object ID
 */
uint32_t NetObj::getUserRights(uint32_t userId) const
{
   uint32_t rights;

   // System always has all rights to any object
   if (userId == 0)
      return 0xFFFFFFFF;

	// Non-admin users have no rights to system objects
	if (m_isSystem)
		return 0;

   // Check if have direct right assignment
   lockACL();
   bool hasDirectRights = m_accessList.getUserRights(userId, &rights);
   unlockACL();

   if (!hasDirectRights)
   {
      // We don't. If this object inherit rights from parents, get them
      if (m_inheritAccessRights)
      {
         rights = 0;
         readLockParentList();
         for(int i = 0; i < getParentList().size(); i++)
            rights |= getParentList().get(i)->getUserRights(userId);
         unlockParentList();
      }
   }

   return rights;
}

/**
 * Check if given user has specific rights on this object
 *
 * @param userId user object ID
 * @param requiredRights bit mask of requested right
 * @return true if user has all rights specified in requested rights bit mask
 */
bool NetObj::checkAccessRights(uint32_t userId, uint32_t requiredRights) const
{
   uint32_t effectiveRights = getUserRights(userId);
   return (effectiveRights & requiredRights) == requiredRights;
}

/**
 * Drop all user privileges on current object
 */
void NetObj::dropUserAccess(uint32_t userId)
{
   lockACL();
   bool modified = m_accessList.deleteElement(userId);
   unlockACL();
   if (modified)
   {
      lockProperties();
      setModified(MODIFY_ACCESS_LIST);
      unlockProperties();
   }
}

/**
 * Set object's management status
 */
bool NetObj::setMgmtStatus(bool isManaged)
{
   int oldStatus;

   lockProperties();

   if ((isManaged && (m_status != STATUS_UNMANAGED)) ||
       ((!isManaged) && (m_status == STATUS_UNMANAGED)))
   {
      unlockProperties();
      return false;  // Status is already correct
   }

   oldStatus = m_status;
   m_status = (isManaged ? STATUS_UNKNOWN : STATUS_UNMANAGED);

   setModified(MODIFY_COMMON_PROPERTIES);
   unlockProperties();

   // Generate event if current object is a node
   if (getObjectClass() == OBJECT_NODE)
      EventBuilder(isManaged ? EVENT_NODE_UNKNOWN : EVENT_NODE_UNMANAGED, m_id)
         .param(_T("previousNodeStatus"), oldStatus)
         .post();

   // Change status for child objects also
   readLockChildList();
   for(int i = 0; i < getChildList().size(); i++)
      getChildList().get(i)->setMgmtStatus(isManaged);
   unlockChildList();

   // Cause parent object(s) to recalculate it's status
   readLockParentList();
   for(int i = 0; i < getParentList().size(); i++)
      getParentList().get(i)->calculateCompoundStatus();
   unlockParentList();
   return true;
}

/**
 * Send message to client, who requests poll, if any
 * This method is used by Node and Interface class objects
 */
void NetObj::sendPollerMsg(const TCHAR *format, ...)
{
   if (m_pollRequestor == nullptr)
      return;

   va_list args;
   va_start(args, format);
   TCHAR buffer[1024];
   _vsntprintf(buffer, 1024, format, args);
   va_end(args);
   m_pollRequestor->sendPollerMsg(m_pollRequestId, buffer);
}

/**
 * Add child node objects (direct and indirect childs) to list
 */
void NetObj::addChildNodesToList(SharedObjectArray<Node> *nodeList, uint32_t userId)
{
   readLockChildList();

   // Walk through our own child list
   for(int i = 0; i < getChildList().size(); i++)
   {
      NetObj *object = getChildList().get(i);
      if (!object->checkAccessRights(userId, OBJECT_ACCESS_READ))
         continue;

      if (object->getObjectClass() == OBJECT_NODE)
      {
         // Check if this node already in the list
			int j;
			for(j = 0; j < nodeList->size(); j++)
				if (nodeList->get(j)->getId() == object->getId())
               break;
         if (j == nodeList->size())
				nodeList->add(static_pointer_cast<Node>(getChildList().getShared(i)));
      }
      else
      {
         object->addChildNodesToList(nodeList, userId);
      }
   }

   unlockChildList();
}

/**
 * Add child data collection targets (direct and indirect childs) to list
 */
void NetObj::addChildDCTargetsToList(SharedObjectArray<DataCollectionTarget> *dctList, uint32_t userId)
{
   readLockChildList();

   // Walk through our own child list
   for(int i = 0; i < getChildList().size(); i++)
   {
      NetObj *object = getChildList().get(i);
      if (!object->checkAccessRights(userId, OBJECT_ACCESS_READ))
         continue;

      if (object->isDataCollectionTarget())
      {
         // Check if this objects already in the list
			int j;
			for(j = 0; j < dctList->size(); j++)
				if (dctList->get(j)->getId() == object->getId())
               break;
         if (j == dctList->size())
				dctList->add(static_pointer_cast<DataCollectionTarget>(getChildList().getShared(i)));
      }
      object->addChildDCTargetsToList(dctList, userId);
   }

   unlockChildList();
}

/**
 * Hide object and all its children
 */
void NetObj::hide()
{
   readLockChildList();
   for(int i = 0; i < getChildList().size(); i++)
      getChildList().get(i)->hide();
   unlockChildList();

	lockProperties();
   m_isHidden = true;
   unlockProperties();
}

/**
 * Unhide object and all its children
 */
void NetObj::unhide()
{
   lockProperties();
   m_isHidden = false;
   if (!m_isSystem)
      EnumerateClientSessions(BroadcastObjectChange, this);
   unlockProperties();

   readLockChildList();
   for(int i = 0; i < getChildList().size(); i++)
      getChildList().get(i)->unhide();
   unlockChildList();
}

/**
 * Return status propagated to parent
 */
int NetObj::getPropagatedStatus()
{
   int iStatus;

   if (m_statusPropAlg == SA_PROPAGATE_DEFAULT)
   {
      iStatus = DefaultPropagatedStatus(m_status);
   }
   else
   {
      switch(m_statusPropAlg)
      {
         case SA_PROPAGATE_UNCHANGED:
            iStatus = m_status;
            break;
         case SA_PROPAGATE_FIXED:
            iStatus = ((m_status > STATUS_NORMAL) && (m_status < STATUS_UNKNOWN)) ? m_fixedStatus : m_status;
            break;
         case SA_PROPAGATE_RELATIVE:
            if ((m_status > STATUS_NORMAL) && (m_status < STATUS_UNKNOWN))
            {
               iStatus = m_status + m_statusShift;
               if (iStatus < 0)
                  iStatus = 0;
               if (iStatus > STATUS_CRITICAL)
                  iStatus = STATUS_CRITICAL;
            }
            else
            {
               iStatus = m_status;
            }
            break;
         case SA_PROPAGATE_TRANSLATED:
            if ((m_status > STATUS_NORMAL) && (m_status < STATUS_UNKNOWN))
            {
               iStatus = m_statusTranslation[m_status - 1];
            }
            else
            {
               iStatus = m_status;
            }
            break;
         default:
            iStatus = STATUS_UNKNOWN;
            break;
      }
   }
   return iStatus;
}

/**
 * Prepare object for deletion. Method should return only
 * when object deletion is safe
 */
void NetObj::prepareForDeletion()
{
   if (m_assetId != 0)
   {
      shared_ptr<Asset> asset = static_pointer_cast<Asset>(FindObjectById(m_assetId, OBJECT_ASSET));

      if (asset != nullptr)
      {
         UnlinkAsset(asset.get(), nullptr);
      }
   }
}

/**
 * Cleanup object before server shutdown
 */
void NetObj::cleanup()
{
}

/**
 * Set object's description.
 */
void NetObj::setAlias(const TCHAR *alias)
{
   lockProperties();
   m_alias = alias;
   setModified(MODIFY_COMMON_PROPERTIES);
   unlockProperties();
}

/**
 * Set object's name on map
 */
void NetObj::setNameOnMap(const TCHAR *name)
{
   lockProperties();
   m_nameOnMap = name;
   setModified(MODIFY_COMMON_PROPERTIES);
   unlockProperties();
}

/**
 * Expands m_commentsSource macros into m_comments.
 * Locks object properties.
 */
void NetObj::expandCommentMacros()
{
   nxlog_debug_tag(_T("obj.comments"), 8, _T("Updating %s comments"), m_name);
   if (!m_commentsSource.isNull() && !m_commentsSource.isEmpty())
   {
      const StringBuffer expandedComments = expandText(m_commentsSource, nullptr, nullptr, shared_ptr<DCObjectInfo>(), nullptr, nullptr, nullptr, nullptr, nullptr);
      lockProperties();
      m_comments = expandedComments;
      setModified(MODIFY_COMMON_PROPERTIES);
      unlockProperties();
   }
}

/**
 * Set object's comments.
 */
void NetObj::setComments(const TCHAR *comments)
{
   const StringBuffer expandedComments = expandText(comments, nullptr, nullptr, shared_ptr<DCObjectInfo>(), nullptr, nullptr, nullptr, nullptr, nullptr);
   lockProperties();
   m_commentsSource = comments;
   m_comments = expandedComments;
   setModified(MODIFY_COMMON_PROPERTIES);
   unlockProperties();
}

/**
 * Copy object's comments to NXCP message
 */
void NetObj::commentsToMessage(NXCPMessage *pMsg)
{
   lockProperties();
   pMsg->setField(VID_COMMENTS, m_comments);
   pMsg->setField(VID_COMMENTS_SOURCE, m_commentsSource);
   unlockProperties();
}

/**
 * Load trusted nodes list from database
 */
bool NetObj::loadTrustedObjects(DB_HANDLE hdb)
{
	TCHAR query[256];
	_sntprintf(query, 256, _T("SELECT trusted_object_id FROM trusted_objects WHERE object_id=%u"), m_id);
	DB_RESULT hResult = DBSelect(hdb, query);
	if (hResult != nullptr)
	{
		int numElements = DBGetNumRows(hResult);
		if (numElements > 0)
		{
		   m_trustedObjects = new IntegerArray<uint32_t>(numElements);
			for(int i = 0; i < numElements; i++)
			{
			   m_trustedObjects->add(DBGetFieldULong(hResult, i, 0));
			}
		}
		DBFreeResult(hResult);
	}
	return (hResult != nullptr);
}

/**
 * Save list of trusted nodes to database
 */
bool NetObj::saveTrustedObjects(DB_HANDLE hdb)
{
   bool success = executeQueryOnObject(hdb, _T("DELETE FROM trusted_objects WHERE object_id=?"));
   lockProperties();
	if (success && (m_trustedObjects != nullptr) && !m_trustedObjects->isEmpty())
	{
	   DB_STATEMENT hStmt = DBPrepare(hdb, _T("INSERT INTO trusted_objects (object_id,trusted_object_id) VALUES (?,?)"), m_trustedObjects->size() > 1);
	   if (hStmt != nullptr)
	   {
	      DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_id);
         for(int i = 0; (i < m_trustedObjects->size()) && success; i++)
         {
            DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, m_trustedObjects->get(i));
            success = DBExecute(hStmt);
         }
         DBFreeStatement(hStmt);
	   }
	   else
	   {
	      success = false;
	   }
	}
   unlockProperties();
	return success;
}

/**
 * Check if given object is in trust list
 * Will always return TRUE if system parameter Objects.Security.CheckTrustedObjects set to false
 */
bool NetObj::isTrustedObject(uint32_t id) const
{
   if (!(g_flags & AF_CHECK_TRUSTED_OBJECTS))
      return true;

   lockProperties();
   bool rc = (m_trustedObjects != nullptr) ? m_trustedObjects->contains(id) : false;
   unlockProperties();
	return rc;
}

/**
 * Get list of parent objects for NXSL script
 */
NXSL_Value *NetObj::getParentsForNXSL(NXSL_VM *vm)
{
	NXSL_Array *parents = new NXSL_Array(vm);
	int index = 0;

	readLockParentList();
	for(int i = 0; i < getParentList().size(); i++)
	{
	   NetObj *obj = getParentList().get(i);
		if (obj->getObjectClass() != OBJECT_TEMPLATE)
		{
			parents->set(index++, obj->createNXSLObject(vm));
		}
	}
	unlockParentList();

	return vm->createValue(parents);
}

/**
 * Get list of child objects for NXSL script
 */
NXSL_Value *NetObj::getChildrenForNXSL(NXSL_VM *vm)
{
	NXSL_Array *children = new NXSL_Array(vm);
	int index = 0;

	readLockChildList();
	for(int i = 0; i < getChildList().size(); i++)
	{
      children->set(index++, getChildList().get(i)->createNXSLObject(vm));
	}
	unlockChildList();

	return vm->createValue(children);
}

/**
 * Get full list of child objects (including both direct and indirect children)
 */
void NetObj::getFullChildListInternal(ObjectIndex *list, bool eventSourceOnly) const
{
	readLockChildList();
	for(int i = 0; i < getChildList().size(); i++)
	{
	   NetObj *object = getChildList().get(i);
		if (!eventSourceOnly || IsEventSource(object->getObjectClass()))
		{
			list->put(object->getId(), object->self());
		}
		object->getFullChildListInternal(list, eventSourceOnly);
	}
	unlockChildList();
}

/**
 * Get full list of child objects (including both direct and indirect children).
 *  Returned array is dynamically allocated and must be deleted by the caller.
 *
 * @param eventSourceOnly if true, only objects that can be event source will be included
 */
unique_ptr<SharedObjectArray<NetObj>> NetObj::getAllChildren(bool eventSourceOnly) const
{
	ObjectIndex list;
	getFullChildListInternal(&list, eventSourceOnly);
	return list.getObjects();
}

/**
 * Get list of child objects (direct only). Returned array is
 * dynamically allocated and must be deleted by the caller.
 *
 * @param typeFilter Only return objects with class ID equals given value.
 *                   Set to -1 to disable filtering.
 */
unique_ptr<SharedObjectArray<NetObj>> NetObj::getChildren(int typeFilter) const
{
	readLockChildList();
	auto list = new SharedObjectArray<NetObj>(getChildList().size());
	for(int i = 0; i < getChildList().size(); i++)
	{
		if ((typeFilter == -1) || (typeFilter == getChildList().get(i)->getObjectClass()))
			list->add(getChildList().getShared(i));
	}
	unlockChildList();
	return unique_ptr<SharedObjectArray<NetObj>>(list);
}

/**
 * Get count of child objects with optional type filter.
 */
int NetObj::getChildrenCount(int typeFilter) const
{
   int count;
   readLockChildList();
   if (typeFilter == -1)
   {
      count = getChildList().size();
   }
   else
   {
      count = 0;
      for(int i = 0; i < getChildList().size(); i++)
      {
          if (typeFilter == getChildList().get(i)->getObjectClass())
             count++;
      }
   }
   unlockChildList();
   return count;
}

/**
 * Get list of parent objects (direct only). Returned array is
 * dynamically allocated and must be deleted by the caller.
 *
 * @param typeFilter Only return objects with class ID equals given value.
 *                   Set to -1 to disable filtering.
 */
unique_ptr<SharedObjectArray<NetObj>> NetObj::getParents(int typeFilter) const
{
    readLockParentList();
    auto list = new SharedObjectArray<NetObj>(getParentList().size(), 16);
    for(int i = 0; i < getParentList().size(); i++)
    {
        if ((typeFilter == -1) || (typeFilter == getParentList().get(i)->getObjectClass()))
           list->add(getParentList().getShared(i));
    }
    unlockParentList();
    return unique_ptr<SharedObjectArray<NetObj>>(list);
}

/**
 * Get count of parent objects with optional type filter.
 */
int NetObj::getParentsCount(int typeFilter) const
{
   int count;
   readLockParentList();
   if (typeFilter == -1)
   {
      count = getParentList().size();
   }
   else
   {
      count = 0;
      for(int i = 0; i < getParentList().size(); i++)
      {
          if (typeFilter == getParentList().get(i)->getObjectClass())
             count++;
      }
   }
   unlockParentList();
   return count;
}

/**
 * FInd child object by name (with optional class filter)
 */
shared_ptr<NetObj> NetObj::findChildObject(const TCHAR *name, int typeFilter) const
{
   shared_ptr<NetObj> object;
	readLockChildList();
	for(int i = 0; i < getChildList().size(); i++)
	{
	   NetObj *o = getChildList().get(i);
      if (((typeFilter == -1) || (typeFilter == o->getObjectClass())) && !_tcsicmp(name, o->getName()))
      {
         object = o->self();
         break;
      }
	}
	unlockChildList();
	return object;
}

/**
 * Find child node by IP address
 */
shared_ptr<Node> NetObj::findChildNode(const InetAddress& addr) const
{
   shared_ptr<Node> node;
   readLockChildList();
   for(int i = 0; i < getChildList().size(); i++)
   {
      NetObj *curr = getChildList().get(i);
      if ((curr->getObjectClass() == OBJECT_NODE) && addr.equals(static_cast<Node*>(curr)->getIpAddress()))
      {
         node = static_cast<Node*>(curr)->self();
         break;
      }
   }
   unlockChildList();
   return node;
}

/**
 * Update indexes for this object and all sub-objects
 */
void NetObj::updateObjectIndexes()
{
   NetObjInsert(self(), false, false);
   readLockChildList();
   for(int i = 0; i < getChildList().size(); i++)
   {
      NetObjInsert(getChildList().getShared(i), false, false);
   }
   unlockChildList();
}

/**
 * Called by client session handler to check if threshold summary should
 * be shown for this object. Default implementation always returns false.
 */
bool NetObj::showThresholdSummary() const
{
	return false;
}

/**
 * Must return true if object is a possible event source
 */
bool NetObj::isEventSource() const
{
   return false;
}

/**
 * Must return true if object is a possible data collection target
 */
bool NetObj::isDataCollectionTarget() const
{
   return false;
}

/**
 * Get module data
 */
ModuleData *NetObj::getModuleData(const TCHAR *module)
{
   m_moduleDataLock.lock();
   ModuleData *data = (m_moduleData != nullptr) ? m_moduleData->get(module) : nullptr;
   m_moduleDataLock.unlock();
   return data;
}

/**
 * Set module data
 */
void NetObj::setModuleData(const TCHAR *module, ModuleData *data)
{
   m_moduleDataLock.lock();
   if (m_moduleData == nullptr)
      m_moduleData = new StringObjectMap<ModuleData>(Ownership::True);
   m_moduleData->set(module, data);
   m_moduleDataLock.unlock();
}

/**
 * Add new location entry
 */
void NetObj::updateGeoLocationHistory(GeoLocation location)
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   if (!isGeoLocationHistoryTableExists(hdb))
   {
      nxlog_debug_tag(DEBUG_TAG_GEOLOCATION, 4, _T("NetObj::updateGeoLocationHistory: Geolocation history table will be created for object %s [%d]"), m_name, m_id);
      if (!createGeoLocationHistoryTable(hdb))
      {
         nxlog_debug_tag(DEBUG_TAG_GEOLOCATION, 4, _T("NetObj::updateGeoLocationHistory: Error creating geolocation history table for object %s [%d]"), m_name, m_id);
         DBConnectionPoolReleaseConnection(hdb);
         return;
      }
   }

   bool isSamePlace;
   double latitude = 0;
   double longitude = 0;
   uint32_t accuracy = 0;
   time_t startTimestamp = 0;
   DB_RESULT hResult;

	const TCHAR *query;
	switch(g_dbSyntax)
	{
		case DB_SYNTAX_ORACLE:
			query = _T("SELECT * FROM (SELECT latitude,longitude,accuracy,start_timestamp FROM gps_history_%u ORDER BY start_timestamp DESC) WHERE ROWNUM<=1");
			break;
		case DB_SYNTAX_MSSQL:
			query = _T("SELECT TOP 1 latitude,longitude,accuracy,start_timestamp FROM gps_history_%u ORDER BY start_timestamp DESC");
			break;
		case DB_SYNTAX_DB2:
			query = _T("SELECT latitude,longitude,accuracy,start_timestamp FROM gps_history_%u ORDER BY start_timestamp DESC FETCH FIRST 200 ROWS ONLY");
			break;
		default:
			query = _T("SELECT latitude,longitude,accuracy,start_timestamp FROM gps_history_%u ORDER BY start_timestamp DESC LIMIT 1");
			break;
	}
   TCHAR preparedQuery[256];
	_sntprintf(preparedQuery, 256, query, m_id);
	DB_STATEMENT hStmt = DBPrepare(hdb, preparedQuery);

   if (hStmt == nullptr)
		goto onFail;

   hResult = DBSelectPrepared(hStmt);
   if (hResult == nullptr)
		goto onFail;
   if (DBGetNumRows(hResult) > 0)
   {
      latitude = DBGetFieldDouble(hResult, 0, 0);
      longitude = DBGetFieldDouble(hResult, 0, 1);
      accuracy = DBGetFieldLong(hResult, 0, 2);
      startTimestamp = static_cast<time_t>(DBGetFieldUInt64(hResult, 0, 3));
      isSamePlace = location.sameLocation(latitude, longitude, accuracy);
   }
   else
   {
      isSamePlace = false;
   }
   DBFreeResult(hResult);
   DBFreeStatement(hStmt);

   if (location.getTimestamp() > startTimestamp)
   {
      if (isSamePlace)
      {
         TCHAR query[256];
         _sntprintf(query, 256, _T("UPDATE gps_history_%u SET end_timestamp=? WHERE start_timestamp=?"), m_id);
         hStmt = DBPrepare(hdb, query);
         if (hStmt == nullptr)
            goto onFail;
         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(location.getTimestamp()));
         DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(startTimestamp));
      }
      else
      {
         TCHAR query[256];
         _sntprintf(query, 256, _T("INSERT INTO gps_history_%u (latitude,longitude,accuracy,start_timestamp,end_timestamp) VALUES (?,?,?,?,?)"), m_id);
         hStmt = DBPrepare(hdb, query);
         if (hStmt == nullptr)
            goto onFail;

         TCHAR lat[32], lon[32];
         _sntprintf(lat, 32, _T("%f"), location.getLatitude());
         _sntprintf(lon, 32, _T("%f"), location.getLongitude());

         DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, lat, DB_BIND_STATIC);
         DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, lon, DB_BIND_STATIC);
         DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, location.getAccuracy());
         DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(location.getTimestamp()));
         DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(location.getTimestamp()));
      }

      if (!DBExecute(hStmt))
      {
         nxlog_debug_tag(DEBUG_TAG_GEOLOCATION, 1, _T("NetObj::updateGeoLocationHistory(%s [%d]): Failed to add location to history (new=[%f, %f, %u, %u] last=[%f, %f, %u, %u])"),
               m_name, m_id, location.getLatitude(), location.getLongitude(), location.getAccuracy(), static_cast<uint32_t>(location.getTimestamp()), latitude, longitude, accuracy, startTimestamp);
      }
      DBFreeStatement(hStmt);
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG_GEOLOCATION, 1, _T("NetObj::updateGeoLocationHistory(%s [%d]): location ignored because it is older then last known position (new=[%f, %f, %u, %u] last=[%f, %f, %u, %u])"),
            m_name, m_id, location.getLatitude(), location.getLongitude(), location.getAccuracy(), static_cast<uint32_t>(location.getTimestamp()), latitude, longitude, accuracy, startTimestamp);
   }

   DBConnectionPoolReleaseConnection(hdb);
   return;

onFail:
   if (hStmt != nullptr)
      DBFreeStatement(hStmt);
   nxlog_debug_tag(DEBUG_TAG_GEOLOCATION, 4, _T("NetObj::updateGeoLocationHistory(%s [%d]): Failed to add location to history"), m_name, m_id);
   DBConnectionPoolReleaseConnection(hdb);
   return;
}

/**
 * Check if given data table exist
 */
bool NetObj::isGeoLocationHistoryTableExists(DB_HANDLE hdb) const
{
   TCHAR table[256];
   _sntprintf(table, 256, _T("gps_history_%u"), m_id);
   int rc = DBIsTableExist(hdb, table);
   if (rc == DBIsTableExist_Failure)
   {
      nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG_GEOLOCATION, _T("Call to DBIsTableExist(\"%s\") failed"), table);
   }
   return rc != DBIsTableExist_NotFound;
}

/**
 * Create table for storing geolocation history for this object
 */
bool NetObj::createGeoLocationHistoryTable(DB_HANDLE hdb)
{
   TCHAR query[256], queryTemplate[256];
   MetaDataReadStr(_T("LocationHistory"), queryTemplate, 256, _T(""));
   _sntprintf(query, 256, queryTemplate, m_id);
   return DBQuery(hdb, query);
}

/**
 * Clean geolocation historical data
 */
void NetObj::cleanGeoLocationHistoryTable(DB_HANDLE hdb, int64_t retentionTime)
{
   if (isGeoLocationHistoryTableExists(hdb))
   {
      TCHAR query[256];
      _sntprintf(query, 256, _T("DELETE FROM gps_history_%u WHERE start_timestamp<") INT64_FMT, m_id, retentionTime);
      DBQuery(hdb, query);
   }
}

/**
 * Set status calculation method
 */
void NetObj::setStatusCalculation(int method, int arg1, int arg2, int arg3, int arg4)
{
   lockProperties();
   m_statusCalcAlg = method;
   switch(method)
   {
      case SA_CALCULATE_SINGLE_THRESHOLD:
         m_statusSingleThreshold = arg1;
         break;
      case SA_CALCULATE_MULTIPLE_THRESHOLDS:
         m_statusThresholds[0] = arg1;
         m_statusThresholds[1] = arg2;
         m_statusThresholds[2] = arg3;
         m_statusThresholds[3] = arg4;
         break;
      default:
         break;
   }
   setModified(MODIFY_COMMON_PROPERTIES);
   unlockProperties();
}

/**
 * Set status propagation method
 */
void NetObj::setStatusPropagation(int method, int arg1, int arg2, int arg3, int arg4)
{
   lockProperties();
   m_statusPropAlg = method;
   switch(method)
   {
      case SA_PROPAGATE_FIXED:
         m_fixedStatus = arg1;
         break;
      case SA_PROPAGATE_RELATIVE:
         m_statusShift = arg1;
         break;
      case SA_PROPAGATE_TRANSLATED:
         m_statusTranslation[0] = arg1;
         m_statusTranslation[1] = arg2;
         m_statusTranslation[2] = arg3;
         m_statusTranslation[3] = arg4;
         break;
      default:
         break;
   }
   setModified(MODIFY_COMMON_PROPERTIES);
   unlockProperties();
}

/**
 * Set geographical location
 */
void NetObj::setGeoLocation(const GeoLocation& geoLocation)
{
   lockProperties();
   if (!m_geoLocation.equals(geoLocation))
   {
      m_geoLocation = geoLocation;
      TCHAR key[32];
      _sntprintf(key, 32, _T("GLupd-%u"), m_id);
      ThreadPoolExecuteSerialized(g_mainThreadPool, key, self(), &NetObj::updateGeoLocationHistory, m_geoLocation);
      setModified(MODIFY_COMMON_PROPERTIES);
   }
   unlockProperties();
}

/**
 * Enter maintenance mode
 */
void NetObj::enterMaintenanceMode(uint32_t userId, const TCHAR *comments)
{
   nxlog_debug_tag(DEBUG_TAG_MAINTENANCE, 4, _T("Entering maintenance mode for object %s [%d] (%s)"), m_name, m_id, getObjectClassName());

   readLockChildList();
   for(int i = 0; i < getChildList().size(); i++)
   {
      NetObj *object = getChildList().get(i);
      if (object->getStatus() != STATUS_UNMANAGED)
         object->enterMaintenanceMode(userId, comments);
   }
   unlockChildList();
}

/**
 * Leave maintenance mode
 */
void NetObj::leaveMaintenanceMode(uint32_t userId)
{
   nxlog_debug_tag(DEBUG_TAG_MAINTENANCE, 4, _T("Leaving maintenance mode for object %s [%d] (%s)"), m_name, m_id, getObjectClassName());

   readLockChildList();
   for(int i = 0; i < getChildList().size(); i++)
   {
      NetObj *object = getChildList().get(i);
      if (object->getStatus() != STATUS_UNMANAGED)
         object->leaveMaintenanceMode(userId);
   }
   unlockChildList();
}

/**
 * Create NXSL object for this object
 */
NXSL_Value *NetObj::createNXSLObject(NXSL_VM *vm)
{
   return vm->createValue(vm->createObject(&g_nxslNetObjClass, new shared_ptr<NetObj>(self())));
}

/**
 * Execute hook script
 *
 * @param hookName hook name. Will find and execute script named Hook::hookName
 */
void NetObj::executeHookScript(const TCHAR *hookName)
{
   if (g_flags & AF_SHUTDOWN)
      return;

   TCHAR scriptName[MAX_PATH] = _T("Hook::");
   _tcslcpy(&scriptName[6], hookName, MAX_PATH - 6);
   ScriptVMHandle vm = CreateServerScriptVM(scriptName, self());
   if (!vm.isValid())
   {
      nxlog_debug_tag(DEBUG_TAG_OBJECT_LIFECYCLE, 7, _T("NetObj::executeHookScript(%s [%u]): hook script \"%s\" %s"), m_name, m_id,
               scriptName, (vm.failureReason() == ScriptVMFailureReason::SCRIPT_IS_EMPTY) ? _T("is empty") : _T("not found"));
      return;
   }

   vm->setUserData(this);
   if (!vm->run())
   {
      nxlog_debug_tag(DEBUG_TAG_OBJECT_LIFECYCLE, 4, _T("NetObj::executeHookScript(%s [%u]): hook script \"%s\" execution error: %s"),
                m_name, m_id, scriptName, vm->getErrorText());
      ReportScriptError(SCRIPT_CONTEXT_OBJECT, this, 0, vm->getErrorText(), scriptName);
      sendPollerMsg(POLLER_ERROR _T("Runtime error in hook script %s (%s)\r\n"), scriptName, vm->getErrorText());
   }
   vm.destroy();
}

/**
 * Callback for serializing custom attributes to JSON document
 */
EnumerationCallbackResult CustomAttributeToJson(const TCHAR *key, const CustomAttribute *attr, json_t *customAttributes)
{
   if(attr->sourceObject != 0 && (attr->flags & CAF_REDEFINED) == 0)
      return _CONTINUE;

   json_array_append_new(customAttributes, attr->toJson(key));
   return _CONTINUE;
}

/**
 * Serialize object to JSON
 */
json_t *NetObj::toJson()
{
   json_t *root = json_object();

   lockProperties();
   json_object_set_new(root, "id", json_integer(m_id));
   json_object_set_new(root, "guid", m_guid.toJson());
   json_object_set_new(root, "timestamp", json_integer(m_timestamp));
   json_object_set_new(root, "name", json_string_t(m_name));
   json_object_set_new(root, "alias", json_string_t(m_alias));
   json_object_set_new(root, "commentsSource", json_string_t(m_commentsSource));
   json_object_set_new(root, "comments", json_string_t(m_comments));
   json_object_set_new(root, "status", json_integer(m_status));
   json_object_set_new(root, "statusCalcAlg", json_integer(m_statusCalcAlg));
   json_object_set_new(root, "statusPropAlg", json_integer(m_statusPropAlg));
   json_object_set_new(root, "fixedStatus", json_integer(m_fixedStatus));
   json_object_set_new(root, "statusShift", json_integer(m_statusShift));
   json_object_set_new(root, "statusTranslation", json_integer_array(m_statusTranslation, 4));
   json_object_set_new(root, "statusSingleThreshold", json_integer(m_statusSingleThreshold));
   json_object_set_new(root, "statusThresholds", json_integer_array(m_statusThresholds, 4));
   json_object_set_new(root, "isSystem", json_boolean(m_isSystem));
   json_object_set_new(root, "maintenanceEventId", json_integer(m_maintenanceEventId));
   json_object_set_new(root, "maintenanceInitiator", json_integer(m_maintenanceInitiator));
   json_object_set_new(root, "mapImage", m_mapImage.toJson());
   json_object_set_new(root, "geoLocation", m_geoLocation.toJson());
   json_object_set_new(root, "postalAddress", m_postalAddress.toJson());
   json_object_set_new(root, "drilldownObjectId", json_integer(m_drilldownObjectId));
   json_object_set_new(root, "dashboards", m_dashboards.toJson());
   json_object_set_new(root, "urls", json_object_array(m_urls));
   json_object_set_new(root, "accessList", m_accessList.toJson());
   json_object_set_new(root, "inheritAccessRights", json_boolean(m_inheritAccessRights));
   json_object_set_new(root, "trustedObjects", (m_trustedObjects != nullptr) ? m_trustedObjects->toJson() : json_array());
   json_object_set_new(root, "creationTime", json_integer(m_creationTime));
   json_object_set_new(root, "categoryId", json_integer(m_categoryId));

   json_t *customAttributes = json_array();
   forEachCustomAttribute(CustomAttributeToJson, customAttributes);
   json_object_set_new(root, "customAttributes", customAttributes);
   unlockProperties();

   json_t *children = json_array();
   readLockChildList();
   for(int i = 0; i < getChildList().size(); i++)
      json_array_append_new(children, json_integer(getChildList().get(i)->getId()));
   unlockChildList();
   json_object_set_new(root, "children", children);

   json_t *parents = json_array();
   readLockParentList();
   for(int i = 0; i < getParentList().size(); i++)
      json_array_append_new(parents, json_integer(getParentList().get(i)->getId()));
   unlockParentList();
   json_object_set_new(root, "parents", parents);

   json_t *responsibleUsers = json_array();
   lockResponsibleUsersList();
   if (m_responsibleUsers != nullptr)
   {
      for(int i = 0; i < m_responsibleUsers->size(); i++)
      {
         ResponsibleUser *r = m_responsibleUsers->get(i);
         json_t *jr = json_object();
         json_object_set_new(jr, "id", json_integer(r->userId));
         json_object_set_new(jr, "tag", json_string_t(r->tag));
         json_array_append_new(responsibleUsers, jr);
      }
   }
   unlockResponsibleUsersList();
   json_object_set_new(root, "responsibleUsers", responsibleUsers);

   return root;
}

/**
 * Expand text
 */
StringBuffer NetObj::expandText(const TCHAR *textTemplate, const Alarm *alarm, const Event *event, const shared_ptr<DCObjectInfo>& dci,
         const TCHAR *userName, const TCHAR *objectName, const TCHAR *instance, const StringMap *inputFields, const StringList *args)
{
   if (textTemplate == nullptr)
      return StringBuffer();

   TCHAR buffer[256];
   int i;

   nxlog_debug_tag(_T("obj.macro"), 7, _T("NetObj::expandText(sourceObject=%u template='%s' alarm=%u event=") UINT64_FMT _T(" instance='%s')"),
             m_id, CHECK_NULL(textTemplate), (alarm == nullptr) ? 0 : alarm->getAlarmId() , (event == nullptr) ? 0 : event->getId(),
             CHECK_NULL(instance));

   StringBuffer output;
   for(const TCHAR *curr = textTemplate; *curr != 0; curr++)
   {
      switch(*curr)
      {
         case '%':   // Metacharacter
            curr++;
            if (*curr == 0)
            {
               curr--;
               break;   // Abnormal loop termination
            }
            switch(*curr)
            {
               case '%':
                  output.append(_T('%'));
                  break;
               case 'a':   // IP address of event source
                  output.append(getPrimaryIpAddress().toString(buffer));
                  break;
               case 'A':   // Associated alarm message
                  if (alarm != nullptr)
                  {
                     output.append(alarm->getMessage());
                  }
                  else if (event != nullptr)
                  {
                     output.append(event->getLastAlarmMessage());
                  }
                  break;
               case 'c':   // Event code
                  output.append((event != nullptr) ? event->getCode() : 0);
                  break;
               case 'E':   // Concatenated event tags
                  if (event != nullptr)
                  {
                     event->getTagsAsList(&output);
                  }
                  break;
               case 'g':   // Source object's GUID
                  output.append(m_guid);
                  break;
               case 'i':   // Source object identifier in form 0xhhhhhhhh
                  output.append(m_id, _T("0x%08X"));
                  break;
               case 'I':   // Source object identifier in decimal form
                  output.append(m_id);
                  break;
               case 'K':   // Associated alarm key
                  if (alarm != nullptr)
                  {
                     output.append(alarm->getKey());
                  }
                  else if (event != nullptr)
                  {
                     output.append(event->getLastAlarmKey());
                  }
                  break;
               case 'm':
                  if (event != nullptr)
                  {
                     output.append(event->getMessage());
                  }
                  break;
               case 'M':   // Custom message (usually set by matching script in EPP)
                  if (event != nullptr)
                  {
                     output.append(event->getCustomMessage());
                  }
                  break;
               case 'n':   // Name of event source
                  output.append((objectName != NULL) ? objectName : getName());
                  break;
               case 'N':   // Event name
                  if (event != nullptr)
                  {
                     output.append(event->getName());
                  }
                  break;
               case 's':   // Severity code
                  if (event != nullptr)
                  {
                     output.append(static_cast<int32_t>(event->getSeverity()));
                  }
                  break;
               case 'S':   // Severity text
                  if (event != nullptr)
                  {
                     output.append(GetStatusAsText(event->getSeverity(), false));
                  }
                  break;
               case 't':   // Event's timestamp
                  output.append(FormatTimestamp((event != nullptr) ? event->getTimestamp() : time(nullptr), buffer));
                  break;
               case 'T':   // Event's timestamp as number of seconds since epoch
                  output.append(static_cast<int64_t>((event != nullptr) ? event->getTimestamp() : time(nullptr)));
                  break;
               case 'u':   // IP address in URL compatible form ([addr] for IPv6, addr for IPv4)
                  if (getPrimaryIpAddress().getFamily() == AF_INET6)
                  {
                     output.append(_T('['));
                     output.append(getPrimaryIpAddress().toString(buffer));
                     output.append(_T(']'));
                  }
                  else
                  {
                     output.append(getPrimaryIpAddress().toString(buffer));
                  }
                  break;
               case 'U':   // User name
                  output.append(userName);
                  break;
               case 'v':   // NetXMS server version
                  output.append(NETXMS_VERSION_STRING);
                  break;
               case 'y': // alarm state
                  if (alarm != nullptr)
                  {
                     output.append(static_cast<int32_t>(alarm->getState()));
                  }
                  break;
               case 'Y': // alarm ID
                  if (alarm != nullptr)
                  {
                     output.append(alarm->getAlarmId());
                  }
                  break;
               case 'z':   // Zone UIN
                  output.append(getZoneUIN());
                  break;
               case 'Z':   // Zone name
                  if (IsZoningEnabled())
                  {
                     shared_ptr<Zone> zone = FindZoneByUIN(getZoneUIN());
                     if (zone != nullptr)
                     {
                        output.append(zone->getName());
                     }
                     else
                     {
                        output.append(_T('['));
                        output.append(getZoneUIN());
                        output.append(_T(']'));
                     }
                  }
                  break;
               case '0':
               case '1':
               case '2':
               case '3':
               case '4':
               case '5':
               case '6':
               case '7':
               case '8':
               case '9':
                  buffer[0] = *curr;
                  if (isdigit(*(curr + 1)))
                  {
                     curr++;
                     buffer[1] = *curr;
                     buffer[2] = 0;
                  }
                  else
                  {
                     buffer[1] = 0;
                  }
                  if (event != nullptr)
                     output.append(event->getParameterList()->get(_tcstol(buffer, nullptr, 10) - 1));
                  else if (args != nullptr)
                     output.append(args->get(_tcstol(buffer, nullptr, 10) - 1));
                  break;
               case '[':   // Script
                  for(i = 0, curr++; (*curr != ']') && (*curr != 0) && (i < 255); curr++)
                  {
                     buffer[i++] = *curr;
                  }
                  if (*curr == 0)  // no terminating ]
                  {
                     curr--;
                  }
                  else
                  {
                     buffer[i] = 0;
                     expandScriptMacro(buffer, alarm, event, dci, &output);
                  }
                  break;
               case '{':   // Custom attribute
                  for(i = 0, curr++; (*curr != '}') && (*curr != 0) && (i < 255); curr++)
                  {
                     buffer[i++] = *curr;
                  }
                  if (*curr == 0)  // no terminating }
                  {
                     curr--;
                  }
                  else
                  {
                     buffer[i] = 0;
                     TCHAR *defaultValue = _tcschr(buffer, _T(':'));
                     if (defaultValue != nullptr)
                     {
                        *defaultValue = 0;
                        defaultValue++;
                     }
                     Trim(buffer);
                     TCHAR *v = nullptr;
                     if (instance != nullptr)
                     {
                        TCHAR tmp[128];
                        _sntprintf(tmp, 128, _T("%s::%s"), buffer, instance);
                        v = getCustomAttributeCopy(tmp);
                     }
                     else if (event != nullptr)
                     {
                        const StringList *names = event->getParameterNames();
                        int index = names->indexOfIgnoreCase(_T("instance"));
                        if (index != -1)
                        {
                           TCHAR tmp[128];
                           _sntprintf(tmp, 128, _T("%s::%s"), buffer, event->getParameter(index));
                           v = getCustomAttributeCopy(tmp);
                        }
                     }
                     if (v == nullptr)
                        v = getCustomAttributeCopy(buffer);
                     if (v != nullptr)
                        output.appendPreallocated(v);
                     else if (defaultValue != nullptr)
                        output.append(defaultValue);
                  }
                  break;
               case '(':   // Input field
                  for(i = 0, curr++; (*curr != ')') && (*curr != 0) && (i < 255); curr++)
                  {
                     buffer[i++] = *curr;
                  }
                  if (*curr == 0)  // no terminating )
                  {
                     curr--;
                  }
                  else if (inputFields != nullptr)
                  {
                     buffer[i] = 0;
                     Trim(buffer);
                     output.append(inputFields->get(buffer));
                  }
                  break;
               case '<':   // Named parameter
                  if (event != nullptr)
                  {
                     for(i = 0, curr++; (*curr != '>') && (*curr != 0) && (i < 255); curr++)
                     {
                        buffer[i++] = *curr;
                     }
                     if (*curr == 0)  // no terminating >
                     {
                        curr--;
                     }
                     else
                     {
                        buffer[i] = 0;
                        TCHAR *modifierEnd = _tcschr(buffer, _T('}'));
                        StringList *list = nullptr;
                        if (buffer[0] == '{' && modifierEnd != nullptr)
                        {
                           *modifierEnd = 0;
                           list = String::split(buffer + 1, _tcslen(buffer + 1), _T(","), true);
                           memmove(buffer, modifierEnd + 1, sizeof(TCHAR) * (_tcslen(modifierEnd + 1) + 1));
                        }
                        const TCHAR *defaultValue = _T("");
                        TCHAR *tmp = _tcschr(buffer, _T(':'));
                        if (tmp != nullptr)
                        {
                           *tmp = 0;
                           defaultValue = tmp + 1;
                        }
                        Trim(buffer);

                        const StringList *names = event->getParameterNames();
                        shared_ptr<DCObjectInfo> formatDci = dci; // DCI used for formatting value
                        if (formatDci == nullptr && event->getDciId() != 0 && isDataCollectionTarget())
                        {
                           formatDci = static_cast<DataCollectionTarget*>(this)->getDCObjectById(event->getDciId(), 0)->createDescriptor();
                        }
                        if (formatDci != nullptr && list != nullptr)
                        {
                           output.append(formatDci->formatValue(event->getParameter(names->indexOfIgnoreCase(buffer), defaultValue), list));
                        }
                        else
                        {
                           output.append(event->getParameter(names->indexOfIgnoreCase(buffer), defaultValue));
                        }

                        delete list;
                     }
                  }
                  break;
               default:    // All other characters are invalid, ignore
                  break;
            }
            break;
         case '\\':  // Escape character
            curr++;
            if (*curr == 0)
            {
               curr--;
               break;   // Abnormal loop termination
            }
            switch(*curr)
            {
               case 't':
                  output.append(_T('\t'));
                  break;
               case 'n':
                  output.append(_T("\r\n"));
                  break;
               default:
                  output.append(*curr);
                  break;
            }
            break;
         default:
            output.append(*curr);
            break;
      }
   }
   return output;
}

/**
 * Expand script macro (intended to be called from expandText)
 */
void NetObj::expandScriptMacro(TCHAR *name, const Alarm *alarm, const Event *event, const shared_ptr<DCObjectInfo>& dci, StringBuffer *output)
{
   // Arguments can be provided as script_name(arg1, arg2, ... argN)
   TCHAR *p = _tcschr(name, _T('('));
   if (p != nullptr)
   {
      size_t l = _tcslen(name) - 1;
      if (name[l] != _T(')'))
      {
         nxlog_debug_tag(_T("obj.macro"), 6, _T("NetObj::ExpandText(%s [%u], %s): missing closing parenthesis in macro \"%%[%s]\""), m_name, m_id, (event != nullptr) ? event->getName() : _T("<no event>"), name);
         return;
      }
      name[l] = 0;
      *p = 0;
      Trim(name);
   }

   // Entry point can be given in form script/entry_point
   char entryPoint[MAX_IDENTIFIER_LENGTH];
   TCHAR *s = _tcschr(name, _T('/'));
   if (s != nullptr)
   {
      *s = 0;
      s++;
      Trim(s);
#ifdef UNICODE
      wchar_to_utf8(s, -1, entryPoint, MAX_IDENTIFIER_LENGTH);
      entryPoint[MAX_IDENTIFIER_LENGTH - 1] = 0;
#else
      strlcpy(entryPoint, s, MAX_IDENTIFIER_LENGTH);
#endif
   }
   else
   {
      entryPoint[0] = 0;
   }
   Trim(name);

   NXSL_VM *vm = CreateServerScriptVM(name, self(),
      ((dci == nullptr) && (event != nullptr) && (event->getDciId() != 0) && isDataCollectionTarget()) ?
            static_cast<DataCollectionTarget*>(this)->getDCObjectById(event->getDciId(), 0)->createDescriptor() : dci);
   if (vm != nullptr)
   {
      if (event != nullptr)
         vm->setGlobalVariable("$event", vm->createValue(vm->createObject(&g_nxslEventClass, event, true)));
      if (alarm != nullptr)
      {
         vm->setGlobalVariable("$alarm", vm->createValue(vm->createObject(&g_nxslAlarmClass, alarm, true)));
         vm->setGlobalVariable("$alarmMessage", vm->createValue(alarm->getMessage()));
         vm->setGlobalVariable("$alarmKey", vm->createValue(alarm->getKey()));
      }

      ObjectRefArray<NXSL_Value> args(16, 16);
      if ((p == nullptr) || ParseValueList(vm, &p, args, true))
      {
         if (vm->run(args, nullptr, nullptr, nullptr, (entryPoint[0] != 0) ? entryPoint : nullptr))
         {
            NXSL_Value *result = vm->getResult();
            const TCHAR *temp = result->getValueAsCString();
            if (temp != nullptr)
            {
               output->append(temp);
               nxlog_debug_tag(_T("obj.macro"), 6, _T("NetObj::ExpandText(%s [%u], %s): Script \"%s\" executed successfully"), m_name, m_id, (event != nullptr) ? event->getName() : _T("<no event>"), name);
            }
         }
         else
         {
            nxlog_debug_tag(_T("obj.macro"), 6, _T("NetObj::ExpandText(%s [%u], %s): Script \"%s\" execution error (%s)"),
                  m_name, m_id, (event != nullptr) ? event->getName() : _T("<no event>"), name, vm->getErrorText());
            ReportScriptError(SCRIPT_CONTEXT_OBJECT, this, 0, vm->getErrorText(), name);
         }
      }
      else
      {
         if (p != nullptr)
            *p = _T('(');
         if (s != nullptr)
            *s = _T('/');
         nxlog_debug_tag(_T("obj.macro"), 6, _T("NetObj::ExpandText(%s [%u], %s): cannot parse argument list in macro \"%%[%s]\""), m_name, m_id, (event != nullptr) ? event->getName() : _T("<no event>"), name);
      }
      delete vm;
   }
   else
   {
      nxlog_debug_tag(_T("obj.macro"), 6, _T("NetObj::ExpandText(%s [%u], %s): Cannot find script \"%s\""), m_name, m_id, (event != nullptr) ? event->getName() : _T("<no event>"), name);
   }
}

/**
 * Internal function to get inherited list of responsible users for object
 */
void NetObj::getAllResponsibleUsersInternal(StructArray<ResponsibleUser> *list, const TCHAR *tag) const
{
   readLockParentList();
   for(int i = 0; i < getParentList().size(); i++)
   {
      NetObj *obj = getParentList().get(i);
      obj->lockResponsibleUsersList();
      if (obj->m_responsibleUsers != nullptr)
      {
         for(int n = 0; n < obj->m_responsibleUsers->size(); n++)
         {
            ResponsibleUser *r = obj->m_responsibleUsers->get(n);
            if ((tag != nullptr) && _tcscmp(r->tag, tag))
               continue;

            bool found = false;
            for(int m = 0; m < list->size(); m++)
            {
               if (list->get(m)->userId == r->userId)
               {
                  found = true;
                  break;
               }
            }
            if (!found)
               list->add(r);
         }
      }
      obj->unlockResponsibleUsersList();
      getParentList().get(i)->getAllResponsibleUsersInternal(list, tag);
   }
   unlockParentList();
}

/**
 * Get all responsible users for object
 */
unique_ptr<StructArray<ResponsibleUser>> NetObj::getAllResponsibleUsers(const TCHAR *tag) const
{
   lockResponsibleUsersList();
   auto responsibleUsers = (m_responsibleUsers != nullptr) ?
            ((tag == nullptr) ? new StructArray<ResponsibleUser>(*m_responsibleUsers) : new StructArray<ResponsibleUser>(m_responsibleUsers->size(), 16)) : new StructArray<ResponsibleUser>(0, 16);
   if ((tag != nullptr) && (m_responsibleUsers != nullptr))
   {
      for(int i = 0; i < m_responsibleUsers->size(); i++)
      {
         ResponsibleUser *r = m_responsibleUsers->get(i);
         if (!_tcscmp(r->tag, tag))
            responsibleUsers->add(r);
      }
   }
   unlockResponsibleUsersList();

   getAllResponsibleUsersInternal(responsibleUsers, tag);
   return unique_ptr<StructArray<ResponsibleUser>>(responsibleUsers);
}

/**
 * Get values for virtual attributes
 */
bool NetObj::getObjectAttribute(const TCHAR *name, TCHAR **value, bool *isAllocated) const
{
   ENUMERATE_MODULES(pfGetObjectAttribute)
   {
      if (CURRENT_MODULE.pfGetObjectAttribute(*this, name, value, isAllocated))
         return true;
   }
   return false;
}

/**
 * Add dashboard to associated dashboard list
 */
bool NetObj::addDashboard(uint32_t id)
{
   bool added = false;
   lockProperties();
   if (!m_dashboards.contains(id))
   {
      m_dashboards.add(id);
      setModified(MODIFY_DASHBOARD_LIST);
      added = true;
   }
   unlockProperties();
   return added;
}

/**
 * Remove dashboard from associated dashboard list
 */
bool NetObj::removeDashboard(uint32_t id)
{
   lockProperties();
   int index = m_dashboards.indexOf(id);
   if (index != -1)
   {
      m_dashboards.remove(index);
      setModified(MODIFY_DASHBOARD_LIST);
   }
   unlockProperties();
   return index != -1;
}
