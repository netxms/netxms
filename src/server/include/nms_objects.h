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
** File: nms_objects.h
**
**/

#ifndef _nms_objects_h_
#define _nms_objects_h_

#include <nms_agent.h>
#include <netxms_maps.h>
#include <geolocation.h>
#include "nxcore_jobs.h"
#include "nms_topo.h"
#include "nxcore_reports.h"


//
// Forward declarations of classes
//

class AgentConnection;
class AgentConnectionEx;
class ClientSession;
class Queue;


//
// Global variables used by inline functions
//

extern DWORD g_dwDiscoveryPollingInterval;
extern DWORD g_dwStatusPollingInterval;
extern DWORD g_dwConfigurationPollingInterval;
extern DWORD g_dwRoutingTableUpdateInterval;
extern DWORD g_dwTopologyPollingInterval;
extern DWORD g_dwConditionPollingInterval;

//
// Constants
//

#define MAX_INTERFACES        4096
#define MAX_ATTR_NAME_LEN     128
#define INVALID_INDEX         0xFFFFFFFF


//
// Last events
//

#define MAX_LAST_EVENTS       8

#define LAST_EVENT_NODE_DOWN  0


//
// Built-in object IDs
//

#define BUILTIN_OID_NETWORK               1
#define BUILTIN_OID_SERVICEROOT           2
#define BUILTIN_OID_TEMPLATEROOT          3
#define BUILTIN_OID_ZONE0                 4
#define BUILTIN_OID_POLICYROOT            5
#define BUILTIN_OID_NETWORKMAPROOT        6
#define BUILTIN_OID_DASHBOARDROOT         7
#define BUILTIN_OID_REPORTROOT            8
#define BUILTIN_OID_BUSINESSSERVICEROOT   9

//
// Node runtime (dynamic) flags
//

#define NDF_QUEUED_FOR_STATUS_POLL     0x0001
#define NDF_QUEUED_FOR_CONFIG_POLL     0x0002
#define NDF_UNREACHABLE                0x0004
#define NDF_AGENT_UNREACHABLE          0x0008
#define NDF_SNMP_UNREACHABLE           0x0010
#define NDF_QUEUED_FOR_DISCOVERY_POLL  0x0020
#define NDF_FORCE_STATUS_POLL          0x0040
#define NDF_FORCE_CONFIGURATION_POLL   0x0080
#define NDF_QUEUED_FOR_ROUTE_POLL      0x0100
#define NDF_CPSNMP_UNREACHABLE         0x0200
#define NDF_RECHECK_CAPABILITIES       0x0400
#define NDF_POLLING_DISABLED           0x0800
#define NDF_CONFIGURATION_POLL_PASSED  0x1000
#define NDF_QUEUED_FOR_TOPOLOGY_POLL   0x2000

#define __NDF_FLAGS_DEFINED


//
// Cluster runtime flags
//

#define CLF_QUEUED_FOR_STATUS_POLL     0x0001
#define CLF_DOWN								0x0002


//
// Status poll types
//

#define POLL_ICMP_PING        0
#define POLL_SNMP             1
#define POLL_NATIVE_AGENT     2


//
// Zone types
//

#define ZONE_TYPE_PASSIVE     0
#define ZONE_TYPE_ACTIVE      1


//
// Template update types
//

#define APPLY_TEMPLATE        0
#define REMOVE_TEMPLATE       1


//
// Queued template update information
//

struct TEMPLATE_UPDATE_INFO
{
   int iUpdateType;
   Template *pTemplate;
   DWORD dwNodeId;
   BOOL bRemoveDCI;
};


//
// DCI configuration for system templates
//

typedef struct
{
	const TCHAR *pszName;
	const TCHAR *pszParam;
	int nInterval;
	int nRetention;
	int nDataType;
	int nOrigin;
	int nFound;
} DCI_CFG;


//
// Object index structure
//

struct INDEX_ELEMENT
{
   QWORD key;
   NetObj *object;
};

class NXCORE_EXPORTABLE ObjectIndex
{
private:
	int m_size;
	int m_allocated;
	INDEX_ELEMENT *m_elements;
	RWLOCK m_lock;

	int findElement(QWORD key);

public:
	ObjectIndex();
	~ObjectIndex();

	bool put(QWORD key, NetObj *object);
	void remove(QWORD key);
	NetObj *get(QWORD key);
	NetObj *find(bool (*comparator)(NetObj *, void *), void *data);

	int getSize();
	ObjectArray<NetObj> *getObjects();

	void forEach(void (*callback)(NetObj *, void *), void *data);
};


//
// Base class for network objects
//

class NXCORE_EXPORTABLE NetObj
{
private:
	static void onObjectDeleteCallback(NetObj *object, void *data);

protected:
   DWORD m_dwId;
	uuid_t m_guid;
   DWORD m_dwTimeStamp;       // Last change time stamp
   DWORD m_dwRefCount;        // Number of references. Object can be destroyed only when this counter is zero
   TCHAR m_szName[MAX_OBJECT_NAME];
   TCHAR *m_pszComments;      // User comments
   int m_iStatus;
   int m_iStatusCalcAlg;      // Status calculation algorithm
   int m_iStatusPropAlg;      // Status propagation algorithm
   int m_iFixedStatus;        // Status if propagation is "Fixed"
   int m_iStatusShift;        // Shift value for "shifted" status propagation
   int m_iStatusTranslation[4];
   int m_iStatusSingleThreshold;
   int m_iStatusThresholds[4];
   BOOL m_bIsModified;
   BOOL m_bIsDeleted;
   BOOL m_bIsHidden;
	BOOL m_bIsSystem;
	uuid_t m_image;
   MUTEX m_mutexData;         // Object data access mutex
   MUTEX m_mutexRefCount;     // Reference counter access mutex
   RWLOCK m_rwlockParentList; // Lock for parent list
   RWLOCK m_rwlockChildList;  // Lock for child list
   DWORD m_dwIpAddr;          // Every object should have an IP address
	GeoLocation m_geoLocation;
   ClientSession *m_pPollRequestor;
	DWORD m_submapId;				// Map object which should be open on drill-down request

   DWORD m_dwChildCount;      // Number of child objects
   NetObj **m_pChildList;     // Array of pointers to child objects

   DWORD m_dwParentCount;     // Number of parent objects
   NetObj **m_pParentList;    // Array of pointers to parent objects

   AccessList *m_pAccessList;
   BOOL m_bInheritAccessRights;
   MUTEX m_mutexACL;

	DWORD m_dwNumTrustedNodes;	// Trusted nodes
	DWORD *m_pdwTrustedNodes;
	
	StringMap m_customAttributes;

   void LockData() { MutexLock(m_mutexData, INFINITE); }
   void UnlockData() { MutexUnlock(m_mutexData); }
   void LockACL() { MutexLock(m_mutexACL, INFINITE); }
   void UnlockACL() { MutexUnlock(m_mutexACL); }
   void LockParentList(BOOL bWrite) 
   { 
      if (bWrite) 
         RWLockWriteLock(m_rwlockParentList, INFINITE);
      else
         RWLockReadLock(m_rwlockParentList, INFINITE); 
   }
   void UnlockParentList() { RWLockUnlock(m_rwlockParentList); }
   void LockChildList(BOOL bWrite) 
   { 
      if (bWrite) 
         RWLockWriteLock(m_rwlockChildList, INFINITE);
      else
         RWLockReadLock(m_rwlockChildList, INFINITE); 
   }
   void UnlockChildList() { RWLockUnlock(m_rwlockChildList); }

   void Modify();                  // Used to mark object as modified

   BOOL loadACLFromDB();
   BOOL saveACLToDB(DB_HANDLE hdb);
   BOOL loadCommonProperties();
   BOOL saveCommonProperties(DB_HANDLE hdb);
	BOOL loadTrustedNodes();
	BOOL saveTrustedNodes(DB_HANDLE hdb);

   void SendPollerMsg(DWORD dwRqId, const TCHAR *pszFormat, ...);

   virtual void PrepareForDeletion();
   virtual void OnObjectDelete(DWORD dwObjectId);

public:
   NetObj();
   virtual ~NetObj();

   virtual int Type() { return OBJECT_GENERIC; }
   
   DWORD IpAddr() { return m_dwIpAddr; }
   DWORD Id() { return m_dwId; }
   const TCHAR *Name() { return m_szName; }
   int Status() { return m_iStatus; }
   int PropagatedStatus(void);
   DWORD TimeStamp() { return m_dwTimeStamp; }
	void getGuid(uuid_t out) { memcpy(out, m_guid, UUID_LENGTH); }

