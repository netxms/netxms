/*
** nxdbmgr - NetXMS database manager
** Copyright (C) 2004-2026 Victor Kirhenshtein
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
** File: check.cpp
**
**/

#include "nxdbmgr.h"
#include <dci_table_creation.h>
#include <nxevent.h>
#include <algorithm>
#include <map>
#include <vector>

/**
 * Set when a statement issued by a set-based check stage fails. No further stage starts after that
 * and the whole check is rolled back: on PostgreSQL a failed statement poisons the transaction, so
 * every later query would fail too and a "no errors" result would be false.
 */
static bool s_checkAborted = false;

/**
 * SQLSelect for set-based check stages: a failure aborts the check
 */
static DB_RESULT CheckSelect(const wchar_t *query)
{
   DB_RESULT hResult = SQLSelect(query);
   if (hResult == nullptr)
      s_checkAborted = true;
   return hResult;
}

/**
 * SQLQuery for set-based check stages: a failure aborts the check
 */
static bool CheckQuery(const wchar_t *query)
{
   if (SQLQuery(query))
      return true;
   s_checkAborted = true;
   return false;
}

/**
 * Check data tables for given object class
 */
static void CollectObjectIdentifiers(const TCHAR *className, IntegerArray<uint32_t> *list)
{
   TCHAR query[1024];
   _sntprintf(query, 1024, _T("SELECT id FROM %s"), className);
   DB_RESULT hResult = SQLSelect(query);
   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         list->add(DBGetFieldULong(hResult, i, 0));
      }
      DBFreeResult(hResult);
   }
}

/**
 * Check if container object exists
 */
static bool IsContainerObjectExists(DB_HANDLE hdb, uint32_t id, int32_t objectClass)
{
   bool exists = false;
   DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT id FROM object_containers WHERE id=? AND object_class=?"));
   if (hStmt != nullptr)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, id);
      DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, objectClass);
      DB_RESULT hResult = DBSelectPrepared(hStmt);
      if (hResult != nullptr)
      {
         exists = (DBGetNumRows(hResult) > 0);
         DBFreeResult(hResult);
      }
      DBFreeStatement(hStmt);
   }
   return exists;
}

/**
 * Get all data collection targets
 */
IntegerArray<uint32_t> GetDataCollectionTargets()
{
   IntegerArray<uint32_t> list(128, 128);
   CollectObjectIdentifiers(L"nodes", &list);
   CollectObjectIdentifiers(L"clusters", &list);
   CollectObjectIdentifiers(L"mobile_devices", &list);
   CollectObjectIdentifiers(L"access_points", &list);
   CollectObjectIdentifiers(L"chassis", &list);
   CollectObjectIdentifiers(L"racks", &list);
   CollectObjectIdentifiers(L"sensors", &list);
   CollectObjectIdentifiers(L"resources", &list);
   CollectObjectIdentifiers(L"cloud_domains", &list);
   CollectObjectIdentifiers(L"traffic_observers", &list);
   CollectObjectIdentifiers(L"observation_points", &list);
   CollectObjectIdentifiers(L"facilities", &list);
   CollectObjectIdentifiers(L"power_domains", &list);
   CollectObjectIdentifiers(L"cooling_zones", &list);
   CollectObjectIdentifiers(L"rooms", &list);
   CollectObjectIdentifiers(L"object_containers WHERE object_class=29 OR object_class=30", &list);   // objects of class "collector" or "circuit"
   return list;
}

/**
 * Check business service checks
 */
static void CheckBusinessServiceCheckBindings()
{
   StartStage(_T("Business service checks - service bindings"));
   DB_RESULT hResult = SQLSelect(_T("SELECT c.id, c.service_id FROM business_service_checks c ")
                           _T("LEFT OUTER JOIN business_services s ON s.id = c.service_id ")
                           _T("LEFT OUTER JOIN business_service_prototypes p ON p.id = c.service_id ")
                           _T("WHERE s.id IS NULL AND p.id IS NULL"));
   if (hResult != nullptr)
   {
      int numChecks = DBGetNumRows(hResult);
      SetStageWorkTotal(numChecks);
      for(int i = 0; i < numChecks; i++)
      {
         g_dbCheckErrors++;
         uint32_t checkId = DBGetFieldULong(hResult, i, 0);
         if (GetYesNoEx(_T("Business service check %u refers to non-existing business service %u. Fix it?"),
                        checkId, DBGetFieldULong(hResult, i, 1)))
         {
            TCHAR query[1024];
            _sntprintf(query, 1024, _T("DELETE FROM business_service_checks WHERE id=%u"), checkId);
            if (SQLQuery(query))
               g_dbCheckFixes++;
         }
         UpdateStageProgress(1);
      }
      DBFreeResult(hResult);
   }
   EndStage();
}

/**
 * Check business service tickets
 */
static void CheckBusinessServiceTicketServiceBindings()
{
   StartStage(_T("Business service tickets - service bindings"));

   IntegerArray<uint32_t> businessServices(64, 64);
   CollectObjectIdentifiers(_T("business_services"), &businessServices);

   DB_RESULT hResult = SQLSelect(_T("SELECT ticket_id,service_id,original_service_id,close_timestamp FROM business_service_tickets"));
   if (hResult != nullptr)
   {
      int numTickets = DBGetNumRows(hResult);
      SetStageWorkTotal(numTickets);
      for(int i = 0; i < numTickets; i++)
      {
         uint32_t serviceId = DBGetFieldULong(hResult, i, 1);
         if (!businessServices.contains(serviceId))
         {
            g_dbCheckErrors++;
            uint32_t ticketId = DBGetFieldULong(hResult, i, 0);
            if (GetYesNoEx(_T("Business service ticket %u refers to non-existing business service %u. Fix it?"), ticketId, serviceId))
            {
               if (DBMgrExecuteQueryOnObject(ticketId, _T("DELETE FROM business_service_tickets WHERE ticket_id=?")))
                  g_dbCheckFixes++;
            }
         }
         else
         {
            uint32_t originalServiceId = DBGetFieldULong(hResult, i, 2);
            if (originalServiceId != 0 && !businessServices.contains(originalServiceId) && (DBGetFieldULong(hResult, i, 3) == 0))
            {
               g_dbCheckErrors++;
               uint32_t ticketId = DBGetFieldULong(hResult, i, 0);
               if (GetYesNoEx(_T("Business service ticket %u refers to non-existing business service %u. Fix it?"), ticketId, serviceId))
               {
                  TCHAR query[256];
                  _sntprintf(query, 256, _T("UPDATE business_service_tickets SET close_timestamp=%u WHERE ticket_id=%u"), static_cast<uint32_t>(time(nullptr)), ticketId);
                  if (SQLQuery(query))
                     g_dbCheckFixes++;
               }
            }
         }
         UpdateStageProgress(1);
      }
      DBFreeResult(hResult);
   }
   EndStage();
}

/**
 * Check business service tickets
 */
static void CheckBusinessServiceTicketCheckBindings()
{
   StartStage(_T("Business service tickets - check bindings"));

   IntegerArray<uint32_t> businessServiceChecks(64, 64);
   CollectObjectIdentifiers(_T("business_service_checks"), &businessServiceChecks);

   DB_RESULT hResult = SQLSelect(_T("SELECT ticket_id,check_id FROM business_service_tickets"));
   if (hResult != nullptr)
   {
      int numTickets = DBGetNumRows(hResult);
      SetStageWorkTotal(numTickets);
      for(int i = 0; i < numTickets; i++)
      {
         uint32_t checkId = DBGetFieldULong(hResult, i, 1);
         if (!businessServiceChecks.contains(checkId))
         {
            g_dbCheckErrors++;
            uint32_t ticketId = DBGetFieldULong(hResult, i, 0);
            if (GetYesNoEx(_T("Business service ticket %u refers to non-existing business service check %u. Fix it?"), ticketId, checkId))
            {
               if (DBMgrExecuteQueryOnObject(ticketId, _T("DELETE FROM business_service_tickets WHERE ticket_id=?")))
                  g_dbCheckFixes++;
            }
         }
         UpdateStageProgress(1);
      }
      DBFreeResult(hResult);
   }
   EndStage();
}

/**
 * Check business service tickets
 */
static void CheckBusinessServiceTicketHierarchy()
{
   StartStage(_T("Business service tickets - hierarchy"));

   DB_RESULT hResult = SQLSelect(_T("SELECT ticket_id,original_ticket_id,close_timestamp FROM business_service_tickets"));
   if (hResult != nullptr)
   {
      int numTickets = DBGetNumRows(hResult);
      SetStageWorkTotal(numTickets);
      for(int i = 0; i < numTickets; i++)
      {
         uint32_t originalTicketId = DBGetFieldULong(hResult, i, 1);
         if (originalTicketId == 0)
         {
            UpdateStageProgress(1);
            continue;
         }

         TCHAR query[512];
         _sntprintf(query, 512, _T("SELECT close_timestamp FROM business_service_tickets WHERE ticket_id=%u"), originalTicketId);
         DB_RESULT originalTicketResult = SQLSelect(query);
         if (originalTicketResult != nullptr)
         {
            if (DBGetNumRows(originalTicketResult) == 0)
            {
               g_dbCheckErrors++;
               uint32_t ticketId = DBGetFieldULong(hResult, i, 0);
               if (GetYesNoEx(_T("Business service ticket %u refers to non-existing parent business service ticket %u as original ticket. Fix it?"), ticketId, originalTicketId))
               {
                  if (DBMgrExecuteQueryOnObject(ticketId, _T("DELETE FROM business_service_tickets WHERE ticket_id=?")))
                     g_dbCheckFixes++;
               }
            }
            else if (DBGetFieldULong(hResult, i, 2) == 0 && DBGetFieldULong(originalTicketResult, 0, 0) != 0)
            {
               g_dbCheckErrors++;
               uint32_t ticketId = DBGetFieldULong(hResult, i, 0);
               if (GetYesNoEx(_T("Opened business service ticket %u refers to closed parent business service ticket %u. Fix it?"), ticketId, originalTicketId))
               {
                  _sntprintf(query, 512, _T("UPDATE business_service_tickets SET close_timestamp=%d WHERE ticket_id=%d"), DBGetFieldULong(originalTicketResult, 0, 0), ticketId);
                  if (SQLQuery(query))
                     g_dbCheckFixes++;
               }
            }

            DBFreeResult(originalTicketResult);
         }
         UpdateStageProgress(1);
      }
      DBFreeResult(hResult);
   }
   EndStage();
}

/**
 * Update business service check status
 */
static void CheckBusinessServiceCheckState()
{
   StartStage(_T("Business service checks - state"));

   DB_RESULT ticketResult = SQLSelect(_T("SELECT ticket_id FROM business_service_tickets WHERE close_timestamp<>0"));
   if (ticketResult != nullptr)
   {
      int numTickets = DBGetNumRows(ticketResult);
      SetStageWorkTotal(numTickets);
      for(int i = 0; i < numTickets; i++)
      {
         uint32_t ticketId = DBGetFieldULong(ticketResult, i, 0);
         TCHAR query[512];
         _sntprintf(query, 512, _T("SELECT id FROM business_service_checks WHERE current_ticket=%u"), ticketId);
         DB_RESULT checkResult = SQLSelect(query);
         if (checkResult != nullptr)
         {
            if (DBGetNumRows(checkResult) > 0)
            {
               g_dbCheckErrors++;
               uint32_t checkId = DBGetFieldULong(checkResult, 0, 0);
               if (GetYesNoEx(_T("Business service check %u refers to closed business service ticket %u. Fix it?"), checkId, ticketId))
               {
                  if (DBMgrExecuteQueryOnObject(checkId, _T("UPDATE business_service_checks SET current_ticket=0,status=0 WHERE id=?")))
                     g_dbCheckFixes++;
               }
            }
            DBFreeResult(checkResult);
         }
         UpdateStageProgress(1);
      }
      DBFreeResult(ticketResult);
   }
   EndStage();
}

/**
 * Check business service downtime
 */
static void CheckBusinessServiceDowntime()
{
   StartStage(_T("Business service downtime"));
   DB_RESULT businessServices = SQLSelect(_T("SELECT id FROM business_services s INNER JOIN business_service_downtime d ON s.id=d.service_id WHERE to_timestamp=0"));
   if (businessServices != nullptr)
   {
      int numServices = DBGetNumRows(businessServices);

      TCHAR query[1024];
      SetStageWorkTotal(numServices);
      for(int i = 0; i < numServices; i++)
      {
         uint32_t serviceId = DBGetFieldULong(businessServices, i, 0);
         _sntprintf(query, 1024, _T("SELECT count(*) FROM business_service_tickets WHERE service_id=%u AND close_timestamp=0"), serviceId);
         DB_RESULT result = SQLSelect(query);
         if (result != nullptr)
         {
            if (DBGetFieldLong(result, 0, 0) == 0)
            {
               g_dbCheckErrors++;
               if (GetYesNoEx(_T("Business service %u has no opened tickets and active downtime. Fix it?"), serviceId))
               {
                  _sntprintf(query, 1024, _T("UPDATE business_service_downtime SET to_timestamp=%u WHERE service_id=%u AND to_timestamp=0"),
                              static_cast<uint32_t>(time(nullptr)), serviceId);
                  if (SQLQuery(query))
                     g_dbCheckFixes++;
               }
            }
            DBFreeResult(result);
         }
         UpdateStageProgress(1);
      }
      DBFreeResult(businessServices);
   }
   EndStage();
}

/**
 * Check that given node is inside at least one container or cluster
 */
static bool NodeInContainer(uint32_t id)
{
   bool result = false;

   TCHAR query[256];
   _sntprintf(query, 256, _T("SELECT container_id FROM container_members WHERE object_id=%d"), id);
   DB_RESULT hResult = SQLSelect(query);
   if (hResult != nullptr)
   {
      result = (DBGetNumRows(hResult) > 0);
      DBFreeResult(hResult);
   }

	if (!result)
	{
		_sntprintf(query, 256, _T("SELECT cluster_id FROM cluster_members WHERE node_id=%d"), id);
		hResult = SQLSelect(query);
		if (hResult != NULL)
		{
			result = (DBGetNumRows(hResult) > 0);
			DBFreeResult(hResult);
		}
	}

   return result;
}

/**
 * Find subnet for unlinked node
 */
static bool FindSubnetForNode(uint32_t id, const TCHAR *name)
{
	bool success = false;

	// Read list of interfaces of given node
   TCHAR query[256];
	_sntprintf(query, 256, _T("SELECT l.ip_addr,l.ip_netmask FROM interfaces i INNER JOIN interface_address_list l ON l.iface_id = i.id WHERE node_id=%u"), id);
	DB_RESULT hResult = SQLSelect(query);
	if (hResult != nullptr)
	{
		int count = DBGetNumRows(hResult);
		for(int i = 0; i < count; i++)
		{
			InetAddress addr = DBGetFieldInetAddr(hResult, i, 0);
         addr.setMaskBits(DBGetFieldLong(hResult, i, 1));
         InetAddress subnet = addr.getSubnetAddress();

         TCHAR buffer[32];
         _sntprintf(query, 256, _T("SELECT id FROM subnets WHERE ip_addr='%s'"), subnet.toString(buffer));
			DB_RESULT hResult2 = SQLSelect(query);
			if (hResult2 != nullptr)
			{
				if (DBGetNumRows(hResult2) > 0)
				{
					uint32_t subnetId = DBGetFieldULong(hResult2, 0, 0);
					g_dbCheckErrors++;
					if (GetYesNoEx(_T("Unlinked node object %d (\"%s\") can be linked to subnet %d (%s). Link?"), id, name, subnetId, buffer))
					{
						_sntprintf(query, 256, _T("INSERT INTO nsmap (subnet_id,node_id) VALUES (%d,%d)"), subnetId, id);
						if (SQLQuery(query))
						{
							success = true;
							g_dbCheckFixes++;
			            DBFreeResult(hResult2);
							break;
						}
						else
						{
							// Node remains unlinked, so error count will be
							// incremented again by node deletion code or next iteration
							g_dbCheckErrors--;
						}
					}
					else
					{
						// Node remains unlinked, so error count will be
						// incremented again by node deletion code
						g_dbCheckErrors--;
					}
				}
				DBFreeResult(hResult2);
			}
		}
		DBFreeResult(hResult);
	}
	return success;
}

