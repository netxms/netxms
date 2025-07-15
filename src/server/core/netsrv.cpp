/*
** NetXMS - Network Management System
** Copyright (C) 2003-2025 Victor Kirhenshtein
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

#define DEBUG_TAG_NETSVC   _T("netsvc")

/**
 * Default constructor
 */
NetworkService::NetworkService() : super()
{
   m_serviceType = NETSRV_HTTP;
   m_pollerNode = 0;
   m_proto = IPPROTO_TCP;
   m_port = 80;
   m_request = nullptr;
   m_response = nullptr;
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
										 shared_ptr<Node> hostNode, uint32_t dwPollerNode) : super(), m_hostNode(hostNode)
{
   m_serviceType = iServiceType;
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
   setCreationTime();
}

/**
 * Destructor
 */
NetworkService::~NetworkService()
{
   MemFree(m_request);
   MemFree(m_response);
}

/**
 * Save object to database
 */
bool NetworkService::saveToDatabase(DB_HANDLE hdb)
{
   bool success = super::saveToDatabase(hdb);

   if (success && (m_modified & MODIFY_OTHER))
   {
      static const TCHAR *columns[] = {
         _T("node_id"), _T("service_type"), _T("ip_bind_addr"), _T("ip_proto"), _T("ip_port"), _T("check_request"),
         _T("check_response"), _T("poller_node_id"), _T("required_polls"), nullptr
      };

      DB_STATEMENT hStmt = DBPrepareMerge(hdb, _T("network_services"), _T("id"), m_id, columns);
      if (hStmt == nullptr)
         return false;

      lockProperties();

      DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_hostNode.lock()->getId());
      DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, (int32_t)m_serviceType);
      DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, m_ipAddress);
      DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, (uint32_t)m_proto);
      DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, (uint32_t)m_port);
      DBBind(hStmt, 6, DB_SQLTYPE_TEXT, m_request, DB_BIND_STATIC);
      DBBind(hStmt, 7, DB_SQLTYPE_TEXT, m_response, DB_BIND_STATIC);
      DBBind(hStmt, 8, DB_SQLTYPE_INTEGER, m_pollerNode);
      DBBind(hStmt, 9, DB_SQLTYPE_INTEGER, m_requiredPollCount);
      DBBind(hStmt, 10, DB_SQLTYPE_INTEGER, m_id);

      success = DBExecute(hStmt);

      DBFreeStatement(hStmt);

      unlockProperties();
   }

   return success;
}

/**
 * Load properties from database
 */
