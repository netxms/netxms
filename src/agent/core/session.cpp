/*
** NetXMS multiplatform core agent
** Copyright (C) 2003-2022 Raden Solutions
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
** File: session.cpp
**
**/

#include "nxagentd.h"
#include <nxstat.h>

/**
 * Externals
 */
void UnregisterSession(uint32_t id);
uint32_t DeployPolicy(NXCPMessage *request, uint64_t serverId, const TCHAR *serverInfo);
uint32_t UninstallPolicy(NXCPMessage *request);
uint32_t GetPolicyInventory(NXCPMessage *msg, uint64_t serverId);
void ClearDataCollectionConfiguration();
uint32_t AddUserAgentNotification(uint64_t serverId, NXCPMessage *request);
uint32_t RemoveUserAgentNotification(uint64_t serverId, NXCPMessage *request);
uint32_t UpdateUserAgentNotifications(uint64_t serverId, NXCPMessage *request);
void RegisterSessionForNotifications(const shared_ptr<CommSession>& session);

extern VolatileCounter g_authenticationFailures;

/**
 * Communication request processing thread pool
 */
ThreadPool *g_commThreadPool = nullptr;

/**
 * Agent action thread pool
 */
ThreadPool *g_executorThreadPool = nullptr;

/**
 * Agent background action thread pool
 */
ThreadPool *g_backgroundThreadPool = nullptr;

/**
 * Web service collector thread pool
 */
ThreadPool *g_webSvcThreadPool = nullptr;

/**
 * Next free session ID
 */
static VolatileCounter s_sessionId = 0;

/**
 * Agent proxy statistics
 */
static VolatileCounter64 s_proxyConnectionRequests = 0;
static VolatileCounter s_activeProxySessions = 0;
static VolatileCounter64 s_tcpProxyConnectionRequests = 0;

/**
 * Handler for agent proxy stats parameters
 */
LONG H_AgentProxyStats(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   switch(*arg)
   {
      case 'A':
         ret_uint(value, static_cast<uint32_t>(s_activeProxySessions));
         break;
      case 'C':
         ret_uint64(value, static_cast<uint64_t>(s_proxyConnectionRequests));
         break;
      case 'T':
         ret_uint64(value, static_cast<uint64_t>(s_tcpProxyConnectionRequests));
         break;
      default:
         return SYSINFO_RC_UNSUPPORTED;
   }
   return SYSINFO_RC_SUCCESS;
}

/**
 * Client session class constructor
 */
CommSession::CommSession(const shared_ptr<AbstractCommChannel>& channel, const InetAddress &serverAddr, bool masterServer, bool controlServer) :
         m_channel(channel), m_downloadFileMap(Ownership::True), m_socketWriteMutex(MutexType::FAST), m_tcpProxyLock(MutexType::FAST),
         m_tcpProxies(0, 16, Ownership::True), m_responseConditionMap(Ownership::True)
{
   m_id = InterlockedIncrement(&s_sessionId);
   m_index = INVALID_INDEX;
   _sntprintf(m_key, 32, _T("CommSession-%u"), m_id);
   _sntprintf(m_debugTag, 16, _T("comm.cs.%u"), m_id);
   m_processingQueue = new ObjectQueue<NXCPMessage>(64, Ownership::True);
   m_protocolVersion = NXCP_VERSION;
   m_hProxySocket = INVALID_SOCKET;
   m_processingThread = INVALID_THREAD_HANDLE;
   m_proxyReadThread = INVALID_THREAD_HANDLE;
   m_tcpProxyReadThread = INVALID_THREAD_HANDLE;
   m_serverId = 0;
   m_serverAddr = serverAddr;
   m_authenticated = (g_dwFlags & AF_REQUIRE_AUTH) ? false : true;
   m_masterServer = masterServer;
   m_controlServer = controlServer;
   m_proxyConnection = false;
   m_acceptTraps = false;
   m_acceptData = false;
   m_acceptFileUpdates = false;
   m_ipv6Aware = false;
   m_bulkReconciliationSupported = false;
   m_disconnected = false;
   m_allowCompression = false;
   m_acceptKeepalive = false;
   m_ts = time(nullptr);
   m_responseQueue = new MsgWaitQueue();
   m_requestId = 0;
}

/**
 * Callback for aborting active file transfers
 */
static EnumerationCallbackResult AbortFileTransfer(const uint32_t& key, DownloadFileInfo *file, CommSession *session)
{
   session->debugPrintf(4, _T("Transfer of file %s aborted because of session termination"), file->getFileName());
   file->close(false);
   return _CONTINUE;
}

/**
 * Destructor
 */
CommSession::~CommSession()
{
   if (m_proxyConnection)
      InterlockedDecrement(&s_activeProxySessions);

   m_channel->shutdown();
   if (m_hProxySocket != INVALID_SOCKET)
      closesocket(m_hProxySocket);
   m_disconnected = true;

   delete m_processingQueue;
   delete m_responseQueue;

   m_downloadFileMap.forEach(AbortFileTransfer, this);
}

/**
 * Debug print in session context
 */
void CommSession::debugPrintf(int level, const TCHAR *format, ...)
{
   va_list args;
   va_start(args, format);
   nxlog_debug_tag2(m_debugTag, level, format, args);
   va_end(args);
}

/**
 * Write log in session context
 */
void CommSession::writeLog(int16_t severity, const TCHAR *format, ...)
{
   va_list args;
   va_start(args, format);
   nxlog_write_tag2(severity, m_debugTag, format, args);
   va_end(args);
}

/**
 * Start all threads
 */
void CommSession::run()
{
   m_processingThread = ThreadCreateEx(this, &CommSession::processingThread);
   ThreadCreate(self(), &CommSession::readThread);
}

/**
 * Disconnect session
 */
void CommSession::disconnect()
{
	debugPrintf(5, _T("CommSession::disconnect()"));
   m_tcpProxyLock.lock();
   m_tcpProxies.clear();
   m_tcpProxyLock.unlock();
   m_channel->shutdown();
   if (m_hProxySocket != -1)
      shutdown(m_hProxySocket, SHUT_RDWR);
   m_disconnected = true;
}

/**
 * Reading thread
 */
