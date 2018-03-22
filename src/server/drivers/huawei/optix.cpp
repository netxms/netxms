/* 
** NetXMS - Network Management System
** Driver for Huawei Optix devices
** Copyright (C) 2003-2018 Victor Kirhenshtein
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
	return NETXMS_BUILD_TAG;
}

/**
 * Check if given device can be potentially supported by driver
 *
 * @param oid Device OID
 */
int OptixDriver::isPotentialDevice(const TCHAR *oid)
{
	return !_tcsicmp(oid, _T(".1.3.6.1.4.1.2011.2.25.1")) || !_tcsicmp(oid, _T(".1.3.6.1.4.1.2011.2.25.2")) ? 255 : 0;
}

/**
 * Check if given device is supported by driver
 *
 * @param snmp SNMP transport
 * @param oid Device OID
 */
bool OptixDriver::isDeviceSupported(SNMP_Transport *snmp, const TCHAR *oid)
{
	return true;
}

/**
 * Do additional checks on the device required by driver.
 * Driver can set device's custom attributes from within
 * this function.
 *
 * @param snmp SNMP transport
 * @param attributes Node's custom attributes
 */
void OptixDriver::analyzeDevice(SNMP_Transport *snmp, const TCHAR *oid, StringMap *attributes, DriverData **driverData)
{
}

/**
 * Handler for IP address enumeration
 */