bool NetworkService::loadFromDatabase(DB_HANDLE hdb, uint32_t id, DB_STATEMENT *preparedStatements)
{
   TCHAR szQuery[256];
   DB_RESULT hResult;
   UINT32 dwHostNodeId;
   bool bResult = false;

   m_id = id;

   if (!loadCommonProperties(hdb, preparedStatements))
      return false;

   _sntprintf(szQuery, 256, _T("SELECT node_id,service_type,")
                               _T("ip_bind_addr,ip_proto,ip_port,check_request,check_response,")
                               _T("poller_node_id,required_polls FROM network_services WHERE id=%d"), id);
   hResult = DBSelect(hdb, szQuery);
   if (hResult == nullptr)
      return false;     // Query failed

   if (DBGetNumRows(hResult) != 0)
   {
      dwHostNodeId = DBGetFieldULong(hResult, 0, 0);
      m_serviceType = DBGetFieldLong(hResult, 0, 1);
      m_ipAddress = DBGetFieldInetAddr(hResult, 0, 2);
      m_proto = (WORD)DBGetFieldULong(hResult, 0, 3);
      m_port = (WORD)DBGetFieldULong(hResult, 0, 4);
      m_request = DBGetField(hResult, 0, 5, nullptr, 0);
      m_response = DBGetField(hResult, 0, 6, nullptr, 0);
      m_pollerNode = DBGetFieldULong(hResult, 0, 7);
      m_requiredPollCount = DBGetFieldULong(hResult, 0, 8);

      // Link service to node
      if (!m_isDeleted)
      {
         // Find host node
         shared_ptr<NetObj> hostNode = static_pointer_cast<Node>(FindObjectById(dwHostNodeId, OBJECT_NODE));
         if (hostNode != nullptr)
         {
            m_hostNode = static_pointer_cast<Node>(hostNode);
            linkObjects(hostNode, self());
            bResult = true;
         }
         else
         {
            nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG_NETSVC, _T("Inconsistent database: network service %s [%u] linked to non-existent node [%u]"), m_name, m_id, dwHostNodeId);
         }

         // Check that polling node ID is valid
         if ((m_pollerNode != 0) && bResult)
         {
            if (FindObjectById(m_pollerNode, OBJECT_NODE) == nullptr)
            {
               nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG_NETSVC, _T("Inconsistent database: network service %s [%u] use non-existent poller node [%u]"), m_name, m_id, m_pollerNode);
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

   loadACLFromDB(hdb, preparedStatements);

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
void NetworkService::fillMessageLocked(NXCPMessage *msg, uint32_t userId)
{
   super::fillMessageLocked(msg, userId);
   msg->setField(VID_SERVICE_TYPE, (WORD)m_serviceType);
   msg->setField(VID_IP_ADDRESS, m_ipAddress);
   msg->setField(VID_IP_PROTO, m_proto);
   msg->setField(VID_IP_PORT, m_port);
   msg->setField(VID_POLLER_NODE_ID, m_pollerNode);
   msg->setField(VID_SERVICE_REQUEST, CHECK_NULL_EX(m_request));
   msg->setField(VID_SERVICE_RESPONSE, CHECK_NULL_EX(m_response));
	msg->setField(VID_REQUIRED_POLLS, (WORD)m_requiredPollCount);
	msg->setField(VID_RESPONSE_TIME, m_responseTime);
}

/**
 * Modify object from message
 */
uint32_t NetworkService::modifyFromMessageInternal(const NXCPMessage& msg, ClientSession *session)
{
   // Polling node
   if (msg.isFieldExist(VID_POLLER_NODE_ID))
   {
      uint32_t nodeId = msg.getFieldAsUInt32(VID_POLLER_NODE_ID);
      if (nodeId == 0)
      {
         m_pollerNode = 0;
      }
      else
      {
         shared_ptr<NetObj> pObject = FindObjectById(nodeId);
         if (pObject != nullptr)
         {
            if (pObject->getObjectClass() == OBJECT_NODE)
            {
               m_pollerNode = nodeId;
            }
            else
            {
               unlockProperties();
               return RCC_INCOMPATIBLE_OPERATION;
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
   if (msg.isFieldExist(VID_IP_ADDRESS))
      m_ipAddress = msg.getFieldAsInetAddress(VID_IP_ADDRESS);

   // Service type
   if (msg.isFieldExist(VID_SERVICE_TYPE))
      m_serviceType = msg.getFieldAsInt16(VID_SERVICE_TYPE);

   // IP protocol
   if (msg.isFieldExist(VID_IP_PROTO))
      m_proto = msg.getFieldAsUInt16(VID_IP_PROTO);

   // TCP/UDP port
   if (msg.isFieldExist(VID_IP_PORT))
      m_port = msg.getFieldAsUInt16(VID_IP_PORT);

   // Number of required polls
   if (msg.isFieldExist(VID_REQUIRED_POLLS))
      m_requiredPollCount = msg.getFieldAsUInt16(VID_REQUIRED_POLLS);

   // Check request
   if (msg.isFieldExist(VID_SERVICE_REQUEST))
   {
      MemFree(m_request);
      m_request = msg.getFieldAsString(VID_SERVICE_REQUEST);
   }

   // Check response
   if (msg.isFieldExist(VID_SERVICE_RESPONSE))
   {
      MemFree(m_response);
      m_response = msg.getFieldAsString(VID_SERVICE_RESPONSE);
   }

   return super::modifyFromMessageInternal(msg, session);
}

/**
 * Save object before maintenance
 */
void NetworkService::saveStateBeforeMaintenance()
{
   lockProperties();
   m_stateBeforeMaintenance = m_status;
   setModified(MODIFY_COMMON_PROPERTIES);
   unlockProperties();
}

/**
 * Regenerate events after maintenance end
 */
void NetworkService::generateEventsAfterMaintenace()
{
   shared_ptr<Node> hostNode = m_hostNode.lock();
   if (hostNode == nullptr)
   {
      m_status = STATUS_UNKNOWN;
      return;     // Service without host node, which is VERY strange
   }

   lockProperties();
   if (m_stateBeforeMaintenance != m_status)
   {
      if (m_pollCount >= ((m_requiredPollCount > 0) ? m_requiredPollCount : g_requiredPolls))
      {
         EventBuilder(m_status == STATUS_NORMAL ? EVENT_SERVICE_UP :
                     (m_status == STATUS_CRITICAL ? EVENT_SERVICE_DOWN : EVENT_SERVICE_UNKNOWN),
                     hostNode->getId())
            .param(_T("serviceName"), m_name)
            .param(_T("serviceObjectId"), m_id)
            .param(_T("serviceType"), m_serviceType)
            .post();
      }
   }
   unlockProperties();
}

/**
 * Perform status poll on network service
 */
void NetworkService::statusPoll(ClientSession *session, uint32_t rqId, const shared_ptr<Node>& pollerNode, ObjectQueue<Event> *eventQueue)
{
   m_pollRequestor = session;
   m_pollRequestId = rqId;

   shared_ptr<Node> hostNode = m_hostNode.lock();
   if (hostNode == nullptr)
   {
      m_status = STATUS_UNKNOWN;
      return;     // Service without host node, which is VERY strange
   }

   sendPollerMsg(_T("   Starting status poll on network service %s\r\n"), m_name);
   sendPollerMsg(_T("      Current service status is %s\r\n"), GetStatusAsText(m_status, true));

   int oldStatus = m_status, newStatus;

   shared_ptr<Node> node;
   if (m_pollerNode != 0)
   {
      node = static_pointer_cast<Node>(FindObjectById(m_pollerNode, OBJECT_NODE));
      if (node == nullptr)
         node = pollerNode;
   }
   else
   {
      node = pollerNode;
   }

   if (node != nullptr)
   {
      TCHAR szBuffer[64];
      uint32_t dwStatus;

      sendPollerMsg(_T("      Polling service from node %s [%s]\r\n"), node->getName(), node->getIpAddress().toString(szBuffer));
      if (node->checkNetworkService(&dwStatus,
                                     m_ipAddress.isValidUnicast() ? m_ipAddress : hostNode->getIpAddress(),
                                     m_serviceType, m_port, m_proto,
                                     m_request, m_response, &m_responseTime) == ERR_SUCCESS)
      {
         newStatus = (dwStatus == 0) ? STATUS_NORMAL : STATUS_CRITICAL;
         sendPollerMsg(_T("      Agent reports service status [%d]\r\n"), dwStatus);
      }
      else
      {
         sendPollerMsg(_T("      Unable to check service status due to agent or communication error\r\n"));
         newStatus = STATUS_UNKNOWN;
      }
   }
   else
   {
      sendPollerMsg(_T("      Unable to find node object for poll\r\n"));
      newStatus = STATUS_UNKNOWN;
   }

	// Reset status to unknown if node has known network connectivity problems
	if ((newStatus == STATUS_CRITICAL) && (node != nullptr) && (node->getState() & DCSF_NETWORK_PATH_PROBLEM))
	{
		newStatus = STATUS_UNKNOWN;
		nxlog_debug_tag(DEBUG_TAG_STATUS_POLL, 6, _T("StatusPoll(%s): Status for network service %s reset to UNKNOWN"), node->getName(), m_name);
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
			sendPollerMsg(_T("      Service status changed to %s\r\n"), GetStatusAsText(m_status, true));
         EventBuilder(m_status == STATUS_NORMAL ? EVENT_SERVICE_UP :
							(m_status == STATUS_CRITICAL ? EVENT_SERVICE_DOWN : EVENT_SERVICE_UNKNOWN),
							hostNode->getId())
            .param(_T("serviceName"), m_name)
            .param(_T("serviceObjectId"), m_id)
            .param(_T("serviceType"), m_serviceType)
            .post(eventQueue);

			lockProperties();
			setModified(MODIFY_RUNTIME);
			unlockProperties();
		}
   }
   sendPollerMsg(_T("   Finished status poll on network service %s\r\n"), m_name);
}

/**
 * Handler for object deletion
 */
void NetworkService::onObjectDelete(const NetObj& object)
{
	lockProperties();
   if (object.getId() == m_pollerNode)
   {
      // If deleted object is our poller node, change it to default
      m_pollerNode = 0;
      setModified(MODIFY_OTHER);
      nxlog_debug_tag(DEBUG_TAG_NETSVC, 3, _T("NetworkService::onObjectDelete(%s [%u]): poller node %s [%u] deleted"), m_name, m_id, object.getName(), object.getId());
   }
	unlockProperties();

	super::onObjectDelete(object);
}

/**
 * Serialize object to JSON
 */
json_t *NetworkService::toJson()
{
   json_t *root = super::toJson();

   lockProperties();

   json_object_set_new(root, "serviceType", json_integer(m_serviceType));
   json_object_set_new(root, "pollerNode", json_integer(m_pollerNode));
   json_object_set_new(root, "protocol", json_integer(m_proto));
   json_object_set_new(root, "port", json_integer(m_port));
   json_object_set_new(root, "ipAddress", m_ipAddress.toJson());
   json_object_set_new(root, "request", json_string_t(m_request));
   json_object_set_new(root, "response", json_string_t(m_response));
   json_object_set_new(root, "pendingStatus", json_integer(m_pendingStatus));
   json_object_set_new(root, "pollCount", json_integer(m_pollCount));
   json_object_set_new(root, "requiredPollCount", json_integer(m_requiredPollCount));
   json_object_set_new(root, "responseTime", json_integer(m_responseTime));

   unlockProperties();
   return root;
}
