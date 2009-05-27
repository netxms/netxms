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

void AgentConnectionEx::OnTrap(CSCPMessage *pMsg)
{
   DWORD dwEventCode;
   int i, iNumArgs;
   Node *pNode;
   TCHAR *pszArgList[32], szBuffer[32];
   TCHAR szFormat[] = "ssssssssssssssssssssssssssssssss";

   DbgPrintf(3, _T("Received trap message from agent at %s"), IpToStr(GetIpAddr(), szBuffer));
   pNode = FindNodeByIP(GetIpAddr());
   if (pNode != NULL)
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
   else
   {
      DbgPrintf(3, _T("Cannot find node for IP address %s"), IpToStr(GetIpAddr(), szBuffer));
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
		if (SendMessage(&msg))
		{
			rcc = WaitForRCC(rqId, getCommandTimeout());
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
