/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2023 Victor Kirhenshtein
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
#include <netxms-version.h>

#define DEBUG_TAG _T("ndd.common")

/**
 * 2.4 GHz (802.11b/g/n) frequency to channel number map
 */
static int s_frequencyMap[14] = { 2412, 2417, 2422, 2427, 2432, 2437, 2442, 2447, 2452, 2457, 2462, 2467, 2472, 2484 };

/**
 * Helper function for converting frequency to channel number
 */
int LIBNXSRV_EXPORTABLE WirelessFrequencyToChannel(int freq)
{
   if ((freq >= 5000) && (freq < 6000))
      return (freq - 5000) / 5;

   for(int i = 0; i < 14; i++)
   {
      if (s_frequencyMap[i] == freq)
         return i + 1;
   }

   return 0;
}

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
void DriverData::attachToNode(uint32_t nodeId, const uuid& nodeGuid, const TCHAR *nodeName)
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
	return nullptr;
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
 * Get device geographical location.
 *
 * @param snmp SNMP transport
 * @param node Node
 * @param driverData driver data
 * @return device geographical location or "UNSET" type location object
 */
GeoLocation NetworkDeviceDriver::getGeoLocation(SNMP_Transport *snmp, NObject *node, DriverData *driverData)
{
   return GeoLocation();
}

/**
 * Handler for enumerating indexes
 */
static uint32_t HandlerIndex(SNMP_Variable *var, SNMP_Transport *transport, InterfaceList *ifList)
{
	ifList->add(new InterfaceInfo(var->getValueAsUInt()));
   return SNMP_ERR_SUCCESS;
}

/**
 * Handler for enumerating indexes via ifXTable
 */
static uint32_t HandlerIndexIfXTable(SNMP_Variable *var, SNMP_Transport *transport, InterfaceList *ifList)
{
   const SNMP_ObjectId& name = var->getName();
   uint32_t index = name.value()[name.length() - 1];
   if (ifList->findByIfIndex(index) == nullptr)
   {
	   ifList->add(new InterfaceInfo(index));
   }
   return SNMP_ERR_SUCCESS;
}

/**
 * Handler for enumerating IP addresses via ipAddrTable
 */
static uint32_t HandlerIpAddr(SNMP_Variable *var, SNMP_Transport *transport, InterfaceList *ifList)
{
   // Get address type and prefix
   SNMP_ObjectId oid = var->getName();
   SNMP_PDU request(SNMP_GET_REQUEST, SnmpNewRequestId(), transport->getSnmpVersion());
   oid.changeElement(9, 3); // ipAdEntNetMask
   request.bindVariable(new SNMP_Variable(oid));
   oid.changeElement(9, 2); // ipAdEntIfIndex
   request.bindVariable(new SNMP_Variable(oid));
   SNMP_PDU *response;
   uint32_t rc = transport->doRequest(&request, &response);
   if (rc == SNMP_ERR_SUCCESS)
   {
      // check number of varbinds and address type (1 = unicast)
      if ((response->getErrorCode() == SNMP_PDU_ERR_SUCCESS) && (response->getNumVariables() == 2))
      {
         InterfaceInfo *iface = ifList->findByIfIndex(response->getVariable(1)->getValueAsUInt());
         if (iface != nullptr)
         {
            iface->ipAddrList.add(InetAddress(ntohl(var->getValueAsUInt()), ntohl(response->getVariable(0)->getValueAsUInt())));
         }
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG, 6, _T("NetworkDeviceDriver::getInterfaces(%p): SNMP GET in HandlerIpAddr failed (%s)"), transport, SnmpGetProtocolErrorText(response->getErrorCode()));
      }
      delete response;
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 6, _T("NetworkDeviceDriver::getInterfaces(%p): SNMP GET in HandlerIpAddr failed (%s)"), transport, SnmpGetErrorText(rc));
   }
   return rc;
}

/**
 * Build IP address from OID part
 */
