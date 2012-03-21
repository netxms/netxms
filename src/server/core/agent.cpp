/* 
** NetXMS - Network Management System
** Copyright (C) 2006 Victor Kirhenshtein
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


//
// Destructor for extended agent connection class
//

AgentConnectionEx::~AgentConnectionEx()
{
}


//
// Trap processor
//

void AgentConnectionEx::onTrap(CSCPMessage *pMsg)
{
   DWORD dwEventCode;
   int i, iNumArgs;
   Node *pNode;
   TCHAR *pszArgList[32], szBuffer[32];
   char szFormat[] = "ssssssssssssssssssssssssssssssss";

	DbgPrintf(3, _T("AgentConnectionEx::onTrap(): Received trap message from agent at %s"), IpToStr(getIpAddr(), szBuffer));
   pNode = FindNodeByIP(0, getIpAddr());	/* FIXME: is it possible to receive traps from other zones? */
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


//
// Handler for data push
//

void AgentConnectionEx::onDataPush(CSCPMessage *msg)
{
	TCHAR name[MAX_PARAM_NAME], value[MAX_RESULT_LENGTH];

	msg->GetVariableStr(VID_NAME, name, MAX_PARAM_NAME);
	msg->GetVariableStr(VID_VALUE, value, MAX_RESULT_LENGTH);

	Node *node = FindNodeByIP(0, getIpAddr());	/* FIXME: is it possible to receive push data form other zones? */
	if (node != NULL)
	{
		DbgPrintf(5, _T("%s: agent data push: %s=%s"), node->Name(), name, value);
		DCObject *dci = node->getDCObjectByName(name);
		if ((dci != NULL) && (dci->getType() == DCO_TYPE_ITEM) && (dci->getDataSource() == DS_PUSH_AGENT) && (dci->getStatus() == ITEM_STATUS_ACTIVE))
		{
			DbgPrintf(5, _T("%s: agent data push: found DCI %d"), node->Name(), dci->getId());
			time_t t = time(NULL);
			node->processNewDciValue((DCItem *)dci, t, value);
			dci->setLastPollTime(t);
		}
		else
		{
			DbgPrintf(5, _T("%s: agent data push: DCI not found for %s"), node->Name(), name);
		}
	}
}


//
// Deploy policy to agent
//

DWORD AgentConnectionEx::deployPolicy(AgentPolicy *policy)
{
	DWORD rqId, rcc;
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


//
// Uninstall policy from agent
//

DWORD AgentConnectionEx::uninstallPolicy(AgentPolicy *policy)
{
	DWORD rqId, rcc;
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