/**
 * Check missing object properties
 */
static void CheckMissingObjectProperties(const wchar_t *table, const wchar_t *className, uint32_t builtinObjectId)
{
   wchar_t query[1024];
   nx_swprintf(query, 1024, L"SELECT o.id FROM %s o LEFT OUTER JOIN object_properties p ON p.object_id = o.id WHERE p.name IS NULL", table);
   DB_RESULT hResult = CheckSelect(query);
   if (hResult == nullptr)
      return;

   int count = DBGetNumRows(hResult);
   for(int i = 0; (i < count) && !s_checkAborted; i++)
   {
      uint32_t id = DBGetFieldULong(hResult, i, 0);
      if (id == builtinObjectId)
         continue;
      g_dbCheckErrors++;
      if (GetYesNoEx(_T("Missing %s object %d properties. Create?"), className, id))
      {
         uuid_t guid;
         _uuid_generate(guid);

         TCHAR guidText[128];
         _sntprintf(query, 1024,
               _T("INSERT INTO object_properties (object_id,guid,name,")
               _T("status,is_deleted,is_hidden,inherit_access_rights,")
               _T("last_modified,status_calc_alg,status_prop_alg,")
               _T("status_fixed_val,status_shift,status_translation,")
               _T("status_single_threshold,status_thresholds,location_type,")
               _T("latitude,longitude,location_accuracy,location_timestamp,")
               _T("map_image,drilldown_object_id,state_before_maint,maint_event_id,flags,")
               _T("state,category,creation_time,maint_initiator,asset_id) VALUES ")
               _T("(%u,'%s','lost_%s_%u',5,0,0,1,") TIME_T_FMT _T(",0,0,0,0,0,0,'00000000',0,")
               _T("'0.000000','0.000000',0,0,'00000000-0000-0000-0000-000000000000',0,'0',0,0,")
               _T("0,0,") TIME_T_FMT _T(",0,0)"),
               id, _uuid_to_string(guid, guidText), className, id,
               TIME_T_FCAST(time(nullptr)), TIME_T_FCAST(time(nullptr)));
         if (CheckQuery(query))
            g_dbCheckFixes++;
      }
   }
   DBFreeResult(hResult);
}

/**
 * Object class table of the loader (objects.cpp)
 */
struct ObjectClassTable
{
   const wchar_t *table;
   const wchar_t *className;
   uint32_t builtinId;     // built-in object of this class that has no properties row, 0 if none
};

/**
 * Class tables in loader order. Business services and prototypes are loaded from object_containers
 * (classes 28 and 15), their own tables are extensions covered by s_containerExtensionTables.
 */
static const ObjectClassTable s_objectClassTables[] =
{
   { L"zones", L"zone", 4 },
   { L"conditions", L"condition", 0 },
   { L"subnets", L"subnet", 0 },
   { L"racks", L"rack", 0 },
   { L"chassis", L"chassis", 0 },
   { L"mobile_devices", L"mobile device", 0 },
   { L"sensors", L"sensor", 0 },
   { L"cloud_domains", L"cloud domain", 0 },
   { L"resources", L"resource", 0 },
   { L"traffic_observers", L"traffic observer", 0 },
   { L"observation_points", L"observation point", 0 },
   { L"nodes", L"node", 0 },
   { L"access_points", L"access point", 0 },
   { L"interfaces", L"interface", 0 },
   { L"network_services", L"network service", 0 },
   { L"vpn_connectors", L"VPN connector", 0 },
   { L"clusters", L"cluster", 0 },
   { L"facilities", L"facility", 0 },
   { L"power_domains", L"power domain", 0 },
   { L"cooling_zones", L"cooling zone", 0 },
   { L"rooms", L"room", 0 },
   { L"assets", L"asset", 0 },
   { L"templates", L"template", 0 },
   { L"network_maps", L"network map", 0 },
   { L"dashboards", L"dashboard", 0 },
   { L"dashboard_templates", L"dashboard template", 0 },
   { L"business_services", L"business service", 0 },
   { L"business_service_prototypes", L"business service prototype", 0 },
   { L"object_containers", L"container", 0 }
};

/**
 * Object classes stored in object_containers that also have a row in their own table
 */
static const struct
{
   int objectClass;
   const wchar_t *table;
} s_containerExtensionTables[] =
{
   { 15, L"business_service_prototypes" },
   { 23, L"dashboards" },
   { 24, L"dashboard_templates" },
   { 28, L"business_services" },
   { 32, L"racks" },
   { 42, L"facilities" },
   { 43, L"power_domains" },
   { 44, L"cooling_zones" },
   { 45, L"rooms" }
};

/**
 * Mirror of BUILTIN_OID_* from nms_objects.h, which nxdbmgr does not include. These objects have
 * properties rows but no class table row.
 */
static const uint32_t s_builtinObjectIds[] = { 1, 2, 3, 4, 5, 6, 7, 9 };

/**
 * Check if object ID belongs to a built-in object
 */
static bool IsBuiltinObjectId(uint32_t id)
{
   for(size_t i = 0; i < sizeof(s_builtinObjectIds) / sizeof(uint32_t); i++)
      if (s_builtinObjectIds[i] == id)
         return true;
   return false;
}

/**
 * Build "(SELECT id[,'table' AS t] FROM t1 UNION ALL ...)" over all class tables
 */
static StringBuffer BuildClassTableUnion(bool withTableName)
{
   StringBuffer sb(L"(");
   for(size_t i = 0; i < sizeof(s_objectClassTables) / sizeof(ObjectClassTable); i++)
   {
      if (i > 0)
         sb.append(L" UNION ALL ");
      sb.append(L"SELECT id");
      if (withTableName)
         sb.append(L",'").append(s_objectClassTables[i].table).append(L"' AS t");
      sb.append(L" FROM ").append(s_objectClassTables[i].table);
   }
   sb.append(L")");
   return sb;
}

/**
 * Every row of every class table must have an object_properties row
 */
static void CheckObjectClassCoverage()
{
   StartStage(L"Object class coverage", sizeof(s_objectClassTables) / sizeof(ObjectClassTable));
   for(size_t i = 0; (i < sizeof(s_objectClassTables) / sizeof(ObjectClassTable)) && !s_checkAborted; i++)
   {
      const ObjectClassTable& c = s_objectClassTables[i];
      CheckMissingObjectProperties(c.table, c.className, c.builtinId);
      UpdateStageProgress(1);
   }
   EndStage();
}

/**
 * Properties rows without a row in any class table. Report only: a module may own the object class.
 */
static void CheckGhostObjectProperties()
{
   StartStage(L"Ghost object properties");
   StringBuffer query(L"SELECT p.object_id,p.name FROM object_properties p LEFT OUTER JOIN ");
   query.append(BuildClassTableUnion(false)).append(L" u ON u.id=p.object_id WHERE u.id IS NULL ORDER BY p.object_id");
   DB_RESULT hResult = CheckSelect(query);
   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         uint32_t id = DBGetFieldULong(hResult, i, 0);
         if (IsBuiltinObjectId(id))
            continue;
         g_dbCheckErrors++;
         wchar_t name[MAX_OBJECT_NAME];
         WriteToTerminalEx(L"\nObject properties [%u] (\"%s\") have no object of any known class\n", id, DBGetField(hResult, i, 1, name, MAX_OBJECT_NAME));
      }
      DBFreeResult(hResult);
   }
   EndStage();
}

/**
 * Same ID in two class tables. An object_containers row next to the matching extension table is
 * one object, anything else is a duplicate. Report only.
 */
static void CheckDuplicateObjectIds()
{
   StartStage(L"Duplicate object IDs");
   StringBuffer query(L"SELECT u.id FROM ");
   query.append(BuildClassTableUnion(false)).append(L" u GROUP BY u.id HAVING count(*) > 1 ORDER BY u.id");
   DB_RESULT hResult = CheckSelect(query);
   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; (i < count) && !s_checkAborted; i++)
      {
         uint32_t id = DBGetFieldULong(hResult, i, 0);

         StringBuffer tableQuery(L"SELECT u.t FROM ");
         tableQuery.append(BuildClassTableUnion(true)).append(L" u WHERE u.id=").append(id);
         DB_RESULT hTables = CheckSelect(tableQuery);
         if (hTables == nullptr)
            break;

         StringList tables;
         for(int j = 0; j < DBGetNumRows(hTables); j++)
            tables.addPreallocated(DBGetField(hTables, j, 0, nullptr, 0));
         DBFreeResult(hTables);

         int containerClass = -1;
         if (tables.contains(L"object_containers"))
         {
            StringBuffer classQuery(L"SELECT object_class FROM object_containers WHERE id=");
            classQuery.append(id);
            DB_RESULT hClass = CheckSelect(classQuery);
            if (hClass == nullptr)
               break;
            if (DBGetNumRows(hClass) > 0)
               containerClass = DBGetFieldLong(hClass, 0, 0);
            DBFreeResult(hClass);
         }

         bool extension = false;
         if ((tables.size() == 2) && (containerClass != -1))
         {
            for(size_t j = 0; j < sizeof(s_containerExtensionTables) / sizeof(s_containerExtensionTables[0]); j++)
               if ((s_containerExtensionTables[j].objectClass == containerClass) && tables.contains(s_containerExtensionTables[j].table))
                  extension = true;
         }
         if (extension)
            continue;

         g_dbCheckErrors++;
         StringBuffer list;
         for(int j = 0; j < tables.size(); j++)
         {
            if (j > 0)
               list.append(L", ");
            list.append(tables.get(j));
            if ((containerClass != -1) && !wcscmp(tables.get(j), L"object_containers"))
               list.append(L" (class ").append(containerClass).append(L")");
         }
         WriteToTerminalEx(L"\nObject ID %u is used by %s\n", id, list.cstr());
      }
      DBFreeResult(hResult);
   }
   EndStage();
}

/**
 * Format the IDs returned by a single-column query as "a, b, c"
 */
static bool ListIds(const wchar_t *query, StringBuffer *list)
{
   DB_RESULT hResult = CheckSelect(query);
   if (hResult == nullptr)
      return false;
   int count = DBGetNumRows(hResult);
   for(int i = 0; i < count; i++)
   {
      if (i > 0)
         list->append(L", ");
      list->append(DBGetFieldULong(hResult, i, 0));
   }
   DBFreeResult(hResult);
   return true;
}

/**
 * Same GUID on more than one object. Report only.
 */
static void CheckDuplicateObjectGuids()
{
   StartStage(L"Duplicate object GUIDs");
   DB_RESULT hResult = CheckSelect(L"SELECT guid FROM object_properties GROUP BY guid HAVING count(*) > 1 ORDER BY guid");
   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; (i < count) && !s_checkAborted; i++)
      {
         wchar_t guid[64];
         DBGetField(hResult, i, 0, guid, 64);
         StringBuffer query(L"SELECT object_id FROM object_properties WHERE guid=");
         query.append(DBPrepareString(g_dbHandle, guid)).append(L" ORDER BY object_id");
         StringBuffer ids;
         if (!ListIds(query, &ids))
            break;
         g_dbCheckErrors++;
         WriteToTerminalEx(L"\nObject GUID %s is shared by objects %s\n", guid, ids.cstr());
      }
      DBFreeResult(hResult);
   }
   EndStage();
}

/**
 * Container membership cycles. Report only: which membership to break is the operator's call.
 * Every strongly connected component of the membership graph with more than one object, or with a
 * self-membership, is one finding listing all its objects and all memberships between them, so
 * overlapping cycles are shown completely in one run. Enumerating elementary cycles individually
 * would be exponential; Tarjan's algorithm is linear.
 */
static void CheckContainerCycles()
{
   StartStage(L"Container membership cycles");
   DB_RESULT hResult = CheckSelect(L"SELECT container_id,object_id FROM container_members ORDER BY container_id,object_id");
   if (hResult == nullptr)
   {
      EndStage();
      return;
   }

   std::map<uint32_t, std::vector<uint32_t>> members;
   int count = DBGetNumRows(hResult);
   for(int i = 0; i < count; i++)
      members[DBGetFieldULong(hResult, i, 0)].push_back(DBGetFieldULong(hResult, i, 1));
   DBFreeResult(hResult);

   struct NodeState
   {
      int index;
      int lowlink;
      bool onStack;
   };
   std::map<uint32_t, NodeState> state;
   std::vector<uint32_t> stack;
   int nextIndex = 0;

   struct Frame
   {
      uint32_t id;
      size_t next;
   };
   std::vector<Frame> path;

   for(auto root = members.begin(); root != members.end(); ++root)
   {
      if (state.find(root->first) != state.end())
         continue;
      state[root->first] = { nextIndex, nextIndex, true };
      nextIndex++;
      stack.push_back(root->first);
      path.push_back({ root->first, 0 });
      while(!path.empty())
      {
         Frame& frame = path.back();
         auto it = members.find(frame.id);
         if ((it != members.end()) && (frame.next < it->second.size()))
         {
            uint32_t member = it->second[frame.next++];
            auto ms = state.find(member);
            if (ms == state.end())
            {
               state[member] = { nextIndex, nextIndex, true };
               nextIndex++;
               stack.push_back(member);
               path.push_back({ member, 0 });
            }
            else if (ms->second.onStack)
            {
               NodeState& fs = state[frame.id];
               if (ms->second.index < fs.lowlink)
                  fs.lowlink = ms->second.index;
            }
            continue;
         }

         uint32_t id = frame.id;
         path.pop_back();
         NodeState& ns = state[id];
         if (!path.empty())
         {
            NodeState& parent = state[path.back().id];
            if (ns.lowlink < parent.lowlink)
               parent.lowlink = ns.lowlink;
         }
         if (ns.lowlink != ns.index)
            continue;

         // id is the root of a strongly connected component
         std::vector<uint32_t> component;
         uint32_t member;
         do
         {
            member = stack.back();
            stack.pop_back();
            state[member].onStack = false;
            component.push_back(member);
         } while(member != id);

         bool selfLoop = false;
         if (component.size() == 1)
         {
            auto edges = members.find(id);
            if (edges != members.end())
               for(size_t j = 0; j < edges->second.size(); j++)
                  if (edges->second[j] == id)
                     selfLoop = true;
         }
         if ((component.size() < 2) && !selfLoop)
            continue;

         std::sort(component.begin(), component.end());
         g_dbCheckErrors++;
         StringBuffer objects, memberships;
         for(size_t j = 0; j < component.size(); j++)
         {
            if (j > 0)
               objects.append(L", ");
            objects.append(component[j]);
            auto edges = members.find(component[j]);
            if (edges == members.end())
               continue;
            for(size_t k = 0; k < edges->second.size(); k++)
            {
               if (!std::binary_search(component.begin(), component.end(), edges->second[k]))
                  continue;
               if (!memberships.isEmpty())
                  memberships.append(L", ");
               memberships.append(component[j]).append(L" -> ").append(edges->second[k]);
            }
         }
         WriteToTerminalEx(L"\nContainer membership cycle among objects %s (%s)\n", objects.cstr(), memberships.cstr());
      }
   }
   EndStage();
}

