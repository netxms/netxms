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
** $module: subnet.cpp
**
**/

#include "nms_core.h"


//
// Subnet class default constructor
//

Subnet::Subnet()
       :NetObj()
{
   m_dwIpNetMask = 0;
}


//
// Subnet class constructor
//

Subnet::Subnet(DWORD dwAddr, DWORD dwNetMask)
{
   m_dwIpAddr = dwAddr;
   m_dwIpNetMask = dwNetMask;
}


//
// Subnet class destructor
//

Subnet::~Subnet()
{
}


//
// Save subnet object to database
//

BOOL Subnet::SaveToDB(void)
{
   char szQuery[1024];
   DB_RESULT hResult;
   DWORD i;
   BOOL bNewObject = TRUE;

   // Lock object's access
   Lock();

   // Check for object's existence in database
   sprintf(szQuery, "SELECT id FROM subnets WHERE id=%ld", m_dwId);
   hResult = DBSelect(g_hCoreDB, szQuery);
   if (hResult != 0)
   {
      if (DBGetNumRows(hResult) > 0)
         bNewObject = FALSE;
      DBFreeResult(hResult);
   }

   // Form and execute INSERT or UPDATE query
   if (bNewObject)
      sprintf(szQuery, "INSERT INTO subnets (id,name,status,is_deleted,ip_addr,ip_netmask) VALUES (%d,'%s',%d,%d,%d,%d)",
              m_dwId, m_szName, m_iStatus, m_bIsDeleted, m_dwIpAddr, m_dwIpNetMask);
   else
      sprintf(szQuery, "UPDATE subnets SET name='%s',status=%d,is_deleted=%d,ip_addr=%d,ip_netmask=%d WHERE id=%d",
              m_szName, m_iStatus, m_bIsDeleted, m_dwIpAddr, m_dwIpNetMask, m_dwId);
   DBQuery(g_hCoreDB, szQuery);

   // Update node to subnet mapping
   sprintf(szQuery, "DELETE FROM nsmap WHERE subnet_id=%d", m_dwId);
   DBQuery(g_hCoreDB, szQuery);
   for(i = 0; i < m_dwChildCount; i++)
   {
      sprintf(szQuery, "INSERT INTO nsmap (subnet_id,node_id) VALUES (%ld,%ld)", m_dwId, m_pChildList[i]->Id());
      DBQuery(g_hCoreDB, szQuery);
   }

   // Clear modifications flag and unlock object
   m_bIsModified = FALSE;
   Unlock();

   return TRUE;
}


//
// Delete subnet object from database
//

BOOL Subnet::DeleteFromDB(void)
{
   char szQuery[256];

   sprintf(szQuery, "DELETE FROM subnets WHERE id=%ld", m_dwId);
   DBQuery(g_hCoreDB, szQuery);
   sprintf(szQuery, "DELETE FROM nsmap WHERE subnet_id=%ld", m_dwId);
   DBQuery(g_hCoreDB, szQuery);
   return TRUE;
}
