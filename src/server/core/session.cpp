/*
** NetXMS - Network Management System
** Copyright (C) 2003-2015 Raden Solutions
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

#include "nxcore.h"
#include <netxms_mt.h>
#include <nxtools.h>

#ifdef _WIN32
#include <psapi.h>
#define write	_write
#define close	_close
#else
#include <dirent.h>
#endif

#ifndef min
#define min(a,b) ((a) < (b) ? (a) : (b))
#endif

#ifndef S_ISREG
#define S_ISREG(m)     (((m) & S_IFMT) == S_IFREG)
#endif

// WARNING! this hack works only for d2i_X509(); be carefull when adding new code
#ifdef OPENSSL_CONST
# undef OPENSSL_CONST
#endif
#if OPENSSL_VERSION_NUMBER >= 0x0090800fL
# define OPENSSL_CONST const
#else
# define OPENSSL_CONST
#endif

/**
 * Constants
 */
#define TRAP_CREATE     1
#define TRAP_UPDATE     2
#define TRAP_DELETE     3

#define MAX_MSG_SIZE    4194304

/**
 * Externals
 */
extern Queue g_nodePollerQueue;
extern Queue g_dataCollectionQueue;
extern Queue g_dciCacheLoaderQueue;

void UnregisterClientSession(int id);
void ResetDiscoveryPoller();
NXCPMessage *ForwardMessageToReportingServer(NXCPMessage *request, ClientSession *session);
void RemovePendingFileTransferRequests(ClientSession *session);

/**
 * Node poller start data
 */
typedef struct
{
   ClientSession *pSession;
   Node *pNode;
   int iPollType;
   UINT32 dwRqId;
} POLLER_START_DATA;

/**
 * Additional processing thread start data
 */
typedef struct
{
	ClientSession *pSession;
	NXCPMessage *pMsg;
} PROCTHREAD_START_DATA;

/**
 *
 */
typedef struct
{
  uuid_t *guid;
  bool removed;
} LIBRARY_IMAGE_UPDATE_INFO;

/**
 * Callback to delete agent connections for loading files in distructor
 */
static void DeleteCallback(NetObj* obj, void *data)
{
   delete (AgentConnection *)obj;
}

/**
 * Callback for sending image library update notifications
 */
static void ImageLibraryUpdateCallback(ClientSession *pSession, void *pArg)
{
	pSession->onLibraryImageChange((uuid_t *)pArg, false);
}

/**
 * Callback for sending image library delete notifications
 */
static void ImageLibraryDeleteCallback(ClientSession *pSession, void *pArg)
{
	pSession->onLibraryImageChange((uuid_t *)pArg, true);
}

/**
 * Additional message processing thread starters
 */
#define CALL_IN_NEW_THREAD(func, msg) \
{ \
	PROCTHREAD_START_DATA *pData = (PROCTHREAD_START_DATA *)malloc(sizeof(PROCTHREAD_START_DATA)); \
	pData->pSession = this; \
	pData->pMsg = msg; \
	msg = NULL; /* prevent deletion by main processing thread*/ \
	m_dwRefCount++; \
	ThreadPoolExecute(g_mainThreadPool, ThreadStarter_##func, pData); \
}

#define DEFINE_THREAD_STARTER(func) \
void ClientSession::ThreadStarter_##func(void *pArg) \
{ \
   ((PROCTHREAD_START_DATA *)pArg)->pSession->func(((PROCTHREAD_START_DATA *)pArg)->pMsg); \
	((PROCTHREAD_START_DATA *)pArg)->pSession->m_dwRefCount--; \
	delete ((PROCTHREAD_START_DATA *)pArg)->pMsg; \
	free(pArg); \
}

DEFINE_THREAD_STARTER(getCollectedData)
DEFINE_THREAD_STARTER(getTableCollectedData)
DEFINE_THREAD_STARTER(clearDCIData)
DEFINE_THREAD_STARTER(forceDCIPoll)
DEFINE_THREAD_STARTER(queryL2Topology)
DEFINE_THREAD_STARTER(sendEventLog)
DEFINE_THREAD_STARTER(sendSyslog)
DEFINE_THREAD_STARTER(createObject)
DEFINE_THREAD_STARTER(getServerFile)
DEFINE_THREAD_STARTER(getAgentFile)
DEFINE_THREAD_STARTER(cancelFileMonitoring)
DEFINE_THREAD_STARTER(queryServerLog)
DEFINE_THREAD_STARTER(getServerLogQueryData)
DEFINE_THREAD_STARTER(executeAction)
DEFINE_THREAD_STARTER(findNodeConnection)
DEFINE_THREAD_STARTER(findMacAddress)
DEFINE_THREAD_STARTER(findIpAddress)
DEFINE_THREAD_STARTER(processConsoleCommand)
DEFINE_THREAD_STARTER(sendMib)
DEFINE_THREAD_STARTER(getNetworkPath)
DEFINE_THREAD_STARTER(queryParameter)
DEFINE_THREAD_STARTER(queryAgentTable)
DEFINE_THREAD_STARTER(getAlarmEvents)
DEFINE_THREAD_STARTER(openHelpdeskIssue)
DEFINE_THREAD_STARTER(forwardToReportingServer)
DEFINE_THREAD_STARTER(fileManagerControl)
DEFINE_THREAD_STARTER(uploadUserFileToAgent)
DEFINE_THREAD_STARTER(getSwitchForwardingDatabase)
DEFINE_THREAD_STARTER(getRoutingTable)
DEFINE_THREAD_STARTER(getLocationHistory)
DEFINE_THREAD_STARTER(executeScript)

/**
 * Client communication read thread starter
 */
THREAD_RESULT THREAD_CALL ClientSession::readThreadStarter(void *pArg)
{
   ((ClientSession *)pArg)->readThread();

   // When ClientSession::ReadThread exits, all other session
   // threads are already stopped, so we can safely destroy
   // session object
   UnregisterClientSession(((ClientSession *)pArg)->getId());
   delete (ClientSession *)pArg;
   return THREAD_OK;
}

 /**
 * Client communication write thread starter
 */
THREAD_RESULT THREAD_CALL ClientSession::writeThreadStarter(void *pArg)
{
   ((ClientSession *)pArg)->writeThread();
   return THREAD_OK;
}

/**
 * Received message processing thread starter
 */
THREAD_RESULT THREAD_CALL ClientSession::processingThreadStarter(void *pArg)
{
   ((ClientSession *)pArg)->processingThread();
   return THREAD_OK;
}

/**
 * Information update processing thread starter
 */
THREAD_RESULT THREAD_CALL ClientSession::updateThreadStarter(void *pArg)
{
   ((ClientSession *)pArg)->updateThread();
   return THREAD_OK;
}

/**
 * Client session class constructor
 */
ClientSession::ClientSession(SOCKET hSocket, struct sockaddr *addr)
{
   m_pSendQueue = new Queue;
   m_pMessageQueue = new Queue;
   m_pUpdateQueue = new Queue;
   m_hSocket = hSocket;
   m_id = -1;
   m_state = SESSION_STATE_INIT;
   m_pCtx = NULL;
   m_hWriteThread = INVALID_THREAD_HANDLE;
   m_hProcessingThread = INVALID_THREAD_HANDLE;
   m_hUpdateThread = INVALID_THREAD_HANDLE;
	m_mutexSocketWrite = MutexCreate();
   m_mutexSendEvents = MutexCreate();
   m_mutexSendSyslog = MutexCreate();
   m_mutexSendTrapLog = MutexCreate();
   m_mutexSendObjects = MutexCreate();
   m_mutexSendAlarms = MutexCreate();
   m_mutexSendActions = MutexCreate();
   m_mutexSendAuditLog = MutexCreate();
   m_mutexSendSituations = MutexCreate();
   m_mutexPollerInit = MutexCreate();
   m_dwFlags = 0;
	m_clientType = CLIENT_TYPE_DESKTOP;
	m_clientAddr = (struct sockaddr *)nx_memdup(addr, (addr->sa_family == AF_INET) ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6));
	if (addr->sa_family == AF_INET)
		IpToStr(ntohl(((struct sockaddr_in *)m_clientAddr)->sin_addr.s_addr), m_workstation);
#ifdef WITH_IPV6
	else
		Ip6ToStr(((struct sockaddr_in6 *)m_clientAddr)->sin6_addr.s6_addr, m_workstation);
#endif
   m_webServerAddress[0] = 0;
   m_loginName[0] = 0;
   _tcscpy(m_sessionName, _T("<not logged in>"));
	_tcscpy(m_clientInfo, _T("n/a"));
   m_dwUserId = INVALID_INDEX;
   m_dwOpenDCIListSize = 0;
   m_pOpenDCIList = NULL;
   m_ppEPPRuleList = NULL;
   m_hCurrFile = -1;
   m_dwFileRqId = 0;
   m_dwRefCount = 0;
   m_dwEncryptionRqId = 0;
   m_condEncryptionSetup = INVALID_CONDITION_HANDLE;
   m_dwActiveChannels = 0;
	m_console = NULL;
   m_loginTime = time(NULL);
   m_musicTypeList.add(_T("wav"));
   _tcscpy(m_language, _T("en"));
}

/**
 * Destructor
 */
ClientSession::~ClientSession()
{
   if (m_hSocket != -1)
      closesocket(m_hSocket);
   delete m_pSendQueue;
   delete m_pMessageQueue;
   delete m_pUpdateQueue;
	safe_free(m_clientAddr);
	MutexDestroy(m_mutexSocketWrite);
   MutexDestroy(m_mutexSendEvents);
   MutexDestroy(m_mutexSendSyslog);
   MutexDestroy(m_mutexSendTrapLog);
   MutexDestroy(m_mutexSendObjects);
   MutexDestroy(m_mutexSendAlarms);
   MutexDestroy(m_mutexSendActions);
   MutexDestroy(m_mutexSendAuditLog);
   MutexDestroy(m_mutexSendSituations);
   MutexDestroy(m_mutexPollerInit);
   safe_free(m_pOpenDCIList);
   if (m_ppEPPRuleList != NULL)
   {
      UINT32 i;

      if (m_dwFlags & CSF_EPP_UPLOAD)  // Aborted in the middle of EPP transfer
         for(i = 0; i < m_dwRecordsUploaded; i++)
            delete m_ppEPPRuleList[i];
      free(m_ppEPPRuleList);
   }
	if (m_pCtx != NULL)
		m_pCtx->decRefCount();
   if (m_condEncryptionSetup != INVALID_CONDITION_HANDLE)
      ConditionDestroy(m_condEncryptionSetup);

	if (m_console != NULL)
	{
		delete m_console->pMsg;
		free(m_console);
	}
   m_musicTypeList.clear();
   if (m_agentConn.size() > 0)
   {
      m_agentConn.forEach(&DeleteCallback, NULL);
   }
}

/**
 * Start all threads
 */
void ClientSession::run()
{
   m_hWriteThread = ThreadCreateEx(writeThreadStarter, 0, this);
   m_hProcessingThread = ThreadCreateEx(processingThreadStarter, 0, this);
   m_hUpdateThread = ThreadCreateEx(updateThreadStarter, 0, this);
   ThreadCreate(readThreadStarter, 0, this);
}

/**
 * Print debug information
 */
void ClientSession::debugPrintf(int level, const TCHAR *format, ...)
{
   if (level <= (int)g_debugLevel)
   {
      va_list args;
		TCHAR buffer[8192];

      va_start(args, format);
      _vsntprintf(buffer, 8192, format, args);
      va_end(args);
		DbgPrintf(level, _T("[CLSN-%d] %s"), m_id, buffer);
   }
}

/**
 * Read thread
 */
void ClientSession::readThread()
{
   TCHAR szBuffer[256];
   UINT32 i;
   NetObj *object;

   SocketMessageReceiver receiver(m_hSocket, 4096, MAX_MSG_SIZE);
   while(true)
   {
      MessageReceiverResult result;
      NXCPMessage *msg = receiver.readMessage(900000, &result);

      // Check for decryption error
      if (result == MSGRECV_DECRYPTION_FAILURE)
      {
         debugPrintf(4, _T("readThread: Unable to decrypt received message"));
         continue;
      }

      // Receive error
      if (msg == NULL)
      {
         debugPrintf(5, _T("readThread: message receiving error (%s)"), AbstractMessageReceiver::resultToText(result));
         break;
      }

      if (g_debugLevel >= 8)
      {
         String msgDump = NXCPMessage::dump(receiver.getRawMessageBuffer(), NXCP_VERSION);
         debugPrintf(8, _T("Message dump:\n%s"), (const TCHAR *)msgDump);
      }

      // Special handling for raw messages
      if (msg->isBinary())
      {
         debugPrintf(6, _T("Received raw message %s"), NXCPMessageCodeName(msg->getCode(), szBuffer));

         if ((msg->getCode() == CMD_FILE_DATA) ||
             (msg->getCode() == CMD_ABORT_FILE_TRANSFER))
         {
            if ((m_hCurrFile != -1) && (m_dwFileRqId == msg->getId()))
            {
               if (msg->getCode() == CMD_FILE_DATA)
               {
                  if (write(m_hCurrFile, msg->getBinaryData(), (int)msg->getBinaryDataSize()) == (int)msg->getBinaryDataSize())
                  {
                     if (msg->isEndOfFile())
                     {
								debugPrintf(6, _T("Got end of file marker"));
                        NXCPMessage response;

                        close(m_hCurrFile);
                        m_hCurrFile = -1;

                        response.setCode(CMD_REQUEST_COMPLETED);
                        response.setId(msg->getId());
                        response.setField(VID_RCC, RCC_SUCCESS);
                        sendMessage(&response);

                        onFileUpload(TRUE);
                     }
                  }
                  else
                  {
							debugPrintf(6, _T("I/O error"));
                     // I/O error
                     NXCPMessage response;

                     close(m_hCurrFile);
                     m_hCurrFile = -1;

                     response.setCode(CMD_REQUEST_COMPLETED);
                     response.setId(msg->getId());
                     response.setField(VID_RCC, RCC_IO_ERROR);
                     sendMessage(&response);

                     onFileUpload(FALSE);
                  }
               }
               else
               {
                  // Abort current file transfer because of client's problem
                  close(m_hCurrFile);
                  m_hCurrFile = -1;

                  onFileUpload(FALSE);
               }
            }
            else
            {
               AgentConnection *conn = (AgentConnection *)m_agentConn.get(msg->getId());
               if (conn != NULL)
               {
                  if (msg->getCode() == CMD_FILE_DATA)
                  {
                     if (conn->sendMessage(msg))  //send raw message
                     {
                        if (msg->isEndOfFile())
                        {
                           debugPrintf(6, _T("Got end of file marker"));
                           //get response with specific ID if ok< then send ok, else send error
                           m_agentConn.remove(msg->getId());
                           delete conn;

                           NXCPMessage response;
                           response.setCode(CMD_REQUEST_COMPLETED);
                           response.setId(msg->getId());
                           response.setField(VID_RCC, RCC_SUCCESS);
                           sendMessage(&response);
                        }
                     }
                     else
                     {
                        debugPrintf(6, _T("Error while sending to agent"));
                        // I/O error
                        m_agentConn.remove(msg->getId());
                        delete conn;

                        NXCPMessage response;
                        response.setCode(CMD_REQUEST_COMPLETED);
                        response.setId(msg->getId());
                        response.setField(VID_RCC, RCC_IO_ERROR); //set result that came from agent
                        sendMessage(&response);
                     }
                  }
                  else
                  {
                     // Resend abort message
                     conn->sendMessage(msg);
                     m_agentConn.remove(msg->getId());
                     delete conn;
                  }
               }
               else
               {
                  debugPrintf(4, _T("Out of state message (ID: %d)"), msg->getId());
               }
            }
         }
         delete msg;
      }
      else
      {
         if ((msg->getCode() == CMD_SESSION_KEY) && (msg->getId() == m_dwEncryptionRqId))
         {
		      debugPrintf(6, _T("Received message %s"), NXCPMessageCodeName(msg->getCode(), szBuffer));
            m_dwEncryptionResult = SetupEncryptionContext(msg, &m_pCtx, NULL, g_pServerKey, NXCP_VERSION);
            receiver.setEncryptionContext(m_pCtx);
            ConditionSet(m_condEncryptionSetup);
            m_dwEncryptionRqId = 0;
            delete msg;
         }
         else if (msg->getCode() == CMD_KEEPALIVE)
			{
		      debugPrintf(6, _T("Received message %s"), NXCPMessageCodeName(msg->getCode(), szBuffer));
				respondToKeepalive(msg->getId());
				delete msg;
			}
			else
         {
            m_pMessageQueue->put(msg);
         }
      }
   }

   // Mark as terminated (sendMessage calls will not work after that point)
   m_dwFlags |= CSF_TERMINATED;

	// Finish update thread first
   m_pUpdateQueue->put(INVALID_POINTER_VALUE);
   ThreadJoin(m_hUpdateThread);

   // Notify other threads to exit
   NXCP_MESSAGE *rawMsg;
   while((rawMsg = (NXCP_MESSAGE *)m_pSendQueue->get()) != NULL)
      free(rawMsg);
   m_pSendQueue->put(INVALID_POINTER_VALUE);

   NXCPMessage *msg;
	while((msg = (NXCPMessage *)m_pMessageQueue->get()) != NULL)
		delete msg;
   m_pMessageQueue->put(INVALID_POINTER_VALUE);

   // Wait for other threads to finish
   ThreadJoin(m_hWriteThread);
   ThreadJoin(m_hProcessingThread);

   // Abort current file upload operation, if any
   if (m_hCurrFile != -1)
   {
      close(m_hCurrFile);
      m_hCurrFile = -1;
      onFileUpload(FALSE);
   }

   // remove all pending file transfers from reporting server
   RemovePendingFileTransferRequests(this);

   // Remove all locks created by this session
   RemoveAllSessionLocks(m_id);
   for(i = 0; i < m_dwOpenDCIListSize; i++)
   {
      object = FindObjectById(m_pOpenDCIList[i]);
      if (object != NULL)
         if ((object->getObjectClass() == OBJECT_NODE) ||
             (object->getObjectClass() == OBJECT_CLUSTER) ||
             (object->getObjectClass() == OBJECT_TEMPLATE))
            ((Template *)object)->unlockDCIList(m_id);
   }

   // Waiting while reference count becomes 0
   if (m_dwRefCount > 0)
   {
      debugPrintf(3, _T("Waiting for pending requests..."));
      do
      {
         ThreadSleep(1);
      } while(m_dwRefCount > 0);
   }

   if (m_dwFlags & CSF_AUTHENTICATED)
   {
      CALL_ALL_MODULES(pfClientSessionClose, (this));
	   WriteAuditLog(AUDIT_SECURITY, TRUE, m_dwUserId, m_workstation, m_id, 0, _T("User logged out (client: %s)"), m_clientInfo);
   }
   debugPrintf(3, _T("Session closed"));
}

/**
 * Network write thread
 */
void ClientSession::writeThread()
{
   while(true)
   {
      NXCP_MESSAGE *rawMsg = (NXCP_MESSAGE *)m_pSendQueue->getOrBlock();
      if (rawMsg == INVALID_POINTER_VALUE)    // Session termination indicator
         break;

      sendRawMessage(rawMsg);
      free(rawMsg);
   }
}

/**
 * Update processing thread
 */
void ClientSession::updateThread()
{
   UPDATE_INFO *pUpdate;
   NXCPMessage msg;

   while(1)
   {
      pUpdate = (UPDATE_INFO *)m_pUpdateQueue->getOrBlock();
      if (pUpdate == INVALID_POINTER_VALUE)    // Session termination indicator
         break;

      switch(pUpdate->dwCategory)
      {
         case INFO_CAT_EVENT:
            MutexLock(m_mutexSendEvents);
            sendMessage((NXCPMessage *)pUpdate->pData);
            MutexUnlock(m_mutexSendEvents);
            delete (NXCPMessage *)pUpdate->pData;
            break;
         case INFO_CAT_SYSLOG_MSG:
            MutexLock(m_mutexSendSyslog);
            msg.setCode(CMD_SYSLOG_RECORDS);
            CreateMessageFromSyslogMsg(&msg, (NX_SYSLOG_RECORD *)pUpdate->pData);
            sendMessage(&msg);
            MutexUnlock(m_mutexSendSyslog);
            free(pUpdate->pData);
            break;
         case INFO_CAT_SNMP_TRAP:
            MutexLock(m_mutexSendTrapLog);
            sendMessage((NXCPMessage *)pUpdate->pData);
            MutexUnlock(m_mutexSendTrapLog);
            delete (NXCPMessage *)pUpdate->pData;
            break;
         case INFO_CAT_AUDIT_RECORD:
            MutexLock(m_mutexSendAuditLog);
            sendMessage((NXCPMessage *)pUpdate->pData);
            MutexUnlock(m_mutexSendAuditLog);
            delete (NXCPMessage *)pUpdate->pData;
            break;
         case INFO_CAT_OBJECT_CHANGE:
            MutexLock(m_mutexSendObjects);
            msg.setCode(CMD_OBJECT_UPDATE);
            if (!((NetObj *)pUpdate->pData)->isDeleted())
            {
               ((NetObj *)pUpdate->pData)->fillMessage(&msg);
               if (m_dwFlags & CSF_SYNC_OBJECT_COMMENTS)
                  ((NetObj *)pUpdate->pData)->commentsToMessage(&msg);
            }
            else
            {
               msg.setField(VID_OBJECT_ID, ((NetObj *)pUpdate->pData)->getId());
               msg.setField(VID_IS_DELETED, (WORD)1);
            }
            sendMessage(&msg);
            MutexUnlock(m_mutexSendObjects);
            msg.deleteAllFields();
            ((NetObj *)pUpdate->pData)->decRefCount();
            break;
         case INFO_CAT_ALARM:
            MutexLock(m_mutexSendAlarms);
            msg.setCode(CMD_ALARM_UPDATE);
            msg.setField(VID_NOTIFICATION_CODE, pUpdate->dwCode);
            FillAlarmInfoMessage(&msg, (NXC_ALARM *)pUpdate->pData);
            sendMessage(&msg);
            MutexUnlock(m_mutexSendAlarms);
            msg.deleteAllFields();
            free(pUpdate->pData);
            break;
         case INFO_CAT_ACTION:
            MutexLock(m_mutexSendActions);
            msg.setCode(CMD_ACTION_DB_UPDATE);
            msg.setField(VID_NOTIFICATION_CODE, pUpdate->dwCode);
            msg.setField(VID_ACTION_ID, ((NXC_ACTION *)pUpdate->pData)->dwId);
            if (pUpdate->dwCode != NX_NOTIFY_ACTION_DELETED)
               FillActionInfoMessage(&msg, (NXC_ACTION *)pUpdate->pData);
            sendMessage(&msg);
            MutexUnlock(m_mutexSendActions);
            msg.deleteAllFields();
            free(pUpdate->pData);
            break;
         case INFO_CAT_SITUATION:
            MutexLock(m_mutexSendSituations);
            sendMessage((NXCPMessage *)pUpdate->pData);
            MutexUnlock(m_mutexSendSituations);
            delete (NXCPMessage *)pUpdate->pData;
            break;
         case INFO_CAT_LIBRARY_IMAGE:
            {
              LIBRARY_IMAGE_UPDATE_INFO *info = (LIBRARY_IMAGE_UPDATE_INFO *)pUpdate->pData;
              msg.setCode(CMD_IMAGE_LIBRARY_UPDATE);
              msg.setField(VID_GUID, (BYTE *)info->guid, UUID_LENGTH);
              if (info->removed)
              {
                msg.setField(VID_FLAGS, (UINT32)1);
              }
              else
              {
                msg.setField(VID_FLAGS, (UINT32)0);
              }
              sendMessage(&msg);
              msg.deleteAllFields();
              free(info->guid);
              free(info);
            }
            break;
         default:
            break;
      }

      free(pUpdate);
   }
}

/**
 * Message processing thread
 */
void ClientSession::processingThread()
{
   NXCPMessage *pMsg;
   TCHAR szBuffer[128];
   UINT32 i;
	int status;

   while(1)
   {
      pMsg = (NXCPMessage *)m_pMessageQueue->getOrBlock();
      if (pMsg == INVALID_POINTER_VALUE)    // Session termination indicator
         break;

      m_wCurrentCmd = pMsg->getCode();
      debugPrintf(6, _T("Received message %s"), NXCPMessageCodeName(m_wCurrentCmd, szBuffer));
      if (!(m_dwFlags & CSF_AUTHENTICATED) &&
          (m_wCurrentCmd != CMD_LOGIN) &&
          (m_wCurrentCmd != CMD_GET_SERVER_INFO) &&
          (m_wCurrentCmd != CMD_REQUEST_ENCRYPTION) &&
          (m_wCurrentCmd != CMD_GET_MY_CONFIG) &&
          (m_wCurrentCmd != CMD_REGISTER_AGENT))
      {
         delete pMsg;
         continue;
      }

      m_state = SESSION_STATE_PROCESSING;
      switch(m_wCurrentCmd)
      {
         case CMD_LOGIN:
            login(pMsg);
            break;
         case CMD_GET_SERVER_INFO:
            sendServerInfo(pMsg->getId());
            break;
         case CMD_GET_MY_CONFIG:
            sendConfigForAgent(pMsg);
            break;
         case CMD_GET_OBJECTS:
            sendAllObjects(pMsg);
            break;
         case CMD_GET_SELECTED_OBJECTS:
            sendSelectedObjects(pMsg);
            break;
         case CMD_GET_EVENTS:
            CALL_IN_NEW_THREAD(sendEventLog, pMsg);
            break;
         case CMD_GET_CONFIG_VARLIST:
            getConfigurationVariables(pMsg->getId());
            break;
         case CMD_GET_PUBLIC_CONFIG_VAR:
            getPublicConfigurationVariable(pMsg);
            break;
         case CMD_SET_CONFIG_VARIABLE:
            setConfigurationVariable(pMsg);
            break;
         case CMD_DELETE_CONFIG_VARIABLE:
            deleteConfigurationVariable(pMsg);
            break;
			case CMD_CONFIG_GET_CLOB:
				getConfigCLOB(pMsg);
				break;
			case CMD_CONFIG_SET_CLOB:
				setConfigCLOB(pMsg);
				break;
         case CMD_LOAD_EVENT_DB:
            sendEventDB(pMsg->getId());
            break;
         case CMD_SET_EVENT_INFO:
            modifyEventTemplate(pMsg);
            break;
         case CMD_DELETE_EVENT_TEMPLATE:
            deleteEventTemplate(pMsg);
            break;
         case CMD_GENERATE_EVENT_CODE:
            generateEventCode(pMsg->getId());
            break;
         case CMD_MODIFY_OBJECT:
            modifyObject(pMsg);
            break;
         case CMD_SET_OBJECT_MGMT_STATUS:
            changeObjectMgmtStatus(pMsg);
            break;
         case CMD_LOAD_USER_DB:
            sendUserDB(pMsg->getId());
            break;
         case CMD_CREATE_USER:
            createUser(pMsg);
            break;
         case CMD_UPDATE_USER:
            updateUser(pMsg);
            break;
         case CMD_DETACH_LDAP_USER:
            detachLdapUser(pMsg);
            break;
         case CMD_DELETE_USER:
            deleteUser(pMsg);
            break;
         case CMD_LOCK_USER_DB:
            lockUserDB(pMsg->getId(), TRUE);
            break;
         case CMD_UNLOCK_USER_DB:
            lockUserDB(pMsg->getId(), FALSE);
            break;
         case CMD_SET_PASSWORD:
            setPassword(pMsg);
            break;
         case CMD_VALIDATE_PASSWORD:
            validatePassword(pMsg);
            break;
         case CMD_GET_NODE_DCI_LIST:
            openNodeDCIList(pMsg);
            break;
         case CMD_UNLOCK_NODE_DCI_LIST:
            closeNodeDCIList(pMsg);
            break;
         case CMD_CREATE_NEW_DCI:
         case CMD_MODIFY_NODE_DCI:
         case CMD_DELETE_NODE_DCI:
            modifyNodeDCI(pMsg);
            break;
         case CMD_SET_DCI_STATUS:
            changeDCIStatus(pMsg);
            break;
         case CMD_COPY_DCI:
            copyDCI(pMsg);
            break;
         case CMD_APPLY_TEMPLATE:
            applyTemplate(pMsg);
            break;
         case CMD_GET_DCI_DATA:
            CALL_IN_NEW_THREAD(getCollectedData, pMsg);
            break;
         case CMD_GET_TABLE_DCI_DATA:
            CALL_IN_NEW_THREAD(getTableCollectedData, pMsg);
            break;
			case CMD_CLEAR_DCI_DATA:
				CALL_IN_NEW_THREAD(clearDCIData, pMsg);
				break;
			case CMD_FORCE_DCI_POLL:
				CALL_IN_NEW_THREAD(forceDCIPoll, pMsg);
				break;
         case CMD_OPEN_EPP:
            openEPP(pMsg);
            break;
         case CMD_CLOSE_EPP:
            closeEPP(pMsg->getId());
            break;
         case CMD_SAVE_EPP:
            saveEPP(pMsg);
            break;
         case CMD_EPP_RECORD:
            processEPPRecord(pMsg);
            break;
         case CMD_GET_MIB_TIMESTAMP:
            sendMIBTimestamp(pMsg->getId());
            break;
         case CMD_GET_MIB:
            CALL_IN_NEW_THREAD(sendMib, pMsg);
            break;
         case CMD_CREATE_OBJECT:
            CALL_IN_NEW_THREAD(createObject, pMsg);
            break;
         case CMD_BIND_OBJECT:
            changeObjectBinding(pMsg, TRUE);
            break;
         case CMD_UNBIND_OBJECT:
            changeObjectBinding(pMsg, FALSE);
            break;
         case CMD_ADD_CLUSTER_NODE:
            addClusterNode(pMsg);
            break;
         case CMD_GET_ALL_ALARMS:
            sendAllAlarms(pMsg->getId());
            break;
         case CMD_GET_ALARM_COMMENTS:
				getAlarmComments(pMsg);
            break;
         case CMD_SET_ALARM_STATUS_FLOW:
            updateAlarmStatusFlow(pMsg);
            break;
         case CMD_UPDATE_ALARM_COMMENT:
				updateAlarmComment(pMsg);
            break;
         case CMD_DELETE_ALARM_COMMENT:
            deleteAlarmComment(pMsg);
            break;
         case CMD_GET_ALARM:
            getAlarm(pMsg);
            break;
         case CMD_GET_ALARM_EVENTS:
            CALL_IN_NEW_THREAD(getAlarmEvents, pMsg);
            break;
         case CMD_ACK_ALARM:
            acknowledgeAlarm(pMsg);
            break;
         case CMD_RESOLVE_ALARM:
            resolveAlarm(pMsg, false);
            break;
         case CMD_TERMINATE_ALARM:
            resolveAlarm(pMsg, true);
            break;
         case CMD_DELETE_ALARM:
            deleteAlarm(pMsg);
            break;
         case CMD_OPEN_HELPDESK_ISSUE:
            CALL_IN_NEW_THREAD(openHelpdeskIssue, pMsg);
            break;
         case CMD_GET_HELPDESK_URL:
            getHelpdeskUrl(pMsg);
            break;
         case CMD_UNLINK_HELPDESK_ISSUE:
            unlinkHelpdeskIssue(pMsg);
            break;
         case CMD_CREATE_ACTION:
            createAction(pMsg);
            break;
         case CMD_MODIFY_ACTION:
            updateAction(pMsg);
            break;
         case CMD_DELETE_ACTION:
            deleteAction(pMsg);
            break;
         case CMD_LOAD_ACTIONS:
            sendAllActions(pMsg->getId());
            break;
         case CMD_GET_CONTAINER_CAT_LIST:
            SendContainerCategories(pMsg->getId());
            break;
         case CMD_DELETE_OBJECT:
            deleteObject(pMsg);
            break;
         case CMD_POLL_NODE:
            forcedNodePoll(pMsg);
            break;
         case CMD_TRAP:
            onTrap(pMsg);
            break;
         case CMD_WAKEUP_NODE:
            onWakeUpNode(pMsg);
            break;
         case CMD_CREATE_TRAP:
            editTrap(TRAP_CREATE, pMsg);
            break;
         case CMD_MODIFY_TRAP:
            editTrap(TRAP_UPDATE, pMsg);
            break;
         case CMD_DELETE_TRAP:
            editTrap(TRAP_DELETE, pMsg);
            break;
         case CMD_LOAD_TRAP_CFG:
            sendAllTraps(pMsg->getId());
            break;
			case CMD_GET_TRAP_CFG_RO:
				sendAllTraps2(pMsg->getId());
				break;
         case CMD_QUERY_PARAMETER:
            CALL_IN_NEW_THREAD(queryParameter, pMsg);
            break;
         case CMD_QUERY_TABLE:
            CALL_IN_NEW_THREAD(queryAgentTable, pMsg);
            break;
         case CMD_LOCK_PACKAGE_DB:
            LockPackageDB(pMsg->getId(), TRUE);
            break;
         case CMD_UNLOCK_PACKAGE_DB:
            LockPackageDB(pMsg->getId(), FALSE);
            break;
         case CMD_GET_PACKAGE_LIST:
            SendAllPackages(pMsg->getId());
            break;
         case CMD_INSTALL_PACKAGE:
            InstallPackage(pMsg);
            break;
         case CMD_REMOVE_PACKAGE:
            RemovePackage(pMsg);
            break;
         case CMD_GET_PARAMETER_LIST:
            getParametersList(pMsg);
            break;
         case CMD_DEPLOY_PACKAGE:
            DeployPackage(pMsg);
            break;
         case CMD_GET_LAST_VALUES:
            getLastValues(pMsg);
            break;
         case CMD_GET_DCI_VALUES:
            getLastValuesByDciId(pMsg);
            break;
         case CMD_GET_TABLE_LAST_VALUES:
            getTableLastValues(pMsg);
            break;
			case CMD_GET_THRESHOLD_SUMMARY:
				getThresholdSummary(pMsg);
				break;
         case CMD_GET_USER_VARIABLE:
            getUserVariable(pMsg);
            break;
         case CMD_SET_USER_VARIABLE:
            setUserVariable(pMsg);
            break;
         case CMD_DELETE_USER_VARIABLE:
            deleteUserVariable(pMsg);
            break;
         case CMD_ENUM_USER_VARIABLES:
            enumUserVariables(pMsg);
            break;
         case CMD_COPY_USER_VARIABLE:
            copyUserVariable(pMsg);
            break;
         case CMD_CHANGE_ZONE:
            changeObjectZone(pMsg);
            break;
         case CMD_REQUEST_ENCRYPTION:
            setupEncryption(pMsg);
            break;
         case CMD_GET_AGENT_CONFIG:
            getAgentConfig(pMsg);
            break;
         case CMD_UPDATE_AGENT_CONFIG:
            updateAgentConfig(pMsg);
            break;
         case CMD_EXECUTE_ACTION:
            CALL_IN_NEW_THREAD(executeAction, pMsg);
            break;
         case CMD_GET_OBJECT_TOOLS:
            getObjectTools(pMsg->getId());
            break;
         case CMD_EXEC_TABLE_TOOL:
            execTableTool(pMsg);
            break;
         case CMD_GET_OBJECT_TOOL_DETAILS:
            getObjectToolDetails(pMsg);
            break;
         case CMD_UPDATE_OBJECT_TOOL:
            updateObjectTool(pMsg);
            break;
         case CMD_DELETE_OBJECT_TOOL:
            deleteObjectTool(pMsg);
            break;
         case CMD_CHANGE_OBJECT_TOOL_STATUS:
            changeObjectToolStatus(pMsg);
            break;
         case CMD_GENERATE_OBJECT_TOOL_ID:
            generateObjectToolId(pMsg->getId());
            break;
         case CMD_CHANGE_SUBSCRIPTION:
            changeSubscription(pMsg);
            break;
         case CMD_GET_SYSLOG:
            CALL_IN_NEW_THREAD(sendSyslog, pMsg);
            break;
         case CMD_GET_SERVER_STATS:
            sendServerStats(pMsg->getId());
            break;
         case CMD_GET_SCRIPT_LIST:
            sendScriptList(pMsg->getId());
            break;
         case CMD_GET_SCRIPT:
            sendScript(pMsg);
            break;
         case CMD_UPDATE_SCRIPT:
            updateScript(pMsg);
            break;
         case CMD_RENAME_SCRIPT:
            renameScript(pMsg);
            break;
         case CMD_DELETE_SCRIPT:
            deleteScript(pMsg);
            break;
         case CMD_GET_SESSION_LIST:
            SendSessionList(pMsg->getId());
            break;
         case CMD_KILL_SESSION:
            KillSession(pMsg);
            break;
         case CMD_GET_TRAP_LOG:
            SendTrapLog(pMsg);
            break;
         case CMD_START_SNMP_WALK:
            StartSnmpWalk(pMsg);
            break;
         case CMD_RESOLVE_DCI_NAMES:
            resolveDCINames(pMsg);
            break;
			case CMD_GET_DCI_INFO:
				SendDCIInfo(pMsg);
				break;
			case CMD_GET_DCI_THRESHOLDS:
				sendDCIThresholds(pMsg);
				break;
         case CMD_GET_DCI_EVENTS_LIST:
            getDCIEventList(pMsg);
            break;
         case CMD_GET_DCI_SCRIPT_LIST:
            getDCIScriptList(pMsg);
            break;
			case CMD_GET_PERFTAB_DCI_LIST:
				sendPerfTabDCIList(pMsg);
				break;
         case CMD_PUSH_DCI_DATA:
            pushDCIData(pMsg);
            break;
         case CMD_GET_AGENT_CFG_LIST:
            sendAgentCfgList(pMsg->getId());
            break;
         case CMD_OPEN_AGENT_CONFIG:
            OpenAgentConfig(pMsg);
            break;
         case CMD_SAVE_AGENT_CONFIG:
            SaveAgentConfig(pMsg);
            break;
         case CMD_DELETE_AGENT_CONFIG:
            DeleteAgentConfig(pMsg);
            break;
         case CMD_SWAP_AGENT_CONFIGS:
            SwapAgentConfigs(pMsg);
            break;
         case CMD_GET_OBJECT_COMMENTS:
            SendObjectComments(pMsg);
            break;
         case CMD_UPDATE_OBJECT_COMMENTS:
            updateObjectComments(pMsg);
            break;
         case CMD_GET_ADDR_LIST:
            getAddrList(pMsg);
            break;
         case CMD_SET_ADDR_LIST:
            setAddrList(pMsg);
            break;
         case CMD_RESET_COMPONENT:
            resetComponent(pMsg);
            break;
         case CMD_EXPORT_CONFIGURATION:
            exportConfiguration(pMsg);
            break;
         case CMD_IMPORT_CONFIGURATION:
            importConfiguration(pMsg);
            break;
			case CMD_GET_GRAPH_LIST:
				sendGraphList(pMsg->getId());
				break;
			case CMD_SAVE_GRAPH:
			   saveGraph(pMsg);
            break;
			case CMD_DELETE_GRAPH:
				deleteGraph(pMsg);
				break;
			case CMD_ADD_CA_CERTIFICATE:
				AddCACertificate(pMsg);
				break;
			case CMD_DELETE_CERTIFICATE:
				DeleteCertificate(pMsg);
				break;
			case CMD_UPDATE_CERT_COMMENTS:
				UpdateCertificateComments(pMsg);
				break;
			case CMD_GET_CERT_LIST:
				getCertificateList(pMsg->getId());
				break;
			case CMD_QUERY_L2_TOPOLOGY:
				CALL_IN_NEW_THREAD(queryL2Topology, pMsg);
				break;
			case CMD_SEND_SMS:
				sendSMS(pMsg);
				break;
			case CMD_GET_COMMUNITY_LIST:
				SendCommunityList(pMsg->getId());
				break;
			case CMD_UPDATE_COMMUNITY_LIST:
				UpdateCommunityList(pMsg);
				break;
			case CMD_GET_USM_CREDENTIALS:
				sendUsmCredentials(pMsg->getId());
				break;
			case CMD_UPDATE_USM_CREDENTIALS:
				updateUsmCredentials(pMsg);
				break;
			case CMD_GET_SITUATION_LIST:
				getSituationList(pMsg->getId());
				break;
			case CMD_CREATE_SITUATION:
				createSituation(pMsg);
				break;
			case CMD_UPDATE_SITUATION:
				updateSituation(pMsg);
				break;
			case CMD_DELETE_SITUATION:
				deleteSituation(pMsg);
				break;
			case CMD_DEL_SITUATION_INSTANCE:
				deleteSituationInstance(pMsg);
				break;
			case CMD_REGISTER_AGENT:
				registerAgent(pMsg);
				break;
			case CMD_GET_SERVER_FILE:
				CALL_IN_NEW_THREAD(getServerFile, pMsg);
				break;
			case CMD_GET_AGENT_FILE:
				CALL_IN_NEW_THREAD(getAgentFile, pMsg);
				break;
         case CMD_CANCEL_FILE_MONITORING:
				CALL_IN_NEW_THREAD(cancelFileMonitoring, pMsg);
				break;
			case CMD_TEST_DCI_TRANSFORMATION:
				testDCITransformation(pMsg);
				break;
			case CMD_EXECUTE_SCRIPT:
            CALL_IN_NEW_THREAD(executeScript, pMsg);
				break;
			case CMD_GET_JOB_LIST:
				sendJobList(pMsg->getId());
				break;
			case CMD_CANCEL_JOB:
				cancelJob(pMsg);
				break;
			case CMD_HOLD_JOB:
				holdJob(pMsg);
				break;
			case CMD_UNHOLD_JOB:
				unholdJob(pMsg);
				break;
			case CMD_DEPLOY_AGENT_POLICY:
				deployAgentPolicy(pMsg, false);
				break;
			case CMD_UNINSTALL_AGENT_POLICY:
				deployAgentPolicy(pMsg, true);
				break;
			case CMD_GET_CURRENT_USER_ATTR:
				getUserCustomAttribute(pMsg);
				break;
			case CMD_SET_CURRENT_USER_ATTR:
				setUserCustomAttribute(pMsg);
				break;
			case CMD_OPEN_SERVER_LOG:
				openServerLog(pMsg);
				break;
			case CMD_CLOSE_SERVER_LOG:
				closeServerLog(pMsg);
				break;
			case CMD_QUERY_LOG:
				CALL_IN_NEW_THREAD(queryServerLog, pMsg);
				break;
			case CMD_GET_LOG_DATA:
				CALL_IN_NEW_THREAD(getServerLogQueryData, pMsg);
				break;
			case CMD_FIND_NODE_CONNECTION:
				CALL_IN_NEW_THREAD(findNodeConnection, pMsg);
				break;
			case CMD_FIND_MAC_LOCATION:
				CALL_IN_NEW_THREAD(findMacAddress, pMsg);
				break;
			case CMD_FIND_IP_LOCATION:
				CALL_IN_NEW_THREAD(findIpAddress, pMsg);
				break;
			case CMD_GET_IMAGE:
				sendLibraryImage(pMsg);
				break;
			case CMD_CREATE_IMAGE:
				updateLibraryImage(pMsg);
				break;
			case CMD_DELETE_IMAGE:
				deleteLibraryImage(pMsg);
				break;
			case CMD_MODIFY_IMAGE:
				updateLibraryImage(pMsg);
				break;
			case CMD_LIST_IMAGES:
				listLibraryImages(pMsg);
				break;
			case CMD_EXECUTE_SERVER_COMMAND:
				executeServerCommand(pMsg);
				break;
			case CMD_LIST_SERVER_FILES:
				listServerFileStore(pMsg);
				break;
			case CMD_UPLOAD_FILE_TO_AGENT:
				uploadFileToAgent(pMsg);
				break;
			case CMD_UPLOAD_FILE:
				receiveFile(pMsg);
				break;
			case CMD_DELETE_FILE:
				deleteFile(pMsg);
				break;
			case CMD_OPEN_CONSOLE:
				openConsole(pMsg->getId());
				break;
			case CMD_CLOSE_CONSOLE:
				closeConsole(pMsg->getId());
				break;
			case CMD_ADM_REQUEST:
				CALL_IN_NEW_THREAD(processConsoleCommand, pMsg);
				break;
			case CMD_GET_VLANS:
				getVlans(pMsg);
				break;
			case CMD_GET_NETWORK_PATH:
				CALL_IN_NEW_THREAD(getNetworkPath, pMsg);
				break;
			case CMD_GET_NODE_COMPONENTS:
				getNodeComponents(pMsg);
				break;
			case CMD_GET_NODE_SOFTWARE:
				getNodeSoftware(pMsg);
				break;
			case CMD_GET_WINPERF_OBJECTS:
				getWinPerfObjects(pMsg);
				break;
			case CMD_LIST_MAPPING_TABLES:
				listMappingTables(pMsg);
				break;
			case CMD_GET_MAPPING_TABLE:
				getMappingTable(pMsg);
				break;
			case CMD_UPDATE_MAPPING_TABLE:
				updateMappingTable(pMsg);
				break;
			case CMD_DELETE_MAPPING_TABLE:
				deleteMappingTable(pMsg);
				break;
			case CMD_GET_WIRELESS_STATIONS:
				getWirelessStations(pMsg);
				break;
         case CMD_GET_SUMMARY_TABLES:
            getSummaryTables(pMsg->getId());
            break;
         case CMD_GET_SUMMARY_TABLE_DETAILS:
            getSummaryTableDetails(pMsg);
            break;
         case CMD_MODIFY_SUMMARY_TABLE:
            modifySummaryTable(pMsg);
            break;
         case CMD_DELETE_SUMMARY_TABLE:
            deleteSummaryTable(pMsg);
            break;
         case CMD_QUERY_SUMMARY_TABLE:
            querySummaryTable(pMsg);
            break;
         case CMD_QUERY_ADHOC_SUMMARY_TABLE:
            queryAdHocSummaryTable(pMsg);
            break;
         case CMD_GET_SUBNET_ADDRESS_MAP:
            getSubnetAddressMap(pMsg);
            break;
         case CMD_GET_EFFECTIVE_RIGHTS:
            getEffectiveRights(pMsg);
            break;
         case CMD_GET_FOLDER_CONTENT:
         case CMD_FILEMGR_DELETE_FILE:
         case CMD_FILEMGR_RENAME_FILE:
         case CMD_FILEMGR_MOVE_FILE:
         case CMD_FILEMGR_CREATE_FOLDER:
            CALL_IN_NEW_THREAD(fileManagerControl, pMsg);
            break;
         case CMD_FILEMGR_UPLOAD:
            CALL_IN_NEW_THREAD(uploadUserFileToAgent, pMsg);
            break;
         case CMD_GET_SWITCH_FDB:
            CALL_IN_NEW_THREAD(getSwitchForwardingDatabase, pMsg);
            break;
         case CMD_GET_ROUTING_TABLE:
            CALL_IN_NEW_THREAD(getRoutingTable, pMsg);
            break;
         case CMD_GET_LOC_HISTORY:
            CALL_IN_NEW_THREAD(getLocationHistory, pMsg);
            break;
         case CMD_TAKE_SCREENSHOT:
            getScreenshot(pMsg);
            break;
         default:
            if ((m_wCurrentCmd >> 8) == 0x11)
            {
               // Reporting Server range (0x1100 - 0x11FF)
               CALL_IN_NEW_THREAD(forwardToReportingServer, pMsg);
               break;
            }

            // Pass message to loaded modules
            for(i = 0; i < g_dwNumModules; i++)
				{
					if (g_pModuleList[i].pfClientCommandHandler != NULL)
					{
						status = g_pModuleList[i].pfClientCommandHandler(m_wCurrentCmd, pMsg, this);
						if (status != NXMOD_COMMAND_IGNORED)
						{
							if (status == NXMOD_COMMAND_ACCEPTED_ASYNC)
							{
								pMsg = NULL;	// Prevent deletion
								m_dwRefCount++;
							}
							break;   // Message was processed by the module
						}
					}
				}
            if (i == g_dwNumModules)
            {
               NXCPMessage response;

               response.setId(pMsg->getId());
               response.setCode(CMD_REQUEST_COMPLETED);
               response.setField(VID_RCC, RCC_NOT_IMPLEMENTED);
               sendMessage(&response);
            }
            break;
      }
      delete pMsg;
      m_state = (m_dwFlags & CSF_AUTHENTICATED) ? SESSION_STATE_IDLE : SESSION_STATE_INIT;
   }
}

/**
 * Respond to client's keepalive message
 */
void ClientSession::respondToKeepalive(UINT32 dwRqId)
{
   NXCPMessage msg;

   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(dwRqId);
   msg.setField(VID_RCC, RCC_SUCCESS);
   sendMessage(&msg);
}

/**
 * Process received file
 */
void ClientSession::onFileUpload(BOOL bSuccess)
{
  // Do processing specific to command initiated file upload
  switch(m_dwUploadCommand)
  {
    case CMD_INSTALL_PACKAGE:
      if (!bSuccess)
      {
        TCHAR szQuery[256];

        _sntprintf(szQuery, 256, _T("DELETE FROM agent_pkg WHERE pkg_id=%d"), m_dwUploadData);
        DBQuery(g_hCoreDB, szQuery);
      }
      break;
    case CMD_MODIFY_IMAGE:
      EnumerateClientSessions(ImageLibraryUpdateCallback, (void *)&m_uploadImageGuid);
      break;
    default:
      break;
  }

  // Remove received file in case of failure
  if (!bSuccess)
    _tunlink(m_szCurrFileName);
}

/**
 * Send message to client
 */
bool ClientSession::sendMessage(NXCPMessage *msg)
{
   if (isTerminated())
      return false;

	if (msg->getCode() != CMD_ADM_MESSAGE)
   {
      TCHAR buffer[128];
		debugPrintf(6, _T("Sending message %s"), NXCPMessageCodeName(msg->getCode(), buffer));
   }

	NXCP_MESSAGE *rawMsg = msg->createMessage();
   if ((g_debugLevel >= 8) && (msg->getCode() != CMD_ADM_MESSAGE))
   {
      String msgDump = NXCPMessage::dump(rawMsg, NXCP_VERSION);
      debugPrintf(8, _T("Message dump:\n%s"), (const TCHAR *)msgDump);
   }

   bool result;
   if (m_pCtx != NULL)
   {
      NXCP_ENCRYPTED_MESSAGE *enMsg = m_pCtx->encryptMessage(rawMsg);
      if (enMsg != NULL)
      {
         result = (SendEx(m_hSocket, (char *)enMsg, ntohl(enMsg->size), 0, m_mutexSocketWrite) == (int)ntohl(enMsg->size));
         free(enMsg);
      }
      else
      {
         result = false;
      }
   }
   else
   {
      result = (SendEx(m_hSocket, (const char *)rawMsg, ntohl(rawMsg->size), 0, m_mutexSocketWrite) == (int)ntohl(rawMsg->size));
   }
   free(rawMsg);

   if (!result)
   {
      closesocket(m_hSocket);
      m_hSocket = -1;
   }
   return result;
}

/**
 * Send raw message to client
 */
void ClientSession::sendRawMessage(NXCP_MESSAGE *msg)
{
   if (isTerminated())
      return;

   UINT16 code = htons(msg->code);
   if (code != CMD_ADM_MESSAGE)
   {
      TCHAR buffer[128];
	   debugPrintf(6, _T("Sending message %s"), NXCPMessageCodeName(ntohs(msg->code), buffer));
   }

   if ((g_debugLevel >= 8) && (code != CMD_ADM_MESSAGE))
   {
      String msgDump = NXCPMessage::dump(msg, NXCP_VERSION);
      debugPrintf(8, _T("Message dump:\n%s"), (const TCHAR *)msgDump);
   }

   bool result;
   if (m_pCtx != NULL)
   {
      NXCP_ENCRYPTED_MESSAGE *enMsg = m_pCtx->encryptMessage(msg);
      if (enMsg != NULL)
      {
         result = (SendEx(m_hSocket, (char *)enMsg, ntohl(enMsg->size), 0, m_mutexSocketWrite) == (int)ntohl(enMsg->size));
         free(enMsg);
      }
      else
      {
         result = false;
      }
   }
   else
   {
      result = (SendEx(m_hSocket, (const char *)msg, ntohl(msg->size), 0, m_mutexSocketWrite) == (int)ntohl(msg->size));
   }

   if (!result)
   {
      closesocket(m_hSocket);
      m_hSocket = -1;
   }
}

/**
 * Send file to client
 */
BOOL ClientSession::sendFile(const TCHAR *file, UINT32 dwRqId, long ofset)
{
   return !isTerminated() ? SendFileOverNXCP(m_hSocket, dwRqId, file, m_pCtx, ofset, NULL, NULL, m_mutexSocketWrite) : FALSE;
}

/**
 * Send server information to client
 */
void ClientSession::sendServerInfo(UINT32 dwRqId)
{
   static UINT32 protocolVersions[] = {
      CLIENT_PROTOCOL_VERSION_BASE,
      CLIENT_PROTOCOL_VERSION_ALARMS,
      CLIENT_PROTOCOL_VERSION_PUSH,
      CLIENT_PROTOCOL_VERSION_TRAP,
      CLIENT_PROTOCOL_VERSION_MOBILE,
      CLIENT_PROTOCOL_VERSION_FULL
   };
   NXCPMessage msg;
	TCHAR szBuffer[1024];
	String strURL;

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(dwRqId);

	// Generate challenge for certificate authentication
#ifdef _WITH_ENCRYPTION
	RAND_bytes(m_challenge, CLIENT_CHALLENGE_SIZE);
#else
	memset(m_challenge, 0, CLIENT_CHALLENGE_SIZE);
#endif

   // Fill message with server info
   msg.setField(VID_RCC, RCC_SUCCESS);
   msg.setField(VID_SERVER_VERSION, NETXMS_VERSION_STRING);
   msg.setField(VID_SERVER_ID, g_serverId);
   msg.setField(VID_SUPPORTED_ENCRYPTION, (UINT32)0);
   msg.setField(VID_PROTOCOL_VERSION, (UINT32)CLIENT_PROTOCOL_VERSION_BASE);
   msg.setFieldFromInt32Array(VID_PROTOCOL_VERSION_EX, sizeof(protocolVersions) / sizeof(UINT32), protocolVersions);
	msg.setField(VID_CHALLENGE, m_challenge, CLIENT_CHALLENGE_SIZE);
   msg.setField(VID_TIMESTAMP, (UINT32)time(NULL));

#if defined(_WIN32)
	TIME_ZONE_INFORMATION tz;
	WCHAR wst[4], wdt[8], *curr;
	int i;

	DWORD tzType = GetTimeZoneInformation(&tz);

	// Create 3 letter abbreviation for standard name
	for(i = 0, curr = tz.StandardName; (*curr != 0) && (i < 3); curr++)
		if (iswupper(*curr))
			wst[i++] = *curr;
	while(i < 3)
		wst[i++] = L'X';
	wst[i] = 0;

	// Create abbreviation for DST name
	for(i = 0, curr = tz.DaylightName; (*curr != 0) && (i < 7); curr++)
		if (iswupper(*curr))
			wdt[i++] = *curr;
	while(i < 3)
		wdt[i++] = L'X';
	wdt[i] = 0;

	LONG effectiveBias;
	switch(tzType)
	{
		case TIME_ZONE_ID_STANDARD:
			effectiveBias = tz.Bias + tz.StandardBias;
			break;
		case TIME_ZONE_ID_DAYLIGHT:
			effectiveBias = tz.Bias + tz.DaylightBias;
			break;
		case TIME_ZONE_ID_UNKNOWN:
			effectiveBias = tz.Bias;
			break;
		default:		// error
			effectiveBias = 0;
			debugPrintf(4, _T("GetTimeZoneInformation() call failed"));
			break;
	}

#ifdef UNICODE
	swprintf(szBuffer, 1024, L"%s%c%02d%s", wst, (effectiveBias > 0) ? '-' : '+',
	         abs(effectiveBias) / 60, (tz.DaylightBias != 0) ? wdt : L"");
#else
	sprintf(szBuffer, "%S%c%02d%S", wst, (effectiveBias > 0) ? '-' : '+',
	        abs(effectiveBias) / 60, (tz.DaylightBias != 0) ? wdt : L"");
#endif

#elif HAVE_TM_GMTOFF  /* not Windows but have tm_gmtoff */

	time_t t = time(NULL);
	int gmtOffset;
#if HAVE_LOCALTIME_R
	struct tm tmbuff;
	struct tm *loc = localtime_r(&t, &tmbuff);
#else
	struct tm *loc = localtime(&t);
#endif
	gmtOffset = loc->tm_gmtoff / 3600;
#ifdef UNICODE
	swprintf(szBuffer, 1024, L"%hs%hc%02d%hs", tzname[0], (gmtOffset >= 0) ? '+' : '-',
	         abs(gmtOffset), (tzname[1] != NULL) ? tzname[1] : "");
#else
	sprintf(szBuffer, "%s%c%02d%s", tzname[0], (gmtOffset >= 0) ? '+' : '-',
	        abs(gmtOffset), (tzname[1] != NULL) ? tzname[1] : "");
#endif

#else /* not Windows and no tm_gmtoff */

   time_t t = time(NULL);
#if HAVE_GMTIME_R
   struct tm tmbuff;
   struct tm *gmt = gmtime_r(&t, &tmbuff);
#else
   struct tm *gmt = gmtime(&t);
#endif
   gmt->tm_isdst = -1;
   int gmtOffset = (int)((t - mktime(gmt)) / 3600);
#ifdef UNICODE
	swprintf(szBuffer, 1024, L"%hs%hc%02d%hs", tzname[0], (gmtOffset >= 0) ? '+' : '-',
	         abs(gmtOffset), (tzname[1] != NULL) ? tzname[1] : "");
#else
	sprintf(szBuffer, "%s%c%02d%s", tzname[0], (gmtOffset >= 0) ? '+' : '-',
	        abs(gmtOffset), (tzname[1] != NULL) ? tzname[1] : "");
#endif

#endif

	msg.setField(VID_TIMEZONE, szBuffer);
	debugPrintf(2, _T("Server time zone: %s"), szBuffer);

	ConfigReadStr(_T("TileServerURL"), szBuffer, 1024, _T("http://tile.openstreetmap.org/"));
	msg.setField(VID_TILE_SERVER_URL, szBuffer);

	ConfigReadStr(_T("DefaultConsoleDateFormat"), szBuffer, 1024, _T("dd.MM.yyyy"));
	msg.setField(VID_DATE_FORMAT, szBuffer);

	ConfigReadStr(_T("DefaultConsoleTimeFormat"), szBuffer, 1024, _T("HH:mm:ss"));
	msg.setField(VID_TIME_FORMAT, szBuffer);

	ConfigReadStr(_T("DefaultConsoleShortTimeFormat"), szBuffer, 1024, _T("HH:mm"));
	msg.setField(VID_SHORT_TIME_FORMAT, szBuffer);

   // Send response
   sendMessage(&msg);
}

/**
 * Authenticate client
 */
void ClientSession::login(NXCPMessage *pRequest)
{
   NXCPMessage msg;
   TCHAR szLogin[MAX_USER_NAME], szPassword[1024];
	int nAuthType;
   bool changePasswd = false, intruderLockout = false;
   UINT32 dwResult;
#ifdef _WITH_ENCRYPTION
	X509 *pCert;
#endif

   // Prepare response message
   msg.setCode(CMD_LOGIN_RESP);
   msg.setId(pRequest->getId());

   // Get client info string
   if (pRequest->isFieldExist(VID_CLIENT_INFO))
   {
      TCHAR szClientInfo[32], szOSInfo[32], szLibVersion[16];

      pRequest->getFieldAsString(VID_CLIENT_INFO, szClientInfo, 32);
      pRequest->getFieldAsString(VID_OS_INFO, szOSInfo, 32);
      pRequest->getFieldAsString(VID_LIBNXCL_VERSION, szLibVersion, 16);
      _sntprintf(m_clientInfo, 96, _T("%s (%s; libnxcl %s)"), szClientInfo, szOSInfo, szLibVersion);
   }

	m_clientType = pRequest->getFieldAsUInt16(VID_CLIENT_TYPE);
	if ((m_clientType < 0) || (m_clientType > CLIENT_TYPE_APPLICATION))
		m_clientType = CLIENT_TYPE_DESKTOP;

   if (m_clientType == CLIENT_TYPE_WEB)
   {
      _tcscpy(m_webServerAddress, m_workstation);
      if (pRequest->isFieldExist(VID_CLIENT_ADDRESS))
      {
         pRequest->getFieldAsString(VID_CLIENT_ADDRESS, m_workstation, 256);
         debugPrintf(5, _T("Real web client address is %s"), m_workstation);
      }
   }

   if (pRequest->isFieldExist(VID_LANGUAGE))
   {
      pRequest->getFieldAsString(VID_LANGUAGE, m_language, 8);
   }

   if (!(m_dwFlags & CSF_AUTHENTICATED))
   {
      pRequest->getFieldAsString(VID_LOGIN_NAME, szLogin, MAX_USER_NAME);
		nAuthType = (int)pRequest->getFieldAsUInt16(VID_AUTH_TYPE);
      debugPrintf(6, _T("authentication type %d"), nAuthType);
		switch(nAuthType)
		{
			case NETXMS_AUTH_TYPE_PASSWORD:
#ifdef UNICODE
				pRequest->getFieldAsString(VID_PASSWORD, szPassword, 256);
#else
				pRequest->getFieldAsUtf8String(VID_PASSWORD, szPassword, 1024);
#endif
				dwResult = AuthenticateUser(szLogin, szPassword, 0, NULL, NULL, &m_dwUserId,
													 &m_dwSystemAccess, &changePasswd, &intruderLockout, false);
				break;
			case NETXMS_AUTH_TYPE_CERTIFICATE:
#ifdef _WITH_ENCRYPTION
				pCert = CertificateFromLoginMessage(pRequest);
				if (pCert != NULL)
				{
					BYTE signature[256];
					UINT32 dwSigLen;

					dwSigLen = pRequest->getFieldAsBinary(VID_SIGNATURE, signature, 256);
					dwResult = AuthenticateUser(szLogin, (TCHAR *)signature, dwSigLen, pCert,
														 m_challenge, &m_dwUserId, &m_dwSystemAccess,
														 &changePasswd, &intruderLockout, false);
					X509_free(pCert);
				}
				else
				{
					dwResult = RCC_BAD_CERTIFICATE;
				}
#else
				dwResult = RCC_NOT_IMPLEMENTED;
#endif
				break;
         case NETXMS_AUTH_TYPE_SSO_TICKET:
            char ticket[1024];
            pRequest->getFieldAsMBString(VID_PASSWORD, ticket, 1024);
            if (CASAuthenticate(ticket, szLogin))
            {
               debugPrintf(5, _T("SSO ticket %hs is valid, login name %s"), ticket, szLogin);
				   dwResult = AuthenticateUser(szLogin, NULL, 0, NULL, NULL, &m_dwUserId,
													    &m_dwSystemAccess, &changePasswd, &intruderLockout, true);
            }
            else
            {
               debugPrintf(5, _T("SSO ticket %hs is invalid"), ticket);
               dwResult = RCC_ACCESS_DENIED;
            }
            break;
			default:
				dwResult = RCC_UNSUPPORTED_AUTH_TYPE;
				break;
		}

      // Additional validation by loaded modules
      if (dwResult == RCC_SUCCESS)
      {
         for(UINT32 i = 0; i < g_dwNumModules; i++)
         {
            if (g_pModuleList[i].pfAdditionalLoginCheck != NULL)
            {
               dwResult = g_pModuleList[i].pfAdditionalLoginCheck(m_dwUserId, pRequest);
               if (dwResult != RCC_SUCCESS)
               {
                  debugPrintf(4, _T("Login blocked by module %s (rcc=%d)"), g_pModuleList[i].szName, dwResult);
                  break;
               }
            }
         }
      }

      if (dwResult == RCC_SUCCESS)
      {
         m_dwFlags |= CSF_AUTHENTICATED;
         nx_strncpy(m_loginName, szLogin, MAX_USER_NAME);
         _sntprintf(m_sessionName, MAX_SESSION_NAME, _T("%s@%s"), szLogin, m_workstation);
         m_loginTime = time(NULL);
         msg.setField(VID_RCC, RCC_SUCCESS);
         msg.setField(VID_USER_SYS_RIGHTS, m_dwSystemAccess);
         msg.setField(VID_USER_ID, m_dwUserId);
			msg.setField(VID_SESSION_ID, (UINT32)m_id);
			msg.setField(VID_CHANGE_PASSWD_FLAG, (WORD)changePasswd);
         msg.setField(VID_DBCONN_STATUS, (UINT16)((g_flags & AF_DB_CONNECTION_LOST) ? 0 : 1));
			msg.setField(VID_ZONING_ENABLED, (UINT16)((g_flags & AF_ENABLE_ZONING) ? 1 : 0));
			msg.setField(VID_POLLING_INTERVAL, ConfigReadULong(_T("DefaultDCIPollingInterval"), 60));
			msg.setField(VID_RETENTION_TIME, ConfigReadULong(_T("DefaultDCIRetentionTime"), 30));
			msg.setField(VID_ALARM_STATUS_FLOW_STATE, (UINT16)ConfigReadInt(_T("StrictAlarmStatusFlow"), 0));
			msg.setField(VID_TIMED_ALARM_ACK_ENABLED, (UINT16)ConfigReadInt(_T("EnableTimedAlarmAck"), 0));
			msg.setField(VID_VIEW_REFRESH_INTERVAL, (UINT16)ConfigReadInt(_T("MinViewRefreshInterval"), 200));
			msg.setField(VID_HELPDESK_LINK_ACTIVE, (UINT16)((g_flags & AF_HELPDESK_LINK_ACTIVE) ? 1 : 0));
			msg.setField(VID_ALARM_LIST_DISP_LIMIT, ConfigReadULong(_T("AlarmListDisplayLimit"), 4096));
         debugPrintf(3, _T("User %s authenticated (language=%s clientInfo=\"%s\")"), m_sessionName, m_language, m_clientInfo);
			WriteAuditLog(AUDIT_SECURITY, TRUE, m_dwUserId, m_workstation, m_id, 0,
            _T("User \"%s\" logged in (language: %s; client info: %s)"), szLogin, m_language, m_clientInfo);
      }
      else
      {
         msg.setField(VID_RCC, dwResult);
			WriteAuditLog(AUDIT_SECURITY, FALSE, m_dwUserId, m_workstation, m_id, 0,
			              _T("User \"%s\" login failed with error code %d (client info: %s)"),
							  szLogin, dwResult, m_clientInfo);
			if (intruderLockout)
			{
				WriteAuditLog(AUDIT_SECURITY, FALSE, m_dwUserId, m_workstation, m_id, 0,
								  _T("User account \"%s\" temporary disabled due to excess count of failed authentication attempts"), szLogin);
			}
      }
   }
   else
   {
      msg.setField(VID_RCC, RCC_OUT_OF_STATE_REQUEST);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Send event configuration to client
 */
void ClientSession::sendEventDB(UINT32 dwRqId)
{
   DB_ASYNC_RESULT hResult;
   NXCPMessage msg;
   TCHAR szBuffer[4096];

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(dwRqId);

   if (checkSysAccessRights(SYSTEM_ACCESS_VIEW_EVENT_DB) || checkSysAccessRights(SYSTEM_ACCESS_EDIT_EVENT_DB) || checkSysAccessRights(SYSTEM_ACCESS_EPP))
   {
      if (!(g_flags & AF_DB_CONNECTION_LOST))
      {
         msg.setField(VID_RCC, RCC_SUCCESS);
         sendMessage(&msg);
         msg.deleteAllFields();

         // Prepare data message
         msg.setCode(CMD_EVENT_DB_RECORD);
         msg.setId(dwRqId);

         DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
         hResult = DBAsyncSelect(hdb, _T("SELECT event_code,event_name,severity,flags,message,description FROM event_cfg"));
         if (hResult != NULL)
         {
            while(DBFetch(hResult))
            {
               msg.setField(VID_EVENT_CODE, DBGetFieldAsyncULong(hResult, 0));
               msg.setField(VID_NAME, DBGetFieldAsync(hResult, 1, szBuffer, 1024));
               msg.setField(VID_SEVERITY, DBGetFieldAsyncULong(hResult, 2));
               msg.setField(VID_FLAGS, DBGetFieldAsyncULong(hResult, 3));

               DBGetFieldAsync(hResult, 4, szBuffer, 4096);
               msg.setField(VID_MESSAGE, szBuffer);

               DBGetFieldAsync(hResult, 5, szBuffer, 4096);
               msg.setField(VID_DESCRIPTION, szBuffer);

               sendMessage(&msg);
               msg.deleteAllFields();
            }
            DBFreeAsyncResult(hResult);
         }
         DBConnectionPoolReleaseConnection(hdb);

         // End-of-list indicator
         msg.setField(VID_EVENT_CODE, (UINT32)0);
			msg.setEndOfSequence();
      }
      else
      {
         msg.setField(VID_RCC, RCC_DB_CONNECTION_LOST);
      }
   }
   else
   {
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);
   }
   sendMessage(&msg);
}

/**
 * Callback for sending event configuration change notifications
 */
static void SendEventDBChangeNotification(ClientSession *session, void *arg)
{
	if (session->isAuthenticated() &&
       (session->checkSysAccessRights(SYSTEM_ACCESS_VIEW_EVENT_DB) ||
        session->checkSysAccessRights(SYSTEM_ACCESS_EDIT_EVENT_DB) ||
        session->checkSysAccessRights(SYSTEM_ACCESS_EPP)))
		session->postMessage((NXCPMessage *)arg);
}

/**
 * Update event template
 */
void ClientSession::modifyEventTemplate(NXCPMessage *pRequest)
{
   NXCPMessage msg;

   // Prepare reply message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(pRequest->getId());

   // Check access rights
   if (checkSysAccessRights(SYSTEM_ACCESS_EDIT_EVENT_DB))
   {
      DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

      // Check if event with specific code exists
      UINT32 dwEventCode = pRequest->getFieldAsUInt32(VID_EVENT_CODE);
		bool bEventExist = IsDatabaseRecordExist(hdb, _T("event_cfg"), _T("event_code"), dwEventCode);

      // Check that we are not trying to create event below 100000
      if (bEventExist || (dwEventCode >= FIRST_USER_EVENT_ID))
      {
         // Prepare and execute SQL query
         TCHAR name[MAX_EVENT_NAME];
         pRequest->getFieldAsString(VID_NAME, name, MAX_EVENT_NAME);
         if (IsValidObjectName(name, TRUE))
         {
            DB_STATEMENT hStmt;
            if (bEventExist)
            {
               hStmt = DBPrepare(hdb, _T("UPDATE event_cfg SET event_name=?,severity=?,flags=?,message=?,description=? WHERE event_code=?"));
            }
            else
            {
               hStmt = DBPrepare(hdb, _T("INSERT INTO event_cfg (event_name,severity,flags,message,description,event_code) VALUES (?,?,?,?,?,?)"));
            }

            if (hStmt != NULL)
            {
               DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, name, DB_BIND_STATIC);
               DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, pRequest->getFieldAsInt32(VID_SEVERITY));
               DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, pRequest->getFieldAsInt32(VID_FLAGS));
               DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, pRequest->getFieldAsString(VID_MESSAGE), DB_BIND_DYNAMIC, MAX_EVENT_MSG_LENGTH - 1);
               DBBind(hStmt, 5, DB_SQLTYPE_TEXT, pRequest->getFieldAsString(VID_DESCRIPTION), DB_BIND_DYNAMIC);
               DBBind(hStmt, 6, DB_SQLTYPE_INTEGER, dwEventCode);

               if (DBExecute(hStmt))
               {
                  msg.setField(VID_RCC, RCC_SUCCESS);
                  ReloadEvents();

					   NXCPMessage nmsg(pRequest);
					   nmsg.setCode(CMD_EVENT_DB_UPDATE);
					   nmsg.setField(VID_NOTIFICATION_CODE, (WORD)NX_NOTIFY_ETMPL_CHANGED);
					   EnumerateClientSessions(SendEventDBChangeNotification, &nmsg);
               }
               else
               {
                  msg.setField(VID_RCC, RCC_DB_FAILURE);
               }
               DBFreeStatement(hStmt);
            }
            else
            {
               msg.setField(VID_RCC, RCC_DB_FAILURE);
            }
         }
         else
         {
            msg.setField(VID_RCC, RCC_INVALID_OBJECT_NAME);
         }
      }
      else
      {
         msg.setField(VID_RCC, RCC_INVALID_EVENT_CODE);
      }
      DBConnectionPoolReleaseConnection(hdb);
   }
   else
   {
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Delete event template
 */
void ClientSession::deleteEventTemplate(NXCPMessage *pRequest)
{
   NXCPMessage msg;
   UINT32 dwEventCode;

   // Prepare reply message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(pRequest->getId());

   dwEventCode = pRequest->getFieldAsUInt32(VID_EVENT_CODE);

   // Check access rights
   if (checkSysAccessRights(SYSTEM_ACCESS_EDIT_EVENT_DB) && (dwEventCode >= FIRST_USER_EVENT_ID))
   {
      TCHAR szQuery[256];

      _sntprintf(szQuery, 256, _T("DELETE FROM event_cfg WHERE event_code=%d"), dwEventCode);
      if (DBQuery(g_hCoreDB, szQuery))
      {
         DeleteEventTemplateFromList(dwEventCode);

			NXCPMessage nmsg;
			nmsg.setCode(CMD_EVENT_DB_UPDATE);
			nmsg.setField(VID_NOTIFICATION_CODE, (WORD)NX_NOTIFY_ETMPL_DELETED);
			nmsg.setField(VID_EVENT_CODE, dwEventCode);
			EnumerateClientSessions(SendEventDBChangeNotification, &nmsg);

         msg.setField(VID_RCC, RCC_SUCCESS);

			WriteAuditLog(AUDIT_SYSCFG, TRUE, m_dwUserId, m_workstation, m_id, 0, _T("Event template %d deleted"), dwEventCode);
      }
      else
      {
         msg.setField(VID_RCC, RCC_DB_FAILURE);
      }
   }
   else
   {
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Generate event code for new event template
 */
void ClientSession::generateEventCode(UINT32 dwRqId)
{
   NXCPMessage msg;

   // Prepare reply message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(dwRqId);

   // Check access rights
   if (checkSysAccessRights(SYSTEM_ACCESS_EDIT_EVENT_DB))
   {
      msg.setField(VID_EVENT_CODE, CreateUniqueId(IDG_EVENT));
   }
   else
   {
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Send all objects to client
 */
void ClientSession::sendAllObjects(NXCPMessage *pRequest)
{
   NXCPMessage msg;

   // Send confirmation message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(pRequest->getId());
   msg.setField(VID_RCC, RCC_SUCCESS);
   sendMessage(&msg);
   msg.deleteAllFields();

   // Change "sync comments" flag
   if (pRequest->getFieldAsUInt16(VID_SYNC_COMMENTS))
      m_dwFlags |= CSF_SYNC_OBJECT_COMMENTS;
   else
      m_dwFlags &= ~CSF_SYNC_OBJECT_COMMENTS;

   // Get client's last known time stamp
   UINT32 dwTimeStamp = pRequest->getFieldAsUInt32(VID_TIMESTAMP);

   // Prepare message
   msg.setCode(CMD_OBJECT);

   // Send objects, one per message
	ObjectArray<NetObj> *objects = g_idxObjectById.getObjects(true);
   MutexLock(m_mutexSendObjects);
	for(int i = 0; i < objects->size(); i++)
	{
		NetObj *object = objects->get(i);
      if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ) &&
          (object->getTimeStamp() >= dwTimeStamp) &&
          !object->isHidden() && !object->isSystem())
      {
         object->fillMessage(&msg);
         if (m_dwFlags & CSF_SYNC_OBJECT_COMMENTS)
            object->commentsToMessage(&msg);
         if (!object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_MODIFY))
         {
            // mask passwords
            msg.setField(VID_SHARED_SECRET, _T("********"));
            msg.setField(VID_SNMP_AUTH_PASSWORD, _T("********"));
            msg.setField(VID_SNMP_PRIV_PASSWORD, _T("********"));
         }
         sendMessage(&msg);
         msg.deleteAllFields();
      }
      object->decRefCount();
	}
	delete objects;

   // Send end of list notification
   msg.setCode(CMD_OBJECT_LIST_END);
   sendMessage(&msg);

   MutexUnlock(m_mutexSendObjects);
}

/**
 * Send selected objects to client
 */
void ClientSession::sendSelectedObjects(NXCPMessage *pRequest)
{
   NXCPMessage msg;

   // Send confirmation message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(pRequest->getId());
   msg.setField(VID_RCC, RCC_SUCCESS);
   sendMessage(&msg);
   msg.deleteAllFields();

   // Change "sync comments" flag
   if (pRequest->getFieldAsBoolean(VID_SYNC_COMMENTS))
      m_dwFlags |= CSF_SYNC_OBJECT_COMMENTS;
   else
      m_dwFlags &= ~CSF_SYNC_OBJECT_COMMENTS;

   UINT32 dwTimeStamp = pRequest->getFieldAsUInt32(VID_TIMESTAMP);
	UINT32 numObjects = pRequest->getFieldAsUInt32(VID_NUM_OBJECTS);
	UINT32 *objects = (UINT32 *)malloc(sizeof(UINT32) * numObjects);
	pRequest->getFieldAsInt32Array(VID_OBJECT_LIST, numObjects, objects);
	UINT32 options = pRequest->getFieldAsUInt16(VID_FLAGS);

   MutexLock(m_mutexSendObjects);

   // Prepare message
	msg.setCode((options & OBJECT_SYNC_SEND_UPDATES) ? CMD_OBJECT_UPDATE : CMD_OBJECT);

   // Send objects, one per message
   for(UINT32 i = 0; i < numObjects; i++)
	{
		NetObj *object = FindObjectById(objects[i]);
      if ((object != NULL) &&
		    object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ) &&
          (object->getTimeStamp() >= dwTimeStamp) &&
          !object->isHidden() && !object->isSystem())
      {
         object->fillMessage(&msg);
         if (m_dwFlags & CSF_SYNC_OBJECT_COMMENTS)
            object->commentsToMessage(&msg);
         if (!object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_MODIFY))
         {
            // mask passwords
            msg.setField(VID_SHARED_SECRET, _T("********"));
            msg.setField(VID_SNMP_AUTH_PASSWORD, _T("********"));
            msg.setField(VID_SNMP_PRIV_PASSWORD, _T("********"));
         }
         sendMessage(&msg);
         msg.deleteAllFields();
      }
	}

   MutexUnlock(m_mutexSendObjects);
	safe_free(objects);

	if (options & OBJECT_SYNC_DUAL_CONFIRM)
	{
		msg.setCode(CMD_REQUEST_COMPLETED);
		msg.setField(VID_RCC, RCC_SUCCESS);
      sendMessage(&msg);
	}
}

/**
 * Send event log records to client
 */
void ClientSession::sendEventLog(NXCPMessage *pRequest)
{
   NXCPMessage msg;
   DB_ASYNC_RESULT hResult = NULL;
   DB_RESULT hTempResult;
   UINT32 dwRqId, dwMaxRecords, dwNumRows, dwId;
   TCHAR szQuery[1024], szBuffer[1024];
   WORD wRecOrder;

   dwRqId = pRequest->getId();
   dwMaxRecords = pRequest->getFieldAsUInt32(VID_MAX_RECORDS);
   wRecOrder = ((g_dbSyntax == DB_SYNTAX_MSSQL) || (g_dbSyntax == DB_SYNTAX_ORACLE)) ? RECORD_ORDER_REVERSED : RECORD_ORDER_NORMAL;

   // Prepare confirmation message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(dwRqId);

   MutexLock(m_mutexSendEvents);
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   // Retrieve events from database
   switch(g_dbSyntax)
   {
      case DB_SYNTAX_MYSQL:
      case DB_SYNTAX_PGSQL:
      case DB_SYNTAX_SQLITE:
         dwNumRows = 0;
         hTempResult = DBSelect(hdb, _T("SELECT count(*) FROM event_log"));
         if (hTempResult != NULL)
         {
            if (DBGetNumRows(hTempResult) > 0)
            {
               dwNumRows = DBGetFieldULong(hTempResult, 0, 0);
            }
            DBFreeResult(hTempResult);
         }
         _sntprintf(szQuery, 1024,
                    _T("SELECT event_id,event_code,event_timestamp,event_source,")
                    _T("event_severity,event_message,user_tag FROM event_log ")
                    _T("ORDER BY event_id LIMIT %u OFFSET %u"),
                    dwMaxRecords, dwNumRows - min(dwNumRows, dwMaxRecords));
         break;
      case DB_SYNTAX_MSSQL:
         _sntprintf(szQuery, 1024,
                    _T("SELECT TOP %u event_id,event_code,event_timestamp,event_source,")
                    _T("event_severity,event_message,user_tag FROM event_log ")
                    _T("ORDER BY event_id DESC"), dwMaxRecords);
         break;
      case DB_SYNTAX_ORACLE:
         _sntprintf(szQuery, 1024,
                    _T("SELECT event_id,event_code,event_timestamp,event_source,")
                    _T("event_severity,event_message,user_tag FROM event_log ")
                    _T("WHERE ROWNUM <= %u ORDER BY event_id DESC"), dwMaxRecords);
         break;
      case DB_SYNTAX_DB2:
         _sntprintf(szQuery, 1024,
                    _T("SELECT event_id,event_code,event_timestamp,event_source,")
                    _T("event_severity,event_message,user_tag FROM event_log ")
                    _T("ORDER BY event_id DESC FETCH FIRST %u ROWS ONLY"), dwMaxRecords);
         break;
      default:
         szQuery[0] = 0;
         break;
   }
   hResult = DBAsyncSelect(hdb, szQuery);
   if (hResult != NULL)
   {
      msg.setField(VID_RCC, RCC_SUCCESS);
   	sendMessage(&msg);
   	msg.deleteAllFields();
	   msg.setCode(CMD_EVENTLOG_RECORDS);

      for(dwId = VID_EVENTLOG_MSG_BASE, dwNumRows = 0; DBFetch(hResult); dwNumRows++)
      {
         if (dwNumRows == 10)
         {
            msg.setField(VID_NUM_RECORDS, dwNumRows);
            msg.setField(VID_RECORDS_ORDER, wRecOrder);
            sendMessage(&msg);
            msg.deleteAllFields();
            dwNumRows = 0;
            dwId = VID_EVENTLOG_MSG_BASE;
         }
         msg.setField(dwId++, DBGetFieldAsyncUInt64(hResult, 0));
         msg.setField(dwId++, DBGetFieldAsyncULong(hResult, 1));
         msg.setField(dwId++, DBGetFieldAsyncULong(hResult, 2));
         msg.setField(dwId++, DBGetFieldAsyncULong(hResult, 3));
         msg.setField(dwId++, (WORD)DBGetFieldAsyncLong(hResult, 4));
         DBGetFieldAsync(hResult, 5, szBuffer, 1024);
         msg.setField(dwId++, szBuffer);
         DBGetFieldAsync(hResult, 6, szBuffer, 1024);
         msg.setField(dwId++, szBuffer);
         msg.setField(dwId++, (UINT32)0);	// Do not send parameters
      }
      DBFreeAsyncResult(hResult);

      // Send remaining records with End-Of-Sequence notification
      msg.setField(VID_NUM_RECORDS, dwNumRows);
      msg.setField(VID_RECORDS_ORDER, wRecOrder);
      msg.setEndOfSequence();
      sendMessage(&msg);
   }
   else
   {
      msg.setField(VID_RCC, RCC_DB_FAILURE);
   	sendMessage(&msg);
	}

   DBConnectionPoolReleaseConnection(hdb);
   MutexUnlock(m_mutexSendEvents);
}

/**
 * Send all configuration variables to client
 */
void ClientSession::getConfigurationVariables(UINT32 dwRqId)
{
   UINT32 i, dwId, dwNumRecords;
   NXCPMessage msg;
   TCHAR szBuffer[MAX_DB_STRING];

   // Prepare message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(dwRqId);

   // Check user rights
   if ((m_dwUserId == 0) || (m_dwSystemAccess & SYSTEM_ACCESS_SERVER_CONFIG))
   {
      // Retrieve configuration variables from database
      DB_RESULT hResult = DBSelect(g_hCoreDB, _T("SELECT var_name,var_value,need_server_restart FROM config WHERE is_visible=1"));
      if (hResult != NULL)
      {
         // Send events, one per message
         dwNumRecords = DBGetNumRows(hResult);
         msg.setField(VID_NUM_VARIABLES, dwNumRecords);
         for(i = 0, dwId = VID_VARLIST_BASE; i < dwNumRecords; i++)
         {
            msg.setField(dwId++, DBGetField(hResult, i, 0, szBuffer, MAX_DB_STRING));
            DBGetField(hResult, i, 1, szBuffer, MAX_DB_STRING);
            DecodeSQLString(szBuffer);
            msg.setField(dwId++, szBuffer);
            msg.setField(dwId++, (WORD)DBGetFieldLong(hResult, i, 2));
         }
         DBFreeResult(hResult);
      }
      msg.setField(VID_RCC, RCC_SUCCESS);
   }
   else
   {
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Get public configuration variable by name
 */
void ClientSession::getPublicConfigurationVariable(NXCPMessage *request)
{
   NXCPMessage msg;
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(request->getId());

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT var_value FROM config WHERE var_name=? AND is_public='Y'"));
   if (hStmt != NULL)
   {
      TCHAR name[64];
      request->getFieldAsString(VID_NAME, name, 64);
      DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, name, DB_BIND_STATIC);

      DB_RESULT hResult = DBSelectPrepared(hStmt);
      if (hResult != NULL)
      {
         if (DBGetNumRows(hResult) > 0)
         {
            TCHAR value[256];
            msg.setField(VID_VALUE, DBGetField(hResult, 0, 0, value, 256));
            msg.setField(VID_RCC, RCC_SUCCESS);
         }
         else
         {
            msg.setField(VID_RCC, RCC_UNKNOWN_CONFIG_VARIABLE);
         }
         DBFreeResult(hResult);
      }
      else
      {
         msg.setField(VID_RCC, RCC_DB_FAILURE);
      }
      DBFreeStatement(hStmt);
   }
   else
   {
      msg.setField(VID_RCC, RCC_DB_FAILURE);
   }

   sendMessage(&msg);
}

/**
 * Set configuration variable's value
 */
void ClientSession::setConfigurationVariable(NXCPMessage *pRequest)
{
   NXCPMessage msg;
   TCHAR szName[MAX_OBJECT_NAME], szValue[MAX_DB_STRING];

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(pRequest->getId());

   // Check user rights
   if ((m_dwUserId == 0) || (m_dwSystemAccess & SYSTEM_ACCESS_SERVER_CONFIG))
   {
      pRequest->getFieldAsString(VID_NAME, szName, MAX_OBJECT_NAME);
      pRequest->getFieldAsString(VID_VALUE, szValue, MAX_DB_STRING);
      if (ConfigWriteStr(szName, szValue, true))
		{
         msg.setField(VID_RCC, RCC_SUCCESS);
			WriteAuditLog(AUDIT_SYSCFG, TRUE, m_dwUserId, m_workstation, m_id, 0,
							  _T("Server configuration variable \"%s\" set to \"%s\""), szName, szValue);
		}
      else
		{
         msg.setField(VID_RCC, RCC_DB_FAILURE);
		}
   }
   else
   {
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Delete configuration variable
 */
void ClientSession::deleteConfigurationVariable(NXCPMessage *pRequest)
{
   NXCPMessage msg;

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(pRequest->getId());

   // Check user rights
   if ((m_dwUserId == 0) || (m_dwSystemAccess & SYSTEM_ACCESS_SERVER_CONFIG))
   {
      TCHAR name[MAX_OBJECT_NAME];
      pRequest->getFieldAsString(VID_NAME, name, MAX_OBJECT_NAME);
      if (ConfigDelete(name))
		{
         msg.setField(VID_RCC, RCC_SUCCESS);
			WriteAuditLog(AUDIT_SYSCFG, TRUE, m_dwUserId, m_workstation, m_id, 0, _T("Server configuration variable \"%s\" deleted"), name);
		}
      else
		{
         msg.setField(VID_RCC, RCC_DB_FAILURE);
		}
   }
   else
   {
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Set configuration clob
 */
void ClientSession::setConfigCLOB(NXCPMessage *pRequest)
{
   NXCPMessage msg;
   TCHAR name[MAX_OBJECT_NAME], *value;

	msg.setId(pRequest->getId());
	msg.setCode(CMD_REQUEST_COMPLETED);

	if (m_dwSystemAccess & SYSTEM_ACCESS_SERVER_CONFIG)
	{
      pRequest->getFieldAsString(VID_NAME, name, MAX_OBJECT_NAME);
		value = pRequest->getFieldAsString(VID_VALUE);
		if (value != NULL)
		{
			if (ConfigWriteCLOB(name, value, TRUE))
			{
				msg.setField(VID_RCC, RCC_SUCCESS);
				WriteAuditLog(AUDIT_SYSCFG, TRUE, m_dwUserId, m_workstation, m_id, 0,
								  _T("Server configuration variable \"%s\" set to \"%s\""), name, value);
				free(value);
			}
			else
			{
				msg.setField(VID_RCC, RCC_DB_FAILURE);
			}
		}
		else
		{
			msg.setField(VID_RCC, RCC_INVALID_REQUEST);
		}
	}
	else
	{
		msg.setField(VID_RCC, RCC_ACCESS_DENIED);
	}

	sendMessage(&msg);
}

/**
 * Get value of configuration clob
 */
void ClientSession::getConfigCLOB(NXCPMessage *pRequest)
{
   NXCPMessage msg;
   TCHAR name[MAX_OBJECT_NAME], *value;

	msg.setId(pRequest->getId());
	msg.setCode(CMD_REQUEST_COMPLETED);

	if (m_dwSystemAccess & SYSTEM_ACCESS_SERVER_CONFIG)
	{
      pRequest->getFieldAsString(VID_NAME, name, MAX_OBJECT_NAME);
		value = ConfigReadCLOB(name, NULL);
		if (value != NULL)
		{
			msg.setField(VID_VALUE, value);
			msg.setField(VID_RCC, RCC_SUCCESS);
			free(value);
		}
		else
		{
			msg.setField(VID_RCC, RCC_UNKNOWN_VARIABLE);
		}
	}
	else
	{
		msg.setField(VID_RCC, RCC_ACCESS_DENIED);
	}

	sendMessage(&msg);
}

/**
 * Close session
 */
void ClientSession::kill()
{
   notify(NX_NOTIFY_SESSION_KILLED);

   // We shutdown socket connection, which will cause
   // read thread to stop, and other threads will follow
   shutdown(m_hSocket, 2);
}

/**
 * Handler for new events
 */
void ClientSession::onNewEvent(Event *pEvent)
{
   UPDATE_INFO *pUpdate;
   NXCPMessage *msg;
   if (isAuthenticated() && (m_dwActiveChannels & NXC_CHANNEL_EVENTS) && (m_dwSystemAccess & SYSTEM_ACCESS_VIEW_EVENT_LOG))
   {
      NetObj *object = FindObjectById(pEvent->getSourceId());
      //If can't find object - just send to all events, if object found send to thous who have rights
      if (object == NULL || object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
      {
         pUpdate = (UPDATE_INFO *)malloc(sizeof(UPDATE_INFO));
         pUpdate->dwCategory = INFO_CAT_EVENT;
         msg = new NXCPMessage;
         msg->setCode(CMD_EVENTLOG_RECORDS);
         pEvent->prepareMessage(msg);
         pUpdate->pData = msg;
         m_pUpdateQueue->put(pUpdate);
      }
   }
}

/**
 * Handler for object changes
 */
void ClientSession::onObjectChange(NetObj *object)
{
   UPDATE_INFO *pUpdate;

   if (isAuthenticated() && (m_dwActiveChannels & NXC_CHANNEL_OBJECTS))
      if (object->isDeleted() || object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
      {
         pUpdate = (UPDATE_INFO *)malloc(sizeof(UPDATE_INFO));
         pUpdate->dwCategory = INFO_CAT_OBJECT_CHANGE;
         pUpdate->pData = object;
         object->incRefCount();
         m_pUpdateQueue->put(pUpdate);
      }
}

/**
 * Send notification message to server
 */
void ClientSession::notify(UINT32 dwCode, UINT32 dwData)
{
   NXCPMessage msg;

   msg.setCode(CMD_NOTIFY);
   msg.setField(VID_NOTIFICATION_CODE, dwCode);
   msg.setField(VID_NOTIFICATION_DATA, dwData);
   sendMessage(&msg);
}

static void SetNodesConflictString(NXCPMessage *msg, UINT32 zoneId, InetAddress ipAddr)
{
   if(ipAddr.isValid())
   {
      TCHAR value[512];

      Node *sameNode = FindNodeByIP(zoneId, ipAddr);
      Subnet *sameSubnet = FindSubnetByIP(zoneId, ipAddr);
      if(sameNode != NULL)
      {
         _sntprintf(value, 512, _T("%s"), sameNode->getName());
      }
      else if (sameSubnet != NULL)
      {
         _sntprintf(value, 512, _T("%s"), sameSubnet->getName());
      }
      else
      {
         _tcscpy(value, _T(""));
      }
      msg->setField(VID_VALUE, value);
   }
}

/**
 * Modify object
 */
void ClientSession::modifyObject(NXCPMessage *pRequest)
{
   UINT32 dwObjectId, dwResult = RCC_SUCCESS;
   NetObj *object;
   NXCPMessage msg;

   // Prepare reply message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(pRequest->getId());

   dwObjectId = pRequest->getFieldAsUInt32(VID_OBJECT_ID);
   object = FindObjectById(dwObjectId);
   if (object != NULL)
   {
      if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_MODIFY))
      {
         // If user attempts to change object's ACL, check
         // if he has OBJECT_ACCESS_ACL permission
         if (pRequest->isFieldExist(VID_ACL_SIZE))
            if (!object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_ACL))
               dwResult = RCC_ACCESS_DENIED;

			// If user attempts to rename object, check object's name
			if (pRequest->isFieldExist(VID_OBJECT_NAME))
			{
				TCHAR name[256];
				pRequest->getFieldAsString(VID_OBJECT_NAME, name, 256);
				if (!IsValidObjectName(name, TRUE))
					dwResult = RCC_INVALID_OBJECT_NAME;
			}

         // If allowed, change object and set completion code
         if (dwResult == RCC_SUCCESS)
			{
            dwResult = object->modifyFromMessage(pRequest);
	         if (dwResult == RCC_SUCCESS)
				{
					object->postModify();
				}
				else if (dwResult == RCC_ALREADY_EXIST)
				{
               //Add information about conflicting nodes
               InetAddress ipAddr;

               if (pRequest->isFieldExist(VID_IP_ADDRESS))
               {
                  ipAddr = pRequest->getFieldAsInetAddress(VID_IP_ADDRESS);
               } else if (pRequest->isFieldExist(VID_PRIMARY_NAME))
               {
                  TCHAR primaryName[MAX_DNS_NAME];
                  pRequest->getFieldAsString(VID_PRIMARY_NAME, primaryName, MAX_DNS_NAME);
                  ipAddr = InetAddress::resolveHostName(primaryName);
               }
               SetNodesConflictString(&msg, ((Node*)object)->getZoneId(), ipAddr);
            }
			}
         msg.setField(VID_RCC, dwResult);

			if (dwResult == RCC_SUCCESS)
			{
				WriteAuditLog(AUDIT_OBJECTS, TRUE, m_dwUserId, m_workstation, m_id, dwObjectId,
								  _T("Object %s modified from client"), object->getName());
			}
			else
			{
				WriteAuditLog(AUDIT_OBJECTS, FALSE, m_dwUserId, m_workstation, m_id, dwObjectId,
								  _T("Failed to modify object from client - error %d"), dwResult);
			}
      }
      else
      {
         msg.setField(VID_RCC, RCC_ACCESS_DENIED);
			WriteAuditLog(AUDIT_OBJECTS, FALSE, m_dwUserId, m_workstation, m_id, dwObjectId,
							  _T("Failed to modify object from client - access denied"), dwResult);
      }
   }
   else
   {
      msg.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Send users database to client
 */
void ClientSession::sendUserDB(UINT32 dwRqId)
{
   NXCPMessage msg;
	UserDatabaseObject **users;
   int i, userCount;

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(dwRqId);
   msg.setField(VID_RCC, RCC_SUCCESS);
   sendMessage(&msg);
	msg.deleteAllFields();

   // Send user database
	users = OpenUserDatabase(&userCount);
   for(i = 0; i < userCount; i++)
   {
		msg.setCode((users[i]->getId() & GROUP_FLAG) ? CMD_GROUP_DATA : CMD_USER_DATA);
		users[i]->fillMessage(&msg);
      sendMessage(&msg);
      msg.deleteAllFields();
   }
	CloseUserDatabase();

   // Send end-of-database notification
   msg.setCode(CMD_USER_DB_EOF);
   sendMessage(&msg);
}

/**
 * Create new user
 */
void ClientSession::createUser(NXCPMessage *pRequest)
{
   NXCPMessage msg;

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(pRequest->getId());

   // Check user rights
   if (!(m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_USERS))
   {
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);
   }
   else if (!(m_dwFlags & CSF_USER_DB_LOCKED))
   {
      // User database have to be locked before any
      // changes to user database can be made
      msg.setField(VID_RCC, RCC_OUT_OF_STATE_REQUEST);
   }
   else
   {
      UINT32 dwResult, dwUserId;
      BOOL bIsGroup;
      TCHAR szUserName[MAX_USER_NAME];

      pRequest->getFieldAsString(VID_USER_NAME, szUserName, MAX_USER_NAME);
      if (IsValidObjectName(szUserName))
      {
         bIsGroup = pRequest->getFieldAsUInt16(VID_IS_GROUP);
         dwResult = CreateNewUser(szUserName, bIsGroup, &dwUserId);
         msg.setField(VID_RCC, dwResult);
         if (dwResult == RCC_SUCCESS)
         {
            msg.setField(VID_USER_ID, dwUserId);   // Send id of new user to client
            WriteAuditLog(AUDIT_SECURITY, TRUE, m_dwUserId, m_workstation, m_id, dwUserId, _T("%s %s created"), bIsGroup ? _T("Group") : _T("User"), szUserName);
         }
      }
      else
      {
         msg.setField(VID_RCC, RCC_INVALID_OBJECT_NAME);
      }
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Update existing user's data
 */
void ClientSession::updateUser(NXCPMessage *pRequest)
{
   NXCPMessage msg;

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(pRequest->getId());

   // Check user rights
   if (!(m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_USERS))
   {
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);
   }
   else if (!(m_dwFlags & CSF_USER_DB_LOCKED))
   {
      // User database have to be locked before any
      // changes to user database can be made
      msg.setField(VID_RCC, RCC_OUT_OF_STATE_REQUEST);
   }
   else
   {
      UINT32 result = ModifyUserDatabaseObject(pRequest);
      if (result == RCC_SUCCESS)
      {
         TCHAR name[MAX_DB_STRING];
         UINT32 id = pRequest->getFieldAsUInt32(VID_USER_ID);
         ResolveUserId(id, name, MAX_DB_STRING);
         WriteAuditLog(AUDIT_SECURITY, TRUE, m_dwUserId, m_workstation, m_id, id,
            _T("%s %s modified"), (id & GROUP_FLAG) ? _T("Group") : _T("User"), name);
      }
      msg.setField(VID_RCC, result);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Delete user
 */
void ClientSession::detachLdapUser(NXCPMessage *pRequest)
{
   NXCPMessage msg;

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(pRequest->getId());
   UINT32 id = pRequest->getFieldAsUInt32(VID_USER_ID);

   // Check user rights
   if (!(m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_USERS))
   {
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);
   }
   else if (!(m_dwFlags & CSF_USER_DB_LOCKED))
   {
      // User database have to be locked before any
      // changes to user database can be made
      msg.setField(VID_RCC, RCC_OUT_OF_STATE_REQUEST);
   }
   else
   {
      UINT32 result = DetachLdapUser(id);
      if (result == RCC_SUCCESS)
      {
         TCHAR name[MAX_DB_STRING];
         ResolveUserId(id, name, MAX_DB_STRING);
         WriteAuditLog(AUDIT_SECURITY, TRUE, m_dwUserId, m_workstation, m_id, id,
            _T("%s %s modified"), (id & GROUP_FLAG) ? _T("Group") : _T("User"), name);
      }
      msg.setField(VID_RCC, result);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Delete user
 */
void ClientSession::deleteUser(NXCPMessage *pRequest)
{
   NXCPMessage msg;
   UINT32 dwUserId;

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(pRequest->getId());

   // Check user rights
   if (!(m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_USERS))
   {
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);
   }
   else if (!(m_dwFlags & CSF_USER_DB_LOCKED))
   {
      // User database have to be locked before any
      // changes to user database can be made
      msg.setField(VID_RCC, RCC_OUT_OF_STATE_REQUEST);
   }
   else
   {
      // Get Id of user to be deleted
      dwUserId = pRequest->getFieldAsUInt32(VID_USER_ID);

      if ((dwUserId != 0) && (dwUserId != GROUP_EVERYONE))
      {
         if (!IsLoggedIn(dwUserId))
         {
            TCHAR name[MAX_DB_STRING];
            ResolveUserId(dwUserId, name, MAX_DB_STRING);
            UINT32 rcc =  DeleteUserDatabaseObject(dwUserId);
            msg.setField(VID_RCC, rcc);
            if(rcc == RCC_SUCCESS)
            {
               WriteAuditLog(AUDIT_SECURITY, TRUE, m_dwUserId, m_workstation, m_id, dwUserId,
                             _T("%s %s [%d] deleted"), (dwUserId & GROUP_FLAG) ? _T("Group") : _T("User"), name, dwUserId);
            }
         }
         else
         {
            // logger in users cannot be deleted
            msg.setField(VID_RCC, RCC_USER_LOGGED_IN);
         }
      }
      else
      {
         // System administrator account and everyone group cannot be deleted
         msg.setField(VID_RCC, RCC_ACCESS_DENIED);
      }
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Lock/unlock user database
 */
void ClientSession::lockUserDB(UINT32 dwRqId, BOOL bLock)
{
   NXCPMessage msg;
   TCHAR szBuffer[256];

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(dwRqId);

   if (m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_USERS)
   {
      if (bLock)
      {
         if (!LockComponent(CID_USER_DB, m_id, m_sessionName, NULL, szBuffer))
         {
            msg.setField(VID_RCC, RCC_COMPONENT_LOCKED);
            msg.setField(VID_LOCKED_BY, szBuffer);
         }
         else
         {
            m_dwFlags |= CSF_USER_DB_LOCKED;
            msg.setField(VID_RCC, RCC_SUCCESS);
         }
      }
      else
      {
         if (m_dwFlags & CSF_USER_DB_LOCKED)
         {
            UnlockComponent(CID_USER_DB);
            m_dwFlags &= ~CSF_USER_DB_LOCKED;
         }
         msg.setField(VID_RCC, RCC_SUCCESS);
      }
   }
   else
   {
      // Current user has no rights for user account management
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Notify client on user database update
 */
void ClientSession::onUserDBUpdate(int code, UINT32 id, UserDatabaseObject *object)
{
   NXCPMessage msg;

   if (isAuthenticated())
   {
      msg.setCode(CMD_USER_DB_UPDATE);
      msg.setId(0);
      msg.setField(VID_UPDATE_TYPE, (WORD)code);

      switch(code)
      {
         case USER_DB_CREATE:
         case USER_DB_MODIFY:
				object->fillMessage(&msg);
            break;
         default:
            msg.setField(VID_USER_ID, id);
            break;
      }

      sendMessage(&msg);
   }
}

/**
 * Change management status for the object
 */
void ClientSession::changeObjectMgmtStatus(NXCPMessage *pRequest)
{
   NXCPMessage msg;
   UINT32 dwObjectId;
   NetObj *object;

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(pRequest->getId());

   // Get object id and check access rights
   dwObjectId = pRequest->getFieldAsUInt32(VID_OBJECT_ID);
   object = FindObjectById(dwObjectId);
   if (object != NULL)
   {
      if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_MODIFY))
      {
			if ((object->getObjectClass() != OBJECT_TEMPLATE) &&
				 (object->getObjectClass() != OBJECT_TEMPLATEGROUP) &&
				 (object->getObjectClass() != OBJECT_TEMPLATEROOT))
			{
				BOOL bIsManaged = (BOOL)pRequest->getFieldAsUInt16(VID_MGMT_STATUS);
				object->setMgmtStatus(bIsManaged);
				msg.setField(VID_RCC, RCC_SUCCESS);
            WriteAuditLog(AUDIT_OBJECTS, TRUE, m_dwUserId, m_workstation, m_id, object->getId(),
               _T("Object %s set to %s state"), object->getName(), bIsManaged ? _T("managed") : _T("unmanaged"));
			}
			else
			{
	         msg.setField(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
			}
      }
      else
      {
         msg.setField(VID_RCC, RCC_ACCESS_DENIED);
      }
   }
   else
   {
      msg.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Validate password for currently logged in user
 */
void ClientSession::validatePassword(NXCPMessage *request)
{
   NXCPMessage msg;
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(request->getId());

#ifdef UNICODE
   TCHAR password[256];
   request->getFieldAsString(VID_PASSWORD, password, 256);
#else
   char password[256];
   request->getFieldAsUtf8String(VID_PASSWORD, password, 256);
#endif

   bool isValid = false;
   msg.setField(VID_RCC, ValidateUserPassword(m_dwUserId, m_loginName, password, &isValid));
   msg.setField(VID_PASSWORD_IS_VALID, (INT16)(isValid ? 1 : 0));

   sendMessage(&msg);
}

/**
 * Set user's password
 */
void ClientSession::setPassword(NXCPMessage *request)
{
   NXCPMessage msg;
   UINT32 dwUserId;

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(request->getId());

   dwUserId = request->getFieldAsUInt32(VID_USER_ID);

   if (((m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_USERS) &&
        !((dwUserId == 0) && (m_dwUserId != 0))) ||   // Only administrator can change password for UID 0
       (dwUserId == m_dwUserId))     // User can change password for itself
   {
      UINT32 dwResult;
      TCHAR newPassword[1024], oldPassword[1024];

#ifdef UNICODE
      request->getFieldAsString(VID_PASSWORD, newPassword, 256);
		if (request->isFieldExist(VID_OLD_PASSWORD))
			request->getFieldAsString(VID_OLD_PASSWORD, oldPassword, 256);
#else
      request->getFieldAsUtf8String(VID_PASSWORD, newPassword, 1024);
		if (request->isFieldExist(VID_OLD_PASSWORD))
			request->getFieldAsUtf8String(VID_OLD_PASSWORD, oldPassword, 1024);
#endif
		else
			oldPassword[0] = 0;
      dwResult = SetUserPassword(dwUserId, newPassword, oldPassword, dwUserId == m_dwUserId);
      msg.setField(VID_RCC, dwResult);

      if (dwResult == RCC_SUCCESS)
      {
         TCHAR userName[MAX_DB_STRING];
         ResolveUserId(dwUserId, userName, MAX_DB_STRING);
         WriteAuditLog(AUDIT_SECURITY, TRUE, m_dwUserId, m_workstation, m_id, 0, _T("Changed password for user %s"), userName);
      }
   }
   else
   {
      // Current user has no rights to change password for specific user
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Send node's DCIs to client and lock data collection settings
 */
void ClientSession::openNodeDCIList(NXCPMessage *request)
{
   NXCPMessage msg;
   UINT32 dwObjectId;
   NetObj *object;
   BOOL bSuccess = FALSE;
   TCHAR szLockInfo[MAX_SESSION_NAME];

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(request->getId());

   // Get node id and check object class and access rights
   dwObjectId = request->getFieldAsUInt32(VID_OBJECT_ID);
   object = FindObjectById(dwObjectId);
   if (object != NULL)
   {
      if ((object->getObjectClass() == OBJECT_NODE) ||
          (object->getObjectClass() == OBJECT_CLUSTER) ||
          (object->getObjectClass() == OBJECT_MOBILEDEVICE) ||
          (object->getObjectClass() == OBJECT_TEMPLATE))
      {
         if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
         {
            // Try to lock DCI list
            if (((Template *)object)->lockDCIList(m_id, m_sessionName, szLockInfo))
            {
               bSuccess = TRUE;
               msg.setField(VID_RCC, RCC_SUCCESS);

               // modify list of open nodes DCI lists
               m_pOpenDCIList = (UINT32 *)realloc(m_pOpenDCIList, sizeof(UINT32) * (m_dwOpenDCIListSize + 1));
               m_pOpenDCIList[m_dwOpenDCIListSize] = dwObjectId;
               m_dwOpenDCIListSize++;
            }
            else
            {
               msg.setField(VID_RCC, RCC_COMPONENT_LOCKED);
               msg.setField(VID_LOCKED_BY, szLockInfo);
            }
         }
         else
         {
            msg.setField(VID_RCC, RCC_ACCESS_DENIED);
         }
      }
      else
      {
         msg.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
      }
   }
   else
   {
      msg.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   // Send response
   sendMessage(&msg);

   // If DCI list was successfully locked, send it to client
   if (bSuccess)
      ((Template *)object)->sendItemsToClient(this, request->getId());
}

/**
 * Unlock node's data collection settings
 */
void ClientSession::closeNodeDCIList(NXCPMessage *request)
{
   NXCPMessage msg;
   UINT32 dwObjectId;
   NetObj *object;

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(request->getId());

   // Get node id and check object class and access rights
   dwObjectId = request->getFieldAsUInt32(VID_OBJECT_ID);
   object = FindObjectById(dwObjectId);
   if (object != NULL)
   {
      if ((object->getObjectClass() == OBJECT_NODE) ||
          (object->getObjectClass() == OBJECT_CLUSTER) ||
          (object->getObjectClass() == OBJECT_MOBILEDEVICE) ||
          (object->getObjectClass() == OBJECT_TEMPLATE))
      {
         if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
         {
            BOOL bSuccess;

            // Try to unlock DCI list
            bSuccess = ((Template *)object)->unlockDCIList(m_id);
            msg.setField(VID_RCC, bSuccess ? RCC_SUCCESS : RCC_OUT_OF_STATE_REQUEST);

            // modify list of open nodes DCI lists
            if (bSuccess)
            {
               UINT32 i;

               for(i = 0; i < m_dwOpenDCIListSize; i++)
                  if (m_pOpenDCIList[i] == dwObjectId)
                  {
                     m_dwOpenDCIListSize--;
                     memmove(&m_pOpenDCIList[i], &m_pOpenDCIList[i + 1], sizeof(UINT32) * (m_dwOpenDCIListSize - i));
                     break;
                  }
            }

            // Queue template update
            if ((object->getObjectClass() == OBJECT_TEMPLATE) || (object->getObjectClass() == OBJECT_CLUSTER))
               ((Template *)object)->queueUpdate();
         }
         else
         {
            msg.setField(VID_RCC, RCC_ACCESS_DENIED);
         }
      }
      else
      {
         msg.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
      }
   }
   else
   {
      msg.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Create, modify, or delete data collection item for node
 */
void ClientSession::modifyNodeDCI(NXCPMessage *request)
{
   NXCPMessage msg;
   UINT32 dwObjectId;
   NetObj *object;

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(request->getId());

   // Get node id and check object class and access rights
   dwObjectId = request->getFieldAsUInt32(VID_OBJECT_ID);
   object = FindObjectById(dwObjectId);
   if (object != NULL)
   {
      if ((object->getObjectClass() == OBJECT_NODE) ||
          (object->getObjectClass() == OBJECT_CLUSTER) ||
          (object->getObjectClass() == OBJECT_MOBILEDEVICE) ||
          (object->getObjectClass() == OBJECT_TEMPLATE))
      {
         if (((Template *)object)->isLockedBySession(m_id))
         {
            if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_MODIFY))
            {
               UINT32 i, dwItemId, dwNumMaps, *pdwMapId, *pdwMapIndex;
               DCObject *dcObject;
               BOOL bSuccess = FALSE;

					int dcObjectType = (int)request->getFieldAsUInt16(VID_DCOBJECT_TYPE);
               switch(request->getCode())
               {
                  case CMD_CREATE_NEW_DCI:
                     // Create dummy DCI
							switch(dcObjectType)
							{
								case DCO_TYPE_ITEM:
									dcObject = new DCItem(CreateUniqueId(IDG_ITEM), _T("no name"), DS_INTERNAL, DCI_DT_INT,
										ConfigReadInt(_T("DefaultDCIPollingInterval"), 60),
										ConfigReadInt(_T("DefaultDCIRetentionTime"), 30), (Node *)object);
									break;
								case DCO_TYPE_TABLE:
									dcObject = new DCTable(CreateUniqueId(IDG_ITEM), _T("no name"), DS_INTERNAL,
										ConfigReadInt(_T("DefaultDCIPollingInterval"), 60),
										ConfigReadInt(_T("DefaultDCIRetentionTime"), 30), (Node *)object);
									break;
								default:
									dcObject = NULL;
									break;
							}
							if (dcObject != NULL)
							{
								dcObject->setStatus(ITEM_STATUS_DISABLED, false);
								if ((bSuccess = ((Template *)object)->addDCObject(dcObject)))
								{
									msg.setField(VID_RCC, RCC_SUCCESS);
									// Return new item id to client
									msg.setField(VID_DCI_ID, dcObject->getId());
								}
								else  // Unable to add item to node
								{
									delete dcObject;
									msg.setField(VID_RCC, RCC_DUPLICATE_DCI);
								}
							}
							else
							{
								msg.setField(VID_RCC, RCC_INVALID_ARGUMENT);
							}
                     break;
                  case CMD_MODIFY_NODE_DCI:
                     dwItemId = request->getFieldAsUInt32(VID_DCI_ID);
							bSuccess = ((Template *)object)->updateDCObject(dwItemId, request, &dwNumMaps, &pdwMapIndex, &pdwMapId);
                     if (bSuccess)
                     {
                        msg.setField(VID_RCC, RCC_SUCCESS);

                        // Send index to id mapping for newly created thresholds to client
								if (dcObjectType == DCO_TYPE_ITEM)
								{
									msg.setField(VID_DCI_NUM_MAPS, dwNumMaps);
									for(i = 0; i < dwNumMaps; i++)
									{
										pdwMapId[i] = htonl(pdwMapId[i]);
										pdwMapIndex[i] = htonl(pdwMapIndex[i]);
									}
									msg.setField(VID_DCI_MAP_IDS, (BYTE *)pdwMapId, sizeof(UINT32) * dwNumMaps);
									msg.setField(VID_DCI_MAP_INDEXES, (BYTE *)pdwMapIndex, sizeof(UINT32) * dwNumMaps);
									safe_free(pdwMapId);
									safe_free(pdwMapIndex);
								}
                     }
                     else
                     {
                        msg.setField(VID_RCC, RCC_INVALID_DCI_ID);
                     }
                     break;
                  case CMD_DELETE_NODE_DCI:
                     dwItemId = request->getFieldAsUInt32(VID_DCI_ID);
                     bSuccess = ((Template *)object)->deleteDCObject(dwItemId, true);
                     msg.setField(VID_RCC, bSuccess ? RCC_SUCCESS : RCC_INVALID_DCI_ID);
                     break;
               }
               if (bSuccess)
                  ((Template *)object)->setDCIModificationFlag();
            }
            else  // User doesn't have MODIFY rights on object
            {
               msg.setField(VID_RCC, RCC_ACCESS_DENIED);
            }
         }
         else  // Nodes DCI list not locked by this session
         {
            msg.setField(VID_RCC, RCC_OUT_OF_STATE_REQUEST);
         }
      }
      else     // Object is not a node
      {
         msg.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
      }
   }
   else  // No object with given ID
   {
      msg.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Change status for one or more DCIs
 */
void ClientSession::changeDCIStatus(NXCPMessage *request)
{
   NXCPMessage msg;
   NetObj *object;

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(request->getId());

   // Get node id and check object class and access rights
   object = FindObjectById(request->getFieldAsUInt32(VID_OBJECT_ID));
   if (object != NULL)
   {
      if ((object->getObjectClass() == OBJECT_NODE) ||
          (object->getObjectClass() == OBJECT_CLUSTER) ||
          (object->getObjectClass() == OBJECT_MOBILEDEVICE) ||
          (object->getObjectClass() == OBJECT_TEMPLATE))
      {
         if (((Template *)object)->isLockedBySession(m_id))
         {
            if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_MODIFY))
            {
               UINT32 dwNumItems, *pdwItemList;
               int iStatus;

               iStatus = request->getFieldAsUInt16(VID_DCI_STATUS);
               dwNumItems = request->getFieldAsUInt32(VID_NUM_ITEMS);
               pdwItemList = (UINT32 *)malloc(sizeof(UINT32) * dwNumItems);
               request->getFieldAsInt32Array(VID_ITEM_LIST, dwNumItems, pdwItemList);
               if (((Template *)object)->setItemStatus(dwNumItems, pdwItemList, iStatus))
                  msg.setField(VID_RCC, RCC_SUCCESS);
               else
                  msg.setField(VID_RCC, RCC_INVALID_DCI_ID);
               free(pdwItemList);
            }
            else  // User doesn't have MODIFY rights on object
            {
               msg.setField(VID_RCC, RCC_ACCESS_DENIED);
            }
         }
         else  // Nodes DCI list not locked by this session
         {
            msg.setField(VID_RCC, RCC_OUT_OF_STATE_REQUEST);
         }
      }
      else     // Object is not a node
      {
         msg.setField(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
      }
   }
   else  // No object with given ID
   {
      msg.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Clear all collected data for DCI
 */
void ClientSession::clearDCIData(NXCPMessage *request)
{
   NXCPMessage msg;
   NetObj *object;
	UINT32 dwItemId;

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(request->getId());

   // Get node id and check object class and access rights
   object = FindObjectById(request->getFieldAsUInt32(VID_OBJECT_ID));
   if (object != NULL)
   {
      if ((object->getObjectClass() == OBJECT_NODE) || (object->getObjectClass() == OBJECT_MOBILEDEVICE) || (object->getObjectClass() == OBJECT_CLUSTER))
      {
         if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_DELETE))
         {
				dwItemId = request->getFieldAsUInt32(VID_DCI_ID);
				debugPrintf(4, _T("ClearDCIData: request for DCI %d at node %d"), dwItemId, object->getId());
            DCObject *dci = ((Template *)object)->getDCObjectById(dwItemId);
				if (dci != NULL)
				{
					msg.setField(VID_RCC, dci->deleteAllData() ? RCC_SUCCESS : RCC_DB_FAILURE);
					debugPrintf(4, _T("ClearDCIData: DCI %d at node %d"), dwItemId, object->getId());
				}
				else
				{
					msg.setField(VID_RCC, RCC_INVALID_DCI_ID);
					debugPrintf(4, _T("ClearDCIData: DCI %d at node %d not found"), dwItemId, object->getId());
				}
         }
         else  // User doesn't have DELETE rights on object
         {
            msg.setField(VID_RCC, RCC_ACCESS_DENIED);
         }
      }
      else     // Object is not a node
      {
         msg.setField(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
      }
   }
   else  // No object with given ID
   {
      msg.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Force DCI data poll for given DCI
 */
void ClientSession::forceDCIPoll(NXCPMessage *request)
{
   NXCPMessage msg;
   NetObj *object;
	UINT32 dwItemId;

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(request->getId());

   // Get node id and check object class and access rights
   object = FindObjectById(request->getFieldAsUInt32(VID_OBJECT_ID));
   if (object != NULL)
   {
      if ((object->getObjectClass() == OBJECT_NODE) || (object->getObjectClass() == OBJECT_MOBILEDEVICE) || (object->getObjectClass() == OBJECT_CLUSTER))
      {
         if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
         {
				dwItemId = request->getFieldAsUInt32(VID_DCI_ID);
				debugPrintf(4, _T("ForceDCIPoll: request for DCI %d at node %d"), dwItemId, object->getId());
            DCObject *dci = ((Template *)object)->getDCObjectById(dwItemId);
				if (dci != NULL)
				{
               dci->setLastPollTime(0);
					msg.setField(VID_RCC, RCC_SUCCESS);
					debugPrintf(4, _T("ForceDCIPoll: DCI %d at node %d"), dwItemId, object->getId());
				}
				else
				{
					msg.setField(VID_RCC, RCC_INVALID_DCI_ID);
					debugPrintf(4, _T("ForceDCIPoll: DCI %d at node %d not found"), dwItemId, object->getId());
				}
         }
         else  // User doesn't have READ rights on object
         {
            msg.setField(VID_RCC, RCC_ACCESS_DENIED);
         }
      }
      else     // Object is not a node
      {
         msg.setField(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
      }
   }
   else  // No object with given ID
   {
      msg.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Copy or move DCI from one node or template to another
 */
void ClientSession::copyDCI(NXCPMessage *pRequest)
{
   NXCPMessage msg;
   NetObj *pSource, *pDestination;
   TCHAR szLockInfo[MAX_SESSION_NAME];
   BOOL bMove;

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(pRequest->getId());

   // Get source and destination
   pSource = FindObjectById(pRequest->getFieldAsUInt32(VID_SOURCE_OBJECT_ID));
   pDestination = FindObjectById(pRequest->getFieldAsUInt32(VID_DESTINATION_OBJECT_ID));
   if ((pSource != NULL) && (pDestination != NULL))
   {
      // Check object types
      if (((pSource->getObjectClass() == OBJECT_NODE) || (pSource->getObjectClass() == OBJECT_MOBILEDEVICE) || (pSource->getObjectClass() == OBJECT_TEMPLATE) || (pSource->getObjectClass() == OBJECT_CLUSTER)) &&
		    ((pDestination->getObjectClass() == OBJECT_NODE) || (pDestination->getObjectClass() == OBJECT_MOBILEDEVICE) || (pDestination->getObjectClass() == OBJECT_TEMPLATE) || (pDestination->getObjectClass() == OBJECT_CLUSTER)))
      {
         if (((Template *)pSource)->isLockedBySession(m_id))
         {
            bMove = pRequest->getFieldAsUInt16(VID_MOVE_FLAG);
            // Check access rights
            if ((pSource->checkAccessRights(m_dwUserId, bMove ? (OBJECT_ACCESS_READ | OBJECT_ACCESS_MODIFY) : OBJECT_ACCESS_READ)) &&
                (pDestination->checkAccessRights(m_dwUserId, OBJECT_ACCESS_MODIFY)))
            {
               // Attempt to lock destination's DCI list
               if ((pDestination->getId() == pSource->getId()) ||
                   (((Template *)pDestination)->lockDCIList(m_id, m_sessionName, szLockInfo)))
               {
                  UINT32 i, *pdwItemList, dwNumItems;
                  const DCObject *pSrcItem;
                  DCObject *pDstItem;
                  int iErrors = 0;

                  // Get list of items to be copied/moved
                  dwNumItems = pRequest->getFieldAsUInt32(VID_NUM_ITEMS);
                  pdwItemList = (UINT32 *)malloc(sizeof(UINT32) * dwNumItems);
                  pRequest->getFieldAsInt32Array(VID_ITEM_LIST, dwNumItems, pdwItemList);

                  // Copy items
                  for(i = 0; i < dwNumItems; i++)
                  {
                     pSrcItem = ((Template *)pSource)->getDCObjectById(pdwItemList[i]);
                     if (pSrcItem != NULL)
                     {
								switch(pSrcItem->getType())
								{
									case DCO_TYPE_ITEM:
		                        pDstItem = new DCItem((DCItem *)pSrcItem);
										break;
									case DCO_TYPE_TABLE:
		                        pDstItem = new DCTable((DCTable *)pSrcItem);
										break;
									default:
										pDstItem = NULL;
										break;
								}
								if (pDstItem != NULL)
								{
									pDstItem->setTemplateId(0, 0);
									pDstItem->changeBinding(CreateUniqueId(IDG_ITEM),
																	(Template *)pDestination, FALSE);
									if (((Template *)pDestination)->addDCObject(pDstItem))
									{
										if (bMove)
										{
											// Delete original item
											if (!((Template *)pSource)->deleteDCObject(pdwItemList[i], TRUE))
											{
												iErrors++;
											}
										}
									}
									else
									{
										delete pDstItem;
										iErrors++;
									}
								}
								else
								{
									DbgPrintf(2, _T("INTERNAL ERROR: ClientSession::CopyDCI(): unknown DCO type %d"), pSrcItem->getType());
									iErrors++;
								}
                     }
                     else
                     {
                        iErrors++;
                     }
                  }

                  // Cleanup
                  free(pdwItemList);
                  if (pDestination->getId() != pSource->getId())
                     ((Template *)pDestination)->unlockDCIList(m_id);
                  msg.setField(VID_RCC, (iErrors == 0) ? RCC_SUCCESS : RCC_DCI_COPY_ERRORS);

                  // Queue template update
                  if (pDestination->getObjectClass() == OBJECT_TEMPLATE)
                     ((Template *)pDestination)->queueUpdate();
               }
               else  // Destination's DCI list already locked by someone else
               {
                  msg.setField(VID_RCC, RCC_COMPONENT_LOCKED);
                  msg.setField(VID_LOCKED_BY, szLockInfo);
               }
            }
            else  // User doesn't have enough rights on object(s)
            {
               msg.setField(VID_RCC, RCC_ACCESS_DENIED);
            }
         }
         else  // Source node DCI list not locked by this session
         {
            msg.setField(VID_RCC, RCC_OUT_OF_STATE_REQUEST);
         }
      }
      else     // Object(s) is not a node
      {
         msg.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
      }
   }
   else  // No object(s) with given ID
   {
      msg.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Send list of thresholds for DCI
 */
void ClientSession::sendDCIThresholds(NXCPMessage *request)
{
   NXCPMessage msg;
   NetObj *object;

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(request->getId());

   // Get node id and check object class and access rights
   object = FindObjectById(request->getFieldAsUInt32(VID_OBJECT_ID));
   if (object != NULL)
   {
      if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
      {
			if ((object->getObjectClass() == OBJECT_NODE) || (object->getObjectClass() == OBJECT_MOBILEDEVICE) || (object->getObjectClass() == OBJECT_CLUSTER))
			{
				DCObject *dci = ((Template *)object)->getDCObjectById(request->getFieldAsUInt32(VID_DCI_ID));
				if ((dci != NULL) && (dci->getType() == DCO_TYPE_ITEM))
				{
					((DCItem *)dci)->fillMessageWithThresholds(&msg);
					msg.setField(VID_RCC, RCC_SUCCESS);
				}
				else
				{
					msg.setField(VID_RCC, RCC_INVALID_DCI_ID);
				}
         }
         else
         {
            msg.setField(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
         }
      }
      else
      {
         msg.setField(VID_RCC, RCC_ACCESS_DENIED);
      }
   }
   else  // No object with given ID
   {
		msg.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Prepare statement for reading data from idata table
 */
static DB_STATEMENT PrepareIDataSelect(DB_HANDLE hdb, UINT32 nodeId, UINT32 maxRows, const TCHAR *condition)
{
	TCHAR query[512];

	switch(g_dbSyntax)
	{
		case DB_SYNTAX_MSSQL:
			_sntprintf(query, 512, _T("SELECT TOP %d idata_timestamp,idata_value FROM idata_%d WHERE item_id=?%s ORDER BY idata_timestamp DESC"),
			           (int)maxRows, (int)nodeId, condition);
			break;
		case DB_SYNTAX_ORACLE:
			_sntprintf(query, 512, _T("SELECT * FROM (SELECT idata_timestamp,idata_value FROM idata_%d WHERE item_id=?%s ORDER BY idata_timestamp DESC) WHERE ROWNUM<=%d"),
						  (int)nodeId, condition, (int)maxRows);
			break;
		case DB_SYNTAX_MYSQL:
		case DB_SYNTAX_PGSQL:
		case DB_SYNTAX_SQLITE:
			_sntprintf(query, 512, _T("SELECT idata_timestamp,idata_value FROM idata_%d WHERE item_id=?%s ORDER BY idata_timestamp DESC LIMIT %d"),
						  (int)nodeId, condition, (int)maxRows);
			break;
		case DB_SYNTAX_DB2:
		   _sntprintf(query, 512, _T("SELECT idata_timestamp,idata_value FROM idata_%d WHERE item_id=?%s ORDER BY idata_timestamp DESC FETCH FIRST %d ROWS ONLY"),
		      (int)nodeId, condition, (int)maxRows);
		   break;
		default:
			DbgPrintf(1, _T(">>> INTERNAL ERROR: unsupported database in PrepareIDataSelect"));
			return NULL;	// Unsupported database
	}
	return DBPrepare(hdb, query);
}

/**
 * Prepare statement for reading data from tdata table
 */
static DB_STATEMENT PrepareTDataSelect(DB_HANDLE hdb, UINT32 nodeId, UINT32 maxRows, const TCHAR *condition)
{
	TCHAR query[1024];

	switch(g_dbSyntax)
	{
		case DB_SYNTAX_MSSQL:
			_sntprintf(query, 1024,
                    _T("SELECT TOP %d d.tdata_timestamp, r.value FROM tdata_%d d")
                    _T("   INNER JOIN tdata_records_%d rec ON rec.record_id=d.record_id ")
                    _T("   INNER JOIN tdata_rows_%d r ON r.row_id=rec.row_id ")
                    _T("WHERE d.item_id=? AND rec.instance=? AND r.column_id=? %s ")
                    _T("ORDER BY d.tdata_timestamp DESC"),
			           (int)maxRows, (int)nodeId, (int)nodeId, (int)nodeId, condition);
			break;
		case DB_SYNTAX_ORACLE:
			_sntprintf(query, 1024,
                    _T("SELECT * FROM (")
                    _T("   SELECT d.tdata_timestamp, r.value FROM tdata_%d d")
                    _T("      INNER JOIN tdata_records_%d rec ON rec.record_id=d.record_id")
                    _T("      INNER JOIN tdata_rows_%d r ON r.row_id=rec.row_id")
                    _T("   WHERE d.item_id=? AND rec.instance=? AND r.column_id=? %s")
                    _T("   ORDER BY d.tdata_timestamp DESC)")
                    _T("WHERE ROWNUM<=%d"),
			           (int)nodeId, (int)nodeId, (int)nodeId, condition, (int)maxRows);
			break;
		case DB_SYNTAX_MYSQL:
		case DB_SYNTAX_PGSQL:
		case DB_SYNTAX_SQLITE:
			_sntprintf(query, 1024,
                    _T("SELECT d.tdata_timestamp, r.value FROM tdata_%d d")
                    _T("   INNER JOIN tdata_records_%d rec ON rec.record_id=d.record_id ")
                    _T("   INNER JOIN tdata_rows_%d r ON r.row_id=rec.row_id ")
                    _T("WHERE d.item_id=? AND rec.instance=? AND r.column_id=? %s ")
                    _T("ORDER BY d.tdata_timestamp DESC LIMIT %d"),
			           (int)nodeId, (int)nodeId, (int)nodeId, condition, (int)maxRows);
			break;
		case DB_SYNTAX_DB2:
			_sntprintf(query, 1024,
                    _T("SELECT d.tdata_timestamp, r.value FROM tdata_%d d")
                    _T("   INNER JOIN tdata_records_%d rec ON rec.record_id=d.record_id ")
                    _T("   INNER JOIN tdata_rows_%d r ON r.row_id=rec.row_id ")
                    _T("WHERE d.item_id=? AND rec.instance=? AND r.column_id=? %s ")
                    _T("ORDER BY d.tdata_timestamp DESC FETCH FIRST %d ROWS ONLY"),
			           (int)nodeId, (int)nodeId, (int)nodeId, condition, (int)maxRows);
			break;
		default:
			DbgPrintf(1, _T(">>> INTERNAL ERROR: unsupported database in PrepareTDataSelect"));
			return NULL;	// Unsupported database
	}
	return DBPrepare(hdb, query);
}

/**
 * Get collected data for table or simple DCI
 */
bool ClientSession::getCollectedDataFromDB(NXCPMessage *request, NXCPMessage *response, DataCollectionTarget *dcTarget, int dciType)
{
	// Find DCI object
	DCObject *dci = dcTarget->getDCObjectById(request->getFieldAsUInt32(VID_DCI_ID));
	if (dci == NULL)
	{
		response->setField(VID_RCC, RCC_INVALID_DCI_ID);
		return false;
	}

	// DCI type in request should match actual DCI type
	if (dci->getType() != dciType)
	{
		response->setField(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
		return false;
	}

	// Check that all required data present in message
	if ((dciType == DCO_TYPE_TABLE) && (!request->isFieldExist(VID_DATA_COLUMN) || !request->isFieldExist(VID_INSTANCE)))
	{
		response->setField(VID_RCC, RCC_INVALID_ARGUMENT);
		return false;
	}

	// Get request parameters
	UINT32 maxRows = request->getFieldAsUInt32(VID_MAX_ROWS);
	UINT32 timeFrom = request->getFieldAsUInt32(VID_TIME_FROM);
	UINT32 timeTo = request->getFieldAsUInt32(VID_TIME_TO);

	if ((maxRows == 0) || (maxRows > MAX_DCI_DATA_RECORDS))
		maxRows = MAX_DCI_DATA_RECORDS;

	TCHAR condition[256] = _T("");
	if (timeFrom != 0)
		_tcscpy(condition, (dciType == DCO_TYPE_TABLE) ? _T(" AND d.tdata_timestamp>=?") : _T(" AND idata_timestamp>=?"));
	if (timeTo != 0)
		_tcscat(condition, (dciType == DCO_TYPE_TABLE) ? _T(" AND d.tdata_timestamp<=?") : _T(" AND idata_timestamp<=?"));

	DCI_DATA_HEADER *pData = NULL;
	DCI_DATA_ROW *pCurr;

	bool success = false;
	DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
	DB_STATEMENT hStmt;
	switch(dciType)
	{
		case DCO_TYPE_ITEM:
			hStmt = PrepareIDataSelect(hdb, dcTarget->getId(), maxRows, condition);
			break;
		case DCO_TYPE_TABLE:
			hStmt = PrepareTDataSelect(hdb, dcTarget->getId(), maxRows, condition);
			break;
		default:
			hStmt = NULL;
			break;
	}

	if (hStmt != NULL)
	{
		TCHAR dataColumn[MAX_COLUMN_NAME] = _T("");

		int pos = 1;
		DBBind(hStmt, pos++, DB_SQLTYPE_INTEGER, dci->getId());
		if (dciType == DCO_TYPE_TABLE)
		{
			request->getFieldAsString(VID_DATA_COLUMN, dataColumn, MAX_COLUMN_NAME);

			DBBind(hStmt, pos++, DB_SQLTYPE_VARCHAR, request->getFieldAsString(VID_INSTANCE), DB_BIND_DYNAMIC);
			DBBind(hStmt, pos++, DB_SQLTYPE_INTEGER, DCTable::columnIdFromName(dataColumn));
		}
		if (timeFrom != 0)
			DBBind(hStmt, pos++, DB_SQLTYPE_INTEGER, timeFrom);
		if (timeTo != 0)
			DBBind(hStmt, pos++, DB_SQLTYPE_INTEGER, timeTo);

		DB_RESULT hResult = DBSelectPrepared(hStmt);
		if (hResult != NULL)
		{
		   static UINT32 m_dwRowSize[] = { 8, 8, 16, 16, 516, 16 };
#if !defined(UNICODE) || defined(UNICODE_UCS4)
			TCHAR szBuffer[MAX_DCI_STRING_VALUE];
#endif

			// Send CMD_REQUEST_COMPLETED message
			response->setField(VID_RCC, RCC_SUCCESS);
			if (dciType == DCO_TYPE_ITEM)
				((DCItem *)dci)->fillMessageWithThresholds(response);
			sendMessage(response);

			UINT32 numRows = (UINT32)DBGetNumRows(hResult);

			int dataType;
			switch(dciType)
			{
				case DCO_TYPE_ITEM:
					dataType = ((DCItem *)dci)->getDataType();
					break;
				case DCO_TYPE_TABLE:
					dataType = ((DCTable *)dci)->getColumnDataType(dataColumn);
					break;
				default:
					dataType = DCI_DT_STRING;
					break;
			}

			// Allocate memory for data and prepare data header
			pData = (DCI_DATA_HEADER *)malloc(numRows * m_dwRowSize[dataType] + sizeof(DCI_DATA_HEADER));
			pData->dataType = htonl((UINT32)dataType);
			pData->dciId = htonl(dci->getId());

			// Fill memory block with records
			pCurr = (DCI_DATA_ROW *)(((char *)pData) + sizeof(DCI_DATA_HEADER));
			for(UINT32 i = 0; i < numRows; i++)
			{
				pCurr->timeStamp = htonl(DBGetFieldULong(hResult, i, 0));
				switch(dataType)
				{
					case DCI_DT_INT:
					case DCI_DT_UINT:
						pCurr->value.int32 = htonl(DBGetFieldULong(hResult, i, 1));
						break;
					case DCI_DT_INT64:
					case DCI_DT_UINT64:
						pCurr->value.ext.v64.int64 = htonq(DBGetFieldUInt64(hResult, i, 1));
						break;
					case DCI_DT_FLOAT:
						pCurr->value.ext.v64.real = htond(DBGetFieldDouble(hResult, i, 1));
						break;
					case DCI_DT_STRING:
#ifdef UNICODE
#ifdef UNICODE_UCS4
						DBGetField(hResult, i, 1, szBuffer, MAX_DCI_STRING_VALUE);
						ucs4_to_ucs2(szBuffer, -1, pCurr->value.string, MAX_DCI_STRING_VALUE);
#else
						DBGetField(hResult, i, 1, pCurr->value.string, MAX_DCI_STRING_VALUE);
#endif
#else
						DBGetField(hResult, i, 1, szBuffer, MAX_DCI_STRING_VALUE);
						mb_to_ucs2(szBuffer, -1, pCurr->value.string, MAX_DCI_STRING_VALUE);
#endif
						SwapWideString(pCurr->value.string);
						break;
				}
				pCurr = (DCI_DATA_ROW *)(((char *)pCurr) + m_dwRowSize[dataType]);
			}
			DBFreeResult(hResult);
			pData->numRows = htonl(numRows);

			// Prepare and send raw message with fetched data
			NXCP_MESSAGE *msg =
				CreateRawNXCPMessage(CMD_DCI_DATA, request->getId(), 0,
											numRows * m_dwRowSize[dataType] + sizeof(DCI_DATA_HEADER),
											pData, NULL);
			free(pData);
			sendRawMessage(msg);
			free(msg);
			success = true;
		}
		else
		{
			response->setField(VID_RCC, RCC_DB_FAILURE);
		}
		DBFreeStatement(hStmt);
	}
	else
	{
		response->setField(VID_RCC, RCC_DB_FAILURE);
	}
	DBConnectionPoolReleaseConnection(hdb);
	return success;
}

/**
 * Get collected data
 */
void ClientSession::getCollectedData(NXCPMessage *request)
{
   NXCPMessage msg;
	bool success = false;

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(request->getId());

   NetObj *object = FindObjectById(request->getFieldAsUInt32(VID_OBJECT_ID));
   if (object != NULL)
   {
		if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
		{
			if ((object->getObjectClass() == OBJECT_NODE) || (object->getObjectClass() == OBJECT_MOBILEDEVICE) || (object->getObjectClass() == OBJECT_CLUSTER))
			{
				if (!(g_flags & AF_DB_CONNECTION_LOST))
				{
					success = getCollectedDataFromDB(request, &msg, (DataCollectionTarget *)object, DCO_TYPE_ITEM);
				}
				else
				{
					msg.setField(VID_RCC, RCC_DB_CONNECTION_LOST);
				}
			}
			else
			{
				msg.setField(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
			}
		}
		else
		{
			msg.setField(VID_RCC, RCC_ACCESS_DENIED);
		}
   }
   else  // No object with given ID
   {
      msg.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   // Send response
   if (!success)
      sendMessage(&msg);
}

/**
 * Get collected data for table DCI
 */
void ClientSession::getTableCollectedData(NXCPMessage *request)
{
   NXCPMessage msg;
	bool success = false;

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(request->getId());

   NetObj *object = FindObjectById(request->getFieldAsUInt32(VID_OBJECT_ID));
   if (object != NULL)
   {
		if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
		{
			if ((object->getObjectClass() == OBJECT_NODE) || (object->getObjectClass() == OBJECT_MOBILEDEVICE) || (object->getObjectClass() == OBJECT_CLUSTER))
			{
				if (!(g_flags & AF_DB_CONNECTION_LOST))
				{
					success = getCollectedDataFromDB(request, &msg, (DataCollectionTarget *)object, DCO_TYPE_TABLE);
				}
				else
				{
					msg.setField(VID_RCC, RCC_DB_CONNECTION_LOST);
				}
			}
			else
			{
				msg.setField(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
			}
		}
		else
		{
			msg.setField(VID_RCC, RCC_ACCESS_DENIED);
		}
   }
   else
   {
      msg.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

	if (!success)
      sendMessage(&msg);
}

/**
 * Send latest collected values for all DCIs of given node
 */
void ClientSession::getLastValues(NXCPMessage *pRequest)
{
   NXCPMessage msg;
   NetObj *object;

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(pRequest->getId());

   // Get node id and check object class and access rights
   object = FindObjectById(pRequest->getFieldAsUInt32(VID_OBJECT_ID));
   if (object != NULL)
   {
      if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
      {
         if ((object->getObjectClass() == OBJECT_NODE) || (object->getObjectClass() == OBJECT_MOBILEDEVICE) ||
             (object->getObjectClass() == OBJECT_TEMPLATE) || (object->getObjectClass() == OBJECT_CLUSTER))
         {
            msg.setField(VID_RCC,
               ((Template *)object)->getLastValues(&msg,
                  pRequest->getFieldAsBoolean(VID_OBJECT_TOOLTIP_ONLY),
                  pRequest->getFieldAsBoolean(VID_OVERVIEW_ONLY),
                  pRequest->getFieldAsBoolean(VID_INCLUDE_NOVALUE_OBJECTS)));
         }
         else
         {
            msg.setField(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
         }
      }
      else
      {
         msg.setField(VID_RCC, RCC_ACCESS_DENIED);
      }
   }
   else  // No object with given ID
   {
      msg.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Send latest collected values for all DCIs from given list.
 * Error message will never be returned. Will be returned only
 * possible DCI values.
 */
void ClientSession::getLastValuesByDciId(NXCPMessage *pRequest)
{
   NXCPMessage msg;
   NetObj *object;
   DCObject *dcoObj;

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(pRequest->getId());
   int size = pRequest->getFieldAsInt32(VID_NUM_ITEMS);
   UINT32 incomingIndex = VID_DCI_VALUES_BASE;
   UINT32 outgoingIndex = VID_DCI_VALUES_BASE;

   for(int i = 0; i < size; i++, incomingIndex += 10)
   {
      TCHAR *value;
      UINT32 type, status;

      object = FindObjectById(pRequest->getFieldAsUInt32(incomingIndex));
      if (object != NULL)
      {
         if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
         {
            if ((object->getObjectClass() == OBJECT_NODE) || (object->getObjectClass() == OBJECT_MOBILEDEVICE) ||
                (object->getObjectClass() == OBJECT_TEMPLATE) || (object->getObjectClass() == OBJECT_CLUSTER))
            {
               UINT32 dciID = pRequest->getFieldAsUInt32(incomingIndex+1);
               dcoObj = ((DataCollectionTarget *)object)->getDCObjectById(dciID);
               if(dcoObj == NULL)
                  continue;

               if (dcoObj->getType() == DCO_TYPE_TABLE)
               {
                  TCHAR * column = pRequest->getFieldAsString(incomingIndex+2);
                  TCHAR * instance = pRequest->getFieldAsString(incomingIndex+3);
                  if(column == NULL || instance == NULL || _tcscmp(column, _T("")) == 0 || _tcscmp(instance, _T("")) == 0)
                  {
                     continue;
                  }

                  Table *t = ((DCTable *)dcoObj)->getLastValue();
                  int columnIndex =  t->getColumnIndex(column);
                  int rowIndex = t->findRowByInstance(instance);
                  type = t->getColumnDataType(columnIndex);
                  value = _tcsdup_ex(t->getAsString(rowIndex, columnIndex));
                  t->decRefCount();

                  safe_free(column);
                  safe_free(instance);
               }
               else
               {
                  if (dcoObj->getType() == DCO_TYPE_ITEM)
                  {
                     type = (WORD)((DCItem *)dcoObj)->getDataType();
                     value = _tcsdup_ex(((DCItem *)dcoObj)->getLastValue());
                  }
                  else
                     continue;
               }


               status = dcoObj->getStatus();

               msg.setField(outgoingIndex++, dciID);
               msg.setField(outgoingIndex++, CHECK_NULL_EX(value));
               msg.setField(outgoingIndex++, type);
               msg.setField(outgoingIndex++, status);
               safe_free(value);
               outgoingIndex += 6;
            }
         }
      }
   }
   // Set result
   msg.setField(VID_NUM_ITEMS, (outgoingIndex - VID_DCI_VALUES_BASE) / 10);
   msg.setField(VID_RCC, RCC_SUCCESS);
   // Send response
   sendMessage(&msg);
}

/**
 * Send latest collected values for given table DCI of given node
 */
void ClientSession::getTableLastValues(NXCPMessage *pRequest)
{
   NXCPMessage msg;
   NetObj *object;

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(pRequest->getId());

   // Get node id and check object class and access rights
   object = FindObjectById(pRequest->getFieldAsUInt32(VID_OBJECT_ID));
   if (object != NULL)
   {
      if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
      {
         if ((object->getObjectClass() == OBJECT_NODE) || (object->getObjectClass() == OBJECT_MOBILEDEVICE) || (object->getObjectClass() == OBJECT_CLUSTER))
         {
				msg.setField(VID_RCC, ((DataCollectionTarget *)object)->getTableLastValues(pRequest->getFieldAsUInt32(VID_DCI_ID), &msg));
         }
         else
         {
            msg.setField(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
         }
      }
      else
      {
         msg.setField(VID_RCC, RCC_ACCESS_DENIED);
      }
   }
   else  // No object with given ID
   {
      msg.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Open event processing policy
 */
void ClientSession::openEPP(NXCPMessage *request)
{
   NXCPMessage msg;
   bool success = false;

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(request->getId());

   bool readOnly = request->getFieldAsUInt16(VID_READ_ONLY) ? true : false;

   if (checkSysAccessRights(SYSTEM_ACCESS_EPP))
   {
      TCHAR buffer[256];
      if (!readOnly && !LockComponent(CID_EPP, m_id, m_sessionName, NULL, buffer))
      {
         msg.setField(VID_RCC, RCC_COMPONENT_LOCKED);
         msg.setField(VID_LOCKED_BY, buffer);
      }
      else
      {
         if (!readOnly)
         {
            m_dwFlags |= CSF_EPP_LOCKED;
         }
         msg.setField(VID_RCC, RCC_SUCCESS);
         msg.setField(VID_NUM_RULES, g_pEventPolicy->getNumRules());
         success = true;
      }
   }
   else
   {
      // Current user has no rights for event policy management
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);
   }

   // Send response
   sendMessage(&msg);

   // Send policy to client
   if (success)
      g_pEventPolicy->sendToClient(this, request->getId());
}

/**
 * Close event processing policy
 */
void ClientSession::closeEPP(UINT32 dwRqId)
{
   NXCPMessage msg;

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(dwRqId);

   if (m_dwSystemAccess & SYSTEM_ACCESS_EPP)
   {
      if (m_dwFlags & CSF_EPP_LOCKED)
      {
         UnlockComponent(CID_EPP);
         m_dwFlags &= ~CSF_EPP_LOCKED;
      }
      msg.setField(VID_RCC, RCC_SUCCESS);
   }
   else
   {
      // Current user has no rights for event policy management
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);
   }

   // Send response
   sendMessage(&msg);
}


/**
 * Save event processing policy
 */
void ClientSession::saveEPP(NXCPMessage *pRequest)
{
   NXCPMessage msg;

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(pRequest->getId());

   if (m_dwSystemAccess & SYSTEM_ACCESS_EPP)
   {
      if (m_dwFlags & CSF_EPP_LOCKED)
      {
         msg.setField(VID_RCC, RCC_SUCCESS);
         m_dwNumRecordsToUpload = pRequest->getFieldAsUInt32(VID_NUM_RULES);
         m_dwRecordsUploaded = 0;
         if (m_dwNumRecordsToUpload == 0)
         {
            g_pEventPolicy->replacePolicy(0, NULL);
            g_pEventPolicy->saveToDB();
         }
         else
         {
            m_dwFlags |= CSF_EPP_UPLOAD;
            m_ppEPPRuleList = (EPRule **)malloc(sizeof(EPRule *) * m_dwNumRecordsToUpload);
            memset(m_ppEPPRuleList, 0, sizeof(EPRule *) * m_dwNumRecordsToUpload);
         }
         debugPrintf(5, _T("Accepted EPP upload request for %d rules"), m_dwNumRecordsToUpload);
      }
      else
      {
         msg.setField(VID_RCC, RCC_OUT_OF_STATE_REQUEST);
      }
   }
   else
   {
      // Current user has no rights for event policy management
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);
   }

   // Send response
   sendMessage(&msg);
}


/**
 * Process EPP rule received from client
 */
void ClientSession::processEPPRecord(NXCPMessage *pRequest)
{
   if (!(m_dwFlags & CSF_EPP_LOCKED))
   {
      NXCPMessage msg;

      msg.setCode(CMD_REQUEST_COMPLETED);
      msg.setId(pRequest->getId());
      msg.setField(VID_RCC, RCC_OUT_OF_STATE_REQUEST);
      sendMessage(&msg);
   }
   else
   {
      if (m_dwRecordsUploaded < m_dwNumRecordsToUpload)
      {
         m_ppEPPRuleList[m_dwRecordsUploaded] = new EPRule(pRequest);
         m_dwRecordsUploaded++;
         if (m_dwRecordsUploaded == m_dwNumRecordsToUpload)
         {
            NXCPMessage msg;

            // All records received, replace event policy...
            debugPrintf(5, _T("Replacing event processing policy with a new one at %p (%d rules)"),
                        m_ppEPPRuleList, m_dwNumRecordsToUpload);
            g_pEventPolicy->replacePolicy(m_dwNumRecordsToUpload, m_ppEPPRuleList);
            g_pEventPolicy->saveToDB();
            m_ppEPPRuleList = NULL;

            // ... and send final confirmation
            msg.setCode(CMD_REQUEST_COMPLETED);
            msg.setId(pRequest->getId());
            msg.setField(VID_RCC, RCC_SUCCESS);
            sendMessage(&msg);

            m_dwFlags &= ~CSF_EPP_UPLOAD;
         }
      }
   }
}

/**
 * Send compiled MIB file to client
 */
void ClientSession::sendMib(NXCPMessage *request)
{
   TCHAR mibFile[MAX_PATH];

   _tcscpy(mibFile, g_netxmsdDataDir);
   _tcscat(mibFile, DFILE_COMPILED_MIB);
	sendFile(mibFile, request->getId(), 0);
}

/**
 * Send timestamp of compiled MIB file to client
 */
void ClientSession::sendMIBTimestamp(UINT32 dwRqId)
{
   NXCPMessage msg;
   TCHAR szBuffer[MAX_PATH];
   UINT32 dwResult, dwTimeStamp;

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(dwRqId);

   _tcscpy(szBuffer, g_netxmsdDataDir);
   _tcscat(szBuffer, DFILE_COMPILED_MIB);
   dwResult = SNMPGetMIBTreeTimestamp(szBuffer, &dwTimeStamp);
   if (dwResult == SNMP_ERR_SUCCESS)
   {
      msg.setField(VID_RCC, RCC_SUCCESS);
      msg.setField(VID_TIMESTAMP, dwTimeStamp);
		msg.setField(VID_FILE_SIZE, FileSize(szBuffer));
   }
   else
   {
      switch(dwResult)
      {
         case SNMP_ERR_FILE_IO:
            msg.setField(VID_RCC, RCC_FILE_IO_ERROR);
            break;
         case SNMP_ERR_BAD_FILE_HEADER:
            msg.setField(VID_RCC, RCC_CORRUPTED_MIB_FILE);
            break;
         default:
            msg.setField(VID_RCC, RCC_INTERNAL_ERROR);
            break;
      }
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Create new object
 */
void ClientSession::createObject(NXCPMessage *pRequest)
{
   NXCPMessage msg;
   NetObj *object = NULL, *pParent;
   int iClass, iServiceType;
   TCHAR szObjectName[MAX_OBJECT_NAME], nodePrimaryName[MAX_DNS_NAME], deviceId[MAX_OBJECT_NAME];
   TCHAR *pszRequest, *pszResponse, *pszComments;
   UINT32 zoneId, nodeId;
   WORD wIpProto, wIpPort;
   InetAddress ipAddr;
   BOOL bParentAlwaysValid = FALSE;

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(pRequest->getId());

   iClass = pRequest->getFieldAsUInt16(VID_OBJECT_CLASS);
	zoneId = pRequest->getFieldAsUInt32(VID_ZONE_ID);

   // Find parent object
   pParent = FindObjectById(pRequest->getFieldAsUInt32(VID_PARENT_ID));
   if (iClass == OBJECT_NODE)
   {
		if (pRequest->isFieldExist(VID_PRIMARY_NAME))
		{
			pRequest->getFieldAsString(VID_PRIMARY_NAME, nodePrimaryName, MAX_DNS_NAME);
         ipAddr = InetAddress::resolveHostName(nodePrimaryName);
		}
		else
		{
         ipAddr = pRequest->getFieldAsInetAddress(VID_IP_ADDRESS);
         ipAddr.toString(nodePrimaryName);
		}
      if ((pParent == NULL) && ipAddr.isValidUnicast())
      {
         pParent = FindSubnetForNode(zoneId, ipAddr);
         bParentAlwaysValid = TRUE;
      }
   }
   if ((pParent != NULL) || (iClass == OBJECT_NODE))
   {
      // User should have create access to parent object
      if ((pParent != NULL) ?
            pParent->checkAccessRights(m_dwUserId, OBJECT_ACCESS_CREATE) :
            g_pEntireNet->checkAccessRights(m_dwUserId, OBJECT_ACCESS_CREATE))
      {
         // Parent object should be of valid type
         if (bParentAlwaysValid || IsValidParentClass(iClass, (pParent != NULL) ? pParent->getObjectClass() : -1))
         {
				// Check zone
				bool zoneIsValid;
				if (IsZoningEnabled() && (zoneId != 0) && (iClass != OBJECT_ZONE))
				{
					zoneIsValid = (g_idxZoneByGUID.get(zoneId) != NULL);
				}
				else
				{
					zoneIsValid = true;
				}
				if (zoneIsValid)
				{
					pRequest->getFieldAsString(VID_OBJECT_NAME, szObjectName, MAX_OBJECT_NAME);
					if (IsValidObjectName(szObjectName, TRUE))
					{
                  // Do additional validation by modules
                  UINT32 moduleRCC = RCC_SUCCESS;
                  for(UINT32 i = 0; i < g_dwNumModules; i++)
	               {
		               if (g_pModuleList[i].pfValidateObjectCreation != NULL)
		               {
                        moduleRCC = g_pModuleList[i].pfValidateObjectCreation(iClass, szObjectName, ipAddr, zoneId, pRequest);
			               if (moduleRCC != RCC_SUCCESS)
                        {
                           DbgPrintf(4, _T("Creation of object \"%s\" of class %d blocked by module %s (RCC=%d)"), szObjectName, iClass, g_pModuleList[i].szName, moduleRCC);
                           break;
                        }
		               }
	               }

                  if (moduleRCC == RCC_SUCCESS)
                  {
                     ObjectTransactionStart();

						   // Create new object
						   switch(iClass)
						   {
							   case OBJECT_NODE:
                           ipAddr.setMaskBits(pRequest->getFieldAsInt32(VID_IP_NETMASK));
								   object = PollNewNode(ipAddr,
															   pRequest->getFieldAsUInt32(VID_CREATION_FLAGS),
															   pRequest->getFieldAsUInt16(VID_AGENT_PORT),
															   pRequest->getFieldAsUInt16(VID_SNMP_PORT),
															   szObjectName,
															   pRequest->getFieldAsUInt32(VID_AGENT_PROXY),
															   pRequest->getFieldAsUInt32(VID_SNMP_PROXY),
															   (pParent != NULL) ? ((pParent->getObjectClass() == OBJECT_CLUSTER) ? (Cluster *)pParent : NULL) : NULL,
															   zoneId, false, false);
								   if (object != NULL)
								   {
									   ((Node *)object)->setPrimaryName(nodePrimaryName);
								   }
								   break;
							   case OBJECT_MOBILEDEVICE:
								   pRequest->getFieldAsString(VID_DEVICE_ID, deviceId, MAX_OBJECT_NAME);
								   object = new MobileDevice(szObjectName, deviceId);
								   NetObjInsert(object, TRUE);
								   break;
							   case OBJECT_CONTAINER:
								   object = new Container(szObjectName, pRequest->getFieldAsUInt32(VID_CATEGORY));
								   NetObjInsert(object, TRUE);
								   object->calculateCompoundStatus();	// Force status change to NORMAL
								   break;
							   case OBJECT_RACK:
								   object = new Rack(szObjectName, (int)pRequest->getFieldAsUInt16(VID_HEIGHT));
								   NetObjInsert(object, TRUE);
								   break;
							   case OBJECT_TEMPLATEGROUP:
								   object = new TemplateGroup(szObjectName);
								   NetObjInsert(object, TRUE);
								   object->calculateCompoundStatus();	// Force status change to NORMAL
								   break;
							   case OBJECT_TEMPLATE:
								   object = new Template(szObjectName);
								   NetObjInsert(object, TRUE);
								   object->calculateCompoundStatus();	// Force status change to NORMAL
								   break;
							   case OBJECT_POLICYGROUP:
								   object = new PolicyGroup(szObjectName);
								   NetObjInsert(object, TRUE);
								   object->calculateCompoundStatus();	// Force status change to NORMAL
								   break;
							   case OBJECT_AGENTPOLICY_CONFIG:
								   object = new AgentPolicyConfig(szObjectName);
								   NetObjInsert(object, TRUE);
								   object->calculateCompoundStatus();	// Force status change to NORMAL
								   break;
							   case OBJECT_CLUSTER:
								   object = new Cluster(szObjectName, zoneId);
								   NetObjInsert(object, TRUE);
								   break;
							   case OBJECT_NETWORKSERVICE:
								   iServiceType = (int)pRequest->getFieldAsUInt16(VID_SERVICE_TYPE);
								   wIpProto = pRequest->getFieldAsUInt16(VID_IP_PROTO);
								   wIpPort = pRequest->getFieldAsUInt16(VID_IP_PORT);
								   pszRequest = pRequest->getFieldAsString(VID_SERVICE_REQUEST);
								   pszResponse = pRequest->getFieldAsString(VID_SERVICE_RESPONSE);
								   object = new NetworkService(iServiceType, wIpProto, wIpPort,
																	     pszRequest, pszResponse, (Node *)pParent);
								   object->setName(szObjectName);
								   NetObjInsert(object, TRUE);
								   break;
							   case OBJECT_VPNCONNECTOR:
								   object = new VPNConnector(TRUE);
								   object->setName(szObjectName);
								   NetObjInsert(object, TRUE);
								   break;
							   case OBJECT_CONDITION:
								   object = new Condition(TRUE);
								   object->setName(szObjectName);
								   NetObjInsert(object, TRUE);
								   break;
							   case OBJECT_NETWORKMAPGROUP:
								   object = new NetworkMapGroup(szObjectName);
								   NetObjInsert(object, TRUE);
								   object->calculateCompoundStatus();	// Force status change to NORMAL
								   break;
							   case OBJECT_NETWORKMAP:
								   object = new NetworkMap((int)pRequest->getFieldAsUInt16(VID_MAP_TYPE), pRequest->getFieldAsUInt32(VID_SEED_OBJECT));
								   object->setName(szObjectName);
								   NetObjInsert(object, TRUE);
								   break;
							   case OBJECT_DASHBOARD:
								   object = new Dashboard(szObjectName);
								   NetObjInsert(object, TRUE);
								   break;
							   case OBJECT_ZONE:
								   if ((zoneId > 0) && (g_idxZoneByGUID.get(zoneId) == NULL))
								   {
									   object = new Zone(zoneId, szObjectName);
									   NetObjInsert(object, TRUE);
								   }
								   else
								   {
									   object = NULL;
								   }
								   break;
							   case OBJECT_BUSINESSSERVICE:
								   object = new BusinessService(szObjectName);
								   NetObjInsert(object, TRUE);
								   break;
							   case OBJECT_NODELINK:
								   nodeId = pRequest->getFieldAsUInt32(VID_NODE_ID);
								   if (nodeId > 0)
								   {
									   object = new NodeLink(szObjectName, nodeId);
									   NetObjInsert(object, TRUE);
								   }
								   else
								   {
									   object = NULL;
								   }
								   break;
							   case OBJECT_SLMCHECK:
								   object = new SlmCheck(szObjectName, pRequest->getFieldAsUInt16(VID_IS_TEMPLATE) ? true : false);
								   NetObjInsert(object, TRUE);
								   break;
							   case OBJECT_INTERFACE:
                           {
                              InterfaceInfo ifInfo(pRequest->getFieldAsUInt32(VID_IF_INDEX));
                              nx_strncpy(ifInfo.name, szObjectName, MAX_DB_STRING);
                              InetAddress addr = pRequest->getFieldAsInetAddress(VID_IP_ADDRESS);
                              if (addr.isValidUnicast())
                                 ifInfo.ipAddrList.add(addr);
                              ifInfo.type = pRequest->getFieldAsUInt32(VID_IF_TYPE);
								      pRequest->getFieldAsBinary(VID_MAC_ADDR, ifInfo.macAddr, MAC_ADDR_LENGTH);
                              ifInfo.slot = pRequest->getFieldAsUInt32(VID_IF_SLOT);
                              ifInfo.port = pRequest->getFieldAsUInt32(VID_IF_PORT);
                              ifInfo.isPhysicalPort = pRequest->getFieldAsBoolean(VID_IS_PHYS_PORT);
								      object = ((Node *)pParent)->createNewInterface(&ifInfo, true);
                           }
								   break;
							   default:
								   // Try to create unknown classes by modules
								   for(UINT32 i = 0; i < g_dwNumModules; i++)
								   {
									   if (g_pModuleList[i].pfCreateObject != NULL)
									   {
										   object = g_pModuleList[i].pfCreateObject(iClass, szObjectName, pParent, pRequest);
										   if (object != NULL)
											   break;
									   }
								   }
								   break;
						   }

						   // If creation was successful do binding and set comments if needed
						   if (object != NULL)
						   {
							   WriteAuditLog(AUDIT_OBJECTS, TRUE, m_dwUserId, m_workstation, m_id, object->getId(), _T("Object %s created"), object->getName());
							   if ((pParent != NULL) &&          // parent can be NULL for nodes
							       (iClass != OBJECT_INTERFACE)) // interface already linked by Node::createNewInterface
							   {
								   pParent->AddChild(object);
								   object->AddParent(pParent);
								   pParent->calculateCompoundStatus();
								   if (pParent->getObjectClass() == OBJECT_CLUSTER)
								   {
									   ((Cluster *)pParent)->applyToTarget((DataCollectionTarget *)object);
								   }
								   if (object->getObjectClass() == OBJECT_NODELINK)
								   {
									   ((NodeLink *)object)->applyTemplates();
								   }
								   if (object->getObjectClass() == OBJECT_NETWORKSERVICE)
								   {
									   if (pRequest->getFieldAsUInt16(VID_CREATE_STATUS_DCI))
									   {
										   TCHAR dciName[MAX_DB_STRING], dciDescription[MAX_DB_STRING];

										   _sntprintf(dciName, MAX_DB_STRING, _T("ChildStatus(%d)"), object->getId());
										   _sntprintf(dciDescription, MAX_DB_STRING, _T("Status of network service %s"), object->getName());
										   ((Node *)pParent)->addDCObject(new DCItem(CreateUniqueId(IDG_ITEM), dciName, DS_INTERNAL, DCI_DT_INT,
											   ConfigReadInt(_T("DefaultDCIPollingInterval"), 60),
											   ConfigReadInt(_T("DefaultDCIRetentionTime"), 30), (Node *)pParent, dciDescription));
									   }
								   }
							   }

							   pszComments = pRequest->getFieldAsString(VID_COMMENTS);
							   if (pszComments != NULL)
								   object->setComments(pszComments);

							   object->unhide();
							   msg.setField(VID_RCC, RCC_SUCCESS);
							   msg.setField(VID_OBJECT_ID, object->getId());
						   }
						   else
						   {
							   // :DIRTY HACK:
							   // PollNewNode will return NULL only if IP already
							   // in use. some new() can fail there too, but server will
							   // crash in that case
							   if (iClass == OBJECT_NODE)
							   {
                  		   msg.setField(VID_RCC, RCC_ALREADY_EXIST);
                  		   //Add to description IP of new created node and name of node with the same IP
                           SetNodesConflictString(&msg, zoneId, ipAddr);
							   }
							   else if (iClass == OBJECT_ZONE)
							   {
                  		   msg.setField(VID_RCC, RCC_ZONE_ID_ALREADY_IN_USE);
							   }
							   else
							   {
                  		   msg.setField(VID_RCC, RCC_OBJECT_CREATION_FAILED);
							   }
						   }

                     ObjectTransactionEnd();
                  }
                  else
                  {
   						msg.setField(VID_RCC, moduleRCC);
                  }
					}
					else
					{
						msg.setField(VID_RCC, RCC_INVALID_OBJECT_NAME);
					}
				}
				else
				{
					msg.setField(VID_RCC, RCC_INVALID_ZONE_ID);
				}
         }
         else
         {
            msg.setField(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
         }
      }
      else
      {
         msg.setField(VID_RCC, RCC_ACCESS_DENIED);
      }
   }
   else
   {
      msg.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Add cluster node
 */
void ClientSession::addClusterNode(NXCPMessage *request)
{
   NXCPMessage msg;
	NetObj *cluster, *node;

	msg.setId(request->getId());
	msg.setCode(CMD_REQUEST_COMPLETED);

   cluster = FindObjectById(request->getFieldAsUInt32(VID_PARENT_ID));
   node = FindObjectById(request->getFieldAsUInt32(VID_CHILD_ID));
	if ((cluster != NULL) && (node != NULL))
	{
		if ((cluster->getObjectClass() == OBJECT_CLUSTER) && (node->getObjectClass() == OBJECT_NODE))
		{
			if (((Node *)node)->getMyCluster() == NULL)
			{
				if (cluster->checkAccessRights(m_dwUserId, OBJECT_ACCESS_MODIFY) &&
					 node->checkAccessRights(m_dwUserId, OBJECT_ACCESS_MODIFY))
				{
					((Cluster *)cluster)->applyToTarget((Node *)node);
					((Node *)node)->setRecheckCapsFlag();
					((Node *)node)->forceConfigurationPoll();

					msg.setField(VID_RCC, RCC_SUCCESS);
					WriteAuditLog(AUDIT_OBJECTS, TRUE, m_dwUserId, m_workstation, m_id, cluster->getId(),
									  _T("Node %s [%d] added to cluster %s [%d]"),
									  node->getName(), node->getId(), cluster->getName(), cluster->getId());
				}
				else
				{
					msg.setField(VID_RCC, RCC_ACCESS_DENIED);
					WriteAuditLog(AUDIT_OBJECTS, FALSE, m_dwUserId, m_workstation, m_id, cluster->getId(),
									  _T("Access denied on adding node %s [%d] to cluster %s [%d]"),
									  node->getName(), node->getId(), cluster->getName(), cluster->getId());
				}
			}
			else
			{
				msg.setField(VID_RCC, RCC_CLUSTER_MEMBER_ALREADY);
			}
		}
		else
		{
			msg.setField(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
		}
	}
	else
	{
		msg.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
	}

   // Send response
   sendMessage(&msg);
}

/**
 * Bind/unbind object
 */
void ClientSession::changeObjectBinding(NXCPMessage *pRequest, BOOL bBind)
{
   NXCPMessage msg;
   NetObj *pParent, *pChild;

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(pRequest->getId());

   // Get parent and child objects
   pParent = FindObjectById(pRequest->getFieldAsUInt32(VID_PARENT_ID));
   pChild = FindObjectById(pRequest->getFieldAsUInt32(VID_CHILD_ID));

   // Check access rights and change binding
   if ((pParent != NULL) && (pChild != NULL))
   {
      // User should have modify access to both objects
      if ((pParent->checkAccessRights(m_dwUserId, OBJECT_ACCESS_MODIFY)) &&
          (pChild->checkAccessRights(m_dwUserId, OBJECT_ACCESS_MODIFY)))
      {
         // Parent object should be container or service root,
			// or template group/root for templates and template groups
         // For unbind, it can also be template or cluster
         if (IsValidParentClass(pChild->getObjectClass(), pParent->getObjectClass()))
         {
            if (bBind)
            {
               // Prevent loops
               if (!pChild->isChild(pParent->getId()))
               {
                  ObjectTransactionStart();
                  pParent->AddChild(pChild);
                  pChild->AddParent(pParent);
                  ObjectTransactionEnd();
                  pParent->calculateCompoundStatus();
                  msg.setField(VID_RCC, RCC_SUCCESS);

						if ((pParent->getObjectClass() == OBJECT_BUSINESSSERVICEROOT) || (pParent->getObjectClass() == OBJECT_BUSINESSSERVICE))
						{
							((ServiceContainer *)pParent)->initUptimeStats();
						}
               }
               else
               {
                  msg.setField(VID_RCC, RCC_OBJECT_LOOP);
               }
            }
            else
            {
               ObjectTransactionStart();
               pParent->DeleteChild(pChild);
               pChild->DeleteParent(pParent);
               ObjectTransactionEnd();
               pParent->calculateCompoundStatus();
               if ((pParent->getObjectClass() == OBJECT_TEMPLATE) &&
                   ((pChild->getObjectClass() == OBJECT_NODE) || (pChild->getObjectClass() == OBJECT_CLUSTER) || (pChild->getObjectClass() == OBJECT_MOBILEDEVICE)))
               {
                  ((Template *)pParent)->queueRemoveFromTarget(pChild->getId(), pRequest->getFieldAsBoolean(VID_REMOVE_DCI));
               }
               else if ((pParent->getObjectClass() == OBJECT_CLUSTER) &&
                        (pChild->getObjectClass() == OBJECT_NODE))
               {
                  ((Cluster *)pParent)->queueRemoveFromTarget(pChild->getId(), TRUE);
						((Node *)pChild)->setRecheckCapsFlag();
						((Node *)pChild)->forceConfigurationPoll();
               }
					else if ((pParent->getObjectClass() == OBJECT_BUSINESSSERVICEROOT) || (pParent->getObjectClass() == OBJECT_BUSINESSSERVICE))
					{
						((ServiceContainer *)pParent)->initUptimeStats();
					}
               msg.setField(VID_RCC, RCC_SUCCESS);
            }
         }
         else
         {
            msg.setField(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
         }
      }
      else
      {
         msg.setField(VID_RCC, RCC_ACCESS_DENIED);
      }
   }
   else
   {
      msg.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Worker thread for object deletion
 */
static void DeleteObjectWorker(void *arg)
{
   ObjectTransactionStart();
	((NetObj *)arg)->deleteObject();
   ObjectTransactionEnd();
}

/**
 * Delete object
 */
void ClientSession::deleteObject(NXCPMessage *pRequest)
{
   NXCPMessage msg;
   NetObj *object;

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(pRequest->getId());

   // Find object to be deleted
   object = FindObjectById(pRequest->getFieldAsUInt32(VID_OBJECT_ID));
   if (object != NULL)
   {
      // Check if it is a built-in object, like "Entire Network"
      if (object->getId() >= 10)  // FIXME: change to 100
      {
         // Check access rights
         if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_DELETE))
         {
				if ((object->getObjectClass() != OBJECT_ZONE) || object->isEmpty())
				{
               ThreadPoolExecute(g_mainThreadPool, DeleteObjectWorker, object);
					msg.setField(VID_RCC, RCC_SUCCESS);
               WriteAuditLog(AUDIT_OBJECTS, TRUE, m_dwUserId, m_workstation, m_id, object->getId(), _T("Object %s deleted"), object->getName());
				}
				else
				{
	            msg.setField(VID_RCC, RCC_ZONE_NOT_EMPTY);
				}
         }
         else
         {
            msg.setField(VID_RCC, RCC_ACCESS_DENIED);
            WriteAuditLog(AUDIT_OBJECTS, FALSE, m_dwUserId, m_workstation, m_id, object->getId(), _T("Access denied on delete object %s"), object->getName());
         }
      }
      else
      {
         msg.setField(VID_RCC, RCC_ACCESS_DENIED);
         WriteAuditLog(AUDIT_OBJECTS, FALSE, m_dwUserId, m_workstation, m_id, object->getId(), _T("Access denied on delete object %s"), object->getName());
      }
   }
   else
   {
      msg.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Process changes in alarms
 */
void ClientSession::onAlarmUpdate(UINT32 dwCode, NXC_ALARM *pAlarm)
{
   UPDATE_INFO *pUpdate;
   NetObj *object;

   if (isAuthenticated() && (m_dwActiveChannels & NXC_CHANNEL_ALARMS))
   {
      object = FindObjectById(pAlarm->dwSourceObject);
      if (object != NULL)
         if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ_ALARMS))
         {
            pUpdate = (UPDATE_INFO *)malloc(sizeof(UPDATE_INFO));
            pUpdate->dwCategory = INFO_CAT_ALARM;
            pUpdate->dwCode = dwCode;
            pUpdate->pData = nx_memdup(pAlarm, sizeof(NXC_ALARM));
            m_pUpdateQueue->put(pUpdate);
         }
   }
}

/**
 * Send all alarms to client
 */
void ClientSession::sendAllAlarms(UINT32 dwRqId)
{
   MutexLock(m_mutexSendAlarms);
   SendAlarmsToClient(dwRqId, this);
   MutexUnlock(m_mutexSendAlarms);
}

/**
 * Get specific alarm object
 */
void ClientSession::getAlarm(NXCPMessage *request)
{
   NXCPMessage msg;

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(request->getId());

   // Get alarm id and it's source object
   UINT32 alarmId = request->getFieldAsUInt32(VID_ALARM_ID);
   NetObj *object = GetAlarmSourceObject(alarmId);
   if (object != NULL)
   {
      // User should have "view alarm" right to the object
      if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ_ALARMS))
      {
         msg.setField(VID_RCC, GetAlarm(alarmId, &msg));
      }
      else
      {
         msg.setField(VID_RCC, RCC_ACCESS_DENIED);
			WriteAuditLog(AUDIT_OBJECTS, FALSE, m_dwUserId, m_workstation, m_id, object->getId(), _T("Access denied on get alarm for object %s"), object->getName());
      }
   }
   else
   {
      // Normally, for existing alarms object will not be NULL,
      // so we assume that alarm id is invalid
      msg.setField(VID_RCC, RCC_INVALID_ALARM_ID);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Get all related events for specific alarm object
 */
void ClientSession::getAlarmEvents(NXCPMessage *request)
{
   NXCPMessage msg;

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(request->getId());

   // Get alarm id and it's source object
   UINT32 alarmId = request->getFieldAsUInt32(VID_ALARM_ID);
   NetObj *object = GetAlarmSourceObject(alarmId);
   if (object != NULL)
   {
      // User should have "view alarm" right to the object and
		// system-wide "view event log" access
      if ((m_dwSystemAccess & SYSTEM_ACCESS_VIEW_EVENT_LOG) && object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ_ALARMS))
      {
         msg.setField(VID_RCC, GetAlarmEvents(alarmId, &msg));
      }
      else
      {
         msg.setField(VID_RCC, RCC_ACCESS_DENIED);
			WriteAuditLog(AUDIT_OBJECTS, FALSE, m_dwUserId, m_workstation, m_id, object->getId(), _T("Access denied on get alarm events for object %s"), object->getName());
      }
   }
   else
   {
      // Normally, for existing alarms object will not be NULL,
      // so we assume that alarm id is invalid
      msg.setField(VID_RCC, RCC_INVALID_ALARM_ID);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Acknowledge alarm
 */
void ClientSession::acknowledgeAlarm(NXCPMessage *pRequest)
{
   NXCPMessage msg;

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(pRequest->getId());

   // Get alarm id and it's source object
   UINT32 alarmId;
   TCHAR hdref[MAX_HELPDESK_REF_LEN];
   NetObj *object;
   bool byHelpdeskRef;
   if (pRequest->isFieldExist(VID_HELPDESK_REF))
   {
      pRequest->getFieldAsString(VID_HELPDESK_REF, hdref, MAX_HELPDESK_REF_LEN);
      object = GetAlarmSourceObject(hdref);
      byHelpdeskRef = true;
   }
   else
   {
      alarmId = pRequest->getFieldAsUInt32(VID_ALARM_ID);
      object = GetAlarmSourceObject(alarmId);
      byHelpdeskRef = false;
   }
   if (object != NULL)
   {
      // User should have "acknowledge alarm" right to the object
      if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_UPDATE_ALARMS))
      {
			msg.setField(VID_RCC,
            byHelpdeskRef ?
            AckAlarmByHDRef(hdref, this, pRequest->getFieldAsUInt16(VID_STICKY_FLAG) != 0, pRequest->getFieldAsUInt32(VID_TIMESTAMP)) :
            AckAlarmById(alarmId, this, pRequest->getFieldAsUInt16(VID_STICKY_FLAG) != 0, pRequest->getFieldAsUInt32(VID_TIMESTAMP)));
      }
      else
      {
         msg.setField(VID_RCC, RCC_ACCESS_DENIED);
			WriteAuditLog(AUDIT_OBJECTS, FALSE, m_dwUserId, m_workstation, m_id, object->getId(), _T("Access denied on acknowledged alarm on object %s"), object->getName());
      }
   }
   else
   {
      // Normally, for existing alarms object will not be NULL,
      // so we assume that alarm id is invalid
      msg.setField(VID_RCC, RCC_INVALID_ALARM_ID);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Resolve/Terminate alarm
 */
void ClientSession::resolveAlarm(NXCPMessage *pRequest, bool terminate)
{
   NXCPMessage msg;

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(pRequest->getId());

   // Get alarm id and it's source object
   UINT32 alarmId;
   TCHAR hdref[MAX_HELPDESK_REF_LEN];
   NetObj *object;
   bool byHelpdeskRef;
   if (pRequest->isFieldExist(VID_HELPDESK_REF))
   {
      pRequest->getFieldAsString(VID_HELPDESK_REF, hdref, MAX_HELPDESK_REF_LEN);
      object = GetAlarmSourceObject(hdref);
      byHelpdeskRef = true;
   }
   else
   {
      alarmId = pRequest->getFieldAsUInt32(VID_ALARM_ID);
      object = GetAlarmSourceObject(alarmId);
      byHelpdeskRef = false;
   }
   if (object != NULL)
   {
      // User should have "terminate alarm" right to the object
      if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_TERM_ALARMS))
      {
         msg.setField(VID_RCC,
            byHelpdeskRef ?
            ResolveAlarmByHDRef(hdref, this, terminate) :
            ResolveAlarmById(alarmId, this, terminate));
      }
      else
      {
         msg.setField(VID_RCC, RCC_ACCESS_DENIED);
			WriteAuditLog(AUDIT_OBJECTS, FALSE, m_dwUserId, m_workstation, m_id, object->getId(),
            _T("Access denied on %s alarm on object %s"), terminate ? _T("terminate") : _T("resolve"), object->getName());
      }
   }
   else
   {
      // Normally, for existing alarms object will not be NULL,
      // so we assume that alarm id is invalid
      msg.setField(VID_RCC, RCC_INVALID_ALARM_ID);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Delete alarm
 */
void ClientSession::deleteAlarm(NXCPMessage *pRequest)
{
   NXCPMessage msg;

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(pRequest->getId());

   // Get alarm id and it's source object
   UINT32 alarmId = pRequest->getFieldAsUInt32(VID_ALARM_ID);
   NetObj *object = GetAlarmSourceObject(alarmId);
   if (object != NULL)
   {
      // User should have "terminate alarm" right to the object
      // and system right "delete alarms"
      if ((object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_TERM_ALARMS)) &&
          (m_dwSystemAccess & SYSTEM_ACCESS_DELETE_ALARMS))
      {
         DeleteAlarm(alarmId, false);
         msg.setField(VID_RCC, RCC_SUCCESS);
      }
      else
      {
         msg.setField(VID_RCC, RCC_ACCESS_DENIED);
			WriteAuditLog(AUDIT_OBJECTS, FALSE, m_dwUserId, m_workstation, m_id, object->getId(), _T("Access denied on delete alarm on object %s"), object->getName());
      }
   }
   else
   {
      // Normally, for existing alarms object will not be NULL,
      // so we assume that alarm id is invalid
      msg.setField(VID_RCC, RCC_INVALID_ALARM_ID);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Open issue in helpdesk system from given alarm
 */
void ClientSession::openHelpdeskIssue(NXCPMessage *request)
{
   NXCPMessage msg;

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(request->getId());

   // Get alarm id and it's source object
   UINT32 alarmId = request->getFieldAsUInt32(VID_ALARM_ID);
   NetObj *object = GetAlarmSourceObject(alarmId);
   if (object != NULL)
   {
      if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_CREATE_ISSUE))
      {
         TCHAR hdref[MAX_HELPDESK_REF_LEN];
         msg.setField(VID_RCC, OpenHelpdeskIssue(alarmId, this, hdref));
         msg.setField(VID_HELPDESK_REF, hdref);
      }
      else
      {
         msg.setField(VID_RCC, RCC_ACCESS_DENIED);
			WriteAuditLog(AUDIT_OBJECTS, FALSE, m_dwUserId, m_workstation, m_id, object->getId(),
            _T("Access denied on creating issue from alarm on object %s"), object->getName());
      }
   }
   else
   {
      // Normally, for existing alarms object will not be NULL,
      // so we assume that alarm id is invalid
      msg.setField(VID_RCC, RCC_INVALID_ALARM_ID);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Get helpdesk URL for given alarm
 */
void ClientSession::getHelpdeskUrl(NXCPMessage *request)
{
   NXCPMessage msg;

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(request->getId());

   // Get alarm id and it's source object
   UINT32 alarmId = request->getFieldAsUInt32(VID_ALARM_ID);
   NetObj *object = GetAlarmSourceObject(alarmId);
   if (object != NULL)
   {
      if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ_ALARMS))
      {
         TCHAR url[MAX_PATH];
         msg.setField(VID_RCC, GetHelpdeskIssueUrlFromAlarm(alarmId, url, MAX_PATH));
         msg.setField(VID_URL, url);
      }
      else
      {
         msg.setField(VID_RCC, RCC_ACCESS_DENIED);
			WriteAuditLog(AUDIT_OBJECTS, FALSE, m_dwUserId, m_workstation, m_id, object->getId(),
            _T("Access denied on getting helpdesk URL for alarm on object %s"), object->getName());
      }
   }
   else
   {
      // Normally, for existing alarms object will not be NULL,
      // so we assume that alarm id is invalid
      msg.setField(VID_RCC, RCC_INVALID_ALARM_ID);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Unlink helpdesk issue from alarm
 */
void ClientSession::unlinkHelpdeskIssue(NXCPMessage *request)
{
   NXCPMessage msg;

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(request->getId());

   // Get alarm id and it's source object
   UINT32 alarmId;
   TCHAR hdref[MAX_HELPDESK_REF_LEN];
   NetObj *object;
   bool byHelpdeskRef;
   if (request->isFieldExist(VID_HELPDESK_REF))
   {
      request->getFieldAsString(VID_HELPDESK_REF, hdref, MAX_HELPDESK_REF_LEN);
      object = GetAlarmSourceObject(hdref);
      byHelpdeskRef = true;
   }
   else
   {
      alarmId = request->getFieldAsUInt32(VID_ALARM_ID);
      object = GetAlarmSourceObject(alarmId);
      byHelpdeskRef = false;
   }
   if (object != NULL)
   {
      // User should have "update alarms" right to the object and "unlink issues" global right
      if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_UPDATE_ALARMS) && checkSysAccessRights(SYSTEM_ACCESS_UNLINK_ISSUES))
      {
         msg.setField(VID_RCC,
            byHelpdeskRef ?
            UnlinkHelpdeskIssueByHDRef(hdref, this) :
            UnlinkHelpdeskIssueById(alarmId, this));
      }
      else
      {
         msg.setField(VID_RCC, RCC_ACCESS_DENIED);
			WriteAuditLog(AUDIT_OBJECTS, FALSE, m_dwUserId, m_workstation, m_id, object->getId(),
            _T("Access denied on unlinking helpdesk issue from alarm on object %s"), object->getName());
      }
   }
   else
   {
      // Normally, for existing alarms object will not be NULL,
      // so we assume that alarm id is invalid
      msg.setField(VID_RCC, RCC_INVALID_ALARM_ID);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Get comments for given alarm
 */
void ClientSession::getAlarmComments(NXCPMessage *request)
{
   NXCPMessage msg;

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(request->getId());

   // Get alarm id and it's source object
   UINT32 alarmId = request->getFieldAsUInt32(VID_ALARM_ID);
   NetObj *object = GetAlarmSourceObject(alarmId);
   if (object != NULL)
   {
      // User should have "view alarms" right to the object
		if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ_ALARMS))
      {
			msg.setField(VID_RCC, GetAlarmComments(alarmId, &msg));
      }
      else
      {
         msg.setField(VID_RCC, RCC_ACCESS_DENIED);
      }
   }
   else
   {
      // Normally, for existing alarms object will not be NULL,
      // so we assume that alarm id is invalid
      msg.setField(VID_RCC, RCC_INVALID_ALARM_ID);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Update alarm comment
 */
void ClientSession::updateAlarmComment(NXCPMessage *request)
{
   NXCPMessage msg;

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(request->getId());

   // Get alarm id and it's source object
   UINT32 alarmId;
   TCHAR hdref[MAX_HELPDESK_REF_LEN];
   NetObj *object;
   bool byHelpdeskRef;
   if (request->isFieldExist(VID_HELPDESK_REF))
   {
      request->getFieldAsString(VID_HELPDESK_REF, hdref, MAX_HELPDESK_REF_LEN);
      object = GetAlarmSourceObject(hdref);
      byHelpdeskRef = true;
   }
   else
   {
      alarmId = request->getFieldAsUInt32(VID_ALARM_ID);
      object = GetAlarmSourceObject(alarmId);
      byHelpdeskRef = false;
   }
   if (object != NULL)
   {
      // User should have "update alarm" right to the object
		if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_UPDATE_ALARMS))
      {
			UINT32 commentId = request->getFieldAsUInt32(VID_COMMENT_ID);
			TCHAR *text = request->getFieldAsString(VID_COMMENTS);
			msg.setField(VID_RCC,
            byHelpdeskRef ?
            AddAlarmComment(hdref, CHECK_NULL(text), m_dwUserId) :
            UpdateAlarmComment(alarmId, commentId, CHECK_NULL(text), m_dwUserId));
			safe_free(text);
      }
      else
      {
         msg.setField(VID_RCC, RCC_ACCESS_DENIED);
      }
   }
   else
   {
      // Normally, for existing alarms object will not be NULL,
      // so we assume that alarm id is invalid
      msg.setField(VID_RCC, RCC_INVALID_ALARM_ID);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Delete alarm comment
 */
void ClientSession::deleteAlarmComment(NXCPMessage *request)
{
   NXCPMessage msg;

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(request->getId());

   // Get alarm id and it's source object
   UINT32 alarmId = request->getFieldAsUInt32(VID_ALARM_ID);
   NetObj *object = GetAlarmSourceObject(alarmId);
   if (object != NULL)
   {
      // User should have "acknowledge alarm" right to the object
		if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_UPDATE_ALARMS))
      {
			UINT32 commentId = request->getFieldAsUInt32(VID_COMMENT_ID);
			msg.setField(VID_RCC, DeleteAlarmCommentByID(alarmId, commentId));
      }
      else
      {
         msg.setField(VID_RCC, RCC_ACCESS_DENIED);
      }
   }
   else
   {
      // Normally, for existing alarms object will not be NULL,
      // so we assume that alarm id is invalid
      msg.setField(VID_RCC, RCC_INVALID_ALARM_ID);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Update alarm status flow mode
 */
void ClientSession::updateAlarmStatusFlow(NXCPMessage *request)
{
   NXCPMessage msg;

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(request->getId());
   int status = request->getFieldAsUInt32(VID_ALARM_STATUS_FLOW_STATE);

   ConfigWriteInt(_T("StrictAlarmStatusFlow"), status, false);
   msg.setField(VID_RCC, RCC_SUCCESS);

   // Send response
   sendMessage(&msg);
}

/**
 * Create new server action
 */
void ClientSession::createAction(NXCPMessage *pRequest)
{
   NXCPMessage msg;

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(pRequest->getId());

   // Check user rights
   if (m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_ACTIONS)
   {
      UINT32 dwResult, dwActionId;
      TCHAR szActionName[MAX_OBJECT_NAME];

      pRequest->getFieldAsString(VID_ACTION_NAME, szActionName, MAX_OBJECT_NAME);
      if (IsValidObjectName(szActionName, TRUE))
      {
         dwResult = CreateNewAction(szActionName, &dwActionId);
         msg.setField(VID_RCC, dwResult);
         if (dwResult == RCC_SUCCESS)
            msg.setField(VID_ACTION_ID, dwActionId);   // Send id of new action to client
      }
      else
      {
         msg.setField(VID_RCC, RCC_INVALID_OBJECT_NAME);
      }
   }
   else
   {
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Update existing action's data
 */
void ClientSession::updateAction(NXCPMessage *pRequest)
{
   NXCPMessage msg;

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(pRequest->getId());

   // Check user rights
   if (m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_ACTIONS)
   {
      msg.setField(VID_RCC, ModifyActionFromMessage(pRequest));
   }
   else
   {
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Delete action
 */
void ClientSession::deleteAction(NXCPMessage *pRequest)
{
   NXCPMessage msg;
   UINT32 dwActionId;

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(pRequest->getId());

   // Check user rights
   if (m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_ACTIONS)
   {
      // Get Id of action to be deleted
      dwActionId = pRequest->getFieldAsUInt32(VID_ACTION_ID);
      if (!g_pEventPolicy->isActionInUse(dwActionId))
      {
         msg.setField(VID_RCC, DeleteActionFromDB(dwActionId));
      }
      else
      {
         msg.setField(VID_RCC, RCC_ACTION_IN_USE);
      }
   }
   else
   {
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Process changes in actions
 */
void ClientSession::onActionDBUpdate(UINT32 dwCode, NXC_ACTION *pAction)
{
   UPDATE_INFO *pUpdate;

   if (isAuthenticated())
   {
      if ((m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_ACTIONS) ||
			 (m_dwSystemAccess & SYSTEM_ACCESS_EPP))
      {
         pUpdate = (UPDATE_INFO *)malloc(sizeof(UPDATE_INFO));
         pUpdate->dwCategory = INFO_CAT_ACTION;
         pUpdate->dwCode = dwCode;
         pUpdate->pData = nx_memdup(pAction, sizeof(NXC_ACTION));
         m_pUpdateQueue->put(pUpdate);
      }
   }
}

/**
 * Send all actions to client
 */
void ClientSession::sendAllActions(UINT32 dwRqId)
{
   NXCPMessage msg;

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(dwRqId);

   // Check user rights
   if ((m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_ACTIONS) ||
       (m_dwSystemAccess & SYSTEM_ACCESS_EPP))
   {
      msg.setField(VID_RCC, RCC_SUCCESS);
      sendMessage(&msg);
      MutexLock(m_mutexSendActions);
      SendActionsToClient(this, dwRqId);
      MutexUnlock(m_mutexSendActions);
   }
   else
   {
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);
      sendMessage(&msg);
   }
}


//
// Send list of configured container categories to client
//

void ClientSession::SendContainerCategories(UINT32 dwRqId)
{
   NXCPMessage msg;
   UINT32 i;

   // Prepare response message
   msg.setCode(CMD_CONTAINER_CAT_DATA);
   msg.setId(dwRqId);

   for(i = 0; i < g_dwNumCategories; i++)
   {
      msg.setField(VID_CATEGORY_ID, g_pContainerCatList[i].dwCatId);
      msg.setField(VID_CATEGORY_NAME, g_pContainerCatList[i].szName);
      //msg.setField(VID_IMAGE_ID, g_pContainerCatList[i].dwImageId);
      msg.setField(VID_DESCRIPTION, g_pContainerCatList[i].pszDescription);
      sendMessage(&msg);
      msg.deleteAllFields();
   }

   // Send end-of-list indicator
   msg.setField(VID_CATEGORY_ID, (UINT32)0);
   sendMessage(&msg);
}

/**
 * Perform a forced node poll
 */
void ClientSession::forcedNodePoll(NXCPMessage *pRequest)
{
   NXCPMessage msg;
   POLLER_START_DATA *pData;
   NetObj *object;

   pData = (POLLER_START_DATA *)malloc(sizeof(POLLER_START_DATA));
   pData->pSession = this;
   MutexLock(m_mutexPollerInit);

   // Prepare response message
   pData->dwRqId = pRequest->getId();
   msg.setCode(CMD_POLLING_INFO);
   msg.setId(pData->dwRqId);

   // Get polling type
   pData->iPollType = pRequest->getFieldAsUInt16(VID_POLL_TYPE);

   // Find object to be polled
   object = FindObjectById(pRequest->getFieldAsUInt32(VID_OBJECT_ID));
   if (object != NULL)
   {
      // We can do polls only for node objects
      if ((object->getObjectClass() == OBJECT_NODE) &&
          ((pData->iPollType == POLL_STATUS) ||
			  (pData->iPollType == POLL_CONFIGURATION_FULL) ||
			  (pData->iPollType == POLL_CONFIGURATION_NORMAL) ||
			  (pData->iPollType == POLL_INSTANCE_DISCOVERY) ||
			  (pData->iPollType == POLL_TOPOLOGY) ||
			  (pData->iPollType == POLL_INTERFACE_NAMES)))
      {
         // Check access rights
         if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_MODIFY))
         {
            ((Node *)object)->incRefCount();
            m_dwRefCount++;

            pData->pNode = (Node *)object;
            ThreadPoolExecute(g_mainThreadPool, pollerThreadStarter, pData);
            msg.setField(VID_RCC, RCC_OPERATION_IN_PROGRESS);
            msg.setField(VID_POLLER_MESSAGE, _T("Poll request accepted\r\n"));
				pData = NULL;
         }
         else
         {
            msg.setField(VID_RCC, RCC_ACCESS_DENIED);
         }
      }
      else
      {
         msg.setField(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
      }
   }
   else
   {
      msg.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   // Send response
   sendMessage(&msg);
   MutexUnlock(m_mutexPollerInit);
	safe_free(pData);
}

/**
 * Send message from poller to client
 */
void ClientSession::sendPollerMsg(UINT32 dwRqId, const TCHAR *pszMsg)
{
   NXCPMessage msg;

   msg.setCode(CMD_POLLING_INFO);
   msg.setId(dwRqId);
   msg.setField(VID_RCC, RCC_OPERATION_IN_PROGRESS);
   msg.setField(VID_POLLER_MESSAGE, pszMsg);
   sendMessage(&msg);
}

/**
 * Forced node poll thread starter
 */
void ClientSession::pollerThreadStarter(void *pArg)
{
   ((POLLER_START_DATA *)pArg)->pSession->pollerThread(
      ((POLLER_START_DATA *)pArg)->pNode,
      ((POLLER_START_DATA *)pArg)->iPollType,
      ((POLLER_START_DATA *)pArg)->dwRqId);
   ((POLLER_START_DATA *)pArg)->pSession->decRefCount();
   free(pArg);
}

/**
 * Node poller thread
 */
void ClientSession::pollerThread(Node *pNode, int iPollType, UINT32 dwRqId)
{
   NXCPMessage msg;

   // Wait while parent thread finishes initialization
   MutexLock(m_mutexPollerInit);
   MutexUnlock(m_mutexPollerInit);

   PollerInfo *poller = NULL;
   switch(iPollType)
   {
      case POLL_STATUS:
         poller = RegisterPoller(POLLER_TYPE_STATUS, pNode);
         poller->startExecution();
         pNode->statusPoll(this, dwRqId, poller);
         break;
      case POLL_CONFIGURATION_FULL:
			pNode->setRecheckCapsFlag();
         // intentionally no break here
      case POLL_CONFIGURATION_NORMAL:
         poller = RegisterPoller(POLLER_TYPE_CONFIGURATION, pNode);
         poller->startExecution();
         pNode->configurationPoll(this, dwRqId, poller, 0);
         break;
      case POLL_INSTANCE_DISCOVERY:
         poller = RegisterPoller(POLLER_TYPE_INSTANCE_DISCOVERY, pNode);
         poller->startExecution();
         pNode->instanceDiscoveryPoll(this, dwRqId, poller);
         break;
      case POLL_TOPOLOGY:
         poller = RegisterPoller(POLLER_TYPE_TOPOLOGY, pNode);
         poller->startExecution();
         pNode->topologyPoll(this, dwRqId, poller);
         break;
      case POLL_INTERFACE_NAMES:
         pNode->updateInterfaceNames(this, dwRqId);
         break;
      default:
         sendPollerMsg(dwRqId, _T("Invalid poll type requested\r\n"));
         break;
   }
   pNode->decRefCount();
   delete poller;

   msg.setCode(CMD_POLLING_INFO);
   msg.setId(dwRqId);
   msg.setField(VID_RCC, RCC_SUCCESS);
   sendMessage(&msg);
}

/**
 * Receive event from user
 */
void ClientSession::onTrap(NXCPMessage *pRequest)
{
   NXCPMessage msg;
   UINT32 dwObjectId, dwEventCode;
   int i, iNumArgs;
   NetObj *object;
   TCHAR *pszArgList[32], szUserTag[MAX_USERTAG_LENGTH] = _T("");
   char szFormat[] = "ssssssssssssssssssssssssssssssss";
   BOOL bSuccess;

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(pRequest->getId());

   // Find event's source object
   dwObjectId = pRequest->getFieldAsUInt32(VID_OBJECT_ID);
   if (dwObjectId != 0)
	{
      object = FindObjectById(dwObjectId);  // Object is specified explicitely
	}
   else   // Client is the source
	{
      InetAddress addr = InetAddress::createFromSockaddr(m_clientAddr);
      if (addr.isLoopback())
      {
			object = FindObjectById(g_dwMgmtNode);
      }
      else
      {
			object = FindNodeByIP(0, addr);
      }
	}
   if (object != NULL)
   {
      // User should have SEND_EVENTS access right to object
      if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_SEND_EVENTS))
      {
         dwEventCode = pRequest->getFieldAsUInt32(VID_EVENT_CODE);
			if ((dwEventCode == 0) && pRequest->isFieldExist(VID_EVENT_NAME))
			{
				TCHAR eventName[256];
				pRequest->getFieldAsString(VID_EVENT_NAME, eventName, 256);
				dwEventCode = EventCodeFromName(eventName, 0);
			}
			pRequest->getFieldAsString(VID_USER_TAG, szUserTag, MAX_USERTAG_LENGTH);
         iNumArgs = pRequest->getFieldAsUInt16(VID_NUM_ARGS);
         if (iNumArgs > 32)
            iNumArgs = 32;
         for(i = 0; i < iNumArgs; i++)
            pszArgList[i] = pRequest->getFieldAsString(VID_EVENT_ARG_BASE + i);

         szFormat[iNumArgs] = 0;
         bSuccess = PostEventWithTag(dwEventCode, object->getId(), szUserTag, (iNumArgs > 0) ? szFormat : NULL,
                                     pszArgList[0], pszArgList[1], pszArgList[2], pszArgList[3],
                                     pszArgList[4], pszArgList[5], pszArgList[6], pszArgList[7],
                                     pszArgList[8], pszArgList[9], pszArgList[10], pszArgList[11],
                                     pszArgList[12], pszArgList[13], pszArgList[14], pszArgList[15],
                                     pszArgList[16], pszArgList[17], pszArgList[18], pszArgList[19],
                                     pszArgList[20], pszArgList[21], pszArgList[22], pszArgList[23],
                                     pszArgList[24], pszArgList[25], pszArgList[26], pszArgList[27],
                                     pszArgList[28], pszArgList[29], pszArgList[30], pszArgList[31]);

         // Cleanup
         for(i = 0; i < iNumArgs; i++)
            safe_free(pszArgList[i]);

         msg.setField(VID_RCC, bSuccess ? RCC_SUCCESS : RCC_INVALID_EVENT_CODE);
      }
      else
      {
         msg.setField(VID_RCC, RCC_ACCESS_DENIED);
      }
   }
   else
   {
      msg.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Wake up node
 */
void ClientSession::onWakeUpNode(NXCPMessage *pRequest)
{
   NXCPMessage msg;

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(pRequest->getId());

   // Find node or interface object
   NetObj *object = FindObjectById(pRequest->getFieldAsUInt32(VID_OBJECT_ID));
   if (object != NULL)
   {
      if ((object->getObjectClass() == OBJECT_NODE) ||
          (object->getObjectClass() == OBJECT_INTERFACE))
      {
         if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_CONTROL))
         {
            UINT32 dwResult;

            if (object->getObjectClass() == OBJECT_NODE)
               dwResult = ((Node *)object)->wakeUp();
            else
               dwResult = ((Interface *)object)->wakeUp();
            msg.setField(VID_RCC, dwResult);
         }
         else
         {
            msg.setField(VID_RCC, RCC_ACCESS_DENIED);
         }
      }
      else
      {
         msg.setField(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
      }
   }
   else
   {
      msg.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Query specific parameter from node
 */
void ClientSession::queryParameter(NXCPMessage *pRequest)
{
   NXCPMessage msg;

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(pRequest->getId());

   // Find node object
   NetObj *object = FindObjectById(pRequest->getFieldAsUInt32(VID_OBJECT_ID));
   if (object != NULL)
   {
      if (object->getObjectClass() == OBJECT_NODE)
      {
         if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
         {
            UINT32 dwResult;
            TCHAR szBuffer[256], szName[MAX_PARAM_NAME];

            pRequest->getFieldAsString(VID_NAME, szName, MAX_PARAM_NAME);
            dwResult = ((Node *)object)->getItemForClient(pRequest->getFieldAsUInt16(VID_DCI_SOURCE_TYPE),
                                                           szName, szBuffer, 256);
            msg.setField(VID_RCC, dwResult);
            if (dwResult == RCC_SUCCESS)
               msg.setField(VID_VALUE, szBuffer);
         }
         else
         {
            msg.setField(VID_RCC, RCC_ACCESS_DENIED);
         }
      }
      else
      {
         msg.setField(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
      }
   }
   else
   {
      msg.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Query specific table from node
 */
void ClientSession::queryAgentTable(NXCPMessage *pRequest)
{
   NXCPMessage msg;

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(pRequest->getId());

   // Find node object
   NetObj *object = FindObjectById(pRequest->getFieldAsUInt32(VID_OBJECT_ID));
   if (object != NULL)
   {
      if (object->getObjectClass() == OBJECT_NODE)
      {
         if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
         {
				TCHAR name[MAX_PARAM_NAME];
				pRequest->getFieldAsString(VID_NAME, name, MAX_PARAM_NAME);

				Table *table;
				UINT32 rcc = ((Node *)object)->getTableForClient(name, &table);
				msg.setField(VID_RCC, rcc);
				if (rcc == RCC_SUCCESS)
				{
					table->fillMessage(msg, 0, -1);
					delete table;
				}
         }
         else
         {
            msg.setField(VID_RCC, RCC_ACCESS_DENIED);
         }
      }
      else
      {
         msg.setField(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
      }
   }
   else
   {
      msg.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Edit trap configuration record
 */
void ClientSession::editTrap(int iOperation, NXCPMessage *pRequest)
{
   NXCPMessage msg;
   UINT32 dwTrapId, dwResult;

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(pRequest->getId());

   // Check access rights
   if (checkSysAccessRights(SYSTEM_ACCESS_CONFIGURE_TRAPS))
   {
      switch(iOperation)
      {
         case TRAP_CREATE:
            dwResult = CreateNewTrap(&dwTrapId);
            msg.setField(VID_RCC, dwResult);
            if (dwResult == RCC_SUCCESS)
               msg.setField(VID_TRAP_ID, dwTrapId);   // Send id of new trap to client
            break;
         case TRAP_UPDATE:
            msg.setField(VID_RCC, UpdateTrapFromMsg(pRequest));
            break;
         case TRAP_DELETE:
            dwTrapId = pRequest->getFieldAsUInt32(VID_TRAP_ID);
            msg.setField(VID_RCC, DeleteTrap(dwTrapId));
            break;
         default:
				msg.setField(VID_RCC, RCC_INVALID_ARGUMENT);
            break;
      }
   }
   else
   {
      // Current user has no rights for trap management
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Send all trap configuration records to client
 */
void ClientSession::sendAllTraps(UINT32 dwRqId)
{
   NXCPMessage msg;
   BOOL bSuccess = FALSE;

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(dwRqId);

	if (checkSysAccessRights(SYSTEM_ACCESS_CONFIGURE_TRAPS))
   {
      msg.setField(VID_RCC, RCC_SUCCESS);
      sendMessage(&msg);
      bSuccess = TRUE;
      SendTrapsToClient(this, dwRqId);
   }
   else
   {
      // Current user has no rights for trap management
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);
   }

   // Send response
   if (!bSuccess)
      sendMessage(&msg);
}

/**
 * Send all trap configuration records to client
 * Send all in one message with less information and don't need a lock
 */
void ClientSession::sendAllTraps2(UINT32 dwRqId)
{
   NXCPMessage msg;

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(dwRqId);

	if (checkSysAccessRights(SYSTEM_ACCESS_CONFIGURE_TRAPS))
   {
      msg.setField(VID_RCC, RCC_SUCCESS);
      CreateTrapCfgMessage(msg);
   }
   else
   {
      // Current user has no rights for trap management
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);
   }

   sendMessage(&msg);
}


//
// Lock/unlock package database
//

void ClientSession::LockPackageDB(UINT32 dwRqId, BOOL bLock)
{
   NXCPMessage msg;
   TCHAR szBuffer[256];

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(dwRqId);

   if (m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_PACKAGES)
   {
      if (bLock)
      {
         if (!LockComponent(CID_PACKAGE_DB, m_id, m_sessionName, NULL, szBuffer))
         {
            msg.setField(VID_RCC, RCC_COMPONENT_LOCKED);
            msg.setField(VID_LOCKED_BY, szBuffer);
         }
         else
         {
            m_dwFlags |= CSF_PACKAGE_DB_LOCKED;
            msg.setField(VID_RCC, RCC_SUCCESS);
         }
      }
      else
      {
         if (m_dwFlags & CSF_PACKAGE_DB_LOCKED)
         {
            UnlockComponent(CID_PACKAGE_DB);
            m_dwFlags &= ~CSF_PACKAGE_DB_LOCKED;
         }
         msg.setField(VID_RCC, RCC_SUCCESS);
      }
   }
   else
   {
      // Current user has no rights for trap management
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Send list of all installed packages to client
 */
void ClientSession::SendAllPackages(UINT32 dwRqId)
{
   NXCPMessage msg;
   DB_ASYNC_RESULT hResult;
   BOOL bSuccess = FALSE;
   TCHAR szBuffer[MAX_DB_STRING];

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(dwRqId);

   if (m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_PACKAGES)
   {
      if (m_dwFlags & CSF_PACKAGE_DB_LOCKED)
      {
         DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
         hResult = DBAsyncSelect(hdb, _T("SELECT pkg_id,version,platform,pkg_file,pkg_name,description FROM agent_pkg"));
         if (hResult != NULL)
         {
            msg.setField(VID_RCC, RCC_SUCCESS);
            sendMessage(&msg);
            bSuccess = TRUE;

            msg.setCode(CMD_PACKAGE_INFO);
            msg.deleteAllFields();

            while(DBFetch(hResult))
            {
               msg.setField(VID_PACKAGE_ID, DBGetFieldAsyncULong(hResult, 0));
               msg.setField(VID_PACKAGE_VERSION, DBGetFieldAsync(hResult, 1, szBuffer, MAX_DB_STRING));
               msg.setField(VID_PLATFORM_NAME, DBGetFieldAsync(hResult, 2, szBuffer, MAX_DB_STRING));
               msg.setField(VID_FILE_NAME, DBGetFieldAsync(hResult, 3, szBuffer, MAX_DB_STRING));
               msg.setField(VID_PACKAGE_NAME, DBGetFieldAsync(hResult, 4, szBuffer, MAX_DB_STRING));
               DBGetFieldAsync(hResult, 5, szBuffer, MAX_DB_STRING);
               DecodeSQLString(szBuffer);
               msg.setField(VID_DESCRIPTION, szBuffer);
               sendMessage(&msg);
               msg.deleteAllFields();
            }

            msg.setField(VID_PACKAGE_ID, (UINT32)0);
            sendMessage(&msg);

            DBFreeAsyncResult(hResult);
         }
         else
         {
            msg.setField(VID_RCC, RCC_DB_FAILURE);
         }
         DBConnectionPoolReleaseConnection(hdb);
      }
      else
      {
         msg.setField(VID_RCC, RCC_OUT_OF_STATE_REQUEST);
      }
   }
   else
   {
      // Current user has no rights for package management
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);
   }

   // Send response
   if (!bSuccess)
      sendMessage(&msg);
}

/**
 * Install package to server
 */
void ClientSession::InstallPackage(NXCPMessage *pRequest)
{
   NXCPMessage msg;
   TCHAR szPkgName[MAX_PACKAGE_NAME_LEN], szDescription[MAX_DB_STRING];
   TCHAR szPkgVersion[MAX_AGENT_VERSION_LEN], szFileName[MAX_DB_STRING];
   TCHAR szPlatform[MAX_PLATFORM_NAME_LEN], *pszEscDescr;
   TCHAR szQuery[2048];

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(pRequest->getId());

   if (m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_PACKAGES)
   {
      if (m_dwFlags & CSF_PACKAGE_DB_LOCKED)
      {
         pRequest->getFieldAsString(VID_PACKAGE_NAME, szPkgName, MAX_PACKAGE_NAME_LEN);
         pRequest->getFieldAsString(VID_DESCRIPTION, szDescription, MAX_DB_STRING);
         pRequest->getFieldAsString(VID_FILE_NAME, szFileName, MAX_DB_STRING);
         pRequest->getFieldAsString(VID_PACKAGE_VERSION, szPkgVersion, MAX_AGENT_VERSION_LEN);
         pRequest->getFieldAsString(VID_PLATFORM_NAME, szPlatform, MAX_PLATFORM_NAME_LEN);

         // Remove possible path specification from file name
         const TCHAR *pszCleanFileName = GetCleanFileName(szFileName);

         if (IsValidObjectName(pszCleanFileName) &&
             IsValidObjectName(szPkgName) &&
             IsValidObjectName(szPkgVersion) &&
             IsValidObjectName(szPlatform))
         {
            // Check if same package already exist
            if (!IsPackageInstalled(szPkgName, szPkgVersion, szPlatform))
            {
               // Check for duplicate file name
               if (!IsPackageFileExist(pszCleanFileName))
               {
                  // Prepare for file receive
                  if (m_hCurrFile == -1)
                  {
                     _tcscpy(m_szCurrFileName, g_netxmsdDataDir);
                     _tcscat(m_szCurrFileName, DDIR_PACKAGES);
                     _tcscat(m_szCurrFileName, FS_PATH_SEPARATOR);
                     _tcscat(m_szCurrFileName, pszCleanFileName);
                     m_hCurrFile = _topen(m_szCurrFileName, O_CREAT | O_TRUNC | O_WRONLY | O_BINARY, S_IRUSR | S_IWUSR);
                     if (m_hCurrFile != -1)
                     {
                        m_dwFileRqId = pRequest->getId();
                        m_dwUploadCommand = CMD_INSTALL_PACKAGE;
                        m_dwUploadData = CreateUniqueId(IDG_PACKAGE);
                        msg.setField(VID_RCC, RCC_SUCCESS);
                        msg.setField(VID_PACKAGE_ID, m_dwUploadData);

                        // Create record in database
                        pszEscDescr = EncodeSQLString(szDescription);
                        _sntprintf(szQuery, 2048, _T("INSERT INTO agent_pkg (pkg_id,pkg_name,")
                                                     _T("version,description,platform,pkg_file) ")
                                                     _T("VALUES (%d,'%s','%s','%s','%s','%s')"),
                                   m_dwUploadData, szPkgName, szPkgVersion, pszEscDescr,
                                   szPlatform, pszCleanFileName);
                        free(pszEscDescr);
                        DBQuery(g_hCoreDB, szQuery);
                     }
                     else
                     {
                        msg.setField(VID_RCC, RCC_IO_ERROR);
                     }
                  }
                  else
                  {
                     msg.setField(VID_RCC, RCC_RESOURCE_BUSY);
                  }
               }
               else
               {
                  msg.setField(VID_RCC, RCC_PACKAGE_FILE_EXIST);
               }
            }
            else
            {
               msg.setField(VID_RCC, RCC_DUPLICATE_PACKAGE);
            }
         }
         else
         {
            msg.setField(VID_RCC, RCC_INVALID_OBJECT_NAME);
         }
      }
      else
      {
         msg.setField(VID_RCC, RCC_OUT_OF_STATE_REQUEST);
      }
   }
   else
   {
      // Current user has no rights for package management
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);
   }

   // Send response
   sendMessage(&msg);
}


//
// Remove package from server
//

void ClientSession::RemovePackage(NXCPMessage *pRequest)
{
   NXCPMessage msg;
   UINT32 dwPkgId;

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(pRequest->getId());

   if (m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_PACKAGES)
   {
      if (m_dwFlags & CSF_PACKAGE_DB_LOCKED)
      {
         dwPkgId = pRequest->getFieldAsUInt32(VID_PACKAGE_ID);
         msg.setField(VID_RCC, UninstallPackage(dwPkgId));
      }
      else
      {
         msg.setField(VID_RCC, RCC_OUT_OF_STATE_REQUEST);
      }
   }
   else
   {
      // Current user has no rights for package management
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Get list of parameters supported by given node
 */
void ClientSession::getParametersList(NXCPMessage *pRequest)
{
   NXCPMessage msg;

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(pRequest->getId());

   NetObj *object = FindObjectById(pRequest->getFieldAsUInt32(VID_OBJECT_ID));
   if (object != NULL)
   {
      switch(object->getObjectClass())
      {
         case OBJECT_NODE:
            msg.setField(VID_RCC, RCC_SUCCESS);
				((Node *)object)->writeParamListToMessage(&msg, pRequest->getFieldAsUInt16(VID_FLAGS));
            break;
         case OBJECT_CLUSTER:
         case OBJECT_TEMPLATE:
            msg.setField(VID_RCC, RCC_SUCCESS);
            WriteFullParamListToMessage(&msg, pRequest->getFieldAsUInt16(VID_FLAGS));
            break;
         default:
            msg.setField(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
            break;
      }
   }
   else
   {
      msg.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Deplay package to node(s)
 */
void ClientSession::DeployPackage(NXCPMessage *pRequest)
{
   NXCPMessage msg;
   UINT32 i, dwNumObjects, *pdwObjectList, dwPkgId;
   NetObj *object;
   TCHAR szQuery[256], szPkgFile[MAX_PATH];
   TCHAR szVersion[MAX_AGENT_VERSION_LEN], szPlatform[MAX_PLATFORM_NAME_LEN];
   DB_RESULT hResult;
   BOOL bSuccess = TRUE;
   MUTEX hMutex;

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(pRequest->getId());

   if (m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_PACKAGES)
   {
		ObjectArray<Node> *nodeList = NULL;

      // Get package ID
      dwPkgId = pRequest->getFieldAsUInt32(VID_PACKAGE_ID);
      if (IsValidPackageId(dwPkgId))
      {
         // Read package information
         _sntprintf(szQuery, 256, _T("SELECT platform,pkg_file,version FROM agent_pkg WHERE pkg_id=%d"), dwPkgId);
         hResult = DBSelect(g_hCoreDB, szQuery);
         if (hResult != NULL)
         {
            if (DBGetNumRows(hResult) > 0)
            {
               DBGetField(hResult, 0, 0, szPlatform, MAX_PLATFORM_NAME_LEN);
               DBGetField(hResult, 0, 1, szPkgFile, MAX_PATH);
               DBGetField(hResult, 0, 2, szVersion, MAX_AGENT_VERSION_LEN);

               // Create list of nodes to be upgraded
               dwNumObjects = pRequest->getFieldAsUInt32(VID_NUM_OBJECTS);
               pdwObjectList = (UINT32 *)malloc(sizeof(UINT32) * dwNumObjects);
               pRequest->getFieldAsInt32Array(VID_OBJECT_LIST, dwNumObjects, pdwObjectList);
					nodeList = new ObjectArray<Node>((int)dwNumObjects);
               for(i = 0; i < dwNumObjects; i++)
               {
                  object = FindObjectById(pdwObjectList[i]);
                  if (object != NULL)
                  {
                     if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
                     {
                        if (object->getObjectClass() == OBJECT_NODE)
                        {
                           // Check if this node already in the list
									int j;
                           for(j = 0; j < nodeList->size(); j++)
                              if (nodeList->get(j)->getId() == pdwObjectList[i])
                                 break;
                           if (j == nodeList->size())
                           {
                              object->incRefCount();
                              nodeList->add((Node *)object);
                           }
                        }
                        else
                        {
                           object->addChildNodesToList(nodeList, m_dwUserId);
                        }
                     }
                     else
                     {
                        msg.setField(VID_RCC, RCC_ACCESS_DENIED);
                        bSuccess = FALSE;
                        break;
                     }
                  }
                  else
                  {
                     msg.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
                     bSuccess = FALSE;
                     break;
                  }
               }
               safe_free(pdwObjectList);
            }
            else
            {
               msg.setField(VID_RCC, RCC_DB_FAILURE);
               bSuccess = FALSE;
            }
            DBFreeResult(hResult);
         }
         else
         {
            msg.setField(VID_RCC, RCC_DB_FAILURE);
            bSuccess = FALSE;
         }
      }
      else
      {
         msg.setField(VID_RCC, RCC_INVALID_PACKAGE_ID);
         bSuccess = FALSE;
      }

      // On success, start upgrade thread
      if (bSuccess)
      {
         DT_STARTUP_INFO *pInfo;

         hMutex = MutexCreate();
         MutexLock(hMutex);

         pInfo = (DT_STARTUP_INFO *)malloc(sizeof(DT_STARTUP_INFO));
         pInfo->nodeList = nodeList;
         pInfo->pSession = this;
         pInfo->mutex = hMutex;
         pInfo->dwRqId = pRequest->getId();
         pInfo->dwPackageId = dwPkgId;
         _tcscpy(pInfo->szPkgFile, szPkgFile);
         _tcscpy(pInfo->szPlatform, szPlatform);
         _tcscpy(pInfo->szVersion, szVersion);

         m_dwRefCount++;
         ThreadCreate(DeploymentManager, 0, pInfo);
         msg.setField(VID_RCC, RCC_SUCCESS);
      }
      else
      {
			if (nodeList != NULL)
			{
				for(int i = 0; i < nodeList->size(); i++)
					nodeList->get(i)->decRefCount();
				delete nodeList;
			}
      }
   }
   else
   {
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);
      bSuccess = FALSE;
   }

   // Send response
   sendMessage(&msg);

   // Allow deployment thread to run
   if (bSuccess)
      MutexUnlock(hMutex);
}

/**
 * Apply data collection template to node or mobile device
 */
void ClientSession::applyTemplate(NXCPMessage *pRequest)
{
   NXCPMessage msg;
   NetObj *pSource, *pDestination;

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(pRequest->getId());

   // Get source and destination
   pSource = FindObjectById(pRequest->getFieldAsUInt32(VID_SOURCE_OBJECT_ID));
   pDestination = FindObjectById(pRequest->getFieldAsUInt32(VID_DESTINATION_OBJECT_ID));
   if ((pSource != NULL) && (pDestination != NULL))
   {
      // Check object types
      if ((pSource->getObjectClass() == OBJECT_TEMPLATE) &&
          ((pDestination->getObjectClass() == OBJECT_NODE) || (pDestination->getObjectClass() == OBJECT_CLUSTER) || (pDestination->getObjectClass() == OBJECT_MOBILEDEVICE)))
      {
         TCHAR szLockInfo[MAX_SESSION_NAME];
         BOOL bLockSucceed = FALSE;

         // Acquire DCI lock if needed
         if (!((Template *)pSource)->isLockedBySession(m_id))
         {
            bLockSucceed = ((Template *)pSource)->lockDCIList(m_id, m_sessionName, szLockInfo);
         }
         else
         {
            bLockSucceed = TRUE;
         }

         if (bLockSucceed)
         {
            // Check access rights
            if ((pSource->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ)) &&
                (pDestination->checkAccessRights(m_dwUserId, OBJECT_ACCESS_MODIFY)))
            {
               // Attempt to lock destination's DCI list
               if (((DataCollectionTarget *)pDestination)->lockDCIList(m_id, m_sessionName, szLockInfo))
               {
                  BOOL bErrors;

                  ObjectTransactionStart();
                  bErrors = ((Template *)pSource)->applyToTarget((DataCollectionTarget *)pDestination);
                  ObjectTransactionEnd();
                  ((Template *)pDestination)->unlockDCIList(m_id);
                  msg.setField(VID_RCC, bErrors ? RCC_DCI_COPY_ERRORS : RCC_SUCCESS);
               }
               else  // Destination's DCI list already locked by someone else
               {
                  msg.setField(VID_RCC, RCC_COMPONENT_LOCKED);
                  msg.setField(VID_LOCKED_BY, szLockInfo);
               }
            }
            else  // User doesn't have enough rights on object(s)
            {
               msg.setField(VID_RCC, RCC_ACCESS_DENIED);
            }

            ((Template *)pSource)->unlockDCIList(m_id);
         }
         else  // Source node DCI list not locked by this session
         {
            msg.setField(VID_RCC, RCC_COMPONENT_LOCKED);
            msg.setField(VID_LOCKED_BY, szLockInfo);
         }
      }
      else     // Object(s) is not a node
      {
         msg.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
      }
   }
   else  // No object(s) with given ID
   {
      msg.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Get user variable
 */
void ClientSession::getUserVariable(NXCPMessage *pRequest)
{
   NXCPMessage msg;
   TCHAR szVarName[MAX_VARIABLE_NAME], szQuery[MAX_VARIABLE_NAME + 256];
   DB_RESULT hResult;
   UINT32 dwUserId;

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(pRequest->getId());

   dwUserId = pRequest->isFieldExist(VID_USER_ID) ? pRequest->getFieldAsUInt32(VID_USER_ID) : m_dwUserId;
   if ((dwUserId == m_dwUserId) || (m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_USERS))
   {
      // Try to read variable from database
      pRequest->getFieldAsString(VID_NAME, szVarName, MAX_VARIABLE_NAME);
      _sntprintf(szQuery, MAX_VARIABLE_NAME + 256,
                 _T("SELECT var_value FROM user_profiles WHERE user_id=%d AND var_name='%s'"),
                 dwUserId, szVarName);
      hResult = DBSelect(g_hCoreDB, szQuery);
      if (hResult != NULL)
      {
         if (DBGetNumRows(hResult) > 0)
         {
            TCHAR *pszData;

            pszData = DBGetField(hResult, 0, 0, NULL, 0);
            DecodeSQLString(pszData);
            msg.setField(VID_RCC, RCC_SUCCESS);
            msg.setField(VID_VALUE, pszData);
            free(pszData);
         }
         else
         {
            msg.setField(VID_RCC, RCC_VARIABLE_NOT_FOUND);
         }
         DBFreeResult(hResult);
      }
      else
      {
         msg.setField(VID_RCC, RCC_DB_FAILURE);
      }
   }
   else
   {
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Set user variable
 */
void ClientSession::setUserVariable(NXCPMessage *pRequest)
{
   NXCPMessage msg;
   TCHAR szVarName[MAX_VARIABLE_NAME], szQuery[8192], *pszValue, *pszRawValue;
   DB_RESULT hResult;
   BOOL bExist = FALSE;
   UINT32 dwUserId;

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(pRequest->getId());

   dwUserId = pRequest->isFieldExist(VID_USER_ID) ? pRequest->getFieldAsUInt32(VID_USER_ID) : m_dwUserId;
   if ((dwUserId == m_dwUserId) || (m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_USERS))
   {
      // Check variable name
      pRequest->getFieldAsString(VID_NAME, szVarName, MAX_VARIABLE_NAME);
      if (IsValidObjectName(szVarName))
      {
         // Check if variable already exist in database
         _sntprintf(szQuery, MAX_VARIABLE_NAME + 256,
                    _T("SELECT var_name FROM user_profiles WHERE user_id=%d AND var_name='%s'"),
                    dwUserId, szVarName);
         hResult = DBSelect(g_hCoreDB, szQuery);
         if (hResult != NULL)
         {
            if (DBGetNumRows(hResult) > 0)
            {
               bExist = TRUE;
            }
            DBFreeResult(hResult);
         }

         // Update value in database
         pszRawValue = pRequest->getFieldAsString(VID_VALUE);
         pszValue = EncodeSQLString(pszRawValue);
         free(pszRawValue);
         if (bExist)
            _sntprintf(szQuery, 8192,
                       _T("UPDATE user_profiles SET var_value='%s' WHERE user_id=%d AND var_name='%s'"),
                       pszValue, dwUserId, szVarName);
         else
            _sntprintf(szQuery, 8192,
                       _T("INSERT INTO user_profiles (user_id,var_name,var_value) VALUES (%d,'%s','%s')"),
                       dwUserId, szVarName, pszValue);
         free(pszValue);
         if (DBQuery(g_hCoreDB, szQuery))
         {
            msg.setField(VID_RCC, RCC_SUCCESS);
         }
         else
         {
            msg.setField(VID_RCC, RCC_DB_FAILURE);
         }
      }
      else
      {
         msg.setField(VID_RCC, RCC_INVALID_OBJECT_NAME);
      }
   }
   else
   {
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Enum user variables
 */
void ClientSession::enumUserVariables(NXCPMessage *pRequest)
{
   NXCPMessage msg;
   TCHAR szPattern[MAX_VARIABLE_NAME], szQuery[256], szName[MAX_DB_STRING];
   UINT32 i, dwNumRows, dwNumVars, dwId, dwUserId;
   DB_RESULT hResult;

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(pRequest->getId());

   dwUserId = pRequest->isFieldExist(VID_USER_ID) ? pRequest->getFieldAsUInt32(VID_USER_ID) : m_dwUserId;
   if ((dwUserId == m_dwUserId) || (m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_USERS))
   {
      pRequest->getFieldAsString(VID_SEARCH_PATTERN, szPattern, MAX_VARIABLE_NAME);
      _sntprintf(szQuery, 256, _T("SELECT var_name FROM user_profiles WHERE user_id=%d"), dwUserId);
      hResult = DBSelect(g_hCoreDB, szQuery);
      if (hResult != NULL)
      {
         dwNumRows = DBGetNumRows(hResult);
         for(i = 0, dwNumVars = 0, dwId = VID_VARLIST_BASE; i < dwNumRows; i++)
         {
            DBGetField(hResult, i, 0, szName, MAX_DB_STRING);
            if (MatchString(szPattern, szName, FALSE))
            {
               dwNumVars++;
               msg.setField(dwId++, szName);
            }
         }
         msg.setField(VID_NUM_VARIABLES, dwNumVars);
         msg.setField(VID_RCC, RCC_SUCCESS);
         DBFreeResult(hResult);
      }
      else
      {
         msg.setField(VID_RCC, RCC_DB_FAILURE);
      }
   }
   else
   {
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Delete user variable(s)
 */
void ClientSession::deleteUserVariable(NXCPMessage *pRequest)
{
   NXCPMessage msg;
   TCHAR szVarName[MAX_VARIABLE_NAME], szQuery[MAX_VARIABLE_NAME + 256];
   UINT32 dwUserId;

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(pRequest->getId());

   dwUserId = pRequest->isFieldExist(VID_USER_ID) ? pRequest->getFieldAsUInt32(VID_USER_ID) : m_dwUserId;
   if ((dwUserId == m_dwUserId) || (m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_USERS))
   {
      // Try to delete variable from database
      pRequest->getFieldAsString(VID_NAME, szVarName, MAX_VARIABLE_NAME);
      TranslateStr(szVarName, _T("*"), _T("%"));
      _sntprintf(szQuery, MAX_VARIABLE_NAME + 256,
                 _T("DELETE FROM user_profiles WHERE user_id=%d AND var_name LIKE '%s'"),
                 dwUserId, szVarName);
      if (DBQuery(g_hCoreDB, szQuery))
      {
         msg.setField(VID_RCC, RCC_SUCCESS);
      }
      else
      {
         msg.setField(VID_RCC, RCC_DB_FAILURE);
      }
   }
   else
   {
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Copy or move user variable(s) to another user
 */
void ClientSession::copyUserVariable(NXCPMessage *pRequest)
{
   NXCPMessage msg;
   TCHAR szVarName[MAX_VARIABLE_NAME], szCurrVar[MAX_VARIABLE_NAME],
         szQuery[32768], *pszValue;
   UINT32 dwSrcUserId, dwDstUserId;
   int i, nRows;
   BOOL bMove, bExist;
   DB_RESULT hResult, hResult2;

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(pRequest->getId());

   if (m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_USERS)
   {
      dwSrcUserId = pRequest->isFieldExist(VID_USER_ID) ? pRequest->getFieldAsUInt32(VID_USER_ID) : m_dwUserId;
      dwDstUserId = pRequest->getFieldAsUInt32(VID_DST_USER_ID);
      bMove = (BOOL)pRequest->getFieldAsUInt16(VID_MOVE_FLAG);
      pRequest->getFieldAsString(VID_NAME, szVarName, MAX_VARIABLE_NAME);
      TranslateStr(szVarName, _T("*"), _T("%"));
      _sntprintf(szQuery, 8192,
                 _T("SELECT var_name,var_value FROM user_profiles WHERE user_id=%d AND var_name LIKE '%s'"),
                 dwSrcUserId, szVarName);
      hResult = DBSelect(g_hCoreDB, szQuery);
      if (hResult != NULL)
      {
         nRows = DBGetNumRows(hResult);
         for(i = 0; i < nRows; i++)
         {
            DBGetField(hResult, i, 0, szCurrVar, MAX_VARIABLE_NAME);

            // Check if variable already exist in database
            _sntprintf(szQuery, 32768,
                       _T("SELECT var_name FROM user_profiles WHERE user_id=%d AND var_name='%s'"),
                       dwDstUserId, szCurrVar);
            hResult2 = DBSelect(g_hCoreDB, szQuery);
            if (hResult2 != NULL)
            {
               bExist = (DBGetNumRows(hResult2) > 0);
               DBFreeResult(hResult2);
            }
            else
            {
               bExist = FALSE;
            }

            pszValue = DBGetField(hResult, i, 1, NULL, 0);
            if (bExist)
               _sntprintf(szQuery, 32768,
                          _T("UPDATE user_profiles SET var_value='%s' WHERE user_id=%d AND var_name='%s'"),
                          pszValue, dwDstUserId, szCurrVar);
            else
               _sntprintf(szQuery, 32768,
                          _T("INSERT INTO user_profiles (user_id,var_name,var_value) VALUES (%d,'%s','%s')"),
                          dwDstUserId, szCurrVar, pszValue);
            free(pszValue);
            DBQuery(g_hCoreDB, szQuery);

            if (bMove)
            {
               _sntprintf(szQuery, 32768,
                          _T("DELETE FROM user_profiles WHERE user_id=%d AND var_name='%s'"),
                          dwSrcUserId, szCurrVar);
               DBQuery(g_hCoreDB, szQuery);
            }
         }
         DBFreeResult(hResult);
         msg.setField(VID_RCC, RCC_SUCCESS);
      }
      else
      {
         msg.setField(VID_RCC, RCC_DB_FAILURE);
      }
   }
   else
   {
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Change object's zone
 */
void ClientSession::changeObjectZone(NXCPMessage *pRequest)
{
   NXCPMessage msg;

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(pRequest->getId());

   // Get object id and check prerequisites
   NetObj *object = FindObjectById(pRequest->getFieldAsUInt32(VID_OBJECT_ID));
   if (object != NULL)
   {
      if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_MODIFY))
      {
			if (object->getObjectClass() == OBJECT_NODE)
			{
				Node *node = (Node *)object;
				UINT32 zoneId = pRequest->getFieldAsUInt32(VID_ZONE_ID);
				Zone *zone = FindZoneByGUID(zoneId);
				if (zone != NULL)
				{
					// Check if target zone already have object with same primary IP
					if ((FindNodeByIP(zoneId, node->getIpAddress()) == NULL) &&
						 (FindSubnetByIP(zoneId, node->getIpAddress()) == NULL))
					{
						node->changeZone(zoneId);
						msg.setField(VID_RCC, RCC_SUCCESS);
					}
					else
					{
						msg.setField(VID_RCC, RCC_ADDRESS_IN_USE);
					}
				}
				else
				{
		         msg.setField(VID_RCC, RCC_INVALID_ZONE_ID);
				}
         }
         else
         {
	         msg.setField(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
         }
      }
      else
      {
         msg.setField(VID_RCC, RCC_ACCESS_DENIED);
      }
   }
   else
   {
      msg.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Setup encryption with client
 */
void ClientSession::setupEncryption(NXCPMessage *request)
{
   NXCPMessage msg;

#ifdef _WITH_ENCRYPTION
	m_dwEncryptionRqId = request->getId();
   m_dwEncryptionResult = RCC_TIMEOUT;
   if (m_condEncryptionSetup == INVALID_CONDITION_HANDLE)
      m_condEncryptionSetup = ConditionCreate(FALSE);

   // Send request for session key
	PrepareKeyRequestMsg(&msg, g_pServerKey, request->getFieldAsBoolean(VID_USE_X509_KEY_FORMAT));
	msg.setId(request->getId());
   sendMessage(&msg);
   msg.deleteAllFields();

   // Wait for encryption setup
   ConditionWait(m_condEncryptionSetup, 30000);

   // Send response
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(request->getId());
   msg.setField(VID_RCC, m_dwEncryptionResult);
#else    /* _WITH_ENCRYPTION not defined */
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(request->getId());
   msg.setField(VID_RCC, RCC_NO_ENCRYPTION_SUPPORT);
#endif

   sendMessage(&msg);
}

/**
 * Get agent's configuration file
 */
void ClientSession::getAgentConfig(NXCPMessage *pRequest)
{
   NXCPMessage msg;
   NetObj *object;
   UINT32 dwResult, size;
   TCHAR *pszConfig;

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(pRequest->getId());

   // Get object id and check prerequisites
   object = FindObjectById(pRequest->getFieldAsUInt32(VID_OBJECT_ID));
   if (object != NULL)
   {
      if (object->getObjectClass() == OBJECT_NODE)
      {
         if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
         {
            AgentConnection *pConn;

            pConn = ((Node *)object)->createAgentConnection();
            if (pConn != NULL)
            {
               dwResult = pConn->getConfigFile(&pszConfig, &size);
               delete pConn;
               switch(dwResult)
               {
                  case ERR_SUCCESS:
                     msg.setField(VID_RCC, RCC_SUCCESS);
                     msg.setField(VID_CONFIG_FILE, pszConfig);
                     free(pszConfig);
                     break;
                  case ERR_ACCESS_DENIED:
                     msg.setField(VID_RCC, RCC_ACCESS_DENIED);
                     break;
                  default:
                     msg.setField(VID_RCC, RCC_COMM_FAILURE);
                     break;
               }
            }
            else
            {
               msg.setField(VID_RCC, RCC_COMM_FAILURE);
            }
         }
         else
         {
            msg.setField(VID_RCC, RCC_ACCESS_DENIED);
         }
      }
      else
      {
         msg.setField(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
      }
   }
   else
   {
      msg.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Update agent's configuration file
 */
void ClientSession::updateAgentConfig(NXCPMessage *pRequest)
{
   NXCPMessage msg;
   NetObj *object;
   UINT32 dwResult;
   TCHAR *pszConfig;

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(pRequest->getId());

   // Get object id and check prerequisites
   object = FindObjectById(pRequest->getFieldAsUInt32(VID_OBJECT_ID));
   if (object != NULL)
   {
      if (object->getObjectClass() == OBJECT_NODE)
      {
         if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_CONTROL))
         {
            AgentConnection *pConn;

            pConn = ((Node *)object)->createAgentConnection();
            if (pConn != NULL)
            {
               pszConfig = pRequest->getFieldAsString(VID_CONFIG_FILE);
               dwResult = pConn->updateConfigFile(pszConfig);
               free(pszConfig);

               if ((pRequest->getFieldAsUInt16(VID_APPLY_FLAG) != 0) &&
                   (dwResult == ERR_SUCCESS))
               {
                  dwResult = pConn->execAction(_T("Agent.Restart"), 0, NULL);
               }

               switch(dwResult)
               {
                  case ERR_SUCCESS:
                     msg.setField(VID_RCC, RCC_SUCCESS);
                     break;
                  case ERR_ACCESS_DENIED:
                     msg.setField(VID_RCC, RCC_ACCESS_DENIED);
                     break;
                  case ERR_IO_FAILURE:
                     msg.setField(VID_RCC, RCC_IO_ERROR);
                     break;
                  case ERR_MALFORMED_COMMAND:
                     msg.setField(VID_RCC, RCC_INTERNAL_ERROR);
                     break;
                  default:
                     msg.setField(VID_RCC, RCC_COMM_FAILURE);
                     break;
               }
               delete pConn;
            }
            else
            {
               msg.setField(VID_RCC, RCC_COMM_FAILURE);
            }
         }
         else
         {
            msg.setField(VID_RCC, RCC_ACCESS_DENIED);
         }
      }
      else
      {
         msg.setField(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
      }
   }
   else
   {
      msg.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Action execution data
 */
class ActionExecutionData
{
public:
   ClientSession *m_session;
   NXCPMessage *m_msg;

   ActionExecutionData(ClientSession *session, UINT32 requestId)
   {
      m_session = session;
      m_msg = new NXCPMessage;
      m_msg->setId(requestId);
   }

   ~ActionExecutionData()
   {
      delete m_msg;
   }
};

/**
 * Action execution callback
 */
static void ActionExecuteCallback(ActionCallbackEvent e, const TCHAR *text, void *arg)
{
   ActionExecutionData *data = (ActionExecutionData *)arg;
   switch(e)
   {
      case ACE_CONNECTED:
         data->m_msg->setCode(CMD_REQUEST_COMPLETED);
         data->m_msg->setField(VID_RCC, RCC_SUCCESS);
         break;
      case ACE_DISCONNECTED:
         data->m_msg->deleteAllFields();
         data->m_msg->setCode(CMD_COMMAND_OUTPUT);
         data->m_msg->setEndOfSequence();
         break;
      case ACE_DATA:
         data->m_msg->deleteAllFields();
         data->m_msg->setCode(CMD_COMMAND_OUTPUT);
         data->m_msg->setField(VID_MESSAGE, text);
         break;
   }
   data->m_session->sendMessage(data->m_msg);
}

/**
 * Execute action on agent
 */
void ClientSession::executeAction(NXCPMessage *pRequest)
{
   NXCPMessage msg;

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(pRequest->getId());

   // Get object id and check prerequisites
   NetObj *object = FindObjectById(pRequest->getFieldAsUInt32(VID_OBJECT_ID));
   if (object != NULL)
   {
      if (object->getObjectClass() == OBJECT_NODE)
      {
         if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_CONTROL))
         {
            AgentConnection *pConn;

            pConn = ((Node *)object)->createAgentConnection();
            if (pConn != NULL)
            {
               TCHAR action[MAX_PARAM_NAME];
               pRequest->getFieldAsString(VID_ACTION_NAME, action, MAX_PARAM_NAME);

               UINT32 rcc;
               bool withOutput = pRequest->getFieldAsBoolean(VID_RECEIVE_OUTPUT);
               if (withOutput)
               {
                  ActionExecutionData data(this, pRequest->getId());
                  rcc = pConn->execAction(action, 0, NULL, true, ActionExecuteCallback, &data);
               }
               else
               {
                  rcc = pConn->execAction(action, 0, NULL);
               }

               switch(rcc)
               {
                  case ERR_SUCCESS:
                     msg.setField(VID_RCC, RCC_SUCCESS);
                     break;
                  case ERR_ACCESS_DENIED:
                     msg.setField(VID_RCC, RCC_ACCESS_DENIED);
                     break;
                  case ERR_IO_FAILURE:
                     msg.setField(VID_RCC, RCC_IO_ERROR);
                     break;
                  case ERR_EXEC_FAILED:
                     msg.setField(VID_RCC, RCC_EXEC_FAILED);
                     break;
                  default:
                     msg.setField(VID_RCC, RCC_COMM_FAILURE);
                     break;
               }
               delete pConn;
            }
            else
            {
               msg.setField(VID_RCC, RCC_COMM_FAILURE);
            }
         }
         else
         {
            msg.setField(VID_RCC, RCC_ACCESS_DENIED);
         }
      }
      else
      {
         msg.setField(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
      }
   }
   else
   {
      msg.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Send tool list to client
 */
void ClientSession::getObjectTools(UINT32 requestId)
{
   NXCPMessage msg;
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(requestId);

   msg.setField(VID_RCC, GetObjectToolsIntoMessage(&msg, m_dwUserId, checkSysAccessRights(SYSTEM_ACCESS_MANAGE_TOOLS)));

   sendMessage(&msg);
}

/**
 * Send tool list to client
 */
void ClientSession::getObjectToolDetails(NXCPMessage *request)
{
   NXCPMessage msg;
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(request->getId());

   if (checkSysAccessRights(SYSTEM_ACCESS_MANAGE_TOOLS))
   {
      msg.setField(VID_RCC, GetObjectToolDetailsIntoMessage(request->getFieldAsUInt32(VID_TOOL_ID), &msg));
   }
   else
   {
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);
      WriteAuditLog(AUDIT_SYSCFG, FALSE, m_dwUserId, m_workstation, m_id, 0, _T("Access denied on reading object tool details"));
   }

   sendMessage(&msg);
   return;
}

/**
 * Update object tool
 */
void ClientSession::updateObjectTool(NXCPMessage *pRequest)
{
   NXCPMessage msg;

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(pRequest->getId());

   // Check user rights
   if (m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_TOOLS)
   {
      msg.setField(VID_RCC, UpdateObjectToolFromMessage(pRequest));
   }
   else
   {
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Delete object tool
 */
void ClientSession::deleteObjectTool(NXCPMessage *pRequest)
{
   NXCPMessage msg;
   UINT32 dwToolId;

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(pRequest->getId());

   // Check user rights
   if (m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_TOOLS)
   {
      dwToolId = pRequest->getFieldAsUInt32(VID_TOOL_ID);
      msg.setField(VID_RCC, DeleteObjectToolFromDB(dwToolId));
   }
   else
   {
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Change Object Tool status (enabled/disabled)
 */
void ClientSession::changeObjectToolStatus(NXCPMessage *pRequest)
{
   NXCPMessage msg;
   UINT32 toolID, enable;

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(pRequest->getId());

   // Check user rights
   if (m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_TOOLS)
   {
      toolID = pRequest->getFieldAsUInt32(VID_TOOL_ID);
      enable = pRequest->getFieldAsUInt32(VID_STATE);
      msg.setField(VID_RCC, ChangeObjectToolStatus(toolID, enable == 0 ? false : true));
   }
   else
   {
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Generate ID for new object tool
 */
void ClientSession::generateObjectToolId(UINT32 dwRqId)
{
   NXCPMessage msg;

   // Prepare reply message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(dwRqId);

   // Check access rights
   if (checkSysAccessRights(SYSTEM_ACCESS_MANAGE_TOOLS))
   {
      msg.setField(VID_TOOL_ID, CreateUniqueId(IDG_OBJECT_TOOL));
   }
   else
   {
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Execute table tool (either SNMP or agent table)
 */
void ClientSession::execTableTool(NXCPMessage *pRequest)
{
   NXCPMessage msg;
   UINT32 dwToolId;
   NetObj *object;

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(pRequest->getId());

   // Check if tool exist and has correct type
   dwToolId = pRequest->getFieldAsUInt32(VID_TOOL_ID);
   if (IsTableTool(dwToolId))
   {
      // Check access
      if (CheckObjectToolAccess(dwToolId, m_dwUserId))
      {
         object = FindObjectById(pRequest->getFieldAsUInt32(VID_OBJECT_ID));
         if (object != NULL)
         {
            if (object->getObjectClass() == OBJECT_NODE)
            {
               msg.setField(VID_RCC,
                               ExecuteTableTool(dwToolId, (Node *)object,
                                                pRequest->getId(), this));
            }
            else
            {
               msg.setField(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
            }
         }
         else
         {
            msg.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
         }
      }
      else
      {
         msg.setField(VID_RCC, RCC_ACCESS_DENIED);
      }
   }
   else
   {
      msg.setField(VID_RCC, RCC_INVALID_TOOL_ID);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Change current subscription
 */
void ClientSession::changeSubscription(NXCPMessage *pRequest)
{
   NXCPMessage msg;
   UINT32 dwFlags;

   dwFlags = pRequest->getFieldAsUInt32(VID_FLAGS);

   if (pRequest->getFieldAsUInt16(VID_OPERATION) != 0)
   {
      m_dwActiveChannels |= dwFlags;   // Subscribe
   }
   else
   {
      m_dwActiveChannels &= ~dwFlags;   // Unsubscribe
   }

   // Send response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(pRequest->getId());
   msg.setField(VID_RCC, RCC_SUCCESS);
   sendMessage(&msg);
}

/**
 * Callback for counting DCIs in the system
 */
static void DciCountCallback(NetObj *object, void *data)
{
	*((UINT32 *)data) += ((Node *)object)->getItemCount();
}

/**
 * Send server statistics
 */
void ClientSession::sendServerStats(UINT32 dwRqId)
{
   NXCPMessage msg;
#ifdef _WIN32
   PROCESS_MEMORY_COUNTERS mc;
#endif

   // Prepare response
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(dwRqId);
   msg.setField(VID_RCC, RCC_SUCCESS);

   // Server version, etc.
   msg.setField(VID_SERVER_VERSION, NETXMS_VERSION_STRING);
   msg.setField(VID_SERVER_UPTIME, (UINT32)(time(NULL) - g_serverStartTime));

   // Number of objects and DCIs
	UINT32 dciCount = 0;
	g_idxNodeById.forEach(DciCountCallback, &dciCount);
   msg.setField(VID_NUM_ITEMS, dciCount);
	msg.setField(VID_NUM_OBJECTS, (UINT32)g_idxObjectById.size());
	msg.setField(VID_NUM_NODES, (UINT32)g_idxNodeById.size());

   // Client sessions
   msg.setField(VID_NUM_SESSIONS, (UINT32)GetSessionCount());

   // Alarms
   GetAlarmStats(&msg);

   // Process info
#ifdef _WIN32
   mc.cb = sizeof(PROCESS_MEMORY_COUNTERS);
   if (GetProcessMemoryInfo(GetCurrentProcess(), &mc, sizeof(PROCESS_MEMORY_COUNTERS)))
   {
      msg.setField(VID_NETXMSD_PROCESS_WKSET, (UINT32)(mc.WorkingSetSize / 1024));
      msg.setField(VID_NETXMSD_PROCESS_VMSIZE, (UINT32)(mc.PagefileUsage / 1024));
   }
#endif

	// Queues
	msg.setField(VID_QSIZE_DCI_POLLER, g_dataCollectionQueue.size());
	msg.setField(VID_QSIZE_DCI_CACHE_LOADER, g_dciCacheLoaderQueue.size());
	msg.setField(VID_QSIZE_DBWRITER, g_dbWriterQueue->size());
	msg.setField(VID_QSIZE_EVENT, g_pEventQueue->size());
	msg.setField(VID_QSIZE_NODE_POLLER, g_nodePollerQueue.size());

   // Send response
   sendMessage(&msg);
}

/**
 * Send script list
 */
void ClientSession::sendScriptList(UINT32 dwRqId)
{
   NXCPMessage msg;
   DB_RESULT hResult;
   UINT32 i, dwNumScripts, dwId;
   TCHAR szBuffer[MAX_DB_STRING];

   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(dwRqId);
   if (m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_SCRIPTS)
   {
      hResult = DBSelect(g_hCoreDB, _T("SELECT script_id,script_name FROM script_library"));
      if (hResult != NULL)
      {
         msg.setField(VID_RCC, RCC_SUCCESS);
         dwNumScripts = DBGetNumRows(hResult);
         msg.setField(VID_NUM_SCRIPTS, dwNumScripts);
         for(i = 0, dwId = VID_SCRIPT_LIST_BASE; i < dwNumScripts; i++)
         {
            msg.setField(dwId++, DBGetFieldULong(hResult, i, 0));
            msg.setField(dwId++, DBGetField(hResult, i, 1, szBuffer, MAX_DB_STRING));
         }
         DBFreeResult(hResult);
      }
      else
      {
         msg.setField(VID_RCC, RCC_DB_FAILURE);
      }
   }
   else
   {
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);
   }
   sendMessage(&msg);
}

/**
 * Send script
 */
void ClientSession::sendScript(NXCPMessage *pRequest)
{
   NXCPMessage msg;
   DB_RESULT hResult;
   UINT32 dwScriptId;
   TCHAR *pszCode, szQuery[256];

   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(pRequest->getId());
   if (m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_SCRIPTS)
   {
      dwScriptId = pRequest->getFieldAsUInt32(VID_SCRIPT_ID);
      _sntprintf(szQuery, 256, _T("SELECT script_name,script_code FROM script_library WHERE script_id=%d"), dwScriptId);
      hResult = DBSelect(g_hCoreDB, szQuery);
      if (hResult != NULL)
      {
         if (DBGetNumRows(hResult) > 0)
         {
				TCHAR name[MAX_DB_STRING];

            msg.setField(VID_NAME, DBGetField(hResult, 0, 0, name, MAX_DB_STRING));

				pszCode = DBGetField(hResult, 0, 1, NULL, 0);
            msg.setField(VID_SCRIPT_CODE, pszCode);
            free(pszCode);
         }
         else
         {
            msg.setField(VID_RCC, RCC_INVALID_SCRIPT_ID);
         }
         DBFreeResult(hResult);
      }
      else
      {
         msg.setField(VID_RCC, RCC_DB_FAILURE);
      }
   }
   else
   {
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);
   }
   sendMessage(&msg);
}

/**
 * Update script in library
 */
void ClientSession::updateScript(NXCPMessage *pRequest)
{
   NXCPMessage msg;

   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(pRequest->getId());
   if (m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_SCRIPTS)
   {
      TCHAR scriptName[MAX_DB_STRING];
      pRequest->getFieldAsString(VID_NAME, scriptName, MAX_DB_STRING);
      if (IsValidScriptName(scriptName))
      {
         TCHAR *scriptSource = pRequest->getFieldAsString(VID_SCRIPT_CODE);
         if (scriptSource != NULL)
         {
            UINT32 scriptId = pRequest->getFieldAsUInt32(VID_SCRIPT_ID);

            DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

            DB_STATEMENT hStmt;
            if (scriptId == 0)
            {
               // New script
               scriptId = CreateUniqueId(IDG_SCRIPT);
               hStmt = DBPrepare(hdb, _T("INSERT INTO script_library (script_name,script_code,script_id) VALUES (?,?,?)"));
            }
            else
            {
               hStmt = DBPrepare(hdb, _T("UPDATE script_library SET script_name=?,script_code=? WHERE script_id=?"));
            }

            if (hStmt != NULL)
            {
               DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, scriptName, DB_BIND_STATIC);
               DBBind(hStmt, 2, DB_SQLTYPE_TEXT, scriptSource, DB_BIND_STATIC);
               DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, scriptId);

               if (DBExecute(hStmt))
               {
                  ReloadScript(scriptId);
                  msg.setField(VID_RCC, RCC_SUCCESS);
                  msg.setField(VID_SCRIPT_ID, scriptId);
               }
               else
               {
                  msg.setField(VID_RCC, RCC_DB_FAILURE);
               }

               DBFreeStatement(hStmt);
            }
            else
            {
               msg.setField(VID_RCC, RCC_DB_FAILURE);
            }

            free(scriptSource);
            DBConnectionPoolReleaseConnection(hdb);
         }
         else
         {
            msg.setField(VID_RCC, RCC_INVALID_REQUEST);
         }
      }
      else
      {
         msg.setField(VID_RCC, RCC_INVALID_SCRIPT_NAME);
      }
   }
   else
   {
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);
   }
   sendMessage(&msg);
}

/**
 * Rename script
 */
void ClientSession::renameScript(NXCPMessage *pRequest)
{
   NXCPMessage msg;
   TCHAR szQuery[4096], szName[MAX_DB_STRING];
   UINT32 dwScriptId;

   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(pRequest->getId());
   if (m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_SCRIPTS)
   {
      dwScriptId = pRequest->getFieldAsUInt32(VID_SCRIPT_ID);
      pRequest->getFieldAsString(VID_NAME, szName, MAX_DB_STRING);
      if (IsValidScriptName(szName))
      {
         if (IsValidScriptId(dwScriptId))
         {
            _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("UPDATE script_library SET script_name=%s WHERE script_id=%d"),
                      (const TCHAR *)DBPrepareString(g_hCoreDB, szName), dwScriptId);
            if (DBQuery(g_hCoreDB, szQuery))
            {
               ReloadScript(dwScriptId);
               msg.setField(VID_RCC, RCC_SUCCESS);
            }
            else
            {
               msg.setField(VID_RCC, RCC_DB_FAILURE);
            }
         }
         else
         {
            msg.setField(VID_RCC, RCC_INVALID_SCRIPT_ID);
         }
      }
      else
      {
         msg.setField(VID_RCC, RCC_INVALID_SCRIPT_NAME);
      }
   }
   else
   {
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);
   }
   sendMessage(&msg);
}

/**
 * Delete script from library
 */
void ClientSession::deleteScript(NXCPMessage *pRequest)
{
   NXCPMessage msg;
   TCHAR szQuery[256];
   UINT32 dwScriptId;

   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(pRequest->getId());
   if (m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_SCRIPTS)
   {
      dwScriptId = pRequest->getFieldAsUInt32(VID_SCRIPT_ID);
      if (IsValidScriptId(dwScriptId))
      {
         _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("DELETE FROM script_library WHERE script_id=%d"), dwScriptId);
         if (DBQuery(g_hCoreDB, szQuery))
         {
            g_pScriptLibrary->lock();
            g_pScriptLibrary->deleteScript(dwScriptId);
            g_pScriptLibrary->unlock();
            msg.setField(VID_RCC, RCC_SUCCESS);
         }
         else
         {
            msg.setField(VID_RCC, RCC_DB_FAILURE);
         }
      }
      else
      {
         msg.setField(VID_RCC, RCC_INVALID_SCRIPT_ID);
      }
   }
   else
   {
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);
   }
   sendMessage(&msg);
}

/**
 * Copy session data to message
 */
static void CopySessionData(ClientSession *pSession, void *pArg)
{
   UINT32 dwId, dwIndex;

   dwIndex = ((NXCPMessage *)pArg)->getFieldAsUInt32(VID_NUM_SESSIONS);
   ((NXCPMessage *)pArg)->setField(VID_NUM_SESSIONS, dwIndex + 1);

   dwId = VID_SESSION_DATA_BASE + dwIndex * 100;
   ((NXCPMessage *)pArg)->setField(dwId++, (UINT32)pSession->getId());
   ((NXCPMessage *)pArg)->setField(dwId++, (WORD)pSession->getCipher());
   ((NXCPMessage *)pArg)->setField(dwId++, (TCHAR *)pSession->getSessionName());
   ((NXCPMessage *)pArg)->setField(dwId++, (TCHAR *)pSession->getClientInfo());
   ((NXCPMessage *)pArg)->setField(dwId++, (TCHAR *)pSession->getLoginName());
}

/**
 * Send list of connected client sessions
 */
void ClientSession::SendSessionList(UINT32 dwRqId)
{
   NXCPMessage msg;

   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(dwRqId);
   if (m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_SESSIONS)
   {
      msg.setField(VID_NUM_SESSIONS, (UINT32)0);
      EnumerateClientSessions(CopySessionData, &msg);
      msg.setField(VID_RCC, RCC_SUCCESS);
   }
   else
   {
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);
   }
   sendMessage(&msg);
}


//
// Forcibly terminate client's session
//

void ClientSession::KillSession(NXCPMessage *pRequest)
{
   NXCPMessage msg;

   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(pRequest->getId());
   if (m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_SESSIONS)
   {
   }
   else
   {
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);
   }
   sendMessage(&msg);
}

/**
 * Handler for new syslog messages
 */
void ClientSession::onSyslogMessage(NX_SYSLOG_RECORD *pRec)
{
   UPDATE_INFO *pUpdate;
   if (isAuthenticated() && isSubscribed(NXC_CHANNEL_SYSLOG) && (m_dwSystemAccess & SYSTEM_ACCESS_VIEW_SYSLOG))
   {
      NetObj *object = FindObjectById(pRec->dwSourceObject);
      //If can't find object - just send to all events, if object found send to thous who have rights
      if (object == NULL || object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ_ALARMS))
      {
         pUpdate = (UPDATE_INFO *)malloc(sizeof(UPDATE_INFO));
         pUpdate->dwCategory = INFO_CAT_SYSLOG_MSG;
         pUpdate->pData = nx_memdup(pRec, sizeof(NX_SYSLOG_RECORD));
         m_pUpdateQueue->put(pUpdate);
      }
   }
}

/**
 * Get latest syslog records
 */
void ClientSession::sendSyslog(NXCPMessage *pRequest)
{
   NXCPMessage msg;
   UINT32 dwMaxRecords, dwNumRows, dwId;
   DB_RESULT hTempResult;
   DB_ASYNC_RESULT hResult;
   TCHAR szQuery[1024], szBuffer[1024];
   WORD wRecOrder;

   wRecOrder = ((g_dbSyntax == DB_SYNTAX_MSSQL) || (g_dbSyntax == DB_SYNTAX_ORACLE)) ? RECORD_ORDER_REVERSED : RECORD_ORDER_NORMAL;
   dwMaxRecords = pRequest->getFieldAsUInt32(VID_MAX_RECORDS);

   // Prepare confirmation message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(pRequest->getId());

   MutexLock(m_mutexSendSyslog);

   // Retrieve events from database
   switch(g_dbSyntax)
   {
      case DB_SYNTAX_MYSQL:
      case DB_SYNTAX_PGSQL:
      case DB_SYNTAX_SQLITE:
         dwNumRows = 0;
         hTempResult = DBSelect(g_hCoreDB, _T("SELECT count(*) FROM syslog"));
         if (hTempResult != NULL)
         {
            if (DBGetNumRows(hTempResult) > 0)
            {
               dwNumRows = DBGetFieldULong(hTempResult, 0, 0);
            }
            DBFreeResult(hTempResult);
         }
         _sntprintf(szQuery, 1024,
                    _T("SELECT msg_id,msg_timestamp,facility,severity,")
                    _T("source_object_id,hostname,msg_tag,msg_text FROM syslog ")
                    _T("ORDER BY msg_id LIMIT %u OFFSET %u"),
                    dwMaxRecords, dwNumRows - min(dwNumRows, dwMaxRecords));
         break;
      case DB_SYNTAX_MSSQL:
         _sntprintf(szQuery, 1024,
                    _T("SELECT TOP %d msg_id,msg_timestamp,facility,severity,")
                    _T("source_object_id,hostname,msg_tag,msg_text FROM syslog ")
                    _T("ORDER BY msg_id DESC"), dwMaxRecords);
         break;
      case DB_SYNTAX_ORACLE:
         _sntprintf(szQuery, 1024,
                    _T("SELECT msg_id,msg_timestamp,facility,severity,")
                    _T("source_object_id,hostname,msg_tag,msg_text FROM syslog ")
                    _T("WHERE ROWNUM <= %u ORDER BY msg_id DESC"), dwMaxRecords);
         break;
      case DB_SYNTAX_DB2:
         _sntprintf(szQuery, 1024,
                    _T("SELECT msg_id,msg_timestamp,facility,severity,")
                    _T("source_object_id,hostname,msg_tag,msg_text FROM syslog ")
                    _T("ORDER BY msg_id DESC FETCH FIRST %u ROWS ONLY"), dwMaxRecords);
         break;
      default:
         szQuery[0] = 0;
         break;
   }
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   hResult = DBAsyncSelect(hdb, szQuery);
   if (hResult != NULL)
   {
		msg.setField(VID_RCC, RCC_SUCCESS);
		sendMessage(&msg);
		msg.deleteAllFields();
		msg.setCode(CMD_SYSLOG_RECORDS);

      // Send records, up to 10 per message
      for(dwId = VID_SYSLOG_MSG_BASE, dwNumRows = 0; DBFetch(hResult); dwNumRows++)
      {
         if (dwNumRows == 10)
         {
            msg.setField(VID_NUM_RECORDS, dwNumRows);
            msg.setField(VID_RECORDS_ORDER, wRecOrder);
            sendMessage(&msg);
            msg.deleteAllFields();
            dwNumRows = 0;
            dwId = VID_SYSLOG_MSG_BASE;
         }
         msg.setField(dwId++, DBGetFieldAsyncUInt64(hResult, 0));
         msg.setField(dwId++, DBGetFieldAsyncULong(hResult, 1));
         msg.setField(dwId++, (WORD)DBGetFieldAsyncLong(hResult, 2));
         msg.setField(dwId++, (WORD)DBGetFieldAsyncLong(hResult, 3));
         msg.setField(dwId++, DBGetFieldAsyncULong(hResult, 4));
         msg.setField(dwId++, DBGetFieldAsync(hResult, 5, szBuffer, 1024));
         msg.setField(dwId++, DBGetFieldAsync(hResult, 6, szBuffer, 1024));
         msg.setField(dwId++, DBGetFieldAsync(hResult, 7, szBuffer, 1024));
      }
      DBFreeAsyncResult(hResult);

      // Send remaining records with End-Of-Sequence notification
      msg.setField(VID_NUM_RECORDS, dwNumRows);
      msg.setField(VID_RECORDS_ORDER, wRecOrder);
      msg.setEndOfSequence();
      sendMessage(&msg);
   }
   else
   {
		msg.setField(VID_RCC, RCC_DB_FAILURE);
		sendMessage(&msg);
	}

   DBConnectionPoolReleaseConnection(hdb);
   MutexUnlock(m_mutexSendSyslog);
}

/**
 * Handler for new traps
 */
void ClientSession::onNewSNMPTrap(NXCPMessage *pMsg)
{
   UPDATE_INFO *pUpdate;
   if (isAuthenticated() && isSubscribed(NXC_CHANNEL_SNMP_TRAPS) && (m_dwSystemAccess & SYSTEM_ACCESS_VIEW_TRAP_LOG))
   {
      NetObj *object = FindObjectById(pMsg->getFieldAsUInt32(VID_TRAP_LOG_MSG_BASE + 3));
      //If can't find object - just send to all events, if object found send to thous who have rights
      if (object == NULL || object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ_ALARMS))
      {
         pUpdate = (UPDATE_INFO *)malloc(sizeof(UPDATE_INFO));
         pUpdate->dwCategory = INFO_CAT_SNMP_TRAP;
         pUpdate->pData = new NXCPMessage(pMsg);
         m_pUpdateQueue->put(pUpdate);
      }
   }
}

/**
 * Send collected trap log
 */
void ClientSession::SendTrapLog(NXCPMessage *pRequest)
{
   NXCPMessage msg;
   TCHAR szBuffer[4096], szQuery[1024];
   UINT32 dwId, dwNumRows, dwMaxRecords;
   DB_RESULT hTempResult;
   DB_ASYNC_RESULT hResult;
   WORD wRecOrder;

   wRecOrder = ((g_dbSyntax == DB_SYNTAX_MSSQL) || (g_dbSyntax == DB_SYNTAX_ORACLE)) ? RECORD_ORDER_REVERSED : RECORD_ORDER_NORMAL;
   dwMaxRecords = pRequest->getFieldAsUInt32(VID_MAX_RECORDS);

   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(pRequest->getId());

   if (m_dwSystemAccess & SYSTEM_ACCESS_VIEW_TRAP_LOG)
   {
      msg.setField(VID_RCC, RCC_SUCCESS);
      sendMessage(&msg);
      msg.deleteAllFields();
      msg.setCode(CMD_TRAP_LOG_RECORDS);

      MutexLock(m_mutexSendTrapLog);

      // Retrieve trap log records from database
      switch(g_dbSyntax)
      {
         case DB_SYNTAX_MYSQL:
         case DB_SYNTAX_PGSQL:
         case DB_SYNTAX_SQLITE:
            dwNumRows = 0;
            hTempResult = DBSelect(g_hCoreDB, _T("SELECT count(*) FROM snmp_trap_log"));
            if (hTempResult != NULL)
            {
               if (DBGetNumRows(hTempResult) > 0)
               {
                  dwNumRows = DBGetFieldULong(hTempResult, 0, 0);
               }
               DBFreeResult(hTempResult);
            }
            _sntprintf(szQuery, 1024,
                       _T("SELECT trap_id,trap_timestamp,ip_addr,object_id,")
                       _T("trap_oid,trap_varlist FROM snmp_trap_log ")
                       _T("ORDER BY trap_id LIMIT %u OFFSET %u"),
                       dwMaxRecords, dwNumRows - min(dwNumRows, dwMaxRecords));
            break;
         case DB_SYNTAX_MSSQL:
            _sntprintf(szQuery, 1024,
                       _T("SELECT TOP %u trap_id,trap_timestamp,ip_addr,object_id,")
                       _T("trap_oid,trap_varlist FROM snmp_trap_log ")
                       _T("ORDER BY trap_id DESC"), dwMaxRecords);
            break;
         case DB_SYNTAX_ORACLE:
            _sntprintf(szQuery, 1024,
                       _T("SELECT trap_id,trap_timestamp,ip_addr,object_id,")
                       _T("trap_oid,trap_varlist FROM snmp_trap_log ")
                       _T("WHERE ROWNUM <= %u ORDER BY trap_id DESC"),
                       dwMaxRecords);
         case DB_SYNTAX_DB2:
            _sntprintf(szQuery, 1024,
                       _T("SELECT trap_id,trap_timestamp,ip_addr,object_id,")
                       _T("trap_oid,trap_varlist FROM snmp_trap_log ")
                       _T("ORDER BY trap_id DESC FETCH FIRST %u ROWS ONLY"),
                       dwMaxRecords);
            break;
         default:
            szQuery[0] = 0;
            break;
      }

      DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
      hResult = DBAsyncSelect(hdb, szQuery);
      if (hResult != NULL)
      {
         // Send events, one per message
         for(dwId = VID_TRAP_LOG_MSG_BASE, dwNumRows = 0; DBFetch(hResult); dwNumRows++)
         {
            if (dwNumRows == 10)
            {
               msg.setField(VID_NUM_RECORDS, dwNumRows);
               msg.setField(VID_RECORDS_ORDER, wRecOrder);
               sendMessage(&msg);
               msg.deleteAllFields();
               dwNumRows = 0;
               dwId = VID_TRAP_LOG_MSG_BASE;
            }
            msg.setField(dwId++, DBGetFieldAsyncUInt64(hResult, 0));
            msg.setField(dwId++, DBGetFieldAsyncULong(hResult, 1));
            msg.setField(dwId++, DBGetFieldAsyncIPAddr(hResult, 2));
            msg.setField(dwId++, DBGetFieldAsyncULong(hResult, 3));
            msg.setField(dwId++, DBGetFieldAsync(hResult, 4, szBuffer, 256));
            msg.setField(dwId++, DBGetFieldAsync(hResult, 5, szBuffer, 4096));
         }
         DBFreeAsyncResult(hResult);

         // Send remaining records with End-Of-Sequence notification
         msg.setField(VID_NUM_RECORDS, dwNumRows);
         msg.setField(VID_RECORDS_ORDER, wRecOrder);
         msg.setEndOfSequence();
      }

      DBConnectionPoolReleaseConnection(hdb);
      MutexUnlock(m_mutexSendTrapLog);
   }
   else
   {
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);
   }
   sendMessage(&msg);
}

/**
 *SNMP walker thread's startup parameters
 */
typedef struct
{
   ClientSession *pSession;
   UINT32 dwRqId;
   NetObj *object;
   TCHAR szBaseOID[MAX_OID_LEN * 4];
} WALKER_THREAD_ARGS;

/**
 * Arguments for SnmpEnumerate callback
 */
typedef struct
{
   NXCPMessage *pMsg;
   UINT32 dwId;
   UINT32 dwNumVars;
   ClientSession *pSession;
} WALKER_ENUM_CALLBACK_ARGS;

/**
 * SNMP walker enumeration callback
 */
static UINT32 WalkerCallback(UINT32 dwVersion, SNMP_Variable *pVar, SNMP_Transport *pTransport, void *pArg)
{
   NXCPMessage *pMsg = ((WALKER_ENUM_CALLBACK_ARGS *)pArg)->pMsg;
   TCHAR szBuffer[4096];
	bool convertToHex = true;

	pVar->getValueAsPrintableString(szBuffer, 4096, &convertToHex);
   pMsg->setField(((WALKER_ENUM_CALLBACK_ARGS *)pArg)->dwId++, (TCHAR *)pVar->getName()->getValueAsText());
	pMsg->setField(((WALKER_ENUM_CALLBACK_ARGS *)pArg)->dwId++, convertToHex ? (UINT32)0xFFFF : pVar->getType());
   pMsg->setField(((WALKER_ENUM_CALLBACK_ARGS *)pArg)->dwId++, szBuffer);
   ((WALKER_ENUM_CALLBACK_ARGS *)pArg)->dwNumVars++;
   if (((WALKER_ENUM_CALLBACK_ARGS *)pArg)->dwNumVars == 50)
   {
      pMsg->setField(VID_NUM_VARIABLES, ((WALKER_ENUM_CALLBACK_ARGS *)pArg)->dwNumVars);
      ((WALKER_ENUM_CALLBACK_ARGS *)pArg)->pSession->sendMessage(pMsg);
      ((WALKER_ENUM_CALLBACK_ARGS *)pArg)->dwNumVars = 0;
      ((WALKER_ENUM_CALLBACK_ARGS *)pArg)->dwId = VID_SNMP_WALKER_DATA_BASE;
      pMsg->deleteAllFields();
   }
   return SNMP_ERR_SUCCESS;
}

/**
 * SNMP walker thread
 */
static void WalkerThread(void *pArg)
{
   NXCPMessage msg;
   WALKER_ENUM_CALLBACK_ARGS args;

   msg.setCode(CMD_SNMP_WALK_DATA);
   msg.setId(((WALKER_THREAD_ARGS *)pArg)->dwRqId);

   args.pMsg = &msg;
   args.dwId = VID_SNMP_WALKER_DATA_BASE;
   args.dwNumVars = 0;
   args.pSession = ((WALKER_THREAD_ARGS *)pArg)->pSession;
   ((Node *)(((WALKER_THREAD_ARGS *)pArg)->object))->callSnmpEnumerate(((WALKER_THREAD_ARGS *)pArg)->szBaseOID, WalkerCallback, &args);
   msg.setField(VID_NUM_VARIABLES, args.dwNumVars);
   msg.setEndOfSequence();
   ((WALKER_THREAD_ARGS *)pArg)->pSession->sendMessage(&msg);

   ((WALKER_THREAD_ARGS *)pArg)->pSession->decRefCount();
   ((WALKER_THREAD_ARGS *)pArg)->object->decRefCount();
   free(pArg);
}

/**
 * Start SNMP walk
 */
void ClientSession::StartSnmpWalk(NXCPMessage *pRequest)
{
   NXCPMessage msg;
   WALKER_THREAD_ARGS *pArg;

   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(pRequest->getId());

   NetObj *object = FindObjectById(pRequest->getFieldAsUInt32(VID_OBJECT_ID));
   if (object != NULL)
   {
      if (object->getObjectClass() == OBJECT_NODE)
      {
         if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
         {
            msg.setField(VID_RCC, RCC_SUCCESS);

            object->incRefCount();
            m_dwRefCount++;

            pArg = (WALKER_THREAD_ARGS *)malloc(sizeof(WALKER_THREAD_ARGS));
            pArg->pSession = this;
            pArg->object = object;
            pArg->dwRqId = pRequest->getId();
            pRequest->getFieldAsString(VID_SNMP_OID, pArg->szBaseOID, MAX_OID_LEN * 4);

            ThreadPoolExecute(g_mainThreadPool, WalkerThread, pArg);
         }
         else
         {
            msg.setField(VID_RCC, RCC_ACCESS_DENIED);
         }
      }
      else
      {
         msg.setField(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
      }
   }
   else
   {
      msg.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
   }
   sendMessage(&msg);
}

/**
 * Resolve single DCI name
 */
UINT32 ClientSession::resolveDCIName(UINT32 dwNode, UINT32 dwItem, TCHAR **ppszName)
{
   DCObject *pItem;
   UINT32 dwResult;

   NetObj *object = FindObjectById(dwNode);
   if (object != NULL)
   {
		if ((object->getObjectClass() == OBJECT_NODE) ||
			 (object->getObjectClass() == OBJECT_CLUSTER) ||
			 (object->getObjectClass() == OBJECT_MOBILEDEVICE) ||
			 (object->getObjectClass() == OBJECT_TEMPLATE))
		{
			if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
			{
				pItem = ((Template *)object)->getDCObjectById(dwItem);
				if (pItem != NULL)
				{
					*ppszName = (TCHAR *)pItem->getDescription();
					dwResult = RCC_SUCCESS;
				}
				else
				{
					dwResult = RCC_INVALID_DCI_ID;
				}
			}
			else
			{
				dwResult = RCC_ACCESS_DENIED;
			}
		}
		else
		{
	      dwResult = RCC_INCOMPATIBLE_OPERATION;
		}
   }
   else
   {
      dwResult = RCC_INVALID_OBJECT_ID;
   }
   return dwResult;
}

/**
 * Resolve DCI identifiers to names
 */
void ClientSession::resolveDCINames(NXCPMessage *pRequest)
{
   UINT32 i, dwId, dwNumDCI, *pdwNodeList, *pdwDCIList, dwResult = RCC_INVALID_ARGUMENT;
   TCHAR *pszName;
   NXCPMessage msg;

   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(pRequest->getId());

   dwNumDCI = pRequest->getFieldAsUInt32(VID_NUM_ITEMS);
   pdwNodeList = (UINT32 *)malloc(sizeof(UINT32) * dwNumDCI);
   pdwDCIList = (UINT32 *)malloc(sizeof(UINT32) * dwNumDCI);
   pRequest->getFieldAsInt32Array(VID_NODE_LIST, dwNumDCI, pdwNodeList);
   pRequest->getFieldAsInt32Array(VID_DCI_LIST, dwNumDCI, pdwDCIList);

   for(i = 0, dwId = VID_DCI_LIST_BASE; i < dwNumDCI; i++)
   {
      dwResult = resolveDCIName(pdwNodeList[i], pdwDCIList[i], &pszName);
      if (dwResult != RCC_SUCCESS)
         break;
      msg.setField(dwId++, pszName);
   }

   free(pdwNodeList);
   free(pdwDCIList);

   msg.setField(VID_RCC, dwResult);
   sendMessage(&msg);
}

/**
 * Send list of available agent configs to client
 */
void ClientSession::sendAgentCfgList(UINT32 dwRqId)
{
   NXCPMessage msg;
   DB_RESULT hResult;
   UINT32 i, dwId, dwNumRows;
   TCHAR szText[MAX_DB_STRING];

   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(dwRqId);

   if (m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_AGENT_CFG)
   {
      hResult = DBSelect(g_hCoreDB, _T("SELECT config_id,config_name,sequence_number FROM agent_configs"));
      if (hResult != NULL)
      {
         dwNumRows = DBGetNumRows(hResult);
         msg.setField(VID_RCC, RCC_SUCCESS);
         msg.setField(VID_NUM_RECORDS, dwNumRows);
         for(i = 0, dwId = VID_AGENT_CFG_LIST_BASE; i < dwNumRows; i++, dwId += 7)
         {
            msg.setField(dwId++, DBGetFieldULong(hResult, i, 0));
            DBGetField(hResult, i, 1, szText, MAX_DB_STRING);
            DecodeSQLString(szText);
            msg.setField(dwId++, szText);
            msg.setField(dwId++, DBGetFieldULong(hResult, i, 2));
         }
         DBFreeResult(hResult);
      }
      else
      {
         msg.setField(VID_RCC, RCC_DB_FAILURE);
      }
   }
   else
   {
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);
   }

   sendMessage(&msg);
}


/**
 *  Open (get all data) server-stored agent's config
 */
void ClientSession::OpenAgentConfig(NXCPMessage *pRequest)
{
   NXCPMessage msg;
   DB_RESULT hResult;
   UINT32 dwCfgId;
   TCHAR *pszStr, szQuery[256], szBuffer[MAX_DB_STRING];

   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(pRequest->getId());

   if (m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_AGENT_CFG)
   {
      dwCfgId = pRequest->getFieldAsUInt32(VID_CONFIG_ID);
      _sntprintf(szQuery, 256, _T("SELECT config_name,config_file,config_filter,sequence_number FROM agent_configs WHERE config_id=%d"), dwCfgId);
      hResult = DBSelect(g_hCoreDB, szQuery);
      if (hResult != NULL)
      {
         if (DBGetNumRows(hResult) > 0)
         {
            msg.setField(VID_RCC, RCC_SUCCESS);
            msg.setField(VID_CONFIG_ID, dwCfgId);
            DecodeSQLStringAndSetVariable(&msg, VID_NAME, DBGetField(hResult, 0, 0, szBuffer, MAX_DB_STRING));
            pszStr = DBGetField(hResult, 0, 1, NULL, 0);
            DecodeSQLStringAndSetVariable(&msg, VID_CONFIG_FILE, pszStr);
            free(pszStr);
            pszStr = DBGetField(hResult, 0, 2, NULL, 0);
            DecodeSQLStringAndSetVariable(&msg, VID_FILTER, pszStr);
            free(pszStr);
            msg.setField(VID_SEQUENCE_NUMBER, DBGetFieldULong(hResult, 0, 3));
         }
         else
         {
            msg.setField(VID_RCC, RCC_INVALID_CONFIG_ID);
         }
         DBFreeResult(hResult);
      }
      else
      {
         msg.setField(VID_RCC, RCC_DB_FAILURE);
      }
   }
   else
   {
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);
   }

   sendMessage(&msg);
}


/**
 *  Save changes to server-stored agent's configuration
 */
void ClientSession::SaveAgentConfig(NXCPMessage *pRequest)
{
   NXCPMessage msg;
   DB_RESULT hResult;
   UINT32 dwCfgId, dwSeqNum;
   TCHAR szQuery[256], szName[MAX_DB_STRING], *pszFilter, *pszText;
   TCHAR *pszQuery, *pszEscFilter, *pszEscText, *pszEscName;
   BOOL bCreate;

   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(pRequest->getId());

   if (m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_AGENT_CFG)
   {
      dwCfgId = pRequest->getFieldAsUInt32(VID_CONFIG_ID);
      _sntprintf(szQuery, 256, _T("SELECT config_name FROM agent_configs WHERE config_id=%d"), dwCfgId);
      hResult = DBSelect(g_hCoreDB, szQuery);
      if (hResult != NULL)
      {
         bCreate = (DBGetNumRows(hResult) == 0);
         DBFreeResult(hResult);

         pRequest->getFieldAsString(VID_NAME, szName, MAX_DB_STRING);
         pszEscName = EncodeSQLString(szName);

         pszFilter = pRequest->getFieldAsString(VID_FILTER);
         pszEscFilter = EncodeSQLString(pszFilter);
         free(pszFilter);

         pszText = pRequest->getFieldAsString(VID_CONFIG_FILE);
         pszEscText = EncodeSQLString(pszText);
         free(pszText);

			size_t qlen = _tcslen(pszEscText) + _tcslen(pszEscFilter) + _tcslen(pszEscName) + 256;
         pszQuery = (TCHAR *)malloc(qlen * sizeof(TCHAR));
         if (bCreate)
         {
            if (dwCfgId == 0)
            {
               // Request for new ID creation
               dwCfgId = CreateUniqueId(IDG_AGENT_CONFIG);
               msg.setField(VID_CONFIG_ID, dwCfgId);

               // Request sequence number
               hResult = DBSelect(g_hCoreDB, _T("SELECT max(sequence_number) FROM agent_configs"));
               if (hResult != NULL)
               {
                  if (DBGetNumRows(hResult) > 0)
                     dwSeqNum = DBGetFieldULong(hResult, 0, 0) + 1;
                  else
                     dwSeqNum = 1;
                  DBFreeResult(hResult);
               }
               else
               {
                  dwSeqNum = 1;
               }
               msg.setField(VID_SEQUENCE_NUMBER, dwSeqNum);
            }
            _sntprintf(pszQuery, qlen, _T("INSERT INTO agent_configs (config_id,config_name,config_filter,config_file,sequence_number) VALUES (%d,'%s','%s','%s',%d)"),
                      dwCfgId, pszEscName, pszEscFilter, pszEscText, dwSeqNum);
         }
         else
         {
            _sntprintf(pszQuery, qlen, _T("UPDATE agent_configs SET config_name='%s',config_filter='%s',config_file='%s',sequence_number=%d WHERE config_id=%d"),
                      pszEscName, pszEscFilter, pszEscText,
                      pRequest->getFieldAsUInt32(VID_SEQUENCE_NUMBER), dwCfgId);
         }
         free(pszEscName);
         free(pszEscText);
         free(pszEscFilter);

         if (DBQuery(g_hCoreDB, pszQuery))
         {
            msg.setField(VID_RCC, RCC_SUCCESS);
         }
         else
         {
            msg.setField(VID_RCC, RCC_DB_FAILURE);
         }
         free(pszQuery);
      }
      else
      {
         msg.setField(VID_RCC, RCC_DB_FAILURE);
      }
   }
   else
   {
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);
   }

   sendMessage(&msg);
}


/**
 * Delete agent's configuration
 */
void ClientSession::DeleteAgentConfig(NXCPMessage *pRequest)
{
   NXCPMessage msg;
   DB_RESULT hResult;
   UINT32 dwCfgId;
   TCHAR szQuery[256];

   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(pRequest->getId());

   if (m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_AGENT_CFG)
   {
      dwCfgId = pRequest->getFieldAsUInt32(VID_CONFIG_ID);
      _sntprintf(szQuery, 256, _T("SELECT config_name FROM agent_configs WHERE config_id=%d"), dwCfgId);
      hResult = DBSelect(g_hCoreDB, szQuery);
      if (hResult != NULL)
      {
         if (DBGetNumRows(hResult) > 0)
         {
            _sntprintf(szQuery, 256, _T("DELETE FROM agent_configs WHERE config_id=%d"), dwCfgId);
            if (DBQuery(g_hCoreDB, szQuery))
            {
               msg.setField(VID_RCC, RCC_SUCCESS);
            }
            else
            {
               msg.setField(VID_RCC, RCC_DB_FAILURE);
            }
         }
         else
         {
            msg.setField(VID_RCC, RCC_INVALID_CONFIG_ID);
         }
         DBFreeResult(hResult);
      }
      else
      {
         msg.setField(VID_RCC, RCC_DB_FAILURE);
      }
   }
   else
   {
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);
   }

   sendMessage(&msg);
}


/**
 * Swap sequence numbers of two agent configs
 */
void ClientSession::SwapAgentConfigs(NXCPMessage *pRequest)
{
   NXCPMessage msg;
   DB_RESULT hResult;
   TCHAR szQuery[256];
   BOOL bRet;

   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(pRequest->getId());

   if (m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_AGENT_CFG)
   {
      _sntprintf(szQuery, 256, _T("SELECT config_id,sequence_number FROM agent_configs WHERE config_id=%d OR config_id=%d"),
                 pRequest->getFieldAsUInt32(VID_CONFIG_ID), pRequest->getFieldAsUInt32(VID_CONFIG_ID_2));
      hResult = DBSelect(g_hCoreDB, szQuery);
      if (hResult != NULL)
      {
         if (DBGetNumRows(hResult) >= 2)
         {
            if (DBBegin(g_hCoreDB))
            {
               _sntprintf(szQuery, 256, _T("UPDATE agent_configs SET sequence_number=%d WHERE config_id=%d"),
                          DBGetFieldULong(hResult, 1, 1), DBGetFieldULong(hResult, 0, 0));
               bRet = DBQuery(g_hCoreDB, szQuery);
               if (bRet)
               {
                  _sntprintf(szQuery, 256, _T("UPDATE agent_configs SET sequence_number=%d WHERE config_id=%d"),
                             DBGetFieldULong(hResult, 0, 1), DBGetFieldULong(hResult, 1, 0));
                  bRet = DBQuery(g_hCoreDB, szQuery);
               }

               if (bRet)
               {
                  DBCommit(g_hCoreDB);
                  msg.setField(VID_RCC, RCC_SUCCESS);
               }
               else
               {
                  DBRollback(g_hCoreDB);
                  msg.setField(VID_RCC, RCC_DB_FAILURE);
               }
            }
            else
            {
               msg.setField(VID_RCC, RCC_DB_FAILURE);
            }
         }
         else
         {
            msg.setField(VID_RCC, RCC_INVALID_CONFIG_ID);
         }
         DBFreeResult(hResult);
      }
      else
      {
         msg.setField(VID_RCC, RCC_DB_FAILURE);
      }
   }
   else
   {
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);
   }

   sendMessage(&msg);
}

/**
 * Send config to agent on request
 */
void ClientSession::sendConfigForAgent(NXCPMessage *pRequest)
{
   NXCPMessage msg;
   TCHAR szPlatform[MAX_DB_STRING], szError[256], szBuffer[256], *pszText;
   WORD wMajor, wMinor, wRelease;
   int i, nNumRows;
   DB_RESULT hResult;
   NXSL_Program *pScript;
   NXSL_Value *ppArgList[5], *pValue;
   UINT32 dwCfgId;

   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(pRequest->getId());

   pRequest->getFieldAsString(VID_PLATFORM_NAME, szPlatform, MAX_DB_STRING);
   wMajor = pRequest->getFieldAsUInt16(VID_VERSION_MAJOR);
   wMinor = pRequest->getFieldAsUInt16(VID_VERSION_MINOR);
   wRelease = pRequest->getFieldAsUInt16(VID_VERSION_RELEASE);
   DbgPrintf(3, _T("Finding config for agent at %s: platform=\"%s\", version=\"%d.%d.%d\""),
             SockaddrToStr(m_clientAddr, szBuffer), szPlatform, (int)wMajor, (int)wMinor, (int)wRelease);

   hResult = DBSelect(g_hCoreDB, _T("SELECT config_id,config_file,config_filter FROM agent_configs ORDER BY sequence_number"));
   if (hResult != NULL)
   {
      nNumRows = DBGetNumRows(hResult);
      for(i = 0; i < nNumRows; i++)
      {
         dwCfgId = DBGetFieldULong(hResult, i, 0);

         // Compile script
         pszText = DBGetField(hResult, i, 2, NULL, 0);
         DecodeSQLString(pszText);
         pScript = (NXSL_Program *)NXSLCompile(pszText, szError, 256);
         free(pszText);

         if (pScript != NULL)
         {
            // Set arguments:
            // $1 - IP address
            // $2 - platform
            // $3 - major version number
            // $4 - minor version number
            // $5 - release number
            ppArgList[0] = new NXSL_Value(SockaddrToStr(m_clientAddr, szBuffer));
            ppArgList[1] = new NXSL_Value(szPlatform);
            ppArgList[2] = new NXSL_Value((LONG)wMajor);
            ppArgList[3] = new NXSL_Value((LONG)wMinor);
            ppArgList[4] = new NXSL_Value((LONG)wRelease);

            // Run script
            DbgPrintf(3, _T("Running configuration matching script %d"), dwCfgId);
            NXSL_VM *vm = new NXSL_VM(new NXSL_ServerEnv);
            if (vm->load(pScript) && vm->run(5, ppArgList))
            {
               pValue = vm->getResult();
               if (pValue->getValueAsInt32() != 0)
               {
                  DbgPrintf(3, _T("Configuration script %d matched for agent %s, sending config"),
                            dwCfgId, SockaddrToStr(m_clientAddr, szBuffer));
                  msg.setField(VID_RCC, (WORD)0);
                  pszText = DBGetField(hResult, i, 1, NULL, 0);
                  DecodeSQLStringAndSetVariable(&msg, VID_CONFIG_FILE, pszText);
                  msg.setField(VID_CONFIG_ID, dwCfgId);
                  free(pszText);
                  break;
               }
               else
               {
                  DbgPrintf(3, _T("Configuration script %d not matched for agent %s"),
                            dwCfgId, SockaddrToStr(m_clientAddr, szBuffer));
               }
            }
            else
            {
               _sntprintf(szError, 256, _T("AgentCfg::%d"), dwCfgId);
               PostEvent(EVENT_SCRIPT_ERROR, g_dwMgmtNode, "ssd", szError, vm->getErrorText(), 0);
            }
            delete pScript;
            delete vm;
         }
         else
         {
            _sntprintf(szBuffer, 256, _T("AgentCfg::%d"), dwCfgId);
            PostEvent(EVENT_SCRIPT_ERROR, g_dwMgmtNode, "ssd", szBuffer, szError, 0);
         }
      }
      DBFreeResult(hResult);

      if (i == nNumRows)
         msg.setField(VID_RCC, (WORD)1);  // No matching configs found
   }
   else
   {
      msg.setField(VID_RCC, (WORD)1);  // DB Failure
   }

   sendMessage(&msg);
}


//
// Send object comments to client
//

void ClientSession::SendObjectComments(NXCPMessage *pRequest)
{
   NXCPMessage msg;
   NetObj *object;

   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(pRequest->getId());

   object = FindObjectById(pRequest->getFieldAsUInt32(VID_OBJECT_ID));
   if (object != NULL)
   {
      if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
      {
         msg.setField(VID_RCC, RCC_SUCCESS);
         object->commentsToMessage(&msg);
      }
      else
      {
         msg.setField(VID_RCC, RCC_ACCESS_DENIED);
      }
   }
   else
   {
      msg.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   sendMessage(&msg);
}

/**
 * Update object comments from client
 */
void ClientSession::updateObjectComments(NXCPMessage *pRequest)
{
   NXCPMessage msg;
   NetObj *object;

   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(pRequest->getId());

   object = FindObjectById(pRequest->getFieldAsUInt32(VID_OBJECT_ID));
   if (object != NULL)
   {
      if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_MODIFY))
      {
         object->setComments(pRequest->getFieldAsString(VID_COMMENTS));
      }
      else
      {
         msg.setField(VID_RCC, RCC_ACCESS_DENIED);
      }
   }
   else
   {
      msg.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   sendMessage(&msg);
}

/**
 * Push DCI data
 */
void ClientSession::pushDCIData(NXCPMessage *pRequest)
{
   NXCPMessage msg;
   UINT32 i, dwObjectId, dwNumItems, dwId, dwItemId;
   NetObj *object;
   DataCollectionTarget **dcTargetList = NULL;
   DCItem **ppItemList = NULL;
   TCHAR szName[256], **ppValueList;
   BOOL bOK;

   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(pRequest->getId());

   dwNumItems = pRequest->getFieldAsUInt32(VID_NUM_ITEMS);
   if (dwNumItems > 0)
   {
      dcTargetList = (DataCollectionTarget **)malloc(sizeof(DataCollectionTarget *) * dwNumItems);
      ppItemList = (DCItem **)malloc(sizeof(DCItem *) * dwNumItems);
      ppValueList = (TCHAR **)malloc(sizeof(TCHAR *) * dwNumItems);
      memset(ppValueList, 0, sizeof(TCHAR *) * dwNumItems);

      for(i = 0, dwId = VID_PUSH_DCI_DATA_BASE, bOK = TRUE; (i < dwNumItems) && bOK; i++)
      {
         bOK = FALSE;

         // Find object either by ID or name (id ID==0)
         dwObjectId = pRequest->getFieldAsUInt32(dwId++);
         if (dwObjectId != 0)
         {
            object = FindObjectById(dwObjectId);
         }
         else
         {
            pRequest->getFieldAsString(dwId++, szName, 256);
				if (szName[0] == _T('@'))
				{
               InetAddress ipAddr = InetAddress::resolveHostName(&szName[1]);
					object = FindNodeByIP(0, ipAddr);
				}
				else
				{
					object = FindObjectByName(szName, OBJECT_NODE);
				}
         }

         // Validate object
         if (object != NULL)
         {
            if ((object->getObjectClass() == OBJECT_NODE) || (object->getObjectClass() == OBJECT_MOBILEDEVICE))
            {
               if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_PUSH_DATA))
               {
                  dcTargetList[i] = (DataCollectionTarget *)object;

                  // Object OK, find DCI by ID or name (if ID==0)
                  dwItemId = pRequest->getFieldAsUInt32(dwId++);
						DCObject *pItem;
                  if (dwItemId != 0)
                  {
                     pItem = dcTargetList[i]->getDCObjectById(dwItemId);
                  }
                  else
                  {
                     pRequest->getFieldAsString(dwId++, szName, 256);
                     pItem = dcTargetList[i]->getDCObjectByName(szName);
                  }

                  if ((pItem != NULL) && (pItem->getType() == DCO_TYPE_ITEM))
                  {
                     if (pItem->getDataSource() == DS_PUSH_AGENT)
                     {
                        ppItemList[i] = (DCItem *)pItem;
                        ppValueList[i] = pRequest->getFieldAsString(dwId++);
                        bOK = TRUE;
                     }
                     else
                     {
                        msg.setField(VID_RCC, RCC_NOT_PUSH_DCI);
                     }
                  }
                  else
                  {
                     msg.setField(VID_RCC, RCC_INVALID_DCI_ID);
                  }
               }
               else
               {
                  msg.setField(VID_RCC, RCC_ACCESS_DENIED);
               }
            }
            else
            {
               msg.setField(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
            }
         }
         else
         {
            msg.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
         }
      }

      // If all items was checked OK, push data
      if (bOK)
      {
         time_t t = pRequest->getFieldAsTime(VID_TIMESTAMP);
         if (t == 0)
            t = time(NULL);
         for(i = 0; i < dwNumItems; i++)
         {
				if (_tcslen(ppValueList[i]) >= MAX_DCI_STRING_VALUE)
					ppValueList[i][MAX_DCI_STRING_VALUE - 1] = 0;
				dcTargetList[i]->processNewDCValue(ppItemList[i], t, ppValueList[i]);
            if (t > ppItemList[i]->getLastPollTime())
				   ppItemList[i]->setLastPollTime(t);
         }
         msg.setField(VID_RCC, RCC_SUCCESS);
      }
      else
      {
         msg.setField(VID_FAILED_DCI_INDEX, i - 1);
      }

      // Cleanup
      for(i = 0; i < dwNumItems; i++)
         safe_free(ppValueList[i]);
      safe_free(ppValueList);
      free(ppItemList);
      free(dcTargetList);
   }
   else
   {
      msg.setField(VID_RCC, RCC_INVALID_ARGUMENT);
   }

   sendMessage(&msg);
}

/**
 * Get address list
 */
void ClientSession::getAddrList(NXCPMessage *pRequest)
{
   NXCPMessage msg;
   TCHAR szQuery[256];
   UINT32 i, dwNumRec, dwId;
   DB_RESULT hResult;

   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(pRequest->getId());

   if (m_dwSystemAccess & SYSTEM_ACCESS_SERVER_CONFIG)
   {
      _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("SELECT addr_type,addr1,addr2 FROM address_lists WHERE list_type=%d"),
                pRequest->getFieldAsUInt32(VID_ADDR_LIST_TYPE));
      hResult = DBSelect(g_hCoreDB, szQuery);
      if (hResult != NULL)
      {
         dwNumRec = DBGetNumRows(hResult);
         msg.setField(VID_NUM_RECORDS, dwNumRec);
         for(i = 0, dwId = VID_ADDR_LIST_BASE; i < dwNumRec; i++, dwId += 7)
         {
            msg.setField(dwId++, DBGetFieldULong(hResult, i, 0));
            msg.setField(dwId++, DBGetFieldIPAddr(hResult, i, 1));
            msg.setField(dwId++, DBGetFieldIPAddr(hResult, i, 2));
         }
         DBFreeResult(hResult);
         msg.setField(VID_RCC, RCC_SUCCESS);
      }
      else
      {
         msg.setField(VID_RCC, RCC_DB_FAILURE);
      }
   }
   else
   {
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);
   }

   sendMessage(&msg);
}

/**
 * Set address list
 */
void ClientSession::setAddrList(NXCPMessage *pRequest)
{
   NXCPMessage msg;
   UINT32 i, dwId, dwNumRec, dwListType;
   TCHAR szQuery[256], szIpAddr1[24], szIpAddr2[24];

   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(pRequest->getId());

   if (m_dwSystemAccess & SYSTEM_ACCESS_SERVER_CONFIG)
   {
      dwListType = pRequest->getFieldAsUInt32(VID_ADDR_LIST_TYPE);
      _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("DELETE FROM address_lists WHERE list_type=%d"), dwListType);
      DBBegin(g_hCoreDB);
      if (DBQuery(g_hCoreDB, szQuery))
      {
         dwNumRec = pRequest->getFieldAsUInt32(VID_NUM_RECORDS);
         for(i = 0, dwId = VID_ADDR_LIST_BASE; i < dwNumRec; i++, dwId += 10)
         {
            _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("INSERT INTO address_lists (list_type,addr_type,addr1,addr2,community_id) VALUES (%d,%d,'%s','%s',0)"),
                      dwListType, pRequest->getFieldAsUInt32(dwId),
                      IpToStr(pRequest->getFieldAsUInt32(dwId + 1), szIpAddr1),
                      IpToStr(pRequest->getFieldAsUInt32(dwId + 2), szIpAddr2));
            if (!DBQuery(g_hCoreDB, szQuery))
               break;
         }

         if (i == dwNumRec)
         {
            DBCommit(g_hCoreDB);
            msg.setField(VID_RCC, RCC_SUCCESS);
         }
         else
         {
            DBRollback(g_hCoreDB);
            msg.setField(VID_RCC, RCC_DB_FAILURE);
         }
      }
      else
      {
         DBRollback(g_hCoreDB);
         msg.setField(VID_RCC, RCC_DB_FAILURE);
      }
   }
   else
   {
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);
   }

   sendMessage(&msg);
}

/**
 * Reset server component
 */
void ClientSession::resetComponent(NXCPMessage *pRequest)
{
   NXCPMessage msg;
   UINT32 dwCode;

   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(pRequest->getId());

   if (m_dwSystemAccess & SYSTEM_ACCESS_SERVER_CONFIG)
   {
      dwCode = pRequest->getFieldAsUInt32(VID_COMPONENT_ID);
      switch(dwCode)
      {
         case SRV_COMPONENT_DISCOVERY_MGR:
            ResetDiscoveryPoller();
            msg.setField(VID_RCC, RCC_SUCCESS);
            break;
         default:
            msg.setField(VID_RCC, RCC_INVALID_ARGUMENT);
            break;
      }
   }
   else
   {
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);
   }

   sendMessage(&msg);
}

/**
 * Get list of events used by template's or node's DCIs
 */
void ClientSession::getDCIEventList(NXCPMessage *request)
{
   NXCPMessage msg;
   NetObj *object;
   UINT32 *pdwEventList, count;

   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(request->getId());

   object = FindObjectById(request->getFieldAsUInt32(VID_OBJECT_ID));
   if (object != NULL)
   {
      if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
      {
         if ((object->getObjectClass() == OBJECT_NODE) ||
             (object->getObjectClass() == OBJECT_CLUSTER) ||
             (object->getObjectClass() == OBJECT_TEMPLATE))
         {
            pdwEventList = ((Template *)object)->getDCIEventsList(&count);
            if (pdwEventList != NULL)
            {
               msg.setField(VID_NUM_EVENTS, count);
               msg.setFieldFromInt32Array(VID_EVENT_LIST, count, pdwEventList);
               free(pdwEventList);
            }
            else
            {
               msg.setField(VID_NUM_EVENTS, (UINT32)0);
            }
            msg.setField(VID_RCC, RCC_SUCCESS);
         }
         else
         {
            msg.setField(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
         }
      }
      else
      {
         msg.setField(VID_RCC, RCC_ACCESS_DENIED);
      }
   }
   else
   {
      msg.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   sendMessage(&msg);
}

/**
 * Data for script names enumeration callback
 */
struct ScriptNamesCallbackData
{
   NXCPMessage *msg;
   UINT32 fieldId;
};

/**
 * Script names enumeration callback
 */
static bool ScriptNamesCallback(const TCHAR *name, void *arg)
{
   ScriptNamesCallbackData *data = (ScriptNamesCallbackData *)arg;
   data->msg->setField(data->fieldId++, ResolveScriptName(name));
   data->msg->setField(data->fieldId++, name);
   return true;
}

/**
 * Get list of scripts used by template's or node's DCIs
 */
void ClientSession::getDCIScriptList(NXCPMessage *request)
{
   NXCPMessage msg;

   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(request->getId());

   NetObj *object = FindObjectById(request->getFieldAsUInt32(VID_OBJECT_ID));
   if (object != NULL)
   {
      if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
      {
         if ((object->getObjectClass() == OBJECT_NODE) ||
             (object->getObjectClass() == OBJECT_CLUSTER) ||
             (object->getObjectClass() == OBJECT_TEMPLATE))
         {
            StringSet *scripts = ((Template *)object)->getDCIScriptList();
            msg.setField(VID_NUM_SCRIPTS, (INT32)scripts->size());
            ScriptNamesCallbackData data;
            data.msg = &msg;
            data.fieldId = VID_SCRIPT_LIST_BASE;
            scripts->forEach(ScriptNamesCallback, &data);
            delete scripts;
            msg.setField(VID_RCC, RCC_SUCCESS);
         }
         else
         {
            msg.setField(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
         }
      }
      else
      {
         msg.setField(VID_RCC, RCC_ACCESS_DENIED);
      }
   }
   else
   {
      msg.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   sendMessage(&msg);
}

/**
 * Export server configuration (event, templates, etc.)
 */
void ClientSession::exportConfiguration(NXCPMessage *pRequest)
{
   NXCPMessage msg;
   UINT32 i, dwNumTemplates;
   UINT32 *pdwList, *pdwTemplateList;
   NetObj *object;

   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(pRequest->getId());

   if (checkSysAccessRights(SYSTEM_ACCESS_CONFIGURE_TRAPS | SYSTEM_ACCESS_VIEW_EVENT_DB | SYSTEM_ACCESS_EPP))
   {
      dwNumTemplates = pRequest->getFieldAsUInt32(VID_NUM_OBJECTS);
      if (dwNumTemplates > 0)
      {
         pdwTemplateList = (UINT32 *)malloc(sizeof(UINT32) * dwNumTemplates);
         pRequest->getFieldAsInt32Array(VID_OBJECT_LIST, dwNumTemplates, pdwTemplateList);
      }
      else
      {
         pdwTemplateList = NULL;
      }

      for(i = 0; i < dwNumTemplates; i++)
      {
         object = FindObjectById(pdwTemplateList[i]);
         if (object != NULL)
         {
            if (object->getObjectClass() == OBJECT_TEMPLATE)
            {
               if (!object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
               {
                  msg.setField(VID_RCC, RCC_ACCESS_DENIED);
                  break;
               }
            }
            else
            {
               msg.setField(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
               break;
            }
         }
         else
         {
            msg.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
            break;
         }
      }

      if (i == dwNumTemplates)   // All objects passed test
      {
         String str;
			TCHAR *temp;

         str = _T("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<configuration>\n\t<formatVersion>4</formatVersion>\n\t<description>");
			temp = pRequest->getFieldAsString(VID_DESCRIPTION);
			str.appendPreallocated(EscapeStringForXML(temp, -1));
			free(temp);
         str += _T("</description>\n");

         // Write events
         str += _T("\t<events>\n");
         UINT32 count = pRequest->getFieldAsUInt32(VID_NUM_EVENTS);
         pdwList = (UINT32 *)malloc(sizeof(UINT32) * count);
         pRequest->getFieldAsInt32Array(VID_EVENT_LIST, count, pdwList);
         for(i = 0; i < count; i++)
            CreateNXMPEventRecord(str, pdwList[i]);
         safe_free(pdwList);
         str += _T("\t</events>\n");

         // Write templates
         str += _T("\t<templates>\n");
         for(i = 0; i < dwNumTemplates; i++)
         {
            object = FindObjectById(pdwTemplateList[i]);
            if (object != NULL)
            {
               ((Template *)object)->createNXMPRecord(str);
            }
         }
         str += _T("\t</templates>\n");

         // Write traps
         str += _T("\t<traps>\n");
         count = pRequest->getFieldAsUInt32(VID_NUM_TRAPS);
         pdwList = (UINT32 *)malloc(sizeof(UINT32) * count);
         pRequest->getFieldAsInt32Array(VID_TRAP_LIST, count, pdwList);
         for(i = 0; i < count; i++)
            CreateTrapExportRecord(str, pdwList[i]);
         safe_free(pdwList);
         str += _T("\t</traps>\n");

         // Write rules
         str += _T("\t<rules>\n");
         count = pRequest->getFieldAsUInt32(VID_NUM_RULES);
         DWORD varId = VID_RULE_LIST_BASE;
         uuid_t guid;
         for(i = 0; i < count; i++)
         {
            pRequest->getFieldAsBinary(varId++, guid, UUID_LENGTH);
            g_pEventPolicy->exportRule(str, guid);
         }
         str += _T("\t</rules>\n");

         // Write scripts
         str.append(_T("\t<scripts>\n"));
         count = pRequest->getFieldAsUInt32(VID_NUM_SCRIPTS);
         pdwList = (UINT32 *)malloc(sizeof(UINT32) * count);
         pRequest->getFieldAsInt32Array(VID_SCRIPT_LIST, count, pdwList);
         for(i = 0; i < count; i++)
            CreateScriptExportRecord(str, pdwList[i]);
         safe_free(pdwList);
         str.append(_T("\t</scripts>\n"));

         // Write object tools
         str.append(_T("\t<objectTools>\n"));
         count = pRequest->getFieldAsUInt32(VID_NUM_TOOLS);
         pdwList = (UINT32 *)malloc(sizeof(UINT32) * count);
         pRequest->getFieldAsInt32Array(VID_TOOL_LIST, count, pdwList);
         for(i = 0; i < count; i++)
            CreateObjectToolExportRecord(str, pdwList[i]);
         safe_free(pdwList);
         str.append(_T("\t</objectTools>\n"));

         // Write DCI summary tables
         str.append(_T("\t<dciSummaryTables>\n"));
         count = pRequest->getFieldAsUInt32(VID_NUM_SUMMARY_TABLES);
         pdwList = (UINT32 *)malloc(sizeof(UINT32) * count);
         pRequest->getFieldAsInt32Array(VID_SUMMARY_TABLE_LIST, count, pdwList);
         for(i = 0; i < count; i++)
            CreateSummaryTableExportRecord(pdwList[i], str);
         safe_free(pdwList);
         str.append(_T("\t</dciSummaryTables>\n"));

			// Close document
			str += _T("</configuration>\n");

         // put result into message
         msg.setField(VID_RCC, RCC_SUCCESS);
         msg.setField(VID_NXMP_CONTENT, (const TCHAR *)str);
      }

      safe_free(pdwTemplateList);
   }
   else
   {
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);
   }

   sendMessage(&msg);
}

/**
 * Import server configuration (events, templates, etc.)
 */
void ClientSession::importConfiguration(NXCPMessage *pRequest)
{
   NXCPMessage msg;
   TCHAR szLockInfo[MAX_SESSION_NAME], szError[1024];
   UINT32 dwFlags;

   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(pRequest->getId());

   if (checkSysAccessRights(SYSTEM_ACCESS_CONFIGURE_TRAPS | SYSTEM_ACCESS_EDIT_EVENT_DB | SYSTEM_ACCESS_EPP))
   {
      char *content = pRequest->getFieldAsUtf8String(VID_NXMP_CONTENT);
      if (content != NULL)
      {
			Config *config = new Config();
         if (config->loadXmlConfigFromMemory(content, (int)strlen(content), NULL, "configuration"))
         {
            // Lock all required components
            if (LockComponent(CID_EPP, m_id, m_sessionName, NULL, szLockInfo))
            {
               m_dwFlags |= CSF_EPP_LOCKED;

               // Validate and import configuration
               dwFlags = pRequest->getFieldAsUInt32(VID_FLAGS);
               if (ValidateConfig(config, dwFlags, szError, 1024))
               {
                  msg.setField(VID_RCC, ImportConfig(config, dwFlags));
               }
               else
               {
                  msg.setField(VID_RCC, RCC_CONFIG_VALIDATION_ERROR);
                  msg.setField(VID_ERROR_TEXT, szError);
               }

					UnlockComponent(CID_EPP);
               m_dwFlags &= ~CSF_EPP_LOCKED;
            }
            else
            {
               msg.setField(VID_RCC, RCC_COMPONENT_LOCKED);
               msg.setField(VID_COMPONENT, (WORD)NXMP_LC_EPP);
               msg.setField(VID_LOCKED_BY, szLockInfo);
            }
         }
         else
         {
            msg.setField(VID_RCC, RCC_CONFIG_PARSE_ERROR);
         }
			delete config;
         free(content);
      }
      else
      {
         msg.setField(VID_RCC, RCC_INVALID_ARGUMENT);
      }
   }
   else
   {
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);
   }

   sendMessage(&msg);
}

/**
 * Send basic DCI info to client
 */
void ClientSession::SendDCIInfo(NXCPMessage *pRequest)
{
   NXCPMessage msg;

   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(pRequest->getId());

   NetObj *object = FindObjectById(pRequest->getFieldAsUInt32(VID_OBJECT_ID));
   if (object != NULL)
   {
      if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
      {
         if ((object->getObjectClass() == OBJECT_NODE) ||
             (object->getObjectClass() == OBJECT_CLUSTER) ||
             (object->getObjectClass() == OBJECT_TEMPLATE))
         {
				DCObject *pItem = ((Template *)object)->getDCObjectById(pRequest->getFieldAsUInt32(VID_DCI_ID));
				if ((pItem != NULL) && (pItem->getType() == DCO_TYPE_ITEM))
				{
					msg.setField(VID_TEMPLATE_ID, pItem->getTemplateId());
					msg.setField(VID_RESOURCE_ID, pItem->getResourceId());
					msg.setField(VID_DCI_DATA_TYPE, (WORD)((DCItem *)pItem)->getDataType());
					msg.setField(VID_DCI_SOURCE_TYPE, (WORD)pItem->getDataSource());
					msg.setField(VID_NAME, pItem->getName());
					msg.setField(VID_DESCRIPTION, pItem->getDescription());
	            msg.setField(VID_RCC, RCC_SUCCESS);
				}
				else
				{
			      msg.setField(VID_RCC, RCC_INVALID_DCI_ID);
				}
         }
         else
         {
            msg.setField(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
         }
      }
      else
      {
         msg.setField(VID_RCC, RCC_ACCESS_DENIED);
      }
   }
   else
   {
      msg.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   sendMessage(&msg);
}

/**
 * Send list of available graphs to client
 */
void ClientSession::sendGraphList(UINT32 dwRqId)
{
   NXCPMessage msg;
	DB_RESULT hResult;
	GRAPH_ACL_ENTRY *pACL = NULL;
	int i, j, nRows, nACLSize;
	UINT32 dwId, dwNumGraphs, dwGraphId, dwOwner;
	UINT32 *pdwUsers, *pdwRights, dwGraphACLSize;
	TCHAR *pszStr;

   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(dwRqId);

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
	pACL = LoadGraphACL(hdb, 0, &nACLSize);
	if (nACLSize != -1)
	{
		hResult = DBSelect(hdb, _T("SELECT graph_id,owner_id,name,config FROM graphs"));
		if (hResult != NULL)
		{
			pdwUsers = (UINT32 *)malloc(sizeof(UINT32) * nACLSize);
			pdwRights = (UINT32 *)malloc(sizeof(UINT32) * nACLSize);
			nRows = DBGetNumRows(hResult);
			for(i = 0, dwNumGraphs = 0, dwId = VID_GRAPH_LIST_BASE; i < nRows; i++)
			{
				dwGraphId = DBGetFieldULong(hResult, i, 0);
				dwOwner = DBGetFieldULong(hResult, i, 1);
				if ((m_dwUserId == 0) ||
				    (m_dwUserId == dwOwner) ||
				    CheckGraphAccess(pACL, nACLSize, dwGraphId, m_dwUserId, NXGRAPH_ACCESS_READ))
				{
					msg.setField(dwId++, dwGraphId);
					msg.setField(dwId++, dwOwner);
					pszStr = DBGetField(hResult, i, 2, NULL, 0);
					if (pszStr != NULL)
					{
						DecodeSQLStringAndSetVariable(&msg, dwId++, pszStr);
						free(pszStr);
					}
					pszStr = DBGetField(hResult, i, 3, NULL, 0);
					if (pszStr != NULL)
					{
						DecodeSQLStringAndSetVariable(&msg, dwId++, pszStr);
						free(pszStr);
					}

					// ACL for graph
					for(j = 0, dwGraphACLSize = 0; j < nACLSize; j++)
					{
						if (pACL[j].dwGraphId == dwGraphId)
						{
							pdwUsers[dwGraphACLSize] = pACL[j].dwUserId;
							pdwRights[dwGraphACLSize] = pACL[j].dwAccess;
							dwGraphACLSize++;
						}
					}
					msg.setField(dwId++, dwGraphACLSize);
					msg.setFieldFromInt32Array(dwId++, dwGraphACLSize, pdwUsers);
					msg.setFieldFromInt32Array(dwId++, dwGraphACLSize, pdwRights);

					dwId += 3;
					dwNumGraphs++;
				}
			}
			DBFreeResult(hResult);
			free(pdwUsers);
			free(pdwRights);
			msg.setField(VID_NUM_GRAPHS, dwNumGraphs);
			msg.setField(VID_RCC, RCC_SUCCESS);
		}
		else
		{
			msg.setField(VID_RCC, RCC_DB_FAILURE);
		}
		safe_free(pACL);
	}
	else
	{
		msg.setField(VID_RCC, RCC_DB_FAILURE);
	}

   DBConnectionPoolReleaseConnection(hdb);
   sendMessage(&msg);
}

/**
 * Save graph
 */
void ClientSession::saveGraph(NXCPMessage *pRequest)
{
   NXCPMessage msg;
	BOOL bNew, bSuccess;
	UINT32 id, graphId, graphUserId, graphAccess, accessRightStatus;
	UINT16 overwrite;
	TCHAR szQuery[16384], *pszEscName, *pszEscData, *pszTemp, dwGraphName[255];
	int i, nACLSize;

   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(pRequest->getId());

	graphId = pRequest->getFieldAsUInt32(VID_GRAPH_ID);
	pRequest->getFieldAsString(VID_NAME,dwGraphName,255);
	overwrite = pRequest->getFieldAsUInt16(VID_FLAGS);

   GRAPH_ACL_AND_ID nameUniq = IsGraphNameExists(dwGraphName);

   if (nameUniq.graphId == graphId)
   {
      nameUniq.status = RCC_SUCCESS;
   }

	if (graphId == 0)
	{
		graphId = nameUniq.graphId ? nameUniq.graphId : CreateUniqueId(IDG_GRAPH);
		bNew = TRUE;
		accessRightStatus = RCC_SUCCESS;
	}
	else
	{
	   accessRightStatus = GetGraphAccessCheckResult(graphId, m_dwUserId);
		bNew = FALSE;
	}

   if (accessRightStatus == RCC_SUCCESS && (nameUniq.status == RCC_SUCCESS || (overwrite && bNew)))
   {
      bSuccess = TRUE;
      if (nameUniq.status != RCC_SUCCESS)
      {
         bNew = FALSE;
         graphId = nameUniq.graphId;
      }
   }
   else
   {
      bSuccess = FALSE;
      msg.setField(VID_RCC, accessRightStatus ? accessRightStatus : nameUniq.status );
   }

	// Create/update graph
	if (bSuccess)
	{
		debugPrintf(5, _T("%s graph %d"), bNew ? _T("Creating") : _T("Updating"), graphId);
		bSuccess = FALSE;
      DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
		if (DBBegin(hdb))
		{
			pRequest->getFieldAsString(VID_NAME, szQuery, 256);
			pszEscName = EncodeSQLString(szQuery);
			pszTemp = pRequest->getFieldAsString(VID_GRAPH_CONFIG);
			pszEscData = EncodeSQLString(CHECK_NULL_EX(pszTemp));
			safe_free(pszTemp);
			if (bNew)
			{
				_sntprintf(szQuery, 16384, _T("INSERT INTO graphs (graph_id,owner_id,name,config) VALUES (%d,%d,'%s','%s')"),
				           graphId, m_dwUserId, pszEscName, pszEscData);
			}
			else
			{
				_sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("DELETE FROM graph_acl WHERE graph_id=%d"), graphId);
				DBQuery(g_hCoreDB, szQuery);

				_sntprintf(szQuery, 16384, _T("UPDATE graphs SET name='%s',config='%s' WHERE graph_id=%d"),
				           pszEscName, pszEscData, graphId);
			}

			if (DBQuery(hdb, szQuery))
			{
				// Insert new ACL
				nACLSize = (int)pRequest->getFieldAsUInt32(VID_ACL_SIZE);
				for(i = 0, id = VID_GRAPH_ACL_BASE, bSuccess = TRUE; i < nACLSize; i++)
				{
					graphUserId = pRequest->getFieldAsUInt32(id++);
					graphAccess = pRequest->getFieldAsUInt32(id++);
					_sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("INSERT INTO graph_acl (graph_id,user_id,user_rights) VALUES (%d,%d,%d)"),
					          graphId, graphUserId, graphAccess);
					if (!DBQuery(hdb, szQuery))
					{
						bSuccess = FALSE;
						msg.setField(VID_RCC, RCC_DB_FAILURE);
						break;
					}
				}
			}
			else
			{
				msg.setField(VID_RCC, RCC_DB_FAILURE);
			}

			if (bSuccess)
			{
				DBCommit(hdb);
				msg.setField(VID_RCC, RCC_SUCCESS);
				msg.setField(VID_GRAPH_ID, graphId);

            //send notificaion
				NXCPMessage update;
				int dwId = VID_GRAPH_LIST_BASE;
            update.setCode(CMD_GRAPH_UPDATE);
            update.setField(dwId++, graphId);
            update.setField(dwId++, m_dwUserId);
            update.setField(dwId++, dwGraphName);
            update.setField(dwId++, pszEscData);

            int nACLSize;
            GRAPH_ACL_ENTRY *pACL = LoadGraphACL(hdb, 0, &nACLSize);
            UINT32 *pdwUsers, *pdwRights;
            pdwUsers = (UINT32 *)malloc(sizeof(UINT32) * nACLSize);
            pdwRights = (UINT32 *)malloc(sizeof(UINT32) * nACLSize);
            // ACL for graph
            for(int j = 0; j < nACLSize; j++)
            {
               pdwUsers[j] = pACL[j].dwUserId;
               pdwRights[j] = pACL[j].dwAccess;
            }
            msg.setField(dwId++, nACLSize);
            msg.setFieldFromInt32Array(dwId++, nACLSize, pdwUsers);
            msg.setFieldFromInt32Array(dwId++, nACLSize, pdwRights);
            msg.setField(VID_NUM_GRAPHS, 1);

            NotifyClientGraphUpdate(&update, graphId);
			}
			else
			{
				DBRollback(hdb);
			}
			free(pszEscName);
			free(pszEscData);
		}
		else
		{
			msg.setField(VID_RCC, RCC_DB_FAILURE);
		}
      DBConnectionPoolReleaseConnection(hdb);
	}

   sendMessage(&msg);
}

/**
 * Delete graph
 */
void ClientSession::deleteGraph(NXCPMessage *pRequest)
{
   NXCPMessage msg;
	UINT32 dwGraphId, dwOwner;
	GRAPH_ACL_ENTRY *pACL = NULL;
	int nACLSize;
	DB_RESULT hResult;
	TCHAR szQuery[256];

   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(pRequest->getId());

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

	dwGraphId = pRequest->getFieldAsUInt32(VID_GRAPH_ID);
	_sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("SELECT owner_id FROM graphs WHERE graph_id=%d"), dwGraphId);
	hResult = DBSelect(hdb, szQuery);
	if (hResult != NULL)
	{
		if (DBGetNumRows(hResult) > 0)
		{
			dwOwner = DBGetFieldULong(hResult, 0, 0);
			pACL = LoadGraphACL(hdb, dwGraphId, &nACLSize);
			if (nACLSize != -1)
			{
				if ((m_dwUserId == 0) ||
				    (m_dwUserId == dwOwner) ||
				    CheckGraphAccess(pACL, nACLSize, dwGraphId, m_dwUserId, NXGRAPH_ACCESS_READ))
				{
					_sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("DELETE FROM graphs WHERE graph_id=%d"), dwGraphId);
					if (DBQuery(hdb, szQuery))
					{
						_sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("DELETE FROM graph_acl WHERE graph_id=%d"), dwGraphId);
						DBQuery(hdb, szQuery);
						msg.setField(VID_RCC, RCC_SUCCESS);
						NotifyClientSessions(NX_NOTIFY_GRAPHS_DELETED, dwGraphId);
					}
					else
					{
						msg.setField(VID_RCC, RCC_DB_FAILURE);
					}
				}
				else
				{
					msg.setField(VID_RCC, RCC_ACCESS_DENIED);
				}
				safe_free(pACL);
			}
			else
			{
				msg.setField(VID_RCC, RCC_DB_FAILURE);
			}
		}
		else
		{
			msg.setField(VID_RCC, RCC_INVALID_GRAPH_ID);
		}
		DBFreeResult(hResult);
	}
	else
	{
		msg.setField(VID_RCC, RCC_DB_FAILURE);
	}

   DBConnectionPoolReleaseConnection(hdb);
   sendMessage(&msg);
}

/**
 * Send list of DCIs to be shown in performance tab
 */
void ClientSession::sendPerfTabDCIList(NXCPMessage *pRequest)
{
   NXCPMessage msg;
	NetObj *object;

	msg.setId(pRequest->getId());
	msg.setCode(CMD_REQUEST_COMPLETED);

	object = FindObjectById(pRequest->getFieldAsUInt32(VID_OBJECT_ID));
	if (object != NULL)
	{
		if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
		{
			if (object->getObjectClass() == OBJECT_NODE)
			{
				msg.setField(VID_RCC, ((Node *)object)->getPerfTabDCIList(&msg));
			}
			else
			{
				msg.setField(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
			}
		}
		else
		{
			msg.setField(VID_RCC, RCC_ACCESS_DENIED);
		}
	}
	else
	{
		msg.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
	}

   sendMessage(&msg);
}


//
// Add CA certificate
//

void ClientSession::AddCACertificate(NXCPMessage *pRequest)
{
   NXCPMessage msg;
#ifdef _WITH_ENCRYPTION
	UINT32 dwLen, dwQLen, dwCertId;
	BYTE *pData;
	OPENSSL_CONST BYTE *p;
	X509 *pCert;
	TCHAR *pszQuery, *pszEscSubject, *pszComments, *pszEscComments;
#endif

	msg.setId(pRequest->getId());
	msg.setCode(CMD_REQUEST_COMPLETED);

	if ((m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_USERS) &&
	    (m_dwSystemAccess & SYSTEM_ACCESS_SERVER_CONFIG))
	{
#ifdef _WITH_ENCRYPTION
		dwLen = pRequest->getFieldAsBinary(VID_CERTIFICATE, NULL, 0);
		if (dwLen > 0)
		{
			pData = (BYTE *)malloc(dwLen);
			pRequest->getFieldAsBinary(VID_CERTIFICATE, pData, dwLen);

			// Validate certificate
			p = pData;
			pCert = d2i_X509(NULL, &p, dwLen);
			if (pCert != NULL)
			{
#ifdef UNICODE
				WCHAR *wname = WideStringFromMBString(CHECK_NULL_A(pCert->name));
				pszEscSubject = EncodeSQLString(wname);
				free(wname);
#else
				pszEscSubject = EncodeSQLString(CHECK_NULL(pCert->name));
#endif
				X509_free(pCert);
				pszComments = pRequest->getFieldAsString(VID_COMMENTS);
				pszEscComments = EncodeSQLString(pszComments);
				free(pszComments);
				dwCertId = CreateUniqueId(IDG_CERTIFICATE);
				dwQLen = dwLen * 2 + (UINT32)_tcslen(pszEscComments) + (UINT32)_tcslen(pszEscSubject) + 256;
				pszQuery = (TCHAR *)malloc(dwQLen * sizeof(TCHAR));
				_sntprintf(pszQuery, dwQLen, _T("INSERT INTO certificates (cert_id,cert_type,subject,comments,cert_data) VALUES (%d,%d,'%s','%s','"),
				           dwCertId, CERT_TYPE_TRUSTED_CA, pszEscSubject, pszEscComments);
				free(pszEscSubject);
				free(pszEscComments);
				BinToStr(pData, dwLen, &pszQuery[_tcslen(pszQuery)]);
				_tcscat(pszQuery, _T("')"));
				if (DBQuery(g_hCoreDB, pszQuery))
				{
               NotifyClientSessions(NX_NOTIFY_CERTIFICATE_CHANGED, dwCertId);
					msg.setField(VID_RCC, RCC_SUCCESS);
					ReloadCertificates();
				}
				else
				{
					msg.setField(VID_RCC, RCC_DB_FAILURE);
				}
				free(pszQuery);
			}
			else
			{
				msg.setField(VID_RCC, RCC_BAD_CERTIFICATE);
			}
			free(pData);
		}
		else
		{
			msg.setField(VID_RCC, RCC_INVALID_REQUEST);
		}
#else
		msg.setField(VID_RCC, RCC_NO_ENCRYPTION_SUPPORT);
#endif
	}
	else
	{
		msg.setField(VID_RCC, RCC_ACCESS_DENIED);
	}

	sendMessage(&msg);
}


//
// Delete certificate
//

void ClientSession::DeleteCertificate(NXCPMessage *pRequest)
{
   NXCPMessage msg;
#ifdef _WITH_ENCRYPTION
	UINT32 dwCertId;
	TCHAR szQuery[256];
#endif

	msg.setId(pRequest->getId());
	msg.setCode(CMD_REQUEST_COMPLETED);

	if ((m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_USERS) &&
	    (m_dwSystemAccess & SYSTEM_ACCESS_SERVER_CONFIG))
	{
#ifdef _WITH_ENCRYPTION
		dwCertId = pRequest->getFieldAsUInt32(VID_CERTIFICATE_ID);
		_sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("DELETE FROM certificates WHERE cert_id=%d"), dwCertId);
		if (DBQuery(g_hCoreDB, szQuery))
		{
			msg.setField(VID_RCC, RCC_SUCCESS);
			NotifyClientSessions(NX_NOTIFY_CERTIFICATE_CHANGED, dwCertId);
			ReloadCertificates();
		}
		else
		{
			msg.setField(VID_RCC, RCC_DB_FAILURE);
		}
#else
		msg.setField(VID_RCC, RCC_NO_ENCRYPTION_SUPPORT);
#endif
	}
	else
	{
		msg.setField(VID_RCC, RCC_ACCESS_DENIED);
	}

	sendMessage(&msg);
}


//
// Update certificate's comments
//

void ClientSession::UpdateCertificateComments(NXCPMessage *pRequest)
{
	UINT32 dwCertId, qlen;
	TCHAR *pszQuery, *pszComments, *pszEscComments;
	DB_RESULT hResult;
   NXCPMessage msg;

	msg.setId(pRequest->getId());
	msg.setCode(CMD_REQUEST_COMPLETED);

	if ((m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_USERS) &&
	    (m_dwSystemAccess & SYSTEM_ACCESS_SERVER_CONFIG))
	{
		dwCertId = pRequest->getFieldAsUInt32(VID_CERTIFICATE_ID);
		pszComments= pRequest->getFieldAsString(VID_COMMENTS);
		if (pszComments != NULL)
		{
			pszEscComments = EncodeSQLString(pszComments);
			free(pszComments);
			qlen = (UINT32)_tcslen(pszEscComments) + 256;
			pszQuery = (TCHAR *)malloc(qlen * sizeof(TCHAR));
			_sntprintf(pszQuery, qlen, _T("SELECT subject FROM certificates WHERE cert_id=%d"), dwCertId);
			hResult = DBSelect(g_hCoreDB, pszQuery);
			if (hResult != NULL)
			{
				if (DBGetNumRows(hResult) > 0)
				{
					_sntprintf(pszQuery, qlen, _T("UPDATE certificates SET comments='%s' WHERE cert_id=%d"), pszEscComments, dwCertId);
					if (DBQuery(g_hCoreDB, pszQuery))
					{
                  NotifyClientSessions(NX_NOTIFY_CERTIFICATE_CHANGED, dwCertId);
						msg.setField(VID_RCC, RCC_SUCCESS);
					}
					else
					{
						msg.setField(VID_RCC, RCC_DB_FAILURE);
					}
				}
				else
				{
					msg.setField(VID_RCC, RCC_INVALID_CERT_ID);
				}
				DBFreeResult(hResult);
			}
			else
			{
				msg.setField(VID_RCC, RCC_DB_FAILURE);
			}
			free(pszEscComments);
			free(pszQuery);
		}
		else
		{
			msg.setField(VID_RCC, RCC_INVALID_REQUEST);
		}
	}
	else
	{
		msg.setField(VID_RCC, RCC_ACCESS_DENIED);
	}

	sendMessage(&msg);
}

/**
 * Send list of installed certificates to client
 */
void ClientSession::getCertificateList(UINT32 dwRqId)
{
   NXCPMessage msg;
	DB_RESULT hResult;
	int i, nRows;
	UINT32 dwId;
	TCHAR *pszText;

	msg.setId(dwRqId);
	msg.setCode(CMD_REQUEST_COMPLETED);

	if ((m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_USERS) &&
	    (m_dwSystemAccess & SYSTEM_ACCESS_SERVER_CONFIG))
	{
		hResult = DBSelect(g_hCoreDB, _T("SELECT cert_id,cert_type,comments,subject FROM certificates"));
		if (hResult != NULL)
		{
			nRows = DBGetNumRows(hResult);
			msg.setField(VID_RCC, RCC_SUCCESS);
			msg.setField(VID_NUM_CERTIFICATES, (UINT32)nRows);
			for(i = 0, dwId = VID_CERT_LIST_BASE; i < nRows; i++, dwId += 6)
			{
				msg.setField(dwId++, DBGetFieldULong(hResult, i, 0));
				msg.setField(dwId++, (WORD)DBGetFieldLong(hResult, i, 1));

				pszText = DBGetField(hResult, i, 2, NULL, 0);
				if (pszText != NULL)
				{
					DecodeSQLString(pszText);
					msg.setField(dwId++, pszText);
				}
				else
				{
					msg.setField(dwId++, _T(""));
				}

				pszText = DBGetField(hResult, i, 3, NULL, 0);
				if (pszText != NULL)
				{
					DecodeSQLString(pszText);
					msg.setField(dwId++, pszText);
				}
				else
				{
					msg.setField(dwId++, _T(""));
				}
			}
			DBFreeResult(hResult);
		}
		else
		{
			msg.setField(VID_RCC, RCC_DB_FAILURE);
		}
	}
	else
	{
		msg.setField(VID_RCC, RCC_ACCESS_DENIED);
	}

	sendMessage(&msg);
}

/**
 * Query layer 2 topology from device
 */
void ClientSession::queryL2Topology(NXCPMessage *pRequest)
{
   NXCPMessage msg;
	NetObj *object;
	UINT32 dwResult;

	msg.setId(pRequest->getId());
	msg.setCode(CMD_REQUEST_COMPLETED);

	object = FindObjectById(pRequest->getFieldAsUInt32(VID_OBJECT_ID));
	if (object != NULL)
	{
		if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
		{
			if (object->getObjectClass() == OBJECT_NODE)
			{
				nxmap_ObjList *pTopology;

				pTopology = ((Node *)object)->getL2Topology();
				if (pTopology == NULL)
				{
					pTopology = ((Node *)object)->buildL2Topology(&dwResult, -1, true);
				}
				else
				{
					dwResult = RCC_SUCCESS;
				}
				if (pTopology != NULL)
				{
					msg.setField(VID_RCC, RCC_SUCCESS);
					pTopology->createMessage(&msg);
					delete pTopology;
				}
				else
				{
					msg.setField(VID_RCC, dwResult);
				}
			}
			else
			{
				msg.setField(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
			}
		}
		else
		{
			msg.setField(VID_RCC, RCC_ACCESS_DENIED);
		}
	}
	else
	{
		msg.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
	}

	sendMessage(&msg);
}

/**
 * Send SMS
 */
void ClientSession::sendSMS(NXCPMessage *pRequest)
{
   NXCPMessage msg;
	TCHAR phone[256], message[256];

	msg.setId(pRequest->getId());
	msg.setCode(CMD_REQUEST_COMPLETED);

	if ((m_dwSystemAccess & SYSTEM_ACCESS_SEND_SMS) && ConfigReadInt(_T("AllowDirectSMS"), 0))
	{
		pRequest->getFieldAsString(VID_RCPT_ADDR, phone, 256);
		pRequest->getFieldAsString(VID_MESSAGE, message, 256);
		PostSMS(phone, message);
		msg.setField(VID_RCC, RCC_SUCCESS);
	}
	else
	{
		msg.setField(VID_RCC, RCC_ACCESS_DENIED);
	}

	sendMessage(&msg);
}

/**
 * Send SNMP community list
 */
void ClientSession::SendCommunityList(UINT32 dwRqId)
{
   NXCPMessage msg;
	int i, count;
	UINT32 id;
	TCHAR buffer[256];
	DB_RESULT hResult;

	msg.setId(dwRqId);
	msg.setCode(CMD_REQUEST_COMPLETED);

	if (m_dwSystemAccess & SYSTEM_ACCESS_SERVER_CONFIG)
	{
      DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
		hResult = DBSelect(hdb, _T("SELECT community FROM snmp_communities"));
		if (hResult != NULL)
		{
			count = DBGetNumRows(hResult);
			msg.setField(VID_NUM_STRINGS, (UINT32)count);
			for(i = 0, id = VID_STRING_LIST_BASE; i < count; i++)
			{
				DBGetField(hResult, i, 0, buffer, 256);
				msg.setField(id++, buffer);
			}
			DBFreeResult(hResult);
			msg.setField(VID_RCC, RCC_SUCCESS);
		}
		else
		{
			msg.setField(VID_RCC, RCC_DB_FAILURE);
		}
      DBConnectionPoolReleaseConnection(hdb);
	}
	else
	{
		msg.setField(VID_RCC, RCC_ACCESS_DENIED);
	}

	sendMessage(&msg);
}

/**
 * Update SNMP community list
 */
void ClientSession::UpdateCommunityList(NXCPMessage *pRequest)
{
   NXCPMessage msg;
	TCHAR value[256], query[1024];
	int i, count;
	UINT32 id;

	msg.setId(pRequest->getId());
	msg.setCode(CMD_REQUEST_COMPLETED);

	if (m_dwSystemAccess & SYSTEM_ACCESS_SERVER_CONFIG)
	{
      DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
		if (DBBegin(hdb))
		{
			DBQuery(hdb, _T("DELETE FROM snmp_communities"));
			count = pRequest->getFieldAsUInt32(VID_NUM_STRINGS);
			for(i = 0, id = VID_STRING_LIST_BASE; i < count; i++)
			{
				pRequest->getFieldAsString(id++, value, 256);
				String escValue = DBPrepareString(hdb, value);
				_sntprintf(query, 1024, _T("INSERT INTO snmp_communities (id,community) VALUES(%d,%s)"), i + 1, (const TCHAR *)escValue);
				if (!DBQuery(hdb, query))
					break;
			}

			if (i == count)
			{
				DBCommit(hdb);
				msg.setField(VID_RCC, RCC_SUCCESS);
			}
			else
			{
				DBRollback(hdb);
				msg.setField(VID_RCC, RCC_DB_FAILURE);
			}
		}
		else
		{
			msg.setField(VID_RCC, RCC_DB_FAILURE);
		}
      DBConnectionPoolReleaseConnection(hdb);
	}
	else
	{
		msg.setField(VID_RCC, RCC_ACCESS_DENIED);
	}

	sendMessage(&msg);
}

/**
 * Send situation list to client
 */
void ClientSession::getSituationList(UINT32 dwRqId)
{
   NXCPMessage msg;

	msg.setId(dwRqId);
	msg.setCode(CMD_REQUEST_COMPLETED);

	if (m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_SITUATIONS)
	{
		MutexLock(m_mutexSendSituations);
		SendSituationListToClient(this, &msg);
		MutexUnlock(m_mutexSendSituations);
	}
	else
	{
		msg.setField(VID_RCC, RCC_ACCESS_DENIED);
		sendMessage(&msg);
	}
}

/**
 * Create new situation
 */
void ClientSession::createSituation(NXCPMessage *pRequest)
{
   NXCPMessage msg;
   Situation *st;
   TCHAR name[MAX_DB_STRING];

	msg.setId(pRequest->getId());
	msg.setCode(CMD_REQUEST_COMPLETED);

	if (m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_SITUATIONS)
	{
		pRequest->getFieldAsString(VID_NAME, name, MAX_DB_STRING);
		st = ::CreateSituation(name);
		if (st != NULL)
		{
			msg.setField(VID_RCC, RCC_SUCCESS);
			msg.setField(VID_SITUATION_ID, st->getId());
		}
		else
		{
			msg.setField(VID_RCC, RCC_INTERNAL_ERROR);
		}
	}
	else
	{
		msg.setField(VID_RCC, RCC_ACCESS_DENIED);
	}

	sendMessage(&msg);
}

/**
 * Update situation
 */
void ClientSession::updateSituation(NXCPMessage *pRequest)
{
   NXCPMessage msg;
   Situation *st;

	msg.setId(pRequest->getId());
	msg.setCode(CMD_REQUEST_COMPLETED);

	if (m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_SITUATIONS)
	{
		st = FindSituationById(pRequest->getFieldAsUInt32(VID_SITUATION_ID));
		if (st != NULL)
		{
			st->UpdateFromMessage(pRequest);
			msg.setField(VID_RCC, RCC_SUCCESS);
		}
		else
		{
			msg.setField(VID_RCC, RCC_INVALID_SITUATION_ID);
		}
	}
	else
	{
		msg.setField(VID_RCC, RCC_ACCESS_DENIED);
	}

	sendMessage(&msg);
}

/**
 * Delete situation
 */
void ClientSession::deleteSituation(NXCPMessage *pRequest)
{
   NXCPMessage msg;

	msg.setId(pRequest->getId());
	msg.setCode(CMD_REQUEST_COMPLETED);

	if (m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_SITUATIONS)
	{
		msg.setField(VID_RCC, ::DeleteSituation(pRequest->getFieldAsUInt32(VID_SITUATION_ID)));
	}
	else
	{
		msg.setField(VID_RCC, RCC_ACCESS_DENIED);
	}

	sendMessage(&msg);
}

/**
 * Delete situation instance
 */
void ClientSession::deleteSituationInstance(NXCPMessage *pRequest)
{
   NXCPMessage msg;
   Situation *st;

	msg.setId(pRequest->getId());
	msg.setCode(CMD_REQUEST_COMPLETED);

	if (m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_SITUATIONS)
	{
		st = FindSituationById(pRequest->getFieldAsUInt32(VID_SITUATION_ID));
		if (st != NULL)
		{
			TCHAR instance[MAX_DB_STRING];

			pRequest->getFieldAsString(VID_SITUATION_INSTANCE, instance, MAX_DB_STRING);
			msg.setField(VID_RCC, st->DeleteInstance(instance) ? RCC_SUCCESS : RCC_INSTANCE_NOT_FOUND);
		}
		else
		{
			msg.setField(VID_RCC, RCC_INVALID_SITUATION_ID);
		}
	}
	else
	{
		msg.setField(VID_RCC, RCC_ACCESS_DENIED);
	}

	sendMessage(&msg);
}

/**
 * Handler for situation chage
 */
void ClientSession::onSituationChange(NXCPMessage *msg)
{
   UPDATE_INFO *pUpdate;

   if (isAuthenticated() && (m_dwActiveChannels & NXC_CHANNEL_SITUATIONS))
   {
      pUpdate = (UPDATE_INFO *)malloc(sizeof(UPDATE_INFO));
      pUpdate->dwCategory = INFO_CAT_SITUATION;
      pUpdate->pData = new NXCPMessage(msg);
      m_pUpdateQueue->put(pUpdate);
   }
}

/**
 * Register agent
 */
void ClientSession::registerAgent(NXCPMessage *pRequest)
{
   NXCPMessage msg;
	Node *node;

   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(pRequest->getId());

	if (ConfigReadInt(_T("EnableAgentRegistration"), 0))
	{
		if (m_clientAddr->sa_family == AF_INET)
		{
			node = FindNodeByIP(0, ntohl(((struct sockaddr_in *)m_clientAddr)->sin_addr.s_addr));
			if (node != NULL)
			{
				// Node already exist, force configuration poll
				node->setRecheckCapsFlag();
				node->forceConfigurationPoll();
			}
			else
			{
				NEW_NODE *info;

				info = (NEW_NODE *)malloc(sizeof(NEW_NODE));
            info->ipAddr = InetAddress::createFromSockaddr(m_clientAddr);
				info->zoneId = 0;	// Add to default zone
				info->ignoreFilter = TRUE;		// Ignore discovery filters and add node anyway
				g_nodePollerQueue.put(info);
			}
			msg.setField(VID_RCC, RCC_SUCCESS);
		}
		else
		{
			msg.setField(VID_RCC, RCC_NOT_IMPLEMENTED);
		}
	}
	else
	{
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);
	}

   sendMessage(&msg);
}

/**
 * Get file from server
 */
void ClientSession::getServerFile(NXCPMessage *pRequest)
{
   NXCPMessage msg;
	TCHAR name[MAX_PATH], fname[MAX_PATH];
	bool musicFile = false;

   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(pRequest->getId());
   pRequest->getFieldAsString(VID_FILE_NAME, name, MAX_PATH);
   for(int i = 0; i < m_musicTypeList.size(); i++)
   {
      TCHAR *extension = _tcsrchr(name, _T('.'));
      if (extension != NULL)
      {
         extension++;
         if(!_tcscmp(extension, m_musicTypeList.get(i)))
         {
            musicFile = true;
            break;
         }
      }
   }

	if ((m_dwSystemAccess & SYSTEM_ACCESS_READ_FILES) || musicFile)
	{
      _tcscpy(fname, g_netxmsdDataDir);
      _tcscat(fname, DDIR_FILES);
      _tcscat(fname, FS_PATH_SEPARATOR);
      _tcscat(fname, GetCleanFileName(name));
		debugPrintf(4, _T("Requested file %s"), fname);
		if (_taccess(fname, 0) == 0)
		{
			debugPrintf(5, _T("Sending file %s"), fname);
			if (SendFileOverNXCP(m_hSocket, pRequest->getId(), fname, m_pCtx, 0, NULL, NULL, m_mutexSocketWrite))
			{
				debugPrintf(5, _T("File %s was succesfully sent"), fname);
		      msg.setField(VID_RCC, RCC_SUCCESS);
			}
			else
			{
				debugPrintf(5, _T("Unable to send file %s: SendFileOverNXCP() failed"), fname);
		      msg.setField(VID_RCC, RCC_IO_ERROR);
			}
		}
		else
		{
			debugPrintf(5, _T("Unable to send file %s: access() failed"), fname);
	      msg.setField(VID_RCC, RCC_IO_ERROR);
		}
	}
	else
	{
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);
	}

   sendMessage(&msg);
}

/**
 * Get file from agent
 */
void ClientSession::getAgentFile(NXCPMessage *request)
{
   NXCPMessage msg;
	TCHAR remoteFile[MAX_PATH];

   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(request->getId());

	NetObj *object = FindObjectById(request->getFieldAsUInt32(VID_OBJECT_ID));
	if (object != NULL)
	{
		if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_DOWNLOAD))
		{
			if (object->getObjectClass() == OBJECT_NODE)
			{
				request->getFieldAsString(VID_FILE_NAME, remoteFile, MAX_PATH);
            bool follow = request->getFieldAsUInt16(VID_FILE_FOLLOW) ? true : false;
				FileDownloadJob *job = new FileDownloadJob((Node *)object, remoteFile, request->getFieldAsUInt32(VID_FILE_SIZE_LIMIT), follow, this, request->getId());
				msg.setField(VID_NAME, job->getLocalFileName());
				msg.setField(VID_RCC, AddJob(job) ? RCC_SUCCESS : RCC_INTERNAL_ERROR);
			}
			else
			{
				msg.setField(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
			}
		}
		else
		{
			msg.setField(VID_RCC, RCC_ACCESS_DENIED);
		}
	}
	else
	{
		msg.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
	}

   sendMessage(&msg);
}

/**
 * Cancel file monitoring
 */
void ClientSession::cancelFileMonitoring(NXCPMessage *request)
{
   NXCPMessage msg;
   NXCPMessage* response;
	TCHAR remoteFile[MAX_PATH];
	UINT32 rcc = 0xFFFFFFFF;

   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(request->getId());

	NetObj *object = FindObjectById(request->getFieldAsUInt32(VID_OBJECT_ID));
	if (object != NULL)
	{
      if (object->getObjectClass() == OBJECT_NODE)
      {
         request->getFieldAsString(VID_FILE_NAME, remoteFile, MAX_PATH);

         MONITORED_FILE * newFile = new MONITORED_FILE();
         _tcscpy(newFile->fileName, remoteFile);
         newFile->nodeID = object->getId();
         newFile->session = this;
         g_monitoringList.removeMonitoringFile(newFile);
         delete newFile;

         Node *node = (Node *)object;
         node->incRefCount();
         AgentConnection *conn = node->createAgentConnection();
         debugPrintf(6, _T("Cancel file monitoring %s"), remoteFile);
         if(conn != NULL)
         {
            request->setId(conn->generateRequestId());
            response = conn->customRequest(request);
            if (response != NULL)
            {
               rcc = response->getFieldAsUInt32(VID_RCC);
               if (rcc == ERR_SUCCESS)
               {
                  msg.setField(VID_RCC, rcc);
                  debugPrintf(6, _T("File monitoring cancelled successfully"));
               }
               else
               {
                  msg.setField(VID_RCC, AgentErrorToRCC(rcc));
                  debugPrintf(6, _T("Error on agent: %d (%s)"), rcc, AgentErrorCodeToText(rcc));
               }
            }
            else
            {
               msg.setField(VID_RCC, RCC_INTERNAL_ERROR);
            }
            delete response;
            delete conn;
         }
         else
         {
            msg.setField(VID_RCC, RCC_INTERNAL_ERROR);
            debugPrintf(6, _T("Connection with node have been lost"));
         }
         node->decRefCount();
      }
      else
      {
         msg.setField(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
      }
	}
	else
	{
		msg.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
	}
   sendMessage(&msg);
}

/**
 * Test DCI transformation script
 */
void ClientSession::testDCITransformation(NXCPMessage *pRequest)
{
   NXCPMessage msg;
   NetObj *object;

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(pRequest->getId());

   // Get node id and check object class and access rights
   object = FindObjectById(pRequest->getFieldAsUInt32(VID_OBJECT_ID));
   if (object != NULL)
   {
      if ((object->getObjectClass() == OBJECT_NODE) || (object->getObjectClass() == OBJECT_CLUSTER) || (object->getObjectClass() == OBJECT_MOBILEDEVICE))
      {
         if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
         {
				BOOL success;
				TCHAR *script, value[256], result[256];

				script = pRequest->getFieldAsString(VID_SCRIPT);
				if (script != NULL)
				{
					pRequest->getFieldAsString(VID_VALUE, value, sizeof(value) / sizeof(TCHAR));
               success = DCItem::testTransformation((DataCollectionTarget *)object, script, value, result, sizeof(result) / sizeof(TCHAR));
					free(script);
					msg.setField(VID_RCC, RCC_SUCCESS);
					msg.setField(VID_EXECUTION_STATUS, (WORD)success);
					msg.setField(VID_EXECUTION_RESULT, result);
				}
				else
				{
	            msg.setField(VID_RCC, RCC_INVALID_ARGUMENT);
				}
         }
         else  // User doesn't have READ rights on object
         {
            msg.setField(VID_RCC, RCC_ACCESS_DENIED);
         }
      }
      else     // Object is not a node
      {
         msg.setField(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
      }
   }
   else  // No object with given ID
   {
      msg.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Execute script in object's context
 */
void ClientSession::executeScript(NXCPMessage *request)
{
   NXCPMessage msg;
   bool success = false;
   NXSL_VM *vm = NULL;

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(request->getId());

   // Get node id and check object class and access rights
   NetObj *object = FindObjectById(request->getFieldAsUInt32(VID_OBJECT_ID));
   if (object != NULL)
   {
      if ((object->getObjectClass() == OBJECT_NODE) ||
          (object->getObjectClass() == OBJECT_CLUSTER) ||
          (object->getObjectClass() == OBJECT_MOBILEDEVICE) ||
          (object->getObjectClass() == OBJECT_CONTAINER) ||
          (object->getObjectClass() == OBJECT_ZONE) ||
          (object->getObjectClass() == OBJECT_SUBNET))
      {
         if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_MODIFY))
         {
				TCHAR *script = request->getFieldAsString(VID_SCRIPT);
				if (script != NULL)
				{
               TCHAR errorMessage[256];
               vm = NXSLCompileAndCreateVM(script, errorMessage, 256, new NXSL_ClientSessionEnv(this, &msg));
               if (vm != NULL)
               {
                  vm->setGlobalVariable(_T("$object"), new NXSL_Value(new NXSL_Object(&g_nxslNetObjClass, object)));
                  if (object->getObjectClass() == OBJECT_NODE)
                  {
                     vm->setGlobalVariable(_T("$node"), new NXSL_Value(new NXSL_Object(&g_nxslNodeClass, object)));
                  }
                  msg.setField(VID_RCC, RCC_SUCCESS);
                  sendMessage(&msg);
                  success = true;
               }
               else
               {
                  msg.setField(VID_RCC, RCC_NXSL_COMPILATION_ERROR);
                  msg.setField(VID_ERROR_TEXT, errorMessage);
               }
					safe_free(script);
				}
				else
				{
	            msg.setField(VID_RCC, RCC_INVALID_ARGUMENT);
				}
         }
         else  // User doesn't have READ rights on object
         {
            msg.setField(VID_RCC, RCC_ACCESS_DENIED);
         }
      }
      else     // Object is not a node
      {
         msg.setField(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
      }
   }
   else  // No object with given ID
   {
      msg.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   // start execution
   if (success)
   {
      msg.setCode(CMD_EXECUTE_SCRIPT_UPDATE);
      if (vm->run())
      {
         TCHAR buffer[1024];
         const TCHAR *value = vm->getResult()->getValueAsCString();
         _sntprintf(buffer, 1024, _T("\n\n*** FINISHED ***\n\nResult: %s\n\n"), CHECK_NULL(value));
         msg.setField(VID_MESSAGE, buffer);
			msg.setField(VID_RCC, RCC_SUCCESS);
         msg.setEndOfSequence();
         sendMessage(&msg);
      }
      else
      {
         msg.setField(VID_ERROR_TEXT, vm->getErrorText());
			msg.setField(VID_RCC, RCC_NXSL_EXECUTION_ERROR);
         msg.setEndOfSequence();
         sendMessage(&msg);
      }
      delete vm;
   }
   else
   {
      // Send response
      sendMessage(&msg);
   }
}

/**
 * Send list of server jobs
 */
void ClientSession::sendJobList(UINT32 dwRqId)
{
	NXCPMessage msg;

	msg.setCode(CMD_REQUEST_COMPLETED);
	msg.setId(dwRqId);
	msg.setField(VID_RCC, RCC_SUCCESS);
	GetJobList(&msg);
	sendMessage(&msg);
}

/**
 * Cancel server job
 */
void ClientSession::cancelJob(NXCPMessage *request)
{
	NXCPMessage msg;

	msg.setCode(CMD_REQUEST_COMPLETED);
	msg.setId(request->getId());
	msg.setField(VID_RCC, ::CancelJob(m_dwUserId, request));
	sendMessage(&msg);
}

/**
 * put server job on hold
 */
void ClientSession::holdJob(NXCPMessage *request)
{
	NXCPMessage msg;

	msg.setCode(CMD_REQUEST_COMPLETED);
	msg.setId(request->getId());
	msg.setField(VID_RCC, ::HoldJob(m_dwUserId, request));
	sendMessage(&msg);
}

/**
 * Allow server job on hold for execution
 */
void ClientSession::unholdJob(NXCPMessage *request)
{
	NXCPMessage msg;

	msg.setCode(CMD_REQUEST_COMPLETED);
	msg.setId(request->getId());
	msg.setField(VID_RCC, ::UnholdJob(m_dwUserId, request));
	sendMessage(&msg);
}

/**
 * Deploy agent policy
 */
void ClientSession::deployAgentPolicy(NXCPMessage *request, bool uninstallFlag)
{
	NXCPMessage msg;

	msg.setCode(CMD_REQUEST_COMPLETED);
	msg.setId(request->getId());

	UINT32 policyId = request->getFieldAsUInt32(VID_POLICY_ID);
	UINT32 targetId = request->getFieldAsUInt32(VID_OBJECT_ID);

	NetObj *policy = FindObjectById(policyId);
	if ((policy != NULL) && (policy->getObjectClass() >= OBJECT_AGENTPOLICY))
	{
		NetObj *target = FindObjectById(targetId);
		if ((target != NULL) && (target->getObjectClass() == OBJECT_NODE))
		{
			if (target->checkAccessRights(m_dwUserId, OBJECT_ACCESS_CONTROL) &&
			    policy->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
			{
				if (((Node *)target)->isNativeAgent())
				{
					ServerJob *job;
					if (uninstallFlag)
						job = new PolicyUninstallJob((Node *)target, (AgentPolicy *)policy, m_dwUserId);
					else
						job = new PolicyDeploymentJob((Node *)target, (AgentPolicy *)policy, m_dwUserId);
					if (AddJob(job))
					{
						msg.setField(VID_RCC, RCC_SUCCESS);
					}
					else
					{
						delete job;
						msg.setField(VID_RCC, RCC_INTERNAL_ERROR);
					}
				}
				else
				{
					msg.setField(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
				}
			}
			else
			{
				msg.setField(VID_RCC, RCC_ACCESS_DENIED);
			}
		}
		else
		{
			msg.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
		}
	}
	else
	{
		msg.setField(VID_RCC, RCC_INVALID_POLICY_ID);
	}

	sendMessage(&msg);
}

/**
 * Get custom attribute for current user
 */
void ClientSession::getUserCustomAttribute(NXCPMessage *request)
{
	NXCPMessage msg;

	msg.setCode(CMD_REQUEST_COMPLETED);
	msg.setId(request->getId());

	TCHAR *name = request->getFieldAsString(VID_NAME);
	if ((name != NULL) && (*name == _T('.')))
	{
		const TCHAR *value = GetUserDbObjectAttr(m_dwUserId, name);
		msg.setField(VID_VALUE, CHECK_NULL_EX(value));
		msg.setField(VID_RCC, RCC_SUCCESS);
	}
	else
	{
		msg.setField(VID_RCC, RCC_ACCESS_DENIED);
	}
	safe_free(name);

	sendMessage(&msg);
}

/**
 * Set custom attribute for current user
 */
void ClientSession::setUserCustomAttribute(NXCPMessage *request)
{
	NXCPMessage msg;

	msg.setCode(CMD_REQUEST_COMPLETED);
	msg.setId(request->getId());

	TCHAR *name = request->getFieldAsString(VID_NAME);
	if ((name != NULL) && (*name == _T('.')))
	{
		TCHAR *value = request->getFieldAsString(VID_VALUE);
		SetUserDbObjectAttr(m_dwUserId, name, CHECK_NULL_EX(value));
		msg.setField(VID_RCC, RCC_SUCCESS);
		safe_free(value);
	}
	else
	{
		msg.setField(VID_RCC, RCC_ACCESS_DENIED);
	}
	safe_free(name);

	sendMessage(&msg);
}

/**
 * Open server log
 */
void ClientSession::openServerLog(NXCPMessage *request)
{
	NXCPMessage msg;

	msg.setCode(CMD_REQUEST_COMPLETED);
	msg.setId(request->getId());

	TCHAR name[256];
	request->getFieldAsString(VID_LOG_NAME, name, 256);

	UINT32 rcc;
	int handle = OpenLog(name, this, &rcc);
	if (handle != -1)
	{
		msg.setField(VID_RCC, RCC_SUCCESS);
		msg.setField(VID_LOG_HANDLE, (UINT32)handle);

		LogHandle *log = AcquireLogHandleObject(this, handle);
		log->getColumnInfo(msg);
		log->unlock();
	}
	else
	{
		msg.setField(VID_RCC, rcc);
	}

	sendMessage(&msg);
}

/**
 * Close server log
 */
void ClientSession::closeServerLog(NXCPMessage *request)
{
	NXCPMessage msg;

	msg.setCode(CMD_REQUEST_COMPLETED);
	msg.setId(request->getId());

	int handle = (int)request->getFieldAsUInt32(VID_LOG_HANDLE);
	msg.setField(VID_RCC, CloseLog(this, handle));

	sendMessage(&msg);
}

/**
 * Query server log
 */
void ClientSession::queryServerLog(NXCPMessage *request)
{
	NXCPMessage msg;

	msg.setCode(CMD_REQUEST_COMPLETED);
	msg.setId(request->getId());

	int handle = (int)request->getFieldAsUInt32(VID_LOG_HANDLE);
	LogHandle *log = AcquireLogHandleObject(this, handle);
	if (log != NULL)
	{
		INT64 rowCount;
		msg.setField(VID_RCC, log->query(new LogFilter(request), &rowCount, getUserId()) ? RCC_SUCCESS : RCC_DB_FAILURE);
		msg.setField(VID_NUM_ROWS, (QWORD)rowCount);
		log->unlock();
	}
	else
	{
		msg.setField(VID_RCC, RCC_INVALID_LOG_HANDLE);
	}

	sendMessage(&msg);
}

/**
 * Get log data from query result
 */
void ClientSession::getServerLogQueryData(NXCPMessage *request)
{
	NXCPMessage msg;
	Table *data = NULL;

	msg.setCode(CMD_REQUEST_COMPLETED);
	msg.setId(request->getId());

	int handle = (int)request->getFieldAsUInt32(VID_LOG_HANDLE);
	LogHandle *log = AcquireLogHandleObject(this, handle);
	if (log != NULL)
	{
		INT64 startRow = request->getFieldAsUInt64(VID_START_ROW);
		INT64 numRows = request->getFieldAsUInt64(VID_NUM_ROWS);
		bool refresh = request->getFieldAsUInt16(VID_FORCE_RELOAD) ? true : false;
		data = log->getData(startRow, numRows, refresh, getUserId()); // pass user id from session
		log->unlock();
		if (data != NULL)
		{
			msg.setField(VID_RCC, RCC_SUCCESS);
		}
		else
		{
			msg.setField(VID_RCC, RCC_DB_FAILURE);
		}
	}
	else
	{
		msg.setField(VID_RCC, RCC_INVALID_LOG_HANDLE);
	}

	sendMessage(&msg);

	if (data != NULL)
	{
		msg.setCode(CMD_LOG_DATA);
		int offset = 0;
		do
		{
			msg.deleteAllFields();
			offset = data->fillMessage(msg, offset, 200);
			sendMessage(&msg);
		} while(offset < data->getNumRows());
		delete data;
	}
}

/**
 * Send SNMP v3 USM credentials
 */
void ClientSession::sendUsmCredentials(UINT32 dwRqId)
{
   NXCPMessage msg;
	int i, count;
	UINT32 id;
	TCHAR buffer[MAX_DB_STRING];
	DB_RESULT hResult;

	msg.setId(dwRqId);
	msg.setCode(CMD_REQUEST_COMPLETED);

	if (m_dwSystemAccess & SYSTEM_ACCESS_SERVER_CONFIG)
	{
		hResult = DBSelect(g_hCoreDB, _T("SELECT user_name,auth_method,priv_method,auth_password,priv_password FROM usm_credentials"));
		if (hResult != NULL)
		{
			count = DBGetNumRows(hResult);
			msg.setField(VID_NUM_RECORDS, (UINT32)count);
			for(i = 0, id = VID_USM_CRED_LIST_BASE; i < count; i++, id += 5)
			{
				DBGetField(hResult, i, 0, buffer, MAX_DB_STRING);	// security name
				msg.setField(id++, buffer);

				msg.setField(id++, (WORD)DBGetFieldLong(hResult, i, 1));	// auth method
				msg.setField(id++, (WORD)DBGetFieldLong(hResult, i, 2));	// priv method

				DBGetField(hResult, i, 3, buffer, MAX_DB_STRING);	// auth password
				msg.setField(id++, buffer);

				DBGetField(hResult, i, 4, buffer, MAX_DB_STRING);	// priv password
				msg.setField(id++, buffer);
			}
			DBFreeResult(hResult);
			msg.setField(VID_RCC, RCC_SUCCESS);
		}
		else
		{
			msg.setField(VID_RCC, RCC_DB_FAILURE);
		}
	}
	else
	{
		msg.setField(VID_RCC, RCC_ACCESS_DENIED);
	}

	sendMessage(&msg);
}

/**
 * Update SNMP v3 USM credentials
 */
void ClientSession::updateUsmCredentials(NXCPMessage *request)
{
   NXCPMessage msg;

	msg.setId(request->getId());
	msg.setCode(CMD_REQUEST_COMPLETED);

	if (m_dwSystemAccess & SYSTEM_ACCESS_SERVER_CONFIG)
	{
		DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
		if (DBBegin(hdb))
		{
			TCHAR query[4096];
			UINT32 id;
			int i = -1;
			int count = (int)request->getFieldAsUInt32(VID_NUM_RECORDS);

			if (DBQuery(hdb, _T("DELETE FROM usm_credentials")))
			{
				for(i = 0, id = VID_USM_CRED_LIST_BASE; i < count; i++, id += 5)
				{
					TCHAR name[MAX_DB_STRING], authPasswd[MAX_DB_STRING], privPasswd[MAX_DB_STRING];

					request->getFieldAsString(id++, name, MAX_DB_STRING);
					int authMethod = (int)request->getFieldAsUInt16(id++);
					int privMethod = (int)request->getFieldAsUInt16(id++);
					request->getFieldAsString(id++, authPasswd, MAX_DB_STRING);
					request->getFieldAsString(id++, privPasswd, MAX_DB_STRING);
					_sntprintf(query, 4096, _T("INSERT INTO usm_credentials (id,user_name,auth_method,priv_method,auth_password,priv_password) VALUES(%d,%s,%d,%d,%s,%s)"),
								  i + 1, (const TCHAR *)DBPrepareString(g_hCoreDB, name), authMethod, privMethod,
								  (const TCHAR *)DBPrepareString(g_hCoreDB, authPasswd), (const TCHAR *)DBPrepareString(g_hCoreDB, privPasswd));
					if (!DBQuery(hdb, query))
						break;
				}
			}

			if (i == count)
			{
				DBCommit(hdb);
				msg.setField(VID_RCC, RCC_SUCCESS);
			}
			else
			{
				DBRollback(hdb);
				msg.setField(VID_RCC, RCC_DB_FAILURE);
			}
		}
		else
		{
			msg.setField(VID_RCC, RCC_DB_FAILURE);
		}
		DBConnectionPoolReleaseConnection(hdb);
	}
	else
	{
		msg.setField(VID_RCC, RCC_ACCESS_DENIED);
	}

	sendMessage(&msg);
}

/**
 * Find connection point for the node
 */
void ClientSession::findNodeConnection(NXCPMessage *request)
{
   NXCPMessage msg;

	msg.setId(request->getId());
	msg.setCode(CMD_REQUEST_COMPLETED);

	UINT32 objectId = request->getFieldAsUInt32(VID_OBJECT_ID);
	NetObj *object = FindObjectById(objectId);
	if ((object != NULL) && !object->isDeleted())
	{
		if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
		{
			debugPrintf(5, _T("findNodeConnection: objectId=%d class=%d name=\"%s\""), objectId, object->getObjectClass(), object->getName());
			NetObj *cp = NULL;
			UINT32 localNodeId, localIfId;
			BYTE localMacAddr[MAC_ADDR_LENGTH];
			int type = 0;
			if (object->getObjectClass() == OBJECT_NODE)
			{
				localNodeId = objectId;
				cp = ((Node *)object)->findConnectionPoint(&localIfId, localMacAddr, &type);
				msg.setField(VID_RCC, RCC_SUCCESS);
			}
			else if (object->getObjectClass() == OBJECT_INTERFACE)
			{
				localNodeId = ((Interface *)object)->getParentNode()->getId();
				localIfId = objectId;
				memcpy(localMacAddr, ((Interface *)object)->getMacAddr(), MAC_ADDR_LENGTH);
				cp = FindInterfaceConnectionPoint(localMacAddr, &type);
				msg.setField(VID_RCC, RCC_SUCCESS);
			}
         else if (object->getObjectClass() == OBJECT_ACCESSPOINT)
			{
				localNodeId = 0;
				localIfId = 0;
				memcpy(localMacAddr, ((AccessPoint *)object)->getMacAddr(), MAC_ADDR_LENGTH);
				cp = FindInterfaceConnectionPoint(localMacAddr, &type);
				msg.setField(VID_RCC, RCC_SUCCESS);
			}
			else
			{
				msg.setField(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
			}

			debugPrintf(5, _T("findNodeConnection: cp=%p type=%d"), cp, type);
			if (cp != NULL)
			{
            Node *node = (cp->getObjectClass() == OBJECT_INTERFACE) ? ((Interface *)cp)->getParentNode() : ((AccessPoint *)cp)->getParentNode();
            if (node != NULL)
            {
               msg.setField(VID_OBJECT_ID, node->getId());
				   msg.setField(VID_INTERFACE_ID, cp->getId());
               msg.setField(VID_IF_INDEX, (cp->getObjectClass() == OBJECT_INTERFACE) ? ((Interface *)cp)->getIfIndex() : (UINT32)0);
				   msg.setField(VID_LOCAL_NODE_ID, localNodeId);
				   msg.setField(VID_LOCAL_INTERFACE_ID, localIfId);
				   msg.setField(VID_MAC_ADDR, localMacAddr, MAC_ADDR_LENGTH);
				   msg.setField(VID_CONNECTION_TYPE, (UINT16)type);
               if (cp->getObjectClass() == OBJECT_INTERFACE)
                  debugPrintf(5, _T("findNodeConnection: nodeId=%d ifId=%d ifName=%s ifIndex=%d"), node->getId(), cp->getId(), cp->getName(), ((Interface *)cp)->getIfIndex());
               else
                  debugPrintf(5, _T("findNodeConnection: nodeId=%d apId=%d apName=%s"), node->getId(), cp->getId(), cp->getName());
            }
            else
            {
      			msg.setField(VID_RCC, RCC_INTERNAL_ERROR);
            }
			}
		}
		else
		{
			msg.setField(VID_RCC, RCC_ACCESS_DENIED);
		}
	}
	else
	{
		msg.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
	}

	sendMessage(&msg);
}

/**
 * Find connection port for given MAC address
 */
void ClientSession::findMacAddress(NXCPMessage *request)
{
   NXCPMessage msg;
	BYTE macAddr[6];

	msg.setId(request->getId());
	msg.setCode(CMD_REQUEST_COMPLETED);

	request->getFieldAsBinary(VID_MAC_ADDR, macAddr, 6);
	int type;
	NetObj *cp = FindInterfaceConnectionPoint(macAddr, &type);
	msg.setField(VID_RCC, RCC_SUCCESS);

	debugPrintf(5, _T("findMacAddress: cp=%p type=%d"), cp, type);
	if (cp != NULL)
	{
		UINT32 localNodeId, localIfId;

		Interface *localIf = FindInterfaceByMAC(macAddr);
		if (localIf != NULL)
		{
			localIfId = localIf->getId();
			localNodeId = localIf->getParentNode()->getId();
		}
		else
		{
			localIfId = 0;
			localNodeId = 0;
		}

      Node *node = (cp->getObjectClass() == OBJECT_INTERFACE) ? ((Interface *)cp)->getParentNode() : ((AccessPoint *)cp)->getParentNode();
      if (node != NULL)
      {
		   msg.setField(VID_OBJECT_ID, node->getId());
		   msg.setField(VID_INTERFACE_ID, cp->getId());
         msg.setField(VID_IF_INDEX, (cp->getObjectClass() == OBJECT_INTERFACE) ? ((Interface *)cp)->getIfIndex() : (UINT32)0);
	      msg.setField(VID_LOCAL_NODE_ID, localNodeId);
		   msg.setField(VID_LOCAL_INTERFACE_ID, localIfId);
		   msg.setField(VID_MAC_ADDR, macAddr, MAC_ADDR_LENGTH);
         msg.setField(VID_IP_ADDRESS, (localIf != NULL) ? localIf->getIpAddressList()->getFirstUnicastAddress() : InetAddress::INVALID);
		   msg.setField(VID_CONNECTION_TYPE, (UINT16)type);
         if (cp->getObjectClass() == OBJECT_INTERFACE)
            debugPrintf(5, _T("findMacAddress: nodeId=%d ifId=%d ifName=%s ifIndex=%d"), node->getId(), cp->getId(), cp->getName(), ((Interface *)cp)->getIfIndex());
         else
            debugPrintf(5, _T("findMacAddress: nodeId=%d apId=%d apName=%s"), node->getId(), cp->getId(), cp->getName());
      }
      else
      {
		   msg.setField(VID_RCC, RCC_INTERNAL_ERROR);
      }
	}

	sendMessage(&msg);
}

/**
 * Find connection port for given IP address
 */
void ClientSession::findIpAddress(NXCPMessage *request)
{
   NXCPMessage msg;
	TCHAR ipAddrText[16];

	msg.setId(request->getId());
	msg.setCode(CMD_REQUEST_COMPLETED);
	msg.setField(VID_RCC, RCC_SUCCESS);

	BYTE macAddr[6];
	bool found = false;

	UINT32 zoneId = request->getFieldAsUInt32(VID_ZONE_ID);
	UINT32 ipAddr = request->getFieldAsUInt32(VID_IP_ADDRESS);
	Interface *iface = FindInterfaceByIP(zoneId, ipAddr);
	if ((iface != NULL) && memcmp(iface->getMacAddr(), "\x00\x00\x00\x00\x00\x00", MAC_ADDR_LENGTH))
	{
		memcpy(macAddr, iface->getMacAddr(), MAC_ADDR_LENGTH);
		found = true;
		debugPrintf(5, _T("findIpAddress(%s): endpoint iface=%s"), IpToStr(ipAddr, ipAddrText), iface->getName());
	}
	else
	{
		// no interface object with this IP or MAC address not known, try to find it in ARP caches
		debugPrintf(5, _T("findIpAddress(%s): interface not found, looking in ARP cache"), IpToStr(ipAddr, ipAddrText));
		Subnet *subnet = FindSubnetForNode(zoneId, ipAddr);
		if (subnet != NULL)
		{
			debugPrintf(5, _T("findIpAddress(%s): found subnet %s"), ipAddrText, subnet->getName());
			found = subnet->findMacAddress(ipAddr, macAddr);
		}
		else
		{
			debugPrintf(5, _T("findIpAddress(%s): subnet not found"), ipAddrText);
		}
	}

	// Find switch port
	if (found)
	{
		int type;
		NetObj *cp = FindInterfaceConnectionPoint(macAddr, &type);

		debugPrintf(5, _T("findIpAddress: cp=%p type=%d"), cp, type);
		if (cp != NULL)
		{
			UINT32 localNodeId, localIfId;

			Interface *localIf = FindInterfaceByMAC(macAddr);
			if (localIf != NULL)
			{
				localIfId = localIf->getId();
				localNodeId = localIf->getParentNode()->getId();
			}
			else
			{
				localIfId = 0;
				localNodeId = 0;
			}

         Node *node = (cp->getObjectClass() == OBJECT_INTERFACE) ? ((Interface *)cp)->getParentNode() : ((AccessPoint *)cp)->getParentNode();
         if (node != NULL)
         {
		      msg.setField(VID_OBJECT_ID, node->getId());
			   msg.setField(VID_INTERFACE_ID, cp->getId());
            msg.setField(VID_IF_INDEX, (cp->getObjectClass() == OBJECT_INTERFACE) ? ((Interface *)cp)->getIfIndex() : (UINT32)0);
			   msg.setField(VID_LOCAL_NODE_ID, localNodeId);
			   msg.setField(VID_LOCAL_INTERFACE_ID, localIfId);
			   msg.setField(VID_MAC_ADDR, macAddr, MAC_ADDR_LENGTH);
			   msg.setField(VID_IP_ADDRESS, ipAddr);
			   msg.setField(VID_CONNECTION_TYPE, (UINT16)type);
            if (cp->getObjectClass() == OBJECT_INTERFACE)
               debugPrintf(5, _T("findIpAddress(%s): nodeId=%d ifId=%d ifName=%s ifIndex=%d"), IpToStr(ipAddr, ipAddrText), node->getId(), cp->getId(), cp->getName(), ((Interface *)cp)->getIfIndex());
            else
               debugPrintf(5, _T("findIpAddress(%s): nodeId=%d apId=%d apName=%s"), IpToStr(ipAddr, ipAddrText), node->getId(), cp->getId(), cp->getName());
         }
		}
	}

	sendMessage(&msg);
}

/**
 * Send image from library to client
 */
void ClientSession::sendLibraryImage(NXCPMessage *request)
{
	NXCPMessage msg;
	TCHAR guidText[64], absFileName[MAX_PATH];
	UINT32 rcc = RCC_SUCCESS;

	msg.setId(request->getId());
	msg.setCode(CMD_REQUEST_COMPLETED);

	uuid_t guid;
	request->getFieldAsBinary(VID_GUID, guid, UUID_LENGTH);
	_uuid_to_string(guid, guidText);
	debugPrintf(5, _T("sendLibraryImage: guid=%s"), guidText);

	if (rcc == RCC_SUCCESS)
	{
		TCHAR query[MAX_DB_STRING];
		_sntprintf(query, MAX_DB_STRING, _T("SELECT name,category,mimetype,protected FROM images WHERE guid = '%s'"), guidText);
		DB_RESULT result = DBSelect(g_hCoreDB, query);
		if (result != NULL)
		{
			int count = DBGetNumRows(result);
			if (count > 0)
			{
				TCHAR buffer[MAX_DB_STRING];

				msg.setField(VID_GUID, guid, UUID_LENGTH);

				DBGetField(result, 0, 0, buffer, MAX_DB_STRING);	// image name
				msg.setField(VID_NAME, buffer);
				DBGetField(result, 0, 1, buffer, MAX_DB_STRING);	// category
				msg.setField(VID_CATEGORY, buffer);
				DBGetField(result, 0, 2, buffer, MAX_DB_STRING);	// mime type
				msg.setField(VID_IMAGE_MIMETYPE, buffer);

				msg.setField(VID_IMAGE_PROTECTED, (WORD)DBGetFieldLong(result, 0, 3));

				_sntprintf(absFileName, MAX_PATH, _T("%s%s%s%s"), g_netxmsdDataDir, DDIR_IMAGES, FS_PATH_SEPARATOR, guidText);
				DbgPrintf(5, _T("sendLibraryImage: guid=%s, absFileName=%s"), guidText, absFileName);

#ifdef _WIN32
				struct _stat st;
#else
				struct stat st;
#endif
				if (_tstat(absFileName, &st) == 0 && S_ISREG(st.st_mode))
				{
					rcc = RCC_SUCCESS;
				}
				else
				{
					rcc = RCC_IO_ERROR;
				}
			}
			else
			{
				rcc = RCC_INVALID_OBJECT_ID;
			}

			DBFreeResult(result);
		}
		else
		{
			rcc = RCC_DB_FAILURE;
		}
	}

	msg.setField(VID_RCC, rcc);
	sendMessage(&msg);

	if (rcc == RCC_SUCCESS)
		sendFile(absFileName, request->getId(), 0);
}

void ClientSession::onLibraryImageChange(uuid_t *guid, bool removed)
{
  UPDATE_INFO *pUpdate;

  if (guid != NULL && isAuthenticated())
  {
    pUpdate = (UPDATE_INFO *)malloc(sizeof(UPDATE_INFO));
    pUpdate->dwCategory = INFO_CAT_LIBRARY_IMAGE;
    LIBRARY_IMAGE_UPDATE_INFO *info = (LIBRARY_IMAGE_UPDATE_INFO *)malloc(sizeof(LIBRARY_IMAGE_UPDATE_INFO));
    info->guid = (uuid_t *)(nx_memdup(guid, UUID_LENGTH));
    info->removed = removed;
    pUpdate->pData = info;
    m_pUpdateQueue->put(pUpdate);
  }
}

/**
 * Update library image from client
 */
void ClientSession::updateLibraryImage(NXCPMessage *request)
{
	NXCPMessage msg;
	UINT32 rcc = RCC_SUCCESS;

	msg.setId(request->getId());
	msg.setCode(CMD_REQUEST_COMPLETED);

   if (!checkSysAccessRights(SYSTEM_ACCESS_MANAGE_IMAGE_LIB))
   {
	   msg.setField(VID_RCC, RCC_ACCESS_DENIED);
      sendMessage(&msg);
      return;
   }

	uuid_t guid;
	_uuid_clear(guid);

	msg.setId(request->getId());
	msg.setCode(CMD_REQUEST_COMPLETED);

   TCHAR name[MAX_OBJECT_NAME] = _T("");
	TCHAR category[MAX_OBJECT_NAME] = _T("");
	TCHAR mimetype[MAX_DB_STRING] = _T("");
	TCHAR absFileName[MAX_PATH] = _T("");

	if (request->isFieldExist(VID_GUID))
	{
		request->getFieldAsBinary(VID_GUID, guid, UUID_LENGTH);
	}

	if (_uuid_is_null(guid))
	{
		_uuid_generate(guid);
	}

	TCHAR guidText[64];
	_uuid_to_string(guid, guidText);

	request->getFieldAsString(VID_NAME, name, MAX_OBJECT_NAME);
	request->getFieldAsString(VID_CATEGORY, category, MAX_OBJECT_NAME);
	request->getFieldAsString(VID_IMAGE_MIMETYPE, mimetype, MAX_DB_STRING);

	//UINT32 imageSize = request->getFieldAsBinary(VID_IMAGE_DATA, NULL, 0);
	//BYTE *imageData = (BYTE *)malloc(imageSize);
	//request->getFieldAsBinary(VID_IMAGE_DATA, imageData, imageSize);

	// Set default values for empty fields
	if (name[0] == 0)
		_tcscpy(name, guidText);
	if (category[0] == 0)
		_tcscpy(category, _T("Default"));
	if (mimetype[0] == 0)
		_tcscpy(mimetype, _T("image/png"));

	//debugPrintf(5, _T("updateLibraryImage: guid=%s, name=%s, category=%s, imageSize=%d"), guidText, name, category, (int)imageSize);
	debugPrintf(5, _T("updateLibraryImage: guid=%s, name=%s, category=%s"), guidText, name, category);

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

	if (rcc == RCC_SUCCESS)
	{
		TCHAR query[MAX_DB_STRING];
		_sntprintf(query, MAX_DB_STRING, _T("SELECT protected FROM images WHERE guid = '%s'"), guidText);
		DB_RESULT result = DBSelect(hdb, query);
		if (result != NULL)
		{
			int count = DBGetNumRows(result);
			TCHAR query[MAX_DB_STRING] = {0};
			if (count > 0)
			{
				BOOL isProtected = DBGetFieldLong(result, 0, 0) != 0; // protected

				if (!isProtected)
				{
					_sntprintf(query, MAX_DB_STRING, _T("UPDATE images SET name = %s, category = %s, mimetype = %s WHERE guid = '%s'"),
							(const TCHAR *)DBPrepareString(g_hCoreDB, name),
							(const TCHAR *)DBPrepareString(g_hCoreDB, category),
							(const TCHAR *)DBPrepareString(g_hCoreDB, mimetype, 32),
							guidText);
				}
				else
				{
					rcc = RCC_INVALID_REQUEST;
				}
			}
			else
			{
				// not found in DB, create
				_sntprintf(query, MAX_DB_STRING, _T("INSERT INTO images (guid, name, category, mimetype, protected) VALUES ('%s', %s, %s, %s, 0)"),
						guidText,
						(const TCHAR *)DBPrepareString(g_hCoreDB, name),
						(const TCHAR *)DBPrepareString(g_hCoreDB, category),
						(const TCHAR *)DBPrepareString(g_hCoreDB, mimetype, 32));
			}

			if (query[0] != 0)
			{
				if (DBQuery(hdb, query))
				{
					// DB up to date, update file)
					_sntprintf(absFileName, MAX_PATH, _T("%s%s%s%s"), g_netxmsdDataDir, DDIR_IMAGES, FS_PATH_SEPARATOR, guidText);
					DbgPrintf(5, _T("updateLibraryImage: guid=%s, absFileName=%s"), guidText, absFileName);

					if (m_hCurrFile == -1)
					{
						m_hCurrFile = _topen(absFileName, O_CREAT | O_TRUNC | O_WRONLY | O_BINARY, S_IRUSR | S_IWUSR);
						if (m_hCurrFile != -1)
						{
							m_dwFileRqId = request->getId();
							m_dwUploadCommand = CMD_MODIFY_IMAGE;
                     memcpy(m_uploadImageGuid, guid, UUID_LENGTH);
						}
						else
						{
							rcc = RCC_IO_ERROR;
						}
					}
					else
					{
						rcc = RCC_RESOURCE_BUSY;
					}
				}
				else
				{
					rcc = RCC_DB_FAILURE;
				}
			}

			DBFreeResult(result);
		}
		else
		{
			rcc = RCC_DB_FAILURE;
		}
	}

	if (rcc == RCC_SUCCESS)
	{
		msg.setField(VID_GUID, guid, UUID_LENGTH);
	}

   DBConnectionPoolReleaseConnection(hdb);
	msg.setField(VID_RCC, rcc);
	sendMessage(&msg);

	if (rcc == RCC_SUCCESS)
	{
		//EnumerateClientSessions(ImageLibraryUpdateCallback, (void *)&guid);
	}
}

/**
 * Delete image from library
 */
void ClientSession::deleteLibraryImage(NXCPMessage *request)
{
	NXCPMessage msg;
	UINT32 rcc = RCC_SUCCESS;
	uuid_t guid;
	TCHAR guidText[64];
	TCHAR query[MAX_DB_STRING];

	msg.setId(request->getId());
	msg.setCode(CMD_REQUEST_COMPLETED);

   if (!checkSysAccessRights(SYSTEM_ACCESS_MANAGE_IMAGE_LIB))
   {
	   msg.setField(VID_RCC, RCC_ACCESS_DENIED);
      sendMessage(&msg);
      return;
   }

	request->getFieldAsBinary(VID_GUID, guid, UUID_LENGTH);
	_uuid_to_string(guid, guidText);
	debugPrintf(5, _T("deleteLibraryImage: guid=%s"), guidText);

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

	_sntprintf(query, MAX_DB_STRING, _T("SELECT protected FROM images WHERE guid = '%s'"), guidText);
	DB_RESULT hResult = DBSelect(hdb, query);
	if (hResult != NULL)
	{
		if (DBGetNumRows(hResult) > 0)
		{
			if (DBGetFieldLong(hResult, 0, 0) == 0)
			{
				_sntprintf(query, MAX_DB_STRING, _T("DELETE FROM images WHERE protected = 0 AND guid = '%s'"), guidText);
				if (DBQuery(hdb, query))
				{
					// TODO: remove from FS?
				}
				else
				{
					rcc = RCC_DB_FAILURE;
				}
			}
			else
			{
				rcc = RCC_ACCESS_DENIED;
			}
		}
		else
		{
			rcc = RCC_INVALID_OBJECT_ID;
		}
		DBFreeResult(hResult);
	}
	else
	{
		rcc = RCC_DB_FAILURE;
	}

   DBConnectionPoolReleaseConnection(hdb);

	msg.setField(VID_RCC, rcc);
	sendMessage(&msg);

	if (rcc == RCC_SUCCESS)
	{
		EnumerateClientSessions(ImageLibraryDeleteCallback, (void *)&guid);
	}
}

/**
 * Send list of available images (in category)
 */
void ClientSession::listLibraryImages(NXCPMessage *request)
{
	NXCPMessage msg;
	TCHAR category[MAX_DB_STRING];
	TCHAR query[MAX_DB_STRING * 2];
	TCHAR buffer[MAX_DB_STRING];
	UINT32 rcc = RCC_SUCCESS;

	msg.setId(request->getId());
	msg.setCode(CMD_REQUEST_COMPLETED);

	if (request->isFieldExist(VID_CATEGORY))
	{
		request->getFieldAsString(VID_CATEGORY, category, MAX_DB_STRING);
	}
	else
	{
		category[0] = 0;
	}
	debugPrintf(5, _T("listLibraryImages: category=%s"), category[0] == 0 ? _T("*ANY*") : category);

	_tcscpy(query, _T("SELECT guid,name,category,mimetype,protected FROM images"));
	if (category[0] != 0)
	{
		_tcscat(query, _T(" WHERE category = "));
      _tcscat(query, (const TCHAR *)DBPrepareString(g_hCoreDB, category));
	}

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
	DB_RESULT result = DBSelect(hdb, query);
	if (result != NULL)
	{
		int count = DBGetNumRows(result);
		msg.setField(VID_NUM_RECORDS, (UINT32)count);
		UINT32 varId = VID_IMAGE_LIST_BASE;
		for (int i = 0; i < count; i++)
		{
			uuid guid = DBGetFieldGUID(result, i, 0);	// guid
			msg.setField(varId++, guid);

			DBGetField(result, i, 1, buffer, MAX_DB_STRING);	// image name
			msg.setField(varId++, buffer);

			DBGetField(result, i, 2, buffer, MAX_DB_STRING);	// category
			msg.setField(varId++, buffer);

			DBGetField(result, i, 3, buffer, MAX_DB_STRING);	// mime type
			msg.setField(varId++, buffer);

			msg.setField(varId++, (WORD)DBGetFieldLong(result, i, 4)); // protected flag
		}

		DBFreeResult(result);
	}
	else
	{
		rcc = RCC_DB_FAILURE;
	}

   DBConnectionPoolReleaseConnection(hdb);
	msg.setField(VID_RCC, rcc);
	sendMessage(&msg);
}

/**
 * Execute server side command on object
 */
void ClientSession::executeServerCommand(NXCPMessage *request)
{
	NXCPMessage msg;

	msg.setId(request->getId());
	msg.setCode(CMD_REQUEST_COMPLETED);

	UINT32 nodeId = request->getFieldAsUInt32(VID_OBJECT_ID);
	NetObj *object = FindObjectById(nodeId);
	if (object != NULL)
	{
		if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_CONTROL))
		{
			if (object->getObjectClass() == OBJECT_NODE)
			{
				TCHAR *cmd = request->getFieldAsString(VID_COMMAND);
				TCHAR *expCmd = ((Node *)object)->expandText(cmd);
				free(cmd);
				WriteAuditLog(AUDIT_OBJECTS, TRUE, m_dwUserId, m_workstation, m_id, nodeId, _T("Server command executed: %s"), expCmd);
            ThreadPoolExecute(g_mainThreadPool, ExecuteServerCommand, 
               new ServerCommandExecData(expCmd, request->getFieldAsBoolean(VID_RECEIVE_OUTPUT), request->getId(), this));
				msg.setField(VID_RCC, RCC_SUCCESS);
			}
			else
			{
				msg.setField(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
			}
		}
		else
		{
			msg.setField(VID_RCC, RCC_ACCESS_DENIED);
			WriteAuditLog(AUDIT_OBJECTS, FALSE, m_dwUserId, m_workstation, m_id, nodeId, _T("Access denied on server command execution"));
		}
	}
	else
	{
		msg.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
	}

	sendMessage(&msg);
}

/**
 * Upload file from server to agent
 */
void ClientSession::uploadFileToAgent(NXCPMessage *request)
{
	NXCPMessage msg;

	msg.setId(request->getId());
	msg.setCode(CMD_REQUEST_COMPLETED);

	UINT32 nodeId = request->getFieldAsUInt32(VID_OBJECT_ID);
	NetObj *object = FindObjectById(nodeId);
	if (object != NULL)
	{
		if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_CONTROL))
		{
			if (object->getObjectClass() == OBJECT_NODE)
			{
				TCHAR *localFile = request->getFieldAsString(VID_FILE_NAME);
				TCHAR *remoteFile = request->getFieldAsString(VID_DESTINATION_FILE_NAME);
				if (localFile != NULL)
				{
					int nLen;
					TCHAR fullPath[MAX_PATH];

					// Create full path to the file store
					_tcscpy(fullPath, g_netxmsdDataDir);
					_tcscat(fullPath, DDIR_FILES);
					_tcscat(fullPath, FS_PATH_SEPARATOR);
					nLen = (int)_tcslen(fullPath);
					nx_strncpy(&fullPath[nLen], GetCleanFileName(localFile), MAX_PATH - nLen);

					ServerJob *job = new FileUploadJob((Node *)object, fullPath, remoteFile, m_dwUserId,
					                                   request->getFieldAsUInt16(VID_CREATE_JOB_ON_HOLD) ? true : false);
					if (AddJob(job))
					{
						WriteAuditLog(AUDIT_OBJECTS, TRUE, m_dwUserId, m_workstation, m_id, nodeId,
										  _T("File upload to agent initiated, local='%s' remote='%s'"), CHECK_NULL(localFile), CHECK_NULL(remoteFile));
						msg.setField(VID_JOB_ID, job->getId());
						msg.setField(VID_RCC, RCC_SUCCESS);
					}
					else
					{
						msg.setField(VID_RCC, RCC_INTERNAL_ERROR);
						delete job;
					}
				}
				else
				{
					msg.setField(VID_RCC, RCC_INVALID_ARGUMENT);
				}
				safe_free(localFile);
				safe_free(remoteFile);
			}
			else
			{
				msg.setField(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
			}
		}
		else
		{
			msg.setField(VID_RCC, RCC_ACCESS_DENIED);
			WriteAuditLog(AUDIT_OBJECTS, FALSE, m_dwUserId, m_workstation, m_id, nodeId, _T("Access denied on file upload"));
		}
	}
	else
	{
		msg.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
	}

	sendMessage(&msg);
}

/**
 * Send to client list of files in server's file store
 */
void ClientSession::listServerFileStore(NXCPMessage *request)
{
	NXCPMessage msg;
	TCHAR path[MAX_PATH];
	StringList extensionList;

	msg.setId(request->getId());
	msg.setCode(CMD_REQUEST_COMPLETED);

	int length = (int)request->getFieldAsUInt32(VID_EXTENSION_COUNT);
	DbgPrintf(8, _T("ClientSession::listServerFileStore: length of filter type array is %d."), length);

   UINT32 varId = VID_EXTENSION_LIST_BASE;

   bool musicFiles = (length > 0);
	for(int i = 0; i < length; i++)
   {
      extensionList.add(request->getFieldAsString(varId++));
      for(int j = 0; j < m_musicTypeList.size(); j++)
      {
         if(_tcscmp(extensionList.get(i), m_musicTypeList.get(j)))
         {
            musicFiles = false;
         }
      }
   }

	if (m_dwSystemAccess & SYSTEM_ACCESS_READ_FILES || musicFiles)
	{
      _tcscpy(path, g_netxmsdDataDir);
      _tcscat(path, DDIR_FILES);
      _TDIR *dir = _topendir(path);
      if (dir != NULL)
      {
         _tcscat(path, FS_PATH_SEPARATOR);
         int pos = (int)_tcslen(path);

         struct _tdirent *d;
   #ifdef _WIN32
         struct _stat st;
   #else
         struct stat st;
   #endif
         UINT32 count = 0, varId = VID_INSTANCE_LIST_BASE;
         while((d = _treaddir(dir)) != NULL)
         {
            if (_tcscmp(d->d_name, _T(".")) && _tcscmp(d->d_name, _T("..")))
            {
               if(length != 0)
               {
                  bool correctType = false;
                  TCHAR *extension = _tcsrchr(d->d_name, _T('.'));
                  if (extension != NULL)
                  {
                     extension++;
                     for(int j = 0; j < extensionList.size(); j++)
                     {
                        if (!_tcscmp(extension, extensionList.get(j)))
                        {
                           correctType = true;
                           break;
                        }
                     }
                  }
                  if (!correctType)
                  {
                     continue;
                  }
               }
               nx_strncpy(&path[pos], d->d_name, MAX_PATH - pos);
               if (_tstat(path, &st) == 0)
               {
                  if (S_ISREG(st.st_mode))
                  {
                     msg.setField(varId++, d->d_name);
                     msg.setField(varId++, (QWORD)st.st_size);
                     msg.setField(varId++, (QWORD)st.st_mtime);
                     varId += 7;
                     count++;
                  }
               }
            }
         }
         _tclosedir(dir);
         msg.setField(VID_INSTANCE_COUNT, count);
         msg.setField(VID_RCC, RCC_SUCCESS);
      }
      else
      {
         msg.setField(VID_RCC, RCC_IO_ERROR);
      }
   }
	else
	{
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);
	}
	extensionList.clear();

	sendMessage(&msg);
}

/**
 * Open server console
 */
void ClientSession::openConsole(UINT32 rqId)
{
	NXCPMessage msg;

	msg.setId(rqId);
	msg.setCode(CMD_REQUEST_COMPLETED);

	if (m_dwSystemAccess & SYSTEM_ACCESS_SERVER_CONSOLE)
	{
		m_dwFlags |= CSF_CONSOLE_OPEN;
		m_console = (CONSOLE_CTX)malloc(sizeof(struct __console_ctx));
		m_console->hSocket = -1;
		m_console->socketMutex = INVALID_MUTEX_HANDLE;
		m_console->pMsg = new NXCPMessage;
		m_console->pMsg->setCode(CMD_ADM_MESSAGE);
		m_console->session = this;
      m_console->output = NULL;
		msg.setField(VID_RCC, RCC_SUCCESS);
	}
	else
	{
		msg.setField(VID_RCC, RCC_ACCESS_DENIED);
	}

	sendMessage(&msg);
}

/**
 * Close server console
 */
void ClientSession::closeConsole(UINT32 rqId)
{
	NXCPMessage msg;

	msg.setId(rqId);
	msg.setCode(CMD_REQUEST_COMPLETED);

	if (m_dwSystemAccess & SYSTEM_ACCESS_SERVER_CONSOLE)
	{
		if (m_dwFlags & CSF_CONSOLE_OPEN)
		{
			m_dwFlags &= ~CSF_CONSOLE_OPEN;
			delete m_console->pMsg;
			safe_free_and_null(m_console);
			msg.setField(VID_RCC, RCC_SUCCESS);
		}
		else
		{
			msg.setField(VID_RCC, RCC_OUT_OF_STATE_REQUEST);
		}
	}
	else
	{
		msg.setField(VID_RCC, RCC_ACCESS_DENIED);
	}

	sendMessage(&msg);
}


//
// Process console command
//

void ClientSession::processConsoleCommand(NXCPMessage *request)
{
	NXCPMessage msg;

	msg.setCode(CMD_REQUEST_COMPLETED);
	msg.setId(request->getId());

	if ((m_dwSystemAccess & SYSTEM_ACCESS_SERVER_CONSOLE) && (m_dwFlags & CSF_CONSOLE_OPEN))
	{
		TCHAR command[256];
		request->getFieldAsString(VID_COMMAND, command, 256);
		int rc = ProcessConsoleCommand(command, m_console);
      switch(rc)
      {
         case CMD_EXIT_SHUTDOWN:
            InitiateShutdown();
            break;
         case CMD_EXIT_CLOSE_SESSION:
				msg.setEndOfSequence();
				break;
         default:
            break;
      }
		msg.setField(VID_RCC, RCC_SUCCESS);
	}
	else
	{
		msg.setField(VID_RCC, RCC_ACCESS_DENIED);
	}

	sendMessage(&msg);
}


//
// Get VLANs configured on device
//

void ClientSession::getVlans(NXCPMessage *request)
{
	NXCPMessage msg;

	msg.setCode(CMD_REQUEST_COMPLETED);
	msg.setId(request->getId());

	NetObj *object = FindObjectById(request->getFieldAsUInt32(VID_OBJECT_ID));
	if (object != NULL)
	{
		if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
		{
			if (object->getObjectClass() == OBJECT_NODE)
			{
				VlanList *vlans = ((Node *)object)->getVlans();
				if (vlans != NULL)
				{
					vlans->fillMessage(&msg);
					vlans->decRefCount();
					msg.setField(VID_RCC, RCC_SUCCESS);
				}
				else
				{
					msg.setField(VID_RCC, RCC_RESOURCE_NOT_AVAILABLE);
				}
			}
			else
			{
				msg.setField(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
			}
		}
		else
		{
			msg.setField(VID_RCC, RCC_ACCESS_DENIED);
		}
	}
	else
	{
		msg.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
	}

	sendMessage(&msg);
}

/**
 * Receive file from client
 */
void ClientSession::receiveFile(NXCPMessage *request)
{
   NXCPMessage msg;

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(request->getId());

	if (m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_FILES)
   {
		TCHAR fileName[MAX_PATH];

      request->getFieldAsString(VID_FILE_NAME, fileName, MAX_PATH);
      const TCHAR *cleanFileName = GetCleanFileName(fileName);

      // Prepare for file receive
      if (m_hCurrFile == -1)
      {
         _tcscpy(m_szCurrFileName, g_netxmsdDataDir);
			_tcscat(m_szCurrFileName, DDIR_FILES);
         _tcscat(m_szCurrFileName, FS_PATH_SEPARATOR);
         _tcscat(m_szCurrFileName, cleanFileName);
         m_hCurrFile = _topen(m_szCurrFileName, O_CREAT | O_TRUNC | O_WRONLY | O_BINARY, S_IRUSR | S_IWUSR);
         if (m_hCurrFile != -1)
         {
            m_dwFileRqId = request->getId();
            m_dwUploadCommand = CMD_UPLOAD_FILE;
            msg.setField(VID_RCC, RCC_SUCCESS);
            WriteAuditLog(AUDIT_SYSCFG, TRUE, m_dwUserId, m_workstation, m_id, 0,
               _T("Started upload of file \"%s\" to server"), fileName);
            NotifyClientSessions(NX_NOTIFY_FILE_LIST_CHANGED, 0);
         }
         else
         {
            msg.setField(VID_RCC, RCC_IO_ERROR);
         }
      }
      else
      {
         msg.setField(VID_RCC, RCC_RESOURCE_BUSY);
      }
   }
   else
   {
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);
   }

   // Send response
   sendMessage(&msg);
}


/**
 * Delete file in store
 */

void ClientSession::deleteFile(NXCPMessage *request)
{
   NXCPMessage msg;

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(request->getId());

	if (m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_FILES)
   {
		TCHAR fileName[MAX_PATH];

      request->getFieldAsString(VID_FILE_NAME, fileName, MAX_PATH);
      const TCHAR *cleanFileName = GetCleanFileName(fileName);

      _tcscpy(m_szCurrFileName, g_netxmsdDataDir);
      _tcscat(m_szCurrFileName, DDIR_FILES);
      _tcscat(m_szCurrFileName, FS_PATH_SEPARATOR);
      _tcscat(m_szCurrFileName, cleanFileName);

      if (_tunlink(m_szCurrFileName) == 0)
      {
         NotifyClientSessions(NX_NOTIFY_FILE_LIST_CHANGED, 0);
         msg.setField(VID_RCC, RCC_SUCCESS);
      }
      else
      {
         msg.setField(VID_RCC, RCC_IO_ERROR);
      }
   }
   else
   {
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Get network path between two nodes
 */
void ClientSession::getNetworkPath(NXCPMessage *request)
{
	NXCPMessage msg;

	msg.setCode(CMD_REQUEST_COMPLETED);
	msg.setId(request->getId());

	NetObj *node1 = FindObjectById(request->getFieldAsUInt32(VID_SOURCE_OBJECT_ID));
	NetObj *node2 = FindObjectById(request->getFieldAsUInt32(VID_DESTINATION_OBJECT_ID));

	if ((node1 != NULL) && (node2 != NULL))
	{
		if (node1->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ) &&
		    node2->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
		{
			if ((node1->getObjectClass() == OBJECT_NODE) && (node2->getObjectClass() == OBJECT_NODE))
			{
				NetworkPath *path = TraceRoute((Node *)node1, (Node *)node2);
				if (path != NULL)
				{
					msg.setField(VID_RCC, RCC_SUCCESS);
					path->fillMessage(&msg);
					delete path;
				}
				else
				{
					msg.setField(VID_RCC, RCC_INTERNAL_ERROR);
				}
			}
			else
			{
				msg.setField(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
			}
		}
		else
		{
			msg.setField(VID_RCC, RCC_ACCESS_DENIED);
		}
	}
	else
	{
		msg.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
	}

	sendMessage(&msg);
}

/**
 * Get physical components of the node
 */
void ClientSession::getNodeComponents(NXCPMessage *request)
{
   NXCPMessage msg;

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(request->getId());

   // Get node id and check object class and access rights
   Node *node = (Node *)FindObjectById(request->getFieldAsUInt32(VID_OBJECT_ID), OBJECT_NODE);
   if (node != NULL)
   {
      if (node->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
      {
			ComponentTree *components = node->getComponents();
			if (components != NULL)
			{
				msg.setField(VID_RCC, RCC_SUCCESS);
				components->fillMessage(&msg, VID_COMPONENT_LIST_BASE);
				components->decRefCount();
			}
			else
			{
				msg.setField(VID_RCC, RCC_NO_COMPONENT_DATA);
			}
      }
      else
      {
         msg.setField(VID_RCC, RCC_ACCESS_DENIED);
      }
   }
   else  // No object with given ID
   {
      msg.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Get list of software packages installed on node
 */
void ClientSession::getNodeSoftware(NXCPMessage *request)
{
   NXCPMessage msg;

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(request->getId());

   // Get node id and check object class and access rights
   Node *node = (Node *)FindObjectById(request->getFieldAsUInt32(VID_OBJECT_ID), OBJECT_NODE);
   if (node != NULL)
   {
      if (node->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
      {
			node->writePackageListToMessage(&msg);
      }
      else
      {
         msg.setField(VID_RCC, RCC_ACCESS_DENIED);
      }
   }
   else  // No object with given ID
   {
      msg.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Get list of Windows performance objects supported by node
 */
void ClientSession::getWinPerfObjects(NXCPMessage *request)
{
   NXCPMessage msg;

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(request->getId());

   // Get node id and check object class and access rights
   Node *node = (Node *)FindObjectById(request->getFieldAsUInt32(VID_OBJECT_ID), OBJECT_NODE);
   if (node != NULL)
   {
      if (node->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
      {
			node->writeWinPerfObjectsToMessage(&msg);
      }
      else
      {
         msg.setField(VID_RCC, RCC_ACCESS_DENIED);
      }
   }
   else  // No object with given ID
   {
      msg.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Get threshold summary for underlying data collection targets
 */
void ClientSession::getThresholdSummary(NXCPMessage *request)
{
   NXCPMessage msg;

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(request->getId());

   // Get object id and check object class and access rights
   NetObj *object = FindObjectById(request->getFieldAsUInt32(VID_OBJECT_ID));
   if (object != NULL)
   {
      if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
      {
			if (object->showThresholdSummary())
			{
				ObjectArray<DataCollectionTarget> *targets = new ObjectArray<DataCollectionTarget>();
				object->addChildDCTargetsToList(targets, m_dwUserId);
				UINT32 varId = VID_THRESHOLD_BASE;
				for(int i = 0; i < targets->size(); i++)
				{
					if (targets->get(i)->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
						varId = targets->get(i)->getThresholdSummary(&msg, varId);
					targets->get(i)->decRefCount();
				}
				delete targets;
			}
			else
			{
	         msg.setField(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
			}
      }
      else
      {
         msg.setField(VID_RCC, RCC_ACCESS_DENIED);
      }
   }
   else  // No object with given ID
   {
      msg.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * List configured mapping tables
 */
void ClientSession::listMappingTables(NXCPMessage *request)
{
   NXCPMessage msg;

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(request->getId());

	if (m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_MAPPING_TBLS)
	{
		msg.setField(VID_RCC, ListMappingTables(&msg));
	}
	else
	{
		msg.setField(VID_RCC, RCC_ACCESS_DENIED);
	}

   // Send response
   sendMessage(&msg);
}

/**
 * Get content of specific mapping table
 */
void ClientSession::getMappingTable(NXCPMessage *request)
{
   NXCPMessage msg;

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(request->getId());
	msg.setField(VID_RCC, GetMappingTable((LONG)request->getFieldAsUInt32(VID_MAPPING_TABLE_ID), &msg));

   // Send response
   sendMessage(&msg);
}

/**
 * Create or update mapping table
 */
void ClientSession::updateMappingTable(NXCPMessage *request)
{
   NXCPMessage msg;

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(request->getId());

	if (m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_MAPPING_TBLS)
	{
		LONG id;
		msg.setField(VID_RCC, UpdateMappingTable(request, &id));
		msg.setField(VID_MAPPING_TABLE_ID, (UINT32)id);
	}
	else
	{
		msg.setField(VID_RCC, RCC_ACCESS_DENIED);
	}

   // Send response
   sendMessage(&msg);
}

/**
 * Delete mapping table
 */
void ClientSession::deleteMappingTable(NXCPMessage *request)
{
   NXCPMessage msg;

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(request->getId());

	if (m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_MAPPING_TBLS)
	{
		msg.setField(VID_RCC, DeleteMappingTable((LONG)request->getFieldAsUInt32(VID_MAPPING_TABLE_ID)));
	}
	else
	{
		msg.setField(VID_RCC, RCC_ACCESS_DENIED);
	}

   // Send response
   sendMessage(&msg);
}

/**
 * Get list of wireless stations registered on controller
 */
void ClientSession::getWirelessStations(NXCPMessage *request)
{
   NXCPMessage msg;

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(request->getId());

   // Get object id and check object class and access rights
	Node *node = (Node *)FindObjectById(request->getFieldAsUInt32(VID_OBJECT_ID), OBJECT_NODE);
   if (node != NULL)
   {
      if (node->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
      {
			if (node->isWirelessController())
			{
				node->writeWsListToMessage(&msg);
	         msg.setField(VID_RCC, RCC_SUCCESS);
			}
			else
			{
	         msg.setField(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
			}
      }
      else
      {
         msg.setField(VID_RCC, RCC_ACCESS_DENIED);
      }
   }
   else  // No object with given ID
   {
      msg.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Get list of configured DCI summary tables
 */
void ClientSession::getSummaryTables(UINT32 rqId)
{
   NXCPMessage msg;

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(rqId);

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_RESULT hResult = DBSelect(hdb, _T("SELECT id,menu_path,title,flags,guid FROM dci_summary_tables"));
   if (hResult != NULL)
   {
      TCHAR buffer[256];
      int count = DBGetNumRows(hResult);
      msg.setField(VID_NUM_ELEMENTS, (UINT32)count);
      UINT32 varId = VID_ELEMENT_LIST_BASE;
      for(int i = 0; i < count; i++)
      {
         msg.setField(varId++, (UINT32)DBGetFieldLong(hResult, i, 0));
         msg.setField(varId++, DBGetField(hResult, i, 1, buffer, 256));
         msg.setField(varId++, DBGetField(hResult, i, 2, buffer, 256));
         msg.setField(varId++, (UINT32)DBGetFieldLong(hResult, i, 3));

         uuid guid = DBGetFieldGUID(hResult, i, 4);
         msg.setField(varId++, guid);

         varId += 5;
      }
      DBFreeResult(hResult);
   }
	else
	{
		msg.setField(VID_RCC, RCC_ACCESS_DENIED);
	}
   DBConnectionPoolReleaseConnection(hdb);

   // Send response
   sendMessage(&msg);
}

/**
 * Get details of DCI summary table
 */
void ClientSession::getSummaryTableDetails(NXCPMessage *request)
{
   NXCPMessage msg;

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(request->getId());

	if (m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_SUMMARY_TBLS)
	{
      LONG id = (LONG)request->getFieldAsUInt32(VID_SUMMARY_TABLE_ID);
      DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
      DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT menu_path,title,node_filter,flags,columns,guid FROM dci_summary_tables WHERE id=?"));
      if (hStmt != NULL)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, id);
         DB_RESULT hResult = DBSelectPrepared(hStmt);
         if (hResult != NULL)
         {
            if (DBGetNumRows(hResult) > 0)
            {
               TCHAR buffer[256];
               msg.setField(VID_SUMMARY_TABLE_ID, (UINT32)id);
               msg.setField(VID_MENU_PATH, DBGetField(hResult, 0, 0, buffer, 256));
               msg.setField(VID_TITLE, DBGetField(hResult, 0, 1, buffer, 256));
               TCHAR *tmp = DBGetField(hResult, 0, 2, NULL, 0);
               if (tmp != NULL)
               {
                  msg.setField(VID_FILTER, tmp);
                  free(tmp);
               }
               msg.setField(VID_FLAGS, DBGetFieldULong(hResult, 0, 3));
               tmp = DBGetField(hResult, 0, 4, NULL, 0);
               if (tmp != NULL)
               {
                  msg.setField(VID_COLUMNS, tmp);
                  free(tmp);
               }
               uuid guid = DBGetFieldGUID(hResult, 0, 5);
               msg.setField(VID_GUID, guid);
            }
            else
            {
               msg.setField(VID_RCC, RCC_INVALID_SUMMARY_TABLE_ID);
            }
            DBFreeResult(hResult);
         }
         else
         {
            msg.setField(VID_RCC, RCC_DB_FAILURE);
         }
         DBFreeStatement(hStmt);
      }
      else
      {
         msg.setField(VID_RCC, RCC_DB_FAILURE);
      }
      DBConnectionPoolReleaseConnection(hdb);
	}
	else
	{
		msg.setField(VID_RCC, RCC_ACCESS_DENIED);
	}

   // Send response
   sendMessage(&msg);
}

/**
 * Modify DCI summary table
 */
void ClientSession::modifySummaryTable(NXCPMessage *request)
{
   NXCPMessage msg;

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(request->getId());

	if (m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_SUMMARY_TBLS)
	{
		LONG id;
		msg.setField(VID_RCC, ModifySummaryTable(request, &id));
		msg.setField(VID_SUMMARY_TABLE_ID, (UINT32)id);
	}
	else
	{
		msg.setField(VID_RCC, RCC_ACCESS_DENIED);
	}

   // Send response
   sendMessage(&msg);
}

/**
 * Delete DCI summary table
 */
void ClientSession::deleteSummaryTable(NXCPMessage *request)
{
   NXCPMessage msg;

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(request->getId());

	if (m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_SUMMARY_TBLS)
	{
		msg.setField(VID_RCC, DeleteSummaryTable((LONG)request->getFieldAsUInt32(VID_SUMMARY_TABLE_ID)));
	}
	else
	{
		msg.setField(VID_RCC, RCC_ACCESS_DENIED);
	}

   // Send response
   sendMessage(&msg);
}

/**
 * Query DCI summary table
 */
void ClientSession::querySummaryTable(NXCPMessage *request)
{
   NXCPMessage msg;

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(request->getId());

   UINT32 rcc;
   Table *result = QuerySummaryTable((LONG)request->getFieldAsUInt32(VID_SUMMARY_TABLE_ID), NULL,
                                      request->getFieldAsUInt32(VID_OBJECT_ID),
                                      m_dwUserId, &rcc);
   if (result != NULL)
   {
      debugPrintf(6, _T("querySummaryTable: %d rows in resulting table"), result->getNumRows());
      msg.setField(VID_RCC, RCC_SUCCESS);
      result->fillMessage(msg, 0, -1);
      delete result;
   }
   else
   {
      msg.setField(VID_RCC, rcc);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Query ad hoc DCI summary table
 */
void ClientSession::queryAdHocSummaryTable(NXCPMessage *request)
{
   NXCPMessage msg;

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(request->getId());

   SummaryTable *tableDefinition = new SummaryTable(request);

   UINT32 rcc;
   Table *result = QuerySummaryTable(0, tableDefinition,
                                      request->getFieldAsUInt32(VID_OBJECT_ID),
                                      m_dwUserId, &rcc);
   if (result != NULL)
   {
      debugPrintf(6, _T("querySummaryTable: %d rows in resulting table"), result->getNumRows());
      msg.setField(VID_RCC, RCC_SUCCESS);
      result->fillMessage(msg, 0, -1);
      delete result;
   }
   else
   {
      msg.setField(VID_RCC, rcc);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Forward event to Reporting Server
 */
void ClientSession::forwardToReportingServer(NXCPMessage *request)
{
   NXCPMessage *msg = NULL;

   if (checkSysAccessRights(SYSTEM_ACCESS_REPORTING_SERVER))
   {
      TCHAR buffer[256];
	   debugPrintf(7, _T("RS: Forwarding message %s"), NXCPMessageCodeName(request->getCode(), buffer));

	   request->setField(VID_USER_NAME, m_loginName);
	   msg = ForwardMessageToReportingServer(request, this);
	   if (msg == NULL)
	   {
		  msg = new NXCPMessage();
		  msg->setCode(CMD_REQUEST_COMPLETED);
		  msg->setId(request->getId());
		  msg->setField(VID_RCC, RCC_COMM_FAILURE);
	   }
   }
   else
   {
	   WriteAuditLog(AUDIT_SECURITY, FALSE, m_dwUserId, m_workstation, m_id, 0, _T("Reporting server access denied"));
	   msg = new NXCPMessage();
	   msg->setCode(CMD_REQUEST_COMPLETED);
	   msg->setId(request->getId());
	   msg->setField(VID_RCC, RCC_ACCESS_DENIED);
   }

   sendMessage(msg);
   delete msg;
}

/**
 * Get address map for a subnet
 */
void ClientSession::getSubnetAddressMap(NXCPMessage *request)
{
   NXCPMessage msg;

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(request->getId());

   // Get node id and check object class and access rights
   Subnet *subnet = (Subnet *)FindObjectById(request->getFieldAsUInt32(VID_OBJECT_ID), OBJECT_SUBNET);
   if (subnet != NULL)
   {
      if (subnet->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
      {
         int length;
         UINT32 *map = subnet->buildAddressMap(&length);
			if (map != NULL)
			{
				msg.setField(VID_RCC, RCC_SUCCESS);
            msg.setFieldFromInt32Array(VID_ADDRESS_MAP, (UINT32)length, map);
            free(map);
			}
			else
			{
				msg.setField(VID_RCC, RCC_INTERNAL_ERROR);
			}
      }
      else
      {
         msg.setField(VID_RCC, RCC_ACCESS_DENIED);
      }
   }
   else  // No object with given ID
   {
      msg.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Get effective rights for object
 */
void ClientSession::getEffectiveRights(NXCPMessage *request)
{
   NXCPMessage msg;

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(request->getId());

   // Get node id and check object class and access rights
   NetObj *object = FindObjectById(request->getFieldAsUInt32(VID_OBJECT_ID));
   if (object != NULL)
   {
      msg.setField(VID_EFFECTIVE_RIGHTS, object->getUserRights(m_dwUserId));
		msg.setField(VID_RCC, RCC_SUCCESS);
   }
   else  // No object with given ID
   {
      msg.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * File manager control calls
 */
void ClientSession::fileManagerControl(NXCPMessage *request)
{
   NXCPMessage msg, *response = NULL, *responseMessage;
	UINT32 rcc = RCC_INTERNAL_ERROR;
   responseMessage = &msg;

   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(request->getId());

   TCHAR fileName[MAX_PATH];
   request->getFieldAsString(VID_FILE_NAME, fileName, MAX_PATH);
   UINT32 objectId = request->getFieldAsUInt32(VID_OBJECT_ID);
	NetObj *object = FindObjectById(objectId);
	if (object != NULL)
	{
		if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_MANAGE_FILES) ||
         (request->getCode() == CMD_GET_FOLDER_CONTENT && object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ)))
		{
			if (object->getObjectClass() == OBJECT_NODE)
			{
            Node *node = (Node *)object;
            node->incRefCount();
            AgentConnection *conn = node->createAgentConnection();
            if(conn != NULL)
            {
               request->setId(conn->generateRequestId());
               response = conn->customRequest(request);
               if (response != NULL)
               {
                  rcc = response->getFieldAsUInt32(VID_RCC);
                  if (rcc == ERR_SUCCESS)
                  {
                     response->setId(msg.getId());
                     response->setCode(CMD_REQUEST_COMPLETED);
                     responseMessage = response;

                     //Add line in audit log
                     switch(request->getCode())
                     {
                        case CMD_GET_FOLDER_CONTENT:
                           WriteAuditLog(AUDIT_SYSCFG, TRUE, m_dwUserId, m_workstation, m_id, objectId,
                              _T("Get content of agents folder \"%s\""), fileName);
                           break;
                        case CMD_FILEMGR_DELETE_FILE:
                           WriteAuditLog(AUDIT_SYSCFG, TRUE, m_dwUserId, m_workstation, m_id, objectId,
                              _T("Delete agents file/folder \"%s\""), fileName);
                           break;
                        case CMD_FILEMGR_RENAME_FILE:
                        {
                           TCHAR newFileName[MAX_PATH];
                           request->getFieldAsString(VID_NEW_FILE_NAME, newFileName, MAX_PATH);
                           WriteAuditLog(AUDIT_SYSCFG, TRUE, m_dwUserId, m_workstation, m_id, objectId,
                              _T("Rename agents file/folder \"%s\" to \"%s\""), fileName, newFileName);
                           break;
                        }
                        case CMD_FILEMGR_MOVE_FILE:
                        {
                           TCHAR newFileName[MAX_PATH];
                           request->getFieldAsString(VID_NEW_FILE_NAME, newFileName, MAX_PATH);
                           WriteAuditLog(AUDIT_SYSCFG, TRUE, m_dwUserId, m_workstation, m_id, objectId,
                              _T("Move agents file/folder from \"%s\" to \"%s\""), fileName, newFileName);
                           break;
                        }
                        case CMD_FILEMGR_CREATE_FOLDER:
                        {
                           TCHAR newFileName[MAX_PATH];
                           request->getFieldAsString(VID_NEW_FILE_NAME, newFileName, MAX_PATH);
                           WriteAuditLog(AUDIT_SYSCFG, TRUE, m_dwUserId, m_workstation, m_id, objectId,
                              _T("Create folder \"%s\""), fileName);
                           break;
                        }
                     }
                  }
                  else
                  {
                     debugPrintf(6, _T("ClientSession::getAgentFolderContent: Error on agent: %d (%s)"), rcc, AgentErrorCodeToText(rcc));
                     rcc = AgentErrorToRCC(rcc);
                     msg.setField(VID_RCC, rcc);
                  }
               }
               else
               {
                  msg.setField(VID_RCC, RCC_TIMEOUT);
               }
               delete conn;
            }
            else
            {
               msg.setField(VID_RCC, RCC_CONNECTION_BROKEN);
            }
            node->decRefCount();
			}
			else
			{
				msg.setField(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
			}
		}
		else
		{
			msg.setField(VID_RCC, RCC_ACCESS_DENIED);
		}
	}
	else
	{
		msg.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
	}

	if (rcc == RCC_ACCESS_DENIED)
	{
      switch(request->getCode())
      {
         case CMD_GET_FOLDER_CONTENT:
            WriteAuditLog(AUDIT_SYSCFG, FALSE, m_dwUserId, m_workstation, m_id, objectId,
               _T("Acess denied to get content of agents folder \"%s\""), fileName);
               break;
         case CMD_FILEMGR_DELETE_FILE:
            WriteAuditLog(AUDIT_SYSCFG, FALSE, m_dwUserId, m_workstation, m_id, objectId,
               _T("Acess denied to delete agents file/folder \"%s\""), fileName);
               break;
         case CMD_FILEMGR_RENAME_FILE:
         {
            TCHAR newFileName[MAX_PATH];
            request->getFieldAsString(VID_NEW_FILE_NAME, newFileName, MAX_PATH);
            WriteAuditLog(AUDIT_SYSCFG, FALSE, m_dwUserId, m_workstation, m_id, objectId,
               _T("Acess denied to rename agents file/folder \"%s\" to \"%s\""), fileName, newFileName);
            break;
         }
         case CMD_FILEMGR_MOVE_FILE:
         {
            TCHAR newFileName[MAX_PATH];
            request->getFieldAsString(VID_NEW_FILE_NAME, newFileName, MAX_PATH);
            WriteAuditLog(AUDIT_SYSCFG, FALSE, m_dwUserId, m_workstation, m_id, objectId,
               _T("Acess denied to move agents file/folder from \"%s\" to \"%s\""), fileName, newFileName);
            break;
         }
         case CMD_FILEMGR_CREATE_FOLDER:
         {
            WriteAuditLog(AUDIT_SYSCFG, FALSE, m_dwUserId, m_workstation, m_id, objectId,
               _T("Acess denied to create folder \"%s\""), fileName);
            break;
         }
      }
	}

   sendMessage(responseMessage);
   if(response != NULL)
      delete response;
}

/**
 * Upload file provided by user directly to agent
 */
void ClientSession::uploadUserFileToAgent(NXCPMessage *request)
{
   NXCPMessage msg, *response = NULL, *responseMessage;
	UINT32 rcc = RCC_INTERNAL_ERROR;
   responseMessage = &msg;

   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(request->getId());

   TCHAR fileName[MAX_PATH];
   request->getFieldAsString(VID_FILE_NAME, fileName, MAX_PATH);
   UINT32 objectId = request->getFieldAsUInt32(VID_OBJECT_ID);
	NetObj *object = FindObjectById(objectId);
	if (object != NULL)
	{
		if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_UPLOAD))
		{
			if (object->getObjectClass() == OBJECT_NODE)
			{
            Node *node = (Node *)object;
            node->incRefCount();
            AgentConnection *conn = node->createAgentConnection();
            if(conn != NULL)
            {
               conn->sendMessage(request);
               response = conn->waitForMessage(CMD_REQUEST_COMPLETED, request->getId(), 10000);
               if (response != NULL)
               {
                  rcc = response->getFieldAsUInt32(VID_RCC);
                  if(rcc == RCC_SUCCESS)
                  {
                     response->setCode(CMD_REQUEST_COMPLETED);
                     responseMessage = response;

                     //Add line in audit log
                     WriteAuditLog(AUDIT_SYSCFG, TRUE, m_dwUserId, m_workstation, m_id, objectId,
                        _T("Started direct upload of file \"%s\" to agent"), fileName);
                     //Set all required for file download
                     m_agentConn.put((QWORD)request->getId(),(NetObj *)conn);
                  }
                  else
                  {
                     msg.setField(VID_RCC, rcc);
                     debugPrintf(6, _T("ClientSession::getAgentFolderContent: Error on agent: %d"), rcc);
                  }
               }
               else
               {
                  msg.setField(VID_RCC, RCC_TIMEOUT);
               }
               if(rcc != RCC_SUCCESS)
               {
                  delete conn;
               }
            }
            else
            {
               msg.setField(VID_RCC, RCC_CONNECTION_BROKEN);
            }
            node->decRefCount();
			}
			else
			{
				msg.setField(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
			}
		}
		else
		{
			msg.setField(VID_RCC, RCC_ACCESS_DENIED);
		}
	}
	else
	{
		msg.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
	}

	if(rcc == RCC_ACCESS_DENIED)
      WriteAuditLog(AUDIT_SYSCFG, FALSE, m_dwUserId, m_workstation, m_id, objectId,
         _T("Access denied for direct upload of file \"%s\" to agent"), fileName);

   sendMessage(responseMessage);
   if(response != NULL)
      delete response;
}

/**
 * Get switch forwarding database
 */
void ClientSession::getSwitchForwardingDatabase(NXCPMessage *request)
{
   NXCPMessage msg;

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(request->getId());

   // Get node id and check object class and access rights
   Node *node = (Node *)FindObjectById(request->getFieldAsUInt32(VID_OBJECT_ID), OBJECT_NODE);
   if (node != NULL)
   {
      if (node->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
      {
         ForwardingDatabase *fdb = node->getSwitchForwardingDatabase();
         if (fdb != NULL)
         {
            msg.setField(VID_RCC, RCC_SUCCESS);
            fdb->fillMessage(&msg);
   			fdb->decRefCount();
         }
         else
         {
            msg.setField(VID_RCC, RCC_NO_FDB);
         }
      }
      else
      {
         msg.setField(VID_RCC, RCC_ACCESS_DENIED);
         WriteAuditLog(AUDIT_OBJECTS, FALSE, m_dwUserId, m_workstation, m_id, node->getId(), _T("Access denied on reading FDB"));
      }
   }
   else  // No object with given ID
   {
      msg.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Get switch forwarding database
 */
void ClientSession::getRoutingTable(NXCPMessage *request)
{
   NXCPMessage msg;

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(request->getId());

   // Get node id and check object class and access rights
   Node *node = (Node *)FindObjectById(request->getFieldAsUInt32(VID_OBJECT_ID), OBJECT_NODE);
   if (node != NULL)
   {
      if (node->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
      {
         ROUTING_TABLE *rt = node->getRoutingTable();
         if (rt != NULL)
         {
            msg.setField(VID_RCC, RCC_SUCCESS);
            msg.setField(VID_NUM_ELEMENTS, (UINT32)rt->iNumEntries);
            UINT32 id = VID_ELEMENT_LIST_BASE;
            for(int i = 0; i < rt->iNumEntries; i++)
            {
               msg.setField(id++, rt->pRoutes[i].dwDestAddr);
               msg.setField(id++, (UINT32)BitsInMask(rt->pRoutes[i].dwDestMask));
               msg.setField(id++, rt->pRoutes[i].dwNextHop);
               msg.setField(id++, rt->pRoutes[i].dwIfIndex);
               msg.setField(id++, rt->pRoutes[i].dwRouteType);
               Interface *iface = node->findInterfaceByIndex(rt->pRoutes[i].dwIfIndex);
               if (iface != NULL)
               {
                  msg.setField(id++, iface->getName());
               }
               else
               {
                  TCHAR buffer[32];
                  _sntprintf(buffer, 32, _T("[%d]"), rt->pRoutes[i].dwIfIndex);
                  msg.setField(id++, buffer);
               }
               id += 4;
            }
   			DestroyRoutingTable(rt);
         }
         else
         {
            msg.setField(VID_RCC, RCC_NO_ROUTING_TABLE);
         }
      }
      else
      {
         msg.setField(VID_RCC, RCC_ACCESS_DENIED);
         WriteAuditLog(AUDIT_OBJECTS, FALSE, m_dwUserId, m_workstation, m_id, node->getId(), _T("Access denied on reading routing table"));
      }
   }
   else  // No object with given ID
   {
      msg.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Get location history for object
 */
void ClientSession::getLocationHistory(NXCPMessage *request)
{
   NXCPMessage msg;

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(request->getId());

   // Get node id and check object class and access rights
   Node *node = (Node *)FindObjectById(request->getFieldAsUInt32(VID_OBJECT_ID), OBJECT_MOBILEDEVICE);
   if (node != NULL)
   {
      if (node->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
      {
         DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
         TCHAR query[256];
         _sntprintf(query, 255, _T("SELECT latitude,longitude,accuracy,start_timestamp,end_timestamp FROM gps_history_%d")
                                             _T(" WHERE start_timestamp<? AND end_timestamp>?"), request->getFieldAsUInt32(VID_OBJECT_ID));

         DB_STATEMENT hStmt = DBPrepare(hdb, query);
         if (hStmt != NULL)
         {
            DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, (INT64)request->getFieldAsTime(VID_TIME_TO));
            DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, (INT64)request->getFieldAsTime(VID_TIME_FROM));
            DB_RESULT hResult = DBSelectPrepared(hStmt);
            if (hResult != NULL)
            {
               int base = VID_LOC_LIST_BASE;
               TCHAR buffer[32];
               msg.setField(VID_NUM_RECORDS, (UINT32)DBGetNumRows(hResult));
               for(int i = 0; i < DBGetNumRows(hResult); i++, base+=10)
               {
                  msg.setField(base, DBGetField(hResult, i, 0, buffer, 32));
                  msg.setField(base+1, DBGetField(hResult, i, 1, buffer, 32));
                  msg.setField(base+2, DBGetFieldULong(hResult, i, 2));
                  msg.setField(base+3, DBGetFieldULong(hResult, i, 3));
                  msg.setField(base+4, DBGetFieldULong(hResult, i, 4));
               }
               DBFreeResult(hResult);
            }
            else
            {
               msg.setField(VID_RCC, RCC_DB_FAILURE);
            }
            DBFreeStatement(hStmt);
         }
         else
         {
            msg.setField(VID_RCC, RCC_DB_FAILURE);
         }
         DBConnectionPoolReleaseConnection(hdb);
      }
      else
      {
         msg.setField(VID_RCC, RCC_ACCESS_DENIED);
         WriteAuditLog(AUDIT_OBJECTS, FALSE, m_dwUserId, m_workstation, m_id, node->getId(), _T("Access denied on reading routing table"));
      }
   }
   else  // No object with given ID
   {
      msg.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Get location history for object
 */
void ClientSession::getScreenshot(NXCPMessage *request)
{
   NXCPMessage msg;

   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(request->getId());

   TCHAR* sessionName = request->getFieldAsString(VID_NAME);
   if(sessionName == NULL)
      sessionName = _tcsdup(_T("Console"));
   UINT32 objectId = request->getFieldAsUInt32(VID_NODE_ID);
	NetObj *object = FindObjectById(objectId);

	if (object != NULL)
	{
		if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
		{
			if (object->getObjectClass() == OBJECT_NODE)
			{
            Node *node = (Node *)object;
            node->incRefCount();
            AgentConnection *conn = node->createAgentConnection();
            if(conn != NULL)
            {
               BYTE *data = NULL;
               size_t size;
               UINT32 dwError = conn->takeScreenshot(sessionName, &data, &size);
               if (dwError == ERR_SUCCESS)
               {
                  msg.setField(VID_FILE_DATA, data, size);
               }
               else
               {
                  msg.setField(VID_RCC, RCC_INTERNAL_ERROR);
               }
               safe_free(data);
               delete conn;
            }
            else
            {
               msg.setField(VID_RCC, RCC_CONNECTION_BROKEN);
            }
            node->decRefCount();
			}
			else
			{
				msg.setField(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
			}
		}
		else
		{
			msg.setField(VID_RCC, RCC_ACCESS_DENIED);
		}
	}
	else
	{
		msg.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
	}
   safe_free(sessionName);
   // Send response
   sendMessage(&msg);
}
