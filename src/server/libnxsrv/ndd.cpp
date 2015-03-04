/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2014 Victor Kirhenshtein
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
** File: ndd.cpp
**/

#include "libnxsrv.h"
#include <nddrv.h>

/**
 * Access point info constructor
 */
AccessPointInfo::AccessPointInfo(BYTE *macAddr, UINT32 ipAddr, AccessPointState state, const TCHAR *name, const TCHAR *vendor, const TCHAR *model, const TCHAR *serial)
{
	memcpy(m_macAddr, macAddr, MAC_ADDR_LENGTH);
   m_ipAddr = ipAddr;
	m_state = state;
	m_name = (name != NULL) ? _tcsdup(name) : NULL;
	m_vendor = (vendor != NULL) ? _tcsdup(vendor) : NULL;
	m_model = (model != NULL) ? _tcsdup(model) : NULL;
	m_serial = (serial != NULL) ? _tcsdup(serial) : NULL;
	m_radioInterfaces = new ObjectArray<RadioInterfaceInfo>(4, 4, true);
}

/**
 * Access point info destructor
 */
AccessPointInfo::~AccessPointInfo()
{
   safe_free(m_name);
   safe_free(m_vendor);
	safe_free(m_model);
	safe_free(m_serial);
	delete m_radioInterfaces;
}

/**
 * Add radio interface
 */
void AccessPointInfo::addRadioInterface(RadioInterfaceInfo *iface)
{
	RadioInterfaceInfo *r = new RadioInterfaceInfo;
	memcpy(r, iface, sizeof(RadioInterfaceInfo));
	m_radioInterfaces->add(r);
}

/**
 * Driver data constructor
 */
DriverData::DriverData()
{
}

/**
 * Driver data destructor
 */
DriverData::~DriverData()
{
}

/**
 * Constructor
 */
NetworkDeviceDriver::NetworkDeviceDriver()
{
}

/**
 * Destructor
 */
NetworkDeviceDriver::~NetworkDeviceDriver()
{
}

/**
 * Get driver name
 */
const TCHAR *NetworkDeviceDriver::getName()
{
	return _T("GENERIC");
}

/**
 * Get driver version
 *
 * @return driver version
 */
const TCHAR *NetworkDeviceDriver::getVersion()
{
	return NETXMS_VERSION_STRING;
}

/**
 * Get custom OID that should be used to test SNMP connectivity. Default
 * implementation always returns NULL.
 *
 * @return OID that should be used to test SNMP connectivity or NULL.
 */
const TCHAR *NetworkDeviceDriver::getCustomTestOID()
{
	return NULL;
}

/**
 * Check if given device can be potentially supported by driver
 *
 * @param oid Device OID
 */
int NetworkDeviceDriver::isPotentialDevice(const TCHAR *oid)
{
	return 1;
}

/**
 * Check if given device is supported by driver
 *
 * @param snmp SNMP transport
 * @param oid Device OID
 */
bool NetworkDeviceDriver::isDeviceSupported(SNMP_Transport *snmp, const TCHAR *oid)
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
 * @param attributes Node's custom attributes
 * @param driverData pointer to pointer to driver-specific data
 */
void NetworkDeviceDriver::analyzeDevice(SNMP_Transport *snmp, const TCHAR *oid, StringMap *attributes, DriverData **driverData)
{
}

/**
 * Handler for enumerating indexes
 */
static UINT32 HandlerIndex(UINT32 dwVersion, SNMP_Variable *pVar, SNMP_Transport *pTransport, void *pArg)
{
	((InterfaceList *)pArg)->add(new InterfaceInfo(pVar->getValueAsUInt()));
   return SNMP_ERR_SUCCESS;
}

/**
 * Handler for enumerating indexes via ifXTable
 */
static UINT32 HandlerIndexIfXTable(UINT32 dwVersion, SNMP_Variable *pVar, SNMP_Transport *pTransport, void *pArg)
{
   SNMP_ObjectId *name = pVar->getName();
   UINT32 index = name->getValue()[name->getLength() - 1];
   if (((InterfaceList *)pArg)->findByIfIndex(index) == NULL)
   {
	   ((InterfaceList *)pArg)->add(new InterfaceInfo(index));
   }
   return SNMP_ERR_SUCCESS;
}