/**
 * Same subnet address and mask more than once in a zone. Report only. The same address with
 * another mask is a different subnet.
 */
static void CheckDuplicateSubnets()
{
   StartStage(L"Duplicate subnets");
   DB_RESULT hResult = CheckSelect(L"SELECT ip_addr,ip_netmask,zone_guid FROM subnets GROUP BY ip_addr,ip_netmask,zone_guid HAVING count(*) > 1 ORDER BY zone_guid,ip_addr,ip_netmask");
   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; (i < count) && !s_checkAborted; i++)
      {
         wchar_t addr[64];
         DBGetField(hResult, i, 0, addr, 64);
         int maskBits = DBGetFieldLong(hResult, i, 1);
         uint32_t zoneUIN = DBGetFieldULong(hResult, i, 2);
         StringBuffer query(L"SELECT id FROM subnets WHERE ip_addr=");
         query.append(DBPrepareString(g_dbHandle, addr)).append(L" AND ip_netmask=").append(maskBits).append(L" AND zone_guid=").append(zoneUIN).append(L" ORDER BY id");
         StringBuffer ids;
         if (!ListIds(query, &ids))
            break;
         g_dbCheckErrors++;
         WriteToTerminalEx(L"\nSubnet %s/%d in zone %u is defined by objects %s\n", addr, maskBits, zoneUIN, ids.cstr());
      }
      DBFreeResult(hResult);
   }
   EndStage();
}

/**
 * Check node objects
 */
static void CheckNodes()
{
   StartStage(_T("Node to subnet bindings"));
   DB_RESULT hResult = SQLSelect(_T("SELECT n.id,p.name FROM nodes n INNER JOIN object_properties p ON p.object_id = n.id WHERE p.is_deleted=0"));
   int count = DBGetNumRows(hResult);
   SetStageWorkTotal(count);
   for(int i = 0; i < count; i++)
   {
      uint32_t nodeId = DBGetFieldULong(hResult, i, 0);

      DB_RESULT hResult2 = SQLSelect(StringBuffer(_T("SELECT subnet_id FROM nsmap WHERE node_id=")).append(nodeId));
      if (hResult2 != nullptr)
      {
         if ((DBGetNumRows(hResult2) == 0) && (!NodeInContainer(nodeId)))
         {
            TCHAR nodeName[MAX_OBJECT_NAME];
            DBGetField(hResult, i, 1, nodeName, MAX_OBJECT_NAME);
            if (!FindSubnetForNode(nodeId, nodeName))
            {
               g_dbCheckErrors++;
               if (GetYesNoEx(_T("Unlinked node object \"%s\" [%u]. Delete it?"), nodeName, nodeId))
               {
                  bool success = SQLQuery(StringBuffer(_T("DELETE FROM nodes WHERE id=")).append(nodeId));
                  success = success && SQLQuery(StringBuffer(_T("DELETE FROM acl WHERE object_id=")).append(nodeId));
                  success = success && SQLQuery(StringBuffer(_T("DELETE FROM icmp_statistics WHERE object_id=")).append(nodeId));
                  success = success && SQLQuery(StringBuffer(_T("DELETE FROM icmp_target_address_list WHERE node_id=")).append(nodeId));
                  success = success && SQLQuery(StringBuffer(_T("DELETE FROM software_inventory WHERE node_id=")).append(nodeId));
                  success = success && SQLQuery(StringBuffer(_T("DELETE FROM hardware_inventory WHERE node_id=")).append(nodeId));
                  success = success && SQLQuery(StringBuffer(_T("DELETE FROM node_components WHERE node_id=")).append(nodeId));
                  success = success && SQLQuery(StringBuffer(_T("DELETE FROM ospf_areas WHERE node_id=")).append(nodeId));
                  success = success && SQLQuery(StringBuffer(_T("DELETE FROM ospf_neighbors WHERE node_id=")).append(nodeId));
                  success = success && SQLQuery(StringBuffer(_T("DELETE FROM node_snmp_agents WHERE node_id=")).append(nodeId));
                  if (success && SQLQuery(StringBuffer(_T("DELETE FROM object_properties WHERE object_id=")).append(nodeId)))
                     g_dbCheckFixes++;
               }
            }
         }
         DBFreeResult(hResult2);
      }
      UpdateStageProgress(1);
   }
   DBFreeResult(hResult);
   EndStage();
}

/**
 * Check node component objects
 */
static void CheckComponents(const TCHAR *pszDisplayName, const TCHAR *pszTable)
{
   TCHAR stageName[256];
   _sntprintf(stageName, 256, _T("%s bindings"), pszDisplayName);
   StartStage(stageName);

   TCHAR query[256];
   _sntprintf(query, 256, _T("SELECT id,node_id FROM %s"), pszTable);
   DB_RESULT hResult = SQLSelect(query);
   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      SetStageWorkTotal(count);
      for(int i = 0; i < count; i++)
      {
         uint32_t objectId = DBGetFieldULong(hResult, i, 0);

         // Check if referred node exists
         _sntprintf(query, 256, _T("SELECT name FROM object_properties WHERE object_id=%u AND is_deleted=0"), DBGetFieldULong(hResult, i, 1));
         DB_RESULT hResult2 = SQLSelect(query);
         if (hResult2 != nullptr)
         {
            if (DBGetNumRows(hResult2) == 0)
            {
               g_dbCheckErrors++;
               TCHAR objectName[MAX_OBJECT_NAME];
               if (GetYesNoEx(_T("Unlinked %s object \"%s\" [%u]. Delete it?"), pszDisplayName, DBMgrGetObjectName(objectId, objectName), objectId))
               {
                  _sntprintf(query, 256, _T("DELETE FROM %s WHERE id=%u"), pszTable, objectId);
                  if (SQLQuery(query))
                  {
                     _sntprintf(query, 256, _T("DELETE FROM object_properties WHERE object_id=%u"), objectId);
                     SQLQuery(query);
                     g_dbCheckFixes++;
                  }
               }
            }
            DBFreeResult(hResult2);
         }
      }
      DBFreeResult(hResult);
   }
   EndStage();
}

/**
 * Check common object properties
 */
static void CheckObjectProperties()
{
   StartStage(_T("Object properties"));
   DB_RESULT hResult = SQLSelect(_T("SELECT object_id,name,last_modified FROM object_properties"));
   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      SetStageWorkTotal(count);
      for(int i = 0; i < count; i++)
      {
         uint32_t objectId = DBGetFieldULong(hResult, i, 0);

         // Check last change time
         if (DBGetFieldULong(hResult, i, 2) == 0)
         {
            g_dbCheckErrors++;
            TCHAR objectName[MAX_OBJECT_NAME];
            if (GetYesNoEx(_T("Object %d [%s] has invalid timestamp. Fix it?"),
				                objectId, DBGetField(hResult, i, 1, objectName, MAX_OBJECT_NAME)))
            {
               TCHAR query[256];
               _sntprintf(query, 256, _T("UPDATE object_properties SET last_modified=") TIME_T_FMT _T(" WHERE object_id=%d"),
                          TIME_T_FCAST(time(nullptr)), (int)objectId);
               if (SQLQuery(query))
                  g_dbCheckFixes++;
            }
         }
         UpdateStageProgress(1);
      }
      DBFreeResult(hResult);
   }
   EndStage();
}

/**
 * Check container membership
 */
static void CheckContainerMembership()
{
   StartStage(_T("Container membership"));
   DB_RESULT containerList = SQLSelect(_T("SELECT object_id,container_id FROM container_members"));
   DB_RESULT objectList = SQLSelect(_T("SELECT object_id FROM object_properties ORDER BY object_id"));
   if (containerList != nullptr && objectList != nullptr)
   {
      int numObjects = DBGetNumRows(objectList);
      uint32_t *objects = MemAllocArrayNoInit<uint32_t>(numObjects);
      for(int i = 0; i < numObjects; i++)
         objects[i] = DBGetFieldULong(objectList, i, 0);

      int numContainers = DBGetNumRows(containerList);
      SetStageWorkTotal(numContainers);
      for(int i = 0; i < numContainers; i++)
      {
         uint32_t objectId = DBGetFieldULong(containerList, i, 0);
         void *match = bsearch(&objectId, objects, numObjects, sizeof(uint32_t),
            [] (const void *key, const void *element) -> int
            {
               uint32_t k = *static_cast<const uint32_t*>(key);
               uint32_t e = *static_cast<const uint32_t*>(element);
               return (k < e) ? -1 : ((k > e) ? 1 : 0);
            });
         if (match == nullptr)
         {
            g_dbCheckErrors++;
            uint32_t containerId = DBGetFieldULong(containerList, i, 1);
            if (GetYesNoEx(_T("Container %u contains non-existing child %u. Fix it?"), containerId, objectId))
            {
               TCHAR query[1024];
               _sntprintf(query, 1024, _T("DELETE FROM container_members WHERE object_id=%u AND container_id=%u"), objectId, containerId);
               if (SQLQuery(query))
                  g_dbCheckFixes++;
            }
         }
         UpdateStageProgress(1);
      }

      MemFree(objects);
   }
   DBFreeResult(containerList);
   DBFreeResult(objectList);
   EndStage();
}

/**
 * Check cluster objects
 */
static void CheckClusters()
{
   StartStage(_T("Cluster member nodes"));
   DB_RESULT hResult = SQLSelect(_T("SELECT cluster_id,node_id FROM cluster_members"));
   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      SetStageWorkTotal(count);
      for(int i = 0; i < count; i++)
      {
         uint32_t nodeId = DBGetFieldULong(hResult, i, 1);
			if (!IsDatabaseRecordExist(g_dbHandle, _T("nodes"), _T("id"), nodeId))
			{
            g_dbCheckErrors++;
            uint32_t clusterId = DBGetFieldULong(hResult, i, 0);
            TCHAR name[MAX_OBJECT_NAME];
            if (GetYesNoEx(_T("Cluster object \"%s\" [%u] refers to non-existing node [%u]. Dereference?"),
				               DBMgrGetObjectName(clusterId, name), clusterId, nodeId))
            {
               TCHAR query[256];
               _sntprintf(query, 256, _T("DELETE FROM cluster_members WHERE cluster_id=%u AND node_id=%u"), clusterId, nodeId);
               if (SQLQuery(query))
               {
                  g_dbCheckFixes++;
               }
            }
			}
			UpdateStageProgress(1);
      }
      DBFreeResult(hResult);
   }
   EndStage();
}

/**
 * Returns TRUE if SELECT returns non-empty set
 */
static bool CheckResultSet(TCHAR *query)
{
   bool result = false;
   DB_RESULT hResult = SQLSelect(query);
   if (hResult != nullptr)
   {
      result = (DBGetNumRows(hResult) > 0);
      DBFreeResult(hResult);
   }
   return result;
}

/**
 * Check event processing policy
 */
static void CheckEPP()
{
   TCHAR query[1024];

   StartStage(_T("Event processing policy"));

   // Source object references. Deleting a rule's last inclusion would make it match every source
   // (epp.cpp: empty inclusion list means match-all), and deleting the last exclusion of a rule
   // without inclusions empties its filter (EPRule::isFilterEmpty counts exclusions). Both cases are
   // reported only; everything else is deleted by its full key, never by object ID alone.
   DB_RESULT hResult = CheckSelect(L"SELECT s.chain_id,s.rule_id,s.object_id,s.exclusion FROM policy_source_list s LEFT OUTER JOIN object_properties p ON p.object_id=s.object_id WHERE p.object_id IS NULL ORDER BY s.chain_id,s.rule_id,s.object_id");
   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; (i < count) && !s_checkAborted; i++)
      {
         uint32_t chainId = DBGetFieldULong(hResult, i, 0);
         uint32_t ruleId = DBGetFieldULong(hResult, i, 1);
         uint32_t objectId = DBGetFieldULong(hResult, i, 2);
         if (IsBuiltinObjectId(objectId))
            continue;   // network, service and template roots and zone 0 are valid sources without a properties row
         wchar_t exclusion[2];
         DBGetField(hResult, i, 3, exclusion, 2);
         bool isExclusion = (exclusion[0] == L'1');

         // Counted live: an earlier deletion in this loop may have changed the rule's lists
         nx_swprintf(query, 1024, L"SELECT count(*) FROM policy_source_list WHERE chain_id=%u AND rule_id=%u AND exclusion='0'", chainId, ruleId);
         DB_RESULT hCount = CheckSelect(query);
         if (hCount == nullptr)
            break;
         int inclusions = DBGetFieldLong(hCount, 0, 0);
         DBFreeResult(hCount);
         nx_swprintf(query, 1024, L"SELECT count(*) FROM policy_source_list WHERE chain_id=%u AND rule_id=%u AND exclusion='1'", chainId, ruleId);
         hCount = CheckSelect(query);
         if (hCount == nullptr)
            break;
         int exclusions = DBGetFieldLong(hCount, 0, 0);
         DBFreeResult(hCount);

         g_dbCheckErrors++;
         if (!isExclusion && (inclusions <= 1))
         {
            WriteToTerminalEx(L"\nInvalid object ID %u is the last source inclusion of policy rule [chain %u, rule %u], deleting it would make the rule match all sources\n", objectId, chainId, ruleId);
         }
         else if (isExclusion && (inclusions == 0) && (exclusions <= 1))
         {
            WriteToTerminalEx(L"\nInvalid object ID %u is the only source filter of policy rule [chain %u, rule %u], deleting it would change how the rule matches sources\n", objectId, chainId, ruleId);
         }
         else if (GetYesNoEx(L"Invalid object ID %u used in policy rule [chain %u, rule %u]. Delete it from this rule?", objectId, chainId, ruleId))
         {
            nx_swprintf(query, 1024, L"DELETE FROM policy_source_list WHERE chain_id=%u AND rule_id=%u AND object_id=%u AND exclusion='%s'", chainId, ruleId, objectId, isExclusion ? L"1" : L"0");
            if (CheckQuery(query))
               g_dbCheckFixes++;
         }
      }
      DBFreeResult(hResult);
   }
   if (s_checkAborted)
   {
      EndStage();
      return;
   }

   // Event references: a rule's last event is reported only, the rule would otherwise match all events
   ResetBulkYesNo();
   hResult = CheckSelect(L"SELECT e.chain_id,e.rule_id,e.event_code FROM policy_event_list e LEFT OUTER JOIN event_cfg c ON c.event_code=e.event_code WHERE c.event_code IS NULL ORDER BY e.chain_id,e.rule_id,e.event_code");
   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; (i < count) && !s_checkAborted; i++)
      {
         uint32_t chainId = DBGetFieldULong(hResult, i, 0);
         uint32_t ruleId = DBGetFieldULong(hResult, i, 1);
         uint32_t eventCode = DBGetFieldULong(hResult, i, 2);

         nx_swprintf(query, 1024, L"SELECT count(*) FROM policy_event_list WHERE chain_id=%u AND rule_id=%u", chainId, ruleId);
         DB_RESULT hCount = CheckSelect(query);
         if (hCount == nullptr)
            break;
         int events = DBGetFieldLong(hCount, 0, 0);
         DBFreeResult(hCount);

         g_dbCheckErrors++;
         if (events <= 1)
         {
            WriteToTerminalEx(L"\nInvalid event code 0x%08X is the last event of policy rule [chain %u, rule %u], deleting it would make the rule match all events\n", eventCode, chainId, ruleId);
         }
         else if (GetYesNoEx(L"Invalid event code 0x%08X referenced in policy rule [chain %u, rule %u]. Delete this reference?", eventCode, chainId, ruleId))
         {
            nx_swprintf(query, 1024, L"DELETE FROM policy_event_list WHERE chain_id=%u AND rule_id=%u AND event_code=%u", chainId, ruleId, eventCode);
            if (CheckQuery(query))
               g_dbCheckFixes++;
         }
      }
      DBFreeResult(hResult);
   }
   if (s_checkAborted)
   {
      EndStage();
      return;
   }

   // Check action ID's
   ResetBulkYesNo();
   hResult = SQLSelect(_T("SELECT action_id FROM policy_action_list"));
   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         uint32_t actionId = DBGetFieldULong(hResult, i, 0);
         _sntprintf(query, 1024, _T("SELECT action_id FROM actions WHERE action_id=%u"), actionId);
         if (!CheckResultSet(query))
         {
            g_dbCheckErrors++;
            if (GetYesNoEx(_T("Invalid action ID %d referenced in policy. Delete this reference?"), actionId))
            {
               _sntprintf(query, 1024, _T("DELETE FROM policy_action_list WHERE action_id=%u"), actionId);
               if (SQLQuery(query))
                  g_dbCheckFixes++;
            }
         }
      }
      DBFreeResult(hResult);
   }

   EndStage();
}

