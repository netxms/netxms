/**
 * NetXMS - Network Management System
 * Driver for Rittal CMC and LCP devices
 * Copyright (C) 2017 Raden Solutions
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

#include "rittal.h"

/**
 * Constructor
 */
RittalDriverData::RittalDriverData() : HostMibDriverData(), m_devices(32, 32, true)
{
   m_cacheTimestamp = 0;
   m_cacheLock = MutexCreate();
}

/**
 * Destructor
 */
RittalDriverData::~RittalDriverData()
{
   MutexDestroy(m_cacheLock);
}

/**
 * Get device by bus and position
 */
RittalDevice *RittalDriverData::getDevice(UINT32 bus, UINT32 position)
{
   for(int i = 0; i < m_devices.size(); i++)
   {
      RittalDevice *d = m_devices.get(i);
      if ((d->bus == bus) && (d->position == position))
         return d;
   }
   return NULL;
}

/**
 * Get device by index
 */
RittalDevice *RittalDriverData::getDevice(UINT32 index)
{
   for(int i = 0; i < m_devices.size(); i++)
   {
      RittalDevice *d = m_devices.get(i);
      if (d->index == index)
         return d;
   }
   return NULL;
}

/**
 * Get metric information by name
 * Metric name expected in form bus/position/variable
 */
bool RittalDriverData::getMetric(const TCHAR *name, SNMP_Transport *snmp, RittalMetric *metric)
{
   const TCHAR *s1 = _tcschr(name, _T('/'));
   if (s1 == NULL)
      return NULL;

   TCHAR buffer[32];
   _tcslcpy(buffer, name, std::min((size_t)32, (size_t)(s1 - name) + 1));
   UINT32 bus = _tcstoul(buffer, NULL, 10);

   s1++;
   const TCHAR *s2 = _tcschr(s1, _T('/'));
   if (s2 == NULL)
      return NULL;

   _tcslcpy(buffer, name, std::min((size_t)32, (size_t)(s2 - s1) + 1));
   UINT32 position = _tcstoul(buffer, NULL, 10);

   MutexLock(m_cacheLock);
   if ((m_cacheTimestamp == 0) || (time(NULL) - m_cacheTimestamp > 3600))
   {
      updateDeviceInfoInternal(snmp);
   }

   bool success = false;
   const RittalDevice *dev = getDevice(bus, position);
   if (dev != NULL)
   {
      s2++;
      for(int i = 0; i < dev->metrics->size(); i++)
      {
         RittalMetric *m = dev->metrics->get(i);
         if (!_tcsicmp(s2, m->name))
         {
            memcpy(metric, m, sizeof(RittalMetric));
            success = true;
         }
      }
   }
   MutexUnlock(m_cacheLock);
   return success;
}

/**
 * Callback for reading device information
 */
UINT32 RittalDriverData::deviceInfoWalkCallback(SNMP_Variable *v, SNMP_Transport *snmp)
{
   SNMP_ObjectId oid = v->getName();

   SNMP_PDU request(SNMP_GET_REQUEST, SnmpNewRequestId(), snmp->getSnmpVersion());

   oid.changeElement(12, 9); // cmcIIIDevBus
   request.bindVariable(new SNMP_Variable(oid));

   oid.changeElement(12, 10); // cmcIIIDevPos
   request.bindVariable(new SNMP_Variable(oid));

   oid.changeElement(12, 3); // cmcIIIDevAlias
   request.bindVariable(new SNMP_Variable(oid));

   SNMP_PDU *response;
   UINT32 rc = snmp->doRequest(&request, &response, SnmpGetDefaultTimeout(), 3);
   if (rc != SNMP_ERR_SUCCESS)
      return rc;

   if (response->getNumVariables() == 3)
   {
      RittalDevice *dev =
               new RittalDevice(oid.getElement(13), response->getVariable(0)->getValueAsUInt(),
                        response->getVariable(1)->getValueAsUInt(), v, response->getVariable(2));
      m_devices.add(dev);
   }
   return SNMP_ERR_SUCCESS;
}

