/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2023 Victor Kirhenshtein
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

#define DEBUG_TAG _T("isc")

//
// Constants
//

#define MAX_MSG_SIZE       262144

#define ISC_STATE_INIT        0
#define ISC_STATE_CONNECTED   1

/**
 * Service handlers
 */
bool EF_SetupSession(ISCSession*, NXCPMessage*);
void EF_CloseSession(ISCSession*);
bool EF_ProcessMessage(ISCSession*, NXCPMessage*, NXCPMessage*);

/**
 * Well-known service list
 */
static ISC_SERVICE m_serviceList[] = 
{
	{ ISC_SERVICE_EVENT_FORWARDER, _T("EventForwarder"),
	  _T("Events.ReceiveForwardedEvents"), EF_SetupSession, EF_CloseSession, EF_ProcessMessage },
	{ 0, nullptr, nullptr }
};

/**
 * Request processing thread
 */
static void ProcessingThread(ISCSession *session)
{
   SOCKET sock = session->getSocket();

   int serviceIndex, state = ISC_STATE_INIT;
   NXCPMessage response;

   TCHAR buffer[256], dbgPrefix[128];
	_sntprintf(dbgPrefix, 128, _T("ISC<%s>:"), session->getPeerAddress().toString(buffer));

	SocketMessageReceiver receiver(sock, 4096, MAX_MSG_SIZE);
   while(true)
   {
      MessageReceiverResult result;
      NXCPMessage *pRequest = receiver.readMessage(300000, &result);
      if ((result == MSGRECV_CLOSED) || (result == MSGRECV_COMM_FAILURE) || (result == MSGRECV_TIMEOUT) || (result == MSGRECV_PROTOCOL_ERROR))
		{
			if (result != MSGRECV_CLOSED)
         	nxlog_debug_tag(DEBUG_TAG, 5, _T("%s message read failed: %s"), dbgPrefix, AbstractMessageReceiver::resultToText(result));
			else
         	nxlog_debug_tag(DEBUG_TAG, 5, _T("%s connection closed"), dbgPrefix);
         break;   // Communication error or closed connection
		}

      if (pRequest == nullptr)
         continue;   // Ignore other errors

      if (pRequest->isControl())
      {
         nxlog_debug_tag(DEBUG_TAG, 5, _T("%s received control message %s"), dbgPrefix, NXCPMessageCodeName(pRequest->getCode(), buffer));
         if (pRequest->getCode() == CMD_GET_NXCP_CAPS)
         {
            NXCP_MESSAGE *pRawMsgOut = (NXCP_MESSAGE *)malloc(NXCP_HEADER_SIZE);
            pRawMsgOut->id = htonl(pRequest->getId());
            pRawMsgOut->code = htons((uint16_t)CMD_NXCP_CAPS);
            pRawMsgOut->flags = htons(MF_CONTROL);
            pRawMsgOut->numFields = htonl(NXCP_VERSION << 24);
            pRawMsgOut->size = htonl(NXCP_HEADER_SIZE);
				if (SendEx(sock, pRawMsgOut, NXCP_HEADER_SIZE, 0, nullptr) != NXCP_HEADER_SIZE)
					nxlog_debug_tag(DEBUG_TAG, 5, _T("%s SendEx() failed in ProcessingThread(): %s"), dbgPrefix, GetLastSocketErrorText(buffer, 256));
				MemFree(pRawMsgOut);
         }
      }
		else
		{
			nxlog_debug_tag(DEBUG_TAG, 5, _T("%s message %s received"), dbgPrefix, NXCPMessageCodeName(pRequest->getCode(), buffer));
			if (pRequest->getCode() == CMD_KEEPALIVE)
			{
				response.setField(VID_RCC, ISC_ERR_SUCCESS);
			}
			else
			{
				if (state == ISC_STATE_INIT)
				{
					if (pRequest->getCode() == CMD_ISC_CONNECT_TO_SERVICE)
					{
						// Find requested service
      				uint32_t serviceId = pRequest->getFieldAsUInt32(VID_SERVICE_ID);
						nxlog_debug_tag(DEBUG_TAG, 4, _T("%s attempt to connect to service %d"), dbgPrefix, serviceId);

						int i;
						for(i = 0; m_serviceList[i].id != 0; i++)
							if (m_serviceList[i].id == serviceId)
								break;
						if (m_serviceList[i].id != 0)
						{
							// Check if service is enabled
							if (ConfigReadBoolean(m_serviceList[i].enableParameter, false))
							{
								if (m_serviceList[i].setupSession(session, pRequest))
								{
									response.setField(VID_RCC, ISC_ERR_SUCCESS);
									state = ISC_STATE_CONNECTED;
									serviceIndex = i;
									nxlog_debug_tag(DEBUG_TAG, 4, _T("%s connected to service %d"), dbgPrefix, serviceId);
								}
								else
								{
									response.setField(VID_RCC, ISC_ERR_SESSION_SETUP_FAILED);
								}
							}
							else
							{
								response.setField(VID_RCC, ISC_ERR_SERVICE_DISABLED);
							}
						}
						else
						{
							response.setField(VID_RCC, ISC_ERR_UNKNOWN_SERVICE);
						}
					}
					else
					{
						nxlog_debug_tag(DEBUG_TAG, 4, _T("%s out of state request"), dbgPrefix);
						response.setField(VID_RCC, ISC_ERR_REQUEST_OUT_OF_STATE);
					}
				}
				else	// Established session
				{
					if (m_serviceList[serviceIndex].processMsg(session, pRequest, &response))
						break;	// Service asks to close session
				}
			}

			response.setId(pRequest->getId());
			response.setCode(CMD_REQUEST_COMPLETED);
			NXCP_MESSAGE *pRawMsgOut = response.serialize();
			nxlog_debug_tag(DEBUG_TAG, 5, _T("%s sending message %s"), dbgPrefix, NXCPMessageCodeName(response.getCode(), buffer));
			if (SendEx(sock, pRawMsgOut, ntohl(pRawMsgOut->size), 0, NULL) != (int)ntohl(pRawMsgOut->size))
				nxlog_debug_tag(DEBUG_TAG, 5, _T("%s SendEx() failed in ProcessingThread(): %s"), dbgPrefix, strerror(WSAGetLastError()));
      
			response.deleteAllFields();
			MemFree(pRawMsgOut);
		}
      delete pRequest;
   }

	// Close_session
	if (state == ISC_STATE_CONNECTED)
		m_serviceList[serviceIndex].closeSession(session);
	nxlog_debug_tag(DEBUG_TAG, 3, _T("%s session closed"), dbgPrefix);

   shutdown(sock, 2);
   closesocket(sock);
	delete session;
}

