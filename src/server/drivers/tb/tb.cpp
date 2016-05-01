/* 
** NetXMS - Network Management System
** Driver for TelcoBridges gateways
** Copyright (C) 2003-2016 Victor Kirhenshtein
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
int TelcoBridgesDriver::isPotentialDevice(const TCHAR *oid)
{
	return (_tcsncmp(oid, _T(".1.3.6.1.4.1.21776."), 19) == 0) ? 254 : 0;
}

/**
 * Check if given device is supported by driver
 *
 * @param snmp SNMP transport
 * @param oid Device OID
 */
bool TelcoBridgesDriver::isDeviceSupported(SNMP_Transport *snmp, const TCHAR *oid)
{
   return true;
}

/**
 * Handler for enumerating indexes
 */
static UINT32 HandlerIndex(SNMP_Variable *var, SNMP_Transport *snmp, void *arg)
{
   if (var->getName().length() == 12)
   {
      const UINT32 *oid = var->getName().value();
      UINT32 ifIndex = (oid[10] << 12) | (oid[11] & 0x0FFF);
      ((InterfaceList *)arg)->add(new InterfaceInfo(ifIndex, 2, &oid[10]));
   }
   else
   {
      UINT32 ifIndex = var->getValueAsUInt();
      ((InterfaceList *)arg)->add(new InterfaceInfo(ifIndex, 1, &ifIndex));
   }
   return SNMP_ERR_SUCCESS;
}

/**
 * Handler for enumerating IP addresses via ipAddrTable
 */
static UINT32 HandlerIpAddr(SNMP_Variable *pVar, SNMP_Transport *pTransport, void *pArg)
{
   UINT32 index, dwNetMask, dwResult;
   UINT32 oidName[MAX_OID_LEN];

   size_t nameLen = pVar->getName().length();
   memcpy(oidName, pVar->getName().value(), nameLen * sizeof(UINT32));
   oidName[nameLen - 5] = 3;  // Retrieve network mask for this IP
   dwResult = SnmpGetEx(pTransport, NULL, oidName, nameLen, &dwNetMask, sizeof(UINT32), 0, NULL);
   if (dwResult != SNMP_ERR_SUCCESS)
	{
		TCHAR buffer[1024];
		DbgPrintf(6, _T("TelcoBridgesDriver::getInterfaces(%p): SNMP GET %s failed (error %d)"), 
			pTransport, SNMPConvertOIDToText(nameLen, oidName, buffer, 1024), (int)dwResult);
		// Continue walk even if we get error for some IP address
		// For example, some Cisco ASA versions reports IP when walking, but does not
		// allow to SNMP GET appropriate entry
      return SNMP_ERR_SUCCESS;
	}

   oidName[nameLen - 5] = 2;  // Retrieve interface index for this IP
   dwResult = SnmpGetEx(pTransport, NULL, oidName, nameLen, &index, sizeof(UINT32), 0, NULL);
   if (dwResult == SNMP_ERR_SUCCESS)
   {
		InterfaceList *ifList = (InterfaceList *)pArg;
      InterfaceInfo *iface = ifList->findByIfIndex(index);
      if (iface == NULL)
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
		DbgPrintf(6, _T("TelcoBridgesDriver::getInterfaces(%p): SNMP GET %s failed (error %d)"), 
			pTransport, SNMPConvertOIDToText(nameLen, oidName, buffer, 1024), (int)dwResult);
		dwResult = SNMP_ERR_SUCCESS;	// continue walk
	}
   return dwResult;
}

/**
 * Get list of interfaces for given node
 *
 * @param snmp SNMP transport
 * @param attributes Node's custom attributes
 */
InterfaceList *TelcoBridgesDriver::getInterfaces(SNMP_Transport *snmp, StringMap *attributes, DriverData *driverData, int useAliases, bool useIfXTable)
{
   bool success = false;
   InterfaceList *ifList = new InterfaceList(1024);

	DbgPrintf(6, _T("TelcoBridgesDriver::getInterfaces(%p)"), snmp);

   // Gather interface indexes
   if (SnmpWalk(snmp, _T(".1.3.6.1.2.1.2.2.1.1"), HandlerIndex, ifList) == SNMP_ERR_SUCCESS)
   {
      // Enumerate interfaces
		for(int i = 0; i < ifList->size(); i++)
      {
			InterfaceInfo *iface = ifList->get(i);

         TCHAR suffix[64], oid[128], buffer[256];
         SNMPConvertOIDToText(iface->ifTableSuffixLength, iface->ifTableSuffix, suffix, 64);

			// Get interface description
	      _sntprintf(oid, 128, _T(".1.3.6.1.2.1.2.2.1.2%s"), suffix);
	      if (SnmpGet(snmp->getSnmpVersion(), snmp, oid, NULL, 0, iface->description, MAX_DB_STRING * sizeof(TCHAR), 0) != SNMP_ERR_SUCCESS)
	         break;
	      nx_strncpy(iface->name, iface->description, MAX_DB_STRING);

         DbgPrintf(6, _T("TelcoBridgesDriver::getInterfaces(%p): processing interface %s (%s)"), snmp, iface->name, suffix);

         // Interface type
         _sntprintf(oid, 128, _T(".1.3.6.1.2.1.2.2.1.3%s"), suffix);
         if (SnmpGet(snmp->getSnmpVersion(), snmp, oid, NULL, 0, &iface->type, sizeof(UINT32), 0) != SNMP_ERR_SUCCESS)
			{
				iface->type = IFTYPE_OTHER;
			}

         // Interface MTU
         _sntprintf(oid, 128, _T(".1.3.6.1.2.1.2.2.1.4%s"), suffix);
         if (SnmpGet(snmp->getSnmpVersion(), snmp, oid, NULL, 0, &iface->mtu, sizeof(UINT32), 0) != SNMP_ERR_SUCCESS)
			{
				iface->mtu = 0;
			}

         // Interface speed
         _sntprintf(oid, 128, _T(".1.3.6.1.2.1.2.2.1.5%s"), suffix);  // ifSpeed
         UINT32 speed;
         if (SnmpGet(snmp->getSnmpVersion(), snmp, oid, NULL, 0, &speed, sizeof(UINT32), 0) == SNMP_ERR_SUCCESS)
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
         if (SnmpGet(snmp->getSnmpVersion(), snmp, oid, NULL, 0, buffer, 256, SG_RAW_RESULT) == SNMP_ERR_SUCCESS)
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
		UINT32 error = SnmpWalk(snmp, _T(".1.3.6.1.2.1.4.20.1.1"), HandlerIpAddr, ifList);
      if (error == SNMP_ERR_SUCCESS)
      {
         success = true;
      }
		else
		{
			DbgPrintf(6, _T("TelcoBridgesDriver::getInterfaces(%p): SNMP WALK .1.3.6.1.2.1.4.20.1.1 failed (%s)"), snmp, SNMPGetErrorText(error));
		}
   }
	else
	{
		DbgPrintf(6, _T("TelcoBridgesDriver::getInterfaces(%p): SNMP WALK .1.3.6.1.2.1.2.2.1.1 failed"), snmp);
	}

   if (!success)
   {
      delete_and_null(ifList);
   }

	DbgPrintf(6, _T("TelcoBridgesDriver::getInterfaces(%p): completed, ifList=%p"), snmp, ifList);
   return ifList;
}

/**
 * Driver entry point
 */
DECLARE_NDD_ENTRY_POINT(s_driverName, TelcoBridgesDriver);

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