void CommSession::readThread()
{
   CommChannelMessageReceiver receiver(m_channel, 4096, MAX_AGENT_MSG_SIZE);
   while(!m_disconnected)
   {
      if (!m_proxyConnection)
      {
         MessageReceiverResult result;
         NXCPMessage *msg = receiver.readMessage((g_dwIdleTimeout + 1) * 1000, &result);

         // Check for decryption error
         if (result == MSGRECV_DECRYPTION_FAILURE)
         {
            debugPrintf(4, _T("Unable to decrypt received message"));
            continue;
         }

         // Check for timeout
         if (result == MSGRECV_TIMEOUT)
         {
            if (m_ts < time(nullptr) - (time_t)g_dwIdleTimeout)
            {
               debugPrintf(5, _T("Session disconnected by timeout (last activity timestamp is %d)"), (int)m_ts);
               break;
            }
            continue;
         }

         // Receive error
         if (msg == nullptr)
         {
            if (result == MSGRECV_CLOSED)
               debugPrintf(5, _T("Communication channel closed by peer"));
            else
               debugPrintf(5, _T("Message receiving error (%s)"), AbstractMessageReceiver::resultToText(result));
            break;
         }

         // Update activity timestamp
         m_ts = time(nullptr);

         if (nxlog_get_debug_level() >= 8)
         {
            String msgDump = NXCPMessage::dump(receiver.getRawMessageBuffer(), m_protocolVersion);
            debugPrintf(8, _T("Message dump:\n%s"), msgDump.cstr());
         }

         if (msg->isBinary())
         {
            TCHAR buffer[64];
            debugPrintf(6, _T("Received raw message %s"), NXCPMessageCodeName(msg->getCode(), buffer));

            if (msg->getCode() == CMD_FILE_DATA)
            {
               DownloadFileInfo *dInfo = m_downloadFileMap.get(msg->getId());
               if (dInfo != nullptr)
               {
                  if (dInfo->write(msg->getBinaryData(), msg->getBinaryDataSize(), msg->isCompressedStream()))
                  {
                     if (msg->isEndOfFile())
                     {
                        debugPrintf(4, _T("Transfer of file %s completed"), dInfo->getFileName());
                        dInfo->close(true);
                        m_downloadFileMap.remove(msg->getId());

                        NXCPMessage response(CMD_REQUEST_COMPLETED, msg->getId(), m_protocolVersion);
                        response.setField(VID_RCC, ERR_SUCCESS);
                        sendMessage(response);
                     }
                  }
                  else
                  {
                     debugPrintf(4, _T("Transfer of file %s aborted because of local I/O error (%s)"),
                           dInfo->getFileName(), _tcserror(errno));
                     dInfo->close(false);
                     m_downloadFileMap.remove(msg->getId());

                     NXCPMessage response(CMD_REQUEST_COMPLETED, msg->getId(), m_protocolVersion);
                     response.setField(VID_RCC, ERR_IO_FAILURE);
                     sendMessage(&response);
                  }
               }
            }
            else if (msg->getCode() == CMD_TCP_PROXY_DATA)
            {
               uint32_t proxyId = msg->getId();
               m_tcpProxyLock.lock();
               for(int i = 0; i < m_tcpProxies.size(); i++)
               {
                  TcpProxy *p = m_tcpProxies.get(i);
                  if (p->getId() == proxyId)
                  {
                     p->writeSocket(msg->getBinaryData(), msg->getBinaryDataSize());
                     break;
                  }
               }
               m_tcpProxyLock.unlock();
            }
            delete msg;
         }
         else if (msg->isControl())
         {
            TCHAR buffer[64];
            debugPrintf(6, _T("Received control message %s"), NXCPMessageCodeName(msg->getCode(), buffer));

            if (msg->getCode() == CMD_GET_NXCP_CAPS)
            {
               uint32_t peerNXCPVersion = msg->getEncodedProtocolVersion(); // Before NXCP version 5 encoded version will be 0, assume version 4
               m_protocolVersion = (peerNXCPVersion == 0) ? 4 : MIN(peerNXCPVersion, NXCP_VERSION);
               debugPrintf(4, _T("Using protocol version %d"), m_protocolVersion);

               NXCP_MESSAGE *response = (NXCP_MESSAGE *)MemAlloc(NXCP_HEADER_SIZE);
               response->id = htonl(msg->getId());
               response->code = htons((uint16_t)CMD_NXCP_CAPS);
               response->flags = htons(MF_CONTROL | MF_NXCP_VERSION(m_protocolVersion));
               response->numFields = htonl(m_protocolVersion << 24);
               response->size = htonl(NXCP_HEADER_SIZE);
               sendRawMessage(response, m_encryptionContext.get());
            }
            delete msg;
         }
         else
         {
            TCHAR buffer[64];
            debugPrintf(6, _T("Received message %s (%d)"), NXCPMessageCodeName(msg->getCode(), buffer), msg->getId());

            uint32_t rcc;
            switch(msg->getCode())
            {
               case CMD_REQUEST_COMPLETED:
                  m_responseQueue->put(msg);
                  break;
               case CMD_REQUEST_SESSION_KEY:
                  if (m_encryptionContext == nullptr)
                  {
                     NXCPEncryptionContext *encryptionContext = nullptr;
                     NXCPMessage *response = nullptr;
                     SetupEncryptionContext(msg, &encryptionContext, &response, nullptr, m_protocolVersion);
                     m_encryptionContext = shared_ptr<NXCPEncryptionContext>(encryptionContext);
                     sendMessage(response);
                     delete response;
                     receiver.setEncryptionContext(m_encryptionContext);
                  }
                  delete msg;
                  break;
               case CMD_SETUP_PROXY_CONNECTION:
                  InterlockedIncrement64(&s_proxyConnectionRequests);
                  rcc = setupProxyConnection(msg);
                  // When proxy session established incoming messages will
                  // not be processed locally. Acknowledgment message sent
                  // by setupProxyConnection() in case of success.
                  if (rcc == ERR_SUCCESS)
                  {
                     InterlockedIncrement(&s_activeProxySessions);
                     m_processingQueue->clear();
                     m_processingQueue->setShutdownMode();
                  }
                  else
                  {
                     NXCPMessage response(CMD_REQUEST_COMPLETED, msg->getId(), m_protocolVersion);
                     response.setField(VID_RCC, rcc);
                     sendMessage(&response);
                  }
                  delete msg;
                  break;
               case CMD_SNMP_REQUEST:
                  if (m_masterServer && (g_dwFlags & AF_ENABLE_SNMP_PROXY))
                  {
                     proxySnmpRequest(msg);
                  }
                  else
                  {
                     NXCPMessage response(CMD_REQUEST_COMPLETED, msg->getId(), m_protocolVersion);
                     response.setField(VID_RCC, ERR_ACCESS_DENIED);
                     sendMessage(&response);
                     delete msg;
                  }
                  break;
               case CMD_QUERY_WEB_SERVICE:
                  if (g_dwFlags & AF_ENABLE_WEBSVC_PROXY)
                  {
                     queryWebService(msg);
                  }
                  else
                  {
                     NXCPMessage response(CMD_REQUEST_COMPLETED, msg->getId(), m_protocolVersion);
                     response.setField(VID_RCC, ERR_ACCESS_DENIED);
                     sendMessage(&response);
                     delete msg;
                  }
                  break;
               case CMD_WEB_SERVICE_CUSTOM_REQUEST:
                  if (g_dwFlags & AF_ENABLE_WEBSVC_PROXY)
                  {
                     webServiceCustomRequest(msg);
                  }
                  else
                  {
                     NXCPMessage response(CMD_REQUEST_COMPLETED, msg->getId(), m_protocolVersion);
                     response.setField(VID_RCC, ERR_ACCESS_DENIED);
                     sendMessage(&response);
                     delete msg;
                  }
                  break;
               default:
                  m_processingQueue->put(msg);
                  break;
            }
         }
      }
      else  // m_proxyConnection
      {
         int rc = m_channel->poll((g_dwIdleTimeout + 1) * 1000);
         if (rc <= 0)
            break;
         if (rc > 0)
         {
            // Update activity timestamp
            m_ts = time(nullptr);

            char buffer[32768];
            ssize_t bytes = m_channel->recv(buffer, 32768);
            if (bytes <= 0)
               break;
            SendEx(m_hProxySocket, buffer, bytes, 0, nullptr);
         }
      }
   }

   // Notify other threads to exit
   m_disconnected = true;
   m_processingQueue->setShutdownMode();
   if (m_hProxySocket != INVALID_SOCKET)
      shutdown(m_hProxySocket, SHUT_RDWR);

   // Wait for other threads to finish
   ThreadJoin(m_processingThread);
   if (m_proxyConnection)
      ThreadJoin(m_proxyReadThread);

   m_tcpProxyLock.lock();
   m_tcpProxies.clear();
   m_tcpProxyLock.unlock();
   ThreadJoin(m_tcpProxyReadThread);

   debugPrintf(4, _T("Session with %s closed"), m_serverAddr.toString().cstr());

   UnregisterSession(m_id);
   m_channel->close();
   debugPrintf(6, _T("Receiver thread stopped"));
}

