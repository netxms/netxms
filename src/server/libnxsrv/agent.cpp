/*
** NetXMS - Network Management System
** Server Library
** Copyright (C) 2003-2018 Victor Kirhenshtein
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
** File: agent.cpp
**
**/

#include "libnxsrv.h"
#include <stdarg.h>
#include <nxstat.h>

#ifndef _WIN32
#define _tell(f) lseek(f,0,SEEK_CUR)
#endif

#define DEBUG_TAG    _T("agent.conn")

/**
 * Constants
 */
#define MAX_MSG_SIZE    268435456

/**
 * Agent connection thread pool
 */
ThreadPool LIBNXSRV_EXPORTABLE *g_agentConnectionThreadPool = NULL;

/**
 * Unique connection ID
 */
static VolatileCounter s_connectionId = 0;

/**
 * Static data
 */
#ifdef _WITH_ENCRYPTION
static int m_iDefaultEncryptionPolicy = ENCRYPTION_ALLOWED;
#else
static int m_iDefaultEncryptionPolicy = ENCRYPTION_DISABLED;
#endif

/**
 * Set default encryption policy for agent communication
 */
void LIBNXSRV_EXPORTABLE SetAgentDEP(int iPolicy)
{
#ifdef _WITH_ENCRYPTION
   m_iDefaultEncryptionPolicy = iPolicy;
#endif
}

/**
 * Receiver thread starter
 */
THREAD_RESULT THREAD_CALL AgentConnection::receiverThreadStarter(void *pArg)
{
   ThreadSetName("AgentReceiver");
   ((AgentConnection *)pArg)->receiverThread();
   ((AgentConnection *)pArg)->decInternalRefCount();
   return THREAD_OK;
}

/**
 * Constructor for AgentConnection
 */
AgentConnection::AgentConnection(const InetAddress& addr, WORD port, int authMethod, const TCHAR *secret, bool allowCompression)
{
   m_internalRefCount = 1;
   m_userRefCount = 1;
   m_debugId = InterlockedIncrement(&s_connectionId);
   m_addr = addr;
   m_wPort = port;
   m_iAuthMethod = authMethod;
   if (secret != NULL)
   {
#ifdef UNICODE
		WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR, secret, -1, m_szSecret, MAX_SECRET_LENGTH, NULL, NULL);
		m_szSecret[MAX_SECRET_LENGTH - 1] = 0;
#else
      nx_strncpy(m_szSecret, secret, MAX_SECRET_LENGTH);
#endif
   }
   else
   {
      m_szSecret[0] = 0;
   }
   m_allowCompression = allowCompression;
   m_channel = NULL;
   m_tLastCommandTime = 0;
   m_pMsgWaitQueue = new MsgWaitQueue;
   m_requestId = 0;
	m_connectionTimeout = 5000;	// 5 seconds
   m_dwCommandTimeout = 5000;   // Default timeout 5 seconds
   m_isConnected = false;
   m_mutexDataLock = MutexCreate();
	m_mutexSocketWrite = MutexCreate();
   m_hReceiverThread = INVALID_THREAD_HANDLE;
   m_pCtx = NULL;
   m_iEncryptionPolicy = m_iDefaultEncryptionPolicy;
   m_useProxy = false;
   m_iProxyAuth = AUTH_NONE;
   m_wProxyPort = 4700;
   m_dwRecvTimeout = 420000;  // 7 minutes
   m_nProtocolVersion = NXCP_VERSION;
	m_hCurrFile = -1;
   m_deleteFileOnDownloadFailure = true;
	m_condFileDownload = ConditionCreate(TRUE);
   m_fileDownloadSucceeded = false;
	m_fileUploadInProgress = false;
   m_sendToClientMessageCallback = NULL;
   m_dwDownloadRequestId = 0;
   m_downloadProgressCallback = NULL;
   m_downloadProgressCallbackArg = NULL;
}

/**
 * Destructor
 */
AgentConnection::~AgentConnection()
{
   debugPrintf(7, _T("AgentConnection destructor called (this=%p, thread=%p)"), this, (void *)m_hReceiverThread);

   ThreadDetach(m_hReceiverThread);

   delete m_pMsgWaitQueue;
	if (m_pCtx != NULL)
		m_pCtx->decRefCount();

	if (m_hCurrFile != -1)
	{
		_close(m_hCurrFile);
		onFileDownload(false);
	}
   else if (m_sendToClientMessageCallback != NULL)
   {
      onFileDownload(false);
   }

	if (m_channel != NULL)
	   m_channel->decRefCount();

   MutexDestroy(m_mutexDataLock);
	MutexDestroy(m_mutexSocketWrite);
	ConditionDestroy(m_condFileDownload);
}

/**
 * Write debug output
 */
void AgentConnection::debugPrintf(int level, const TCHAR *format, ...)
{
   va_list args;
   va_start(args, format);
   nxlog_debug_tag_object2(DEBUG_TAG, m_debugId, level, format, args);
   va_end(args);
}

/**
 * Receiver thread
 */
