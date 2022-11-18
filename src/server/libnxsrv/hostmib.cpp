/*
** NetXMS - Network Management System
** Copyright (C) 2003-2022 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation; either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: hostmib.cpp
**/

#include "libnxsrv.h"
#include <nddrv.h>

#define DEBUG_TAG _T("ndd.hostmib")

/**
 * Storage entry helper - get free space in bytes
 */
void HostMibStorageEntry::getFree(TCHAR *buffer, size_t len) const
{
   _sntprintf(buffer, len, UINT64_FMT, static_cast<uint64_t>(size - used) * static_cast<uint64_t>(unitSize));
}

/**
 * Storage entry helper - get free space in percents
 */
void HostMibStorageEntry::getFreePerc(TCHAR *buffer, size_t len) const
{
   _sntprintf(buffer, len, _T("%f"), static_cast<double>(size - used) * 100.0 / static_cast<double>(size));
}

/**
 * Storage entry helper - get total space in bytes
 */
void HostMibStorageEntry::getTotal(TCHAR *buffer, size_t len) const
{
   _sntprintf(buffer, len, UINT64_FMT, static_cast<uint64_t>(size) * static_cast<uint64_t>(unitSize));
}

/**
 * Storage entry helper - get used space in bytes
 */
void HostMibStorageEntry::getUsed(TCHAR *buffer, size_t len) const
{
   _sntprintf(buffer, len, UINT64_FMT, static_cast<uint64_t>(used) * static_cast<uint64_t>(unitSize));
}

/**
 * Storage entry helper - get used space in percents
 */
void HostMibStorageEntry::getUsedPerc(TCHAR *buffer, size_t len) const
{
   _sntprintf(buffer, len, _T("%f"), static_cast<double>(used) * 100.0 / static_cast<double>(size));
}

/**
 * Storage entry helper - get metric by name
 */
bool HostMibStorageEntry::getMetric(const TCHAR *metric, TCHAR *buffer, size_t len) const
{
   if (!_tcsicmp(metric, _T("Free")))
      getFree(buffer, len);
   else if (!_tcsicmp(metric, _T("FreePerc")))
      getFreePerc(buffer, len);
   else if (!_tcsicmp(metric, _T("Total")))
      getTotal(buffer, len);
   else if (!_tcsicmp(metric, _T("Used")))
      getUsed(buffer, len);
   else if (!_tcsicmp(metric, _T("UsedPerc")))
      getUsedPerc(buffer, len);
   else
      return false;
   return true;
}

/**
 * Constructor
 */
HostMibDriverData::HostMibDriverData() : DriverData(), m_storage(16, 16, Ownership::True), m_storageCacheMutex(MutexType::FAST)
{
   m_storageCacheTimestamp = 0;
}

/**
 * Destructor
 */
HostMibDriverData::~HostMibDriverData()
{
}

/**
 * Callback for processing storage walk
 */
uint32_t HostMibDriverData::updateStorageCacheCallback(SNMP_Variable *v, SNMP_Transport *snmp)
{
   SNMP_ObjectId oid = v->getName();

   SNMP_PDU request(SNMP_GET_REQUEST, SnmpNewRequestId(), snmp->getSnmpVersion());

   oid.changeElement(10, 2);  // hrStorageType
   request.bindVariable(new SNMP_Variable(oid));

   oid.changeElement(10, 4);  // hrStorageAllocationUnits
   request.bindVariable(new SNMP_Variable(oid));

   oid.changeElement(10, 5);  // hrStorageSize
   request.bindVariable(new SNMP_Variable(oid));

   oid.changeElement(10, 6);  // hrStorageUsed
   request.bindVariable(new SNMP_Variable(oid));

   SNMP_PDU *response;
   uint32_t rc = snmp->doRequest(&request, &response);
   if (rc != SNMP_ERR_SUCCESS)
      return rc;

   if (response->getNumVariables() == 4)
   {
      HostMibStorageEntry *e = new HostMibStorageEntry;
      v->getValueAsString(e->name, 128);
      SNMP_ObjectId type = response->getVariable(0)->getValueAsObjectId();
      if (type.compare(_T(".1.3.6.1.2.1.25.2.1")) == OID_LONGER)
         e->type = static_cast<HostMibStorageType>(type.getElement(9));
      else
         e->type = hrStorageOther;
      e->unitSize = response->getVariable(1)->getValueAsUInt();
      e->size = response->getVariable(2)->getValueAsUInt();
      e->used = response->getVariable(3)->getValueAsUInt();
      e->lastUpdate = time(NULL);
      memcpy(e->oid, oid.value(), 12 * sizeof(UINT32));
      m_storage.add(e);
   }
   delete response;
   return SNMP_ERR_SUCCESS;
}

/**
 * Update storage entry cache
 */
void HostMibDriverData::updateStorageCache(SNMP_Transport *snmp)
{
   m_storageCacheMutex.lock();
   m_storage.clear();
   SnmpWalk(snmp, _T(".1.3.6.1.2.1.25.2.3.1.3"), this, &HostMibDriverData::updateStorageCacheCallback);
   m_storageCacheTimestamp = time(nullptr);
   nxlog_debug_tag(DEBUG_TAG, 5, _T("Storage cache updated for node %s [%u]:"), m_nodeName, m_nodeId);
   for(int i = 0; i < m_storage.size(); i++)
   {
      HostMibStorageEntry *e = m_storage.get(i);
      nxlog_debug_tag(DEBUG_TAG, 5, _T("   \"%s\": type=%d, size=%u, used=%u, unit=%u"), e->name, e->type, e->size, e->used, e->unitSize);
   }
   m_storageCacheMutex.unlock();
}

/**
 * Get storage entry
 */
const HostMibStorageEntry *HostMibDriverData::getStorageEntry(SNMP_Transport *snmp, const TCHAR *name, HostMibStorageType type)
{
   if ((m_storageCacheTimestamp == 0) || (time(nullptr) - m_storageCacheTimestamp > 3600))
      updateStorageCache(snmp);

   m_storageCacheMutex.lock();

   HostMibStorageEntry *entry = nullptr;
   for(int i = 0; i < m_storage.size(); i++)
   {
      HostMibStorageEntry *e = m_storage.get(i);
      if ((e->type == type) && ((name == nullptr) || !_tcscmp(name, e->name)))
      {
         entry = e;
         break;
      }
   }

   if (entry != nullptr)
   {
      time_t now = time(nullptr);
      if (entry->lastUpdate + 5 < now)
      {
         if (SnmpGetEx(snmp, nullptr, entry->oid, 12, &entry->used, sizeof(uint32_t), 0, nullptr) == SNMP_ERR_SUCCESS)
            entry->lastUpdate = now;
         else
            entry = nullptr;  // return error
      }
   }

   m_storageCacheMutex.unlock();
   return entry;
}
