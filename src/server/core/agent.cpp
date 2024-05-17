/*
** NetXMS - Network Management System
** Copyright (C) 2003-2024 Victor Kirhenshtein
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
void EnqueueSNMPTrap(SNMP_PDU *pdu, const InetAddress& srcAddr, int32_t zoneUIN, int srcPort, SNMP_Transport *snmpTransport, SNMP_Engine *localEngine);
void QueueProxiedSyslogMessage(const InetAddress &addr, int32_t zoneUIN, uint32_t nodeId, time_t timestamp, const char *msg, int msgLen);
void QueueWindowsEvent(WindowsEvent *event);

/**
 * Create normal agent connection
 */
AgentConnectionEx::AgentConnectionEx(uint32_t nodeId, const InetAddress& ipAddr, uint16_t port, const TCHAR *secret, bool allowCompression) :
         AgentConnection(ipAddr, port, secret, allowCompression)
{
   m_nodeId = nodeId;
   m_tcpProxySession = nullptr;

   // Set DB writer queue threshold to 3/4 of max db writer queue size, or 250000 if queue size is not limited
   m_dbWriterQueueThreshold = ConfigReadInt64(_T("DBWriter.MaxQueueSize"), 0) * 3 / 4;
   if (m_dbWriterQueueThreshold <= 0)
      m_dbWriterQueueThreshold = 250000;
}

/**
 * Create agent connection within tunnel
 */
AgentConnectionEx::AgentConnectionEx(uint32_t nodeId, const shared_ptr<AgentTunnel>& tunnel, const TCHAR *secret, bool allowCompression) :
         AgentConnection(InetAddress::INVALID, 0, secret, allowCompression), m_tunnel(tunnel)
{
   m_nodeId = nodeId;
   m_tcpProxySession = nullptr;

   // Set DB writer queue threshold to 3/4 of max db writer queue size, or 250000 if queue size is not limited
   m_dbWriterQueueThreshold = ConfigReadInt64(_T("DBWriter.MaxQueueSize"), 0) * 3 / 4;
   if (m_dbWriterQueueThreshold <= 0)
      m_dbWriterQueueThreshold = 250000;
}

/**
 * Destructor for extended agent connection class
 */
AgentConnectionEx::~AgentConnectionEx()
{
}

/**
 * Create communication channel
 */
shared_ptr<AbstractCommChannel> AgentConnectionEx::createChannel()
{
   if (m_tunnel != nullptr)
      return m_tunnel->createChannel();
   if (isProxyMode() && (m_proxyTunnel != nullptr))
      return m_proxyTunnel->createChannel();
   return AgentConnection::createChannel();
}

/**
 * Set proxy tunnel to use
 */