void AgentConnection::receiverThread()
{
   AbstractCommChannel *channel = m_channel;
   UINT32 msgBufferSize = 1024;

   // Initialize raw message receiving function
   NXCP_BUFFER *msgBuffer = (NXCP_BUFFER *)malloc(sizeof(NXCP_BUFFER));
   NXCPInitBuffer(msgBuffer);

   // Allocate space for raw message
   NXCP_MESSAGE *rawMsg = (NXCP_MESSAGE *)malloc(msgBufferSize);
#ifdef _WITH_ENCRYPTION
   BYTE *decryptionBuffer = (BYTE *)malloc(msgBufferSize);
#else
   BYTE *decryptionBuffer = NULL;
#endif

   while(true)
   {
      // Shrink buffer after receiving large message
      if (msgBufferSize > 131072)
      {
         msgBufferSize = 131072;
         rawMsg = (NXCP_MESSAGE *)realloc(rawMsg, msgBufferSize);
         if (decryptionBuffer != NULL)
            decryptionBuffer = (BYTE *)realloc(decryptionBuffer, msgBufferSize);
      }

      // Receive raw message
      int rc = RecvNXCPMessageEx(channel, &rawMsg, msgBuffer, &msgBufferSize,
                                 &m_pCtx, (decryptionBuffer != NULL) ? &decryptionBuffer : NULL,
                                 m_dwRecvTimeout, MAX_MSG_SIZE);
      if (rc <= 0)
      {
         if ((rc != 0) && (WSAGetLastError() != WSAESHUTDOWN))
            debugPrintf(6, _T("AgentConnection::ReceiverThread(): RecvNXCPMessage() failed: error=%d, socket_error=%d"), rc, WSAGetLastError());
         else
            debugPrintf(6, _T("AgentConnection::ReceiverThread(): communication channel shutdown"));
         break;
      }

      // Check if we get too large message
      if (rc == 1)
      {
         TCHAR buffer[64];
         debugPrintf(6, _T("Received too large message %s (%d bytes)"), NXCPMessageCodeName(ntohs(rawMsg->code), buffer), ntohl(rawMsg->size));
         continue;
      }

      // Check if we are unable to decrypt message
      if (rc == 2)
      {
         debugPrintf(6, _T("Unable to decrypt received message"));
         continue;
      }

      // Check for timeout
      if (rc == 3)
      {
         if (m_fileUploadInProgress)
            continue;   // Receive timeout may occur when uploading large files via slow links
         debugPrintf(6, _T("Timed out waiting for message"));
         break;
      }

      // Check that actual received packet size is equal to encoded in packet
      if ((int)ntohl(rawMsg->size) != rc)
      {
         debugPrintf(6, _T("RecvMsg: Bad packet length [size=%d ActualSize=%d]"), ntohl(rawMsg->size), rc);
         continue;   // Bad packet, wait for next
      }

      if (ntohs(rawMsg->flags) & MF_BINARY)
      {
         // Convert message header to host format
         rawMsg->id = ntohl(rawMsg->id);
         rawMsg->code = ntohs(rawMsg->code);
         rawMsg->numFields = ntohl(rawMsg->numFields);
         if (nxlog_get_debug_level_tag_object(DEBUG_TAG, m_debugId) >= 6)
         {
            TCHAR buffer[64];
            debugPrintf(6, _T("Received raw message %s (%d) from agent at %s"),
               NXCPMessageCodeName(rawMsg->code, buffer), rawMsg->id, (const TCHAR *)m_addr.toString());
         }

         if ((rawMsg->code == CMD_FILE_DATA) && (rawMsg->id == m_dwDownloadRequestId))
         {
            if (m_sendToClientMessageCallback != NULL)
            {
               rawMsg->code = ntohs(rawMsg->code);
               rawMsg->numFields = ntohl(rawMsg->numFields);
               m_sendToClientMessageCallback(rawMsg, m_downloadProgressCallbackArg);

               if (ntohs(rawMsg->flags) & MF_END_OF_FILE)
               {
                  m_sendToClientMessageCallback = NULL;
                  onFileDownload(true);
               }
               else
               {
                  if (m_downloadProgressCallback != NULL)
                  {
                     m_downloadProgressCallback(rawMsg->size - (NXCP_HEADER_SIZE + 8), m_downloadProgressCallbackArg);
                  }
               }
            }
            else
            {
               if (m_hCurrFile != -1)
               {
                  if (_write(m_hCurrFile, rawMsg->fields, rawMsg->numFields) == (int)rawMsg->numFields)
                  {
                     if (ntohs(rawMsg->flags) & MF_END_OF_FILE)
                     {
                        _close(m_hCurrFile);
                        m_hCurrFile = -1;

                        onFileDownload(true);
                     }
                     else
                     {
                        if (m_downloadProgressCallback != NULL)
                        {
                           m_downloadProgressCallback(_tell(m_hCurrFile), m_downloadProgressCallbackArg);
                        }
                     }
                  }
               }
               else
               {
                  // I/O error
                  _close(m_hCurrFile);
                  m_hCurrFile = -1;

                  onFileDownload(false);
               }
            }
         }
         else if ((rawMsg->code == CMD_ABORT_FILE_TRANSFER) && (rawMsg->id == m_dwDownloadRequestId))
         {
            if (m_sendToClientMessageCallback != NULL)
            {
               rawMsg->code = ntohs(rawMsg->code);
               rawMsg->numFields = ntohl(rawMsg->numFields);
               m_sendToClientMessageCallback(rawMsg, m_downloadProgressCallbackArg);
               m_sendToClientMessageCallback = NULL;

               onFileDownload(false);
            }
            else
            {
               //error on agent side
               _close(m_hCurrFile);
               m_hCurrFile = -1;

               onFileDownload(false);
            }
         }
         else if (rawMsg->code == CMD_TCP_PROXY_DATA)
         {
            processTcpProxyData(rawMsg->id, rawMsg->fields, rawMsg->numFields);
         }
      }
      else if (ntohs(rawMsg->flags) & MF_CONTROL)
      {
         // Convert message header to host format
         rawMsg->id = ntohl(rawMsg->id);
         rawMsg->code = ntohs(rawMsg->code);
         rawMsg->flags = ntohs(rawMsg->flags);
         rawMsg->numFields = ntohl(rawMsg->numFields);
         if (nxlog_get_debug_level_tag_object(DEBUG_TAG, m_debugId) >= 6)
         {
            TCHAR buffer[64];
            debugPrintf(6, _T("Received control message %s from agent at %s"),
               NXCPMessageCodeName(rawMsg->code, buffer), (const TCHAR *)m_addr.toString());
         }
         m_pMsgWaitQueue->put((NXCP_MESSAGE *)nx_memdup(rawMsg, ntohl(rawMsg->size)));
      }
      else
      {
         // Create message object from raw message
         NXCPMessage *msg = NXCPMessage::deserialize(rawMsg, m_nProtocolVersion);
         if (msg != NULL)
         {
            if (nxlog_get_debug_level_tag_object(DEBUG_TAG, m_debugId) >= 6)
            {
               TCHAR buffer[64];
               debugPrintf(6, _T("Received message %s (%d) from agent at %s"),
                  NXCPMessageCodeName(msg->getCode(), buffer), msg->getId(), (const TCHAR *)m_addr.toString());
            }
            switch(msg->getCode())
            {
               case CMD_REQUEST_COMPLETED:
               case CMD_SESSION_KEY:
                  m_pMsgWaitQueue->put(msg);
                  break;
               case CMD_TRAP:
                  if (g_agentConnectionThreadPool != NULL)
                  {
                     incInternalRefCount();
                     ThreadPoolExecute(g_agentConnectionThreadPool, this, &AgentConnection::onTrapCallback, msg);
                  }
                  else
                  {
                     delete msg;
                  }
                  break;
               case CMD_SYSLOG_RECORDS:
                  if (g_agentConnectionThreadPool != NULL)
                  {
                     incInternalRefCount();
                     ThreadPoolExecute(g_agentConnectionThreadPool, this, &AgentConnection::onSyslogMessageCallback, msg);
                  }
                  else
                  {
                     delete msg;
                  }
                  break;
               case CMD_PUSH_DCI_DATA:
                  if (g_agentConnectionThreadPool != NULL)
                  {
                     incInternalRefCount();
                     ThreadPoolExecute(g_agentConnectionThreadPool, this, &AgentConnection::onDataPushCallback, msg);
                  }
                  else
                  {
                     delete msg;
                  }
                  break;
               case CMD_DCI_DATA:
                  if (g_agentConnectionThreadPool != NULL)
                  {
                     incInternalRefCount();
                     ThreadPoolExecute(g_agentConnectionThreadPool, this, &AgentConnection::processCollectedDataCallback, msg);
                  }
                  else
                  {
                     NXCPMessage response;
                     response.setCode(CMD_REQUEST_COMPLETED);
                     response.setId(msg->getId());
                     response.setField(VID_RCC, ERR_INTERNAL_ERROR);
                     sendMessage(&response);
                     delete msg;
                  }
                  break;
               case CMD_FILE_MONITORING:
                  onFileMonitoringData(msg);
                  delete msg;
                  break;
               case CMD_SNMP_TRAP:
                  if (g_agentConnectionThreadPool != NULL)
                  {
                     incInternalRefCount();
                     ThreadPoolExecute(g_agentConnectionThreadPool, this, &AgentConnection::onSnmpTrapCallback, msg);
                  }
                  else
                  {
                     delete msg;
                  }
                  break;
               case CMD_CLOSE_TCP_PROXY:
                  processTcpProxyData(msg->getFieldAsUInt32(VID_CHANNEL_ID), NULL, 0);
                  delete msg;
                  break;
               default:
                  if (processCustomMessage(msg))
                     delete msg;
                  else
                     m_pMsgWaitQueue->put(msg);
                  break;
            }
         }
         else
         {
            debugPrintf(6, _T("RecvMsg: message deserialization error"));
         }
      }
   }
   debugPrintf(6, _T("Receiver loop terminated"));

   // Close socket and mark connection as disconnected
   lock();
	if (m_hCurrFile != -1)
	{
		_close(m_hCurrFile);
		m_hCurrFile = -1;
		onFileDownload(false);
	}
   else if (m_sendToClientMessageCallback != NULL)
   {
      m_sendToClientMessageCallback = NULL;
      onFileDownload(false);
   }

	debugPrintf(6, _T("Closing communication channel"));
	channel->close();
	channel->decRefCount();
	if (m_pCtx != NULL)
	{
		m_pCtx->decRefCount();
		m_pCtx = NULL;
	}
   m_isConnected = false;
   unlock();

   free(rawMsg);
   free(msgBuffer);
#ifdef _WITH_ENCRYPTION
   free(decryptionBuffer);
#endif

   debugPrintf(6, _T("Receiver thread stopped"));
}

/**
 * Create channel. Default implementation creates socket channel.
 */
AbstractCommChannel *AgentConnection::createChannel()
{
   SOCKET s = m_useProxy ?
            ConnectToHost(m_proxyAddr, m_wProxyPort, m_connectionTimeout) :
            ConnectToHost(m_addr, m_wPort, m_connectionTimeout);

   // Connect to server
   if (s == INVALID_SOCKET)
   {
      TCHAR buffer[64];
      debugPrintf(5, _T("Cannot establish connection with agent at %s:%d"),
               m_useProxy ? m_proxyAddr.toString(buffer) : m_addr.toString(buffer),
               (int)(m_useProxy ? m_wProxyPort : m_wPort));
      return NULL;
   }

   return new SocketCommChannel(s);
}

/**
 * Acquire communication channel. Caller must call decRefCount to release channel.
 */
AbstractCommChannel *AgentConnection::acquireChannel()
{
   lock();
   AbstractCommChannel *channel = m_channel;
   if (channel != NULL)
      channel->incRefCount();
   unlock();
   return channel;
}

/**
 * Connect to agent
 */
