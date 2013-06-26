/*
** NetXMS - Network Management System
** Copyright (C) 2003-2010 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation; either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: isc.cpp
**
**/

#include "libnxsrv.h"

/**
 * Default receiver buffer size
 */
#define RECEIVER_BUFFER_SIZE        262144

/**
 * Texts for ISC error codes
 */
const TCHAR LIBNXSRV_EXPORTABLE *ISCErrorCodeToText(UINT32 code)
{
   static const TCHAR *errorText[] =
	{
		_T("Success"),
		_T("Unknown service"),
		_T("Request out of state"),
		_T("Service disabled"),
		_T("Encryption required"),
		_T("Connection broken"),
		_T("Already connected"),
		_T("Socket error"),
		_T("Connect failed"),
		_T("Invalid or incompatible NXCP version"),
		_T("Request timed out"),
		_T("Command or function not implemented"),
		_T("No suitable ciphers found"),
		_T("Invalid public key"),
		_T("Invalid session key"),
		_T("Internal error"),
		_T("Session setup failed"),
		_T("Object not found"),
		_T("Failed to post event")
	};

	if (code <= ISC_ERR_POST_EVENT_FAILED)
		return errorText[code];
   return _T("Unknown error code");
}

/**
 * Receiver thread starter
 */
THREAD_RESULT THREAD_CALL ISC::receiverThreadStarter(void *arg)
{
   ((ISC *)arg)->receiverThread();
   return THREAD_OK;
}

/**
 * Default constructor for ISC - normally shouldn't be used
 */
ISC::ISC()
{
	m_flags = 0;
   m_addr = inet_addr("127.0.0.1");
   m_port = NETXMS_ISC_PORT;
   m_socket = -1;
   m_msgWaitQueue = new MsgWaitQueue;
   m_requestId = 1;
   m_hReceiverThread = INVALID_THREAD_HANDLE;
   m_ctx = NULL;
   m_recvTimeout = 420000;  // 7 minutes
	m_commandTimeout = 10000;	// 10 seconds
   m_mutexDataLock = MutexCreate();
	m_socketLock = MutexCreate();
}

/**
 * Create ISC connector for give IP address and port
 */
ISC::ISC(UINT32 addr, WORD port)
{
	m_flags = 0;
   m_addr = addr;
   m_port = port;
   m_socket = -1;
   m_msgWaitQueue = new MsgWaitQueue;
   m_requestId = 1;
   m_hReceiverThread = INVALID_THREAD_HANDLE;
   m_ctx = NULL;
   m_recvTimeout = 420000;  // 7 minutes
	m_commandTimeout = 10000;	// 10 seconds
   m_mutexDataLock = MutexCreate();
	m_socketLock = MutexCreate();
}

/**
 * Destructor
 */
ISC::~ISC()
{
   // Disconnect from peer
   disconnect();

   // Wait for receiver thread termination
   ThreadJoin(m_hReceiverThread);
   
	// Close socket if active
	Lock();
   if (m_socket != -1)
	{
      closesocket(m_socket);
		m_socket = -1;
	}
	Unlock();

   delete m_msgWaitQueue;
	if (m_ctx != NULL)
		m_ctx->decRefCount();

   MutexDestroy(m_mutexDataLock);
	MutexDestroy(m_socketLock);
}


//
// Print message. This function is virtual and can be overrided in
// derived classes. Default implementation will print message to stdout.
//

void ISC::PrintMsg(const TCHAR *format, ...)
{
   va_list args;

   va_start(args, format);
   _vtprintf(format, args);
   va_end(args);
   _tprintf(_T("\n"));
}

/**
 * Receiver thread
 */
