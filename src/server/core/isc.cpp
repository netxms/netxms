/* 
** NetXMS - Network Management System
** Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008 Victor Kirhenshtein
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
** File: isc.cpp
**
**/

#include "nxcore.h"


//
// Constants
//

#define MAX_MSG_SIZE       32768

#define ISC_STATE_INIT        0
#define ISC_STATE_CONNECTED   1


//
// Externals
//

void *EF_SetupSession(CSCPMessage *);
void EF_CloseSession(void *);
BOOL EF_ProcessMessage(void *, CSCPMessage *, CSCPMessage *);


//
// Well-known service list
//

static ISC_SERVICE m_serviceList[] = 
{
	{ ISC_SERVICE_EVENT_FORWARDER, _T("EventForwarder"),
	  _T("EnableEventForwarder"), EF_SetupSession, EF_CloseSession, EF_ProcessMessage },
	{ 0, NULL, NULL }
};


//
// Request processing thread
//

static THREAD_RESULT THREAD_CALL ProcessingThread(void *pArg)
{
   SOCKET sock = CAST_FROM_POINTER(pArg, SOCKET);
   int i, err, state = ISC_STATE_INIT;
   CSCP_MESSAGE *pRawMsg, *pRawMsgOut;
   CSCP_BUFFER *pRecvBuffer;
   CSCPMessage *pRequest, response;
   DWORD serviceId;
	void *serviceData = NULL;
   static CSCP_ENCRYPTION_CONTEXT *pDummyCtx = NULL;

   pRawMsg = (CSCP_MESSAGE *)malloc(MAX_MSG_SIZE);
   pRecvBuffer = (CSCP_BUFFER *)malloc(sizeof(CSCP_BUFFER));
   RecvNXCPMessage(0, NULL, pRecvBuffer, 0, NULL, NULL, 0);

   while(1)
   {
      err = RecvNXCPMessage(sock, pRawMsg, pRecvBuffer, MAX_MSG_SIZE, &pDummyCtx, NULL, INFINITE);
      if (err <= 0)
         break;   // Communication error or closed connection

      if (err == 1)
         continue;   // Too big message

      pRequest = new CSCPMessage(pRawMsg);
		if (pRequest->GetCode() == CMD_KEEPALIVE)
		{
			response.SetVariable(VID_RCC, ISC_ERR_SUCCESS);
		}
		else
		{
			if (state == ISC_STATE_INIT)
			{
				if (pRequest->GetCode() == CMD_ISC_CONNECT_TO_SERVICE)
				{
					// Find requested service
      			serviceId = pRequest->GetVariableLong(VID_SERVICE_ID);
					for(i = 0; m_serviceList[i].id != 0; i++)
						if (m_serviceList[i].id == serviceId)
							break;
					if (m_serviceList[i].id != 0)
					{
						// Check if service is enabled
						if (ConfigReadInt(m_serviceList[i].enableParameter, 0) != 0)
						{
							serviceData = m_serviceList[i].setupSession(pRequest);
							if (serviceData != NULL)
							{
								response.SetVariable(VID_RCC, ISC_ERR_SUCCESS);
								state = ISC_STATE_CONNECTED;
							}
							else
							{
								response.SetVariable(VID_RCC, ISC_ERR_SESSION_SETUP_FAILED);
							}
						}
						else
						{
							response.SetVariable(VID_RCC, ISC_ERR_SERVICE_DISABLED);
						}
					}
					else
					{
						response.SetVariable(VID_RCC, ISC_ERR_UNKNOWN_SERVICE);
					}
				}
				else
				{
					response.SetVariable(VID_RCC, ISC_ERR_REQUEST_OUT_OF_STATE);
				}
			}
			else	// Established session
			{
				if (m_serviceList[serviceId].processMsg(serviceData, pRequest, &response))
					break;	// Service asks to close session
			}
		}
		
		response.SetId(pRequest->GetId());
      response.SetCode(CMD_REQUEST_COMPLETED);
      pRawMsgOut = response.CreateMessage();
      SendEx(sock, pRawMsgOut, ntohl(pRawMsgOut->dwSize), 0);
      
		response.DeleteAllVariables();
      free(pRawMsgOut);
      delete pRequest;
   }

	// Close_session
	if (serviceData != NULL)
		m_serviceList[serviceId].closeSession(serviceData);

   shutdown(sock, 2);
   closesocket(sock);
   free(pRawMsg);
   free(pRecvBuffer);
   return THREAD_OK;
}


//
// Local administrative interface listener thread
//

THREAD_RESULT THREAD_CALL ISCListener(void *pArg)
{
   SOCKET sock, sockClient;
   struct sockaddr_in servAddr;
   int errorCount = 0;
   socklen_t iSize;

   // Create socket
   if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
   {
      nxlog_write(MSG_SOCKET_FAILED, EVENTLOG_ERROR_TYPE, "s", _T("ISCListener"));
      return THREAD_OK;
   }

	SetSocketReuseFlag(sock);

   // Fill in local address structure
   memset(&servAddr, 0, sizeof(struct sockaddr_in));
   servAddr.sin_family = AF_INET;
   servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
   servAddr.sin_port = htons(NETXMS_ISC_PORT);

   // Bind socket
   if (bind(sock, (struct sockaddr *)&servAddr, sizeof(struct sockaddr_in)) != 0)
   {
      nxlog_write(MSG_BIND_ERROR, EVENTLOG_ERROR_TYPE, "dse", NETXMS_ISC_PORT, "ISCListener", WSAGetLastError());
      closesocket(sock);
      /* TODO: we should initiate shutdown from here */
      return THREAD_OK;
   }

   // Set up queue
   listen(sock, SOMAXCONN);

   // Wait for connection requests
   while(!ShutdownInProgress())
   {
      iSize = sizeof(struct sockaddr_in);
      if ((sockClient = accept(sock, (struct sockaddr *)&servAddr, &iSize)) == -1)
      {
         int error;

#ifdef _WIN32
         error = WSAGetLastError();
         if (error != WSAEINTR)
#else
         error = errno;
         if (error != EINTR)
#endif
            nxlog_write(MSG_ACCEPT_ERROR, EVENTLOG_ERROR_TYPE, "e", error);
         errorCount++;
         if (errorCount > 1000)
         {
            nxlog_write(MSG_TOO_MANY_ACCEPT_ERRORS, EVENTLOG_WARNING_TYPE, NULL);
            errorCount = 0;
         }
         ThreadSleepMs(500);
      }

      errorCount = 0;     // Reset consecutive errors counter

      // Create new session structure and threads
      ThreadCreate(ProcessingThread, 0, CAST_TO_POINTER(sockClient, void *));
   }

   closesocket(sock);
   return THREAD_OK;
}