bool AgentConnection::connect(RSA *pServerKey, UINT32 *pdwError, UINT32 *pdwSocketError, UINT64 serverId)
{
   TCHAR szBuffer[256];
   bool success = false;
   bool forceEncryption = false;
   bool secondPass = false;
   UINT32 dwError = 0;

   if (pdwError != NULL)
      *pdwError = ERR_INTERNAL_ERROR;

   if (pdwSocketError != NULL)
      *pdwSocketError = 0;

   // Check if already connected
   if (m_isConnected)
      return false;

   // Wait for receiver thread from previous connection, if any
   ThreadJoin(m_hReceiverThread);
   m_hReceiverThread = INVALID_THREAD_HANDLE;

   // Check if we need to close existing channel
   if (m_channel != NULL)
   {
      m_channel->decRefCount();
      m_channel = NULL;
   }

   m_channel = createChannel();
   if (m_channel == NULL)
   {
      debugPrintf(6, _T("Cannot create communication channel"));
      dwError = ERR_CONNECT_FAILED;
      goto connect_cleanup;
   }

   if (!NXCPGetPeerProtocolVersion(m_channel, &m_nProtocolVersion, m_mutexSocketWrite))
   {
      debugPrintf(6, _T("Protocol version negotiation failed"));
      dwError = ERR_INTERNAL_ERROR;
      goto connect_cleanup;
   }
   debugPrintf(6, _T("Using NXCP version %d"), m_nProtocolVersion);

   // Start receiver thread
   incInternalRefCount();
   m_channel->incRefCount();  // for receiver thread
   m_hReceiverThread = ThreadCreateEx(receiverThreadStarter, 0, this);

   // Setup encryption
setup_encryption:
   if ((m_iEncryptionPolicy == ENCRYPTION_PREFERRED) ||
       (m_iEncryptionPolicy == ENCRYPTION_REQUIRED) ||
       forceEncryption)    // Agent require encryption
   {
      if (pServerKey != NULL)
      {
         dwError = setupEncryption(pServerKey);
         if ((dwError != ERR_SUCCESS) &&
             ((m_iEncryptionPolicy == ENCRYPTION_REQUIRED) || forceEncryption))
            goto connect_cleanup;
      }
      else
      {
         if ((m_iEncryptionPolicy == ENCRYPTION_REQUIRED) || forceEncryption)
         {
            dwError = ERR_ENCRYPTION_REQUIRED;
            goto connect_cleanup;
         }
      }
   }

   // Authenticate itself to agent
   if ((dwError = authenticate(m_useProxy && !secondPass)) != ERR_SUCCESS)
   {
      if ((dwError == ERR_ENCRYPTION_REQUIRED) &&
          (m_iEncryptionPolicy != ENCRYPTION_DISABLED))
      {
         forceEncryption = true;
         goto setup_encryption;
      }
      debugPrintf(5, _T("Authentication to agent %s failed (%s)"), m_addr.toString(szBuffer),
               AgentErrorCodeToText(dwError));
      goto connect_cleanup;
   }

   // Test connectivity and inform agent about server capabilities
   if ((dwError = setServerCapabilities()) != ERR_SUCCESS)
   {
      if ((dwError == ERR_ENCRYPTION_REQUIRED) &&
          (m_iEncryptionPolicy != ENCRYPTION_DISABLED))
      {
         forceEncryption = true;
         goto setup_encryption;
      }
      if (dwError != ERR_UNKNOWN_COMMAND) // Older agents may not support enable IPv6 command
      {
         debugPrintf(5, _T("Communication with agent %s failed (%s)"), m_addr.toString(szBuffer), AgentErrorCodeToText(dwError));
         goto connect_cleanup;
      }
   }

   if (m_useProxy && !secondPass)
   {
      dwError = setupProxyConnection();
      if (dwError != ERR_SUCCESS)
         goto connect_cleanup;
		lock();
		if (m_pCtx != NULL)
		{
			m_pCtx->decRefCount();
	      m_pCtx = NULL;
		}
		unlock();

		debugPrintf(6, _T("Proxy connection established"));

		// Renegotiate NXCP version with actual target agent
	   NXCP_MESSAGE msg;
	   msg.id = 0;
	   msg.numFields = 0;
	   msg.size = htonl(NXCP_HEADER_SIZE);
	   msg.code = htons(CMD_GET_NXCP_CAPS);
	   msg.flags = htons(MF_CONTROL);
	   if (m_channel->send(&msg, NXCP_HEADER_SIZE, m_mutexSocketWrite) == NXCP_HEADER_SIZE)
	   {
	      NXCP_MESSAGE *rsp = m_pMsgWaitQueue->waitForRawMessage(CMD_NXCP_CAPS, 0, m_dwCommandTimeout);
	      if (rsp != NULL)
	      {
	         m_nProtocolVersion = rsp->numFields >> 24;
	         free(rsp);
	      }
	      else
	      {
	         // assume that peer doesn't understand CMD_GET_NXCP_CAPS message
	         // and set version number to 1
	         m_nProtocolVersion = 1;
	      }
         debugPrintf(6, _T("Using NXCP version %d after re-negotioation"), m_nProtocolVersion);
	   }
	   else
	   {
	      debugPrintf(6, _T("Protocol version re-negotiation failed - cannot send CMD_GET_NXCP_CAPS message"));
         dwError = ERR_CONNECTION_BROKEN;
         goto connect_cleanup;
	   }

      secondPass = true;
      forceEncryption = false;
      goto setup_encryption;
   }

   if (serverId != 0)
      setServerId(serverId);

   success = true;
   dwError = ERR_SUCCESS;

connect_cleanup:
   if (!success)
   {
		if (pdwSocketError != NULL)
			*pdwSocketError = (UINT32)WSAGetLastError();

      lock();
      if (m_channel != NULL)
         m_channel->shutdown();
      unlock();
      ThreadJoin(m_hReceiverThread);
      m_hReceiverThread = INVALID_THREAD_HANDLE;

      lock();
      if (m_channel != NULL)
      {
         m_channel->close();
         m_channel->decRefCount();
         m_channel = NULL;
      }

		if (m_pCtx != NULL)
		{
			m_pCtx->decRefCount();
	      m_pCtx = NULL;
		}

      unlock();
   }
   m_isConnected = success;
   if (pdwError != NULL)
      *pdwError = dwError;
   return success;
}

/**
 * Disconnect from agent
 */
void AgentConnection::disconnect()
{
   debugPrintf(6, _T("disconnect() called"));
   lock();
	if (m_hCurrFile != -1)
	{
		_close(m_hCurrFile);
		m_hCurrFile = -1;
		onFileDownload(false);
	}
	else if (m_sendToClientMessageCallback != NULL)
	{
      m_sendToClientMessageCallback = NULL;
      onFileDownload(false);
	}

   if (m_channel != NULL)
   {
      m_channel->shutdown();
      m_channel->decRefCount();
      m_channel = NULL;
   }
   m_isConnected = false;
   unlock();
   debugPrintf(6, _T("Disconnect completed"));
}

/**
 * Set authentication data
 */
void AgentConnection::setAuthData(int method, const TCHAR *secret)
{
   m_iAuthMethod = method;
#ifdef UNICODE
	WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR, secret, -1, m_szSecret, MAX_SECRET_LENGTH, NULL, NULL);
	m_szSecret[MAX_SECRET_LENGTH - 1] = 0;
#else
   nx_strncpy(m_szSecret, secret, MAX_SECRET_LENGTH);
#endif
}

/**
 * Get interface list from agent
 */
InterfaceList *AgentConnection::getInterfaceList()
{
   StringList *data;
   if (getList(_T("Net.InterfaceList"), &data) != ERR_SUCCESS)
      return NULL;

   InterfaceList *pIfList = new InterfaceList(data->size());

   // Parse result set. Each line should have the following format:
   // index ip_address/mask_bits iftype mac_address name
   for(int i = 0; i < data->size(); i++)
   {
      TCHAR *line = _tcsdup(data->get(i));
      TCHAR *pBuf = line;
      UINT32 ifIndex = 0;

      // Index
      TCHAR *pChar = _tcschr(pBuf, ' ');
      if (pChar != NULL)
      {
         *pChar = 0;
         ifIndex = _tcstoul(pBuf, NULL, 10);
         pBuf = pChar + 1;
      }

      bool newInterface = false;
      InterfaceInfo *iface = pIfList->findByIfIndex(ifIndex);
      if (iface == NULL)
      {
         iface = new InterfaceInfo(ifIndex);
         newInterface = true;
      }

      // Address and mask
      pChar = _tcschr(pBuf, _T(' '));
      if (pChar != NULL)
      {
         TCHAR *pSlash;
         static TCHAR defaultMask[] = _T("24");

         *pChar = 0;
         pSlash = _tcschr(pBuf, _T('/'));
         if (pSlash != NULL)
         {
            *pSlash = 0;
            pSlash++;
         }
         else     // Just a paranoia protection, should'n happen if agent working correctly
         {
            pSlash = defaultMask;
         }
         InetAddress addr = InetAddress::parse(pBuf);
         if (addr.isValid())
         {
            addr.setMaskBits(_tcstol(pSlash, NULL, 10));
            // Agent may return 0.0.0.0/0 for interfaces without IP address
            if ((addr.getFamily() != AF_INET) || (addr.getAddressV4() != 0))
               iface->ipAddrList.add(addr);
         }
         pBuf = pChar + 1;
      }

      if (newInterface)
      {
         // Interface type
         pChar = _tcschr(pBuf, ' ');
         if (pChar != NULL)
         {
            *pChar = 0;

            TCHAR *eptr;
            iface->type = _tcstoul(pBuf, &eptr, 10);

            // newer agents can return if_type(mtu)
            if (*eptr == _T('('))
            {
               pBuf = eptr + 1;
               eptr = _tcschr(pBuf, _T(')'));
               if (eptr != NULL)
               {
                  *eptr = 0;
                  iface->mtu = _tcstol(pBuf, NULL, 10);
               }
            }

            pBuf = pChar + 1;
         }

         // MAC address
         pChar = _tcschr(pBuf, ' ');
         if (pChar != NULL)
         {
            *pChar = 0;
            StrToBin(pBuf, iface->macAddr, MAC_ADDR_LENGTH);
            pBuf = pChar + 1;
         }

         // Name (set description to name)
         _tcslcpy(iface->name, pBuf, MAX_DB_STRING);
         _tcslcpy(iface->description, pBuf, MAX_DB_STRING);

         pIfList->add(iface);
      }
      free(line);
   }

   delete data;
   return pIfList;
}

/**
 * Get parameter value
 */
UINT32 AgentConnection::getParameter(const TCHAR *pszParam, UINT32 dwBufSize, TCHAR *pszBuffer)
{
   if (!m_isConnected)
      return ERR_NOT_CONNECTED;

   NXCPMessage msg(m_nProtocolVersion);
   UINT32 dwRqId = generateRequestId();
   msg.setCode(CMD_GET_PARAMETER);
   msg.setId(dwRqId);
   msg.setField(VID_PARAMETER, pszParam);

   UINT32 dwRetCode;
   if (sendMessage(&msg))
   {
      NXCPMessage *response = waitForMessage(CMD_REQUEST_COMPLETED, dwRqId, m_dwCommandTimeout);
      if (response != NULL)
      {
         dwRetCode = response->getFieldAsUInt32(VID_RCC);
         if (dwRetCode == ERR_SUCCESS)
         {
            if (response->isFieldExist(VID_VALUE))
            {
               response->getFieldAsString(VID_VALUE, pszBuffer, dwBufSize);
            }
            else
            {
               dwRetCode = ERR_MALFORMED_RESPONSE;
               debugPrintf(3, _T("Malformed response to CMD_GET_PARAMETER"));
            }
         }
         delete response;
      }
      else
      {
         dwRetCode = ERR_REQUEST_TIMEOUT;
      }
   }
   else
   {
      dwRetCode = ERR_CONNECTION_BROKEN;
   }
   return dwRetCode;
}

/**
 * Get ARP cache
 */
