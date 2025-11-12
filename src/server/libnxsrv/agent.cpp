/*
** NetXMS - Network Management System
** Server Library
** Copyright (C) 2003-2025 Victor Kirhenshtein
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
#include <netxms-regex.h>

#ifndef _WIN32
#define _tell(f) lseek((f),0,SEEK_CUR)
#endif

#define DEBUG_TAG    _T("agent.conn")

/**
 * Constants
 */
#define MAX_MSG_SIZE    268435456
#define FILE_PART_SIZE  (1024 * 1024)

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
static int m_iDefaultEncryptionPolicy = ENCRYPTION_ALLOWED;
static ObjectArray<BackgroundSocketPollerHandle> s_pollers(64, 64, Ownership::True);
static Mutex s_pollerListLock(MutexType::FAST);
static bool s_shutdownMode = false;
static uint32_t s_maxConnectionsPerPoller = std::min(256, SOCKET_POLLER_MAX_SOCKETS - 1);

/**
 * Set default encryption policy for agent communication
 */
void LIBNXSRV_EXPORTABLE SetAgentDEP(int iPolicy)
{
   m_iDefaultEncryptionPolicy = iPolicy;
}

/**
 * Set shutdown mode for agent connections
 */
void LIBNXSRV_EXPORTABLE DisableAgentConnections()
{
   s_pollerListLock.lock();
   s_shutdownMode = true;
   for(int i = 0; i < s_pollers.size(); i++)
      s_pollers.get(i)->poller.shutdown();
   s_pollerListLock.unlock();
}

/**
 * Agent connection receiver
 */
class AgentConnectionReceiver : public enable_shared_from_this<AgentConnectionReceiver>
{
private:
   weak_ptr<AgentConnection> m_connection;
   uint32_t m_debugId;
   uint32_t m_recvTimeout;
   shared_ptr<AbstractCommChannel> m_channel;
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

   AgentConnectionReceiver(const shared_ptr<AgentConnection>& connection) : m_connection(connection), m_channel(connection->m_channel)
   {
      m_debugId = connection->m_debugId;
      m_messageReceiver = new CommChannelMessageReceiver(m_channel, 4096, MAX_MSG_SIZE);
      m_recvTimeout = connection->m_recvTimeout;
      _sntprintf(m_threadPoolKey, 16, _T("RECV-%u"), m_debugId);
      m_attached = true;
   }

