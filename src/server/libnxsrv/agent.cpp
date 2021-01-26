/*
** NetXMS - Network Management System
** Server Library
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
** File: agent.cpp
**
**/

#include "libnxsrv.h"
#include <stdarg.h>
#include <nxstat.h>

#ifndef _WIN32
#define _tell(f) lseek((f),0,SEEK_CUR)
#endif

#define DEBUG_TAG    _T("agent.conn")

/**
 * Constants
 */
#define MAX_MSG_SIZE    268435456

/**
 * Agent connection thread pool
 */
LIBNXSRV_EXPORTABLE_VAR(ThreadPool *g_agentConnectionThreadPool) = nullptr;

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
static ObjectArray<BackgroundSocketPollerHandle> s_pollers(64, 64, Ownership::True);
static Mutex s_pollerListLock(true);
static uint32_t s_maxConnectionsPerPoller = 256;
static bool s_shutdownMode = false;

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
 * Set shutdown mode for agent connections
 */
void LIBNXSRV_EXPORTABLE DisableAgentConnections()
{
   s_pollerListLock.lock();
   s_shutdownMode = true;
   s_pollers.clear();
   s_pollerListLock.unlock();
}

/**
 * Agent connection receiver
 */
class AgentConnectionReceiver
{
private:
   weak_ptr<AgentConnection> m_connection;
   weak_ptr<AgentConnectionReceiver> m_self;
   uint32_t m_debugId;
   uint32_t m_recvTimeout;
   AbstractCommChannel *m_channel;
   TCHAR m_threadPoolKey[16];
   bool m_attached;

   void debugPrintf(int level, const TCHAR *format, ...);

   static void channelPollerCallback(BackgroundSocketPollResult pollResult, AbstractCommChannel *channel, const shared_ptr<AgentConnectionReceiver>& receiver);

   bool readChannel();
   MessageReceiverResult readMessage(bool allowChannelRead);
   void finalize();

public:
   shared_ptr<NXCPEncryptionContext> m_encryptionContext;
   CommChannelMessageReceiver *m_messageReceiver;

   static shared_ptr<AgentConnectionReceiver> create(const shared_ptr<AgentConnection>& connection)
   {
      auto receiver = make_shared<AgentConnectionReceiver>(connection);
      receiver->m_self = receiver;
      return receiver;
   }

   AgentConnectionReceiver(const shared_ptr<AgentConnection>& connection) : m_connection(connection)
   {
      m_debugId = connection->m_debugId;
      m_channel = connection->m_channel;
      m_messageReceiver = new CommChannelMessageReceiver(m_channel, 4096, MAX_MSG_SIZE);
      m_channel->incRefCount();
      m_recvTimeout = connection->m_recvTimeout; // 7 minutes
      _sntprintf(m_threadPoolKey, 16, _T("RECV-%u"), m_debugId);
      m_attached = true;
   }

   ~AgentConnectionReceiver()
   {
      debugPrintf(7, _T("AgentConnectionReceiver destructor called (this=%p)"), this);

      delete m_messageReceiver;
      if (m_channel != nullptr)
         m_channel->decRefCount();
   }

   void start();

   void detach()
   {
      m_attached = false;
      m_connection.reset();
   }
};

/**
 * Write debug output in receiver
 */
void AgentConnectionReceiver::debugPrintf(int level, const TCHAR *format, ...)
{
   va_list args;
   va_start(args, format);
   nxlog_debug_tag_object2(DEBUG_TAG, m_debugId, level, format, args);
   va_end(args);
}

/**
 * Callback for channel poller
 */
void AgentConnectionReceiver::channelPollerCallback(BackgroundSocketPollResult pollResult, AbstractCommChannel *channel, const shared_ptr<AgentConnectionReceiver>& receiver)
{
   if (pollResult == BackgroundSocketPollResult::SUCCESS)
   {
      if (!s_shutdownMode && receiver->m_attached && receiver->readChannel())
      {
         channel->backgroundPoll(receiver->m_recvTimeout, channelPollerCallback, receiver);
         return;
      }
   }
   else
   {
      receiver->debugPrintf(5, _T("Channel poll error (%d)"), static_cast<int>(pollResult));
   }
   if (g_agentConnectionThreadPool != nullptr)
      ThreadPoolExecuteSerialized(g_agentConnectionThreadPool, receiver->m_threadPoolKey, receiver, &AgentConnectionReceiver::finalize);
   else
      receiver->finalize();
}

/**
 * Start receiver
 */
void AgentConnectionReceiver::start()
{
   m_channel->backgroundPoll(m_recvTimeout, channelPollerCallback, m_self.lock());
}

/**
 * Read messages from channel
 */
bool AgentConnectionReceiver::readChannel()
{
   MessageReceiverResult result = readMessage(true);
   while(result == MSGRECV_SUCCESS)
      result = readMessage(false);
   return (result == MSGRECV_WANT_READ) || (result == MSGRECV_WANT_WRITE);
}

/**
 * Read single message from channel
 */
