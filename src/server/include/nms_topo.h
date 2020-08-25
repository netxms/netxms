/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2016 Victor Kirhenshtein
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
	uint32_t port;                    // Port number
	uint32_t ifIndex;                 // Interface index
	BYTE macAddr[MAC_ADDR_LENGTH]; // MAC address
	uint32_t nodeObject;              // ID of node object or 0 if not found
	uint16_t vlanId;
	uint16_t type;
};

/**
 * FDB port mapping entry
 */
struct PORT_MAPPING_ENTRY
{
   uint32_t port;
   uint32_t ifIndex;
};

/**
 * Switch forwarding database
 */
class ForwardingDatabase : public RefCountObject
{
private:
   uint32_t m_nodeId;
	int m_fdbSize;
	int m_fdbAllocated;
	FDB_ENTRY *m_fdb;
	int m_pmSize;
	int m_pmAllocated;
	PORT_MAPPING_ENTRY *m_portMap;
	time_t m_timestamp;
	uint16_t m_currentVlanId;

	uint32_t ifIndexFromPort(uint32_t port);
	static String interfaceIndexToName(const shared_ptr<NetObj>& node, uint32_t index);

public:
	ForwardingDatabase(uint32_t nodeId);
	virtual ~ForwardingDatabase();

	void addEntry(FDB_ENTRY *entry);
	void addPortMapping(PORT_MAPPING_ENTRY *entry);
	void sort();

	time_t getTimeStamp() { return m_timestamp; }
	int getAge() { return (int)(time(NULL) - m_timestamp); }
	int getSize() { return m_fdbSize; }
	FDB_ENTRY *getEntry(int index) { return ((index >= 0) && (index < m_fdbSize)) ? &m_fdb[index] : NULL; }

   void setCurrentVlanId(UINT16 vlanId) { m_currentVlanId = vlanId; }
   UINT16 getCurrentVlanId() { return m_currentVlanId; }

   uint32_t findMacAddress(const BYTE *macAddr, bool *isStatic);
	bool isSingleMacOnPort(uint32_t ifIndex, BYTE *macAddr = NULL);
	int getMacCountOnPort(uint32_t ifIndex);

   void print(CONSOLE_CTX ctx, Node *owner);
   void fillMessage(NXCPMessage *msg);
   Table *getAsTable();
};

/**
 * Link layer discovery protocols
 */
enum LinkLayerProtocol
{
   LL_PROTO_UNKNOWN = 0, /* unknown source */
   LL_PROTO_FDB  = 1,    /* obtained from switch forwarding database */
   LL_PROTO_CDP  = 2,    /* Cisco Discovery Protocol */
   LL_PROTO_LLDP = 3,    /* Link Layer Discovery Protocol */
   LL_PROTO_NDP  = 4,    /* Nortel Discovery Protocol */
   LL_PROTO_EDP  = 5,    /* Extreme Discovery Protocol */
   LL_PROTO_STP  = 6     /* Spanning Tree Protocol */
};

/**
 * Link layer neighbor information
 */
struct LL_NEIGHBOR_INFO
{
	uint32_t ifLocal;           // Local interface index
	uint32_t ifRemote;          // Remote interface index
	uint32_t objectId;		    // ID of connected object
	bool isPtToPt;			       // true if this is point-to-point link
	LinkLayerProtocol protocol; // Protocol used to obtain information
   bool isCached;              // true if this is cached information
};

/**
 * link layer neighbors
 */
class LinkLayerNeighbors : public RefCountObject
{
private:
	int m_count;
	int m_allocated;
	LL_NEIGHBOR_INFO *m_connections;
	void *m_data[4];

	bool isDuplicate(LL_NEIGHBOR_INFO *info);

public:
	LinkLayerNeighbors();
	virtual ~LinkLayerNeighbors();

