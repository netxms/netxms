/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2012 Victor Kirhenshtein
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
** File: netsrv.cpp
**
**/

#include "nxcore.h"

/**
 * Default constructor
 */
NetworkService::NetworkService() : NetObj()
{
   m_iServiceType = NETSRV_HTTP;
   m_pHostNode = NULL;
   m_dwPollerNode = 0;
   m_wProto = IPPROTO_TCP;
   m_wPort = 80;
   m_pszRequest = NULL;
   m_pszResponse = NULL;
	m_iPendingStatus = -1;
	m_iPollCount = 0;
	m_iRequiredPollCount = 0;	// Use system default
}

/**
 * Extended constructor
 * Note that pszRequest and pszResponse should be dynamically allocated
 * and they will be freed by object's destructor!!!
 */
NetworkService::NetworkService(int iServiceType, WORD wProto, WORD wPort,
                               TCHAR *pszRequest, TCHAR *pszResponse,
										 Node *pHostNode, DWORD dwPollerNode) : NetObj()
{
   m_iServiceType = iServiceType;
   m_pHostNode = pHostNode;
   m_dwPollerNode = dwPollerNode;
   m_wProto = wProto;
   m_wPort = wPort;
   m_pszRequest = pszRequest;
   m_pszResponse = pszResponse;
	m_iPendingStatus = -1;
	m_iPollCount = 0;
	m_iRequiredPollCount = 0;	// Use system default
   m_bIsHidden = TRUE;
}

/**
 * Destructor
 */
NetworkService::~NetworkService()
{
   safe_free(m_pszRequest);
   safe_free(m_pszResponse);
}

/**
 * Save object to database
 */
BOOL NetworkService::SaveToDB(DB_HANDLE hdb)
{
   LockData();

   saveCommonProperties(hdb);

   // Form and execute INSERT or UPDATE query
	DB_STATEMENT hStmt;
   if (IsDatabaseRecordExist(hdb, _T("network_services"), _T("id"), m_dwId))
   {
		hStmt = DBPrepare(hdb, _T("UPDATE network_services SET node_id=?,")
                             _T("service_type=?,ip_bind_addr=?,")
                             _T("ip_proto=?,ip_port=?,check_request=?,")
                             _T("check_responce=?,poller_node_id=?,")
		                       _T("required_polls=? WHERE id=?"));
   }
   else
   {
		hStmt = DBPrepare(hdb, _T("INSERT INTO network_services (node_id,")
                             _T("service_type,ip_bind_addr,ip_proto,ip_port,")
                             _T("check_request,check_responce,poller_node_id,")
		                       _T("required_polls,id) VALUES (?,?,?,?,?,?,?,?,?,?)"));
   }
	if (hStmt != NULL)
	{
	   TCHAR szIpAddr[32];

		DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_pHostNode->Id());
		DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, (LONG)m_iServiceType);
		DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, IpToStr(m_dwIpAddr, szIpAddr), DB_BIND_STATIC);
		DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, (DWORD)m_wProto);
		DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, (DWORD)m_wPort);
		DBBind(hStmt, 6, DB_SQLTYPE_TEXT, m_pszRequest, DB_BIND_STATIC);
		DBBind(hStmt, 7, DB_SQLTYPE_TEXT, m_pszResponse, DB_BIND_STATIC);
		DBBind(hStmt, 8, DB_SQLTYPE_INTEGER, m_dwPollerNode);
		DBBind(hStmt, 9, DB_SQLTYPE_INTEGER, (LONG)m_iRequiredPollCount);
		DBBind(hStmt, 10, DB_SQLTYPE_INTEGER, m_dwId);

		DBExecute(hStmt);

		DBFreeStatement(hStmt);
	}
                 
   // Save access list
   saveACLToDB(hdb);

   // Unlock object and clear modification flag
   m_bIsModified = FALSE;
   UnlockData();
   return TRUE;
}

/**
 * Load properties from database
 */