/**
 * Handler for enumerating IP addresses via ipAddrTable
 */
static UINT32 HandlerIpAddr(UINT32 dwVersion, SNMP_Variable *pVar, SNMP_Transport *pTransport, void *pArg)
{
   UINT32 index, dwNetMask, dwResult;
   UINT32 oidName[MAX_OID_LEN];

   size_t nameLen = pVar->getName()->getLength();
   memcpy(oidName, pVar->getName()->getValue(), nameLen * sizeof(UINT32));
   oidName[nameLen - 5] = 3;  // Retrieve network mask for this IP
   dwResult = SnmpGet(dwVersion, pTransport, NULL, oidName, nameLen, &dwNetMask, sizeof(UINT32), 0);
   if (dwResult != SNMP_ERR_SUCCESS)
	{
		TCHAR buffer[1024];
		DbgPrintf(6, _T("NetworkDeviceDriver::getInterfaces(%p): SNMP GET %s failed (error %d)"), 
			pTransport, SNMPConvertOIDToText(nameLen, oidName, buffer, 1024), (int)dwResult);
		// Continue walk even if we get error for some IP address
		// For example, some Cisco ASA versions reports IP when walking, but does not
		// allow to SNMP GET appropriate entry
      return SNMP_ERR_SUCCESS;
	}

   oidName[nameLen - 5] = 2;  // Retrieve interface index for this IP
   dwResult = SnmpGet(dwVersion, pTransport, NULL, oidName, nameLen, &index, sizeof(UINT32), 0);
   if (dwResult == SNMP_ERR_SUCCESS)
   {
		InterfaceList *ifList = (InterfaceList *)pArg;
      InterfaceInfo *iface = ifList->findByIfIndex(index);
      if (iface != NULL)
      {
         iface->ipAddrList.add(InetAddress(ntohl(pVar->getValueAsUInt()), BitsInMask(dwNetMask)));
		}
   }
	else
	{
		TCHAR buffer[1024];
		DbgPrintf(6, _T("NetworkDeviceDriver::getInterfaces(%p): SNMP GET %s failed (error %d)"), 
			pTransport, SNMPConvertOIDToText(nameLen, oidName, buffer, 1024), (int)dwResult);
		dwResult = SNMP_ERR_SUCCESS;	// continue walk
	}
   return dwResult;
}

/**
 * Handler for enumerating IP addresses via ipAddressTable
 */
static UINT32 HandlerIpAddressTable(UINT32 version, SNMP_Variable *var, SNMP_Transport *snmp, void *arg)
{
   InterfaceList *ifList = (InterfaceList *)arg;

   UINT32 oid[128];
   size_t oidLen = var->getName()->getLength();
   memcpy(oid, var->getName()->getValue(), oidLen * sizeof(UINT32));

   // Check address family (1 = ipv4, 2 = ipv6)
   if ((oid[10] != 1) && (oid[10] != 2))
      return SNMP_ERR_SUCCESS;

   UINT32 ifIndex = var->getValueAsUInt();
   InterfaceInfo *iface = ifList->findByIfIndex(ifIndex);
   if (iface == NULL)
      return SNMP_ERR_SUCCESS;

   // Build IP address from OID
   InetAddress addr;
   if (oid[10] == 1)
   {
      addr = InetAddress((UINT32)((oid[12] << 24) | (oid[13] << 16) | (oid[14] << 8) | oid[15]));
   }
   else
   {
      BYTE bytes[16];
      for(int i = 12, j = 0; j < 16; i++, j++)
         bytes[j] = (BYTE)oid[i];
      addr = InetAddress(bytes);
   }
   if (iface->hasAddress(addr))
      return SNMP_ERR_SUCCESS;   // This IP already set from ipAddrTable

   // Get address type and prefix
   SNMP_PDU request(SNMP_GET_REQUEST, SnmpNewRequestId(), snmp->getSnmpVersion());
   oid[9] = 4; // ipAddressType
   request.bindVariable(new SNMP_Variable(oid, oidLen));
   oid[9] = 5; // ipAddressPrefix
   request.bindVariable(new SNMP_Variable(oid, oidLen));
   SNMP_PDU *response;
   if (snmp->doRequest(&request, &response, g_snmpTimeout, 3) == SNMP_ERR_SUCCESS)
   {
      // check number of varbinds and address type (1 = unicast)
      if ((response->getNumVariables() == 2) && (response->getVariable(0)->getValueAsInt() == 1))
      {
         SNMP_ObjectId *prefix = response->getVariable(1)->getValueAsObjectId();
         if ((prefix != NULL) && !prefix->isZeroDotZero())
         {
            // Last element in ipAddressPrefixTable index is prefix length
            addr.setMaskBits((int)prefix->getValue()[prefix->getLength() - 1]);
         }
         else
         {
            ifList->setPrefixWalkNeeded();
         }
         delete prefix;
         iface->ipAddrList.add(addr);
      }
      delete response;
   }
   return SNMP_ERR_SUCCESS;
}

