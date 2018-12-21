/*
** NetXMS - Network Management System
** Copyright (C) 2003-2016 Victor Kirhenshtein
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
NetworkService::NetworkService() : super()
{
   m_serviceType = NETSRV_HTTP;
   m_hostNode = NULL;
   m_pollerNode = 0;
   m_proto = IPPROTO_TCP;
   m_port = 80;
   m_request = NULL;
   m_response = NULL;
	m_pendingStatus = -1;
	m_pollCount = 0;
	m_requiredPollCount = 0;	// Use system default
   m_responseTime = 0;
}

/**
 * Extended constructor
 * Note that pszRequest and pszResponse should be dynamically allocated
 * and they will be freed by object's destructor!!!
 */
NetworkService::NetworkService(int iServiceType, WORD wProto, WORD wPort,
                               TCHAR *pszRequest, TCHAR *pszResponse,
										 Node *pHostNode, UINT32 dwPollerNode) : super()
{
   m_serviceType = iServiceType;
   m_hostNode = pHostNode;
   m_pollerNode = dwPollerNode;
   m_proto = wProto;
   m_port = wPort;
   m_request = pszRequest;
   m_response = pszResponse;
	m_pendingStatus = -1;
	m_pollCount = 0;
	m_requiredPollCount = 0;	// Use system default
   m_responseTime = 0;
   m_isHidden = true;
}

/**
 * Destructor
 */
NetworkService::~NetworkService()
{
   free(m_request);
   free(m_response);
}

/**
 * Save object to database
 */
bool NetworkService::saveToDatabase(DB_HANDLE hdb)
{
   lockProperties();

   bool success = saveCommonProperties(hdb);

   if (success && (m_modified & MODIFY_OTHER))
   {
      DB_STATEMENT hStmt;
      if (IsDatabaseRecordExist(hdb, _T("network_services"), _T("id"), m_id))
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
         TCHAR szIpAddr[64];

         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_hostNode->getId());
         DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, (LONG)m_serviceType);
         DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, m_ipAddress.toString(szIpAddr), DB_BIND_STATIC);
         DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, (UINT32)m_proto);
         DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, (UINT32)m_port);
         DBBind(hStmt, 6, DB_SQLTYPE_TEXT, m_request, DB_BIND_STATIC);
         DBBind(hStmt, 7, DB_SQLTYPE_TEXT, m_response, DB_BIND_STATIC);
         DBBind(hStmt, 8, DB_SQLTYPE_INTEGER, m_pollerNode);
         DBBind(hStmt, 9, DB_SQLTYPE_INTEGER, m_requiredPollCount);
         DBBind(hStmt, 10, DB_SQLTYPE_INTEGER, m_id);

         success = DBExecute(hStmt);

         DBFreeStatement(hStmt);
      }
      else
      {
         success = false;
      }
   }

   // Save access list
   if (success)
      success = saveACLToDB(hdb);

   // Unlock object and clear modification flag
   m_modified = 0;
   unlockProperties();
   return success;
}

/**
 * Load properties from database
 */
bool NetworkService::loadFromDatabase(DB_HANDLE hdb, UINT32 dwId)
{
   TCHAR szQuery[256];
   DB_RESULT hResult;
   UINT32 dwHostNodeId;
   NetObj *pObject;
   bool bResult = false;

   m_id = dwId;

   if (!loadCommonProperties(hdb))
      return false;

   _sntprintf(szQuery, 256, _T("SELECT node_id,service_type,")
                               _T("ip_bind_addr,ip_proto,ip_port,check_request,check_responce,")
                               _T("poller_node_id,required_polls FROM network_services WHERE id=%d"), dwId);
   hResult = DBSelect(hdb, szQuery);
   if (hResult == NULL)
      return false;     // Query failed

   if (DBGetNumRows(hResult) != 0)
   {
      dwHostNodeId = DBGetFieldULong(hResult, 0, 0);
      m_serviceType = DBGetFieldLong(hResult, 0, 1);
      m_ipAddress = DBGetFieldInetAddr(hResult, 0, 2);
      m_proto = (WORD)DBGetFieldULong(hResult, 0, 3);
      m_port = (WORD)DBGetFieldULong(hResult, 0, 4);
      m_request = DBGetField(hResult, 0, 5, NULL, 0);
      m_response = DBGetField(hResult, 0, 6, NULL, 0);
      m_pollerNode = DBGetFieldULong(hResult, 0, 7);
      m_requiredPollCount = DBGetFieldULong(hResult, 0, 8);

      // Link service to node
      if (!m_isDeleted)
      {
         // Find host node
         pObject = FindObjectById(dwHostNodeId);
         if (pObject == NULL)
         {
            nxlog_write(MSG_INVALID_NODE_ID_EX, EVENTLOG_ERROR_TYPE, "dds", dwId, dwHostNodeId, _T("network service"));
         }
         else if (pObject->getObjectClass() != OBJECT_NODE)
         {
            nxlog_write(MSG_NODE_NOT_NODE, EVENTLOG_ERROR_TYPE, "dd", dwId, dwHostNodeId);
         }
         else
         {
            m_hostNode = (Node *)pObject;
            pObject->addChild(this);
            addParent(pObject);
            bResult = true;
         }

         // Check that polling node ID is valid
         if ((m_pollerNode != 0) && bResult)
         {
            pObject = FindObjectById(m_pollerNode);
            if (pObject == NULL)
            {
               nxlog_write(MSG_INVALID_NODE_ID_EX, EVENTLOG_ERROR_TYPE, "dds", dwId, m_pollerNode, _T("network service"));
               bResult = false;
            }
            else if (pObject->getObjectClass() != OBJECT_NODE)
            {
               nxlog_write(MSG_NODE_NOT_NODE, EVENTLOG_ERROR_TYPE, "dd", dwId, m_pollerNode);
               bResult = false;
            }
         }
      }
      else
      {
         bResult = true;
      }
   }

   DBFreeResult(hResult);

   // Load access list
   loadACLFromDB(hdb);

   return bResult;
}

