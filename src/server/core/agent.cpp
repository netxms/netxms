/*
** NetXMS - Network Management System
** Copyright (C) 2003-2020 Victor Kirhenshtein
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
** File: agent.cpp
**
**/

#include "nxcore.h"
#include <agent_tunnel.h>

/**
 * Externals
 */
void ProcessTrap(SNMP_PDU *pdu, const InetAddress& srcAddr, int32_t zoneUIN, int srcPort, SNMP_Transport *pTransport, SNMP_Engine *localEngine, bool isInformRq);
void QueueProxiedSyslogMessage(const InetAddress &addr, int32_t zoneUIN, uint32_t nodeId, time_t timestamp, const char *msg, int msgLen);
void QueueWindowsEvent(WindowsEvent *event);

/**
 * Create normal agent connection
 */
AgentConnectionEx::AgentConnectionEx(uint32_t nodeId, const InetAddress& ipAddr, uint16_t port, const TCHAR *secret, bool allowCompression) :
         AgentConnection(ipAddr, port, secret, allowCompression)
{
   m_nodeId = nodeId;
   m_tunnel = nullptr;
   m_proxyTunnel = nullptr;
   m_tcpProxySession = nullptr;
}

/**
 * Create agent connection within tunnel
 */
AgentConnectionEx::AgentConnectionEx(uint32_t nodeId, AgentTunnel *tunnel, const TCHAR *secret, bool allowCompression) :
         AgentConnection(InetAddress::INVALID, 0, secret, allowCompression)
{
   m_nodeId = nodeId;
   m_tunnel = tunnel;
   m_tunnel->incRefCount();
   m_proxyTunnel = nullptr;
   m_tcpProxySession = nullptr;
}

/**
 * Destructor for extended agent connection class
 */
AgentConnectionEx::~AgentConnectionEx()
{
   if (m_tunnel != nullptr)
      m_tunnel->decRefCount();
   if (m_proxyTunnel != nullptr)
      m_proxyTunnel->decRefCount();
}

/**
 * Create communication channel
 */
AbstractCommChannel *AgentConnectionEx::createChannel()
{
   if (m_tunnel != nullptr)
      return m_tunnel->createChannel();
   if (isProxyMode() && (m_proxyTunnel != nullptr))
      return m_proxyTunnel->createChannel();
   return AgentConnection::createChannel();
}

/**
 * Set tunnel to use
 */
void AgentConnectionEx::setTunnel(AgentTunnel *tunnel)
{
   if (m_tunnel != nullptr)
      m_tunnel->decRefCount();
   m_tunnel = tunnel;
   if (m_tunnel != nullptr)
      m_tunnel->incRefCount();
}

/**
 * Set proxy tunnel to use
 */
void AgentConnectionEx::setProxy(AgentTunnel *tunnel, const TCHAR *secret)
{
   if (m_proxyTunnel != nullptr)
      m_proxyTunnel->decRefCount();
   m_proxyTunnel = tunnel;
   if (m_proxyTunnel != nullptr)
      m_proxyTunnel->incRefCount();
   setProxy(InetAddress::INVALID, 0, secret);
}

/**
 * Trap processor
 */
void AgentConnectionEx::onTrap(NXCPMessage *pMsg)
{
   if (IsShutdownInProgress())
      return;

   TCHAR szBuffer[64];
   debugPrintf(3, _T("AgentConnectionEx::onTrap(): Received trap message from agent at %s, node ID %d"), getIpAddr().toString(szBuffer), m_nodeId);

   shared_ptr<Node> node;
   if (m_nodeId != 0)
	   node = static_pointer_cast<Node>(FindObjectById(m_nodeId, OBJECT_NODE));
	if (node == nullptr)
      node = FindNodeByIP(0, getIpAddr());
   if (node != nullptr)
   {
      if (node->getStatus() != STATUS_UNMANAGED)
      {
		   // Check for duplicate traps - only accept traps with ID
		   // higher than last received
		   // agents prior to 1.1.6 will not send trap id
		   // we should accept trap in that case to maintain compatibility
		   bool acceptTrap;
		   QWORD trapId = pMsg->getFieldAsUInt64(VID_TRAP_ID);
		   if (trapId != 0)
		   {
			   acceptTrap = node->checkAgentTrapId(trapId);
			   debugPrintf(5, _T("AgentConnectionEx::onTrap(): trapID is%s valid"), acceptTrap ? _T("") : _T(" not"));
		   }
		   else
		   {
			   acceptTrap = true;
			   debugPrintf(5, _T("AgentConnectionEx::onTrap(): trap ID not provided"));
		   }

		   if (acceptTrap)
		   {
			   UINT32 dwEventCode = pMsg->getFieldAsUInt32(VID_EVENT_CODE);
			   if ((dwEventCode == 0) && pMsg->isFieldExist(VID_EVENT_NAME))
			   {
				   TCHAR eventName[256];
				   pMsg->getFieldAsString(VID_EVENT_NAME, eventName, 256);
				   dwEventCode = EventCodeFromName(eventName, 0);
			   }
			   debugPrintf(3, _T("Event from trap: %d"), dwEventCode);

			   StringList parameters(pMsg, VID_EVENT_ARG_BASE, VID_NUM_ARGS);
			   PostEvent(dwEventCode, EventOrigin::AGENT, pMsg->getFieldAsTime(VID_TIMESTAMP), node->getId(), parameters);
		   }
      }
      else
      {
         debugPrintf(3, _T("AgentConnectionEx::onTrap(): node %s [%d] in in UNMANAGED state - trap ignored"), node->getName(), node->getId());
      }
   }
   else
   {
      debugPrintf(3, _T("AgentConnectionEx::onTrap(): Cannot find node for IP address %s"), getIpAddr().toString(szBuffer));
   }
}

