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
** File: hasync.cpp
**
** Warm standby state synchronization (doc/HA_Design.md sections 3.3, 3.4).
** Applies change journal entries to the standby's in-memory object model and
** alarm list: an object change replaces the in-memory instance with a fresh
** load from the database, a tombstone detaches and drops the instance, and
** alarm entries reload the corresponding alarm row. Runs only on a quiescent
** standby (no pollers, no sessions, no event processing), which is what makes
** instance replacement safe.
**
** Relations are rebuilt the same way startup builds them: each class links
** its database-owned relations during loadFromDatabase()/postLoad(), and
** relations owned by the counterpart object are re-attached ("grafted") from
** the replaced instance's reference lists. A removed relation always journals
** both endpoints, so a stale graft is corrected when the owning side's entry
** is applied.
**
**/

#include "nxcore.h"
#include <nxcore_ha.h>
#include <map>

#define DEBUG_TAG L"ha.sync"

/**
 * Set when the warm state can no longer be trusted (apply failure, journal
 * truncated past our watermark, unsupported object class). A dirty standby
 * must not activate from its warm state; the activation path restarts the
 * process into a fresh standby instead.
 */
static std::atomic<bool> s_dirty(false);

/**
 * Mark warm state as dirty
 */
void HASyncSetDirty(const wchar_t *reason)
{
   if (!s_dirty.exchange(true))
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, L"Standby warm state marked as inconsistent (%s); this node will rebuild its state before activating", reason);
}

/**
 * Check if warm state is dirty
 */
bool HASyncIsDirty()
{
   return s_dirty.load();
}

/**
 * Object class load order, mirroring LoadObjects(): classes whose load path
 * links to other objects come after the classes they reference. Root classes
 * are handled separately (in-place refresh); unknown (module-provided)
 * classes are unsupported by live sync.
 */
static int GetClassLoadRank(int objectClass)
{
   switch(objectClass)
   {
      case OBJECT_ZONE: return 0;
      case OBJECT_CONDITION: return 1;
      case OBJECT_SUBNET: return 2;
      case OBJECT_RACK: return 3;
      case OBJECT_CHASSIS: return 4;
      case OBJECT_MOBILEDEVICE: return 5;
      case OBJECT_SENSOR: return 6;
      case OBJECT_CLOUDDOMAIN: return 7;
      case OBJECT_RESOURCE: return 8;
      case OBJECT_NODE: return 9;
      case OBJECT_WIRELESSDOMAIN: return 10;
      case OBJECT_ACCESSPOINT: return 11;
      case OBJECT_INTERFACE: return 12;
      case OBJECT_NETWORKSERVICE: return 13;
      case OBJECT_VPNCONNECTOR: return 14;
      case OBJECT_CLUSTER: return 15;
      case OBJECT_COLLECTOR: return 16;
      case OBJECT_CIRCUIT: return 17;
      case OBJECT_ASSET: return 18;
      case OBJECT_ASSETGROUP: return 19;
      case OBJECT_TEMPLATE: return 20;
      case OBJECT_NETWORKMAP: return 21;
      case OBJECT_CONTAINER: return 22;
      case OBJECT_TEMPLATEGROUP: return 23;
      case OBJECT_NETWORKMAPGROUP: return 24;
      case OBJECT_DASHBOARD: return 25;
      case OBJECT_DASHBOARDTEMPLATE: return 26;
      case OBJECT_DASHBOARDGROUP: return 27;
      case OBJECT_BUSINESSSERVICE: return 28;
      case OBJECT_BUSINESSSERVICEPROTO: return 29;
      default: return -1;
   }
}

/**
 * Create empty object instance for the given class (same constructors
 * LoadObjects() uses)
 */