BOOL NetworkService::CreateFromDB(DWORD dwId)
{
   TCHAR szQuery[256];
   DB_RESULT hResult;
   DWORD dwHostNodeId;
   NetObj *pObject;
   BOOL bResult = FALSE;

   m_dwId = dwId;

   if (!loadCommonProperties())
      return FALSE;

   _sntprintf(szQuery, 256, _T("SELECT node_id,service_type,")
                               _T("ip_bind_addr,ip_proto,ip_port,check_request,check_responce,")
                               _T("poller_node_id,required_polls FROM network_services WHERE id=%d"), dwId);
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
      m_pszRequest = DBGetField(hResult, 0, 5, NULL, 0);
      m_pszResponse = DBGetField(hResult, 0, 6, NULL, 0);
      m_dwPollerNode = DBGetFieldULong(hResult, 0, 7);
      m_iRequiredPollCount = DBGetFieldLong(hResult, 0, 8);

      // Link service to node
      if (!m_bIsDeleted)
      {
         // Find host node
         pObject = FindObjectById(dwHostNodeId);
         if (pObject == NULL)
         {
            nxlog_write(MSG_INVALID_NODE_ID_EX, EVENTLOG_ERROR_TYPE, "dds", dwId, dwHostNodeId, _T("network service"));
         }
         else if (pObject->Type() != OBJECT_NODE)
         {
            nxlog_write(MSG_NODE_NOT_NODE, EVENTLOG_ERROR_TYPE, "dd", dwId, dwHostNodeId);
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
               nxlog_write(MSG_INVALID_NODE_ID_EX, EVENTLOG_ERROR_TYPE,
                        "dds", dwId, m_dwPollerNode, _T("network service"));
               bResult = FALSE;
            }
            else if (pObject->Type() != OBJECT_NODE)
            {
               nxlog_write(MSG_NODE_NOT_NODE, EVENTLOG_ERROR_TYPE, "dd", dwId, m_dwPollerNode);
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
   loadACLFromDB();

   return bResult;
}

/**
 * Delete object from database
 */
BOOL NetworkService::DeleteFromDB()
{
   TCHAR szQuery[128];
   BOOL bSuccess;

   bSuccess = NetObj::DeleteFromDB();
   if (bSuccess)
   {
      _sntprintf(szQuery, 128, _T("DELETE FROM network_services WHERE id=%d"), m_dwId);
      QueueSQLRequest(szQuery);
   }
   return bSuccess;
}

/**
 * Create NXCP message with object's data
 */
void NetworkService::CreateMessage(CSCPMessage *pMsg)
{
   NetObj::CreateMessage(pMsg);
   pMsg->SetVariable(VID_SERVICE_TYPE, (WORD)m_iServiceType);
   pMsg->SetVariable(VID_IP_PROTO, m_wProto);
   pMsg->SetVariable(VID_IP_PORT, m_wPort);
   pMsg->SetVariable(VID_POLLER_NODE_ID, m_dwPollerNode);
   pMsg->SetVariable(VID_SERVICE_REQUEST, CHECK_NULL_EX(m_pszRequest));
   pMsg->SetVariable(VID_SERVICE_RESPONSE, CHECK_NULL_EX(m_pszResponse));
	pMsg->SetVariable(VID_REQUIRED_POLLS, (WORD)m_iRequiredPollCount);
}

/**
 * Modify object from message
 */
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

   // Number of required polls
   if (pRequest->IsVariableExist(VID_REQUIRED_POLLS))
      m_iRequiredPollCount = (int)pRequest->GetVariableShort(VID_REQUIRED_POLLS);

   // Check request
   if (pRequest->IsVariableExist(VID_SERVICE_REQUEST))
   {
      safe_free(m_pszRequest);
      m_pszRequest = pRequest->GetVariableStr(VID_SERVICE_REQUEST);
   }

   // Check response
   if (pRequest->IsVariableExist(VID_SERVICE_RESPONSE))
   {
      safe_free(m_pszResponse);
      m_pszResponse = pRequest->GetVariableStr(VID_SERVICE_RESPONSE);
   }

   return NetObj::ModifyFromMessage(pRequest, TRUE);
}

/**
 * Perform status poll on network service
 */
void NetworkService::StatusPoll(ClientSession *pSession, DWORD dwRqId, Node *pPollerNode, Queue *pEventQueue)
{
   int oldStatus = m_iStatus, newStatus;
   Node *pNode;

   m_pPollRequestor = pSession;
   if (m_pHostNode == NULL)
   {
      m_iStatus = STATUS_UNKNOWN;
      return;     // Service without host node, which is VERY strange
   }

   SendPollerMsg(dwRqId, _T("   Starting status poll on network service %s\r\n"), m_szName);
   SendPollerMsg(dwRqId, _T("      Current service status is %s\r\n"), g_szStatusTextSmall[m_iStatus]);

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

      SendPollerMsg(dwRqId, _T("      Polling service from node %s [%s]\r\n"),
                    pNode->Name(), IpToStr(pNode->IpAddr(), szBuffer));
      if (pNode->CheckNetworkService(&dwStatus, 
                                     (m_dwIpAddr == 0) ? m_pHostNode->IpAddr() : m_dwIpAddr,
                                     m_iServiceType, m_wPort, m_wProto, 
                                     m_pszRequest, m_pszResponse) == ERR_SUCCESS)
      {
         newStatus = (dwStatus == 0) ? STATUS_NORMAL : STATUS_CRITICAL;
         SendPollerMsg(dwRqId, _T("      Agent reports service status [%d]\r\n"), dwStatus);
      }
      else
      {
         SendPollerMsg(dwRqId, _T("      Unable to check service status due to agent or communication error\r\n"));
         newStatus = STATUS_UNKNOWN;
      }

      if (pNode != pPollerNode)
         pNode->DecRefCount();
   }
   else
   {
      SendPollerMsg(dwRqId, _T("      Unable to find node object for poll\r\n"));
      newStatus = STATUS_UNKNOWN;
   }

	// Reset status to unknown if node has known network connectivity problems
	if ((newStatus == STATUS_CRITICAL) && (pNode->getRuntimeFlags() & NDF_NETWORK_PATH_PROBLEM))
	{
		newStatus = STATUS_UNKNOWN;
		DbgPrintf(6, _T("StatusPoll(%s): Status for network service %s reset to UNKNOWN"), pNode->Name(), m_szName);
	}
   
   if (newStatus != oldStatus)
   {
		if (newStatus == m_iPendingStatus)
		{
			m_iPollCount++;
		}
		else
		{
			m_iPendingStatus = newStatus;
			m_iPollCount = 1;
		}

		if (m_iPollCount >= ((m_iRequiredPollCount > 0) ? m_iRequiredPollCount : g_nRequiredPolls))
		{
			m_iStatus = newStatus;
			m_iPendingStatus = -1;	// Invalidate pending status
			SendPollerMsg(dwRqId, _T("      Service status changed to %s\r\n"), g_szStatusTextSmall[m_iStatus]);
			PostEventEx(pEventQueue, m_iStatus == STATUS_NORMAL ? EVENT_SERVICE_UP : 
							(m_iStatus == STATUS_CRITICAL ? EVENT_SERVICE_DOWN : EVENT_SERVICE_UNKNOWN),
							m_pHostNode->Id(), "sdd", m_szName, m_dwId, m_iServiceType);
			LockData();
			Modify();
			UnlockData();
		}
   }
   SendPollerMsg(dwRqId, _T("   Finished status poll on network service %s\r\n"), m_szName);
}

/**
 * Handler for object deletion
 */
void NetworkService::OnObjectDelete(DWORD dwObjectId)
{
	LockData();
   if (dwObjectId == m_dwPollerNode)
   {
      // If deleted object is our poller node, change it to default
      m_dwPollerNode = 0;
      Modify();
      DbgPrintf(3, _T("Service \"%s\": poller node %d deleted"), m_szName, dwObjectId);
   }
	UnlockData();
}
