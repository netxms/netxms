/**
 * NetXMS - Network Management System
 * Driver for Mikrotik devices
 * Copyright (C) 2017-2022 Raden Solutions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * File: data.cpp
 */

#include "mikrotik.h"
#include <netxms-version.h>



/**
 * public function to update device information with mutex lock here
 */
void MikrotikDriverData::updateDeviceInfo(SNMP_Transport *snmp)
{
   m_cacheLock.lock();
   updateDeviceInfoInternal(snmp);
   m_cacheLock.unlock();
}



/**
 * private function to update device information without mutex lock here
 */
void MikrotikDriverData::updateDeviceInfoInternal(SNMP_Transport *snmp)
{
   m_oidCache.clear();
   if (SnmpWalk(snmp, _T(".1.3.6.1.4.1.14988.1.1.3.100.1.2"), this, &MikrotikDriverData::metricInfoWalkCallback) == SNMP_ERR_SUCCESS)
   {
      nxlog_debug_tag(MIKROTIK_DEBUG_TAG, 7, _T("MikrotikDriverData::updateDeviceInfoInternal: updated device information successfully"));
   }
   m_cacheTimestamp = time(NULL);
}

/**
 * Callback for reading metric information and fill the list of available metrics for this device
 */
uint32_t MikrotikDriverData::metricInfoWalkCallback(SNMP_Variable *v, SNMP_Transport *snmp)
{
   SNMP_ObjectId oid = v->getName();

   oid.changeElement(12, 3); // change 12th element to 3, to get the gauge
   TCHAR name[MAX_OBJECT_NAME];
   v->getValueAsString(name, MAX_OBJECT_NAME);
   m_oidCache.set(name, oid.toString());
   return SNMP_ERR_SUCCESS;
}


/**
 * Register driver's metrics
 */
void MikrotikDriverData::registerMetrics(ObjectArray<AgentParameterDefinition> *metrics)
{
   m_cacheLock.lock();
   StringList *keyList = m_oidCache.keys();
   for (int i = 0; i < keyList->size(); i++)
   {
      metrics->add(new AgentParameterDefinition(keyList->get(i), keyList->get(i), DCI_DT_UINT ));
   }
   nxlog_debug_tag(MIKROTIK_DEBUG_TAG, 7, _T("MikrotikDriverData::registerMetrics: %d new metrics registered "), keyList->size());
   delete keyList;
   m_cacheLock.unlock();

}

/**
 * Get metric information by name
 * Metric name expected in form bus/position/variable
 */
bool MikrotikDriverData::getMetric(const TCHAR *name, SNMP_Transport *snmp, StringBuffer *metric)
{
   m_cacheLock.lock();
   if ((m_cacheTimestamp == 0) || (time(nullptr) - m_cacheTimestamp > 3600))
   {
      updateDeviceInfoInternal(snmp);
   }
   const TCHAR *oid = m_oidCache.get(name); //returns oid
   if (oid == nullptr)
   {
      nxlog_debug_tag(MIKROTIK_DEBUG_TAG, 7, _T("MikrotikDriverData::getMetric(%s): metric object NOT found"), name);
      m_cacheLock.unlock();
      return false;
   }
   metric->append(oid);
   m_cacheLock.unlock();
   return true;
}