static shared_ptr<NetObj> CreateObjectInstance(int objectClass)
{
   switch(objectClass)
   {
      case OBJECT_ZONE: return make_shared<Zone>();
      case OBJECT_CONDITION: return make_shared<ConditionObject>();
      case OBJECT_SUBNET: return make_shared<Subnet>();
      case OBJECT_RACK: return make_shared<Rack>();
      case OBJECT_CHASSIS: return make_shared<Chassis>();
      case OBJECT_MOBILEDEVICE: return make_shared<MobileDevice>();
      case OBJECT_SENSOR: return make_shared<Sensor>();
      case OBJECT_CLOUDDOMAIN: return make_shared<CloudDomain>();
      case OBJECT_RESOURCE: return make_shared<Resource>();
      case OBJECT_NODE: return make_shared<Node>();
      case OBJECT_WIRELESSDOMAIN: return make_shared<WirelessDomain>();
      case OBJECT_ACCESSPOINT: return make_shared<AccessPoint>();
      case OBJECT_INTERFACE: return make_shared<Interface>();
      case OBJECT_NETWORKSERVICE: return make_shared<NetworkService>();
      case OBJECT_VPNCONNECTOR: return make_shared<VPNConnector>();
      case OBJECT_CLUSTER: return make_shared<Cluster>();
      case OBJECT_COLLECTOR: return make_shared<Collector>();
      case OBJECT_CIRCUIT: return make_shared<Circuit>();
      case OBJECT_ASSET: return make_shared<Asset>();
      case OBJECT_ASSETGROUP: return make_shared<AssetGroup>();
      case OBJECT_TEMPLATE: return make_shared<Template>();
      case OBJECT_NETWORKMAP: return make_shared<NetworkMap>();
      case OBJECT_CONTAINER: return make_shared<Container>();
      case OBJECT_TEMPLATEGROUP: return make_shared<TemplateGroup>();
      case OBJECT_NETWORKMAPGROUP: return make_shared<NetworkMapGroup>();
      case OBJECT_DASHBOARD: return make_shared<Dashboard>();
      case OBJECT_DASHBOARDTEMPLATE: return make_shared<DashboardTemplate>();
      case OBJECT_DASHBOARDGROUP: return make_shared<DashboardGroup>();
      case OBJECT_BUSINESSSERVICE: return make_shared<BusinessService>();
      case OBJECT_BUSINESSSERVICEPROTO: return make_shared<BusinessServicePrototype>();
      default: return shared_ptr<NetObj>();
   }
}

/**
 * Check if the parent-child relation for the given parent class is loaded by
 * the parent (container_members read in the container's postLoad, cluster
 * member and template target lists read in loadFromDatabase). Relations not
 * listed here are established by the child object's own load path.
 */
static bool IsRelationLoadedByParent(int parentClass)
{
   switch(parentClass)
   {
      case OBJECT_SERVICEROOT:
      case OBJECT_TEMPLATEROOT:
      case OBJECT_NETWORKMAPROOT:
      case OBJECT_DASHBOARDROOT:
      case OBJECT_ASSETROOT:
      case OBJECT_BUSINESSSERVICEROOT:
      case OBJECT_CONTAINER:
      case OBJECT_COLLECTOR:
      case OBJECT_CIRCUIT:
      case OBJECT_WIRELESSDOMAIN:
      case OBJECT_TEMPLATEGROUP:
      case OBJECT_NETWORKMAPGROUP:
      case OBJECT_DASHBOARDGROUP:
      case OBJECT_ASSETGROUP:
      case OBJECT_CLUSTER:
      case OBJECT_TEMPLATE:
      case OBJECT_BUSINESSSERVICE:
      case OBJECT_BUSINESSSERVICEPROTO:
         return true;
      default:
         return false;
   }
}

/**
 * Get built-in root object for the given class (nullptr if not a root class)
 */
static shared_ptr<NetObj> GetRootObject(int objectClass)
{
   switch(objectClass)
   {
      case OBJECT_NETWORK: return g_entireNetwork;
      case OBJECT_SERVICEROOT: return g_infrastructureServiceRoot;
      case OBJECT_TEMPLATEROOT: return g_templateRoot;
      case OBJECT_NETWORKMAPROOT: return g_mapRoot;
      case OBJECT_DASHBOARDROOT: return g_dashboardRoot;
      case OBJECT_ASSETROOT: return g_assetRoot;
      case OBJECT_BUSINESSSERVICEROOT: return g_businessServiceRoot;
      default: return shared_ptr<NetObj>();
   }
}

/**
 * Apply object tombstone: detach the instance from the object graph and drop
 * it from all indexes. Objects the deletion mutated on the active node
 * (former parents, peers, referencing objects) arrive through their own
 * journal entries.
 */
static void ApplyObjectTombstone(uint32_t objectId)
{
   shared_ptr<NetObj> object = g_idxObjectById.get(objectId);
   if (object == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 6, L"Tombstone for object [%u] which is not in memory", objectId);
      return;
   }
   nxlog_debug_tag(DEBUG_TAG, 5, L"Applying tombstone for object %s [%u]", object->getName(), objectId);
   NetObjDeleteFromIndexes(*object);
   object->detachForClusterSync(true);
   g_idxObjectById.remove(objectId);
}

/**
 * Refresh built-in root object in place: common properties, ACL and (for
 * container-like roots) the member list, reconciled against the database
 */
