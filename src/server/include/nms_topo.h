/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2023 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: nms_topo.h
**
**/

#ifndef _nms_topo_h_
#define _nms_topo_h_

#include <netxms_maps.h>

class Node;
class Interface;

/**
 * LLDP local port info
 */
struct LLDP_LOCAL_PORT_INFO
{
   uint32_t portNumber;
   uint32_t localIdSubtype;
   BYTE localId[256];
   size_t localIdLen;
   TCHAR ifDescr[192];
};

/**
 * Build LLDP OID depending on MIB version
 */
static inline String LLDP_OID(const TCHAR *suffix, bool v2)
{
   StringBuffer oid(v2 ? _T(".1.3.111.2.802.1.1.13.") : _T(".1.0.8802.1.1.2."));
   oid.append(suffix);
   return oid;
}

/**
 * Get LLDP MIB name
 */
static inline const TCHAR *LLDP_MIB_NAME(bool v2)
{
   return v2 ? _T("LLDP-V2-MIB") : _T("LLDP-MIB");
}

/**
 * Network path element type
 */
enum class NetworkPathElementType
{
   ROUTE = 0,
   VPN = 1,
   PROXY = 2,
   DUMMY = 3
};

/**
 * Network path element
 */
struct NetworkPathElement
{
   NetworkPathElementType type;
   shared_ptr<NetObj> object;    // Current hop object
   InetAddress nextHop;          // Next hop address
   InetAddress route;            // Route used (UNSPEC for VPN connectors and direct access)
   uint32_t ifIndex;             // Interface index or object ID
   TCHAR name[MAX_OBJECT_NAME];
};

/**
 * Network path trace
 */
class NetworkPath
{
private:
   InetAddress m_sourceAddress;
   bool m_complete;
   ObjectArray<NetworkPathElement> m_path;

public:
   NetworkPath(const InetAddress& srcAddr);
   ~NetworkPath();

   void addHop(const shared_ptr<NetObj>& currentObject, const InetAddress& nextHop, const InetAddress& route, uint32_t ifIndex, const TCHAR *name);
   void addHop(const shared_ptr<NetObj>& currentObject, NetworkPathElementType type, uint32_t nextHopId, const TCHAR *name);
   void setComplete() { m_complete = true; }

   const InetAddress& getSourceAddress() const { return m_sourceAddress; }
   bool isComplete() const { return m_complete; }
   int getHopCount() const { return m_path.size(); }
   NetworkPathElement *getHopInfo(int index) const { return m_path.get(index); }

   void fillMessage(NXCPMessage *msg) const;
   void print(ServerConsole *console, int padding) const;
};

/**
 * FDB entry
 */
struct FDB_ENTRY
{
   uint32_t port;       // Port number
   uint32_t ifIndex;    // Interface index
   MacAddress macAddr;  // MAC address
   uint32_t nodeObject; // ID of node object or 0 if not found
   uint16_t vlanId;     // VLAN ID
   uint16_t type;       // Entry type
};

/**
 * FDB port mapping entry
 */
struct PORT_MAPPING_ENTRY
{
   uint32_t port;
   uint32_t ifIndex;
};

#ifdef _WIN32
template class NXCORE_EXPORTABLE StructArray<FDB_ENTRY>;
template class NXCORE_EXPORTABLE StructArray<PORT_MAPPING_ENTRY>;
#endif

/**
 * Switch forwarding database
 */
class NXCORE_EXPORTABLE ForwardingDatabase
{
private:
   uint32_t m_nodeId;
   StructArray<FDB_ENTRY> m_fdb;
   StructArray<PORT_MAPPING_ENTRY> m_portMap;
   time_t m_timestamp;
   bool m_portReferenceByIfIndex;
   uint16_t m_currentVlanId;

   uint32_t ifIndexFromPort(uint32_t port);
   static String interfaceIndexToName(const shared_ptr<NetObj>& node, uint32_t index);

public:
   ForwardingDatabase(uint32_t nodeId, bool portReferenceByIfIndex);

   void addEntry(FDB_ENTRY *entry);
   void addPortMapping(PORT_MAPPING_ENTRY *entry) { m_portMap.add(entry); }
   void sort();

   time_t getTimeStamp() const { return m_timestamp; }
   int getAge() const { return static_cast<int>(time(nullptr) - m_timestamp); }
   int getSize() const { return m_fdb.size(); }
   FDB_ENTRY *getEntry(int index) const { return m_fdb.get(index); }

   void setCurrentVlanId(uint16_t vlanId) { m_currentVlanId = vlanId; }
   uint16_t getCurrentVlanId() const { return m_currentVlanId; }

   uint32_t findMacAddress(const MacAddress& macAddr, bool *isStatic);
   void findMacAddressByPattern(const BYTE* macPattern, size_t macPatternSize, HashSet<MacAddress>* hs);
   bool isSingleMacOnPort(uint32_t ifIndex, MacAddress *macAddr = nullptr);
   int getMacCountOnPort(uint32_t ifIndex);