	void addConnection(LL_NEIGHBOR_INFO *info);
	LL_NEIGHBOR_INFO *getConnection(int index) { return ((index >= 0) && (index < m_count)) ? &m_connections[index] : NULL; }

	void setData(int index, void *data) { if ((index >= 0) && (index < 4)) m_data[index] = data; }
	void *getData(int index) { return ((index >= 0) && (index < 4)) ? m_data[index] : NULL; }
	void setData(void *data) { setData(0, data); }
	void *getData() { return getData(0); }
	int size() { return m_count; }
};


//
// VRRP information
//

#define VRRP_STATE_INITIALIZE 1
#define VRRP_STATE_BACKUP     2
#define VRRP_STATE_MASTER     3

#define VRRP_VIP_ACTIVE       1
#define VRRP_VIP_DISABLED     2
#define VRRP_VIP_NOTREADY     3

class VrrpRouter
{
	friend UINT32 VRRPHandler(SNMP_Variable *, SNMP_Transport *, void *);

private:
	UINT32 m_id;
	UINT32 m_ifIndex;
	int m_state;
	BYTE m_virtualMacAddr[MAC_ADDR_LENGTH];
	int m_ipAddrCount;
	UINT32 *m_ipAddrList;

	void addVirtualIP(SNMP_Variable *var);
	static UINT32 walkerCallback(SNMP_Variable *var, SNMP_Transport *transport, void *arg);

protected:
	bool readVirtualIP(SNMP_Transport *transport);

public:
	VrrpRouter(UINT32 id, UINT32 ifIndex, int state, BYTE *macAddr);
	~VrrpRouter();

	UINT32 getId() { return m_id; }
	UINT32 getIfIndex() { return m_ifIndex; }
	int getState() { return m_state; }
	BYTE *getVirtualMacAddr() { return m_virtualMacAddr; }
	int getVipCount() { return m_ipAddrCount; }
	UINT32 getVip(int index) { return ((index >= 0) && (index < m_ipAddrCount)) ? m_ipAddrList[index] : 0; }
};

class VrrpInfo
{
	friend UINT32 VRRPHandler(SNMP_Variable *, SNMP_Transport *, void *);

private:
	int m_version;
	ObjectArray<VrrpRouter> *m_routers;

protected:
	void addRouter(VrrpRouter *router) { m_routers->add(router); }

public:
	VrrpInfo(int version);
	~VrrpInfo();

	int getVersion() { return m_version; }
	int size() { return m_routers->size(); }
	VrrpRouter *getRouter(int index) { return m_routers->get(index); }
};


//
// Topology functions
//

shared_ptr<NetworkPath> TraceRoute(const shared_ptr<Node>& src, const shared_ptr<Node>& dest);
void BuildL2Topology(NetworkMapObjectList &topology, Node *root, int nDepth, bool includeEndNodes);
ForwardingDatabase *GetSwitchForwardingDatabase(Node *node);
shared_ptr<NetObj> FindInterfaceConnectionPoint(const MacAddress& macAddr, int *type);

ObjectArray<LLDP_LOCAL_PORT_INFO> *GetLLDPLocalPortInfo(SNMP_Transport *snmp);

LinkLayerNeighbors *BuildLinkLayerNeighborList(Node *node);
void AddLLDPNeighbors(Node *node, LinkLayerNeighbors *nbs);
void AddNDPNeighbors(Node *node, LinkLayerNeighbors *nbs);
void AddCDPNeighbors(Node *node, LinkLayerNeighbors *nbs);
void AddSTPNeighbors(Node *node, LinkLayerNeighbors *nbs);
void BuildLldpId(int type, const BYTE *data, int length, TCHAR *id, int idLen);

void BridgeMapPorts(SNMP_Transport *transport, InterfaceList *ifList);

VrrpInfo *GetVRRPInfo(Node *node);

const TCHAR *GetLinkLayerProtocolName(LinkLayerProtocol p); 

#endif   /* _nms_topo_h_ */
