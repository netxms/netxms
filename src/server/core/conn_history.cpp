/*
** NetXMS - Network Management System
** Copyright (C) 2003-2026 Raden Solutions
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
** File: conn_history.cpp
**
**/

#include "nxcore.h"
#include <nms_topo.h>

#define DEBUG_TAG _T("topology.history")

/**
 * Connection history event types
 */
enum ConnectionHistoryEventType
{
   CONN_EVENT_CONNECT = 0,
   CONN_EVENT_DISCONNECT = 1,
   CONN_EVENT_MOVE = 2
};

/**
 * Connection point info used for diffing
 */
struct ConnectionPoint
{
   uint32_t ifIndex;
   uint16_t vlanId;
};

/**
 * Connection history change record
 */
struct ConnectionHistoryChange
{
   MacAddress macAddress;
   ConnectionHistoryEventType eventType;
   uint32_t ifIndex;
   uint16_t vlanId;
   uint32_t oldIfIndex;
   uint16_t oldVlanId;
   uint32_t oldSwitchId;        // Old switch node ID for cross-switch moves (0 for same-switch)
   uint32_t oldInterfaceObjId;  // Old interface object ID for cross-switch moves (from DB)
};

/**
 * Extract connection points from FDB (MACs that are the only entry on their port).
 * Ports without interface object (for example rejected by Hook::CreateInterface) and
 * interfaces excluded from topology are ignored.
 */
static void ExtractConnectionPoints(const shared_ptr<ForwardingDatabase>& fdb, const Node& node, HashMap<MacAddress, ConnectionPoint> *connections)
{
   if (fdb == nullptr)
      return;

   HashSet<uint32_t> checkedPorts;

   for (int i = 0; i < fdb->getSize(); i++)
   {
      ForwardingDatabaseEntry *entry = fdb->getEntry(i);
      if (entry->ifIndex == 0)
         continue;

      if (checkedPorts.contains(entry->ifIndex))
         continue;
      checkedPorts.put(entry->ifIndex);

      shared_ptr<Interface> iface = node.findInterfaceByIndex(entry->ifIndex);
      if (iface == nullptr)
      {
         nxlog_debug_tag(DEBUG_TAG, 7, L"ExtractConnectionPoints(%u): ignoring port %u (no interface object)", node.getId(), entry->ifIndex);
         continue;
      }
      if (iface->isExcludedFromTopology())
      {
         nxlog_debug_tag(DEBUG_TAG, 7, L"ExtractConnectionPoints(%u): ignoring port %u (interface %s excluded from topology)", node.getId(), entry->ifIndex, iface->getName());
         continue;
      }

      MacAddress mac;
      if (fdb->isSingleMacOnPort(entry->ifIndex, &mac))
      {
         // Find the actual FDB entry matching this MAC to get vlanId
         uint16_t vlanId = 0;
         for (int j = 0; j < fdb->getSize(); j++)
         {
            ForwardingDatabaseEntry *e = fdb->getEntry(j);
            if ((e->ifIndex == entry->ifIndex) && e->macAddr.equals(mac))
            {
               vlanId = e->vlanId;
               break;
            }
         }

         auto *cp = new ConnectionPoint();
         cp->ifIndex = entry->ifIndex;
         cp->vlanId = vlanId;
         connections->set(mac, cp);
      }
   }
}

/**
 * Reconcile detected changes with connection history database.
 * The cached previous FDB on a switch can be stale when a MAC moved to another switch
 * and back between polls, leading to three types of errors:
 * - A CONNECT may actually be a cross-switch MOVE (MAC was on another switch)
 * - A same-switch MOVE may actually be a cross-switch MOVE (stale prevFdb still had the MAC
 *   on an old port, but the MAC was actually on a different switch)
 * - A DISCONNECT may be spurious (the cross-switch move was already recorded by the other switch)
 *
 * This function queries the most recent history record for each affected MAC and corrects
 * these cases when the record shows the MAC was last connected on a different switch.
 */