   ~AgentConnectionReceiver()
   {
      if (!(g_flags & AF_SHUTDOWN))
         debugPrintf(7, _T("AgentConnectionReceiver destructor called (this=%p)"), this);
      delete m_messageReceiver;
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
   m_channel->backgroundPoll(m_recvTimeout, channelPollerCallback, shared_from_this());
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
 * Create key for callback processing in thread pool
 */
static inline void CreateCallbackKey(TCHAR prefix, AgentConnectionReceiver *receiver, TCHAR *key)
{
   key[0] = 'C';
   key[1] = prefix;
   key[2] = '-';
   IntegerToString(CAST_FROM_POINTER(receiver, uint64_t), &key[3]);
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

      if ((msg->getCode() == CMD_FILE_DATA) && (msg->getId() == connection->m_downloadRequestId))
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
      else if ((msg->getCode() == CMD_ABORT_FILE_TRANSFER) && (msg->getId() == connection->m_downloadRequestId))
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
         connection->processTcpProxyData(msg->getId(), msg->getBinaryData(), msg->getBinaryDataSize(), false);
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
      connection->m_messageWaitQueue.put(msg);
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
            connection->m_messageWaitQueue.put(msg);
            break;
         case CMD_TRAP:
            if (g_agentConnectionThreadPool != nullptr)
            {
               TCHAR key[64];
               CreateCallbackKey('E', this, key);
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
               CreateCallbackKey('Y', this, key);
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
               CreateCallbackKey('W', this, key);
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
            if (g_agentConnectionThreadPool != nullptr)
            {
               TCHAR key[64];
               CreateCallbackKey('F', this, key);
               ThreadPoolExecuteSerialized(g_agentConnectionThreadPool, key, connection, &AgentConnection::onFileMonitoringDataCallback, msg);
            }
            else
            {
               delete msg;
            }
            break;
         case CMD_SNMP_TRAP:
            if (g_agentConnectionThreadPool != nullptr)
            {
               TCHAR key[64];
               CreateCallbackKey('T', this, key);
               ThreadPoolExecuteSerialized(g_agentConnectionThreadPool, key, connection, &AgentConnection::onSnmpTrapCallback, msg);
            }
            else
            {
               delete msg;
            }
            break;
         case CMD_CLOSE_TCP_PROXY:
            connection->processTcpProxyData(msg->getFieldAsUInt32(VID_CHANNEL_ID), nullptr, 0, msg->getFieldAsBoolean(VID_ERROR_INDICATOR));
            delete msg;
            break;
         case CMD_NOTIFY:
            if (g_agentConnectionThreadPool != nullptr)
            {
               TCHAR key[64];
               CreateCallbackKey('N', this, key);
               ThreadPoolExecuteSerialized(g_agentConnectionThreadPool, key, connection, &AgentConnection::onNotifyCallback, msg);
            }
            else
            {
               delete msg;
            }
            break;
         case CMD_KEEPALIVE:
            if (g_agentConnectionThreadPool != nullptr)
            {
               NXCPMessage *response = new NXCPMessage(CMD_REQUEST_COMPLETED, msg->getId());
               connection->postMessage(response);  // Response will be deleted by postMessage
            }
            else
            {
               NXCPMessage response(CMD_REQUEST_COMPLETED, msg->getId());
               connection->sendMessage(&response);
            }
            delete msg;
            break;
         default:
            if (connection->processCustomMessage(msg))
               delete msg;
            else
               connection->m_messageWaitQueue.put(msg);
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
AgentConnection::AgentConnection(const InetAddress& addr, uint16_t port, const TCHAR *secret, bool allowCompression) : m_condFileDownload(true)
{
   m_debugId = InterlockedIncrement(&s_connectionId);
   m_addr = addr;
   m_port = port;
   if ((secret != nullptr) && (*secret != 0))
   {
		wchar_to_utf8(secret, -1, m_secret, MAX_SECRET_LENGTH);
		m_secret[MAX_SECRET_LENGTH - 1] = 0;
      DecryptPasswordA("netxms", m_secret, m_secret, MAX_SECRET_LENGTH);
   }
   else
   {
      m_secret[0] = 0;
   }
   m_allowCompression = allowCompression;
   m_tLastCommandTime = 0;
   m_requestId = 0;
	m_connectionTimeout = 5000;	// 5 seconds
   m_commandTimeout = 5000;   // Default timeout 5 seconds
   m_recvTimeout = 420000; // 7 minutes
   m_isConnected = false;
   m_encryptionPolicy = m_iDefaultEncryptionPolicy;
   m_useProxy = false;
   m_proxyPort = 4700;
   m_proxySecret[0] = 0;
   m_nProtocolVersion = NXCP_VERSION;
	m_hCurrFile = -1;
   m_deleteFileOnDownloadFailure = true;
   m_fileDownloadSucceeded = false;
	m_fileUploadInProgress = false;
   m_fileUpdateConnection = false;
   m_downloadRequestId = 0;
   m_downloadActivityTimestamp = 0;
   m_bulkDataProcessing = 0;
   m_controlServer = false;
   m_masterServer = false;

   TCHAR buffer[64];
   debugPrintf(5, _T("New connection created (address=%s port=%u compression=%s)"), m_addr.toString(buffer), m_port, allowCompression ? _T("allowed") : _T("forbidden"));
}

/**
 * Destructor
 */
AgentConnection::~AgentConnection()
{
   if (!(g_flags & AF_SHUTDOWN))
      debugPrintf(7, _T("AgentConnection destructor called (this=%p)"), this);

   if (m_receiver != nullptr)
      m_receiver->detach();

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
      m_channel->shutdown();
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
shared_ptr<AbstractCommChannel> AgentConnection::createChannel()
{
   if (s_shutdownMode)
      return shared_ptr<AbstractCommChannel>();

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
      return shared_ptr<AbstractCommChannel>();
   }

   // Select socket poller
   BackgroundSocketPollerHandle *sp = nullptr;
   s_pollerListLock.lock();
   if (s_shutdownMode)
   {
      shutdown(s, SHUT_RDWR);
      closesocket(s);
      s_pollerListLock.unlock();
      return shared_ptr<AbstractCommChannel>();
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

   return make_shared<SocketCommChannel>(s, sp);
}

/**
 * Acquire communication channel. Caller must call decRefCount to release channel.
 */
shared_ptr<AbstractCommChannel> AgentConnection::acquireChannel()
{
   lock();
   shared_ptr<AbstractCommChannel> channel(m_channel);
   unlock();
   return channel;
}

/**
 * Acquire encryption context
 */
shared_ptr<NXCPEncryptionContext> AgentConnection::acquireEncryptionContext()
{
   lock();
   shared_ptr<NXCPEncryptionContext> ctx = (m_receiver != nullptr) ? m_receiver->m_encryptionContext : shared_ptr<NXCPEncryptionContext>();
   unlock();
   return ctx;
}

/**
 * Connect to agent
 */
bool AgentConnection::connect(RSA_KEY serverKey, uint32_t *error, uint32_t *socketError, uint64_t serverId)
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

   // Detach from existing channel if any
   m_channel.reset();

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

   if (!NXCPGetPeerProtocolVersion(m_channel, &m_nProtocolVersion, &m_mutexSocketWrite))
   {
      debugPrintf(6, _T("Protocol version negotiation failed"));
      dwError = ERR_INTERNAL_ERROR;
      goto connect_cleanup;
   }
   debugPrintf(6, _T("Using NXCP version %d"), m_nProtocolVersion);

   // Start receiver thread
   lock();
   m_receiver = make_shared<AgentConnectionReceiver>(shared_from_this());
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
	   if (m_channel->send(&msg, NXCP_HEADER_SIZE, &m_mutexSocketWrite) == NXCP_HEADER_SIZE)
	   {
	      NXCPMessage *rsp = m_messageWaitQueue.waitForMessage(CMD_NXCP_CAPS, 0, m_commandTimeout);
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
         m_channel.reset();
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
      m_channel.reset();
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
      wchar_to_utf8(secret, -1, m_secret, MAX_SECRET_LENGTH);
      m_secret[MAX_SECRET_LENGTH - 1] = 0;
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
   Table *ifTable;
   if (getTable(_T("Net.Interfaces"), &ifTable) == ERR_SUCCESS)
      return parseInterfaceTable(ifTable);

   StringList *ifList;
   if (getList(_T("Net.InterfaceList"), &ifList) == ERR_SUCCESS)
      return parseInterfaceList(ifList);

   return nullptr;
}

/**
 * Parse interface table received from agent (via Net.Interfaces)
 */
InterfaceList *AgentConnection::parseInterfaceTable(Table *data)
{
   int cIndex = data->getColumnIndex(_T("INDEX"));
   int cName = data->getColumnIndex(_T("NAME"));
   int cAlias = data->getColumnIndex(_T("ALIAS"));
   int cType = data->getColumnIndex(_T("TYPE"));
   int cMTU = data->getColumnIndex(_T("MTU"));
   int cMAC = data->getColumnIndex(_T("MAC_ADDRESS"));
   int cIPList = data->getColumnIndex(_T("IP_ADDRESSES"));
   int cSpeed = data->getColumnIndex(_T("SPEED"));
   int cMaxSpeed = data->getColumnIndex(_T("MAX_SPEED"));

   if ((cIndex == -1) || (cName == -1) || (cMAC == -1) || (cIPList == -1))
      return nullptr;

   InterfaceList *ifList = new InterfaceList(data->getNumRows());

   for(int i = 0; i < data->getNumRows(); i++)
   {
      uint32_t ifIndex = data->getAsUInt(i, cIndex);
      if (ifIndex == 0)
         continue;

      auto iface = new InterfaceInfo(ifIndex);
      wcslcpy(iface->name, data->getAsString(i, cName, L""), MAX_DB_STRING);
      wcslcpy(iface->alias, data->getAsString(i, cAlias, L""), MAX_DB_STRING);
      iface->type = data->getAsUInt(i, cType);
      if (iface->type == 0)
         iface->type = IFTYPE_OTHER;
      iface->mtu = data->getAsUInt(i, cMTU);
      if (cSpeed != -1)
         iface->speed = data->getAsUInt64(i, cSpeed);
      if (cMaxSpeed != -1)
         iface->maxSpeed = data->getAsUInt64(i, cMaxSpeed);
      StrToBin(data->getAsString(i, cMAC, _T("000000000000")), iface->macAddr, MAC_ADDR_LENGTH);

      StringBuffer::split(data->getAsString(i, cIPList, _T("")), _T(","), true,
         [iface] (const String& element) -> void
         {
            InetAddress addr;
            ssize_t slashIndex = element.find(_T("/"));
            if (slashIndex != String::npos)
            {
               addr = InetAddress::parse(element.substring(0, slashIndex));
               addr.setMaskBits(_tcstol(element.substring(slashIndex + 1, -1), nullptr, 10));
            }
            else
            {
               addr = InetAddress::parse(element);
               addr.setMaskBits(24);
            }
            if (addr.isValid())
            {
               iface->ipAddrList.add(addr);
            }
         });

      ifList->add(iface);
   }

   delete data;
   return ifList;
}

/**
 * Parse interface list received from agent (via Net.InterfaceList)
 */
InterfaceList *AgentConnection::parseInterfaceList(StringList *data)
{
   InterfaceList *ifList = new InterfaceList(data->size());

   // Parse result set. Each line should have the following format:
   // index ip_address/mask_bits iftype mac_address name
   // index ip_address/mask_bits iftype(mtu) mac_address name
   for(int i = 0; i < data->size(); i++)
   {
      TCHAR *line = MemCopyString(data->get(i));
      TCHAR *pBuf = line;
      uint32_t ifIndex = 0;

      // Index
      TCHAR *ch = _tcschr(pBuf, ' ');
      if (ch != nullptr)
      {
         *ch = 0;
         ifIndex = _tcstoul(pBuf, nullptr, 10);
         pBuf = ch + 1;
      }

      bool newInterface = false;
      InterfaceInfo *iface = ifList->findByIfIndex(ifIndex);
      if (iface == nullptr)
      {
         iface = new InterfaceInfo(ifIndex);
         newInterface = true;
      }

      // Address and mask
      ch = _tcschr(pBuf, _T(' '));
      if (ch != nullptr)
      {
         static TCHAR defaultMask[] = _T("24");

         *ch = 0;
         TCHAR *slash = _tcschr(pBuf, _T('/'));
         if (slash != nullptr)
         {
            *slash = 0;
            slash++;
         }
         else     // Just a paranoia protection, shouldn't happen if agent working correctly
         {
            slash = defaultMask;
         }
         InetAddress addr = InetAddress::parse(pBuf);
         if (addr.isValid())
         {
            addr.setMaskBits(_tcstol(slash, nullptr, 10));
            // Agent may return 0.0.0.0/0 for interfaces without IP address
            if ((addr.getFamily() != AF_INET) || (addr.getAddressV4() != 0))
               iface->ipAddrList.add(addr);
         }
         pBuf = ch + 1;
      }

      if (newInterface)
      {
         // Interface type
         ch = _tcschr(pBuf, ' ');
         if (ch != nullptr)
         {
            *ch = 0;

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

            pBuf = ch + 1;
         }

         // MAC address
         ch = _tcschr(pBuf, ' ');
         if (ch != nullptr)
         {
            *ch = 0;
            StrToBin(pBuf, iface->macAddr, MAC_ADDR_LENGTH);
            pBuf = ch + 1;
         }

         // Name (set description to name)
         _tcslcpy(iface->name, pBuf, MAX_DB_STRING);
         _tcslcpy(iface->description, pBuf, MAX_DB_STRING);

         ifList->add(iface);
      }
      MemFree(line);
   }

   delete data;
   return ifList;
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
uint32_t AgentConnection::queryWebService(WebServiceRequestType requestType, const TCHAR *url, HttpRequestMethod httpRequestMethod, const TCHAR *requestData,
      uint32_t requestTimeout, uint32_t retentionTime, const TCHAR *login, const TCHAR *password, WebServiceAuthType authType, const StringMap& headers,
      const StringList& pathList, bool verifyCert, bool verifyHost, bool followLocation, bool forcePlainTextParser, void *results)
{
   if (!m_isConnected)
      return ERR_NOT_CONNECTED;

   NXCPMessage msg(m_nProtocolVersion);
   uint32_t requestId = generateRequestId();
   msg.setCode(CMD_QUERY_WEB_SERVICE);
   msg.setId(requestId);
   msg.setField(VID_URL, url);
   msg.setField(VID_HTTP_REQUEST_METHOD, static_cast<uint16_t>(httpRequestMethod));
   msg.setField(VID_REQUEST_DATA, requestData);
   msg.setField(VID_TIMEOUT, requestTimeout);
   msg.setField(VID_RETENTION_TIME, retentionTime);
   msg.setField(VID_LOGIN_NAME, login);
   msg.setField(VID_PASSWORD, password);
   msg.setField(VID_AUTH_TYPE, static_cast<uint16_t>(authType));
   msg.setField(VID_VERIFY_CERT, verifyCert);
   msg.setField(VID_VERIFY_HOST, verifyHost);
   msg.setField(VID_FOLLOW_LOCATION, followLocation);
   msg.setField(VID_FORCE_PLAIN_TEXT_PARSER, forcePlainTextParser);
   headers.fillMessage(&msg, VID_HEADERS_BASE, VID_NUM_HEADERS);
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
               static_cast<StringMap*>(results)->addAllFromMessage(*response, VID_PARAM_LIST_BASE, VID_NUM_PARAMETERS);
            }
            else if ((requestType == WebServiceRequestType::LIST) && response->isFieldExist(VID_NUM_ELEMENTS))
            {
               static_cast<StringList*>(results)->addAllFromMessage(*response, VID_ELEMENT_LIST_BASE, VID_NUM_ELEMENTS);
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
uint32_t AgentConnection::queryWebServiceList(const TCHAR *url, HttpRequestMethod httpRequestMethod, const TCHAR *requestData, uint32_t requestTimeout,
      uint32_t retentionTime, const TCHAR *login, const TCHAR *password, WebServiceAuthType authType, const StringMap& headers, const TCHAR *path,
      bool verifyCert, bool verifyHost, bool followLocation, bool forcePlainTextParser, StringList *results)
{
   StringList pathList;
   pathList.add(path);
   return queryWebService(WebServiceRequestType::LIST, url, httpRequestMethod, requestData, requestTimeout, retentionTime, login, password,
         authType, headers, pathList, verifyCert, verifyHost, followLocation, forcePlainTextParser, results);
}

/**
 * Query web service for parameters
 */
uint32_t AgentConnection::queryWebServiceParameters(const TCHAR *url, HttpRequestMethod httpRequestMethod, const TCHAR *requestData, uint32_t requestTimeout,
      uint32_t retentionTime, const TCHAR *login, const TCHAR *password, WebServiceAuthType authType, const StringMap& headers, const StringList& pathList,
      bool verifyCert, bool verifyHost, bool followLocation, bool forcePlainTextParser, StringMap *results)
{
   return queryWebService(WebServiceRequestType::PARAMETER, url, httpRequestMethod, requestData, requestTimeout, retentionTime, login, password,
         authType, headers, pathList, verifyCert, verifyHost, followLocation, forcePlainTextParser, results);
}

/**
 * Get ARP cache
 */
shared_ptr<ArpCache> AgentConnection::getArpCache()
{
   StringList *data;
   if (getList(_T("Net.ArpCache"), &data) != ERR_SUCCESS)
      return shared_ptr<ArpCache>();

   // Create empty structure
   shared_ptr<ArpCache> arpCache = make_shared<ArpCache>();

   // Parse data lines
   // Each line has form of XXXXXXXXXXXX a.b.c.d n
   // where XXXXXXXXXXXX is a MAC address (12 hexadecimal digits)
   // a.b.c.d is an IP address in decimal dotted notation
   // n is an interface index
   TCHAR line[256];
   for(int i = 0; i < data->size(); i++)
   {
      _tcslcpy(line, data->get(i), 256);
      TCHAR *buffer = line;
      if (_tcslen(buffer) < 20)     // Invalid line
      {
         debugPrintf(7, _T("AgentConnection::getArpCache(): invalid line received from agent (\"%s\")"), line);
         continue;
      }

      // MAC address
      BYTE macAddr[6];
      StrToBin(buffer, macAddr, 6);
      buffer += 12;

      // IP address
      while(*buffer == ' ')
         buffer++;
      TCHAR *ch = _tcschr(buffer, _T(' '));
      if (ch != nullptr)
         *ch = 0;
      InetAddress ipAddr = InetAddress::parse(buffer);

      // Interface index
      uint32_t ifIndex = (ch != nullptr) ? _tcstoul(ch + 1, nullptr, 10) : 0;

      arpCache->addEntry(ipAddr, MacAddress(macAddr, 6), ifIndex);
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
   if (!sendMessage(&msg))
      return ERR_CONNECTION_BROKEN;

   // 750 ms should be enough for agent to respond to keepalive even on slow links
   return waitForRCC(requestId, std::min(m_commandTimeout, static_cast<uint32_t>(750)));
}

/**
 * inform agent about server capabilities
 */
uint32_t AgentConnection::setServerCapabilities()
{
   NXCPMessage msg(m_nProtocolVersion);
   uint32_t requestId = generateRequestId();
   msg.setCode(CMD_SET_SERVER_CAPABILITIES);
   msg.setField(VID_ENABLED, true);   // Enables IPv6 on pre-2.0 agents
   msg.setField(VID_IPV6_SUPPORT, true);
   msg.setField(VID_BULK_RECONCILIATION, true);
   msg.setField(VID_ENABLE_COMPRESSION, m_allowCompression);
   msg.setField(VID_ACCEPT_KEEPALIVE, true);
   msg.setId(requestId);
   if (!sendMessage(&msg))
      return ERR_CONNECTION_BROKEN;

   NXCPMessage *response = m_messageWaitQueue.waitForMessage(CMD_REQUEST_COMPLETED, requestId, m_commandTimeout);
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
   NXCPMessage *response = m_messageWaitQueue.waitForMessage(CMD_REQUEST_COMPLETED, requestId, timeout);
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
   shared_ptr<AbstractCommChannel> channel = acquireChannel();
   if (channel == nullptr)
      return false;

   if (nxlog_get_debug_level_tag_object(DEBUG_TAG, m_debugId) >= 6)
   {
      TCHAR codeName[64], ipAddrText[64];
      debugPrintf(6, _T("Sending message %s (%d) to agent at %s"),
         NXCPMessageCodeName(pMsg->getCode(), codeName), pMsg->getId(), m_addr.toString(ipAddrText));
   }

   bool success;
   NXCP_MESSAGE *rawMsg = pMsg->serialize(m_allowCompression);
	shared_ptr<NXCPEncryptionContext> encryptionContext = acquireEncryptionContext();
   if (encryptionContext != nullptr)
   {
      NXCP_ENCRYPTED_MESSAGE *encryptedMsg = encryptionContext->encryptMessage(rawMsg);
      if (encryptedMsg != nullptr)
      {
         success = (channel->send(encryptedMsg, ntohl(encryptedMsg->size), &m_mutexSocketWrite) == (int)ntohl(encryptedMsg->size));
         MemFree(encryptedMsg);
      }
      else
      {
         success = false;
      }
   }
   else
   {
      success = (channel->send(rawMsg, ntohl(rawMsg->size), &m_mutexSocketWrite) == (int)ntohl(rawMsg->size));
   }
   MemFree(rawMsg);
   return success;
}

/**
 * Callback for sending NXCP message in background
 */
void AgentConnection::postMessageCallback(NXCPMessage *msg)
{
   sendMessage(msg);
   delete msg;
}

/**
 * Send NXCP message in background. Provided message will be destroyed after sending.
 */
void AgentConnection::postMessage(NXCPMessage *msg)
{
   TCHAR key[64];
   _sntprintf(key, 64, _T("PostMessage_%p"), this);
   ThreadPoolExecuteSerialized(g_agentConnectionThreadPool, key, shared_from_this(), &AgentConnection::postMessageCallback, msg);
}

/**
 * Send raw message to agent
 */
bool AgentConnection::sendRawMessage(NXCP_MESSAGE *pMsg)
{
   shared_ptr<AbstractCommChannel> channel = acquireChannel();
   if (channel == nullptr)
      return false;

   if (nxlog_get_debug_level_tag_object(DEBUG_TAG, m_debugId) >= 6)
   {
      TCHAR codeName[64], ipAddrText[64];
      debugPrintf(6, _T("Sending raw message %s (%d) to agent at %s"),
         NXCPMessageCodeName(ntohs(pMsg->code), codeName), ntohl(pMsg->id), m_addr.toString(ipAddrText));
   }

   bool success;
   NXCP_MESSAGE *rawMsg = pMsg;
	shared_ptr<NXCPEncryptionContext> encryptionContext = acquireEncryptionContext();
   if (encryptionContext != nullptr)
   {
      NXCP_ENCRYPTED_MESSAGE *pEnMsg = encryptionContext->encryptMessage(rawMsg);
      if (pEnMsg != nullptr)
      {
         success = (channel->send(pEnMsg, ntohl(pEnMsg->size), &m_mutexSocketWrite) == (int)ntohl(pEnMsg->size));
         MemFree(pEnMsg);
      }
      else
      {
         success = false;
      }
   }
   else
   {
      success = (channel->send(rawMsg, ntohl(rawMsg->size), &m_mutexSocketWrite) == (int)ntohl(rawMsg->size));
   }
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
   ThreadPoolExecuteSerialized(g_agentConnectionThreadPool, key, shared_from_this(), &AgentConnection::postRawMessageCallback, msg);
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
 * Callback for processing file monitoring data on separate thread
 */
void AgentConnection::onFileMonitoringDataCallback(NXCPMessage *msg)
{
   onFileMonitoringData(msg);
   delete msg;
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
 * Callback for handling agent notification messages on a separate thread
 */
void AgentConnection::onNotifyCallback(NXCPMessage *msg)
{
   onNotify(msg);
   delete msg;
}

/**
 * Notification message handler. Should be implemented in derived classes
 * to implement actual processing. Default implementation does nothing.
 */
void AgentConnection::onNotify(NXCPMessage *msg)
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
   *list = nullptr;
   if (!m_isConnected)
      return ERR_NOT_CONNECTED;

   NXCPMessage msg(CMD_GET_LIST, generateRequestId(), m_nProtocolVersion);
   msg.setField(VID_PARAMETER, param);

   uint32_t rcc;
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

   return rcc;
}

/**
 * Get table
 */
uint32_t AgentConnection::getTable(const TCHAR *pszParam, Table **table)
{
   *table = nullptr;
   if (!m_isConnected)
      return ERR_NOT_CONNECTED;

   uint32_t requestId = generateRequestId();
   NXCPMessage msg(CMD_GET_TABLE, requestId, m_nProtocolVersion);
   msg.setField(VID_PARAMETER, pszParam);

   uint32_t rcc;
   if (sendMessage(&msg))
   {
      NXCPMessage *response = waitForMessage(CMD_REQUEST_COMPLETED, requestId, m_commandTimeout);
      if (response != nullptr)
      {
         rcc = response->getFieldAsUInt32(VID_RCC);
         if (rcc == ERR_SUCCESS)
         {
            *table = new Table(*response);
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
 * Authenticate to agent
 */
uint32_t AgentConnection::authenticate(BOOL bProxyData)
{
   const char *secret = bProxyData ? m_proxySecret : m_secret;
   if (*secret == 0)
      return ERR_SUCCESS;  // No authentication required

   uint32_t requestId = generateRequestId();
   NXCPMessage msg(CMD_AUTHENTICATE, requestId, m_nProtocolVersion);
   msg.setField(VID_AUTH_METHOD, static_cast<uint16_t>(AUTH_SHA1_HASH));  // For compatibility with agents before 3.3
   BYTE hash[SHA1_DIGEST_SIZE];
   CalculateSHA1Hash(reinterpret_cast<const BYTE*>(secret), strlen(secret), hash);
   msg.setField(VID_SHARED_SECRET, hash, SHA1_DIGEST_SIZE);
   if (!sendMessage(&msg))
      return ERR_CONNECTION_BROKEN;

   return waitForRCC(requestId, m_commandTimeout);
}

/**
 * Get time of remote system
 */
uint32_t AgentConnection::getRemoteSystemTime(int64_t *remoteTime, int32_t *offset, uint32_t *roundtripTime, bool *allowSync)
{
   NXCPMessage request(CMD_GET_SYSTEM_TIME, generateRequestId(), m_nProtocolVersion);

   int64_t sendTime = GetCurrentTimeMs();
   if (!sendMessage(&request))
      return ERR_CONNECTION_BROKEN;

   NXCPMessage *response = waitForMessage(CMD_REQUEST_COMPLETED, request.getId(), m_commandTimeout);
   int64_t now = GetCurrentTimeMs();

   if (response == nullptr)
      return ERR_REQUEST_TIMEOUT;

   uint32_t rcc = response->getFieldAsUInt32(VID_RCC);
   if (rcc == ERR_SUCCESS)
   {
      uint32_t rtt = static_cast<uint32_t>(now - sendTime);
      *remoteTime = response->getFieldAsInt64(VID_TIMESTAMP);
      if (offset != nullptr)
         *offset = static_cast<int32_t>(*remoteTime - (now - rtt / 2));
      if (roundtripTime != nullptr)
         *roundtripTime = rtt;
      if (allowSync != nullptr)
         *allowSync = response->getFieldAsBoolean(VID_TIME_SYNC_ALLOWED);
   }
   delete response;
   return rcc;
}

/**
 * Synchronize time with server
 */
uint32_t AgentConnection::synchronizeTime()
{
   int32_t avgOffset = 0;
   uint32_t avgRTT = 0;
   for(int i = 0; i < 5; i++)
   {
      int64_t remoteTime;
      int32_t offset;
      uint32_t rtt;
      uint32_t rcc = getRemoteSystemTime(&remoteTime, &offset, &rtt);
      if (rcc != ERR_SUCCESS)
         return rcc;
      avgOffset += offset;
      avgRTT += rtt;
   }
   avgOffset /= 5;
   avgRTT /= 5;
   if (abs(avgOffset) < 500)
      return ERR_SUCCESS;  // Time difference is too small to attempt such coarse sync

   NXCPMessage request(CMD_SET_SYSTEM_TIME, generateRequestId(), m_nProtocolVersion);
   request.setField(VID_ACCURACY, avgRTT / 2);
   request.setField(VID_TIMESTAMP, GetCurrentTimeMs());
   if (!sendMessage(&request))
      return ERR_CONNECTION_BROKEN;

   return waitForRCC(request.getId(), m_commandTimeout);
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
 * Context for file upload
 */
struct FileUploadContext
{
   AgentConnection *connection;
   std::function<void (size_t)> userCallback;
   time_t lastProbeTime;
   uint32_t messageCount;
};

/**
 * NXCP file upload progress callback
 */
static void FileUploadProgressCalback(size_t bytesTransferred, void *_context)
{
   auto context = static_cast<FileUploadContext*>(_context);

   context->messageCount++;
   time_t now = time(nullptr);
   if ((now - context->lastProbeTime > 5) || (context->messageCount > 8))
   {
      // Call nop() method on agent connection to send keepalive request
      // This will throttle file transfer (because server will wait for agent response) and
      // prevent timeout in background poller on server side
      context->connection->nop();
      context->messageCount = 0;
      context->lastProbeTime = now;
   }

   if (context->userCallback != nullptr)
      context->userCallback(bytesTransferred);
}

/**
 * Upload file to agent
 */
uint32_t AgentConnection::uploadFile(const TCHAR *localFile, const TCHAR *destinationFile, bool allowPathExpansion,
      std::function<void (size_t)> progressCallback, NXCPStreamCompressionMethod compMethod)
{
   if (!m_isConnected)
      return ERR_NOT_CONNECTED;

   // Disable compression if it is disabled on connection level or if agent do not support it
   if (!m_allowCompression || (m_nProtocolVersion < 4))
      compMethod = NXCP_STREAM_COMPRESSION_NONE;

   NXCPMessage msg(m_nProtocolVersion);
   uint32_t requestId = generateRequestId();
   msg.setId(requestId);

   time_t lastModTime = 0;
   NX_STAT_STRUCT st;
   if (CALL_STAT_FOLLOW_SYMLINK(localFile, &st) == 0)
   {
      lastModTime = st.st_mtime;
   }

   bool messageResendRequired;
   FileTransferResumeMode resumeMode = FileTransferResumeMode::CHECK;
   uint32_t rcc;
   uint64_t offset = 0;
   do
   {
      messageResendRequired = false;
      // Use core agent if destination file name is not set and file manager subagent otherwise
      if ((destinationFile == nullptr) || (*destinationFile == 0))
      {
         msg.setCode(CMD_TRANSFER_FILE);
         const TCHAR *fname = (destinationFile == nullptr) || (*destinationFile == 0) ? localFile : destinationFile;
         int i;
         for(i = (int)_tcslen(fname) - 1; (i >= 0) && (fname[i] != '\\') && (fname[i] != '/'); i--);
         msg.setField(VID_FILE_NAME, &fname[i + 1]);
      }
      else
      {
         msg.setCode(CMD_FILEMGR_UPLOAD);
         msg.setField(VID_OVERWRITE, true);
         msg.setField(VID_FILE_NAME, destinationFile);
         msg.setField(VID_ALLOW_PATH_EXPANSION, allowPathExpansion);
      }
      msg.setFieldFromTime(VID_MODIFICATION_TIME, lastModTime);
      msg.setField(VID_RESUME_MODE, static_cast<uint16_t>(resumeMode));

      if (sendMessage(&msg))
      {
         NXCPMessage *response = waitForMessage(CMD_REQUEST_COMPLETED, requestId, m_commandTimeout);
         if (response != nullptr)
         {
            rcc = response->getFieldAsInt32(VID_RCC);
            if (rcc == ERR_FILE_APPEND_POSSIBLE)
            {
               BYTE localHash[MD5_DIGEST_SIZE];
               BYTE remoteHash[MD5_DIGEST_SIZE];
               memset(localHash, 0, sizeof(localHash));
               memset(remoteHash, 0, sizeof(remoteHash));
               response->getFieldAsBinary(VID_HASH_MD5, remoteHash, MD5_DIGEST_SIZE);
               uint64_t remoteSize = response->getFieldAsUInt64(VID_FILE_SIZE);
               CalculateFileMD5Hash(localFile, localHash, remoteSize);
               if (!memcmp(remoteHash, localHash, MD5_DIGEST_SIZE))
               {
                  resumeMode = FileTransferResumeMode::RESUME;
                  messageResendRequired = true;
                  offset = remoteSize;
                  // Even if files are equals .part file might need to be renamed
               }
               else
               {
                  resumeMode = FileTransferResumeMode::OVERWRITE;
                  messageResendRequired = true;
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
   } while (messageResendRequired);

   if (rcc == ERR_SUCCESS)
   {
      shared_ptr<AbstractCommChannel> channel = acquireChannel();
      if (channel != nullptr)
      {
         debugPrintf(5, _T("Sending file \"%s\" to agent %s compression"),
                  localFile, (compMethod == NXCP_STREAM_COMPRESSION_NONE) ? _T("without") : _T("with"));
         m_fileUploadInProgress = true;
         shared_ptr<NXCPEncryptionContext> ctx = acquireEncryptionContext();

         FileUploadContext context;
         context.connection = this;
         context.userCallback = progressCallback;
         context.lastProbeTime = time(nullptr);
         context.messageCount = 0;

         if (SendFileOverNXCP(channel.get(), requestId, localFile, ctx.get(), offset, FileUploadProgressCalback, &context, &m_mutexSocketWrite, compMethod, nullptr))
         {
            rcc = waitForRCC(requestId, std::max(m_commandTimeout, static_cast<uint32_t>(30000)));  // Wait at least 30 seconds for file transfer confirmation
         }
         else
         {
            rcc = ERR_IO_FAILURE;
         }
         m_fileUploadInProgress = false;
      }
      else
      {
         rcc = ERR_CONNECTION_BROKEN;
      }
   }

   return rcc;
}

/**
 * Change file owner
 */
uint32_t AgentConnection::changeFileOwner(const TCHAR *file, bool allowPathExpansion, const TCHAR *newOwner, const TCHAR *newGroup)
{
   if (!m_isConnected)
      return ERR_NOT_CONNECTED;

   uint32_t requestId = generateRequestId();
   NXCPMessage msg(CMD_FILEMGR_CHOWN, requestId, m_nProtocolVersion);

   msg.setField(VID_FILE_NAME, file);
   msg.setField(VID_ALLOW_PATH_EXPANSION, allowPathExpansion);
   msg.setField(VID_USER_NAME, newOwner);
   msg.setField(VID_GROUP_NAME, newGroup);

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
 * Change file permissions
 */
uint32_t AgentConnection::changeFilePermissions(const TCHAR *file, bool allowPathExpansion, uint32_t permissions, const TCHAR *newOwner, const TCHAR *newGroup)
{
   if (!m_isConnected)
      return ERR_NOT_CONNECTED;

   uint32_t requestId = generateRequestId();
   NXCPMessage msg(CMD_FILEMGR_CHMOD, requestId, m_nProtocolVersion);

   msg.setField(VID_FILE_NAME, file);
   msg.setField(VID_ALLOW_PATH_EXPANSION, allowPathExpansion);
   msg.setField(VID_FILE_PERMISSIONS, permissions);

   //For Windows agents
   msg.setField(VID_USER_NAME, newOwner);
   msg.setField(VID_GROUP_NAME, newGroup);

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
 * Download file from agent
 */
uint32_t AgentConnection::downloadFile(const TCHAR *sourceFile, const TCHAR *destinationFile, bool allowPathExpansion,
      std::function<void (size_t)> progressCallback, NXCPStreamCompressionMethod compMethod)
{
   if (!m_isConnected)
      return ERR_NOT_CONNECTED;

   // Disable compression if it is disabled on connection level or if agent do not support it
   if (!m_allowCompression || (m_nProtocolVersion < 4))
      compMethod = NXCP_STREAM_COMPRESSION_NONE;

   NXCPMessage msg(CMD_GET_AGENT_FILE, generateRequestId(), m_nProtocolVersion);
   msg.setField(VID_FILE_NAME, sourceFile);
   msg.setField(VID_ALLOW_PATH_EXPANSION, allowPathExpansion);
   msg.setField(VID_FILE_OFFSET, 0);
   msg.setField(VID_ENABLE_COMPRESSION, compMethod != NXCP_STREAM_COMPRESSION_NONE);
   msg.setField(VID_COMPRESSION_METHOD, static_cast<uint16_t>(compMethod));

   NXCPMessage *response = customRequest(&msg, destinationFile, false, progressCallback, nullptr);
   uint32_t rcc;
   if (response != nullptr)
   {
      rcc = response->getFieldAsUInt32(VID_RCC);
      nxlog_debug_tag(DEBUG_TAG, 5, _T("Download request for file %s RCC=%u (%s)"), sourceFile, rcc, AgentErrorCodeToText(rcc));
      delete response;
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("Download request for file %s timeout"), sourceFile);
      rcc = ERR_REQUEST_TIMEOUT;
   }
   return rcc;
}

/**
 * Cancel file monitoring
 */
uint32_t AgentConnection::cancelFileMonitoring(const TCHAR *agentFileId)
{
   if (!m_isConnected)
      return ERR_NOT_CONNECTED;

   NXCPMessage request(CMD_CANCEL_FILE_MONITORING, generateRequestId(), m_nProtocolVersion);
   request.setField(VID_FILE_NAME, agentFileId);
   uint32_t rcc;
   if (sendMessage(&request))
   {
      NXCPMessage *response = waitForMessage(CMD_REQUEST_COMPLETED, request.getId(), m_commandTimeout);
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
 * Get file fingerprint
 */
uint32_t AgentConnection::getFileFingerprint(const TCHAR *file, RemoteFileFingerprint *fp)
{
   if (!m_isConnected)
      return ERR_NOT_CONNECTED;

   NXCPMessage request(CMD_FILEMGR_GET_FILE_FINGERPRINT, generateRequestId(), m_nProtocolVersion);
   request.setField(VID_FILE_NAME, file);

   uint32_t rcc;
   if (sendMessage(&request))
   {
      NXCPMessage *response = waitForMessage(CMD_REQUEST_COMPLETED, request.getId(), m_commandTimeout);
      if (response != nullptr)
      {
         rcc = response->getFieldAsUInt32(VID_RCC);
         if (rcc == ERR_SUCCESS)
         {
            fp->size = response->getFieldAsUInt64(VID_FILE_SIZE);
            fp->crc32 = static_cast<uint32_t>(response->getFieldAsUInt64(VID_HASH_CRC32));
            response->getFieldAsBinary(VID_HASH_MD5, fp->md5, MD5_DIGEST_SIZE);
            response->getFieldAsBinary(VID_HASH_SHA256, fp->sha256, SHA256_DIGEST_SIZE);
            const BYTE *data = response->getBinaryFieldPtr(VID_FILE_DATA, &fp->dataLength);
            if ((data != nullptr) && (fp->dataLength > 0))
            {
               memcpy(fp->data, data, fp->dataLength);
            }
            else
            {
               fp->dataLength = 0;
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
 * Send upgrade command
 */
uint32_t AgentConnection::startUpgrade(const TCHAR *pkgName)
{
   if (!m_isConnected)
      return ERR_NOT_CONNECTED;

   NXCPMessage request(CMD_UPGRADE_AGENT, generateRequestId(), m_nProtocolVersion);
   int i;
   for(i = (int)_tcslen(pkgName) - 1;
       (i >= 0) && (pkgName[i] != '\\') && (pkgName[i] != '/'); i--);
   request.setField(VID_FILE_NAME, &pkgName[i + 1]);

   uint32_t rcc;
   if (sendMessage(&request))
   {
      rcc = waitForRCC(request.getId(), m_commandTimeout);
   }
   else
   {
      rcc = ERR_CONNECTION_BROKEN;
   }
   return rcc;
}

/**
 * Send package installation command
 */
uint32_t AgentConnection::installPackage(const TCHAR *pkgName, const TCHAR *pkgType, const TCHAR *command)
{
   if (!m_isConnected)
      return ERR_NOT_CONNECTED;

   NXCPMessage request(CMD_INSTALL_PACKAGE, generateRequestId(), m_nProtocolVersion);
   int i;
   for(i = (int)_tcslen(pkgName) - 1; (i >= 0) && (pkgName[i] != '\\') && (pkgName[i] != '/'); i--);
   request.setField(VID_FILE_NAME, &pkgName[i + 1]);
   request.setField(VID_PACKAGE_TYPE, pkgType);
   request.setField(VID_COMMAND, command);

   uint32_t rcc;
   if (sendMessage(&request))
   {
      rcc = waitForRCC(request.getId(), m_commandTimeout);
   }
   else
   {
      rcc = ERR_CONNECTION_BROKEN;
   }
   return rcc;
}

/**
 * Check status of network service via agent
 */
uint32_t AgentConnection::checkNetworkService(uint32_t *status, const InetAddress& addr, int serviceType, uint16_t port,
         uint16_t proto, const TCHAR *serviceRequest, const TCHAR *serviceResponse, uint32_t *responseTime)
{
   if (!m_isConnected)
      return ERR_NOT_CONNECTED;

   static uint16_t defaultPort[] = { 7, 22, 110, 25, 21, 80, 443, 23 };

   NXCPMessage request(CMD_CHECK_NETWORK_SERVICE, generateRequestId(), m_nProtocolVersion);
   request.setField(VID_IP_ADDRESS, addr);
   request.setField(VID_SERVICE_TYPE, (WORD)serviceType);
   request.setField(VID_IP_PORT,
      (port != 0) ? port :
         defaultPort[((serviceType >= NETSRV_CUSTOM) &&
                         (serviceType <= NETSRV_TELNET)) ? serviceType : 0]);
   request.setField(VID_IP_PROTO, (proto != 0) ? proto : (uint16_t)IPPROTO_TCP);
   request.setField(VID_SERVICE_REQUEST, serviceRequest);
   request.setField(VID_SERVICE_RESPONSE, serviceResponse);

   uint32_t rcc;
   if (sendMessage(&request))
   {
      // Wait up to 90 seconds for results
      NXCPMessage *response = waitForMessage(CMD_REQUEST_COMPLETED, request.getId(), 90000);
      if (response != nullptr)
      {
         rcc = response->getFieldAsUInt32(VID_RCC);
         if (rcc == ERR_SUCCESS)
         {
            *status = response->getFieldAsUInt32(VID_SERVICE_STATUS);
            if (responseTime != nullptr)
            {
               *responseTime = response->getFieldAsUInt32(VID_RESPONSE_TIME);
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
 * Get list of supported parameters from agent
 */
uint32_t AgentConnection::getSupportedParameters(ObjectArray<AgentParameterDefinition> **paramList, ObjectArray<AgentTableDefinition> **tableList)
{
   *paramList = nullptr;
	*tableList = nullptr;

   if (!m_isConnected)
      return ERR_NOT_CONNECTED;

   uint32_t requestId = generateRequestId();
   NXCPMessage msg(CMD_GET_PARAMETER_LIST, requestId, m_nProtocolVersion);

   uint32_t rcc;
   if (sendMessage(&msg))
   {
      NXCPMessage *response = waitForMessage(CMD_REQUEST_COMPLETED, requestId, m_commandTimeout);
      if (response != nullptr)
      {
         rcc = response->getFieldAsUInt32(VID_RCC);
			debugPrintf(6, _T("AgentConnection::getSupportedParameters(): RCC=%d"), rcc);
         if (rcc == ERR_SUCCESS)
         {
            uint32_t count = response->getFieldAsUInt32(VID_NUM_PARAMETERS);
            ObjectArray<AgentParameterDefinition> *plist = new ObjectArray<AgentParameterDefinition>(count, 16, Ownership::True);
            for(uint32_t i = 0, id = VID_PARAM_LIST_BASE; i < count; i++)
            {
               plist->add(new AgentParameterDefinition(response, id));
               id += 3;
            }
				*paramList = plist;
				debugPrintf(6, _T("AgentConnection::getSupportedParameters(): %d parameters received from agent"), count);

            count = response->getFieldAsUInt32(VID_NUM_TABLES);
            ObjectArray<AgentTableDefinition> *tlist = new ObjectArray<AgentTableDefinition>(count, 16, Ownership::True);
            for(uint32_t i = 0, id = VID_TABLE_LIST_BASE; i < count; i++)
            {
               tlist->add(new AgentTableDefinition(response, id));
               id += 3;
            }
				*tableList = tlist;
				debugPrintf(6, _T("AgentConnection::getSupportedParameters(): %d tables received from agent"), count);
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
 * Setup encryption
 */
uint32_t AgentConnection::setupEncryption(RSA_KEY serverKey)
{
   uint32_t requestId = generateRequestId();
   NXCPMessage msg(m_nProtocolVersion);
   msg.setId(requestId);
   PrepareKeyRequestMsg(&msg, serverKey, false);

   uint32_t result;
   if (sendMessage(&msg))
   {
      NXCPMessage *response = waitForMessage(CMD_SESSION_KEY, requestId, m_commandTimeout);
      if (response != nullptr)
      {
         NXCPEncryptionContext *encryptionContext = nullptr;
         uint32_t rcc = SetupEncryptionContext(response, &encryptionContext, nullptr, serverKey, m_nProtocolVersion);
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

            *content = WideStringFromUTF8String((char *)utf8Text);
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
   char *utf8content = UTF8StringFromWideString(content);
   msg.setField(VID_CONFIG_FILE, (BYTE *)utf8content, strlen(utf8content));
   MemFree(utf8content);

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
shared_ptr<RoutingTable> AgentConnection::getRoutingTable(size_t limit)
{
   StringList *data;
   if (getList(_T("Net.IP.RoutingTable"), &data) != ERR_SUCCESS)
      return nullptr;

   if ((limit != 0) && (data->size() > limit))
   {
      delete data;
      return nullptr;  // Routing table is too big
   }

   auto routingTable = make_shared<RoutingTable>(data->size());
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
         route.destination = InetAddress::parse(pBuf);
         route.destination.setMaskBits(_tcstol(pSlash, nullptr, 10));
         pBuf = pChar + 1;
      }

      // Next hop address
      pChar = _tcschr(pBuf, _T(' '));
      if (pChar != nullptr)
      {
         *pChar = 0;
         route.nextHop = InetAddress::parse(pBuf);
         pBuf = pChar + 1;
      }

      // Interface index
      pChar = _tcschr(pBuf, ' ');
      if (pChar != nullptr)
      {
         *pChar = 0;
         route.ifIndex = _tcstoul(pBuf, nullptr, 10);
         pBuf = pChar + 1;
      }

      // Route type (maybe followed by optional metric)
      pChar = _tcschr(pBuf, ' ');
      if (pChar != nullptr)
      {
         *pChar = 0;
         route.routeType = _tcstoul(pBuf, nullptr, 10);
         pBuf = pChar + 1;

         // Metric
         route.routeType = _tcstoul(pBuf, nullptr, 10);
      }
      else
      {
         route.routeType = _tcstoul(pBuf, nullptr, 10);
      }

      routingTable->add(route);
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
      wchar_to_utf8(secret, -1, m_proxySecret, MAX_SECRET_LENGTH);
      m_proxySecret[MAX_SECRET_LENGTH - 1] = 0;
      DecryptPasswordA("netxms", m_proxySecret, m_proxySecret, MAX_SECRET_LENGTH);
   }
   else
   {
      m_proxySecret[0] = 0;
   }
   m_useProxy = true;

   TCHAR buffer[64];
   debugPrintf(5, _T("New proxy settings: address=%s port=%u"), addr.toString(buffer), port);
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
uint32_t AgentConnection::enableTraps()
{
   NXCPMessage request(CMD_ENABLE_AGENT_TRAPS, generateRequestId(), m_nProtocolVersion);
   if (!sendMessage(&request))
      return ERR_CONNECTION_BROKEN;
   return waitForRCC(request.getId(), m_commandTimeout);
}

/**
 * Enable trap receiving on connection
 */
uint32_t AgentConnection::enableFileUpdates()
{
   NXCPMessage request(CMD_ENABLE_FILE_UPDATES, generateRequestId(), m_nProtocolVersion);
   if (!sendMessage(&request))
      return ERR_CONNECTION_BROKEN;

   uint32_t rcc = waitForRCC(request.getId(), m_commandTimeout);
   if (rcc == ERR_SUCCESS)
      m_fileUpdateConnection = true;
   return rcc;
}

/**
 * Set activation token for agent component
 */
uint32_t AgentConnection::setComponentToken(const char *component, uint32_t expirationTime, const char *secret)
{
   debugPrintf(5, _T("Issuing component token (component=%hs, expiration=%u)"), component, expirationTime);

   AgentComponentToken token;
   memset(&token, 0, sizeof(token));
   strlcpy(token.component, component, sizeof(token.component));
   time_t now = time(nullptr);
   token.expirationTime = htonq(now + expirationTime);
   token.issuingTime = htonq(now);
   SignMessage(&token, sizeof(token) - sizeof(token.hmac), reinterpret_cast<const BYTE*>(secret), strlen(secret), token.hmac);

   NXCPMessage request(CMD_SET_COMPONENT_TOKEN, generateRequestId(), m_nProtocolVersion);
   request.setField(VID_TOKEN, reinterpret_cast<BYTE*>(&token), sizeof(token));
   if (!sendMessage(&request))
      return ERR_CONNECTION_BROKEN;
   return waitForRCC(request.getId(), m_commandTimeout);
}

/**
 * Take screenshot from remote system
 */
uint32_t AgentConnection::takeScreenshot(const TCHAR *sessionName, BYTE **data, size_t *size)
{
   NXCPMessage request(CMD_TAKE_SCREENSHOT, generateRequestId(), m_nProtocolVersion);
   request.setField(VID_NAME, sessionName);
   if (sendMessage(&request))
   {
      NXCPMessage *response = waitForMessage(CMD_REQUEST_COMPLETED, request.getId(), m_commandTimeout);
      if (response != nullptr)
      {
         uint32_t rcc = response->getFieldAsUInt32(VID_RCC);
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
NXCPMessage *AgentConnection::customRequest(NXCPMessage *request, const TCHAR *recvFile, bool append, std::function<void (size_t)> downloadProgressCallback, std::function<void (NXCPMessage*)> fileResendCallback)
{
	NXCPMessage *msg = nullptr;

   uint32_t requestId = generateRequestId();
   request->setId(requestId);
	if (recvFile != nullptr)
	{
	   uint32_t rcc = prepareFileDownload(recvFile, requestId, append, downloadProgressCallback, fileResendCallback);
		if (rcc != ERR_SUCCESS)
		{
			// Create fake response message
			msg = new NXCPMessage(CMD_REQUEST_COMPLETED, requestId);
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
            while(true)
            {
               if (m_condFileDownload.wait(120000))	 // 120 seconds inactivity timeout
               {
                  if (!m_fileDownloadSucceeded)
                  {
                     msg->setField(VID_RCC, ERR_IO_FAILURE);
                     if (m_deleteFileOnDownloadFailure)
                        _tremove(recvFile);
                  }
                  break;
               }

               // Check if server didn't receive any updates from agent within 300 seconds
               if (time(nullptr) - m_downloadActivityTimestamp > 300)
               {
                  msg->setField(VID_RCC, ERR_REQUEST_TIMEOUT);
                  break;
               }
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
   msg.setField(VID_REQUEST_ID, m_downloadRequestId);

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
uint32_t AgentConnection::prepareFileDownload(const TCHAR *fileName, uint32_t rqId, bool append, std::function<void (size_t)> downloadProgressCallback, std::function<void (NXCPMessage*)> fileResendCallback)
{
   if (fileResendCallback == nullptr)
   {
      if (m_hCurrFile != -1)
      {
         debugPrintf(4, _T("AgentConnection::PrepareFileDownload(\"%s\", append=%s, rqId=%u): another download is in progress"), fileName, BooleanToString(append), rqId);
         return ERR_RESOURCE_BUSY;
      }

      _tcslcpy(m_currentFileName, fileName, MAX_PATH);
      m_condFileDownload.reset();
      m_hCurrFile = _topen(fileName, (append ? 0 : (O_CREAT | O_TRUNC)) | O_RDWR | O_BINARY, S_IREAD | S_IWRITE);
      if (m_hCurrFile == -1)
      {
         debugPrintf(4, _T("AgentConnection::PrepareFileDownload(\"%s\", append=%s, rqId=%u): cannot open file (%s)"), fileName, BooleanToString(append), rqId, _tcserror(errno));
      }
      else
      {
         if (append)
            _lseek(m_hCurrFile, 0, SEEK_END);
      }

      m_downloadRequestId = rqId;
      m_downloadActivityTimestamp = time(nullptr);
      m_downloadProgressCallback = downloadProgressCallback;

      m_sendToClientMessageCallback = nullptr;

      return (m_hCurrFile != -1) ? ERR_SUCCESS : ERR_FILE_OPEN_ERROR;
   }
   else
   {
      m_condFileDownload.reset();

      m_downloadRequestId = rqId;
      m_downloadActivityTimestamp = time(nullptr);
      m_downloadProgressCallback = downloadProgressCallback;

      m_sendToClientMessageCallback = fileResendCallback;

      return ERR_SUCCESS;
   }
}

/**
 * Process incoming file data
 */
void AgentConnection::processFileData(NXCPMessage *msg)
{
   m_downloadActivityTimestamp = time(nullptr);
   if (m_sendToClientMessageCallback != nullptr)
   {
      m_sendToClientMessageCallback(msg);
      if (msg->isEndOfFile())
      {
         m_sendToClientMessageCallback = nullptr;
         onFileDownload(true);
      }
      else
      {
         if (m_downloadProgressCallback != nullptr)
         {
            m_downloadProgressCallback(msg->getBinaryDataSize());
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
               m_downloadProgressCallback(_tell(m_hCurrFile));
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
      m_sendToClientMessageCallback(msg);
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
	m_condFileDownload.set();
}

/**
 * Enable trap receiving on connection
 */
uint32_t AgentConnection::getPolicyInventory(AgentPolicyInfo **info)
{
   NXCPMessage request(CMD_GET_POLICY_INVENTORY, generateRequestId(), m_nProtocolVersion);

	*info = nullptr;
   uint32_t rcc;
   if (sendMessage(&request))
	{
		NXCPMessage *response = waitForMessage(CMD_REQUEST_COMPLETED, request.getId(), m_commandTimeout);
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
uint32_t AgentConnection::uninstallPolicy(const uuid& guid)
{
	NXCPMessage request(CMD_UNINSTALL_AGENT_POLICY, generateRequestId(), m_nProtocolVersion);
	request.setField(VID_GUID, guid);
	if (!sendMessage(&request))
	   return ERR_CONNECTION_BROKEN;
	return waitForRCC(request.getId(), m_commandTimeout);
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
uint32_t AgentConnection::processCollectedData(NXCPMessage *msg)
{
   return ERR_NOT_IMPLEMENTED;
}

/**
 * Process collected data information in bulk mode (for DCI with agent-side cache)
 */
uint32_t AgentConnection::processBulkCollectedData(NXCPMessage *request, NXCPMessage *response)
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
uint32_t AgentConnection::setupTcpProxy(const InetAddress& ipAddr, uint16_t port, uint32_t *channelId)
{
   uint32_t requestId = generateRequestId();
   *channelId = requestId;

   NXCPMessage msg(CMD_SETUP_TCP_PROXY, requestId, m_nProtocolVersion);
   msg.setField(VID_IP_ADDRESS, ipAddr);
   msg.setField(VID_PORT, port);
   msg.setField(VID_CHANNEL_ID, requestId);  // Assign channel ID from server side (agents before 4.5.3 will ignore it)

   uint32_t rcc;
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
uint32_t AgentConnection::closeTcpProxy(uint32_t channelId)
{
   uint32_t requestId = generateRequestId();
   NXCPMessage msg(CMD_CLOSE_TCP_PROXY, requestId, m_nProtocolVersion);
   msg.setField(VID_CHANNEL_ID, channelId);
   uint32_t rcc;
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
void AgentConnection::processTcpProxyData(uint32_t channelId, const void *data, size_t size, bool errorIndicator)
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
 * Get list of user sessions
 */
uint32_t AgentConnection::getUserSessions(ObjectArray<UserSession> **sessions)
{
   *sessions = nullptr;
   if (!m_isConnected)
      return ERR_NOT_CONNECTED;

   bool hasSessionAgentInfo = false;

   Table *table;
   uint32_t rcc = getTable(_T("System.ActiveUserSessions"), &table);
   if (rcc == ERR_SUCCESS)
   {
      *sessions = new ObjectArray<UserSession>(table->getNumRows(), 16, Ownership::True);

      int cId = table->getColumnIndex(_T("ID"));
      int cUserName = table->getColumnIndex(_T("USER_NAME"));
      int cTerminal = table->getColumnIndex(_T("TERMINAL"));
      int cState = table->getColumnIndex(_T("STATE"));
      int cClientName = table->getColumnIndex(_T("CLIENT_NAME"));
      int cClientAddress = table->getColumnIndex(_T("CLIENT_ADDRESS"));
      int cDisplay = table->getColumnIndex(_T("CLIENT_DISPLAY"));
      int cConnectTime = table->getColumnIndex(_T("CONNECT_TIMESTAMP"));
      int cLogonTime = table->getColumnIndex(_T("LOGON_TIMESTAMP"));
      int cIdleTime = table->getColumnIndex(_T("IDLE_TIME"));
      int cAgentType = table->getColumnIndex(_T("AGENT_TYPE"));
      int cAgentPid = table->getColumnIndex(_T("AGENT_PID"));

      hasSessionAgentInfo = (cAgentType != -1);

      for(int i = 0; i < table->getNumRows(); i++)
      {
         UserSession *session = new UserSession();
         session->id = table->getAsUInt(i, cId);
         session->loginName = table->getAsString(i, cUserName, _T(""));
         session->terminal = table->getAsString(i, cTerminal, _T(""));
         session->connected = _tcsicmp(table->getAsString(i, cState, _T("")), _T("Active")) == 0;
         session->clientName = table->getAsString(i, cClientName, _T(""));
         session->connectTime = static_cast<time_t>(table->getAsInt64(i, cConnectTime));
         session->loginTime = static_cast<time_t>(table->getAsInt64(i, cLogonTime));
         session->idleTime = static_cast<time_t>(table->getAsInt64(i, cIdleTime));

         if (cAgentType != -1)
         {
            session->agentType = table->getAsInt(i, cAgentType);
            session->agentPID = table->getAsInt(i, cAgentPid);
         }

         const TCHAR *addr = table->getAsString(i, cClientAddress, _T(""));
         if (*addr != 0)
            session->clientAddress = InetAddress::parse(addr);

         TCHAR displayInfo[128];
         _tcslcpy(displayInfo, table->getAsString(i, cDisplay, _T("")), 128);
         if (displayInfo[0] != 0)
         {
            int values[3];
            memset(values, 0, sizeof(values));
            TCHAR *e = displayInfo;
            for(int j = 0; j < 3; j++)
            {
               TCHAR *p = _tcschr(e, _T('x'));
               if (p != nullptr)
                  *p = 0;
               TCHAR *eptr;
               values[j] = _tcstol(e, &eptr, 10);
               if ((*eptr != 0) || (p == nullptr))
                  break;
               e = p + 1;
            }
            if ((values[0] != 0) && (values[1] != 0) && (values[2] != 0))
            {
               session->displayWidth = values[0];
               session->displayHeight = values[1];
               session->displayColorDepth = values[2];
            }
         }

         (*sessions)->add(session);
      }

      delete table;
   }
   else if (rcc == ERR_UNKNOWN_METRIC)
   {
      StringList *list;
      rcc = getList(_T("System.ActiveUserSessions"), &list);
      if (rcc == ERR_SUCCESS)
      {
         *sessions = new ObjectArray<UserSession>(list->size(), 16, Ownership::True);

         // Each line has format "login" "terminal" "address"
         const char *eptr;
         int eoffset;
         PCRE *re = _pcre_compile_t(reinterpret_cast<const PCRE_TCHAR*>(_T("^\"([^\"]*)\" \"([^\"]*)\" \"([^\"]*)\"$")), PCRE_COMMON_FLAGS | PCRE_CASELESS, &eptr, &eoffset, nullptr);
         if (re != nullptr)
         {
            int pmatch[30];
            for(int i = 0; i < list->size(); i++)
            {
               const TCHAR *entry = list->get(i);
               if (_pcre_exec_t(re, nullptr, reinterpret_cast<const PCRE_TCHAR*>(entry), static_cast<int>(_tcslen(entry)), 0, 0, pmatch, 30) == 4)
               {
                  UserSession *session = new UserSession();
                  session->id = i;
                  session->loginName = String(&entry[pmatch[2]], pmatch[3] - pmatch[2]);
                  session->terminal = String(&entry[pmatch[4]], pmatch[5] - pmatch[4]);

                  String addr(&entry[pmatch[6]], pmatch[7] - pmatch[6]);
                  if (!addr.isEmpty())
                     session->clientAddress = InetAddress::parse(addr);

                  (*sessions)->add(session);
               }
            }
            _pcre_free_t(re);
         }

         delete list;
      }
   }

   if ((rcc == ERR_SUCCESS) && !hasSessionAgentInfo && (getTable(_T("Agent.SessionAgents"), &table) == ERR_SUCCESS))
   {
      int cId = table->getColumnIndex(_T("SESSION_ID"));
      int cAgentType = table->getColumnIndex(_T("AGENT_TYPE"));
      int cAgentPid = table->getColumnIndex(_T("AGENT_PID"));
      for(int i = 0; i < table->getNumRows(); i++)
      {
         uint32_t sid = table->getAsUInt(i, cId);

         UserSession *session = nullptr;
         for(int j = 0; j < (*sessions)->size(); j++)
         {
            UserSession *s = (*sessions)->get(j);
            if (s->id == sid)
            {
               session = s;
               break;
            }
         }

         if (session != nullptr)
         {
            session->agentPID = table->getAsUInt(i, cAgentPid);
            session->agentType = table->getAsUInt(i, cAgentType);
         }
      }
      delete table;
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
   if ((m_description == nullptr) || (*m_description == 0))
   {
      MemFree(m_description);
      m_description = MemCopyString(m_name);
   }
   m_dataType = msg->getFieldAsInt16(baseId + 2);
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
   m_description = ((description != nullptr) && (*description != 0)) ? MemCopyString(description) : MemCopyString(name);
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
      m_permissions = msg->getFieldAsUInt32(baseId + 4);
      m_ownerUser = msg->getFieldAsString(baseId + 5);
      m_ownerGroup = msg->getFieldAsString(baseId + 6);
   }
   else
   {
      m_size = 0;
      m_mtime = 0;
      memset(m_hash, 0, MD5_DIGEST_SIZE);
      m_permissions = 0;
      m_ownerUser = nullptr;
      m_ownerGroup = nullptr;
   }
}

/**
 * Destroy remote file info object
 */
RemoteFileInfo::~RemoteFileInfo()
{
   MemFree(m_name);
   MemFree(m_ownerUser);
   MemFree(m_ownerGroup);
}
