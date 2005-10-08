/* 
** NetXMS - Network Management System
** Copyright (C) 2003, 2004, 2005 Victor Kirhenshtein
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
** $module: session.cpp
**
**/

#include "nxcore.h"

#ifndef _WIN32
#include <dirent.h>
#endif


//
// Constants
//

#define TRAP_CREATE     1
#define TRAP_UPDATE     2
#define TRAP_DELETE     3

#define RAW_MSG_SIZE    262144


//
// Externals
//

void UnregisterSession(DWORD dwIndex);


//
// Node poller start data
//

typedef struct
{
   ClientSession *pSession;
   Node *pNode;
   int iPollType;
   DWORD dwRqId;
} POLLER_START_DATA;


//
// Object tool acl entry
//

typedef struct
{
   DWORD dwToolId;
   DWORD dwUserId;
} OBJECT_TOOL_ACL;


//
// Fill CSCP message with user data
//

static void FillUserInfoMessage(CSCPMessage *pMsg, NMS_USER *pUser)
{
   pMsg->SetVariable(VID_USER_ID, pUser->dwId);
   pMsg->SetVariable(VID_USER_NAME, pUser->szName);
   pMsg->SetVariable(VID_USER_FLAGS, pUser->wFlags);
   pMsg->SetVariable(VID_USER_SYS_RIGHTS, pUser->wSystemRights);
   pMsg->SetVariable(VID_USER_FULL_NAME, pUser->szFullName);
   pMsg->SetVariable(VID_USER_DESCRIPTION, pUser->szDescription);
}


//
// Fill CSCP message with user group data
//

static void FillGroupInfoMessage(CSCPMessage *pMsg, NMS_USER_GROUP *pGroup)
{
   DWORD i, dwId;

   pMsg->SetVariable(VID_USER_ID, pGroup->dwId);
   pMsg->SetVariable(VID_USER_NAME, pGroup->szName);
   pMsg->SetVariable(VID_USER_FLAGS, pGroup->wFlags);
   pMsg->SetVariable(VID_USER_SYS_RIGHTS, pGroup->wSystemRights);
   pMsg->SetVariable(VID_USER_DESCRIPTION, pGroup->szDescription);
   pMsg->SetVariable(VID_NUM_MEMBERS, pGroup->dwNumMembers);
   for(i = 0, dwId = VID_GROUP_MEMBER_BASE; i < pGroup->dwNumMembers; i++, dwId++)
      pMsg->SetVariable(dwId, pGroup->pMembers[i]);
}


//
// Client communication read thread starter
//

THREAD_RESULT THREAD_CALL ClientSession::ReadThreadStarter(void *pArg)
{
   ((ClientSession *)pArg)->ReadThread();

   // When ClientSession::ReadThread exits, all other session
   // threads are already stopped, so we can safely destroy
   // session object
   UnregisterSession(((ClientSession *)pArg)->GetIndex());
   delete (ClientSession *)pArg;
   return THREAD_OK;
}


//
// Client communication write thread starter
//

THREAD_RESULT THREAD_CALL ClientSession::WriteThreadStarter(void *pArg)
{
   ((ClientSession *)pArg)->WriteThread();
   return THREAD_OK;
}


//
// Received message processing thread starter
//

THREAD_RESULT THREAD_CALL ClientSession::ProcessingThreadStarter(void *pArg)
{
   ((ClientSession *)pArg)->ProcessingThread();
   return THREAD_OK;
}


//
// Information update processing thread starter
//

THREAD_RESULT THREAD_CALL ClientSession::UpdateThreadStarter(void *pArg)
{
   ((ClientSession *)pArg)->UpdateThread();
   return THREAD_OK;
}


//
// Forced node poll thread starter
//

THREAD_RESULT THREAD_CALL ClientSession::PollerThreadStarter(void *pArg)
{
   ((POLLER_START_DATA *)pArg)->pSession->PollerThread(
      ((POLLER_START_DATA *)pArg)->pNode,
      ((POLLER_START_DATA *)pArg)->iPollType,
      ((POLLER_START_DATA *)pArg)->dwRqId);
   ((POLLER_START_DATA *)pArg)->pSession->DecRefCount();
   free(pArg);
   return THREAD_OK;
}


//
// Client session class constructor
//

ClientSession::ClientSession(SOCKET hSocket, DWORD dwHostAddr)
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
   m_mutexSendEvents = MutexCreate();
   m_mutexSendSyslog = MutexCreate();
   m_mutexSendObjects = MutexCreate();
   m_mutexSendAlarms = MutexCreate();
   m_mutexSendActions = MutexCreate();
   m_mutexPollerInit = MutexCreate();
   m_dwFlags = 0;
   m_dwHostAddr = dwHostAddr;
   strcpy(m_szUserName, "<not logged in>");
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
}


//
// Destructor
//

ClientSession::~ClientSession()
{
   if (m_hSocket != -1)
      closesocket(m_hSocket);
   delete m_pSendQueue;
   delete m_pMessageQueue;
   delete m_pUpdateQueue;
   safe_free(m_pMsgBuffer);
   MutexDestroy(m_mutexSendEvents);
   MutexDestroy(m_mutexSendSyslog);
   MutexDestroy(m_mutexSendObjects);
   MutexDestroy(m_mutexSendAlarms);
   MutexDestroy(m_mutexSendActions);
   MutexDestroy(m_mutexPollerInit);
   safe_free(m_pOpenDCIList);
   if (m_ppEPPRuleList != NULL)
   {
      DWORD i;

      if (m_dwFlags & CSF_EPP_UPLOAD)  // Aborted in the middle of EPP transfer
         for(i = 0; i < m_dwRecordsUploaded; i++)
            delete m_ppEPPRuleList[i];
      free(m_ppEPPRuleList);
   }
   DestroyEncryptionContext(m_pCtx);
}


//
// Start all threads
//

void ClientSession::Run(void)
{
   m_hWriteThread = ThreadCreateEx(WriteThreadStarter, 0, this);
   m_hProcessingThread = ThreadCreateEx(ProcessingThreadStarter, 0, this);
   m_hUpdateThread = ThreadCreateEx(UpdateThreadStarter, 0, this);
   ThreadCreate(ReadThreadStarter, 0, this);
}


//
// Print debug information
//

void ClientSession::DebugPrintf(char *szFormat, ...)
{
   if ((g_dwFlags & AF_STANDALONE) && (g_dwFlags & AF_DEBUG_CSCP))
   {
      va_list args;

      printf("*CSCP(%d)* ", m_dwIndex);
      va_start(args, szFormat);
      vprintf(szFormat, args);
      va_end(args);
   }
}


//
// ReadThread()
//

void ClientSession::ReadThread(void)
{
   CSCP_MESSAGE *pRawMsg;
   CSCPMessage *pMsg;
   BYTE *pDecryptionBuffer = NULL;
   TCHAR szBuffer[256];
   int iErr;
   DWORD i;
   NetObj *pObject;
   WORD wFlags;

   // Initialize raw message receiving function
   RecvCSCPMessage(0, NULL, m_pMsgBuffer, 0, NULL, NULL);

   pRawMsg = (CSCP_MESSAGE *)malloc(RAW_MSG_SIZE);
#ifdef _WITH_ENCRYPTION
   pDecryptionBuffer = (BYTE *)malloc(RAW_MSG_SIZE);
#endif
   while(1)
   {
      if ((iErr = RecvCSCPMessage(m_hSocket, pRawMsg, 
                                  m_pMsgBuffer, RAW_MSG_SIZE, 
                                  &m_pCtx, pDecryptionBuffer)) <= 0)
         break;

      // Check if message is too large
      if (iErr == 1)
      {
         DebugPrintf("Received message %s is too large (%d bytes)\n",
                     CSCPMessageCodeName(ntohs(pRawMsg->wCode), szBuffer),
                     ntohl(pRawMsg->dwSize));
         continue;
      }

      // Check for decryption error
      if (iErr == 2)
      {
         DebugPrintf("Unable to decrypt received message\n");
         continue;
      }

      // Check that actual received packet size is equal to encoded in packet
      if ((int)ntohl(pRawMsg->dwSize) != iErr)
      {
         DebugPrintf("Actual message size doesn't match wSize value (%d,%d)\n", iErr, ntohl(pRawMsg->dwSize));
         continue;   // Bad packet, wait for next
      }

      // Special handling for raw messages
      wFlags = ntohs(pRawMsg->wFlags);
      if (wFlags & MF_BINARY)
      {
         // Convert message header to host format
         pRawMsg->dwId = ntohl(pRawMsg->dwId);
         pRawMsg->wCode = ntohs(pRawMsg->wCode);
         pRawMsg->dwNumVars = ntohl(pRawMsg->dwNumVars);
         DebugPrintf("Received raw message %s\n", CSCPMessageCodeName(pRawMsg->wCode, szBuffer));

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
                        CSCPMessage msg;

                        close(m_hCurrFile);
                        m_hCurrFile = -1;
                  
                        msg.SetCode(CMD_REQUEST_COMPLETED);
                        msg.SetId(pRawMsg->dwId);
                        msg.SetVariable(VID_RCC, RCC_SUCCESS);
                        SendMessage(&msg);

                        OnFileUpload(TRUE);
                     }
                  }
                  else
                  {
                     // I/O error
                     CSCPMessage msg;

                     close(m_hCurrFile);
                     m_hCurrFile = -1;
               
                     msg.SetCode(CMD_REQUEST_COMPLETED);
                     msg.SetId(pRawMsg->dwId);
                     msg.SetVariable(VID_RCC, RCC_IO_ERROR);
                     SendMessage(&msg);

                     OnFileUpload(FALSE);
                  }
               }
               else
               {
                  // Abort current file transfer because of client's problem
                  close(m_hCurrFile);
                  m_hCurrFile = -1;
               
                  OnFileUpload(FALSE);
               }
            }
            else
            {
               DebugPrintf("Out of state message (ID: %d)\n", pRawMsg->dwId);
            }
         }
      }
      else
      {
         // Create message object from raw message
         pMsg = new CSCPMessage(pRawMsg);
         if ((pMsg->GetCode() == CMD_SESSION_KEY) && (pMsg->GetId() == m_dwEncryptionRqId))
         {
            m_dwEncryptionResult = SetupEncryptionContext(pMsg, &m_pCtx, NULL, g_pServerKey);
            ConditionSet(m_condEncryptionSetup);
            m_dwEncryptionRqId = 0;
            delete pMsg;
         }
         else
         {
            m_pMessageQueue->Put(pMsg);
         }
      }
   }
   if (iErr < 0)
      WriteLog(MSG_SESSION_CLOSED, EVENTLOG_WARNING_TYPE, "e", WSAGetLastError());
   free(pRawMsg);
#ifdef _WITH_ENCRYPTION
   free(pDecryptionBuffer);
#endif

   // Notify other threads to exit
   m_pSendQueue->Clear();
   m_pSendQueue->Put(INVALID_POINTER_VALUE);
   m_pMessageQueue->Clear();
   m_pMessageQueue->Put(INVALID_POINTER_VALUE);
   m_pUpdateQueue->Clear();
   m_pUpdateQueue->Put(INVALID_POINTER_VALUE);

   // Wait for other threads to finish
   ThreadJoin(m_hWriteThread);
   ThreadJoin(m_hProcessingThread);
   ThreadJoin(m_hUpdateThread);

   // Abort current file upload operation, if any
   if (m_hCurrFile != -1)
   {
      close(m_hCurrFile);
      m_hCurrFile = -1;
      OnFileUpload(FALSE);
   }

   // Remove all locks created by this session
   RemoveAllSessionLocks(m_dwIndex);
   for(i = 0; i < m_dwOpenDCIListSize; i++)
   {
      pObject = FindObjectById(m_pOpenDCIList[i]);
      if (pObject != NULL)
         if ((pObject->Type() == OBJECT_NODE) ||
             (pObject->Type() == OBJECT_TEMPLATE))
            ((Template *)pObject)->UnlockDCIList(m_dwIndex);
   }

   // Waiting while reference count becomes 0
   if (m_dwRefCount > 0)
   {
      DebugPrintf("Waiting for pending requests...\n");
      do
      {
         ThreadSleep(1);
      } while(m_dwRefCount > 0);
   }

   DebugPrintf("Session closed\n");
}


//
// WriteThread()
//

void ClientSession::WriteThread(void)
{
   CSCP_MESSAGE *pRawMsg;
   CSCP_ENCRYPTED_MESSAGE *pEnMsg;
   char szBuffer[128];
   BOOL bResult;

   while(1)
   {
      pRawMsg = (CSCP_MESSAGE *)m_pSendQueue->GetOrBlock();
      if (pRawMsg == INVALID_POINTER_VALUE)    // Session termination indicator
         break;

      DebugPrintf("Sending message %s\n", CSCPMessageCodeName(ntohs(pRawMsg->wCode), szBuffer));
      if (m_pCtx != NULL)
      {
         pEnMsg = CSCPEncryptMessage(m_pCtx, pRawMsg);
         if (pEnMsg != NULL)
         {
            bResult = (SendEx(m_hSocket, (char *)pEnMsg, ntohl(pEnMsg->dwSize), 0) == (int)ntohl(pEnMsg->dwSize));
            free(pEnMsg);
         }
         else
         {
            bResult = FALSE;
         }
      }
      else
      {
         bResult = (SendEx(m_hSocket, (const char *)pRawMsg, ntohl(pRawMsg->dwSize), 0) == (int)ntohl(pRawMsg->dwSize));
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


//
// Update processing thread
//

void ClientSession::UpdateThread(void)
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
            MutexLock(m_mutexSendEvents, INFINITE);
            m_pSendQueue->Put(CreateRawCSCPMessage(CMD_EVENT, 0, sizeof(NXC_EVENT), pUpdate->pData, NULL));
            MutexUnlock(m_mutexSendEvents);
            free(pUpdate->pData);
            break;
         case INFO_CAT_SYSLOG_MSG:
            MutexLock(m_mutexSendSyslog, INFINITE);
            msg.SetCode(CMD_SYSLOG_RECORDS);
            CreateMessageFromSyslogMsg(&msg, (NX_LOG_RECORD *)pUpdate->pData);
            SendMessage(&msg);
            MutexUnlock(m_mutexSendSyslog);
            free(pUpdate->pData);
            break;
         case INFO_CAT_OBJECT_CHANGE:
            MutexLock(m_mutexSendObjects, INFINITE);
            msg.SetCode(CMD_OBJECT_UPDATE);
            ((NetObj *)pUpdate->pData)->CreateMessage(&msg);
            SendMessage(&msg);
            MutexUnlock(m_mutexSendObjects);
            msg.DeleteAllVariables();
            ((NetObj *)pUpdate->pData)->DecRefCount();
            break;
         case INFO_CAT_ALARM:
            MutexLock(m_mutexSendAlarms, INFINITE);
            msg.SetCode(CMD_ALARM_UPDATE);
            msg.SetVariable(VID_NOTIFICATION_CODE, pUpdate->dwCode);
            FillAlarmInfoMessage(&msg, (NXC_ALARM *)pUpdate->pData);
            SendMessage(&msg);
            MutexUnlock(m_mutexSendAlarms);
            msg.DeleteAllVariables();
            free(pUpdate->pData);
            break;
         case INFO_CAT_ACTION:
            MutexLock(m_mutexSendActions, INFINITE);
            msg.SetCode(CMD_ACTION_DB_UPDATE);
            msg.SetVariable(VID_NOTIFICATION_CODE, pUpdate->dwCode);
            msg.SetVariable(VID_ACTION_ID, ((NXC_ACTION *)pUpdate->pData)->dwId);
            if (pUpdate->dwCode != NX_NOTIFY_ACTION_DELETED)
               FillActionInfoMessage(&msg, (NXC_ACTION *)pUpdate->pData);
            SendMessage(&msg);
            MutexUnlock(m_mutexSendActions);
            msg.DeleteAllVariables();
            free(pUpdate->pData);
            break;
         default:
            break;
      }

      free(pUpdate);
   }
}


