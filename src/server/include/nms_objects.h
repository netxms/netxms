/* 
** NetXMS - Network Management System
** Copyright (C) 2003, 2004, 2005 Victor Kirhenshtein
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
** $module: nms_objects.h
**
**/

#ifndef _nms_objects_h_
#define _nms_objects_h_

#include <nms_agent.h>
#include <nxnt.h>


//
// Forward declarations of classes
//

class AgentConnection;
class ClientSession;
class Queue;


//
// Global variables used by inline functions
//

extern DWORD g_dwDiscoveryPollingInterval;
extern DWORD g_dwStatusPollingInterval;
extern DWORD g_dwConfigurationPollingInterval;


//
// Constants
//

#define MAX_INTERFACES        4096
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


//
// Discovery flags
//

#define DF_CHECK_INTERFACES   0x0001
#define DF_CHECK_ARP          0x0002
#define DF_DEFAULT            (DF_CHECK_INTERFACES | DF_CHECK_ARP)


//
// Node runtime (dynamic) flags
//

#define NDF_QUEUED_FOR_STATUS_POLL     0x0001
#define NDF_QUEUED_FOR_CONFIG_POLL     0x0002
#define NDF_UNREACHEABLE               0x0004
#define NDF_AGENT_UNREACHEABLE         0x0008
#define NDF_SNMP_UNREACHEABLE          0x0010
#define NDF_QUEUED_FOR_DISCOVERY_POLL  0x0020
#define NDF_FORCE_STATUS_POLL          0x0040
#define NDF_FORCE_CONFIGURATION_POLL   0x0080


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
// Base class for network objects
//

class NXCORE_EXPORTABLE NetObj
{
protected:
   DWORD m_dwId;
   DWORD m_dwTimeStamp;    // Last change time stamp
   DWORD m_dwRefCount;     // Number of references. Object can be deleted only when this counter is zero
   char m_szName[MAX_OBJECT_NAME];
   int m_iStatus;
   BOOL m_bIsModified;
   BOOL m_bIsDeleted;
   BOOL m_bIsHidden;
   MUTEX m_hMutex;         // Generic object access mutex
   MUTEX m_mutexRefCount;  // Reference counter access mutex
   DWORD m_dwIpAddr;       // Every object should have an IP address
   DWORD m_dwImageId;      // Custom image id or 0 if object has default image
   ClientSession *m_pPollRequestor;

   DWORD m_dwChildCount;   // Number of child objects
   NetObj **m_pChildList;  // Array of pointers to child objects

   DWORD m_dwParentCount;  // Number of parent objects
   NetObj **m_pParentList; // Array of pointers to parent objects

   AccessList *m_pAccessList;
   BOOL m_bInheritAccessRights;

   void Lock(void) { MutexLock(m_hMutex, INFINITE); }
   void Unlock(void) { MutexUnlock(m_hMutex); }

   void Modify(void);                  // Used to mark object as modified

   BOOL LoadACLFromDB(void);
   BOOL SaveACLToDB(void);
   BOOL LoadCommonProperties(void);
   BOOL SaveCommonProperties(void);

   void SendPollerMsg(DWORD dwRqId, TCHAR *pszFormat, ...);

   virtual void OnObjectDelete(DWORD dwObjectId);

public:
   NetObj();
   virtual ~NetObj();

   virtual int Type(void) { return OBJECT_GENERIC; }
   
   DWORD IpAddr(void) { return m_dwIpAddr; }
   DWORD Id(void) { return m_dwId; }
   const char *Name(void) { return m_szName; }
   int Status(void) { return m_iStatus; }
   DWORD TimeStamp(void) { return m_dwTimeStamp; }

   BOOL IsModified(void) { return m_bIsModified; }
   BOOL IsDeleted(void) { return m_bIsDeleted; }
   BOOL IsOrphaned(void) { return m_dwParentCount == 0 ? TRUE : FALSE; }
   BOOL IsEmpty(void) { return m_dwChildCount == 0 ? TRUE : FALSE; }

