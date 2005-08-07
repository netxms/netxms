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
** $module: netsrv.cpp
**
**/

#include "nxcore.h"


//
// Default constructor
//

NetworkService::NetworkService()
               :NetObj()
{
   m_iServiceType = NETSRV_HTTP;
   m_pHostNode = NULL;
   m_dwPollerNode = 0;
   m_wProto = IPPROTO_TCP;
   m_wPort = 80;
   m_pszRequest = NULL;
   m_pszResponce = NULL;
}


//
// Extended constructor
// Note that pszRequest and pszResponce should be dynamically allocated
// and they will be freed by object's destructor!!!
//

NetworkService::NetworkService(int iServiceType, WORD wProto, WORD wPort,
                               TCHAR *pszRequest, TCHAR *pszResponce,
                               Node *pHostNode, DWORD dwPollerNode)
{
   m_iServiceType = iServiceType;
   m_pHostNode = pHostNode;
   m_dwPollerNode = dwPollerNode;
   m_wProto = wProto;
   m_wPort = wPort;
   m_pszRequest = pszRequest;
   m_pszResponce = pszResponce;
   m_bIsHidden = TRUE;
}


//
// Destructor
//

NetworkService::~NetworkService()
{
   safe_free(m_pszRequest);
   safe_free(m_pszResponce);
}


//
// Save object to database
//

BOOL NetworkService::SaveToDB(DB_HANDLE hdb)
{
   TCHAR *pszEscRequest, *pszEscResponce, szQuery[16384], szIpAddr[32];
   DB_RESULT hResult;
   BOOL bNewObject = TRUE;

   LockData();

   SaveCommonProperties(hdb);

   // Check for object's existence in database
   sprintf(szQuery, "SELECT id FROM network_services WHERE id=%ld", m_dwId);
   hResult = DBSelect(hdb, szQuery);
   if (hResult != 0)
   {
      if (DBGetNumRows(hResult) > 0)
         bNewObject = FALSE;
      DBFreeResult(hResult);
   }

   // Form and execute INSERT or UPDATE query
   pszEscRequest = EncodeSQLString(CHECK_NULL_EX(m_pszRequest));
   pszEscResponce = EncodeSQLString(CHECK_NULL_EX(m_pszResponce));
   if (bNewObject)
      _sntprintf(szQuery, 16384, _T("INSERT INTO network_services (id,node_id,"
                                    "service_type,ip_bind_addr,ip_proto,ip_port,"
                                    "check_request,check_responce,poller_node_id) VALUES "
                                    "(%ld,%ld,%d,'%s',%d,%d,'%s','%s',%ld)"),
                 m_dwId, m_pHostNode->Id(), m_iServiceType,
                 IpToStr(m_dwIpAddr, szIpAddr), m_wProto, m_wPort, pszEscRequest,
                 pszEscResponce, m_dwPollerNode);
   else
      _sntprintf(szQuery, 16384, _T("UPDATE network_services SET node_id=%ld,"
                                    "service_type=%d,ip_bind_addr='%s',"
                                    "ip_proto=%d,ip_port=%d,check_request='%s',"
                                    "check_responce='%s',poller_node_id=%ld WHERE id=%ld"),
                 m_pHostNode->Id(), m_iServiceType,
                 IpToStr(m_dwIpAddr, szIpAddr), m_wProto, m_wPort, pszEscRequest,
                 pszEscResponce, m_dwPollerNode, m_dwId);
   free(pszEscRequest);
   free(pszEscResponce);
   DBQuery(hdb, szQuery);
                 
   // Save access list
   SaveACLToDB(hdb);

   // Unlock object and clear modification flag
   m_bIsModified = FALSE;
   UnlockData();
   return TRUE;
}


//
// Load properties from database
//

