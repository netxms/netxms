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
** File: agent.cpp
**
**/

#include "nxcore.h"

/**
 * Externals
 */
extern Queue g_nodePollerQueue;
void ProcessTrap(SNMP_PDU *pdu, const InetAddress& srcAddr, int srcPort, SNMP_Transport *pTransport, SNMP_Engine *localEngine, bool isInformRq);

/**
 * Destructor for extended agent connection class
 */
AgentConnectionEx::~AgentConnectionEx()
{
}

/**
 * Trap processor
 */
void AgentConnectionEx::onTrap(NXCPMessage *pMsg)
{
   UINT32 dwEventCode;
   int i, iNumArgs;
   Node *pNode = NULL;
   TCHAR *pszArgList[32], szBuffer[32];
   char szFormat[] = "ssssssssssssssssssssssssssssssss";

   DbgPrintf(3, _T("AgentConnectionEx::onTrap(): Received trap message from agent at %s, node ID %d"), getIpAddr().toString(szBuffer), m_nodeId);
	if (m_nodeId != 0)
		pNode = (Node *)FindObjectById(m_nodeId, OBJECT_NODE);
	if (pNode == NULL)
      pNode = FindNodeByIP(0, getIpAddr().getAddressV4());
   if (pNode != NULL)
   {
      if (pNode->Status() != STATUS_UNMANAGED)
      {
		   // Check for duplicate traps - only accept traps with ID
		   // higher than last received
		   // agents prior to 1.1.6 will not send trap id
		   // we should accept trap in that case to maintain compatibility
		   bool acceptTrap;
		   QWORD trapId = pMsg->getFieldAsUInt64(VID_TRAP_ID);
		   if (trapId != 0)
		   {
			   acceptTrap = pNode->checkAgentTrapId(trapId);
			   DbgPrintf(5, _T("AgentConnectionEx::onTrap(): trapID is%s valid"), acceptTrap ? _T("") : _T(" not"));
		   }
		   else
		   {
			   acceptTrap = true;
			   DbgPrintf(5, _T("AgentConnectionEx::onTrap(): trap ID not provided"));
		   }

		   if (acceptTrap)
		   {
			   dwEventCode = pMsg->getFieldAsUInt32(VID_EVENT_CODE);
			   if ((dwEventCode == 0) && pMsg->isFieldExist(VID_EVENT_NAME))
			   {
				   TCHAR eventName[256];
				   pMsg->getFieldAsString(VID_EVENT_NAME, eventName, 256);
				   dwEventCode = EventCodeFromName(eventName, 0);
			   }
			   iNumArgs = (int)pMsg->getFieldAsUInt16(VID_NUM_ARGS);
			   if (iNumArgs > 32)
				   iNumArgs = 32;
			   for(i = 0; i < iNumArgs; i++)
				   pszArgList[i] = pMsg->getFieldAsString(VID_EVENT_ARG_BASE + i);
			   DbgPrintf(3, _T("Event from trap: %d"), dwEventCode);

			   szFormat[iNumArgs] = 0;
			   PostEvent(dwEventCode, pNode->getId(), (iNumArgs > 0) ? szFormat : NULL,
						    pszArgList[0], pszArgList[1], pszArgList[2], pszArgList[3],
						    pszArgList[4], pszArgList[5], pszArgList[6], pszArgList[7],
						    pszArgList[8], pszArgList[9], pszArgList[10], pszArgList[11],
						    pszArgList[12], pszArgList[13], pszArgList[14], pszArgList[15],
						    pszArgList[16], pszArgList[17], pszArgList[18], pszArgList[19],
						    pszArgList[20], pszArgList[21], pszArgList[22], pszArgList[23],
						    pszArgList[24], pszArgList[25], pszArgList[26], pszArgList[27],
						    pszArgList[28], pszArgList[29], pszArgList[30], pszArgList[31]);

			   // Cleanup
			   for(i = 0; i < iNumArgs; i++)
				   free(pszArgList[i]);
		   }
      }
      else
      {
         DbgPrintf(3, _T("AgentConnectionEx::onTrap(): node %s [%d] in in UNMANAGED state - trap ignored"), pNode->getName(), pNode->getId());
      }
   }
   else
   {
      DbgPrintf(3, _T("AgentConnectionEx::onTrap(): Cannot find node for IP address %s"), getIpAddr().toString(szBuffer));
   }
}