   DWORD RefCount(void);
   void IncRefCount(void);
   void DecRefCount(void);

   BOOL IsChild(DWORD dwObjectId);

   void AddChild(NetObj *pObject);     // Add reference to child object
   void AddParent(NetObj *pObject);    // Add reference to parent object

   void DeleteChild(NetObj *pObject);  // Delete reference to child object
   void DeleteParent(NetObj *pObject); // Delete reference to parent object

   void Delete(BOOL bIndexLocked);     // Prepare object for deletion

   BOOL IsHidden(void) { return m_bIsHidden; }
   void Hide(void);
   void Unhide(void);

   virtual BOOL SaveToDB(void);
   virtual BOOL DeleteFromDB(void);
   virtual BOOL CreateFromDB(DWORD dwId);

   NetObj *GetParent(DWORD dwIndex = 0) { return dwIndex < m_dwParentCount ? m_pParentList[dwIndex] : NULL; }

   void SetId(DWORD dwId) { m_dwId = dwId; Modify(); }
   void SetMgmtStatus(BOOL bIsManaged);
   void SetName(char *pszName) { strncpy(m_szName, pszName, MAX_OBJECT_NAME); Modify(); }
   void ResetStatus(void) { m_iStatus = STATUS_UNKNOWN; Modify(); }

   virtual void CalculateCompoundStatus(void);

   virtual void CreateMessage(CSCPMessage *pMsg);
   virtual DWORD ModifyFromMessage(CSCPMessage *pRequest, BOOL bAlreadyLocked = FALSE);

   DWORD GetUserRights(DWORD dwUserId);
   BOOL CheckAccessRights(DWORD dwUserId, DWORD dwRequiredRights);
   void DropUserAccess(DWORD dwUserId);

   void AddChildNodesToList(DWORD *pdwNumNodes, Node ***pppNodeList, DWORD dwUserId);

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
   TCHAR *m_pszDescription;
   BOOL m_bDCIListModified;

   void LoadItemsFromDB(void);
   void DestroyItems(void);

public:
   Template();
   virtual ~Template();

   virtual int Type(void) { return OBJECT_TEMPLATE; }

   virtual BOOL SaveToDB(void);
   virtual BOOL DeleteFromDB(void);
   virtual BOOL CreateFromDB(DWORD dwId);

   virtual void CreateMessage(CSCPMessage *pMsg);
   virtual DWORD ModifyFromMessage(CSCPMessage *pRequest, BOOL bAlreadyLocked = FALSE);

   virtual void CalculateCompoundStatus(void);

   int VersionMajor(void) { return m_dwVersion >> 16; }
   int VersionMinor(void) { return m_dwVersion & 0xFFFF; }
   const TCHAR *Description(void) { return CHECK_NULL(m_pszDescription); }

   DWORD GetItemCount(void) { return m_dwNumItems; }
   BOOL AddItem(DCItem *pItem, BOOL bLocked = FALSE);
   BOOL UpdateItem(DWORD dwItemId, CSCPMessage *pMsg, DWORD *pdwNumMaps, 
                   DWORD **ppdwMapIndex, DWORD **ppdwMapId);
   BOOL DeleteItem(DWORD dwItemId);
   BOOL SetItemStatus(DWORD dwNumItems, DWORD *pdwItemList, int iStatus);
   int GetItemType(DWORD dwItemId);
   DCItem *GetItemById(DWORD dwItemId);
   DCItem *GetItemByIndex(DWORD dwIndex);
   BOOL LockDCIList(DWORD dwSessionId);
   BOOL UnlockDCIList(DWORD dwSessionId);
   void SetDCIModificationFlag(void) { m_bDCIListModified = TRUE; }
   void SendItemsToClient(ClientSession *pSession, DWORD dwRqId);
   BOOL IsLockedBySession(DWORD dwSessionId) { return m_dwDCILockStatus == dwSessionId; }

