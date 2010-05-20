/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2009 Victor Kirhenshtein
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
#include <nxnt.h>
#include <netxms_maps.h>
#include <geolocation.h>
#include "nxcore_jobs.h"


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

#define BUILTIN_OID_NETWORK         1
#define BUILTIN_OID_SERVICEROOT     2
#define BUILTIN_OID_TEMPLATEROOT    3
#define BUILTIN_OID_ZONE0           4
#define BUILTIN_OID_POLICYROOT      5


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
// Base class for network objects
//

class NXCORE_EXPORTABLE NetObj
{
protected:
   DWORD m_dwId;
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
   MUTEX m_mutexData;         // Object data access mutex
   MUTEX m_mutexRefCount;     // Reference counter access mutex
   RWLOCK m_rwlockParentList; // Lock for parent list
   RWLOCK m_rwlockChildList;  // Lock for child list
   DWORD m_dwIpAddr;          // Every object should have an IP address
	GeoLocation m_geoLocation;
   ClientSession *m_pPollRequestor;

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

   void LockData(void) { MutexLock(m_mutexData, INFINITE); }
   void UnlockData(void) { MutexUnlock(m_mutexData); }
   void LockACL(void) { MutexLock(m_mutexACL, INFINITE); }
   void UnlockACL(void) { MutexUnlock(m_mutexACL); }
   void LockParentList(BOOL bWrite) 
   { 
      if (bWrite) 
         RWLockWriteLock(m_rwlockParentList, INFINITE);
      else
         RWLockReadLock(m_rwlockParentList, INFINITE); 
   }
   void UnlockParentList(void) { RWLockUnlock(m_rwlockParentList); }
   void LockChildList(BOOL bWrite) 
   { 
      if (bWrite) 
         RWLockWriteLock(m_rwlockChildList, INFINITE);
      else
         RWLockReadLock(m_rwlockChildList, INFINITE); 
   }
   void UnlockChildList(void) { RWLockUnlock(m_rwlockChildList); }

   void Modify(void);                  // Used to mark object as modified

   BOOL LoadACLFromDB(void);
   BOOL SaveACLToDB(DB_HANDLE hdb);
   BOOL LoadCommonProperties(void);
   BOOL SaveCommonProperties(DB_HANDLE hdb);
	BOOL LoadTrustedNodes(void);
	BOOL SaveTrustedNodes(DB_HANDLE hdb);

   void SendPollerMsg(DWORD dwRqId, const TCHAR *pszFormat, ...);

   virtual void PrepareForDeletion(void);
   virtual void OnObjectDelete(DWORD dwObjectId);

public:
   NetObj();
   virtual ~NetObj();

   virtual int Type(void) { return OBJECT_GENERIC; }
   
   DWORD IpAddr(void) { return m_dwIpAddr; }
   DWORD Id(void) { return m_dwId; }
   const TCHAR *Name(void) { return m_szName; }
   int Status(void) { return m_iStatus; }
   int PropagatedStatus(void);
   DWORD TimeStamp(void) { return m_dwTimeStamp; }

   BOOL IsModified(void) { return m_bIsModified; }
   BOOL IsDeleted(void) { return m_bIsDeleted; }
   BOOL IsOrphaned(void) { return m_dwParentCount == 0 ? TRUE : FALSE; }
   BOOL IsEmpty(void) { return m_dwChildCount == 0 ? TRUE : FALSE; }
	
	BOOL IsSystem(void) { return m_bIsSystem; }
	void SetSystemFlag(BOOL bFlag) { m_bIsSystem = bFlag; }

   DWORD RefCount(void);
   void IncRefCount(void);
   void DecRefCount(void);

   BOOL IsChild(DWORD dwObjectId);
	BOOL IsTrustedNode(DWORD id);

   void AddChild(NetObj *pObject);     // Add reference to child object
   void AddParent(NetObj *pObject);    // Add reference to parent object

   void DeleteChild(NetObj *pObject);  // Delete reference to child object
   void DeleteParent(NetObj *pObject); // Delete reference to parent object

   void Delete(BOOL bIndexLocked);     // Prepare object for deletion

   BOOL IsHidden(void) { return m_bIsHidden; }
   void Hide(void);
   void Unhide(void);

   virtual BOOL SaveToDB(DB_HANDLE hdb);
   virtual BOOL DeleteFromDB(void);
   virtual BOOL CreateFromDB(DWORD dwId);

   void SetId(DWORD dwId) { m_dwId = dwId; Modify(); }
   void SetMgmtStatus(BOOL bIsManaged);
   void SetName(const TCHAR *pszName) { nx_strncpy(m_szName, pszName, MAX_OBJECT_NAME); Modify(); }
   void ResetStatus(void) { m_iStatus = STATUS_UNKNOWN; Modify(); }
   void SetComments(TCHAR *pszText);

