/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2011 Victor Kirhenshtein
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


//
// Hop information structure
//

struct HOP_INFO
{
   DWORD nextHop;     // Next hop address
   NetObj *object;    // Current hop object
   DWORD ifIndex;     // Interface index or VPN connector object ID
   bool isVpn;        // TRUE if next hop is behind VPN tunnel
};


//
// Network path trace
//

class NetworkPath
{
private:
	int m_hopCount;
	int m_allocated;
	HOP_INFO *m_path;
	bool m_complete;

public:
	NetworkPath();
	~NetworkPath();

	void addHop(DWORD nextHop, NetObj *currentObject, DWORD ifIndex, bool isVpn);
	void setComplete() { m_complete = true; }

	bool isComplete() { return m_complete; }
	int getHopCount() { return m_hopCount; }
	HOP_INFO *getHopInfo(int index) { return ((index >= 0) && (index < m_hopCount)) ? &m_path[index] : NULL; }

	void fillMessage(CSCPMessage *msg);
};


//
// Switch forwarding database
//

struct FDB_ENTRY
{
	DWORD port;                    // Port number
	DWORD ifIndex;                 // Interface index
	BYTE macAddr[MAC_ADDR_LENGTH]; // MAC address
	DWORD nodeObject;              // ID of node object or 0 if not found
};

struct PORT_MAPPING_ENTRY
{
	DWORD port;
	DWORD ifIndex;
};

class ForwardingDatabase : public RefCountObject
{
private:
	int m_fdbSize;
	int m_fdbAllocated;
	FDB_ENTRY *m_fdb;
	int m_pmSize;
	int m_pmAllocated;
	PORT_MAPPING_ENTRY *m_portMap;
	time_t m_timestamp;

	DWORD ifIndexFromPort(DWORD port);

public:
	ForwardingDatabase();
	virtual ~ForwardingDatabase();

	void addEntry(FDB_ENTRY *entry);
	void addPortMapping(PORT_MAPPING_ENTRY *entry);
	void sort();

	time_t getTimeStamp() { return m_timestamp; }
	int getAge() { return (int)(time(NULL) - m_timestamp); }
	int getSize() { return m_fdbSize; }
	FDB_ENTRY *getEntry(int index) { return ((index >= 0) && (index < m_fdbSize)) ? &m_fdb[index] : NULL; }

	DWORD findMacAddress(const BYTE *macAddr);
	bool isSingleMacOnPort(DWORD ifIndex, BYTE *macAddr = NULL);
	int getMacCountOnPort(DWORD ifIndex);
};


//
// link layer neighbors
//

#define LL_PROTO_FDB    0		/* obtained from switch forwarding database */
#define LL_PROTO_CDP    1		/* Cisco Discovery Protocol */
#define LL_PROTO_LLDP   2		/* Link Layer Discovery Protocol */
#define LL_PROTO_NDP    3		/* Nortel Discovery Protocol */
#define LL_PROTO_EDP    4		/* Extreme Discovery Protocol */

struct LL_NEIGHBOR_INFO
{
	DWORD ifLocal;			// Local interface index
	DWORD ifRemote;		// Remote interface index
	DWORD objectId;		// ID of connected object
	bool isPtToPt;			// true if this is point-to-point link
	int protocol;			// Protocol used to obtain information
};

class LinkLayerNeighbors : public RefCountObject
{
private:
	int m_count;
	int m_allocated;
	LL_NEIGHBOR_INFO *m_connections;
	void *m_data;

	bool isDuplicate(LL_NEIGHBOR_INFO *info);

public:
	LinkLayerNeighbors();
	virtual ~LinkLayerNeighbors();

	void addConnection(LL_NEIGHBOR_INFO *info);
	LL_NEIGHBOR_INFO *getConnection(int index) { return ((index >= 0) && (index < m_count)) ? &m_connections[index] : NULL; }

	void setData(void *data) { m_data = data; }
	void *getData() { return m_data; }
	int getSize() { return m_count; }
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
	friend DWORD VRRPHandler(DWORD, SNMP_Variable *, SNMP_Transport *, void *);

private:
	DWORD m_id;
	DWORD m_ifIndex;
	int m_state;
	BYTE m_virtualMacAddr[MAC_ADDR_LENGTH];
	int m_ipAddrCount;
	DWORD *m_ipAddrList;

	void addVirtualIP(SNMP_Variable *var);
	static DWORD walkerCallback(DWORD snmpVersion, SNMP_Variable *var, SNMP_Transport *transport, void *arg);

protected:
	bool readVirtualIP(DWORD snmpVersion, SNMP_Transport *transport);

public:
	VrrpRouter(DWORD id, DWORD ifIndex, int state, BYTE *macAddr);
	~VrrpRouter();

	DWORD getId() { return m_id; }
	DWORD getIfIndex() { return m_ifIndex; }
	int getState() { return m_state; }
	BYTE *getVirtualMacAddr() { return m_virtualMacAddr; }
	int getVipCount() { return m_ipAddrCount; }
	DWORD getVip(int index) { return ((index >= 0) && (index < m_ipAddrCount)) ? m_ipAddrList[index] : 0; }
};

class VrrpInfo
{
	friend DWORD VRRPHandler(DWORD, SNMP_Variable *, SNMP_Transport *, void *);

private:
	int m_version;
	ObjectArray<VrrpRouter> *m_routers;

protected:
	void addRouter(VrrpRouter *router) { m_routers->add(router); }

public:
	VrrpInfo(int version);
	~VrrpInfo();

	int getVersion() { return m_version; }
	int getSize() { return m_routers->size(); }
	VrrpRouter *getRouter(int index) { return m_routers->get(index); }
};


//
// Topology functions
//

NetworkPath *TraceRoute(Node *pSrc, Node *pDest);
void BuildL2Topology(nxmap_ObjList &topology, Node *root, int nDepth);
ForwardingDatabase *GetSwitchForwardingDatabase(Node *node);
Interface *FindInterfaceConnectionPoint(const BYTE *macAddr, bool *exactMatch);

LinkLayerNeighbors *BuildLinkLayerNeighborList(Node *node);
void AddLLDPNeighbors(Node *node, LinkLayerNeighbors *nbs);
void AddNDPNeighbors(Node *node, LinkLayerNeighbors *nbs);
void AddCDPNeighbors(Node *node, LinkLayerNeighbors *nbs);

void BridgeMapPorts(int snmpVersion, SNMP_Transport *transport, InterfaceList *ifList);

VrrpInfo *GetVRRPInfo(Node *node);

#endif   /* _nms_topo_h_ */
