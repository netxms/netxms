/*
** NetXMS multiplatform core agent
** Copyright (C) 2003-2016 Victor Kirhenshtein
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
THREAD_RESULT THREAD_CALL CommSession::readThreadStarter(void *pArg)
{
   ((CommSession *)pArg)->readThread();
   UnregisterSession(((CommSession *)pArg)->getIndex(), ((CommSession *)pArg)->getId());
   ((CommSession *)pArg)->debugPrintf(6, _T("Receiver thread stopped"));
   ((CommSession *)pArg)->decRefCount();
   return THREAD_OK;
}

/**
 * Client communication write thread
 */
THREAD_RESULT THREAD_CALL CommSession::writeThreadStarter(void *pArg)
{
   ((CommSession *)pArg)->writeThread();
   return THREAD_OK;
}

/**
 * Received message processing thread
 */
THREAD_RESULT THREAD_CALL CommSession::processingThreadStarter(void *pArg)
{
   ((CommSession *)pArg)->processingThread();
   return THREAD_OK;
}

/**
 * Client communication write thread
 */
THREAD_RESULT THREAD_CALL CommSession::proxyReadThreadStarter(void *pArg)
{
   ((CommSession *)pArg)->proxyReadThread();
   return THREAD_OK;
}

/**
 * Client session class constructor
 */
CommSession::CommSession(SOCKET hSocket, const InetAddress &serverAddr, bool masterServer, bool controlServer) : m_downloadFileMap(true)
{
   m_id = InterlockedIncrement(&s_sessionId);
   m_index = INVALID_INDEX;
   m_sendQueue = new Queue;
   m_processingQueue = new Queue;
   m_hSocket = hSocket;
   m_hProxySocket = INVALID_SOCKET;
   m_hWriteThread = INVALID_THREAD_HANDLE;
   m_hProcessingThread = INVALID_THREAD_HANDLE;
   m_hProxyReadThread = INVALID_THREAD_HANDLE;
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
   m_compressor = NULL;
   m_pCtx = NULL;
   m_ts = time(NULL);
   m_socketWriteMutex = MutexCreate();
   m_responseQueue = new MsgWaitQueue();
   m_requestId = 0;
}

/**
 * Destructor
 */
CommSession::~CommSession()
{
   if (m_proxyConnection)
      InterlockedDecrement(&s_activeProxySessions);

   shutdown(m_hSocket, SHUT_RDWR);
   closesocket(m_hSocket);
   if (m_hProxySocket != INVALID_SOCKET)
      closesocket(m_hProxySocket);

   void *p;
   while((p = m_sendQueue->get()) != NULL)
      if (p != INVALID_POINTER_VALUE)
         free(p);
   delete m_sendQueue;

   while((p = m_processingQueue->get()) != NULL)
      if (p != INVALID_POINTER_VALUE)
         delete (NXCPMessage *)p;
   delete m_processingQueue;
   delete m_compressor;
	if ((m_pCtx != NULL) && (m_pCtx != PROXY_ENCRYPTION_CTX))
		m_pCtx->decRefCount();
	MutexDestroy(m_socketWriteMutex);
   delete m_responseQueue;
}

/**
 * Debug print in session context
 */
void CommSession::debugPrintf(int level, const TCHAR *format, ...)
{
   if (level > nxlog_get_debug_level())
      return;

   va_list args;
   TCHAR buffer[8192];

   va_start(args, format);
   _vsntprintf(buffer, 8192, format, args);
   va_end(args);

   nxlog_write(MSG_DEBUG_SESSION, EVENTLOG_DEBUG_TYPE, "dds", m_index, m_id, buffer);
}

/**
 * Start all threads
 */
void CommSession::run()
{
   m_hWriteThread = ThreadCreateEx(writeThreadStarter, 0, this);
   m_hProcessingThread = ThreadCreateEx(processingThreadStarter, 0, this);
   ThreadCreate(readThreadStarter, 0, this);
}

/**
 * Disconnect session
 */
void CommSession::disconnect()
{
	debugPrintf(5, _T("CommSession::disconnect()"));
   shutdown(m_hSocket, SHUT_RDWR);
   if (m_hProxySocket != -1)
      shutdown(m_hProxySocket, SHUT_RDWR);
   m_disconnected = true;
}

/**
 * Reading thread
 */