   virtual void CalculateCompoundStatus(BOOL bForcedRecalc = FALSE);

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
   const char *ParentList(char *szBuffer);
   const char *ChildList(char *szBuffer);
   const char *TimeStampAsText(void) { return ctime((time_t *)&m_dwTimeStamp); }
};


//
// Inline functions of NetObj class
//

inline DWORD NetObj::RefCount(void)
{ 
   DWORD dwRefCount;

   MutexLock(m_mutexRefCount, INFINITE);
   dwRefCount = m_dwRefCount;
   MutexUnlock(m_mutexRefCount);
   return dwRefCount; 
}

inline void NetObj::IncRefCount(void)
{ 
   MutexLock(m_mutexRefCount, INFINITE);
   m_dwRefCount++;
   MutexUnlock(m_mutexRefCount);
}

inline void NetObj::DecRefCount(void)
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

   virtual void PrepareForDeletion(void);

   void LoadItemsFromDB(void);
   void DestroyItems(void);

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

   virtual void CalculateCompoundStatus(BOOL bForcedRecalc = FALSE);

   int VersionMajor(void) { return m_dwVersion >> 16; }
   int VersionMinor(void) { return m_dwVersion & 0xFFFF; }

   DWORD GetItemCount(void) { return m_dwNumItems; }
   BOOL AddItem(DCItem *pItem, BOOL bLocked = FALSE);
   BOOL UpdateItem(DWORD dwItemId, CSCPMessage *pMsg, DWORD *pdwNumMaps, 
                   DWORD **ppdwMapIndex, DWORD **ppdwMapId);
   BOOL DeleteItem(DWORD dwItemId, BOOL bNeedLock);
   BOOL SetItemStatus(DWORD dwNumItems, DWORD *pdwItemList, int iStatus);
   int GetItemType(DWORD dwItemId);
   DCItem *GetItemById(DWORD dwItemId);
   DCItem *GetItemByIndex(DWORD dwIndex);
   DCItem *GetItemByName(const TCHAR *pszName);
   DCItem *GetItemByDescription(const TCHAR *pszDescription);
   BOOL LockDCIList(DWORD dwSessionId, const TCHAR *pszNewOwner, TCHAR *pszCurrOwner);
   BOOL UnlockDCIList(DWORD dwSessionId);
   void SetDCIModificationFlag(void) { m_bDCIListModified = TRUE; }
   void SendItemsToClient(ClientSession *pSession, DWORD dwRqId);
   BOOL IsLockedBySession(DWORD dwSessionId) { return m_dwDCILockStatus == dwSessionId; }
   DWORD *GetDCIEventsList(DWORD *pdwCount);
	void ValidateSystemTemplate(void);
	void ValidateDCIList(DCI_CFG *cfg);

   BOOL ApplyToNode(Node *pNode);
	BOOL isApplicable(Node *node);
	BOOL isAutoApplyEnabled() { return m_applyFilter != NULL; }
	void setAutoApplyFilter(const TCHAR *filter);
   void queueUpdate(void);
   void queueRemoveFromNode(DWORD dwNodeId, BOOL bRemoveDCI);

   void CreateNXMPRecord(String &str);
	
	BOOL EnumDCI(BOOL (* pfCallback)(DCItem *, DWORD, void *), void *pArg);
	void AssociateItems(void);
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

public:
	Cluster();
   Cluster(const TCHAR *pszName);
	virtual ~Cluster();

   virtual int Type(void) { return OBJECT_CLUSTER; }
   virtual BOOL SaveToDB(DB_HANDLE hdb);
   virtual BOOL DeleteFromDB(void);
   virtual BOOL CreateFromDB(DWORD dwId);

   virtual void CreateMessage(CSCPMessage *pMsg);
   virtual DWORD ModifyFromMessage(CSCPMessage *pRequest, BOOL bAlreadyLocked = FALSE);

   virtual void CalculateCompoundStatus(BOOL bForcedRecalc = FALSE);

	BOOL IsSyncAddr(DWORD dwAddr);
	BOOL IsVirtualAddr(DWORD dwAddr);
	BOOL IsResourceOnNode(DWORD dwResource, DWORD dwNode);

   void StatusPoll(ClientSession *pSession, DWORD dwRqId, int nPoller);
   void LockForStatusPoll(void) { m_dwFlags |= CLF_QUEUED_FOR_STATUS_POLL; }
   BOOL ReadyForStatusPoll(void) 
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
   DWORD m_dwIfIndex;
   DWORD m_dwIfType;
   DWORD m_dwIpNetMask;
   BYTE m_bMacAddr[MAC_ADDR_LENGTH];
   QWORD m_qwLastDownEventId;
	BOOL m_bSyntheticMask;
	int m_iPendingStatus;
	int m_iPollCount;
	int m_iRequiredPollCount;

