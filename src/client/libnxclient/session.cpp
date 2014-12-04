/*
** NetXMS - Network Management System
** Client Library
** Copyright (C) 2003-2014 Victor Kirhenshtein
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
** File: session.cpp
**
**/

#include "libnxclient.h"

/**
 * Max NXCP message size
 */
#define MAX_MSG_SIZE    4194304

/**
 * Default controller destructor
 */
Controller::~Controller()
{
}

/**
 * Constructor
 */
NXCSession::NXCSession()
{
   m_controllers = new StringObjectMap<Controller>(true);
   m_msgId = 0;
   m_dataLock = MutexCreate();
   m_msgSendLock = MutexCreate();
   m_connected = false;
   m_disconnected = false;
   m_hSocket = INVALID_SOCKET;
   m_msgWaitQueue = NULL;
   m_encryptionContext = NULL;
   m_receiverThread = INVALID_THREAD_HANDLE;
   m_serverVersion[0] = 0;
   m_serverTimeZone[0] = 0;
   m_userId = 0;
   m_systemRights = 0;
   m_commandTimeout = 60000;  // 60 seconds
}

/**
 * Destructor
 */
NXCSession::~NXCSession()
{
   disconnect();
   delete m_controllers;
   MutexDestroy(m_dataLock);
   MutexDestroy(m_msgSendLock);
}

/**
 * Get controller
 */
Controller *NXCSession::getController(const TCHAR *name)
{
   MutexLock(m_dataLock);
   Controller *c = m_controllers->get(name);
   if (c == NULL)
   {
      if (!_tcsicmp(name, CONTROLLER_ALARMS))
         c = new AlarmController(this);
      else if (!_tcsicmp(name, CONTROLLER_DATA_COLLECTION))
         c = new DataCollectionController(this);
      else if (!_tcsicmp(name, CONTROLLER_EVENTS))
         c = new EventController(this);
      else if (!_tcsicmp(name, CONTROLLER_OBJECTS))
         c = new ObjectController(this);
      else if (!_tcsicmp(name, CONTROLLER_SERVER))
         c = new ServerController(this);

      if (c != NULL)
         m_controllers->set(name, c);
   }
   MutexUnlock(m_dataLock);
   return c;
}

/**
 * Disconnect
 */
void NXCSession::disconnect()
{
   if (!m_connected || m_disconnected)
      return;

   // Close socket
   if (m_hSocket != INVALID_SOCKET)
   {
      shutdown(m_hSocket, SHUT_RDWR);
      closesocket(m_hSocket);
   }

   if (m_receiverThread != INVALID_THREAD_HANDLE)
      ThreadJoin(m_receiverThread);

   // Clear message wait queue
   delete m_msgWaitQueue;

   m_connected = false;
   m_disconnected = true;
}

/**
 * Connect to server
 */
