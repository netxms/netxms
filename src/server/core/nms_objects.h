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
// Constants
//

#define MAX_OBJECT_NAME       64
#define MAX_INTERFACES        4096
#define INVALID_INDEX         0xFFFFFFFF
#define MAX_COMMUNITY_LENGTH  32


//
// Node flags
//

#define NF_IS_SNMP         0x0001
#define NF_IS_NATIVE_AGENT 0x0002
#define NF_IS_BRIDGE       0x0004
#define NF_IS_ROUTER       0x0008


//
// Object's status
//

#define STATUS_NORMAL      0
#define STATUS_INFO        1
#define STATUS_WARNING     2
#define STATUS_ERROR       3
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

   BOOL IsSNMPSupported(void) { return m_dwFlags & NF_IS_SNMP; }
   BOOL IsNativeAgent(void) { return m_dwFlags & NF_IS_NATIVE_AGENT; }
   BOOL IsBridge(void) { return m_dwFlags & NF_IS_BRIDGE; }
   BOOL IsRouter(void) { return m_dwFlags & NF_IS_ROUTER; }

   void AddInterface(Interface *pInterface) { AddChild(pInterface); pInterface->AddParent(this); }
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

void NetObjInsert(NetObj *pObject);
void NetObjDelete(NetObj *pObject);

NetObj *FindObjectById(DWORD dwId);
Node *FindNodeByIP(DWORD dwAddr);
Subnet *FindSubnetByIP(DWORD dwAddr);

BOOL LoadObjects(void);


//
// Global variables
//

extern INDEX *g_pIndexById;
extern DWORD g_dwIdIndexSize;
extern INDEX *g_pSubnetIndexByAddr;
extern DWORD g_dwSubnetAddrIndexSize;
extern INDEX *g_pNodeIndexByAddr;
extern DWORD g_dwNodeAddrIndexSize;
extern INDEX *g_pInterfaceIndexByAddr;
extern DWORD g_dwInterfaceAddrIndexSize;
extern Network *g_pEntireNet;


#endif   /* _nms_objects_h_ */