/**
 * ISC listener thread
 */
void ISCListener()
{
   SOCKET sock, sockClient;
   struct sockaddr_in servAddr;
   int errorCount = 0;
   socklen_t iSize;
	ISCSession *session;
	TCHAR buffer[32];

   // Create socket
   if ((sock = CreateSocket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
   {
      TCHAR buffer[1024];
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Unable to create socket for ISC listener (%s)"), GetLastSocketErrorText(buffer, 1024));
      return;
   }

	SetSocketExclusiveAddrUse(sock);
	SetSocketReuseFlag(sock);
#ifndef _WIN32
   fcntl(sock, F_SETFD, fcntl(sock, F_GETFD) | FD_CLOEXEC);
#endif

   // Fill in local address structure
   memset(&servAddr, 0, sizeof(struct sockaddr_in));
   servAddr.sin_family = AF_INET;
   servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
   servAddr.sin_port = htons(NETXMS_ISC_PORT);

   // Bind socket
   if (bind(sock, (struct sockaddr *)&servAddr, sizeof(struct sockaddr_in)) != 0)
   {
      TCHAR buffer[1024];
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Unable to bind socket for ISC listener (%s)"), GetLastSocketErrorText(buffer, 1024));
      closesocket(sock);
      /* TODO: we should initiate shutdown from here */
      return;
   }

   // Set up queue
   listen(sock, SOMAXCONN);
	nxlog_debug_tag(DEBUG_TAG, 1, _T("ISC listener started"));

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
         {
            TCHAR buffer[1024];
            nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Unable to accept incoming ISC connection (%s)"), GetLastSocketErrorText(buffer, 1024));
         }
         errorCount++;
         if (errorCount > 1000)
         {
            nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("Too many consecutive errors on accept() call in ISC listener"));
            errorCount = 0;
         }
         ThreadSleepMs(500);
      }
		else
		{
			errorCount = 0;     // Reset consecutive errors counter

			// Create new session structure and threads
			InetAddress peerAddress = InetAddress::createFromSockaddr(reinterpret_cast<struct sockaddr*>(&servAddr));
			nxlog_debug_tag(DEBUG_TAG, 3, _T("New ISC connection from %s"), peerAddress.toString(buffer));
			session = new ISCSession(sockClient, peerAddress);
			ThreadCreate(ProcessingThread, session);
		}
   }

   closesocket(sock);
	nxlog_debug_tag(DEBUG_TAG, 1, _T("ISC listener stopped"));
}