/**
 * Handler for enumerating IP address prefixes via ipAddressPrefixTable
 */
static UINT32 HandlerIpAddressPrefixTable(UINT32 version, SNMP_Variable *var, SNMP_Transport *snmp, void *arg)
{
   InterfaceList *ifList = (InterfaceList *)arg;
   const UINT32 *oid = var->getName()->getValue();
   
   // Check address family (1 = ipv4, 2 = ipv6)
   if ((oid[10] != 1) && (oid[10] != 2))
      return SNMP_ERR_SUCCESS;

   // Build IP address from OID
   InetAddress prefix;
   if (oid[10] == 1)
   {
      prefix = InetAddress((UINT32)((oid[13] << 24) | (oid[14] << 16) | (oid[15] << 8) | oid[16]));
      prefix.setMaskBits((int)oid[17]);
   }
   else
   {
      BYTE bytes[16];
      for(int i = 13, j = 0; j < 16; i++, j++)
         bytes[j] = (BYTE)oid[i];
      prefix = InetAddress(bytes);
      prefix.setMaskBits((int)oid[29]);
   }

   // Find matching IP and set mask
   InterfaceInfo *iface = ifList->findByIfIndex(oid[12]);
   if (iface != NULL)
   {
      for(int i = 0; i < iface->ipAddrList.size(); i++)
      {
         InetAddress *addr = iface->ipAddrList.getList()->get(i);
         if ((addr != NULL) && prefix.contain(*addr))
         {
            addr->setMaskBits(prefix.getMaskBits());
         }
      }
   }
   return SNMP_ERR_SUCCESS;
}

/**
 * Get list of interfaces for given node
 *
 * @param snmp SNMP transport
 * @param attributes Node's custom attributes
 * @param useAliases policy for interface alias usage
 * @param useIfXTable if true, usage of ifXTable is allowed
 */