ArpCache *AgentConnection::getArpCache()
{
   StringList *data;
   if (getList(_T("Net.ArpCache"), &data) != ERR_SUCCESS)
      return NULL;

   // Create empty structure
   ArpCache *arpCache = new ArpCache();

   TCHAR szByte[4], *pBuf, *pChar;
   szByte[2] = 0;

   // Parse data lines
   // Each line has form of XXXXXXXXXXXX a.b.c.d n
   // where XXXXXXXXXXXX is a MAC address (12 hexadecimal digits)
   // a.b.c.d is an IP address in decimal dotted notation
   // n is an interface index
   for(int i = 0; i < data->size(); i++)
   {
      TCHAR *line = _tcsdup(data->get(i));
      pBuf = line;
      if (_tcslen(pBuf) < 20)     // Invalid line
      {
         debugPrintf(7, _T("AgentConnection::getArpCache(): invalid line received from agent (\"%s\")"), line);
         free(line);
         continue;
      }

      // MAC address
      BYTE macAddr[6];
      for(int j = 0; j < 6; j++)
      {
         memcpy(szByte, pBuf, sizeof(TCHAR) * 2);
         macAddr[j] = (BYTE)_tcstol(szByte, NULL, 16);
         pBuf += 2;
      }

      // IP address
      while(*pBuf == ' ')
         pBuf++;
      pChar = _tcschr(pBuf, _T(' '));
      if (pChar != NULL)
         *pChar = 0;
      InetAddress ipAddr = InetAddress::parse(pBuf);

      // Interface index
      UINT32 ifIndex = (pChar != NULL) ? _tcstoul(pChar + 1, NULL, 10) : 0;

      arpCache->addEntry(ipAddr, MacAddress(macAddr, 6), ifIndex);

      free(line);
   }

   delete data;
   return arpCache;
}

/**
 * Send dummy command to agent (can be used for keepalive)
 */
UINT32 AgentConnection::nop()
{
   if (!m_isConnected)
      return ERR_CONNECTION_BROKEN;

   NXCPMessage msg(m_nProtocolVersion);
   UINT32 dwRqId;

   dwRqId = generateRequestId();
   msg.setCode(CMD_KEEPALIVE);
   msg.setId(dwRqId);
   if (sendMessage(&msg))
      return waitForRCC(dwRqId, m_dwCommandTimeout);
   else
      return ERR_CONNECTION_BROKEN;
}

/**
 * inform agent about server capabilities
 */
UINT32 AgentConnection::setServerCapabilities()
{
   NXCPMessage msg(m_nProtocolVersion);
   UINT32 dwRqId = generateRequestId();
   msg.setCode(CMD_SET_SERVER_CAPABILITIES);
   msg.setField(VID_ENABLED, (INT16)1);   // Enables IPv6 on pre-2.0 agents
   msg.setField(VID_IPV6_SUPPORT, (INT16)1);
   msg.setField(VID_BULK_RECONCILIATION, (INT16)1);
   msg.setField(VID_ENABLE_COMPRESSION, (INT16)(m_allowCompression ? 1 : 0));
   msg.setId(dwRqId);
   if (sendMessage(&msg))
      return waitForRCC(dwRqId, m_dwCommandTimeout);
   else
      return ERR_CONNECTION_BROKEN;
}

/**
 * Set server ID
 */
UINT32 AgentConnection::setServerId(UINT64 serverId)
{
   NXCPMessage msg(m_nProtocolVersion);
   UINT32 dwRqId = generateRequestId();
   msg.setCode(CMD_SET_SERVER_ID);
   msg.setField(VID_SERVER_ID, serverId);
   msg.setId(dwRqId);
   if (sendMessage(&msg))
      return waitForRCC(dwRqId, m_dwCommandTimeout);
   else
      return ERR_CONNECTION_BROKEN;
}

/**
 * Wait for request completion code
 */
UINT32 AgentConnection::waitForRCC(UINT32 dwRqId, UINT32 dwTimeOut)
{
   UINT32 rcc;
   NXCPMessage *response = m_pMsgWaitQueue->waitForMessage(CMD_REQUEST_COMPLETED, dwRqId, dwTimeOut);
   if (response != NULL)
   {
      rcc = response->getFieldAsUInt32(VID_RCC);
      delete response;
   }
   else
   {
      rcc = ERR_REQUEST_TIMEOUT;
   }
   return rcc;
}

/**
 * Send message to agent
 */
bool AgentConnection::sendMessage(NXCPMessage *pMsg)
{
   AbstractCommChannel *channel = acquireChannel();
   if (channel == NULL)
      return false;

   if (nxlog_get_debug_level_tag_object(DEBUG_TAG, m_debugId) >= 6)
   {
      TCHAR buffer[64];
      debugPrintf(6, _T("Sending message %s (%d) to agent at %s"),
         NXCPMessageCodeName(pMsg->getCode(), buffer), pMsg->getId(), (const TCHAR *)m_addr.toString());
   }

   bool success;
   NXCP_MESSAGE *rawMsg = pMsg->serialize(m_allowCompression);
	NXCPEncryptionContext *pCtx = acquireEncryptionContext();
   if (pCtx != NULL)
   {
      NXCP_ENCRYPTED_MESSAGE *pEnMsg = pCtx->encryptMessage(rawMsg);
      if (pEnMsg != NULL)
      {
         success = (channel->send(pEnMsg, ntohl(pEnMsg->size), m_mutexSocketWrite) == (int)ntohl(pEnMsg->size));
         free(pEnMsg);
      }
      else
      {
         success = false;
      }
		pCtx->decRefCount();
   }
   else
   {
      success = (channel->send(rawMsg, ntohl(rawMsg->size), m_mutexSocketWrite) == (int)ntohl(rawMsg->size));
   }
   free(rawMsg);
   channel->decRefCount();
   return success;
}

/**
 * Send raw message to agent
 */
bool AgentConnection::sendRawMessage(NXCP_MESSAGE *pMsg)
{
   AbstractCommChannel *channel = acquireChannel();
   if (channel == NULL)
      return false;

   bool success;
   NXCP_MESSAGE *rawMsg = pMsg;
	NXCPEncryptionContext *pCtx = acquireEncryptionContext();
   if (pCtx != NULL)
   {
      NXCP_ENCRYPTED_MESSAGE *pEnMsg = pCtx->encryptMessage(rawMsg);
      if (pEnMsg != NULL)
      {
         success = (channel->send(pEnMsg, ntohl(pEnMsg->size), m_mutexSocketWrite) == (int)ntohl(pEnMsg->size));
         free(pEnMsg);
      }
      else
      {
         success = false;
      }
		pCtx->decRefCount();
   }
   else
   {
      success = (channel->send(rawMsg, ntohl(rawMsg->size), m_mutexSocketWrite) == (int)ntohl(rawMsg->size));
   }
   channel->decRefCount();
   return success;
}

/**
 * Callback for sending raw NXCP message in background
 */
void AgentConnection::postRawMessageCallback(NXCP_MESSAGE *msg)
{
   sendRawMessage(msg);
   free(msg);
   decInternalRefCount();
}

/**
 * Send raw NXCP message in background. Provided message will be destroyed after sending.
 */
void AgentConnection::postRawMessage(NXCP_MESSAGE *msg)
{
   incInternalRefCount();
   ThreadPoolExecuteSerialized(g_agentConnectionThreadPool, _T("TcpProxy"), this, &AgentConnection::postRawMessageCallback, msg);
}

/**
 * Callback for processing incoming event on separate thread
 */
void AgentConnection::onTrapCallback(NXCPMessage *msg)
{
   onTrap(msg);
   delete msg;
   decInternalRefCount();
}

/**
 * Trap handler. Should be overriden in derived classes to implement
 * actual trap processing. Default implementation do nothing.
 */
void AgentConnection::onTrap(NXCPMessage *pMsg)
{
}

/**
 * Callback for processing incoming syslog message on separate thread
 */
void AgentConnection::onSyslogMessageCallback(NXCPMessage *msg)
{
   onSyslogMessage(msg);
   delete msg;
   decInternalRefCount();
}

/**
 * Syslog message handler. Should be overriden in derived classes to implement
 * actual message processing. Default implementation do nothing.
 */
void AgentConnection::onSyslogMessage(NXCPMessage *pMsg)
{
}

/**
 * Callback for processing data push on separate thread
 */
void AgentConnection::onDataPushCallback(NXCPMessage *msg)
{
   onDataPush(msg);
   delete msg;
   decInternalRefCount();
}

/**
 * Data push handler. Should be overriden in derived classes to implement
 * actual data push processing. Default implementation do nothing.
 */
void AgentConnection::onDataPush(NXCPMessage *pMsg)
{
}

/**
 * Monitoring data handler. Should be overriden in derived classes to implement
 * actual monitoring data processing. Default implementation do nothing.
 */
void AgentConnection::onFileMonitoringData(NXCPMessage *pMsg)
{
}

/**
 * Callback for processing data push on separate thread
 */
void AgentConnection::onSnmpTrapCallback(NXCPMessage *msg)
{
   onSnmpTrap(msg);
   delete msg;
   decInternalRefCount();
}

/**
 * SNMP trap handler. Should be overriden in derived classes to implement
 * actual SNMP trap processing. Default implementation do nothing.
 */
void AgentConnection::onSnmpTrap(NXCPMessage *pMsg)
{
}

/**
 * Custom message handler
 * If returns true, message considered as processed and will not be placed in wait queue
 */
bool AgentConnection::processCustomMessage(NXCPMessage *pMsg)
{
	return false;
}

/**
 * Get list of values
 */
UINT32 AgentConnection::getList(const TCHAR *param, StringList **list)
{
   UINT32 rcc;
   *list = NULL;
   if (m_isConnected)
   {
      NXCPMessage msg(CMD_GET_LIST, generateRequestId(), m_nProtocolVersion);
      msg.setField(VID_PARAMETER, param);
      if (sendMessage(&msg))
      {
         NXCPMessage *response = waitForMessage(CMD_REQUEST_COMPLETED, msg.getId(), m_dwCommandTimeout);
         if (response != NULL)
         {
            rcc = response->getFieldAsUInt32(VID_RCC);
            if (rcc == ERR_SUCCESS)
            {
               *list = new StringList();
               int count = response->getFieldAsInt32(VID_NUM_STRINGS);
               for(int i = 0; i < count; i++)
                  (*list)->addPreallocated(response->getFieldAsString(VID_ENUM_VALUE_BASE + i));
            }
            delete response;
         }
         else
         {
            rcc = ERR_REQUEST_TIMEOUT;
         }
      }
      else
      {
         rcc = ERR_CONNECTION_BROKEN;
      }
   }
   else
   {
      rcc = ERR_NOT_CONNECTED;
   }

   return rcc;
}

