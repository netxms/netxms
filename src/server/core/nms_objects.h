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

//
// Constants
//

#define MAX_OBJECT_NAME    64
#define MAX_INTERFACES     4096


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
// Interface structure
//

struct Interface
{
   DWORD dwId;
   char szName[MAX_OBJECT_NAME];
   int iStatus;
   DWORD dwIndex;
   DWORD dwIpAddr;
   DWORD dwIpNetMask;
};


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

   void Lock(void) { MutexLock(m_hMutex, INFINITE); }
   void Unlock(void) { MutexUnlock(m_hMutex); }

public:
   NetObj();
   virtual ~NetObj();

   BOOL IsModified(void) { return m_bIsModified; }
   BOOL IsDeleted(void) { return m_bIsDeleted; }
};


//
// Node
//

class Subnet;

class Node : public NetObj
{
protected:
   DWORD m_dwPrimaryIP;
   DWORD m_dwFlags;
   DWORD m_dwIfCount;
   Interface *m_pIfList;
   DWORD m_dwSubnetCount;
   Subnet *m_pSubnetList[MAX_INTERFACES];

public:
   Node();
   virtual ~Node();

   BOOL IsSNMPSupported(void) { return m_dwFlags & NF_IS_SNMP; }
   BOOL IsNativeAgent(void) { return m_dwFlags & NF_IS_NATIVE_AGENT; }
   BOOL IsBridge(void) { return m_dwFlags & NF_IS_BRIDGE; }
   BOOL IsRouter(void) { return m_dwFlags & NF_IS_ROUTER; }

   void Delete(void);

   void SaveToDB(void);
   void DeleteFromDB(void);
};


//
// Subnet
//

class Subnet : public NetObj
{
protected:
   DWORD m_dwIpAddr;
   DWORD m_dwIpNetMask;
   DWORD m_dwNodeCount;
   Node **m_pNodeList;

public:
   Subnet();
   virtual ~Subnet();

   void LinkNode(Node *pNode);
   void UnlinkNode(Node *pNode);

   void Delete(void);

   void SaveToDB(void);
   void DeleteFromDB(void);
};


//
// Functions
//

void ObjectsInit(void);
void ObjectsGlobalLock(void);
void ObjectsGlobalUnlock(void);


//
// Global variables
//

extern DWORD g_dwNodeCount;
extern DWORD g_dwSubnetCount;
extern Node **g_pNodeList;
extern Subnet **g_pSubnetList;

#endif   /* _nms_objects_h_ */
