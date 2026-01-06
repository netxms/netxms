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
** File: port_stop_list.cpp
**
**/

#include "nxcore.h"

#define DEBUG_TAG _T("port.stoplist")

/**
 * Port stop list entry
 */
struct PortStopEntry
{
   uint32_t objectId;
   int32_t id;
   uint16_t port;
   char protocol;   // 'T' = TCP, 'U' = UDP, 'B' = Both
};

/**
 * In-memory cache of port stop lists
 */
static StructArray<PortStopEntry> s_portStopList;
static Mutex s_portStopListLock(MutexType::FAST);

/**
 * Get port stop list entries for specific object
 */
static void GetObjectPortStopList(uint32_t objectId, IntegerArray<uint16_t> *tcpPorts, IntegerArray<uint16_t> *udpPorts)
{
   s_portStopListLock.lock();
   for (int i = 0; i < s_portStopList.size(); i++)
   {
      PortStopEntry *e = s_portStopList.get(i);
      if (e->objectId == objectId)
      {
         if ((e->protocol == 'T') || (e->protocol == 'B'))
         {
            if (tcpPorts != nullptr)
               tcpPorts->add(e->port);
         }
         if ((e->protocol == 'U') || (e->protocol == 'B'))
         {
            if (udpPorts != nullptr)
               udpPorts->add(e->port);
         }
      }
   }
   s_portStopListLock.unlock();
}

/**
 * Check if object has its own port stop list configured
 */
static bool HasOwnPortStopList(uint32_t objectId)
{
   bool hasOwn = false;
   s_portStopListLock.lock();
   for (int i = 0; i < s_portStopList.size(); i++)
   {
      if (s_portStopList.get(i)->objectId == objectId)
      {
         hasOwn = true;
         break;
      }
   }
   s_portStopListLock.unlock();
   return hasOwn;
}

/**
 * Get effective port stop list for an object with inheritance
 * If object has its own list, use it. Otherwise, merge from all parents up to Zone.
 */
void GetEffectivePortStopList(uint32_t objectId, IntegerArray<uint16_t> *tcpPorts, IntegerArray<uint16_t> *udpPorts)
{
   shared_ptr<NetObj> object = FindObjectById(objectId);
   if (object == nullptr)
      return;

   // Check if object has its own stop list (stops inheritance)
   if (HasOwnPortStopList(objectId))
   {
      GetObjectPortStopList(objectId, tcpPorts, udpPorts);
      return;
   }

   // Traverse parent hierarchy
   HashSet<uint32_t> visited;
   visited.put(objectId);

   unique_ptr<SharedObjectArray<NetObj>> parents = object->getParents();
   for (int i = 0; i < parents->size(); i++)
   {
      NetObj *parent = parents->get(i);
      if ((parent->getObjectClass() != OBJECT_CONTAINER) && (parent->getObjectClass() != OBJECT_COLLECTOR) && (parent->getObjectClass() != OBJECT_CLUSTER) && (parent->getObjectClass() != OBJECT_SUBNET))
         continue;
      if (visited.contains(parent->getId()))
         continue;
      visited.put(parent->getId());
      GetObjectPortStopList(parent->getId(), tcpPorts, udpPorts);
   }

   // Check Zone
   int32_t zoneUIN = object->getZoneUIN();
   if (zoneUIN != 0)
   {
      shared_ptr<Zone> zone = FindZoneByUIN(zoneUIN);
      if ((zone != nullptr) && !visited.contains(zone->getId()))
      {
         GetObjectPortStopList(zone->getId(), tcpPorts, udpPorts);
      }
   }
}

/**
 * Check if port is blocked for given object (with inheritance)
 */
bool IsPortBlocked(uint32_t objectId, uint16_t port, bool tcp)
{
   IntegerArray<uint16_t> ports;
   if (tcp)
      GetEffectivePortStopList(objectId, &ports, nullptr);
   else
      GetEffectivePortStopList(objectId, nullptr, &ports);

   return ports.contains(port);
}

/**
 * Load port stop list from database at startup
 */
void LoadPortStopList()
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_RESULT hResult = DBSelect(hdb, _T("SELECT object_id,id,port,protocol FROM port_stop_list ORDER BY object_id,id"));
   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      for (int i = 0; i < count; i++)
      {
         PortStopEntry *e = s_portStopList.addPlaceholder();
         e->objectId = DBGetFieldULong(hResult, i, 0);
         e->id = DBGetFieldLong(hResult, i, 1);
         e->port = DBGetFieldUInt16(hResult, i, 2);

         TCHAR protocol[2];
         DBGetField(hResult, i, 3, protocol, 2);
         e->protocol = static_cast<char>(protocol[0]);
      }
      nxlog_debug_tag(DEBUG_TAG, 2, _T("Loaded %d port stop list entries"), count);
      DBFreeResult(hResult);
   }
   DBConnectionPoolReleaseConnection(hdb);
}

/**
 * Get port stop list for object into NXCP message
 */