static void ReconcileChangesWithHistory(ObjectArray<ConnectionHistoryChange> *changes, DB_HANDLE hdb, uint32_t switchId)
{
   // Secondary sort by record_id ensures deterministic ordering when timestamps are equal
   const wchar_t *query;
   switch(g_dbSyntax)
   {
      case DB_SYNTAX_MSSQL:
         query = L"SELECT TOP 1 switch_id,interface_id,event_type FROM connection_history WHERE mac_address=? ORDER BY event_timestamp DESC,record_id DESC";
         break;
      case DB_SYNTAX_ORACLE:
         query = L"SELECT * FROM (SELECT switch_id,interface_id,event_type FROM connection_history WHERE mac_address=? ORDER BY event_timestamp DESC,record_id DESC) WHERE ROWNUM<=1";
         break;
      default:
         query = L"SELECT switch_id,interface_id,event_type FROM connection_history WHERE mac_address=? ORDER BY event_timestamp DESC,record_id DESC LIMIT 1";
         break;
   }

   DB_STATEMENT hStmt = DBPrepare(hdb, query);
   if (hStmt == nullptr)
      return;

   int upgraded = 0;
   int suppressed = 0;
   for (int i = 0; i < changes->size(); i++)
   {
      ConnectionHistoryChange *change = changes->get(i);
      bool isConnect = (change->eventType == CONN_EVENT_CONNECT);
      bool isSameSwitchMove = (change->eventType == CONN_EVENT_MOVE) && (change->oldSwitchId == 0);
      bool isDisconnect = (change->eventType == CONN_EVENT_DISCONNECT);
      if (!isConnect && !isSameSwitchMove && !isDisconnect)
         continue;

      DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, change->macAddress);
      DB_RESULT hResult = DBSelectPrepared(hStmt);
      if (hResult != nullptr)
      {
         bool suppress = false;
         if (DBGetNumRows(hResult) > 0)
         {
            uint32_t prevSwitchId = DBGetFieldUInt32(hResult, 0, 0);
            uint32_t prevInterfaceId = DBGetFieldUInt32(hResult, 0, 1);
            int32_t prevEventType = DBGetFieldInt32(hResult, 0, 2);

            // The MAC was last seen connected (not disconnected) on a different switch
            if ((prevEventType != CONN_EVENT_DISCONNECT) && (prevSwitchId != switchId))
            {
               wchar_t macStr[64];
               if (isDisconnect)
               {
                  // MAC already recorded as connected on a different switch - this
                  // disconnect is from a stale cached FDB and should be suppressed
                  nxlog_debug_tag(DEBUG_TAG, 4, L"ReconcileChangesWithHistory(%u): DISCONNECT for %s suppressed (MAC already on switch %u)",
                     switchId, change->macAddress.toString(macStr), prevSwitchId);
                  suppress = true;
               }
               else
               {
                  // Upgrade CONNECT or stale same-switch MOVE to cross-switch MOVE
                  change->eventType = CONN_EVENT_MOVE;
                  change->oldSwitchId = prevSwitchId;
                  change->oldInterfaceObjId = prevInterfaceId;
                  change->oldIfIndex = 0;
                  change->oldVlanId = 0;
                  upgraded++;
                  nxlog_debug_tag(DEBUG_TAG, 4, L"ReconcileChangesWithHistory(%u): %s for %s upgraded to cross-switch MOVE (from switch %u iface [%u])",
                     switchId, isConnect ? L"CONNECT" : L"same-switch MOVE", change->macAddress.toString(macStr), prevSwitchId, prevInterfaceId);
               }
            }
         }
         DBFreeResult(hResult);
         if (suppress)
         {
            changes->remove(i);
            i--;
            suppressed++;
         }
      }
   }
   DBFreeStatement(hStmt);

   if (upgraded > 0 || suppressed > 0)
      nxlog_debug_tag(DEBUG_TAG, 4, L"ReconcileChangesWithHistory(%u): %d events upgraded to cross-switch MOVE, %d spurious DISCONNECTs suppressed", switchId, upgraded, suppressed);
}

/**
 * Resolve IP address for a MAC address
 */
static InetAddress ResolveIPForMAC(const MacAddress& mac)
{
   shared_ptr<Interface> iface = FindInterfaceByMAC(mac);
   return (iface != nullptr) ? iface->getFirstIpAddress() : InetAddress();
}

/**
 * Write connection history record to database
 */