/**
 * Create idata_xx table
 */
bool CreateIDataTable(uint32_t objectId)
{
   wchar_t query[512];
   for(int i = 0; i < DCI_TABLE_CREATION_SLOT_COUNT; i++)
   {
      if (BuildIDataCreationQuery(g_dbSyntax, objectId, i, query, 512))
      {
         if (!SQLQuery(query))
            return false;
      }
   }
   return true;
}

/**
 * Create tdata_xx table
 */
bool CreateTDataTable(uint32_t objectId)
{
   wchar_t query[512];
   for(int i = 0; i < DCI_TABLE_CREATION_SLOT_COUNT; i++)
   {
      if (BuildTDataCreationQuery(g_dbSyntax, objectId, i, query, 512))
      {
         if (!SQLQuery(query))
            return false;
      }
   }
   return true;
}

/**
 * Create idata_1h_xx / idata_1d_xx aggregate table (issue #419).
 */
bool CreateIDataAggregateTable(uint32_t objectId, bool hourly)
{
   wchar_t query[512];
   for(int i = 0; i < DCI_TABLE_CREATION_SLOT_COUNT; i++)
   {
      if (BuildIDataAggregateCreationQuery(g_dbSyntax, hourly, objectId, i, query, 512))
      {
         if (!SQLQuery(query))
            return false;
      }
   }
   return true;
}

/**
 * DCI information
 */
struct DciInfo
{
   uint32_t id;
   uint32_t nodeId;
};

/**
 * Cached DCI information
 */
static DciInfo* s_dciCache = nullptr;
static size_t s_dciCacheSize = 0;
static DciInfo* s_tableDciCache = nullptr;
static size_t s_tableDciCacheSize = 0;

/**
 * Check if DCI exists
 */
static bool IsDciExists(uint32_t dciId, uint32_t nodeId, bool isTable)
{
   DciInfo* cache = isTable ? s_tableDciCache : s_dciCache;
   if (cache == nullptr)
   {
      TCHAR query[256];
      _sntprintf(query, 256, _T("SELECT item_id,node_id FROM %s ORDER BY item_id"), isTable ? _T("dc_tables") : _T("items"));
      DB_RESULT hResult = SQLSelect(query);
      if (hResult == nullptr)
         return false;

      int count = DBGetNumRows(hResult);
      if (isTable)
      {
         s_tableDciCache = MemAllocArrayNoInit<DciInfo>(count);
         s_tableDciCacheSize = count;
         cache = s_tableDciCache;
      }
      else
      {
         s_dciCache = MemAllocArrayNoInit<DciInfo>(count);
         s_dciCacheSize = count;
         cache = s_dciCache;
      }

      for(int i = 0; i < count; i++)
      {
         cache[i].id = DBGetFieldULong(hResult, i, 0);
         cache[i].nodeId = DBGetFieldULong(hResult, i, 1);
      }
      DBFreeResult(hResult);
   }
   size_t cacheSize = isTable ? s_tableDciCacheSize : s_dciCacheSize;

   DciInfo *dci = static_cast<DciInfo*>(bsearch(&dciId, cache, cacheSize, sizeof(DciInfo), [](const void *key, const void *e) -> int
      {
         uint32_t id = static_cast<const DciInfo*>(e)->id;
         uint32_t k = *static_cast<const uint32_t*>(key);
         return (k < id) ? -1 : ((k > id) ? 1 : 0);
      }));
   return (dci != nullptr) && ((nodeId == 0) || (nodeId == dci->nodeId));
}

/**
 * Check if DCI is in deleted DCI list
 */
static bool isDciInDeleteList(uint32_t nodeId, uint32_t dciId)
{
   DB_STATEMENT hStmt = DBPrepare(g_dbHandle, _T("SELECT type FROM dci_delete_list WHERE node_id=? AND dci_id=?"));
   if (hStmt == nullptr)
      return false;

   DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, nodeId);
   DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, dciId);

   DB_RESULT hResult = DBSelectPrepared(hStmt);
   bool inList = (hResult != nullptr) && (DBGetNumRows(hResult) > 0);
   DBFreeResult(hResult);

   DBFreeStatement(hStmt);
   return inList;
}

/**
 * Check collected data
 */
static void CheckCollectedData(bool isTable)
{
   StartStage(isTable ? _T("Table DCI history records") : _T("DCI history records"));

	int64_t now = GetCurrentTimeMs();
	IntegerArray<uint32_t> targets = GetDataCollectionTargets();
	SetStageWorkTotal(targets.size());
	for(int i = 0; i < targets.size(); i++)
   {
      uint32_t objectId = targets.get(i);
      TCHAR query[1024];
      _sntprintf(query, 1024, _T("SELECT count(*) FROM %s_%d WHERE %s_timestamp>") INT64_FMT,
               isTable ? _T("tdata") : _T("idata"), objectId, isTable ? _T("tdata") : _T("idata"), now);
      DB_RESULT hResult = SQLSelect(query);
      if (hResult != nullptr)
      {
         if (DBGetFieldLong(hResult, 0, 0) > 0)
         {
            g_dbCheckErrors++;
            if (GetYesNoEx(_T("Found collected data for node [%u] with timestamp in the future. Delete invalid records?"), objectId))
            {
               _sntprintf(query, 1024, _T("DELETE FROM %s_%d WHERE %s_timestamp>") INT64_FMT,
                        isTable ? _T("tdata") : _T("idata"), objectId, isTable ? _T("tdata") : _T("idata"), now);
               if (SQLQuery(query))
                  g_dbCheckFixes++;
            }
         }
         DBFreeResult(hResult);
      }
   }

	ResetBulkYesNo();

   for(int i = 0; i < targets.size(); i++)
   {
      uint32_t objectId = targets.get(i);
      wchar_t query[1024];
      _sntprintf(query, 1024, _T("SELECT distinct(item_id) FROM %s_%d"), isTable ? _T("tdata") : _T("idata"), objectId);
      DB_RESULT hResult = SQLSelect(query);
      if (hResult != nullptr)
      {
         int count = DBGetNumRows(hResult);
         for(int i = 0; i < count; i++)
         {
            uint32_t id = DBGetFieldLong(hResult, i, 0);
            if (!IsDciExists(id, objectId, isTable) && !isDciInDeleteList(objectId, id))
            {
               g_dbCheckErrors++;
               if (GetYesNoEx(_T("Found collected data for non-existing DCI [%u] on node [%u]. Delete invalid records?"), id, objectId))
               {
                  _sntprintf(query, 1024, _T("DELETE FROM %s_%d WHERE item_id=%u"), isTable ? _T("tdata") : _T("idata"), objectId, id);
                  if (SQLQuery(query))
                     g_dbCheckFixes++;
               }
            }
         }
         DBFreeResult(hResult);
      }

      UpdateStageProgress(1);
   }

	EndStage();
}

/**
 * Check collected data - single table version
 */
static void CheckCollectedDataSingleTable(bool isTable)
{
   StartStage(isTable ? _T("Table DCI history records") : _T("DCI history records"), 2);

   int64_t now = GetCurrentTimeMs();
   TCHAR query[1024];
   _sntprintf(query, 1024, _T("SELECT count(*) FROM %s WHERE %s_timestamp>") INT64_FMT,
            isTable ? _T("tdata") : _T("idata"), isTable ? _T("tdata") : _T("idata"), now);
   DB_RESULT hResult = SQLSelect(query);
   if (hResult != nullptr)
   {
      if (DBGetFieldLong(hResult, 0, 0) > 0)
      {
         g_dbCheckErrors++;
         if (GetYesNoEx(_T("Found collected data with timestamp in the future. Delete invalid records?")))
         {
            _sntprintf(query, 1024, _T("DELETE FROM %s WHERE %s_timestamp>") INT64_FMT,
                     isTable ? _T("tdata") : _T("idata"), isTable ? _T("tdata") : _T("idata"), now);
            if (SQLQuery(query))
               g_dbCheckFixes++;
         }
      }
      DBFreeResult(hResult);
   }

   UpdateStageProgress(1);
   ResetBulkYesNo();

   _sntprintf(query, 1024, _T("SELECT distinct(item_id) FROM %s"), isTable ? _T("tdata") : _T("idata"));
   hResult = SQLSelect(query);
   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         uint32_t id = DBGetFieldLong(hResult, i, 0);
         if (!IsDciExists(id, 0, isTable))
         {
            g_dbCheckErrors++;
            if (GetYesNoEx(_T("Found collected data for non-existing DCI [%u]. Delete invalid records?"), id))
            {
               _sntprintf(query, 1024, _T("DELETE FROM %s WHERE item_id=%d"), isTable ? _T("tdata") : _T("idata"), id);
               if (SQLQuery(query))
                  g_dbCheckFixes++;
            }
         }
      }
      DBFreeResult(hResult);
   }

   EndStage();
}

/**
 * Check collected data - single table TSDB version
 */
static void CheckCollectedDataSingleTable_TSDB(bool isTable)
{
   static const TCHAR *sclasses[] = { _T("default"), _T("7"), _T("30"), _T("90"), _T("180"), _T("other") };

   StartStage(isTable ? _T("Table DCI history records") : _T("DCI history records"), 6);
   time_t now = time(NULL);

   for(int sc = 0; sc < 6; sc++)
   {
      TCHAR query[1024];
      _sntprintf(query, 1024, _T("SELECT count(*) FROM %s_sc_%s WHERE %s_timestamp > to_timestamp(") TIME_T_FMT _T(")"),
               isTable ? _T("tdata") : _T("idata"), sclasses[sc], isTable ? _T("tdata") : _T("idata"), TIME_T_FCAST(now));
      DB_RESULT hResult = SQLSelect(query);
      if (hResult != NULL)
      {
         if (DBGetFieldLong(hResult, 0, 0) > 0)
         {
            g_dbCheckErrors++;
            if (GetYesNoEx(_T("Found collected data with timestamp in the future. Delete invalid records?")))
            {
               _sntprintf(query, 1024, _T("DELETE FROM %s_sc_%s WHERE %s_timestamp > to_timestamp(") TIME_T_FMT _T(")"),
                        isTable ? _T("tdata") : _T("idata"), sclasses[sc], isTable ? _T("tdata") : _T("idata"), TIME_T_FCAST(now));
               if (SQLQuery(query))
                  g_dbCheckFixes++;
            }
         }
         DBFreeResult(hResult);
      }

      UpdateStageProgress(1);
   }
   EndStage();
}

/**
 * Check sample attributes of collected DCI data
 */
static void CheckSampleAttributes()
{
   StartStage(L"DCI sample attributes", 3);

   wchar_t query[1024];
   int64_t now = GetCurrentTimeMs();
   nx_swprintf(query, 1024, L"SELECT count(*) FROM dci_sample_attributes WHERE sample_timestamp>" INT64_FMT, now);
   DB_RESULT hResult = SQLSelect(query);
   if (hResult != nullptr)
   {
      if (DBGetFieldLong(hResult, 0, 0) > 0)
      {
         g_dbCheckErrors++;
         if (GetYesNoEx(L"Found DCI sample attributes with timestamp in the future. Delete invalid records?"))
         {
            nx_swprintf(query, 1024, L"DELETE FROM dci_sample_attributes WHERE sample_timestamp>" INT64_FMT, now);
            if (SQLQuery(query))
               g_dbCheckFixes++;
         }
      }
      DBFreeResult(hResult);
   }

   UpdateStageProgress(1);
   ResetBulkYesNo();

   hResult = SQLSelect(L"SELECT distinct(item_id) FROM dci_sample_attributes");
   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         uint32_t id = DBGetFieldULong(hResult, i, 0);
         if (!IsDciExists(id, 0, false))
         {
            g_dbCheckErrors++;
            if (GetYesNoEx(L"Found sample attributes for non-existing DCI [%u]. Delete invalid records?", id))
            {
               nx_swprintf(query, 1024, L"DELETE FROM dci_sample_attributes WHERE item_id=%u", id);
               if (SQLQuery(query))
                  g_dbCheckFixes++;
            }
         }
      }
      DBFreeResult(hResult);
   }

   UpdateStageProgress(1);
   ResetBulkYesNo();

   // Computation method records are never deleted, so reference to missing method cannot be repaired by deleting sample attributes
   hResult = SQLSelect(L"SELECT distinct(method_id) FROM dci_sample_attributes WHERE method_id<>0 AND method_id NOT IN (SELECT id FROM kpi_methods)");
   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         uint32_t id = DBGetFieldULong(hResult, i, 0);
         g_dbCheckErrors++;
         if (GetYesNoEx(L"Found sample attributes referencing non-existing computation method [%u]. Reset method reference in those records?", id))
         {
            nx_swprintf(query, 1024, L"UPDATE dci_sample_attributes SET method_id=0 WHERE method_id=%u", id);
            if (SQLQuery(query))
               g_dbCheckFixes++;
         }
      }
      DBFreeResult(hResult);
   }

   EndStage();
}

/**
 * Check raw DCI values
 */
static void CheckRawDciValues()
{
   StartStage(_T("Raw DCI values table"));

   int64_t now = GetCurrentTimeMs();

   DB_RESULT hResult = SQLSelect(_T("SELECT item_id FROM raw_dci_values"));
   if (hResult != NULL)
   {
      int count = DBGetNumRows(hResult);
      SetStageWorkTotal(count + 1);
      for(int i = 0; i < count; i++)
      {
         UINT32 id = DBGetFieldLong(hResult, i, 0);
         if (!IsDciExists(id, 0, false))
         {
            g_dbCheckErrors++;
            if (GetYesNoEx(_T("Found raw value record for non-existing DCI [%u]. Delete it?"), id))
            {
               TCHAR query[256];
               _sntprintf(query, 256, _T("DELETE FROM raw_dci_values WHERE item_id=%d"), id);
               if (SQLQuery(query))
                  g_dbCheckFixes++;
            }
         }
         UpdateStageProgress(1);
      }
      DBFreeResult(hResult);
   }

   ResetBulkYesNo();
   TCHAR query[1024];
   _sntprintf(query, 1024, _T("SELECT count(*) FROM raw_dci_values WHERE last_poll_time>") INT64_FMT, now);
   hResult = SQLSelect(query);
   if (hResult != NULL)
   {
      if (DBGetFieldLong(hResult, 0, 0) > 0)
      {
         g_dbCheckErrors++;
         if (GetYesNoEx(_T("Found DCIs with last poll timestamp in the future. Fix it?")))
         {
            _sntprintf(query, 1024, _T("UPDATE raw_dci_values SET last_poll_time=") INT64_FMT _T(" WHERE last_poll_time>") INT64_FMT, now, now);
            if (SQLQuery(query))
               g_dbCheckFixes++;
         }
      }
      DBFreeResult(hResult);
   }
   UpdateStageProgress(1);

   EndStage();
}