//
// Message processing thread
//

void ClientSession::ProcessingThread(void)
{
   CSCPMessage *pMsg;
   char szBuffer[128];
   DWORD i;

   while(1)
   {
      pMsg = (CSCPMessage *)m_pMessageQueue->GetOrBlock();
      if (pMsg == INVALID_POINTER_VALUE)    // Session termination indicator
         break;

      DebugPrintf("Received message %s\n", CSCPMessageCodeName(pMsg->GetCode(), szBuffer));
      if (!(m_dwFlags & CSF_AUTHENTICATED) && 
          (pMsg->GetCode() != CMD_LOGIN) && 
          (pMsg->GetCode() != CMD_GET_SERVER_INFO) &&
          (pMsg->GetCode() != CMD_REQUEST_ENCRYPTION))
      {
         delete pMsg;
         continue;
      }

      m_wCurrentCmd = pMsg->GetCode();
      m_iState = SESSION_STATE_PROCESSING;
      switch(m_wCurrentCmd)
      {
         case CMD_LOGIN:
            Login(pMsg);
            break;
         case CMD_GET_SERVER_INFO:
            SendServerInfo(pMsg->GetId());
            break;
         case CMD_GET_OBJECTS:
            SendAllObjects(pMsg);
            break;
         case CMD_GET_EVENTS:
            SendAllEvents(pMsg);
            break;
         case CMD_GET_CONFIG_VARLIST:
            SendAllConfigVars(pMsg->GetId());
            break;
         case CMD_SET_CONFIG_VARIABLE:
            SetConfigVariable(pMsg);
            break;
         case CMD_DELETE_CONFIG_VARIABLE:
            DeleteConfigVariable(pMsg);
            break;
         case CMD_LOAD_EVENT_DB:
            SendEventDB(pMsg->GetId());
            break;
         case CMD_LOCK_EVENT_DB:
            LockEventDB(pMsg->GetId());
            break;
         case CMD_UNLOCK_EVENT_DB:
            UnlockEventDB(pMsg->GetId());
            break;
         case CMD_SET_EVENT_INFO:
            SetEventInfo(pMsg);
            break;
         case CMD_DELETE_EVENT_TEMPLATE:
            DeleteEventTemplate(pMsg);
            break;
         case CMD_GENERATE_EVENT_CODE:
            GenerateEventCode(pMsg->GetId());
            break;
         case CMD_MODIFY_OBJECT:
            ModifyObject(pMsg);
            break;
         case CMD_SET_OBJECT_MGMT_STATUS:
            ChangeObjectMgmtStatus(pMsg);
            break;
         case CMD_LOAD_USER_DB:
            SendUserDB(pMsg->GetId());
            break;
         case CMD_CREATE_USER:
            CreateUser(pMsg);
            break;
         case CMD_UPDATE_USER:
            UpdateUser(pMsg);
            break;
         case CMD_DELETE_USER:
            DeleteUser(pMsg);
            break;
         case CMD_LOCK_USER_DB:
            LockUserDB(pMsg->GetId(), TRUE);
            break;
         case CMD_UNLOCK_USER_DB:
            LockUserDB(pMsg->GetId(), FALSE);
            break;
         case CMD_SET_PASSWORD:
            SetPassword(pMsg);
            break;
         case CMD_GET_NODE_DCI_LIST:
            OpenNodeDCIList(pMsg);
            break;
         case CMD_UNLOCK_NODE_DCI_LIST:
            CloseNodeDCIList(pMsg);
            break;
         case CMD_CREATE_NEW_DCI:
         case CMD_MODIFY_NODE_DCI:
         case CMD_DELETE_NODE_DCI:
            ModifyNodeDCI(pMsg);
            break;
         case CMD_SET_DCI_STATUS:
            ChangeDCIStatus(pMsg);
            break;
         case CMD_COPY_DCI:
            CopyDCI(pMsg);
            break;
         case CMD_APPLY_TEMPLATE:
            ApplyTemplate(pMsg);
            break;
         case CMD_GET_DCI_DATA:
            GetCollectedData(pMsg);
            break;
         case CMD_OPEN_EPP:
            OpenEPP(pMsg->GetId());
            break;
         case CMD_CLOSE_EPP:
            CloseEPP(pMsg->GetId());
            break;
         case CMD_SAVE_EPP:
            SaveEPP(pMsg);
            break;
         case CMD_EPP_RECORD:
            ProcessEPPRecord(pMsg);
            break;
         case CMD_GET_MIB_LIST:
            SendMIBList(pMsg->GetId());
            break;
         case CMD_GET_MIB:
            SendMIB(pMsg);
            break;
         case CMD_CREATE_OBJECT:
            CreateObject(pMsg);
            break;
         case CMD_BIND_OBJECT:
            ChangeObjectBinding(pMsg, TRUE);
            break;
         case CMD_UNBIND_OBJECT:
            ChangeObjectBinding(pMsg, FALSE);
            break;
         case CMD_GET_IMAGE_LIST:
            SendImageCatalogue(this, pMsg->GetId(), pMsg->GetVariableShort(VID_IMAGE_FORMAT));
            break;
         case CMD_LOAD_IMAGE_FILE:
            SendImageFile(this, pMsg->GetId(), pMsg->GetVariableLong(VID_IMAGE_ID),
                          pMsg->GetVariableShort(VID_IMAGE_FORMAT));
            break;
         case CMD_GET_DEFAULT_IMAGE_LIST:
            SendDefaultImageList(this, pMsg->GetId());
            break;
         case CMD_GET_ALL_ALARMS:
            SendAllAlarms(pMsg->GetId(), pMsg->GetVariableShort(VID_IS_ACK));
            break;
         case CMD_GET_ALARM:
            break;
         case CMD_ACK_ALARM:
            AcknowlegeAlarm(pMsg);
            break;
         case CMD_DELETE_ALARM:
            DeleteAlarm(pMsg);
            break;
         case CMD_LOCK_ACTION_DB:
            LockActionDB(pMsg->GetId(), TRUE);
            break;
         case CMD_UNLOCK_ACTION_DB:
            LockActionDB(pMsg->GetId(), FALSE);
            break;
         case CMD_CREATE_ACTION:
            CreateAction(pMsg);
            break;
         case CMD_MODIFY_ACTION:
            UpdateAction(pMsg);
            break;
         case CMD_DELETE_ACTION:
            DeleteAction(pMsg);
            break;
         case CMD_LOAD_ACTIONS:
            SendAllActions(pMsg->GetId());
            break;
         case CMD_GET_CONTAINER_CAT_LIST:
            SendContainerCategories(pMsg->GetId());
            break;
         case CMD_DELETE_OBJECT:
            DeleteObject(pMsg);
            break;
         case CMD_POLL_NODE:
            ForcedNodePoll(pMsg);
            break;
         case CMD_TRAP:
            OnTrap(pMsg);
            break;
         case CMD_WAKEUP_NODE:
            OnWakeUpNode(pMsg);
            break;
         case CMD_LOCK_TRAP_CFG:
            LockTrapCfg(pMsg->GetId(), TRUE);
            break;
         case CMD_UNLOCK_TRAP_CFG:
            LockTrapCfg(pMsg->GetId(), FALSE);
            break;
         case CMD_CREATE_TRAP:
            EditTrap(TRAP_CREATE, pMsg);
            break;
         case CMD_MODIFY_TRAP:
            EditTrap(TRAP_UPDATE, pMsg);
            break;
         case CMD_DELETE_TRAP:
            EditTrap(TRAP_DELETE, pMsg);
            break;
         case CMD_LOAD_TRAP_CFG:
            SendAllTraps(pMsg->GetId());
            break;
         case CMD_QUERY_PARAMETER:
            QueryParameter(pMsg);
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
            SendParametersList(pMsg);
            break;
         case CMD_DEPLOY_PACKAGE:
            DeployPackage(pMsg);
            break;
         case CMD_GET_LAST_VALUES:
            SendLastValues(pMsg);
            break;
         case CMD_GET_USER_VARIABLE:
            GetUserVariable(pMsg);
            break;
         case CMD_SET_USER_VARIABLE:
            SetUserVariable(pMsg);
            break;
         case CMD_DELETE_USER_VARIABLE:
            DeleteUserVariable(pMsg);
            break;
         case CMD_ENUM_USER_VARIABLES:
            EnumUserVariables(pMsg);
            break;
         case CMD_CHANGE_IP_ADDR:
            ChangeObjectIP(pMsg);
            break;
         case CMD_REQUEST_ENCRYPTION:
            SetupEncryption(pMsg->GetId());
            break;
         case CMD_GET_AGENT_CONFIG:
            GetAgentConfig(pMsg);
            break;
         case CMD_UPDATE_AGENT_CONFIG:
            UpdateAgentConfig(pMsg);
            break;
         case CMD_EXECUTE_ACTION:
            ExecuteAction(pMsg);
            break;
         case CMD_GET_OBJECT_TOOLS:
            SendObjectTools(pMsg->GetId());
            break;
         case CMD_EXEC_TABLE_TOOL:
            ExecTableTool(pMsg);
            break;
         case CMD_CHANGE_SUBSCRIPTION:
            ChangeSubscription(pMsg);
            break;
         case CMD_GET_SYSLOG:
            SendSyslog(pMsg);
            break;
         case CMD_GET_LPP_LIST:
            SendLogPoliciesList(pMsg->GetId());
            break;
         default:
            // Pass message to loaded modules
            for(i = 0; i < g_dwNumModules; i++)
               if (g_pModuleList[i].pfCommandHandler(m_wCurrentCmd, pMsg, this))
                  break;   // Message was processed by the module
            if (i == g_dwNumModules)
            {
               CSCPMessage response;

               response.SetId(pMsg->GetId());
               response.SetCode(CMD_REQUEST_COMPLETED);
               response.SetVariable(VID_RCC, RCC_NOT_IMPLEMENTED);
               SendMessage(&response);
            }
            break;
      }
      delete pMsg;
      m_iState = (m_dwFlags & CSF_AUTHENTICATED) ? SESSION_STATE_IDLE : SESSION_STATE_INIT;
   }
}


//
// Process received file
//

void ClientSession::OnFileUpload(BOOL bSuccess)
{
   // Do processing specific to command initiated file upload
   switch(m_dwUploadCommand)
   {
      case CMD_INSTALL_PACKAGE:
         if (!bSuccess)
         {
            TCHAR szQuery[256];

            _sntprintf(szQuery, 256, _T("DELETE FROM agent_pkg WHERE pkg_id=%ld"), m_dwUploadData);
            DBQuery(g_hCoreDB, szQuery);
         }
         break;
      default:
         break;
   }

   // Remove received file in case of failure
   if (!bSuccess)
      _tunlink(m_szCurrFileName);
}


//
// Send server information to client
//

void ClientSession::SendServerInfo(DWORD dwRqId)
{
   CSCPMessage msg;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(dwRqId);

   // Fill message with server info
   msg.SetVariable(VID_RCC, RCC_SUCCESS);
   msg.SetVariable(VID_SERVER_VERSION, NETXMS_VERSION_STRING);
   msg.SetVariable(VID_SERVER_ID, (BYTE *)&g_qwServerId, sizeof(QWORD));
   msg.SetVariable(VID_SUPPORTED_ENCRYPTION, (DWORD)0);
   msg.SetVariable(VID_PROTOCOL_VERSION, (DWORD)CLIENT_PROTOCOL_VERSION);

   // Send response
   SendMessage(&msg);
}


//
// Authenticate client
//

void ClientSession::Login(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   BYTE szPassword[SHA1_DIGEST_SIZE];
   char szLogin[MAX_USER_NAME], szBuffer[32];

   // Prepare response message
   msg.SetCode(CMD_LOGIN_RESP);
   msg.SetId(pRequest->GetId());

   // Get client info string
   if (pRequest->IsVariableExist(VID_CLIENT_INFO))
   {
      TCHAR szClientInfo[32], szOSInfo[32], szLibVersion[16];
      
      pRequest->GetVariableStr(VID_CLIENT_INFO, szClientInfo, 32);
      pRequest->GetVariableStr(VID_OS_INFO, szOSInfo, 32);
      pRequest->GetVariableStr(VID_LIBNXCL_VERSION, szLibVersion, 16);
      _sntprintf(m_szClientInfo, 96, _T("%s (%s; libnxcl %s)"),
                 szClientInfo, szOSInfo, szLibVersion);
   }

   if (!(m_dwFlags & CSF_AUTHENTICATED))
   {
      
      pRequest->GetVariableStr(VID_LOGIN_NAME, szLogin, MAX_USER_NAME);
      pRequest->GetVariableBinary(VID_PASSWORD, szPassword, SHA1_DIGEST_SIZE);

      if (AuthenticateUser(szLogin, szPassword, &m_dwUserId, &m_dwSystemAccess))
      {
         m_dwFlags |= CSF_AUTHENTICATED;
         sprintf(m_szUserName, "%s@%s", szLogin, IpToStr(m_dwHostAddr, szBuffer));
         msg.SetVariable(VID_RCC, RCC_SUCCESS);
         DebugPrintf("User %s authenticated\n", m_szUserName);
      }
      else
      {
         msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
      }
   }
   else
   {
      msg.SetVariable(VID_RCC, RCC_OUT_OF_STATE_REQUEST);
   }

   // Send response
   SendMessage(&msg);
}


//
// Send event configuration to client
//

void ClientSession::SendEventDB(DWORD dwRqId)
{
   DB_ASYNC_RESULT hResult;
   CSCPMessage msg;
   char szBuffer[1024];

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(dwRqId);

   if (!CheckSysAccessRights(SYSTEM_ACCESS_VIEW_EVENT_DB))
   {
      msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
      SendMessage(&msg);
   }
   else
   {
      msg.SetVariable(VID_RCC, RCC_SUCCESS);
      SendMessage(&msg);
      msg.DeleteAllVariables();

      // Prepare data message
      msg.SetCode(CMD_EVENT_DB_RECORD);
      msg.SetId(dwRqId);

      hResult = DBAsyncSelect(g_hCoreDB, "SELECT event_code,event_name,severity,flags,message,description FROM event_cfg");
      if (hResult != NULL)
      {
         while(DBFetch(hResult))
         {
            msg.SetVariable(VID_EVENT_CODE, DBGetFieldAsyncULong(hResult, 0));
            msg.SetVariable(VID_NAME, DBGetFieldAsync(hResult, 1, szBuffer, 1024));
            msg.SetVariable(VID_SEVERITY, DBGetFieldAsyncULong(hResult, 2));
            msg.SetVariable(VID_FLAGS, DBGetFieldAsyncULong(hResult, 3));

            DBGetFieldAsync(hResult, 4, szBuffer, 1024);
            DecodeSQLString(szBuffer);
            msg.SetVariable(VID_MESSAGE, szBuffer);

            DBGetFieldAsync(hResult, 5, szBuffer, 1024);
            DecodeSQLString(szBuffer);
            msg.SetVariable(VID_DESCRIPTION, szBuffer);

            SendMessage(&msg);
            msg.DeleteAllVariables();
         }
         DBFreeAsyncResult(hResult);
      }

      // Send end-of-list indicator
      msg.SetVariable(VID_EVENT_CODE, (DWORD)0);
      SendMessage(&msg);
   }
}


//
// Lock event configuration database
//