BOOL NetworkService::CreateFromDB(DWORD dwId)
{
   TCHAR szQuery[256];
   DB_RESULT hResult;
   DWORD dwHostNodeId;
   NetObj *pObject;
   BOOL bResult = FALSE;

   m_dwId = dwId;

   if (!LoadCommonProperties())
      return FALSE;

   _sntprintf(szQuery, 256, _T("SELECT node_id,service_type,"
                               "ip_bind_addr,ip_proto,ip_port,check_request,check_responce,"
                               "poller_node_id FROM network_services WHERE id=%ld"), dwId);
   hResult = DBSelect(g_hCoreDB, szQuery);
   if (hResult == NULL)
      return FALSE;     // Query failed

   if (DBGetNumRows(hResult) != 0)
   {
      dwHostNodeId = DBGetFieldULong(hResult, 0, 0);
      m_iServiceType = DBGetFieldLong(hResult, 0, 1);
      m_dwIpAddr = DBGetFieldIPAddr(hResult, 0, 2);
      m_wProto = (WORD)DBGetFieldULong(hResult, 0, 3);
      m_wPort = (WORD)DBGetFieldULong(hResult, 0, 4);
      m_pszRequest = _tcsdup(CHECK_NULL_EX(DBGetField(hResult, 0, 5)));
      DecodeSQLString(m_pszRequest);
      m_pszResponce = _tcsdup(CHECK_NULL_EX(DBGetField(hResult, 0, 6)));
      DecodeSQLString(m_pszResponce);
      m_dwPollerNode = DBGetFieldULong(hResult, 0, 7);

      // Link service to node
      if (!m_bIsDeleted)
      {
         // Find host node
         pObject = FindObjectById(dwHostNodeId);
         if (pObject == NULL)
         {
            WriteLog(MSG_INVALID_NODE_ID_EX, EVENTLOG_ERROR_TYPE, "dds",
                     dwId, dwHostNodeId, "network service");
         }
         else if (pObject->Type() != OBJECT_NODE)
         {
            WriteLog(MSG_NODE_NOT_NODE, EVENTLOG_ERROR_TYPE, "dd", dwId, dwHostNodeId);
         }
         else
         {
            m_pHostNode = (Node *)pObject;
            pObject->AddChild(this);
            AddParent(pObject);
            bResult = TRUE;
         }

         // Check that polling node ID is valid
         if ((m_dwPollerNode != 0) && bResult)
         {
            pObject = FindObjectById(m_dwPollerNode);
            if (pObject == NULL)
            {
               WriteLog(MSG_INVALID_NODE_ID_EX, EVENTLOG_ERROR_TYPE,
                        "dds", dwId, m_dwPollerNode, "network service");
               bResult = FALSE;
            }
            else if (pObject->Type() != OBJECT_NODE)
            {
               WriteLog(MSG_NODE_NOT_NODE, EVENTLOG_ERROR_TYPE, "dd", dwId, m_dwPollerNode);
               bResult = FALSE;
            }
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
// Delete object from database
//

BOOL NetworkService::DeleteFromDB(void)
{
   TCHAR szQuery[128];
   BOOL bSuccess;

   bSuccess = NetObj::DeleteFromDB();
   if (bSuccess)
   {
      _sntprintf(szQuery, 128, _T("DELETE FROM network_services WHERE id=%ld"), m_dwId);
      QueueSQLRequest(szQuery);
   }
   return bSuccess;
}


//
// Create CSCP message with object's data
//

void NetworkService::CreateMessage(CSCPMessage *pMsg)
{
   NetObj::CreateMessage(pMsg);
   pMsg->SetVariable(VID_SERVICE_TYPE, (WORD)m_iServiceType);
   pMsg->SetVariable(VID_IP_PROTO, m_wProto);
   pMsg->SetVariable(VID_IP_PORT, m_wPort);
   pMsg->SetVariable(VID_POLLER_NODE_ID, m_dwPollerNode);
   pMsg->SetVariable(VID_SERVICE_REQUEST, CHECK_NULL_EX(m_pszRequest));
   pMsg->SetVariable(VID_SERVICE_RESPONCE, CHECK_NULL_EX(m_pszResponce));
}


//
// Modify object from message
//

DWORD NetworkService::ModifyFromMessage(CSCPMessage *pRequest, BOOL bAlreadyLocked)
{
   if (!bAlreadyLocked)
      LockData();

   // Polling node
   if (pRequest->IsVariableExist(VID_POLLER_NODE_ID))
   {
      DWORD dwNodeId;

      dwNodeId = pRequest->GetVariableLong(VID_POLLER_NODE_ID);
      if (dwNodeId == 0)
      {
         m_dwPollerNode = 0;
      }
      else
      {
         NetObj *pObject;

         pObject = FindObjectById(dwNodeId);
         if (pObject != NULL)
         {
            if (pObject->Type() == OBJECT_NODE)
            {
               m_dwPollerNode = dwNodeId;
            }
            else
            {
               UnlockData();
               return RCC_INVALID_OBJECT_ID;
            }
         }
         else
         {
            UnlockData();
            return RCC_INVALID_OBJECT_ID;
         }
      }
   }

   // Listen IP address
   if (pRequest->IsVariableExist(VID_IP_ADDRESS))
      m_dwIpAddr = pRequest->GetVariableLong(VID_IP_ADDRESS);

   // Service type
   if (pRequest->IsVariableExist(VID_SERVICE_TYPE))
      m_iServiceType = (int)pRequest->GetVariableShort(VID_SERVICE_TYPE);

   // IP protocol
   if (pRequest->IsVariableExist(VID_IP_PROTO))
      m_wProto = pRequest->GetVariableShort(VID_IP_PROTO);

   // TCP/UDP port
   if (pRequest->IsVariableExist(VID_IP_PORT))
      m_wPort = pRequest->GetVariableShort(VID_IP_PORT);

   // Check request
   if (pRequest->IsVariableExist(VID_SERVICE_REQUEST))
   {
      safe_free(m_pszRequest);
      m_pszRequest = pRequest->GetVariableStr(VID_SERVICE_REQUEST);
   }

   // Check responce
   if (pRequest->IsVariableExist(VID_SERVICE_RESPONCE))
   {
      safe_free(m_pszResponce);
      m_pszResponce = pRequest->GetVariableStr(VID_SERVICE_RESPONCE);
   }

   return NetObj::ModifyFromMessage(pRequest, TRUE);
}


//
// Perform status poll on network service
//

void NetworkService::StatusPoll(ClientSession *pSession, DWORD dwRqId,
                                Node *pPollerNode, Queue *pEventQueue)
{
   int iOldStatus = m_iStatus;
   Node *pNode;

   m_pPollRequestor = pSession;
   if (m_pHostNode == NULL)
   {
      m_iStatus = STATUS_UNKNOWN;
      return;     // Service without host node, which is VERY strange
   }

   SendPollerMsg(dwRqId, "   Starting status poll on network service %s\r\n"
                         "      Current status is %s\r\n",
                 m_szName, g_pszStatusName[m_iStatus]);

   if (m_dwPollerNode != 0)
   {
      pNode = (Node *)FindObjectById(m_dwPollerNode);
      if (pNode != NULL)
         pNode->IncRefCount();
      else
         pNode = pPollerNode;
   }
   else
   {
      pNode = pPollerNode;
   }

   if (pNode != NULL)
   {
      TCHAR szBuffer[16];
      DWORD dwStatus;

      SendPollerMsg(dwRqId, "      Polling service from node %s [%s]\r\n",
                    pNode->Name(), IpToStr(pNode->IpAddr(), szBuffer));
      if (pNode->CheckNetworkService(&dwStatus, 
                                     (m_dwIpAddr == 0) ? m_pHostNode->IpAddr() : m_dwIpAddr,
                                     m_iServiceType, m_wPort, m_wProto, 
                                     m_pszRequest, m_pszResponce) == ERR_SUCCESS)
      {
         m_iStatus = (dwStatus == 0) ? STATUS_NORMAL : STATUS_CRITICAL;
         SendPollerMsg(dwRqId, "      Agent reports service status [%d]\r\n", dwStatus);
      }
      else
      {
         SendPollerMsg(dwRqId, "      Unable to check service status due to agent or communication error\r\n");
         m_iStatus = STATUS_UNKNOWN;
      }

      if (pNode != pPollerNode)
         pNode->DecRefCount();
   }
   else
   {
      SendPollerMsg(dwRqId, "      Unable to find node object for poll\r\n");
      m_iStatus = STATUS_UNKNOWN;
   }

   if (m_iStatus != iOldStatus)
   {
      SendPollerMsg(dwRqId, "      Service status changed to %s\r\n", g_pszStatusName[m_iStatus]);
      PostEventEx(pEventQueue, m_iStatus == STATUS_NORMAL ? EVENT_SERVICE_UP : 
                  (m_iStatus == STATUS_CRITICAL ? EVENT_SERVICE_DOWN : EVENT_SERVICE_UNKNOWN),
                  m_pHostNode->Id(), "sdd", m_szName, m_dwId, m_iServiceType);
      Modify();
   }
   SendPollerMsg(dwRqId, "   Finished status poll on network service %s\r\n", m_szName);
}


//
// Handler for object deletion
//

void NetworkService::OnObjectDelete(DWORD dwObjectId)
{
   if (dwObjectId == m_dwPollerNode)
   {
      // If deleted object is our poller node, change it to default
      m_dwPollerNode = 0;
      Modify();
      DbgPrintf(AF_DEBUG_MISC, _T("Service \"%s\": poller node %ld deleted"), m_szName, dwObjectId);
   }
}