/**
 * Incoming syslog message processor
 */
void AgentConnectionEx::onSyslogMessage(const NXCPMessage& msg)
{
   if (IsShutdownInProgress())
      return;

   TCHAR buffer[64];
   debugPrintf(3, _T("AgentConnectionEx::onSyslogMessage(): Received message from agent at %s, node ID %d"), getIpAddr().toString(buffer), m_nodeId);

   int32_t zoneUIN = msg.getFieldAsUInt32(VID_ZONE_UIN);
   shared_ptr<Node> node;
   if (m_nodeId != 0)
      node = static_pointer_cast<Node>(FindObjectById(m_nodeId, OBJECT_NODE));
   if (node == nullptr)
      node = FindNodeByIP(zoneUIN, getIpAddr());
   if (node != nullptr)
   {
      // Check for duplicate messages - only accept messages with ID
      // higher than last received
      if (node->checkSyslogMessageId(msg.getFieldAsUInt64(VID_REQUEST_ID)))
      {
         int msgLen = msg.getFieldAsInt32(VID_MESSAGE_LENGTH);
         if (msgLen < 2048)
         {
            char message[2048];
            msg.getFieldAsBinary(VID_MESSAGE, (BYTE *)message, msgLen + 1);
            InetAddress sourceAddr = msg.getFieldAsInetAddress(VID_IP_ADDRESS);
            uint32_t sourceNodeId = 0;
            if (sourceAddr.isLoopback())
            {
               debugPrintf(5, _T("Source IP address for syslog message is loopback, setting source node ID to %d"), m_nodeId);
               sourceNodeId = m_nodeId;
            }
            QueueProxiedSyslogMessage(sourceAddr, zoneUIN, sourceNodeId, msg.getFieldAsTime(VID_TIMESTAMP), message, msgLen);
         }
      }
      else
      {
         debugPrintf(5, _T("AgentConnectionEx::onSyslogMessage(): message ID is invalid (node %s [%d])"), node->getName(), node->getId());
      }
   }
   else
   {
      debugPrintf(5, _T("AgentConnectionEx::onSyslogMessage(): Cannot find node for IP address %s"), getIpAddr().toString(buffer));
   }
}

/**
 * Incoming Windows events processor
 */
void AgentConnectionEx::onWindowsEvent(const NXCPMessage& msg)
{
   if (IsShutdownInProgress())
      return;

   TCHAR buffer[64];
   debugPrintf(3, _T("AgentConnectionEx::onWindowsEvent(): Received event from agent at %s, node ID %d"),
         getIpAddr().toString(buffer), m_nodeId);

   int32_t zoneUIN = msg.getFieldAsUInt32(VID_ZONE_UIN);
   shared_ptr<Node> node;
   if (m_nodeId != 0)
      node = static_pointer_cast<Node>(FindObjectById(m_nodeId, OBJECT_NODE));
   if (node == nullptr)
      node = FindNodeByIP(zoneUIN, getIpAddr());
   if (node != nullptr)
   {
      // Check for duplicate messages - only accept messages with ID
      // higher than last received
      if (node->checkWindowsEventId(msg.getFieldAsUInt64(VID_REQUEST_ID)))
      {
         QueueWindowsEvent(new WindowsEvent(node->getId(), zoneUIN, msg));
      }
      else
      {
         debugPrintf(5, _T("AgentConnectionEx::onWindowsEvent(): event ID is invalid (node %s [%d])"), node->getName(), node->getId());
      }
   }
   else
   {
      debugPrintf(5, _T("AgentConnectionEx::onWindowsEvent(): Cannot find node for IP address %s"), getIpAddr().toString(buffer));
   }
}

/**
 * Handler for data push
 */
