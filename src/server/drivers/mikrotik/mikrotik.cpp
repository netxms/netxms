/* 
** NetXMS - Network Management System
** Driver for Mikrotik routers
** Copyright (C) 2003-2024 Victor Kirhenshtein
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
** File: mikrotik.cpp
**/

#include "mikrotik.h"
#include <netxms-version.h>

/**
 * Driver version
 */
static TCHAR s_driverVersion[] = NETXMS_VERSION_STRING;

/**
 * Get driver name
 */
const TCHAR *MikrotikDriver::getName()
{
   return _T("MIKROTIK");
}

/**
 * Get driver version
 */
const TCHAR *MikrotikDriver::getVersion()
{
   return s_driverVersion;
}

/**
 * Check if given device can be potentially supported by driver
 *
 * @param oid Device OID
 */
int MikrotikDriver::isPotentialDevice(const TCHAR *oid)
{
    return !_tcscmp(oid, _T(".1.3.6.1.4.1.14988.1")) ? 254 : 0;
}

/**
 * Check if given device is supported by driver
 *
 * @param snmp SNMP transport
 * @param oid Device OID
 */
bool MikrotikDriver::isDeviceSupported(SNMP_Transport *snmp, const TCHAR *oid)
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
void MikrotikDriver::analyzeDevice(SNMP_Transport *snmp, const TCHAR *oid, NObject *node, DriverData **driverData)
{
   if (*driverData == nullptr)
   {
      *driverData = new MikrotikDriverData();
      nxlog_debug_tag(DEBUG_TAG, 8, _T("MikrotikDriver::analyzeDevice: new driver data object created."));
   }
   static_cast<MikrotikDriverData*>(*driverData)->updateDeviceInfo(snmp);
   node->setCustomAttribute(_T(".mikrotik.fdbUsesIfIndex"),
      (SnmpWalkCount(snmp, _T(".1.3.6.1.2.1.17.1.4.1.2")) == 0) ? _T("true") : _T("false"), StateChange::IGNORE);
}

/**
 * Returns true if FDB uses ifIndex instead of bridge port number for referencing interfaces.
 *
 * @return true if FDB uses ifIndex instead of bridge port number for referencing interfaces
 */
