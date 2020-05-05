/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2020 Victor Kirhenshtein
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

#define DEBUG_TAG _T("ndd.common")

/**
 * Serialize radio interface information to JSON
 */
json_t *RadioInterfaceInfo::toJson() const
{
   json_t *root = json_object();
   json_object_set_new(root, "index", json_integer(index));
   json_object_set_new(root, "name", json_string_t(name));
   char macAddrText[64];
   json_object_set_new(root, "macAddr", json_string(BinToStrA(macAddr, MAC_ADDR_LENGTH, macAddrText)));
   json_object_set_new(root, "channel", json_integer(channel));
   json_object_set_new(root, "powerDBm", json_integer(powerDBm));
   json_object_set_new(root, "powerMW", json_integer(powerMW));
   return root;
}

/**
 * Access point info constructor
 */
AccessPointInfo::AccessPointInfo(uint32_t index, const MacAddress& macAddr, const InetAddress& ipAddr, AccessPointState state,
         const TCHAR *name, const TCHAR *vendor, const TCHAR *model, const TCHAR *serial) : m_macAddr(macAddr), m_ipAddr(ipAddr)
{
   m_index = index;
	m_state = state;
	m_name = MemCopyString(name);
	m_vendor = MemCopyString(vendor);
	m_model = MemCopyString(model);
	m_serial = MemCopyString(serial);
	m_radioInterfaces = new ObjectArray<RadioInterfaceInfo>(4, 4, Ownership::True);
}

/**
 * Access point info destructor
 */
AccessPointInfo::~AccessPointInfo()
{
   MemFree(m_name);
   MemFree(m_vendor);
   MemFree(m_model);
   MemFree(m_serial);
	delete m_radioInterfaces;
}

/**
 * Add radio interface
 */
void AccessPointInfo::addRadioInterface(const RadioInterfaceInfo& iface)
{
	RadioInterfaceInfo *r = new RadioInterfaceInfo;
	memcpy(r, &iface, sizeof(RadioInterfaceInfo));
	m_radioInterfaces->add(r);
}

/**
 * Driver data constructor
 */
DriverData::DriverData()
{
   m_nodeId = 0;
   m_nodeGuid = uuid::NULL_UUID;
   m_nodeName[0] = 0;
}

/**
 * Driver data destructor
 */
DriverData::~DriverData()
{
}

/**
 * Attach driver data to node
 */