InterfaceList *NetworkDeviceDriver::getInterfaces(SNMP_Transport *snmp, StringMap *attributes, DriverData *driverData, int useAliases, bool useIfXTable)
{
   LONG i, iNumIf;
   TCHAR szOid[128], szBuffer[256];
   InterfaceList *pIfList = NULL;
   BOOL bSuccess = FALSE;

	DbgPrintf(6, _T("NetworkDeviceDriver::getInterfaces(%p,%d,%s)"), snmp, useAliases, useIfXTable ? _T("true") : _T("false"));

   // Get number of interfaces
	UINT32 error = SnmpGet(snmp->getSnmpVersion(), snmp, _T(".1.3.6.1.2.1.2.1.0"), NULL, 0, &iNumIf, sizeof(LONG), 0);
	if (error != SNMP_ERR_SUCCESS)
	{
		DbgPrintf(6, _T("NetworkDeviceDriver::getInterfaces(%p): SNMP GET .1.3.6.1.2.1.2.1.0 failed (%s)"), snmp, SNMPGetErrorText(error));
      return NULL;
	}

	// Some devices may return completely insane values here
	// (for example, DLink DGS-3612 can return negative value)
	// Anyway it's just a hint for initial array size
	if ((iNumIf <= 0) || (iNumIf > 4096))
		iNumIf = 64;
      
   // Create empty list
   pIfList = new InterfaceList(iNumIf);

   // Gather interface indexes
   if (SnmpWalk(snmp->getSnmpVersion(), snmp, _T(".1.3.6.1.2.1.2.2.1.1"), HandlerIndex, pIfList, FALSE) == SNMP_ERR_SUCCESS)
   {
      // Gather additional interfaces from ifXTable
      SnmpWalk(snmp->getSnmpVersion(), snmp, _T(".1.3.6.1.2.1.31.1.1.1.1"), HandlerIndexIfXTable, pIfList, FALSE);

      // Enumerate interfaces
		for(i = 0; i < pIfList->size(); i++)
      {
			InterfaceInfo *iface = pIfList->get(i);

			// Get interface description
	      _sntprintf(szOid, 128, _T(".1.3.6.1.2.1.2.2.1.2.%d"), iface->index);
	      if (SnmpGet(snmp->getSnmpVersion(), snmp, szOid, NULL, 0, iface->description, MAX_DB_STRING * sizeof(TCHAR), 0) != SNMP_ERR_SUCCESS)
         {
            // Try to get name from ifXTable
	         _sntprintf(szOid, 128, _T(".1.3.6.1.2.1.31.1.1.1.1.%d"), iface->index);
	         if (SnmpGet(snmp->getSnmpVersion(), snmp, szOid, NULL, 0, iface->description, MAX_DB_STRING * sizeof(TCHAR), 0) != SNMP_ERR_SUCCESS)
   	         break;
         }

         // Get interface alias
	      _sntprintf(szOid, 128, _T(".1.3.6.1.2.1.31.1.1.1.18.%d"), iface->index);
			if (SnmpGet(snmp->getSnmpVersion(), snmp, szOid, NULL, 0,
	                  iface->alias, MAX_DB_STRING * sizeof(TCHAR), 0) == SNMP_ERR_SUCCESS)
         {
            StrStrip(iface->alias);
         }
         else
         {
            iface->alias[0] = 0;
         }

         // Set name to ifAlias if needed
         if (useAliases > 0)
         {
            _tcscpy(iface->name, iface->alias);
         }

			// Try to get interface name from ifXTable, if unsuccessful or disabled, use ifDescr from ifTable
         _sntprintf(szOid, 128, _T(".1.3.6.1.2.1.31.1.1.1.1.%d"), iface->index);
         if (!useIfXTable ||
				 (SnmpGet(snmp->getSnmpVersion(), snmp, szOid, NULL, 0, szBuffer, sizeof(szBuffer), 0) != SNMP_ERR_SUCCESS))
         {
		      nx_strncpy(szBuffer, iface->description, 256);
		   }

			// Build full interface object name
         switch(useAliases)
         {
         	case 1:	// Use only alias if available, otherwise name
         		if (iface->name[0] == 0)
	         		nx_strncpy(iface->name, szBuffer, MAX_DB_STRING);	// Alias is empty or not available
         		break;
         	case 2:	// Concatenate alias with name
         	case 3:	// Concatenate name with alias
         		if (iface->name[0] != 0)
         		{
						if (useAliases == 2)
						{
         				if  (_tcslen(iface->name) < (MAX_DB_STRING - 3))
         				{
		      				_sntprintf(&iface->name[_tcslen(iface->name)], MAX_DB_STRING - _tcslen(iface->name), _T(" (%s)"), szBuffer);
		      				iface->name[MAX_DB_STRING - 1] = 0;
		      			}
						}
						else
						{
							TCHAR temp[MAX_DB_STRING];

							_tcscpy(temp, iface->name);
		         		nx_strncpy(iface->name, szBuffer, MAX_DB_STRING);
         				if  (_tcslen(iface->name) < (MAX_DB_STRING - 3))
         				{
		      				_sntprintf(&iface->name[_tcslen(iface->name)], MAX_DB_STRING - _tcslen(iface->name), _T(" (%s)"), temp);
		      				iface->name[MAX_DB_STRING - 1] = 0;
		      			}
						}
         		}
         		else
         		{
	         		nx_strncpy(iface->name, szBuffer, MAX_DB_STRING);	// Alias is empty or not available
					}
         		break;
         	default:	// Use only name
         		nx_strncpy(iface->name, szBuffer, MAX_DB_STRING);
         		break;
         }

         // Interface type
         _sntprintf(szOid, 128, _T(".1.3.6.1.2.1.2.2.1.3.%d"), iface->index);
         if (SnmpGet(snmp->getSnmpVersion(), snmp, szOid, NULL, 0, &iface->type, sizeof(UINT32), 0) != SNMP_ERR_SUCCESS)
			{
				iface->type = IFTYPE_OTHER;
			}

         // Interface MTU
         _sntprintf(szOid, 128, _T(".1.3.6.1.2.1.2.2.1.4.%d"), iface->index);
         if (SnmpGet(snmp->getSnmpVersion(), snmp, szOid, NULL, 0, &iface->mtu, sizeof(UINT32), 0) != SNMP_ERR_SUCCESS)
			{
				iface->type = IFTYPE_OTHER;
			}

         // MAC address
         _sntprintf(szOid, 128, _T(".1.3.6.1.2.1.2.2.1.6.%d"), iface->index);
         memset(szBuffer, 0, MAC_ADDR_LENGTH);
         if (SnmpGet(snmp->getSnmpVersion(), snmp, szOid, NULL, 0, szBuffer, 256, SG_RAW_RESULT) == SNMP_ERR_SUCCESS)
			{
	         memcpy(iface->macAddr, szBuffer, MAC_ADDR_LENGTH);
			}
			else
			{
				// Unable to get MAC address
	         memset(iface->macAddr, 0, MAC_ADDR_LENGTH);
			}
      }

      // Interface IP address'es and netmasks
		error = SnmpWalk(snmp->getSnmpVersion(), snmp, _T(".1.3.6.1.2.1.4.20.1.1"), HandlerIpAddr, pIfList, FALSE);
      if (error == SNMP_ERR_SUCCESS)
      {
         bSuccess = TRUE;
      }
		else
		{
			DbgPrintf(6, _T("NetworkDeviceDriver::getInterfaces(%p): SNMP WALK .1.3.6.1.2.1.4.20.1.1 failed (%s)"), snmp, SNMPGetErrorText(error));
		}

      // Get IP addresses from ipAddressTable if available
		SnmpWalk(snmp->getSnmpVersion(), snmp, _T(".1.3.6.1.2.1.4.34.1.3"), HandlerIpAddressTable, pIfList, FALSE);
      if (pIfList->isPrefixWalkNeeded())
      {
   		SnmpWalk(snmp->getSnmpVersion(), snmp, _T(".1.3.6.1.2.1.4.32.1.5"), HandlerIpAddressPrefixTable, pIfList, FALSE);
      }
   }
	else
	{
		DbgPrintf(6, _T("NetworkDeviceDriver::getInterfaces(%p): SNMP WALK .1.3.6.1.2.1.2.2.1.1 failed"), snmp);
	}

   if (!bSuccess)
   {
      delete_and_null(pIfList);
   }

	DbgPrintf(6, _T("NetworkDeviceDriver::getInterfaces(%p): completed, ifList=%p"), snmp, pIfList);
   return pIfList;
}