UINT32 NXCSession::connect(const TCHAR *host, const TCHAR *login, const TCHAR *password, UINT32 flags, const TCHAR *clientInfo)
{
   if (m_connected || m_disconnected)
      return RCC_OUT_OF_STATE_REQUEST;

   TCHAR hostname[128];
   nx_strncpy(hostname, host, 128);
   Trim(hostname);

   // Check if server given in form host:port
   // If IPv6 address is given, it must be enclosed in [] if port is also specified
   WORD port = SERVER_LISTEN_PORT_FOR_CLIENTS;
   TCHAR *p = _tcsrchr(hostname, _T(':'));
   if ((p != NULL) && (p != hostname) &&
       (((hostname[0] != _T('[')) && (NumChars(hostname, _T(':') == 1)) || (*(p - 1) == _T(']')))))
   {
      *p = 0;
      p++;
      TCHAR *eptr;
      int n = _tcstol(p, &eptr, 10);
      if ((*eptr != 0) || (n < 1) || (n > 65535))
         return RCC_INVALID_ARGUMENT;
      port = (WORD)n;
   }
   DebugPrintf(_T("NXCSession::connect: host=\"%s\" port=%d"), hostname, (int)port);

   InetAddress addr = InetAddress::resolveHostName(hostname);
   if (!addr.isValid())
      return RCC_COMM_FAILURE;

   struct sockaddr *sa;
   int saLen;

   struct sockaddr_in sa4;
#ifdef WITH_IPV6
   struct sockaddr_in6 sa6;
#endif
   if (addr.getFamily() == AF_INET)
   {
      saLen = sizeof(sa4);
      memset(&sa4, 0, saLen);
      sa = (struct sockaddr *)&sa4;
      sa4.sin_family = AF_INET;
      sa4.sin_addr.s_addr = htonl(addr.getAddressV4());
      sa4.sin_port = htons(port);
   }
   else
   {
#ifdef WITH_IPV6
      saLen = sizeof(sa6);
      memset(&sa6, 0, saLen);
      sa = (struct sockaddr *)&sa6;
      sa6.sin6_family = AF_INET6;
      memcpy(&sa6.sin6_addr, addr.getAddressV6(), 16);
      sa6.sin6_port = ntohs(port);
#else
      return RCC_NOT_IMPLEMENTED;
#endif
   }

   m_hSocket = socket(addr.getFamily(), SOCK_STREAM, 0);
   if (m_hSocket == INVALID_SOCKET)
      return RCC_COMM_FAILURE;

   if (::connect(m_hSocket, sa, saLen) != 0)
   {
      closesocket(m_hSocket);
      m_hSocket = INVALID_SOCKET;
      return RCC_COMM_FAILURE;
   }

   m_connected = true;
   m_msgWaitQueue = new MsgWaitQueue();
   m_receiverThread = ThreadCreateEx(receiverThreadStarter, 0, this);
   
   UINT32 rcc = RCC_COMM_FAILURE;

   // Query server information
   NXCPMessage msg;
   msg.setId(createMessageId());
   msg.setCode(CMD_GET_SERVER_INFO);
   if (sendMessage(&msg))
   {
      NXCPMessage *response = waitForMessage(CMD_REQUEST_COMPLETED, msg.getId());
      if (response != NULL)
      {
         rcc = response->getFieldAsUInt32(VID_RCC);
         if (rcc == RCC_SUCCESS)
         {
            response->getFieldAsBinary(VID_SERVER_ID, m_serverId, 8);
            response->getFieldAsString(VID_SERVER_VERSION, m_serverVersion, 64);
				response->getFieldAsString(VID_TIMEZONE, m_serverTimeZone, MAX_TZ_LEN);

            if (flags & NXCF_EXACT_VERSION_MATCH)
            {
               if (_tcsncmp(m_serverVersion, NETXMS_VERSION_STRING, 64))
                  rcc = RCC_VERSION_MISMATCH;
            }
				if (!(flags & NXCF_IGNORE_PROTOCOL_VERSION))
				{
					if (response->getFieldAsUInt32(VID_PROTOCOL_VERSION) != CLIENT_PROTOCOL_VERSION)
						rcc = RCC_BAD_PROTOCOL;
				}
         }
         delete response;
      }
      else
      {
         rcc = RCC_TIMEOUT;
      }
   }

   // Request encryption if needed
   if ((rcc == RCC_SUCCESS) && (flags & NXCF_ENCRYPT))
   {
      msg.deleteAllFields();
      msg.setId(createMessageId());
      msg.setCode(CMD_REQUEST_ENCRYPTION);
      if (sendMessage(&msg))
      {
         rcc = waitForRCC(msg.getId());
      }
      else
      {
         rcc = RCC_COMM_FAILURE;
      }
   }

   if (rcc == RCC_SUCCESS)
   {
      msg.deleteAllFields();
      msg.setId(createMessageId());
      msg.setCode(CMD_LOGIN);
      msg.setField(VID_LOGIN_NAME, login);
	   if (flags & NXCF_USE_CERTIFICATE)
	   {
         /* TODO: implement certificate auth */
			msg.setField(VID_AUTH_TYPE, (UINT16)NETXMS_AUTH_TYPE_CERTIFICATE);
	   }
	   else
	   {
		   msg.setField(VID_PASSWORD, password);
		   msg.setField(VID_AUTH_TYPE, (UINT16)NETXMS_AUTH_TYPE_PASSWORD);
	   }
      msg.setField(VID_CLIENT_INFO, (clientInfo != NULL) ? clientInfo : _T("Unnamed Client"));
      msg.setField(VID_LIBNXCL_VERSION, NETXMS_VERSION_STRING);
      
      TCHAR buffer[64];
      GetOSVersionString(buffer, 64);
      msg.setField(VID_OS_INFO, buffer);

      if (sendMessage(&msg))
      {
         NXCPMessage *response = waitForMessage(CMD_LOGIN_RESP, msg.getId());
         if (response != NULL)
         {
            rcc = response->getFieldAsUInt32(VID_RCC);
            if (rcc == RCC_SUCCESS)
            {
               m_userId = response->getFieldAsUInt32(VID_USER_ID);
               m_systemRights = response->getFieldAsUInt64(VID_USER_SYS_RIGHTS);
               m_passwordChangeNeeded = response->getFieldAsBoolean(VID_CHANGE_PASSWD_FLAG);
            }
            delete response;
         }
         else
         {
            rcc = RCC_TIMEOUT;
         }
      }
      else
      {
         rcc = RCC_COMM_FAILURE;
      }
   }

   m_connected = true;
   if (rcc != RCC_SUCCESS)
      disconnect();

   return rcc;
}

