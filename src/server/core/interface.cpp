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
** $module: interface.cpp
**
**/

#include "nms_core.h"


//
// Default constructor for Interface object
//

Interface::Interface()
          : NetObj()
{
   m_dwIpNetMask = 0;
   m_dwIfIndex = 0;
   m_dwIfType = IFTYPE_OTHER;
}


//
// Constructor for "fake" interface object
//

Interface::Interface(DWORD dwAddr, DWORD dwNetMask)
          : NetObj()
{
   strcpy(m_szName, "lan0");
   m_dwIpAddr = dwAddr;
   m_dwIpNetMask = dwNetMask;
   m_dwIfIndex = 1;
   m_dwIfType = IFTYPE_OTHER;
}


//
// Constructor for normal interface object
//

Interface::Interface(char *szName, DWORD dwIndex, DWORD dwAddr, DWORD dwNetMask, DWORD dwType)
          : NetObj()
{
   strncpy(m_szName, szName, MAX_OBJECT_NAME);
   m_dwIfIndex = dwIndex;
   m_dwIfType = dwType;
   m_dwIpAddr = dwAddr;
   m_dwIpNetMask = dwNetMask;
}


//
// Interfacet class destructor
//

Interface::~Interface()
{
}


//
// Save interface object to database
//

BOOL Interface::SaveToDB(void)
{
   char szQuery[1024];
   BOOL bNewObject = TRUE;
   DWORD dwNodeId;
   DB_RESULT hResult;

   // Lock object's access
   Lock();

   // Check for object's existence in database
   sprintf(szQuery, "SELECT id FROM interfaces WHERE id=%ld", m_dwId);
   hResult = DBSelect(g_hCoreDB, szQuery);
   if (hResult != 0)
   {
      if (DBGetNumRows(hResult) > 0)
         bNewObject = FALSE;
      DBFreeResult(hResult);
   }

   // Determine owning node's ID
   if (m_dwParentCount > 0)   // Always should be
      dwNodeId = m_pParentList[0]->Id();
   else
      dwNodeId = 0;

   // Form and execute INSERT or UPDATE query
   if (bNewObject)
      sprintf(szQuery, "INSERT INTO interfaces (id,name,status,is_deleted,ip_addr,ip_netmask,node_id,if_type,if_index) "
                       "VALUES (%d,'%s',%d,%d,%d,%d,%d,%d,%d)",
              m_dwId, m_szName, m_iStatus, m_bIsDeleted, m_dwIpAddr, m_dwIpNetMask, dwNodeId, m_dwIfType, m_dwIfIndex);
   else
      sprintf(szQuery, "UPDATE subnets SET name='%s',status=%d,is_deleted=%d,ip_addr=%d,ip_netmask=%d,node_id=%d,if_type=%d,if_index=%d WHERE id=%d",
              m_szName, m_iStatus, m_bIsDeleted, m_dwIpAddr, m_dwIpNetMask, dwNodeId, m_dwIfType, m_dwIfIndex, m_dwId);
   DBQuery(g_hCoreDB, szQuery);

   // Clear modifications flag and unlock object
   m_bIsModified = FALSE;
   Unlock();

   return TRUE;
}


//
// Delete interface object from database
//

BOOL Interface::DeleteFromDB(void)
{
   char szQuery[128];

   sprintf(szQuery, "DELETE FROM interfaces WHERE id=%ld", m_dwId);
   DBQuery(g_hCoreDB, szQuery);
   return TRUE;
}
