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
   m_pPollNode = NULL;
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
                               Node *pHostNode, Node *pPollNode)
{
   m_iServiceType = iServiceType;
   m_pHostNode = pHostNode;
   m_pPollNode = pPollNode;
   m_wProto = wProto;
   m_wPort = wPort;
   m_pszRequest = pszRequest;
   m_pszResponce = pszResponce;
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

BOOL NetworkService::SaveToDB(void)
{
   TCHAR *pszEscRequest, *pszEscResponce, szQuery[16384], szIpAddr[32];
   DB_RESULT hResult;
   BOOL bNewObject = TRUE;

   Lock();

   // Check for object's existence in database
   sprintf(szQuery, "SELECT id FROM network_services WHERE id=%ld", m_dwId);
   hResult = DBSelect(g_hCoreDB, szQuery);
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
      _sntprintf(szQuery, 16384, _T("INSERT INTO network_services (id,name,status,is_deleted,"
                                    "node_id,service_type,ip_bind_addr,ip_proto,ip_port,"
                                    "check_request,check_responce,poll_node_id,image_id) VALUES "
                                    "(%ld,'%s',%d,%d,%ld,%d,'%s',%d,%d,'%s','%s',%ld,%ld)"),
                 m_dwId, m_szName, m_iStatus, m_bIsDeleted, m_pHostNode->Id(), m_iServiceType,
                 IpToStr(m_dwIpAddr, szIpAddr), m_wProto, m_wPort, pszEscRequest,
                 pszEscResponce, (m_pPollNode == NULL) ? 0 : m_pPollNode->Id(), m_dwImageId);
   else
      _sntprintf(szQuery, 16384, _T("UPDATE network_services SET name='%s',status=%d,"
                                    "is_deleted=%d,node_id=%ld,service_type=%d,ip_bind_addr='%s',"
                                    "ip_proto=%d,ip_port=%d,check_request='%s',"
                                    "check_responce='%s',poll_node_id=%ld,image_id=%ld WHERE id=%ld"),
                 m_szName, m_iStatus, m_bIsDeleted, m_pHostNode->Id(), m_iServiceType,
                 IpToStr(m_dwIpAddr, szIpAddr), m_wProto, m_wPort, pszEscRequest,
                 pszEscResponce, (m_pPollNode == NULL) ? 0 : m_pPollNode->Id(), m_dwImageId, m_dwId);
   free(pszEscRequest);
   free(pszEscResponce);
   DBQuery(g_hCoreDB, szQuery);
                 
   // Save access list
   SaveACLToDB();

   // Unlock object and clear modification flag
   Unlock();
   m_bIsModified = FALSE;
   return TRUE;
}


//
// Load properties from database
//

BOOL NetworkService::CreateFromDB(DWORD dwId)
{
   TCHAR szQuery[256];
   DB_RESULT hResult;
   DWORD dwHostNodeId, dwPollNodeId;
   NetObj *pObject;
   BOOL bResult = FALSE;

   m_dwId = dwId;

   _sntprintf(szQuery, 256, _T("SELECT name,status,is_deleted,node_id,service_type,"
                               "ip_bind_addr,ip_proto,ip_port,check_request,check_responce,"
                               "poll_node_id,image_id FROM network_services WHERE id=%ld"), dwId);
   hResult = DBSelect(g_hCoreDB, szQuery);
   if (hResult == NULL)
      return FALSE;     // Query failed

   if (DBGetNumRows(hResult) != 0)
   {
      strncpy(m_szName, DBGetField(hResult, 0, 0), MAX_OBJECT_NAME);
      m_iStatus = DBGetFieldLong(hResult, 0, 1);
      m_bIsDeleted = DBGetFieldLong(hResult, 0, 2);
      dwHostNodeId = DBGetFieldULong(hResult, 0, 3);
      m_iServiceType = DBGetFieldLong(hResult, 0, 4);
      m_dwIpAddr = DBGetFieldIPAddr(hResult, 0, 5);
      m_wProto = (WORD)DBGetFieldULong(hResult, 0, 6);
      m_wPort = (WORD)DBGetFieldULong(hResult, 0, 7);
      m_pszRequest = _tcsdup(CHECK_NULL_EX(DBGetField(hResult, 0, 8)));
      m_pszResponce = _tcsdup(CHECK_NULL_EX(DBGetField(hResult, 0, 9)));
      dwPollNodeId = DBGetFieldULong(hResult, 0, 10);
      m_dwImageId = DBGetFieldULong(hResult, 0, 11);

      // Link service to node
      if (!m_bIsDeleted)
      {
         // Find host node
         pObject = FindObjectById(dwHostNodeId);
         if (pObject == NULL)
         {
            WriteLog(MSG_INVALID_NODE_ID, EVENTLOG_ERROR_TYPE, "dd", dwId, dwHostNodeId);
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

         // Find polling node
         if ((dwPollNodeId != 0) && bResult)
         {
            pObject = FindObjectById(dwPollNodeId);
            if (pObject == NULL)
            {
               WriteLog(MSG_INVALID_NODE_ID, EVENTLOG_ERROR_TYPE, "dd", dwId, dwPollNodeId);
               bResult = FALSE;
            }
            else if (pObject->Type() != OBJECT_NODE)
            {
               WriteLog(MSG_NODE_NOT_NODE, EVENTLOG_ERROR_TYPE, "dd", dwId, dwPollNodeId);
               bResult = FALSE;
            }
            else
            {
               m_pPollNode = (Node *)pObject;
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
   pMsg->SetVariable(VID_POLLER_NODE_ID, (m_pPollNode == NULL) ? 0 : m_pPollNode->Id());
   pMsg->SetVariable(VID_SERVICE_REQUEST, CHECK_NULL_EX(m_pszRequest));
   pMsg->SetVariable(VID_SERVICE_RESPONCE, CHECK_NULL_EX(m_pszResponce));
}


//
// Modify object from message
//

DWORD NetworkService::ModifyFromMessage(CSCPMessage *pRequest, BOOL bAlreadyLocked)
{
   if (!bAlreadyLocked)
      Lock();

   // Polling node
   if (pRequest->IsVariableExist(VID_POLLER_NODE_ID))
   {
      DWORD dwNodeId;

      dwNodeId = pRequest->GetVariableLong(VID_POLLER_NODE_ID);
      if (dwNodeId == 0)
      {
         m_pPollNode = NULL;
      }
      else
      {
         NetObj *pObject;

         pObject = FindObjectById(dwNodeId);
         if (pObject != NULL)
         {
            if (pObject->Type() == OBJECT_NODE)
            {
               m_pPollNode = (Node *)pObject;
            }
            else
            {
               Unlock();
               return RCC_INVALID_OBJECT_ID;
            }
         }
         else
         {
            Unlock();
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