void DriverData::attachToNode(UINT32 nodeId, const uuid& nodeGuid, const TCHAR *nodeName)
{
   m_nodeId = nodeId;
   m_nodeGuid = nodeGuid;
   _tcslcpy(m_nodeName, nodeName, MAX_OBJECT_NAME);
   nxlog_debug_tag(DEBUG_TAG, 5, _T("Driver data attached to node %s [%u]"), nodeName, nodeId);
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
 * @return true if device is supported
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
 * @param oid Device OID
 * @param node Node
 * @param driverData pointer to pointer to driver-specific data
 */
void NetworkDeviceDriver::analyzeDevice(SNMP_Transport *snmp, const TCHAR *oid, NObject *node, DriverData **driverData)
{
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
bool NetworkDeviceDriver::getHardwareInformation(SNMP_Transport *snmp, NObject *node, DriverData *driverData, DeviceHardwareInfo *hwInfo)
{
   return false;
}

/**
 * Get device virtualization type.
 *
 * @param snmp SNMP transport
 * @param node Node
 * @param driverData driver data
 * @param vtype pointer to virtualization type enum to fill
 * @return true if virtualization type is known
 */
bool NetworkDeviceDriver::getVirtualizationType(SNMP_Transport *snmp, NObject *node, DriverData *driverData, VirtualizationType *vtype)
{
   return false;
}

/**
 * Handler for enumerating indexes
 */
static UINT32 HandlerIndex(SNMP_Variable *pVar, SNMP_Transport *pTransport, void *pArg)
{
	((InterfaceList *)pArg)->add(new InterfaceInfo(pVar->getValueAsUInt()));
   return SNMP_ERR_SUCCESS;
}

/**
 * Handler for enumerating indexes via ifXTable
 */
static UINT32 HandlerIndexIfXTable(SNMP_Variable *pVar, SNMP_Transport *pTransport, void *pArg)
{
   const SNMP_ObjectId& name = pVar->getName();
   UINT32 index = name.value()[name.length() - 1];
   if (((InterfaceList *)pArg)->findByIfIndex(index) == NULL)
   {
	   ((InterfaceList *)pArg)->add(new InterfaceInfo(index));
   }
   return SNMP_ERR_SUCCESS;
}

/**
 * Handler for enumerating IP addresses via ipAddrTable
 */
static UINT32 HandlerIpAddr(SNMP_Variable *var, SNMP_Transport *snmp, void *context)
{
   // Get address type and prefix
   SNMP_ObjectId oid = var->getName();
   SNMP_PDU request(SNMP_GET_REQUEST, SnmpNewRequestId(), snmp->getSnmpVersion());
   oid.changeElement(9, 3); // ipAdEntNetMask
   request.bindVariable(new SNMP_Variable(oid));
   oid.changeElement(9, 2); // ipAdEntIfIndex
   request.bindVariable(new SNMP_Variable(oid));
   SNMP_PDU *response;
   uint32_t rc = snmp->doRequest(&request, &response, SnmpGetDefaultTimeout(), 3);
   if (rc == SNMP_ERR_SUCCESS)
   {
      // check number of varbinds and address type (1 = unicast)
      if ((response->getErrorCode() == SNMP_PDU_ERR_SUCCESS) && (response->getNumVariables() == 2))
      {
         InterfaceList *ifList = static_cast<InterfaceList*>(context);
         InterfaceInfo *iface = ifList->findByIfIndex(response->getVariable(1)->getValueAsUInt());
         if (iface != nullptr)
         {
            iface->ipAddrList.add(InetAddress(ntohl(var->getValueAsUInt()), ntohl(response->getVariable(0)->getValueAsUInt())));
         }
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG, 6, _T("NetworkDeviceDriver::getInterfaces(%p): SNMP GET in HandlerIpAddr failed (%s)"), snmp, SNMPGetProtocolErrorText(response->getErrorCode()));
      }
      delete response;
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 6, _T("NetworkDeviceDriver::getInterfaces(%p): SNMP GET in HandlerIpAddr failed (%s)"), snmp, SNMPGetErrorText(rc));
   }
   return rc;
}

/**
 * Handler for enumerating IP addresses via ipAddressTable
 */