/**
 * Get table
 */
UINT32 AgentConnection::getTable(const TCHAR *pszParam, Table **table)
{
   NXCPMessage msg(m_nProtocolVersion), *pResponse;
   UINT32 dwRqId, dwRetCode;

	*table = NULL;
   if (m_isConnected)
   {
      dwRqId = generateRequestId();
      msg.setCode(CMD_GET_TABLE);
      msg.setId(dwRqId);
      msg.setField(VID_PARAMETER, pszParam);
      if (sendMessage(&msg))
      {
         pResponse = waitForMessage(CMD_REQUEST_COMPLETED, dwRqId, m_dwCommandTimeout);
         if (pResponse != NULL)
         {
            dwRetCode = pResponse->getFieldAsUInt32(VID_RCC);
            if (dwRetCode == ERR_SUCCESS)
            {
					*table = new Table(pResponse);
            }
            delete pResponse;
         }
         else
         {
            dwRetCode = ERR_REQUEST_TIMEOUT;
         }
      }
      else
      {
         dwRetCode = ERR_CONNECTION_BROKEN;
      }
   }
   else
   {
      dwRetCode = ERR_NOT_CONNECTED;
   }

   return dwRetCode;
}

/**
 * Authenticate to agent
 */
UINT32 AgentConnection::authenticate(BOOL bProxyData)
{
   NXCPMessage msg(m_nProtocolVersion);
   UINT32 dwRqId;
   BYTE hash[32];
   int iAuthMethod = bProxyData ? m_iProxyAuth : m_iAuthMethod;
   const char *pszSecret = bProxyData ? m_szProxySecret : m_szSecret;
#ifdef UNICODE
   WCHAR szBuffer[MAX_SECRET_LENGTH];
#endif

   if (iAuthMethod == AUTH_NONE)
      return ERR_SUCCESS;  // No authentication required

   dwRqId = generateRequestId();
   msg.setCode(CMD_AUTHENTICATE);
   msg.setId(dwRqId);
   msg.setField(VID_AUTH_METHOD, (WORD)iAuthMethod);
   switch(iAuthMethod)
   {
      case AUTH_PLAINTEXT:
#ifdef UNICODE
         MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pszSecret, -1, szBuffer, MAX_SECRET_LENGTH);
         msg.setField(VID_SHARED_SECRET, szBuffer);
#else
         msg.setField(VID_SHARED_SECRET, pszSecret);
#endif
         break;
      case AUTH_MD5_HASH:
         CalculateMD5Hash((BYTE *)pszSecret, (int)strlen(pszSecret), hash);
         msg.setField(VID_SHARED_SECRET, hash, MD5_DIGEST_SIZE);
         break;
      case AUTH_SHA1_HASH:
         CalculateSHA1Hash((BYTE *)pszSecret, (int)strlen(pszSecret), hash);
         msg.setField(VID_SHARED_SECRET, hash, SHA1_DIGEST_SIZE);
         break;
      default:
         break;
   }
   if (sendMessage(&msg))
      return waitForRCC(dwRqId, m_dwCommandTimeout);
   else
      return ERR_CONNECTION_BROKEN;
}

/**
 * Execute action on agent
 */
UINT32 AgentConnection::execAction(const TCHAR *action, int argc, const TCHAR * const *argv,
                                   bool withOutput, void (* outputCallback)(ActionCallbackEvent, const TCHAR *, void *), void *cbData)
{
   NXCPMessage msg(m_nProtocolVersion);
   UINT32 dwRqId;
   int i;

   if (!m_isConnected)
      return ERR_NOT_CONNECTED;

   dwRqId = generateRequestId();
   msg.setCode(CMD_ACTION);
   msg.setId(dwRqId);
   msg.setField(VID_ACTION_NAME, action);
   msg.setField(VID_RECEIVE_OUTPUT, (UINT16)(withOutput ? 1 : 0));
   msg.setField(VID_NUM_ARGS, (UINT32)argc);
   for(i = 0; i < argc; i++)
      msg.setField(VID_ACTION_ARG_BASE + i, argv[i]);

   if (sendMessage(&msg))
   {
      if (withOutput)
      {
         UINT32 rcc = waitForRCC(dwRqId, m_dwCommandTimeout);
         if (rcc == ERR_SUCCESS)
         {
            outputCallback(ACE_CONNECTED, NULL, cbData);    // Indicate successful start
            bool eos = false;
            while(!eos)
            {
               NXCPMessage *response = waitForMessage(CMD_COMMAND_OUTPUT, dwRqId, m_dwCommandTimeout * 10);
               if (response != NULL)
               {
                  eos = response->isEndOfSequence();
                  if (response->isFieldExist(VID_MESSAGE))
                  {
                     TCHAR line[4096];
                     response->getFieldAsString(VID_MESSAGE, line, 4096);
                     outputCallback(ACE_DATA, line, cbData);
                  }
                  delete response;
               }
               else
               {
                  return ERR_REQUEST_TIMEOUT;
               }
            }
            outputCallback(ACE_DISCONNECTED, NULL, cbData);
            return ERR_SUCCESS;
         }
         else
         {
            return rcc;
         }
      }
      else
      {
         return waitForRCC(dwRqId, m_dwCommandTimeout);
      }
   }
   else
   {
      return ERR_CONNECTION_BROKEN;
   }
}

/**
 * Upload file to agent
 */
UINT32 AgentConnection::uploadFile(const TCHAR *localFile, const TCHAR *destinationFile, void (* progressCallback)(INT64, void *), void *cbArg, NXCPStreamCompressionMethod compMethod)
{
   UINT32 dwRqId, dwResult;
   NXCPMessage msg(m_nProtocolVersion);

   // Disable compression if it is disabled on connection level or if agent do not support it
   if (!m_allowCompression || (m_nProtocolVersion < 4))
      compMethod = NXCP_STREAM_COMPRESSION_NONE;

   if (!m_isConnected)
      return ERR_NOT_CONNECTED;

   dwRqId = generateRequestId();
   msg.setId(dwRqId);

   time_t lastModTime = 0;
   NX_STAT_STRUCT st;
   if (CALL_STAT(localFile, &st) == 0)
   {
      lastModTime = st.st_mtime;
   }

   // Use core agent if destination file name is not set and file manager subagent otherwise
   if ((destinationFile == NULL) || (*destinationFile == 0))
   {
      msg.setCode(CMD_TRANSFER_FILE);
      int i;
      for(i = (int)_tcslen(localFile) - 1;
          (i >= 0) && (localFile[i] != '\\') && (localFile[i] != '/'); i--);
      msg.setField(VID_FILE_NAME, &localFile[i + 1]);
   }
   else
   {
      msg.setCode(CMD_FILEMGR_UPLOAD);
      msg.setField(VID_OVERWRITE, true);
		msg.setField(VID_FILE_NAME, destinationFile);
   }
   msg.setFieldFromTime(VID_MODIFICATION_TIME, lastModTime);

   if (sendMessage(&msg))
   {
      dwResult = waitForRCC(dwRqId, m_dwCommandTimeout);
   }
   else
   {
      dwResult = ERR_CONNECTION_BROKEN;
   }

   if (dwResult == ERR_SUCCESS)
   {
      AbstractCommChannel *channel = acquireChannel();
      if (channel != NULL)
      {
         debugPrintf(5, _T("Sending file \"%s\" to agent %s compression"),
                  localFile, (compMethod == NXCP_STREAM_COMPRESSION_NONE) ? _T("without") : _T("with"));
         m_fileUploadInProgress = true;
         NXCPEncryptionContext *ctx = acquireEncryptionContext();
         if (SendFileOverNXCP(channel, dwRqId, localFile, ctx, 0, progressCallback, cbArg, m_mutexSocketWrite, compMethod))
            dwResult = waitForRCC(dwRqId, m_dwCommandTimeout);
         else
            dwResult = ERR_IO_FAILURE;
         m_fileUploadInProgress = false;
         if (ctx != NULL)
            ctx->decRefCount();
         channel->decRefCount();
      }
      else
      {
         dwResult = ERR_CONNECTION_BROKEN;
      }
   }

   return dwResult;
}

/**
 * Send upgrade command
 */
UINT32 AgentConnection::startUpgrade(const TCHAR *pszPkgName)
{
   UINT32 dwRqId, dwResult;
   NXCPMessage msg(m_nProtocolVersion);
   int i;

   if (!m_isConnected)
      return ERR_NOT_CONNECTED;

   dwRqId = generateRequestId();

   msg.setCode(CMD_UPGRADE_AGENT);
   msg.setId(dwRqId);
   for(i = (int)_tcslen(pszPkgName) - 1;
       (i >= 0) && (pszPkgName[i] != '\\') && (pszPkgName[i] != '/'); i--);
   msg.setField(VID_FILE_NAME, &pszPkgName[i + 1]);

   if (sendMessage(&msg))
   {
      dwResult = waitForRCC(dwRqId, m_dwCommandTimeout);
   }
   else
   {
      dwResult = ERR_CONNECTION_BROKEN;
   }

   return dwResult;
}

/**
 * Check status of network service via agent
 */