/**
 * Handler for data push
 */
void AgentConnectionEx::onDataPush(NXCPMessage *msg)
{
	TCHAR name[MAX_PARAM_NAME], value[MAX_RESULT_LENGTH];

	msg->getFieldAsString(VID_NAME, name, MAX_PARAM_NAME);
	msg->getFieldAsString(VID_VALUE, value, MAX_RESULT_LENGTH);

   Node *sender = NULL;
	if (m_nodeId != 0)
		sender = (Node *)FindObjectById(m_nodeId, OBJECT_NODE);
   if (sender == NULL)
      sender = FindNodeByIP(0, getIpAddr().getAddressV4());

	if (sender != NULL)
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
			DbgPrintf(5, _T("AgentConnectionEx::onDataPush(): requestId is%s valid"), acceptRequest ? _T("") : _T(" not"));
		}
		else
		{
			acceptRequest = true;
			DbgPrintf(5, _T("AgentConnectionEx::onDataPush(): request ID not provided"));
		}

		if (acceptRequest)
		{
         Node *target;
         UINT32 objectId = msg->getFieldAsUInt32(VID_OBJECT_ID);
         if (objectId != 0)
         {
            // push on behalf of other node
            target = (Node *)FindObjectById(objectId, OBJECT_NODE);
            if (target != NULL)
            {
               if (target->isTrustedNode(sender->getId()))
               {
                  DbgPrintf(5, _T("%s: agent data push: target set to %s [%d]"), sender->getName(), target->getName(), target->getId());
               }
               else
               {
                  DbgPrintf(5, _T("%s: agent data push: not in trusted node list for target %s [%d]"), sender->getName(), target->getName(), target->getId());
                  target = NULL;
               }
            }
         }
         else
         {
            target = sender;
         }

         if (target != NULL)
         {
		      DbgPrintf(5, _T("%s: agent data push: %s=%s"), target->getName(), name, value);
		      DCObject *dci = target->getDCObjectByName(name);
		      if ((dci != NULL) && (dci->getType() == DCO_TYPE_ITEM) && (dci->getDataSource() == DS_PUSH_AGENT) && (dci->getStatus() == ITEM_STATUS_ACTIVE))
		      {
			      DbgPrintf(5, _T("%s: agent data push: found DCI %d"), target->getName(), dci->getId());
               time_t t = msg->getFieldAsTime(VID_TIMESTAMP);
               if (t == 0)
			         t = time(NULL);
			      target->processNewDCValue(dci, t, value);
               if (t > dci->getLastPollTime())
			         dci->setLastPollTime(t);
		      }
		      else
		      {
			      DbgPrintf(5, _T("%s: agent data push: DCI not found for %s"), target->getName(), name);
		      }
         }
         else
         {
		      DbgPrintf(5, _T("%s: agent data push: target node not found or not accessible"), sender->getName());
         }
      }
	}
}

/**
 * Print message.
 */
void AgentConnectionEx::printMsg(const TCHAR *format, ...)
{
   va_list args;

   va_start(args, format);
   DbgPrintf2(6, format, args);
   va_end(args);
}

/**
 * Recieve file monitoring information and resend to all required user sessions
 */
void AgentConnectionEx::onFileMonitoringData(NXCPMessage *pMsg)
{
	Node *object = NULL;
	if (m_nodeId != 0)
		object = (Node *)FindObjectById(m_nodeId, OBJECT_NODE);
	if (object != NULL)
	{
	   TCHAR remoteFile[MAX_PATH];
      pMsg->getFieldAsString(VID_FILE_NAME, remoteFile, MAX_PATH);
      ObjectArray<ClientSession>* result = g_monitoringList.findClientByFNameAndNodeID(remoteFile, object->getId());
      DbgPrintf(6, _T("AgentConnectionEx::onFileMonitoringData: found %d sessions for remote file %s on node %s [%d]"), result->size(), remoteFile, object->getName(), object->getId());
      for(int i = 0; i < result->size(); i++)
      {
         result->get(i)->sendMessage(pMsg);
      }
      if(result->size() == 0)
      {
         DbgPrintf(6, _T("AgentConnectionEx::onFileMonitoringData: unknown subscription will be canceled."));
         AgentConnection *conn = object->createAgentConnection();
         if(conn != NULL)
         {
            NXCPMessage request;
            request.setId(conn->generateRequestId());
            request.setCode(CMD_CANCEL_FILE_MONITORING);
            request.setField(VID_FILE_NAME, remoteFile);
            request.setField(VID_OBJECT_ID, object->getId());
            NXCPMessage* response = conn->customRequest(&request);
            delete response;
         }
         delete conn;
      }
      delete result;
	}
	else
	{
		g_monitoringList.removeDisconectedNode(m_nodeId);
      DbgPrintf(6, _T("AgentConnectionEx::onFileMonitoringData: node object not found"));
	}
}

