/*
** nxdbmgr - NetXMS database manager
** Copyright (C) 2004-2025 Victor Kirhenshtein
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
IntegerArray<uint32_t> *GetDataCollectionTargets()
{
   IntegerArray<uint32_t> *list = new IntegerArray<uint32_t>(128, 128);
   CollectObjectIdentifiers(_T("nodes"), list);
   CollectObjectIdentifiers(_T("clusters"), list);
   CollectObjectIdentifiers(_T("mobile_devices"), list);
   CollectObjectIdentifiers(_T("access_points"), list);
   CollectObjectIdentifiers(_T("chassis"), list);
   CollectObjectIdentifiers(_T("sensors"), list);
   CollectObjectIdentifiers(_T("object_containers WHERE object_class=29"), list);   // objects of class "collector"
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
static void CheckMissingObjectProperties(const TCHAR *table, const TCHAR *className, uint32_t builtinObjectId)
{
   TCHAR query[1024];
   _sntprintf(query, 1024, _T("SELECT o.id FROM %s o LEFT OUTER JOIN object_properties p ON p.object_id = o.id WHERE p.name IS NULL"), table);
   DB_RESULT hResult = SQLSelect(query);
   if (hResult == nullptr)
      return;

   int count = DBGetNumRows(hResult);
   for(int i = 0; i < count; i++)
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
               _T("status,is_deleted,is_system,inherit_access_rights,")
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
         if (SQLQuery(query))
            g_dbCheckFixes++;
      }
   }
   DBFreeResult(hResult);
}

/**
 * Check zone objects
 */
static void CheckZones()
{
   StartStage(_T("Zone object properties"));
   CheckMissingObjectProperties(_T("zones"), _T("zone"), 4);
   EndStage();
}

/**
 * Check node objects
 */
static void CheckNodes()
{
   StartStage(_T("Node object properties"));
   CheckMissingObjectProperties(_T("nodes"), _T("node"), 0);
   EndStage();

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
   _sntprintf(stageName, 256, _T("%s object properties"), pszDisplayName);
   StartStage(stageName);
   CheckMissingObjectProperties(pszTable, pszDisplayName, 0);
   EndStage();

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
 * Check access point objects
 */
static void CheckAccessPoints()
{
   StartStage(_T("Access point object properties"));
   CheckMissingObjectProperties(_T("access_points"), _T("access point"), 0);
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
   StartStage(_T("Cluster object properties"));
   CheckMissingObjectProperties(_T("clusters"), _T("cluster"), 0);
   EndStage();

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

   // Check source object ID's
   DB_RESULT hResult = SQLSelect(_T("SELECT object_id FROM policy_source_list"));
   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         uint32_t objectId = DBGetFieldULong(hResult, i, 0);
         _sntprintf(query, 1024, _T("SELECT object_id FROM object_properties WHERE object_id=%d"), objectId);
         if (!CheckResultSet(query))
         {
            g_dbCheckErrors++;
            if (GetYesNoEx(_T("Invalid object ID %d used in policy. Delete it from policy?"), objectId))
            {
               _sntprintf(query, 1024, _T("DELETE FROM policy_source_list WHERE object_id=%d"), objectId);
               if (SQLQuery(query))
                  g_dbCheckFixes++;
            }
         }
      }
      DBFreeResult(hResult);
   }

   // Check event ID's
   ResetBulkYesNo();
   hResult = SQLSelect(_T("SELECT event_code FROM policy_event_list"));
   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         uint32_t eventCode = DBGetFieldULong(hResult, i, 0);
         _sntprintf(query, 1024, _T("SELECT event_code FROM event_cfg WHERE event_code=%u"), eventCode);
         if (!CheckResultSet(query))
         {
            g_dbCheckErrors++;
            if (GetYesNoEx(_T("Invalid event code 0x%08X referenced in policy. Delete this reference?"), eventCode))
            {
               _sntprintf(query, 1024, _T("DELETE FROM policy_event_list WHERE event_code=%u"), eventCode);
               if (SQLQuery(query))
                  g_dbCheckFixes++;
            }
         }
      }
      DBFreeResult(hResult);
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
   TCHAR query[256], queryTemplate[256];
   DBMgrMetaDataReadStr(_T("IDataTableCreationCommand"), queryTemplate, 255, _T(""));
   _sntprintf(query, 256, queryTemplate, objectId);
   if (!SQLQuery(query))
		return false;

   for(int i = 0; i < 10; i++)
   {
      _sntprintf(query, 256, _T("IDataIndexCreationCommand_%d"), i);
      DBMgrMetaDataReadStr(query, queryTemplate, 255, _T(""));
      if (queryTemplate[0] != 0)
      {
         _sntprintf(query, 256, queryTemplate, objectId, objectId);
         if (!SQLQuery(query))
				return false;
      }
   }

	return true;
}

/**
 * Create tdata_xx table - pre V281 version
 */