MessageReceiverResult AgentConnectionReceiver::readMessage(bool allowChannelRead)
{
   // Receive raw message
   MessageReceiverResult result;
   NXCPMessage *msg = m_messageReceiver->readMessage(0, &result, allowChannelRead);
   if ((result == MSGRECV_WANT_READ) || (result == MSGRECV_WANT_WRITE))
      return result;

   // Check for decryption error
   if (result == MSGRECV_DECRYPTION_FAILURE)
   {
      debugPrintf(6, _T("Unable to decrypt received message"));
      return MSGRECV_SUCCESS; // continue reading
   }

   shared_ptr<AgentConnection> connection = m_connection.lock();
   if (connection == nullptr)
   {
      delete msg;
      return MSGRECV_COMM_FAILURE;   // Parent connection was destroyed
   }

   // Check for timeout
   if (result == MSGRECV_TIMEOUT)
   {
      if (connection->m_fileUploadInProgress)
         return MSGRECV_WANT_READ;   // Receive timeout may occur when uploading large files via slow links
      debugPrintf(6, _T("Timed out waiting for message"));
      return MSGRECV_TIMEOUT;
   }

   // Receive error
   if (msg == nullptr)
   {
      if (result == MSGRECV_CLOSED)
         debugPrintf(6, _T("Communication channel shutdown"));
      else
         debugPrintf(6, _T("Message receiving error (%s)"), AbstractMessageReceiver::resultToText(result));
      return result;
   }

   if (IsShutdownInProgress())
   {
      debugPrintf(6, _T("Process shutdown"));
      delete msg;
      return MSGRECV_COMM_FAILURE;
   }

   if (msg->isBinary())
   {
      if (nxlog_get_debug_level_tag_object(DEBUG_TAG, m_debugId) >= 6)
      {
         TCHAR buffer[64];
         debugPrintf(6, _T("Received raw message %s (%d) from agent at %s"),
            NXCPMessageCodeName(msg->getCode(), buffer), msg->getId(), connection->m_addr.toString().cstr());
      }

      if ((msg->getCode() == CMD_FILE_DATA) && (msg->getId() == connection->m_dwDownloadRequestId))
      {
         if (g_agentConnectionThreadPool != nullptr)
         {
            TCHAR key[64];
            _sntprintf(key, 64, _T("FileTransfer_%p"), this);
            ThreadPoolExecuteSerialized(g_agentConnectionThreadPool, key, connection, &AgentConnection::processFileData, msg);
         }
         else
         {
            connection->processFileData(msg);
         }
         msg = nullptr; // Prevent delete
      }
      else if ((msg->getCode() == CMD_ABORT_FILE_TRANSFER) && (msg->getId() == connection->m_dwDownloadRequestId))
      {
         if (g_agentConnectionThreadPool != nullptr)
         {
            TCHAR key[64];
            _sntprintf(key, 64, _T("FileTransfer_%p"), this);
            ThreadPoolExecuteSerialized(g_agentConnectionThreadPool, key, connection, &AgentConnection::processFileTransferAbort, msg);
         }
         else
         {
            connection->processFileTransferAbort(msg);
         }
         msg = nullptr; // Prevent delete
      }
      else if (msg->getCode() == CMD_TCP_PROXY_DATA)
      {
         connection->processTcpProxyData(msg->getId(), msg->getBinaryData(), msg->getBinaryDataSize());
      }
      delete msg;
   }
   else if (msg->isControl())
   {
      if (nxlog_get_debug_level_tag_object(DEBUG_TAG, m_debugId) >= 6)
      {
         TCHAR buffer[64];
         debugPrintf(6, _T("Received control message %s from agent at %s"),
            NXCPMessageCodeName(msg->getCode(), buffer), connection->m_addr.toString().cstr());
      }
      connection->m_pMsgWaitQueue->put(msg);
   }
   else
   {
      if (nxlog_get_debug_level_tag_object(DEBUG_TAG, m_debugId) >= 6)
      {
         TCHAR buffer[64];
         debugPrintf(6, _T("Received message %s (%d) from agent at %s"),
            NXCPMessageCodeName(msg->getCode(), buffer), msg->getId(), (const TCHAR *)connection->m_addr.toString());
      }
      switch(msg->getCode())
      {
         case CMD_REQUEST_COMPLETED:
         case CMD_SESSION_KEY:
            connection->m_pMsgWaitQueue->put(msg);
            break;
         case CMD_TRAP:
            if (g_agentConnectionThreadPool != nullptr)
            {
               TCHAR key[64];
               _sntprintf(key, 64, _T("EventProc_%p"), this);
               ThreadPoolExecuteSerialized(g_agentConnectionThreadPool, key, connection, &AgentConnection::onTrapCallback, msg);
            }
            else
            {
               delete msg;
            }
            break;
         case CMD_SYSLOG_RECORDS:
            if (g_agentConnectionThreadPool != nullptr)
            {
               TCHAR key[64];
               _sntprintf(key, 64, _T("Syslog_%p"), this);
               ThreadPoolExecuteSerialized(g_agentConnectionThreadPool, key, connection, &AgentConnection::onSyslogMessageCallback, msg);
            }
            else
            {
               delete msg;
            }
            break;
         case CMD_WINDOWS_EVENT:
            if (g_agentConnectionThreadPool != nullptr)
            {
               TCHAR key[64];
               _sntprintf(key, 64, _T("WinEvent_%p"), this);
               ThreadPoolExecuteSerialized(g_agentConnectionThreadPool, key, connection, &AgentConnection::onWindowsEventCallback, msg);
            }
            else
            {
               delete msg;
            }
            break;
         case CMD_PUSH_DCI_DATA:
            if (g_agentConnectionThreadPool != nullptr)
            {
               ThreadPoolExecute(g_agentConnectionThreadPool, connection, &AgentConnection::onDataPushCallback, msg);
            }
            else
            {
               delete msg;
            }
            break;
         case CMD_DCI_DATA:
            if (g_agentConnectionThreadPool != nullptr)
            {
               ThreadPoolExecute(g_agentConnectionThreadPool, connection, &AgentConnection::processCollectedDataCallback, msg);
            }
            else
            {
               NXCPMessage response(CMD_REQUEST_COMPLETED, msg->getId(), connection->m_nProtocolVersion);
               response.setField(VID_RCC, ERR_INTERNAL_ERROR);
               connection->sendMessage(&response);
               delete msg;
            }
            break;
         case CMD_GET_SSH_KEYS:
            if (g_agentConnectionThreadPool != nullptr)
            {
               ThreadPoolExecute(g_agentConnectionThreadPool, connection, &AgentConnection::getSshKeysCallback, msg);
            }
            else
            {
               NXCPMessage response(CMD_REQUEST_COMPLETED, msg->getId(), connection->m_nProtocolVersion);
               response.setField(VID_RCC, ERR_INTERNAL_ERROR);
               connection->sendMessage(&response);
               delete msg;
            }
            break;
         case CMD_FILE_MONITORING:
            connection->onFileMonitoringData(msg);
            delete msg;
            break;
         case CMD_SNMP_TRAP:
            if (g_agentConnectionThreadPool != nullptr)
            {
               TCHAR key[64];
               _sntprintf(key, 64, _T("SNMPTrap_%p"), this);
               ThreadPoolExecuteSerialized(g_agentConnectionThreadPool, key, connection, &AgentConnection::onSnmpTrapCallback, msg);
            }
            else
            {
               delete msg;
            }
            break;
         case CMD_CLOSE_TCP_PROXY:
            connection->processTcpProxyData(msg->getFieldAsUInt32(VID_CHANNEL_ID), nullptr, 0);
            delete msg;
            break;
         default:
            if (connection->processCustomMessage(msg))
               delete msg;
            else
               connection->m_pMsgWaitQueue->put(msg);
            break;
      }
   }
   return MSGRECV_SUCCESS;
}

/**
 * Finalize receiver cleanup
 */
void AgentConnectionReceiver::finalize()
{
   debugPrintf(6, _T("Receiver loop terminated"));

   // Close socket and mark connection as disconnected
   m_channel->close();

   shared_ptr<AgentConnection> connection = m_connection.lock();
   if (connection != nullptr)
   {
      connection->lock();
      if (connection->m_hCurrFile != -1)
      {
         _close(connection->m_hCurrFile);
         connection->m_hCurrFile = -1;
         connection->onFileDownload(false);
      }
      else if (connection->m_sendToClientMessageCallback != nullptr)
      {
         connection->m_sendToClientMessageCallback = nullptr;
         connection->onFileDownload(false);
      }

      debugPrintf(6, _T("Closing communication channel"));
      connection->m_isConnected = false;
      connection->unlock();

      connection->onDisconnect();
   }

   debugPrintf(6, _T("Receiver cleanup completed"));
}

/**
 * Constructor for AgentConnection
 */
AgentConnection::AgentConnection(const InetAddress& addr, uint16_t port, const TCHAR *secret, bool allowCompression)
{
#ifdef _WIN32
   m_self = new weak_ptr<AgentConnection>();
#endif
   m_debugId = InterlockedIncrement(&s_connectionId);
   m_addr = addr;
   m_port = port;
   if ((secret != nullptr) && (*secret != 0))
   {
#ifdef UNICODE
		WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR, secret, -1, m_secret, MAX_SECRET_LENGTH, nullptr, nullptr);
		m_secret[MAX_SECRET_LENGTH - 1] = 0;
#else
      strlcpy(m_secret, secret, MAX_SECRET_LENGTH);
#endif
      DecryptPasswordA("netxms", m_secret, m_secret, MAX_SECRET_LENGTH);
   }
   else
   {
      m_secret[0] = 0;
   }
   m_allowCompression = allowCompression;
   m_channel = nullptr;
   m_tLastCommandTime = 0;
   m_pMsgWaitQueue = new MsgWaitQueue;
   m_requestId = 0;
	m_connectionTimeout = 5000;	// 5 seconds
   m_commandTimeout = 5000;   // Default timeout 5 seconds
   m_recvTimeout = 420000; // 7 minutes
   m_isConnected = false;
   m_mutexDataLock = MutexCreate();
	m_mutexSocketWrite = MutexCreate();
   m_encryptionPolicy = m_iDefaultEncryptionPolicy;
   m_useProxy = false;
   m_proxyPort = 4700;
   m_proxySecret[0] = 0;
   m_nProtocolVersion = NXCP_VERSION;
	m_hCurrFile = -1;
   m_deleteFileOnDownloadFailure = true;
	m_condFileDownload = ConditionCreate(true);
   m_fileDownloadSucceeded = false;
	m_fileUploadInProgress = false;
   m_fileUpdateConnection = false;
   m_sendToClientMessageCallback = nullptr;
   m_dwDownloadRequestId = 0;
   m_downloadProgressCallback = nullptr;
   m_downloadProgressCallbackArg = nullptr;
   m_bulkDataProcessing = 0;
   m_controlServer = false;
   m_masterServer = false;
}

/**
 * Destructor
 */
