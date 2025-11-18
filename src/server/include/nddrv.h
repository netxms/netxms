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
** File: nddrv.h
**
**/

#ifndef _nddrv_h_
#define _nddrv_h_

#include <nms_common.h>
#include <nxsrvapi.h>

/**
 * API version
 */
#define NDDRV_API_VERSION           12

/**
 * Begin driver list
 */
#define NDD_BEGIN_DRIVER_LIST static ObjectArray<NetworkDeviceDriver> *s_createInstances() { \
   ObjectArray<NetworkDeviceDriver> *drivers = new ObjectArray<NetworkDeviceDriver>(4, 4, Ownership::False);

/**
 * Declare driver within list
 */
#define NDD_DRIVER(implClass) \
   drivers->add(new implClass);

/**
 * End driver list
 */
#define NDD_END_DRIVER_LIST \
   return drivers; \
}

/**
 * NDD module entry point
 */
#define DECLARE_NDD_MODULE_ENTRY_POINT \
extern "C" __EXPORT_VAR(int nddAPIVersion); \
__EXPORT_VAR(int nddAPIVersion) = NDDRV_API_VERSION; \
extern "C" __EXPORT ObjectArray<NetworkDeviceDriver> *nddCreateInstances() { return s_createInstances(); }

/**
 * NDD module entry point - single driver
 */
#define DECLARE_NDD_ENTRY_POINT(implClass) \
extern "C" __EXPORT_VAR(int nddAPIVersion); \
__EXPORT_VAR(int nddAPIVersion) = NDDRV_API_VERSION; \
extern "C" __EXPORT ObjectArray<NetworkDeviceDriver> *nddCreateInstances() { \
   ObjectArray<NetworkDeviceDriver> *drivers = new ObjectArray<NetworkDeviceDriver>(4, 4, Ownership::False); \
   drivers->add(new implClass); \
   return drivers; \
}

/**
 * Port numbering schemes
 */
enum PortNumberingScheme
{
	NDD_PN_UNKNOWN = 0, // port layout not known to driver
	NDD_PN_CUSTOM = 1,  // custom layout, driver defines location of each port
	NDD_PN_LR_UD = 2,   // left-to-right, then up-down:
	                    //  1 2 3 4
	                    //  5 6 7 8
	NDD_PN_LR_DU = 3,   // left-to-right, then down-up:
	                    //  5 6 7 8
	                    //  1 2 3 4
	NDD_PN_UD_LR = 4,   // up-down, then left-right:
	                    //  1 3 5 7
	                    //  2 4 6 8
	NDD_PN_DU_LR = 5    // down-up, then left-right:
	                    //  2 4 6 8
	                    //  1 3 5 7
};

/**
 * Modules orientation on the switch
 */
enum ModuleOrientation
{
	NDD_ORIENTATION_HORIZONTAL = 0,
	NDD_ORIENTATION_VERTICAL = 1
};

/**
 * Cluster modes
 */
enum
{
   CLUSTER_MODE_UNKNOWN = -1,
   CLUSTER_MODE_STANDALONE = 0,
   CLUSTER_MODE_ACTIVE = 1,
   CLUSTER_MODE_STANDBY = 2
};

/**
 * Access point state
 */
enum AccessPointState
{
   AP_UP = 0,
   AP_UNPROVISIONED = 1,
   AP_DOWN = 2,
   AP_UNKNOWN = 3
};

/**
 * Radio band
 */
enum RadioBand
{
   RADIO_BAND_UNKNOWN = 0,
   RADIO_BAND_2_4_GHZ = 1,
   RADIO_BAND_3_65_GHZ = 2,
   RADIO_BAND_5_GHZ = 3,
   RADIO_BAND_6_GHZ = 4
};

/**
 * Radio band display name
 */
const TCHAR LIBNXSRV_EXPORTABLE *RadioBandDisplayName(RadioBand band);

/**
 * Module layout definition
 */
