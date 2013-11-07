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

#define MAX_MSG_SIZE       262144

#define ISC_STATE_INIT        0
#define ISC_STATE_CONNECTED   1


//
// Externals
//

BOOL EF_SetupSession(ISCSession *, CSCPMessage *);
void EF_CloseSession(ISCSession *);
BOOL EF_ProcessMessage(ISCSession *, CSCPMessage *, CSCPMessage *);


//
// Well-known service list
//

static ISC_SERVICE m_serviceList[] = 
{
	{ ISC_SERVICE_EVENT_FORWARDER, _T("EventForwarder"),
	  _T("ReceiveForwardedEvents"), EF_SetupSession, EF_CloseSession, EF_ProcessMessage },
	{ 0, NULL, NULL }
};


//
// Request processing thread
//

static THREAD_RESULT THREAD_CALL ProcessingThread(void *arg)
{
	ISCSession *session = (ISCSession *)arg;
   SOCKET sock = session->GetSocket();
   int i, err, serviceIndex, state = ISC_STATE_INIT;
   CSCP_MESSAGE *pRawMsg, *pRawMsgOut;
   CSCP_BUFFER *pRecvBuffer;
   CSCPMessage *pRequest, response;
   UINT32 serviceId;
	void *serviceData = NULL;
	TCHAR buffer[256], dbgPrefix[128];
	WORD wFlags;
	static NXCPEncryptionContext *pDummyCtx = NULL;

	_sntprintf(dbgPrefix, 128, _T("ISC<%s>:"), IpToStr(session->GetPeerAddress(), buffer));

   pRawMsg = (CSCP_MESSAGE *)malloc(MAX_MSG_SIZE);
   pRecvBuffer = (CSCP_BUFFER *)malloc(sizeof(CSCP_BUFFER));
   RecvNXCPMessage(0, NULL, pRecvBuffer, 0, NULL, NULL, 0);

   while(1)
   {
      err = RecvNXCPMessage(sock, pRawMsg, pRecvBuffer, MAX_MSG_SIZE, &pDummyCtx, NULL, INFINITE);
      if (err <= 0)
		{
			if (err == -1)
         	DbgPrintf(5, _T("%s RecvNXCPMessage() failed: syserr=%s"), dbgPrefix, strerror(WSAGetLastError()));
			else
         	DbgPrintf(5, _T("%s connection closed"), dbgPrefix);
         break;   // Communication error or closed connection
		}

      if (err == 1)
         continue;   // Too big message

      wFlags = ntohs(pRawMsg->wFlags);
      if (wFlags & MF_CONTROL)
      {
         // Convert message header to host format
         pRawMsg->dwId = ntohl(pRawMsg->dwId);
         pRawMsg->wCode = ntohs(pRawMsg->wCode);
         pRawMsg->dwNumVars = ntohl(pRawMsg->dwNumVars);
         DbgPrintf(5, _T("%s received control message %s"), dbgPrefix, NXCPMessageCodeName(pRawMsg->wCode, buffer));

         if (pRawMsg->wCode == CMD_GET_NXCP_CAPS)
         {
            pRawMsgOut = (CSCP_MESSAGE *)malloc(CSCP_HEADER_SIZE);
            pRawMsgOut->dwId = htonl(pRawMsg->dwId);
            pRawMsgOut->wCode = htons((WORD)CMD_NXCP_CAPS);
            pRawMsgOut->wFlags = htons(MF_CONTROL);
            pRawMsgOut->dwNumVars = htonl(NXCP_VERSION << 24);
            pRawMsgOut->dwSize = htonl(CSCP_HEADER_SIZE);
				if (SendEx(sock, pRawMsgOut, CSCP_HEADER_SIZE, 0, NULL) != CSCP_HEADER_SIZE)
					DbgPrintf(5, _T("%s SendEx() failed in ProcessingThread(): %s"), dbgPrefix, strerror(WSAGetLastError()));
				free(pRawMsgOut);
         }
      }
		else
		{
			pRequest = new CSCPMessage(pRawMsg);
			DbgPrintf(5, _T("%s message %s received"), dbgPrefix, NXCPMessageCodeName(pRequest->GetCode(), buffer));
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
						DbgPrintf(4, _T("%s attempt to connect to service %d"), dbgPrefix, serviceId);
						for(i = 0; m_serviceList[i].id != 0; i++)
							if (m_serviceList[i].id == serviceId)
								break;
						if (m_serviceList[i].id != 0)
						{
							// Check if service is enabled
							if (ConfigReadInt(m_serviceList[i].enableParameter, 0) != 0)
							{
								if (m_serviceList[i].setupSession(session, pRequest))
								{
									response.SetVariable(VID_RCC, ISC_ERR_SUCCESS);
									state = ISC_STATE_CONNECTED;
									serviceIndex = i;
									DbgPrintf(4, _T("%s connected to service %d"), dbgPrefix, serviceId);
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
						DbgPrintf(4, _T("%s out of state request"), dbgPrefix);
						response.SetVariable(VID_RCC, ISC_ERR_REQUEST_OUT_OF_STATE);
					}
				}
				else	// Established session
				{
					if (m_serviceList[serviceIndex].processMsg(session, pRequest, &response))
						break;	// Service asks to close session
				}
			}
			
			response.SetId(pRequest->GetId());
			response.SetCode(CMD_REQUEST_COMPLETED);
			pRawMsgOut = response.CreateMessage();
			DbgPrintf(5, _T("%s sending message %s"), dbgPrefix, NXCPMessageCodeName(response.GetCode(), buffer));
			if (SendEx(sock, pRawMsgOut, ntohl(pRawMsgOut->dwSize), 0, NULL) != (int)ntohl(pRawMsgOut->dwSize))
				DbgPrintf(5, _T("%s SendEx() failed in ProcessingThread(): %s"), dbgPrefix, strerror(WSAGetLastError()));
      
			response.deleteAllVariables();
			free(pRawMsgOut);
			delete pRequest;
		}
   }

	// Close_session
	if (state == ISC_STATE_CONNECTED)
		m_serviceList[serviceIndex].closeSession(session);
	DbgPrintf(3, _T("%s session closed"), dbgPrefix);

   shutdown(sock, 2);
   closesocket(sock);
   free(pRawMsg);
   free(pRecvBuffer);
	delete (ISCSession *)arg;
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
	ISCSession *session;
	TCHAR buffer[32];

   // Create socket
   if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
   {
      nxlog_write(MSG_SOCKET_FAILED, EVENTLOG_ERROR_TYPE, "s", _T("ISCListener"));
      return THREAD_OK;
   }

	SetSocketExclusiveAddrUse(sock);
	SetSocketReuseFlag(sock);

   // Fill in local address structure
   memset(&servAddr, 0, sizeof(struct sockaddr_in));
   servAddr.sin_family = AF_INET;
   servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
   servAddr.sin_port = htons(NETXMS_ISC_PORT);

   // Bind socket
   if (bind(sock, (struct sockaddr *)&servAddr, sizeof(struct sockaddr_in)) != 0)
   {
      nxlog_write(MSG_BIND_ERROR, EVENTLOG_ERROR_TYPE, "dse", NETXMS_ISC_PORT, _T("ISCListener"), WSAGetLastError());
      closesocket(sock);
      /* TODO: we should initiate shutdown from here */
      return THREAD_OK;
   }

   // Set up queue
   listen(sock, SOMAXCONN);
	DbgPrintf(1, _T("ISC listener started"));

   // Wait for connection requests
   while(!IsShutdownInProgress())
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
		else
		{
			errorCount = 0;     // Reset consecutive errors counter

			// Create new session structure and threads
			DbgPrintf(3, _T("New ISC connection from %s"), IpToStr(ntohl(servAddr.sin_addr.s_addr), buffer));
			session = new ISCSession(sockClient, &servAddr);
			ThreadCreate(ProcessingThread, 0, session);
		}
   }

   closesocket(sock);
	DbgPrintf(1, _T("ISC listener stopped"));
   return THREAD_OK;
}

