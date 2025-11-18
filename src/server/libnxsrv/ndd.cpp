/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2025 Victor Kirhenshtein
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
 * Get display name for radio band
 */
const TCHAR LIBNXSRV_EXPORTABLE *RadioBandDisplayName(RadioBand band)
{
   switch(band)
   {
      case RADIO_BAND_2_4_GHZ:
         return _T("2.4 GHz");
      case RADIO_BAND_3_65_GHZ:
         return _T("3.65 GHz");
      case RADIO_BAND_5_GHZ:
         return _T("5 GHz");
      case RADIO_BAND_6_GHZ:
         return _T("6 GHz");
      default:
         return _T("Unknown");
   }
}

/**
 * Helper function for converting frequency to radio band
 */
RadioBand LIBNXSRV_EXPORTABLE WirelessFrequencyToBand(int freq)
{
   if ((freq >= 2412 ) && (freq <= 2484))
      return RADIO_BAND_2_4_GHZ;
   if ((freq >= 3657 ) && (freq <= 3693))
      return RADIO_BAND_3_65_GHZ;
   if ((freq >= 4915 ) && (freq <= 5825))
      return RADIO_BAND_5_GHZ;
   if ((freq >= 5925 ) && (freq <= 7125))
      return RADIO_BAND_6_GHZ;
   return RADIO_BAND_UNKNOWN;
}

/**
 * Helper function for converting frequency to channel number
 */
uint16_t LIBNXSRV_EXPORTABLE WirelessFrequencyToChannel(int freq)
{
   if ((freq >= 5000) && (freq <= 5825))
      return static_cast<uint16_t>((freq - 5000) / 5);
   if ((freq >= 4915) && (freq <= 4980))
      return static_cast<uint16_t>(freq / 5 - 800);

   for(int i = 0; i < 14; i++)
   {
      if (s_frequencyMap[i] == freq)
         return static_cast<uint16_t>(i + 1);
   }

   return 0;
}

/**
 * Helper function for converting wureless channel to frequency
 */
uint16_t LIBNXSRV_EXPORTABLE WirelessChannelToFrequency(RadioBand band, uint16_t channel)
{
   switch(band)
   {
      case RADIO_BAND_2_4_GHZ:
         return ((channel > 0) && (channel < 15)) ? s_frequencyMap[channel - 1] : 0;
      case RADIO_BAND_5_GHZ:
         return (channel >= 184) ? ((channel + 800) * 5) : (channel * 5 + 5000);
      default:
         return 0;
   }
}

/**
 * Compare two lists of radio interfaces
 */
bool LIBNXSRV_EXPORTABLE CompareRadioInterfaceLists(const StructArray<RadioInterfaceInfo> *list1, const StructArray<RadioInterfaceInfo> *list2)
{
   if (list1 == nullptr)
      return list2 == nullptr;

   if (list2 == nullptr)
      return false;

   if (list1->size() != list2->size())
      return false;

   for(int i = 0; i < list1->size(); i++)
   {
      RadioInterfaceInfo *r1 = list1->get(i);
      RadioInterfaceInfo *r2 = nullptr;
      for(int j = 0; j < list2->size(); j++)
      {
         RadioInterfaceInfo *r = list2->get(j);
         if (r1->index == r->index)
         {
            r2 = r;
            break;
         }
      }
      if (r2 == nullptr)
         return false;
      if (memcmp(r1->bssid, r2->bssid, MAC_ADDR_LENGTH) ||
          (r1->frequency != r2->frequency) ||
          (r1->band != r2->band) ||
          (r1->channel != r2->channel) ||
          (r1->ifIndex != r2->ifIndex) ||
          _tcscmp(r1->name, r2->name) ||
          (r1->powerDBm != r2->powerDBm) ||
          (r1->powerMW != r2->powerMW) ||
          _tcscmp(r1->ssid, r2->ssid))
         return false;
   }

   return true;
}

/**
 * Serialize radio interface information to JSON
 */
json_t *RadioInterfaceInfo::toJson() const
{
   json_t *root = json_object();
   json_object_set_new(root, "index", json_integer(index));
   json_object_set_new(root, "ifIndex", json_integer(ifIndex));
   json_object_set_new(root, "name", json_string_t(name));
   char bssidText[64];
   json_object_set_new(root, "bssid", json_string(BinToStrA(bssid, MAC_ADDR_LENGTH, bssidText)));
   json_object_set_new(root, "ssid", json_string_t(ssid));
   json_object_set_new(root, "band", json_integer(band));
   json_object_set_new(root, "channel", json_integer(channel));
   json_object_set_new(root, "frequency", json_integer(frequency));
   json_object_set_new(root, "powerDBm", json_integer(powerDBm));
   json_object_set_new(root, "powerMW", json_integer(powerMW));
   return root;
}

/**
 * Access point info constructor
 */
AccessPointInfo::AccessPointInfo(uint32_t index, const MacAddress& macAddr, const InetAddress& ipAddr, AccessPointState state,
         const TCHAR *name, const TCHAR *vendor, const TCHAR *model, const TCHAR *serial) : m_macAddr(macAddr), m_ipAddr(ipAddr), m_radioInterfaces(0, 4)
{
   m_index = index;
	m_state = state;
	m_name = MemCopyString(name);
	m_vendor = MemCopyString(vendor);
	m_model = MemCopyString(model);
	m_serial = MemCopyString(serial);
	m_controllerId = 0;
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
}