void AgentConnectionEx::setProxy(const shared_ptr<AgentTunnel>& tunnel, const TCHAR *secret)
{
   m_proxyTunnel = tunnel;
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
		   uint64_t trapId = pMsg->getFieldAsUInt64(VID_TRAP_ID);
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
			   uint32_t eventCode = pMsg->getFieldAsUInt32(VID_EVENT_CODE);
			   if ((eventCode == 0) && pMsg->isFieldExist(VID_EVENT_NAME))
			   {
				   TCHAR eventName[256];
				   pMsg->getFieldAsString(VID_EVENT_NAME, eventName, 256);
				   eventCode = EventCodeFromName(eventName, 0);
               debugPrintf(4, _T("Received event name \"%s\" (resolved to code %u)"), eventName, eventCode);
			   }
			   else
			   {
			      debugPrintf(4, _T("Received event code %u"), eventCode);
			   }

			   EventBuilder eventBuilder(eventCode, node->getId());
			   eventBuilder.origin(EventOrigin::AGENT).originTimestamp(pMsg->getFieldAsTime(VID_TIMESTAMP));
			   if (pMsg->isFieldExist(VID_EVENT_ARG_NAMES_BASE))
			      eventBuilder.params(*pMsg, VID_EVENT_ARG_BASE, VID_EVENT_ARG_NAMES_BASE, VID_NUM_ARGS);
			   else
               eventBuilder.params(*pMsg, VID_EVENT_ARG_BASE, VID_NUM_ARGS);
            eventBuilder.post();
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
         shared_ptr<DataCollectionTarget> target;
         uint32_t objectId = msg->getFieldAsUInt32(VID_OBJECT_ID);
         if (objectId != 0)
         {
            // push on behalf of other node
            shared_ptr<NetObj> object = FindObjectById(objectId);
            if ((object != nullptr) && object->isDataCollectionTarget())
            {
               if (object->isTrustedObject(sender->getId()))
               {
                  target = static_pointer_cast<DataCollectionTarget>(object);
                  debugPrintf(5, _T("%s: agent data push: target set to %s [%d]"), sender->getName(), target->getName(), target->getId());
               }
               else
               {
                  debugPrintf(5, _T("%s: agent data push: not in trusted object list for target %s [%d]"), sender->getName(), target->getName(), target->getId());
               }
            }
         }
         else
         {
            target = sender;
         }

         if (target != nullptr)
         {
            debugPrintf(5, _T("%s: agent data push: %s=%s"), target->getName(), name, value);
		      shared_ptr<DCObject> dci = target->getDCObjectByName(name, 0);
		      if ((dci != nullptr) && (dci->getType() == DCO_TYPE_ITEM) && (dci->getDataSource() == DS_PUSH_AGENT) && (dci->getStatus() == ITEM_STATUS_ACTIVE))
		      {
		         debugPrintf(5, _T("%s: agent data push: found DCI %d"), target->getName(), dci->getId());
               time_t t = msg->getFieldAsTime(VID_TIMESTAMP);
               if (t == 0)
			         t = time(nullptr);
			      target->processNewDCValue(dci, t, value, shared_ptr<Table>());
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
 * Receive file monitoring information and resend to all required user sessions
 */
void AgentConnectionEx::onFileMonitoringData(NXCPMessage *pMsg)
{
   if (IsShutdownInProgress())
      return;

	shared_ptr<Node> node;
	if (m_nodeId != 0)
		node = static_pointer_cast<Node>(FindObjectById(m_nodeId, OBJECT_NODE));
	if (node != nullptr)
	{
	   TCHAR agentFileId[MAX_PATH];
      pMsg->getFieldAsString(VID_FILE_NAME, agentFileId, MAX_PATH);
      unique_ptr<StructArray<std::pair<ClientSession*, uuid>>> sessions = FindFileMonitoringSessions(agentFileId);
      debugPrintf(6, _T("AgentConnectionEx::onFileMonitoringData: found %d sessions for file monitors with agent ID %s on node %s [%u]"),
            sessions->size(), agentFileId, node->getName(), node->getId());
      for(int i = 0; i < sessions->size(); i++)
      {
         std::pair<ClientSession*, uuid> *s = sessions->get(i);
         pMsg->setField(VID_MONITOR_ID, s->second);
         if (!s->first->sendMessage(pMsg))
         {
            RemoveFileMonitorByClientId(s->second);
         }
      }
	}
	else
	{
	   RemoveFileMonitorsByNodeId(m_nodeId);
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
   debugPrintf(6, _T("AgentConnectionEx::processCustomMessage: processing message %s ID %u"), NXCPMessageCodeName(msg->getCode(), buffer), msg->getId());

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

         SNMP_PDU *pdu = new SNMP_PDU;
         SNMP_SecurityContext *sctx = (originNode != nullptr) ? originNode->getSnmpSecurityContext() : nullptr;
         if (pdu->parse(pduBytes, pduLenght, sctx, true))
         {
            debugPrintf(6, _T("AgentConnectionEx::onSnmpTrap(): received PDU of type %d"), pdu->getCommand());
            if ((pdu->getCommand() == SNMP_TRAP) || (pdu->getCommand() == SNMP_INFORM_REQUEST))
            {
               bool isInformRequest = (pdu->getCommand() == SNMP_INFORM_REQUEST);
               SNMP_ProxyTransport *snmpTransport = isInformRequest ? CreateSNMPProxyTransport(self(), originNode.get(), originSenderIP, msg->getFieldAsUInt16(VID_PORT)) : nullptr;
               if ((pdu->getVersion() == SNMP_VERSION_3) && (pdu->getCommand() == SNMP_INFORM_REQUEST))
               {
                  SNMP_SecurityContext *context = snmpTransport->getSecurityContext();
                  context->setAuthoritativeEngine(localEngine);
               }
               if (snmpTransport != nullptr)
                  snmpTransport->setWaitForResponse(false);
               EnqueueSNMPTrap(pdu, originSenderIP, zoneUIN, msg->getFieldAsUInt16(VID_PORT), snmpTransport, &localEngine);
               pdu = nullptr; // prevent delete (PDU will be deleted by trap processor)
               delete snmpTransport;
            }
            else if ((pdu->getVersion() == SNMP_VERSION_3) && (pdu->getCommand() == SNMP_GET_REQUEST) && (pdu->getAuthoritativeEngine().getIdLen() == 0))
            {
               // Engine ID discovery
               debugPrintf(6, _T("AgentConnectionEx::onSnmpTrap(): EngineId discovery"));

               SNMP_ProxyTransport *snmpTransport = CreateSNMPProxyTransport(self(), originNode.get(), originSenderIP, msg->getFieldAsUInt16(VID_PORT));

               SNMP_PDU response(SNMP_REPORT, pdu->getRequestId(), pdu->getVersion());
               response.setReportable(false);
               response.setMessageId(pdu->getMessageId());
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
            else if (pdu->getCommand() == SNMP_REPORT)
            {
               debugPrintf(6, _T("AgentConnectionEx::onSnmpTrap(): REPORT PDU with error %s"), pdu->getVariable(0)->getName().toString().cstr());
            }
         }
         else
         {
            debugPrintf(6, _T("AgentConnectionEx::onSnmpTrap(): error parsing PDU"));
         }
         delete pdu;
         delete sctx;
      }
   }
   else
   {
      debugPrintf(3, _T("AgentConnectionEx::onSnmpTrap(): Cannot find node for IP address %s"), getIpAddr().toString(ipStringBuffer));
   }
}

/**
 * Handle agent notification message
 */
void AgentConnectionEx::onNotify(NXCPMessage *msg)
{
   TCHAR notificationCode[32];
   msg->getFieldAsString(VID_NOTIFICATION_CODE, notificationCode, 32);

   if (!_tcscmp(notificationCode, _T("AgentRestart")))
   {
      // Set restart grace timestamp
      shared_ptr<Node> node;
      if (m_nodeId != 0)
         node = static_pointer_cast<Node>(FindObjectById(m_nodeId, OBJECT_NODE));
      if (node == nullptr)
         node = FindNodeByIP(0, getIpAddr());
      if (node != nullptr)
         node->setAgentRestartTime();
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
 * Make web service custom request
 */
WebServiceCallResult *AgentConnectionEx::webServiceCustomRequest(HttpRequestMethod requestMethod, const TCHAR *url, uint32_t requestTimeout, const TCHAR *login, const TCHAR *password,
         const WebServiceAuthType authType, const StringMap& headers, bool verifyCert, bool verifyHost, bool followLocation, uint32_t cacheRetentionTime, const TCHAR *data)
{
   WebServiceCallResult *result = new WebServiceCallResult();
   NXCPMessage msg(getProtocolVersion());
   uint32_t requestId = generateRequestId();
   msg.setCode(CMD_WEB_SERVICE_CUSTOM_REQUEST);
   msg.setId(requestId);
   msg.setField(VID_URL, url);
   msg.setField(VID_HTTP_REQUEST_METHOD, static_cast<uint16_t>(requestMethod));
   msg.setField(VID_TIMEOUT, requestTimeout);
   msg.setField(VID_LOGIN_NAME, login);
   msg.setField(VID_PASSWORD, password);
   msg.setField(VID_AUTH_TYPE, static_cast<uint16_t>(authType));
   msg.setField(VID_VERIFY_CERT, verifyCert);
   msg.setField(VID_VERIFY_HOST, verifyHost);
   msg.setField(VID_FOLLOW_LOCATION, followLocation);
   msg.setField(VID_RETENTION_TIME, cacheRetentionTime);
   headers.fillMessage(&msg, VID_HEADERS_BASE, VID_NUM_HEADERS);
   msg.setField(VID_REQUEST_DATA, data);

   uint32_t rcc;
   if (sendMessage(&msg))
   {
      NXCPMessage *response = waitForMessage(CMD_REQUEST_COMPLETED, requestId, getCommandTimeout());
      if (response != nullptr)
      {
         rcc = response->getFieldAsUInt32(VID_RCC);
         if (rcc == ERR_SUCCESS)
         {
            result->httpResponseCode = response->getFieldAsUInt32(VID_WEBSVC_RESPONSE_CODE);
            result->document = response->getFieldAsString(VID_WEBSVC_RESPONSE);
            result->success = true;
         }
         response->getFieldAsString(VID_WEBSVC_ERROR_TEXT, result->errorMessage, WEBSVC_ERROR_TEXT_MAX_SIZE);
         delete response;
      }
      else
      {
         rcc = ERR_REQUEST_TIMEOUT;
         _tcslcpy(result->errorMessage, _T("Agent request timeout"), WEBSVC_ERROR_TEXT_MAX_SIZE);
      }
   }
   else
   {
      rcc = ERR_CONNECTION_BROKEN;
      _tcslcpy(result->errorMessage, _T("Agent connection broken"), WEBSVC_ERROR_TEXT_MAX_SIZE);
   }
   result->agentErrorCode = rcc;
   return result;
}

/**
 * Process collected data information (for DCI with agent-side cache)
 */
uint32_t AgentConnectionEx::processCollectedData(NXCPMessage *msg)
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
   if ((origin != DS_NATIVE_AGENT) && (origin != DS_SNMP_AGENT) && (origin != DS_MODBUS))
   {
      debugPrintf(5, _T("AgentConnectionEx::processCollectedData: unsupported data source type %d"), origin);
      return ERR_INTERNAL_ERROR;
   }

   // Check that server is not overloaded with DCI data
   int64_t queueSize = GetIDataWriterQueueSize();
   if (queueSize >= m_dbWriterQueueThreshold)
   {
      debugPrintf(5, _T("AgentConnectionEx::processCollectedData: database writer queue is too large (") INT64_FMT _T(") - cannot accept new data"), queueSize);
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

   uint32_t dciId = msg->getFieldAsUInt32(VID_DCI_ID);
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
   uint32_t status = msg->getFieldAsUInt32(VID_STATUS);
   bool success = true;

   debugPrintf(7, _T("AgentConnectionEx::processCollectedData: processing DCI %s [%d] (type=%d) (status=%d) on object %s [%d]"),
               dcObject->getName().cstr(), dciId, type, status, target->getName(), target->getId());

   switch(status)
   {
      case ERR_SUCCESS:
      {
         TCHAR itemValue[MAX_RESULT_LENGTH];
         shared_ptr<Table> tableValue;
         switch(type)
         {
            case DCO_TYPE_ITEM:
               msg->getFieldAsString(VID_VALUE, itemValue, MAX_RESULT_LENGTH);
               break;
            case DCO_TYPE_TABLE:
               tableValue = make_shared<Table>(*msg);
               static_cast<DCTable*>(dcObject.get())->updateResultColumns(tableValue);
               break;
            default:
               debugPrintf(5, _T("AgentConnectionEx::processCollectedData: invalid type %d of DCI %s [%d] on object %s [%d]"),
                           type, dcObject->getName().cstr(), dciId, target->getName(), target->getId());
               return ERR_INTERNAL_ERROR;
         }

         if (dcObject->getStatus() == ITEM_STATUS_NOT_SUPPORTED)
            dcObject->setStatus(ITEM_STATUS_ACTIVE, true);
         success = target->processNewDCValue(dcObject, t, itemValue, tableValue);
         if (t > dcObject->getLastPollTime())
            dcObject->setLastPollTime(t);
         break;
      }
      case ERR_UNKNOWN_METRIC:
      case ERR_UNSUPPORTED_METRIC:
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
uint32_t AgentConnectionEx::processBulkCollectedData(NXCPMessage *request, NXCPMessage *response)
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
      debugPrintf(5, _T("AgentConnectionEx::processBulkCollectedData: cannot find node object (node ID = %u)"), m_nodeId);
      return ERR_INTERNAL_ERROR;
   }

   // Check that server is not overloaded with DCI data
   int64_t queueSize = GetIDataWriterQueueSize();
   if (queueSize >= m_dbWriterQueueThreshold)
   {
      debugPrintf(5, _T("AgentConnectionEx::processBulkCollectedData: database writer queue is too large (") INT64_FMT _T(") - cannot accept new data"), queueSize);
      return ERR_RESOURCE_BUSY;
   }

   int count = request->getFieldAsInt16(VID_NUM_ELEMENTS);
   if (count > MAX_BULK_DATA_BLOCK_SIZE)
      count = MAX_BULK_DATA_BLOCK_SIZE;
   debugPrintf(5, _T("AgentConnectionEx::processBulkCollectedData: %d elements from node %s [%u]"), count, node->getName(), node->getId());

   // Use half timeout for sending progress updates
   uint32_t agentTimeout = request->getFieldAsUInt32(VID_TIMEOUT) / 2;

   shared_ptr<Table> tableValue;
   BYTE status[MAX_BULK_DATA_BLOCK_SIZE];
   memset(status, 0, MAX_BULK_DATA_BLOCK_SIZE);
   uint32_t fieldId = VID_ELEMENT_LIST_BASE;
   int64_t startTime = GetCurrentTimeMs();
   for(int i = 0; i < count; i++, fieldId += 10)
   {
      uint32_t elapsed = static_cast<UINT32>(GetCurrentTimeMs() - startTime);
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
      if ((origin != DS_NATIVE_AGENT) && (origin != DS_SNMP_AGENT) && (origin != DS_MODBUS))
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
         debugPrintf(5, _T("AgentConnectionEx::processBulkCollectedData: cannot find DCI with ID %u on object %s [%u] (element %d)"),
                     dciId, target->getName(), target->getId(), i);
         status[i] = BULK_DATA_REC_FAILURE;
         continue;
      }

      int type = request->getFieldAsInt16(fieldId + 2);
      if ((type != DCO_TYPE_ITEM) || (dcObject->getType() != type) || (dcObject->getDataSource() != origin) || (dcObject->getAgentCacheMode() != AGENT_CACHE_ON))
      {
         debugPrintf(5, _T("AgentConnectionEx::processBulkCollectedData: DCI %s [%u] on object %s [%u] configuration mismatch (element %d)"),
                     dcObject->getName().cstr(), dciId, target->getName(), target->getId(), i);
         status[i] = BULK_DATA_REC_FAILURE;
         continue;
      }

      TCHAR *value = request->getFieldAsString(fieldId + 5);
      uint32_t statusCode = request->getFieldAsUInt32(fieldId + 6);
      debugPrintf(7, _T("AgentConnectionEx::processBulkCollectedData: processing DCI %s [%u] (type=%d) (status=%d) on object %s [%u] (element %d)"),
                  dcObject->getName().cstr(), dciId, type, statusCode, target->getName(), target->getId(), i);
      time_t t = request->getFieldAsTime(fieldId + 4);
      bool success = true;

      switch(statusCode)
      {
         case ERR_SUCCESS:
            if (dcObject->getStatus() == ITEM_STATUS_NOT_SUPPORTED)
               dcObject->setStatus(ITEM_STATUS_ACTIVE, true);
            success = target->processNewDCValue(dcObject, t, value, tableValue);
            if (t > dcObject->getLastPollTime())
               dcObject->setLastPollTime(t);
            break;
         case ERR_UNKNOWN_METRIC:
         case ERR_UNSUPPORTED_METRIC:
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
void AgentConnectionEx::processTcpProxyData(uint32_t channelId, const void *data, size_t size, bool errorIndicator)
{
   if (m_tcpProxySession != nullptr)
      m_tcpProxySession->processTcpProxyData(this, channelId, data, size, errorIndicator);
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
      RemoveFileMonitorsByNodeId(m_nodeId);

   if (m_tcpProxySession != nullptr)
      m_tcpProxySession->processTcpProxyAgentDisconnect(this);
}
