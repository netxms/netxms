/* 
** NetXMS - Network Management System
** Driver for Huawei Optix devices
** Copyright (C) 2003-2024 Raden Solutions
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
** File: optix.cpp
**/

#include "huawei.h"

#define DEBUG_TAG _T("ndd.huawei.optix")

/**
 * Get driver name
 */
const TCHAR *OptixDriver::getName()
{
   return _T("OPTIX");
}

/**
 * Get driver version
 */
const TCHAR *OptixDriver::getVersion()
{
   return NETXMS_VERSION_STRING;
}

/**
 * Check if given device can be potentially supported by driver
 *
 * @param oid Device OID
 */
int OptixDriver::isPotentialDevice(const SNMP_ObjectId& oid)
{
	return oid.equals({ 1, 3, 6, 1, 4, 1, 2011, 2, 25, 1 }) || oid.equals({ 1, 3, 6, 1, 4, 1, 2011, 2, 25, 2 }) ? 255 : 0;
}

/**
 * Check if given device is supported by driver
 *
 * @param snmp SNMP transport
 * @param oid Device OID
 */
bool OptixDriver::isDeviceSupported(SNMP_Transport *snmp, const SNMP_ObjectId& oid)
{
	return true;
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
bool OptixDriver::getHardwareInformation(SNMP_Transport *snmp, NObject *node, DriverData *driverData, DeviceHardwareInfo *hwInfo)
{
   _tcscpy(hwInfo->vendor, _T("Huawei"));
   return true;
}

/**
 * Handler for IP address enumeration
 */
static uint32_t HandlerIpAddrList(SNMP_Variable *var, SNMP_Transport *snmp, InterfaceList *ifList)
{
   SNMP_ObjectId oid = var->getName();
   uint32_t module = oid.getElement(oid.length() - 3);
   uint32_t pic = oid.getElement(oid.length() - 2);
   uint32_t port = oid.getElement(oid.length() - 1);

   InterfaceInfo *iface = ifList->findByPhysicalLocation(0, module, pic, port);
   if (iface == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 7, _T("HandlerIpAddrList: Cannot find interface object for %u/%u/%u"), module, pic, port);
      return SNMP_ERR_SUCCESS;
   }

   SNMP_PDU request(SNMP_GET_REQUEST, SnmpNewRequestId(), snmp->getSnmpVersion());
   oid.changeElement(15, 4); // optixL3PortIpMask
   request.bindVariable(new SNMP_Variable(oid));

   SNMP_PDU *response;
   if (snmp->doRequest(&request, &response) == SNMP_ERR_SUCCESS)
   {
      if (response->getNumVariables() == request.getNumVariables())
      {
         TCHAR ipAddrStr[64], ipNetMaskStr[64];
         InetAddress addr = InetAddress::parse(var->getValueAsString(ipAddrStr, 64));
         InetAddress mask = InetAddress::parse(response->getVariable(0)->getValueAsString(ipNetMaskStr, 64));
         nxlog_debug_tag(DEBUG_TAG, 7, _T("HandlerIpAddrList: got IP address/mask %s/%s for interface %u/%u/%u"),
                  addr.toString(ipAddrStr), mask.toString(ipNetMaskStr), module, pic, port);
         if (addr.isValid() && mask.isValid())
         {
            addr.setMaskBits(BitsInMask(mask.getAddressV4()));
            iface->ipAddrList.add(addr);
         }
      }
      delete response;
   }

   return SNMP_ERR_SUCCESS;
}

/**
 * Get interface speed from work mode
 */
static inline uint64_t SpeedFromWorkMode(uint32_t mode)
{
   switch(mode)
   {
      case 1:
      case 2:
         return _ULL(10000000);
      case 3:
      case 4:
         return _ULL(100000000);
      case 5:
      case 6:
         return _ULL(1000000000);
      case 7:
      case 8:
         return _ULL(10000000000);
      default:
         return 0;
   }
}

/**
 * Handler for Ethernet ports enumeration
 */
static uint32_t HandlerEthPortList(SNMP_Variable *var, SNMP_Transport *snmp, InterfaceList *ifList)
{
   InterfaceInfo *iface = new InterfaceInfo(0);
   iface->isPhysicalPort = true;
   iface->type = IFTYPE_ETHERNET_CSMACD;

   SNMP_ObjectId oid = var->getName();
   iface->location.module = oid.getElement(oid.length() - 3);
   iface->location.pic = oid.getElement(oid.length() - 1);
   iface->location.port = oid.getElement(oid.length() - 1);
   iface->index = (iface->location.chassis << 24) | (iface->location.module << 16) | (iface->location.pic << 8) | iface->location.port;

   iface->ifTableSuffixLength = 3;
   memcpy(iface->ifTableSuffix, oid.value() + oid.length() - 3, 3 * sizeof(UINT32));

   _sntprintf(iface->name, MAX_DB_STRING, _T("%u/%u/%u"), iface->location.module, iface->location.pic, iface->location.port);
   _tcscpy(iface->description, iface->name);

   TCHAR macAddrText[64];
   memcpy(iface->macAddr, MacAddress::parse(var->getValueAsString(macAddrText, 64)).value(), MAC_ADDR_LENGTH);

   SNMP_PDU request(SNMP_GET_REQUEST, SnmpNewRequestId(), snmp->getSnmpVersion());
   oid.changeElement(15, 3); // optixEthPortMtu
   request.bindVariable(new SNMP_Variable(oid));

   oid.changeElement(15, 4); // optixEthPortWorkMode
   request.bindVariable(new SNMP_Variable(oid));

   SNMP_PDU *response;
   if (snmp->doRequest(&request, &response) == SNMP_ERR_SUCCESS)
   {
      if (response->getNumVariables() == request.getNumVariables())
      {
         iface->mtu = response->getVariable(0)->getValueAsUInt();
         iface->speed = SpeedFromWorkMode(response->getVariable(1)->getValueAsUInt());
      }
      delete response;
   }

   ifList->add(iface);
   return SNMP_ERR_SUCCESS;
}