/**
 * Check thresholds
 */
static void CheckThresholds()
{
   StartStage(_T("DCI thresholds"));

   DB_RESULT hResult = SQLSelect(_T("SELECT threshold_id,item_id FROM thresholds"));
   if (hResult != NULL)
   {
      int count = DBGetNumRows(hResult);
      SetStageWorkTotal(count);
      for(int i = 0; i < count; i++)
      {
         uint32_t dciId = DBGetFieldULong(hResult, i, 1);
         if (!IsDciExists(dciId, 0, false))
         {
            g_dbCheckErrors++;
            if (GetYesNoEx(_T("Found threshold configuration for non-existing DCI [%d]. Delete?"), dciId))
            {
               TCHAR query[256];
               _sntprintf(query, 256, _T("DELETE FROM thresholds WHERE threshold_id=%d AND item_id=%d"), DBGetFieldLong(hResult, i, 0), dciId);
               if (SQLQuery(query))
                  g_dbCheckFixes++;
            }
         }
         UpdateStageProgress(1);
      }
      DBFreeResult(hResult);
   }

   EndStage();
}

/**
 * Check thresholds
 */
static void CheckTableThresholds()
{
   StartStage(_T("Table DCI thresholds"));

   DB_RESULT hResult = SQLSelect(_T("SELECT id,table_id FROM dct_thresholds"));
   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      SetStageWorkTotal(count);
      for(int i = 0; i < count; i++)
      {
         uint32_t dciId = DBGetFieldULong(hResult, i, 1);
         if (!IsDciExists(dciId, 0, true))
         {
            g_dbCheckErrors++;
            if (GetYesNoEx(_T("Found threshold configuration for non-existing table DCI [%d]. Delete?"), dciId))
            {
               uint32_t id = DBGetFieldLong(hResult, i, 0);

               TCHAR query[256];
               _sntprintf(query, 256, _T("DELETE FROM dct_threshold_instances WHERE threshold_id=%d"), id);
               if (SQLQuery(query))
               {
                  _sntprintf(query, 256, _T("DELETE FROM dct_threshold_conditions WHERE threshold_id=%d"), id);
                  if (SQLQuery(query))
                  {
                     _sntprintf(query, 256, _T("DELETE FROM dct_thresholds WHERE id=%d"), id);
                     if (SQLQuery(query))
                        g_dbCheckFixes++;
                  }
               }
            }
         }
         UpdateStageProgress(1);
      }
      DBFreeResult(hResult);
   }

   EndStage();
}

/**
 * Check data collection items
 */
static void CheckDataCollectionItems()
{
   StartStage(_T("DCI configuration"));

   DB_RESULT hResult = SQLSelect(_T("SELECT item_id,node_id FROM items WHERE node_id NOT IN (SELECT object_id FROM object_properties)"));
   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         g_dbCheckErrors++;
         uint32_t dciId = DBGetFieldLong(hResult, i, 0);
         uint32_t objectId = DBGetFieldLong(hResult, i, 1);
         if (GetYesNoEx(_T("DCI [%u] belongs to non-existing object [%u]. Delete?"), dciId, objectId))
         {
            TCHAR query[256];
            _sntprintf(query, 256, _T("DELETE FROM items WHERE item_id=%u"), dciId);
            if (SQLQuery(query))
            {
               _sntprintf(query, 256, _T("DELETE FROM thresholds WHERE item_id=%u"), dciId);
               if (SQLQuery(query))
               {
                  _sntprintf(query, 256, _T("DELETE FROM raw_dci_values WHERE item_id=%u"), dciId);
                  if (SQLQuery(query))
                     g_dbCheckFixes++;
               }
            }
         }
      }
      DBFreeResult(hResult);
   }

   hResult = SQLSelect(_T("SELECT item_id,node_id FROM dc_tables WHERE node_id NOT IN (SELECT object_id FROM object_properties)"));
   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         g_dbCheckErrors++;
         uint32_t dciId = DBGetFieldLong(hResult, i, 0);
         uint32_t objectId = DBGetFieldLong(hResult, i, 1);
         if (GetYesNoEx(_T("Table DCI [%u] belongs to non-existing object [%u]. Delete?"), dciId, objectId))
         {
            TCHAR query[256];
            _sntprintf(query, 256, _T("DELETE FROM dc_tables WHERE item_id=%u"), dciId);
            if (SQLQuery(query))
            {
               _sntprintf(query, 256, _T("DELETE FROM dc_table_columns WHERE table_id=%u"), dciId);
               if (SQLQuery(query))
                  g_dbCheckFixes++;
            }
         }
      }
      DBFreeResult(hResult);
   }

   EndStage();
}

/**
 * Check DCI source nodes (proxy_node field)
 */
static void CheckDCISourceNodes()
{
   StartStage(_T("DCI source nodes"));

   // Check items table
   DB_RESULT hResult = SQLSelect(_T("SELECT item_id,node_id,proxy_node FROM items WHERE proxy_node<>0"));
   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      SetStageWorkTotal(count);
      for(int i = 0; i < count; i++)
      {
         uint32_t proxyNodeId = DBGetFieldULong(hResult, i, 2);
         if (!IsDatabaseRecordExist(g_dbHandle, _T("nodes"), _T("id"), proxyNodeId))
         {
            g_dbCheckErrors++;
            uint32_t dciId = DBGetFieldULong(hResult, i, 0);
            uint32_t nodeId = DBGetFieldULong(hResult, i, 1);
            if (GetYesNoEx(_T("DCI [%u] on node [%u] refers to non-existing source node [%u]. Reset source node to none?"),
                           dciId, nodeId, proxyNodeId))
            {
               TCHAR query[256];
               _sntprintf(query, 256, _T("UPDATE items SET proxy_node=0 WHERE item_id=%u"), dciId);
               if (SQLQuery(query))
                  g_dbCheckFixes++;
            }
         }
         UpdateStageProgress(1);
      }
      DBFreeResult(hResult);
   }

   // Check dc_tables table
   ResetBulkYesNo();
   hResult = SQLSelect(_T("SELECT item_id,node_id,proxy_node FROM dc_tables WHERE proxy_node<>0"));
   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         uint32_t proxyNodeId = DBGetFieldULong(hResult, i, 2);
         if (!IsDatabaseRecordExist(g_dbHandle, _T("nodes"), _T("id"), proxyNodeId))
         {
            g_dbCheckErrors++;
            uint32_t dciId = DBGetFieldULong(hResult, i, 0);
            uint32_t nodeId = DBGetFieldULong(hResult, i, 1);
            if (GetYesNoEx(_T("Table DCI [%u] on node [%u] refers to non-existing source node [%u]. Reset source node to none?"),
                           dciId, nodeId, proxyNodeId))
            {
               TCHAR query[256];
               _sntprintf(query, 256, _T("UPDATE dc_tables SET proxy_node=0 WHERE item_id=%u"), dciId);
               if (SQLQuery(query))
                  g_dbCheckFixes++;
            }
         }
      }
      DBFreeResult(hResult);
   }

   EndStage();
}

/**
 * Check if given data table exist
 */
bool IsDataTableExist(const wchar_t *format, uint32_t id)
{
   wchar_t table[256];
   nx_swprintf(table, 256, format, id);
   int rc = DBIsTableExist(g_dbHandle, table);
   if (rc == DBIsTableExist_Failure)
   {
      WriteToTerminalEx(L"WARNING: call to DBIsTableExist(\"%s\") failed\n", table);
   }
   return rc != DBIsTableExist_NotFound;
}

/**
 * Check data tables
 */
static void CheckDataTables()
{
   if (DBMgrMetaDataReadInt32(_T("SingleTablePerfData"), 0))
      return;  // Single table mode

   StartStage(_T("Data tables"));
	IntegerArray<uint32_t> targets = GetDataCollectionTargets();
	SetStageWorkTotal(targets.size());
	for(int i = 0; i < targets.size(); i++)
   {
	   uint32_t objectId = targets.get(i);

      // IDATA
      if (!IsDataTableExist(_T("idata_%u"), objectId))
      {
			g_dbCheckErrors++;

         TCHAR objectName[MAX_OBJECT_NAME];
         DBMgrGetObjectName(objectId, objectName);
			if (GetYesNoEx(_T("Data collection table (IDATA) for object %s [%d] not found. Create? (Y/N) "), objectName, objectId))
			{
				if (CreateIDataTable(objectId))
					g_dbCheckFixes++;
			}
      }

      // TDATA
      if (!IsDataTableExist(_T("tdata_%u"), objectId))
      {
			g_dbCheckErrors++;

         TCHAR objectName[MAX_OBJECT_NAME];
         DBMgrGetObjectName(objectId, objectName);
			if (GetYesNoEx(_T("Data collection table (TDATA) for %s [%d] not found. Create? (Y/N) "), objectName, objectId))
			{
				if (CreateTDataTable(objectId))
					g_dbCheckFixes++;
			}
      }

      UpdateStageProgress(1);
   }
	EndStage();


   StartStage(_T("Orphaned data tables"));
   StringList *tables = DBGetTableList(g_dbHandle);
   if (tables != nullptr)
   {
      SetStageWorkTotal(tables->size());
      for(int i = 0; i < tables->size(); i++)
      {
         const wchar_t *table = tables->get(i);
         if (!wcsncmp(table, L"idata_", 6) || !wcsncmp(table, L"tdata_", 6))
         {
            // Could be also idata_1h_NNN or idata_1d_NNN
            const wchar_t *oid = &table[6];
            if (!wcsncmp(oid, L"1h_", 3) || !wcsncmp(oid, L"1d_", 3))
               oid = &table[9];

            wchar_t *eptr;
            uint32_t objectId = wcstoul(oid, &eptr, 10);
            if ((*eptr == 0) && !targets.contains(objectId))
            {
               g_dbCheckErrors++;
               if (GetYesNoEx(_T("Data collection table %s belongs to deleted object and no longer in use. Delete it? (Y/N) "), table))
               {
                  wchar_t query[256];
                  if (!wcsncmp(table, L"tdata_", 6))
                  {
                     // Check for tdata_rows_NNN and tdata_records_NNN from older versions
                     if (IsDataTableExist(_T("tdata_rows_%u"), objectId))
                     {
                        _sntprintf(query, 256, _T("DROP TABLE tdata_rows_%u"), objectId);
                        SQLQuery(query);
                     }
                     if (IsDataTableExist(_T("tdata_records_%u"), objectId))
                     {
                        _sntprintf(query, 256, _T("DROP TABLE tdata_records_%u"), objectId);
                        SQLQuery(query);
                     }
                  }
                  _sntprintf(query, 256, _T("DROP TABLE %s"), table);
                  if (SQLQuery(query))
                     g_dbCheckFixes++;
               }
            }
         }
         UpdateStageProgress(1);
      }
      delete tables;
   }
   EndStage();
}

/**
 * Check template to data collection target mapping
 */
static void CheckTemplateToTargetMapping()
{
   TCHAR name[256], query[256];
   StartStage(_T("Template mapping"));
   DB_RESULT hResult = SQLSelect(_T("SELECT template_id,node_id FROM dct_node_map ORDER BY template_id"));
   if (hResult != nullptr)
   {
      int numRows = DBGetNumRows(hResult);
      SetStageWorkTotal(numRows);
      for(int i = 0; i < numRows; i++)
      {
         uint32_t templateId = DBGetFieldULong(hResult, i, 0);
         uint32_t targetId = DBGetFieldULong(hResult, i, 1);

         // Check node existence
         if (!IsDatabaseRecordExist(g_dbHandle, _T("nodes"), _T("id"), targetId) &&
             !IsDatabaseRecordExist(g_dbHandle, _T("clusters"), _T("id"), targetId) &&
             !IsDatabaseRecordExist(g_dbHandle, _T("mobile_devices"), _T("id"), targetId) &&
             !IsDatabaseRecordExist(g_dbHandle, _T("sensors"), _T("id"), targetId) &&
             !IsDatabaseRecordExist(g_dbHandle, _T("access_points"), _T("id"), targetId) &&
             !IsDatabaseRecordExist(g_dbHandle, _T("traffic_observers"), _T("id"), targetId) &&
             !IsDatabaseRecordExist(g_dbHandle, _T("observation_points"), _T("id"), targetId) &&
             !IsDatabaseRecordExist(g_dbHandle, _T("facilities"), _T("id"), targetId) &&
             !IsDatabaseRecordExist(g_dbHandle, _T("power_domains"), _T("id"), targetId) &&
             !IsDatabaseRecordExist(g_dbHandle, _T("cooling_zones"), _T("id"), targetId) &&
             !IsDatabaseRecordExist(g_dbHandle, _T("rooms"), _T("id"), targetId) &&
             !IsContainerObjectExists(g_dbHandle, targetId, OBJECT_COLLECTOR))
         {
            if (IsDatabaseRecordExist(g_dbHandle, _T("object_containers"), _T("id"), templateId))
            {
               g_dbCheckErrors++;
               if (GetYesNoEx(_T("Found possibly misplaced object binding %u to %u. Fix it?"), templateId, targetId))
               {
                  _sntprintf(query, 256, _T("DELETE FROM dct_node_map WHERE template_id=%u AND node_id=%u"), templateId, targetId);
                  if (SQLQuery(query))
                  {
                     _sntprintf(query, 256, _T("SELECT * FROM container_members WHERE container_id=%u AND object_id=%u"), templateId, targetId);
                     DB_RESULT containerMemberResult = SQLSelect(query);
                     if (containerMemberResult != NULL)
                     {
                        if (DBGetNumRows(containerMemberResult) == 0)
                        {
                           _sntprintf(query, 256, _T("INSERT INTO container_members (container_id,object_id) VALUES (%u,%u)"), templateId, targetId);
                           SQLQuery(query);
                        }

                        DBFreeResult(containerMemberResult);
                     }
                     g_dbCheckFixes++;
                  }
               }
            }
            else
            {
               g_dbCheckErrors++;
               DBMgrGetObjectName(templateId, name);
               if (GetYesNoEx(_T("Template %u [%s] mapped to non-existent object %u. Delete this mapping?"), templateId, name, targetId))
               {
                  _sntprintf(query, 256, _T("DELETE FROM dct_node_map WHERE template_id=%u AND node_id=%u"), templateId, targetId);
                  if (SQLQuery(query))
                     g_dbCheckFixes++;
               }
            }
         }
         UpdateStageProgress(1);
      }
      DBFreeResult(hResult);
   }
   EndStage();
}

/**
 * Check network map links
 */