UINT32 AgentConnection::checkNetworkService(UINT32 *pdwStatus, const InetAddress& addr, int iServiceType,
                                            WORD wPort, WORD wProto, const TCHAR *pszRequest,
                                            const TCHAR *pszResponse, UINT32 *responseTime)
{
   UINT32 dwRqId, dwResult;
   NXCPMessage msg(m_nProtocolVersion), *pResponse;
   static WORD m_wDefaultPort[] = { 7, 22, 110, 25, 21, 80, 443, 23 };

   if (!m_isConnected)
      return ERR_NOT_CONNECTED;

   dwRqId = generateRequestId();

   msg.setCode(CMD_CHECK_NETWORK_SERVICE);
   msg.setId(dwRqId);
   msg.setField(VID_IP_ADDRESS, addr);
   msg.setField(VID_SERVICE_TYPE, (WORD)iServiceType);
   msg.setField(VID_IP_PORT,
      (wPort != 0) ? wPort :
         m_wDefaultPort[((iServiceType >= NETSRV_CUSTOM) &&
                         (iServiceType <= NETSRV_TELNET)) ? iServiceType : 0]);
   msg.setField(VID_IP_PROTO, (wProto != 0) ? wProto : (WORD)IPPROTO_TCP);
   msg.setField(VID_SERVICE_REQUEST, pszRequest);
   msg.setField(VID_SERVICE_RESPONSE, pszResponse);

   if (sendMessage(&msg))
   {
      // Wait up to 90 seconds for results
      pResponse = waitForMessage(CMD_REQUEST_COMPLETED, dwRqId, 90000);
      if (pResponse != NULL)
      {
         dwResult = pResponse->getFieldAsUInt32(VID_RCC);
         if (dwResult == ERR_SUCCESS)
         {
            *pdwStatus = pResponse->getFieldAsUInt32(VID_SERVICE_STATUS);
            if (responseTime != NULL)
            {
               *responseTime = pResponse->getFieldAsUInt32(VID_RESPONSE_TIME);
            }
         }
         delete pResponse;
      }
      else
      {
         dwResult = ERR_REQUEST_TIMEOUT;
      }
   }
   else
   {
      dwResult = ERR_CONNECTION_BROKEN;
   }

   return dwResult;
}

/**
 * Get list of supported parameters from agent
 */
UINT32 AgentConnection::getSupportedParameters(ObjectArray<AgentParameterDefinition> **paramList, ObjectArray<AgentTableDefinition> **tableList)
{
   UINT32 dwRqId, dwResult;
   NXCPMessage msg(m_nProtocolVersion), *pResponse;

   *paramList = NULL;
	*tableList = NULL;

   if (!m_isConnected)
      return ERR_NOT_CONNECTED;

   dwRqId = generateRequestId();

   msg.setCode(CMD_GET_PARAMETER_LIST);
   msg.setId(dwRqId);

   if (sendMessage(&msg))
   {
      pResponse = waitForMessage(CMD_REQUEST_COMPLETED, dwRqId, m_dwCommandTimeout);
      if (pResponse != NULL)
      {
         dwResult = pResponse->getFieldAsUInt32(VID_RCC);
			DbgPrintf(6, _T("AgentConnection::getSupportedParameters(): RCC=%d"), dwResult);
         if (dwResult == ERR_SUCCESS)
         {
            UINT32 count = pResponse->getFieldAsUInt32(VID_NUM_PARAMETERS);
            ObjectArray<AgentParameterDefinition> *plist = new ObjectArray<AgentParameterDefinition>(count, 16, true);
            for(UINT32 i = 0, id = VID_PARAM_LIST_BASE; i < count; i++)
            {
               plist->add(new AgentParameterDefinition(pResponse, id));
               id += 3;
            }
				*paramList = plist;
				DbgPrintf(6, _T("AgentConnection::getSupportedParameters(): %d parameters received from agent"), count);

            count = pResponse->getFieldAsUInt32(VID_NUM_TABLES);
            ObjectArray<AgentTableDefinition> *tlist = new ObjectArray<AgentTableDefinition>(count, 16, true);
            for(UINT32 i = 0, id = VID_TABLE_LIST_BASE; i < count; i++)
            {
               tlist->add(new AgentTableDefinition(pResponse, id));
               id += 3;
            }
				*tableList = tlist;
				DbgPrintf(6, _T("AgentConnection::getSupportedParameters(): %d tables received from agent"), count);
			}
         delete pResponse;
      }
      else
      {
         dwResult = ERR_REQUEST_TIMEOUT;
      }
   }
   else
   {
      dwResult = ERR_CONNECTION_BROKEN;
   }

   return dwResult;
}

/**
 * Setup encryption
 */
UINT32 AgentConnection::setupEncryption(RSA *pServerKey)
{
#ifdef _WITH_ENCRYPTION
   NXCPMessage msg(m_nProtocolVersion), *pResp;
   UINT32 dwRqId, dwError, dwResult;

   dwRqId = generateRequestId();

   PrepareKeyRequestMsg(&msg, pServerKey, false);
   msg.setId(dwRqId);
   if (sendMessage(&msg))
   {
      pResp = waitForMessage(CMD_SESSION_KEY, dwRqId, m_dwCommandTimeout);
      if (pResp != NULL)
      {
         dwResult = SetupEncryptionContext(pResp, &m_pCtx, NULL, pServerKey, m_nProtocolVersion);
         switch(dwResult)
         {
            case RCC_SUCCESS:
               dwError = ERR_SUCCESS;
               break;
            case RCC_NO_CIPHERS:
               dwError = ERR_NO_CIPHERS;
               break;
            case RCC_INVALID_PUBLIC_KEY:
               dwError = ERR_INVALID_PUBLIC_KEY;
               break;
            case RCC_INVALID_SESSION_KEY:
               dwError = ERR_INVALID_SESSION_KEY;
               break;
            default:
               dwError = ERR_INTERNAL_ERROR;
               break;
         }
			delete pResp;
      }
      else
      {
         dwError = ERR_REQUEST_TIMEOUT;
      }
   }
   else
   {
      dwError = ERR_CONNECTION_BROKEN;
   }

   return dwError;
#else
   return ERR_NOT_IMPLEMENTED;
#endif
}

/**
 * Get configuration file from agent
 */
UINT32 AgentConnection::getConfigFile(TCHAR **ppszConfig, UINT32 *pdwSize)
{
   *ppszConfig = NULL;
   *pdwSize = 0;

   if (!m_isConnected)
      return ERR_NOT_CONNECTED;

   UINT32 dwResult;
   UINT32 dwRqId = generateRequestId();

   NXCPMessage msg(m_nProtocolVersion);
   msg.setCode(CMD_GET_AGENT_CONFIG);
   msg.setId(dwRqId);

   if (sendMessage(&msg))
   {
      NXCPMessage *pResponse = waitForMessage(CMD_REQUEST_COMPLETED, dwRqId, m_dwCommandTimeout);
      if (pResponse != NULL)
      {
         dwResult = pResponse->getFieldAsUInt32(VID_RCC);
         if (dwResult == ERR_SUCCESS)
         {
            size_t size = pResponse->getFieldAsBinary(VID_CONFIG_FILE, NULL, 0);
            BYTE *utf8Text = (BYTE *)malloc(size + 1);
            pResponse->getFieldAsBinary(VID_CONFIG_FILE, (BYTE *)utf8Text, size);

            // We expect text file, so replace all non-printable characters with spaces
            for(size_t i = 0; i < size; i++)
               if ((utf8Text[i] < ' ') &&
                   (utf8Text[i] != '\t') &&
                   (utf8Text[i] != '\r') &&
                   (utf8Text[i] != '\n'))
                  utf8Text[i] = ' ';
            utf8Text[size] = 0;

#ifdef UNICODE
            *ppszConfig = WideStringFromUTF8String((char *)utf8Text);
#else
            *ppszConfig = MBStringFromUTF8String((char *)utf8Text);
#endif
            free(utf8Text);
            *pdwSize = (UINT32)_tcslen(*ppszConfig);
         }
         delete pResponse;
      }
      else
      {
         dwResult = ERR_REQUEST_TIMEOUT;
      }
   }
   else
   {
      dwResult = ERR_CONNECTION_BROKEN;
   }

   return dwResult;
}

/**
 * Update configuration file on agent
 */
UINT32 AgentConnection::updateConfigFile(const TCHAR *pszConfig)
{
   UINT32 dwRqId, dwResult;
   NXCPMessage msg(m_nProtocolVersion);
#ifdef UNICODE
   int nChars;
   BYTE *pBuffer;
#endif

   if (!m_isConnected)
      return ERR_NOT_CONNECTED;

   dwRqId = generateRequestId();

   msg.setCode(CMD_UPDATE_AGENT_CONFIG);
   msg.setId(dwRqId);
#ifdef UNICODE
   nChars = (int)_tcslen(pszConfig);
   pBuffer = (BYTE *)malloc(nChars + 1);
   WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR,
                       pszConfig, nChars, (char *)pBuffer, nChars + 1, NULL, NULL);
   msg.setField(VID_CONFIG_FILE, pBuffer, nChars);
   free(pBuffer);
#else
   msg.setField(VID_CONFIG_FILE, (BYTE *)pszConfig, (UINT32)strlen(pszConfig));
#endif

   if (sendMessage(&msg))
   {
      dwResult = waitForRCC(dwRqId, m_dwCommandTimeout);
   }
   else
   {
      dwResult = ERR_CONNECTION_BROKEN;
   }

   return dwResult;
}

/**
 * Get routing table from agent
 */