public:
   Interface();
   Interface(DWORD dwAddr, DWORD dwNetMask, BOOL bSyntheticMask);
   Interface(const char *szName, DWORD dwIndex, DWORD dwAddr, DWORD dwNetMask, DWORD dwType);
   virtual ~Interface();

   virtual int Type(void) { return OBJECT_INTERFACE; }
   virtual BOOL SaveToDB(DB_HANDLE hdb);
   virtual BOOL DeleteFromDB(void);
   virtual BOOL CreateFromDB(DWORD dwId);

   Node *GetParentNode(void);

   void SetMacAddr(BYTE *pbNewMac) { memcpy(m_bMacAddr, pbNewMac, MAC_ADDR_LENGTH); Modify(); }

   DWORD IpNetMask(void) { return m_dwIpNetMask; }
   DWORD IfIndex(void) { return m_dwIfIndex; }
   DWORD IfType(void) { return m_dwIfType; }
   const BYTE *MacAddr(void) { return m_bMacAddr; }
	BOOL IsSyntheticMask(void) { return m_bSyntheticMask; }

   QWORD GetLastDownEventId(void) { return m_qwLastDownEventId; }
   void SetLastDownEventId(QWORD qwId) { m_qwLastDownEventId = qwId; }

   BOOL IsFake(void) { return (m_dwIfIndex == 1) && 
                              (m_dwIfType == IFTYPE_OTHER) &&
                              (!_tcscmp(m_szName, _T("lan0"))) &&
                              (!memcmp(m_bMacAddr, "\x00\x00\x00\x00\x00\x00", 6)); }
   void SetIpAddr(DWORD dwNewAddr);

   void StatusPoll(ClientSession *pSession, DWORD dwRqId, Queue *pEventQueue,
	                BOOL bClusterSync, SNMP_Transport *pTransport);
   
	virtual void CreateMessage(CSCPMessage *pMsg);
   virtual DWORD ModifyFromMessage(CSCPMessage *pRequest, BOOL bAlreadyLocked = FALSE);

   DWORD WakeUp(void);
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

   virtual int Type(void) { return OBJECT_NETWORKSERVICE; }

   virtual BOOL SaveToDB(DB_HANDLE hdb);
   virtual BOOL DeleteFromDB(void);
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

   Node *GetParentNode(void);

public:
   VPNConnector();
   VPNConnector(BOOL bIsHidden);
   virtual ~VPNConnector();

   virtual int Type(void) { return OBJECT_VPNCONNECTOR; }

   virtual BOOL SaveToDB(DB_HANDLE hdb);
   virtual BOOL DeleteFromDB(void);
   virtual BOOL CreateFromDB(DWORD dwId);

   virtual void CreateMessage(CSCPMessage *pMsg);
   virtual DWORD ModifyFromMessage(CSCPMessage *pRequest, BOOL bAlreadyLocked = FALSE);

   BOOL IsLocalAddr(DWORD dwIpAddr);
   BOOL IsRemoteAddr(DWORD dwIpAddr);
   DWORD GetPeerGatewayAddr(void);
};


//
// Node
//

class NXCORE_EXPORTABLE Node : public Template
{
protected:
   DWORD m_dwFlags;
   DWORD m_dwDynamicFlags;       // Flags used at runtime by server
	int m_iPendingStatus;
	int m_iPollCount;
	int m_iRequiredPollCount;
   DWORD m_dwZoneGUID;
   WORD m_wAgentPort;
   WORD m_wAuthMethod;
   DWORD m_dwNodeType;
   char m_szSharedSecret[MAX_SECRET_LENGTH];
   int m_iStatusPollType;
   int m_snmpVersion;
   WORD m_wSNMPPort;
	WORD m_nUseIfXTable;
	SNMP_SecurityContext *m_snmpSecurity;
   char m_szObjectId[MAX_OID_LEN * 4];
   char m_szAgentVersion[MAX_AGENT_VERSION_LEN];
   char m_szPlatformName[MAX_PLATFORM_NAME_LEN];
	char m_szSysDescription[MAX_DB_STRING];  // Agent's System.Uname or SNMP sysDescr
   DWORD m_dwNumParams;           // Number of elements in supported parameters list
   NXC_AGENT_PARAM *m_pParamList; // List of supported parameters
   time_t m_tLastDiscoveryPoll;
   time_t m_tLastStatusPoll;
   time_t m_tLastConfigurationPoll;
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
	nxmap_ObjList *m_pTopology;		// Layer 2 topology
	time_t m_tLastTopologyPoll;
	ServerJobQueue *m_jobQueue;

