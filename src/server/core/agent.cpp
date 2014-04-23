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
 * Destructor for extended agent connection class
 */
AgentConnectionEx::~AgentConnectionEx()
{
}

/**
 * Trap processor
 */
void AgentConnectionEx::onTrap(CSCPMessage *pMsg)
{
   UINT32 dwEventCode;
   int i, iNumArgs;
   Node *pNode = NULL;
   TCHAR *pszArgList[32], szBuffer[32];
   char szFormat[] = "ssssssssssssssssssssssssssssssss";

	DbgPrintf(3, _T("AgentConnectionEx::onTrap(): Received trap message from agent at %s, node ID %d"), IpToStr(getIpAddr(), szBuffer), m_nodeId);
	if (m_nodeId != 0)
		pNode = (Node *)FindObjectById(m_nodeId, OBJECT_NODE);
	if (pNode == NULL)
		pNode = FindNodeByIP(0, getIpAddr());
   if (pNode != NULL)
   {
		// Check for duplicate traps - only accept traps with ID
		// higher than last received
		// agents prior to 1.1.6 will not send trap id
		// we should accept trap in that case to maintain compatibility
		bool acceptTrap;
		QWORD trapId = pMsg->GetVariableInt64(VID_TRAP_ID);
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
			dwEventCode = pMsg->GetVariableLong(VID_EVENT_CODE);
			if ((dwEventCode == 0) && pMsg->isFieldExist(VID_EVENT_NAME))
			{
				TCHAR eventName[256];
				pMsg->GetVariableStr(VID_EVENT_NAME, eventName, 256);
				dwEventCode = EventCodeFromName(eventName, 0);
			}
			iNumArgs = (int)pMsg->GetVariableShort(VID_NUM_ARGS);
			if (iNumArgs > 32)
				iNumArgs = 32;
			for(i = 0; i < iNumArgs; i++)
				pszArgList[i] = pMsg->GetVariableStr(VID_EVENT_ARG_BASE + i);
			DbgPrintf(3, _T("Event from trap: %d"), dwEventCode);

			// Following call is not very good, but I'm too lazy now
			// to change PostEvent()
			szFormat[iNumArgs] = 0;
			PostEvent(dwEventCode, pNode->Id(), (iNumArgs > 0) ? szFormat : NULL,
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
      DbgPrintf(3, _T("Cannot find node for IP address %s"), IpToStr(getIpAddr(), szBuffer));
   }
}

/**
 * Handler for data push
 */
void AgentConnectionEx::onDataPush(CSCPMessage *msg)
{
	TCHAR name[MAX_PARAM_NAME], value[MAX_RESULT_LENGTH];

	msg->GetVariableStr(VID_NAME, name, MAX_PARAM_NAME);
	msg->GetVariableStr(VID_VALUE, value, MAX_RESULT_LENGTH);

   Node *sender = NULL;
	if (m_nodeId != 0)
		sender = (Node *)FindObjectById(m_nodeId, OBJECT_NODE);
   if (sender == NULL)
	   sender = FindNodeByIP(0, getIpAddr());

	if (sender != NULL)
	{
		// Check for duplicate data requests - only accept requests with ID
		// higher than last received
		// agents prior to 1.2.10 will not send request id
		// we should accept data in that case to maintain compatibility
		bool acceptRequest;
		QWORD requestId = msg->GetVariableInt64(VID_REQUEST_ID);
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
         UINT32 objectId = msg->GetVariableLong(VID_OBJECT_ID);
         if (objectId != 0)
         {
            // push on behalf of other node
            target = (Node *)FindObjectById(objectId, OBJECT_NODE);
            if (target != NULL)
            {
               if (target->isTrustedNode(sender->Id()))
               {
                  DbgPrintf(5, _T("%s: agent data push: target set to %s [%d]"), sender->Name(), target->Name(), target->Id());
               }
               else
               {
                  DbgPrintf(5, _T("%s: agent data push: not in trusted node list for target %s [%d]"), sender->Name(), target->Name(), target->Id());
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
		      DbgPrintf(5, _T("%s: agent data push: %s=%s"), target->Name(), name, value);
		      DCObject *dci = target->getDCObjectByName(name);
		      if ((dci != NULL) && (dci->getType() == DCO_TYPE_ITEM) && (dci->getDataSource() == DS_PUSH_AGENT) && (dci->getStatus() == ITEM_STATUS_ACTIVE))
		      {
			      DbgPrintf(5, _T("%s: agent data push: found DCI %d"), target->Name(), dci->getId());
			      time_t t = time(NULL);
			      target->processNewDCValue(dci, t, value);
			      dci->setLastPollTime(t);
		      }
		      else
		      {
			      DbgPrintf(5, _T("%s: agent data push: DCI not found for %s"), target->Name(), name);
		      }
         }
         else
         {
		      DbgPrintf(5, _T("%s: agent data push: target node not found or not accessible"), sender->Name());
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
void AgentConnectionEx::onFileMonitoringData(CSCPMessage *pMsg)
{
	NetObj *object = NULL;
	if (m_nodeId != 0)
		object = (Node *)FindObjectById(m_nodeId, OBJECT_NODE);
	if (object != NULL)
	{
	   TCHAR remoteFile[MAX_PATH];
      pMsg->GetVariableStr(VID_FILE_NAME, remoteFile, MAX_PATH);
      ObjectArray<ClientSession>* result = g_monitoringList.findClientByFNameAndNodeID(remoteFile, object->Id());
      DbgPrintf(6, _T("AgentConnectionEx::onFileMonitoringData: found %d sessions for remote file %s on node %s [%d]"), result->size(), remoteFile, object->Name(), object->Id());
      for(int i = 0; i < result->size(); i++)
      {
         result->get(i)->sendMessage(pMsg);
      }
      if(result->size() == 0)
      {
         DbgPrintf(6, _T("AgentConnectionEx::onFileMonitoringData: unknown subscription will be canceled."));
         Node *node = (Node *)object;
         AgentConnection *conn = node->createAgentConnection();
         if(conn != NULL)
         {
            CSCPMessage request;
            request.SetId(conn->generateRequestId());
            request.SetCode(CMD_CANCEL_FILE_MONITORING);
            request.SetVariable(VID_FILE_NAME, remoteFile);
            request.SetVariable(VID_OBJECT_ID, node->Id());
            CSCPMessage* response = conn->customRequest(&request);
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
 * Deploy policy to agent
 */
UINT32 AgentConnectionEx::deployPolicy(AgentPolicy *policy)
{
	UINT32 rqId, rcc;
	CSCPMessage msg(getProtocolVersion());

   rqId = generateRequestId();
   msg.SetId(rqId);
	msg.SetCode(CMD_DEPLOY_AGENT_POLICY);
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
	CSCPMessage msg(getProtocolVersion());

   rqId = generateRequestId();
   msg.SetId(rqId);
	msg.SetCode(CMD_UNINSTALL_AGENT_POLICY);
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