bool CreateTDataTable_preV281(uint32_t objectId)
{
   TCHAR query[256], queryTemplate[256];
   DBMgrMetaDataReadStr(_T("TDataTableCreationCommand"), queryTemplate, 255, _T(""));
   _sntprintf(query, 256, queryTemplate, objectId);
   if (!SQLQuery(query))
		return false;

   for(int i = 0; i < 10; i++)
   {
      _sntprintf(query, 256, _T("TDataIndexCreationCommand_%d"), i);
      DBMgrMetaDataReadStr(query, queryTemplate, 255, _T(""));
      if (queryTemplate[0] != 0)
      {
         _sntprintf(query, 256, queryTemplate, objectId, objectId);
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
   TCHAR query[256], queryTemplate[256];

   for(int i = 0; i < 10; i++)
   {
      _sntprintf(query, 256, _T("TDataTableCreationCommand_%d"), i);
      DBMgrMetaDataReadStr(query, queryTemplate, 255, _T(""));
      if (queryTemplate[0] != 0)
      {
         _sntprintf(query, 256, queryTemplate, objectId, objectId);
         if (!SQLQuery(query))
		      return false;
      }
   }

   for(int i = 0; i < 10; i++)
   {
      _sntprintf(query, 256, _T("TDataIndexCreationCommand_%d"), i);
      DBMgrMetaDataReadStr(query, queryTemplate, 255, _T(""));
      if (queryTemplate[0] != 0)
      {
         _sntprintf(query, 256, queryTemplate, objectId, objectId);
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

	time_t now = time(nullptr);
	IntegerArray<uint32_t> *targets = GetDataCollectionTargets();
	SetStageWorkTotal(targets->size());
	for(int i = 0; i < targets->size(); i++)
   {
      UINT32 objectId = targets->get(i);
      TCHAR query[1024];
      _sntprintf(query, 1024, _T("SELECT count(*) FROM %s_%d WHERE %s_timestamp>") TIME_T_FMT,
               isTable ? _T("tdata") : _T("idata"), objectId, isTable ? _T("tdata") : _T("idata"), TIME_T_FCAST(now));
      DB_RESULT hResult = SQLSelect(query);
      if (hResult != NULL)
      {
         if (DBGetFieldLong(hResult, 0, 0) > 0)
         {
            g_dbCheckErrors++;
            if (GetYesNoEx(_T("Found collected data for node [%d] with timestamp in the future. Delete invalid records?"), objectId))
            {
               _sntprintf(query, 1024, _T("DELETE FROM %s_%d WHERE %s_timestamp>") TIME_T_FMT,
                        isTable ? _T("tdata") : _T("idata"), objectId, isTable ? _T("tdata") : _T("idata"), TIME_T_FCAST(now));
               if (SQLQuery(query))
                  g_dbCheckFixes++;
            }
         }
         DBFreeResult(hResult);
      }
   }

	ResetBulkYesNo();

   for(int i = 0; i < targets->size(); i++)
   {
      uint32_t objectId = targets->get(i);
      TCHAR query[1024];
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
	delete targets;

	EndStage();
}

/**
 * Check collected data - single table version
 */
static void CheckCollectedDataSingleTable(bool isTable)
{
   StartStage(isTable ? _T("Table DCI history records") : _T("DCI history records"), 2);

   time_t now = time(nullptr);
   TCHAR query[1024];
   _sntprintf(query, 1024, _T("SELECT count(*) FROM %s WHERE %s_timestamp>") TIME_T_FMT,
            isTable ? _T("tdata") : _T("idata"), isTable ? _T("tdata") : _T("idata"), TIME_T_FCAST(now));
   DB_RESULT hResult = SQLSelect(query);
   if (hResult != nullptr)
   {
      if (DBGetFieldLong(hResult, 0, 0) > 0)
      {
         g_dbCheckErrors++;
         if (GetYesNoEx(_T("Found collected data with timestamp in the future. Delete invalid records?")))
         {
            _sntprintf(query, 1024, _T("DELETE FROM %s WHERE %s_timestamp>") TIME_T_FMT,
                     isTable ? _T("tdata") : _T("idata"), isTable ? _T("tdata") : _T("idata"), TIME_T_FCAST(now));
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
            if (GetYesNoEx(_T("Found collected data for non-existing DCI [%d]. Delete invalid records?"), id))
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
 * Check raw DCI values
 */
static void CheckRawDciValues()
{
   StartStage(_T("Raw DCI values table"));

   time_t now = time(NULL);

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
            if (GetYesNoEx(_T("Found raw value record for non-existing DCI [%d]. Delete it?"), id))
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
   _sntprintf(query, 1024, _T("SELECT count(*) FROM raw_dci_values WHERE last_poll_time>") TIME_T_FMT, TIME_T_FCAST(now));
   hResult = SQLSelect(query);
   if (hResult != NULL)
   {
      if (DBGetFieldLong(hResult, 0, 0) > 0)
      {
         g_dbCheckErrors++;
         if (GetYesNoEx(_T("Found DCIs with last poll timestamp in the future. Fix it?")))
         {
            _sntprintf(query, 1024, _T("UPDATE raw_dci_values SET last_poll_time=") TIME_T_FMT _T(" WHERE last_poll_time>") TIME_T_FMT, TIME_T_FCAST(now), TIME_T_FCAST(now));
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
 * Check if given data table exist
 */
bool IsDataTableExist(const TCHAR *format, uint32_t id)
{
   TCHAR table[256];
   _sntprintf(table, 256, format, id);
   int rc = DBIsTableExist(g_dbHandle, table);
   if (rc == DBIsTableExist_Failure)
   {
      _tprintf(_T("WARNING: call to DBIsTableExist(\"%s\") failed\n"), table);
   }
   return rc != DBIsTableExist_NotFound;
}

/**
 * Check data tables
 */
static void CheckDataTables()
{
   if (DBMgrMetaDataReadInt32(_T("SingeTablePerfData"), 0))
      return;  // Single table mode

   StartStage(_T("Data tables"));
	IntegerArray<uint32_t> *targets = GetDataCollectionTargets();
	SetStageWorkTotal(targets->size());
	for(int i = 0; i < targets->size(); i++)
   {
	   uint32_t objectId = targets->get(i);

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
         const TCHAR *table = tables->get(i);
         if (!_tcsncmp(table, _T("idata_"), 6) || !_tcsncmp(table, _T("tdata_"), 6))
         {
            TCHAR *eptr;
            uint32_t objectId = _tcstoul(&table[6], &eptr, 10);
            if ((*eptr == 0) && !targets->contains(objectId))
            {
               g_dbCheckErrors++;
               if (GetYesNoEx(_T("Data collection table %s belongs to deleted object and no longer in use. Delete it? (Y/N) "), table))
               {
                  TCHAR query[256];
                  if (!_tcsncmp(table, _T("tdata_"), 6))
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

   delete targets;
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
 * Check business services
 */
static void CheckBusinessServices()
{
   StartStage(_T("Business services"));
   CheckMissingObjectProperties(_T("business_services"), _T("business service"), 0);
   EndStage();

   StartStage(_T("Business service prototypes"));
   CheckMissingObjectProperties(_T("business_service_prototypes"), _T("business service prototype"), 0);
   EndStage();
}

/**
 * Check assets
 */
static void CheckAssets()
{
   StartStage(_T("Assets"));
   CheckMissingObjectProperties(_T("assets"), _T("assets"), 0);
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
 * Check database for errors
 */
void CheckDatabase()
{
   if (g_checkDataTablesOnly)
	   _tprintf(_T("Checking database (data tables only):\n"));
   else
	   _tprintf(_T("Checking database (%s collected data):\n"), g_checkData ? _T("including") : _T("excluding"));

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
      DBBegin(g_dbHandle);

      if (g_checkDataTablesOnly)
      {
         CheckDataTables();
      }
      else
      {
         CheckZones();
         CheckNodes();
         CheckComponents(_T("Interface"), _T("interfaces"));
         CheckComponents(_T("Network service"), _T("network_services"));
         CheckClusters();
         CheckAccessPoints();
         CheckTemplateToTargetMapping();
         CheckBusinessServices();
         CheckObjectProperties();
         CheckContainerMembership();
         CheckEPP();
         CheckMapLinks();
         CheckDataTables();
         CheckDataCollectionItems();
         CheckRawDciValues();
         CheckThresholds();
         CheckTableThresholds();
         CheckBusinessServiceCheckBindings();
         CheckBusinessServiceTicketServiceBindings();
         CheckBusinessServiceTicketCheckBindings();
         CheckBusinessServiceTicketHierarchy();
         CheckBusinessServiceCheckState();
         CheckBusinessServiceDowntime();
         CheckAssets();
         CheckAssetNodeLinks();
         if (g_checkData)
         {
            if (DBMgrMetaDataReadInt32(_T("SingeTablePerfData"), 0))
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
         }
         CheckModuleSchemas();
      }

      if (g_dbCheckErrors == 0)
      {
         _tprintf(_T("Database doesn't contain any errors\n"));
         DBCommit(g_dbHandle);
      }
      else
      {
         _tprintf(_T("%d errors was found, %d errors was corrected\n"), g_dbCheckErrors, g_dbCheckFixes);
         if (g_dbCheckFixes == g_dbCheckErrors)
            _tprintf(_T("All errors in database was fixed\n"));
         else
            _tprintf(_T("Database still contain errors\n"));
         if (g_dbCheckFixes > 0)
         {
            if (GetYesNo(_T("Commit changes?")))
            {
               _tprintf(_T("Committing changes...\n"));
               if (DBCommit(g_dbHandle))
                  _tprintf(_T("Changes was successfully committed to database\n"));
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
      completed = true;

      RemoveDatabaseLock();

      MemFree(s_dciCache);
      MemFree(s_tableDciCache);
   }

   _tprintf(_T("Database check %s\n"), completed ? _T("completed") : _T("aborted"));
}