/**
 * Send prepared raw message over the network and destroy it
 */
bool CommSession::sendRawMessage(NXCP_MESSAGE *msg, NXCPEncryptionContext *ctx)
{
   if (m_disconnected)
   {
      MemFree(msg);
      debugPrintf(6, _T("Aborting sendRawMessage call because session is disconnected"));
      return false;
   }

   bool success = true;
   if (nxlog_get_debug_level() >= 6)
   {
      TCHAR buffer[128];
      debugPrintf(6, _T("Sending message %s (ID %d; size %d; %s)"), NXCPMessageCodeName(ntohs(msg->code), buffer),
               ntohl(msg->id), ntohl(msg->size),
               ntohs(msg->flags) & MF_COMPRESSED ? _T("compressed") : _T("uncompressed"));
      if (nxlog_get_debug_level() >= 8)
      {
         String msgDump = NXCPMessage::dump(msg, m_protocolVersion);
         debugPrintf(8, _T("Outgoing message dump:\n%s"), (const TCHAR *)msgDump);
      }
   }

   if (ctx != nullptr)
   {
      NXCP_ENCRYPTED_MESSAGE *enMsg = ctx->encryptMessage(msg);
      if (enMsg != NULL)
      {
         if (m_channel->send(enMsg, ntohl(enMsg->size), &m_socketWriteMutex) <= 0)
         {
            success = false;
         }
         MemFree(enMsg);
      }
   }
   else
   {
      if (m_channel->send(msg, ntohl(msg->size), &m_socketWriteMutex) <= 0)
      {
         success = false;
      }
   }

	if (!success)
	{
      TCHAR buffer[128];
	   debugPrintf(6, _T("CommSession::SendRawMessage() for %s (size %d) failed (error %d: %s)"),
	            NXCPMessageCodeName(ntohs(msg->code), buffer), ntohl(msg->size), WSAGetLastError(), _tcserror(WSAGetLastError()));
	}
	MemFree(msg);
   return success;
}

/**
 * Send message directly to socket
 */
bool CommSession::sendMessage(const NXCPMessage *msg)
{
   if (m_disconnected)
      return false;

   return sendRawMessage(msg->serialize(m_allowCompression), m_encryptionContext.get());
}

/**
 * Send raw message directly to socket
 */
bool CommSession::sendRawMessage(const NXCP_MESSAGE *msg)
{
   return sendRawMessage(MemCopyBlock(msg, ntohl(msg->size)), m_encryptionContext.get());
}

/**
 * Post message
 */
void CommSession::postMessage(const NXCPMessage *msg)
{
   if (m_disconnected)
      return;
   ThreadPoolExecuteSerialized(g_commThreadPool, m_key, self(), &CommSession::sendMessageInBackground, msg->serialize(m_allowCompression));
}

/**
 * Post raw message
 */
void CommSession::postRawMessage(const NXCP_MESSAGE *msg)
{
   if (m_disconnected)
      return;
   ThreadPoolExecuteSerialized(g_commThreadPool, m_key, self(), &CommSession::sendMessageInBackground, MemCopyBlock(msg, ntohl(msg->size)));
}

/**
 * Send message on background thread
 */
void CommSession::sendMessageInBackground(NXCP_MESSAGE *msg)
{
   sendRawMessage(msg, m_encryptionContext.get());
}

/**
 * Message processing thread
 */
