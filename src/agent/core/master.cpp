/*
** NetXMS multiplatform core agent
** Copyright (C) 2003-2018 Victor Kirhenshtein
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
** File: master.cpp
**
**/

#include "nxagentd.h"

/**
 * Restart agent
 */
LONG RestartAgent();

/**
 * Handler for CMD_GET_PARAMETER command
 */
static void H_GetParameter(NXCPMessage *pRequest, NXCPMessage *pMsg)
{
   TCHAR name[MAX_RUNTIME_PARAM_NAME];
   pRequest->getFieldAsString(VID_PARAMETER, name, MAX_RUNTIME_PARAM_NAME);

   TCHAR value[MAX_RESULT_LENGTH];
   VirtualSession session(0);
   UINT32 rcc = GetParameterValue(name, value, &session);
   pMsg->setField(VID_RCC, rcc);
   if (rcc == ERR_SUCCESS)
      pMsg->setField(VID_VALUE, value);
}

/**
 * Handler for CMD_GET_TABLE command
 */
static void H_GetTable(NXCPMessage *pRequest, NXCPMessage *pMsg)
{
   TCHAR name[MAX_RUNTIME_PARAM_NAME];
   pRequest->getFieldAsString(VID_PARAMETER, name, MAX_RUNTIME_PARAM_NAME);

   Table value;
   VirtualSession session(0);
   UINT32 rcc = GetTableValue(name, &value, &session);
   pMsg->setField(VID_RCC, rcc);
   if (rcc == ERR_SUCCESS)
		value.fillMessage(*pMsg, 0, -1);
}

/**
 * Handler for CMD_GET_LIST command
 */
static void H_GetList(NXCPMessage *pRequest, NXCPMessage *pMsg)
{
   TCHAR name[MAX_RUNTIME_PARAM_NAME];
   pRequest->getFieldAsString(VID_PARAMETER, name, MAX_RUNTIME_PARAM_NAME);

   StringList value;
   VirtualSession session(0);
   UINT32 rcc = GetListValue(name, &value, &session);
   pMsg->setField(VID_RCC, rcc);
   if (rcc == ERR_SUCCESS)
   {
		pMsg->setField(VID_NUM_STRINGS, (UINT32)value.size());
		for(int i = 0; i < value.size(); i++)
			pMsg->setField(VID_ENUM_VALUE_BASE + i, value.get(i));
   }
}

/**
 * Execute action
 */
void ExecuteAction(NXCPMessage *request, NXCPMessage *response, AbstractCommSession *session)
{
   UINT32 rcc = ERR_UNKNOWN_PARAMETER;
   AgentActionExecutor *executor = AgentActionExecutor::createAgentExecutor(request, session, &rcc);
   DebugPrintf(6, _T("Creating AgentActionExecutor instance"));
   if (executor != NULL)
   {
      rcc = (executor->execute() ? ERR_SUCCESS : ERR_EXEC_FAILED);
      if (rcc == ERR_SUCCESS)
         ThreadPoolScheduleRelative(g_agentActionThreadPool, g_execTimeout, AgentActionExecutor::stopAction, executor);
   }
   else
      DebugPrintf(6, _T("AgentActionExecutor instance creation failed"));

   response->setField(VID_RCC, rcc);
}

void ExecuteAction(const TCHAR *cmd, const StringList *args)
{
   VirtualSession *session = new VirtualSession(0);
   AgentActionExecutor *executor = AgentActionExecutor::createAgentExecutor(cmd, args);
   if (executor != NULL && executor->execute())
      ThreadPoolScheduleRelative(g_agentActionThreadPool, g_execTimeout, AgentActionExecutor::stopAction, executor);
}

/**
 * Pipe to master agent
 */
static NamedPipe *s_pipe = NULL;

/**
 * Listener thread for master agent commands
 */