void CommSession::readThread()
{
   SocketMessageReceiver receiver(m_hSocket, 4096, MAX_AGENT_MSG_SIZE);
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
                  const BYTE *data;
                  int dataSize;
                  if (msg->isCompressed())
                  {
                     const BYTE *in = msg->getBinaryData();
                     if (m_compressor == NULL)
                     {
                        NXCPCompressionMethod method = (NXCPCompressionMethod)(*in);
                        m_compressor = StreamCompressor::create(method, false, FILE_BUFFER_SIZE);
                        if (m_compressor == NULL)
                        {
                           debugPrintf(5, _T("Unable to create stream compressor for method %d"), (int)method);
                           data = NULL;
                           dataSize = -1;
                        }
                     }

                     if (m_compressor != NULL)
                     {
                        dataSize = (int)m_compressor->decompress(in + 4, msg->getBinaryDataSize() - 4, &data);
                        if (dataSize != (int)ntohs(*((UINT16 *)(in + 2))))
                        {
                           // decompressed block size validation failed
                           dataSize = -1;
                        }
                     }
                  }
                  else
                  {
                     data = msg->getBinaryData();
                     dataSize = (int)msg->getBinaryDataSize();
                  }

                  if ((dataSize >= 0) && dInfo->write(data, dataSize))
                  {
                     if (msg->isEndOfFile())
                     {
                        NXCPMessage response;

                        dInfo->close(true);
                        m_downloadFileMap.remove(msg->getId());
                        delete_and_null(m_compressor);

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
                     delete_and_null(m_compressor);

                     response.setCode(CMD_REQUEST_COMPLETED);
                     response.setId(msg->getId());
                     response.setField(VID_RCC, ERR_IO_FAILURE);
                     sendMessage(&response);
                  }
               }
            }
         }
         else if (msg->isControl())
         {
            TCHAR buffer[64];
            debugPrintf(6, _T("Received control message %s"), NXCPMessageCodeName(msg->getCode(), buffer));

            if (msg->getCode() == CMD_GET_NXCP_CAPS)
            {
               NXCP_MESSAGE *pMsg = (NXCP_MESSAGE *)malloc(NXCP_HEADER_SIZE);
               pMsg->id = htonl(msg->getId());
               pMsg->code = htons((WORD)CMD_NXCP_CAPS);
               pMsg->flags = htons(MF_CONTROL);
               pMsg->numFields = htonl(NXCP_VERSION << 24);
               pMsg->size = htonl(NXCP_HEADER_SIZE);
               sendRawMessage(pMsg, m_pCtx);
            }
            delete msg;
         }
         else
         {
            TCHAR buffer[64];
            debugPrintf(6, _T("Received message %s"), NXCPMessageCodeName(msg->getCode(), buffer));

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
         SocketPoller sp;
         sp.add(m_hSocket);
         int rc = sp.poll((g_dwIdleTimeout + 1) * 1000);
         if (rc <= 0)
            break;
         if (rc > 0)
         {
            // Update activity timestamp
            m_ts = time(NULL);

            char buffer[32768];
            rc = recv(m_hSocket, buffer, 32768, 0);
            if (rc <= 0)
               break;
            SendEx(m_hProxySocket, buffer, rc, 0, NULL);
         }
      }
   }

   // Notify other threads to exit
   m_sendQueue->put(INVALID_POINTER_VALUE);
   m_processingQueue->put(INVALID_POINTER_VALUE);
   if (m_hProxySocket != INVALID_SOCKET)
      shutdown(m_hProxySocket, SHUT_RDWR);

   // Wait for other threads to finish
   ThreadJoin(m_hWriteThread);
   ThreadJoin(m_hProcessingThread);
   if (m_proxyConnection)
      ThreadJoin(m_hProxyReadThread);

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
   TCHAR buffer[128];
   debugPrintf(6, _T("Sending message %s (size %d)"), NXCPMessageCodeName(ntohs(msg->code), buffer), ntohl(msg->size));
   if (nxlog_get_debug_level() >= 8)
   {
      String msgDump = NXCPMessage::dump(msg, NXCP_VERSION);
      debugPrintf(8, _T("Outgoing message dump:\n%s"), (const TCHAR *)msgDump);
   }
   if ((ctx != NULL) && (ctx != PROXY_ENCRYPTION_CTX))
   {
      NXCP_ENCRYPTED_MESSAGE *enMsg = ctx->encryptMessage(msg);
      if (enMsg != NULL)
      {
         if (SendEx(m_hSocket, (const char *)enMsg, ntohl(enMsg->size), 0, m_socketWriteMutex) <= 0)
         {
            success = false;
         }
         free(enMsg);
      }
   }
   else
   {
      if (SendEx(m_hSocket, (const char *)msg, ntohl(msg->size), 0, m_socketWriteMutex) <= 0)
      {
         success = false;
      }
   }
	if (!success)
	   debugPrintf(6, _T("CommSession::SendRawMessage() for %s (size %d) failed"), NXCPMessageCodeName(ntohs(msg->code), buffer), ntohl(msg->size));
   free(msg);
   return success;
}

