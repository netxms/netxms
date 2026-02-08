/* 
** NetXMS - Network Management System
** Driver for Planet Technology Corp. LAN switch devices
** Copyright (C) 2003-2025 Raden Solutions
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
** File: lansw.cpp
**/

#include "planet.h"
#include <netxms-regex.h>
#include <nms_agent.h>

#define DEBUG_TAG _T("ndd.planet.lansw")

/**
 * Get driver name
 */
const wchar_t *PlanetLanSwDriver::getName()
{
   return L"PLANET-LANSW";
}

/**
 * Get driver version
 */
const wchar_t *PlanetLanSwDriver::getVersion()
{
   return NETXMS_VERSION_STRING;
}

/**
 * Check if given device can be potentially supported by driver
 *
 * @param oid Device OID
 */
int PlanetLanSwDriver::isPotentialDevice(const SNMP_ObjectId& oid)
{
	return oid.startsWith({ 1, 3, 6, 1, 4, 1, 10456, 1 }) ? 255 : 0;
}

/**
 * Check if given device is supported by driver
 *
 * @param snmp SNMP transport
 * @param oid Device OID
 */
bool PlanetLanSwDriver::isDeviceSupported(SNMP_Transport *snmp, const SNMP_ObjectId& oid)
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
void PlanetLanSwDriver::analyzeDevice(SNMP_Transport *snmp, const SNMP_ObjectId& oid, NObject *node, DriverData **driverData)
{
   if (*driverData == nullptr)
   {
      *driverData = new LanSwDriverData();
      nxlog_debug_tag(DEBUG_TAG, 8, _T("PlanetLanSwDriver::analyzeDevice: new driver data object created for %s [%u]"), node->getName(), node->getId());
   }
}

/**
 * Get hardware information from device.
 *
 * @param snmp SNMP transport
 * @param node Node
 * @param driverData driver data
 * @param hwInfo pointer to hardware information structure to fill
 * @return true if hardware information is available
 */
bool PlanetLanSwDriver::getHardwareInformation(SNMP_Transport *snmp, NObject *node, DriverData *driverData, DeviceHardwareInfo *hwInfo)
{
   wcscpy(hwInfo->vendor, L"Planet Technology Corp.");

   SNMP_PDU request(SNMP_GET_REQUEST, SnmpNewRequestId(), snmp->getSnmpVersion());
   request.bindVariable(new SNMP_Variable({ 1, 3, 6, 1, 4, 1, 10456, 3, 6, 21, 0 }));  // nmsSwitchName
   request.bindVariable(new SNMP_Variable({ 1, 3, 6, 1, 4, 1, 10456, 3, 6, 3, 0 }));  // nmschassisId (serial number)
   request.bindVariable(new SNMP_Variable({ 1, 3, 6, 1, 4, 1, 10456, 3, 6, 10, 1, 6, 0 }));  // nmscardSwVersion (only for non-modular switches)

   SNMP_PDU *response;
   if (snmp->doRequest(&request, &response) == SNMP_ERR_SUCCESS)
   {
      wchar_t buffer[256];

      SNMP_Variable *v = response->getVariable(0);
      if ((v != nullptr) && (v->getType() == ASN_OCTET_STRING))
      {
         wcslcpy(hwInfo->productName, v->getValueAsString(buffer, 256), 128);
      }

      v = response->getVariable(1);
      if ((v != nullptr) && (v->getType() == ASN_OCTET_STRING))
      {
         wcslcpy(hwInfo->serialNumber, v->getValueAsString(buffer, 256), 32);
      }

      v = response->getVariable(2);
      if ((v != nullptr) && (v->getType() == ASN_OCTET_STRING))
      {
         wcslcpy(hwInfo->productVersion, v->getValueAsString(buffer, 256), 16);
      }

      delete response;
   }
   return true;
}

/**
 * Get list of interfaces for given node
 *
 * @param snmp SNMP transport
 * @param node Node
 * @param driverData driver data
 * @param useIfXTable if true, usage of ifXTable is allowed
 */
