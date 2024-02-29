/* 
** NetXMS - Network Management System
** Driver for TelcoBridges gateways
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
** File: tb.cpp
**/

#include "tb.h"
#include <netxms-version.h>

#define DEBUG_TAG _T("ndd.tb")

/**
 * Driver name
 */
static TCHAR s_driverName[] = _T("TB");

/**
 * Driver version
 */
static TCHAR s_driverVersion[] = NETXMS_VERSION_STRING;

/**
 * Get driver name
 */
const TCHAR *TelcoBridgesDriver::getName()
{
	return s_driverName;
}

/**
 * Get driver version
 */
const TCHAR *TelcoBridgesDriver::getVersion()
{
	return s_driverVersion;
}

/**
 * Check if given device can be potentially supported by driver
 *
 * @param oid Device OID
 */
int TelcoBridgesDriver::isPotentialDevice(const SNMP_ObjectId& oid)
{
	return oid.startsWith({ 1, 3, 6, 1, 4, 1, 21776 }) ? 254 : 0;
}

/**
 * Check if given device is supported by driver
 *
 * @param snmp SNMP transport
 * @param oid Device OID
 */
bool TelcoBridgesDriver::isDeviceSupported(SNMP_Transport *snmp, const SNMP_ObjectId& oid)
{
   return true;
}

/**
 * Handler for enumerating indexes
 */
static uint32_t HandlerIndex(SNMP_Variable *var, SNMP_Transport *snmp, InterfaceList *ifList)
{
   if (var->getName().length() == 12)
   {
      const uint32_t *oid = var->getName().value();
      uint32_t ifIndex = (oid[10] << 12) | (oid[11] & 0x0FFF);
      ifList->add(new InterfaceInfo(ifIndex, 2, &oid[10]));
   }
   else
   {
      uint32_t ifIndex = var->getValueAsUInt();
      ifList->add(new InterfaceInfo(ifIndex, 1, &ifIndex));
   }
   return SNMP_ERR_SUCCESS;
}

/**
 * Handler for enumerating IP addresses via ipAddrTable
 */
static uint32_t HandlerIpAddr(SNMP_Variable *pVar, SNMP_Transport *pTransport, InterfaceList *ifList)
{
   uint32_t index, dwNetMask, dwResult;
   uint32_t oidName[MAX_OID_LEN];

   size_t nameLen = pVar->getName().length();
   memcpy(oidName, pVar->getName().value(), nameLen * sizeof(uint32_t));
   oidName[nameLen - 5] = 3;  // Retrieve network mask for this IP
   dwResult = SnmpGetEx(pTransport, nullptr, oidName, nameLen, &dwNetMask, sizeof(uint32_t), 0, nullptr);
   if (dwResult != SNMP_ERR_SUCCESS)
	{
		TCHAR buffer[1024];
		nxlog_debug_tag(DEBUG_TAG, 6, _T("TelcoBridgesDriver::getInterfaces(%p): SNMP GET %s failed (error %d)"),
			pTransport, SnmpConvertOIDToText(nameLen, oidName, buffer, 1024), (int)dwResult);
		// Continue walk even if we get error for some IP address
		// For example, some Cisco ASA versions reports IP when walking, but does not
		// allow to SNMP GET appropriate entry
      return SNMP_ERR_SUCCESS;
	}

   oidName[nameLen - 5] = 2;  // Retrieve interface index for this IP
   dwResult = SnmpGetEx(pTransport, nullptr, oidName, nameLen, &index, sizeof(uint32_t), 0, nullptr);
   if (dwResult == SNMP_ERR_SUCCESS)
   {
      InterfaceInfo *iface = ifList->findByIfIndex(index);
      if (iface == nullptr)
      {
         iface = new InterfaceInfo(index);
         _sntprintf(iface->name, MAX_DB_STRING, _T("ip%d"), index);
         _tcscpy(iface->description, iface->name);
         iface->type = IFTYPE_IP;
         ifList->add(iface);
      }
      iface->ipAddrList.add(InetAddress(ntohl(pVar->getValueAsUInt()), dwNetMask));
   }
	else
	{
		TCHAR buffer[1024];
		nxlog_debug_tag(DEBUG_TAG, 6, _T("TelcoBridgesDriver::getInterfaces(%p): SNMP GET %s failed (error %d)"),
			pTransport, SnmpConvertOIDToText(nameLen, oidName, buffer, 1024), (int)dwResult);
		dwResult = SNMP_ERR_SUCCESS;	// continue walk
	}
   return dwResult;
}

/**
 * Get list of interfaces for given node
 *
 * @param snmp SNMP transport
 * @param node Node
 */