void ClientSession::LockEventDB(DWORD dwRqId)
{
   CSCPMessage msg;
   char szBuffer[1024];

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(dwRqId);

   if (!CheckSysAccessRights(SYSTEM_ACCESS_EDIT_EVENT_DB))
   {
      msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
   }
   else if (!LockComponent(CID_EVENT_DB, m_dwIndex, m_szUserName, NULL, szBuffer))
   {
      msg.SetVariable(VID_RCC, RCC_COMPONENT_LOCKED);
      msg.SetVariable(VID_LOCKED_BY, szBuffer);
   }
   else
   {
      m_dwFlags |= CSF_EVENT_DB_LOCKED;
      msg.SetVariable(VID_RCC, RCC_SUCCESS);
   }
   SendMessage(&msg);
}


//
// Close event configuration database
//

void ClientSession::UnlockEventDB(DWORD dwRqId)
{
   CSCPMessage msg;

   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(dwRqId);

   if (m_dwFlags & CSF_EVENT_DB_LOCKED)
   {
      UnlockComponent(CID_EVENT_DB);
      m_dwFlags &= ~CSF_EVENT_DB_LOCKED;
      msg.SetVariable(VID_RCC, RCC_SUCCESS);
   }
   else
   {
      msg.SetVariable(VID_RCC, RCC_OUT_OF_STATE_REQUEST);
   }
   SendMessage(&msg);
}


//
// Update event template
//

