/*
** NetXMS - Network Management System
** Copyright (C) 2003-2020 Raden Solutions
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
#include <nxstat.h>
#include <entity_mib.h>
#include <nxcore_websvc.h>
#include <nxcore_logs.h>
#include <nxcore_syslog.h>
#include <nxcore_ps.h>
#include <nms_pkg.h>

#ifdef _WIN32
#include <psapi.h>
#else
#include <dirent.h>
#endif

#ifdef WITH_ZMQ
#include "zeromq.h"
#endif

/**
 * Constants
 */
#define TRAP_CREATE     1
#define TRAP_UPDATE     2
#define TRAP_DELETE     3

#define MAX_MSG_SIZE    4194304

#define DEBUG_TAG _T("client.session")

/**
 * Externals
 */
extern ObjectQueue<DiscoveredAddress> g_nodePollerQueue;
extern ThreadPool *g_clientThreadPool;
extern ThreadPool *g_dataCollectorThreadPool;

void UnregisterClientSession(session_id_t id);
void ResetDiscoveryPoller();
NXCPMessage *ForwardMessageToReportingServer(NXCPMessage *request, ClientSession *session);
void RemovePendingFileTransferRequests(ClientSession *session);
bool UpdateAddressListFromMessage(NXCPMessage *msg);
void FillComponentsMessage(NXCPMessage *msg);
void GetClientConfigurationHints(NXCPMessage *msg);

void GetPredictionEngines(NXCPMessage *msg);
bool GetPredictedData(ClientSession *session, const NXCPMessage *request, NXCPMessage *response, const DataCollectionTarget& dcTarget);

void GetAgentTunnels(NXCPMessage *msg);
uint32_t BindAgentTunnel(uint32_t tunnelId, uint32_t nodeId, uint32_t userId);
uint32_t UnbindAgentTunnel(uint32_t nodeId, uint32_t userId);

void StartManualActiveDiscovery(ObjectArray<InetAddressListElement> *addressList);

void GetObjectPhysicalLinks(const NetObj *object, uint32_t userId, uint32_t patchPanelId, NXCPMessage *msg);
uint32_t AddPhysicalLink(const NXCPMessage *msg, uint32_t userId);
bool DeletePhysicalLink(uint32_t id, uint32_t userId);

/**
 * Client session console constructor
 */
ClientSessionConsole::ClientSessionConsole(ClientSession *session, UINT16 msgCode)
{
   m_session = session;
   m_messageCode = msgCode;
}

/**
 * Client session console destructor
 */
ClientSessionConsole::~ClientSessionConsole()
{
}

/**
 * Client session console constructor
 */
void ClientSessionConsole::write(const TCHAR *text)
{
   NXCPMessage msg(m_messageCode, 0);
   msg.setField(VID_MESSAGE, text);
   m_session->postMessage(&msg);
}

/**
 * Callback for sending image library delete notifications
 */
static void ImageLibraryDeleteCallback(ClientSession *pSession, void *context)
{
	pSession->onLibraryImageChange(*static_cast<const uuid*>(context), true);
}

/**
 * Callback for cancelling pending file transfer
 */
static EnumerationCallbackResult CancelFileTransfer(const uint32_t& key, ServerDownloadFileInfo *file)
{
   file->close(false);
   return _CONTINUE;
}

/**
 * Socket poller callback
 */
void ClientSession::socketPollerCallback(BackgroundSocketPollResult pollResult, SOCKET hSocket, ClientSession *session)
{
   if (!(session->m_flags & CSF_TERMINATE_REQUESTED))
   {
      if (pollResult == BackgroundSocketPollResult::SUCCESS)
      {
         if (session->readSocket() && !(session->m_flags & CSF_TERMINATE_REQUESTED))
         {
            session->m_socketPoller->poller.poll(hSocket, 900000, socketPollerCallback, session);
            return;
         }
      }
      else
      {
         session->debugPrintf(5, _T("Socket poll error (%d)"), static_cast<int>(pollResult));
      }
   }
   ThreadPoolExecute(g_clientThreadPool, terminate, session);
}

/**
 * Terminate session
 */
void ClientSession::terminate(ClientSession *session)
{
   session->finalize();
   UnregisterClientSession(session->m_id);
   delete session;
}

/**
 * Client session class constructor
 */
ClientSession::ClientSession(SOCKET hSocket, const InetAddress& addr)
{
   m_id = -1;
   m_socket = hSocket;
   m_socketPoller = nullptr;
   m_messageReceiver = nullptr;
   m_pCtx = nullptr;
	m_mutexSocketWrite = MutexCreate();
   m_mutexSendAlarms = MutexCreate();
   m_mutexSendActions = MutexCreate();
   m_mutexSendAuditLog = MutexCreate();
   m_mutexPollerInit = MutexCreate();
   m_subscriptionLock = MutexCreateFast();
   m_subscriptions = new StringObjectMap<UINT32>(Ownership::True);
   m_flags = 0;
	m_clientType = CLIENT_TYPE_DESKTOP;
	m_clientAddr = addr;
	m_clientAddr.toString(m_workstation);
   m_webServerAddress[0] = 0;
   m_loginName[0] = 0;
   _tcscpy(m_sessionName, _T("<not logged in>"));
	_tcscpy(m_clientInfo, _T("n/a"));
   m_dwUserId = INVALID_INDEX;
   m_systemAccessRights = 0;
   m_ppEPPRuleList = nullptr;
   m_dwNumRecordsToUpload = 0;
   m_dwRecordsUploaded = 0;
   m_refCount = 0;
   m_encryptionRqId = 0;
   m_encryptionResult = 0;
   m_condEncryptionSetup = INVALID_CONDITION_HANDLE;
	m_console = nullptr;
   m_loginTime = time(nullptr);
   m_soundFileTypes.add(_T("wav"));
   _tcscpy(m_language, _T("en"));
   m_serverCommands = new SynchronizedSharedHashMap<pid_t, ProcessExecutor>();
   m_downloadFileMap = new SynchronizedHashMap<uint32_t, ServerDownloadFileInfo>(Ownership::True);
   m_tcpProxyConnections = new ObjectArray<TcpProxy>(0, 16, Ownership::True);
   m_tcpProxyLock = MutexCreate();
   m_tcpProxyChannelId = 0;
   m_pendingObjectNotifications = new HashSet<uint32_t>();
   m_pendingObjectNotificationsLock = MutexCreate();
   m_objectNotificationDelay = 200;
}

/**
 * Destructor
 */
ClientSession::~ClientSession()
{
   if (m_socketPoller != nullptr)
      InterlockedDecrement(&m_socketPoller->usageCount);
   delete m_messageReceiver;
   if (m_socket != INVALID_SOCKET)
   {
      shutdown(m_socket, SHUT_RDWR);
      closesocket(m_socket);
      debugPrintf(6, _T("Socket closed"));
   }
	MutexDestroy(m_mutexSocketWrite);
   MutexDestroy(m_mutexSendAlarms);
   MutexDestroy(m_mutexSendActions);
   MutexDestroy(m_mutexSendAuditLog);
   MutexDestroy(m_mutexPollerInit);
   MutexDestroy(m_subscriptionLock);
   delete m_subscriptions;
   if (m_ppEPPRuleList != nullptr)
   {
      if (m_flags & CSF_EPP_UPLOAD)  // Aborted in the middle of EPP transfer
      {
         for(UINT32 i = 0; i < m_dwRecordsUploaded; i++)
            delete m_ppEPPRuleList[i];
      }
      MemFree(m_ppEPPRuleList);
   }
	if (m_pCtx != nullptr)
		m_pCtx->decRefCount();
   if (m_condEncryptionSetup != INVALID_CONDITION_HANDLE)
      ConditionDestroy(m_condEncryptionSetup);

	delete m_console;

   m_soundFileTypes.clear();

   m_downloadFileMap->forEach(CancelFileTransfer);
   delete m_downloadFileMap;

   delete m_serverCommands;
   delete m_tcpProxyConnections;
   MutexDestroy(m_tcpProxyLock);
   delete m_pendingObjectNotifications;
   MutexDestroy(m_pendingObjectNotificationsLock);

   debugPrintf(5, _T("Session object destroyed"));
}

/**
 * Start session
 */
bool ClientSession::start()
{
   if (m_socketPoller == nullptr)
      return false;
   m_messageReceiver = new SocketMessageReceiver(m_socket, 4096, MAX_MSG_SIZE);
   m_socketPoller->poller.poll(m_socket, 900000, socketPollerCallback, this);
   return true;
}

/**
 * Check if file transfer is stalled and should be cancelled
 */
EnumerationCallbackResult ClientSession::checkFileTransfer(const uint32_t &key, ServerDownloadFileInfo *fileTransfer, 
      std::pair<ClientSession*, IntegerArray<uint32_t>*> *context)
{
   if (time(nullptr) - fileTransfer->getLastUpdateTime() > 60)
   {
      context->first->debugPrintf(3, _T("File transfer [%u] for file \"%s\" cancelled by timeout"), key, fileTransfer->getFileName());
      context->second->add(key);
      fileTransfer->close(false);
   }
   return _CONTINUE;
}

/**
 * Run session housekeeper
 */
void ClientSession::runHousekeeper()
{
   IntegerArray<uint32_t> cancelList;
   std::pair<ClientSession*, IntegerArray<uint32_t>*> context(this, &cancelList);
   m_downloadFileMap->forEach(ClientSession::checkFileTransfer, &context);
   for (int i = 0; i < cancelList.size(); i++)
   {
      m_downloadFileMap->remove(cancelList.get(i));
   }
}

/**
 * Print debug information
 */
void ClientSession::debugPrintf(int level, const TCHAR *format, ...)
{
   va_list args;
   va_start(args, format);
   nxlog_debug_tag_object2(DEBUG_TAG, m_id, level, format, args);
   va_end(args);
}

/**
 * Write audit log
 */
void ClientSession::writeAuditLog(const TCHAR *subsys, bool success, UINT32 objectId, const TCHAR *format, ...)
{
   va_list args;
   va_start(args, format);
   WriteAuditLog2(subsys, success, m_dwUserId, m_workstation, m_id, objectId, format, args);
   va_end(args);
}

/**
 * Write audit log with old and new values for changed entity
 */
void ClientSession::writeAuditLogWithValues(const TCHAR *subsys, bool success, UINT32 objectId, const TCHAR *oldValue, const TCHAR *newValue, char valueType, const TCHAR *format, ...)
{
   va_list args;
   va_start(args, format);
   WriteAuditLogWithValues2(subsys, success, m_dwUserId, m_workstation, m_id, objectId, oldValue, newValue, valueType, format, args);
   va_end(args);
}

/**
 * Write audit log with old and new values for changed entity
 */
void ClientSession::writeAuditLogWithValues(const TCHAR *subsys, bool success, UINT32 objectId, json_t *oldValue, json_t *newValue, const TCHAR *format, ...)
{
   va_list args;
   va_start(args, format);
   WriteAuditLogWithJsonValues2(subsys, success, m_dwUserId, m_workstation, m_id, objectId, oldValue, newValue, format, args);
   va_end(args);
}

/**
 * Check channel subscription
 */
bool ClientSession::isSubscribedTo(const TCHAR *channel) const
{
   MutexLock(m_subscriptionLock);
   bool subscribed = m_subscriptions->contains(channel);
   MutexUnlock(m_subscriptionLock);
   return subscribed;
}

/**
 * Callback for closing all open data collection configurations
 */
static EnumerationCallbackResult CloseDataCollectionConfiguration(const uint32_t *id, void *arg)
{
   shared_ptr<NetObj> object = FindObjectById(*id);
   if ((object != nullptr) && (object->isDataCollectionTarget() || (object->getObjectClass() == OBJECT_TEMPLATE)))
      static_cast<DataCollectionOwner&>(*object).applyDCIChanges(false);
   return _CONTINUE;
}

/**
 * Read from socket (called from socket poller callback)
 */
bool ClientSession::readSocket()
{
   MessageReceiverResult result = readMessage(true);
   while(result == MSGRECV_SUCCESS)
      result = readMessage(false);
   return (result == MSGRECV_WANT_READ) || (result == MSGRECV_WANT_WRITE);
}

/**
 * Read single message from socket (called from socket poller callback)
 */
MessageReceiverResult ClientSession::readMessage(bool allowSocketRead)
{
   MessageReceiverResult result;
   NXCPMessage *msg = m_messageReceiver->readMessage(0, &result, allowSocketRead);

   if ((result == MSGRECV_WANT_READ) || (result == MSGRECV_WANT_WRITE))
      return result;

   if (result == MSGRECV_DECRYPTION_FAILURE)
   {
      debugPrintf(4, _T("readSocket: Unable to decrypt received message"));
      return result;
   }

   // Receive error
   if (msg == nullptr)
   {
      if (result == MSGRECV_CLOSED)
         debugPrintf(5, _T("readSocket: connection closed"));
      else
         debugPrintf(5, _T("readSocket: message receiving error (%s)"), AbstractMessageReceiver::resultToText(result));
      return result;
   }

   if (nxlog_get_debug_level_tag_object(DEBUG_TAG, m_id) >= 8)
   {
      String msgDump = NXCPMessage::dump(m_messageReceiver->getRawMessageBuffer(), NXCP_VERSION);
      debugPrintf(8, _T("Message dump:\n%s"), (const TCHAR *)msgDump);
   }

   // Special handling for raw messages
   if (msg->isBinary())
   {
      TCHAR buffer[256];
      debugPrintf(6, _T("Received raw message %s"), NXCPMessageCodeName(msg->getCode(), buffer));

      if ((msg->getCode() == CMD_FILE_DATA) || (msg->getCode() == CMD_ABORT_FILE_TRANSFER))
      {
         incRefCount();
         TCHAR key[32];
         _sntprintf(key, 32, _T("FT_%d"), m_id);
         ThreadPoolExecuteSerialized(g_clientThreadPool, key, this, &ClientSession::processFileTransferMessage, msg);
         msg = nullptr;
      }
      else if (msg->getCode() == CMD_TCP_PROXY_DATA)
      {
         shared_ptr<AgentConnectionEx> conn;
         uint32_t agentChannelId = 0;
         MutexLock(m_tcpProxyLock);
         for(int i = 0; i < m_tcpProxyConnections->size(); i++)
         {
            TcpProxy *p = m_tcpProxyConnections->get(i);
            if (p->clientChannelId == msg->getId())
            {
               conn = p->agentConnection;
               agentChannelId = p->agentChannelId;
               break;
            }
         }
         MutexUnlock(m_tcpProxyLock);
         if (conn != nullptr)
         {
            size_t size = msg->getBinaryDataSize();
            size_t msgSize = size + NXCP_HEADER_SIZE;
            if (msgSize % 8 != 0)
               msgSize += 8 - msgSize % 8;
            NXCP_MESSAGE *fwmsg = static_cast<NXCP_MESSAGE*>(MemAlloc(msgSize));
            fwmsg->code = htons(CMD_TCP_PROXY_DATA);
            fwmsg->flags = htons(MF_BINARY);
            fwmsg->id = htonl(agentChannelId);
            fwmsg->numFields = htonl(static_cast<UINT32>(size));
            fwmsg->size = htonl(static_cast<UINT32>(msgSize));
            memcpy(fwmsg->fields, msg->getBinaryData(), size);
            conn->postRawMessage(fwmsg);
         }
      }
      delete msg;
   }
   else
   {
      if ((msg->getCode() == CMD_SESSION_KEY) && (msg->getId() == m_encryptionRqId))
      {
         debugPrintf(6, _T("Received message CMD_SESSION_KEY"));
         m_encryptionResult = SetupEncryptionContext(msg, &m_pCtx, nullptr, g_pServerKey, NXCP_VERSION);
         m_messageReceiver->setEncryptionContext(m_pCtx);
         ConditionSet(m_condEncryptionSetup);
         m_encryptionRqId = 0;
         delete msg;
      }
      else if (msg->getCode() == CMD_KEEPALIVE)
      {
         debugPrintf(6, _T("Received message CMD_KEEPALIVE"));
         respondToKeepalive(msg->getId());
         delete msg;
      }
      else if ((msg->getCode() == CMD_EPP_RECORD) || (msg->getCode() == CMD_OPEN_EPP) || (msg->getCode() == CMD_SAVE_EPP) || (msg->getCode() == CMD_CLOSE_EPP))
      {
         TCHAR buffer[64];
         debugPrintf(6, _T("Received message %s"), NXCPMessageCodeName(msg->getCode(), buffer));

         incRefCount();
         TCHAR key[64];
         _sntprintf(key, 64, _T("EPP_%d"), m_id);
         switch(msg->getCode())
         {
            case CMD_EPP_RECORD:
               ThreadPoolExecuteSerialized(g_clientThreadPool, key, this, &ClientSession::processEventProcessingPolicyRecord, msg);
               break;
            case CMD_OPEN_EPP:
               ThreadPoolExecuteSerialized(g_clientThreadPool, key, this, &ClientSession::openEventProcessingPolicy, msg);
               break;
            case CMD_SAVE_EPP:
               ThreadPoolExecuteSerialized(g_clientThreadPool, key, this, &ClientSession::saveEventProcessingPolicy, msg);
               break;
            case CMD_CLOSE_EPP:
               ThreadPoolExecuteSerialized(g_clientThreadPool, key, this, &ClientSession::closeEventProcessingPolicy, msg);
               break;
         }
      }
      else
      {
         incRefCount();
         ThreadPoolExecute(g_clientThreadPool, this, &ClientSession::processRequest, msg);
      }
   }
   return MSGRECV_SUCCESS;
}

/**
 * Finalize session termination
 */
void ClientSession::finalize()
{
   // Mark as terminated (sendMessage calls will not work after that point)
   InterlockedOr(&m_flags, CSF_TERMINATED);

   // remove all pending file transfers from reporting server
   RemovePendingFileTransferRequests(this);

   RemoveAllSessionLocks(m_id);
   m_openDataCollectionConfigurations.forEach(CloseDataCollectionConfiguration, nullptr);

   // Waiting while reference count becomes 0
   if (m_refCount > 0)
   {
      debugPrintf(3, _T("Waiting for pending requests..."));
      do
      {
         ThreadSleep(1);
      } while(m_refCount > 0);
   }

   CloseAllLogsForSession(m_id);

   if (m_flags & CSF_AUTHENTICATED)
   {
      CALL_ALL_MODULES(pfClientSessionClose, (this));
      WriteAuditLog(AUDIT_SECURITY, TRUE, m_dwUserId, m_workstation, m_id, 0, _T("User logged out (client: %s)"), m_clientInfo);
   }
   debugPrintf(3, _T("Session closed"));
}

/**
 * Finalize file transfer to agent
 */
void ClientSession::finalizeFileTransferToAgent(shared_ptr<AgentConnection> conn, uint32_t requestId)
{
   debugPrintf(6, _T("Waiting for final file transfer confirmation from agent for request ID %u"), requestId);
   uint32_t rcc = conn->waitForRCC(requestId, conn->getCommandTimeout());
   debugPrintf(6, _T("File transfer request %u: %s"), requestId, AgentErrorCodeToText(rcc));

   NXCPMessage response(CMD_REQUEST_COMPLETED, requestId);
   response.setField(VID_RCC, AgentErrorToRCC(rcc));
   sendMessage(&response);
   decRefCount();
}

/**
 * Process file transfer message
 */
void ClientSession::processFileTransferMessage(NXCPMessage *msg)
{
   ServerDownloadFileInfo *dInfo = m_downloadFileMap->get(msg->getId());
   if (dInfo != nullptr)
   {
      if (msg->getCode() == CMD_FILE_DATA)
      {
         if (dInfo->write(msg->getBinaryData(), msg->getBinaryDataSize(), msg->isCompressedStream()))
         {
            if (msg->isEndOfFile())
            {
               debugPrintf(6, _T("Got end of file marker"));
               NXCPMessage response;

               response.setCode(CMD_REQUEST_COMPLETED);
               response.setId(msg->getId());
               response.setField(VID_RCC, RCC_SUCCESS);
               sendMessage(&response);

               dInfo->close(true);
               m_downloadFileMap->remove(msg->getId());
            }
         }
         else
         {
            debugPrintf(6, _T("I/O error"));

            NXCPMessage response(CMD_REQUEST_COMPLETED, msg->getId());
            response.setField(VID_RCC, RCC_IO_ERROR);
            sendMessage(&response);

            dInfo->close(false);
            m_downloadFileMap->remove(msg->getId());
         }
      }
      else
      {
         // Abort current file transfer because of client's problem
         dInfo->close(false);
         m_downloadFileMap->remove(msg->getId());
      }
   }
   else
   {
      shared_ptr<AgentConnection> conn = m_agentConnections.get(msg->getId());
      if (conn != nullptr)
      {
         if (msg->getCode() == CMD_FILE_DATA)
         {
            if (conn->sendMessage(msg))  //send raw message
            {
               if (msg->isEndOfFile())
               {
                  debugPrintf(6, _T("Got end of file marker for request ID %u"), msg->getId());
                  incRefCount();
                  ThreadPoolExecute(g_clientThreadPool, this, &ClientSession::finalizeFileTransferToAgent, conn, msg->getId());
                  m_agentConnections.remove(msg->getId());
               }
            }
            else
            {
               debugPrintf(6, _T("Error while sending file to agent (request ID %u)"), msg->getId());
               m_agentConnections.remove(msg->getId());

               NXCPMessage response(CMD_REQUEST_COMPLETED, msg->getId());
               response.setField(VID_RCC, RCC_COMM_FAILURE);
               sendMessage(&response);
            }
         }
         else
         {
            // Resend abort message
            conn->sendMessage(msg);
            m_agentConnections.remove(msg->getId());
         }
      }
      else
      {
         debugPrintf(4, _T("Out of state message (ID: %d)"), msg->getId());
      }
   }
   delete msg;
   decRefCount();
}

/**
 * Request processing
 */
void ClientSession::processRequest(NXCPMessage *request)
{
   uint16_t code = request->getCode();

   TCHAR buffer[128];
   debugPrintf(6, _T("Received message %s"), NXCPMessageCodeName(code, buffer));
   if (!(m_flags & CSF_AUTHENTICATED) &&
       (code != CMD_LOGIN) &&
       (code != CMD_GET_SERVER_INFO) &&
       (code != CMD_REQUEST_ENCRYPTION) &&
       (code != CMD_GET_MY_CONFIG) &&
       (code != CMD_REGISTER_AGENT))
   {
      delete request;
      decRefCount();
      return;
   }

   switch(code)
   {
      case CMD_LOGIN:
         login(request);
         break;
      case CMD_GET_SERVER_INFO:
         sendServerInfo(request->getId());
         break;
      case CMD_GET_MY_CONFIG:
         sendConfigForAgent(request);
         break;
      case CMD_GET_OBJECTS:
         getObjects(request);
         break;
      case CMD_GET_SELECTED_OBJECTS:
         getSelectedObjects(request);
         break;
      case CMD_QUERY_OBJECTS:
         queryObjects(request);
         break;
      case CMD_QUERY_OBJECT_DETAILS:
         queryObjectDetails(request);
         break;
      case CMD_GET_CONFIG_VARLIST:
         getConfigurationVariables(request->getId());
         break;
      case CMD_GET_PUBLIC_CONFIG_VAR:
         getPublicConfigurationVariable(request);
         break;
      case CMD_SET_CONFIG_VARIABLE:
         setConfigurationVariable(request);
         break;
      case CMD_SET_CONFIG_TO_DEFAULT:
         setDefaultConfigurationVariableValues(request);
         break;
      case CMD_DELETE_CONFIG_VARIABLE:
         deleteConfigurationVariable(request);
         break;
      case CMD_CONFIG_GET_CLOB:
         getConfigCLOB(request);
         break;
      case CMD_CONFIG_SET_CLOB:
         setConfigCLOB(request);
         break;
      case CMD_GET_ALARM_CATEGORIES:
         getAlarmCategories(request->getId());
         break;
      case CMD_MODIFY_ALARM_CATEGORY:
         modifyAlarmCategory(request);
         break;
      case CMD_DELETE_ALARM_CATEGORY:
         deleteAlarmCategory(request);
         break;
      case CMD_LOAD_EVENT_DB:
         sendEventDB(request);
         break;
      case CMD_SET_EVENT_INFO:
         modifyEventTemplate(request);
         break;
      case CMD_DELETE_EVENT_TEMPLATE:
         deleteEventTemplate(request);
         break;
      case CMD_GENERATE_EVENT_CODE:
         generateEventCode(request->getId());
         break;
      case CMD_MODIFY_OBJECT:
         modifyObject(request);
         break;
      case CMD_SET_OBJECT_MGMT_STATUS:
         changeObjectMgmtStatus(request);
         break;
      case CMD_ENTER_MAINT_MODE:
         enterMaintenanceMode(request);
         break;
      case CMD_LEAVE_MAINT_MODE:
         leaveMaintenanceMode(request);
         break;
      case CMD_LOAD_USER_DB:
         sendUserDB(request->getId());
         break;
      case CMD_GET_SELECTED_USERS:
         getSelectedUsers(request);
         break;
      case CMD_CREATE_USER:
         createUser(request);
         break;
      case CMD_UPDATE_USER:
         updateUser(request);
         break;
      case CMD_DETACH_LDAP_USER:
         detachLdapUser(request);
         break;
      case CMD_DELETE_USER:
         deleteUser(request);
         break;
      case CMD_LOCK_USER_DB:
         lockUserDB(request->getId(), TRUE);
         break;
      case CMD_UNLOCK_USER_DB:
         lockUserDB(request->getId(), FALSE);
         break;
      case CMD_SET_PASSWORD:
         setPassword(request);
         break;
      case CMD_VALIDATE_PASSWORD:
         validatePassword(request);
         break;
      case CMD_GET_NODE_DCI_LIST:
         openNodeDCIList(request);
         break;
      case CMD_UNLOCK_NODE_DCI_LIST:
         closeNodeDCIList(request);
         break;
      case CMD_MODIFY_NODE_DCI:
      case CMD_DELETE_NODE_DCI:
         modifyNodeDCI(request);
         break;
      case CMD_SET_DCI_STATUS:
         changeDCIStatus(request);
         break;
      case CMD_COPY_DCI:
         copyDCI(request);
         break;
      case CMD_APPLY_TEMPLATE:
         applyTemplate(request);
         break;
      case CMD_GET_DCI_DATA:
         getCollectedData(request);
         break;
      case CMD_GET_TABLE_DCI_DATA:
         getTableCollectedData(request);
         break;
      case CMD_CLEAR_DCI_DATA:
         clearDCIData(request);
         break;
      case CMD_DELETE_DCI_ENTRY:
         deleteDCIEntry(request);
         break;
      case CMD_FORCE_DCI_POLL:
         forceDCIPoll(request);
         break;
      case CMD_RECALCULATE_DCI_VALUES:
         recalculateDCIValues(request);
         break;
      case CMD_GET_MIB_TIMESTAMP:
         sendMIBTimestamp(request->getId());
         break;
      case CMD_GET_MIB:
         sendMib(request);
         break;
      case CMD_CREATE_OBJECT:
         createObject(request);
         break;
      case CMD_BIND_OBJECT:
         changeObjectBinding(request, TRUE);
         break;
      case CMD_UNBIND_OBJECT:
         changeObjectBinding(request, FALSE);
         break;
      case CMD_ADD_CLUSTER_NODE:
         addClusterNode(request);
         break;
      case CMD_GET_ALL_ALARMS:
         getAlarms(request);
         break;
      case CMD_GET_ALARM_COMMENTS:
         getAlarmComments(request);
         break;
      case CMD_SET_ALARM_STATUS_FLOW:
         updateAlarmStatusFlow(request);
         break;
      case CMD_UPDATE_ALARM_COMMENT:
         updateAlarmComment(request);
         break;
      case CMD_DELETE_ALARM_COMMENT:
         deleteAlarmComment(request);
         break;
      case CMD_GET_ALARM:
         getAlarm(request);
         break;
      case CMD_GET_ALARM_EVENTS:
         getAlarmEvents(request);
         break;
      case CMD_ACK_ALARM:
         acknowledgeAlarm(request);
         break;
      case CMD_RESOLVE_ALARM:
         resolveAlarm(request, false);
         break;
      case CMD_TERMINATE_ALARM:
         resolveAlarm(request, true);
         break;
      case CMD_DELETE_ALARM:
         deleteAlarm(request);
         break;
      case CMD_BULK_RESOLVE_ALARMS:
         bulkResolveAlarms(request, false);
         break;
      case CMD_BULK_TERMINATE_ALARMS:
         bulkResolveAlarms(request, true);
         break;
      case CMD_OPEN_HELPDESK_ISSUE:
         openHelpdeskIssue(request);
         break;
      case CMD_GET_HELPDESK_URL:
         getHelpdeskUrl(request);
         break;
      case CMD_UNLINK_HELPDESK_ISSUE:
         unlinkHelpdeskIssue(request);
         break;
      case CMD_CREATE_ACTION:
         createAction(request);
         break;
      case CMD_MODIFY_ACTION:
         updateAction(request);
         break;
      case CMD_DELETE_ACTION:
         deleteAction(request);
         break;
      case CMD_LOAD_ACTIONS:
         sendAllActions(request->getId());
         break;
      case CMD_DELETE_OBJECT:
         deleteObject(request);
         break;
      case CMD_POLL_NODE:
         forcedNodePoll(request);
         break;
      case CMD_TRAP:
         onTrap(request);
         break;
      case CMD_WAKEUP_NODE:
         onWakeUpNode(request);
         break;
      case CMD_CREATE_TRAP:
         editTrap(TRAP_CREATE, request);
         break;
      case CMD_MODIFY_TRAP:
         editTrap(TRAP_UPDATE, request);
         break;
      case CMD_DELETE_TRAP:
         editTrap(TRAP_DELETE, request);
         break;
      case CMD_LOAD_TRAP_CFG:
         sendAllTraps(request->getId());
         break;
      case CMD_GET_TRAP_CFG_RO:
         sendAllTraps2(request->getId());
         break;
      case CMD_QUERY_PARAMETER:
         queryParameter(request);
         break;
      case CMD_QUERY_TABLE:
         queryAgentTable(request);
         break;
      case CMD_GET_PACKAGE_LIST:
         getInstalledPackages(request->getId());
         break;
      case CMD_INSTALL_PACKAGE:
         installPackage(request);
         break;
      case CMD_REMOVE_PACKAGE:
         removePackage(request);
         break;
      case CMD_GET_PARAMETER_LIST:
         getParametersList(request);
         break;
      case CMD_DEPLOY_PACKAGE:
         deployPackage(request);
         break;
      case CMD_GET_LAST_VALUES:
         getLastValues(request);
         break;
      case CMD_GET_DCI_VALUES:
         getLastValuesByDciId(request);
         break;
      case CMD_GET_TABLE_LAST_VALUE:
         getTableLastValue(request);
         break;
      case CMD_GET_DCI_LAST_VALUE:
         getLastValue(request);
         break;
      case CMD_GET_ACTIVE_THRESHOLDS:
         getActiveThresholds(request);
         break;
      case CMD_GET_THRESHOLD_SUMMARY:
         getThresholdSummary(request);
         break;
      case CMD_GET_USER_VARIABLE:
         getUserVariable(request);
         break;
      case CMD_SET_USER_VARIABLE:
         setUserVariable(request);
         break;
      case CMD_DELETE_USER_VARIABLE:
         deleteUserVariable(request);
         break;
      case CMD_ENUM_USER_VARIABLES:
         enumUserVariables(request);
         break;
      case CMD_COPY_USER_VARIABLE:
         copyUserVariable(request);
         break;
      case CMD_CHANGE_ZONE:
         changeObjectZone(request);
         break;
      case CMD_REQUEST_ENCRYPTION:
         setupEncryption(request);
         break;
      case CMD_READ_AGENT_CONFIG_FILE:
         readAgentConfigFile(request);
         break;
      case CMD_WRITE_AGENT_CONFIG_FILE:
         writeAgentConfigFile(request);
         break;
      case CMD_EXECUTE_ACTION:
         executeAction(request);
         break;
      case CMD_GET_OBJECT_TOOLS:
         getObjectTools(request->getId());
         break;
      case CMD_EXEC_TABLE_TOOL:
         execTableTool(request);
         break;
      case CMD_GET_OBJECT_TOOL_DETAILS:
         getObjectToolDetails(request);
         break;
      case CMD_UPDATE_OBJECT_TOOL:
         updateObjectTool(request);
         break;
      case CMD_DELETE_OBJECT_TOOL:
         deleteObjectTool(request);
         break;
      case CMD_CHANGE_OBJECT_TOOL_STATUS:
         changeObjectToolStatus(request);
         break;
      case CMD_GENERATE_OBJECT_TOOL_ID:
         generateObjectToolId(request->getId());
         break;
      case CMD_CHANGE_SUBSCRIPTION:
         changeSubscription(request);
         break;
      case CMD_GET_SERVER_STATS:
         sendServerStats(request->getId());
         break;
      case CMD_GET_SCRIPT_LIST:
         sendScriptList(request->getId());
         break;
      case CMD_GET_SCRIPT:
         sendScript(request);
         break;
      case CMD_UPDATE_SCRIPT:
         updateScript(request);
         break;
      case CMD_RENAME_SCRIPT:
         renameScript(request);
         break;
      case CMD_DELETE_SCRIPT:
         deleteScript(request);
         break;
      case CMD_GET_SESSION_LIST:
         getSessionList(request->getId());
         break;
      case CMD_KILL_SESSION:
         killSession(request);
         break;
      case CMD_START_SNMP_WALK:
         startSnmpWalk(request);
         break;
      case CMD_RESOLVE_DCI_NAMES:
         resolveDCINames(request);
         break;
      case CMD_GET_DCI_INFO:
         SendDCIInfo(request);
         break;
      case CMD_GET_DCI_THRESHOLDS:
         sendDCIThresholds(request);
         break;
      case CMD_GET_RELATED_EVENTS_LIST:
         getRelatedEventList(request);
         break;
      case CMD_GET_DCI_SCRIPT_LIST:
         getDCIScriptList(request);
         break;
      case CMD_GET_PERFTAB_DCI_LIST:
         sendPerfTabDCIList(request);
         break;
      case CMD_PUSH_DCI_DATA:
         pushDCIData(request);
         break;
      case CMD_GET_AGENT_CONFIGURATION_LIST:
         getAgentConfigurationList(request->getId());
         break;
      case CMD_GET_AGENT_CONFIGURATION:
         getAgentConfiguration(request);
         break;
      case CMD_UPDATE_AGENT_CONFIGURATION:
         updateAgentConfiguration(request);
         break;
      case CMD_DELETE_AGENT_CONFIGURATION:
         deleteAgentConfiguration(request);
         break;
      case CMD_SWAP_AGENT_CONFIGURATIONS:
         swapAgentConfigurations(request);
         break;
      case CMD_GET_OBJECT_COMMENTS:
         getObjectComments(request);
         break;
      case CMD_UPDATE_OBJECT_COMMENTS:
         updateObjectComments(request);
         break;
      case CMD_GET_ADDR_LIST:
         getAddrList(request);
         break;
      case CMD_SET_ADDR_LIST:
         setAddrList(request);
         break;
      case CMD_RESET_COMPONENT:
         resetComponent(request);
         break;
      case CMD_EXPORT_CONFIGURATION:
         exportConfiguration(request);
         break;
      case CMD_IMPORT_CONFIGURATION:
         importConfiguration(request);
         break;
      case CMD_GET_GRAPH_LIST:
         sendGraphList(request);
         break;
      case CMD_GET_GRAPH:
         sendGraph(request);
         break;
      case CMD_SAVE_GRAPH:
         saveGraph(request);
         break;
      case CMD_DELETE_GRAPH:
         deleteGraph(request);
         break;
      case CMD_QUERY_L2_TOPOLOGY:
         queryL2Topology(request);
         break;
      case CMD_QUERY_INTERNAL_TOPOLOGY:
         queryInternalCommunicationTopology(request);
         break;
      case CMD_SEND_NOTIFICATION:
         sendNotification(request);
         break;
      case CMD_GET_COMMUNITY_LIST:
      case CMD_GET_USM_CREDENTIALS:
      case CMD_GET_SNMP_PORT_LIST:
      case CMD_GET_SHARED_SECRET_LIST:
         sendNetworkCredList(request);
         break;
      case CMD_UPDATE_COMMUNITY_LIST:
         updateCommunityList(request);
         break;
      case CMD_UPDATE_SHARED_SECRET_LIST:
         updateSharedSecretList(request);
         break;
      case CMD_UPDATE_SNMP_PORT_LIST:
         updateSNMPPortList(request);
         break;
      case CMD_UPDATE_USM_CREDENTIALS:
         updateUsmCredentials(request);
         break;
      case CMD_GET_PERSISTENT_STORAGE:
         getPersistantStorage(request->getId());
         break;
      case CMD_SET_PSTORAGE_VALUE:
         setPstorageValue(request);
         break;
      case CMD_DELETE_PSTORAGE_VALUE:
         deletePstorageValue(request);
         break;
      case CMD_REGISTER_AGENT:
         registerAgent(request);
         break;
      case CMD_GET_SERVER_FILE:
         getServerFile(request);
         break;
      case CMD_GET_AGENT_FILE:
         getAgentFile(request);
         break;
      case CMD_CANCEL_FILE_MONITORING:
         cancelFileMonitoring(request);
         break;
      case CMD_TEST_DCI_TRANSFORMATION:
         testDCITransformation(request);
         break;
      case CMD_EXECUTE_SCRIPT:
         executeScript(request);
         break;
      case CMD_EXECUTE_LIBRARY_SCRIPT:
         executeLibraryScript(request);
         break;
      case CMD_GET_JOB_LIST:
         sendJobList(request->getId());
         break;
      case CMD_CANCEL_JOB:
         cancelJob(request);
         break;
      case CMD_HOLD_JOB:
         holdJob(request);
         break;
      case CMD_UNHOLD_JOB:
         unholdJob(request);
         break;
      case CMD_GET_CURRENT_USER_ATTR:
         getUserCustomAttribute(request);
         break;
      case CMD_SET_CURRENT_USER_ATTR:
         setUserCustomAttribute(request);
         break;
      case CMD_OPEN_SERVER_LOG:
         openServerLog(request);
         break;
      case CMD_CLOSE_SERVER_LOG:
         closeServerLog(request);
         break;
      case CMD_QUERY_LOG:
         queryServerLog(request);
         break;
      case CMD_GET_LOG_DATA:
         getServerLogQueryData(request);
         break;
      case CMD_GET_LOG_RECORD_DETAILS:
         getServerLogRecordDetails(request);
         break;
      case CMD_FIND_NODE_CONNECTION:
         findNodeConnection(request);
         break;
      case CMD_FIND_MAC_LOCATION:
         findMacAddress(request);
         break;
      case CMD_FIND_IP_LOCATION:
         findIpAddress(request);
         break;
      case CMD_FIND_HOSTNAME_LOCATION:
         findHostname(request);
         break;
      case CMD_GET_IMAGE:
         sendLibraryImage(request);
         break;
      case CMD_CREATE_IMAGE:
         updateLibraryImage(request);
         break;
      case CMD_DELETE_IMAGE:
         deleteLibraryImage(request);
         break;
      case CMD_MODIFY_IMAGE:
         updateLibraryImage(request);
         break;
      case CMD_LIST_IMAGES:
         listLibraryImages(request);
         break;
      case CMD_EXECUTE_SERVER_COMMAND:
         executeServerCommand(request);
         break;
      case CMD_STOP_SERVER_COMMAND:
         stopServerCommand(request);
         break;
      case CMD_LIST_SERVER_FILES:
         listServerFileStore(request);
         break;
      case CMD_UPLOAD_FILE_TO_AGENT:
         uploadFileToAgent(request);
         break;
      case CMD_UPLOAD_FILE:
         receiveFile(request);
         break;
      case CMD_DELETE_FILE:
         deleteFile(request);
         break;
      case CMD_OPEN_CONSOLE:
         openConsole(request->getId());
         break;
      case CMD_CLOSE_CONSOLE:
         closeConsole(request->getId());
         break;
      case CMD_ADM_REQUEST:
         processConsoleCommand(request);
         break;
      case CMD_GET_VLANS:
         getVlans(request);
         break;
      case CMD_GET_NETWORK_PATH:
         getNetworkPath(request);
         break;
      case CMD_GET_NODE_COMPONENTS:
         getNodeComponents(request);
         break;
      case CMD_GET_NODE_SOFTWARE:
         getNodeSoftware(request);
         break;
      case CMD_GET_NODE_HARDWARE:
         getNodeHardware(request);
         break;
      case CMD_GET_WINPERF_OBJECTS:
         getWinPerfObjects(request);
         break;
      case CMD_LIST_MAPPING_TABLES:
         listMappingTables(request);
         break;
      case CMD_GET_MAPPING_TABLE:
         getMappingTable(request);
         break;
      case CMD_UPDATE_MAPPING_TABLE:
         updateMappingTable(request);
         break;
      case CMD_DELETE_MAPPING_TABLE:
         deleteMappingTable(request);
         break;
      case CMD_GET_WIRELESS_STATIONS:
         getWirelessStations(request);
         break;
      case CMD_GET_SUMMARY_TABLES:
         getSummaryTables(request->getId());
         break;
      case CMD_GET_SUMMARY_TABLE_DETAILS:
         getSummaryTableDetails(request);
         break;
      case CMD_MODIFY_SUMMARY_TABLE:
         modifySummaryTable(request);
         break;
      case CMD_DELETE_SUMMARY_TABLE:
         deleteSummaryTable(request);
         break;
      case CMD_QUERY_SUMMARY_TABLE:
         querySummaryTable(request);
         break;
      case CMD_QUERY_ADHOC_SUMMARY_TABLE:
         queryAdHocSummaryTable(request);
         break;
      case CMD_GET_SUBNET_ADDRESS_MAP:
         getSubnetAddressMap(request);
         break;
      case CMD_GET_EFFECTIVE_RIGHTS:
         getEffectiveRights(request);
         break;
      case CMD_GET_FOLDER_SIZE:
      case CMD_GET_FOLDER_CONTENT:
      case CMD_FILEMGR_DELETE_FILE:
      case CMD_FILEMGR_RENAME_FILE:
      case CMD_FILEMGR_MOVE_FILE:
      case CMD_FILEMGR_CREATE_FOLDER:
      case CMD_FILEMGR_COPY_FILE:
         fileManagerControl(request);
         break;
      case CMD_FILEMGR_UPLOAD:
         uploadUserFileToAgent(request);
         break;
      case CMD_GET_SWITCH_FDB:
         getSwitchForwardingDatabase(request);
         break;
      case CMD_GET_ROUTING_TABLE:
         getRoutingTable(request);
         break;
      case CMD_GET_LOC_HISTORY:
         getLocationHistory(request);
         break;
      case CMD_TAKE_SCREENSHOT:
         getScreenshot(request);
         break;
      case CMD_COMPILE_SCRIPT:
         compileScript(request);
         break;
      case CMD_CLEAN_AGENT_DCI_CONF:
         cleanAgentDciConfiguration(request);
         break;
      case CMD_RESYNC_AGENT_DCI_CONF:
         resyncAgentDciConfiguration(request);
         break;
      case CMD_LIST_SCHEDULE_CALLBACKS:
         getSchedulerTaskHandlers(request);
         break;
      case CMD_LIST_SCHEDULES:
         getScheduledTasks(request);
         break;
      case CMD_ADD_SCHEDULE:
         addScheduledTask(request);
         break;
      case CMD_UPDATE_SCHEDULE:
         updateScheduledTask(request);
         break;
      case CMD_REMOVE_SCHEDULE:
         removeScheduledTask(request);
         break;
      case CMD_GET_REPOSITORIES:
         getRepositories(request);
         break;
      case CMD_ADD_REPOSITORY:
         addRepository(request);
         break;
      case CMD_MODIFY_REPOSITORY:
         modifyRepository(request);
         break;
      case CMD_DELETE_REPOSITORY:
         deleteRepository(request);
         break;
      case CMD_GET_AGENT_TUNNELS:
         getAgentTunnels(request);
         break;
      case CMD_BIND_AGENT_TUNNEL:
         bindAgentTunnel(request);
         break;
      case CMD_UNBIND_AGENT_TUNNEL:
         unbindAgentTunnel(request);
         break;
      case CMD_GET_PREDICTION_ENGINES:
         getPredictionEngines(request);
         break;
      case CMD_GET_PREDICTED_DATA:
         getPredictedData(request);
         break;
      case CMD_EXPAND_MACROS:
         expandMacros(request);
         break;
      case CMD_SETUP_TCP_PROXY:
         setupTcpProxy(request);
         break;
      case CMD_GET_AGENT_POLICY_LIST:
         getPolicyList(request);
         break;
      case CMD_GET_AGENT_POLICY:
         getPolicy(request);
         break;
      case CMD_UPDATE_AGENT_POLICY:
         updatePolicy(request);
         break;
      case CMD_DELETE_AGENT_POLICY:
         deletePolicy(request);
         break;
      case CMD_GET_DEPENDENT_NODES:
         getDependentNodes(request);
         break;
      case CMD_POLICY_EDITOR_CLOSED:
         onPolicyEditorClose(request);
         break;
      case CMD_POLICY_FORCE_APPLY:
         forceApplyPolicy(request);
         break;
      case CMD_GET_MATCHING_DCI:
         getMatchingDCI(request);
         break;
      case CMD_GET_UA_NOTIFICATIONS:
         getUserAgentNotification(request);
         break;
      case CMD_ADD_UA_NOTIFICATION:
         addUserAgentNotification(request);
         break;
      case CMD_RECALL_UA_NOTIFICATION:
         recallUserAgentNotification(request);
         break;
      case CMD_GET_NOTIFICATION_CHANNELS:
         getNotificationChannels(request->getId());
         break;
      case CMD_ADD_NOTIFICATION_CHANNEL:
         addNotificationChannel(request);
         break;
      case CMD_UPDATE_NOTIFICATION_CHANNEL:
         updateNotificationChannel(request);
         break;
      case CMD_DELETE_NOTIFICATION_CHANNEL:
         removeNotificationChannel(request);
         break;
      case CMD_RENAME_NOTIFICATION_CHANNEL:
         renameNotificationChannel(request);
         break;
      case CMD_GET_NOTIFICATION_DRIVERS:
         getNotificationDriverNames(request->getId());
         break;
      case CMD_START_ACTIVE_DISCOVERY:
         startActiveDiscovery(request);
         break;
      case CMD_GET_PHYSICAL_LINKS:
         getPhysicalLinks(request);
         break;
      case CMD_UPDATE_PHYSICAL_LINK:
         updatePhysicalLink(request);
         break;
      case CMD_DELETE_PHYSICAL_LINK:
         deletePhysicalLink(request);
         break;
      case CMD_GET_WEB_SERVICES:
         getWebServices(request);
         break;
      case CMD_MODIFY_WEB_SERVICE:
         modifyWebService(request);
         break;
      case CMD_DELETE_WEB_SERVICE:
         deleteWebService(request);
         break;
      case CMD_GET_OBJECT_CATEGORIES:
         getObjectCategories(request);
         break;
      case CMD_MODIFY_OBJECT_CATEGORY:
         modifyObjectCategory(request);
         break;
      case CMD_DELETE_OBJECT_CATEGORY:
         deleteObjectCategory(request);
         break;
      case CMD_GET_GEO_AREAS:
         getGeoAreas(request);
         break;
      case CMD_MODIFY_GEO_AREA:
         modifyGeoArea(request);
         break;
      case CMD_DELETE_GEO_AREA:
         deleteGeoArea(request);
         break;
      case CMD_FIND_PROXY_FOR_NODE:
         findProxyForNode(request);
         break;
#ifdef WITH_ZMQ
      case CMD_ZMQ_SUBSCRIBE_EVENT:
         zmqManageSubscription(request, zmq::EVENT, true);
         break;
      case CMD_ZMQ_UNSUBSCRIBE_EVENT:
         zmqManageSubscription(request, zmq::EVENT, false);
         break;
      case CMD_ZMQ_SUBSCRIBE_DATA:
         zmqManageSubscription(request, zmq::DATA, true);
         break;
      case CMD_ZMQ_UNSUBSCRIBE_DATA:
         zmqManageSubscription(request, zmq::DATA, false);
         break;
      case CMD_ZMQ_GET_EVT_SUBSCRIPTIONS:
         zmqListSubscriptions(request, zmq::EVENT);
         break;
      case CMD_ZMQ_GET_DATA_SUBSCRIPTIONS:
         zmqListSubscriptions(request, zmq::DATA);
         break;
#endif
      default:
         if ((code >> 8) == 0x11)
         {
            // Reporting Server range (0x1100 - 0x11FF)
            forwardToReportingServer(request);
         }
         else
         {
            // Pass message to loaded modules
            bool processedByModule = false;
            ENUMERATE_MODULES(pfClientCommandHandler)
            {
               int status = CURRENT_MODULE.pfClientCommandHandler(code, request, this);
               if (status != NXMOD_COMMAND_IGNORED)
               {
                  if (status == NXMOD_COMMAND_ACCEPTED_ASYNC)
                  {
                     request = nullptr;	// Prevent deletion
                  }
                  processedByModule = true;
                  break;   // Message was processed by the module
               }
            }
            if (!processedByModule)
            {
               NXCPMessage response(CMD_REQUEST_COMPLETED, request->getId());
               response.setField(VID_RCC, RCC_NOT_IMPLEMENTED);
               sendMessage(response);
            }
         }
         break;
   }
   delete request;
   decRefCount();
}

/**
 * Respond to client's keepalive message
 */
void ClientSession::respondToKeepalive(UINT32 dwRqId)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, dwRqId);
   msg.setField(VID_RCC, RCC_SUCCESS);
   postMessage(msg);
}

/**
 * Send message to client
 */
bool ClientSession::sendMessage(const NXCPMessage& msg)
{
   if (isTerminated())
      return false;

	NXCP_MESSAGE *rawMsg = msg.serialize((m_flags & CSF_COMPRESSION_ENABLED) != 0);

   if ((nxlog_get_debug_level_tag_object(DEBUG_TAG, m_id) >= 6) && (msg.getCode() != CMD_ADM_MESSAGE))
   {
      TCHAR buffer[128];
      debugPrintf(6, _T("Sending%s message %s (%d bytes)"),
               (ntohs(rawMsg->flags) & MF_COMPRESSED) ? _T(" compressed") : _T(""), NXCPMessageCodeName(msg.getCode(), buffer), ntohl(rawMsg->size));
      if (nxlog_get_debug_level_tag_object(DEBUG_TAG, m_id) >= 8)
      {
         String msgDump = NXCPMessage::dump(rawMsg, NXCP_VERSION);
         debugPrintf(8, _T("Message dump:\n%s"), (const TCHAR *)msgDump);
      }
   }

   bool result;
   if (m_pCtx != nullptr)
   {
      NXCP_ENCRYPTED_MESSAGE *enMsg = m_pCtx->encryptMessage(rawMsg);
      if (enMsg != nullptr)
      {
         result = (SendEx(m_socket, (char *)enMsg, ntohl(enMsg->size), 0, m_mutexSocketWrite) == (int)ntohl(enMsg->size));
         MemFree(enMsg);
      }
      else
      {
         result = false;
      }
   }
   else
   {
      result = (SendEx(m_socket, (const char *)rawMsg, ntohl(rawMsg->size), 0, m_mutexSocketWrite) == (int)ntohl(rawMsg->size));
   }
   MemFree(rawMsg);

   if (!result)
   {
      InterlockedOr(&m_flags, CSF_TERMINATE_REQUESTED);
      m_socketPoller->poller.cancel(m_socket);
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

   uint16_t code = htons(msg->code);
   if ((code != CMD_ADM_MESSAGE) && (nxlog_get_debug_level_tag_object(DEBUG_TAG, m_id) >= 6))
   {
      TCHAR buffer[128];
	   debugPrintf(6, _T("Sending%s message %s (%d bytes)"),
	            (ntohs(msg->flags) & MF_COMPRESSED) ? _T(" compressed") : _T(""), NXCPMessageCodeName(ntohs(msg->code), buffer), ntohl(msg->size));
      if (nxlog_get_debug_level_tag_object(DEBUG_TAG, m_id) >= 8)
      {
         String msgDump = NXCPMessage::dump(msg, NXCP_VERSION);
         debugPrintf(8, _T("Message dump:\n%s"), (const TCHAR *)msgDump);
      }
   }

   bool result;
   if (m_pCtx != nullptr)
   {
      NXCP_ENCRYPTED_MESSAGE *emsg = m_pCtx->encryptMessage(msg);
      if (emsg != nullptr)
      {
         result = (SendEx(m_socket, (char *)emsg, ntohl(emsg->size), 0, m_mutexSocketWrite) == (int)ntohl(emsg->size));
         MemFree(emsg);
      }
      else
      {
         result = false;
      }
   }
   else
   {
      result = (SendEx(m_socket, (const char *)msg, ntohl(msg->size), 0, m_mutexSocketWrite) == (int)ntohl(msg->size));
   }

   if (!result)
   {
      InterlockedOr(&m_flags, CSF_TERMINATE_REQUESTED);
      m_socketPoller->poller.cancel(m_socket);
   }
}

/**
 * Send raw message to client and delete message after sending
 */
void ClientSession::sendRawMessageAndDelete(NXCP_MESSAGE *msg)
{
   sendRawMessage(msg);
   MemFree(msg);
   decRefCount();
}

/**
 * Send raw message in background and delete after sending
 */
void ClientSession::postRawMessageAndDelete(NXCP_MESSAGE *msg)
{
   TCHAR key[32];
   _sntprintf(key, 32, _T("POST/%u"), m_id);
   incRefCount();
   ThreadPoolExecuteSerialized(g_clientThreadPool, key, this, &ClientSession::sendRawMessageAndDelete, msg);
}

/**
 * Send file to client
 */
bool ClientSession::sendFile(const TCHAR *file, uint32_t requestId, long ofset, bool allowCompression)
{
   return !isTerminated() ? SendFileOverNXCP(m_socket, requestId, file, m_pCtx,
            ofset, nullptr, nullptr, m_mutexSocketWrite, isCompressionEnabled() && allowCompression ? NXCP_STREAM_COMPRESSION_DEFLATE : NXCP_STREAM_COMPRESSION_NONE) : false;
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
      CLIENT_PROTOCOL_VERSION_FULL,
      CLIENT_PROTOCOL_VERSION_TCPPROXY,
      CLIENT_PROTOCOL_VERSION_SCHEDULER
   };
   NXCPMessage msg;
	TCHAR szBuffer[MAX_CONFIG_VALUE];
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
   msg.setField(VID_SERVER_VERSION, NETXMS_BUILD_TAG);
   msg.setField(VID_SERVER_ID, g_serverId);
   msg.setField(VID_SUPPORTED_ENCRYPTION, (UINT32)0);
   msg.setField(VID_PROTOCOL_VERSION, (UINT32)CLIENT_PROTOCOL_VERSION_BASE);
   msg.setFieldFromInt32Array(VID_PROTOCOL_VERSION_EX, sizeof(protocolVersions) / sizeof(UINT32), protocolVersions);
   msg.setField(VID_CHALLENGE, m_challenge, CLIENT_CHALLENGE_SIZE);
   msg.setField(VID_TIMESTAMP, (UINT32)time(nullptr));

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

	time_t t = time(nullptr);
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
	         abs(gmtOffset), (tzname[1] != nullptr) ? tzname[1] : "");
#else
	sprintf(szBuffer, "%s%c%02d%s", tzname[0], (gmtOffset >= 0) ? '+' : '-',
	        abs(gmtOffset), (tzname[1] != nullptr) ? tzname[1] : "");
#endif

#else /* not Windows and no tm_gmtoff */

   time_t t = time(nullptr);
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
	         abs(gmtOffset), (tzname[1] != nullptr) ? tzname[1] : "");
#else
	sprintf(szBuffer, "%s%c%02d%s", tzname[0], (gmtOffset >= 0) ? '+' : '-',
	        abs(gmtOffset), (tzname[1] != nullptr) ? tzname[1] : "");
#endif

#endif

	msg.setField(VID_TIMEZONE, szBuffer);
	debugPrintf(2, _T("Server time zone: %s"), szBuffer);

	ConfigReadStr(_T("TileServerURL"), szBuffer, MAX_CONFIG_VALUE, _T("http://tile.openstreetmap.org/"));
	msg.setField(VID_TILE_SERVER_URL, szBuffer);

	ConfigReadStr(_T("DefaultConsoleDateFormat"), szBuffer, MAX_CONFIG_VALUE, _T("dd.MM.yyyy"));
	msg.setField(VID_DATE_FORMAT, szBuffer);

	ConfigReadStr(_T("DefaultConsoleTimeFormat"), szBuffer, MAX_CONFIG_VALUE, _T("HH:mm:ss"));
	msg.setField(VID_TIME_FORMAT, szBuffer);

	ConfigReadStr(_T("DefaultConsoleShortTimeFormat"), szBuffer, MAX_CONFIG_VALUE, _T("HH:mm"));
	msg.setField(VID_SHORT_TIME_FORMAT, szBuffer);

	FillComponentsMessage(&msg);

   // Send response
   sendMessage(msg);
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
   uint32_t rcc;
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

   if (!(m_flags & CSF_AUTHENTICATED))
   {
      uint32_t graceLogins = 0;
      bool closeOtherSessions = false;
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
				rcc = AuthenticateUser(szLogin, szPassword, 0, nullptr, nullptr, &m_dwUserId,
													 &m_systemAccessRights, &changePasswd, &intruderLockout,
													 &closeOtherSessions, false, &graceLogins);
				break;
			case NETXMS_AUTH_TYPE_CERTIFICATE:
#ifdef _WITH_ENCRYPTION
				pCert = CertificateFromLoginMessage(pRequest);
				if (pCert != nullptr)
				{
               size_t sigLen;
					const BYTE *signature = pRequest->getBinaryFieldPtr(VID_SIGNATURE, &sigLen);
               if (signature != nullptr)
               {
                  rcc = AuthenticateUser(szLogin, reinterpret_cast<const TCHAR*>(signature), sigLen,
                        pCert, m_challenge, &m_dwUserId, &m_systemAccessRights, &changePasswd, &intruderLockout,
                        &closeOtherSessions, false, &graceLogins);
               }
               else
               {
                  rcc = RCC_INVALID_REQUEST;
               }
					X509_free(pCert);
				}
				else
				{
					rcc = RCC_BAD_CERTIFICATE;
				}
#else
				rcc = RCC_NOT_IMPLEMENTED;
#endif
				break;
         case NETXMS_AUTH_TYPE_SSO_TICKET:
            char ticket[1024];
            pRequest->getFieldAsMBString(VID_PASSWORD, ticket, 1024);
            if (CASAuthenticate(ticket, szLogin))
            {
               debugPrintf(5, _T("SSO ticket %hs is valid, login name %s"), ticket, szLogin);
				   rcc = AuthenticateUser(szLogin, nullptr, 0, nullptr, nullptr, &m_dwUserId,
													    &m_systemAccessRights, &changePasswd, &intruderLockout,
													    &closeOtherSessions, true, &graceLogins);
            }
            else
            {
               debugPrintf(5, _T("SSO ticket %hs is invalid"), ticket);
               rcc = RCC_ACCESS_DENIED;
            }
            break;
			default:
				rcc = RCC_UNSUPPORTED_AUTH_TYPE;
				break;
		}

      // Additional validation by loaded modules
      if (rcc == RCC_SUCCESS)
      {
         ENUMERATE_MODULES(pfAdditionalLoginCheck)
         {
            rcc = CURRENT_MODULE.pfAdditionalLoginCheck(m_dwUserId, pRequest);
            if (rcc != RCC_SUCCESS)
            {
               debugPrintf(4, _T("Login blocked by module %s (rcc=%d)"), CURRENT_MODULE.szName, rcc);
               break;
            }
         }
      }

      if (rcc == RCC_SUCCESS)
      {
         InterlockedOr(&m_flags, CSF_AUTHENTICATED);
         _tcslcpy(m_loginName, szLogin, MAX_USER_NAME);
         _sntprintf(m_sessionName, MAX_SESSION_NAME, _T("%s@%s"), szLogin, m_workstation);
         m_loginTime = time(nullptr);
         msg.setField(VID_RCC, RCC_SUCCESS);
         msg.setField(VID_USER_SYS_RIGHTS, m_systemAccessRights);
         msg.setField(VID_USER_ID, m_dwUserId);
			msg.setField(VID_SESSION_ID, (UINT32)m_id);
			msg.setField(VID_CHANGE_PASSWD_FLAG, (WORD)changePasswd);
         msg.setField(VID_DBCONN_STATUS, (UINT16)((g_flags & AF_DB_CONNECTION_LOST) ? 0 : 1));
			msg.setField(VID_ZONING_ENABLED, (UINT16)((g_flags & AF_ENABLE_ZONING) ? 1 : 0));
			msg.setField(VID_POLLING_INTERVAL, (INT32)DCObject::m_defaultPollingInterval);
			msg.setField(VID_RETENTION_TIME, (INT32)DCObject::m_defaultRetentionTime);
			msg.setField(VID_ALARM_STATUS_FLOW_STATE, ConfigReadBoolean(_T("StrictAlarmStatusFlow"), false));
			msg.setField(VID_TIMED_ALARM_ACK_ENABLED, ConfigReadBoolean(_T("EnableTimedAlarmAck"), false));
			msg.setField(VID_VIEW_REFRESH_INTERVAL, (UINT16)ConfigReadInt(_T("Client.MinViewRefreshInterval"), 300));
			msg.setField(VID_HELPDESK_LINK_ACTIVE, (UINT16)((g_flags & AF_HELPDESK_LINK_ACTIVE) ? 1 : 0));
			msg.setField(VID_ALARM_LIST_DISP_LIMIT, ConfigReadULong(_T("Client.AlarmList.DisplayLimit"), 4096));
         msg.setField(VID_SERVER_COMMAND_TIMEOUT, ConfigReadULong(_T("ServerCommandOutputTimeout"), 60));
         msg.setField(VID_GRACE_LOGINS, graceLogins);

         GetClientConfigurationHints(&msg);
         FillLicenseProblemsMessage(&msg);

         if (pRequest->getFieldAsBoolean(VID_ENABLE_COMPRESSION))
         {
            debugPrintf(3, _T("Protocol level compression is supported by client"));
            InterlockedOr(&m_flags, CSF_COMPRESSION_ENABLED);
            msg.setField(VID_ENABLE_COMPRESSION, true);
         }
         else
         {
            debugPrintf(3, _T("Protocol level compression is not supported by client"));
         }

         TCHAR buffer[MAX_DB_STRING];
         ConfigReadStr(_T("ServerName"), buffer, MAX_DB_STRING, _T(""));
         msg.setField(VID_SERVER_NAME, buffer);

         ConfigReadStr(_T("ServerColor"), buffer, MAX_DB_STRING, _T(""));
         msg.setField(VID_SERVER_COLOR, buffer);

         ConfigReadStr(_T("MessageOfTheDay"), buffer, MAX_DB_STRING, _T(""));
         msg.setField(VID_MESSAGE_OF_THE_DAY, buffer);

         debugPrintf(3, _T("User %s authenticated (language=%s clientInfo=\"%s\")"), m_sessionName, m_language, m_clientInfo);
			writeAuditLog(AUDIT_SECURITY, true, 0, _T("User \"%s\" logged in (language: %s; client info: %s)"), szLogin, m_language, m_clientInfo);

			if (closeOtherSessions)
			{
			   debugPrintf(5, _T("Closing other sessions for user %s"), m_loginName);
			   CloseOtherSessions(m_dwUserId, m_id);
			}
      }
      else
      {
         msg.setField(VID_RCC, rcc);
			writeAuditLog(AUDIT_SECURITY, false, 0,
			              _T("User \"%s\" login failed with error code %d (client info: %s)"),
							  szLogin, rcc, m_clientInfo);
			if (intruderLockout)
			{
				writeAuditLog(AUDIT_SECURITY, false, 0,
								  _T("User account \"%s\" temporary disabled due to excess count of failed authentication attempts"), szLogin);
			}
			m_dwUserId = INVALID_INDEX;   // reset user ID to avoid incorrect count of logged in sessions for that user
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
 * Update system access rights for logged in user
 */
void ClientSession::updateSystemAccessRights()
{
   if (!(m_flags & CSF_AUTHENTICATED))
      return;

   uint64_t systemAccessRights = GetEffectiveSystemRights(m_dwUserId);
   if (systemAccessRights != m_systemAccessRights)
   {
      debugPrintf(3, _T("System access rights updated (") UINT64X_FMT(_T("016")) _T(" -> ")  UINT64X_FMT(_T("016")) _T(")"), m_systemAccessRights, systemAccessRights);
      m_systemAccessRights = systemAccessRights;

      NXCPMessage msg;
      msg.setCode(CMD_UPDATE_SYSTEM_ACCESS_RIGHTS);
      msg.setField(VID_USER_SYS_RIGHTS, systemAccessRights);
      postMessage(&msg);
   }
}

/**
 * Send alarm categories to client
*/
void ClientSession::getAlarmCategories(UINT32 requestId)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, requestId);
   if (checkSysAccessRights(SYSTEM_ACCESS_EPP))
   {
      GetAlarmCategories(&msg);
      msg.setField(VID_RCC, RCC_SUCCESS);
   }
   else
   {
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);
   }
   sendMessage(&msg);
}

/**
* Update alarm category
*/
void ClientSession::modifyAlarmCategory(NXCPMessage *request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request->getId());
   if (checkSysAccessRights(SYSTEM_ACCESS_EPP))
   {
      uint32_t id = 0;
      msg.setField(VID_RCC, UpdateAlarmCategory(request, &id));
      msg.setField(VID_CATEGORY_ID, id);
   }
   else
   {
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);
   }
   sendMessage(&msg);
}

/**
* Delete alarm category
*/
void ClientSession::deleteAlarmCategory(NXCPMessage *request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request->getId());
   if (checkSysAccessRights(SYSTEM_ACCESS_EPP))
   {
      uint32_t categoryId = request->getFieldAsInt32(VID_CATEGORY_ID);
      if (!g_pEventPolicy->isCategoryInUse(categoryId))
      {
         msg.setField(VID_RCC, DeleteAlarmCategory(categoryId));
      }
      else
      {
         msg.setField(VID_RCC, RCC_CATEGORY_IN_USE);
      }
   }
   else
   {
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);
   }
   sendMessage(&msg);
}

/**
 * Send event configuration to client
 */
void ClientSession::sendEventDB(NXCPMessage *pRequest)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, pRequest->getId());
   if (checkSysAccessRights(SYSTEM_ACCESS_VIEW_EVENT_DB) || checkSysAccessRights(SYSTEM_ACCESS_EDIT_EVENT_DB) || checkSysAccessRights(SYSTEM_ACCESS_EPP))
   {
      if (!(g_flags & AF_DB_CONNECTION_LOST))
      {
         msg.setField(VID_RCC, RCC_SUCCESS);
         GetEventConfiguration(&msg);
      }
   }
   else
   {
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);
   }
   sendMessage(&msg);
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
      json_t *oldValue, *newValue;
      UINT32 rcc = UpdateEventTemplate(pRequest, &msg, &oldValue, &newValue);
      if (rcc == RCC_SUCCESS)
      {
         TCHAR name[MAX_EVENT_NAME];
         pRequest->getFieldAsString(VID_NAME, name, MAX_EVENT_NAME);
         writeAuditLogWithValues(AUDIT_SYSCFG, true, 0, oldValue, newValue, _T("Event template %s [%d] modified"), name, pRequest->getFieldAsUInt32(VID_EVENT_CODE));
         json_decref(oldValue);
         json_decref(newValue);
      }
      msg.setField(VID_RCC, rcc);
   }
   else
   {
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);
      writeAuditLog(AUDIT_SYSCFG, false, 0, _T("Access denied on modify event template [%d]"), pRequest->getFieldAsUInt32(VID_EVENT_CODE)); // TODO change message
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
      UINT32 rcc = DeleteEventTemplate(dwEventCode);
      if (rcc == RCC_SUCCESS)
			writeAuditLog(AUDIT_SYSCFG, true, 0, _T("Event template [%d] deleted"), dwEventCode);

      msg.setField(VID_RCC, rcc);
   }
   else
   {
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);
      writeAuditLog(AUDIT_SYSCFG, false, 0, _T("Access denied on delete event template [%d]"), dwEventCode);
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
 * Data for session object filter
 */
struct SessionObjectFilterData
{
   ClientSession *session;
   time_t baseTimeStamp;
};

/**
 * Object filter for ClientSession::sendAllObjects
 */
static bool SessionObjectFilter(NetObj *object, void *data)
{
   return !object->isHidden() && !object->isSystem() && !object->isDeleted() &&
          (object->getTimeStamp() >= ((SessionObjectFilterData *)data)->baseTimeStamp) &&
          object->checkAccessRights(((SessionObjectFilterData *)data)->session->getUserId(), OBJECT_ACCESS_READ);
}

/**
 * Send all objects to client
 */
void ClientSession::getObjects(NXCPMessage *request)
{
   NXCPMessage msg;

   // Send confirmation message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(request->getId());
   msg.setField(VID_RCC, RCC_SUCCESS);
   sendMessage(&msg);
   msg.deleteAllFields();

   // Change "sync comments" flag
   if (request->getFieldAsBoolean(VID_SYNC_COMMENTS))
      InterlockedOr(&m_flags, CSF_SYNC_OBJECT_COMMENTS);
   else
      InterlockedAnd(&m_flags, ~CSF_SYNC_OBJECT_COMMENTS);

   // Set sync components flag
   bool syncNodeComponents = false;
   if (request->getFieldAsBoolean(VID_SYNC_NODE_COMPONENTS))
      syncNodeComponents = true;

   // Prepare message
   msg.setCode(CMD_OBJECT);

   // Send objects, one per message
   SessionObjectFilterData data;
   data.session = this;
   data.baseTimeStamp = request->getFieldAsTime(VID_TIMESTAMP);
	SharedObjectArray<NetObj> *objects = g_idxObjectById.getObjects(SessionObjectFilter, &data);
	for(int i = 0; i < objects->size(); i++)
	{
      NetObj *object = objects->get(i);
	   if (!syncNodeComponents && (object->getObjectClass() == OBJECT_INTERFACE ||
	         object->getObjectClass() == OBJECT_ACCESSPOINT || object->getObjectClass() == OBJECT_VPNCONNECTOR ||
	         object->getObjectClass() == OBJECT_NETWORKSERVICE))
	   {
         continue;
	   }

      object->fillMessage(&msg, m_dwUserId);
      if (m_flags & CSF_SYNC_OBJECT_COMMENTS)
         object->commentsToMessage(&msg);
      if ((object->getObjectClass() == OBJECT_NODE) && !object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_MODIFY))
      {
         // mask passwords
         msg.setField(VID_SHARED_SECRET, _T("********"));
         msg.setField(VID_SNMP_AUTH_PASSWORD, _T("********"));
         msg.setField(VID_SNMP_PRIV_PASSWORD, _T("********"));
      }
      sendMessage(&msg);
      msg.deleteAllFields();
	}
	delete objects;

   // Send end of list notification
   msg.setCode(CMD_OBJECT_LIST_END);
   sendMessage(&msg);

   InterlockedOr(&m_flags, CSF_OBJECT_SYNC_FINISHED);
}

/**
 * Send selected objects to client
 */
void ClientSession::getSelectedObjects(NXCPMessage *request)
{
   NXCPMessage msg;

   // Send confirmation message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(request->getId());
   msg.setField(VID_RCC, RCC_SUCCESS);
   sendMessage(&msg);
   msg.deleteAllFields();

   // Change "sync comments" flag
   if (request->getFieldAsBoolean(VID_SYNC_COMMENTS))
      InterlockedOr(&m_flags, CSF_SYNC_OBJECT_COMMENTS);
   else
      InterlockedAnd(&m_flags, ~CSF_SYNC_OBJECT_COMMENTS);

   time_t timestamp = request->getFieldAsTime(VID_TIMESTAMP);
	IntegerArray<uint32_t> objects;
	request->getFieldAsInt32Array(VID_OBJECT_LIST, &objects);
	uint32_t options = request->getFieldAsUInt16(VID_FLAGS);

   // Prepare message
	msg.setCode((options & OBJECT_SYNC_SEND_UPDATES) ? CMD_OBJECT_UPDATE : CMD_OBJECT);

   // Send objects, one per message
   for(int i = 0; i < objects.size(); i++)
	{
		shared_ptr<NetObj> object = FindObjectById(objects.get(i));
      if ((object != nullptr) &&
		    object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ) &&
          (object->getTimeStamp() >= timestamp) &&
          !object->isHidden() && !object->isSystem())
      {
         object->fillMessage(&msg, m_dwUserId);
         if (m_flags & CSF_SYNC_OBJECT_COMMENTS)
            object->commentsToMessage(&msg);
         if ((object->getObjectClass() == OBJECT_NODE) && !object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_MODIFY))
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

   InterlockedOr(&m_flags, CSF_OBJECT_SYNC_FINISHED);

	if (options & OBJECT_SYNC_DUAL_CONFIRM)
	{
		msg.setCode(CMD_REQUEST_COMPLETED);
		msg.setField(VID_RCC, RCC_SUCCESS);
      sendMessage(&msg);
	}
}

/**
 * Query objects
 */
void ClientSession::queryObjects(NXCPMessage *request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request->getId());

   TCHAR *query = request->getFieldAsString(VID_QUERY);
   TCHAR errorMessage[1024];
   ObjectArray<ObjectQueryResult> *objects = QueryObjects(query, m_dwUserId, errorMessage, 1024);
   if (objects != nullptr)
   {
      UINT32 *idList = new UINT32[objects->size()];
      for(int i = 0; i < objects->size(); i++)
      {
         idList[i] = objects->get(i)->object->getId();
      }
      msg.setFieldFromInt32Array(VID_OBJECT_LIST, objects->size(), idList);
      delete[] idList;
      delete objects;
   }
   else
   {
      msg.setField(VID_RCC, RCC_NXSL_EXECUTION_ERROR);
      msg.setField(VID_ERROR_TEXT, errorMessage);
   }
   MemFree(query);

   sendMessage(&msg);
}

/**
 * Query object details
 */
void ClientSession::queryObjectDetails(NXCPMessage *request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request->getId());

   TCHAR *query = request->getFieldAsString(VID_QUERY);
   StringList fields(request, VID_FIELD_LIST_BASE, VID_FIELDS);
   StringList orderBy(request, VID_ORDER_FIELD_LIST_BASE, VID_ORDER_FIELDS);
   TCHAR errorMessage[1024];
   ObjectArray<ObjectQueryResult> *objects = QueryObjects(query, m_dwUserId, errorMessage, 1024,
            &fields, &orderBy, request->getFieldAsUInt32(VID_RECORD_LIMIT));
   if (objects != nullptr)
   {
      UINT32 *idList = new UINT32[objects->size()];
      UINT32 fieldId = VID_ELEMENT_LIST_BASE;
      for(int i = 0; i < objects->size(); i++)
      {
         ObjectQueryResult *curr = objects->get(i);
         idList[i] = curr->object->getId();
         curr->values->fillMessage(&msg, fieldId + 1, fieldId);
         fieldId += curr->values->size() + 1;
      }
      msg.setFieldFromInt32Array(VID_OBJECT_LIST, objects->size(), idList);
      delete[] idList;
      delete objects;
   }
   else
   {
      msg.setField(VID_RCC, RCC_NXSL_EXECUTION_ERROR);
      msg.setField(VID_ERROR_TEXT, errorMessage);
   }
   MemFree(query);

   sendMessage(&msg);
}

/**
 * Send all configuration variables to client
 */
void ClientSession::getConfigurationVariables(UINT32 dwRqId)
{
   UINT32 i, dwId, dwNumRecords;
   NXCPMessage msg;
   TCHAR szBuffer[MAX_CONFIG_VALUE];

   // Prepare message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(dwRqId);

   // Check user rights
   if ((m_dwUserId == 0) || (m_systemAccessRights & SYSTEM_ACCESS_SERVER_CONFIG))
   {
      DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

      // Retrieve configuration variables from database
      DB_RESULT hResult = DBSelect(hdb, _T("SELECT var_name,var_value,need_server_restart,data_type,description,default_value,units FROM config WHERE is_visible=1"));
      if (hResult != nullptr)
      {
         // Send events, one per message
         dwNumRecords = DBGetNumRows(hResult);
         msg.setField(VID_NUM_VARIABLES, dwNumRecords);
         for(i = 0, dwId = VID_VARLIST_BASE; i < dwNumRecords; i++, dwId += 10)
         {
            msg.setField(dwId, DBGetField(hResult, i, 0, szBuffer, MAX_DB_STRING));
            msg.setField(dwId + 1, DBGetField(hResult, i, 1, szBuffer, MAX_CONFIG_VALUE));
            msg.setField(dwId + 2, (WORD)DBGetFieldLong(hResult, i, 2));
            msg.setField(dwId + 3, DBGetField(hResult, i, 3, szBuffer, MAX_CONFIG_VALUE));
            msg.setField(dwId + 4, DBGetField(hResult, i, 4, szBuffer, MAX_CONFIG_VALUE));
            msg.setField(dwId + 5, DBGetField(hResult, i, 5, szBuffer, MAX_CONFIG_VALUE));
            msg.setField(dwId + 6, DBGetField(hResult, i, 6, szBuffer, MAX_CONFIG_VALUE));
         }
         DBFreeResult(hResult);

         hResult = DBSelect(hdb, _T("SELECT var_name,var_value,var_description FROM config_values"));
         if (hResult != nullptr)
         {
            dwNumRecords = DBGetNumRows(hResult);
            msg.setField(VID_NUM_VALUES, dwNumRecords);
            for(i = 0; i < dwNumRecords; i++)
            {
               msg.setField(dwId++, DBGetField(hResult, i, 0, szBuffer, MAX_DB_STRING));
               msg.setField(dwId++, DBGetField(hResult, i, 1, szBuffer, MAX_CONFIG_VALUE));
               msg.setField(dwId++, DBGetField(hResult, i, 2, szBuffer, MAX_DB_STRING));
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
 * Get public configuration variable by name
 */
void ClientSession::getPublicConfigurationVariable(NXCPMessage *request)
{
   NXCPMessage msg;
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(request->getId());

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT var_value FROM config WHERE var_name=? AND is_public='Y'"));
   if (hStmt != nullptr)
   {
      TCHAR name[64];
      request->getFieldAsString(VID_NAME, name, 64);
      DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, name, DB_BIND_STATIC);

      DB_RESULT hResult = DBSelectPrepared(hStmt);
      if (hResult != nullptr)
      {
         if (DBGetNumRows(hResult) > 0)
         {
            TCHAR value[MAX_CONFIG_VALUE];
            msg.setField(VID_VALUE, DBGetField(hResult, 0, 0, value, MAX_CONFIG_VALUE));
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

   DBConnectionPoolReleaseConnection(hdb);

   sendMessage(&msg);
}

/**
 * Set configuration variable's value
 */
void ClientSession::setConfigurationVariable(NXCPMessage *pRequest)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, pRequest->getId());

   TCHAR name[MAX_OBJECT_NAME];
   pRequest->getFieldAsString(VID_NAME, name, MAX_OBJECT_NAME);

   if ((m_dwUserId == 0) || (m_systemAccessRights & SYSTEM_ACCESS_SERVER_CONFIG))
   {
      TCHAR oldValue[MAX_CONFIG_VALUE], newValue[MAX_CONFIG_VALUE];
      pRequest->getFieldAsString(VID_VALUE, newValue, MAX_CONFIG_VALUE);
      ConfigReadStr(name, oldValue, MAX_CONFIG_VALUE, _T(""));
      if (ConfigWriteStr(name, newValue, true))
      {
         msg.setField(VID_RCC, RCC_SUCCESS);
         writeAuditLogWithValues(AUDIT_SYSCFG, true, 0, oldValue, newValue, 'T',
                                 _T("Server configuration variable \"%s\" changed from \"%s\" to \"%s\""), name, oldValue, newValue);
      }
      else
      {
         msg.setField(VID_RCC, RCC_DB_FAILURE);
      }
   }
   else
   {
      writeAuditLog(AUDIT_SYSCFG, false, 0, _T("Access denied on setting server configuration variable \"%s\""), name);
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Set configuration variable's values to default
 */
void ClientSession::setDefaultConfigurationVariableValues(NXCPMessage *pRequest)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, pRequest->getId());
   UINT32 rcc = RCC_SUCCESS;

   if (checkSysAccessRights(SYSTEM_ACCESS_SERVER_CONFIG))
   {
      DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
      DB_STATEMENT stmt = DBPrepare(hdb, _T("SELECT default_value FROM config WHERE var_name=?"));
      if (stmt != nullptr)
      {
         int numVars = pRequest->getFieldAsInt32(VID_NUM_VARIABLES);
         UINT32 fieldId = VID_VARLIST_BASE;
         TCHAR varName[64], defValue[MAX_CONFIG_VALUE];
         for(int i = 0; i < numVars; i++)
         {
            pRequest->getFieldAsString(fieldId++, varName, 64);
            DBBind(stmt, 1, DB_SQLTYPE_VARCHAR, varName, DB_BIND_STATIC);

            DB_RESULT result = DBSelectPrepared(stmt);
            if (result != nullptr)
            {
               DBGetField(result, 0, 0, defValue, MAX_CONFIG_VALUE);
               ConfigWriteStr(varName, defValue, false);
            }
            else
            {
               rcc = RCC_DB_FAILURE;
               break;
            }
         }
         DBFreeStatement(stmt);
      }
      else
      {
         rcc = RCC_DB_FAILURE;
      }
      DBConnectionPoolReleaseConnection(hdb);
   }
   else
   {
      writeAuditLog(AUDIT_SYSCFG, false, 0, _T("Access denied for setting server configuration variables to default"));
      rcc = RCC_ACCESS_DENIED;
   }
   // Send response
   msg.setField(VID_RCC, rcc);
   sendMessage(&msg);

}

/**
 * Delete configuration variable
 */
void ClientSession::deleteConfigurationVariable(NXCPMessage *pRequest)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, pRequest->getId());

   TCHAR name[MAX_OBJECT_NAME];
   pRequest->getFieldAsString(VID_NAME, name, MAX_OBJECT_NAME);

   if ((m_dwUserId == 0) || (m_systemAccessRights & SYSTEM_ACCESS_SERVER_CONFIG))
   {
      if (ConfigDelete(name))
		{
         msg.setField(VID_RCC, RCC_SUCCESS);
			writeAuditLog(AUDIT_SYSCFG, true, 0, _T("Server configuration variable \"%s\" deleted"), name);
		}
      else
		{
         msg.setField(VID_RCC, RCC_DB_FAILURE);
		}
   }
   else
   {
      writeAuditLog(AUDIT_SYSCFG, false, 0, _T("Access denied on delete server configuration variable \"%s\""), name);
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
   NXCPMessage msg(CMD_REQUEST_COMPLETED, pRequest->getId());

   TCHAR name[MAX_OBJECT_NAME];
   pRequest->getFieldAsString(VID_NAME, name, MAX_OBJECT_NAME);
	if (m_systemAccessRights & SYSTEM_ACCESS_SERVER_CONFIG)
	{
		TCHAR *newValue = pRequest->getFieldAsString(VID_VALUE);
		if (newValue != nullptr)
		{
		   TCHAR *oldValue = ConfigReadCLOB(name, _T(""));
			if (ConfigWriteCLOB(name, newValue, TRUE))
			{
				msg.setField(VID_RCC, RCC_SUCCESS);
				writeAuditLogWithValues(AUDIT_SYSCFG, true, 0, oldValue, newValue, 'T',
								            _T("Server configuration variable (long) \"%s\" changed"), name);
			}
			else
			{
				msg.setField(VID_RCC, RCC_DB_FAILURE);
			}
			free(oldValue);
			free(newValue);
		}
		else
		{
			msg.setField(VID_RCC, RCC_INVALID_REQUEST);
		}
	}
	else
	{
	   writeAuditLog(AUDIT_SYSCFG, false, 0, _T("Access denied on setting server configuration variable \"%s\""), name);
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

	if (m_systemAccessRights & SYSTEM_ACCESS_SERVER_CONFIG)
	{
      pRequest->getFieldAsString(VID_NAME, name, MAX_OBJECT_NAME);
		value = ConfigReadCLOB(name, nullptr);
		if (value != nullptr)
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
 * Force terminate session
 */
void ClientSession::terminate()
{
   notify(NX_NOTIFY_SESSION_KILLED);
   InterlockedOr(&m_flags, CSF_TERMINATE_REQUESTED);
   m_socketPoller->poller.cancel(m_socket);
}

/**
 * Handler for new events
 */
void ClientSession::onNewEvent(Event *pEvent)
{
   if (isAuthenticated() && isSubscribedTo(NXC_CHANNEL_EVENTS) && (m_systemAccessRights & SYSTEM_ACCESS_VIEW_EVENT_LOG))
   {
      shared_ptr<NetObj> object = FindObjectById(pEvent->getSourceId());
      // If can't find object - just send to all events, if object found send to thous who have rights
      if ((object == nullptr) || object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
      {
         NXCPMessage msg(CMD_EVENTLOG_RECORDS, 0);
         pEvent->prepareMessage(&msg);
         postMessage(&msg);
      }
   }
}

/**
 * Send object update (executed in thread pool)
 */
void ClientSession::sendObjectUpdate(shared_ptr<NetObj> object)
{
   StringBuffer key(_T("ObjectUpdate"));
   key.append(m_id);
   UINT32 waitTime = ThreadPoolGetSerializedRequestMaxWaitTime(g_clientThreadPool, key);

   MutexLock(m_pendingObjectNotificationsLock);
   m_pendingObjectNotifications->remove(object->getId());

   if ((waitTime > m_objectNotificationDelay * 2) && (m_objectNotificationDelay < 1600))
   {
      m_objectNotificationDelay *= 2;
   }

   if ((waitTime < m_objectNotificationDelay / 2) && (m_objectNotificationDelay > 200))
   {
      m_objectNotificationDelay /= 2;
   }

   MutexUnlock(m_pendingObjectNotificationsLock);
   debugPrintf(5, _T("Sending update for object %s [%d]"), object->getName(), object->getId());

   NXCPMessage msg(CMD_OBJECT_UPDATE, 0);
   if (!object->isDeleted())
   {
      object->fillMessage(&msg, m_dwUserId);
      if (m_flags & CSF_SYNC_OBJECT_COMMENTS)
         object->commentsToMessage(&msg);
      if ((object->getObjectClass() == OBJECT_NODE) && !object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_MODIFY))
      {
         // mask passwords
         msg.setField(VID_SHARED_SECRET, _T("********"));
         msg.setField(VID_SNMP_AUTH_PASSWORD, _T("********"));
         msg.setField(VID_SNMP_PRIV_PASSWORD, _T("********"));
      }
   }
   else
   {
      msg.setField(VID_OBJECT_ID, object->getId());
      msg.setField(VID_IS_DELETED, (UINT16)1);
   }
   sendMessage(&msg);
   decRefCount();
}

/**
 * Schedule object update (executed in thread pool relative schedule)
 */
void ClientSession::scheduleObjectUpdate(shared_ptr<NetObj> object)
{
   TCHAR key[64];
   _sntprintf(key, 64, _T("ObjectUpdate_%d"), m_id);
   if (ThreadPoolGetSerializedRequestCount(g_clientThreadPool, key) < 500)
   {
      debugPrintf(5, _T("Scheduling update for object %s [%d]"), object->getName(), object->getId());
      ThreadPoolExecuteSerialized(g_clientThreadPool, key, this, &ClientSession::sendObjectUpdate, object);
   }
   else
   {
      debugPrintf(5, _T("Drop update for object %s [%d]"), object->getName(), object->getId());
      MutexLock(m_pendingObjectNotificationsLock);
      m_pendingObjectNotifications->remove(object->getId());
      MutexUnlock(m_pendingObjectNotificationsLock);
      if ((m_flags & CSF_OBJECTS_OUT_OF_SYNC) == 0)
      {
         InterlockedOr(&m_flags, CSF_OBJECTS_OUT_OF_SYNC);
         notify(NX_NOTIFY_OBJECTS_OUT_OF_SYNC);
      }
      decRefCount();
   }
}

/**
 * Handler for object changes
 */
void ClientSession::onObjectChange(const shared_ptr<NetObj>& object)
{
   if (((m_flags & CSF_OBJECT_SYNC_FINISHED) > 0) && isAuthenticated() &&
         isSubscribedTo(NXC_CHANNEL_OBJECTS) &&
      (object->isDeleted() || object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ)))
   {
      MutexLock(m_pendingObjectNotificationsLock);
      if (!m_pendingObjectNotifications->contains(object->getId()))
      {
         m_pendingObjectNotifications->put(object->getId());
         incRefCount();
         ThreadPoolScheduleRelative(g_clientThreadPool, m_objectNotificationDelay, this, &ClientSession::scheduleObjectUpdate, object);
      }
      MutexUnlock(m_pendingObjectNotificationsLock);
   }
}

/**
 * Send notification message to server
 */
void ClientSession::notify(uint32_t code, uint32_t data)
{
   NXCPMessage msg(CMD_NOTIFY, 0);
   msg.setField(VID_NOTIFICATION_CODE, code);
   msg.setField(VID_NOTIFICATION_DATA, data);
   postMessage(&msg);
}

/**
 * Set information about conflicting nodes to VID_VALUE field
 */
static void SetNodesConflictString(NXCPMessage *msg, int32_t zoneUIN, const InetAddress& ipAddr)
{
   if (!ipAddr.isValid())
      return;

   shared_ptr<Node> sameNode = FindNodeByIP(zoneUIN, ipAddr);
   shared_ptr<Subnet> sameSubnet = FindSubnetByIP(zoneUIN, ipAddr);
   if (sameNode != nullptr)
   {
      msg->setField(VID_VALUE, sameNode->getName());
   }
   else if (sameSubnet != nullptr)
   {
      msg->setField(VID_VALUE, sameSubnet->getName());
   }
   else
   {
      msg->setField(VID_VALUE, _T(""));
   }
}

/**
 * Modify object
 */
void ClientSession::modifyObject(NXCPMessage *pRequest)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, pRequest->getId());

   uint32_t rcc = RCC_SUCCESS;
   uint32_t dwObjectId = pRequest->getFieldAsUInt32(VID_OBJECT_ID);
   shared_ptr<NetObj> object = FindObjectById(dwObjectId);
   if (object != nullptr)
   {
      if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_MODIFY))
      {
         json_t *oldValue = object->toJson();

         // If user attempts to change object's ACL, check
         // if he has OBJECT_ACCESS_ACL permission
         if (pRequest->isFieldExist(VID_ACL_SIZE))
            if (!object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_ACL))
               rcc = RCC_ACCESS_DENIED;

         // If allowed, change object and set completion code
         if (rcc == RCC_SUCCESS)
			{
            rcc = object->modifyFromMessage(pRequest);
	         if (rcc == RCC_SUCCESS)
				{
					object->postModify();
				}
				else if (rcc == RCC_ALREADY_EXIST)
				{
               // Add information about conflicting nodes
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
               SetNodesConflictString(&msg, static_cast<Node&>(*object).getZoneUIN(), ipAddr);
            }
			}
         msg.setField(VID_RCC, rcc);

			if (rcc == RCC_SUCCESS)
			{
			   json_t *newValue = object->toJson();
			   writeAuditLogWithValues(AUDIT_OBJECTS, true, dwObjectId, oldValue, newValue, _T("Object %s modified from client"), object->getName());
	         json_decref(newValue);
			}
			json_decref(oldValue);
      }
      else
      {
         msg.setField(VID_RCC, RCC_ACCESS_DENIED);
			writeAuditLog(AUDIT_OBJECTS, false, dwObjectId, _T("Access denied on object modification"));
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
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(dwRqId);
   msg.setField(VID_RCC, RCC_SUCCESS);
   sendMessage(&msg);
	msg.deleteAllFields();

   // Send user database
	Iterator<UserDatabaseObject> *users = OpenUserDatabase();
	while(users->hasNext())
   {
	   UserDatabaseObject *object = users->next();
		msg.setCode(object->isGroup() ? CMD_GROUP_DATA : CMD_USER_DATA);
		object->fillMessage(&msg);
      sendMessage(&msg);
      msg.deleteAllFields();
   }
	CloseUserDatabase(users);

   // Send end-of-database notification
   msg.setCode(CMD_USER_DB_EOF);
   sendMessage(&msg);
}

/**
 * Get users from given set
 */
void ClientSession::getSelectedUsers(NXCPMessage *request)
{
   NXCPMessage msg;
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(request->getId());
   msg.setField(VID_RCC, RCC_SUCCESS);
   sendMessage(&msg);
   msg.deleteAllFields();

   IntegerArray<UINT32> userIds;
   request->getFieldAsInt32Array(VID_OBJECT_LIST, &userIds);

   // Prepare message
   msg.setCode(CMD_USER_DATA);

   // Send user database
   ObjectArray<UserDatabaseObject> *users = FindUserDBObjects(&userIds);
   for (int i = 0; i < users->size(); i++)
   {
      UserDatabaseObject *object = users->get(i);
      msg.setCode(object->isGroup() ? CMD_GROUP_DATA : CMD_USER_DATA);
      object->fillMessage(&msg);
      sendMessage(&msg);
      msg.deleteAllFields();
   }
   delete users;

   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setField(VID_RCC, RCC_SUCCESS);
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
   if (!(m_systemAccessRights & SYSTEM_ACCESS_MANAGE_USERS))
   {
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);
   }
   else if (!(m_flags & CSF_USER_DB_LOCKED))
   {
      // User database have to be locked before any
      // changes to user database can be made
      msg.setField(VID_RCC, RCC_OUT_OF_STATE_REQUEST);
   }
   else
   {
      UINT32 rcc, dwUserId;
      TCHAR szUserName[MAX_USER_NAME];

      pRequest->getFieldAsString(VID_USER_NAME, szUserName, MAX_USER_NAME);
      if (IsValidObjectName(szUserName))
      {
         bool isGroup = pRequest->getFieldAsBoolean(VID_IS_GROUP);
         rcc = CreateNewUser(szUserName, isGroup, &dwUserId);
         msg.setField(VID_RCC, rcc);
         if (rcc == RCC_SUCCESS)
         {
            msg.setField(VID_USER_ID, dwUserId);   // Send id of new user to client
            writeAuditLog(AUDIT_SECURITY, true, 0, _T("%s %s created"), isGroup ? _T("Group") : _T("User"), szUserName);
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
   if (!(m_systemAccessRights & SYSTEM_ACCESS_MANAGE_USERS))
   {
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);
   }
   else if (!(m_flags & CSF_USER_DB_LOCKED))
   {
      // User database have to be locked before any
      // changes to user database can be made
      msg.setField(VID_RCC, RCC_OUT_OF_STATE_REQUEST);
   }
   else
   {
      json_t *oldData = nullptr, *newData = nullptr;
      UINT32 result = ModifyUserDatabaseObject(pRequest, &oldData, &newData);
      if (result == RCC_SUCCESS)
      {
         TCHAR name[MAX_USER_NAME];
         UINT32 id = pRequest->getFieldAsUInt32(VID_USER_ID);
         writeAuditLogWithValues(AUDIT_SECURITY, true, 0, oldData, newData,
            _T("%s %s modified"), (id & GROUP_FLAG) ? _T("Group") : _T("User"), ResolveUserId(id, name, true));
      }
      msg.setField(VID_RCC, result);
      json_decref(oldData);
      json_decref(newData);
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
   if (!(m_systemAccessRights & SYSTEM_ACCESS_MANAGE_USERS))
   {
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);
   }
   else if (!(m_flags & CSF_USER_DB_LOCKED))
   {
      // User database have to be locked before any
      // changes to user database can be made
      msg.setField(VID_RCC, RCC_OUT_OF_STATE_REQUEST);
   }
   else
   {
      uint32_t rcc = DetachLDAPUser(id);
      if (rcc == RCC_SUCCESS)
      {
         TCHAR name[MAX_DB_STRING];
         writeAuditLog(AUDIT_SECURITY, true, 0,
            _T("%s %s detached from LDAP account"), (id & GROUP_FLAG) ? _T("Group") : _T("User"), ResolveUserId(id, name, true));
      }
      msg.setField(VID_RCC, rcc);
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
   if (!(m_systemAccessRights & SYSTEM_ACCESS_MANAGE_USERS))
   {
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);

      dwUserId = pRequest->getFieldAsUInt32(VID_USER_ID);
      TCHAR name[MAX_USER_NAME];
      ResolveUserId(dwUserId, name, true);
      writeAuditLog(AUDIT_SECURITY, false, 0, _T("Access denied on delete %s %s [%d]"), (dwUserId & GROUP_FLAG) ? _T("group") : _T("user"), name, dwUserId);
   }
   else if (!(m_flags & CSF_USER_DB_LOCKED))
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
            TCHAR name[MAX_USER_NAME];
            ResolveUserId(dwUserId, name, true);
            UINT32 rcc =  DeleteUserDatabaseObject(dwUserId);
            msg.setField(VID_RCC, rcc);
            if (rcc == RCC_SUCCESS)
            {
               writeAuditLog(AUDIT_SECURITY, true, 0, _T("%s %s [%d] deleted"), (dwUserId & GROUP_FLAG) ? _T("Group") : _T("User"), name, dwUserId);
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

   if (m_systemAccessRights & SYSTEM_ACCESS_MANAGE_USERS)
   {
      if (bLock)
      {
         if (!LockComponent(CID_USER_DB, m_id, m_sessionName, nullptr, szBuffer))
         {
            msg.setField(VID_RCC, RCC_COMPONENT_LOCKED);
            msg.setField(VID_LOCKED_BY, szBuffer);
         }
         else
         {
            InterlockedOr(&m_flags, CSF_USER_DB_LOCKED);
            msg.setField(VID_RCC, RCC_SUCCESS);
         }
      }
      else
      {
         if (m_flags & CSF_USER_DB_LOCKED)
         {
            UnlockComponent(CID_USER_DB);
            InterlockedAnd(&m_flags, ~CSF_USER_DB_LOCKED);
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
 * Change management status for the object
 */
void ClientSession::changeObjectMgmtStatus(NXCPMessage *pRequest)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, pRequest->getId());

   // Get object id and check access rights
   uint32_t dwObjectId = pRequest->getFieldAsUInt32(VID_OBJECT_ID);
   shared_ptr<NetObj> object = FindObjectById(dwObjectId);
   if (object != nullptr)
   {
      if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_MODIFY))
      {
			if ((object->getObjectClass() != OBJECT_TEMPLATE) &&
				 (object->getObjectClass() != OBJECT_TEMPLATEGROUP) &&
				 (object->getObjectClass() != OBJECT_TEMPLATEROOT))
			{
				bool bIsManaged = pRequest->getFieldAsBoolean(VID_MGMT_STATUS);
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
 * Enter maintenance mode for object
 */
void ClientSession::enterMaintenanceMode(NXCPMessage *request)
{
   NXCPMessage msg;
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(request->getId());

   // Get object id and check access rights
   shared_ptr<NetObj> object = FindObjectById(request->getFieldAsUInt32(VID_OBJECT_ID));
   if (object != nullptr)
   {
      if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_MAINTENANCE))
      {
         if ((object->getObjectClass() == OBJECT_CONTAINER) ||
             (object->getObjectClass() == OBJECT_CLUSTER) ||
             (object->getObjectClass() == OBJECT_NODE) ||
             (object->getObjectClass() == OBJECT_MOBILEDEVICE) ||
             (object->getObjectClass() == OBJECT_ACCESSPOINT) ||
             (object->getObjectClass() == OBJECT_CHASSIS) ||
             (object->getObjectClass() == OBJECT_ZONE) ||
             (object->getObjectClass() == OBJECT_SUBNET) ||
             (object->getObjectClass() == OBJECT_NETWORK) ||
             (object->getObjectClass() == OBJECT_SERVICEROOT)||
             (object->getObjectClass() == OBJECT_SENSOR))
         {
            TCHAR *comments = request->getFieldAsString(VID_COMMENTS);
            object->enterMaintenanceMode(m_dwUserId, comments);
            MemFree(comments);
            msg.setField(VID_RCC, RCC_SUCCESS);
            WriteAuditLog(AUDIT_OBJECTS, TRUE, m_dwUserId, m_workstation, m_id, object->getId(),
               _T("Requested maintenance mode enter for object %s [%d]"), object->getName(), object->getId());
         }
         else
         {
            msg.setField(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
         }
      }
      else
      {
         msg.setField(VID_RCC, RCC_ACCESS_DENIED);
         WriteAuditLog(AUDIT_OBJECTS, FALSE, m_dwUserId, m_workstation, m_id, object->getId(),
            _T("Access denied on maintenance mode enter request for object %s [%d]"), object->getName(), object->getId());
      }
   }
   else
   {
      msg.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   sendMessage(&msg);
}

/**
 * Leave maintenance mode for object
 */
void ClientSession::leaveMaintenanceMode(NXCPMessage *request)
{
   NXCPMessage msg;
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(request->getId());

   // Get object id and check access rights
   shared_ptr<NetObj> object = FindObjectById(request->getFieldAsUInt32(VID_OBJECT_ID));
   if (object != nullptr)
   {
      if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_MAINTENANCE))
      {
         if ((object->getObjectClass() == OBJECT_CONTAINER) ||
             (object->getObjectClass() == OBJECT_CLUSTER) ||
             (object->getObjectClass() == OBJECT_NODE) ||
             (object->getObjectClass() == OBJECT_MOBILEDEVICE) ||
             (object->getObjectClass() == OBJECT_ACCESSPOINT) ||
             (object->getObjectClass() == OBJECT_CHASSIS) ||
             (object->getObjectClass() == OBJECT_ZONE) ||
             (object->getObjectClass() == OBJECT_SUBNET) ||
             (object->getObjectClass() == OBJECT_NETWORK) ||
             (object->getObjectClass() == OBJECT_SERVICEROOT) ||
             (object->getObjectClass() == OBJECT_SENSOR))
         {
            object->leaveMaintenanceMode(m_dwUserId);
            msg.setField(VID_RCC, RCC_SUCCESS);
            WriteAuditLog(AUDIT_OBJECTS, TRUE, m_dwUserId, m_workstation, m_id, object->getId(),
               _T("Requested maintenance mode exit for object %s [%d]"), object->getName(), object->getId());
         }
         else
         {
            msg.setField(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
         }
      }
      else
      {
         msg.setField(VID_RCC, RCC_ACCESS_DENIED);
         WriteAuditLog(AUDIT_OBJECTS, FALSE, m_dwUserId, m_workstation, m_id, object->getId(),
            _T("Access denied on maintenance mode exit request for object %s [%d]"), object->getName(), object->getId());
      }
   }
   else
   {
      msg.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

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
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(request->getId());

   UINT32 userId = request->getFieldAsUInt32(VID_USER_ID);
   if ((m_systemAccessRights & SYSTEM_ACCESS_MANAGE_USERS) || (userId == m_dwUserId))     // User can change password for itself
   {
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

      UINT32 rcc = SetUserPassword(userId, newPassword, oldPassword, userId == m_dwUserId);
      msg.setField(VID_RCC, rcc);

      if (rcc == RCC_SUCCESS)
      {
         TCHAR userName[MAX_USER_NAME];
         WriteAuditLog(AUDIT_SECURITY, TRUE, m_dwUserId, m_workstation, m_id, 0,
                  _T("Changed password for user %s"), ResolveUserId(userId, userName, true));
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
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request->getId());
   bool success = false;

   // Get node id and check object class and access rights
   uint32_t objectId = request->getFieldAsUInt32(VID_OBJECT_ID);
   shared_ptr<NetObj> object = FindObjectById(objectId);
   if (object != nullptr)
   {
      if (object->isDataCollectionTarget() || (object->getObjectClass() == OBJECT_TEMPLATE))
      {
         if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
         {
            success = true;
            msg.setField(VID_RCC, RCC_SUCCESS);
            if (!request->getFieldAsBoolean(VID_IS_REFRESH))
            {
               m_openDataCollectionConfigurations.put(objectId);
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
   if (success)
      static_cast<DataCollectionOwner&>(*object).sendItemsToClient(this, request->getId());
}

/**
 * Unlock node's data collection settings
 */
void ClientSession::closeNodeDCIList(NXCPMessage *request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request->getId());

   // Get node id and check object class and access rights
   uint32_t objectId = request->getFieldAsUInt32(VID_OBJECT_ID);
   shared_ptr<NetObj> object = FindObjectById(objectId);
   if (object != nullptr)
   {
      if (object->isDataCollectionTarget() || (object->getObjectClass() == OBJECT_TEMPLATE))
      {
         if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
         {
            static_cast<DataCollectionOwner&>(*object).applyDCIChanges(false);
            m_openDataCollectionConfigurations.remove(objectId);
            msg.setField(VID_RCC, RCC_SUCCESS);
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
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request->getId());

   // Get node id and check object class and access rights
   uint32_t dwObjectId = request->getFieldAsUInt32(VID_OBJECT_ID);
   shared_ptr<NetObj> object = FindObjectById(dwObjectId);
   if (object != nullptr)
   {
      if (object->isDataCollectionTarget() || (object->getObjectClass() == OBJECT_TEMPLATE))
      {
         if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_MODIFY))
         {
            bool success = false;

            json_t *oldValue = object->toJson();

            uint32_t itemId;
            int dcObjectType = request->getFieldAsInt16(VID_DCOBJECT_TYPE);
            switch(request->getCode())
            {
               case CMD_MODIFY_NODE_DCI:
                  itemId = request->getFieldAsUInt32(VID_DCI_ID);
                  if (itemId == 0) // create new if id is 0
                  {
                     int dcObjectType = (int)request->getFieldAsUInt16(VID_DCOBJECT_TYPE);
                     // Create dummy DCI
                     DCObject *dcObject;
                     switch(dcObjectType)
                     {
                        case DCO_TYPE_ITEM:
                           dcObject = new DCItem(CreateUniqueId(IDG_ITEM), _T("no name"), DS_INTERNAL, DCI_DT_INT,
                                    nullptr, nullptr, static_pointer_cast<DataCollectionOwner>(object));
                           break;
                        case DCO_TYPE_TABLE:
                           dcObject = new DCTable(CreateUniqueId(IDG_ITEM), _T("no name"), DS_INTERNAL,
                                    nullptr, nullptr, static_pointer_cast<DataCollectionOwner>(object));
                           break;
                        default:
                           dcObject = nullptr;
                           break;
                     }
                     if (dcObject != nullptr)
                     {
                        dcObject->setStatus(ITEM_STATUS_DISABLED, false);
                        if (static_cast<DataCollectionOwner&>(*object).addDCObject(dcObject, false, false))
                        {
                           itemId = dcObject->getId();
                           msg.setField(VID_RCC, RCC_SUCCESS);
                           success = true;
                           // Return new item id to client
                           msg.setField(VID_DCI_ID, dcObject->getId());
                           request->setField(VID_DCI_ID, dcObject->getId());
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
                  }
                  else
                  {
                     success = true;
                  }

                  // Update existing
                  if (success)
                  {
                     uint32_t mapCount, *mapId, *mapIndex;
                     success = static_cast<DataCollectionOwner&>(*object).updateDCObject(itemId, request, &mapCount, &mapIndex, &mapId, m_dwUserId);
                     if (success)
                     {
                        msg.setField(VID_RCC, RCC_SUCCESS);
                        shared_ptr<DCObject> dco = static_cast<DataCollectionOwner&>(*object).getDCObjectById(itemId, 0, true);
                        if (dco != nullptr)
                           NotifyClientsOnDCIUpdate(static_cast<DataCollectionOwner&>(*object), dco.get());

                        // Send index to id mapping for newly created thresholds to client
                        if (dcObjectType == DCO_TYPE_ITEM)
                        {
                           msg.setField(VID_DCI_NUM_MAPS, mapCount);
                           for(uint32_t i = 0; i < mapCount; i++)
                           {
                              mapId[i] = htonl(mapId[i]);
                              mapIndex[i] = htonl(mapIndex[i]);
                           }
                           msg.setField(VID_DCI_MAP_IDS, reinterpret_cast<BYTE*>(mapId), sizeof(uint32_t) * mapCount);
                           msg.setField(VID_DCI_MAP_INDEXES, reinterpret_cast<BYTE*>(mapIndex), sizeof(uint32_t) * mapCount);
                           MemFree(mapId);
                           MemFree(mapIndex);
                        }
                     }
                     else
                     {
                        msg.setField(VID_RCC, RCC_INVALID_DCI_ID);
                     }
                  }
                  break;
               case CMD_DELETE_NODE_DCI:
                  itemId = request->getFieldAsUInt32(VID_DCI_ID);
                  success = static_cast<DataCollectionOwner&>(*object).deleteDCObject(itemId, true, m_dwUserId);
                  msg.setField(VID_RCC, success ? RCC_SUCCESS : RCC_INVALID_DCI_ID);
                  break;
            }
            if (success)
            {
               if (m_openDataCollectionConfigurations.contains(dwObjectId))
                  static_cast<DataCollectionOwner&>(*object).setDCIModificationFlag();
               else
                  static_cast<DataCollectionOwner&>(*object).applyDCIChanges(true);
               json_t *newValue = object->toJson();
               writeAuditLogWithValues(AUDIT_OBJECTS, true, dwObjectId, oldValue, newValue, _T("Data collection configuration changed for object %s"), object->getName());
               json_decref(newValue);
            }
            json_decref(oldValue);
         }
         else  // User doesn't have MODIFY rights on object
         {
            msg.setField(VID_RCC, RCC_ACCESS_DENIED);
            writeAuditLog(AUDIT_OBJECTS, false, dwObjectId, _T("Access denied on changing data collection configuration for object %s"), object->getName());
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
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request->getId());

   // Get node id and check object class and access rights
   shared_ptr<NetObj> object = FindObjectById(request->getFieldAsUInt32(VID_OBJECT_ID));
   if (object != nullptr)
   {
      if (object->isDataCollectionTarget() || (object->getObjectClass() == OBJECT_TEMPLATE))
      {
         if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_MODIFY))
         {
            UINT32 dwNumItems, *pdwItemList;
            int iStatus;

            iStatus = request->getFieldAsUInt16(VID_DCI_STATUS);
            dwNumItems = request->getFieldAsUInt32(VID_NUM_ITEMS);
            pdwItemList = MemAllocArray<UINT32>(dwNumItems);
            request->getFieldAsInt32Array(VID_ITEM_LIST, dwNumItems, pdwItemList);
            if (static_cast<DataCollectionOwner&>(*object).setItemStatus(dwNumItems, pdwItemList, iStatus))
               msg.setField(VID_RCC, RCC_SUCCESS);
            else
               msg.setField(VID_RCC, RCC_INVALID_DCI_ID);
            MemFree(pdwItemList);
         }
         else  // User doesn't have MODIFY rights on object
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
 * Recalculate values for DCI
 */
void ClientSession::recalculateDCIValues(NXCPMessage *request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request->getId());

   shared_ptr<NetObj> object = FindObjectById(request->getFieldAsUInt32(VID_OBJECT_ID));
   if (object != nullptr)
   {
      if (object->isDataCollectionTarget())
      {
         if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_MODIFY))
         {
            UINT32 dciId = request->getFieldAsUInt32(VID_DCI_ID);
            debugPrintf(4, _T("recalculateDCIValues: request for DCI %d at target %s [%d]"), dciId, object->getName(), object->getId());
            shared_ptr<DCObject> dci = static_cast<DataCollectionTarget&>(*object).getDCObjectById(dciId, m_dwUserId);
            if (dci != nullptr)
            {
               if (dci->getType() == DCO_TYPE_ITEM)
               {
                  debugPrintf(4, _T("recalculateDCIValues: DCI \"%s\" [%d] at target %s [%d]"), dci->getDescription().cstr(), dciId, object->getName(), object->getId());
                  DCIRecalculationJob *job = new DCIRecalculationJob(static_pointer_cast<DataCollectionTarget>(object), static_cast<DCItem*>(dci.get()), m_dwUserId);
                  if (AddJob(job))
                  {
                     msg.setField(VID_RCC, RCC_SUCCESS);
                     msg.setField(VID_JOB_ID, job->getId());
                     writeAuditLog(AUDIT_OBJECTS, true, object->getId(), _T("Data recalculation for DCI \"%s\" [%d] on object \"%s\" [%d] started (job ID %d)"),
                              dci->getDescription().cstr(), dci->getId(), object->getName(), object->getId(), job->getId());
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
               msg.setField(VID_RCC, RCC_INVALID_DCI_ID);
               debugPrintf(4, _T("recalculateDCIValues: DCI %d at target %s [%d] not found"), dciId, object->getName(), object->getId());
            }
         }
         else  // User doesn't have MODIFY rights on object
         {
            msg.setField(VID_RCC, RCC_ACCESS_DENIED);
            writeAuditLog(AUDIT_OBJECTS, false, object->getId(), _T("Access denied on recalculating DCI data"));
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

   sendMessage(&msg);
}

/**
 * Clear all collected data for DCI
 */
void ClientSession::clearDCIData(NXCPMessage *request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request->getId());

   // Get node id and check object class and access rights
   shared_ptr<NetObj> object = FindObjectById(request->getFieldAsUInt32(VID_OBJECT_ID));
   if (object != nullptr)
   {
      if (object->isDataCollectionTarget())
      {
         if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_MODIFY))
         {
				UINT32 dciId = request->getFieldAsUInt32(VID_DCI_ID);
				debugPrintf(4, _T("ClearDCIData: request for DCI %d at node %d"), dciId, object->getId());
            shared_ptr<DCObject> dci = static_cast<DataCollectionOwner&>(*object).getDCObjectById(dciId, m_dwUserId);
				if (dci != nullptr)
				{
					msg.setField(VID_RCC, dci->deleteAllData() ? RCC_SUCCESS : RCC_DB_FAILURE);
					debugPrintf(4, _T("ClearDCIData: DCI %d at node %d"), dciId, object->getId());
	            writeAuditLog(AUDIT_OBJECTS, true, object->getId(), _T("Collected data for DCI \"%s\" [%d] on object \"%s\" [%d] cleared"),
	                     dci->getDescription().cstr(), dci->getId(), object->getName(), object->getId());
				}
				else
				{
					msg.setField(VID_RCC, RCC_INVALID_DCI_ID);
					debugPrintf(4, _T("ClearDCIData: DCI %d at node %d not found"), dciId, object->getId());
				}
         }
         else  // User doesn't have DELETE rights on object
         {
            writeAuditLog(AUDIT_OBJECTS, false, object->getId(), _T("Access denied on clear DCI data"));
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
 * Delete single entry from collected DCI data
 */
void ClientSession::deleteDCIEntry(NXCPMessage *request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request->getId());

   // Get node id and check object class and access rights
   shared_ptr<NetObj> object = FindObjectById(request->getFieldAsUInt32(VID_OBJECT_ID));
   if (object != nullptr)
   {
      if (object->isDataCollectionTarget())
      {
         if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_MODIFY))
         {
            UINT32 dciId = request->getFieldAsUInt32(VID_DCI_ID);
            debugPrintf(4, _T("DeleteDCIEntry: request for DCI %d at node %d"), dciId, object->getId());
            shared_ptr<DCObject> dci = static_cast<DataCollectionOwner&>(*object).getDCObjectById(dciId, m_dwUserId);
            if (dci != nullptr)
            {
               msg.setField(VID_RCC, dci->deleteEntry(request->getFieldAsUInt32(VID_TIMESTAMP)) ? RCC_SUCCESS : RCC_DB_FAILURE);
               debugPrintf(4, _T("DeleteDCIEntry: DCI %d at node %d"), dciId, object->getId());
               writeAuditLog(AUDIT_OBJECTS, true, object->getId(), _T("Collected data entry for DCI \"%s\" [%d] on object \"%s\" [%d] was deleted"),
                        dci->getDescription().cstr(), dci->getId(), object->getName(), object->getId());
            }
            else
            {
               msg.setField(VID_RCC, RCC_INVALID_DCI_ID);
               debugPrintf(4, _T("DeleteDCIEntry: DCI %d at node %d not found"), dciId, object->getId());
            }
         }
         else  // User doesn't have DELETE rights on object
         {
            writeAuditLog(AUDIT_OBJECTS, false, object->getId(), _T("Access denied on clear DCI data"));
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
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request->getId());

   // Get node id and check object class and access rights
   shared_ptr<NetObj> object = FindObjectById(request->getFieldAsUInt32(VID_OBJECT_ID));
   if (object != nullptr)
   {
      if (object->isDataCollectionTarget())
      {
         if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
         {
				uint32_t dciId = request->getFieldAsUInt32(VID_DCI_ID);
				debugPrintf(4, _T("ForceDCIPoll: request for DCI %d at node %d"), dciId, object->getId());
            shared_ptr<DCObject> dci = static_cast<DataCollectionOwner&>(*object).getDCObjectById(dciId, m_dwUserId);
				if (dci != nullptr)
				{
				   dci->requestForcePoll(this);
					msg.setField(VID_RCC, RCC_SUCCESS);
					debugPrintf(4, _T("ForceDCIPoll: DCI %d at node %d"), dciId, object->getId());
				}
				else
				{
					msg.setField(VID_RCC, RCC_INVALID_DCI_ID);
					debugPrintf(4, _T("ForceDCIPoll: DCI %d at node %d not found"), dciId, object->getId());
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

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(pRequest->getId());

   // Get source and destination
   shared_ptr<NetObj> pSource = FindObjectById(pRequest->getFieldAsUInt32(VID_SOURCE_OBJECT_ID));
   shared_ptr<NetObj> pDestination = FindObjectById(pRequest->getFieldAsUInt32(VID_DESTINATION_OBJECT_ID));
   if ((pSource != nullptr) && (pDestination != nullptr))
   {
      // Check object types
      if ((pSource->isDataCollectionTarget() || (pSource->getObjectClass() == OBJECT_TEMPLATE)) &&
		    (pDestination->isDataCollectionTarget() || (pDestination->getObjectClass() == OBJECT_TEMPLATE)))
      {
         bool doMove = pRequest->getFieldAsBoolean(VID_MOVE_FLAG);
         // Check access rights
         if ((pSource->checkAccessRights(m_dwUserId, doMove ? (OBJECT_ACCESS_READ | OBJECT_ACCESS_MODIFY) : OBJECT_ACCESS_READ)) &&
             (pDestination->checkAccessRights(m_dwUserId, OBJECT_ACCESS_MODIFY)))
         {
            UINT32 i, *pdwItemList, dwNumItems;
            int iErrors = 0;

            // Get list of items to be copied/moved
            dwNumItems = pRequest->getFieldAsUInt32(VID_NUM_ITEMS);
            pdwItemList = MemAllocArray<UINT32>(dwNumItems);
            pRequest->getFieldAsInt32Array(VID_ITEM_LIST, dwNumItems, pdwItemList);

            // Copy items
            for(i = 0; i < dwNumItems; i++)
            {
               shared_ptr<DCObject> pSrcItem = static_cast<DataCollectionOwner&>(*pSource).getDCObjectById(pdwItemList[i], m_dwUserId);
               if (pSrcItem != nullptr)
               {
                  DCObject *pDstItem = pSrcItem->clone();
                  pDstItem->setTemplateId(0, 0);
                  pDstItem->changeBinding(CreateUniqueId(IDG_ITEM),
                           static_pointer_cast<DataCollectionOwner>(pDestination), FALSE);
                  if (static_cast<DataCollectionOwner&>(*pDestination).addDCObject(pDstItem))
                  {
                     if (doMove)
                     {
                        // Delete original item
                        if (!static_cast<DataCollectionOwner&>(*pSource).deleteDCObject(pdwItemList[i], true, m_dwUserId))
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
                  iErrors++;
               }
            }

            // Cleanup
            MemFree(pdwItemList);
            static_cast<DataCollectionOwner&>(*pDestination).applyDCIChanges(!m_openDataCollectionConfigurations.contains(pDestination->getId()));
            msg.setField(VID_RCC, (iErrors == 0) ? RCC_SUCCESS : RCC_DCI_COPY_ERRORS);

            // Queue template update
            if (pDestination->getObjectClass() == OBJECT_TEMPLATE)
               static_cast<DataCollectionOwner&>(*pDestination).queueUpdate();
         }
         else  // User doesn't have enough rights on object(s)
         {
            msg.setField(VID_RCC, RCC_ACCESS_DENIED);
         }
      }
      else     // Object(s) is not a node
      {
         msg.setField(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
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
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request->getId());

   // Get node id and check object class and access rights
   shared_ptr<NetObj> object = FindObjectById(request->getFieldAsUInt32(VID_OBJECT_ID));
   if (object != nullptr)
   {
      if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
      {
			if (object->isDataCollectionTarget())
			{
				shared_ptr<DCObject> dci = static_cast<DataCollectionTarget&>(*object).getDCObjectById(request->getFieldAsUInt32(VID_DCI_ID), m_dwUserId);
				if ((dci != nullptr) && (dci->getType() == DCO_TYPE_ITEM))
				{
				   static_cast<DCItem&>(*dci).fillMessageWithThresholds(&msg, false);
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

#define SELECTION_COLUMNS (historicalDataType != DCO_TYPE_RAW) ? tablePrefix : _T(""), (historicalDataType == DCO_TYPE_BOTH) ? _T("_value,raw_value") : (historicalDataType == DCO_TYPE_PROCESSED) ?  _T("_value") : _T("raw_value")

/**
 * Prepare statement for reading data from idata/tdata table
 */
static DB_STATEMENT PrepareDataSelect(DB_HANDLE hdb, UINT32 nodeId, int dciType, DCObjectStorageClass storageClass,
         UINT32 maxRows, HistoricalDataType historicalDataType, const TCHAR *condition)
{
   const TCHAR *tablePrefix = (dciType == DCO_TYPE_ITEM) ? _T("idata") : _T("tdata");
	TCHAR query[512];
	if (g_flags & AF_SINGLE_TABLE_PERF_DATA)
	{
      switch(g_dbSyntax)
      {
         case DB_SYNTAX_MSSQL:
            _sntprintf(query, 512, _T("SELECT TOP %d %s_timestamp,%s%s FROM %s WHERE item_id=?%s ORDER BY %s_timestamp DESC"),
                     (int)maxRows, tablePrefix, SELECTION_COLUMNS,
                     tablePrefix, condition, tablePrefix);
            break;
         case DB_SYNTAX_ORACLE:
            _sntprintf(query, 512, _T("SELECT * FROM (SELECT %s_timestamp,%s%s FROM %s WHERE item_id=?%s ORDER BY %s_timestamp DESC) WHERE ROWNUM<=%d"),
                     tablePrefix, SELECTION_COLUMNS,
                     tablePrefix, condition, tablePrefix, (int)maxRows);
            break;
         case DB_SYNTAX_MYSQL:
         case DB_SYNTAX_PGSQL:
         case DB_SYNTAX_SQLITE:
            _sntprintf(query, 512, _T("SELECT %s_timestamp,%s%s FROM %s WHERE item_id=?%s ORDER BY %s_timestamp DESC LIMIT %d"),
                     tablePrefix, SELECTION_COLUMNS,
                     tablePrefix, condition, tablePrefix, (int)maxRows);
            break;
         case DB_SYNTAX_TSDB:
            _sntprintf(query, 512, _T("SELECT date_part('epoch',%s_timestamp)::int,%s%s FROM %s_sc_%s WHERE item_id=?%s ORDER BY %s_timestamp DESC LIMIT %d"),
                     tablePrefix, SELECTION_COLUMNS,
                     tablePrefix, DCObject::getStorageClassName(storageClass), condition, tablePrefix, (int)maxRows);
            break;
         case DB_SYNTAX_DB2:
            _sntprintf(query, 512, _T("SELECT %s_timestamp,%s%s FROM %s WHERE item_id=?%s ORDER BY %s_timestamp DESC FETCH FIRST %d ROWS ONLY"),
                     tablePrefix, SELECTION_COLUMNS,
                     tablePrefix, condition, tablePrefix, (int)maxRows);
            break;
         default:
            DbgPrintf(1, _T("INTERNAL ERROR: unsupported database in PrepareDataSelect"));
            return nullptr;   // Unsupported database
      }
	}
	else
	{
      switch(g_dbSyntax)
      {
         case DB_SYNTAX_MSSQL:
            _sntprintf(query, 512, _T("SELECT TOP %d %s_timestamp,%s%s FROM %s_%u WHERE item_id=?%s ORDER BY %s_timestamp DESC"),
                     (int)maxRows, tablePrefix, SELECTION_COLUMNS,
                     tablePrefix, nodeId, condition, tablePrefix);
            break;
         case DB_SYNTAX_ORACLE:
            _sntprintf(query, 512, _T("SELECT * FROM (SELECT %s_timestamp,%s%s FROM %s_%u WHERE item_id=?%s ORDER BY %s_timestamp DESC) WHERE ROWNUM<=%d"),
                     tablePrefix, SELECTION_COLUMNS,
                     tablePrefix, nodeId, condition, tablePrefix, (int)maxRows);
            break;
         case DB_SYNTAX_MYSQL:
         case DB_SYNTAX_PGSQL:
         case DB_SYNTAX_SQLITE:
         case DB_SYNTAX_TSDB:
            _sntprintf(query, 512, _T("SELECT %s_timestamp,%s%s FROM %s_%u WHERE item_id=?%s ORDER BY %s_timestamp DESC LIMIT %d"),
                     tablePrefix, SELECTION_COLUMNS,
                     tablePrefix, nodeId, condition, tablePrefix, (int)maxRows);
            break;
         case DB_SYNTAX_DB2:
            _sntprintf(query, 512, _T("SELECT %s_timestamp,%s%s FROM %s_%u WHERE item_id=?%s ORDER BY %s_timestamp DESC FETCH FIRST %d ROWS ONLY"),
                     tablePrefix, SELECTION_COLUMNS,
                     tablePrefix, nodeId, condition, tablePrefix, (int)maxRows);
            break;
         default:
            DbgPrintf(1, _T("INTERNAL ERROR: unsupported database in PrepareDataSelect"));
            return nullptr;	// Unsupported database
      }
	}
	return DBPrepare(hdb, query);
}

/**
 * Get collected data for table or simple DCI
 */
bool ClientSession::getCollectedDataFromDB(NXCPMessage *request, NXCPMessage *response, const DataCollectionTarget& dcTarget, int dciType, HistoricalDataType historicalDataType)
{
   static UINT32 s_rowSize[] = { 8, 8, 16, 16, 516, 16, 8, 8, 16 };

	// Find DCI object
	shared_ptr<DCObject> dci = dcTarget.getDCObjectById(request->getFieldAsUInt32(VID_DCI_ID), 0);
	if (dci == nullptr)
	{
		response->setField(VID_RCC, RCC_INVALID_DCI_ID);
		return false;
	}

	if (!dci->hasAccess(m_dwUserId))
	{
      response->setField(VID_RCC, RCC_ACCESS_DENIED);
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

   DCI_DATA_HEADER *pData = nullptr;
   DCI_DATA_ROW *pCurr;

	// If only last value requested, try to get it from cache first
	if ((maxRows == 1) && (timeTo == 0) && (historicalDataType == DCO_TYPE_PROCESSED))
	{
	   debugPrintf(7, _T("getCollectedDataFromDB: maxRows set to 1, will try to read cached value"));

      TCHAR dataColumn[MAX_COLUMN_NAME] = _T("");
      ItemValue value;

	   if (dciType == DCO_TYPE_ITEM)
	   {
	      ItemValue *v = static_cast<DCItem*>(dci.get())->getInternalLastValue();
	      if (v == nullptr)
	         goto read_from_db;
	      value = *v;
	      delete v;
	   }
	   else
	   {
         request->getFieldAsString(VID_DATA_COLUMN, dataColumn, MAX_COLUMN_NAME);
         Table *t = static_cast<DCTable*>(dci.get())->getLastValue();
         if (t == nullptr)
            goto read_from_db;

         int column = t->getColumnIndex(dataColumn);
         if (column == -1)
         {
            t->decRefCount();
            goto read_from_db;
         }

         TCHAR instance[256];
         request->getFieldAsString(VID_INSTANCE, instance, 256);
         int row = t->findRowByInstance(instance);

         switch(static_cast<DCTable*>(dci.get())->getColumnDataType(dataColumn))
         {
            case DCI_DT_INT:
               value = (row != -1) ? t->getAsInt(row, column) : (int32_t)0;
               break;
            case DCI_DT_UINT:
            case DCI_DT_COUNTER32:
               value = (row != -1) ? t->getAsUInt(row, column) : (uint32_t)0;
               break;
            case DCI_DT_INT64:
               value = (row != -1) ? t->getAsInt64(row, column) : (int64_t)0;
               break;
            case DCI_DT_UINT64:
            case DCI_DT_COUNTER64:
               value = (row != -1) ? t->getAsUInt64(row, column) : (uint64_t)0;
               break;
            case DCI_DT_FLOAT:
               value = (row != -1) ? t->getAsDouble(row, column) : (double)0;
               break;
            case DCI_DT_STRING:
               value = (row != -1) ? t->getAsString(row, column) : _T("");
               break;
         }
         t->decRefCount();
	   }

      // Send CMD_REQUEST_COMPLETED message
      response->setField(VID_RCC, RCC_SUCCESS);
      if (dciType == DCO_TYPE_ITEM)
         static_cast<DCItem*>(dci.get())->fillMessageWithThresholds(response, false);
      sendMessage(response);

      int dataType;
      switch(dciType)
      {
         case DCO_TYPE_ITEM:
            dataType = static_cast<DCItem*>(dci.get())->getDataType();
            break;
         case DCO_TYPE_TABLE:
            dataType = static_cast<DCTable*>(dci.get())->getColumnDataType(dataColumn);
            break;
         default:
            dataType = DCI_DT_STRING;
            break;
      }

      // Allocate memory for data and prepare data header
      pData = (DCI_DATA_HEADER *)MemAlloc(s_rowSize[dataType] + sizeof(DCI_DATA_HEADER));
      pData->dataType = htonl((UINT32)dataType);
      pData->dciId = htonl(dci->getId());

      // Fill memory block with records
      pCurr = (DCI_DATA_ROW *)(((char *)pData) + sizeof(DCI_DATA_HEADER));
      pCurr->timeStamp = (UINT32)dci->getLastPollTime();
      switch(dataType)
      {
         case DCI_DT_INT:
         case DCI_DT_UINT:
         case DCI_DT_COUNTER32:
            pCurr->value.int32 = htonl(value.getUInt32());
            break;
         case DCI_DT_INT64:
         case DCI_DT_UINT64:
         case DCI_DT_COUNTER64:
            pCurr->value.ext.v64.int64 = htonq(value.getUInt64());
            break;
         case DCI_DT_FLOAT:
            pCurr->value.ext.v64.real = htond(value.getDouble());
            break;
         case DCI_DT_STRING:
#ifdef UNICODE
#ifdef UNICODE_UCS4
            ucs4_to_ucs2(value.getString(), -1, pCurr->value.string, MAX_DCI_STRING_VALUE);
#else
            wcslcpy(pCurr->value.string, value.getString(), MAX_DCI_STRING_VALUE);
#endif
#else
            mb_to_ucs2(value.getString(), -1, pCurr->value.string, MAX_DCI_STRING_VALUE);
#endif
            SwapUCS2String(pCurr->value.string);
            break;
      }
      pData->numRows = 1;

      // Prepare and send raw message with fetched data
      NXCP_MESSAGE *msg =
         CreateRawNXCPMessage(CMD_DCI_DATA, request->getId(), 0,
                              pData, s_rowSize[dataType] + sizeof(DCI_DATA_HEADER),
                              nullptr, isCompressionEnabled());
      MemFree(pData);
      sendRawMessage(msg);
      MemFree(msg);

      return true;
	}

read_from_db:
   debugPrintf(7, _T("getCollectedDataFromDB: will read from database (maxRows = %d)"), maxRows);

	TCHAR condition[256] = _T("");
	if ((g_dbSyntax == DB_SYNTAX_TSDB) && (g_flags & AF_SINGLE_TABLE_PERF_DATA))
	{
      if (timeFrom != 0)
         _tcscpy(condition, (dciType == DCO_TYPE_TABLE) ? _T(" AND tdata_timestamp>=to_timestamp(?)") : _T(" AND idata_timestamp>=to_timestamp(?)"));
      if (timeTo != 0)
         _tcscat(condition, (dciType == DCO_TYPE_TABLE) ? _T(" AND tdata_timestamp<=to_timestamp(?)") : _T(" AND idata_timestamp<=to_timestamp(?)"));
	}
	else
	{
      if (timeFrom != 0)
         _tcscpy(condition, (dciType == DCO_TYPE_TABLE) ? _T(" AND tdata_timestamp>=?") : _T(" AND idata_timestamp>=?"));
      if (timeTo != 0)
         _tcscat(condition, (dciType == DCO_TYPE_TABLE) ? _T(" AND tdata_timestamp<=?") : _T(" AND idata_timestamp<=?"));
	}

	bool success = false;
	DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
	DB_STATEMENT hStmt = PrepareDataSelect(hdb, dcTarget.getId(), dciType, dci->getStorageClass(), maxRows, historicalDataType, condition);
	if (hStmt != nullptr)
	{
		TCHAR dataColumn[MAX_COLUMN_NAME] = _T("");
      TCHAR instance[256];

		int pos = 1;
		DBBind(hStmt, pos++, DB_SQLTYPE_INTEGER, dci->getId());
		if (dciType == DCO_TYPE_TABLE)
		{
			request->getFieldAsString(VID_DATA_COLUMN, dataColumn, MAX_COLUMN_NAME);
         request->getFieldAsString(VID_INSTANCE, instance, 256);
		}
		if (timeFrom != 0)
			DBBind(hStmt, pos++, DB_SQLTYPE_INTEGER, timeFrom);
		if (timeTo != 0)
			DBBind(hStmt, pos++, DB_SQLTYPE_INTEGER, timeTo);

		DB_UNBUFFERED_RESULT hResult = DBSelectPreparedUnbuffered(hStmt);
		if (hResult != nullptr)
		{
#if !defined(UNICODE) || defined(UNICODE_UCS4)
			TCHAR szBuffer[MAX_DCI_STRING_VALUE];
#endif

			// Send CMD_REQUEST_COMPLETED message
			response->setField(VID_RCC, RCC_SUCCESS);
			if (dciType == DCO_TYPE_ITEM)
			   static_cast<DCItem*>(dci.get())->fillMessageWithThresholds(response, false);
			sendMessage(response);

			int dataType;
			switch(dciType)
			{
				case DCO_TYPE_ITEM:
					dataType = static_cast<DCItem*>(dci.get())->getDataType();
					break;
				case DCO_TYPE_TABLE:
					dataType = static_cast<DCTable*>(dci.get())->getColumnDataType(dataColumn);
					break;
				default:
					dataType = DCI_DT_STRING;
					break;
			}

			// Allocate memory for data and prepare data header
			int allocated = 8192;
			int rows = 0;
			pData = (DCI_DATA_HEADER *)MemAlloc(allocated * s_rowSize[dataType] + sizeof(DCI_DATA_HEADER));
			pData->dataType = htonl((UINT32)dataType);
			pData->dciId = htonl(dci->getId());

			// Fill memory block with records
			pCurr = (DCI_DATA_ROW *)(((char *)pData) + sizeof(DCI_DATA_HEADER));
			while(DBFetch(hResult))
			{
			   if (rows == allocated)
			   {
			      allocated += 8192;
		         pData = MemRealloc(pData, allocated * s_rowSize[dataType] + sizeof(DCI_DATA_HEADER));
		         pCurr = (DCI_DATA_ROW *)(((char *)pData + s_rowSize[dataType] * rows) + sizeof(DCI_DATA_HEADER));
			   }
            rows++;

				pCurr->timeStamp = htonl(DBGetFieldULong(hResult, 0));
				if (dciType == DCO_TYPE_ITEM)
				{
               switch(dataType)
               {
                  case DCI_DT_INT:
                  case DCI_DT_UINT:
                  case DCI_DT_COUNTER32:
                     pCurr->value.int32 = htonl(DBGetFieldULong(hResult, 1));
                     break;
                  case DCI_DT_INT64:
                  case DCI_DT_UINT64:
                  case DCI_DT_COUNTER64:
                     pCurr->value.ext.v64.int64 = htonq(DBGetFieldUInt64(hResult, 1));
                     break;
                  case DCI_DT_FLOAT:
                     pCurr->value.ext.v64.real = htond(DBGetFieldDouble(hResult, 1));
                     break;
                  case DCI_DT_STRING:
#ifdef UNICODE
#ifdef UNICODE_UCS4
                     DBGetField(hResult, 1, szBuffer, MAX_DCI_STRING_VALUE);
                     ucs4_to_ucs2(szBuffer, -1, pCurr->value.string, MAX_DCI_STRING_VALUE);
#else
                     DBGetField(hResult, 1, pCurr->value.string, MAX_DCI_STRING_VALUE);
#endif
#else
                     DBGetField(hResult, 1, szBuffer, MAX_DCI_STRING_VALUE);
                     mb_to_ucs2(szBuffer, -1, pCurr->value.string, MAX_DCI_STRING_VALUE);
#endif
                     SwapUCS2String(pCurr->value.string);
                     break;
               }

               if (historicalDataType == DCO_TYPE_BOTH)
               {
                  pCurr = (DCI_DATA_ROW *)(((char *)pCurr) + s_rowSize[dataType]);
                  if (rows == allocated)
                  {
                     allocated += 8192;
                     pData = (DCI_DATA_HEADER *)realloc(pData, allocated * s_rowSize[dataType] + sizeof(DCI_DATA_HEADER));
                     pCurr = (DCI_DATA_ROW *)(((char *)pData + s_rowSize[dataType] * rows) + sizeof(DCI_DATA_HEADER));
                  }
                  rows++;

                  pCurr->timeStamp = 0;   // raw value indicator
                  switch(dataType)
                  {
                     case DCI_DT_INT:
                     case DCI_DT_UINT:
                     case DCI_DT_COUNTER32:
                        pCurr->value.int32 = htonl(DBGetFieldULong(hResult, 2));
                        break;
                     case DCI_DT_INT64:
                     case DCI_DT_UINT64:
                     case DCI_DT_COUNTER64:
                        pCurr->value.ext.v64.int64 = htonq(DBGetFieldUInt64(hResult, 2));
                        break;
                     case DCI_DT_FLOAT:
                        pCurr->value.ext.v64.real = htond(DBGetFieldDouble(hResult, 2));
                        break;
                     case DCI_DT_STRING:
   #ifdef UNICODE
   #ifdef UNICODE_UCS4
                        DBGetField(hResult, 2, szBuffer, MAX_DCI_STRING_VALUE);
                        ucs4_to_ucs2(szBuffer, -1, pCurr->value.string, MAX_DCI_STRING_VALUE);
   #else
                        DBGetField(hResult, 2, pCurr->value.string, MAX_DCI_STRING_VALUE);
   #endif
   #else
                        DBGetField(hResult, 2, szBuffer, MAX_DCI_STRING_VALUE);
                        mb_to_ucs2(szBuffer, -1, pCurr->value.string, MAX_DCI_STRING_VALUE);
   #endif
                        SwapUCS2String(pCurr->value.string);
                        break;
                  }
               }
				}
				else
				{
				   char *encodedTable = DBGetFieldUTF8(hResult, 1, nullptr, 0);
				   if (encodedTable != nullptr)
				   {
				      Table *table = Table::createFromPackedXML(encodedTable);
				      if (table != nullptr)
				      {
				         int row = table->findRowByInstance(instance);
				         int col = table->getColumnIndex(dataColumn);
		               switch(dataType)
		               {
		                  case DCI_DT_INT:
                           pCurr->value.int32 = htonl((UINT32)table->getAsInt(row, col));
                           break;
		                  case DCI_DT_UINT:
		                  case DCI_DT_COUNTER32:
		                     pCurr->value.int32 = htonl(table->getAsUInt(row, col));
		                     break;
		                  case DCI_DT_INT64:
                           pCurr->value.ext.v64.int64 = htonq((UINT64)table->getAsInt64(row, col));
                           break;
		                  case DCI_DT_UINT64:
		                  case DCI_DT_COUNTER64:
		                     pCurr->value.ext.v64.int64 = htonq(table->getAsUInt64(row, col));
		                     break;
		                  case DCI_DT_FLOAT:
		                     pCurr->value.ext.v64.real = htond(table->getAsDouble(row, col));
		                     break;
		                  case DCI_DT_STRING:
#ifdef UNICODE
#ifdef UNICODE_UCS4
		                     ucs4_to_ucs2(CHECK_NULL_EX(table->getAsString(row, col)), -1, pCurr->value.string, MAX_DCI_STRING_VALUE);
#else
		                     wcslcpy(pCurr->value.string, CHECK_NULL_EX(table->getAsString(row, col)), MAX_DCI_STRING_VALUE);
#endif
#else
		                     mb_to_ucs2(CHECK_NULL_EX(table->getAsString(row, col)), -1, pCurr->value.string, MAX_DCI_STRING_VALUE);
#endif
		                     SwapUCS2String(pCurr->value.string);
		                     break;
		               }
				         delete table;
				      }
				      MemFree(encodedTable);
				   }
				}
				pCurr = (DCI_DATA_ROW *)(((char *)pCurr) + s_rowSize[dataType]);
			}
			DBFreeResult(hResult);
			pData->numRows = htonl(rows);

			// Prepare and send raw message with fetched data
			NXCP_MESSAGE *msg =
				CreateRawNXCPMessage(CMD_DCI_DATA, request->getId(), 0,
											pData, rows * s_rowSize[dataType] + sizeof(DCI_DATA_HEADER),
											nullptr, isCompressionEnabled());
			MemFree(pData);
			sendRawMessage(msg);
			MemFree(msg);
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

   shared_ptr<NetObj> object = FindObjectById(request->getFieldAsUInt32(VID_OBJECT_ID));
   if (object != nullptr)
   {
		if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
		{
			if (object->isDataCollectionTarget())
			{
				if (!(g_flags & AF_DB_CONNECTION_LOST))
				{
					success = getCollectedDataFromDB(request, &msg, static_cast<DataCollectionTarget&>(*object),
					         DCO_TYPE_ITEM, static_cast<HistoricalDataType>(request->getFieldAsInt16(VID_HISTORICAL_DATA_TYPE)));
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

   shared_ptr<NetObj> object = FindObjectById(request->getFieldAsUInt32(VID_OBJECT_ID));
   if (object != nullptr)
   {
		if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
		{
			if (object->isDataCollectionTarget())
			{
				if (!(g_flags & AF_DB_CONNECTION_LOST))
				{
					success = getCollectedDataFromDB(request, &msg, static_cast<DataCollectionTarget&>(*object), DCO_TYPE_TABLE, DCO_TYPE_PROCESSED);
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
   NXCPMessage msg(CMD_REQUEST_COMPLETED, pRequest->getId());

   // Get node id and check object class and access rights
   shared_ptr<NetObj> object = FindObjectById(pRequest->getFieldAsUInt32(VID_OBJECT_ID));
   if (object != nullptr)
   {
      if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
      {
         if (object->isDataCollectionTarget())
         {
            msg.setField(VID_RCC,
               static_cast<DataCollectionTarget&>(*object).getLastValues(&msg,
                  pRequest->getFieldAsBoolean(VID_OBJECT_TOOLTIP_ONLY),
                  pRequest->getFieldAsBoolean(VID_OVERVIEW_ONLY),
                  pRequest->getFieldAsBoolean(VID_INCLUDE_NOVALUE_OBJECTS),
                  m_dwUserId));
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
   NXCPMessage msg(CMD_REQUEST_COMPLETED, pRequest->getId());

   int size = pRequest->getFieldAsInt32(VID_NUM_ITEMS);
   UINT32 incomingIndex = VID_DCI_VALUES_BASE;
   UINT32 outgoingIndex = VID_DCI_VALUES_BASE;
   for(int i = 0; i < size; i++, incomingIndex += 10)
   {
      shared_ptr<NetObj> object = FindObjectById(pRequest->getFieldAsUInt32(incomingIndex));
      if (object != nullptr)
      {
         if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
         {
            if (object->isDataCollectionTarget() || (object->getObjectClass() == OBJECT_TEMPLATE))
            {
               UINT32 dciID = pRequest->getFieldAsUInt32(incomingIndex+1);
               shared_ptr<DCObject> dcoObj = static_cast<DataCollectionTarget&>(*object).getDCObjectById(dciID, m_dwUserId);
               if (dcoObj == nullptr)
                  continue;

               INT16 type;
               UINT32 mostCriticalSeverity;
               TCHAR *value;
               if (dcoObj->getType() == DCO_TYPE_TABLE)
               {
                  TCHAR *column = pRequest->getFieldAsString(incomingIndex + 2);
                  TCHAR *instance = pRequest->getFieldAsString(incomingIndex + 3);
                  if ((column == nullptr) || (instance == nullptr) || (column[0] == 0) || (instance[0] == 0))
                  {
                     MemFree(column);
                     MemFree(instance);
                     continue;
                  }

                  Table *t = static_cast<DCTable*>(dcoObj.get())->getLastValue();
                  int columnIndex =  t->getColumnIndex(column);
                  int rowIndex = t->findRowByInstance(instance);
                  type = t->getColumnDataType(columnIndex);
                  value = MemCopyString(t->getAsString(rowIndex, columnIndex));
                  mostCriticalSeverity = SEVERITY_NORMAL;
                  t->decRefCount();

                  MemFree(column);
                  MemFree(instance);
               }
               else if (dcoObj->getType() == DCO_TYPE_ITEM)
               {
                  DCItem* item = static_cast<DCItem*>(dcoObj.get());
                  type = item->getDataType();
                  value = MemCopyString(item->getLastValue());
                  mostCriticalSeverity = item->getThresholdSeverity();
               }
               else
               {
                  continue;
               }

               msg.setField(outgoingIndex + 1, dciID);
               msg.setField(outgoingIndex + 2, CHECK_NULL_EX(value));
               msg.setField(outgoingIndex + 3, type);
               msg.setField(outgoingIndex + 4, static_cast<INT16>(dcoObj->getStatus()));
               msg.setField(outgoingIndex + 5, object->getId());
               msg.setField(outgoingIndex + 6, static_cast<INT16>(dcoObj->getDataSource()));
               msg.setField(outgoingIndex + 7, dcoObj->getName());
               msg.setField(outgoingIndex + 8, dcoObj->getDescription());
               msg.setField(outgoingIndex + 9, mostCriticalSeverity);
               MemFree(value);
               outgoingIndex += 10;
            }
         }
      }
   }

   msg.setField(VID_NUM_ITEMS, (outgoingIndex - VID_DCI_VALUES_BASE) / 10);
   msg.setField(VID_RCC, RCC_SUCCESS);

   sendMessage(&msg);
}

/**
 * Send latest collected value for given table DCI of given node
 */
void ClientSession::getTableLastValue(NXCPMessage *request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request->getId());

   // Get node id and check object class and access rights
   shared_ptr<NetObj> object = FindObjectById(request->getFieldAsUInt32(VID_OBJECT_ID));
   if (object != nullptr)
   {
      if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
      {
         if (object->isDataCollectionTarget())
         {
				msg.setField(VID_RCC, static_cast<DataCollectionTarget&>(*object).getTableLastValue(request->getFieldAsUInt32(VID_DCI_ID), &msg));
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
 * Send latest collected value for given DCI (table or single valued) of given node
 */
void ClientSession::getLastValue(NXCPMessage *request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request->getId());

   // Get node id and check object class and access rights
   shared_ptr<NetObj> object = FindObjectById(request->getFieldAsUInt32(VID_OBJECT_ID));
   if (object != nullptr)
   {
      if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
      {
         if (object->isDataCollectionTarget())
         {
            msg.setField(VID_RCC, static_cast<DataCollectionTarget&>(*object).getDciLastValue(request->getFieldAsUInt32(VID_DCI_ID), &msg));
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
 * Send only active DCI thresholds for given DCI config
 */
void ClientSession::getActiveThresholds(NXCPMessage *pRequest)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, pRequest->getId());

   UINT32 base = VID_DCI_VALUES_BASE;
   int numItems = pRequest->getFieldAsInt32(VID_NUM_ITEMS);

   for(int i = 0; i < numItems; i++, base+=2)
   {
      shared_ptr<NetObj> object = FindObjectById(pRequest->getFieldAsUInt32(base));
      if (object != nullptr && object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
      {
         if (object->isDataCollectionTarget() || (object->getObjectClass() == OBJECT_TEMPLATE))
         {
            shared_ptr<DCObject> dcObject = static_cast<DataCollectionTarget&>(*object).getDCObjectById(pRequest->getFieldAsUInt32(base + 1), m_dwUserId);
            if (dcObject != nullptr)
               static_cast<DCItem&>(*dcObject).fillMessageWithThresholds(&msg, true);
         }
      }
   }

   msg.setField(VID_RCC, RCC_SUCCESS);
   sendMessage(&msg);
}

/**
 * Open event processing policy
 */
void ClientSession::openEventProcessingPolicy(NXCPMessage *request)
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
      if (!readOnly && !LockComponent(CID_EPP, m_id, m_sessionName, nullptr, buffer))
      {
         msg.setField(VID_RCC, RCC_COMPONENT_LOCKED);
         msg.setField(VID_LOCKED_BY, buffer);
      }
      else
      {
         if (!readOnly)
         {
            InterlockedOr(&m_flags, CSF_EPP_LOCKED);
         }
         msg.setField(VID_RCC, RCC_SUCCESS);
         msg.setField(VID_NUM_RULES, g_pEventPolicy->getNumRules());
         success = true;
         writeAuditLog(AUDIT_SYSCFG, true, 0, _T("Open event processing policy"));
      }
   }
   else
   {
      // Current user has no rights for event policy management
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);
      writeAuditLog(AUDIT_SYSCFG, false, 0, _T("Access denied on opening event processing policy"));
   }

   // Send response
   sendMessage(&msg);

   // Send policy to client
   if (success)
   {
      g_pEventPolicy->sendToClient(this, request->getId());
   }

   // This handler is called directly, not through processRequest,
   // so session reference count should be decremented here and
   // request message deleted
   delete request;
   decRefCount();
}

/**
 * Close event processing policy
 */
void ClientSession::closeEventProcessingPolicy(NXCPMessage *request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request->getId());
   if (m_systemAccessRights & SYSTEM_ACCESS_EPP)
   {
      if (m_flags & CSF_EPP_LOCKED)
      {
         if (m_ppEPPRuleList != nullptr)
         {
            if (m_flags & CSF_EPP_UPLOAD)  // Aborted in the middle of EPP transfer
            {
               for(UINT32 i = 0; i < m_dwRecordsUploaded; i++)
                  delete m_ppEPPRuleList[i];
            }
            MemFreeAndNull(m_ppEPPRuleList);
         }
         InterlockedAnd(&m_flags, ~(CSF_EPP_LOCKED | CSF_EPP_UPLOAD));
         UnlockComponent(CID_EPP);
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

   // This handler is called directly, not through processRequest,
   // so session reference count should be decremented here and
   // request message deleted
   delete request;
   decRefCount();
}

/**
 * Save event processing policy
 */
void ClientSession::saveEventProcessingPolicy(NXCPMessage *request)
{
   NXCPMessage msg;

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(request->getId());

   if (m_systemAccessRights & SYSTEM_ACCESS_EPP)
   {
      if (m_flags & CSF_EPP_LOCKED)
      {
         msg.setField(VID_RCC, RCC_SUCCESS);
         m_dwNumRecordsToUpload = request->getFieldAsUInt32(VID_NUM_RULES);
         m_dwRecordsUploaded = 0;
         if (m_dwNumRecordsToUpload == 0)
         {
            g_pEventPolicy->replacePolicy(0, nullptr);
            if (g_pEventPolicy->saveToDB())
            {
               writeAuditLog(AUDIT_SYSCFG, true, 0, _T("Event processing policy cleared"));
            }
            else
            {
               msg.setField(VID_RCC, RCC_DB_FAILURE);
            }
         }
         else
         {
            InterlockedOr(&m_flags, CSF_EPP_UPLOAD);
            m_ppEPPRuleList = MemAllocArray<EPRule*>(m_dwNumRecordsToUpload);
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
      writeAuditLog(AUDIT_SYSCFG, false, 0, _T("Access denied on event processing policy change"));
   }

   // Send response
   sendMessage(&msg);

   // This handler is called directly, not through processRequest,
   // so session reference count should be decremented here and
   // request message deleted
   delete request;
   decRefCount();
}

/**
 * Process EPP rule received from client
 */
void ClientSession::processEventProcessingPolicyRecord(NXCPMessage *request)
{
   if (!(m_flags & CSF_EPP_LOCKED) || !(m_flags & CSF_EPP_UPLOAD))
   {
      NXCPMessage msg(CMD_REQUEST_COMPLETED, request->getId());
      msg.setField(VID_RCC, RCC_OUT_OF_STATE_REQUEST);
      sendMessage(&msg);
   }
   else
   {
      if (m_dwRecordsUploaded < m_dwNumRecordsToUpload)
      {
         m_ppEPPRuleList[m_dwRecordsUploaded] = new EPRule(request);
         m_dwRecordsUploaded++;
         if (m_dwRecordsUploaded == m_dwNumRecordsToUpload)
         {
            // All records received, replace event policy...
            debugPrintf(5, _T("Replacing event processing policy with a new one at %p (%d rules)"),
                        m_ppEPPRuleList, m_dwNumRecordsToUpload);
            json_t *oldVersion = g_pEventPolicy->toJson();
            g_pEventPolicy->replacePolicy(m_dwNumRecordsToUpload, m_ppEPPRuleList);
            bool success = g_pEventPolicy->saveToDB();
            MemFreeAndNull(m_ppEPPRuleList);
            json_t *newVersion = g_pEventPolicy->toJson();

            // ... and send final confirmation
            NXCPMessage msg(CMD_REQUEST_COMPLETED, request->getId());
            msg.setField(VID_RCC, success ? RCC_SUCCESS : RCC_DB_FAILURE);
            sendMessage(&msg);

            InterlockedAnd(&m_flags, ~CSF_EPP_UPLOAD);

            writeAuditLogWithValues(AUDIT_SYSCFG, true, 0, oldVersion, newVersion, _T("Event processing policy updated"));
            json_decref(oldVersion);
            json_decref(newVersion);
         }
      }
   }

   // This handler is called directly, not through processRequest,
   // so session reference count should be decremented here and
   // request message deleted
   delete request;
   decRefCount();
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
void ClientSession::createObject(NXCPMessage *request)
{
   NXCPMessage msg;
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(request->getId());

   int objectClass = request->getFieldAsUInt16(VID_OBJECT_CLASS);
   int32_t zoneUIN = request->getFieldAsUInt32(VID_ZONE_UIN);

   // Find parent object
   shared_ptr<NetObj> parent = FindObjectById(request->getFieldAsUInt32(VID_PARENT_ID));

   TCHAR nodePrimaryName[MAX_DNS_NAME];
   InetAddress ipAddr;
   bool parentAlwaysValid = false;

   if (objectClass == OBJECT_NODE)
   {
		if (request->isFieldExist(VID_PRIMARY_NAME))
		{
			request->getFieldAsString(VID_PRIMARY_NAME, nodePrimaryName, MAX_DNS_NAME);
         ipAddr = ResolveHostName(zoneUIN, nodePrimaryName);
		}
		else
		{
         ipAddr = request->getFieldAsInetAddress(VID_IP_ADDRESS);
         ipAddr.toString(nodePrimaryName);
		}
      if ((parent == nullptr) && ipAddr.isValidUnicast())
      {
         parent = FindSubnetForNode(zoneUIN, ipAddr);
         parentAlwaysValid = true;
      }
   }

   if ((parent != nullptr) || (objectClass == OBJECT_NODE))
   {
      // User should have create access to parent object
      if ((parent != nullptr) ?
            parent->checkAccessRights(m_dwUserId, OBJECT_ACCESS_CREATE) :
            g_entireNetwork->checkAccessRights(m_dwUserId, OBJECT_ACCESS_CREATE))
      {
         // Parent object should be of valid type
         if (parentAlwaysValid || IsValidParentClass(objectClass, (parent != nullptr) ? parent->getObjectClass() : -1))
         {
				// Check zone
				bool zoneIsValid;
				if (IsZoningEnabled() && (zoneUIN != 0) && (objectClass != OBJECT_ZONE))
				{
					zoneIsValid = (FindZoneByUIN(zoneUIN) != nullptr);
				}
				else
				{
					zoneIsValid = true;
				}

				if (zoneIsValid)
				{
				   TCHAR objectName[MAX_OBJECT_NAME];
					request->getFieldAsString(VID_OBJECT_NAME, objectName, MAX_OBJECT_NAME);

					// Do additional validation by modules
               UINT32 moduleRCC = RCC_SUCCESS;
               ENUMERATE_MODULES(pfValidateObjectCreation)
               {
                  moduleRCC = CURRENT_MODULE.pfValidateObjectCreation(objectClass, objectName, ipAddr, zoneUIN, request);
                  if (moduleRCC != RCC_SUCCESS)
                  {
                     debugPrintf(4, _T("Creation of object \"%s\" of class %d blocked by module %s (RCC=%d)"),
                              objectName, objectClass, CURRENT_MODULE.szName, moduleRCC);
                     break;
                  }
               }

               if (moduleRCC == RCC_SUCCESS)
               {
                  ObjectTransactionStart();

                  // Create new object
                  shared_ptr<NetObj> object;
                  UINT32 nodeId;
                  TCHAR deviceId[MAX_OBJECT_NAME];
                  switch(objectClass)
                  {
                     case OBJECT_BUSINESSSERVICE:
                        object = MakeSharedNObject<BusinessService>(objectName);
                        NetObjInsert(object, true, false);
                        break;
                     case OBJECT_CHASSIS:
                        object = MakeSharedNObject<Chassis>(objectName, request->getFieldAsUInt32(VID_CONTROLLER_ID));
                        NetObjInsert(object, true, false);
                        break;
                     case OBJECT_CLUSTER:
                        object = MakeSharedNObject<Cluster>(objectName, zoneUIN);
                        NetObjInsert(object, true, false);
                        break;
                     case OBJECT_CONDITION:
                        object = MakeSharedNObject<ConditionObject>(true);
                        object->setName(objectName);
                        NetObjInsert(object, true, false);
                        break;
                     case OBJECT_CONTAINER:
                        object = MakeSharedNObject<Container>(objectName, request->getFieldAsUInt32(VID_CATEGORY));
                        NetObjInsert(object, true, false);
                        object->calculateCompoundStatus();  // Force status change to NORMAL
                        break;
                     case OBJECT_DASHBOARD:
                        object = MakeSharedNObject<Dashboard>(objectName);
                        NetObjInsert(object, true, false);
                        break;
                     case OBJECT_DASHBOARDGROUP:
                        object = MakeSharedNObject<DashboardGroup>(objectName);
                        NetObjInsert(object, true, false);
                        object->calculateCompoundStatus();
                        break;
                     case OBJECT_INTERFACE:
                        {
                           InterfaceInfo ifInfo(request->getFieldAsUInt32(VID_IF_INDEX));
                           _tcslcpy(ifInfo.name, objectName, MAX_DB_STRING);
                           InetAddress addr = request->getFieldAsInetAddress(VID_IP_ADDRESS);
                           if (addr.isValidUnicast())
                              ifInfo.ipAddrList.add(addr);
                           ifInfo.type = request->getFieldAsUInt32(VID_IF_TYPE);
                           request->getFieldAsBinary(VID_MAC_ADDR, ifInfo.macAddr, MAC_ADDR_LENGTH);
                           ifInfo.location.chassis = request->getFieldAsUInt32(VID_PHY_CHASSIS);
                           ifInfo.location.module = request->getFieldAsUInt32(VID_PHY_MODULE);
                           ifInfo.location.pic = request->getFieldAsUInt32(VID_PHY_PIC);
                           ifInfo.location.port = request->getFieldAsUInt32(VID_PHY_PORT);
                           ifInfo.isPhysicalPort = request->getFieldAsBoolean(VID_IS_PHYS_PORT);
                           object = static_cast<Node&>(*parent).createNewInterface(&ifInfo, true, false);
                        }
                        break;
                     case OBJECT_MOBILEDEVICE:
                        request->getFieldAsString(VID_DEVICE_ID, deviceId, MAX_OBJECT_NAME);
                        object = MakeSharedNObject<MobileDevice>(objectName, deviceId);
                        NetObjInsert(object, true, false);
                        break;
                     case OBJECT_SENSOR:
                        object = Sensor::create(objectName, request);
                        if (object != nullptr)
                           NetObjInsert(object, true, false);
                        break;
                     case OBJECT_NETWORKMAP:
                        {
                           IntegerArray<UINT32> seeds;
                           request->getFieldAsInt32Array(VID_SEED_OBJECTS, &seeds);
                           object = MakeSharedNObject<NetworkMap>((int)request->getFieldAsUInt16(VID_MAP_TYPE), &seeds);
                           object->setName(objectName);
                           NetObjInsert(object, true, false);
                        }
                        break;
                     case OBJECT_NETWORKMAPGROUP:
                        object = MakeSharedNObject<NetworkMapGroup>(objectName);
                        NetObjInsert(object, true, false);
                        object->calculateCompoundStatus();  // Force status change to NORMAL
                        break;
                     case OBJECT_NETWORKSERVICE:
                        object = MakeSharedNObject<NetworkService>(request->getFieldAsInt16(VID_SERVICE_TYPE),
                                                    request->getFieldAsUInt16(VID_IP_PROTO),
                                                    request->getFieldAsUInt16(VID_IP_PORT),
                                                    request->getFieldAsString(VID_SERVICE_REQUEST),
                                                    request->getFieldAsString(VID_SERVICE_RESPONSE),
                                                    static_pointer_cast<Node>(parent));
                        object->setName(objectName);
                        NetObjInsert(object, true, false);
                        break;
                     case OBJECT_NODE:
                     {
                        NewNodeData newNodeData(request, ipAddr);
                        if ((parent != nullptr) && (parent->getObjectClass() == OBJECT_CLUSTER))
                           newNodeData.cluster = static_pointer_cast<Cluster>(parent);
                        object = PollNewNode(&newNodeData);
                        if (object != nullptr)
                        {
                           static_cast<Node&>(*object).setPrimaryHostName(nodePrimaryName);
                        }
                        break;
                     }
                     case OBJECT_NODELINK:
                        nodeId = request->getFieldAsUInt32(VID_NODE_ID);
                        if (nodeId > 0)
                        {
                           object = MakeSharedNObject<NodeLink>(objectName, nodeId);
                           NetObjInsert(object, true, false);
                        }
                        break;
                     case OBJECT_RACK:
                        object = MakeSharedNObject<Rack>(objectName, (int)request->getFieldAsUInt16(VID_HEIGHT));
                        NetObjInsert(object, true, false);
                        break;
                     case OBJECT_SLMCHECK:
                        object = MakeSharedNObject<SlmCheck>(objectName, request->getFieldAsBoolean(VID_IS_TEMPLATE));
                        NetObjInsert(object, true, false);
                        break;
                     case OBJECT_TEMPLATE:
                        object = MakeSharedNObject<Template>(objectName);
                        NetObjInsert(object, true, false);
                        object->calculateCompoundStatus();  // Force status change to NORMAL
                        break;
                     case OBJECT_TEMPLATEGROUP:
                        object = MakeSharedNObject<TemplateGroup>(objectName);
                        NetObjInsert(object, true, false);
                        object->calculateCompoundStatus();	// Force status change to NORMAL
                        break;
                     case OBJECT_VPNCONNECTOR:
                        object = MakeSharedNObject<VPNConnector>(TRUE);
                        object->setName(objectName);
                        NetObjInsert(object, true, false);
                        break;
                     case OBJECT_ZONE:
                        if (zoneUIN == 0)
                           zoneUIN = FindUnusedZoneUIN();
                        if ((zoneUIN > 0) && (zoneUIN != ALL_ZONES) && (FindZoneByUIN(zoneUIN) == nullptr))
                        {
                           object = MakeSharedNObject<Zone>(zoneUIN, objectName);
                           NetObjInsert(object, true, false);
                        }
                        break;
                     default:
                        // Try to create unknown classes by modules
                        ENUMERATE_MODULES(pfCreateObject)
                        {
                           object = CURRENT_MODULE.pfCreateObject(objectClass, objectName, parent, request);
                           if (object != nullptr)
                              break;
                        }
                        break;
                  }

                  // If creation was successful do binding and set comments if needed
                  if (object != nullptr)
                  {
                     json_t *objData = object->toJson();
                     WriteAuditLogWithJsonValues(AUDIT_OBJECTS, true, m_dwUserId, m_workstation, m_id, object->getId(), nullptr, objData,
                        _T("Object %s created (class %s)"), object->getName(), object->getObjectClassName());
                     json_decref(objData);
                     if ((parent != nullptr) &&          // parent can be nullptr for nodes
                         (objectClass != OBJECT_INTERFACE)) // interface already linked by Node::createNewInterface
                     {
                        parent->addChild(object);
                        object->addParent(parent);
                        parent->calculateCompoundStatus();
                        if (parent->getObjectClass() == OBJECT_CLUSTER)
                        {
                           static_cast<Cluster&>(*parent).applyToTarget(static_pointer_cast<DataCollectionTarget>(object));
                        }
                        if (object->getObjectClass() == OBJECT_NODELINK)
                        {
                           static_cast<NodeLink&>(*object).applyTemplates();
                        }
                        else if ((object->getObjectClass() == OBJECT_NETWORKSERVICE) && request->getFieldAsBoolean(VID_CREATE_STATUS_DCI))
                        {
                           TCHAR dciName[MAX_DB_STRING], dciDescription[MAX_DB_STRING];
                           _sntprintf(dciName, MAX_DB_STRING, _T("ChildStatus(%d)"), object->getId());
                           _sntprintf(dciDescription, MAX_DB_STRING, _T("Status of network service %s"), object->getName());
                           static_cast<Node&>(*parent).addDCObject(new DCItem(CreateUniqueId(IDG_ITEM), dciName, DS_INTERNAL, DCI_DT_INT,
                                    nullptr, nullptr, static_pointer_cast<Node>(parent), dciDescription));
                        }
                     }

                     TCHAR *comments = request->getFieldAsString(VID_COMMENTS);
                     if (comments != nullptr)
                     {
                        object->setComments(comments);
                        MemFree(comments);
                     }

                     object->unhide();
                     msg.setField(VID_RCC, RCC_SUCCESS);
                     msg.setField(VID_OBJECT_ID, object->getId());
                  }
                  else
                  {
                     // :DIRTY HACK:
                     // PollNewNode will return nullptr only if IP already
                     // in use. some new() can fail there too, but server will
                     // crash in that case
                     if (objectClass == OBJECT_NODE)
                     {
                        msg.setField(VID_RCC, RCC_ALREADY_EXIST);
                        //Add to description IP of new created node and name of node with the same IP
                        SetNodesConflictString(&msg, zoneUIN, ipAddr);
                     }
                     else if (objectClass == OBJECT_ZONE)
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
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request->getId());

   shared_ptr<NetObj> cluster = FindObjectById(request->getFieldAsUInt32(VID_PARENT_ID));
   shared_ptr<NetObj> node = FindObjectById(request->getFieldAsUInt32(VID_CHILD_ID));
	if ((cluster != nullptr) && (node != nullptr))
	{
		if ((cluster->getObjectClass() == OBJECT_CLUSTER) && (node->getObjectClass() == OBJECT_NODE))
		{
			if (static_cast<Node&>(*node).getMyCluster() == nullptr)
			{
				if (cluster->checkAccessRights(m_dwUserId, OBJECT_ACCESS_MODIFY) &&
					 node->checkAccessRights(m_dwUserId, OBJECT_ACCESS_MODIFY))
				{
					static_cast<Cluster&>(*cluster).applyToTarget(static_pointer_cast<Node>(node));
					static_cast<Node&>(*node).setRecheckCapsFlag();
					static_cast<Node&>(*node).forceConfigurationPoll();

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
   NXCPMessage msg(CMD_REQUEST_COMPLETED, pRequest->getId());

   // Get parent and child objects
   shared_ptr<NetObj> pParent = FindObjectById(pRequest->getFieldAsUInt32(VID_PARENT_ID));
   shared_ptr<NetObj> pChild = FindObjectById(pRequest->getFieldAsUInt32(VID_CHILD_ID));

   // Check access rights and change binding
   if ((pParent != nullptr) && (pChild != nullptr))
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
                  pParent->addChild(pChild);
                  pChild->addParent(pParent);
                  ObjectTransactionEnd();
                  pParent->calculateCompoundStatus();
                  msg.setField(VID_RCC, RCC_SUCCESS);

						if ((pParent->getObjectClass() == OBJECT_BUSINESSSERVICEROOT) || (pParent->getObjectClass() == OBJECT_BUSINESSSERVICE))
						{
							static_cast<ServiceContainer&>(*pParent).initUptimeStats();
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
               pParent->deleteChild(*pChild);
               pChild->deleteParent(*pParent);
               ObjectTransactionEnd();
               pParent->calculateCompoundStatus();
               if ((pParent->getObjectClass() == OBJECT_TEMPLATE) && pChild->isDataCollectionTarget())
               {
                  static_cast<Template&>(*pParent).queueRemoveFromTarget(pChild->getId(), pRequest->getFieldAsBoolean(VID_REMOVE_DCI));
               }
               else if ((pParent->getObjectClass() == OBJECT_CLUSTER) && (pChild->getObjectClass() == OBJECT_NODE))
               {
                  static_cast<Cluster&>(*pParent).queueRemoveFromTarget(pChild->getId(), TRUE);
                  static_cast<Node&>(*pChild).setRecheckCapsFlag();
                  static_cast<Node&>(*pChild).forceConfigurationPoll();
               }
					else if ((pParent->getObjectClass() == OBJECT_BUSINESSSERVICEROOT) || (pParent->getObjectClass() == OBJECT_BUSINESSSERVICE))
					{
						static_cast<ServiceContainer&>(*pParent).initUptimeStats();
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
static void DeleteObjectWorker(const shared_ptr<NetObj>& object)
{
   ObjectTransactionStart();
	object->deleteObject();
   ObjectTransactionEnd();
}

/**
 * Delete object
 */
void ClientSession::deleteObject(NXCPMessage *pRequest)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, pRequest->getId());

   // Find object to be deleted
   shared_ptr<NetObj> object = FindObjectById(pRequest->getFieldAsUInt32(VID_OBJECT_ID));
   if (object != nullptr)
   {
      // Check if it is a built-in object, like "Entire Network"
      if (object->getId() >= 10)  // FIXME: change to 100
      {
         // Check access rights
         if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_DELETE))
         {
				if ((object->getObjectClass() != OBJECT_ZONE) || object->isEmpty())
				{
               ThreadPoolExecute(g_clientThreadPool, DeleteObjectWorker, object);
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
 * Alarm update worker function (executed in thread pool)
 */
void ClientSession::alarmUpdateWorker(Alarm *alarm)
{
   NXCPMessage msg(CMD_ALARM_UPDATE, 0);
   alarm->fillMessage(&msg);
   MutexLock(m_mutexSendAlarms);
   sendMessage(&msg);
   MutexUnlock(m_mutexSendAlarms);
   delete alarm;
}

/**
 * Process changes in alarms
 */
void ClientSession::onAlarmUpdate(UINT32 code, const Alarm *alarm)
{
   if (isAuthenticated() && isSubscribedTo(NXC_CHANNEL_ALARMS))
   {
      shared_ptr<NetObj> object = FindObjectById(alarm->getSourceObject());
      if ((object != nullptr) &&
          object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ_ALARMS) &&
          alarm->checkCategoryAccess(this))
      {
         ThreadPoolExecute(g_clientThreadPool, this, &ClientSession::alarmUpdateWorker, new Alarm(alarm, false, code));
      }
   }
}

/**
 * Send all alarms to client
 */
void ClientSession::getAlarms(NXCPMessage *request)
{
   MutexLock(m_mutexSendAlarms);
   SendAlarmsToClient(request->getId(), this);
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
   shared_ptr<NetObj> object = GetAlarmSourceObject(alarmId);
   if (object != nullptr)
   {
      // User should have "view alarm" right to the object
      if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ_ALARMS))
      {
         msg.setField(VID_RCC, GetAlarm(alarmId, m_dwUserId, &msg, this));
      }
      else
      {
         msg.setField(VID_RCC, RCC_ACCESS_DENIED);
			WriteAuditLog(AUDIT_OBJECTS, FALSE, m_dwUserId, m_workstation, m_id, object->getId(), _T("Access denied on get alarm for object %s"), object->getName());
      }
   }
   else
   {
      // Normally, for existing alarms object will not be nullptr,
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
   shared_ptr<NetObj> object = GetAlarmSourceObject(alarmId);
   if (object != nullptr)
   {
      // User should have "view alarm" right to the object and
		// system-wide "view event log" access
      if ((m_systemAccessRights & SYSTEM_ACCESS_VIEW_EVENT_LOG) && object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ_ALARMS))
      {
         msg.setField(VID_RCC, GetAlarmEvents(alarmId, m_dwUserId, &msg, this));
      }
      else
      {
         msg.setField(VID_RCC, RCC_ACCESS_DENIED);
			WriteAuditLog(AUDIT_OBJECTS, FALSE, m_dwUserId, m_workstation, m_id, object->getId(), _T("Access denied on get alarm events for object %s"), object->getName());
      }
   }
   else
   {
      // Normally, for existing alarms object will not be nullptr,
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
   shared_ptr<NetObj> object;
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
   if (object != nullptr)
   {
      // User should have "acknowledge alarm" right to the object
      if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_UPDATE_ALARMS))
      {
			msg.setField(VID_RCC,
            byHelpdeskRef ?
            AckAlarmByHDRef(hdref, this, pRequest->getFieldAsUInt16(VID_STICKY_FLAG) != 0, pRequest->getFieldAsUInt32(VID_TIMESTAMP)) :
            AckAlarmById(alarmId, this, pRequest->getFieldAsUInt16(VID_STICKY_FLAG) != 0, pRequest->getFieldAsUInt32(VID_TIMESTAMP), false));
      }
      else
      {
         msg.setField(VID_RCC, RCC_ACCESS_DENIED);
			WriteAuditLog(AUDIT_OBJECTS, FALSE, m_dwUserId, m_workstation, m_id, object->getId(), _T("Access denied on acknowledged alarm on object %s"), object->getName());
      }
   }
   else
   {
      // Normally, for existing alarms object will not be nullptr,
      // so we assume that alarm id is invalid
      msg.setField(VID_RCC, RCC_INVALID_ALARM_ID);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Terminate bulk alarms
 */
void ClientSession::bulkResolveAlarms(NXCPMessage *pRequest, bool terminate)
{
   NXCPMessage msg;
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(pRequest->getId());

   IntegerArray<UINT32> alarmIds, failIds, failCodes;
   pRequest->getFieldAsInt32Array(VID_ALARM_ID_LIST, &alarmIds);

   ResolveAlarmsById(&alarmIds, &failIds, &failCodes, this, terminate, false);
   msg.setField(VID_RCC, RCC_SUCCESS);

   if (failIds.size() > 0)
   {
      msg.setFieldFromInt32Array(VID_ALARM_ID_LIST, &failIds);
      msg.setFieldFromInt32Array(VID_FAIL_CODE_LIST, &failCodes);
   }
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

   // Get alarm id and its source object
   UINT32 alarmId;
   TCHAR hdref[MAX_HELPDESK_REF_LEN];
   shared_ptr<NetObj> object;
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
   if (object != nullptr)
   {
      // User should have "terminate alarm" right to the object
      if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_TERM_ALARMS))
      {
         msg.setField(VID_RCC,
            byHelpdeskRef ?
            ResolveAlarmByHDRef(hdref, this, terminate) :
            ResolveAlarmById(alarmId, this, terminate, false));
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
      // Normally, for existing alarms object will not be nullptr,
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
   shared_ptr<NetObj> object = GetAlarmSourceObject(alarmId);
   if (object != nullptr)
   {
      // User should have "terminate alarm" right to the object
      // and system right "delete alarms"
      if ((object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_TERM_ALARMS)) &&
          (m_systemAccessRights & SYSTEM_ACCESS_DELETE_ALARMS))
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
      // Normally, for existing alarms object will not be nullptr,
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
   shared_ptr<NetObj> object = GetAlarmSourceObject(alarmId);
   if (object != nullptr)
   {
      if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_CREATE_ISSUE))
      {
         TCHAR hdref[MAX_HELPDESK_REF_LEN];
         msg.setField(VID_RCC, OpenHelpdeskIssue(alarmId, this, hdref));
         msg.setField(VID_HELPDESK_REF, hdref);
         WriteAuditLog(AUDIT_OBJECTS, TRUE, m_dwUserId, m_workstation, m_id, object->getId(),
            _T("Helpdesk issue created successfully from alarm on object %s"), object->getName());
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
      // Normally, for existing alarms object will not be nullptr,
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
   shared_ptr<NetObj> object = GetAlarmSourceObject(alarmId);
   if (object != nullptr)
   {
      if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ_ALARMS))
      {
         TCHAR url[MAX_PATH];
         msg.setField(VID_RCC, GetHelpdeskIssueUrlFromAlarm(alarmId, m_dwUserId, url, MAX_PATH, this));
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
      // Normally, for existing alarms object will not be nullptr,
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
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request->getId());

   // Get alarm id and it's source object
   UINT32 alarmId;
   TCHAR hdref[MAX_HELPDESK_REF_LEN];
   shared_ptr<NetObj> object;
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
   if (object != nullptr)
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
      // Normally, for existing alarms object will not be nullptr,
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
   shared_ptr<NetObj> object = GetAlarmSourceObject(alarmId);
   if (object != nullptr)
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
      // Normally, for existing alarms object will not be nullptr,
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
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request->getId());

   // Get alarm id and it's source object
   UINT32 alarmId;
   TCHAR hdref[MAX_HELPDESK_REF_LEN];
   shared_ptr<NetObj> object;
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
   if (object != nullptr)
   {
      // User should have "update alarm" right to the object
		if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_UPDATE_ALARMS))
      {
			UINT32 commentId = request->getFieldAsUInt32(VID_COMMENT_ID);
			TCHAR *text = request->getFieldAsString(VID_COMMENTS);
			msg.setField(VID_RCC,
            byHelpdeskRef ?
            AddAlarmComment(hdref, CHECK_NULL(text), m_dwUserId) :
            UpdateAlarmComment(alarmId, &commentId, CHECK_NULL(text), m_dwUserId));
			MemFree(text);
      }
      else
      {
         msg.setField(VID_RCC, RCC_ACCESS_DENIED);
      }
   }
   else
   {
      // Normally, for existing alarms object will not be nullptr,
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
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request->getId());

   // Get alarm id and it's source object
   UINT32 alarmId = request->getFieldAsUInt32(VID_ALARM_ID);
   shared_ptr<NetObj> object = GetAlarmSourceObject(alarmId);
   if (object != nullptr)
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
      // Normally, for existing alarms object will not be nullptr,
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
   if (m_systemAccessRights & SYSTEM_ACCESS_MANAGE_ACTIONS)
   {
      TCHAR actionName[MAX_OBJECT_NAME];
      pRequest->getFieldAsString(VID_ACTION_NAME, actionName, MAX_OBJECT_NAME);
      uint32_t actionId;
      uint32_t rcc = CreateAction(actionName, &actionId);
      msg.setField(VID_RCC, rcc);
      if (rcc == RCC_SUCCESS)
         msg.setField(VID_ACTION_ID, actionId);   // Send id of new action to client
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
   if (m_systemAccessRights & SYSTEM_ACCESS_MANAGE_ACTIONS)
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
   NXCPMessage msg(CMD_REQUEST_COMPLETED, pRequest->getId());

   // Check user rights
   if (m_systemAccessRights & SYSTEM_ACCESS_MANAGE_ACTIONS)
   {
      // Get Id of action to be deleted
      UINT32 actionId = pRequest->getFieldAsUInt32(VID_ACTION_ID);
      if (!g_pEventPolicy->isActionInUse(actionId))
      {
         msg.setField(VID_RCC, DeleteAction(actionId));
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
 * Send action configuration update message
 */
void ClientSession::sendActionDBUpdateMessage(NXCP_MESSAGE *msg)
{
   MutexLock(m_mutexSendActions);
   sendRawMessage(msg);
   MutexUnlock(m_mutexSendActions);
   free(msg);
}

/**
 * Process changes in actions
 */
void ClientSession::onActionDBUpdate(UINT32 dwCode, const Action *action)
{
   if (isAuthenticated() && (checkSysAccessRights(SYSTEM_ACCESS_MANAGE_ACTIONS) || checkSysAccessRights(SYSTEM_ACCESS_EPP)))
   {
      NXCPMessage msg(CMD_ACTION_DB_UPDATE, 0);
      msg.setField(VID_NOTIFICATION_CODE, dwCode);
      msg.setField(VID_ACTION_ID, action->id);
      if (dwCode != NX_NOTIFY_ACTION_DELETED)
         action->fillMessage(&msg);
      ThreadPoolExecute(g_clientThreadPool, this, &ClientSession::sendActionDBUpdateMessage,
               msg.serialize((m_flags & CSF_COMPRESSION_ENABLED) != 0));
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
   if ((m_systemAccessRights & SYSTEM_ACCESS_MANAGE_ACTIONS) ||
       (m_systemAccessRights & SYSTEM_ACCESS_EPP))
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

/**
 * Perform a forced node poll
 */
void ClientSession::forcedNodePoll(NXCPMessage *pRequest)
{
   MutexLock(m_mutexPollerInit);

   NXCPMessage response(CMD_REQUEST_COMPLETED, pRequest->getId());

   int pollType = pRequest->getFieldAsUInt16(VID_POLL_TYPE);

   // Find object to be polled
   shared_ptr<NetObj> object = FindObjectById(pRequest->getFieldAsUInt32(VID_OBJECT_ID));
   if (object != nullptr)
   {
      // We can do polls for node, sensor, cluster objects
      if (((object->getObjectClass() == OBJECT_NODE) &&
          ((pollType == POLL_CONFIGURATION_FULL) ||
			  (pollType == POLL_TOPOLOGY) ||
			  (pollType == POLL_INTERFACE_NAMES)))
			  || (object->isDataCollectionTarget() &&
          ((pollType == POLL_STATUS) ||
			  (pollType == POLL_CONFIGURATION_NORMAL) ||
			  (pollType == POLL_INSTANCE_DISCOVERY))))
      {
         // Check access rights
         if (((pollType == POLL_CONFIGURATION_FULL) && object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_MODIFY)) ||
               ((pollType != POLL_CONFIGURATION_FULL) && object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ)))
         {
            InterlockedIncrement(&m_refCount);

            ThreadPoolExecute(g_clientThreadPool, this, &ClientSession::pollerThread,
                     static_pointer_cast<DataCollectionTarget>(object), pollType, pRequest->getId());

            NXCPMessage msg(CMD_POLLING_INFO, pRequest->getId());
            msg.setField(VID_RCC, RCC_OPERATION_IN_PROGRESS);
            msg.setField(VID_POLLER_MESSAGE, _T("Poll request accepted, waiting for outstanding polling requests to complete...\r\n"));
            sendMessage(&msg);

            response.setField(VID_RCC, RCC_SUCCESS);
         }
         else
         {
            response.setField(VID_RCC, RCC_ACCESS_DENIED);
         }
      }
      else
      {
         response.setField(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
      }
   }
   else
   {
      response.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   // Send response
   sendMessage(&response);
   MutexUnlock(m_mutexPollerInit);
}

/**
 * Send message from poller to client
 */
void ClientSession::sendPollerMsg(uint32_t requestId, const TCHAR *text)
{
   NXCPMessage msg(CMD_POLLING_INFO, requestId);
   msg.setField(VID_RCC, RCC_OPERATION_IN_PROGRESS);
   msg.setField(VID_POLLER_MESSAGE, text);
   sendMessage(&msg);
}

/**
 * Node poller thread
 */
void ClientSession::pollerThread(shared_ptr<DataCollectionTarget> object, int pollType, uint32_t requestId)
{
   // Wait while parent thread finishes initialization
   MutexLock(m_mutexPollerInit);
   MutexUnlock(m_mutexPollerInit);

   switch(pollType)
   {
      case POLL_STATUS:
         object->startForcedStatusPoll();
         object->statusPollWorkerEntry(RegisterPoller(PollerType::STATUS, object), this, requestId);
         break;
      case POLL_CONFIGURATION_FULL:
         if (object->getObjectClass() == OBJECT_NODE)
            static_cast<Node&>(*object).setRecheckCapsFlag();
         // intentionally no break here
      case POLL_CONFIGURATION_NORMAL:
         object->startForcedConfigurationPoll();
         object->configurationPollWorkerEntry(RegisterPoller(PollerType::CONFIGURATION, object), this, requestId);
         break;
      case POLL_INSTANCE_DISCOVERY:
         object->startForcedInstancePoll();
         object->instanceDiscoveryPollWorkerEntry(RegisterPoller(PollerType::INSTANCE_DISCOVERY, object), this, requestId);
         break;
      case POLL_TOPOLOGY:
         if (object->getObjectClass() == OBJECT_NODE)
         {
            static_cast<Node&>(*object).startForcedTopologyPoll();
            static_cast<Node&>(*object).topologyPollWorkerEntry(RegisterPoller(PollerType::TOPOLOGY, object), this, requestId);
         }
         break;
      case POLL_INTERFACE_NAMES:
         if (object->getObjectClass() == OBJECT_NODE)
         {
            static_cast<Node&>(*object).updateInterfaceNames(this, requestId);
         }
         break;
      default:
         sendPollerMsg(requestId, _T("Invalid poll type requested\r\n"));
         break;
   }

   NXCPMessage msg(CMD_POLLING_INFO, requestId);
   msg.setField(VID_RCC, RCC_SUCCESS);
   sendMessage(&msg);

   decRefCount();
}

/**
 * Receive event from user
 */
void ClientSession::onTrap(NXCPMessage *pRequest)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, pRequest->getId());

   // Find event's source object
   uint32_t objectId = pRequest->getFieldAsUInt32(VID_OBJECT_ID);
   shared_ptr<NetObj> object;
   if (objectId != 0)
	{
      object = FindObjectById(objectId);  // Object is specified explicitely
	}
   else   // Client is the source
	{
      if (m_clientAddr.isLoopback())
      {
			object = FindObjectById(g_dwMgmtNode);
      }
      else
      {
			object = FindNodeByIP(0, m_clientAddr);
      }
	}

   if (object != nullptr)
   {
      // User should have SEND_EVENTS access right to object
      if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_SEND_EVENTS))
      {
         uint32_t eventCode = pRequest->getFieldAsUInt32(VID_EVENT_CODE);
			if ((eventCode == 0) && pRequest->isFieldExist(VID_EVENT_NAME))
			{
				TCHAR eventName[256];
				pRequest->getFieldAsString(VID_EVENT_NAME, eventName, 256);
				eventCode = EventCodeFromName(eventName, 0);
			}
		   TCHAR userTag[MAX_USERTAG_LENGTH] = _T("");
			pRequest->getFieldAsString(VID_USER_TAG, userTag, MAX_USERTAG_LENGTH);
			StrStrip(userTag);
			StringList parameters(pRequest, VID_EVENT_ARG_BASE, VID_NUM_ARGS);
         bool success = PostEventWithTag(eventCode, EventOrigin::CLIENT, pRequest->getFieldAsTime(VID_ORIGIN_TIMESTAMP), object->getId(), userTag, parameters);
         msg.setField(VID_RCC, success ? RCC_SUCCESS : RCC_INVALID_EVENT_CODE);
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
 * Wake up node
 */
void ClientSession::onWakeUpNode(NXCPMessage *pRequest)
{
   NXCPMessage msg;

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(pRequest->getId());

   // Find node or interface object
   shared_ptr<NetObj> object = FindObjectById(pRequest->getFieldAsUInt32(VID_OBJECT_ID));
   if (object != nullptr)
   {
      if ((object->getObjectClass() == OBJECT_NODE) ||
          (object->getObjectClass() == OBJECT_INTERFACE))
      {
         if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_CONTROL))
         {
            UINT32 dwResult;
            if (object->getObjectClass() == OBJECT_NODE)
               dwResult = static_cast<Node&>(*object).wakeUp();
            else
               dwResult = static_cast<Interface&>(*object).wakeUp();
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
   shared_ptr<NetObj> object = FindObjectById(pRequest->getFieldAsUInt32(VID_OBJECT_ID));
   if (object != nullptr)
   {
      if (object->getObjectClass() == OBJECT_NODE)
      {
         TCHAR value[256], name[MAX_PARAM_NAME];
         pRequest->getFieldAsString(VID_NAME, name, MAX_PARAM_NAME);
         uint32_t rcc = static_cast<Node&>(*object).getMetricForClient(pRequest->getFieldAsUInt16(VID_DCI_SOURCE_TYPE), m_dwUserId, name, value, 256);
         msg.setField(VID_RCC, rcc);
         if (rcc == RCC_SUCCESS)
            msg.setField(VID_VALUE, value);
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
   shared_ptr<NetObj> object = FindObjectById(pRequest->getFieldAsUInt32(VID_OBJECT_ID));
   if (object != nullptr)
   {
      if (object->getObjectClass() == OBJECT_NODE)
      {
         TCHAR name[MAX_PARAM_NAME];
         pRequest->getFieldAsString(VID_NAME, name, MAX_PARAM_NAME);

         // Allow access to agent table Agent.SessionAgents if user has access rights for taking screenshots
         // Data from this table required by client to determine correct UI session name
         if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ_AGENT) ||
             (!_tcsicmp(name, _T("Agent.SessionAgents")) && object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_SCREENSHOT)))
         {

				Table *table;
				UINT32 rcc = static_cast<Node&>(*object).getTableForClient(name, &table);
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
      CreateTrapCfgMessage(&msg);
   }
   else
   {
      // Current user has no rights for trap management
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);
   }

   sendMessage(&msg);
}

/**
 * Send list of all installed packages to client
 */
void ClientSession::getInstalledPackages(UINT32 requestId)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, requestId);

   BOOL bSuccess = FALSE;
   TCHAR szBuffer[MAX_DB_STRING];

   if (m_systemAccessRights & SYSTEM_ACCESS_MANAGE_PACKAGES)
   {
      DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
      DB_UNBUFFERED_RESULT hResult = DBSelectUnbuffered(hdb, _T("SELECT pkg_id,version,platform,pkg_file,pkg_name,description FROM agent_pkg"));
      if (hResult != nullptr)
      {
         msg.setField(VID_RCC, RCC_SUCCESS);
         sendMessage(&msg);
         bSuccess = TRUE;

         msg.setCode(CMD_PACKAGE_INFO);
         msg.deleteAllFields();

         while(DBFetch(hResult))
         {
            msg.setField(VID_PACKAGE_ID, DBGetFieldULong(hResult, 0));
            msg.setField(VID_PACKAGE_VERSION, DBGetField(hResult, 1, szBuffer, MAX_DB_STRING));
            msg.setField(VID_PLATFORM_NAME, DBGetField(hResult, 2, szBuffer, MAX_DB_STRING));
            msg.setField(VID_FILE_NAME, DBGetField(hResult, 3, szBuffer, MAX_DB_STRING));
            msg.setField(VID_PACKAGE_NAME, DBGetField(hResult, 4, szBuffer, MAX_DB_STRING));
            msg.setField(VID_DESCRIPTION, DBGetField(hResult, 5, szBuffer, MAX_DB_STRING));
            sendMessage(&msg);
            msg.deleteAllFields();
         }

         msg.setField(VID_PACKAGE_ID, (UINT32)0);
         sendMessage(&msg);

         DBFreeResult(hResult);
      }
      else
      {
         msg.setField(VID_RCC, RCC_DB_FAILURE);
      }
      DBConnectionPoolReleaseConnection(hdb);
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
void ClientSession::installPackage(NXCPMessage *request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request->getId());

   if (m_systemAccessRights & SYSTEM_ACCESS_MANAGE_PACKAGES)
   {
      TCHAR szPkgName[MAX_PACKAGE_NAME_LEN], szDescription[MAX_DB_STRING];
      TCHAR szPkgVersion[MAX_AGENT_VERSION_LEN], szFileName[MAX_DB_STRING];
      TCHAR szPlatform[MAX_PLATFORM_NAME_LEN];

      request->getFieldAsString(VID_PACKAGE_NAME, szPkgName, MAX_PACKAGE_NAME_LEN);
      request->getFieldAsString(VID_DESCRIPTION, szDescription, MAX_DB_STRING);
      request->getFieldAsString(VID_FILE_NAME, szFileName, MAX_DB_STRING);
      request->getFieldAsString(VID_PACKAGE_VERSION, szPkgVersion, MAX_AGENT_VERSION_LEN);
      request->getFieldAsString(VID_PLATFORM_NAME, szPlatform, MAX_PLATFORM_NAME_LEN);

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
               TCHAR fullFileName[MAX_PATH];
               _tcscpy(fullFileName, g_netxmsdDataDir);
               _tcscat(fullFileName, DDIR_PACKAGES);
               _tcscat(fullFileName, FS_PATH_SEPARATOR);
               _tcscat(fullFileName, pszCleanFileName);

               ServerDownloadFileInfo *fInfo = new ServerDownloadFileInfo(fullFileName, CMD_INSTALL_PACKAGE);
               if (fInfo->open())
               {
                  uint32_t uploadData = CreateUniqueId(IDG_PACKAGE);
                  fInfo->setUploadData(uploadData);
                  m_downloadFileMap->set(request->getId(), fInfo);
                  msg.setField(VID_RCC, RCC_SUCCESS);
                  msg.setField(VID_PACKAGE_ID, uploadData);

                  // Create record in database
                  fInfo->updateAgentPkgDBInfo(szDescription, szPkgName, szPkgVersion, szPlatform, pszCleanFileName);
               }
               else
               {
                  delete fInfo;
                  msg.setField(VID_RCC, RCC_IO_ERROR);
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
      // Current user has no rights for package management
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);
   }

   // Send response
   sendMessage(&msg);
}

/**
 * Remove package from server
 */
void ClientSession::removePackage(NXCPMessage *request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request->getId());

   if (m_systemAccessRights & SYSTEM_ACCESS_MANAGE_PACKAGES)
   {
      UINT32 pkgId = request->getFieldAsUInt32(VID_PACKAGE_ID);
      msg.setField(VID_RCC, UninstallPackage(pkgId));
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
void ClientSession::getParametersList(NXCPMessage *request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request->getId());

   shared_ptr<NetObj> object = FindObjectById(request->getFieldAsUInt32(VID_OBJECT_ID));
   if (object != nullptr)
   {
      int origin = request->isFieldExist(VID_DCI_SOURCE_TYPE) ? request->getFieldAsInt16(VID_DCI_SOURCE_TYPE) : DS_NATIVE_AGENT;
      switch(object->getObjectClass())
      {
         case OBJECT_NODE:
            msg.setField(VID_RCC, RCC_SUCCESS);
				static_cast<Node&>(*object).writeParamListToMessage(&msg, origin, request->getFieldAsUInt16(VID_FLAGS));
            break;
         case OBJECT_CLUSTER:
         case OBJECT_TEMPLATE:
            msg.setField(VID_RCC, RCC_SUCCESS);
            WriteFullParamListToMessage(&msg, origin, request->getFieldAsUInt16(VID_FLAGS));
            break;
         case OBJECT_CHASSIS:
            if (static_cast<Chassis&>(*object).getControllerId() != 0)
            {
               shared_ptr<NetObj> controller = FindObjectById(static_cast<Chassis&>(*object).getControllerId(), OBJECT_NODE);
               if (controller != nullptr)
                  static_cast<Node&>(*controller).writeParamListToMessage(&msg, origin, request->getFieldAsUInt16(VID_FLAGS));
            }
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
void ClientSession::deployPackage(NXCPMessage *request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request->getId());
   PackageDeploymentTask *task = nullptr;

   if (m_systemAccessRights & SYSTEM_ACCESS_MANAGE_PACKAGES)
   {
      bool success = true;

      // Get package ID
      uint32_t packageId = request->getFieldAsUInt32(VID_PACKAGE_ID);
      if (IsValidPackageId(packageId))
      {
         DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

         // Read package information
         TCHAR query[256];
         _sntprintf(query, 256, _T("SELECT platform,pkg_file,version FROM agent_pkg WHERE pkg_id=%u"), packageId);
         DB_RESULT hResult = DBSelect(hdb, query);
         if (hResult != nullptr)
         {
            if (DBGetNumRows(hResult) > 0)
            {
               TCHAR szPkgFile[MAX_PATH], szVersion[MAX_AGENT_VERSION_LEN], szPlatform[MAX_PLATFORM_NAME_LEN];
               DBGetField(hResult, 0, 0, szPlatform, MAX_PLATFORM_NAME_LEN);
               DBGetField(hResult, 0, 1, szPkgFile, MAX_PATH);
               DBGetField(hResult, 0, 2, szVersion, MAX_AGENT_VERSION_LEN);

               // Create list of nodes to be upgraded
               IntegerArray<uint32_t> objectList;
               request->getFieldAsInt32Array(VID_OBJECT_LIST, &objectList);
		         task = new PackageDeploymentTask(this, request->getId(), packageId, szPlatform, szPkgFile, szVersion);
               for(int i = 0; i < objectList.size(); i++)
               {
                  shared_ptr<NetObj> object = FindObjectById(objectList.get(i));
                  if (object != nullptr)
                  {
                     if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
                     {
                        if (object->getObjectClass() == OBJECT_NODE)
                        {
                           // Check if this node already in the list
									int j;
                           for(j = 0; j < task->nodeList.size(); j++)
                              if (task->nodeList.get(j)->getId() == objectList.get(i))
                                 break;
                           if (j == task->nodeList.size())
                           {
                              task->nodeList.add(static_pointer_cast<Node>(object));
                           }
                        }
                        else
                        {
                           object->addChildNodesToList(&task->nodeList, m_dwUserId);
                        }
                     }
                     else
                     {
                        msg.setField(VID_RCC, RCC_ACCESS_DENIED);
                        success = false;
                        break;
                     }
                  }
                  else
                  {
                     msg.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
                     success = false;
                     break;
                  }
               }
            }
            else
            {
               msg.setField(VID_RCC, RCC_DB_FAILURE);
               success = false;
            }
            DBFreeResult(hResult);
         }
         else
         {
            msg.setField(VID_RCC, RCC_DB_FAILURE);
            success = false;
         }
         DBConnectionPoolReleaseConnection(hdb);
      }
      else
      {
         msg.setField(VID_RCC, RCC_INVALID_PACKAGE_ID);
         success = false;
      }

      // On success, start upgrade thread
      if (success)
      {
         msg.setField(VID_RCC, RCC_SUCCESS);
      }
      else
      {
         delete_and_null(task);
      }
   }
   else
   {
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);
   }

   // Send response
   sendMessage(&msg);

   // Start deployment thread
   if (task != nullptr)
   {
      InterlockedIncrement(&m_refCount);
      ThreadCreate(DeploymentManager, task);
   }
}

/**
 * Apply data collection template to node or mobile device
 */
void ClientSession::applyTemplate(NXCPMessage *pRequest)
{
   NXCPMessage msg;

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(pRequest->getId());

   // Get source and destination
   shared_ptr<NetObj> pSource = FindObjectById(pRequest->getFieldAsUInt32(VID_SOURCE_OBJECT_ID));
   shared_ptr<NetObj> pDestination = FindObjectById(pRequest->getFieldAsUInt32(VID_DESTINATION_OBJECT_ID));
   if ((pSource != nullptr) && (pDestination != nullptr))
   {
      // Check object types
      if ((pSource->getObjectClass() == OBJECT_TEMPLATE) && pDestination->isDataCollectionTarget())
      {
         // Check access rights
         if ((pSource->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ)) &&
             (pDestination->checkAccessRights(m_dwUserId, OBJECT_ACCESS_MODIFY)))
         {
            ObjectTransactionStart();
            bool bErrors = static_cast<Template&>(*pSource).applyToTarget(static_pointer_cast<DataCollectionTarget>(pDestination));
            ObjectTransactionEnd();
            static_cast<DataCollectionOwner&>(*pDestination).applyDCIChanges(false);
            msg.setField(VID_RCC, bErrors ? RCC_DCI_COPY_ERRORS : RCC_SUCCESS);
         }
         else  // User doesn't have enough rights on object(s)
         {
            msg.setField(VID_RCC, RCC_ACCESS_DENIED);
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
   NXCPMessage msg(CMD_REQUEST_COMPLETED, pRequest->getId());

   uint32_t dwUserId = pRequest->isFieldExist(VID_USER_ID) ? pRequest->getFieldAsUInt32(VID_USER_ID) : m_dwUserId;
   if ((dwUserId == m_dwUserId) || (m_systemAccessRights & SYSTEM_ACCESS_MANAGE_USERS))
   {
      DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

      // Try to read variable from database
      DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT var_value FROM user_profiles WHERE user_id=? AND var_name=?"));
      if (hStmt != nullptr)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, dwUserId);
         DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, pRequest->getFieldAsString(VID_NAME, nullptr, 0), DB_BIND_DYNAMIC);
         DB_RESULT hResult = DBSelectPrepared(hStmt);
         if (hResult != nullptr)
         {
            if (DBGetNumRows(hResult) > 0)
            {
               TCHAR *value = DBGetField(hResult, 0, 0, nullptr, 0);
               msg.setField(VID_RCC, RCC_SUCCESS);
               msg.setField(VID_VALUE, value);
               MemFree(value);
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

   sendMessage(&msg);
}

/**
 * Set user variable
 */
void ClientSession::setUserVariable(NXCPMessage *pRequest)
{
   NXCPMessage msg;
   TCHAR szVarName[MAX_USERVAR_NAME_LENGTH];
   DB_RESULT hResult;
   BOOL bExist = FALSE;
   UINT32 dwUserId;

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(pRequest->getId());

   dwUserId = pRequest->isFieldExist(VID_USER_ID) ? pRequest->getFieldAsUInt32(VID_USER_ID) : m_dwUserId;
   if ((dwUserId == m_dwUserId) || (m_systemAccessRights & SYSTEM_ACCESS_MANAGE_USERS))
   {
      // Check variable name
      pRequest->getFieldAsString(VID_NAME, szVarName, MAX_USERVAR_NAME_LENGTH);
      if (IsValidObjectName(szVarName))
      {
         DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

         // Check if variable already exist in database
         DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT var_name FROM user_profiles WHERE user_id=? AND var_name=?"));
         if (hStmt != nullptr)
         {
            DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, dwUserId);
            DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, szVarName, DB_BIND_STATIC, MAX_USERVAR_NAME_LENGTH - 1);
            hResult = DBSelectPrepared(hStmt);
            if (hResult != nullptr)
            {
               if (DBGetNumRows(hResult) > 0)
                  bExist = TRUE;
               DBFreeResult(hResult);
            }
            DBFreeStatement(hStmt);
         }
         else
            msg.setField(VID_RCC, RCC_DB_FAILURE);

         // Update value in database
         if (bExist)
            hStmt = DBPrepare(hdb, _T("UPDATE user_profiles SET var_value=? WHERE user_id=? AND var_name=?"));
         else
            hStmt = DBPrepare(hdb, _T("INSERT INTO user_profiles (var_value,user_id,var_name) VALUES (?,?,?)"));

         if (hStmt != nullptr)
         {
            DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, dwUserId);
            DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, szVarName, DB_BIND_STATIC, MAX_USERVAR_NAME_LENGTH - 1);
            DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, pRequest->getFieldAsString(VID_VALUE), DB_BIND_DYNAMIC);

            if (DBExecute(hStmt))
               msg.setField(VID_RCC, RCC_SUCCESS);
            else
               msg.setField(VID_RCC, RCC_DB_FAILURE);
            DBFreeStatement(hStmt);
         }
         else
            msg.setField(VID_RCC, RCC_DB_FAILURE);

         DBConnectionPoolReleaseConnection(hdb);
      }
      else
         msg.setField(VID_RCC, RCC_INVALID_OBJECT_NAME);
   }
   else
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);

   // Send response
   sendMessage(&msg);
}

/**
 * Enum user variables
 */
void ClientSession::enumUserVariables(NXCPMessage *pRequest)
{
   NXCPMessage msg;
   TCHAR szPattern[MAX_USERVAR_NAME_LENGTH], szQuery[256], szName[MAX_DB_STRING];
   UINT32 i, dwNumRows, dwNumVars, dwId, dwUserId;
   DB_RESULT hResult;

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(pRequest->getId());

   dwUserId = pRequest->isFieldExist(VID_USER_ID) ? pRequest->getFieldAsUInt32(VID_USER_ID) : m_dwUserId;
   if ((dwUserId == m_dwUserId) || (m_systemAccessRights & SYSTEM_ACCESS_MANAGE_USERS))
   {
      pRequest->getFieldAsString(VID_SEARCH_PATTERN, szPattern, MAX_USERVAR_NAME_LENGTH);
      DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
      _sntprintf(szQuery, 256, _T("SELECT var_name FROM user_profiles WHERE user_id=%d"), dwUserId);
      hResult = DBSelect(hdb, szQuery);
      if (hResult != nullptr)
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
 * Delete user variable(s)
 */
void ClientSession::deleteUserVariable(NXCPMessage *pRequest)
{
   NXCPMessage msg;
   TCHAR szVarName[MAX_USERVAR_NAME_LENGTH];
   UINT32 dwUserId;

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(pRequest->getId());

   dwUserId = pRequest->isFieldExist(VID_USER_ID) ? pRequest->getFieldAsUInt32(VID_USER_ID) : m_dwUserId;
   if ((dwUserId == m_dwUserId) || (m_systemAccessRights & SYSTEM_ACCESS_MANAGE_USERS))
   {
      DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

      // Try to delete variable from database
      pRequest->getFieldAsString(VID_NAME, szVarName, MAX_USERVAR_NAME_LENGTH);
      TranslateStr(szVarName, _T("*"), _T("%"));
      DB_STATEMENT hStmt = DBPrepare(hdb, _T("DELETE FROM user_profiles WHERE user_id=? AND var_name LIKE ?"));
      if (hStmt != nullptr)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, dwUserId);
         DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, szVarName, DB_BIND_STATIC, MAX_USERVAR_NAME_LENGTH - 1);

         if (DBExecute(hStmt))
            msg.setField(VID_RCC, RCC_SUCCESS);
         else
            msg.setField(VID_RCC, RCC_DB_FAILURE);

         DBFreeStatement(hStmt);
      }
      else
         msg.setField(VID_RCC, RCC_DB_FAILURE);
      DBConnectionPoolReleaseConnection(hdb);
   }
   else
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);

   // Send response
   sendMessage(&msg);
}

/**
 * Copy or move user variable(s) to another user
 */
void ClientSession::copyUserVariable(NXCPMessage *pRequest)
{
   NXCPMessage msg;
   TCHAR szVarName[MAX_USERVAR_NAME_LENGTH], szCurrVar[MAX_USERVAR_NAME_LENGTH];
   UINT32 dwSrcUserId, dwDstUserId;
   int i, nRows;
   BOOL bMove, bExist;
   DB_RESULT hResult, hResult2;

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(pRequest->getId());

   if (m_systemAccessRights & SYSTEM_ACCESS_MANAGE_USERS)
   {
      DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

      dwSrcUserId = pRequest->isFieldExist(VID_USER_ID) ? pRequest->getFieldAsUInt32(VID_USER_ID) : m_dwUserId;
      dwDstUserId = pRequest->getFieldAsUInt32(VID_DST_USER_ID);
      bMove = (BOOL)pRequest->getFieldAsUInt16(VID_MOVE_FLAG);
      pRequest->getFieldAsString(VID_NAME, szVarName, MAX_USERVAR_NAME_LENGTH);
      TranslateStr(szVarName, _T("*"), _T("%"));
      DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT var_name,var_value FROM user_profiles WHERE user_id=? AND var_name LIKE ?"));
      if (hStmt != nullptr)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, dwSrcUserId);
         DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, szVarName, DB_BIND_STATIC, MAX_USERVAR_NAME_LENGTH - 1);

         hResult = DBSelectPrepared(hStmt);
         DBFreeStatement(hStmt);
         if (hResult != nullptr)
         {
            nRows = DBGetNumRows(hResult);
            for(i = 0; i < nRows; i++)
            {
               DBGetField(hResult, i, 0, szCurrVar, MAX_USERVAR_NAME_LENGTH);

               // Check if variable already exist in database
               hStmt = DBPrepare(hdb, _T("SELECT var_name FROM user_profiles WHERE user_id=? AND var_name=?"));
               if (hStmt != nullptr)
               {
                  DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, dwDstUserId);
                  DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, szCurrVar, DB_BIND_STATIC, MAX_USERVAR_NAME_LENGTH - 1);

                  hResult2 = DBSelectPrepared(hStmt);
                  if (hResult2 != nullptr)
                  {
                     bExist = (DBGetNumRows(hResult2) > 0);
                     DBFreeResult(hResult2);
                  }
                  else
                  {
                     bExist = FALSE;
                  }
                  DBFreeStatement(hStmt);
               }
               else
               {
                  msg.setField(VID_RCC, RCC_DB_FAILURE);
                  break;
               }

               if (bExist)
                  hStmt = DBPrepare(hdb, _T("UPDATE user_profiles SET var_value=? WHERE user_id=? AND var_name=?"));
               else
                  hStmt = DBPrepare(hdb, _T("INSERT INTO user_profiles (var_value,user_id,var_name) VALUES (?,?,?)"));

               if (hStmt != nullptr)
               {
                  DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, DBGetField(hResult, i, 1, nullptr, 0), DB_BIND_DYNAMIC);
                  DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, dwDstUserId);
                  DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, szCurrVar, DB_BIND_STATIC, MAX_USERVAR_NAME_LENGTH - 1);

                  DBExecute(hStmt);
                  DBFreeStatement(hStmt);
               }
               else
               {
                  msg.setField(VID_RCC, RCC_DB_FAILURE);
                  break;
               }

               if (bMove)
               {
                  hStmt = DBPrepare(hdb, _T("DELETE FROM user_profiles WHERE user_id=? AND var_name=?"));
                  if (hStmt != nullptr)
                  {
                     DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, dwSrcUserId);
                     DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, szCurrVar, DB_BIND_STATIC, MAX_USERVAR_NAME_LENGTH - 1);

                     DBExecute(hStmt);
                     DBFreeStatement(hStmt);
                  }
                  else
                  {
                     msg.setField(VID_RCC, RCC_DB_FAILURE);
                     break;
                  }
               }

               writeAuditLog(AUDIT_SECURITY, true, 0, _T("User variable %s %s from [%d] to [%d]"),
                        szCurrVar, bMove ? _T("moved") : _T("copied"), dwSrcUserId, dwDstUserId);
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
         msg.setField(VID_RCC, RCC_DB_FAILURE);
      }
      DBConnectionPoolReleaseConnection(hdb);
   }
   else
   {
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);
      writeAuditLog(AUDIT_SECURITY, false, 0, _T("Access denied on copy user variable"));
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
   shared_ptr<NetObj> object = FindObjectById(pRequest->getFieldAsUInt32(VID_OBJECT_ID));
   if (object != nullptr)
   {
      if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_MODIFY))
      {
			if (object->getObjectClass() == OBJECT_NODE)
			{
			   int32_t zoneUIN = pRequest->getFieldAsUInt32(VID_ZONE_UIN);
				shared_ptr<Zone> zone = FindZoneByUIN(zoneUIN);
				if (zone != nullptr)
				{
					// Check if target zone already have object with same primary IP
					if ((static_cast<Node&>(*object).getFlags() & NF_EXTERNAL_GATEWAY) ||
					    ((FindNodeByIP(zoneUIN, static_cast<Node&>(*object).getIpAddress()) == nullptr) &&
						  (FindSubnetByIP(zoneUIN, static_cast<Node&>(*object).getIpAddress()) == nullptr)))
					{
					   static_cast<Node&>(*object).changeZone(zoneUIN);
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
	m_encryptionRqId = request->getId();
   m_encryptionResult = RCC_TIMEOUT;
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
   msg.setField(VID_RCC, m_encryptionResult);
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
void ClientSession::readAgentConfigFile(NXCPMessage *request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request->getId());

   // Get object id and check prerequisites
   shared_ptr<NetObj> object = FindObjectById(request->getFieldAsUInt32(VID_OBJECT_ID));
   if (object != nullptr)
   {
      if (object->getObjectClass() == OBJECT_NODE)
      {
         if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ_AGENT))
         {
            shared_ptr<AgentConnectionEx> pConn = static_cast<Node&>(*object).createAgentConnection();
            if (pConn != nullptr)
            {
               TCHAR *content;
               size_t size;
               uint32_t dwResult = pConn->readConfigFile(&content, &size);
               switch(dwResult)
               {
                  case ERR_SUCCESS:
                     msg.setField(VID_RCC, RCC_SUCCESS);
                     msg.setField(VID_CONFIG_FILE, content);
                     MemFree(content);
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
 * Write agent's configuration file
 */
void ClientSession::writeAgentConfigFile(NXCPMessage *request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request->getId());

   // Get object id and check prerequisites
   shared_ptr<NetObj> object = FindObjectById(request->getFieldAsUInt32(VID_OBJECT_ID));
   if (object != nullptr)
   {
      if (object->getObjectClass() == OBJECT_NODE)
      {
         if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_CONTROL))
         {
            shared_ptr<AgentConnectionEx> pConn = static_cast<Node&>(*object).createAgentConnection();
            if (pConn != nullptr)
            {
               TCHAR *content = request->getFieldAsString(VID_CONFIG_FILE);
               uint32_t agentRCC = pConn->writeConfigFile(content);
               MemFree(content);

               if ((request->getFieldAsUInt16(VID_APPLY_FLAG) != 0) && (agentRCC == ERR_SUCCESS))
               {
                  StringList list;
                  agentRCC = pConn->execAction(_T("Agent.Restart"), list);
               }

               switch(agentRCC)
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
void ClientSession::executeAction(NXCPMessage *request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request->getId());

   // Get object id and check prerequisites
   shared_ptr<NetObj> object = FindObjectById(request->getFieldAsUInt32(VID_OBJECT_ID));
   if (object != nullptr)
   {
      if (object->getObjectClass() == OBJECT_NODE)
      {
         TCHAR action[MAX_PARAM_NAME];
         TCHAR originalActionString[MAX_PARAM_NAME];
         request->getFieldAsString(VID_ACTION_NAME, action, MAX_PARAM_NAME);
         _tcsncpy(originalActionString, action, MAX_PARAM_NAME);
         bool expandString = request->getFieldAsBoolean(VID_EXPAND_STRING);

         if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_CONTROL))
         {
            shared_ptr<AgentConnectionEx> pConn = static_cast<Node&>(*object).createAgentConnection();
            if (pConn != nullptr)
            {
               StringList *list = nullptr;
               StringMap inputFields;
               Alarm *alarm = nullptr;
               if (expandString)
               {
                  inputFields.loadMessage(request, VID_INPUT_FIELD_COUNT, VID_INPUT_FIELD_BASE);
                  alarm = FindAlarmById(request->getFieldAsUInt32(VID_ALARM_ID));
                  if ((alarm != nullptr) && !object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ_ALARMS) && !alarm->checkCategoryAccess(this))
                  {
                     msg.setField(VID_RCC, RCC_ACCESS_DENIED);
                     sendMessage(&msg);
                     delete alarm;
                     return;
                  }
                  list = SplitCommandLine(object->expandText(action, alarm, nullptr, shared_ptr<DCObjectInfo>(), m_loginName, nullptr, &inputFields, nullptr));
                  _tcslcpy(action, list->get(0), MAX_PARAM_NAME);
                  list->remove(0);
               }
               else
               {
                  int argc = request->getFieldAsInt16(VID_NUM_ARGS);
                  if (argc > 64)
                  {
                     debugPrintf(4, _T("executeAction: too many arguments (%d)"), argc);
                     argc = 64;
                  }

                  list = new StringList();
                  uint32_t fieldId = VID_ACTION_ARG_BASE;
                  for(int i = 0; i < argc; i++)
                     list->addPreallocated(request->getFieldAsString(fieldId++));
               }

               uint32_t rcc;
               bool withOutput = request->getFieldAsBoolean(VID_RECEIVE_OUTPUT);
               if (withOutput)
               {
                  ActionExecutionData data(this, request->getId());
                  rcc = pConn->execAction(action, *list, true, ActionExecuteCallback, &data);
               }
               else
               {
                  rcc = pConn->execAction(action, *list);
               }
               debugPrintf(4, _T("executeAction: rcc=%d"), rcc);

               if (expandString && (request->getFieldAsInt32(VID_NUM_MASKED_FIELDS) > 0))
               {
                  delete list;
                  StringList maskedFields(request, VID_MASKED_FIELD_LIST_BASE, VID_NUM_MASKED_FIELDS);
                  for (int i = 0; i < maskedFields.size(); i++)
                  {
                     inputFields.set(maskedFields.get(i), _T("******"));
                  }
                  list = SplitCommandLine(object->expandText(originalActionString, alarm, nullptr, shared_ptr<DCObjectInfo>(), m_loginName, nullptr, &inputFields, nullptr));
                  list->remove(0);
               }

               StringBuffer args;
               args.appendPreallocated(list->join(_T(", ")));

               switch(rcc)
               {
                  case ERR_SUCCESS:
                     msg.setField(VID_RCC, RCC_SUCCESS);
                     writeAuditLog(AUDIT_OBJECTS, true, object->getId(), (args.length() > 0 ? _T("Executed agent action %s, with fields: %s") :
                                                                                             _T("Executed agent action %s")), action, args.cstr());
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
               delete list;
               delete alarm;
            }
            else
            {
               msg.setField(VID_RCC, RCC_COMM_FAILURE);
            }
         }
         else
         {
            msg.setField(VID_RCC, RCC_ACCESS_DENIED);
            writeAuditLog(AUDIT_OBJECTS, false, object->getId(), _T("Access denied on executing agent action %s"), action);
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
   NXCPMessage msg(CMD_REQUEST_COMPLETED, requestId);
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
void ClientSession::updateObjectTool(NXCPMessage *request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request->getId());
   if (m_systemAccessRights & SYSTEM_ACCESS_MANAGE_TOOLS)
   {
      uint32_t rcc = UpdateObjectToolFromMessage(request);
      msg.setField(VID_RCC, rcc);
      if (rcc == RCC_SUCCESS)
         writeAuditLog(AUDIT_SYSCFG, true, 0, _T("Object tool [%u] updated"), request->getFieldAsUInt32(VID_TOOL_ID));
   }
   else
   {
      writeAuditLog(AUDIT_SYSCFG, false, 0, _T("Access denied on updating object tool [%u]"), request->getFieldAsUInt32(VID_TOOL_ID));
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);
   }
   sendMessage(&msg);
}

/**
 * Delete object tool
 */
void ClientSession::deleteObjectTool(NXCPMessage *request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request->getId());
   uint32_t toolId = request->getFieldAsUInt32(VID_TOOL_ID);
   if (m_systemAccessRights & SYSTEM_ACCESS_MANAGE_TOOLS)
   {
      uint32_t rcc = DeleteObjectToolFromDB(toolId);
      msg.setField(VID_RCC, rcc);
      if (rcc == RCC_SUCCESS)
         writeAuditLog(AUDIT_SYSCFG, true, 0, _T("Object tool [%u] deleted"), toolId);
   }
   else
   {
      writeAuditLog(AUDIT_SYSCFG, false, 0, _T("Access denied on deleting object tool [%u]"), toolId);
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);
   }
   sendMessage(&msg);
}

/**
 * Change Object Tool status (enabled/disabled)
 */
void ClientSession::changeObjectToolStatus(NXCPMessage *request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request->getId());
   if (m_systemAccessRights & SYSTEM_ACCESS_MANAGE_TOOLS)
   {
      uint32_t toolID = request->getFieldAsUInt32(VID_TOOL_ID);
      msg.setField(VID_RCC, ChangeObjectToolStatus(toolID, request->getFieldAsBoolean(VID_STATE)));
   }
   else
   {
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);
   }
   sendMessage(&msg);
}

/**
 * Generate ID for new object tool
 */
void ClientSession::generateObjectToolId(uint32_t requestId)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, requestId);
   if (checkSysAccessRights(SYSTEM_ACCESS_MANAGE_TOOLS))
   {
      msg.setField(VID_TOOL_ID, CreateUniqueId(IDG_OBJECT_TOOL));
   }
   else
   {
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);
   }
   sendMessage(&msg);
}

/**
 * Execute table tool (either SNMP or agent table)
 */
void ClientSession::execTableTool(NXCPMessage *pRequest)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, pRequest->getId());

   // Check if tool exist and has correct type
   uint32_t dwToolId = pRequest->getFieldAsUInt32(VID_TOOL_ID);
   if (IsTableTool(dwToolId))
   {
      // Check access
      if (CheckObjectToolAccess(dwToolId, m_dwUserId))
      {
         shared_ptr<NetObj> object = FindObjectById(pRequest->getFieldAsUInt32(VID_OBJECT_ID));
         if (object != nullptr)
         {
            if (object->getObjectClass() == OBJECT_NODE)
            {
               msg.setField(VID_RCC,
                               ExecuteTableTool(dwToolId, static_pointer_cast<Node>(object),
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
void ClientSession::changeSubscription(NXCPMessage *request)
{
   NXCPMessage msg;
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(request->getId());

   TCHAR channel[64];
   request->getFieldAsString(VID_NAME, channel, 64);
   Trim(channel);
   if (channel[0] != 0)
   {
      MutexLock(m_subscriptionLock);
      UINT32 *count = m_subscriptions->get(channel);
      if (request->getFieldAsBoolean(VID_OPERATION))
      {
         // Subscribe
         if (count == nullptr)
         {
            count = new UINT32;
            *count = 1;
            m_subscriptions->set(channel, count);
         }
         else
         {
            (*count)++;
         }
         debugPrintf(5, _T("Subscription added: %s (%d)"), channel, *count);
      }
      else
      {
         // Unsubscribe
         if (count != nullptr)
         {
            (*count)--;
            debugPrintf(5, _T("Subscription removed: %s (%d)"), channel, *count);
            if (*count == 0)
               m_subscriptions->remove(channel);
         }
      }
      MutexUnlock(m_subscriptionLock);
      msg.setField(VID_RCC, RCC_SUCCESS);
   }
   else
   {
      msg.setField(VID_RCC, RCC_INVALID_ARGUMENT);
   }

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
   msg.setField(VID_SERVER_UPTIME, (UINT32)(time(nullptr) - g_serverStartTime));

   // Number of objects and DCIs
	UINT32 dciCount = 0;
	g_idxNodeById.forEach(DciCountCallback, &dciCount);
   msg.setField(VID_NUM_ITEMS, dciCount);
	msg.setField(VID_NUM_OBJECTS, (UINT32)g_idxObjectById.size());
	msg.setField(VID_NUM_NODES, (UINT32)g_idxNodeById.size());

   // Client sessions
   msg.setField(VID_NUM_SESSIONS, (UINT32)GetSessionCount(true, true, -1, nullptr));

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
   ThreadPoolInfo poolInfo;
   ThreadPoolGetInfo(g_dataCollectorThreadPool, &poolInfo);
	msg.setField(VID_QSIZE_DCI_POLLER, (poolInfo.activeRequests > poolInfo.curThreads) ? poolInfo.activeRequests - poolInfo.curThreads : 0);

	msg.setField(VID_QSIZE_DCI_CACHE_LOADER, static_cast<uint32_t>(g_dciCacheLoaderQueue.size()));
	msg.setField(VID_QSIZE_DBWRITER, static_cast<uint32_t>(g_dbWriterQueue.size()));
	msg.setField(VID_QSIZE_EVENT, static_cast<uint32_t>(g_eventQueue.size()));
	msg.setField(VID_QSIZE_NODE_POLLER, static_cast<uint32_t>(GetDiscoveryPollerQueueSize()));

   // Send response
   sendMessage(&msg);
}

/**
 * Send script list
 */
void ClientSession::sendScriptList(UINT32 dwRqId)
{
   NXCPMessage msg;
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(dwRqId);
   if (m_systemAccessRights & SYSTEM_ACCESS_MANAGE_SCRIPTS)
   {
      GetServerScriptLibrary()->fillMessage(&msg);
      msg.setField(VID_RCC, RCC_SUCCESS);;
   }
   else
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);
   sendMessage(&msg);
}

/**
 * Send script
 */
void ClientSession::sendScript(NXCPMessage *pRequest)
{
   NXCPMessage msg;
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(pRequest->getId());

   if (m_systemAccessRights & SYSTEM_ACCESS_MANAGE_SCRIPTS)
   {
      UINT32 id = pRequest->getFieldAsUInt32(VID_SCRIPT_ID);
      NXSL_LibraryScript *script = GetServerScriptLibrary()->findScript(id);
      if (script != nullptr)
         script->fillMessage(&msg);
      else
         msg.setField(VID_RCC, RCC_INVALID_SCRIPT_ID);
   }
   else
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);
   sendMessage(&msg);
}

/**
 * Update script in library
 */
void ClientSession::updateScript(NXCPMessage *request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request->getId());
   if (m_systemAccessRights & SYSTEM_ACCESS_MANAGE_SCRIPTS)
   {
      uint32_t scriptId = 0;
      msg.setField(VID_RCC, UpdateScript(request, &scriptId, this));
      msg.setField(VID_SCRIPT_ID, scriptId);
   }
   else
   {
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);
      TCHAR scriptName[MAX_DB_STRING];
      request->getFieldAsString(VID_NAME, scriptName, MAX_DB_STRING);
      uint32_t scriptId = ResolveScriptName(scriptName);
      if (scriptId == 0)
         writeAuditLog(AUDIT_SYSCFG, false, 0, _T("Access denied on creating library script %s"), scriptName);
      else
         writeAuditLog(AUDIT_SYSCFG, false, 0, _T("Access denied on updating library script %s [%u]"), scriptName, scriptId);
   }
   sendMessage(&msg);
}

/**
 * Rename script
 */
void ClientSession::renameScript(NXCPMessage *request)
{
   NXCPMessage msg;

   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(request->getId());
   if (m_systemAccessRights & SYSTEM_ACCESS_MANAGE_SCRIPTS)
   {
      uint32_t scriptId = request->getFieldAsUInt32(VID_SCRIPT_ID);
      TCHAR oldName[MAX_DB_STRING];
      if (GetScriptName(scriptId, oldName, MAX_DB_STRING))
      {
         TCHAR newName[MAX_DB_STRING];
         request->getFieldAsString(VID_NAME, newName, MAX_DB_STRING);
         uint32_t rcc = RenameScript(scriptId, newName);
         msg.setField(VID_RCC, rcc);
         if (rcc == RCC_SUCCESS)
         {
            writeAuditLog(AUDIT_SYSCFG, true, 0, _T("Library script with ID %u renamed from %s to %s"), scriptId, oldName, newName);
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
      TCHAR newName[MAX_DB_STRING];
      writeAuditLog(AUDIT_SYSCFG, false, 0, _T("Access denied on renaming library script with ID %u to %s"),
               request->getFieldAsUInt32(VID_SCRIPT_ID), request->getFieldAsString(VID_NAME, newName, MAX_DB_STRING));
   }
   sendMessage(&msg);
}

/**
 * Delete script from library
 */
void ClientSession::deleteScript(NXCPMessage *request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request->getId());
   uint32_t scriptId = request->getFieldAsUInt32(VID_SCRIPT_ID);
   if (m_systemAccessRights & SYSTEM_ACCESS_MANAGE_SCRIPTS)
   {
      TCHAR scriptName[MAX_DB_STRING];
      if (GetScriptName(scriptId, scriptName, MAX_DB_STRING))
      {
         uint32_t rcc = DeleteScript(scriptId);
         msg.setField(VID_RCC, rcc);
         if (rcc == RCC_SUCCESS)
         {
            writeAuditLog(AUDIT_SYSCFG, true, 0, _T("Library script [%u] deleted"), scriptId);
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
      TCHAR scriptName[MAX_DB_STRING];
      if (!GetScriptName(scriptId, scriptName, MAX_DB_STRING))
         _tcscpy(scriptName, _T("<unknown>"));
      writeAuditLog(AUDIT_SYSCFG, false, 0, _T("Access denied on deleting library script %s [%u]"), scriptName, scriptId);
   }
   sendMessage(&msg);
}

/**
 * Copy session data to message
 */
static void CopySessionData(ClientSession *pSession, NXCPMessage *msg)
{
   uint32_t index = msg->getFieldAsUInt32(VID_NUM_SESSIONS);
   msg->setField(VID_NUM_SESSIONS, index + 1);

   uint32_t fieldId = VID_SESSION_DATA_BASE + index * 100;
   msg->setField(fieldId++, (UINT32)pSession->getId());
   msg->setField(fieldId++, (WORD)pSession->getCipher());
   msg->setField(fieldId++, (TCHAR *)pSession->getSessionName());
   msg->setField(fieldId++, (TCHAR *)pSession->getClientInfo());
   msg->setField(fieldId++, (TCHAR *)pSession->getLoginName());
}

/**
 * Send list of connected client sessions
 */
void ClientSession::getSessionList(uint32_t requestId)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, requestId);
   if (m_systemAccessRights & SYSTEM_ACCESS_MANAGE_SESSIONS)
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

/**
 * Forcibly terminate client's session
 */
void ClientSession::killSession(NXCPMessage *request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request->getId());
   if (m_systemAccessRights & SYSTEM_ACCESS_MANAGE_SESSIONS)
   {
      session_id_t id = request->getFieldAsInt32(VID_SESSION_ID);
      bool success = KillClientSession(id);
      msg.setField(VID_RCC, success ? RCC_SUCCESS : RCC_INVALID_SESSION_HANDLE);
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
void ClientSession::onSyslogMessage(const SyslogMessage *sm)
{
   if (isAuthenticated() && isSubscribedTo(NXC_CHANNEL_SYSLOG) && (m_systemAccessRights & SYSTEM_ACCESS_VIEW_SYSLOG))
   {
      shared_ptr<Node> node = sm->getNode();
      // If can't find object - just send to all sessions, if object found send to those who have rights
      if ((node == nullptr) || node->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ_ALARMS))
      {
         NXCPMessage msg(CMD_SYSLOG_RECORDS, 0);
         sm->fillNXCPMessage(&msg);
         postMessage(&msg);
      }
   }
}

/**
 * Handler for new traps
 */
void ClientSession::onNewSNMPTrap(NXCPMessage *pMsg)
{
   if (isAuthenticated() && isSubscribedTo(NXC_CHANNEL_SNMP_TRAPS) && (m_systemAccessRights & SYSTEM_ACCESS_VIEW_TRAP_LOG))
   {
      shared_ptr<NetObj> object = FindObjectById(pMsg->getFieldAsUInt32(VID_TRAP_LOG_MSG_BASE + 3));
      // If can't find object - just send to all events, if object found send to thous who have rights
      if ((object == nullptr) || object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ_ALARMS))
      {
         postMessage(pMsg);
      }
   }
}

/**
 * SNMP walker thread's startup parameters
 */
struct SNMP_WalkerThreadArgs
{
   ClientSession *session;
   uint32_t requestId;
   shared_ptr<Node> node;
   TCHAR *baseOID;

   SNMP_WalkerThreadArgs(ClientSession *_session, uint32_t _requestId, const shared_ptr<Node>& _node) : node(_node)
   {
      session = _session;
      requestId = _requestId;
      baseOID = nullptr;
   }

   ~SNMP_WalkerThreadArgs()
   {
      session->decRefCount();
      MemFree(baseOID);
   }
};

/**
 * Arguments for SnmpEnumerate callback
 */
struct SNMP_WalkerContext
{
   NXCPMessage *pMsg;
   uint32_t dwId;
   uint32_t dwNumVars;
   ClientSession *pSession;
};

/**
 * SNMP walker enumeration callback
 */
static UINT32 WalkerCallback(SNMP_Variable *pVar, SNMP_Transport *pTransport, void *pArg)
{
   NXCPMessage *pMsg = ((SNMP_WalkerContext *)pArg)->pMsg;
   TCHAR szBuffer[4096];
	bool convertToHex = true;

   pMsg->setField(((SNMP_WalkerContext *)pArg)->dwId++, pVar->getName().toString(szBuffer, 4096));
   pVar->getValueAsPrintableString(szBuffer, 4096, &convertToHex);
   pMsg->setField(((SNMP_WalkerContext *)pArg)->dwId++, convertToHex ? (UINT32)0xFFFF : pVar->getType());
   pMsg->setField(((SNMP_WalkerContext *)pArg)->dwId++, szBuffer);
   ((SNMP_WalkerContext *)pArg)->dwNumVars++;
   if (((SNMP_WalkerContext *)pArg)->dwNumVars == 50)
   {
      pMsg->setField(VID_NUM_VARIABLES, ((SNMP_WalkerContext *)pArg)->dwNumVars);
      ((SNMP_WalkerContext *)pArg)->pSession->sendMessage(pMsg);
      ((SNMP_WalkerContext *)pArg)->dwNumVars = 0;
      ((SNMP_WalkerContext *)pArg)->dwId = VID_SNMP_WALKER_DATA_BASE;
      pMsg->deleteAllFields();
   }
   return SNMP_ERR_SUCCESS;
}

/**
 * SNMP walker thread
 */
static void SNMP_WalkerThread(SNMP_WalkerThreadArgs *args)
{
   NXCPMessage msg(CMD_SNMP_WALK_DATA, args->requestId);

   SNMP_WalkerContext context;
   context.pMsg = &msg;
   context.dwId = VID_SNMP_WALKER_DATA_BASE;
   context.dwNumVars = 0;
   context.pSession = args->session;
   args->node->callSnmpEnumerate(args->baseOID, WalkerCallback, &context);
   msg.setField(VID_NUM_VARIABLES, context.dwNumVars);
   msg.setEndOfSequence();
   args->session->sendMessage(&msg);
   delete args;
}

/**
 * Start SNMP walk
 */
void ClientSession::startSnmpWalk(NXCPMessage *request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request->getId());

   shared_ptr<NetObj> object = FindObjectById(request->getFieldAsUInt32(VID_OBJECT_ID));
   if (object != nullptr)
   {
      if (object->getObjectClass() == OBJECT_NODE)
      {
         if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ_SNMP))
         {
            msg.setField(VID_RCC, RCC_SUCCESS);

            InterlockedIncrement(&m_refCount);

            SNMP_WalkerThreadArgs *args = new SNMP_WalkerThreadArgs(this, request->getId(), static_pointer_cast<Node>(object));
            args->baseOID = request->getFieldAsString(VID_SNMP_OID);
            ThreadPoolExecute(g_clientThreadPool, SNMP_WalkerThread, args);
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
UINT32 ClientSession::resolveDCIName(UINT32 dwNode, UINT32 dwItem, TCHAR *ppszName)
{
   UINT32 dwResult;

   shared_ptr<NetObj> object = FindObjectById(dwNode);
   if (object != nullptr)
   {
		if (object->isDataCollectionTarget() || (object->getObjectClass() == OBJECT_TEMPLATE))
		{
			if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
			{
				shared_ptr<DCObject> pItem = static_cast<DataCollectionOwner&>(*object).getDCObjectById(dwItem, m_dwUserId);
				if (pItem != nullptr)
				{
               _tcsncpy(ppszName, pItem->getDescription(), MAX_DB_STRING);
					dwResult = RCC_SUCCESS;
				}
				else
				{
               _sntprintf(ppszName, MAX_DB_STRING, _T("[%d]"), dwItem);
					dwResult = RCC_SUCCESS;
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
   NXCPMessage msg;

   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(pRequest->getId());

   dwNumDCI = pRequest->getFieldAsUInt32(VID_NUM_ITEMS);
   pdwNodeList = MemAllocArray<UINT32>(dwNumDCI);
   pdwDCIList = MemAllocArray<UINT32>(dwNumDCI);
   pRequest->getFieldAsInt32Array(VID_NODE_LIST, dwNumDCI, pdwNodeList);
   pRequest->getFieldAsInt32Array(VID_DCI_LIST, dwNumDCI, pdwDCIList);

   for(i = 0, dwId = VID_DCI_LIST_BASE; i < dwNumDCI; i++)
   {
      TCHAR m_description[MAX_DB_STRING];
      dwResult = resolveDCIName(pdwNodeList[i], pdwDCIList[i], m_description);
      if (dwResult != RCC_SUCCESS)
         break;
      msg.setField(dwId++, m_description);
   }

   free(pdwNodeList);
   free(pdwDCIList);

   msg.setField(VID_RCC, dwResult);
   sendMessage(&msg);
}

/**
 * Get list of available agent configurations
 */
void ClientSession::getAgentConfigurationList(uint32_t requestId)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, requestId);

   if (m_systemAccessRights & SYSTEM_ACCESS_MANAGE_AGENT_CFG)
   {
      DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
      DB_RESULT hResult = DBSelect(hdb, _T("SELECT config_id,config_name,sequence_number FROM agent_configs"));
      if (hResult != nullptr)
      {
         int count = DBGetNumRows(hResult);
         msg.setField(VID_RCC, RCC_SUCCESS);
         msg.setField(VID_NUM_RECORDS, count);
         uint32_t fieldId = VID_AGENT_CFG_LIST_BASE;
         for(int i = 0; i < count; i++, fieldId += 7)
         {
            msg.setField(fieldId++, DBGetFieldULong(hResult, i, 0));
            TCHAR name[MAX_DB_STRING];
            msg.setField(fieldId++, DBGetField(hResult, i, 1, name, MAX_DB_STRING));
            msg.setField(fieldId++, DBGetFieldULong(hResult, i, 2));
         }
         DBFreeResult(hResult);
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
 *  Get server-stored agent's configuration file
 */
void ClientSession::getAgentConfiguration(NXCPMessage *request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request->getId());

   if (m_systemAccessRights & SYSTEM_ACCESS_MANAGE_AGENT_CFG)
   {
      DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
      uint32_t configId = request->getFieldAsUInt32(VID_CONFIG_ID);
      TCHAR query[256];
      _sntprintf(query, 256, _T("SELECT config_name,config_file,config_filter,sequence_number FROM agent_configs WHERE config_id=%u"), configId);
      DB_RESULT hResult = DBSelect(hdb, query);
      if (hResult != nullptr)
      {
         if (DBGetNumRows(hResult) > 0)
         {
            msg.setField(VID_RCC, RCC_SUCCESS);
            msg.setField(VID_CONFIG_ID, configId);

            TCHAR name[MAX_DB_STRING];
            msg.setField(VID_NAME, DBGetField(hResult, 0, 0, name, MAX_DB_STRING));

            TCHAR *text = DBGetField(hResult, 0, 1, nullptr, 0);
            msg.setField(VID_CONFIG_FILE, text);
            MemFree(text);

            text = DBGetField(hResult, 0, 2, nullptr, 0);
            msg.setField(VID_FILTER, text);
            MemFree(text);

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
      DBConnectionPoolReleaseConnection(hdb);
   }
   else
   {
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);
   }

   sendMessage(&msg);
}

/**
 * Update server-stored agent's configuration
 */
void ClientSession::updateAgentConfiguration(NXCPMessage *request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request->getId());

   if (m_systemAccessRights & SYSTEM_ACCESS_MANAGE_AGENT_CFG)
   {
      DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
      uint32_t configId = request->getFieldAsUInt32(VID_CONFIG_ID);

      uint32_t sequence;
      DB_STATEMENT hStmt;
      if (IsDatabaseRecordExist(hdb, _T("agent_configs"), _T("config_id"), configId))
      {
         sequence = request->getFieldAsUInt32(VID_SEQUENCE_NUMBER);
         hStmt = DBPrepare(hdb, _T("UPDATE agent_configs SET config_name=?,config_filter=?,config_file=?,sequence_number=? WHERE config_id=?"));
      }
      else
      {
         if (configId == 0)
         {
            // Request for new ID creation
            configId = CreateUniqueId(IDG_AGENT_CONFIG);
            msg.setField(VID_CONFIG_ID, configId);

            // Request sequence number
            DB_RESULT hResult = DBSelect(hdb, _T("SELECT max(sequence_number) FROM agent_configs"));
            if (hResult != nullptr)
            {
               if (DBGetNumRows(hResult) > 0)
                  sequence = DBGetFieldULong(hResult, 0, 0) + 1;
               else
                  sequence = 1;
               DBFreeResult(hResult);
            }
            else
            {
               sequence = 1;
            }
            msg.setField(VID_SEQUENCE_NUMBER, sequence);
         }
         else
         {
            sequence = request->getFieldAsUInt32(VID_SEQUENCE_NUMBER);
         }
         hStmt = DBPrepare(hdb,  _T("INSERT INTO agent_configs (config_name,config_filter,config_file,sequence_number,config_id) VALUES (?,?,?,?,?)"));
      }
      if (hStmt != nullptr)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, request->getFieldAsString(VID_NAME), DB_BIND_DYNAMIC);
         DBBind(hStmt, 2, DB_SQLTYPE_TEXT, request->getFieldAsString(VID_FILTER), DB_BIND_DYNAMIC);
         DBBind(hStmt, 3, DB_SQLTYPE_TEXT, request->getFieldAsString(VID_CONFIG_FILE), DB_BIND_DYNAMIC);
         DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, sequence);
         DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, configId);
         if (DBExecute(hStmt))
         {
            msg.setField(VID_RCC, RCC_SUCCESS);
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

   sendMessage(&msg);
}

/**
 * Delete agent's configuration
 */
void ClientSession::deleteAgentConfiguration(NXCPMessage *request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request->getId());

   if (m_systemAccessRights & SYSTEM_ACCESS_MANAGE_AGENT_CFG)
   {
      DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
      uint32_t configId = request->getFieldAsUInt32(VID_CONFIG_ID);
      if (IsDatabaseRecordExist(hdb, _T("agent_configs"), _T("config_id"), configId))
      {
         if (ExecuteQueryOnObject(hdb, configId, _T("DELETE FROM agent_configs WHERE config_id=?")))
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
      DBConnectionPoolReleaseConnection(hdb);
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
void ClientSession::swapAgentConfigurations(NXCPMessage *request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request->getId());

   if (m_systemAccessRights & SYSTEM_ACCESS_MANAGE_AGENT_CFG)
   {
      DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

      TCHAR query[256];
      _sntprintf(query, 256, _T("SELECT config_id,sequence_number FROM agent_configs WHERE config_id=%d OR config_id=%d"),
                 request->getFieldAsUInt32(VID_CONFIG_ID), request->getFieldAsUInt32(VID_CONFIG_ID_2));
      DB_RESULT hResult = DBSelect(hdb, query);
      if (hResult != nullptr)
      {
         if (DBGetNumRows(hResult) >= 2)
         {
            if (DBBegin(hdb))
            {
               _sntprintf(query, 256, _T("UPDATE agent_configs SET sequence_number=%d WHERE config_id=%d"),
                          DBGetFieldULong(hResult, 1, 1), DBGetFieldULong(hResult, 0, 0));
               bool success = DBQuery(hdb, query);
               if (success)
               {
                  _sntprintf(query, 256, _T("UPDATE agent_configs SET sequence_number=%d WHERE config_id=%d"),
                             DBGetFieldULong(hResult, 0, 1), DBGetFieldULong(hResult, 1, 0));
                  success = DBQuery(hdb, query);
               }

               if (success)
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
      DBConnectionPoolReleaseConnection(hdb);
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
void ClientSession::sendConfigForAgent(NXCPMessage *request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request->getId());

   TCHAR platform[MAX_DB_STRING];
   request->getFieldAsString(VID_PLATFORM_NAME, platform, MAX_DB_STRING);
   uint16_t versionMajor = request->getFieldAsUInt16(VID_VERSION_MAJOR);
   uint16_t versionMinor = request->getFieldAsUInt16(VID_VERSION_MINOR);
   uint16_t versionRelease = request->getFieldAsUInt16(VID_VERSION_RELEASE);
   debugPrintf(3, _T("Finding config for agent at %s: platform=\"%s\", version=\"%d.%d.%d\""),
            m_clientAddr.toString().cstr(), platform, (int)versionMajor, (int)versionMinor, (int)versionRelease);

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_RESULT hResult = DBSelect(hdb, _T("SELECT config_id,config_file,config_filter FROM agent_configs ORDER BY sequence_number"));
   if (hResult != nullptr)
   {
      int nNumRows = DBGetNumRows(hResult);
      int i;
      for(i = 0; i < nNumRows; i++)
      {
         uint32_t configId = DBGetFieldULong(hResult, i, 0);

         // Compile script
         TCHAR *filterSource = DBGetField(hResult, i, 2, nullptr, 0);
         TCHAR errorMessage[256];
         NXSL_VM *filter = NXSLCompileAndCreateVM(filterSource, errorMessage, 256, new NXSL_ServerEnv());
         MemFree(filterSource);

         if (filter != nullptr)
         {
            // Set arguments:
            // $1 - IP address
            // $2 - platform
            // $3 - major version number
            // $4 - minor version number
            // $5 - release number
            NXSL_Value *args[5];
            args[0] = filter->createValue(m_clientAddr.toString());
            args[1] = filter->createValue(platform);
            args[2] = filter->createValue((LONG)versionMajor);
            args[3] = filter->createValue((LONG)versionMinor);
            args[4] = filter->createValue((LONG)versionRelease);

            // Run script
            DbgPrintf(3, _T("Running configuration matching script %d"), configId);
            if (filter->run(5, args))
            {
               NXSL_Value *pValue = filter->getResult();
               if (pValue->isTrue())
               {
                  DbgPrintf(3, _T("Configuration script %d matched for agent %s, sending config"), configId, m_clientAddr.toString().cstr());
                  msg.setField(VID_RCC, (WORD)0);
                  TCHAR *content = DBGetField(hResult, i, 1, nullptr, 0);
                  msg.setField(VID_CONFIG_FILE, content);
                  msg.setField(VID_CONFIG_ID, configId);
                  MemFree(content);
                  delete filter;
                  break;
               }
               else
               {
                  DbgPrintf(3, _T("Configuration script %d not matched for agent %s"), configId, m_clientAddr.toString().cstr());
               }
            }
            else
            {
               _sntprintf(errorMessage, 256, _T("AgentCfg::%d"), configId);
               PostSystemEvent(EVENT_SCRIPT_ERROR, g_dwMgmtNode, "ssd", errorMessage, filter->getErrorText(), 0);
            }
            delete filter;
         }
         else
         {
            TCHAR scriptName[256];
            _sntprintf(scriptName, 256, _T("AgentCfg::%d"), configId);
            PostSystemEvent(EVENT_SCRIPT_ERROR, g_dwMgmtNode, "ssd", scriptName, errorMessage, 0);
         }
      }
      DBFreeResult(hResult);

      if (i == nNumRows)
         msg.setField(VID_RCC, (uint16_t)1);  // No matching configs found
   }
   else
   {
      msg.setField(VID_RCC, (uint16_t)1);  // DB Failure
   }
   DBConnectionPoolReleaseConnection(hdb);

   sendMessage(&msg);
}

/**
 * Send object comments to client
 */
void ClientSession::getObjectComments(NXCPMessage *request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request->getId());

   shared_ptr<NetObj> object = FindObjectById(request->getFieldAsUInt32(VID_OBJECT_ID));
   if (object != nullptr)
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
   NXCPMessage msg(CMD_REQUEST_COMPLETED, pRequest->getId());

   shared_ptr<NetObj> object = FindObjectById(pRequest->getFieldAsUInt32(VID_OBJECT_ID));
   if (object != nullptr)
   {
      if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_MODIFY))
      {
         TCHAR *comments = pRequest->getFieldAsString(VID_COMMENTS);
         object->setComments(comments);
         MemFree(comments);
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
 * Data push element
 */
struct ClientDataPushElement
{
   shared_ptr<DataCollectionTarget> dcTarget;
   shared_ptr<DCObject> dci;
   TCHAR *value;

   ClientDataPushElement(const shared_ptr<DataCollectionTarget>& _dcTarget, const shared_ptr<DCObject> &_dci, TCHAR *_value) : dcTarget(_dcTarget), dci(_dci)
   {
      value = _value;
   }

   ~ClientDataPushElement()
   {
      MemFree(value);
   }
};

/**
 * Push DCI data
 */
void ClientSession::pushDCIData(NXCPMessage *pRequest)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, pRequest->getId());

   int count = pRequest->getFieldAsInt32(VID_NUM_ITEMS);
   if (count > 0)
   {
      ObjectArray<ClientDataPushElement> values(count, 16, Ownership::True);

      UINT32 fieldId = VID_PUSH_DCI_DATA_BASE;
      bool bOK = true;
      int i;
      for(i = 0; (i < count) && bOK; i++)
      {
         bOK = FALSE;

         // Find object either by ID or name (id ID==0)
         shared_ptr<NetObj> object;
         UINT32 objectId = pRequest->getFieldAsUInt32(fieldId++);
         if (objectId != 0)
         {
            object = FindObjectById(objectId);
         }
         else
         {
            TCHAR name[256];
            pRequest->getFieldAsString(fieldId++, name, 256);
				if (name[0] == _T('@'))
				{
               InetAddress ipAddr = InetAddress::resolveHostName(&name[1]);
					object = FindNodeByIP(0, ipAddr);
				}
				else
				{
					object = FindObjectByName(name, OBJECT_NODE);
				}
         }

         // Validate object
         if (object != nullptr)
         {
            if (object->isDataCollectionTarget())
            {
               if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_PUSH_DATA))
               {
                  // Object OK, find DCI by ID or name (if ID==0)
                  UINT32 dciId = pRequest->getFieldAsUInt32(fieldId++);
						shared_ptr<DCObject> pItem;
                  if (dciId != 0)
                  {
                     pItem = static_cast<DataCollectionTarget&>(*object).getDCObjectById(dciId, m_dwUserId);
                  }
                  else
                  {
                     TCHAR name[256];
                     pRequest->getFieldAsString(fieldId++, name, 256);
                     pItem = static_cast<DataCollectionTarget&>(*object).getDCObjectByName(name, m_dwUserId);
                  }

                  if ((pItem != nullptr) && (pItem->getType() == DCO_TYPE_ITEM))
                  {
                     if (pItem->getDataSource() == DS_PUSH_AGENT)
                     {
                        values.add(new ClientDataPushElement(static_pointer_cast<DataCollectionTarget>(object), pItem, pRequest->getFieldAsString(fieldId++)));
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
            t = time(nullptr);
         for(int i = 0; i < values.size(); i++)
         {
            ClientDataPushElement *e = values.get(i);
				if (_tcslen(e->value) >= MAX_DCI_STRING_VALUE)
					e->value[MAX_DCI_STRING_VALUE - 1] = 0;
				e->dcTarget->processNewDCValue(e->dci, t, e->value);
            if (t > e->dci->getLastPollTime())
				   e->dci->setLastPollTime(t);
         }
         msg.setField(VID_RCC, RCC_SUCCESS);
      }
      else
      {
         msg.setField(VID_FAILED_DCI_INDEX, i - 1);
      }
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
void ClientSession::getAddrList(NXCPMessage *request)
{
   NXCPMessage msg;
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(request->getId());

   if (m_systemAccessRights & SYSTEM_ACCESS_SERVER_CONFIG)
   {
      ObjectArray<InetAddressListElement> *list = LoadServerAddressList(request->getFieldAsInt32(VID_ADDR_LIST_TYPE));
      if (list != nullptr)
      {
         msg.setField(VID_NUM_RECORDS, (INT32)list->size());

         UINT32 fieldId = VID_ADDR_LIST_BASE;
         for(int i = 0; i < list->size(); i++)
         {
            list->get(i)->fillMessage(&msg, fieldId);
            fieldId += 10;
         }
         msg.setField(VID_RCC, RCC_SUCCESS);
         delete list;
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
void ClientSession::setAddrList(NXCPMessage *request)
{
   NXCPMessage msg;
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(request->getId());

   int listType = request->getFieldAsInt32(VID_ADDR_LIST_TYPE);
   if (m_systemAccessRights & SYSTEM_ACCESS_SERVER_CONFIG)
   {
      if (UpdateAddressListFromMessage(request))
      {
         msg.setField(VID_RCC, RCC_SUCCESS);
         WriteAuditLog(AUDIT_SYSCFG, true, m_dwUserId, m_workstation, m_id, 0, _T("Address list %d modified"), listType);
      }
      else
      {
         msg.setField(VID_RCC, RCC_DB_FAILURE);
      }
   }
   else
   {
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);
      WriteAuditLog(AUDIT_SYSCFG, false, m_dwUserId, m_workstation, m_id, 0, _T("Access denied on modify address list %d"), listType);
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

   if (m_systemAccessRights & SYSTEM_ACCESS_SERVER_CONFIG)
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
void ClientSession::getRelatedEventList(NXCPMessage *request)
{
   NXCPMessage msg;
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(request->getId());

   shared_ptr<NetObj> object = FindObjectById(request->getFieldAsUInt32(VID_OBJECT_ID));
   if (object != nullptr)
   {
      if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
      {
         if (object->isDataCollectionTarget() || (object->getObjectClass() == OBJECT_TEMPLATE))
         {
            HashSet<uint32_t> *eventList = static_cast<DataCollectionOwner&>(*object).getRelatedEventsList();
            msg.setField(VID_NUM_EVENTS, (UINT32)eventList->size());
            msg.setFieldFromInt32Array(VID_EVENT_LIST, eventList);
            delete eventList;
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

   shared_ptr<NetObj> object = FindObjectById(request->getFieldAsUInt32(VID_OBJECT_ID));
   if (object != nullptr)
   {
      if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
      {
         if (object->isDataCollectionTarget() || (object->getObjectClass() == OBJECT_TEMPLATE))
         {
            StringSet *scripts = static_cast<DataCollectionOwner&>(*object).getDCIScriptList();
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
   NXCPMessage msg(CMD_REQUEST_COMPLETED, pRequest->getId());

   if (checkSysAccessRights(SYSTEM_ACCESS_CONFIGURE_TRAPS | SYSTEM_ACCESS_VIEW_EVENT_DB | SYSTEM_ACCESS_EPP))
   {
      uint32_t *pdwTemplateList;
      uint32_t dwNumTemplates = pRequest->getFieldAsUInt32(VID_NUM_OBJECTS);
      if (dwNumTemplates > 0)
      {
         pdwTemplateList = MemAllocArray<uint32_t>(dwNumTemplates);
         pRequest->getFieldAsInt32Array(VID_OBJECT_LIST, dwNumTemplates, pdwTemplateList);
      }
      else
      {
         pdwTemplateList = nullptr;
      }

      uint32_t i;
      for(i = 0; i < dwNumTemplates; i++)
      {
         shared_ptr<NetObj> object = FindObjectById(pdwTemplateList[i]);
         if (object != nullptr)
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
         TCHAR osVersion[256];
         GetOSVersionString(osVersion, 256);

         StringBuffer xml(_T("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<configuration>\n\t<formatVersion>4</formatVersion>\n\t<server>\n\t\t<version>")
                  NETXMS_VERSION_STRING _T("</version>\n\t\t<buildTag>") NETXMS_BUILD_TAG _T("</buildTag>\n\t\t<operatingSystem>"));
         xml.appendPreallocated(EscapeStringForXML(osVersion, -1));
         xml.append(_T("</operatingSystem>\n\t</server>\n\t<description>"));
			TCHAR *description = pRequest->getFieldAsString(VID_DESCRIPTION);
			xml.appendPreallocated(EscapeStringForXML(description, -1));
			MemFree(description);
         xml.append(_T("</description>\n"));

         // Write events
         xml += _T("\t<events>\n");
         uint32_t count = pRequest->getFieldAsUInt32(VID_NUM_EVENTS);
         uint32_t *pdwList = MemAllocArray<uint32_t>(count);
         pRequest->getFieldAsInt32Array(VID_EVENT_LIST, count, pdwList);
         for(i = 0; i < count; i++)
            CreateEventTemplateExportRecord(xml, pdwList[i]);
         MemFree(pdwList);
         xml += _T("\t</events>\n");

         // Write templates
         xml += _T("\t<templates>\n");
         for(i = 0; i < dwNumTemplates; i++)
         {
            shared_ptr<NetObj> object = FindObjectById(pdwTemplateList[i]);
            if (object != nullptr)
            {
               static_cast<Template&>(*object).createExportRecord(xml);
            }
         }
         xml += _T("\t</templates>\n");

         // Write traps
         xml += _T("\t<traps>\n");
         count = pRequest->getFieldAsUInt32(VID_NUM_TRAPS);
         pdwList = MemAllocArray<UINT32>(count);
         pRequest->getFieldAsInt32Array(VID_TRAP_LIST, count, pdwList);
         for(i = 0; i < count; i++)
            CreateTrapExportRecord(xml, pdwList[i]);
         MemFree(pdwList);
         xml += _T("\t</traps>\n");

         // Write rules
         xml += _T("\t<rules>\n");
         count = pRequest->getFieldAsUInt32(VID_NUM_RULES);
         DWORD varId = VID_RULE_LIST_BASE;
         uuid_t guid;
         for(i = 0; i < count; i++)
         {
            pRequest->getFieldAsBinary(varId++, guid, UUID_LENGTH);
            g_pEventPolicy->exportRule(xml, guid);
         }
         xml += _T("\t</rules>\n");

         if (count > 0) //add rule order information if at least one rule is exported
         {
            xml += _T("\t<ruleOrdering>\n");
            g_pEventPolicy->exportRuleOrgering(xml);
            xml += _T("\t</ruleOrdering>\n");
         }

         // Write scripts
         xml.append(_T("\t<scripts>\n"));
         count = pRequest->getFieldAsUInt32(VID_NUM_SCRIPTS);
         pdwList = MemAllocArray<UINT32>(count);
         pRequest->getFieldAsInt32Array(VID_SCRIPT_LIST, count, pdwList);
         for(i = 0; i < count; i++)
            CreateScriptExportRecord(xml, pdwList[i]);
         MemFree(pdwList);
         xml.append(_T("\t</scripts>\n"));

         // Write object tools
         xml.append(_T("\t<objectTools>\n"));
         count = pRequest->getFieldAsUInt32(VID_NUM_TOOLS);
         pdwList = MemAllocArray<UINT32>(count);
         pRequest->getFieldAsInt32Array(VID_TOOL_LIST, count, pdwList);
         for(i = 0; i < count; i++)
            CreateObjectToolExportRecord(xml, pdwList[i]);
         MemFree(pdwList);
         xml.append(_T("\t</objectTools>\n"));

         // Write DCI summary tables
         xml.append(_T("\t<dciSummaryTables>\n"));
         count = pRequest->getFieldAsUInt32(VID_NUM_SUMMARY_TABLES);
         pdwList = MemAllocArray<UINT32>(count);
         pRequest->getFieldAsInt32Array(VID_SUMMARY_TABLE_LIST, count, pdwList);
         for(i = 0; i < count; i++)
            CreateSummaryTableExportRecord(pdwList[i], xml);
         MemFree(pdwList);
         xml.append(_T("\t</dciSummaryTables>\n"));

         // Write actions
         xml.append(_T("\t<actions>\n"));
         count = pRequest->getFieldAsUInt32(VID_NUM_ACTIONS);
         pdwList = MemAllocArray<UINT32>(count);
         pRequest->getFieldAsInt32Array(VID_ACTION_LIST, count, pdwList);
         for(i = 0; i < count; i++)
            CreateActionExportRecord(xml, pdwList[i]);
         MemFree(pdwList);
         xml.append(_T("\t</actions>\n"));

         // Write web service definition
         xml.append(_T("\t<webServiceDefinitions>\n"));
         count = pRequest->getFieldAsUInt32(VID_WEB_SERVICE_DEF_COUNT);
         pdwList = MemAllocArray<UINT32>(count);
         pRequest->getFieldAsInt32Array(VID_WEB_SERVICE_DEF_LIST, count, pdwList);
         CreateWebServiceDefinitionExportRecord(xml, count, pdwList);
         MemFree(pdwList);
         xml.append(_T("\t</webServiceDefinitions>\n"));

			// Close document
			xml += _T("</configuration>\n");

         // put result into message
         msg.setField(VID_RCC, RCC_SUCCESS);
         msg.setField(VID_NXMP_CONTENT, xml);
      }

      MemFree(pdwTemplateList);
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

   if (checkSysAccessRights(SYSTEM_ACCESS_IMPORT_CONFIGURATION))
   {
      char *content = pRequest->getFieldAsUtf8String(VID_NXMP_CONTENT);
      if (content != nullptr)
      {
         Config config(false);
         if (config.loadXmlConfigFromMemory(content, strlen(content), nullptr, "configuration"))
         {
            // Lock all required components
            if (LockComponent(CID_EPP, m_id, m_sessionName, nullptr, szLockInfo))
            {
               InterlockedOr(&m_flags, CSF_EPP_LOCKED);

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
					InterlockedAnd(&m_flags, ~CSF_EPP_LOCKED);
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
         MemFree(content);
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

   shared_ptr<NetObj> object = FindObjectById(pRequest->getFieldAsUInt32(VID_OBJECT_ID));
   if (object != nullptr)
   {
      if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
      {
         if (object->isDataCollectionTarget() || (object->getObjectClass() == OBJECT_TEMPLATE))
         {
            shared_ptr<DCObject> dcObject = static_cast<DataCollectionOwner&>(*object).getDCObjectById(pRequest->getFieldAsUInt32(VID_DCI_ID), m_dwUserId);
				if ((dcObject != nullptr) && (dcObject->getType() == DCO_TYPE_ITEM))
				{
					msg.setField(VID_TEMPLATE_ID, dcObject->getTemplateId());
					msg.setField(VID_RESOURCE_ID, dcObject->getResourceId());
					msg.setField(VID_DCI_DATA_TYPE, static_cast<UINT16>(static_cast<DCItem&>(*dcObject).getDataType()));
					msg.setField(VID_DCI_SOURCE_TYPE, static_cast<UINT16>(static_cast<DCItem&>(*dcObject).getDataSource()));
					msg.setField(VID_NAME, dcObject->getName());
					msg.setField(VID_DESCRIPTION, dcObject->getDescription());
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

void ClientSession::sendGraph(NXCPMessage *request)
{
   NXCPMessage msg;
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(request->getId());
   UINT32 rcc = GetGraphAccessCheckResult(request->getFieldAsUInt32(VID_GRAPH_ID), m_dwUserId);
   if(rcc == RCC_SUCCESS)
   {
      FillGraphListMsg(&msg, m_dwUserId, false, request->getFieldAsUInt32(VID_GRAPH_ID));
   }
   else
      msg.setField(VID_RCC, rcc);

   sendMessage(&msg);
}

/**
 * Send list of available graphs to client
 */
void ClientSession::sendGraphList(NXCPMessage *request)
{
   NXCPMessage msg;
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(request->getId());
   bool templageGraphs = request->getFieldAsBoolean(VID_GRAPH_TEMPALTE);
   FillGraphListMsg(&msg, m_dwUserId, templageGraphs, 0);
   sendMessage(&msg);
}

/**
 * Save graph
 */
void ClientSession::saveGraph(NXCPMessage *pRequest)
{
   NXCPMessage msg;
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(pRequest->getId());
   SaveGraph(pRequest, m_dwUserId, &msg);
   sendMessage(&msg);
}

/**
 * Delete graph
 */
void ClientSession::deleteGraph(NXCPMessage *pRequest)
{
   NXCPMessage msg;
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(pRequest->getId());
   UINT32 result = DeleteGraph(pRequest->getFieldAsUInt32(VID_GRAPH_ID), m_dwUserId);
   msg.setField(VID_RCC, result);
   sendMessage(&msg);
}

/**
 * Send list of DCIs to be shown in performance tab
 */
void ClientSession::sendPerfTabDCIList(NXCPMessage *pRequest)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, pRequest->getId());

	shared_ptr<NetObj> object = FindObjectById(pRequest->getFieldAsUInt32(VID_OBJECT_ID));
	if (object != nullptr)
	{
		if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
		{
			if (object->isDataCollectionTarget())
			{
				msg.setField(VID_RCC, static_cast<DataCollectionTarget&>(*object).getPerfTabDCIList(&msg, m_dwUserId));
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
 * Query layer 2 topology from device
 */
void ClientSession::queryL2Topology(NXCPMessage *pRequest)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, pRequest->getId());

	shared_ptr<NetObj> object = FindObjectById(pRequest->getFieldAsUInt32(VID_OBJECT_ID));
	if (object != nullptr)
	{
		if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
		{
			if (object->getObjectClass() == OBJECT_NODE)
			{
			   uint32_t dwResult;
			   NetworkMapObjectList *topology = static_cast<Node&>(*object).getL2Topology();
				if (topology == nullptr)
				{
				   topology = static_cast<Node&>(*object).buildL2Topology(&dwResult, -1, true);
				}
				else
				{
					dwResult = RCC_SUCCESS;
				}
				if (topology != nullptr)
				{
					msg.setField(VID_RCC, RCC_SUCCESS);
					topology->createMessage(&msg);
					delete topology;
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
 * Query object internal connection topology
 */
void ClientSession::queryInternalCommunicationTopology(NXCPMessage *pRequest)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, pRequest->getId());

   shared_ptr<NetObj> object = FindObjectById(pRequest->getFieldAsUInt32(VID_OBJECT_ID));
   if (object != nullptr)
   {
      if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
      {
         NetworkMapObjectList *topology = nullptr;
         if (object->getObjectClass() == OBJECT_NODE)
         {
            topology = static_cast<Node&>(*object).buildInternalConnectionTopology();
         }
         else if (object->getObjectClass() == OBJECT_SENSOR)
         {
            topology = static_cast<Sensor&>(*object).buildInternalConnectionTopology();
         }

         if (topology != nullptr)
         {
            msg.setField(VID_RCC, RCC_SUCCESS);
            topology->createMessage(&msg);
            delete topology;
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
 * Get list of dependent nodes
 */
void ClientSession::getDependentNodes(NXCPMessage *request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request->getId());

   shared_ptr<NetObj> object = FindObjectById(request->getFieldAsUInt32(VID_NODE_ID));
   if (object != nullptr)
   {
      if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
      {
         if (object->getObjectClass() == OBJECT_NODE)
         {
            StructArray<DependentNode> *dependencies = GetNodeDependencies(object->getId());
            msg.setField(VID_NUM_ELEMENTS, dependencies->size());
            UINT32 fieldId = VID_ELEMENT_LIST_BASE;
            for(int i = 0; i < dependencies->size(); i++)
            {
               msg.setField(fieldId++, dependencies->get(i)->nodeId);
               msg.setField(fieldId++, dependencies->get(i)->dependencyType);
               fieldId += 8;
            }
            msg.setField(VID_RCC, RCC_SUCCESS);
            delete dependencies;
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
 * Send notification
 */
void ClientSession::sendNotification(NXCPMessage *pRequest)
{
   NXCPMessage msg;
	TCHAR channelName[MAX_OBJECT_NAME], *phone, *message, *subject;

	msg.setId(pRequest->getId());
	msg.setCode(CMD_REQUEST_COMPLETED);

	if ((m_systemAccessRights & SYSTEM_ACCESS_SEND_NOTIFICATION))
	{
	   pRequest->getFieldAsString(VID_CHANNEL_NAME, channelName, MAX_OBJECT_NAME);
	   if (IsNotificationChannelExists(channelName))
	   {
	      phone = pRequest->getFieldAsString(VID_RCPT_ADDR);
	      subject = pRequest->getFieldAsString(VID_EMAIL_SUBJECT);
	      message = pRequest->getFieldAsString(VID_MESSAGE);
	      SendNotification(channelName, phone, subject, message);
	      msg.setField(VID_RCC, RCC_SUCCESS);
	      MemFree(phone);
	      MemFree(subject);
	      MemFree(message);
	   }
	   else
	   {
         msg.setField(VID_RCC, RCC_NO_CHANNEL_NAME);
	   }
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
void ClientSession::sendNetworkCredList(NXCPMessage *request)
{
   NXCPMessage msg;
	msg.setId(request->getId());
	msg.setCode(CMD_REQUEST_COMPLETED);

	if (m_systemAccessRights & SYSTEM_ACCESS_SERVER_CONFIG)
	{
	   bool allZones = !request->isFieldExist(VID_ZONE_UIN);
	   if (allZones)
	   {
	      switch (request->getCode())
	      {
	         case CMD_GET_COMMUNITY_LIST:
	            GetFullCommunityList(&msg);
	            break;
	         case CMD_GET_USM_CREDENTIALS:
	            GetFullUsmCredentialList(&msg);
               break;
	         case CMD_GET_SNMP_PORT_LIST:
	            GetFullSnmpPortList(&msg);
	            break;
	         case CMD_GET_SHARED_SECRET_LIST:
	            GetFullAgentSecretList(&msg);
	            break;
	      }
	   }
	   else
	   {
	      int32_t zoneUIN = request->getFieldAsInt32(VID_ZONE_UIN);
	      shared_ptr<Zone> zone = FindZoneByUIN(zoneUIN);
	      if (zone == nullptr && zoneUIN != SNMP_CONFIG_GLOBAL)
	      {
	         msg.setField(VID_RCC, RCC_INVALID_ZONE_ID);
	      }
	      else
	      {
	         switch(request->getCode())
	         {
               case CMD_GET_COMMUNITY_LIST:
                  GetZoneCommunityList(&msg, zoneUIN);
                  break;
               case CMD_GET_USM_CREDENTIALS:
                  GetZoneUsmCredentialList(&msg, zoneUIN);
                  break;
               case CMD_GET_SNMP_PORT_LIST:
                  GetZoneSnmpPortList(&msg, zoneUIN);
                  break;
               case CMD_GET_SHARED_SECRET_LIST:
                  GetZoneAgentSecretList(&msg, zoneUIN);
                  break;
	         }
	      }
	   }
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
void ClientSession::updateCommunityList(NXCPMessage *pRequest)
{
   NXCPMessage msg;
	UINT32 rcc = RCC_SUCCESS;

	msg.setId(pRequest->getId());
	msg.setCode(CMD_REQUEST_COMPLETED);

	if (m_systemAccessRights & SYSTEM_ACCESS_SERVER_CONFIG)
	{
      int32_t zoneUIN = pRequest->getFieldAsInt32(VID_ZONE_UIN);
      shared_ptr<Zone> zone = FindZoneByUIN(zoneUIN);
      if (zoneUIN == -1 || zone != nullptr)
      {
         DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
         if (DBBegin(hdb))
         {
            ExecuteQueryOnObject(hdb, zoneUIN, _T("DELETE FROM snmp_communities WHERE zone=?"));
            UINT32 stringBase = VID_COMMUNITY_STRING_LIST_BASE;
            DB_STATEMENT hStmt = DBPrepare(hdb, _T("INSERT INTO snmp_communities (id,community,zone) VALUES(?,?,?)"), true);
            if (hStmt != nullptr)
            {
               int count = pRequest->getFieldAsUInt32(VID_NUM_STRINGS);
               DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, zoneUIN);
               for(int i = 0; i < count; i++)
               {
                  DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, i + 1);
                  DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, pRequest->getFieldAsString(stringBase++), DB_BIND_DYNAMIC);
                  if (!DBExecute(hStmt))
                  {
                     rcc = RCC_DB_FAILURE;
                     break;
                  }
               }
               DBFreeStatement(hStmt);
            }
            else
               rcc = RCC_DB_FAILURE;

            if (rcc == RCC_SUCCESS)
            {
               DBCommit(hdb);
               NotifyClientSessions(NX_NOTIFY_COMMUNITIES_CONFIG_CHANGED, zoneUIN);
            }
            else
               DBRollback(hdb);
         }
         else
            rcc = RCC_DB_FAILURE;
         DBConnectionPoolReleaseConnection(hdb);
      }
      else
         rcc = RCC_INVALID_ZONE_ID;
	}
	else
		rcc = RCC_ACCESS_DENIED;

	msg.setField(VID_RCC, rcc);
	sendMessage(&msg);
}

/**
 * Send Persistant Storage list to client
 */
void ClientSession::getPersistantStorage(UINT32 dwRqId)
{
   NXCPMessage msg;

	msg.setId(dwRqId);
	msg.setCode(CMD_REQUEST_COMPLETED);

	if (m_systemAccessRights & SYSTEM_ACCESS_PERSISTENT_STORAGE)
	{
		GetPersistentStorageList(&msg);
	}
	else
	{
		msg.setField(VID_RCC, RCC_ACCESS_DENIED);
	}
   sendMessage(&msg);
}

/**
 * Set persistent storage value
 */
void ClientSession::setPstorageValue(NXCPMessage *pRequest)
{
   NXCPMessage msg;

	msg.setId(pRequest->getId());
	msg.setCode(CMD_REQUEST_COMPLETED);

	if (m_systemAccessRights & SYSTEM_ACCESS_PERSISTENT_STORAGE)
	{
      TCHAR key[256], *value;
		pRequest->getFieldAsString(VID_PSTORAGE_KEY, key, 256);
		value = pRequest->getFieldAsString(VID_PSTORAGE_VALUE);
		SetPersistentStorageValue(key, value);
		free(value);
      msg.setField(VID_RCC, RCC_SUCCESS);
	}
	else
	{
		msg.setField(VID_RCC, RCC_ACCESS_DENIED);
	}

	sendMessage(&msg);
}

/**
 * Delete persistent storage value
 */
void ClientSession::deletePstorageValue(NXCPMessage *pRequest)
{
   NXCPMessage msg;

	msg.setId(pRequest->getId());
	msg.setCode(CMD_REQUEST_COMPLETED);

	if (m_systemAccessRights & SYSTEM_ACCESS_PERSISTENT_STORAGE)
	{
      TCHAR key[256];
      //key[0]=0;
		pRequest->getFieldAsString(VID_PSTORAGE_KEY, key, 256);
		bool success = DeletePersistentStorageValue(key);
		msg.setField(VID_RCC, success ? RCC_SUCCESS : RCC_INVALID_PSTORAGE_KEY);
	}
	else
	{
		msg.setField(VID_RCC, RCC_ACCESS_DENIED);
	}

	sendMessage(&msg);
}

/**
 * Register agent
 */
void ClientSession::registerAgent(NXCPMessage *pRequest)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, pRequest->getId());

	if (ConfigReadBoolean(_T("EnableAgentRegistration"), false))
	{
	   int32_t zoneUIN = pRequest->getFieldAsUInt32(VID_ZONE_UIN);
      shared_ptr<Node> node = FindNodeByIP(zoneUIN, m_clientAddr);
      if (node != nullptr)
      {
         // Node already exist, force configuration poll
         node->setRecheckCapsFlag();
         node->forceConfigurationPoll();
      }
      else
      {
         DiscoveredAddress *info = MemAllocStruct<DiscoveredAddress>();
         info->ipAddr = m_clientAddr;
         info->zoneUIN = zoneUIN;
         info->ignoreFilter = TRUE;		// Ignore discovery filters and add node anyway
         info->sourceType = DA_SRC_AGENT_REGISTRATION;
         info->sourceNodeId = 0;
         g_nodePollerQueue.put(info);
      }
      msg.setField(VID_RCC, RCC_SUCCESS);
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
void ClientSession::getServerFile(NXCPMessage *request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request->getId());

   TCHAR name[MAX_PATH];
   request->getFieldAsString(VID_FILE_NAME, name, MAX_PATH);
   TCHAR *extension = _tcsrchr(name, _T('.'));
   bool soundFile = (extension != nullptr) && m_soundFileTypes.contains(extension);

	if ((m_systemAccessRights & SYSTEM_ACCESS_READ_SERVER_FILES) || soundFile)
	{
	   TCHAR fname[MAX_PATH];
      _tcslcpy(fname, g_netxmsdDataDir, MAX_PATH);
      _tcslcat(fname, DDIR_FILES, MAX_PATH);
      _tcslcat(fname, FS_PATH_SEPARATOR, MAX_PATH);
      _tcslcat(fname, GetCleanFileName(name), MAX_PATH);
		debugPrintf(4, _T("getServerFile: Requested file: %s"), fname);
		if (_taccess(fname, 0) == 0)
		{
			debugPrintf(5, _T("getServerFile: Sending file %s"), fname);
			if (SendFileOverNXCP(m_socket, request->getId(), fname, m_pCtx, 0, nullptr, nullptr, m_mutexSocketWrite))
			{
				debugPrintf(5, _T("getServerFile: File %s was successfully sent"), fname);
		      msg.setField(VID_RCC, RCC_SUCCESS);
			}
			else
			{
				debugPrintf(5, _T("getServerFile: Unable to send file %s: SendFileOverNXCP() failed"), fname);
		      msg.setField(VID_RCC, RCC_IO_ERROR);
			}
		}
		else
		{
			debugPrintf(5, _T("getServerFile: Unable to send file %s: access() failed"), fname);
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
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request->getId());

	bool success = false;

	shared_ptr<NetObj> object = FindObjectById(request->getFieldAsUInt32(VID_OBJECT_ID));
	if (object != nullptr)
	{
		if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_DOWNLOAD))
		{
			if (object->getObjectClass() == OBJECT_NODE)
			{
			   TCHAR remoteFile[MAX_PATH];
				request->getFieldAsString(VID_FILE_NAME, remoteFile, MAX_PATH);
            bool monitor = request->getFieldAsBoolean(VID_FILE_FOLLOW);
            shared_ptr<FileDownloadTask> task;
            if (request->getFieldAsBoolean(VID_EXPAND_STRING))
            {
               StringMap inputFields;
               inputFields.loadMessage(request, VID_INPUT_FIELD_COUNT, VID_INPUT_FIELD_BASE);
               Alarm *alarm = FindAlarmById(request->getFieldAsUInt32(VID_ALARM_ID));
               if ((alarm != nullptr) && !object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ_ALARMS) && !alarm->checkCategoryAccess(this))
               {
                  msg.setField(VID_RCC, RCC_ACCESS_DENIED);
               }
               else
               {
                  task = make_shared<FileDownloadTask>(static_pointer_cast<Node>(object), this, request->getId(),
                           object->expandText(remoteFile, alarm, nullptr, shared_ptr<DCObjectInfo>(), m_loginName, nullptr, &inputFields, nullptr),
                           true, request->getFieldAsUInt32(VID_FILE_SIZE_LIMIT), monitor);
                  success = true;
               }
               delete alarm;
            }
            else
            {
               task = make_shared<FileDownloadTask>(static_pointer_cast<Node>(object), this, request->getId(),
                        remoteFile, false, request->getFieldAsUInt32(VID_FILE_SIZE_LIMIT), monitor);
               success = true;
            }

				if (success)
				{
		         writeAuditLog(AUDIT_OBJECTS, true, object->getId(), _T("Initiated download of file \"%s\" from node %s"), remoteFile, object->getName());
				   ThreadPoolExecute(g_clientThreadPool, task, &FileDownloadTask::run);
				}
			}
			else
			{
				msg.setField(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
			}
		}
		else
		{
		   writeAuditLog(AUDIT_OBJECTS, false, object->getId(), _T("Access denied on download file from node %s"), object->getName());
			msg.setField(VID_RCC, RCC_ACCESS_DENIED);
		}
	}
	else
	{
		msg.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
	}

	if (!success) // In case of success download task will send response message with all required information
	   sendMessage(&msg);
}

/**
 * Cancel file monitoring
 */
void ClientSession::cancelFileMonitoring(NXCPMessage *request)
{
   NXCPMessage msg;
   NXCPMessage* response;
	UINT32 rcc = 0xFFFFFFFF;

   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(request->getId());

	shared_ptr<NetObj> object = FindObjectById(request->getFieldAsUInt32(VID_OBJECT_ID));
	if (object != nullptr)
	{
      if (object->getObjectClass() == OBJECT_NODE)
      {
         MONITORED_FILE file;
         request->getFieldAsString(VID_FILE_NAME, file.fileName, MAX_PATH);
         file.nodeID = object->getId();
         file.session = this;
         g_monitoringList.removeFile(&file);

         shared_ptr<AgentConnectionEx> conn = static_cast<Node&>(*object).createAgentConnection();
         debugPrintf(6, _T("Cancel file monitoring for %s"), file.fileName);
         if (conn != nullptr)
         {
            request->setProtocolVersion(conn->getProtocolVersion());
            request->setId(conn->generateRequestId());
            response = conn->customRequest(request);
            if (response != nullptr)
            {
               rcc = response->getFieldAsUInt32(VID_RCC);
               if (rcc == ERR_SUCCESS)
               {
                  msg.setField(VID_RCC, rcc);
                  debugPrintf(6, _T("cancelFileMonitoring(%s): success"), file.fileName);
               }
               else
               {
                  msg.setField(VID_RCC, AgentErrorToRCC(rcc));
                  debugPrintf(6, _T("cancelFileMonitoring(%s): agent error %d (%s)"), file.fileName, rcc, AgentErrorCodeToText(rcc));
               }
               delete response;
            }
            else
            {
               msg.setField(VID_RCC, RCC_INTERNAL_ERROR);
            }
         }
         else
         {
            msg.setField(VID_RCC, RCC_INTERNAL_ERROR);
            debugPrintf(6, _T("cancelFileMonitoring(%s): connection with node have been lost"), file.fileName);
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
 * Test DCI transformation script
 */
void ClientSession::testDCITransformation(NXCPMessage *pRequest)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, pRequest->getId());

   // Get node id and check object class and access rights
   shared_ptr<NetObj> object = FindObjectById(pRequest->getFieldAsUInt32(VID_OBJECT_ID));
   if (object != nullptr)
   {
      if (object->isDataCollectionTarget())
      {
         if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
         {
				TCHAR *script = pRequest->getFieldAsString(VID_SCRIPT);
				if (script != nullptr)
				{
				   shared_ptr<DCObjectInfo> dcObjectInfo;
				   if (pRequest->isFieldExist(VID_DCI_ID))
				   {
				      UINT32 dciId = pRequest->getFieldAsUInt32(VID_DCI_ID);
				      shared_ptr<DCObject> dcObject = static_cast<DataCollectionTarget&>(*object).getDCObjectById(dciId, m_dwUserId);
				      dcObjectInfo = make_shared<DCObjectInfo>(pRequest, dcObject.get());
				   }

				   TCHAR value[256], result[256];
					pRequest->getFieldAsString(VID_VALUE, value, sizeof(value) / sizeof(TCHAR));
               bool success = DCItem::testTransformation(static_cast<DataCollectionTarget&>(*object), dcObjectInfo, script, value, result, sizeof(result) / sizeof(TCHAR));
					MemFree(script);
					msg.setField(VID_RCC, RCC_SUCCESS);
					msg.setField(VID_EXECUTION_STATUS, success);
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
   NXSL_VM *vm = nullptr;

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(request->getId());

   // Get node id and check object class and access rights
   shared_ptr<NetObj> object = FindObjectById(request->getFieldAsUInt32(VID_OBJECT_ID));
   if (object != nullptr)
   {
      if ((object->getObjectClass() == OBJECT_NODE) ||
          (object->getObjectClass() == OBJECT_CLUSTER) ||
          (object->getObjectClass() == OBJECT_MOBILEDEVICE) ||
          (object->getObjectClass() == OBJECT_CHASSIS) ||
          (object->getObjectClass() == OBJECT_RACK) ||
          (object->getObjectClass() == OBJECT_CONTAINER) ||
          (object->getObjectClass() == OBJECT_ZONE) ||
          (object->getObjectClass() == OBJECT_SUBNET) ||
          (object->getObjectClass() == OBJECT_SENSOR) ||
          (object->getObjectClass() == OBJECT_NETWORK) ||
          (object->getObjectClass() == OBJECT_SERVICEROOT))
      {
         TCHAR *script = request->getFieldAsString(VID_SCRIPT);
         if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_MODIFY))
         {
				if (script != nullptr)
				{
               TCHAR errorMessage[256];
               vm = NXSLCompileAndCreateVM(script, errorMessage, 256, new NXSL_ClientSessionEnv(this, &msg));
               if (vm != nullptr)
               {
                  SetupServerScriptVM(vm, object, shared_ptr<DCObjectInfo>());
                  msg.setField(VID_RCC, RCC_SUCCESS);
                  sendMessage(&msg);
                  success = true;
                  writeAuditLogWithValues(AUDIT_OBJECTS, true, object->getId(), nullptr, script, 'T', _T("Executed ad-hoc script for object %s [%u]"), object->getName(), object->getId());
               }
               else
               {
                  msg.setField(VID_RCC, RCC_NXSL_COMPILATION_ERROR);
                  msg.setField(VID_ERROR_TEXT, errorMessage);
               }
				}
				else
				{
	            msg.setField(VID_RCC, RCC_INVALID_ARGUMENT);
				}
         }
         else  // User doesn't have READ rights on object
         {
            writeAuditLogWithValues(AUDIT_OBJECTS, false, object->getId(), nullptr, script, 'T', _T("Access denied on ad-hoc script execution for object %s [%u]"), object->getName(), object->getId());
            msg.setField(VID_RCC, RCC_ACCESS_DENIED);
         }
         MemFree(script);
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

      int count = request->getFieldAsInt16(VID_NUM_FIELDS);
      ObjectRefArray<NXSL_Value> sargs(count, 1);
      if (request->isFieldExist(VID_PARAMETER))
      {
         TCHAR parameters[1024];
         request->getFieldAsString(VID_PARAMETER, parameters, 1024);
         TCHAR *p = parameters;
         ParseValueList(vm, &p, sargs, false);
      }
      else if (count > 0)
      {
         UINT32 fieldId = VID_FIELD_LIST_BASE;
         for(int i = 0; i < count; i++)
         {
            SharedString value = request->getFieldAsSharedString(fieldId++);
            sargs.add(vm->createValue(value));
         }
      }

      if (vm->run(sargs))
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
 * Library script execution data
 */
class LibraryScriptExecutionData
{
public:
   NXSL_VM *vm;
   ObjectRefArray<NXSL_Value> args;
   TCHAR *name;

   LibraryScriptExecutionData(NXSL_VM *_vm, StringList *_args) : args(16, 16)
   {
      vm = _vm;
      for(int i = 1; i < _args->size(); i++)
         args.add(vm->createValue(_args->get(i)));
      name = _tcsdup(_args->get(0));
   }
   ~LibraryScriptExecutionData()
   {
      delete vm;
      free(name);
   }
};

/**
 * Callback for executing library script on separate thread pool
 */
static void ExecuteLibraryScript(void *arg)
{
   LibraryScriptExecutionData *d = (LibraryScriptExecutionData *)arg;
   nxlog_debug(6, _T("Starting background execution of library script %s"), d->name);
   if (d->vm->run(d->args))
   {
      nxlog_debug(6, _T("Background execution of library script %s completed"), d->name);
   }
   else
   {
      nxlog_debug(6, _T("Background execution of library script %s failed (%s)"), d->name, d->vm->getErrorText());
   }
   delete d;
}

/**
 * Execute library script in object's context
 */
void ClientSession::executeLibraryScript(NXCPMessage *request)
{
   NXCPMessage msg;
   bool success = false;
   NXSL_VM *vm = nullptr;
   StringList *args = nullptr;
   bool withOutput = request->getFieldAsBoolean(VID_RECEIVE_OUTPUT);

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(request->getId());

   // Get node id and check object class and access rights
   shared_ptr<NetObj> object = FindObjectById(request->getFieldAsUInt32(VID_OBJECT_ID));
   TCHAR *script = request->getFieldAsString(VID_SCRIPT);
   if (object != nullptr)
   {
      if ((object->getObjectClass() == OBJECT_NODE) ||
          (object->getObjectClass() == OBJECT_CLUSTER) ||
          (object->getObjectClass() == OBJECT_MOBILEDEVICE) ||
          (object->getObjectClass() == OBJECT_CHASSIS) ||
          (object->getObjectClass() == OBJECT_CONTAINER) ||
          (object->getObjectClass() == OBJECT_ZONE) ||
          (object->getObjectClass() == OBJECT_SUBNET) ||
          (object->getObjectClass() == OBJECT_SENSOR))
      {
         if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_CONTROL))
         {
            if (script != nullptr)
            {
               // Do macro expansion if target object is a node
               if (object->getObjectClass() == OBJECT_NODE)
               {
                  StringMap *inputFields;
                  int count = request->getFieldAsInt16(VID_NUM_FIELDS);
                  if (count > 0)
                  {
                     inputFields = new StringMap();
                     UINT32 fieldId = VID_FIELD_LIST_BASE;
                     for(int i = 0; i < count; i++)
                     {
                        TCHAR *name = request->getFieldAsString(fieldId++);
                        TCHAR *value = request->getFieldAsString(fieldId++);
                        inputFields->setPreallocated(name, value);
                     }
                  }
                  else
                  {
                     inputFields = nullptr;
                  }

                  Alarm *alarm = FindAlarmById(request->getFieldAsUInt32(VID_ALARM_ID));
                  if(alarm != nullptr && !object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ_ALARMS) && !alarm->checkCategoryAccess(this))
                  {
                     msg.setField(VID_RCC, RCC_ACCESS_DENIED);
                     sendMessage(&msg);
                     delete alarm;
                     delete inputFields;
                     return;
                  }
                  String expScript = object->expandText(script, alarm, nullptr, shared_ptr<DCObjectInfo>(), m_loginName, nullptr, inputFields, nullptr);
                  MemFree(script);
                  script = MemCopyString(expScript);
                  delete alarm;
                  delete inputFields;
               }

               args = ParseCommandLine(script);
               if (args->size() > 0)
               {
                  NXSL_Environment *env = withOutput ? new NXSL_ClientSessionEnv(this, &msg) : new NXSL_ServerEnv();
                  vm = GetServerScriptLibrary()->createVM(args->get(0), env);
                  if (vm != nullptr)
                  {
                     SetupServerScriptVM(vm, object, shared_ptr<DCObjectInfo>());
                     WriteAuditLog(AUDIT_OBJECTS, true, m_dwUserId, m_workstation, m_id, object->getId(), _T("'%s' script successfully executed."), CHECK_NULL(script));
                     msg.setField(VID_RCC, RCC_SUCCESS);
                     sendMessage(&msg);
                     success = true;
                  }
                  else
                  {
                     msg.setField(VID_RCC, RCC_INVALID_SCRIPT_NAME);
                  }
               }
               else
               {
                  msg.setField(VID_RCC, RCC_INVALID_ARGUMENT);
               }
            }
            else
            {
               msg.setField(VID_RCC, RCC_INVALID_ARGUMENT);
            }
         }
         else  // User doesn't have CONTROL rights on object
         {
			   writeAuditLog(AUDIT_OBJECTS, false, object->getId(), _T("Access denied on executing library script \"%s\" on object %s [%u]"),
                  CHECK_NULL(script), object->getName(), object->getId());
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
      writeAuditLog(AUDIT_OBJECTS, true, object->getId(), _T("Executed library script \"%s\" for object %s [%u]"), script, object->getName(), object->getId());
      if (withOutput)
      {
         ObjectRefArray<NXSL_Value> sargs(args->size() - 1, 1);
         for(int i = 1; i < args->size(); i++)
            sargs.add(vm->createValue(args->get(i)));
         msg.setCode(CMD_EXECUTE_SCRIPT_UPDATE);
         if (vm->run(sargs))
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
         ThreadPoolExecute(g_clientThreadPool, ExecuteLibraryScript, new LibraryScriptExecutionData(vm, args));
      }
   }
   else
   {
      // Send response
      sendMessage(&msg);
   }

   MemFree(script);
   delete args;
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
 * Get custom attribute for current user
 */
void ClientSession::getUserCustomAttribute(NXCPMessage *request)
{
	NXCPMessage msg;

	msg.setCode(CMD_REQUEST_COMPLETED);
	msg.setId(request->getId());

	TCHAR *name = request->getFieldAsString(VID_NAME);
	if ((name != nullptr) && (*name == _T('.')))
	{
		const TCHAR *value = GetUserDbObjectAttr(m_dwUserId, name);
		msg.setField(VID_VALUE, CHECK_NULL_EX(value));
		msg.setField(VID_RCC, RCC_SUCCESS);
	}
	else
	{
		msg.setField(VID_RCC, RCC_ACCESS_DENIED);
	}
	MemFree(name);

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
	if ((name != nullptr) && (*name == _T('.')))
	{
		TCHAR *value = request->getFieldAsString(VID_VALUE);
		SetUserDbObjectAttr(m_dwUserId, name, CHECK_NULL_EX(value));
		msg.setField(VID_RCC, RCC_SUCCESS);
		MemFree(value);
	}
	else
	{
		msg.setField(VID_RCC, RCC_ACCESS_DENIED);
	}
	MemFree(name);

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

	uint32_t rcc;
	int32_t handle = OpenLog(name, this, &rcc);
	if (handle != -1)
	{
		msg.setField(VID_RCC, RCC_SUCCESS);
		msg.setField(VID_LOG_HANDLE, handle);

		LogHandle *log = AcquireLogHandleObject(this, handle);
		log->getColumnInfo(&msg);
		log->release();
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

	int32_t handle = request->getFieldAsInt32(VID_LOG_HANDLE);
	LogHandle *log = AcquireLogHandleObject(this, handle);
	if (log != nullptr)
	{
		INT64 rowCount;
		msg.setField(VID_RCC, log->query(new LogFilter(*request, log), &rowCount, getUserId()) ? RCC_SUCCESS : RCC_DB_FAILURE);
		msg.setField(VID_NUM_ROWS, rowCount);
		log->release();
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
	Table *data = nullptr;

	msg.setCode(CMD_REQUEST_COMPLETED);
	msg.setId(request->getId());

	int32_t handle = (int)request->getFieldAsUInt32(VID_LOG_HANDLE);
	LogHandle *log = AcquireLogHandleObject(this, handle);
	if (log != nullptr)
	{
		INT64 startRow = request->getFieldAsUInt64(VID_START_ROW);
		INT64 numRows = request->getFieldAsUInt64(VID_NUM_ROWS);
		bool refresh = request->getFieldAsUInt16(VID_FORCE_RELOAD) ? true : false;
		data = log->getData(startRow, numRows, refresh, m_dwUserId); // pass user id from session
		log->release();
		if (data != nullptr)
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

	if (data != nullptr)
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
 * Get details for server log record
 */
void ClientSession::getServerLogRecordDetails(NXCPMessage *request)
{
   NXCPMessage msg;

   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(request->getId());

   int32_t handle = request->getFieldAsInt32(VID_LOG_HANDLE);
   LogHandle *log = AcquireLogHandleObject(this, handle);
   if (log != nullptr)
   {
      log->getRecordDetails(request->getFieldAsInt64(VID_RECORD_ID), &msg);
      log->release();
   }
   else
   {
      msg.setField(VID_RCC, RCC_INVALID_LOG_HANDLE);
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
   UINT32 rcc = RCC_SUCCESS;

	if (m_systemAccessRights & SYSTEM_ACCESS_SERVER_CONFIG)
	{
      int32_t zoneUIN = request->getFieldAsInt32(VID_ZONE_UIN);
      shared_ptr<Zone> zone = FindZoneByUIN(zoneUIN);
      if (zoneUIN == -1 || zone != nullptr)
      {
         DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
         if (DBBegin(hdb))
         {
            if (ExecuteQueryOnObject(hdb, zoneUIN, _T("DELETE FROM usm_credentials WHERE zone=?")))
            {
               DB_STATEMENT hStmt = DBPrepare(hdb, _T("INSERT INTO usm_credentials (id,user_name,auth_method,priv_method,auth_password,priv_password,comments,zone) VALUES(?,?,?,?,?,?,?,?)"), true);
               if (hStmt != nullptr)
               {
                  UINT32 id = VID_USM_CRED_LIST_BASE;
                  int count = (int)request->getFieldAsUInt32(VID_NUM_RECORDS);
                  DBBind(hStmt, 8, DB_SQLTYPE_INTEGER, zoneUIN);
                  for(int i = 0; i < count; i++, id += 4)
                  {
                     DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, i + 1);
                     DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, request->getFieldAsString(id++), DB_BIND_DYNAMIC);
                     DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, (int)request->getFieldAsUInt16(id++)); // Auth method
                     DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, (int)request->getFieldAsUInt16(id++)); // Priv method
                     DBBind(hStmt, 5, DB_SQLTYPE_VARCHAR, request->getFieldAsString(id++), DB_BIND_DYNAMIC);
                     DBBind(hStmt, 6, DB_SQLTYPE_VARCHAR, request->getFieldAsString(id++), DB_BIND_DYNAMIC);
                     DBBind(hStmt, 7, DB_SQLTYPE_VARCHAR, request->getFieldAsString(id++), DB_BIND_DYNAMIC);
                     if (!DBExecute(hStmt))
                     {
                        rcc = RCC_DB_FAILURE;
                        break;
                     }
                  }
                  DBFreeStatement(hStmt);
               }
               else
               {
                  rcc = RCC_DB_FAILURE;
               }
            }
            else
            {
               rcc = RCC_DB_FAILURE;
            }

            if (rcc == RCC_SUCCESS)
            {
               DBCommit(hdb);
               NotifyClientSessions(NX_NOTIFY_USM_CONFIG_CHANGED, zoneUIN);
            }
            else
               DBRollback(hdb);
         }
         else
         {
            rcc = RCC_DB_FAILURE;
         }
         DBConnectionPoolReleaseConnection(hdb);
      }
	}
	else
      rcc = RCC_ACCESS_DENIED;

   msg.setField(VID_RCC, rcc);
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
	shared_ptr<NetObj> object = FindObjectById(objectId);
	if ((object != nullptr) && !object->isDeleted())
	{
		if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
		{
			debugPrintf(5, _T("findNodeConnection: objectId=%d class=%d name=\"%s\""), objectId, object->getObjectClass(), object->getName());
			shared_ptr<NetObj> cp;
			UINT32 localNodeId, localIfId;
			BYTE localMacAddr[MAC_ADDR_LENGTH];
			int type = 0;
			if (object->getObjectClass() == OBJECT_NODE)
			{
				localNodeId = objectId;
				cp = static_cast<Node&>(*object).findConnectionPoint(&localIfId, localMacAddr, &type);
				msg.setField(VID_RCC, RCC_SUCCESS);
			}
			else if (object->getObjectClass() == OBJECT_INTERFACE)
			{
				localNodeId = static_cast<Interface&>(*object).getParentNodeId();
				localIfId = objectId;
				memcpy(localMacAddr, static_cast<Interface&>(*object).getMacAddr().value(), MAC_ADDR_LENGTH);
				cp = FindInterfaceConnectionPoint(MacAddress(localMacAddr, MAC_ADDR_LENGTH), &type);
				msg.setField(VID_RCC, RCC_SUCCESS);
			}
         else if (object->getObjectClass() == OBJECT_ACCESSPOINT)
			{
				localNodeId = 0;
				localIfId = 0;
				memcpy(localMacAddr, static_cast<AccessPoint&>(*object).getMacAddr().value(), MAC_ADDR_LENGTH);
				cp = FindInterfaceConnectionPoint(MacAddress(localMacAddr, MAC_ADDR_LENGTH), &type);
				msg.setField(VID_RCC, RCC_SUCCESS);
			}
			else
			{
				msg.setField(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
			}

			if (cp != nullptr)
         {
            debugPrintf(5, _T("findNodeConnection: cp=%s [%u] type=%d"), cp->getName(), cp->getId(), type);
            shared_ptr<Node> node = (cp->getObjectClass() == OBJECT_INTERFACE) ? static_cast<Interface&>(*cp).getParentNode() : static_cast<AccessPoint&>(*cp).getParentNode();
            if (node != nullptr)
            {
               msg.setField(VID_OBJECT_ID, node->getId());
				   msg.setField(VID_INTERFACE_ID, cp->getId());
               msg.setField(VID_IF_INDEX, (cp->getObjectClass() == OBJECT_INTERFACE) ? static_cast<Interface&>(*cp).getIfIndex() : (UINT32)0);
				   msg.setField(VID_LOCAL_NODE_ID, localNodeId);
				   msg.setField(VID_LOCAL_INTERFACE_ID, localIfId);
				   msg.setField(VID_MAC_ADDR, localMacAddr, MAC_ADDR_LENGTH);
				   msg.setField(VID_CONNECTION_TYPE, (UINT16)type);
               if (cp->getObjectClass() == OBJECT_INTERFACE)
                  debugPrintf(5, _T("findNodeConnection: nodeId=%d ifId=%d ifName=%s ifIndex=%d"), node->getId(), cp->getId(), cp->getName(), static_cast<Interface&>(*cp).getIfIndex());
               else
                  debugPrintf(5, _T("findNodeConnection: nodeId=%d apId=%d apName=%s"), node->getId(), cp->getId(), cp->getName());
            }
            else
            {
               debugPrintf(5, _T("findNodeConnection: cp=(null)"));
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
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request->getId());

	MacAddress macAddr = request->getFieldAsMacAddress(VID_MAC_ADDR);
	int type;
	shared_ptr<NetObj> cp = FindInterfaceConnectionPoint(macAddr, &type);
	msg.setField(VID_RCC, RCC_SUCCESS);

	if (cp != nullptr)
	{
      debugPrintf(5, _T("findMacAddress: cp=%s [%u] type=%d"), cp->getName(), cp->getId(), type);
		uint32_t localNodeId, localIfId;
		shared_ptr<Interface> localIf = FindInterfaceByMAC(macAddr);
		if (localIf != nullptr)
		{
			localIfId = localIf->getId();
			localNodeId = localIf->getParentNodeId();
		}
		else
		{
			localIfId = 0;
			localNodeId = 0;
		}

      shared_ptr<Node> node = (cp->getObjectClass() == OBJECT_INTERFACE) ? static_cast<Interface&>(*cp).getParentNode() : static_cast<AccessPoint&>(*cp).getParentNode();
      if (node != nullptr)
      {
		   msg.setField(VID_OBJECT_ID, node->getId());
		   msg.setField(VID_INTERFACE_ID, cp->getId());
         msg.setField(VID_IF_INDEX, (cp->getObjectClass() == OBJECT_INTERFACE) ? static_cast<Interface&>(*cp).getIfIndex() : (uint32_t)0);
	      msg.setField(VID_LOCAL_NODE_ID, localNodeId);
		   msg.setField(VID_LOCAL_INTERFACE_ID, localIfId);
		   msg.setField(VID_MAC_ADDR, macAddr);
         msg.setField(VID_IP_ADDRESS, (localIf != nullptr) ? localIf->getIpAddressList()->getFirstUnicastAddress() : InetAddress::INVALID);
		   msg.setField(VID_CONNECTION_TYPE, (UINT16)type);
         if (cp->getObjectClass() == OBJECT_INTERFACE)
            debugPrintf(5, _T("findMacAddress: nodeId=%d ifId=%d ifName=%s ifIndex=%d"), node->getId(), cp->getId(), cp->getName(), static_cast<Interface&>(*cp).getIfIndex());
         else
            debugPrintf(5, _T("findMacAddress: nodeId=%d apId=%d apName=%s"), node->getId(), cp->getId(), cp->getName());
      }
      else
      {
		   msg.setField(VID_RCC, RCC_INTERNAL_ERROR);
      }
	}
	else
	{
      debugPrintf(5, _T("findMacAddress: cp=(null)"));
      shared_ptr<Interface> localIf = FindInterfaceByMAC(macAddr);
      if (localIf != nullptr)
      {
         msg.setField(VID_LOCAL_NODE_ID, localIf->getParentNodeId());
         msg.setField(VID_LOCAL_INTERFACE_ID, localIf->getId());
         msg.setField(VID_MAC_ADDR, macAddr);
         msg.setField(VID_IP_ADDRESS, localIf->getIpAddressList()->getFirstUnicastAddress());

         if (localIf->getPeerInterfaceId() != 0)
         {
            msg.setField(VID_CONNECTION_TYPE, static_cast<uint16_t>(CP_TYPE_DIRECT));
            msg.setField(VID_OBJECT_ID, localIf->getPeerNodeId());
            shared_ptr<NetObj> remoteIf = FindObjectById(localIf->getPeerInterfaceId(), OBJECT_INTERFACE);
            if (remoteIf != nullptr)
            {
               msg.setField(VID_INTERFACE_ID, remoteIf->getId());
               msg.setField(VID_IF_INDEX, static_cast<Interface&>(*remoteIf).getIfIndex());
            }
         }
         else
         {
            msg.setField(VID_CONNECTION_TYPE, static_cast<uint16_t>(CP_TYPE_UNKNOWN));
         }

         TCHAR buffer[64];
         debugPrintf(5, _T("findMacAddress: MAC address %s not found in FDB but interface found (%s on %s [%u])"),
                     macAddr.toString(buffer), localIf->getName(), localIf->getParentNodeName().cstr(), localIf->getParentNodeId());
      }
	}

	sendMessage(&msg);
}

/**
 * Find connection port for given IP address
 */
void ClientSession::findIpAddress(NXCPMessage *request)
{
   TCHAR ipAddrText[64];
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request->getId());
	msg.setField(VID_RCC, RCC_SUCCESS);

	MacAddress macAddr;
	bool found = false;
	int32_t zoneUIN = request->getFieldAsUInt32(VID_ZONE_UIN);
	InetAddress ipAddr = request->getFieldAsInetAddress(VID_IP_ADDRESS);
	shared_ptr<Interface> iface = FindInterfaceByIP(zoneUIN, ipAddr);
	if ((iface != nullptr) && iface->getMacAddr().isValid())
	{
		macAddr = iface->getMacAddr();
		found = true;
		debugPrintf(5, _T("findIpAddress(%s): endpoint iface=%s"), ipAddr.toString(ipAddrText), iface->getName());
	}
	else
	{
		// no interface object with this IP or MAC address not known, try to find it in ARP caches
		debugPrintf(5, _T("findIpAddress(%s): interface not found, looking in ARP cache"), ipAddr.toString(ipAddrText));
		shared_ptr<Subnet> subnet = FindSubnetForNode(zoneUIN, ipAddr);
		if (subnet != nullptr)
		{
			debugPrintf(5, _T("findIpAddress(%s): found subnet %s"), ipAddrText, subnet->getName());
			macAddr = subnet->findMacAddress(ipAddr);
			found = macAddr.isValid();
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
		shared_ptr<NetObj> cp = FindInterfaceConnectionPoint(macAddr, &type);
		if (cp != nullptr)
		{
			uint32_t localNodeId, localIfId;
			shared_ptr<Interface> localIf = (iface != nullptr) ? iface : FindInterfaceByMAC(macAddr);
			if (localIf != nullptr)
			{
				localIfId = localIf->getId();
				localNodeId = localIf->getParentNodeId();
			}
			else
			{
				localIfId = 0;
				localNodeId = 0;
			}

         shared_ptr<Node> node = (cp->getObjectClass() == OBJECT_INTERFACE) ? static_cast<Interface&>(*cp).getParentNode() : static_cast<AccessPoint&>(*cp).getParentNode();
         if (node != nullptr)
         {
		      msg.setField(VID_OBJECT_ID, node->getId());
			   msg.setField(VID_INTERFACE_ID, cp->getId());
            msg.setField(VID_IF_INDEX, (cp->getObjectClass() == OBJECT_INTERFACE) ? static_cast<Interface&>(*cp).getIfIndex() : (UINT32)0);
			   msg.setField(VID_LOCAL_NODE_ID, localNodeId);
			   msg.setField(VID_LOCAL_INTERFACE_ID, localIfId);
			   msg.setField(VID_MAC_ADDR, macAddr);
			   msg.setField(VID_IP_ADDRESS, ipAddr);
			   msg.setField(VID_CONNECTION_TYPE, (UINT16)type);
            if (cp->getObjectClass() == OBJECT_INTERFACE)
            {
               debugPrintf(5, _T("findIpAddress(%s): nodeId=%d ifId=%d ifName=%s ifIndex=%d"), ipAddr.toString(ipAddrText),
                        node->getId(), cp->getId(), cp->getName(), static_cast<Interface&>(*cp).getIfIndex());
            }
            else
            {
               debugPrintf(5, _T("findIpAddress(%s): nodeId=%d apId=%d apName=%s"), ipAddr.toString(ipAddrText),
                        node->getId(), cp->getId(), cp->getName());
            }
         }
		}
		else if (iface != nullptr)
		{
		   // Connection not found but node with given IP address registered in the system
         msg.setField(VID_CONNECTION_TYPE, (UINT16)CP_TYPE_UNKNOWN);
         msg.setField(VID_MAC_ADDR, macAddr);
         msg.setField(VID_IP_ADDRESS, ipAddr);
         msg.setField(VID_LOCAL_NODE_ID, iface->getParentNodeId());
         msg.setField(VID_LOCAL_INTERFACE_ID, iface->getId());
		}
	}
   else if (iface != nullptr)
   {
      // Connection not found but node with given IP address registered in the system
      msg.setField(VID_CONNECTION_TYPE, (UINT16)CP_TYPE_UNKNOWN);
      msg.setField(VID_IP_ADDRESS, ipAddr);
      msg.setField(VID_LOCAL_NODE_ID, iface->getParentNodeId());
      msg.setField(VID_LOCAL_INTERFACE_ID, iface->getId());
   }

	sendMessage(&msg);
}

/**
 * Find nodes by hostname
 */
void ClientSession::findHostname(NXCPMessage *request)
{
   NXCPMessage msg;

   msg.setId(request->getId());
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setField(VID_RCC, RCC_SUCCESS);

   int32_t zoneUIN = request->getFieldAsUInt32(VID_ZONE_UIN);
   TCHAR hostname[MAX_DNS_NAME];
   request->getFieldAsString(VID_HOSTNAME, hostname, MAX_DNS_NAME);

   SharedObjectArray<NetObj> *nodes = FindNodesByHostname(zoneUIN, hostname);

   msg.setField(VID_NUM_ELEMENTS, nodes->size());

   uint32_t fieldId = VID_ELEMENT_LIST_BASE;
   for(int i = 0; i < nodes->size(); i++)
   {
      msg.setField(fieldId++, nodes->get(i)->getId());
   }
   delete nodes;

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
	   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

		TCHAR query[MAX_DB_STRING];
		_sntprintf(query, MAX_DB_STRING, _T("SELECT name,category,mimetype,protected FROM images WHERE guid = '%s'"), guidText);
		DB_RESULT result = DBSelect(hdb, query);
		if (result != nullptr)
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

				NX_STAT_STRUCT st;
				if ((CALL_STAT(absFileName, &st) == 0) && S_ISREG(st.st_mode))
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
		DBConnectionPoolReleaseConnection(hdb);
	}

	msg.setField(VID_RCC, rcc);
	sendMessage(&msg);

	if (rcc == RCC_SUCCESS)
		sendFile(absFileName, request->getId(), 0);
}

/**
 * Handle image library change
 */
void ClientSession::onLibraryImageChange(const uuid& guid, bool removed)
{
   if (guid.isNull() || !isAuthenticated())
      return;

   NXCPMessage msg(CMD_IMAGE_LIBRARY_UPDATE, 0);
   msg.setField(VID_GUID, guid);
   msg.setField(VID_FLAGS, (UINT32)(removed ? 1 : 0));
   postMessage(msg);
}

/**
 * Update library image from client
 */
void ClientSession::updateLibraryImage(NXCPMessage *request)
{
	NXCPMessage msg(CMD_REQUEST_COMPLETED, request->getId());
   if (!checkSysAccessRights(SYSTEM_ACCESS_MANAGE_IMAGE_LIB))
   {
	   msg.setField(VID_RCC, RCC_ACCESS_DENIED);
      sendMessage(&msg);
      return;
   }

   TCHAR name[MAX_OBJECT_NAME] = _T("");
	TCHAR category[MAX_OBJECT_NAME] = _T("");
	TCHAR mimetype[MAX_DB_STRING] = _T("");
	TCHAR absFileName[MAX_PATH] = _T("");

	uuid guid = request->getFieldAsGUID(VID_GUID);
	if (guid.isNull())
	   guid = uuid::generate();

	TCHAR guidText[64];
	guid.toString(guidText);

	request->getFieldAsString(VID_NAME, name, MAX_OBJECT_NAME);
	request->getFieldAsString(VID_CATEGORY, category, MAX_OBJECT_NAME);
	request->getFieldAsString(VID_IMAGE_MIMETYPE, mimetype, MAX_DB_STRING);

	// Set default values for empty fields
	if (name[0] == 0)
		_tcscpy(name, guidText);
	if (category[0] == 0)
		_tcscpy(category, _T("Default"));
	if (mimetype[0] == 0)
		_tcscpy(mimetype, _T("image/unknown"));

	debugPrintf(5, _T("updateLibraryImage: guid=%s, name=%s, category=%s"), guidText, name, category);

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

	uint32_t rcc = RCC_SUCCESS;
   TCHAR query[MAX_DB_STRING];
   _sntprintf(query, MAX_DB_STRING, _T("SELECT protected FROM images WHERE guid = '%s'"), guidText);
   DB_RESULT result = DBSelect(hdb, query);
   if (result != nullptr)
   {
      int count = DBGetNumRows(result);
      TCHAR query[MAX_DB_STRING] = {0};
      if (count > 0)
      {
         BOOL isProtected = DBGetFieldLong(result, 0, 0) != 0; // protected

         if (!isProtected)
         {
            _sntprintf(query, MAX_DB_STRING, _T("UPDATE images SET name = %s, category = %s, mimetype = %s WHERE guid = '%s'"),
                  (const TCHAR *)DBPrepareString(hdb, name),
                  (const TCHAR *)DBPrepareString(hdb, category),
                  (const TCHAR *)DBPrepareString(hdb, mimetype, 32),
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
               (const TCHAR *)DBPrepareString(hdb, name),
               (const TCHAR *)DBPrepareString(hdb, category),
               (const TCHAR *)DBPrepareString(hdb, mimetype, 32));
      }

      if (query[0] != 0)
      {
         if (DBQuery(hdb, query))
         {
            // DB up to date, update file)
            _sntprintf(absFileName, MAX_PATH, _T("%s%s%s%s"), g_netxmsdDataDir, DDIR_IMAGES, FS_PATH_SEPARATOR, guidText);
            debugPrintf(5, _T("updateLibraryImage: guid=%s, absFileName=%s"), guidText, absFileName);

            ServerDownloadFileInfo *dInfo = new ServerDownloadFileInfo(absFileName, CMD_MODIFY_IMAGE);
            if (dInfo->open())
            {
               dInfo->setImageGuid(guid);
               m_downloadFileMap->set(request->getId(), dInfo);
            }
            else
            {
               rcc = RCC_IO_ERROR;
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

	if (rcc == RCC_SUCCESS)
	{
		msg.setField(VID_GUID, guid);
	}

   DBConnectionPoolReleaseConnection(hdb);
	msg.setField(VID_RCC, rcc);
	sendMessage(&msg);
}

/**
 * Delete image from library
 */
void ClientSession::deleteLibraryImage(NXCPMessage *request)
{
	NXCPMessage msg(CMD_REQUEST_COMPLETED, request->getId());

   if (!checkSysAccessRights(SYSTEM_ACCESS_MANAGE_IMAGE_LIB))
   {
	   msg.setField(VID_RCC, RCC_ACCESS_DENIED);
      sendMessage(&msg);
      return;
   }

	uuid guid = request->getFieldAsGUID(VID_GUID);
   TCHAR guidText[64];
	guid.toString(guidText);
	debugPrintf(5, _T("deleteLibraryImage: guid=%s"), guidText);

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   uint32_t rcc = RCC_SUCCESS;
   TCHAR query[MAX_DB_STRING];
	_sntprintf(query, MAX_DB_STRING, _T("SELECT protected FROM images WHERE guid = '%s'"), guidText);
	DB_RESULT hResult = DBSelect(hdb, query);
	if (hResult != nullptr)
	{
		if (DBGetNumRows(hResult) > 0)
		{
			if (DBGetFieldLong(hResult, 0, 0) == 0)
			{
				_sntprintf(query, MAX_DB_STRING, _T("DELETE FROM images WHERE protected = 0 AND guid = '%s'"), guidText);
				if (DBQuery(hdb, query))
				{
				   TCHAR fileName[MAX_PATH];
               _sntprintf(fileName, MAX_PATH, _T("%s%s%s%s"), g_netxmsdDataDir, DDIR_IMAGES, FS_PATH_SEPARATOR, guidText);
               DbgPrintf(5, _T("deleteLibraryImage: guid=%s, fileName=%s"), guidText, fileName);
               _tremove(fileName);
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
		EnumerateClientSessions(ImageLibraryDeleteCallback, &guid);
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

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

	_tcscpy(query, _T("SELECT guid,name,category,mimetype,protected FROM images"));
	if (category[0] != 0)
	{
		_tcscat(query, _T(" WHERE category = "));
      _tcscat(query, (const TCHAR *)DBPrepareString(hdb, category));
	}

	DB_RESULT result = DBSelect(hdb, query);
	if (result != nullptr)
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

	uint32_t nodeId = request->getFieldAsUInt32(VID_OBJECT_ID);
	shared_ptr<NetObj> object = FindObjectById(nodeId);
	if (object != nullptr)
	{
		if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_CONTROL))
		{
			if (object->getObjectClass() == OBJECT_NODE)
			{
			   shared_ptr<ServerCommandExecutor> cmd = make_shared<ServerCommandExecutor>(request, this);
			   if (cmd->execute())
			   {
			      pid_t taskId = cmd->getProcessId();
			      debugPrintf(5, _T("Started process executor %u for command %s"), static_cast<uint32_t>(taskId), cmd->getMaskedCommand());
			      m_serverCommands->set(taskId, cmd);
               writeAuditLog(AUDIT_OBJECTS, true, nodeId, _T("Server command executed: %s"), cmd->getMaskedCommand());
               msg.setField(VID_COMMAND_ID, static_cast<uint64_t>(taskId));
               msg.setField(VID_RCC, RCC_SUCCESS);
			   }
			   else
			   {
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
			writeAuditLog(AUDIT_OBJECTS, false, nodeId, _T("Access denied on server command execution"));
		}
	}
	else
	{
		msg.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
	}

	sendMessage(&msg);
}

/**
 * Stop server command
 */
void ClientSession::stopServerCommand(NXCPMessage *request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request->getId());

   shared_ptr<ProcessExecutor> cmd = m_serverCommands->getShared(static_cast<pid_t>(request->getFieldAsUInt64(VID_COMMAND_ID)));
   if (cmd != nullptr)
   {
      cmd->stop();
      msg.setField(VID_RCC, RCC_SUCCESS);
      sendMessage(&msg);
   }
   else
   {
      msg.setField(VID_RCC, RCC_INVALID_REQUEST);
      sendMessage(&msg);
   }
}

/**
 * Wait for process executor completion
 */
static void WaitForExecutorCompletion(const shared_ptr<ProcessExecutor>& executor)
{
   executor->waitForCompletion(INFINITE);
   nxlog_debug_tag(DEBUG_TAG, 5, _T("Process executor %u completed"), static_cast<uint32_t>(executor->getProcessId()));
}

/**
 * Unregister server command (expected to be called from executor itself)
 */
void ClientSession::unregisterServerCommand(pid_t taskId)
{
   shared_ptr<ProcessExecutor> executor = m_serverCommands->getShared(taskId);
   if (executor != nullptr)
   {
      m_serverCommands->remove(taskId);
      ThreadPoolExecuteSerialized(g_clientThreadPool, _T("UnregisterCmd"), WaitForExecutorCompletion, executor);
   }
}

/**
 * Upload file from server to agent
 */
void ClientSession::uploadFileToAgent(NXCPMessage *request)
{
	NXCPMessage msg(CMD_REQUEST_COMPLETED, request->getId());

	uint32_t nodeId = request->getFieldAsUInt32(VID_OBJECT_ID);
	shared_ptr<NetObj> object = FindObjectById(nodeId);
	if (object != nullptr)
	{
		if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_CONTROL))
		{
			if (object->getObjectClass() == OBJECT_NODE)
			{
				TCHAR *localFile = request->getFieldAsString(VID_FILE_NAME);
				TCHAR *remoteFile = request->getFieldAsString(VID_DESTINATION_FILE_NAME);
				if (localFile != nullptr)
				{
					auto job = new FileUploadJob(static_pointer_cast<Node>(object), localFile, remoteFile, m_dwUserId, request->getFieldAsBoolean(VID_CREATE_JOB_ON_HOLD));
					if (AddJob(job))
					{
						writeAuditLog(AUDIT_OBJECTS, true, nodeId, _T("File upload to node %s initiated, local=\"%s\", remote=\"%s\""), object->getName(), CHECK_NULL(localFile), CHECK_NULL(remoteFile));
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
				MemFree(localFile);
				MemFree(remoteFile);
			}
			else
			{
				msg.setField(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
			}
		}
		else
		{
			msg.setField(VID_RCC, RCC_ACCESS_DENIED);
			writeAuditLog(AUDIT_OBJECTS, false, nodeId, _T("Access denied on upload file to node %s"), object->getName());
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
	msg.setId(request->getId());
	msg.setCode(CMD_REQUEST_COMPLETED);

	int length = (int)request->getFieldAsUInt32(VID_EXTENSION_COUNT);
	debugPrintf(8, _T("ClientSession::listServerFileStore: length of filter type array is %d."), length);

   uint32_t varId = VID_EXTENSION_LIST_BASE;
   StringList extensionList;
   bool soundFiles = (length > 0);
	for(int i = 0; i < length; i++)
   {
	   TCHAR ext[256];
	   request->getFieldAsString(varId++, ext, 256);
	   if (!m_soundFileTypes.contains(ext))
	      soundFiles = false;;
      extensionList.add(ext);
   }

	if ((m_systemAccessRights & SYSTEM_ACCESS_READ_SERVER_FILES) || soundFiles)
	{
	   TCHAR path[MAX_PATH];
      _tcslcpy(path, g_netxmsdDataDir, MAX_PATH);
      _tcslcat(path, DDIR_FILES, MAX_PATH);
      _TDIR *dir = _topendir(path);
      if (dir != nullptr)
      {
         _tcscat(path, FS_PATH_SEPARATOR);
         int pos = (int)_tcslen(path);

         struct _tdirent *d;
         NX_STAT_STRUCT st;
         UINT32 count = 0, varId = VID_INSTANCE_LIST_BASE;
         while((d = _treaddir(dir)) != nullptr)
         {
            if (_tcscmp(d->d_name, _T(".")) && _tcscmp(d->d_name, _T("..")))
            {
               if(length != 0)
               {
                  bool correctType = false;
                  TCHAR *extension = _tcsrchr(d->d_name, _T('.'));
                  if (extension != nullptr)
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
               _tcslcpy(&path[pos], d->d_name, MAX_PATH - pos);
               if (CALL_STAT(path, &st) == 0)
               {
                  if (S_ISREG(st.st_mode))
                  {
                     msg.setField(varId++, d->d_name);
                     msg.setField(varId++, (uint64_t)st.st_size);
                     msg.setField(varId++, (uint64_t)st.st_mtime);
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

	if (m_systemAccessRights & SYSTEM_ACCESS_SERVER_CONSOLE)
	{
      if (!(m_flags & CSF_CONSOLE_OPEN))
      {
         InterlockedOr(&m_flags, CSF_CONSOLE_OPEN);
         m_console = new ClientSessionConsole(this);
      }
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

	if (m_systemAccessRights & SYSTEM_ACCESS_SERVER_CONSOLE)
	{
		if (m_flags & CSF_CONSOLE_OPEN)
		{
		   InterlockedAnd(&m_flags, ~CSF_CONSOLE_OPEN);
			delete_and_null(m_console);
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

/**
 * Process console command
 */
void ClientSession::processConsoleCommand(NXCPMessage *request)
{
	NXCPMessage msg;

	msg.setCode(CMD_REQUEST_COMPLETED);
	msg.setId(request->getId());

	if ((m_systemAccessRights & SYSTEM_ACCESS_SERVER_CONSOLE) && (m_flags & CSF_CONSOLE_OPEN))
	{
		TCHAR command[256];
		request->getFieldAsString(VID_COMMAND, command, 256);
		int rc = ProcessConsoleCommand(command, m_console);
      switch(rc)
      {
         case CMD_EXIT_SHUTDOWN:
            InitiateShutdown(ShutdownReason::FROM_REMOTE_CONSOLE);
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

/**
 * Get VLANs configured on device
 */
void ClientSession::getVlans(NXCPMessage *request)
{
	NXCPMessage msg;

	msg.setCode(CMD_REQUEST_COMPLETED);
	msg.setId(request->getId());

	shared_ptr<NetObj> object = FindObjectById(request->getFieldAsUInt32(VID_OBJECT_ID));
	if (object != nullptr)
	{
		if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
		{
			if (object->getObjectClass() == OBJECT_NODE)
			{
				shared_ptr<VlanList> vlans = static_cast<Node&>(*object).getVlans();
				if (vlans != nullptr)
				{
					vlans->fillMessage(&msg);
					msg.setField(VID_RCC, RCC_SUCCESS);
		         writeAuditLog(AUDIT_OBJECTS, true, object->getId(), _T("VLAN information read"));
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
			writeAuditLog(AUDIT_OBJECTS, false, object->getId(), _T("Access denied on reading VLAN information"));
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
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request->getId());

	if (m_systemAccessRights & SYSTEM_ACCESS_MANAGE_SERVER_FILES)
   {
		TCHAR fileName[MAX_PATH];
		TCHAR fullPath[MAX_PATH];

      request->getFieldAsString(VID_FILE_NAME, fileName, MAX_PATH);
      const TCHAR *cleanFileName = GetCleanFileName(fileName);
      _tcscpy(fullPath, g_netxmsdDataDir);
      _tcscat(fullPath, DDIR_FILES);
      _tcscat(fullPath, FS_PATH_SEPARATOR);
      _tcscat(fullPath, cleanFileName);

      ServerDownloadFileInfo *fInfo = new ServerDownloadFileInfo(fullPath, CMD_UPLOAD_FILE, request->getFieldAsTime(VID_MODIFICATION_TIME));

      if (fInfo->open())
      {
         m_downloadFileMap->set(request->getId(), fInfo);
         msg.setField(VID_RCC, RCC_SUCCESS);
         writeAuditLog(AUDIT_SYSCFG, true, 0, _T("Started upload of file \"%s\" to server"), fileName);
         NotifyClientSessions(NX_NOTIFY_FILE_LIST_CHANGED, 0);
      }
      else
      {
         delete fInfo;
         msg.setField(VID_RCC, RCC_IO_ERROR);
      }
   }
   else
   {
      writeAuditLog(AUDIT_SYSCFG, false, 0, _T("Access denied on upload file to server"));
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

	if (m_systemAccessRights & SYSTEM_ACCESS_MANAGE_SERVER_FILES)
   {
		TCHAR fileName[MAX_PATH];
		TCHAR fullPath[MAX_PATH];

      request->getFieldAsString(VID_FILE_NAME, fileName, MAX_PATH);
      const TCHAR *cleanFileName = GetCleanFileName(fileName);

      _tcslcpy(fullPath, g_netxmsdDataDir, MAX_PATH);
      _tcslcat(fullPath, DDIR_FILES, MAX_PATH);
      _tcslcat(fullPath, FS_PATH_SEPARATOR, MAX_PATH);
      _tcslcat(fullPath, cleanFileName, MAX_PATH);

      if (_tunlink(fullPath) == 0)
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

	shared_ptr<NetObj> node1 = FindObjectById(request->getFieldAsUInt32(VID_SOURCE_OBJECT_ID));
	shared_ptr<NetObj> node2 = FindObjectById(request->getFieldAsUInt32(VID_DESTINATION_OBJECT_ID));

	if ((node1 != nullptr) && (node2 != nullptr))
	{
		if (node1->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ) &&
		    node2->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
		{
			if ((node1->getObjectClass() == OBJECT_NODE) && (node2->getObjectClass() == OBJECT_NODE))
			{
				shared_ptr<NetworkPath> path = TraceRoute(static_pointer_cast<Node>(node1), static_pointer_cast<Node>(node2));
				if (path != nullptr)
				{
					msg.setField(VID_RCC, RCC_SUCCESS);
					path->fillMessage(&msg);
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
   shared_ptr<NetObj> node = FindObjectById(request->getFieldAsUInt32(VID_OBJECT_ID), OBJECT_NODE);
   if (node != nullptr)
   {
      if (node->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
      {
			shared_ptr<ComponentTree> components = static_cast<Node&>(*node).getComponents();
			if (components != nullptr)
			{
				msg.setField(VID_RCC, RCC_SUCCESS);
				components->fillMessage(&msg, VID_COMPONENT_LIST_BASE);
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
   shared_ptr<NetObj> node = FindObjectById(request->getFieldAsUInt32(VID_OBJECT_ID), OBJECT_NODE);
   if (node != nullptr)
   {
      if (node->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
      {
         static_cast<Node&>(*node).writePackageListToMessage(&msg);
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
 * Get list of hardware installed on node
 */
void ClientSession::getNodeHardware(NXCPMessage *request)
{
   NXCPMessage msg;

   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(request->getId());

   shared_ptr<NetObj> node = FindObjectById(request->getFieldAsUInt32(VID_OBJECT_ID), OBJECT_NODE);
   if (node != nullptr)
   {
      if (node->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
         static_cast<Node&>(*node).writeHardwareListToMessage(&msg);
      else
         msg.setField(VID_RCC, RCC_ACCESS_DENIED);
   }
   else
      msg.setField(VID_RCC, RCC_INVALID_OBJECT_ID);

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
   shared_ptr<NetObj> node = FindObjectById(request->getFieldAsUInt32(VID_OBJECT_ID), OBJECT_NODE);
   if (node != nullptr)
   {
      if (node->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
      {
			static_cast<Node&>(*node).writeWinPerfObjectsToMessage(&msg);
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
   shared_ptr<NetObj> object = FindObjectById(request->getFieldAsUInt32(VID_OBJECT_ID));
   if (object != nullptr)
   {
      if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
      {
			if (object->showThresholdSummary())
			{
				auto targets = new SharedObjectArray<DataCollectionTarget>();
				object->addChildDCTargetsToList(targets, m_dwUserId);
				UINT32 varId = VID_THRESHOLD_BASE;
				for(int i = 0; i < targets->size(); i++)
				{
					if (targets->get(i)->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
						varId = targets->get(i)->getThresholdSummary(&msg, varId, m_dwUserId);
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
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request->getId());

	if (m_systemAccessRights & SYSTEM_ACCESS_MANAGE_MAPPING_TBLS)
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

	if (m_systemAccessRights & SYSTEM_ACCESS_MANAGE_MAPPING_TBLS)
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

	if (m_systemAccessRights & SYSTEM_ACCESS_MANAGE_MAPPING_TBLS)
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
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request->getId());

   // Get object id and check object class and access rights
	shared_ptr<NetObj> node = FindObjectById(request->getFieldAsUInt32(VID_OBJECT_ID), OBJECT_NODE);
   if (node != nullptr)
   {
      if (node->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
      {
			if (static_cast<Node&>(*node).isWirelessController())
			{
			   static_cast<Node&>(*node).writeWsListToMessage(&msg);
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
   NXCPMessage msg(CMD_REQUEST_COMPLETED, rqId);

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_RESULT hResult = DBSelect(hdb, _T("SELECT id,menu_path,title,flags,guid FROM dci_summary_tables"));
   if (hResult != nullptr)
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
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request->getId());

	if (m_systemAccessRights & SYSTEM_ACCESS_MANAGE_SUMMARY_TBLS)
	{
      LONG id = (LONG)request->getFieldAsUInt32(VID_SUMMARY_TABLE_ID);
      DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
      DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT menu_path,title,node_filter,flags,columns,guid,table_dci_name FROM dci_summary_tables WHERE id=?"));
      if (hStmt != nullptr)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, id);
         DB_RESULT hResult = DBSelectPrepared(hStmt);
         if (hResult != nullptr)
         {
            if (DBGetNumRows(hResult) > 0)
            {
               TCHAR buffer[256];
               msg.setField(VID_SUMMARY_TABLE_ID, (UINT32)id);
               msg.setField(VID_MENU_PATH, DBGetField(hResult, 0, 0, buffer, 256));
               msg.setField(VID_TITLE, DBGetField(hResult, 0, 1, buffer, 256));
               TCHAR *tmp = DBGetField(hResult, 0, 2, nullptr, 0);
               if (tmp != nullptr)
               {
                  msg.setField(VID_FILTER, tmp);
                  free(tmp);
               }
               msg.setField(VID_FLAGS, DBGetFieldULong(hResult, 0, 3));
               tmp = DBGetField(hResult, 0, 4, nullptr, 0);
               if (tmp != nullptr)
               {
                  msg.setField(VID_COLUMNS, tmp);
                  free(tmp);
               }
               uuid guid = DBGetFieldGUID(hResult, 0, 5);
               msg.setField(VID_GUID, guid);
               msg.setField(VID_DCI_NAME, DBGetField(hResult, 0, 6, buffer, 256));
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
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request->getId());

	if (m_systemAccessRights & SYSTEM_ACCESS_MANAGE_SUMMARY_TBLS)
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
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request->getId());

	if (m_systemAccessRights & SYSTEM_ACCESS_MANAGE_SUMMARY_TBLS)
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
   Table *result = QuerySummaryTable((LONG)request->getFieldAsUInt32(VID_SUMMARY_TABLE_ID), nullptr,
                                      request->getFieldAsUInt32(VID_OBJECT_ID),
                                      m_dwUserId, &rcc);
   if (result != nullptr)
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
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request->getId());

   SummaryTable *tableDefinition = new SummaryTable(request);

   UINT32 rcc;
   Table *result = QuerySummaryTable(0, tableDefinition,
                                      request->getFieldAsUInt32(VID_OBJECT_ID),
                                      m_dwUserId, &rcc);
   if (result != nullptr)
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
   NXCPMessage *msg = nullptr;

   if (checkSysAccessRights(SYSTEM_ACCESS_REPORTING_SERVER))
   {
      TCHAR buffer[256];
	   debugPrintf(7, _T("RS: Forwarding message %s"), NXCPMessageCodeName(request->getCode(), buffer));

	   request->setField(VID_USER_NAME, m_loginName);
	   msg = ForwardMessageToReportingServer(request, this);
	   if (msg == nullptr)
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
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request->getId());

   // Get node id and check object class and access rights
   shared_ptr<NetObj> subnet = FindObjectById(request->getFieldAsUInt32(VID_OBJECT_ID), OBJECT_SUBNET);
   if (subnet != nullptr)
   {
      if (subnet->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
      {
         int length;
         UINT32 *map = static_cast<Subnet&>(*subnet).buildAddressMap(&length);
			if (map != nullptr)
			{
				msg.setField(VID_RCC, RCC_SUCCESS);
            msg.setFieldFromInt32Array(VID_ADDRESS_MAP, (UINT32)length, map);
            MemFree(map);
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
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request->getId());

   // Get node id and check object class and access rights
   shared_ptr<NetObj> object = FindObjectById(request->getFieldAsUInt32(VID_OBJECT_ID));
   if (object != nullptr)
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
   NXCPMessage msg, *response = nullptr, *responseMessage;
	UINT32 rcc = RCC_INTERNAL_ERROR;
   responseMessage = &msg;

   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(request->getId());

   TCHAR fileName[MAX_PATH];
   request->getFieldAsString(VID_FILE_NAME, fileName, MAX_PATH);
   UINT32 objectId = request->getFieldAsUInt32(VID_OBJECT_ID);
	shared_ptr<NetObj> object = FindObjectById(objectId);
	if (object != nullptr)
	{
		if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_MANAGE_FILES) ||
         (request->getCode() == CMD_GET_FOLDER_CONTENT && object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ)) ||
		 (request->getCode() == CMD_GET_FOLDER_SIZE && object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ)))
		{
			if (object->getObjectClass() == OBJECT_NODE)
			{
            shared_ptr<AgentConnectionEx> conn = static_cast<Node&>(*object).createAgentConnection();
            if (conn != nullptr)
            {
               request->setId(conn->generateRequestId());
               request->setProtocolVersion(conn->getProtocolVersion());
               if ((request->getCode() == CMD_GET_FOLDER_CONTENT) && request->getFieldAsBoolean(VID_ALLOW_MULTIPART))
               {
                  debugPrintf(5, _T("Processing multipart agent folder content request"));
                  if (conn->sendMessage(request))
                  {
                     while(true)
                     {
                        response = conn->waitForMessage(CMD_REQUEST_COMPLETED, request->getId(), g_agentCommandTimeout);
                        // old agent versions do not support multipart messages for folder content
                        // and will return single request completion message without VID_ALLOW_MULTIPART set
                        if ((response == nullptr) || !response->getFieldAsBoolean(VID_ALLOW_MULTIPART))
                        {
                           if (response != nullptr)
                              debugPrintf(5, _T("Received agent response without multipart flag set"));
                           break;
                        }
                        response->setId(msg.getId());
                        response->setProtocolVersion(NXCP_VERSION);
                        sendMessage(response);
                        if (response->isEndOfSequence())
                        {
                           delete response;
                           response = conn->waitForMessage(CMD_REQUEST_COMPLETED, request->getId(), g_agentCommandTimeout);
                           break;
                        }
                        delete response;
                     }
                  }
                  else
                  {
                     response = new NXCPMessage();
                     response->setField(VID_RCC, ERR_CONNECTION_BROKEN);
                  }
               }
               else
               {
                  response = conn->customRequest(request);
               }

               if (response != nullptr)
               {
                  rcc = response->getFieldAsUInt32(VID_RCC);
                  if (rcc == ERR_SUCCESS)
                  {
                     response->setId(msg.getId());
                     response->setCode(CMD_REQUEST_COMPLETED);
                     response->setProtocolVersion(NXCP_VERSION);
                     responseMessage = response;

                     // Add line in audit log
                     switch(request->getCode())
                     {
                        case CMD_GET_FOLDER_SIZE:
                           writeAuditLog(AUDIT_OBJECTS, true, objectId, _T("Get size of directory \"%s\" on node %s"), fileName, object->getName());
                           break;
                        case CMD_GET_FOLDER_CONTENT:
                           writeAuditLog(AUDIT_OBJECTS, true, objectId, _T("Get content of directory \"%s\" on node %s"), fileName, object->getName());
                           break;
                        case CMD_FILEMGR_DELETE_FILE:
                           writeAuditLog(AUDIT_OBJECTS, true, objectId, _T("Delete file \"%s\" on node %s"), fileName, object->getName());
                           break;
                        case CMD_FILEMGR_RENAME_FILE:
                        {
                           TCHAR newFileName[MAX_PATH];
                           request->getFieldAsString(VID_NEW_FILE_NAME, newFileName, MAX_PATH);
                           writeAuditLog(AUDIT_OBJECTS, true, objectId,
                                         _T("Rename file \"%s\" to \"%s\" on node %s"), fileName, newFileName, object->getName());
                           break;
                        }
                        case CMD_FILEMGR_MOVE_FILE:
                        {
                           TCHAR newFileName[MAX_PATH];
                           request->getFieldAsString(VID_NEW_FILE_NAME, newFileName, MAX_PATH);
                           writeAuditLog(AUDIT_OBJECTS, true, objectId,
                                         _T("Move file \"%s\" to \"%s\" on node %s"), fileName, newFileName, object->getName());
                           break;
                        }
                        case CMD_FILEMGR_CREATE_FOLDER:
                        {
                           TCHAR newFileName[MAX_PATH];
                           request->getFieldAsString(VID_NEW_FILE_NAME, newFileName, MAX_PATH);
                           writeAuditLog(AUDIT_OBJECTS, true, objectId, _T("Create directory \"%s\" on node %s"), fileName, object->getName());
                           break;
                        }
                        case CMD_FILEMGR_COPY_FILE:
                        {
                           TCHAR newFileName[MAX_PATH];
                           request->getFieldAsString(VID_NEW_FILE_NAME, newFileName, MAX_PATH);
                           writeAuditLog(AUDIT_OBJECTS, true, objectId,
                                         _T("Copy file \"%s\" to \"%s\" on node %s"), fileName, newFileName, object->getName());
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
            }
            else
            {
               msg.setField(VID_RCC, RCC_CONNECTION_BROKEN);
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

	if (rcc == RCC_ACCESS_DENIED)
	{
      TCHAR newFileName[MAX_PATH];
      switch(request->getCode())
      {
         case CMD_GET_FOLDER_SIZE:
            writeAuditLog(AUDIT_OBJECTS, false, objectId, _T("Access denied on reading size of directory \"%s\" on node %s"),
                     fileName, object->getName());
            break;
         case CMD_GET_FOLDER_CONTENT:
            writeAuditLog(AUDIT_OBJECTS, false, objectId, _T("Access denied on reading content of directory \"%s\" on node %s"),
                     fileName, object->getName());
            break;
         case CMD_FILEMGR_CREATE_FOLDER:
            writeAuditLog(AUDIT_OBJECTS, false, objectId, _T("Access denied on create directory \"%s\" on node %s"), fileName,
                     object->getName());
            break;
         case CMD_FILEMGR_DELETE_FILE:
            writeAuditLog(AUDIT_OBJECTS, false, objectId, _T("Access denied on delete file \"%s\" on node %s"), fileName,
                     object->getName());
            break;
         case CMD_FILEMGR_RENAME_FILE:
            request->getFieldAsString(VID_NEW_FILE_NAME, newFileName, MAX_PATH);
            writeAuditLog(AUDIT_OBJECTS, false, objectId, _T("Access denied on rename file \"%s\" to \"%s\" on node %s"), fileName,
                     newFileName, object->getName());
            break;
         case CMD_FILEMGR_MOVE_FILE:
            request->getFieldAsString(VID_NEW_FILE_NAME, newFileName, MAX_PATH);
            writeAuditLog(AUDIT_OBJECTS, false, objectId, _T("Access denied on move file \"%s\" to \"%s\" on node %s"), fileName,
                     newFileName, object->getName());
            break;
         case CMD_FILEMGR_COPY_FILE:
            request->getFieldAsString(VID_NEW_FILE_NAME, newFileName, MAX_PATH);
            writeAuditLog(AUDIT_OBJECTS, false, objectId, _T("Access denied on copy file \"%s\" to \"%s\" on node %s"), fileName,
                     newFileName, object->getName());
            break;
      }
	}

   sendMessage(responseMessage);
   delete response;
}

/**
 * Upload file provided by user directly to agent
 */
void ClientSession::uploadUserFileToAgent(NXCPMessage *request)
{
   NXCPMessage msg, *response = nullptr, *responseMessage;
	uint32_t rcc = RCC_INTERNAL_ERROR;
   responseMessage = &msg;

   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(request->getId());

   TCHAR fileName[MAX_PATH];
   request->getFieldAsString(VID_FILE_NAME, fileName, MAX_PATH);
   uint32_t objectId = request->getFieldAsUInt32(VID_OBJECT_ID);
	shared_ptr<NetObj> object = FindObjectById(objectId);
	if (object != nullptr)
	{
		if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_UPLOAD))
		{
			if (object->getObjectClass() == OBJECT_NODE)
			{
            shared_ptr<AgentConnectionEx> conn = static_cast<Node&>(*object).createAgentConnection();
            if (conn != nullptr)
            {
               request->setField(VID_ALLOW_PATH_EXPANSION, false);   // explicitly disable path expansion
               conn->sendMessage(request);
               response = conn->waitForMessage(CMD_REQUEST_COMPLETED, request->getId(), 10000);
               if (response != nullptr)
               {
                  rcc = response->getFieldAsUInt32(VID_RCC);
                  if (rcc == RCC_SUCCESS)
                  {
                     writeAuditLog(AUDIT_OBJECTS, true, objectId, _T("Started direct upload of file \"%s\" to node %s"), fileName, object->getName());
                     response->setCode(CMD_REQUEST_COMPLETED);
                     response->setField(VID_ENABLE_COMPRESSION, conn->isCompressionAllowed());
                     responseMessage = response;
                     m_agentConnections.put(request->getId(), conn);
                  }
                  else
                  {
                     msg.setField(VID_RCC, AgentErrorToRCC(rcc));
                     debugPrintf(6, _T("ClientSession::getAgentFolderContent: Error on agent: %d"), rcc);
                  }
               }
               else
               {
                  msg.setField(VID_RCC, RCC_TIMEOUT);
               }
            }
            else
            {
               msg.setField(VID_RCC, RCC_CONNECTION_BROKEN);
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

	if (rcc == RCC_ACCESS_DENIED)
	{
      writeAuditLog(AUDIT_OBJECTS, false, objectId, _T("Access denied for direct upload of file \"%s\" to node %s"), fileName, object->getName());
	}

   sendMessage(responseMessage);
   if (response != nullptr)
      delete response;
}

/**
 * Get switch forwarding database
 */
void ClientSession::getSwitchForwardingDatabase(NXCPMessage *request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request->getId());

   // Get node id and check object class and access rights
   shared_ptr<NetObj> node = FindObjectById(request->getFieldAsUInt32(VID_OBJECT_ID), OBJECT_NODE);
   if (node != nullptr)
   {
      if (node->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
      {
         ForwardingDatabase *fdb = static_cast<Node&>(*node).getSwitchForwardingDatabase();
         if (fdb != nullptr)
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
         writeAuditLog(AUDIT_OBJECTS, false, node->getId(), _T("Access denied on reading FDB"));
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
 * Get routing table
 */
void ClientSession::getRoutingTable(NXCPMessage *request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request->getId());

   // Get node id and check object class and access rights
   shared_ptr<NetObj> node = FindObjectById(request->getFieldAsUInt32(VID_OBJECT_ID), OBJECT_NODE);
   if (node != nullptr)
   {
      if (node->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
      {
         RoutingTable *rt = static_cast<Node&>(*node).getRoutingTable();
         if (rt != nullptr)
         {
            msg.setField(VID_RCC, RCC_SUCCESS);
            msg.setField(VID_NUM_ELEMENTS, (UINT32)rt->size());
            uint32_t id = VID_ELEMENT_LIST_BASE;
            for(int i = 0; i < rt->size(); i++)
            {
               ROUTE *route = rt->get(i);
               msg.setField(id++, route->dwDestAddr);
               msg.setField(id++, (UINT32)BitsInMask(route->dwDestMask));
               msg.setField(id++, route->dwNextHop);
               msg.setField(id++, route->dwIfIndex);
               msg.setField(id++, route->dwRouteType);
               shared_ptr<Interface> iface = static_cast<Node&>(*node).findInterfaceByIndex(route->dwIfIndex);
               if (iface != nullptr)
               {
                  msg.setField(id++, iface->getName());
               }
               else
               {
                  TCHAR buffer[32];
                  _sntprintf(buffer, 32, _T("[%u]"), route->dwIfIndex);
                  msg.setField(id++, buffer);
               }
               id += 4;
            }
            delete rt;
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
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request->getId());

   // Get node id and check object class and access rights
   shared_ptr<NetObj> device = FindObjectById(request->getFieldAsUInt32(VID_OBJECT_ID), OBJECT_MOBILEDEVICE);
   if (device != nullptr)
   {
      if (device->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
      {
         DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
         TCHAR query[256];
         _sntprintf(query, 255, _T("SELECT latitude,longitude,accuracy,start_timestamp,end_timestamp FROM gps_history_%d")
                                             _T(" WHERE start_timestamp<? AND end_timestamp>?"), request->getFieldAsUInt32(VID_OBJECT_ID));

         DB_STATEMENT hStmt = DBPrepare(hdb, query);
         if (hStmt != nullptr)
         {
            DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, (INT64)request->getFieldAsTime(VID_TIME_TO));
            DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, (INT64)request->getFieldAsTime(VID_TIME_FROM));
            DB_RESULT hResult = DBSelectPrepared(hStmt);
            if (hResult != nullptr)
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
         WriteAuditLog(AUDIT_OBJECTS, FALSE, m_dwUserId, m_workstation, m_id, device->getId(), _T("Access denied on reading routing table"));
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
 * Get screenshot of object
 */
void ClientSession::getScreenshot(NXCPMessage *request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request->getId());

   TCHAR* sessionName = request->getFieldAsString(VID_NAME);
   if(sessionName == nullptr)
      sessionName = _tcsdup(_T("Console"));
   UINT32 objectId = request->getFieldAsUInt32(VID_NODE_ID);
	shared_ptr<NetObj> object = FindObjectById(objectId);
	if (object != nullptr)
	{
		if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_SCREENSHOT))
		{
			if (object->getObjectClass() == OBJECT_NODE)
			{
            shared_ptr<AgentConnectionEx> conn = static_cast<Node&>(*object).createAgentConnection();
            if (conn != nullptr)
            {
               // Screenshot transfer can take significant amount of time on slow links
               conn->setCommandTimeout(60000);

               BYTE *data = nullptr;
               size_t size;
               UINT32 dwError = conn->takeScreenshot(sessionName, &data, &size);
               if (dwError == ERR_SUCCESS)
               {
                  msg.setField(VID_FILE_DATA, data, size);
               }
               else
               {
                  msg.setField(VID_RCC, AgentErrorToRCC(dwError));
               }
               MemFree(data);
            }
            else
            {
               msg.setField(VID_RCC, RCC_CONNECTION_BROKEN);
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
   MemFree(sessionName);
   // Send response
   sendMessage(&msg);
}

/**
 * Compile script
 */
void ClientSession::compileScript(NXCPMessage *request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request->getId());

	TCHAR *source = request->getFieldAsString(VID_SCRIPT);
	if (source != nullptr)
	{
      TCHAR errorMessage[256];
      int errorLine;
      NXSL_Program *script = NXSLCompile(source, errorMessage, 256, &errorLine);
      if (script != nullptr)
      {
         msg.setField(VID_COMPILATION_STATUS, (INT16)1);
         if (request->getFieldAsBoolean(VID_SERIALIZE))
         {
            ByteStream bs;
            script->serialize(bs);

            size_t size;
            const BYTE *code = bs.buffer(&size);
            msg.setField(VID_SCRIPT_CODE, code, size);
         }
         delete script;
      }
      else
      {
         msg.setField(VID_COMPILATION_STATUS, (INT16)0);
         msg.setField(VID_ERROR_TEXT, errorMessage);
         msg.setField(VID_ERROR_LINE, (INT32)errorLine);
      }
      msg.setField(VID_RCC, RCC_SUCCESS);
      free(source);
	}
	else
	{
      msg.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
	}

   sendMessage(&msg);
}

/**
 * Clean configuration and collected data of DCIs on agent
 */
void ClientSession::cleanAgentDciConfiguration(NXCPMessage *request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request->getId());

   uint32_t objectId = request->getFieldAsUInt32(VID_NODE_ID);
	shared_ptr<NetObj> object = FindObjectById(objectId);
	if (object != nullptr)
	{
      if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_CONTROL))
		{
			if (object->getObjectClass() == OBJECT_NODE)
			{
            shared_ptr<AgentConnectionEx> conn = static_cast<Node&>(*object).createAgentConnection();
            if (conn != nullptr)
            {
               static_cast<Node&>(*object).clearDataCollectionConfigFromAgent(conn.get());
               msg.setField(VID_RCC, RCC_SUCCESS);
            }
            else
            {
               msg.setField(VID_RCC, RCC_CONNECTION_BROKEN);
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
      msg.setField(VID_RCC, RCC_INVALID_ARGUMENT);
	}

   sendMessage(&msg);
}

/**
 * Triggers offline DCI configuration synchronization with node
 */
void ClientSession::resyncAgentDciConfiguration(NXCPMessage *request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request->getId());

   UINT32 objectId = request->getFieldAsUInt32(VID_NODE_ID);
	shared_ptr<NetObj> node = FindObjectById(objectId);
   if (node != nullptr)
	{
      if (node->checkAccessRights(m_dwUserId, OBJECT_ACCESS_CONTROL))
		{
			if (node->getObjectClass() == OBJECT_NODE)
			{
			   static_cast<Node&>(*node).forceSyncDataCollectionConfig();
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
      msg.setField(VID_RCC, RCC_INVALID_ARGUMENT);
	}

   sendMessage(&msg);
}

/**
 * List all possible Schedule ID's
 */
void ClientSession::getSchedulerTaskHandlers(NXCPMessage *request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request->getId());
   GetSchedulerTaskHandlers(&msg, m_systemAccessRights);
   msg.setField(VID_RCC, RCC_SUCCESS);
   sendMessage(&msg);
}

/**
 * List all existing schedules
 */
void ClientSession::getScheduledTasks(NXCPMessage *request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request->getId());
   GetScheduledTasks(&msg, m_dwUserId, m_systemAccessRights);
   msg.setField(VID_RCC, RCC_SUCCESS);
   sendMessage(&msg);
}

/**
 * Add new schedule
 */
void ClientSession::addScheduledTask(NXCPMessage *request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request->getId());
   UINT32 result = CreateScheduledTaskFromMsg(request, m_dwUserId, m_systemAccessRights);
   msg.setField(VID_RCC, result);
   sendMessage(&msg);
}

/**
 * Update existing schedule
 */
void ClientSession::updateScheduledTask(NXCPMessage *request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request->getId());
   UINT32 result = UpdateScheduledTaskFromMsg(request, m_dwUserId, m_systemAccessRights);
   msg.setField(VID_RCC, result);
   sendMessage(&msg);
}

/**
 * Remove/delete schedule
 */
void ClientSession::removeScheduledTask(NXCPMessage *request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request->getId());
   UINT32 result = DeleteScheduledTask(request->getFieldAsUInt32(VID_SCHEDULED_TASK_ID), m_dwUserId, m_systemAccessRights);
   msg.setField(VID_RCC, result);
   sendMessage(&msg);
}

/**
 * Get list of registered prediction engines
 */
void ClientSession::getPredictionEngines(NXCPMessage *request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request->getId());
   GetPredictionEngines(&msg);
   msg.setField(VID_RCC, RCC_SUCCESS);
   sendMessage(&msg);
}

/**
 * Get predicted data
 */
void ClientSession::getPredictedData(NXCPMessage *request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request->getId());
   bool success = false;

   shared_ptr<NetObj> object = FindObjectById(request->getFieldAsUInt32(VID_OBJECT_ID));
   if (object != nullptr)
   {
      if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
      {
         if (object->isDataCollectionTarget())
         {
            if (!(g_flags & AF_DB_CONNECTION_LOST))
            {
               success = GetPredictedData(this, request, &msg, static_cast<DataCollectionTarget&>(*object));
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
 * Get list of agent tunnels
 */
void ClientSession::getAgentTunnels(NXCPMessage *request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request->getId());
   if (m_systemAccessRights & SYSTEM_ACCESS_REGISTER_AGENTS)
   {
      GetAgentTunnels(&msg);
      msg.setField(VID_RCC, RCC_SUCCESS);
      writeAuditLog(AUDIT_SYSCFG, true, 0, _T("Read list of agent tunnels"));
   }
   else
   {
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);
      writeAuditLog(AUDIT_SYSCFG, false, 0, _T("Access denied on reading list of agent tunnels"));
   }
   sendMessage(&msg);
}

/**
 * Bind agent tunnel to node
 */
void ClientSession::bindAgentTunnel(NXCPMessage *request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request->getId());
   if (m_systemAccessRights & SYSTEM_ACCESS_REGISTER_AGENTS)
   {
      UINT32 nodeId = request->getFieldAsUInt32(VID_NODE_ID);
      UINT32 tunnelId = request->getFieldAsUInt32(VID_TUNNEL_ID);
      UINT32 rcc = BindAgentTunnel(tunnelId, nodeId, m_dwUserId);
      msg.setField(VID_RCC, rcc);
      if (rcc == RCC_SUCCESS)
      {
         writeAuditLog(AUDIT_SYSCFG, true, nodeId, _T("Agent tunnel bound to node"));
      }
   }
   else
   {
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);
      writeAuditLog(AUDIT_SYSCFG, false, 0, _T("Access denied on binding agent tunnel"));
   }
   sendMessage(&msg);
}

/**
 * Unbind agent tunnel from node
 */
void ClientSession::unbindAgentTunnel(NXCPMessage *request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request->getId());
   if (m_systemAccessRights & SYSTEM_ACCESS_REGISTER_AGENTS)
   {
      UINT32 nodeId = request->getFieldAsUInt32(VID_NODE_ID);
      UINT32 rcc = UnbindAgentTunnel(nodeId, m_dwUserId);
      msg.setField(VID_RCC, rcc);
      if (rcc == RCC_SUCCESS)
      {
         writeAuditLog(AUDIT_SYSCFG, true, nodeId, _T("Agent tunnel unbound from node"));
      }
   }
   else
   {
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);
      writeAuditLog(AUDIT_SYSCFG, false, 0, _T("Access denied on unbinding agent tunnel"));
   }
   sendMessage(&msg);
}

/**
 * Setup TCP proxy via agent
 */
void ClientSession::setupTcpProxy(NXCPMessage *request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request->getId());
   if (m_systemAccessRights & SYSTEM_ACCESS_SETUP_TCP_PROXY)
   {
      shared_ptr<NetObj> object = FindObjectById(request->getFieldAsUInt32(VID_NODE_ID));
      if (object != nullptr)
      {
         if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_CONTROL))
         {
            uint32_t rcc = RCC_SUCCESS;
            shared_ptr<Node> node;
            if (object->getObjectClass() == OBJECT_NODE)
            {
               node = static_pointer_cast<Node>(object);
            }
            else if (object->getObjectClass() == OBJECT_ZONE)
            {
               node = static_pointer_cast<Node>(FindObjectById(static_cast<Zone&>(*object).getProxyNodeId(nullptr, false), OBJECT_NODE));
               if (node == nullptr)
               {
                  debugPrintf(4, _T("Requested TCP proxy for zone %s but it doesn't have available proxy nodes"), object->getName());
                  rcc = RCC_RESOURCE_NOT_AVAILABLE;
               }
            }
            else
            {
               rcc = RCC_INCOMPATIBLE_OPERATION;
            }
            if (rcc == RCC_SUCCESS)
            {
               shared_ptr<AgentConnectionEx> conn = node->createAgentConnection();
               if (conn != nullptr)
               {
                  conn->setTcpProxySession(this);
                  InetAddress ipAddr = request->getFieldAsInetAddress(VID_IP_ADDRESS);
                  uint16_t port = request->getFieldAsUInt16(VID_PORT);
                  uint32_t agentChannelId;
                  rcc = conn->setupTcpProxy(ipAddr, port, &agentChannelId);
                  if (rcc == ERR_SUCCESS)
                  {
                     uint32_t clientChannelId = InterlockedIncrement(&m_tcpProxyChannelId);
                     MutexLock(m_tcpProxyLock);
                     m_tcpProxyConnections->add(new TcpProxy(conn, agentChannelId, clientChannelId, node->getId()));
                     MutexUnlock(m_tcpProxyLock);
                     msg.setField(VID_RCC, RCC_SUCCESS);
                     msg.setField(VID_CHANNEL_ID, clientChannelId);
                     writeAuditLog(AUDIT_SYSCFG, true, node->getId(), _T("Created TCP proxy to %s port %d via %s [%u] (client channel %u)"),
                              (const TCHAR *)ipAddr.toString(), (int)port, node->getName(), node->getId(), clientChannelId);
                     debugPrintf(3, _T("Created TCP proxy to %s port %d via %s [%d]"),
                              (const TCHAR *)ipAddr.toString(), (int)port, node->getName(), node->getId());
                  }
                  else
                  {
                     msg.setField(VID_RCC, AgentErrorToRCC(rcc));
                  }
               }
               else
               {
                  msg.setField(VID_RCC, RCC_COMM_FAILURE);
               }
            }
            else
            {
               msg.setField(VID_RCC, rcc);
            }
         }
         else
         {
            msg.setField(VID_RCC, RCC_ACCESS_DENIED);
            writeAuditLog(AUDIT_SYSCFG, false, object->getId(), _T("Access denied on setting up TCP proxy"));
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
      writeAuditLog(AUDIT_SYSCFG, false, 0, _T("Access denied on setting up TCP proxy"));
   }
   sendMessage(&msg);
}

/**
 * Close TCP proxy session
 */
void ClientSession::closeTcpProxy(NXCPMessage *request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request->getId());

   uint32_t clientChannelId = request->getFieldAsUInt32(VID_CHANNEL_ID);

   shared_ptr<AgentConnectionEx> conn;
   uint32_t agentChannelId, nodeId;
   MutexLock(m_tcpProxyLock);
   for(int i = 0; i < m_tcpProxyConnections->size(); i++)
   {
      TcpProxy *p = m_tcpProxyConnections->get(i);
      if (p->clientChannelId == clientChannelId)
      {
         agentChannelId = p->agentChannelId;
         nodeId = p->nodeId;
         conn = p->agentConnection;
         m_tcpProxyConnections->remove(i);
         break;
      }
   }
   MutexUnlock(m_tcpProxyLock);

   if (conn != nullptr)
   {
      conn->closeTcpProxy(agentChannelId);
      writeAuditLog(AUDIT_SYSCFG, true, nodeId, _T("Closed TCP proxy channel %u"), clientChannelId);
   }

   sendMessage(&msg);
}

/**
 * Process TCP proxy data (in direction from from agent to client)
 */
void ClientSession::processTcpProxyData(AgentConnectionEx *conn, UINT32 agentChannelId, const void *data, size_t size)
{
   UINT32 clientChannelId = 0;
   MutexLock(m_tcpProxyLock);
   for(int i = 0; i < m_tcpProxyConnections->size(); i++)
   {
      TcpProxy *p = m_tcpProxyConnections->get(i);
      if ((p->agentConnection.get() == conn) && (p->agentChannelId == agentChannelId))
      {
         clientChannelId = p->clientChannelId;
         if (size == 0) // close indicator
         {
            debugPrintf(5, _T("Received TCP proxy channel %u close notification"), clientChannelId);
            m_tcpProxyConnections->remove(i);
         }
         break;
      }
   }
   MutexUnlock(m_tcpProxyLock);

   if (clientChannelId != 0)
   {
      if (size > 0)
      {
         size_t msgSize = size + NXCP_HEADER_SIZE;
         if (msgSize % 8 != 0)
            msgSize += 8 - msgSize % 8;
         NXCP_MESSAGE *msg = (NXCP_MESSAGE *)MemAlloc(msgSize);
         msg->code = htons(CMD_TCP_PROXY_DATA);
         msg->flags = htons(MF_BINARY);
         msg->id = htonl(clientChannelId);
         msg->numFields = htonl(static_cast<UINT32>(size));
         msg->size = htonl(static_cast<UINT32>(msgSize));
         memcpy(msg->fields, data, size);
         postRawMessageAndDelete(msg);
      }
      else
      {
         NXCPMessage msg(CMD_CLOSE_TCP_PROXY, 0);
         msg.setField(VID_CHANNEL_ID, clientChannelId);
         postMessage(&msg);
      }
   }
}

/**
 * Expand event processing macros
 */
void ClientSession::expandMacros(NXCPMessage *request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request->getId());

   StringMap inputFields;
   inputFields.loadMessage(request, VID_INPUT_FIELD_COUNT, VID_INPUT_FIELD_BASE);

   int fieldCount = request->getFieldAsUInt32(VID_STRING_COUNT);
   uint32_t inFieldId = VID_EXPAND_STRING_BASE;
   uint32_t outFieldId = VID_EXPAND_STRING_BASE;
   for(int i = 0; i < fieldCount; i++, inFieldId += 2, outFieldId++)
   {
      TCHAR *textToExpand = request->getFieldAsString(inFieldId++);
      shared_ptr<NetObj> object = FindObjectById(request->getFieldAsUInt32(inFieldId++));
      if (object == nullptr)
      {
         msg.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
         sendMessage(&msg);
         MemFree(textToExpand);
         return;
      }
      Alarm *alarm = FindAlarmById(request->getFieldAsUInt32(inFieldId++));
      if (!object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ) ||
          ((alarm != nullptr) && !object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ_ALARMS) && !alarm->checkCategoryAccess(this)))
      {
         msg.setField(VID_RCC, RCC_ACCESS_DENIED);
         sendMessage(&msg);
         MemFree(textToExpand);
         delete alarm;
         return;
      }

      String result = object->expandText(textToExpand, alarm, nullptr, shared_ptr<DCObjectInfo>(), m_loginName, nullptr, &inputFields, nullptr);
      msg.setField(outFieldId, result);
      debugPrintf(7, _T("ClientSession::expandMacros(): template=\"%s\", result=\"%s\""), textToExpand, (const TCHAR *)result);
      MemFree(textToExpand);
      delete alarm;
   }

   msg.setField(VID_RCC, RCC_SUCCESS);
   sendMessage(&msg);
}

/**
 * Update or create policy from message
 */
void ClientSession::updatePolicy(NXCPMessage *request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request->getId());

   shared_ptr<NetObj> templateObject = FindObjectById(request->getFieldAsUInt32(VID_TEMPLATE_ID), OBJECT_TEMPLATE);
   if (templateObject != nullptr)
   {
      if (templateObject->checkAccessRights(m_dwUserId, OBJECT_ACCESS_MODIFY))
      {
         uuid guid = static_cast<Template&>(*templateObject).updatePolicyFromMessage(request);
         if(!guid.isNull())
         {
            msg.setField(VID_GUID, guid);
            msg.setField(VID_RCC, RCC_SUCCESS);
         }
         else
         {
            msg.setField(VID_RCC, RCC_NO_SUCH_POLICY);
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
 * Delete agent policy from template
 */
void ClientSession::deletePolicy(NXCPMessage *request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request->getId());

   shared_ptr<NetObj> templateObject = FindObjectById(request->getFieldAsUInt32(VID_TEMPLATE_ID), OBJECT_TEMPLATE);
   if(templateObject != nullptr)
   {
      if (templateObject->checkAccessRights(m_dwUserId, OBJECT_ACCESS_MODIFY))
      {
         if (static_cast<Template&>(*templateObject).removePolicy(request->getFieldAsGUID(VID_GUID)))
            msg.setField(VID_RCC, RCC_SUCCESS);
         else
            msg.setField(VID_RCC, RCC_NO_SUCH_POLICY);
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
 * Get policy list for template
 */
void ClientSession::getPolicyList(NXCPMessage *request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request->getId());

   shared_ptr<NetObj> templateObject = FindObjectById(request->getFieldAsUInt32(VID_TEMPLATE_ID), OBJECT_TEMPLATE);
   if (templateObject != nullptr)
   {
      if (templateObject->checkAccessRights(m_dwUserId, OBJECT_ACCESS_MODIFY))
      {
         static_cast<Template&>(*templateObject).fillPolicyListMessage(&msg);
         msg.setField(VID_RCC, RCC_SUCCESS);
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
 * Get policy list for template
 */
void ClientSession::getPolicy(NXCPMessage *request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request->getId());

   shared_ptr<NetObj> templateObject = FindObjectById(request->getFieldAsUInt32(VID_TEMPLATE_ID), OBJECT_TEMPLATE);
   if (templateObject != nullptr)
   {
      if (templateObject->checkAccessRights(m_dwUserId, OBJECT_ACCESS_MODIFY))
      {
         uuid guid = request->getFieldAsGUID(VID_GUID);
         if (static_cast<Template&>(*templateObject).fillPolicyDetailsMessage(&msg, guid))
            msg.setField(VID_RCC, RCC_SUCCESS);
         else
            msg.setField(VID_RCC, RCC_INVALID_POLICY_ID);
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
 * Worker thread for policy apply
 */
static void ApplyPolicyChanges(const shared_ptr<NetObj>& templateObject)
{
   static_cast<Template&>(*templateObject).applyPolicyChanges();
}

/**
 * Get policy list for template
 */
void ClientSession::onPolicyEditorClose(NXCPMessage *request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request->getId());

   shared_ptr<NetObj> templateObject = FindObjectById(request->getFieldAsUInt32(VID_TEMPLATE_ID), OBJECT_TEMPLATE);
   if(templateObject != nullptr)
   {
      ThreadPoolExecute(g_clientThreadPool, ApplyPolicyChanges, templateObject);
      msg.setField(VID_RCC, RCC_SUCCESS);
   }
   else
      msg.setField(VID_RCC, RCC_INVALID_OBJECT_ID);

   sendMessage(&msg);
}

/**
 * Worker thread for policy force apply
 */
static void ForceApplyPolicyChanges(const shared_ptr<NetObj>& templateObject)
{
   static_cast<Template&>(*templateObject).forceApplyPolicyChanges();
}

/**
 * Force apply
 */
void ClientSession::forceApplyPolicy(NXCPMessage *pRequest)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, pRequest->getId());

   // Get source and destination
   shared_ptr<NetObj> templateObject = FindObjectById(pRequest->getFieldAsUInt32(VID_TEMPLATE_ID), OBJECT_TEMPLATE);
   if (templateObject != nullptr)
   {
      // Check access rights
      if (templateObject->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
      {
         ThreadPoolExecute(g_clientThreadPool, ForceApplyPolicyChanges, templateObject);
         msg.setField(VID_RCC, RCC_SUCCESS);
      }
      else  // User doesn't have enough rights on object(s)
      {
         msg.setField(VID_RCC, RCC_ACCESS_DENIED);
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
 * Resolve object DCO's by regex, if objectNameRegex is nullptr, objectId will be used instead
 *
 * @param objectId for single object
 * @param objectNameRegex for multiple objects
 * @param dciRegex
 * @param searchByName
 * @return
 */
SharedObjectArray<DCObject> *ClientSession::resolveDCOsByRegex(UINT32 objectId, const TCHAR *objectNameRegex, const TCHAR *dciRegex, bool searchByName)
{
   if (dciRegex == nullptr || dciRegex[0] == 0)
      return nullptr;

   SharedObjectArray<DCObject> *dcoList = nullptr;

   if (objectNameRegex == nullptr || objectNameRegex[0] == 0)
   {
      shared_ptr<NetObj> object = FindObjectById(objectId);
      if ((object != nullptr) && object->isDataCollectionTarget() && object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
         dcoList = static_cast<DataCollectionTarget&>(*object).getDCObjectsByRegex(dciRegex, searchByName, m_dwUserId);
   }
   else
   {
      SharedObjectArray<NetObj> *objects = FindObjectsByRegex(objectNameRegex);
      if (objects != nullptr)
      {
         dcoList = new SharedObjectArray<DCObject>();
         for(int i = 0; i < objects->size(); i++)
         {
            NetObj *object = objects->get(i);
            if (object->isDataCollectionTarget() && object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
            {
               SharedObjectArray<DCObject> *nodeDcoList = static_cast<DataCollectionTarget&>(*object).getDCObjectsByRegex(dciRegex, searchByName, m_dwUserId);
               if (nodeDcoList != nullptr)
               {
                  for(int n = 0; n < nodeDcoList->size(); n++)
                  {
                     dcoList->add(nodeDcoList->getShared(n));
                  }
                  delete(nodeDcoList);
               }
            }
         }
         delete(objects);
      }
   }

   return dcoList;
}

/**
 * Get a list of dci's matched by regex
 *
 * @param request
 */
void ClientSession::getMatchingDCI(NXCPMessage *request)
{
   NXCPMessage msg;
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(request->getId());

   TCHAR objectName[MAX_OBJECT_NAME], dciName[MAX_OBJECT_NAME];
   objectName[0] = 0, dciName[0] = 0;
   UINT32 objectId = 0;

   if (request->isFieldExist(VID_OBJECT_NAME))
      request->getFieldAsString(VID_OBJECT_NAME, objectName, MAX_OBJECT_NAME);
   else
      objectId = request->getFieldAsUInt32(VID_OBJECT_ID);
   UINT32 flags = request->getFieldAsInt32(VID_FLAGS);
   request->getFieldAsString(VID_DCI_NAME, dciName, MAX_OBJECT_NAME);

   SharedObjectArray<DCObject> *dcoList = resolveDCOsByRegex(objectId, objectName, dciName, (flags & DCI_RES_SEARCH_NAME));
   if (dcoList != nullptr)
   {
      UINT32 dciBase = VID_DCI_VALUES_BASE, count = 0;
      for(int i = 0; i < dcoList->size(); i++)
      {
         if (dcoList->get(i)->getType() == DCO_TYPE_ITEM)
         {
            DCItem *item = static_cast<DCItem *>(dcoList->get(i));
            msg.setField(dciBase + 1, item->getId());
            msg.setField(dciBase + 2, CHECK_NULL_EX(item->getLastValue()));
            msg.setField(dciBase + 3, item->getType());
            msg.setField(dciBase + 4, item->getStatus());
            msg.setField(dciBase + 5, item->getOwnerId());
            msg.setField(dciBase + 6, item->getDataSource());
            msg.setField(dciBase + 7, item->getName());
            msg.setField(dciBase + 8, item->getDescription());
            dciBase += 10;
            count++;
         }
      }
      msg.setField(VID_NUM_ITEMS, count);
      delete dcoList;
   }
   else
   {
      msg.setField(VID_RCC, RCC_INVALID_ARGUMENT);
   }

   sendMessage(&msg);
}

/**
 * Get list of user agent messages
 */
void ClientSession::getUserAgentNotification(NXCPMessage *request)
{
   NXCPMessage msg;
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(request->getId());

   if (m_systemAccessRights & SYSTEM_ACCESS_UA_NOTIFICATIONS)
   {
      g_userAgentNotificationListMutex.lock();
      msg.setField(VID_UA_NOTIFICATION_COUNT, g_userAgentNotificationList.size());
      int base = VID_UA_NOTIFICATION_BASE;
      for(int i = 0; i < g_userAgentNotificationList.size(); i++, base+=10)
      {
         g_userAgentNotificationList.get(i)->fillMessage(base, &msg);
      }
      g_userAgentNotificationListMutex.unlock();
      msg.setField(VID_RCC, RCC_SUCCESS);
   }
   else
   {
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);
      writeAuditLog(AUDIT_SYSCFG, false, 0, _T("Access denied getting user agent notifications"));
   }

   sendMessage(&msg);
}

/**
 * Create new user agent message and send to the nodes
 */
void ClientSession::addUserAgentNotification(NXCPMessage *request)
{
   NXCPMessage msg;
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(request->getId());

   if (m_systemAccessRights & SYSTEM_ACCESS_UA_NOTIFICATIONS)
   {
      IntegerArray<UINT32> objectList(16, 16);
      request->getFieldAsInt32Array(VID_UA_NOTIFICATION_BASE + 1, &objectList);
      UINT32 rcc = RCC_SUCCESS;
      for (int i = 0; i < objectList.size(); i++)
      {
         shared_ptr<NetObj> object = FindObjectById(objectList.get(i));
         if (object == nullptr)
         {
            rcc = RCC_INVALID_OBJECT_ID;
            break;
         }
         if (!object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
         {
            rcc = RCC_ACCESS_DENIED;
            writeAuditLog(AUDIT_SYSCFG, false, objectList.get(i), _T("Access denied on user agent notification creation"));
            break;
         }
      }
      if (rcc == RCC_SUCCESS)
      {
         TCHAR tmp[MAX_USER_AGENT_MESSAGE_SIZE];
         UserAgentNotificationItem *uan = CreateNewUserAgentNotification(request->getFieldAsString(VID_UA_NOTIFICATION_BASE, tmp, MAX_USER_AGENT_MESSAGE_SIZE),
               objectList, request->getFieldAsTime(VID_UA_NOTIFICATION_BASE + 2), request->getFieldAsTime(VID_UA_NOTIFICATION_BASE + 3),
               request->getFieldAsBoolean(VID_UA_NOTIFICATION_BASE + 4), m_dwUserId);
         json_t *objData = uan->toJson();
         WriteAuditLogWithJsonValues(AUDIT_OBJECTS, true, m_dwUserId, m_workstation, m_id, uan->getId(), nullptr, objData,
               _T("User agent notification %d created"), uan->getId());
         json_decref(objData);
         uan->decRefCount();
      }
      msg.setField(VID_RCC, rcc);
   }
   else
   {
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);
      writeAuditLog(AUDIT_SYSCFG, false, 0, _T("Access denied on user agent notification creation"));
   }

   sendMessage(&msg);
}

/**
 * Recall user user message
 */
void ClientSession::recallUserAgentNotification(NXCPMessage *request)
{
   NXCPMessage msg;
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(request->getId());

   if (m_systemAccessRights & SYSTEM_ACCESS_UA_NOTIFICATIONS)
   {
      g_userAgentNotificationListMutex.lock();
      int base = VID_UA_NOTIFICATION_BASE;
      UserAgentNotificationItem *uan = nullptr;
      UINT32 objectId = request->getFieldAsUInt32(VID_OBJECT_ID);
      for(int i = 0; i < g_userAgentNotificationList.size(); i++, base+=10)
      {
         UserAgentNotificationItem *tmp = g_userAgentNotificationList.get(i);
         if(tmp->getId() == objectId)
         {
            if(!tmp->isRecalled() && (tmp->getStartTime() != 0))
            {
               tmp->recall();
               uan = tmp;
               uan->incRefCount();
            }
            break;
         }
      }
      g_userAgentNotificationListMutex.unlock();

      if(uan != nullptr)
      {
         json_t *objData = uan->toJson();
         WriteAuditLogWithJsonValues(AUDIT_OBJECTS, true, m_dwUserId, m_workstation, m_id, uan->getId(), nullptr, objData,
            _T("User agent notification %d recalled"), uan->getId());
         json_decref(objData);
         ThreadPoolExecute(g_clientThreadPool, uan, &UserAgentNotificationItem::processUpdate);
         NotifyClientSessions(NX_NOTIFY_USER_AGENT_MESSAGE_CHANGED, (UINT32)uan->getId());
         msg.setField(VID_RCC, RCC_SUCCESS);
      }
      else
         msg.setField(VID_RCC, RCC_INVALID_OBJECT_ID);

   }
   else
   {
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);
      writeAuditLog(AUDIT_SYSCFG, false, 0, _T("Access denied on user agent notification recall"));
   }

   sendMessage(&msg);
}

/**
 * Get notification channel
 */
void ClientSession::getNotificationChannels(UINT32 requestId)
{
   NXCPMessage msg;
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(requestId);
   if(m_systemAccessRights & SYSTEM_ACCESS_SERVER_CONFIG)
   {
      GetNotificationChannels(&msg);
      msg.setField(VID_RCC, RCC_SUCCESS);
   }
   else
   {
      writeAuditLog(AUDIT_SYSCFG, false, 0, _T("Access denied on reading notification channel list"));
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);
   }
   sendMessage(&msg);
}

/**
 * Add notification channel
 */
void ClientSession::addNotificationChannel(NXCPMessage *request)
{
   NXCPMessage msg;
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(request->getId());
   if (m_systemAccessRights & SYSTEM_ACCESS_SERVER_CONFIG)
   {
      TCHAR name[MAX_OBJECT_NAME];
      request->getFieldAsString(VID_NAME, name, MAX_OBJECT_NAME);
      if (name[0] != 0)
      {
         TCHAR driverName[MAX_OBJECT_NAME];
         request->getFieldAsString(VID_DRIVER_NAME, driverName, MAX_OBJECT_NAME);
         if (driverName[0] != 0)
         {
            if (!IsNotificationChannelExists(name))
            {
               TCHAR description[MAX_NC_DESCRIPTION];
               request->getFieldAsString(VID_DESCRIPTION, description, MAX_NC_DESCRIPTION);
               char *configuration = request->getFieldAsMBString(VID_XML_CONFIG, nullptr, 0);
               CreateNotificationChannelAndSave(name, description, driverName, configuration);
               msg.setField(VID_RCC, RCC_SUCCESS);
               NotifyClientSessions(NX_NOTIFICATION_CHANNEL_CHANGED, 0);
               writeAuditLog(AUDIT_SYSCFG, true, 0, _T("Created new notification channel %s"), name);
            }
            else
            {
               msg.setField(VID_RCC, RCC_CHANNEL_ALREADY_EXIST);
            }
         }
         else
         {
            msg.setField(VID_RCC, RCC_INVALID_DRIVER_NAME);
         }
      }
      else
      {
         msg.setField(VID_RCC, RCC_INVALID_CHANNEL_NAME);
      }
   }
   else
   {
      writeAuditLog(AUDIT_SYSCFG, false, 0, _T("Access denied on new notification channel creation"));
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);
   }
   sendMessage(&msg);
}

/**
 * Update notificaiton channel
 */
void ClientSession::updateNotificationChannel(NXCPMessage *request)
{
   NXCPMessage msg;
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(request->getId());
   if (m_systemAccessRights & SYSTEM_ACCESS_SERVER_CONFIG)
   {
      TCHAR name[MAX_OBJECT_NAME];
      request->getFieldAsString(VID_NAME, name, MAX_OBJECT_NAME);
      if (name[0] != 0)
      {
         TCHAR driverName[MAX_OBJECT_NAME];
         request->getFieldAsString(VID_DRIVER_NAME, driverName, MAX_OBJECT_NAME);
         if (driverName[0] != 0)
         {
            if (IsNotificationChannelExists(name))
            {
               TCHAR description[MAX_NC_DESCRIPTION];
               request->getFieldAsString(VID_DESCRIPTION, description, MAX_NC_DESCRIPTION);
               char *configuration = request->getFieldAsMBString(VID_XML_CONFIG, nullptr, 0);
               UpdateNotificationChannel(name, description, driverName, configuration);
               msg.setField(VID_RCC, RCC_SUCCESS);
               NotifyClientSessions(NX_NOTIFICATION_CHANNEL_CHANGED, 0);
               MemFree(configuration);
               writeAuditLog(AUDIT_SYSCFG, true, 0, _T("Updated configuration of notification channel %s"), name);
            }
            else
            {
               msg.setField(VID_RCC, RCC_NO_CHANNEL_NAME);
            }
         }
         else
         {
            msg.setField(VID_RCC, RCC_INVALID_DRIVER_NAME);
         }
      }
      else
      {
         msg.setField(VID_RCC, RCC_INVALID_CHANNEL_NAME);
      }
   }
   else
   {
      writeAuditLog(AUDIT_SYSCFG, false, 0, _T("Access denied on notification channel update"));
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);
   }
   sendMessage(&msg);

}

/**
 * Remove notification channel
 */
void ClientSession::removeNotificationChannel(NXCPMessage *request)
{
   NXCPMessage msg;
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(request->getId());
   if (m_systemAccessRights & SYSTEM_ACCESS_SERVER_CONFIG)
   {
      TCHAR name[MAX_OBJECT_NAME];
      request->getFieldAsString(VID_NAME, name, MAX_OBJECT_NAME);
      if (!CheckChannelIsUsedInAction(name))
      {
         if (DeleteNotificationChannel(name))
         {
            NotifyClientSessions(NX_NOTIFICATION_CHANNEL_CHANGED, 0);
            msg.setField(VID_RCC, RCC_SUCCESS);
            writeAuditLog(AUDIT_SYSCFG, true, 0, _T("Notification channel %s deleted"), name);
         }
         else
         {
            msg.setField(VID_RCC, RCC_NO_CHANNEL_NAME);
         }
      }
      else
      {
         msg.setField(VID_RCC, RCC_CHANNEL_IN_USE);
      }
   }
   else
   {
      writeAuditLog(AUDIT_SYSCFG, false, 0, _T("Access denied on notification channel deletion"));
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);
   }
   sendMessage(&msg);
}

/**
 * Rename notification channel
 */
void ClientSession::renameNotificationChannel(NXCPMessage *request)
{
   NXCPMessage msg;
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(request->getId());
   if (m_systemAccessRights & SYSTEM_ACCESS_SERVER_CONFIG)
   {
      TCHAR *name = request->getFieldAsString(VID_NAME);
      if (name[0] != 0)
      {
         if (IsNotificationChannelExists(name))
         {
            TCHAR *newName = request->getFieldAsString(VID_NEW_NAME);
            if (!IsNotificationChannelExists(newName))
            {
               writeAuditLog(AUDIT_SYSCFG, true, 0, _T("Notification channel %s renamed to %s"), name, newName);
               RenameNotificationChannel(name, newName); //will release names
               msg.setField(VID_RCC, RCC_SUCCESS);

            }
            else
            {
               MemFree(name);
               MemFree(newName);
               msg.setField(VID_RCC, RCC_CHANNEL_ALREADY_EXIST);
            }
         }
         else
         {
            MemFree(name);
            msg.setField(VID_RCC, RCC_NO_CHANNEL_NAME);
         }
      }
      else
      {
         MemFree(name);
         msg.setField(VID_RCC, RCC_INVALID_CHANNEL_NAME);
      }
   }
   else
   {
      writeAuditLog(AUDIT_SYSCFG, false, 0, _T("Access denied on notification channel rename"));
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);
   }
   sendMessage(&msg);
}

/**
 * Get notification driver names
 */
void ClientSession::getNotificationDriverNames(UINT32 requestId)
{
   NXCPMessage msg;
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(requestId);
   if (m_systemAccessRights & SYSTEM_ACCESS_SERVER_CONFIG)
   {
      GetNotificationDrivers(&msg);
      msg.setField(VID_RCC, RCC_SUCCESS);
   }
   else
   {
      writeAuditLog(AUDIT_SYSCFG, false, 0, _T("Access denied on reading notification driver list"));
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);
   }
   sendMessage(&msg);
}

/**
 * Start active discovery with provided range
 */
void ClientSession::startActiveDiscovery(NXCPMessage *request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request->getId());

   if (m_systemAccessRights & SYSTEM_ACCESS_SERVER_CONFIG)
   {
      int count = request->getFieldAsInt32(VID_NUM_RECORDS);
      if (count > 0)
      {
         UINT32 fieldId = VID_ADDR_LIST_BASE;
         ObjectArray<InetAddressListElement> *addressList = new ObjectArray<InetAddressListElement>(0, 16, Ownership::True);
         StringBuffer ranges;
         for (int i = 0; i < count; i++, fieldId += 10)
         {
            if (ranges.length() > 0)
               ranges.append(_T(", "));
            InetAddressListElement *el = new InetAddressListElement(request, fieldId);
            addressList->add(el);
            ranges.append(el->toString());
         }

         ThreadPoolExecute(g_clientThreadPool, StartManualActiveDiscovery, addressList);
         WriteAuditLog(AUDIT_SYSCFG, true, m_dwUserId, m_workstation, m_id, 0, _T("Manual active discovery started for ranges: %s"), (const TCHAR *)ranges);
      }
   }
   else
   {
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);
      WriteAuditLog(AUDIT_SYSCFG, false, m_dwUserId, m_workstation, m_id, 0, _T("Access denied on manual active discovery"));
   }

   sendMessage(&msg);
}

/**
 * Get physical links
 */
void ClientSession::getPhysicalLinks(NXCPMessage *request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request->getId());
   bool success = true;
   shared_ptr<NetObj> object = FindObjectById(request->getFieldAsInt32(VID_OBJECT_ID));
   if(request->isFieldExist(VID_OBJECT_ID)) //Field won't exist if we need full list
   {
      if (object == nullptr)
      {
         msg.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
         success = false;
      }
      else if (!object->checkAccessRights(getUserId(), OBJECT_ACCESS_READ))
      {
         WriteAuditLog(AUDIT_SYSCFG, false, m_dwUserId, m_workstation, m_id, object->getId(), _T("Access denied on object read"));
         msg.setField(VID_RCC, RCC_ACCESS_DENIED);
         success = false;
      }
   }

   if (success)
   {
      GetObjectPhysicalLinks(object.get(), getUserId(), request->getFieldAsInt32(VID_PATCH_PANNEL_ID), &msg);
      msg.setField(VID_RCC, RCC_SUCCESS);
   }

   sendMessage(&msg);
}

/**
 * Create or modify physical link
 */
void ClientSession::updatePhysicalLink(NXCPMessage *request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request->getId());

   uint32_t rcc = AddPhysicalLink(request, getUserId());
   if (rcc == RCC_SUCCESS)
   {
      msg.setField(VID_RCC, RCC_SUCCESS);
      WriteAuditLog(AUDIT_SYSCFG, true, m_dwUserId, m_workstation, m_id, 0, _T("Physical link [%u] updated"), request->getFieldAsUInt32(VID_OBJECT_LINKS_BASE));
   }
   else if (rcc == RCC_ACCESS_DENIED)
   {
      WriteAuditLog(AUDIT_SYSCFG, false, m_dwUserId, m_workstation, m_id, 0, _T("Access denied on physical link [%u] update"), request->getFieldAsUInt32(VID_OBJECT_LINKS_BASE));
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);
   }
   else
   {
      msg.setField(VID_RCC, RCC_ENDPOINT_ALREADY_IN_USE);
   }

   sendMessage(&msg);
}

/**
 * Delete physical link
 */
void ClientSession::deletePhysicalLink(NXCPMessage *request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request->getId());

   if (DeletePhysicalLink(request->getFieldAsUInt32(VID_PHYSICAL_LINK_ID), getUserId()))
   {
      WriteAuditLog(AUDIT_SYSCFG, true, m_dwUserId, m_workstation, m_id, 0, _T("Physical link [%u] deleted"), request->getFieldAsUInt32(VID_PHYSICAL_LINK_ID));
      msg.setField(VID_RCC, RCC_SUCCESS);
   }
   else
   {
      WriteAuditLog(AUDIT_SYSCFG, false, m_dwUserId, m_workstation, m_id, 0, _T("Access denied on physical link [%u] delete"), request->getFieldAsUInt32(VID_PHYSICAL_LINK_ID));
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);
   }

   sendMessage(&msg);
}

/**
 * Get configured web service definitions
 */
void ClientSession::getWebServices(NXCPMessage *request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request->getId());

   if (m_systemAccessRights & SYSTEM_ACCESS_WEB_SERVICE_DEFINITIONS)
   {
      SharedObjectArray<WebServiceDefinition> *definitions = GetWebServiceDefinitions();

      response.setField(VID_RCC, RCC_SUCCESS);
      response.setField(VID_NUM_ELEMENTS, definitions->size());
      sendMessage(&response);

      response.setCode(CMD_WEB_SERVICE_DEFINITION);
      for(int i = 0; i < definitions->size(); i++)
      {
         response.deleteAllFields();
         definitions->get(i)->fillMessage(&response);
         sendMessage(&response);
      }

      delete definitions;
   }
   else
   {
      WriteAuditLog(AUDIT_SYSCFG, false, m_dwUserId, m_workstation, m_id, 0, _T("Access denied on reading configured web service definitions"));
      response.setField(VID_RCC, RCC_ACCESS_DENIED);
      sendMessage(&response);
   }
}

/**
 * Modify web service definition
 */
void ClientSession::modifyWebService(NXCPMessage *request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request->getId());

   if (m_systemAccessRights & SYSTEM_ACCESS_WEB_SERVICE_DEFINITIONS)
   {
      SharedString name = request->getFieldAsSharedString(VID_NAME);
      uint32_t rcc;
      auto existing = FindWebServiceDefinition(name);
      if (existing != nullptr && request->getFieldAsUInt32(VID_WEBSVC_ID) != existing->getId()) //Prevent new web service definition creation with the same name
      {
         rcc = RCC_NAME_ALEARDY_EXISTS;
      }
      else
      {
         auto definition = make_shared<WebServiceDefinition>(request);
         rcc = ModifyWebServiceDefinition(definition);
         if (rcc == RCC_SUCCESS)
         {
            response.setField(VID_WEBSVC_ID, definition->getId());
            WriteAuditLog(AUDIT_SYSCFG, true, m_dwUserId, m_workstation, m_id, 0, _T("Web service definition \"%s\" [%u] modified"),
                     definition->getName(), definition->getId());
         }
      }
      response.setField(VID_RCC, rcc);
   }
   else
   {
      WriteAuditLog(AUDIT_SYSCFG, false, m_dwUserId, m_workstation, m_id, 0, _T("Access denied on changing web service definition"));
      response.setField(VID_RCC, RCC_ACCESS_DENIED);
   }

   sendMessage(&response);
}

/**
 * Delete web service definition
 */
void ClientSession::deleteWebService(NXCPMessage *request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request->getId());

   if (m_systemAccessRights & SYSTEM_ACCESS_WEB_SERVICE_DEFINITIONS)
   {
      uint32_t id = request->getFieldAsUInt32(VID_WEBSVC_ID);
      uint32_t rcc = DeleteWebServiceDefinition(id);
      response.setField(VID_RCC, rcc);
      if (rcc == RCC_SUCCESS)
         WriteAuditLog(AUDIT_SYSCFG, true, m_dwUserId, m_workstation, m_id, 0, _T("Web service definition [%u] deleted"), id);
   }
   else
   {
      WriteAuditLog(AUDIT_SYSCFG, false, m_dwUserId, m_workstation, m_id, 0, _T("Access denied on deleting web service definition"));
      response.setField(VID_RCC, RCC_ACCESS_DENIED);
   }

   sendMessage(&response);
}

/**
 * Get configured object categories
 */
void ClientSession::getObjectCategories(NXCPMessage *request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request->getId());
   ObjectCategoriesToMessage(&response);
   response.setField(VID_RCC, RCC_SUCCESS);
   sendMessage(&response);
}

/**
 * Modify object category
 */
void ClientSession::modifyObjectCategory(NXCPMessage *request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request->getId());

   if (m_systemAccessRights & SYSTEM_ACCESS_OBJECT_CATEGORIES)
   {
      uint32_t categoryId;
      uint32_t rcc = ModifyObjectCategory(*request, &categoryId);
      if (rcc == RCC_SUCCESS)
      {
         response.setField(VID_CATEGORY_ID, categoryId);
         shared_ptr<ObjectCategory> category = GetObjectCategory(categoryId);
         writeAuditLog(AUDIT_SYSCFG, true, 0, _T("Object category \"%s\" [%u] modified"),
                  (category != nullptr) ? category->getName() : _T("<unknown>"), categoryId);
      }
      response.setField(VID_RCC, rcc);
   }
   else
   {
      writeAuditLog(AUDIT_SYSCFG, false, 0, _T("Access denied on changing object category"));
      response.setField(VID_RCC, RCC_ACCESS_DENIED);
   }

   sendMessage(&response);
}

/**
 * Delete object category
 */
void ClientSession::deleteObjectCategory(NXCPMessage *request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request->getId());

   if (m_systemAccessRights & SYSTEM_ACCESS_OBJECT_CATEGORIES)
   {
      uint32_t id = request->getFieldAsUInt32(VID_CATEGORY_ID);
      uint32_t rcc = DeleteObjectCategory(id, request->getFieldAsBoolean(VID_FORCE_DELETE));
      response.setField(VID_RCC, rcc);
      if (rcc == RCC_SUCCESS)
         writeAuditLog(AUDIT_SYSCFG, true, 0, _T("Object category [%u] deleted"), id);
   }
   else
   {
      writeAuditLog(AUDIT_SYSCFG, false, 0, _T("Access denied on deleting object category"));
      response.setField(VID_RCC, RCC_ACCESS_DENIED);
   }

   sendMessage(&response);
}

/**
 * Get configured geo areas
 */
void ClientSession::getGeoAreas(NXCPMessage *request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request->getId());
   GeoAreasToMessage(&response);
   response.setField(VID_RCC, RCC_SUCCESS);
   sendMessage(&response);
}

/**
 * Modify geo area
 */
void ClientSession::modifyGeoArea(NXCPMessage *request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request->getId());

   if (m_systemAccessRights & SYSTEM_ACCESS_MANAGE_GEO_AREAS)
   {
      uint32_t areaId;
      uint32_t rcc = ModifyGeoArea(*request, &areaId);
      if (rcc == RCC_SUCCESS)
      {
         response.setField(VID_AREA_ID, areaId);
         shared_ptr<GeoArea> area = GetGeoArea(areaId);
         writeAuditLog(AUDIT_SYSCFG, true, 0, _T("Geo area \"%s\" [%u] modified"),
                  (area != nullptr) ? area->getName() : _T("<unknown>"), areaId);
      }
      response.setField(VID_RCC, rcc);
   }
   else
   {
      writeAuditLog(AUDIT_SYSCFG, false, 0, _T("Access denied on changing geo area"));
      response.setField(VID_RCC, RCC_ACCESS_DENIED);
   }

   sendMessage(&response);
}

/**
 * Delete geo area
 */
void ClientSession::deleteGeoArea(NXCPMessage *request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request->getId());

   if (m_systemAccessRights & SYSTEM_ACCESS_MANAGE_GEO_AREAS)
   {
      uint32_t id = request->getFieldAsUInt32(VID_AREA_ID);
      uint32_t rcc = DeleteGeoArea(id, request->getFieldAsBoolean(VID_FORCE_DELETE));
      response.setField(VID_RCC, rcc);
      if (rcc == RCC_SUCCESS)
         writeAuditLog(AUDIT_SYSCFG, true, 0, _T("Geo area [%u] deleted"), id);
   }
   else
   {
      writeAuditLog(AUDIT_SYSCFG, false, 0, _T("Access denied on deleting geo area"));
      response.setField(VID_RCC, RCC_ACCESS_DENIED);
   }

   sendMessage(&response);
}

/**
 * Update shared secret list
 */
void ClientSession::updateSharedSecretList(NXCPMessage *pRequest)
{
   NXCPMessage msg;
   UINT32 rcc = RCC_SUCCESS;

   msg.setId(pRequest->getId());
   msg.setCode(CMD_REQUEST_COMPLETED);

   if (m_systemAccessRights & SYSTEM_ACCESS_SERVER_CONFIG)
   {
      int32_t zoneUIN = pRequest->getFieldAsInt32(VID_ZONE_UIN);
      shared_ptr<Zone> zone = FindZoneByUIN(zoneUIN);
      if (zoneUIN == -1 || zone != nullptr)
      {
         DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
         if (DBBegin(hdb))
         {
            ExecuteQueryOnObject(hdb, zoneUIN, _T("DELETE FROM shared_secrets WHERE zone=?"));
            UINT32 baseId = VID_SHARED_SECRET_LIST_BASE;
            DB_STATEMENT hStmt = DBPrepare(hdb, _T("INSERT INTO shared_secrets (id,secret,zone) VALUES(?,?,?)"), true);
            if (hStmt != nullptr)
            {
               DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, zoneUIN);
               int count = pRequest->getFieldAsUInt32(VID_NUM_ELEMENTS);
               for(int i = 0; i < count; i++)
               {
                  DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, i + 1);
                  DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, pRequest->getFieldAsString(baseId++), DB_BIND_DYNAMIC);
                  if (!DBExecute(hStmt))
                  {
                     rcc = RCC_DB_FAILURE;
                     break;
                  }
               }
               DBFreeStatement(hStmt);
            }
            else
               rcc = RCC_DB_FAILURE;

            if (rcc == RCC_SUCCESS)
            {
               DBCommit(hdb);
               NotifyClientSessions(NX_NOTIFY_SECRET_CONFIG_CHANGED, zoneUIN);
            }
            else
               DBRollback(hdb);
         }
         else
            rcc = RCC_DB_FAILURE;
         DBConnectionPoolReleaseConnection(hdb);
      }
      else
         rcc = RCC_INVALID_ZONE_ID;
   }
   else
      rcc = RCC_ACCESS_DENIED;

   msg.setField(VID_RCC, rcc);
   sendMessage(&msg);
}

/**
 * Update SNMP port list
 */
void ClientSession::updateSNMPPortList(NXCPMessage *request)
{
   uint32_t rcc = RCC_SUCCESS;
   if (m_systemAccessRights & SYSTEM_ACCESS_SERVER_CONFIG)
   {
      int32_t zoneUIN = request->getFieldAsInt32(VID_ZONE_UIN);
      shared_ptr<Zone> zone = FindZoneByUIN(zoneUIN);
      if (zoneUIN == -1 || zone != nullptr)
      {
         DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
         if (DBBegin(hdb))
         {
            ExecuteQueryOnObject(hdb, zoneUIN, _T("DELETE FROM snmp_ports WHERE zone=?"));
            UINT32 baseId = VID_ZONE_SNMP_PORT_LIST_BASE;
            DB_STATEMENT hStmt = DBPrepare(hdb, _T("INSERT INTO snmp_ports (id,port,zone) VALUES(?,?,?)"), true);
            if (hStmt != nullptr)
            {
               int count = request->getFieldAsUInt32(VID_ZONE_SNMP_PORT_COUNT);
               DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, zoneUIN);
               for(int i = 0; i < count; i++)
               {
                  DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, i + 1);
                  DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, request->getFieldAsUInt16(baseId++));
                  if (!DBExecute(hStmt))
                  {
                     rcc = RCC_DB_FAILURE;
                     break;
                  }
               }
               DBFreeStatement(hStmt);
            }
            else
            {
               rcc = RCC_DB_FAILURE;
            }

            if (rcc == RCC_SUCCESS)
            {
               DBCommit(hdb);
               NotifyClientSessions(NX_NOTIFY_PORTS_CONFIG_CHANGED, zoneUIN);
            }
            else
            {
               DBRollback(hdb);
            }
         }
         else
         {
            rcc = RCC_DB_FAILURE;
         }
         DBConnectionPoolReleaseConnection(hdb);
      }
      else
      {
         rcc = RCC_INVALID_ZONE_ID;
      }
   }
   else
   {
      rcc = RCC_ACCESS_DENIED;
   }

   NXCPMessage msg(CMD_REQUEST_COMPLETED, request->getId());
   msg.setField(VID_RCC, rcc);
   sendMessage(&msg);
}

/**
 * Find proxy for node
 */
void ClientSession::findProxyForNode(NXCPMessage *request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request->getId());

   shared_ptr<Node> node = static_pointer_cast<Node>(FindObjectById(request->getFieldAsUInt32(VID_NODE_ID), OBJECT_NODE));
   if (node != nullptr)
   {
      if (node->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
      {
         msg.setField(VID_AGENT_PROXY, node->getEffectiveAgentProxy());
         msg.setField(VID_RCC, RCC_SUCCESS);
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

#ifdef WITH_ZMQ

/**
 * Manage subscription for ZMQ forwarder
 */
void ClientSession::zmqManageSubscription(NXCPMessage *request, zmq::SubscriptionType type, bool subscribe)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request->getId());

   UINT32 objectId = request->getFieldAsUInt32(VID_OBJECT_ID);
   shared_ptr<NetObj> object = FindObjectById(objectId);
   if ((object != nullptr) && !object->isDeleted())
   {
      if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
      {
         UINT32 dciId = request->getFieldAsUInt32(VID_DCI_ID);
         UINT32 eventCode = request->getFieldAsUInt32(VID_EVENT_CODE);
         bool success;
         switch (type)
         {
            case zmq::EVENT:
               if (subscribe)
               {
                  success = ZmqSubscribeEvent(objectId, eventCode, dciId);
               }
               else
               {
                  success = ZmqUnsubscribeEvent(objectId, eventCode, dciId);
               }
               break;
            case zmq::DATA:
               if (subscribe)
               {
                  success = ZmqSubscribeData(objectId, dciId);
               }
               else
               {
                  success = ZmqUnsubscribeData(objectId, dciId);
               }
               break;
         }
         msg.setField(VID_RCC, success ? RCC_SUCCESS : RCC_INTERNAL_ERROR);
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

void ClientSession::zmqListSubscriptions(NXCPMessage *request, zmq::SubscriptionType type)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request->getId());
   ZmqFillSubscriptionListMessage(&msg, type);
   msg.setField(VID_RCC, RCC_SUCCESS);
   sendMessage(&msg);
}

#endif