static UINT32 HandlerIpAddressTable(SNMP_Variable *var, SNMP_Transport *snmp, void *arg)
{
   InterfaceList *ifList = (InterfaceList *)arg;

   uint32_t oid[128];
   size_t oidLen = var->getName().length();
   memcpy(oid, var->getName().value(), oidLen * sizeof(uint32_t));

   // Check address family (1 = ipv4, 2 = ipv6)
   if ((oid[10] != 1) && (oid[10] != 2))
      return SNMP_ERR_SUCCESS;

   uint32_t ifIndex = var->getValueAsUInt();
   InterfaceInfo *iface = ifList->findByIfIndex(ifIndex);
   if (iface == nullptr)
      return SNMP_ERR_SUCCESS;

   // Build IP address from OID
   InetAddress addr;
   if (((oid[10] == 1) && (oid[11] == 4)) || ((oid[10] == 3) && (oid[11] == 8))) // ipv4 and ipv4z
   {
      addr = InetAddress((uint32_t)((oid[12] << 24) | (oid[13] << 16) | (oid[14] << 8) | oid[15]));
   }
   else if (((oid[10] == 2) && (oid[11] == 16)) || ((oid[10] == 4) && (oid[11] == 20))) // ipv6 and ipv6z
   {
      BYTE bytes[16];
      for(int i = 12, j = 0; j < 16; i++, j++)
         bytes[j] = (BYTE)oid[i];
      addr = InetAddress(bytes);
   }
   else
   {
      return SNMP_ERR_SUCCESS;   // Unknown or unsupported address format
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
   if (snmp->doRequest(&request, &response, SnmpGetDefaultTimeout(), 3) == SNMP_ERR_SUCCESS)
   {
      // check number of varbinds and address type (1 = unicast)
      if ((response->getNumVariables() == 2) && (response->getVariable(0)->getValueAsInt() == 1))
      {
         SNMP_ObjectId prefix = response->getVariable(1)->getValueAsObjectId();
         if (prefix.isValid() && !prefix.isZeroDotZero())
         {
            // Last element in ipAddressPrefixTable index is prefix length
            addr.setMaskBits((int)prefix.value()[prefix.length() - 1]);
         }
         else
         {
            ifList->setPrefixWalkNeeded();
         }
         iface->ipAddrList.add(addr);
      }
      delete response;
   }
   return SNMP_ERR_SUCCESS;
}

/**
 * Handler for enumerating IP address prefixes via ipAddressPrefixTable
 */
static UINT32 HandlerIpAddressPrefixTable(SNMP_Variable *var, SNMP_Transport *snmp, void *arg)
{
   InterfaceList *ifList = (InterfaceList *)arg;
   const UINT32 *oid = var->getName().value();
   
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
 * @param node Node
 * @param driverData driver data
 * @param useAliases policy for interface alias usage
 * @param useIfXTable if true, usage of ifXTable is allowed
 */
InterfaceList *NetworkDeviceDriver::getInterfaces(SNMP_Transport *snmp, NObject *node, DriverData *driverData, int useAliases, bool useIfXTable)
{
   bool success = false;

	nxlog_debug_tag(DEBUG_TAG, 6, _T("NetworkDeviceDriver::getInterfaces(%p,%d,%s)"), snmp, useAliases, useIfXTable ? _T("true") : _T("false"));

   // Get number of interfaces
	// Some devices may not return ifNumber at all or return completely insane values
   // (for example, DLink DGS-3612 can return negative value)
   // Anyway it's just a hint for initial array size
	UINT32 interfaceCount = 0;
	SnmpGet(snmp->getSnmpVersion(), snmp, _T(".1.3.6.1.2.1.2.1.0"), NULL, 0, &interfaceCount, sizeof(LONG), 0);
	if ((interfaceCount <= 0) || (interfaceCount > 4096))
	{
	   nxlog_debug_tag(DEBUG_TAG, 6, _T("NetworkDeviceDriver::getInterfaces(%p): invalid interface count %d received from device"), snmp, interfaceCount);
		interfaceCount = 64;
	}

   // Create empty list
	InterfaceList *ifList = new InterfaceList(interfaceCount);

   // Gather interface indexes
   if (SnmpWalk(snmp, _T(".1.3.6.1.2.1.2.2.1.1"), HandlerIndex, ifList) == SNMP_ERR_SUCCESS)
   {
      // Gather additional interfaces from ifXTable
      SnmpWalk(snmp, _T(".1.3.6.1.2.1.31.1.1.1.1"), HandlerIndexIfXTable, ifList);

      // Enumerate interfaces
		for(int i = 0; i < ifList->size(); i++)
      {
			InterfaceInfo *iface = ifList->get(i);

			// Get interface description
		   TCHAR oid[128];
	      _sntprintf(oid, 128, _T(".1.3.6.1.2.1.2.2.1.2.%d"), iface->index);
	      if (SnmpGet(snmp->getSnmpVersion(), snmp, oid, NULL, 0, iface->description, MAX_DB_STRING * sizeof(TCHAR), 0) != SNMP_ERR_SUCCESS)
         {
            // Try to get name from ifXTable
	         _sntprintf(oid, 128, _T(".1.3.6.1.2.1.31.1.1.1.1.%d"), iface->index);
	         if (SnmpGet(snmp->getSnmpVersion(), snmp, oid, NULL, 0, iface->description, MAX_DB_STRING * sizeof(TCHAR), 0) != SNMP_ERR_SUCCESS)
	         {
	            nxlog_debug_tag(DEBUG_TAG, 6, _T("NetworkDeviceDriver::getInterfaces(%p): cannot read interface description for interface %u"), snmp, iface->index);
   	         continue;
	         }
         }

         // Get interface alias
	      _sntprintf(oid, 128, _T(".1.3.6.1.2.1.31.1.1.1.18.%d"), iface->index);
			if (SnmpGet(snmp->getSnmpVersion(), snmp, oid, NULL, 0,
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
         TCHAR buffer[256];
         _sntprintf(oid, 128, _T(".1.3.6.1.2.1.31.1.1.1.1.%d"), iface->index);
         if (!useIfXTable ||
				 (SnmpGet(snmp->getSnmpVersion(), snmp, oid, NULL, 0, buffer, sizeof(buffer), 0) != SNMP_ERR_SUCCESS))
         {
		      _tcslcpy(buffer, iface->description, 256);
		   }

			// Build full interface object name
         switch(useAliases)
         {
         	case 1:	// Use only alias if available, otherwise name
         		if (iface->name[0] == 0)
         		   _tcslcpy(iface->name, buffer, MAX_DB_STRING);	// Alias is empty or not available
         		break;
         	case 2:	// Concatenate alias with name
         	case 3:	// Concatenate name with alias
         		if (iface->name[0] != 0)
         		{
						if (useAliases == 2)
						{
         				if  (_tcslen(iface->name) < (MAX_DB_STRING - 3))
         				{
		      				_sntprintf(&iface->name[_tcslen(iface->name)], MAX_DB_STRING - _tcslen(iface->name), _T(" (%s)"), buffer);
		      				iface->name[MAX_DB_STRING - 1] = 0;
		      			}
						}
						else
						{
							TCHAR temp[MAX_DB_STRING];

							_tcscpy(temp, iface->name);
							_tcslcpy(iface->name, buffer, MAX_DB_STRING);
         				if  (_tcslen(iface->name) < (MAX_DB_STRING - 3))
         				{
		      				_sntprintf(&iface->name[_tcslen(iface->name)], MAX_DB_STRING - _tcslen(iface->name), _T(" (%s)"), temp);
		      				iface->name[MAX_DB_STRING - 1] = 0;
		      			}
						}
         		}
         		else
         		{
         		   _tcslcpy(iface->name, buffer, MAX_DB_STRING);	// Alias is empty or not available
					}
         		break;
         	default:	// Use only name
         	   _tcslcpy(iface->name, buffer, MAX_DB_STRING);
         		break;
         }

         // Interface type
         _sntprintf(oid, 128, _T(".1.3.6.1.2.1.2.2.1.3.%d"), iface->index);
         if (SnmpGet(snmp->getSnmpVersion(), snmp, oid, NULL, 0, &iface->type, sizeof(UINT32), 0) != SNMP_ERR_SUCCESS)
			{
				iface->type = IFTYPE_OTHER;
			}

         // Interface MTU
         _sntprintf(oid, 128, _T(".1.3.6.1.2.1.2.2.1.4.%d"), iface->index);
         if (SnmpGet(snmp->getSnmpVersion(), snmp, oid, NULL, 0, &iface->mtu, sizeof(UINT32), 0) != SNMP_ERR_SUCCESS)
			{
				iface->mtu = 0;
			}

         // Interface speed
         _sntprintf(oid, 128, _T(".1.3.6.1.2.1.31.1.1.1.15.%d"), iface->index);  // try ifHighSpeed first
         UINT32 speed;
         if (SnmpGet(snmp->getSnmpVersion(), snmp, oid, NULL, 0, &speed, sizeof(UINT32), 0) != SNMP_ERR_SUCCESS)
			{
				speed = 0;
			}
         if (speed < 2000)  // ifHighSpeed not supported or slow interface
         {
            _sntprintf(oid, 128, _T(".1.3.6.1.2.1.2.2.1.5.%d"), iface->index);  // ifSpeed
            if (SnmpGet(snmp->getSnmpVersion(), snmp, oid, NULL, 0, &speed, sizeof(UINT32), 0) == SNMP_ERR_SUCCESS)
            {
               iface->speed = speed;
            }
            else
            {
               iface->speed = 0;
            }
         }
         else
         {
            iface->speed = static_cast<uint64_t>(speed) * _ULL(1000000);
         }

         // MAC address
         _sntprintf(oid, 128, _T(".1.3.6.1.2.1.2.2.1.6.%d"), iface->index);
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

      // Interface IP addresses and netmasks
		UINT32 error = SnmpWalk(snmp, _T(".1.3.6.1.2.1.4.20.1.1"), HandlerIpAddr, ifList);
      if (error == SNMP_ERR_SUCCESS)
      {
         success = true;
      }
		else
		{
		   nxlog_debug_tag(DEBUG_TAG, 6, _T("NetworkDeviceDriver::getInterfaces(%p): SNMP WALK .1.3.6.1.2.1.4.20.1.1 failed (%s)"), snmp, SNMPGetErrorText(error));
		}

      // Get IP addresses from ipAddressTable if available
		SnmpWalk(snmp, _T(".1.3.6.1.2.1.4.34.1.3"), HandlerIpAddressTable, ifList);
      if (ifList->isPrefixWalkNeeded())
      {
   		SnmpWalk(snmp, _T(".1.3.6.1.2.1.4.32.1.5"), HandlerIpAddressPrefixTable, ifList);
      }
   }
	else
	{
		nxlog_debug_tag(DEBUG_TAG, 6, _T("NetworkDeviceDriver::getInterfaces(%p): SNMP WALK .1.3.6.1.2.1.2.2.1.1 failed"), snmp);
	}

   if (!success)
   {
      delete_and_null(ifList);
   }

   nxlog_debug_tag(DEBUG_TAG, 6, _T("NetworkDeviceDriver::getInterfaces(%p): completed, ifList=%p"), snmp, ifList);
   return ifList;
}

/**
 * Get interface state. Both states must be set to UNKNOWN if cannot be read from device.
 * 
 * @param snmp SNMP transport
 * @param node Node
 * @param driverData driver's data
 * @param ifIndex interface index
 * @param ifTableSuffixLen length of interface table suffix
 * @param ifTableSuffix interface table suffix
 * @param adminState OUT: interface administrative state
 * @param operState OUT: interface operational state
 */
void NetworkDeviceDriver::getInterfaceState(SNMP_Transport *snmp, NObject *node, DriverData *driverData, UINT32 ifIndex,
                                            int ifTableSuffixLen, UINT32 *ifTableSuffix, InterfaceAdminState *adminState, InterfaceOperState *operState)
{
   UINT32 state = 0;
   TCHAR oid[256], suffix[128];
   if (ifTableSuffixLen > 0)
      _sntprintf(oid, 256, _T(".1.3.6.1.2.1.2.2.1.7%s"), SNMPConvertOIDToText(ifTableSuffixLen, ifTableSuffix, suffix, 128)); // Interface administrative state
   else
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
         if (ifTableSuffixLen > 0)
            _sntprintf(oid, 256, _T(".1.3.6.1.2.1.2.2.1.8%s"), SNMPConvertOIDToText(ifTableSuffixLen, ifTableSuffix, suffix, 128));
         else
            _sntprintf(oid, 256, _T(".1.3.6.1.2.1.2.2.1.8.%d"), (int)ifIndex);
         SnmpGet(snmp->getSnmpVersion(), snmp, oid, NULL, 0, &state, sizeof(UINT32), 0);
         switch(state)
         {
            case 1:
               *operState = IF_OPER_STATE_UP;
               break;
            case 2:  // down: interface is down
            case 7:  // lowerLayerDown: down due to state of lower-layer interface(s)
               *operState = IF_OPER_STATE_DOWN;
               break;
            case 3:
					*operState = IF_OPER_STATE_TESTING;
               break;
            case 5:
               *operState = IF_OPER_STATE_DORMANT;
               break;
            case 6:
               *operState = IF_OPER_STATE_NOT_PRESENT;
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
static UINT32 HandlerVlanList(SNMP_Variable *var, SNMP_Transport *transport, void *arg)
{
   VlanList *vlanList = (VlanList *)arg;

	VlanInfo *vlan = new VlanInfo(var->getName().getLastElement(), VLAN_PRM_BPORT);

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
static UINT32 HandlerVlanEgressPorts(SNMP_Variable *var, SNMP_Transport *transport, void *arg)
{
   VlanList *vlanList = (VlanList *)arg;
	UINT32 vlanId = var->getName().getLastElement();
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
 * @param node Node
 * @param driverData driver-specific data previously created in analyzeDevice
 * @return VLAN list or NULL
 */
VlanList *NetworkDeviceDriver::getVlans(SNMP_Transport *snmp, NObject *node, DriverData *driverData)
{
	VlanList *list = new VlanList();
	
   // dot1qVlanStaticName
	if (SnmpWalk(snmp, _T(".1.3.6.1.2.1.17.7.1.4.3.1.1"), HandlerVlanList, list) != SNMP_ERR_SUCCESS)
		goto failure;

   // dot1qVlanCurrentEgressPorts
	if (SnmpWalk(snmp, _T(".1.3.6.1.2.1.17.7.1.4.2.1.4"), HandlerVlanEgressPorts, list) != SNMP_ERR_SUCCESS)
		goto failure;

   if (list->getData() == nullptr)
   {
      // Some devices does not return anything under dot1qVlanCurrentEgressPorts.
      // In that case we use dot1qVlanStaticEgressPorts
	   if (SnmpWalk(snmp, _T(".1.3.6.1.2.1.17.7.1.4.3.1.2"), HandlerVlanEgressPorts, list) != SNMP_ERR_SUCCESS)
		   goto failure;
   }

   return list;

failure:
   delete list;
   return nullptr;
}

/** 
 * Get orientation of the modules in the device. Default implementation always returns NDD_ORIENTATION_HORIZONTAL.
 *
 * @param snmp SNMP transport
 * @param node Node
 * @param driverData driver-specific data previously created in analyzeDevice
 * @return module orientation
 */
int NetworkDeviceDriver::getModulesOrientation(SNMP_Transport *snmp, NObject *node, DriverData *driverData)
{
	return NDD_ORIENTATION_HORIZONTAL;
}

/** 
 * Get port layout of given module. Default implementation always set NDD_PN_UNKNOWN as layout.
 *
 * @param snmp SNMP transport
 * @param node Node
 * @param driverData driver-specific data previously created in analyzeDevice
 * @param module Module number (starting from 1)
 * @param layout Layout structure to fill
 */
void NetworkDeviceDriver::getModuleLayout(SNMP_Transport *snmp, NObject *node, DriverData *driverData, int module, NDD_MODULE_LAYOUT *layout)
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
 * @param node Node
 * @param driverData driver-specific data previously created in analyzeDevice
 * @return cluster mode (one of CLUSTER_MODE_STANDALONE, CLUSTER_MODE_ACTIVE, CLUSTER_MODE_STANDBY)
 */
int NetworkDeviceDriver::getClusterMode(SNMP_Transport *snmp, NObject *node, DriverData *driverData)
{
   return CLUSTER_MODE_STANDALONE;
}

/**
 * Returns true if device is a wireless controller. Default implementation always return false.
 *
 * @param snmp SNMP transport
 * @param node Node
 * @param driverData driver-specific data previously created in analyzeDevice
 * @return true if device is a wireless controller
 */
bool NetworkDeviceDriver::isWirelessController(SNMP_Transport *snmp, NObject *node, DriverData *driverData)
{
   return false;
}

/**
 * Get list of wireless access points managed by this controller. Default implementation always return NULL.
 *
 * @param snmp SNMP transport
 * @param node Node
 * @param driverData driver-specific data previously created in analyzeDevice
 * @return list of access points
 */
ObjectArray<AccessPointInfo> *NetworkDeviceDriver::getAccessPoints(SNMP_Transport *snmp, NObject *node, DriverData *driverData)
{
   return NULL;
}

/**
 * Get list of associated wireless stations. Default implementation always return NULL.
 *
 * @param snmp SNMP transport
 * @param node Node
 * @param driverData driver-specific data previously created in analyzeDevice
 * @return list of associated wireless stations
 */
ObjectArray<WirelessStationInfo> *NetworkDeviceDriver::getWirelessStations(SNMP_Transport *snmp, NObject *node, DriverData *driverData)
{
   return NULL;
}

/**
 * Get access point state
 *
 * @param snmp SNMP transport
 * @param node Node
 * @param driverData driver-specific data previously created in analyzeDevice
 * @param apIndex access point index
 * @param macAddr access point MAC address
 * @param ipAddr access point IP address
 * @param radioInterfaces list of radio interfaces for this AP
 * @return state of access point or AP_UNKNOWN if it cannot be determined
 */
AccessPointState NetworkDeviceDriver::getAccessPointState(SNMP_Transport *snmp, NObject *node, DriverData *driverData,
                                                          UINT32 apIndex, const MacAddress& macAddr, const InetAddress& ipAddr,
                                                          const ObjectArray<RadioInterfaceInfo> *radioInterfaces)
{
   return AP_UNKNOWN;
}

/**
 * Check if driver can provide additional metrics
 */
bool NetworkDeviceDriver::hasMetrics()
{
   return false;
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
DataCollectionError NetworkDeviceDriver::getMetric(SNMP_Transport *snmp, NObject *node, DriverData *driverData, const TCHAR *name, TCHAR *value, size_t size)
{
   return DCE_NOT_SUPPORTED;
}

/**
 * Get list of metrics supported by driver
 *
 * @param snmp SNMP transport
 * @param node Node
 * @param driverData driver-specific data previously created in analyzeDevice
 * @return list of metrics supported by driver or NULL on error
 */
ObjectArray<AgentParameterDefinition> *NetworkDeviceDriver::getAvailableMetrics(SNMP_Transport *snmp, NObject *node, DriverData *driverData)
{
   return NULL;
}

/**
 * Register standard host MIB metrics
 *
 * @param metrics list of driver's metrics to fill
 */
void NetworkDeviceDriver::registerHostMibMetrics(ObjectArray<AgentParameterDefinition> *metrics)
{
   metrics->add(new AgentParameterDefinition(_T("HostMib.Memory.Physical.Free"), _T("Free physical memory"), DCI_DT_UINT64));
   metrics->add(new AgentParameterDefinition(_T("HostMib.Memory.Physical.FreePerc"), _T("Percentage of free physical memory"), DCI_DT_FLOAT));
   metrics->add(new AgentParameterDefinition(_T("HostMib.Memory.Physical.Total"), _T("Total physical memory"), DCI_DT_UINT64));
   metrics->add(new AgentParameterDefinition(_T("HostMib.Memory.Physical.Used"), _T("Used physical memory"), DCI_DT_UINT64));
   metrics->add(new AgentParameterDefinition(_T("HostMib.Memory.Physical.UsedPerc"), _T("Percentage of used physical memory"), DCI_DT_FLOAT));
}

/**
 * Get value of standard host MIB metric
 *
 * @param snmp SNMP transport
 * @param driverData driver-specific data previously created in analyzeDevice (must be derived from HostMibDriverData)
 * @param name metric name
 * @param value buffer for metric value (size at least MAX_RESULT_LENGTH)
 * @param size buffer size
 * @return data collection error code
 */
DataCollectionError NetworkDeviceDriver::getHostMibMetric(SNMP_Transport *snmp, HostMibDriverData *driverData, const TCHAR *name, TCHAR *value, size_t size)
{
   const HostMibStorageEntry *e;
   const TCHAR *suffix;
   if (!_tcsnicmp(name, _T("HostMib.Memory.Physical."), 24))
   {
      e = driverData->getPhysicalMemory(snmp);
      suffix = &name[24];
   }
   else
   {
      return DCE_NOT_SUPPORTED;
   }

   return ((e != NULL) && e->getMetric(suffix, value, size)) ? DCE_SUCCESS : DCE_NOT_SUPPORTED;
}

/**
 * Handler for ARP enumeration
 */
static UINT32 HandlerArp(SNMP_Variable *var, SNMP_Transport *transport, void *arg)
{
   SNMP_PDU request(SNMP_GET_REQUEST, SnmpNewRequestId(), transport->getSnmpVersion());

   SNMP_ObjectId oid = var->getName();
   oid.changeElement(oid.length() - 6, 1);   // ifIndex
   request.bindVariable(new SNMP_Variable(oid));

   oid.changeElement(oid.length() - 6, 2);   // MAC address
   request.bindVariable(new SNMP_Variable(oid));

   SNMP_PDU *response;
   UINT32 rcc = transport->doRequest(&request, &response, SnmpGetDefaultTimeout(), 3);
   if (rcc != SNMP_ERR_SUCCESS)
      return rcc;

   if (response->getNumVariables() == request.getNumVariables())
   {
      ArpCache *arpCache = static_cast<ArpCache*>(arg);
      arpCache->addEntry(ntohl(var->getValueAsUInt()), response->getVariable(1)->getValueAsMACAddr(), response->getVariable(0)->getValueAsUInt());
   }

   delete response;
   return SNMP_ERR_SUCCESS;
}

/**
 * Get ARP cache
 *
 * @param snmp SNMP transport
 * @param driverData driver-specific data previously created in analyzeDevice (must be derived from HostMibDriverData)
 * @return ARP cache or NULL on failure
 */
ArpCache *NetworkDeviceDriver::getArpCache(SNMP_Transport *snmp, DriverData *driverData)
{
   ArpCache *arpCache = new ArpCache();
   if (SnmpWalk(snmp, _T(".1.3.6.1.2.1.4.22.1.3"), HandlerArp, arpCache) != SNMP_ERR_SUCCESS)
   {
      arpCache->decRefCount();
      arpCache = NULL;
   }
   return arpCache;
}