static void CheckMapLinks()
{
   StartStage(_T("Network map links"));

   for(int pass = 1; pass <= 2; pass++)
   {
      TCHAR query[1024];
      _sntprintf(query, 1024,
         _T("SELECT network_map_links.map_id,network_map_links.element1,network_map_links.element2 ")
         _T("FROM network_map_links ")
         _T("LEFT OUTER JOIN network_map_elements ON ")
         _T("   network_map_links.map_id = network_map_elements.map_id AND ")
         _T("   network_map_links.element%d = network_map_elements.element_id ")
         _T("WHERE network_map_elements.element_id IS NULL"), pass);

      DB_RESULT hResult = SQLSelect(query);
      if (hResult != NULL)
      {
         int count = DBGetNumRows(hResult);
         for(int i = 0; i < count; i++)
         {
            g_dbCheckErrors++;
            DWORD mapId = DBGetFieldULong(hResult, i, 0);
            TCHAR name[MAX_OBJECT_NAME];
				DBMgrGetObjectName(mapId, name);
            if (GetYesNoEx(_T("Invalid link on network map %s [%d]. Delete?"), name, mapId))
            {
               _sntprintf(query, 256, _T("DELETE FROM network_map_links WHERE map_id=%d AND element1=%d AND element2=%d"),
                          mapId, DBGetFieldLong(hResult, i, 1), DBGetFieldLong(hResult, i, 2));
               if (SQLQuery(query))
                  g_dbCheckFixes++;
            }
         }
         DBFreeResult(hResult);
      }
   }
   EndStage();
}

/**
 * Check asset to node and node to asset linking
 */
static void CheckAssetNodeLinks()
{
   TCHAR name[256], query[512];
   StartStage(_T("Asset to node links"));
   DB_RESULT hResult = SQLSelect(_T("SELECT id,linked_object_id FROM assets WHERE linked_object_id!=0"));
   if (hResult != nullptr)
   {
      int numRows = DBGetNumRows(hResult);
      SetStageWorkTotal(numRows);
      for(int i = 0; i < numRows; i++)
      {
         uint32_t assetId = DBGetFieldULong(hResult, i, 0);
         uint32_t nodeId = DBGetFieldULong(hResult, i, 1);

         // Check node existence
         if (IsDatabaseRecordExist(g_dbHandle, _T("object_properties"), _T("object_id"), nodeId))
         {
            _sntprintf(query, 512, _T("SELECT object_id FROM object_properties WHERE asset_id=%u and object_id=%u"), assetId, nodeId);
            DB_RESULT checkResult = SQLSelect(query);
            if (checkResult != nullptr)
            {
               if (DBGetNumRows(checkResult) == 0)
               {
                  g_dbCheckErrors++;
                  if (GetYesNoEx(_T("Found possibly misplaced asset linking %u to object %u. Fix it?"), assetId, nodeId))
                  {
                     _sntprintf(query, 512, _T("UPDATE assets SET linked_object_id=0 WHERE id=%u"), assetId);
                     if (SQLQuery(query))
                     {
                        g_dbCheckFixes++;
                     }
                  }
               }
               DBFreeResult(checkResult);
            }
         }
         else
         {
            g_dbCheckErrors++;
            DBMgrGetObjectName(assetId, name);
            if (GetYesNoEx(_T("Asset %u [%s] linked to non-existent node %d. Delete this mapping?"), assetId, name, nodeId))
            {
               _sntprintf(query, 512, _T("UPDATE assets SET linked_object_id=0 WHERE id=%u"), assetId);
               if (SQLQuery(query))
                  g_dbCheckFixes++;
            }
         }
         UpdateStageProgress(1);
      }
      DBFreeResult(hResult);
   }
   EndStage();

   StartStage(_T("Node to asset links"));
   hResult = SQLSelect(_T("SELECT object_id,asset_id FROM object_properties WHERE asset_id!=0"));
   if (hResult != nullptr)
   {
      int numRows = DBGetNumRows(hResult);
      SetStageWorkTotal(numRows);
      for(int i = 0; i < numRows; i++)
      {
         uint32_t nodeId = DBGetFieldULong(hResult, i, 0);
         uint32_t assetId = DBGetFieldULong(hResult, i, 1);

         // Check asset existence
         if (IsDatabaseRecordExist(g_dbHandle, _T("assets"), _T("id"), assetId))
         {
            _sntprintf(query, 512, _T("SELECT id FROM assets WHERE id=%u and linked_object_id=%u"), assetId, nodeId);
            DB_RESULT checkResult = SQLSelect(query);
            if (checkResult != nullptr)
            {
               if (DBGetNumRows(checkResult) == 0)
               {
                  g_dbCheckErrors++;
                  if (GetYesNoEx(_T("Found possibly misplaced object linking %u to asset %u. Fix it?"), nodeId, assetId))
                  {
                     _sntprintf(query, 512, _T("UPDATE object_properties SET asset_id=0 WHERE object_id=%u"), nodeId);
                     if (SQLQuery(query))
                     {
                        g_dbCheckFixes++;
                     }
                  }
               }
               DBFreeResult(checkResult);
            }
         }
         else
         {
            g_dbCheckErrors++;
            DBMgrGetObjectName(nodeId, name);
            if (GetYesNoEx(_T("Node %u [%s] linked to non-existent asset %d. Delete this mapping?"), nodeId, name, assetId))
            {
               _sntprintf(query, 512, _T("UPDATE object_properties SET asset_id=0 WHERE object_id=%u"), nodeId);
               if (SQLQuery(query))
                  g_dbCheckFixes++;
            }
         }
         UpdateStageProgress(1);
      }
      DBFreeResult(hResult);
   }
   EndStage();
}

/**
 * Lock database
 */
static bool LockDatabase()
{
   TCHAR buffer[MAX_DB_STRING], bufferInfo[MAX_DB_STRING];
   bool result = false;

   DBMgrConfigReadStr(_T("DBLockStatus"), buffer, MAX_DB_STRING, _T("ERROR"));
   if (_tcscmp(buffer, _T("ERROR")))
   {
      int32_t lockFlag = DBMgrConfigReadInt32(_T("DBLockFlag"), 0);
      bool locked = _tcscmp(buffer, _T("UNLOCKED")) || (lockFlag != 0);

      if (locked)
      {
         DBMgrConfigReadStr(_T("DBLockInfo"), bufferInfo, MAX_DB_STRING, _T("<error>"));
         if (GetYesNo(_T("Database is locked by server %s [%s]\nDo you wish to force database unlock?"), buffer, bufferInfo))
         {
            locked = false;
            _tprintf(_T("Database lock removed\n"));
         }
      }
      if (!locked)
      {
         CreateConfigParam(_T("DBLockStatus"), _T("NXDBMGR Check"), false, true, true);
         CreateConfigParam(_T("DBLockFlag"), _T("0"), false, true, true);
         GetLocalHostName(buffer, MAX_DB_STRING, false);
         CreateConfigParam(_T("DBLockInfo"), buffer, false, false, true);
         _sntprintf(buffer, 64, _T("%u"), GetCurrentProcessId());
         CreateConfigParam(_T("DBLockPID"), buffer, false, false, true);
         result = true;
      }

      return result;
   }
   else
   {
      _tprintf(_T("Unable to get database lock status\n"));
      return result;
   }
}

/**
 * Unlock database
 */
static void RemoveDatabaseLock()
{
   CreateConfigParam(_T("DBLockStatus"), _T("UNLOCKED"), false, true, true);
   CreateConfigParam(_T("DBLockInfo"), _T(""),  false, false, true);
   CreateConfigParam(_T("DBLockPID"), _T("0"), false, false, true);
}

/**
 * Template bindings of one DCI table. template_id is polymorphic: the owner itself for instance
 * discovered DCIs, a cluster for cluster-applied DCIs, or a template. Only a binding to an object
 * that no longer exists is repaired; the other findings need the operator.
 */
static void CheckTemplateBindingsOfTable(const wchar_t *table, const wchar_t *kind)
{
   StringBuffer query(L"SELECT d.item_id,d.node_id,d.template_id,d.template_item_id,r.item_id,c.id,t.id,p.object_id,cm.node_id,nm.node_id FROM ");
   query.append(table).append(L" d LEFT OUTER JOIN ").append(table).append(L" r ON r.item_id=d.template_item_id AND r.node_id=d.node_id")
        .append(L" LEFT OUTER JOIN clusters c ON c.id=d.template_id")
        .append(L" LEFT OUTER JOIN templates t ON t.id=d.template_id")
        .append(L" LEFT OUTER JOIN object_properties p ON p.object_id=d.template_id")
        .append(L" LEFT OUTER JOIN cluster_members cm ON cm.cluster_id=d.template_id AND cm.node_id=d.node_id")
        .append(L" LEFT OUTER JOIN dct_node_map nm ON nm.template_id=d.template_id AND nm.node_id=d.node_id")
        .append(L" WHERE d.template_id<>0 ORDER BY d.item_id");
   DB_RESULT hResult = CheckSelect(query);
   if (hResult == nullptr)
      return;

   int count = DBGetNumRows(hResult);
   SetStageWorkTotal(count);
   for(int i = 0; (i < count) && !s_checkAborted; i++)
   {
      uint32_t dciId = DBGetFieldULong(hResult, i, 0);
      uint32_t ownerId = DBGetFieldULong(hResult, i, 1);
      uint32_t templateId = DBGetFieldULong(hResult, i, 2);
      uint32_t rootId = DBGetFieldULong(hResult, i, 3);
      bool rootExists = DBGetFieldULong(hResult, i, 4) != 0;
      bool isCluster = DBGetFieldULong(hResult, i, 5) != 0;
      bool isTemplate = DBGetFieldULong(hResult, i, 6) != 0;
      bool ownerExists = DBGetFieldULong(hResult, i, 7) != 0;
      bool isMember = DBGetFieldULong(hResult, i, 8) != 0;
      bool isApplied = DBGetFieldULong(hResult, i, 9) != 0;

      if (templateId == ownerId)
      {
         if (!rootExists)
         {
            g_dbCheckErrors++;
            WriteToTerminalEx(L"\n%s [%u] on object [%u] has instance discovery root [%u] which does not exist on that object\n", kind, dciId, ownerId, rootId);
         }
      }
      else if (isCluster)
      {
         if (!isMember)
         {
            g_dbCheckErrors++;
            WriteToTerminalEx(L"\n%s [%u] on object [%u] is bound to cluster [%u] but the object is not a member of that cluster\n", kind, dciId, ownerId, templateId);
         }
      }
      else if (isTemplate)
      {
         if (!isApplied)
         {
            g_dbCheckErrors++;
            WriteToTerminalEx(L"\n%s [%u] on object [%u] is bound to template [%u] which is not applied to that object\n", kind, dciId, ownerId, templateId);
         }
      }
      else if (!ownerExists)
      {
         g_dbCheckErrors++;
         if (GetYesNoEx(L"%s [%u] on object [%u] is bound to non-existing template [%u]. Unbind?", kind, dciId, ownerId, templateId))
         {
            StringBuffer fix(L"UPDATE ");
            fix.append(table).append(L" SET template_id=0,template_item_id=0 WHERE item_id=").append(dciId);
            if (CheckQuery(fix))
               g_dbCheckFixes++;
         }
      }
      else
      {
         g_dbCheckErrors++;
         WriteToTerminalEx(L"\n%s [%u] on object [%u] is bound to object [%u] which is neither a template nor a cluster\n", kind, dciId, ownerId, templateId);
      }
      UpdateStageProgress(1);
   }
   DBFreeResult(hResult);
}

/**
 * Check template, cluster and instance bindings of DCIs and table DCIs
 */
static void CheckTemplateBindings()
{
   StartStage(L"DCI template bindings");
   CheckTemplateBindingsOfTable(L"items", L"DCI");
   EndStage();
   if (s_checkAborted)
      return;

   StartStage(L"Table DCI template bindings");
   CheckTemplateBindingsOfTable(L"dc_tables", L"Table DCI");
   EndStage();
}

/**
 * Coupled reference pair (an object ID and an interface ID) where each non-zero half must exist in
 * object_properties and the whole group of columns is reset together. Peers may be access points
 * (the AP ID is stored in both columns), so object_properties is the only usable parent.
 */
static void CheckCoupledReferences(const wchar_t *stage, const wchar_t *table, const wchar_t *objectColumn, const wchar_t *ifaceColumn,
      const wchar_t *kind, const wchar_t *what, const wchar_t *resetClause)
{
   StartStage(stage);
   StringBuffer query(L"SELECT t.id,t.");
   query.append(objectColumn).append(L",t.").append(ifaceColumn).append(L",po.object_id,pi.object_id FROM ").append(table)
        .append(L" t LEFT OUTER JOIN object_properties po ON po.object_id=t.").append(objectColumn)
        .append(L" LEFT OUTER JOIN object_properties pi ON pi.object_id=t.").append(ifaceColumn)
        .append(L" WHERE (t.").append(objectColumn).append(L"<>0 AND po.object_id IS NULL) OR (t.").append(ifaceColumn).append(L"<>0 AND pi.object_id IS NULL) ORDER BY t.id");
   DB_RESULT hResult = CheckSelect(query);
   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      SetStageWorkTotal(count);
      for(int i = 0; (i < count) && !s_checkAborted; i++)
      {
         uint32_t id = DBGetFieldULong(hResult, i, 0);
         uint32_t objectId = DBGetFieldULong(hResult, i, 1);
         uint32_t ifaceId = DBGetFieldULong(hResult, i, 2);
         bool objectMissing = (objectId != 0) && (DBGetFieldULong(hResult, i, 3) == 0);
         bool ifaceMissing = (ifaceId != 0) && (DBGetFieldULong(hResult, i, 4) == 0);

         StringBuffer missing;
         if (objectMissing)
            missing.append(L"object [").append(objectId).append(L"]");
         if (ifaceMissing)
         {
            if (objectMissing)
               missing.append(L" and ");
            missing.append(L"interface [").append(ifaceId).append(L"]");
         }

         g_dbCheckErrors++;
         if (GetYesNoEx(L"%s [%u] %s refers to non-existing %s. Reset it?", kind, id, what, missing.cstr()))
         {
            StringBuffer fix(L"UPDATE ");
            fix.append(table).append(L" SET ").append(resetClause).append(L" WHERE id=").append(id);
            if (CheckQuery(fix))
               g_dbCheckFixes++;
         }
         UpdateStageProgress(1);
      }
      DBFreeResult(hResult);
   }
   EndStage();
}

/**
 * Interface peer information: node, interface, protocol and timestamp are cleared together
 */
static void CheckInterfacePeers()
{
   CheckCoupledReferences(L"Interface peers", L"interfaces", L"peer_node_id", L"peer_if_id", L"Interface", L"peer",
         L"peer_node_id=0,peer_if_id=0,peer_proto=0,peer_last_updated=0");
}

/**
 * Node path check results: reason, node and interface are reset together
 */
static void CheckNodePathCheckResults()
{
   CheckCoupledReferences(L"Node path check results", L"nodes", L"path_check_node_id", L"path_check_iface_id", L"Node", L"path check result",
         L"path_check_reason=0,path_check_node_id=0,path_check_iface_id=0");
}

/**
 * Column holding an event code, with the event the server would use by default
 */
struct EventCodeReference
{
   const wchar_t *table;
   const wchar_t *column;
   uint32_t defaultCode;
   bool zeroAllowed;    // zero means "no event"
};