ROUTING_TABLE *AgentConnection::getRoutingTable()
{
   StringList *data;
   if (getList(_T("Net.IP.RoutingTable"), &data) != ERR_SUCCESS)
      return NULL;

   ROUTING_TABLE *pRT = (ROUTING_TABLE *)malloc(sizeof(ROUTING_TABLE));
   pRT->iNumEntries = data->size();
   pRT->pRoutes = (ROUTE *)calloc(data->size(), sizeof(ROUTE));
   for(int i = 0; i < data->size(); i++)
   {
      TCHAR *line = _tcsdup(data->get(i));
      TCHAR *pBuf = line;

      // Destination address and mask
      TCHAR *pChar = _tcschr(pBuf, _T(' '));
      if (pChar != NULL)
      {
         TCHAR *pSlash;
         static TCHAR defaultMask[] = _T("24");

         *pChar = 0;
         pSlash = _tcschr(pBuf, _T('/'));
         if (pSlash != NULL)
         {
            *pSlash = 0;
            pSlash++;
         }
         else     // Just a paranoia protection, should'n happen if agent working correctly
         {
            pSlash = defaultMask;
         }
         pRT->pRoutes[i].dwDestAddr = ntohl(_t_inet_addr(pBuf));
         UINT32 dwBits = _tcstoul(pSlash, NULL, 10);
         pRT->pRoutes[i].dwDestMask = (dwBits == 32) ? 0xFFFFFFFF : (~(0xFFFFFFFF >> dwBits));
         pBuf = pChar + 1;
      }

      // Next hop address
      pChar = _tcschr(pBuf, _T(' '));
      if (pChar != NULL)
      {
         *pChar = 0;
         pRT->pRoutes[i].dwNextHop = ntohl(_t_inet_addr(pBuf));
         pBuf = pChar + 1;
      }

      // Interface index
      pChar = _tcschr(pBuf, ' ');
      if (pChar != NULL)
      {
         *pChar = 0;
         pRT->pRoutes[i].dwIfIndex = _tcstoul(pBuf, NULL, 10);
         pBuf = pChar + 1;
      }

      // Route type
      pRT->pRoutes[i].dwRouteType = _tcstoul(pBuf, NULL, 10);

      free(line);
   }

   delete data;
   return pRT;
}

/**
 * Set proxy information
 */
void AgentConnection::setProxy(InetAddress addr, WORD wPort, int iAuthMethod, const TCHAR *pszSecret)
{
   m_proxyAddr = addr;
   m_wProxyPort = wPort;
   m_iProxyAuth = iAuthMethod;
   if (pszSecret != NULL)
   {
#ifdef UNICODE
      WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR,
                          pszSecret, -1, m_szProxySecret, MAX_SECRET_LENGTH, NULL, NULL);
#else
      nx_strncpy(m_szProxySecret, pszSecret, MAX_SECRET_LENGTH);
#endif
   }
   else
   {
      m_szProxySecret[0] = 0;
   }
   m_useProxy = true;
}

/**
 * Setup proxy connection
 */
UINT32 AgentConnection::setupProxyConnection()
{
   NXCPMessage msg(m_nProtocolVersion);
   UINT32 dwRqId;

   dwRqId = generateRequestId();
   msg.setCode(CMD_SETUP_PROXY_CONNECTION);
   msg.setId(dwRqId);
   msg.setField(VID_IP_ADDRESS, m_addr.getAddressV4());  // For compatibility with agents < 2.2.7
   msg.setField(VID_DESTINATION_ADDRESS, m_addr);
   msg.setField(VID_AGENT_PORT, m_wPort);
   if (sendMessage(&msg))
      return waitForRCC(dwRqId, 60000);   // Wait 60 seconds for remote connect
   else
      return ERR_CONNECTION_BROKEN;
}

/**
 * Enable trap receiving on connection
 */
UINT32 AgentConnection::enableTraps()
{
   NXCPMessage msg(m_nProtocolVersion);
   UINT32 dwRqId;

   dwRqId = generateRequestId();
   msg.setCode(CMD_ENABLE_AGENT_TRAPS);
   msg.setId(dwRqId);
   if (sendMessage(&msg))
      return waitForRCC(dwRqId, m_dwCommandTimeout);
   else
      return ERR_CONNECTION_BROKEN;
}

/**
 * Enable trap receiving on connection
 */
UINT32 AgentConnection::enableFileUpdates()
{
   NXCPMessage msg(m_nProtocolVersion);
   UINT32 dwRqId;

   dwRqId = generateRequestId();
   msg.setCode(CMD_ENABLE_FILE_UPDATES);
   msg.setId(dwRqId);
   if (sendMessage(&msg))
   {
      return waitForRCC(dwRqId, m_dwCommandTimeout);
   }
   else
      return ERR_CONNECTION_BROKEN;
}

/**
 * Take screenshot from remote system
 */
UINT32 AgentConnection::takeScreenshot(const TCHAR *sessionName, BYTE **data, size_t *size)
{
   NXCPMessage msg(m_nProtocolVersion);
   UINT32 dwRqId;

   dwRqId = generateRequestId();
   msg.setCode(CMD_TAKE_SCREENSHOT);
   msg.setId(dwRqId);
   msg.setField(VID_NAME, sessionName);
   if (sendMessage(&msg))
   {
      NXCPMessage *response = waitForMessage(CMD_REQUEST_COMPLETED, dwRqId, m_dwCommandTimeout);
      if (response != NULL)
      {
         UINT32 rcc = response->getFieldAsUInt32(VID_RCC);
         if (rcc == ERR_SUCCESS)
         {
            const BYTE *p = response->getBinaryFieldPtr(VID_FILE_DATA, size);
            if (p != NULL)
            {
               *data = (BYTE *)malloc(*size);
               memcpy(*data, p, *size);
            }
            else
            {
               *data = NULL;
            }
         }
         delete response;
         return rcc;
      }
      else
      {
         return ERR_REQUEST_TIMEOUT;
      }
   }
   else
   {
      return ERR_CONNECTION_BROKEN;
   }
}

/**
 * Resolve hostname by IP address in local network
 */
TCHAR *AgentConnection::getHostByAddr(const InetAddress& ipAddr, TCHAR *buf, size_t bufLen)
{
   NXCPMessage msg(m_nProtocolVersion);
   UINT32 dwRqId;

   dwRqId = generateRequestId();
   msg.setCode(CMD_HOST_BY_IP);
   msg.setId(dwRqId);
   msg.setField(VID_IP_ADDRESS, ipAddr);
   TCHAR *result = NULL;
   if (sendMessage(&msg))
   {
      NXCPMessage *response = waitForMessage(CMD_REQUEST_COMPLETED, dwRqId, m_dwCommandTimeout);
      if (response != NULL)
      {
         UINT32 rcc = response->getFieldAsUInt32(VID_RCC);
         if (rcc == ERR_SUCCESS)
         {
            result = response->getFieldAsString(VID_NAME, buf, bufLen);
         }
         delete response;
         return result;
      }
      else
      {
         return result;
      }
   }
   else
   {
      return result;
   }
}

/**
 * Send custom request to agent
 */
NXCPMessage *AgentConnection::customRequest(NXCPMessage *pRequest, const TCHAR *recvFile, bool append,
         void (*downloadProgressCallback)(size_t, void *), void (*fileResendCallback)(NXCP_MESSAGE *, void *), void *cbArg)
{
   UINT32 dwRqId, rcc;
	NXCPMessage *msg = NULL;

   dwRqId = generateRequestId();
   pRequest->setId(dwRqId);
	if (recvFile != NULL)
	{
		rcc = prepareFileDownload(recvFile, dwRqId, append, downloadProgressCallback, fileResendCallback,cbArg);
		if (rcc != ERR_SUCCESS)
		{
			// Create fake response message
			msg = new NXCPMessage;
			msg->setCode(CMD_REQUEST_COMPLETED);
			msg->setId(dwRqId);
			msg->setField(VID_RCC, rcc);
		}
	}

	if (msg == NULL)
	{
		if (sendMessage(pRequest))
		{
			msg = waitForMessage(CMD_REQUEST_COMPLETED, dwRqId, m_dwCommandTimeout);
			if ((msg != NULL) && (recvFile != NULL))
			{
				if (msg->getFieldAsUInt32(VID_RCC) == ERR_SUCCESS)
				{
					if (ConditionWait(m_condFileDownload, 1800000))	 // 30 min timeout
					{
						if (!m_fileDownloadSucceeded)
						{
							msg->setField(VID_RCC, ERR_IO_FAILURE);
							if (m_deleteFileOnDownloadFailure)
								_tremove(recvFile);
						}
					}
					else
					{
						msg->setField(VID_RCC, ERR_REQUEST_TIMEOUT);
					}
				}
				else
				{
               if (fileResendCallback != NULL)
               {
                  _close(m_hCurrFile);
                  m_hCurrFile = -1;
                  _tremove(recvFile);
               }
				}
			}

		}
	}

	return msg;
}

/**
 * Cancel file download
 */
UINT32 AgentConnection::cancelFileDownload()
{
   NXCPMessage msg(CMD_CANCEL_FILE_DOWNLOAD, generateRequestId(), getProtocolVersion());
   msg.setField(VID_REQUEST_ID, m_dwDownloadRequestId);

   UINT32 rcc;
   if (sendMessage(&msg))
   {
      NXCPMessage *result = waitForMessage(CMD_REQUEST_COMPLETED, msg.getId(), m_dwCommandTimeout);
      if (result != NULL)
      {
         rcc = result->getFieldAsUInt32(VID_RCC);
         delete result;
      }
      else
      {
         rcc = ERR_REQUEST_TIMEOUT;
      }
   }
   else
   {
      rcc = ERR_CONNECTION_BROKEN;
   }
   return rcc;
}

/**
 * Prepare for file download
 */
UINT32 AgentConnection::prepareFileDownload(const TCHAR *fileName, UINT32 rqId, bool append, void (*downloadProgressCallback)(size_t, void *),
                                             void (* fileResendCallback)(NXCP_MESSAGE *, void *), void *cbArg)
{
   if (fileResendCallback == NULL)
   {
      if (m_hCurrFile != -1)
         return ERR_RESOURCE_BUSY;

      nx_strncpy(m_currentFileName, fileName, MAX_PATH);
      ConditionReset(m_condFileDownload);
      m_hCurrFile = _topen(fileName, (append ? 0 : (O_CREAT | O_TRUNC)) | O_RDWR | O_BINARY, S_IREAD | S_IWRITE);
      if (m_hCurrFile == -1)
      {
         DbgPrintf(4, _T("AgentConnection::PrepareFileDownload(): cannot open file %s (%s); append=%d rqId=%d"),
                   fileName, _tcserror(errno), append, rqId);
      }
      else
      {
         if (append)
            lseek(m_hCurrFile, 0, SEEK_END);
      }

      m_dwDownloadRequestId = rqId;
      m_downloadProgressCallback = downloadProgressCallback;
      m_downloadProgressCallbackArg = cbArg;

      m_sendToClientMessageCallback = NULL;

      return (m_hCurrFile != -1) ? ERR_SUCCESS : ERR_FILE_OPEN_ERROR;
   }
   else
   {
      ConditionReset(m_condFileDownload);

      m_dwDownloadRequestId = rqId;
      m_downloadProgressCallback = downloadProgressCallback;
      m_downloadProgressCallbackArg = cbArg;

      m_sendToClientMessageCallback = fileResendCallback;

      return ERR_SUCCESS;
   }
}