/**
 * Fill NXCP message with device view data
 */
void DeviceView::fillMessage(NXCPMessage *msg) const
{
   msg->setField(VID_NUM_ELEMENTS, m_elements.size());
   uint32_t fieldId = VID_ELEMENT_LIST_BASE;
   for(int i = 0; i < m_elements.size(); i++)
   {
      DeviceViewElement *e = m_elements.get(i);
      msg->setField(fieldId++, e->x);
      msg->setField(fieldId++, e->y);
      msg->setField(fieldId++, e->width);
      msg->setField(fieldId++, e->height);
      msg->setField(fieldId++, e->flags);
      msg->setField(fieldId++, e->backgroundColor.toInteger());
      msg->setField(fieldId++, e->borderColor.toInteger());
      msg->setField(fieldId++, e->imageName);
      msg->setField(fieldId++, e->commands);
      fieldId += 11;
   }

   msg->setField(VID_NUM_IMAGES, m_images.size());
   fieldId = VID_IMAGE_LIST_BASE;
   for(int i = 0; i < m_images.size(); i++)
   {
      DeviceViewImage *image = m_images.get(i);
      msg->setField(fieldId++, image->name);
      msg->setField(fieldId++, image->data, image->size);
      fieldId += 8;
   }
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
int NetworkDeviceDriver::isPotentialDevice(const SNMP_ObjectId& oid)
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
bool NetworkDeviceDriver::isDeviceSupported(SNMP_Transport *snmp, const SNMP_ObjectId& oid)
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
void NetworkDeviceDriver::analyzeDevice(SNMP_Transport *snmp, const SNMP_ObjectId& oid, NObject *node, DriverData **driverData)
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
 * Check if this driver supports ENTITY MIB emulation for given device.
 *
 * @param snmp SNMP transport
 * @param node Node
 * @param driverData driver data
 * @return true if this driver supports ENTITY MIB emulation for given device
 */
bool NetworkDeviceDriver::isEntityMibEmulationSupported(SNMP_Transport *snmp, NObject *node, DriverData *driverData)
{
   return false;
}

/**
 * Build hardware component tree (simulate ENTITY MIB for devices that does not support it).
 *
 * @param snmp SNMP transport
 * @param node Node
 * @param driverData driver data
 * @return hardware component tree or NULL
 */
shared_ptr<ComponentTree> NetworkDeviceDriver::buildComponentTree(SNMP_Transport *snmp, NObject *node, DriverData *driverData)
{
   return shared_ptr<ComponentTree>();
}

/**
 * Build device view. On success driver should return device view witl elements in drawing order.
 * First element should define a rectangle that encompasses all subsequent elements, therefore defining picture size.
 *
 * @param snmp SNMP transport
 * @param node Node
 * @param driverData driver data
 * @param rootComponent root element of component tree (can be NULL)
 * @return device view elements in drawing order or NULL
 */
shared_ptr<DeviceView> NetworkDeviceDriver::buildDeviceView(SNMP_Transport *snmp, NObject *node, DriverData *driverData, const Component *rootComponent)
{
   return shared_ptr<DeviceView>();
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
 * Build IP address from OID part (encoded as type length value)
 * If "withMask" set to true, OID element following address is interpreted as prefix length
 */
InetAddress LIBNXSRV_EXPORTABLE InetAddressFromOID(const uint32_t* oid, bool withMask, int *shift)
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
   if (shift != nullptr)
      *shift = static_cast<int>(oid[1]) + (withMask ? 3 : 2);
   return addr;
}

/**
 * Prefix value for ipAddressTable
 */
static SNMP_ObjectId s_ipAddressPrefixEntry { 1, 3, 6, 1, 2, 1, 4, 32, 1 };

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
   InetAddress addr = InetAddressFromOID(&oid[10], false, nullptr);
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
         SNMP_ObjectId prefix = response->getVariable(1)->getValueAsObjectId();
         if (prefix.isValid() && !prefix.isZeroDotZero() && (prefix.compare(s_ipAddressPrefixEntry) == OID_LONGER))
         {
            // Last element in ipAddressPrefixTable index is prefix length
            addr.setMaskBits((int)prefix.value()[prefix.length() - 1]);
         }
         else
         {
            TCHAR buffer[64];
            nxlog_debug_tag(DEBUG_TAG, 7, _T("NetworkDeviceDriver::getInterfaces(%p): no prefix reference for %s"), transport, addr.toString(buffer));
            ifList->setPrefixWalkNeeded();
            addr.setMaskBits(0);
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
   InetAddress prefix = InetAddressFromOID(&oid[11], true, nullptr);
   if (!prefix.isValid())
      return SNMP_ERR_SUCCESS;   // Unknown or unsupported address format

   // Find matching IP and set mask
   InterfaceInfo *iface = ifList->findByIfIndex(oid[10]);
   if (iface != nullptr)
   {
      for(int i = 0; i < iface->ipAddrList.size(); i++)
      {
         InetAddress *addr = iface->ipAddrList.getList().get(i);
         if ((addr != nullptr) && prefix.contains(*addr))
         {
            addr->setMaskBits(prefix.getMaskBits());
            TCHAR buffer[64];
            nxlog_debug_tag(DEBUG_TAG, 7, _T("NetworkDeviceDriver::getInterfaces(%p): mask bits set from prefix table: %s/%d"), transport, addr->toString(buffer), addr->getMaskBits());
         }
      }
   }
   return SNMP_ERR_SUCCESS;
}

/**
 * Handler for enumerating IP address prefixes via local routes in inetCidrRouteTable
 */
static void ProcessInetCidrRouteingTableEntry(SNMP_Variable *var, SNMP_Transport *transport, InterfaceList *ifList, const wchar_t *debugId)
{
   if (var->getName().length() < 27)
      return;

   uint32_t oid[256];
   memset(oid, 0, sizeof(oid));
   memcpy(oid, var->getName().value(), var->getName().length() * sizeof(uint32_t));

   // Check route type, only use 1 (other) and 3 (local)
   if ((var->getValueAsInt() != 1) && (var->getValueAsInt() != 3))
      return;

   // Build IP address and next hop from OID
   int shift;
   InetAddress prefix = InetAddressFromOID(&oid[11], true, &shift);
   if (!prefix.isValid() || prefix.isAnyLocal() || prefix.isMulticast() || (prefix.getMaskBits() == 0) || (prefix.getHostBits() == 0))
      return;   // Unknown or unsupported address format, or prefix of no interest

   uint32_t *policy = &oid[11 + shift]; // Policy follows prefix
   if (static_cast<size_t>(policy - oid + 3) >= var->getName().length())
      return;   // Check that length is valid and do not point beyond OID end

   InetAddress nextHop = InetAddressFromOID(policy + policy[0] + 1, false, nullptr);
   if (!nextHop.isValid() || !nextHop.isAnyLocal())
      return;   // Unknown or unsupported address format, or next hop is not 0.0.0.0

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
            if ((addr != nullptr) && (addr->getHostBits() == 0) && prefix.contains(*addr))
            {
               addr->setMaskBits(prefix.getMaskBits());
               TCHAR buffer[64];
               nxlog_debug_tag(DEBUG_TAG, 7, _T("NetworkDeviceDriver::getInterfaces(%s): mask bits set from routing table: %s/%d"), debugId, addr->toString(buffer), addr->getMaskBits());
            }
         }
      }
   }
}