   BOOL IsModified(void) { return m_bIsModified; }
   BOOL IsDeleted(void) { return m_bIsDeleted; }
   BOOL IsOrphaned(void) { return m_dwParentCount == 0 ? TRUE : FALSE; }
   BOOL IsEmpty(void) { return m_dwChildCount == 0 ? TRUE : FALSE; }
	
	BOOL IsSystem() { return m_bIsSystem; }
	void SetSystemFlag(BOOL bFlag) { m_bIsSystem = bFlag; }

   DWORD RefCount();
   void IncRefCount();
   void DecRefCount();

   BOOL IsChild(DWORD dwObjectId);
	BOOL IsTrustedNode(DWORD id);

   void AddChild(NetObj *pObject);     // Add reference to child object
   void AddParent(NetObj *pObject);    // Add reference to parent object

   void DeleteChild(NetObj *pObject);  // Delete reference to child object
   void DeleteParent(NetObj *pObject); // Delete reference to parent object

   void deleteObject();     // Prepare object for deletion

   BOOL isHidden() { return m_bIsHidden; }
   void hide();
   void unhide();

   virtual BOOL SaveToDB(DB_HANDLE hdb);
   virtual BOOL DeleteFromDB(void);
   virtual BOOL CreateFromDB(DWORD dwId);

   void setId(DWORD dwId) { m_dwId = dwId; Modify(); }
	void generateGuid() { uuid_generate(m_guid); }
   void setMgmtStatus(BOOL bIsManaged);
   void setName(const TCHAR *pszName) { nx_strncpy(m_szName, pszName, MAX_OBJECT_NAME); Modify(); }
   void resetStatus() { m_iStatus = STATUS_UNKNOWN; Modify(); }
   void setComments(TCHAR *pszText);	/* pszText must be dynamically allocated */

   virtual void calculateCompoundStatus(BOOL bForcedRecalc = FALSE);

   virtual void CreateMessage(CSCPMessage *pMsg);
   virtual DWORD ModifyFromMessage(CSCPMessage *pRequest, BOOL bAlreadyLocked = FALSE);

   void CommentsToMessage(CSCPMessage *pMsg);

   DWORD GetUserRights(DWORD dwUserId);
   BOOL CheckAccessRights(DWORD dwUserId, DWORD dwRequiredRights);
   void DropUserAccess(DWORD dwUserId);

   void AddChildNodesToList(DWORD *pdwNumNodes, Node ***pppNodeList, DWORD dwUserId);
   
   const TCHAR *GetCustomAttribute(const TCHAR *name) { return m_customAttributes.get(name); }
   void SetCustomAttribute(const TCHAR *name, const TCHAR *value) { m_customAttributes.set(name, value); Modify(); }
   void SetCustomAttributePV(const TCHAR *name, TCHAR *value) { m_customAttributes.setPreallocated(_tcsdup(name), value); Modify(); }
   void DeleteCustomAttribute(const TCHAR *name) { m_customAttributes.remove(name); Modify(); }

   // Debug methods
   const TCHAR *ParentList(TCHAR *szBuffer);
   const TCHAR *ChildList(TCHAR *szBuffer);
};


//
// Inline functions of NetObj class
//

inline DWORD NetObj::RefCount()
{ 
   DWORD dwRefCount;

   MutexLock(m_mutexRefCount, INFINITE);
   dwRefCount = m_dwRefCount;
   MutexUnlock(m_mutexRefCount);
   return dwRefCount; 
}

inline void NetObj::IncRefCount()
{ 
   MutexLock(m_mutexRefCount, INFINITE);
   m_dwRefCount++;
   MutexUnlock(m_mutexRefCount);
}

inline void NetObj::DecRefCount()
{ 
   MutexLock(m_mutexRefCount, INFINITE);
   if (m_dwRefCount > 0) 
      m_dwRefCount--; 
   MutexUnlock(m_mutexRefCount);
}


//
// Node template class
//

class NXCORE_EXPORTABLE Template : public NetObj
{
protected:
   DWORD m_dwNumItems;     // Number of data collection items
   DCItem **m_ppItems;     // Data collection items
   DWORD m_dwDCILockStatus;
   DWORD m_dwVersion;
   BOOL m_bDCIListModified;
   TCHAR m_szCurrDCIOwner[MAX_SESSION_NAME];
	TCHAR *m_applyFilterSource;
	NXSL_Program *m_applyFilter;
	MUTEX m_mutexDciAccess;

   virtual void PrepareForDeletion();

   void loadItemsFromDB();
   void destroyItems();

	void lockDciAccess() { MutexLock(m_mutexDciAccess, INFINITE); }
	void unlockDciAccess() { MutexUnlock(m_mutexDciAccess); }

public:
   Template();
   Template(const TCHAR *pszName);
	Template(ConfigEntry *config);
   virtual ~Template();

   virtual int Type(void) { return OBJECT_TEMPLATE; }

   virtual BOOL SaveToDB(DB_HANDLE hdb);
   virtual BOOL DeleteFromDB(void);
   virtual BOOL CreateFromDB(DWORD dwId);

   virtual void CreateMessage(CSCPMessage *pMsg);
   virtual DWORD ModifyFromMessage(CSCPMessage *pRequest, BOOL bAlreadyLocked = FALSE);

   virtual void calculateCompoundStatus(BOOL bForcedRecalc = FALSE);

   int getVersionMajor() { return m_dwVersion >> 16; }
   int getVersionMinor() { return m_dwVersion & 0xFFFF; }

   DWORD getItemCount() { return m_dwNumItems; }
   bool addItem(DCItem *pItem, bool alreadyLocked = false);
   bool updateItem(DWORD dwItemId, CSCPMessage *pMsg, DWORD *pdwNumMaps, 
                   DWORD **ppdwMapIndex, DWORD **ppdwMapId);
   bool deleteItem(DWORD dwItemId, bool needLock);
   bool setItemStatus(DWORD dwNumItems, DWORD *pdwItemList, int iStatus);
   int getItemType(DWORD dwItemId);
   DCItem *getItemById(DWORD dwItemId);
   DCItem *getItemByIndex(DWORD dwIndex);
   DCItem *getItemByName(const TCHAR *pszName);
   DCItem *getItemByDescription(const TCHAR *pszDescription);
   BOOL LockDCIList(DWORD dwSessionId, const TCHAR *pszNewOwner, TCHAR *pszCurrOwner);
   BOOL UnlockDCIList(DWORD dwSessionId);
   void SetDCIModificationFlag(void) { m_bDCIListModified = TRUE; }
   void SendItemsToClient(ClientSession *pSession, DWORD dwRqId);
   BOOL IsLockedBySession(DWORD dwSessionId) { return m_dwDCILockStatus == dwSessionId; }
   DWORD *getDCIEventsList(DWORD *pdwCount);
	void ValidateSystemTemplate(void);
	void ValidateDCIList(DCI_CFG *cfg);

   BOOL ApplyToNode(Node *pNode);
	BOOL isApplicable(Node *node);
	BOOL isAutoApplyEnabled() { return m_applyFilter != NULL; }
	void setAutoApplyFilter(const TCHAR *filter);
   void queueUpdate();
   void queueRemoveFromNode(DWORD dwNodeId, BOOL bRemoveDCI);

   void CreateNXMPRecord(String &str);
	
	BOOL EnumDCI(BOOL (* pfCallback)(DCItem *, DWORD, void *), void *pArg);
	void associateItems();
};


//
// Cluster class
//

class NXCORE_EXPORTABLE Cluster : public Template
{
protected:
	DWORD m_dwClusterType;
   DWORD m_dwNumSyncNets;
   IP_NETWORK *m_pSyncNetList;
	DWORD m_dwNumResources;
	CLUSTER_RESOURCE *m_pResourceList;
	DWORD m_dwFlags;
	time_t m_tmLastPoll;
	DWORD m_zoneId;

public:
	Cluster();
   Cluster(const TCHAR *pszName, DWORD zoneId);
	virtual ~Cluster();

   virtual int Type() { return OBJECT_CLUSTER; }
   virtual BOOL SaveToDB(DB_HANDLE hdb);
   virtual BOOL DeleteFromDB();
   virtual BOOL CreateFromDB(DWORD dwId);

   virtual void CreateMessage(CSCPMessage *pMsg);
   virtual DWORD ModifyFromMessage(CSCPMessage *pRequest, BOOL bAlreadyLocked = FALSE);

   virtual void calculateCompoundStatus(BOOL bForcedRecalc = FALSE);

	BOOL isSyncAddr(DWORD dwAddr);
	BOOL isVirtualAddr(DWORD dwAddr);
	BOOL isResourceOnNode(DWORD dwResource, DWORD dwNode);
   DWORD getZoneId() { return m_zoneId; }