void AgentConnectionEx::onDataPush(NXCPMessage *msg)
{
   if (IsShutdownInProgress())
      return;

	TCHAR name[MAX_PARAM_NAME], value[MAX_RESULT_LENGTH];
	msg->getFieldAsString(VID_NAME, name, MAX_PARAM_NAME);
	msg->getFieldAsString(VID_VALUE, value, MAX_RESULT_LENGTH);

   shared_ptr<Node> sender;
	if (m_nodeId != 0)
		sender = static_pointer_cast<Node>(FindObjectById(m_nodeId, OBJECT_NODE));
   if (sender == nullptr)
      sender = FindNodeByIP(0, getIpAddr());

	if (sender != nullptr)
	{
		// Check for duplicate data requests - only accept requests with ID
		// higher than last received
		// agents prior to 1.2.10 will not send request id
		// we should accept data in that case to maintain compatibility
		bool acceptRequest;
		QWORD requestId = msg->getFieldAsUInt64(VID_REQUEST_ID);
		if (requestId != 0)
		{
			acceptRequest = sender->checkAgentPushRequestId(requestId);
			debugPrintf(5, _T("AgentConnectionEx::onDataPush(): requestId is%s valid"), acceptRequest ? _T("") : _T(" not"));
		}
		else
		{
			acceptRequest = true;
			debugPrintf(5, _T("AgentConnectionEx::onDataPush(): request ID not provided"));
		}

		if (acceptRequest)
		{
         shared_ptr<Node> target;
         UINT32 objectId = msg->getFieldAsUInt32(VID_OBJECT_ID);
         if (objectId != 0)
         {
            // push on behalf of other node
            target = static_pointer_cast<Node>(FindObjectById(objectId, OBJECT_NODE));
            if (target != nullptr)
            {
               if (target->isTrustedNode(sender->getId()))
               {
                  DbgPrintf(5, _T("%s: agent data push: target set to %s [%d]"), sender->getName(), target->getName(), target->getId());
               }
               else
               {
                  DbgPrintf(5, _T("%s: agent data push: not in trusted node list for target %s [%d]"), sender->getName(), target->getName(), target->getId());
                  target.reset();
               }
            }
         }
         else
         {
            target = sender;
         }

         if (target != nullptr)
         {
		      DbgPrintf(5, _T("%s: agent data push: %s=%s"), target->getName(), name, value);
		      shared_ptr<DCObject> dci = target->getDCObjectByName(name, 0);
		      if ((dci != nullptr) && (dci->getType() == DCO_TYPE_ITEM) && (dci->getDataSource() == DS_PUSH_AGENT) && (dci->getStatus() == ITEM_STATUS_ACTIVE))
		      {
			      DbgPrintf(5, _T("%s: agent data push: found DCI %d"), target->getName(), dci->getId());
               time_t t = msg->getFieldAsTime(VID_TIMESTAMP);
               if (t == 0)
			         t = time(nullptr);
			      target->processNewDCValue(dci, t, value);
               if (t > dci->getLastPollTime())
			         dci->setLastPollTime(t);
		      }
		      else
		      {
		         debugPrintf(5, _T("%s: agent data push: DCI not found for %s"), target->getName(), name);
		      }
         }
         else
         {
            debugPrintf(5, _T("%s: agent data push: target node not found or not accessible"), sender->getName());
         }
      }
	}
}

/**
 * Cancel unknown file monitoring
 */
static void CancelUnknownFileMonitoring(Node *object,TCHAR *remoteFile)
{
   nxlog_debug(6, _T("AgentConnectionEx::onFileMonitoringData: unknown subscription will be canceled"));
   shared_ptr<AgentConnection> conn = object->createAgentConnection();
   if (conn != nullptr)
   {
      NXCPMessage request(conn->getProtocolVersion());
      request.setId(conn->generateRequestId());
      request.setCode(CMD_CANCEL_FILE_MONITORING);
      request.setField(VID_FILE_NAME, remoteFile);
      request.setField(VID_OBJECT_ID, object->getId());
      NXCPMessage* response = conn->customRequest(&request);
      delete response;
   }
}

/**
 * Receive file monitoring information and resend to all required user sessions
 */
