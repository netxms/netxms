/* 
** NetXMS multiplatform core agent
** Copyright (C) 2003-2013 Victor Kirhenshtein
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
 * Handler for CMD_GET_PARAMETER command
 */
static void H_GetParameter(CSCPMessage *pRequest, CSCPMessage *pMsg)
{
   TCHAR name[MAX_PARAM_NAME], value[MAX_RESULT_LENGTH];
   UINT32 dwErrorCode;

   pRequest->GetVariableStr(VID_PARAMETER, name, MAX_PARAM_NAME);
   dwErrorCode = GetParameterValue(0, name, value);
   pMsg->SetVariable(VID_RCC, dwErrorCode);
   if (dwErrorCode == ERR_SUCCESS)
      pMsg->SetVariable(VID_VALUE, value);
}

/**
 * Handler for CMD_GET_TABLE command
 */
static void H_GetTable(CSCPMessage *pRequest, CSCPMessage *pMsg)
{
   TCHAR name[MAX_PARAM_NAME];
	Table value;
   UINT32 dwErrorCode;

   pRequest->GetVariableStr(VID_PARAMETER, name, MAX_PARAM_NAME);
   dwErrorCode = GetTableValue(0, name, &value);
   pMsg->SetVariable(VID_RCC, dwErrorCode);
   if (dwErrorCode == ERR_SUCCESS)
		value.fillMessage(*pMsg, 0, -1);
}

/**
 * Handler for CMD_GET_LIST command
 */
static void H_GetList(CSCPMessage *pRequest, CSCPMessage *pMsg)
{
   TCHAR name[MAX_PARAM_NAME];
	StringList value;
   UINT32 dwErrorCode;

   pRequest->GetVariableStr(VID_PARAMETER, name, MAX_PARAM_NAME);
   dwErrorCode = GetListValue(0, name, &value);
   pMsg->SetVariable(VID_RCC, dwErrorCode);
   if (dwErrorCode == ERR_SUCCESS)
   {
		pMsg->SetVariable(VID_NUM_STRINGS, (UINT32)value.getSize());
		for(int i = 0; i < value.getSize(); i++)
			pMsg->SetVariable(VID_ENUM_VALUE_BASE + i, value.getValue(i));
   }
}

/**
 * Listener thread for master agent commands
 */
THREAD_RESULT THREAD_CALL MasterAgentListener(void *arg)
{
	TCHAR pipeName[MAX_PATH], buffer[256];
	_sntprintf(pipeName, MAX_PATH, _T("\\\\.\\pipe\\nxagentd.subagent.%s"), g_masterAgent);

   while(!(g_dwFlags & AF_SHUTDOWN))
	{
#ifdef _WIN32
		HANDLE hPipe = CreateFile(pipeName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
		if (hPipe != INVALID_HANDLE_VALUE)
		{
			DWORD pipeMode = PIPE_READMODE_MESSAGE;
			SetNamedPipeHandleState(hPipe, &pipeMode, NULL, NULL);
#else
      int hPipe = socket(AF_UNIX, SOCK_STREAM, 0);
      if (hPipe != -1)
      {
	      struct sockaddr_un remote;
	      remote.sun_family = AF_UNIX;
#ifdef UNICODE
         WideCharToMultiByte(CP_UTF8, 0, pipeName, -1, remote.sun_path, MAX_PATH, NULL, NULL);
#else
	      strcpy(remote.sun_path, pipeName);
#endif
	      if (connect(hPipe, (struct sockaddr *)&remote, SUN_LEN(&remote)) == -1)
         {
            close(hPipe);
            hPipe = -1;
         }
      }

      if (hPipe != -1)
      {
#endif
			AgentWriteDebugLog(1, _T("Connected to master agent"));

			while(!(g_dwFlags & AF_SHUTDOWN))
			{
				CSCPMessage *msg = ReadMessageFromPipe(hPipe, NULL);
				if ((msg == NULL) || (g_dwFlags & AF_SHUTDOWN))
					break;
				AgentWriteDebugLog(6, _T("Received message %s from master agent"), NXCPMessageCodeName(msg->GetCode(), buffer));
				
				CSCPMessage response;
				response.SetCode(CMD_REQUEST_COMPLETED);
				response.SetId(msg->GetId());
				switch(msg->GetCode())
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
					case CMD_GET_PARAMETER_LIST:
						response.SetVariable(VID_RCC, ERR_SUCCESS);
						GetParameterList(&response);
						break;
					case CMD_GET_ENUM_LIST:
						response.SetVariable(VID_RCC, ERR_SUCCESS);
						GetEnumList(&response);
						break;
					case CMD_GET_TABLE_LIST:
						response.SetVariable(VID_RCC, ERR_SUCCESS);
						GetTableList(&response);
						break;
					default:
						response.SetVariable(VID_RCC, ERR_UNKNOWN_COMMAND);
						break;
				}
				delete msg;

				// Send response to pipe
				CSCP_MESSAGE *rawMsg = response.CreateMessage();
            bool sendSuccess = SendMessageToPipe(hPipe, rawMsg);
            free(rawMsg);
            if (!sendSuccess)
               break;
			}
#ifdef _WIN32
			CloseHandle(hPipe);
#else
         close(hPipe);
#endif
			AgentWriteDebugLog(1, _T("Disconnected from master agent"));
		}
		else
		{
#ifdef _WIN32
			if (GetLastError() == ERROR_PIPE_BUSY)
			{
				WaitNamedPipe(pipeName, 5000);
			}
			else
#endif
			{
				AgentWriteDebugLog(1, _T("Cannot connect to master agent, will retry in 5 seconds"));
				ThreadSleep(5);
			}
		}
	}
	AgentWriteDebugLog(1, _T("Master agent listener stopped"));
	return THREAD_OK;
}