   void statusPoll(ClientSession *pSession, DWORD dwRqId, int nPoller);
   void lockForStatusPoll() { m_dwFlags |= CLF_QUEUED_FOR_STATUS_POLL; }
   BOOL isReadyForStatusPoll() 
   {
      return ((m_iStatus != STATUS_UNMANAGED) && (!m_bIsDeleted) &&
              (!(m_dwFlags & CLF_QUEUED_FOR_STATUS_POLL)) &&
              ((DWORD)time(NULL) - (DWORD)m_tmLastPoll > g_dwStatusPollingInterval))
                  ? TRUE : FALSE;
   }
};


//
// Interface class
//

class NXCORE_EXPORTABLE Interface : public NetObj
{
protected:
	DWORD m_flags;
	TCHAR m_description[MAX_DB_STRING];	// Interface description - value of ifDescr for SNMP, equals to name for NetXMS agent
   DWORD m_dwIfIndex;
   DWORD m_dwIfType;
   DWORD m_dwIpNetMask;
   BYTE m_bMacAddr[MAC_ADDR_LENGTH];
	DWORD m_bridgePortNumber;		// 802.1D port number
	DWORD m_slotNumber;				// Vendor/device specific slot number
	DWORD m_portNumber;				// Vendor/device specific port number
	DWORD m_peerNodeId;				// ID of peer node object, or 0 if unknown
	DWORD m_peerInterfaceId;		// ID of peer interface object, or 0 if unknown
	WORD m_dot1xPaeAuthState;		// 802.1x port auth state
	WORD m_dot1xBackendAuthState;	// 802.1x backend auth state
   QWORD m_qwLastDownEventId;
	int m_iPendingStatus;
	int m_iPollCount;
	int m_iRequiredPollCount;
   DWORD m_zoneId;

public:
   Interface();
   Interface(DWORD dwAddr, DWORD dwNetMask, DWORD zoneId, bool bSyntheticMask);
   Interface(const TCHAR *name, const TCHAR *descr, DWORD index, DWORD ipAddr, DWORD ipNetMask, DWORD ifType, DWORD zoneId);
   virtual ~Interface();

   virtual int Type() { return OBJECT_INTERFACE; }
   virtual BOOL SaveToDB(DB_HANDLE hdb);
   virtual BOOL DeleteFromDB();
   virtual BOOL CreateFromDB(DWORD dwId);

   Node *getParentNode();

   DWORD getZoneId() { return m_zoneId; }
   DWORD getIpNetMask() { return m_dwIpNetMask; }
   DWORD getIfIndex() { return m_dwIfIndex; }
   DWORD getIfType() { return m_dwIfType; }
	DWORD getBridgePortNumber() { return m_bridgePortNumber; }
	DWORD getSlotNumber() { return m_slotNumber; }
	DWORD getPortNumber() { return m_portNumber; }
	DWORD getPeerNodeId() { return m_peerNodeId; }
	DWORD getPeerInterfaceId() { return m_peerInterfaceId; }
	const TCHAR *getDescription() { return m_description; }
   const BYTE *getMacAddr() { return m_bMacAddr; }
	bool isSyntheticMask() { return (m_flags & IF_SYNTHETIC_MASK) ? true : false; }
	bool isPhysicalPort() { return (m_flags & IF_PHYSICAL_PORT) ? true : false; }
   bool isFake() { return (m_dwIfIndex == 1) && 
                          (m_dwIfType == IFTYPE_OTHER) &&
                          (!_tcscmp(m_szName, _T("lan0")) || !_tcscmp(m_szName, _T("unknown"))) &&
                          (!memcmp(m_bMacAddr, "\x00\x00\x00\x00\x00\x00", 6)); }

   QWORD getLastDownEventId() { return m_qwLastDownEventId; }
   void setLastDownEventId(QWORD qwId) { m_qwLastDownEventId = qwId; }

   void setMacAddr(const BYTE *pbNewMac) { memcpy(m_bMacAddr, pbNewMac, MAC_ADDR_LENGTH); Modify(); }
   void setIpAddr(DWORD dwNewAddr);
   void setIpNetMask(DWORD dwNewMask);
   void setBridgePortNumber(DWORD bpn) { m_bridgePortNumber = bpn; Modify(); }
   void setSlotNumber(DWORD slot) { m_slotNumber = slot; Modify(); }
   void setPortNumber(DWORD port) { m_portNumber = port; Modify(); }
	void setPhysicalPortFlag(bool isPhysical) { if (isPhysical) m_flags |= IF_PHYSICAL_PORT; else m_flags &= ~IF_PHYSICAL_PORT; Modify(); }
	void setPeer(DWORD nodeId, DWORD ifId) { m_peerNodeId = nodeId; m_peerInterfaceId = ifId; Modify(); }
   void setDescription(const TCHAR *descr) { nx_strncpy(m_description, descr, MAX_DB_STRING); Modify(); }

	void updateZoneId();

   void StatusPoll(ClientSession *pSession, DWORD dwRqId, Queue *pEventQueue,
	                BOOL bClusterSync, SNMP_Transport *pTransport);
   
	virtual void CreateMessage(CSCPMessage *pMsg);
   virtual DWORD ModifyFromMessage(CSCPMessage *pRequest, BOOL bAlreadyLocked = FALSE);

   DWORD wakeUp();
};


//
// Network service class
//

class NetworkService : public NetObj
{
protected:
   int m_iServiceType;   // SSH, POP3, etc.
   Node *m_pHostNode;    // Pointer to node object which hosts this service
   DWORD m_dwPollerNode; // ID of node object which is used for polling
                         // If 0, m_pHostNode->m_dwPollerNode will be used
   WORD m_wProto;        // Protocol (TCP, UDP, etc.)
   WORD m_wPort;         // TCP or UDP port number
   TCHAR *m_pszRequest;  // Service-specific request
   TCHAR *m_pszResponse; // Service-specific expected response
	int m_iPendingStatus;
	int m_iPollCount;
	int m_iRequiredPollCount;

   virtual void OnObjectDelete(DWORD dwObjectId);

public:
   NetworkService();
   NetworkService(int iServiceType, WORD wProto, WORD wPort,
                  TCHAR *pszRequest, TCHAR *pszResponse,
                  Node *pHostNode = NULL, DWORD dwPollerNode = 0);
   virtual ~NetworkService();

   virtual int Type() { return OBJECT_NETWORKSERVICE; }

   virtual BOOL SaveToDB(DB_HANDLE hdb);
   virtual BOOL DeleteFromDB();
   virtual BOOL CreateFromDB(DWORD dwId);

   void StatusPoll(ClientSession *pSession, DWORD dwRqId, Node *pPollerNode, Queue *pEventQueue);

   virtual void CreateMessage(CSCPMessage *pMsg);
   virtual DWORD ModifyFromMessage(CSCPMessage *pRequest, BOOL bAlreadyLocked = FALSE);
};


//
// VPN connector class
//

class VPNConnector : public NetObj
{
protected:
   DWORD m_dwPeerGateway;        // Object ID of peer gateway
   DWORD m_dwNumLocalNets;
   IP_NETWORK *m_pLocalNetList;
   DWORD m_dwNumRemoteNets;
   IP_NETWORK *m_pRemoteNetList;

   Node *GetParentNode();

public:
   VPNConnector();
   VPNConnector(BOOL bIsHidden);
   virtual ~VPNConnector();

   virtual int Type() { return OBJECT_VPNCONNECTOR; }

   virtual BOOL SaveToDB(DB_HANDLE hdb);
   virtual BOOL DeleteFromDB();
   virtual BOOL CreateFromDB(DWORD dwId);

   virtual void CreateMessage(CSCPMessage *pMsg);
   virtual DWORD ModifyFromMessage(CSCPMessage *pRequest, BOOL bAlreadyLocked = FALSE);

   BOOL IsLocalAddr(DWORD dwIpAddr);
   BOOL IsRemoteAddr(DWORD dwIpAddr);
   DWORD GetPeerGatewayAddr();
};


//
// Node
//