void AgentConnectionEx::onFileMonitoringData(NXCPMessage *pMsg)
{
   if (IsShutdownInProgress())
      return;

	shared_ptr<Node> object;
	if (m_nodeId != 0)
		object = static_pointer_cast<Node>(FindObjectById(m_nodeId, OBJECT_NODE));
	if (object != nullptr)
	{
	   TCHAR remoteFile[MAX_PATH];
      pMsg->getFieldAsString(VID_FILE_NAME, remoteFile, MAX_PATH);
      ObjectArray<ClientSession> *result = g_monitoringList.findClientByFNameAndNodeID(remoteFile, object->getId());
      debugPrintf(6, _T("AgentConnectionEx::onFileMonitoringData: found %d sessions for remote file %s on node %s [%d]"), result->size(), remoteFile, object->getName(), object->getId());
      int validSessionCount = result->size();
      for(int i = 0; i < result->size(); i++)
      {
         if (!result->get(i)->sendMessage(pMsg))
         {
            MONITORED_FILE file;
            _tcslcpy(file.fileName, remoteFile, MAX_PATH);
            file.nodeID = m_nodeId;
            file.session = result->get(i);
            g_monitoringList.removeFile(&file);
            validSessionCount--;

            if (validSessionCount == 0)
               CancelUnknownFileMonitoring(object.get(), remoteFile);
         }
      }
      if (result->size() == 0)
      {
         CancelUnknownFileMonitoring(object.get(), remoteFile);
      }
      delete result;
	}
	else
	{
		g_monitoringList.removeDisconnectedNode(m_nodeId);
		debugPrintf(6, _T("AgentConnectionEx::onFileMonitoringData: node object not found"));
	}
}

/**
 * Ask modules if they can process custom message
 */
bool AgentConnectionEx::processCustomMessage(NXCPMessage *msg)
{
   if (IsShutdownInProgress())
      return false;

   TCHAR buffer[128];
   DbgPrintf(6, _T("AgentConnectionEx::processCustomMessage: processing message %s ID %d"),
      NXCPMessageCodeName(msg->getCode(), buffer), msg->getId());

   ENUMERATE_MODULES(pfOnAgentMessage)
   {
      if (CURRENT_MODULE.pfOnAgentMessage(msg, m_nodeId))
         return true;    // accepted by module
   }
   return false;
}

/**
 * Create SNMP proxy transport for sending trap response
 */
static SNMP_ProxyTransport *CreateSNMPProxyTransport(const shared_ptr<AgentConnectionEx>& conn, Node *originNode, const InetAddress& originAddr, UINT16 port)
{
   SNMP_ProxyTransport *snmpTransport = new SNMP_ProxyTransport(conn, originAddr, port);
   if (originNode != nullptr)
   {
      snmpTransport->setSecurityContext(originNode->getSnmpSecurityContext());
   }
   return snmpTransport;
}

/**
 * Recieve trap sent throught proxy agent
 */