/**
 * Callback for reading metric information
 */
UINT32 RittalDriverData::metricInfoWalkCallback(SNMP_Variable *v, SNMP_Transport *snmp)
{
   SNMP_ObjectId oid = v->getName();

   RittalDevice *dev = getDevice(oid.getElement(13));
   if (dev == NULL)
   {
      nxlog_debug_tag(RITTAL_DEBUG_TAG, 4, _T("Internal error: missing device entry for variable %s"), (const TCHAR *)oid.toString());
      return SNMP_ERR_SUCCESS;
   }

   SNMP_PDU request(SNMP_GET_REQUEST, SnmpNewRequestId(), snmp->getSnmpVersion());
   oid.changeElement(12, 6); // cmcIIIVarDatatype
   request.bindVariable(new SNMP_Variable(oid));

   SNMP_PDU *response;
   UINT32 rc = snmp->doRequest(&request, &response, SnmpGetDefaultTimeout(), 3);
   if (rc != SNMP_ERR_SUCCESS)
      return rc;

   if (response->getNumVariables() == 1)
   {
      RittalMetric *m = new RittalMetric;
      m->index = oid.getElement(14);
      v->getValueAsString(m->name, MAX_OBJECT_NAME);
      m->dataType = response->getVariable(0)->getValueAsInt();
      memcpy(m->oid, oid.value(), 15);
      m->oid[12] = (m->dataType == 2) ? 11 : 10; // Use cmcIIIVarValueInt for integer variables and cmcIIIVarValueStr for the rest
      _sntprintf(m->description, 256, _T("%s: %s"), dev->alias, m->name);
      dev->metrics->add(m);
   }

   return SNMP_ERR_SUCCESS;
}

/**
 * Update device information
 */
void RittalDriverData::updateDeviceInfoInternal(SNMP_Transport *snmp)
{
   m_devices.clear();
   if (SnmpWalk(snmp, _T(".1.3.6.1.4.1.2606.7.4.1.2.1.2"), this, &RittalDriverData::deviceInfoWalkCallback) == SNMP_ERR_SUCCESS)
   {
      SnmpWalk(snmp, _T(".1.3.6.1.4.1.2606.7.4.2.2.1.3"), this, &RittalDriverData::metricInfoWalkCallback);
   }
   m_cacheTimestamp = time(NULL);

   nxlog_debug_tag(RITTAL_DEBUG_TAG, 5, _T("Updated device information for node %s [%u]:"), m_nodeName, m_nodeId);
   for(int i = 0; i < m_devices.size(); i++)
   {
      RittalDevice *d = m_devices.get(i);
      nxlog_debug_tag(RITTAL_DEBUG_TAG, 5, _T("   %u/%u: %s (%s)"), d->bus, d->position, d->name, d->alias);
   }
}

/**
 * Update device information
 */
void RittalDriverData::updateDeviceInfo(SNMP_Transport *snmp)
{
   MutexLock(m_cacheLock);
   updateDeviceInfoInternal(snmp);
   MutexUnlock(m_cacheLock);
}

/**
 * Register driver's metrics
 */
void RittalDriverData::registerMetrics(ObjectArray<AgentParameterDefinition> *metrics)
{
   MutexLock(m_cacheLock);
   for(int i = 0; i < m_devices.size(); i++)
   {
      RittalDevice *d = m_devices.get(i);
      for(int j = 0; j < d->metrics->size(); j++)
      {
         RittalMetric *m = d->metrics->get(j);
         TCHAR name[256];
         _sntprintf(name, 256, _T("%u/%u/%s"), d->bus, d->position, m->name);
         metrics->add(new AgentParameterDefinition(name, m->description, (m->dataType == 2) ? DCI_DT_INT : DCI_DT_STRING));
      }
   }
   MutexUnlock(m_cacheLock);
}
