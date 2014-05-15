/*
** NetXMS - Network Management System
** Copyright (C) 2003-2014 Raden Solutions
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
extern Queue g_statusPollQueue;
extern Queue g_configPollQueue;
extern Queue g_routePollQueue;
extern Queue g_discoveryPollQueue;
extern Queue g_nodePollerQueue;
extern Queue g_conditionPollerQueue;
extern Queue *g_pItemQueue;

void UnregisterClientSession(UINT32 dwIndex);
void ResetDiscoveryPoller();
CSCPMessage *ForwardMessageToReportingServer(CSCPMessage *request, ClientSession *session);
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
	CSCPMessage *pMsg;
} PROCTHREAD_START_DATA;

/**
 * Object tool acl entry
 */
typedef struct
{
   UINT32 dwToolId;
   UINT32 dwUserId;
} OBJECT_TOOL_ACL;

/**
 *
 */
typedef struct
{
  uuid_t *guid;
  bool removed;
} LIBRARY_IMAGE_UPDATE_INFO;

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
	ThreadCreate(ThreadStarter_##func, 0, pData); \
}

#define DEFINE_THREAD_STARTER(func) \
THREAD_RESULT THREAD_CALL ClientSession::ThreadStarter_##func(void *pArg) \
{ \
   ((PROCTHREAD_START_DATA *)pArg)->pSession->func(((PROCTHREAD_START_DATA *)pArg)->pMsg); \
	((PROCTHREAD_START_DATA *)pArg)->pSession->m_dwRefCount--; \
	delete ((PROCTHREAD_START_DATA *)pArg)->pMsg; \
	free(pArg); \
   return THREAD_OK; \
}

DEFINE_THREAD_STARTER(getCollectedData)
DEFINE_THREAD_STARTER(getTableCollectedData)
DEFINE_THREAD_STARTER(clearDCIData)
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

/**
 * Client communication read thread starter
 */
THREAD_RESULT THREAD_CALL ClientSession::readThreadStarter(void *pArg)
{
   ((ClientSession *)pArg)->readThread();

   // When ClientSession::ReadThread exits, all other session
   // threads are already stopped, so we can safely destroy
   // session object
   UnregisterClientSession(((ClientSession *)pArg)->getIndex());
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
 * Forced node poll thread starter
 */
THREAD_RESULT THREAD_CALL ClientSession::pollerThreadStarter(void *pArg)
{
   ((POLLER_START_DATA *)pArg)->pSession->pollerThread(
      ((POLLER_START_DATA *)pArg)->pNode,
      ((POLLER_START_DATA *)pArg)->iPollType,
      ((POLLER_START_DATA *)pArg)->dwRqId);
   ((POLLER_START_DATA *)pArg)->pSession->decRefCount();
   free(pArg);
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
   m_dwIndex = INVALID_INDEX;
   m_iState = SESSION_STATE_INIT;
   m_pMsgBuffer = (CSCP_BUFFER *)malloc(sizeof(CSCP_BUFFER));
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
   _tcscpy(m_szUserName, _T("<not logged in>"));
	_tcscpy(m_szClientInfo, _T("n/a"));
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
   safe_free(m_pMsgBuffer);
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
		TCHAR buffer[4096];

      va_start(args, format);
      _vsntprintf(buffer, 4096, format, args);
      va_end(args);
		DbgPrintf(level, _T("[CLSN-%d] %s"), m_dwIndex, buffer);
   }
}

/**
 * Read thread
 */
