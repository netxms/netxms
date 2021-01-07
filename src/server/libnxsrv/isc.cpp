/*
** NetXMS - Network Management System
** Copyright (C) 2003-2021 Victor Kirhenshtein
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
   m_addr = InetAddress::LOOPBACK;
   m_port = NETXMS_ISC_PORT;
   m_socket = -1;
   m_messageReceiver = nullptr;
   m_msgWaitQueue = new MsgWaitQueue;
   m_requestId = 1;
   m_hReceiverThread = INVALID_THREAD_HANDLE;
   m_recvTimeout = 420000;  // 7 minutes
	m_commandTimeout = 10000;	// 10 seconds
   m_mutexDataLock = MutexCreate();
	m_socketLock = MutexCreate();
	m_protocolVersion = NXCP_VERSION;
}

/**
 * Create ISC connector for give IP address and port
 */
ISC::ISC(const InetAddress& addr, uint16_t port)
{
	m_flags = 0;
   m_addr = addr;
   m_port = port;
   m_socket = -1;
   m_messageReceiver = nullptr;
   m_msgWaitQueue = new MsgWaitQueue;
   m_requestId = 1;
   m_hReceiverThread = INVALID_THREAD_HANDLE;
   m_recvTimeout = 420000;  // 7 minutes
	m_commandTimeout = 10000;	// 10 seconds
   m_mutexDataLock = MutexCreate();
	m_socketLock = MutexCreate();
   m_protocolVersion = NXCP_VERSION;
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
	lock();
   if (m_socket != -1)
	{
      closesocket(m_socket);
		m_socket = -1;
	}
	unlock();

   delete m_msgWaitQueue;
   delete m_messageReceiver;

   MutexDestroy(m_mutexDataLock);
	MutexDestroy(m_socketLock);
}

/**
 * Print message. This function is virtual and can be overrided in
 * derived classes. Default implementation will print message to stdout.
 */
void ISC::printMessage(const TCHAR *format, ...)
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
   ssize_t err;
   TCHAR szBuffer[128], szIpAddr[16];
	SOCKET nSocket;

	lock();
	m_messageReceiver = new SocketMessageReceiver(nSocket, 4096, RECEIVER_BUFFER_SIZE);
	unlock();

   // Message receiving loop
   while(true)
   {
      MessageReceiverResult result;
      NXCPMessage *pMsg = m_messageReceiver->readMessage(m_recvTimeout, &result);
      if ((result == MSGRECV_CLOSED) || (result == MSGRECV_TIMEOUT) || (result == MSGRECV_COMM_FAILURE))
		{
			printMessage(_T("ISC::ReceiverThread(): message read error (%s)"), AbstractMessageReceiver::resultToText(result));
         break;
		}

      if (pMsg == nullptr)
         continue;   // Ignore other problems and try to read next message

      if (onMessage(pMsg))
      {
         // message was consumed by handler
         delete pMsg;
      }
      else
      {
         m_msgWaitQueue->put(pMsg);
      }
   }

   // Close socket and mark connection as disconnected
   lock();
   if (err == 0)
      shutdown(m_socket, SHUT_RDWR);
   closesocket(m_socket);
   m_socket = -1;
   m_ctx.reset();
   delete_and_null(m_messageReceiver);
   m_flags &= ~ISCF_IS_CONNECTED;
   unlock();
}

/**
 * Connect to ISC peer
 */
UINT32 ISC::connect(UINT32 service, RSA *pServerKey, BOOL requireEncryption)
{
   TCHAR szBuffer[256];
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

   struct sockaddr *sa;

   // Create socket
   m_socket = CreateSocket(m_addr.getFamily(), SOCK_STREAM, 0);
   if (m_socket == INVALID_SOCKET)
   {
		rcc = ISC_ERR_SOCKET_ERROR;
      printMessage(_T("ISC: Call to socket() failed"));
      goto connect_cleanup;
   }

   // Fill in address structure
   SockAddrBuffer sb;
   sa = m_addr.fillSockAddr(&sb, m_port);

   // Connect to server
   if (ConnectEx(m_socket, sa, SA_LEN(sa), 5000) == -1)
   {
		rcc = ISC_ERR_CONNECT_FAILED;
      printMessage(_T("Cannot establish connection with ISC peer %s"), m_addr.toString(szBuffer));
      goto connect_cleanup;
   }

	// Set non-blocking mode
	SetSocketNonBlocking(m_socket);
	
   if (!NXCPGetPeerProtocolVersion(m_socket, &m_protocolVersion, m_socketLock))
   {
		rcc = ISC_ERR_INVALID_NXCP_VERSION;
      printMessage(_T("Cannot detect NXCP version for ISC peer %s"), m_addr.toString(szBuffer));
      goto connect_cleanup;
   }

   if (m_protocolVersion > NXCP_VERSION)
   {
		rcc = ISC_ERR_INVALID_NXCP_VERSION;
      printMessage(_T("ISC peer %s uses incompatible NXCP version %d"), m_addr.toString(szBuffer), m_protocolVersion);
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
         printMessage(_T("Cannot setup encrypted channel with ISC peer %s"), m_addr.toString(szBuffer));
			goto connect_cleanup;
		}
   }
   else
   {
      if (m_flags & ISCF_REQUIRE_ENCRYPTION)
      {
			rcc = ISC_ERR_ENCRYPTION_REQUIRED;
         printMessage(_T("Cannot setup encrypted channel with ISC peer %s"), m_addr.toString(szBuffer));
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
      printMessage(_T("Communication with ISC peer %s failed (%s)"), m_addr.toString(szBuffer), ISCErrorCodeToText(rcc));
      goto connect_cleanup;
   }

   rcc = connectToService(service);

connect_cleanup:
   if (rcc != ISC_ERR_SUCCESS)
   {
      lock();
		m_flags &= ~ISCF_IS_CONNECTED;
      if (m_socket != -1)
         shutdown(m_socket, SHUT_RDWR);
      unlock();
      ThreadJoin(m_hReceiverThread);
      m_hReceiverThread = INVALID_THREAD_HANDLE;

      lock();
      if (m_socket != -1)
      {
         closesocket(m_socket);
         m_socket = -1;
      }
      m_ctx.reset();
      unlock();
   }

   return rcc;
}