InterfaceList *TelcoBridgesDriver::getInterfaces(SNMP_Transport *snmp, NObject *node, DriverData *driverData, int useAliases, bool useIfXTable)
{
   bool success = false;
   InterfaceList *ifList = new InterfaceList(1024);

	nxlog_debug_tag(DEBUG_TAG, 6, _T("TelcoBridgesDriver::getInterfaces(%p)"), snmp);

   // Gather interface indexes
   if (SnmpWalk(snmp, _T(".1.3.6.1.2.1.2.2.1.1"), HandlerIndex, ifList) == SNMP_ERR_SUCCESS)
   {
      // Enumerate interfaces
		for(int i = 0; i < ifList->size(); i++)
      {
			InterfaceInfo *iface = ifList->get(i);

         TCHAR suffix[64], oid[128], buffer[256];
         SnmpConvertOIDToText(iface->ifTableSuffixLength, iface->ifTableSuffix, suffix, 64);

			// Get interface description
	      _sntprintf(oid, 128, _T(".1.3.6.1.2.1.2.2.1.2%s"), suffix);
	      if (SnmpGet(snmp->getSnmpVersion(), snmp, oid, nullptr, 0, iface->description, MAX_DB_STRING * sizeof(TCHAR), 0) != SNMP_ERR_SUCCESS)
	         break;
	      _tcslcpy(iface->name, iface->description, MAX_DB_STRING);

         nxlog_debug_tag(DEBUG_TAG, 6, _T("TelcoBridgesDriver::getInterfaces(%p): processing interface %s (%s)"), snmp, iface->name, suffix);

         // Interface type
         _sntprintf(oid, 128, _T(".1.3.6.1.2.1.2.2.1.3%s"), suffix);
         if (SnmpGet(snmp->getSnmpVersion(), snmp, oid, nullptr, 0, &iface->type, sizeof(uint32_t), 0) != SNMP_ERR_SUCCESS)
			{
				iface->type = IFTYPE_OTHER;
			}

         // Interface MTU
         _sntprintf(oid, 128, _T(".1.3.6.1.2.1.2.2.1.4%s"), suffix);
         if (SnmpGet(snmp->getSnmpVersion(), snmp, oid, nullptr, 0, &iface->mtu, sizeof(uint32_t), 0) != SNMP_ERR_SUCCESS)
			{
				iface->mtu = 0;
			}

         // Interface speed
         _sntprintf(oid, 128, _T(".1.3.6.1.2.1.2.2.1.5%s"), suffix);  // ifSpeed
         uint32_t speed;
         if (SnmpGet(snmp->getSnmpVersion(), snmp, oid, nullptr, 0, &speed, sizeof(uint32_t), 0) == SNMP_ERR_SUCCESS)
         {
            iface->speed = (UINT64)speed;
         }
         else
         {
            iface->speed = 0;
         }

         // MAC address
         _sntprintf(oid, 128, _T(".1.3.6.1.2.1.2.2.1.6%s"), suffix);
         memset(buffer, 0, MAC_ADDR_LENGTH);
         if (SnmpGet(snmp->getSnmpVersion(), snmp, oid, nullptr, 0, buffer, 256, SG_RAW_RESULT) == SNMP_ERR_SUCCESS)
			{
	         memcpy(iface->macAddr, buffer, MAC_ADDR_LENGTH);
			}
			else
			{
				// Unable to get MAC address
	         memset(iface->macAddr, 0, MAC_ADDR_LENGTH);
			}
      }

      // Interface IP address'es and netmasks
		uint32_t error = SnmpWalk(snmp, _T(".1.3.6.1.2.1.4.20.1.1"), HandlerIpAddr, ifList);
      if (error == SNMP_ERR_SUCCESS)
      {
         success = true;
      }
		else
		{
			nxlog_debug_tag(DEBUG_TAG, 6, _T("TelcoBridgesDriver::getInterfaces(%p): SNMP WALK .1.3.6.1.2.1.4.20.1.1 failed (%s)"), snmp, SnmpGetErrorText(error));
		}
   }
	else
	{
		nxlog_debug_tag(DEBUG_TAG, 6, _T("TelcoBridgesDriver::getInterfaces(%p): SNMP WALK .1.3.6.1.2.1.2.2.1.1 failed"), snmp);
	}

   if (!success)
   {
      delete_and_null(ifList);
   }

	nxlog_debug_tag(DEBUG_TAG, 6, _T("TelcoBridgesDriver::getInterfaces(%p): completed, ifList=%p"), snmp, ifList);
   return ifList;
}

/**
 * Driver entry point
 */
DECLARE_NDD_ENTRY_POINT(TelcoBridgesDriver);

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