/**
 * Delete object from database
 */
bool NetworkService::deleteFromDatabase(DB_HANDLE hdb)
{
   bool success = super::deleteFromDatabase(hdb);
   if (success)
      success = executeQueryOnObject(hdb, _T("DELETE FROM network_services WHERE id=?"));
   return success;
}

/**
 * Create NXCP message with object's data
 */
void NetworkService::fillMessageInternal(NXCPMessage *pMsg, UINT32 userId)
{
   super::fillMessageInternal(pMsg, userId);
   pMsg->setField(VID_SERVICE_TYPE, (WORD)m_serviceType);
   pMsg->setField(VID_IP_ADDRESS, m_ipAddress);
   pMsg->setField(VID_IP_PROTO, m_proto);
   pMsg->setField(VID_IP_PORT, m_port);
   pMsg->setField(VID_POLLER_NODE_ID, m_pollerNode);
   pMsg->setField(VID_SERVICE_REQUEST, CHECK_NULL_EX(m_request));
   pMsg->setField(VID_SERVICE_RESPONSE, CHECK_NULL_EX(m_response));
	pMsg->setField(VID_REQUIRED_POLLS, (WORD)m_requiredPollCount);
	pMsg->setField(VID_RESPONSE_TIME, m_responseTime);
}

/**
 * Modify object from message
 */
UINT32 NetworkService::modifyFromMessageInternal(NXCPMessage *pRequest)
{
   // Polling node
   if (pRequest->isFieldExist(VID_POLLER_NODE_ID))
   {
      UINT32 dwNodeId;

      dwNodeId = pRequest->getFieldAsUInt32(VID_POLLER_NODE_ID);
      if (dwNodeId == 0)
      {
         m_pollerNode = 0;
      }
      else
      {
         NetObj *pObject;

         pObject = FindObjectById(dwNodeId);
         if (pObject != NULL)
         {
            if (pObject->getObjectClass() == OBJECT_NODE)
            {
               m_pollerNode = dwNodeId;
            }
            else
            {
               unlockProperties();
               return RCC_INVALID_OBJECT_ID;
            }
         }
         else
         {
            unlockProperties();
            return RCC_INVALID_OBJECT_ID;
         }
      }
   }

   // Listen IP address
   if (pRequest->isFieldExist(VID_IP_ADDRESS))
      m_ipAddress = pRequest->getFieldAsInetAddress(VID_IP_ADDRESS);

   // Service type
   if (pRequest->isFieldExist(VID_SERVICE_TYPE))
      m_serviceType = (int)pRequest->getFieldAsUInt16(VID_SERVICE_TYPE);

   // IP protocol
   if (pRequest->isFieldExist(VID_IP_PROTO))
      m_proto = pRequest->getFieldAsUInt16(VID_IP_PROTO);

   // TCP/UDP port
   if (pRequest->isFieldExist(VID_IP_PORT))
      m_port = pRequest->getFieldAsUInt16(VID_IP_PORT);

   // Number of required polls
   if (pRequest->isFieldExist(VID_REQUIRED_POLLS))
      m_requiredPollCount = (int)pRequest->getFieldAsUInt16(VID_REQUIRED_POLLS);

   // Check request
   if (pRequest->isFieldExist(VID_SERVICE_REQUEST))
   {
      MemFree(m_request);
      m_request = pRequest->getFieldAsString(VID_SERVICE_REQUEST);
   }

   // Check response
   if (pRequest->isFieldExist(VID_SERVICE_RESPONSE))
   {
      MemFree(m_response);
      m_response = pRequest->getFieldAsString(VID_SERVICE_RESPONSE);
   }

   return super::modifyFromMessageInternal(pRequest);
}

/**
 * Perform status poll on network service
 */
