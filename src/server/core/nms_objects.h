/* 
** Project X - Network Management System
** Copyright (C) 2003 Victor Kirhenshtein
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


//
// Forward declarations of classes
//

class AgentConnection;


//
// Global variables used by inline functions
//

extern DWORD g_dwDiscoveryPollingInterval;
extern DWORD g_dwStatusPollingInterval;
extern DWORD g_dwConfigurationPollingInterval;


//
// Constants
//

#define MAX_OBJECT_NAME       64
#define MAX_ITEM_NAME         256
#define MAX_INTERFACES        4096
#define INVALID_INDEX         0xFFFFFFFF
#define MAX_COMMUNITY_LENGTH  32


//
// Interface information structure used by discovery functions
//

struct INTERFACE_INFO
{
   char szName[MAX_OBJECT_NAME];
   DWORD dwIndex;
   DWORD dwType;
   DWORD dwIpAddr;
   DWORD dwIpNetMask;
   int iNumSecondary;      // Number of secondary IP's on this interface
};


//
// Interface list used by discovery functions
//

struct INTERFACE_LIST
{
   int iNumEntries;              // Number of entries in pInterfaces
   int iEnumPos;                 // Used by index enumeration handler
   INTERFACE_INFO *pInterfaces;  // Interface entries
};


//
// Single ARP cache entry
//

struct ARP_ENTRY
{
   DWORD dwIndex;       // Interface index
   DWORD dwIpAddr;
   BYTE bMacAddr[6];
};


//
// ARP cache structure used by discovery functions
//

struct ARP_CACHE
{
   DWORD dwNumEntries;
   ARP_ENTRY *pEntries;
};


//
// Data collection item structure
//

struct DC_ITEM
{
   DWORD dwId;
   char szName[MAX_ITEM_NAME];
   int iPollingInterval;   // Polling interval in seconds
   int iRetentionTime;     // Retention time in seconds
   BYTE iSource;           // SNMP or native agent?
   BYTE iDataType;
   BYTE iStatus;           // Item status: active, disabled or not supported
};


//
// Data types
//

#define DTYPE_INTEGER   0
#define DTYPE_INT64     1
#define DTYPE_STRING    2
#define DTYPE_FLOAT     3


//
// Data sources
//

#define DS_SNMP_AGENT      1
#define DS_NATIVE_AGENT    2


//
// Item status
//

#define ITEM_STATUS_ACTIVE          0
#define ITEM_STATUS_DISABLED        1
#define ITEM_STATUS_NOT_SUPPORTED   2


//
// Node flags
//

#define NF_IS_SNMP         0x0001
#define NF_IS_NATIVE_AGENT 0x0002
#define NF_IS_BRIDGE       0x0004
#define NF_IS_ROUTER       0x0008
#define NF_IS_LOCAL_MGMT   0x0010


//
// Object's status
//

#define STATUS_NORMAL      0
#define STATUS_MINOR       1
#define STATUS_WARNING     2
#define STATUS_MAJOR       3
#define STATUS_CRITICAL    4
#define STATUS_UNKNOWN     5
#define STATUS_UNMANAGED   6


//
// Discovery flags
//

#define DF_CHECK_INTERFACES   0x0001
#define DF_CHECK_ARP          0x0002
#define DF_DEFAULT            (DF_CHECK_INTERFACES | DF_CHECK_ARP)


//
// Status poll types
//

#define POLL_ICMP_PING        0
#define POLL_SNMP             1
#define POLL_NATIVE_AGENT     2


//
// Authentication method
//

#define AUTH_NONE          0
#define AUTH_PLAINTEXT     1
#define AUTH_MD5_HASH      2
#define AUTH_SHA1_HASH     3


//
// Object types
//

#define OBJECT_GENERIC     0
#define OBJECT_SUBNET      1
#define OBJECT_NODE        2
#define OBJECT_INTERFACE   3
#define OBJECT_NETWORK     4


//
// Base class for network objects
//

class NetObj
{
protected:
   DWORD m_dwId;
   char m_szName[MAX_OBJECT_NAME];
   int m_iStatus;
   BOOL m_bIsModified;
   BOOL m_bIsDeleted;
   MUTEX m_hMutex;
   DWORD m_dwIpAddr;       // Every object should have an IP address

   DWORD m_dwChildCount;   // Number of child objects
   NetObj **m_pChildList;  // Array of pointers to child objects

   DWORD m_dwParentCount;  // Number of parent objects
   NetObj **m_pParentList; // Array of pointers to parent objects

   void Lock(void) { MutexLock(m_hMutex, INFINITE); }
   void Unlock(void) { MutexUnlock(m_hMutex); }

public:
   NetObj();
   virtual ~NetObj();

   virtual int Type(void) { return OBJECT_GENERIC; }
   
   DWORD IpAddr(void) { return m_dwIpAddr; }
   DWORD Id(void) { return m_dwId; }
   const char *Name(void) { return m_szName; }
   int Status(void) { return m_iStatus; }

   BOOL IsModified(void) { return m_bIsModified; }
   BOOL IsDeleted(void) { return m_bIsDeleted; }
   BOOL IsOrphaned(void) { return m_dwParentCount == 0 ? TRUE : FALSE; }

   void AddChild(NetObj *pObject);     // Add reference to child object
   void AddParent(NetObj *pObject);    // Add reference to parent object

   void DeleteChild(NetObj *pObject);  // Delete reference to child object
   void DeleteParent(NetObj *pObject); // Delete reference to parent object

   void Delete(void);                  // Prepare object for deletion

   virtual BOOL SaveToDB(void);
   virtual BOOL DeleteFromDB(void);
   virtual BOOL CreateFromDB(DWORD dwId);

   NetObj *GetParent(DWORD dwIndex = 0) { return dwIndex < m_dwParentCount ? m_pParentList[dwIndex] : NULL; }

   void SetId(DWORD dwId) { m_dwId = dwId; m_bIsModified = TRUE; }

   virtual void CalculateCompoundStatus(void);

   // Debug methods
   const char *ParentList(char *szBuffer);
   const char *ChildList(char *szBuffer);
};


//
// Interface class
//

class Interface : public NetObj
{
protected:
   DWORD m_dwIfIndex;
   DWORD m_dwIfType;
   DWORD m_dwIpNetMask;

public:
   Interface();
   Interface(DWORD dwAddr, DWORD dwNetMask);
   Interface(char *szName, DWORD dwIndex, DWORD dwAddr, DWORD dwNetMask, DWORD dwType);
   virtual ~Interface();

   virtual int Type(void) { return OBJECT_INTERFACE; }
   virtual BOOL SaveToDB(void);
   virtual BOOL DeleteFromDB(void);
   virtual BOOL CreateFromDB(DWORD dwId);

   DWORD IpNetMask(void) { return m_dwIpNetMask; }
   DWORD IfIndex(void) { return m_dwIfIndex; }

   void StatusPoll(void);
};


//
// Node
//

class Node : public NetObj
{
protected:
   DWORD m_dwFlags;
   DWORD m_dwDiscoveryFlags;
   WORD m_wAgentPort;
   WORD m_wAuthMethod;
   char m_szSharedSecret[MAX_SECRET_LENGTH];
   int m_iStatusPollType;
   int m_iSNMPVersion;
   char m_szCommunityString[MAX_COMMUNITY_LENGTH];
   char m_szObjectId[MAX_OID_LEN * 4];
   time_t m_tLastDiscoveryPoll;
   time_t m_tLastStatusPoll;
   time_t m_tLastConfigurationPoll;
   int m_iSnmpAgentFails;
   int m_iNativeAgentFails;
   MUTEX m_hPollerMutex;
   DWORD m_dwNumItems;     // Number of data collection items
   DC_ITEM *m_pItems;      // Data collection items
   AgentConnection *m_pAgentConnection;

   void PollerLock(void) { MutexLock(m_hPollerMutex, INFINITE); }
   void PollerUnlock(void) { MutexUnlock(m_hPollerMutex); }

public:
   Node();
   Node(DWORD dwAddr, DWORD dwFlags, DWORD dwDiscoveryFlags);
   virtual ~Node();

   virtual int Type(void) { return OBJECT_NODE; }

   virtual BOOL SaveToDB(void);
   virtual BOOL DeleteFromDB(void);
   virtual BOOL CreateFromDB(DWORD dwId);

   DWORD Flags(void) { return m_dwFlags; }
   DWORD DiscoveryFlags(void) { return m_dwDiscoveryFlags; }

   BOOL IsSNMPSupported(void) { return m_dwFlags & NF_IS_SNMP ? TRUE : FALSE; }
   BOOL IsNativeAgent(void) { return m_dwFlags & NF_IS_NATIVE_AGENT ? TRUE : FALSE; }
   BOOL IsBridge(void) { return m_dwFlags & NF_IS_BRIDGE ? TRUE : FALSE; }
   BOOL IsRouter(void) { return m_dwFlags & NF_IS_ROUTER ? TRUE : FALSE; }
   BOOL IsLocalManagenet(void) { return m_dwFlags & NF_IS_LOCAL_MGMT ? TRUE : FALSE; }

   const char *ObjectId(void) { return m_szObjectId; }

   void AddInterface(Interface *pInterface) { AddChild(pInterface); pInterface->AddParent(this); }
   void CreateNewInterface(DWORD dwAddr, DWORD dwNetMask, char *szName = NULL, DWORD dwIndex = 0, DWORD dwType = 0);
   void DeleteInterface(Interface *pInterface);

   BOOL NewNodePoll(DWORD dwNetMask);

   ARP_CACHE *GetArpCache(void);
   INTERFACE_LIST *GetInterfaceList(void);
   Interface *FindInterface(DWORD dwIndex, DWORD dwHostAddr);

   BOOL ReadyForDiscoveryPoll(void) { return (DWORD)time(NULL) - (DWORD)m_tLastDiscoveryPoll > g_dwDiscoveryPollingInterval ? TRUE : FALSE; }
   void SetDiscoveryPollTimeStamp(void) { m_tLastDiscoveryPoll = time(NULL); }
   BOOL ReadyForStatusPoll(void) { return (DWORD)time(NULL) - (DWORD)m_tLastStatusPoll > g_dwStatusPollingInterval ? TRUE : FALSE; }
   void StatusPoll(void);
   BOOL ReadyForConfigurationPoll(void) { return (DWORD)time(NULL) - (DWORD)m_tLastConfigurationPoll > g_dwConfigurationPollingInterval ? TRUE : FALSE; }
   void ConfigurationPoll(void);

   virtual void CalculateCompoundStatus(void);

   void LoadItemsFromDB(void);

   BOOL ConnectToAgent(void);
   DWORD GetItemFromSNMP(char *szParam, DWORD dwBufSize, char *szBuffer);
   DWORD GetItemFromAgent(char *szParam, DWORD dwBufSize, char *szBuffer);
};


//
// Subnet
//

class Subnet : public NetObj
{
protected:
   DWORD m_dwIpNetMask;

public:
   Subnet();
   Subnet(DWORD dwAddr, DWORD dwNetMask);
   virtual ~Subnet();

   virtual int Type(void) { return OBJECT_SUBNET; }

   virtual BOOL SaveToDB(void);
   virtual BOOL DeleteFromDB(void);
   virtual BOOL CreateFromDB(DWORD dwId);

   void AddNode(Node *pNode) { AddChild(pNode); pNode->AddParent(this); }
};


//
// Entire network
//

class Network : public NetObj
{
public:
   Network();
   virtual ~Network();

   virtual int Type(void) { return OBJECT_NETWORK; }
   virtual BOOL SaveToDB(void);

   void AddSubnet(Subnet *pSubnet) { AddChild(pSubnet); pSubnet->AddParent(this); }
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
// Functions
//

void ObjectsInit(void);
void ObjectsGlobalLock(void);
void ObjectsGlobalUnlock(void);

void NetObjInsert(NetObj *pObject, BOOL bNewObject);
void NetObjDelete(NetObj *pObject);

NetObj *FindObjectById(DWORD dwId);
Node *FindNodeByIP(DWORD dwAddr);
Subnet *FindSubnetByIP(DWORD dwAddr);
DWORD FindLocalMgmtNode(void);

BOOL LoadObjects(void);


//
// Global variables
//

extern DWORD g_dwMgmtNode;
extern INDEX *g_pIndexById;
extern DWORD g_dwIdIndexSize;
extern INDEX *g_pSubnetIndexByAddr;
extern DWORD g_dwSubnetAddrIndexSize;
extern INDEX *g_pNodeIndexByAddr;
extern DWORD g_dwNodeAddrIndexSize;
extern INDEX *g_pInterfaceIndexByAddr;
extern DWORD g_dwInterfaceAddrIndexSize;
extern Network *g_pEntireNet;
extern MUTEX g_hMutexIdIndex;
extern MUTEX g_hMutexNodeIndex;
extern MUTEX g_hMutexSubnetIndex;
extern MUTEX g_hMutexInterfaceIndex;
extern MUTEX g_hMutexObjectAccess;


#endif   /* _nms_objects_h_ */