class NXCORE_EXPORTABLE Node : public Template
{
protected:
	TCHAR m_primaryName[MAX_DNS_NAME];
   DWORD m_dwFlags;
   DWORD m_dwDynamicFlags;       // Flags used at runtime by server
	int m_iPendingStatus;
	int m_iPollCount;
	int m_iRequiredPollCount;
   DWORD m_zoneId;
   WORD m_wAgentPort;
   WORD m_wAuthMethod;
   TCHAR m_szSharedSecret[MAX_SECRET_LENGTH];
   int m_iStatusPollType;
   int m_snmpVersion;
   WORD m_wSNMPPort;
	WORD m_nUseIfXTable;
	SNMP_SecurityContext *m_snmpSecurity;
   TCHAR m_szObjectId[MAX_OID_LEN * 4];
   TCHAR m_szAgentVersion[MAX_AGENT_VERSION_LEN];
   TCHAR m_szPlatformName[MAX_PLATFORM_NAME_LEN];
	TCHAR *m_sysDescription;  // Agent's System.Uname or SNMP sysDescr
	TCHAR *m_sysName;				// SNMP sysName
	TCHAR *m_lldpNodeId;			// lldpLocChassisId combined with lldpLocChassisIdSubtype, or NULL for non-LLDP nodes
	NetworkDeviceDriver *m_driver;
   DWORD m_dwNumParams;           // Number of elements in supported parameters list
   NXC_AGENT_PARAM *m_pParamList; // List of supported parameters
   time_t m_tLastDiscoveryPoll;
   time_t m_tLastStatusPoll;
   time_t m_tLastConfigurationPoll;
	time_t m_tLastTopologyPoll;
   time_t m_tLastRTUpdate;
   time_t m_tFailTimeSNMP;
   time_t m_tFailTimeAgent;
   MUTEX m_hPollerMutex;
   MUTEX m_hAgentAccessMutex;
   MUTEX m_mutexRTAccess;
	MUTEX m_mutexTopoAccess;
   AgentConnectionEx *m_pAgentConnection;
   DWORD m_dwPollerNode;      // Node used for network service polling
   DWORD m_dwProxyNode;       // Node used as proxy for agent connection
	DWORD m_dwSNMPProxy;			// Node used as proxy for SNMP requests
   QWORD m_qwLastEvents[MAX_LAST_EVENTS];
   ROUTING_TABLE *m_pRoutingTable;
	ForwardingDatabase *m_fdb;
	LinkLayerNeighbors *m_linkLayerNeighbors;
	VlanList *m_vlans;
	VrrpInfo *m_vrrpInfo;
	BYTE m_baseBridgeAddress[MAC_ADDR_LENGTH];	// Bridge base address (dot1dBaseBridgeAddress in bridge MIB)
	nxmap_ObjList *m_pTopology;	// For compatibility, to be removed in 1.1.x
	time_t m_topologyRebuildTimestamp;
	ServerJobQueue *m_jobQueue;

   void pollerLock() { MutexLock(m_hPollerMutex, INFINITE); }
   void pollerUnlock() { MutexUnlock(m_hPollerMutex); }

   void agentLock() { MutexLock(m_hAgentAccessMutex, INFINITE); }
   void agentUnlock() { MutexUnlock(m_hAgentAccessMutex); }

   void routingTableLock() { MutexLock(m_mutexRTAccess, INFINITE); }
   void routingTableUnlock() { MutexUnlock(m_mutexRTAccess); }

   BOOL CheckSNMPIntegerValue(SNMP_Transport *pTransport, const TCHAR *pszOID, int nValue);
   void CheckOSPFSupport(SNMP_Transport *pTransport);
	void addVrrpInterfaces(InterfaceList *ifList);
	BOOL resolveName(BOOL useOnlyDNS);
   void setAgentProxy(AgentConnection *pConn);

   DWORD getInterfaceCount(Interface **ppInterface);

   void checkInterfaceNames(InterfaceList *pIfList);
	void checkSubnetBinding(InterfaceList *pIfList);
	void checkAgentPolicyBinding(AgentConnection *conn);

	void ApplySystemTemplates();
	void ApplyUserTemplates();

	void updateContainerMembership();
	BOOL updateInterfaceConfiguration(DWORD dwRqId, DWORD dwNetMask);

   virtual void PrepareForDeletion();
   virtual void OnObjectDelete(DWORD dwObjectId);

public:
   Node();
   Node(DWORD dwAddr, DWORD dwFlags, DWORD dwProxyNode, DWORD dwSNMPProxy, DWORD dwZone);
   virtual ~Node();

   virtual int Type() { return OBJECT_NODE; }

   virtual BOOL SaveToDB(DB_HANDLE hdb);
   virtual BOOL DeleteFromDB();
   virtual BOOL CreateFromDB(DWORD dwId);

	TCHAR *expandText(const TCHAR *pszTemplate);

	Cluster *getMyCluster();

   DWORD getFlags() { return m_dwFlags; }
   DWORD getRuntimeFlags() { return m_dwDynamicFlags; }
   DWORD getZoneId() { return m_zoneId; }
   void setLocalMgmtFlag() { m_dwFlags |= NF_IS_LOCAL_MGMT; }
   void clearLocalMgmtFlag() { m_dwFlags &= ~NF_IS_LOCAL_MGMT; }

   bool isSNMPSupported() { return m_dwFlags & NF_IS_SNMP ? true : false; }
   bool isNativeAgent() { return m_dwFlags & NF_IS_NATIVE_AGENT ? true : false; }
   bool isBridge() { return m_dwFlags & NF_IS_BRIDGE ? true : false; }
   bool isRouter() { return m_dwFlags & NF_IS_ROUTER ? true : false; }
   bool isLocalManagement() { return m_dwFlags & NF_IS_LOCAL_MGMT ? true : false; }

	LONG getSNMPVersion() { return m_snmpVersion; }
	const TCHAR *getSNMPObjectId() { return m_szObjectId; }
	const TCHAR *getAgentVersion() { return m_szAgentVersion; }
	const TCHAR *getPlatformName() { return m_szPlatformName; }
   const TCHAR *getObjectId() { return m_szObjectId; }
	const TCHAR *getSysName() { return CHECK_NULL_EX(m_sysName); }
	const TCHAR *getSysDescription() { return CHECK_NULL_EX(m_sysDescription); }
	const TCHAR *getLLDPNodeId() { return m_lldpNodeId; }

   BOOL isDown() { return m_dwDynamicFlags & NDF_UNREACHABLE ? TRUE : FALSE; }

   void addInterface(Interface *pInterface) { AddChild(pInterface); pInterface->AddParent(this); }
   void createNewInterface(DWORD dwAddr, DWORD dwNetMask, const TCHAR *name = NULL, const TCHAR *descr = NULL,
                           DWORD dwIndex = 0, DWORD dwType = 0, BYTE *pbMacAddr = NULL, DWORD bridgePort = 0,
									DWORD slot = 0, DWORD port = 0, bool physPort = false);
   void deleteInterface(Interface *pInterface);

   void changeIPAddress(DWORD dwIpAddr);
	void changeZone(DWORD newZone);

   ARP_CACHE *getArpCache();
   InterfaceList *getInterfaceList();
   Interface *findInterface(DWORD dwIndex, DWORD dwHostAddr);
   Interface *findInterface(const TCHAR *name);
	Interface *findInterfaceByMAC(const BYTE *macAddr);
	Interface *findInterfaceByIP(DWORD ipAddr);
	Interface *findInterfaceBySlotAndPort(DWORD slot, DWORD port);
	Interface *findBridgePort(DWORD bridgePortNumber);
	BOOL isMyIP(DWORD dwIpAddr);
   int getInterfaceStatusFromSNMP(SNMP_Transport *pTransport, DWORD dwIndex);
   int getInterfaceStatusFromAgent(DWORD dwIndex);
   ROUTING_TABLE *getRoutingTable();
   ROUTING_TABLE *getCachedRoutingTable() { return m_pRoutingTable; }
	LinkLayerNeighbors *getLinkLayerNeighbors();
	VlanList *getVlans();
   BOOL getNextHop(DWORD dwSrcAddr, DWORD dwDestAddr, DWORD *pdwNextHop,
                   DWORD *pdwIfIndex, BOOL *pbIsVPN);

	void setRecheckCapsFlag() { m_dwDynamicFlags |= NDF_RECHECK_CAPABILITIES; }
   void setDiscoveryPollTimeStamp();
   void statusPoll(ClientSession *pSession, DWORD dwRqId, int nPoller);
   void configurationPoll(ClientSession *pSession, DWORD dwRqId, int nPoller, DWORD dwNetMask);
	void topologyPoll(ClientSession *pSession, DWORD dwRqId, int nPoller);
	void resolveVlanPorts(VlanList *vlanList);
	void updateInterfaceNames(ClientSession *pSession, DWORD dwRqId);
   void updateRoutingTable();
   bool isReadyForStatusPoll();
   bool isReadyForConfigurationPoll();
   bool isReadyForDiscoveryPoll();
   bool isReadyForRoutePoll();
   bool isReadyForTopologyPoll();
   void lockForStatusPoll();
   void lockForConfigurationPoll();
   void lockForDiscoveryPoll();
   void lockForRoutePoll();
   void lockForTopologyPoll();
	void forceConfigurationPoll() { m_dwDynamicFlags |= NDF_FORCE_CONFIGURATION_POLL; }