void ISC::receiverThread()
{
   CSCPMessage *pMsg;
   CSCP_MESSAGE *pRawMsg;
   CSCP_BUFFER *pMsgBuffer;
   BYTE *pDecryptionBuffer = NULL;
   int err;
   TCHAR szBuffer[128], szIpAddr[16];
	SOCKET nSocket;

   // Initialize raw message receiving function
   pMsgBuffer = (CSCP_BUFFER *)malloc(sizeof(CSCP_BUFFER));
   RecvNXCPMessage(0, NULL, pMsgBuffer, 0, NULL, NULL, 0);

   // Allocate space for raw message
   pRawMsg = (CSCP_MESSAGE *)malloc(RECEIVER_BUFFER_SIZE);
#ifdef _WITH_ENCRYPTION
   pDecryptionBuffer = (BYTE *)malloc(RECEIVER_BUFFER_SIZE);
#endif

   // Message receiving loop
   while(1)
   {
      // Receive raw message
      Lock();
		nSocket = m_socket;
		Unlock();
      if ((err = RecvNXCPMessage(nSocket, pRawMsg, pMsgBuffer, RECEIVER_BUFFER_SIZE,
                                 &m_ctx, pDecryptionBuffer, m_recvTimeout)) <= 0)
		{
			PrintMsg(_T("ISC::ReceiverThread(): RecvNXCPMessage() failed: error=%d, socket_error=%d"), err, WSAGetLastError());
         break;
		}

      // Check if we get too large message
      if (err == 1)
      {
         PrintMsg(_T("Received too large message %s (%d bytes)"), 
                  NXCPMessageCodeName(ntohs(pRawMsg->wCode), szBuffer),
                  ntohl(pRawMsg->dwSize));
         continue;
      }

      // Check if we are unable to decrypt message
      if (err == 2)
      {
         PrintMsg(_T("Unable to decrypt received message"));
         continue;
      }

      // Check for timeout
      if (err == 3)
      {
         PrintMsg(_T("Timed out waiting for message"));
         break;
      }

      // Check that actual received packet size is equal to encoded in packet
      if ((int)ntohl(pRawMsg->dwSize) != err)
      {
         PrintMsg(_T("RecvMsg: Bad packet length [dwSize=%d ActualSize=%d]"), ntohl(pRawMsg->dwSize), err);
         continue;   // Bad packet, wait for next
      }

		if (ntohs(pRawMsg->wFlags) & MF_BINARY)
		{
         // Convert message header to host format
         DbgPrintf(6, _T("ISC: Received raw message %s from peer at %s"),
			          NXCPMessageCodeName(ntohs(pRawMsg->wCode), szBuffer), IpToStr(m_addr, szIpAddr));
         onBinaryMessage(pRawMsg);
		}
		else
		{
			// Create message object from raw message
			pMsg = new CSCPMessage(pRawMsg, m_protocolVersion);
			m_msgWaitQueue->put(pMsg);
		}
   }

   // Close socket and mark connection as disconnected
   Lock();
   if (err == 0)
      shutdown(m_socket, SHUT_RDWR);
   closesocket(m_socket);
   m_socket = -1;
	if (m_ctx != NULL)
	{
		m_ctx->decRefCount();;
		m_ctx = NULL;
	}
   m_flags &= ~ISCF_IS_CONNECTED;;
   Unlock();

   free(pRawMsg);
   free(pMsgBuffer);
#ifdef _WITH_ENCRYPTION
   free(pDecryptionBuffer);
#endif
}

/**
 * Connect to ISC peer
 */