static void WriteHistoryRecord(DB_STATEMENT hStmt, uint32_t switchId, const ConnectionHistoryChange& change)
{
   uint32_t recordId = CreateUniqueId(IDG_CONNECTION_HISTORY);
   time_t now = time(nullptr);

   // Resolve IP and node
   InetAddress ip = ResolveIPForMAC(change.macAddress);
   shared_ptr<Node> node = FindNodeByMAC(change.macAddress);
   uint32_t nodeId = (node != nullptr) ? node->getId() : 0;

   // Find interface object ID
   shared_ptr<NetObj> switchObj = FindObjectById(switchId, OBJECT_NODE);
   uint32_t interfaceObjId = 0;
   if (switchObj != nullptr)
   {
      shared_ptr<Interface> iface = static_cast<Node*>(switchObj.get())->findInterfaceByIndex(change.ifIndex);
      if (iface != nullptr)
         interfaceObjId = iface->getId();
   }

   // Resolve old interface and switch for MOVE events
   uint32_t oldSwitchId = 0;
   uint32_t oldInterfaceObjId = 0;
   if (change.eventType == CONN_EVENT_MOVE)
   {
      if (change.oldSwitchId != 0)
      {
         // Cross-switch move - old interface object ID comes from DB lookup
         oldSwitchId = change.oldSwitchId;
         oldInterfaceObjId = change.oldInterfaceObjId;
      }
      else if (change.oldIfIndex != 0 && switchObj != nullptr)
      {
         // Same-switch move - resolve old ifIndex to object ID on current switch
         shared_ptr<Interface> oldIface = static_cast<Node*>(switchObj.get())->findInterfaceByIndex(change.oldIfIndex);
         if (oldIface != nullptr)
            oldInterfaceObjId = oldIface->getId();
      }
   }

   DBBind(hStmt, 1, DB_SQLTYPE_BIGINT, static_cast<int64_t>(recordId));
   DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, static_cast<int32_t>(now));
   DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, change.macAddress);
   wchar_t ipStr[64];
   DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, ip.isValid() && !ip.isAnyLocal() ? ip.toString(ipStr) : nullptr, DB_BIND_STATIC);
   DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, nodeId);
   DBBind(hStmt, 6, DB_SQLTYPE_INTEGER, switchId);
   DBBind(hStmt, 7, DB_SQLTYPE_INTEGER, interfaceObjId);
   DBBind(hStmt, 8, DB_SQLTYPE_INTEGER, static_cast<int32_t>(change.vlanId));
   DBBind(hStmt, 9, DB_SQLTYPE_INTEGER, static_cast<int32_t>(change.eventType));
   DBBind(hStmt, 10, DB_SQLTYPE_INTEGER, oldSwitchId);
   DBBind(hStmt, 11, DB_SQLTYPE_INTEGER, oldInterfaceObjId);
   DBExecute(hStmt);
}

/**
 * Post connection history event
 */