   virtual void calculateCompoundStatus(BOOL bForcedRecalc = FALSE);

   BOOL connectToAgent(DWORD *error = NULL, DWORD *socketError = NULL);
   DWORD GetItemFromSNMP(WORD port, const TCHAR *szParam, DWORD dwBufSize, TCHAR *szBuffer);
   DWORD GetItemFromCheckPointSNMP(const TCHAR *szParam, DWORD dwBufSize, TCHAR *szBuffer);
   DWORD GetItemFromAgent(const TCHAR *szParam, DWORD dwBufSize, TCHAR *szBuffer);
   DWORD GetInternalItem(const TCHAR *szParam, DWORD dwBufSize, TCHAR *szBuffer);
	DWORD getTableFromAgent(const TCHAR *name, Table **table);
   void queueItemsForPolling(Queue *pPollerQueue);
   DWORD getItemForClient(int iOrigin, const TCHAR *pszParam, TCHAR *pszBuffer, DWORD dwBufSize);
   DWORD getTableForClient(const TCHAR *name, Table **table);
   DWORD getLastValues(CSCPMessage *pMsg);
	void processNewDciValue(DCItem *item, time_t currTime, const TCHAR *value);
   void cleanDCIData();
   BOOL applyTemplateItem(DWORD dwTemplateId, DCItem *pItem);
   void cleanDeletedTemplateItems(DWORD dwTemplateId, DWORD dwNumItems, DWORD *pdwItemList);
   void unbindFromTemplate(DWORD dwTemplateId, BOOL bRemoveDCI);
   void updateDciCache();
	DWORD getPerfTabDCIList(CSCPMessage *pMsg);
	NXSL_Array *getParentsForNXSL();

   void openParamList(DWORD *pdwNumParams, NXC_AGENT_PARAM **ppParamList);
   void closeParamList() { UnlockData(); }

   AgentConnectionEx *createAgentConnection();
	SNMP_Transport *createSnmpTransport(WORD port = 0);
	SNMP_SecurityContext *getSnmpSecurityContext();

   virtual void CreateMessage(CSCPMessage *pMsg);
   virtual DWORD ModifyFromMessage(CSCPMessage *pRequest, BOOL bAlreadyLocked = FALSE);
   void WriteParamListToMessage(CSCPMessage *pMsg);

   DWORD wakeUp();

   void AddService(NetworkService *pNetSrv) { AddChild(pNetSrv); pNetSrv->AddParent(this); }
   DWORD CheckNetworkService(DWORD *pdwStatus, DWORD dwIpAddr, int iServiceType, WORD wPort = 0,
                             WORD wProto = 0, TCHAR *pszRequest = NULL, TCHAR *pszResponse = NULL);

   QWORD GetLastEventId(int nIndex) { return ((nIndex >= 0) && (nIndex < MAX_LAST_EVENTS)) ? m_qwLastEvents[nIndex] : 0; }
   void SetLastEventId(int nIndex, QWORD qwId) { if ((nIndex >= 0) && (nIndex < MAX_LAST_EVENTS)) m_qwLastEvents[nIndex] = qwId; }

   DWORD CallSnmpEnumerate(const TCHAR *pszRootOid, 
      DWORD (* pHandler)(DWORD, SNMP_Variable *, SNMP_Transport *, void *), void *pArg);

	nxmap_ObjList *GetL2Topology();
	nxmap_ObjList *BuildL2Topology(DWORD *pdwStatus);
	ForwardingDatabase *getSwitchForwardingDatabase();
	Interface *findConnectionPoint(DWORD *localIfId, BYTE *localMacAddr);

	ServerJobQueue *getJobQueue() { return m_jobQueue; }
};


//
// Inline functions for Node class
//

inline void Node::setDiscoveryPollTimeStamp()
{
   m_tLastDiscoveryPoll = time(NULL);
   m_dwDynamicFlags &= ~NDF_QUEUED_FOR_DISCOVERY_POLL;
}

inline bool Node::isReadyForStatusPoll() 
{
	if (m_bIsDeleted)
		return false;
	//if (GetMyCluster() != NULL)
	//	return FALSE;	// Cluster nodes should be polled from cluster status poll
   if (m_dwDynamicFlags & NDF_FORCE_STATUS_POLL)
   {
      m_dwDynamicFlags &= ~NDF_FORCE_STATUS_POLL;
      return true;
   }
   return (m_iStatus != STATUS_UNMANAGED) &&
	       (!(m_dwFlags & NF_DISABLE_STATUS_POLL)) &&
          (!(m_dwDynamicFlags & NDF_QUEUED_FOR_STATUS_POLL)) &&
          (!(m_dwDynamicFlags & NDF_POLLING_DISABLED)) &&
			 (getMyCluster() == NULL) &&
          ((DWORD)time(NULL) - (DWORD)m_tLastStatusPoll > g_dwStatusPollingInterval);
}

inline bool Node::isReadyForConfigurationPoll() 
{ 
	if (m_bIsDeleted)
		return false;
   if (m_dwDynamicFlags & NDF_FORCE_CONFIGURATION_POLL)
   {
      m_dwDynamicFlags &= ~NDF_FORCE_CONFIGURATION_POLL;
      return true;
   }
   return (m_iStatus != STATUS_UNMANAGED) &&
	       (!(m_dwFlags & NF_DISABLE_CONF_POLL)) &&
          (!(m_dwDynamicFlags & NDF_QUEUED_FOR_CONFIG_POLL)) &&
          (!(m_dwDynamicFlags & NDF_POLLING_DISABLED)) &&
          ((DWORD)time(NULL) - (DWORD)m_tLastConfigurationPoll > g_dwConfigurationPollingInterval);
}

inline bool Node::isReadyForDiscoveryPoll() 
{ 
	if (m_bIsDeleted)
		return false;
   return (g_dwFlags & AF_ENABLE_NETWORK_DISCOVERY) &&
          (m_iStatus != STATUS_UNMANAGED) &&
          (!(m_dwDynamicFlags & NDF_QUEUED_FOR_DISCOVERY_POLL)) &&
          (!(m_dwDynamicFlags & NDF_POLLING_DISABLED)) &&
          (m_dwDynamicFlags & NDF_CONFIGURATION_POLL_PASSED) &&
          ((DWORD)time(NULL) - (DWORD)m_tLastDiscoveryPoll > g_dwDiscoveryPollingInterval);
}

inline bool Node::isReadyForRoutePoll() 
{ 
	if (m_bIsDeleted)
		return false;
   return (m_iStatus != STATUS_UNMANAGED) &&
	       (!(m_dwFlags & NF_DISABLE_ROUTE_POLL)) &&
          (!(m_dwDynamicFlags & NDF_QUEUED_FOR_ROUTE_POLL)) &&
          (!(m_dwDynamicFlags & NDF_POLLING_DISABLED)) &&
          ((DWORD)time(NULL) - (DWORD)m_tLastRTUpdate > g_dwRoutingTableUpdateInterval);
}

inline bool Node::isReadyForTopologyPoll() 
{ 
	if (m_bIsDeleted)
		return false;
   return (m_iStatus != STATUS_UNMANAGED) &&
	       (!(m_dwFlags & NF_DISABLE_TOPOLOGY_POLL)) &&
          (!(m_dwDynamicFlags & NDF_QUEUED_FOR_TOPOLOGY_POLL)) &&
          (!(m_dwDynamicFlags & NDF_POLLING_DISABLED)) &&
          ((DWORD)time(NULL) - (DWORD)m_tLastTopologyPoll > g_dwTopologyPollingInterval);
}

inline void Node::lockForStatusPoll()
{ 
   LockData(); 
   m_dwDynamicFlags |= NDF_QUEUED_FOR_STATUS_POLL; 
   UnlockData(); 
}

inline void Node::lockForConfigurationPoll() 
{ 
   LockData(); 
   m_dwDynamicFlags |= NDF_QUEUED_FOR_CONFIG_POLL; 
   UnlockData(); 
}

inline void Node::lockForDiscoveryPoll() 
{ 
   LockData(); 
   m_dwDynamicFlags |= NDF_QUEUED_FOR_DISCOVERY_POLL; 
   UnlockData(); 
}

inline void Node::lockForTopologyPoll() 
{ 
   LockData(); 
   m_dwDynamicFlags |= NDF_QUEUED_FOR_TOPOLOGY_POLL; 
   UnlockData(); 
}