static bool ResyncRootObject(const shared_ptr<NetObj>& root, DB_HANDLE hdb, DB_STATEMENT *preparedStatements)
{
   nxlog_debug_tag(DEBUG_TAG, 5, L"Refreshing root object %s [%u]", root->getName(), root->getId());
   if (!root->resyncCommonDataForClusterSync(hdb, preparedStatements))
      return false;

   if (root->getObjectClass() == OBJECT_NETWORK)
      return true;   // network children (zones/subnets) attach themselves

   // Reconcile member list against container_members
   wchar_t query[128];
   nx_swprintf(query, 128, L"SELECT object_id FROM container_members WHERE container_id=%u", root->getId());
   DB_RESULT hResult = DBSelect(hdb, query);
   if (hResult == nullptr)
      return false;

   IntegerArray<uint32_t> memberList;
   HashSet<uint32_t> memberSet;
   int count = DBGetNumRows(hResult);
   for(int i = 0; i < count; i++)
   {
      uint32_t memberId = DBGetFieldUInt32(hResult, i, 0);
      memberList.add(memberId);
      memberSet.put(memberId);
   }
   DBFreeResult(hResult);

   unique_ptr<SharedObjectArray<NetObj>> children = root->getChildren();
   for(int i = 0; i < children->size(); i++)
   {
      NetObj *child = children->get(i);
      if (!memberSet.contains(child->getId()))
         NetObj::unlinkObjects(root.get(), child);
   }
   for(int i = 0; i < memberList.size(); i++)
   {
      shared_ptr<NetObj> member = FindObjectById(memberList.get(i));
      if ((member != nullptr) && !member->isDeleted())
         NetObj::linkObjects(root, member);   // no-op when already linked
   }
   return true;
}

/**
 * Link parent-owned relations of the given object from their database
 * tables (container_members for the container family and roots,
 * cluster_members and dct_node_map for data collection targets). Driven by
 * the database rather than by grafting from the replaced instance so that a
 * newly created object gets its parents even when the parent's own journal
 * entry was applied in an earlier round.
 */
static void LinkParentOwnedRelations(const shared_ptr<NetObj>& object, DB_HANDLE hdb)
{
   const wchar_t *queries[3];
   int count = 0;
   queries[count++] = L"SELECT container_id FROM container_members WHERE object_id=?";
   if (object->getObjectClass() == OBJECT_NODE)
      queries[count++] = L"SELECT cluster_id FROM cluster_members WHERE node_id=?";
   if (object->isDataCollectionTarget())
      queries[count++] = L"SELECT template_id FROM dct_node_map WHERE node_id=?";

   for(int i = 0; i < count; i++)
   {
      DB_STATEMENT hStmt = DBPrepare(hdb, queries[i]);
      if (hStmt == nullptr)
         continue;
      DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, object->getId());
      DB_RESULT hResult = DBSelectPrepared(hStmt);
      if (hResult != nullptr)
      {
         int rows = DBGetNumRows(hResult);
         for(int row = 0; row < rows; row++)
         {
            shared_ptr<NetObj> parent = FindObjectById(DBGetFieldUInt32(hResult, row, 0));
            if ((parent != nullptr) && !parent->isDeleted())
               NetObj::linkObjects(parent, object);   // no-op when already linked
         }
         DBFreeResult(hResult);
      }
      DBFreeStatement(hStmt);
   }
}

/**
 * Replaced-object context for the post-load graft pass
 */
struct ObjectSwapContext
{
   shared_ptr<NetObj> object;
   std::vector<std::pair<uint32_t, int>> oldChildren;   // id, class
};

/**
 * Apply a batch of object journal entries. Tombstones first, then changes in
 * class load order (mirroring LoadObjects()), then relation grafts and
 * post-load hooks once every replacement instance is in the indexes.
 */