void AgentConnectionEx::onSnmpTrap(NXCPMessage *msg)
{
   if (IsShutdownInProgress())
      return;

   shared_ptr<Node> proxyNode;
   TCHAR ipStringBuffer[4096];

   static BYTE engineId[] = { 0x80, 0x00, 0x00, 0x00, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01, 0x00 };
	SNMP_Engine localEngine(engineId, 12);

	debugPrintf(3, _T("AgentConnectionEx::onSnmpTrap(): Received SNMP trap message from agent at %s, node ID %d"),
      getIpAddr().toString(ipStringBuffer), m_nodeId);

	if (m_nodeId != 0)
		proxyNode = static_pointer_cast<Node>(FindObjectById(m_nodeId, OBJECT_NODE));
   if (proxyNode != nullptr)
   {
      // Check for duplicate traps - only accept traps with ID
      // higher than last received
      bool acceptTrap;
      UINT32 trapId = msg->getId();
      if (trapId != 0)
      {
         acceptTrap = proxyNode->checkSNMPTrapId(trapId);
         debugPrintf(5, _T("AgentConnectionEx::onSnmpTrap(): SNMP trapID is%s valid"), acceptTrap ? _T("") : _T(" not"));
      }
      else
      {
         acceptTrap = false;
         debugPrintf(5, _T("AgentConnectionEx::onSnmpTrap(): SNMP trap ID not provided"));
      }

      if (acceptTrap)
      {
         InetAddress originSenderIP = msg->getFieldAsInetAddress(VID_IP_ADDRESS);
         size_t pduLenght;
         const BYTE *pduBytes = msg->getBinaryFieldPtr(VID_PDU, &pduLenght);
         int32_t zoneUIN;
         if (IsZoningEnabled())
         {
            shared_ptr<Zone> zone = FindZoneByProxyId(m_nodeId);
            zoneUIN = (zone != nullptr) ? zone->getUIN() : msg->getFieldAsInt32(VID_ZONE_UIN);
         }
         else
         {
            zoneUIN = 0;
         }
         shared_ptr<Node> originNode = FindNodeByIP(zoneUIN, originSenderIP);

         // Do not check for node existence here - it will be checked by ProcessTrap

         SNMP_PDU pdu;
         SNMP_SecurityContext *sctx = (originNode != nullptr) ? originNode->getSnmpSecurityContext() : nullptr;
         if (pdu.parse(pduBytes, pduLenght, sctx, true))
         {
            nxlog_debug(6, _T("SNMPTrapReceiver: received PDU of type %d"), pdu.getCommand());
            if ((pdu.getCommand() == SNMP_TRAP) || (pdu.getCommand() == SNMP_INFORM_REQUEST))
            {
               bool isInformRequest = (pdu.getCommand() == SNMP_INFORM_REQUEST);
               SNMP_ProxyTransport *snmpTransport = isInformRequest ? CreateSNMPProxyTransport(self(), originNode.get(), originSenderIP, msg->getFieldAsUInt16(VID_PORT)) : nullptr;
               if ((pdu.getVersion() == SNMP_VERSION_3) && (pdu.getCommand() == SNMP_INFORM_REQUEST))
               {
                  SNMP_SecurityContext *context = snmpTransport->getSecurityContext();
                  context->setAuthoritativeEngine(localEngine);
               }
               if (snmpTransport != nullptr)
                  snmpTransport->setWaitForResponse(false);
               ProcessTrap(&pdu, originSenderIP, zoneUIN, msg->getFieldAsUInt16(VID_PORT), snmpTransport, &localEngine, isInformRequest);
               delete snmpTransport;
            }
            else if ((pdu.getVersion() == SNMP_VERSION_3) && (pdu.getCommand() == SNMP_GET_REQUEST) && (pdu.getAuthoritativeEngine().getIdLen() == 0))
            {
               // Engine ID discovery
               nxlog_debug(6, _T("SNMPTrapReceiver: EngineId discovery"));

               SNMP_ProxyTransport *snmpTransport = CreateSNMPProxyTransport(self(), originNode.get(), originSenderIP, msg->getFieldAsUInt16(VID_PORT));

               SNMP_PDU response(SNMP_REPORT, pdu.getRequestId(), pdu.getVersion());
               response.setReportable(false);
               response.setMessageId(pdu.getMessageId());
               response.setContextEngineId(localEngine.getId(), localEngine.getIdLen());

               SNMP_Variable *var = new SNMP_Variable(_T(".1.3.6.1.6.3.15.1.1.4.0"));
               var->setValueFromString(ASN_INTEGER, _T("2"));
               response.bindVariable(var);

               SNMP_SecurityContext *context = new SNMP_SecurityContext();
               localEngine.setTime((int)time(nullptr));
               context->setAuthoritativeEngine(localEngine);
               context->setSecurityModel(SNMP_SECURITY_MODEL_USM);
               context->setAuthMethod(SNMP_AUTH_NONE);
               context->setPrivMethod(SNMP_ENCRYPT_NONE);
               snmpTransport->setSecurityContext(context);

               snmpTransport->setWaitForResponse(false);
               snmpTransport->sendMessage(&response, 0);
               delete snmpTransport;
            }
            else if (pdu.getCommand() == SNMP_REPORT)
            {
               debugPrintf(6, _T("AgentConnectionEx::onSnmpTrap(): REPORT PDU with error %s"), pdu.getVariable(0)->getName().toString().cstr());
            }
         }
         else
         {
            debugPrintf(6, _T("AgentConnectionEx::onSnmpTrap(): error parsing PDU"));
         }
         delete sctx;
      }
   }
   else
   {
      debugPrintf(3, _T("AgentConnectionEx::onSnmpTrap(): Cannot find node for IP address %s"), getIpAddr().toString(ipStringBuffer));
   }
}

/**
 * Deploy policy to agent
 */
uint32_t AgentConnectionEx::deployPolicy(NXCPMessage *msg)
{
   uint32_t rqId, rcc;

   rqId = generateRequestId();
   msg->setId(rqId);
	msg->setCode(CMD_DEPLOY_AGENT_POLICY);
   if (sendMessage(msg))
   {
      rcc = waitForRCC(rqId, getCommandTimeout());
   }
   else
   {
      rcc = ERR_CONNECTION_BROKEN;
   }
   return rcc;
}

/**
 * Uninstall policy from agent
 */
uint32_t AgentConnectionEx::uninstallPolicy(uuid guid, const TCHAR *type, bool newTypeFormatSupported)
{
   uint32_t rqId, rcc;
	NXCPMessage msg(getProtocolVersion());

   rqId = generateRequestId();
   msg.setId(rqId);
	msg.setCode(CMD_UNINSTALL_AGENT_POLICY);
	if (newTypeFormatSupported)
   {
	   msg.setField(VID_POLICY_TYPE, type);
   }
   else
   {
      if (!_tcscmp(type, _T("AgentConfig")))
      {
         msg.setField(VID_POLICY_TYPE, AGENT_POLICY_CONFIG);
      }
      else if (!_tcscmp(type, _T("LogParserConfig")))
      {
         msg.setField(VID_POLICY_TYPE, AGENT_POLICY_LOG_PARSER);
      }
   }
   msg.setField(VID_GUID, guid);
   if (sendMessage(&msg))
   {
      rcc = waitForRCC(rqId, getCommandTimeout());
   }
   else
   {
      rcc = ERR_CONNECTION_BROKEN;
   }
   return rcc;
}