/**
 * Get list of interfaces for given node
 *
 * @param snmp SNMP transport
 * @param node Node
 */
InterfaceList *OptixDriver::getInterfaces(SNMP_Transport *snmp, NObject *node, DriverData *driverData, bool useIfXTable)
{
   InterfaceList *ifList = new InterfaceList();

	// Walk ethernet ports
   if (SnmpWalk(snmp, _T(".1.3.6.1.4.1.2011.2.25.4.50.52.1.1.1.23"), HandlerEthPortList, ifList) != SNMP_ERR_SUCCESS) // optixEthPortMac
   {
      delete ifList;
      return nullptr;
   }

   // Walk IP addresses
   if (SnmpWalk(snmp, _T(".1.3.6.1.4.1.2011.2.25.4.50.52.1.5.1.3"), HandlerIpAddrList, ifList) != SNMP_ERR_SUCCESS) // optixL3PortIpAddr
   {
      delete ifList;
      return nullptr;
   }

   return ifList;
}

/**
 * Get interface state. Both states must be set to UNKNOWN if cannot be read from device.
 *
 * @param snmp SNMP transport
 * @param node Node
 * @param driverData driver's data
 * @param ifIndex interface index
 * @param ifName interface name
 * @param ifType interface type
 * @param ifTableSuffixLen length of interface table suffix
 * @param ifTableSuffix interface table suffix
 * @param adminState OUT: interface administrative state
 * @param operState OUT: interface operational state
 * @param speed OUT: updated interface speed
 */
void OptixDriver::getInterfaceState(SNMP_Transport *snmp, NObject *node, DriverData *driverData, uint32_t ifIndex, const TCHAR *ifName,
         uint32_t ifType, int ifTableSuffixLen, const uint32_t *ifTableSuffix, InterfaceAdminState *adminState, InterfaceOperState *operState, uint64_t *speed)
{
   *adminState = IF_ADMIN_STATE_UNKNOWN;
   *operState = IF_OPER_STATE_UNKNOWN;

   SNMP_PDU request(SNMP_GET_REQUEST, SnmpNewRequestId(), snmp->getSnmpVersion());

   SNMP_ObjectId oid = SNMP_ObjectId::parse(_T(".1.3.6.1.4.1.2011.2.25.4.50.52.1.1.1.18"));  // optixEthPortEnable
   oid.extend(ifTableSuffix, ifTableSuffixLen);
   request.bindVariable(new SNMP_Variable(oid));

   oid.changeElement(15, 37); // optixEthPortUpDownStauts
   request.bindVariable(new SNMP_Variable(oid));

   oid.changeElement(15, 4); // optixEthPortWorkMode
   request.bindVariable(new SNMP_Variable(oid));

   SNMP_PDU *response;
   if (snmp->doRequest(&request, &response) == SNMP_ERR_SUCCESS)
   {
      if (response->getNumVariables() == request.getNumVariables())
      {
         if (response->getVariable(0)->getValueAsInt() == 1) // port enabled
         {
            *adminState = IF_ADMIN_STATE_UP;
            *operState = (response->getVariable(1)->getValueAsInt() == 1) ? IF_OPER_STATE_UP : IF_OPER_STATE_DOWN;
         }
         else
         {
            *adminState = IF_ADMIN_STATE_DOWN;
            *operState = IF_OPER_STATE_DOWN;
         }
         *speed = SpeedFromWorkMode(response->getVariable(2)->getValueAsUInt());
      }
      delete response;
   }
}

/**
 * Handler for ARP enumeration
 */
static uint32_t HandlerArp(SNMP_Variable *var, SNMP_Transport *transport, void *arg)
{
   // IP address used as table element index - each OID element at range 16..19 represents single byte
   const SNMP_ObjectId& oid = var->getName();
   uint32_t ipAddr = (oid.getElement(16) << 24) | (oid.getElement(17) << 16) | (oid.getElement(18) << 8) | oid.getElement(19);
   TCHAR buffer[256];
   MacAddress macAddr = MacAddress::parse(var->getValueAsString(buffer, 256));
   if (macAddr.isValid())
   {
      ArpCache *arpCache = static_cast<ArpCache*>(arg);
      arpCache->addEntry(ipAddr, macAddr);
   }
   return SNMP_ERR_SUCCESS;
}

/**
 * Get ARP cache
 *
 * @param snmp SNMP transport
 * @param driverData driver-specific data previously created in analyzeDevice (must be derived from HostMibDriverData)
 * @return ARP cache or NULL on failure
 */
shared_ptr<ArpCache> OptixDriver::getArpCache(SNMP_Transport *snmp, DriverData *driverData)
{
   shared_ptr<ArpCache> arpCache = make_shared<ArpCache>();
   if (SnmpWalk(snmp, _T(".1.3.6.1.4.1.2011.2.25.4.50.23.1.1.1.3"), HandlerArp, arpCache.get()) != SNMP_ERR_SUCCESS)
      arpCache.reset();
   return arpCache;
}