struct NDD_MODULE_LAYOUT
{
	int rows;					    // number of port rows on the module
	int numberingScheme;		    // port numbering scheme
	int columns;                // number of columns for custom layout
	uint16_t portRows[256];     // row numbers for ports
	uint16_t portColumns[256];  // column numbers for ports
};

/**
 * Device hardware information
 */
struct DeviceHardwareInfo
{
   wchar_t vendor[128];
   wchar_t productCode[32];
   wchar_t productName[128];
   wchar_t productVersion[16];
   wchar_t serialNumber[32];
};

/**
 * Device view element flags
 */
#define DVF_BACKGROUND  0x0001
#define DVF_BORDER      0x0002

/**
 * Image to be used by device view
 */
struct DeviceViewImage
{
   const wchar_t *name;
   size_t size;
   const BYTE *data;
};

/**
 * Device view element
 */
struct DeviceViewElement
{
   int x;
   int y;
   int width;
   int height;
   int flags;
   Color backgroundColor;
   Color borderColor;
   const TCHAR *imageName; // driver image name, can be null
   const TCHAR *commands;  // drawing commands, can be null
};

#ifdef _WIN32
template class LIBNXSRV_TEMPLATE_EXPORTABLE StructArray<DeviceViewElement>;
template class LIBNXSRV_TEMPLATE_EXPORTABLE StructArray<DeviceViewImage>;
#endif

/**
 * Device view
 */
class LIBNXSRV_EXPORTABLE DeviceView
{
private:
   StructArray<DeviceViewElement> m_elements;
   StructArray<DeviceViewImage> m_images;
   time_t m_timestamp;

public:
   DeviceView() : m_elements(0, 16), m_images(0, 8)
   {
      m_timestamp = time(nullptr);
   }
   DeviceView(const StructArray<DeviceViewElement>& elements, const StructArray<DeviceViewImage> images) : m_elements(elements), m_images(images)
   {
      m_timestamp = time(nullptr);
   }

   void fillMessage(NXCPMessage *msg) const;
   time_t getTimestamp() const { return m_timestamp; }
};

/**
 * Interface identification type
 */
enum class InterfaceIdType
{
   INDEX = 0,
   NAME = 1
};

/**
 * Interface identification
 */
struct InterfaceId
{
   InterfaceIdType type;
   union
   {
      uint32_t ifIndex;
      TCHAR ifName[192];
   } value;
};

/**
 * Max SSID length
 */
#define MAX_SSID_LENGTH 33

/**
 * Radio interface information
 */
struct LIBNXSRV_EXPORTABLE RadioInterfaceInfo
{
	uint32_t index;
	uint32_t ifIndex; // Index of corresponding element from ifTable, if available
	TCHAR name[MAX_OBJECT_NAME];
   BYTE bssid[MAC_ADDR_LENGTH];
   TCHAR ssid[MAX_SSID_LENGTH];
   RadioBand band;
   uint16_t frequency;  // MHz
   uint16_t channel;
   int32_t powerDBm;
   int32_t powerMW;

   json_t *toJson() const;
};

#ifdef _WIN32
template class LIBNXSRV_TEMPLATE_EXPORTABLE StructArray<RadioInterfaceInfo>;
#endif

/**
 * Wireless access point information
 */
class LIBNXSRV_EXPORTABLE AccessPointInfo
{
private:
   uint32_t m_index;
   MacAddress m_macAddr;
   InetAddress m_ipAddr;
   AccessPointState m_state;
   TCHAR *m_name;
   TCHAR *m_vendor;
   TCHAR *m_model;
   TCHAR *m_serial;
	StructArray<RadioInterfaceInfo> m_radioInterfaces;
	uint32_t m_controllerId;   // Used by the server

public:
   AccessPointInfo(uint32_t index, const MacAddress& macAddr, const InetAddress& ipAddr, AccessPointState state,
            const TCHAR *name, const TCHAR *vendor, const TCHAR *model, const TCHAR *serial);
   ~AccessPointInfo();