   BOOL ApplyToNode(Node *pNode);
   void QueueUpdate(void);
   void QueueRemoveFromNode(DWORD dwNodeId, BOOL bRemoveDCI);
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

public:
   Interface();
   Interface(DWORD dwAddr, DWORD dwNetMask);
   Interface(char *szName, DWORD dwIndex, DWORD dwAddr, DWORD dwNetMask, DWORD dwType);
   virtual ~Interface();

   virtual int Type(void) { return OBJECT_INTERFACE; }
   virtual BOOL SaveToDB(void);
   virtual BOOL DeleteFromDB(void);
   virtual BOOL CreateFromDB(DWORD dwId);

   Node *GetParentNode(void);

   void SetMacAddr(BYTE *pbNewMac) { memcpy(m_bMacAddr, pbNewMac, MAC_ADDR_LENGTH); Modify(); }

   DWORD IpNetMask(void) { return m_dwIpNetMask; }
   DWORD IfIndex(void) { return m_dwIfIndex; }
   DWORD IfType(void) { return m_dwIfType; }
   const BYTE *MacAddr(void) { return m_bMacAddr; }

   BOOL IsFake(void) { return (m_dwIfIndex == 1) && 
                              (m_dwIfType == IFTYPE_OTHER) &&
                              (!_tcscmp(m_szName, _T("lan0"))) &&
                              (!memcmp(m_bMacAddr, "\x00\x00\x00\x00\x00\x00", 6)); }

   void StatusPoll(ClientSession *pSession, DWORD dwRqId);
   virtual void CreateMessage(CSCPMessage *pMsg);

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
   TCHAR *m_pszResponce; // Service-specific expected responce

   virtual void OnObjectDelete(DWORD dwObjectId);

public:
   NetworkService();
   NetworkService(int iServiceType, WORD wProto, WORD wPort,
                  TCHAR *pszRequest, TCHAR *pszResponce,
                  Node *pHostNode = NULL, DWORD dwPollerNode = 0);
   virtual ~NetworkService();

   virtual int Type(void) { return OBJECT_NETWORKSERVICE; }

   virtual BOOL SaveToDB(void);
   virtual BOOL DeleteFromDB(void);
   virtual BOOL CreateFromDB(DWORD dwId);

   void StatusPoll(ClientSession *pSession, DWORD dwRqId, Node *pPollerNode);

   virtual void CreateMessage(CSCPMessage *pMsg);
   virtual DWORD ModifyFromMessage(CSCPMessage *pRequest, BOOL bAlreadyLocked = FALSE);
};


//
// Node
//

class NXCORE_EXPORTABLE Node : public Template
{
protected:
   DWORD m_dwFlags;
   DWORD m_dwDiscoveryFlags;
   DWORD m_dwDynamicFlags;       // Flags used at runtime by server
   DWORD m_dwZoneGUID;
   WORD m_wAgentPort;
   WORD m_wAuthMethod;
   DWORD m_dwNodeType;
   char m_szSharedSecret[MAX_SECRET_LENGTH];
   int m_iStatusPollType;
   int m_iSNMPVersion;
   char m_szCommunityString[MAX_COMMUNITY_LENGTH];
   char m_szObjectId[MAX_OID_LEN * 4];
   char m_szAgentVersion[MAX_AGENT_VERSION_LEN];
   char m_szPlatformName[MAX_PLATFORM_NAME_LEN];
   DWORD m_dwNumParams;           // Number of elements in supported parameters list
   NXC_AGENT_PARAM *m_pParamList; // List of supported parameters
   time_t m_tLastDiscoveryPoll;
   time_t m_tLastStatusPoll;
   time_t m_tLastConfigurationPoll;
   MUTEX m_hPollerMutex;
   MUTEX m_hAgentAccessMutex;
   AgentConnection *m_pAgentConnection;
   DWORD m_dwPollerNode;      // Node used for network service polling
   QWORD m_qwLastEvents[MAX_LAST_EVENTS];

   void PollerLock(void) { MutexLock(m_hPollerMutex, INFINITE); }
   void PollerUnlock(void) { MutexUnlock(m_hPollerMutex); }