/**
 * Get interface speed. Will not change value pointed by "speed" if interface speed cannot be retrieved.
 */
bool NetworkDeviceDriver::getInterfaceSpeed(SNMP_Transport *snmp, uint32_t ifIndex, int ifTableSuffixLen, const uint32_t *ifTableSuffix, uint64_t *speed)
{
   uint32_t oid[128] = { 1, 3, 6, 1, 2, 1, 31, 1, 1, 1, 15 };
   size_t oidLen;

   // try ifHighSpeed first
   if (ifTableSuffixLen > 0)
   {
      memcpy(&oid[11], ifTableSuffix, ifTableSuffixLen * sizeof(uint32_t));
      oidLen = 11 + ifTableSuffixLen;
   }
   else
   {
      oid[11] = ifIndex;
      oidLen = 12;
   }

   uint32_t ifHighSpeed;
   bool success = (SnmpGet(snmp->getSnmpVersion(), snmp, nullptr, oid, oidLen, &ifHighSpeed, sizeof(uint32_t), 0) == SNMP_ERR_SUCCESS);
   if (!success || (ifHighSpeed < 2000))  // ifHighSpeed not supported or slow interface, use ifSpeed
   {
      static uint32_t ifTable[] = { 1, 3, 6, 1, 2, 1, 2, 2, 1, 5 };
      memcpy(oid, ifTable, 10 * sizeof(uint32_t));
      if (ifTableSuffixLen > 0)
      {
         memcpy(&oid[10], ifTableSuffix, ifTableSuffixLen * sizeof(uint32_t));
         oidLen = 10 + ifTableSuffixLen;
      }
      else
      {
         oid[10] = ifIndex;
         oidLen = 11;
      }

      uint32_t ifSpeed;
      if (SnmpGet(snmp->getSnmpVersion(), snmp, nullptr, oid, oidLen, &ifSpeed, sizeof(uint32_t), 0) == SNMP_ERR_SUCCESS)
      {
         *speed = ifSpeed;
      }
      else if (success)
      {
         // Successfully got ifHighSpeed but not ifSpeed
         *speed = static_cast<uint64_t>(ifHighSpeed) * _ULL(1000000);
      }
   }
   else
   {
      *speed = static_cast<uint64_t>(ifHighSpeed) * _ULL(1000000);
   }

   return success;
}

/**
 * Get list of interfaces for given node
 *
 * @param snmp SNMP transport
 * @param node Node
 * @param driverData driver data
 * @param useIfXTable if true, usage of ifXTable is allowed
 */
