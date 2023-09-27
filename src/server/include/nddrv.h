/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2021 Victor Kirhenshtein
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
#define NDDRV_API_VERSION           10

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
   AP_ADOPTED = 0,
   AP_UNADOPTED = 1,
   AP_DOWN = 2,
   AP_UNKNOWN = 3
};

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
   TCHAR vendor[128];
   TCHAR productCode[32];
   TCHAR productName[128];
   TCHAR productVersion[16];
   TCHAR serialNumber[32];
};

/**
 * Device view element flags
 */
#define DVF_BACKGROUND  0x0001
#define DVF_BORDER      0x0002

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
   const TCHAR *imageName; // library image name, can be null
   const TCHAR *commands;  // drawing commands, can be null
};

#ifdef _WIN32
template class LIBNXSRV_EXPORTABLE StructArray<DeviceViewElement>;
#endif

/**
 * Device view
 */
class LIBNXSRV_EXPORTABLE DeviceView
{
private:
   StructArray<DeviceViewElement> m_elements;
   time_t m_timestamp;

public:
   DeviceView() : m_elements(0, 16)
   {
      m_timestamp = time(nullptr);
   }
   DeviceView(const StructArray<DeviceViewElement>& elements) : m_elements(elements)
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
 * Radio interface information
 */
struct LIBNXSRV_EXPORTABLE RadioInterfaceInfo
{
	uint32_t index;
	TCHAR name[64];
	BYTE macAddr[MAC_ADDR_LENGTH];
   uint32_t channel;
   int32_t powerDBm;
   int32_t powerMW;

   json_t *toJson() const;
};

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
	ObjectArray<RadioInterfaceInfo> *m_radioInterfaces;

public:
   AccessPointInfo(uint32_t index, const MacAddress& macAddr, const InetAddress& ipAddr, AccessPointState state,
            const TCHAR *name, const TCHAR *vendor, const TCHAR *model, const TCHAR *serial);
   ~AccessPointInfo();

	void addRadioInterface(const RadioInterfaceInfo& iface);
	void setMacAddr(const MacAddress& macAddr) { m_macAddr = macAddr; }

	uint32_t getIndex() const { return m_index; }
	const MacAddress& getMacAddr() const { return m_macAddr; }
   const InetAddress& getIpAddr() const { return m_ipAddr; }
	AccessPointState getState() const { return m_state; }
	const TCHAR *getName() const { return m_name; }
	const TCHAR *getVendor() const { return m_vendor; }
	const TCHAR *getModel() const { return m_model; }
	const TCHAR *getSerial() const { return m_serial; }
	const ObjectArray<RadioInterfaceInfo> *getRadioInterfaces() const { return m_radioInterfaces; }
};

/**
 * Wireless station AP match policy
 */
#define AP_MATCH_BY_RFINDEX   0
#define AP_MATCH_BY_BSSID     1

/**
 * Wireless station information
 */
struct WirelessStationInfo
{
	// This part filled by driver
   BYTE macAddr[MAC_ADDR_LENGTH];
	InetAddress ipAddr;
	int rfIndex;	// radio interface index
   BYTE bssid[MAC_ADDR_LENGTH];
   short apMatchPolicy;
	TCHAR ssid[MAX_OBJECT_NAME];
   int vlan;
   int signalStrength;
   uint32_t txRate;
   uint32_t rxRate;

	// This part filled by core
   uint32_t apObjectId;
   uint32_t nodeId;
   TCHAR rfName[MAX_OBJECT_NAME];
};

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
template class LIBNXSRV_EXPORTABLE ObjectArray<HostMibStorageEntry>;
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
   const HostMibStorageEntry *getPhysicalMemory(SNMP_Transport *snmp) { return getStorageEntry(snmp, NULL, hrStorageRam); }
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
 * Link layer neighbor information
 */
struct LinkLayerNeighborInfo
{
   uint32_t ifLocal;      // Local interface index
   InterfaceId ifRemote;  // Remote interface identification
   TCHAR remoteName[192]; // sysName or DNS host name of connected object
   InetAddress remoteIP;  // IP address of connected object
   MacAddress remoteMAC;  // MAC address of connected object
   bool isPtToPt;         // true if this is point-to-point link