	void addRadioInterface(const RadioInterfaceInfo& iface) { m_radioInterfaces.add(iface); }
	void setMacAddr(const MacAddress& macAddr) { m_macAddr = macAddr; }

	uint32_t getIndex() const { return m_index; }
	const MacAddress& getMacAddr() const { return m_macAddr; }
   const InetAddress& getIpAddr() const { return m_ipAddr; }
	AccessPointState getState() const { return m_state; }
	const TCHAR *getName() const { return m_name; }
	const TCHAR *getVendor() const { return m_vendor; }
	const TCHAR *getModel() const { return m_model; }
	const TCHAR *getSerial() const { return m_serial; }
	const StructArray<RadioInterfaceInfo>& getRadioInterfaces() const { return m_radioInterfaces; }

	uint32_t getControllerId() const { return m_controllerId; }
	void setControllerId(uint32_t controllerId) { m_controllerId = controllerId; }
};

/**
 * Wireless station AP match policy
 */
#define AP_MATCH_BY_RFINDEX   0
#define AP_MATCH_BY_BSSID     1
#define AP_MATCH_BY_SERIAL    2

/**
 * Wireless station information
 */
struct WirelessStationInfo
{
	// This part filled by driver
   BYTE macAddr[MAC_ADDR_LENGTH];
	InetAddress ipAddr;
	uint32_t rfIndex;	// radio interface index
   int16_t apMatchPolicy;
   BYTE bssid[MAC_ADDR_LENGTH];
   TCHAR ssid[MAX_SSID_LENGTH];
   TCHAR apSerial[24];
   int32_t vlan;
   int32_t rssi;
   uint32_t txRate;
   uint32_t rxRate;

	// This part filled by core
   uint32_t apObjectId;
   uint32_t nodeId;
   TCHAR rfName[MAX_OBJECT_NAME];

   // Fill NXCP message
   void fillMessage(NXCPMessage *msg, uint32_t baseId)
   {
      uint32_t fieldId = baseId;
      msg->setField(fieldId++, macAddr, MAC_ADDR_LENGTH);
      msg->setField(fieldId++, ipAddr);
      msg->setField(fieldId++, ssid);
      msg->setField(fieldId++, static_cast<uint16_t>(vlan));
      msg->setField(fieldId++, apObjectId);
      msg->setField(fieldId++, rfIndex);
      msg->setField(fieldId++, rfName);
      msg->setField(fieldId++, nodeId);
      msg->setField(fieldId++, rssi);
   }
};

/**
 * Bridge port number mapping to interface index
 */
struct BridgePort
{
   uint32_t portNumber;
   uint32_t ifIndex;
};

#ifdef _WIN32
template class LIBNXSRV_TEMPLATE_EXPORTABLE StructArray<BridgePort>;
#endif

/**
 * FDB entry
 */
struct ForwardingDatabaseEntry
{
   uint32_t bridgePort; // Bridge port number
   uint32_t ifIndex;    // Interface index (filled by server core if bridgePort is set)
   MacAddress macAddr;  // MAC address
   uint32_t nodeObject; // ID of node object or 0 if not found (filled by server core)
   uint16_t vlanId;     // VLAN ID
   uint16_t type;       // Entry type
};

#ifdef _WIN32
template class LIBNXSRV_TEMPLATE_EXPORTABLE StructArray<ForwardingDatabaseEntry>;
#endif

/**
 * Base class for driver data
 */
class LIBNXSRV_EXPORTABLE DriverData
{
protected:
   uint32_t m_nodeId;
   uuid m_nodeGuid;
   TCHAR m_nodeName[MAX_OBJECT_NAME];

public:
   DriverData();
   virtual ~DriverData();

   void attachToNode(uint32_t nodeId, const uuid& nodeGuid, const TCHAR *nodeName);