static void PostConnectionEvent(uint32_t switchId, const ConnectionHistoryChange& change)
{
   shared_ptr<NetObj> switchObj = FindObjectById(switchId, OBJECT_NODE);
   if (switchObj == nullptr)
      return;

   InetAddress ip = ResolveIPForMAC(change.macAddress);

   // Get port name
   wchar_t portName[MAX_OBJECT_NAME] = L"";
   shared_ptr<Interface> iface = static_cast<Node*>(switchObj.get())->findInterfaceByIndex(change.ifIndex);
   if (iface != nullptr)
      _tcslcpy(portName, iface->getName(), MAX_OBJECT_NAME);

   switch (change.eventType)
   {
      case CONN_EVENT_CONNECT:
         EventBuilder(EVENT_MAC_CONNECTED, switchId)
            .param(L"macAddress", change.macAddress)
            .param(L"switchName", switchObj->getName())
            .param(L"portName", portName)
            .param(L"ipAddress", ip)
            .param(L"vlanId", static_cast<int32_t>(change.vlanId))
            .post();
         break;

      case CONN_EVENT_DISCONNECT:
         EventBuilder(EVENT_MAC_DISCONNECTED, switchId)
            .param(L"macAddress", change.macAddress)
            .param(L"switchName", switchObj->getName())
            .param(L"portName", portName)
            .param(L"ipAddress", ip)
            .param(L"vlanId", static_cast<int32_t>(change.vlanId))
            .post();
         break;

      case CONN_EVENT_MOVE:
      {
         wchar_t oldPortName[MAX_OBJECT_NAME] = L"";
         const wchar_t *oldSwitchName = switchObj->getName();
         shared_ptr<NetObj> oldSwitchObj;

         if (change.oldSwitchId != 0)
         {
            // Cross-switch move - resolve old switch and interface from object IDs
            oldSwitchObj = FindObjectById(change.oldSwitchId, OBJECT_NODE);
            if (oldSwitchObj != nullptr)
               oldSwitchName = oldSwitchObj->getName();
            if (change.oldInterfaceObjId != 0)
            {
               shared_ptr<NetObj> oldIfaceObj = FindObjectById(change.oldInterfaceObjId, OBJECT_INTERFACE);
               if (oldIfaceObj != nullptr)
                  wcslcpy(oldPortName, oldIfaceObj->getName(), MAX_OBJECT_NAME);
            }
         }
         else
         {
            // Same-switch move - resolve old interface by ifIndex on current switch
            shared_ptr<Interface> oldIface = static_cast<Node*>(switchObj.get())->findInterfaceByIndex(change.oldIfIndex);
            if (oldIface != nullptr)
               wcslcpy(oldPortName, oldIface->getName(), MAX_OBJECT_NAME);
         }

         EventBuilder(EVENT_MAC_MOVED, switchId)
            .param(L"macAddress", change.macAddress)
            .param(L"switchName", switchObj->getName())
            .param(L"oldPortName", oldPortName)
            .param(L"newPortName", portName)
            .param(L"ipAddress", ip)
            .param(L"vlanId", static_cast<int32_t>(change.vlanId))
            .param(L"oldSwitchName", oldSwitchName)
            .param(L"oldSwitchId", (change.oldSwitchId != 0) ? change.oldSwitchId : switchId)
            .post();
         break;
      }
   }
}

/**
 * Update connection history for a switch node
 * Called during topology poll after FDB is collected
 */