static const EventCodeReference s_eventCodeReferences[] =
{
   { L"thresholds", L"event_code", EVENT_THRESHOLD_REACHED, false },
   { L"thresholds", L"rearm_event_code", EVENT_THRESHOLD_REARMED, false },
   { L"dct_thresholds", L"activation_event", EVENT_TABLE_THRESHOLD_ACTIVATED, false },
   { L"dct_thresholds", L"deactivation_event", EVENT_TABLE_THRESHOLD_DEACTIVATED, false },
   { L"conditions", L"activation_event", EVENT_CONDITION_ACTIVATED, false },
   { L"conditions", L"deactivation_event", EVENT_CONDITION_DEACTIVATED, false },
   { L"snmp_trap_cfg", L"event_code", EVENT_SNMP_UNMATCHED_TRAP, false },
   { L"event_policy", L"alarm_timeout_event", EVENT_ALARM_TIMEOUT, true }
};

/**
 * Event codes that no longer exist in event_cfg, grouped per code. The fix resets them to the
 * default event of that column, which is only offered when the default itself exists.
 */
static void CheckEventCodeReferences()
{
   for(size_t i = 0; (i < sizeof(s_eventCodeReferences) / sizeof(EventCodeReference)) && !s_checkAborted; i++)
   {
      const EventCodeReference& r = s_eventCodeReferences[i];
      wchar_t stageName[256];
      nx_swprintf(stageName, 256, L"%s.%s", r.table, r.column);
      StartStage(stageName);

      StringBuffer query(L"SELECT t.");
      query.append(r.column).append(L",count(*) FROM ").append(r.table).append(L" t LEFT OUTER JOIN event_cfg e ON e.event_code=t.").append(r.column).append(L" WHERE ");
      if (r.zeroAllowed)
         query.append(L"t.").append(r.column).append(L"<>0 AND ");
      query.append(L"e.event_code IS NULL GROUP BY t.").append(r.column);
      DB_RESULT hResult = CheckSelect(query);
      if (hResult != nullptr)
      {
         int count = DBGetNumRows(hResult);
         SetStageWorkTotal(count);
         // A failed lookup aborts the check through CheckSelect; only an empty result means the default is missing
         bool defaultExists = false;
         if (count > 0)
         {
            StringBuffer defaultQuery(L"SELECT event_code FROM event_cfg WHERE event_code=");
            defaultQuery.append(r.defaultCode);
            DB_RESULT hDefault = CheckSelect(defaultQuery);
            if (hDefault != nullptr)
            {
               defaultExists = DBGetNumRows(hDefault) > 0;
               DBFreeResult(hDefault);
            }
         }
         for(int j = 0; (j < count) && !s_checkAborted; j++)
         {
            uint32_t code = DBGetFieldULong(hResult, j, 0);
            int rows = DBGetFieldLong(hResult, j, 1);
            g_dbCheckErrors++;
            if (!defaultExists)
            {
               WriteToTerminalEx(L"\n%d rows in %s.%s refer to non-existing event code %u and default event %u does not exist either\n", rows, r.table, r.column, code, r.defaultCode);
            }
            else if (GetYesNoEx(L"%d rows in %s.%s refer to non-existing event code %u. Reset to default event %u (EPP behavior may change)?", rows, r.table, r.column, code, r.defaultCode))
            {
               StringBuffer fix(L"UPDATE ");
               fix.append(r.table).append(L" SET ").append(r.column).append(L"=").append(r.defaultCode).append(L" WHERE ").append(r.column).append(L"=").append(code);
               if (CheckQuery(fix))
                  g_dbCheckFixes++;
            }
            UpdateStageProgress(1);
         }
         DBFreeResult(hResult);
      }
      EndStage();
   }
}

/**
 * Notification actions whose channel does not exist. Report only: the operator picks the channel.
 * FORWARD_EVENT actions store the forwarder name in the same column and are not channel references.
 */
static void CheckNotificationChannelReferences()
{
   StartStage(L"Notification channel references");
   StringBuffer query(L"SELECT a.action_id,a.action_name,a.channel_name FROM actions a LEFT OUTER JOIN notification_channels c ON c.name=a.channel_name WHERE a.action_type=");
   // LIKE '_%' is "at least one character" on every supported database; <>'' is never true on Oracle where '' is NULL
   query.append(static_cast<int>(ServerActionType::NOTIFICATION)).append(L" AND a.channel_name LIKE '_%' AND c.name IS NULL ORDER BY a.action_id");
   DB_RESULT hResult = CheckSelect(query);
   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         g_dbCheckErrors++;
         wchar_t name[MAX_OBJECT_NAME], channel[MAX_OBJECT_NAME];
         WriteToTerminalEx(L"\nNotification action [%u] (\"%s\") uses non-existing notification channel \"%s\"\n", DBGetFieldULong(hResult, i, 0),
               DBGetField(hResult, i, 1, name, MAX_OBJECT_NAME), DBGetField(hResult, i, 2, channel, MAX_OBJECT_NAME));
      }
      DBFreeResult(hResult);
   }
   EndStage();
}

/**
 * What to do with child rows whose parent does not exist
 */
enum class OrphanFix
{
   DeleteRow,     // DELETE FROM child WHERE key=X
   ResetToZero,   // UPDATE child SET key=0 WHERE key=X
   ReportOnly
};

/**
 * Child-to-parent relation checked by CheckOrphanRelation()
 */
struct OrphanRelation
{
   const wchar_t *childTable;
   const wchar_t *childKey;
   const wchar_t *parentSource;  // table name or parenthesized subquery, aliased as p
   const wchar_t *parentKey;
   const wchar_t *parentName;    // "object", "DCI", "template", ... for messages
   OrphanFix fix;
   bool zeroAllowed;             // true: zero means "not set" and is skipped
   const wchar_t *warning;       // appended to the prompt, may be nullptr
};

/**
 * Relations checked by CheckOrphanRelations(), in dependency order: a parent's own orphan check
 * runs before checks of its dependents. Zero is skipped unless the column is a required ownership
 * link, because a false positive on a legitimately unset reference would delete valid rows while a
 * skipped garbage row with key zero is harmless.
 */
static const OrphanRelation s_orphanRelations[] =
{
   { L"acl", L"object_id", L"object_properties", L"object_id", L"object", OrphanFix::DeleteRow, true, nullptr },
   { L"object_custom_attributes", L"object_id", L"object_properties", L"object_id", L"object", OrphanFix::DeleteRow, true, nullptr },
   { L"object_urls", L"object_id", L"object_properties", L"object_id", L"object", OrphanFix::DeleteRow, true, nullptr },
   { L"trusted_objects", L"object_id", L"object_properties", L"object_id", L"object", OrphanFix::DeleteRow, true, nullptr },
   { L"trusted_objects", L"trusted_object_id", L"object_properties", L"object_id", L"object", OrphanFix::DeleteRow, true, nullptr },
   { L"responsible_users", L"object_id", L"object_properties", L"object_id", L"object", OrphanFix::DeleteRow, true, nullptr },
   { L"object_access_snapshot", L"object_id", L"object_properties", L"object_id", L"object", OrphanFix::DeleteRow, true, nullptr },
   { L"pollable_objects", L"id", L"object_properties", L"object_id", L"object", OrphanFix::DeleteRow, true, nullptr },
   { L"dc_targets", L"id", L"object_properties", L"object_id", L"object", OrphanFix::DeleteRow, true, nullptr },
   { L"auto_bind_target", L"object_id", L"object_properties", L"object_id", L"object", OrphanFix::DeleteRow, true, nullptr },
   { L"versionable_object", L"object_id", L"object_properties", L"object_id", L"object", OrphanFix::DeleteRow, true, nullptr },
   { L"maintenance_journal", L"object_id", L"object_properties", L"object_id", L"object", OrphanFix::DeleteRow, true, nullptr },
   { L"active_downtimes", L"object_id", L"object_properties", L"object_id", L"object", OrphanFix::DeleteRow, true, nullptr },
   { L"downtime_log", L"object_id", L"object_properties", L"object_id", L"object", OrphanFix::DeleteRow, true, nullptr },
   { L"container_members", L"container_id", L"object_properties", L"object_id", L"container", OrphanFix::DeleteRow, true, nullptr },
   { L"nsmap", L"subnet_id", L"object_properties", L"object_id", L"subnet", OrphanFix::DeleteRow, true, nullptr },
   { L"nsmap", L"node_id", L"object_properties", L"object_id", L"node", OrphanFix::DeleteRow, true, nullptr },
   { L"zone_proxies", L"object_id", L"object_properties", L"object_id", L"zone", OrphanFix::DeleteRow, true, nullptr },
   { L"zone_proxies", L"proxy_node", L"object_properties", L"object_id", L"proxy node", OrphanFix::DeleteRow, true, nullptr },
   { L"icmp_statistics", L"object_id", L"object_properties", L"object_id", L"object", OrphanFix::DeleteRow, true, nullptr },
   { L"icmp_target_address_list", L"node_id", L"object_properties", L"object_id", L"node", OrphanFix::DeleteRow, true, nullptr },
   { L"software_inventory", L"node_id", L"object_properties", L"object_id", L"node", OrphanFix::DeleteRow, true, nullptr },
   { L"hardware_inventory", L"node_id", L"object_properties", L"object_id", L"node", OrphanFix::DeleteRow, true, nullptr },
   { L"node_components", L"node_id", L"object_properties", L"object_id", L"node", OrphanFix::DeleteRow, true, nullptr },
   { L"ospf_areas", L"node_id", L"object_properties", L"object_id", L"node", OrphanFix::DeleteRow, true, nullptr },
   { L"ospf_neighbors", L"node_id", L"object_properties", L"object_id", L"node", OrphanFix::DeleteRow, true, nullptr },
   { L"node_snmp_agents", L"node_id", L"object_properties", L"object_id", L"node", OrphanFix::DeleteRow, true, nullptr },
   { L"interface_address_list", L"iface_id", L"object_properties", L"object_id", L"interface", OrphanFix::DeleteRow, false, nullptr },
   { L"interface_vlan_list", L"iface_id", L"object_properties", L"object_id", L"interface", OrphanFix::DeleteRow, false, nullptr },
   { L"cluster_members", L"cluster_id", L"object_properties", L"object_id", L"cluster", OrphanFix::DeleteRow, true, nullptr },
   { L"cluster_sync_subnets", L"cluster_id", L"object_properties", L"object_id", L"cluster", OrphanFix::DeleteRow, true, nullptr },
   { L"rack_passive_elements", L"rack_id", L"object_properties", L"object_id", L"rack", OrphanFix::DeleteRow, true, nullptr },
   { L"room_passive_elements", L"room_id", L"object_properties", L"object_id", L"room", OrphanFix::DeleteRow, true, nullptr },
   { L"physical_links", L"left_object_id", L"object_properties", L"object_id", L"object", OrphanFix::DeleteRow, true, nullptr },
   { L"physical_links", L"right_object_id", L"object_properties", L"object_id", L"object", OrphanFix::DeleteRow, true, nullptr },
   { L"network_map_elements", L"map_id", L"object_properties", L"object_id", L"network map", OrphanFix::DeleteRow, true, nullptr },
   { L"network_map_links", L"map_id", L"object_properties", L"object_id", L"network map", OrphanFix::DeleteRow, true, nullptr },
   { L"network_map_seed_nodes", L"map_id", L"object_properties", L"object_id", L"network map", OrphanFix::DeleteRow, true, nullptr },
   { L"network_map_seed_nodes", L"seed_node_id", L"object_properties", L"object_id", L"seed node", OrphanFix::DeleteRow, true, nullptr },
   { L"network_map_deleted_nodes", L"map_id", L"object_properties", L"object_id", L"network map", OrphanFix::DeleteRow, true, nullptr },
   { L"dashboard_elements", L"dashboard_id", L"object_properties", L"object_id", L"dashboard", OrphanFix::DeleteRow, true, nullptr },
   { L"dashboard_template_instances", L"dashboard_template_id", L"object_properties", L"object_id", L"dashboard template", OrphanFix::DeleteRow, true, nullptr },
   { L"dashboard_associations", L"object_id", L"object_properties", L"object_id", L"object", OrphanFix::DeleteRow, true, nullptr },
   { L"business_service_downtime", L"service_id", L"object_properties", L"object_id", L"business service", OrphanFix::DeleteRow, true, nullptr },
   { L"asset_properties", L"asset_id", L"object_properties", L"object_id", L"asset", OrphanFix::DeleteRow, true, nullptr },
   { L"vpn_connector_networks", L"vpn_id", L"object_properties", L"object_id", L"VPN connector", OrphanFix::DeleteRow, true, nullptr },
   { L"resource_tags", L"resource_id", L"object_properties", L"object_id", L"resource", OrphanFix::DeleteRow, true, nullptr },
   { L"observation_point_hosts", L"point_id", L"object_properties", L"object_id", L"observation point", OrphanFix::DeleteRow, true, nullptr },
   { L"ap_common", L"owner_id", L"object_properties", L"object_id", L"template", OrphanFix::DeleteRow, true, nullptr },
   { L"cond_dci_map", L"condition_id", L"object_properties", L"object_id", L"condition", OrphanFix::DeleteRow, true, nullptr },
   { L"port_stop_list", L"object_id", L"object_properties", L"object_id", L"object", OrphanFix::DeleteRow, true, nullptr },
   { L"object_ai_data", L"object_id", L"object_properties", L"object_id", L"object", OrphanFix::DeleteRow, true, nullptr },

   // Class table parents
   { L"radios", L"owner_id", L"(SELECT id FROM nodes UNION SELECT id FROM access_points)", L"id", L"node or access point", OrphanFix::DeleteRow, true, nullptr },
   { L"cluster_resources", L"cluster_id", L"clusters", L"id", L"cluster", OrphanFix::DeleteRow, true, nullptr },
   { L"dashboard_associations", L"dashboard_id", L"dashboards", L"id", L"dashboard", OrphanFix::DeleteRow, true, nullptr },

   // DCI parents, then table threshold satellites
   { L"dci_schedules", L"item_id", L"(SELECT item_id FROM items UNION SELECT item_id FROM dc_tables)", L"item_id", L"DCI", OrphanFix::DeleteRow, true, nullptr },
   { L"dci_access", L"dci_id", L"(SELECT item_id FROM items UNION SELECT item_id FROM dc_tables)", L"item_id", L"DCI", OrphanFix::DeleteRow, true, nullptr },
   { L"dc_table_columns", L"table_id", L"dc_tables", L"item_id", L"table DCI", OrphanFix::DeleteRow, true, nullptr },
   { L"dct_threshold_conditions", L"threshold_id", L"dct_thresholds", L"id", L"table threshold", OrphanFix::DeleteRow, true, nullptr },
   { L"dct_threshold_instances", L"threshold_id", L"dct_thresholds", L"id", L"table threshold", OrphanFix::DeleteRow, true, nullptr },

   // Optional references reset to "not set", as Node::onObjectDelete does for a deleted object
   { L"nodes", L"poller_node_id", L"object_properties", L"object_id", L"object", OrphanFix::ResetToZero, true, L"polling routing may change" },
   { L"nodes", L"proxy_node", L"object_properties", L"object_id", L"object", OrphanFix::ResetToZero, true, L"polling routing may change" },
   { L"nodes", L"snmp_proxy", L"object_properties", L"object_id", L"object", OrphanFix::ResetToZero, true, L"polling routing may change" },
   { L"nodes", L"eip_proxy", L"object_properties", L"object_id", L"object", OrphanFix::ResetToZero, true, L"polling routing may change" },
   { L"nodes", L"icmp_proxy", L"object_properties", L"object_id", L"object", OrphanFix::ResetToZero, true, L"polling routing may change" },
   { L"nodes", L"ssh_proxy", L"object_properties", L"object_id", L"object", OrphanFix::ResetToZero, true, L"polling routing may change" },
   { L"nodes", L"netconf_proxy", L"object_properties", L"object_id", L"object", OrphanFix::ResetToZero, true, L"polling routing may change" },
   { L"nodes", L"vnc_proxy", L"object_properties", L"object_id", L"object", OrphanFix::ResetToZero, true, L"polling routing may change" },
   { L"nodes", L"mqtt_proxy", L"object_properties", L"object_id", L"object", OrphanFix::ResetToZero, true, L"polling routing may change" },
   { L"nodes", L"modbus_proxy", L"object_properties", L"object_id", L"object", OrphanFix::ResetToZero, true, L"polling routing may change" },
   { L"nodes", L"physical_container_id", L"object_properties", L"object_id", L"object", OrphanFix::ResetToZero, true, nullptr },
   { L"interfaces", L"parent_iface", L"object_properties", L"object_id", L"interface", OrphanFix::ResetToZero, true, nullptr },
   { L"object_properties", L"drilldown_object_id", L"object_properties", L"object_id", L"object", OrphanFix::ResetToZero, true, nullptr },
   { L"conditions", L"source_object", L"object_properties", L"object_id", L"object", OrphanFix::ResetToZero, true, nullptr },
   { L"dashboards", L"forced_context_object_id", L"object_properties", L"object_id", L"object", OrphanFix::ResetToZero, true, nullptr },
   { L"items", L"related_object", L"object_properties", L"object_id", L"object", OrphanFix::ResetToZero, true, nullptr },
   { L"dc_tables", L"related_object", L"object_properties", L"object_id", L"object", OrphanFix::ResetToZero, true, nullptr },

   // Report only: no repair is safe without knowing the operator's intent
   { L"nodes", L"zone_guid", L"zones", L"zone_guid", L"zone", OrphanFix::ReportOnly, true, nullptr },
   { L"subnets", L"zone_guid", L"zones", L"zone_guid", L"zone", OrphanFix::ReportOnly, true, nullptr },
   { L"dci_delete_list", L"node_id", L"object_properties", L"object_id", L"object", OrphanFix::ReportOnly, true, nullptr },
   { L"business_service_checks", L"related_object", L"object_properties", L"object_id", L"object", OrphanFix::ReportOnly, true, nullptr },
   { L"business_service_checks", L"prototype_service_id", L"object_properties", L"object_id", L"object", OrphanFix::ReportOnly, true, nullptr },
   { L"business_service_checks", L"related_dci", L"(SELECT item_id FROM items UNION SELECT item_id FROM dc_tables)", L"item_id", L"DCI", OrphanFix::ReportOnly, true, nullptr },
   { L"cond_dci_map", L"node_id", L"object_properties", L"object_id", L"object", OrphanFix::ReportOnly, true, nullptr },
   { L"cond_dci_map", L"dci_id", L"(SELECT item_id FROM items UNION SELECT item_id FROM dc_tables)", L"item_id", L"DCI", OrphanFix::ReportOnly, true, nullptr },
   { L"scheduled_tasks", L"object_id", L"object_properties", L"object_id", L"object", OrphanFix::ReportOnly, true, nullptr },
   { L"dashboard_template_instances", L"instance_object_id", L"object_properties", L"object_id", L"object", OrphanFix::ReportOnly, true, nullptr },

   // Event processing policy chains: chain 0 is the main chain and has its own row, so zero is not "not set"
   { L"event_policy", L"chain_id", L"event_policy_chain", L"chain_id", L"chain", OrphanFix::DeleteRow, false, nullptr },
   { L"policy_chain_acl", L"chain_id", L"event_policy_chain", L"chain_id", L"chain", OrphanFix::DeleteRow, false, nullptr },
   { L"policy_chain_call_list", L"target_chain_id", L"event_policy_chain", L"chain_id", L"chain", OrphanFix::DeleteRow, false, nullptr },
   { L"alarm_category_map", L"category_id", L"alarm_categories", L"id", L"alarm category", OrphanFix::DeleteRow, true, nullptr }
};