   void PollerLock(void) { MutexLock(m_hPollerMutex, INFINITE); }
   void PollerUnlock(void) { MutexUnlock(m_hPollerMutex); }

   void AgentLock(void) { MutexLock(m_hAgentAccessMutex, INFINITE); }
   void AgentUnlock(void) { MutexUnlock(m_hAgentAccessMutex); }

   void RTLock(void) { MutexLock(m_mutexRTAccess, INFINITE); }
   void RTUnlock(void) { MutexUnlock(m_mutexRTAccess); }

   BOOL CheckSNMPIntegerValue(SNMP_Transport *pTransport, const char *pszOID, int nValue);
   void CheckOSPFSupport(SNMP_Transport *pTransport);
	BOOL ResolveName(BOOL useOnlyDNS);
   void SetAgentProxy(AgentConnection *pConn);

   DWORD GetInterfaceCount(Interface **ppInterface);

   void CheckInterfaceNames(INTERFACE_LIST *pIfList);
	void CheckSubnetBinding(INTERFACE_LIST *pIfList);

	void ApplySystemTemplates();
	void ApplyUserTemplates();

	void UpdateContainerMembership();

   virtual void PrepareForDeletion(void);
   virtual void OnObjectDelete(DWORD dwObjectId);

public:
   Node();
   Node(DWORD dwAddr, DWORD dwFlags, DWORD dwProxyNode, DWORD dwSNMPProxy, DWORD dwZone);
   virtual ~Node();

   virtual int Type(void) { return OBJECT_NODE; }

   virtual BOOL SaveToDB(DB_HANDLE hdb);
   virtual BOOL DeleteFromDB(void);
   virtual BOOL CreateFromDB(DWORD dwId);

	Cluster *GetMyCluster(void);

   DWORD Flags(void) { return m_dwFlags; }
   DWORD RuntimeFlags(void) { return m_dwDynamicFlags; }
   DWORD ZoneGUID(void) { return m_dwZoneGUID; }
   void SetLocalMgmtFlag(void) { m_dwFlags |= NF_IS_LOCAL_MGMT; }
   void ClearLocalMgmtFlag(void) { m_dwFlags &= ~NF_IS_LOCAL_MGMT; }

   BOOL IsSNMPSupported(void) { return m_dwFlags & NF_IS_SNMP ? TRUE : FALSE; }
   BOOL IsNativeAgent(void) { return m_dwFlags & NF_IS_NATIVE_AGENT ? TRUE : FALSE; }
   BOOL IsBridge(void) { return m_dwFlags & NF_IS_BRIDGE ? TRUE : FALSE; }
   BOOL IsRouter(void) { return m_dwFlags & NF_IS_ROUTER ? TRUE : FALSE; }
   BOOL IsLocalManagement(void) { return m_dwFlags & NF_IS_LOCAL_MGMT ? TRUE : FALSE; }

	LONG GetSNMPVersion() { return m_snmpVersion; }
	const TCHAR *GetSNMPObjectId() { return m_szObjectId; }
	const TCHAR *GetAgentVersion() { return m_szAgentVersion; }
	const TCHAR *GetPlatformName() { return m_szPlatformName; }

   BOOL IsDown(void) { return m_dwDynamicFlags & NDF_UNREACHABLE ? TRUE : FALSE; }

   const char *ObjectId(void) { return m_szObjectId; }

   void AddInterface(Interface *pInterface) { AddChild(pInterface); pInterface->AddParent(this); }
   void CreateNewInterface(DWORD dwAddr, DWORD dwNetMask, char *szName = NULL, 
                           DWORD dwIndex = 0, DWORD dwType = 0, BYTE *pbMacAddr = NULL);
   void DeleteInterface(Interface *pInterface);

   void ChangeIPAddress(DWORD dwIpAddr);

   ARP_CACHE *GetArpCache(void);
   INTERFACE_LIST *GetInterfaceList(void);
   Interface *FindInterface(DWORD dwIndex, DWORD dwHostAddr);
	BOOL IsMyIP(DWORD dwIpAddr);
   int GetInterfaceStatusFromSNMP(SNMP_Transport *pTransport, DWORD dwIndex);
   int GetInterfaceStatusFromAgent(DWORD dwIndex);
   ROUTING_TABLE *GetRoutingTable(void);
   ROUTING_TABLE *GetCachedRoutingTable(void) { return m_pRoutingTable; }
   BOOL GetNextHop(DWORD dwSrcAddr, DWORD dwDestAddr, DWORD *pdwNextHop,
                   DWORD *pdwIfIndex, BOOL *pbIsVPN);

