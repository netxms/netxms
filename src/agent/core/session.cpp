/*
** NetXMS multiplatform core agent
** Copyright (C) 2003-2014 Victor Kirhenshtein
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

#ifdef _WIN32
#define write	_write
#define close	_close
#endif

/**
 * Externals
 */
void UnregisterSession(UINT32 dwIndex);
void ProxySNMPRequest(NXCPMessage *pRequest, NXCPMessage *pResponse);
UINT32 DeployPolicy(CommSession *session, NXCPMessage *request);
UINT32 UninstallPolicy(CommSession *session, NXCPMessage *request);
UINT32 GetPolicyInventory(CommSession *session, NXCPMessage *msg);
void ClearDataCollectionConfiguration();

/**
 * Max message size
 */
#define MAX_MSG_SIZE    4194304

/**
 * Client communication read thread
 */
THREAD_RESULT THREAD_CALL CommSession::readThreadStarter(void *pArg)
{
   ((CommSession *)pArg)->readThread();

   // When CommSession::ReadThread exits, all other session
   // threads are already stopped, so we can safely destroy
   // session object
   UnregisterSession(((CommSession *)pArg)->getIndex());
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
CommSession::CommSession(SOCKET hSocket, const InetAddress &serverAddr, bool masterServer, bool controlServer)
{
   m_sendQueue = new Queue;
   m_processingQueue = new Queue;
   m_hSocket = hSocket;
   m_hProxySocket = INVALID_SOCKET;
   m_dwIndex = INVALID_INDEX;
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
   m_acceptFileUpdates = false;
   m_ipv6Aware = false;
   m_hCurrFile = -1;
   m_fileRqId = 0;
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
   shutdown(m_hSocket, SHUT_RDWR);
   closesocket(m_hSocket);
   if (m_hProxySocket != INVALID_SOCKET)
      closesocket(m_hProxySocket);
   delete m_sendQueue;
   delete m_processingQueue;
   if (m_hCurrFile != -1)
      close(m_hCurrFile);
   delete m_compressor;
	if ((m_pCtx != NULL) && (m_pCtx != PROXY_ENCRYPTION_CTX))
		m_pCtx->decRefCount();
	MutexDestroy(m_socketWriteMutex);
   delete m_responseQueue;
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
	DebugPrintf(m_dwIndex, 5, _T("CommSession::disconnect()"));
   shutdown(m_hSocket, SHUT_RDWR);
   if (m_hProxySocket != -1)
      shutdown(m_hProxySocket, SHUT_RDWR);
}

/**
 * Reading thread
 */
void CommSession::readThread()
{
   SocketMessageReceiver receiver(m_hSocket, 4096, MAX_MSG_SIZE);
   while(true)
   {
      if (!m_proxyConnection)
      {
         MessageReceiverResult result;
         NXCPMessage *msg = receiver.readMessage((g_dwIdleTimeout + 1) * 1000, &result);

         // Check for decryption error
         if (result == MSGRECV_DECRYPTION_FAILURE)
         {
            DebugPrintf(m_dwIndex, 4, _T("Unable to decrypt received message"));
            continue;
         }

         // Check for timeout
         if (result == MSGRECV_TIMEOUT)
         {
            if (m_ts < time(NULL) - (time_t)g_dwIdleTimeout)
            {
               DebugPrintf(m_dwIndex, 5, _T("Session disconnected by timeout (last activity timestamp is %d)"), (int)m_ts);
               break;
            }
            continue;
         }

         // Receive error
         if (msg == NULL)
         {
            DebugPrintf(m_dwIndex, 5, _T("Message receiving error (%s)"), AbstractMessageReceiver::resultToText(result));
            break;
         }

         // Update activity timestamp
         m_ts = time(NULL);

         if (g_debugLevel >= 8)
         {
            String msgDump = NXCPMessage::dump(receiver.getRawMessageBuffer(), NXCP_VERSION);
            DebugPrintf(m_dwIndex, 8, _T("Message dump:\n%s"), (const TCHAR *)msgDump);
         }

         if (msg->isBinary())
         {
            TCHAR buffer[64];
            DebugPrintf(m_dwIndex, 6, _T("Received raw message %s"), NXCPMessageCodeName(msg->getCode(), buffer));

            if (msg->getCode() == CMD_FILE_DATA)
            {
               if ((m_hCurrFile != -1) && (m_fileRqId == msg->getId()))
               {
                  const BYTE *data;
                  int dataSize;
                  if (msg->isCompressed())
                  {
                     BYTE *in = msg->getBinaryData();
                     if (m_compressor == NULL)
                     {
                        NXCPCompressionMethod method = (NXCPCompressionMethod)(*in);
                        m_compressor = StreamCompressor::create(method, false, FILE_BUFFER_SIZE);
                        if (m_compressor == NULL)
                        {
                           DebugPrintf(m_dwIndex, 5, _T("Unable to create stream compressor for method %d"), (int)method);
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

                  if ((dataSize >= 0) && (write(m_hCurrFile, data, dataSize) == dataSize))
                  {
                     if (msg->isEndOfFile())
                     {
                        NXCPMessage response;

                        close(m_hCurrFile);
                        m_hCurrFile = -1;
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

                     close(m_hCurrFile);
                     m_hCurrFile = -1;
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
            DebugPrintf(m_dwIndex, 6, _T("Received control message %s"), NXCPMessageCodeName(msg->getCode(), buffer));

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
            DebugPrintf(m_dwIndex, 6, _T("Received message %s"), NXCPMessageCodeName(msg->getCode(), buffer));

            if (msg->getCode() == CMD_REQUEST_SESSION_KEY)
            {
               if (m_pCtx == NULL)
               {
                  NXCPMessage *pResponse;
                  SetupEncryptionContext(msg, &m_pCtx, &pResponse, NULL, NXCP_VERSION);
                  sendMessage(pResponse);
                  delete pResponse;
                  receiver.setEncryptionContext(m_pCtx);
               }
               delete msg;
            }
            else if (msg->getCode() == CMD_REQUEST_COMPLETED)
            {
               m_responseQueue->put(msg);
            }
            else if (msg->getCode() == CMD_SETUP_PROXY_CONNECTION)
            {
               DWORD rcc = setupProxyConnection(msg);
               // When proxy session established incoming messages will
               // not be processed locally. Acknowledgement message sent
               // by SetupProxyConnection() in case of success.
               if (rcc == ERR_SUCCESS)
               {
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
            }
            else
            {
               m_processingQueue->put(msg);
            }
         }
      }
      else  // m_proxyConnection
      {
         fd_set rdfs;
         struct timeval tv;
         char buffer[32768];

         FD_ZERO(&rdfs);
         FD_SET(m_hSocket, &rdfs);
         tv.tv_sec = g_dwIdleTimeout + 1;
         tv.tv_usec = 0;
         int rc = select(SELECT_NFDS(m_hSocket + 1), &rdfs, NULL, NULL, &tv);
         if (rc <= 0)
            break;
         if (rc > 0)
         {
            // Update activity timestamp
            m_ts = time(NULL);

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

   DebugPrintf(m_dwIndex, 5, _T("Session with %s closed"), (const TCHAR *)m_serverAddr.toString());
}

/**
 * Send prepared raw message over the network and destroy it
 */
BOOL CommSession::sendRawMessage(NXCP_MESSAGE *pMsg, NXCPEncryptionContext *pCtx)
{
   BOOL bResult = TRUE;
   TCHAR szBuffer[128];

   DebugPrintf(m_dwIndex, 6, _T("Sending message %s (size %d)"), NXCPMessageCodeName(ntohs(pMsg->code), szBuffer), ntohl(pMsg->size));
   if ((pCtx != NULL) && (pCtx != PROXY_ENCRYPTION_CTX))
   {
      NXCP_ENCRYPTED_MESSAGE *enMsg = pCtx->encryptMessage(pMsg);
      if (enMsg != NULL)
      {
         if (SendEx(m_hSocket, (const char *)enMsg, ntohl(enMsg->size), 0, m_socketWriteMutex) <= 0)
         {
            bResult = FALSE;
         }
         free(enMsg);
      }
   }
   else
   {
      if (SendEx(m_hSocket, (const char *)pMsg, ntohl(pMsg->size), 0, m_socketWriteMutex) <= 0)
      {
         bResult = FALSE;
      }
   }
	if (!bResult)
	   DebugPrintf(m_dwIndex, 6, _T("CommSession::SendRawMessage() for %s (size %d) failed"), NXCPMessageCodeName(ntohs(pMsg->code), szBuffer), ntohl(pMsg->size));
   free(pMsg);
   return bResult;
}

/**
 * Writing thread
 */
void CommSession::writeThread()
{
   NXCP_MESSAGE *pMsg;

   while(1)
   {
      pMsg = (NXCP_MESSAGE *)m_sendQueue->getOrBlock();
      if (pMsg == INVALID_POINTER_VALUE)    // Session termination indicator
         break;

      if (!sendRawMessage(pMsg, m_pCtx))
         break;
   }
   m_sendQueue->clear();
}

/**
 * Message processing thread
 */
void CommSession::processingThread()
{
   NXCPMessage *pMsg;
   TCHAR szBuffer[128];
   NXCPMessage msg;
   UINT32 dwCommand, dwRet;

   while(1)
   {
      pMsg = (NXCPMessage *)m_processingQueue->getOrBlock();
      if (pMsg == INVALID_POINTER_VALUE)    // Session termination indicator
         break;
      dwCommand = pMsg->getCode();

      // Prepare response message
      msg.setCode(CMD_REQUEST_COMPLETED);
      msg.setId(pMsg->getId());

      // Check if authentication required
      if ((!m_authenticated) && (dwCommand != CMD_AUTHENTICATE))
      {
			DebugPrintf(m_dwIndex, 6, _T("Authentication required"));
         msg.setField(VID_RCC, ERR_AUTH_REQUIRED);
      }
      else if ((g_dwFlags & AF_REQUIRE_ENCRYPTION) && (m_pCtx == NULL))
      {
			DebugPrintf(m_dwIndex, 6, _T("Encryption required"));
         msg.setField(VID_RCC, ERR_ENCRYPTION_REQUIRED);
      }
      else
      {
         switch(dwCommand)
         {
            case CMD_AUTHENTICATE:
               authenticate(pMsg, &msg);
               break;
            case CMD_GET_PARAMETER:
               getParameter(pMsg, &msg);
               break;
            case CMD_GET_LIST:
               getList(pMsg, &msg);
               break;
            case CMD_GET_TABLE:
               getTable(pMsg, &msg);
               break;
            case CMD_KEEPALIVE:
               msg.setField(VID_RCC, ERR_SUCCESS);
               break;
            case CMD_ACTION:
               action(pMsg, &msg);
               break;
            case CMD_TRANSFER_FILE:
               recvFile(pMsg, &msg);
               break;
            case CMD_UPGRADE_AGENT:
               msg.setField(VID_RCC, upgrade(pMsg));
               break;
            case CMD_GET_PARAMETER_LIST:
               msg.setField(VID_RCC, ERR_SUCCESS);
               GetParameterList(&msg);
               break;
            case CMD_GET_ENUM_LIST:
               msg.setField(VID_RCC, ERR_SUCCESS);
               GetEnumList(&msg);
               break;
            case CMD_GET_TABLE_LIST:
               msg.setField(VID_RCC, ERR_SUCCESS);
               GetTableList(&msg);
               break;
            case CMD_GET_AGENT_CONFIG:
               getConfig(&msg);
               break;
            case CMD_UPDATE_AGENT_CONFIG:
               updateConfig(pMsg, &msg);
               break;
            case CMD_ENABLE_AGENT_TRAPS:
               if (m_masterServer)
               {
                  m_acceptTraps = true;
                  msg.setField(VID_RCC, ERR_SUCCESS);
               }
               else
               {
                  msg.setField(VID_RCC, ERR_ACCESS_DENIED);
               }
               break;
            case CMD_ENABLE_FILE_UPDATES:
               if (m_masterServer)
               {
                  m_acceptFileUpdates = true;
                  msg.setField(VID_RCC, ERR_SUCCESS);
               }
               else
               {
                  msg.setField(VID_RCC, ERR_ACCESS_DENIED);
               }
               break;
				case CMD_SNMP_REQUEST:
					if (m_masterServer && (g_dwFlags & AF_ENABLE_SNMP_PROXY))
					{
						ProxySNMPRequest(pMsg, &msg);
					}
					else
					{
                  msg.setField(VID_RCC, ERR_ACCESS_DENIED);
					}
					break;
				case CMD_DEPLOY_AGENT_POLICY:
					if (m_masterServer)
					{
						msg.setField(VID_RCC, DeployPolicy(this, pMsg));
					}
					else
					{
                  msg.setField(VID_RCC, ERR_ACCESS_DENIED);
					}
					break;
				case CMD_UNINSTALL_AGENT_POLICY:
					if (m_masterServer)
					{
						msg.setField(VID_RCC, UninstallPolicy(this, pMsg));
					}
					else
					{
                  msg.setField(VID_RCC, ERR_ACCESS_DENIED);
					}
					break;
				case CMD_GET_POLICY_INVENTORY:
					if (m_masterServer)
					{
						msg.setField(VID_RCC, GetPolicyInventory(this, &msg));
					}
					else
					{
                  msg.setField(VID_RCC, ERR_ACCESS_DENIED);
					}
					break;
            case CMD_TAKE_SCREENSHOT:
					if (m_controlServer)
					{
                  TCHAR sessionName[256];
                  pMsg->getFieldAsString(VID_NAME, sessionName, 256);
                  DebugPrintf(m_dwIndex, 6, _T("Take snapshot from session \"%s\""), sessionName);
                  SessionAgentConnector *conn = AcquireSessionAgentConnector(sessionName);
                  if (conn != NULL)
                  {
                     DebugPrintf(m_dwIndex, 6, _T("Session agent connector acquired"));
                     conn->takeScreenshot(&msg);
                     conn->decRefCount();
                  }
                  else
                  {
                     msg.setField(VID_RCC, ERR_NO_SESSION_AGENT);
                  }
					}
					else
					{
                  msg.setField(VID_RCC, ERR_ACCESS_DENIED);
					}
					break;
            case CMD_ENABLE_IPV6:
               m_ipv6Aware = pMsg->getFieldAsBoolean(VID_ENABLED);
               msg.setField(VID_RCC, ERR_SUCCESS);
               break;
            case CMD_SET_SERVER_ID:
               m_serverId = pMsg->getFieldAsUInt64(VID_SERVER_ID);
               DebugPrintf(m_dwIndex, 1, _T("Server ID set to ") UINT64X_FMT(_T("016")), m_serverId);
               msg.setField(VID_RCC, ERR_SUCCESS);
               break;
            case CMD_DATA_COLLECTION_CONFIG:
               if (m_serverId != 0)
               {
                  ConfigureDataCollection(m_serverId, pMsg);
                  msg.setField(VID_RCC, ERR_SUCCESS);
               }
               else
               {
                  DebugPrintf(m_dwIndex, 1, _T("Data collection configuration command received but server ID is not set"));
                  msg.setField(VID_RCC, ERR_SERVER_ID_UNSET);
               }
               break;
            case CMD_CLEAN_AGENT_DCI_CONF:
               if (m_masterServer)
               {
                  ClearDataCollectionConfiguration();
                  msg.setField(VID_RCC, ERR_SUCCESS);
               }
               else
               {
                  msg.setField(VID_RCC, ERR_ACCESS_DENIED);
               }
               break;
            default:
               // Attempt to process unknown command by subagents
               if (!ProcessCmdBySubAgent(dwCommand, pMsg, &msg, this))
                  msg.setField(VID_RCC, ERR_UNKNOWN_COMMAND);
               break;
         }
      }
      delete pMsg;

      // Send response
      sendMessage(&msg);
      msg.deleteAllFields();
   }

stop_processing:
   ;
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
               nxlog_write(MSG_AUTH_FAILED, EVENTLOG_WARNING_TYPE, "Is", &m_serverAddr, "PLAIN");
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
   dwErrorCode = GetParameterValue(m_dwIndex, szParameter, szValue, this);
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
   UINT32 dwErrorCode = GetListValue(m_dwIndex, szParameter, &value, this);
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
   UINT32 dwErrorCode = GetTableValue(m_dwIndex, szParameter, &value, this);
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
		DebugPrintf(m_dwIndex, 5, _T("CommSession::recvFile(): Preparing for receiving file \"%s\""), szFileName);
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
UINT32 CommSession::openFile(TCHAR *szFullPath, UINT32 requestId)
{
   if (m_hCurrFile != -1)
   {
      return ERR_RESOURCE_BUSY;
   }
   else
   {
      DebugPrintf(m_dwIndex, 5, _T("CommSession::recvFile(): Writing to local file \"%s\""), szFullPath);
      m_hCurrFile = _topen(szFullPath, O_CREAT | O_TRUNC | O_WRONLY | O_BINARY, 0600);
      if (m_hCurrFile == -1)
      {
         DebugPrintf(m_dwIndex, 2, _T("CommSession::recvFile(): Error opening file \"%s\" for writing (%s)"), szFullPath, _tcserror(errno));
         return ERR_IO_FAILURE;
      }
      else
      {
         m_fileRqId = requestId;
         return ERR_SUCCESS;
      }
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
      Config *registry = AgentOpenRegistry();
      registry->setValue(_T("/upgrade/file"), szFullPath);
      AgentCloseRegistry(true);

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
            if (write(hFile, pConfig, size) == size)
               pMsg->setField(VID_RCC, ERR_SUCCESS);
            else
               pMsg->setField(VID_RCC, ERR_IO_FAILURE);
            close(hFile);
         }
         else
         {
				DebugPrintf(m_dwIndex, 2, _T("CommSession::updateConfig(): Error opening file \"%s\" for writing (%s)"),
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

            DebugPrintf(m_dwIndex, 5, _T("Established proxy connection to %s:%d"), IpToStr(dwAddr, szBuffer), wPort);
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
   fd_set rdfs;
   struct timeval tv;
   char buffer[32768];
   int nRet;

   while(1)
   {
      FD_ZERO(&rdfs);
      FD_SET(m_hProxySocket, &rdfs);
      tv.tv_sec = 0;
      tv.tv_usec = 5000000;   // Half-second timeout
      nRet = select(SELECT_NFDS(m_hProxySocket + 1), &rdfs, NULL, NULL, &tv);
      if (nRet < 0)
         break;
      if (nRet > 0)
      {
         nRet = recv(m_hProxySocket, buffer, 32768, 0);
         if (nRet <= 0)
            break;
         SendEx(m_hSocket, buffer, nRet, 0, m_socketWriteMutex);
      }
   }
   disconnect();
}

/**
 * Wait for request completion
 */
UINT32 CommSession::doRequest(NXCPMessage *msg, UINT32 timeout)
{
   sendMessage(msg);
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
 * Generate new request ID
 */
UINT32 CommSession::generateRequestId()
{
   return (UINT32)InterlockedIncrement(&m_requestId);
}
