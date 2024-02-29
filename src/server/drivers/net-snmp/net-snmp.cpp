/**
 * NetXMS - Network Management System
 * Driver for NetSNMP agents
 * Copyright (C) 2017-2024 Raden Solutions
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
 * File: net-snmp.cpp
 */

#include "net-snmp.h"
#include <netxms-version.h>

#define DEBUG_TAG _T("ndd.net-snmp")

/**
 * Do additional checks on the device required by driver.
 * Driver can set device's custom attributes and driver's data from within this method.
 * If driver's data was set on previous call, same pointer will be passed on all subsequent calls.
 * It is driver's responsibility to destroy existing object if it is to be replaced . One data
 * object should not be used for multiple nodes. Data object may be destroyed by framework when no longer needed.
 *
 * @param snmp SNMP transport
 * @param oid Device OID
 * @param node Node
 * @param driverData pointer to pointer to driver-specific data
 */
void NetSnmpBaseDriver::analyzeDevice(SNMP_Transport *snmp, const SNMP_ObjectId& oid, NObject *node, DriverData **driverData)
{
   if (*driverData == nullptr)
      *driverData = new NetSnmpDriverData();
   static_cast<NetSnmpDriverData*>(*driverData)->updateStorageCache(snmp);
}

/**
 * Check if driver can provide additional metrics
 */
bool NetSnmpBaseDriver::hasMetrics()
{
   return true;
}

/**
 * Get value of given metric
 *
 * @param snmp SNMP transport
 * @param node Node
 * @param driverData driver-specific data previously created in analyzeDevice
 * @param name metric name
 * @param value buffer for metric value (size at least MAX_RESULT_LENGTH)
 * @param size buffer size
 * @return data collection error code
 */
DataCollectionError NetSnmpBaseDriver::getMetric(SNMP_Transport *snmp, NObject *node, DriverData *driverData, const TCHAR *name, TCHAR *value, size_t size)
{
   if (driverData == NULL)
      return DCE_COLLECTION_ERROR;

   nxlog_debug_tag(DEBUG_TAG, 7, _T("NetSnmpBaseDriver::getMetric(%s [%u]): Requested metric \"%s\""), driverData->getNodeName(), driverData->getNodeId(), name);
   NetSnmpDriverData *d = static_cast<NetSnmpDriverData*>(driverData);

   DataCollectionError rc = getHostMibMetric(snmp, d, name, value, size);
   if (rc != DCE_NOT_SUPPORTED)
      return rc;

   const HostMibStorageEntry *e;
   const TCHAR *suffix;
   if (!_tcsnicmp(name, _T("HostMib.Memory.Swap."), 20))
   {
      e = d->getSwapSpace(snmp);
      suffix = &name[20];
   }
   else if (!_tcsnicmp(name, _T("HostMib.Memory.Virtual."), 23))
   {
      e = d->getVirtualMemory(snmp);
      suffix = &name[23];
   }
   else
   {
      return DCE_NOT_SUPPORTED;
   }

   if (e == NULL)
      return DCE_COLLECTION_ERROR;

   nxlog_debug_tag(DEBUG_TAG, 7, _T("NetSnmpBaseDriver::getMetric(%s [%u]): Storage entry found (name=%s, size=%u, used=%u, unit=%u)"),
            driverData->getNodeName(), driverData->getNodeId(), e->name, e->size, e->used, e->unitSize);
   return e->getMetric(suffix, value, size) ? DCE_SUCCESS : DCE_NOT_SUPPORTED;
}

/**
 * Get list of metrics supported by driver
 *
 * @param snmp SNMP transport
 * @param node Node
 * @param driverData driver-specific data previously created in analyzeDevice
 * @return list of metrics supported by driver or NULL on error
 */
ObjectArray<AgentParameterDefinition> *NetSnmpBaseDriver::getAvailableMetrics(SNMP_Transport *snmp, NObject *node, DriverData *driverData)
{
   ObjectArray<AgentParameterDefinition> *metrics = new ObjectArray<AgentParameterDefinition>(16, 16, Ownership::True);
   registerHostMibMetrics(metrics);
   metrics->add(new AgentParameterDefinition(_T("HostMib.Memory.Swap.Free"), _T("Free swap area"), DCI_DT_UINT64));
   metrics->add(new AgentParameterDefinition(_T("HostMib.Memory.Swap.FreePerc"), _T("Percentage of free swap area"), DCI_DT_FLOAT));
   metrics->add(new AgentParameterDefinition(_T("HostMib.Memory.Swap.Total"), _T("Total swap area"), DCI_DT_UINT64));
   metrics->add(new AgentParameterDefinition(_T("HostMib.Memory.Swap.Used"), _T("Used swap area"), DCI_DT_UINT64));
   metrics->add(new AgentParameterDefinition(_T("HostMib.Memory.Swap.UsedPerc"), _T("Percentage of used swap area"), DCI_DT_FLOAT));
   metrics->add(new AgentParameterDefinition(_T("HostMib.Memory.Virtual.Free"), _T("Free virtual memory"), DCI_DT_UINT64));
   metrics->add(new AgentParameterDefinition(_T("HostMib.Memory.Virtual.FreePerc"), _T("Percentage of free virtual memory"), DCI_DT_FLOAT));
   metrics->add(new AgentParameterDefinition(_T("HostMib.Memory.Virtual.Total"), _T("Total virtual memory"), DCI_DT_UINT64));
   metrics->add(new AgentParameterDefinition(_T("HostMib.Memory.Virtual.Used"), _T("Used virtual memory"), DCI_DT_UINT64));
   metrics->add(new AgentParameterDefinition(_T("HostMib.Memory.Virtual.UsedPerc"), _T("Percentage of used virtual memory"), DCI_DT_FLOAT));
   return metrics;
}

/**
 * Driver entry point
 */
NDD_BEGIN_DRIVER_LIST
NDD_DRIVER(NetSnmpDriver)
NDD_DRIVER(TeltonikaDriver)
NDD_END_DRIVER_LIST
DECLARE_NDD_MODULE_ENTRY_POINT

/**
 * DLL entry point
 */
#ifdef _WIN32

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	if (dwReason == DLL_PROCESS_ATTACH)
		DisableThreadLibraryCalls(hInstance);
	return TRUE;
}

#endif