InterfaceList *NetworkDeviceDriver::getInterfaces(SNMP_Transport *snmp, NObject *node, DriverData *driverData, bool useIfXTable)
{
   bool success = false;

   wchar_t debugId[128];
   nx_swprintf(debugId, 128, L"%s [%u]", node->getName(), node->getId());
	nxlog_debug_tag(DEBUG_TAG, 6, L"NetworkDeviceDriver::getInterfaces(%s): useIfXTable=%s, transport=%p", debugId, BooleanToString(useIfXTable), snmp);

   // Get number of interfaces
	// Some devices may not return ifNumber at all or return completely insane values
   // (for example, DLink DGS-3612 can return negative value)
   // Anyway it's just a hint for initial array size
	uint32_t interfaceCount = 0;
	SnmpGet(snmp->getSnmpVersion(), snmp, { 1, 3, 6, 1, 2, 1, 2, 1, 0 }, &interfaceCount, sizeof(uint32_t), 0);
	if ((interfaceCount <= 0) || (interfaceCount > 4096))
	{
	   nxlog_debug_tag(DEBUG_TAG, 6, _T("NetworkDeviceDriver::getInterfaces(%s): invalid interface count %d received from device"), debugId, interfaceCount);
		interfaceCount = 64;
	}

   // Create empty list
	InterfaceList *ifList = new InterfaceList(interfaceCount);

   // Gather interface indexes
   nxlog_debug_tag(DEBUG_TAG, 7, _T("NetworkDeviceDriver::getInterfaces(%s): reading indexes from ifTable"), debugId);
   uint32_t rc = SnmpWalk(snmp, { 1, 3, 6, 1, 2, 1, 2, 2, 1, 1 },
      [ifList] (SNMP_Variable *var) -> uint32_t
      {
         ifList->add(new InterfaceInfo(var->getValueAsUInt()));
         return SNMP_ERR_SUCCESS;
      });
   if (rc == SNMP_ERR_SUCCESS)
   {
      // Gather additional interfaces from ifXTable
      nxlog_debug_tag(DEBUG_TAG, 7, _T("NetworkDeviceDriver::getInterfaces(%s): reading indexes from ifXTable"), debugId);
      rc = SnmpWalk(snmp, { 1, 3, 6, 1, 2, 1, 31, 1, 1, 1, 1 }, HandlerIndexIfXTable, ifList);
      if (rc != SNMP_ERR_SUCCESS)
         nxlog_debug_tag(DEBUG_TAG, 6, _T("NetworkDeviceDriver::getInterfaces(%s): SNMP WALK 1.3.6.1.2.1.31.1.1.1.1 failed (%s)"), debugId, SnmpGetErrorText(rc));

      // Enumerate interfaces
      nxlog_debug_tag(DEBUG_TAG, 7, _T("NetworkDeviceDriver::getInterfaces(%s): reading interface information"), debugId);
		for(int i = 0; i < ifList->size(); i++)
      {
			InterfaceInfo *iface = ifList->get(i);

			// Get interface description
		   uint32_t oid[32] = { 1, 3, 6, 1, 2, 1, 2, 2, 1, 2, iface->index };
	      if (SnmpGet(snmp->getSnmpVersion(), snmp, nullptr, oid, 11, iface->description, MAX_DB_STRING * sizeof(TCHAR), 0) != SNMP_ERR_SUCCESS)
         {
            // Try to get name from ifXTable
	         static uint32_t ifXTable[] = { 1, 3, 6, 1, 2, 1, 31, 1, 1, 1, 1 };
	         memcpy(oid, ifXTable, sizeof(ifXTable));
	         oid[11] = iface->index;
	         if (SnmpGet(snmp->getSnmpVersion(), snmp, nullptr, oid, 12, iface->description, MAX_DB_STRING * sizeof(TCHAR), 0) != SNMP_ERR_SUCCESS)
	         {
	            nxlog_debug_tag(DEBUG_TAG, 6, _T("NetworkDeviceDriver::getInterfaces(%s): cannot read interface description for interface %u"), debugId, iface->index);
   	         continue;
	         }
         }

         // Interface speed
         if (!getInterfaceSpeed(snmp, iface->index, 0, nullptr, &iface->speed))
            iface->speed = 0;

         SNMP_PDU request(SNMP_GET_REQUEST, SnmpNewRequestId(), snmp->getSnmpVersion());
         request.bindVariable(new SNMP_Variable({ 1, 3, 6, 1, 2, 1, 31, 1, 1, 1, 18, iface->index }));  // ifAlias
         request.bindVariable(new SNMP_Variable({ 1, 3, 6, 1, 2, 1, 31, 1, 1, 1, 1, iface->index }));  // ifName
         request.bindVariable(new SNMP_Variable({ 1, 3, 6, 1, 2, 1, 2, 2, 1, 3, iface->index }));  // ifType
         request.bindVariable(new SNMP_Variable({ 1, 3, 6, 1, 2, 1, 2, 2, 1, 4, iface->index }));  // MTU
         request.bindVariable(new SNMP_Variable({ 1, 3, 6, 1, 2, 1, 2, 2, 1, 6, iface->index }));  // MAC address
         SNMP_PDU *response;
         if (snmp->doRequest(&request, &response) == SNMP_ERR_SUCCESS)
         {
            SNMP_Variable *var = response->getVariable(0);
            if ((var != nullptr) && (var->getType() == ASN_OCTET_STRING))
            {
               var->getValueAsString(iface->alias, MAX_DB_STRING);
               Trim(iface->alias);
            }

            // Try to get interface name from ifXTable, if unsuccessful or disabled, use ifDescr from ifTable
            var = response->getVariable(1);
            if (useIfXTable && (var != nullptr) && (var->getType() == ASN_OCTET_STRING))
            {
               var->getValueAsString(iface->name, MAX_DB_STRING);
            }
            else
            {
               wcslcpy(iface->name, iface->description, MAX_DB_STRING);
            }

            // Interface type
            var = response->getVariable(2);
            if ((var != nullptr) && var->isInteger())
            {
               iface->type = var->getValueAsUInt();
            }
            else
            {
               iface->type = IFTYPE_OTHER;
            }

            // Interface MTU
            var = response->getVariable(3);
            if ((var != nullptr) && var->isInteger())
            {
               iface->mtu = var->getValueAsUInt();
            }

            // MAC address
            var = response->getVariable(4);
            if ((var != nullptr) && (var->getType() == ASN_OCTET_STRING))
            {
               var->getRawValue(iface->macAddr, MAC_ADDR_LENGTH);
            }

            delete response;
         }
      }

      // Interface IP addresses and netmasks from ipAddrTable
		if (!node->getCustomAttributeAsBoolean(_T("snmp.ignore.ipAddrTable"), false))
		{
	      nxlog_debug_tag(DEBUG_TAG, 7, _T("NetworkDeviceDriver::getInterfaces(%s): reading ipAddrTable"), debugId);
         uint32_t error = SnmpWalk(snmp, { 1, 3, 6, 1, 2, 1, 4, 20, 1, 1 }, HandlerIpAddr, ifList);
         if (error == SNMP_ERR_SUCCESS)
         {
            success = true;
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG, 6, _T("NetworkDeviceDriver::getInterfaces(%s): SNMP WALK 1.3.6.1.2.1.4.20.1.1 failed (%s)"), debugId, SnmpGetErrorText(error));
         }
		}
		else
		{
         success = true;
		}

      // Get IP addresses from ipAddressTable if available
      if (!node->getCustomAttributeAsBoolean(_T("snmp.ignore.ipAddressTable"), false))
      {
         nxlog_debug_tag(DEBUG_TAG, 7, _T("NetworkDeviceDriver::getInterfaces(%s): reading ipAddressTable"), debugId);
         SnmpWalk(snmp, { 1, 3, 6, 1, 2, 1, 4, 34, 1, 3 }, HandlerIpAddressTable, ifList);
         if (ifList->isPrefixWalkNeeded())
         {
            nxlog_debug_tag(DEBUG_TAG, 6, _T("NetworkDeviceDriver::getInterfaces(%s): prefix length is not set for some IP addresses, walk on prefix table is needed"), debugId);
            SnmpWalk(snmp, { 1, 3, 6, 1, 2, 1, 4, 32, 1, 5 }, HandlerIpAddressPrefixTable, ifList);

            // Re-check addresses
            bool prefixNotFoundV4 = false, prefixNotFoundV6 = false;
            for(int i = 0; (i < ifList->size()) && (!prefixNotFoundV4 || !prefixNotFoundV6); i++)
            {
               InterfaceInfo *iface = ifList->get(i);
               for(int j = 0; (j < iface->ipAddrList.size()) && (!prefixNotFoundV4 || !prefixNotFoundV6); j++)
               {
                  InetAddress a = iface->ipAddrList.get(j);
                  if (a.getMaskBits() == 0)
                  {
                     if (a.getFamily() == AF_INET)
                        prefixNotFoundV4 = true;
                     else
                        prefixNotFoundV6 = true;
                  }
               }
            }

            if (prefixNotFoundV4 || prefixNotFoundV6)
            {
               nxlog_debug_tag(DEBUG_TAG, 6, _T("NetworkDeviceDriver::getInterfaces(%s): doing prefix lookup in routing table (IPv4=%s IPv6=%s)"),
                  debugId, BooleanToString(prefixNotFoundV4), BooleanToString(prefixNotFoundV6));
               int limit = 1000;
               if (prefixNotFoundV4)
               {
                  SnmpWalk(snmp, { 1, 3, 6, 1, 2, 1, 4, 24, 7, 1, 8, 1 },
                     [&limit, ifList, snmp, &debugId] (SNMP_Variable *v) -> uint32_t
                     {
                        ProcessInetCidrRouteingTableEntry(v, snmp, ifList, debugId);
                        return (--limit == 0) ? SNMP_ERR_ABORTED : SNMP_ERR_SUCCESS;
                     });
                  if (limit == 0)
                     nxlog_debug_tag(DEBUG_TAG, 6, _T("NetworkDeviceDriver::getInterfaces(%s): prefix lookup aborted because routing table is too big"), debugId);
               }
               if (prefixNotFoundV6 && (limit > 0))
               {
                  limit = 1000;
                  SnmpWalk(snmp, { 1, 3, 6, 1, 2, 1, 4, 24, 7, 1, 8, 2 },
                     [&limit, ifList, snmp, &debugId] (SNMP_Variable *v) -> uint32_t
                     {
                        ProcessInetCidrRouteingTableEntry(v, snmp, ifList, debugId);
                        return (--limit == 0) ? SNMP_ERR_ABORTED : SNMP_ERR_SUCCESS;
                     });
                  if (limit == 0)
                     nxlog_debug_tag(DEBUG_TAG, 6, _T("NetworkDeviceDriver::getInterfaces(%s): prefix lookup aborted because routing table is too big"), debugId);
               }
            }
            else
            {
               nxlog_debug_tag(DEBUG_TAG, 6, _T("NetworkDeviceDriver::getInterfaces(%s): prefix lookup in routing table is not needed"), debugId);
            }
         }
      }
   }
	else
	{
		nxlog_debug_tag(DEBUG_TAG, 6, _T("NetworkDeviceDriver::getInterfaces(%s): SNMP WALK 1.3.6.1.2.1.2.2.1.1 failed (%s)"), debugId, SnmpGetErrorText(rc));
	}

   if (!success)
   {
      delete_and_null(ifList);
   }

   nxlog_debug_tag(DEBUG_TAG, 6, _T("NetworkDeviceDriver::getInterfaces(%s): completed, ifList=%p"), debugId, ifList);
   return ifList;
}