void CommSession::processingThread()
{
   while(true)
   {
      NXCPMessage *request = m_processingQueue->getOrBlock();
      if (request == INVALID_POINTER_VALUE)    // Session termination indicator
         break;
      uint16_t command = request->getCode();

      // Prepare response message
      NXCPMessage response(CMD_REQUEST_COMPLETED, request->getId(), m_protocolVersion);

      // Check if authentication required
      if ((!m_authenticated) && (command != CMD_AUTHENTICATE))
      {
         debugPrintf(6, _T("Authentication required"));
         response.setField(VID_RCC, ERR_AUTH_REQUIRED);
      }
      else if ((g_dwFlags & AF_REQUIRE_ENCRYPTION) && (m_encryptionContext == nullptr))
      {
         debugPrintf(6, _T("Encryption required"));
         response.setField(VID_RCC, ERR_ENCRYPTION_REQUIRED);
      }
      else
      {
         switch(command)
         {
            case CMD_AUTHENTICATE:
               authenticate(request, &response);
               break;
            case CMD_GET_PARAMETER:
               getParameter(request, &response);
               break;
            case CMD_GET_LIST:
               getList(request, &response);
               break;
            case CMD_GET_TABLE:
               getTable(request, &response);
               break;
            case CMD_KEEPALIVE:
               response.setField(VID_RCC, ERR_SUCCESS);
               break;
            case CMD_ACTION:
               action(request, &response);
               break;
            case CMD_TRANSFER_FILE:
               recvFile(request, &response);
               break;
            case CMD_INSTALL_PACKAGE:
               response.setField(VID_RCC, installPackage(request));
               break;
            case CMD_UPGRADE_AGENT:
               response.setField(VID_RCC, upgrade(request));
               break;
            case CMD_GET_PARAMETER_LIST:
               response.setField(VID_RCC, ERR_SUCCESS);
               GetParameterList(&response);
               break;
            case CMD_GET_ENUM_LIST:
               response.setField(VID_RCC, ERR_SUCCESS);
               GetEnumList(&response);
               break;
            case CMD_GET_TABLE_LIST:
               response.setField(VID_RCC, ERR_SUCCESS);
               GetTableList(&response);
               break;
            case CMD_READ_AGENT_CONFIG_FILE:
               getConfig(&response);
               break;
            case CMD_WRITE_AGENT_CONFIG_FILE:
               updateConfig(request, &response);
               break;
            case CMD_ENABLE_AGENT_TRAPS:
               m_acceptTraps = true;
               RegisterSessionForNotifications(self());
               response.setField(VID_RCC, ERR_SUCCESS);
               break;
            case CMD_ENABLE_FILE_UPDATES:
               if (m_masterServer)
               {
                  m_acceptFileUpdates = true;
                  response.setField(VID_RCC, ERR_SUCCESS);
               }
               else
               {
                  response.setField(VID_RCC, ERR_ACCESS_DENIED);
               }
               break;
				case CMD_DEPLOY_AGENT_POLICY:
					if (m_masterServer)
					{
					   debugPrintf(3, _T("Processing policy deployment request"));
					   response.setField(VID_RCC, DeployPolicy(request, m_serverId, m_serverAddr.toString()));
					}
					else
					{
					   response.setField(VID_RCC, ERR_ACCESS_DENIED);
					}
					break;
				case CMD_UNINSTALL_AGENT_POLICY:
					if (m_masterServer)
					{
                  debugPrintf(3, _T("Processing policy uninstall request"));
					   response.setField(VID_RCC, UninstallPolicy(request));
					}
					else
					{
					   response.setField(VID_RCC, ERR_ACCESS_DENIED);
					}
					break;
				case CMD_GET_POLICY_INVENTORY:
					if (m_masterServer)
					{
					   response.setField(VID_RCC, GetPolicyInventory(&response, m_serverId));
					}
					else
					{
					   response.setField(VID_RCC, ERR_ACCESS_DENIED);
					}
					break;
            case CMD_TAKE_SCREENSHOT:
					if (m_controlServer)
					{
                  TCHAR sessionName[256];
                  request->getFieldAsString(VID_NAME, sessionName, 256);
                  debugPrintf(6, _T("Take screenshot from session \"%s\""), sessionName);
                  shared_ptr<SessionAgentConnector> conn = AcquireSessionAgentConnector(sessionName);
                  if (conn != nullptr)
                  {
                     debugPrintf(6, _T("Session agent connector acquired"));
                     conn->takeScreenshot(&response);
                  }
                  else
                  {
                     response.setField(VID_RCC, ERR_NO_SESSION_AGENT);
                  }
					}
					else
					{
					   response.setField(VID_RCC, ERR_ACCESS_DENIED);
					}
					break;
            case CMD_GET_HOSTNAME_BY_IPADDR:
               getHostNameByAddr(request, &response);
               break;
            case CMD_SET_SERVER_CAPABILITIES:
               // Servers before 2.0 use VID_ENABLED
               m_ipv6Aware = request->isFieldExist(VID_IPV6_SUPPORT) ? request->getFieldAsBoolean(VID_IPV6_SUPPORT) : request->getFieldAsBoolean(VID_ENABLED);
               m_bulkReconciliationSupported = request->getFieldAsBoolean(VID_BULK_RECONCILIATION);
               m_allowCompression = request->getFieldAsBoolean(VID_ENABLE_COMPRESSION);
               m_acceptKeepalive = request->getFieldAsBoolean(VID_ACCEPT_KEEPALIVE);
               response.setField(VID_RCC, ERR_SUCCESS);
               response.setField(VID_FLAGS, static_cast<uint16_t>((m_controlServer ? 0x01 : 0x00) | (m_masterServer ? 0x02 : 0x00)));
               debugPrintf(4, _T("Server capabilities: IPv6: %s; bulk reconciliation: %s; compression: %s"),
                           m_ipv6Aware ? _T("yes") : _T("no"),
                           m_bulkReconciliationSupported ? _T("yes") : _T("no"),
                           m_allowCompression ? _T("yes") : _T("no"));
               break;
            case CMD_SET_SERVER_ID:
               m_serverId = request->getFieldAsUInt64(VID_SERVER_ID);
               debugPrintf(4, _T("Server ID set to ") UINT64X_FMT(_T("016")), m_serverId);
               response.setField(VID_RCC, ERR_SUCCESS);
               break;
            case CMD_DATA_COLLECTION_CONFIG:
               if (m_serverId != 0)
               {
                  ConfigureDataCollection(m_serverId, *request);
                  m_acceptData = true;
                  response.setField(VID_RCC, ERR_SUCCESS);
               }
               else
               {
                  writeLog(NXLOG_WARNING, _T("Data collection configuration command received but server ID is not set"));
                  response.setField(VID_RCC, ERR_SERVER_ID_UNSET);
               }
               break;
            case CMD_CLEAN_AGENT_DCI_CONF:
               if (m_masterServer)
               {
                  ClearDataCollectionConfiguration();
                  response.setField(VID_RCC, ERR_SUCCESS);
               }
               else
               {
                  response.setField(VID_RCC, ERR_ACCESS_DENIED);
               }
               break;
            case CMD_SETUP_TCP_PROXY:
               InterlockedIncrement64(&s_tcpProxyConnectionRequests);
               if (m_masterServer && (g_dwFlags & AF_ENABLE_TCP_PROXY))
               {
                  setupTcpProxy(request, &response);
               }
               else
               {
                  response.setField(VID_RCC, ERR_ACCESS_DENIED);
               }
               break;
            case CMD_CLOSE_TCP_PROXY:
               if (m_masterServer && (g_dwFlags & AF_ENABLE_TCP_PROXY))
               {
                  response.setField(VID_RCC, closeTcpProxy(request));
               }
               else
               {
                  response.setField(VID_RCC, ERR_ACCESS_DENIED);
               }
               break;
            case CMD_ADD_UA_NOTIFICATION:
               if (m_masterServer)
               {
                  response.setField(VID_RCC, AddUserAgentNotification(m_serverId, request));
               }
               else
               {
                  response.setField(VID_RCC, ERR_ACCESS_DENIED);
               }
               break;
            case CMD_RECALL_UA_NOTIFICATION:
               if (m_masterServer)
               {
                  response.setField(VID_RCC, RemoveUserAgentNotification(m_serverId, request));
               }
               else
               {
                  response.setField(VID_RCC, ERR_ACCESS_DENIED);
               }
               break;
            case CMD_UPDATE_UA_NOTIFICATIONS:
               if (m_masterServer)
               {
                  response.setField(VID_RCC, UpdateUserAgentNotifications(m_serverId, request));
               }
               else
               {
                  response.setField(VID_RCC, ERR_ACCESS_DENIED);
               }
               break;
            default:
               // Attempt to process unknown command by subagents
               if (!ProcessCommandBySubAgent(command, request, &response, this))
                  response.setField(VID_RCC, ERR_UNKNOWN_COMMAND);
               break;
         }
      }
      delete request;

      // Send response
      sendMessage(&response);
      setResponseSentCondition(response.getId());
   }
}

/**
 * Log authentication failure
 */
static inline void LogAuthFailure(const InetAddress& serverAddr, const TCHAR *method)
{
   TCHAR buffer[64];
   nxlog_write(NXLOG_WARNING, _T("Authentication failed for peer %s (method = %s)"), serverAddr.toString(buffer), method);
   InterlockedIncrement(&g_authenticationFailures);
}

/**
 * Authenticate peer
 */