   void AgentLock(void) { MutexLock(m_hAgentAccessMutex, INFINITE); }
   void AgentUnlock(void) { MutexUnlock(m_hAgentAccessMutex); }

   void CheckOSPFSupport(void);

   DWORD GetInterfaceCount(Interface **ppInterface);

   virtual void OnObjectDelete(DWORD dwObjectId);

public:
   Node();
   Node(DWORD dwAddr, DWORD dwFlags, DWORD dwDiscoveryFlags, DWORD dwZone);
   virtual ~Node();

   virtual int Type(void) { return OBJECT_NODE; }

   virtual BOOL SaveToDB(void);
   virtual BOOL DeleteFromDB(void);
   virtual BOOL CreateFromDB(DWORD dwId);

   DWORD Flags(void) { return m_dwFlags; }
   DWORD DiscoveryFlags(void) { return m_dwDiscoveryFlags; }
   DWORD RuntimeFlags(void) { return m_dwDynamicFlags; }
   DWORD ZoneGUID(void) { return m_dwZoneGUID; }

   BOOL IsSNMPSupported(void) { return m_dwFlags & NF_IS_SNMP ? TRUE : FALSE; }
   BOOL IsNativeAgent(void) { return m_dwFlags & NF_IS_NATIVE_AGENT ? TRUE : FALSE; }
   BOOL IsBridge(void) { return m_dwFlags & NF_IS_BRIDGE ? TRUE : FALSE; }
   BOOL IsRouter(void) { return m_dwFlags & NF_IS_ROUTER ? TRUE : FALSE; }
   BOOL IsLocalManagement(void) { return m_dwFlags & NF_IS_LOCAL_MGMT ? TRUE : FALSE; }

   const char *ObjectId(void) { return m_szObjectId; }

   void AddInterface(Interface *pInterface) { AddChild(pInterface); pInterface->AddParent(this); }
   void CreateNewInterface(DWORD dwAddr, DWORD dwNetMask, char *szName = NULL, 
                           DWORD dwIndex = 0, DWORD dwType = 0, BYTE *pbMacAddr = NULL);
   void DeleteInterface(Interface *pInterface);

   void NewNodePoll(DWORD dwNetMask);
   void ChangeIPAddress(DWORD dwIpAddr);

   ARP_CACHE *GetArpCache(void);
   INTERFACE_LIST *GetInterfaceList(void);
   Interface *FindInterface(DWORD dwIndex, DWORD dwHostAddr);
   int GetInterfaceStatusFromSNMP(DWORD dwIndex);
   int GetInterfaceStatusFromAgent(DWORD dwIndex);

   void SetDiscoveryPollTimeStamp(void) { m_tLastDiscoveryPoll = time(NULL); }
   void StatusPoll(ClientSession *pSession, DWORD dwRqId, int nPoller);
   void ConfigurationPoll(ClientSession *pSession, DWORD dwRqId, int nPoller);
   BOOL ReadyForStatusPoll(void);
   BOOL ReadyForConfigurationPoll(void);
   BOOL ReadyForDiscoveryPoll(void);
   void LockForStatusPoll(void);
   void LockForConfigurationPoll(void);
   void LockForDiscoveryPoll(void);

   virtual void CalculateCompoundStatus(void);

   BOOL ConnectToAgent(void);
   DWORD GetItemFromSNMP(const char *szParam, DWORD dwBufSize, char *szBuffer);
   DWORD GetItemFromAgent(const char *szParam, DWORD dwBufSize, char *szBuffer);
   DWORD GetInternalItem(const char *szParam, DWORD dwBufSize, char *szBuffer);
   void QueueItemsForPolling(Queue *pPollerQueue);
   DWORD GetItemForClient(int iOrigin, const char *pszParam, char *pszBuffer, DWORD dwBufSize);
   DWORD GetLastValues(CSCPMessage *pMsg);
   void CleanDCIData(void);
   BOOL ApplyTemplateItem(DCItem *pItem);
   void CleanDeletedTemplateItems(DWORD dwTemplateId, DWORD dwNumItems, DWORD *pdwItemList);
   void UnbindFromTemplate(DWORD dwTemplateId, BOOL bRemoveDCI);