	void SetRecheckCapsFlag() { m_dwDynamicFlags |= NDF_RECHECK_CAPABILITIES; }
   void SetDiscoveryPollTimeStamp();
   void StatusPoll(ClientSession *pSession, DWORD dwRqId, int nPoller);
   void ConfigurationPoll(ClientSession *pSession, DWORD dwRqId, int nPoller, DWORD dwNetMask);
	void UpdateInterfaceNames(ClientSession *pSession, DWORD dwRqId);
   void UpdateRoutingTable();
   BOOL ReadyForStatusPoll();
   BOOL ReadyForConfigurationPoll();
   BOOL ReadyForDiscoveryPoll();
   BOOL ReadyForRoutePoll();
   void LockForStatusPoll();
   void LockForConfigurationPoll();
   void LockForDiscoveryPoll();
   void LockForRoutePoll();
	void ForceConfigurationPoll() { m_dwDynamicFlags |= NDF_FORCE_CONFIGURATION_POLL; }

   virtual void CalculateCompoundStatus(BOOL bForcedRecalc = FALSE);

   BOOL ConnectToAgent(void);
   DWORD GetItemFromSNMP(const char *szParam, DWORD dwBufSize, char *szBuffer);
   DWORD GetItemFromCheckPointSNMP(const char *szParam, DWORD dwBufSize, char *szBuffer);
   DWORD GetItemFromAgent(const char *szParam, DWORD dwBufSize, char *szBuffer);
   DWORD GetInternalItem(const char *szParam, DWORD dwBufSize, char *szBuffer);
   void QueueItemsForPolling(Queue *pPollerQueue);
   DWORD GetItemForClient(int iOrigin, const char *pszParam, char *pszBuffer, DWORD dwBufSize);
   DWORD GetLastValues(CSCPMessage *pMsg);
   void CleanDCIData(void);
   BOOL ApplyTemplateItem(DWORD dwTemplateId, DCItem *pItem);
   void CleanDeletedTemplateItems(DWORD dwTemplateId, DWORD dwNumItems, DWORD *pdwItemList);
   void UnbindFromTemplate(DWORD dwTemplateId, BOOL bRemoveDCI);
   void UpdateDCICache(void);
	DWORD getPerfTabDCIList(CSCPMessage *pMsg);

   void OpenParamList(DWORD *pdwNumParams, NXC_AGENT_PARAM **ppParamList);
   void CloseParamList(void) { UnlockData(); }

   AgentConnectionEx *createAgentConnection();
	SNMP_Transport *createSnmpTransport();
	SNMP_SecurityContext *getSnmpSecurityContext();

   virtual void CreateMessage(CSCPMessage *pMsg);
   virtual DWORD ModifyFromMessage(CSCPMessage *pRequest, BOOL bAlreadyLocked = FALSE);
   void WriteParamListToMessage(CSCPMessage *pMsg);

   DWORD WakeUp(void);

   void AddService(NetworkService *pNetSrv) { AddChild(pNetSrv); pNetSrv->AddParent(this); }
   DWORD CheckNetworkService(DWORD *pdwStatus, DWORD dwIpAddr, int iServiceType, WORD wPort = 0,
                             WORD wProto = 0, TCHAR *pszRequest = NULL, TCHAR *pszResponse = NULL);

   QWORD GetLastEventId(int nIndex) { return ((nIndex >= 0) && (nIndex < MAX_LAST_EVENTS)) ? m_qwLastEvents[nIndex] : 0; }
   void SetLastEventId(int nIndex, QWORD qwId) { if ((nIndex >= 0) && (nIndex < MAX_LAST_EVENTS)) m_qwLastEvents[nIndex] = qwId; }

   DWORD CallSnmpEnumerate(const char *pszRootOid, 
      DWORD (* pHandler)(DWORD, SNMP_Variable *, SNMP_Transport *, void *), void *pArg);

	nxmap_ObjList *GetL2Topology(void);
	nxmap_ObjList *BuildL2Topology(DWORD *pdwStatus);

	ServerJobQueue *getJobQueue() { return m_jobQueue; }
};


//
// Inline functions for Node class
//

inline void Node::SetDiscoveryPollTimeStamp(void)
{
   m_tLastDiscoveryPoll = time(NULL);
   m_dwDynamicFlags &= ~NDF_QUEUED_FOR_DISCOVERY_POLL;
}

inline BOOL Node::ReadyForStatusPoll(void) 
{
	if (m_bIsDeleted)
		return FALSE;
   if (m_dwDynamicFlags & NDF_FORCE_STATUS_POLL)
   {
      m_dwDynamicFlags &= ~NDF_FORCE_STATUS_POLL;
      return TRUE;
   }
   return ((m_iStatus != STATUS_UNMANAGED) &&
	        (!(m_dwFlags & NF_DISABLE_STATUS_POLL)) &&
           (!(m_dwDynamicFlags & NDF_QUEUED_FOR_STATUS_POLL)) &&
           (!(m_dwDynamicFlags & NDF_POLLING_DISABLED)) &&
			  (GetMyCluster() == NULL) &&
           ((DWORD)time(NULL) - (DWORD)m_tLastStatusPoll > g_dwStatusPollingInterval))
               ? TRUE : FALSE;
}