/**
 * Get interface state. Both states must be set to UNKNOWN if cannot be read from device. This method should not change value pointed by
 * "speed" if interface speed cannot be retrieved.
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
   uint32_t oid[128] = { 1, 3, 6, 1, 2, 1, 2, 2, 1, 7 };
   size_t oidLen;
   if (ifTableSuffixLen > 0)
   {
      memcpy(&oid[10], ifTableSuffix, ifTableSuffixLen * sizeof(uint32_t));
      oidLen = ifTableSuffixLen + 10;
   }
   else
   {
      oidLen = 11;
      oid[10] = ifIndex;
   }

   SNMP_PDU request(SNMP_GET_REQUEST, SnmpNewRequestId(), snmp->getSnmpVersion());
   request.bindVariable(new SNMP_Variable(oid, oidLen));

   oid[9] = 8; // oper state
   request.bindVariable(new SNMP_Variable(oid, oidLen));

   SNMP_PDU *response;
   if (snmp->doRequest(&request, &response) == SNMP_ERR_SUCCESS)
   {
      int32_t state = (response->getNumVariables() > 0) ? response->getVariable(0)->getValueAsInt() : 0;
      switch(state)
      {
         case 2:
            *adminState = IF_ADMIN_STATE_DOWN;
            *operState = IF_OPER_STATE_DOWN;
            break;
         case 1:
         case 3:
            *adminState = static_cast<InterfaceAdminState>(state);
            // Check interface operational state
            state = (response->getNumVariables() > 1) ? response->getVariable(1)->getValueAsInt() : 0;
            switch(state)
            {
               case 1:
                  *operState = IF_OPER_STATE_UP;
                  getInterfaceSpeed(snmp, ifIndex, ifTableSuffixLen, ifTableSuffix, speed);
                  break;
               case 2:  // down: interface is down
               case 7:  // lowerLayerDown: down due to state of lower-layer interface(s)
                  *operState = IF_OPER_STATE_DOWN;
                  break;
               case 3:
                  *operState = IF_OPER_STATE_TESTING;
                  getInterfaceSpeed(snmp, ifIndex, ifTableSuffixLen, ifTableSuffix, speed);
                  break;
               case 5:
                  *operState = IF_OPER_STATE_DORMANT;
                  getInterfaceSpeed(snmp, ifIndex, ifTableSuffixLen, ifTableSuffix, speed);
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
      delete response;
   }
   else
   {
      *adminState = IF_ADMIN_STATE_UNKNOWN;
      *operState = IF_OPER_STATE_UNKNOWN;
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
 * Default implementation always return false.
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
 * Returns true if lldpRemLocalPortNum is expected to be valid interface index or bridge port number.
 * Default implementation always return true.
 *
 * @param node Node
 * @param driverData driver-specific data previously created in analyzeDevice
 * @return true if lldpRemLocalPortNum is expected to be valid
 */