/**
 * File upload completion handler
 */
void AgentConnection::onFileDownload(bool success)
{
   if (!success && m_deleteFileOnDownloadFailure)
		_tremove(m_currentFileName);
	m_fileDownloadSucceeded = success;
	ConditionSet(m_condFileDownload);
}

/**
 * Enable trap receiving on connection
 */
UINT32 AgentConnection::getPolicyInventory(AgentPolicyInfo **info)
{
   NXCPMessage msg(m_nProtocolVersion);
   UINT32 dwRqId, rcc;

	*info = NULL;
   dwRqId = generateRequestId();
   msg.setCode(CMD_GET_POLICY_INVENTORY);
   msg.setId(dwRqId);
   if (sendMessage(&msg))
	{
		NXCPMessage *response = waitForMessage(CMD_REQUEST_COMPLETED, dwRqId, m_dwCommandTimeout);
		if (response != NULL)
		{
			rcc = response->getFieldAsUInt32(VID_RCC);
			if (rcc == ERR_SUCCESS)
				*info = new AgentPolicyInfo(response);
			delete response;
		}
		else
		{
	      rcc = ERR_REQUEST_TIMEOUT;
		}
	}
   else
	{
      rcc = ERR_CONNECTION_BROKEN;
	}
	return rcc;
}

/**
 * Uninstall policy by GUID
 */
UINT32 AgentConnection::uninstallPolicy(const uuid& guid)
{
	UINT32 rqId, rcc;
	NXCPMessage msg(m_nProtocolVersion);

   rqId = generateRequestId();
   msg.setId(rqId);
	msg.setCode(CMD_UNINSTALL_AGENT_POLICY);
	msg.setField(VID_GUID, guid);
	if (sendMessage(&msg))
	{
		rcc = waitForRCC(rqId, m_dwCommandTimeout);
	}
	else
	{
		rcc = ERR_CONNECTION_BROKEN;
	}
   return rcc;
}

/**
 * Acquire encryption context
 */
NXCPEncryptionContext *AgentConnection::acquireEncryptionContext()
{
	lock();
	NXCPEncryptionContext *ctx = m_pCtx;
	if (ctx != NULL)
		ctx->incRefCount();
	unlock();
	return ctx;
}

/**
 * Callback for processing collected data on separate thread
 */
void AgentConnection::processCollectedDataCallback(NXCPMessage *msg)
{
   NXCPMessage response;
   response.setCode(CMD_REQUEST_COMPLETED);
   response.setId(msg->getId());

   if (msg->getFieldAsBoolean(VID_BULK_RECONCILIATION))
   {
      UINT32 rcc = processBulkCollectedData(msg, &response);
      response.setField(VID_RCC, rcc);
   }
   else
   {
      UINT32 rcc = processCollectedData(msg);
      response.setField(VID_RCC, rcc);
   }

   sendMessage(&response);
   delete msg;
   decInternalRefCount();
}

/**
 * Process collected data information (for DCI with agent-side cache)
 */
UINT32 AgentConnection::processCollectedData(NXCPMessage *msg)
{
   return ERR_NOT_IMPLEMENTED;
}

/**
 * Process collected data information in bulk mode (for DCI with agent-side cache)
 */
UINT32 AgentConnection::processBulkCollectedData(NXCPMessage *request, NXCPMessage *response)
{
   return ERR_NOT_IMPLEMENTED;
}

/**
 * Setup TCP proxy
 */
UINT32 AgentConnection::setupTcpProxy(const InetAddress& ipAddr, UINT16 port, UINT32 *channelId)
{
   UINT32 requestId = generateRequestId();
   NXCPMessage msg(CMD_SETUP_TCP_PROXY, requestId, m_nProtocolVersion);
   msg.setField(VID_IP_ADDRESS, ipAddr);
   msg.setField(VID_PORT, port);

   UINT32 rcc;
   if (sendMessage(&msg))
   {
      NXCPMessage *response = waitForMessage(CMD_REQUEST_COMPLETED, requestId, m_dwCommandTimeout);
      if (response != NULL)
      {
         rcc = response->getFieldAsUInt32(VID_RCC);
         if (rcc == ERR_SUCCESS)
         {
            *channelId = response->getFieldAsUInt32(VID_CHANNEL_ID);
         }
         delete response;
      }
      else
      {
         rcc = ERR_REQUEST_TIMEOUT;
      }
   }
   else
   {
      rcc = ERR_CONNECTION_BROKEN;
   }
   return rcc;
}

/**
 * Close TCP proxy
 */
UINT32 AgentConnection::closeTcpProxy(UINT32 channelId)
{
   UINT32 requestId = generateRequestId();
   NXCPMessage msg(CMD_CLOSE_TCP_PROXY, requestId, m_nProtocolVersion);
   msg.setField(VID_CHANNEL_ID, channelId);
   UINT32 rcc;
   if (sendMessage(&msg))
   {
      NXCPMessage *response = waitForMessage(CMD_REQUEST_COMPLETED, requestId, m_dwCommandTimeout);
      if (response != NULL)
      {
         rcc = response->getFieldAsUInt32(VID_RCC);
         delete response;
      }
      else
      {
         rcc = ERR_REQUEST_TIMEOUT;
      }
   }
   else
   {
      rcc = ERR_CONNECTION_BROKEN;
   }
   return rcc;
}

/**
 * Process data received from TCP proxy
 */
void AgentConnection::processTcpProxyData(UINT32 channelId, const void *data, size_t size)
{
}

/**
 * Create new agent parameter definition from NXCP message
 */
AgentParameterDefinition::AgentParameterDefinition(NXCPMessage *msg, UINT32 baseId)
{
   m_name = msg->getFieldAsString(baseId);
   m_description = msg->getFieldAsString(baseId + 1);
   m_dataType = (int)msg->getFieldAsUInt16(baseId + 2);
}

/**
 * Create new agent parameter definition from another definition object
 */
AgentParameterDefinition::AgentParameterDefinition(AgentParameterDefinition *src)
{
   m_name = _tcsdup_ex(src->m_name);
   m_description = _tcsdup_ex(src->m_description);
   m_dataType = src->m_dataType;
}

/**
 * Create new agent parameter definition from scratch
 */
AgentParameterDefinition::AgentParameterDefinition(const TCHAR *name, const TCHAR *description, int dataType)
{
   m_name = _tcsdup_ex(name);
   m_description = _tcsdup_ex(description);
   m_dataType = dataType;
}

/**
 * Destructor for agent parameter definition
 */
AgentParameterDefinition::~AgentParameterDefinition()
{
   safe_free(m_name);
   safe_free(m_description);
}

/**
 * Fill NXCP message
 */
UINT32 AgentParameterDefinition::fillMessage(NXCPMessage *msg, UINT32 baseId)
{
   msg->setField(baseId, m_name);
   msg->setField(baseId + 1, m_description);
   msg->setField(baseId + 2, (WORD)m_dataType);
   return 3;
}

/**
 * Create new agent table definition from NXCP message
 */
AgentTableDefinition::AgentTableDefinition(NXCPMessage *msg, UINT32 baseId)
{
   m_name = msg->getFieldAsString(baseId);
   m_description = msg->getFieldAsString(baseId + 2);

   TCHAR *instanceColumns = msg->getFieldAsString(baseId + 1);
   if (instanceColumns != NULL)
   {
      m_instanceColumns = new StringList(instanceColumns, _T("|"));
      free(instanceColumns);
   }
   else
   {
      m_instanceColumns = new StringList;
   }

   m_columns = new ObjectArray<AgentTableColumnDefinition>(16, 16, true);
}

/**
 * Create new agent table definition from another definition object
 */
AgentTableDefinition::AgentTableDefinition(AgentTableDefinition *src)
{
   m_name = (src->m_name != NULL) ? _tcsdup(src->m_name) : NULL;
   m_description = (src->m_description != NULL) ? _tcsdup(src->m_description) : NULL;
   m_instanceColumns = new StringList(src->m_instanceColumns);
   m_columns = new ObjectArray<AgentTableColumnDefinition>(16, 16, true);
   for(int i = 0; i < src->m_columns->size(); i++)
   {
      m_columns->add(new AgentTableColumnDefinition(src->m_columns->get(i)));
   }
}
/**
 * Destructor for agent table definition
 */
AgentTableDefinition::~AgentTableDefinition()
{
   safe_free(m_name);
   safe_free(m_description);
   delete m_instanceColumns;
   delete m_columns;
}

/**
 * Fill NXCP message
 */
UINT32 AgentTableDefinition::fillMessage(NXCPMessage *msg, UINT32 baseId)
{
   msg->setField(baseId + 1, m_name);
   msg->setField(baseId + 2, m_description);

   TCHAR *instanceColumns = m_instanceColumns->join(_T("|"));
   msg->setField(baseId + 3, instanceColumns);
   free(instanceColumns);

   UINT32 varId = baseId + 4;
   for(int i = 0; i < m_columns->size(); i++)
   {
      msg->setField(varId++, m_columns->get(i)->m_name);
      msg->setField(varId++, (WORD)m_columns->get(i)->m_dataType);
   }

   msg->setField(baseId, varId - baseId);
   return varId - baseId;
}