inline BOOL Node::ReadyForConfigurationPoll(void) 
{ 
	if (m_bIsDeleted)
		return FALSE;
   if (m_dwDynamicFlags & NDF_FORCE_CONFIGURATION_POLL)
   {
      m_dwDynamicFlags &= ~NDF_FORCE_CONFIGURATION_POLL;
      return TRUE;
   }
   return ((m_iStatus != STATUS_UNMANAGED) &&
	        (!(m_dwFlags & NF_DISABLE_CONF_POLL)) &&
           (!(m_dwDynamicFlags & NDF_QUEUED_FOR_CONFIG_POLL)) &&
           (!(m_dwDynamicFlags & NDF_POLLING_DISABLED)) &&
           ((DWORD)time(NULL) - (DWORD)m_tLastConfigurationPoll > g_dwConfigurationPollingInterval))
               ? TRUE : FALSE;
}

inline BOOL Node::ReadyForDiscoveryPoll(void) 
{ 
	if (m_bIsDeleted)
		return FALSE;
   return ((g_dwFlags & AF_ENABLE_NETWORK_DISCOVERY) &&
           (m_iStatus != STATUS_UNMANAGED) &&
           (!(m_dwDynamicFlags & NDF_QUEUED_FOR_DISCOVERY_POLL)) &&
           (!(m_dwDynamicFlags & NDF_POLLING_DISABLED)) &&
           (m_dwDynamicFlags & NDF_CONFIGURATION_POLL_PASSED) &&
           ((DWORD)time(NULL) - (DWORD)m_tLastDiscoveryPoll > g_dwDiscoveryPollingInterval))
               ? TRUE : FALSE; 
}

inline BOOL Node::ReadyForRoutePoll(void) 
{ 
	if (m_bIsDeleted)
		return FALSE;
   return ((m_iStatus != STATUS_UNMANAGED) &&
	        (!(m_dwFlags & NF_DISABLE_ROUTE_POLL)) &&
           (!(m_dwDynamicFlags & NDF_QUEUED_FOR_ROUTE_POLL)) &&
           (!(m_dwDynamicFlags & NDF_POLLING_DISABLED)) &&
           ((DWORD)time(NULL) - (DWORD)m_tLastRTUpdate > g_dwRoutingTableUpdateInterval))
               ? TRUE : FALSE; 
}

inline void Node::LockForStatusPoll(void)
{ 
   LockData(); 
   m_dwDynamicFlags |= NDF_QUEUED_FOR_STATUS_POLL; 
   UnlockData(); 
}

inline void Node::LockForConfigurationPoll(void) 
{ 
   LockData(); 
   m_dwDynamicFlags |= NDF_QUEUED_FOR_CONFIG_POLL; 
   UnlockData(); 
}

inline void Node::LockForDiscoveryPoll(void) 
{ 
   LockData(); 
   m_dwDynamicFlags |= NDF_QUEUED_FOR_DISCOVERY_POLL; 
   UnlockData(); 
}

inline void Node::LockForRoutePoll(void) 
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
   DWORD m_dwZoneGUID;
	BOOL m_bSyntheticMask;

public:
   Subnet();
   Subnet(DWORD dwAddr, DWORD dwNetMask, DWORD dwZone, BOOL bSyntheticMask);
   virtual ~Subnet();

   virtual int Type(void) { return OBJECT_SUBNET; }

   virtual BOOL SaveToDB(DB_HANDLE hdb);
   virtual BOOL DeleteFromDB(void);
   virtual BOOL CreateFromDB(DWORD dwId);

   void AddNode(Node *pNode) { AddChild(pNode); pNode->AddParent(this); }
   virtual void CreateMessage(CSCPMessage *pMsg);

   DWORD IpNetMask(void) { return m_dwIpNetMask; }
   DWORD ZoneGUID(void) { return m_dwZoneGUID; }
	BOOL IsSyntheticMask(void) { return m_bSyntheticMask; }

	void SetCorrectMask(DWORD dwAddr, DWORD dwMask);
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
   virtual void LoadFromDB(void);
   virtual const char *DefaultName(void) { return "Root Object"; }

   void LinkChildObjects(void);
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

   virtual int Type(void) { return OBJECT_SERVICEROOT; }
   virtual const char *DefaultName(void) { return "All Services"; }
};


//
// Template root
//