   shared_ptr<Table> getAsTable();

   void print(ServerConsole *console, const Node& owner);
   void fillMessage(NXCPMessage *msg);
};

#ifdef _WIN32
template class NXCORE_EXPORTABLE shared_ptr<ForwardingDatabase>;
#endif

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
   LL_PROTO_OTHER   = 7     /* Other (proprietary) protocol, information provided by driver */
};

/**
 * Link layer neighbor information
 */
struct L1_NEIGHBOR_INFO
{
   uint32_t ifLocal;           // Local interface index
   uint32_t ifRemote;          // Remote interface index
   uint32_t objectId;          // ID of connected object
   StringBuffer routeInfo;           // Information about route between links
};

/**
 * Link layer neighbor information
 */
struct LL_NEIGHBOR_INFO
{
   uint32_t ifLocal;           // Local interface index
   uint32_t ifRemote;          // Remote interface index
   uint32_t objectId;          // ID of connected object
   bool isPtToPt;              // true if this is point-to-point link
   LinkLayerProtocol protocol; // Protocol used to obtain information
   bool isCached;              // true if this is cached information
};

#ifdef _WIN32
template class NXCORE_EXPORTABLE StructArray<LL_NEIGHBOR_INFO>;
#endif

/**
 * link layer neighbors
 */
class NXCORE_EXPORTABLE LinkLayerNeighbors
{
private:
   StructArray<LL_NEIGHBOR_INFO> m_connections;
   HashSet<uint32_t> m_multipointInterfaces;  // List of interfaces where more than one MAC was found
   void *m_data[4];

   bool isDuplicate(LL_NEIGHBOR_INFO *info);

public:
   LinkLayerNeighbors();

   void addConnection(LL_NEIGHBOR_INFO *info);
   LL_NEIGHBOR_INFO *getConnection(int index) const { return m_connections.get(index); }

   void setData(int index, void *data) { if ((index >= 0) && (index < 4)) m_data[index] = data; }
   void *getData(int index) const { return ((index >= 0) && (index < 4)) ? m_data[index] : nullptr; }
   void setData(void *data) { setData(0, data); }
   void *getData() const { return getData(0); }

   int size() const { return m_connections.size(); }

   void markMultipointInterface(uint32_t ifIndex) { m_multipointInterfaces.put(ifIndex); }
   bool isMultipointInterface(uint32_t ifIndex) const { return m_multipointInterfaces.contains(ifIndex); }
};

#ifdef _WIN32
template class NXCORE_EXPORTABLE shared_ptr<LinkLayerNeighbors>;
#endif

//
// VRRP information
//

#define VRRP_STATE_INITIALIZE 1
#define VRRP_STATE_BACKUP     2
#define VRRP_STATE_MASTER     3

#define VRRP_VIP_ACTIVE       1
#define VRRP_VIP_DISABLED     2
#define VRRP_VIP_NOTREADY     3

/**
 * VRRP router
 */
class VrrpRouter
{
   friend uint32_t VRRPHandler(SNMP_Variable *, SNMP_Transport *, void *);

private:
   uint32_t m_id;
   uint32_t m_ifIndex;
   int m_state;
   BYTE m_virtualMacAddr[MAC_ADDR_LENGTH];
   int m_ipAddrCount;
   uint32_t *m_ipAddrList;

   void addVirtualIP(SNMP_Variable *var);

protected:
   bool readVirtualIP(SNMP_Transport *transport);

public:
   VrrpRouter(uint32_t id, uint32_t ifIndex, int state, const BYTE *macAddr);
   ~VrrpRouter();

   uint32_t getId() const { return m_id; }
   uint32_t getIfIndex() const { return m_ifIndex; }
   int getState() const { return m_state; }
   const BYTE *getVirtualMacAddr() const { return m_virtualMacAddr; }
   int getVipCount() const { return m_ipAddrCount; }
   uint32_t getVip(int index) const { return ((index >= 0) && (index < m_ipAddrCount)) ? m_ipAddrList[index] : 0; }
};

/**
 * VRRP information
 */
class VrrpInfo
{
   friend uint32_t VRRPHandler(SNMP_Variable *, SNMP_Transport *, void *);

private:
   int m_version;
   ObjectArray<VrrpRouter> m_routers;

protected:
   void addRouter(VrrpRouter *router) { m_routers.add(router); }

public:
   VrrpInfo(int version) : m_routers(0, 16, Ownership::True)
   {
      m_version = version;
   }

   int getVersion() { return m_version; }
   int size() { return m_routers.size(); }
   VrrpRouter *getRouter(int index) { return m_routers.get(index); }
};

/**
 * Spanning tree port state
 */
enum class SpanningTreePortState : int16_t
{
   UNKNOWN = 0,
   DISABLED = 1,
   BLOCKING = 2,
   LISTENING = 3,
   LEARNING = 4,
   FORWARDING = 5,
   BROKEN = 6
};

