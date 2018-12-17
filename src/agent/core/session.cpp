/*
** NetXMS multiplatform core agent
** Copyright (C) 2003-2018 Victor Kirhenshtein
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

/**
 * Externals
 */
void UnregisterSession(UINT32 index, UINT32 id);
UINT32 DeployPolicy(CommSession *session, NXCPMessage *request);
UINT32 UninstallPolicy(CommSession *session, NXCPMessage *request);
UINT32 GetPolicyInventory(CommSession *session, NXCPMessage *msg);
void ClearDataCollectionConfiguration();

/**
 * SNMP proxy thread pool
 */
ThreadPool *g_snmpProxyThreadPool = NULL;

/**
 * Communication request processing thread pool
 */
ThreadPool *g_commThreadPool = NULL;


/**
 * Agent action thread pool
 */
ThreadPool *g_agentActionThreadPool = NULL;

/**
 * Next free session ID
 */
static VolatileCounter s_sessionId = 0;

/**
 * Agent proxy statistics
 */
static UINT64 s_proxyConnectionRequests = 0;
static VolatileCounter s_activeProxySessions = 0;

/**
 * Handler for agent proxy stats parameters
 */
LONG H_AgentProxyStats(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   switch(*arg)
   {
      case 'A':
         ret_uint(value, (UINT32)s_activeProxySessions);
         break;
      case 'C':
         ret_uint64(value, s_proxyConnectionRequests);
         break;
      default:
         return SYSINFO_RC_UNSUPPORTED;
   }
   return SYSINFO_RC_SUCCESS;
}

/**
 * Client communication read thread
 */
THREAD_RESULT THREAD_CALL CommSession::readThreadStarter(void *arg)
{
   static_cast<CommSession *>(arg)->readThread();
   UnregisterSession(static_cast<CommSession *>(arg)->getIndex(), static_cast<CommSession *>(arg)->getId());
   static_cast<CommSession *>(arg)->debugPrintf(6, _T("Receiver thread stopped"));
   static_cast<CommSession *>(arg)->decRefCount();
   return THREAD_OK;
}

/**
 * Received message processing thread
 */
THREAD_RESULT THREAD_CALL CommSession::processingThreadStarter(void *arg)
{
   static_cast<CommSession *>(arg)->processingThread();
   return THREAD_OK;
}

/**
 * Proxy read thread
 */
THREAD_RESULT THREAD_CALL CommSession::proxyReadThreadStarter(void *arg)
{
   static_cast<CommSession *>(arg)->proxyReadThread();
   return THREAD_OK;
}

/**
 * TCP proxy read thread
 */
THREAD_RESULT THREAD_CALL CommSession::tcpProxyReadThreadStarter(void *arg)
{
   static_cast<CommSession *>(arg)->tcpProxyReadThread();
   return THREAD_OK;
}

/**
 * Client session class constructor
 */
CommSession::CommSession(AbstractCommChannel *channel, const InetAddress &serverAddr, bool masterServer, bool controlServer) : m_downloadFileMap(true), m_tcpProxies(0, 16, true)
{
   m_id = InterlockedIncrement(&s_sessionId);
   m_index = INVALID_INDEX;
   _sntprintf(m_key, 32, _T("CommSession-%u"), m_id);
   m_processingQueue = new Queue;
   m_channel = channel;
   m_channel->incRefCount();
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
   m_pCtx = NULL;
   m_ts = time(NULL);
   m_socketWriteMutex = MutexCreate();
   m_responseQueue = new MsgWaitQueue();
   m_requestId = 0;
   m_tcpProxyLock = MutexCreate();
}

/**
 * Destructor
 */
CommSession::~CommSession()
{
   if (m_proxyConnection)
      InterlockedDecrement(&s_activeProxySessions);

   m_channel->shutdown();
   m_channel->close();
   m_channel->decRefCount();
   if (m_hProxySocket != INVALID_SOCKET)
      closesocket(m_hProxySocket);
   m_disconnected = true;

   void *p;
   while((p = m_processingQueue->get()) != NULL)
      if (p != INVALID_POINTER_VALUE)
         delete (NXCPMessage *)p;
   delete m_processingQueue;
	if ((m_pCtx != NULL) && (m_pCtx != PROXY_ENCRYPTION_CTX))
		m_pCtx->decRefCount();
	MutexDestroy(m_socketWriteMutex);
   delete m_responseQueue;
   MutexDestroy(m_tcpProxyLock);
}