bool NetworkDeviceDriver::isValidLldpRemLocalPortNum(const NObject *node, DriverData *driverData)
{
   return true;
}

/**
 * Handler for VLAN egress port enumeration
 */
static void ProcessVlanPortRecord(SNMP_Variable *var, VlanList *vlanList, bool tagged)
{
   uint32_t vlanId = var->getName().getLastElement();
	VlanInfo *vlan = vlanList->findById(vlanId);
	if (vlan != nullptr)
	{
		BYTE buffer[4096];
		size_t size = var->getRawValue(buffer, 4096);
		for(int i = 0; i < (int)size; i++)
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
		   uint32_t port = i * 8 + 1;
		   BYTE map = buffer[i];
		   BYTE mask = 0x80;
		   while(mask > 0)
		   {
		      if (map & mask)
		      {
		         vlan->add(port, tagged);
		      }
		      mask >>= 1;
		      port++;
		   }
		}
	}
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
	VlanList *vlanList = new VlanList();

   // dot1qVlanStaticName
	if (SnmpWalk(snmp, { 1, 3, 6, 1, 2, 1, 17, 7, 1, 4, 3, 1, 1 },
	      [vlanList] (SNMP_Variable *var) -> uint32_t
	      {
	         TCHAR buffer[256];
	         VlanInfo *vlan = new VlanInfo(var->getName().getLastElement(), VLAN_PRM_BPORT, var->getValueAsString(buffer, 256));
	         vlanList->add(vlan);
	         return SNMP_ERR_SUCCESS;
	      }) != SNMP_ERR_SUCCESS)
		goto failure;

   // dot1qVlanCurrentUntaggedPorts
   if (SnmpWalk(snmp, { 1, 3, 6, 1, 2, 1, 17, 7, 1, 4, 2, 1, 5 },
         [vlanList] (SNMP_Variable *var) -> uint32_t
         {
            ProcessVlanPortRecord(var, vlanList, false);
            return SNMP_ERR_SUCCESS;
         }) != SNMP_ERR_SUCCESS)
      goto failure;

   // dot1qVlanCurrentEgressPorts
   if (SnmpWalk(snmp, { 1, 3, 6, 1, 2, 1, 17, 7, 1, 4, 2, 1, 4 },
         [vlanList] (SNMP_Variable *var) -> uint32_t
         {
            ProcessVlanPortRecord(var, vlanList, true);
            vlanList->setData(vlanList);  // to indicate that callback was called
            return SNMP_ERR_SUCCESS;
         }) != SNMP_ERR_SUCCESS)
      goto failure;

   if (vlanList->getData() == nullptr)
   {
      // Some devices does not return anything under dot1qVlanCurrentEgressPorts.
      // In that case we use dot1qVlanStaticEgressPorts
      if (SnmpWalk(snmp, { 1, 3, 6, 1, 2, 1, 17, 7, 1, 4, 3, 1, 2 },
            [vlanList] (SNMP_Variable *var) -> uint32_t
            {
               ProcessVlanPortRecord(var, vlanList, false);
               return SNMP_ERR_SUCCESS;
            }) != SNMP_ERR_SUCCESS)
		   goto failure;
   }

   return vlanList;