void UpdateConnectionHistory(uint32_t switchId, const shared_ptr<ForwardingDatabase>& newFdb, const shared_ptr<ForwardingDatabase>& prevFdb)
{
   if (!ConfigReadBoolean(L"ConnectionHistory.EnableCollection", true))
      return;

   // First poll — no previous state to compare against
   if (prevFdb == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, L"UpdateConnectionHistory(%u): no previous FDB, skipping (first poll)", switchId);
      return;
   }

   if (newFdb == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, L"UpdateConnectionHistory(%u): new FDB is null, skipping", switchId);
      return;
   }

   shared_ptr<Node> switchNode = static_pointer_cast<Node>(FindObjectById(switchId, OBJECT_NODE));
   if (switchNode == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, L"UpdateConnectionHistory(%u): switch node object not found, skipping", switchId);
      return;
   }

   // Extract connection points from both FDBs
   HashMap<MacAddress, ConnectionPoint> newConnections(Ownership::True);
   HashMap<MacAddress, ConnectionPoint> prevConnections(Ownership::True);
   ExtractConnectionPoints(newFdb, *switchNode, &newConnections);
   ExtractConnectionPoints(prevFdb, *switchNode, &prevConnections);

   nxlog_debug_tag(DEBUG_TAG, 5, L"UpdateConnectionHistory(%u): new=%d prev=%d connection points",
      switchId, newConnections.size(), prevConnections.size());

   // Guard against FDB clear (switch restart): if >80% of connection points disappeared, log warning
   if ((prevConnections.size() > 10) && (newConnections.size() == 0))
   {
      nxlog_debug_tag(DEBUG_TAG, 3, L"UpdateConnectionHistory(%u): all %d connection points disappeared, possible switch restart",
         switchId, prevConnections.size());
   }

   // Build change list
   ObjectArray<ConnectionHistoryChange> changes(16, 16, Ownership::True);

   // Check for CONNECT and MOVE events
   newConnections.forEach(
      [&prevConnections, &changes] (const MacAddress& mac, ConnectionPoint *cp) -> EnumerationCallbackResult
      {
         ConnectionPoint *prevCp = prevConnections.get(mac);
         if (prevCp == nullptr)
         {
            // New connection (may be upgraded to cross-switch MOVE later)
            auto *change = new ConnectionHistoryChange();
            change->macAddress = mac;
            change->eventType = CONN_EVENT_CONNECT;
            change->ifIndex = cp->ifIndex;
            change->vlanId = cp->vlanId;
            change->oldIfIndex = 0;
            change->oldVlanId = 0;
            change->oldSwitchId = 0;
            change->oldInterfaceObjId = 0;
            changes.add(change);
         }
         else if (prevCp->ifIndex != cp->ifIndex)
         {
            // MAC moved to different port on same switch
            auto *change = new ConnectionHistoryChange();
            change->macAddress = mac;
            change->eventType = CONN_EVENT_MOVE;
            change->ifIndex = cp->ifIndex;
            change->vlanId = cp->vlanId;
            change->oldIfIndex = prevCp->ifIndex;
            change->oldVlanId = prevCp->vlanId;
            change->oldSwitchId = 0;
            change->oldInterfaceObjId = 0;
            changes.add(change);
         }
         return _CONTINUE;
      });

   // Check for DISCONNECT events
   prevConnections.forEach(
      [&newConnections, &changes] (const MacAddress& mac, ConnectionPoint *cp) -> EnumerationCallbackResult
      {
         if (newConnections.get(mac) == nullptr)
         {
            auto *change = new ConnectionHistoryChange();
            change->macAddress = mac;
            change->eventType = CONN_EVENT_DISCONNECT;
            change->ifIndex = cp->ifIndex;
            change->vlanId = cp->vlanId;
            change->oldIfIndex = 0;
            change->oldVlanId = 0;
            change->oldSwitchId = 0;
            change->oldInterfaceObjId = 0;
            changes.add(change);
         }
         return _CONTINUE;
      });

   if (changes.size() == 0)
   {
      nxlog_debug_tag(DEBUG_TAG, 6, L"UpdateConnectionHistory(%u): no changes detected", switchId);
      return;
   }

   nxlog_debug_tag(DEBUG_TAG, 4, L"UpdateConnectionHistory(%u): %d changes detected", switchId, changes.size());

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   // Reconcile changes with history to detect cross-switch moves and suppress stale events
   ReconcileChangesWithHistory(&changes, hdb, switchId);

   // Write changes to database in a batch
   DB_STATEMENT hStmt = DBPrepare(hdb,
      (g_dbSyntax == DB_SYNTAX_TSDB) ?
         L"INSERT INTO connection_history (record_id,event_timestamp,mac_address,ip_address,"
         L"node_id,switch_id,interface_id,vlan_id,event_type,old_switch_id,old_interface_id) "
         L"VALUES (?,to_timestamp(?),?,?,?,?,?,?,?,?,?)" :
         L"INSERT INTO connection_history (record_id,event_timestamp,mac_address,ip_address,"
         L"node_id,switch_id,interface_id,vlan_id,event_type,old_switch_id,old_interface_id) "
         L"VALUES (?,?,?,?,?,?,?,?,?,?,?)");
   if (hStmt != nullptr)
   {
      DBBegin(hdb);
      for (int i = 0; i < changes.size(); i++)
      {
         WriteHistoryRecord(hStmt, switchId, *changes.get(i));
      }
      DBCommit(hdb);
      DBFreeStatement(hStmt);
   }
   DBConnectionPoolReleaseConnection(hdb);

   // Post events
   for (int i = 0; i < changes.size(); i++)
   {
      PostConnectionEvent(switchId, *changes.get(i));
   }
}

/**
 * Get connection history from database
 */