/**
 * Debug print in session context
 */
void CommSession::debugPrintf(int level, const TCHAR *format, ...)
{
   va_list args;
   va_start(args, format);
   nxlog_debug_tag_object2(_T("comm.cs"), m_id, level, format, args);
   va_end(args);
}

/**
 * Start all threads
 */
void CommSession::run()
{
   m_processingThread = ThreadCreateEx(processingThreadStarter, 0, this);
   ThreadCreate(readThreadStarter, 0, this);
}

/**
 * Disconnect session
 */
void CommSession::disconnect()
{
	debugPrintf(5, _T("CommSession::disconnect()"));
	m_tcpProxies.clear();
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
            if (m_ts < time(NULL) - (time_t)g_dwIdleTimeout)
            {
               debugPrintf(5, _T("Session disconnected by timeout (last activity timestamp is %d)"), (int)m_ts);
               break;
            }
            continue;
         }

         // Receive error
         if (msg == NULL)
         {
            if (result == MSGRECV_CLOSED)
               debugPrintf(5, _T("Communication channel closed by peer"));
            else
               debugPrintf(5, _T("Message receiving error (%s)"), AbstractMessageReceiver::resultToText(result));
            break;
         }

         // Update activity timestamp
         m_ts = time(NULL);

         if (nxlog_get_debug_level() >= 8)
         {
            String msgDump = NXCPMessage::dump(receiver.getRawMessageBuffer(), NXCP_VERSION);
            debugPrintf(8, _T("Message dump:\n%s"), (const TCHAR *)msgDump);
         }

         if (msg->isBinary())
         {
            TCHAR buffer[64];
            debugPrintf(6, _T("Received raw message %s"), NXCPMessageCodeName(msg->getCode(), buffer));

            if (msg->getCode() == CMD_FILE_DATA)
            {
               DownloadFileInfo *dInfo = m_downloadFileMap.get(msg->getId());
               if (dInfo != NULL)
               {
                  if (dInfo->write(msg->getBinaryData(), msg->getBinaryDataSize(), msg->isCompressedStream()))
                  {
                     if (msg->isEndOfFile())
                     {
                        NXCPMessage response;

                        dInfo->close(true);
                        m_downloadFileMap.remove(msg->getId());

                        response.setCode(CMD_REQUEST_COMPLETED);
                        response.setId(msg->getId());
                        response.setField(VID_RCC, ERR_SUCCESS);
                        sendMessage(&response);
                     }
                  }
                  else
                  {
                     // I/O error
                     NXCPMessage response;

                     dInfo->close(false);
                     m_downloadFileMap.remove(msg->getId());

                     response.setCode(CMD_REQUEST_COMPLETED);
                     response.setId(msg->getId());
                     response.setField(VID_RCC, ERR_IO_FAILURE);
                     sendMessage(&response);
                  }
               }
            }
            else if (msg->getCode() == CMD_TCP_PROXY_DATA)
            {
               UINT32 proxyId = msg->getId();
               MutexLock(m_tcpProxyLock);
               for(int i = 0; i < m_tcpProxies.size(); i++)
               {
                  TcpProxy *p = m_tcpProxies.get(i);
                  if (p->getId() == proxyId)
                  {
                     p->writeSocket(msg->getBinaryData(), msg->getBinaryDataSize());
                     break;
                  }
               }
               MutexUnlock(m_tcpProxyLock);
            }
         }
         else if (msg->isControl())
         {
            TCHAR buffer[64];
            debugPrintf(6, _T("Received control message %s"), NXCPMessageCodeName(msg->getCode(), buffer));

            if (msg->getCode() == CMD_GET_NXCP_CAPS)
            {
               NXCP_MESSAGE *response = (NXCP_MESSAGE *)malloc(NXCP_HEADER_SIZE);
               response->id = htonl(msg->getId());
               response->code = htons((WORD)CMD_NXCP_CAPS);
               response->flags = htons(MF_CONTROL);
               response->numFields = htonl(NXCP_VERSION << 24);
               response->size = htonl(NXCP_HEADER_SIZE);
               sendRawMessage(response, m_pCtx);
            }
            delete msg;
         }
         else
         {
            TCHAR buffer[64];
            debugPrintf(6, _T("Received message %s (%d)"), NXCPMessageCodeName(msg->getCode(), buffer), msg->getId());

            UINT32 rcc;
            switch(msg->getCode())
            {
               case CMD_REQUEST_COMPLETED:
                  m_responseQueue->put(msg);
                  break;
               case CMD_REQUEST_SESSION_KEY:
                  if (m_pCtx == NULL)
                  {
                     NXCPMessage *pResponse;
                     SetupEncryptionContext(msg, &m_pCtx, &pResponse, NULL, NXCP_VERSION);
                     sendMessage(pResponse);
                     delete pResponse;
                     receiver.setEncryptionContext(m_pCtx);
                  }
                  delete msg;
                  break;
               case CMD_SETUP_PROXY_CONNECTION:
                  s_proxyConnectionRequests++;
                  rcc = setupProxyConnection(msg);
                  // When proxy session established incoming messages will
                  // not be processed locally. Acknowledgment message sent
                  // by setupProxyConnection() in case of success.
                  if (rcc == ERR_SUCCESS)
                  {
                     InterlockedIncrement(&s_activeProxySessions);
                     m_processingQueue->put(INVALID_POINTER_VALUE);
                  }
                  else
                  {
                     NXCPMessage response;
                     response.setCode(CMD_REQUEST_COMPLETED);
                     response.setId(msg->getId());
                     response.setField(VID_RCC, rcc);
                     sendMessage(&response);
                  }
                  delete msg;
                  break;
               case CMD_SNMP_REQUEST:
                  if (m_masterServer && (g_dwFlags & AF_ENABLE_SNMP_PROXY))
                  {
                     incRefCount();
                     ThreadPoolExecute(g_snmpProxyThreadPool, this, &CommSession::proxySnmpRequest, msg);
                  }
                  else
                  {
                     NXCPMessage response;
                     response.setCode(CMD_REQUEST_COMPLETED);
                     response.setId(msg->getId());
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
            m_ts = time(NULL);

            char buffer[32768];
            rc = m_channel->recv(buffer, 32768);
            if (rc <= 0)
               break;
            SendEx(m_hProxySocket, buffer, rc, 0, NULL);
         }
      }
   }

   // Notify other threads to exit
   m_processingQueue->put(INVALID_POINTER_VALUE);
   if (m_hProxySocket != INVALID_SOCKET)
      shutdown(m_hProxySocket, SHUT_RDWR);

   // Wait for other threads to finish
   ThreadJoin(m_processingThread);
   if (m_proxyConnection)
      ThreadJoin(m_proxyReadThread);

   m_tcpProxies.clear();
   ThreadJoin(m_tcpProxyReadThread);

   debugPrintf(5, _T("Session with %s closed"), (const TCHAR *)m_serverAddr.toString());
}