class NXCORE_EXPORTABLE TemplateRoot : public UniversalRoot
{
public:
   TemplateRoot();
   virtual ~TemplateRoot();

   virtual int Type(void) { return OBJECT_TEMPLATEROOT; }
   virtual const char *DefaultName(void) { return "Templates"; }
   virtual void CalculateCompoundStatus(BOOL bForcedRecalc = FALSE);
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
   Container(TCHAR *pszName, DWORD dwCategory);
   virtual ~Container();

   virtual int Type(void) { return OBJECT_CONTAINER; }
  
   virtual BOOL SaveToDB(DB_HANDLE hdb);
   virtual BOOL DeleteFromDB(void);
   virtual BOOL CreateFromDB(DWORD dwId);

   virtual void CreateMessage(CSCPMessage *pMsg);
   virtual DWORD ModifyFromMessage(CSCPMessage *pRequest, BOOL bAlreadyLocked = FALSE);

   DWORD Category(void) { return m_dwCategory; }

   void LinkChildObjects(void);
   void LinkObject(NetObj *pObject) { AddChild(pObject); pObject->AddParent(this); }

	BOOL IsSuitableForNode(Node *node);
	BOOL IsAutoBindEnabled() { return m_bindFilter != NULL; }

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

   virtual int Type(void) { return OBJECT_TEMPLATEGROUP; }
   virtual void CalculateCompoundStatus(BOOL bForcedRecalc = FALSE);
};


//
// Zone object
//

class Zone : public NetObj
{
protected:
   DWORD m_dwZoneGUID;
   int m_iZoneType;
   DWORD m_dwControllerIpAddr;
   DWORD m_dwAddrListSize;
   DWORD *m_pdwIpAddrList;

public:
   Zone();
   virtual ~Zone();

   virtual int Type(void) { return OBJECT_ZONE; }

   virtual BOOL SaveToDB(DB_HANDLE hdb);
   virtual BOOL DeleteFromDB(void);
   virtual BOOL CreateFromDB(DWORD dwId);

   virtual void CreateMessage(CSCPMessage *pMsg);
   virtual DWORD ModifyFromMessage(CSCPMessage *pRequest, BOOL bAlreadyLocked = FALSE);

   DWORD GUID(void) { return m_dwZoneGUID; }

   void AddSubnet(Subnet *pSubnet) { AddChild(pSubnet); pSubnet->AddParent(this); }
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

   virtual int Type(void) { return OBJECT_CONDITION; }

   virtual BOOL SaveToDB(DB_HANDLE hdb);
   virtual BOOL DeleteFromDB(void);
   virtual BOOL CreateFromDB(DWORD dwId);

   virtual void CreateMessage(CSCPMessage *pMsg);
   virtual DWORD ModifyFromMessage(CSCPMessage *pRequest, BOOL bAlreadyLocked = FALSE);

   void Check(void);

   void LockForPoll(void);
   void EndPoll(void);

   BOOL ReadyForPoll(void) 
   {
      return ((m_iStatus != STATUS_UNMANAGED) && 
              (!m_bQueuedForPolling) && (!m_bIsDeleted) &&
              ((DWORD)time(NULL) - (DWORD)m_tmLastPoll > g_dwConditionPollingInterval))
                  ? TRUE : FALSE;
   }

   int GetCacheSizeForDCI(DWORD dwItemId, BOOL bNoLock);
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

	BOOL SavePolicyCommonProperties(DB_HANDLE hdb);

public:
   AgentPolicy(int type);
   AgentPolicy(const TCHAR *name, int type);
   virtual ~AgentPolicy();

   virtual int Type(void) { return OBJECT_AGENTPOLICY; }

   virtual BOOL SaveToDB(DB_HANDLE hdb);
   virtual BOOL DeleteFromDB(void);
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
	TCHAR m_fileName[MAX_POLICY_CONFIG_NAME];
	TCHAR *m_fileContent;

public:
   AgentPolicyConfig();
   AgentPolicyConfig(const TCHAR *name);
   virtual ~AgentPolicyConfig();

   virtual int Type(void) { return OBJECT_AGENTPOLICY_CONFIG; }

   virtual BOOL SaveToDB(DB_HANDLE hdb);
   virtual BOOL DeleteFromDB(void);
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
   PolicyGroup(TCHAR *pszName) : Container(pszName, 0) { }
   virtual ~PolicyGroup() { }

   virtual int Type(void) { return OBJECT_POLICYGROUP; }
   virtual void CalculateCompoundStatus(BOOL bForcedRecalc = FALSE);
};


//
// Policy root
//

class NXCORE_EXPORTABLE PolicyRoot : public UniversalRoot
{
public:
   PolicyRoot();
   virtual ~PolicyRoot();