static InetAddress InetAddressFromOID(const uint32_t* oid, bool withMask)
{
   InetAddress addr;
   if (((oid[0] == 1) && (oid[1] == 4)) || ((oid[0] == 3) && (oid[1] == 8))) // ipv4 and ipv4z
   {
      addr = InetAddress((oid[2] << 24) | (oid[3] << 16) | (oid[4] << 8) | oid[5]);
      if (withMask)
         addr.setMaskBits(static_cast<int>(*(oid + oid[1] + 2)));
   }
   else if (((oid[0] == 2) && (oid[1] == 16)) || ((oid[0] == 4) && (oid[1] == 20))) // ipv6 and ipv6z
   {
      BYTE bytes[16];
      const uint32_t *p = oid + 2;
      for(int i = 0; i < 16; i++)
         bytes[i] = static_cast<BYTE>(*p++);
      addr = InetAddress(bytes);
      if (withMask)
         addr.setMaskBits(static_cast<int>(*(oid + oid[1] + 2)));
   }
   return addr;
}

/**
 * Handler for enumerating IP addresses via ipAddressTable
 */
static uint32_t HandlerIpAddressTable(SNMP_Variable *var, SNMP_Transport *transport, InterfaceList *ifList)
{
   uint32_t oid[128];
   size_t oidLen = var->getName().length();
   memcpy(oid, var->getName().value(), oidLen * sizeof(uint32_t));

   uint32_t ifIndex = var->getValueAsUInt();
   InterfaceInfo *iface = ifList->findByIfIndex(ifIndex);
   if (iface == nullptr)
      return SNMP_ERR_SUCCESS;

   // Build IP address from OID
   InetAddress addr = InetAddressFromOID(&oid[10], false);
   if (!addr.isValid())
      return SNMP_ERR_SUCCESS;   // Unknown or unsupported address format

   if (iface->hasAddress(addr))
      return SNMP_ERR_SUCCESS;   // This IP already set from ipAddrTable

   // Get address type and prefix
   SNMP_PDU request(SNMP_GET_REQUEST, SnmpNewRequestId(), transport->getSnmpVersion());
   oid[9] = 4; // ipAddressType
   request.bindVariable(new SNMP_Variable(oid, oidLen));
   oid[9] = 5; // ipAddressPrefix
   request.bindVariable(new SNMP_Variable(oid, oidLen));
   SNMP_PDU *response;
   if (transport->doRequest(&request, &response) == SNMP_ERR_SUCCESS)
   {
      // check number of varbinds and address type (1 = unicast)
      if ((response->getNumVariables() == 2) && (response->getVariable(0)->getValueAsInt() == 1))
      {
         static SNMP_ObjectId ipAddressPrefixEntry = SNMP_ObjectId::parse(_T(".1.3.6.1.2.1.4.32.1"));
         SNMP_ObjectId prefix = response->getVariable(1)->getValueAsObjectId();
         if (prefix.isValid() && !prefix.isZeroDotZero() && (prefix.compare(ipAddressPrefixEntry) == OID_LONGER))
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
static uint32_t HandlerIpAddressPrefixTable(SNMP_Variable *var, SNMP_Transport *transport, InterfaceList *ifList)
{
   const uint32_t *oid = var->getName().value();
   
   // Build IP address from OID
   InetAddress prefix = InetAddressFromOID(&oid[11], true);
   if (!prefix.isValid())
      return SNMP_ERR_SUCCESS;   // Unknown or unsupported address format

   // Find matching IP and set mask
   InterfaceInfo *iface = ifList->findByIfIndex(oid[10]);
   if (iface != nullptr)
   {
      for(int i = 0; i < iface->ipAddrList.size(); i++)
      {
         InetAddress *addr = iface->ipAddrList.getList().get(i);
         if ((addr != nullptr) && prefix.contain(*addr))
         {
            addr->setMaskBits(prefix.getMaskBits());
         }
      }
   }
   return SNMP_ERR_SUCCESS;
}

/**
 * Handler for enumerating IP address prefixes via local routes in inetCidrRouteTable
 */
static uint32_t HandlerInetCidrRouteTable(SNMP_Variable *var, SNMP_Transport *transport, InterfaceList *ifList)
{
   if (var->getName().length() < 27)
      return SNMP_ERR_SUCCESS;

   uint32_t oid[256];
   memset(oid, 0, sizeof(oid));
   memcpy(oid, var->getName().value(), var->getName().length() * sizeof(uint32_t));

   // Check route type, only use 1 (other) and 3 (local)
   if ((var->getValueAsInt() != 1) && (var->getValueAsInt() != 3))
      return SNMP_ERR_SUCCESS;

   // Build IP address and next hop from OID
   InetAddress prefix = InetAddressFromOID(&oid[11], true);
   if (!prefix.isValid() || prefix.isAnyLocal() || prefix.isMulticast() || (prefix.getMaskBits() == 0) || (prefix.getHostBits() == 0))
      return SNMP_ERR_SUCCESS;   // Unknown or unsupported address format, or prefix of no interest

   uint32_t *policy = oid + oid[12] + 14; // Policy follows prefix, oid[12] contains prefix length
   if (static_cast<size_t>(policy - oid + 3) >= var->getName().length())
      return SNMP_ERR_SUCCESS;   // Check that length is valid and do not point beyond OID end

   InetAddress nextHop = InetAddressFromOID(policy + policy[0] + 1, false);
   if (!nextHop.isValid() || !nextHop.isAnyLocal())
      return SNMP_ERR_SUCCESS;   // Unknown or unsupported address format, or next hop is not 0.0.0.0

   // Get interface index
   oid[10] = 7;   // inetCidrRouteIfIndex
   uint32_t ifIndex = 0;
   if (SnmpGetEx(transport, nullptr, oid, var->getName().length(), &ifIndex, sizeof(uint32_t), 0, nullptr) == SNMP_ERR_SUCCESS)
   {
      InterfaceInfo *iface = ifList->findByIfIndex(ifIndex);
      if (iface != nullptr)
      {
         for(int i = 0; i < iface->ipAddrList.size(); i++)
         {
            InetAddress *addr = iface->ipAddrList.getList().get(i);
            if ((addr != nullptr) && (addr->getHostBits() == 0) && prefix.contain(*addr))
            {
               addr->setMaskBits(prefix.getMaskBits());
            }
         }
      }
   }
   return SNMP_ERR_SUCCESS;
}

/**
 * Get interface speed
 */
uint64_t NetworkDeviceDriver::getInterfaceSpeed(SNMP_Transport *snmp, uint32_t ifIndex, int ifTableSuffixLen, const uint32_t *ifTableSuffix)
{
   TCHAR oid[256], suffix[128];

   // try ifHighSpeed first
   if (ifTableSuffixLen > 0)
      _sntprintf(oid, 256, _T(".1.3.6.1.2.1.31.1.1.1.15%s"), SnmpConvertOIDToText(ifTableSuffixLen, ifTableSuffix, suffix, 128));
   else
      _sntprintf(oid, 256, _T(".1.3.6.1.2.1.31.1.1.1.15.%u"), ifIndex);

   uint64_t speed;

   uint32_t ifHighSpeed;
   if (SnmpGet(snmp->getSnmpVersion(), snmp, oid, nullptr, 0, &ifHighSpeed, sizeof(uint32_t), 0) != SNMP_ERR_SUCCESS)
   {
      ifHighSpeed = 0;
   }
   if (ifHighSpeed < 2000)  // ifHighSpeed not supported or slow interface, use ifSpeed
   {
      if (ifTableSuffixLen > 0)
         _sntprintf(oid, 256, _T(".1.3.6.1.2.1.2.2.1.5%s"), SnmpConvertOIDToText(ifTableSuffixLen, ifTableSuffix, suffix, 128));
      else
         _sntprintf(oid, 256, _T(".1.3.6.1.2.1.2.2.1.5.%u"), ifIndex);

      uint32_t ifSpeed;
      if (SnmpGet(snmp->getSnmpVersion(), snmp, oid, nullptr, 0, &ifSpeed, sizeof(uint32_t), 0) != SNMP_ERR_SUCCESS)
      {
         ifSpeed = 0;
      }
      speed = ifSpeed;
   }
   else
   {
      speed = static_cast<uint64_t>(ifHighSpeed) * _ULL(1000000);
   }

   return speed;
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
InterfaceList *NetworkDeviceDriver::getInterfaces(SNMP_Transport *snmp, NObject *node, DriverData *driverData, bool useIfXTable)
{
   bool success = false;

	nxlog_debug_tag(DEBUG_TAG, 6, _T("NetworkDeviceDriver::getInterfaces(%p,%s)"), snmp, BooleanToString(useIfXTable));

   // Get number of interfaces
	// Some devices may not return ifNumber at all or return completely insane values
   // (for example, DLink DGS-3612 can return negative value)
   // Anyway it's just a hint for initial array size
	uint32_t interfaceCount = 0;
	SnmpGet(snmp->getSnmpVersion(), snmp, _T(".1.3.6.1.2.1.2.1.0"), nullptr, 0, &interfaceCount, sizeof(LONG), 0);
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
	      _sntprintf(oid, 128, _T(".1.3.6.1.2.1.2.2.1.2.%u"), iface->index);
	      if (SnmpGet(snmp->getSnmpVersion(), snmp, oid, nullptr, 0, iface->description, MAX_DB_STRING * sizeof(TCHAR), 0) != SNMP_ERR_SUCCESS)
         {
            // Try to get name from ifXTable
	         _sntprintf(oid, 128, _T(".1.3.6.1.2.1.31.1.1.1.1.%u"), iface->index);
	         if (SnmpGet(snmp->getSnmpVersion(), snmp, oid, nullptr, 0, iface->description, MAX_DB_STRING * sizeof(TCHAR), 0) != SNMP_ERR_SUCCESS)
	         {
	            nxlog_debug_tag(DEBUG_TAG, 6, _T("NetworkDeviceDriver::getInterfaces(%p): cannot read interface description for interface %u"), snmp, iface->index);
   	         continue;
	         }
         }

         // Get interface alias
	      _sntprintf(oid, 128, _T(".1.3.6.1.2.1.31.1.1.1.18.%u"), iface->index);  // ifAlias
			if (SnmpGet(snmp->getSnmpVersion(), snmp, oid, nullptr, 0, iface->alias, MAX_DB_STRING * sizeof(TCHAR), 0) == SNMP_ERR_SUCCESS)
         {
            Trim(iface->alias);
         }
         else
         {
            iface->alias[0] = 0;
         }

			// Try to get interface name from ifXTable, if unsuccessful or disabled, use ifDescr from ifTable
         _sntprintf(oid, 128, _T(".1.3.6.1.2.1.31.1.1.1.1.%u"), iface->index);
         if (!useIfXTable ||
				 (SnmpGet(snmp->getSnmpVersion(), snmp, oid, nullptr, 0, iface->name, MAX_DB_STRING * sizeof(TCHAR), 0) != SNMP_ERR_SUCCESS))
         {
		      _tcslcpy(iface->name, iface->description, MAX_DB_STRING);
		   }

         // Interface type
         _sntprintf(oid, 128, _T(".1.3.6.1.2.1.2.2.1.3.%u"), iface->index);
         if (SnmpGet(snmp->getSnmpVersion(), snmp, oid, nullptr, 0, &iface->type, sizeof(uint32_t), 0) != SNMP_ERR_SUCCESS)
			{
				iface->type = IFTYPE_OTHER;
			}

         // Interface MTU
         _sntprintf(oid, 128, _T(".1.3.6.1.2.1.2.2.1.4.%u"), iface->index);
         if (SnmpGet(snmp->getSnmpVersion(), snmp, oid, nullptr, 0, &iface->mtu, sizeof(uint32_t), 0) != SNMP_ERR_SUCCESS)
			{
				iface->mtu = 0;
			}

         // Interface speed
         iface->speed = getInterfaceSpeed(snmp, iface->index, 0, nullptr);

         // MAC address
         _sntprintf(oid, 128, _T(".1.3.6.1.2.1.2.2.1.6.%u"), iface->index);
         BYTE buffer[256];
         memset(buffer, 0, sizeof(buffer));
         if (SnmpGet(snmp->getSnmpVersion(), snmp, oid, nullptr, 0, buffer, sizeof(buffer), SG_RAW_RESULT) == SNMP_ERR_SUCCESS)
			{
	         memcpy(iface->macAddr, buffer, MAC_ADDR_LENGTH);
			}
			else
			{
				// Unable to get MAC address
	         memset(iface->macAddr, 0, MAC_ADDR_LENGTH);
			}
      }

      // Interface IP addresses and netmasks from ipAddrTable
		if (!node->getCustomAttributeAsBoolean(_T("snmp.ignore.ipAddrTable"), false))
		{
         uint32_t error = SnmpWalk(snmp, _T(".1.3.6.1.2.1.4.20.1.1"), HandlerIpAddr, ifList);
         if (error == SNMP_ERR_SUCCESS)
         {
            success = true;
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG, 6, _T("NetworkDeviceDriver::getInterfaces(%p): SNMP WALK .1.3.6.1.2.1.4.20.1.1 failed (%s)"), snmp, SnmpGetErrorText(error));
         }
		}
		else
		{
         success = true;
		}

      // Get IP addresses from ipAddressTable if available
      if (!node->getCustomAttributeAsBoolean(_T("snmp.ignore.ipAddressTable"), false))
      {
         SnmpWalk(snmp, _T(".1.3.6.1.2.1.4.34.1.3"), HandlerIpAddressTable, ifList);
         if (ifList->isPrefixWalkNeeded())
         {
            SnmpWalk(snmp, _T(".1.3.6.1.2.1.4.32.1.5"), HandlerIpAddressPrefixTable, ifList);
            SnmpWalk(snmp, _T(".1.3.6.1.2.1.4.24.7.1.8"), HandlerInetCidrRouteTable, ifList);
         }
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
 * @param ifName interface name
 * @param ifType interface type
 * @param ifTableSuffixLen length of interface table suffix
 * @param ifTableSuffix interface table suffix
 * @param adminState OUT: interface administrative state
 * @param operState OUT: interface operational state
 * @param speed OUT: updated interface speed
 */
void NetworkDeviceDriver::getInterfaceState(SNMP_Transport *snmp, NObject *node, DriverData *driverData, uint32_t ifIndex, const TCHAR *ifName,
         uint32_t ifType, int ifTableSuffixLen, const uint32_t *ifTableSuffix, InterfaceAdminState *adminState, InterfaceOperState *operState, uint64_t *speed)
{
   uint32_t state = 0;
   TCHAR oid[256], suffix[128];
   if (ifTableSuffixLen > 0)
      _sntprintf(oid, 256, _T(".1.3.6.1.2.1.2.2.1.7%s"), SnmpConvertOIDToText(ifTableSuffixLen, ifTableSuffix, suffix, 128)); // Interface administrative state
   else
      _sntprintf(oid, 256, _T(".1.3.6.1.2.1.2.2.1.7.%u"), ifIndex); // Interface administrative state
   SnmpGet(snmp->getSnmpVersion(), snmp, oid, nullptr, 0, &state, sizeof(uint32_t), 0);

   switch(state)
   {
		case 2:
			*adminState = IF_ADMIN_STATE_DOWN;
			*operState = IF_OPER_STATE_DOWN;
         break;
      case 1:
		case 3:
			*adminState = static_cast<InterfaceAdminState>(state);
         // Get interface operational state
         state = 0;
         if (ifTableSuffixLen > 0)
            _sntprintf(oid, 256, _T(".1.3.6.1.2.1.2.2.1.8%s"), SnmpConvertOIDToText(ifTableSuffixLen, ifTableSuffix, suffix, 128));
         else
            _sntprintf(oid, 256, _T(".1.3.6.1.2.1.2.2.1.8.%u"), ifIndex);
         SnmpGet(snmp->getSnmpVersion(), snmp, oid, nullptr, 0, &state, sizeof(uint32_t), 0);
         switch(state)
         {
            case 1:
               *operState = IF_OPER_STATE_UP;
               *speed = getInterfaceSpeed(snmp, ifIndex, ifTableSuffixLen, ifTableSuffix);
               break;
            case 2:  // down: interface is down
            case 7:  // lowerLayerDown: down due to state of lower-layer interface(s)
               *operState = IF_OPER_STATE_DOWN;
               break;
            case 3:
					*operState = IF_OPER_STATE_TESTING;
               *speed = getInterfaceSpeed(snmp, ifIndex, ifTableSuffixLen, ifTableSuffix);
               break;
            case 5:
               *operState = IF_OPER_STATE_DORMANT;
               *speed = getInterfaceSpeed(snmp, ifIndex, ifTableSuffixLen, ifTableSuffix);
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
 * Translate LLDP port name (port ID subtype 5) to local interface id. Default implementation always returns false.
 *
 * @param snmp SNMP transport
 * @param node Node
 * @param driverData driver's data
 * @param lldpName port name received from LLDP MIB
 * @param id interface ID structure to be filled at success
 * @return true if interface identification provided
 */
bool NetworkDeviceDriver::lldpNameToInterfaceId(SNMP_Transport *snmp, NObject *node, DriverData *driverData, const TCHAR *lldpName, InterfaceId *id)
{
   return false;
}

/**
 * Returns true if lldpRemTable uses ifIndex instead of bridge port number for referencing interfaces.
 * Default implementation always return false;
 *
 * @param node Node
 * @param driverData driver-specific data previously created in analyzeDevice
 * @return true if lldpRemTable uses ifIndex instead of bridge port number
 */
bool NetworkDeviceDriver::isLldpRemTableUsingIfIndex(const NObject *node, DriverData *driverData)
{
   return false;
}

/**
 * Handler for VLAN enumeration
 */
static uint32_t HandlerVlanList(SNMP_Variable *var, SNMP_Transport *transport, VlanList *vlanList)
{
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

	uint32_t port = offset;
	BYTE mask = 0x80;
	while(mask > 0)
	{
		if (map & mask)
		{
			vlan->add(port);
		}
		mask >>= 1;
		port++;
	}
}

/**
 * Handler for VLAN egress port enumeration
 */
static uint32_t HandlerVlanEgressPorts(SNMP_Variable *var, SNMP_Transport *transport, VlanList *vlanList)
{
   uint32_t vlanId = var->getName().getLastElement();
	VlanInfo *vlan = vlanList->findById(vlanId);
	if (vlan != nullptr)
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
 * Returns true if FDB uses ifIndex instead of bridge port number for referencing interfaces.
 * Default implementation always return false;
 *
 * @param node Node
 * @param driverData driver-specific data previously created in analyzeDevice
 * @return true if FDB uses ifIndex instead of bridge port number for referencing interfaces
 */
bool NetworkDeviceDriver::isFdbUsingIfIndex(const NObject *node, DriverData *driverData)
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

   return ((e != nullptr) && e->getMetric(suffix, value, size)) ? DCE_SUCCESS : DCE_NOT_SUPPORTED;
}

/**
 * Handler for ARP enumeration.
 * ipNetToMediaTable indexed by ipNetToMediaIfIndex followed by ipNetToMediaNetAddress
 */
static uint32_t HandlerArp(SNMP_Variable *var, SNMP_Transport *transport, ArpCache *arpCache)
{
   MacAddress macAddr = var->getValueAsMACAddr();
   if (macAddr.isValid())
   {
      const SNMP_ObjectId& oid = var->getName();
      uint32_t ifIndex = oid.getElement(oid.length() - 5);
      uint32_t ipAddr = (oid.getElement(oid.length() - 4) << 24) | (oid.getElement(oid.length() - 3) << 16) | (oid.getElement(oid.length() - 2) << 8) | oid.getElement(oid.length() - 1);
      arpCache->addEntry(InetAddress(ipAddr), macAddr, ifIndex);
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
shared_ptr<ArpCache> NetworkDeviceDriver::getArpCache(SNMP_Transport *snmp, DriverData *driverData)
{
   shared_ptr<ArpCache> arpCache = make_shared<ArpCache>();
   if (SnmpWalk(snmp, _T(".1.3.6.1.2.1.4.22.1.2"), HandlerArp, arpCache.get()) != SNMP_ERR_SUCCESS)
      arpCache.reset();
   return arpCache;
}
