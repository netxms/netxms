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
** $module: node.cpp
**
**/

#include "nms_core.h"


//
// Node class default constructor
//

Node::Node()
     :NetObj()
{
   m_dwFlags = 0;
   m_dwDiscoveryFlags = 0;
   m_wAgentPort = AGENT_LISTEN_PORT;
   m_wAuthMethod = AUTH_NONE;
   m_szSharedSecret[0] = 0;
   m_iStatusPollType = POLL_ICMP_PING;
   m_iSNMPVersion = SNMP_VERSION_1;
   strcpy(m_szCommunityString, "public");
   m_szObjectId[0] = 0;
}


//
// Constructor for new node object
//

Node::Node(DWORD dwAddr, DWORD dwFlags, DWORD dwDiscoveryFlags)
     :NetObj()
{
   m_dwIpAddr = dwAddr;
   m_dwFlags = dwFlags;
   m_dwDiscoveryFlags = dwDiscoveryFlags;
   m_wAgentPort = AGENT_LISTEN_PORT;
   m_wAuthMethod = AUTH_NONE;
   m_szSharedSecret[0] = 0;
   m_iStatusPollType = POLL_ICMP_PING;
   m_iSNMPVersion = SNMP_VERSION_1;
   strcpy(m_szCommunityString, "public");
   IpToStr(dwAddr, m_szName);    // Make default name from IP address
   m_szObjectId[0] = 0;
}


//
// Node destructor
//

Node::~Node()
{
}


//
// Create object from database data
//

BOOL Node::CreateFromDB(DWORD dwId)
{
   char szQuery[256];
   DB_RESULT hResult;
   int i, iNumRows;
   DWORD dwSubnetId;
   NetObj *pObject;

   sprintf(szQuery, "SELECT id,name,status,primary_ip,is_snmp,is_agent,is_bridge,"
                    "is_router,snmp_version,discovery_flags,auth_method,secret,"
                    "agent_port,status_poll_type,community,snmp_oid FROM nodes WHERE id=%d", dwId);
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

   // Flags
   if (DBGetFieldLong(hResult, 0, 4))
      m_dwFlags |= NF_IS_SNMP;
   if (DBGetFieldLong(hResult, 0, 5))
      m_dwFlags |= NF_IS_NATIVE_AGENT;
   if (DBGetFieldLong(hResult, 0, 6))
      m_dwFlags |= NF_IS_BRIDGE;
   if (DBGetFieldLong(hResult, 0, 7))
      m_dwFlags |= NF_IS_ROUTER;

   m_iSNMPVersion = DBGetFieldLong(hResult, 0, 8);
   m_dwDiscoveryFlags = DBGetFieldULong(hResult, 0, 9);
   m_wAuthMethod = (WORD)DBGetFieldLong(hResult, 0, 10);
   strncpy(m_szSharedSecret, DBGetField(hResult, 0, 11), MAX_SECRET_LENGTH);
   m_wAgentPort = (WORD)DBGetFieldLong(hResult, 0, 12);
   m_iStatusPollType = DBGetFieldLong(hResult, 0, 13);
   strncpy(m_szCommunityString, DBGetField(hResult, 0, 14), MAX_COMMUNITY_LENGTH);
   strncpy(m_szObjectId, DBGetField(hResult, 0, 15), MAX_OID_LEN * 4);

   DBFreeResult(hResult);

   // Link node to subnets
   sprintf(szQuery, "SELECT subnet_id FROM nsmap WHERE node_id=%d", dwId);
   hResult = DBSelect(g_hCoreDB, szQuery);
   if (hResult == 0)
      return FALSE;     // Query failed

   if (DBGetNumRows(hResult) == 0)
   {
      DBFreeResult(hResult);
      return FALSE;     // No parents - it shouldn't happen if database isn't corrupted
   }

   BOOL bResult = FALSE;
   iNumRows = DBGetNumRows(hResult);
   for(i = 0; i < iNumRows; i++)
   {
      dwSubnetId = DBGetFieldULong(hResult, i, 0);
      pObject = FindObjectById(dwSubnetId);
      if (pObject == NULL)
      {
         WriteLog(MSG_INVALID_SUBNET_ID, EVENTLOG_ERROR_TYPE, "dd", dwId, dwSubnetId);
         break;
      }
      else if (pObject->Type() != OBJECT_SUBNET)
      {
         WriteLog(MSG_SUBNET_NOT_SUBNET, EVENTLOG_ERROR_TYPE, "dd", dwId, dwSubnetId);
         break;
      }
      else
      {
         pObject->AddChild(this);
         AddParent(pObject);
         bResult = TRUE;
      }
   }

   DBFreeResult(hResult);
   return bResult;
}


//
// Save object to database
//