/**
 * Get interface status. Both states must be set to UNKNOWN if cannot be read from device.
 * 
 * @param snmp SNMP transport
 * @param attributes node's custom attributes
 * @param driverData driver's data
 * @param ifIndex interface index
 * @param adminState OUT: interface administrative state
 * @param operState OUT: interface operational state
 */
void NetworkDeviceDriver::getInterfaceState(SNMP_Transport *snmp, StringMap *attributes, DriverData *driverData, UINT32 ifIndex, InterfaceAdminState *adminState, InterfaceOperState *operState)
{
   UINT32 state = 0;
   TCHAR oid[256];
   _sntprintf(oid, 256, _T(".1.3.6.1.2.1.2.2.1.7.%d"), (int)ifIndex); // Interface administrative state
   SnmpGet(snmp->getSnmpVersion(), snmp, oid, NULL, 0, &state, sizeof(UINT32), 0);

   switch(state)
   {
		case 2:
			*adminState = IF_ADMIN_STATE_DOWN;
			*operState = IF_OPER_STATE_DOWN;
         break;
      case 1:
		case 3:
			*adminState = (InterfaceAdminState)state;
         // Get interface operational state
         state = 0;
         _sntprintf(oid, 256, _T(".1.3.6.1.2.1.2.2.1.8.%d"), (int)ifIndex);
         SnmpGet(snmp->getSnmpVersion(), snmp, oid, NULL, 0, &state, sizeof(UINT32), 0);
         switch(state)
         {
            case 3:
					*operState = IF_OPER_STATE_TESTING;
               break;
            case 2:  // down: interface is down
				case 7:	// lowerLayerDown: down due to state of lower-layer interface(s)
					*operState = IF_OPER_STATE_DOWN;
               break;
            case 1:
					*operState = IF_OPER_STATE_UP;
               break;
            default:
					*operState = IF_OPER_STATE_UNKNOWN;
               break;
         }
         break;
      default:
			*adminState = IF_ADMIN_STATE_UNKNOWN;
			*operState = IF_OPER_STATE_UNKNOWN;
         break;
   }
}