void NetworkService::statusPoll(ClientSession *session, UINT32 rqId, Node *pollerNode, Queue *eventQueue)
{
   int oldStatus = m_status, newStatus;
   Node *pNode;

   m_pollRequestor = session;
   if (m_hostNode == NULL)
   {
      m_status = STATUS_UNKNOWN;
      return;     // Service without host node, which is VERY strange
   }

   sendPollerMsg(rqId, _T("   Starting status poll on network service %s\r\n"), m_name);
   sendPollerMsg(rqId, _T("      Current service status is %s\r\n"), GetStatusAsText(m_status, true));

   if (m_pollerNode != 0)
   {
      pNode = (Node *)FindObjectById(m_pollerNode);
      if (pNode != NULL)
         pNode->incRefCount();
      else
         pNode = pollerNode;
   }
   else
   {
      pNode = pollerNode;
   }

   if (pNode != NULL)
   {
      TCHAR szBuffer[64];
      UINT32 dwStatus;

      sendPollerMsg(rqId, _T("      Polling service from node %s [%s]\r\n"),
                    pNode->getName(), pNode->getIpAddress().toString(szBuffer));
      if (pNode->checkNetworkService(&dwStatus,
                                     m_ipAddress.isValidUnicast() ? m_ipAddress : m_hostNode->getIpAddress(),
                                     m_serviceType, m_port, m_proto,
                                     m_request, m_response, &m_responseTime) == ERR_SUCCESS)
      {
         newStatus = (dwStatus == 0) ? STATUS_NORMAL : STATUS_CRITICAL;
         sendPollerMsg(rqId, _T("      Agent reports service status [%d]\r\n"), dwStatus);
      }
      else
      {
         sendPollerMsg(rqId, _T("      Unable to check service status due to agent or communication error\r\n"));
         newStatus = STATUS_UNKNOWN;
      }

      if (pNode != pollerNode)
         pNode->decRefCount();
   }
   else
   {
      sendPollerMsg(rqId, _T("      Unable to find node object for poll\r\n"));
      newStatus = STATUS_UNKNOWN;
   }

	// Reset status to unknown if node has known network connectivity problems
	if ((newStatus == STATUS_CRITICAL) && (pNode->getState() & DCSF_NETWORK_PATH_PROBLEM))
	{
		newStatus = STATUS_UNKNOWN;
		DbgPrintf(6, _T("StatusPoll(%s): Status for network service %s reset to UNKNOWN"), pNode->getName(), m_name);
	}

   if (newStatus != oldStatus)
   {
		if (newStatus == m_pendingStatus)
		{
			m_pollCount++;
		}
		else
		{
			m_pendingStatus = newStatus;
			m_pollCount = 1;
		}

		if (m_pollCount >= ((m_requiredPollCount > 0) ? m_requiredPollCount : g_requiredPolls))
		{
			m_status = newStatus;
			m_pendingStatus = -1;	// Invalidate pending status
			sendPollerMsg(rqId, _T("      Service status changed to %s\r\n"), GetStatusAsText(m_status, true));
			PostEventEx(eventQueue, m_status == STATUS_NORMAL ? EVENT_SERVICE_UP :
							(m_status == STATUS_CRITICAL ? EVENT_SERVICE_DOWN : EVENT_SERVICE_UNKNOWN),
							m_hostNode->getId(), "sdd", m_name, m_id, m_serviceType);
			lockProperties();
			setModified(MODIFY_RUNTIME);
			unlockProperties();
		}
   }
   sendPollerMsg(rqId, _T("   Finished status poll on network service %s\r\n"), m_name);
}

/**
 * Handler for object deletion
 */
void NetworkService::onObjectDelete(UINT32 objectId)
{
	lockProperties();
   if (objectId == m_pollerNode)
   {
      // If deleted object is our poller node, change it to default
      m_pollerNode = 0;
      setModified(MODIFY_OTHER);
      DbgPrintf(3, _T("NetworkService::onObjectDelete(%s [%u]): poller node %u deleted"), m_name, m_id, objectId);
   }
	unlockProperties();

	super::onObjectDelete(objectId);
}

/**
 * Serialize object to JSON
 */
json_t *NetworkService::toJson()
{
   json_t *root = super::toJson();
   json_object_set_new(root, "serviceType", json_integer(m_serviceType));
   json_object_set_new(root, "pollerNode", json_integer(m_pollerNode));
   json_object_set_new(root, "proto", json_integer(m_proto));
   json_object_set_new(root, "port", json_integer(m_port));
   json_object_set_new(root, "ipAddress", m_ipAddress.toJson());
   json_object_set_new(root, "request", json_string_t(m_request));
   json_object_set_new(root, "response", json_string_t(m_response));
   json_object_set_new(root, "pendingStatus", json_integer(m_pendingStatus));
   json_object_set_new(root, "pollCount", json_integer(m_pollCount));
   json_object_set_new(root, "requiredPollCount", json_integer(m_requiredPollCount));
   json_object_set_new(root, "responseTime", json_integer(m_responseTime));
   return root;
}