   AgentConnection *CreateAgentConnection(void);

   virtual void CreateMessage(CSCPMessage *pMsg);
   virtual DWORD ModifyFromMessage(CSCPMessage *pRequest, BOOL bAlreadyLocked = FALSE);
   void WriteParamListToMessage(CSCPMessage *pMsg);

   DWORD WakeUp(void);

   void AddService(NetworkService *pNetSrv) { AddChild(pNetSrv); pNetSrv->AddParent(this); }
   DWORD CheckNetworkService(DWORD *pdwStatus, DWORD dwIpAddr, int iServiceType, WORD wPort = 0,
                             WORD wProto = 0, TCHAR *pszRequest = NULL, TCHAR *pszResponce = NULL);

   QWORD LastEventId(int nIndex) { return ((nIndex >= 0) && (nIndex < MAX_LAST_EVENTS)) ? m_qwLastEvents[nIndex] : 0; }
};


//
// Inline functions for Node class
//

inline BOOL Node::ReadyForStatusPoll(void) 
{
   if (m_dwDynamicFlags & NDF_FORCE_STATUS_POLL)
   {
      m_dwDynamicFlags &= ~NDF_FORCE_STATUS_POLL;
      return TRUE;
   }
   return ((m_iStatus != STATUS_UNMANAGED) && 
           (!(m_dwDynamicFlags & NDF_QUEUED_FOR_STATUS_POLL)) &&
           ((DWORD)time(NULL) - (DWORD)m_tLastStatusPoll > g_dwStatusPollingInterval))
               ? TRUE : FALSE;
}

inline BOOL Node::ReadyForConfigurationPoll(void) 
{ 
   if (m_dwDynamicFlags & NDF_FORCE_CONFIGURATION_POLL)
   {
      m_dwDynamicFlags &= ~NDF_FORCE_CONFIGURATION_POLL;
      return TRUE;
   }
   return ((m_iStatus != STATUS_UNMANAGED) &&
           (!(m_dwDynamicFlags & NDF_QUEUED_FOR_CONFIG_POLL)) &&
           ((DWORD)time(NULL) - (DWORD)m_tLastConfigurationPoll > g_dwConfigurationPollingInterval))
               ? TRUE : FALSE;
}

inline BOOL Node::ReadyForDiscoveryPoll(void) 
{ 
   return ((m_iStatus != STATUS_UNMANAGED) &&
           (!(m_dwDynamicFlags & NDF_QUEUED_FOR_DISCOVERY_POLL)) &&
           ((DWORD)time(NULL) - (DWORD)m_tLastDiscoveryPoll > g_dwDiscoveryPollingInterval))
               ? TRUE : FALSE; 
}

inline void Node::LockForStatusPoll(void)
{ 
   Lock(); 
   m_dwDynamicFlags |= NDF_QUEUED_FOR_STATUS_POLL; 
   Unlock(); 
}

inline void Node::LockForConfigurationPoll(void) 
{ 
   Lock(); 
   m_dwDynamicFlags |= NDF_QUEUED_FOR_CONFIG_POLL; 
   Unlock(); 
}

inline void Node::LockForDiscoveryPoll(void) 
{ 
   Lock(); 
   m_dwDynamicFlags |= NDF_QUEUED_FOR_DISCOVERY_POLL; 
   Unlock(); 
}


//
// Subnet
//

class NXCORE_EXPORTABLE Subnet : public NetObj
{
protected:
   DWORD m_dwIpNetMask;
   DWORD m_dwZoneGUID;

public:
   Subnet();
   Subnet(DWORD dwAddr, DWORD dwNetMask, DWORD dwZone);
   virtual ~Subnet();

   virtual int Type(void) { return OBJECT_SUBNET; }

   virtual BOOL SaveToDB(void);
   virtual BOOL DeleteFromDB(void);
   virtual BOOL CreateFromDB(DWORD dwId);