BOOL Node::SaveToDB(void)
{
   char szQuery[4096];
   DB_RESULT hResult;
   BOOL bNewObject = TRUE;

   // Lock object's access
   Lock();

   // Check for object's existence in database
   sprintf(szQuery, "SELECT id FROM nodes WHERE id=%ld", m_dwId);
   hResult = DBSelect(g_hCoreDB, szQuery);
   if (hResult != 0)
   {
      if (DBGetNumRows(hResult) > 0)
         bNewObject = FALSE;
      DBFreeResult(hResult);
   }

   // Form and execute INSERT or UPDATE query
   if (bNewObject)
      sprintf(szQuery, "INSERT INTO nodes (id,name,status,is_deleted,primary_ip,"
                       "is_snmp,is_agent,is_bridge,is_router,snmp_version,community,"
                       "discovery_flags,status_poll_type,agent_port,auth_method,secret,"
                       "snmp_oid)"
                       " VALUES (%d,'%s',%d,%d,%d,%d,%d,%d,%d,%d,'%s',%d,%d,%d,%d,'%s','%s')",
              m_dwId, m_szName, m_iStatus, m_bIsDeleted, m_dwIpAddr, 
              m_dwFlags & NF_IS_SNMP ? 1 : 0,
              m_dwFlags & NF_IS_NATIVE_AGENT ? 1 : 0,
              m_dwFlags & NF_IS_BRIDGE ? 1 : 0,
              m_dwFlags & NF_IS_ROUTER ? 1 : 0,
              m_iSNMPVersion, m_szCommunityString, m_dwDiscoveryFlags, m_iStatusPollType,
              m_wAgentPort,m_wAuthMethod,m_szSharedSecret, m_szObjectId);
   else
      sprintf(szQuery, "UPDATE nodes SET name='%s',status=%d,is_deleted=%d,primary_ip=%d,"
                       "is_snmp=%d,is_agent=%d,is_bridge=%d,is_router=%d,snmp_version=%d,"
                       "community='%s',discovery_flags=%d,status_poll_type=%d,agent_port=%d,"
                       "auth_method=%d,secret='%s',snmp_oid='%s' WHERE id=%d",
              m_szName, m_iStatus, m_bIsDeleted, m_dwIpAddr, 
              m_dwFlags & NF_IS_SNMP ? 1 : 0,
              m_dwFlags & NF_IS_NATIVE_AGENT ? 1 : 0,
              m_dwFlags & NF_IS_BRIDGE ? 1 : 0,
              m_dwFlags & NF_IS_ROUTER ? 1 : 0,
              m_iSNMPVersion, m_szCommunityString, m_dwDiscoveryFlags, 
              m_iStatusPollType, m_wAgentPort, m_wAuthMethod, m_szSharedSecret, 
              m_szObjectId, m_dwId);
   DBQuery(g_hCoreDB, szQuery);

   // Clear modifications flag and unlock object
   m_bIsModified = FALSE;
   Unlock();

   return TRUE;
}


//
// Delete object from database
//

BOOL Node::DeleteFromDB(void)
{
   char szQuery[256];

   sprintf(szQuery, "DELETE FROM nodes WHERE id=%ld", m_dwId);
   DBQuery(g_hCoreDB, szQuery);
   return TRUE;
}


//
// Poll newly discovered node
// Usually called once by node poller thread when new node is discovered
// and object for it is created
//

BOOL Node::NewNodePoll(DWORD dwNetMask)
{
   // Determine node's capabilities
   if (SnmpGet(m_dwIpAddr, m_szCommunityString, ".1.3.6.1.2.1.1.2.0", NULL, 0, m_szObjectId))
      m_dwFlags |= NF_IS_SNMP;

   // Get interface list
   if (m_dwFlags & NF_IS_SNMP)
   {
      INTERFACE_LIST *pIfList;
      int i;

      pIfList = SnmpGetInterfaceList(m_dwIpAddr, m_szCommunityString);
      for(i = 0; i < pIfList->iNumEntries; i++)
      {
         Interface *pInterface = new Interface(pIfList->pInterfaces[i].szName, 
            pIfList->pInterfaces[i].dwIndex, pIfList->pInterfaces[i].dwIpAddr,
            pIfList->pInterfaces[i].dwIpNetMask, pIfList->pInterfaces[i].dwType);
         NetObjInsert(pInterface, TRUE);
         AddInterface(pInterface);
      }
      DestroyInterfaceList(pIfList);
   }
   else  // No SNMP, no native agent - create pseudo interface object
   {
      Interface *pInterface = new Interface(m_dwIpAddr, dwNetMask);
      NetObjInsert(pInterface, TRUE);
      AddInterface(pInterface);
   }

   return TRUE;
}
