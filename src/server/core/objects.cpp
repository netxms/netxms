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
** $module: objects.cpp
**
**/

#include "nms_core.h"


//
// Global variables
//

DWORD g_dwNodeCount = 0;
DWORD g_dwSubnetCount = 0;
Node **g_pNodeList = NULL;
Subnet **g_pSubnetList = NULL;


//
// Static data
//

static MUTEX m_hMutexObjectAccess;


//
// NetObj class constructor
//

NetObj::NetObj()
{
   m_hMutex = MutexCreate();
   m_iStatus = STATUS_UNMANAGED;
   m_szName[0] = 0;
   m_bIsModified = FALSE;
}


//
// NetObj class destructor
//

NetObj::~NetObj()
{
   MutexDestroy(m_hMutex);
}


//
// Node class constructor
//

Node::Node()
     :NetObj()
{
}


//
// Node class destructor
//

Node::~Node()
{
}


//
// Save node object to database
//

void Node::SaveToDB(void)
{
}


//
// Delete node from database
//

void Node::DeleteFromDB(void)
{
   char szQuery[256];

   sprintf(szQuery, "UPDATE nodes SET isDeleted=1 WHERE id=%ld", m_dwId);
   DBQuery(g_hCoreDB, szQuery);
}


//
// Delete node
//

void Node::Delete(void)
{
   DWORD i;

   // Unlink node from all subnets
   for(i = 0; i < m_dwSubnetCount; i++)
      m_pSubnetList[i]->UnlinkNode(this);

   // Mark for deletion
   m_bIsDeleted = TRUE;
}


//
// Subnet class constructor
//

Subnet::Subnet()
       :NetObj()
{
   m_pNodeList = NULL;
   m_dwNodeCount = 0;
}


//
// Subnet class destructor
//

Subnet::~Subnet()
{
   if (m_pNodeList != NULL)
      free(m_pNodeList);
}


//
// Save subnet object to database
//

void Subnet::SaveToDB(void)
{
   char szQuery[1024];
   DB_RESULT hResult;

   sprintf(szQuery, "SELECT id FROM subnets WHERE id=%ld", m_dwId);
   hResult = DBSelect(g_hCoreDB, szQuery);
}


//
// Delete subnet object from database
//

void Subnet::DeleteFromDB(void)
{
   char szQuery[256];

   sprintf(szQuery, "UPDATE subnets SET isDeleted=1 WHERE id=%ld", m_dwId);
   DBQuery(g_hCoreDB, szQuery);
}


//
// Delete subnet object
//

void Subnet::Delete(void)
{
}


//
// Link node to subnet
//

void Subnet::LinkNode(Node *pNode)
{
   DWORD i;

   for(i = 0; i < m_dwNodeCount; i++)
      if (m_pNodeList[i] == pNode)
         return;     // Already linked to this subnet
   m_pNodeList = (Node **)realloc(m_pNodeList, sizeof(Node *) * (m_dwNodeCount + 1));
   m_pNodeList[m_dwNodeCount++] = pNode;
}


//
// Unlink node from subnet
//

void Subnet::UnlinkNode(Node *pNode)
{
   DWORD i;

   for(i = 0; i < m_dwNodeCount; i++)
      if (m_pNodeList[i] == pNode)
      {
         memmove(&m_pNodeList[i], &m_pNodeList[i + 1], sizeof(Node *) * (m_dwNodeCount - i - 1));
         m_dwNodeCount--;
         break;
      }
}


//
// Initialize objects infrastructure
//

void ObjectsInit(void)
{
   m_hMutexObjectAccess = MutexCreate();
}


//
// Lock write access to all objects
//

void ObjectsGlobalLock(void)
{
   MutexLock(m_hMutexObjectAccess, INFINITE);
}


//
// Unlock write access to all objects
//

void ObjectsGlobalUnlock(void)
{
   MutexUnlock(m_hMutexObjectAccess);
}