/**
 * Event processing policy tables keyed by (chain_id, rule_id)
 */
static const wchar_t *s_eppRuleTables[] =
{
   L"policy_action_list",
   L"policy_timer_cancellation_list",
   L"policy_event_list",
   L"policy_time_frame_list",
   L"policy_source_list",
   L"policy_pstorage_actions",
   L"policy_cattr_actions",
   L"alarm_category_map",
   L"policy_chain_call_list"
};

/**
 * Check event processing policy tables for rows whose (chain_id, rule_id) has no rule
 */
static void CheckEppRelations()
{
   for(size_t i = 0; (i < sizeof(s_eppRuleTables) / sizeof(s_eppRuleTables[0])) && !s_checkAborted; i++)
   {
      const wchar_t *table = s_eppRuleTables[i];
      StartStage(table);

      StringBuffer query(L"SELECT c.chain_id,c.rule_id,count(*) FROM ");
      query.append(table).append(L" c LEFT OUTER JOIN event_policy p ON p.chain_id=c.chain_id AND p.rule_id=c.rule_id WHERE p.rule_id IS NULL GROUP BY c.chain_id,c.rule_id");
      DB_RESULT hResult = CheckSelect(query);
      if (hResult != nullptr)
      {
         int count = DBGetNumRows(hResult);
         SetStageWorkTotal(count);
         for(int j = 0; (j < count) && !s_checkAborted; j++)
         {
            uint32_t chainId = DBGetFieldULong(hResult, j, 0);
            uint32_t ruleId = DBGetFieldULong(hResult, j, 1);
            g_dbCheckErrors++;
            if (GetYesNoEx(L"%d rows in %s refer to non-existing rule [chain %u, rule %u]. Delete them?", DBGetFieldLong(hResult, j, 2), table, chainId, ruleId))
            {
               StringBuffer fix(L"DELETE FROM ");
               fix.append(table).append(L" WHERE chain_id=").append(chainId).append(L" AND rule_id=").append(ruleId);
               if (CheckQuery(fix))
                  g_dbCheckFixes++;
            }
            UpdateStageProgress(1);
         }
         DBFreeResult(hResult);
      }
      EndStage();
   }
}

/**
 * Check one child-to-parent relation: one grouped query, one prompt per missing parent
 */
static void CheckOrphanRelation(const OrphanRelation& r)
{
   wchar_t stageName[256];
   nx_swprintf(stageName, 256, L"%s.%s", r.childTable, r.childKey);
   StartStage(stageName);

   StringBuffer query(L"SELECT c.");
   query.append(r.childKey).append(L",count(*) FROM ").append(r.childTable).append(L" c LEFT OUTER JOIN ")
        .append(r.parentSource).append(L" p ON p.").append(r.parentKey).append(L"=c.").append(r.childKey)
        .append(L" WHERE ");
   if (r.zeroAllowed)
      query.append(L"c.").append(r.childKey).append(L"<>0 AND ");
   query.append(L"p.").append(r.parentKey).append(L" IS NULL GROUP BY c.").append(r.childKey);

   DB_RESULT hResult = CheckSelect(query);
   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      SetStageWorkTotal(count);
      for(int i = 0; (i < count) && !s_checkAborted; i++)
      {
         uint32_t id = DBGetFieldULong(hResult, i, 0);
         int rows = DBGetFieldLong(hResult, i, 1);

         // Built-in objects (network root, template root, zone 0, ...) exist in the server without a
         // properties row in older databases, so references to them are valid
         if (!wcscmp(r.parentSource, L"object_properties") && IsBuiltinObjectId(id))
         {
            UpdateStageProgress(1);
            continue;
         }
         g_dbCheckErrors++;

         StringBuffer message;
         message.appendFormattedString(L"%d rows in %s.%s refer to non-existing %s [%u]", rows, r.childTable, r.childKey, r.parentName, id);
         if (r.warning != nullptr)
            message.append(L" (").append(r.warning).append(L")");

         StringBuffer fix;
         switch(r.fix)
         {
            case OrphanFix::DeleteRow:
               message.append(L". Delete them?");
               fix.append(L"DELETE FROM ").append(r.childTable).append(L" WHERE ").append(r.childKey).append(L"=").append(id);
               break;
            case OrphanFix::ResetToZero:
               message.append(L". Reset reference?");
               fix.append(L"UPDATE ").append(r.childTable).append(L" SET ").append(r.childKey).append(L"=0 WHERE ").append(r.childKey).append(L"=").append(id);
               break;
            case OrphanFix::ReportOnly:
               WriteToTerminalEx(L"\n%s\n", message.cstr());
               break;
         }

         if (!fix.isEmpty() && GetYesNoEx(L"%s", message.cstr()))
         {
            if (CheckQuery(fix))
               g_dbCheckFixes++;
         }
         UpdateStageProgress(1);
      }
      DBFreeResult(hResult);
   }
   EndStage();
}

/**
 * Check all relations from s_orphanRelations
 */
static void CheckOrphanRelations()
{
   for(size_t i = 0; (i < sizeof(s_orphanRelations) / sizeof(OrphanRelation)) && !s_checkAborted; i++)
      CheckOrphanRelation(s_orphanRelations[i]);
}

/**
 * Interface object stages
 */
static void CheckInterfaces()
{
   CheckComponents(L"Interface", L"interfaces");
}

/**
 * Network service object stages
 */
static void CheckNetworkServices()
{
   CheckComponents(L"Network service", L"network_services");
}

/**
 * Check stages in execution order. Stops at the first stage that sets s_checkAborted.
 */
static void (*s_checkStages[])() =
{
   CheckObjectClassCoverage,
   CheckGhostObjectProperties,
   CheckDuplicateObjectIds,
   CheckDuplicateObjectGuids,
   CheckNodes,
   CheckInterfaces,
   CheckNetworkServices,
   CheckClusters,
   CheckContainerCycles,
   CheckDuplicateSubnets,
   CheckTemplateToTargetMapping,
   CheckObjectProperties,
   CheckContainerMembership,
   CheckEPP,
   CheckMapLinks,
   CheckDataTables,
   CheckDataCollectionItems,
   CheckDCISourceNodes,
   CheckTemplateBindings,
   CheckInterfacePeers,
   CheckNodePathCheckResults,
   CheckEventCodeReferences,
   CheckNotificationChannelReferences,
   CheckRawDciValues,
   CheckThresholds,
   CheckTableThresholds,
   CheckBusinessServiceCheckBindings,
   CheckBusinessServiceTicketServiceBindings,
   CheckBusinessServiceTicketCheckBindings,
   CheckBusinessServiceTicketHierarchy,
   CheckBusinessServiceCheckState,
   CheckBusinessServiceDowntime,
   CheckAssetNodeLinks,
   CheckOrphanRelations,
   CheckEppRelations
};

/**
 * Check database for errors
 */
void CheckDatabase()
{
   if (g_checkDataTablesOnly)
	   _tprintf(_T("Checking database (data tables only):\n"));
   else
	   _tprintf(_T("Checking database (%s collected data):\n"), g_checkData ? _T("including") : _T("excluding"));

   // Stale TimescaleDB extension registration breaks DDL statements with error that does not indicate real cause
   if (!ValidateTimescaleDBExtension() && !g_ignoreErrors)
   {
      _tprintf(_T("Database check aborted\n"));
      return;
   }

   // Get database format version
   int32_t major, minor;
   if (!DBGetSchemaVersion(g_dbHandle, &major, &minor))
   {
      _tprintf(_T("Unable to determine database schema version\n"));
      _tprintf(_T("Database check aborted\n"));
      return;
   }
   if ((major > DB_SCHEMA_VERSION_MAJOR) || ((major == DB_SCHEMA_VERSION_MAJOR) && (minor > DB_SCHEMA_VERSION_MINOR)))
   {
       _tprintf(_T("Your database has format version %d.%d, this tool is compiled for version %d.%d.\n")
                   _T("You need to upgrade your server before using this database.\n"),
                major, minor, DB_SCHEMA_VERSION_MAJOR, DB_SCHEMA_VERSION_MINOR);
       _tprintf(_T("Database check aborted\n"));
       return;
   }
   if ((major < DB_SCHEMA_VERSION_MAJOR) || ((major == DB_SCHEMA_VERSION_MAJOR) && (minor < DB_SCHEMA_VERSION_MINOR)))
   {
      _tprintf(_T("Your database has format version %d.%d, this tool is compiled for version %d.%d.\nUse \"upgrade\" command to upgrade your database first.\n"),
               major, minor, DB_SCHEMA_VERSION_MAJOR, DB_SCHEMA_VERSION_MINOR);
      _tprintf(_T("Database check aborted\n"));
      return;
   }
   if (!CheckModuleSchemaVersions())
   {
      _tprintf(_T("Database schema extensions version check failed\n"));
      _tprintf(_T("Database check aborted\n"));
      return;
   }

   bool completed = false;

   // Check if database is locked
   if (LockDatabase())
   {
      if (!DBBegin(g_dbHandle))
      {
         WriteToTerminal(L"Cannot start transaction\n");
         RemoveDatabaseLock();
         WriteToTerminal(L"Database check aborted\n");
         return;
      }

      if (g_checkDataTablesOnly)
      {
         CheckDataTables();
      }
      else
      {
         for(size_t i = 0; (i < sizeof(s_checkStages) / sizeof(s_checkStages[0])) && !s_checkAborted; i++)
            s_checkStages[i]();
         if (g_checkData && !s_checkAborted)
         {
            if (DBMgrMetaDataReadInt32(_T("SingleTablePerfData"), 0))
            {
               if (g_dbSyntax == DB_SYNTAX_TSDB)
               {
                  CheckCollectedDataSingleTable_TSDB(false);
                  CheckCollectedDataSingleTable_TSDB(true);
               }
               else
               {
                  CheckCollectedDataSingleTable(false);
                  CheckCollectedDataSingleTable(true);
               }
            }
            else
            {
               CheckCollectedData(false);
               CheckCollectedData(true);
            }
            CheckSampleAttributes();
         }
         if (!s_checkAborted)
            CheckModuleSchemas();
      }

      if (s_checkAborted)
      {
         WriteToTerminal(L"SQL error during check, rolling back changes...\n");
         DBRollback(g_dbHandle);
      }
      else if (g_dbCheckErrors == 0)
      {
         if (DBCommit(g_dbHandle))
         {
            _tprintf(_T("Database doesn't contain any errors\n"));
            completed = true;
         }
         else
         {
            WriteToTerminal(L"Cannot commit transaction\n");
         }
      }
      else
      {
         _tprintf(_T("%d errors was found, %d errors was corrected\n"), g_dbCheckErrors, g_dbCheckFixes);
         if (g_dbCheckFixes == g_dbCheckErrors)
            _tprintf(_T("All errors in database was fixed\n"));
         else
            _tprintf(_T("Database still contain errors\n"));
         completed = true;
         if (g_dbCheckFixes > 0)
         {
            if (GetYesNo(_T("Commit changes?")))
            {
               _tprintf(_T("Committing changes...\n"));
               if (DBCommit(g_dbHandle))
               {
                  _tprintf(_T("Changes was successfully committed to database\n"));
               }
               else
               {
                  WriteToTerminal(L"Cannot commit transaction\n");
                  completed = false;
               }
            }
            else
            {
               _tprintf(_T("Rolling back changes...\n"));
               if (DBRollback(g_dbHandle))
                  _tprintf(_T("All changes made to database was cancelled\n"));
            }
         }
         else
         {
            DBRollback(g_dbHandle);
         }
      }

      RemoveDatabaseLock();

      MemFree(s_dciCache);
      MemFree(s_tableDciCache);
   }

   _tprintf(_T("Database check %s\n"), completed ? _T("completed") : _T("aborted"));
}