/**
 * Send prepared raw message over the network and destroy it
 */
bool CommSession::sendRawMessage(NXCP_MESSAGE *msg, NXCPEncryptionContext *ctx)
{
   if (m_disconnected)
   {
      free(msg);
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
         String msgDump = NXCPMessage::dump(msg, NXCP_VERSION);
         debugPrintf(8, _T("Outgoing message dump:\n%s"), (const TCHAR *)msgDump);
      }
   }

   if ((ctx != NULL) && (ctx != PROXY_ENCRYPTION_CTX))
   {
      NXCP_ENCRYPTED_MESSAGE *enMsg = ctx->encryptMessage(msg);
      if (enMsg != NULL)
      {
         if (m_channel->send(enMsg, ntohl(enMsg->size), m_socketWriteMutex) <= 0)
         {
            success = false;
         }
         free(enMsg);
      }
   }
   else
   {
      if (m_channel->send(msg, ntohl(msg->size), m_socketWriteMutex) <= 0)
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
   free(msg);
   return success;
}

/**
 * Send message directly to socket
 */
bool CommSession::sendMessage(const NXCPMessage *msg)
{
   if (m_disconnected)
      return false;

   return sendRawMessage(msg->serialize(m_allowCompression), m_pCtx);
}

/**
 * Send raw message directly to socket
 */