   virtual int Type(void) { return OBJECT_POLICYROOT; }
   virtual const char *DefaultName(void) { return "Policies"; }
   virtual void CalculateCompoundStatus(BOOL bForcedRecalc = FALSE);
};


//
// Object index structure
//

struct INDEX
{
   DWORD dwKey;
   void *pObject;
};


//
// Container category information
//

struct CONTAINER_CATEGORY
{
   DWORD dwCatId;
   char szName[MAX_OBJECT_NAME];
   char *pszDescription;
   DWORD dwImageId;
};


//
// Functions
//

void ObjectsInit(void);

void NXCORE_EXPORTABLE NetObjInsert(NetObj *pObject, BOOL bNewObject);
void NetObjDeleteFromIndexes(NetObj *pObject);
void NetObjDelete(NetObj *pObject);

void UpdateNodeIndex(DWORD dwOldIpAddr, DWORD dwNewIpAddr, NetObj *pObject);
void UpdateInterfaceIndex(DWORD dwOldIpAddr, DWORD dwNewIpAddr, NetObj *pObject);

NetObj NXCORE_EXPORTABLE *FindObjectById(DWORD dwId);
NetObj NXCORE_EXPORTABLE *FindObjectByName(const TCHAR *name, int objClass);
Template NXCORE_EXPORTABLE *FindTemplateByName(const TCHAR *pszName);
Node NXCORE_EXPORTABLE *FindNodeByIP(DWORD dwAddr);
Subnet NXCORE_EXPORTABLE *FindSubnetByIP(DWORD dwAddr);
Subnet NXCORE_EXPORTABLE *FindSubnetForNode(DWORD dwNodeAddr);
DWORD NXCORE_EXPORTABLE FindLocalMgmtNode(void);
CONTAINER_CATEGORY NXCORE_EXPORTABLE *FindContainerCategory(DWORD dwId);
Zone NXCORE_EXPORTABLE *FindZoneByGUID(DWORD dwZoneGUID);

BOOL LoadObjects(void);
void DumpObjects(CONSOLE_CTX pCtx);

void DeleteUserFromAllObjects(DWORD dwUserId);

BOOL IsValidParentClass(int iChildClass, int iParentClass);

int DefaultPropagatedStatus(int iObjectStatus);
int GetDefaultStatusCalculation(int *pnSingleThreshold, int **ppnThresholds);


//
// Global variables
//

extern Network NXCORE_EXPORTABLE *g_pEntireNet;
extern ServiceRoot NXCORE_EXPORTABLE *g_pServiceRoot;
extern TemplateRoot NXCORE_EXPORTABLE *g_pTemplateRoot;
extern PolicyRoot NXCORE_EXPORTABLE *g_pPolicyRoot;

extern DWORD NXCORE_EXPORTABLE g_dwMgmtNode;
extern INDEX NXCORE_EXPORTABLE *g_pIndexById;
extern DWORD NXCORE_EXPORTABLE g_dwIdIndexSize;
extern INDEX NXCORE_EXPORTABLE *g_pSubnetIndexByAddr;
extern DWORD NXCORE_EXPORTABLE g_dwSubnetAddrIndexSize;
extern INDEX NXCORE_EXPORTABLE *g_pNodeIndexByAddr;
extern DWORD NXCORE_EXPORTABLE g_dwNodeAddrIndexSize;
extern INDEX NXCORE_EXPORTABLE *g_pInterfaceIndexByAddr;
extern DWORD NXCORE_EXPORTABLE g_dwInterfaceAddrIndexSize;
extern INDEX NXCORE_EXPORTABLE *g_pZoneIndexByGUID;
extern DWORD NXCORE_EXPORTABLE g_dwZoneGUIDIndexSize;
extern INDEX NXCORE_EXPORTABLE *g_pConditionIndex;
extern DWORD NXCORE_EXPORTABLE g_dwConditionIndexSize;
extern RWLOCK NXCORE_EXPORTABLE g_rwlockIdIndex;
extern RWLOCK NXCORE_EXPORTABLE g_rwlockNodeIndex;
extern RWLOCK NXCORE_EXPORTABLE g_rwlockSubnetIndex;
extern RWLOCK NXCORE_EXPORTABLE g_rwlockInterfaceIndex;
extern RWLOCK NXCORE_EXPORTABLE g_rwlockZoneIndex;
extern RWLOCK NXCORE_EXPORTABLE g_rwlockConditionIndex;
extern DWORD g_dwNumCategories;
extern CONTAINER_CATEGORY *g_pContainerCatList;
extern const char *g_szClassName[];
extern BOOL g_bModificationsLocked;
extern Queue *g_pTemplateUpdateQueue;


#endif   /* _nms_objects_h_ */