void GetConnectionHistory(const NXCPMessage& request, NXCPMessage *response)
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   StringBuffer query((g_dbSyntax == DB_SYNTAX_TSDB) ?
      L"SELECT record_id,date_part('epoch',event_timestamp)::int,mac_address,ip_address,node_id,"
      L"switch_id,interface_id,vlan_id,event_type,old_switch_id,old_interface_id "
      L"FROM connection_history WHERE 1=1" :
      L"SELECT record_id,event_timestamp,mac_address,ip_address,node_id,"
      L"switch_id,interface_id,vlan_id,event_type,old_switch_id,old_interface_id "
      L"FROM connection_history WHERE 1=1");

   if (request.isFieldExist(VID_MAC_ADDR))
   {
      MacAddress macAddr = request.getFieldAsMacAddress(VID_MAC_ADDR);
      if (macAddr.isValid())
      {
         query.append(L" AND mac_address='");
         query.append(macAddr.toString(MacAddressNotation::FLAT_STRING));
         query.append(L"'");
      }
   }

   uint32_t objectId = request.getFieldAsUInt32(VID_OBJECT_ID);
   if (objectId != 0)
   {
      query.append(L" AND (switch_id=");
      query.append(objectId);
      query.append(L" OR node_id=");
      query.append(objectId);
      query.append(L")");
   }

   uint32_t interfaceId = request.getFieldAsUInt32(VID_INTERFACE_ID);
   if (interfaceId != 0)
   {
      query.append(L" AND interface_id=");
      query.append(interfaceId);
   }

   time_t timeFrom = static_cast<time_t>(request.getFieldAsUInt32(VID_TIME_FROM));
   if (timeFrom != 0)
   {
      if (g_dbSyntax == DB_SYNTAX_TSDB)
      {
         query.append(L" AND event_timestamp>=to_timestamp(");
         query.append(static_cast<int64_t>(timeFrom));
         query.append(L")");
      }
      else
      {
         query.append(L" AND event_timestamp>=");
         query.append(static_cast<int64_t>(timeFrom));
      }
   }

   time_t timeTo = static_cast<time_t>(request.getFieldAsUInt32(VID_TIME_TO));
   if (timeTo != 0)
   {
      if (g_dbSyntax == DB_SYNTAX_TSDB)
      {
         query.append(L" AND event_timestamp<=to_timestamp(");
         query.append(static_cast<int64_t>(timeTo));
         query.append(L")");
      }
      else
      {
         query.append(L" AND event_timestamp<=");
         query.append(static_cast<int64_t>(timeTo));
      }
   }

   query.append(L" ORDER BY event_timestamp DESC");

   int32_t maxRecords = request.getFieldAsInt32(VID_MAX_RECORDS);
   if (maxRecords <= 0 || maxRecords > 1000)
      maxRecords = 1000;

   switch(g_dbSyntax)
   {
      case DB_SYNTAX_MYSQL:
      case DB_SYNTAX_PGSQL:
      case DB_SYNTAX_SQLITE:
      case DB_SYNTAX_TSDB:
         query.append(L" LIMIT ");
         query.append(maxRecords);
         break;
      case DB_SYNTAX_ORACLE:
         query.insert(0, L"SELECT * FROM (");
         query.append(L") WHERE ROWNUM<=");
         query.append(maxRecords);
         break;
      case DB_SYNTAX_MSSQL:
         query.insert(7, L"TOP");
         query.insert(11, maxRecords);
         break;
      default:
         break; // no need to modify query for other databases
   }

   DB_RESULT hResult = DBSelect(hdb, query.cstr());
   if (hResult != nullptr)
   {
      int count = std::min(DBGetNumRows(hResult), maxRecords);
      response->setField(VID_NUM_ELEMENTS, static_cast<uint32_t>(count));

      uint32_t fieldId = VID_ELEMENT_LIST_BASE;
      wchar_t buffer[256];
      for (int i = 0; i < count; i++)
      {
         response->setField(fieldId++, DBGetFieldInt64(hResult, i, 0));        // record_id
         response->setField(fieldId++, DBGetFieldULong(hResult, i, 1));        // event_timestamp
         response->setField(fieldId++, DBGetFieldMacAddr(hResult, i, 2));      // mac_address
         response->setField(fieldId++, DBGetField(hResult, i, 3, buffer, 64)); // ip_address
         response->setField(fieldId++, DBGetFieldULong(hResult, i, 4));        // node_id
         response->setField(fieldId++, DBGetFieldULong(hResult, i, 5));        // switch_id
         response->setField(fieldId++, DBGetFieldULong(hResult, i, 6));        // interface_id
         response->setField(fieldId++, DBGetFieldULong(hResult, i, 7));        // vlan_id
         response->setField(fieldId++, DBGetFieldULong(hResult, i, 8));        // event_type
         response->setField(fieldId++, DBGetFieldULong(hResult, i, 9));        // old_switch_id
         response->setField(fieldId++, DBGetFieldULong(hResult, i, 10));       // old_interface_id
         fieldId += 9; // padding to 20-field stride
      }
      DBFreeResult(hResult);
      response->setField(VID_RCC, RCC_SUCCESS);
   }
   else
   {
      response->setField(VID_RCC, RCC_DB_FAILURE);
   }

   DBConnectionPoolReleaseConnection(hdb);
}
