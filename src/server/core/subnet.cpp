/* 
** NetXMS - Network Management System
** Copyright (C) 2003, 2004 Victor Kirhenshtein
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
   char szBuffer[32];

   m_dwIpAddr = dwAddr;
   m_dwIpNetMask = dwNetMask;
   sprintf(m_szName, "%s/%d", IpToStr(dwAddr, szBuffer), BitsInMask(dwNetMask));
}


//
// Subnet class destructor
//

Subnet::~Subnet()
{
}


//
// Create object from database data
//

BOOL Subnet::CreateFromDB(DWORD dwId)
{
   char szQuery[256];
   DB_RESULT hResult;

   sprintf(szQuery, "SELECT id,name,status,ip_addr,ip_netmask,image_id FROM subnets WHERE id=%d", dwId);
   hResult = DBSelect(g_hCoreDB, szQuery);
   if (hResult == 0)
      return FALSE;     // Query failed

   if (DBGetNumRows(hResult) == 0)
   {
      DBFreeResult(hResult);
      return FALSE;
   }

   m_dwId = dwId;
   strncpy(m_szName, DBGetField(hResult, 0, 1), MAX_OBJECT_NAME);
   m_iStatus = DBGetFieldLong(hResult, 0, 2);
   m_dwIpAddr = DBGetFieldULong(hResult, 0, 3);
   m_dwIpNetMask = DBGetFieldULong(hResult, 0, 4);
   m_dwImageId = DBGetFieldULong(hResult, 0, 5);

   DBFreeResult(hResult);

   // Load access list
   LoadACLFromDB();

   return TRUE;
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
      sprintf(szQuery, "INSERT INTO subnets (id,name,status,is_deleted,ip_addr,ip_netmask,image_id) "
                       "VALUES (%ld,'%s',%d,%d,%ld,%ld,%ld)",
              m_dwId, m_szName, m_iStatus, m_bIsDeleted, m_dwIpAddr, m_dwIpNetMask, m_dwImageId);
   else
      sprintf(szQuery, "UPDATE subnets SET name='%s',status=%d,is_deleted=%d,ip_addr=%ld,"
                       "ip_netmask=%ld,image_id=%ld WHERE id=%ld",
              m_szName, m_iStatus, m_bIsDeleted, m_dwIpAddr, m_dwIpNetMask, m_dwImageId, m_dwId);
   DBQuery(g_hCoreDB, szQuery);

   // Update node to subnet mapping
   sprintf(szQuery, "DELETE FROM nsmap WHERE subnet_id=%d", m_dwId);
   DBQuery(g_hCoreDB, szQuery);
   for(i = 0; i < m_dwChildCount; i++)
   {
      sprintf(szQuery, "INSERT INTO nsmap (subnet_id,node_id) VALUES (%ld,%ld)", m_dwId, m_pChildList[i]->Id());
      DBQuery(g_hCoreDB, szQuery);
   }

   // Save access list
   SaveACLToDB();

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
   BOOL bSuccess;

   bSuccess = NetObj::DeleteFromDB();
   if (bSuccess)
   {
      sprintf(szQuery, "DELETE FROM subnets WHERE id=%ld", m_dwId);
      QueueSQLRequest(szQuery);
      sprintf(szQuery, "DELETE FROM nsmap WHERE subnet_id=%ld", m_dwId);
      QueueSQLRequest(szQuery);
   }
   return bSuccess;
}


//
// Create CSCP message with object's data
//

void Subnet::CreateMessage(CSCPMessage *pMsg)
{
   NetObj::CreateMessage(pMsg);
   pMsg->SetVariable(VID_IP_NETMASK, m_dwIpNetMask);
}