UINT32 ISC::connect(UINT32 service, RSA *pServerKey, BOOL requireEncryption)
{
   struct sockaddr_in sa;
   TCHAR szBuffer[256];
   BOOL bForceEncryption = FALSE, bSecondPass = FALSE;
   UINT32 rcc = ISC_ERR_INTERNAL_ERROR;

   // Check if already connected
   if (m_flags & ISCF_IS_CONNECTED)
      return ISC_ERR_ALREADY_CONNECTED;

	if (requireEncryption)
		m_flags |= ISCF_REQUIRE_ENCRYPTION;
	else
		m_flags &= ~ISCF_REQUIRE_ENCRYPTION;

   // Wait for receiver thread from previous connection, if any
   ThreadJoin(m_hReceiverThread);
   m_hReceiverThread = INVALID_THREAD_HANDLE;

   // Check if we need to close existing socket
   if (m_socket != -1)
      closesocket(m_socket);

   // Create socket
   m_socket = socket(AF_INET, SOCK_STREAM, 0);
   if (m_socket == -1)
   {
		rcc = ISC_ERR_SOCKET_ERROR;
      PrintMsg(_T("ISC: Call to socket() failed"));
      goto connect_cleanup;
   }

   // Fill in address structure
   memset(&sa, 0, sizeof(sa));
   sa.sin_family = AF_INET;
   sa.sin_addr.s_addr = m_addr;
   sa.sin_port = htons(m_port);

   // Connect to server
   if (::connect(m_socket, (struct sockaddr *)&sa, sizeof(sa)) == -1)
   {
		rcc = ISC_ERR_CONNECT_FAILED;
      PrintMsg(_T("Cannot establish connection with ISC peer %s"),
               IpToStr(ntohl(m_addr), szBuffer));
      goto connect_cleanup;
   }

	// Set non-blocking mode
	SetSocketNonBlocking(m_socket);
	
   if (!NXCPGetPeerProtocolVersion(m_socket, &m_protocolVersion, m_socketLock))
   {
		rcc = ISC_ERR_INVALID_NXCP_VERSION;
      PrintMsg(_T("Cannot detect NXCP version for ISC peer %s"),
               IpToStr(ntohl(m_addr), szBuffer));
      goto connect_cleanup;
   }

   if (m_protocolVersion > NXCP_VERSION)
   {
		rcc = ISC_ERR_INVALID_NXCP_VERSION;
      PrintMsg(_T("ISC peer %s uses incompatible NXCP version %d"),
               IpToStr(ntohl(m_addr), szBuffer), m_protocolVersion);
      goto connect_cleanup;
   }

   // Start receiver thread
   m_hReceiverThread = ThreadCreateEx(receiverThreadStarter, 0, this);

   // Setup encryption
setup_encryption:
   if (pServerKey != NULL)
   {
      rcc = setupEncryption(pServerKey);
      if ((rcc != ERR_SUCCESS) &&
			 (m_flags & ISCF_REQUIRE_ENCRYPTION))
		{
			PrintMsg(_T("Cannot setup encrypted channel with ISC peer %s"),
						IpToStr(ntohl(m_addr), szBuffer));
			goto connect_cleanup;
		}
   }
   else
   {
      if (m_flags & ISCF_REQUIRE_ENCRYPTION)
      {
			rcc = ISC_ERR_ENCRYPTION_REQUIRED;
			PrintMsg(_T("Cannot setup encrypted channel with ISC peer %s"),
						IpToStr(ntohl(m_addr), szBuffer));
         goto connect_cleanup;
      }
   }

   // Test connectivity
	m_flags |= ISCF_IS_CONNECTED;
   if ((rcc = nop()) != ERR_SUCCESS)
   {
      if (rcc == ISC_ERR_ENCRYPTION_REQUIRED)
      {
			m_flags |= ISCF_REQUIRE_ENCRYPTION;
         goto setup_encryption;
      }
      PrintMsg(_T("Communication with ISC peer %s failed (%s)"), IpToStr(ntohl(m_addr), szBuffer),
               ISCErrorCodeToText(rcc));
      goto connect_cleanup;
   }

   rcc = connectToService(service);

connect_cleanup:
   if (rcc != ISC_ERR_SUCCESS)
   {
      Lock();
		m_flags &= ~ISCF_IS_CONNECTED;
      if (m_socket != -1)
         shutdown(m_socket, SHUT_RDWR);
      Unlock();
      ThreadJoin(m_hReceiverThread);
      m_hReceiverThread = INVALID_THREAD_HANDLE;

      Lock();
      if (m_socket != -1)
      {
         closesocket(m_socket);
         m_socket = -1;
      }

		if (m_ctx != NULL)
		{
			m_ctx->decRefCount();
			m_ctx = NULL;
		}

      Unlock();
   }

   return rcc;
}

/**
 * Disconnect from ISC peer
 */
void ISC::disconnect()
{
   Lock();
   if (m_socket != -1)
   {
      shutdown(m_socket, SHUT_RDWR);
	   m_flags &= ~ISCF_IS_CONNECTED;;
   }
   Unlock();
}

/**
 * Send message to peer
 */