bool MikrotikDriver::isFdbUsingIfIndex(const NObject *node, DriverData *driverData)
{
   return node->getCustomAttributeAsBoolean(_T(".mikrotik.fdbUsesIfIndex"), false);
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
bool MikrotikDriver::getHardwareInformation(SNMP_Transport *snmp, NObject *node, DriverData *driverData, DeviceHardwareInfo *hwInfo)
{
   _tcscpy(hwInfo->vendor, _T("MikroTik"));

   SNMP_PDU request(SNMP_GET_REQUEST, SnmpNewRequestId(), snmp->getSnmpVersion());
   request.bindVariable(new SNMP_Variable(_T(".1.3.6.1.2.1.1.1.0")));
   request.bindVariable(new SNMP_Variable(_T(".1.3.6.1.4.1.14988.1.1.7.8.0")));  // Product name
   request.bindVariable(new SNMP_Variable(_T(".1.3.6.1.4.1.14988.1.1.7.4.0")));  // Firmware version
   request.bindVariable(new SNMP_Variable(_T(".1.3.6.1.4.1.14988.1.1.7.3.0")));  // Serial number

   SNMP_PDU *response;
   if (snmp->doRequest(&request, &response) == SNMP_ERR_SUCCESS)
   {
      TCHAR buffer[256];

      const SNMP_Variable *v = response->getVariable(0);
      if ((v != nullptr) && (v->getType() == ASN_OCTET_STRING))
      {
         v->getValueAsString(buffer, 256);
         TCHAR *p = !_tcsnicmp(buffer, _T("RouterOS "), 9) ? &buffer[9] : buffer;
         _tcslcpy(hwInfo->productCode, p, 32);
      }

      v = response->getVariable(1);
      if ((v != nullptr) && (v->getType() == ASN_OCTET_STRING))
      {
         _tcslcpy(hwInfo->productName, v->getValueAsString(buffer, 256), 128);
      }
      else
      {
         // Use product code as product name if product name OID is not supported
         _tcscpy(hwInfo->productName, hwInfo->productCode);
      }

      v = response->getVariable(2);
      if ((v != nullptr) && (v->getType() == ASN_OCTET_STRING))
      {
         _tcslcpy(hwInfo->productVersion, v->getValueAsString(buffer, 256), 16);
      }

      v = response->getVariable(3);
      if ((v != nullptr) && (v->getType() == ASN_OCTET_STRING))
      {
         _tcslcpy(hwInfo->serialNumber, v->getValueAsString(buffer, 256), 32);
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
 */
InterfaceList *MikrotikDriver::getInterfaces(SNMP_Transport *snmp, NObject *node, DriverData *driverData, bool useIfXTable)
{
    // Get interface list from standard MIB
    InterfaceList *ifList = NetworkDeviceDriver::getInterfaces(snmp, node, driverData, useIfXTable);
    if (ifList == nullptr)
        return nullptr;

   for(int i = 0; i < ifList->size(); i++)
   {
      InterfaceInfo *iface = ifList->get(i);
      if (iface->type == IFTYPE_ETHERNET_CSMACD)
      {
         iface->isPhysicalPort = true;
         iface->location.module = 1;
         iface->location.port = iface->index;
         iface->bridgePort = iface->index;
      }
   }
   return ifList;
}

/**
 * Translate LLDP port name (port ID subtype 5) to local interface id.
 *
 * @param snmp SNMP transport
 * @param node Node
 * @param driverData driver's data
 * @param lldpName port name received from LLDP MIB
 * @param id interface ID structure to be filled at success
 * @return true if interface identification provided
 */
bool MikrotikDriver::lldpNameToInterfaceId(SNMP_Transport *snmp, NObject *node, DriverData *driverData, const TCHAR *lldpName, InterfaceId *id)
{
   // bridged interfaces can be reported by Mikrotik device as bridge/interface or bridge/bonding/interface
   const TCHAR *s = _tcsrchr(lldpName, _T('/'));
   if (s != nullptr)
   {
      id->type = InterfaceIdType::NAME;
      _tcslcpy(id->value.ifName, s + 1, 192);
      return true;
   }
   return false;
}

/**
 * SNMP walker callback which just counts number of varbinds
 */
static uint32_t CountingSnmpWalkerCallback(SNMP_Variable *var, SNMP_Transport *transport, int *counter)
{
    (*counter)++;
    return SNMP_ERR_SUCCESS;
}

/**
 * Check switch for wireless capabilities
 *
 * @param snmp SNMP transport
 * @param attributes Node custom attributes
 * @param driverData optional pointer to user data
 */
bool MikrotikDriver::isWirelessController(SNMP_Transport *snmp, NObject *node, DriverData *driverData)
{
   int count = 0;
   SnmpWalk(snmp, _T(".1.3.6.1.4.1.14988.1.1.1.3.1.4"), CountingSnmpWalkerCallback, &count);
   return count > 0;
}

/**
 * Handler for access point enumeration - adopted
 */
static uint32_t HandlerAccessPointList(SNMP_Variable *var, SNMP_Transport *snmp, ObjectArray<AccessPointInfo> *apList)
{
   const SNMP_ObjectId& name = var->getName();
   size_t nameLen = name.length();
   uint32_t oid[MAX_OID_LEN];
   memcpy(oid, name.value(), nameLen * sizeof(uint32_t));
   uint32_t apIndex = oid[nameLen - 1];

   SNMP_PDU request(SNMP_GET_REQUEST, SnmpNewRequestId(), snmp->getSnmpVersion());

   oid[nameLen - 2] = 5;   // mtxrWlApBssid
   request.bindVariable(new SNMP_Variable(oid, nameLen));

   oid[nameLen - 2] = 7;   // mtxrWlApFreq
   request.bindVariable(new SNMP_Variable(oid, nameLen));

   SnmpParseOID(_T(".1.3.6.1.2.1.2.2.1.2"), oid, MAX_OID_LEN); // ifDescr
   oid[10] = apIndex;
   request.bindVariable(new SNMP_Variable(oid, 11));

   oid[9] = 6; // ifPhysAddress
   request.bindVariable(new SNMP_Variable(oid, 11));

   SNMP_PDU *response;
   if (snmp->doRequest(&request, &response) == SNMP_ERR_SUCCESS)
   {
      if (response->getNumVariables() == 4)
      {
         MacAddress macAddr = response->getVariable(0)->getValueAsMACAddr();

         // At least some Mikrotik devices returns empty BSSID - use radio interface MAC in that case
         if (!macAddr.isValid())
            macAddr = response->getVariable(3)->getValueAsMACAddr();

         TCHAR name[MAX_OBJECT_NAME];
         AccessPointInfo *ap = new AccessPointInfo(apIndex, macAddr, InetAddress::INVALID, AP_ADOPTED, var->getValueAsString(name, MAX_OBJECT_NAME), nullptr, nullptr, nullptr);

         TCHAR macAddrText[64];
         nxlog_debug_tag(DEBUG_TAG, 6, _T("AP: index=%d name=%s macAddr=%s"), apIndex, name, macAddr.toString(macAddrText));

         RadioInterfaceInfo radio;
         memset(&radio, 0, sizeof(RadioInterfaceInfo));
         response->getVariable(2)->getValueAsString(radio.name, 64);
         response->getVariable(3)->getRawValue(radio.macAddr, MAC_ADDR_LENGTH);
         radio.index = apIndex;
         radio.channel = WirelessFrequencyToChannel(response->getVariable(1)->getValueAsInt());

         ap->addRadioInterface(radio);
         apList->add(ap);
      }
      delete response;
   }

   return SNMP_ERR_SUCCESS;
}

/**
 * Get access points
 *
 * @param snmp SNMP transport
 * @param attributes Node custom attributes
 * @param driverData optional pointer to user data
 */
ObjectArray<AccessPointInfo> *MikrotikDriver::getAccessPoints(SNMP_Transport *snmp, NObject *node, DriverData *driverData)
{
   ObjectArray<AccessPointInfo> *apList = new ObjectArray<AccessPointInfo>(0, 16, Ownership::True);
   if (SnmpWalk(snmp, _T(".1.3.6.1.4.1.14988.1.1.1.3.1.4"), HandlerAccessPointList, apList) != SNMP_ERR_SUCCESS)
   {
      delete apList;
      return nullptr;
   }
   return apList;
}

/**
 * Get access point state
 *
 * @param snmp SNMP transport
 * @param node Node
 * @param driverData driver-specific data previously created in analyzeDevice
 * @param apIndex access point index
 * @param macAdddr access point MAC address
 * @param ipAddr access point IP address
 * @param radioInterfaces radio interfaces of this AP
 * @return state of access point or AP_UNKNOWN if it cannot be determined
 */
AccessPointState MikrotikDriver::getAccessPointState(SNMP_Transport *snmp, NObject *node, DriverData *driverData,
      uint32_t apIndex, const MacAddress &macAddr, const InetAddress &ipAddr, const StructArray<RadioInterfaceInfo>& radioInterfaces)
{
   return AP_ADOPTED;
}

/**
 * Handler for mobile units enumeration
 */
static uint32_t HandlerWirelessStationList(SNMP_Variable *var, SNMP_Transport *snmp, ObjectArray<WirelessStationInfo> *wsList)
{
   const SNMP_ObjectId& name = var->getName();
   uint32_t apIndex = name.getElement(name.length() - 1);

   WirelessStationInfo *info = new WirelessStationInfo;
   memset(info, 0, sizeof(WirelessStationInfo));
   var->getRawValue(info->macAddr, MAC_ADDR_LENGTH);
   info->ipAddr = InetAddress();
   info->vlan = 1;
   info->rfIndex = apIndex;
   info->apMatchPolicy = AP_MATCH_BY_RFINDEX;

   TCHAR oid[256];
   _sntprintf(oid, 256, _T(".1.3.6.1.4.1.14988.1.1.1.3.1.4.%u"), apIndex);
   if (SnmpGet(snmp->getSnmpVersion(), snmp, oid, NULL, 0, info->ssid, sizeof(info->ssid), SG_STRING_RESULT) != SNMP_ERR_SUCCESS)
   {
      info->ssid[0] = 0;
   }

   wsList->add(info);
   return SNMP_ERR_SUCCESS;
}

/**
 * Get registered wireless stations (clients)
 *
 * @param snmp SNMP transport
 * @param attributes Node custom attributes
 * @param driverData optional pointer to user data
 */
ObjectArray<WirelessStationInfo> *MikrotikDriver::getWirelessStations(SNMP_Transport *snmp, NObject *node, DriverData *driverData)
{
   ObjectArray<WirelessStationInfo> *wsList = new ObjectArray<WirelessStationInfo>(0, 16, Ownership::True);
   if (SnmpWalk(snmp, _T(".1.3.6.1.4.1.14988.1.1.1.2.1.1"),// mtxrWlRtabAddr
                HandlerWirelessStationList, wsList) != SNMP_ERR_SUCCESS)
   {
      delete wsList;
      wsList = nullptr;
   }
   return wsList;
}

/**
 * Get list of metrics supported by driver
 *
 * @param snmp SNMP transport
 * @param node Node
 * @param driverData driver-specific data previously created in analyzeDevice
 * @return list of metrics supported by driver or NULL on error
 */

ObjectArray<AgentParameterDefinition> *MikrotikDriver::getAvailableMetrics(SNMP_Transport *snmp, NObject *node, DriverData *driverData)
{
   ObjectArray<AgentParameterDefinition> *metrics = new ObjectArray<AgentParameterDefinition>(16, 16, Ownership::True);
   registerHostMibMetrics(metrics);
   static_cast<MikrotikDriverData*>(driverData)->registerMetrics(metrics);
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
DataCollectionError MikrotikDriver::getMetric(SNMP_Transport *snmp, NObject *node, DriverData *driverData, const TCHAR *name, TCHAR *value, size_t size)
{
   if (driverData == nullptr)
      return DCE_COLLECTION_ERROR;

   MikrotikDriverData *d = static_cast<MikrotikDriverData*>(driverData);
   TCHAR oid[64];
   if (!d->getMetricOID(name, snmp, oid))
   {
      nxlog_debug_tag(DEBUG_TAG, 7, _T("MikrotikDriver::getMetric(%s [%u]): OID for metric \"%s\" not found"), driverData->getNodeName(), driverData->getNodeId(), name);
      return DCE_NOT_SUPPORTED;
   }

   nxlog_debug_tag(DEBUG_TAG, 7, _T("MikrotikDriver::getMetric(%s [%u]): metric name \"%s\" resolved to OID %s"), driverData->getNodeName(), driverData->getNodeId(), name, oid);
   return (SnmpGetEx(snmp, oid, nullptr, 0, value, size * sizeof(TCHAR), SG_STRING_RESULT) == SNMP_ERR_SUCCESS) ? DCE_SUCCESS : DCE_COMM_ERROR;
}

/**
 * Check if driver can provide additional metrics
 */
bool MikrotikDriver::hasMetrics()
{
   return true;
}

/**
 * Driver entry point
 */
DECLARE_NDD_ENTRY_POINT(MikrotikDriver);

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