static void ApplyObjectChanges(std::vector<HAJournalEntry>& entries)
{
   // Keep only the latest entry per object
   std::map<uint32_t, HAJournalEntry> latest;
   for(const HAJournalEntry& e : entries)
   {
      auto it = latest.find(e.entityId);
      if ((it == latest.end()) || (it->second.seq < e.seq))
         latest[e.entityId] = e;
   }

   std::vector<HAJournalEntry> changes;
   for(auto& p : latest)
   {
      if (p.second.changeType == HAJournalChangeType::DELETE)
         ApplyObjectTombstone(p.second.entityId);
      else
         changes.push_back(p.second);
   }
   if (changes.empty())
      return;

   std::sort(changes.begin(), changes.end(),
      [] (const HAJournalEntry& a, const HAJournalEntry& b) -> bool
      {
         int ra = GetClassLoadRank(a.entityClass);
         int rb = GetClassLoadRank(b.entityClass);
         return (ra != rb) ? (ra < rb) : (a.seq < b.seq);
      });

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_STATEMENT preparedStatements[LSI_MAX_VALUE];
   memset(preparedStatements, 0, sizeof(preparedStatements));

   std::vector<ObjectSwapContext> swaps;
   std::vector<shared_ptr<NetObj>> changedRoots;
   for(const HAJournalEntry& e : changes)
   {
      shared_ptr<NetObj> root = GetRootObject(e.entityClass);
      if (root != nullptr)
      {
         changedRoots.push_back(root);   // refreshed after the swap pass so new members are in the indexes
         continue;
      }

      if (GetClassLoadRank(e.entityClass) < 0)
      {
         wchar_t reason[128];
         nx_swprintf(reason, 128, L"object class %d is not supported by live synchronization", e.entityClass);
         HASyncSetDirty(reason);
         continue;
      }

      ObjectSwapContext context;
      shared_ptr<NetObj> old = g_idxObjectById.get(e.entityId);
      if (old != nullptr)
      {
         unique_ptr<SharedObjectArray<NetObj>> children = old->getChildren();
         for(int i = 0; i < children->size(); i++)
            context.oldChildren.emplace_back(children->get(i)->getId(), children->get(i)->getObjectClass());
         NetObjDeleteFromIndexes(*old);
         old->detachForClusterSync(false);
         g_idxObjectById.remove(e.entityId);
      }

      shared_ptr<NetObj> object = CreateObjectInstance(e.entityClass);
      if (!object->loadFromDatabase(hdb, e.entityId, preparedStatements))
      {
         object->destroy();
         if (IsDatabaseRecordExist(hdb, L"object_properties", L"object_id", e.entityId))
         {
            wchar_t reason[128];
            nx_swprintf(reason, 128, L"failed to reload object [%u] of class %d", e.entityId, e.entityClass);
            HASyncSetDirty(reason);
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG, 5, L"Object [%u] deleted concurrently with reload", e.entityId);
         }
         continue;
      }
      nxlog_debug_tag(DEBUG_TAG, 5, L"Object %s [%u] %s from database", object->getName(), e.entityId, (old != nullptr) ? L"reloaded" : L"loaded");
      NetObjInsert(object, false, false);
      context.object = object;
      swaps.push_back(std::move(context));
   }

   // Re-attach relations: parent-owned ones from their database tables,
   // child-owned ones of unchanged children by grafting from the replaced
   // instance's child list, then re-create the links LoadObjects()
   // establishes outside object load code
   for(ObjectSwapContext& context : swaps)
   {
      NetObj *object = context.object.get();
      if (object->isDeleted())
         continue;

      LinkParentOwnedRelations(context.object, hdb);
      if (!IsRelationLoadedByParent(object->getObjectClass()))
      {
         for(auto& child : context.oldChildren)
         {
            shared_ptr<NetObj> c = FindObjectById(child.first);
            if ((c != nullptr) && !c->isDeleted())
               NetObj::linkObjects(context.object, c);
         }
      }

      switch(object->getObjectClass())
      {
         case OBJECT_ZONE:
            NetObj::linkObjects(g_entireNetwork, context.object);
            break;
         case OBJECT_SUBNET:
            if (IsZoningEnabled())
            {
               shared_ptr<Zone> zone = FindZoneByUIN(static_cast<Subnet*>(object)->getZoneUIN());
               if (zone != nullptr)
                  NetObj::linkObjects(zone, context.object);
            }
            else
            {
               NetObj::linkObjects(g_entireNetwork, context.object);
            }
            break;
         case OBJECT_NODE:
            if (IsZoningEnabled())
            {
               shared_ptr<Zone> zone = FindZoneByProxyId(object->getId());
               if (zone != nullptr)
                  zone->updateProxyStatus(static_pointer_cast<Node>(context.object), false);
            }
            break;
      }
   }

   // Refresh changed root objects once every replacement instance is in the
   // indexes (member reconciliation must be able to resolve new members)
   for(shared_ptr<NetObj>& root : changedRoots)
   {
      if (!ResyncRootObject(root, hdb, preparedStatements))
         HASyncSetDirty(L"root object refresh failed");
   }

   // Post-load hooks run after all links exist, same as at startup
   for(ObjectSwapContext& context : swaps)
   {
      context.object->postLoad();
      context.object->clearInheritedAccessCache();   // subtree may cache rights inherited through the old instance
   }

   for(int i = 0; i < LSI_MAX_VALUE; i++)
      DBFreeStatement(preparedStatements[i]);
   DBConnectionPoolReleaseConnection(hdb);
}

/**
 * Apply one round of journal entries to the warm standby state (called by the
 * channel applier thread and by the activation-time final replay). Objects
 * are applied before alarms so a new alarm's source object exists.
 */
void HASyncApplyJournalBatch(const std::vector<HAJournalEntry>& entries)
{
   std::vector<HAJournalEntry> objectEntries;
   for(const HAJournalEntry& e : entries)
      if (e.entityType == HAJournalEntityType::OBJECT)
         objectEntries.push_back(e);
   if (!objectEntries.empty())
      ApplyObjectChanges(objectEntries);

   for(const HAJournalEntry& e : entries)
   {
      if (e.entityType == HAJournalEntityType::ALARM)
         SyncAlarmFromDatabase(e.entityId);   // handles reload and removal
   }
}