   void AddNode(Node *pNode) { AddChild(pNode); pNode->AddParent(this); }
   virtual void CreateMessage(CSCPMessage *pMsg);

   DWORD IpNetMask(void) { return m_dwIpNetMask; }
   DWORD ZoneGUID(void) { return m_dwZoneGUID; }
};


//
// Universal root object
//

class NXCORE_EXPORTABLE UniversalRoot : public NetObj
{
public:
   UniversalRoot();
   virtual ~UniversalRoot();

   virtual BOOL SaveToDB(void);
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
   virtual void CalculateCompoundStatus(void);
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
   char *m_pszDescription;

public:
   Container();
   Container(char *pszName, DWORD dwCategory, char *pszDescription);
   virtual ~Container();

   virtual int Type(void) { return OBJECT_CONTAINER; }
  
   virtual BOOL SaveToDB(void);
   virtual BOOL DeleteFromDB(void);
   virtual BOOL CreateFromDB(DWORD dwId);

   virtual void CreateMessage(CSCPMessage *pMsg);

   DWORD Category(void) { return m_dwCategory; }
   const TCHAR *Description(void) { return CHECK_NULL(m_pszDescription); }

   void LinkChildObjects(void);
   void LinkObject(NetObj *pObject) { AddChild(pObject); pObject->AddParent(this); }
};


//
// Template group object
//

class NXCORE_EXPORTABLE TemplateGroup : public Container
{
public:
   TemplateGroup() : Container() { }
   TemplateGroup(char *pszName, char *pszDescription) : Container(pszName, 0, pszDescription) { }
   virtual ~TemplateGroup() { }

   virtual int Type(void) { return OBJECT_TEMPLATEGROUP; }
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
   TCHAR *m_pszDescription;

public:
   Zone();
   virtual ~Zone();

   virtual int Type(void) { return OBJECT_ZONE; }

   virtual BOOL SaveToDB(void);
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
   virtual BOOL SaveToDB(void);

   void AddSubnet(Subnet *pSubnet) { AddChild(pSubnet); pSubnet->AddParent(this); }
   void AddZone(Zone *pZone) { AddChild(pZone); pZone->AddParent(this); }
   void LoadFromDB(void);
};


//
// Object index structure
//

struct INDEX
{
   DWORD dwKey;
   NetObj *pObject;
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

void NetObjInsert(NetObj *pObject, BOOL bNewObject);
void NetObjDeleteFromIndexes(NetObj *pObject);
void NetObjDelete(NetObj *pObject);

void UpdateNodeIndex(DWORD dwOldIpAddr, DWORD dwNewIpAddr, NetObj *pObject);

NetObj NXCORE_EXPORTABLE *FindObjectById(DWORD dwId);
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


//
// Global variables
//

extern Network *g_pEntireNet;
extern ServiceRoot *g_pServiceRoot;
extern TemplateRoot *g_pTemplateRoot;

extern DWORD g_dwMgmtNode;
extern INDEX *g_pIndexById;
extern DWORD g_dwIdIndexSize;
extern INDEX *g_pSubnetIndexByAddr;
extern DWORD g_dwSubnetAddrIndexSize;
extern INDEX *g_pNodeIndexByAddr;
extern DWORD g_dwNodeAddrIndexSize;
extern INDEX *g_pInterfaceIndexByAddr;
extern DWORD g_dwInterfaceAddrIndexSize;
extern INDEX *g_pZoneIndexByGUID;
extern DWORD g_dwZoneGUIDIndexSize;
extern RWLOCK g_rwlockIdIndex;
extern RWLOCK g_rwlockNodeIndex;
extern RWLOCK g_rwlockSubnetIndex;
extern RWLOCK g_rwlockInterfaceIndex;
extern RWLOCK g_rwlockZoneIndex;
extern DWORD g_dwNumCategories;
extern CONTAINER_CATEGORY *g_pContainerCatList;
extern char *g_szClassName[];
extern BOOL g_bModificationsLocked;
extern Queue *g_pTemplateUpdateQueue;


#endif   /* _nms_objects_h_ */