/**
 * Handler for VLAN enumeration
 */
static UINT32 HandlerVlanList(UINT32 version, SNMP_Variable *var, SNMP_Transport *transport, void *arg)
{
   VlanList *vlanList = (VlanList *)arg;

	VlanInfo *vlan = new VlanInfo(var->getName()->getValue()[var->getName()->getLength() - 1], VLAN_PRM_BPORT);

	TCHAR buffer[256];
	vlan->setName(var->getValueAsString(buffer, 256));

	vlanList->add(vlan);
   return SNMP_ERR_SUCCESS;
}

/**
 * Parse VLAN membership bit map
 *
 * @param vlanList VLAN list
 * @param ifIndex interface index for current interface
 * @param map VLAN membership map
 * @param offset VLAN ID offset from 0
 */
static void ParseVlanPorts(VlanList *vlanList, VlanInfo *vlan, BYTE map, int offset)
{
	// VLAN egress port map description from Q-BRIDGE-MIB:
	// ===================================================
	// Each octet within this value specifies a set of eight
	// ports, with the first octet specifying ports 1 through
	// 8, the second octet specifying ports 9 through 16, etc.
	// Within each octet, the most significant bit represents
	// the lowest numbered port, and the least significant bit
	// represents the highest numbered port.  Thus, each port
	// of the bridge is represented by a single bit within the
	// value of this object.  If that bit has a value of '1'
	// then that port is included in the set of ports; the port
	// is not included if its bit has a value of '0'.

	int port = offset;
	BYTE mask = 0x80;
	while(mask > 0)
	{
		if (map & mask)
		{
			vlan->add((UINT32)port);
		}
		mask >>= 1;
		port++;
	}
}

/**
 * Handler for VLAN egress port enumeration
 */
static UINT32 HandlerVlanEgressPorts(UINT32 version, SNMP_Variable *var, SNMP_Transport *transport, void *arg)
{
   VlanList *vlanList = (VlanList *)arg;
	UINT32 vlanId = var->getName()->getValue()[var->getName()->getLength() - 1];
	VlanInfo *vlan = vlanList->findById(vlanId);
	if (vlan != NULL)
	{
		BYTE buffer[4096];
		size_t size = var->getRawValue(buffer, 4096);
		for(int i = 0; i < (int)size; i++)
		{
			ParseVlanPorts(vlanList, vlan, buffer[i], i * 8 + 1);
		}
	}
   vlanList->setData(vlanList);  // to indicate that callback was called
	return SNMP_ERR_SUCCESS;
}

/**
 * Get list of VLANs on given node
 *
 * @param snmp SNMP transport
 * @param attributes Node's custom attributes
 * @param driverData driver-specific data previously created in analyzeDevice
 * @return VLAN list or NULL
 */