AgentConnection::~AgentConnection()
{
   debugPrintf(7, _T("AgentConnection destructor called (this=%p)"), this);

   if (m_receiver != nullptr)
      m_receiver->detach();

   delete m_pMsgWaitQueue;

	if (m_hCurrFile != -1)
	{
		_close(m_hCurrFile);
		onFileDownload(false);
	}
   else if (m_sendToClientMessageCallback != nullptr)
   {
      onFileDownload(false);
   }

	if (m_channel != nullptr)
	{
	   m_channel->shutdown();
	   m_channel->decRefCount();
	}

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
 * Create channel. Default implementation creates socket channel.
 */
AbstractCommChannel *AgentConnection::createChannel()
{
   if (s_shutdownMode)
      return nullptr;

   SOCKET s = m_useProxy ?
            ConnectToHost(m_proxyAddr, m_proxyPort, m_connectionTimeout) :
            ConnectToHost(m_addr, m_port, m_connectionTimeout);

   // Connect to server
   if (s == INVALID_SOCKET)
   {
      TCHAR buffer[64];
      debugPrintf(5, _T("Cannot establish connection with agent at %s:%d"),
               m_useProxy ? m_proxyAddr.toString(buffer) : m_addr.toString(buffer),
               (int)(m_useProxy ? m_proxyPort : m_port));
      return nullptr;
   }

   // Select socket poller
   BackgroundSocketPollerHandle *sp = nullptr;
   s_pollerListLock.lock();
   if (s_shutdownMode)
   {
      shutdown(s, SHUT_RDWR);
      closesocket(s);
      s_pollerListLock.unlock();
      return nullptr;
   }
   for(int i = 0; i < s_pollers.size(); i++)
   {
      BackgroundSocketPollerHandle *p = s_pollers.get(i);
      if (static_cast<uint32_t>(InterlockedIncrement(&p->usageCount)) < s_maxConnectionsPerPoller)
      {
         sp = p;
         break;
      }
      InterlockedDecrement(&p->usageCount);
   }
   if (sp == nullptr)
   {
      sp = new BackgroundSocketPollerHandle();
      sp->usageCount = 1;
      s_pollers.add(sp);
   }
   s_pollerListLock.unlock();

   return new SocketCommChannel(s, sp);
}

/**
 * Acquire communication channel. Caller must call decRefCount to release channel.
 */
AbstractCommChannel *AgentConnection::acquireChannel()
{
   lock();
   AbstractCommChannel *channel = m_channel;
   if (channel != nullptr)
      channel->incRefCount();
   unlock();
   return channel;
}

/**
 * Acquire encryption context
 */
shared_ptr<NXCPEncryptionContext> AgentConnection::acquireEncryptionContext()
{
   lock();
   shared_ptr<NXCPEncryptionContext> ctx = m_receiver->m_encryptionContext;
   unlock();
   return ctx;
}

/**
 * Connect to agent
 */
bool AgentConnection::connect(RSA *serverKey, uint32_t *error, uint32_t *socketError, uint64_t serverId)
{
   TCHAR szBuffer[256];
   bool success = false;
   bool forceEncryption = false;
   bool secondPass = false;
   uint32_t dwError = 0;

   if (error != nullptr)
      *error = ERR_INTERNAL_ERROR;

   if (socketError != nullptr)
      *socketError = 0;

   if (s_shutdownMode)
      return false;

   lock();

   // Check if already connected
   if (m_isConnected)
   {
      unlock();
      return false;
   }

   // Wait for receiver thread from previous connection, if any
   if (m_receiver != nullptr)
   {
      m_receiver->detach();
      m_receiver.reset();
   }

   // Check if we need to close existing channel
   if (m_channel != nullptr)
   {
      m_channel->decRefCount();
      m_channel = nullptr;
   }

   unlock();

   auto channel = createChannel();
   if (channel == nullptr)
   {
      debugPrintf(6, _T("Cannot create communication channel"));
      dwError = ERR_CONNECT_FAILED;
      goto connect_cleanup;
   }

   lock();
   m_channel = channel;
   unlock();

   if (!NXCPGetPeerProtocolVersion(m_channel, &m_nProtocolVersion, m_mutexSocketWrite))
   {
      debugPrintf(6, _T("Protocol version negotiation failed"));
      dwError = ERR_INTERNAL_ERROR;
      goto connect_cleanup;
   }
   debugPrintf(6, _T("Using NXCP version %d"), m_nProtocolVersion);

   // Start receiver thread
   lock();
   m_receiver = AgentConnectionReceiver::create(self());
   m_receiver->start();
   unlock();

   // Setup encryption
setup_encryption:
   if ((m_encryptionPolicy == ENCRYPTION_PREFERRED) ||
       (m_encryptionPolicy == ENCRYPTION_REQUIRED) ||
       forceEncryption)    // Agent require encryption
   {
      if (serverKey != nullptr)
      {
         dwError = setupEncryption(serverKey);
         if ((dwError != ERR_SUCCESS) &&
             ((m_encryptionPolicy == ENCRYPTION_REQUIRED) || forceEncryption))
            goto connect_cleanup;
      }
      else
      {
         if ((m_encryptionPolicy == ENCRYPTION_REQUIRED) || forceEncryption)
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
          (m_encryptionPolicy != ENCRYPTION_DISABLED))
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
          (m_encryptionPolicy != ENCRYPTION_DISABLED))
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
		m_receiver->m_encryptionContext.reset();
		unlock();

		debugPrintf(6, _T("Proxy connection established"));

		// Renegotiate NXCP version with actual target agent
	   NXCP_MESSAGE msg;
	   msg.id = 0;
	   msg.numFields = 0;
	   msg.size = htonl(NXCP_HEADER_SIZE);
	   msg.code = htons(CMD_GET_NXCP_CAPS);
	   msg.flags = htons(MF_CONTROL | MF_NXCP_VERSION(NXCP_VERSION));
	   if (m_channel->send(&msg, NXCP_HEADER_SIZE, m_mutexSocketWrite) == NXCP_HEADER_SIZE)
	   {
	      NXCPMessage *rsp = m_pMsgWaitQueue->waitForMessage(CMD_NXCP_CAPS, 0, m_commandTimeout);
	      if (rsp != nullptr)
	      {
	         if (rsp->isControl())
	            m_nProtocolVersion = rsp->getControlData() >> 24;
	         else
	            m_nProtocolVersion = 1; // assume that peer doesn't understand CMD_GET_NXCP_CAPS message
	         delete rsp;
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
		if (socketError != nullptr)
			*socketError = (UINT32)WSAGetLastError();

      lock();

      if (m_receiver != nullptr)
      {
         m_receiver->detach();
         m_receiver.reset();
      }

      if (m_channel != nullptr)
      {
         m_channel->shutdown();
         m_channel->close();
         m_channel->decRefCount();
         m_channel = nullptr;
      }

      unlock();
   }
   m_isConnected = success;
   if (error != nullptr)
      *error = dwError;
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
	else if (m_sendToClientMessageCallback != nullptr)
	{
      m_sendToClientMessageCallback = nullptr;
      onFileDownload(false);
	}

   if (m_channel != nullptr)
   {
      m_channel->shutdown();
      m_channel->decRefCount();
      m_channel = nullptr;
   }
   m_isConnected = false;
   unlock();
   debugPrintf(6, _T("Disconnect completed"));
}

/**
 * Disconnect handler. Default implementation does nothing.
 */
void AgentConnection::onDisconnect()
{
}

/**
 * Set shared secret for authentication (nullptr will disable authentication)
 */
void AgentConnection::setSharedSecret(const TCHAR *secret)
{
   if ((secret != nullptr) && (*secret != 0))
   {
#ifdef UNICODE
      WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR, secret, -1, m_secret, MAX_SECRET_LENGTH, nullptr, nullptr);
      m_secret[MAX_SECRET_LENGTH - 1] = 0;
#else
      strlcpy(m_secret, secret, MAX_SECRET_LENGTH);
#endif
      DecryptPasswordA("netxms", m_secret, m_secret, MAX_SECRET_LENGTH);
   }
   else
   {
      m_secret[0] = 0;
   }
}

/**
 * Get interface list from agent
 */
InterfaceList *AgentConnection::getInterfaceList()
{
   StringList *data;
   if (getList(_T("Net.InterfaceList"), &data) != ERR_SUCCESS)
      return nullptr;

   InterfaceList *pIfList = new InterfaceList(data->size());

   // Parse result set. Each line should have the following format:
   // index ip_address/mask_bits iftype mac_address name
   // index ip_address/mask_bits iftype(mtu) mac_address name
   for(int i = 0; i < data->size(); i++)
   {
      TCHAR *line = MemCopyString(data->get(i));
      TCHAR *pBuf = line;
      UINT32 ifIndex = 0;

      // Index
      TCHAR *pChar = _tcschr(pBuf, ' ');
      if (pChar != nullptr)
      {
         *pChar = 0;
         ifIndex = _tcstoul(pBuf, nullptr, 10);
         pBuf = pChar + 1;
      }

      bool newInterface = false;
      InterfaceInfo *iface = pIfList->findByIfIndex(ifIndex);
      if (iface == nullptr)
      {
         iface = new InterfaceInfo(ifIndex);
         newInterface = true;
      }

      // Address and mask
      pChar = _tcschr(pBuf, _T(' '));
      if (pChar != nullptr)
      {
         TCHAR *pSlash;
         static TCHAR defaultMask[] = _T("24");

         *pChar = 0;
         pSlash = _tcschr(pBuf, _T('/'));
         if (pSlash != nullptr)
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
            addr.setMaskBits(_tcstol(pSlash, nullptr, 10));
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
         if (pChar != nullptr)
         {
            *pChar = 0;

            TCHAR *eptr;
            iface->type = _tcstoul(pBuf, &eptr, 10);

            // newer agents can return if_type(mtu)
            if (*eptr == _T('('))
            {
               pBuf = eptr + 1;
               eptr = _tcschr(pBuf, _T(')'));
               if (eptr != nullptr)
               {
                  *eptr = 0;
                  iface->mtu = _tcstol(pBuf, nullptr, 10);
               }
            }

            pBuf = pChar + 1;
         }

         // MAC address
         pChar = _tcschr(pBuf, ' ');
         if (pChar != nullptr)
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
      MemFree(line);
   }

   delete data;
   return pIfList;
}

/**
 * Get parameter value
 */
uint32_t AgentConnection::getParameter(const TCHAR *param, TCHAR *buffer, size_t size)
{
   if (!m_isConnected)
      return ERR_NOT_CONNECTED;

   NXCPMessage msg(m_nProtocolVersion);
   uint32_t requestId = generateRequestId();
   msg.setCode(CMD_GET_PARAMETER);
   msg.setId(requestId);
   msg.setField(VID_PARAMETER, param);

   uint32_t rcc;
   if (sendMessage(&msg))
   {
      NXCPMessage *response = waitForMessage(CMD_REQUEST_COMPLETED, requestId, m_commandTimeout);
      if (response != nullptr)
      {
         rcc = response->getFieldAsUInt32(VID_RCC);
         if (rcc == ERR_SUCCESS)
         {
            if (response->isFieldExist(VID_VALUE))
            {
               response->getFieldAsString(VID_VALUE, buffer, size);
            }
            else
            {
               rcc = ERR_MALFORMED_RESPONSE;
               debugPrintf(3, _T("Malformed response to CMD_GET_PARAMETER"));
            }
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
 * Query web service. Request type determines if parameter or list mode will be used.
 * Only first element of "pathList" will be used for list request.
 * "results" argument should point to StringMap for parameters request and to StringList for list request.
 * For list first element form parameters list will be used. If parameters list is empty
 * "/" will be used for XML and JSON types and "(*)" will be used for text type.
 */
uint32_t AgentConnection::queryWebService(WebServiceRequestType requestType, const TCHAR *url, uint32_t requestTimeout,
         uint32_t retentionTime, const TCHAR *login, const TCHAR *password, WebServiceAuthType authType, const StringMap& headers,
         const StringList& pathList, bool verifyCert, bool verifyHost, bool forcePlainTextParser, void *results)
{
   if (!m_isConnected)
      return ERR_NOT_CONNECTED;

   NXCPMessage msg(m_nProtocolVersion);
   uint32_t requestId = generateRequestId();
   msg.setCode(CMD_QUERY_WEB_SERVICE);
   msg.setId(requestId);
   msg.setField(VID_URL, url);
   msg.setField(VID_TIMEOUT, requestTimeout);
   msg.setField(VID_RETENTION_TIME, retentionTime);
   msg.setField(VID_LOGIN_NAME, login);
   msg.setField(VID_PASSWORD, password);
   msg.setField(VID_AUTH_TYPE, static_cast<uint16_t>(authType));
   msg.setField(VID_VERIFY_CERT, verifyCert);
   msg.setField(VID_VERIFY_HOST, verifyHost);
   msg.setField(VID_FORCE_PLAIN_TEXT_PARSER, forcePlainTextParser);
   headers.fillMessage(&msg, VID_NUM_HEADERS, VID_HEADERS_BASE);
   msg.setField(VID_REQUEST_TYPE, static_cast<uint16_t>(requestType));
   pathList.fillMessage(&msg, VID_PARAM_LIST_BASE, VID_NUM_PARAMETERS);

   uint32_t rcc;
   if (sendMessage(&msg))
   {
      NXCPMessage *response = waitForMessage(CMD_REQUEST_COMPLETED, requestId, m_commandTimeout);
      if (response != nullptr)
      {
         rcc = response->getFieldAsUInt32(VID_RCC);
         if (rcc == ERR_SUCCESS)
         {
            if ((requestType == WebServiceRequestType::PARAMETER) && response->isFieldExist(VID_NUM_PARAMETERS))
            {
               static_cast<StringMap*>(results)->loadMessage(response, VID_NUM_PARAMETERS, VID_PARAM_LIST_BASE);
            }
            else if ((requestType == WebServiceRequestType::LIST) && response->isFieldExist(VID_NUM_ELEMENTS))
            {
               static_cast<StringList*>(results)->loadMessage(response, VID_ELEMENT_LIST_BASE, VID_NUM_ELEMENTS);
            }
            else
            {
               rcc = ERR_MALFORMED_RESPONSE;
               debugPrintf(4, _T("Malformed response to CMD_QUERY_WEB_SERVICE"));
            }
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
 * Method is used for both - to get parameters and to get list.
 * For list first element form parameters list will be used. If parameters list is empty:
 * "/" will be used for XML and JSOn types and "(*)" will be used for text type.
 */
uint32_t AgentConnection::queryWebServiceList(const TCHAR *url, uint32_t requestTimeout, uint32_t retentionTime, const TCHAR *login, const TCHAR *password,
         WebServiceAuthType authType, const StringMap& headers, const TCHAR *path, bool verifyCert, bool verifyHost, bool forcePlainTextParser, StringList *results)
{
   StringList pathList;
   pathList.add(path);
   return queryWebService(WebServiceRequestType::LIST, url, requestTimeout, retentionTime, login, password, authType, headers,
            pathList, verifyCert, verifyHost, forcePlainTextParser, results);
}

/**
 * Query web service for parameters
 */
uint32_t AgentConnection::queryWebServiceParameters(const TCHAR *url, uint32_t requestTimeout, uint32_t retentionTime, const TCHAR *login, const TCHAR *password,
         WebServiceAuthType authType, const StringMap& headers, const StringList& pathList, bool verifyCert, bool verifyHost, bool forcePlainTextParser, StringMap *results)
{
   return queryWebService(WebServiceRequestType::PARAMETER, url, requestTimeout, retentionTime, login, password, authType, headers,
            pathList, verifyCert, verifyHost, forcePlainTextParser, results);
}

/**
 * Get ARP cache
 */
ArpCache *AgentConnection::getArpCache()
{
   StringList *data;
   if (getList(_T("Net.ArpCache"), &data) != ERR_SUCCESS)
      return nullptr;

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
      TCHAR *line = MemCopyString(data->get(i));
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
         macAddr[j] = (BYTE)_tcstol(szByte, nullptr, 16);
         pBuf += 2;
      }

      // IP address
      while(*pBuf == ' ')
         pBuf++;
      pChar = _tcschr(pBuf, _T(' '));
      if (pChar != nullptr)
         *pChar = 0;
      InetAddress ipAddr = InetAddress::parse(pBuf);

      // Interface index
      UINT32 ifIndex = (pChar != nullptr) ? _tcstoul(pChar + 1, nullptr, 10) : 0;

      arpCache->addEntry(ipAddr, MacAddress(macAddr, 6), ifIndex);

      free(line);
   }

   delete data;
   return arpCache;
}

/**
 * Send dummy command to agent (can be used for keepalive)
 */
uint32_t AgentConnection::nop()
{
   if (!m_isConnected)
      return ERR_CONNECTION_BROKEN;

   NXCPMessage msg(m_nProtocolVersion);
   uint32_t requestId = generateRequestId();
   msg.setCode(CMD_KEEPALIVE);
   msg.setId(requestId);
   if (sendMessage(&msg))
      return waitForRCC(requestId, m_commandTimeout);
   else
      return ERR_CONNECTION_BROKEN;
}

/**
 * inform agent about server capabilities
 */
uint32_t AgentConnection::setServerCapabilities()
{
   NXCPMessage msg(m_nProtocolVersion);
   uint32_t requestId = generateRequestId();
   msg.setCode(CMD_SET_SERVER_CAPABILITIES);
   msg.setField(VID_ENABLED, (INT16)1);   // Enables IPv6 on pre-2.0 agents
   msg.setField(VID_IPV6_SUPPORT, (INT16)1);
   msg.setField(VID_BULK_RECONCILIATION, (INT16)1);
   msg.setField(VID_ENABLE_COMPRESSION, (INT16)(m_allowCompression ? 1 : 0));
   msg.setId(requestId);
   if (!sendMessage(&msg))
      return ERR_CONNECTION_BROKEN;

   NXCPMessage *response = m_pMsgWaitQueue->waitForMessage(CMD_REQUEST_COMPLETED, requestId, m_commandTimeout);
   if (response == nullptr)
      return ERR_REQUEST_TIMEOUT;

   uint32_t rcc = response->getFieldAsUInt32(VID_RCC);
   if (rcc == ERR_SUCCESS)
   {
      if (response->isFieldExist(VID_FLAGS))
      {
         uint16_t flags = response->getFieldAsUInt16(VID_FLAGS);
         if (flags & 0x01)
            m_controlServer = true;
         if (flags & 0x02)
            m_masterServer = true;
      }
      else
      {
         // Agents before 2.2.13 do not return access flags, assume this server has full access
         m_controlServer = true;
         m_masterServer = true;
      }
   }
   delete response;
   return rcc;
}

/**
 * Set server ID
 */
uint32_t AgentConnection::setServerId(uint64_t serverId)
{
   NXCPMessage msg(m_nProtocolVersion);
   uint32_t requestId = generateRequestId();
   msg.setCode(CMD_SET_SERVER_ID);
   msg.setField(VID_SERVER_ID, serverId);
   msg.setId(requestId);
   if (sendMessage(&msg))
      return waitForRCC(requestId, m_commandTimeout);
   else
      return ERR_CONNECTION_BROKEN;
}

/**
 * Wait for request completion code
 */
uint32_t AgentConnection::waitForRCC(uint32_t requestId, uint32_t timeout)
{
   uint32_t rcc;
   NXCPMessage *response = m_pMsgWaitQueue->waitForMessage(CMD_REQUEST_COMPLETED, requestId, timeout);
   if (response != nullptr)
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
   if (channel == nullptr)
      return false;

   if (nxlog_get_debug_level_tag_object(DEBUG_TAG, m_debugId) >= 6)
   {
      TCHAR buffer[64];
      debugPrintf(6, _T("Sending message %s (%d) to agent at %s"),
         NXCPMessageCodeName(pMsg->getCode(), buffer), pMsg->getId(), (const TCHAR *)m_addr.toString());
   }

   bool success;
   NXCP_MESSAGE *rawMsg = pMsg->serialize(m_allowCompression);
	shared_ptr<NXCPEncryptionContext> encryptionContext = acquireEncryptionContext();
   if (encryptionContext != nullptr)
   {
      NXCP_ENCRYPTED_MESSAGE *encryptedMsg = encryptionContext->encryptMessage(rawMsg);
      if (encryptedMsg != nullptr)
      {
         success = (channel->send(encryptedMsg, ntohl(encryptedMsg->size), m_mutexSocketWrite) == (int)ntohl(encryptedMsg->size));
         MemFree(encryptedMsg);
      }
      else
      {
         success = false;
      }
   }
   else
   {
      success = (channel->send(rawMsg, ntohl(rawMsg->size), m_mutexSocketWrite) == (int)ntohl(rawMsg->size));
   }
   MemFree(rawMsg);
   channel->decRefCount();
   return success;
}

/**
 * Send raw message to agent
 */
bool AgentConnection::sendRawMessage(NXCP_MESSAGE *pMsg)
{
   AbstractCommChannel *channel = acquireChannel();
   if (channel == nullptr)
      return false;

   bool success;
   NXCP_MESSAGE *rawMsg = pMsg;
	shared_ptr<NXCPEncryptionContext> encryptionContext = acquireEncryptionContext();
   if (encryptionContext != nullptr)
   {
      NXCP_ENCRYPTED_MESSAGE *pEnMsg = encryptionContext->encryptMessage(rawMsg);
      if (pEnMsg != nullptr)
      {
         success = (channel->send(pEnMsg, ntohl(pEnMsg->size), m_mutexSocketWrite) == (int)ntohl(pEnMsg->size));
         free(pEnMsg);
      }
      else
      {
         success = false;
      }
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
   MemFree(msg);
}

/**
 * Send raw NXCP message in background. Provided message will be destroyed after sending.
 */
void AgentConnection::postRawMessage(NXCP_MESSAGE *msg)
{
   TCHAR key[64];
   _sntprintf(key, 64, _T("PostMessage_%p"), this);
   ThreadPoolExecuteSerialized(g_agentConnectionThreadPool, key, self(), &AgentConnection::postRawMessageCallback, msg);
}

/**
 * Callback for processing incoming event on separate thread
 */
void AgentConnection::onTrapCallback(NXCPMessage *msg)
{
   onTrap(msg);
   delete msg;
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
   onSyslogMessage(*msg);
   delete msg;
}

/**
 * Syslog message handler. Should be overriden in derived classes to implement
 * actual message processing. Default implementation do nothing.
 */
void AgentConnection::onSyslogMessage(const NXCPMessage& msg)
{
}

/**
 * Callback for processing incoming windows evens on separate thread
 */
void AgentConnection::onWindowsEventCallback(NXCPMessage *msg)
{
   onWindowsEvent(*msg);
   delete msg;
}

/**
 * Windows event handler. Should be overriden in derived classes to implement
 * actual event processing. Default implementation do nothing.
 */
void AgentConnection::onWindowsEvent(const NXCPMessage& msg)
{
}

/**
 * Callback for processing data push on separate thread
 */
void AgentConnection::onDataPushCallback(NXCPMessage *msg)
{
   onDataPush(msg);
   delete msg;
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
uint32_t AgentConnection::getList(const TCHAR *param, StringList **list)
{
   uint32_t rcc;
   *list = nullptr;
   if (m_isConnected)
   {
      NXCPMessage msg(CMD_GET_LIST, generateRequestId(), m_nProtocolVersion);
      msg.setField(VID_PARAMETER, param);
      if (sendMessage(&msg))
      {
         NXCPMessage *response = waitForMessage(CMD_REQUEST_COMPLETED, msg.getId(), m_commandTimeout);
         if (response != nullptr)
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
uint32_t AgentConnection::getTable(const TCHAR *pszParam, Table **table)
{
   NXCPMessage msg(m_nProtocolVersion), *pResponse;
   uint32_t dwRqId, dwRetCode;

	*table = nullptr;
   if (m_isConnected)
   {
      dwRqId = generateRequestId();
      msg.setCode(CMD_GET_TABLE);
      msg.setId(dwRqId);
      msg.setField(VID_PARAMETER, pszParam);
      if (sendMessage(&msg))
      {
         pResponse = waitForMessage(CMD_REQUEST_COMPLETED, dwRqId, m_commandTimeout);
         if (pResponse != nullptr)
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
uint32_t AgentConnection::authenticate(BOOL bProxyData)
{
   const char *secret = bProxyData ? m_proxySecret : m_secret;
   if (*secret == 0)
      return ERR_SUCCESS;  // No authentication required

   NXCPMessage msg(m_nProtocolVersion);
   msg.setCode(CMD_AUTHENTICATE);
   uint32_t requestId = generateRequestId();
   msg.setId(requestId);
   msg.setField(VID_AUTH_METHOD, (WORD)AUTH_SHA1_HASH);  // For compatibility with agents before 3.3
   BYTE hash[SHA1_DIGEST_SIZE];
   CalculateSHA1Hash(reinterpret_cast<const BYTE*>(secret), strlen(secret), hash);
   msg.setField(VID_SHARED_SECRET, hash, SHA1_DIGEST_SIZE);
   if (sendMessage(&msg))
      return waitForRCC(requestId, m_commandTimeout);
   else
      return ERR_CONNECTION_BROKEN;
}

/**
 * Execute command on agent
 */
uint32_t AgentConnection::executeCommand(const TCHAR *command, const StringList &args,
      bool withOutput, void (*outputCallback)(ActionCallbackEvent, const TCHAR*, void*), void *cbData)
{
   if (!m_isConnected)
      return ERR_NOT_CONNECTED;

   NXCPMessage request(CMD_ACTION, generateRequestId(), m_nProtocolVersion);
   request.setField(VID_ACTION_NAME, command);
   request.setField(VID_RECEIVE_OUTPUT, withOutput);
   args.fillMessage(&request, VID_ACTION_ARG_BASE, VID_NUM_ARGS);

   if (sendMessage(&request))
   {
      if (withOutput)
      {
         uint32_t rcc = waitForRCC(request.getId(), m_commandTimeout);
         if (rcc == ERR_SUCCESS)
         {
            outputCallback(ACE_CONNECTED, nullptr, cbData);    // Indicate successful start
            bool eos = false;
            while(!eos)
            {
               NXCPMessage *response = waitForMessage(CMD_COMMAND_OUTPUT, request.getId(), m_commandTimeout * 10);
               if (response != nullptr)
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
            outputCallback(ACE_DISCONNECTED, nullptr, cbData);
            return ERR_SUCCESS;
         }
         else
         {
            return rcc;
         }
      }
      else
      {
         return waitForRCC(request.getId(), m_commandTimeout);
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
UINT32 AgentConnection::uploadFile(const TCHAR *localFile, const TCHAR *destinationFile, bool allowPathExpansion,
         void (* progressCallback)(INT64, void *), void *cbArg, NXCPStreamCompressionMethod compMethod)
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
   if ((destinationFile == nullptr) || (*destinationFile == 0))
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
		msg.setField(VID_ALLOW_PATH_EXPANSION, allowPathExpansion);
   }
   msg.setFieldFromTime(VID_MODIFICATION_TIME, lastModTime);

   if (sendMessage(&msg))
   {
      dwResult = waitForRCC(dwRqId, m_commandTimeout);
   }
   else
   {
      dwResult = ERR_CONNECTION_BROKEN;
   }

   if (dwResult == ERR_SUCCESS)
   {
      AbstractCommChannel *channel = acquireChannel();
      if (channel != nullptr)
      {
         debugPrintf(5, _T("Sending file \"%s\" to agent %s compression"),
                  localFile, (compMethod == NXCP_STREAM_COMPRESSION_NONE) ? _T("without") : _T("with"));
         m_fileUploadInProgress = true;
         shared_ptr<NXCPEncryptionContext> ctx = acquireEncryptionContext();
         if (SendFileOverNXCP(channel, dwRqId, localFile, ctx.get(), 0, progressCallback, cbArg, m_mutexSocketWrite, compMethod))
            dwResult = waitForRCC(dwRqId, m_commandTimeout);
         else
            dwResult = ERR_IO_FAILURE;
         m_fileUploadInProgress = false;
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
      dwResult = waitForRCC(dwRqId, m_commandTimeout);
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
      if (pResponse != nullptr)
      {
         dwResult = pResponse->getFieldAsUInt32(VID_RCC);
         if (dwResult == ERR_SUCCESS)
         {
            *pdwStatus = pResponse->getFieldAsUInt32(VID_SERVICE_STATUS);
            if (responseTime != nullptr)
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

   *paramList = nullptr;
	*tableList = nullptr;

   if (!m_isConnected)
      return ERR_NOT_CONNECTED;

   dwRqId = generateRequestId();

   msg.setCode(CMD_GET_PARAMETER_LIST);
   msg.setId(dwRqId);

   if (sendMessage(&msg))
   {
      pResponse = waitForMessage(CMD_REQUEST_COMPLETED, dwRqId, m_commandTimeout);
      if (pResponse != nullptr)
      {
         dwResult = pResponse->getFieldAsUInt32(VID_RCC);
			DbgPrintf(6, _T("AgentConnection::getSupportedParameters(): RCC=%d"), dwResult);
         if (dwResult == ERR_SUCCESS)
         {
            uint32_t count = pResponse->getFieldAsUInt32(VID_NUM_PARAMETERS);
            ObjectArray<AgentParameterDefinition> *plist = new ObjectArray<AgentParameterDefinition>(count, 16, Ownership::True);
            for(uint32_t i = 0, id = VID_PARAM_LIST_BASE; i < count; i++)
            {
               plist->add(new AgentParameterDefinition(pResponse, id));
               id += 3;
            }
				*paramList = plist;
				DbgPrintf(6, _T("AgentConnection::getSupportedParameters(): %d parameters received from agent"), count);

            count = pResponse->getFieldAsUInt32(VID_NUM_TABLES);
            ObjectArray<AgentTableDefinition> *tlist = new ObjectArray<AgentTableDefinition>(count, 16, Ownership::True);
            for(uint32_t i = 0, id = VID_TABLE_LIST_BASE; i < count; i++)
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
uint32_t AgentConnection::setupEncryption(RSA *pServerKey)
{
#ifdef _WITH_ENCRYPTION
   uint32_t requestId = generateRequestId();
   NXCPMessage msg(m_nProtocolVersion);
   msg.setId(requestId);
   PrepareKeyRequestMsg(&msg, pServerKey, false);

   uint32_t result;
   if (sendMessage(&msg))
   {
      NXCPMessage *response = waitForMessage(CMD_SESSION_KEY, requestId, m_commandTimeout);
      if (response != nullptr)
      {
         NXCPEncryptionContext *encryptionContext = nullptr;
         uint32_t rcc = SetupEncryptionContext(response, &encryptionContext, nullptr, pServerKey, m_nProtocolVersion);
         switch(rcc)
         {
            case RCC_SUCCESS:
               m_receiver->m_encryptionContext = shared_ptr<NXCPEncryptionContext>(encryptionContext);
               m_receiver->m_messageReceiver->setEncryptionContext(m_receiver->m_encryptionContext);
               result = ERR_SUCCESS;
               break;
            case RCC_NO_CIPHERS:
               result = ERR_NO_CIPHERS;
               break;
            case RCC_INVALID_PUBLIC_KEY:
               result = ERR_INVALID_PUBLIC_KEY;
               break;
            case RCC_INVALID_SESSION_KEY:
               result = ERR_INVALID_SESSION_KEY;
               break;
            default:
               result = ERR_INTERNAL_ERROR;
               break;
         }
			delete response;
      }
      else
      {
         result = ERR_REQUEST_TIMEOUT;
      }
   }
   else
   {
      result = ERR_CONNECTION_BROKEN;
   }

   return result;
#else
   return ERR_NOT_IMPLEMENTED;
#endif
}

/**
 * Get configuration file from agent
 */
uint32_t AgentConnection::readConfigFile(TCHAR **content, size_t *sizeptr)
{
   *content = nullptr;
   *sizeptr = 0;

   if (!m_isConnected)
      return ERR_NOT_CONNECTED;

   uint32_t rcc;
   uint32_t requestId = generateRequestId();

   NXCPMessage msg(m_nProtocolVersion);
   msg.setCode(CMD_READ_AGENT_CONFIG_FILE);
   msg.setId(requestId);

   if (sendMessage(&msg))
   {
      NXCPMessage *response = waitForMessage(CMD_REQUEST_COMPLETED, requestId, m_commandTimeout);
      if (response != nullptr)
      {
         rcc = response->getFieldAsUInt32(VID_RCC);
         if (rcc == ERR_SUCCESS)
         {
            size_t size = response->getFieldAsBinary(VID_CONFIG_FILE, nullptr, 0);
            BYTE *utf8Text = (BYTE *)malloc(size + 1);
            response->getFieldAsBinary(VID_CONFIG_FILE, (BYTE *)utf8Text, size);

            // We expect text file, so replace all non-printable characters with spaces
            for(size_t i = 0; i < size; i++)
               if ((utf8Text[i] < ' ') &&
                   (utf8Text[i] != '\t') &&
                   (utf8Text[i] != '\r') &&
                   (utf8Text[i] != '\n'))
                  utf8Text[i] = ' ';
            utf8Text[size] = 0;

#ifdef UNICODE
            *content = WideStringFromUTF8String((char *)utf8Text);
#else
            *content = MBStringFromUTF8String((char *)utf8Text);
#endif
            MemFree(utf8Text);
            *sizeptr = _tcslen(*content);
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
 * Update configuration file on agent
 */
uint32_t AgentConnection::writeConfigFile(const TCHAR *content)
{
   NXCPMessage msg(m_nProtocolVersion);

   if (!m_isConnected)
      return ERR_NOT_CONNECTED;

   uint32_t requestId = generateRequestId();

   msg.setCode(CMD_WRITE_AGENT_CONFIG_FILE);
   msg.setId(requestId);
#ifdef UNICODE
   char *utf8content = UTF8StringFromWideString(content);
   msg.setField(VID_CONFIG_FILE, (BYTE *)utf8content, strlen(utf8content));
   MemFree(utf8content);
#else
   msg.setField(VID_CONFIG_FILE, (const BYTE *)content, strlen(content));
#endif

   uint32_t rcc;
   if (sendMessage(&msg))
   {
      rcc = waitForRCC(requestId, m_commandTimeout);
   }
   else
   {
      rcc = ERR_CONNECTION_BROKEN;
   }

   return rcc;
}

/**
 * Get routing table from agent
 */
RoutingTable *AgentConnection::getRoutingTable()
{
   StringList *data;
   if (getList(_T("Net.IP.RoutingTable"), &data) != ERR_SUCCESS)
      return nullptr;

   auto routingTable = new RoutingTable(data->size(), 64);
   for(int i = 0; i < data->size(); i++)
   {
      TCHAR *line = MemCopyString(data->get(i));
      TCHAR *pBuf = line;

      ROUTE route;
      memset(&route, 0, sizeof(route));

      // Destination address and mask
      TCHAR *pChar = _tcschr(pBuf, _T(' '));
      if (pChar != nullptr)
      {
         TCHAR *pSlash;
         static TCHAR defaultMask[] = _T("24");

         *pChar = 0;
         pSlash = _tcschr(pBuf, _T('/'));
         if (pSlash != nullptr)
         {
            *pSlash = 0;
            pSlash++;
         }
         else     // Just a paranoia protection, should'n happen if agent working correctly
         {
            pSlash = defaultMask;
         }
         route.dwDestAddr = InetAddress::parse(pBuf).getAddressV4();
         uint32_t bits = _tcstoul(pSlash, nullptr, 10);
         route.dwDestMask = (bits == 32) ? 0xFFFFFFFF : (~(0xFFFFFFFF >> bits));
         pBuf = pChar + 1;
      }

      // Next hop address
      pChar = _tcschr(pBuf, _T(' '));
      if (pChar != nullptr)
      {
         *pChar = 0;
         route.dwNextHop = InetAddress::parse(pBuf).getAddressV4();
         pBuf = pChar + 1;
      }

      // Interface index
      pChar = _tcschr(pBuf, ' ');
      if (pChar != nullptr)
      {
         *pChar = 0;
         route.dwIfIndex = _tcstoul(pBuf, nullptr, 10);
         pBuf = pChar + 1;
      }

      // Route type
      route.dwRouteType = _tcstoul(pBuf, nullptr, 10);

      routingTable->add(&route);
      MemFree(line);
   }

   delete data;
   return routingTable;
}

/**
 * Set proxy information
 */
void AgentConnection::setProxy(const InetAddress& addr, uint16_t port, const TCHAR *secret)
{
   m_proxyAddr = addr;
   m_proxyPort = port;
   if ((secret != nullptr) && (*secret != 0))
   {
#ifdef UNICODE
      WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR, secret, -1, m_proxySecret, MAX_SECRET_LENGTH, nullptr, nullptr);
      m_proxySecret[MAX_SECRET_LENGTH - 1] = 0;
#else
      strlcpy(m_proxySecret, secret, MAX_SECRET_LENGTH);
#endif
      DecryptPasswordA("netxms", m_proxySecret, m_proxySecret, MAX_SECRET_LENGTH);
   }
   else
   {
      m_proxySecret[0] = 0;
   }
   m_useProxy = true;
}

/**
 * Setup proxy connection
 */
uint32_t AgentConnection::setupProxyConnection()
{
   NXCPMessage msg(m_nProtocolVersion);
   uint32_t requestId = generateRequestId();
   msg.setCode(CMD_SETUP_PROXY_CONNECTION);
   msg.setId(requestId);
   msg.setField(VID_IP_ADDRESS, m_addr.getAddressV4());  // For compatibility with agents < 2.2.7
   msg.setField(VID_DESTINATION_ADDRESS, m_addr);
   msg.setField(VID_AGENT_PORT, m_port);
   if (sendMessage(&msg))
      return waitForRCC(requestId, 60000);   // Wait 60 seconds for remote connect
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
      return waitForRCC(dwRqId, m_commandTimeout);
   else
      return ERR_CONNECTION_BROKEN;
}

/**
 * Enable trap receiving on connection
 */
uint32_t AgentConnection::enableFileUpdates()
{
   NXCPMessage msg(m_nProtocolVersion);
   UINT32 dwRqId;

   dwRqId = generateRequestId();
   msg.setCode(CMD_ENABLE_FILE_UPDATES);
   msg.setId(dwRqId);
   if (!sendMessage(&msg))
      return ERR_CONNECTION_BROKEN;

   uint32_t rcc = waitForRCC(dwRqId, m_commandTimeout);
   if (rcc == ERR_SUCCESS)
      m_fileUpdateConnection = true;
   return rcc;
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
      NXCPMessage *response = waitForMessage(CMD_REQUEST_COMPLETED, dwRqId, m_commandTimeout);
      if (response != nullptr)
      {
         UINT32 rcc = response->getFieldAsUInt32(VID_RCC);
         if (rcc == ERR_SUCCESS)
         {
            const BYTE *p = response->getBinaryFieldPtr(VID_FILE_DATA, size);
            if (p != nullptr)
            {
               *data = (BYTE *)malloc(*size);
               memcpy(*data, p, *size);
            }
            else
            {
               *data = nullptr;
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
TCHAR *AgentConnection::getHostByAddr(const InetAddress& ipAddr, TCHAR *buffer, size_t bufLen)
{
   NXCPMessage msg(m_nProtocolVersion);
   UINT32 dwRqId = generateRequestId();
   msg.setCode(CMD_GET_HOSTNAME_BY_IPADDR);
   msg.setId(dwRqId);
   msg.setField(VID_IP_ADDRESS, ipAddr);
   TCHAR *result = nullptr;
   if (sendMessage(&msg))
   {
      NXCPMessage *response = waitForMessage(CMD_REQUEST_COMPLETED, dwRqId, m_commandTimeout);
      if (response != nullptr)
      {
         if (response->getFieldAsUInt32(VID_RCC) == ERR_SUCCESS)
         {
            result = response->getFieldAsString(VID_NAME, buffer, bufLen);
            if ((result != nullptr) && (*result == 0))
            {
               // Agents before 2.2.16 can return empty string instead of error if IP cannot be resolved
               if (buffer == nullptr)
                  MemFree(result);
               result = nullptr;
            }
         }
         delete response;
      }
   }
   return result;
}

/**
 * Send custom request to agent
 */
NXCPMessage *AgentConnection::customRequest(NXCPMessage *request, const TCHAR *recvFile, bool append,
         void (*downloadProgressCallback)(size_t, void*), void (*fileResendCallback)(NXCPMessage*, void*), void *cbArg)
{
	NXCPMessage *msg = nullptr;

   uint32_t requestId = generateRequestId();
   request->setId(requestId);
	if (recvFile != nullptr)
	{
	   uint32_t rcc = prepareFileDownload(recvFile, requestId, append, downloadProgressCallback, fileResendCallback, cbArg);
		if (rcc != ERR_SUCCESS)
		{
			// Create fake response message
			msg = new NXCPMessage;
			msg->setCode(CMD_REQUEST_COMPLETED);
			msg->setId(requestId);
			msg->setField(VID_RCC, rcc);
		}
	}

	if ((msg == nullptr) && sendMessage(request))
   {
      msg = waitForMessage(CMD_REQUEST_COMPLETED, requestId, m_commandTimeout);
      if ((msg != nullptr) && (recvFile != nullptr))
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
            if (fileResendCallback != nullptr)
            {
               _close(m_hCurrFile);
               m_hCurrFile = -1;
               _tremove(recvFile);
            }
         }
      }

   }

	return msg;
}

/**
 * Cancel file download
 */
uint32_t AgentConnection::cancelFileDownload()
{
   NXCPMessage msg(CMD_CANCEL_FILE_DOWNLOAD, generateRequestId(), getProtocolVersion());
   msg.setField(VID_REQUEST_ID, m_dwDownloadRequestId);

   uint32_t rcc;
   if (sendMessage(&msg))
   {
      NXCPMessage *result = waitForMessage(CMD_REQUEST_COMPLETED, msg.getId(), m_commandTimeout);
      if (result != nullptr)
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
uint32_t AgentConnection::prepareFileDownload(const TCHAR *fileName, uint32_t rqId, bool append,
         void (*downloadProgressCallback)(size_t, void*), void (*fileResendCallback)(NXCPMessage*, void*), void *cbArg)
{
   if (fileResendCallback == nullptr)
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
            _lseek(m_hCurrFile, 0, SEEK_END);
      }

      m_dwDownloadRequestId = rqId;
      m_downloadProgressCallback = downloadProgressCallback;
      m_downloadProgressCallbackArg = cbArg;

      m_sendToClientMessageCallback = nullptr;

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
 * Process incoming file data
 */
void AgentConnection::processFileData(NXCPMessage *msg)
{
   if (m_sendToClientMessageCallback != nullptr)
   {
      m_sendToClientMessageCallback(msg, m_downloadProgressCallbackArg);
      if (msg->isEndOfFile())
      {
         m_sendToClientMessageCallback = nullptr;
         onFileDownload(true);
      }
      else
      {
         if (m_downloadProgressCallback != nullptr)
         {
            m_downloadProgressCallback(msg->getBinaryDataSize(), m_downloadProgressCallbackArg);
         }
      }
   }
   else
   {
      if (m_hCurrFile != -1)
      {
         if (_write(m_hCurrFile, msg->getBinaryData(), static_cast<int>(msg->getBinaryDataSize())) == static_cast<int>(msg->getBinaryDataSize()))
         {
            if (msg->isEndOfFile())
            {
               _close(m_hCurrFile);
               m_hCurrFile = -1;
               onFileDownload(true);
            }
            else if (m_downloadProgressCallback != nullptr)
            {
               m_downloadProgressCallback(_tell(m_hCurrFile), m_downloadProgressCallbackArg);
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
   delete msg;
}

/**
 * Process file transfer abort request
 */
void AgentConnection::processFileTransferAbort(NXCPMessage *msg)
{
   if (m_sendToClientMessageCallback != nullptr)
   {
      m_sendToClientMessageCallback(msg, m_downloadProgressCallbackArg);
      m_sendToClientMessageCallback = nullptr;
   }
   else
   {
      _close(m_hCurrFile);
      m_hCurrFile = -1;
   }
   onFileDownload(false);
   delete msg;
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

	*info = nullptr;
   uint32_t requestId = generateRequestId();
   msg.setCode(CMD_GET_POLICY_INVENTORY);
   msg.setId(requestId);
   uint32_t rcc;
   if (sendMessage(&msg))
	{
		NXCPMessage *response = waitForMessage(CMD_REQUEST_COMPLETED, requestId, m_commandTimeout);
		if (response != nullptr)
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
		rcc = waitForRCC(rqId, m_commandTimeout);
	}
	else
	{
		rcc = ERR_CONNECTION_BROKEN;
	}
   return rcc;
}

/**
 * Callback for processing collected data on separate thread
 */
void AgentConnection::processCollectedDataCallback(NXCPMessage *msg)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, msg->getId(), m_nProtocolVersion);

   if (msg->getFieldAsBoolean(VID_BULK_RECONCILIATION))
   {
      // Check that only one bulk data processor is running
      if (InterlockedIncrement(&m_bulkDataProcessing) == 1)
      {
         response.setField(VID_RCC, processBulkCollectedData(msg, &response));
      }
      else
      {
         response.setField(VID_RCC, ERR_RESOURCE_BUSY);
      }
      InterlockedDecrement(&m_bulkDataProcessing);
   }
   else
   {
      uint32_t rcc = processCollectedData(msg);
      response.setField(VID_RCC, rcc);
   }

   sendMessage(&response);
   delete msg;
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
 * Callback for getting SSH keys by id
 */
void AgentConnection::getSshKeysCallback(NXCPMessage *msg)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, msg->getId(), m_nProtocolVersion);
   getSshKeys(msg, &response);
   sendMessage(&response);
   delete msg;
}

/**
 * Get SSH key function
 */
void AgentConnection::getSshKeys(NXCPMessage *request, NXCPMessage *response)
{
   response->setField(VID_RCC, ERR_NOT_IMPLEMENTED);
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
      NXCPMessage *response = waitForMessage(CMD_REQUEST_COMPLETED, requestId, m_commandTimeout);
      if (response != nullptr)
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
      NXCPMessage *response = waitForMessage(CMD_REQUEST_COMPLETED, requestId, m_commandTimeout);
      if (response != nullptr)
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
void AgentConnection::processTcpProxyData(uint32_t channelId, const void *data, size_t size)
{
}

/**
 * Get file set information
 */
uint32_t AgentConnection::getFileSetInfo(const StringList &fileSet, bool allowPathExpansion, ObjectArray<RemoteFileInfo> **fileSetInfo)
{
   *fileSetInfo = nullptr;
   uint32_t requestId = generateRequestId();
   NXCPMessage msg(CMD_GET_FILE_SET_DETAILS, requestId, m_nProtocolVersion);
   msg.setField(VID_ALLOW_PATH_EXPANSION, allowPathExpansion);
   fileSet.fillMessage(&msg, VID_ELEMENT_LIST_BASE, VID_NUM_ELEMENTS);
   uint32_t rcc;
   if (sendMessage(&msg))
   {
      NXCPMessage *response = waitForMessage(CMD_REQUEST_COMPLETED, requestId, m_commandTimeout);
      if (response != nullptr)
      {
         rcc = response->getFieldAsUInt32(VID_RCC);
         if (rcc == ERR_SUCCESS)
         {
            int count = response->getFieldAsInt32(VID_NUM_ELEMENTS);
            if (count == fileSet.size())
            {
               auto info = new ObjectArray<RemoteFileInfo>(count, 16, Ownership::True);
               UINT32 fieldId = VID_ELEMENT_LIST_BASE;
               for(int i = 0; i < count; i++)
               {
                  info->add(new RemoteFileInfo(response, fieldId, fileSet.get(i)));
                  fieldId += 10;
               }
               *fileSetInfo = info;
            }
            else
            {
               rcc = ERR_INTERNAL_ERROR;
            }
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
 * Create new agent parameter definition from NXCP message
 */
AgentParameterDefinition::AgentParameterDefinition(const NXCPMessage *msg, uint32_t baseId)
{
   m_name = msg->getFieldAsString(baseId);
   m_description = msg->getFieldAsString(baseId + 1);
   m_dataType = (int)msg->getFieldAsUInt16(baseId + 2);
}

/**
 * Create new agent parameter definition from another definition object
 */
AgentParameterDefinition::AgentParameterDefinition(const AgentParameterDefinition *src)
{
   m_name = MemCopyString(src->m_name);
   m_description = MemCopyString(src->m_description);
   m_dataType = src->m_dataType;
}

/**
 * Create new agent parameter definition from scratch
 */
AgentParameterDefinition::AgentParameterDefinition(const TCHAR *name, const TCHAR *description, int dataType)
{
   m_name = MemCopyString(name);
   m_description = MemCopyString(description);
   m_dataType = dataType;
}

/**
 * Destructor for agent parameter definition
 */
AgentParameterDefinition::~AgentParameterDefinition()
{
   MemFree(m_name);
   MemFree(m_description);
}

/**
 * Fill NXCP message
 */
uint32_t AgentParameterDefinition::fillMessage(NXCPMessage *msg, uint32_t baseId) const
{
   msg->setField(baseId, m_name);
   msg->setField(baseId + 1, m_description);
   msg->setField(baseId + 2, static_cast<uint16_t>(m_dataType));
   return 3;
}

/**
 * Create new agent table definition from NXCP message
 */
AgentTableDefinition::AgentTableDefinition(const NXCPMessage *msg, uint32_t baseId)
{
   m_name = msg->getFieldAsString(baseId);
   m_description = msg->getFieldAsString(baseId + 2);

   TCHAR *instanceColumns = msg->getFieldAsString(baseId + 1);
   if (instanceColumns != nullptr)
   {
      m_instanceColumns = new StringList(instanceColumns, _T("|"));
      MemFree(instanceColumns);
   }
   else
   {
      m_instanceColumns = new StringList;
   }

   m_columns = new ObjectArray<AgentTableColumnDefinition>(16, 16, Ownership::True);
}

/**
 * Create new agent table definition from another definition object
 */
AgentTableDefinition::AgentTableDefinition(const AgentTableDefinition *src)
{
   m_name = MemCopyString(src->m_name);
   m_description = MemCopyString(src->m_description);
   m_instanceColumns = new StringList(src->m_instanceColumns);
   m_columns = new ObjectArray<AgentTableColumnDefinition>(16, 16, Ownership::True);
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
   MemFree(m_name);
   MemFree(m_description);
   delete m_instanceColumns;
   delete m_columns;
}

/**
 * Fill NXCP message
 */
uint32_t AgentTableDefinition::fillMessage(NXCPMessage *msg, uint32_t baseId) const
{
   msg->setField(baseId + 1, m_name);
   msg->setField(baseId + 2, m_description);

   TCHAR *instanceColumns = m_instanceColumns->join(_T("|"));
   msg->setField(baseId + 3, instanceColumns);
   free(instanceColumns);

   uint32_t fieldId = baseId + 4;
   for(int i = 0; i < m_columns->size(); i++)
   {
      msg->setField(fieldId++, m_columns->get(i)->m_name);
      msg->setField(fieldId++, (WORD)m_columns->get(i)->m_dataType);
   }

   msg->setField(baseId, fieldId - baseId);
   return fieldId - baseId;
}

/**
 * Create remote file info object
 */
RemoteFileInfo::RemoteFileInfo(NXCPMessage *msg, uint32_t baseId, const TCHAR *name)
{
   m_name = MemCopyString(name);
   m_status = msg->getFieldAsUInt32(baseId);
   if (m_status == ERR_SUCCESS)
   {
      m_size = msg->getFieldAsUInt64(baseId + 1);
      m_mtime = msg->getFieldAsTime(baseId + 2);
      msg->getFieldAsBinary(baseId + 3, m_hash, MD5_DIGEST_SIZE);
   }
   else
   {
      m_size = 0;
      m_mtime = 0;
      memset(m_hash, 0, MD5_DIGEST_SIZE);
   }
}

/**
 * Destroy remote file info object
 */
RemoteFileInfo::~RemoteFileInfo()
{
   MemFree(m_name);
}