inline void Node::lockForRoutePoll() 
{ 
   LockData(); 
   m_dwDynamicFlags |= NDF_QUEUED_FOR_ROUTE_POLL; 
   UnlockData(); 
}


//
// Subnet
//

class NXCORE_EXPORTABLE Subnet : public NetObj
{
protected:
   DWORD m_dwIpNetMask;
   DWORD m_zoneId;
	bool m_bSyntheticMask;

public:
   Subnet();
   Subnet(DWORD dwAddr, DWORD dwNetMask, DWORD dwZone, bool bSyntheticMask);
   virtual ~Subnet();

   virtual int Type() { return OBJECT_SUBNET; }

   virtual BOOL SaveToDB(DB_HANDLE hdb);
   virtual BOOL DeleteFromDB(void);
   virtual BOOL CreateFromDB(DWORD dwId);

   void AddNode(Node *pNode) { AddChild(pNode); pNode->AddParent(this); }
   virtual void CreateMessage(CSCPMessage *pMsg);

   DWORD getIpNetMask() { return m_dwIpNetMask; }
   DWORD getZoneId() { return m_zoneId; }
	bool isSyntheticMask() { return m_bSyntheticMask; }

	void setCorrectMask(DWORD dwAddr, DWORD dwMask);
};


//
// Universal root object
//

class NXCORE_EXPORTABLE UniversalRoot : public NetObj
{
public:
   UniversalRoot();
   virtual ~UniversalRoot();

   virtual BOOL SaveToDB(DB_HANDLE hdb);
   virtual void LoadFromDB();

   void LinkChildObjects();
   void LinkObject(NetObj *pObject) { AddChild(pObject); pObject->AddParent(this); }
};


//
// Service root
//

class NXCORE_EXPORTABLE ServiceRoot : public UniversalRoot
{
public:
   ServiceRoot();
   virtual ~ServiceRoot();

   virtual int Type() { return OBJECT_SERVICEROOT; }
};


//
// Template root
//

class NXCORE_EXPORTABLE TemplateRoot : public UniversalRoot
{
public:
   TemplateRoot();
   virtual ~TemplateRoot();

   virtual int Type() { return OBJECT_TEMPLATEROOT; }
   virtual void calculateCompoundStatus(BOOL bForcedRecalc = FALSE);
};


//
// Generic container object
//

class NXCORE_EXPORTABLE Container : public NetObj
{
private:
   DWORD *m_pdwChildIdList;
   DWORD m_dwChildIdListSize;

protected:
   DWORD m_dwCategory;
	NXSL_Program *m_bindFilter;
	TCHAR *m_bindFilterSource;

public:
   Container();
   Container(const TCHAR *pszName, DWORD dwCategory);
   virtual ~Container();

   virtual int Type(void) { return OBJECT_CONTAINER; }
  
   virtual BOOL SaveToDB(DB_HANDLE hdb);
   virtual BOOL DeleteFromDB(void);
   virtual BOOL CreateFromDB(DWORD dwId);

   virtual void CreateMessage(CSCPMessage *pMsg);
   virtual DWORD ModifyFromMessage(CSCPMessage *pRequest, BOOL bAlreadyLocked = FALSE);

   DWORD getCategory() { return m_dwCategory; }

   void linkChildObjects();
   void linkObject(NetObj *pObject) { AddChild(pObject); pObject->AddParent(this); }

	bool isSuitableForNode(Node *node);
	bool isAutoBindEnabled() { return m_bindFilter != NULL; }

	void setAutoBindFilter(const TCHAR *script);
};


//
// Template group object
//

class NXCORE_EXPORTABLE TemplateGroup : public Container
{
public:
   TemplateGroup() : Container() { }
   TemplateGroup(TCHAR *pszName) : Container(pszName, 0) { }
   virtual ~TemplateGroup() { }

   virtual int Type() { return OBJECT_TEMPLATEGROUP; }
   virtual void calculateCompoundStatus(BOOL bForcedRecalc = FALSE);
};


//
// Zone object
//

class Zone : public NetObj
{
protected:
   DWORD m_zoneId;
   DWORD m_agentProxy;
   DWORD m_snmpProxy;
	DWORD m_icmpProxy;
	ObjectIndex *m_idxInterfaceByAddr;
	ObjectIndex *m_idxSubnetByAddr;

public:
   Zone();
   Zone(DWORD zoneId, const TCHAR *name);
   virtual ~Zone();

   virtual int Type() { return OBJECT_ZONE; }

   virtual BOOL SaveToDB(DB_HANDLE hdb);
   virtual BOOL DeleteFromDB();
   virtual BOOL CreateFromDB(DWORD dwId);

   virtual void CreateMessage(CSCPMessage *pMsg);
   virtual DWORD ModifyFromMessage(CSCPMessage *pRequest, BOOL bAlreadyLocked = FALSE);

   DWORD getZoneId() { return m_zoneId; }
	DWORD getAgentProxy() { return m_agentProxy; }
	DWORD getSnmpProxy() { return m_snmpProxy; }
	DWORD getIcmpProxy() { return m_icmpProxy; }

   void addSubnet(Subnet *pSubnet) { AddChild(pSubnet); pSubnet->AddParent(this); }

	void addToIndex(Subnet *subnet) { m_idxSubnetByAddr->put(subnet->IpAddr(), subnet); }
	void addToIndex(Interface *iface) { m_idxInterfaceByAddr->put(iface->IpAddr(), iface); }
	void removeFromIndex(Subnet *subnet) { m_idxSubnetByAddr->remove(subnet->IpAddr()); }
	void removeFromIndex(Interface *iface) { m_idxInterfaceByAddr->remove(iface->IpAddr()); }
	void updateInterfaceIndex(DWORD oldIp, DWORD newIp, Interface *iface);
	Subnet *getSubnetByAddr(DWORD ipAddr) { return(Subnet *) m_idxSubnetByAddr->get(ipAddr); }
	Interface *getInterfaceByAddr(DWORD ipAddr) { return (Interface *)m_idxInterfaceByAddr->get(ipAddr); }
	Subnet *findSubnet(bool (*comparator)(NetObj *, void *), void *data) { return (Subnet *)m_idxSubnetByAddr->find(comparator, data); }
	Interface *findInterface(bool (*comparator)(NetObj *, void *), void *data) { return (Interface *)m_idxInterfaceByAddr->find(comparator, data); }
};


//
// Entire network
//

class NXCORE_EXPORTABLE Network : public NetObj
{
public:
   Network();
   virtual ~Network();

   virtual int Type(void) { return OBJECT_NETWORK; }
   virtual BOOL SaveToDB(DB_HANDLE hdb);

   void AddSubnet(Subnet *pSubnet) { AddChild(pSubnet); pSubnet->AddParent(this); }
   void AddZone(Zone *pZone) { AddChild(pZone); pZone->AddParent(this); }
   void LoadFromDB(void);
};


//
// Condition
//

class NXCORE_EXPORTABLE Condition : public NetObj
{
protected:
   DWORD m_dwDCICount;
   INPUT_DCI *m_pDCIList;
   TCHAR *m_pszScript;
   NXSL_Program *m_pCompiledScript;
   DWORD m_dwActivationEventCode;
   DWORD m_dwDeactivationEventCode;
   DWORD m_dwSourceObject;
   int m_nActiveStatus;
   int m_nInactiveStatus;
   BOOL m_bIsActive;
   time_t m_tmLastPoll;
   BOOL m_bQueuedForPolling;

public:
   Condition();
   Condition(BOOL bHidden);
   virtual ~Condition();

   virtual int Type() { return OBJECT_CONDITION; }

   virtual BOOL SaveToDB(DB_HANDLE hdb);
   virtual BOOL DeleteFromDB(void);
   virtual BOOL CreateFromDB(DWORD dwId);

   virtual void CreateMessage(CSCPMessage *pMsg);
   virtual DWORD ModifyFromMessage(CSCPMessage *pRequest, BOOL bAlreadyLocked = FALSE);

   void check();

   void LockForPoll(void);
   void EndPoll(void);

   BOOL ReadyForPoll(void) 
   {
      return ((m_iStatus != STATUS_UNMANAGED) && 
              (!m_bQueuedForPolling) && (!m_bIsDeleted) &&
              ((DWORD)time(NULL) - (DWORD)m_tmLastPoll > g_dwConditionPollingInterval))
                  ? TRUE : FALSE;
   }

   int getCacheSizeForDCI(DWORD dwItemId, BOOL bNoLock);
};


//
// Generic agent policy object
//

class NXCORE_EXPORTABLE AgentPolicy : public NetObj
{
protected:
	DWORD m_version;
	int m_policyType;
	TCHAR *m_description;