InterfaceList *PlanetLanSwDriver::getInterfaces(SNMP_Transport *snmp, NObject *node, DriverData *driverData, bool useIfXTable)
{
   InterfaceList *ifList = NetworkDeviceDriver::getInterfaces(snmp, node, driverData, true);
   if (ifList == nullptr)
      return nullptr;

   wchar_t debugId[128];
   nx_swprintf(debugId, 128, L"%s [%u]", node->getName(), node->getId());

   SnmpWalk(snmp, { 1, 3, 6, 1, 4, 1, 10456, 3, 6, 12, 1, 3 }, // nmscardIfSlotNumber
      [ifList, &debugId] (SNMP_Variable *var) -> uint32_t
      {
         const SNMP_ObjectId& name = var->getName();
         uint32_t ifIndex = name.getLastElement();
         InterfaceInfo *iface = ifList->findByIfIndex(ifIndex);
         if (iface != nullptr)
         {
            iface->location.module = var->getValueAsInt();
            iface->isPhysicalPort = true;
            nxlog_debug_tag(DEBUG_TAG, 6, L"PlanetLanSwDriver::getInterfaces(%s): Interface %u (%s): slot number %d", debugId, ifIndex, iface->name, iface->location.module);
         }
         return SNMP_ERR_SUCCESS;
      });

   SnmpWalk(snmp, { 1, 3, 6, 1, 4, 1, 10456, 3, 6, 12, 1, 2 }, // nmscardIfPortNumber
      [ifList, &debugId] (SNMP_Variable *var) -> uint32_t
      {
         const SNMP_ObjectId& name = var->getName();
         uint32_t ifIndex = name.getLastElement();
         InterfaceInfo *iface = ifList->findByIfIndex(ifIndex);
         if (iface != nullptr)
         {
            uint32_t n = var->getValueAsUInt();
            iface->location.pic = (n >> 16) & 0x0F;
            iface->location.port = n & 0xFFFF;
            iface->isPhysicalPort = true;
            nxlog_debug_tag(DEBUG_TAG, 6, L"PlanetLanSwDriver::getInterfaces(%s): Interface %u (%s): port location %d.%d.%d", debugId, ifIndex, iface->name,
               iface->location.module, iface->location.pic, iface->location.port);
         }
         return SNMP_ERR_SUCCESS;
      });

   const char *eptr;
   int eoffset;
   PCRE *reBase = _pcre_compile_t(reinterpret_cast<const PCRE_TCHAR*>(_T("^(TGiga|Giga)Ethernet([0-9]+)/([0-9]+)$")), PCRE_COMMON_FLAGS | PCRE_CASELESS, &eptr, &eoffset, nullptr);
   if (reBase == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("PlanetLanSwDriver::getInterfaces(%s): cannot compile base regexp: %hs at offset %d"), debugId, eptr, eoffset);
      return ifList;
   }
   PCRE *reFex = _pcre_compile_t(reinterpret_cast<const PCRE_TCHAR*>(_T("^(TGiga|Giga)Ethernet([0-9]+)/([0-9]+)/([0-9]+)$")), PCRE_COMMON_FLAGS | PCRE_CASELESS, &eptr, &eoffset, nullptr);
   if (reFex == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("PlanetLanSwDriver::getInterfaces(%s): cannot compile FEX regexp: %hs at offset %d"), debugId, eptr, eoffset);
      _pcre_free_t(reBase);
      return ifList;
   }

   int pmatch[30];
   for (int i = 0; i < ifList->size(); i++)
   {
      InterfaceInfo *iface = ifList->get(i);
      if ((iface->type == IFTYPE_ETHERNET_CSMACD) || (iface->type == IFTYPE_GIGABIT_ETHERNET))
      {
         iface->isPhysicalPort = true;

         // Determine max speed from interface name
         if (!wcsncmp(iface->name, L"GigaEthernet", 12))
         {
            iface->maxSpeed = _ULL(1000000000);
         }
         else if (!wcsncmp(iface->name, L"TGigaEthernet", 13))
         {
            iface->maxSpeed = _ULL(10000000000);
         }
         else if (!wcsncmp(iface->name, L"FastEthernet", 12))
         {
            iface->maxSpeed = 100000000;
         }

         if (iface->location.module != 0 || iface->location.port != 0)
            continue;  // Location already set from slot/port number tables

         if (_pcre_exec_t(reBase, nullptr, reinterpret_cast<PCRE_TCHAR*>(iface->name), static_cast<int>(_tcslen(iface->name)), 0, 0, pmatch, 30) == 4)
         {
            iface->location.module = IntegerFromCGroup(iface->name, pmatch, 2);
            iface->location.port = IntegerFromCGroup(iface->name, pmatch, 3);
            nxlog_debug_tag(DEBUG_TAG, 6, L"PlanetLanSwDriver::getInterfaces(%s): Interface %u (%s): parsed location %d.%d.%d", debugId, iface->index, iface->name,
               iface->location.module, iface->location.pic, iface->location.port);
         }
         else if (_pcre_exec_t(reFex, nullptr, reinterpret_cast<PCRE_TCHAR*>(iface->name), static_cast<int>(_tcslen(iface->name)), 0, 0, pmatch, 30) == 5)
         {
            iface->location.module = IntegerFromCGroup(iface->name, pmatch, 2);
            iface->location.pic = IntegerFromCGroup(iface->name, pmatch, 3);
            iface->location.port = IntegerFromCGroup(iface->name, pmatch, 4);
            nxlog_debug_tag(DEBUG_TAG, 6, L"PlanetLanSwDriver::getInterfaces(%s): Interface %u (%s): parsed location %d.%d.%d", debugId, iface->index, iface->name,
               iface->location.module, iface->location.pic, iface->location.port);
         }
      }
   }

   _pcre_free_t(reBase);
   _pcre_free_t(reFex);
   return ifList;
}