void CommSession::authenticate(NXCPMessage *pRequest, NXCPMessage *pMsg)
{
   if (m_authenticated)
   {
      // Already authenticated
      pMsg->setField(VID_RCC, (g_dwFlags & AF_REQUIRE_AUTH) ? ERR_ALREADY_AUTHENTICATED : ERR_AUTH_NOT_REQUIRED);
   }
   else
   {
      TCHAR szSecret[MAX_SECRET_LENGTH];
      BYTE hash[32];

      int authMethod = pRequest->getFieldAsInt16(VID_AUTH_METHOD);
      if (authMethod == 0)
         authMethod = AUTH_SHA1_HASH;
      switch(authMethod)
      {
         case AUTH_PLAINTEXT:
            pRequest->getFieldAsString(VID_SHARED_SECRET, szSecret, MAX_SECRET_LENGTH);
            if (!_tcscmp(szSecret, g_szSharedSecret))
            {
               m_authenticated = true;
               pMsg->setField(VID_RCC, ERR_SUCCESS);
            }
            else
            {
               LogAuthFailure(m_serverAddr, _T("PLAIN"));
               pMsg->setField(VID_RCC, ERR_AUTH_FAILED);
            }
            break;
         case AUTH_MD5_HASH:
            pRequest->getFieldAsBinary(VID_SHARED_SECRET, (BYTE *)szSecret, MD5_DIGEST_SIZE);
#ifdef UNICODE
				{
					char sharedSecret[256];
					wchar_to_utf8(g_szSharedSecret, -1, sharedSecret, 256);
					sharedSecret[255] = 0;
					CalculateMD5Hash((BYTE *)sharedSecret, strlen(sharedSecret), hash);
				}
#else
            CalculateMD5Hash((BYTE *)g_szSharedSecret, strlen(g_szSharedSecret), hash);
#endif
            if (!memcmp(szSecret, hash, MD5_DIGEST_SIZE))
            {
               m_authenticated = true;
               pMsg->setField(VID_RCC, ERR_SUCCESS);
            }
            else
            {
               LogAuthFailure(m_serverAddr, _T("MD5"));
               pMsg->setField(VID_RCC, ERR_AUTH_FAILED);
            }
            break;
         case AUTH_SHA1_HASH:
            pRequest->getFieldAsBinary(VID_SHARED_SECRET, (BYTE *)szSecret, SHA1_DIGEST_SIZE);
#ifdef UNICODE
				{
					char sharedSecret[256];
					wchar_to_utf8(g_szSharedSecret, -1, sharedSecret, 256);
					sharedSecret[255] = 0;
					CalculateSHA1Hash((BYTE *)sharedSecret, strlen(sharedSecret), hash);
				}
#else
            CalculateSHA1Hash((BYTE *)g_szSharedSecret, strlen(g_szSharedSecret), hash);
#endif
            if (!memcmp(szSecret, hash, SHA1_DIGEST_SIZE))
            {
               m_authenticated = true;
               pMsg->setField(VID_RCC, ERR_SUCCESS);
            }
            else
            {
               LogAuthFailure(m_serverAddr, _T("SHA1"));
               pMsg->setField(VID_RCC, ERR_AUTH_FAILED);
            }
            break;
         default:
            pMsg->setField(VID_RCC, ERR_NOT_IMPLEMENTED);
            break;
      }
   }
}

/**
 * Get metric's value
 */
void CommSession::getParameter(NXCPMessage *request, NXCPMessage *response)
{
   TCHAR parameter[MAX_RUNTIME_PARAM_NAME];
   request->getFieldAsString(VID_PARAMETER, parameter, MAX_RUNTIME_PARAM_NAME);

   TCHAR value[MAX_RESULT_LENGTH];
   uint32_t errorCode = GetMetricValue(parameter, value, this);

   response->setField(VID_RCC, errorCode);
   if (errorCode == ERR_SUCCESS)
      response->setField(VID_VALUE, value);
}

/**
 * Get list of values
 */
void CommSession::getList(NXCPMessage *request, NXCPMessage *response)
{
   TCHAR *name = request->getFieldAsString(VID_PARAMETER);
   StringList value;
   uint32_t rcc = GetListValue(name, &value, this);
   response->setField(VID_RCC, rcc);
   if (rcc == ERR_SUCCESS)
   {
      value.fillMessage(response, VID_ENUM_VALUE_BASE, VID_NUM_STRINGS);
   }
   MemFree(name);
}

/**
 * Get table
 */
void CommSession::getTable(NXCPMessage *request, NXCPMessage *response)
{
   TCHAR *name = request->getFieldAsString(VID_PARAMETER);
   Table value;
   uint32_t rcc = GetTableValue(name, &value, this);
   response->setField(VID_RCC, rcc);
   if (rcc == ERR_SUCCESS)
   {
		value.fillMessage(response, 0, -1);	// no row limit
   }
   MemFree(name);
}

/**
 * Query web service
 */
void CommSession::queryWebService(NXCPMessage *request)
{
   TCHAR *url = request->getFieldAsString(VID_URL);
   ThreadPoolExecuteSerialized(g_webSvcThreadPool, url, QueryWebService, request, static_pointer_cast<AbstractCommSession>(self()));
   MemFree(url);
}

/**
 * Make web service custom request
 */
void CommSession::webServiceCustomRequest(NXCPMessage *request)
{
   TCHAR *url = request->getFieldAsString(VID_URL);
   ThreadPoolExecuteSerialized(g_webSvcThreadPool, url, WebServiceCustomRequest, request, static_pointer_cast<AbstractCommSession>(self()));
   MemFree(url);
}

/**
 * Perform action on request
 */
void CommSession::action(NXCPMessage *request, NXCPMessage *response)
{
   if ((g_dwFlags & AF_ENABLE_ACTIONS) && m_controlServer)
      ExecuteAction(*request, response, self());
   else
      response->setField(VID_RCC, ERR_ACCESS_DENIED);
}

/**
 * Prepare for receiving file
 */
void CommSession::recvFile(NXCPMessage *request, NXCPMessage *response)
{
	TCHAR szFileName[MAX_PATH], szFullPath[MAX_PATH];

	if (m_masterServer)
	{
		szFileName[0] = 0;
		request->getFieldAsString(VID_FILE_NAME, szFileName, MAX_PATH);
		debugPrintf(5, _T("CommSession::recvFile(): Preparing for receiving file \"%s\""), szFileName);
      BuildFullPath(szFileName, szFullPath);

		// Check if for some reason we have already opened file
      openFile(response, szFullPath, request->getId(), request->getFieldAsTime(VID_MODIFICATION_TIME), static_cast<FileTransferResumeMode>(request->getFieldAsUInt16(VID_RESUME_MODE)));
	}
	else
	{
		response->setField(VID_RCC, ERR_ACCESS_DENIED);
	}
}

/**
 * Open file for writing
 */
void CommSession::openFile(NXCPMessage *response, TCHAR *szFullPath, uint32_t requestId, time_t fileModTime, FileTransferResumeMode resumeMode)
{
   if (resumeMode == FileTransferResumeMode::CHECK)
   {
      if (DownloadFileInfo::getFileInfo(response, szFullPath) == ERR_FILE_APPEND_POSSIBLE) //do not start download if append is possible
         return;
   }

   DownloadFileInfo *fInfo = new DownloadFileInfo(szFullPath, fileModTime);
   debugPrintf(4, _T("CommSession::openFile(): Writing to local file \"%s\""), szFullPath);
   if (!fInfo->open(resumeMode == FileTransferResumeMode::RESUME))
   {
      delete fInfo;
      debugPrintf(3, _T("CommSession::openFile(): Error opening file \"%s\" for writing (%s)"), szFullPath, _tcserror(errno));
      response->setField(VID_RCC, ERR_IO_FAILURE);
   }
   else
   {
      m_downloadFileMap.set(requestId, fInfo);
      response->setField(VID_RCC, ERR_SUCCESS);
   }
}

/**
 * File sending context
 */
struct FileSendContext
{
   CommSession *session;
   time_t lastProbeTime;
   uint32_t msgCount;
};

/**
 * Progress callback for file sending with keepalive messages
 */
static void SendFileProgressCallbackWithKeepalive(size_t bytesTransferred, void *cbArg)
{
   auto context = static_cast<FileSendContext*>(cbArg);
   context->session->updateTimeStamp();
   context->msgCount++;
   time_t now = time(nullptr);
   if ((now - context->lastProbeTime > 5) || (context->msgCount > 8))
   {
      NXCPMessage request(CMD_KEEPALIVE, static_cast<uint32_t>(now));
      context->session->sendMessage(request);
      NXCPMessage *response = context->session->waitForMessage(CMD_REQUEST_COMPLETED, request.getId(), 30000);
      delete response;   // FIXME: do actual check and allow callback to abort file transfer?
      context->lastProbeTime = now;
      context->msgCount = 0;
   }
}

