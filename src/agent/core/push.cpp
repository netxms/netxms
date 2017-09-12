/* 
** NetXMS multiplatform core agent
** Copyright (C) 2003-2017 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be usefu,,
** but ITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: push.cpp
**
**/

#include "nxagentd.h"

/**
 * Request ID
 */
static UINT64 s_requestIdHigh = (UINT64)time(NULL) << 32;
static VolatileCounter s_requestIdLow = 0;

/**
 * Push parameter's data
 */
bool PushData(const TCHAR *parameter, const TCHAR *value, UINT32 objectId, time_t timestamp)
{
	NXCPMessage msg;
	bool success = false;

	AgentWriteDebugLog(6, _T("PushData: \"%s\" = \"%s\""), parameter, value);

	msg.setCode(CMD_PUSH_DCI_DATA);
	msg.setField(VID_NAME, parameter);
	msg.setField(VID_VALUE, value);
   msg.setField(VID_OBJECT_ID, objectId);
   msg.setFieldFromTime(VID_TIMESTAMP, timestamp);
   msg.setField(VID_REQUEST_ID, s_requestIdHigh | (UINT64)InterlockedIncrement(&s_requestIdLow)); 

   if (g_dwFlags & AF_SUBAGENT_LOADER)
   {
      success = SendMessageToMasterAgent(&msg);
   }
   else
   {
      MutexLock(g_hSessionListAccess);
      for(DWORD i = 0; i < g_dwMaxSessions; i++)
         if (g_pSessionList[i] != NULL)
            if (g_pSessionList[i]->canAcceptTraps())
            {
               g_pSessionList[i]->sendMessage(&msg);
               success = true;
            }
      MutexUnlock(g_hSessionListAccess);
   }
	return success;
}

/**
 * Process push request
 */
static void ProcessPushRequest(NamedPipe *pipe, void *arg)
{
	TCHAR buffer[256];

	AgentWriteDebugLog(5, _T("ProcessPushRequest: connection established"));
   PipeMessageReceiver receiver(pipe->handle(), 8192, 1048576);  // 8K initial, 1M max
	while(true)
	{
      MessageReceiverResult result;
		NXCPMessage *msg = receiver.readMessage(5000, &result);
		if (msg == NULL)
			break;
		AgentWriteDebugLog(6, _T("ProcessPushRequest: received message %s"), NXCPMessageCodeName(msg->getCode(), buffer));
		if (msg->getCode() == CMD_PUSH_DCI_DATA)
		{
         UINT32 objectId = msg->getFieldAsUInt32(VID_OBJECT_ID);
			UINT32 count = msg->getFieldAsUInt32(VID_NUM_ITEMS);
         time_t timestamp = msg->getFieldAsTime(VID_TIMESTAMP);
			UINT32 varId = VID_PUSH_DCI_DATA_BASE;
			for(DWORD i = 0; i < count; i++)
			{
				TCHAR name[MAX_PARAM_NAME], value[MAX_RESULT_LENGTH];
				msg->getFieldAsString(varId++, name, MAX_PARAM_NAME);
				msg->getFieldAsString(varId++, value, MAX_RESULT_LENGTH);
				PushData(name, value, objectId, timestamp);
			}
		}
		delete msg;
	}
	AgentWriteDebugLog(5, _T("ProcessPushRequest: connection by user %s closed"), pipe->user());
}

/**
 * Pipe listener for push requests
 */
static NamedPipeListener *s_listener;

/**
 * Start push connector
 */
void StartPushConnector()
{
   const TCHAR *user = g_config->getValue(_T("/%agent/PushUser"), _T("*"));
   s_listener = NamedPipeListener::create(_T("nxagentd.push"), ProcessPushRequest, NULL,
            (user != NULL) && (user[0] != 0) && _tcscmp(user, _T("*")) ? user : NULL);
   if (s_listener != NULL)
      s_listener->start();
}
