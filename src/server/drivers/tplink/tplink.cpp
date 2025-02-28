/*
 ** NetXMS - Network Management System
 ** Driver for TP-Link  switches
 **
 ** Licensed under MIT lisence, as stated by the original
 ** author: https://dev.raden.solutions/issues/779#note-4
 **
 ** Copyright (c) 2015 Procyshin Dmitriy
 ** Copyright (c) 2023-2024 Raden Solutions
 ** Copyleft (l) 2023 Anatoly Rudnev
 **
 ** Permission is hereby granted, free of charge, to any person obtaining a copy of
 ** this software and associated documentation files (the “Software”), to deal in
 ** the Software without restriction, including without limitation the rights to
 ** use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 ** of the Software, and to permit persons to whom the Software is furnished to do
 ** so, subject to the following conditions:
 **
 ** The above copyright notice and this permission notice shall be included in all
 ** copies or substantial portions of the Software.
 **
 ** THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 ** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 ** FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 ** AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 ** LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 ** OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 ** SOFTWARE.
 **
 ** File: tplink.cpp
 **/

#include "tplink.h"
#include <netxms-version.h>
#include <netxms-regex.h>

#define DEBUG_TAG_TPLINK _T("ndd.tplink")

/**
 * Get driver name
 */
const TCHAR* TPLinkDriver::getName()
{
   return _T("TPLINK");
}

/**
 * Get driver version
 */
const TCHAR* TPLinkDriver::getVersion()
{
   return NETXMS_VERSION_STRING;
}

/**
 * Check if given device can be potentially supported by driver
 *
 * @param oid Device OID
 */
int TPLinkDriver::isPotentialDevice(const SNMP_ObjectId& oid)
{
   return oid.startsWith({ 1, 3, 6, 1, 4, 1, 11863, 5 }) ? 127 : 0;
}

/**
 * Check if given device is supported by driver
 *
 * @param snmp SNMP transport
 * @param oid Device OID
 */
bool TPLinkDriver::isDeviceSupported(SNMP_Transport *snmp, const SNMP_ObjectId& oid)
{
   return true;
}

/**
 * Get list of interfaces for given node
 *
 * @param snmp SNMP transport
 * @param node Node
 */