failure:
   delete vlanList;
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
 * Get mapping between bridge ports and interfaces.
 *
 * @param snmp SNMP transport
 * @param node Node
 * @param driverData driver-specific data previously created in analyzeDevice
 * @return bridge port to interface mapping or NULL on failure
 */
StructArray<BridgePort> *NetworkDeviceDriver::getBridgePorts(SNMP_Transport *snmp, NObject *node, DriverData *driverData)
{
   auto bports = new StructArray<BridgePort>(0, 64);
   if (SnmpWalk(snmp, { 1, 3, 6, 1, 2, 1, 17, 1, 4, 1, 2 },
      [bports] (SNMP_Variable *var) -> uint32_t
      {
         BridgePort *bp = bports->addPlaceholder();
         bp->portNumber = var->getName().getElement(11);
         bp->ifIndex = var->getValueAsUInt();
         return SNMP_ERR_SUCCESS;
      }) != SNMP_ERR_SUCCESS)
   {
      delete_and_null(bports);
   }
   return bports;
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
 * FDB walker's callback
 */
static uint32_t FDBHandler(SNMP_Variable *var, SNMP_Transport *snmp, StructArray<ForwardingDatabaseEntry> *fdb)
{
   SNMP_ObjectId oid(var->getName());

   // Get port number and status
   SNMP_PDU request(SNMP_GET_REQUEST, SnmpNewRequestId(), snmp->getSnmpVersion());

   oid.changeElement(10, 2);  // 1.3.6.1.2.1.17.4.3.1.2 - port number
   request.bindVariable(new SNMP_Variable(oid));

   oid.changeElement(10, 3);  // 1.3.6.1.2.1.17.4.3.1.3 - status
   request.bindVariable(new SNMP_Variable(oid));

   SNMP_PDU *response;
   uint32_t rcc = snmp->doRequest(&request, &response);
   if (rcc == SNMP_ERR_SUCCESS)
   {
      SNMP_Variable *varPort = response->getVariable(0);
      SNMP_Variable *varStatus = response->getVariable(1);
      if (varPort != nullptr && varStatus != nullptr)
      {
         uint32_t port = varPort->getValueAsUInt();
         int status = varStatus->getValueAsInt();
         if ((port > 0) && ((status == 3) || (status == 5) || (status == 6)))  // status: 3 == learned, 5 == static, 6 == secure (possibly H3C specific)
         {
            ForwardingDatabaseEntry *entry = fdb->addPlaceholder();
            memset(entry, 0, sizeof(ForwardingDatabaseEntry));
            entry->bridgePort = port;
            entry->macAddr = var->getValueAsMACAddr();
            entry->vlanId = 1;
            entry->type = static_cast<uint16_t>(status);
         }
      }
      delete response;
   }

   return rcc;
}

/**
 * dot1qTpFdbEntry walker's callback
 */
static uint32_t Dot1qTpFdbHandler(SNMP_Variable *var, SNMP_Transport *snmp, StructArray<ForwardingDatabaseEntry> *fdb)
{
   uint32_t port = var->getValueAsUInt();
   if (port == 0)
      return SNMP_ERR_SUCCESS;

   // Get port number and status
   SNMP_PDU request(SNMP_GET_REQUEST, SnmpNewRequestId(), snmp->getSnmpVersion());

   SNMP_ObjectId oid(var->getName());
   oid.changeElement(12, 3);  // 1.3.6.1.2.1.17.7.1.2.2.1.3 - status
   request.bindVariable(new SNMP_Variable(oid));

   SNMP_PDU *response;
   uint32_t rcc = snmp->doRequest(&request, &response);
   if (rcc == SNMP_ERR_SUCCESS)
   {
      int status = response->getVariable(0)->getValueAsInt();
      if ((status == 3) || (status == 5) || (status == 6)) // status: 3 == learned, 5 == static, 6 == secure (possibly H3C specific)
      {
         ForwardingDatabaseEntry *entry = fdb->addPlaceholder();
         memset(entry, 0, sizeof(ForwardingDatabaseEntry));
         entry->bridgePort = port;
         size_t oidLen = oid.length();
         BYTE macAddrBytes[MAC_ADDR_LENGTH];
         for(size_t i = oidLen - MAC_ADDR_LENGTH, j = 0; i < oidLen; i++)
            macAddrBytes[j++] = oid.getElement(i);
         entry->macAddr = MacAddress(macAddrBytes, MAC_ADDR_LENGTH);
         entry->vlanId = static_cast<uint16_t>(oid.getElement(oidLen - MAC_ADDR_LENGTH - 1));
         entry->type = static_cast<uint16_t>(status);
      }
      delete response;
   }

   return rcc;
}

/**
 * Get switch forwarding database.
 *
 * @param snmp SNMP transport
 * @param node Node
 * @param driverData driver-specific data previously created in analyzeDevice
 * @return switch forwarding database or NULL on failure
 */
StructArray<ForwardingDatabaseEntry> *NetworkDeviceDriver::getForwardingDatabase(SNMP_Transport *snmp, NObject *node, DriverData *driverData)
{
   auto fdb = new StructArray<ForwardingDatabaseEntry>(64, 64);

   if (SnmpWalk(snmp, { 1, 3, 6, 1, 2, 1, 17, 7, 1, 2, 2, 1, 2 },
      [snmp, fdb] (SNMP_Variable *var) -> uint32_t
      {
         return Dot1qTpFdbHandler(var, snmp, fdb);
      }) != SNMP_ERR_SUCCESS)
   {
      delete fdb;
      return nullptr;
   }

   int size = fdb->size();
   nxlog_debug_tag(DEBUG_TAG_TOPO_FDB, 5, _T("NetworkDeviceDriver::getForwardingDatabase(%s [%u]): %d entries read from dot1qTpFdbTable"), node->getName(), node->getId(), size);

   if (SnmpWalk(snmp, { 1, 3, 6, 1, 2, 1, 17, 4, 3, 1, 1 },
      [snmp, fdb] (SNMP_Variable *var) -> uint32_t
      {
         return FDBHandler(var, snmp, fdb);
      }) != SNMP_ERR_SUCCESS)
   {
      delete fdb;
      return nullptr;
   }

   if (isFdbUsingIfIndex(node, driverData))
   {
      for(int i = 0; i < fdb->size(); i++)
      {
         ForwardingDatabaseEntry *e = fdb->get(i);
         e->ifIndex = e->bridgePort;
      }
   }

   nxlog_debug_tag(DEBUG_TAG_TOPO_FDB, 5, _T("NetworkDeviceDriver::getForwardingDatabase(%s [%u]): %d entries read from dot1dTpFdbTable"), node->getName(), node->getId(), fdb->size() - size);
   return fdb;
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
 * Returns true if device is a standalone wireless access point (not managed by a controller). Default implementation always return false.
 *
 * @param snmp SNMP transport
 * @param node Node
 * @param driverData driver-specific data previously created in analyzeDevice
 * @return true if device is a standalone wireless access point
 */
bool NetworkDeviceDriver::isWirelessAccessPoint(SNMP_Transport *snmp, NObject *node, DriverData *driverData)
{
   return false;
}

/**
 * Get list of radio interfaces for standalone access point. Default implementation always return NULL.
 *
 * @param snmp SNMP transport
 * @param node Node
 * @param driverData driver-specific data previously created in analyzeDevice
 * @return list of radio interfaces for standalone access point
 */
StructArray<RadioInterfaceInfo> *NetworkDeviceDriver::getRadioInterfaces(SNMP_Transport *snmp, NObject *node, DriverData *driverData)
{
   return nullptr;
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
   return nullptr;
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
   return nullptr;
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
      uint32_t apIndex, const MacAddress& macAddr, const InetAddress& ipAddr, const StructArray<RadioInterfaceInfo>& radioInterfaces)
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
DataCollectionError NetworkDeviceDriver::getMetric(SNMP_Transport *snmp, NObject *node, DriverData *driverData, const wchar_t *name, wchar_t *value, size_t size)
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
   if (SnmpWalk(snmp, { 1, 3, 6, 1, 2, 1, 4, 22, 1, 2 }, HandlerArp, arpCache.get()) != SNMP_ERR_SUCCESS)
      arpCache.reset();
   return arpCache;
}

/**
 * Get link layer neighbors.
 *
 * @param snmp SNMP transport
 * @param driverData driver-specific data previously created in analyzeDevice
 * @param ignoreStandardMibs driver can set referenced variable to true to indicate that server should not attempt to read data from standard MIBs (LLDP, CDP, NDP, STP)
 * @return known link layer neighbors or NULL if none known or functionality is not supported
 */
ObjectArray<LinkLayerNeighborInfo> *NetworkDeviceDriver::getLinkLayerNeighbors(SNMP_Transport *snmp, DriverData *driverData, bool *ignoreStandardMibs)
{
   return nullptr;
}