/**
 * Ask modules if they can procress custom message
 */
bool AgentConnectionEx::processCustomMessage(NXCPMessage *msg)
{
    for(UINT32 i = 0; i < g_dwNumModules; i++)
    {
        if (g_pModuleList[i].pfOnAgentMessage != NULL)
        {
            if (g_pModuleList[i].pfOnAgentMessage(msg, m_nodeId))
                return true;    // accepted by module
        }
    }

   return false;
}

/**
 * Recieve trap sent throught proxy agent
 */
void AgentConnectionEx::onSnmpTrap(NXCPMessage *msg)
{
   Node *proxyNode = NULL;
   TCHAR ipStringBuffer[4096];

   static BYTE engineId[] = { 0x80, 0x00, 0x00, 0x00, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01, 0x00 };
	SNMP_Engine localEngine(engineId, 12);

   DbgPrintf(3, _T("AgentConnectionEx::onSnmpTrap(): Received SNMP trap message from agent at %s, node ID %d"), getIpAddr().toString(ipStringBuffer), m_nodeId);
	if (m_nodeId != 0)
		proxyNode = (Node *)FindObjectById(m_nodeId, OBJECT_NODE);
   if (proxyNode != NULL)
   {
      // Check for duplicate traps - only accept traps with ID
      // higher than last received
      bool acceptTrap;
      UINT32 trapId = msg->getId();
      if (trapId != 0)
      {
         acceptTrap = proxyNode->checkSNMPTrapId(trapId);
         DbgPrintf(5, _T("AgentConnectionEx::onSnmpTrap(): SNMP trapID is%s valid"), acceptTrap ? _T("") : _T(" not"));
      }
      else
      {
         acceptTrap = false;
         DbgPrintf(5, _T("AgentConnectionEx::onSnmpTrap(): SNMP trap ID not provided"));
      }

      if (acceptTrap)
      {
         InetAddress originSenderIP = msg->getFieldAsInetAddress(VID_IP_ADDRESS);
         UINT32 pduLenght = msg->getFieldAsUInt32(VID_PDU_SIZE);
         BYTE *pduBytes = (BYTE*)malloc(pduLenght);
         msg->getFieldAsBinary(VID_PDU, pduBytes, pduLenght);
         Node *originNode = FindNodeByIP(0, originSenderIP); //create function

         SNMP_ProxyTransport *pTransport;
         if(originNode != NULL)
            pTransport = (SNMP_ProxyTransport*)originNode->createSnmpTransport((WORD)msg->getFieldAsUInt16(VID_PORT));

         if(ConfigReadInt(_T("LogAllSNMPTraps"), FALSE) && originNode == NULL)
         {
            AgentConnection *pConn;

            pConn = proxyNode->createAgentConnection();
            if (pConn != NULL)
            {
               pTransport = new SNMP_ProxyTransport(pConn, originSenderIP, msg->getFieldAsUInt16(VID_PORT));
            }
         }

         if(pTransport != NULL)
         {
            pTransport->setWaitForResponse(false);
            SNMP_PDU *pdu = new SNMP_PDU;
            if(pdu->parse(pduBytes, pduLenght, (originNode != NULL) ? originNode->getSnmpSecurityContext() : NULL, true))
            {
               DbgPrintf(6, _T("SNMPTrapReceiver: received PDU of type %d"), pdu->getCommand());
               if ((pdu->getCommand() == SNMP_TRAP) || (pdu->getCommand() == SNMP_INFORM_REQUEST))
               {
                  if ((pdu->getVersion() == SNMP_VERSION_3) && (pdu->getCommand() == SNMP_INFORM_REQUEST))
                  {
                     SNMP_SecurityContext *context = pTransport->getSecurityContext();
                     context->setAuthoritativeEngine(localEngine);
                  }
                  ProcessTrap(pdu, originSenderIP, msg->getFieldAsUInt16(VID_PORT), pTransport, &localEngine, pdu->getCommand() == SNMP_INFORM_REQUEST);
               }
               else if ((pdu->getVersion() == SNMP_VERSION_3) && (pdu->getCommand() == SNMP_GET_REQUEST) && (pdu->getAuthoritativeEngine().getIdLen() == 0))
               {
                  // Engine ID discovery
                  DbgPrintf(6, _T("SNMPTrapReceiver: EngineId discovery"));

                  SNMP_PDU *response = new SNMP_PDU(SNMP_REPORT, pdu->getRequestId(), pdu->getVersion());
                  response->setReportable(false);
                  response->setMessageId(pdu->getMessageId());
                  response->setContextEngineId(localEngine.getId(), localEngine.getIdLen());

                  SNMP_Variable *var = new SNMP_Variable(_T(".1.3.6.1.6.3.15.1.1.4.0"));
                  var->setValueFromString(ASN_INTEGER, _T("2"));
                  response->bindVariable(var);

                  SNMP_SecurityContext *context = new SNMP_SecurityContext();
                  localEngine.setTime((int)time(NULL));
                  context->setAuthoritativeEngine(localEngine);
                  context->setSecurityModel(SNMP_SECURITY_MODEL_USM);
                  context->setAuthMethod(SNMP_AUTH_NONE);
                  context->setPrivMethod(SNMP_ENCRYPT_NONE);
                  pTransport->setSecurityContext(context);

                  pTransport->sendMessage(response);
                  delete response;
               }
               else if (pdu->getCommand() == SNMP_REPORT)
               {
                  DbgPrintf(6, _T("AgentConnectionEx::onSnmpTrap(): REPORT PDU with error %s"), pdu->getVariable(0)->getName()->getValueAsText());
               }
               delete pdu;
            }
            else if (pdu->getCommand() == SNMP_REPORT)
            {
               DbgPrintf(6, _T("AgentConnectionEx::onSnmpTrap(): REPORT PDU with error %s"), pdu->getVariable(0)->getName()->getValueAsText());
            }
         }
         else
         {
            DbgPrintf(3, _T("AgentConnectionEx::onSnmpTrap(): cannot find origin node with IP %s and not accepting traps from unknown sources"), originSenderIP.toString(ipStringBuffer));
         }
      }
   }
   else
   {
      DbgPrintf(3, _T("AgentConnectionEx::onSnmpTrap(): Cannot find node for IP address %s"), getIpAddr().toString(ipStringBuffer));
   }
}