/**
 * Process collected data information (for DCI with agent-side cache)
 */
UINT32 AgentConnectionEx::processCollectedData(NXCPMessage *msg)
{
   if (IsShutdownInProgress())
      return ERR_INTERNAL_ERROR;

   if (m_nodeId == 0)
   {
      debugPrintf(5, _T("AgentConnectionEx::processCollectedData: node ID is 0 for agent session"));
      return ERR_INTERNAL_ERROR;
   }

	shared_ptr<Node> node = static_pointer_cast<Node>(FindObjectById(m_nodeId, OBJECT_NODE));
   if (node == nullptr)
   {
      debugPrintf(5, _T("AgentConnectionEx::processCollectedData: cannot find node object (node ID = %d)"), m_nodeId);
      return ERR_INTERNAL_ERROR;
   }

   int origin = msg->getFieldAsInt16(VID_DCI_SOURCE_TYPE);
   if ((origin != DS_NATIVE_AGENT) && (origin != DS_SNMP_AGENT))
   {
      debugPrintf(5, _T("AgentConnectionEx::processCollectedData: unsupported data source type %d"), origin);
      return ERR_INTERNAL_ERROR;
   }

   // Check that server is not overloaded with DCI data
   INT64 queueSize = GetIDataWriterQueueSize();
   if (queueSize > 250000)
   {
      debugPrintf(5, _T("AgentConnectionEx::processCollectedData: database writer queue is too large (%d) - cannot accept new data"), queueSize);
      return ERR_RESOURCE_BUSY;
   }

   shared_ptr<DataCollectionTarget> target;
   uuid targetId = msg->getFieldAsGUID(VID_NODE_ID);
   if (!targetId.isNull())
   {
      shared_ptr<NetObj> object = FindObjectByGUID(targetId, -1);
      if (object == nullptr)
      {
         TCHAR buffer[64];
         debugPrintf(5, _T("AgentConnectionEx::processCollectedData: cannot find target node with GUID %s"), targetId.toString(buffer));
         return ERR_INTERNAL_ERROR;
      }
      if (!object->isDataCollectionTarget())
      {
         TCHAR buffer[64];
         debugPrintf(5, _T("AgentConnectionEx::processCollectedData: object with GUID %s is not a data collection target"), targetId.toString(buffer));
         return ERR_INTERNAL_ERROR;
      }
      target = static_pointer_cast<DataCollectionTarget>(object);
   }
   else
   {
      target = node;
   }

   UINT32 dciId = msg->getFieldAsUInt32(VID_DCI_ID);
   shared_ptr<DCObject> dcObject = target->getDCObjectById(dciId, 0);
   if (dcObject == nullptr)
   {
      debugPrintf(5, _T("AgentConnectionEx::processCollectedData: cannot find DCI with ID %d on object %s [%d]"),
                  dciId, target->getName(), target->getId());
      return ERR_INTERNAL_ERROR;
   }

   int type = msg->getFieldAsInt16(VID_DCOBJECT_TYPE);
   if ((dcObject->getType() != type) || (dcObject->getDataSource() != origin) || (dcObject->getAgentCacheMode() != AGENT_CACHE_ON))
   {
      debugPrintf(5, _T("AgentConnectionEx::processCollectedData: DCI %s [%d] on object %s [%d] configuration mismatch"),
                  dcObject->getName().cstr(), dciId, target->getName(), target->getId());
      return ERR_INTERNAL_ERROR;
   }

   time_t t = msg->getFieldAsTime(VID_TIMESTAMP);
   UINT32 status = msg->getFieldAsUInt32(VID_STATUS);
   bool success = true;

   debugPrintf(7, _T("AgentConnectionEx::processCollectedData: processing DCI %s [%d] (type=%d) (status=%d) on object %s [%d]"),
               dcObject->getName().cstr(), dciId, type, status, target->getName(), target->getId());

   switch(status)
   {
      case ERR_SUCCESS:
      {
         void *value;
         switch(type)
         {
            case DCO_TYPE_ITEM:
               value = msg->getFieldAsString(VID_VALUE);
               break;
            case DCO_TYPE_LIST:
               value = new StringList();
               break;
            case DCO_TYPE_TABLE:
               value = new Table(msg);
               break;
            default:
               debugPrintf(5, _T("AgentConnectionEx::processCollectedData: invalid type %d of DCI %s [%d] on object %s [%d]"),
                           type, dcObject->getName().cstr(), dciId, target->getName(), target->getId());
               return ERR_INTERNAL_ERROR;
         }

         if (dcObject->getStatus() == ITEM_STATUS_NOT_SUPPORTED)
            dcObject->setStatus(ITEM_STATUS_ACTIVE, true);
         success = target->processNewDCValue(dcObject, t, value);
         if (t > dcObject->getLastPollTime())
            dcObject->setLastPollTime(t);

         switch(type)
         {
            case DCO_TYPE_ITEM:
               MemFree(value);
               break;
            case DCO_TYPE_LIST:
               delete static_cast<StringList*>(value);
               break;
            case DCO_TYPE_TABLE:
               // DCTable will keep ownership of created table
               break;
         }
         break;
      }
      case ERR_UNKNOWN_PARAMETER:
         if (dcObject->getStatus() == ITEM_STATUS_NOT_SUPPORTED)
            dcObject->setStatus(ITEM_STATUS_ACTIVE, true);
         dcObject->processNewError(false, t);
         break;
      case ERR_NO_SUCH_INSTANCE:
         if (dcObject->getStatus() == ITEM_STATUS_NOT_SUPPORTED)
            dcObject->setStatus(ITEM_STATUS_ACTIVE, true);
         dcObject->processNewError(true, t);
         break;
      case ERR_INTERNAL_ERROR:
         dcObject->processNewError(true, t);
         break;
   }

   return success ? ERR_SUCCESS : ERR_INTERNAL_ERROR;
}

