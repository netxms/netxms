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
// Create object from database data
//

BOOL Interface::CreateFromDB(DWORD dwId)
{
   char szQuery[256];
   DB_RESULT hResult;
   DWORD dwNodeId;
   NetObj *pObject;
   BOOL bResult = FALSE;

   sprintf(szQuery, "SELECT id,name,status,ip_addr,ip_netmask,if_type,if_index,node_id,"
                    "image_id,is_deleted FROM interfaces WHERE id=%d", dwId);
   hResult = DBSelect(g_hCoreDB, szQuery);
   if (hResult == NULL)
      return FALSE;     // Query failed

   if (DBGetNumRows(hResult) != 0)
   {
      m_dwId = dwId;
      strncpy(m_szName, DBGetField(hResult, 0, 1), MAX_OBJECT_NAME);
      m_iStatus = DBGetFieldLong(hResult, 0, 2);
      m_dwIpAddr = DBGetFieldULong(hResult, 0, 3);
      m_dwIpNetMask = DBGetFieldULong(hResult, 0, 4);
      m_dwIfType = DBGetFieldULong(hResult, 0, 5);
      m_dwIfIndex = DBGetFieldULong(hResult, 0, 6);
      dwNodeId = DBGetFieldULong(hResult, 0, 7);
      m_dwImageId = DBGetFieldULong(hResult, 0, 8);
      m_bIsDeleted = DBGetFieldLong(hResult, 0, 9);

      // Link interface to node
      if (!m_bIsDeleted)
      {
         pObject = FindObjectById(dwNodeId);
         if (pObject == NULL)
         {
            WriteLog(MSG_INVALID_NODE_ID, EVENTLOG_ERROR_TYPE, "dd", dwId, dwNodeId);
         }
         else if (pObject->Type() != OBJECT_NODE)
         {
            WriteLog(MSG_NODE_NOT_NODE, EVENTLOG_ERROR_TYPE, "dd", dwId, dwNodeId);
         }
         else
         {
            pObject->AddChild(this);
            AddParent(pObject);
            bResult = TRUE;
         }
      }
      else
      {
         bResult = TRUE;
      }
   }

   DBFreeResult(hResult);

   // Load access list
   LoadACLFromDB();

   return bResult;
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
      sprintf(szQuery, "INSERT INTO interfaces (id,name,status,is_deleted,ip_addr,"
                       "ip_netmask,node_id,if_type,if_index,image_id) "
                       "VALUES (%ld,'%s',%d,%d,%ld,%ld,%ld,%ld,%ld,%ld)",
              m_dwId, m_szName, m_iStatus, m_bIsDeleted, m_dwIpAddr, m_dwIpNetMask, dwNodeId,
              m_dwIfType, m_dwIfIndex, m_dwImageId);
   else
      sprintf(szQuery, "UPDATE interfaces SET name='%s',status=%d,is_deleted=%d,"
                       "ip_addr=%ld,ip_netmask=%ld,node_id=%ld,if_type=%ld,"
                       "if_index=%ld,image_id=%ld WHERE id=%ld",
              m_szName, m_iStatus, m_bIsDeleted, m_dwIpAddr, m_dwIpNetMask, dwNodeId,
              m_dwIfType, m_dwIfIndex, m_dwImageId, m_dwId);
   DBQuery(g_hCoreDB, szQuery);

   // Save access list
   SaveACLToDB();

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
   BOOL bSuccess;

   bSuccess = NetObj::DeleteFromDB();
   if (bSuccess)
   {
      sprintf(szQuery, "DELETE FROM interfaces WHERE id=%ld", m_dwId);
      QueueSQLRequest(szQuery);
   }
   return bSuccess;
}


//
// Perform status poll on interface
//

void Interface::StatusPoll(void)
{
   int iOldStatus = m_iStatus;
   DWORD dwPingStatus;

   if (m_dwIpAddr == 0)
   {
      m_iStatus = STATUS_UNKNOWN;
      return;     // Interface has no IP address, we cannot check it
   }

   dwPingStatus = IcmpPing(m_dwIpAddr, 3, 1500, NULL);
   if (dwPingStatus == ICMP_RAW_SOCK_FAILED)
      WriteLog(MSG_RAW_SOCK_FAILED, EVENTLOG_WARNING_TYPE, NULL);
   m_iStatus = (dwPingStatus == ICMP_SUCCESS) ? STATUS_NORMAL : STATUS_CRITICAL;
   if (m_iStatus != iOldStatus)
   {
      PostEvent(m_iStatus == STATUS_NORMAL ? EVENT_INTERFACE_UP : EVENT_INTERFACE_DOWN,
                GetParent()->Id(), "dsaad", m_dwId, m_szName, m_dwIpAddr, m_dwIpNetMask,
                m_dwIfIndex);
      Modify();
   }
}


//
// Create CSCP message with object's data
//

void Interface::CreateMessage(CSCPMessage *pMsg)
{
   NetObj::CreateMessage(pMsg);
   pMsg->SetVariable(VID_IF_INDEX, m_dwIfIndex);
   pMsg->SetVariable(VID_IF_TYPE, m_dwIfType);
   pMsg->SetVariable(VID_IP_NETMASK, m_dwIpNetMask);
}