/**
 * Send message
 */
bool NXCSession::sendMessage(NXCPMessage *msg)
{
   if (!m_connected)
      return false;

   TCHAR buffer[128];
   DebugPrintf(_T("NXCSession::sendMessage(\"%s\", id:%d)"), NXCPMessageCodeName(msg->getCode(), buffer), msg->getId());

   bool result;
   NXCP_MESSAGE *rawMsg = msg->createMessage();
	MutexLock(m_msgSendLock);
   if (m_encryptionContext != NULL)
   {
      NXCP_ENCRYPTED_MESSAGE *emsg = m_encryptionContext->encryptMessage(rawMsg);
      if (emsg != NULL)
      {
         result = (SendEx(m_hSocket, (char *)emsg, ntohl(emsg->size), 0, NULL) == (int)ntohl(emsg->size));
         free(emsg);
      }
      else
      {
         result = FALSE;
      }
   }
   else
   {
      result = (SendEx(m_hSocket, (char *)rawMsg, ntohl(rawMsg->size), 0, NULL) == (int)ntohl(rawMsg->size));
   }
	MutexUnlock(m_msgSendLock);
   free(rawMsg);
   return result;
}

/**
 * Wait for message
 */
NXCPMessage *NXCSession::waitForMessage(UINT16 code, UINT32 id, UINT32 timeout)
{
   if (!m_connected)
      return NULL;

   return m_msgWaitQueue->waitForMessage(code, id, (timeout == 0) ? m_commandTimeout : timeout);
}

/**
 * Wait for CMD_REQUEST_COMPLETED message and return RCC
 */
UINT32 NXCSession::waitForRCC(UINT32 id, UINT32 timeout)
{
   NXCPMessage *response = waitForMessage(CMD_REQUEST_COMPLETED, id, timeout);
   if (response == NULL)
      return RCC_TIMEOUT;

   UINT32 rcc = response->getFieldAsUInt32(VID_RCC);
   delete response;
   return rcc;
}

/**
 * Starter for receiver thread
 */
THREAD_RESULT THREAD_CALL NXCSession::receiverThreadStarter(void *arg)
{
   ((NXCSession *)arg)->receiverThread();
   return THREAD_OK;
}

/**
 * Receiver thread
 */
void NXCSession::receiverThread()
{
   SocketMessageReceiver receiver(m_hSocket, 4096, MAX_MSG_SIZE);
   while(true)
   {
      MessageReceiverResult result;
      NXCPMessage *msg = receiver.readMessage(900000, &result);

      // Check for decryption error
      if (result == MSGRECV_DECRYPTION_FAILURE)
      {
         DebugPrintf(_T("NXCSession::receiverThread: Unable to decrypt received message"));
         continue;
      }

      // Receive error
      if (msg == NULL)
      {
         DebugPrintf(_T("NXCSession::receiverThread: message receiving error (%s)"), AbstractMessageReceiver::resultToText(result));
         break;
      }

      TCHAR buffer[128];
      DebugPrintf(_T("NXCSession::receiveMessage(\"%s\", id:%d)"), NXCPMessageCodeName(msg->getCode(), buffer), msg->getId());

      switch(msg->getCode())
      {
         case CMD_REQUEST_SESSION_KEY:
            if (m_encryptionContext == NULL)
            {
               NXCPMessage *response;
               MutexLock(m_dataLock);
               SetupEncryptionContext(msg, &m_encryptionContext, &response, NULL, NXCP_VERSION);
               receiver.setEncryptionContext(m_encryptionContext);
               MutexUnlock(m_dataLock);
               sendMessage(response);
               delete response;
            }
            break;
         case CMD_NOTIFY:
            onNotify(msg);
            break;
         default:
            m_msgWaitQueue->put(msg);
            msg = NULL;    // prevent destruction
            break;
      }
      delete msg;
   }
}

/**
 * Notification message handler
 */
void NXCSession::onNotify(NXCPMessage *msg)
{
}