/**
 * Process collected data information in bulk mode (for DCI with agent-side cache)
 */
UINT32 AgentConnectionEx::processBulkCollectedData(NXCPMessage *request, NXCPMessage *response)
{
   if (IsShutdownInProgress())
      return ERR_INTERNAL_ERROR;

   if (m_nodeId == 0)
   {
      debugPrintf(5, _T("AgentConnectionEx::processBulkCollectedData: node ID is 0 for agent session"));
      return ERR_INTERNAL_ERROR;
   }

   shared_ptr<Node> node = static_pointer_cast<Node>(FindObjectById(m_nodeId, OBJECT_NODE));
   if (node == nullptr)
   {
      debugPrintf(5, _T("AgentConnectionEx::processBulkCollectedData: cannot find node object (node ID = %d)"), m_nodeId);
      return ERR_INTERNAL_ERROR;
   }

   // Check that server is not overloaded with DCI data
   INT64 queueSize = GetIDataWriterQueueSize();
   if (queueSize > 250000)
   {
      debugPrintf(5, _T("AgentConnectionEx::processBulkCollectedData: database writer queue is too large (%d) - cannot accept new data"), queueSize);
      return ERR_RESOURCE_BUSY;
   }

   int count = request->getFieldAsInt16(VID_NUM_ELEMENTS);
   if (count > MAX_BULK_DATA_BLOCK_SIZE)
      count = MAX_BULK_DATA_BLOCK_SIZE;
   debugPrintf(5, _T("AgentConnectionEx::processBulkCollectedData: %d elements from node %s [%d]"), count, node->getName(), node->getId());

   // Use half timeout for sending progress updates
   UINT32 agentTimeout = request->getFieldAsUInt32(VID_TIMEOUT) / 2;

   BYTE status[MAX_BULK_DATA_BLOCK_SIZE];
   memset(status, 0, MAX_BULK_DATA_BLOCK_SIZE);
   UINT32 fieldId = VID_ELEMENT_LIST_BASE;
   INT64 startTime = GetCurrentTimeMs();
   for(int i = 0; i < count; i++, fieldId += 10)
   {
      UINT32 elapsed = static_cast<UINT32>(GetCurrentTimeMs() - startTime);
      if ((agentTimeout > 0) && (elapsed >= agentTimeout)) // agent timeout 0 means that agent will not understand processing notifications
      {
         NXCPMessage msg(CMD_REQUEST_COMPLETED, request->getId(), getProtocolVersion());
         msg.setField(VID_RCC, ERR_PROCESSING);
         msg.setField(VID_PROGRESS, i * 100 / count);
         postRawMessage(msg.serialize(isCompressionAllowed()));
         startTime = GetCurrentTimeMs();
      }

      if (IsShutdownInProgress())
      {
         status[i] = BULK_DATA_REC_RETRY;
         continue;
      }

      int origin = request->getFieldAsInt16(fieldId + 1);
      if ((origin != DS_NATIVE_AGENT) && (origin != DS_SNMP_AGENT))
      {
         debugPrintf(5, _T("AgentConnectionEx::processBulkCollectedData: unsupported data source type %d (element %d)"), origin, i);
         status[i] = BULK_DATA_REC_FAILURE;
         continue;
      }

      shared_ptr<DataCollectionTarget> target;
      uuid targetId = request->getFieldAsGUID(fieldId + 3);
      if (!targetId.isNull())
      {
         shared_ptr<NetObj> object = FindObjectByGUID(targetId, -1);
         if (object == nullptr)
         {
            TCHAR buffer[64];
            debugPrintf(5, _T("AgentConnectionEx::processBulkCollectedData: cannot find target object with GUID %s (element %d)"),
                        targetId.toString(buffer), i);
            status[i] = BULK_DATA_REC_FAILURE;
            continue;
         }
         if (!object->isDataCollectionTarget())
         {
            TCHAR buffer[64];
            debugPrintf(5, _T("AgentConnectionEx::processBulkCollectedData: object with GUID %s (element %d) is not a data collection target"),
                        targetId.toString(buffer), i);
            status[i] = BULK_DATA_REC_FAILURE;
            continue;
         }
         target = static_pointer_cast<DataCollectionTarget>(object);
      }
      else
      {
         target = node;
      }

      uint32_t dciId = request->getFieldAsUInt32(fieldId);
      shared_ptr<DCObject> dcObject = target->getDCObjectById(dciId, 0);
      if (dcObject == nullptr)
      {
         debugPrintf(5, _T("AgentConnectionEx::processBulkCollectedData: cannot find DCI with ID %d on object %s [%d] (element %d)"),
                     dciId, target->getName(), target->getId(), i);
         status[i] = BULK_DATA_REC_FAILURE;
         continue;
      }

      int type = request->getFieldAsInt16(fieldId + 2);
      if ((type != DCO_TYPE_ITEM) || (dcObject->getType() != type) || (dcObject->getDataSource() != origin) || (dcObject->getAgentCacheMode() != AGENT_CACHE_ON))
      {
         debugPrintf(5, _T("AgentConnectionEx::processBulkCollectedData: DCI %s [%d] on object %s [%d] configuration mismatch (element %d)"),
                     dcObject->getName().cstr(), dciId, target->getName(), target->getId(), i);
         status[i] = BULK_DATA_REC_FAILURE;
         continue;
      }

      void *value = request->getFieldAsString(fieldId + 5);
      uint32_t statusCode = request->getFieldAsUInt32(fieldId + 6);
      debugPrintf(7, _T("AgentConnectionEx::processBulkCollectedData: processing DCI %s [%d] (type=%d) (status=%d) on object %s [%d] (element %d)"),
                  dcObject->getName().cstr(), dciId, type, statusCode, target->getName(), target->getId(), i);
      time_t t = request->getFieldAsTime(fieldId + 4);
      bool success = true;

      switch(statusCode)
      {
         case ERR_SUCCESS:
            if (dcObject->getStatus() == ITEM_STATUS_NOT_SUPPORTED)
               dcObject->setStatus(ITEM_STATUS_ACTIVE, true);
            success = target->processNewDCValue(dcObject, t, value);
            if (t > dcObject->getLastPollTime())
               dcObject->setLastPollTime(t);
            break;
         case ERR_UNKNOWN_PARAMETER:
            if (dcObject->getStatus() == ITEM_STATUS_NOT_SUPPORTED)
               dcObject->setStatus(ITEM_STATUS_ACTIVE, true);
            dcObject->processNewError(false, t);
            break;
         case ERR_NO_SUCH_INSTANCE:
            if (dcObject->getStatus() == ITEM_STATUS_NOT_SUPPORTED)
               dcObject->setStatus(ITEM_STATUS_ACTIVE, true);
            dcObject->processNewError(true, t);
            break;
         case ERR_INTERNAL_ERROR:
            dcObject->processNewError(true, t);
            break;
      }

      status[i] = success ? BULK_DATA_REC_SUCCESS : BULK_DATA_REC_FAILURE;
      MemFree(value);
   }

   response->setField(VID_STATUS, status, count);
   return ERR_SUCCESS;
}

/**
 * Set client session for receiving TCP proxy packets
 */
void AgentConnectionEx::setTcpProxySession(ClientSession *session)
{
   m_tcpProxySession = session;
}

/**
 * Process TCP proxy message
 */
void AgentConnectionEx::processTcpProxyData(uint32_t channelId, const void *data, size_t size)
{
   if (m_tcpProxySession != nullptr)
      m_tcpProxySession->processTcpProxyData(this, channelId, data, size);
}

/**
 * Get SSH key by id
 */
void AgentConnectionEx::getSshKeys(NXCPMessage *msg, NXCPMessage *response)
{
   FindSshKeyById(msg->getFieldAsUInt32(VID_SSH_KEY_ID), response);
}

/**
 * Disconnect handler
 */
void AgentConnectionEx::onDisconnect()
{
   // Cancel file monitoring locally
   if (isFileUpdateConnection())
      g_monitoringList.removeDisconnectedNode(m_nodeId);
}