void ClientSession::readThread()
{
	UINT32 msgBufferSize = 1024;
   CSCP_MESSAGE *pRawMsg;
   CSCPMessage *pMsg;
   BYTE *pDecryptionBuffer = NULL;
   TCHAR szBuffer[256];
   int iErr;
   UINT32 i;
   NetObj *object;
   WORD wFlags;

   // Initialize raw message receiving function
   RecvNXCPMessage(0, NULL, m_pMsgBuffer, 0, NULL, NULL, 0);

   pRawMsg = (CSCP_MESSAGE *)malloc(msgBufferSize);
#ifdef _WITH_ENCRYPTION
   pDecryptionBuffer = (BYTE *)malloc(msgBufferSize);
#endif
   while(1)
   {
		// Shrink buffer after receiving large message
		if (msgBufferSize > 131072)
		{
			msgBufferSize = 131072;
		   pRawMsg = (CSCP_MESSAGE *)realloc(pRawMsg, msgBufferSize);
			if (pDecryptionBuffer != NULL)
			   pDecryptionBuffer = (BYTE *)realloc(pDecryptionBuffer, msgBufferSize);
		}

      if ((iErr = RecvNXCPMessageEx(m_hSocket, &pRawMsg, m_pMsgBuffer, &msgBufferSize,
		                              &m_pCtx, (pDecryptionBuffer != NULL) ? &pDecryptionBuffer : NULL,
												900000, MAX_MSG_SIZE)) <= 0)  // timeout 15 minutes
		{
         debugPrintf(5, _T("RecvNXCPMessageEx failed (%d)"), iErr);
         break;
      }

      // Receive timeout
      if (iErr == 3)
      {
         debugPrintf(5, _T("RecvNXCPMessageEx: receive timeout"));
         break;
      }

      // Check if message is too large
      if (iErr == 1)
      {
         debugPrintf(4, _T("Received message %s is too large (%d bytes)"),
                     NXCPMessageCodeName(ntohs(pRawMsg->wCode), szBuffer),
                     ntohl(pRawMsg->dwSize));
         continue;
      }

      // Check for decryption error
      if (iErr == 2)
      {
         debugPrintf(4, _T("Unable to decrypt received message"));
         continue;
      }

      // Check that actual received packet size is equal to encoded in packet
      if ((int)ntohl(pRawMsg->dwSize) != iErr)
      {
         debugPrintf(4, _T("Actual message size doesn't match wSize value (%d,%d)"), iErr, ntohl(pRawMsg->dwSize));
         continue;   // Bad packet, wait for next
      }

      if (g_debugLevel >= 8)
      {
         String msgDump = CSCPMessage::dump(pRawMsg, NXCP_VERSION);
         debugPrintf(8, _T("Message dump:\n%s"), (const TCHAR *)msgDump);
      }

      // Special handling for raw messages
      wFlags = ntohs(pRawMsg->wFlags);
      if (wFlags & MF_BINARY)
      {
         // Convert message header to host format
         pRawMsg->dwId = ntohl(pRawMsg->dwId);
         pRawMsg->wCode = ntohs(pRawMsg->wCode);
         pRawMsg->dwNumVars = ntohl(pRawMsg->dwNumVars);
         debugPrintf(6, _T("Received raw message %s"), NXCPMessageCodeName(pRawMsg->wCode, szBuffer));

         if ((pRawMsg->wCode == CMD_FILE_DATA) ||
             (pRawMsg->wCode == CMD_ABORT_FILE_TRANSFER))
         {
            if ((m_hCurrFile != -1) && (m_dwFileRqId == pRawMsg->dwId))
            {
               if (pRawMsg->wCode == CMD_FILE_DATA)
               {
                  if (write(m_hCurrFile, pRawMsg->df, pRawMsg->dwNumVars) == (int)pRawMsg->dwNumVars)
                  {
                     if (wFlags & MF_END_OF_FILE)
                     {
								debugPrintf(6, _T("Got end of file marker"));
                        CSCPMessage msg;

                        close(m_hCurrFile);
                        m_hCurrFile = -1;

                        msg.SetCode(CMD_REQUEST_COMPLETED);
                        msg.SetId(pRawMsg->dwId);
                        msg.SetVariable(VID_RCC, RCC_SUCCESS);
                        sendMessage(&msg);

                        onFileUpload(TRUE);
                     }
                  }
                  else
                  {
							debugPrintf(6, _T("I/O error"));
                     // I/O error
                     CSCPMessage msg;

                     close(m_hCurrFile);
                     m_hCurrFile = -1;

                     msg.SetCode(CMD_REQUEST_COMPLETED);
                     msg.SetId(pRawMsg->dwId);
                     msg.SetVariable(VID_RCC, RCC_IO_ERROR);
                     sendMessage(&msg);

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
               debugPrintf(4, _T("Out of state message (ID: %d)"), pRawMsg->dwId);
            }
         }
      }
      else
      {
         // Create message object from raw message
         pMsg = new CSCPMessage(pRawMsg);
         if ((pMsg->GetCode() == CMD_SESSION_KEY) && (pMsg->GetId() == m_dwEncryptionRqId))
         {
		      debugPrintf(6, _T("Received message %s"), NXCPMessageCodeName(pMsg->GetCode(), szBuffer));
            m_dwEncryptionResult = SetupEncryptionContext(pMsg, &m_pCtx, NULL, g_pServerKey, NXCP_VERSION);
            ConditionSet(m_condEncryptionSetup);
            m_dwEncryptionRqId = 0;
            delete pMsg;
         }
         else if (pMsg->GetCode() == CMD_KEEPALIVE)
			{
		      debugPrintf(6, _T("Received message %s"), NXCPMessageCodeName(pMsg->GetCode(), szBuffer));
				respondToKeepalive(pMsg->GetId());
				delete pMsg;
			}
			else
         {
            m_pMessageQueue->Put(pMsg);
         }
      }
   }
   if (iErr < 0)
      nxlog_write(MSG_SESSION_CLOSED, EVENTLOG_WARNING_TYPE, "e", WSAGetLastError());
   free(pRawMsg);
#ifdef _WITH_ENCRYPTION
   free(pDecryptionBuffer);
#endif

	// Finish update thread first
   m_pUpdateQueue->Put(INVALID_POINTER_VALUE);
   ThreadJoin(m_hUpdateThread);

   // Notify other threads to exit
	while((pRawMsg = (CSCP_MESSAGE *)m_pSendQueue->Get()) != NULL)
		free(pRawMsg);
   m_pSendQueue->Put(INVALID_POINTER_VALUE);
	while((pMsg = (CSCPMessage *)m_pMessageQueue->Get()) != NULL)
		delete pMsg;
   m_pMessageQueue->Put(INVALID_POINTER_VALUE);

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
   RemoveAllSessionLocks(m_dwIndex);
   for(i = 0; i < m_dwOpenDCIListSize; i++)
   {
      object = FindObjectById(m_pOpenDCIList[i]);
      if (object != NULL)
         if ((object->Type() == OBJECT_NODE) ||
             (object->Type() == OBJECT_CLUSTER) ||
             (object->Type() == OBJECT_TEMPLATE))
            ((Template *)object)->unlockDCIList(m_dwIndex);
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

	WriteAuditLog(AUDIT_SECURITY, TRUE, m_dwUserId, m_workstation, 0, _T("User logged out (client: %s)"), m_szClientInfo);
   debugPrintf(3, _T("Session closed"));
}

/**
 * Network write thread
 */
void ClientSession::writeThread()
{
   CSCP_MESSAGE *pRawMsg;
   CSCP_ENCRYPTED_MESSAGE *pEnMsg;
   TCHAR szBuffer[128];
   BOOL bResult;

   while(1)
   {
      pRawMsg = (CSCP_MESSAGE *)m_pSendQueue->GetOrBlock();
      if (pRawMsg == INVALID_POINTER_VALUE)    // Session termination indicator
         break;

		if (ntohs(pRawMsg->wCode) != CMD_ADM_MESSAGE)
			debugPrintf(6, _T("Sending message %s"), NXCPMessageCodeName(ntohs(pRawMsg->wCode), szBuffer));

      if (m_pCtx != NULL)
      {
         pEnMsg = CSCPEncryptMessage(m_pCtx, pRawMsg);
         if (pEnMsg != NULL)
         {
            bResult = (SendEx(m_hSocket, (char *)pEnMsg, ntohl(pEnMsg->dwSize), 0, m_mutexSocketWrite) == (int)ntohl(pEnMsg->dwSize));
            free(pEnMsg);
         }
         else
         {
            bResult = FALSE;
         }
      }
      else
      {
         bResult = (SendEx(m_hSocket, (const char *)pRawMsg, ntohl(pRawMsg->dwSize), 0, m_mutexSocketWrite) == (int)ntohl(pRawMsg->dwSize));
      }
      free(pRawMsg);

      if (!bResult)
      {
         closesocket(m_hSocket);
         m_hSocket = -1;
         break;
      }
   }
}

/**
 * Update processing thread
 */
void ClientSession::updateThread()
{
   UPDATE_INFO *pUpdate;
   CSCPMessage msg;

   while(1)
   {
      pUpdate = (UPDATE_INFO *)m_pUpdateQueue->GetOrBlock();
      if (pUpdate == INVALID_POINTER_VALUE)    // Session termination indicator
         break;

      switch(pUpdate->dwCategory)
      {
         case INFO_CAT_EVENT:
            MutexLock(m_mutexSendEvents);
            sendMessage((CSCPMessage *)pUpdate->pData);
            MutexUnlock(m_mutexSendEvents);
            delete (CSCPMessage *)pUpdate->pData;
            break;
         case INFO_CAT_SYSLOG_MSG:
            MutexLock(m_mutexSendSyslog);
            msg.SetCode(CMD_SYSLOG_RECORDS);
            CreateMessageFromSyslogMsg(&msg, (NX_SYSLOG_RECORD *)pUpdate->pData);
            sendMessage(&msg);
            MutexUnlock(m_mutexSendSyslog);
            free(pUpdate->pData);
            break;
         case INFO_CAT_SNMP_TRAP:
            MutexLock(m_mutexSendTrapLog);
            sendMessage((CSCPMessage *)pUpdate->pData);
            MutexUnlock(m_mutexSendTrapLog);
            delete (CSCPMessage *)pUpdate->pData;
            break;
         case INFO_CAT_AUDIT_RECORD:
            MutexLock(m_mutexSendAuditLog);
            sendMessage((CSCPMessage *)pUpdate->pData);
            MutexUnlock(m_mutexSendAuditLog);
            delete (CSCPMessage *)pUpdate->pData;
            break;
         case INFO_CAT_OBJECT_CHANGE:
            MutexLock(m_mutexSendObjects);
            msg.SetCode(CMD_OBJECT_UPDATE);
            if (!((NetObj *)pUpdate->pData)->isDeleted())
            {
               ((NetObj *)pUpdate->pData)->CreateMessage(&msg);
               if (m_dwFlags & CSF_SYNC_OBJECT_COMMENTS)
                  ((NetObj *)pUpdate->pData)->commentsToMessage(&msg);
            }
            else
            {
               msg.SetVariable(VID_OBJECT_ID, ((NetObj *)pUpdate->pData)->Id());
               msg.SetVariable(VID_IS_DELETED, (WORD)1);
            }
            sendMessage(&msg);
            MutexUnlock(m_mutexSendObjects);
            msg.deleteAllVariables();
            ((NetObj *)pUpdate->pData)->decRefCount();
            break;
         case INFO_CAT_ALARM:
            MutexLock(m_mutexSendAlarms);
            msg.SetCode(CMD_ALARM_UPDATE);
            msg.SetVariable(VID_NOTIFICATION_CODE, pUpdate->dwCode);
            FillAlarmInfoMessage(&msg, (NXC_ALARM *)pUpdate->pData);
            sendMessage(&msg);
            MutexUnlock(m_mutexSendAlarms);
            msg.deleteAllVariables();
            free(pUpdate->pData);
            break;
         case INFO_CAT_ACTION:
            MutexLock(m_mutexSendActions);
            msg.SetCode(CMD_ACTION_DB_UPDATE);
            msg.SetVariable(VID_NOTIFICATION_CODE, pUpdate->dwCode);
            msg.SetVariable(VID_ACTION_ID, ((NXC_ACTION *)pUpdate->pData)->dwId);
            if (pUpdate->dwCode != NX_NOTIFY_ACTION_DELETED)
               FillActionInfoMessage(&msg, (NXC_ACTION *)pUpdate->pData);
            sendMessage(&msg);
            MutexUnlock(m_mutexSendActions);
            msg.deleteAllVariables();
            free(pUpdate->pData);
            break;
         case INFO_CAT_SITUATION:
            MutexLock(m_mutexSendSituations);
            sendMessage((CSCPMessage *)pUpdate->pData);
            MutexUnlock(m_mutexSendSituations);
            delete (CSCPMessage *)pUpdate->pData;
            break;
         case INFO_CAT_LIBRARY_IMAGE:
            {
              LIBRARY_IMAGE_UPDATE_INFO *info = (LIBRARY_IMAGE_UPDATE_INFO *)pUpdate->pData;
              msg.SetCode(CMD_IMAGE_LIBRARY_UPDATE);
              msg.SetVariable(VID_GUID, (BYTE *)info->guid, UUID_LENGTH);
              if (info->removed)
              {
                msg.SetVariable(VID_FLAGS, (UINT32)1);
              }
              else
              {
                msg.SetVariable(VID_FLAGS, (UINT32)0);
              }
              sendMessage(&msg);
              msg.deleteAllVariables();
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
   CSCPMessage *pMsg;
   TCHAR szBuffer[128];
   UINT32 i;
	int status;

   while(1)
   {
      pMsg = (CSCPMessage *)m_pMessageQueue->GetOrBlock();
      if (pMsg == INVALID_POINTER_VALUE)    // Session termination indicator
         break;

      m_wCurrentCmd = pMsg->GetCode();
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

      m_iState = SESSION_STATE_PROCESSING;
      switch(m_wCurrentCmd)
      {
         case CMD_LOGIN:
            login(pMsg);
            break;
         case CMD_GET_SERVER_INFO:
            sendServerInfo(pMsg->GetId());
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
            sendAllConfigVars(pMsg->GetId());
            break;
         case CMD_SET_CONFIG_VARIABLE:
            setConfigVariable(pMsg);
            break;
         case CMD_DELETE_CONFIG_VARIABLE:
            deleteConfigVariable(pMsg);
            break;
			case CMD_CONFIG_GET_CLOB:
				getConfigCLOB(pMsg);
				break;
			case CMD_CONFIG_SET_CLOB:
				setConfigCLOB(pMsg);
				break;
         case CMD_LOAD_EVENT_DB:
            sendEventDB(pMsg->GetId());
            break;
         case CMD_SET_EVENT_INFO:
            modifyEventTemplate(pMsg);
            break;
         case CMD_DELETE_EVENT_TEMPLATE:
            deleteEventTemplate(pMsg);
            break;
         case CMD_GENERATE_EVENT_CODE:
            generateEventCode(pMsg->GetId());
            break;
         case CMD_MODIFY_OBJECT:
            modifyObject(pMsg);
            break;
         case CMD_SET_OBJECT_MGMT_STATUS:
            changeObjectMgmtStatus(pMsg);
            break;
         case CMD_LOAD_USER_DB:
            sendUserDB(pMsg->GetId());
            break;
         case CMD_CREATE_USER:
            createUser(pMsg);
            break;
         case CMD_UPDATE_USER:
            updateUser(pMsg);
            break;
         case CMD_DELETE_USER:
            deleteUser(pMsg);
            break;
         case CMD_LOCK_USER_DB:
            lockUserDB(pMsg->GetId(), TRUE);
            break;
         case CMD_UNLOCK_USER_DB:
            lockUserDB(pMsg->GetId(), FALSE);
            break;
         case CMD_SET_PASSWORD:
            setPassword(pMsg);
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
         case CMD_OPEN_EPP:
            openEPP(pMsg);
            break;
         case CMD_CLOSE_EPP:
            closeEPP(pMsg->GetId());
            break;
         case CMD_SAVE_EPP:
            saveEPP(pMsg);
            break;
         case CMD_EPP_RECORD:
            processEPPRecord(pMsg);
            break;
         case CMD_GET_MIB_TIMESTAMP:
            sendMIBTimestamp(pMsg->GetId());
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
            sendAllAlarms(pMsg->GetId());
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
            sendAllActions(pMsg->GetId());
            break;
         case CMD_GET_CONTAINER_CAT_LIST:
            SendContainerCategories(pMsg->GetId());
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
            sendAllTraps(pMsg->GetId());
            break;
			case CMD_GET_TRAP_CFG_RO:
				sendAllTraps2(pMsg->GetId());
				break;
         case CMD_QUERY_PARAMETER:
            CALL_IN_NEW_THREAD(queryParameter, pMsg);
            break;
         case CMD_QUERY_TABLE:
            CALL_IN_NEW_THREAD(queryAgentTable, pMsg);
            break;
         case CMD_LOCK_PACKAGE_DB:
            LockPackageDB(pMsg->GetId(), TRUE);
            break;
         case CMD_UNLOCK_PACKAGE_DB:
            LockPackageDB(pMsg->GetId(), FALSE);
            break;
         case CMD_GET_PACKAGE_LIST:
            SendAllPackages(pMsg->GetId());
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
            sendObjectTools(pMsg->GetId());
            break;
         case CMD_EXEC_TABLE_TOOL:
            execTableTool(pMsg);
            break;
         case CMD_GET_OBJECT_TOOL_DETAILS:
            sendObjectToolDetails(pMsg);
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
            generateObjectToolId(pMsg->GetId());
            break;
         case CMD_CHANGE_SUBSCRIPTION:
            changeSubscription(pMsg);
            break;
         case CMD_GET_SYSLOG:
            CALL_IN_NEW_THREAD(sendSyslog, pMsg);
            break;
         case CMD_GET_SERVER_STATS:
            sendServerStats(pMsg->GetId());
            break;
         case CMD_GET_SCRIPT_LIST:
            sendScriptList(pMsg->GetId());
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
            SendSessionList(pMsg->GetId());
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
            sendDCIEventList(pMsg);
            break;
			case CMD_GET_PERFTAB_DCI_LIST:
				sendPerfTabDCIList(pMsg);
				break;
         case CMD_PUSH_DCI_DATA:
            pushDCIData(pMsg);
            break;
         case CMD_GET_AGENT_CFG_LIST:
            sendAgentCfgList(pMsg->GetId());
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
				sendGraphList(pMsg->GetId());
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
				getCertificateList(pMsg->GetId());
				break;
			case CMD_QUERY_L2_TOPOLOGY:
				CALL_IN_NEW_THREAD(queryL2Topology, pMsg);
				break;
			case CMD_SEND_SMS:
				sendSMS(pMsg);
				break;
			case CMD_GET_COMMUNITY_LIST:
				SendCommunityList(pMsg->GetId());
				break;
			case CMD_UPDATE_COMMUNITY_LIST:
				UpdateCommunityList(pMsg);
				break;
			case CMD_GET_USM_CREDENTIALS:
				sendUsmCredentials(pMsg->GetId());
				break;
			case CMD_UPDATE_USM_CREDENTIALS:
				updateUsmCredentials(pMsg);
				break;
			case CMD_GET_SITUATION_LIST:
				getSituationList(pMsg->GetId());
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
			case CMD_GET_JOB_LIST:
				sendJobList(pMsg->GetId());
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
				openConsole(pMsg->GetId());
				break;
			case CMD_CLOSE_CONSOLE:
				closeConsole(pMsg->GetId());
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
            getSummaryTables(pMsg->GetId());
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
         case CMD_GET_SUBNET_ADDRESS_MAP:
            getSubnetAddressMap(pMsg);
            break;
         case CMD_GET_EFFECTIVE_RIGHTS:
            getEffectiveRights(pMsg);
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
               CSCPMessage response;

               response.SetId(pMsg->GetId());
               response.SetCode(CMD_REQUEST_COMPLETED);
               response.SetVariable(VID_RCC, RCC_NOT_IMPLEMENTED);
               sendMessage(&response);
            }
            break;
      }
      delete pMsg;
      m_iState = (m_dwFlags & CSF_AUTHENTICATED) ? SESSION_STATE_IDLE : SESSION_STATE_INIT;
   }
}

/**
 * Respond to client's keepalive message
 */
void ClientSession::respondToKeepalive(UINT32 dwRqId)
{
   CSCPMessage msg;

   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_RCC, RCC_SUCCESS);
   sendMessage(&msg);
}

/**
 * Process received file
 */
static void SendLibraryImageUpdate(ClientSession *pSession, void *pArg);
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
      EnumerateClientSessions(SendLibraryImageUpdate, (void *)&m_uploadImageGuid);
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
void ClientSession::sendMessage(CSCPMessage *msg)
{
   TCHAR szBuffer[128];
   BOOL bResult;

	if (msg->GetCode() != CMD_ADM_MESSAGE)
		debugPrintf(6, _T("Sending message %s"), NXCPMessageCodeName(msg->GetCode(), szBuffer));

	CSCP_MESSAGE *pRawMsg = msg->createMessage();
   if ((g_debugLevel >= 8) && (msg->GetCode() != CMD_ADM_MESSAGE))
   {
      String msgDump = CSCPMessage::dump(pRawMsg, NXCP_VERSION);
      debugPrintf(8, _T("Message dump:\n%s"), (const TCHAR *)msgDump);
   }
   if (m_pCtx != NULL)
   {
      CSCP_ENCRYPTED_MESSAGE *pEnMsg = CSCPEncryptMessage(m_pCtx, pRawMsg);
      if (pEnMsg != NULL)
      {
         bResult = (SendEx(m_hSocket, (char *)pEnMsg, ntohl(pEnMsg->dwSize), 0, m_mutexSocketWrite) == (int)ntohl(pEnMsg->dwSize));
         free(pEnMsg);
      }
      else
      {
         bResult = FALSE;
      }
   }
   else
   {
      bResult = (SendEx(m_hSocket, (const char *)pRawMsg, ntohl(pRawMsg->dwSize), 0, m_mutexSocketWrite) == (int)ntohl(pRawMsg->dwSize));
   }
   free(pRawMsg);

   if (!bResult)
   {
      closesocket(m_hSocket);
      m_hSocket = -1;
   }
}

/**
 * Send raw message to client
 */
void ClientSession::sendRawMessage(CSCP_MESSAGE *msg)
{
   TCHAR szBuffer[128];
   BOOL bResult;

	debugPrintf(6, _T("Sending raw message %s"), NXCPMessageCodeName(ntohs(msg->wCode), szBuffer));
   if (m_pCtx != NULL)
   {
      CSCP_ENCRYPTED_MESSAGE *pEnMsg = CSCPEncryptMessage(m_pCtx, msg);
      if (pEnMsg != NULL)
      {
         bResult = (SendEx(m_hSocket, (char *)pEnMsg, ntohl(pEnMsg->dwSize), 0, m_mutexSocketWrite) == (int)ntohl(pEnMsg->dwSize));
         free(pEnMsg);
      }
      else
      {
         bResult = FALSE;
      }
   }
   else
   {
      bResult = (SendEx(m_hSocket, (const char *)msg, ntohl(msg->dwSize), 0, m_mutexSocketWrite) == (int)ntohl(msg->dwSize));
   }

   if (!bResult)
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
	return SendFileOverNXCP(m_hSocket, dwRqId, file, m_pCtx, ofset, NULL, NULL, m_mutexSocketWrite);
}

/**
 * Send server information to client
 */
void ClientSession::sendServerInfo(UINT32 dwRqId)
{
   CSCPMessage msg;
	TCHAR szBuffer[1024];
	String strURL;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(dwRqId);

	// Generate challenge for certificate authentication
#ifdef _WITH_ENCRYPTION
	RAND_bytes(m_challenge, CLIENT_CHALLENGE_SIZE);
#else
	memset(m_challenge, 0, CLIENT_CHALLENGE_SIZE);
#endif

   // Fill message with server info
   msg.SetVariable(VID_RCC, RCC_SUCCESS);
   msg.SetVariable(VID_SERVER_VERSION, NETXMS_VERSION_STRING);
   msg.SetVariable(VID_SERVER_ID, (BYTE *)&g_qwServerId, sizeof(QWORD));
   msg.SetVariable(VID_SUPPORTED_ENCRYPTION, (UINT32)0);
   msg.SetVariable(VID_PROTOCOL_VERSION, (UINT32)CLIENT_PROTOCOL_VERSION);
	msg.SetVariable(VID_CHALLENGE, m_challenge, CLIENT_CHALLENGE_SIZE);
   msg.SetVariable(VID_TIMESTAMP, (UINT32)time(NULL));

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
#endif
#if HAVE_LOCALTIME_R
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

	msg.SetVariable(VID_TIMEZONE, szBuffer);
	debugPrintf(2, _T("Server time zone: %s"), szBuffer);

	ConfigReadStr(_T("TileServerURL"), szBuffer, 1024, _T("http://tile.openstreetmap.org/"));
	msg.SetVariable(VID_TILE_SERVER_URL, szBuffer);

	ConfigReadStr(_T("DefaultConsoleDateFormat"), szBuffer, 1024, _T("dd.MM.yyyy"));
	msg.SetVariable(VID_DATE_FORMAT, szBuffer);

	ConfigReadStr(_T("DefaultConsoleTimeFormat"), szBuffer, 1024, _T("HH:mm:ss"));
	msg.SetVariable(VID_TIME_FORMAT, szBuffer);

	ConfigReadStr(_T("DefaultConsoleShortTimeFormat"), szBuffer, 1024, _T("HH:mm"));
	msg.SetVariable(VID_SHORT_TIME_FORMAT, szBuffer);

   // Send response
   sendMessage(&msg);
}

/**
 * Authenticate client
 */
void ClientSession::login(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   TCHAR szLogin[MAX_USER_NAME], szPassword[1024];
	int nAuthType;
   bool changePasswd = false, intruderLockout = false;
   UINT32 dwResult;
#ifdef _WITH_ENCRYPTION
	X509 *pCert;
#endif

   // Prepare response message
   msg.SetCode(CMD_LOGIN_RESP);
   msg.SetId(pRequest->GetId());

   // Get client info string
   if (pRequest->isFieldExist(VID_CLIENT_INFO))
   {
      TCHAR szClientInfo[32], szOSInfo[32], szLibVersion[16];

      pRequest->GetVariableStr(VID_CLIENT_INFO, szClientInfo, 32);
      pRequest->GetVariableStr(VID_OS_INFO, szOSInfo, 32);
      pRequest->GetVariableStr(VID_LIBNXCL_VERSION, szLibVersion, 16);
      _sntprintf(m_szClientInfo, 96, _T("%s (%s; libnxcl %s)"),
                 szClientInfo, szOSInfo, szLibVersion);
   }

	m_clientType = pRequest->GetVariableShort(VID_CLIENT_TYPE);
	if ((m_clientType < 0) || (m_clientType > CLIENT_TYPE_APPLICATION))
		m_clientType = CLIENT_TYPE_DESKTOP;

   if (m_clientType == CLIENT_TYPE_WEB)
   {
      _tcscpy(m_webServerAddress, m_workstation);
      if (pRequest->isFieldExist(VID_CLIENT_ADDRESS))
      {
         pRequest->GetVariableStr(VID_CLIENT_ADDRESS, m_workstation, 256);
         debugPrintf(5, _T("Real web client address is %s"), m_workstation);
      }
   }

   if (!(m_dwFlags & CSF_AUTHENTICATED))
   {
      pRequest->GetVariableStr(VID_LOGIN_NAME, szLogin, MAX_USER_NAME);
		nAuthType = (int)pRequest->GetVariableShort(VID_AUTH_TYPE);
      debugPrintf(6, _T("authentication type %d"), nAuthType);
		switch(nAuthType)
		{
			case NETXMS_AUTH_TYPE_PASSWORD:
#ifdef UNICODE
				pRequest->GetVariableStr(VID_PASSWORD, szPassword, 256);
#else
				pRequest->GetVariableStrUTF8(VID_PASSWORD, szPassword, 1024);
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

					dwSigLen = pRequest->GetVariableBinary(VID_SIGNATURE, signature, 256);
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
            pRequest->GetVariableStrA(VID_PASSWORD, ticket, 1024);
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
         _sntprintf(m_szUserName, MAX_SESSION_NAME, _T("%s@%s"), szLogin, m_workstation);
         m_loginTime = time(NULL);
         msg.SetVariable(VID_RCC, RCC_SUCCESS);
         msg.SetVariable(VID_USER_SYS_RIGHTS, m_dwSystemAccess);
         msg.SetVariable(VID_USER_ID, m_dwUserId);
			msg.SetVariable(VID_SESSION_ID, m_dwIndex);
			msg.SetVariable(VID_CHANGE_PASSWD_FLAG, (WORD)changePasswd);
         msg.SetVariable(VID_DBCONN_STATUS, (UINT16)((g_dwFlags & AF_DB_CONNECTION_LOST) ? 0 : 1));
			msg.SetVariable(VID_ZONING_ENABLED, (UINT16)((g_dwFlags & AF_ENABLE_ZONING) ? 1 : 0));
			msg.SetVariable(VID_POLLING_INTERVAL, ConfigReadULong(_T("DefaultDCIPollingInterval"), 60));
			msg.SetVariable(VID_RETENTION_TIME, ConfigReadULong(_T("DefaultDCIRetentionTime"), 30));
			msg.SetVariable(VID_ALARM_STATUS_FLOW_STATE, (UINT16)ConfigReadInt(_T("StrictAlarmStatusFlow"), 0));
			msg.SetVariable(VID_TIMED_ALARM_ACK_ENABLED, (UINT16)ConfigReadInt(_T("EnableTimedAlarmAck"), 0));
			msg.SetVariable(VID_VIEW_REFRESH_INTERVAL, (UINT16)ConfigReadInt(_T("MinViewRefreshInterval"), 200));
			msg.SetVariable(VID_HELPDESK_LINK_ACTIVE, (UINT16)((g_dwFlags & AF_HELPDESK_LINK_ACTIVE) ? 1 : 0));
         debugPrintf(3, _T("User %s authenticated"), m_szUserName);
			WriteAuditLog(AUDIT_SECURITY, TRUE, m_dwUserId, m_workstation, 0,
			              _T("User \"%s\" logged in (client info: %s)"), szLogin, m_szClientInfo);
      }
      else
      {
         msg.SetVariable(VID_RCC, dwResult);
			WriteAuditLog(AUDIT_SECURITY, FALSE, m_dwUserId, m_workstation, 0,
			              _T("User \"%s\" login failed with error code %d (client info: %s)"),
							  szLogin, dwResult, m_szClientInfo);
			if (intruderLockout)
			{
				WriteAuditLog(AUDIT_SECURITY, FALSE, m_dwUserId, m_workstation, 0,
								  _T("User account \"%s\" temporary disabled due to excess count of failed authentication attempts"), szLogin);
			}
      }
   }
   else
   {
      msg.SetVariable(VID_RCC, RCC_OUT_OF_STATE_REQUEST);
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
   CSCPMessage msg;
   TCHAR szBuffer[4096];

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(dwRqId);

   if (checkSysAccessRights(SYSTEM_ACCESS_VIEW_EVENT_DB) || checkSysAccessRights(SYSTEM_ACCESS_EDIT_EVENT_DB) || checkSysAccessRights(SYSTEM_ACCESS_EPP))
   {
      if (!(g_dwFlags & AF_DB_CONNECTION_LOST))
      {
         msg.SetVariable(VID_RCC, RCC_SUCCESS);
         sendMessage(&msg);
         msg.deleteAllVariables();

         // Prepare data message
         msg.SetCode(CMD_EVENT_DB_RECORD);
         msg.SetId(dwRqId);

         DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
         hResult = DBAsyncSelect(hdb, _T("SELECT event_code,event_name,severity,flags,message,description FROM event_cfg"));
         if (hResult != NULL)
         {
            while(DBFetch(hResult))
            {
               msg.SetVariable(VID_EVENT_CODE, DBGetFieldAsyncULong(hResult, 0));
               msg.SetVariable(VID_NAME, DBGetFieldAsync(hResult, 1, szBuffer, 1024));
               msg.SetVariable(VID_SEVERITY, DBGetFieldAsyncULong(hResult, 2));
               msg.SetVariable(VID_FLAGS, DBGetFieldAsyncULong(hResult, 3));

               DBGetFieldAsync(hResult, 4, szBuffer, 4096);
               msg.SetVariable(VID_MESSAGE, szBuffer);

               DBGetFieldAsync(hResult, 5, szBuffer, 4096);
               msg.SetVariable(VID_DESCRIPTION, szBuffer);

               sendMessage(&msg);
               msg.deleteAllVariables();
            }
            DBFreeAsyncResult(hResult);
         }
         DBConnectionPoolReleaseConnection(hdb);

         // End-of-list indicator
         msg.SetVariable(VID_EVENT_CODE, (UINT32)0);
			msg.setEndOfSequence();
      }
      else
      {
         msg.SetVariable(VID_RCC, RCC_DB_CONNECTION_LOST);
      }
   }
   else
   {
      msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
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
		session->postMessage((CSCPMessage *)arg);
}

/**
 * Update event template
 */
void ClientSession::modifyEventTemplate(CSCPMessage *pRequest)
{
   CSCPMessage msg;

   // Prepare reply message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   // Check access rights
   if (checkSysAccessRights(SYSTEM_ACCESS_EDIT_EVENT_DB))
   {
      TCHAR szQuery[8192], szName[MAX_EVENT_NAME];

      DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

      // Check if event with specific code exists
      UINT32 dwEventCode = pRequest->GetVariableLong(VID_EVENT_CODE);
		bool bEventExist = IsDatabaseRecordExist(hdb, _T("event_cfg"), _T("event_code"), dwEventCode);

      // Check that we are not trying to create event below 100000
      if (bEventExist || (dwEventCode >= FIRST_USER_EVENT_ID))
      {
         // Prepare and execute SQL query
         pRequest->GetVariableStr(VID_NAME, szName, MAX_EVENT_NAME);
         if (IsValidObjectName(szName))
         {
				TCHAR szMessage[MAX_EVENT_MSG_LENGTH], *pszDescription;

            pRequest->GetVariableStr(VID_MESSAGE, szMessage, MAX_EVENT_MSG_LENGTH);
            pszDescription = pRequest->GetVariableStr(VID_DESCRIPTION);

            if (bEventExist)
            {
               _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("UPDATE event_cfg SET event_name='%s',severity=%d,flags=%d,message=%s,description=%s WHERE event_code=%d"),
                       szName, pRequest->GetVariableLong(VID_SEVERITY), pRequest->GetVariableLong(VID_FLAGS),
							  (const TCHAR *)DBPrepareString(g_hCoreDB, szMessage),
							  (const TCHAR *)DBPrepareString(g_hCoreDB, pszDescription), dwEventCode);
            }
            else
            {
               _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("INSERT INTO event_cfg (event_code,event_name,severity,flags,")
                                _T("message,description) VALUES (%d,'%s',%d,%d,%s,%s)"),
                       dwEventCode, szName, pRequest->GetVariableLong(VID_SEVERITY),
                       pRequest->GetVariableLong(VID_FLAGS), (const TCHAR *)DBPrepareString(g_hCoreDB, szMessage),
							  (const TCHAR *)DBPrepareString(g_hCoreDB, pszDescription));
            }

            safe_free(pszDescription);

            if (DBQuery(hdb, szQuery))
            {
               msg.SetVariable(VID_RCC, RCC_SUCCESS);
               ReloadEvents();
               NotifyClientSessions(NX_NOTIFY_EVENTDB_CHANGED, 0);

					CSCPMessage nmsg(pRequest);
					nmsg.SetCode(CMD_EVENT_DB_UPDATE);
					nmsg.SetVariable(VID_NOTIFICATION_CODE, (WORD)NX_NOTIFY_ETMPL_CHANGED);
					EnumerateClientSessions(SendEventDBChangeNotification, &nmsg);
            }
            else
            {
               msg.SetVariable(VID_RCC, RCC_DB_FAILURE);
            }
         }
         else
         {
            msg.SetVariable(VID_RCC, RCC_INVALID_OBJECT_NAME);
         }
      }
      else
      {
         msg.SetVariable(VID_RCC, RCC_INVALID_EVENT_CODE);
      }
      DBConnectionPoolReleaseConnection(hdb);
   }
   else
   {
      msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Delete event template
 */
void ClientSession::deleteEventTemplate(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   UINT32 dwEventCode;

   // Prepare reply message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   dwEventCode = pRequest->GetVariableLong(VID_EVENT_CODE);

   // Check access rights
   if (checkSysAccessRights(SYSTEM_ACCESS_EDIT_EVENT_DB) && (dwEventCode >= FIRST_USER_EVENT_ID))
   {
      TCHAR szQuery[256];

      _sntprintf(szQuery, 256, _T("DELETE FROM event_cfg WHERE event_code=%d"), dwEventCode);
      if (DBQuery(g_hCoreDB, szQuery))
      {
         DeleteEventTemplateFromList(dwEventCode);
         NotifyClientSessions(NX_NOTIFY_EVENTDB_CHANGED, 0);

			CSCPMessage nmsg;
			nmsg.SetCode(CMD_EVENT_DB_UPDATE);
			nmsg.SetVariable(VID_NOTIFICATION_CODE, (WORD)NX_NOTIFY_ETMPL_DELETED);
			nmsg.SetVariable(VID_EVENT_CODE, dwEventCode);
			EnumerateClientSessions(SendEventDBChangeNotification, &nmsg);

         msg.SetVariable(VID_RCC, RCC_SUCCESS);

			WriteAuditLog(AUDIT_SYSCFG, TRUE, m_dwUserId, m_workstation, 0,
							  _T("Event template %d deleted"), dwEventCode);
      }
      else
      {
         msg.SetVariable(VID_RCC, RCC_DB_FAILURE);
      }
   }
   else
   {
      msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Generate event code for new event template
 */
void ClientSession::generateEventCode(UINT32 dwRqId)
{
   CSCPMessage msg;

   // Prepare reply message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(dwRqId);

   // Check access rights
   if (checkSysAccessRights(SYSTEM_ACCESS_EDIT_EVENT_DB))
   {
      msg.SetVariable(VID_EVENT_CODE, CreateUniqueId(IDG_EVENT));
   }
   else
   {
      msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Send all objects to client
 */
void ClientSession::sendAllObjects(CSCPMessage *pRequest)
{
   CSCPMessage msg;

   // Send confirmation message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());
   msg.SetVariable(VID_RCC, RCC_SUCCESS);
   sendMessage(&msg);
   msg.deleteAllVariables();

   // Change "sync comments" flag
   if (pRequest->GetVariableShort(VID_SYNC_COMMENTS))
      m_dwFlags |= CSF_SYNC_OBJECT_COMMENTS;
   else
      m_dwFlags &= ~CSF_SYNC_OBJECT_COMMENTS;

   // Get client's last known time stamp
   UINT32 dwTimeStamp = pRequest->GetVariableLong(VID_TIMESTAMP);

   // Prepare message
   msg.SetCode(CMD_OBJECT);

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
         object->CreateMessage(&msg);
         if (m_dwFlags & CSF_SYNC_OBJECT_COMMENTS)
            object->commentsToMessage(&msg);
         if (!object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_MODIFY))
         {
            // mask passwords
            msg.SetVariable(VID_SHARED_SECRET, _T("********"));
            msg.SetVariable(VID_SNMP_AUTH_PASSWORD, _T("********"));
            msg.SetVariable(VID_SNMP_PRIV_PASSWORD, _T("********"));
         }
         sendMessage(&msg);
         msg.deleteAllVariables();
      }
      object->decRefCount();
	}
	delete objects;

   // Send end of list notification
   msg.SetCode(CMD_OBJECT_LIST_END);
   sendMessage(&msg);

   MutexUnlock(m_mutexSendObjects);
}

/**
 * Send selected objects to client
 */
void ClientSession::sendSelectedObjects(CSCPMessage *pRequest)
{
   CSCPMessage msg;

   // Send confirmation message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());
   msg.SetVariable(VID_RCC, RCC_SUCCESS);
   sendMessage(&msg);
   msg.deleteAllVariables();

   // Change "sync comments" flag
   if (pRequest->GetVariableShort(VID_SYNC_COMMENTS))
      m_dwFlags |= CSF_SYNC_OBJECT_COMMENTS;
   else
      m_dwFlags &= ~CSF_SYNC_OBJECT_COMMENTS;

   UINT32 dwTimeStamp = pRequest->GetVariableLong(VID_TIMESTAMP);
	UINT32 numObjects = pRequest->GetVariableLong(VID_NUM_OBJECTS);
	UINT32 *objects = (UINT32 *)malloc(sizeof(UINT32) * numObjects);
	pRequest->getFieldAsInt32Array(VID_OBJECT_LIST, numObjects, objects);
	UINT32 options = pRequest->GetVariableShort(VID_FLAGS);

   MutexLock(m_mutexSendObjects);

   // Prepare message
	msg.SetCode((options & OBJECT_SYNC_SEND_UPDATES) ? CMD_OBJECT_UPDATE : CMD_OBJECT);

   // Send objects, one per message
   for(UINT32 i = 0; i < numObjects; i++)
	{
		NetObj *object = FindObjectById(objects[i]);
      if ((object != NULL) &&
		    object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ) &&
          (object->getTimeStamp() >= dwTimeStamp) &&
          !object->isHidden() && !object->isSystem())
      {
         object->CreateMessage(&msg);
         if (m_dwFlags & CSF_SYNC_OBJECT_COMMENTS)
            object->commentsToMessage(&msg);
         if (!object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_MODIFY))
         {
            // mask passwords
            msg.SetVariable(VID_SHARED_SECRET, _T("********"));
            msg.SetVariable(VID_SNMP_AUTH_PASSWORD, _T("********"));
            msg.SetVariable(VID_SNMP_PRIV_PASSWORD, _T("********"));
         }
         sendMessage(&msg);
         msg.deleteAllVariables();
      }
	}

   MutexUnlock(m_mutexSendObjects);
	safe_free(objects);

	if (options & OBJECT_SYNC_DUAL_CONFIRM)
	{
		msg.SetCode(CMD_REQUEST_COMPLETED);
		msg.SetVariable(VID_RCC, RCC_SUCCESS);
      sendMessage(&msg);
	}
}

/**
 * Send event log records to client
 */
void ClientSession::sendEventLog(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   DB_ASYNC_RESULT hResult = NULL;
   DB_RESULT hTempResult;
   UINT32 dwRqId, dwMaxRecords, dwNumRows, dwId;
   TCHAR szQuery[1024], szBuffer[1024];
   WORD wRecOrder;

   dwRqId = pRequest->GetId();
   dwMaxRecords = pRequest->GetVariableLong(VID_MAX_RECORDS);
   wRecOrder = ((g_nDBSyntax == DB_SYNTAX_MSSQL) || (g_nDBSyntax == DB_SYNTAX_ORACLE)) ? RECORD_ORDER_REVERSED : RECORD_ORDER_NORMAL;

   // Prepare confirmation message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(dwRqId);

   MutexLock(m_mutexSendEvents);
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   // Retrieve events from database
   switch(g_nDBSyntax)
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
      msg.SetVariable(VID_RCC, RCC_SUCCESS);
   	sendMessage(&msg);
   	msg.deleteAllVariables();
	   msg.SetCode(CMD_EVENTLOG_RECORDS);

      for(dwId = VID_EVENTLOG_MSG_BASE, dwNumRows = 0; DBFetch(hResult); dwNumRows++)
      {
         if (dwNumRows == 10)
         {
            msg.SetVariable(VID_NUM_RECORDS, dwNumRows);
            msg.SetVariable(VID_RECORDS_ORDER, wRecOrder);
            sendMessage(&msg);
            msg.deleteAllVariables();
            dwNumRows = 0;
            dwId = VID_EVENTLOG_MSG_BASE;
         }
         msg.SetVariable(dwId++, DBGetFieldAsyncUInt64(hResult, 0));
         msg.SetVariable(dwId++, DBGetFieldAsyncULong(hResult, 1));
         msg.SetVariable(dwId++, DBGetFieldAsyncULong(hResult, 2));
         msg.SetVariable(dwId++, DBGetFieldAsyncULong(hResult, 3));
         msg.SetVariable(dwId++, (WORD)DBGetFieldAsyncLong(hResult, 4));
         DBGetFieldAsync(hResult, 5, szBuffer, 1024);
         msg.SetVariable(dwId++, szBuffer);
         DBGetFieldAsync(hResult, 6, szBuffer, 1024);
         msg.SetVariable(dwId++, szBuffer);
         msg.SetVariable(dwId++, (UINT32)0);	// Do not send parameters
      }
      DBFreeAsyncResult(hResult);

      // Send remaining records with End-Of-Sequence notification
      msg.SetVariable(VID_NUM_RECORDS, dwNumRows);
      msg.SetVariable(VID_RECORDS_ORDER, wRecOrder);
      msg.setEndOfSequence();
      sendMessage(&msg);
   }
   else
   {
      msg.SetVariable(VID_RCC, RCC_DB_FAILURE);
   	sendMessage(&msg);
	}

   DBConnectionPoolReleaseConnection(hdb);
   MutexUnlock(m_mutexSendEvents);
}

/**
 * Send all configuration variables to client
 */
void ClientSession::sendAllConfigVars(UINT32 dwRqId)
{
   UINT32 i, dwId, dwNumRecords;
   CSCPMessage msg;
   DB_RESULT hResult;
   TCHAR szBuffer[MAX_DB_STRING];

   // Prepare message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(dwRqId);

   // Check user rights
   if ((m_dwUserId == 0) || (m_dwSystemAccess & SYSTEM_ACCESS_SERVER_CONFIG))
   {
      // Retrieve configuration variables from database
      hResult = DBSelect(g_hCoreDB, _T("SELECT var_name,var_value,need_server_restart FROM config WHERE is_visible=1"));
      if (hResult != NULL)
      {
         // Send events, one per message
         dwNumRecords = DBGetNumRows(hResult);
         msg.SetVariable(VID_NUM_VARIABLES, dwNumRecords);
         for(i = 0, dwId = VID_VARLIST_BASE; i < dwNumRecords; i++)
         {
            msg.SetVariable(dwId++, DBGetField(hResult, i, 0, szBuffer, MAX_DB_STRING));
            DBGetField(hResult, i, 1, szBuffer, MAX_DB_STRING);
            DecodeSQLString(szBuffer);
            msg.SetVariable(dwId++, szBuffer);
            msg.SetVariable(dwId++, (WORD)DBGetFieldLong(hResult, i, 2));
         }
         DBFreeResult(hResult);
      }
      msg.SetVariable(VID_RCC, RCC_SUCCESS);
   }
   else
   {
      msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Set configuration variable's value
 */
void ClientSession::setConfigVariable(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   TCHAR szName[MAX_OBJECT_NAME], szValue[MAX_DB_STRING];

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   // Check user rights
   if ((m_dwUserId == 0) || (m_dwSystemAccess & SYSTEM_ACCESS_SERVER_CONFIG))
   {
      pRequest->GetVariableStr(VID_NAME, szName, MAX_OBJECT_NAME);
      pRequest->GetVariableStr(VID_VALUE, szValue, MAX_DB_STRING);
      if (ConfigWriteStr(szName, szValue, TRUE))
		{
         msg.SetVariable(VID_RCC, RCC_SUCCESS);
			WriteAuditLog(AUDIT_SYSCFG, TRUE, m_dwUserId, m_workstation, 0,
							  _T("Server configuration variable \"%s\" set to \"%s\""), szName, szValue);
		}
      else
		{
         msg.SetVariable(VID_RCC, RCC_DB_FAILURE);
		}
   }
   else
   {
      msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Delete configuration variable
 */
void ClientSession::deleteConfigVariable(CSCPMessage *pRequest)
{
   CSCPMessage msg;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   // Check user rights
   if ((m_dwUserId == 0) || (m_dwSystemAccess & SYSTEM_ACCESS_SERVER_CONFIG))
   {
      TCHAR name[MAX_OBJECT_NAME];
      pRequest->GetVariableStr(VID_NAME, name, MAX_OBJECT_NAME);
      if (ConfigDelete(name))
		{
         msg.SetVariable(VID_RCC, RCC_SUCCESS);
			WriteAuditLog(AUDIT_SYSCFG, TRUE, m_dwUserId, m_workstation, 0, _T("Server configuration variable \"%s\" deleted"), name);
		}
      else
		{
         msg.SetVariable(VID_RCC, RCC_DB_FAILURE);
		}
   }
   else
   {
      msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Set configuration clob
 */
void ClientSession::setConfigCLOB(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   TCHAR name[MAX_OBJECT_NAME], *value;

	msg.SetId(pRequest->GetId());
	msg.SetCode(CMD_REQUEST_COMPLETED);

	if (m_dwSystemAccess & SYSTEM_ACCESS_SERVER_CONFIG)
	{
      pRequest->GetVariableStr(VID_NAME, name, MAX_OBJECT_NAME);
		value = pRequest->GetVariableStr(VID_VALUE);
		if (value != NULL)
		{
			if (ConfigWriteCLOB(name, value, TRUE))
			{
				msg.SetVariable(VID_RCC, RCC_SUCCESS);
				WriteAuditLog(AUDIT_SYSCFG, TRUE, m_dwUserId, m_workstation, 0,
								  _T("Server configuration variable \"%s\" set to \"%s\""), name, value);
				free(value);
			}
			else
			{
				msg.SetVariable(VID_RCC, RCC_DB_FAILURE);
			}
		}
		else
		{
			msg.SetVariable(VID_RCC, RCC_INVALID_REQUEST);
		}
	}
	else
	{
		msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
	}

	sendMessage(&msg);
}

/**
 * Get value of configuration clob
 */
void ClientSession::getConfigCLOB(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   TCHAR name[MAX_OBJECT_NAME], *value;

	msg.SetId(pRequest->GetId());
	msg.SetCode(CMD_REQUEST_COMPLETED);

	if (m_dwSystemAccess & SYSTEM_ACCESS_SERVER_CONFIG)
	{
      pRequest->GetVariableStr(VID_NAME, name, MAX_OBJECT_NAME);
		value = ConfigReadCLOB(name, NULL);
		if (value != NULL)
		{
			msg.SetVariable(VID_VALUE, value);
			msg.SetVariable(VID_RCC, RCC_SUCCESS);
			free(value);
		}
		else
		{
			msg.SetVariable(VID_RCC, RCC_UNKNOWN_VARIABLE);
		}
	}
	else
	{
		msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
	}

	sendMessage(&msg);
}

/**
 * Close session
 */
void ClientSession::kill()
{
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
   CSCPMessage *msg;

   if (isAuthenticated() && (m_dwActiveChannels & NXC_CHANNEL_EVENTS))
   {
      pUpdate = (UPDATE_INFO *)malloc(sizeof(UPDATE_INFO));
      pUpdate->dwCategory = INFO_CAT_EVENT;
      msg = new CSCPMessage;
      msg->SetCode(CMD_EVENTLOG_RECORDS);
      pEvent->prepareMessage(msg);
      pUpdate->pData = msg;
      m_pUpdateQueue->Put(pUpdate);
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
         m_pUpdateQueue->Put(pUpdate);
      }
}

/**
 * Send notification message to server
 */
void ClientSession::notify(UINT32 dwCode, UINT32 dwData)
{
   CSCPMessage msg;

   msg.SetCode(CMD_NOTIFY);
   msg.SetVariable(VID_NOTIFICATION_CODE, dwCode);
   msg.SetVariable(VID_NOTIFICATION_DATA, dwData);
   sendMessage(&msg);
}

/**
 * Modify object
 */
void ClientSession::modifyObject(CSCPMessage *pRequest)
{
   UINT32 dwObjectId, dwResult = RCC_SUCCESS;
   NetObj *object;
   CSCPMessage msg;

   // Prepare reply message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   dwObjectId = pRequest->GetVariableLong(VID_OBJECT_ID);
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
				pRequest->GetVariableStr(VID_OBJECT_NAME, name, 256);
				if (!IsValidObjectName(name, TRUE))
					dwResult = RCC_INVALID_OBJECT_NAME;
			}

         // If allowed, change object and set completion code
         if (dwResult == RCC_SUCCESS)
			{
            dwResult = object->ModifyFromMessage(pRequest);
	         if (dwResult == RCC_SUCCESS)
				{
					object->postModify();
				}
			}
         msg.SetVariable(VID_RCC, dwResult);

			if (dwResult == RCC_SUCCESS)
			{
				WriteAuditLog(AUDIT_OBJECTS, TRUE, m_dwUserId, m_workstation, dwObjectId,
								  _T("Object %s modified from client"), object->Name());
			}
			else
			{
				WriteAuditLog(AUDIT_OBJECTS, FALSE, m_dwUserId, m_workstation, dwObjectId,
								  _T("Failed to modify object from client - error %d"), dwResult);
			}
      }
      else
      {
         msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
			WriteAuditLog(AUDIT_OBJECTS, FALSE, m_dwUserId, m_workstation, dwObjectId,
							  _T("Failed to modify object from client - access denied"), dwResult);
      }
   }
   else
   {
      msg.SetVariable(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Send users database to client
 */
void ClientSession::sendUserDB(UINT32 dwRqId)
{
   CSCPMessage msg;
	UserDatabaseObject **users;
   int i, userCount;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_RCC, RCC_SUCCESS);
   sendMessage(&msg);
	msg.deleteAllVariables();

   // Send user database
	users = OpenUserDatabase(&userCount);
   for(i = 0; i < userCount; i++)
   {
		msg.SetCode((users[i]->getId() & GROUP_FLAG) ? CMD_GROUP_DATA : CMD_USER_DATA);
		users[i]->fillMessage(&msg);
      sendMessage(&msg);
      msg.deleteAllVariables();
   }
	CloseUserDatabase();

   // Send end-of-database notification
   msg.SetCode(CMD_USER_DB_EOF);
   sendMessage(&msg);
}

/**
 * Create new user
 */
void ClientSession::createUser(CSCPMessage *pRequest)
{
   CSCPMessage msg;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   // Check user rights
   if (!(m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_USERS))
   {
      msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
   }
   else if (!(m_dwFlags & CSF_USER_DB_LOCKED))
   {
      // User database have to be locked before any
      // changes to user database can be made
      msg.SetVariable(VID_RCC, RCC_OUT_OF_STATE_REQUEST);
   }
   else
   {
      UINT32 dwResult, dwUserId;
      BOOL bIsGroup;
      TCHAR szUserName[MAX_USER_NAME];

      pRequest->GetVariableStr(VID_USER_NAME, szUserName, MAX_USER_NAME);
      if (IsValidObjectName(szUserName))
      {
         bIsGroup = pRequest->GetVariableShort(VID_IS_GROUP);
         dwResult = CreateNewUser(szUserName, bIsGroup, &dwUserId);
         msg.SetVariable(VID_RCC, dwResult);
         if (dwResult == RCC_SUCCESS)
         {
            msg.SetVariable(VID_USER_ID, dwUserId);   // Send id of new user to client
            WriteAuditLog(AUDIT_SECURITY, TRUE, m_dwUserId, m_workstation, dwUserId, _T("%s %s created"), bIsGroup ? _T("Group") : _T("User"), szUserName);
         }
      }
      else
      {
         msg.SetVariable(VID_RCC, RCC_INVALID_OBJECT_NAME);
      }
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Update existing user's data
 */
void ClientSession::updateUser(CSCPMessage *pRequest)
{
   CSCPMessage msg;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   // Check user rights
   if (!(m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_USERS))
   {
      msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
   }
   else if (!(m_dwFlags & CSF_USER_DB_LOCKED))
   {
      // User database have to be locked before any
      // changes to user database can be made
      msg.SetVariable(VID_RCC, RCC_OUT_OF_STATE_REQUEST);
   }
   else
   {
      UINT32 result = ModifyUserDatabaseObject(pRequest);
      if (result == RCC_SUCCESS)
      {
         TCHAR name[MAX_DB_STRING];
         UINT32 id = pRequest->GetVariableLong(VID_USER_ID);
         ResolveUserId(id, name, MAX_DB_STRING);
         WriteAuditLog(AUDIT_SECURITY, TRUE, m_dwUserId, m_workstation, id,
            _T("%s %s modified"), (id & GROUP_FLAG) ? _T("Group") : _T("User"), name);
      }
      msg.SetVariable(VID_RCC, result);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Delete user
 */
void ClientSession::deleteUser(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   UINT32 dwUserId;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   // Check user rights
   if (!(m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_USERS))
   {
      msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
   }
   else if (!(m_dwFlags & CSF_USER_DB_LOCKED))
   {
      // User database have to be locked before any
      // changes to user database can be made
      msg.SetVariable(VID_RCC, RCC_OUT_OF_STATE_REQUEST);
   }
   else
   {
      // Get Id of user to be deleted
      dwUserId = pRequest->GetVariableLong(VID_USER_ID);

      if ((dwUserId != 0) && (dwUserId != GROUP_EVERYONE))
      {
         if (!IsLoggedIn(dwUserId))
         {
            TCHAR name[MAX_DB_STRING];
            ResolveUserId(dwUserId, name, MAX_DB_STRING);
            UINT32 rcc =  DeleteUserDatabaseObject(dwUserId);
            msg.SetVariable(VID_RCC, rcc);
            if(rcc == RCC_SUCCESS)
            {
               WriteAuditLog(AUDIT_SECURITY, TRUE, m_dwUserId, m_workstation, dwUserId,
                             _T("%s %s [%d] deleted"), (dwUserId & GROUP_FLAG) ? _T("Group") : _T("User"), name, dwUserId);
            }
         }
         else
         {
            // logger in users cannot be deleted
            msg.SetVariable(VID_RCC, RCC_USER_LOGGED_IN);
         }
      }
      else
      {
         // System administrator account and everyone group cannot be deleted
         msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
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
   CSCPMessage msg;
   TCHAR szBuffer[256];

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(dwRqId);

   if (m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_USERS)
   {
      if (bLock)
      {
         if (!LockComponent(CID_USER_DB, m_dwIndex, m_szUserName, NULL, szBuffer))
         {
            msg.SetVariable(VID_RCC, RCC_COMPONENT_LOCKED);
            msg.SetVariable(VID_LOCKED_BY, szBuffer);
         }
         else
         {
            m_dwFlags |= CSF_USER_DB_LOCKED;
            msg.SetVariable(VID_RCC, RCC_SUCCESS);
         }
      }
      else
      {
         if (m_dwFlags & CSF_USER_DB_LOCKED)
         {
            UnlockComponent(CID_USER_DB);
            m_dwFlags &= ~CSF_USER_DB_LOCKED;
         }
         msg.SetVariable(VID_RCC, RCC_SUCCESS);
      }
   }
   else
   {
      // Current user has no rights for user account management
      msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Notify client on user database update
 */
void ClientSession::onUserDBUpdate(int code, UINT32 id, UserDatabaseObject *object)
{
   CSCPMessage msg;

   if (isAuthenticated())
   {
      msg.SetCode(CMD_USER_DB_UPDATE);
      msg.SetId(0);
      msg.SetVariable(VID_UPDATE_TYPE, (WORD)code);

      switch(code)
      {
         case USER_DB_CREATE:
         case USER_DB_MODIFY:
				object->fillMessage(&msg);
            break;
         default:
            msg.SetVariable(VID_USER_ID, id);
            break;
      }

      sendMessage(&msg);
   }
}

/**
 * Change management status for the object
 */
void ClientSession::changeObjectMgmtStatus(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   UINT32 dwObjectId;
   NetObj *object;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   // Get object id and check access rights
   dwObjectId = pRequest->GetVariableLong(VID_OBJECT_ID);
   object = FindObjectById(dwObjectId);
   if (object != NULL)
   {
      if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_MODIFY))
      {
			if ((object->Type() != OBJECT_TEMPLATE) &&
				 (object->Type() != OBJECT_TEMPLATEGROUP) &&
				 (object->Type() != OBJECT_TEMPLATEROOT))
			{
				BOOL bIsManaged = (BOOL)pRequest->GetVariableShort(VID_MGMT_STATUS);
				object->setMgmtStatus(bIsManaged);
				msg.SetVariable(VID_RCC, RCC_SUCCESS);
            WriteAuditLog(AUDIT_OBJECTS, TRUE, m_dwUserId, m_workstation, object->Id(),
               _T("Object %s set to %s state"), object->Name(), bIsManaged ? _T("managed") : _T("unmanaged"));
			}
			else
			{
	         msg.SetVariable(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
			}
      }
      else
      {
         msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
      }
   }
   else
   {
      msg.SetVariable(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Set user's password
 */
void ClientSession::setPassword(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   UINT32 dwUserId;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   dwUserId = pRequest->GetVariableLong(VID_USER_ID);

   if (((m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_USERS) &&
        !((dwUserId == 0) && (m_dwUserId != 0))) ||   // Only administrator can change password for UID 0
       (dwUserId == m_dwUserId))     // User can change password for itself
   {
      UINT32 dwResult;
      TCHAR newPassword[1024], oldPassword[1024];

#ifdef UNICODE
      pRequest->GetVariableStr(VID_PASSWORD, newPassword, 256);
		if (pRequest->isFieldExist(VID_OLD_PASSWORD))
			pRequest->GetVariableStr(VID_OLD_PASSWORD, oldPassword, 256);
#else
      pRequest->GetVariableStrUTF8(VID_PASSWORD, newPassword, 1024);
		if (pRequest->isFieldExist(VID_OLD_PASSWORD))
			pRequest->GetVariableStrUTF8(VID_OLD_PASSWORD, oldPassword, 1024);
#endif
		else
			oldPassword[0] = 0;
      dwResult = SetUserPassword(dwUserId, newPassword, oldPassword, dwUserId == m_dwUserId);
      msg.SetVariable(VID_RCC, dwResult);

      if (dwResult == RCC_SUCCESS)
      {
         TCHAR userName[MAX_DB_STRING];
         ResolveUserId(dwUserId, userName, MAX_DB_STRING);
         WriteAuditLog(AUDIT_SECURITY, TRUE, m_dwUserId, m_workstation, 0, _T("Changed password for user %s"), userName);
      }
   }
   else
   {
      // Current user has no rights to change password for specific user
      msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Send node's DCIs to client and lock data collection settings
 */
void ClientSession::openNodeDCIList(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   UINT32 dwObjectId;
   NetObj *object;
   BOOL bSuccess = FALSE;
   TCHAR szLockInfo[MAX_SESSION_NAME];

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   // Get node id and check object class and access rights
   dwObjectId = pRequest->GetVariableLong(VID_OBJECT_ID);
   object = FindObjectById(dwObjectId);
   if (object != NULL)
   {
      if ((object->Type() == OBJECT_NODE) ||
          (object->Type() == OBJECT_CLUSTER) ||
          (object->Type() == OBJECT_MOBILEDEVICE) ||
          (object->Type() == OBJECT_TEMPLATE))
      {
         if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
         {
            // Try to lock DCI list
            if (((Template *)object)->lockDCIList(m_dwIndex, m_szUserName, szLockInfo))
            {
               bSuccess = TRUE;
               msg.SetVariable(VID_RCC, RCC_SUCCESS);

               // Modify list of open nodes DCI lists
               m_pOpenDCIList = (UINT32 *)realloc(m_pOpenDCIList, sizeof(UINT32) * (m_dwOpenDCIListSize + 1));
               m_pOpenDCIList[m_dwOpenDCIListSize] = dwObjectId;
               m_dwOpenDCIListSize++;
            }
            else
            {
               msg.SetVariable(VID_RCC, RCC_COMPONENT_LOCKED);
               msg.SetVariable(VID_LOCKED_BY, szLockInfo);
            }
         }
         else
         {
            msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
         }
      }
      else
      {
         msg.SetVariable(VID_RCC, RCC_INVALID_OBJECT_ID);
      }
   }
   else
   {
      msg.SetVariable(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   // Send response
   sendMessage(&msg);

   // If DCI list was successfully locked, send it to client
   if (bSuccess)
      ((Template *)object)->sendItemsToClient(this, pRequest->GetId());
}

/**
 * Unlock node's data collection settings
 */
void ClientSession::closeNodeDCIList(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   UINT32 dwObjectId;
   NetObj *object;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   // Get node id and check object class and access rights
   dwObjectId = pRequest->GetVariableLong(VID_OBJECT_ID);
   object = FindObjectById(dwObjectId);
   if (object != NULL)
   {
      if ((object->Type() == OBJECT_NODE) ||
          (object->Type() == OBJECT_CLUSTER) ||
          (object->Type() == OBJECT_MOBILEDEVICE) ||
          (object->Type() == OBJECT_TEMPLATE))
      {
         if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
         {
            BOOL bSuccess;

            // Try to unlock DCI list
            bSuccess = ((Template *)object)->unlockDCIList(m_dwIndex);
            msg.SetVariable(VID_RCC, bSuccess ? RCC_SUCCESS : RCC_OUT_OF_STATE_REQUEST);

            // Modify list of open nodes DCI lists
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
            if ((object->Type() == OBJECT_TEMPLATE) || (object->Type() == OBJECT_CLUSTER))
               ((Template *)object)->queueUpdate();
         }
         else
         {
            msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
         }
      }
      else
      {
         msg.SetVariable(VID_RCC, RCC_INVALID_OBJECT_ID);
      }
   }
   else
   {
      msg.SetVariable(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Create, modify, or delete data collection item for node
 */
void ClientSession::modifyNodeDCI(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   UINT32 dwObjectId;
   NetObj *object;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   // Get node id and check object class and access rights
   dwObjectId = pRequest->GetVariableLong(VID_OBJECT_ID);
   object = FindObjectById(dwObjectId);
   if (object != NULL)
   {
      if ((object->Type() == OBJECT_NODE) ||
          (object->Type() == OBJECT_CLUSTER) ||
          (object->Type() == OBJECT_MOBILEDEVICE) ||
          (object->Type() == OBJECT_TEMPLATE))
      {
         if (((Template *)object)->isLockedBySession(m_dwIndex))
         {
            if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_MODIFY))
            {
               UINT32 i, dwItemId, dwNumMaps, *pdwMapId, *pdwMapIndex;
               DCObject *dcObject;
               BOOL bSuccess = FALSE;

					int dcObjectType = (int)pRequest->GetVariableShort(VID_DCOBJECT_TYPE);
               switch(pRequest->GetCode())
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
									msg.SetVariable(VID_RCC, RCC_SUCCESS);
									// Return new item id to client
									msg.SetVariable(VID_DCI_ID, dcObject->getId());
								}
								else  // Unable to add item to node
								{
									delete dcObject;
									msg.SetVariable(VID_RCC, RCC_DUPLICATE_DCI);
								}
							}
							else
							{
								msg.SetVariable(VID_RCC, RCC_INVALID_ARGUMENT);
							}
                     break;
                  case CMD_MODIFY_NODE_DCI:
                     dwItemId = pRequest->GetVariableLong(VID_DCI_ID);
							bSuccess = ((Template *)object)->updateDCObject(dwItemId, pRequest, &dwNumMaps, &pdwMapIndex, &pdwMapId);
                     if (bSuccess)
                     {
                        msg.SetVariable(VID_RCC, RCC_SUCCESS);

                        // Send index to id mapping for newly created thresholds to client
								if (dcObjectType == DCO_TYPE_ITEM)
								{
									msg.SetVariable(VID_DCI_NUM_MAPS, dwNumMaps);
									for(i = 0; i < dwNumMaps; i++)
									{
										pdwMapId[i] = htonl(pdwMapId[i]);
										pdwMapIndex[i] = htonl(pdwMapIndex[i]);
									}
									msg.SetVariable(VID_DCI_MAP_IDS, (BYTE *)pdwMapId, sizeof(UINT32) * dwNumMaps);
									msg.SetVariable(VID_DCI_MAP_INDEXES, (BYTE *)pdwMapIndex, sizeof(UINT32) * dwNumMaps);
									safe_free(pdwMapId);
									safe_free(pdwMapIndex);
								}
                     }
                     else
                     {
                        msg.SetVariable(VID_RCC, RCC_INVALID_DCI_ID);
                     }
                     break;
                  case CMD_DELETE_NODE_DCI:
                     dwItemId = pRequest->GetVariableLong(VID_DCI_ID);
                     bSuccess = ((Template *)object)->deleteDCObject(dwItemId, true);
                     msg.SetVariable(VID_RCC, bSuccess ? RCC_SUCCESS : RCC_INVALID_DCI_ID);
                     break;
               }
               if (bSuccess)
                  ((Template *)object)->setDCIModificationFlag();
            }
            else  // User doesn't have MODIFY rights on object
            {
               msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
            }
         }
         else  // Nodes DCI list not locked by this session
         {
            msg.SetVariable(VID_RCC, RCC_OUT_OF_STATE_REQUEST);
         }
      }
      else     // Object is not a node
      {
         msg.SetVariable(VID_RCC, RCC_INVALID_OBJECT_ID);
      }
   }
   else  // No object with given ID
   {
      msg.SetVariable(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Change status for one or more DCIs
 */
void ClientSession::changeDCIStatus(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   NetObj *object;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   // Get node id and check object class and access rights
   object = FindObjectById(pRequest->GetVariableLong(VID_OBJECT_ID));
   if (object != NULL)
   {
      if ((object->Type() == OBJECT_NODE) ||
          (object->Type() == OBJECT_CLUSTER) ||
          (object->Type() == OBJECT_MOBILEDEVICE) ||
          (object->Type() == OBJECT_TEMPLATE))
      {
         if (((Template *)object)->isLockedBySession(m_dwIndex))
         {
            if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_MODIFY))
            {
               UINT32 dwNumItems, *pdwItemList;
               int iStatus;

               iStatus = pRequest->GetVariableShort(VID_DCI_STATUS);
               dwNumItems = pRequest->GetVariableLong(VID_NUM_ITEMS);
               pdwItemList = (UINT32 *)malloc(sizeof(UINT32) * dwNumItems);
               pRequest->getFieldAsInt32Array(VID_ITEM_LIST, dwNumItems, pdwItemList);
               if (((Template *)object)->setItemStatus(dwNumItems, pdwItemList, iStatus))
                  msg.SetVariable(VID_RCC, RCC_SUCCESS);
               else
                  msg.SetVariable(VID_RCC, RCC_INVALID_DCI_ID);
               free(pdwItemList);
            }
            else  // User doesn't have MODIFY rights on object
            {
               msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
            }
         }
         else  // Nodes DCI list not locked by this session
         {
            msg.SetVariable(VID_RCC, RCC_OUT_OF_STATE_REQUEST);
         }
      }
      else     // Object is not a node
      {
         msg.SetVariable(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
      }
   }
   else  // No object with given ID
   {
      msg.SetVariable(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Clear all collected data for DCI
 */
void ClientSession::clearDCIData(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   NetObj *object;
	UINT32 dwItemId;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   // Get node id and check object class and access rights
   object = FindObjectById(pRequest->GetVariableLong(VID_OBJECT_ID));
   if (object != NULL)
   {
      if ((object->Type() == OBJECT_NODE) || (object->Type() == OBJECT_MOBILEDEVICE) || (object->Type() == OBJECT_CLUSTER))
      {
         if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_DELETE))
         {
				dwItemId = pRequest->GetVariableLong(VID_DCI_ID);
				debugPrintf(4, _T("ClearDCIData: request for DCI %d at node %d"), dwItemId, object->Id());
            DCObject *dci = ((Template *)object)->getDCObjectById(dwItemId);
				if (dci != NULL)
				{
					msg.SetVariable(VID_RCC, dci->deleteAllData() ? RCC_SUCCESS : RCC_DB_FAILURE);
					debugPrintf(4, _T("ClearDCIData: DCI %d at node %d"), dwItemId, object->Id());
				}
				else
				{
					msg.SetVariable(VID_RCC, RCC_INVALID_DCI_ID);
					debugPrintf(4, _T("ClearDCIData: DCI %d at node %d not found"), dwItemId, object->Id());
				}
         }
         else  // User doesn't have DELETE rights on object
         {
            msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
         }
      }
      else     // Object is not a node
      {
         msg.SetVariable(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
      }
   }
   else  // No object with given ID
   {
      msg.SetVariable(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Copy or move DCI from one node or template to another
 */
void ClientSession::copyDCI(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   NetObj *pSource, *pDestination;
   TCHAR szLockInfo[MAX_SESSION_NAME];
   BOOL bMove;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   // Get source and destination
   pSource = FindObjectById(pRequest->GetVariableLong(VID_SOURCE_OBJECT_ID));
   pDestination = FindObjectById(pRequest->GetVariableLong(VID_DESTINATION_OBJECT_ID));
   if ((pSource != NULL) && (pDestination != NULL))
   {
      // Check object types
      if (((pSource->Type() == OBJECT_NODE) || (pSource->Type() == OBJECT_MOBILEDEVICE) || (pSource->Type() == OBJECT_TEMPLATE) || (pSource->Type() == OBJECT_CLUSTER)) &&
		    ((pDestination->Type() == OBJECT_NODE) || (pDestination->Type() == OBJECT_MOBILEDEVICE) || (pDestination->Type() == OBJECT_TEMPLATE) || (pDestination->Type() == OBJECT_CLUSTER)))
      {
         if (((Template *)pSource)->isLockedBySession(m_dwIndex))
         {
            bMove = pRequest->GetVariableShort(VID_MOVE_FLAG);
            // Check access rights
            if ((pSource->checkAccessRights(m_dwUserId, bMove ? (OBJECT_ACCESS_READ | OBJECT_ACCESS_MODIFY) : OBJECT_ACCESS_READ)) &&
                (pDestination->checkAccessRights(m_dwUserId, OBJECT_ACCESS_MODIFY)))
            {
               // Attempt to lock destination's DCI list
               if ((pDestination->Id() == pSource->Id()) ||
                   (((Template *)pDestination)->lockDCIList(m_dwIndex, m_szUserName, szLockInfo)))
               {
                  UINT32 i, *pdwItemList, dwNumItems;
                  const DCObject *pSrcItem;
                  DCObject *pDstItem;
                  int iErrors = 0;

                  // Get list of items to be copied/moved
                  dwNumItems = pRequest->GetVariableLong(VID_NUM_ITEMS);
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
                  if (pDestination->Id() != pSource->Id())
                     ((Template *)pDestination)->unlockDCIList(m_dwIndex);
                  msg.SetVariable(VID_RCC, (iErrors == 0) ? RCC_SUCCESS : RCC_DCI_COPY_ERRORS);

                  // Queue template update
                  if (pDestination->Type() == OBJECT_TEMPLATE)
                     ((Template *)pDestination)->queueUpdate();
               }
               else  // Destination's DCI list already locked by someone else
               {
                  msg.SetVariable(VID_RCC, RCC_COMPONENT_LOCKED);
                  msg.SetVariable(VID_LOCKED_BY, szLockInfo);
               }
            }
            else  // User doesn't have enough rights on object(s)
            {
               msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
            }
         }
         else  // Source node DCI list not locked by this session
         {
            msg.SetVariable(VID_RCC, RCC_OUT_OF_STATE_REQUEST);
         }
      }
      else     // Object(s) is not a node
      {
         msg.SetVariable(VID_RCC, RCC_INVALID_OBJECT_ID);
      }
   }
   else  // No object(s) with given ID
   {
      msg.SetVariable(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Send list of thresholds for DCI
 */
void ClientSession::sendDCIThresholds(CSCPMessage *request)
{
   CSCPMessage msg;
   NetObj *object;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(request->GetId());

   // Get node id and check object class and access rights
   object = FindObjectById(request->GetVariableLong(VID_OBJECT_ID));
   if (object != NULL)
   {
      if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
      {
			if ((object->Type() == OBJECT_NODE) || (object->Type() == OBJECT_MOBILEDEVICE) || (object->Type() == OBJECT_CLUSTER))
			{
				DCObject *dci = ((Template *)object)->getDCObjectById(request->GetVariableLong(VID_DCI_ID));
				if ((dci != NULL) && (dci->getType() == DCO_TYPE_ITEM))
				{
					((DCItem *)dci)->fillMessageWithThresholds(&msg);
					msg.SetVariable(VID_RCC, RCC_SUCCESS);
				}
				else
				{
					msg.SetVariable(VID_RCC, RCC_INVALID_DCI_ID);
				}
         }
         else
         {
            msg.SetVariable(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
         }
      }
      else
      {
         msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
      }
   }
   else  // No object with given ID
   {
		msg.SetVariable(VID_RCC, RCC_INVALID_OBJECT_ID);
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

	switch(g_nDBSyntax)
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

	switch(g_nDBSyntax)
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
bool ClientSession::getCollectedDataFromDB(CSCPMessage *request, CSCPMessage *response, DataCollectionTarget *dcTarget, int dciType)
{
	// Find DCI object
	DCObject *dci = dcTarget->getDCObjectById(request->GetVariableLong(VID_DCI_ID));
	if (dci == NULL)
	{
		response->SetVariable(VID_RCC, RCC_INVALID_DCI_ID);
		return false;
	}

	// DCI type in request should match actual DCI type
	if (dci->getType() != dciType)
	{
		response->SetVariable(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
		return false;
	}

	// Check that all required data present in message
	if ((dciType == DCO_TYPE_TABLE) && (!request->isFieldExist(VID_DATA_COLUMN) || !request->isFieldExist(VID_INSTANCE)))
	{
		response->SetVariable(VID_RCC, RCC_INVALID_ARGUMENT);
		return false;
	}

	// Get request parameters
	UINT32 maxRows = request->GetVariableLong(VID_MAX_ROWS);
	UINT32 timeFrom = request->GetVariableLong(VID_TIME_FROM);
	UINT32 timeTo = request->GetVariableLong(VID_TIME_TO);

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
			hStmt = PrepareIDataSelect(hdb, dcTarget->Id(), maxRows, condition);
			break;
		case DCO_TYPE_TABLE:
			hStmt = PrepareTDataSelect(hdb, dcTarget->Id(), maxRows, condition);
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
			request->GetVariableStr(VID_DATA_COLUMN, dataColumn, MAX_COLUMN_NAME);

			DBBind(hStmt, pos++, DB_SQLTYPE_VARCHAR, request->GetVariableStr(VID_INSTANCE), DB_BIND_DYNAMIC);
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
			response->SetVariable(VID_RCC, RCC_SUCCESS);
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
			CSCP_MESSAGE *msg =
				CreateRawNXCPMessage(CMD_DCI_DATA, request->GetId(), 0,
											numRows * m_dwRowSize[dataType] + sizeof(DCI_DATA_HEADER),
											pData, NULL);
			free(pData);
			sendRawMessage(msg);
			free(msg);
			success = true;
		}
		else
		{
			response->SetVariable(VID_RCC, RCC_DB_FAILURE);
		}
		DBFreeStatement(hStmt);
	}
	else
	{
		response->SetVariable(VID_RCC, RCC_DB_FAILURE);
	}
	DBConnectionPoolReleaseConnection(hdb);
	return success;
}

/**
 * Get collected data
 */
void ClientSession::getCollectedData(CSCPMessage *request)
{
   CSCPMessage msg;
	bool success = false;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(request->GetId());

   NetObj *object = FindObjectById(request->GetVariableLong(VID_OBJECT_ID));
   if (object != NULL)
   {
		if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
		{
			if ((object->Type() == OBJECT_NODE) || (object->Type() == OBJECT_MOBILEDEVICE) || (object->Type() == OBJECT_CLUSTER))
			{
				if (!(g_dwFlags & AF_DB_CONNECTION_LOST))
				{
					success = getCollectedDataFromDB(request, &msg, (DataCollectionTarget *)object, DCO_TYPE_ITEM);
				}
				else
				{
					msg.SetVariable(VID_RCC, RCC_DB_CONNECTION_LOST);
				}
			}
			else
			{
				msg.SetVariable(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
			}
		}
		else
		{
			msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
		}
   }
   else  // No object with given ID
   {
      msg.SetVariable(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   // Send response
   if (!success)
      sendMessage(&msg);
}

/**
 * Get collected data for table DCI
 */
void ClientSession::getTableCollectedData(CSCPMessage *request)
{
   CSCPMessage msg;
	bool success = false;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(request->GetId());

   NetObj *object = FindObjectById(request->GetVariableLong(VID_OBJECT_ID));
   if (object != NULL)
   {
		if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
		{
			if ((object->Type() == OBJECT_NODE) || (object->Type() == OBJECT_MOBILEDEVICE) || (object->Type() == OBJECT_CLUSTER))
			{
				if (!(g_dwFlags & AF_DB_CONNECTION_LOST))
				{
					success = getCollectedDataFromDB(request, &msg, (DataCollectionTarget *)object, DCO_TYPE_TABLE);
				}
				else
				{
					msg.SetVariable(VID_RCC, RCC_DB_CONNECTION_LOST);
				}
			}
			else
			{
				msg.SetVariable(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
			}
		}
		else
		{
			msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
		}
   }
   else
   {
      msg.SetVariable(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

	if (!success)
      sendMessage(&msg);
}

/**
 * Send latest collected values for all DCIs of given node
 */
void ClientSession::getLastValues(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   NetObj *object;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   // Get node id and check object class and access rights
   object = FindObjectById(pRequest->GetVariableLong(VID_OBJECT_ID));
   if (object != NULL)
   {
      if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
      {
         if ((object->Type() == OBJECT_NODE) || (object->Type() == OBJECT_MOBILEDEVICE) ||
             (object->Type() == OBJECT_TEMPLATE) || (object->Type() == OBJECT_CLUSTER))
         {
            msg.SetVariable(VID_RCC, ((Template *)object)->getLastValues(&msg, pRequest->GetVariableShort(VID_OBJECT_TOOLTIP_ONLY) ? true : false));
         }
         else
         {
            msg.SetVariable(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
         }
      }
      else
      {
         msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
      }
   }
   else  // No object with given ID
   {
      msg.SetVariable(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Send latest collected values for all DCIs from given list.
 * Error message will never be returned. Will be returned only
 * possible DCI values.
 */
void ClientSession::getLastValuesByDciId(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   NetObj *object;
   DCObject *dcoObj;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());
   int size = pRequest->getFieldAsInt32(VID_NUM_ITEMS);
   UINT32 incomingIndex = VID_DCI_VALUES_BASE;
   UINT32 outgoingIndex = VID_DCI_VALUES_BASE;

   for(int i = 0 ; i < size; i++, incomingIndex+=10)
   {
      TCHAR *value;
      UINT32 type, status;

      object = FindObjectById(pRequest->GetVariableLong(incomingIndex));
      if (object != NULL)
      {
         if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
         {
            if ((object->Type() == OBJECT_NODE) || (object->Type() == OBJECT_MOBILEDEVICE) ||
                (object->Type() == OBJECT_TEMPLATE) || (object->Type() == OBJECT_CLUSTER))
            {
               UINT32 dciID = pRequest->GetVariableLong(incomingIndex+1);
               dcoObj = ((DataCollectionTarget *)object)->getDCObjectById(dciID);
               if(dcoObj == NULL)
                  continue;

               if (dcoObj->getType() == DCO_TYPE_TABLE)
               {
                  TCHAR * column = pRequest->GetVariableStr(incomingIndex+2);
                  TCHAR * instance = pRequest->GetVariableStr(incomingIndex+3);
                  if(column == NULL || instance == NULL || _tcscmp(column, _T("")) == 0 || _tcscmp(instance, _T("")) == 0)
                  {
                     continue;
                  }

                  Table *t = ((DCTable *)dcoObj)->getLastValue();
                  int columnIndex =  t->getColumnIndex(column);
                  int rowIndex = t->findRowByInstance(instance);
                  type = t->getColumnDataType(columnIndex);
                  value = _tcsdup(t->getAsString(rowIndex, columnIndex));
                  t->decRefCount();

                  safe_free(column);
                  safe_free(instance);
               }
               else
               {
                  if (dcoObj->getType() == DCO_TYPE_ITEM)
                  {
                     type = (WORD)((DCItem *)dcoObj)->getDataType();
                     value = _tcsdup(((DCItem *)dcoObj)->getLastValue());
                  }
                  else
                     continue;
               }


               status = dcoObj->getStatus();

               msg.SetVariable(outgoingIndex, dciID);
               msg.SetVariable(outgoingIndex+1, value);
               msg.SetVariable(outgoingIndex+2, type);
               msg.SetVariable(outgoingIndex+3, status);
               safe_free(value);
               outgoingIndex +=10;
            }
         }
      }
   }
   // Set result
   msg.SetVariable(VID_NUM_ITEMS, (outgoingIndex - VID_DCI_VALUES_BASE)/10);
   msg.SetVariable(VID_RCC, RCC_SUCCESS);
   // Send response
   sendMessage(&msg);
}

/**
 * Send latest collected values for given table DCI of given node
 */
void ClientSession::getTableLastValues(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   NetObj *object;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   // Get node id and check object class and access rights
   object = FindObjectById(pRequest->GetVariableLong(VID_OBJECT_ID));
   if (object != NULL)
   {
      if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
      {
         if ((object->Type() == OBJECT_NODE) || (object->Type() == OBJECT_MOBILEDEVICE) || (object->Type() == OBJECT_CLUSTER))
         {
				msg.SetVariable(VID_RCC, ((DataCollectionTarget *)object)->getTableLastValues(pRequest->GetVariableLong(VID_DCI_ID), &msg));
         }
         else
         {
            msg.SetVariable(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
         }
      }
      else
      {
         msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
      }
   }
   else  // No object with given ID
   {
      msg.SetVariable(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Open event processing policy
 */
void ClientSession::openEPP(CSCPMessage *request)
{
   CSCPMessage msg;
   bool success = false;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(request->GetId());

   bool readOnly = request->GetVariableShort(VID_READ_ONLY) ? true : false;

   if (checkSysAccessRights(SYSTEM_ACCESS_EPP))
   {
      TCHAR buffer[256];
      if (!readOnly && !LockComponent(CID_EPP, m_dwIndex, m_szUserName, NULL, buffer))
      {
         msg.SetVariable(VID_RCC, RCC_COMPONENT_LOCKED);
         msg.SetVariable(VID_LOCKED_BY, buffer);
      }
      else
      {
         if (!readOnly)
         {
            m_dwFlags |= CSF_EPP_LOCKED;
         }
         msg.SetVariable(VID_RCC, RCC_SUCCESS);
         msg.SetVariable(VID_NUM_RULES, g_pEventPolicy->getNumRules());
         success = true;
      }
   }
   else
   {
      // Current user has no rights for event policy management
      msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
   }

   // Send response
   sendMessage(&msg);

   // Send policy to client
   if (success)
      g_pEventPolicy->sendToClient(this, request->GetId());
}

/**
 * Close event processing policy
 */
void ClientSession::closeEPP(UINT32 dwRqId)
{
   CSCPMessage msg;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(dwRqId);

   if (m_dwSystemAccess & SYSTEM_ACCESS_EPP)
   {
      if (m_dwFlags & CSF_EPP_LOCKED)
      {
         UnlockComponent(CID_EPP);
         m_dwFlags &= ~CSF_EPP_LOCKED;
      }
      msg.SetVariable(VID_RCC, RCC_SUCCESS);
   }
   else
   {
      // Current user has no rights for event policy management
      msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
   }

   // Send response
   sendMessage(&msg);
}


//
// Save event processing policy
//

void ClientSession::saveEPP(CSCPMessage *pRequest)
{
   CSCPMessage msg;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   if (m_dwSystemAccess & SYSTEM_ACCESS_EPP)
   {
      if (m_dwFlags & CSF_EPP_LOCKED)
      {
         msg.SetVariable(VID_RCC, RCC_SUCCESS);
         m_dwNumRecordsToUpload = pRequest->GetVariableLong(VID_NUM_RULES);
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
         msg.SetVariable(VID_RCC, RCC_OUT_OF_STATE_REQUEST);
      }
   }
   else
   {
      // Current user has no rights for event policy management
      msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
   }

   // Send response
   sendMessage(&msg);
}


//
// Process EPP rule received from client
//

void ClientSession::processEPPRecord(CSCPMessage *pRequest)
{
   if (!(m_dwFlags & CSF_EPP_LOCKED))
   {
      CSCPMessage msg;

      msg.SetCode(CMD_REQUEST_COMPLETED);
      msg.SetId(pRequest->GetId());
      msg.SetVariable(VID_RCC, RCC_OUT_OF_STATE_REQUEST);
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
            CSCPMessage msg;

            // All records received, replace event policy...
            debugPrintf(5, _T("Replacing event processing policy with a new one at %p (%d rules)"),
                        m_ppEPPRuleList, m_dwNumRecordsToUpload);
            g_pEventPolicy->replacePolicy(m_dwNumRecordsToUpload, m_ppEPPRuleList);
            g_pEventPolicy->saveToDB();
            m_ppEPPRuleList = NULL;

            // ... and send final confirmation
            msg.SetCode(CMD_REQUEST_COMPLETED);
            msg.SetId(pRequest->GetId());
            msg.SetVariable(VID_RCC, RCC_SUCCESS);
            sendMessage(&msg);

            m_dwFlags &= ~CSF_EPP_UPLOAD;
         }
      }
   }
}

/**
 * Send compiled MIB file to client
 */
void ClientSession::sendMib(CSCPMessage *request)
{
   TCHAR szBuffer[MAX_PATH];

   // Send compiled MIB file
   _tcscpy(szBuffer, g_szDataDir);
   _tcscat(szBuffer, DFILE_COMPILED_MIB);
	sendFile(szBuffer, request->GetId(), 0);
}

/**
 * Send timestamp of compiled MIB file to client
 */
void ClientSession::sendMIBTimestamp(UINT32 dwRqId)
{
   CSCPMessage msg;
   TCHAR szBuffer[MAX_PATH];
   UINT32 dwResult, dwTimeStamp;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(dwRqId);

   _tcscpy(szBuffer, g_szDataDir);
   _tcscat(szBuffer, DFILE_COMPILED_MIB);
   dwResult = SNMPGetMIBTreeTimestamp(szBuffer, &dwTimeStamp);
   if (dwResult == SNMP_ERR_SUCCESS)
   {
      msg.SetVariable(VID_RCC, RCC_SUCCESS);
      msg.SetVariable(VID_TIMESTAMP, dwTimeStamp);
		msg.SetVariable(VID_FILE_SIZE, FileSize(szBuffer));
   }
   else
   {
      switch(dwResult)
      {
         case SNMP_ERR_FILE_IO:
            msg.SetVariable(VID_RCC, RCC_FILE_IO_ERROR);
            break;
         case SNMP_ERR_BAD_FILE_HEADER:
            msg.SetVariable(VID_RCC, RCC_CORRUPTED_MIB_FILE);
            break;
         default:
            msg.SetVariable(VID_RCC, RCC_INTERNAL_ERROR);
            break;
      }
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Create new object
 */
void ClientSession::createObject(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   NetObj *object = NULL, *pParent;
   int iClass, iServiceType;
   TCHAR szObjectName[MAX_OBJECT_NAME], nodePrimaryName[MAX_DNS_NAME], deviceId[MAX_OBJECT_NAME];
   TCHAR *pszRequest, *pszResponse, *pszComments;
   UINT32 dwIpAddr, zoneId, nodeId;
   WORD wIpProto, wIpPort;
	BYTE macAddr[MAC_ADDR_LENGTH];
   BOOL bParentAlwaysValid = FALSE;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   iClass = pRequest->GetVariableShort(VID_OBJECT_CLASS);
	zoneId = pRequest->GetVariableLong(VID_ZONE_ID);

   // Find parent object
   pParent = FindObjectById(pRequest->GetVariableLong(VID_PARENT_ID));
   if (iClass == OBJECT_NODE)
   {
		if (pRequest->isFieldExist(VID_PRIMARY_NAME))
		{
			pRequest->GetVariableStr(VID_PRIMARY_NAME, nodePrimaryName, MAX_DNS_NAME);
			dwIpAddr = ntohl(ResolveHostName(nodePrimaryName));
		}
		else
		{
			dwIpAddr = pRequest->GetVariableLong(VID_IP_ADDRESS);
			IpToStr(dwIpAddr, nodePrimaryName);
		}
      if ((pParent == NULL) && (dwIpAddr != 0))
      {
         pParent = FindSubnetForNode(zoneId, dwIpAddr);
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
         if (bParentAlwaysValid || IsValidParentClass(iClass, (pParent != NULL) ? pParent->Type() : -1))
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
					pRequest->GetVariableStr(VID_OBJECT_NAME, szObjectName, MAX_OBJECT_NAME);
					if (IsValidObjectName(szObjectName, TRUE))
					{
                  // Do additional validation by modules
                  UINT32 moduleRCC = RCC_SUCCESS;
                  for(UINT32 i = 0; i < g_dwNumModules; i++)
	               {
		               if (g_pModuleList[i].pfValidateObjectCreation != NULL)
		               {
                        moduleRCC = g_pModuleList[i].pfValidateObjectCreation(iClass, szObjectName, dwIpAddr, zoneId, pRequest);
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
								   object = PollNewNode(dwIpAddr,
															    pRequest->GetVariableLong(VID_IP_NETMASK),
															    pRequest->GetVariableLong(VID_CREATION_FLAGS),
															    pRequest->GetVariableShort(VID_AGENT_PORT),
															    pRequest->GetVariableShort(VID_SNMP_PORT),
															    szObjectName,
															    pRequest->GetVariableLong(VID_AGENT_PROXY),
															    pRequest->GetVariableLong(VID_SNMP_PROXY),
															    (pParent != NULL) ? ((pParent->Type() == OBJECT_CLUSTER) ? (Cluster *)pParent : NULL) : NULL,
															    zoneId, false, false);
								   if (object != NULL)
								   {
									   ((Node *)object)->setPrimaryName(nodePrimaryName);
								   }
								   break;
							   case OBJECT_MOBILEDEVICE:
								   pRequest->GetVariableStr(VID_DEVICE_ID, deviceId, MAX_OBJECT_NAME);
								   object = new MobileDevice(szObjectName, deviceId);
								   NetObjInsert(object, TRUE);
								   break;
							   case OBJECT_CONTAINER:
								   object = new Container(szObjectName, pRequest->GetVariableLong(VID_CATEGORY));
								   NetObjInsert(object, TRUE);
								   object->calculateCompoundStatus();	// Force status change to NORMAL
								   break;
							   case OBJECT_RACK:
								   object = new Rack(szObjectName, (int)pRequest->GetVariableShort(VID_HEIGHT));
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
								   iServiceType = (int)pRequest->GetVariableShort(VID_SERVICE_TYPE);
								   wIpProto = pRequest->GetVariableShort(VID_IP_PROTO);
								   wIpPort = pRequest->GetVariableShort(VID_IP_PORT);
								   pszRequest = pRequest->GetVariableStr(VID_SERVICE_REQUEST);
								   pszResponse = pRequest->GetVariableStr(VID_SERVICE_RESPONSE);
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
								   object = new NetworkMap((int)pRequest->GetVariableShort(VID_MAP_TYPE), pRequest->GetVariableLong(VID_SEED_OBJECT));
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
								   nodeId = pRequest->GetVariableLong(VID_NODE_ID);
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
								   object = new SlmCheck(szObjectName, pRequest->GetVariableShort(VID_IS_TEMPLATE) ? true : false);
								   NetObjInsert(object, TRUE);
								   break;
							   case OBJECT_INTERFACE:
								   pRequest->GetVariableBinary(VID_MAC_ADDR, macAddr, MAC_ADDR_LENGTH);
								   object = ((Node *)pParent)->createNewInterface(pRequest->GetVariableLong(VID_IP_ADDRESS),
								                                                   pRequest->GetVariableLong(VID_IP_NETMASK),
								                                                   szObjectName, NULL,
																				               pRequest->GetVariableLong(VID_IF_INDEX),
								                                                   pRequest->GetVariableLong(VID_IF_TYPE),
																				               macAddr, 0,
																				               pRequest->GetVariableLong(VID_IF_SLOT),
																				               pRequest->GetVariableLong(VID_IF_PORT),
																				               pRequest->GetVariableShort(VID_IS_PHYS_PORT) ? true : false,
																								   true,
                                                                           false);
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
							   WriteAuditLog(AUDIT_OBJECTS, TRUE, m_dwUserId, m_workstation, object->Id(), _T("Object %s created"), object->Name());
							   if ((pParent != NULL) &&          // parent can be NULL for nodes
							       (iClass != OBJECT_INTERFACE)) // interface already linked by Node::createNewInterface
							   {
								   pParent->AddChild(object);
								   object->AddParent(pParent);
								   pParent->calculateCompoundStatus();
								   if (pParent->Type() == OBJECT_CLUSTER)
								   {
									   ((Cluster *)pParent)->applyToTarget((DataCollectionTarget *)object);
								   }
								   if (object->Type() == OBJECT_NODELINK)
								   {
									   ((NodeLink *)object)->applyTemplates();
								   }
								   if (object->Type() == OBJECT_NETWORKSERVICE)
								   {
									   if (pRequest->GetVariableShort(VID_CREATE_STATUS_DCI))
									   {
										   TCHAR dciName[MAX_DB_STRING], dciDescription[MAX_DB_STRING];

										   _sntprintf(dciName, MAX_DB_STRING, _T("ChildStatus(%d)"), object->Id());
										   _sntprintf(dciDescription, MAX_DB_STRING, _T("Status of network service %s"), object->Name());
										   ((Node *)pParent)->addDCObject(new DCItem(CreateUniqueId(IDG_ITEM), dciName, DS_INTERNAL, DCI_DT_INT,
											   ConfigReadInt(_T("DefaultDCIPollingInterval"), 60),
											   ConfigReadInt(_T("DefaultDCIRetentionTime"), 30), (Node *)pParent, dciDescription));
									   }
								   }
							   }

							   pszComments = pRequest->GetVariableStr(VID_COMMENTS);
							   if (pszComments != NULL)
								   object->setComments(pszComments);

							   object->unhide();
							   msg.SetVariable(VID_RCC, RCC_SUCCESS);
							   msg.SetVariable(VID_OBJECT_ID, object->Id());
						   }
						   else
						   {
							   // :DIRTY HACK:
							   // PollNewNode will return NULL only if IP already
							   // in use. some new() can fail there too, but server will
							   // crash in that case
							   if (iClass == OBJECT_NODE)
							   {
                  		   msg.SetVariable(VID_RCC, RCC_ALREADY_EXIST);
							   }
							   else if (iClass == OBJECT_ZONE)
							   {
                  		   msg.SetVariable(VID_RCC, RCC_ZONE_ID_ALREADY_IN_USE);
							   }
							   else
							   {
                  		   msg.SetVariable(VID_RCC, RCC_OBJECT_CREATION_FAILED);
							   }
						   }

                     ObjectTransactionEnd();
                  }
                  else
                  {
   						msg.SetVariable(VID_RCC, moduleRCC);
                  }
					}
					else
					{
						msg.SetVariable(VID_RCC, RCC_INVALID_OBJECT_NAME);
					}
				}
				else
				{
					msg.SetVariable(VID_RCC, RCC_INVALID_ZONE_ID);
				}
         }
         else
         {
            msg.SetVariable(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
         }
      }
      else
      {
         msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
      }
   }
   else
   {
      msg.SetVariable(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Add cluster node
 */
void ClientSession::addClusterNode(CSCPMessage *request)
{
   CSCPMessage msg;
	NetObj *cluster, *node;

	msg.SetId(request->GetId());
	msg.SetCode(CMD_REQUEST_COMPLETED);

   cluster = FindObjectById(request->GetVariableLong(VID_PARENT_ID));
   node = FindObjectById(request->GetVariableLong(VID_CHILD_ID));
	if ((cluster != NULL) && (node != NULL))
	{
		if ((cluster->Type() == OBJECT_CLUSTER) && (node->Type() == OBJECT_NODE))
		{
			if (((Node *)node)->getMyCluster() == NULL)
			{
				if (cluster->checkAccessRights(m_dwUserId, OBJECT_ACCESS_MODIFY) &&
					 node->checkAccessRights(m_dwUserId, OBJECT_ACCESS_MODIFY))
				{
					((Cluster *)cluster)->applyToTarget((Node *)node);
					((Node *)node)->setRecheckCapsFlag();
					((Node *)node)->forceConfigurationPoll();

					msg.SetVariable(VID_RCC, RCC_SUCCESS);
					WriteAuditLog(AUDIT_OBJECTS, TRUE, m_dwUserId, m_workstation, cluster->Id(),
									  _T("Node %s [%d] added to cluster %s [%d]"),
									  node->Name(), node->Id(), cluster->Name(), cluster->Id());
				}
				else
				{
					msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
					WriteAuditLog(AUDIT_OBJECTS, FALSE, m_dwUserId, m_workstation, cluster->Id(),
									  _T("Access denied on adding node %s [%d] to cluster %s [%d]"),
									  node->Name(), node->Id(), cluster->Name(), cluster->Id());
				}
			}
			else
			{
				msg.SetVariable(VID_RCC, RCC_CLUSTER_MEMBER_ALREADY);
			}
		}
		else
		{
			msg.SetVariable(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
		}
	}
	else
	{
		msg.SetVariable(VID_RCC, RCC_INVALID_OBJECT_ID);
	}

   // Send response
   sendMessage(&msg);
}

/**
 * Bind/unbind object
 */
void ClientSession::changeObjectBinding(CSCPMessage *pRequest, BOOL bBind)
{
   CSCPMessage msg;
   NetObj *pParent, *pChild;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   // Get parent and child objects
   pParent = FindObjectById(pRequest->GetVariableLong(VID_PARENT_ID));
   pChild = FindObjectById(pRequest->GetVariableLong(VID_CHILD_ID));

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
         if (IsValidParentClass(pChild->Type(), pParent->Type()))
         {
            if (bBind)
            {
               // Prevent loops
               if (!pChild->isChild(pParent->Id()))
               {
                  ObjectTransactionStart();
                  pParent->AddChild(pChild);
                  pChild->AddParent(pParent);
                  ObjectTransactionEnd();
                  pParent->calculateCompoundStatus();
                  msg.SetVariable(VID_RCC, RCC_SUCCESS);

						if ((pParent->Type() == OBJECT_BUSINESSSERVICEROOT) || (pParent->Type() == OBJECT_BUSINESSSERVICE))
						{
							((ServiceContainer *)pParent)->initUptimeStats();
						}
               }
               else
               {
                  msg.SetVariable(VID_RCC, RCC_OBJECT_LOOP);
               }
            }
            else
            {
               ObjectTransactionStart();
               pParent->DeleteChild(pChild);
               pChild->DeleteParent(pParent);
               ObjectTransactionEnd();
               pParent->calculateCompoundStatus();
               if ((pParent->Type() == OBJECT_TEMPLATE) &&
                   ((pChild->Type() == OBJECT_NODE) || (pChild->Type() == OBJECT_CLUSTER) || (pChild->Type() == OBJECT_MOBILEDEVICE)))
               {
                  ((Template *)pParent)->queueRemoveFromTarget(pChild->Id(), pRequest->GetVariableShort(VID_REMOVE_DCI));
               }
               else if ((pParent->Type() == OBJECT_CLUSTER) &&
                        (pChild->Type() == OBJECT_NODE))
               {
                  ((Cluster *)pParent)->queueRemoveFromTarget(pChild->Id(), TRUE);
						((Node *)pChild)->setRecheckCapsFlag();
						((Node *)pChild)->forceConfigurationPoll();
               }
					else if ((pParent->Type() == OBJECT_BUSINESSSERVICEROOT) || (pParent->Type() == OBJECT_BUSINESSSERVICE))
					{
						((ServiceContainer *)pParent)->initUptimeStats();
					}
               msg.SetVariable(VID_RCC, RCC_SUCCESS);
            }
         }
         else
         {
            msg.SetVariable(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
         }
      }
      else
      {
         msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
      }
   }
   else
   {
      msg.SetVariable(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Worker thread for object deletion
 */
static THREAD_RESULT THREAD_CALL DeleteObjectWorker(void *arg)
{
   ObjectTransactionStart();
	((NetObj *)arg)->deleteObject();
   ObjectTransactionEnd();
	return THREAD_OK;
}

/**
 * Delete object
 */
void ClientSession::deleteObject(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   NetObj *object;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   // Find object to be deleted
   object = FindObjectById(pRequest->GetVariableLong(VID_OBJECT_ID));
   if (object != NULL)
   {
      // Check if it is a built-in object, like "Entire Network"
      if (object->Id() >= 10)  // FIXME: change to 100
      {
         // Check access rights
         if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_DELETE))
         {
				if ((object->Type() != OBJECT_ZONE) || object->isEmpty())
				{
					ThreadCreate(DeleteObjectWorker, 0, object);
					msg.SetVariable(VID_RCC, RCC_SUCCESS);
               WriteAuditLog(AUDIT_OBJECTS, TRUE, m_dwUserId, m_workstation, object->Id(), _T("Object %s deleted"), object->Name());
				}
				else
				{
	            msg.SetVariable(VID_RCC, RCC_ZONE_NOT_EMPTY);
				}
         }
         else
         {
            msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
            WriteAuditLog(AUDIT_OBJECTS, FALSE, m_dwUserId, m_workstation, object->Id(), _T("Access denied on delete object %s"), object->Name());
         }
      }
      else
      {
         msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
         WriteAuditLog(AUDIT_OBJECTS, FALSE, m_dwUserId, m_workstation, object->Id(), _T("Access denied on delete object %s"), object->Name());
      }
   }
   else
   {
      msg.SetVariable(VID_RCC, RCC_INVALID_OBJECT_ID);
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
            m_pUpdateQueue->Put(pUpdate);
         }
   }
}

/**
 * Send all alarms to client
 */
void ClientSession::sendAllAlarms(UINT32 dwRqId)
{
   MutexLock(m_mutexSendAlarms);
   g_alarmMgr.sendAlarmsToClient(dwRqId, this);
   MutexUnlock(m_mutexSendAlarms);
}

/**
 * Get specific alarm object
 */
void ClientSession::getAlarm(CSCPMessage *request)
{
   CSCPMessage msg;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(request->GetId());

   // Get alarm id and it's source object
   UINT32 alarmId = request->GetVariableLong(VID_ALARM_ID);
   NetObj *object = g_alarmMgr.getAlarmSourceObject(alarmId);
   if (object != NULL)
   {
      // User should have "view alarm" right to the object
      if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ_ALARMS))
      {
         msg.SetVariable(VID_RCC, g_alarmMgr.getAlarm(alarmId, &msg));
      }
      else
      {
         msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
			WriteAuditLog(AUDIT_OBJECTS, FALSE, m_dwUserId, m_workstation, object->Id(), _T("Access denied on get alarm for object %s"), object->Name());
      }
   }
   else
   {
      // Normally, for existing alarms object will not be NULL,
      // so we assume that alarm id is invalid
      msg.SetVariable(VID_RCC, RCC_INVALID_ALARM_ID);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Get all related events for specific alarm object
 */
void ClientSession::getAlarmEvents(CSCPMessage *request)
{
   CSCPMessage msg;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(request->GetId());

   // Get alarm id and it's source object
   UINT32 alarmId = request->GetVariableLong(VID_ALARM_ID);
   NetObj *object = g_alarmMgr.getAlarmSourceObject(alarmId);
   if (object != NULL)
   {
      // User should have "view alarm" right to the object and
		// system-wide "view event log" access
      if ((m_dwSystemAccess & SYSTEM_ACCESS_VIEW_EVENT_LOG) && object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ_ALARMS))
      {
         msg.SetVariable(VID_RCC, g_alarmMgr.getAlarmEvents(alarmId, &msg));
      }
      else
      {
         msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
			WriteAuditLog(AUDIT_OBJECTS, FALSE, m_dwUserId, m_workstation, object->Id(), _T("Access denied on get alarm events for object %s"), object->Name());
      }
   }
   else
   {
      // Normally, for existing alarms object will not be NULL,
      // so we assume that alarm id is invalid
      msg.SetVariable(VID_RCC, RCC_INVALID_ALARM_ID);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Acknowledge alarm
 */
void ClientSession::acknowledgeAlarm(CSCPMessage *pRequest)
{
   CSCPMessage msg;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   // Get alarm id and it's source object
   UINT32 alarmId;
   TCHAR hdref[MAX_HELPDESK_REF_LEN];
   NetObj *object;
   bool byHelpdeskRef;
   if (pRequest->isFieldExist(VID_HELPDESK_REF))
   {
      pRequest->GetVariableStr(VID_HELPDESK_REF, hdref, MAX_HELPDESK_REF_LEN);
      object = g_alarmMgr.getAlarmSourceObject(hdref);
      byHelpdeskRef = true;
   }
   else
   {
      alarmId = pRequest->GetVariableLong(VID_ALARM_ID);
      object = g_alarmMgr.getAlarmSourceObject(alarmId);
      byHelpdeskRef = false;
   }
   if (object != NULL)
   {
      // User should have "acknowledge alarm" right to the object
      if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_UPDATE_ALARMS))
      {
			msg.SetVariable(VID_RCC,
            byHelpdeskRef ?
            g_alarmMgr.ackByHDRef(hdref, this, pRequest->GetVariableShort(VID_STICKY_FLAG) != 0, pRequest->GetVariableLong(VID_TIMESTAMP)) :
            g_alarmMgr.ackById(alarmId, this, pRequest->GetVariableShort(VID_STICKY_FLAG) != 0, pRequest->GetVariableLong(VID_TIMESTAMP)));
      }
      else
      {
         msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
			WriteAuditLog(AUDIT_OBJECTS, FALSE, m_dwUserId, m_workstation, object->Id(), _T("Access denied on acknowledged alarm on object %s"), object->Name());
      }
   }
   else
   {
      // Normally, for existing alarms object will not be NULL,
      // so we assume that alarm id is invalid
      msg.SetVariable(VID_RCC, RCC_INVALID_ALARM_ID);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Resolve/Terminate alarm
 */
void ClientSession::resolveAlarm(CSCPMessage *pRequest, bool terminate)
{
   CSCPMessage msg;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   // Get alarm id and it's source object
   UINT32 alarmId;
   TCHAR hdref[MAX_HELPDESK_REF_LEN];
   NetObj *object;
   bool byHelpdeskRef;
   if (pRequest->isFieldExist(VID_HELPDESK_REF))
   {
      pRequest->GetVariableStr(VID_HELPDESK_REF, hdref, MAX_HELPDESK_REF_LEN);
      object = g_alarmMgr.getAlarmSourceObject(hdref);
      byHelpdeskRef = true;
   }
   else
   {
      alarmId = pRequest->GetVariableLong(VID_ALARM_ID);
      object = g_alarmMgr.getAlarmSourceObject(alarmId);
      byHelpdeskRef = false;
   }
   if (object != NULL)
   {
      // User should have "terminate alarm" right to the object
      if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_TERM_ALARMS))
      {
         msg.SetVariable(VID_RCC,
            byHelpdeskRef ?
            g_alarmMgr.resolveByHDRef(hdref, this, terminate) :
            g_alarmMgr.resolveById(alarmId, this, terminate));
      }
      else
      {
         msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
			WriteAuditLog(AUDIT_OBJECTS, FALSE, m_dwUserId, m_workstation, object->Id(),
            _T("Access denied on %s alarm on object %s"), terminate ? _T("terminate") : _T("resolve"), object->Name());
      }
   }
   else
   {
      // Normally, for existing alarms object will not be NULL,
      // so we assume that alarm id is invalid
      msg.SetVariable(VID_RCC, RCC_INVALID_ALARM_ID);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Delete alarm
 */
void ClientSession::deleteAlarm(CSCPMessage *pRequest)
{
   CSCPMessage msg;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   // Get alarm id and it's source object
   UINT32 alarmId = pRequest->GetVariableLong(VID_ALARM_ID);
   NetObj *object = g_alarmMgr.getAlarmSourceObject(alarmId);
   if (object != NULL)
   {
      // User should have "terminate alarm" right to the object
      // and system right "delete alarms"
      if ((object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_TERM_ALARMS)) &&
          (m_dwSystemAccess & SYSTEM_ACCESS_DELETE_ALARMS))
      {
         g_alarmMgr.deleteAlarm(alarmId, false);
         msg.SetVariable(VID_RCC, RCC_SUCCESS);
      }
      else
      {
         msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
			WriteAuditLog(AUDIT_OBJECTS, FALSE, m_dwUserId, m_workstation, object->Id(), _T("Access denied on delete alarm on object %s"), object->Name());
      }
   }
   else
   {
      // Normally, for existing alarms object will not be NULL,
      // so we assume that alarm id is invalid
      msg.SetVariable(VID_RCC, RCC_INVALID_ALARM_ID);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Open issue in helpdesk system from given alarm
 */
void ClientSession::openHelpdeskIssue(CSCPMessage *request)
{
   CSCPMessage msg;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(request->GetId());

   // Get alarm id and it's source object
   UINT32 alarmId = request->GetVariableLong(VID_ALARM_ID);
   NetObj *object = g_alarmMgr.getAlarmSourceObject(alarmId);
   if (object != NULL)
   {
      if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_CREATE_ISSUE))
      {
         TCHAR hdref[MAX_HELPDESK_REF_LEN];
         msg.SetVariable(VID_RCC, g_alarmMgr.openHelpdeskIssue(alarmId, this, hdref));
         msg.SetVariable(VID_HELPDESK_REF, hdref);
      }
      else
      {
         msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
			WriteAuditLog(AUDIT_OBJECTS, FALSE, m_dwUserId, m_workstation, object->Id(),
            _T("Access denied on creating issue from alarm on object %s"), object->Name());
      }
   }
   else
   {
      // Normally, for existing alarms object will not be NULL,
      // so we assume that alarm id is invalid
      msg.SetVariable(VID_RCC, RCC_INVALID_ALARM_ID);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Get helpdesk URL for given alarm
 */
void ClientSession::getHelpdeskUrl(CSCPMessage *request)
{
   CSCPMessage msg;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(request->GetId());

   // Get alarm id and it's source object
   UINT32 alarmId = request->GetVariableLong(VID_ALARM_ID);
   NetObj *object = g_alarmMgr.getAlarmSourceObject(alarmId);
   if (object != NULL)
   {
      if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ_ALARMS))
      {
         TCHAR url[MAX_PATH];
         msg.SetVariable(VID_RCC, g_alarmMgr.getHelpdeskIssueUrl(alarmId, url, MAX_PATH));
         msg.SetVariable(VID_URL, url);
      }
      else
      {
         msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
			WriteAuditLog(AUDIT_OBJECTS, FALSE, m_dwUserId, m_workstation, object->Id(),
            _T("Access denied on getting helpdesk URL for alarm on object %s"), object->Name());
      }
   }
   else
   {
      // Normally, for existing alarms object will not be NULL,
      // so we assume that alarm id is invalid
      msg.SetVariable(VID_RCC, RCC_INVALID_ALARM_ID);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Unlink helpdesk issue from alarm
 */
void ClientSession::unlinkHelpdeskIssue(CSCPMessage *request)
{
   CSCPMessage msg;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(request->GetId());

   // Get alarm id and it's source object
   UINT32 alarmId;
   TCHAR hdref[MAX_HELPDESK_REF_LEN];
   NetObj *object;
   bool byHelpdeskRef;
   if (request->isFieldExist(VID_HELPDESK_REF))
   {
      request->GetVariableStr(VID_HELPDESK_REF, hdref, MAX_HELPDESK_REF_LEN);
      object = g_alarmMgr.getAlarmSourceObject(hdref);
      byHelpdeskRef = true;
   }
   else
   {
      alarmId = request->GetVariableLong(VID_ALARM_ID);
      object = g_alarmMgr.getAlarmSourceObject(alarmId);
      byHelpdeskRef = false;
   }
   if (object != NULL)
   {
      // User should have "update alarms" right to the object and "unlink issues" global right
      if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_UPDATE_ALARMS) && checkSysAccessRights(SYSTEM_ACCESS_UNLINK_ISSUES))
      {
         msg.SetVariable(VID_RCC,
            byHelpdeskRef ?
            g_alarmMgr.unlinkIssueByHDRef(hdref, this) :
            g_alarmMgr.unlinkIssueById(alarmId, this));
      }
      else
      {
         msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
			WriteAuditLog(AUDIT_OBJECTS, FALSE, m_dwUserId, m_workstation, object->Id(),
            _T("Access denied on unlinking helpdesk issue from alarm on object %s"), object->Name());
      }
   }
   else
   {
      // Normally, for existing alarms object will not be NULL,
      // so we assume that alarm id is invalid
      msg.SetVariable(VID_RCC, RCC_INVALID_ALARM_ID);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Get comments for given alarm
 */
void ClientSession::getAlarmComments(CSCPMessage *request)
{
   CSCPMessage msg;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(request->GetId());

   // Get alarm id and it's source object
   UINT32 alarmId = request->GetVariableLong(VID_ALARM_ID);
   NetObj *object = g_alarmMgr.getAlarmSourceObject(alarmId);
   if (object != NULL)
   {
      // User should have "view alarms" right to the object
		if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ_ALARMS))
      {
			msg.SetVariable(VID_RCC, g_alarmMgr.getAlarmComments(alarmId, &msg));
      }
      else
      {
         msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
      }
   }
   else
   {
      // Normally, for existing alarms object will not be NULL,
      // so we assume that alarm id is invalid
      msg.SetVariable(VID_RCC, RCC_INVALID_ALARM_ID);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Update alarm comment
 */
void ClientSession::updateAlarmComment(CSCPMessage *request)
{
   CSCPMessage msg;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(request->GetId());

   // Get alarm id and it's source object
   UINT32 alarmId;
   TCHAR hdref[MAX_HELPDESK_REF_LEN];
   NetObj *object;
   bool byHelpdeskRef;
   if (request->isFieldExist(VID_HELPDESK_REF))
   {
      request->GetVariableStr(VID_HELPDESK_REF, hdref, MAX_HELPDESK_REF_LEN);
      object = g_alarmMgr.getAlarmSourceObject(hdref);
      byHelpdeskRef = true;
   }
   else
   {
      alarmId = request->GetVariableLong(VID_ALARM_ID);
      object = g_alarmMgr.getAlarmSourceObject(alarmId);
      byHelpdeskRef = false;
   }
   if (object != NULL)
   {
      // User should have "update alarm" right to the object
		if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_UPDATE_ALARMS))
      {
			UINT32 commentId = request->GetVariableLong(VID_COMMENT_ID);
			TCHAR *text = request->GetVariableStr(VID_COMMENTS);
			msg.SetVariable(VID_RCC,
            byHelpdeskRef ?
            g_alarmMgr.addAlarmComment(hdref, CHECK_NULL(text), m_dwUserId) :
            g_alarmMgr.updateAlarmComment(alarmId, commentId, CHECK_NULL(text), m_dwUserId));
			safe_free(text);
      }
      else
      {
         msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
      }
   }
   else
   {
      // Normally, for existing alarms object will not be NULL,
      // so we assume that alarm id is invalid
      msg.SetVariable(VID_RCC, RCC_INVALID_ALARM_ID);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Delete alarm comment
 */
void ClientSession::deleteAlarmComment(CSCPMessage *request)
{
   CSCPMessage msg;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(request->GetId());

   // Get alarm id and it's source object
   UINT32 alarmId = request->GetVariableLong(VID_ALARM_ID);
   NetObj *object = g_alarmMgr.getAlarmSourceObject(alarmId);
   if (object != NULL)
   {
      // User should have "acknowledge alarm" right to the object
		if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_UPDATE_ALARMS))
      {
			UINT32 commentId = request->GetVariableLong(VID_COMMENT_ID);
			msg.SetVariable(VID_RCC, g_alarmMgr.deleteAlarmCommentByID(alarmId, commentId));
      }
      else
      {
         msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
      }
   }
   else
   {
      // Normally, for existing alarms object will not be NULL,
      // so we assume that alarm id is invalid
      msg.SetVariable(VID_RCC, RCC_INVALID_ALARM_ID);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Update alarm status flow mode
 */
void ClientSession::updateAlarmStatusFlow(CSCPMessage *request)
{
   CSCPMessage msg;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(request->GetId());
   int status = request->GetVariableLong(VID_ALARM_STATUS_FLOW_STATE);

   ConfigWriteInt(_T("StrictAlarmStatusFlow"), status, false);
   msg.SetVariable(VID_RCC, RCC_SUCCESS);

   // Send response
   sendMessage(&msg);
}

/**
 * Create new server action
 */
void ClientSession::createAction(CSCPMessage *pRequest)
{
   CSCPMessage msg;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   // Check user rights
   if (m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_ACTIONS)
   {
      UINT32 dwResult, dwActionId;
      TCHAR szActionName[MAX_OBJECT_NAME];

      pRequest->GetVariableStr(VID_ACTION_NAME, szActionName, MAX_OBJECT_NAME);
      if (IsValidObjectName(szActionName, TRUE))
      {
         dwResult = CreateNewAction(szActionName, &dwActionId);
         msg.SetVariable(VID_RCC, dwResult);
         if (dwResult == RCC_SUCCESS)
            msg.SetVariable(VID_ACTION_ID, dwActionId);   // Send id of new action to client
      }
      else
      {
         msg.SetVariable(VID_RCC, RCC_INVALID_OBJECT_NAME);
      }
   }
   else
   {
      msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Update existing action's data
 */
void ClientSession::updateAction(CSCPMessage *pRequest)
{
   CSCPMessage msg;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   // Check user rights
   if (m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_ACTIONS)
   {
      msg.SetVariable(VID_RCC, ModifyActionFromMessage(pRequest));
   }
   else
   {
      msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Delete action
 */
void ClientSession::deleteAction(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   UINT32 dwActionId;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   // Check user rights
   if (m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_ACTIONS)
   {
      // Get Id of action to be deleted
      dwActionId = pRequest->GetVariableLong(VID_ACTION_ID);
      if (!g_pEventPolicy->isActionInUse(dwActionId))
      {
         msg.SetVariable(VID_RCC, DeleteActionFromDB(dwActionId));
      }
      else
      {
         msg.SetVariable(VID_RCC, RCC_ACTION_IN_USE);
      }
   }
   else
   {
      msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
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
         m_pUpdateQueue->Put(pUpdate);
      }
   }
}

/**
 * Send all actions to client
 */
void ClientSession::sendAllActions(UINT32 dwRqId)
{
   CSCPMessage msg;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(dwRqId);

   // Check user rights
   if ((m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_ACTIONS) ||
       (m_dwSystemAccess & SYSTEM_ACCESS_EPP))
   {
      msg.SetVariable(VID_RCC, RCC_SUCCESS);
      sendMessage(&msg);
      MutexLock(m_mutexSendActions);
      SendActionsToClient(this, dwRqId);
      MutexUnlock(m_mutexSendActions);
   }
   else
   {
      msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
      sendMessage(&msg);
   }
}


//
// Send list of configured container categories to client
//

void ClientSession::SendContainerCategories(UINT32 dwRqId)
{
   CSCPMessage msg;
   UINT32 i;

   // Prepare response message
   msg.SetCode(CMD_CONTAINER_CAT_DATA);
   msg.SetId(dwRqId);

   for(i = 0; i < g_dwNumCategories; i++)
   {
      msg.SetVariable(VID_CATEGORY_ID, g_pContainerCatList[i].dwCatId);
      msg.SetVariable(VID_CATEGORY_NAME, g_pContainerCatList[i].szName);
      //msg.SetVariable(VID_IMAGE_ID, g_pContainerCatList[i].dwImageId);
      msg.SetVariable(VID_DESCRIPTION, g_pContainerCatList[i].pszDescription);
      sendMessage(&msg);
      msg.deleteAllVariables();
   }

   // Send end-of-list indicator
   msg.SetVariable(VID_CATEGORY_ID, (UINT32)0);
   sendMessage(&msg);
}

/**
 * Perform a forced node poll
 */
void ClientSession::forcedNodePoll(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   POLLER_START_DATA *pData;
   NetObj *object;

   pData = (POLLER_START_DATA *)malloc(sizeof(POLLER_START_DATA));
   pData->pSession = this;
   MutexLock(m_mutexPollerInit);

   // Prepare response message
   pData->dwRqId = pRequest->GetId();
   msg.SetCode(CMD_POLLING_INFO);
   msg.SetId(pData->dwRqId);

   // Get polling type
   pData->iPollType = pRequest->GetVariableShort(VID_POLL_TYPE);

   // Find object to be polled
   object = FindObjectById(pRequest->GetVariableLong(VID_OBJECT_ID));
   if (object != NULL)
   {
      // We can do polls only for node objects
      if ((object->Type() == OBJECT_NODE) &&
          ((pData->iPollType == POLL_STATUS) ||
			  (pData->iPollType == POLL_CONFIGURATION) ||
			  (pData->iPollType == POLL_TOPOLOGY) ||
			  (pData->iPollType == POLL_INTERFACE_NAMES)))
      {
         // Check access rights
         if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_MODIFY))
         {
            ((Node *)object)->incRefCount();
            m_dwRefCount++;

            pData->pNode = (Node *)object;
            ThreadCreate(pollerThreadStarter, 0, pData);
            msg.SetVariable(VID_RCC, RCC_OPERATION_IN_PROGRESS);
            msg.SetVariable(VID_POLLER_MESSAGE, _T("Poll request accepted\r\n"));
				pData = NULL;
         }
         else
         {
            msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
         }
      }
      else
      {
         msg.SetVariable(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
      }
   }
   else
   {
      msg.SetVariable(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   // Send response
   sendMessage(&msg);
   MutexUnlock(m_mutexPollerInit);
	safe_free(pData);
}


//
// Send message from poller to client
//

void ClientSession::sendPollerMsg(UINT32 dwRqId, const TCHAR *pszMsg)
{
   CSCPMessage msg;

   msg.SetCode(CMD_POLLING_INFO);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_RCC, RCC_OPERATION_IN_PROGRESS);
   msg.SetVariable(VID_POLLER_MESSAGE, pszMsg);
   sendMessage(&msg);
}


//
// Node poller thread
//

void ClientSession::pollerThread(Node *pNode, int iPollType, UINT32 dwRqId)
{
   CSCPMessage msg;

   // Wait while parent thread finishes initialization
   MutexLock(m_mutexPollerInit);
   MutexUnlock(m_mutexPollerInit);

   switch(iPollType)
   {
      case POLL_STATUS:
         pNode->statusPoll(this, dwRqId, -1);
         break;
      case POLL_CONFIGURATION:
			pNode->setRecheckCapsFlag();
         pNode->configurationPoll(this, dwRqId, -1, 0);
         break;
      case POLL_TOPOLOGY:
         pNode->topologyPoll(this, dwRqId, -1);
         break;
      case POLL_INTERFACE_NAMES:
         pNode->updateInterfaceNames(this, dwRqId);
         break;
      default:
         sendPollerMsg(dwRqId, _T("Invalid poll type requested\r\n"));
         break;
   }
   pNode->decRefCount();

   msg.SetCode(CMD_POLLING_INFO);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_RCC, RCC_SUCCESS);
   sendMessage(&msg);
}

/**
 * Receive event from user
 */
void ClientSession::onTrap(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   UINT32 dwObjectId, dwEventCode;
   int i, iNumArgs;
   NetObj *object;
   TCHAR *pszArgList[32], szUserTag[MAX_USERTAG_LENGTH] = _T("");
   char szFormat[] = "ssssssssssssssssssssssssssssssss";
   BOOL bSuccess;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   // Find event's source object
   dwObjectId = pRequest->GetVariableLong(VID_OBJECT_ID);
   if (dwObjectId != 0)
	{
      object = FindObjectById(dwObjectId);  // Object is specified explicitely
	}
   else   // Client is the source
	{
		if (m_clientAddr->sa_family == AF_INET)
		{
			UINT32 addr = ntohl(((struct sockaddr_in *)m_clientAddr)->sin_addr.s_addr);
			if (addr == 0x7F000001)
				object = FindObjectById(g_dwMgmtNode);	// Client on loopback
			else
				object = FindNodeByIP(0, addr);
		}
		else
		{
#ifdef WITH_IPV6
			object = NULL;	// TODO: find object by IPv6 address
#else
			object = NULL;
#endif
		}
	}
   if (object != NULL)
   {
      // User should have SEND_EVENTS access right to object
      if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_SEND_EVENTS))
      {
         dwEventCode = pRequest->GetVariableLong(VID_EVENT_CODE);
			if ((dwEventCode == 0) && pRequest->isFieldExist(VID_EVENT_NAME))
			{
				TCHAR eventName[256];
				pRequest->GetVariableStr(VID_EVENT_NAME, eventName, 256);
				dwEventCode = EventCodeFromName(eventName, 0);
			}
			pRequest->GetVariableStr(VID_USER_TAG, szUserTag, MAX_USERTAG_LENGTH);
         iNumArgs = pRequest->GetVariableShort(VID_NUM_ARGS);
         if (iNumArgs > 32)
            iNumArgs = 32;
         for(i = 0; i < iNumArgs; i++)
            pszArgList[i] = pRequest->GetVariableStr(VID_EVENT_ARG_BASE + i);

         szFormat[iNumArgs] = 0;
         bSuccess = PostEventWithTag(dwEventCode, object->Id(), szUserTag, (iNumArgs > 0) ? szFormat : NULL,
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

         msg.SetVariable(VID_RCC, bSuccess ? RCC_SUCCESS : RCC_INVALID_EVENT_CODE);
      }
      else
      {
         msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
      }
   }
   else
   {
      msg.SetVariable(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Wake up node
 */
void ClientSession::onWakeUpNode(CSCPMessage *pRequest)
{
   CSCPMessage msg;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   // Find node or interface object
   NetObj *object = FindObjectById(pRequest->GetVariableLong(VID_OBJECT_ID));
   if (object != NULL)
   {
      if ((object->Type() == OBJECT_NODE) ||
          (object->Type() == OBJECT_INTERFACE))
      {
         if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_CONTROL))
         {
            UINT32 dwResult;

            if (object->Type() == OBJECT_NODE)
               dwResult = ((Node *)object)->wakeUp();
            else
               dwResult = ((Interface *)object)->wakeUp();
            msg.SetVariable(VID_RCC, dwResult);
         }
         else
         {
            msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
         }
      }
      else
      {
         msg.SetVariable(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
      }
   }
   else
   {
      msg.SetVariable(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Query specific parameter from node
 */
void ClientSession::queryParameter(CSCPMessage *pRequest)
{
   CSCPMessage msg;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   // Find node object
   NetObj *object = FindObjectById(pRequest->GetVariableLong(VID_OBJECT_ID));
   if (object != NULL)
   {
      if (object->Type() == OBJECT_NODE)
      {
         if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
         {
            UINT32 dwResult;
            TCHAR szBuffer[256], szName[MAX_PARAM_NAME];

            pRequest->GetVariableStr(VID_NAME, szName, MAX_PARAM_NAME);
            dwResult = ((Node *)object)->getItemForClient(pRequest->GetVariableShort(VID_DCI_SOURCE_TYPE),
                                                           szName, szBuffer, 256);
            msg.SetVariable(VID_RCC, dwResult);
            if (dwResult == RCC_SUCCESS)
               msg.SetVariable(VID_VALUE, szBuffer);
         }
         else
         {
            msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
         }
      }
      else
      {
         msg.SetVariable(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
      }
   }
   else
   {
      msg.SetVariable(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Query specific table from node
 */
void ClientSession::queryAgentTable(CSCPMessage *pRequest)
{
   CSCPMessage msg;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   // Find node object
   NetObj *object = FindObjectById(pRequest->GetVariableLong(VID_OBJECT_ID));
   if (object != NULL)
   {
      if (object->Type() == OBJECT_NODE)
      {
         if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
         {
				TCHAR name[MAX_PARAM_NAME];
				pRequest->GetVariableStr(VID_NAME, name, MAX_PARAM_NAME);

				Table *table;
				UINT32 rcc = ((Node *)object)->getTableForClient(name, &table);
				msg.SetVariable(VID_RCC, rcc);
				if (rcc == RCC_SUCCESS)
				{
					table->fillMessage(msg, 0, -1);
					delete table;
				}
         }
         else
         {
            msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
         }
      }
      else
      {
         msg.SetVariable(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
      }
   }
   else
   {
      msg.SetVariable(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Edit trap configuration record
 */
void ClientSession::editTrap(int iOperation, CSCPMessage *pRequest)
{
   CSCPMessage msg;
   UINT32 dwTrapId, dwResult;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   // Check access rights
   if (checkSysAccessRights(SYSTEM_ACCESS_CONFIGURE_TRAPS))
   {
      switch(iOperation)
      {
         case TRAP_CREATE:
            dwResult = CreateNewTrap(&dwTrapId);
            msg.SetVariable(VID_RCC, dwResult);
            if (dwResult == RCC_SUCCESS)
               msg.SetVariable(VID_TRAP_ID, dwTrapId);   // Send id of new trap to client
            break;
         case TRAP_UPDATE:
            msg.SetVariable(VID_RCC, UpdateTrapFromMsg(pRequest));
            break;
         case TRAP_DELETE:
            dwTrapId = pRequest->GetVariableLong(VID_TRAP_ID);
            msg.SetVariable(VID_RCC, DeleteTrap(dwTrapId));
            break;
         default:
				msg.SetVariable(VID_RCC, RCC_INVALID_ARGUMENT);
            break;
      }
   }
   else
   {
      // Current user has no rights for trap management
      msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Send all trap configuration records to client
 */
void ClientSession::sendAllTraps(UINT32 dwRqId)
{
   CSCPMessage msg;
   BOOL bSuccess = FALSE;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(dwRqId);

	if (checkSysAccessRights(SYSTEM_ACCESS_CONFIGURE_TRAPS))
   {
      msg.SetVariable(VID_RCC, RCC_SUCCESS);
      sendMessage(&msg);
      bSuccess = TRUE;
      SendTrapsToClient(this, dwRqId);
   }
   else
   {
      // Current user has no rights for trap management
      msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
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
   CSCPMessage msg;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(dwRqId);

	if (checkSysAccessRights(SYSTEM_ACCESS_CONFIGURE_TRAPS))
   {
      msg.SetVariable(VID_RCC, RCC_SUCCESS);
      CreateTrapCfgMessage(msg);
   }
   else
   {
      // Current user has no rights for trap management
      msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
   }

   sendMessage(&msg);
}


//
// Lock/unlock package database
//

void ClientSession::LockPackageDB(UINT32 dwRqId, BOOL bLock)
{
   CSCPMessage msg;
   TCHAR szBuffer[256];

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(dwRqId);

   if (m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_PACKAGES)
   {
      if (bLock)
      {
         if (!LockComponent(CID_PACKAGE_DB, m_dwIndex, m_szUserName, NULL, szBuffer))
         {
            msg.SetVariable(VID_RCC, RCC_COMPONENT_LOCKED);
            msg.SetVariable(VID_LOCKED_BY, szBuffer);
         }
         else
         {
            m_dwFlags |= CSF_PACKAGE_DB_LOCKED;
            msg.SetVariable(VID_RCC, RCC_SUCCESS);
         }
      }
      else
      {
         if (m_dwFlags & CSF_PACKAGE_DB_LOCKED)
         {
            UnlockComponent(CID_PACKAGE_DB);
            m_dwFlags &= ~CSF_PACKAGE_DB_LOCKED;
         }
         msg.SetVariable(VID_RCC, RCC_SUCCESS);
      }
   }
   else
   {
      // Current user has no rights for trap management
      msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Send list of all installed packages to client
 */
void ClientSession::SendAllPackages(UINT32 dwRqId)
{
   CSCPMessage msg;
   DB_ASYNC_RESULT hResult;
   BOOL bSuccess = FALSE;
   TCHAR szBuffer[MAX_DB_STRING];

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(dwRqId);

   if (m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_PACKAGES)
   {
      if (m_dwFlags & CSF_PACKAGE_DB_LOCKED)
      {
         DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
         hResult = DBAsyncSelect(hdb, _T("SELECT pkg_id,version,platform,pkg_file,pkg_name,description FROM agent_pkg"));
         if (hResult != NULL)
         {
            msg.SetVariable(VID_RCC, RCC_SUCCESS);
            sendMessage(&msg);
            bSuccess = TRUE;

            msg.SetCode(CMD_PACKAGE_INFO);
            msg.deleteAllVariables();

            while(DBFetch(hResult))
            {
               msg.SetVariable(VID_PACKAGE_ID, DBGetFieldAsyncULong(hResult, 0));
               msg.SetVariable(VID_PACKAGE_VERSION, DBGetFieldAsync(hResult, 1, szBuffer, MAX_DB_STRING));
               msg.SetVariable(VID_PLATFORM_NAME, DBGetFieldAsync(hResult, 2, szBuffer, MAX_DB_STRING));
               msg.SetVariable(VID_FILE_NAME, DBGetFieldAsync(hResult, 3, szBuffer, MAX_DB_STRING));
               msg.SetVariable(VID_PACKAGE_NAME, DBGetFieldAsync(hResult, 4, szBuffer, MAX_DB_STRING));
               DBGetFieldAsync(hResult, 5, szBuffer, MAX_DB_STRING);
               DecodeSQLString(szBuffer);
               msg.SetVariable(VID_DESCRIPTION, szBuffer);
               sendMessage(&msg);
               msg.deleteAllVariables();
            }

            msg.SetVariable(VID_PACKAGE_ID, (UINT32)0);
            sendMessage(&msg);

            DBFreeAsyncResult(hResult);
         }
         else
         {
            msg.SetVariable(VID_RCC, RCC_DB_FAILURE);
         }
         DBConnectionPoolReleaseConnection(hdb);
      }
      else
      {
         msg.SetVariable(VID_RCC, RCC_OUT_OF_STATE_REQUEST);
      }
   }
   else
   {
      // Current user has no rights for package management
      msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
   }

   // Send response
   if (!bSuccess)
      sendMessage(&msg);
}

/**
 * Install package to server
 */
void ClientSession::InstallPackage(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   TCHAR szPkgName[MAX_PACKAGE_NAME_LEN], szDescription[MAX_DB_STRING];
   TCHAR szPkgVersion[MAX_AGENT_VERSION_LEN], szFileName[MAX_DB_STRING];
   TCHAR szPlatform[MAX_PLATFORM_NAME_LEN], *pszEscDescr;
   TCHAR szQuery[2048];

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   if (m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_PACKAGES)
   {
      if (m_dwFlags & CSF_PACKAGE_DB_LOCKED)
      {
         pRequest->GetVariableStr(VID_PACKAGE_NAME, szPkgName, MAX_PACKAGE_NAME_LEN);
         pRequest->GetVariableStr(VID_DESCRIPTION, szDescription, MAX_DB_STRING);
         pRequest->GetVariableStr(VID_FILE_NAME, szFileName, MAX_DB_STRING);
         pRequest->GetVariableStr(VID_PACKAGE_VERSION, szPkgVersion, MAX_AGENT_VERSION_LEN);
         pRequest->GetVariableStr(VID_PLATFORM_NAME, szPlatform, MAX_PLATFORM_NAME_LEN);

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
                     _tcscpy(m_szCurrFileName, g_szDataDir);
                     _tcscat(m_szCurrFileName, DDIR_PACKAGES);
                     _tcscat(m_szCurrFileName, FS_PATH_SEPARATOR);
                     _tcscat(m_szCurrFileName, pszCleanFileName);
                     m_hCurrFile = _topen(m_szCurrFileName, O_CREAT | O_TRUNC | O_WRONLY | O_BINARY, S_IRUSR | S_IWUSR);
                     if (m_hCurrFile != -1)
                     {
                        m_dwFileRqId = pRequest->GetId();
                        m_dwUploadCommand = CMD_INSTALL_PACKAGE;
                        m_dwUploadData = CreateUniqueId(IDG_PACKAGE);
                        msg.SetVariable(VID_RCC, RCC_SUCCESS);
                        msg.SetVariable(VID_PACKAGE_ID, m_dwUploadData);

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
                        msg.SetVariable(VID_RCC, RCC_IO_ERROR);
                     }
                  }
                  else
                  {
                     msg.SetVariable(VID_RCC, RCC_RESOURCE_BUSY);
                  }
               }
               else
               {
                  msg.SetVariable(VID_RCC, RCC_PACKAGE_FILE_EXIST);
               }
            }
            else
            {
               msg.SetVariable(VID_RCC, RCC_DUPLICATE_PACKAGE);
            }
         }
         else
         {
            msg.SetVariable(VID_RCC, RCC_INVALID_OBJECT_NAME);
         }
      }
      else
      {
         msg.SetVariable(VID_RCC, RCC_OUT_OF_STATE_REQUEST);
      }
   }
   else
   {
      // Current user has no rights for package management
      msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
   }

   // Send response
   sendMessage(&msg);
}


//
// Remove package from server
//

void ClientSession::RemovePackage(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   UINT32 dwPkgId;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   if (m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_PACKAGES)
   {
      if (m_dwFlags & CSF_PACKAGE_DB_LOCKED)
      {
         dwPkgId = pRequest->GetVariableLong(VID_PACKAGE_ID);
         msg.SetVariable(VID_RCC, UninstallPackage(dwPkgId));
      }
      else
      {
         msg.SetVariable(VID_RCC, RCC_OUT_OF_STATE_REQUEST);
      }
   }
   else
   {
      // Current user has no rights for package management
      msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Get list of parameters supported by given node
 */
void ClientSession::getParametersList(CSCPMessage *pRequest)
{
   CSCPMessage msg;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   NetObj *object = FindObjectById(pRequest->GetVariableLong(VID_OBJECT_ID));
   if (object != NULL)
   {
      switch(object->Type())
      {
         case OBJECT_NODE:
            msg.SetVariable(VID_RCC, RCC_SUCCESS);
				((Node *)object)->writeParamListToMessage(&msg, pRequest->GetVariableShort(VID_FLAGS));
            break;
         case OBJECT_CLUSTER:
         case OBJECT_TEMPLATE:
            msg.SetVariable(VID_RCC, RCC_SUCCESS);
            WriteFullParamListToMessage(&msg, pRequest->GetVariableShort(VID_FLAGS));
            break;
         default:
            msg.SetVariable(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
            break;
      }
   }
   else
   {
      msg.SetVariable(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Deplay package to node(s)
 */
void ClientSession::DeployPackage(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   UINT32 i, dwNumObjects, *pdwObjectList, dwPkgId;
   NetObj *object;
   TCHAR szQuery[256], szPkgFile[MAX_PATH];
   TCHAR szVersion[MAX_AGENT_VERSION_LEN], szPlatform[MAX_PLATFORM_NAME_LEN];
   DB_RESULT hResult;
   BOOL bSuccess = TRUE;
   MUTEX hMutex;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   if (m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_PACKAGES)
   {
		ObjectArray<Node> *nodeList = NULL;

      // Get package ID
      dwPkgId = pRequest->GetVariableLong(VID_PACKAGE_ID);
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
               dwNumObjects = pRequest->GetVariableLong(VID_NUM_OBJECTS);
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
                        if (object->Type() == OBJECT_NODE)
                        {
                           // Check if this node already in the list
									int j;
                           for(j = 0; j < nodeList->size(); j++)
                              if (nodeList->get(j)->Id() == pdwObjectList[i])
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
                        msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
                        bSuccess = FALSE;
                        break;
                     }
                  }
                  else
                  {
                     msg.SetVariable(VID_RCC, RCC_INVALID_OBJECT_ID);
                     bSuccess = FALSE;
                     break;
                  }
               }
               safe_free(pdwObjectList);
            }
            else
            {
               msg.SetVariable(VID_RCC, RCC_DB_FAILURE);
               bSuccess = FALSE;
            }
            DBFreeResult(hResult);
         }
         else
         {
            msg.SetVariable(VID_RCC, RCC_DB_FAILURE);
            bSuccess = FALSE;
         }
      }
      else
      {
         msg.SetVariable(VID_RCC, RCC_INVALID_PACKAGE_ID);
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
         pInfo->dwRqId = pRequest->GetId();
         pInfo->dwPackageId = dwPkgId;
         _tcscpy(pInfo->szPkgFile, szPkgFile);
         _tcscpy(pInfo->szPlatform, szPlatform);
         _tcscpy(pInfo->szVersion, szVersion);

         m_dwRefCount++;
         ThreadCreate(DeploymentManager, 0, pInfo);
         msg.SetVariable(VID_RCC, RCC_SUCCESS);
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
      msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
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
void ClientSession::applyTemplate(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   NetObj *pSource, *pDestination;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   // Get source and destination
   pSource = FindObjectById(pRequest->GetVariableLong(VID_SOURCE_OBJECT_ID));
   pDestination = FindObjectById(pRequest->GetVariableLong(VID_DESTINATION_OBJECT_ID));
   if ((pSource != NULL) && (pDestination != NULL))
   {
      // Check object types
      if ((pSource->Type() == OBJECT_TEMPLATE) &&
          ((pDestination->Type() == OBJECT_NODE) || (pDestination->Type() == OBJECT_CLUSTER) || (pDestination->Type() == OBJECT_MOBILEDEVICE)))
      {
         TCHAR szLockInfo[MAX_SESSION_NAME];
         BOOL bLockSucceed = FALSE;

         // Acquire DCI lock if needed
         if (!((Template *)pSource)->isLockedBySession(m_dwIndex))
         {
            bLockSucceed = ((Template *)pSource)->lockDCIList(m_dwIndex, m_szUserName, szLockInfo);
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
               if (((DataCollectionTarget *)pDestination)->lockDCIList(m_dwIndex, m_szUserName, szLockInfo))
               {
                  BOOL bErrors;

                  ObjectTransactionStart();
                  bErrors = ((Template *)pSource)->applyToTarget((DataCollectionTarget *)pDestination);
                  ObjectTransactionEnd();
                  ((Template *)pDestination)->unlockDCIList(m_dwIndex);
                  msg.SetVariable(VID_RCC, bErrors ? RCC_DCI_COPY_ERRORS : RCC_SUCCESS);
               }
               else  // Destination's DCI list already locked by someone else
               {
                  msg.SetVariable(VID_RCC, RCC_COMPONENT_LOCKED);
                  msg.SetVariable(VID_LOCKED_BY, szLockInfo);
               }
            }
            else  // User doesn't have enough rights on object(s)
            {
               msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
            }

            ((Template *)pSource)->unlockDCIList(m_dwIndex);
         }
         else  // Source node DCI list not locked by this session
         {
            msg.SetVariable(VID_RCC, RCC_COMPONENT_LOCKED);
            msg.SetVariable(VID_LOCKED_BY, szLockInfo);
         }
      }
      else     // Object(s) is not a node
      {
         msg.SetVariable(VID_RCC, RCC_INVALID_OBJECT_ID);
      }
   }
   else  // No object(s) with given ID
   {
      msg.SetVariable(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Get user variable
 */
void ClientSession::getUserVariable(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   TCHAR szVarName[MAX_VARIABLE_NAME], szQuery[MAX_VARIABLE_NAME + 256];
   DB_RESULT hResult;
   UINT32 dwUserId;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   dwUserId = pRequest->isFieldExist(VID_USER_ID) ? pRequest->GetVariableLong(VID_USER_ID) : m_dwUserId;
   if ((dwUserId == m_dwUserId) || (m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_USERS))
   {
      // Try to read variable from database
      pRequest->GetVariableStr(VID_NAME, szVarName, MAX_VARIABLE_NAME);
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
            msg.SetVariable(VID_RCC, RCC_SUCCESS);
            msg.SetVariable(VID_VALUE, pszData);
            free(pszData);
         }
         else
         {
            msg.SetVariable(VID_RCC, RCC_VARIABLE_NOT_FOUND);
         }
         DBFreeResult(hResult);
      }
      else
      {
         msg.SetVariable(VID_RCC, RCC_DB_FAILURE);
      }
   }
   else
   {
      msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Set user variable
 */
void ClientSession::setUserVariable(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   TCHAR szVarName[MAX_VARIABLE_NAME], szQuery[8192], *pszValue, *pszRawValue;
   DB_RESULT hResult;
   BOOL bExist = FALSE;
   UINT32 dwUserId;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   dwUserId = pRequest->isFieldExist(VID_USER_ID) ? pRequest->GetVariableLong(VID_USER_ID) : m_dwUserId;
   if ((dwUserId == m_dwUserId) || (m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_USERS))
   {
      // Check variable name
      pRequest->GetVariableStr(VID_NAME, szVarName, MAX_VARIABLE_NAME);
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
         pszRawValue = pRequest->GetVariableStr(VID_VALUE);
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
            msg.SetVariable(VID_RCC, RCC_SUCCESS);
         }
         else
         {
            msg.SetVariable(VID_RCC, RCC_DB_FAILURE);
         }
      }
      else
      {
         msg.SetVariable(VID_RCC, RCC_INVALID_OBJECT_NAME);
      }
   }
   else
   {
      msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Enum user variables
 */
void ClientSession::enumUserVariables(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   TCHAR szPattern[MAX_VARIABLE_NAME], szQuery[256], szName[MAX_DB_STRING];
   UINT32 i, dwNumRows, dwNumVars, dwId, dwUserId;
   DB_RESULT hResult;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   dwUserId = pRequest->isFieldExist(VID_USER_ID) ? pRequest->GetVariableLong(VID_USER_ID) : m_dwUserId;
   if ((dwUserId == m_dwUserId) || (m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_USERS))
   {
      pRequest->GetVariableStr(VID_SEARCH_PATTERN, szPattern, MAX_VARIABLE_NAME);
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
               msg.SetVariable(dwId++, szName);
            }
         }
         msg.SetVariable(VID_NUM_VARIABLES, dwNumVars);
         msg.SetVariable(VID_RCC, RCC_SUCCESS);
         DBFreeResult(hResult);
      }
      else
      {
         msg.SetVariable(VID_RCC, RCC_DB_FAILURE);
      }
   }
   else
   {
      msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Delete user variable(s)
 */
void ClientSession::deleteUserVariable(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   TCHAR szVarName[MAX_VARIABLE_NAME], szQuery[MAX_VARIABLE_NAME + 256];
   UINT32 dwUserId;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   dwUserId = pRequest->isFieldExist(VID_USER_ID) ? pRequest->GetVariableLong(VID_USER_ID) : m_dwUserId;
   if ((dwUserId == m_dwUserId) || (m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_USERS))
   {
      // Try to delete variable from database
      pRequest->GetVariableStr(VID_NAME, szVarName, MAX_VARIABLE_NAME);
      TranslateStr(szVarName, _T("*"), _T("%"));
      _sntprintf(szQuery, MAX_VARIABLE_NAME + 256,
                 _T("DELETE FROM user_profiles WHERE user_id=%d AND var_name LIKE '%s'"),
                 dwUserId, szVarName);
      if (DBQuery(g_hCoreDB, szQuery))
      {
         msg.SetVariable(VID_RCC, RCC_SUCCESS);
      }
      else
      {
         msg.SetVariable(VID_RCC, RCC_DB_FAILURE);
      }
   }
   else
   {
      msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Copy or move user variable(s) to another user
 */
void ClientSession::copyUserVariable(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   TCHAR szVarName[MAX_VARIABLE_NAME], szCurrVar[MAX_VARIABLE_NAME],
         szQuery[32768], *pszValue;
   UINT32 dwSrcUserId, dwDstUserId;
   int i, nRows;
   BOOL bMove, bExist;
   DB_RESULT hResult, hResult2;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   if (m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_USERS)
   {
      dwSrcUserId = pRequest->isFieldExist(VID_USER_ID) ? pRequest->GetVariableLong(VID_USER_ID) : m_dwUserId;
      dwDstUserId = pRequest->GetVariableLong(VID_DST_USER_ID);
      bMove = (BOOL)pRequest->GetVariableShort(VID_MOVE_FLAG);
      pRequest->GetVariableStr(VID_NAME, szVarName, MAX_VARIABLE_NAME);
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
         msg.SetVariable(VID_RCC, RCC_SUCCESS);
      }
      else
      {
         msg.SetVariable(VID_RCC, RCC_DB_FAILURE);
      }
   }
   else
   {
      msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Change object's zone
 */
void ClientSession::changeObjectZone(CSCPMessage *pRequest)
{
   CSCPMessage msg;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   // Get object id and check prerequisites
   NetObj *object = FindObjectById(pRequest->GetVariableLong(VID_OBJECT_ID));
   if (object != NULL)
   {
      if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_MODIFY))
      {
			if (object->Type() == OBJECT_NODE)
			{
				Node *node = (Node *)object;
				UINT32 zoneId = pRequest->GetVariableLong(VID_ZONE_ID);
				Zone *zone = FindZoneByGUID(zoneId);
				if (zone != NULL)
				{
					// Check if target zone already have object with same primary IP
					if ((FindNodeByIP(zoneId, node->IpAddr()) == NULL) &&
						 (FindSubnetByIP(zoneId, node->IpAddr()) == NULL))
					{
						node->changeZone(zoneId);
						msg.SetVariable(VID_RCC, RCC_SUCCESS);
					}
					else
					{
						msg.SetVariable(VID_RCC, RCC_ADDRESS_IN_USE);
					}
				}
				else
				{
		         msg.SetVariable(VID_RCC, RCC_INVALID_ZONE_ID);
				}
         }
         else
         {
	         msg.SetVariable(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
         }
      }
      else
      {
         msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
      }
   }
   else
   {
      msg.SetVariable(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Setup encryption with client
 */
void ClientSession::setupEncryption(CSCPMessage *request)
{
   CSCPMessage msg;

#ifdef _WITH_ENCRYPTION
	m_dwEncryptionRqId = request->GetId();
   m_dwEncryptionResult = RCC_TIMEOUT;
   if (m_condEncryptionSetup == INVALID_CONDITION_HANDLE)
      m_condEncryptionSetup = ConditionCreate(FALSE);

   // Send request for session key
	PrepareKeyRequestMsg(&msg, g_pServerKey, request->GetVariableShort(VID_USE_X509_KEY_FORMAT) != 0);
	msg.SetId(request->GetId());
   sendMessage(&msg);
   msg.deleteAllVariables();

   // Wait for encryption setup
   ConditionWait(m_condEncryptionSetup, 30000);

   // Send response
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(request->GetId());
   msg.SetVariable(VID_RCC, m_dwEncryptionResult);
#else    /* _WITH_ENCRYPTION not defined */
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(request->GetId());
   msg.SetVariable(VID_RCC, RCC_NO_ENCRYPTION_SUPPORT);
#endif

   sendMessage(&msg);
}

/**
 * Get agent's configuration file
 */
void ClientSession::getAgentConfig(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   NetObj *object;
   UINT32 dwResult, dwSize;
   TCHAR *pszConfig;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   // Get object id and check prerequisites
   object = FindObjectById(pRequest->GetVariableLong(VID_OBJECT_ID));
   if (object != NULL)
   {
      if (object->Type() == OBJECT_NODE)
      {
         if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
         {
            AgentConnection *pConn;

            pConn = ((Node *)object)->createAgentConnection();
            if (pConn != NULL)
            {
               dwResult = pConn->getConfigFile(&pszConfig, &dwSize);
               delete pConn;
               switch(dwResult)
               {
                  case ERR_SUCCESS:
                     msg.SetVariable(VID_RCC, RCC_SUCCESS);
                     msg.SetVariable(VID_CONFIG_FILE, pszConfig);
                     free(pszConfig);
                     break;
                  case ERR_ACCESS_DENIED:
                     msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
                     break;
                  default:
                     msg.SetVariable(VID_RCC, RCC_COMM_FAILURE);
                     break;
               }
            }
            else
            {
               msg.SetVariable(VID_RCC, RCC_COMM_FAILURE);
            }
         }
         else
         {
            msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
         }
      }
      else
      {
         msg.SetVariable(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
      }
   }
   else
   {
      msg.SetVariable(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Update agent's configuration file
 */
void ClientSession::updateAgentConfig(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   NetObj *object;
   UINT32 dwResult;
   TCHAR *pszConfig;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   // Get object id and check prerequisites
   object = FindObjectById(pRequest->GetVariableLong(VID_OBJECT_ID));
   if (object != NULL)
   {
      if (object->Type() == OBJECT_NODE)
      {
         if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_CONTROL))
         {
            AgentConnection *pConn;

            pConn = ((Node *)object)->createAgentConnection();
            if (pConn != NULL)
            {
               pszConfig = pRequest->GetVariableStr(VID_CONFIG_FILE);
               dwResult = pConn->updateConfigFile(pszConfig);
               free(pszConfig);

               if ((pRequest->GetVariableShort(VID_APPLY_FLAG) != 0) &&
                   (dwResult == ERR_SUCCESS))
               {
                  dwResult = pConn->execAction(_T("Agent.Restart"), 0, NULL);
               }

               switch(dwResult)
               {
                  case ERR_SUCCESS:
                     msg.SetVariable(VID_RCC, RCC_SUCCESS);
                     break;
                  case ERR_ACCESS_DENIED:
                     msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
                     break;
                  case ERR_IO_FAILURE:
                     msg.SetVariable(VID_RCC, RCC_IO_ERROR);
                     break;
                  case ERR_MALFORMED_COMMAND:
                     msg.SetVariable(VID_RCC, RCC_INTERNAL_ERROR);
                     break;
                  default:
                     msg.SetVariable(VID_RCC, RCC_COMM_FAILURE);
                     break;
               }
               delete pConn;
            }
            else
            {
               msg.SetVariable(VID_RCC, RCC_COMM_FAILURE);
            }
         }
         else
         {
            msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
         }
      }
      else
      {
         msg.SetVariable(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
      }
   }
   else
   {
      msg.SetVariable(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Execute action on agent
 */
void ClientSession::executeAction(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   UINT32 dwResult;
   TCHAR szAction[MAX_PARAM_NAME];

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   // Get object id and check prerequisites
   NetObj *object = FindObjectById(pRequest->GetVariableLong(VID_OBJECT_ID));
   if (object != NULL)
   {
      if (object->Type() == OBJECT_NODE)
      {
         if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_CONTROL))
         {
            AgentConnection *pConn;

            pConn = ((Node *)object)->createAgentConnection();
            if (pConn != NULL)
            {
               pRequest->GetVariableStr(VID_ACTION_NAME, szAction, MAX_PARAM_NAME);
               dwResult = pConn->execAction(szAction, 0, NULL);

               switch(dwResult)
               {
                  case ERR_SUCCESS:
                     msg.SetVariable(VID_RCC, RCC_SUCCESS);
                     break;
                  case ERR_ACCESS_DENIED:
                     msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
                     break;
                  case ERR_IO_FAILURE:
                     msg.SetVariable(VID_RCC, RCC_IO_ERROR);
                     break;
                  case ERR_EXEC_FAILED:
                     msg.SetVariable(VID_RCC, RCC_EXEC_FAILED);
                     break;
                  default:
                     msg.SetVariable(VID_RCC, RCC_COMM_FAILURE);
                     break;
               }
               delete pConn;
            }
            else
            {
               msg.SetVariable(VID_RCC, RCC_COMM_FAILURE);
            }
         }
         else
         {
            msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
         }
      }
      else
      {
         msg.SetVariable(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
      }
   }
   else
   {
      msg.SetVariable(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Send tool list to client
 */
void ClientSession::sendObjectTools(UINT32 dwRqId)
{
   CSCPMessage msg;
   DB_RESULT hResult;
   UINT32 i, j, dwAclSize, dwNumTools, dwNumMsgRec, dwToolId, dwId;
   OBJECT_TOOL_ACL *pAccessList;
   TCHAR *pszStr, szBuffer[MAX_DB_STRING];

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(dwRqId);

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   hResult = DBSelect(hdb, _T("SELECT tool_id,user_id FROM object_tools_acl"));
   if (hResult != NULL)
   {
      dwAclSize = DBGetNumRows(hResult);
      pAccessList = (OBJECT_TOOL_ACL *)malloc(sizeof(OBJECT_TOOL_ACL) * dwAclSize);
      for(i = 0; i < dwAclSize; i++)
      {
         pAccessList[i].dwToolId = DBGetFieldULong(hResult, i, 0);
         pAccessList[i].dwUserId = DBGetFieldULong(hResult, i, 1);
      }
      DBFreeResult(hResult);

      hResult = DBSelect(hdb, _T("SELECT tool_id,tool_name,tool_type,tool_data,flags,description,matching_oid,confirmation_text FROM object_tools"));
      if (hResult != NULL)
      {
         dwNumTools = DBGetNumRows(hResult);
         for(i = 0, dwId = VID_OBJECT_TOOLS_BASE, dwNumMsgRec = 0; i < dwNumTools; i++)
         {
            dwToolId = DBGetFieldULong(hResult, i, 0);
            if ((m_dwUserId != 0) && (!(m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_TOOLS)))
            {
               for(j = 0; j < dwAclSize; j++)
               {
                  if (pAccessList[j].dwToolId == dwToolId)
                  {
                     if ((pAccessList[j].dwUserId == m_dwUserId) ||
                         (pAccessList[j].dwUserId == GROUP_EVERYONE))
                        break;
                     if (pAccessList[j].dwUserId & GROUP_FLAG)
                        if (CheckUserMembership(m_dwUserId, pAccessList[j].dwUserId))
                           break;
                  }
               }
            }

            if ((m_dwUserId == 0) ||
                (m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_TOOLS) ||
                (j < dwAclSize))   // User has access to this tool
            {
               msg.SetVariable(dwId, dwToolId);

               // name
               DBGetField(hResult, i, 1, szBuffer, MAX_DB_STRING);
               msg.SetVariable(dwId + 1, szBuffer);

               msg.SetVariable(dwId + 2, (WORD)DBGetFieldLong(hResult, i, 2));

               // data
               pszStr = DBGetField(hResult, i, 3, NULL, 0);
               msg.SetVariable(dwId + 3, pszStr);
               free(pszStr);

               msg.SetVariable(dwId + 4, DBGetFieldULong(hResult, i, 4));

               // description
               DBGetField(hResult, i, 5, szBuffer, MAX_DB_STRING);
               msg.SetVariable(dwId + 5, szBuffer);

               // matching OID
               DBGetField(hResult, i, 6, szBuffer, MAX_DB_STRING);
               msg.SetVariable(dwId + 6, szBuffer);

               // confirmation text
               DBGetField(hResult, i, 7, szBuffer, MAX_DB_STRING);
               msg.SetVariable(dwId + 7, szBuffer);

               dwNumMsgRec++;
               dwId += 10;
            }
         }
         msg.SetVariable(VID_NUM_TOOLS, dwNumMsgRec);

         DBFreeResult(hResult);
      }
      else
      {
         msg.SetVariable(VID_RCC, RCC_DB_FAILURE);
      }

      free(pAccessList);
   }
   else
   {
      msg.SetVariable(VID_RCC, RCC_DB_FAILURE);
   }
   DBConnectionPoolReleaseConnection(hdb);

   // Send response
   sendMessage(&msg);
}

/**
 * Send tool list to client
 */
void ClientSession::sendObjectToolDetails(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   DB_RESULT hResult;
   UINT32 dwToolId, dwId, *pdwAcl;
   TCHAR *pszStr, szBuffer[MAX_DB_STRING];
   int i, iNumRows, nType;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   if (m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_TOOLS)
   {

      dwToolId = pRequest->GetVariableLong(VID_TOOL_ID);
      DB_STATEMENT statment = DBPrepare(hdb, _T("SELECT tool_name,tool_type,tool_data,description,flags,matching_oid,confirmation_text FROM object_tools WHERE tool_id=?"));
      if (statment == NULL)
         goto failure;
      DBBind(statment, 1, DB_SQLTYPE_INTEGER, dwToolId);

      hResult = DBSelectPrepared(statment);
      if (hResult != NULL)
      {
         if (DBGetNumRows(hResult) > 0)
         {
				msg.SetVariable(VID_TOOL_ID, dwToolId);

            DBGetField(hResult, 0, 0, szBuffer, MAX_DB_STRING);
            msg.SetVariable(VID_NAME, szBuffer);

            nType = DBGetFieldLong(hResult, 0, 1);
            msg.SetVariable(VID_TOOL_TYPE, (WORD)nType);

            pszStr = DBGetField(hResult, 0, 2, NULL, 0);
            msg.SetVariable(VID_TOOL_DATA, pszStr);
            free(pszStr);

            DBGetField(hResult, 0, 3, szBuffer, MAX_DB_STRING);
            msg.SetVariable(VID_DESCRIPTION, szBuffer);

            msg.SetVariable(VID_FLAGS, DBGetFieldULong(hResult, 0, 4));

            DBGetField(hResult, 0, 5, szBuffer, MAX_DB_STRING);
            msg.SetVariable(VID_TOOL_OID, szBuffer);

            DBGetField(hResult, 0, 6, szBuffer, MAX_DB_STRING);
            msg.SetVariable(VID_CONFIRMATION_TEXT, szBuffer);

            DBFreeResult(hResult);

            // Access list
            DBFreeStatement(statment);
            statment = DBPrepare(hdb, _T("SELECT user_id FROM object_tools_acl WHERE tool_id=?"));
            if (statment == NULL)
               goto failure;
            DBBind(statment, 1, DB_SQLTYPE_INTEGER, dwToolId);

            hResult = DBSelectPrepared(statment);
            if (hResult != NULL)
            {
               iNumRows = DBGetNumRows(hResult);
               msg.SetVariable(VID_ACL_SIZE, (UINT32)iNumRows);
               if (iNumRows > 0)
               {
                  pdwAcl = (UINT32 *)malloc(sizeof(UINT32) * iNumRows);
                  for(i = 0; i < iNumRows; i++)
                     pdwAcl[i] = DBGetFieldULong(hResult, i, 0);
                  msg.setFieldInt32Array(VID_ACL, iNumRows, pdwAcl);
                  free(pdwAcl);
               }
               DBFreeResult(hResult);

               // Column information for table tools
               if ((nType == TOOL_TYPE_TABLE_SNMP) || (nType == TOOL_TYPE_TABLE_AGENT))
               {
                  DBFreeStatement(statment);
                  statment = DBPrepare(hdb, _T("SELECT col_name,col_oid,col_format,col_substr ")
                                                          _T("FROM object_tools_table_columns WHERE tool_id=? ")
                                                          _T("ORDER BY col_number"));
                  if (statment == NULL)
                     goto failure;
                  DBBind(statment, 1, DB_SQLTYPE_INTEGER, dwToolId);

                  hResult = DBSelectPrepared(statment);
                  if (hResult != NULL)
                  {
                     iNumRows = DBGetNumRows(hResult);
                     msg.SetVariable(VID_NUM_COLUMNS, (WORD)iNumRows);
                     for(i = 0, dwId = VID_COLUMN_INFO_BASE; i < iNumRows; i++)
                     {
                        DBGetField(hResult, i, 0, szBuffer, MAX_DB_STRING);
                        msg.SetVariable(dwId++, szBuffer);
                        msg.SetVariable(dwId++, DBGetField(hResult, i, 1, szBuffer, MAX_DB_STRING));
                        msg.SetVariable(dwId++, (WORD)DBGetFieldLong(hResult, i, 2));
                        msg.SetVariable(dwId++, (WORD)DBGetFieldLong(hResult, i, 3));
                     }
                     DBFreeResult(hResult);
                     msg.SetVariable(VID_RCC, RCC_SUCCESS);
                  }
                  else
                  {
                     msg.deleteAllVariables();
                     msg.SetVariable(VID_RCC, RCC_DB_FAILURE);
                  }
               }
            }
            else
            {
               msg.deleteAllVariables();
               msg.SetVariable(VID_RCC, RCC_DB_FAILURE);
            }
         }
         else
         {
            DBFreeResult(hResult);
            msg.SetVariable(VID_RCC, RCC_INVALID_TOOL_ID);
         }
      }
      else
      {
         msg.SetVariable(VID_RCC, RCC_DB_FAILURE);
      }
      DBFreeStatement(statment);
   }
   else
   {
      // Current user has no rights for object tools management
      msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
   }

   // Send response
   DBConnectionPoolReleaseConnection(hdb);
   sendMessage(&msg);
   return;

failure:
   DBConnectionPoolReleaseConnection(hdb);
   msg.SetVariable(VID_RCC, RCC_DB_FAILURE);
   sendMessage(&msg);
   return;
}

/**
 * Update object tool
 */
void ClientSession::updateObjectTool(CSCPMessage *pRequest)
{
   CSCPMessage msg;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   // Check user rights
   if (m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_TOOLS)
   {
      msg.SetVariable(VID_RCC, UpdateObjectToolFromMessage(pRequest));
   }
   else
   {
      msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Delete object tool
 */
void ClientSession::deleteObjectTool(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   UINT32 dwToolId;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   // Check user rights
   if (m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_TOOLS)
   {
      dwToolId = pRequest->GetVariableLong(VID_TOOL_ID);
      msg.SetVariable(VID_RCC, DeleteObjectToolFromDB(dwToolId));
   }
   else
   {
      msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Change Object Tool status (enabled/disabled)
 */
void ClientSession::changeObjectToolStatus(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   UINT32 toolID, enable;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   // Check user rights
   if (m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_TOOLS)
   {
      toolID = pRequest->GetVariableLong(VID_TOOL_ID);
      enable = pRequest->GetVariableLong(VID_STATE);
      msg.SetVariable(VID_RCC, ChangeObjectToolStatus(toolID, enable == 0 ? false : true));
   }
   else
   {
      msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Generate ID for new object tool
 */
void ClientSession::generateObjectToolId(UINT32 dwRqId)
{
   CSCPMessage msg;

   // Prepare reply message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(dwRqId);

   // Check access rights
   if (checkSysAccessRights(SYSTEM_ACCESS_MANAGE_TOOLS))
   {
      msg.SetVariable(VID_TOOL_ID, CreateUniqueId(IDG_OBJECT_TOOL));
   }
   else
   {
      msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Execute table tool (either SNMP or agent table)
 */
void ClientSession::execTableTool(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   UINT32 dwToolId;
   NetObj *object;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   // Check if tool exist and has correct type
   dwToolId = pRequest->GetVariableLong(VID_TOOL_ID);
   if (IsTableTool(dwToolId))
   {
      // Check access
      if (CheckObjectToolAccess(dwToolId, m_dwUserId))
      {
         object = FindObjectById(pRequest->GetVariableLong(VID_OBJECT_ID));
         if (object != NULL)
         {
            if (object->Type() == OBJECT_NODE)
            {
               msg.SetVariable(VID_RCC,
                               ExecuteTableTool(dwToolId, (Node *)object,
                                                pRequest->GetId(), this));
            }
            else
            {
               msg.SetVariable(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
            }
         }
         else
         {
            msg.SetVariable(VID_RCC, RCC_INVALID_OBJECT_ID);
         }
      }
      else
      {
         msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
      }
   }
   else
   {
      msg.SetVariable(VID_RCC, RCC_INVALID_TOOL_ID);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Change current subscription
 */
void ClientSession::changeSubscription(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   UINT32 dwFlags;

   dwFlags = pRequest->GetVariableLong(VID_FLAGS);

   if (pRequest->GetVariableShort(VID_OPERATION) != 0)
   {
      m_dwActiveChannels |= dwFlags;   // Subscribe
   }
   else
   {
      m_dwActiveChannels &= ~dwFlags;   // Unsubscribe
   }

   // Send response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());
   msg.SetVariable(VID_RCC, RCC_SUCCESS);
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
   CSCPMessage msg;
#ifdef _WIN32
   PROCESS_MEMORY_COUNTERS mc;
#endif

   // Prepare response
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_RCC, RCC_SUCCESS);

   // Server version, etc.
   msg.SetVariable(VID_SERVER_VERSION, NETXMS_VERSION_STRING);
   msg.SetVariable(VID_SERVER_UPTIME, (UINT32)(time(NULL) - g_serverStartTime));

   // Number of objects and DCIs
	UINT32 dciCount = 0;
	g_idxNodeById.forEach(DciCountCallback, &dciCount);
   msg.SetVariable(VID_NUM_ITEMS, dciCount);
	msg.SetVariable(VID_NUM_OBJECTS, (UINT32)g_idxObjectById.getSize());
	msg.SetVariable(VID_NUM_NODES, (UINT32)g_idxNodeById.getSize());

   // Client sessions
   msg.SetVariable(VID_NUM_SESSIONS, (UINT32)GetSessionCount());

   // Alarms
   g_alarmMgr.getAlarmStats(&msg);

   // Process info
#ifdef _WIN32
   mc.cb = sizeof(PROCESS_MEMORY_COUNTERS);
   if (GetProcessMemoryInfo(GetCurrentProcess(), &mc, sizeof(PROCESS_MEMORY_COUNTERS)))
   {
      msg.SetVariable(VID_NETXMSD_PROCESS_WKSET, (UINT32)(mc.WorkingSetSize / 1024));
      msg.SetVariable(VID_NETXMSD_PROCESS_VMSIZE, (UINT32)(mc.PagefileUsage / 1024));
   }
#endif

	// Queues
	msg.SetVariable(VID_QSIZE_CONDITION_POLLER, g_conditionPollerQueue.Size());
	msg.SetVariable(VID_QSIZE_CONF_POLLER, g_configPollQueue.Size());
	msg.SetVariable(VID_QSIZE_DCI_POLLER, g_pItemQueue->Size());
	msg.SetVariable(VID_QSIZE_DBWRITER, g_pLazyRequestQueue->Size());
	msg.SetVariable(VID_QSIZE_EVENT, g_pEventQueue->Size());
	msg.SetVariable(VID_QSIZE_DISCOVERY, g_discoveryPollQueue.Size());
	msg.SetVariable(VID_QSIZE_NODE_POLLER, g_nodePollerQueue.Size());
	msg.SetVariable(VID_QSIZE_ROUTE_POLLER, g_routePollQueue.Size());
	msg.SetVariable(VID_QSIZE_STATUS_POLLER, g_statusPollQueue.Size());

   // Send response
   sendMessage(&msg);
}

/**
 * Send script list
 */
void ClientSession::sendScriptList(UINT32 dwRqId)
{
   CSCPMessage msg;
   DB_RESULT hResult;
   UINT32 i, dwNumScripts, dwId;
   TCHAR szBuffer[MAX_DB_STRING];

   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(dwRqId);
   if (m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_SCRIPTS)
   {
      hResult = DBSelect(g_hCoreDB, _T("SELECT script_id,script_name FROM script_library"));
      if (hResult != NULL)
      {
         msg.SetVariable(VID_RCC, RCC_SUCCESS);
         dwNumScripts = DBGetNumRows(hResult);
         msg.SetVariable(VID_NUM_SCRIPTS, dwNumScripts);
         for(i = 0, dwId = VID_SCRIPT_LIST_BASE; i < dwNumScripts; i++)
         {
            msg.SetVariable(dwId++, DBGetFieldULong(hResult, i, 0));
            msg.SetVariable(dwId++, DBGetField(hResult, i, 1, szBuffer, MAX_DB_STRING));
         }
         DBFreeResult(hResult);
      }
      else
      {
         msg.SetVariable(VID_RCC, RCC_DB_FAILURE);
      }
   }
   else
   {
      msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
   }
   sendMessage(&msg);
}

/**
 * Send script
 */
void ClientSession::sendScript(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   DB_RESULT hResult;
   UINT32 dwScriptId;
   TCHAR *pszCode, szQuery[256];

   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());
   if (m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_SCRIPTS)
   {
      dwScriptId = pRequest->GetVariableLong(VID_SCRIPT_ID);
      _sntprintf(szQuery, 256, _T("SELECT script_name,script_code FROM script_library WHERE script_id=%d"), dwScriptId);
      hResult = DBSelect(g_hCoreDB, szQuery);
      if (hResult != NULL)
      {
         if (DBGetNumRows(hResult) > 0)
         {
				TCHAR name[MAX_DB_STRING];

            msg.SetVariable(VID_NAME, DBGetField(hResult, 0, 0, name, MAX_DB_STRING));

				pszCode = DBGetField(hResult, 0, 1, NULL, 0);
            msg.SetVariable(VID_SCRIPT_CODE, pszCode);
            free(pszCode);
         }
         else
         {
            msg.SetVariable(VID_RCC, RCC_INVALID_SCRIPT_ID);
         }
         DBFreeResult(hResult);
      }
      else
      {
         msg.SetVariable(VID_RCC, RCC_DB_FAILURE);
      }
   }
   else
   {
      msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
   }
   sendMessage(&msg);
}

/**
 * Update script in library
 */
void ClientSession::updateScript(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   TCHAR *pszCode, *pszQuery, szName[MAX_DB_STRING];
   UINT32 dwScriptId;

   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());
   if (m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_SCRIPTS)
   {
      dwScriptId = pRequest->GetVariableLong(VID_SCRIPT_ID);
      pRequest->GetVariableStr(VID_NAME, szName, MAX_DB_STRING);
      if (IsValidScriptName(szName))
      {
         pszCode = pRequest->GetVariableStr(VID_SCRIPT_CODE);
         if (pszCode != NULL)
         {
				/* TODO: change to binding variable */
				String prepCode = DBPrepareString(g_hCoreDB, pszCode);
            free(pszCode);

				size_t qlen = (size_t)prepCode.getSize() + MAX_DB_STRING + 256;
            pszQuery = (TCHAR *)malloc(qlen * sizeof(TCHAR));

            if (dwScriptId == 0)
            {
               // New script
               dwScriptId = CreateUniqueId(IDG_SCRIPT);
               _sntprintf(pszQuery, qlen, _T("INSERT INTO script_library (script_id,script_name,script_code) VALUES (%d,%s,%s)"),
                         dwScriptId, (const TCHAR *)DBPrepareString(g_hCoreDB, szName), (const TCHAR *)prepCode);
            }
            else
            {
               _sntprintf(pszQuery, qlen, _T("UPDATE script_library SET script_name=%s,script_code=%s WHERE script_id=%d"),
                         (const TCHAR *)DBPrepareString(g_hCoreDB, szName), (const TCHAR *)prepCode, dwScriptId);
            }
            if (DBQuery(g_hCoreDB, pszQuery))
            {
               ReloadScript(dwScriptId);
               msg.SetVariable(VID_RCC, RCC_SUCCESS);
               msg.SetVariable(VID_SCRIPT_ID, dwScriptId);
            }
            else
            {
               msg.SetVariable(VID_RCC, RCC_DB_FAILURE);
            }
            free(pszQuery);
         }
         else
         {
            msg.SetVariable(VID_RCC, RCC_INVALID_REQUEST);
         }
      }
      else
      {
         msg.SetVariable(VID_RCC, RCC_INVALID_SCRIPT_NAME);
      }
   }
   else
   {
      msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
   }
   sendMessage(&msg);
}

/**
 * Rename script
 */
void ClientSession::renameScript(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   TCHAR szQuery[4096], szName[MAX_DB_STRING];
   UINT32 dwScriptId;

   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());
   if (m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_SCRIPTS)
   {
      dwScriptId = pRequest->GetVariableLong(VID_SCRIPT_ID);
      pRequest->GetVariableStr(VID_NAME, szName, MAX_DB_STRING);
      if (IsValidScriptName(szName))
      {
         if (IsValidScriptId(dwScriptId))
         {
            _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("UPDATE script_library SET script_name=%s WHERE script_id=%d"),
                      (const TCHAR *)DBPrepareString(g_hCoreDB, szName), dwScriptId);
            if (DBQuery(g_hCoreDB, szQuery))
            {
               ReloadScript(dwScriptId);
               msg.SetVariable(VID_RCC, RCC_SUCCESS);
            }
            else
            {
               msg.SetVariable(VID_RCC, RCC_DB_FAILURE);
            }
         }
         else
         {
            msg.SetVariable(VID_RCC, RCC_INVALID_SCRIPT_ID);
         }
      }
      else
      {
         msg.SetVariable(VID_RCC, RCC_INVALID_SCRIPT_NAME);
      }
   }
   else
   {
      msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
   }
   sendMessage(&msg);
}

/**
 * Delete script from library
 */
void ClientSession::deleteScript(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   TCHAR szQuery[256];
   UINT32 dwScriptId;

   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());
   if (m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_SCRIPTS)
   {
      dwScriptId = pRequest->GetVariableLong(VID_SCRIPT_ID);
      if (IsValidScriptId(dwScriptId))
      {
         _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("DELETE FROM script_library WHERE script_id=%d"), dwScriptId);
         if (DBQuery(g_hCoreDB, szQuery))
         {
            g_pScriptLibrary->lock();
            g_pScriptLibrary->deleteScript(dwScriptId);
            g_pScriptLibrary->unlock();
            msg.SetVariable(VID_RCC, RCC_SUCCESS);
         }
         else
         {
            msg.SetVariable(VID_RCC, RCC_DB_FAILURE);
         }
      }
      else
      {
         msg.SetVariable(VID_RCC, RCC_INVALID_SCRIPT_ID);
      }
   }
   else
   {
      msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
   }
   sendMessage(&msg);
}

/**
 * Copy session data to message
 */
static void CopySessionData(ClientSession *pSession, void *pArg)
{
   UINT32 dwId, dwIndex;

   dwIndex = ((CSCPMessage *)pArg)->GetVariableLong(VID_NUM_SESSIONS);
   ((CSCPMessage *)pArg)->SetVariable(VID_NUM_SESSIONS, dwIndex + 1);

   dwId = VID_SESSION_DATA_BASE + dwIndex * 100;
   ((CSCPMessage *)pArg)->SetVariable(dwId++, pSession->getIndex());
   ((CSCPMessage *)pArg)->SetVariable(dwId++, (WORD)pSession->getCipher());
   ((CSCPMessage *)pArg)->SetVariable(dwId++, (TCHAR *)pSession->getUserName());
   ((CSCPMessage *)pArg)->SetVariable(dwId++, (TCHAR *)pSession->getClientInfo());
}

/**
 * Send list of connected client sessions
 */
void ClientSession::SendSessionList(UINT32 dwRqId)
{
   CSCPMessage msg;

   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(dwRqId);
   if (m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_SESSIONS)
   {
      msg.SetVariable(VID_NUM_SESSIONS, (UINT32)0);
      EnumerateClientSessions(CopySessionData, &msg);
      msg.SetVariable(VID_RCC, RCC_SUCCESS);
   }
   else
   {
      msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
   }
   sendMessage(&msg);
}


//
// Forcibly terminate client's session
//

void ClientSession::KillSession(CSCPMessage *pRequest)
{
   CSCPMessage msg;

   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());
   if (m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_SESSIONS)
   {
   }
   else
   {
      msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
   }
   sendMessage(&msg);
}

/**
 * Handler for new syslog messages
 */
void ClientSession::onSyslogMessage(NX_SYSLOG_RECORD *pRec)
{
   UPDATE_INFO *pUpdate;

   if (isAuthenticated() && isSubscribed(NXC_CHANNEL_SYSLOG))
   {
      pUpdate = (UPDATE_INFO *)malloc(sizeof(UPDATE_INFO));
      pUpdate->dwCategory = INFO_CAT_SYSLOG_MSG;
      pUpdate->pData = nx_memdup(pRec, sizeof(NX_SYSLOG_RECORD));
      m_pUpdateQueue->Put(pUpdate);
   }
}

/**
 * Get latest syslog records
 */
void ClientSession::sendSyslog(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   UINT32 dwMaxRecords, dwNumRows, dwId;
   DB_RESULT hTempResult;
   DB_ASYNC_RESULT hResult;
   TCHAR szQuery[1024], szBuffer[1024];
   WORD wRecOrder;

   wRecOrder = ((g_nDBSyntax == DB_SYNTAX_MSSQL) || (g_nDBSyntax == DB_SYNTAX_ORACLE)) ? RECORD_ORDER_REVERSED : RECORD_ORDER_NORMAL;
   dwMaxRecords = pRequest->GetVariableLong(VID_MAX_RECORDS);

   // Prepare confirmation message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   MutexLock(m_mutexSendSyslog);

   // Retrieve events from database
   switch(g_nDBSyntax)
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
		msg.SetVariable(VID_RCC, RCC_SUCCESS);
		sendMessage(&msg);
		msg.deleteAllVariables();
		msg.SetCode(CMD_SYSLOG_RECORDS);

      // Send records, up to 10 per message
      for(dwId = VID_SYSLOG_MSG_BASE, dwNumRows = 0; DBFetch(hResult); dwNumRows++)
      {
         if (dwNumRows == 10)
         {
            msg.SetVariable(VID_NUM_RECORDS, dwNumRows);
            msg.SetVariable(VID_RECORDS_ORDER, wRecOrder);
            sendMessage(&msg);
            msg.deleteAllVariables();
            dwNumRows = 0;
            dwId = VID_SYSLOG_MSG_BASE;
         }
         msg.SetVariable(dwId++, DBGetFieldAsyncUInt64(hResult, 0));
         msg.SetVariable(dwId++, DBGetFieldAsyncULong(hResult, 1));
         msg.SetVariable(dwId++, (WORD)DBGetFieldAsyncLong(hResult, 2));
         msg.SetVariable(dwId++, (WORD)DBGetFieldAsyncLong(hResult, 3));
         msg.SetVariable(dwId++, DBGetFieldAsyncULong(hResult, 4));
         msg.SetVariable(dwId++, DBGetFieldAsync(hResult, 5, szBuffer, 1024));
         msg.SetVariable(dwId++, DBGetFieldAsync(hResult, 6, szBuffer, 1024));
         msg.SetVariable(dwId++, DBGetFieldAsync(hResult, 7, szBuffer, 1024));
      }
      DBFreeAsyncResult(hResult);

      // Send remaining records with End-Of-Sequence notification
      msg.SetVariable(VID_NUM_RECORDS, dwNumRows);
      msg.SetVariable(VID_RECORDS_ORDER, wRecOrder);
      msg.setEndOfSequence();
      sendMessage(&msg);
   }
   else
   {
		msg.SetVariable(VID_RCC, RCC_DB_FAILURE);
		sendMessage(&msg);
	}

   DBConnectionPoolReleaseConnection(hdb);
   MutexUnlock(m_mutexSendSyslog);
}

/**
 * Handler for new traps
 */
void ClientSession::onNewSNMPTrap(CSCPMessage *pMsg)
{
   UPDATE_INFO *pUpdate;
   if (isAuthenticated() && isSubscribed(NXC_CHANNEL_SNMP_TRAPS))
   {
      pUpdate = (UPDATE_INFO *)malloc(sizeof(UPDATE_INFO));
      pUpdate->dwCategory = INFO_CAT_SNMP_TRAP;
      pUpdate->pData = new CSCPMessage(pMsg);
      m_pUpdateQueue->Put(pUpdate);
   }
}

/**
 * Send collected trap log
 */
void ClientSession::SendTrapLog(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   TCHAR szBuffer[4096], szQuery[1024];
   UINT32 dwId, dwNumRows, dwMaxRecords;
   DB_RESULT hTempResult;
   DB_ASYNC_RESULT hResult;
   WORD wRecOrder;

   wRecOrder = ((g_nDBSyntax == DB_SYNTAX_MSSQL) || (g_nDBSyntax == DB_SYNTAX_ORACLE)) ? RECORD_ORDER_REVERSED : RECORD_ORDER_NORMAL;
   dwMaxRecords = pRequest->GetVariableLong(VID_MAX_RECORDS);

   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   if (m_dwSystemAccess & SYSTEM_ACCESS_VIEW_TRAP_LOG)
   {
      msg.SetVariable(VID_RCC, RCC_SUCCESS);
      sendMessage(&msg);
      msg.deleteAllVariables();
      msg.SetCode(CMD_TRAP_LOG_RECORDS);

      MutexLock(m_mutexSendTrapLog);

      // Retrieve trap log records from database
      switch(g_nDBSyntax)
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
               msg.SetVariable(VID_NUM_RECORDS, dwNumRows);
               msg.SetVariable(VID_RECORDS_ORDER, wRecOrder);
               sendMessage(&msg);
               msg.deleteAllVariables();
               dwNumRows = 0;
               dwId = VID_TRAP_LOG_MSG_BASE;
            }
            msg.SetVariable(dwId++, DBGetFieldAsyncUInt64(hResult, 0));
            msg.SetVariable(dwId++, DBGetFieldAsyncULong(hResult, 1));
            msg.SetVariable(dwId++, DBGetFieldAsyncIPAddr(hResult, 2));
            msg.SetVariable(dwId++, DBGetFieldAsyncULong(hResult, 3));
            msg.SetVariable(dwId++, DBGetFieldAsync(hResult, 4, szBuffer, 256));
            msg.SetVariable(dwId++, DBGetFieldAsync(hResult, 5, szBuffer, 4096));
         }
         DBFreeAsyncResult(hResult);

         // Send remaining records with End-Of-Sequence notification
         msg.SetVariable(VID_NUM_RECORDS, dwNumRows);
         msg.SetVariable(VID_RECORDS_ORDER, wRecOrder);
         msg.setEndOfSequence();
      }

      DBConnectionPoolReleaseConnection(hdb);
      MutexUnlock(m_mutexSendTrapLog);
   }
   else
   {
      msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
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
   CSCPMessage *pMsg;
   UINT32 dwId;
   UINT32 dwNumVars;
   ClientSession *pSession;
} WALKER_ENUM_CALLBACK_ARGS;

/**
 * SNMP walker enumeration callback
 */
static UINT32 WalkerCallback(UINT32 dwVersion, SNMP_Variable *pVar, SNMP_Transport *pTransport, void *pArg)
{
   CSCPMessage *pMsg = ((WALKER_ENUM_CALLBACK_ARGS *)pArg)->pMsg;
   TCHAR szBuffer[4096];
	bool convertToHex = true;

	pVar->getValueAsPrintableString(szBuffer, 4096, &convertToHex);
   pMsg->SetVariable(((WALKER_ENUM_CALLBACK_ARGS *)pArg)->dwId++, (TCHAR *)pVar->getName()->getValueAsText());
	pMsg->SetVariable(((WALKER_ENUM_CALLBACK_ARGS *)pArg)->dwId++, convertToHex ? (UINT32)0xFFFF : pVar->getType());
   pMsg->SetVariable(((WALKER_ENUM_CALLBACK_ARGS *)pArg)->dwId++, szBuffer);
   ((WALKER_ENUM_CALLBACK_ARGS *)pArg)->dwNumVars++;
   if (((WALKER_ENUM_CALLBACK_ARGS *)pArg)->dwNumVars == 50)
   {
      pMsg->SetVariable(VID_NUM_VARIABLES, ((WALKER_ENUM_CALLBACK_ARGS *)pArg)->dwNumVars);
      ((WALKER_ENUM_CALLBACK_ARGS *)pArg)->pSession->sendMessage(pMsg);
      ((WALKER_ENUM_CALLBACK_ARGS *)pArg)->dwNumVars = 0;
      ((WALKER_ENUM_CALLBACK_ARGS *)pArg)->dwId = VID_SNMP_WALKER_DATA_BASE;
      pMsg->deleteAllVariables();
   }
   return SNMP_ERR_SUCCESS;
}


//
// SNMP walker thread
//

static THREAD_RESULT THREAD_CALL WalkerThread(void *pArg)
{
   CSCPMessage msg;
   WALKER_ENUM_CALLBACK_ARGS args;

   msg.SetCode(CMD_SNMP_WALK_DATA);
   msg.SetId(((WALKER_THREAD_ARGS *)pArg)->dwRqId);

   args.pMsg = &msg;
   args.dwId = VID_SNMP_WALKER_DATA_BASE;
   args.dwNumVars = 0;
   args.pSession = ((WALKER_THREAD_ARGS *)pArg)->pSession;
   ((Node *)(((WALKER_THREAD_ARGS *)pArg)->object))->callSnmpEnumerate(((WALKER_THREAD_ARGS *)pArg)->szBaseOID, WalkerCallback, &args);
   msg.SetVariable(VID_NUM_VARIABLES, args.dwNumVars);
   msg.setEndOfSequence();
   ((WALKER_THREAD_ARGS *)pArg)->pSession->sendMessage(&msg);

   ((WALKER_THREAD_ARGS *)pArg)->pSession->decRefCount();
   ((WALKER_THREAD_ARGS *)pArg)->object->decRefCount();
   free(pArg);
   return THREAD_OK;
}

/**
 * Start SNMP walk
 */
void ClientSession::StartSnmpWalk(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   WALKER_THREAD_ARGS *pArg;

   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   NetObj *object = FindObjectById(pRequest->GetVariableLong(VID_OBJECT_ID));
   if (object != NULL)
   {
      if (object->Type() == OBJECT_NODE)
      {
         if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
         {
            msg.SetVariable(VID_RCC, RCC_SUCCESS);

            object->incRefCount();
            m_dwRefCount++;

            pArg = (WALKER_THREAD_ARGS *)malloc(sizeof(WALKER_THREAD_ARGS));
            pArg->pSession = this;
            pArg->object = object;
            pArg->dwRqId = pRequest->GetId();
            pRequest->GetVariableStr(VID_SNMP_OID, pArg->szBaseOID, MAX_OID_LEN * 4);

            ThreadCreate(WalkerThread, 0, pArg);
         }
         else
         {
            msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
         }
      }
      else
      {
         msg.SetVariable(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
      }
   }
   else
   {
      msg.SetVariable(VID_RCC, RCC_INVALID_OBJECT_ID);
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
		if ((object->Type() == OBJECT_NODE) ||
			 (object->Type() == OBJECT_CLUSTER) ||
			 (object->Type() == OBJECT_MOBILEDEVICE) ||
			 (object->Type() == OBJECT_TEMPLATE))
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
void ClientSession::resolveDCINames(CSCPMessage *pRequest)
{
   UINT32 i, dwId, dwNumDCI, *pdwNodeList, *pdwDCIList, dwResult = RCC_INVALID_ARGUMENT;
   TCHAR *pszName;
   CSCPMessage msg;

   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   dwNumDCI = pRequest->GetVariableLong(VID_NUM_ITEMS);
   pdwNodeList = (UINT32 *)malloc(sizeof(UINT32) * dwNumDCI);
   pdwDCIList = (UINT32 *)malloc(sizeof(UINT32) * dwNumDCI);
   pRequest->getFieldAsInt32Array(VID_NODE_LIST, dwNumDCI, pdwNodeList);
   pRequest->getFieldAsInt32Array(VID_DCI_LIST, dwNumDCI, pdwDCIList);

   for(i = 0, dwId = VID_DCI_LIST_BASE; i < dwNumDCI; i++)
   {
      dwResult = resolveDCIName(pdwNodeList[i], pdwDCIList[i], &pszName);
      if (dwResult != RCC_SUCCESS)
         break;
      msg.SetVariable(dwId++, pszName);
   }

   free(pdwNodeList);
   free(pdwDCIList);

   msg.SetVariable(VID_RCC, dwResult);
   sendMessage(&msg);
}

/**
 * Send list of available agent configs to client
 */
void ClientSession::sendAgentCfgList(UINT32 dwRqId)
{
   CSCPMessage msg;
   DB_RESULT hResult;
   UINT32 i, dwId, dwNumRows;
   TCHAR szText[MAX_DB_STRING];

   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(dwRqId);

   if (m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_AGENT_CFG)
   {
      hResult = DBSelect(g_hCoreDB, _T("SELECT config_id,config_name,sequence_number FROM agent_configs"));
      if (hResult != NULL)
      {
         dwNumRows = DBGetNumRows(hResult);
         msg.SetVariable(VID_RCC, RCC_SUCCESS);
         msg.SetVariable(VID_NUM_RECORDS, dwNumRows);
         for(i = 0, dwId = VID_AGENT_CFG_LIST_BASE; i < dwNumRows; i++, dwId += 7)
         {
            msg.SetVariable(dwId++, DBGetFieldULong(hResult, i, 0));
            DBGetField(hResult, i, 1, szText, MAX_DB_STRING);
            DecodeSQLString(szText);
            msg.SetVariable(dwId++, szText);
            msg.SetVariable(dwId++, DBGetFieldULong(hResult, i, 2));
         }
         DBFreeResult(hResult);
      }
      else
      {
         msg.SetVariable(VID_RCC, RCC_DB_FAILURE);
      }
   }
   else
   {
      msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
   }

   sendMessage(&msg);
}


/**
 *  Open (get all data) server-stored agent's config
 */
void ClientSession::OpenAgentConfig(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   DB_RESULT hResult;
   UINT32 dwCfgId;
   TCHAR *pszStr, szQuery[256], szBuffer[MAX_DB_STRING];

   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   if (m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_AGENT_CFG)
   {
      dwCfgId = pRequest->GetVariableLong(VID_CONFIG_ID);
      _sntprintf(szQuery, 256, _T("SELECT config_name,config_file,config_filter,sequence_number FROM agent_configs WHERE config_id=%d"), dwCfgId);
      hResult = DBSelect(g_hCoreDB, szQuery);
      if (hResult != NULL)
      {
         if (DBGetNumRows(hResult) > 0)
         {
            msg.SetVariable(VID_RCC, RCC_SUCCESS);
            msg.SetVariable(VID_CONFIG_ID, dwCfgId);
            DecodeSQLStringAndSetVariable(&msg, VID_NAME, DBGetField(hResult, 0, 0, szBuffer, MAX_DB_STRING));
            pszStr = DBGetField(hResult, 0, 1, NULL, 0);
            DecodeSQLStringAndSetVariable(&msg, VID_CONFIG_FILE, pszStr);
            free(pszStr);
            pszStr = DBGetField(hResult, 0, 2, NULL, 0);
            DecodeSQLStringAndSetVariable(&msg, VID_FILTER, pszStr);
            free(pszStr);
            msg.SetVariable(VID_SEQUENCE_NUMBER, DBGetFieldULong(hResult, 0, 3));
         }
         else
         {
            msg.SetVariable(VID_RCC, RCC_INVALID_CONFIG_ID);
         }
         DBFreeResult(hResult);
      }
      else
      {
         msg.SetVariable(VID_RCC, RCC_DB_FAILURE);
      }
   }
   else
   {
      msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
   }

   sendMessage(&msg);
}


/**
 *  Save changes to server-stored agent's configuration
 */
void ClientSession::SaveAgentConfig(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   DB_RESULT hResult;
   UINT32 dwCfgId, dwSeqNum;
   TCHAR szQuery[256], szName[MAX_DB_STRING], *pszFilter, *pszText;
   TCHAR *pszQuery, *pszEscFilter, *pszEscText, *pszEscName;
   BOOL bCreate;

   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   if (m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_AGENT_CFG)
   {
      dwCfgId = pRequest->GetVariableLong(VID_CONFIG_ID);
      _sntprintf(szQuery, 256, _T("SELECT config_name FROM agent_configs WHERE config_id=%d"), dwCfgId);
      hResult = DBSelect(g_hCoreDB, szQuery);
      if (hResult != NULL)
      {
         bCreate = (DBGetNumRows(hResult) == 0);
         DBFreeResult(hResult);

         pRequest->GetVariableStr(VID_NAME, szName, MAX_DB_STRING);
         pszEscName = EncodeSQLString(szName);

         pszFilter = pRequest->GetVariableStr(VID_FILTER);
         pszEscFilter = EncodeSQLString(pszFilter);
         free(pszFilter);

         pszText = pRequest->GetVariableStr(VID_CONFIG_FILE);
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
               msg.SetVariable(VID_CONFIG_ID, dwCfgId);

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
               msg.SetVariable(VID_SEQUENCE_NUMBER, dwSeqNum);
            }
            _sntprintf(pszQuery, qlen, _T("INSERT INTO agent_configs (config_id,config_name,config_filter,config_file,sequence_number) VALUES (%d,'%s','%s','%s',%d)"),
                      dwCfgId, pszEscName, pszEscFilter, pszEscText, dwSeqNum);
         }
         else
         {
            _sntprintf(pszQuery, qlen, _T("UPDATE agent_configs SET config_name='%s',config_filter='%s',config_file='%s',sequence_number=%d WHERE config_id=%d"),
                      pszEscName, pszEscFilter, pszEscText,
                      pRequest->GetVariableLong(VID_SEQUENCE_NUMBER), dwCfgId);
         }
         free(pszEscName);
         free(pszEscText);
         free(pszEscFilter);

         if (DBQuery(g_hCoreDB, pszQuery))
         {
            msg.SetVariable(VID_RCC, RCC_SUCCESS);
         }
         else
         {
            msg.SetVariable(VID_RCC, RCC_DB_FAILURE);
         }
         free(pszQuery);
      }
      else
      {
         msg.SetVariable(VID_RCC, RCC_DB_FAILURE);
      }
   }
   else
   {
      msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
   }

   sendMessage(&msg);
}


/**
 * Delete agent's configuration
 */
void ClientSession::DeleteAgentConfig(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   DB_RESULT hResult;
   UINT32 dwCfgId;
   TCHAR szQuery[256];

   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   if (m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_AGENT_CFG)
   {
      dwCfgId = pRequest->GetVariableLong(VID_CONFIG_ID);
      _sntprintf(szQuery, 256, _T("SELECT config_name FROM agent_configs WHERE config_id=%d"), dwCfgId);
      hResult = DBSelect(g_hCoreDB, szQuery);
      if (hResult != NULL)
      {
         if (DBGetNumRows(hResult) > 0)
         {
            _sntprintf(szQuery, 256, _T("DELETE FROM agent_configs WHERE config_id=%d"), dwCfgId);
            if (DBQuery(g_hCoreDB, szQuery))
            {
               msg.SetVariable(VID_RCC, RCC_SUCCESS);
            }
            else
            {
               msg.SetVariable(VID_RCC, RCC_DB_FAILURE);
            }
         }
         else
         {
            msg.SetVariable(VID_RCC, RCC_INVALID_CONFIG_ID);
         }
         DBFreeResult(hResult);
      }
      else
      {
         msg.SetVariable(VID_RCC, RCC_DB_FAILURE);
      }
   }
   else
   {
      msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
   }

   sendMessage(&msg);
}


/**
 * Swap sequence numbers of two agent configs
 */
void ClientSession::SwapAgentConfigs(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   DB_RESULT hResult;
   TCHAR szQuery[256];
   BOOL bRet;

   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   if (m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_AGENT_CFG)
   {
      _sntprintf(szQuery, 256, _T("SELECT config_id,sequence_number FROM agent_configs WHERE config_id=%d OR config_id=%d"),
                 pRequest->GetVariableLong(VID_CONFIG_ID), pRequest->GetVariableLong(VID_CONFIG_ID_2));
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
                  msg.SetVariable(VID_RCC, RCC_SUCCESS);
               }
               else
               {
                  DBRollback(g_hCoreDB);
                  msg.SetVariable(VID_RCC, RCC_DB_FAILURE);
               }
            }
            else
            {
               msg.SetVariable(VID_RCC, RCC_DB_FAILURE);
            }
         }
         else
         {
            msg.SetVariable(VID_RCC, RCC_INVALID_CONFIG_ID);
         }
         DBFreeResult(hResult);
      }
      else
      {
         msg.SetVariable(VID_RCC, RCC_DB_FAILURE);
      }
   }
   else
   {
      msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
   }

   sendMessage(&msg);
}

/**
 * Send config to agent on request
 */
void ClientSession::sendConfigForAgent(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   TCHAR szPlatform[MAX_DB_STRING], szError[256], szBuffer[256], *pszText;
   WORD wMajor, wMinor, wRelease;
   int i, nNumRows;
   DB_RESULT hResult;
   NXSL_Program *pScript;
   NXSL_Value *ppArgList[5], *pValue;
   UINT32 dwCfgId;

   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   pRequest->GetVariableStr(VID_PLATFORM_NAME, szPlatform, MAX_DB_STRING);
   wMajor = pRequest->GetVariableShort(VID_VERSION_MAJOR);
   wMinor = pRequest->GetVariableShort(VID_VERSION_MINOR);
   wRelease = pRequest->GetVariableShort(VID_VERSION_RELEASE);
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
                  msg.SetVariable(VID_RCC, (WORD)0);
                  pszText = DBGetField(hResult, i, 1, NULL, 0);
                  DecodeSQLStringAndSetVariable(&msg, VID_CONFIG_FILE, pszText);
                  msg.SetVariable(VID_CONFIG_ID, dwCfgId);
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
         msg.SetVariable(VID_RCC, (WORD)1);  // No matching configs found
   }
   else
   {
      msg.SetVariable(VID_RCC, (WORD)1);  // DB Failure
   }

   sendMessage(&msg);
}


//
// Send object comments to client
//

void ClientSession::SendObjectComments(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   NetObj *object;

   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   object = FindObjectById(pRequest->GetVariableLong(VID_OBJECT_ID));
   if (object != NULL)
   {
      if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
      {
         msg.SetVariable(VID_RCC, RCC_SUCCESS);
         object->commentsToMessage(&msg);
      }
      else
      {
         msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
      }
   }
   else
   {
      msg.SetVariable(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   sendMessage(&msg);
}

/**
 * Update object comments from client
 */
void ClientSession::updateObjectComments(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   NetObj *object;

   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   object = FindObjectById(pRequest->GetVariableLong(VID_OBJECT_ID));
   if (object != NULL)
   {
      if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_MODIFY))
      {
         object->setComments(pRequest->GetVariableStr(VID_COMMENTS));
      }
      else
      {
         msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
      }
   }
   else
   {
      msg.SetVariable(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   sendMessage(&msg);
}

/**
 * Push DCI data
 */
void ClientSession::pushDCIData(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   UINT32 i, dwObjectId, dwNumItems, dwId, dwItemId;
   NetObj *object;
   DataCollectionTarget **dcTargetList = NULL;
   DCItem **ppItemList = NULL;
   TCHAR szName[256], **ppValueList;
   BOOL bOK;

   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   dwNumItems = pRequest->GetVariableLong(VID_NUM_ITEMS);
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
         dwObjectId = pRequest->GetVariableLong(dwId++);
         if (dwObjectId != 0)
         {
            object = FindObjectById(dwObjectId);
         }
         else
         {
            pRequest->GetVariableStr(dwId++, szName, 256);
				if (szName[0] == _T('@'))
				{
					UINT32 ipAddr = ntohl(ResolveHostName(&szName[1]));
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
            if ((object->Type() == OBJECT_NODE) || (object->Type() == OBJECT_MOBILEDEVICE))
            {
               if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_PUSH_DATA))
               {
                  dcTargetList[i] = (DataCollectionTarget *)object;

                  // Object OK, find DCI by ID or name (if ID==0)
                  dwItemId = pRequest->GetVariableLong(dwId++);
						DCObject *pItem;
                  if (dwItemId != 0)
                  {
                     pItem = dcTargetList[i]->getDCObjectById(dwItemId);
                  }
                  else
                  {
                     pRequest->GetVariableStr(dwId++, szName, 256);
                     pItem = dcTargetList[i]->getDCObjectByName(szName);
                  }

                  if ((pItem != NULL) && (pItem->getType() == DCO_TYPE_ITEM))
                  {
                     if (pItem->getDataSource() == DS_PUSH_AGENT)
                     {
                        ppItemList[i] = (DCItem *)pItem;
                        ppValueList[i] = pRequest->GetVariableStr(dwId++);
                        bOK = TRUE;
                     }
                     else
                     {
                        msg.SetVariable(VID_RCC, RCC_NOT_PUSH_DCI);
                     }
                  }
                  else
                  {
                     msg.SetVariable(VID_RCC, RCC_INVALID_DCI_ID);
                  }
               }
               else
               {
                  msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
               }
            }
            else
            {
               msg.SetVariable(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
            }
         }
         else
         {
            msg.SetVariable(VID_RCC, RCC_INVALID_OBJECT_ID);
         }
      }

      // If all items was checked OK, push data
      if (bOK)
      {
         time_t t;

         time(&t);
         for(i = 0; i < dwNumItems; i++)
         {
				if (_tcslen(ppValueList[i]) >= MAX_DCI_STRING_VALUE)
					ppValueList[i][MAX_DCI_STRING_VALUE - 1] = 0;
				dcTargetList[i]->processNewDCValue(ppItemList[i], t, ppValueList[i]);
				ppItemList[i]->setLastPollTime(t);
         }
         msg.SetVariable(VID_RCC, RCC_SUCCESS);
      }
      else
      {
         msg.SetVariable(VID_FAILED_DCI_INDEX, i - 1);
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
      msg.SetVariable(VID_RCC, RCC_INVALID_ARGUMENT);
   }

   sendMessage(&msg);
}

/**
 * Get address list
 */
void ClientSession::getAddrList(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   TCHAR szQuery[256];
   UINT32 i, dwNumRec, dwId;
   DB_RESULT hResult;

   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   if (m_dwSystemAccess & SYSTEM_ACCESS_SERVER_CONFIG)
   {
      _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("SELECT addr_type,addr1,addr2 FROM address_lists WHERE list_type=%d"),
                pRequest->GetVariableLong(VID_ADDR_LIST_TYPE));
      hResult = DBSelect(g_hCoreDB, szQuery);
      if (hResult != NULL)
      {
         dwNumRec = DBGetNumRows(hResult);
         msg.SetVariable(VID_NUM_RECORDS, dwNumRec);
         for(i = 0, dwId = VID_ADDR_LIST_BASE; i < dwNumRec; i++, dwId += 7)
         {
            msg.SetVariable(dwId++, DBGetFieldULong(hResult, i, 0));
            msg.SetVariable(dwId++, DBGetFieldIPAddr(hResult, i, 1));
            msg.SetVariable(dwId++, DBGetFieldIPAddr(hResult, i, 2));
         }
         DBFreeResult(hResult);
         msg.SetVariable(VID_RCC, RCC_SUCCESS);
      }
      else
      {
         msg.SetVariable(VID_RCC, RCC_DB_FAILURE);
      }
   }
   else
   {
      msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
   }

   sendMessage(&msg);
}

/**
 * Set address list
 */
void ClientSession::setAddrList(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   UINT32 i, dwId, dwNumRec, dwListType;
   TCHAR szQuery[256], szIpAddr1[24], szIpAddr2[24];

   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   if (m_dwSystemAccess & SYSTEM_ACCESS_SERVER_CONFIG)
   {
      dwListType = pRequest->GetVariableLong(VID_ADDR_LIST_TYPE);
      _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("DELETE FROM address_lists WHERE list_type=%d"), dwListType);
      DBBegin(g_hCoreDB);
      if (DBQuery(g_hCoreDB, szQuery))
      {
         dwNumRec = pRequest->GetVariableLong(VID_NUM_RECORDS);
         for(i = 0, dwId = VID_ADDR_LIST_BASE; i < dwNumRec; i++, dwId += 10)
         {
            _sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("INSERT INTO address_lists (list_type,addr_type,addr1,addr2,community_id) VALUES (%d,%d,'%s','%s',0)"),
                      dwListType, pRequest->GetVariableLong(dwId),
                      IpToStr(pRequest->GetVariableLong(dwId + 1), szIpAddr1),
                      IpToStr(pRequest->GetVariableLong(dwId + 2), szIpAddr2));
            if (!DBQuery(g_hCoreDB, szQuery))
               break;
         }

         if (i == dwNumRec)
         {
            DBCommit(g_hCoreDB);
            msg.SetVariable(VID_RCC, RCC_SUCCESS);
         }
         else
         {
            DBRollback(g_hCoreDB);
            msg.SetVariable(VID_RCC, RCC_DB_FAILURE);
         }
      }
      else
      {
         DBRollback(g_hCoreDB);
         msg.SetVariable(VID_RCC, RCC_DB_FAILURE);
      }
   }
   else
   {
      msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
   }

   sendMessage(&msg);
}

/**
 * Reset server component
 */
void ClientSession::resetComponent(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   UINT32 dwCode;

   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   if (m_dwSystemAccess & SYSTEM_ACCESS_SERVER_CONFIG)
   {
      dwCode = pRequest->GetVariableLong(VID_COMPONENT_ID);
      switch(dwCode)
      {
         case SRV_COMPONENT_DISCOVERY_MGR:
            ResetDiscoveryPoller();
            msg.SetVariable(VID_RCC, RCC_SUCCESS);
            break;
         default:
            msg.SetVariable(VID_RCC, RCC_INVALID_ARGUMENT);
            break;
      }
   }
   else
   {
      msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
   }

   sendMessage(&msg);
}


//
// Send list of events used by template's or node's DCIs
//

void ClientSession::sendDCIEventList(CSCPMessage *request)
{
   CSCPMessage msg;
   NetObj *object;
   UINT32 *pdwEventList, dwCount;

   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(request->GetId());

   object = FindObjectById(request->GetVariableLong(VID_OBJECT_ID));
   if (object != NULL)
   {
      if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
      {
         if ((object->Type() == OBJECT_NODE) ||
             (object->Type() == OBJECT_CLUSTER) ||
             (object->Type() == OBJECT_TEMPLATE))
         {
            pdwEventList = ((Template *)object)->getDCIEventsList(&dwCount);
            if (pdwEventList != NULL)
            {
               msg.SetVariable(VID_NUM_EVENTS, dwCount);
               msg.setFieldInt32Array(VID_EVENT_LIST, dwCount, pdwEventList);
               free(pdwEventList);
            }
            else
            {
               msg.SetVariable(VID_NUM_EVENTS, (UINT32)0);
            }
            msg.SetVariable(VID_RCC, RCC_SUCCESS);
         }
         else
         {
            msg.SetVariable(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
         }
      }
      else
      {
         msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
      }
   }
   else
   {
      msg.SetVariable(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   sendMessage(&msg);
}

/**
 * Export server configuration (event, templates, etc.)
 */
void ClientSession::exportConfiguration(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   UINT32 i, dwCount, dwNumTemplates;
   UINT32 *pdwList, *pdwTemplateList;
   NetObj *object;

   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   if (checkSysAccessRights(SYSTEM_ACCESS_CONFIGURE_TRAPS | SYSTEM_ACCESS_VIEW_EVENT_DB | SYSTEM_ACCESS_EPP))
   {
      dwNumTemplates = pRequest->GetVariableLong(VID_NUM_OBJECTS);
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
            if (object->Type() == OBJECT_TEMPLATE)
            {
               if (!object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
               {
                  msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
                  break;
               }
            }
            else
            {
               msg.SetVariable(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
               break;
            }
         }
         else
         {
            msg.SetVariable(VID_RCC, RCC_INVALID_OBJECT_ID);
            break;
         }
      }

      if (i == dwNumTemplates)   // All objects passed test
      {
         String str;
			TCHAR *temp;

         str = _T("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<configuration>\n\t<formatVersion>3</formatVersion>\n\t<description>");
			temp = pRequest->GetVariableStr(VID_DESCRIPTION);
			str.addDynamicString(EscapeStringForXML(temp, -1));
			free(temp);
         str += _T("</description>\n");

         // Write events
         str += _T("\t<events>\n");
         dwCount = pRequest->GetVariableLong(VID_NUM_EVENTS);
         pdwList = (UINT32 *)malloc(sizeof(UINT32) * dwCount);
         pRequest->getFieldAsInt32Array(VID_EVENT_LIST, dwCount, pdwList);
         for(i = 0; i < dwCount; i++)
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
         dwCount = pRequest->GetVariableLong(VID_NUM_TRAPS);
         pdwList = (UINT32 *)malloc(sizeof(UINT32) * dwCount);
         pRequest->getFieldAsInt32Array(VID_TRAP_LIST, dwCount, pdwList);
         for(i = 0; i < dwCount; i++)
            CreateNXMPTrapRecord(str, pdwList[i]);
         safe_free(pdwList);
         str += _T("\t</traps>\n");

         // Write rules
         str += _T("\t<rules>\n");
         dwCount = pRequest->GetVariableLong(VID_NUM_RULES);
         DWORD varId = VID_RULE_LIST_BASE;
         uuid_t guid;
         for(i = 0; i < dwCount; i++)
         {
            pRequest->GetVariableBinary(varId++, guid, UUID_LENGTH);
            g_pEventPolicy->exportRule(str, guid);
         }
         str += _T("\t</rules>\n");

			// Close document
			str += _T("</configuration>\n");

         // Put result into message
         msg.SetVariable(VID_RCC, RCC_SUCCESS);
         msg.SetVariable(VID_NXMP_CONTENT, (const TCHAR *)str);
      }

      safe_free(pdwTemplateList);
   }
   else
   {
      msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
   }

   sendMessage(&msg);
}

/**
 * Import server configuration (events, templates, etc.)
 */
void ClientSession::importConfiguration(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   TCHAR szLockInfo[MAX_SESSION_NAME], szError[1024];
   UINT32 dwFlags;

   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   if (checkSysAccessRights(SYSTEM_ACCESS_CONFIGURE_TRAPS | SYSTEM_ACCESS_EDIT_EVENT_DB | SYSTEM_ACCESS_EPP))
   {
      char *content = pRequest->GetVariableStrUTF8(VID_NXMP_CONTENT);
      if (content != NULL)
      {
			Config *config = new Config();
         if (config->loadXmlConfigFromMemory(content, (int)strlen(content), NULL, "configuration"))
         {
            // Lock all required components
            if (LockComponent(CID_EPP, m_dwIndex, m_szUserName, NULL, szLockInfo))
            {
               m_dwFlags |= CSF_EPP_LOCKED;

               // Validate and import configuration
               dwFlags = pRequest->GetVariableLong(VID_FLAGS);
               if (ValidateConfig(config, dwFlags, szError, 1024))
               {
                  msg.SetVariable(VID_RCC, ImportConfig(config, dwFlags));
               }
               else
               {
                  msg.SetVariable(VID_RCC, RCC_CONFIG_VALIDATION_ERROR);
                  msg.SetVariable(VID_ERROR_TEXT, szError);
               }

					UnlockComponent(CID_EPP);
               m_dwFlags &= ~CSF_EPP_LOCKED;
            }
            else
            {
               msg.SetVariable(VID_RCC, RCC_COMPONENT_LOCKED);
               msg.SetVariable(VID_COMPONENT, (WORD)NXMP_LC_EPP);
               msg.SetVariable(VID_LOCKED_BY, szLockInfo);
            }
         }
         else
         {
            msg.SetVariable(VID_RCC, RCC_CONFIG_PARSE_ERROR);
         }
			delete config;
         free(content);
      }
      else
      {
         msg.SetVariable(VID_RCC, RCC_INVALID_ARGUMENT);
      }
   }
   else
   {
      msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
   }

   sendMessage(&msg);
}

/**
 * Send basic DCI info to client
 */
void ClientSession::SendDCIInfo(CSCPMessage *pRequest)
{
   CSCPMessage msg;

   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   NetObj *object = FindObjectById(pRequest->GetVariableLong(VID_OBJECT_ID));
   if (object != NULL)
   {
      if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
      {
         if ((object->Type() == OBJECT_NODE) ||
             (object->Type() == OBJECT_CLUSTER) ||
             (object->Type() == OBJECT_TEMPLATE))
         {
				DCObject *pItem = ((Template *)object)->getDCObjectById(pRequest->GetVariableLong(VID_DCI_ID));
				if ((pItem != NULL) && (pItem->getType() == DCO_TYPE_ITEM))
				{
					msg.SetVariable(VID_TEMPLATE_ID, pItem->getTemplateId());
					msg.SetVariable(VID_RESOURCE_ID, pItem->getResourceId());
					msg.SetVariable(VID_DCI_DATA_TYPE, (WORD)((DCItem *)pItem)->getDataType());
					msg.SetVariable(VID_DCI_SOURCE_TYPE, (WORD)pItem->getDataSource());
					msg.SetVariable(VID_NAME, pItem->getName());
					msg.SetVariable(VID_DESCRIPTION, pItem->getDescription());
	            msg.SetVariable(VID_RCC, RCC_SUCCESS);
				}
				else
				{
			      msg.SetVariable(VID_RCC, RCC_INVALID_DCI_ID);
				}
         }
         else
         {
            msg.SetVariable(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
         }
      }
      else
      {
         msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
      }
   }
   else
   {
      msg.SetVariable(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   sendMessage(&msg);
}

/**
 * Send list of available graphs to client
 */
void ClientSession::sendGraphList(UINT32 dwRqId)
{
   CSCPMessage msg;
	DB_RESULT hResult;
	GRAPH_ACL_ENTRY *pACL = NULL;
	int i, j, nRows, nACLSize;
	UINT32 dwId, dwNumGraphs, dwGraphId, dwOwner;
	UINT32 *pdwUsers, *pdwRights, dwGraphACLSize;
	TCHAR *pszStr;

   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(dwRqId);

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
					msg.SetVariable(dwId++, dwGraphId);
					msg.SetVariable(dwId++, dwOwner);
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
					msg.SetVariable(dwId++, dwGraphACLSize);
					msg.setFieldInt32Array(dwId++, dwGraphACLSize, pdwUsers);
					msg.setFieldInt32Array(dwId++, dwGraphACLSize, pdwRights);

					dwId += 3;
					dwNumGraphs++;
				}
			}
			DBFreeResult(hResult);
			free(pdwUsers);
			free(pdwRights);
			msg.SetVariable(VID_NUM_GRAPHS, dwNumGraphs);
			msg.SetVariable(VID_RCC, RCC_SUCCESS);
		}
		else
		{
			msg.SetVariable(VID_RCC, RCC_DB_FAILURE);
		}
		safe_free(pACL);
	}
	else
	{
		msg.SetVariable(VID_RCC, RCC_DB_FAILURE);
	}

   DBConnectionPoolReleaseConnection(hdb);
   sendMessage(&msg);
}

/**
 * Save graph
 */
void ClientSession::saveGraph(CSCPMessage *pRequest)
{
   CSCPMessage msg;
	BOOL bNew, bSuccess;
	UINT32 id, graphId, graphUserId, graphAccess, accessRightStatus;
	UINT16 overwrite;
	TCHAR szQuery[16384], *pszEscName, *pszEscData, *pszTemp, dwGraphName[255];
	int i, nACLSize;

   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

	graphId = pRequest->GetVariableLong(VID_GRAPH_ID);
	pRequest->GetVariableStr(VID_NAME,dwGraphName,255);
	overwrite = pRequest->GetVariableShort(VID_FLAGS);

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
      msg.SetVariable(VID_RCC, accessRightStatus ? accessRightStatus : nameUniq.status );
   }

	// Create/update graph
	if (bSuccess)
	{
		debugPrintf(5, _T("%s graph %d"), bNew ? _T("Creating") : _T("Updating"), graphId);
		bSuccess = FALSE;
      DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
		if (DBBegin(hdb))
		{
			pRequest->GetVariableStr(VID_NAME, szQuery, 256);
			pszEscName = EncodeSQLString(szQuery);
			pszTemp = pRequest->GetVariableStr(VID_GRAPH_CONFIG);
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
			free(pszEscName);
			free(pszEscData);

			if (DBQuery(hdb, szQuery))
			{
				// Insert new ACL
				nACLSize = (int)pRequest->GetVariableLong(VID_ACL_SIZE);
				for(i = 0, id = VID_GRAPH_ACL_BASE, bSuccess = TRUE; i < nACLSize; i++)
				{
					graphUserId = pRequest->GetVariableLong(id++);
					graphAccess = pRequest->GetVariableLong(id++);
					_sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("INSERT INTO graph_acl (graph_id,user_id,user_rights) VALUES (%d,%d,%d)"),
					          graphId, graphUserId, graphAccess);
					if (!DBQuery(hdb, szQuery))
					{
						bSuccess = FALSE;
						msg.SetVariable(VID_RCC, RCC_DB_FAILURE);
						break;
					}
				}
			}
			else
			{
				msg.SetVariable(VID_RCC, RCC_DB_FAILURE);
			}

			if (bSuccess)
			{
				DBCommit(hdb);
				msg.SetVariable(VID_RCC, RCC_SUCCESS);
				msg.SetVariable(VID_GRAPH_ID, graphId);
				notify(NX_NOTIFY_GRAPHS_CHANGED);
			}
			else
			{
				DBRollback(hdb);
			}
		}
		else
		{
			msg.SetVariable(VID_RCC, RCC_DB_FAILURE);
		}
      DBConnectionPoolReleaseConnection(hdb);
	}

   sendMessage(&msg);
}

/**
 * Delete graph
 */
void ClientSession::deleteGraph(CSCPMessage *pRequest)
{
   CSCPMessage msg;
	UINT32 dwGraphId, dwOwner;
	GRAPH_ACL_ENTRY *pACL = NULL;
	int nACLSize;
	DB_RESULT hResult;
	TCHAR szQuery[256];

   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

	dwGraphId = pRequest->GetVariableLong(VID_GRAPH_ID);
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
						msg.SetVariable(VID_RCC, RCC_SUCCESS);
						notify(NX_NOTIFY_GRAPHS_CHANGED);
					}
					else
					{
						msg.SetVariable(VID_RCC, RCC_DB_FAILURE);
					}
				}
				else
				{
					msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
				}
				safe_free(pACL);
			}
			else
			{
				msg.SetVariable(VID_RCC, RCC_DB_FAILURE);
			}
		}
		else
		{
			msg.SetVariable(VID_RCC, RCC_INVALID_GRAPH_ID);
		}
		DBFreeResult(hResult);
	}
	else
	{
		msg.SetVariable(VID_RCC, RCC_DB_FAILURE);
	}

   DBConnectionPoolReleaseConnection(hdb);
   sendMessage(&msg);
}

/**
 * Send list of DCIs to be shown in performance tab
 */
void ClientSession::sendPerfTabDCIList(CSCPMessage *pRequest)
{
   CSCPMessage msg;
	NetObj *object;

	msg.SetId(pRequest->GetId());
	msg.SetCode(CMD_REQUEST_COMPLETED);

	object = FindObjectById(pRequest->GetVariableLong(VID_OBJECT_ID));
	if (object != NULL)
	{
		if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
		{
			if (object->Type() == OBJECT_NODE)
			{
				msg.SetVariable(VID_RCC, ((Node *)object)->getPerfTabDCIList(&msg));
			}
			else
			{
				msg.SetVariable(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
			}
		}
		else
		{
			msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
		}
	}
	else
	{
		msg.SetVariable(VID_RCC, RCC_INVALID_OBJECT_ID);
	}

   sendMessage(&msg);
}


//
// Add CA certificate
//

void ClientSession::AddCACertificate(CSCPMessage *pRequest)
{
   CSCPMessage msg;
#ifdef _WITH_ENCRYPTION
	UINT32 dwLen, dwQLen, dwCertId;
	BYTE *pData;
	OPENSSL_CONST BYTE *p;
	X509 *pCert;
	TCHAR *pszQuery, *pszEscSubject, *pszComments, *pszEscComments;
#endif

	msg.SetId(pRequest->GetId());
	msg.SetCode(CMD_REQUEST_COMPLETED);

	if ((m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_USERS) &&
	    (m_dwSystemAccess & SYSTEM_ACCESS_SERVER_CONFIG))
	{
#ifdef _WITH_ENCRYPTION
		dwLen = pRequest->GetVariableBinary(VID_CERTIFICATE, NULL, 0);
		if (dwLen > 0)
		{
			pData = (BYTE *)malloc(dwLen);
			pRequest->GetVariableBinary(VID_CERTIFICATE, pData, dwLen);

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
				pszComments = pRequest->GetVariableStr(VID_COMMENTS);
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
					msg.SetVariable(VID_RCC, RCC_SUCCESS);
					ReloadCertificates();
				}
				else
				{
					msg.SetVariable(VID_RCC, RCC_DB_FAILURE);
				}
				free(pszQuery);
			}
			else
			{
				msg.SetVariable(VID_RCC, RCC_BAD_CERTIFICATE);
			}
			free(pData);
		}
		else
		{
			msg.SetVariable(VID_RCC, RCC_INVALID_REQUEST);
		}
#else
		msg.SetVariable(VID_RCC, RCC_NO_ENCRYPTION_SUPPORT);
#endif
	}
	else
	{
		msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
	}

	sendMessage(&msg);
}


//
// Delete certificate
//

void ClientSession::DeleteCertificate(CSCPMessage *pRequest)
{
   CSCPMessage msg;
#ifdef _WITH_ENCRYPTION
	UINT32 dwCertId;
	TCHAR szQuery[256];
#endif

	msg.SetId(pRequest->GetId());
	msg.SetCode(CMD_REQUEST_COMPLETED);

	if ((m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_USERS) &&
	    (m_dwSystemAccess & SYSTEM_ACCESS_SERVER_CONFIG))
	{
#ifdef _WITH_ENCRYPTION
		dwCertId = pRequest->GetVariableLong(VID_CERTIFICATE_ID);
		_sntprintf(szQuery, sizeof(szQuery) / sizeof(TCHAR), _T("DELETE FROM certificates WHERE cert_id=%d"), dwCertId);
		if (DBQuery(g_hCoreDB, szQuery))
		{
			msg.SetVariable(VID_RCC, RCC_SUCCESS);
			NotifyClientSessions(NX_NOTIFY_CERTIFICATE_CHANGED, dwCertId);
			ReloadCertificates();
		}
		else
		{
			msg.SetVariable(VID_RCC, RCC_DB_FAILURE);
		}
#else
		msg.SetVariable(VID_RCC, RCC_NO_ENCRYPTION_SUPPORT);
#endif
	}
	else
	{
		msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
	}

	sendMessage(&msg);
}


//
// Update certificate's comments
//

void ClientSession::UpdateCertificateComments(CSCPMessage *pRequest)
{
	UINT32 dwCertId, qlen;
	TCHAR *pszQuery, *pszComments, *pszEscComments;
	DB_RESULT hResult;
   CSCPMessage msg;

	msg.SetId(pRequest->GetId());
	msg.SetCode(CMD_REQUEST_COMPLETED);

	if ((m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_USERS) &&
	    (m_dwSystemAccess & SYSTEM_ACCESS_SERVER_CONFIG))
	{
		dwCertId = pRequest->GetVariableLong(VID_CERTIFICATE_ID);
		pszComments= pRequest->GetVariableStr(VID_COMMENTS);
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
						msg.SetVariable(VID_RCC, RCC_SUCCESS);
					}
					else
					{
						msg.SetVariable(VID_RCC, RCC_DB_FAILURE);
					}
				}
				else
				{
					msg.SetVariable(VID_RCC, RCC_INVALID_CERT_ID);
				}
				DBFreeResult(hResult);
			}
			else
			{
				msg.SetVariable(VID_RCC, RCC_DB_FAILURE);
			}
			free(pszEscComments);
			free(pszQuery);
		}
		else
		{
			msg.SetVariable(VID_RCC, RCC_INVALID_REQUEST);
		}
	}
	else
	{
		msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
	}

	sendMessage(&msg);
}

/**
 * Send list of installed certificates to client
 */
void ClientSession::getCertificateList(UINT32 dwRqId)
{
   CSCPMessage msg;
	DB_RESULT hResult;
	int i, nRows;
	UINT32 dwId;
	TCHAR *pszText;

	msg.SetId(dwRqId);
	msg.SetCode(CMD_REQUEST_COMPLETED);

	if ((m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_USERS) &&
	    (m_dwSystemAccess & SYSTEM_ACCESS_SERVER_CONFIG))
	{
		hResult = DBSelect(g_hCoreDB, _T("SELECT cert_id,cert_type,comments,subject FROM certificates"));
		if (hResult != NULL)
		{
			nRows = DBGetNumRows(hResult);
			msg.SetVariable(VID_RCC, RCC_SUCCESS);
			msg.SetVariable(VID_NUM_CERTIFICATES, (UINT32)nRows);
			for(i = 0, dwId = VID_CERT_LIST_BASE; i < nRows; i++, dwId += 6)
			{
				msg.SetVariable(dwId++, DBGetFieldULong(hResult, i, 0));
				msg.SetVariable(dwId++, (WORD)DBGetFieldLong(hResult, i, 1));

				pszText = DBGetField(hResult, i, 2, NULL, 0);
				if (pszText != NULL)
				{
					DecodeSQLString(pszText);
					msg.SetVariable(dwId++, pszText);
				}
				else
				{
					msg.SetVariable(dwId++, _T(""));
				}

				pszText = DBGetField(hResult, i, 3, NULL, 0);
				if (pszText != NULL)
				{
					DecodeSQLString(pszText);
					msg.SetVariable(dwId++, pszText);
				}
				else
				{
					msg.SetVariable(dwId++, _T(""));
				}
			}
			DBFreeResult(hResult);
		}
		else
		{
			msg.SetVariable(VID_RCC, RCC_DB_FAILURE);
		}
	}
	else
	{
		msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
	}

	sendMessage(&msg);
}

/**
 * Query layer 2 topology from device
 */
void ClientSession::queryL2Topology(CSCPMessage *pRequest)
{
   CSCPMessage msg;
	NetObj *object;
	UINT32 dwResult;

	msg.SetId(pRequest->GetId());
	msg.SetCode(CMD_REQUEST_COMPLETED);

	object = FindObjectById(pRequest->GetVariableLong(VID_OBJECT_ID));
	if (object != NULL)
	{
		if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
		{
			if (object->Type() == OBJECT_NODE)
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
					msg.SetVariable(VID_RCC, RCC_SUCCESS);
					pTopology->createMessage(&msg);
					delete pTopology;
				}
				else
				{
					msg.SetVariable(VID_RCC, dwResult);
				}
			}
			else
			{
				msg.SetVariable(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
			}
		}
		else
		{
			msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
		}
	}
	else
	{
		msg.SetVariable(VID_RCC, RCC_INVALID_OBJECT_ID);
	}

	sendMessage(&msg);
}

/**
 * Send SMS
 */
void ClientSession::sendSMS(CSCPMessage *pRequest)
{
   CSCPMessage msg;
	TCHAR phone[256], message[256];

	msg.SetId(pRequest->GetId());
	msg.SetCode(CMD_REQUEST_COMPLETED);

	if ((m_dwSystemAccess & SYSTEM_ACCESS_SEND_SMS) && ConfigReadInt(_T("AllowDirectSMS"), 0))
	{
		pRequest->GetVariableStr(VID_RCPT_ADDR, phone, 256);
		pRequest->GetVariableStr(VID_MESSAGE, message, 256);
		PostSMS(phone, message);
		msg.SetVariable(VID_RCC, RCC_SUCCESS);
	}
	else
	{
		msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
	}

	sendMessage(&msg);
}

/**
 * Send SNMP community list
 */
void ClientSession::SendCommunityList(UINT32 dwRqId)
{
   CSCPMessage msg;
	int i, count;
	UINT32 id;
	TCHAR buffer[256];
	DB_RESULT hResult;

	msg.SetId(dwRqId);
	msg.SetCode(CMD_REQUEST_COMPLETED);

	if (m_dwSystemAccess & SYSTEM_ACCESS_SERVER_CONFIG)
	{
      DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
		hResult = DBSelect(hdb, _T("SELECT community FROM snmp_communities"));
		if (hResult != NULL)
		{
			count = DBGetNumRows(hResult);
			msg.SetVariable(VID_NUM_STRINGS, (UINT32)count);
			for(i = 0, id = VID_STRING_LIST_BASE; i < count; i++)
			{
				DBGetField(hResult, i, 0, buffer, 256);
				msg.SetVariable(id++, buffer);
			}
			DBFreeResult(hResult);
			msg.SetVariable(VID_RCC, RCC_SUCCESS);
		}
		else
		{
			msg.SetVariable(VID_RCC, RCC_DB_FAILURE);
		}
      DBConnectionPoolReleaseConnection(hdb);
	}
	else
	{
		msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
	}

	sendMessage(&msg);
}

/**
 * Update SNMP community list
 */
void ClientSession::UpdateCommunityList(CSCPMessage *pRequest)
{
   CSCPMessage msg;
	TCHAR value[256], query[1024];
	int i, count;
	UINT32 id;

	msg.SetId(pRequest->GetId());
	msg.SetCode(CMD_REQUEST_COMPLETED);

	if (m_dwSystemAccess & SYSTEM_ACCESS_SERVER_CONFIG)
	{
      DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
		if (DBBegin(hdb))
		{
			DBQuery(hdb, _T("DELETE FROM snmp_communities"));
			count = pRequest->GetVariableLong(VID_NUM_STRINGS);
			for(i = 0, id = VID_STRING_LIST_BASE; i < count; i++)
			{
				pRequest->GetVariableStr(id++, value, 256);
				String escValue = DBPrepareString(hdb, value);
				_sntprintf(query, 1024, _T("INSERT INTO snmp_communities (id,community) VALUES(%d,%s)"), i + 1, (const TCHAR *)escValue);
				if (!DBQuery(hdb, query))
					break;
			}

			if (i == count)
			{
				DBCommit(hdb);
				msg.SetVariable(VID_RCC, RCC_SUCCESS);
			}
			else
			{
				DBRollback(hdb);
				msg.SetVariable(VID_RCC, RCC_DB_FAILURE);
			}
		}
		else
		{
			msg.SetVariable(VID_RCC, RCC_DB_FAILURE);
		}
      DBConnectionPoolReleaseConnection(hdb);
	}
	else
	{
		msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
	}

	sendMessage(&msg);
}

/**
 * Send situation list to client
 */
void ClientSession::getSituationList(UINT32 dwRqId)
{
   CSCPMessage msg;

	msg.SetId(dwRqId);
	msg.SetCode(CMD_REQUEST_COMPLETED);

	if (m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_SITUATIONS)
	{
		MutexLock(m_mutexSendSituations);
		SendSituationListToClient(this, &msg);
		MutexUnlock(m_mutexSendSituations);
	}
	else
	{
		msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
		sendMessage(&msg);
	}
}

/**
 * Create new situation
 */
void ClientSession::createSituation(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   Situation *st;
   TCHAR name[MAX_DB_STRING];

	msg.SetId(pRequest->GetId());
	msg.SetCode(CMD_REQUEST_COMPLETED);

	if (m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_SITUATIONS)
	{
		pRequest->GetVariableStr(VID_NAME, name, MAX_DB_STRING);
		st = ::CreateSituation(name);
		if (st != NULL)
		{
			msg.SetVariable(VID_RCC, RCC_SUCCESS);
			msg.SetVariable(VID_SITUATION_ID, st->GetId());
		}
		else
		{
			msg.SetVariable(VID_RCC, RCC_INTERNAL_ERROR);
		}
	}
	else
	{
		msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
	}

	sendMessage(&msg);
}

/**
 * Update situation
 */
void ClientSession::updateSituation(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   Situation *st;

	msg.SetId(pRequest->GetId());
	msg.SetCode(CMD_REQUEST_COMPLETED);

	if (m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_SITUATIONS)
	{
		st = FindSituationById(pRequest->GetVariableLong(VID_SITUATION_ID));
		if (st != NULL)
		{
			st->UpdateFromMessage(pRequest);
			msg.SetVariable(VID_RCC, RCC_SUCCESS);
		}
		else
		{
			msg.SetVariable(VID_RCC, RCC_INVALID_SITUATION_ID);
		}
	}
	else
	{
		msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
	}

	sendMessage(&msg);
}

/**
 * Delete situation
 */
void ClientSession::deleteSituation(CSCPMessage *pRequest)
{
   CSCPMessage msg;

	msg.SetId(pRequest->GetId());
	msg.SetCode(CMD_REQUEST_COMPLETED);

	if (m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_SITUATIONS)
	{
		msg.SetVariable(VID_RCC, ::DeleteSituation(pRequest->GetVariableLong(VID_SITUATION_ID)));
	}
	else
	{
		msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
	}

	sendMessage(&msg);
}

/**
 * Delete situation instance
 */
void ClientSession::deleteSituationInstance(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   Situation *st;

	msg.SetId(pRequest->GetId());
	msg.SetCode(CMD_REQUEST_COMPLETED);

	if (m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_SITUATIONS)
	{
		st = FindSituationById(pRequest->GetVariableLong(VID_SITUATION_ID));
		if (st != NULL)
		{
			TCHAR instance[MAX_DB_STRING];

			pRequest->GetVariableStr(VID_SITUATION_INSTANCE, instance, MAX_DB_STRING);
			msg.SetVariable(VID_RCC, st->DeleteInstance(instance) ? RCC_SUCCESS : RCC_INSTANCE_NOT_FOUND);
		}
		else
		{
			msg.SetVariable(VID_RCC, RCC_INVALID_SITUATION_ID);
		}
	}
	else
	{
		msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
	}

	sendMessage(&msg);
}

/**
 * Handler for situation chage
 */
void ClientSession::onSituationChange(CSCPMessage *msg)
{
   UPDATE_INFO *pUpdate;

   if (isAuthenticated() && (m_dwActiveChannels & NXC_CHANNEL_SITUATIONS))
   {
      pUpdate = (UPDATE_INFO *)malloc(sizeof(UPDATE_INFO));
      pUpdate->dwCategory = INFO_CAT_SITUATION;
      pUpdate->pData = new CSCPMessage(msg);
      m_pUpdateQueue->Put(pUpdate);
   }
}

/**
 * Register agent
 */
void ClientSession::registerAgent(CSCPMessage *pRequest)
{
   CSCPMessage msg;
	Node *node;

   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

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
				info->dwIpAddr = ntohl(((struct sockaddr_in *)m_clientAddr)->sin_addr.s_addr);
				info->dwNetMask = 0;
				info->zoneId = 0;	// Add to default zone
				info->ignoreFilter = TRUE;		// Ignore discovery filters and add node anyway
				g_nodePollerQueue.Put(info);
			}
			msg.SetVariable(VID_RCC, RCC_SUCCESS);
		}
		else
		{
			msg.SetVariable(VID_RCC, RCC_NOT_IMPLEMENTED);
		}
	}
	else
	{
      msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
	}

   sendMessage(&msg);
}

/**
 * Get file from server
 */
void ClientSession::getServerFile(CSCPMessage *pRequest)
{
   CSCPMessage msg;
	TCHAR name[MAX_PATH], fname[MAX_PATH];
	bool musicFile = false;

   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());
   pRequest->GetVariableStr(VID_FILE_NAME, name, MAX_PATH);
   for(int i = 0; i < m_musicTypeList.getSize(); i++)
   {
      TCHAR *extension = _tcsrchr(name, _T('.'));
      if (extension != NULL)
      {
         extension++;
         if(!_tcscmp(extension, m_musicTypeList.getValue(i)))
         {
            musicFile = true;
            break;
         }
      }
   }

	if ((m_dwSystemAccess & SYSTEM_ACCESS_READ_FILES) || musicFile)
	{
      _tcscpy(fname, g_szDataDir);
      _tcscat(fname, DDIR_FILES);
      _tcscat(fname, FS_PATH_SEPARATOR);
      _tcscat(fname, GetCleanFileName(name));
		debugPrintf(4, _T("Requested file %s"), fname);
		if (_taccess(fname, 0) == 0)
		{
			debugPrintf(5, _T("Sending file %s"), fname);
			if (SendFileOverNXCP(m_hSocket, pRequest->GetId(), fname, m_pCtx, 0, NULL, NULL, m_mutexSocketWrite))
			{
				debugPrintf(5, _T("File %s was succesfully sent"), fname);
		      msg.SetVariable(VID_RCC, RCC_SUCCESS);
			}
			else
			{
				debugPrintf(5, _T("Unable to send file %s: SendFileOverNXCP() failed"), fname);
		      msg.SetVariable(VID_RCC, RCC_IO_ERROR);
			}
		}
		else
		{
			debugPrintf(5, _T("Unable to send file %s: access() failed"), fname);
	      msg.SetVariable(VID_RCC, RCC_IO_ERROR);
		}
	}
	else
	{
      msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
	}

   sendMessage(&msg);
}

/**
 * Get file from agent
 */
void ClientSession::getAgentFile(CSCPMessage *request)
{
   CSCPMessage msg;
	TCHAR remoteFile[MAX_PATH];

   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(request->GetId());

	NetObj *object = FindObjectById(request->GetVariableLong(VID_OBJECT_ID));
	if (object != NULL)
	{
		if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_CONTROL))
		{
			if (object->Type() == OBJECT_NODE)
			{
				request->GetVariableStr(VID_FILE_NAME, remoteFile, MAX_PATH);
            bool follow = request->GetVariableShort(VID_FILE_FOLLOW) ? true : false;
				FileDownloadJob *job = new FileDownloadJob((Node *)object, remoteFile, request->GetVariableLong(VID_FILE_SIZE_LIMIT), follow, this, request->GetId());
				msg.SetVariable(VID_NAME, job->getLocalFileName());
				msg.SetVariable(VID_RCC, AddJob(job) ? RCC_SUCCESS : RCC_INTERNAL_ERROR);
			}
			else
			{
				msg.SetVariable(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
			}
		}
		else
		{
			msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
		}
	}
	else
	{
		msg.SetVariable(VID_RCC, RCC_INVALID_OBJECT_ID);
	}

   sendMessage(&msg);
}

/**
 * Cancel file monitoring
 */
void ClientSession::cancelFileMonitoring(CSCPMessage *request)
{
   CSCPMessage msg;
   CSCPMessage* response;
	TCHAR remoteFile[MAX_PATH];
	UINT32 rcc = 0xFFFFFFFF;

   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(request->GetId());

	NetObj *object = FindObjectById(request->GetVariableLong(VID_OBJECT_ID));
	if (object != NULL)
	{
      if (object->Type() == OBJECT_NODE)
      {
         request->GetVariableStr(VID_FILE_NAME, remoteFile, MAX_PATH);

         MONITORED_FILE * newFile = new MONITORED_FILE();
         _tcscpy(newFile->fileName, remoteFile);
         newFile->nodeID = object->Id();
         newFile->session = this;
         g_monitoringList.removeMonitoringFile(newFile);
         delete newFile;

         Node *node = (Node *)object;
         node->incRefCount();
         AgentConnection *conn = node->createAgentConnection();
         debugPrintf(6, _T("Cancel file monitoring %s"), remoteFile);
         if(conn != NULL)
         {
            request->SetId(conn->generateRequestId());
            response = conn->customRequest(request);
            if (response != NULL)
            {
               rcc = response->GetVariableLong(VID_RCC);
               if(rcc == RCC_SUCCESS)
               {
                  msg.SetVariable(VID_RCC, rcc);
                  debugPrintf(6, _T("File monitoring cancelled sucessfully"));
               }
               else
               {
                  msg.SetVariable(VID_RCC, RCC_INTERNAL_ERROR);

                  debugPrintf(6, _T("Error on agent: %d"), rcc);
               }
            }
            else
            {
               msg.SetVariable(VID_RCC, RCC_INTERNAL_ERROR);
            }
            delete response;
            delete conn;
         }
         else
         {
            msg.SetVariable(VID_RCC, RCC_INTERNAL_ERROR);
            debugPrintf(6, _T("Connection with node have been lost"));
         }
         node->decRefCount();
      }
      else
      {
         msg.SetVariable(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
      }
	}
	else
	{
		msg.SetVariable(VID_RCC, RCC_INVALID_OBJECT_ID);
	}
   sendMessage(&msg);
}

/**
 * Test DCI transformation script
 */
void ClientSession::testDCITransformation(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   NetObj *object;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   // Get node id and check object class and access rights
   object = FindObjectById(pRequest->GetVariableLong(VID_OBJECT_ID));
   if (object != NULL)
   {
      if (object->Type() == OBJECT_NODE)
      {
         if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
         {
				BOOL success;
				TCHAR *script, value[256], result[256];

				script = pRequest->GetVariableStr(VID_SCRIPT);
				if (script != NULL)
				{
					pRequest->GetVariableStr(VID_VALUE, value, sizeof(value) / sizeof(TCHAR));
               success = DCItem::testTransformation((Node *)object, script, value, result, sizeof(result) / sizeof(TCHAR));
					free(script);
					msg.SetVariable(VID_RCC, RCC_SUCCESS);
					msg.SetVariable(VID_EXECUTION_STATUS, (WORD)success);
					msg.SetVariable(VID_EXECUTION_RESULT, result);
				}
				else
				{
	            msg.SetVariable(VID_RCC, RCC_INVALID_ARGUMENT);
				}
         }
         else  // User doesn't have READ rights on object
         {
            msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
         }
      }
      else     // Object is not a node
      {
         msg.SetVariable(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
      }
   }
   else  // No object with given ID
   {
      msg.SetVariable(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Send list of server jobs
 */
void ClientSession::sendJobList(UINT32 dwRqId)
{
	CSCPMessage msg;

	msg.SetCode(CMD_REQUEST_COMPLETED);
	msg.SetId(dwRqId);
	msg.SetVariable(VID_RCC, RCC_SUCCESS);
	GetJobList(&msg);
	sendMessage(&msg);
}

/**
 * Cancel server job
 */
void ClientSession::cancelJob(CSCPMessage *pRequest)
{
	CSCPMessage msg;

	msg.SetCode(CMD_REQUEST_COMPLETED);
	msg.SetId(pRequest->GetId());
	msg.SetVariable(VID_RCC, ::CancelJob(m_dwUserId, pRequest));
	sendMessage(&msg);
}

/**
 * Put server job on hold
 */
void ClientSession::holdJob(CSCPMessage *pRequest)
{
	CSCPMessage msg;

	msg.SetCode(CMD_REQUEST_COMPLETED);
	msg.SetId(pRequest->GetId());
	msg.SetVariable(VID_RCC, ::HoldJob(m_dwUserId, pRequest));
	sendMessage(&msg);
}

/**
 * Allow server job on hold for execution
 */
void ClientSession::unholdJob(CSCPMessage *pRequest)
{
	CSCPMessage msg;

	msg.SetCode(CMD_REQUEST_COMPLETED);
	msg.SetId(pRequest->GetId());
	msg.SetVariable(VID_RCC, ::UnholdJob(m_dwUserId, pRequest));
	sendMessage(&msg);
}

/**
 * Deploy agent policy
 */
void ClientSession::deployAgentPolicy(CSCPMessage *request, bool uninstallFlag)
{
	CSCPMessage msg;

	msg.SetCode(CMD_REQUEST_COMPLETED);
	msg.SetId(request->GetId());

	UINT32 policyId = request->GetVariableLong(VID_POLICY_ID);
	UINT32 targetId = request->GetVariableLong(VID_OBJECT_ID);

	NetObj *policy = FindObjectById(policyId);
	if ((policy != NULL) && (policy->Type() >= OBJECT_AGENTPOLICY))
	{
		NetObj *target = FindObjectById(targetId);
		if ((target != NULL) && (target->Type() == OBJECT_NODE))
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
						msg.SetVariable(VID_RCC, RCC_SUCCESS);
					}
					else
					{
						delete job;
						msg.SetVariable(VID_RCC, RCC_INTERNAL_ERROR);
					}
				}
				else
				{
					msg.SetVariable(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
				}
			}
			else
			{
				msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
			}
		}
		else
		{
			msg.SetVariable(VID_RCC, RCC_INVALID_OBJECT_ID);
		}
	}
	else
	{
		msg.SetVariable(VID_RCC, RCC_INVALID_POLICY_ID);
	}

	sendMessage(&msg);
}

/**
 * Get custom attribute for current user
 */
void ClientSession::getUserCustomAttribute(CSCPMessage *request)
{
	CSCPMessage msg;

	msg.SetCode(CMD_REQUEST_COMPLETED);
	msg.SetId(request->GetId());

	TCHAR *name = request->GetVariableStr(VID_NAME);
	if ((name != NULL) && (*name == _T('.')))
	{
		const TCHAR *value = GetUserDbObjectAttr(m_dwUserId, name);
		msg.SetVariable(VID_VALUE, CHECK_NULL_EX(value));
		msg.SetVariable(VID_RCC, RCC_SUCCESS);
	}
	else
	{
		msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
	}
	safe_free(name);

	sendMessage(&msg);
}

/**
 * Set custom attribute for current user
 */
void ClientSession::setUserCustomAttribute(CSCPMessage *request)
{
	CSCPMessage msg;

	msg.SetCode(CMD_REQUEST_COMPLETED);
	msg.SetId(request->GetId());

	TCHAR *name = request->GetVariableStr(VID_NAME);
	if ((name != NULL) && (*name == _T('.')))
	{
		TCHAR *value = request->GetVariableStr(VID_VALUE);
		SetUserDbObjectAttr(m_dwUserId, name, CHECK_NULL_EX(value));
		msg.SetVariable(VID_RCC, RCC_SUCCESS);
		safe_free(value);
	}
	else
	{
		msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
	}
	safe_free(name);

	sendMessage(&msg);
}

/**
 * Open server log
 */
void ClientSession::openServerLog(CSCPMessage *request)
{
	CSCPMessage msg;

	msg.SetCode(CMD_REQUEST_COMPLETED);
	msg.SetId(request->GetId());

	TCHAR name[256];
	request->GetVariableStr(VID_LOG_NAME, name, 256);

	UINT32 rcc;
	int handle = OpenLog(name, this, &rcc);
	if (handle != -1)
	{
		msg.SetVariable(VID_RCC, RCC_SUCCESS);
		msg.SetVariable(VID_LOG_HANDLE, (UINT32)handle);

		LogHandle *log = AcquireLogHandleObject(this, handle);
		log->getColumnInfo(msg);
		log->unlock();
	}
	else
	{
		msg.SetVariable(VID_RCC, rcc);
	}

	sendMessage(&msg);
}

/**
 * Close server log
 */
void ClientSession::closeServerLog(CSCPMessage *request)
{
	CSCPMessage msg;

	msg.SetCode(CMD_REQUEST_COMPLETED);
	msg.SetId(request->GetId());

	int handle = (int)request->GetVariableLong(VID_LOG_HANDLE);
	msg.SetVariable(VID_RCC, CloseLog(this, handle));

	sendMessage(&msg);
}

/**
 * Query server log
 */
void ClientSession::queryServerLog(CSCPMessage *request)
{
	CSCPMessage msg;

	msg.SetCode(CMD_REQUEST_COMPLETED);
	msg.SetId(request->GetId());

	int handle = (int)request->GetVariableLong(VID_LOG_HANDLE);
	LogHandle *log = AcquireLogHandleObject(this, handle);
	if (log != NULL)
	{
		INT64 rowCount;
		msg.SetVariable(VID_RCC, log->query(new LogFilter(request), &rowCount, getUserId()) ? RCC_SUCCESS : RCC_DB_FAILURE);
		msg.SetVariable(VID_NUM_ROWS, (QWORD)rowCount);
		log->unlock();
	}
	else
	{
		msg.SetVariable(VID_RCC, RCC_INVALID_LOG_HANDLE);
	}

	sendMessage(&msg);
}

/**
 * Get log data from query result
 */
void ClientSession::getServerLogQueryData(CSCPMessage *request)
{
	CSCPMessage msg;
	Table *data = NULL;

	msg.SetCode(CMD_REQUEST_COMPLETED);
	msg.SetId(request->GetId());

	int handle = (int)request->GetVariableLong(VID_LOG_HANDLE);
	LogHandle *log = AcquireLogHandleObject(this, handle);
	if (log != NULL)
	{
		INT64 startRow = request->GetVariableInt64(VID_START_ROW);
		INT64 numRows = request->GetVariableInt64(VID_NUM_ROWS);
		bool refresh = request->GetVariableShort(VID_FORCE_RELOAD) ? true : false;
		data = log->getData(startRow, numRows, refresh, getUserId()); // pass user id from session
		log->unlock();
		if (data != NULL)
		{
			msg.SetVariable(VID_RCC, RCC_SUCCESS);
		}
		else
		{
			msg.SetVariable(VID_RCC, RCC_DB_FAILURE);
		}
	}
	else
	{
		msg.SetVariable(VID_RCC, RCC_INVALID_LOG_HANDLE);
	}

	sendMessage(&msg);

	if (data != NULL)
	{
		msg.SetCode(CMD_LOG_DATA);
		int offset = 0;
		do
		{
			msg.deleteAllVariables();
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
   CSCPMessage msg;
	int i, count;
	UINT32 id;
	TCHAR buffer[MAX_DB_STRING];
	DB_RESULT hResult;

	msg.SetId(dwRqId);
	msg.SetCode(CMD_REQUEST_COMPLETED);

	if (m_dwSystemAccess & SYSTEM_ACCESS_SERVER_CONFIG)
	{
		hResult = DBSelect(g_hCoreDB, _T("SELECT user_name,auth_method,priv_method,auth_password,priv_password FROM usm_credentials"));
		if (hResult != NULL)
		{
			count = DBGetNumRows(hResult);
			msg.SetVariable(VID_NUM_RECORDS, (UINT32)count);
			for(i = 0, id = VID_USM_CRED_LIST_BASE; i < count; i++, id += 5)
			{
				DBGetField(hResult, i, 0, buffer, MAX_DB_STRING);	// security name
				msg.SetVariable(id++, buffer);

				msg.SetVariable(id++, (WORD)DBGetFieldLong(hResult, i, 1));	// auth method
				msg.SetVariable(id++, (WORD)DBGetFieldLong(hResult, i, 2));	// priv method

				DBGetField(hResult, i, 3, buffer, MAX_DB_STRING);	// auth password
				msg.SetVariable(id++, buffer);

				DBGetField(hResult, i, 4, buffer, MAX_DB_STRING);	// priv password
				msg.SetVariable(id++, buffer);
			}
			DBFreeResult(hResult);
			msg.SetVariable(VID_RCC, RCC_SUCCESS);
		}
		else
		{
			msg.SetVariable(VID_RCC, RCC_DB_FAILURE);
		}
	}
	else
	{
		msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
	}

	sendMessage(&msg);
}

/**
 * Update SNMP v3 USM credentials
 */
void ClientSession::updateUsmCredentials(CSCPMessage *request)
{
   CSCPMessage msg;

	msg.SetId(request->GetId());
	msg.SetCode(CMD_REQUEST_COMPLETED);

	if (m_dwSystemAccess & SYSTEM_ACCESS_SERVER_CONFIG)
	{
		DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
		if (DBBegin(hdb))
		{
			TCHAR query[4096];
			UINT32 id;
			int i = -1;
			int count = (int)request->GetVariableLong(VID_NUM_RECORDS);

			if (DBQuery(hdb, _T("DELETE FROM usm_credentials")))
			{
				for(i = 0, id = VID_USM_CRED_LIST_BASE; i < count; i++, id += 5)
				{
					TCHAR name[MAX_DB_STRING], authPasswd[MAX_DB_STRING], privPasswd[MAX_DB_STRING];

					request->GetVariableStr(id++, name, MAX_DB_STRING);
					int authMethod = (int)request->GetVariableShort(id++);
					int privMethod = (int)request->GetVariableShort(id++);
					request->GetVariableStr(id++, authPasswd, MAX_DB_STRING);
					request->GetVariableStr(id++, privPasswd, MAX_DB_STRING);
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
				msg.SetVariable(VID_RCC, RCC_SUCCESS);
			}
			else
			{
				DBRollback(hdb);
				msg.SetVariable(VID_RCC, RCC_DB_FAILURE);
			}
		}
		else
		{
			msg.SetVariable(VID_RCC, RCC_DB_FAILURE);
		}
		DBConnectionPoolReleaseConnection(hdb);
	}
	else
	{
		msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
	}

	sendMessage(&msg);
}

/**
 * Find connection point for the node
 */
void ClientSession::findNodeConnection(CSCPMessage *request)
{
   CSCPMessage msg;

	msg.SetId(request->GetId());
	msg.SetCode(CMD_REQUEST_COMPLETED);

	UINT32 objectId = request->GetVariableLong(VID_OBJECT_ID);
	NetObj *object = FindObjectById(objectId);
	if ((object != NULL) && !object->isDeleted())
	{
		if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
		{
			debugPrintf(5, _T("findNodeConnection: objectId=%d class=%d name=\"%s\""), objectId, object->Type(), object->Name());
			NetObj *cp = NULL;
			UINT32 localNodeId, localIfId;
			BYTE localMacAddr[MAC_ADDR_LENGTH];
			int type = 0;
			if (object->Type() == OBJECT_NODE)
			{
				localNodeId = objectId;
				cp = ((Node *)object)->findConnectionPoint(&localIfId, localMacAddr, &type);
				msg.SetVariable(VID_RCC, RCC_SUCCESS);
			}
			else if (object->Type() == OBJECT_INTERFACE)
			{
				localNodeId = ((Interface *)object)->getParentNode()->Id();
				localIfId = objectId;
				memcpy(localMacAddr, ((Interface *)object)->getMacAddr(), MAC_ADDR_LENGTH);
				cp = FindInterfaceConnectionPoint(localMacAddr, &type);
				msg.SetVariable(VID_RCC, RCC_SUCCESS);
			}
         else if (object->Type() == OBJECT_ACCESSPOINT)
			{
				localNodeId = 0;
				localIfId = 0;
				memcpy(localMacAddr, ((AccessPoint *)object)->getMacAddr(), MAC_ADDR_LENGTH);
				cp = FindInterfaceConnectionPoint(localMacAddr, &type);
				msg.SetVariable(VID_RCC, RCC_SUCCESS);
			}
			else
			{
				msg.SetVariable(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
			}

			debugPrintf(5, _T("findNodeConnection: cp=%p type=%d"), cp, type);
			if (cp != NULL)
			{
            Node *node = (cp->Type() == OBJECT_INTERFACE) ? ((Interface *)cp)->getParentNode() : ((AccessPoint *)cp)->getParentNode();
            if (node != NULL)
            {
               msg.SetVariable(VID_OBJECT_ID, node->Id());
				   msg.SetVariable(VID_INTERFACE_ID, cp->Id());
               msg.SetVariable(VID_IF_INDEX, (cp->Type() == OBJECT_INTERFACE) ? ((Interface *)cp)->getIfIndex() : (UINT32)0);
				   msg.SetVariable(VID_LOCAL_NODE_ID, localNodeId);
				   msg.SetVariable(VID_LOCAL_INTERFACE_ID, localIfId);
				   msg.SetVariable(VID_MAC_ADDR, localMacAddr, MAC_ADDR_LENGTH);
				   msg.SetVariable(VID_CONNECTION_TYPE, (UINT16)type);
               if (cp->Type() == OBJECT_INTERFACE)
                  debugPrintf(5, _T("findNodeConnection: nodeId=%d ifId=%d ifName=%s ifIndex=%d"), node->Id(), cp->Id(), cp->Name(), ((Interface *)cp)->getIfIndex());
               else
                  debugPrintf(5, _T("findNodeConnection: nodeId=%d apId=%d apName=%s"), node->Id(), cp->Id(), cp->Name());
            }
            else
            {
      			msg.SetVariable(VID_RCC, RCC_INTERNAL_ERROR);
            }
			}
		}
		else
		{
			msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
		}
	}
	else
	{
		msg.SetVariable(VID_RCC, RCC_INVALID_OBJECT_ID);
	}

	sendMessage(&msg);
}

/**
 * Find connection port for given MAC address
 */
void ClientSession::findMacAddress(CSCPMessage *request)
{
   CSCPMessage msg;
	BYTE macAddr[6];

	msg.SetId(request->GetId());
	msg.SetCode(CMD_REQUEST_COMPLETED);

	request->GetVariableBinary(VID_MAC_ADDR, macAddr, 6);
	int type;
	NetObj *cp = FindInterfaceConnectionPoint(macAddr, &type);
	msg.SetVariable(VID_RCC, RCC_SUCCESS);

	debugPrintf(5, _T("findMacAddress: cp=%p type=%d"), cp, type);
	if (cp != NULL)
	{
		UINT32 localNodeId, localIfId;

		Interface *localIf = FindInterfaceByMAC(macAddr);
		if (localIf != NULL)
		{
			localIfId = localIf->Id();
			localNodeId = localIf->getParentNode()->Id();
		}
		else
		{
			localIfId = 0;
			localNodeId = 0;
		}

      Node *node = (cp->Type() == OBJECT_INTERFACE) ? ((Interface *)cp)->getParentNode() : ((AccessPoint *)cp)->getParentNode();
      if (node != NULL)
      {
		   msg.SetVariable(VID_OBJECT_ID, node->Id());
		   msg.SetVariable(VID_INTERFACE_ID, cp->Id());
         msg.SetVariable(VID_IF_INDEX, (cp->Type() == OBJECT_INTERFACE) ? ((Interface *)cp)->getIfIndex() : (UINT32)0);
	      msg.SetVariable(VID_LOCAL_NODE_ID, localNodeId);
		   msg.SetVariable(VID_LOCAL_INTERFACE_ID, localIfId);
		   msg.SetVariable(VID_MAC_ADDR, macAddr, MAC_ADDR_LENGTH);
		   msg.SetVariable(VID_IP_ADDRESS, (localIf != NULL) ? localIf->IpAddr() : (UINT32)0);
		   msg.SetVariable(VID_CONNECTION_TYPE, (UINT16)type);
         if (cp->Type() == OBJECT_INTERFACE)
            debugPrintf(5, _T("findMacAddress: nodeId=%d ifId=%d ifName=%s ifIndex=%d"), node->Id(), cp->Id(), cp->Name(), ((Interface *)cp)->getIfIndex());
         else
            debugPrintf(5, _T("findMacAddress: nodeId=%d apId=%d apName=%s"), node->Id(), cp->Id(), cp->Name());
      }
      else
      {
		   msg.SetVariable(VID_RCC, RCC_INTERNAL_ERROR);
      }
	}

	sendMessage(&msg);
}

/**
 * Find connection port for given IP address
 */
void ClientSession::findIpAddress(CSCPMessage *request)
{
   CSCPMessage msg;
	TCHAR ipAddrText[16];

	msg.SetId(request->GetId());
	msg.SetCode(CMD_REQUEST_COMPLETED);
	msg.SetVariable(VID_RCC, RCC_SUCCESS);

	BYTE macAddr[6];
	bool found = false;

	UINT32 zoneId = request->GetVariableLong(VID_ZONE_ID);
	UINT32 ipAddr = request->GetVariableLong(VID_IP_ADDRESS);
	Interface *iface = FindInterfaceByIP(zoneId, ipAddr);
	if ((iface != NULL) && memcmp(iface->getMacAddr(), "\x00\x00\x00\x00\x00\x00", MAC_ADDR_LENGTH))
	{
		memcpy(macAddr, iface->getMacAddr(), MAC_ADDR_LENGTH);
		found = true;
		debugPrintf(5, _T("findIpAddress(%s): endpoint iface=%s"), IpToStr(ipAddr, ipAddrText), iface->Name());
	}
	else
	{
		// no interface object with this IP or MAC address not known, try to find it in ARP caches
		debugPrintf(5, _T("findIpAddress(%s): interface not found, looking in ARP cache"), IpToStr(ipAddr, ipAddrText));
		Subnet *subnet = FindSubnetForNode(zoneId, ipAddr);
		if (subnet != NULL)
		{
			debugPrintf(5, _T("findIpAddress(%s): found subnet %s"), ipAddrText, subnet->Name());
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
				localIfId = localIf->Id();
				localNodeId = localIf->getParentNode()->Id();
			}
			else
			{
				localIfId = 0;
				localNodeId = 0;
			}

         Node *node = (cp->Type() == OBJECT_INTERFACE) ? ((Interface *)cp)->getParentNode() : ((AccessPoint *)cp)->getParentNode();
         if (node != NULL)
         {
		      msg.SetVariable(VID_OBJECT_ID, node->Id());
			   msg.SetVariable(VID_INTERFACE_ID, cp->Id());
            msg.SetVariable(VID_IF_INDEX, (cp->Type() == OBJECT_INTERFACE) ? ((Interface *)cp)->getIfIndex() : (UINT32)0);
			   msg.SetVariable(VID_LOCAL_NODE_ID, localNodeId);
			   msg.SetVariable(VID_LOCAL_INTERFACE_ID, localIfId);
			   msg.SetVariable(VID_MAC_ADDR, macAddr, MAC_ADDR_LENGTH);
			   msg.SetVariable(VID_IP_ADDRESS, ipAddr);
			   msg.SetVariable(VID_CONNECTION_TYPE, (UINT16)type);
            if (cp->Type() == OBJECT_INTERFACE)
               debugPrintf(5, _T("findIpAddress(%s): nodeId=%d ifId=%d ifName=%s ifIndex=%d"), IpToStr(ipAddr, ipAddrText), node->Id(), cp->Id(), cp->Name(), ((Interface *)cp)->getIfIndex());
            else
               debugPrintf(5, _T("findIpAddress(%s): nodeId=%d apId=%d apName=%s"), IpToStr(ipAddr, ipAddrText), node->Id(), cp->Id(), cp->Name());
         }
		}
	}

	sendMessage(&msg);
}

/**
 * Send image from library to client
 */
void ClientSession::sendLibraryImage(CSCPMessage *request)
{
	CSCPMessage msg;
	TCHAR guidText[64], absFileName[MAX_PATH];
	UINT32 rcc = RCC_SUCCESS;

	msg.SetId(request->GetId());
	msg.SetCode(CMD_REQUEST_COMPLETED);

	uuid_t guid;
	request->GetVariableBinary(VID_GUID, guid, UUID_LENGTH);
	uuid_to_string(guid, guidText);
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

				msg.SetVariable(VID_GUID, guid, UUID_LENGTH);

				DBGetField(result, 0, 0, buffer, MAX_DB_STRING);	// image name
				msg.SetVariable(VID_NAME, buffer);
				DBGetField(result, 0, 1, buffer, MAX_DB_STRING);	// category
				msg.SetVariable(VID_CATEGORY, buffer);
				DBGetField(result, 0, 2, buffer, MAX_DB_STRING);	// mime type
				msg.SetVariable(VID_IMAGE_MIMETYPE, buffer);

				msg.SetVariable(VID_IMAGE_PROTECTED, (WORD)DBGetFieldLong(result, 0, 3));

				_sntprintf(absFileName, MAX_PATH, _T("%s%s%s%s"), g_szDataDir, DDIR_IMAGES, FS_PATH_SEPARATOR, guidText);
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

	msg.SetVariable(VID_RCC, rcc);
	sendMessage(&msg);

	if (rcc == RCC_SUCCESS)
		sendFile(absFileName, request->GetId(), 0);
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
    m_pUpdateQueue->Put(pUpdate);
  }
}


//
// Send updates to all connected clients
//

static void SendLibraryImageUpdate(ClientSession *pSession, void *pArg)
{
	pSession->onLibraryImageChange((uuid_t *)pArg, false);
}

static void SendLibraryImageDelete(ClientSession *pSession, void *pArg)
{
	pSession->onLibraryImageChange((uuid_t *)pArg, true);
}

/**
 * Update library image from client
 */
void ClientSession::updateLibraryImage(CSCPMessage *request)
{
	CSCPMessage msg;
	UINT32 rcc = RCC_SUCCESS;

	msg.SetId(request->GetId());
	msg.SetCode(CMD_REQUEST_COMPLETED);

   if (!checkSysAccessRights(SYSTEM_ACCESS_MANAGE_IMAGE_LIB))
   {
	   msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
      sendMessage(&msg);
      return;
   }

	uuid_t guid;
	uuid_clear(guid);

	msg.SetId(request->GetId());
	msg.SetCode(CMD_REQUEST_COMPLETED);

   TCHAR name[MAX_OBJECT_NAME] = _T("");
	TCHAR category[MAX_OBJECT_NAME] = _T("");
	TCHAR mimetype[MAX_DB_STRING] = _T("");
	TCHAR absFileName[MAX_PATH] = _T("");

	if (request->isFieldExist(VID_GUID))
	{
		request->GetVariableBinary(VID_GUID, guid, UUID_LENGTH);
	}

	if (uuid_is_null(guid))
	{
		uuid_generate(guid);
	}

	TCHAR guidText[64];
	uuid_to_string(guid, guidText);

	request->GetVariableStr(VID_NAME, name, MAX_OBJECT_NAME);
	request->GetVariableStr(VID_CATEGORY, category, MAX_OBJECT_NAME);
	request->GetVariableStr(VID_IMAGE_MIMETYPE, mimetype, MAX_DB_STRING);

	//UINT32 imageSize = request->GetVariableBinary(VID_IMAGE_DATA, NULL, 0);
	//BYTE *imageData = (BYTE *)malloc(imageSize);
	//request->GetVariableBinary(VID_IMAGE_DATA, imageData, imageSize);

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
					_sntprintf(absFileName, MAX_PATH, _T("%s%s%s%s"), g_szDataDir, DDIR_IMAGES, FS_PATH_SEPARATOR, guidText);
					DbgPrintf(5, _T("updateLibraryImage: guid=%s, absFileName=%s"), guidText, absFileName);

					if (m_hCurrFile == -1)
					{
						m_hCurrFile = _topen(absFileName, O_CREAT | O_TRUNC | O_WRONLY | O_BINARY, S_IRUSR | S_IWUSR);
						if (m_hCurrFile != -1)
						{
							m_dwFileRqId = request->GetId();
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
		msg.SetVariable(VID_GUID, guid, UUID_LENGTH);
	}

   DBConnectionPoolReleaseConnection(hdb);
	msg.SetVariable(VID_RCC, rcc);
	sendMessage(&msg);

	if (rcc == RCC_SUCCESS)
	{
		//EnumerateClientSessions(SendLibraryImageUpdate, (void *)&guid);
	}
}

/**
 * Delete image from library
 */
void ClientSession::deleteLibraryImage(CSCPMessage *request)
{
	CSCPMessage msg;
	UINT32 rcc = RCC_SUCCESS;
	uuid_t guid;
	TCHAR guidText[64];
	TCHAR query[MAX_DB_STRING];

	msg.SetId(request->GetId());
	msg.SetCode(CMD_REQUEST_COMPLETED);

   if (!checkSysAccessRights(SYSTEM_ACCESS_MANAGE_IMAGE_LIB))
   {
	   msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
      sendMessage(&msg);
      return;
   }

	request->GetVariableBinary(VID_GUID, guid, UUID_LENGTH);
	uuid_to_string(guid, guidText);
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

	msg.SetVariable(VID_RCC, rcc);
	sendMessage(&msg);

	if (rcc == RCC_SUCCESS)
	{
		EnumerateClientSessions(SendLibraryImageDelete, (void *)&guid);
	}
}

/**
 * Send list of available images (in category)
 */
void ClientSession::listLibraryImages(CSCPMessage *request)
{
	CSCPMessage msg;
	TCHAR category[MAX_DB_STRING];
	TCHAR query[MAX_DB_STRING * 2];
	TCHAR buffer[MAX_DB_STRING];
	uuid_t guid;
	UINT32 rcc = RCC_SUCCESS;

	msg.SetId(request->GetId());
	msg.SetCode(CMD_REQUEST_COMPLETED);

	if (request->isFieldExist(VID_CATEGORY))
	{
		request->GetVariableStr(VID_CATEGORY, category, MAX_DB_STRING);
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
		msg.SetVariable(VID_NUM_RECORDS, (UINT32)count);
		UINT32 varId = VID_IMAGE_LIST_BASE;
		for (int i = 0; i < count; i++)
		{
			DBGetFieldGUID(result, i, 0, guid);	// guid
			msg.SetVariable(varId++, guid, UUID_LENGTH);

			DBGetField(result, i, 1, buffer, MAX_DB_STRING);	// image name
			msg.SetVariable(varId++, buffer);

			DBGetField(result, i, 2, buffer, MAX_DB_STRING);	// category
			msg.SetVariable(varId++, buffer);

			DBGetField(result, i, 3, buffer, MAX_DB_STRING);	// mime type
			msg.SetVariable(varId++, buffer);

			msg.SetVariable(varId++, (WORD)DBGetFieldLong(result, i, 4)); // protected flag
		}

		DBFreeResult(result);
	}
	else
	{
		rcc = RCC_DB_FAILURE;
	}

   DBConnectionPoolReleaseConnection(hdb);
	msg.SetVariable(VID_RCC, rcc);
	sendMessage(&msg);
}

/**
 * Worker thread for server side command execution
 */
static THREAD_RESULT THREAD_CALL RunCommand(void *arg)
{
	DbgPrintf(5, _T("Running server-side command: %s"), (TCHAR *)arg);
	if (_tsystem((TCHAR *)arg) == -1)
	   DbgPrintf(5, _T("Failed to execute command \"%s\""), (TCHAR *)arg);
	free(arg);
	return THREAD_OK;
}

/**
 * Execute server side command on object
 */
void ClientSession::executeServerCommand(CSCPMessage *request)
{
	CSCPMessage msg;

	msg.SetId(request->GetId());
	msg.SetCode(CMD_REQUEST_COMPLETED);

	UINT32 nodeId = request->GetVariableLong(VID_OBJECT_ID);
	NetObj *object = FindObjectById(nodeId);
	if (object != NULL)
	{
		if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_CONTROL))
		{
			if (object->Type() == OBJECT_NODE)
			{
				TCHAR *cmd = request->GetVariableStr(VID_COMMAND);
				TCHAR *expCmd = ((Node *)object)->expandText(cmd);
				free(cmd);
				WriteAuditLog(AUDIT_OBJECTS, TRUE, m_dwUserId, m_workstation, nodeId, _T("Server command executed: %s"), expCmd);
				ThreadCreate(RunCommand, 0, expCmd);
				msg.SetVariable(VID_RCC, RCC_SUCCESS);
			}
			else
			{
				msg.SetVariable(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
			}
		}
		else
		{
			msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
			WriteAuditLog(AUDIT_OBJECTS, FALSE, m_dwUserId, m_workstation, nodeId, _T("Access denied on server command execution"));
		}
	}
	else
	{
		msg.SetVariable(VID_RCC, RCC_INVALID_OBJECT_ID);
	}

	sendMessage(&msg);
}


//
// Upload file from server to agent
//

void ClientSession::uploadFileToAgent(CSCPMessage *request)
{
	CSCPMessage msg;

	msg.SetId(request->GetId());
	msg.SetCode(CMD_REQUEST_COMPLETED);

	UINT32 nodeId = request->GetVariableLong(VID_OBJECT_ID);
	NetObj *object = FindObjectById(nodeId);
	if (object != NULL)
	{
		if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_CONTROL))
		{
			if (object->Type() == OBJECT_NODE)
			{
				TCHAR *localFile = request->GetVariableStr(VID_FILE_NAME);
				TCHAR *remoteFile = request->GetVariableStr(VID_DESTINATION_FILE_NAME);
				if (localFile != NULL)
				{
					int nLen;
					TCHAR fullPath[MAX_PATH];

					// Create full path to the file store
					_tcscpy(fullPath, g_szDataDir);
					_tcscat(fullPath, DDIR_FILES);
					_tcscat(fullPath, FS_PATH_SEPARATOR);
					nLen = (int)_tcslen(fullPath);
					nx_strncpy(&fullPath[nLen], GetCleanFileName(localFile), MAX_PATH - nLen);

					ServerJob *job = new FileUploadJob((Node *)object, fullPath, remoteFile, m_dwUserId,
					                                   request->GetVariableShort(VID_CREATE_JOB_ON_HOLD) ? true : false);
					if (AddJob(job))
					{
						WriteAuditLog(AUDIT_OBJECTS, TRUE, m_dwUserId, m_workstation, nodeId,
										  _T("File upload to agent initiated, local='%s' remote='%s'"), CHECK_NULL(localFile), CHECK_NULL(remoteFile));
						msg.SetVariable(VID_JOB_ID, job->getId());
						msg.SetVariable(VID_RCC, RCC_SUCCESS);
					}
					else
					{
						msg.SetVariable(VID_RCC, RCC_INTERNAL_ERROR);
						delete job;
					}
				}
				else
				{
					msg.SetVariable(VID_RCC, RCC_INVALID_ARGUMENT);
				}
				safe_free(localFile);
				safe_free(remoteFile);
			}
			else
			{
				msg.SetVariable(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
			}
		}
		else
		{
			msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
			WriteAuditLog(AUDIT_OBJECTS, FALSE, m_dwUserId, m_workstation, nodeId, _T("Access denied on file upload"));
		}
	}
	else
	{
		msg.SetVariable(VID_RCC, RCC_INVALID_OBJECT_ID);
	}

	sendMessage(&msg);
}

/**
 * Send to client list of files in server's file store
 */
void ClientSession::listServerFileStore(CSCPMessage *request)
{
	CSCPMessage msg;
	TCHAR path[MAX_PATH];
	StringList extensionList;

	msg.SetId(request->GetId());
	msg.SetCode(CMD_REQUEST_COMPLETED);

	int length = (int)request->GetVariableLong(VID_EXTENSION_COUNT);
	DbgPrintf(8, _T("ClientSession::listServerFileStore: length of filter type array is %d."), length);

   UINT32 varId = VID_EXTENSION_LIST_BASE;

   bool musicFiles = (length > 0);
	for(int i = 0; i < length; i++)
   {
      extensionList.add(request->GetVariableStr(varId++));
      for(int j = 0; j < m_musicTypeList.getSize(); j++)
      {
         if(_tcscmp(extensionList.getValue(i), m_musicTypeList.getValue(j)))
         {
            musicFiles = false;
         }
      }
   }

	if (m_dwSystemAccess & SYSTEM_ACCESS_READ_FILES || musicFiles)
	{
      _tcscpy(path, g_szDataDir);
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
                     for(int j = 0; j < extensionList.getSize(); j++)
                     {
                        if (!_tcscmp(extension, extensionList.getValue(j)))
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
                     msg.SetVariable(varId++, d->d_name);
                     msg.SetVariable(varId++, (QWORD)st.st_size);
                     msg.SetVariable(varId++, (QWORD)st.st_mtime);
                     varId += 7;
                     count++;
                  }
               }
            }
         }
         _tclosedir(dir);
         msg.SetVariable(VID_INSTANCE_COUNT, count);
         msg.SetVariable(VID_RCC, RCC_SUCCESS);
      }
      else
      {
         msg.SetVariable(VID_RCC, RCC_IO_ERROR);
      }
   }
	else
	{
      msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
	}
	extensionList.clear();

	sendMessage(&msg);
}


//
// Open server console
//

void ClientSession::openConsole(UINT32 rqId)
{
	CSCPMessage msg;

	msg.SetId(rqId);
	msg.SetCode(CMD_REQUEST_COMPLETED);

	if (m_dwSystemAccess & SYSTEM_ACCESS_SERVER_CONSOLE)
	{
		m_dwFlags |= CSF_CONSOLE_OPEN;
		m_console = (CONSOLE_CTX)malloc(sizeof(struct __console_ctx));
		m_console->hSocket = -1;
		m_console->socketMutex = INVALID_MUTEX_HANDLE;
		m_console->pMsg = new CSCPMessage;
		m_console->pMsg->SetCode(CMD_ADM_MESSAGE);
		m_console->session = this;
      m_console->output = NULL;
		msg.SetVariable(VID_RCC, RCC_SUCCESS);
	}
	else
	{
		msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
	}

	sendMessage(&msg);
}

/**
 * Close server console
 */
void ClientSession::closeConsole(UINT32 rqId)
{
	CSCPMessage msg;

	msg.SetId(rqId);
	msg.SetCode(CMD_REQUEST_COMPLETED);

	if (m_dwSystemAccess & SYSTEM_ACCESS_SERVER_CONSOLE)
	{
		if (m_dwFlags & CSF_CONSOLE_OPEN)
		{
			m_dwFlags &= ~CSF_CONSOLE_OPEN;
			delete m_console->pMsg;
			safe_free_and_null(m_console);
			msg.SetVariable(VID_RCC, RCC_SUCCESS);
		}
		else
		{
			msg.SetVariable(VID_RCC, RCC_OUT_OF_STATE_REQUEST);
		}
	}
	else
	{
		msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
	}

	sendMessage(&msg);
}


//
// Process console command
//

void ClientSession::processConsoleCommand(CSCPMessage *request)
{
	CSCPMessage msg;

	msg.SetCode(CMD_REQUEST_COMPLETED);
	msg.SetId(request->GetId());

	if ((m_dwSystemAccess & SYSTEM_ACCESS_SERVER_CONSOLE) && (m_dwFlags & CSF_CONSOLE_OPEN))
	{
		TCHAR command[256];
		request->GetVariableStr(VID_COMMAND, command, 256);
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
		msg.SetVariable(VID_RCC, RCC_SUCCESS);
	}
	else
	{
		msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
	}

	sendMessage(&msg);
}


//
// Get VLANs configured on device
//

void ClientSession::getVlans(CSCPMessage *request)
{
	CSCPMessage msg;

	msg.SetCode(CMD_REQUEST_COMPLETED);
	msg.SetId(request->GetId());

	NetObj *object = FindObjectById(request->GetVariableLong(VID_OBJECT_ID));
	if (object != NULL)
	{
		if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
		{
			if (object->Type() == OBJECT_NODE)
			{
				VlanList *vlans = ((Node *)object)->getVlans();
				if (vlans != NULL)
				{
					vlans->fillMessage(&msg);
					vlans->decRefCount();
					msg.SetVariable(VID_RCC, RCC_SUCCESS);
				}
				else
				{
					msg.SetVariable(VID_RCC, RCC_RESOURCE_NOT_AVAILABLE);
				}
			}
			else
			{
				msg.SetVariable(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
			}
		}
		else
		{
			msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
		}
	}
	else
	{
		msg.SetVariable(VID_RCC, RCC_INVALID_OBJECT_ID);
	}

	sendMessage(&msg);
}

/**
 * Receive file from client
 */
void ClientSession::receiveFile(CSCPMessage *request)
{
   CSCPMessage msg;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(request->GetId());

	if (m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_FILES)
   {
		TCHAR fileName[MAX_PATH];

      request->GetVariableStr(VID_FILE_NAME, fileName, MAX_PATH);
      const TCHAR *cleanFileName = GetCleanFileName(fileName);

      // Prepare for file receive
      if (m_hCurrFile == -1)
      {
         _tcscpy(m_szCurrFileName, g_szDataDir);
			_tcscat(m_szCurrFileName, DDIR_FILES);
         _tcscat(m_szCurrFileName, FS_PATH_SEPARATOR);
         _tcscat(m_szCurrFileName, cleanFileName);
         m_hCurrFile = _topen(m_szCurrFileName, O_CREAT | O_TRUNC | O_WRONLY | O_BINARY, S_IRUSR | S_IWUSR);
         if (m_hCurrFile != -1)
         {
            m_dwFileRqId = request->GetId();
            m_dwUploadCommand = CMD_UPLOAD_FILE;
            msg.SetVariable(VID_RCC, RCC_SUCCESS);
            WriteAuditLog(AUDIT_SYSCFG, TRUE, m_dwUserId, m_workstation, 0,
               _T("Started upload of file \"%s\" to server"), fileName);
            NotifyClientSessions(NX_NOTIFY_FILE_LIST_CHANGED, 0);
         }
         else
         {
            msg.SetVariable(VID_RCC, RCC_IO_ERROR);
         }
      }
      else
      {
         msg.SetVariable(VID_RCC, RCC_RESOURCE_BUSY);
      }
   }
   else
   {
      msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
   }

   // Send response
   sendMessage(&msg);
}


/**
 * Delete file in store
 */

void ClientSession::deleteFile(CSCPMessage *request)
{
   CSCPMessage msg;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(request->GetId());

	if (m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_FILES)
   {
		TCHAR fileName[MAX_PATH];

      request->GetVariableStr(VID_FILE_NAME, fileName, MAX_PATH);
      const TCHAR *cleanFileName = GetCleanFileName(fileName);

      _tcscpy(m_szCurrFileName, g_szDataDir);
      _tcscat(m_szCurrFileName, DDIR_FILES);
      _tcscat(m_szCurrFileName, FS_PATH_SEPARATOR);
      _tcscat(m_szCurrFileName, cleanFileName);

      if (_tunlink(m_szCurrFileName) == 0)
      {
         NotifyClientSessions(NX_NOTIFY_FILE_LIST_CHANGED, 0);
         msg.SetVariable(VID_RCC, RCC_SUCCESS);
      }
      else
      {
         msg.SetVariable(VID_RCC, RCC_IO_ERROR);
      }
   }
   else
   {
      msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Get network path between two nodes
 */
void ClientSession::getNetworkPath(CSCPMessage *request)
{
	CSCPMessage msg;

	msg.SetCode(CMD_REQUEST_COMPLETED);
	msg.SetId(request->GetId());

	NetObj *node1 = FindObjectById(request->GetVariableLong(VID_SOURCE_OBJECT_ID));
	NetObj *node2 = FindObjectById(request->GetVariableLong(VID_DESTINATION_OBJECT_ID));

	if ((node1 != NULL) && (node2 != NULL))
	{
		if (node1->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ) &&
		    node2->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
		{
			if ((node1->Type() == OBJECT_NODE) && (node2->Type() == OBJECT_NODE))
			{
				NetworkPath *path = TraceRoute((Node *)node1, (Node *)node2);
				if (path != NULL)
				{
					msg.SetVariable(VID_RCC, RCC_SUCCESS);
					path->fillMessage(&msg);
					delete path;
				}
				else
				{
					msg.SetVariable(VID_RCC, RCC_INTERNAL_ERROR);
				}
			}
			else
			{
				msg.SetVariable(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
			}
		}
		else
		{
			msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
		}
	}
	else
	{
		msg.SetVariable(VID_RCC, RCC_INVALID_OBJECT_ID);
	}

	sendMessage(&msg);
}

/**
 * Get physical components of the node
 */
void ClientSession::getNodeComponents(CSCPMessage *request)
{
   CSCPMessage msg;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(request->GetId());

   // Get node id and check object class and access rights
   Node *node = (Node *)FindObjectById(request->GetVariableLong(VID_OBJECT_ID), OBJECT_NODE);
   if (node != NULL)
   {
      if (node->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
      {
			ComponentTree *components = node->getComponents();
			if (components != NULL)
			{
				msg.SetVariable(VID_RCC, RCC_SUCCESS);
				components->fillMessage(&msg, VID_COMPONENT_LIST_BASE);
				components->decRefCount();
			}
			else
			{
				msg.SetVariable(VID_RCC, RCC_NO_COMPONENT_DATA);
			}
      }
      else
      {
         msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
      }
   }
   else  // No object with given ID
   {
      msg.SetVariable(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Get list of software packages installed on node
 */
void ClientSession::getNodeSoftware(CSCPMessage *request)
{
   CSCPMessage msg;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(request->GetId());

   // Get node id and check object class and access rights
   Node *node = (Node *)FindObjectById(request->GetVariableLong(VID_OBJECT_ID), OBJECT_NODE);
   if (node != NULL)
   {
      if (node->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
      {
			node->writePackageListToMessage(&msg);
      }
      else
      {
         msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
      }
   }
   else  // No object with given ID
   {
      msg.SetVariable(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Get list of Windows performance objects supported by node
 */
void ClientSession::getWinPerfObjects(CSCPMessage *request)
{
   CSCPMessage msg;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(request->GetId());

   // Get node id and check object class and access rights
   Node *node = (Node *)FindObjectById(request->GetVariableLong(VID_OBJECT_ID), OBJECT_NODE);
   if (node != NULL)
   {
      if (node->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
      {
			node->writeWinPerfObjectsToMessage(&msg);
      }
      else
      {
         msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
      }
   }
   else  // No object with given ID
   {
      msg.SetVariable(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Get threshold summary for underlying data collection targets
 */
void ClientSession::getThresholdSummary(CSCPMessage *request)
{
   CSCPMessage msg;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(request->GetId());

   // Get object id and check object class and access rights
   NetObj *object = FindObjectById(request->GetVariableLong(VID_OBJECT_ID));
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
	         msg.SetVariable(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
			}
      }
      else
      {
         msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
      }
   }
   else  // No object with given ID
   {
      msg.SetVariable(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * List configured mapping tables
 */
void ClientSession::listMappingTables(CSCPMessage *request)
{
   CSCPMessage msg;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(request->GetId());

	if (m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_MAPPING_TBLS)
	{
		msg.SetVariable(VID_RCC, ListMappingTables(&msg));
	}
	else
	{
		msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
	}

   // Send response
   sendMessage(&msg);
}

/**
 * Get content of specific mapping table
 */
void ClientSession::getMappingTable(CSCPMessage *request)
{
   CSCPMessage msg;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(request->GetId());
	msg.SetVariable(VID_RCC, GetMappingTable((LONG)request->GetVariableLong(VID_MAPPING_TABLE_ID), &msg));

   // Send response
   sendMessage(&msg);
}

/**
 * Create or update mapping table
 */
void ClientSession::updateMappingTable(CSCPMessage *request)
{
   CSCPMessage msg;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(request->GetId());

	if (m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_MAPPING_TBLS)
	{
		LONG id;
		msg.SetVariable(VID_RCC, UpdateMappingTable(request, &id));
		msg.SetVariable(VID_MAPPING_TABLE_ID, (UINT32)id);
	}
	else
	{
		msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
	}

   // Send response
   sendMessage(&msg);
}

/**
 * Delete mapping table
 */
void ClientSession::deleteMappingTable(CSCPMessage *request)
{
   CSCPMessage msg;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(request->GetId());

	if (m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_MAPPING_TBLS)
	{
		msg.SetVariable(VID_RCC, DeleteMappingTable((LONG)request->GetVariableLong(VID_MAPPING_TABLE_ID)));
	}
	else
	{
		msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
	}

   // Send response
   sendMessage(&msg);
}

/**
 * Get list of wireless stations registered on controller
 */
void ClientSession::getWirelessStations(CSCPMessage *request)
{
   CSCPMessage msg;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(request->GetId());

   // Get object id and check object class and access rights
	Node *node = (Node *)FindObjectById(request->GetVariableLong(VID_OBJECT_ID), OBJECT_NODE);
   if (node != NULL)
   {
      if (node->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
      {
			if (node->isWirelessController())
			{
				node->writeWsListToMessage(&msg);
	         msg.SetVariable(VID_RCC, RCC_SUCCESS);
			}
			else
			{
	         msg.SetVariable(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
			}
      }
      else
      {
         msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
      }
   }
   else  // No object with given ID
   {
      msg.SetVariable(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Get list of configured DCI summary tables
 */
void ClientSession::getSummaryTables(UINT32 rqId)
{
   CSCPMessage msg;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(rqId);

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_RESULT hResult = DBSelect(hdb, _T("SELECT id,menu_path,title,flags FROM dci_summary_tables"));
   if (hResult != NULL)
   {
      TCHAR buffer[256];
      int count = DBGetNumRows(hResult);
      msg.SetVariable(VID_NUM_ELEMENTS, (UINT32)count);
      UINT32 varId = VID_ELEMENT_LIST_BASE;
      for(int i = 0; i < count; i++)
      {
         msg.SetVariable(varId++, (UINT32)DBGetFieldLong(hResult, i, 0));
         msg.SetVariable(varId++, DBGetField(hResult, i, 1, buffer, 256));
         msg.SetVariable(varId++, DBGetField(hResult, i, 2, buffer, 256));
         msg.SetVariable(varId++, (UINT32)DBGetFieldLong(hResult, i, 3));
         varId += 6;
      }
      DBFreeResult(hResult);
   }
	else
	{
		msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
	}
   DBConnectionPoolReleaseConnection(hdb);

   // Send response
   sendMessage(&msg);
}

/**
 * Get details of DCI summary table
 */
void ClientSession::getSummaryTableDetails(CSCPMessage *request)
{
   CSCPMessage msg;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(request->GetId());

	if (m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_SUMMARY_TBLS)
	{
      LONG id = (LONG)request->GetVariableLong(VID_SUMMARY_TABLE_ID);
      DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
      DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT menu_path,title,node_filter,flags,columns FROM dci_summary_tables WHERE id=?"));
      if (hStmt != NULL)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, id);
         DB_RESULT hResult = DBSelectPrepared(hStmt);
         if (hResult != NULL)
         {
            if (DBGetNumRows(hResult) > 0)
            {
               TCHAR buffer[256];
               msg.SetVariable(VID_SUMMARY_TABLE_ID, (UINT32)id);
               msg.SetVariable(VID_MENU_PATH, DBGetField(hResult, 0, 0, buffer, 256));
               msg.SetVariable(VID_TITLE, DBGetField(hResult, 0, 1, buffer, 256));
               TCHAR *tmp = DBGetField(hResult, 0, 2, NULL, 0);
               if (tmp != NULL)
               {
                  msg.SetVariable(VID_FILTER, tmp);
                  free(tmp);
               }
               msg.SetVariable(VID_FLAGS, DBGetFieldULong(hResult, 0, 3));
               tmp = DBGetField(hResult, 0, 4, NULL, 0);
               if (tmp != NULL)
               {
                  msg.SetVariable(VID_COLUMNS, tmp);
                  free(tmp);
               }
            }
            else
            {
               msg.SetVariable(VID_RCC, RCC_INVALID_SUMMARY_TABLE_ID);
            }
            DBFreeResult(hResult);
         }
         else
         {
            msg.SetVariable(VID_RCC, RCC_DB_FAILURE);
         }
         DBFreeStatement(hStmt);
      }
      else
      {
         msg.SetVariable(VID_RCC, RCC_DB_FAILURE);
      }
      DBConnectionPoolReleaseConnection(hdb);
	}
	else
	{
		msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
	}

   // Send response
   sendMessage(&msg);
}

/**
 * Modify DCI summary table
 */
void ClientSession::modifySummaryTable(CSCPMessage *request)
{
   CSCPMessage msg;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(request->GetId());

	if (m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_SUMMARY_TBLS)
	{
		LONG id;
		msg.SetVariable(VID_RCC, ModifySummaryTable(request, &id));
		msg.SetVariable(VID_SUMMARY_TABLE_ID, (UINT32)id);
	}
	else
	{
		msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
	}

   // Send response
   sendMessage(&msg);
}

/**
 * Delete DCI summary table
 */
void ClientSession::deleteSummaryTable(CSCPMessage *request)
{
   CSCPMessage msg;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(request->GetId());

	if (m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_SUMMARY_TBLS)
	{
		msg.SetVariable(VID_RCC, DeleteSummaryTable((LONG)request->GetVariableLong(VID_SUMMARY_TABLE_ID)));
	}
	else
	{
		msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
	}

   // Send response
   sendMessage(&msg);
}

/**
 * Query DCI summary table
 */
void ClientSession::querySummaryTable(CSCPMessage *request)
{
   CSCPMessage msg;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(request->GetId());

   UINT32 rcc;
   Table *result = QuerySummaryTable((LONG)request->GetVariableLong(VID_SUMMARY_TABLE_ID),
                                      request->GetVariableLong(VID_OBJECT_ID),
                                      m_dwUserId, &rcc);
   if (result != NULL)
   {
      debugPrintf(6, _T("querySummaryTable: %d rows in resulting table"), result->getNumRows());
      msg.SetVariable(VID_RCC, RCC_SUCCESS);
      result->fillMessage(msg, 0, -1);
      delete result;
   }
   else
   {
      msg.SetVariable(VID_RCC, rcc);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Forward event to Reporting Server
 */
void ClientSession::forwardToReportingServer(CSCPMessage *request)
{
   CSCPMessage *msg = NULL;

   if (checkSysAccessRights(SYSTEM_ACCESS_REPORTING_SERVER))
   {
      TCHAR buffer[256];
	   debugPrintf(7, _T("RS: Forwarding message %s"), NXCPMessageCodeName(request->GetCode(), buffer));

	   request->SetVariable(VID_USER_NAME, getUserName());
	   msg = ForwardMessageToReportingServer(request, this);
	   if (msg == NULL)
	   {
		  msg = new CSCPMessage();
		  msg->SetCode(CMD_REQUEST_COMPLETED);
		  msg->SetId(request->GetId());
		  msg->SetVariable(VID_RCC, RCC_COMM_FAILURE);
	   }
   }
   else
   {
	   WriteAuditLog(AUDIT_SECURITY, FALSE, m_dwUserId, m_workstation, 0, _T("Reporting server access denied"));
	   msg = new CSCPMessage();
	   msg->SetCode(CMD_REQUEST_COMPLETED);
	   msg->SetId(request->GetId());
	   msg->SetVariable(VID_RCC, RCC_ACCESS_DENIED);
   }

   sendMessage(msg);
   delete msg;
}

/**
 * Get address map for a subnet
 */
void ClientSession::getSubnetAddressMap(CSCPMessage *request)
{
   CSCPMessage msg;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(request->GetId());

   // Get node id and check object class and access rights
   Subnet *subnet = (Subnet *)FindObjectById(request->GetVariableLong(VID_OBJECT_ID), OBJECT_SUBNET);
   if (subnet != NULL)
   {
      if (subnet->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
      {
         int length;
         UINT32 *map = subnet->buildAddressMap(&length);
			if (map != NULL)
			{
				msg.SetVariable(VID_RCC, RCC_SUCCESS);
            msg.setFieldInt32Array(VID_ADDRESS_MAP, (UINT32)length, map);
            free(map);
			}
			else
			{
				msg.SetVariable(VID_RCC, RCC_INTERNAL_ERROR);
			}
      }
      else
      {
         msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
      }
   }
   else  // No object with given ID
   {
      msg.SetVariable(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Get effective rights for object
 */
void ClientSession::getEffectiveRights(CSCPMessage *request)
{
   CSCPMessage msg;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(request->GetId());

   // Get node id and check object class and access rights
   NetObj *object = FindObjectById(request->GetVariableLong(VID_OBJECT_ID));
   if (object != NULL)
   {
      msg.SetVariable(VID_EFFECTIVE_RIGHTS, object->getUserRights(m_dwUserId));
		msg.SetVariable(VID_RCC, RCC_SUCCESS);
   }
   else  // No object with given ID
   {
      msg.SetVariable(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   // Send response
   sendMessage(&msg);
}