InterfaceList* TPLinkDriver::getInterfaces(SNMP_Transport *snmp, NObject *node, DriverData *driverData, bool useIfXTable)
{
   InterfaceList *ifList = NetworkDeviceDriver::getInterfaces(snmp, node, driverData, false);
   if (ifList == nullptr)
      return nullptr;

   // Include only ethernet interfaces in a physical port list (previous version used ifIndex based computation)
   // algorithm borrowed from Cisco Generic driver
   const char *eptr;
   int eoffset;
   PCRE *reFex = _pcre_compile_t(
         reinterpret_cast<const PCRE_TCHAR*>(_T("^(gigabitEthernet|ten-gigabitEthernet) ([0-9]+)/([0-9]+)/([0-9]+)$")),
         PCRE_COMMON_FLAGS | PCRE_CASELESS, &eptr, &eoffset, nullptr);
   if (reFex == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG_TPLINK, 6, _T("TPlinkDriver::getInterfaces: cannot compile FEX regexp: %hs at offset %d"), eptr, eoffset);
      return ifList;
   }

   int pmatch[30];
   for (int i = 0; i < ifList->size(); i++)
   {
      InterfaceInfo *iface = ifList->get(i);
      if (_pcre_exec_t(reFex, nullptr, reinterpret_cast<PCRE_TCHAR*>(iface->name), static_cast<int>(_tcslen(iface->name)), 0, 0, pmatch, 30) == 5)
      {
         nxlog_debug_tag(DEBUG_TAG_TPLINK, 6, _T("TPLinkDriver::getInterfaces: ifName=\"%s\" ifIndex=%u"), iface->name, iface->index);
         iface->isPhysicalPort = true;
         iface->location.chassis = IntegerFromCGroup(iface->name, pmatch, 2);
         iface->location.module = IntegerFromCGroup(iface->name, pmatch, 3);
         iface->location.port = IntegerFromCGroup(iface->name, pmatch, 4);
      }
   }

   _pcre_free_t(reFex);
   return ifList;
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
bool TPLinkDriver::getHardwareInformation(SNMP_Transport *snmp, NObject *node, DriverData *driverData, DeviceHardwareInfo *hwInfo)
{
   _tcscpy(hwInfo->vendor, _T("TP-Link"));

   SNMP_PDU request(SNMP_GET_REQUEST, SnmpNewRequestId(), snmp->getSnmpVersion());
   request.bindVariable(new SNMP_Variable(_T(".1.3.6.1.4.1.11863.6.1.1.6.0"))); // Product code
   request.bindVariable(new SNMP_Variable(_T(".1.3.6.1.4.1.11863.6.1.1.1.0"))); // Product name
   request.bindVariable(new SNMP_Variable(_T(".1.3.6.1.4.1.11863.6.1.1.5.0"))); // Firmware version
   request.bindVariable(new SNMP_Variable(_T(".1.3.6.1.4.1.11863.6.1.1.8.0"))); // Serial number

   SNMP_PDU *response;
   if (snmp->doRequest(&request, &response) == SNMP_ERR_SUCCESS)
   {
      TCHAR buffer[256];

      const SNMP_Variable *v = response->getVariable(0);
      if ((v != nullptr) && (v->getType() == ASN_OCTET_STRING))
      {
         _tcslcpy(hwInfo->productCode, v->getValueAsString(buffer, 256), 32);
      }

      v = response->getVariable(1);
      if ((v != nullptr) && (v->getType() == ASN_OCTET_STRING))
      {
         _tcslcpy(hwInfo->productName, v->getValueAsString(buffer, 256), 128);
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
 * Returns true if lldpRemLocalPortNum is expected to be valid interface index or bridge port number.
 * Default implementation always return true.
 *
 * @param node Node
 * @param driverData driver-specific data previously created in analyzeDevice
 * @return true if lldpRemLocalPortNum is expected to be valid
 */
bool TPLinkDriver::isValidLldpRemLocalPortNum(const NObject *node, DriverData *driverData)
{
   return false;
}

/**
 * Handler for Ports enumeration
 *
 * Same algorithm for tagged and access ports
 * VLAN ports returned as string contained mix of single port 1/0/X, port
 * ranges 1/0/X-Y or in a simplest form - single ports 1/0/X or empty string
 * 1/0/1,1/0/3-5,1/0/7-9,1/0/11-13,1/0/15-17,1/0/20-21,1/0/24-28
 */
static uint32_t ParseVlanPorts(SNMP_Variable *var, SNMP_Transport *transport, VlanList *vlanList)
{
   uint32_t vlanId = var->getName().getLastElement();

   TCHAR buffer[1024];
   var->getValueAsString(buffer, 1024);

   StringList items = String::split(buffer, _T(","));
   for (int j = 0; j < items.size(); j++)
   {
      wchar_t dataLine[256];
      wcslcpy(dataLine, items.get(j), 256);

      // Split portion 1/0/1-3 or 1/0/1 to 1-3 or 1
      // We should get 3 elements and need only 3rd items
      wchar_t *portsSource = wcschr(dataLine, L'/');
      if (portsSource != nullptr)
      {
         // by now we assume ports in vlan assignmed named as '1/0/XX'
         // because pointer contain position of MODULE as '0/' - whe shold increase pointer to skip 0 and SLASH  
         portsSource += 3;

         nxlog_debug_tag(DEBUG_TAG_TPLINK, 7, _T("ParseVlanPorts: VLAN: %u; input: \"%s\"; extracted: \"%s\""), vlanId, dataLine, portsSource);
         wchar_t *separator = wcschr(portsSource, L'-');
         if (separator != nullptr)
         {
            *separator = 0;
            separator++;
            // we get port range with start_port and end_port
            // 1/0/XX-YY - extracted as XX-YY
            // we need to convert string to uint32 and add 49152 to get proper ifIndex
            uint32_t start = _tcstoul(portsSource, nullptr, 10) + 49152;
            uint32_t end = _tcstoul(separator, nullptr, 10) + 49152;
            nxlog_debug_tag(DEBUG_TAG_TPLINK, 7, _T("ParseVlanPorts: VLAN: %u; port range start: 1/0/%s (ifIndex: %u); end: 1/0/%s (ifIndex %u)"),
                     vlanId, portsSource, start, separator, end);
            for (uint32_t port = start; port <= end; port++)
            {
               vlanList->addMemberPort(vlanId, port);
            }
         }
         else
         {
            uint32_t port = _tcstoul(portsSource, nullptr, 10) + 49152;
            nxlog_debug_tag(DEBUG_TAG_TPLINK, 7, _T("ParseVlanPorts: VLAN: %u; matched \"%s\"; single port: \"%s\" (ifIndex: %u)"),
                     vlanId, portsSource, portsSource, port);
            vlanList->addMemberPort(vlanId, port);
         }
      }
   }

   return SNMP_ERR_SUCCESS;
}

/**
 * Handler for VLAN enumeration
 */
static uint32_t HandlerVlanList(SNMP_Variable *var, SNMP_Transport *transport, VlanList *vlanList)
{
   VlanInfo *vlan = new VlanInfo(var->getName().getLastElement(), VLAN_PRM_IFINDEX);

   TCHAR buffer[256];
   vlan->setName(var->getValueAsString(buffer, 256));

   vlanList->add(vlan);
   return SNMP_ERR_SUCCESS;
}

/**
 * Get list of VLANs on given node
 *
 * @param snmp SNMP transport
 * @param node Node
 * @param driverData driver-specific data previously created in analyzeDevice
 * @return VLAN list or NULL
 */
VlanList* TPLinkDriver::getVlans(SNMP_Transport *snmp, NObject *node, DriverData *driverData)
{
   auto list = new VlanList();

   nxlog_debug_tag(DEBUG_TAG_TPLINK, 5, _T("TPLinkDriver::getVlans: Processing VLANs for nodeId %u"), node->getId());

   if (SnmpWalk(snmp, _T(".1.3.6.1.4.1.11863.6.14.1.2.1.1.2"), HandlerVlanList, list) != SNMP_ERR_SUCCESS)
   {
      delete list;
      return nullptr;
   }

   // Process TAGGED ports
   nxlog_debug_tag(DEBUG_TAG_TPLINK, 7, _T("TPLinkDriver::getVlans(%s [%u]): Parsing TRUNK ports to VALN bindings"), node->getName(), node->getId());
   if (SnmpWalk(snmp, _T(".1.3.6.1.4.1.11863.6.14.1.2.1.1.3"), ParseVlanPorts, list) != SNMP_ERR_SUCCESS)
   {
      nxlog_debug_tag(DEBUG_TAG_TPLINK, 5, _T("TPLinkDriver::getVlans(%s [%u]): Can't process TAGGED ports to VALN bindings"), node->getName(), node->getId());
   }

   // Process ACCESS ports
   nxlog_debug_tag(DEBUG_TAG_TPLINK, 7, _T("TPLinkDriver::getVlans(%s [%u]): Parsing ACCESS ports to VALN bindings"), node->getName(), node->getId());
   if (SnmpWalk(snmp, _T(".1.3.6.1.4.1.11863.6.14.1.2.1.1.4"), ParseVlanPorts, list) != SNMP_ERR_SUCCESS)
   {
      nxlog_debug_tag(DEBUG_TAG_TPLINK, 5, _T("TPLinkDriver::getVlans(%s [%u]): Can't process ACCESS ports to VLAN bindings"), node->getName(), node->getId());
   }

   return list;
}

/**
 * Driver entry point
 */
DECLARE_NDD_ENTRY_POINT (TPLinkDriver);

#ifdef _WIN32

/**
 * DLL entry point
 */
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved) {
  if (dwReason == DLL_PROCESS_ATTACH)
    DisableThreadLibraryCalls(hInstance);
  return TRUE;
}

#endif