/**
 * Send message directly to socket
 */
bool CommSession::sendMessage(NXCPMessage *msg)
{
   if (m_disconnected)
      return false;

   return sendRawMessage(msg->createMessage(), m_pCtx);
}

/**
 * Send raw message directly to socket
 */
bool CommSession::sendRawMessage(NXCP_MESSAGE *msg)
{
   return sendRawMessage((NXCP_MESSAGE *)nx_memdup(msg, ntohl(msg->size)), m_pCtx);
}

/**
 * Writing thread
 */
void CommSession::writeThread()
{
   while(true)
   {
      NXCP_MESSAGE *msg = (NXCP_MESSAGE *)m_sendQueue->getOrBlock();
      if (msg == INVALID_POINTER_VALUE)    // Session termination indicator
         break;

      if (!sendRawMessage(msg, m_pCtx))
         break;
   }
   debugPrintf(6, _T("writer thread stopped"));
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
            case CMD_SET_SERVER_CAPABILITIES:
               // Servers before 2.0 use VID_ENABLED
               m_ipv6Aware = request->isFieldExist(VID_IPV6_SUPPORT) ? request->getFieldAsBoolean(VID_IPV6_SUPPORT) : request->getFieldAsBoolean(VID_ENABLED);
               m_bulkReconciliationSupported = request->getFieldAsBoolean(VID_BULK_RECONCILIATION);
               response.setField(VID_RCC, ERR_SUCCESS);
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
            default:
               // Attempt to process unknown command by subagents
               if (!ProcessCmdBySubAgent(command, request, &response, this))
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
   TCHAR szParameter[MAX_PARAM_NAME], szValue[MAX_RESULT_LENGTH];
   UINT32 dwErrorCode;

   pRequest->getFieldAsString(VID_PARAMETER, szParameter, MAX_PARAM_NAME);
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
   TCHAR szParameter[MAX_PARAM_NAME];
   pRequest->getFieldAsString(VID_PARAMETER, szParameter, MAX_PARAM_NAME);

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
   TCHAR szParameter[MAX_PARAM_NAME];

   pRequest->getFieldAsString(VID_PARAMETER, szParameter, MAX_PARAM_NAME);

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
   {
      // Get action name and arguments
      TCHAR action[MAX_PARAM_NAME];
      pRequest->getFieldAsString(VID_ACTION_NAME, action, MAX_PARAM_NAME);

      int numArgs = pRequest->getFieldAsInt32(VID_NUM_ARGS);
      StringList *args = new StringList;
      for(int i = 0; i < numArgs; i++)
			args->addPreallocated(pRequest->getFieldAsString(VID_ACTION_ARG_BASE + i));

      // Execute action
      if (pRequest->getFieldAsBoolean(VID_RECEIVE_OUTPUT))
      {
         UINT32 rcc = ExecActionWithOutput(this, pRequest->getId(), action, args);
         pMsg->setField(VID_RCC, rcc);
      }
      else
      {
         UINT32 rcc = ExecAction(action, args, this);
         pMsg->setField(VID_RCC, rcc);
         delete args;
      }
   }
   else
   {
      pMsg->setField(VID_RCC, ERR_ACCESS_DENIED);
   }
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
      pMsg->setField(VID_RCC, openFile(szFullPath, pRequest->getId()));
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
bool CommSession::sendFile(UINT32 requestId, const TCHAR *file, long offset)
{
   if (m_disconnected)
      return false;
	return SendFileOverNXCP(m_hSocket, requestId, file, m_pCtx, offset, SendFileProgressCallback, this, m_socketWriteMutex) ? true : false;
}

/**
 * Upgrade agent from package in the file store
 */
UINT32 CommSession::upgrade(NXCPMessage *pRequest)
{
   if (m_masterServer)
   {
      TCHAR szPkgName[MAX_PATH], szFullPath[MAX_PATH];

      szPkgName[0] = 0;
      pRequest->getFieldAsString(VID_FILE_NAME, szPkgName, MAX_PATH);
      BuildFullPath(szPkgName, szFullPath);

      //Create line in registry file with upgrade file name to delete it after system start
      DB_HANDLE hdb = GetLocalDatabaseHandle();
      if(hdb != NULL)
      {
         TCHAR upgradeFileInsert[256];
         _sntprintf(upgradeFileInsert, 256, _T("INSERT INTO registry (attribute,value) VALUES ('upgrade.file',%s)"), (const TCHAR *)DBPrepareString(hdb, szPkgName));
         DBQuery(hdb, upgradeFileInsert);
      }

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
      BYTE *pConfig;
      int hFile;
      UINT32 size;

      if (pRequest->isFieldExist(VID_CONFIG_FILE))
      {
         size = pRequest->getFieldAsBinary(VID_CONFIG_FILE, NULL, 0);
         pConfig = (BYTE *)malloc(size);
         pRequest->getFieldAsBinary(VID_CONFIG_FILE, pConfig, size);
         hFile = _topen(g_szConfigFile, O_CREAT | O_TRUNC | O_WRONLY, 0644);
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
            if (_write(hFile, pConfig, size) == size)
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
UINT32 CommSession::setupProxyConnection(NXCPMessage *pRequest)
{
   UINT32 dwResult, dwAddr;
   WORD wPort;
   struct sockaddr_in sa;
   NXCPEncryptionContext *pSavedCtx;
   TCHAR szBuffer[32];

   if (m_masterServer && (g_dwFlags & AF_ENABLE_PROXY))
   {
      dwAddr = pRequest->getFieldAsUInt32(VID_IP_ADDRESS);
      wPort = pRequest->getFieldAsUInt16(VID_AGENT_PORT);
      m_hProxySocket = socket(AF_INET, SOCK_STREAM, 0);
      if (m_hProxySocket != INVALID_SOCKET)
      {
         // Fill in address structure
         memset(&sa, 0, sizeof(sa));
         sa.sin_addr.s_addr = htonl(dwAddr);
         sa.sin_family = AF_INET;
         sa.sin_port = htons(wPort);
         if (connect(m_hProxySocket, (struct sockaddr *)&sa, sizeof(sa)) != -1)
         {
            NXCPMessage msg;
            NXCP_MESSAGE *pRawMsg;

            // Stop writing thread
            m_sendQueue->put(INVALID_POINTER_VALUE);

            // Wait while all queued messages will be sent
            while(m_sendQueue->size() > 0)
               ThreadSleepMs(100);

            // Finish proxy connection setup
            pSavedCtx = m_pCtx;
            m_pCtx = PROXY_ENCRYPTION_CTX;
            m_proxyConnection = true;
            dwResult = ERR_SUCCESS;
            m_hProxyReadThread = ThreadCreateEx(proxyReadThreadStarter, 0, this);

            // Send confirmation message
            // We cannot use sendMessage() and writing thread, because
            // encryption context already overriden, and writing thread
            // already stopped
            msg.setCode(CMD_REQUEST_COMPLETED);
            msg.setId(pRequest->getId());
            msg.setField(VID_RCC, RCC_SUCCESS);
            pRawMsg = msg.createMessage();
            sendRawMessage(pRawMsg, pSavedCtx);
				if (pSavedCtx != NULL)
					pSavedCtx->decRefCount();

            debugPrintf(5, _T("Established proxy connection to %s:%d"), IpToStr(dwAddr, szBuffer), wPort);
         }
         else
         {
            dwResult = ERR_CONNECT_FAILED;
         }
      }
      else
      {
         dwResult = ERR_SOCKET_ERROR;
      }
   }
   else
   {
      dwResult = ERR_ACCESS_DENIED;
   }
   return dwResult;
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
         SendEx(m_hSocket, buffer, rc, 0, m_socketWriteMutex);
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

/**
 * Generate new request ID
 */
UINT32 CommSession::generateRequestId()
{
   return (UINT32)InterlockedIncrement(&m_requestId);
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
 * Debug print in virtual session context
 */
void VirtualSession::debugPrintf(int level, const TCHAR *format, ...)
{
   if (level > nxlog_get_debug_level())
      return;

   va_list args;
   TCHAR buffer[8192];

   va_start(args, format);
   _vsntprintf(buffer, 8192, format, args);
   va_end(args);

   nxlog_write(MSG_DEBUG_VSESSION, EVENTLOG_DEBUG_TYPE, "ds", m_id, buffer);
}