void ClientSession::SetEventInfo(CSCPMessage *pRequest)
{
   CSCPMessage msg;

   // Prepare reply message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   // Check if we have event configuration database opened
   if (!(m_dwFlags & CSF_EVENT_DB_LOCKED))
   {
      msg.SetVariable(VID_RCC, RCC_OUT_OF_STATE_REQUEST);
   }
   else
   {
      // Check access rights
      if (CheckSysAccessRights(SYSTEM_ACCESS_EDIT_EVENT_DB))
      {
         char szQuery[4096], szName[MAX_EVENT_NAME];
         DWORD dwEventCode;
         BOOL bEventExist = FALSE;
         DB_RESULT hResult;

         // Check if event with specific code exists
         dwEventCode = pRequest->GetVariableLong(VID_EVENT_CODE);
         sprintf(szQuery, "SELECT event_code FROM event_cfg WHERE event_code=%ld", dwEventCode);
         hResult = DBSelect(g_hCoreDB, szQuery);
         if (hResult != NULL)
         {
            if (DBGetNumRows(hResult) > 0)
               bEventExist = TRUE;
            DBFreeResult(hResult);
         }

         // Check that we are not trying to create event below 100000
         if (bEventExist || (dwEventCode >= FIRST_USER_EVENT_ID))
         {
            // Prepare and execute SQL query
            pRequest->GetVariableStr(VID_NAME, szName, MAX_EVENT_NAME);
            if (IsValidObjectName(szName))
            {
               char szMessage[MAX_DB_STRING], *pszDescription, *pszEscMsg, *pszEscDescr;

               pRequest->GetVariableStr(VID_MESSAGE, szMessage, MAX_DB_STRING);
               pszEscMsg = EncodeSQLString(szMessage);

               pszDescription = pRequest->GetVariableStr(VID_DESCRIPTION);
               pszEscDescr = EncodeSQLString(pszDescription);
               safe_free(pszDescription);

               if (bEventExist)
               {
                  sprintf(szQuery, "UPDATE event_cfg SET event_name='%s',severity=%ld,flags=%ld,message='%s',description='%s' WHERE event_code=%ld",
                          szName, pRequest->GetVariableLong(VID_SEVERITY), pRequest->GetVariableLong(VID_FLAGS),
                          pszEscMsg, pszEscDescr, dwEventCode);
               }
               else
               {
                  sprintf(szQuery, "INSERT INTO event_cfg (event_code,event_name,severity,flags,"
                                   "message,description) VALUES (%ld,'%s',%ld,%ld,'%s','%s')",
                          dwEventCode, szName, pRequest->GetVariableLong(VID_SEVERITY),
                          pRequest->GetVariableLong(VID_FLAGS), pszEscMsg, pszEscDescr);
               }

               free(pszEscMsg);
               free(pszEscDescr);

               if (DBQuery(g_hCoreDB, szQuery))
               {
                  msg.SetVariable(VID_RCC, RCC_SUCCESS);
                  ReloadEvents();
                  EnumerateClientSessions(NotifyClient, (void *)NX_NOTIFY_EVENTDB_CHANGED);
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
      }
      else
      {
         msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
      }
   }

   // Send response
   SendMessage(&msg);
}


//
// Delete event template
//

void ClientSession::DeleteEventTemplate(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   DWORD dwEventCode;

   // Prepare reply message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   // Check if we have event configuration database opened
   if (!(m_dwFlags & CSF_EVENT_DB_LOCKED))
   {
      msg.SetVariable(VID_RCC, RCC_OUT_OF_STATE_REQUEST);
   }
   else
   {
      dwEventCode = pRequest->GetVariableLong(VID_EVENT_CODE);

      // Check access rights
      if (CheckSysAccessRights(SYSTEM_ACCESS_EDIT_EVENT_DB) && (dwEventCode >= FIRST_USER_EVENT_ID))
      {
         TCHAR szQuery[256];

         _stprintf(szQuery, _T("DELETE FROM event_cfg WHERE event_code=%ld"), dwEventCode);
         if (DBQuery(g_hCoreDB, szQuery))
         {
            DeleteEventTemplateFromList(dwEventCode);
            EnumerateClientSessions(NotifyClient, (void *)NX_NOTIFY_EVENTDB_CHANGED);
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
   }

   // Send response
   SendMessage(&msg);
}


//
// Generate ID for new event template
//

void ClientSession::GenerateEventCode(DWORD dwRqId)
{
   CSCPMessage msg;

   // Prepare reply message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(dwRqId);

   // Check if we have event configuration database opened
   if (!(m_dwFlags & CSF_EVENT_DB_LOCKED))
   {
      msg.SetVariable(VID_RCC, RCC_OUT_OF_STATE_REQUEST);
   }
   else
   {
      // Check access rights
      if (CheckSysAccessRights(SYSTEM_ACCESS_EDIT_EVENT_DB))
      {
         msg.SetVariable(VID_EVENT_CODE, CreateUniqueId(IDG_EVENT));
      }
      else
      {
         msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
      }
   }

   // Send response
   SendMessage(&msg);
}


//
// Send all objects to client
//

void ClientSession::SendAllObjects(CSCPMessage *pRequest)
{
   DWORD i, dwTimeStamp;
   CSCPMessage msg;

   // Send confirmation message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());
   msg.SetVariable(VID_RCC, RCC_SUCCESS);
   SendMessage(&msg);
   msg.DeleteAllVariables();

   // Get client's last known time stamp
   dwTimeStamp = pRequest->GetVariableLong(VID_TIMESTAMP);

   MutexLock(m_mutexSendObjects, INFINITE);

   // Prepare message
   msg.SetCode(CMD_OBJECT);

   // Send objects, one per message
   RWLockReadLock(g_rwlockIdIndex, INFINITE);
   for(i = 0; i < g_dwIdIndexSize; i++)
      if ((g_pIndexById[i].pObject->CheckAccessRights(m_dwUserId, OBJECT_ACCESS_READ)) &&
          (g_pIndexById[i].pObject->TimeStamp() >= dwTimeStamp) &&
          (!g_pIndexById[i].pObject->IsHidden()))
      {
         g_pIndexById[i].pObject->CreateMessage(&msg);
         SendMessage(&msg);
         msg.DeleteAllVariables();
      }
   RWLockUnlock(g_rwlockIdIndex);

   // Send end of list notification
   msg.SetCode(CMD_OBJECT_LIST_END);
   SendMessage(&msg);

   MutexUnlock(m_mutexSendObjects);
}


//
// Send all events to client
//

void ClientSession::SendAllEvents(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   DB_ASYNC_RESULT hResult = NULL;
   DB_RESULT hTempResult;
   NXC_EVENT event;
   DWORD dwRqId, dwMaxRecords, dwNumRows;
   TCHAR szQuery[1024];
#ifndef UNICODE
   char szBuffer[MAX_EVENT_MSG_LENGTH];
#endif

   dwRqId = pRequest->GetId();
   dwMaxRecords = pRequest->GetVariableLong(VID_MAX_RECORDS);

   // Send confirmation message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_RCC, RCC_SUCCESS);
   SendMessage(&msg);
   msg.DeleteAllVariables();

   MutexLock(m_mutexSendEvents, INFINITE);

   // Retrieve events from database
   switch(g_dwDBSyntax)
   {
      case DB_SYNTAX_MYSQL:
      case DB_SYNTAX_PGSQL:
         hTempResult = DBSelect(g_hCoreDB, _T("SELECT count(*) FROM event_log"));
         if (hTempResult != NULL)
         {
            if (DBGetNumRows(hTempResult) > 0)
            {
               dwNumRows = DBGetFieldULong(hTempResult, 0, 0);
            }
            else
            {
               dwNumRows = 0;
            }
            DBFreeResult(hTempResult);
         }
         _sntprintf(szQuery, 1024,
                    _T("SELECT event_id,event_code,event_timestamp,event_source,")
                    _T("event_severity,event_message FROM event_log ")
                    _T("ORDER BY event_timestamp LIMIT %lu OFFSET %lu"),
                    dwMaxRecords, dwNumRows - min(dwNumRows, dwMaxRecords));
         break;
      case DB_SYNTAX_MSSQL:
         _sntprintf(szQuery, 1024,
                    _T("SELECT event_id,event_code,event_timestamp,event_source,")
                    _T("event_severity,event_message INTO temp_log_%ld FROM event_log ")
                    _T("ORDER BY event_timestamp DESC LIMIT %lu;"),
                    _T("SELECT * FROM temp_log_%ld ORDER BY event_timestamp;")
                    _T("DROP TABLE temp_log_%ld"),
                    m_dwIndex, dwMaxRecords, m_dwIndex, m_dwIndex);
         break;
      default:
         szQuery[0] = 0;
         break;
   }
   hResult = DBAsyncSelect(g_hCoreDB, szQuery);
   if (hResult != NULL)
   {
      // Send events, one per message
      while(DBFetch(hResult))
      {
         event.qwEventId = htonq(DBGetFieldAsyncUInt64(hResult, 0));
         event.dwEventCode = htonl(DBGetFieldAsyncULong(hResult, 1));
         event.dwTimeStamp = htonl(DBGetFieldAsyncULong(hResult, 2));
         event.dwSourceId = htonl(DBGetFieldAsyncULong(hResult, 3));
         event.dwSeverity = htonl(DBGetFieldAsyncULong(hResult, 4));
#ifdef UNICODE
         DBGetFieldAsync(hResult, 5, event.szMessage, MAX_EVENT_MSG_LENGTH);
         DecodeSQLString(event.szMessage);
#else
         DBGetFieldAsync(hResult, 5, szBuffer, MAX_EVENT_MSG_LENGTH);
         DecodeSQLString(szBuffer);
         MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, szBuffer, -1, 
                             (WCHAR *)event.szMessage, MAX_EVENT_MSG_LENGTH);
         ((WCHAR *)event.szMessage)[MAX_EVENT_MSG_LENGTH - 1] = 0;
#endif
         SwapWideString((WCHAR *)event.szMessage);
         m_pSendQueue->Put(CreateRawCSCPMessage(CMD_EVENT, dwRqId, sizeof(NXC_EVENT), &event, NULL));
      }
      DBFreeAsyncResult(hResult);
   }

   // Send end of list notification
   msg.SetCode(CMD_EVENT_LIST_END);
   SendMessage(&msg);

   MutexUnlock(m_mutexSendEvents);
}


//
// Send all configuration variables to client
//

void ClientSession::SendAllConfigVars(DWORD dwRqId)
{
   DWORD i, dwId, dwNumRecords;
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
      hResult = DBSelect(g_hCoreDB, "SELECT var_name,var_value,need_server_restart FROM config WHERE is_visible=1");
      if (hResult != NULL)
      {
         // Send events, one per message
         dwNumRecords = DBGetNumRows(hResult);
         msg.SetVariable(VID_NUM_VARIABLES, dwNumRecords);
         for(i = 0, dwId = VID_VARLIST_BASE; i < dwNumRecords; i++)
         {
            msg.SetVariable(dwId++, DBGetField(hResult, i, 0));
            _tcsncpy(szBuffer, DBGetField(hResult, i, 1), MAX_DB_STRING);
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
   SendMessage(&msg);
}


//
// Set configuration variable's value
//

void ClientSession::SetConfigVariable(CSCPMessage *pRequest)
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
         msg.SetVariable(VID_RCC, RCC_SUCCESS);
      else
         msg.SetVariable(VID_RCC, RCC_DB_FAILURE);
   }
   else
   {
      msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
   }

   // Send response
   SendMessage(&msg);
}


//
// Delete configuration variable
//

void ClientSession::DeleteConfigVariable(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   TCHAR szName[MAX_OBJECT_NAME], szQuery[1024];

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   // Check user rights
   if ((m_dwUserId == 0) || (m_dwSystemAccess & SYSTEM_ACCESS_SERVER_CONFIG))
   {
      pRequest->GetVariableStr(VID_NAME, szName, MAX_OBJECT_NAME);
      _sntprintf(szQuery, 1024, _T("DELETE FROM config WHERE var_name='%s'"), szName);
      if (DBQuery(g_hCoreDB, szQuery))
         msg.SetVariable(VID_RCC, RCC_SUCCESS);
      else
         msg.SetVariable(VID_RCC, RCC_DB_FAILURE);
   }
   else
   {
      msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
   }

   // Send response
   SendMessage(&msg);
}


//
// Close session forcibly
//

void ClientSession::Kill(void)
{
   // We shutdown socket connection, which will cause
   // read thread to stop, and other threads will follow
   shutdown(m_hSocket, 2);
}


//
// Handler for new events
//

void ClientSession::OnNewEvent(Event *pEvent)
{
   UPDATE_INFO *pUpdate;

   if (IsAuthenticated() && (m_dwActiveChannels & NXC_CHANNEL_EVENTS))
   {
      pUpdate = (UPDATE_INFO *)malloc(sizeof(UPDATE_INFO));
      pUpdate->dwCategory = INFO_CAT_EVENT;
      pUpdate->pData = malloc(sizeof(NXC_EVENT));
      pEvent->PrepareMessage((NXC_EVENT *)pUpdate->pData);
      m_pUpdateQueue->Put(pUpdate);
   }
}


//
// Handler for new syslog messages
//

void ClientSession::OnSyslogMessage(NX_LOG_RECORD *pRec)
{
   UPDATE_INFO *pUpdate;

   if (IsAuthenticated() && (m_dwActiveChannels & NXC_CHANNEL_SYSLOG))
   {
      pUpdate = (UPDATE_INFO *)malloc(sizeof(UPDATE_INFO));
      pUpdate->dwCategory = INFO_CAT_SYSLOG_MSG;
      pUpdate->pData = nx_memdup(pRec, sizeof(NX_LOG_RECORD));
      m_pUpdateQueue->Put(pUpdate);
   }
}


//
// Handler for object changes
//

void ClientSession::OnObjectChange(NetObj *pObject)
{
   UPDATE_INFO *pUpdate;

   if (IsAuthenticated() && (m_dwActiveChannels & NXC_CHANNEL_OBJECTS))
      if (pObject->CheckAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
      {
         pUpdate = (UPDATE_INFO *)malloc(sizeof(UPDATE_INFO));
         pUpdate->dwCategory = INFO_CAT_OBJECT_CHANGE;
         pUpdate->pData = pObject;
         pObject->IncRefCount();
         m_pUpdateQueue->Put(pUpdate);
      }
}


//
// Send notification message to server
//

void ClientSession::Notify(DWORD dwCode, DWORD dwData)
{
   CSCPMessage msg;

   msg.SetCode(CMD_NOTIFY);
   msg.SetVariable(VID_NOTIFICATION_CODE, dwCode);
   msg.SetVariable(VID_NOTIFICATION_DATA, dwData);
   SendMessage(&msg);
}


//
// Modify object
//

void ClientSession::ModifyObject(CSCPMessage *pRequest)
{
   DWORD dwObjectId, dwResult = RCC_SUCCESS;
   NetObj *pObject;
   CSCPMessage msg;

   // Prepare reply message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   dwObjectId = pRequest->GetVariableLong(VID_OBJECT_ID);
   pObject = FindObjectById(dwObjectId);
   if (pObject != NULL)
   {
      if (pObject->CheckAccessRights(m_dwUserId, OBJECT_ACCESS_MODIFY))
      {
         // If user attempts to change object's ACL, check
         // if he has OBJECT_ACCESS_CONTROL permission
         if (pRequest->IsVariableExist(VID_ACL_SIZE))
            if (!pObject->CheckAccessRights(m_dwUserId, OBJECT_ACCESS_CONTROL))
               dwResult = RCC_ACCESS_DENIED;

         // If allowed, change object and set completion code
         if (dwResult != RCC_ACCESS_DENIED)
            dwResult = pObject->ModifyFromMessage(pRequest);
         msg.SetVariable(VID_RCC, dwResult);
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
   SendMessage(&msg);
}


//
// Send users database to client
//

void ClientSession::SendUserDB(DWORD dwRqId)
{
   CSCPMessage msg;
   DWORD i;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_RCC, RCC_SUCCESS);
   SendMessage(&msg);

   // Send users
   msg.SetCode(CMD_USER_DATA);
   for(i = 0; i < g_dwNumUsers; i++)
   {
      FillUserInfoMessage(&msg, &g_pUserList[i]);
      SendMessage(&msg);
      msg.DeleteAllVariables();
   }

   // Send groups
   msg.SetCode(CMD_GROUP_DATA);
   for(i = 0; i < g_dwNumGroups; i++)
   {
      FillGroupInfoMessage(&msg, &g_pGroupList[i]);
      SendMessage(&msg);
      msg.DeleteAllVariables();
   }

   // Send end-of-database notification
   msg.SetCode(CMD_USER_DB_EOF);
   SendMessage(&msg);
}


//
// Create new user
//

void ClientSession::CreateUser(CSCPMessage *pRequest)
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
      DWORD dwResult, dwUserId;
      BOOL bIsGroup;
      char szUserName[MAX_USER_NAME];

      pRequest->GetVariableStr(VID_USER_NAME, szUserName, MAX_USER_NAME);
      if (IsValidObjectName(szUserName))
      {
         bIsGroup = pRequest->GetVariableShort(VID_IS_GROUP);
         dwResult = CreateNewUser(szUserName, bIsGroup, &dwUserId);
         msg.SetVariable(VID_RCC, dwResult);
         if (dwResult == RCC_SUCCESS)
            msg.SetVariable(VID_USER_ID, dwUserId);   // Send id of new user to client
      }
      else
      {
         msg.SetVariable(VID_RCC, RCC_INVALID_OBJECT_NAME);
      }
   }

   // Send response
   SendMessage(&msg);
}


//
// Update existing user's data
//

void ClientSession::UpdateUser(CSCPMessage *pRequest)
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
      DWORD dwUserId, dwResult;

      dwUserId = pRequest->GetVariableLong(VID_USER_ID);
      if (dwUserId & GROUP_FLAG)
      {
         NMS_USER_GROUP group;
         DWORD i, dwId;

         group.dwId = dwUserId;
         pRequest->GetVariableStr(VID_USER_DESCRIPTION, group.szDescription, MAX_USER_DESCR);
         pRequest->GetVariableStr(VID_USER_NAME, group.szName, MAX_USER_NAME);
         group.wFlags = pRequest->GetVariableShort(VID_USER_FLAGS);
         group.wSystemRights = pRequest->GetVariableShort(VID_USER_SYS_RIGHTS);
         group.dwNumMembers = pRequest->GetVariableLong(VID_NUM_MEMBERS);
         group.pMembers = (DWORD *)malloc(sizeof(DWORD) * group.dwNumMembers);
         for(i = 0, dwId = VID_GROUP_MEMBER_BASE; i < group.dwNumMembers; i++, dwId++)
            group.pMembers[i] = pRequest->GetVariableLong(dwId);
         dwResult = ModifyGroup(&group);
         safe_free(group.pMembers);
      }
      else
      {
         NMS_USER user;

         user.dwId = dwUserId;
         pRequest->GetVariableStr(VID_USER_DESCRIPTION, user.szDescription, MAX_USER_DESCR);
         pRequest->GetVariableStr(VID_USER_FULL_NAME, user.szFullName, MAX_USER_FULLNAME);
         pRequest->GetVariableStr(VID_USER_NAME, user.szName, MAX_USER_NAME);
         user.wFlags = pRequest->GetVariableShort(VID_USER_FLAGS);
         user.wSystemRights = pRequest->GetVariableShort(VID_USER_SYS_RIGHTS);
         dwResult = ModifyUser(&user);
      }
      msg.SetVariable(VID_RCC, dwResult);
   }

   // Send response
   SendMessage(&msg);
}


//
// Delete user
//

void ClientSession::DeleteUser(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   DWORD dwUserId;

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

      if (dwUserId != 0)
      {
         DWORD dwResult;

         dwResult = DeleteUserFromDB(dwUserId);
         msg.SetVariable(VID_RCC, dwResult);
      }
      else
      {
         // Nobody can delete system administrator account
         msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
      }
   }

   // Send response
   SendMessage(&msg);
}


//
// Lock/unlock user database
//

void ClientSession::LockUserDB(DWORD dwRqId, BOOL bLock)
{
   CSCPMessage msg;
   char szBuffer[256];

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
   SendMessage(&msg);
}


//
// Notify client on user database update
//

void ClientSession::OnUserDBUpdate(int iCode, DWORD dwUserId, NMS_USER *pUser, NMS_USER_GROUP *pGroup)
{
   CSCPMessage msg;

   if (IsAuthenticated())
   {
      msg.SetCode(CMD_USER_DB_UPDATE);
      msg.SetId(0);
      msg.SetVariable(VID_UPDATE_TYPE, (WORD)iCode);

      switch(iCode)
      {
         case USER_DB_CREATE:
            msg.SetVariable(VID_USER_ID, dwUserId);
            if (dwUserId & GROUP_FLAG)
               msg.SetVariable(VID_USER_NAME, pGroup->szName);
            else
               msg.SetVariable(VID_USER_NAME, pUser->szName);
            break;
         case USER_DB_MODIFY:
            if (dwUserId & GROUP_FLAG)
               FillGroupInfoMessage(&msg, pGroup);
            else
               FillUserInfoMessage(&msg, pUser);
            break;
         default:
            msg.SetVariable(VID_USER_ID, dwUserId);
            break;
      }

      SendMessage(&msg);
   }
}


//
// Change management status for the object
//

void ClientSession::ChangeObjectMgmtStatus(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   DWORD dwObjectId;
   NetObj *pObject;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   // Get object id and check access rights
   dwObjectId = pRequest->GetVariableLong(VID_OBJECT_ID);
   pObject = FindObjectById(dwObjectId);
   if (pObject != NULL)
   {
      if (pObject->CheckAccessRights(m_dwUserId, OBJECT_ACCESS_MODIFY))
      {
         BOOL bIsManaged;

         bIsManaged = (BOOL)pRequest->GetVariableShort(VID_MGMT_STATUS);
         pObject->SetMgmtStatus(bIsManaged);
         msg.SetVariable(VID_RCC, RCC_SUCCESS);
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
   SendMessage(&msg);
}


// 
// Set user's password
//

void ClientSession::SetPassword(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   DWORD dwUserId;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   dwUserId = pRequest->GetVariableLong(VID_USER_ID);

   if (((m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_USERS) &&
        !((dwUserId == 0) && (m_dwUserId != 0))) ||   // Only administrator can change password for UID 0
       (dwUserId == m_dwUserId))     // User can change password for itself
   {
      DWORD dwResult;
      BYTE szPassword[SHA1_DIGEST_SIZE];

      pRequest->GetVariableBinary(VID_PASSWORD, szPassword, SHA1_DIGEST_SIZE);
      dwResult = SetUserPassword(dwUserId, szPassword);
      msg.SetVariable(VID_RCC, dwResult);
   }
   else
   {
      // Current user has no rights to change password for specific user
      msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
   }

   // Send response
   SendMessage(&msg);
}


//
// Send node's DCIs to client and lock data collection settings
//

void ClientSession::OpenNodeDCIList(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   DWORD dwObjectId;
   NetObj *pObject;
   BOOL bSuccess = FALSE;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   // Get node id and check object class and access rights
   dwObjectId = pRequest->GetVariableLong(VID_OBJECT_ID);
   pObject = FindObjectById(dwObjectId);
   if (pObject != NULL)
   {
      if ((pObject->Type() == OBJECT_NODE) ||
          (pObject->Type() == OBJECT_TEMPLATE))
      {
         if (pObject->CheckAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
         {
            // Try to lock DCI list
            bSuccess = ((Template *)pObject)->LockDCIList(m_dwIndex);
            msg.SetVariable(VID_RCC, bSuccess ? RCC_SUCCESS : RCC_COMPONENT_LOCKED);

            // Modify list of open nodes DCI lists
            if (bSuccess)
            {
               m_pOpenDCIList = (DWORD *)realloc(m_pOpenDCIList, sizeof(DWORD) * (m_dwOpenDCIListSize + 1));
               m_pOpenDCIList[m_dwOpenDCIListSize] = dwObjectId;
               m_dwOpenDCIListSize++;
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
   SendMessage(&msg);

   // If DCI list was successfully locked, send it to client
   if (bSuccess)
      ((Template *)pObject)->SendItemsToClient(this, pRequest->GetId());
}


//
// Unlock node's data collection settings
//

void ClientSession::CloseNodeDCIList(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   DWORD dwObjectId;
   NetObj *pObject;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   // Get node id and check object class and access rights
   dwObjectId = pRequest->GetVariableLong(VID_OBJECT_ID);
   pObject = FindObjectById(dwObjectId);
   if (pObject != NULL)
   {
      if ((pObject->Type() == OBJECT_NODE) ||
          (pObject->Type() == OBJECT_TEMPLATE))
      {
         if (pObject->CheckAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
         {
            BOOL bSuccess;

            // Try to unlock DCI list
            bSuccess = ((Template *)pObject)->UnlockDCIList(m_dwIndex);
            msg.SetVariable(VID_RCC, bSuccess ? RCC_SUCCESS : RCC_OUT_OF_STATE_REQUEST);

            // Modify list of open nodes DCI lists
            if (bSuccess)
            {
               DWORD i;

               for(i = 0; i < m_dwOpenDCIListSize; i++)
                  if (m_pOpenDCIList[i] == dwObjectId)
                  {
                     m_dwOpenDCIListSize--;
                     memmove(&m_pOpenDCIList[i], &m_pOpenDCIList[i + 1],
                             sizeof(DWORD) * (m_dwOpenDCIListSize - i));
                     break;
                  }
            }

            // Queue template update
            if (pObject->Type() == OBJECT_TEMPLATE)
               ((Template *)pObject)->QueueUpdate();
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
   SendMessage(&msg);
}


//
// Create, modify, or delete data collection item for node
//

void ClientSession::ModifyNodeDCI(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   DWORD dwObjectId;
   NetObj *pObject;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   // Get node id and check object class and access rights
   dwObjectId = pRequest->GetVariableLong(VID_OBJECT_ID);
   pObject = FindObjectById(dwObjectId);
   if (pObject != NULL)
   {
      if ((pObject->Type() == OBJECT_NODE) ||
          (pObject->Type() == OBJECT_TEMPLATE))
      {
         if (((Template *)pObject)->IsLockedBySession(m_dwIndex))
         {
            if (pObject->CheckAccessRights(m_dwUserId, OBJECT_ACCESS_MODIFY))
            {
               DWORD i, dwItemId, dwNumMaps, *pdwMapId, *pdwMapIndex;
               DCItem *pItem;
               BOOL bSuccess;

               switch(pRequest->GetCode())
               {
                  case CMD_CREATE_NEW_DCI:
                     // Create dummy DCI
                     pItem = new DCItem(CreateUniqueId(IDG_ITEM), "no name", DS_INTERNAL, 
                                        DCI_DT_INT, 60, 30, (Node *)pObject);
                     pItem->SetStatus(ITEM_STATUS_DISABLED);
                     if (((Template *)pObject)->AddItem(pItem))
                     {
                        msg.SetVariable(VID_RCC, RCC_SUCCESS);
                        // Return new item id to client
                        msg.SetVariable(VID_DCI_ID, pItem->Id());
                     }
                     else  // Unable to add item to node
                     {
                        delete pItem;
                        msg.SetVariable(VID_RCC, RCC_DUPLICATE_DCI);
                     }
                     break;
                  case CMD_MODIFY_NODE_DCI:
                     dwItemId = pRequest->GetVariableLong(VID_DCI_ID);
                     bSuccess = ((Template *)pObject)->UpdateItem(dwItemId, pRequest, &dwNumMaps,
                                                                  &pdwMapIndex, &pdwMapId);
                     if (bSuccess)
                     {
                        msg.SetVariable(VID_RCC, RCC_SUCCESS);

                        // Send index to id mapping for newly created thresholds to client
                        msg.SetVariable(VID_DCI_NUM_MAPS, dwNumMaps);
                        for(i = 0; i < dwNumMaps; i++)
                        {
                           pdwMapId[i] = htonl(pdwMapId[i]);
                           pdwMapIndex[i] = htonl(pdwMapIndex[i]);
                        }
                        msg.SetVariable(VID_DCI_MAP_IDS, (BYTE *)pdwMapId, sizeof(DWORD) * dwNumMaps);
                        msg.SetVariable(VID_DCI_MAP_INDEXES, (BYTE *)pdwMapIndex, sizeof(DWORD) * dwNumMaps);
                        safe_free(pdwMapId);
                        safe_free(pdwMapIndex);
                     }
                     else
                     {
                        msg.SetVariable(VID_RCC, RCC_INVALID_DCI_ID);
                     }
                     break;
                  case CMD_DELETE_NODE_DCI:
                     dwItemId = pRequest->GetVariableLong(VID_DCI_ID);
                     bSuccess = ((Template *)pObject)->DeleteItem(dwItemId);
                     msg.SetVariable(VID_RCC, bSuccess ? RCC_SUCCESS : RCC_INVALID_DCI_ID);
                     break;
               }
               if (bSuccess)
                  ((Template *)pObject)->SetDCIModificationFlag();
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
   SendMessage(&msg);
}


//
// Change status for one or more DCIs
//

void ClientSession::ChangeDCIStatus(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   NetObj *pObject;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   // Get node id and check object class and access rights
   pObject = FindObjectById(pRequest->GetVariableLong(VID_OBJECT_ID));
   if (pObject != NULL)
   {
      if ((pObject->Type() == OBJECT_NODE) ||
          (pObject->Type() == OBJECT_TEMPLATE))
      {
         if (((Template *)pObject)->IsLockedBySession(m_dwIndex))
         {
            if (pObject->CheckAccessRights(m_dwUserId, OBJECT_ACCESS_MODIFY))
            {
               DWORD dwNumItems, *pdwItemList;
               int iStatus;

               iStatus = pRequest->GetVariableShort(VID_DCI_STATUS);
               dwNumItems = pRequest->GetVariableLong(VID_NUM_ITEMS);
               pdwItemList = (DWORD *)malloc(sizeof(DWORD) * dwNumItems);
               pRequest->GetVariableInt32Array(VID_ITEM_LIST, dwNumItems, pdwItemList);
               if (((Template *)pObject)->SetItemStatus(dwNumItems, pdwItemList, iStatus))
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
         msg.SetVariable(VID_RCC, RCC_INVALID_OBJECT_ID);
      }
   }
   else  // No object with given ID
   {
      msg.SetVariable(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   // Send response
   SendMessage(&msg);
}


//
// Copy DCI from one node or template to another
//

void ClientSession::CopyDCI(CSCPMessage *pRequest)
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
      if (((pSource->Type() == OBJECT_NODE) || (pSource->Type() == OBJECT_TEMPLATE)) && 
          ((pDestination->Type() == OBJECT_NODE) || (pDestination->Type() == OBJECT_TEMPLATE)))
      {
         if (((Template *)pSource)->IsLockedBySession(m_dwIndex))
         {
            // Check access rights
            if ((pSource->CheckAccessRights(m_dwUserId, OBJECT_ACCESS_READ)) &&
                (pDestination->CheckAccessRights(m_dwUserId, OBJECT_ACCESS_MODIFY)))
            {
               // Attempt to lock destination's DCI list
               if ((pDestination->Id() == pSource->Id()) ||
                   (((Template *)pDestination)->LockDCIList(m_dwIndex)))
               {
                  DWORD i, *pdwItemList, dwNumItems;
                  const DCItem *pSrcItem;
                  DCItem *pDstItem;
                  int iErrors = 0;

                  // Get list of items to be copied
                  dwNumItems = pRequest->GetVariableLong(VID_NUM_ITEMS);
                  pdwItemList = (DWORD *)malloc(sizeof(DWORD) * dwNumItems);
                  pRequest->GetVariableInt32Array(VID_ITEM_LIST, dwNumItems, pdwItemList);

                  // Copy items
                  for(i = 0; i < dwNumItems; i++)
                  {
                     pSrcItem = ((Template *)pSource)->GetItemById(pdwItemList[i]);
                     if (pSrcItem != NULL)
                     {
                        pDstItem = new DCItem(pSrcItem);
                        pDstItem->ChangeBinding(CreateUniqueId(IDG_ITEM),
                                                (Template *)pDestination);
                        if (!((Template *)pDestination)->AddItem(pDstItem))
                        {
                           delete pDstItem;
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
                     ((Template *)pDestination)->UnlockDCIList(m_dwIndex);
                  msg.SetVariable(VID_RCC, (iErrors == 0) ? RCC_SUCCESS : RCC_DCI_COPY_ERRORS);

                  // Queue template update
                  if (pDestination->Type() == OBJECT_TEMPLATE)
                     ((Template *)pDestination)->QueueUpdate();
               }
               else  // Destination's DCI list already locked by someone else
               {
                  msg.SetVariable(VID_RCC, RCC_COMPONENT_LOCKED);
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
   SendMessage(&msg);
}


//
// Get collected data
//

void ClientSession::GetCollectedData(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   DWORD dwObjectId;
   NetObj *pObject;
   BOOL bSuccess = FALSE;
   static DWORD m_dwRowSize[] = { 8, 8, 12, 12, 260, 12 };

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   // Get node id and check object class and access rights
   dwObjectId = pRequest->GetVariableLong(VID_OBJECT_ID);
   pObject = FindObjectById(dwObjectId);
   if (pObject != NULL)
   {
      if (pObject->CheckAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
      {
         DB_ASYNC_RESULT hResult;
         DWORD dwItemId, dwMaxRows, dwTimeFrom, dwTimeTo;
         DWORD dwAllocatedRows = 100, dwNumRows = 0;
         char szQuery[512], szCond[256];
         int iPos = 0, iType;
         DCI_DATA_HEADER *pData = NULL;
         DCI_DATA_ROW *pCurr;

         // Get request parameters
         dwItemId = pRequest->GetVariableLong(VID_DCI_ID);
         dwMaxRows = pRequest->GetVariableLong(VID_MAX_ROWS);
         dwTimeFrom = pRequest->GetVariableLong(VID_TIME_FROM);
         dwTimeTo = pRequest->GetVariableLong(VID_TIME_TO);

         szCond[0] = 0;
         if (dwTimeFrom != 0)
         {
            sprintf(szCond, " AND idata_timestamp>=%d", dwTimeFrom);
            iPos = strlen(szCond);
         }
         if (dwTimeTo != 0)
         {
            sprintf(&szCond[iPos], " AND idata_timestamp<=%d", dwTimeTo);
         }

         // Get item's data type to determine actual row size
         iType = ((Node *)pObject)->GetItemType(dwItemId);

         // Create database-dependent query for fetching N rows
         if (dwMaxRows > 0)
         {
            switch(g_dwDBSyntax)
            {
               case DB_SYNTAX_MSSQL:
                  sprintf(szQuery, "SELECT TOP %ld idata_timestamp,idata_value FROM idata_%d WHERE item_id=%d%s ORDER BY idata_timestamp DESC",
                          dwMaxRows, dwObjectId, dwItemId, szCond);
                  break;
               case DB_SYNTAX_ORACLE:
                  sprintf(szQuery, "SELECT idata_timestamp,idata_value FROM idata_%d WHERE item_id=%d%s AND ROWNUM <= %ld ORDER BY idata_timestamp DESC",
                          dwObjectId, dwItemId, szCond, dwMaxRows);
                  break;
               case DB_SYNTAX_MYSQL:
               case DB_SYNTAX_PGSQL:
                  sprintf(szQuery, "SELECT idata_timestamp,idata_value FROM idata_%d WHERE item_id=%d%s ORDER BY idata_timestamp DESC LIMIT %ld",
                          dwObjectId, dwItemId, szCond, dwMaxRows);
                  break;
               default:
                  sprintf(szQuery, "SELECT idata_timestamp,idata_value FROM idata_%d WHERE item_id=%d%s ORDER BY idata_timestamp DESC",
                          dwObjectId, dwItemId, szCond);
                  break;
            }
         }
         else
         {
            sprintf(szQuery, "SELECT idata_timestamp,idata_value FROM idata_%d WHERE item_id=%d%s ORDER BY idata_timestamp DESC",
                    dwObjectId, dwItemId, szCond);
         }
         hResult = DBAsyncSelect(g_hCoreDB, szQuery);
         if (hResult != NULL)
         {
            // Send CMD_REQUEST_COMPLETED message
            msg.SetVariable(VID_RCC, RCC_SUCCESS);
            SendMessage(&msg);

            // Allocate initial memory block and prepare data header
            pData = (DCI_DATA_HEADER *)malloc(dwAllocatedRows * m_dwRowSize[iType] + sizeof(DCI_DATA_HEADER));
            pData->dwDataType = htonl((DWORD)iType);
            pData->dwItemId = htonl(dwItemId);

            // Fill memory block with records
            pCurr = (DCI_DATA_ROW *)(((char *)pData) + sizeof(DCI_DATA_HEADER));
            while(DBFetch(hResult))
            {
               if ((dwMaxRows > 0) && (dwNumRows >= dwMaxRows))
                  break;

               // Extend buffer if we are at the end
               if (dwNumRows == dwAllocatedRows)
               {
                  dwAllocatedRows += 50;
                  pData = (DCI_DATA_HEADER *)realloc(pData, 
                     dwAllocatedRows * m_dwRowSize[iType] + sizeof(DCI_DATA_HEADER));
                  pCurr = (DCI_DATA_ROW *)(((char *)pData) + sizeof(DCI_DATA_HEADER) + m_dwRowSize[iType] * dwNumRows);
               }

               dwNumRows++;

               pCurr->dwTimeStamp = htonl(DBGetFieldAsyncULong(hResult, 0));
               switch(iType)
               {
                  case DCI_DT_INT:
                  case DCI_DT_UINT:
                     pCurr->value.dwInteger = htonl(DBGetFieldAsyncULong(hResult, 1));
                     break;
                  case DCI_DT_INT64:
                  case DCI_DT_UINT64:
                     pCurr->value.qwInt64 = htonq(DBGetFieldAsyncUInt64(hResult, 1));
                     break;
                  case DCI_DT_FLOAT:
                     pCurr->value.dFloat = htond(DBGetFieldAsyncDouble(hResult, 1));
                     break;
                  case DCI_DT_STRING:
                     DBGetFieldAsync(hResult, 1, pCurr->value.szString, MAX_DCI_STRING_VALUE);
                     break;
               }
               pCurr = (DCI_DATA_ROW *)(((char *)pCurr) + m_dwRowSize[iType]);
            }
            DBFreeAsyncResult(hResult);
            pData->dwNumRows = htonl(dwNumRows);

            // Prepare and send raw message with fetched data
            m_pSendQueue->Put(
               CreateRawCSCPMessage(CMD_DCI_DATA, pRequest->GetId(), 
                                    dwNumRows * m_dwRowSize[iType] + sizeof(DCI_DATA_HEADER),
                                    pData, NULL));
            free(pData);
            bSuccess = TRUE;
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
   }
   else  // No object with given ID
   {
      msg.SetVariable(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   // Send response
   if (!bSuccess)
      SendMessage(&msg);
}


//
// Send latest collected values for all DCIs of given node
//

void ClientSession::SendLastValues(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   NetObj *pObject;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   // Get node id and check object class and access rights
   pObject = FindObjectById(pRequest->GetVariableLong(VID_OBJECT_ID));
   if (pObject != NULL)
   {
      if (pObject->CheckAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
      {
         if (pObject->Type() == OBJECT_NODE)
         {
            msg.SetVariable(VID_RCC, ((Node *)pObject)->GetLastValues(&msg));
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
   SendMessage(&msg);
}


//
// Open event processing policy
//

void ClientSession::OpenEPP(DWORD dwRqId)
{
   CSCPMessage msg;
   char szBuffer[256];
   BOOL bSuccess = FALSE;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(dwRqId);

   if (m_dwSystemAccess & SYSTEM_ACCESS_EPP)
   {
      if (!LockComponent(CID_EPP, m_dwIndex, m_szUserName, NULL, szBuffer))
      {
         msg.SetVariable(VID_RCC, RCC_COMPONENT_LOCKED);
         msg.SetVariable(VID_LOCKED_BY, szBuffer);
      }
      else
      {
         m_dwFlags |= CSF_EPP_LOCKED;
         msg.SetVariable(VID_RCC, RCC_SUCCESS);
         msg.SetVariable(VID_NUM_RULES, g_pEventPolicy->NumRules());
         bSuccess = TRUE;
      }
   }
   else
   {
      // Current user has no rights for event policy management
      msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
   }

   // Send response
   SendMessage(&msg);

   // Send policy to client
   if (bSuccess)
      g_pEventPolicy->SendToClient(this, dwRqId);
}


//
// Close event processing policy
//

void ClientSession::CloseEPP(DWORD dwRqId)
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
   SendMessage(&msg);
}


//
// Save event processing policy
//

void ClientSession::SaveEPP(CSCPMessage *pRequest)
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
            g_pEventPolicy->ReplacePolicy(0, NULL);
            g_pEventPolicy->SaveToDB();
         }
         else
         {
            m_dwFlags |= CSF_EPP_UPLOAD;
            m_ppEPPRuleList = (EPRule **)malloc(sizeof(EPRule *) * m_dwNumRecordsToUpload);
            memset(m_ppEPPRuleList, 0, sizeof(EPRule *) * m_dwNumRecordsToUpload);
         }
         DebugPrintf("Accepted EPP upload request for %d rules\n", m_dwNumRecordsToUpload);
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
   SendMessage(&msg);
}


//
// Process EPP rule received from client
//

void ClientSession::ProcessEPPRecord(CSCPMessage *pRequest)
{
   if (!(m_dwFlags & CSF_EPP_LOCKED))
   {
      CSCPMessage msg;

      msg.SetCode(CMD_REQUEST_COMPLETED);
      msg.SetId(pRequest->GetId());
      msg.SetVariable(VID_RCC, RCC_OUT_OF_STATE_REQUEST);
      SendMessage(&msg);
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
            DebugPrintf("Replacing event processing policy with a new one at %p (%d rules)\n",
                        m_ppEPPRuleList, m_dwNumRecordsToUpload);
            g_pEventPolicy->ReplacePolicy(m_dwNumRecordsToUpload, m_ppEPPRuleList);
            g_pEventPolicy->SaveToDB();
            m_ppEPPRuleList = NULL;
            
            // ... and send final confirmation
            msg.SetCode(CMD_REQUEST_COMPLETED);
            msg.SetId(pRequest->GetId());
            msg.SetVariable(VID_RCC, RCC_SUCCESS);
            SendMessage(&msg);

            m_dwFlags &= ~CSF_EPP_UPLOAD;
         }
      }
   }
}


//
// Send list of available MIB files to client
//

void ClientSession::SendMIBList(DWORD dwRqId)
{
   CSCPMessage msg;
   DWORD dwId1, dwId2, dwNumFiles;
   DIR *dir;
   int iBufPos;
   struct dirent *dptr;
   char szBuffer[MAX_PATH];
   BYTE md5Hash[MD5_DIGEST_SIZE];

   // Prepare response message
   msg.SetCode(CMD_MIB_LIST);
   msg.SetId(dwRqId);

   // Read directory
   dwNumFiles = 0;
   strcpy(szBuffer, g_szDataDir);
   strcat(szBuffer, DDIR_MIBS);
   dir = opendir(szBuffer);
   if (dir != NULL)
   {
      strcat(szBuffer, FS_PATH_SEPARATOR);
      iBufPos = strlen(szBuffer);
      dwId1 = VID_MIB_NAME_BASE;
      dwId2 = VID_MIB_HASH_BASE;
      while((dptr = readdir(dir)) != NULL)
      {
         if (dptr->d_name[0] == '.')
            continue;

         strcpy(&szBuffer[iBufPos], dptr->d_name);
         if (CalculateFileMD5Hash(szBuffer, md5Hash))
         {
            msg.SetVariable(dwId1++, dptr->d_name);
            msg.SetVariable(dwId2++, md5Hash, MD5_DIGEST_SIZE);
            dwNumFiles++;
         }
      }
      closedir(dir);
   }

   msg.SetVariable(VID_NUM_MIBS, dwNumFiles);

   // Send response
   SendMessage(&msg);
}


//
// Send requested MIB file to client
//

void ClientSession::SendMIB(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   BYTE *pFile;
   DWORD dwFileSize;
   char szBuffer[MAX_PATH], szMIB[MAX_PATH];

   // Prepare response message
   msg.SetCode(CMD_MIB);
   msg.SetId(pRequest->GetId());

   // Get name of the requested file
   pRequest->GetVariableStr(VID_MIB_NAME, szMIB, MAX_PATH);

   // Load file into memory
   strcpy(szBuffer, g_szDataDir);
   strcat(szBuffer, DDIR_MIBS);
#ifdef _WIN32
   strcat(szBuffer, "\\");
#else
   strcat(szBuffer, "/");
#endif
   strcat(szBuffer, szMIB);
   pFile = LoadFile(szBuffer, &dwFileSize);
   if (pFile != NULL)
   {
      msg.SetVariable(VID_RCC, RCC_SUCCESS);
      msg.SetVariable(VID_MIB_FILE_SIZE, dwFileSize);
      msg.SetVariable(VID_MIB_FILE, pFile, dwFileSize);
      free(pFile);
   }
   else
   {
      msg.SetVariable(VID_RCC, RCC_SYSTEM_FAILURE);
   }

   // Send response
   SendMessage(&msg);
}


//
// Create new object
//

void ClientSession::CreateObject(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   NetObj *pObject, *pParent;
   int iClass, iServiceType;
   TCHAR *pDescription, szObjectName[MAX_OBJECT_NAME];
   TCHAR *pszRequest, *pszResponse;
   DWORD dwIpAddr;
   WORD wIpProto, wIpPort;
   BOOL bParentAlwaysValid = FALSE;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   iClass = pRequest->GetVariableShort(VID_OBJECT_CLASS);

   // Find parent object
   pParent = FindObjectById(pRequest->GetVariableLong(VID_PARENT_ID));
   if (iClass == OBJECT_NODE)
   {
      dwIpAddr = pRequest->GetVariableLong(VID_IP_ADDRESS);
      if ((pParent == NULL) && (dwIpAddr != 0))
      {
         pParent = FindSubnetForNode(dwIpAddr);
         bParentAlwaysValid = TRUE;
      }
   }
   if ((pParent != NULL) || (iClass == OBJECT_NODE))
   {
      // User should have create access to parent object
      if ((pParent != NULL) ? 
            pParent->CheckAccessRights(m_dwUserId, OBJECT_ACCESS_CREATE) :
            g_pEntireNet->CheckAccessRights(m_dwUserId, OBJECT_ACCESS_CREATE))
      {
         // Parent object should be of valid type
         if (bParentAlwaysValid || IsValidParentClass(iClass, (pParent != NULL) ? pParent->Type() : -1))
         {
            pRequest->GetVariableStr(VID_OBJECT_NAME, szObjectName, MAX_OBJECT_NAME);
            if (IsValidObjectName(szObjectName))
            {
               // Create new object
               switch(iClass)
               {
                  case OBJECT_NODE:
                     pObject = PollNewNode(dwIpAddr, pRequest->GetVariableLong(VID_IP_NETMASK),
                                           DF_DEFAULT, szObjectName);
                     break;
                  case OBJECT_CONTAINER:
                     pDescription = pRequest->GetVariableStr(VID_DESCRIPTION);
                     pObject = new Container(szObjectName, 
                                             pRequest->GetVariableLong(VID_CATEGORY),
                                             pDescription);
                     safe_free(pDescription);
                     NetObjInsert(pObject, TRUE);
                     break;
                  case OBJECT_TEMPLATEGROUP:
                     pDescription = pRequest->GetVariableStr(VID_DESCRIPTION);
                     pObject = new TemplateGroup(szObjectName, pDescription);
                     safe_free(pDescription);
                     NetObjInsert(pObject, TRUE);
                     break;
                  case OBJECT_TEMPLATE:
                     pObject = new Template;
                     pObject->SetName(szObjectName);
                     NetObjInsert(pObject, TRUE);
                     break;
                  case OBJECT_NETWORKSERVICE:
                     iServiceType = (int)pRequest->GetVariableShort(VID_SERVICE_TYPE);
                     wIpProto = pRequest->GetVariableShort(VID_IP_PROTO);
                     wIpPort = pRequest->GetVariableShort(VID_IP_PORT);
                     pszRequest = pRequest->GetVariableStr(VID_SERVICE_REQUEST);
                     pszResponse = pRequest->GetVariableStr(VID_SERVICE_RESPONSE);
                     pObject = new NetworkService(iServiceType, wIpProto, wIpPort, 
                                                  pszRequest, pszResponse, (Node *)pParent);
                     pObject->SetName(szObjectName);
                     NetObjInsert(pObject, TRUE);
                     break;
                  case OBJECT_VPNCONNECTOR:
                     pObject = new VPNConnector(TRUE);
                     pObject->SetName(szObjectName);
                     NetObjInsert(pObject, TRUE);
                     break;
                  default:
                     break;
               }

               // If creation was successful do binding
               if (pObject != NULL)
               {
                  if (pParent != NULL)    // parent can be NULL for nodes
                  {
                     pParent->AddChild(pObject);
                     pObject->AddParent(pParent);
                     pParent->CalculateCompoundStatus();
                  }
                  pObject->Unhide();
                  msg.SetVariable(VID_RCC, RCC_SUCCESS);
                  msg.SetVariable(VID_OBJECT_ID, pObject->Id());
               }
               else
               {
                  msg.SetVariable(VID_RCC, RCC_OBJECT_CREATION_FAILED);
               }
            }
            else
            {
               msg.SetVariable(VID_RCC, RCC_INVALID_OBJECT_NAME);
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
   SendMessage(&msg);
}


//
// Bind/unbind object
//

void ClientSession::ChangeObjectBinding(CSCPMessage *pRequest, BOOL bBind)
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
      if ((pParent->CheckAccessRights(m_dwUserId, OBJECT_ACCESS_MODIFY)) &&
          (pChild->CheckAccessRights(m_dwUserId, OBJECT_ACCESS_MODIFY)))
      {
         // Parent object should be container or service root
         // For unbind, it can also be template
         if ((pParent->Type() == OBJECT_CONTAINER) ||
             (pParent->Type() == OBJECT_SERVICEROOT) ||
             ((pParent->Type() == OBJECT_TEMPLATE) && (!bBind)))
         {
            if (bBind)
            {
               // Prevent loops
               if (!pChild->IsChild(pParent->Id()))
               {
                  pParent->AddChild(pChild);
                  pChild->AddParent(pParent);
                  pParent->CalculateCompoundStatus();
                  msg.SetVariable(VID_RCC, RCC_SUCCESS);
               }
               else
               {
                  msg.SetVariable(VID_RCC, RCC_OBJECT_LOOP);
               }
            }
            else
            {
               pParent->DeleteChild(pChild);
               pChild->DeleteParent(pParent);
               if ((pParent->Type() == OBJECT_TEMPLATE) &&
                   (pChild->Type() == OBJECT_NODE))
               {
                  ((Template *)pParent)->QueueRemoveFromNode(pChild->Id(), 
                                                pRequest->GetVariableShort(VID_REMOVE_DCI));
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
   SendMessage(&msg);
}


//
// Delete object
//

void ClientSession::DeleteObject(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   NetObj *pObject;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   // Find object to be deleted
   pObject = FindObjectById(pRequest->GetVariableLong(VID_OBJECT_ID));
   if (pObject != NULL)
   {
      // Check if it is a built-in object, like "Entire Network"
      if (pObject->Id() >= 10)
      {
         // Check access rights
         if (pObject->CheckAccessRights(m_dwUserId, OBJECT_ACCESS_DELETE))
         {
            pObject->Delete(FALSE);
            msg.SetVariable(VID_RCC, RCC_SUCCESS);
         }
         else
         {
            msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
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
   SendMessage(&msg);
}


//
// Process changes in alarms
//

void ClientSession::OnAlarmUpdate(DWORD dwCode, NXC_ALARM *pAlarm)
{
   UPDATE_INFO *pUpdate;
   NetObj *pObject;

   if (IsAuthenticated() && (m_dwActiveChannels & NXC_CHANNEL_ALARMS))
   {
      pObject = FindObjectById(pAlarm->dwSourceObject);
      if (pObject != NULL)
         if (pObject->CheckAccessRights(m_dwUserId, OBJECT_ACCESS_READ_ALARMS))
         {
            pUpdate = (UPDATE_INFO *)malloc(sizeof(UPDATE_INFO));
            pUpdate->dwCategory = INFO_CAT_ALARM;
            pUpdate->dwCode = dwCode;
            pUpdate->pData = nx_memdup(pAlarm, sizeof(NXC_ALARM));
            m_pUpdateQueue->Put(pUpdate);
         }
   }
}


//
// Send all alarms to client
//

void ClientSession::SendAllAlarms(DWORD dwRqId, BOOL bIncludeAck)
{
   MutexLock(m_mutexSendAlarms, INFINITE);
   g_alarmMgr.SendAlarmsToClient(dwRqId, bIncludeAck, this);
   MutexUnlock(m_mutexSendAlarms);
}


//
// Acknowlege alarm
//

void ClientSession::AcknowlegeAlarm(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   NetObj *pObject;
   DWORD dwAlarmId;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   // Get alarm id and it's source object
   dwAlarmId = pRequest->GetVariableLong(VID_ALARM_ID);
   pObject = g_alarmMgr.GetAlarmSourceObject(dwAlarmId);
   if (pObject != NULL)
   {
      // User should have "acknowlege alarm" right to the object
      if (pObject->CheckAccessRights(m_dwUserId, OBJECT_ACCESS_ACK_ALARMS))
      {
         g_alarmMgr.AckById(dwAlarmId, m_dwUserId);
         msg.SetVariable(VID_RCC, RCC_SUCCESS);
      }
      else
      {
         msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
      }
   }
   else
   {
      // Normally, for existing alarms pObject will not be NULL,
      // so we assume that alarm id is invalid
      msg.SetVariable(VID_RCC, RCC_INVALID_ALARM_ID);
   }

   // Send response
   SendMessage(&msg);
}


//
// Delete alarm
//

void ClientSession::DeleteAlarm(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   NetObj *pObject;
   DWORD dwAlarmId;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   // Get alarm id and it's source object
   dwAlarmId = pRequest->GetVariableLong(VID_ALARM_ID);
   pObject = g_alarmMgr.GetAlarmSourceObject(dwAlarmId);
   if (pObject != NULL)
   {
      // User should have "acknowlege alarm" right to the object
      // and system right "delete alarms"
      if ((pObject->CheckAccessRights(m_dwUserId, OBJECT_ACCESS_ACK_ALARMS)) &&
          (m_dwSystemAccess & SYSTEM_ACCESS_DELETE_ALARMS))
      {
         g_alarmMgr.DeleteAlarm(dwAlarmId);
         msg.SetVariable(VID_RCC, RCC_SUCCESS);
      }
      else
      {
         msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
      }
   }
   else
   {
      // Normally, for existing alarms pObject will not be NULL,
      // so we assume that alarm id is invalid
      msg.SetVariable(VID_RCC, RCC_INVALID_ALARM_ID);
   }

   // Send response
   SendMessage(&msg);
}


//
// Lock/unlock action configuration database
//

void ClientSession::LockActionDB(DWORD dwRqId, BOOL bLock)
{
   CSCPMessage msg;
   char szBuffer[256];

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(dwRqId);

   if (m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_ACTIONS)
   {
      if (bLock)
      {
         if (!LockComponent(CID_ACTION_DB, m_dwIndex, m_szUserName, NULL, szBuffer))
         {
            msg.SetVariable(VID_RCC, RCC_COMPONENT_LOCKED);
            msg.SetVariable(VID_LOCKED_BY, szBuffer);
         }
         else
         {
            m_dwFlags |= CSF_ACTION_DB_LOCKED;
            msg.SetVariable(VID_RCC, RCC_SUCCESS);
         }
      }
      else
      {
         if (m_dwFlags & CSF_ACTION_DB_LOCKED)
         {
            UnlockComponent(CID_ACTION_DB);
            m_dwFlags &= ~CSF_ACTION_DB_LOCKED;
         }
         msg.SetVariable(VID_RCC, RCC_SUCCESS);
      }
   }
   else
   {
      // Current user has no rights for action management
      msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
   }

   // Send response
   SendMessage(&msg);
}


//
// Create new action
//

void ClientSession::CreateAction(CSCPMessage *pRequest)
{
   CSCPMessage msg;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   // Check user rights
   if (!(m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_ACTIONS))
   {
      msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
   }
   else if (!(m_dwFlags & CSF_ACTION_DB_LOCKED))
   {
      // Action database have to be locked before any
      // changes can be made
      msg.SetVariable(VID_RCC, RCC_OUT_OF_STATE_REQUEST);
   }
   else
   {
      DWORD dwResult, dwActionId;
      char szActionName[MAX_USER_NAME];

      pRequest->GetVariableStr(VID_ACTION_NAME, szActionName, MAX_OBJECT_NAME);
      if (IsValidObjectName(szActionName))
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

   // Send response
   SendMessage(&msg);
}


//
// Update existing action's data
//

void ClientSession::UpdateAction(CSCPMessage *pRequest)
{
   CSCPMessage msg;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   // Check user rights
   if (!(m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_ACTIONS))
   {
      msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
   }
   else if (!(m_dwFlags & CSF_ACTION_DB_LOCKED))
   {
      // Action database have to be locked before any
      // changes can be made
      msg.SetVariable(VID_RCC, RCC_OUT_OF_STATE_REQUEST);
   }
   else
   {
      msg.SetVariable(VID_RCC, ModifyActionFromMessage(pRequest));
   }

   // Send response
   SendMessage(&msg);
}


//
// Delete action
//

void ClientSession::DeleteAction(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   DWORD dwActionId;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   // Check user rights
   if (!(m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_ACTIONS))
   {
      msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
   }
   else if (!(m_dwFlags & CSF_ACTION_DB_LOCKED))
   {
      // Action database have to be locked before any
      // changes can be made
      msg.SetVariable(VID_RCC, RCC_OUT_OF_STATE_REQUEST);
   }
   else
   {
      // Get Id of action to be deleted
      dwActionId = pRequest->GetVariableLong(VID_ACTION_ID);
      if (!g_pEventPolicy->ActionInUse(dwActionId))
      {
         msg.SetVariable(VID_RCC, DeleteActionFromDB(dwActionId));
      }
      else
      {
         msg.SetVariable(VID_RCC, RCC_ACTION_IN_USE);
      }
   }

   // Send response
   SendMessage(&msg);
}


//
// Process changes in actions
//

void ClientSession::OnActionDBUpdate(DWORD dwCode, NXC_ACTION *pAction)
{
   UPDATE_INFO *pUpdate;

   if (IsAuthenticated())
   {
      if (m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_ACTIONS)
      {
         pUpdate = (UPDATE_INFO *)malloc(sizeof(UPDATE_INFO));
         pUpdate->dwCategory = INFO_CAT_ACTION;
         pUpdate->dwCode = dwCode;
         pUpdate->pData = nx_memdup(pAction, sizeof(NXC_ACTION));
         m_pUpdateQueue->Put(pUpdate);
      }
   }
}


//
// Send all actions to client
//

void ClientSession::SendAllActions(DWORD dwRqId)
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
      SendMessage(&msg);
      MutexLock(m_mutexSendActions, INFINITE);
      SendActionsToClient(this, dwRqId);
      MutexUnlock(m_mutexSendActions);
   }
   else
   {
      msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
      SendMessage(&msg);
   }
}


//
// Send list of configured container categories to client
//

void ClientSession::SendContainerCategories(DWORD dwRqId)
{
   CSCPMessage msg;
   DWORD i;

   // Prepare response message
   msg.SetCode(CMD_CONTAINER_CAT_DATA);
   msg.SetId(dwRqId);

   for(i = 0; i < g_dwNumCategories; i++)
   {
      msg.SetVariable(VID_CATEGORY_ID, g_pContainerCatList[i].dwCatId);
      msg.SetVariable(VID_CATEGORY_NAME, g_pContainerCatList[i].szName);
      msg.SetVariable(VID_IMAGE_ID, g_pContainerCatList[i].dwImageId);
      msg.SetVariable(VID_DESCRIPTION, g_pContainerCatList[i].pszDescription);
      SendMessage(&msg);
      msg.DeleteAllVariables();
   }

   // Send end-of-list indicator
   msg.SetVariable(VID_CATEGORY_ID, (DWORD)0);
   SendMessage(&msg);
}


//
// Perform a forced node poll
//

void ClientSession::ForcedNodePoll(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   POLLER_START_DATA *pData;
   NetObj *pObject;

   pData = (POLLER_START_DATA *)malloc(sizeof(POLLER_START_DATA));
   pData->pSession = this;
   MutexLock(m_mutexPollerInit, INFINITE);

   // Prepare response message
   pData->dwRqId = pRequest->GetId();
   msg.SetCode(CMD_POLLING_INFO);
   msg.SetId(pData->dwRqId);

   // Get polling type
   pData->iPollType = pRequest->GetVariableShort(VID_POLL_TYPE);

   // Find object to be deleted
   pObject = FindObjectById(pRequest->GetVariableLong(VID_OBJECT_ID));
   if (pObject != NULL)
   {
      // We can do polls only for node objects
      if ((pObject->Type() == OBJECT_NODE) &&
          ((pData->iPollType == POLL_STATUS) || (pData->iPollType == POLL_CONFIGURATION)))
      {
         // Check access rights
         if (pObject->CheckAccessRights(m_dwUserId, OBJECT_ACCESS_MODIFY))
         {
            ((Node *)pObject)->IncRefCount();
            m_dwRefCount++;

            pData->pNode = (Node *)pObject;
            ThreadCreate(PollerThreadStarter, 0, pData);
            msg.SetVariable(VID_RCC, RCC_OPERATION_IN_PROGRESS);
            msg.SetVariable(VID_POLLER_MESSAGE, _T("Poll request accepted\r\n"));
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
   SendMessage(&msg);
   MutexUnlock(m_mutexPollerInit);
}


//
// Send message fro poller to client
//

void ClientSession::SendPollerMsg(DWORD dwRqId, TCHAR *pszMsg)
{
   CSCPMessage msg;

   msg.SetCode(CMD_POLLING_INFO);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_RCC, RCC_OPERATION_IN_PROGRESS);
   msg.SetVariable(VID_POLLER_MESSAGE, pszMsg);
   SendMessage(&msg);
}


//
// Node poller thread
//

void ClientSession::PollerThread(Node *pNode, int iPollType, DWORD dwRqId)
{
   CSCPMessage msg;

   // Wait while parent thread finishes initialization
   MutexLock(m_mutexPollerInit, INFINITE);
   MutexUnlock(m_mutexPollerInit);

   switch(iPollType)
   {
      case POLL_STATUS:
         pNode->StatusPoll(this, dwRqId, -1);
         break;
      case POLL_CONFIGURATION:
         pNode->ConfigurationPoll(this, dwRqId, -1);
         break;
      default:
         SendPollerMsg(dwRqId, _T("Invalid poll type requested\r\n"));
         break;
   }
   pNode->DecRefCount();

   msg.SetCode(CMD_POLLING_INFO);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_RCC, RCC_SUCCESS);
   SendMessage(&msg);
}


//
// Receive event from user
//

void ClientSession::OnTrap(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   DWORD dwObjectId, dwEventCode;
   int i, iNumArgs;
   NetObj *pObject;
   TCHAR *pszArgList[32];
   TCHAR szFormat[] = "ssssssssssssssssssssssssssssssss";
   BOOL bSuccess;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   // Find event's source object
   dwObjectId = pRequest->GetVariableLong(VID_OBJECT_ID);
   if (dwObjectId != 0)
      pObject = FindObjectById(dwObjectId);  // Object is specified explicitely
   else
      pObject = FindNodeByIP(m_dwHostAddr);  // Client is the source
   if (pObject != NULL)
   {
      // User should have SEND_EVENTS access right to object
      if (pObject->CheckAccessRights(m_dwUserId, OBJECT_ACCESS_SEND_EVENTS))
      {
         dwEventCode = pRequest->GetVariableLong(VID_EVENT_CODE);
         iNumArgs = pRequest->GetVariableShort(VID_NUM_ARGS);
         if (iNumArgs > 32)
            iNumArgs = 32;
         for(i = 0; i < iNumArgs; i++)
            pszArgList[i] = pRequest->GetVariableStr(VID_EVENT_ARG_BASE + i);

         // Following call is not very good, but I'm too lazy now
         // to change PostEvent()
         szFormat[iNumArgs] = 0;
         bSuccess = PostEvent(dwEventCode, pObject->Id(), (iNumArgs > 0) ? szFormat : NULL,
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
            free(pszArgList[i]);

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
   SendMessage(&msg);
}


//
// Wake up node
//

void ClientSession::OnWakeUpNode(CSCPMessage *pRequest)
{
   NetObj *pObject;
   CSCPMessage msg;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   // Find node or interface object
   pObject = FindObjectById(pRequest->GetVariableLong(VID_OBJECT_ID));
   if (pObject != NULL)
   {
      if ((pObject->Type() == OBJECT_NODE) ||
          (pObject->Type() == OBJECT_INTERFACE))
      {
         if (pObject->CheckAccessRights(m_dwUserId, OBJECT_ACCESS_CONTROL))
         {
            DWORD dwResult;

            if (pObject->Type() == OBJECT_NODE)
               dwResult = ((Node *)pObject)->WakeUp();
            else
               dwResult = ((Interface *)pObject)->WakeUp();
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
   SendMessage(&msg);
}


//
// Query specific parameter from node
//

void ClientSession::QueryParameter(CSCPMessage *pRequest)
{
   NetObj *pObject;
   CSCPMessage msg;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   // Find node object
   pObject = FindObjectById(pRequest->GetVariableLong(VID_OBJECT_ID));
   if (pObject != NULL)
   {
      if (pObject->Type() == OBJECT_NODE)
      {
         if (pObject->CheckAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
         {
            DWORD dwResult;
            TCHAR szBuffer[256], szName[MAX_PARAM_NAME];

            pRequest->GetVariableStr(VID_NAME, szName, MAX_PARAM_NAME);
            dwResult = ((Node *)pObject)->GetItemForClient(pRequest->GetVariableShort(VID_DCI_SOURCE_TYPE),
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
   SendMessage(&msg);
}


//
// Edit trap configuration record
//

void ClientSession::EditTrap(int iOperation, CSCPMessage *pRequest)
{
   CSCPMessage msg;
   DWORD dwTrapId, dwResult;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   // Check access rights
   if (m_dwSystemAccess & SYSTEM_ACCESS_CONFIGURE_TRAPS)
   {
      if (m_dwFlags & CSF_TRAP_CFG_LOCKED)
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
               msg.SetVariable(VID_RCC, RCC_SYSTEM_FAILURE);
               break;
         }
      }
      else
      {
         msg.SetVariable(VID_RCC, RCC_OUT_OF_STATE_REQUEST);
      }
   }
   else
   {
      // Current user has no rights for trap management
      msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
   }

   // Send response
   SendMessage(&msg);
}


//
// Lock/unlock trap configuration
//

void ClientSession::LockTrapCfg(DWORD dwRqId, BOOL bLock)
{
   CSCPMessage msg;
   char szBuffer[256];

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(dwRqId);

   if (m_dwSystemAccess & SYSTEM_ACCESS_CONFIGURE_TRAPS)
   {
      if (bLock)
      {
         if (!LockComponent(CID_TRAP_CFG, m_dwIndex, m_szUserName, NULL, szBuffer))
         {
            msg.SetVariable(VID_RCC, RCC_COMPONENT_LOCKED);
            msg.SetVariable(VID_LOCKED_BY, szBuffer);
         }
         else
         {
            m_dwFlags |= CSF_TRAP_CFG_LOCKED;
            msg.SetVariable(VID_RCC, RCC_SUCCESS);
         }
      }
      else
      {
         if (m_dwFlags & CSF_TRAP_CFG_LOCKED)
         {
            UnlockComponent(CID_TRAP_CFG);
            m_dwFlags &= ~CSF_TRAP_CFG_LOCKED;
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
   SendMessage(&msg);
}


//
// Send all trap configuration records to client
//

void ClientSession::SendAllTraps(DWORD dwRqId)
{
   CSCPMessage msg;
   BOOL bSuccess = FALSE;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(dwRqId);

   if (m_dwSystemAccess & SYSTEM_ACCESS_CONFIGURE_TRAPS)
   {
      if (m_dwFlags & CSF_TRAP_CFG_LOCKED)
      {
         msg.SetVariable(VID_RCC, RCC_SUCCESS);
         SendMessage(&msg);
         bSuccess = TRUE;
         SendTrapsToClient(this, dwRqId);
      }
      else
      {
         msg.SetVariable(VID_RCC, RCC_OUT_OF_STATE_REQUEST);
      }
   }
   else
   {
      // Current user has no rights for trap management
      msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
   }

   // Send response
   if (!bSuccess)
      SendMessage(&msg);
}


//
// Lock/unlock package database
//

void ClientSession::LockPackageDB(DWORD dwRqId, BOOL bLock)
{
   CSCPMessage msg;
   char szBuffer[256];

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
   SendMessage(&msg);
}


//
// Send list of all installed packages to client
//

void ClientSession::SendAllPackages(DWORD dwRqId)
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
         hResult = DBAsyncSelect(g_hCoreDB, "SELECT pkg_id,version,platform,pkg_file,pkg_name,description FROM agent_pkg");
         if (hResult != NULL)
         {
            msg.SetVariable(VID_RCC, RCC_SUCCESS);
            SendMessage(&msg);
            bSuccess = TRUE;

            msg.SetCode(CMD_PACKAGE_INFO);
            msg.DeleteAllVariables();

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
               SendMessage(&msg);
               msg.DeleteAllVariables();
            }

            msg.SetVariable(VID_PACKAGE_ID, (DWORD)0);
            SendMessage(&msg);

            DBFreeAsyncResult(hResult);
         }
         else
         {
            msg.SetVariable(VID_RCC, RCC_DB_FAILURE);
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
   if (!bSuccess)
      SendMessage(&msg);
}


//
// Install package to server
//

void ClientSession::InstallPackage(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   TCHAR szPkgName[MAX_PACKAGE_NAME_LEN], szDescription[MAX_DB_STRING];
   TCHAR szPkgVersion[MAX_AGENT_VERSION_LEN], szFileName[MAX_DB_STRING];
   TCHAR szPlatform[MAX_PLATFORM_NAME_LEN], *pszCleanFileName, *pszEscDescr;
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
         pszCleanFileName = GetCleanFileName(szFileName);

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
                        _sntprintf(szQuery, 2048, _T("INSERT INTO agent_pkg (pkg_id,pkg_name,"
                                                     "version,description,platform,pkg_file) "
                                                     "VALUES (%ld,'%s','%s','%s','%s','%s')"),
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
   SendMessage(&msg);
}


//
// Remove package from server
//

void ClientSession::RemovePackage(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   DWORD dwPkgId;

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
   SendMessage(&msg);
}


//
// Send list of parameters supported by given node to client
//

void ClientSession::SendParametersList(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   NetObj *pObject;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   pObject = FindObjectById(pRequest->GetVariableLong(VID_OBJECT_ID));
   if (pObject != NULL)
   {
      switch(pObject->Type())
      {
         case OBJECT_NODE:
            msg.SetVariable(VID_RCC, RCC_SUCCESS);
            ((Node *)pObject)->WriteParamListToMessage(&msg);
            break;
         case OBJECT_TEMPLATE:
            msg.SetVariable(VID_RCC, RCC_SUCCESS);
            WriteFullParamListToMessage(&msg);
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
   SendMessage(&msg);
}


//
// Deplay package to node(s)
//

void ClientSession::DeployPackage(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   DWORD i, j, dwNumObjects, *pdwObjectList, dwNumNodes, dwPkgId;
   Node **ppNodeList;
   NetObj *pObject;
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
      dwNumNodes = 0;
      ppNodeList = NULL;

      // Get package ID
      dwPkgId = pRequest->GetVariableLong(VID_PACKAGE_ID);
      if (IsValidPackageId(dwPkgId))
      {
         // Read package information
         _sntprintf(szQuery, 256, _T("SELECT platform,pkg_file,version FROM agent_pkg WHERE pkg_id=%ld"), dwPkgId);
         hResult = DBSelect(g_hCoreDB, szQuery);
         if ((hResult != NULL) && (DBGetNumRows(hResult) > 0))
         {
            _tcsncpy(szPlatform, DBGetField(hResult, 0, 0), MAX_PLATFORM_NAME_LEN);
            _tcsncpy(szPkgFile, DBGetField(hResult, 0, 1), MAX_PATH);
            _tcsncpy(szVersion, DBGetField(hResult, 0, 2), MAX_AGENT_VERSION_LEN);

            // Create list of nodes to be upgraded
            dwNumObjects = pRequest->GetVariableLong(VID_NUM_OBJECTS);
            pdwObjectList = (DWORD *)malloc(sizeof(DWORD) * dwNumObjects);
            pRequest->GetVariableInt32Array(VID_OBJECT_LIST, dwNumObjects, pdwObjectList);
            for(i = 0; i < dwNumObjects; i++)
            {
               pObject = FindObjectById(pdwObjectList[i]);
               if (pObject != NULL)
               {
                  if (pObject->CheckAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
                  {
                     if (pObject->Type() == OBJECT_NODE)
                     {
                        // Check if this node already in the list
                        for(j = 0; j < dwNumNodes; j++)
                           if (ppNodeList[j]->Id() == pdwObjectList[i])
                              break;
                        if (j == dwNumNodes)
                        {
                           pObject->IncRefCount();
                           ppNodeList = (Node **)realloc(ppNodeList, sizeof(Node *) * (dwNumNodes + 1));
                           ppNodeList[dwNumNodes] = (Node *)pObject;
                           dwNumNodes++;
                        }
                     }
                     else
                     {
                        pObject->AddChildNodesToList(&dwNumNodes, &ppNodeList, m_dwUserId);
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
         MutexLock(hMutex, INFINITE);

         pInfo = (DT_STARTUP_INFO *)malloc(sizeof(DT_STARTUP_INFO));
         pInfo->dwNumNodes = dwNumNodes;
         pInfo->ppNodeList = ppNodeList;
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
         for(i = 0; i < dwNumNodes; i++)
            ppNodeList[i]->DecRefCount();
         safe_free(ppNodeList);
      }
   }
   else
   {
      msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
      bSuccess = FALSE;
   }

   // Send response
   SendMessage(&msg);

   // Allow deployment thread to run
   if (bSuccess)
      MutexUnlock(hMutex);
}


//
// Apply data collection template to node
//

void ClientSession::ApplyTemplate(CSCPMessage *pRequest)
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
          (pDestination->Type() == OBJECT_NODE))
      {
         BOOL bLockSucceed = FALSE;

         // Acquire DCI lock if needed
         if (!((Template *)pSource)->IsLockedBySession(m_dwIndex))
         {
            bLockSucceed = ((Template *)pSource)->LockDCIList(m_dwIndex);
         }
         else
         {
            bLockSucceed = TRUE;
         }

         if (bLockSucceed)
         {
            // Check access rights
            if ((pSource->CheckAccessRights(m_dwUserId, OBJECT_ACCESS_READ)) &&
                (pDestination->CheckAccessRights(m_dwUserId, OBJECT_ACCESS_MODIFY)))
            {
               // Attempt to lock destination's DCI list
               if (((Node *)pDestination)->LockDCIList(m_dwIndex))
               {
                  BOOL bErrors;

                  bErrors = ((Template *)pSource)->ApplyToNode((Node *)pDestination);
                  ((Template *)pDestination)->UnlockDCIList(m_dwIndex);
                  msg.SetVariable(VID_RCC, bErrors ? RCC_DCI_COPY_ERRORS : RCC_SUCCESS);
               }
               else  // Destination's DCI list already locked by someone else
               {
                  msg.SetVariable(VID_RCC, RCC_COMPONENT_LOCKED);
               }
            }
            else  // User doesn't have enough rights on object(s)
            {
               msg.SetVariable(VID_RCC, RCC_ACCESS_DENIED);
            }

            ((Template *)pSource)->UnlockDCIList(m_dwIndex);
         }
         else  // Source node DCI list not locked by this session
         {
            msg.SetVariable(VID_RCC, RCC_COMPONENT_LOCKED);
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
   SendMessage(&msg);
}


//
// Get user variable
//

void ClientSession::GetUserVariable(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   TCHAR szVarName[MAX_VARIABLE_NAME], szQuery[MAX_VARIABLE_NAME + 256];
   DB_RESULT hResult;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   // Try to read variable from database
   pRequest->GetVariableStr(VID_NAME, szVarName, MAX_VARIABLE_NAME);
   _sntprintf(szQuery, MAX_VARIABLE_NAME + 256,
              _T("SELECT var_value FROM user_profiles WHERE user_id=%ld AND var_name='%s'"),
              m_dwUserId, szVarName);
   hResult = DBSelect(g_hCoreDB, szQuery);
   if (hResult != NULL)
   {
      if (DBGetNumRows(hResult) > 0)
      {
         TCHAR *pszData;

         pszData = _tcsdup(DBGetField(hResult, 0, 0));
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

   // Send response
   SendMessage(&msg);
}


//
// Set user variable
//

void ClientSession::SetUserVariable(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   TCHAR szVarName[MAX_VARIABLE_NAME], szQuery[8192], *pszValue, *pszRawValue;
   DB_RESULT hResult;
   BOOL bExist = FALSE;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   // Check variable name
   pRequest->GetVariableStr(VID_NAME, szVarName, MAX_VARIABLE_NAME);
   if (IsValidObjectName(szVarName))
   {
      // Check if variable already exist in database
      _sntprintf(szQuery, MAX_VARIABLE_NAME + 256,
                 _T("SELECT var_value FROM user_profiles WHERE user_id=%ld AND var_name='%s'"),
                 m_dwUserId, szVarName);
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
                    _T("UPDATE user_profiles SET var_value='%s' WHERE user_id=%ld AND var_name='%s'"),
                    pszValue, m_dwUserId, szVarName);
      else
         _sntprintf(szQuery, 8192,
                    _T("INSERT INTO user_profiles (user_id,var_name,var_value) VALUES (%ld,'%s','%s')"),
                    m_dwUserId, szVarName, pszValue);
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

   // Send response
   SendMessage(&msg);
}


//
// Enum user variables
//

void ClientSession::EnumUserVariables(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   TCHAR *pszName, szPattern[MAX_VARIABLE_NAME], szQuery[256];
   DWORD i, dwNumRows, dwNumVars, dwId;
   DB_RESULT hResult;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   pRequest->GetVariableStr(VID_SEARCH_PATTERN, szPattern, MAX_VARIABLE_NAME);
   _sntprintf(szQuery, 256, _T("SELECT var_name FROM user_profiles WHERE user_id=%ld"), m_dwUserId);
   hResult = DBSelect(g_hCoreDB, szQuery);
   if (hResult != NULL)
   {
      dwNumRows = DBGetNumRows(hResult);
      for(i = 0, dwNumVars = 0, dwId = VID_VARLIST_BASE; i < dwNumRows; i++)
      {
         pszName = DBGetField(hResult, i, 0);
         if (MatchString(szPattern, pszName, FALSE))
         {
            dwNumVars++;
            msg.SetVariable(dwId++, pszName);
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

   // Send response
   SendMessage(&msg);
}


//
// Delete user variable
//

void ClientSession::DeleteUserVariable(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   TCHAR szVarName[MAX_VARIABLE_NAME], szQuery[MAX_VARIABLE_NAME + 256];

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   // Try to read variable from database
   pRequest->GetVariableStr(VID_NAME, szVarName, MAX_VARIABLE_NAME);
   _sntprintf(szQuery, MAX_VARIABLE_NAME + 256,
              _T("DELETE FROM user_profiles WHERE user_id=%ld AND var_name='%s'"),
              m_dwUserId, szVarName);
   if (DBQuery(g_hCoreDB, szQuery))
   {
      msg.SetVariable(VID_RCC, RCC_SUCCESS);
   }
   else
   {
      msg.SetVariable(VID_RCC, RCC_DB_FAILURE);
   }

   // Send response
   SendMessage(&msg);
}


//
// Change object's IP address
//

void ClientSession::ChangeObjectIP(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   NetObj *pObject;
   DWORD dwIpAddr;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   // Get object id and check prerequisites
   pObject = FindObjectById(pRequest->GetVariableLong(VID_OBJECT_ID));
   if (pObject != NULL)
   {
      if (pObject->Type() == OBJECT_NODE)
      {
         if (pObject->CheckAccessRights(m_dwUserId, OBJECT_ACCESS_MODIFY))
         {
            dwIpAddr = pRequest->GetVariableLong(VID_IP_ADDRESS);

            // Check if we already have object with given IP
            if ((FindNodeByIP(dwIpAddr) == NULL) &&
                (FindSubnetByIP(dwIpAddr) == NULL))
            {
               ((Node *)pObject)->ChangeIPAddress(dwIpAddr);
               msg.SetVariable(VID_RCC, RCC_SUCCESS);
            }
            else
            {
               msg.SetVariable(VID_RCC, RCC_ADDRESS_IN_USE);
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
   SendMessage(&msg);
}


//
// Setup encryption with client
//

void ClientSession::SetupEncryption(DWORD dwRqId)
{
   CSCPMessage msg;

#ifdef _WITH_ENCRYPTION
   m_dwEncryptionRqId = dwRqId;
   m_dwEncryptionResult = RCC_TIMEOUT;
   if (m_condEncryptionSetup == INVALID_CONDITION_HANDLE)
      m_condEncryptionSetup = ConditionCreate(FALSE);

   // Send request for session key
   PrepareKeyRequestMsg(&msg, g_pServerKey);
   msg.SetId(dwRqId);
   SendMessage(&msg);
   msg.DeleteAllVariables();

   // Wait for encryption setup
   ConditionWait(m_condEncryptionSetup, 3000);

   // Send response
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_RCC, m_dwEncryptionResult);
#else    /* _WITH_ENCRYPTION not defined */
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_RCC, RCC_NO_ENCRYPTION_SUPPORT);
#endif

   SendMessage(&msg);
}


//
// Get agent's configuration file
//

void ClientSession::GetAgentConfig(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   NetObj *pObject;
   DWORD dwResult, dwSize;
   TCHAR *pszConfig;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   // Get object id and check prerequisites
   pObject = FindObjectById(pRequest->GetVariableLong(VID_OBJECT_ID));
   if (pObject != NULL)
   {
      if (pObject->Type() == OBJECT_NODE)
      {
         if (pObject->CheckAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
         {
            AgentConnection *pConn;

            pConn = ((Node *)pObject)->CreateAgentConnection();
            if (pConn != NULL)
            {
               dwResult = pConn->GetConfigFile(&pszConfig, &dwSize);
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
   SendMessage(&msg);
}


//
// Update agent's configuration file
//

void ClientSession::UpdateAgentConfig(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   NetObj *pObject;
   DWORD dwResult;
   TCHAR *pszConfig;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   // Get object id and check prerequisites
   pObject = FindObjectById(pRequest->GetVariableLong(VID_OBJECT_ID));
   if (pObject != NULL)
   {
      if (pObject->Type() == OBJECT_NODE)
      {
         if (pObject->CheckAccessRights(m_dwUserId, OBJECT_ACCESS_CONTROL))
         {
            AgentConnection *pConn;

            pConn = ((Node *)pObject)->CreateAgentConnection();
            if (pConn != NULL)
            {
               pszConfig = pRequest->GetVariableStr(VID_CONFIG_FILE);
               dwResult = pConn->UpdateConfigFile(pszConfig);
               free(pszConfig);

               if ((pRequest->GetVariableShort(VID_APPLY_FLAG) != 0) &&
                   (dwResult == ERR_SUCCESS))
               {
                  dwResult = pConn->ExecAction("Agent.Restart", 0, NULL);
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
                  case ERR_MAILFORMED_COMMAND:
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
   SendMessage(&msg);
}


//
// Execute action on client
//

void ClientSession::ExecuteAction(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   NetObj *pObject;
   DWORD dwResult;
   TCHAR szAction[MAX_PARAM_NAME];

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(pRequest->GetId());

   // Get object id and check prerequisites
   pObject = FindObjectById(pRequest->GetVariableLong(VID_OBJECT_ID));
   if (pObject != NULL)
   {
      if (pObject->Type() == OBJECT_NODE)
      {
         if (pObject->CheckAccessRights(m_dwUserId, OBJECT_ACCESS_CONTROL))
         {
            AgentConnection *pConn;

            pConn = ((Node *)pObject)->CreateAgentConnection();
            if (pConn != NULL)
            {
               pRequest->GetVariableStr(VID_ACTION_NAME, szAction, MAX_PARAM_NAME);
               dwResult = pConn->ExecAction(szAction, 0, NULL);

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
   SendMessage(&msg);
}


//
// Send tool list to client
//

void ClientSession::SendObjectTools(DWORD dwRqId)
{
   CSCPMessage msg;
   DB_RESULT hResult;
   DWORD i, j, dwAclSize, dwNumTools, dwNumMsgRec, dwToolId, dwId;
   OBJECT_TOOL_ACL *pAccessList;

   // Prepare response message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(dwRqId);

   hResult = DBSelect(g_hCoreDB, _T("SELECT tool_id,user_id FROM object_tools_acl"));
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

      hResult = DBSelect(g_hCoreDB, _T("SELECT tool_id,tool_name,tool_type,tool_data,flags,description FROM object_tools"));
      if (hResult != NULL)
      {
         dwNumTools = DBGetNumRows(hResult);
         for(i = 0, dwId = VID_OBJECT_TOOLS_BASE, dwNumMsgRec = 0; i < dwNumTools; i++)
         {
            dwToolId = DBGetFieldULong(hResult, i, 0);
            if (m_dwUserId != 0)
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

            if ((m_dwUserId == 0) || (j < dwAclSize))   // User has access to this tool
            {
               msg.SetVariable(dwId, dwToolId);
               msg.SetVariable(dwId + 1, DBGetField(hResult, i, 1));
               msg.SetVariable(dwId + 2, (WORD)DBGetFieldLong(hResult, i, 2));
               msg.SetVariable(dwId + 3, DBGetField(hResult, i, 3));
               msg.SetVariable(dwId + 4, DBGetFieldULong(hResult, i, 4));
               msg.SetVariable(dwId + 5, DBGetField(hResult, i, 5));
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

   // Send response
   SendMessage(&msg);
}


//
// Execute table tool (either SNMP or agent table)
//

void ClientSession::ExecTableTool(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   DWORD dwToolId;
   NetObj *pObject;

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
         pObject = FindObjectById(pRequest->GetVariableLong(VID_OBJECT_ID));
         if (pObject != NULL)
         {
            if (pObject->Type() == OBJECT_NODE)
            {
               msg.SetVariable(VID_RCC,
                               ExecuteTableTool(dwToolId, (Node *)pObject,
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
   SendMessage(&msg);
}


//
// Change current subscription
//

void ClientSession::ChangeSubscription(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   DWORD dwFlags;

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
   SendMessage(&msg);
}


//
// Get latest syslog records
//

void ClientSession::SendSyslog(CSCPMessage *pRequest)
{
   CSCPMessage msg;
   DWORD dwRqId, dwMaxRecords, dwNumRows, dwId;
   DB_RESULT hTempResult;
   DB_ASYNC_RESULT hResult;
   TCHAR szQuery[1024], szBuffer[1024];

   dwRqId = pRequest->GetId();
   dwMaxRecords = pRequest->GetVariableLong(VID_MAX_RECORDS);

   // Send confirmation message
   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(dwRqId);
   msg.SetVariable(VID_RCC, RCC_SUCCESS);
   SendMessage(&msg);
   msg.DeleteAllVariables();
   msg.SetCode(CMD_SYSLOG_RECORDS);

   MutexLock(m_mutexSendSyslog, INFINITE);

   // Retrieve events from database
   switch(g_dwDBSyntax)
   {
      case DB_SYNTAX_MYSQL:
      case DB_SYNTAX_PGSQL:
         hTempResult = DBSelect(g_hCoreDB, _T("SELECT count(*) FROM syslog"));
         if (hTempResult != NULL)
         {
            if (DBGetNumRows(hTempResult) > 0)
            {
               dwNumRows = DBGetFieldULong(hTempResult, 0, 0);
            }
            else
            {
               dwNumRows = 0;
            }
            DBFreeResult(hTempResult);
         }
         _sntprintf(szQuery, 1024,
                    _T("SELECT msg_id,msg_timestamp,facility,severity,")
                    _T("source_object_id,hostname,msg_tag,msg_text FROM syslog ")
                    _T("ORDER BY msg_timestamp LIMIT %lu OFFSET %lu"),
                    dwMaxRecords, dwNumRows - min(dwNumRows, dwMaxRecords));
         break;
      case DB_SYNTAX_MSSQL:
         _sntprintf(szQuery, 1024,
                    _T("SELECT msg_id,msg_timestamp,facility,severity,")
                    _T("source_object_id,hostname,msg_tag,msg_text ")
                    _T("INTO temp_syslog_%ld FROM syslog ")
                    _T("ORDER BY msg_timestamp DESC LIMIT %lu;"),
                    _T("SELECT * FROM temp_syslog_%ld ORDER BY msg_timestamp;")
                    _T("DROP TABLE temp_syslog_%ld"),
                    m_dwIndex, dwMaxRecords, m_dwIndex, m_dwIndex);
         break;
      default:
         szQuery[0] = 0;
         break;
   }
   hResult = DBAsyncSelect(g_hCoreDB, szQuery);
   if (hResult != NULL)
   {
      // Send events, one per message
      for(dwId = VID_SYSLOG_MSG_BASE, dwNumRows = 0; DBFetch(hResult); dwNumRows++)
      {
         if (dwNumRows == 10)
         {
            msg.SetVariable(VID_NUM_RECORDS, dwNumRows);
            SendMessage(&msg);
            msg.DeleteAllVariables();
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
         DBGetFieldAsync(hResult, 7, szBuffer, 1024);
         DecodeSQLString(szBuffer);
         msg.SetVariable(dwId++, szBuffer);
      }
      DBFreeAsyncResult(hResult);

      // Send remaining records with End-Of-Sequence notification
      msg.SetVariable(VID_NUM_RECORDS, dwNumRows);
      msg.SetEndOfSequence();
      SendMessage(&msg);
      msg.DeleteAllVariables();
   }

   MutexUnlock(m_mutexSendSyslog);
}


//
// Send list of log policies
//

void ClientSession::SendLogPoliciesList(DWORD dwRqId)
{
   CSCPMessage msg;
   DB_RESULT hResult;
   DWORD i, dwNumRows, dwId;

   msg.SetCode(CMD_REQUEST_COMPLETED);
   msg.SetId(dwRqId);

   if (m_dwSystemAccess & SYSTEM_ACCESS_MANAGE_LPP)
   {
      hResult = DBSelect(g_hCoreDB, _T("SELECT lpp_id,lpp_name,lpp_version,lpp_flags FROM lpp"));
      if (hResult != NULL)
      {
         dwNumRows = DBGetNumRows(hResult);
         msg.SetVariable(VID_NUM_RECORDS, dwNumRows);
         for(i = 0, dwId = VID_LPP_LIST_BASE; i < dwNumRows; i++)
         {
            msg.SetVariable(dwId++, DBGetFieldULong(hResult, i, 0));
            msg.SetVariable(dwId++, DBGetField(hResult, i, 1));
            msg.SetVariable(dwId++, DBGetFieldULong(hResult, i, 2));
            msg.SetVariable(dwId++, DBGetFieldULong(hResult, i, 3));
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

   SendMessage(&msg);
}