static UINT32 HandlerIpAddrList(SNMP_Variable *var, SNMP_Transport *snmp, void *arg)
{
   InterfaceList *ifList = static_cast<InterfaceList*>(arg);

   SNMP_ObjectId oid = var->getName();
   UINT32 slot = oid.getElement(oid.length() - 3);
   UINT32 port = oid.getElement(oid.length() - 1);

   InterfaceInfo *iface = ifList->findByPhyPosition(slot, port);
   if (iface == NULL)
   {
      nxlog_debug_tag(DEBUG_TAG, 7, _T("HandlerIpAddrList: Cannot find interface object for %d/%d"), slot, port);
      return SNMP_ERR_SUCCESS;
   }

   SNMP_PDU request(SNMP_GET_REQUEST, SnmpNewRequestId(), snmp->getSnmpVersion());
   oid.changeElement(15, 4); // optixL3PortIpMask
   request.bindVariable(new SNMP_Variable(oid));

   SNMP_PDU *response;
   if (snmp->doRequest(&request, &response, SnmpGetDefaultTimeout(), 3) == SNMP_ERR_SUCCESS)
   {
      if (response->getNumVariables() == request.getNumVariables())
      {
         TCHAR ipAddrStr[64], ipNetMaskStr[64];
         InetAddress addr = InetAddress::parse(var->getValueAsString(ipAddrStr, 64));
         InetAddress mask = InetAddress::parse(response->getVariable(0)->getValueAsString(ipNetMaskStr, 64));
         nxlog_debug_tag(DEBUG_TAG, 7, _T("HandlerIpAddrList: got IP address/mask %s/%s for interface %d/%d"), addr.toString(ipAddrStr), mask.toString(ipNetMaskStr), slot, port);
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
 * Handler for Ethernet ports enumeration
 */
static UINT32 HandlerEthPortList(SNMP_Variable *var, SNMP_Transport *snmp, void *arg)
{
   InterfaceList *ifList = static_cast<InterfaceList*>(arg);

   InterfaceInfo *iface = new InterfaceInfo(0);
   iface->isPhysicalPort = true;
   iface->type = IFTYPE_ETHERNET_CSMACD;

   SNMP_ObjectId oid = var->getName();
   iface->slot = oid.getElement(oid.length() - 3);
   iface->port = oid.getElement(oid.length() - 1);
   iface->index = (iface->slot << 8) | iface->port;

   iface->ifTableSuffixLength = 3;
   memcpy(iface->ifTableSuffix, oid.value() + oid.length() - 3, 3 * sizeof(UINT32));

   _sntprintf(iface->name, MAX_DB_STRING, _T("%d/%d"), iface->slot, iface->port);
   _tcscpy(iface->description, iface->name);

   TCHAR macAddrText[64];
   memcpy(iface->macAddr, MacAddress::parse(var->getValueAsString(macAddrText, 64)).value(), MAC_ADDR_LENGTH);

   SNMP_PDU request(SNMP_GET_REQUEST, SnmpNewRequestId(), snmp->getSnmpVersion());
   oid.changeElement(15, 3); // optixEthPortMtu
   request.bindVariable(new SNMP_Variable(oid));

   oid.changeElement(15, 4); // optixEthPortWorkMode
   request.bindVariable(new SNMP_Variable(oid));

   SNMP_PDU *response;
   if (snmp->doRequest(&request, &response, SnmpGetDefaultTimeout(), 3) == SNMP_ERR_SUCCESS)
   {
      if (response->getNumVariables() == request.getNumVariables())
      {
         iface->mtu = response->getVariable(0)->getValueAsUInt();
         switch(response->getVariable(1)->getValueAsUInt())
         {
            case 1:
            case 2:
               iface->speed = 10000000;
               break;
            case 3:
            case 4:
               iface->speed = 100000000;
               break;
            case 5:
            case 6:
               iface->speed = 1000000000;
               break;
            case 7:
            case 8:
               iface->speed = 10000000000;
               break;
            default:
               iface->speed = 0;
               break;
         }
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
 * @param attributes Node's custom attributes
 */
InterfaceList *OptixDriver::getInterfaces(SNMP_Transport *snmp, StringMap *attributes, DriverData *driverData, int useAliases, bool useIfXTable)
{
	InterfaceList *ifList = new InterfaceList();

	// Walk ethernet ports
   if (SnmpWalk(snmp, _T(".1.3.6.1.4.1.2011.2.25.4.50.52.1.1.1.23"), // optixEthPortMac
                HandlerEthPortList, ifList) != SNMP_ERR_SUCCESS)
   {
      delete ifList;
      return NULL;
   }

   // Walk IP addresses
   if (SnmpWalk(snmp, _T(".1.3.6.1.4.1.2011.2.25.4.50.52.1.5.1.3"), // optixL3PortIpAddr
                HandlerIpAddrList, ifList) != SNMP_ERR_SUCCESS)
   {
      delete ifList;
      return NULL;
   }

	return ifList;
}

/**
 * Get interface state. Both states must be set to UNKNOWN if cannot be read from device.
 *
 * @param snmp SNMP transport
 * @param attributes node's custom attributes
 * @param driverData driver's data
 * @param ifIndex interface index
 * @param ifTableSuffixLen length of interface table suffix
 * @param ifTableSuffix interface table suffix
 * @param adminState OUT: interface administrative state
 * @param operState OUT: interface operational state
 */
void OptixDriver::getInterfaceState(SNMP_Transport *snmp, StringMap *attributes, DriverData *driverData, UINT32 ifIndex,
                                    int ifTableSuffixLen, UINT32 *ifTableSuffix, InterfaceAdminState *adminState, InterfaceOperState *operState)
{
   *adminState = IF_ADMIN_STATE_UNKNOWN;
   *operState = IF_OPER_STATE_UNKNOWN;

   SNMP_PDU request(SNMP_GET_REQUEST, SnmpNewRequestId(), snmp->getSnmpVersion());

   SNMP_ObjectId oid = SNMP_ObjectId::parse(_T(".1.3.6.1.4.1.2011.2.25.4.50.52.1.1.1.18"));  // optixEthPortEnable
   oid.extend(ifTableSuffix, ifTableSuffixLen);
   request.bindVariable(new SNMP_Variable(oid));

   oid.changeElement(15, 37); // optixEthPortUpDownStauts
   request.bindVariable(new SNMP_Variable(oid));

   SNMP_PDU *response;
   if (snmp->doRequest(&request, &response, SnmpGetDefaultTimeout(), 3) == SNMP_ERR_SUCCESS)
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
      }
      delete response;
   }
}

/**
 * Handler for ARP enumeration
 */
static UINT32 HandlerArp(SNMP_Variable *var, SNMP_Transport *transport, void *arg)
{
   // IP address used as table element index - each OID element at range 16..19 represents single byte
   const SNMP_ObjectId& oid = var->getName();
   UINT32 ipAddr = (oid.getElement(16) << 24) | (oid.getElement(17) << 16) | (oid.getElement(18) << 8) | oid.getElement(19);
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
ArpCache *OptixDriver::getArpCache(SNMP_Transport *snmp, DriverData *driverData)
{
   ArpCache *arpCache = new ArpCache();
   if (SnmpWalk(snmp, _T(".1.3.6.1.4.1.2011.2.25.4.50.23.1.1.1.3"), HandlerArp, arpCache) != SNMP_ERR_SUCCESS)
   {
      arpCache->decRefCount();
      arpCache = NULL;
   }
   return arpCache;
}