/**
 * Progress callback for file sending without keepalive messages
 */
static void SendFileProgressCallback(size_t bytesTransferred, void *cbArg)
{
   static_cast<CommSession*>(cbArg)->updateTimeStamp();
}

/**
 * Send file to server
 */
bool CommSession::sendFile(uint32_t requestId, const TCHAR *file, off64_t offset, NXCPStreamCompressionMethod compressionMethod, VolatileCounter *cancellationFlag)
{
   if (m_disconnected)
      return false;

   if (!m_acceptKeepalive)
      return SendFileOverNXCP(m_channel.get(), requestId, file, m_encryptionContext.get(), offset, SendFileProgressCallback, this, &m_socketWriteMutex, compressionMethod, cancellationFlag);

   FileSendContext context;
   context.session = this;
   context.lastProbeTime = time(nullptr);
   context.msgCount = 0;
   return SendFileOverNXCP(m_channel.get(), requestId, file, m_encryptionContext.get(), offset, SendFileProgressCallbackWithKeepalive, &context,
         &m_socketWriteMutex, compressionMethod, cancellationFlag);
}

/**
 * Upgrade agent from package in the file store
 */
uint32_t CommSession::upgrade(NXCPMessage *request)
{
   if (m_masterServer)
   {
      TCHAR packageName[MAX_PATH] = _T("");
      request->getFieldAsString(VID_FILE_NAME, packageName, MAX_PATH);
            
      // Store upgrade file name to delete it after system start
      WriteRegistry(_T("upgrade.file"), packageName);
      writeLog(NXLOG_INFO, _T("Starting agent upgrade using package %s"), packageName);

      TCHAR fullPath[MAX_PATH];
      BuildFullPath(packageName, fullPath);
      return UpgradeAgent(fullPath);
   }
   else
   {
      writeLog(NXLOG_WARNING, _T("Upgrade request from server which is not master (upgrade will not start)"));
      return ERR_ACCESS_DENIED;
   }
}

/**
 * Process executor for package installer
 */
class PackageInstallerProcessExecutor : public ProcessExecutor
{
private:
   CommSession *m_session;

protected:
   virtual void onOutput(const char *text, size_t length) override
   {
      m_session->debugPrintf(4, _T("Installer output: %hs"), text);
   }

public:
   PackageInstallerProcessExecutor(const TCHAR *command, CommSession *session) : ProcessExecutor(command)
   {
      m_session = session;
      m_sendOutput = true;
   }
};

/**
 * Install package previously uploaded to the file store
 */
uint32_t CommSession::installPackage(NXCPMessage *request)
{
   if (m_masterServer)
   {
      char packageType[16] = "";
      request->getFieldAsMBString(VID_PACKAGE_TYPE, packageType, 16);

      TCHAR packageName[MAX_PATH] = _T("");
      request->getFieldAsString(VID_FILE_NAME, packageName, MAX_PATH);
      TCHAR fullPath[MAX_PATH];
      BuildFullPath(packageName, fullPath);

      TCHAR command[MAX_PATH] = _T("");
      request->getFieldAsString(VID_COMMAND, command, MAX_PATH);

      StringBuffer commandLine;
      if (!stricmp(packageType, "executable"))
      {
         if (command[0] != 0)
         {
            commandLine.append(command);
            commandLine.replace(_T("${file}"), fullPath);
         }
         else
         {
            commandLine.append(_T("\""));
            commandLine.append(fullPath);
            commandLine.append(_T("\""));
         }
      }
#ifdef _WIN32
      else if (!stricmp(packageType, "msi"))
      {
         commandLine.append(_T("msiexec.exe /i \""));
         commandLine.append(fullPath);
         commandLine.append(_T("\" /qn"));
         if (command[0] != 0)
         {
            commandLine.append(_T(" "));
            commandLine.append(command);
         }
      }
      else if (!stricmp(packageType, "msp"))
      {
         commandLine.append(_T("msiexec.exe /p \""));
         commandLine.append(fullPath);
         commandLine.append(_T("\" /qn"));
         if (command[0] != 0)
         {
            commandLine.append(_T(" "));
            commandLine.append(command);
         }
         commandLine.append(_T(" REINSTALLMODE=\"ecmus\" REINSTALL=\"ALL\""));
      }
      else if (!stricmp(packageType, "msu"))
      {
         commandLine.append(_T("wusa.exe \""));
         commandLine.append(fullPath);
         commandLine.append(_T("\" /quiet"));
         if (command[0] != 0)
         {
            commandLine.append(_T(" "));
            commandLine.append(command);
         }
      }
#else
      else if (!stricmp(packageType, "deb"))
      {
         commandLine.append(_T("/usr/bin/dpkg -i"));
         if (command[0] != 0)
         {
            commandLine.append(_T(" "));
            commandLine.append(command);
         }
         commandLine.append(_T(" '"));
         commandLine.append(fullPath);
         commandLine.append(_T("'"));
      }
      else if (!stricmp(packageType, "rpm"))
      {
         commandLine.append(_T("/usr/bin/rpm -i"));
         if (command[0] != 0)
         {
            commandLine.append(_T(" "));
            commandLine.append(command);
         }
         commandLine.append(_T(" '"));
         commandLine.append(fullPath);
         commandLine.append(_T("'"));
      }
      else if (!stricmp(packageType, "tgz"))
      {
         commandLine.append(_T("tar"));
         if (command[0] != 0)
         {
            commandLine.append(_T(" -C '"));
            commandLine.append(command);
            commandLine.append(_T("'"));
         }
         commandLine.append(_T(" zxf '"));
         commandLine.append(fullPath);
         commandLine.append(_T("'"));
      }
#endif
      else if (!stricmp(packageType, "zip"))
      {
         commandLine.append(_T("unzip"));
         commandLine.append(_T(" '"));
         commandLine.append(fullPath);
         commandLine.append(_T("'"));
         if (command[0] != 0)
         {
            commandLine.append(_T(" -d '"));
            commandLine.append(command);
            commandLine.append(_T("'"));
         }
      }

      writeLog(NXLOG_INFO, _T("Starting package installation using command line %s"), commandLine.cstr());
      PackageInstallerProcessExecutor executor(commandLine, this);
      if (!executor.execute())
      {
         writeLog(NXLOG_INFO, _T("Package installer execution failed"));
         return ERR_EXEC_FAILED;
      }
      return executor.waitForCompletion(600000) ? ERR_SUCCESS : ERR_REQUEST_TIMEOUT; // FIXME: what timeout to use?
   }
   else
   {
      writeLog(NXLOG_WARNING, _T("Package installation request from server which is not master (installation will not start)"));
      return ERR_ACCESS_DENIED;
   }
}

/**
 * Get agent's configuration file
 */
void CommSession::getConfig(NXCPMessage *pMsg)
{
   if (m_masterServer)
   {
      pMsg->setField(VID_RCC,
         pMsg->setFieldFromFile(VID_CONFIG_FILE, g_szConfigFile) ? ERR_SUCCESS : ERR_IO_FAILURE);
   }
   else
   {
      pMsg->setField(VID_RCC, ERR_ACCESS_DENIED);
   }
}

/**
 * Update agent's configuration file
 */