THREAD_RESULT THREAD_CALL MasterAgentListener(void *arg)
{
   while(!(g_dwFlags & AF_SHUTDOWN))
	{
      TCHAR pipeName[MAX_PATH];
      _sntprintf(pipeName, MAX_PIPE_NAME_LEN, _T("nxagentd.subagent.%s"), g_masterAgent);
      s_pipe = NamedPipe::connect(pipeName, 5000);

      if (s_pipe != NULL)
      {
			AgentWriteDebugLog(1, _T("Connected to master agent"));

         PipeMessageReceiver receiver(s_pipe->handle(), 8192, 1048576);  // 8K initial, 1M max
			while(!(g_dwFlags & AF_SHUTDOWN))
			{
            MessageReceiverResult result;
				NXCPMessage *msg = receiver.readMessage(5000, &result);
				if (msg == NULL)
            {
				   if (result == MSGRECV_TIMEOUT)
				      continue;
               AgentWriteDebugLog(6, _T("MasterAgentListener: receiver failure (%s)"), AbstractMessageReceiver::resultToText(result));
					break;
            }

				if  (g_dwFlags & AF_SHUTDOWN)
				{
				   delete msg;
				   break;
				}

            TCHAR buffer[256];
				AgentWriteDebugLog(6, _T("Received message %s from master agent"), NXCPMessageCodeName(msg->getCode(), buffer));

				NXCPMessage response;
				response.setCode(CMD_REQUEST_COMPLETED);
				response.setId(msg->getId());
				switch(msg->getCode())
				{
					case CMD_GET_PARAMETER:
						H_GetParameter(msg, &response);
						break;
					case CMD_GET_TABLE:
						H_GetTable(msg, &response);
						break;
					case CMD_GET_LIST:
						H_GetList(msg, &response);
						break;
               case CMD_EXECUTE_ACTION:
                  ExecuteAction(msg, &response, new ProxySession(msg));
                  break;
					case CMD_GET_PARAMETER_LIST:
						response.setField(VID_RCC, ERR_SUCCESS);
						GetParameterList(&response);
						break;
					case CMD_GET_ENUM_LIST:
						response.setField(VID_RCC, ERR_SUCCESS);
						GetEnumList(&response);
						break;
					case CMD_GET_TABLE_LIST:
						response.setField(VID_RCC, ERR_SUCCESS);
						GetTableList(&response);
						break;
					case CMD_GET_ACTION_LIST:
						response.setField(VID_RCC, ERR_SUCCESS);
						GetActionList(&response);
						break;
               case CMD_SHUTDOWN:
                  Shutdown();
                  ThreadSleep(10);
                  exit(0);
                  break;
               case CMD_RESTART:
                  RestartAgent();
                  ThreadSleep(10);
                  exit(0);
                  break;
					default:
						response.setField(VID_RCC, ERR_UNKNOWN_COMMAND);
						break;
				}
				delete msg;

				// Send response to pipe
				NXCP_MESSAGE *rawMsg = response.serialize();
            bool sendSuccess = s_pipe->write(rawMsg, ntohl(rawMsg->size));
            free(rawMsg);
            if (!sendSuccess)
               break;
			}
			delete_and_null(s_pipe);
			AgentWriteDebugLog(1, _T("Disconnected from master agent"));
		}
		else
		{
         AgentWriteDebugLog(1, _T("Cannot connect to master agent, will retry in 5 seconds"));
         ThreadSleep(5);
		}
	}
	AgentWriteDebugLog(1, _T("Master agent listener stopped"));
	return THREAD_OK;
}

/**
 * Send message to master agent
 */
bool SendMessageToMasterAgent(NXCPMessage *msg)
{
   NXCP_MESSAGE *rawMsg = msg->serialize();
   bool success = SendRawMessageToMasterAgent(rawMsg);
   free(rawMsg);
   return success;
}

/**
 * Send raw message to master agent
 */
bool SendRawMessageToMasterAgent(NXCP_MESSAGE *msg)
{
   if (s_pipe == NULL)
      return false;
   return s_pipe->write(msg, ntohl(msg->size));
}