   uint32_t getNodeId() const { return m_nodeId; }
   const uuid& getNodeGuid() const { return m_nodeGuid; }
   const TCHAR *getNodeName() const { return m_nodeName; }
};

/**
 * Storage type
 */
enum HostMibStorageType
{
   hrStorageCompactDisc = 7,
   hrStorageFixedDisk = 4,
   hrStorageFlashMemory = 9,
   hrStorageFloppyDisk = 6,
   hrStorageNetworkDisk = 10,
   hrStorageOther = 1,
   hrStorageRam = 2,
   hrStorageRamDisk = 8,
   hrStorageRemovableDisk = 5,
   hrStorageVirtualMemory = 3
};

/**
 * Storage entry
 */
struct LIBNXSRV_EXPORTABLE HostMibStorageEntry
{
   TCHAR name[128];
   uint32_t unitSize;
   uint32_t size;
   uint32_t used;
   HostMibStorageType type;
   uint32_t oid[12];
   time_t lastUpdate;

   void getFree(TCHAR *buffer, size_t len) const;
   void getFreePerc(TCHAR *buffer, size_t len) const;
   void getTotal(TCHAR *buffer, size_t len) const;
   void getUsed(TCHAR *buffer, size_t len) const;
   void getUsedPerc(TCHAR *buffer, size_t len) const;
   bool getMetric(const TCHAR *metric, TCHAR *buffer, size_t len) const;
};

#ifdef _WIN32
template class LIBNXSRV_TEMPLATE_EXPORTABLE ObjectArray<HostMibStorageEntry>;
#endif

/**
 * Host MIB support for drivers
 */
class LIBNXSRV_EXPORTABLE HostMibDriverData : public DriverData
{
protected:
   ObjectArray<HostMibStorageEntry> m_storage;
   time_t m_storageCacheTimestamp;
   Mutex m_storageCacheMutex;

   uint32_t updateStorageCacheCallback(SNMP_Variable *v, SNMP_Transport *snmp);

public:
   HostMibDriverData();
   virtual ~HostMibDriverData();

   void updateStorageCache(SNMP_Transport *snmp);
   const HostMibStorageEntry *getStorageEntry(SNMP_Transport *snmp, const TCHAR *name, HostMibStorageType type);
   const HostMibStorageEntry *getPhysicalMemory(SNMP_Transport *snmp) { return getStorageEntry(snmp, nullptr, hrStorageRam); }
};

/**
 * Device capabilities
 */
enum class DeviceCapability
{
   PER_VLAN_FDB,
   BRIDGE_PORT_NUMBERS_IN_FDB
};

/**
 * Link layer discovery protocols
 */
enum LinkLayerProtocol
{
   LL_PROTO_UNKNOWN = 0,    /* unknown source */
   LL_PROTO_FDB     = 1,    /* obtained from switch forwarding database */
   LL_PROTO_CDP     = 2,    /* Cisco Discovery Protocol */
   LL_PROTO_LLDP    = 3,    /* Link Layer Discovery Protocol */
   LL_PROTO_NDP     = 4,    /* Nortel Discovery Protocol */
   LL_PROTO_EDP     = 5,    /* Extreme Discovery Protocol */
   LL_PROTO_STP     = 6,    /* Spanning Tree Protocol */
   LL_PROTO_OTHER   = 7,    /* Other (proprietary) protocol, information provided by driver */
   LL_PROTO_MANUAL  = 8     /* Information provided manually */
};

/**
 * Link layer neighbor information
 */
struct LinkLayerNeighborInfo
{
   uint32_t ifLocal;           // Local interface index
   InterfaceId ifRemote;       // Remote interface identification
   wchar_t remoteName[192];    // sysName or DNS host name of connected object
   InetAddress remoteIP;       // IP address of connected object
   MacAddress remoteMAC;       // MAC address of connected object
   bool isPtToPt;              // true if this is point-to-point link
   LinkLayerProtocol protocol; // Discovery protocol