/**
 * Deploy policy to agent
 */
UINT32 AgentConnectionEx::deployPolicy(AgentPolicy *policy)
{
	UINT32 rqId, rcc;
	NXCPMessage msg(getProtocolVersion());

   rqId = generateRequestId();
   msg.setId(rqId);
	msg.setCode(CMD_DEPLOY_AGENT_POLICY);
	if (policy->createDeploymentMessage(&msg))
	{
		if (sendMessage(&msg))
		{
			rcc = waitForRCC(rqId, getCommandTimeout());
		}
		else
		{
			rcc = ERR_CONNECTION_BROKEN;
		}
	}
	else
	{
		rcc = ERR_INTERNAL_ERROR;
	}
   return rcc;
}

/**
 * Uninstall policy from agent
 */
UINT32 AgentConnectionEx::uninstallPolicy(AgentPolicy *policy)
{
	UINT32 rqId, rcc;
	NXCPMessage msg(getProtocolVersion());

   rqId = generateRequestId();
   msg.setId(rqId);
	msg.setCode(CMD_UNINSTALL_AGENT_POLICY);
	if (policy->createUninstallMessage(&msg))
	{
		if (sendMessage(&msg))
		{
			rcc = waitForRCC(rqId, getCommandTimeout());
		}
		else
		{
			rcc = ERR_CONNECTION_BROKEN;
		}
	}
	else
	{
		rcc = ERR_INTERNAL_ERROR;
	}
   return rcc;
}