   LinkLayerNeighborInfo()
   {
      ifLocal = 0;
      memset(&ifRemote, 0, sizeof(InterfaceId));
      remoteName[0] = 0;
      isPtToPt = false;
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
   uint64_t getInterfaceSpeed(SNMP_Transport *snmp, uint32_t ifIndex, int ifTableSuffixLen, uint32_t *ifTableSuffix);
   void registerHostMibMetrics(ObjectArray<AgentParameterDefinition> *metrics);
   DataCollectionError getHostMibMetric(SNMP_Transport *snmp, HostMibDriverData *driverData, const TCHAR *name, TCHAR *value, size_t size);

public:
   NetworkDeviceDriver();
   virtual ~NetworkDeviceDriver();

   virtual const TCHAR *getName();
   virtual const TCHAR *getVersion();

   virtual const TCHAR *getCustomTestOID();
   virtual int isPotentialDevice(const TCHAR *oid);
   virtual bool isDeviceSupported(SNMP_Transport *snmp, const TCHAR *oid);
   virtual void analyzeDevice(SNMP_Transport *snmp, const TCHAR *oid, NObject *node, DriverData **driverData);
   virtual bool getHardwareInformation(SNMP_Transport *snmp, NObject *node, DriverData *driverData, DeviceHardwareInfo *hwInfo);
   virtual bool isEntityMibEmulationSupported(SNMP_Transport *snmp, NObject *node, DriverData *driverData);
   virtual shared_ptr<ComponentTree> buildComponentTree(SNMP_Transport *snmp, NObject *node, DriverData *driverData);
   virtual shared_ptr<DeviceView> buildDeviceView(SNMP_Transport *snmp, NObject *node, DriverData *driverData, const Component *rootComponent);
   virtual int getModulesOrientation(SNMP_Transport *snmp, NObject *node, DriverData *driverData);
   virtual void getModuleLayout(SNMP_Transport *snmp, NObject *node, DriverData *driverData, int module, NDD_MODULE_LAYOUT *layout);
   virtual bool getVirtualizationType(SNMP_Transport *snmp, NObject *node, DriverData *driverData, VirtualizationType *vtype);
   virtual GeoLocation getGeoLocation(SNMP_Transport *snmp, NObject *node, DriverData *driverData);
   virtual InterfaceList *getInterfaces(SNMP_Transport *snmp, NObject *node, DriverData *driverData, bool useIfXTable);
   virtual void getInterfaceState(SNMP_Transport *snmp, NObject *node, DriverData *driverData, uint32_t ifIndex,
            int ifTableSuffixLen, uint32_t *ifTableSuffix, InterfaceAdminState *adminState, InterfaceOperState *operState, uint64_t *speed);
   virtual bool lldpNameToInterfaceId(SNMP_Transport *snmp, NObject *node, DriverData *driverData, const TCHAR *lldpName, InterfaceId *id);
   virtual bool isLldpRemTableUsingIfIndex(const NObject *node, DriverData *driverData);
   virtual VlanList *getVlans(SNMP_Transport *snmp, NObject *node, DriverData *driverData);
   virtual bool isPerVlanFdbSupported();
   virtual bool isFdbUsingIfIndex(const NObject *node, DriverData *driverData);
   virtual int getClusterMode(SNMP_Transport *snmp, NObject *node, DriverData *driverData);
   virtual bool isWirelessController(SNMP_Transport *snmp, NObject *node, DriverData *driverData);
   virtual ObjectArray<AccessPointInfo> *getAccessPoints(SNMP_Transport *snmp, NObject *node, DriverData *driverData);
   virtual ObjectArray<WirelessStationInfo> *getWirelessStations(SNMP_Transport *snmp, NObject *node, DriverData *driverData);
   virtual AccessPointState getAccessPointState(SNMP_Transport *snmp, NObject *node, DriverData *driverData,
                                                UINT32 apIndex, const MacAddress& macAddr, const InetAddress& ipAddr,
                                                const ObjectArray<RadioInterfaceInfo> *radioInterfaces);
   virtual bool hasMetrics();
   virtual DataCollectionError getMetric(SNMP_Transport *snmp, NObject *node, DriverData *driverData, const TCHAR *name, TCHAR *value, size_t size);
   virtual ObjectArray<AgentParameterDefinition> *getAvailableMetrics(SNMP_Transport *snmp, NObject *node, DriverData *driverData);
   virtual shared_ptr<ArpCache> getArpCache(SNMP_Transport *snmp, DriverData *driverData);
   virtual ObjectArray<LinkLayerNeighborInfo> *getLinkLayerNeighbors(SNMP_Transport *snmp, DriverData *driverData);
};

/**
 * Helper function for converting frequency to channel number
 */
int LIBNXSRV_EXPORTABLE WirelessFrequencyToChannel(int freq);

#endif   /* _nddrv_h_ */