	BOOL savePolicyCommonProperties(DB_HANDLE hdb);

public:
   AgentPolicy(int type);
   AgentPolicy(const TCHAR *name, int type);
   virtual ~AgentPolicy();

   virtual int Type() { return OBJECT_AGENTPOLICY; }

   virtual BOOL SaveToDB(DB_HANDLE hdb);
   virtual BOOL DeleteFromDB();
   virtual BOOL CreateFromDB(DWORD dwId);

   virtual void CreateMessage(CSCPMessage *pMsg);
   virtual DWORD ModifyFromMessage(CSCPMessage *pRequest, BOOL bAlreadyLocked = FALSE);

	virtual bool createDeploymentMessage(CSCPMessage *msg);
	virtual bool createUninstallMessage(CSCPMessage *msg);

	void linkNode(Node *node) { AddChild(node); node->AddParent(this); }
	void unlinkNode(Node *node) { DeleteChild(node); node->DeleteParent(this); }
};


//
// Agent config policy object
//

class NXCORE_EXPORTABLE AgentPolicyConfig : public AgentPolicy
{
protected:
	TCHAR *m_fileContent;

public:
   AgentPolicyConfig();
   AgentPolicyConfig(const TCHAR *name);
   virtual ~AgentPolicyConfig();

   virtual int Type() { return OBJECT_AGENTPOLICY_CONFIG; }

   virtual BOOL SaveToDB(DB_HANDLE hdb);
   virtual BOOL DeleteFromDB();
   virtual BOOL CreateFromDB(DWORD dwId);

   virtual void CreateMessage(CSCPMessage *pMsg);
   virtual DWORD ModifyFromMessage(CSCPMessage *pRequest, BOOL bAlreadyLocked = FALSE);

	virtual bool createDeploymentMessage(CSCPMessage *msg);
	virtual bool createUninstallMessage(CSCPMessage *msg);
};


//
// Policy group object
//

class NXCORE_EXPORTABLE PolicyGroup : public Container
{
public:
   PolicyGroup() : Container() { }
   PolicyGroup(const TCHAR *pszName) : Container(pszName, 0) { }
   virtual ~PolicyGroup() { }

   virtual int Type() { return OBJECT_POLICYGROUP; }
   virtual void calculateCompoundStatus(BOOL bForcedRecalc = FALSE);
};


//
// Policy root
//

class NXCORE_EXPORTABLE PolicyRoot : public UniversalRoot
{
public:
   PolicyRoot();
   virtual ~PolicyRoot();

   virtual int Type() { return OBJECT_POLICYROOT; }
   virtual void calculateCompoundStatus(BOOL bForcedRecalc = FALSE);
};


//
// Network map root
//

class NXCORE_EXPORTABLE NetworkMapRoot : public UniversalRoot
{
public:
   NetworkMapRoot();
   virtual ~NetworkMapRoot();

   virtual int Type() { return OBJECT_NETWORKMAPROOT; }
   virtual void calculateCompoundStatus(BOOL bForcedRecalc = FALSE);
};


//
// Network map group object
//

class NXCORE_EXPORTABLE NetworkMapGroup : public Container
{
public:
   NetworkMapGroup() : Container() { }
   NetworkMapGroup(const TCHAR *pszName) : Container(pszName, 0) { }
   virtual ~NetworkMapGroup() { }

   virtual int Type() { return OBJECT_NETWORKMAPGROUP; }
   virtual void calculateCompoundStatus(BOOL bForcedRecalc = FALSE);
};


//
// Network map object
//

class NXCORE_EXPORTABLE NetworkMap : public NetObj
{
protected:
	int m_mapType;
	DWORD m_seedObject;
	int m_layout;
	uuid_t m_background;
	double m_backgroundLatitude;
	double m_backgroundLongitude;
	int m_backgroundZoom;
	int m_numElements;
	NetworkMapElement **m_elements;
	int m_numLinks;
	NetworkMapLink **m_links;

public:
   NetworkMap();
	NetworkMap(int type, DWORD seed);
   virtual ~NetworkMap();

   virtual int Type() { return OBJECT_NETWORKMAP; }
   virtual void calculateCompoundStatus(BOOL bForcedRecalc = FALSE);

	virtual BOOL SaveToDB(DB_HANDLE hdb);
   virtual BOOL DeleteFromDB();
   virtual BOOL CreateFromDB(DWORD dwId);

   virtual void CreateMessage(CSCPMessage *pMsg);
   virtual DWORD ModifyFromMessage(CSCPMessage *pRequest, BOOL bAlreadyLocked = FALSE);
};


//
// Dashboard tree root
//

class NXCORE_EXPORTABLE DashboardRoot : public UniversalRoot
{
public:
   DashboardRoot();
   virtual ~DashboardRoot();

   virtual int Type() { return OBJECT_DASHBOARDROOT; }
   virtual void calculateCompoundStatus(BOOL bForcedRecalc = FALSE);
};


//
// Dashboard object
//

class DashboardElement
{
public:
	int m_type;
	TCHAR *m_data;
	TCHAR *m_layout;

	DashboardElement() { m_data = NULL; m_layout = NULL; }
	~DashboardElement() { safe_free(m_data); safe_free(m_layout); }
};

class NXCORE_EXPORTABLE Dashboard : public Container
{
protected:
	int m_numColumns;
	ObjectArray<DashboardElement> *m_elements;

public:
   Dashboard();
   Dashboard(const TCHAR *name);
   virtual ~Dashboard();

   virtual int Type() { return OBJECT_DASHBOARD; }
   virtual void calculateCompoundStatus(BOOL bForcedRecalc = FALSE);

	virtual BOOL SaveToDB(DB_HANDLE hdb);
   virtual BOOL DeleteFromDB();
   virtual BOOL CreateFromDB(DWORD dwId);

   virtual void CreateMessage(CSCPMessage *pMsg);
   virtual DWORD ModifyFromMessage(CSCPMessage *pRequest, BOOL bAlreadyLocked = FALSE);
};


//
// Report root
//

class NXCORE_EXPORTABLE ReportRoot : public UniversalRoot
{
public:
   ReportRoot();
   virtual ~ReportRoot();

   virtual int Type() { return OBJECT_REPORTROOT; }
   virtual void calculateCompoundStatus(BOOL bForcedRecalc = FALSE);
};


//
// Report group object
//

class NXCORE_EXPORTABLE ReportGroup : public Container
{
public:
   ReportGroup() : Container() { }
   ReportGroup(const TCHAR *pszName) : Container(pszName, 0) { }
   virtual ~ReportGroup() { }

   virtual int Type() { return OBJECT_REPORTGROUP; }
   virtual void calculateCompoundStatus(BOOL bForcedRecalc = FALSE);
};


//
// Report object
//

class NXCORE_EXPORTABLE Report : public NetObj
{
protected:
	TCHAR *m_definition;

public:
   Report();
   Report(const TCHAR *name);
   virtual ~Report();

   virtual int Type() { return OBJECT_REPORT; }
   virtual void calculateCompoundStatus(BOOL bForcedRecalc = FALSE);

	virtual BOOL SaveToDB(DB_HANDLE hdb);
   virtual BOOL DeleteFromDB();
   virtual BOOL CreateFromDB(DWORD dwId);

   virtual void CreateMessage(CSCPMessage *pMsg);
   virtual DWORD ModifyFromMessage(CSCPMessage *pRequest, BOOL bAlreadyLocked = FALSE);

	const TCHAR *getDefinition() { return CHECK_NULL_EX(m_definition); }

	DWORD execute(StringMap *parameters, DWORD userId);
};



//
// SLM check object
//

class NXCORE_EXPORTABLE SlmCheck : public NetObj
{
protected:
	Threshold *m_threshold;
	enum CheckType { check_undefined = 0, check_script = 1, check_threshold = 2 } m_type;
	TCHAR *m_script;
	NXSL_Program *m_pCompiledScript;
	TCHAR m_reason[256];
	bool m_isTemplate;
	DWORD m_currentTicketId;

	void setScript(const TCHAR *script);
	DWORD getOwnerId();
	NXSL_Value *getNodeObjectForNXSL();
	bool insertTicket();
	void closeTicket();
	void setReason(const TCHAR *reason) { nx_strncpy(m_reason, CHECK_NULL_EX(reason), 256); }

public:
	SlmCheck();
	SlmCheck(const TCHAR *name);
	virtual ~SlmCheck();

	virtual int Type() { return OBJECT_SLMCHECK; }

	virtual BOOL SaveToDB(DB_HANDLE hdb);
	virtual BOOL DeleteFromDB();
	virtual BOOL CreateFromDB(DWORD dwId);