/**
 * Check if driver can provide additional metrics
 */
bool PlanetLanSwDriver::hasMetrics()
{
   return true;
}

/**
 * Get list of metrics supported by driver
 *
 * @param snmp SNMP transport
 * @param node Node
 * @param driverData driver-specific data previously created in analyzeDevice
 * @return list of metrics supported by driver or NULL on error
 */

ObjectArray<AgentParameterDefinition> *PlanetLanSwDriver::getAvailableMetrics(SNMP_Transport *snmp, NObject *node, DriverData *driverData)
{
   ObjectArray<AgentParameterDefinition> *metrics = new ObjectArray<AgentParameterDefinition>(16, 16, Ownership::True);
   metrics->add(new AgentParameterDefinition(L"Card.CPUUsage(*)", L"Card {instance}: CPU usage", DCI_DT_UINT));
   metrics->add(new AgentParameterDefinition(L"Card.MemoryUsage(*)", L"Card {instance}: memory usage", DCI_DT_UINT));
   metrics->add(new AgentParameterDefinition(L"Card.Model(*)", L"Card {instance}: model", DCI_DT_STRING));
   metrics->add(new AgentParameterDefinition(L"Card.SerialNumber(*)", L"Card {instance}: serial number", DCI_DT_STRING));
   metrics->add(new AgentParameterDefinition(L"Card.Temperature(*)", L"Card {instance}: temperature", DCI_DT_INT));
   metrics->add(new AgentParameterDefinition(L"Card.Voltage(*)", L"Card {instance}: voltage", DCI_DT_INT));
   return metrics;
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
DataCollectionError PlanetLanSwDriver::getMetric(SNMP_Transport *snmp, NObject *node, DriverData *driverData, const wchar_t *name, wchar_t *value, size_t size)
{
   if (driverData == nullptr)
      return DCE_COLLECTION_ERROR;

   uint32_t cardIndex;
   if (!AgentGetMetricArgAsUInt32(name, 1, &cardIndex))
      return DCE_NOT_SUPPORTED;

   uint32_t metricId;
   if (!wcsnicmp(name, L"Card.CPUUsage(", 14))
   {
      metricId = 11; // nmscardCPUUtilization
   }
   else if (!wcsnicmp(name, L"Card.MemoryUsage(", 17))
   {
      metricId = 12; // nmscardMEMUtilization
   }
   else if (!wcsnicmp(name, L"Card.Model(", 11))
   {
      metricId = 3; // nmscardDescr
   }
   else if (!wcsnicmp(name, L"Card.SerialNumber(", 18))
   {
      metricId = 4; // nmscardSerial
   }
   else if (!wcsnicmp(name, L"Card.Temperature(", 17))
   {
      metricId = 13; // nmscardTemperature
   }
   else if (!wcsnicmp(name, L"Card.Voltage(", 13))
   {
      metricId = 14; // nmscardVoltage
   }
   else
   {
      return DCE_NOT_SUPPORTED;
   }

   return static_cast<LanSwDriverData*>(driverData)->getMetric(snmp, cardIndex, metricId, value, size);
}

/**
 * Driver data destructor
 */
LanSwDriverData::~LanSwDriverData()
{
   delete m_cardTable;
}

/**
 * Get metric from SNMP snapshot
 */
DataCollectionError LanSwDriverData::getMetric(SNMP_Transport *transport, uint32_t cardIndex, uint32_t metric, wchar_t *value, size_t size)
{
   LockGuard lock(m_cacheLock);

   time_t now = time(nullptr);
   if ((m_cardTable == nullptr) || (now - m_cacheTimestamp > 30))
   {
      delete m_cardTable;
      m_cardTable = SNMP_Snapshot::create(transport, { 1, 3, 6, 1, 4, 1, 10456, 3, 6, 10 }, 1000); // nmscardTable
      if (m_cardTable == nullptr)
         return DCE_COMM_ERROR;
      m_cacheTimestamp = now;
   }

   const SNMP_Variable *v = m_cardTable->get({ 1, 3, 6, 1, 4, 1, 10456, 3, 6, 10, 1, metric, cardIndex });
   if (v == nullptr)
      return DCE_NOT_SUPPORTED;

   bool convert = false;
   v->getValueAsPrintableString(value, size, &convert);
   return DCE_SUCCESS;
}