void CommSession::updateConfig(NXCPMessage *request, NXCPMessage *response)
{
   if (m_masterServer)
   {
      size_t size;
      const BYTE *config = request->getBinaryFieldPtr(VID_CONFIG_FILE, &size);
      if (config != nullptr)
      {
         debugPrintf(5, _T("CommSession::updateConfig(): writing %u bytes to %s"), static_cast<uint32_t>(size), g_szConfigFile);
         SaveFileStatus status = SaveFile(g_szConfigFile, config, size, false, true);
         if (status == SaveFileStatus::SUCCESS)
         {
            response->setField(VID_RCC, ERR_SUCCESS);
            g_restartPending = true;
            writeLog(NXLOG_INFO, _T("CommSession::updateConfig(): agent configuration file replaced"));
         }
         else if ((status == SaveFileStatus::OPEN_ERROR) || (status == SaveFileStatus::RENAME_ERROR))
         {
            writeLog(NXLOG_WARNING, _T("CommSession::updateConfig(): cannot opening file \"%s\" for writing (%s)"), g_szConfigFile, _tcserror(errno));
            response->setField(VID_RCC, ERR_FILE_OPEN_ERROR);
         }
         else
         {
            writeLog(NXLOG_WARNING, _T("CommSession::updateConfig(): error writing file (%s)"), _tcserror(errno));
            response->setField(VID_RCC, ERR_IO_FAILURE);
         }
      }
      else
      {
         writeLog(NXLOG_WARNING, _T("CommSession::updateConfig(): file content not provided in request"));
         response->setField(VID_RCC, ERR_MALFORMED_COMMAND);
      }
   }
   else
   {
      writeLog(NXLOG_WARNING, _T("CommSession::updateConfig(): access denied"));
      response->setField(VID_RCC, ERR_ACCESS_DENIED);
   }
}

/**
 * Get hostname by IP address
 */
void CommSession::getHostNameByAddr(NXCPMessage *request, NXCPMessage *response)
{
   InetAddress addr = request->getFieldAsInetAddress(VID_IP_ADDRESS);
   if (addr.isValid())
   {
      TCHAR dnsName[MAX_DNS_NAME];
      if (addr.getHostByAddr(dnsName, MAX_DNS_NAME) != NULL)
      {
         response->setField(VID_NAME, dnsName);
         response->setField(VID_RCC, ERR_SUCCESS);
      }
      else
      {
         response->setField(VID_RCC, ERR_NO_SUCH_INSTANCE);
      }
   }
   else
   {
      response->setField(VID_RCC, ERR_BAD_ARGUMENTS);
   }
}

/**
 * Setup proxy connection
 */
uint32_t CommSession::setupProxyConnection(NXCPMessage *request)
{
   if (!m_masterServer || !(g_dwFlags & AF_ENABLE_PROXY))
      return ERR_ACCESS_DENIED;

   InetAddress addr = request->isFieldExist(VID_DESTINATION_ADDRESS) ?
            request->getFieldAsInetAddress(VID_DESTINATION_ADDRESS) :
            request->getFieldAsInetAddress(VID_IP_ADDRESS);
   uint16_t port = request->getFieldAsUInt16(VID_AGENT_PORT);
   m_hProxySocket = ConnectToHost(addr, port, 10000);
   if (m_hProxySocket == INVALID_SOCKET)
   {
      debugPrintf(5, _T("Failed to setup proxy connection to %s:%d"), (const TCHAR *)addr.toString(), port);
      return ERR_CONNECT_FAILED;
   }

   // Finish proxy connection setup
   shared_ptr<NXCPEncryptionContext> savedCtx = m_encryptionContext;
   m_encryptionContext.reset();
   m_proxyConnection = true;
   m_proxyReadThread = ThreadCreateEx(this, &CommSession::proxyReadThread);

   // Send confirmation message
   // We cannot use sendMessage() because encryption context already overridden
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request->getId());
   msg.setField(VID_RCC, RCC_SUCCESS);
   NXCP_MESSAGE *pRawMsg = msg.serialize();
   sendRawMessage(pRawMsg, savedCtx.get());

   debugPrintf(5, _T("Established proxy connection to %s:%d"), addr.toString().cstr(), port);
   return ERR_SUCCESS;
}

/**
 * Proxy reading thread
 */
void CommSession::proxyReadThread()
{
   SocketPoller sp;
   while(true)
   {
      sp.reset();
      sp.add(m_hProxySocket);
      int rc = sp.poll(500);   // Half-second timeout
      if (rc < 0)
         break;
      if (rc > 0)
      {
         char buffer[32768];
         rc = recv(m_hProxySocket, buffer, 32768, 0);
         if (rc <= 0)
            break;
         m_channel->send(buffer, rc, &m_socketWriteMutex);
      }
   }
   disconnect();
}

/**
 * Wait for request completion
 */