bool CommSession::sendRawMessage(const NXCP_MESSAGE *msg)
{
   return sendRawMessage((NXCP_MESSAGE *)nx_memdup(msg, ntohl(msg->size)), m_pCtx);
}

/**
 * Post message
 */
void CommSession::postMessage(const NXCPMessage *msg)
{
   if (m_disconnected)
      return;
   incRefCount();
   ThreadPoolExecuteSerialized(g_commThreadPool, m_key, this, &CommSession::sendMessageInBackground, msg->serialize(m_allowCompression));
}

/**
 * Post raw message
 */
void CommSession::postRawMessage(const NXCP_MESSAGE *msg)
{
   if (m_disconnected)
      return;
   incRefCount();
   ThreadPoolExecuteSerialized(g_commThreadPool, m_key, this, &CommSession::sendMessageInBackground, static_cast<NXCP_MESSAGE*>(nx_memdup(msg, ntohl(msg->size))));
}

/**
 * Send message on background thread
 */
void CommSession::sendMessageInBackground(NXCP_MESSAGE *msg)
{
   sendRawMessage(msg, m_pCtx);
   decRefCount();
}

/**
 * Message processing thread
 */
void CommSession::processingThread()
{
   NXCPMessage response;
   while(true)
   {
      NXCPMessage *request = (NXCPMessage *)m_processingQueue->getOrBlock();
      if (request == INVALID_POINTER_VALUE)    // Session termination indicator
         break;
      UINT32 command = request->getCode();

      // Prepare response message
      response.setCode(CMD_REQUEST_COMPLETED);
      response.setId(request->getId());

      // Check if authentication required
      if ((!m_authenticated) && (command != CMD_AUTHENTICATE))
      {
			debugPrintf(6, _T("Authentication required"));
			response.setField(VID_RCC, ERR_AUTH_REQUIRED);
      }
      else if ((g_dwFlags & AF_REQUIRE_ENCRYPTION) && (m_pCtx == NULL))
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
            case CMD_GET_AGENT_CONFIG:
               getConfig(&response);
               break;
            case CMD_UPDATE_AGENT_CONFIG:
               updateConfig(request, &response);
               break;
            case CMD_ENABLE_AGENT_TRAPS:
               m_acceptTraps = true;
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
					   response.setField(VID_RCC, DeployPolicy(this, request));
					}
					else
					{
					   response.setField(VID_RCC, ERR_ACCESS_DENIED);
					}
					break;
				case CMD_UNINSTALL_AGENT_POLICY:
					if (m_masterServer)
					{
					   response.setField(VID_RCC, UninstallPolicy(this, request));
					}
					else
					{
					   response.setField(VID_RCC, ERR_ACCESS_DENIED);
					}
					break;
				case CMD_GET_POLICY_INVENTORY:
					if (m_masterServer)
					{
					   response.setField(VID_RCC, GetPolicyInventory(this, &response));
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
                  SessionAgentConnector *conn = AcquireSessionAgentConnector(sessionName);
                  if (conn != NULL)
                  {
                     debugPrintf(6, _T("Session agent connector acquired"));
                     conn->takeScreenshot(&response);
                     conn->decRefCount();
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
            case CMD_HOST_BY_IP:
            {
               InetAddress addr = request->getFieldAsInetAddress(VID_IP_ADDRESS);
               TCHAR dnsName[MAX_DNS_NAME];
               response.setField(VID_NAME, addr.getHostByAddr(dnsName, MAX_DNS_NAME));
               response.setField(VID_RCC, ERR_SUCCESS);
               break;
            }
            case CMD_SET_SERVER_CAPABILITIES:
               // Servers before 2.0 use VID_ENABLED
               m_ipv6Aware = request->isFieldExist(VID_IPV6_SUPPORT) ? request->getFieldAsBoolean(VID_IPV6_SUPPORT) : request->getFieldAsBoolean(VID_ENABLED);
               m_bulkReconciliationSupported = request->getFieldAsBoolean(VID_BULK_RECONCILIATION);
               m_allowCompression = request->getFieldAsBoolean(VID_ENABLE_COMPRESSION);
               response.setField(VID_RCC, ERR_SUCCESS);
               response.setField(VID_FLAGS, static_cast<UINT16>((m_controlServer ? 0x01 : 0x00) | (m_masterServer ? 0x02 : 0x00)));
               debugPrintf(1, _T("Server capabilities: IPv6: %s; bulk reconciliation: %s; compression: %s"),
                           m_ipv6Aware ? _T("yes") : _T("no"),
                           m_bulkReconciliationSupported ? _T("yes") : _T("no"),
                           m_allowCompression ? _T("yes") : _T("no"));
               break;
            case CMD_SET_SERVER_ID:
               m_serverId = request->getFieldAsUInt64(VID_SERVER_ID);
               debugPrintf(1, _T("Server ID set to ") UINT64X_FMT(_T("016")), m_serverId);
               response.setField(VID_RCC, ERR_SUCCESS);
               break;
            case CMD_DATA_COLLECTION_CONFIG:
               if (m_serverId != 0)
               {
                  ConfigureDataCollection(m_serverId, request);
                  m_acceptData = true;
                  response.setField(VID_RCC, ERR_SUCCESS);
               }
               else
               {
                  debugPrintf(1, _T("Data collection configuration command received but server ID is not set"));
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
      response.deleteAllFields();
   }
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
      WORD wAuthMethod;

      wAuthMethod = pRequest->getFieldAsUInt16(VID_AUTH_METHOD);
      switch(wAuthMethod)
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
               nxlog_write(MSG_AUTH_FAILED, EVENTLOG_WARNING_TYPE, "Is", &m_serverAddr, _T("PLAIN"));
               pMsg->setField(VID_RCC, ERR_AUTH_FAILED);
            }
            break;
         case AUTH_MD5_HASH:
            pRequest->getFieldAsBinary(VID_SHARED_SECRET, (BYTE *)szSecret, MD5_DIGEST_SIZE);
#ifdef UNICODE
				{
					char sharedSecret[256];
					WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR, g_szSharedSecret, -1, sharedSecret, 256, NULL, NULL);
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
               nxlog_write(MSG_AUTH_FAILED, EVENTLOG_WARNING_TYPE, "Is", &m_serverAddr, _T("MD5"));
               pMsg->setField(VID_RCC, ERR_AUTH_FAILED);
            }
            break;
         case AUTH_SHA1_HASH:
            pRequest->getFieldAsBinary(VID_SHARED_SECRET, (BYTE *)szSecret, SHA1_DIGEST_SIZE);
#ifdef UNICODE
				{
					char sharedSecret[256];
					WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR, g_szSharedSecret, -1, sharedSecret, 256, NULL, NULL);
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
               nxlog_write(MSG_AUTH_FAILED, EVENTLOG_WARNING_TYPE, "Is", &m_serverAddr, _T("SHA1"));
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
 * Get parameter's value
 */
void CommSession::getParameter(NXCPMessage *pRequest, NXCPMessage *pMsg)
{
   TCHAR szParameter[MAX_RUNTIME_PARAM_NAME], szValue[MAX_RESULT_LENGTH];
   UINT32 dwErrorCode;

   pRequest->getFieldAsString(VID_PARAMETER, szParameter, MAX_RUNTIME_PARAM_NAME);
   dwErrorCode = GetParameterValue(szParameter, szValue, this);
   pMsg->setField(VID_RCC, dwErrorCode);
   if (dwErrorCode == ERR_SUCCESS)
      pMsg->setField(VID_VALUE, szValue);
}

/**
 * Get list of values
 */
void CommSession::getList(NXCPMessage *pRequest, NXCPMessage *pMsg)
{
   TCHAR szParameter[MAX_RUNTIME_PARAM_NAME];
   pRequest->getFieldAsString(VID_PARAMETER, szParameter, MAX_RUNTIME_PARAM_NAME);

   StringList value;
   UINT32 dwErrorCode = GetListValue(szParameter, &value, this);
   pMsg->setField(VID_RCC, dwErrorCode);
   if (dwErrorCode == ERR_SUCCESS)
   {
      value.fillMessage(pMsg, VID_ENUM_VALUE_BASE, VID_NUM_STRINGS);
   }
}

/**
 * Get table
 */
void CommSession::getTable(NXCPMessage *pRequest, NXCPMessage *pMsg)
{
   TCHAR szParameter[MAX_RUNTIME_PARAM_NAME];

   pRequest->getFieldAsString(VID_PARAMETER, szParameter, MAX_RUNTIME_PARAM_NAME);

   Table value;
   UINT32 dwErrorCode = GetTableValue(szParameter, &value, this);
   pMsg->setField(VID_RCC, dwErrorCode);
   if (dwErrorCode == ERR_SUCCESS)
   {
		value.fillMessage(*pMsg, 0, -1);	// no row limit
   }
}

/**
 * Perform action on request
 */
void CommSession::action(NXCPMessage *pRequest, NXCPMessage *pMsg)
{
   if ((g_dwFlags & AF_ENABLE_ACTIONS) && m_controlServer)
      ExecuteAction(pRequest, pMsg, this);
   else
      pMsg->setField(VID_RCC, ERR_ACCESS_DENIED);
}

/**
 * Prepare for receiving file
 */
void CommSession::recvFile(NXCPMessage *pRequest, NXCPMessage *pMsg)
{
	TCHAR szFileName[MAX_PATH], szFullPath[MAX_PATH];

	if (m_masterServer)
	{
		szFileName[0] = 0;
		pRequest->getFieldAsString(VID_FILE_NAME, szFileName, MAX_PATH);
		debugPrintf(5, _T("CommSession::recvFile(): Preparing for receiving file \"%s\""), szFileName);
      BuildFullPath(szFileName, szFullPath);

		// Check if for some reason we have already opened file
      pMsg->setField(VID_RCC, openFile(szFullPath, pRequest->getId(), pRequest->getFieldAsTime(VID_MODIFICATION_TIME)));
	}
	else
	{
		pMsg->setField(VID_RCC, ERR_ACCESS_DENIED);
	}
}

/**
 * Open file for writing
 */
UINT32 CommSession::openFile(TCHAR *szFullPath, UINT32 requestId, time_t fileModTime)
{
   DownloadFileInfo *fInfo = new DownloadFileInfo(szFullPath, fileModTime);
   debugPrintf(5, _T("CommSession::recvFile(): Writing to local file \"%s\""), szFullPath);

   if (!fInfo->open())
   {
      delete fInfo;
      debugPrintf(2, _T("CommSession::recvFile(): Error opening file \"%s\" for writing (%s)"), szFullPath, _tcserror(errno));
      return ERR_IO_FAILURE;
   }
   else
   {
      m_downloadFileMap.set(requestId, fInfo);
      return ERR_SUCCESS;
   }
}

/**
 * Progress callback for file sending
 */
static void SendFileProgressCallback(INT64 bytesTransferred, void *cbArg)
{
	((CommSession *)cbArg)->updateTimeStamp();
}

/**
 * Send file to server
 */
bool CommSession::sendFile(UINT32 requestId, const TCHAR *file, long offset, bool allowCompression, VolatileCounter *cancellationFlag)
{
   if (m_disconnected)
      return false;
	return SendFileOverNXCP(m_channel, requestId, file, m_pCtx, offset, SendFileProgressCallback, this, m_socketWriteMutex, 
            allowCompression ? NXCP_STREAM_COMPRESSION_DEFLATE : NXCP_STREAM_COMPRESSION_NONE, cancellationFlag);
}

/**
 * Upgrade agent from package in the file store
 */
UINT32 CommSession::upgrade(NXCPMessage *request)
{
   if (m_masterServer)
   {
      TCHAR szPkgName[MAX_PATH], szFullPath[MAX_PATH];

      szPkgName[0] = 0;
      request->getFieldAsString(VID_FILE_NAME, szPkgName, MAX_PATH);
      BuildFullPath(szPkgName, szFullPath);

      //Create line in registry file with upgrade file name to delete it after system start
      WriteRegistry(_T("upgrade.file"), szPkgName);
      return UpgradeAgent(szFullPath);
   }
   else
   {
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
void CommSession::updateConfig(NXCPMessage *pRequest, NXCPMessage *pMsg)
{
   if (m_masterServer)
   {
      if (pRequest->isFieldExist(VID_CONFIG_FILE))
      {
         size_t size = pRequest->getFieldAsBinary(VID_CONFIG_FILE, NULL, 0);
         BYTE *pConfig = (BYTE *)malloc(size);
         pRequest->getFieldAsBinary(VID_CONFIG_FILE, pConfig, size);
         int hFile = _topen(g_szConfigFile, O_CREAT | O_TRUNC | O_WRONLY, 0644);
         if (hFile != -1)
         {
            if (size > 0)
            {
               for(UINT32 i = 0; i < size - 1; i++)
                  if (pConfig[i] == 0x0D)
                  {
                     size--;
                     memmove(&pConfig[i], &pConfig[i + 1], size - i);
							i--;
                  }
            }
            if (_write(hFile, pConfig, static_cast<unsigned int>(size)) == size)
               pMsg->setField(VID_RCC, ERR_SUCCESS);
            else
               pMsg->setField(VID_RCC, ERR_IO_FAILURE);
            _close(hFile);
         }
         else
         {
				debugPrintf(2, _T("CommSession::updateConfig(): Error opening file \"%s\" for writing (%s)"),
                        g_szConfigFile, _tcserror(errno));
            pMsg->setField(VID_RCC, ERR_FILE_OPEN_ERROR);
         }
         free(pConfig);
      }
      else
      {
         pMsg->setField(VID_RCC, ERR_MALFORMED_COMMAND);
      }
   }
   else
   {
      pMsg->setField(VID_RCC, ERR_ACCESS_DENIED);
   }
}

/**
 * Setup proxy connection
 */
UINT32 CommSession::setupProxyConnection(NXCPMessage *request)
{
   if (!m_masterServer || !(g_dwFlags & AF_ENABLE_PROXY))
      return ERR_ACCESS_DENIED;

   InetAddress addr = request->isFieldExist(VID_DESTINATION_ADDRESS) ?
            request->getFieldAsInetAddress(VID_DESTINATION_ADDRESS) :
            request->getFieldAsInetAddress(VID_IP_ADDRESS);
   UINT16 port = request->getFieldAsUInt16(VID_AGENT_PORT);
   m_hProxySocket = ConnectToHost(addr, port, 10000);
   if (m_hProxySocket == INVALID_SOCKET)
   {
      debugPrintf(5, _T("Failed to setup proxy connection to %s:%d"), (const TCHAR *)addr.toString(), port);
      return ERR_CONNECT_FAILED;
   }

   // Finish proxy connection setup
   NXCPEncryptionContext *pSavedCtx = m_pCtx;
   m_pCtx = PROXY_ENCRYPTION_CTX;
   m_proxyConnection = true;
   m_proxyReadThread = ThreadCreateEx(proxyReadThreadStarter, 0, this);

   // Send confirmation message
   // We cannot use sendMessage() because encryption context already overridden
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request->getId());
   msg.setField(VID_RCC, RCC_SUCCESS);
   NXCP_MESSAGE *pRawMsg = msg.serialize();
   sendRawMessage(pRawMsg, pSavedCtx);
   if (pSavedCtx != NULL)
      pSavedCtx->decRefCount();

   debugPrintf(5, _T("Established proxy connection to %s:%d"), (const TCHAR *)addr.toString(), port);
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
         m_channel->send(buffer, rc, m_socketWriteMutex);
      }
   }
   disconnect();
}