/**
 * OSPF area
 */
struct OSPFArea
{
   uint32_t id;
   uint32_t lsaCount;
   uint32_t areaBorderRouterCount;
   uint32_t asBorderRouterCount;
};

/**
 * OSPF neighbor states
 */
enum class OSPFNeighborState : int16_t
{
   UNKNOWN = 0,
   DOWN = 1,
   ATTEMPT = 2,
   INIT = 3,
   TWO_WAY = 4,
   EXCHANGE_START = 5,
   EXCHANGE = 6,
   LOADING = 7,
   FULL = 8
};

/**
 * OSPF neighbor
 */
struct OSPFNeighbor
{
   InetAddress ipAddress;
   uint32_t routerId;
   uint32_t nodeId;
   uint32_t ifIndex;
   uint32_t ifObject;
   uint32_t areaId;  // Only for virtual neighbors
   bool isVirtual;
   OSPFNeighborState state;
};

/**
 * OSPF interface types
 */
enum class OSPFInterfaceType : int16_t
{
   UNKNOWN = 0,
   BROADCAST = 1,
   NBMA = 2,
   POINT_TO_POINT = 3,
   POINT_TO_MULTIPOINT = 4
};

/**
 * OSPF interface states
 */
enum class OSPFInterfaceState : int16_t
{
   UNKNOWN = 0,
   DOWN = 1,
   LOOPBACK = 2,
   WAITING = 3,
   POINT_TO_POINT = 4,
   DESIGNATED_ROUTER = 5,
   BACKUP_DESIGNATED_ROUTER = 6,
   OTHER_DESIGNATED_ROUTER = 7
};

/**
 * OSPF interface
 */
struct OSPFInterface
{
   uint32_t ifIndex;
   uint32_t areaId;
   OSPFInterfaceType type;
   OSPFInterfaceState state;
   uint32_t dr;
   uint32_t bdr;
};

//////////////////////////////////
// Topology functions

shared_ptr<NetworkPath> TraceRoute(const shared_ptr<Node>& src, const shared_ptr<Node>& dest);
const ROUTE *SelectBestRoute(const RoutingTable& routes, const InetAddress& destination);
void BuildL2Topology(NetworkMapObjectList &topology, Node *root, int depth, bool includeEndNodes, bool useL1Topology);
shared_ptr<ForwardingDatabase> GetSwitchForwardingDatabase(Node *node);
shared_ptr<NetObj> FindInterfaceConnectionPoint(const MacAddress& macAddr, int *type);

class MacAddressInfo
{
private:
   MacAddress m_macAddr;
   shared_ptr<Interface> m_owner;
   shared_ptr<NetObj> m_connectionPoint;
   int m_type;

public:
   MacAddressInfo(const MacAddress& macAddr, const shared_ptr<Interface>& owner, const shared_ptr<NetObj>& connectionPoint, int type)
      : m_macAddr(macAddr), m_owner(owner), m_connectionPoint(connectionPoint)
   {
      m_type = type;
   }

   void fillMessage(NXCPMessage* msg, uint32_t base) const;
};

void FindMacAddresses(const BYTE* macPattern, size_t macPatternSize, ObjectArray<MacAddressInfo>* out, int searchLimit);

ObjectArray<LLDP_LOCAL_PORT_INFO> *GetLLDPLocalPortInfo(const Node& node, SNMP_Transport *snmp);

shared_ptr<LinkLayerNeighbors> BuildLinkLayerNeighborList(Node *node);
void AddLLDPNeighbors(Node *node, LinkLayerNeighbors *nbs);
void AddNDPNeighbors(Node *node, LinkLayerNeighbors *nbs);
void AddCDPNeighbors(Node *node, LinkLayerNeighbors *nbs);
void AddSTPNeighbors(Node *node, LinkLayerNeighbors *nbs);
String BuildLldpId(int type, const BYTE *data, size_t length);

const TCHAR *STPPortStateToText(SpanningTreePortState state);

void BridgeMapPorts(SNMP_Transport *transport, InterfaceList *ifList);

VrrpInfo *GetVRRPInfo(Node *node);

const TCHAR *GetLinkLayerProtocolName(LinkLayerProtocol p); 

unique_ptr<NetworkMapObjectList> BuildIPTopology(const shared_ptr<Node>& root, int radius, bool includeEndNodes);

bool CollectOSPFInformation(Node *node, StructArray<OSPFArea> *areas, StructArray<OSPFInterface> *interfaces, StructArray<OSPFNeighbor> *neighbors);
const TCHAR *OSPFNeighborStateToText(OSPFNeighborState state);
const TCHAR *OSPFInterfaceStateToText(OSPFInterfaceState state);
const TCHAR *OSPFInterfaceTypeToText(OSPFInterfaceType type);
unique_ptr<NetworkMapObjectList> BuildOSPFTopology(const shared_ptr<Node>& root, int radius);

#endif   /* _nms_topo_h_ */