BOOL ISC::sendMessage(CSCPMessage *pMsg)
{
   CSCP_MESSAGE *pRawMsg;
   CSCP_ENCRYPTED_MESSAGE *pEnMsg;
   BOOL bResult;

	if (!(m_flags & ISCF_IS_CONNECTED))
		return FALSE;

   if (pMsg->GetId() == 0)
   {
      pMsg->SetId((UINT32)InterlockedIncrement(&m_requestId));
   }

   pRawMsg = pMsg->CreateMessage();
   if (m_ctx != NULL)
   {
      pEnMsg = CSCPEncryptMessage(m_ctx, pRawMsg);
      if (pEnMsg != NULL)
      {
         bResult = (SendEx(m_socket, (char *)pEnMsg, ntohl(pEnMsg->dwSize), 0, m_socketLock) == (int)ntohl(pEnMsg->dwSize));
         free(pEnMsg);
      }
      else
      {
         bResult = FALSE;
      }
   }
   else
   {
      bResult = (SendEx(m_socket, (char *)pRawMsg, ntohl(pRawMsg->dwSize), 0, m_socketLock) == (int)ntohl(pRawMsg->dwSize));
   }
   free(pRawMsg);
   return bResult;
}

/**
 * Wait for request completion code
 */
UINT32 ISC::waitForRCC(UINT32 rqId, UINT32 timeOut)
{
   CSCPMessage *pMsg;
   UINT32 dwRetCode;

   pMsg = m_msgWaitQueue->waitForMessage(CMD_REQUEST_COMPLETED, rqId, timeOut);
   if (pMsg != NULL)
   {
      dwRetCode = pMsg->GetVariableLong(VID_RCC);
      delete pMsg;
   }
   else
   {
      dwRetCode = ISC_ERR_REQUEST_TIMEOUT;
   }
   return dwRetCode;
}

/**
 * Setup encryption
 */
UINT32 ISC::setupEncryption(RSA *pServerKey)
{
#ifdef _WITH_ENCRYPTION
   CSCPMessage msg(m_protocolVersion), *pResp;
   UINT32 dwRqId, dwError, dwResult;

   dwRqId = (UINT32)InterlockedIncrement(&m_requestId);

   PrepareKeyRequestMsg(&msg, pServerKey, false);
   msg.SetId(dwRqId);
   if (sendMessage(&msg))
   {
      pResp = waitForMessage(CMD_SESSION_KEY, dwRqId, m_commandTimeout);
      if (pResp != NULL)
      {
         dwResult = SetupEncryptionContext(pResp, &m_ctx, NULL, pServerKey, m_protocolVersion);
         switch(dwResult)
         {
            case RCC_SUCCESS:
               dwError = ISC_ERR_SUCCESS;
               break;
            case RCC_NO_CIPHERS:
               dwError = ISC_ERR_NO_CIPHERS;
               break;
            case RCC_INVALID_PUBLIC_KEY:
               dwError = ISC_ERR_INVALID_PUBLIC_KEY;
               break;
            case RCC_INVALID_SESSION_KEY:
               dwError = ISC_ERR_INVALID_SESSION_KEY;
               break;
            default:
               dwError = ISC_ERR_INTERNAL_ERROR;
               break;
         }
			delete pResp;
      }
      else
      {
         dwError = ISC_ERR_REQUEST_TIMEOUT;
      }
   }
   else
   {
      dwError = ISC_ERR_CONNECTION_BROKEN;
   }

   return dwError;
#else
   return ISC_ERR_NOT_IMPLEMENTED;
#endif
}

/**
 * Send dummy command to peer (can be used for keepalive)
 */
UINT32 ISC::nop()
{
   CSCPMessage msg(m_protocolVersion);
   UINT32 dwRqId;

   dwRqId = (UINT32)InterlockedIncrement(&m_requestId);
   msg.SetCode(CMD_KEEPALIVE);
   msg.SetId(dwRqId);
   if (sendMessage(&msg))
      return waitForRCC(dwRqId, m_commandTimeout);
   else
      return ISC_ERR_CONNECTION_BROKEN;
}

/**
 * Connect to requested service
 */
UINT32 ISC::connectToService(UINT32 service)
{
   CSCPMessage msg(m_protocolVersion);
   UINT32 dwRqId;

   dwRqId = (UINT32)InterlockedIncrement(&m_requestId);
   msg.SetCode(CMD_ISC_CONNECT_TO_SERVICE);
   msg.SetId(dwRqId);
	msg.SetVariable(VID_SERVICE_ID, service);
   if (sendMessage(&msg))
      return waitForRCC(dwRqId, m_commandTimeout);
   else
      return ISC_ERR_CONNECTION_BROKEN;
}

/**
 * Binary message handler. Default implementation do nothing.
 */
void ISC::onBinaryMessage(CSCP_MESSAGE *rawMsg)
{
}