   LinkLayerNeighborInfo()
   {
      ifLocal = 0;
      memset(&ifRemote, 0, sizeof(InterfaceId));
      remoteName[0] = 0;
      isPtToPt = false;
      protocol = LL_PROTO_OTHER;
   }
};

class Component;
class ComponentTree;

/**
 * Base class for device drivers
 */
class LIBNXSRV_EXPORTABLE NetworkDeviceDriver
{
protected:
   bool getInterfaceSpeed(SNMP_Transport *snmp, uint32_t ifIndex, int ifTableSuffixLen, const uint32_t *ifTableSuffix, uint64_t *speed);
   void registerHostMibMetrics(ObjectArray<AgentParameterDefinition> *metrics);
   DataCollectionError getHostMibMetric(SNMP_Transport *snmp, HostMibDriverData *driverData, const TCHAR *name, TCHAR *value, size_t size);

public:
   NetworkDeviceDriver();
   virtual ~NetworkDeviceDriver();

   virtual const wchar_t *getName();
   virtual const wchar_t *getVersion();

   virtual const wchar_t *getCustomTestOID();
   virtual int isPotentialDevice(const SNMP_ObjectId& oid);
   virtual bool isDeviceSupported(SNMP_Transport *snmp, const SNMP_ObjectId& oid);
   virtual void analyzeDevice(SNMP_Transport *snmp, const SNMP_ObjectId& oid, NObject *node, DriverData **driverData);
   virtual bool getHardwareInformation(SNMP_Transport *snmp, NObject *node, DriverData *driverData, DeviceHardwareInfo *hwInfo);
   virtual bool isEntityMibEmulationSupported(SNMP_Transport *snmp, NObject *node, DriverData *driverData);
   virtual shared_ptr<ComponentTree> buildComponentTree(SNMP_Transport *snmp, NObject *node, DriverData *driverData);
   virtual shared_ptr<DeviceView> buildDeviceView(SNMP_Transport *snmp, NObject *node, DriverData *driverData, const Component *rootComponent);
   virtual int getModulesOrientation(SNMP_Transport *snmp, NObject *node, DriverData *driverData);
   virtual void getModuleLayout(SNMP_Transport *snmp, NObject *node, DriverData *driverData, int module, NDD_MODULE_LAYOUT *layout);
   virtual bool getVirtualizationType(SNMP_Transport *snmp, NObject *node, DriverData *driverData, VirtualizationType *vtype);
   virtual GeoLocation getGeoLocation(SNMP_Transport *snmp, NObject *node, DriverData *driverData);
   virtual InterfaceList *getInterfaces(SNMP_Transport *snmp, NObject *node, DriverData *driverData, bool useIfXTable);
   virtual void getInterfaceState(SNMP_Transport *snmp, NObject *node, DriverData *driverData, uint32_t ifIndex, const wchar_t *ifName,
            uint32_t ifType, int ifTableSuffixLen, const uint32_t *ifTableSuffix, InterfaceAdminState *adminState, InterfaceOperState *operState, uint64_t *speed);
   virtual bool lldpNameToInterfaceId(SNMP_Transport *snmp, NObject *node, DriverData *driverData, const wchar_t *lldpName, InterfaceId *id);
   virtual bool isLldpRemTableUsingIfIndex(const NObject *node, DriverData *driverData);
   virtual bool isValidLldpRemLocalPortNum(const NObject *node, DriverData *driverData);
   virtual StructArray<BridgePort> *getBridgePorts(SNMP_Transport *snmp, NObject *node, DriverData *driverData);
   virtual bool isFdbUsingIfIndex(const NObject *node, DriverData *driverData);
   virtual StructArray<ForwardingDatabaseEntry> *getForwardingDatabase(SNMP_Transport *snmp, NObject *node, DriverData *driverData);
   virtual VlanList *getVlans(SNMP_Transport *snmp, NObject *node, DriverData *driverData);
   virtual int getClusterMode(SNMP_Transport *snmp, NObject *node, DriverData *driverData);
   virtual bool isWirelessAccessPoint(SNMP_Transport *snmp, NObject *node, DriverData *driverData);
   virtual StructArray<RadioInterfaceInfo> *getRadioInterfaces(SNMP_Transport *snmp, NObject *node, DriverData *driverData);
   virtual bool isWirelessController(SNMP_Transport *snmp, NObject *node, DriverData *driverData);
   virtual ObjectArray<AccessPointInfo> *getAccessPoints(SNMP_Transport *snmp, NObject *node, DriverData *driverData);
   virtual ObjectArray<WirelessStationInfo> *getWirelessStations(SNMP_Transport *snmp, NObject *node, DriverData *driverData);
   virtual AccessPointState getAccessPointState(SNMP_Transport *snmp, NObject *node, DriverData *driverData,
         uint32_t apIndex, const MacAddress& macAddr, const InetAddress& ipAddr, const StructArray<RadioInterfaceInfo>& radioInterfaces);
   virtual bool hasMetrics();
   virtual DataCollectionError getMetric(SNMP_Transport *snmp, NObject *node, DriverData *driverData, const wchar_t *name, wchar_t *value, size_t size);
   virtual ObjectArray<AgentParameterDefinition> *getAvailableMetrics(SNMP_Transport *snmp, NObject *node, DriverData *driverData);
   virtual shared_ptr<ArpCache> getArpCache(SNMP_Transport *snmp, DriverData *driverData);
   virtual ObjectArray<LinkLayerNeighborInfo> *getLinkLayerNeighbors(SNMP_Transport *snmp, DriverData *driverData, bool *ignoreStandardMibs);
};