uint32_t CommSession::doRequest(NXCPMessage *msg, uint32_t timeout)
{
   if (!sendMessage(msg))
      return ERR_CONNECTION_BROKEN;

   NXCPMessage *response = m_responseQueue->waitForMessage(CMD_REQUEST_COMPLETED, msg->getId(), timeout);
   uint32_t rcc;
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
 * Wait for request completion
 */
NXCPMessage *CommSession::doRequestEx(NXCPMessage *msg, uint32_t timeout)
{
   if (!sendMessage(msg))
      return nullptr;
   return m_responseQueue->waitForMessage(CMD_REQUEST_COMPLETED, msg->getId(), timeout);
}

/**
 * Wait for specific message
 */
NXCPMessage *CommSession::waitForMessage(UINT16 code, UINT32 id, UINT32 timeout)
{
   return m_responseQueue->waitForMessage(code, id, timeout);
}

/**
 * Generate new request ID
 */
uint32_t CommSession::generateRequestId()
{
   return static_cast<uint32_t>(InterlockedIncrement(&m_requestId));
}

/**
 * Setup TCP proxy
 */
void CommSession::setupTcpProxy(NXCPMessage *request, NXCPMessage *response)
{
   uint32_t rcc = ERR_CONNECT_FAILED;
   InetAddress addr = request->getFieldAsInetAddress(VID_IP_ADDRESS);
   uint16_t port = request->getFieldAsUInt16(VID_PORT);
   SOCKET s = ConnectToHost(addr, port, 5000);
   if (s != INVALID_SOCKET)
   {
      TcpProxy *proxy = new TcpProxy(this, s);
      response->setField(VID_CHANNEL_ID, proxy->getId());
      m_tcpProxyLock.lock();
      m_tcpProxies.add(proxy);
      if (m_tcpProxyReadThread == INVALID_THREAD_HANDLE)
         m_tcpProxyReadThread = ThreadCreateEx(this, &CommSession::tcpProxyReadThread);
      m_tcpProxyLock.unlock();
      debugPrintf(5, _T("TCP proxy %d created (destination address %s port %d)"),
               proxy->getId(), (const TCHAR *)addr.toString(), (int)port);
      rcc = ERR_SUCCESS;
   }
   else
   {
      debugPrintf(5, _T("Cannot setup TCP proxy (cannot connect to %s port %d - %hs)"),
               (const TCHAR *)addr.toString(), (int)port, strerror(errno));
   }
   response->setField(VID_RCC, rcc);
}

/**
 * Close TCP proxy
 */
uint32_t CommSession::closeTcpProxy(NXCPMessage *request)
{
   uint32_t rcc = ERR_INVALID_OBJECT;
   uint32_t id = request->getFieldAsUInt32(VID_CHANNEL_ID);
   m_tcpProxyLock.lock();
   for(int i = 0; i < m_tcpProxies.size(); i++)
   {
      if (m_tcpProxies.get(i)->getId() == id)
      {
         m_tcpProxies.remove(i);
         rcc = ERR_SUCCESS;
         break;
      }
   }
   m_tcpProxyLock.unlock();
   return rcc;
}

/**
 * TCP proxy read thread
 */
void CommSession::tcpProxyReadThread()
{
   debugPrintf(2, _T("TCP proxy read thread started"));

   SocketPoller sp;
   while(!m_disconnected)
   {
      sp.reset();

      m_tcpProxyLock.lock();
      if (m_tcpProxies.isEmpty())
      {
         m_tcpProxyLock.unlock();
         ThreadSleepMs(500);
         continue;
      }
      for(int i = 0; i < m_tcpProxies.size(); i++)
         sp.add(m_tcpProxies.get(i)->getSocket());
      m_tcpProxyLock.unlock();

      int rc = sp.poll(500);
      if (rc < 0)
         break;

      if (rc > 0)
      {
         m_tcpProxyLock.lock();
         for(int i = 0; i < m_tcpProxies.size(); i++)
         {
            TcpProxy *p = m_tcpProxies.get(i);
            if (sp.isSet(p->getSocket()))
            {
               if (!p->readSocket())
               {
                  // Socket read error, close proxy
                  debugPrintf(5, _T("TCP proxy %d closed because of %s"), p->getId(), p->isReadError() ? _T("socket read error") : _T("socket closure"));
                  m_tcpProxies.remove(i);
                  i--;
               }
            }
         }
         m_tcpProxyLock.unlock();
      }
   }

   debugPrintf(2, _T("TCP proxy read thread stopped"));
}

/**
 * Prepare setup message for proxy session on external subagent side
 */
void CommSession::prepareProxySessionSetupMsg(NXCPMessage *msg)
{
   msg->setField(VID_SESSION_ID, m_id);
   msg->setField(VID_SERVER_ID, m_serverId);
   msg->setField(VID_IP_ADDRESS, m_serverAddr);

   uint32_t flags = 0;
   if (m_masterServer)
      flags |= 0x01;
   if (m_controlServer)
      flags |= 0x02;
   if (m_acceptData)
      flags |= 0x04;
   if (m_acceptTraps)
      flags |= 0x08;
   if (m_acceptFileUpdates)
      flags |= 0x10;
   if (m_ipv6Aware)
      flags |= 0x20;
   msg->setField(VID_FLAGS, flags);
}

/**
 * Sets condition after response was sent, if anyone is signed up for this response
 */
void CommSession::setResponseSentCondition(uint32_t responseId)
{
   Condition* responseCondition = m_responseConditionMap.get(responseId);
   if (responseCondition != nullptr)
      responseCondition->set();
}

/**
 * Signing for condition on response sending
 */
void CommSession::registerForResponseSentCondition(uint32_t responseId)
{
   auto responseCondition = new Condition(true);
   m_responseConditionMap.set(responseId, responseCondition);
}

/**
 * Prepare setup message for proxy session on external subagent side
 */
void CommSession::waitForResponseSentCondition(uint32_t responseId)
{
   Condition* responseCondition = m_responseConditionMap.get(responseId);
   if (responseCondition != nullptr)
   {
      responseCondition->wait();
   }
   m_responseConditionMap.remove(responseId);
}

/**
 * Virtual session constructor
 */
VirtualSession::VirtualSession(uint64_t serverId)
{
   m_id = InterlockedIncrement(&s_sessionId);
   m_serverId = serverId;
   _sntprintf(m_debugTag, 16, _T("comm.vs.%u"), m_id);
}

/**
 * Virtual session destructor
 */
VirtualSession::~VirtualSession()
{
}

/**
 * Debug print in virtual session context
 */
void VirtualSession::debugPrintf(int level, const TCHAR *format, ...)
{
   va_list args;
   va_start(args, format);
   nxlog_debug_tag2(m_debugTag, level, format, args);
   va_end(args);
}

/**
 * Write log in virtual session context
 */
void VirtualSession::writeLog(int16_t severity, const TCHAR *format, ...)
{
   va_list args;
   va_start(args, format);
   nxlog_write_tag2(severity, m_debugTag, format, args);
   va_end(args);
}

/**
 * Proxy session constructor
 */
ProxySession::ProxySession(NXCPMessage *msg)
{
   m_id = msg->getFieldAsUInt32(VID_SESSION_ID);
   m_serverId = msg->getFieldAsUInt64(VID_SERVER_ID);
   m_serverAddress = msg->getFieldAsInetAddress(VID_IP_ADDRESS);
   _sntprintf(m_debugTag, 16, _T("comm.ps.%u"), m_id);

   uint32_t flags = msg->getFieldAsUInt32(VID_FLAGS);
   m_masterServer = ((flags & 0x01) != 0);
   m_controlServer = ((flags & 0x02) != 0);
   m_canAcceptData = ((flags & 0x04) != 0);
   m_canAcceptTraps = ((flags & 0x08) != 0);
   m_canAcceptFileUpdates = ((flags & 0x10) != 0);
   m_ipv6Aware = ((flags & 0x20) != 0);
}

/**
 * Proxy session destructor
 */
ProxySession::~ProxySession()
{
}

/**
 * Debug print in proxy session context
 */
void ProxySession::debugPrintf(int level, const TCHAR *format, ...)
{
   va_list args;
   va_start(args, format);
   nxlog_debug_tag2(m_debugTag, level, format, args);
   va_end(args);
}

/**
 * Write log in proxy session context
 */
void ProxySession::writeLog(int16_t severity, const TCHAR *format, ...)
{
   va_list args;
   va_start(args, format);
   nxlog_write_tag2(severity, m_debugTag, format, args);
   va_end(args);
}

/**
 * Send message to client via master agent
 */
bool ProxySession::sendMessage(const NXCPMessage *msg)
{
   NXCP_MESSAGE *rawMsg = msg->serialize();
   bool success = sendRawMessage(rawMsg);
   free(rawMsg);
   return success;
}

/**
 * Post message to client via master agent
 */
void ProxySession::postMessage(const NXCPMessage *msg)
{
   sendMessage(msg);
}

/**
 * Send raw message to client via master agent
 */
bool ProxySession::sendRawMessage(const NXCP_MESSAGE *msg)
{
   UINT32 msgSize = ntohl(msg->size);
   NXCP_MESSAGE *pmsg = (NXCP_MESSAGE *)malloc(msgSize + NXCP_HEADER_SIZE);
   pmsg->code = htons(CMD_PROXY_MESSAGE);
   pmsg->id = htonl(m_id);
   pmsg->flags = htons(MF_BINARY);
   pmsg->size = htonl(msgSize + NXCP_HEADER_SIZE);
   pmsg->numFields = msg->size;
   memcpy(pmsg->fields, msg, msgSize);
   bool success = SendRawMessageToMasterAgent(pmsg);
   free(pmsg);
   return success;
}

/**
 * Post raw message to client via master agent
 */
void ProxySession::postRawMessage(const NXCP_MESSAGE *msg)
{
   sendRawMessage(msg);
}