/**
 * Wait for request completion
 */
UINT32 CommSession::doRequest(NXCPMessage *msg, UINT32 timeout)
{
   if (!sendMessage(msg))
      return ERR_CONNECTION_BROKEN;

   NXCPMessage *response = m_responseQueue->waitForMessage(CMD_REQUEST_COMPLETED, msg->getId(), timeout);
   UINT32 rcc;
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
 * Wait for request completion
 */
NXCPMessage *CommSession::doRequestEx(NXCPMessage *msg, UINT32 timeout)
{
   if (!sendMessage(msg))
      return NULL;
   return m_responseQueue->waitForMessage(CMD_REQUEST_COMPLETED, msg->getId(), timeout);
}

NXCPMessage *CommSession::waitForMessage(UINT16 code, UINT32 id, UINT32 timeout)
{
   return m_responseQueue->waitForMessage(code, id, timeout);
}

/**
 * Generate new request ID
 */
UINT32 CommSession::generateRequestId()
{
   return (UINT32)InterlockedIncrement(&m_requestId);
}

/**
 * Setup TCP proxy
 */
void CommSession::setupTcpProxy(NXCPMessage *request, NXCPMessage *response)
{
   UINT32 rcc = ERR_CONNECT_FAILED;
   InetAddress addr = request->getFieldAsInetAddress(VID_IP_ADDRESS);
   UINT16 port = request->getFieldAsUInt16(VID_PORT);
   SOCKET s = ConnectToHost(addr, port, 5000);
   if (s != INVALID_SOCKET)
   {
      TcpProxy *proxy = new TcpProxy(this, s);
      response->setField(VID_CHANNEL_ID, proxy->getId());
      MutexLock(m_tcpProxyLock);
      m_tcpProxies.add(proxy);
      if (m_tcpProxyReadThread == INVALID_THREAD_HANDLE)
         m_tcpProxyReadThread = ThreadCreateEx(CommSession::tcpProxyReadThreadStarter, 0, this);
      MutexUnlock(m_tcpProxyLock);
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
UINT32 CommSession::closeTcpProxy(NXCPMessage *request)
{
   UINT32 rcc = ERR_INVALID_OBJECT;
   UINT32 id = request->getFieldAsUInt32(VID_CHANNEL_ID);
   MutexLock(m_tcpProxyLock);
   for(int i = 0; i < m_tcpProxies.size(); i++)
   {
      if (m_tcpProxies.get(i)->getId() == id)
      {
         m_tcpProxies.remove(i);
         rcc = ERR_SUCCESS;
         break;
      }
   }
   MutexUnlock(m_tcpProxyLock);
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

      MutexLock(m_tcpProxyLock);
      if (m_tcpProxies.isEmpty())
      {
         MutexUnlock(m_tcpProxyLock);
         ThreadSleepMs(500);
         continue;
      }
      for(int i = 0; i < m_tcpProxies.size(); i++)
         sp.add(m_tcpProxies.get(i)->getSocket());
      MutexUnlock(m_tcpProxyLock);

      int rc = sp.poll(500);
      if (rc < 0)
         break;

      if (rc > 0)
      {
         MutexLock(m_tcpProxyLock);
         for(int i = 0; i < m_tcpProxies.size(); i++)
         {
            TcpProxy *p = m_tcpProxies.get(i);
            if (sp.isSet(p->getSocket()))
            {
               if (!p->readSocket())
               {
                  // Socket read error, close proxy
                  debugPrintf(5, _T("TCP proxy %d closed because of socket read error"), p->getId());
                  m_tcpProxies.remove(i);
                  i--;
               }
            }
         }
         MutexUnlock(m_tcpProxyLock);
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

   UINT32 flags = 0;
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
 * Virtual session constructor
 */
VirtualSession::VirtualSession(UINT64 serverId)
{
   m_id = InterlockedIncrement(&s_sessionId);
   m_serverId = serverId;
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
   nxlog_debug_tag_object2(_T("comm.vs"), m_id, level, format, args);
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
   
   UINT32 flags = msg->getFieldAsUInt32(VID_FLAGS);
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
   if (level > nxlog_get_debug_level())
      return;

   va_list args;
   TCHAR buffer[8192];

   va_start(args, format);
   _vsntprintf(buffer, 8192, format, args);
   va_end(args);

   nxlog_write(MSG_DEBUG_PSESSION, EVENTLOG_DEBUG_TYPE, "ds", m_id, buffer);
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