/**
 * Wireless controller bridge interface
 */
struct WirelessControllerBridge
{
   ObjectArray<AccessPointInfo> *(*getAccessPoints)(NObject *wirelessDomain);
   ObjectArray<WirelessStationInfo> *(*getWirelessStations)(NObject *wirelessDomain);
   AccessPointState (*getAccessPointState)(NObject *wirelessDomain, uint32_t apIndex, const MacAddress& macAddr, const InetAddress& ipAddr, const TCHAR *serial, const StructArray<RadioInterfaceInfo>& radioInterfaces);
   DataCollectionError (*getAccessPointMetric)(NObject *wirelessDomain, uint32_t apIndex, const MacAddress& macAddr, const InetAddress& ipAddr, const TCHAR *serial, const TCHAR *name, TCHAR *value, size_t size);
   ObjectArray<WirelessStationInfo> *(*getAccessPointWirelessStations)(NObject *wirelessDomain, uint32_t apIndex, const MacAddress& macAddr, const InetAddress& ipAddr, const TCHAR *serial);
};

/**
 * Helper function for converting frequency to radio band
 */
RadioBand LIBNXSRV_EXPORTABLE WirelessFrequencyToBand(int freq);

/**
 * Helper function for converting frequency to channel number
 */
uint16_t LIBNXSRV_EXPORTABLE WirelessFrequencyToChannel(int freq);

/**
 * Helper function for converting wureless channel to frequency
 */
uint16_t LIBNXSRV_EXPORTABLE WirelessChannelToFrequency(RadioBand band, uint16_t channel);

/**
 * Compare two lists of radio interfaces
 */
bool LIBNXSRV_EXPORTABLE CompareRadioInterfaceLists(const StructArray<RadioInterfaceInfo> *list1, const StructArray<RadioInterfaceInfo> *list2);

/**
 * Build IP address from OID part (encoded as type length value)
 * If "withMask" set to true, OID element following address is interpreted as prefix length
 */
InetAddress LIBNXSRV_EXPORTABLE InetAddressFromOID(const uint32_t* oid, bool withMask, int *shift);

#endif   /* _nddrv_h_ */
