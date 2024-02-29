/**
 * NetXMS - Network Management System
 * Driver for Rittal CMC and LCP devices
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
 * File: rittal.cpp
 */

#include "rittal.h"
#include <netxms-version.h>

/**
 * Get driver name
 */
const TCHAR *RittalDriver::getName()
{
   return _T("RITTAL");
}

/**
 * Get driver version
 */
const TCHAR *RittalDriver::getVersion()
{
   return NETXMS_VERSION_STRING;
}

/**
 * Check if given device can be potentially supported by driver
 *
 * @param oid Device OID
 */
int RittalDriver::isPotentialDevice(const SNMP_ObjectId& oid)
{
   return oid.equals({ 1, 3, 6, 1, 4, 1, 2606, 7 }) ? 254 : 0;
}

/**
 * Check if given device is supported by driver
 *
 * @param snmp SNMP transport
 * @param oid Device OID
 */
bool RittalDriver::isDeviceSupported(SNMP_Transport *snmp, const SNMP_ObjectId& oid)
{
	return true;
}

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
void RittalDriver::analyzeDevice(SNMP_Transport *snmp, const SNMP_ObjectId& oid, NObject *node, DriverData **driverData)
{
   if (*driverData == nullptr)
      *driverData = new RittalDriverData();
   static_cast<RittalDriverData*>(*driverData)->updateStorageCache(snmp);
   static_cast<RittalDriverData*>(*driverData)->updateDeviceInfo(snmp);
}

/**
 * Check if driver can provide additional metrics
 */
bool RittalDriver::hasMetrics()
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
DataCollectionError RittalDriver::getMetric(SNMP_Transport *snmp, NObject *node, DriverData *driverData, const TCHAR *name, TCHAR *value, size_t size)
{
   if (driverData == nullptr)
      return DCE_COLLECTION_ERROR;

   nxlog_debug_tag(RITTAL_DEBUG_TAG, 7, _T("RittalDriver::getMetric(%s [%u]): Requested metric \"%s\""), driverData->getNodeName(), driverData->getNodeId(), name);
   RittalDriverData *d = static_cast<RittalDriverData*>(driverData);

   DataCollectionError rc = getHostMibMetric(snmp, d, name, value, size);
   if (rc != DCE_NOT_SUPPORTED)
      return rc;

   RittalMetric metric;
   if (!d->getMetric(name, snmp, &metric))
      return DCE_NOT_SUPPORTED;

   TCHAR oidText[256];
   nxlog_debug_tag(RITTAL_DEBUG_TAG, 7, _T("RittalDriver::getMetric(%s [%u]): metric object found, OID=\"%s\""),
            driverData->getNodeName(), driverData->getNodeId(), SnmpConvertOIDToText(15, metric.oid, oidText, 256));
   return (SnmpGetEx(snmp, nullptr, metric.oid, 15, value, size * sizeof(TCHAR), SG_STRING_RESULT, nullptr) == SNMP_ERR_SUCCESS) ? DCE_SUCCESS : DCE_COMM_ERROR;
}

/**
 * Get list of metrics supported by driver
 *
 * @param snmp SNMP transport
 * @param node Node
 * @param driverData driver-specific data previously created in analyzeDevice
 * @return list of metrics supported by driver or NULL on error
 */
ObjectArray<AgentParameterDefinition> *RittalDriver::getAvailableMetrics(SNMP_Transport *snmp, NObject *node, DriverData *driverData)
{
   ObjectArray<AgentParameterDefinition> *metrics = new ObjectArray<AgentParameterDefinition>(16, 16, Ownership::True);
   registerHostMibMetrics(metrics);
   static_cast<RittalDriverData*>(driverData)->registerMetrics(metrics);
   return metrics;
}

/**
 * Driver entry point
 */
DECLARE_NDD_ENTRY_POINT(RittalDriver);

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