/**
 * Disconnect from ISC peer
 */
void ISC::disconnect()
{
   lock();
   if (m_socket != -1)
   {
      shutdown(m_socket, SHUT_RDWR);
	   m_flags &= ~ISCF_IS_CONNECTED;;
   }
   unlock();
}

/**
 * Send message to peer
 */
BOOL ISC::sendMessage(NXCPMessage *pMsg)
{
   NXCP_MESSAGE *pRawMsg;
   NXCP_ENCRYPTED_MESSAGE *pEnMsg;
   BOOL bResult;

	if (!(m_flags & ISCF_IS_CONNECTED))
		return FALSE;

   if (pMsg->getId() == 0)
   {
      pMsg->setId((UINT32)InterlockedIncrement(&m_requestId));
   }

   pRawMsg = pMsg->serialize();
   if (m_ctx != NULL)
   {
      pEnMsg = m_ctx->encryptMessage(pRawMsg);
      if (pEnMsg != NULL)
      {
         bResult = (SendEx(m_socket, (char *)pEnMsg, ntohl(pEnMsg->size), 0, m_socketLock) == (int)ntohl(pEnMsg->size));
         MemFree(pEnMsg);
      }
      else
      {
         bResult = FALSE;
      }
   }
   else
   {
      bResult = (SendEx(m_socket, (char *)pRawMsg, ntohl(pRawMsg->size), 0, m_socketLock) == (int)ntohl(pRawMsg->size));
   }
   MemFree(pRawMsg);
   return bResult;
}

/**
 * Wait for request completion code
 */
UINT32 ISC::waitForRCC(UINT32 rqId, UINT32 timeOut)
{
   NXCPMessage *pMsg;
   UINT32 dwRetCode;

   pMsg = m_msgWaitQueue->waitForMessage(CMD_REQUEST_COMPLETED, rqId, timeOut);
   if (pMsg != NULL)
   {
      dwRetCode = pMsg->getFieldAsUInt32(VID_RCC);
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
   NXCPMessage msg(m_protocolVersion), *pResp;
   UINT32 dwRqId, dwError, dwResult;

   dwRqId = (UINT32)InterlockedIncrement(&m_requestId);

   PrepareKeyRequestMsg(&msg, pServerKey, false);
   msg.setId(dwRqId);
   if (sendMessage(&msg))
   {
      pResp = waitForMessage(CMD_SESSION_KEY, dwRqId, m_commandTimeout);
      if (pResp != nullptr)
      {
         NXCPEncryptionContext *ctx = nullptr;
         dwResult = SetupEncryptionContext(pResp, &ctx, nullptr, pServerKey, m_protocolVersion);
         switch(dwResult)
         {
            case RCC_SUCCESS:
               lock();
               m_ctx = shared_ptr<NXCPEncryptionContext>(ctx);
               if (m_messageReceiver != nullptr)
                  m_messageReceiver->setEncryptionContext(m_ctx);
               unlock();
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
   NXCPMessage msg(m_protocolVersion);
   uint32_t dwRqId = (UINT32)InterlockedIncrement(&m_requestId);
   msg.setCode(CMD_KEEPALIVE);
   msg.setId(dwRqId);
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
   NXCPMessage msg(m_protocolVersion);
   UINT32 dwRqId = (UINT32)InterlockedIncrement(&m_requestId);
   msg.setCode(CMD_ISC_CONNECT_TO_SERVICE);
   msg.setId(dwRqId);
	msg.setField(VID_SERVICE_ID, service);
   if (sendMessage(&msg))
      return waitForRCC(dwRqId, m_commandTimeout);
   else
      return ISC_ERR_CONNECTION_BROKEN;
}

/**
 * Incoming message handler. Default implementation do nothing and return false.
 * Should return true if message was consumed and should not be put into wait queue
 */
bool ISC::onMessage(NXCPMessage *msg)
{
   return false;
}