VlanList *NetworkDeviceDriver::getVlans(SNMP_Transport *snmp, StringMap *attributes, DriverData *driverData)
{
	VlanList *list = new VlanList();
	
   // dot1qVlanStaticName
	if (SnmpWalk(snmp->getSnmpVersion(), snmp, _T(".1.3.6.1.2.1.17.7.1.4.3.1.1"), HandlerVlanList, list, FALSE) != SNMP_ERR_SUCCESS)
		goto failure;

   // dot1qVlanCurrentEgressPorts
	if (SnmpWalk(snmp->getSnmpVersion(), snmp, _T(".1.3.6.1.2.1.17.7.1.4.2.1.4"), HandlerVlanEgressPorts, list, FALSE) != SNMP_ERR_SUCCESS)
		goto failure;

   if (list->getData() == NULL)
   {
      // Some devices does not return anything under dot1qVlanCurrentEgressPorts.
      // In that case we use dot1qVlanStaticEgressPorts
	   if (SnmpWalk(snmp->getSnmpVersion(), snmp, _T(".1.3.6.1.2.1.17.7.1.4.3.1.2"), HandlerVlanEgressPorts, list, FALSE) != SNMP_ERR_SUCCESS)
		   goto failure;
   }

	return list;

failure:
	delete list;
	return NULL;
}

/** 
 * Get orientation of the modules in the device. Default implementation always returns NDD_ORIENTATION_HORIZONTAL.
 *
 * @param snmp SNMP transport
 * @param attributes Node's custom attributes
 * @param driverData driver-specific data previously created in analyzeDevice
 * @return module orientation
 */
int NetworkDeviceDriver::getModulesOrientation(SNMP_Transport *snmp, StringMap *attributes, DriverData *driverData)
{
	return NDD_ORIENTATION_HORIZONTAL;
}

/** 
 * Get port layout of given module. Default implementation always set NDD_PN_UNKNOWN as layout.
 *
 * @param snmp SNMP transport
 * @param attributes Node's custom attributes
 * @param driverData driver-specific data previously created in analyzeDevice
 * @param module Module number (starting from 1)
 * @param layout Layout structure to fill
 */
void NetworkDeviceDriver::getModuleLayout(SNMP_Transport *snmp, StringMap *attributes, DriverData *driverData, int module, NDD_MODULE_LAYOUT *layout)
{
	layout->numberingScheme = NDD_PN_UNKNOWN;
	layout->rows = 2;
}

/**
 * Returns true if per-VLAN FDB supported by device (accessible using community@vlan_id).
 * Default implementation always return false;
 *
 * @return true if per-VLAN FDB supported by device
 */
bool NetworkDeviceDriver::isPerVlanFdbSupported()
{
	return false;
}

/**
 * Get device cluster mode. Default implementation always return CLUSTER_MODE_STANDALONE.
 *
 * @param snmp SNMP transport
 * @param attributes Node's custom attributes
 * @param driverData driver-specific data previously created in analyzeDevice
 * @return cluster mode (one of CLUSTER_MODE_STANDALONE, CLUSTER_MODE_ACTIVE, CLUSTER_MODE_STANDBY)
 */
int NetworkDeviceDriver::getClusterMode(SNMP_Transport *snmp, StringMap *attributes, DriverData *driverData)
{
   return CLUSTER_MODE_STANDALONE;
}

/**
 * Returns true if device is a wireless controller. Default implementation always return false.
 *
 * @param snmp SNMP transport
 * @param attributes Node's custom attributes
 * @param driverData driver-specific data previously created in analyzeDevice
 * @return true if device is a wireless controller
 */
bool NetworkDeviceDriver::isWirelessController(SNMP_Transport *snmp, StringMap *attributes, DriverData *driverData)
{
   return false;
}

/**
 * Get list of wireless access points managed by this controller. Default implementation always return NULL.
 *
 * @param snmp SNMP transport
 * @param attributes Node's custom attributes
 * @param driverData driver-specific data previously created in analyzeDevice
 * @return list of access points
 */
ObjectArray<AccessPointInfo> *NetworkDeviceDriver::getAccessPoints(SNMP_Transport *snmp, StringMap *attributes, DriverData *driverData)
{
   return NULL;
}

/**
 * Get list of associated wireless stations. Default implementation always return NULL.
 *
 * @param snmp SNMP transport
 * @param attributes Node's custom attributes
 * @param driverData driver-specific data previously created in analyzeDevice
 * @return list of associated wireless stations
 */
ObjectArray<WirelessStationInfo> *NetworkDeviceDriver::getWirelessStations(SNMP_Transport *snmp, StringMap *attributes, DriverData *driverData)
{
   return NULL;
}