	virtual void CreateMessage(CSCPMessage *pMsg);
	virtual DWORD ModifyFromMessage(CSCPMessage *pRequest, BOOL bAlreadyLocked = FALSE);

	void execute();
	bool isTemplate() const { return m_isTemplate; }
	const TCHAR *getReason() { return m_reason; }
};

//
// Service container - common logic for BusinessService, NodeLink and BusinessServiceRoot
//

class NXCORE_EXPORTABLE ServiceContainer: public Container 
{
	enum Period { DAY, WEEK, MONTH };

protected:
	time_t m_prevUptimeUpdateTime;
	int m_prevUptimeUpdateStatus;
	double m_uptimeDay;
	double m_uptimeWeek;
	double m_uptimeMonth;
	LONG m_downtimeDay;
	LONG m_downtimeWeek;
	LONG m_downtimeMonth;
	LONG m_prevDiffDay;
	LONG m_prevDiffWeek;
	LONG m_prevDiffMonth;

	static LONG logRecordId;
	static const LONG secondsInDay = 3600 * 24;
	static const LONG secondsInWeek = 3600 * 24 * 7;
	static LONG getSecondsInMonth();
	static LONG getSecondsInPeriod(Period period) { return period == MONTH ? getSecondsInMonth() : (period == WEEK ? secondsInWeek : secondsInDay); }
	static LONG getSecondsSinceBeginningOf(Period period, time_t *beginTime = NULL);

	void initServiceContainer();
	BOOL addHistoryRecord();
	void initUptimeStats();
	void updateUptimeStats();
	double getUptimeFromDBFor(Period period, LONG *downtime);

public:
	ServiceContainer();
	ServiceContainer(const TCHAR *pszName, DWORD dwCategory);
	virtual void calculateCompoundStatus(BOOL bForcedRecalc = FALSE);
	virtual void setStatus(int newStatus);
};

//
// Business service root
//

class NXCORE_EXPORTABLE BusinessServiceRoot : public UniversalRoot
{
public:
	BusinessServiceRoot();
	virtual ~BusinessServiceRoot();

	virtual void calculateCompoundStatus(BOOL bForcedRecalc = FALSE) { UniversalRoot::calculateCompoundStatus(bForcedRecalc); }
	virtual BOOL SaveToDB(DB_HANDLE hdb) { return UniversalRoot::SaveToDB(hdb); }
	virtual int Type() { return OBJECT_BUSINESSSERVICEROOT; }
};


//
// Business service object
//

class NXCORE_EXPORTABLE BusinessService : public ServiceContainer
{
protected:
	bool m_busy;
	time_t m_lastPollTime;
	int m_lastPollStatus;

public:
	BusinessService();
	BusinessService(const TCHAR *name);
	virtual ~BusinessService();

	virtual int Type() { return OBJECT_BUSINESSSERVICE; }
	virtual void calculateCompoundStatus(BOOL bForcedRecalc = FALSE);

	virtual BOOL SaveToDB(DB_HANDLE hdb);
	virtual BOOL DeleteFromDB();
	virtual BOOL CreateFromDB(DWORD dwId);

	virtual void CreateMessage(CSCPMessage *pMsg);
	virtual DWORD ModifyFromMessage(CSCPMessage *pRequest, BOOL bAlreadyLocked = FALSE);

	bool isReadyForPolling();
	void lockForPolling();
	void poll(ClientSession *pSession, DWORD dwRqId, int nPoller);
};

//
// Node link object for business service
//

class NXCORE_EXPORTABLE NodeLink : public ServiceContainer
{
protected:
	DWORD m_nodeId;

public:
	NodeLink();
	NodeLink(const TCHAR *name, DWORD nodeId);
	virtual ~NodeLink();

	virtual int Type() { return OBJECT_NODELINK; }
	virtual void calculateCompoundStatus(BOOL bForcedRecalc = FALSE);

	virtual BOOL SaveToDB(DB_HANDLE hdb);
	virtual BOOL DeleteFromDB();
	virtual BOOL CreateFromDB(DWORD dwId);

	virtual void CreateMessage(CSCPMessage *pMsg);
	virtual DWORD ModifyFromMessage(CSCPMessage *pRequest, BOOL bAlreadyLocked = FALSE);

	void execute();
	BOOL applyTemplates();

	DWORD getNodeId() { return m_nodeId; }
};


//
// Container category information
//

struct CONTAINER_CATEGORY
{
   DWORD dwCatId;
   TCHAR szName[MAX_OBJECT_NAME];
   TCHAR *pszDescription;
   DWORD dwImageId;
};


//
// Functions
//

void ObjectsInit();

void NXCORE_EXPORTABLE NetObjInsert(NetObj *pObject, BOOL bNewObject);
void NetObjDeleteFromIndexes(NetObj *pObject);
void NetObjDelete(NetObj *pObject);

void UpdateInterfaceIndex(DWORD dwOldIpAddr, DWORD dwNewIpAddr, Interface *pObject);

NetObj NXCORE_EXPORTABLE *FindObjectById(DWORD dwId);
NetObj NXCORE_EXPORTABLE *FindObjectByName(const TCHAR *name, int objClass);
NetObj NXCORE_EXPORTABLE *FindObjectByGUID(uuid_t guid, int objClass);
Template NXCORE_EXPORTABLE *FindTemplateByName(const TCHAR *pszName);
Node NXCORE_EXPORTABLE *FindNodeByIP(DWORD zoneId, DWORD ipAddr);
Node NXCORE_EXPORTABLE *FindNodeByMAC(const BYTE *macAddr);
Node NXCORE_EXPORTABLE *FindNodeByLLDPId(const TCHAR *lldpId);
Interface NXCORE_EXPORTABLE *FindInterfaceByMAC(const BYTE *macAddr);
Interface NXCORE_EXPORTABLE *FindInterfaceByDescription(const TCHAR *description);
Subnet NXCORE_EXPORTABLE *FindSubnetByIP(DWORD zoneId, DWORD ipAddr);
Subnet NXCORE_EXPORTABLE *FindSubnetForNode(DWORD zoneId, DWORD dwNodeAddr);
DWORD NXCORE_EXPORTABLE FindLocalMgmtNode();
CONTAINER_CATEGORY NXCORE_EXPORTABLE *FindContainerCategory(DWORD dwId);
Zone NXCORE_EXPORTABLE *FindZoneByGUID(DWORD dwZoneGUID);
Cluster NXCORE_EXPORTABLE *FindClusterByResourceIP(DWORD zone, DWORD ipAddr);
bool NXCORE_EXPORTABLE IsClusterIP(DWORD zone, DWORD ipAddr);

BOOL LoadObjects();
void DumpObjects(CONSOLE_CTX pCtx);

void DeleteUserFromAllObjects(DWORD dwUserId);

BOOL IsValidParentClass(int iChildClass, int iParentClass);
bool IsAgentPolicyObject(NetObj *object);

int DefaultPropagatedStatus(int iObjectStatus);
int GetDefaultStatusCalculation(int *pnSingleThreshold, int **ppnThresholds);


//
// Global variables
//

extern Network NXCORE_EXPORTABLE *g_pEntireNet;
extern ServiceRoot NXCORE_EXPORTABLE *g_pServiceRoot;
extern TemplateRoot NXCORE_EXPORTABLE *g_pTemplateRoot;
extern PolicyRoot NXCORE_EXPORTABLE *g_pPolicyRoot;
extern NetworkMapRoot NXCORE_EXPORTABLE *g_pMapRoot;
extern DashboardRoot NXCORE_EXPORTABLE *g_pDashboardRoot;
extern ReportRoot NXCORE_EXPORTABLE *g_pReportRoot;
extern BusinessServiceRoot NXCORE_EXPORTABLE *g_pBusinessServiceRoot;

extern DWORD NXCORE_EXPORTABLE g_dwMgmtNode;
extern DWORD g_dwNumCategories;
extern CONTAINER_CATEGORY *g_pContainerCatList;
extern const TCHAR *g_szClassName[];
extern BOOL g_bModificationsLocked;
extern Queue *g_pTemplateUpdateQueue;

extern ObjectIndex NXCORE_EXPORTABLE g_idxObjectById;
extern ObjectIndex NXCORE_EXPORTABLE g_idxSubnetByAddr;
extern ObjectIndex NXCORE_EXPORTABLE g_idxInterfaceByAddr;
extern ObjectIndex NXCORE_EXPORTABLE g_idxZoneByGUID;
extern ObjectIndex NXCORE_EXPORTABLE g_idxNodeById;
extern ObjectIndex NXCORE_EXPORTABLE g_idxConditionById;


#endif   /* _nms_objects_h_ */