void PortStopListToMessage(uint32_t objectId, NXCPMessage *msg)
{
   s_portStopListLock.lock();

   uint32_t count = 0;
   uint32_t fieldId = VID_PORT_STOP_LIST_BASE;
   for (int i = 0; i < s_portStopList.size(); i++)
   {
      PortStopEntry *e = s_portStopList.get(i);
      if (e->objectId == objectId)
      {
         msg->setField(fieldId++, e->port);
         msg->setField(fieldId++, static_cast<int16_t>(e->protocol));
         fieldId += 8;  // Reserve space for future fields
         count++;
      }
   }

   s_portStopListLock.unlock();

   msg->setField(VID_PORT_STOP_COUNT, count);
}

/**
 * Update port stop list from NXCP message
 */
uint32_t UpdatePortStopList(const NXCPMessage& request, uint32_t objectId)
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   if (!DBBegin(hdb))
   {
      DBConnectionPoolReleaseConnection(hdb);
      return RCC_DB_FAILURE;
   }

   StructArray<PortStopEntry> newEntries(0, 32);

   uint32_t rcc;
   DB_STATEMENT hStmt = DBPrepare(hdb, _T("DELETE FROM port_stop_list WHERE object_id=?"));
   if (hStmt != nullptr)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, objectId);
      if (DBExecute(hStmt))
      {
         rcc = RCC_SUCCESS;
         int count = request.getFieldAsInt32(VID_PORT_STOP_COUNT);
         if (count > 0)
         {
            DB_STATEMENT hStmt2 = DBPrepare(hdb, _T("INSERT INTO port_stop_list (object_id,id,port,protocol) VALUES(?,?,?,?)"), count > 1);
            if (hStmt2 != nullptr)
            {
               DBBind(hStmt2, 1, DB_SQLTYPE_INTEGER, objectId);

               uint32_t fieldId = VID_PORT_STOP_LIST_BASE;
               for (int i = 0; i < count; i++)
               {
                  uint16_t port = request.getFieldAsUInt16(fieldId++);
                  int16_t protocol = request.getFieldAsInt16(fieldId++);
                  fieldId += 8;  // Skip reserved fields

                  DBBind(hStmt2, 2, DB_SQLTYPE_INTEGER, i + 1);
                  DBBind(hStmt2, 3, DB_SQLTYPE_INTEGER, port);

                  TCHAR protocolStr[2];
                  protocolStr[0] = static_cast<TCHAR>(protocol);
                  protocolStr[1] = 0;
                  DBBind(hStmt2, 4, DB_SQLTYPE_VARCHAR, protocolStr, DB_BIND_STATIC);

                  if (!DBExecute(hStmt2))
                  {
                     rcc = RCC_DB_FAILURE;
                     break;
                  }

                  PortStopEntry *e = newEntries.addPlaceholder();
                  e->objectId = objectId;
                  e->id = i + 1;
                  e->port = port;
                  e->protocol = static_cast<char>(protocol);
               }
               DBFreeStatement(hStmt2);
            }
            else
            {
               rcc = RCC_DB_FAILURE;
            }
         }
      }
      else
      {
         rcc = RCC_DB_FAILURE;
      }
      DBFreeStatement(hStmt);
   }
   else
   {
      rcc = RCC_DB_FAILURE;
   }

   if (rcc == RCC_SUCCESS)
   {
      DBCommit(hdb);

      // Update in-memory cache
      s_portStopListLock.lock();
      for (int i = s_portStopList.size() - 1; i >= 0; i--)
      {
         if (s_portStopList.get(i)->objectId == objectId)
         {
            s_portStopList.remove(i);
         }
      }
      s_portStopList.addAll(newEntries);
      s_portStopListLock.unlock();

      NotifyClientSessions(NX_NOTIFY_PORT_STOP_LIST_CHANGED, objectId);
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Updated port stop list for object %u (%d entries)"), objectId, newEntries.size());
   }
   else
   {
      DBRollback(hdb);
   }

   DBConnectionPoolReleaseConnection(hdb);
   return rcc;
}

/**
 * Delete port stop list entries for object (called when object is deleted)
 */
void DeletePortStopList(uint32_t objectId)
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_STATEMENT hStmt = DBPrepare(hdb, _T("DELETE FROM port_stop_list WHERE object_id=?"));
   if (hStmt != nullptr)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, objectId);
      DBExecute(hStmt);
      DBFreeStatement(hStmt);
   }
   DBConnectionPoolReleaseConnection(hdb);

   // Update in-memory cache
   s_portStopListLock.lock();
   for (int i = s_portStopList.size() - 1; i >= 0; i--)
   {
      if (s_portStopList.get(i)->objectId == objectId)
      {
         s_portStopList.remove(i);
      }
   }
   s_portStopListLock.unlock();
}

/**
 * Get effective blocked port list for zone (used during discovery)
 */
void GetEffectivePortStopListForZone(int32_t zoneUIN, IntegerArray<uint16_t> *tcpPorts, IntegerArray<uint16_t> *udpPorts)
{
   if (zoneUIN == 0)
      return;

   shared_ptr<Zone> zone = FindZoneByUIN(zoneUIN);
   if (zone != nullptr)
   {
      GetObjectPortStopList(zone->getId(), tcpPorts, udpPorts);
   }
}
