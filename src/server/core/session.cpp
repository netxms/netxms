/*
** NetXMS - Network Management System
** Copyright (C) 2003-2023 Raden Solutions
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
#include <nxcore_jobs.h>
#include <nxcore_syslog.h>
#include <nxcore_ps.h>
#include <nxcore_discovery.h>
#include <nms_pkg.h>
#include <nxcore_2fa.h>
#include <asset_management.h>
#include <netxms-version.h>

#ifdef _WIN32
#include <psapi.h>
#else
#include <dirent.h>
#endif

/**
 * Constants
 */
#define TRAP_CREATE     1
#define TRAP_UPDATE     2
#define TRAP_DELETE     3

#define DEBUG_TAG _T("client.session")

/**
 * Client login information
 */
struct LoginInfo
{
   TwoFactorAuthenticationToken *token;
   TCHAR loginName[MAX_USER_NAME];
   uint32_t graceLogins;
   bool changePassword;
   bool closeOtherSessions;
   bool intruderLockout;

   LoginInfo()
   {
      token = nullptr;
      graceLogins = 0;
      changePassword = false;
      closeOtherSessions = false;
      intruderLockout = false;
   }
   ~LoginInfo()
   {
      delete token;
   }
};

/**
 * Externals
 */
extern ThreadPool *g_clientThreadPool;
extern ThreadPool *g_dataCollectorThreadPool;

void UnregisterClientSession(session_id_t id);
void ResetDiscoveryPoller();
NXCPMessage *ForwardMessageToReportingServer(NXCPMessage *request, ClientSession *session);
void RemovePendingFileTransferRequests(ClientSession *session);
bool UpdateAddressListFromMessage(const NXCPMessage& msg);
void FillComponentsMessage(NXCPMessage *msg);
void GetClientConfigurationHints(NXCPMessage *msg);

void GetPredictionEngines(NXCPMessage *msg);
bool GetPredictedData(ClientSession *session, const NXCPMessage& request, NXCPMessage *response, const DataCollectionTarget& dcTarget);

void GetAgentTunnels(NXCPMessage *msg);
uint32_t BindAgentTunnel(uint32_t tunnelId, uint32_t nodeId, uint32_t userId);
uint32_t UnbindAgentTunnel(uint32_t nodeId, uint32_t userId);

void StartManualActiveDiscovery(ObjectArray<InetAddressListElement> *addressList);

void GetObjectPhysicalLinks(const NetObj *object, uint32_t userId, uint32_t patchPanelId, NXCPMessage *msg);
uint32_t AddPhysicalLink(const NXCPMessage& msg, uint32_t userId);
bool DeletePhysicalLink(uint32_t id, uint32_t userId);

unique_ptr<ObjectArray<EventReference>> GetAllEventReferences(uint32_t eventCode);

uint32_t ReadMaintenanceJournal(SharedObjectArray<NetObj>& sources, NXCPMessage* response, uint32_t maxEntries);
uint32_t AddMaintenanceJournalRecord(const NXCPMessage& request, uint32_t userId);
uint32_t UpdateMaintenanceJournalRecord(const NXCPMessage& request, uint32_t userId);

/**
 * Maximum client message size
 */
uint64_t g_maxClientMessageSize = 4 * 1024 * 1024; // 4MB

/**
 * Client session console constructor
 */
ClientSessionConsole::ClientSessionConsole(ClientSession *session, uint16_t msgCode)
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
ClientSession::ClientSession(SOCKET hSocket, const InetAddress& addr) : m_downloadFileMap(Ownership::True), m_condEncryptionSetup(false),
         m_subscriptions(Ownership::True), m_subscriptionLock(MutexType::FAST), m_pendingObjectNotificationsLock(MutexType::FAST)
{
   m_id = -1;
   m_socket = hSocket;
   m_socketPoller = nullptr;
   m_messageReceiver = nullptr;
   m_loginInfo = nullptr;
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
	m_console = nullptr;
   m_loginTime = time(nullptr);
   m_soundFileTypes.add(_T("wav"));
   _tcscpy(m_language, _T("en"));
   m_tcpProxyConnections = new ObjectArray<TcpProxy>(0, 16, Ownership::True);
   m_tcpProxyChannelId = 0;
   m_pendingObjectNotifications = new HashSet<uint32_t>();
   m_objectNotificationScheduled = false;
   m_objectNotificationBatchSize = 500;
   m_objectNotificationDelay = 200;
   m_lastScreenshotObject = 0;
   m_lastScreenshotTime = 0;
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
   if (m_ppEPPRuleList != nullptr)
   {
      if (m_flags & CSF_EPP_UPLOAD)  // Aborted in the middle of EPP transfer
      {
         for(UINT32 i = 0; i < m_dwRecordsUploaded; i++)
            delete m_ppEPPRuleList[i];
      }
      MemFree(m_ppEPPRuleList);
   }

	delete m_console;

   m_soundFileTypes.clear();

   m_downloadFileMap.forEach([] (const uint32_t& key, ServerDownloadFileInfo *file) -> EnumerationCallbackResult
      {
         file->close(false);
         return _CONTINUE;
      });

   delete m_tcpProxyConnections;
   delete m_pendingObjectNotifications;

   delete m_loginInfo;

   debugPrintf(5, _T("Session object destroyed"));
}

/**
 * Start session
 */
bool ClientSession::start()
{
   if (m_socketPoller == nullptr)
      return false;
   m_messageReceiver = new SocketMessageReceiver(m_socket, 4096, static_cast<size_t>(g_maxClientMessageSize));
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
   m_downloadFileMap.forEach(ClientSession::checkFileTransfer, &context);
   for (int i = 0; i < cancelList.size(); i++)
   {
      m_downloadFileMap.remove(cancelList.get(i));
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
void ClientSession::writeAuditLog(const TCHAR *subsys, bool success, uint32_t objectId, const TCHAR *format, ...) const
{
   va_list args;
   va_start(args, format);
   WriteAuditLog2(subsys, success, m_dwUserId, m_workstation, m_id, objectId, format, args);
   va_end(args);
}

/**
 * Write audit log with old and new values for changed entity
 */
void ClientSession::writeAuditLogWithValues(const TCHAR *subsys, bool success, uint32_t objectId, const TCHAR *oldValue, const TCHAR *newValue, char valueType, const TCHAR *format, ...) const
{
   va_list args;
   va_start(args, format);
   WriteAuditLogWithValues2(subsys, success, m_dwUserId, m_workstation, m_id, objectId, oldValue, newValue, valueType, format, args);
   va_end(args);
}

/**
 * Write audit log with old and new values for changed entity
 */
void ClientSession::writeAuditLogWithValues(const TCHAR *subsys, bool success, uint32_t objectId, json_t *oldValue, json_t *newValue, const TCHAR *format, ...) const
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
   m_subscriptionLock.lock();
   bool subscribed = m_subscriptions.contains(channel);
   m_subscriptionLock.unlock();
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
         m_tcpProxyLock.lock();
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
         m_tcpProxyLock.unlock();
         if ((conn != nullptr) && conn->isConnected())
         {
            size_t size = msg->getBinaryDataSize();
            size_t msgSize = size + NXCP_HEADER_SIZE;
            if (msgSize % 8 != 0)
               msgSize += 8 - msgSize % 8;
            NXCP_MESSAGE *fwmsg = static_cast<NXCP_MESSAGE*>(MemAlloc(msgSize));
            fwmsg->code = htons(CMD_TCP_PROXY_DATA);
            fwmsg->flags = htons(MF_BINARY);
            fwmsg->id = htonl(agentChannelId);
            fwmsg->numFields = htonl(static_cast<uint32_t>(size));
            fwmsg->size = htonl(static_cast<uint32_t>(msgSize));
            memcpy(fwmsg->fields, msg->getBinaryData(), size);
            conn->postRawMessage(fwmsg);
         }
         else
         {
            debugPrintf(5, _T("Missing or broken TCP proxy channel %u"), msg->getId());
            if (conn != nullptr)
            {
               m_tcpProxyLock.lock();
               for(int i = 0; i < m_tcpProxyConnections->size(); i++)
               {
                  TcpProxy *p = m_tcpProxyConnections->get(i);
                  if (p->clientChannelId == msg->getId())
                  {
                     m_tcpProxyConnections->remove(i);
                     break;
                  }
               }
               m_tcpProxyLock.unlock();
            }

            NXCPMessage response(CMD_CLOSE_TCP_PROXY, 0);
            response.setField(VID_CHANNEL_ID, msg->getId());
            response.setField(VID_RCC, RCC_COMM_FAILURE);
            postMessage(response);
         }
      }
      delete msg;
   }
   else
   {
      if ((msg->getCode() == CMD_SESSION_KEY) && (msg->getId() == m_encryptionRqId))
      {
         debugPrintf(6, _T("Received message CMD_SESSION_KEY"));
         NXCPEncryptionContext *encryptionContext = nullptr;
         m_encryptionResult = SetupEncryptionContext(msg, &encryptionContext, nullptr, g_serverKey, NXCP_VERSION);
         m_encryptionContext = shared_ptr<NXCPEncryptionContext>(encryptionContext);
         m_messageReceiver->setEncryptionContext(m_encryptionContext);
         m_condEncryptionSetup.set();
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

   RemovePendingFileTransferRequests(this);
   RemoveFileMonitorsBySessionId(m_id);
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
   uint32_t rcc = conn->waitForRCC(requestId, 60000); // Wait up to 60 seconds for final confirmation
   debugPrintf(6, _T("File transfer request %u: %s"), requestId, AgentErrorCodeToText(rcc));

   // Send final confirmation to client
   NXCPMessage response(CMD_REQUEST_COMPLETED, requestId);
   response.setEndOfSequence();
   response.setField(VID_RCC, AgentErrorToRCC(rcc));
   sendMessage(response);
   decRefCount();
}

/**
 * Process file transfer message
 */
void ClientSession::processFileTransferMessage(NXCPMessage *msg)
{
   ServerDownloadFileInfo *dInfo = m_downloadFileMap.get(msg->getId());
   if (dInfo != nullptr)
   {
      if (msg->getCode() == CMD_FILE_DATA)
      {
         if (dInfo->write(msg->getBinaryData(), msg->getBinaryDataSize(), msg->isCompressedStream()))
         {
            if (msg->isEndOfFile())
            {
               debugPrintf(6, _T("Got end of file marker"));

               NXCPMessage response(CMD_REQUEST_COMPLETED, msg->getId());
               response.setField(VID_RCC, RCC_SUCCESS);
               sendMessage(response);

               dInfo->close(true);
               m_downloadFileMap.remove(msg->getId());
            }
         }
         else
         {
            debugPrintf(6, _T("I/O error"));

            NXCPMessage response(CMD_REQUEST_COMPLETED, msg->getId());
            response.setField(VID_RCC, RCC_IO_ERROR);
            sendMessage(response);

            dInfo->close(false);
            m_downloadFileMap.remove(msg->getId());
         }
      }
      else
      {
         // Abort current file transfer because of client's problem
         dInfo->close(false);
         m_downloadFileMap.remove(msg->getId());
      }
   }
   else
   {
      shared_ptr<AgentFileTransfer> ft = m_agentFileTransfers.get(msg->getId());
      if (ft != nullptr)
      {
         if (msg->getCode() == CMD_FILE_DATA)
         {
            // Check if we should start caching file on file system - this could happen if connection to agent is much slower
            // than connection from client (32 requests is about 1MB of file data)
            if ((ft->fileName == nullptr) && (ThreadPoolGetSerializedRequestCount(g_clientThreadPool, ft->key) > 32))
            {
               debugPrintf(6, _T("Slow file send to agent (request ID %u), start caching on file system"), msg->getId());
               TCHAR fileName[MAX_PATH];
               GetNetXMSDirectory(nxDirData, fileName);
               _tcslcat(fileName, FS_PATH_SEPARATOR, MAX_PATH);
               _tcslcat(fileName, ft->key, MAX_PATH);
               ft->fileName = MemCopyString(fileName);
               incRefCount();
               ThreadPoolExecuteSerialized(g_clientThreadPool, ft->key, this, &ClientSession::agentFileTransferFromFile, ft);
            }

            if (ft->fileName != nullptr)
            {
               bool success;
               ft->mutex.lock();
               int fd = _topen(ft->fileName, O_CREAT | O_APPEND | O_WRONLY | O_BINARY, S_IRUSR | S_IWUSR);
               if (fd != -1)
               {
                  NXCP_MESSAGE *data = msg->serialize(ft->connection->isCompressionAllowed());
                  int bytes = static_cast<int>(ntohl(data->size));
                  success = (_write(fd, data, bytes) == bytes);
                  _close(fd);
                  MemFree(data);
               }
               else
               {
                  success = false;
               }
               ft->mutex.unlock();
               if (success && msg->isEndOfFile())
               {
                  ft->active = false;
               }
               else if (!success)
               {
                  debugPrintf(6, _T("Error writing to file transfer cache file (request ID %u): %s"), msg->getId(), _tcserror(errno));

                  NXCPMessage response(CMD_REQUEST_COMPLETED, msg->getId());
                  response.setField(VID_RCC, RCC_COMM_FAILURE);
                  sendMessage(response);

                  ft->failure = true;
                  ft->active = false;
                  m_agentFileTransfers.remove(msg->getId());
               }
            }
            else
            {
               incRefCount();
               ThreadPoolExecuteSerialized(g_clientThreadPool, ft->key, this, &ClientSession::sendAgentFileTransferMessage, ft, msg);
               msg = nullptr;
            }
         }
         else if (msg->getCode() == CMD_ABORT_FILE_TRANSFER)
         {
            // Resend abort message
            incRefCount();
            ft->failure = true;
            ft->active = false;
            ThreadPoolExecuteSerialized(g_clientThreadPool, ft->key, this, &ClientSession::sendAgentFileTransferMessage, ft, msg);
            m_agentFileTransfers.remove(msg->getId());
            msg = nullptr;
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
 * Send agent file transfer message
 */
void ClientSession::sendAgentFileTransferMessage(shared_ptr<AgentFileTransfer> ft, NXCPMessage *msg)
{
   bool success = ft->connection->sendMessage(msg);
   if (msg->getCode() == CMD_FILE_DATA)
   {
      if (success)
      {
         if (msg->isEndOfFile())
         {
            debugPrintf(6, _T("Got end of file marker for request ID %u"), msg->getId());
            incRefCount();
            ThreadPoolExecuteSerialized(g_clientThreadPool, ft->key, this, &ClientSession::finalizeFileTransferToAgent, ft->connection, msg->getId());
            m_agentFileTransfers.remove(msg->getId());
         }
         else if (ft->reportProgress)
         {
            // Update number of bytes transferred
            uint64_t bytes;
            if (msg->isCompressedStream())
            {
               const BYTE *payload = msg->getBinaryData();
               bytes = (static_cast<uint64_t>(payload[2]) << 8) | static_cast<uint64_t>(payload[3]);
            }
            else
            {
               bytes = msg->getBinaryDataSize();
            }
            ft->bytesTransferred += bytes;

            time_t now = time(nullptr);
            if (now - ft->lastReportTime >= 2)
            {
               NXCPMessage response(CMD_REQUEST_COMPLETED, msg->getId());
               response.setField(VID_RCC, RCC_SUCCESS);
               response.setField(VID_FILE_SIZE, ft->bytesTransferred);
               sendMessage(response);
               ft->lastReportTime = now;
            }
         }
      }
      else
      {
         debugPrintf(6, _T("Error while sending file to agent (request ID %u)"), msg->getId());
         m_agentFileTransfers.remove(msg->getId());

         NXCPMessage response(CMD_REQUEST_COMPLETED, msg->getId());
         response.setField(VID_RCC, RCC_COMM_FAILURE);
         sendMessage(response);
      }
   }
   delete msg;
   decRefCount();
}

/**
 * Send data from agent file transfer cache file
 */
bool ClientSession::sendDataFromCacheFile(AgentFileTransfer *ft)
{
   if (_taccess(ft->fileName, 0) != 0)
      return true;   // Assume cache file does not exist yet

   TCHAR tempFileName[MAX_PATH];
   _tcscpy(tempFileName, ft->fileName);
   _tcscat(tempFileName, _T(".out"));

   ft->mutex.lock();
   _trename(ft->fileName, tempFileName);
   ft->mutex.unlock();

   int fd = _topen(tempFileName, O_RDONLY | O_BINARY);
   if (fd == -1)
   {
      debugPrintf(6, _T("Cannot open file transfer cache file %s (%s)"), tempFileName, _tcserror(errno));
      _tremove(tempFileName);
      return false;
   }

   bool success = true;
   uint32_t msgCount = 0;
   time_t lastProbeTime = time(nullptr);
   size_t bufferSize = FILE_BUFFER_SIZE + 64;
   char *buffer = MemAllocArrayNoInit<char>(bufferSize);

   while(true)
   {
      int bytes = _read(fd, buffer, NXCP_HEADER_SIZE);
      if (bytes < 0)
      {
         debugPrintf(6, _T("Cannot read file transfer cache file %s (%s)"), tempFileName, _tcserror(errno));
         success = false;
         break;
      }

      if (bytes == 0)
         break;

      if (bytes < NXCP_HEADER_SIZE)
      {
         debugPrintf(6, _T("Corrupted file transfer cache file %s"), tempFileName);
         success = false;
         break;
      }

      int msgSize = ntohl(reinterpret_cast<NXCP_MESSAGE*>(buffer)->size);
      if (msgSize > bufferSize)
      {
         bufferSize = msgSize;
         buffer = MemRealloc(buffer, bufferSize);
      }

      bytes = _read(fd, buffer + NXCP_HEADER_SIZE, msgSize - NXCP_HEADER_SIZE);
      if (bytes != msgSize - NXCP_HEADER_SIZE)
      {
         if (bytes < 0)
            debugPrintf(6, _T("Cannot read file transfer cache file %s (%s)"), tempFileName, _tcserror(errno));
         else
            debugPrintf(6, _T("Corrupted file transfer cache file %s"), tempFileName);
         success = false;
         break;
      }

      success = ft->connection->sendRawMessage(reinterpret_cast<NXCP_MESSAGE*>(buffer));
      if (!success)
      {
         debugPrintf(6, _T("Cannot send data from file transfer cache file %s"), tempFileName);
         break;
      }
      msgCount++;

      // Call nop() method on agent connection to send keepalive request
      // This will throttle file transfer (because server will wait for agent response) and
      // prevent timeout in background poller on server side
      time_t now = time(nullptr);
      if ((now - lastProbeTime > 5) || (msgCount > 8))
      {
         uint32_t rcc = ft->connection->nop();
         if (rcc != ERR_SUCCESS)
         {
            debugPrintf(6, _T("Agent connection validation failed for file transfer %u (%s)"), ft->requestId, AgentErrorCodeToText(rcc));
            success = false;
            break;
         }
         lastProbeTime = now;
         msgCount = 0;
      }

      if (ft->reportProgress)
      {
         // Update number of bytes transferred
         uint16_t flags = ntohs(reinterpret_cast<NXCP_MESSAGE*>(buffer)->flags);
         uint64_t dataBytes;
         if ((flags & (MF_COMPRESSED | MF_STREAM)) == (MF_COMPRESSED | MF_STREAM))
         {
            dataBytes = ntohs(*reinterpret_cast<uint16_t*>(buffer + NXCP_HEADER_SIZE + 2));
         }
         else
         {
            dataBytes = ntohl(reinterpret_cast<NXCP_MESSAGE*>(buffer)->numFields);
         }
         ft->bytesTransferred += dataBytes;

         if (now - ft->lastReportTime >= 5)
         {
            NXCPMessage notification(CMD_REQUEST_COMPLETED, ft->requestId);
            notification.setField(VID_RCC, RCC_SUCCESS);
            notification.setField(VID_FILE_SIZE, ft->bytesTransferred);
            sendMessage(notification);
            ft->lastReportTime = now;
         }
      }
   }

   _close(fd);
   _tremove(tempFileName);
   MemFree(buffer);
   return success;
}

/**
 * Continue file transfer from file
 */
void ClientSession::agentFileTransferFromFile(shared_ptr<AgentFileTransfer> ft)
{
   // If server switched to cached file transfer mode it means that connection to agent is slow
   // Increase command timeout on agent connection so connection checks does not fail
   ft->connection->setCommandTimeout(60000);

   bool failure = false;
   while(ft->active)
   {
      if (!sendDataFromCacheFile(ft.get()))
      {
         failure = true;
         break;
      }
   }

   if (!failure && !ft->failure)
   {
      // Transfer from client complete, send rest of cache file to agent
      if (sendDataFromCacheFile(ft.get()))
      {
         debugPrintf(6, _T("Got end of file marker for request ID %u"), ft->requestId);
         incRefCount();
         ThreadPoolExecuteSerialized(g_clientThreadPool, ft->key, this, &ClientSession::finalizeFileTransferToAgent, ft->connection, ft->requestId);
      }
      else
      {
         failure = true;
      }
   }

   if (failure || ft->failure)
   {
      NXCPMessage response(CMD_REQUEST_COMPLETED, ft->requestId);
      response.setField(VID_RCC, RCC_IO_ERROR);
      sendMessage(response);

      NXCP_MESSAGE *msg = CreateRawNXCPMessage(CMD_FILE_DATA, ft->requestId, MF_STREAM | MF_END_OF_FILE, nullptr, 0, nullptr, false);
      ft->connection->sendRawMessage(msg);
      MemFree(msg);
   }

   m_agentFileTransfers.remove(ft->requestId);
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
       (code != CMD_REGISTER_AGENT) &&
       (code != CMD_2FA_PREPARE_CHALLENGE) &&
       (code != CMD_2FA_VALIDATE_RESPONSE))
   {
      debugPrintf(6, _T("Cannot process request: session is not authenticated"));
      delete request;
      decRefCount();
      return;
   }

   switch(code)
   {
      case CMD_LOGIN:
         login(*request);
         break;
      case CMD_GET_SERVER_INFO:
         sendServerInfo(*request);
         break;
      case CMD_GET_MY_CONFIG:
         sendConfigForAgent(*request);
         break;
      case CMD_GET_OBJECTS:
         getObjects(*request);
         break;
      case CMD_GET_SELECTED_OBJECTS:
         getSelectedObjects(*request);
         break;
      case CMD_QUERY_OBJECTS:
         queryObjects(*request);
         break;
      case CMD_QUERY_OBJECT_DETAILS:
         queryObjectDetails(*request);
         break;
      case CMD_GET_OBJECT_QUERIES:
         getObjectQueries(*request);
         break;
      case CMD_MODIFY_OBJECT_QUERY:
         modifyObjectQuery(*request);
         break;
      case CMD_DELETE_OBJECT_QUERY:
         deleteObjectQuery(*request);
         break;
      case CMD_GET_CONFIG_VARLIST:
         getConfigurationVariables(*request);
         break;
      case CMD_GET_PUBLIC_CONFIG_VAR:
         getPublicConfigurationVariable(*request);
         break;
      case CMD_SET_CONFIG_VARIABLE:
         setConfigurationVariable(*request);
         break;
      case CMD_SET_CONFIG_TO_DEFAULT:
         setDefaultConfigurationVariableValues(*request);
         break;
      case CMD_DELETE_CONFIG_VARIABLE:
         deleteConfigurationVariable(*request);
         break;
      case CMD_CONFIG_GET_CLOB:
         getConfigCLOB(*request);
         break;
      case CMD_CONFIG_SET_CLOB:
         setConfigCLOB(*request);
         break;
      case CMD_GET_ALARM_CATEGORIES:
         getAlarmCategories(*request);
         break;
      case CMD_MODIFY_ALARM_CATEGORY:
         modifyAlarmCategory(*request);
         break;
      case CMD_DELETE_ALARM_CATEGORY:
         deleteAlarmCategory(*request);
         break;
      case CMD_LOAD_EVENT_DB:
         getEventTemplates(*request);
         break;
      case CMD_SET_EVENT_INFO:
         modifyEventTemplate(*request);
         break;
      case CMD_DELETE_EVENT_TEMPLATE:
         deleteEventTemplate(*request);
         break;
      case CMD_GENERATE_EVENT_CODE:
         generateEventCode(*request);
         break;
      case CMD_MODIFY_OBJECT:
         modifyObject(*request);
         break;
      case CMD_SET_OBJECT_MGMT_STATUS:
         changeObjectMgmtStatus(*request);
         break;
      case CMD_ENTER_MAINT_MODE:
         enterMaintenanceMode(*request);
         break;
      case CMD_LEAVE_MAINT_MODE:
         leaveMaintenanceMode(*request);
         break;
      case CMD_LOAD_USER_DB:
         sendUserDB(*request);
         break;
      case CMD_GET_SELECTED_USERS:
         getSelectedUsers(*request);
         break;
      case CMD_CREATE_USER:
         createUser(*request);
         break;
      case CMD_UPDATE_USER:
         updateUser(*request);
         break;
      case CMD_DETACH_LDAP_USER:
         detachLdapUser(*request);
         break;
      case CMD_DELETE_USER:
         deleteUser(*request);
         break;
      case CMD_SET_PASSWORD:
         setPassword(*request);
         break;
      case CMD_VALIDATE_PASSWORD:
         validatePassword(*request);
         break;
      case CMD_GET_NODE_DCI_LIST:
         openNodeDCIList(*request);
         break;
      case CMD_UNLOCK_NODE_DCI_LIST:
         closeNodeDCIList(*request);
         break;
      case CMD_MODIFY_NODE_DCI:
      case CMD_DELETE_NODE_DCI:
         modifyNodeDCI(*request);
         break;
      case CMD_SET_DCI_STATUS:
         changeDCIStatus(*request);
         break;
      case CMD_COPY_DCI:
         copyDCI(*request);
         break;
      case CMD_BULK_DCI_UPDATE:
         bulkDCIUpdate(*request);
         break;
      case CMD_APPLY_TEMPLATE:
         applyTemplate(*request);
         break;
      case CMD_GET_DCI_DATA:
         getCollectedData(*request);
         break;
      case CMD_GET_TABLE_DCI_DATA:
         getTableCollectedData(*request);
         break;
      case CMD_CLEAR_DCI_DATA:
         clearDCIData(*request);
         break;
      case CMD_DELETE_DCI_ENTRY:
         deleteDCIEntry(*request);
         break;
      case CMD_FORCE_DCI_POLL:
         forceDCIPoll(*request);
         break;
      case CMD_RECALCULATE_DCI_VALUES:
         recalculateDCIValues(*request);
         break;
      case CMD_GET_MIB_TIMESTAMP:
         getMIBTimestamp(*request);
         break;
      case CMD_GET_MIB:
         getMIBFile(*request);
         break;
      case CMD_CREATE_OBJECT:
         createObject(*request);
         break;
      case CMD_BIND_OBJECT:
         changeObjectBinding(*request, TRUE);
         break;
      case CMD_UNBIND_OBJECT:
         changeObjectBinding(*request, FALSE);
         break;
      case CMD_ADD_CLUSTER_NODE:
         addClusterNode(*request);
         break;
      case CMD_GET_ALL_ALARMS:
         getAlarms(*request);
         break;
      case CMD_GET_ALARM_COMMENTS:
         getAlarmComments(*request);
         break;
      case CMD_SET_ALARM_STATUS_FLOW:
         updateAlarmStatusFlow(*request);
         break;
      case CMD_UPDATE_ALARM_COMMENT:
         updateAlarmComment(*request);
         break;
      case CMD_DELETE_ALARM_COMMENT:
         deleteAlarmComment(*request);
         break;
      case CMD_GET_ALARM:
         getAlarm(*request);
         break;
      case CMD_GET_ALARM_EVENTS:
         getAlarmEvents(*request);
         break;
      case CMD_ACK_ALARM:
         acknowledgeAlarm(*request);
         break;
      case CMD_RESOLVE_ALARM:
         resolveAlarm(*request, false);
         break;
      case CMD_TERMINATE_ALARM:
         resolveAlarm(*request, true);
         break;
      case CMD_DELETE_ALARM:
         deleteAlarm(*request);
         break;
      case CMD_BULK_RESOLVE_ALARMS:
         bulkResolveAlarms(*request, false);
         break;
      case CMD_BULK_TERMINATE_ALARMS:
         bulkResolveAlarms(*request, true);
         break;
      case CMD_OPEN_HELPDESK_ISSUE:
         openHelpdeskIssue(*request);
         break;
      case CMD_GET_HELPDESK_URL:
         getHelpdeskUrl(*request);
         break;
      case CMD_UNLINK_HELPDESK_ISSUE:
         unlinkHelpdeskIssue(*request);
         break;
      case CMD_CREATE_ACTION:
         createAction(*request);
         break;
      case CMD_MODIFY_ACTION:
         updateAction(*request);
         break;
      case CMD_DELETE_ACTION:
         deleteAction(*request);
         break;
      case CMD_LOAD_ACTIONS:
         sendAllActions(*request);
         break;
      case CMD_DELETE_OBJECT:
         deleteObject(*request);
         break;
      case CMD_POLL_OBJECT:
         forcedObjectPoll(*request);
         break;
      case CMD_TRAP:
         onTrap(*request);
         break;
      case CMD_WAKEUP_NODE:
         onWakeUpNode(*request);
         break;
      case CMD_CREATE_TRAP:
         editTrap(*request, TRAP_CREATE);
         break;
      case CMD_MODIFY_TRAP:
         editTrap(*request, TRAP_UPDATE);
         break;
      case CMD_DELETE_TRAP:
         editTrap(*request, TRAP_DELETE);
         break;
      case CMD_LOAD_TRAP_CFG:
         sendAllTraps(*request);
         break;
      case CMD_GET_TRAP_CFG_RO:
         sendAllTraps2(*request);
         break;
      case CMD_QUERY_PARAMETER:
         queryParameter(*request);
         break;
      case CMD_QUERY_TABLE:
         queryAgentTable(*request);
         break;
      case CMD_GET_PACKAGE_LIST:
         getInstalledPackages(*request);
         break;
      case CMD_INSTALL_PACKAGE:
         installPackage(*request);
         break;
      case CMD_UPDATE_PACKAGE_METADATA:
         updatePackageMetadata(*request);
         break;
      case CMD_REMOVE_PACKAGE:
         removePackage(*request);
         break;
      case CMD_DEPLOY_PACKAGE:
         deployPackage(*request);
         break;
      case CMD_GET_PARAMETER_LIST:
         getParametersList(*request);
         break;
      case CMD_GET_LAST_VALUES:
         getLastValues(*request);
         break;
      case CMD_GET_DCI_VALUES:
         getLastValuesByDciId(*request);
         break;
      case CMD_GET_TOOLTIP_LAST_VALUES:
         getTooltipLastValues(*request);
         break;
      case CMD_GET_TABLE_LAST_VALUE:
         getTableLastValue(*request);
         break;
      case CMD_GET_DCI_LAST_VALUE:
         getLastValue(*request);
         break;
      case CMD_GET_ACTIVE_THRESHOLDS:
         getActiveThresholds(*request);
         break;
      case CMD_GET_THRESHOLD_SUMMARY:
         getThresholdSummary(*request);
         break;
      case CMD_GET_USER_VARIABLE:
         getUserVariable(*request);
         break;
      case CMD_SET_USER_VARIABLE:
         setUserVariable(*request);
         break;
      case CMD_DELETE_USER_VARIABLE:
         deleteUserVariable(*request);
         break;
      case CMD_ENUM_USER_VARIABLES:
         enumUserVariables(*request);
         break;
      case CMD_COPY_USER_VARIABLE:
         copyUserVariable(*request);
         break;
      case CMD_CHANGE_ZONE:
         changeObjectZone(*request);
         break;
      case CMD_REQUEST_ENCRYPTION:
         setupEncryption(*request);
         break;
      case CMD_READ_AGENT_CONFIG_FILE:
         readAgentConfigFile(*request);
         break;
      case CMD_WRITE_AGENT_CONFIG_FILE:
         writeAgentConfigFile(*request);
         break;
      case CMD_EXECUTE_ACTION:
         executeAction(*request);
         break;
      case CMD_GET_OBJECT_TOOLS:
         getObjectTools(*request);
         break;
      case CMD_EXEC_TABLE_TOOL:
         execTableTool(*request);
         break;
      case CMD_GET_OBJECT_TOOL_DETAILS:
         getObjectToolDetails(*request);
         break;
      case CMD_UPDATE_OBJECT_TOOL:
         updateObjectTool(*request);
         break;
      case CMD_DELETE_OBJECT_TOOL:
         deleteObjectTool(*request);
         break;
      case CMD_CHANGE_OBJECT_TOOL_STATUS:
         changeObjectToolStatus(*request);
         break;
      case CMD_GENERATE_OBJECT_TOOL_ID:
         generateObjectToolId(*request);
         break;
      case CMD_CHANGE_SUBSCRIPTION:
         changeSubscription(*request);
         break;
      case CMD_GET_SERVER_STATS:
         sendServerStats(*request);
         break;
      case CMD_GET_SCRIPT_LIST:
         sendScriptList(*request);
         break;
      case CMD_GET_SCRIPT:
         sendScript(*request);
         break;
      case CMD_UPDATE_SCRIPT:
         updateScript(*request);
         break;
      case CMD_RENAME_SCRIPT:
         renameScript(*request);
         break;
      case CMD_DELETE_SCRIPT:
         deleteScript(*request);
         break;
      case CMD_GET_SESSION_LIST:
         getSessionList(*request);
         break;
      case CMD_KILL_SESSION:
         killSession(*request);
         break;
      case CMD_START_SNMP_WALK:
         startSnmpWalk(*request);
         break;
      case CMD_RESOLVE_DCI_NAMES:
         resolveDCINames(*request);
         break;
      case CMD_GET_DCI_INFO:
         getDCIInfo(*request);
         break;
      case CMD_GET_DCI_MEASUREMENT_UNITS:
         getDciMeasurementUnits(*request);
         break;
      case CMD_GET_DCI_THRESHOLDS:
         sendDCIThresholds(*request);
         break;
      case CMD_GET_RELATED_EVENTS_LIST:
         getRelatedEventList(*request);
         break;
      case CMD_GET_DCI_SCRIPT_LIST:
         getDCIScriptList(*request);
         break;
      case CMD_GET_PERFTAB_DCI_LIST:
         getPerfTabDCIList(*request);
         break;
      case CMD_PUSH_DCI_DATA:
         pushDCIData(*request);
         break;
      case CMD_GET_AGENT_CONFIGURATION_LIST:
         getAgentConfigurationList(*request);
         break;
      case CMD_GET_AGENT_CONFIGURATION:
         getAgentConfiguration(*request);
         break;
      case CMD_UPDATE_AGENT_CONFIGURATION:
         updateAgentConfiguration(*request);
         break;
      case CMD_DELETE_AGENT_CONFIGURATION:
         deleteAgentConfiguration(*request);
         break;
      case CMD_SWAP_AGENT_CONFIGURATIONS:
         swapAgentConfigurations(*request);
         break;
      case CMD_GET_OBJECT_COMMENTS:
         getObjectComments(*request);
         break;
      case CMD_UPDATE_OBJECT_COMMENTS:
         updateObjectComments(*request);
         break;
      case CMD_GET_ADDR_LIST:
         getAddrList(*request);
         break;
      case CMD_SET_ADDR_LIST:
         setAddrList(*request);
         break;
      case CMD_RESET_COMPONENT:
         resetComponent(*request);
         break;
      case CMD_EXPORT_CONFIGURATION:
         exportConfiguration(*request);
         break;
      case CMD_IMPORT_CONFIGURATION:
         importConfiguration(*request);
         break;
      case CMD_IMPORT_CONFIGURATION_FILE:
         importConfigurationFromFile(*request);
         break;
      case CMD_GET_GRAPH_LIST:
         getGraphList(*request);
         break;
      case CMD_GET_GRAPH:
         getGraph(*request);
         break;
      case CMD_SAVE_GRAPH:
         saveGraph(*request);
         break;
      case CMD_DELETE_GRAPH:
         deleteGraph(*request);
         break;
      case CMD_QUERY_L2_TOPOLOGY:
         queryL2Topology(*request);
         break;
      case CMD_QUERY_OSPF_TOPOLOGY:
         queryOSPFTopology(*request);
         break;
      case CMD_QUERY_INTERNAL_TOPOLOGY:
         queryInternalCommunicationTopology(*request);
         break;
      case CMD_SEND_NOTIFICATION:
         sendNotification(*request);
         break;
      case CMD_GET_COMMUNITY_LIST:
      case CMD_GET_USM_CREDENTIALS:
      case CMD_GET_WELL_KNOWN_PORT_LIST:
      case CMD_GET_SHARED_SECRET_LIST:
      case CMD_GET_SSH_CREDENTIALS:
         getNetworkCredentials(*request);
         break;
      case CMD_UPDATE_COMMUNITY_LIST:
         updateCommunityList(*request);
         break;
      case CMD_UPDATE_SHARED_SECRET_LIST:
         updateSharedSecretList(*request);
         break;
      case CMD_UPDATE_WELL_KNOWN_PORT_LIST:
         updateWellKnownPortList(*request);
         break;
      case CMD_UPDATE_USM_CREDENTIALS:
         updateUsmCredentials(*request);
         break;
      case CMD_UPDATE_SSH_CREDENTIALS:
         updateSshCredentials(*request);
         break;
      case CMD_GET_PERSISTENT_STORAGE:
         getPersistantStorage(*request);
         break;
      case CMD_SET_PSTORAGE_VALUE:
         setPstorageValue(*request);
         break;
      case CMD_DELETE_PSTORAGE_VALUE:
         deletePstorageValue(*request);
         break;
      case CMD_REGISTER_AGENT:
         registerAgent(*request);
         break;
      case CMD_GET_SERVER_FILE:
         getServerFile(*request);
         break;
      case CMD_GET_AGENT_FILE:
         getAgentFile(*request);
         break;
      case CMD_CANCEL_FILE_MONITORING:
         cancelFileMonitoring(*request);
         break;
      case CMD_TEST_DCI_TRANSFORMATION:
         testDCITransformation(*request);
         break;
      case CMD_EXECUTE_SCRIPT:
         executeScript(*request);
         break;
      case CMD_EXECUTE_LIBRARY_SCRIPT:
         executeLibraryScript(*request);
         break;
      case CMD_GET_JOB_LIST:
         sendJobList(*request);
         break;
      case CMD_CANCEL_JOB:
         cancelJob(*request);
         break;
      case CMD_HOLD_JOB:
         holdJob(*request);
         break;
      case CMD_UNHOLD_JOB:
         unholdJob(*request);
         break;
      case CMD_GET_CURRENT_USER_ATTR:
         getUserCustomAttribute(*request);
         break;
      case CMD_SET_CURRENT_USER_ATTR:
         setUserCustomAttribute(*request);
         break;
      case CMD_OPEN_SERVER_LOG:
         openServerLog(*request);
         break;
      case CMD_CLOSE_SERVER_LOG:
         closeServerLog(*request);
         break;
      case CMD_QUERY_LOG:
         queryServerLog(*request);
         break;
      case CMD_GET_LOG_DATA:
         getServerLogQueryData(*request);
         break;
      case CMD_GET_LOG_RECORD_DETAILS:
         getServerLogRecordDetails(*request);
         break;
      case CMD_FIND_NODE_CONNECTION:
         findNodeConnection(*request);
         break;
      case CMD_FIND_MAC_LOCATION:
         findMacAddress(*request);
         break;
      case CMD_FIND_IP_LOCATION:
         findIpAddress(*request);
         break;
      case CMD_FIND_HOSTNAME_LOCATION:
         findHostname(*request);
         break;
      case CMD_GET_IMAGE:
         sendLibraryImage(*request);
         break;
      case CMD_CREATE_IMAGE:
         updateLibraryImage(*request);
         break;
      case CMD_DELETE_IMAGE:
         deleteLibraryImage(*request);
         break;
      case CMD_MODIFY_IMAGE:
         updateLibraryImage(*request);
         break;
      case CMD_LIST_IMAGES:
         listLibraryImages(*request);
         break;
      case CMD_EXECUTE_SERVER_COMMAND:
         executeServerCommand(*request);
         break;
      case CMD_STOP_SERVER_COMMAND:
         stopServerCommand(*request);
         break;
      case CMD_LIST_SERVER_FILES:
         listServerFileStore(*request);
         break;
      case CMD_UPLOAD_FILE_TO_AGENT:
         uploadFileToAgent(*request);
         break;
      case CMD_UPLOAD_FILE:
         receiveFile(*request);
         break;
      case CMD_RENAME_FILE:
         renameFile(*request);
         break;
      case CMD_DELETE_FILE:
         deleteFile(*request);
         break;
      case CMD_OPEN_CONSOLE:
         openConsole(*request);
         break;
      case CMD_CLOSE_CONSOLE:
         closeConsole(*request);
         break;
      case CMD_ADM_REQUEST:
         processConsoleCommand(*request);
         break;
      case CMD_GET_VLANS:
         getVlans(*request);
         break;
      case CMD_GET_NETWORK_PATH:
         getNetworkPath(*request);
         break;
      case CMD_GET_NODE_COMPONENTS:
         getNodeComponents(*request);
         break;
      case CMD_GET_NODE_SOFTWARE:
         getNodeSoftware(*request);
         break;
      case CMD_GET_NODE_HARDWARE:
         getNodeHardware(*request);
         break;
      case CMD_GET_WINPERF_OBJECTS:
         getWinPerfObjects(*request);
         break;
      case CMD_LIST_MAPPING_TABLES:
         listMappingTables(*request);
         break;
      case CMD_GET_MAPPING_TABLE:
         getMappingTable(*request);
         break;
      case CMD_UPDATE_MAPPING_TABLE:
         updateMappingTable(*request);
         break;
      case CMD_DELETE_MAPPING_TABLE:
         deleteMappingTable(*request);
         break;
      case CMD_GET_WIRELESS_STATIONS:
         getWirelessStations(*request);
         break;
      case CMD_GET_SUMMARY_TABLES:
         getSummaryTables(*request);
         break;
      case CMD_GET_SUMMARY_TABLE_DETAILS:
         getSummaryTableDetails(*request);
         break;
      case CMD_MODIFY_SUMMARY_TABLE:
         modifySummaryTable(*request);
         break;
      case CMD_DELETE_SUMMARY_TABLE:
         deleteSummaryTable(*request);
         break;
      case CMD_QUERY_SUMMARY_TABLE:
         querySummaryTable(*request);
         break;
      case CMD_QUERY_ADHOC_SUMMARY_TABLE:
         queryAdHocSummaryTable(*request);
         break;
      case CMD_GET_SUBNET_ADDRESS_MAP:
         getSubnetAddressMap(*request);
         break;
      case CMD_GET_EFFECTIVE_RIGHTS:
         getEffectiveRights(*request);
         break;
      case CMD_GET_FOLDER_CONTENT:
      case CMD_GET_FOLDER_SIZE:
      case CMD_FILEMGR_COPY_FILE:
      case CMD_FILEMGR_CREATE_FOLDER:
      case CMD_FILEMGR_DELETE_FILE:
      case CMD_FILEMGR_GET_FILE_FINGERPRINT:
      case CMD_FILEMGR_MOVE_FILE:
      case CMD_FILEMGR_RENAME_FILE:
         fileManagerControl(request);
         break;
      case CMD_FILEMGR_UPLOAD:
         uploadUserFileToAgent(request);
         break;
      case CMD_GET_SWITCH_FDB:
         getSwitchForwardingDatabase(*request);
         break;
      case CMD_GET_ROUTING_TABLE:
         getRoutingTable(*request);
         break;
      case CMD_GET_LOC_HISTORY:
         getLocationHistory(*request);
         break;
      case CMD_TAKE_SCREENSHOT:
         getScreenshot(*request);
         break;
      case CMD_COMPILE_SCRIPT:
         compileScript(*request);
         break;
      case CMD_CLEAN_AGENT_DCI_CONF:
         cleanAgentDciConfiguration(*request);
         break;
      case CMD_RESYNC_AGENT_DCI_CONF:
         resyncAgentDciConfiguration(*request);
         break;
      case CMD_LIST_SCHEDULE_CALLBACKS:
         getSchedulerTaskHandlers(*request);
         break;
      case CMD_LIST_SCHEDULES:
         getScheduledTasks(*request);
         break;
      case CMD_ADD_SCHEDULE:
         addScheduledTask(*request);
         break;
      case CMD_UPDATE_SCHEDULE:
         updateScheduledTask(*request);
         break;
      case CMD_REMOVE_SCHEDULE:
         removeScheduledTask(*request);
         break;
      case CMD_GET_REPOSITORIES:
         getRepositories(*request);
         break;
      case CMD_ADD_REPOSITORY:
         addRepository(*request);
         break;
      case CMD_MODIFY_REPOSITORY:
         modifyRepository(*request);
         break;
      case CMD_DELETE_REPOSITORY:
         deleteRepository(*request);
         break;
      case CMD_GET_AGENT_TUNNELS:
         getAgentTunnels(*request);
         break;
      case CMD_BIND_AGENT_TUNNEL:
         bindAgentTunnel(*request);
         break;
      case CMD_UNBIND_AGENT_TUNNEL:
         unbindAgentTunnel(*request);
         break;
      case CMD_GET_PREDICTION_ENGINES:
         getPredictionEngines(*request);
         break;
      case CMD_GET_PREDICTED_DATA:
         getPredictedData(*request);
         break;
      case CMD_EXPAND_MACROS:
         expandMacros(*request);
         break;
      case CMD_SETUP_TCP_PROXY:
         setupTcpProxy(*request);
         break;
      case CMD_GET_AGENT_POLICY_LIST:
         getPolicyList(*request);
         break;
      case CMD_GET_AGENT_POLICY:
         getPolicy(*request);
         break;
      case CMD_UPDATE_AGENT_POLICY:
         updatePolicy(*request);
         break;
      case CMD_DELETE_AGENT_POLICY:
         deletePolicy(*request);
         break;
      case CMD_GET_DEPENDENT_NODES:
         getDependentNodes(*request);
         break;
      case CMD_POLICY_EDITOR_CLOSED:
         onPolicyEditorClose(*request);
         break;
      case CMD_POLICY_FORCE_APPLY:
         forceApplyPolicy(*request);
         break;
      case CMD_GET_MATCHING_DCI:
         getMatchingDCI(*request);
         break;
      case CMD_GET_UA_NOTIFICATIONS:
         getUserAgentNotification(*request);
         break;
      case CMD_ADD_UA_NOTIFICATION:
         addUserAgentNotification(*request);
         break;
      case CMD_RECALL_UA_NOTIFICATION:
         recallUserAgentNotification(*request);
         break;
      case CMD_GET_NOTIFICATION_CHANNELS:
         getNotificationChannels(*request);
         break;
      case CMD_ADD_NOTIFICATION_CHANNEL:
         addNotificationChannel(*request);
         break;
      case CMD_UPDATE_NOTIFICATION_CHANNEL:
         updateNotificationChannel(*request);
         break;
      case CMD_DELETE_NOTIFICATION_CHANNEL:
         removeNotificationChannel(*request);
         break;
      case CMD_RENAME_NOTIFICATION_CHANNEL:
         renameNotificationChannel(*request);
         break;
      case CMD_GET_NOTIFICATION_DRIVERS:
         getNotificationDrivers(*request);
         break;
      case CMD_START_ACTIVE_DISCOVERY:
         startActiveDiscovery(*request);
         break;
      case CMD_GET_PHYSICAL_LINKS:
         getPhysicalLinks(*request);
         break;
      case CMD_UPDATE_PHYSICAL_LINK:
         updatePhysicalLink(*request);
         break;
      case CMD_DELETE_PHYSICAL_LINK:
         deletePhysicalLink(*request);
         break;
      case CMD_GET_WEB_SERVICES:
         getWebServices(*request);
         break;
      case CMD_MODIFY_WEB_SERVICE:
         modifyWebService(*request);
         break;
      case CMD_DELETE_WEB_SERVICE:
         deleteWebService(*request);
         break;
      case CMD_GET_OBJECT_CATEGORIES:
         getObjectCategories(*request);
         break;
      case CMD_MODIFY_OBJECT_CATEGORY:
         modifyObjectCategory(*request);
         break;
      case CMD_DELETE_OBJECT_CATEGORY:
         deleteObjectCategory(*request);
         break;
      case CMD_GET_GEO_AREAS:
         getGeoAreas(*request);
         break;
      case CMD_MODIFY_GEO_AREA:
         modifyGeoArea(*request);
         break;
      case CMD_DELETE_GEO_AREA:
         deleteGeoArea(*request);
         break;
      case CMD_FIND_PROXY_FOR_NODE:
         findProxyForNode(*request);
         break;
      case CMD_GET_SCHEDULED_REPORTING_TASKS:
         getScheduledReportingTasks(*request);
         break;
      case CMD_GET_SSH_KEYS_LIST:
         getSshKeys(*request);
         break;
      case CMD_DELETE_SSH_KEY:
         deleteSshKey(*request);
         break;
      case CMD_UPDATE_SSH_KEYS:
         updateSshKey(*request);
         break;
      case CMD_GENERATE_SSH_KEYS:
         generateSshKey(*request);
         break;
      case CMD_2FA_PREPARE_CHALLENGE:
         prepare2FAChallenge(*request);
         break;
      case CMD_2FA_VALIDATE_RESPONSE:
         validate2FAResponse(*request);
         break;
      case CMD_2FA_GET_DRIVERS:
         get2FADrivers(*request);
         break;
      case CMD_2FA_GET_METHODS:
         get2FAMethods(*request);
         break;
      case CMD_2FA_MODIFY_METHOD:
         modify2FAMethod(*request);
         break;
      case CMD_2FA_RENAME_METHOD:
         rename2FAMethod(*request);
         break;
      case CMD_2FA_DELETE_METHOD:
         delete2FAMethod(*request);
         break;
      case CMD_2FA_GET_USER_BINDINGS:
         getUser2FABindings(*request);
         break;
      case CMD_2FA_MODIFY_USER_BINDING:
         modifyUser2FABinding(*request);
         break;
      case CMD_2FA_DELETE_USER_BINDING:
         deleteUser2FABinding(*request);
         break;
      case CMD_GET_BIZSVC_CHECK_LIST:
         getBusinessServiceCheckList(*request);
         break;
      case CMD_UPDATE_BIZSVC_CHECK:
         modifyBusinessServiceCheck(*request);
         break;
      case CMD_DELETE_BIZSVC_CHECK:
         deleteBusinessServiceCheck(*request);
         break;
      case CMD_GET_BUSINESS_SERVICE_UPTIME:
         getBusinessServiceUptime(*request);
         break;
      case CMD_GET_BUSINESS_SERVICE_TICKETS:
         getBusinessServiceTickets(*request);
         break;
      case CMD_SSH_COMMAND:
         executeSshCommand(*request);
         break;
      case CMD_FIND_DCI:
         findDci(*request);
         break;
      case CMD_GET_EVENT_REFERENCES:
         getEventRefences(*request);
         break;
      case CMD_READ_MAINTENANCE_JOURNAL:
         readMaintenanceJournal(*request);
         break;
      case CMD_WRITE_MAINTENANCE_JOURNAL:
         writeMaintenanceJournal(*request);
         break;
      case CMD_UPDATE_MAINTENANCE_JOURNAL:
         updateMaintenanceJournal(*request);
         break;
      case CMD_CLONE_MAP:
         cloneNetworkMap(*request);
         break;
      case CMD_FIND_VENDOR_BY_MAC:
         findVendorByMac(*request);
         break;
      case CMD_GET_OSPF_DATA:
         getOspfData(*request);
         break;
      case CMD_GET_ASSET_MANAGEMENT_SCHEMA:
         getAssetManagementSchema(*request);
         break;
      case CMD_CREATE_ASSET_ATTRIBUTE:
         createAssetAttribute(*request);
         break;
      case CMD_UPDATE_ASSET_ATTRIBUTE:
         updateAssetAttribute(*request);
         break;
      case CMD_DELETE_ASSET_ATTRIBUTE:
         deleteAssetAttribute(*request);
         break;
      case CMD_SET_ASSET_PROPERTY:
         setAssetProperty(*request);
         break;
      case CMD_DELETE_ASSET_PROPERTY:
         deleteAssetProperty(*request);
         break;
      case CMD_LINK_ASSET:
         linkAsset(*request);
         break;
      case CMD_UNLINK_ASSET:
         unlinkAsset(*request);
         break;
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
   if (m_encryptionContext != nullptr)
   {
      NXCP_ENCRYPTED_MESSAGE *enMsg = m_encryptionContext->encryptMessage(rawMsg);
      if (enMsg != nullptr)
      {
         result = (SendEx(m_socket, (char *)enMsg, ntohl(enMsg->size), 0, &m_mutexSocketWrite) == (int)ntohl(enMsg->size));
         MemFree(enMsg);
      }
      else
      {
         result = false;
      }
   }
   else
   {
      result = (SendEx(m_socket, (const char *)rawMsg, ntohl(rawMsg->size), 0, &m_mutexSocketWrite) == (int)ntohl(rawMsg->size));
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
   if (m_encryptionContext != nullptr)
   {
      NXCP_ENCRYPTED_MESSAGE *emsg = m_encryptionContext->encryptMessage(msg);
      if (emsg != nullptr)
      {
         result = (SendEx(m_socket, (char *)emsg, ntohl(emsg->size), 0, &m_mutexSocketWrite) == (int)ntohl(emsg->size));
         MemFree(emsg);
      }
      else
      {
         result = false;
      }
   }
   else
   {
      result = (SendEx(m_socket, (const char *)msg, ntohl(msg->size), 0, &m_mutexSocketWrite) == (int)ntohl(msg->size));
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
bool ClientSession::sendFile(const TCHAR *file, uint32_t requestId, off64_t offset, bool allowCompression)
{
   return !isTerminated() ? SendFileOverNXCP(m_socket, requestId, file, m_encryptionContext.get(),
            offset, nullptr, nullptr, &m_mutexSocketWrite, isCompressionEnabled() && allowCompression ? NXCP_STREAM_COMPRESSION_DEFLATE : NXCP_STREAM_COMPRESSION_NONE) : false;
}

/**
 * Send server information to client
 */
void ClientSession::sendServerInfo(const NXCPMessage& request)
{
   static uint32_t protocolVersions[] = {
      CLIENT_PROTOCOL_VERSION_BASE,
      CLIENT_PROTOCOL_VERSION_ALARMS,
      CLIENT_PROTOCOL_VERSION_PUSH,
      CLIENT_PROTOCOL_VERSION_TRAP,
      CLIENT_PROTOCOL_VERSION_MOBILE,
      CLIENT_PROTOCOL_VERSION_FULL,
      CLIENT_PROTOCOL_VERSION_TCPPROXY,
      CLIENT_PROTOCOL_VERSION_SCHEDULER
   };
	TCHAR szBuffer[MAX_CONFIG_VALUE];
	String strURL;

   NXCPMessage msg(CMD_REQUEST_COMPLETED, request.getId());

   // Generate challenge for certificate authentication
#ifdef _WITH_ENCRYPTION
   RAND_bytes(m_challenge, CLIENT_CHALLENGE_SIZE);
#else
   memset(m_challenge, 0, CLIENT_CHALLENGE_SIZE);
#endif

   // Fill message with server info
   msg.setField(VID_RCC, RCC_SUCCESS);
   msg.setField(VID_SERVER_VERSION, NETXMS_VERSION_STRING);
   msg.setField(VID_SERVER_BUILD, NETXMS_BUILD_TAG);
   msg.setField(VID_SERVER_ID, g_serverId);
   msg.setField(VID_PROTOCOL_VERSION, static_cast<uint32_t>(CLIENT_PROTOCOL_VERSION_BASE));
   msg.setFieldFromInt32Array(VID_PROTOCOL_VERSION_EX, sizeof(protocolVersions) / sizeof(uint32_t), protocolVersions);
   msg.setField(VID_CHALLENGE, m_challenge, CLIENT_CHALLENGE_SIZE);
   msg.setField(VID_TIMESTAMP, (UINT32)time(nullptr));

   GetSystemTimeZone(szBuffer, sizeof(szBuffer) / sizeof(TCHAR));
   msg.setField(VID_TIMEZONE, szBuffer);
   debugPrintf(2, _T("Server time zone: %s"), szBuffer);

   ConfigReadStr(_T("Client.TileServerURL"), szBuffer, MAX_CONFIG_VALUE, _T("http://tile.netxms.org/osm/"));
   msg.setField(VID_TILE_SERVER_URL, szBuffer);

   ConfigReadStr(_T("Client.DefaultConsoleDateFormat"), szBuffer, MAX_CONFIG_VALUE, _T("dd.MM.yyyy"));
   msg.setField(VID_DATE_FORMAT, szBuffer);

   ConfigReadStr(_T("Client.DefaultConsoleTimeFormat"), szBuffer, MAX_CONFIG_VALUE, _T("HH:mm:ss"));
   msg.setField(VID_TIME_FORMAT, szBuffer);

   ConfigReadStr(_T("Client.DefaultConsoleShortTimeFormat"), szBuffer, MAX_CONFIG_VALUE, _T("HH:mm"));
   msg.setField(VID_SHORT_TIME_FORMAT, szBuffer);

   FillComponentsMessage(&msg);

   sendMessage(msg);
}

/**
 * Authenticate user by password
 */
uint32_t ClientSession::authenticateUserByPassword(const NXCPMessage& request, LoginInfo *loginInfo)
{
   request.getFieldAsString(VID_LOGIN_NAME, loginInfo->loginName, MAX_USER_NAME);
   TCHAR password[1024];
#ifdef UNICODE
   request.getFieldAsString(VID_PASSWORD, password, 256);
#else
   request.getFieldAsUtf8String(VID_PASSWORD, password, 1024);
#endif
   return AuthenticateUser(loginInfo->loginName, password, 0, nullptr, nullptr, &m_dwUserId, &m_systemAccessRights, &loginInfo->changePassword,
         &loginInfo->intruderLockout, &loginInfo->closeOtherSessions, false, &loginInfo->graceLogins);
}

/**
 * Authenticate user by certificate
 */
uint32_t ClientSession::authenticateUserByCertificate(const NXCPMessage& request, LoginInfo *loginInfo)
{
   uint32_t rcc;
#ifdef _WITH_ENCRYPTION
   request.getFieldAsString(VID_LOGIN_NAME, loginInfo->loginName, MAX_USER_NAME);
   X509 *pCert = CertificateFromLoginMessage(request);
   if (pCert != nullptr)
   {
      size_t sigLen;
      const BYTE *signature = request.getBinaryFieldPtr(VID_SIGNATURE, &sigLen);
      if (signature != nullptr)
      {
         rcc = AuthenticateUser(loginInfo->loginName, reinterpret_cast<const TCHAR*>(signature), sigLen,
               pCert, m_challenge, &m_dwUserId, &m_systemAccessRights, &loginInfo->changePassword, &loginInfo->intruderLockout,
               &loginInfo->closeOtherSessions, false, &loginInfo->graceLogins);
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
   return rcc;
}

/**
 * Authenticate user by SSO ticket
 */
uint32_t ClientSession::authenticateUserBySSOTicket(const NXCPMessage& request, LoginInfo *loginInfo)
{
   uint32_t rcc;
   char ticket[1024];
   request.getFieldAsMBString(VID_PASSWORD, ticket, 1024);
   request.getFieldAsString(VID_LOGIN_NAME, loginInfo->loginName, MAX_USER_NAME);
   if (CASAuthenticate(ticket, loginInfo->loginName))
   {
      debugPrintf(5, _T("SSO ticket %hs is valid, login name %s"), ticket, loginInfo->loginName);
      rcc = AuthenticateUser(loginInfo->loginName, nullptr, 0, nullptr, nullptr, &m_dwUserId, &m_systemAccessRights,
            &loginInfo->changePassword, &loginInfo->intruderLockout, &loginInfo->closeOtherSessions, true, &loginInfo->graceLogins);
   }
   else
   {
      debugPrintf(5, _T("SSO ticket %hs is invalid"), ticket);
      rcc = RCC_ACCESS_DENIED;
   }
   return rcc;
}

/**
 * Authenticate user by token
 */
uint32_t ClientSession::authenticateUserByToken(const NXCPMessage& request, LoginInfo *loginInfo)
{
   TCHAR buffer[64];
   request.getFieldAsString(VID_LOGIN_NAME, buffer, 64);
   UserAuthenticationToken token = UserAuthenticationToken::parse(buffer);

   uint32_t rcc;
   if (!token.isNull())
   {
      uint32_t userId;
      if (ValidateAuthenticationToken(token, &userId))
      {
         if (ResolveUserId(userId, loginInfo->loginName) != nullptr)
         {
            debugPrintf(5, _T("Authentication token is valid, login name %s"), loginInfo->loginName);
            rcc = AuthenticateUser(loginInfo->loginName, nullptr, 0, nullptr, nullptr, &m_dwUserId, &m_systemAccessRights,
                  &loginInfo->changePassword, &loginInfo->intruderLockout, &loginInfo->closeOtherSessions, true, &loginInfo->graceLogins);
         }
         else
         {
            debugPrintf(5, _T("Provided authentication token is valid but associated login is not"));
            rcc = RCC_ACCESS_DENIED;
         }
      }
      else
      {
         debugPrintf(5, _T("Provided authentication token is invalid"));
         rcc = RCC_ACCESS_DENIED;
      }
   }
   else
   {
      debugPrintf(5, _T("Provided authentication token has invalid format"));
      rcc = RCC_ACCESS_DENIED;
   }
   return rcc;
}

/**
 * Authenticate client
 */
void ClientSession::login(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

   // Get client info string
   if (request.isFieldExist(VID_CLIENT_INFO))
   {
      TCHAR clientInfo[32], osInfo[32], libVersion[16];
      request.getFieldAsString(VID_CLIENT_INFO, clientInfo, 32);
      request.getFieldAsString(VID_OS_INFO, osInfo, 32);
      request.getFieldAsString(VID_LIBNXCL_VERSION, libVersion, 16);
      _sntprintf(m_clientInfo, 96, _T("%s (%s; libnxcl %s)"), clientInfo, osInfo, libVersion);
   }

	m_clientType = request.getFieldAsUInt16(VID_CLIENT_TYPE);
	if ((m_clientType < 0) || (m_clientType > CLIENT_TYPE_APPLICATION))
		m_clientType = CLIENT_TYPE_DESKTOP;

   if (m_clientType == CLIENT_TYPE_WEB)
   {
      _tcscpy(m_webServerAddress, m_workstation);
      if (request.isFieldExist(VID_CLIENT_ADDRESS))
      {
         request.getFieldAsString(VID_CLIENT_ADDRESS, m_workstation, 64);
         debugPrintf(4, _T("Real web client address is %s"), m_workstation);
      }
   }

   if (request.isFieldExist(VID_LANGUAGE))
   {
      request.getFieldAsString(VID_LANGUAGE, m_language, 8);
   }

   if (!(m_flags & CSF_AUTHENTICATED))
   {
      uint32_t rcc;
      m_loginInfo = new LoginInfo();
      int authType = request.getFieldAsInt16(VID_AUTH_TYPE);
      debugPrintf(4, _T("Selected authentication type is %d"), authType);
		switch(authType)
		{
			case NETXMS_AUTH_TYPE_PASSWORD:
            rcc = authenticateUserByPassword(request, m_loginInfo);
				break;
			case NETXMS_AUTH_TYPE_CERTIFICATE:
            rcc = authenticateUserByCertificate(request, m_loginInfo);
				break;
         case NETXMS_AUTH_TYPE_SSO_TICKET:
            rcc = authenticateUserBySSOTicket(request, m_loginInfo);
            break;
         case NETXMS_AUTH_TYPE_TOKEN:
            rcc = authenticateUserByToken(request, m_loginInfo);
            break;
			default:
				rcc = RCC_UNSUPPORTED_AUTH_TYPE;
				break;
		}

      if (rcc == RCC_SUCCESS)
      {
         unique_ptr<StringList> methods = GetUserConfigured2FAMethods(m_dwUserId);
         if (methods->isEmpty())
         {
            debugPrintf(4, _T("Two-factor authentication is not required (not configured)"));
            rcc = finalizeLogin(request, &response);
            delete_and_null(m_loginInfo);
         }
         else
         {
            size_t tokenSize;
            const BYTE *token = request.getBinaryFieldPtr(VID_TRUSTED_DEVICE_TOKEN, &tokenSize);
            if (Validate2FATrustedDeviceToken(token, tokenSize, m_dwUserId))
            {
               debugPrintf(4, _T("Two-factor authentication is not required (valid trusted device token provided)"));
               rcc = finalizeLogin(request, &response);
               delete_and_null(m_loginInfo);
            }
            else
            {
               debugPrintf(4, _T("Two-factor authentication is required (%d methods available)"), methods->size());
               rcc = RCC_NEED_2FA;
               methods->fillMessage(&response, VID_2FA_METHOD_LIST_BASE, VID_2FA_METHOD_COUNT);
               response.setField(VID_TRUSTED_DEVICES_ALLOWED, ConfigReadULong(_T("Server.Security.2FA.TrustedDeviceTTL"), 0) > 0);
            }
         }
      }
      else
      {
         writeAuditLog(AUDIT_SECURITY, false, 0, _T("User \"%s\" login failed with error code %d (client info: %s)"), m_loginInfo->loginName, rcc, m_clientInfo);
         if (m_loginInfo->intruderLockout)
         {
            writeAuditLog(AUDIT_SECURITY, false, 0, _T("User account \"%s\" temporary disabled due to excess count of failed authentication attempts"), m_loginInfo->loginName);
         }
         m_dwUserId = INVALID_INDEX;   // reset user ID to avoid incorrect count of logged in sessions for that user
      }
      response.setField(VID_RCC, rcc);
   }
   else
   {
      response.setField(VID_RCC, RCC_OUT_OF_STATE_REQUEST);
   }

   sendMessage(response);
}

/**
 * Prepares 2FA challenge for user
 */
void ClientSession::prepare2FAChallenge(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());
   TCHAR method[MAX_OBJECT_NAME];
   request.getFieldAsString(VID_2FA_METHOD, method, MAX_OBJECT_NAME);
   delete m_loginInfo->token;
   m_loginInfo->token = Prepare2FAChallenge(method, m_dwUserId);
   if (m_loginInfo->token != nullptr)
   {
      response.setField(VID_CHALLENGE, m_loginInfo->token->getChallenge());
      response.setField(VID_QR_LABEL, m_loginInfo->token->getQRLabel());
      response.setField(VID_RCC, RCC_SUCCESS);
   }
   else
   {
      response.setField(VID_RCC, RCC_FAILED_2FA_PREPARATION);
   }
   sendMessage(response);
}

/**
 * Validating user 2FA response
 */
void ClientSession::validate2FAResponse(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());
   if (m_loginInfo != nullptr)
   {
      TCHAR userResponse[1024];
      request.getFieldAsString(VID_2FA_RESPONSE, userResponse, 1024);
      BYTE *trustedDeviceToken = nullptr;
      size_t trustedDeviceTokenSize = 0;
      if (Validate2FAResponse(m_loginInfo->token, userResponse, m_dwUserId, &trustedDeviceToken, &trustedDeviceTokenSize))
      {
         uint32_t rcc = finalizeLogin(request, &response);
         response.setField(VID_RCC, rcc);
         if ((rcc == RCC_SUCCESS) && (trustedDeviceToken != nullptr))
         {
            response.setField(VID_TRUSTED_DEVICE_TOKEN, trustedDeviceToken, trustedDeviceTokenSize);
         }
         MemFree(trustedDeviceToken);
      }
      else
      {
         response.setField(VID_RCC, RCC_FAILED_2FA_VALIDATION);
      }
      delete_and_null(m_loginInfo);
   }
   else
   {
      response.setField(VID_RCC, RCC_OUT_OF_STATE_REQUEST);
   }
   sendMessage(response);
}

/**
 * Finalize login
 */
uint32_t ClientSession::finalizeLogin(const NXCPMessage& request, NXCPMessage *response)
{
   uint32_t rcc = RCC_SUCCESS;

   // Additional validation by loaded modules
   ENUMERATE_MODULES(pfAdditionalLoginCheck)
   {
      rcc = CURRENT_MODULE.pfAdditionalLoginCheck(m_dwUserId, request);
      if (rcc != RCC_SUCCESS)
      {
         debugPrintf(4, _T("Login blocked by module %s (rcc=%u)"), CURRENT_MODULE.szName, rcc);
         break;
      }
   }

   // Execute login hook script
   ScriptVMHandle vm = CreateServerScriptVM(_T("Hook::Login"), shared_ptr<NetObj>());
   if (vm.isValid())
   {
      vm->setGlobalVariable("$session", vm->createValue(vm->createObject(&g_nxslClientSessionClass, this)));
      vm->setGlobalVariable("$user", GetUserDBObjectForNXSL(m_dwUserId, vm));
      if (vm->run())
      {
         NXSL_Value *result = vm->getResult();
         if (!result->isNull() && result->isFalse())
         {
            debugPrintf(4, _T("Login blocked by hook script"));
            rcc = RCC_ACCESS_DENIED;
         }
      }
      else
      {
         debugPrintf(4, _T("Login hook script execution error (%s)"), vm->getErrorText());
         ReportScriptError(SCRIPT_CONTEXT_LOGIN, nullptr, 0, vm->getErrorText(), _T("Hook::Login"));
      }
      vm.destroy();
   }
   else
   {
      debugPrintf(4, _T("Login hook script %s"), (vm.failureReason() == ScriptVMFailureReason::SCRIPT_IS_EMPTY) ? _T("is empty") : _T("not found"));
   }

   if (rcc == RCC_SUCCESS)
   {
      InterlockedOr(&m_flags, CSF_AUTHENTICATED);
      _tcslcpy(m_loginName, m_loginInfo->loginName, MAX_USER_NAME);
      _sntprintf(m_sessionName, MAX_SESSION_NAME, _T("%s@%s"), m_loginName, m_workstation);
      m_loginTime = time(nullptr);
      response->setField(VID_USER_SYS_RIGHTS, m_systemAccessRights);
      response->setField(VID_USER_ID, m_dwUserId);
      response->setField(VID_USER_NAME, m_loginName);
      response->setField(VID_SESSION_ID, (uint32_t)m_id);
      response->setField(VID_CHANGE_PASSWD_FLAG, (uint16_t)m_loginInfo->changePassword);
      response->setField(VID_DBCONN_STATUS, (uint16_t)((g_flags & AF_DB_CONNECTION_LOST) ? 0 : 1));
      response->setField(VID_ZONING_ENABLED, (uint16_t)((g_flags & AF_ENABLE_ZONING) ? 1 : 0));
      response->setField(VID_POLLING_INTERVAL, (int32_t)DCObject::m_defaultPollingInterval);
      response->setField(VID_RETENTION_TIME, (int32_t)DCObject::m_defaultRetentionTime);
      response->setField(VID_ALARM_STATUS_FLOW_STATE, ConfigReadBoolean(_T("Alarms.StrictStatusFlow"), false));
      response->setField(VID_TIMED_ALARM_ACK_ENABLED, ConfigReadBoolean(_T("Alarms.EnableTimedAck"), false));
      response->setField(VID_VIEW_REFRESH_INTERVAL, (uint16_t)ConfigReadInt(_T("Client.MinViewRefreshInterval"), 300));
      response->setField(VID_HELPDESK_LINK_ACTIVE, (uint16_t)((g_flags & AF_HELPDESK_LINK_ACTIVE) ? 1 : 0));
      response->setField(VID_ALARM_LIST_DISP_LIMIT, ConfigReadULong(_T("Client.AlarmList.DisplayLimit"), 4096));
      response->setField(VID_SERVER_COMMAND_TIMEOUT, ConfigReadULong(_T("Server.CommandOutputTimeout"), 60));
      response->setField(VID_GRACE_LOGINS, m_loginInfo->graceLogins);

      TCHAR tags[1024];
      ConfigReadStr(_T("Objects.ResponsibleUsers.AllowedTags"), tags, 1024, _T(""));
      response->setField(VID_RESPONSIBLE_USER_TAGS, tags);

      GetClientConfigurationHints(response);
      FillLicenseProblemsMessage(response);

      if (request.getFieldAsBoolean(VID_ENABLE_COMPRESSION))
      {
         debugPrintf(3, _T("Protocol level compression is supported by client"));
         InterlockedOr(&m_flags, CSF_COMPRESSION_ENABLED);
         response->setField(VID_ENABLE_COMPRESSION, true);
      }
      else
      {
         debugPrintf(3, _T("Protocol level compression is not supported by client"));
      }

      TCHAR buffer[MAX_DB_STRING];
      ConfigReadStr(_T("Server.Name"), buffer, MAX_DB_STRING, _T(""));
      response->setField(VID_SERVER_NAME, buffer);

      ConfigReadStr(_T("Server.Color"), buffer, MAX_DB_STRING, _T(""));
      response->setField(VID_SERVER_COLOR, buffer);

      ConfigReadStr(_T("Server.MessageOfTheDay"), buffer, MAX_DB_STRING, _T(""));
      response->setField(VID_MESSAGE_OF_THE_DAY, buffer);

      debugPrintf(3, _T("User %s authenticated (language=%s clientInfo=\"%s\")"), m_sessionName, m_language, m_clientInfo);
      writeAuditLog(AUDIT_SECURITY, true, 0, _T("User \"%s\" logged in (language: %s; client info: %s)"), m_loginInfo->loginName, m_language, m_clientInfo);

      if (m_loginInfo->closeOtherSessions)
      {
         debugPrintf(5, _T("Closing other sessions for user %s"), m_loginName);
         CloseOtherSessions(m_dwUserId, m_id);
      }
   }
   else
   {
      writeAuditLog(AUDIT_SECURITY, false, 0, _T("User \"%s\" login failed with error code %u (client info: %s)"), m_loginInfo->loginName, rcc, m_clientInfo);
      if (m_loginInfo->intruderLockout)
      {
         writeAuditLog(AUDIT_SECURITY, false, 0, _T("User account \"%s\" temporary disabled due to excess count of failed authentication attempts"), m_loginInfo->loginName);
      }
      m_dwUserId = INVALID_INDEX;   // reset user ID to avoid incorrect count of logged in sessions for that user
   }

   return rcc;
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
      postMessage(msg);
   }
}

/**
 * Send alarm categories to client
*/
void ClientSession::getAlarmCategories(const NXCPMessage& request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request.getId());
   if (checkSysAccessRights(SYSTEM_ACCESS_EPP))
   {
      GetAlarmCategories(&msg);
      msg.setField(VID_RCC, RCC_SUCCESS);
   }
   else
   {
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);
   }
   sendMessage(msg);
}

/**
* Update alarm category
*/
void ClientSession::modifyAlarmCategory(const NXCPMessage& request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request.getId());
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
void ClientSession::deleteAlarmCategory(const NXCPMessage& request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request.getId());
   if (checkSysAccessRights(SYSTEM_ACCESS_EPP))
   {
      uint32_t categoryId = request.getFieldAsInt32(VID_CATEGORY_ID);
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
 * Get configured event templates
 */
void ClientSession::getEventTemplates(const NXCPMessage& request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request.getId());
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
   sendMessage(msg);
}

/**
 * Update event template
 */
void ClientSession::modifyEventTemplate(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

   // Check access rights
   if (checkSysAccessRights(SYSTEM_ACCESS_EDIT_EVENT_DB))
   {
      json_t *oldValue, *newValue;
      uint32_t rcc = UpdateEventTemplate(request, &response, &oldValue, &newValue);
      if (rcc == RCC_SUCCESS)
      {
         TCHAR name[MAX_EVENT_NAME];
         request.getFieldAsString(VID_NAME, name, MAX_EVENT_NAME);
         writeAuditLogWithValues(AUDIT_SYSCFG, true, 0, oldValue, newValue, _T("Event template %s [%d] modified"), name, request.getFieldAsUInt32(VID_EVENT_CODE));
         json_decref(oldValue);
         json_decref(newValue);
      }
      response.setField(VID_RCC, rcc);
   }
   else
   {
      response.setField(VID_RCC, RCC_ACCESS_DENIED);
      writeAuditLog(AUDIT_SYSCFG, false, 0, _T("Access denied on modify event template [%d]"), request.getFieldAsUInt32(VID_EVENT_CODE)); // TODO change message
   }

   sendMessage(response);
}

/**
 * Delete event template
 */
void ClientSession::deleteEventTemplate(const NXCPMessage& request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request.getId());

   uint32_t eventCode = request.getFieldAsUInt32(VID_EVENT_CODE);

   // Check access rights
   if (checkSysAccessRights(SYSTEM_ACCESS_EDIT_EVENT_DB) && (eventCode >= FIRST_USER_EVENT_ID))
   {
      uint32_t rcc = DeleteEventTemplate(eventCode);
      if (rcc == RCC_SUCCESS)
			writeAuditLog(AUDIT_SYSCFG, true, 0, _T("Event template [%u] deleted"), eventCode);

      msg.setField(VID_RCC, rcc);
   }
   else
   {
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);
      writeAuditLog(AUDIT_SYSCFG, false, 0, _T("Access denied on delete event template [%u]"), eventCode);
   }

   sendMessage(msg);
}

/**
 * Generate event code for new event template
 */
void ClientSession::generateEventCode(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());
   if (checkSysAccessRights(SYSTEM_ACCESS_EDIT_EVENT_DB))
   {
      response.setField(VID_EVENT_CODE, CreateUniqueId(IDG_EVENT));
   }
   else
   {
      response.setField(VID_RCC, RCC_ACCESS_DENIED);
   }
   sendMessage(response);
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
void ClientSession::getObjects(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());
   response.setField(VID_RCC, RCC_SUCCESS);
   sendMessage(response);    // Send confirmation message
   response.deleteAllFields();

   // Change "sync comments" flag
   if (request.getFieldAsBoolean(VID_SYNC_COMMENTS))
      InterlockedOr(&m_flags, CSF_SYNC_OBJECT_COMMENTS);
   else
      InterlockedAnd(&m_flags, ~CSF_SYNC_OBJECT_COMMENTS);

   // Set sync components flag
   bool syncNodeComponents = false;
   if (request.getFieldAsBoolean(VID_SYNC_NODE_COMPONENTS))
      syncNodeComponents = true;

   // Prepare message
   response.setCode(CMD_OBJECT);

   // Send objects, one per message
   SessionObjectFilterData data;
   data.session = this;
   data.baseTimeStamp = request.getFieldAsTime(VID_TIMESTAMP);
	unique_ptr<SharedObjectArray<NetObj>> objects = g_idxObjectById.getObjects(SessionObjectFilter, &data);
	for(int i = 0; i < objects->size(); i++)
	{
      NetObj *object = objects->get(i);
	   if (!syncNodeComponents && (object->getObjectClass() == OBJECT_INTERFACE ||
	         object->getObjectClass() == OBJECT_ACCESSPOINT || object->getObjectClass() == OBJECT_VPNCONNECTOR ||
	         object->getObjectClass() == OBJECT_NETWORKSERVICE))
	   {
         continue;
	   }

      object->fillMessage(&response, m_dwUserId);
      if (m_flags & CSF_SYNC_OBJECT_COMMENTS)
         object->commentsToMessage(&response);
      if ((object->getObjectClass() == OBJECT_NODE) && !object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_MODIFY))
      {
         // mask passwords
         response.setField(VID_SHARED_SECRET, _T("********"));
         response.setField(VID_SNMP_AUTH_OBJECT, _T("********"));
         response.setField(VID_SNMP_AUTH_PASSWORD, _T("********"));
         response.setField(VID_SNMP_PRIV_PASSWORD, _T("********"));
         response.setField(VID_SSH_PASSWORD, _T("********"));
      }
      sendMessage(response);
      response.deleteAllFields();
	}

   // Send end of list notification
   response.setCode(CMD_OBJECT_LIST_END);
   sendMessage(&response);

   InterlockedOr(&m_flags, CSF_OBJECT_SYNC_FINISHED);
}

/**
 * Send selected objects to client
 */
void ClientSession::getSelectedObjects(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());
   response.setField(VID_RCC, RCC_SUCCESS);
   sendMessage(response);    // Send confirmation message
   response.deleteAllFields();

   // Change "sync comments" flag
   if (request.getFieldAsBoolean(VID_SYNC_COMMENTS))
      InterlockedOr(&m_flags, CSF_SYNC_OBJECT_COMMENTS);
   else
      InterlockedAnd(&m_flags, ~CSF_SYNC_OBJECT_COMMENTS);

   time_t timestamp = request.getFieldAsTime(VID_TIMESTAMP);
	IntegerArray<uint32_t> objects;
	request.getFieldAsInt32Array(VID_OBJECT_LIST, &objects);
	uint32_t options = request.getFieldAsUInt16(VID_FLAGS);

   // Prepare message
	response.setCode((options & OBJECT_SYNC_SEND_UPDATES) ? CMD_OBJECT_UPDATE : CMD_OBJECT);

   // Send objects, one per message
   for(int i = 0; i < objects.size(); i++)
	{
		shared_ptr<NetObj> object = FindObjectById(objects.get(i));
      if ((object != nullptr) &&
		    object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ) &&
          (object->getTimeStamp() >= timestamp) &&
          !object->isHidden() && !object->isSystem())
      {
         object->fillMessage(&response, m_dwUserId);
         if (m_flags & CSF_SYNC_OBJECT_COMMENTS)
            object->commentsToMessage(&response);
         if ((object->getObjectClass() == OBJECT_NODE) && !object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_MODIFY))
         {
            // mask passwords
            response.setField(VID_SHARED_SECRET, _T("********"));
            response.setField(VID_SNMP_AUTH_OBJECT, _T("********"));
            response.setField(VID_SNMP_AUTH_PASSWORD, _T("********"));
            response.setField(VID_SNMP_PRIV_PASSWORD, _T("********"));
            response.setField(VID_SSH_PASSWORD, _T("********"));
         }
         sendMessage(response);
         response.deleteAllFields();
      }
	}

   InterlockedOr(&m_flags, CSF_OBJECT_SYNC_FINISHED);

	if (options & OBJECT_SYNC_DUAL_CONFIRM)
	{
		response.setCode(CMD_REQUEST_COMPLETED);
		response.setField(VID_RCC, RCC_SUCCESS);
      sendMessage(&response);
	}
}

/**
 * Query objects
 */
void ClientSession::queryObjects(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

   TCHAR *query = request.getFieldAsString(VID_QUERY);
   TCHAR errorMessage[1024];
   unique_ptr<ObjectArray<ObjectQueryResult>> objects = QueryObjects(query, m_dwUserId, errorMessage, 1024);
   if (objects != nullptr)
   {
      uint32_t *idList = static_cast<uint32_t*>(MemAllocLocal(objects->size() * sizeof(uint32_t)));
      for(int i = 0; i < objects->size(); i++)
      {
         idList[i] = objects->get(i)->object->getId();
      }
      response.setFieldFromInt32Array(VID_OBJECT_LIST, objects->size(), idList);
      MemFreeLocal(idList);
   }
   else
   {
      response.setField(VID_RCC, RCC_NXSL_EXECUTION_ERROR);
      response.setField(VID_ERROR_TEXT, errorMessage);
   }
   MemFree(query);

   sendMessage(&response);
}

/**
 * Query object details
 */
void ClientSession::queryObjectDetails(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

   TCHAR *query = request.getFieldAsString(VID_QUERY);
   StringList fields(request, VID_FIELD_LIST_BASE, VID_FIELDS);
   StringList orderBy(request, VID_ORDER_FIELD_LIST_BASE, VID_ORDER_FIELDS);
   StringMap inputFields(request, VID_INPUT_FIELD_BASE, VID_INPUT_FIELD_COUNT);
   TCHAR errorMessage[1024];
   unique_ptr<ObjectArray<ObjectQueryResult>> objects = QueryObjects(query, m_dwUserId, errorMessage, 1024,
            request.getFieldAsBoolean(VID_READ_ALL_FIELDS), &fields, &orderBy, &inputFields, request.getFieldAsUInt32(VID_RECORD_LIMIT));
   if (objects != nullptr)
   {
      uint32_t *idList = static_cast<uint32_t*>MemAllocLocal(objects->size() * sizeof(uint32_t));
      uint32_t fieldId = VID_ELEMENT_LIST_BASE;
      for(int i = 0; i < objects->size(); i++)
      {
         ObjectQueryResult *curr = objects->get(i);
         idList[i] = curr->object->getId();
         curr->values->fillMessage(&response, fieldId + 1, fieldId);
         fieldId += curr->values->size() * 2 + 1;
      }
      response.setFieldFromInt32Array(VID_OBJECT_LIST, objects->size(), idList);
      MemFreeLocal(idList);
   }
   else
   {
      response.setField(VID_RCC, RCC_NXSL_EXECUTION_ERROR);
      response.setField(VID_ERROR_TEXT, errorMessage);
   }
   MemFree(query);

   sendMessage(response);
}

/**
 * Get object queries
 */
void ClientSession::getObjectQueries(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());
   response.setField(VID_RCC, GetObjectQueries(&response));
   sendMessage(response);
}

/**
 * Create or modify object query
 */
void ClientSession::modifyObjectQuery(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());
   if (m_systemAccessRights & SYSTEM_ACCESS_MANAGE_OBJECT_QUERIES)
   {
      uint32_t queryId;
      uint32_t rcc = ModifyObjectQuery(request, &queryId);
      response.setField(VID_RCC, rcc);
      if (rcc == RCC_SUCCESS)
      {
         response.setField(VID_QUERY_ID, queryId);
         writeAuditLog(AUDIT_SYSCFG, true, 0, _T("Object query [%u] modified"), queryId);
      }
   }
   else
   {
      response.setField(VID_RCC, RCC_ACCESS_DENIED);
      writeAuditLog(AUDIT_SYSCFG, false, 0, _T("Access denied on modify object query"));
   }
   sendMessage(response);
}

/**
 * Delete object query
 */
void ClientSession::deleteObjectQuery(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());
   if (m_systemAccessRights & SYSTEM_ACCESS_MANAGE_OBJECT_QUERIES)
   {
      uint32_t queryId = request.getFieldAsUInt32(VID_QUERY_ID);
      uint32_t rcc = DeleteObjectQuery(queryId);
      response.setField(VID_RCC, rcc);
      if (rcc == RCC_SUCCESS)
         writeAuditLog(AUDIT_SYSCFG, true, 0, _T("Object query [%u] deleted"), queryId);
   }
   else
   {
      response.setField(VID_RCC, RCC_ACCESS_DENIED);
      writeAuditLog(AUDIT_SYSCFG, false, 0, _T("Access denied on delete object query"));
   }
   sendMessage(response);
}

/**
 * Send all configuration variables to client
 */
void ClientSession::getConfigurationVariables(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

   // Check user rights
   if ((m_dwUserId == 0) || (m_systemAccessRights & SYSTEM_ACCESS_SERVER_CONFIG))
   {
      DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

      // Retrieve configuration variables from database
      DB_RESULT hResult = DBSelect(hdb, _T("SELECT var_name,var_value,need_server_restart,data_type,description,default_value,units FROM config WHERE is_visible=1"));
      if (hResult != nullptr)
      {
         // Send events, one per message
         int count = DBGetNumRows(hResult);
         response.setField(VID_NUM_VARIABLES, static_cast<uint32_t>(count));
         uint32_t fieldId = VID_VARLIST_BASE;
         TCHAR buffer[MAX_CONFIG_VALUE];
         for(int i = 0; i < count; i++, fieldId += 10)
         {
            response.setField(fieldId, DBGetField(hResult, i, 0, buffer, MAX_DB_STRING));
            response.setField(fieldId + 1, DBGetField(hResult, i, 1, buffer, MAX_CONFIG_VALUE));
            response.setField(fieldId + 2, (WORD)DBGetFieldLong(hResult, i, 2));
            response.setField(fieldId + 3, DBGetField(hResult, i, 3, buffer, MAX_CONFIG_VALUE));
            response.setField(fieldId + 4, DBGetField(hResult, i, 4, buffer, MAX_CONFIG_VALUE));
            response.setField(fieldId + 5, DBGetField(hResult, i, 5, buffer, MAX_CONFIG_VALUE));
            response.setField(fieldId + 6, DBGetField(hResult, i, 6, buffer, MAX_CONFIG_VALUE));
         }
         DBFreeResult(hResult);

         hResult = DBSelect(hdb, _T("SELECT var_name,var_value,var_description FROM config_values"));
         if (hResult != nullptr)
         {
            count = DBGetNumRows(hResult);
            response.setField(VID_NUM_VALUES, count);
            for(int i = 0; i < count; i++)
            {
               response.setField(fieldId++, DBGetField(hResult, i, 0, buffer, MAX_DB_STRING));
               response.setField(fieldId++, DBGetField(hResult, i, 1, buffer, MAX_CONFIG_VALUE));
               response.setField(fieldId++, DBGetField(hResult, i, 2, buffer, MAX_DB_STRING));
            }
            DBFreeResult(hResult);

            response.setField(VID_RCC, RCC_SUCCESS);
         }
         else
         {
            response.setField(VID_RCC, RCC_DB_FAILURE);
         }
      }
      else
      {
         response.setField(VID_RCC, RCC_DB_FAILURE);
      }
      DBConnectionPoolReleaseConnection(hdb);
   }
   else
   {
      response.setField(VID_RCC, RCC_ACCESS_DENIED);
   }

   sendMessage(response);
}

/**
 * Get public configuration variable by name
 */
void ClientSession::getPublicConfigurationVariable(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT var_value FROM config WHERE var_name=? AND is_public='Y'"));
   if (hStmt != nullptr)
   {
      TCHAR name[64];
      request.getFieldAsString(VID_NAME, name, 64);
      DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, name, DB_BIND_STATIC);

      DB_RESULT hResult = DBSelectPrepared(hStmt);
      if (hResult != nullptr)
      {
         if (DBGetNumRows(hResult) > 0)
         {
            TCHAR value[MAX_CONFIG_VALUE];
            response.setField(VID_VALUE, DBGetField(hResult, 0, 0, value, MAX_CONFIG_VALUE));
            response.setField(VID_RCC, RCC_SUCCESS);
         }
         else
         {
            response.setField(VID_RCC, RCC_UNKNOWN_CONFIG_VARIABLE);
         }
         DBFreeResult(hResult);
      }
      else
      {
         response.setField(VID_RCC, RCC_DB_FAILURE);
      }
      DBFreeStatement(hStmt);
   }
   else
   {
      response.setField(VID_RCC, RCC_DB_FAILURE);
   }

   DBConnectionPoolReleaseConnection(hdb);

   sendMessage(response);
}

/**
 * Set configuration variable's value
 */
void ClientSession::setConfigurationVariable(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

   TCHAR name[MAX_OBJECT_NAME];
   request.getFieldAsString(VID_NAME, name, MAX_OBJECT_NAME);

   if ((m_dwUserId == 0) || (m_systemAccessRights & SYSTEM_ACCESS_SERVER_CONFIG))
   {
      TCHAR oldValue[MAX_CONFIG_VALUE], newValue[MAX_CONFIG_VALUE];
      request.getFieldAsString(VID_VALUE, newValue, MAX_CONFIG_VALUE);
      ConfigReadStr(name, oldValue, MAX_CONFIG_VALUE, _T(""));
      if (ConfigWriteStr(name, newValue, true))
      {
         response.setField(VID_RCC, RCC_SUCCESS);
         writeAuditLogWithValues(AUDIT_SYSCFG, true, 0, oldValue, newValue, 'T',
                                 _T("Server configuration variable \"%s\" changed from \"%s\" to \"%s\""), name, oldValue, newValue);
      }
      else
      {
         response.setField(VID_RCC, RCC_DB_FAILURE);
      }
   }
   else
   {
      writeAuditLog(AUDIT_SYSCFG, false, 0, _T("Access denied on setting server configuration variable \"%s\""), name);
      response.setField(VID_RCC, RCC_ACCESS_DENIED);
   }

   sendMessage(response);
}

/**
 * Set configuration variable's values to default
 */
void ClientSession::setDefaultConfigurationVariableValues(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());
   if (checkSysAccessRights(SYSTEM_ACCESS_SERVER_CONFIG))
   {
      DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
      DB_STATEMENT stmt = DBPrepare(hdb, _T("SELECT default_value FROM config WHERE var_name=?"));
      if (stmt != nullptr)
      {
         int numVars = request.getFieldAsInt32(VID_NUM_VARIABLES);
         UINT32 fieldId = VID_VARLIST_BASE;
         TCHAR varName[64], defValue[MAX_CONFIG_VALUE];
         for(int i = 0; i < numVars; i++)
         {
            request.getFieldAsString(fieldId++, varName, 64);
            DBBind(stmt, 1, DB_SQLTYPE_VARCHAR, varName, DB_BIND_STATIC);

            DB_RESULT result = DBSelectPrepared(stmt);
            if (result != nullptr)
            {
               DBGetField(result, 0, 0, defValue, MAX_CONFIG_VALUE);
               ConfigWriteStr(varName, defValue, false);
               response.setField(VID_RCC, RCC_SUCCESS);
               DBFreeResult(result);
            }
            else
            {
               response.setField(VID_RCC, RCC_DB_FAILURE);
               break;
            }
         }
         DBFreeStatement(stmt);
      }
      else
      {
         response.setField(VID_RCC, RCC_DB_FAILURE);
      }
      DBConnectionPoolReleaseConnection(hdb);
   }
   else
   {
      writeAuditLog(AUDIT_SYSCFG, false, 0, _T("Access denied for setting server configuration variables to default"));
      response.setField(VID_RCC, RCC_ACCESS_DENIED);
   }
   sendMessage(&response);

}

/**
 * Delete configuration variable
 */
void ClientSession::deleteConfigurationVariable(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

   TCHAR name[MAX_OBJECT_NAME];
   request.getFieldAsString(VID_NAME, name, MAX_OBJECT_NAME);

   if ((m_dwUserId == 0) || (m_systemAccessRights & SYSTEM_ACCESS_SERVER_CONFIG))
   {
      if (ConfigDelete(name))
		{
         response.setField(VID_RCC, RCC_SUCCESS);
			writeAuditLog(AUDIT_SYSCFG, true, 0, _T("Server configuration variable \"%s\" deleted"), name);
		}
      else
		{
         response.setField(VID_RCC, RCC_DB_FAILURE);
		}
   }
   else
   {
      writeAuditLog(AUDIT_SYSCFG, false, 0, _T("Access denied on delete server configuration variable \"%s\""), name);
      response.setField(VID_RCC, RCC_ACCESS_DENIED);
   }

   sendMessage(response);
}

/**
 * Set configuration clob
 */
void ClientSession::setConfigCLOB(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

   TCHAR name[MAX_OBJECT_NAME];
   request.getFieldAsString(VID_NAME, name, MAX_OBJECT_NAME);
	if (m_systemAccessRights & SYSTEM_ACCESS_SERVER_CONFIG)
	{
		TCHAR *newValue = request.getFieldAsString(VID_VALUE);
		if (newValue != nullptr)
		{
		   TCHAR *oldValue = ConfigReadCLOB(name, _T(""));
			if (ConfigWriteCLOB(name, newValue, TRUE))
			{
				response.setField(VID_RCC, RCC_SUCCESS);
				writeAuditLogWithValues(AUDIT_SYSCFG, true, 0, oldValue, newValue, 'T',
								            _T("Server configuration variable (long) \"%s\" changed"), name);
			}
			else
			{
				response.setField(VID_RCC, RCC_DB_FAILURE);
			}
			MemFree(oldValue);
			MemFree(newValue);
		}
		else
		{
			response.setField(VID_RCC, RCC_INVALID_REQUEST);
		}
	}
	else
	{
	   writeAuditLog(AUDIT_SYSCFG, false, 0, _T("Access denied on setting server configuration variable \"%s\""), name);
		response.setField(VID_RCC, RCC_ACCESS_DENIED);
	}

	sendMessage(response);
}

/**
 * Get value of configuration clob
 */
void ClientSession::getConfigCLOB(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

	if (m_systemAccessRights & SYSTEM_ACCESS_SERVER_CONFIG)
	{
	   TCHAR name[MAX_OBJECT_NAME];
      request.getFieldAsString(VID_NAME, name, MAX_OBJECT_NAME);
		TCHAR *value = ConfigReadCLOB(name, nullptr);
		if (value != nullptr)
		{
			response.setField(VID_VALUE, value);
			response.setField(VID_RCC, RCC_SUCCESS);
			MemFree(value);
		}
		else
		{
			response.setField(VID_RCC, RCC_UNKNOWN_VARIABLE);
		}
	}
	else
	{
		response.setField(VID_RCC, RCC_ACCESS_DENIED);
	}

	sendMessage(response);
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
 * Send pending object updates
 */
void ClientSession::sendObjectUpdates()
{
   if ((m_flags & (CSF_TERMINATE_REQUESTED | CSF_TERMINATED)) != 0)
   {
      decRefCount();
      return;
   }

   uint32_t *idList = reinterpret_cast<uint32_t*>(alloca(m_objectNotificationBatchSize * sizeof(uint32_t)));
   size_t count = 0;

   m_pendingObjectNotificationsLock.lock();
   auto it = m_pendingObjectNotifications->begin();
   while(it.hasNext() && (count < m_objectNotificationBatchSize))
   {
      idList[count++] = *it.next();
      it.remove();
   }
   m_pendingObjectNotificationsLock.unlock();

   int64_t startTime = GetCurrentTimeMs();

   NXCPMessage response(CMD_OBJECT_UPDATE, 0);
   for(size_t i = 0; i < count; i++)
   {
      response.deleteAllFields();
      shared_ptr<NetObj> object = FindObjectById(idList[i]);
      if ((object != nullptr) && !object->isDeleted())
      {
         object->fillMessage(&response, m_dwUserId);
         if (m_flags & CSF_SYNC_OBJECT_COMMENTS)
            object->commentsToMessage(&response);
         if ((object->getObjectClass() == OBJECT_NODE) && !object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_MODIFY))
         {
            // mask passwords
            response.setField(VID_SHARED_SECRET, _T("********"));
            response.setField(VID_SNMP_AUTH_OBJECT, _T("********"));
            response.setField(VID_SNMP_AUTH_PASSWORD, _T("********"));
            response.setField(VID_SNMP_PRIV_PASSWORD, _T("********"));
            response.setField(VID_SSH_PASSWORD, _T("********"));
         }
      }
      else
      {
         response.setField(VID_OBJECT_ID, idList[i]);
         response.setField(VID_IS_DELETED, true);
      }
      sendMessage(response);
   }

   uint32_t elapsedTime = static_cast<uint32_t>(GetCurrentTimeMs() - startTime);
   if ((elapsedTime > 500) && ((m_objectNotificationBatchSize > 100) || (m_objectNotificationDelay < 1000)))
   {
      if (m_objectNotificationBatchSize > 100)
         m_objectNotificationBatchSize -= 100;
      if (m_objectNotificationDelay < 1000)
         m_objectNotificationDelay += 100;
      debugPrintf(4, _T("Object notification settings changed: batchSize=%u delay=%u"), static_cast<uint32_t>(m_objectNotificationBatchSize), m_objectNotificationDelay);
   }
   else if ((elapsedTime < 500) && ((m_objectNotificationBatchSize < 1000) || (m_objectNotificationDelay > 200)))
   {
      if (m_objectNotificationBatchSize < 1000)
         m_objectNotificationBatchSize += 1000;
      if (m_objectNotificationDelay > 200)
         m_objectNotificationDelay -= 100;
      debugPrintf(4, _T("Object notification settings changed: batchSize=%u delay=%u"), static_cast<uint32_t>(m_objectNotificationBatchSize), m_objectNotificationDelay);
   }

   if (count == m_objectNotificationBatchSize)
   {
      if ((m_flags & CSF_OBJECTS_OUT_OF_SYNC) == 0)
      {
         InterlockedOr(&m_flags, CSF_OBJECTS_OUT_OF_SYNC);
         notify(NX_NOTIFY_OBJECTS_OUT_OF_SYNC);
         debugPrintf(4, _T("Objects out of sync flag set"));
      }
   }
   else
   {
      if ((m_flags & CSF_OBJECTS_OUT_OF_SYNC) != 0)
      {
         InterlockedAnd(&m_flags, ~CSF_OBJECTS_OUT_OF_SYNC);
         notify(NX_NOTIFY_OBJECTS_IN_SYNC);
         debugPrintf(4, _T("Objects out of sync flag cleared"));
      }
   }

   m_pendingObjectNotificationsLock.lock();
   if ((m_pendingObjectNotifications->size() > 0) && ((m_flags & (CSF_TERMINATE_REQUESTED | CSF_TERMINATED)) == 0))
   {
      ThreadPoolScheduleRelative(g_clientThreadPool, m_objectNotificationDelay, this, &ClientSession::sendObjectUpdates);
   }
   else
   {
      m_objectNotificationScheduled = false;
      decRefCount();
   }
   m_pendingObjectNotificationsLock.unlock();
}

/**
 * Handler for object changes
 */
void ClientSession::onObjectChange(const shared_ptr<NetObj>& object)
{
   if (((m_flags & CSF_OBJECT_SYNC_FINISHED) > 0) && isAuthenticated() && isSubscribedTo(NXC_CHANNEL_OBJECTS) &&
      (object->isDeleted() || object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ)))
   {
      m_pendingObjectNotificationsLock.lock();
      m_pendingObjectNotifications->put(object->getId());
      if (!m_objectNotificationScheduled)
      {
         m_objectNotificationScheduled = true;
         incRefCount();
         ThreadPoolScheduleRelative(g_clientThreadPool, m_objectNotificationDelay, this, &ClientSession::sendObjectUpdates);
      }
      m_pendingObjectNotificationsLock.unlock();
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
void ClientSession::modifyObject(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

   uint32_t rcc = RCC_SUCCESS;
   uint32_t dwObjectId = request.getFieldAsUInt32(VID_OBJECT_ID);
   shared_ptr<NetObj> object = FindObjectById(dwObjectId);
   if (object != nullptr)
   {
      if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_MODIFY))
      {
         json_t *oldValue = object->toJson();

         // If user attempts to change object's ACL, check
         // if he has OBJECT_ACCESS_ACL permission
         if (request.isFieldExist(VID_ACL_SIZE))
            if (!object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_ACL))
               rcc = RCC_ACCESS_DENIED;

         // If allowed, change object and set completion code
         if (rcc == RCC_SUCCESS)
			{
            rcc = object->modifyFromMessage(request);
	         if (rcc == RCC_SUCCESS)
				{
					object->postModify();
				}
				else if (rcc == RCC_IP_ADDRESS_CONFLICT)
				{
               // Add information about conflicting nodes
               InetAddress ipAddr;
               if (request.isFieldExist(VID_IP_ADDRESS))
               {
                  ipAddr = request.getFieldAsInetAddress(VID_IP_ADDRESS);
               }
               else if (request.isFieldExist(VID_PRIMARY_NAME))
               {
                  TCHAR primaryName[MAX_DNS_NAME];
                  request.getFieldAsString(VID_PRIMARY_NAME, primaryName, MAX_DNS_NAME);
                  ipAddr = InetAddress::resolveHostName(primaryName);
               }
               SetNodesConflictString(&response, static_cast<Node&>(*object).getZoneUIN(), ipAddr);
            }
			}
         response.setField(VID_RCC, rcc);

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
         response.setField(VID_RCC, RCC_ACCESS_DENIED);
			writeAuditLog(AUDIT_OBJECTS, false, dwObjectId, _T("Access denied on object modification"));
      }
   }
   else
   {
      response.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   // Send response
   sendMessage(response);
}

/**
 * Send users database to client
 */
void ClientSession::sendUserDB(const NXCPMessage& request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request.getId());
   msg.setField(VID_RCC, RCC_SUCCESS);
   sendMessage(msg);
	msg.deleteAllFields();

   // Send user database
	Iterator<UserDatabaseObject> users = OpenUserDatabase();
	while(users.hasNext())
   {
	   UserDatabaseObject *object = users.next();
		msg.setCode(object->isGroup() ? CMD_GROUP_DATA : CMD_USER_DATA);
		object->fillMessage(&msg);
      sendMessage(msg);
      msg.deleteAllFields();
   }
	CloseUserDatabase();

   // Send end-of-database notification
   msg.setCode(CMD_USER_DB_EOF);
   sendMessage(msg);
}

/**
 * Get users from given set
 */
void ClientSession::getSelectedUsers(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());
   response.setField(VID_RCC, RCC_SUCCESS);
   sendMessage(response);
   response.deleteAllFields();

   IntegerArray<uint32_t> userIds;
   request.getFieldAsInt32Array(VID_OBJECT_LIST, &userIds);

   // Prepare message
   response.setCode(CMD_USER_DATA);

   // Send user database
   unique_ptr<ObjectArray<UserDatabaseObject>> users = FindUserDBObjects(userIds);
   for (int i = 0; i < users->size(); i++)
   {
      UserDatabaseObject *object = users->get(i);
      response.setCode(object->isGroup() ? CMD_GROUP_DATA : CMD_USER_DATA);
      object->fillMessage(&response);
      sendMessage(response);
      response.deleteAllFields();
   }

   response.setCode(CMD_REQUEST_COMPLETED);
   response.setField(VID_RCC, RCC_SUCCESS);
   sendMessage(response);
}

/**
 * Create new user
 */
void ClientSession::createUser(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

   // Check user rights
   if (m_systemAccessRights & SYSTEM_ACCESS_MANAGE_USERS)
   {
      TCHAR userName[MAX_USER_NAME];
      request.getFieldAsString(VID_USER_NAME, userName, MAX_USER_NAME);
      if (IsValidObjectName(userName))
      {
         bool isGroup = request.getFieldAsBoolean(VID_IS_GROUP);
         uint32_t userId;
         uint32_t rcc = CreateNewUser(userName, isGroup, &userId);
         response.setField(VID_RCC, rcc);
         if (rcc == RCC_SUCCESS)
         {
            response.setField(VID_USER_ID, userId);   // Send id of new user to client
            writeAuditLog(AUDIT_SECURITY, true, 0, _T("%s %s created"), isGroup ? _T("Group") : _T("User"), userName);
         }
      }
      else
      {
         response.setField(VID_RCC, RCC_INVALID_OBJECT_NAME);
      }
   }
   else
   {
      response.setField(VID_RCC, RCC_ACCESS_DENIED);
   }

   sendMessage(response);
}

/**
 * Update existing user's data
 */
void ClientSession::updateUser(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

   if (m_systemAccessRights & SYSTEM_ACCESS_MANAGE_USERS)
   {
      json_t *oldData = nullptr, *newData = nullptr;
      uint32_t result = ModifyUserDatabaseObject(request, &oldData, &newData);
      if (result == RCC_SUCCESS)
      {
         TCHAR name[MAX_USER_NAME];
         uint32_t id = request.getFieldAsUInt32(VID_USER_ID);
         writeAuditLogWithValues(AUDIT_SECURITY, true, 0, oldData, newData,
               _T("%s %s modified"), (id & GROUP_FLAG) ? _T("Group") : _T("User"), ResolveUserId(id, name, true));
      }
      response.setField(VID_RCC, result);
      json_decref(oldData);
      json_decref(newData);
   }
   else
   {
      response.setField(VID_RCC, RCC_ACCESS_DENIED);
   }

   sendMessage(response);
}

/**
 * Detach LDAP user
 */
void ClientSession::detachLdapUser(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());
   uint32_t id = request.getFieldAsUInt32(VID_USER_ID);

   if (m_systemAccessRights & SYSTEM_ACCESS_MANAGE_USERS)
   {
      uint32_t rcc = DetachLDAPUser(id);
      if (rcc == RCC_SUCCESS)
      {
         TCHAR name[MAX_DB_STRING];
         writeAuditLog(AUDIT_SECURITY, true, 0,
               _T("%s %s detached from LDAP account"), (id & GROUP_FLAG) ? _T("Group") : _T("User"), ResolveUserId(id, name, true));
      }
      response.setField(VID_RCC, rcc);
   }
   else
   {
      response.setField(VID_RCC, RCC_ACCESS_DENIED);
   }

   sendMessage(response);
}

/**
 * Delete user
 */
void ClientSession::deleteUser(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

   // Check user rights
   if (m_systemAccessRights & SYSTEM_ACCESS_MANAGE_USERS)
   {
      uint32_t userId = request.getFieldAsUInt32(VID_USER_ID);
      if ((userId != 0) && (userId != GROUP_EVERYONE))
      {
         if (!IsLoggedIn(userId))
         {
            TCHAR name[MAX_USER_NAME];
            ResolveUserId(userId, name, true);
            UINT32 rcc =  DeleteUserDatabaseObject(userId);
            response.setField(VID_RCC, rcc);
            if (rcc == RCC_SUCCESS)
            {
               writeAuditLog(AUDIT_SECURITY, true, 0, _T("%s %s [%d] deleted"), (userId & GROUP_FLAG) ? _T("Group") : _T("User"), name, userId);
            }
         }
         else
         {
            // logger in users cannot be deleted
            response.setField(VID_RCC, RCC_USER_LOGGED_IN);
         }
      }
      else
      {
         // System administrator account and everyone group cannot be deleted
         response.setField(VID_RCC, RCC_ACCESS_DENIED);
      }
   }
   else
   {
      response.setField(VID_RCC, RCC_ACCESS_DENIED);

      uint32_t userId = request.getFieldAsUInt32(VID_USER_ID);
      TCHAR name[MAX_USER_NAME];
      ResolveUserId(userId, name, true);
      writeAuditLog(AUDIT_SECURITY, false, 0, _T("Access denied on delete %s %s [%d]"), (userId & GROUP_FLAG) ? _T("group") : _T("user"), name, userId);
   }

   sendMessage(response);
}

/**
 * Change management status for the object
 */
void ClientSession::changeObjectMgmtStatus(const NXCPMessage& request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request.getId());

   // Get object id and check access rights
   uint32_t dwObjectId = request.getFieldAsUInt32(VID_OBJECT_ID);
   shared_ptr<NetObj> object = FindObjectById(dwObjectId);
   if (object != nullptr)
   {
      if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_MODIFY))
      {
			if ((object->getObjectClass() != OBJECT_TEMPLATE) &&
				 (object->getObjectClass() != OBJECT_TEMPLATEGROUP) &&
				 (object->getObjectClass() != OBJECT_TEMPLATEROOT))
			{
				bool bIsManaged = request.getFieldAsBoolean(VID_MGMT_STATUS);
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
   sendMessage(msg);
}

/**
 * Enter maintenance mode for object
 */
void ClientSession::enterMaintenanceMode(const NXCPMessage& request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request.getId());

   // Get object id and check access rights
   shared_ptr<NetObj> object = FindObjectById(request.getFieldAsUInt32(VID_OBJECT_ID));
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
            TCHAR *comments = request.getFieldAsString(VID_COMMENTS);
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

   sendMessage(msg);
}

/**
 * Leave maintenance mode for object
 */
void ClientSession::leaveMaintenanceMode(const NXCPMessage& request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request.getId());

   // Get object id and check access rights
   shared_ptr<NetObj> object = FindObjectById(request.getFieldAsUInt32(VID_OBJECT_ID));
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

   sendMessage(msg);
}

/**
 * Validate password for currently logged in user
 */
void ClientSession::validatePassword(const NXCPMessage& request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request.getId());

#ifdef UNICODE
   TCHAR password[256];
   request.getFieldAsString(VID_PASSWORD, password, 256);
#else
   char password[256];
   request.getFieldAsUtf8String(VID_PASSWORD, password, 256);
#endif

   bool isValid = false;
   msg.setField(VID_RCC, ValidateUserPassword(m_dwUserId, m_loginName, password, &isValid));
   msg.setField(VID_PASSWORD_IS_VALID, (INT16)(isValid ? 1 : 0));

   sendMessage(msg);
}

/**
 * Set user's password
 */
void ClientSession::setPassword(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

   uint32_t userId = request.getFieldAsUInt32(VID_USER_ID);
   if ((m_systemAccessRights & SYSTEM_ACCESS_MANAGE_USERS) || (userId == m_dwUserId))     // User can change password for itself
   {
      TCHAR newPassword[1024], oldPassword[1024];
#ifdef UNICODE
      request.getFieldAsString(VID_PASSWORD, newPassword, 256);
		if (request.isFieldExist(VID_OLD_PASSWORD))
			request.getFieldAsString(VID_OLD_PASSWORD, oldPassword, 256);
#else
      request.getFieldAsUtf8String(VID_PASSWORD, newPassword, 1024);
		if (request.isFieldExist(VID_OLD_PASSWORD))
			request.getFieldAsUtf8String(VID_OLD_PASSWORD, oldPassword, 1024);
#endif
		else
			oldPassword[0] = 0;

      uint32_t rcc = SetUserPassword(userId, newPassword, oldPassword, userId == m_dwUserId);
      response.setField(VID_RCC, rcc);

      if (rcc == RCC_SUCCESS)
      {
         TCHAR userName[MAX_USER_NAME];
         WriteAuditLog(AUDIT_SECURITY, TRUE, m_dwUserId, m_workstation, m_id, 0, _T("Changed password for user %s"), ResolveUserId(userId, userName, true));
      }
   }
   else
   {
      // Current user has no rights to change password for specific user
      response.setField(VID_RCC, RCC_ACCESS_DENIED);
   }

   // Send response
   sendMessage(response);
}

/**
 * Send node's DCIs to client and lock data collection settings
 */
void ClientSession::openNodeDCIList(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());
   bool success = false;

   // Get node id and check object class and access rights
   uint32_t objectId = request.getFieldAsUInt32(VID_OBJECT_ID);
   shared_ptr<NetObj> object = FindObjectById(objectId);
   if (object != nullptr)
   {
      if (object->isDataCollectionTarget() || (object->getObjectClass() == OBJECT_TEMPLATE))
      {
         if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
         {
            success = true;
            response.setField(VID_RCC, RCC_SUCCESS);
            if (!request.getFieldAsBoolean(VID_IS_REFRESH))
            {
               m_openDataCollectionConfigurations.put(objectId);
            }
         }
         else
         {
            response.setField(VID_RCC, RCC_ACCESS_DENIED);
         }
      }
      else
      {
         response.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
      }
   }
   else
   {
      response.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   sendMessage(response);

   // If DCI list was successfully locked, send it to client
   if (success)
      static_cast<DataCollectionOwner&>(*object).sendItemsToClient(this, request.getId());
}

/**
 * Unlock node's data collection settings
 */
void ClientSession::closeNodeDCIList(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

   // Get node id and check object class and access rights
   uint32_t objectId = request.getFieldAsUInt32(VID_OBJECT_ID);
   shared_ptr<NetObj> object = FindObjectById(objectId);
   if (object != nullptr)
   {
      if (object->isDataCollectionTarget() || (object->getObjectClass() == OBJECT_TEMPLATE))
      {
         if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
         {
            static_cast<DataCollectionOwner&>(*object).applyDCIChanges(false);
            if (!request.getFieldAsBoolean(VID_COMMIT_ONLY))
               m_openDataCollectionConfigurations.remove(objectId);
            response.setField(VID_RCC, RCC_SUCCESS);
         }
         else
         {
            response.setField(VID_RCC, RCC_ACCESS_DENIED);
         }
      }
      else
      {
         response.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
      }
   }
   else
   {
      response.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   sendMessage(response);
}

/**
 * Create, modify, or delete data collection item for node
 */
void ClientSession::modifyNodeDCI(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

   // Get node id and check object class and access rights
   uint32_t dwObjectId = request.getFieldAsUInt32(VID_OBJECT_ID);
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
            int dcObjectType = request.getFieldAsInt16(VID_DCOBJECT_TYPE);
            switch(request.getCode())
            {
               case CMD_MODIFY_NODE_DCI:
                  itemId = request.getFieldAsUInt32(VID_DCI_ID);
                  if (itemId == 0) // create new if id is 0
                  {
                     int dcObjectType = (int)request.getFieldAsUInt16(VID_DCOBJECT_TYPE);
                     // Create dummy DCI
                     DCObject *dcObject;
                     switch(dcObjectType)
                     {
                        case DCO_TYPE_ITEM:
                           dcObject = new DCItem(CreateUniqueId(IDG_ITEM), _T("no name"), DS_INTERNAL, DCI_DT_INT,
                                 DC_POLLING_SCHEDULE_DEFAULT, nullptr, DC_RETENTION_DEFAULT, nullptr,
                                 static_pointer_cast<DataCollectionOwner>(object));
                           break;
                        case DCO_TYPE_TABLE:
                           dcObject = new DCTable(CreateUniqueId(IDG_ITEM), _T("no name"), DS_INTERNAL,
                                 DC_POLLING_SCHEDULE_DEFAULT, nullptr, DC_RETENTION_DEFAULT, nullptr,
                                 static_pointer_cast<DataCollectionOwner>(object));
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
                           response.setField(VID_RCC, RCC_SUCCESS);
                           success = true;
                           // Return new item id to client
                           response.setField(VID_DCI_ID, dcObject->getId());
                        }
                        else  // Unable to add item to node
                        {
                           delete dcObject;
                           response.setField(VID_RCC, RCC_DUPLICATE_DCI);
                        }
                     }
                     else
                     {
                        response.setField(VID_RCC, RCC_INVALID_ARGUMENT);
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
                        response.setField(VID_RCC, RCC_SUCCESS);
                        shared_ptr<DCObject> dco = static_cast<DataCollectionOwner&>(*object).getDCObjectById(itemId, 0, true);
                        if (dco != nullptr)
                           NotifyClientsOnDCIUpdate(static_cast<DataCollectionOwner&>(*object), dco.get());

                        // Send index to id mapping for newly created thresholds to client
                        if (dcObjectType == DCO_TYPE_ITEM)
                        {
                           response.setField(VID_DCI_NUM_MAPS, mapCount);
                           for(uint32_t i = 0; i < mapCount; i++)
                           {
                              mapId[i] = htonl(mapId[i]);
                              mapIndex[i] = htonl(mapIndex[i]);
                           }
                           response.setField(VID_DCI_MAP_IDS, reinterpret_cast<BYTE*>(mapId), sizeof(uint32_t) * mapCount);
                           response.setField(VID_DCI_MAP_INDEXES, reinterpret_cast<BYTE*>(mapIndex), sizeof(uint32_t) * mapCount);
                           MemFree(mapId);
                           MemFree(mapIndex);
                        }
                     }
                     else
                     {
                        response.setField(VID_RCC, RCC_INVALID_DCI_ID);
                     }
                  }
                  break;
               case CMD_DELETE_NODE_DCI:
                  itemId = request.getFieldAsUInt32(VID_DCI_ID);
                  success = static_cast<DataCollectionOwner&>(*object).deleteDCObject(itemId, true, m_dwUserId);
                  response.setField(VID_RCC, success ? RCC_SUCCESS : RCC_INVALID_DCI_ID);
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
            response.setField(VID_RCC, RCC_ACCESS_DENIED);
            writeAuditLog(AUDIT_OBJECTS, false, dwObjectId, _T("Access denied on changing data collection configuration for object %s"), object->getName());
         }
      }
      else     // Object is not a node
      {
         response.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
      }
   }
   else  // No object with given ID
   {
      response.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   sendMessage(response);
}

/**
 * Change status for one or more DCIs
 */
void ClientSession::changeDCIStatus(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

   // Get node id and check object class and access rights
   shared_ptr<NetObj> object = FindObjectById(request.getFieldAsUInt32(VID_OBJECT_ID));
   if (object != nullptr)
   {
      if (object->isDataCollectionTarget() || (object->getObjectClass() == OBJECT_TEMPLATE))
      {
         if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_MODIFY))
         {
            int status = request.getFieldAsInt16(VID_DCI_STATUS);
            IntegerArray<uint32_t> dciList(request.getFieldAsUInt32(VID_NUM_ITEMS));
            request.getFieldAsInt32Array(VID_ITEM_LIST, &dciList);
            if (static_cast<DataCollectionOwner&>(*object).setItemStatus(dciList, status, true))
               response.setField(VID_RCC, RCC_SUCCESS);
            else
               response.setField(VID_RCC, RCC_INVALID_DCI_ID);
         }
         else  // User doesn't have MODIFY rights on object
         {
            response.setField(VID_RCC, RCC_ACCESS_DENIED);
         }
      }
      else     // Object is not a node
      {
         response.setField(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
      }
   }
   else  // No object with given ID
   {
      response.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   sendMessage(response);
}

/**
 * Recalculate values for DCI
 */
void ClientSession::recalculateDCIValues(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

   shared_ptr<NetObj> object = FindObjectById(request.getFieldAsUInt32(VID_OBJECT_ID));
   if (object != nullptr)
   {
      if (object->isDataCollectionTarget())
      {
         if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_MODIFY))
         {
            UINT32 dciId = request.getFieldAsUInt32(VID_DCI_ID);
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
                     response.setField(VID_RCC, RCC_SUCCESS);
                     response.setField(VID_JOB_ID, job->getId());
                     writeAuditLog(AUDIT_OBJECTS, true, object->getId(), _T("Data recalculation for DCI \"%s\" [%d] on object \"%s\" [%d] started (job ID %d)"),
                              dci->getDescription().cstr(), dci->getId(), object->getName(), object->getId(), job->getId());
                  }
                  else
                  {
                     delete job;
                     response.setField(VID_RCC, RCC_INTERNAL_ERROR);
                  }
               }
               else
               {
                  response.setField(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
               }
            }
            else
            {
               response.setField(VID_RCC, RCC_INVALID_DCI_ID);
               debugPrintf(4, _T("recalculateDCIValues: DCI %d at target %s [%d] not found"), dciId, object->getName(), object->getId());
            }
         }
         else  // User doesn't have MODIFY rights on object
         {
            response.setField(VID_RCC, RCC_ACCESS_DENIED);
            writeAuditLog(AUDIT_OBJECTS, false, object->getId(), _T("Access denied on recalculating DCI data"));
         }
      }
      else     // Object is not a node
      {
         response.setField(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
      }
   }
   else  // No object with given ID
   {
      response.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   sendMessage(response);
}

/**
 * Clear all collected data for DCI
 */
void ClientSession::clearDCIData(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

   // Get node id and check object class and access rights
   shared_ptr<NetObj> object = FindObjectById(request.getFieldAsUInt32(VID_OBJECT_ID));
   if (object != nullptr)
   {
      if (object->isDataCollectionTarget())
      {
         if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_MODIFY))
         {
				uint32_t dciId = request.getFieldAsUInt32(VID_DCI_ID);
				debugPrintf(4, _T("ClearDCIData: request for DCI %d at node %d"), dciId, object->getId());
            shared_ptr<DCObject> dci = static_cast<DataCollectionOwner&>(*object).getDCObjectById(dciId, m_dwUserId);
				if (dci != nullptr)
				{
					response.setField(VID_RCC, dci->deleteAllData() ? RCC_SUCCESS : RCC_DB_FAILURE);
					debugPrintf(4, _T("ClearDCIData: DCI %d at node %d"), dciId, object->getId());
	            writeAuditLog(AUDIT_OBJECTS, true, object->getId(), _T("Collected data for DCI \"%s\" [%d] on object \"%s\" [%d] cleared"),
	                     dci->getDescription().cstr(), dci->getId(), object->getName(), object->getId());
               notify(NX_NOTIFY_FORCE_DCI_POLL, object->getId());
				}
				else
				{
					response.setField(VID_RCC, RCC_INVALID_DCI_ID);
					debugPrintf(4, _T("ClearDCIData: DCI %d at node %d not found"), dciId, object->getId());
				}
         }
         else  // User doesn't have DELETE rights on object
         {
            writeAuditLog(AUDIT_OBJECTS, false, object->getId(), _T("Access denied on clear DCI data"));
            response.setField(VID_RCC, RCC_ACCESS_DENIED);
         }
      }
      else     // Object is not a node
      {
         response.setField(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
      }
   }
   else  // No object with given ID
   {
      response.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   // Send response
   sendMessage(response);
}

/**
 * Delete single entry from collected DCI data
 */
void ClientSession::deleteDCIEntry(const NXCPMessage& request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request.getId());

   // Get node id and check object class and access rights
   shared_ptr<NetObj> object = FindObjectById(request.getFieldAsUInt32(VID_OBJECT_ID));
   if (object != nullptr)
   {
      if (object->isDataCollectionTarget())
      {
         if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_MODIFY))
         {
            uint32_t dciId = request.getFieldAsUInt32(VID_DCI_ID);
            debugPrintf(4, _T("DeleteDCIEntry: request for DCI %d at node %d"), dciId, object->getId());
            shared_ptr<DCObject> dci = static_cast<DataCollectionOwner&>(*object).getDCObjectById(dciId, m_dwUserId);
            if (dci != nullptr)
            {
               msg.setField(VID_RCC, dci->deleteEntry(request.getFieldAsUInt32(VID_TIMESTAMP)) ? RCC_SUCCESS : RCC_DB_FAILURE);
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
void ClientSession::forceDCIPoll(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

   // Get node id and check object class and access rights
   shared_ptr<NetObj> object = FindObjectById(request.getFieldAsUInt32(VID_OBJECT_ID));
   if (object != nullptr)
   {
      if (object->isDataCollectionTarget())
      {
         if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
         {
				uint32_t dciId = request.getFieldAsUInt32(VID_DCI_ID);
				debugPrintf(4, _T("ForceDCIPoll: request for DCI %d at node %d"), dciId, object->getId());
            shared_ptr<DCObject> dci = static_cast<DataCollectionOwner&>(*object).getDCObjectById(dciId, m_dwUserId);
				if (dci != nullptr)
				{
				   if (dci->hasAccess(m_dwUserId))
				   {
                  dci->requestForcePoll(this);
                  response.setField(VID_RCC, RCC_SUCCESS);
                  debugPrintf(4, _T("ForceDCIPoll: DCI %d at node %d"), dciId, object->getId());
                  writeAuditLog(AUDIT_OBJECTS, true, object->getId(), _T("Forced DCI poll initiated for DCI \"%s\" [%u]"), dci->getDescription().cstr(), dci->getId());
				   }
				   else  // User doesn't have access to this DCI
				   {
		            response.setField(VID_RCC, RCC_ACCESS_DENIED);
		            writeAuditLog(AUDIT_OBJECTS, false, object->getId(), _T("Access denied on forced poll for DCI \"%s\" [%u]"), dci->getDescription().cstr(), dci->getId());
				   }
				}
				else
				{
					response.setField(VID_RCC, RCC_INVALID_DCI_ID);
					debugPrintf(4, _T("ForceDCIPoll: DCI %d at node %d not found"), dciId, object->getId());
				}
         }
         else  // User doesn't have READ rights on object
         {
            response.setField(VID_RCC, RCC_ACCESS_DENIED);
            writeAuditLog(AUDIT_OBJECTS, false, object->getId(), _T("Access denied on forced DCI poll"));
         }
      }
      else     // Object is not a node
      {
         response.setField(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
      }
   }
   else  // No object with given ID
   {
      response.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   sendMessage(response);
}

/**
 * Copy or move DCI from one node or template to another
 */
void ClientSession::copyDCI(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

   // Get source and destination
   shared_ptr<NetObj> sourceObject = FindObjectById(request.getFieldAsUInt32(VID_SOURCE_OBJECT_ID));
   shared_ptr<NetObj> destinationObject = FindObjectById(request.getFieldAsUInt32(VID_DESTINATION_OBJECT_ID));
   if ((sourceObject != nullptr) && (destinationObject != nullptr))
   {
      // Check object types
      if ((sourceObject->isDataCollectionTarget() || (sourceObject->getObjectClass() == OBJECT_TEMPLATE)) &&
		    (destinationObject->isDataCollectionTarget() || (destinationObject->getObjectClass() == OBJECT_TEMPLATE)))
      {
         bool doMove = request.getFieldAsBoolean(VID_MOVE_FLAG);
         // Check access rights
         if ((sourceObject->checkAccessRights(m_dwUserId, doMove ? (OBJECT_ACCESS_READ | OBJECT_ACCESS_MODIFY) : OBJECT_ACCESS_READ)) &&
             (destinationObject->checkAccessRights(m_dwUserId, OBJECT_ACCESS_MODIFY)))
         {
            UINT32 i, *pdwItemList, dwNumItems;
            int iErrors = 0;

            // Get list of items to be copied/moved
            dwNumItems = request.getFieldAsUInt32(VID_NUM_ITEMS);
            pdwItemList = MemAllocArray<UINT32>(dwNumItems);
            request.getFieldAsInt32Array(VID_ITEM_LIST, dwNumItems, pdwItemList);

            // Copy items
            for(i = 0; i < dwNumItems; i++)
            {
               shared_ptr<DCObject> srcItem = static_cast<DataCollectionOwner&>(*sourceObject).getDCObjectById(pdwItemList[i], m_dwUserId);
               if ((srcItem != nullptr) && srcItem->hasAccess(m_dwUserId))
               {
                  DCObject *dstItem = srcItem->clone();
                  dstItem->setTemplateId(0, 0);
                  dstItem->changeBinding(CreateUniqueId(IDG_ITEM),
                           static_pointer_cast<DataCollectionOwner>(destinationObject), FALSE);
                  if (static_cast<DataCollectionOwner&>(*destinationObject).addDCObject(dstItem))
                  {
                     if (doMove)
                     {
                        // Delete original item
                        if (!static_cast<DataCollectionOwner&>(*sourceObject).deleteDCObject(pdwItemList[i], true, m_dwUserId))
                        {
                           iErrors++;
                        }
                     }
                  }
                  else
                  {
                     delete dstItem;
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
            static_cast<DataCollectionOwner&>(*destinationObject).applyDCIChanges(!m_openDataCollectionConfigurations.contains(destinationObject->getId()));
            response.setField(VID_RCC, (iErrors == 0) ? RCC_SUCCESS : RCC_DCI_COPY_ERRORS);

            // Queue template update
            if (destinationObject->getObjectClass() == OBJECT_TEMPLATE)
               static_cast<DataCollectionOwner&>(*destinationObject).queueUpdate();
         }
         else  // User doesn't have enough rights on object(s)
         {
            response.setField(VID_RCC, RCC_ACCESS_DENIED);
         }
      }
      else     // Object(s) is not a node
      {
         response.setField(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
      }
   }
   else  // No object(s) with given ID
   {
      response.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   // Send response
   sendMessage(&response);
}

/**
 * Perform bulk update of DCI set
 */
void ClientSession::bulkDCIUpdate(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

   // Get node id and check object class and access rights
   shared_ptr<NetObj> object = FindObjectById(request.getFieldAsUInt32(VID_OBJECT_ID));
   if (object != nullptr)
   {
      if (object->isDataCollectionTarget() || (object->getObjectClass() == OBJECT_TEMPLATE))
      {
         if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_MODIFY))
         {
            response.setField(VID_NUM_ITEMS, static_cast<DataCollectionOwner&>(*object).updateMultipleDCObjects(request, m_dwUserId));
            response.setField(VID_RCC, RCC_SUCCESS);
            writeAuditLog(AUDIT_OBJECTS, true, object->getId(), _T("Successful bulk DCI update"));
         }
         else  // User doesn't have MODIFY rights on object
         {
            response.setField(VID_RCC, RCC_ACCESS_DENIED);
            writeAuditLog(AUDIT_OBJECTS, false, object->getId(), _T("Access denied on DCI bulk update"));
         }
      }
      else     // Object is not a node
      {
         response.setField(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
      }
   }
   else  // No object with given ID
   {
      response.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   sendMessage(response);
}

/**
 * Send list of thresholds for DCI
 */
void ClientSession::sendDCIThresholds(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

   // Get node id and check object class and access rights
   shared_ptr<NetObj> object = FindObjectById(request.getFieldAsUInt32(VID_OBJECT_ID));
   if (object != nullptr)
   {
      if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
      {
			if (object->isDataCollectionTarget())
			{
				shared_ptr<DCObject> dci = static_cast<DataCollectionTarget&>(*object).getDCObjectById(request.getFieldAsUInt32(VID_DCI_ID), m_dwUserId);
				if ((dci != nullptr) && (dci->getType() == DCO_TYPE_ITEM))
				{
				   if (dci->hasAccess(m_dwUserId))
				   {
                  static_cast<DCItem&>(*dci).fillMessageWithThresholds(&response, false);
                  response.setField(VID_RCC, RCC_SUCCESS);
				   }
				   else
				   {
			         response.setField(VID_RCC, RCC_ACCESS_DENIED);
				   }
				}
				else
				{
					response.setField(VID_RCC, RCC_INVALID_DCI_ID);
				}
         }
         else
         {
            response.setField(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
         }
      }
      else
      {
         response.setField(VID_RCC, RCC_ACCESS_DENIED);
      }
   }
   else  // No object with given ID
   {
		response.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   sendMessage(response);
}

#define SELECTION_COLUMNS (historicalDataType != HDT_RAW) ? tablePrefix : _T(""), (historicalDataType == HDT_RAW_AND_PROCESSED) ? _T("_value,raw_value") : ((historicalDataType == HDT_PROCESSED) || (historicalDataType == HDT_FULL_TABLE)) ?  _T("_value") : _T("raw_value")

/**
 * Prepare statement for reading data from idata/tdata table
 */
static DB_STATEMENT PrepareDataSelect(DB_HANDLE hdb, uint32_t nodeId, int dciType, DCObjectStorageClass storageClass,
         uint32_t maxRows, HistoricalDataType historicalDataType, const TCHAR *condition)
{
   const TCHAR *tablePrefix = (dciType == DCO_TYPE_ITEM) ? _T("idata") : _T("tdata");
	TCHAR query[512];
	if (g_flags & AF_SINGLE_TABLE_PERF_DATA)
	{
      switch(g_dbSyntax)
      {
         case DB_SYNTAX_MSSQL:
            _sntprintf(query, 512, _T("SELECT TOP %u %s_timestamp,%s%s FROM %s WHERE item_id=?%s ORDER BY %s_timestamp DESC"),
                     maxRows, tablePrefix, SELECTION_COLUMNS,
                     tablePrefix, condition, tablePrefix);
            break;
         case DB_SYNTAX_ORACLE:
            _sntprintf(query, 512, _T("SELECT * FROM (SELECT %s_timestamp,%s%s FROM %s WHERE item_id=?%s ORDER BY %s_timestamp DESC) WHERE ROWNUM<=%u"),
                     tablePrefix, SELECTION_COLUMNS,
                     tablePrefix, condition, tablePrefix, maxRows);
            break;
         case DB_SYNTAX_MYSQL:
         case DB_SYNTAX_PGSQL:
         case DB_SYNTAX_SQLITE:
            _sntprintf(query, 512, _T("SELECT %s_timestamp,%s%s FROM %s WHERE item_id=?%s ORDER BY %s_timestamp DESC LIMIT %u"),
                     tablePrefix, SELECTION_COLUMNS,
                     tablePrefix, condition, tablePrefix, maxRows);
            break;
         case DB_SYNTAX_TSDB:
            _sntprintf(query, 512, _T("SELECT date_part('epoch',%s_timestamp)::int,%s%s FROM %s_sc_%s WHERE item_id=?%s ORDER BY %s_timestamp DESC LIMIT %u"),
                     tablePrefix, SELECTION_COLUMNS,
                     tablePrefix, DCObject::getStorageClassName(storageClass), condition, tablePrefix, maxRows);
            break;
         case DB_SYNTAX_DB2:
            _sntprintf(query, 512, _T("SELECT %s_timestamp,%s%s FROM %s WHERE item_id=?%s ORDER BY %s_timestamp DESC FETCH FIRST %u ROWS ONLY"),
                     tablePrefix, SELECTION_COLUMNS,
                     tablePrefix, condition, tablePrefix, maxRows);
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
            _sntprintf(query, 512, _T("SELECT TOP %u %s_timestamp,%s%s FROM %s_%u WHERE item_id=?%s ORDER BY %s_timestamp DESC"),
                     maxRows, tablePrefix, SELECTION_COLUMNS,
                     tablePrefix, nodeId, condition, tablePrefix);
            break;
         case DB_SYNTAX_ORACLE:
            _sntprintf(query, 512, _T("SELECT * FROM (SELECT %s_timestamp,%s%s FROM %s_%u WHERE item_id=?%s ORDER BY %s_timestamp DESC) WHERE ROWNUM<=%u"),
                     tablePrefix, SELECTION_COLUMNS,
                     tablePrefix, nodeId, condition, tablePrefix, maxRows);
            break;
         case DB_SYNTAX_MYSQL:
         case DB_SYNTAX_PGSQL:
         case DB_SYNTAX_SQLITE:
         case DB_SYNTAX_TSDB:
            _sntprintf(query, 512, _T("SELECT %s_timestamp,%s%s FROM %s_%u WHERE item_id=?%s ORDER BY %s_timestamp DESC LIMIT %u"),
                     tablePrefix, SELECTION_COLUMNS,
                     tablePrefix, nodeId, condition, tablePrefix, maxRows);
            break;
         case DB_SYNTAX_DB2:
            _sntprintf(query, 512, _T("SELECT %s_timestamp,%s%s FROM %s_%u WHERE item_id=?%s ORDER BY %s_timestamp DESC FETCH FIRST %u ROWS ONLY"),
                     tablePrefix, SELECTION_COLUMNS,
                     tablePrefix, nodeId, condition, tablePrefix, maxRows);
            break;
         default:
            DbgPrintf(1, _T("INTERNAL ERROR: unsupported database in PrepareDataSelect"));
            return nullptr;	// Unsupported database
      }
	}
	return DBPrepare(hdb, query);
}

/**
 * Row sizes for sending DCI data to client
 */
static uint32_t s_rowSize[] = { 8, 8, 16, 16, 516, 16, 8, 8, 16 };

/**
 * Process results from SELECT statement for DCI data
 */
static void ProcessDataSelectResults(DB_UNBUFFERED_RESULT hResult, ClientSession *session, uint32_t requestId,
         const shared_ptr<DCObject>& dci, HistoricalDataType historicalDataType, const TCHAR* dataColumn, const TCHAR *instance)
{
   int dataType;
   switch(dci->getType())
   {
      case DCO_TYPE_ITEM:
         dataType = static_cast<DCItem&>(*dci).getDataType();
         break;
      case DCO_TYPE_TABLE:
         dataType = static_cast<DCTable&>(*dci).getColumnDataType(dataColumn);
         break;
      default:
         dataType = DCI_DT_STRING;
         break;
   }

   // Allocate memory for data and prepare data header
   int allocated = 8192;
   int rows = 0;
   auto pData = (DCI_DATA_HEADER *)MemAlloc(allocated * s_rowSize[dataType] + sizeof(DCI_DATA_HEADER));
   pData->dataType = htonl(static_cast<uint32_t>(dataType));
   pData->dciId = htonl(dci->getId());

#if !defined(UNICODE) || defined(UNICODE_UCS4)
   TCHAR textBuffer[MAX_DCI_STRING_VALUE];
#endif

   // Fill memory block with records
   auto currRow = (DCI_DATA_ROW *)(((char *)pData) + sizeof(DCI_DATA_HEADER));
   while(DBFetch(hResult))
   {
      if (rows == allocated)
      {
         allocated += 8192;
         pData = MemRealloc(pData, allocated * s_rowSize[dataType] + sizeof(DCI_DATA_HEADER));
         currRow = (DCI_DATA_ROW *)(((char *)pData + s_rowSize[dataType] * rows) + sizeof(DCI_DATA_HEADER));
      }
      rows++;

      currRow->timeStamp = htonl(DBGetFieldULong(hResult, 0));
      if (dci->getType() == DCO_TYPE_ITEM)
      {
         switch(dataType)
         {
            case DCI_DT_INT:
            case DCI_DT_UINT:
            case DCI_DT_COUNTER32:
               currRow->value.int32 = htonl(DBGetFieldULong(hResult, 1));
               break;
            case DCI_DT_INT64:
            case DCI_DT_UINT64:
            case DCI_DT_COUNTER64:
               currRow->value.ext.v64.int64 = htonq(DBGetFieldUInt64(hResult, 1));
               break;
            case DCI_DT_FLOAT:
               currRow->value.ext.v64.real = htond(DBGetFieldDouble(hResult, 1));
               break;
            case DCI_DT_STRING:
#ifdef UNICODE
#ifdef UNICODE_UCS4
               DBGetField(hResult, 1, textBuffer, MAX_DCI_STRING_VALUE);
               ucs4_to_ucs2(textBuffer, -1, currRow->value.string, MAX_DCI_STRING_VALUE);
#else
               DBGetField(hResult, 1, currRow->value.string, MAX_DCI_STRING_VALUE);
#endif
#else
               DBGetField(hResult, 1, textBuffer, MAX_DCI_STRING_VALUE);
               mb_to_ucs2(textBuffer, -1, currRow->value.string, MAX_DCI_STRING_VALUE);
#endif
               SwapUCS2String(currRow->value.string);
               break;
         }

         if (historicalDataType == HDT_RAW_AND_PROCESSED)
         {
            currRow = (DCI_DATA_ROW *)(((char *)currRow) + s_rowSize[dataType]);
            if (rows == allocated)
            {
               allocated += 8192;
               pData = (DCI_DATA_HEADER *)realloc(pData, allocated * s_rowSize[dataType] + sizeof(DCI_DATA_HEADER));
               currRow = (DCI_DATA_ROW *)(((char *)pData + s_rowSize[dataType] * rows) + sizeof(DCI_DATA_HEADER));
            }
            rows++;

            currRow->timeStamp = 0;   // raw value indicator
            switch(dataType)
            {
               case DCI_DT_INT:
               case DCI_DT_UINT:
               case DCI_DT_COUNTER32:
                  currRow->value.int32 = htonl(DBGetFieldULong(hResult, 2));
                  break;
               case DCI_DT_INT64:
               case DCI_DT_UINT64:
               case DCI_DT_COUNTER64:
                  currRow->value.ext.v64.int64 = htonq(DBGetFieldUInt64(hResult, 2));
                  break;
               case DCI_DT_FLOAT:
                  currRow->value.ext.v64.real = htond(DBGetFieldDouble(hResult, 2));
                  break;
               case DCI_DT_STRING:
#ifdef UNICODE
#ifdef UNICODE_UCS4
                  DBGetField(hResult, 2, textBuffer, MAX_DCI_STRING_VALUE);
                  ucs4_to_ucs2(textBuffer, -1, currRow->value.string, MAX_DCI_STRING_VALUE);
#else
                  DBGetField(hResult, 2, currRow->value.string, MAX_DCI_STRING_VALUE);
#endif
#else
                  DBGetField(hResult, 2, textBuffer, MAX_DCI_STRING_VALUE);
                  mb_to_ucs2(textBuffer, -1, currRow->value.string, MAX_DCI_STRING_VALUE);
#endif
                  SwapUCS2String(currRow->value.string);
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
                     currRow->value.int32 = htonl((UINT32)table->getAsInt(row, col));
                     break;
                  case DCI_DT_UINT:
                  case DCI_DT_COUNTER32:
                     currRow->value.int32 = htonl(table->getAsUInt(row, col));
                     break;
                  case DCI_DT_INT64:
                     currRow->value.ext.v64.int64 = htonq((UINT64)table->getAsInt64(row, col));
                     break;
                  case DCI_DT_UINT64:
                  case DCI_DT_COUNTER64:
                     currRow->value.ext.v64.int64 = htonq(table->getAsUInt64(row, col));
                     break;
                  case DCI_DT_FLOAT:
                     currRow->value.ext.v64.real = htond(table->getAsDouble(row, col));
                     break;
                  case DCI_DT_STRING:
#ifdef UNICODE
#ifdef UNICODE_UCS4
                     ucs4_to_ucs2(CHECK_NULL_EX(table->getAsString(row, col)), -1, currRow->value.string, MAX_DCI_STRING_VALUE);
#else
                     wcslcpy(currRow->value.string, CHECK_NULL_EX(table->getAsString(row, col)), MAX_DCI_STRING_VALUE);
#endif
#else
                     mb_to_ucs2(CHECK_NULL_EX(table->getAsString(row, col)), -1, currRow->value.string, MAX_DCI_STRING_VALUE);
#endif
                     SwapUCS2String(currRow->value.string);
                     break;
               }
               delete table;
            }
            MemFree(encodedTable);
         }
      }
      currRow = (DCI_DATA_ROW *)(((char *)currRow) + s_rowSize[dataType]);
   }
   pData->numRows = htonl(rows);

   // Prepare and send raw message with fetched data
   NXCP_MESSAGE *msg =
      CreateRawNXCPMessage(CMD_DCI_DATA, requestId, 0,
                           pData, rows * s_rowSize[dataType] + sizeof(DCI_DATA_HEADER),
                           nullptr, session->isCompressionEnabled());
   MemFree(pData);
   session->sendRawMessage(msg);
   MemFree(msg);
}

/**
 * Process results from SELECT statement for table DCI data with full tables as result
 */
static void ProcessTableDataSelectResults(DB_UNBUFFERED_RESULT hResult, ClientSession *session, uint32_t requestId)
{
   NXCPMessage msg(CMD_DCI_DATA, requestId);
   while(DBFetch(hResult))
   {
      char *encodedTable = DBGetFieldUTF8(hResult, 1, nullptr, 0);
      if (encodedTable != nullptr)
      {
         Table *table = Table::createFromPackedXML(encodedTable);
         if (table != nullptr)
         {
            msg.setField(VID_TIMESTAMP, DBGetFieldULong(hResult, 0));
            table->fillMessage(&msg, 0, -1);
            delete table;
            session->sendMessage(msg);
            msg.deleteAllFields();
         }
         MemFree(encodedTable);
      }
   }

   msg.setField(VID_TIMESTAMP, static_cast<uint32_t>(0));   // End of data indicator
   session->sendMessage(msg);
}

/**
 * Get collected data for table or simple DCI
 */
bool ClientSession::getCollectedDataFromDB(const NXCPMessage& request, NXCPMessage *response, const DataCollectionTarget& dcTarget, int dciType, HistoricalDataType historicalDataType)
{
	// Find DCI object
	shared_ptr<DCObject> dci = dcTarget.getDCObjectById(request.getFieldAsUInt32(VID_DCI_ID), 0);
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
	// "full table" request type is only available for table DCIs
	// "raw" request type is only available for single value DCI
	if ((dci->getType() != dciType) || ((historicalDataType == HDT_FULL_TABLE) && (dciType != DCO_TYPE_TABLE)) ||
	    (((historicalDataType == HDT_RAW) || (historicalDataType == HDT_RAW_AND_PROCESSED)) && (dciType != DCO_TYPE_ITEM)))
	{
		response->setField(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
		return false;
	}

	// Check that all required data present in message
	if ((dciType == DCO_TYPE_TABLE) && (historicalDataType != HDT_FULL_TABLE) && (!request.isFieldExist(VID_DATA_COLUMN) || !request.isFieldExist(VID_INSTANCE)))
	{
		response->setField(VID_RCC, RCC_INVALID_ARGUMENT);
		return false;
	}

	// Get request parameters
	uint32_t maxRows = request.getFieldAsUInt32(VID_MAX_ROWS);
	uint32_t timeFrom = request.getFieldAsUInt32(VID_TIME_FROM);
	uint32_t timeTo = request.getFieldAsUInt32(VID_TIME_TO);

	if ((maxRows == 0) || (maxRows > MAX_DCI_DATA_RECORDS))
		maxRows = MAX_DCI_DATA_RECORDS;

	// If only last value requested, try to get it from cache first
	if ((maxRows == 1) && (timeTo == 0) && (historicalDataType == HDT_PROCESSED))
	{
	   debugPrintf(7, _T("getCollectedDataFromDB: maxRows set to 1, will try to read cached value"));

      TCHAR dataColumn[MAX_COLUMN_NAME] = _T("");
      ItemValue value;

	   if (dciType == DCO_TYPE_ITEM)
	   {
	      ItemValue *v = static_cast<DCItem&>(*dci).getInternalLastValue();
	      if (v == nullptr)
	         goto read_from_db;
	      value = *v;
	      delete v;
	   }
	   else
	   {
         request.getFieldAsString(VID_DATA_COLUMN, dataColumn, MAX_COLUMN_NAME);
         shared_ptr<Table> t = static_cast<DCTable&>(*dci).getLastValue();
         if (t == nullptr)
            goto read_from_db;

         int column = t->getColumnIndex(dataColumn);
         if (column == -1)
            goto read_from_db;

         TCHAR instance[256];
         request.getFieldAsString(VID_INSTANCE, instance, 256);
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
      auto pData = (DCI_DATA_HEADER *)MemAlloc(s_rowSize[dataType] + sizeof(DCI_DATA_HEADER));
      pData->dataType = htonl(static_cast<uint32_t>(dataType));
      pData->dciId = htonl(dci->getId());

      // Fill memory block with records
      auto currRow = (DCI_DATA_ROW *)(((char *)pData) + sizeof(DCI_DATA_HEADER));
      currRow->timeStamp = static_cast<uint32_t>(dci->getLastPollTime());
      switch(dataType)
      {
         case DCI_DT_INT:
         case DCI_DT_UINT:
         case DCI_DT_COUNTER32:
            currRow->value.int32 = htonl(value.getUInt32());
            break;
         case DCI_DT_INT64:
         case DCI_DT_UINT64:
         case DCI_DT_COUNTER64:
            currRow->value.ext.v64.int64 = htonq(value.getUInt64());
            break;
         case DCI_DT_FLOAT:
            currRow->value.ext.v64.real = htond(value.getDouble());
            break;
         case DCI_DT_STRING:
#ifdef UNICODE
#ifdef UNICODE_UCS4
            ucs4_to_ucs2(value.getString(), -1, currRow->value.string, MAX_DCI_STRING_VALUE);
#else
            wcslcpy(currRow->value.string, value.getString(), MAX_DCI_STRING_VALUE);
#endif
#else
            mb_to_ucs2(value.getString(), -1, currRow->value.string, MAX_DCI_STRING_VALUE);
#endif
            SwapUCS2String(currRow->value.string);
            break;
      }
      pData->numRows = 1;

      // Prepare and send raw message with fetched data
      NXCP_MESSAGE *msg =
         CreateRawNXCPMessage(CMD_DCI_DATA, request.getId(), 0,
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
			request.getFieldAsString(VID_DATA_COLUMN, dataColumn, MAX_COLUMN_NAME);
         request.getFieldAsString(VID_INSTANCE, instance, 256);
		}
		if (timeFrom != 0)
			DBBind(hStmt, pos++, DB_SQLTYPE_INTEGER, timeFrom);
		if (timeTo != 0)
			DBBind(hStmt, pos++, DB_SQLTYPE_INTEGER, timeTo);

		DB_UNBUFFERED_RESULT hResult = DBSelectPreparedUnbuffered(hStmt);
		if (hResult != nullptr)
		{
			// Send CMD_REQUEST_COMPLETED message
			response->setField(VID_RCC, RCC_SUCCESS);
			if (dciType == DCO_TYPE_ITEM)
			   static_cast<DCItem*>(dci.get())->fillMessageWithThresholds(response, false);
			sendMessage(response);

			if (historicalDataType == HDT_FULL_TABLE)
            ProcessTableDataSelectResults(hResult, this, request.getId());
			else
			   ProcessDataSelectResults(hResult, this, request.getId(), dci, historicalDataType, dataColumn, instance);

		   DBFreeResult(hResult);
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
void ClientSession::getCollectedData(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());
	bool success = false;

   shared_ptr<NetObj> object = FindObjectById(request.getFieldAsUInt32(VID_OBJECT_ID));
   if (object != nullptr)
   {
		if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
		{
			if (object->isDataCollectionTarget())
			{
				if (!(g_flags & AF_DB_CONNECTION_LOST))
				{
					success = getCollectedDataFromDB(request, &response, static_cast<DataCollectionTarget&>(*object),
					         DCO_TYPE_ITEM, static_cast<HistoricalDataType>(request.getFieldAsInt16(VID_HISTORICAL_DATA_TYPE)));
				}
				else
				{
					response.setField(VID_RCC, RCC_DB_CONNECTION_LOST);
				}
			}
			else
			{
				response.setField(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
			}
		}
		else
		{
			response.setField(VID_RCC, RCC_ACCESS_DENIED);
		}
   }
   else  // No object with given ID
   {
      response.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   // Send response
   if (!success)
      sendMessage(response);
}

/**
 * Get collected data for table DCI
 */
void ClientSession::getTableCollectedData(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());
	bool success = false;

   shared_ptr<NetObj> object = FindObjectById(request.getFieldAsUInt32(VID_OBJECT_ID));
   if (object != nullptr)
   {
		if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
		{
			if (object->isDataCollectionTarget())
			{
				if (!(g_flags & AF_DB_CONNECTION_LOST))
				{
					success = getCollectedDataFromDB(request, &response, static_cast<DataCollectionTarget&>(*object),
					         DCO_TYPE_TABLE, static_cast<HistoricalDataType>(request.getFieldAsInt16(VID_HISTORICAL_DATA_TYPE)));
				}
				else
				{
					response.setField(VID_RCC, RCC_DB_CONNECTION_LOST);
				}
			}
			else
			{
				response.setField(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
			}
		}
		else
		{
			response.setField(VID_RCC, RCC_ACCESS_DENIED);
		}
   }
   else
   {
      response.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

	if (!success)
      sendMessage(response);
}

/**
 * Send latest collected values for all DCIs of given node
 */
void ClientSession::getLastValues(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

   // Get node id and check object class and access rights
   shared_ptr<NetObj> object = FindObjectById(request.getFieldAsUInt32(VID_OBJECT_ID));
   if (object != nullptr)
   {
      if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
      {
         if (object->isDataCollectionTarget())
         {
            response.setField(VID_RCC,
               static_cast<DataCollectionTarget&>(*object).getLastValues(&response,
                  request.getFieldAsBoolean(VID_OBJECT_TOOLTIP_ONLY),
                  request.getFieldAsBoolean(VID_OVERVIEW_ONLY),
                  request.getFieldAsBoolean(VID_INCLUDE_NOVALUE_OBJECTS),
                  m_dwUserId));
         }
         else
         {
            response.setField(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
         }
      }
      else
      {
         response.setField(VID_RCC, RCC_ACCESS_DENIED);
      }
   }
   else  // No object with given ID
   {
      response.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   sendMessage(response);
}

/**
 * Send latest collected values for all DCIs from given list.
 * Error message will never be returned. Will be returned only
 * possible DCI values.
 */
void ClientSession::getLastValuesByDciId(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

   int size = request.getFieldAsInt32(VID_NUM_ITEMS);
   uint32_t incomingIndex = VID_DCI_VALUES_BASE;
   uint32_t outgoingIndex = VID_DCI_VALUES_BASE;
   for(int i = 0; i < size; i++, incomingIndex += 10)
   {
      shared_ptr<NetObj> object = FindObjectById(request.getFieldAsUInt32(incomingIndex));
      if (object != nullptr)
      {
         if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
         {
            if (object->isDataCollectionTarget() || (object->getObjectClass() == OBJECT_TEMPLATE))
            {
               uint32_t dciID = request.getFieldAsUInt32(incomingIndex + 1);
               shared_ptr<DCObject> dcoObj = static_cast<DataCollectionTarget&>(*object).getDCObjectById(dciID, m_dwUserId);
               if (dcoObj == nullptr)
                  continue;

               SharedString column = request.getFieldAsSharedString(incomingIndex + 2);
               SharedString instance = request.getFieldAsSharedString(incomingIndex + 3);
               dcoObj->fillLastValueSummaryMessage(&response, outgoingIndex, column, instance);
               outgoingIndex += 50;
            }
         }
      }
   }

   response.setField(VID_NUM_ITEMS, (outgoingIndex - VID_DCI_VALUES_BASE) / 50);
   response.setField(VID_RCC, RCC_SUCCESS);

   sendMessage(response);
}

/**
 * Send tooltip visible latest collected values for all nodes
 * Error message will never be returned. Will be returned only
 * possible DCI values.
 */
void ClientSession::getTooltipLastValues(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

   IntegerArray<uint32_t> nodes;
   request.getFieldAsInt32Array(VID_OBJECT_LIST, &nodes);
   uint32_t index = VID_DCI_VALUES_BASE;
   for(int i = 0; i < nodes.size(); i++)
   {
      shared_ptr<NetObj> object = FindObjectById(nodes.get(i));
      if (object != nullptr)
      {
         if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
         {
            if (object->isDataCollectionTarget())
            {
               static_cast<DataCollectionTarget&>(*object).getTooltipLastValues(&response, m_dwUserId, &index);
            }
         }
      }
   }

   response.setField(VID_NUM_ITEMS, (index - VID_DCI_VALUES_BASE) / 50);
   response.setField(VID_RCC, RCC_SUCCESS);

   sendMessage(&response);
}

/**
 * Send latest collected value for given table DCI of given node
 */
void ClientSession::getTableLastValue(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

   // Get node id and check object class and access rights
   shared_ptr<NetObj> object = FindObjectById(request.getFieldAsUInt32(VID_OBJECT_ID));
   if (object != nullptr)
   {
      if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
      {
         if (object->isDataCollectionTarget())
         {
				response.setField(VID_RCC, static_cast<DataCollectionTarget&>(*object).getTableLastValue(request.getFieldAsUInt32(VID_DCI_ID), &response));
         }
         else
         {
            response.setField(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
         }
      }
      else
      {
         response.setField(VID_RCC, RCC_ACCESS_DENIED);
      }
   }
   else  // No object with given ID
   {
      response.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   // Send response
   sendMessage(&response);
}

/**
 * Send latest collected value for given DCI (table or single valued) of given node
 */
void ClientSession::getLastValue(const NXCPMessage& request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request.getId());

   // Get node id and check object class and access rights
   shared_ptr<NetObj> object = FindObjectById(request.getFieldAsUInt32(VID_OBJECT_ID));
   if (object != nullptr)
   {
      if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
      {
         if (object->isDataCollectionTarget())
         {
            msg.setField(VID_RCC, static_cast<DataCollectionTarget&>(*object).getDciLastValue(request.getFieldAsUInt32(VID_DCI_ID), &msg));
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
void ClientSession::getActiveThresholds(const NXCPMessage& request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request.getId());

   UINT32 base = VID_DCI_VALUES_BASE;
   int numItems = request.getFieldAsInt32(VID_NUM_ITEMS);

   for(int i = 0; i < numItems; i++, base+=2)
   {
      shared_ptr<NetObj> object = FindObjectById(request.getFieldAsUInt32(base));
      if (object != nullptr && object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
      {
         if (object->isDataCollectionTarget() || (object->getObjectClass() == OBJECT_TEMPLATE))
         {
            shared_ptr<DCObject> dcObject = static_cast<DataCollectionTarget&>(*object).getDCObjectById(request.getFieldAsUInt32(base + 1), m_dwUserId);
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
      if (!readOnly && !LockEPP(m_id, m_sessionName, nullptr, buffer))
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
         UnlockEPP();
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
         m_ppEPPRuleList[m_dwRecordsUploaded] = new EPRule(*request);
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
 * Get compiled MIB file
 */
void ClientSession::getMIBFile(const NXCPMessage& request)
{
   TCHAR mibFile[MAX_PATH];
   _tcscpy(mibFile, g_netxmsdDataDir);
   _tcscat(mibFile, DFILE_COMPILED_MIB);
	sendFile(mibFile, request.getId(), 0);
}

/**
 * Get timestamp of compiled MIB file
 */
void ClientSession::getMIBTimestamp(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

   TCHAR mibFileName[MAX_PATH];
   _tcscpy(mibFileName, g_netxmsdDataDir);
   _tcscat(mibFileName, DFILE_COMPILED_MIB);

   uint32_t timestamp;
   uint32_t rc = SnmpGetMIBTreeTimestamp(mibFileName, &timestamp);
   if (rc == SNMP_ERR_SUCCESS)
   {
      response.setField(VID_RCC, RCC_SUCCESS);
      response.setField(VID_TIMESTAMP, timestamp);
		response.setField(VID_FILE_SIZE, FileSize(mibFileName));
   }
   else
   {
      switch(rc)
      {
         case SNMP_ERR_FILE_IO:
            response.setField(VID_RCC, RCC_FILE_IO_ERROR);
            break;
         case SNMP_ERR_BAD_FILE_HEADER:
            response.setField(VID_RCC, RCC_CORRUPTED_MIB_FILE);
            break;
         default:
            response.setField(VID_RCC, RCC_INTERNAL_ERROR);
            break;
      }
   }

   sendMessage(response);
}

/**
 * Create new object
 */
void ClientSession::createObject(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

   int objectClass = request.getFieldAsUInt16(VID_OBJECT_CLASS);
   int32_t zoneUIN = request.getFieldAsUInt32(VID_ZONE_UIN);

   // Find parent object
   shared_ptr<NetObj> parent = FindObjectById(request.getFieldAsUInt32(VID_PARENT_ID));

   TCHAR nodePrimaryName[MAX_DNS_NAME];
   InetAddress ipAddr;
   bool parentAlwaysValid = false;

   if (objectClass == OBJECT_NODE)
   {
		if (request.isFieldExist(VID_PRIMARY_NAME))
		{
			request.getFieldAsString(VID_PRIMARY_NAME, nodePrimaryName, MAX_DNS_NAME);
         ipAddr = ResolveHostName(zoneUIN, nodePrimaryName);
		}
		else
		{
         ipAddr = request.getFieldAsInetAddress(VID_IP_ADDRESS);
         ipAddr.toString(nodePrimaryName);
		}
      if ((parent == nullptr) && ipAddr.isValidUnicast())
      {
         parent = FindSubnetForNode(zoneUIN, ipAddr);
         parentAlwaysValid = true;
      }
   }

   uint32_t assetId = request.getFieldAsUInt32(VID_ASSET_ID);
   shared_ptr<Asset> asset = static_pointer_cast<Asset>(FindObjectById(assetId, OBJECT_ASSET));

   if (((parent != nullptr) || (objectClass == OBJECT_NODE)) &&
       ((assetId == 0) || (asset != nullptr)))
   {
      // User should have create access to parent object
      if ((parent != nullptr) ?
            parent->checkAccessRights(m_dwUserId, OBJECT_ACCESS_CREATE) :
            g_entireNetwork->checkAccessRights(m_dwUserId, OBJECT_ACCESS_CREATE))
      {
         TCHAR objectName[MAX_OBJECT_NAME];
         request.getFieldAsString(VID_OBJECT_NAME, objectName, MAX_OBJECT_NAME);

         // Do additional checks
         uint32_t rcc = RCC_SUCCESS;

         // Parent object should be of valid type
         // If asset ID is set then class should be valid target for linking asset
         if ((!parentAlwaysValid && !IsValidParentClass(objectClass, (parent != nullptr) ? parent->getObjectClass() : -1)) ||
             ((assetId != 0) && !IsValidAssetLinkTargetClass(objectClass)))
         {
            debugPrintf(4, _T("Creation of object \"%s\" of class %d under %s [%u] failed (incompatible operation)"), objectName, objectClass,
                  (parent != nullptr) ? parent->getName() : _T("<no parent>"), (parent != nullptr) ? parent->getId() : 0);
            rcc = RCC_INCOMPATIBLE_OPERATION;
         }

         // Check zone
         if ((rcc == RCC_SUCCESS) && IsZoningEnabled() && (zoneUIN != 0) && (objectClass != OBJECT_ZONE) && (FindZoneByUIN(zoneUIN) == nullptr))
         {
            debugPrintf(4, _T("Creation of object \"%s\" of class %d under %s [%u] failed (invalid zone UIN %d)"), objectName, objectClass,
                  (parent != nullptr) ? parent->getName() : _T("<no parent>"), (parent != nullptr) ? parent->getId() : 0, zoneUIN);
            rcc = RCC_INVALID_ZONE_ID;
         }

         // Check if all mandatory asset properties are set
         StringMap assetProperties;
         if ((rcc == RCC_SUCCESS) && (objectClass == OBJECT_ASSET))
         {
            assetProperties.addAllFromMessage(request, VID_ASSET_PROPERTIES_BASE, VID_NUM_ASSET_PROPERTIES);
            unique_ptr<StringSet> mandatoryAttributes = GetAssetAttributeNames(true);
            for(const TCHAR *a : *mandatoryAttributes)
            {
               if (!assetProperties.contains(a))
               {
                  debugPrintf(4, _T("Creation of asset object \"%s\" under %s [%u] failed (mandatory attribute \"%s\" missing)"), objectName,
                        (parent != nullptr) ? parent->getName() : _T("<no parent>"), (parent != nullptr) ? parent->getId() : 0, a);
                  rcc = RCC_MANDATORY_ATTRIBUTE_MISSING;
                  break;
               }
            }
         }

         // Do additional validation by modules
         if (rcc == RCC_SUCCESS)
         {
            ENUMERATE_MODULES(pfValidateObjectCreation)
            {
               rcc = CURRENT_MODULE.pfValidateObjectCreation(objectClass, objectName, ipAddr, zoneUIN, request);
               if (rcc != RCC_SUCCESS)
               {
                  debugPrintf(4, _T("Creation of object \"%s\" of class %d blocked by module %s (RCC=%u)"), objectName, objectClass, CURRENT_MODULE.szName, rcc);
                  break;
               }
            }
         }

         if (rcc == RCC_SUCCESS)
         {
            // Create new object
            shared_ptr<NetObj> object;
            TCHAR deviceId[MAX_OBJECT_NAME];
            switch(objectClass)
            {
               case OBJECT_ASSET:
                  object = make_shared<Asset>(objectName, assetProperties);
                  NetObjInsert(object, true, false);
                  object->calculateCompoundStatus();  // Force status change to NORMAL
                  break;
               case OBJECT_ASSETGROUP:
                  object = make_shared<AssetGroup>(objectName);
                  NetObjInsert(object, true, false);
                  object->calculateCompoundStatus();  // Force status change to NORMAL
                  break;
               case OBJECT_BUSINESSSERVICEPROTO:
                  object = make_shared<BusinessServicePrototype>(objectName, request.getFieldAsUInt32(VID_INSTD_METHOD));
                  NetObjInsert(object, true, false);
                  break;
               case OBJECT_BUSINESSSERVICE:
                  object = make_shared<BusinessService>(objectName);
                  NetObjInsert(object, true, false);
                  break;
               case OBJECT_CHASSIS:
                  object = make_shared<Chassis>(objectName, request.getFieldAsUInt32(VID_CONTROLLER_ID));
                  NetObjInsert(object, true, false);
                  break;
               case OBJECT_CLUSTER:
                  object = make_shared<Cluster>(objectName, zoneUIN);
                  NetObjInsert(object, true, false);
                  break;
               case OBJECT_CONDITION:
                  object = make_shared<ConditionObject>(true);
                  object->setName(objectName);
                  NetObjInsert(object, true, false);
                  break;
               case OBJECT_CONTAINER:
                  object = make_shared<Container>(objectName, request.getFieldAsUInt32(VID_CATEGORY));
                  NetObjInsert(object, true, false);
                  object->calculateCompoundStatus();  // Force status change to NORMAL
                  break;
               case OBJECT_DASHBOARD:
                  object = make_shared<Dashboard>(objectName);
                  NetObjInsert(object, true, false);
                  break;
               case OBJECT_DASHBOARDGROUP:
                  object = make_shared<DashboardGroup>(objectName);
                  NetObjInsert(object, true, false);
                  object->calculateCompoundStatus();
                  break;
               case OBJECT_INTERFACE:
                  {
                     InterfaceInfo ifInfo(request.getFieldAsUInt32(VID_IF_INDEX));
                     _tcslcpy(ifInfo.name, objectName, MAX_DB_STRING);
                     InetAddress addr = request.getFieldAsInetAddress(VID_IP_ADDRESS);
                     if (addr.isValidUnicast())
                        ifInfo.ipAddrList.add(addr);
                     ifInfo.type = request.getFieldAsUInt32(VID_IF_TYPE);
                     request.getFieldAsBinary(VID_MAC_ADDR, ifInfo.macAddr, MAC_ADDR_LENGTH);
                     ifInfo.location.chassis = request.getFieldAsUInt32(VID_PHY_CHASSIS);
                     ifInfo.location.module = request.getFieldAsUInt32(VID_PHY_MODULE);
                     ifInfo.location.pic = request.getFieldAsUInt32(VID_PHY_PIC);
                     ifInfo.location.port = request.getFieldAsUInt32(VID_PHY_PORT);
                     ifInfo.isPhysicalPort = request.getFieldAsBoolean(VID_IS_PHYS_PORT);
                     object = static_cast<Node&>(*parent).createNewInterface(&ifInfo, true, false);
                  }
                  break;
               case OBJECT_MOBILEDEVICE:
                  request.getFieldAsString(VID_DEVICE_ID, deviceId, MAX_OBJECT_NAME);
                  object = make_shared<MobileDevice>(objectName, deviceId);
                  NetObjInsert(object, true, false);
                  break;
               case OBJECT_SENSOR:
                  object = Sensor::create(objectName, request);
                  if (object != nullptr)
                     NetObjInsert(object, true, false);
                  break;
               case OBJECT_NETWORKMAP:
                  {
                     IntegerArray<uint32_t> seeds;
                     request.getFieldAsInt32Array(VID_SEED_OBJECTS, &seeds);
                     object = make_shared<NetworkMap>((int)request.getFieldAsUInt16(VID_MAP_TYPE), seeds);
                     object->setName(objectName);
                     NetObjInsert(object, true, false);
                  }
                  break;
               case OBJECT_NETWORKMAPGROUP:
                  object = make_shared<NetworkMapGroup>(objectName);
                  NetObjInsert(object, true, false);
                  object->calculateCompoundStatus();  // Force status change to NORMAL
                  break;
               case OBJECT_NETWORKSERVICE:
                  object = make_shared<NetworkService>(request.getFieldAsInt16(VID_SERVICE_TYPE),
                                              request.getFieldAsUInt16(VID_IP_PROTO),
                                              request.getFieldAsUInt16(VID_IP_PORT),
                                              request.getFieldAsString(VID_SERVICE_REQUEST),
                                              request.getFieldAsString(VID_SERVICE_RESPONSE),
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
               case OBJECT_RACK:
                  object = make_shared<Rack>(objectName, (int)request.getFieldAsUInt16(VID_HEIGHT));
                  NetObjInsert(object, true, false);
                  break;
               case OBJECT_SUBNET:
               {
                  auto ipAddr = request.getFieldAsInetAddress(VID_IP_ADDRESS);
                  IntegerArray<uint32_t> objects = CheckSubnetOverlap(ipAddr, zoneUIN);
                  if (objects.size() > 0)
                  {
                     response.setFieldFromInt32Array(VID_OBJECT_LIST, objects);
                  }
                  else
                  {
                     object = make_shared<Subnet>(objectName, ipAddr, zoneUIN);
                     NetObjInsert(object, true, false);
                  }
                  break;
               }
               case OBJECT_TEMPLATE:
                  object = make_shared<Template>(objectName);
                  NetObjInsert(object, true, false);
                  object->calculateCompoundStatus();  // Force status change to NORMAL
                  break;
               case OBJECT_TEMPLATEGROUP:
                  object = make_shared<TemplateGroup>(objectName);
                  NetObjInsert(object, true, false);
                  object->calculateCompoundStatus();	// Force status change to NORMAL
                  break;
               case OBJECT_VPNCONNECTOR:
                  object = make_shared<VPNConnector>(true);
                  object->setName(objectName);
                  NetObjInsert(object, true, false);
                  break;
               case OBJECT_ZONE:
                  if (zoneUIN == 0)
                     zoneUIN = FindUnusedZoneUIN();
                  if ((zoneUIN > 0) && (zoneUIN != ALL_ZONES) && (FindZoneByUIN(zoneUIN) == nullptr))
                  {
                     object = make_shared<Zone>(zoneUIN, objectName);
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
                  else if ((object->getObjectClass() == OBJECT_NETWORKSERVICE) && request.getFieldAsBoolean(VID_CREATE_STATUS_DCI))
                  {
                     TCHAR dciName[MAX_DB_STRING], dciDescription[MAX_DB_STRING];
                     _sntprintf(dciName, MAX_DB_STRING, _T("ChildStatus(%d)"), object->getId());
                     _sntprintf(dciDescription, MAX_DB_STRING, _T("Status of network service %s"), object->getName());
                     static_cast<Node&>(*parent).addDCObject(new DCItem(CreateUniqueId(IDG_ITEM), dciName, DS_INTERNAL, DCI_DT_INT,
                           DC_POLLING_SCHEDULE_DEFAULT, nullptr, DC_RETENTION_DEFAULT, nullptr, static_pointer_cast<Node>(parent),
                           dciDescription));
                  }
                  else if (object->getObjectClass() == OBJECT_ASSET)
                  {
                     for (KeyValuePair<const TCHAR> *pair : assetProperties)
                        WriteAssetChangeLog(object->getId(), pair->key, AssetOperation::Create, nullptr, pair->value, m_dwUserId, 0);
                  }
               }

               TCHAR *comments = request.getFieldAsString(VID_COMMENTS);
               if (comments != nullptr)
               {
                  object->setComments(comments);
                  MemFree(comments);
               }

               if (request.isFieldExist(VID_ALIAS))
               {
                  TCHAR alias[MAX_OBJECT_ALIAS_LEN];
                  request.getFieldAsString(VID_ALIAS, alias, MAX_OBJECT_ALIAS_LEN);
                  object->setAlias(alias);
               }

               json_t *objData = object->toJson();
               WriteAuditLogWithJsonValues(AUDIT_OBJECTS, true, m_dwUserId, m_workstation, m_id, object->getId(), nullptr, objData,
                     _T("Object %s created (class %s)"), object->getName(), object->getObjectClassName());
               json_decref(objData);

               if (asset != nullptr)
               {
                  LinkAsset(asset.get(), object.get(), this);
               }

               object->unhide();
               response.setField(VID_RCC, RCC_SUCCESS);
               response.setField(VID_OBJECT_ID, object->getId());
            }
            else
            {
               // :DIRTY HACK:
               // PollNewNode will return nullptr only if IP already
               // in use. some new() can fail there too, but server will
               // crash in that case
               if (objectClass == OBJECT_NODE)
               {
                  response.setField(VID_RCC, RCC_IP_ADDRESS_CONFLICT);
                  // Add to description IP of new created node and name of node with the same IP
                  SetNodesConflictString(&response, zoneUIN, ipAddr);
               }
               else if (objectClass == OBJECT_ZONE)
               {
                  response.setField(VID_RCC, RCC_ZONE_ID_ALREADY_IN_USE);
               }
               else if (objectClass == OBJECT_SUBNET)
               {
                  response.setField(VID_RCC, RCC_SUBNET_OVERLAP);
               }
               else
               {
                  response.setField(VID_RCC, RCC_OBJECT_CREATION_FAILED);
               }
            }
         }
         else
         {
            response.setField(VID_RCC, rcc);
         }
      }
      else
      {
         writeAuditLog(AUDIT_OBJECTS, false, (parent != nullptr) ? parent->getId() : 0, _T("Access denied on new object creation"));
         response.setField(VID_RCC, RCC_ACCESS_DENIED);
      }
   }
   else
   {
      response.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   sendMessage(response);
}

/**
 * Add cluster node
 */
void ClientSession::addClusterNode(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

   shared_ptr<NetObj> cluster = FindObjectById(request.getFieldAsUInt32(VID_PARENT_ID));
   shared_ptr<NetObj> node = FindObjectById(request.getFieldAsUInt32(VID_CHILD_ID));
	if ((cluster != nullptr) && (node != nullptr))
	{
		if ((cluster->getObjectClass() == OBJECT_CLUSTER) && (node->getObjectClass() == OBJECT_NODE))
		{
		   shared_ptr<Cluster> currentCluster = static_cast<Node&>(*node).getMyCluster();
			if (currentCluster == nullptr)
			{
				if (cluster->checkAccessRights(m_dwUserId, OBJECT_ACCESS_MODIFY) &&
					 node->checkAccessRights(m_dwUserId, OBJECT_ACCESS_MODIFY))
				{
				   if (static_cast<Cluster&>(*cluster).addNode(static_pointer_cast<Node>(node)))
				   {
                  response.setField(VID_RCC, RCC_SUCCESS);
                  WriteAuditLog(AUDIT_OBJECTS, TRUE, m_dwUserId, m_workstation, m_id, cluster->getId(),
                                _T("Node %s [%d] added to cluster %s [%d]"),
                                node->getName(), node->getId(), cluster->getName(), cluster->getId());
				   }
				   else
				   {
	               response.setField(VID_RCC, RCC_ZONE_MISMATCH);
				   }
				}
				else
				{
					response.setField(VID_RCC, RCC_ACCESS_DENIED);
					WriteAuditLog(AUDIT_OBJECTS, FALSE, m_dwUserId, m_workstation, m_id, cluster->getId(),
									  _T("Access denied on adding node %s [%d] to cluster %s [%d]"),
									  node->getName(), node->getId(), cluster->getName(), cluster->getId());
				}
			}
			else
			{
			   if (currentCluster->getId() == cluster->getId())
			   {
               response.setField(VID_RCC, RCC_SUCCESS);
			   }
			   else
			   {
			      response.setField(VID_RCC, RCC_CLUSTER_MEMBER_ALREADY);
			   }
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

   sendMessage(response);
}

/**
 * Bind/unbind object
 */
void ClientSession::changeObjectBinding(const NXCPMessage& request, bool bind)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

   // Get parent and child objects
   shared_ptr<NetObj> pParent = FindObjectById(request.getFieldAsUInt32(VID_PARENT_ID));
   shared_ptr<NetObj> pChild = FindObjectById(request.getFieldAsUInt32(VID_CHILD_ID));

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
            if (bind)
            {
               // Prevent loops
               if (!pChild->isChild(pParent->getId()))
               {
                  pParent->addChild(pChild);
                  pChild->addParent(pParent);
                  pParent->calculateCompoundStatus();
                  response.setField(VID_RCC, RCC_SUCCESS);
               }
               else
               {
                  response.setField(VID_RCC, RCC_OBJECT_LOOP);
               }
            }
            else
            {
               pParent->deleteChild(*pChild);
               pChild->deleteParent(*pParent);
               pParent->calculateCompoundStatus();
               if ((pParent->getObjectClass() == OBJECT_TEMPLATE) && pChild->isDataCollectionTarget())
               {
                  static_cast<Template&>(*pParent).queueRemoveFromTarget(pChild->getId(), request.getFieldAsBoolean(VID_REMOVE_DCI));
               }
               else if ((pParent->getObjectClass() == OBJECT_CLUSTER) && (pChild->getObjectClass() == OBJECT_NODE))
               {
                  static_pointer_cast<Cluster>(pParent)->removeNode(static_pointer_cast<Node>(pChild));
               }
               response.setField(VID_RCC, RCC_SUCCESS);
            }
         }
         else
         {
            response.setField(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
         }
      }
      else
      {
         response.setField(VID_RCC, RCC_ACCESS_DENIED);
      }
   }
   else
   {
      response.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   sendMessage(response);
}

/**
 * Worker thread for object deletion
 */
static void DeleteObjectWorker(const shared_ptr<NetObj>& object)
{
	object->deleteObject();
}

/**
 * Delete object
 */
void ClientSession::deleteObject(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

   // Find object to be deleted
   shared_ptr<NetObj> object = FindObjectById(request.getFieldAsUInt32(VID_OBJECT_ID));
   if (object != nullptr)
   {
      // Check if it is a built-in object, like "Entire Network"
      if (object->getId() >= 10)  // FIXME: change to 100
      {
         // Check access rights
         if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_DELETE))
         {
				if ((object->getObjectClass() == OBJECT_ZONE) && !object->isEmpty())
				{
	            response.setField(VID_RCC, RCC_ZONE_NOT_EMPTY);
				}
				else if ((object->getObjectClass() == OBJECT_ASSET) && !ConfigReadBoolean(_T("Objects.Assets.AllowDeleteIfLinked"), false) && (static_cast<Asset&>(*object).getLinkedObjectId() != 0))
				{
               response.setField(VID_RCC, RCC_ASSET_IS_LINKED);
				}
				else
				{
               ThreadPoolExecute(g_clientThreadPool, DeleteObjectWorker, object);
               response.setField(VID_RCC, RCC_SUCCESS);
               WriteAuditLog(AUDIT_OBJECTS, TRUE, m_dwUserId, m_workstation, m_id, object->getId(), _T("Object %s deleted"), object->getName());
				}
         }
         else
         {
            response.setField(VID_RCC, RCC_ACCESS_DENIED);
            WriteAuditLog(AUDIT_OBJECTS, FALSE, m_dwUserId, m_workstation, m_id, object->getId(), _T("Access denied on delete object %s"), object->getName());
         }
      }
      else
      {
         response.setField(VID_RCC, RCC_ACCESS_DENIED);
         WriteAuditLog(AUDIT_OBJECTS, FALSE, m_dwUserId, m_workstation, m_id, object->getId(), _T("Access denied on delete object %s"), object->getName());
      }
   }
   else
   {
      response.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   sendMessage(response);
}

/**
 * Alarm update worker function (executed in thread pool)
 */
void ClientSession::alarmUpdateWorker(Alarm *alarm)
{
   NXCPMessage msg(CMD_ALARM_UPDATE, 0);
   alarm->fillMessage(&msg);
   m_mutexSendAlarms.lock();
   sendMessage(msg);
   m_mutexSendAlarms.unlock();
   delete alarm;
   decRefCount();
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
         incRefCount();
         TCHAR key[16];
         _sntprintf(key, 16, _T("ALRM-%d"), m_id);
         ThreadPoolExecuteSerialized(g_clientThreadPool, key, this, &ClientSession::alarmUpdateWorker, new Alarm(alarm, false, code));
      }
   }
}

/**
 * Send all alarms to client
 */
void ClientSession::getAlarms(const NXCPMessage& request)
{
   m_mutexSendAlarms.lock();
   SendAlarmsToClient(request.getId(), this);
   m_mutexSendAlarms.unlock();
}

/**
 * Get specific alarm object
 */
void ClientSession::getAlarm(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

   // Get alarm id and it's source object
   uint32_t alarmId = request.getFieldAsUInt32(VID_ALARM_ID);
   shared_ptr<NetObj> object = GetAlarmSourceObject(alarmId);
   if (object != nullptr)
   {
      // User should have "view alarm" right to the object
      if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ_ALARMS))
      {
         response.setField(VID_RCC, GetAlarm(alarmId, m_dwUserId, &response, this));
      }
      else
      {
         response.setField(VID_RCC, RCC_ACCESS_DENIED);
			WriteAuditLog(AUDIT_OBJECTS, FALSE, m_dwUserId, m_workstation, m_id, object->getId(), _T("Access denied on get alarm for object %s"), object->getName());
      }
   }
   else
   {
      // Normally, for existing alarms object will not be nullptr,
      // so we assume that alarm id is invalid
      response.setField(VID_RCC, RCC_INVALID_ALARM_ID);
   }

   sendMessage(response);
}

/**
 * Get all related events for specific alarm object
 */
void ClientSession::getAlarmEvents(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

   // Get alarm id and it's source object
   uint32_t alarmId = request.getFieldAsUInt32(VID_ALARM_ID);
   shared_ptr<NetObj> object = GetAlarmSourceObject(alarmId);
   if (object != nullptr)
   {
      // User should have "view alarm" right to the object and
		// system-wide "view event log" access
      if ((m_systemAccessRights & SYSTEM_ACCESS_VIEW_EVENT_LOG) && object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ_ALARMS))
      {
         response.setField(VID_RCC, GetAlarmEvents(alarmId, m_dwUserId, &response, this));
      }
      else
      {
         response.setField(VID_RCC, RCC_ACCESS_DENIED);
			WriteAuditLog(AUDIT_OBJECTS, FALSE, m_dwUserId, m_workstation, m_id, object->getId(), _T("Access denied on get alarm events for object %s"), object->getName());
      }
   }
   else
   {
      // Normally, for existing alarms object will not be nullptr,
      // so we assume that alarm id is invalid
      response.setField(VID_RCC, RCC_INVALID_ALARM_ID);
   }

   sendMessage(&response);
}

/**
 * Acknowledge alarm
 */
void ClientSession::acknowledgeAlarm(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

   // Get alarm id and it's source object
   uint32_t alarmId = 0;
   TCHAR hdref[MAX_HELPDESK_REF_LEN];
   shared_ptr<NetObj> object;
   bool byHelpdeskRef;
   if (request.isFieldExist(VID_HELPDESK_REF))
   {
      request.getFieldAsString(VID_HELPDESK_REF, hdref, MAX_HELPDESK_REF_LEN);
      object = GetAlarmSourceObject(hdref);
      byHelpdeskRef = true;
   }
   else
   {
      alarmId = request.getFieldAsUInt32(VID_ALARM_ID);
      object = GetAlarmSourceObject(alarmId);
      byHelpdeskRef = false;
   }
   if (object != nullptr)
   {
      // User should have "acknowledge alarm" right to the object
      if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_UPDATE_ALARMS))
      {
			response.setField(VID_RCC,
            byHelpdeskRef ?
            AckAlarmByHDRef(hdref, this, request.getFieldAsUInt16(VID_STICKY_FLAG) != 0, request.getFieldAsUInt32(VID_TIMESTAMP)) :
            AckAlarmById(alarmId, this, request.getFieldAsUInt16(VID_STICKY_FLAG) != 0, request.getFieldAsUInt32(VID_TIMESTAMP), false));
      }
      else
      {
         response.setField(VID_RCC, RCC_ACCESS_DENIED);
			WriteAuditLog(AUDIT_OBJECTS, FALSE, m_dwUserId, m_workstation, m_id, object->getId(), _T("Access denied on acknowledged alarm on object %s"), object->getName());
      }
   }
   else
   {
      // Normally, for existing alarms object will not be nullptr,
      // so we assume that alarm id is invalid
      response.setField(VID_RCC, RCC_INVALID_ALARM_ID);
   }

   sendMessage(response);
}

/**
 * Terminate bulk alarms
 */
void ClientSession::bulkResolveAlarms(const NXCPMessage& request, bool terminate)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

   IntegerArray<uint32_t> alarmIds, failIds, failCodes;
   request.getFieldAsInt32Array(VID_ALARM_ID_LIST, &alarmIds);

   ResolveAlarmsById(alarmIds, &failIds, &failCodes, this, terminate, false);
   response.setField(VID_RCC, RCC_SUCCESS);
   if (failIds.size() > 0)
   {
      response.setFieldFromInt32Array(VID_ALARM_ID_LIST, &failIds);
      response.setFieldFromInt32Array(VID_FAIL_CODE_LIST, &failCodes);
   }

   sendMessage(response);
}

/**
 * Resolve/Terminate alarm
 */
void ClientSession::resolveAlarm(const NXCPMessage& request, bool terminate)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

   // Get alarm id and its source object
   uint32_t alarmId = 0;
   TCHAR hdref[MAX_HELPDESK_REF_LEN];
   shared_ptr<NetObj> object;
   bool byHelpdeskRef;
   if (request.isFieldExist(VID_HELPDESK_REF))
   {
      request.getFieldAsString(VID_HELPDESK_REF, hdref, MAX_HELPDESK_REF_LEN);
      object = GetAlarmSourceObject(hdref);
      byHelpdeskRef = true;
   }
   else
   {
      alarmId = request.getFieldAsUInt32(VID_ALARM_ID);
      object = GetAlarmSourceObject(alarmId);
      byHelpdeskRef = false;
   }
   if (object != nullptr)
   {
      // User should have "terminate alarm" right to the object
      if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_TERM_ALARMS))
      {
         response.setField(VID_RCC,
            byHelpdeskRef ?
            ResolveAlarmByHDRef(hdref, this, terminate) :
            ResolveAlarmById(alarmId, this, terminate, false));
      }
      else
      {
         response.setField(VID_RCC, RCC_ACCESS_DENIED);
			WriteAuditLog(AUDIT_OBJECTS, FALSE, m_dwUserId, m_workstation, m_id, object->getId(),
            _T("Access denied on %s alarm on object %s"), terminate ? _T("terminate") : _T("resolve"), object->getName());
      }
   }
   else
   {
      // Normally, for existing alarms object will not be nullptr,
      // so we assume that alarm id is invalid
      response.setField(VID_RCC, RCC_INVALID_ALARM_ID);
   }

   sendMessage(response);
}

/**
 * Delete alarm
 */
void ClientSession::deleteAlarm(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

   // Get alarm id and it's source object
   uint32_t alarmId = request.getFieldAsUInt32(VID_ALARM_ID);
   shared_ptr<NetObj> object = GetAlarmSourceObject(alarmId);
   if (object != nullptr)
   {
      // User should have "terminate alarm" right to the object
      // and system right "delete alarms"
      if ((object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_TERM_ALARMS)) &&
          (m_systemAccessRights & SYSTEM_ACCESS_DELETE_ALARMS))
      {
         DeleteAlarm(alarmId, false);
         response.setField(VID_RCC, RCC_SUCCESS);
      }
      else
      {
         response.setField(VID_RCC, RCC_ACCESS_DENIED);
			WriteAuditLog(AUDIT_OBJECTS, FALSE, m_dwUserId, m_workstation, m_id, object->getId(), _T("Access denied on delete alarm on object %s"), object->getName());
      }
   }
   else
   {
      // Normally, for existing alarms object will not be nullptr,
      // so we assume that alarm id is invalid
      response.setField(VID_RCC, RCC_INVALID_ALARM_ID);
   }

   sendMessage(response);
}

/**
 * Open issue in helpdesk system from given alarm
 */
void ClientSession::openHelpdeskIssue(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

   // Get alarm id and it's source object
   uint32_t alarmId = request.getFieldAsUInt32(VID_ALARM_ID);
   shared_ptr<NetObj> object = GetAlarmSourceObject(alarmId);
   if (object != nullptr)
   {
      if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_CREATE_ISSUE))
      {
         TCHAR hdref[MAX_HELPDESK_REF_LEN];
         response.setField(VID_RCC, OpenHelpdeskIssue(alarmId, this, hdref));
         response.setField(VID_HELPDESK_REF, hdref);
         WriteAuditLog(AUDIT_OBJECTS, TRUE, m_dwUserId, m_workstation, m_id, object->getId(),
            _T("Helpdesk issue created successfully from alarm on object %s"), object->getName());
      }
      else
      {
         response.setField(VID_RCC, RCC_ACCESS_DENIED);
			WriteAuditLog(AUDIT_OBJECTS, FALSE, m_dwUserId, m_workstation, m_id, object->getId(),
            _T("Access denied on creating issue from alarm on object %s"), object->getName());
      }
   }
   else
   {
      // Normally, for existing alarms object will not be nullptr,
      // so we assume that alarm id is invalid
      response.setField(VID_RCC, RCC_INVALID_ALARM_ID);
   }

   sendMessage(response);
}

/**
 * Get helpdesk URL for given alarm
 */
void ClientSession::getHelpdeskUrl(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

   // Get alarm id and it's source object
   uint32_t alarmId = request.getFieldAsUInt32(VID_ALARM_ID);
   shared_ptr<NetObj> object = GetAlarmSourceObject(alarmId);
   if (object != nullptr)
   {
      if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ_ALARMS))
      {
         TCHAR url[MAX_PATH];
         response.setField(VID_RCC, GetHelpdeskIssueUrlFromAlarm(alarmId, m_dwUserId, url, MAX_PATH, this));
         response.setField(VID_URL, url);
      }
      else
      {
         response.setField(VID_RCC, RCC_ACCESS_DENIED);
			WriteAuditLog(AUDIT_OBJECTS, FALSE, m_dwUserId, m_workstation, m_id, object->getId(),
            _T("Access denied on getting helpdesk URL for alarm on object %s"), object->getName());
      }
   }
   else
   {
      // Normally, for existing alarms object will not be nullptr,
      // so we assume that alarm id is invalid
      response.setField(VID_RCC, RCC_INVALID_ALARM_ID);
   }

   sendMessage(response);
}

/**
 * Unlink helpdesk issue from alarm
 */
void ClientSession::unlinkHelpdeskIssue(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

   // Get alarm id and it's source object
   uint32_t alarmId = 0;
   TCHAR hdref[MAX_HELPDESK_REF_LEN];
   shared_ptr<NetObj> object;
   bool byHelpdeskRef;
   if (request.isFieldExist(VID_HELPDESK_REF))
   {
      request.getFieldAsString(VID_HELPDESK_REF, hdref, MAX_HELPDESK_REF_LEN);
      object = GetAlarmSourceObject(hdref);
      byHelpdeskRef = true;
   }
   else
   {
      alarmId = request.getFieldAsUInt32(VID_ALARM_ID);
      object = GetAlarmSourceObject(alarmId);
      byHelpdeskRef = false;
   }
   if (object != nullptr)
   {
      // User should have "update alarms" right to the object and "unlink issues" global right
      if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_UPDATE_ALARMS) && checkSysAccessRights(SYSTEM_ACCESS_UNLINK_ISSUES))
      {
         response.setField(VID_RCC,
            byHelpdeskRef ?
            UnlinkHelpdeskIssueByHDRef(hdref, this) :
            UnlinkHelpdeskIssueById(alarmId, this));
      }
      else
      {
         response.setField(VID_RCC, RCC_ACCESS_DENIED);
			WriteAuditLog(AUDIT_OBJECTS, FALSE, m_dwUserId, m_workstation, m_id, object->getId(),
            _T("Access denied on unlinking helpdesk issue from alarm on object %s"), object->getName());
      }
   }
   else
   {
      // Normally, for existing alarms object will not be nullptr,
      // so we assume that alarm id is invalid
      response.setField(VID_RCC, RCC_INVALID_ALARM_ID);
   }

   sendMessage(response);
}

/**
 * Get comments for given alarm
 */
void ClientSession::getAlarmComments(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

   // Get alarm id and it's source object
   uint32_t alarmId = request.getFieldAsUInt32(VID_ALARM_ID);
   shared_ptr<NetObj> object = GetAlarmSourceObject(alarmId);
   if (object != nullptr)
   {
      // User should have "view alarms" right to the object
		if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ_ALARMS))
      {
			response.setField(VID_RCC, GetAlarmComments(alarmId, &response));
      }
      else
      {
         response.setField(VID_RCC, RCC_ACCESS_DENIED);
      }
   }
   else
   {
      // Normally, for existing alarms object will not be nullptr,
      // so we assume that alarm id is invalid
      response.setField(VID_RCC, RCC_INVALID_ALARM_ID);
   }

   sendMessage(response);
}

/**
 * Update alarm comment
 */
void ClientSession::updateAlarmComment(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

   // Get alarm id and it's source object
   uint32_t alarmId = 0;
   TCHAR hdref[MAX_HELPDESK_REF_LEN];
   shared_ptr<NetObj> object;
   bool byHelpdeskRef;
   if (request.isFieldExist(VID_HELPDESK_REF))
   {
      request.getFieldAsString(VID_HELPDESK_REF, hdref, MAX_HELPDESK_REF_LEN);
      object = GetAlarmSourceObject(hdref);
      byHelpdeskRef = true;
   }
   else
   {
      alarmId = request.getFieldAsUInt32(VID_ALARM_ID);
      object = GetAlarmSourceObject(alarmId);
      byHelpdeskRef = false;
   }
   if (object != nullptr)
   {
      // User should have "update alarm" right to the object
		if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_UPDATE_ALARMS))
      {
			uint32_t commentId = request.getFieldAsUInt32(VID_COMMENT_ID);
			TCHAR *text = request.getFieldAsString(VID_COMMENTS);
			response.setField(VID_RCC,
            byHelpdeskRef ?
            AddAlarmComment(hdref, CHECK_NULL(text), m_dwUserId) :
            UpdateAlarmComment(alarmId, &commentId, CHECK_NULL(text), m_dwUserId));
			MemFree(text);
      }
      else
      {
         response.setField(VID_RCC, RCC_ACCESS_DENIED);
      }
   }
   else
   {
      // Normally, for existing alarms object will not be nullptr,
      // so we assume that alarm id is invalid
      response.setField(VID_RCC, RCC_INVALID_ALARM_ID);
   }

   sendMessage(response);
}

/**
 * Delete alarm comment
 */
void ClientSession::deleteAlarmComment(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

   // Get alarm id and it's source object
   uint32_t alarmId = request.getFieldAsUInt32(VID_ALARM_ID);
   shared_ptr<NetObj> object = GetAlarmSourceObject(alarmId);
   if (object != nullptr)
   {
      // User should have "acknowledge alarm" right to the object
		if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_UPDATE_ALARMS))
      {
			UINT32 commentId = request.getFieldAsUInt32(VID_COMMENT_ID);
			response.setField(VID_RCC, DeleteAlarmCommentByID(alarmId, commentId));
      }
      else
      {
         response.setField(VID_RCC, RCC_ACCESS_DENIED);
      }
   }
   else
   {
      // Normally, for existing alarms object will not be nullptr,
      // so we assume that alarm id is invalid
      response.setField(VID_RCC, RCC_INVALID_ALARM_ID);
   }

   sendMessage(response);
}

/**
 * Update alarm status flow mode
 */
void ClientSession::updateAlarmStatusFlow(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());
   int status = request.getFieldAsUInt32(VID_ALARM_STATUS_FLOW_STATE);

   ConfigWriteInt(_T("Alarms.StrictStatusFlow"), status, false);
   response.setField(VID_RCC, RCC_SUCCESS);

   sendMessage(response);
}

/**
 * Create new server action
 */
void ClientSession::createAction(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

   if (m_systemAccessRights & SYSTEM_ACCESS_MANAGE_ACTIONS)
   {
      TCHAR actionName[MAX_OBJECT_NAME];
      request.getFieldAsString(VID_ACTION_NAME, actionName, MAX_OBJECT_NAME);
      uint32_t actionId;
      uint32_t rcc = CreateAction(actionName, &actionId);
      response.setField(VID_RCC, rcc);
      if (rcc == RCC_SUCCESS)
         response.setField(VID_ACTION_ID, actionId);   // Send id of new action to client
   }
   else
   {
      response.setField(VID_RCC, RCC_ACCESS_DENIED);
   }

   sendMessage(response);
}

/**
 * Update existing action's data
 */
void ClientSession::updateAction(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());
   if (m_systemAccessRights & SYSTEM_ACCESS_MANAGE_ACTIONS)
   {
      response.setField(VID_RCC, ModifyActionFromMessage(request));
   }
   else
   {
      response.setField(VID_RCC, RCC_ACCESS_DENIED);
   }
   sendMessage(response);
}

/**
 * Delete action
 */
void ClientSession::deleteAction(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

   if (m_systemAccessRights & SYSTEM_ACCESS_MANAGE_ACTIONS)
   {
      // Get Id of action to be deleted
      uint32_t actionId = request.getFieldAsUInt32(VID_ACTION_ID);
      if (!g_pEventPolicy->isActionInUse(actionId))
      {
         response.setField(VID_RCC, DeleteAction(actionId));
      }
      else
      {
         response.setField(VID_RCC, RCC_ACTION_IN_USE);
      }
   }
   else
   {
      response.setField(VID_RCC, RCC_ACCESS_DENIED);
   }

   sendMessage(response);
}

/**
 * Send action configuration update message
 */
void ClientSession::sendActionDBUpdateMessage(NXCP_MESSAGE *msg)
{
   m_mutexSendActions.lock();
   sendRawMessage(msg);
   m_mutexSendActions.unlock();
   MemFree(msg);
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
void ClientSession::sendAllActions(const NXCPMessage& request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request.getId());

   if ((m_systemAccessRights & SYSTEM_ACCESS_MANAGE_ACTIONS) || (m_systemAccessRights & SYSTEM_ACCESS_EPP))
   {
      msg.setField(VID_RCC, RCC_SUCCESS);
      sendMessage(msg);
      m_mutexSendActions.lock();
      SendActionsToClient(this, request.getId());
      m_mutexSendActions.unlock();
   }
   else
   {
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);
      sendMessage(msg);
   }
}

/**
 * Perform a forced object poll
 */
void ClientSession::forcedObjectPoll(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

   bool isValidPoll = false;
   int pollType = request.getFieldAsUInt16(VID_POLL_TYPE);

   // Find object to be polled
   Pollable *pollableObject;
   shared_ptr<NetObj> object = FindObjectById(request.getFieldAsUInt32(VID_OBJECT_ID));
   if (object != nullptr)
   {
      pollableObject = object->getAsPollable();
      if (pollableObject != nullptr)
      {
         if (pollType == POLL_STATUS)
            isValidPoll = pollableObject->isStatusPollAvailable();
         else if (pollType == POLL_CONFIGURATION_FULL)
            isValidPoll = (pollableObject->isConfigurationPollAvailable() && object->getObjectClass() == OBJECT_NODE);
         else if (pollType == POLL_CONFIGURATION_NORMAL)
            isValidPoll = pollableObject->isConfigurationPollAvailable();
         else if (pollType == POLL_INSTANCE_DISCOVERY)
            isValidPoll = pollableObject->isInstanceDiscoveryPollAvailable();
         else if (pollType == POLL_TOPOLOGY)
            isValidPoll = pollableObject->isTopologyPollAvailable();
         else if (pollType == POLL_ROUTING_TABLE)
            isValidPoll = pollableObject->isRoutingTablePollAvailable();
         else if (pollType == POLL_DISCOVERY)
            isValidPoll = pollableObject->isDiscoveryPollAvailable();
         else if (pollType == POLL_AUTOBIND)
            isValidPoll = pollableObject->isAutobindPollAvailable();
         else if (pollType == POLL_INTERFACE_NAMES)
            isValidPoll = (object->getObjectClass() == OBJECT_NODE);
      }
      if (isValidPoll)
      {
         // Check access rights
         if (((pollType == POLL_CONFIGURATION_FULL) && object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_MODIFY)) ||
               ((pollType != POLL_CONFIGURATION_FULL) && object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ)))
         {
            sendPollerMsg(request.getId(), _T("Poll request accepted, waiting for outstanding polling requests to complete...\r\n"));
            response.setField(VID_RCC, RCC_SUCCESS);
         }
         else
         {
            isValidPoll = false;
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

   sendMessage(response);

   if (isValidPoll)
   {
      switch(pollType)
      {
         case POLL_STATUS:
            pollableObject->doForcedStatusPoll(RegisterPoller(PollerType::STATUS, object), this, request.getId());
            break;
         case POLL_CONFIGURATION_FULL:
            if (object->getObjectClass() == OBJECT_NODE)
               static_cast<Node&>(*object).setRecheckCapsFlag();
            // intentionally no break here
         case POLL_CONFIGURATION_NORMAL:
            pollableObject->doForcedConfigurationPoll(RegisterPoller(PollerType::CONFIGURATION, object), this, request.getId());
            break;
         case POLL_INSTANCE_DISCOVERY:
            pollableObject->doForcedInstanceDiscoveryPoll(RegisterPoller(PollerType::INSTANCE_DISCOVERY, object), this, request.getId());
            break;
         case POLL_TOPOLOGY:
            pollableObject->doForcedTopologyPoll(RegisterPoller(PollerType::TOPOLOGY, object), this, request.getId());
            break;
         case POLL_ROUTING_TABLE:
            pollableObject->doForcedRoutingTablePoll(RegisterPoller(PollerType::ROUTING_TABLE, object), this, request.getId());
            break;
         case POLL_DISCOVERY:
            pollableObject->doForcedDiscoveryPoll(RegisterPoller(PollerType::DISCOVERY, object), this, request.getId());
            break;
         case POLL_AUTOBIND:
            pollableObject->doForcedAutobindPoll(RegisterPoller(PollerType::AUTOBIND, object), this, request.getId());
            break;
         case POLL_INTERFACE_NAMES:
            if (object->getObjectClass() == OBJECT_NODE)
            {
               static_cast<Node&>(*object).updateInterfaceNames(this, request.getId());
            }
            break;
         default:
            sendPollerMsg(request.getId(), _T("Invalid poll type requested\r\n"));
            break;
      }

      NXCPMessage msg(CMD_POLLING_INFO, request.getId());
      msg.setField(VID_RCC, RCC_SUCCESS);
      sendMessage(msg);
   }
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
 * Receive event from user
 */
void ClientSession::onTrap(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

   // Find event's source object
   uint32_t objectId = request.getFieldAsUInt32(VID_OBJECT_ID);
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
         uint32_t eventCode = request.getFieldAsUInt32(VID_EVENT_CODE);
			if ((eventCode == 0) && request.isFieldExist(VID_EVENT_NAME))
			{
				TCHAR eventName[256];
				request.getFieldAsString(VID_EVENT_NAME, eventName, 256);
				eventCode = EventCodeFromName(eventName, 0);
			}
		   TCHAR userTag[MAX_USERTAG_LENGTH] = _T("");
			request.getFieldAsString(VID_USER_TAG, userTag, MAX_USERTAG_LENGTH);
			Trim(userTag);
			StringList parameters(request, VID_EVENT_ARG_BASE, VID_NUM_ARGS);
         bool success = PostEventWithTag(eventCode, EventOrigin::CLIENT, request.getFieldAsTime(VID_ORIGIN_TIMESTAMP), object->getId(), userTag, parameters);
         response.setField(VID_RCC, success ? RCC_SUCCESS : RCC_INVALID_EVENT_CODE);
      }
      else
      {
         response.setField(VID_RCC, RCC_ACCESS_DENIED);
      }
   }
   else
   {
      response.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   sendMessage(&response);
}

/**
 * Wake up node
 */
void ClientSession::onWakeUpNode(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

   // Find node or interface object
   shared_ptr<NetObj> object = FindObjectById(request.getFieldAsUInt32(VID_OBJECT_ID));
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
            response.setField(VID_RCC, dwResult);
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
}

/**
 * Query specific parameter from node
 */
void ClientSession::queryParameter(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

   // Find node object
   shared_ptr<NetObj> object = FindObjectById(request.getFieldAsUInt32(VID_OBJECT_ID));
   if (object != nullptr)
   {
      if (object->getObjectClass() == OBJECT_NODE)
      {
         TCHAR value[256], name[MAX_PARAM_NAME];
         request.getFieldAsString(VID_NAME, name, MAX_PARAM_NAME);
         uint32_t rcc = static_cast<Node&>(*object).getMetricForClient(request.getFieldAsUInt16(VID_DCI_SOURCE_TYPE), m_dwUserId, name, value, 256);
         response.setField(VID_RCC, rcc);
         if (rcc == RCC_SUCCESS)
            response.setField(VID_VALUE, value);
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
}

/**
 * Query specific table from node
 */
void ClientSession::queryAgentTable(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

   // Find node object
   shared_ptr<NetObj> object = FindObjectById(request.getFieldAsUInt32(VID_OBJECT_ID));
   if (object != nullptr)
   {
      if (object->getObjectClass() == OBJECT_NODE)
      {
         TCHAR name[MAX_PARAM_NAME];
         request.getFieldAsString(VID_NAME, name, MAX_PARAM_NAME);

         // Allow access to agent table Agent.SessionAgents if user has access rights for taking screenshots
         // Data from this table required by client to determine correct UI session name
         if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ_AGENT) ||
             (!_tcsicmp(name, _T("Agent.SessionAgents")) && object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_SCREENSHOT)))
         {
				shared_ptr<Table> table;
				uint32_t rcc = static_cast<Node&>(*object).getTableForClient(name, &table);
				response.setField(VID_RCC, rcc);
				if (rcc == RCC_SUCCESS)
				{
					table->fillMessage(&response, 0, -1);
				}
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
}

/**
 * Edit trap configuration record
 */
void ClientSession::editTrap(const NXCPMessage& request, int operation)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

   // Check access rights
   if (checkSysAccessRights(SYSTEM_ACCESS_CONFIGURE_TRAPS))
   {
      uint32_t trapId, rcc;
      switch(operation)
      {
         case TRAP_CREATE:
            rcc = CreateNewTrap(&trapId);
            response.setField(VID_RCC, rcc);
            if (rcc == RCC_SUCCESS)
               response.setField(VID_TRAP_ID, trapId);   // Send id of new trap to client
            break;
         case TRAP_UPDATE:
            response.setField(VID_RCC, UpdateTrapFromMsg(request));
            break;
         case TRAP_DELETE:
            trapId = request.getFieldAsUInt32(VID_TRAP_ID);
            response.setField(VID_RCC, DeleteTrap(trapId));
            break;
         default:
				response.setField(VID_RCC, RCC_INVALID_ARGUMENT);
            break;
      }
   }
   else
   {
      // Current user has no rights for trap management
      response.setField(VID_RCC, RCC_ACCESS_DENIED);
   }

   sendMessage(response);
}

/**
 * Send all trap configuration records to client
 */
void ClientSession::sendAllTraps(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());
	if (checkSysAccessRights(SYSTEM_ACCESS_CONFIGURE_TRAPS))
   {
      response.setField(VID_RCC, RCC_SUCCESS);
      sendMessage(response);
      SendTrapsToClient(this, request.getId());
   }
   else
   {
      // Current user has no rights for trap management
      response.setField(VID_RCC, RCC_ACCESS_DENIED);
      sendMessage(response);
   }
}

/**
 * Send all trap configuration records to client
 * Send all in one message with less information and don't need a lock
 */
void ClientSession::sendAllTraps2(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());
	if (checkSysAccessRights(SYSTEM_ACCESS_CONFIGURE_TRAPS))
   {
      response.setField(VID_RCC, RCC_SUCCESS);
      CreateTrapCfgMessage(&response);
   }
   else
   {
      // Current user has no rights for trap management
      response.setField(VID_RCC, RCC_ACCESS_DENIED);
   }
   sendMessage(response);
}

/**
 * Send list of all installed packages to client
 */
void ClientSession::getInstalledPackages(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

   bool success = false;
   if (m_systemAccessRights & SYSTEM_ACCESS_MANAGE_PACKAGES)
   {
      DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
      DB_UNBUFFERED_RESULT hResult = DBSelectUnbuffered(hdb, _T("SELECT pkg_id,version,platform,pkg_file,pkg_type,pkg_name,description,command FROM agent_pkg"));
      if (hResult != nullptr)
      {
         response.setField(VID_RCC, RCC_SUCCESS);
         sendMessage(response);
         success = true;

         response.setCode(CMD_PACKAGE_INFO);
         response.deleteAllFields();

         TCHAR buffer[MAX_DB_STRING];
         while(DBFetch(hResult))
         {
            response.setField(VID_PACKAGE_ID, DBGetFieldULong(hResult, 0));
            response.setField(VID_PACKAGE_VERSION, DBGetField(hResult, 1, buffer, MAX_DB_STRING));
            response.setField(VID_PLATFORM_NAME, DBGetField(hResult, 2, buffer, MAX_DB_STRING));
            response.setField(VID_FILE_NAME, DBGetField(hResult, 3, buffer, MAX_DB_STRING));
            response.setField(VID_PACKAGE_TYPE, DBGetField(hResult, 4, buffer, MAX_DB_STRING));
            response.setField(VID_PACKAGE_NAME, DBGetField(hResult, 5, buffer, MAX_DB_STRING));
            response.setField(VID_DESCRIPTION, DBGetField(hResult, 6, buffer, MAX_DB_STRING));
            response.setField(VID_COMMAND, DBGetField(hResult, 7, buffer, MAX_DB_STRING));
            sendMessage(response);
            response.deleteAllFields();
         }

         response.setField(VID_PACKAGE_ID, (UINT32)0);
         sendMessage(response);

         DBFreeResult(hResult);
      }
      else
      {
         response.setField(VID_RCC, RCC_DB_FAILURE);
      }
      DBConnectionPoolReleaseConnection(hdb);
   }
   else
   {
      // Current user has no rights for package management
      response.setField(VID_RCC, RCC_ACCESS_DENIED);
   }

   if (!success)
      sendMessage(response);
}

/**
 * Install package to server
 */
void ClientSession::installPackage(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

   if (m_systemAccessRights & SYSTEM_ACCESS_MANAGE_PACKAGES)
   {
      TCHAR packageName[MAX_PACKAGE_NAME_LEN], description[MAX_DB_STRING];
      TCHAR packageVersion[MAX_AGENT_VERSION_LEN], packageType[16], fileName[MAX_DB_STRING];
      TCHAR platform[MAX_PLATFORM_NAME_LEN], command[MAX_DB_STRING];

      request.getFieldAsString(VID_PACKAGE_NAME, packageName, MAX_PACKAGE_NAME_LEN);
      request.getFieldAsString(VID_DESCRIPTION, description, MAX_DB_STRING);
      request.getFieldAsString(VID_FILE_NAME, fileName, MAX_DB_STRING);
      request.getFieldAsString(VID_PACKAGE_VERSION, packageVersion, MAX_AGENT_VERSION_LEN);
      request.getFieldAsString(VID_PACKAGE_TYPE, packageType, 16);
      request.getFieldAsString(VID_PLATFORM_NAME, platform, MAX_PLATFORM_NAME_LEN);
      request.getFieldAsString(VID_COMMAND, command, MAX_DB_STRING);

      // Remove possible path specification from file name
      const TCHAR *cleanFileName = GetCleanFileName(fileName);

      if (IsValidObjectName(cleanFileName, true) && IsValidObjectName(packageName))
      {
         // Check if same package already exist
         if (!IsPackageInstalled(packageName, packageVersion, platform))
         {
            // Check for duplicate file name
            if (!IsPackageFileExist(cleanFileName))
            {
               TCHAR fullFileName[MAX_PATH];
               _tcscpy(fullFileName, g_netxmsdDataDir);
               _tcscat(fullFileName, DDIR_PACKAGES);
               _tcscat(fullFileName, FS_PATH_SEPARATOR);
               _tcscat(fullFileName, cleanFileName);

               ServerDownloadFileInfo *fInfo = new ServerDownloadFileInfo(fullFileName,
                  [] (const TCHAR *fileName, uint32_t uploadData, bool success) -> void
                  {
                     if (!success)
                     {
                        DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
                        ExecuteQueryOnObject(hdb, uploadData, _T("DELETE FROM agent_pkg WHERE pkg_id=?"));
                        DBConnectionPoolReleaseConnection(hdb);
                     }
                  });
               if (fInfo->open())
               {
                  uint32_t packageId = CreateUniqueId(IDG_PACKAGE);
                  fInfo->setUploadData(packageId);
                  m_downloadFileMap.set(request.getId(), fInfo);
                  response.setField(VID_RCC, RCC_SUCCESS);
                  response.setField(VID_PACKAGE_ID, packageId);

                  // Create record in database
                  fInfo->updatePackageDBInfo(description, packageName, packageVersion, packageType, platform, cleanFileName, command);
               }
               else
               {
                  delete fInfo;
                  response.setField(VID_RCC, RCC_IO_ERROR);
               }
            }
            else
            {
               response.setField(VID_RCC, RCC_PACKAGE_FILE_EXIST);
            }
         }
         else
         {
            response.setField(VID_RCC, RCC_DUPLICATE_PACKAGE);
         }
      }
      else
      {
         response.setField(VID_RCC, RCC_INVALID_OBJECT_NAME);
      }
   }
   else
   {
      // Current user has no rights for package management
      response.setField(VID_RCC, RCC_ACCESS_DENIED);
   }

   // Send response
   sendMessage(&response);
}

/**
 * Update metadata for already installed package
 */
void ClientSession::updatePackageMetadata(const NXCPMessage& request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request.getId());

   if (m_systemAccessRights & SYSTEM_ACCESS_MANAGE_PACKAGES)
   {
      TCHAR packageName[MAX_PACKAGE_NAME_LEN], description[MAX_DB_STRING];
      TCHAR packageVersion[MAX_AGENT_VERSION_LEN], packageType[16];
      TCHAR platform[MAX_PLATFORM_NAME_LEN], command[MAX_DB_STRING];

      request.getFieldAsString(VID_PACKAGE_NAME, packageName, MAX_PACKAGE_NAME_LEN);
      request.getFieldAsString(VID_DESCRIPTION, description, MAX_DB_STRING);
      request.getFieldAsString(VID_PACKAGE_VERSION, packageVersion, MAX_AGENT_VERSION_LEN);
      request.getFieldAsString(VID_PACKAGE_TYPE, packageType, 16);
      request.getFieldAsString(VID_PLATFORM_NAME, platform, MAX_PLATFORM_NAME_LEN);
      request.getFieldAsString(VID_COMMAND, command, MAX_DB_STRING);

      uint32_t packageId = request.getFieldAsUInt32(VID_PACKAGE_ID);
      if (IsValidPackageId(packageId))
      {
         if (IsValidObjectName(packageName))
         {
            bool success = false;
            DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
            DB_STATEMENT hStmt = DBPrepare(hdb, _T("UPDATE agent_pkg SET pkg_name=?,version=?,description=?,platform=?,pkg_type=?,command=? WHERE pkg_id=?"));
            if (hStmt != nullptr)
            {
               DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, packageName, DB_BIND_STATIC);
               DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, packageVersion, DB_BIND_STATIC);
               DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, description, DB_BIND_STATIC);
               DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, platform, DB_BIND_STATIC);
               DBBind(hStmt, 5, DB_SQLTYPE_VARCHAR, packageType, DB_BIND_STATIC);
               DBBind(hStmt, 6, DB_SQLTYPE_VARCHAR, command, DB_BIND_STATIC);
               DBBind(hStmt, 7, DB_SQLTYPE_INTEGER, packageId);
               success = DBExecute(hStmt);
               DBFreeStatement(hStmt);
            }
            DBConnectionPoolReleaseConnection(hdb);
            msg.setField(VID_RCC, success ? RCC_SUCCESS : RCC_DB_FAILURE);
         }
         else
         {
            msg.setField(VID_RCC, RCC_INVALID_OBJECT_NAME);
         }
      }
      else
      {
         msg.setField(VID_RCC, RCC_INVALID_PACKAGE_ID);
      }
   }
   else
   {
      // Current user has no rights for package management
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);
   }

   sendMessage(msg);
}

/**
 * Remove package from server
 */
void ClientSession::removePackage(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

   if (m_systemAccessRights & SYSTEM_ACCESS_MANAGE_PACKAGES)
   {
      uint32_t pkgId = request.getFieldAsUInt32(VID_PACKAGE_ID);
      response.setField(VID_RCC, UninstallPackage(pkgId));
   }
   else
   {
      // Current user has no rights for package management
      response.setField(VID_RCC, RCC_ACCESS_DENIED);
   }

   sendMessage(response);
}

/**
 * Get list of parameters supported by given node
 */
void ClientSession::getParametersList(const NXCPMessage& request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request.getId());

   shared_ptr<NetObj> object = FindObjectById(request.getFieldAsUInt32(VID_OBJECT_ID));
   if (object != nullptr)
   {
      int origin = request.isFieldExist(VID_DCI_SOURCE_TYPE) ? request.getFieldAsInt16(VID_DCI_SOURCE_TYPE) : DS_NATIVE_AGENT;
      switch(object->getObjectClass())
      {
         case OBJECT_NODE:
            msg.setField(VID_RCC, RCC_SUCCESS);
				static_cast<Node&>(*object).writeParamListToMessage(&msg, origin, request.getFieldAsUInt16(VID_FLAGS));
            break;
         case OBJECT_CLUSTER:
         case OBJECT_TEMPLATE:
            msg.setField(VID_RCC, RCC_SUCCESS);
            WriteFullParamListToMessage(&msg, origin, request.getFieldAsUInt16(VID_FLAGS));
            break;
         case OBJECT_CHASSIS:
            if (static_cast<Chassis&>(*object).getControllerId() != 0)
            {
               shared_ptr<NetObj> controller = FindObjectById(static_cast<Chassis&>(*object).getControllerId(), OBJECT_NODE);
               if (controller != nullptr)
                  static_cast<Node&>(*controller).writeParamListToMessage(&msg, origin, request.getFieldAsUInt16(VID_FLAGS));
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
void ClientSession::deployPackage(const NXCPMessage& request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request.getId());
   PackageDeploymentTask *task = nullptr;

   if (m_systemAccessRights & SYSTEM_ACCESS_MANAGE_PACKAGES)
   {
      bool success = true;

      // Get package ID
      uint32_t packageId = request.getFieldAsUInt32(VID_PACKAGE_ID);
      if (IsValidPackageId(packageId))
      {
         DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

         // Read package information
         TCHAR query[256];
         _sntprintf(query, 256, _T("SELECT platform,pkg_file,pkg_type,version,command FROM agent_pkg WHERE pkg_id=%u"), packageId);
         DB_RESULT hResult = DBSelect(hdb, query);
         if (hResult != nullptr)
         {
            if (DBGetNumRows(hResult) > 0)
            {
               TCHAR packageFile[256], packageType[16], version[MAX_AGENT_VERSION_LEN], platform[MAX_PLATFORM_NAME_LEN], command[256];
               DBGetField(hResult, 0, 0, platform, MAX_PLATFORM_NAME_LEN);
               DBGetField(hResult, 0, 1, packageFile, 256);
               DBGetField(hResult, 0, 2, packageType, 16);
               DBGetField(hResult, 0, 3, version, MAX_AGENT_VERSION_LEN);
               DBGetField(hResult, 0, 4, command, 256);

               // Create list of nodes to be upgraded
               IntegerArray<uint32_t> objectList;
               request.getFieldAsInt32Array(VID_OBJECT_LIST, &objectList);
		         task = new PackageDeploymentTask(this, request.getId(), packageId, platform, packageFile, packageType, version, command);
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
               msg.setField(VID_RCC, RCC_INVALID_PACKAGE_ID);
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
   sendMessage(msg);

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
void ClientSession::applyTemplate(const NXCPMessage& request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request.getId());

   // Get source and destination
   shared_ptr<NetObj> pSource = FindObjectById(request.getFieldAsUInt32(VID_SOURCE_OBJECT_ID));
   shared_ptr<NetObj> pDestination = FindObjectById(request.getFieldAsUInt32(VID_DESTINATION_OBJECT_ID));
   if ((pSource != nullptr) && (pDestination != nullptr))
   {
      // Check object types
      if ((pSource->getObjectClass() == OBJECT_TEMPLATE) && pDestination->isDataCollectionTarget())
      {
         // Check access rights
         if ((pSource->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ)) &&
             (pDestination->checkAccessRights(m_dwUserId, OBJECT_ACCESS_MODIFY)))
         {
            bool bErrors = static_cast<Template&>(*pSource).applyToTarget(static_pointer_cast<DataCollectionTarget>(pDestination));
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
   sendMessage(msg);
}

/**
 * Get user variable
 */
void ClientSession::getUserVariable(const NXCPMessage& request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request.getId());

   uint32_t dwUserId = request.isFieldExist(VID_USER_ID) ? request.getFieldAsUInt32(VID_USER_ID) : m_dwUserId;
   if ((dwUserId == m_dwUserId) || (m_systemAccessRights & SYSTEM_ACCESS_MANAGE_USERS))
   {
      DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

      // Try to read variable from database
      DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT var_value FROM user_profiles WHERE user_id=? AND var_name=?"));
      if (hStmt != nullptr)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, dwUserId);
         DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, request.getFieldAsString(VID_NAME, nullptr, 0), DB_BIND_DYNAMIC);
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
void ClientSession::setUserVariable(const NXCPMessage& request)
{
   NXCPMessage msg;
   TCHAR szVarName[MAX_USERVAR_NAME_LENGTH];
   DB_RESULT hResult;
   BOOL bExist = FALSE;
   UINT32 dwUserId;

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(request.getId());

   dwUserId = request.isFieldExist(VID_USER_ID) ? request.getFieldAsUInt32(VID_USER_ID) : m_dwUserId;
   if ((dwUserId == m_dwUserId) || (m_systemAccessRights & SYSTEM_ACCESS_MANAGE_USERS))
   {
      // Check variable name
      request.getFieldAsString(VID_NAME, szVarName, MAX_USERVAR_NAME_LENGTH);
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
            DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, request.getFieldAsString(VID_VALUE), DB_BIND_DYNAMIC);

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
void ClientSession::enumUserVariables(const NXCPMessage& request)
{
   NXCPMessage msg;
   TCHAR szPattern[MAX_USERVAR_NAME_LENGTH], szQuery[256], szName[MAX_DB_STRING];
   UINT32 i, dwNumRows, dwNumVars, dwId, dwUserId;
   DB_RESULT hResult;

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(request.getId());

   dwUserId = request.isFieldExist(VID_USER_ID) ? request.getFieldAsUInt32(VID_USER_ID) : m_dwUserId;
   if ((dwUserId == m_dwUserId) || (m_systemAccessRights & SYSTEM_ACCESS_MANAGE_USERS))
   {
      request.getFieldAsString(VID_SEARCH_PATTERN, szPattern, MAX_USERVAR_NAME_LENGTH);
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
void ClientSession::deleteUserVariable(const NXCPMessage& request)
{
   NXCPMessage msg;
   TCHAR szVarName[MAX_USERVAR_NAME_LENGTH];
   UINT32 dwUserId;

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(request.getId());

   dwUserId = request.isFieldExist(VID_USER_ID) ? request.getFieldAsUInt32(VID_USER_ID) : m_dwUserId;
   if ((dwUserId == m_dwUserId) || (m_systemAccessRights & SYSTEM_ACCESS_MANAGE_USERS))
   {
      DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

      // Try to delete variable from database
      request.getFieldAsString(VID_NAME, szVarName, MAX_USERVAR_NAME_LENGTH);
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
void ClientSession::copyUserVariable(const NXCPMessage& request)
{
   NXCPMessage msg;
   TCHAR szVarName[MAX_USERVAR_NAME_LENGTH], szCurrVar[MAX_USERVAR_NAME_LENGTH];
   UINT32 dwSrcUserId, dwDstUserId;
   int i, nRows;
   BOOL bMove, bExist;
   DB_RESULT hResult, hResult2;

   // Prepare response message
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(request.getId());

   if (m_systemAccessRights & SYSTEM_ACCESS_MANAGE_USERS)
   {
      DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

      dwSrcUserId = request.isFieldExist(VID_USER_ID) ? request.getFieldAsUInt32(VID_USER_ID) : m_dwUserId;
      dwDstUserId = request.getFieldAsUInt32(VID_DST_USER_ID);
      bMove = (BOOL)request.getFieldAsUInt16(VID_MOVE_FLAG);
      request.getFieldAsString(VID_NAME, szVarName, MAX_USERVAR_NAME_LENGTH);
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
void ClientSession::changeObjectZone(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

   // Get object id and check prerequisites
   shared_ptr<NetObj> object = FindObjectById(request.getFieldAsUInt32(VID_OBJECT_ID));
   if (object != nullptr)
   {
      if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_MODIFY))
      {
			if (object->getObjectClass() == OBJECT_NODE || object->getObjectClass() == OBJECT_CLUSTER)
			{
			   int32_t zoneUIN = request.getFieldAsUInt32(VID_ZONE_UIN);
				shared_ptr<Zone> zone = FindZoneByUIN(zoneUIN);
				if (zone != nullptr)
				{
					// Check if target zone already have object with same primary IP
				   if (object->getObjectClass() == OBJECT_NODE)
				   {
                  if ((static_cast<Node&>(*object).getFlags() & NF_EXTERNAL_GATEWAY) ||
                      ((FindNodeByIP(zoneUIN, static_cast<Node&>(*object).getIpAddress()) == nullptr) &&
                       (FindSubnetByIP(zoneUIN, static_cast<Node&>(*object).getIpAddress()) == nullptr)))
                  {
                     if (static_cast<Node&>(*object).getMyCluster() == nullptr)
                     {
                        static_cast<Node&>(*object).changeZone(zoneUIN);
                        response.setField(VID_RCC, RCC_SUCCESS);
                     }
                     else
                     {
                        response.setField(VID_RCC, RCC_ZONE_CHANGE_FORBIDDEN);
                     }
                  }
                  else
                  {
                     response.setField(VID_RCC, RCC_ADDRESS_IN_USE);
                  }
				   }
				   else
				   {
                  static_cast<Cluster&>(*object).changeZone(zoneUIN);
				   }
				}
				else
				{
		         response.setField(VID_RCC, RCC_INVALID_ZONE_ID);
				}
         }
         else
         {
	         response.setField(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
         }
      }
      else
      {
         response.setField(VID_RCC, RCC_ACCESS_DENIED);
      }
   }
   else
   {
      response.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   sendMessage(response);
}

/**
 * Setup encryption with client
 */
void ClientSession::setupEncryption(const NXCPMessage& request)
{
#ifdef _WITH_ENCRYPTION
	m_encryptionRqId = request.getId();
   m_encryptionResult = RCC_TIMEOUT;

   // Send request for session key
   NXCPMessage msg;
   msg.setId(request.getId());
	PrepareKeyRequestMsg(&msg, g_serverKey, request.getFieldAsBoolean(VID_USE_X509_KEY_FORMAT));
	msg.setId(request.getId());
   sendMessage(msg);
   msg.deleteAllFields();

   // Wait for encryption setup
   m_condEncryptionSetup.wait(30000);

   // Send final response
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setField(VID_RCC, m_encryptionResult);
#else    /* _WITH_ENCRYPTION not defined */
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request.getId());
   msg.setField(VID_RCC, RCC_NO_ENCRYPTION_SUPPORT);
#endif

   sendMessage(msg);
}

/**
 * Get agent's configuration file
 */
void ClientSession::readAgentConfigFile(const NXCPMessage& request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request.getId());

   // Get object id and check prerequisites
   shared_ptr<NetObj> object = FindObjectById(request.getFieldAsUInt32(VID_OBJECT_ID));
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
void ClientSession::writeAgentConfigFile(const NXCPMessage& request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request.getId());

   // Get object id and check prerequisites
   shared_ptr<NetObj> object = FindObjectById(request.getFieldAsUInt32(VID_OBJECT_ID));
   if (object != nullptr)
   {
      if (object->getObjectClass() == OBJECT_NODE)
      {
         if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_CONTROL))
         {
            shared_ptr<AgentConnectionEx> pConn = static_cast<Node&>(*object).createAgentConnection();
            if (pConn != nullptr)
            {
               TCHAR *content = request.getFieldAsString(VID_CONFIG_FILE);
               uint32_t agentRCC = pConn->writeConfigFile(content);
               MemFree(content);

               if ((request.getFieldAsUInt16(VID_APPLY_FLAG) != 0) && (agentRCC == ERR_SUCCESS))
               {
                  StringList list;
                  agentRCC = pConn->executeCommand(_T("Agent.Restart"), list);
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
   NXCPMessage m_msg;

   ActionExecutionData(ClientSession *session, uint32_t requestId) : m_msg(CMD_COMMAND_OUTPUT, requestId)
   {
      m_session = session;
   }
};

/**
 * Action execution callback
 */
static void ActionExecuteCallback(ActionCallbackEvent e, const TCHAR *text, void *arg)
{
   ActionExecutionData *data = static_cast<ActionExecutionData*>(arg);
   switch(e)
   {
      case ACE_CONNECTED:
         data->m_msg.setCode(CMD_REQUEST_COMPLETED);
         data->m_msg.setField(VID_RCC, RCC_SUCCESS);
         break;
      case ACE_DISCONNECTED:
         data->m_msg.deleteAllFields();
         data->m_msg.setCode(CMD_COMMAND_OUTPUT);
         data->m_msg.setEndOfSequence();
         break;
      case ACE_DATA:
         data->m_msg.deleteAllFields();
         data->m_msg.setCode(CMD_COMMAND_OUTPUT);
         data->m_msg.setField(VID_MESSAGE, text);
         break;
   }
   data->m_session->sendMessage(data->m_msg);
}

/**
 * Execute action on agent
 */
void ClientSession::executeAction(const NXCPMessage& request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request.getId());

   // Get object id and check prerequisites
   shared_ptr<NetObj> object = FindObjectById(request.getFieldAsUInt32(VID_OBJECT_ID));
   if (object != nullptr)
   {
      if (object->getObjectClass() == OBJECT_NODE)
      {
         TCHAR action[MAX_PARAM_NAME];
         TCHAR originalActionString[MAX_PARAM_NAME];
         request.getFieldAsString(VID_ACTION_NAME, action, MAX_PARAM_NAME);
         _tcsncpy(originalActionString, action, MAX_PARAM_NAME);
         bool expandString = request.getFieldAsBoolean(VID_EXPAND_STRING);

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
                  inputFields.addAllFromMessage(request, VID_INPUT_FIELD_BASE, VID_INPUT_FIELD_COUNT);
                  alarm = FindAlarmById(request.getFieldAsUInt32(VID_ALARM_ID));
                  if ((alarm != nullptr) && !object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ_ALARMS) && !alarm->checkCategoryAccess(this))
                  {
                     msg.setField(VID_RCC, RCC_ACCESS_DENIED);
                     sendMessage(&msg);
                     delete alarm;
                     return;
                  }
                  list = SplitCommandLine(object->expandText(action, alarm, nullptr, shared_ptr<DCObjectInfo>(), m_loginName, nullptr, nullptr, &inputFields, nullptr));
                  _tcslcpy(action, list->get(0), MAX_PARAM_NAME);
                  list->remove(0);
               }
               else
               {
                  int argc = request.getFieldAsInt16(VID_NUM_ARGS);
                  if (argc > 64)
                  {
                     debugPrintf(4, _T("executeAction: too many arguments (%d)"), argc);
                     argc = 64;
                  }

                  list = new StringList();
                  uint32_t fieldId = VID_ACTION_ARG_BASE;
                  for(int i = 0; i < argc; i++)
                     list->addPreallocated(request.getFieldAsString(fieldId++));
               }

               uint32_t rcc;
               bool withOutput = request.getFieldAsBoolean(VID_RECEIVE_OUTPUT);
               if (withOutput)
               {
                  ActionExecutionData data(this, request.getId());
                  rcc = pConn->executeCommand(action, *list, true, ActionExecuteCallback, &data);
               }
               else
               {
                  rcc = pConn->executeCommand(action, *list);
               }
               debugPrintf(4, _T("executeAction: rcc=%d"), rcc);

               if (expandString && (request.getFieldAsInt32(VID_NUM_MASKED_FIELDS) > 0))
               {
                  delete list;
                  StringList maskedFields(request, VID_MASKED_FIELD_LIST_BASE, VID_NUM_MASKED_FIELDS);
                  for (int i = 0; i < maskedFields.size(); i++)
                  {
                     inputFields.set(maskedFields.get(i), _T("******"));
                  }
                  list = SplitCommandLine(object->expandText(originalActionString, alarm, nullptr, shared_ptr<DCObjectInfo>(), m_loginName, nullptr, nullptr, &inputFields, nullptr));
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
void ClientSession::getObjectTools(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());
   response.setField(VID_RCC, GetObjectToolsIntoMessage(&response, m_dwUserId, checkSysAccessRights(SYSTEM_ACCESS_MANAGE_TOOLS)));
   sendMessage(response);
}

/**
 * Send tool list to client
 */
void ClientSession::getObjectToolDetails(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());
   if (checkSysAccessRights(SYSTEM_ACCESS_MANAGE_TOOLS))
   {
      response.setField(VID_RCC, GetObjectToolDetailsIntoMessage(request.getFieldAsUInt32(VID_TOOL_ID), &response));
   }
   else
   {
      response.setField(VID_RCC, RCC_ACCESS_DENIED);
      WriteAuditLog(AUDIT_SYSCFG, FALSE, m_dwUserId, m_workstation, m_id, 0, _T("Access denied on reading object tool details"));
   }
   sendMessage(response);
}

/**
 * Update object tool
 */
void ClientSession::updateObjectTool(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());
   if (m_systemAccessRights & SYSTEM_ACCESS_MANAGE_TOOLS)
   {
      uint32_t rcc = UpdateObjectToolFromMessage(request);
      response.setField(VID_RCC, rcc);
      if (rcc == RCC_SUCCESS)
         writeAuditLog(AUDIT_SYSCFG, true, 0, _T("Object tool [%u] updated"), request.getFieldAsUInt32(VID_TOOL_ID));
   }
   else
   {
      writeAuditLog(AUDIT_SYSCFG, false, 0, _T("Access denied on updating object tool [%u]"), request.getFieldAsUInt32(VID_TOOL_ID));
      response.setField(VID_RCC, RCC_ACCESS_DENIED);
   }
   sendMessage(response);
}

/**
 * Delete object tool
 */
void ClientSession::deleteObjectTool(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());
   uint32_t toolId = request.getFieldAsUInt32(VID_TOOL_ID);
   if (m_systemAccessRights & SYSTEM_ACCESS_MANAGE_TOOLS)
   {
      uint32_t rcc = DeleteObjectToolFromDB(toolId);
      response.setField(VID_RCC, rcc);
      if (rcc == RCC_SUCCESS)
         writeAuditLog(AUDIT_SYSCFG, true, 0, _T("Object tool [%u] deleted"), toolId);
   }
   else
   {
      writeAuditLog(AUDIT_SYSCFG, false, 0, _T("Access denied on deleting object tool [%u]"), toolId);
      response.setField(VID_RCC, RCC_ACCESS_DENIED);
   }
   sendMessage(response);
}

/**
 * Change Object Tool status (enabled/disabled)
 */
void ClientSession::changeObjectToolStatus(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());
   if (m_systemAccessRights & SYSTEM_ACCESS_MANAGE_TOOLS)
   {
      uint32_t toolID = request.getFieldAsUInt32(VID_TOOL_ID);
      response.setField(VID_RCC, ChangeObjectToolStatus(toolID, request.getFieldAsBoolean(VID_STATE)));
   }
   else
   {
      response.setField(VID_RCC, RCC_ACCESS_DENIED);
   }
   sendMessage(response);
}

/**
 * Generate ID for new object tool
 */
void ClientSession::generateObjectToolId(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());
   if (checkSysAccessRights(SYSTEM_ACCESS_MANAGE_TOOLS))
   {
      response.setField(VID_TOOL_ID, CreateUniqueId(IDG_OBJECT_TOOL));
   }
   else
   {
      response.setField(VID_RCC, RCC_ACCESS_DENIED);
   }
   sendMessage(response);
}

/**
 * Execute table tool (either SNMP or agent table)
 */
void ClientSession::execTableTool(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

   // Check if tool exist and has correct type
   uint32_t dwToolId = request.getFieldAsUInt32(VID_TOOL_ID);
   if (IsTableTool(dwToolId))
   {
      // Check access
      if (CheckObjectToolAccess(dwToolId, m_dwUserId))
      {
         shared_ptr<NetObj> object = FindObjectById(request.getFieldAsUInt32(VID_OBJECT_ID));
         if (object != nullptr)
         {
            if (object->getObjectClass() == OBJECT_NODE)
            {
               response.setField(VID_RCC,
                               ExecuteTableTool(dwToolId, static_pointer_cast<Node>(object),
                                                request.getId(), this));
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
      }
      else
      {
         response.setField(VID_RCC, RCC_ACCESS_DENIED);
      }
   }
   else
   {
      response.setField(VID_RCC, RCC_INVALID_TOOL_ID);
   }

   sendMessage(response);
}

/**
 * Change current subscription
 */
void ClientSession::changeSubscription(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

   TCHAR channel[64];
   request.getFieldAsString(VID_NAME, channel, 64);
   Trim(channel);
   if (channel[0] != 0)
   {
      m_subscriptionLock.lock();
      uint32_t *count = m_subscriptions.get(channel);
      if (request.getFieldAsBoolean(VID_OPERATION))
      {
         // Subscribe
         if (count == nullptr)
         {
            count = new UINT32;
            *count = 1;
            m_subscriptions.set(channel, count);
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
               m_subscriptions.remove(channel);
         }
      }
      m_subscriptionLock.unlock();
      response.setField(VID_RCC, RCC_SUCCESS);
   }
   else
   {
      response.setField(VID_RCC, RCC_INVALID_ARGUMENT);
   }

   sendMessage(response);
}

/**
 * Callback for counting DCIs in the system
 */
static void DciCountCallback(NetObj *object, uint32_t *count)
{
	*count += static_cast<DataCollectionTarget*>(object)->getItemCount();
}

/**
 * Send server statistics
 */
void ClientSession::sendServerStats(const NXCPMessage& request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request.getId());
   msg.setField(VID_RCC, RCC_SUCCESS);

   // Server version, etc.
   msg.setField(VID_SERVER_VERSION, NETXMS_VERSION_STRING);
   msg.setField(VID_SERVER_UPTIME, static_cast<uint32_t>(time(nullptr) - g_serverStartTime));

   // Number of objects and DCIs
	uint32_t dciCount = 0;
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
   PROCESS_MEMORY_COUNTERS mc;
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
   sendMessage(msg);
}

/**
 * Send script list
 */
void ClientSession::sendScriptList(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());
   if (m_systemAccessRights & SYSTEM_ACCESS_MANAGE_SCRIPTS)
   {
      GetServerScriptLibrary()->fillMessage(&response);
      response.setField(VID_RCC, RCC_SUCCESS);;
   }
   else
   {
      response.setField(VID_RCC, RCC_ACCESS_DENIED);
   }
   sendMessage(response);
}

/**
 * Send script
 */
void ClientSession::sendScript(const NXCPMessage& request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request.getId());

   if (m_systemAccessRights & SYSTEM_ACCESS_MANAGE_SCRIPTS)
   {
      uint32_t id = request.getFieldAsUInt32(VID_SCRIPT_ID);
      NXSL_LibraryScript *script = GetServerScriptLibrary()->findScript(id);
      if (script != nullptr)
         script->fillMessage(&msg);
      else
         msg.setField(VID_RCC, RCC_INVALID_SCRIPT_ID);
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
void ClientSession::updateScript(const NXCPMessage& request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request.getId());
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
      request.getFieldAsString(VID_NAME, scriptName, MAX_DB_STRING);
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
void ClientSession::renameScript(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());
   if (m_systemAccessRights & SYSTEM_ACCESS_MANAGE_SCRIPTS)
   {
      uint32_t scriptId = request.getFieldAsUInt32(VID_SCRIPT_ID);
      TCHAR oldName[MAX_DB_STRING];
      if (GetScriptName(scriptId, oldName, MAX_DB_STRING))
      {
         TCHAR newName[MAX_DB_STRING];
         request.getFieldAsString(VID_NAME, newName, MAX_DB_STRING);
         uint32_t rcc = RenameScript(scriptId, newName);
         response.setField(VID_RCC, rcc);
         if (rcc == RCC_SUCCESS)
         {
            writeAuditLog(AUDIT_SYSCFG, true, 0, _T("Library script with ID %u renamed from %s to %s"), scriptId, oldName, newName);
         }
      }
      else
      {
         response.setField(VID_RCC, RCC_INVALID_SCRIPT_ID);
      }
   }
   else
   {
      response.setField(VID_RCC, RCC_ACCESS_DENIED);
      TCHAR newName[MAX_DB_STRING];
      writeAuditLog(AUDIT_SYSCFG, false, 0, _T("Access denied on renaming library script with ID %u to %s"),
               request.getFieldAsUInt32(VID_SCRIPT_ID), request.getFieldAsString(VID_NAME, newName, MAX_DB_STRING));
   }
   sendMessage(response);
}

/**
 * Delete script from library
 */
void ClientSession::deleteScript(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());
   uint32_t scriptId = request.getFieldAsUInt32(VID_SCRIPT_ID);
   if (m_systemAccessRights & SYSTEM_ACCESS_MANAGE_SCRIPTS)
   {
      TCHAR scriptName[MAX_DB_STRING];
      if (GetScriptName(scriptId, scriptName, MAX_DB_STRING))
      {
         uint32_t rcc = DeleteScript(scriptId);
         response.setField(VID_RCC, rcc);
         if (rcc == RCC_SUCCESS)
         {
            writeAuditLog(AUDIT_SYSCFG, true, 0, _T("Library script [%u] deleted"), scriptId);
         }
      }
      else
      {
         response.setField(VID_RCC, RCC_INVALID_SCRIPT_ID);
      }
   }
   else
   {
      response.setField(VID_RCC, RCC_ACCESS_DENIED);
      TCHAR scriptName[MAX_DB_STRING];
      if (!GetScriptName(scriptId, scriptName, MAX_DB_STRING))
         _tcscpy(scriptName, _T("<unknown>"));
      writeAuditLog(AUDIT_SYSCFG, false, 0, _T("Access denied on deleting library script %s [%u]"), scriptName, scriptId);
   }
   sendMessage(response);
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
void ClientSession::getSessionList(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());
   if (m_systemAccessRights & SYSTEM_ACCESS_MANAGE_SESSIONS)
   {
      response.setField(VID_NUM_SESSIONS, (UINT32)0);
      EnumerateClientSessions(CopySessionData, &response);
      response.setField(VID_RCC, RCC_SUCCESS);
   }
   else
   {
      response.setField(VID_RCC, RCC_ACCESS_DENIED);
   }
   sendMessage(response);
}

/**
 * Forcibly terminate client's session
 */
void ClientSession::killSession(const NXCPMessage& request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request.getId());
   if (m_systemAccessRights & SYSTEM_ACCESS_MANAGE_SESSIONS)
   {
      session_id_t id = request.getFieldAsInt32(VID_SESSION_ID);
      bool success = KillClientSession(id);
      msg.setField(VID_RCC, success ? RCC_SUCCESS : RCC_INVALID_SESSION_HANDLE);
   }
   else
   {
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);
   }
   sendMessage(msg);
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
   NXCPMessage *msg;
   uint32_t fieldId;
   uint32_t varbindCount;
   ClientSession *session;
};

/**
 * SNMP walker enumeration callback
 */
static uint32_t WalkerCallback(SNMP_Variable *pVar, SNMP_Transport *transport, SNMP_WalkerContext *context)
{
   NXCPMessage *msg = context->msg;
   TCHAR buffer[4096];
	bool convertToHex = true;

   msg->setField(context->fieldId++, pVar->getName().toString(buffer, 4096));
   pVar->getValueAsPrintableString(buffer, 4096, &convertToHex);
   msg->setField(context->fieldId++, convertToHex ? (UINT32)0xFFFF : pVar->getType());
   msg->setField(context->fieldId++, buffer);
   context->varbindCount++;
   if (context->varbindCount == 50)
   {
      msg->setField(VID_NUM_VARIABLES, context->varbindCount);
      context->session->sendMessage(msg);
      context->varbindCount = 0;
      context->fieldId = VID_SNMP_WALKER_DATA_BASE;
      msg->deleteAllFields();
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
   context.msg = &msg;
   context.fieldId = VID_SNMP_WALKER_DATA_BASE;
   context.varbindCount = 0;
   context.session = args->session;
   args->node->callSnmpEnumerate(args->baseOID, WalkerCallback, &context);
   msg.setField(VID_NUM_VARIABLES, context.varbindCount);
   msg.setEndOfSequence();
   args->session->sendMessage(msg);
   delete args;
}

/**
 * Start SNMP walk
 */
void ClientSession::startSnmpWalk(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

   shared_ptr<NetObj> object = FindObjectById(request.getFieldAsUInt32(VID_OBJECT_ID));
   if (object != nullptr)
   {
      if (object->getObjectClass() == OBJECT_NODE)
      {
         if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ_SNMP))
         {
            response.setField(VID_RCC, RCC_SUCCESS);

            InterlockedIncrement(&m_refCount);

            SNMP_WalkerThreadArgs *args = new SNMP_WalkerThreadArgs(this, request.getId(), static_pointer_cast<Node>(object));
            args->baseOID = request.getFieldAsString(VID_SNMP_OID);
            ThreadPoolExecute(g_clientThreadPool, SNMP_WalkerThread, args);
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
   sendMessage(response);
}

/**
 * Resolve single DCI name
 */
uint32_t ClientSession::resolveDCIName(uint32_t nodeId, uint32_t dciId, TCHAR *name)
{
   uint32_t rcc;
   shared_ptr<NetObj> object = FindObjectById(nodeId);
   if (object != nullptr)
   {
		if (object->isDataCollectionTarget() || (object->getObjectClass() == OBJECT_TEMPLATE))
		{
			if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
			{
				shared_ptr<DCObject> dci = static_cast<DataCollectionOwner&>(*object).getDCObjectById(dciId, m_dwUserId);
				if (dci != nullptr)
				{
               _tcslcpy(name, dci->getDescription(), MAX_DB_STRING);
					rcc = RCC_SUCCESS;
				}
				else
				{
               _sntprintf(name, MAX_DB_STRING, _T("[%d]"), dciId);
					rcc = RCC_SUCCESS;
				}
			}
			else
			{
				rcc = RCC_ACCESS_DENIED;
			}
		}
		else
		{
	      rcc = RCC_INCOMPATIBLE_OPERATION;
		}
   }
   else
   {
      rcc = RCC_INVALID_OBJECT_ID;
   }
   return rcc;
}

/**
 * Resolve DCI identifiers to names
 */
void ClientSession::resolveDCINames(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

   uint32_t count = request.getFieldAsUInt32(VID_NUM_ITEMS);
   uint32_t *nodeList = MemAllocArray<uint32_t>(count);
   uint32_t *dciList = MemAllocArray<uint32_t>(count);
   request.getFieldAsInt32Array(VID_NODE_LIST, count, nodeList);
   request.getFieldAsInt32Array(VID_DCI_LIST, count, dciList);

   uint32_t rcc = RCC_INVALID_ARGUMENT;
   uint32_t i = 0;
   uint32_t fieldId = VID_DCI_LIST_BASE;
   for(; i < count; i++)
   {
      TCHAR m_description[MAX_DB_STRING];
      rcc = resolveDCIName(nodeList[i], dciList[i], m_description);
      if (rcc != RCC_SUCCESS)
         break;
      response.setField(fieldId++, dciList[i]);
      response.setField(fieldId++, m_description);
   }
   response.setField(VID_NUM_ITEMS, i);

   MemFree(nodeList);
   MemFree(dciList);

   response.setField(VID_RCC, rcc);
   sendMessage(response);
}

/**
 * Get list of available agent configurations
 */
void ClientSession::getAgentConfigurationList(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

   if (m_systemAccessRights & SYSTEM_ACCESS_MANAGE_AGENT_CFG)
   {
      DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
      DB_RESULT hResult = DBSelect(hdb, _T("SELECT config_id,config_name,sequence_number FROM agent_configs"));
      if (hResult != nullptr)
      {
         int count = DBGetNumRows(hResult);
         response.setField(VID_RCC, RCC_SUCCESS);
         response.setField(VID_NUM_RECORDS, count);
         uint32_t fieldId = VID_AGENT_CFG_LIST_BASE;
         for(int i = 0; i < count; i++, fieldId += 7)
         {
            response.setField(fieldId++, DBGetFieldULong(hResult, i, 0));
            TCHAR name[MAX_DB_STRING];
            response.setField(fieldId++, DBGetField(hResult, i, 1, name, MAX_DB_STRING));
            response.setField(fieldId++, DBGetFieldULong(hResult, i, 2));
         }
         DBFreeResult(hResult);
      }
      else
      {
         response.setField(VID_RCC, RCC_DB_FAILURE);
      }
      DBConnectionPoolReleaseConnection(hdb);
   }
   else
   {
      response.setField(VID_RCC, RCC_ACCESS_DENIED);
   }

   sendMessage(response);
}

/**
 *  Get server-stored agent's configuration file
 */
void ClientSession::getAgentConfiguration(const NXCPMessage& request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request.getId());

   if (m_systemAccessRights & SYSTEM_ACCESS_MANAGE_AGENT_CFG)
   {
      DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
      uint32_t configId = request.getFieldAsUInt32(VID_CONFIG_ID);
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
void ClientSession::updateAgentConfiguration(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

   if (m_systemAccessRights & SYSTEM_ACCESS_MANAGE_AGENT_CFG)
   {
      DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
      uint32_t configId = request.getFieldAsUInt32(VID_CONFIG_ID);

      uint32_t sequence;
      DB_STATEMENT hStmt;
      if ((configId != 0) && IsDatabaseRecordExist(hdb, _T("agent_configs"), _T("config_id"), configId))
      {
         sequence = request.getFieldAsUInt32(VID_SEQUENCE_NUMBER);
         hStmt = DBPrepare(hdb, _T("UPDATE agent_configs SET config_name=?,config_filter=?,config_file=?,sequence_number=? WHERE config_id=?"));
      }
      else
      {
         if (configId == 0)
         {
            // Request for new ID creation
            configId = CreateUniqueId(IDG_AGENT_CONFIG);
            response.setField(VID_CONFIG_ID, configId);

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
            response.setField(VID_SEQUENCE_NUMBER, sequence);
         }
         else
         {
            sequence = request.getFieldAsUInt32(VID_SEQUENCE_NUMBER);
         }
         hStmt = DBPrepare(hdb, _T("INSERT INTO agent_configs (config_name,config_filter,config_file,sequence_number,config_id) VALUES (?,?,?,?,?)"));
      }
      if (hStmt != nullptr)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, request.getFieldAsString(VID_NAME), DB_BIND_DYNAMIC);
         DBBind(hStmt, 2, DB_SQLTYPE_TEXT, request.getFieldAsString(VID_FILTER), DB_BIND_DYNAMIC);
         DBBind(hStmt, 3, DB_SQLTYPE_TEXT, request.getFieldAsString(VID_CONFIG_FILE), DB_BIND_DYNAMIC);
         DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, sequence);
         DBBind(hStmt, 5, DB_SQLTYPE_INTEGER, configId);
         if (DBExecute(hStmt))
         {
            response.setField(VID_RCC, RCC_SUCCESS);
         }
         else
         {
            response.setField(VID_RCC, RCC_DB_FAILURE);
         }
         DBFreeStatement(hStmt);
      }
      else
      {
         response.setField(VID_RCC, RCC_DB_FAILURE);
      }
      DBConnectionPoolReleaseConnection(hdb);
   }
   else
   {
      response.setField(VID_RCC, RCC_ACCESS_DENIED);
   }

   sendMessage(response);
}

/**
 * Delete agent's configuration
 */
void ClientSession::deleteAgentConfiguration(const NXCPMessage& request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request.getId());

   if (m_systemAccessRights & SYSTEM_ACCESS_MANAGE_AGENT_CFG)
   {
      DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
      uint32_t configId = request.getFieldAsUInt32(VID_CONFIG_ID);
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
void ClientSession::swapAgentConfigurations(const NXCPMessage& request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request.getId());

   if (m_systemAccessRights & SYSTEM_ACCESS_MANAGE_AGENT_CFG)
   {
      DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

      TCHAR query[256];
      _sntprintf(query, 256, _T("SELECT config_id,sequence_number FROM agent_configs WHERE config_id=%d OR config_id=%d"),
                 request.getFieldAsUInt32(VID_CONFIG_ID), request.getFieldAsUInt32(VID_CONFIG_ID_2));
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
void ClientSession::sendConfigForAgent(const NXCPMessage& request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request.getId());

   TCHAR platform[MAX_DB_STRING];
   request.getFieldAsString(VID_PLATFORM_NAME, platform, MAX_DB_STRING);
   uint16_t versionMajor = request.getFieldAsUInt16(VID_VERSION_MAJOR);
   uint16_t versionMinor = request.getFieldAsUInt16(VID_VERSION_MINOR);
   uint16_t versionRelease = request.getFieldAsUInt16(VID_VERSION_RELEASE);
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
               ReportScriptError(SCRIPT_CONTEXT_AGENT_CFG, nullptr, 0, filter->getErrorText(), _T("AgentCfg::%d"), configId);
            }
            delete filter;
         }
         else
         {
            ReportScriptError(SCRIPT_CONTEXT_AGENT_CFG, nullptr, 0, errorMessage, _T("AgentCfg::%d"), configId);
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
void ClientSession::getObjectComments(const NXCPMessage& request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request.getId());

   shared_ptr<NetObj> object = FindObjectById(request.getFieldAsUInt32(VID_OBJECT_ID));
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
 * Update object comments from client.
 * Recieves comments source, expands it and sends expanded comments to all clients via setModified in setComments.
 */
void ClientSession::updateObjectComments(const NXCPMessage& request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request.getId());

   shared_ptr<NetObj> object = FindObjectById(request.getFieldAsUInt32(VID_OBJECT_ID));
   if (object != nullptr)
   {
      if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_MODIFY))
      {
         TCHAR *commentsSource = request.getFieldAsString(VID_COMMENTS);
         object->setComments(commentsSource);
         MemFree(commentsSource);
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
void ClientSession::pushDCIData(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

   int count = request.getFieldAsInt32(VID_NUM_ITEMS);
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
         UINT32 objectId = request.getFieldAsUInt32(fieldId++);
         if (objectId != 0)
         {
            object = FindObjectById(objectId);
         }
         else
         {
            TCHAR name[256];
            request.getFieldAsString(fieldId++, name, 256);
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
                  UINT32 dciId = request.getFieldAsUInt32(fieldId++);
						shared_ptr<DCObject> pItem;
                  if (dciId != 0)
                  {
                     pItem = static_cast<DataCollectionTarget&>(*object).getDCObjectById(dciId, m_dwUserId);
                  }
                  else
                  {
                     TCHAR name[256];
                     request.getFieldAsString(fieldId++, name, 256);
                     pItem = static_cast<DataCollectionTarget&>(*object).getDCObjectByName(name, m_dwUserId);
                  }

                  if ((pItem != nullptr) && (pItem->getType() == DCO_TYPE_ITEM))
                  {
                     if (pItem->getDataSource() == DS_PUSH_AGENT)
                     {
                        values.add(new ClientDataPushElement(static_pointer_cast<DataCollectionTarget>(object), pItem, request.getFieldAsString(fieldId++)));
                        bOK = TRUE;
                     }
                     else
                     {
                        response.setField(VID_RCC, RCC_NOT_PUSH_DCI);
                     }
                  }
                  else
                  {
                     response.setField(VID_RCC, RCC_INVALID_DCI_ID);
                  }
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
      }

      // If all items was checked OK, push data
      if (bOK)
      {
         time_t t = request.getFieldAsTime(VID_TIMESTAMP);
         if (t == 0)
            t = time(nullptr);
         shared_ptr<Table> tableValue; // Empty pointer to pass to processNewDCValue()
         for(int i = 0; i < values.size(); i++)
         {
            ClientDataPushElement *e = values.get(i);
				if (_tcslen(e->value) >= MAX_DCI_STRING_VALUE)
					e->value[MAX_DCI_STRING_VALUE - 1] = 0;
				e->dcTarget->processNewDCValue(e->dci, t, e->value, tableValue);
            if (t > e->dci->getLastPollTime())
				   e->dci->setLastPollTime(t);
         }
         response.setField(VID_RCC, RCC_SUCCESS);
      }
      else
      {
         response.setField(VID_FAILED_DCI_INDEX, i - 1);
      }
   }
   else
   {
      response.setField(VID_RCC, RCC_INVALID_ARGUMENT);
   }

   sendMessage(&response);
}

/**
 * Get address list
 */
void ClientSession::getAddrList(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

   if (m_systemAccessRights & SYSTEM_ACCESS_SERVER_CONFIG)
   {
      ObjectArray<InetAddressListElement> *list = LoadServerAddressList(request.getFieldAsInt32(VID_ADDR_LIST_TYPE));
      if (list != nullptr)
      {
         response.setField(VID_NUM_RECORDS, (INT32)list->size());

         UINT32 fieldId = VID_ADDR_LIST_BASE;
         for(int i = 0; i < list->size(); i++)
         {
            list->get(i)->fillMessage(&response, fieldId);
            fieldId += 10;
         }
         response.setField(VID_RCC, RCC_SUCCESS);
         delete list;
      }
      else
      {
         response.setField(VID_RCC, RCC_DB_FAILURE);
      }
   }
   else
   {
      response.setField(VID_RCC, RCC_ACCESS_DENIED);
   }

   sendMessage(response);
}

/**
 * Set address list
 */
void ClientSession::setAddrList(const NXCPMessage& request)
{
   NXCPMessage msg;
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(request.getId());

   int listType = request.getFieldAsInt32(VID_ADDR_LIST_TYPE);
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
void ClientSession::resetComponent(const NXCPMessage& request)
{
   NXCPMessage msg;
   UINT32 dwCode;

   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(request.getId());

   if (m_systemAccessRights & SYSTEM_ACCESS_SERVER_CONFIG)
   {
      dwCode = request.getFieldAsUInt32(VID_COMPONENT_ID);
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
void ClientSession::getRelatedEventList(const NXCPMessage& request)
{
   NXCPMessage msg;
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(request.getId());

   shared_ptr<NetObj> object = FindObjectById(request.getFieldAsUInt32(VID_OBJECT_ID));
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
void ClientSession::getDCIScriptList(const NXCPMessage& request)
{
   NXCPMessage msg;

   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(request.getId());

   shared_ptr<NetObj> object = FindObjectById(request.getFieldAsUInt32(VID_OBJECT_ID));
   if (object != nullptr)
   {
      if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
      {
         if (object->isDataCollectionTarget() || (object->getObjectClass() == OBJECT_TEMPLATE))
         {
            unique_ptr<StringSet> scripts = static_cast<DataCollectionOwner&>(*object).getDCIScriptList();
            msg.setField(VID_NUM_SCRIPTS, (INT32)scripts->size());
            ScriptNamesCallbackData data;
            data.msg = &msg;
            data.fieldId = VID_SCRIPT_LIST_BASE;
            scripts->forEach(ScriptNamesCallback, &data);
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
void ClientSession::exportConfiguration(const NXCPMessage& request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request.getId());

   if (checkSysAccessRights(SYSTEM_ACCESS_CONFIGURE_TRAPS | SYSTEM_ACCESS_VIEW_EVENT_DB | SYSTEM_ACCESS_EPP))
   {
      uint32_t *pdwTemplateList;
      uint32_t dwNumTemplates = request.getFieldAsUInt32(VID_NUM_OBJECTS);
      if (dwNumTemplates > 0)
      {
         pdwTemplateList = MemAllocArray<uint32_t>(dwNumTemplates);
         request.getFieldAsInt32Array(VID_OBJECT_LIST, dwNumTemplates, pdwTemplateList);
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
			TCHAR *description = request.getFieldAsString(VID_DESCRIPTION);
			xml.appendPreallocated(EscapeStringForXML(description, -1));
			MemFree(description);
         xml.append(_T("</description>\n"));

         // Write events
         xml += _T("\t<events>\n");
         uint32_t count = request.getFieldAsUInt32(VID_NUM_EVENTS);
         uint32_t *pdwList = MemAllocArray<uint32_t>(count);
         request.getFieldAsInt32Array(VID_EVENT_LIST, count, pdwList);
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
         count = request.getFieldAsUInt32(VID_NUM_TRAPS);
         pdwList = MemAllocArray<UINT32>(count);
         request.getFieldAsInt32Array(VID_TRAP_LIST, count, pdwList);
         for(i = 0; i < count; i++)
            CreateTrapExportRecord(xml, pdwList[i]);
         MemFree(pdwList);
         xml += _T("\t</traps>\n");

         // Write rules
         xml += _T("\t<rules>\n");
         count = request.getFieldAsUInt32(VID_NUM_RULES);
         DWORD varId = VID_RULE_LIST_BASE;
         uuid_t guid;
         for(i = 0; i < count; i++)
         {
            request.getFieldAsBinary(varId++, guid, UUID_LENGTH);
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
         count = request.getFieldAsUInt32(VID_NUM_SCRIPTS);
         pdwList = MemAllocArray<UINT32>(count);
         request.getFieldAsInt32Array(VID_SCRIPT_LIST, count, pdwList);
         for(i = 0; i < count; i++)
            CreateScriptExportRecord(xml, pdwList[i]);
         MemFree(pdwList);
         xml.append(_T("\t</scripts>\n"));

         // Write object tools
         xml.append(_T("\t<objectTools>\n"));
         count = request.getFieldAsUInt32(VID_NUM_TOOLS);
         pdwList = MemAllocArray<UINT32>(count);
         request.getFieldAsInt32Array(VID_TOOL_LIST, count, pdwList);
         for(i = 0; i < count; i++)
            CreateObjectToolExportRecord(xml, pdwList[i]);
         MemFree(pdwList);
         xml.append(_T("\t</objectTools>\n"));

         // Write DCI summary tables
         xml.append(_T("\t<dciSummaryTables>\n"));
         count = request.getFieldAsUInt32(VID_NUM_SUMMARY_TABLES);
         pdwList = MemAllocArray<UINT32>(count);
         request.getFieldAsInt32Array(VID_SUMMARY_TABLE_LIST, count, pdwList);
         for(i = 0; i < count; i++)
            CreateSummaryTableExportRecord(pdwList[i], xml);
         MemFree(pdwList);
         xml.append(_T("\t</dciSummaryTables>\n"));

         // Write actions
         xml.append(_T("\t<actions>\n"));
         count = request.getFieldAsUInt32(VID_NUM_ACTIONS);
         pdwList = MemAllocArray<UINT32>(count);
         request.getFieldAsInt32Array(VID_ACTION_LIST, count, pdwList);
         for(i = 0; i < count; i++)
            CreateActionExportRecord(xml, pdwList[i]);
         MemFree(pdwList);
         xml.append(_T("\t</actions>\n"));

         // Write web service definition
         xml.append(_T("\t<webServiceDefinitions>\n"));
         count = request.getFieldAsUInt32(VID_WEB_SERVICE_DEF_COUNT);
         pdwList = MemAllocArray<UINT32>(count);
         request.getFieldAsInt32Array(VID_WEB_SERVICE_DEF_LIST, count, pdwList);
         CreateWebServiceDefinitionExportRecord(xml, count, pdwList);
         MemFree(pdwList);
         xml.append(_T("\t</webServiceDefinitions>\n"));

         // Asset attributes
         xml.append(_T("\t<assetManagementSchema>\n"));
         StringList names(request, VID_ASSET_ATTRIBUTE_NAMES);
         ExportAssetManagementSchema(xml, names);
         xml.append(_T("\t</assetManagementSchema>\n"));

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
void ClientSession::importConfiguration(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

   if (checkSysAccessRights(SYSTEM_ACCESS_IMPORT_CONFIGURATION))
   {
      char *content = request.getFieldAsUtf8String(VID_NXMP_CONTENT);
      if (content != nullptr)
      {
         Config config(false);
         if (config.loadXmlConfigFromMemory(content, strlen(content), nullptr, "configuration", false))
         {
            finalizeConfigurationImport(config, request.getFieldAsUInt32(VID_FLAGS), &response);
         }
         else
         {
            response.setField(VID_RCC, RCC_CONFIG_PARSE_ERROR);
         }
         MemFree(content);
      }
      else
      {
         response.setField(VID_RCC, RCC_INVALID_ARGUMENT);
      }
   }
   else
   {
      response.setField(VID_RCC, RCC_ACCESS_DENIED);
   }

   sendMessage(response);
}

/**
 * Import server configuration (events, templates, etc.)
 */
void ClientSession::importConfigurationFromFile(const NXCPMessage& request)
{
   uint32_t requestId = request.getId();
   NXCPMessage response(CMD_REQUEST_COMPLETED, requestId);

   if (checkSysAccessRights(SYSTEM_ACCESS_IMPORT_CONFIGURATION))
   {
      TCHAR fileName[MAX_PATH];
      _sntprintf(fileName, MAX_PATH, _T("%s") FS_PATH_SEPARATOR _T("import-%s"), g_netxmsdDataDir, uuid::generate().toString().cstr());
      debugPrintf(5, _T("importConfigurationFromFile: fileName=%s"), fileName);

      ServerDownloadFileInfo *dInfo = new ServerDownloadFileInfo(fileName,
         [this, requestId] (const TCHAR *fileName, uint32_t uploadData, bool success) -> void
         {
            if (!success)
               return;

            NXCPMessage response2(CMD_REQUEST_COMPLETED, requestId);

            Config config(false);
            if (config.loadXmlConfig(fileName, "configuration", false))
            {
               finalizeConfigurationImport(config, uploadData, &response2);
            }
            else
            {
               response2.setField(VID_RCC, RCC_CONFIG_PARSE_ERROR);
            }

            sendMessage(response2);

            _tremove(fileName);
         });
      if (dInfo->open())
      {
         dInfo->setUploadData(request.getFieldAsUInt32(VID_FLAGS));
         m_downloadFileMap.set(request.getId(), dInfo);
      }
      else
      {
         response.setField(VID_RCC, RCC_IO_ERROR);
      }
   }
   else
   {
      response.setField(VID_RCC, RCC_ACCESS_DENIED);
   }

   sendMessage(response);
}

/**
 * Finalize configuration import after successful transfer
 */
void ClientSession::finalizeConfigurationImport(const Config& config, uint32_t flags, NXCPMessage *response)
{
   // Lock all required components
   TCHAR lockInfo[MAX_SESSION_NAME], errorMessage[1024];
   if (LockEPP(m_id, m_sessionName, nullptr, lockInfo))
   {
      InterlockedOr(&m_flags, CSF_EPP_LOCKED);

      // Validate and import configuration
      if (ValidateConfig(config, flags, errorMessage, 1024))
      {
         response->setField(VID_RCC, ImportConfig(config, flags));
      }
      else
      {
         response->setField(VID_RCC, RCC_CONFIG_VALIDATION_ERROR);
         response->setField(VID_ERROR_TEXT, errorMessage);
      }

      UnlockEPP();
      InterlockedAnd(&m_flags, ~CSF_EPP_LOCKED);
   }
   else
   {
      response->setField(VID_RCC, RCC_COMPONENT_LOCKED);
      response->setField(VID_COMPONENT, static_cast<uint16_t>(NXMP_LC_EPP));
      response->setField(VID_LOCKED_BY, lockInfo);
   }
}

/**
 * Get basic DCI info
 */
void ClientSession::getDCIInfo(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

   shared_ptr<NetObj> object = FindObjectById(request.getFieldAsUInt32(VID_OBJECT_ID));
   if (object != nullptr)
   {
      if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
      {
         if (object->isDataCollectionTarget() || (object->getObjectClass() == OBJECT_TEMPLATE))
         {
            shared_ptr<DCObject> dcObject = static_cast<DataCollectionOwner&>(*object).getDCObjectById(request.getFieldAsUInt32(VID_DCI_ID), m_dwUserId);
				if ((dcObject != nullptr) && (dcObject->getType() == DCO_TYPE_ITEM))
				{
					response.setField(VID_TEMPLATE_ID, dcObject->getTemplateId());
					response.setField(VID_RESOURCE_ID, dcObject->getResourceId());
					response.setField(VID_DCI_DATA_TYPE, static_cast<UINT16>(static_cast<DCItem&>(*dcObject).getDataType()));
					response.setField(VID_DCI_SOURCE_TYPE, static_cast<UINT16>(static_cast<DCItem&>(*dcObject).getDataSource()));
					response.setField(VID_NAME, dcObject->getName());
					response.setField(VID_DESCRIPTION, dcObject->getDescription());
	            response.setField(VID_RCC, RCC_SUCCESS);
				}
				else
				{
			      response.setField(VID_RCC, RCC_INVALID_DCI_ID);
				}
         }
         else
         {
            response.setField(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
         }
      }
      else
      {
         response.setField(VID_RCC, RCC_ACCESS_DENIED);
      }
   }
   else
   {
      response.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   sendMessage(response);
}


/**
 * Get DCI measurement units
 */
void ClientSession::getDciMeasurementUnits(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

   size_t count = request.getFieldAsUInt32(VID_NUM_ITEMS);
   Buffer<uint32_t> nodeList(count);
   Buffer<uint32_t> dciList(count);
   request.getFieldAsInt32Array(VID_NODE_LIST, count, nodeList);
   request.getFieldAsInt32Array(VID_DCI_LIST, count, dciList);

   uint32_t returnCount = 0;
   uint32_t fieldId = VID_DCI_LIST_BASE;
   for(size_t i = 0; i < count; i++)
   {
      shared_ptr<NetObj> object = FindObjectById(nodeList[i]);
      if (object != nullptr && object->isDataCollectionTarget())
      {
         if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
         {
            shared_ptr<DCObject> dci = static_cast<DataCollectionTarget&>(*object).getDCObjectById(dciList[i], m_dwUserId);
            if (dci != nullptr)
            {
               if (dci->getType() == DCO_TYPE_ITEM)
               {
                  response.setField(fieldId++, dciList[i]);
                  response.setField(fieldId++, static_cast<DCItem&>(*dci).getUnitName());
                  response.setField(fieldId++, static_cast<DCItem&>(*dci).getMultiplier());
                  returnCount++;
               }
               else
               {
                  debugPrintf(4, _T("getDciMeasurementUnits: invalid DCI %d type at target %s [%d] not found"), dciList[i], object->getName(), object->getId());
               }
            }
            else
            {
               response.setField(VID_RCC, RCC_INVALID_DCI_ID);
               debugPrintf(4, _T("getDciMeasurementUnits: DCI %d at target %s [%d] not found"), dciList[i], object->getName(), object->getId());
            }
         }
         else
         {
            writeAuditLog(AUDIT_OBJECTS, false, object->getId(), _T("Access denied on getting DCI info"));
         }
      }
      else
      {
         if (object == nullptr)
            debugPrintf(6, _T("getDciMeasurementUnits: invalid object id %u"), nodeList[i]);
         else
            debugPrintf(6, _T("getDciMeasurementUnits: object %s [%u] is not a data collection target"), object->getName(), object->getId());
      }
   }
   response.setField(VID_NUM_ITEMS, returnCount);
   response.setField(VID_RCC, RCC_SUCCESS);
   sendMessage(response);
}

/**
 * Get graph
 */
void ClientSession::getGraph(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());
   uint32_t rcc = GetGraphAccessCheckResult(request.getFieldAsUInt32(VID_GRAPH_ID), m_dwUserId, NXGRAPH_ACCESS_READ);
   if (rcc == RCC_SUCCESS)
   {
      FillGraphListMsg(&response, m_dwUserId, false, request.getFieldAsUInt32(VID_GRAPH_ID));
   }
   response.setField(VID_RCC, rcc);
   sendMessage(response);
}

/**
 * Send list of available graphs to client
 */
void ClientSession::getGraphList(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());
   FillGraphListMsg(&response, m_dwUserId, request.getFieldAsBoolean(VID_GRAPH_TEMPALTE), 0);
   sendMessage(response);
}

/**
 * Save graph
 */
void ClientSession::saveGraph(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());
   SaveGraph(request, m_dwUserId, &response);
   sendMessage(response);
}

/**
 * Delete graph
 */
void ClientSession::deleteGraph(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());
   response.setField(VID_RCC, DeleteGraph(request.getFieldAsUInt32(VID_GRAPH_ID), m_dwUserId));
   sendMessage(response);
}

/**
 * Get list of DCIs to be shown in performance tab
 */
void ClientSession::getPerfTabDCIList(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

	shared_ptr<NetObj> object = FindObjectById(request.getFieldAsUInt32(VID_OBJECT_ID));
	if (object != nullptr)
	{
		if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
		{
			if (object->isDataCollectionTarget())
			{
				response.setField(VID_RCC, static_cast<DataCollectionTarget&>(*object).getPerfTabDCIList(&response, m_dwUserId));
			}
			else
			{
				response.setField(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
			}
		}
		else
		{
			response.setField(VID_RCC, RCC_ACCESS_DENIED);
		}
	}
	else
	{
		response.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
	}

   sendMessage(&response);
}

/**
 * Query layer 2 topology from device
 */
void ClientSession::queryL2Topology(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

	shared_ptr<NetObj> object = FindObjectById(request.getFieldAsUInt32(VID_OBJECT_ID));
	if (object != nullptr)
	{
		if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
		{
			if (object->getObjectClass() == OBJECT_NODE)
			{
			   uint32_t rcc;
			   shared_ptr<NetworkMapObjectList> topology = static_cast<Node&>(*object).getL2Topology();
				if (topology == nullptr)
				{
				   topology = static_cast<Node&>(*object).buildL2Topology(&rcc, -1, true, false);
				}
				else
				{
					rcc = RCC_SUCCESS;
				}
				if (topology != nullptr)
				{
					response.setField(VID_RCC, RCC_SUCCESS);
					topology->createMessage(&response);
				}
				else
				{
					response.setField(VID_RCC, rcc);
				}
			}
			else
			{
				response.setField(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
			}
		}
		else
		{
			response.setField(VID_RCC, RCC_ACCESS_DENIED);
		}
	}
	else
	{
		response.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
	}

	sendMessage(response);
}

/**
 * Query object internal connection topology
 */
void ClientSession::queryInternalCommunicationTopology(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

   shared_ptr<NetObj> object = FindObjectById(request.getFieldAsUInt32(VID_OBJECT_ID));
   if (object != nullptr)
   {
      if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
      {
         unique_ptr<NetworkMapObjectList> topology;
         if (object->getObjectClass() == OBJECT_NODE)
         {
            topology = static_cast<Node&>(*object).buildInternalCommunicationTopology();
         }
         else if (object->getObjectClass() == OBJECT_SENSOR)
         {
            topology = static_cast<Sensor&>(*object).buildInternalCommunicationTopology();
         }

         if (topology != nullptr)
         {
            response.setField(VID_RCC, RCC_SUCCESS);
            topology->createMessage(&response);
         }
         else
         {
            response.setField(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
         }
      }
      else
      {
         response.setField(VID_RCC, RCC_ACCESS_DENIED);
      }
   }
   else
   {
      response.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   sendMessage(response);
}

/**
 * Query OSPF topology from device
 */
void ClientSession::queryOSPFTopology(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

   shared_ptr<NetObj> object = FindObjectById(request.getFieldAsUInt32(VID_OBJECT_ID));
   if (object != nullptr)
   {
      if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
      {
         if (object->getObjectClass() == OBJECT_NODE)
         {
            unique_ptr<NetworkMapObjectList> topology = BuildOSPFTopology(static_pointer_cast<Node>(object), -1);
            if (topology != nullptr)
            {
               response.setField(VID_RCC, RCC_SUCCESS);
               topology->createMessage(&response);
            }
            else
            {
               response.setField(VID_RCC, RCC_INTERNAL_ERROR);
            }
         }
         else
         {
            response.setField(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
         }
      }
      else
      {
         response.setField(VID_RCC, RCC_ACCESS_DENIED);
      }
   }
   else
   {
      response.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   sendMessage(response);
}

/**
 * Get list of dependent nodes
 */
void ClientSession::getDependentNodes(const NXCPMessage& request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request.getId());

   shared_ptr<NetObj> object = FindObjectById(request.getFieldAsUInt32(VID_NODE_ID));
   if (object != nullptr)
   {
      if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
      {
         if (object->getObjectClass() == OBJECT_NODE)
         {
            unique_ptr<StructArray<DependentNode>> dependencies = GetNodeDependencies(object->getId());
            msg.setField(VID_NUM_ELEMENTS, dependencies->size());
            UINT32 fieldId = VID_ELEMENT_LIST_BASE;
            for(int i = 0; i < dependencies->size(); i++)
            {
               msg.setField(fieldId++, dependencies->get(i)->nodeId);
               msg.setField(fieldId++, dependencies->get(i)->dependencyType);
               fieldId += 8;
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
 * Send notification
 */
void ClientSession::sendNotification(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

	if ((m_systemAccessRights & SYSTEM_ACCESS_SEND_NOTIFICATION))
	{
	   TCHAR channelName[MAX_OBJECT_NAME];
	   request.getFieldAsString(VID_CHANNEL_NAME, channelName, MAX_OBJECT_NAME);
	   if (IsNotificationChannelExists(channelName))
	   {
	      TCHAR *phone = request.getFieldAsString(VID_RCPT_ADDR);
	      TCHAR *subject = request.getFieldAsString(VID_EMAIL_SUBJECT);
	      TCHAR *message = request.getFieldAsString(VID_MESSAGE);
	      SendNotification(channelName, phone, subject, message);
	      response.setField(VID_RCC, RCC_SUCCESS);
	      MemFree(phone);
	      MemFree(subject);
	      MemFree(message);
	   }
	   else
	   {
         response.setField(VID_RCC, RCC_NO_CHANNEL_NAME);
	   }
	}
	else
	{
		response.setField(VID_RCC, RCC_ACCESS_DENIED);
	}

	sendMessage(response);
}

/**
 * Send persistent storage entries to client
 */
void ClientSession::getPersistantStorage(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());
	if (m_systemAccessRights & SYSTEM_ACCESS_PERSISTENT_STORAGE)
	{
		GetPersistentStorageList(&response);
	}
	else
	{
		response.setField(VID_RCC, RCC_ACCESS_DENIED);
	}
   sendMessage(response);
}

/**
 * Set persistent storage value
 */
void ClientSession::setPstorageValue(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

	if (m_systemAccessRights & SYSTEM_ACCESS_PERSISTENT_STORAGE)
	{
      TCHAR key[256], *value;
		request.getFieldAsString(VID_PSTORAGE_KEY, key, 256);
		value = request.getFieldAsString(VID_PSTORAGE_VALUE);
		SetPersistentStorageValue(key, value);
		free(value);
      response.setField(VID_RCC, RCC_SUCCESS);
	}
	else
	{
		response.setField(VID_RCC, RCC_ACCESS_DENIED);
	}

	sendMessage(response);
}

/**
 * Delete persistent storage value
 */
void ClientSession::deletePstorageValue(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

	if (m_systemAccessRights & SYSTEM_ACCESS_PERSISTENT_STORAGE)
	{
      TCHAR key[256];
      //key[0]=0;
		request.getFieldAsString(VID_PSTORAGE_KEY, key, 256);
		bool success = DeletePersistentStorageValue(key);
		response.setField(VID_RCC, success ? RCC_SUCCESS : RCC_INVALID_PSTORAGE_KEY);
	}
	else
	{
	   response.setField(VID_RCC, RCC_ACCESS_DENIED);
	}

   sendMessage(response);
}

/**
 * Register agent
 */
void ClientSession::registerAgent(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

	if (ConfigReadBoolean(_T("Agent.EnableRegistration"), false))
	{
	   int32_t zoneUIN = request.getFieldAsUInt32(VID_ZONE_UIN);
      shared_ptr<Node> node = FindNodeByIP(zoneUIN, m_clientAddr);
      if (node != nullptr)
      {
         // Node already exist, force configuration poll
         node->setRecheckCapsFlag();
         node->forceConfigurationPoll();
      }
      else
      {
         DiscoveredAddress *info = new DiscoveredAddress(m_clientAddr, zoneUIN, 0, DA_SRC_AGENT_REGISTRATION);
         info->ignoreFilter = true;		// Ignore discovery filters and add node anyway
         g_nodePollerQueue.put(info);
      }
      response.setField(VID_RCC, RCC_SUCCESS);
	}
	else
	{
      response.setField(VID_RCC, RCC_ACCESS_DENIED);
	}

   sendMessage(response);
}

/**
 * Get file from server
 */
void ClientSession::getServerFile(const NXCPMessage& request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request.getId());

   TCHAR name[MAX_PATH];
   request.getFieldAsString(VID_FILE_NAME, name, MAX_PATH);
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
			if (SendFileOverNXCP(m_socket, request.getId(), fname, m_encryptionContext.get(), 0, nullptr, nullptr, &m_mutexSocketWrite))
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
void ClientSession::getAgentFile(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

	bool success = false;

	shared_ptr<NetObj> object = FindObjectById(request.getFieldAsUInt32(VID_OBJECT_ID));
	if (object != nullptr)
	{
		if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_DOWNLOAD))
		{
			if (object->getObjectClass() == OBJECT_NODE)
			{
			   TCHAR remoteFile[MAX_PATH];
				request.getFieldAsString(VID_FILE_NAME, remoteFile, MAX_PATH);
            bool monitor = request.getFieldAsBoolean(VID_FILE_FOLLOW);
            shared_ptr<FileDownloadTask> task;
            if (request.getFieldAsBoolean(VID_EXPAND_STRING))
            {
               StringMap inputFields;
               inputFields.addAllFromMessage(request, VID_INPUT_FIELD_BASE, VID_INPUT_FIELD_COUNT);
               Alarm *alarm = FindAlarmById(request.getFieldAsUInt32(VID_ALARM_ID));
               if ((alarm != nullptr) && !object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ_ALARMS) && !alarm->checkCategoryAccess(this))
               {
                  response.setField(VID_RCC, RCC_ACCESS_DENIED);
               }
               else
               {
                  task = make_shared<FileDownloadTask>(static_pointer_cast<Node>(object), this, request.getId(),
                           object->expandText(remoteFile, alarm, nullptr, shared_ptr<DCObjectInfo>(), m_loginName, nullptr, nullptr, &inputFields, nullptr),
                           true, request.getFieldAsUInt32(VID_FILE_SIZE_LIMIT), monitor);
                  success = true;
               }
               delete alarm;
            }
            else
            {
               task = make_shared<FileDownloadTask>(static_pointer_cast<Node>(object), this, request.getId(),
                        remoteFile, false, request.getFieldAsUInt32(VID_FILE_SIZE_LIMIT), monitor);
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
				response.setField(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
			}
		}
		else
		{
		   writeAuditLog(AUDIT_OBJECTS, false, object->getId(), _T("Access denied on download file from node %s"), object->getName());
			response.setField(VID_RCC, RCC_ACCESS_DENIED);
		}
	}
	else
	{
		response.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
	}

	if (!success) // In case of success download task will send response message with all required information
	   sendMessage(response);
}

/**
 * Cancel file monitoring
 */
void ClientSession::cancelFileMonitoring(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());
   bool success;
   if (request.isFieldExist(VID_MONITOR_ID))
   {
      uuid clientId = request.getFieldAsGUID(VID_MONITOR_ID);
      success = RemoveFileMonitorByClientId(clientId, m_id);
      TCHAR buffer[64];
      debugPrintf(5, _T("Requested cancellation for file monitor with client ID %s (result=%s)"), clientId.toString(buffer), success ? _T("success") : _T("failure"));
   }
   else
   {
      // For compatibility with older clients that do cancel using agent file ID
      TCHAR agentFileId[64];
      request.getFieldAsString(VID_FILE_NAME, agentFileId, 64);
      success = RemoveFileMonitorsByAgentId(agentFileId, m_id);
      debugPrintf(5, _T("Requested cancellation for file monitor with agent file ID %s (result=%s)"), agentFileId, success ? _T("success") : _T("failure"));
   }
   response.setField(VID_RCC, success ? RCC_SUCCESS : RCC_INVALID_ARGUMENT);
   sendMessage(response);
}

/**
 * Test DCI transformation script
 */
void ClientSession::testDCITransformation(const NXCPMessage& request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request.getId());

   // Get node id and check object class and access rights
   shared_ptr<NetObj> object = FindObjectById(request.getFieldAsUInt32(VID_OBJECT_ID));
   if (object != nullptr)
   {
      if (object->isDataCollectionTarget())
      {
         if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
         {
				TCHAR *script = request.getFieldAsString(VID_SCRIPT);
				if (script != nullptr)
				{
				   shared_ptr<DCObjectInfo> dcObjectInfo;
				   if (request.isFieldExist(VID_DCI_ID))
				   {
				      uint32_t dciId = request.getFieldAsUInt32(VID_DCI_ID);
				      shared_ptr<DCObject> dcObject = static_cast<DataCollectionTarget&>(*object).getDCObjectById(dciId, m_dwUserId);
				      dcObjectInfo = make_shared<DCObjectInfo>(request, dcObject.get());
				   }

				   TCHAR value[256], result[256];
					request.getFieldAsString(VID_VALUE, value, sizeof(value) / sizeof(TCHAR));
               bool success = DCItem::testTransformation(static_cast<DataCollectionTarget*>(object.get()), dcObjectInfo, script, value, result, sizeof(result) / sizeof(TCHAR));
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
void ClientSession::executeScript(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

   bool success = false;
   NXSL_VM *vm = nullptr;

   // Get node id and check object class and access rights
   shared_ptr<NetObj> object = FindObjectById(request.getFieldAsUInt32(VID_OBJECT_ID));
   if (object != nullptr)
   {
      TCHAR *script = request.getFieldAsString(VID_SCRIPT);
      if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_MODIFY) || (!ConfigReadBoolean(_T("Objects.ScriptExecution.RequireWriteAccess"), true) && object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ)))
      {
         if (script != nullptr)
         {
            TCHAR errorMessage[256];
            vm = NXSLCompileAndCreateVM(script, errorMessage, 256, new NXSL_ClientSessionEnv(this, &response));
            if (vm != nullptr)
            {
               SetupServerScriptVM(vm, object, shared_ptr<DCObjectInfo>());
               response.setField(VID_RCC, RCC_SUCCESS);
               sendMessage(response);
               success = true;
               writeAuditLogWithValues(AUDIT_OBJECTS, true, object->getId(), nullptr, script, 'T', _T("Executed ad-hoc script for object %s [%u]"), object->getName(), object->getId());
            }
            else
            {
               response.setField(VID_RCC, RCC_NXSL_COMPILATION_ERROR);
               response.setField(VID_ERROR_TEXT, errorMessage);
            }
         }
         else
         {
            response.setField(VID_RCC, RCC_INVALID_ARGUMENT);
         }
      }
      else  // User doesn't have READ rights on object
      {
         writeAuditLogWithValues(AUDIT_OBJECTS, false, object->getId(), nullptr, script, 'T', _T("Access denied on ad-hoc script execution for object %s [%u]"), object->getName(), object->getId());
         response.setField(VID_RCC, RCC_ACCESS_DENIED);
      }
      MemFree(script);
   }
   else  // No object with given ID
   {
      response.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   // start execution
   if (success)
   {
      response.setCode(CMD_EXECUTE_SCRIPT_UPDATE);

      int count = request.getFieldAsInt16(VID_NUM_FIELDS);
      ObjectRefArray<NXSL_Value> sargs(count, 1);
      if (request.isFieldExist(VID_PARAMETER))
      {
         TCHAR parameters[1024];
         request.getFieldAsString(VID_PARAMETER, parameters, 1024);
         TCHAR *p = parameters;
         ParseValueList(vm, &p, sargs, false);
      }
      else if (count > 0)
      {
         uint32_t fieldId = VID_FIELD_LIST_BASE;
         for(int i = 0; i < count; i++)
         {
            SharedString value = request.getFieldAsSharedString(fieldId++);
            sargs.add(vm->createValue(value));
         }
      }

      if (vm->run(sargs))
      {
         TCHAR buffer[1024];
         const TCHAR *value = vm->getResult()->getValueAsCString();
         _sntprintf(buffer, 1024, _T("\n\n*** FINISHED ***\n\nResult: %s\n\n"), CHECK_NULL(value));
         buffer[1023] = 0;
         response.setField(VID_MESSAGE, buffer);
         response.setField(VID_RCC, RCC_SUCCESS);
         response.setEndOfSequence();
         sendMessage(response);

         if (request.getFieldAsBoolean(VID_RESULT_AS_MAP))
         {
            response.setCode(CMD_SCRIPT_EXECUTION_RESULT);
            NXSL_Value *result = vm->getResult();
            StringMap map;
            if (result->isHashMap())
            {
               result->getValueAsHashMap()->toStringMap(&map);
            }
            else if (result->isArray())
            {
               NXSL_Array *a = result->getValueAsArray();
               for(int i = 0; i < a->size(); i++)
               {
                  NXSL_Value *e = a->getByPosition(i);
                  TCHAR key[16];
                  _sntprintf(key, 16, _T("%d"), i + 1);
                  if (e->isHashMap())
                  {
                     json_t *json = e->toJson();
                     char *jsonText = json_dumps(json, 0);
                     map.setPreallocated(MemCopyString(key), TStringFromUTF8String(jsonText));
                     MemFree(jsonText);
                     json_decref(json);
                  }
                  else
                  {
                     map.set(key, e->getValueAsCString());
                  }
               }
            }
            else
            {
               map.set(_T("1"), result->getValueAsCString());
            }
            map.fillMessage(&response, VID_ELEMENT_LIST_BASE, VID_NUM_ELEMENTS);
            sendMessage(response);
         }
      }
      else
      {
         response.setField(VID_ERROR_TEXT, vm->getErrorText());
         response.setField(VID_RCC, RCC_NXSL_EXECUTION_ERROR);
         response.setEndOfSequence();
         sendMessage(response);

         if (!request.getFieldAsBoolean(VID_DEVELOPMENT_MODE))
            ReportScriptError(SCRIPT_CONTEXT_CLIENT, object.get(), 0, vm->getErrorText(), _T("ClientSession::executeScript"));
      }
      delete vm;
   }
   else
   {
      sendMessage(response);
   }
}

/**
 * Execute library script in object's context
 */
void ClientSession::executeLibraryScript(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());
   bool success = false;
   NXSL_VM *vm = nullptr;
   StringList *args = nullptr;
   bool withOutput = request.getFieldAsBoolean(VID_RECEIVE_OUTPUT);

   // Get node id and check object class and access rights
   shared_ptr<NetObj> object = FindObjectById(request.getFieldAsUInt32(VID_OBJECT_ID));
   TCHAR *script = request.getFieldAsString(VID_SCRIPT);
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
                  int count = request.getFieldAsInt16(VID_NUM_FIELDS);
                  if (count > 0)
                  {
                     inputFields = new StringMap();
                     uint32_t fieldId = VID_FIELD_LIST_BASE;
                     for(int i = 0; i < count; i++)
                     {
                        TCHAR *name = request.getFieldAsString(fieldId++);
                        TCHAR *value = request.getFieldAsString(fieldId++);
                        inputFields->setPreallocated(name, value);
                     }
                  }
                  else
                  {
                     inputFields = nullptr;
                  }

                  Alarm *alarm = FindAlarmById(request.getFieldAsUInt32(VID_ALARM_ID));
                  if ((alarm != nullptr) && !object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ_ALARMS) && !alarm->checkCategoryAccess(this))
                  {
                     response.setField(VID_RCC, RCC_ACCESS_DENIED);
                     sendMessage(response);
                     delete alarm;
                     delete inputFields;
                     return;
                  }
                  String expScript = object->expandText(script, alarm, nullptr, shared_ptr<DCObjectInfo>(), m_loginName, nullptr, nullptr, inputFields, nullptr);
                  MemFree(script);
                  script = MemCopyString(expScript);
                  delete alarm;
                  delete inputFields;
               }

               args = ParseCommandLine(script);
               if (args->size() > 0)
               {
                  NXSL_Environment *env = withOutput ? new NXSL_ClientSessionEnv(this, &response) : new NXSL_ServerEnv();
                  vm = GetServerScriptLibrary()->createVM(args->get(0), env);
                  if (vm != nullptr)
                  {
                     SetupServerScriptVM(vm, object, shared_ptr<DCObjectInfo>());
                     WriteAuditLog(AUDIT_OBJECTS, true, m_dwUserId, m_workstation, m_id, object->getId(), _T("'%s' script successfully executed."), CHECK_NULL(script));
                     response.setField(VID_RCC, RCC_SUCCESS);
                     sendMessage(response);
                     success = true;
                  }
                  else
                  {
                     response.setField(VID_RCC, RCC_INVALID_SCRIPT_NAME);
                  }
               }
               else
               {
                  response.setField(VID_RCC, RCC_INVALID_ARGUMENT);
               }
            }
            else
            {
               response.setField(VID_RCC, RCC_INVALID_ARGUMENT);
            }
         }
         else  // User doesn't have CONTROL rights on object
         {
			   writeAuditLog(AUDIT_OBJECTS, false, object->getId(), _T("Access denied on executing library script \"%s\" on object %s [%u]"),
                  CHECK_NULL(script), object->getName(), object->getId());
            response.setField(VID_RCC, RCC_ACCESS_DENIED);
         }
      }
      else     // Object is not a node
      {
         response.setField(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
      }
   }
   else  // No object with given ID
   {
      response.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   // start execution
   if (success)
   {
      writeAuditLog(AUDIT_OBJECTS, true, object->getId(), _T("Executed library script \"%s\" for object %s [%u]"), script, object->getName(), object->getId());

      ObjectRefArray<NXSL_Value> sargs(args->size() - 1, 1);
      for(int i = 1; i < args->size(); i++)
         sargs.add(vm->createValue(args->get(i)));
      response.setCode(CMD_EXECUTE_SCRIPT_UPDATE);

      if (vm->run(sargs))
      {
         if (withOutput)
         {
            TCHAR buffer[1024];
            const TCHAR *value = vm->getResult()->getValueAsCString();
            _sntprintf(buffer, 1024, _T("\n\n*** FINISHED ***\n\nResult: %s\n\n"), CHECK_NULL(value));
            response.setField(VID_MESSAGE, buffer);
         }
         response.setField(VID_RCC, RCC_SUCCESS);
         response.setEndOfSequence();
         sendMessage(response);
      }
      else
      {
         response.setField(VID_ERROR_TEXT, vm->getErrorText());
         response.setField(VID_RCC, RCC_NXSL_EXECUTION_ERROR);
         response.setEndOfSequence();
         sendMessage(response);

         if (!request.getFieldAsBoolean(VID_DEVELOPMENT_MODE))
            ReportScriptError(SCRIPT_CONTEXT_CLIENT, object.get(), 0, vm->getErrorText(), script);
      }
      delete vm;
   }
   else
   {
      sendMessage(response);
   }

   MemFree(script);
   delete args;
}

/**
 * Send list of server jobs
 */
void ClientSession::sendJobList(const NXCPMessage& request)
{
	NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());
	response.setField(VID_RCC, RCC_SUCCESS);
	GetJobList(&response);
	sendMessage(response);
}

/**
 * Cancel server job
 */
void ClientSession::cancelJob(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());
   response.setField(VID_RCC, CancelJob(m_dwUserId, request));
   sendMessage(response);
}

/**
 * put server job on hold
 */
void ClientSession::holdJob(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());
   response.setField(VID_RCC, HoldJob(m_dwUserId, request));
   sendMessage(response);
}

/**
 * Allow server job on hold for execution
 */
void ClientSession::unholdJob(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());
	response.setField(VID_RCC, UnholdJob(m_dwUserId, request));
	sendMessage(response);
}

/**
 * Get custom attribute for current user
 */
void ClientSession::getUserCustomAttribute(const NXCPMessage& request)
{
	NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

	TCHAR *name = request.getFieldAsString(VID_NAME);
	if ((name != nullptr) && (*name == _T('.')))
	{
		const TCHAR *value = GetUserDbObjectAttr(m_dwUserId, name);
		response.setField(VID_VALUE, CHECK_NULL_EX(value));
		response.setField(VID_RCC, RCC_SUCCESS);
	}
	else
	{
		response.setField(VID_RCC, RCC_ACCESS_DENIED);
	}
	MemFree(name);

	sendMessage(&response);
}

/**
 * Set custom attribute for current user
 */
void ClientSession::setUserCustomAttribute(const NXCPMessage& request)
{
	NXCPMessage msg;

	msg.setCode(CMD_REQUEST_COMPLETED);
	msg.setId(request.getId());

	TCHAR *name = request.getFieldAsString(VID_NAME);
	if ((name != nullptr) && (*name == _T('.')))
	{
		TCHAR *value = request.getFieldAsString(VID_VALUE);
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
void ClientSession::openServerLog(const NXCPMessage& request)
{
	NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

	TCHAR name[256];
	request.getFieldAsString(VID_LOG_NAME, name, 256);

	uint32_t rcc;
	int32_t handle = OpenLog(name, this, &rcc);
	if (handle != -1)
	{
		response.setField(VID_RCC, RCC_SUCCESS);
		response.setField(VID_LOG_HANDLE, handle);

		shared_ptr<LogHandle> log = AcquireLogHandleObject(this, handle);
		log->getColumnInfo(&response);
		log->release();
	}
	else
	{
		response.setField(VID_RCC, rcc);
	}

	sendMessage(response);
}

/**
 * Close server log
 */
void ClientSession::closeServerLog(const NXCPMessage& request)
{
	NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

	int handle = request.getFieldAsInt32(VID_LOG_HANDLE);
	response.setField(VID_RCC, CloseLog(this, handle));

	sendMessage(response);
}

/**
 * Query server log
 */
void ClientSession::queryServerLog(const NXCPMessage& request)
{
	NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

	int32_t handle = request.getFieldAsInt32(VID_LOG_HANDLE);
	shared_ptr<LogHandle> log = AcquireLogHandleObject(this, handle);
	if (log != nullptr)
	{
		int64_t rowCount;
		response.setField(VID_RCC, log->query(new LogFilter(request, log.get()), &rowCount, getUserId()) ? RCC_SUCCESS : RCC_DB_FAILURE);
		response.setField(VID_NUM_ROWS, rowCount);
		log->release();
	}
	else
	{
		response.setField(VID_RCC, RCC_INVALID_LOG_HANDLE);
	}

	sendMessage(response);
}

/**
 * Get log data from query result
 */
void ClientSession::getServerLogQueryData(const NXCPMessage& request)
{
	NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());
	Table *data = nullptr;

	int32_t handle = (int)request.getFieldAsUInt32(VID_LOG_HANDLE);
	shared_ptr<LogHandle> log = AcquireLogHandleObject(this, handle);
	if (log != nullptr)
	{
	   int64_t startRow = request.getFieldAsUInt64(VID_START_ROW);
	   int64_t numRows = request.getFieldAsUInt64(VID_NUM_ROWS);
		bool refresh = request.getFieldAsUInt16(VID_FORCE_RELOAD) ? true : false;
		data = log->getData(startRow, numRows, refresh, m_dwUserId); // pass user id from session
		log->release();
		if (data != nullptr)
		{
			response.setField(VID_RCC, RCC_SUCCESS);
		}
		else
		{
			response.setField(VID_RCC, RCC_DB_FAILURE);
		}
	}
	else
	{
		response.setField(VID_RCC, RCC_INVALID_LOG_HANDLE);
	}

	sendMessage(response);

	if (data != nullptr)
	{
		response.setCode(CMD_LOG_DATA);
		int offset = 0;
		do
		{
			response.deleteAllFields();
			offset = data->fillMessage(&response, offset, 200);
			sendMessage(response);
		} while(offset < data->getNumRows());
		delete data;
	}
}

/**
 * Get details for server log record
 */
void ClientSession::getServerLogRecordDetails(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

   int32_t handle = request.getFieldAsInt32(VID_LOG_HANDLE);
   shared_ptr<LogHandle> log = AcquireLogHandleObject(this, handle);
   if (log != nullptr)
   {
      log->getRecordDetails(request.getFieldAsInt64(VID_RECORD_ID), &response);
      log->release();
   }
   else
   {
      response.setField(VID_RCC, RCC_INVALID_LOG_HANDLE);
   }

   sendMessage(response);
}

/**
 * Find connection point for the node
 */
void ClientSession::findNodeConnection(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

	uint32_t objectId = request.getFieldAsUInt32(VID_OBJECT_ID);
	shared_ptr<NetObj> object = FindObjectById(objectId);
	if ((object != nullptr) && !object->isDeleted())
	{
		if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
		{
			debugPrintf(5, _T("findNodeConnection: objectId=%d class=%d name=\"%s\""), objectId, object->getObjectClass(), object->getName());
			shared_ptr<NetObj> cp;
			uint32_t localNodeId, localIfId;
			BYTE localMacAddr[MAC_ADDR_LENGTH];
			int type = 0;
			if (object->getObjectClass() == OBJECT_NODE)
			{
				localNodeId = objectId;
				cp = static_cast<Node&>(*object).findConnectionPoint(&localIfId, localMacAddr, &type);
				response.setField(VID_RCC, RCC_SUCCESS);
			}
			else if (object->getObjectClass() == OBJECT_INTERFACE)
			{
				localNodeId = static_cast<Interface&>(*object).getParentNodeId();
				localIfId = objectId;
				memcpy(localMacAddr, static_cast<Interface&>(*object).getMacAddr().value(), MAC_ADDR_LENGTH);
				cp = FindInterfaceConnectionPoint(MacAddress(localMacAddr, MAC_ADDR_LENGTH), &type);
				response.setField(VID_RCC, RCC_SUCCESS);
			}
         else if (object->getObjectClass() == OBJECT_ACCESSPOINT)
			{
				localNodeId = 0;
				localIfId = 0;
				memcpy(localMacAddr, static_cast<AccessPoint&>(*object).getMacAddr().value(), MAC_ADDR_LENGTH);
				cp = FindInterfaceConnectionPoint(MacAddress(localMacAddr, MAC_ADDR_LENGTH), &type);
				response.setField(VID_RCC, RCC_SUCCESS);
			}
			else
			{
				response.setField(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
			}

			if (cp != nullptr)
         {
            debugPrintf(5, _T("findNodeConnection: cp=%s [%u] type=%d"), cp->getName(), cp->getId(), type);
            shared_ptr<Node> node = (cp->getObjectClass() == OBJECT_INTERFACE) ? static_cast<Interface&>(*cp).getParentNode() : static_cast<AccessPoint&>(*cp).getParentNode();
            if (node != nullptr)
            {
               response.setField(VID_OBJECT_ID, node->getId());
				   response.setField(VID_INTERFACE_ID, cp->getId());
               response.setField(VID_IF_INDEX, (cp->getObjectClass() == OBJECT_INTERFACE) ? static_cast<Interface&>(*cp).getIfIndex() : (UINT32)0);
				   response.setField(VID_LOCAL_NODE_ID, localNodeId);
				   response.setField(VID_LOCAL_INTERFACE_ID, localIfId);
				   response.setField(VID_MAC_ADDR, localMacAddr, MAC_ADDR_LENGTH);
				   response.setField(VID_CONNECTION_TYPE, (UINT16)type);
               if (cp->getObjectClass() == OBJECT_INTERFACE)
                  debugPrintf(5, _T("findNodeConnection: nodeId=%u ifId=%u ifName=%s ifIndex=%u"), node->getId(), cp->getId(), cp->getName(), static_cast<Interface&>(*cp).getIfIndex());
               else
                  debugPrintf(5, _T("findNodeConnection: nodeId=%u apId=%u apName=%s"), node->getId(), cp->getId(), cp->getName());
            }
            else
            {
               debugPrintf(5, _T("findNodeConnection: cp=(null)"));
      			response.setField(VID_RCC, RCC_INTERNAL_ERROR);
            }
			}
		}
		else
		{
			response.setField(VID_RCC, RCC_ACCESS_DENIED);
		}
	}
	else
	{
		response.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
	}

	sendMessage(&response);
}

/**
 * Find nodes and/or connection ports for given MAC address pattern
 */
void ClientSession::findMacAddress(const NXCPMessage& request)
{
   BYTE macPattern[MAC_ADDR_LENGTH];
   size_t macPatternSize = request.getFieldAsBinary(VID_MAC_ADDR, macPattern, MAC_ADDR_LENGTH);
   int searchLimit = request.getFieldAsInt32(VID_MAX_RECORDS);

   ObjectArray<MacAddressInfo> icpl(0, 16, Ownership::True);
   FindMacAddresses(macPattern, macPatternSize, &icpl, searchLimit);

   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());
   response.setField(VID_RCC, RCC_SUCCESS);

   if (icpl.isEmpty())
   {
      response.setField(VID_NUM_ELEMENTS, 0);
   }
   else
   {
      response.setField(VID_NUM_ELEMENTS, icpl.size());
      uint32_t base = VID_ELEMENT_LIST_BASE;
      for (int i = 0; i < icpl.size(); i++, base += 10)
         icpl.get(i)->fillMessage(&response, base);
   }
   sendMessage(&response);
}

/**
 * Find connection port for given IP address
 */
void ClientSession::findIpAddress(const NXCPMessage& request)
{
   TCHAR ipAddrText[64];
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request.getId());
	msg.setField(VID_RCC, RCC_SUCCESS);

	MacAddress macAddr;
	bool found = false;
	int32_t zoneUIN = request.getFieldAsUInt32(VID_ZONE_UIN);
	InetAddress ipAddr = request.getFieldAsInetAddress(VID_IP_ADDRESS);
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
               debugPrintf(5, _T("findIpAddress(%s): nodeId=%u ifId=%u ifName=%s ifIndex=%u"), ipAddr.toString(ipAddrText),
                        node->getId(), cp->getId(), cp->getName(), static_cast<Interface&>(*cp).getIfIndex());
            }
            else
            {
               debugPrintf(5, _T("findIpAddress(%s): nodeId=%u apId=%u apName=%s"), ipAddr.toString(ipAddrText), node->getId(), cp->getId(), cp->getName());
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
void ClientSession::findHostname(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());
   response.setField(VID_RCC, RCC_SUCCESS);

   int32_t zoneUIN = request.getFieldAsUInt32(VID_ZONE_UIN);
   TCHAR hostname[MAX_DNS_NAME];
   request.getFieldAsString(VID_HOSTNAME, hostname, MAX_DNS_NAME);

   unique_ptr<SharedObjectArray<NetObj>> nodes = FindNodesByHostname(zoneUIN, hostname);

   response.setField(VID_NUM_ELEMENTS, nodes->size());

   uint32_t fieldId = VID_ELEMENT_LIST_BASE;
   for(int i = 0; i < nodes->size(); i++)
   {
      response.setField(fieldId++, nodes->get(i)->getId());
   }

   sendMessage(&response);
}

/**
 * Send image from library to client
 */
void ClientSession::sendLibraryImage(const NXCPMessage& request)
{
	NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

	TCHAR guidText[64], absFileName[MAX_PATH];
	uint32_t rcc = RCC_SUCCESS;

	uuid_t guid;
	request.getFieldAsBinary(VID_GUID, guid, UUID_LENGTH);
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

				response.setField(VID_GUID, guid, UUID_LENGTH);

				DBGetField(result, 0, 0, buffer, MAX_DB_STRING);	// image name
				response.setField(VID_NAME, buffer);
				DBGetField(result, 0, 1, buffer, MAX_DB_STRING);	// category
				response.setField(VID_CATEGORY, buffer);
				DBGetField(result, 0, 2, buffer, MAX_DB_STRING);	// mime type
				response.setField(VID_IMAGE_MIMETYPE, buffer);

				response.setField(VID_IMAGE_PROTECTED, (WORD)DBGetFieldLong(result, 0, 3));

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

	response.setField(VID_RCC, rcc);
	sendMessage(&response);

	if (rcc == RCC_SUCCESS)
		sendFile(absFileName, request.getId(), 0);
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
void ClientSession::updateLibraryImage(const NXCPMessage& request)
{
	NXCPMessage msg(CMD_REQUEST_COMPLETED, request.getId());
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

	uuid guid = request.getFieldAsGUID(VID_GUID);
	if (guid.isNull())
	   guid = uuid::generate();

	TCHAR guidText[64];
	guid.toString(guidText);

	request.getFieldAsString(VID_NAME, name, MAX_OBJECT_NAME);
	request.getFieldAsString(VID_CATEGORY, category, MAX_OBJECT_NAME);
	request.getFieldAsString(VID_IMAGE_MIMETYPE, mimetype, MAX_DB_STRING);

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
            rcc = RCC_PROTECTED_IMAGE;
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

            ServerDownloadFileInfo *dInfo = new ServerDownloadFileInfo(absFileName,
               [guid] (const TCHAR *fileName, uint32_t uploadData, bool success) -> void
               {
                  if (success)
                     EnumerateClientSessions([] (ClientSession *pSession, void *arg) { pSession->onLibraryImageChange(*((const uuid *)arg), false); }, (void *)&guid);
               });
            if (dInfo->open())
            {
               m_downloadFileMap.set(request.getId(), dInfo);
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
void ClientSession::deleteLibraryImage(const NXCPMessage& request)
{
	NXCPMessage msg(CMD_REQUEST_COMPLETED, request.getId());

   if (!checkSysAccessRights(SYSTEM_ACCESS_MANAGE_IMAGE_LIB))
   {
	   msg.setField(VID_RCC, RCC_ACCESS_DENIED);
      sendMessage(msg);
      return;
   }

	uuid guid = request.getFieldAsGUID(VID_GUID);
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
            rcc = RCC_PROTECTED_IMAGE;
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
	sendMessage(msg);

	if (rcc == RCC_SUCCESS)
	{
		EnumerateClientSessions(ImageLibraryDeleteCallback, &guid);
	}
}

/**
 * Send list of available images (in category)
 */
void ClientSession::listLibraryImages(const NXCPMessage& request)
{
	NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

	uint32_t rcc = RCC_SUCCESS;

   TCHAR category[MAX_DB_STRING];
	if (request.isFieldExist(VID_CATEGORY))
	{
		request.getFieldAsString(VID_CATEGORY, category, MAX_DB_STRING);
	}
	else
	{
		category[0] = 0;
	}
	debugPrintf(5, _T("listLibraryImages: category=%s"), category[0] == 0 ? _T("*ANY*") : category);

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   StringBuffer query(_T("SELECT guid,name,category,mimetype,protected FROM images"));
	if (category[0] != 0)
	{
		query.append(_T(" WHERE category="));
      query.append(DBPrepareString(hdb, category));
	}

	DB_RESULT result = DBSelect(hdb, query);
	if (result != nullptr)
	{
		int count = DBGetNumRows(result);
		response.setField(VID_NUM_RECORDS, static_cast<uint32_t>(count));

		TCHAR buffer[MAX_DB_STRING];
		uint32_t varId = VID_IMAGE_LIST_BASE;
		for (int i = 0; i < count; i++)
		{
			uuid guid = DBGetFieldGUID(result, i, 0);	// guid
			response.setField(varId++, guid);

			DBGetField(result, i, 1, buffer, MAX_DB_STRING);	// image name
			response.setField(varId++, buffer);

			DBGetField(result, i, 2, buffer, MAX_DB_STRING);	// category
			response.setField(varId++, buffer);

			DBGetField(result, i, 3, buffer, MAX_DB_STRING);	// mime type
			response.setField(varId++, buffer);

			response.setField(varId++, static_cast<uint16_t>(DBGetFieldLong(result, i, 4))); // protected flag
		}

		DBFreeResult(result);
	}
	else
	{
		rcc = RCC_DB_FAILURE;
	}

   DBConnectionPoolReleaseConnection(hdb);
	response.setField(VID_RCC, rcc);
	sendMessage(response);
}

/**
 * Execute server side command on object
 */
void ClientSession::executeServerCommand(const NXCPMessage& request)
{
	NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

	uint32_t nodeId = request.getFieldAsUInt32(VID_OBJECT_ID);
	shared_ptr<NetObj> object = FindObjectById(nodeId);
	if (object != nullptr)
	{
		if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_CONTROL))
		{
			if ((object->getObjectClass() == OBJECT_NODE) || (object->getObjectClass() == OBJECT_CONTAINER) || (object->getObjectClass() == OBJECT_SERVICEROOT) ||
			    (object->getObjectClass() == OBJECT_SUBNET) || (object->getObjectClass() == OBJECT_CLUSTER) || (object->getObjectClass() == OBJECT_ZONE))
			{
			   shared_ptr<ServerCommandExecutor> commandExecutor = ServerCommandExecutor::createFromMessage(request, this);
            uint32_t taskId = commandExecutor->getId();
			   m_serverCommands.put(taskId, commandExecutor);
			   if (commandExecutor->execute())
			   {
			      debugPrintf(5, _T("Started process executor %u for command %s, node id %d"), taskId, commandExecutor->getMaskedCommand(), nodeId);
               writeAuditLog(AUDIT_OBJECTS, true, nodeId, _T("Server command executed: %s"), commandExecutor->getMaskedCommand());
               response.setField(VID_COMMAND_ID, taskId);
               response.setField(VID_RCC, RCC_SUCCESS);
			   }
			   else
			   {
	            m_serverCommands.remove(taskId);
               response.setField(VID_RCC, RCC_INTERNAL_ERROR);
			   }
			}
			else
			{
				response.setField(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
			}
		}
		else
		{
			response.setField(VID_RCC, RCC_ACCESS_DENIED);
			writeAuditLog(AUDIT_OBJECTS, false, nodeId, _T("Access denied on server command execution"));
		}
	}
	else
	{
		response.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
	}

	sendMessage(response);
}

/**
 * Stop server command
 */
void ClientSession::stopServerCommand(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

   shared_ptr<ProcessExecutor> cmd = m_serverCommands.get(static_cast<pid_t>(request.getFieldAsUInt64(VID_COMMAND_ID)));
   if (cmd != nullptr)
   {
      cmd->stop();
      response.setField(VID_RCC, RCC_SUCCESS);
   }
   else
   {
      response.setField(VID_RCC, RCC_INVALID_REQUEST);
   }

   sendMessage(response);
}

/**
 * Wait for process executor completion
 */
static void WaitForExecutorCompletion(const shared_ptr<ProcessExecutor>& executor)
{
   executor->waitForCompletion(INFINITE);
   nxlog_debug_tag(DEBUG_TAG, 5, _T("Process executor %u completed"), executor->getId());
}

/**
 * Unregister server command (expected to be called from executor itself)
 */
void ClientSession::unregisterServerCommand(pid_t taskId)
{
   shared_ptr<ProcessExecutor> executor = m_serverCommands.get(taskId);
   if (executor != nullptr)
   {
      m_serverCommands.remove(taskId);
      ThreadPoolExecuteSerialized(g_clientThreadPool, _T("UnregisterCmd"), WaitForExecutorCompletion, executor);
   }
}

/**
 * Upload file from server to agent
 */
void ClientSession::uploadFileToAgent(const NXCPMessage& request)
{
	NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

	uint32_t nodeId = request.getFieldAsUInt32(VID_OBJECT_ID);
	shared_ptr<NetObj> object = FindObjectById(nodeId);
	if (object != nullptr)
	{
		if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_CONTROL))
		{
			if (object->getObjectClass() == OBJECT_NODE)
			{
				TCHAR *localFile = request.getFieldAsString(VID_FILE_NAME);
				TCHAR *remoteFile = request.getFieldAsString(VID_DESTINATION_FILE_NAME);
				if (localFile != nullptr)
				{
					auto job = new FileUploadJob(static_pointer_cast<Node>(object), localFile, remoteFile, m_dwUserId, request.getFieldAsBoolean(VID_CREATE_JOB_ON_HOLD));
					if (AddJob(job))
					{
						writeAuditLog(AUDIT_OBJECTS, true, nodeId, _T("File upload to node %s initiated, local=\"%s\", remote=\"%s\""), object->getName(), CHECK_NULL(localFile), CHECK_NULL(remoteFile));
						response.setField(VID_JOB_ID, job->getId());
						response.setField(VID_RCC, RCC_SUCCESS);
					}
					else
					{
						response.setField(VID_RCC, RCC_INTERNAL_ERROR);
						delete job;
					}
				}
				else
				{
					response.setField(VID_RCC, RCC_INVALID_ARGUMENT);
				}
				MemFree(localFile);
				MemFree(remoteFile);
			}
			else
			{
				response.setField(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
			}
		}
		else
		{
			response.setField(VID_RCC, RCC_ACCESS_DENIED);
			writeAuditLog(AUDIT_OBJECTS, false, nodeId, _T("Access denied on upload file to node %s"), object->getName());
		}
	}
	else
	{
		response.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
	}

	sendMessage(response);
}

/**
 * Send to client list of files in server's file store
 */
void ClientSession::listServerFileStore(const NXCPMessage& request)
{
	NXCPMessage msg;
	msg.setId(request.getId());
	msg.setCode(CMD_REQUEST_COMPLETED);

	int length = (int)request.getFieldAsUInt32(VID_EXTENSION_COUNT);
	debugPrintf(8, _T("ClientSession::listServerFileStore: length of filter type array is %d."), length);

   uint32_t varId = VID_EXTENSION_LIST_BASE;
   StringList extensionList;
   bool soundFiles = (length > 0);
	for(int i = 0; i < length; i++)
   {
	   TCHAR ext[256];
	   request.getFieldAsString(varId++, ext, 256);
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
         uint32_t count = 0, varId = VID_INSTANCE_LIST_BASE;
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
void ClientSession::openConsole(const NXCPMessage& request)
{
	NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

	if (m_systemAccessRights & SYSTEM_ACCESS_SERVER_CONSOLE)
	{
      if (!(m_flags & CSF_CONSOLE_OPEN))
      {
         InterlockedOr(&m_flags, CSF_CONSOLE_OPEN);
         m_console = new ClientSessionConsole(this);
      }
      response.setField(VID_RCC, RCC_SUCCESS);
	}
	else
	{
		response.setField(VID_RCC, RCC_ACCESS_DENIED);
	}

	sendMessage(response);
}

/**
 * Close server console
 */
void ClientSession::closeConsole(const NXCPMessage& request)
{
	NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

	if (m_systemAccessRights & SYSTEM_ACCESS_SERVER_CONSOLE)
	{
		if (m_flags & CSF_CONSOLE_OPEN)
		{
		   InterlockedAnd(&m_flags, ~CSF_CONSOLE_OPEN);
			delete_and_null(m_console);
			response.setField(VID_RCC, RCC_SUCCESS);
		}
		else
		{
			response.setField(VID_RCC, RCC_OUT_OF_STATE_REQUEST);
		}
	}
	else
	{
		response.setField(VID_RCC, RCC_ACCESS_DENIED);
	}

	sendMessage(response);
}

/**
 * Process console command
 */
void ClientSession::processConsoleCommand(const NXCPMessage& request)
{
	NXCPMessage msg(CMD_REQUEST_COMPLETED, request.getId());

	if ((m_systemAccessRights & SYSTEM_ACCESS_SERVER_CONSOLE) && (m_flags & CSF_CONSOLE_OPEN))
	{
		TCHAR command[256];
		request.getFieldAsString(VID_COMMAND, command, 256);
		int rc = ProcessConsoleCommand(command, m_console);
      switch(rc)
      {
         case CMD_EXIT_SHUTDOWN:
            ThreadCreate(InitiateShutdown, ShutdownReason::FROM_REMOTE_CONSOLE);
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
void ClientSession::getVlans(const NXCPMessage& request)
{
	NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

	shared_ptr<NetObj> object = FindObjectById(request.getFieldAsUInt32(VID_OBJECT_ID));
	if (object != nullptr)
	{
		if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
		{
			if (object->getObjectClass() == OBJECT_NODE)
			{
				shared_ptr<VlanList> vlans = static_cast<Node&>(*object).getVlans();
				if (vlans != nullptr)
				{
					vlans->fillMessage(&response);
					response.setField(VID_RCC, RCC_SUCCESS);
		         writeAuditLog(AUDIT_OBJECTS, true, object->getId(), _T("VLAN information read"));
				}
				else
				{
					response.setField(VID_RCC, RCC_RESOURCE_NOT_AVAILABLE);
				}
			}
			else
			{
				response.setField(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
			}
		}
		else
		{
			response.setField(VID_RCC, RCC_ACCESS_DENIED);
			writeAuditLog(AUDIT_OBJECTS, false, object->getId(), _T("Access denied on reading VLAN information"));
		}
	}
	else
	{
		response.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
	}

	sendMessage(response);
}

/**
 * Receive file from client
 */
void ClientSession::receiveFile(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

	if (m_systemAccessRights & SYSTEM_ACCESS_MANAGE_SERVER_FILES)
   {
		TCHAR fileName[MAX_PATH];
		TCHAR fullPath[MAX_PATH];

      request.getFieldAsString(VID_FILE_NAME, fileName, MAX_PATH);
      const TCHAR *cleanFileName = GetCleanFileName(fileName);
      _tcscpy(fullPath, g_netxmsdDataDir);
      _tcscat(fullPath, DDIR_FILES);
      _tcscat(fullPath, FS_PATH_SEPARATOR);
      _tcscat(fullPath, cleanFileName);

      ServerDownloadFileInfo *fInfo = new ServerDownloadFileInfo(fullPath, request.getFieldAsTime(VID_MODIFICATION_TIME));
      if (fInfo->open())
      {
         m_downloadFileMap.set(request.getId(), fInfo);
         response.setField(VID_RCC, RCC_SUCCESS);
         writeAuditLog(AUDIT_SYSCFG, true, 0, _T("Started upload of file \"%s\" to server"), fileName);
         NotifyClientSessions(NX_NOTIFY_FILE_LIST_CHANGED, 0);
      }
      else
      {
         delete fInfo;
         response.setField(VID_RCC, RCC_IO_ERROR);
      }
   }
   else
   {
      writeAuditLog(AUDIT_SYSCFG, false, 0, _T("Access denied on upload file to server"));
      response.setField(VID_RCC, RCC_ACCESS_DENIED);
   }

   sendMessage(response);
}

/**
 * Rename file in store
 */
void ClientSession::renameFile(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

	if (m_systemAccessRights & SYSTEM_ACCESS_MANAGE_SERVER_FILES)
   {
		TCHAR oldFileName[MAX_PATH];
      request.getFieldAsString(VID_FILE_NAME, oldFileName, MAX_PATH);

      TCHAR newFileName[MAX_PATH];
      request.getFieldAsString(VID_NEW_FILE_NAME, newFileName, MAX_PATH);

      TCHAR fullPathOld[MAX_PATH], fullPathNew[MAX_PATH];
      _tcslcpy(fullPathOld, g_netxmsdDataDir, MAX_PATH);
      _tcslcat(fullPathOld, DDIR_FILES, MAX_PATH);
      _tcslcat(fullPathOld, FS_PATH_SEPARATOR, MAX_PATH);
      _tcscpy(fullPathNew, fullPathOld);
      _tcslcat(fullPathOld, GetCleanFileName(oldFileName), MAX_PATH);
      _tcslcat(fullPathNew, GetCleanFileName(newFileName), MAX_PATH);

      if (_trename(fullPathOld, fullPathNew) == 0)
      {
         NotifyClientSessions(NX_NOTIFY_FILE_LIST_CHANGED, 0);
         response.setField(VID_RCC, RCC_SUCCESS);
      }
      else
      {
         response.setField(VID_RCC, RCC_IO_ERROR);
      }
   }
   else
   {
      response.setField(VID_RCC, RCC_ACCESS_DENIED);
   }

   sendMessage(response);
}

/**
 * Delete file in store
 */
void ClientSession::deleteFile(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

   if (m_systemAccessRights & SYSTEM_ACCESS_MANAGE_SERVER_FILES)
   {
      TCHAR fileName[MAX_PATH];
      request.getFieldAsString(VID_FILE_NAME, fileName, MAX_PATH);

      TCHAR fullPath[MAX_PATH];
      _tcslcpy(fullPath, g_netxmsdDataDir, MAX_PATH);
      _tcslcat(fullPath, DDIR_FILES, MAX_PATH);
      _tcslcat(fullPath, FS_PATH_SEPARATOR, MAX_PATH);
      _tcslcat(fullPath, GetCleanFileName(fileName), MAX_PATH);

      if (_tunlink(fullPath) == 0)
      {
         NotifyClientSessions(NX_NOTIFY_FILE_LIST_CHANGED, 0);
         response.setField(VID_RCC, RCC_SUCCESS);
      }
      else
      {
         response.setField(VID_RCC, RCC_IO_ERROR);
      }
   }
   else
   {
      response.setField(VID_RCC, RCC_ACCESS_DENIED);
   }

   sendMessage(response);
}

/**
 * Get network path between two nodes
 */
void ClientSession::getNetworkPath(const NXCPMessage& request)
{
	NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

	shared_ptr<NetObj> node1 = FindObjectById(request.getFieldAsUInt32(VID_SOURCE_OBJECT_ID));
	shared_ptr<NetObj> node2 = FindObjectById(request.getFieldAsUInt32(VID_DESTINATION_OBJECT_ID));

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
					response.setField(VID_RCC, RCC_SUCCESS);
					path->fillMessage(&response);
				}
				else
				{
					response.setField(VID_RCC, RCC_INTERNAL_ERROR);
				}
			}
			else
			{
				response.setField(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
			}
		}
		else
		{
			response.setField(VID_RCC, RCC_ACCESS_DENIED);
		}
	}
	else
	{
		response.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
	}

	sendMessage(response);
}

/**
 * Get physical components of the node
 */
void ClientSession::getNodeComponents(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

   // Get node id and check object class and access rights
   shared_ptr<NetObj> node = FindObjectById(request.getFieldAsUInt32(VID_OBJECT_ID), OBJECT_NODE);
   if (node != nullptr)
   {
      if (node->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
      {
			shared_ptr<ComponentTree> components = static_cast<Node&>(*node).getComponents();
			if (components != nullptr)
			{
				response.setField(VID_RCC, RCC_SUCCESS);
				components->fillMessage(&response, VID_COMPONENT_LIST_BASE);
			}
			else
			{
				response.setField(VID_RCC, RCC_NO_COMPONENT_DATA);
			}
      }
      else
      {
         response.setField(VID_RCC, RCC_ACCESS_DENIED);
      }
   }
   else  // No object with given ID
   {
      response.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   sendMessage(response);
}

/**
 * Get list of software packages installed on node
 */
void ClientSession::getNodeSoftware(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

   // Get node id and check object class and access rights
   shared_ptr<NetObj> node = FindObjectById(request.getFieldAsUInt32(VID_OBJECT_ID), OBJECT_NODE);
   if (node != nullptr)
   {
      if (node->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
      {
         static_cast<Node&>(*node).writePackageListToMessage(&response);
      }
      else
      {
         response.setField(VID_RCC, RCC_ACCESS_DENIED);
      }
   }
   else  // No object with given ID
   {
      response.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   sendMessage(response);
}

/**
 * Get list of hardware installed on node
 */
void ClientSession::getNodeHardware(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

   shared_ptr<NetObj> node = FindObjectById(request.getFieldAsUInt32(VID_OBJECT_ID), OBJECT_NODE);
   if (node != nullptr)
   {
      if (node->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
         static_cast<Node&>(*node).writeHardwareListToMessage(&response);
      else
         response.setField(VID_RCC, RCC_ACCESS_DENIED);
   }
   else
   {
      response.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   sendMessage(response);
}

/**
 * Get list of Windows performance objects supported by node
 */
void ClientSession::getWinPerfObjects(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

   // Get node id and check object class and access rights
   shared_ptr<NetObj> node = FindObjectById(request.getFieldAsUInt32(VID_OBJECT_ID), OBJECT_NODE);
   if (node != nullptr)
   {
      if (node->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
      {
			static_cast<Node&>(*node).writeWinPerfObjectsToMessage(&response);
      }
      else
      {
         response.setField(VID_RCC, RCC_ACCESS_DENIED);
      }
   }
   else  // No object with given ID
   {
      response.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   sendMessage(response);
}

/**
 * Get threshold summary for underlying data collection targets
 */
void ClientSession::getThresholdSummary(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

   // Get object id and check object class and access rights
   shared_ptr<NetObj> object = FindObjectById(request.getFieldAsUInt32(VID_OBJECT_ID));
   if (object != nullptr)
   {
      if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
      {
			if (object->showThresholdSummary())
			{
				auto targets = new SharedObjectArray<DataCollectionTarget>();
				object->addChildDCTargetsToList(targets, m_dwUserId);
				uint32_t fieldId = VID_THRESHOLD_BASE;
				for(int i = 0; i < targets->size(); i++)
				{
					if (targets->get(i)->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
						fieldId = targets->get(i)->getThresholdSummary(&response, fieldId, m_dwUserId);
				}
				delete targets;
			}
			else
			{
	         response.setField(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
			}
      }
      else
      {
         response.setField(VID_RCC, RCC_ACCESS_DENIED);
      }
   }
   else  // No object with given ID
   {
      response.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   sendMessage(response);
}

/**
 * List configured mapping tables
 */
void ClientSession::listMappingTables(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());
	if (m_systemAccessRights & SYSTEM_ACCESS_MANAGE_MAPPING_TBLS)
	{
		response.setField(VID_RCC, ListMappingTables(&response));
	}
	else
	{
		response.setField(VID_RCC, RCC_ACCESS_DENIED);
		writeAuditLog(AUDIT_SYSCFG, false, 0, _T("Access denied on reading mapping table list"));
	}
   sendMessage(response);
}

/**
 * Get content of specific mapping table
 */
void ClientSession::getMappingTable(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());
	response.setField(VID_RCC, GetMappingTable((LONG)request.getFieldAsUInt32(VID_MAPPING_TABLE_ID), &response));
   sendMessage(response);
}

/**
 * Create or update mapping table
 */
void ClientSession::updateMappingTable(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());
	if (m_systemAccessRights & SYSTEM_ACCESS_MANAGE_MAPPING_TBLS)
	{
		int32_t id;
		response.setField(VID_RCC, UpdateMappingTable(request, &id, this));
		response.setField(VID_MAPPING_TABLE_ID, (UINT32)id);
	}
	else
	{
		response.setField(VID_RCC, RCC_ACCESS_DENIED);
      writeAuditLog(AUDIT_SYSCFG, false, 0, _T("Access denied on updating mapping table [%d]"), request.getFieldAsInt32(VID_MAPPING_TABLE_ID));
	}
   sendMessage(response);
}

/**
 * Delete mapping table
 */
void ClientSession::deleteMappingTable(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());
	if (m_systemAccessRights & SYSTEM_ACCESS_MANAGE_MAPPING_TBLS)
	{
		response.setField(VID_RCC, DeleteMappingTable(request.getFieldAsInt32(VID_MAPPING_TABLE_ID), this));
	}
	else
	{
		response.setField(VID_RCC, RCC_ACCESS_DENIED);
      writeAuditLog(AUDIT_SYSCFG, false, 0, _T("Access denied on deleting mapping table [%d]"), request.getFieldAsInt32(VID_MAPPING_TABLE_ID));
	}
   sendMessage(response);
}

/**
 * Get list of wireless stations registered on controller
 */
void ClientSession::getWirelessStations(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

   // Get object id and check object class and access rights
	shared_ptr<NetObj> node = FindObjectById(request.getFieldAsUInt32(VID_OBJECT_ID), OBJECT_NODE);
   if (node != nullptr)
   {
      if (node->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
      {
			if (static_cast<Node&>(*node).isWirelessController())
			{
			   static_cast<Node&>(*node).writeWsListToMessage(&response);
	         response.setField(VID_RCC, RCC_SUCCESS);
			}
			else
			{
	         response.setField(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
			}
      }
      else
      {
         response.setField(VID_RCC, RCC_ACCESS_DENIED);
      }
   }
   else  // No object with given ID
   {
      response.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   sendMessage(response);
}

/**
 * Get list of configured DCI summary tables
 */
void ClientSession::getSummaryTables(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_RESULT hResult = DBSelect(hdb, _T("SELECT id,menu_path,title,flags,guid FROM dci_summary_tables"));
   if (hResult != nullptr)
   {
      TCHAR buffer[256];
      int count = DBGetNumRows(hResult);
      response.setField(VID_NUM_ELEMENTS, (UINT32)count);
      uint32_t varId = VID_ELEMENT_LIST_BASE;
      for(int i = 0; i < count; i++)
      {
         response.setField(varId++, (UINT32)DBGetFieldLong(hResult, i, 0));
         response.setField(varId++, DBGetField(hResult, i, 1, buffer, 256));
         response.setField(varId++, DBGetField(hResult, i, 2, buffer, 256));
         response.setField(varId++, (UINT32)DBGetFieldLong(hResult, i, 3));

         uuid guid = DBGetFieldGUID(hResult, i, 4);
         response.setField(varId++, guid);

         varId += 5;
      }
      DBFreeResult(hResult);
   }
	else
	{
		response.setField(VID_RCC, RCC_ACCESS_DENIED);
	}
   DBConnectionPoolReleaseConnection(hdb);

   sendMessage(response);
}

/**
 * Get details of DCI summary table
 */
void ClientSession::getSummaryTableDetails(const NXCPMessage& request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request.getId());

	if (m_systemAccessRights & SYSTEM_ACCESS_MANAGE_SUMMARY_TBLS)
	{
      LONG id = (LONG)request.getFieldAsUInt32(VID_SUMMARY_TABLE_ID);
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
void ClientSession::modifySummaryTable(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

	if (m_systemAccessRights & SYSTEM_ACCESS_MANAGE_SUMMARY_TBLS)
	{
		uint32_t id;
		response.setField(VID_RCC, ModifySummaryTable(request, &id));
		response.setField(VID_SUMMARY_TABLE_ID, (UINT32)id);
	}
	else
	{
		response.setField(VID_RCC, RCC_ACCESS_DENIED);
	}

   sendMessage(response);
}

/**
 * Delete DCI summary table
 */
void ClientSession::deleteSummaryTable(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

	if (m_systemAccessRights & SYSTEM_ACCESS_MANAGE_SUMMARY_TBLS)
	{
		response.setField(VID_RCC, DeleteSummaryTable((LONG)request.getFieldAsUInt32(VID_SUMMARY_TABLE_ID)));
	}
	else
	{
		response.setField(VID_RCC, RCC_ACCESS_DENIED);
	}

   sendMessage(response);
}

/**
 * Query DCI summary table
 */
void ClientSession::querySummaryTable(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

   uint32_t rcc;
   Table *result = QuerySummaryTable(request.getFieldAsUInt32(VID_SUMMARY_TABLE_ID), nullptr,
         request.getFieldAsUInt32(VID_OBJECT_ID), m_dwUserId, &rcc);
   if (result != nullptr)
   {
      debugPrintf(6, _T("querySummaryTable: %d rows in resulting table"), result->getNumRows());
      response.setField(VID_RCC, RCC_SUCCESS);
      result->fillMessage(&response, 0, -1);
      delete result;
   }
   else
   {
      response.setField(VID_RCC, rcc);
   }

   sendMessage(response);
}

/**
 * Query ad hoc DCI summary table
 */
void ClientSession::queryAdHocSummaryTable(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

   SummaryTable *tableDefinition = new SummaryTable(request);

   uint32_t rcc;
   Table *result = QuerySummaryTable(0, tableDefinition, request.getFieldAsUInt32(VID_OBJECT_ID), m_dwUserId, &rcc);
   if (result != nullptr)
   {
      debugPrintf(6, _T("querySummaryTable: %d rows in resulting table"), result->getNumRows());
      response.setField(VID_RCC, RCC_SUCCESS);
      result->fillMessage(&response, 0, -1);
      delete result;
   }
   else
   {
      response.setField(VID_RCC, rcc);
   }

   sendMessage(response);
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

      request->setField(VID_USER_ID, m_dwUserId);
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
void ClientSession::getSubnetAddressMap(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

   // Get node id and check object class and access rights
   shared_ptr<NetObj> subnet = FindObjectById(request.getFieldAsUInt32(VID_OBJECT_ID), OBJECT_SUBNET);
   if (subnet != nullptr)
   {
      if (subnet->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
      {
         int length;
         UINT32 *map = static_cast<Subnet&>(*subnet).buildAddressMap(&length);
			if (map != nullptr)
			{
				response.setField(VID_RCC, RCC_SUCCESS);
            response.setFieldFromInt32Array(VID_ADDRESS_MAP, (UINT32)length, map);
            MemFree(map);
			}
			else
			{
				response.setField(VID_RCC, RCC_INTERNAL_ERROR);
			}
      }
      else
      {
         response.setField(VID_RCC, RCC_ACCESS_DENIED);
      }
   }
   else  // No object with given ID
   {
      response.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   sendMessage(response);
}

/**
 * Get effective rights for object
 */
void ClientSession::getEffectiveRights(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

   // Get node id and check object class and access rights
   shared_ptr<NetObj> object = FindObjectById(request.getFieldAsUInt32(VID_OBJECT_ID));
   if (object != nullptr)
   {
      response.setField(VID_EFFECTIVE_RIGHTS, object->getUserRights(m_dwUserId));
		response.setField(VID_RCC, RCC_SUCCESS);
   }
   else  // No object with given ID
   {
      response.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   sendMessage(response);
}

/**
 * File manager control calls
 */
void ClientSession::fileManagerControl(NXCPMessage *request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request->getId());
   NXCPMessage *response = nullptr, *responseMessage = &msg;
   uint32_t rcc = RCC_INTERNAL_ERROR;

   TCHAR fileName[MAX_PATH];
   request->getFieldAsString(VID_FILE_NAME, fileName, MAX_PATH);
   uint32_t objectId = request->getFieldAsUInt32(VID_OBJECT_ID);
   shared_ptr<NetObj> object = FindObjectById(objectId);
	if (object != nullptr)
	{
	   if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_MANAGE_FILES) ||
          ((request->getCode() == CMD_GET_FOLDER_CONTENT || request->getCode() == CMD_GET_FOLDER_SIZE) && object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ)) ||
          (request->getCode() == CMD_FILEMGR_GET_FILE_FINGERPRINT && object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_DOWNLOAD)))
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
                        case CMD_FILEMGR_GET_FILE_FINGERPRINT:
                        {
                           writeAuditLog(AUDIT_OBJECTS, true, objectId, _T("Get fingerprint for file \"%s\" on node %s"), fileName, object->getName());
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
               msg.setField(VID_RCC, RCC_NO_CONNECTION_TO_AGENT);
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
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request->getId());
   NXCPMessage *response = nullptr, *responseMessage = &msg;
	uint32_t rcc = RCC_INTERNAL_ERROR;

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
               request->setProtocolVersion(conn->getProtocolVersion());

               conn->sendMessage(request);
               response = conn->waitForMessage(CMD_REQUEST_COMPLETED, request->getId(), 10000);
               if (response != nullptr)
               {
                  rcc = response->getFieldAsUInt32(VID_RCC);
                  if (rcc == ERR_SUCCESS)
                  {
                     writeAuditLog(AUDIT_OBJECTS, true, objectId, _T("Started direct upload of file \"%s\" to node %s"), fileName, object->getName());
                     response->setCode(CMD_REQUEST_COMPLETED);
                     response->setField(VID_ENABLE_COMPRESSION, conn->isCompressionAllowed());
                     response->setField(VID_REPORT_PROGRESS, true);    // Indicate that server will report transfer progress to client
                     responseMessage = response;
                     m_agentFileTransfers.put(request->getId(), make_shared<AgentFileTransfer>(m_id, request->getId(), conn, request->getFieldAsBoolean(VID_REPORT_PROGRESS)));
                  }
                  else if (rcc == ERR_FILE_APPEND_POSSIBLE)
                  {
                     response->setCode(CMD_REQUEST_COMPLETED);
                     response->setField(VID_ENABLE_COMPRESSION, conn->isCompressionAllowed());
                     response->setField(VID_RCC, RCC_FILE_APPEND_POSSIBLE);
                     responseMessage = response;
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
               msg.setField(VID_RCC, RCC_NO_CONNECTION_TO_AGENT);
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
void ClientSession::getSwitchForwardingDatabase(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

   // Get node id and check object class and access rights
   shared_ptr<NetObj> node = FindObjectById(request.getFieldAsUInt32(VID_OBJECT_ID), OBJECT_NODE);
   if (node != nullptr)
   {
      if (node->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
      {
         shared_ptr<ForwardingDatabase> fdb = static_cast<Node&>(*node).getSwitchForwardingDatabase();
         if (fdb != nullptr)
         {
            response.setField(VID_RCC, RCC_SUCCESS);
            fdb->fillMessage(&response);
         }
         else
         {
            response.setField(VID_RCC, RCC_NO_FDB);
         }
      }
      else
      {
         response.setField(VID_RCC, RCC_ACCESS_DENIED);
         writeAuditLog(AUDIT_OBJECTS, false, node->getId(), _T("Access denied on reading FDB"));
      }
   }
   else  // No object with given ID
   {
      response.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   // Send response
   sendMessage(&response);
}

/**
 * Get routing table
 */
void ClientSession::getRoutingTable(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

   // Get node id and check object class and access rights
   shared_ptr<NetObj> node = FindObjectById(request.getFieldAsUInt32(VID_OBJECT_ID), OBJECT_NODE);
   if (node != nullptr)
   {
      if (node->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
      {
         RoutingTable *rt = static_cast<Node&>(*node).getRoutingTable();
         if (rt != nullptr)
         {
            response.setField(VID_RCC, RCC_SUCCESS);
            response.setField(VID_NUM_ELEMENTS, (UINT32)rt->size());
            uint32_t id = VID_ELEMENT_LIST_BASE;
            for(int i = 0; i < rt->size(); i++)
            {
               ROUTE *route = rt->get(i);
               response.setField(id++, route->dwDestAddr);
               response.setField(id++, (UINT32)BitsInMask(route->dwDestMask));
               response.setField(id++, route->dwNextHop);
               response.setField(id++, route->dwIfIndex);
               response.setField(id++, route->dwRouteType);
               shared_ptr<Interface> iface = static_cast<Node&>(*node).findInterfaceByIndex(route->dwIfIndex);
               if (iface != nullptr)
               {
                  response.setField(id++, iface->getName());
               }
               else
               {
                  TCHAR buffer[32];
                  _sntprintf(buffer, 32, _T("[%u]"), route->dwIfIndex);
                  response.setField(id++, buffer);
               }
               id += 4;
            }
            delete rt;
         }
         else
         {
            response.setField(VID_RCC, RCC_NO_ROUTING_TABLE);
         }
      }
      else
      {
         response.setField(VID_RCC, RCC_ACCESS_DENIED);
         WriteAuditLog(AUDIT_OBJECTS, FALSE, m_dwUserId, m_workstation, m_id, node->getId(), _T("Access denied on reading routing table"));
      }
   }
   else  // No object with given ID
   {
      response.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   // Send response
   sendMessage(&response);
}

/**
 * Get location history for object
 */
void ClientSession::getLocationHistory(const NXCPMessage& request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request.getId());

   // Get node id and check object class and access rights
   shared_ptr<NetObj> device = FindObjectById(request.getFieldAsUInt32(VID_OBJECT_ID), OBJECT_MOBILEDEVICE);
   if (device != nullptr)
   {
      if (device->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
      {
         DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
         TCHAR query[256];
         _sntprintf(query, 255, _T("SELECT latitude,longitude,accuracy,start_timestamp,end_timestamp FROM gps_history_%d")
                                             _T(" WHERE start_timestamp<? AND end_timestamp>?"), request.getFieldAsUInt32(VID_OBJECT_ID));

         DB_STATEMENT hStmt = DBPrepare(hdb, query);
         if (hStmt != nullptr)
         {
            DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, (INT64)request.getFieldAsTime(VID_TIME_TO));
            DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, (INT64)request.getFieldAsTime(VID_TIME_FROM));
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
void ClientSession::getScreenshot(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

   TCHAR sessionName[256] = _T("");
   request.getFieldAsString(VID_NAME, sessionName, 256);
   if (sessionName[0] == 0)
      _tcscpy(sessionName, _T("Console"));

   uint32_t objectId = request.getFieldAsUInt32(VID_NODE_ID);
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
               uint32_t rc = conn->takeScreenshot(sessionName, &data, &size);
               if (rc == ERR_SUCCESS)
               {
                  // Avoid writing screenshot audit records for same object too fast
                  if ((m_lastScreenshotObject != objectId) || (time(nullptr) - m_lastScreenshotTime > 3600))
                  {
                     writeAuditLog(AUDIT_OBJECTS, true, objectId, _T("Screenshot taken for session \"%s\""), sessionName);
                     m_lastScreenshotObject = objectId;
                     m_lastScreenshotTime = time(nullptr);
                  }
                  response.setField(VID_RCC, RCC_SUCCESS);
                  response.setField(VID_FILE_DATA, data, size);
               }
               else
               {
                  response.setField(VID_RCC, AgentErrorToRCC(rc));
               }
               MemFree(data);
            }
            else
            {
               response.setField(VID_RCC, RCC_NO_CONNECTION_TO_AGENT);
            }
			}
			else
			{
				response.setField(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
			}
		}
		else
		{
		   writeAuditLog(AUDIT_OBJECTS, false, objectId, _T("Access denied on screenshot for session \"%s\""), sessionName);
			response.setField(VID_RCC, RCC_ACCESS_DENIED);
		}
	}
	else
	{
		response.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
	}

   sendMessage(response);
}

/**
 * Compile script
 */
void ClientSession::compileScript(const NXCPMessage& request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request.getId());

	TCHAR *source = request.getFieldAsString(VID_SCRIPT);
	if (source != nullptr)
	{
      TCHAR errorMessage[256];
      int errorLine;
      NXSL_ServerEnv env;
      NXSL_Program *script = NXSLCompile(source, errorMessage, 256, &errorLine, &env);
      if (script != nullptr)
      {
         msg.setField(VID_COMPILATION_STATUS, (INT16)1);
         if (request.getFieldAsBoolean(VID_SERIALIZE))
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
void ClientSession::cleanAgentDciConfiguration(const NXCPMessage& request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request.getId());

   uint32_t objectId = request.getFieldAsUInt32(VID_NODE_ID);
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
               msg.setField(VID_RCC, RCC_NO_CONNECTION_TO_AGENT);
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
void ClientSession::resyncAgentDciConfiguration(const NXCPMessage& request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request.getId());

   UINT32 objectId = request.getFieldAsUInt32(VID_NODE_ID);
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
void ClientSession::getSchedulerTaskHandlers(const NXCPMessage& request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request.getId());
   GetSchedulerTaskHandlers(&msg, m_systemAccessRights);
   msg.setField(VID_RCC, RCC_SUCCESS);
   sendMessage(&msg);
}

/**
 * List all existing schedules
 */
void ClientSession::getScheduledTasks(const NXCPMessage& request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request.getId());
   GetScheduledTasks(&msg, m_dwUserId, m_systemAccessRights);
   msg.setField(VID_RCC, RCC_SUCCESS);
   sendMessage(&msg);
}

/**
 * Add new schedule
 */
void ClientSession::addScheduledTask(const NXCPMessage& request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request.getId());
   uint32_t result = CreateScheduledTaskFromMsg(request, m_dwUserId, m_systemAccessRights);
   msg.setField(VID_RCC, result);
   sendMessage(&msg);
}

/**
 * Update existing schedule
 */
void ClientSession::updateScheduledTask(const NXCPMessage& request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request.getId());
   uint32_t result = UpdateScheduledTaskFromMsg(request, m_dwUserId, m_systemAccessRights);
   msg.setField(VID_RCC, result);
   sendMessage(&msg);
}

/**
 * Remove/delete schedule
 */
void ClientSession::removeScheduledTask(const NXCPMessage& request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request.getId());
   uint32_t result = DeleteScheduledTask(request.getFieldAsUInt32(VID_SCHEDULED_TASK_ID), m_dwUserId, m_systemAccessRights);
   msg.setField(VID_RCC, result);
   sendMessage(&msg);
}

/**
 * Filter by report ID for schedule task enumeration
 */
static bool ReportingScheduledTaskFilter(const ScheduledTask *task, void *context)
{
   if (_tcscmp(task->getTaskHandlerId(), EXECUTE_REPORT_TASK_ID))
      return false;

   String data = task->getPersistentData();
   ssize_t indexStart = data.find(_T("<reportId>"));
   if (indexStart == String::npos)
      return false;

   ssize_t indexEnd = data.find(_T("</reportId>"), indexStart + 10);
   if (indexEnd == String::npos)
      return false;

   uuid reportId = uuid::parse(data.substring(indexStart + 10, indexEnd - indexStart - 10));
   return reportId.equals(*static_cast<uuid*>(context));
}

/**
 *
 * Get list of scheduled executions for given report
 */
void ClientSession::getScheduledReportingTasks(const NXCPMessage& request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request.getId());
   uuid reportId = request.getFieldAsGUID(VID_REPORT_DEFINITION);
   GetScheduledTasks(&msg, m_dwUserId, m_systemAccessRights, ReportingScheduledTaskFilter, &reportId);
   msg.setField(VID_RCC, RCC_SUCCESS);
   sendMessage(&msg);
}

/**
 * Get list of registered prediction engines
 */
void ClientSession::getPredictionEngines(const NXCPMessage& request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request.getId());
   GetPredictionEngines(&msg);
   msg.setField(VID_RCC, RCC_SUCCESS);
   sendMessage(&msg);
}

/**
 * Get predicted data
 */
void ClientSession::getPredictedData(const NXCPMessage& request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request.getId());
   bool success = false;

   shared_ptr<NetObj> object = FindObjectById(request.getFieldAsUInt32(VID_OBJECT_ID));
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
void ClientSession::getAgentTunnels(const NXCPMessage& request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request.getId());
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
void ClientSession::bindAgentTunnel(const NXCPMessage& request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request.getId());
   if (m_systemAccessRights & SYSTEM_ACCESS_REGISTER_AGENTS)
   {
      UINT32 nodeId = request.getFieldAsUInt32(VID_NODE_ID);
      UINT32 tunnelId = request.getFieldAsUInt32(VID_TUNNEL_ID);
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
void ClientSession::unbindAgentTunnel(const NXCPMessage& request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request.getId());
   if (m_systemAccessRights & SYSTEM_ACCESS_REGISTER_AGENTS)
   {
      UINT32 nodeId = request.getFieldAsUInt32(VID_NODE_ID);
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
void ClientSession::setupTcpProxy(const NXCPMessage& request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request.getId());
   if (m_systemAccessRights & SYSTEM_ACCESS_SETUP_TCP_PROXY)
   {
      InetAddress ipAddr;
      shared_ptr<NetObj> aclObject, proxyObject;
      uint32_t proxyId = request.getFieldAsUInt32(VID_TCP_PROXY);
      if (proxyId != 0)
      {
         ipAddr = request.getFieldAsInetAddress(VID_IP_ADDRESS);
         proxyObject = FindObjectById(proxyId);
         aclObject = proxyObject;
      }
      else
      {
         shared_ptr<NetObj> targetNode = FindObjectById(request.getFieldAsUInt32(VID_NODE_ID), OBJECT_NODE);
         if (targetNode != nullptr)
         {
            ipAddr = targetNode->getPrimaryIpAddress();
            proxyId = static_cast<Node&>(*targetNode).getEffectiveTcpProxy();
            proxyObject = FindObjectById(proxyId);
            aclObject = targetNode;
         }
      }

      if ((aclObject != nullptr) && (proxyObject != nullptr))
      {
         if (aclObject->checkAccessRights(m_dwUserId, OBJECT_ACCESS_CONTROL))
         {
            uint32_t rcc = RCC_SUCCESS;
            shared_ptr<Node> proxyNode;
            if (proxyObject->getObjectClass() == OBJECT_NODE)
            {
               proxyNode = static_pointer_cast<Node>(proxyObject);
            }
            else if (proxyObject->getObjectClass() == OBJECT_ZONE)
            {
               proxyNode = static_pointer_cast<Node>(FindObjectById(static_cast<Zone&>(*proxyObject).getProxyNodeId(nullptr, false), OBJECT_NODE));
               if (proxyNode == nullptr)
               {
                  debugPrintf(4, _T("Requested TCP proxy for zone %s but it doesn't have available proxy nodes"), proxyObject->getName());
                  rcc = RCC_RESOURCE_NOT_AVAILABLE;
               }
            }
            else
            {
               rcc = RCC_INCOMPATIBLE_OPERATION;
            }
            if (rcc == RCC_SUCCESS)
            {
               uint16_t port = request.getFieldAsUInt16(VID_PORT);
               debugPrintf(4, _T("Setting up TCP proxy to %s:%d via node %s [%u]"), ipAddr.toString().cstr(), port, proxyNode->getName(), proxyNode->getId());
               shared_ptr<AgentConnectionEx> conn = proxyNode->createAgentConnection();
               if (conn != nullptr)
               {
                  conn->setTcpProxySession(this);
                  uint32_t agentChannelId;
                  rcc = conn->setupTcpProxy(ipAddr, port, &agentChannelId);
                  if (rcc == ERR_SUCCESS)
                  {
                     uint32_t clientChannelId = InterlockedIncrement(&m_tcpProxyChannelId);
                     m_tcpProxyLock.lock();
                     m_tcpProxyConnections->add(new TcpProxy(conn, agentChannelId, clientChannelId, proxyNode->getId()));
                     m_tcpProxyLock.unlock();
                     msg.setField(VID_RCC, RCC_SUCCESS);
                     msg.setField(VID_CHANNEL_ID, clientChannelId);
                     writeAuditLog(AUDIT_SYSCFG, true, proxyNode->getId(), _T("Created TCP proxy to %s port %d via %s [%u] (client channel %u)"),
                              (const TCHAR *)ipAddr.toString(), (int)port, proxyNode->getName(), proxyNode->getId(), clientChannelId);
                     debugPrintf(3, _T("Created TCP proxy to %s:%d via node %s [%d]"), ipAddr.toString().cstr(), port, proxyNode->getName(), proxyNode->getId());
                  }
                  else
                  {
                     msg.setField(VID_RCC, AgentErrorToRCC(rcc));
                     debugPrintf(4, _T("TCP proxy setup failed (%s)"), AgentErrorCodeToText(rcc));
                  }
               }
               else
               {
                  msg.setField(VID_RCC, RCC_COMM_FAILURE);
                  debugPrintf(4, _T("TCP proxy setup failed (no connection to agent)"));
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
            writeAuditLog(AUDIT_SYSCFG, false, aclObject->getId(), _T("Access denied on setting up TCP proxy"));
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
void ClientSession::closeTcpProxy(const NXCPMessage& request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request.getId());

   uint32_t clientChannelId = request.getFieldAsUInt32(VID_CHANNEL_ID);

   shared_ptr<AgentConnectionEx> conn;
   uint32_t agentChannelId, nodeId;
   m_tcpProxyLock.lock();
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
   m_tcpProxyLock.unlock();

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
void ClientSession::processTcpProxyData(AgentConnectionEx *conn, uint32_t agentChannelId, const void *data, size_t size, bool errorIndicator)
{
   uint32_t clientChannelId = 0;
   m_tcpProxyLock.lock();
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
   m_tcpProxyLock.unlock();

   if (clientChannelId != 0)
   {
      if (size > 0)
      {
         size_t msgSize = size + NXCP_HEADER_SIZE;
         if (msgSize % 8 != 0)
            msgSize += 8 - msgSize % 8;
         NXCP_MESSAGE *msg = static_cast<NXCP_MESSAGE*>(MemAlloc(msgSize));
         msg->code = htons(CMD_TCP_PROXY_DATA);
         msg->flags = htons(MF_BINARY);
         msg->id = htonl(clientChannelId);
         msg->numFields = htonl(static_cast<uint32_t>(size));
         msg->size = htonl(static_cast<uint32_t>(msgSize));
         memcpy(msg->fields, data, size);
         postRawMessageAndDelete(msg);
      }
      else
      {
         NXCPMessage msg(CMD_CLOSE_TCP_PROXY, 0);
         msg.setField(VID_CHANNEL_ID, clientChannelId);
         msg.setField(VID_RCC, errorIndicator ? RCC_REMOTE_SOCKET_READ_ERROR : RCC_SUCCESS);
         postMessage(msg);
      }
   }
}

/**
 * Process disconnect notification from agent session used for TCP proxy
 */
void ClientSession::processTcpProxyAgentDisconnect(AgentConnectionEx *conn)
{
   IntegerArray<uint32_t> clientChannelList;
   m_tcpProxyLock.lock();
   for(int i = 0; i < m_tcpProxyConnections->size(); i++)
   {
      TcpProxy *p = m_tcpProxyConnections->get(i);
      if (p->agentConnection.get() == conn)
      {
         clientChannelList.add(p->clientChannelId);
         debugPrintf(5, _T("TCP proxy channel %u closed because of agent session disconnect"), p->clientChannelId);
         m_tcpProxyConnections->remove(i);
         i--;
      }
   }
   m_tcpProxyLock.unlock();

   if (!clientChannelList.isEmpty())
   {
      NXCPMessage msg(CMD_CLOSE_TCP_PROXY, 0);
      msg.setField(VID_RCC, RCC_COMM_FAILURE);
      for(int i = 0; i < clientChannelList.size(); i++)
      {
         msg.setField(VID_CHANNEL_ID, clientChannelList.get(i));
         postMessage(msg);
      }
   }
}

/**
 * Expand event processing macros
 */
void ClientSession::expandMacros(const NXCPMessage& request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request.getId());

   StringMap inputFields(request, VID_INPUT_FIELD_BASE, VID_INPUT_FIELD_COUNT);

   int fieldCount = request.getFieldAsUInt32(VID_STRING_COUNT);
   uint32_t inFieldId = VID_EXPAND_STRING_BASE;
   uint32_t outFieldId = VID_EXPAND_STRING_BASE;
   for(int i = 0; i < fieldCount; i++, inFieldId += 2, outFieldId++)
   {
      TCHAR *textToExpand = request.getFieldAsString(inFieldId++);
      shared_ptr<NetObj> object = FindObjectById(request.getFieldAsUInt32(inFieldId++));
      if (object == nullptr)
      {
         msg.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
         sendMessage(&msg);
         MemFree(textToExpand);
         return;
      }
      Alarm *alarm = FindAlarmById(request.getFieldAsUInt32(inFieldId++));
      if (!object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ) ||
          ((alarm != nullptr) && !object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ_ALARMS) && !alarm->checkCategoryAccess(this)))
      {
         msg.setField(VID_RCC, RCC_ACCESS_DENIED);
         sendMessage(&msg);
         MemFree(textToExpand);
         delete alarm;
         return;
      }

      String result = object->expandText(textToExpand, alarm, nullptr, shared_ptr<DCObjectInfo>(), m_loginName, nullptr, nullptr, &inputFields, nullptr);
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
void ClientSession::updatePolicy(const NXCPMessage& request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request.getId());

   shared_ptr<NetObj> templateObject = FindObjectById(request.getFieldAsUInt32(VID_TEMPLATE_ID), OBJECT_TEMPLATE);
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
void ClientSession::deletePolicy(const NXCPMessage& request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request.getId());

   shared_ptr<NetObj> templateObject = FindObjectById(request.getFieldAsUInt32(VID_TEMPLATE_ID), OBJECT_TEMPLATE);
   if(templateObject != nullptr)
   {
      if (templateObject->checkAccessRights(m_dwUserId, OBJECT_ACCESS_MODIFY))
      {
         if (static_cast<Template&>(*templateObject).removePolicy(request.getFieldAsGUID(VID_GUID)))
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
void ClientSession::getPolicyList(const NXCPMessage& request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request.getId());

   shared_ptr<NetObj> templateObject = FindObjectById(request.getFieldAsUInt32(VID_TEMPLATE_ID), OBJECT_TEMPLATE);
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
void ClientSession::getPolicy(const NXCPMessage& request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request.getId());

   shared_ptr<NetObj> templateObject = FindObjectById(request.getFieldAsUInt32(VID_TEMPLATE_ID), OBJECT_TEMPLATE);
   if (templateObject != nullptr)
   {
      if (templateObject->checkAccessRights(m_dwUserId, OBJECT_ACCESS_MODIFY))
      {
         uuid guid = request.getFieldAsGUID(VID_GUID);
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
void ClientSession::onPolicyEditorClose(const NXCPMessage& request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request.getId());
   shared_ptr<NetObj> templateObject = FindObjectById(request.getFieldAsUInt32(VID_TEMPLATE_ID), OBJECT_TEMPLATE);
   if(templateObject != nullptr)
   {
      ThreadPoolExecute(g_clientThreadPool, ApplyPolicyChanges, templateObject);
      msg.setField(VID_RCC, RCC_SUCCESS);
   }
   else
   {
      msg.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
   }
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
void ClientSession::forceApplyPolicy(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

   // Get source and destination
   shared_ptr<NetObj> templateObject = FindObjectById(request.getFieldAsUInt32(VID_TEMPLATE_ID), OBJECT_TEMPLATE);
   if (templateObject != nullptr)
   {
      // Check access rights
      if (templateObject->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
      {
         ThreadPoolExecute(g_clientThreadPool, ForceApplyPolicyChanges, templateObject);
         response.setField(VID_RCC, RCC_SUCCESS);
      }
      else  // User doesn't have enough rights on object(s)
      {
         response.setField(VID_RCC, RCC_ACCESS_DENIED);
      }
   }
   else  // No object(s) with given ID
   {
      response.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   sendMessage(response);
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
unique_ptr<SharedObjectArray<DCObject>> ClientSession::resolveDCOsByRegex(UINT32 objectId, const TCHAR *objectNameRegex, const TCHAR *dciRegex, bool searchByName)
{
   if (dciRegex == nullptr || dciRegex[0] == 0)
      return unique_ptr<SharedObjectArray<DCObject>>();

   unique_ptr<SharedObjectArray<DCObject>> dcoList;

   if (objectNameRegex == nullptr || objectNameRegex[0] == 0)
   {
      shared_ptr<NetObj> object = FindObjectById(objectId);
      if ((object != nullptr) && object->isDataCollectionTarget() && object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
         dcoList = static_cast<DataCollectionTarget&>(*object).getDCObjectsByRegex(dciRegex, searchByName, m_dwUserId);
   }
   else
   {
      unique_ptr<SharedObjectArray<NetObj>> objects = FindObjectsByRegex(objectNameRegex);
      if (objects != nullptr)
      {
         dcoList = make_unique<SharedObjectArray<DCObject>>();
         for(int i = 0; i < objects->size(); i++)
         {
            NetObj *object = objects->get(i);
            if (object->isDataCollectionTarget() && object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
            {
               unique_ptr<SharedObjectArray<DCObject>> nodeDcoList = static_cast<DataCollectionTarget&>(*object).getDCObjectsByRegex(dciRegex, searchByName, m_dwUserId);
               if (nodeDcoList != nullptr)
               {
                  for(int n = 0; n < nodeDcoList->size(); n++)
                  {
                     dcoList->add(nodeDcoList->getShared(n));
                  }
               }
            }
         }
      }
   }

   return dcoList;
}

/**
 * Get a list of dci's matched by regex
 *
 * @param request
 */
void ClientSession::getMatchingDCI(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

   TCHAR objectName[MAX_OBJECT_NAME], dciName[MAX_OBJECT_NAME];
   objectName[0] = 0, dciName[0] = 0;
   uint32_t objectId = 0;

   if (request.isFieldExist(VID_OBJECT_NAME))
      request.getFieldAsString(VID_OBJECT_NAME, objectName, MAX_OBJECT_NAME);
   else
      objectId = request.getFieldAsUInt32(VID_OBJECT_ID);
   uint32_t flags = request.getFieldAsInt32(VID_FLAGS);
   request.getFieldAsString(VID_DCI_NAME, dciName, MAX_OBJECT_NAME);

   unique_ptr<SharedObjectArray<DCObject>> dcoList = resolveDCOsByRegex(objectId, objectName, dciName, (flags & DCI_RES_SEARCH_NAME));
   if (dcoList != nullptr)
   {
      uint32_t dciBase = VID_DCI_VALUES_BASE, count = 0;
      for(int i = 0; i < dcoList->size(); i++)
      {
         if (dcoList->get(i)->getType() == DCO_TYPE_ITEM)
         {
            dcoList->get(i)->fillLastValueSummaryMessage(&response, dciBase);
            dciBase += 50;
            count++;
         }
      }
      response.setField(VID_NUM_ITEMS, count);
   }
   else
   {
      response.setField(VID_RCC, RCC_INVALID_ARGUMENT);
   }

   sendMessage(&response);
}

/**
 * Get list of user agent messages
 */
void ClientSession::getUserAgentNotification(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

   if (m_systemAccessRights & SYSTEM_ACCESS_UA_NOTIFICATIONS)
   {
      g_userAgentNotificationListMutex.lock();
      response.setField(VID_UA_NOTIFICATION_COUNT, g_userAgentNotificationList.size());
      int base = VID_UA_NOTIFICATION_BASE;
      for(int i = 0; i < g_userAgentNotificationList.size(); i++, base+=10)
      {
         g_userAgentNotificationList.get(i)->fillMessage(base, &response);
      }
      g_userAgentNotificationListMutex.unlock();
      response.setField(VID_RCC, RCC_SUCCESS);
   }
   else
   {
      response.setField(VID_RCC, RCC_ACCESS_DENIED);
      writeAuditLog(AUDIT_SYSCFG, false, 0, _T("Access denied getting user support application notifications"));
   }

   sendMessage(&response);
}

/**
 * Create new user agent message and send to the nodes
 */
void ClientSession::addUserAgentNotification(const NXCPMessage& request)
{
   NXCPMessage msg;
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(request.getId());

   if (m_systemAccessRights & SYSTEM_ACCESS_UA_NOTIFICATIONS)
   {
      IntegerArray<UINT32> objectList(16, 16);
      request.getFieldAsInt32Array(VID_UA_NOTIFICATION_BASE + 1, &objectList);
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
            writeAuditLog(AUDIT_SYSCFG, false, objectList.get(i), _T("Access denied on user support application notification creation"));
            break;
         }
      }
      if (rcc == RCC_SUCCESS)
      {
         TCHAR tmp[MAX_USER_AGENT_MESSAGE_SIZE];
         UserAgentNotificationItem *uan = CreateNewUserAgentNotification(request.getFieldAsString(VID_UA_NOTIFICATION_BASE, tmp, MAX_USER_AGENT_MESSAGE_SIZE),
               objectList, request.getFieldAsTime(VID_UA_NOTIFICATION_BASE + 2), request.getFieldAsTime(VID_UA_NOTIFICATION_BASE + 3),
               request.getFieldAsBoolean(VID_UA_NOTIFICATION_BASE + 4), m_dwUserId);
         json_t *objData = uan->toJson();
         WriteAuditLogWithJsonValues(AUDIT_OBJECTS, true, m_dwUserId, m_workstation, m_id, uan->getId(), nullptr, objData,
               _T("User support application notification %d created"), uan->getId());
         json_decref(objData);
         uan->decRefCount();
      }
      msg.setField(VID_RCC, rcc);
   }
   else
   {
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);
      writeAuditLog(AUDIT_SYSCFG, false, 0, _T("Access denied on user support application notification creation"));
   }

   sendMessage(&msg);
}

/**
 * Recall user user message
 */
void ClientSession::recallUserAgentNotification(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

   if (m_systemAccessRights & SYSTEM_ACCESS_UA_NOTIFICATIONS)
   {
      g_userAgentNotificationListMutex.lock();
      int base = VID_UA_NOTIFICATION_BASE;
      UserAgentNotificationItem *uan = nullptr;
      uint32_t objectId = request.getFieldAsUInt32(VID_OBJECT_ID);
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

      if (uan != nullptr)
      {
         json_t *objData = uan->toJson();
         WriteAuditLogWithJsonValues(AUDIT_OBJECTS, true, m_dwUserId, m_workstation, m_id, uan->getId(), nullptr, objData,
            _T("User support application notification %d recalled"), uan->getId());
         json_decref(objData);
         ThreadPoolExecute(g_clientThreadPool, uan, &UserAgentNotificationItem::processUpdate);
         NotifyClientSessions(NX_NOTIFY_USER_AGENT_MESSAGE_CHANGED, (UINT32)uan->getId());
         response.setField(VID_RCC, RCC_SUCCESS);
      }
      else
      {
         response.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
      }
   }
   else
   {
      response.setField(VID_RCC, RCC_ACCESS_DENIED);
      writeAuditLog(AUDIT_SYSCFG, false, 0, _T("Access denied on user support application notification recall"));
   }

   sendMessage(response);
}

/**
 * Get notification channel
 */
void ClientSession::getNotificationChannels(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());
   if (m_systemAccessRights & SYSTEM_ACCESS_SERVER_CONFIG)
   {
      GetNotificationChannels(&response);
      response.setField(VID_RCC, RCC_SUCCESS);
   }
   else
   {
      writeAuditLog(AUDIT_SYSCFG, false, 0, _T("Access denied on reading notification channel list"));
      response.setField(VID_RCC, RCC_ACCESS_DENIED);
   }
   sendMessage(response);
}

/**
 * Add notification channel
 */
void ClientSession::addNotificationChannel(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());
   if (m_systemAccessRights & SYSTEM_ACCESS_SERVER_CONFIG)
   {
      TCHAR name[MAX_OBJECT_NAME];
      request.getFieldAsString(VID_NAME, name, MAX_OBJECT_NAME);
      if (name[0] != 0)
      {
         TCHAR driverName[MAX_OBJECT_NAME];
         request.getFieldAsString(VID_DRIVER_NAME, driverName, MAX_OBJECT_NAME);
         if (driverName[0] != 0)
         {
            if (!IsNotificationChannelExists(name))
            {
               TCHAR description[MAX_NC_DESCRIPTION];
               request.getFieldAsString(VID_DESCRIPTION, description, MAX_NC_DESCRIPTION);
               char *configuration = request.getFieldAsMBString(VID_XML_CONFIG, nullptr, 0);
               CreateNotificationChannelAndSave(name, description, driverName, configuration);
               response.setField(VID_RCC, RCC_SUCCESS);
               NotifyClientSessions(NX_NOTIFY_NC_CHANNEL_CHANGED, 0);
               writeAuditLog(AUDIT_SYSCFG, true, 0, _T("Created new notification channel %s"), name);
            }
            else
            {
               response.setField(VID_RCC, RCC_CHANNEL_ALREADY_EXIST);
            }
         }
         else
         {
            response.setField(VID_RCC, RCC_INVALID_DRIVER_NAME);
         }
      }
      else
      {
         response.setField(VID_RCC, RCC_INVALID_CHANNEL_NAME);
      }
   }
   else
   {
      writeAuditLog(AUDIT_SYSCFG, false, 0, _T("Access denied on new notification channel creation"));
      response.setField(VID_RCC, RCC_ACCESS_DENIED);
   }
   sendMessage(&response);
}

/**
 * Update notificaiton channel
 */
void ClientSession::updateNotificationChannel(const NXCPMessage& request)
{
   NXCPMessage msg;
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(request.getId());
   if (m_systemAccessRights & SYSTEM_ACCESS_SERVER_CONFIG)
   {
      TCHAR name[MAX_OBJECT_NAME];
      request.getFieldAsString(VID_NAME, name, MAX_OBJECT_NAME);
      if (name[0] != 0)
      {
         TCHAR driverName[MAX_OBJECT_NAME];
         request.getFieldAsString(VID_DRIVER_NAME, driverName, MAX_OBJECT_NAME);
         if (driverName[0] != 0)
         {
            if (IsNotificationChannelExists(name))
            {
               TCHAR description[MAX_NC_DESCRIPTION];
               request.getFieldAsString(VID_DESCRIPTION, description, MAX_NC_DESCRIPTION);
               char *configuration = request.getFieldAsMBString(VID_XML_CONFIG, nullptr, 0);
               UpdateNotificationChannel(name, description, driverName, configuration);
               msg.setField(VID_RCC, RCC_SUCCESS);
               NotifyClientSessions(NX_NOTIFY_NC_CHANNEL_CHANGED, 0);
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
void ClientSession::removeNotificationChannel(const NXCPMessage& request)
{
   NXCPMessage msg;
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(request.getId());
   if (m_systemAccessRights & SYSTEM_ACCESS_SERVER_CONFIG)
   {
      TCHAR name[MAX_OBJECT_NAME];
      request.getFieldAsString(VID_NAME, name, MAX_OBJECT_NAME);
      if (!CheckChannelIsUsedInAction(name))
      {
         if (DeleteNotificationChannel(name))
         {
            NotifyClientSessions(NX_NOTIFY_NC_CHANNEL_CHANGED, 0);
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
void ClientSession::renameNotificationChannel(const NXCPMessage& request)
{
   NXCPMessage msg;
   msg.setCode(CMD_REQUEST_COMPLETED);
   msg.setId(request.getId());
   if (m_systemAccessRights & SYSTEM_ACCESS_SERVER_CONFIG)
   {
      TCHAR *name = request.getFieldAsString(VID_NAME);
      if (name[0] != 0)
      {
         if (IsNotificationChannelExists(name))
         {
            TCHAR *newName = request.getFieldAsString(VID_NEW_NAME);
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
 * Get list of available notification drivers
 */
void ClientSession::getNotificationDrivers(const NXCPMessage& request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request.getId());
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
void ClientSession::startActiveDiscovery(const NXCPMessage& request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request.getId());

   if (m_systemAccessRights & SYSTEM_ACCESS_SERVER_CONFIG)
   {
      int count = request.getFieldAsInt32(VID_NUM_RECORDS);
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
void ClientSession::getPhysicalLinks(const NXCPMessage& request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request.getId());
   bool success = true;
   shared_ptr<NetObj> object = FindObjectById(request.getFieldAsInt32(VID_OBJECT_ID));
   if(request.isFieldExist(VID_OBJECT_ID)) //Field won't exist if we need full list
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
      GetObjectPhysicalLinks(object.get(), getUserId(), request.getFieldAsInt32(VID_PATCH_PANNEL_ID), &msg);
      msg.setField(VID_RCC, RCC_SUCCESS);
   }

   sendMessage(&msg);
}

/**
 * Create or modify physical link
 */
void ClientSession::updatePhysicalLink(const NXCPMessage& request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request.getId());

   uint32_t rcc = AddPhysicalLink(request, getUserId());
   if (rcc == RCC_SUCCESS)
   {
      msg.setField(VID_RCC, RCC_SUCCESS);
      WriteAuditLog(AUDIT_SYSCFG, true, m_dwUserId, m_workstation, m_id, 0, _T("Physical link [%u] updated"), request.getFieldAsUInt32(VID_OBJECT_LINKS_BASE));
   }
   else if (rcc == RCC_ACCESS_DENIED)
   {
      WriteAuditLog(AUDIT_SYSCFG, false, m_dwUserId, m_workstation, m_id, 0, _T("Access denied on physical link [%u] update"), request.getFieldAsUInt32(VID_OBJECT_LINKS_BASE));
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
void ClientSession::deletePhysicalLink(const NXCPMessage& request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request.getId());

   if (DeletePhysicalLink(request.getFieldAsUInt32(VID_PHYSICAL_LINK_ID), getUserId()))
   {
      WriteAuditLog(AUDIT_SYSCFG, true, m_dwUserId, m_workstation, m_id, 0, _T("Physical link [%u] deleted"), request.getFieldAsUInt32(VID_PHYSICAL_LINK_ID));
      msg.setField(VID_RCC, RCC_SUCCESS);
   }
   else
   {
      WriteAuditLog(AUDIT_SYSCFG, false, m_dwUserId, m_workstation, m_id, 0, _T("Access denied on physical link [%u] delete"), request.getFieldAsUInt32(VID_PHYSICAL_LINK_ID));
      msg.setField(VID_RCC, RCC_ACCESS_DENIED);
   }

   sendMessage(&msg);
}

/**
 * Get configured web service definitions
 */
void ClientSession::getWebServices(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

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
void ClientSession::modifyWebService(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

   if (m_systemAccessRights & SYSTEM_ACCESS_WEB_SERVICE_DEFINITIONS)
   {
      SharedString name = request.getFieldAsSharedString(VID_NAME);
      uint32_t rcc;
      auto existing = FindWebServiceDefinition(name);
      if (existing != nullptr && request.getFieldAsUInt32(VID_WEBSVC_ID) != existing->getId()) //Prevent new web service definition creation with the same name
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
void ClientSession::deleteWebService(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

   if (m_systemAccessRights & SYSTEM_ACCESS_WEB_SERVICE_DEFINITIONS)
   {
      uint32_t id = request.getFieldAsUInt32(VID_WEBSVC_ID);
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
void ClientSession::getObjectCategories(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());
   ObjectCategoriesToMessage(&response);
   response.setField(VID_RCC, RCC_SUCCESS);
   sendMessage(&response);
}

/**
 * Modify object category
 */
void ClientSession::modifyObjectCategory(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

   if (m_systemAccessRights & SYSTEM_ACCESS_OBJECT_CATEGORIES)
   {
      uint32_t categoryId;
      uint32_t rcc = ModifyObjectCategory(request, &categoryId);
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
void ClientSession::deleteObjectCategory(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

   if (m_systemAccessRights & SYSTEM_ACCESS_OBJECT_CATEGORIES)
   {
      uint32_t id = request.getFieldAsUInt32(VID_CATEGORY_ID);
      uint32_t rcc = DeleteObjectCategory(id, request.getFieldAsBoolean(VID_FORCE_DELETE));
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
void ClientSession::getGeoAreas(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());
   GeoAreasToMessage(&response);
   response.setField(VID_RCC, RCC_SUCCESS);
   sendMessage(&response);
}

/**
 * Modify geo area
 */
void ClientSession::modifyGeoArea(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

   if (m_systemAccessRights & SYSTEM_ACCESS_MANAGE_GEO_AREAS)
   {
      uint32_t areaId;
      uint32_t rcc = ModifyGeoArea(request, &areaId);
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
void ClientSession::deleteGeoArea(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

   if (m_systemAccessRights & SYSTEM_ACCESS_MANAGE_GEO_AREAS)
   {
      uint32_t id = request.getFieldAsUInt32(VID_AREA_ID);
      uint32_t rcc = DeleteGeoArea(id, request.getFieldAsBoolean(VID_FORCE_DELETE));
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
 * Get configured network credentials
 */
void ClientSession::getNetworkCredentials(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

   if (request.isFieldExist(VID_ZONE_UIN)) // specific zone
   {
      int32_t zoneUIN = request.getFieldAsInt32(VID_ZONE_UIN);
      shared_ptr<Zone> zone;
      if (zoneUIN != ALL_ZONES)
         zone = FindZoneByUIN(zoneUIN);
      if ((zoneUIN == ALL_ZONES) || (zone != nullptr))
      {
         if (((zoneUIN == ALL_ZONES) && (m_systemAccessRights & SYSTEM_ACCESS_SERVER_CONFIG)) ||
             ((zone != nullptr) && zone->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ)))
         {
            TCHAR tag[16];
            switch(request.getCode())
            {
               case CMD_GET_COMMUNITY_LIST:
                  ZoneCommunityListToMessage(zoneUIN, &response);
                  break;
               case CMD_GET_USM_CREDENTIALS:
                  ZoneUsmCredentialsListToMessage(zoneUIN, &response);
                  break;
               case CMD_GET_WELL_KNOWN_PORT_LIST:
                  request.getFieldAsString(VID_TAG, tag, 16);
                  ZoneWellKnownPortListToMessage(tag, zoneUIN, &response);
                  break;
               case CMD_GET_SHARED_SECRET_LIST:
                  ZoneAgentSecretListToMessage(zoneUIN, &response);
                  break;
               case CMD_GET_SSH_CREDENTIALS:
                  ZoneSSHCredentialsListToMessage(zoneUIN, &response);
                  break;
            }
         }
         else
         {
            response.setField(VID_RCC, RCC_ACCESS_DENIED);
         }
      }
      else
      {
         response.setField(VID_RCC, RCC_INVALID_ZONE_ID);
      }
   }
   else  // All zones
   {
      if (m_systemAccessRights & SYSTEM_ACCESS_SERVER_CONFIG)
      {
         TCHAR tag[16];
         switch (request.getCode())
         {
            case CMD_GET_SHARED_SECRET_LIST:
               FullAgentSecretListToMessage(m_dwUserId, &response);
               break;
            case CMD_GET_SSH_CREDENTIALS:
               FullSSHCredentialsListToMessage(m_dwUserId, &response);
               break;
            case CMD_GET_COMMUNITY_LIST:
               FullCommunityListToMessage(m_dwUserId, &response);
               break;
            case CMD_GET_USM_CREDENTIALS:
               FullUsmCredentialsListToMessage(m_dwUserId, &response);
               break;
            case CMD_GET_WELL_KNOWN_PORT_LIST:
               request.getFieldAsString(VID_TAG, tag, 16);
               FullWellKnownPortListToMessage(tag, m_dwUserId, &response);
               break;
         }
      }
      else
      {
         response.setField(VID_RCC, RCC_ACCESS_DENIED);
      }
   }

   sendMessage(response);
}

/**
 * Update SNMP community list
 */
void ClientSession::updateCommunityList(const NXCPMessage& request)
{
   int32_t zoneUIN = request.getFieldAsInt32(VID_ZONE_UIN);
   shared_ptr<Zone> zone;
   if (zoneUIN != ALL_ZONES)
      zone = FindZoneByUIN(zoneUIN);

   uint32_t rcc;
   if ((zoneUIN == ALL_ZONES) || (zone != nullptr))
   {
      if (((zoneUIN == ALL_ZONES) && (m_systemAccessRights & SYSTEM_ACCESS_SERVER_CONFIG)) ||
          ((zone != nullptr) && zone->checkAccessRights(m_dwUserId, OBJECT_ACCESS_MODIFY)))
      {
         rcc = UpdateCommunityList(request, zoneUIN);
         if (rcc == RCC_SUCCESS)
            writeAuditLog(AUDIT_SYSCFG, true, (zone != nullptr) ? zone->getId() : 0, _T("Updated list of well-known SNMP community strings"));
      }
      else
      {
         writeAuditLog(AUDIT_SYSCFG, false, (zone != nullptr) ? zone->getId() : 0, _T("Access denied on updating list of well-known SNMP community strings"));
         rcc = RCC_ACCESS_DENIED;
      }
   }
   else
   {
      rcc = RCC_INVALID_ZONE_ID;
   }

   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());
   response.setField(VID_RCC, rcc);
   sendMessage(response);
}

/**
 * Update SNMP v3 USM credentials
 */
void ClientSession::updateUsmCredentials(const NXCPMessage& request)
{
   int32_t zoneUIN = request.getFieldAsInt32(VID_ZONE_UIN);
   shared_ptr<Zone> zone;
   if (zoneUIN != ALL_ZONES)
      zone = FindZoneByUIN(zoneUIN);

   uint32_t rcc;
   if ((zoneUIN == ALL_ZONES) || (zone != nullptr))
   {
      if (((zoneUIN == ALL_ZONES) && (m_systemAccessRights & SYSTEM_ACCESS_SERVER_CONFIG)) ||
          ((zone != nullptr) && zone->checkAccessRights(m_dwUserId, OBJECT_ACCESS_MODIFY)))
      {
         rcc = UpdateUsmCredentialsList(request, zoneUIN);
         if (rcc == RCC_SUCCESS)
            writeAuditLog(AUDIT_SYSCFG, true, (zone != nullptr) ? zone->getId() : 0, _T("Updated list of well-known SNMPv3 USM credentials"));
      }
      else
      {
         writeAuditLog(AUDIT_SYSCFG, false, (zone != nullptr) ? zone->getId() : 0, _T("Access denied on updating list of well-known SNMPv3 USM credentials"));
         rcc = RCC_ACCESS_DENIED;
      }
   }
   else
   {
      rcc = RCC_INVALID_ZONE_ID;
   }

   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());
   response.setField(VID_RCC, rcc);
   sendMessage(response);
}

/**
 * Update shared secret list
 */
void ClientSession::updateSharedSecretList(const NXCPMessage& request)
{
   int32_t zoneUIN = request.getFieldAsInt32(VID_ZONE_UIN);
   shared_ptr<Zone> zone;
   if (zoneUIN != ALL_ZONES)
      zone = FindZoneByUIN(zoneUIN);

   uint32_t rcc;
   if ((zoneUIN == ALL_ZONES) || (zone != nullptr))
   {
      if (((zoneUIN == ALL_ZONES) && (m_systemAccessRights & SYSTEM_ACCESS_SERVER_CONFIG)) ||
          ((zone != nullptr) && zone->checkAccessRights(m_dwUserId, OBJECT_ACCESS_MODIFY)))
      {
         rcc = UpdateAgentSecretList(request, zoneUIN);
         if (rcc == RCC_SUCCESS)
            writeAuditLog(AUDIT_SYSCFG, true, (zone != nullptr) ? zone->getId() : 0, _T("Updated list of well-known agent shared secrets"));
      }
      else
      {
         writeAuditLog(AUDIT_SYSCFG, false, (zone != nullptr) ? zone->getId() : 0, _T("Access denied on updating list of well-known agent shared secrets"));
         rcc = RCC_ACCESS_DENIED;
      }
   }
   else
   {
      rcc = RCC_INVALID_ZONE_ID;
   }

   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());
   response.setField(VID_RCC, rcc);
   sendMessage(response);
}

/**
 * Update list of well-known SSH credentials. Existing list will be replaced by provided one.
 *
 * Called by:
 * CMD_UPDATE_SSH_CREDENTIALS
 *
 * Expected input parameters:
 * VID_ZONE_UIN            Zone UIN (unique identification number)
 * VID_NUM_RECORDS         Number of SSH credentials
 * VID_ELEMENT_LIST_BASE   Base element of SSH credentials list
 *
 * Return values:
 * VID_RCC                          Request completion code
 */
void ClientSession::updateSshCredentials(const NXCPMessage& request)
{
   int32_t zoneUIN = request.getFieldAsInt32(VID_ZONE_UIN);
   shared_ptr<Zone> zone;
   if (zoneUIN != ALL_ZONES)
      zone = FindZoneByUIN(zoneUIN);

   uint32_t rcc;
   if ((zoneUIN == ALL_ZONES) || (zone != nullptr))
   {
      if (((zoneUIN == ALL_ZONES) && (m_systemAccessRights & SYSTEM_ACCESS_SERVER_CONFIG)) ||
          ((zone != nullptr) && zone->checkAccessRights(m_dwUserId, OBJECT_ACCESS_MODIFY)))
      {
         rcc = UpdateSSHCredentials(request, zoneUIN);
         if (rcc == RCC_SUCCESS)
            writeAuditLog(AUDIT_SYSCFG, true, (zone != nullptr) ? zone->getId() : 0, _T("Updated list of well-known SSH credentials"));
      }
      else
      {
         writeAuditLog(AUDIT_SYSCFG, false, (zone != nullptr) ? zone->getId() : 0, _T("Access denied on updating list of well-known SSH credentials"));
         rcc = RCC_ACCESS_DENIED;
      }
   }
   else
   {
      rcc = RCC_INVALID_ZONE_ID;
   }

   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());
   response.setField(VID_RCC, rcc);
   sendMessage(response);
}

/**
 * Update SNMP port list
 */
void ClientSession::updateWellKnownPortList(const NXCPMessage& request)
{
   int32_t zoneUIN = request.getFieldAsInt32(VID_ZONE_UIN);
   shared_ptr<Zone> zone;
   if (zoneUIN != ALL_ZONES)
      zone = FindZoneByUIN(zoneUIN);

   uint32_t rcc;
   if ((zoneUIN == ALL_ZONES) || (zone != nullptr))
   {
      if (((zoneUIN == ALL_ZONES) && (m_systemAccessRights & SYSTEM_ACCESS_SERVER_CONFIG)) ||
          ((zone != nullptr) && zone->checkAccessRights(m_dwUserId, OBJECT_ACCESS_MODIFY)))
      {
         TCHAR tag[16];
         request.getFieldAsString(VID_TAG, tag, 16);
         rcc = UpdateWellKnownPortList(request, tag, zoneUIN);
         if (rcc == RCC_SUCCESS)
            writeAuditLog(AUDIT_SYSCFG, true, (zone != nullptr) ? zone->getId() : 0, _T("Updated list of well-known ports"));
      }
      else
      {
         writeAuditLog(AUDIT_SYSCFG, false, (zone != nullptr) ? zone->getId() : 0, _T("Access denied on updating list of well-known ports"));
         rcc = RCC_ACCESS_DENIED;
      }
   }
   else
   {
      rcc = RCC_INVALID_ZONE_ID;
   }

   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());
   response.setField(VID_RCC, rcc);
   sendMessage(response);
}

/**
 * Find proxy for node
 */
void ClientSession::findProxyForNode(const NXCPMessage& request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request.getId());

   shared_ptr<Node> node = static_pointer_cast<Node>(FindObjectById(request.getFieldAsUInt32(VID_NODE_ID), OBJECT_NODE));
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

/**
 * Get SSH key list
 */
void ClientSession::getSshKeys(const NXCPMessage& request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request.getId());
   FillMessageWithSshKeys(&msg, request.getFieldAsBoolean(VID_INCLUDE_PUBLIC_KEY));
   msg.setField(VID_RCC, RCC_SUCCESS);
   sendMessage(&msg);
}

/**
 * Delete ssh key data
 */
void ClientSession::deleteSshKey(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());
   if (m_systemAccessRights & SYSTEM_ACCESS_SSH_KEY_CONFIGURATION)
   {
      DeleteSshKey(&response, request.getFieldAsInt32(VID_SSH_KEY_ID), request.getFieldAsBoolean(VID_FORCE_DELETE));
   }
   else
   {
      writeAuditLog(AUDIT_SYSCFG, false, 0, _T("Access denied on delete SSH key"));
      response.setField(VID_RCC, RCC_ACCESS_DENIED);
   }
   sendMessage(response);
}

/**
 * Update or create ssh key data entry
 */
void ClientSession::updateSshKey(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());
   if (m_systemAccessRights & SYSTEM_ACCESS_SSH_KEY_CONFIGURATION)
   {
      CreateOrEditSshKey(request);
      response.setField(VID_RCC, RCC_SUCCESS);
   }
   else
   {
      writeAuditLog(AUDIT_SYSCFG, false, 0, _T("Access denied on update SSH key"));
      response.setField(VID_RCC, RCC_ACCESS_DENIED);
   }
   sendMessage(response);
}

/**
 * Generate new SSH key
 */
void ClientSession::generateSshKey(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());
   if (m_systemAccessRights & SYSTEM_ACCESS_SSH_KEY_CONFIGURATION)
   {
      String name = request.getFieldAsSharedString(VID_NAME);
      response.setField(VID_RCC, GenerateSshKey(name));
   }
   else
   {
      writeAuditLog(AUDIT_SYSCFG, false, 0, _T("Access denied on generate SSH key"));
      response.setField(VID_RCC, RCC_ACCESS_DENIED);
   }
   sendMessage(response);
}

/**
 * Get notification driver names
 */
void ClientSession::get2FADrivers(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());
   if (m_systemAccessRights & SYSTEM_ACCESS_MANAGE_2FA_METHODS)
   {
      Get2FADrivers(&response);
      response.setField(VID_RCC, RCC_SUCCESS);
   }
   else
   {
      writeAuditLog(AUDIT_SYSCFG, false, 0, _T("Access denied on reading two-factor authentication driver list"));
      response.setField(VID_RCC, RCC_ACCESS_DENIED);
   }
   sendMessage(response);
}

/**
 * Returns list of configured 2FA methods with method name, description and method loading status
 */
void ClientSession::get2FAMethods(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());
   Get2FAMethods(&response);
   response.setField(VID_RCC, RCC_SUCCESS);
   sendMessage(response);
}

/**
 * API call for creating or modifying 2FA method
 */
void ClientSession::modify2FAMethod(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());
   if (m_systemAccessRights & SYSTEM_ACCESS_MANAGE_2FA_METHODS)
   {
      TCHAR name[MAX_OBJECT_NAME], driver[MAX_OBJECT_NAME], description[MAX_2FA_DESCRIPTION];
      request.getFieldAsString(VID_NAME, name, MAX_OBJECT_NAME);
      request.getFieldAsString(VID_DRIVER_NAME, driver, MAX_OBJECT_NAME);
      request.getFieldAsString(VID_DESCRIPTION, description, MAX_2FA_DESCRIPTION);
      char *config = request.getFieldAsUtf8String(VID_CONFIG_FILE_DATA); // will be freed by Modify2FAMethod
      uint32_t rcc = Modify2FAMethod(name, driver, description, config);
      response.setField(VID_RCC, rcc);
   }
   else
   {
      writeAuditLog(AUDIT_SYSCFG, false, 0, _T("Access denied on modify 2FA method"));
      response.setField(VID_RCC, RCC_ACCESS_DENIED);
   }
   sendMessage(response);
}

/**
 * Rename two-factor authentication method
 */
void ClientSession::rename2FAMethod(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());
   if (m_systemAccessRights & SYSTEM_ACCESS_MANAGE_2FA_METHODS)
   {
      TCHAR *name = request.getFieldAsString(VID_NAME);
      if ((name != nullptr) && (name[0] != 0))
      {
         if (Is2FAMethodExists(name))
         {
            TCHAR *newName = request.getFieldAsString(VID_NEW_NAME);
            if ((newName != nullptr) && (newName[0] != 0) && !Is2FAMethodExists(newName))
            {
               writeAuditLog(AUDIT_SYSCFG, true, 0, _T("Two-factor authentication method \"%s\" renamed to \"%s\""), name, newName);
               Rename2FAMethod(name, newName);
               response.setField(VID_RCC, RCC_SUCCESS);
            }
            else
            {
               response.setField(VID_RCC, RCC_CHANNEL_ALREADY_EXIST);
            }
            MemFree(newName);
         }
         else
         {
            response.setField(VID_RCC, RCC_NO_CHANNEL_NAME);
         }
      }
      else
      {
         response.setField(VID_RCC, RCC_INVALID_CHANNEL_NAME);
      }
      MemFree(name);
   }
   else
   {
      writeAuditLog(AUDIT_SYSCFG, false, 0, _T("Access denied on two-factor authentication method rename"));
      response.setField(VID_RCC, RCC_ACCESS_DENIED);
   }
   sendMessage(response);
}

/**
 * Delete 2FA method
 */
void ClientSession::delete2FAMethod(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());
   if (m_systemAccessRights & SYSTEM_ACCESS_MANAGE_2FA_METHODS)
   {
      TCHAR name[MAX_OBJECT_NAME];
      request.getFieldAsString(VID_NAME, name, MAX_OBJECT_NAME);
      response.setField(VID_RCC, Delete2FAMethod(name));
   }
   else
   {
      writeAuditLog(AUDIT_SYSCFG, false, 0, _T("Access denied on deleting 2FA method"));
      response.setField(VID_RCC, RCC_ACCESS_DENIED);
   }
   sendMessage(response);
}

/**
 * Returns list of configured 2FA method bindings with method name and configuration
 */
void ClientSession::getUser2FABindings(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());
   uint32_t userId = request.getFieldAsUInt32(VID_USER_ID);
   if ((userId == m_dwUserId) || (m_systemAccessRights & SYSTEM_ACCESS_MANAGE_USERS))
   {
      FillUser2FAMethodBindingInfo(userId, &response);
      response.setField(VID_RCC, RCC_SUCCESS);
   }
   else
   {
      writeAuditLog(AUDIT_SECURITY, false, 0, _T("Access denied on getting 2FA method bindings"));
      response.setField(VID_RCC, RCC_ACCESS_DENIED);
   }
   sendMessage(response);
}

/**
 * API call for creating or modifying 2FA method binding
 */
void ClientSession::modifyUser2FABinding(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());
   TCHAR buffer[MAX_USER_NAME];
   uint32_t userId = request.getFieldAsUInt32(VID_USER_ID);
   if ((userId == m_dwUserId) || (m_systemAccessRights & SYSTEM_ACCESS_MANAGE_USERS))
   {
      TCHAR methodName[MAX_OBJECT_NAME];
      request.getFieldAsString(VID_2FA_METHOD, methodName, MAX_OBJECT_NAME);
      StringMap configuration(request, VID_2FA_METHOD_LIST_BASE, VID_2FA_METHOD_COUNT);
      uint32_t rcc = ModifyUser2FAMethodBinding(userId, methodName, configuration);
      response.setField(VID_RCC, rcc);
      if (rcc == RCC_SUCCESS)
      {
         writeAuditLog(AUDIT_SECURITY, true, 0, _T("2FA method \"%s\" binding configuration updated for user \"%s\""), methodName, ResolveUserId(userId, buffer, true));
      }
   }
   else
   {
      writeAuditLog(AUDIT_SECURITY, false, 0, _T("Access denied on modify 2FA method binding for user \"%s\""), ResolveUserId(userId, buffer, true));
      response.setField(VID_RCC, RCC_ACCESS_DENIED);
   }
   sendMessage(response);
}

/**
 * Delete 2FA method
 */
void ClientSession::deleteUser2FABinding(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());
   uint32_t userId = request.getFieldAsUInt32(VID_USER_ID);
   if ((userId == m_dwUserId) || (m_systemAccessRights & SYSTEM_ACCESS_MANAGE_USERS))
   {
      TCHAR name[MAX_OBJECT_NAME];
      request.getFieldAsString(VID_2FA_RESPONSE, name, MAX_OBJECT_NAME);
      response.setField(VID_RCC, DeleteUser2FAMethodBinding(userId, name));
   }
   else
   {
      TCHAR buffer[MAX_USER_NAME];
      writeAuditLog(AUDIT_SECURITY, false, 0, _T("Access denied on deleting 2FA method for user \"%s\""), ResolveUserId(userId, buffer, true));
      response.setField(VID_RCC, RCC_ACCESS_DENIED);
   }
   sendMessage(response);
}

/**
 * Get business service check list
 * Expected input parameters:
 * VID_OBJECT_ID                     ID of business service
 *
 * Return values:
 * VID_CHECK_COUNT                   Number of checks. List offset is 10.
 * VID_CHECK_LIST_BASE + offset      Id of business service check
 * VID_CHECK_LIST_BASE + offset + 1  Business service check violation reason
 * VID_CHECK_LIST_BASE + offset + 2  Related data collection item id in related data collection target object
 * VID_CHECK_LIST_BASE + offset + 3  Related NetObj object id
 * VID_CHECK_LIST_BASE + offset + 4  Threshold for object check and DCI check
 * VID_CHECK_LIST_BASE + offset + 5  Business service check name
 * VID_CHECK_LIST_BASE + offset + 6  Script text for script check
 * VID_RCC                           Request Completion Code
 */
void ClientSession::getBusinessServiceCheckList(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());
   shared_ptr<NetObj> object = FindObjectById(request.getFieldAsUInt32(VID_OBJECT_ID));
   if (object != nullptr)
   {
      if ((object->getObjectClass() == OBJECT_BUSINESSSERVICE) || (object->getObjectClass() == OBJECT_BUSINESSSERVICEPROTO))
      {
         if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
         {
            unique_ptr<SharedObjectArray<BusinessServiceCheck>> checks = static_cast<BaseBusinessService&>(*object).getChecks();
            uint32_t fieldId = VID_CHECK_LIST_BASE;
            for (const shared_ptr<BusinessServiceCheck>& check : *checks)
            {
               check->fillMessage(&response, fieldId);
               fieldId += 100;
            }
            response.setField(VID_CHECK_COUNT, checks->size());
            response.setField(VID_RCC, RCC_SUCCESS);
         }
         else
         {
            writeAuditLog(AUDIT_OBJECTS, false, object->getId(), _T("Access denied on reading business service check list"));
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
   sendMessage(response);
}

/**
 * Modify business service check
 * Expected input parameters:
 * VID_OBJECT_ID         ID of business service
 * VID_CHECK_ID          ID of business service check
 * VID_CHECK_TYPE        Business service check type. Object = 0, Script = 1, DCI = 2
 * VID_RELATED_OBJECT    Related NetObj object id. Mandatory in object and DCI checks, optional in script check
 * VID_RELATED_DCI       Related data collection item id in related data collection target object. Mandatory in DCI checks
 * VID_SCRIPT            Script text for script check. Optional, without it script check just do nothing and stays in NORMAL state.
 * VID_DESCRIPTION       Business service check description
 * VID_THRESHOLD         Threshold for object check and DCI check. If not set, default threshold will be used instead
 *
 * Return values:
 * VID_RCC                       Request Completion Code
 */
void ClientSession::modifyBusinessServiceCheck(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());
   shared_ptr<NetObj> object = FindObjectById(request.getFieldAsUInt32(VID_OBJECT_ID));
   if (object != nullptr)
   {
      if ((object->getObjectClass() == OBJECT_BUSINESSSERVICE) || (object->getObjectClass() == OBJECT_BUSINESSSERVICEPROTO))
      {
         if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_MODIFY))
         {
            writeAuditLog(AUDIT_OBJECTS, true, object->getId(), _T("Business service check modified"));
            response.setField(VID_RCC, static_cast<BaseBusinessService&>(*object).modifyCheckFromMessage(request));
         }
         else
         {
            writeAuditLog(AUDIT_OBJECTS, false, object->getId(), _T("Access denied on modifying business service check"));
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
   sendMessage(response);
}

/**
 * Delete business service check from business service
 * Expected input parameters:
 * VID_OBJECT_ID  id of business service
 * VID_CHECK_ID   id of business service check
 *
 * Return values:
 * VID_RCC                       Request Completion Code
 */
void ClientSession::deleteBusinessServiceCheck(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());
   shared_ptr<NetObj> object = FindObjectById(request.getFieldAsUInt32(VID_OBJECT_ID));
   if (object != nullptr)
   {
      if ((object->getObjectClass() == OBJECT_BUSINESSSERVICE) || (object->getObjectClass() == OBJECT_BUSINESSSERVICEPROTO))
      {
         if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_MODIFY))
         {
            writeAuditLog(AUDIT_OBJECTS, true, object->getId(), _T("Business service check deleted"));
            response.setField(VID_RCC, static_cast<BaseBusinessService&>(*object).deleteCheck(request.getFieldAsUInt32(VID_CHECK_ID)));
         }
         else
         {
            writeAuditLog(AUDIT_OBJECTS, false, object->getId(), _T("Access denied on deleting business service check"));
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
   sendMessage(response);
}

/**
 * Get business service uptime
 * Expected input parameters:
 * VID_OBJECT_ID                 id of business service
 * VID_TIME_FROM                 Make uptime calculation from this time. Seconds since the Epoch.
 * VID_TIME_TO                   Make uptime calculation to this time. Seconds since the Epoch.
 *
 * Return values:
 * VID_BUSINESS_SERVICE_UPTIME   Business service uptime in percents. Double.
 * VID_RCC                       Request Completion Code
 */
void ClientSession::getBusinessServiceUptime(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());
   shared_ptr<NetObj> object = FindObjectById(request.getFieldAsUInt32(VID_OBJECT_ID), OBJECT_BUSINESSSERVICE);
   if (object != nullptr)
   {
      if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
      {
         time_t from = 0;
         if (request.isFieldExist(VID_TIME_FROM))
         {
            from = request.getFieldAsUInt64(VID_TIME_FROM);
         }
         time_t to = time(nullptr);
         if (request.isFieldExist(VID_TIME_TO))
         {
            to = request.getFieldAsUInt64(VID_TIME_TO);
         }

         response.setField(VID_BUSINESS_SERVICE_UPTIME, GetServiceUptime(object->getId(), from, to));
         response.setField(VID_RCC, RCC_SUCCESS);
      }
      else
      {
         writeAuditLog(AUDIT_OBJECTS, false, object->getId(), _T("Access denied on reading business service uptime"));
         response.setField(VID_RCC, RCC_ACCESS_DENIED);
      }
   }
   else
   {
      response.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
   }
   sendMessage(response);
}

/**
 * Get business service tickets list for business service
 * Expected input parameters:
 * VID_OBJECT_ID                 Id of business service
 * VID_TIME_FROM                 Get valid tickets from this time. Seconds since the Epoch.
 * VID_TIME_TO                   Get valid tickets to this time. Seconds since the Epoch.
 *
 * Return values:
 * VID_TICKET_COUNT                  Number of tickets. List offset is 10.
 * VID_TICKET_LIST_BASE + offset     Id of business service ticket
 * VID_TICKET_LIST_BASE + offset + 1 Id of business service
 * VID_TICKET_LIST_BASE + offset + 2 Id of business service check
 * VID_TICKET_LIST_BASE + offset + 3 Ticket creation timestamp
 * VID_TICKET_LIST_BASE + offset + 4 Ticket closing timestamp
 * VID_TICKET_LIST_BASE + offset + 5 Reason
 * VID_TICKET_LIST_BASE + offset + 6 Business service check description
 * VID_RCC                           Request Completion Code
 */
void ClientSession::getBusinessServiceTickets(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());
   shared_ptr<NetObj> object = FindObjectById(request.getFieldAsUInt32(VID_OBJECT_ID), OBJECT_BUSINESSSERVICE);
   if (object != nullptr)
   {
      if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
      {
         time_t from = 0;
         if (request.isFieldExist(VID_TIME_FROM))
         {
            from = request.getFieldAsUInt64(VID_TIME_FROM);
         }
         time_t to = time(nullptr);
         if (request.isFieldExist(VID_TIME_TO))
         {
            to = request.getFieldAsUInt64(VID_TIME_TO);
         }
         GetServiceTickets(object->getId(), from, to, &response);
         response.setField(VID_RCC, RCC_SUCCESS);
      }
      else
      {
         writeAuditLog(AUDIT_OBJECTS, false, object->getId(), _T("Access denied on reading business service tickets"));
         response.setField(VID_RCC, RCC_ACCESS_DENIED);
      }
   }
   else
   {
      response.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
   }
   sendMessage(response);
}

/**
 * Execute SSH command on agent
 * Expected input parameters:
 * VID_OBJECT_ID                 Id of node
 * VID_COMMAND                   Command to execute
 * VID_RECEIVE_OUTPUT            Boolean flag for sending output of command.
 *
 * Return values:
 * VID_RCC                           Request Completion Code
 */
void ClientSession::executeSshCommand(const NXCPMessage& request)
{
   NXCPMessage msg(CMD_REQUEST_COMPLETED, request.getId());
   shared_ptr<Node> node = static_pointer_cast<Node>(FindObjectById(request.getFieldAsUInt32(VID_OBJECT_ID), OBJECT_NODE));
   if (node != nullptr)
   {
      TCHAR command[MAX_PARAM_NAME];
      request.getFieldAsString(VID_COMMAND, command, MAX_PARAM_NAME);

      if (node->checkAccessRights(m_dwUserId, OBJECT_ACCESS_CONTROL))
      {
         uint32_t proxyId = node->getEffectiveSshProxy();
         shared_ptr<Node> proxy = static_pointer_cast<Node>(FindObjectById(proxyId, OBJECT_NODE));
         if (proxy != nullptr)
         {
            shared_ptr<AgentConnectionEx> conn = proxy->createAgentConnection();
            if (conn != nullptr)
            {
               StringList list;
               TCHAR ipAddr[64];
               list.add(node->getIpAddress().toString(ipAddr));
               list.add(node->getSshPort());
               list.add(node->getSshLogin());
               list.add(node->getSshPassword());
               list.add(command);
               list.add(node->getSshKeyId());

               uint32_t rcc;
               bool withOutput = request.getFieldAsBoolean(VID_RECEIVE_OUTPUT);
               if (withOutput)
               {
                  ActionExecutionData data(this, request.getId());
                  rcc = conn->executeCommand(_T("SSH.Command"), list, true, ActionExecuteCallback, &data);
               }
               else
               {
                  rcc = conn->executeCommand(_T("SSH.Command"), list);
               }
               debugPrintf(4, _T("executeSshCommand: rcc=%d"), rcc);

               msg.setField(VID_RCC, AgentErrorToRCC(rcc));
               if (rcc == ERR_SUCCESS)
               {
                  writeAuditLog(AUDIT_OBJECTS, true, node->getId(),  _T("Executed SSH command \"%s\" on %s:%u as %s"),
                        command, node->getIpAddress().toString(ipAddr), node->getSshPort(), node->getSshLogin().cstr());
               }
            }
            else
            {
               msg.setField(VID_RCC, RCC_COMM_FAILURE);
            }
         }
         else
         {
            msg.setField(VID_RCC, RCC_INVALID_SSH_PROXY_ID);
         }
      }
      else
      {
         msg.setField(VID_RCC, RCC_ACCESS_DENIED);
         writeAuditLog(AUDIT_OBJECTS, false, node->getId(), _T("Access denied on executing SSH command %s"), command);
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
 * Find DCI by given parameters
 * Expected input parameters:
 * VID_OBJECT_ID                 Root object id
 * VID_SEARCH_PATTERN            Search string
 *
 * Return values:
 * VID_NUM_ITEMS                 Number of found DCIs. Ofset 50
 * VID_DCI_VALUES_BASE           TODO:
 * ...
 * VID_RCC                       Request Completion Code
 */
void ClientSession::findDci(const NXCPMessage &request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

   shared_ptr<NetObj> rootObject = FindObjectById(request.getFieldAsUInt32(VID_OBJECT_ID));
   if (rootObject != nullptr)
   {
      if (rootObject->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
      {
         SearchQuery query(request.getFieldAsSharedString(VID_SEARCH_PATTERN));
         SharedObjectArray<DCObject> result(0, 64);

         SharedObjectArray<DataCollectionTarget> targets;
         if (rootObject->isDataCollectionTarget())
         {
            targets.add(static_pointer_cast<DataCollectionTarget>(rootObject));
         }
         else
         {
            rootObject->addChildDCTargetsToList(&targets, m_dwUserId);
         }
         for(int i = 0; i < targets.size(); i++)
         {
            if (targets.get(i)->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
               targets.get(i)->findDcis(query, m_dwUserId, &result);
         }

         uint32_t fieldId = VID_DCI_VALUES_BASE;
         uint32_t count = 0;
         for(const shared_ptr<DCObject>& dci : result)
         {
            dci->fillLastValueSummaryMessage(&response, fieldId);
            fieldId += 50;
            count++;
         }
         response.setField(VID_NUM_ITEMS, count);
      }
      else
      {
         response.setField(VID_RCC, RCC_ACCESS_DENIED);
      }
   }
   else  // No object with given ID
   {
      response.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   sendMessage(response);
}

/**
 * Collect information about objects that are using the event and send it back to client.
 */
void ClientSession::getEventRefences(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

   if (checkSysAccessRights(SYSTEM_ACCESS_VIEW_EVENT_DB) ||
       checkSysAccessRights(SYSTEM_ACCESS_EDIT_EVENT_DB) ||
       checkSysAccessRights(SYSTEM_ACCESS_EPP))
   {
      uint32_t eventCode = request.getFieldAsInt32(VID_EVENT_CODE);
      unique_ptr<ObjectArray<EventReference>> eventReferences = GetAllEventReferences(eventCode);

      response.setField(VID_NUM_ELEMENTS, eventReferences->size());
      uint32_t fieldId = VID_ELEMENT_LIST_BASE;
      for (int i = 0; i < eventReferences->size(); i++, fieldId += 100)
         eventReferences->get(i)->fillMessage(&response, fieldId);

      response.setField(VID_RCC, RCC_SUCCESS);
   }
   else
   {
      response.setField(VID_RCC, RCC_ACCESS_DENIED);
      writeAuditLog(AUDIT_SYSCFG, false, 0, _T("Access denied getting event references"));
   }

   sendMessage(response);
}

/**
 * Get all maintenance journal entries for the given object
 *
 * Called by:
 * CMD_READ_MAINTENANCE_JOURNAL
 *
 * Expected input parameters:
 * VID_OBJECT_ID               Journal owner (source) ID
 * VID_MAX_RECORDS             Maximum numer of entries to get
 *
 * Return values:
 * VID_NUM_ELEMENTS            Number of maintenance entries
 * VID_RCC                     Request completion code
 * VID_ELEMENT_LIST_BASE - 1   Flag for the case that not all available entries are sent back bun only last 1000
 * VID_ELEMENT_LIST_BASE       Base for maintenance entry list. Also first maintenance entry ID
 * VID_ELEMENT_LIST_BASE + 1   Journal ID = journal owner (source) ID
 * VID_ELEMENT_LIST_BASE + 2   author
 * VID_ELEMENT_LIST_BASE + 3   lastEditedBy
 * VID_ELEMENT_LIST_BASE + 4   description
 * VID_ELEMENT_LIST_BASE + 5   creationTime
 * VID_ELEMENT_LIST_BASE + 6   modificationTime
 *
 * Second maintenance entry starts from VID_ELEMENT_LIST_BASE + 10
 */
void ClientSession::readMaintenanceJournal(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

   shared_ptr<NetObj> object = FindObjectById(request.getFieldAsUInt32(VID_OBJECT_ID));
   if (object != nullptr)
   {
      if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
      {
         unique_ptr<SharedObjectArray<NetObj>> sources = object->getAllChildren(false);
         SharedPtrIterator<NetObj> it = sources->begin();
         while (it.hasNext())
         {
            if (!it.next()->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
               it.remove();
         }
         sources->add(object);

         uint32_t rcc = ReadMaintenanceJournal(*sources, &response, request.getFieldAsUInt32(VID_MAX_RECORDS));
         if (rcc == RCC_SUCCESS)
         {
            writeAuditLog(AUDIT_OBJECTS, true, object->getId(), _T("Maintenance journal read successfully"));
            debugPrintf(6, _T("Maintenance journal for object %s [%u] read successfully"), object->getName(), object->getId());
         }
         response.setField(VID_RCC, rcc);
      }
      else
      {
         response.setField(VID_RCC, RCC_ACCESS_DENIED);
         writeAuditLog(AUDIT_OBJECTS, false, object->getId(), _T("Maintenance journal read failed: access denied"));
         debugPrintf(6, _T("Maintenance journal read for object %s [%u] failed: access denied"), object->getName(), object->getId());
      }
   }
   else
   {
      response.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
      debugPrintf(6, _T("Maintenance journal read for object [%u] failed: invalid object ID"), request.getFieldAsUInt32(VID_OBJECT_ID));
   }

   sendMessage(response);
}

/**
 * Create new entry in the maintenance journal
 *
 * Called by:
 * CMD_WRITE_MAINTENANCE_JOURNAL
 *
 * Expected input parameters:
 * VID_OBJECT_ID     Journal owner (source) ID
 * VID_DESCRIPTION   Maintenance entry description
 *
 * Return values:
 * VID_RCC                          Request completion code
 *
 * NX_NOTIFY_MAINTENANCE_JOURNAL_CHANGED notification with source object ID is generated
 */
void ClientSession::writeMaintenanceJournal(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

   shared_ptr<NetObj> object = FindObjectById(request.getFieldAsUInt32(VID_OBJECT_ID));
   if (object != nullptr)
   {
      if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_WRITE_MJOURNAL))
      {
         uint32_t rcc = AddMaintenanceJournalRecord(request, m_dwUserId);
         if (rcc == RCC_SUCCESS)
         {
            writeAuditLog(AUDIT_OBJECTS, true, object->getId(), _T("New maintenance journal entry created successfully"));
            debugPrintf(6, _T("New maintenance journal entry for object %d created successfully for object %d"), object->getId());
         }
         response.setField(VID_RCC, rcc);
      }
      else
      {
         response.setField(VID_RCC, RCC_ACCESS_DENIED);
         writeAuditLog(AUDIT_OBJECTS, false, object->getId(), _T("New maintenance journal entry create failed: access denied"));
         debugPrintf(6, _T("New maintenance journal entry create for object %d failed: access denied"), object->getId());
      }
   }
   else
   {
      response.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
      debugPrintf(6, _T("New maintenance journal entry create for object %d failed: invalid object ID"), request.getFieldAsUInt32(VID_OBJECT_ID));
   }

   sendMessage(response);
}

/**
 * Update given maintenance journal entry
 *
 * Called by:
 * CMD_UPDATE_MAINTENANCE_JOURNAL
 *
 * Expected input parameters:
 * VID_OBJECT_ID     Journal owner (source) ID
 * VID_RECORD_ID      Maintenance entry ID
 * VID_DESCRIPTION   Maintenance entry description
 *
 * Return values:
 * VID_RCC                          Request completion code
 *
 * NX_NOTIFY_MAINTENANCE_JOURNAL_CHANGED notification with source object ID is generated
 */
void ClientSession::updateMaintenanceJournal(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

   uint32_t entryId = request.getFieldAsUInt32(VID_RECORD_ID);
   shared_ptr<NetObj> object = FindObjectById(request.getFieldAsUInt32(VID_OBJECT_ID));
   if (object != nullptr)
   {
      if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_WRITE_MJOURNAL))
      {
         uint32_t rcc = UpdateMaintenanceJournalRecord(request, m_dwUserId);
         if (rcc == RCC_SUCCESS)
         {
            writeAuditLog(AUDIT_OBJECTS, true, object->getId(), _T("Maintenance journal entry %d edited successfully"), entryId);
            debugPrintf(6, _T("Maintenance journal entry %d edited succsessfully for object %d"), entryId, object->getId());
         }
         response.setField(VID_RCC, rcc);
      }
      else
      {
         response.setField(VID_RCC, RCC_ACCESS_DENIED);
         writeAuditLog(AUDIT_OBJECTS, false, object->getId(), _T("Access denied on update of maintenance journal entry %u"), entryId);
         debugPrintf(6, _T("Maintenance journal entry %u for object %s [%u] failed: access denied"), entryId, object->getName(), object->getId());
      }
   }
   else
   {
      response.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
      debugPrintf(6, _T("Maintenance journal entry %u edit failed: invalid object ID %u"), entryId, request.getFieldAsUInt32(VID_OBJECT_ID));
   }

   sendMessage(response);
}

/**
 * Clone network map
 */
void ClientSession::cloneNetworkMap(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

   shared_ptr<NetObj> map = FindObjectById(request.getFieldAsUInt32(VID_MAP_ID), OBJECT_NETWORKMAP);
   if (map != nullptr)
   {
      if (map->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
      {
         SharedString name = request.getFieldAsSharedString(VID_NAME);
         SharedString alias = request.getFieldAsSharedString(VID_ALIAS);
         static_cast<NetworkMap&>(*map).clone(name, alias);
         writeAuditLog(AUDIT_OBJECTS, true, map->getId(), _T("Network map cloned successfully"));
         response.setField(VID_RCC, RCC_SUCCESS);
      }
      else
      {
         response.setField(VID_RCC, RCC_ACCESS_DENIED);
         writeAuditLog(AUDIT_OBJECTS, false, map->getId(), _T("Access denied on clone network map"));
      }
   }
   else
   {
      response.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   sendMessage(response);
}

/**
 * Find vendor by MAC address
 */
void ClientSession::findVendorByMac(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());
   FindVendorByMacList(request, &response);
   response.setField(VID_RCC, RCC_SUCCESS);
   sendMessage(response);
}

/**
 * Get OSPF data for given node
 */
void ClientSession::getOspfData(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

   shared_ptr<NetObj> node = FindObjectById(request.getFieldAsUInt32(VID_NODE_ID), OBJECT_NODE);
   if (node != nullptr)
   {
      if (node->checkAccessRights(m_dwUserId, OBJECT_ACCESS_READ))
      {
         if (static_cast<Node&>(*node).isOSPFSupported())
         {
            static_cast<Node&>(*node).writeOSPFDataToMessage(&response);
            response.setField(VID_RCC, RCC_SUCCESS);
         }
         else
         {
            response.setField(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
         }
      }
      else
      {
         response.setField(VID_RCC, RCC_ACCESS_DENIED);
         writeAuditLog(AUDIT_OBJECTS, false, node->getId(), _T("Access denied on reading OSPF information"));
      }
   }
   else
   {
      response.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   sendMessage(response);
}

/**
 * Get asset management schema
 *
 * Called by:
 * CMD_GET_ASSET_MANAGEMENT_SCHEMA
 *
 * Return values:
 * VID_RCC                             Request completion code
 * VID_NUM_ASSET_ATTRIBUTES            Number of attributes
 * VID_AM_ATTRIBUTES_BASE              Base for assert management entries entry list. Also first assert management entries name
 * VID_AM_ATTRIBUTES_BASE + 1          Display name
 * VID_AM_ATTRIBUTES_BASE + 2          Data type
 * VID_AM_ATTRIBUTES_BASE + 3          If it is mandatory
 * VID_AM_ATTRIBUTES_BASE + 4          If it should be uinque
 * VID_AM_ATTRIBUTES_BASE + 5          Auto fill script
 * VID_AM_ATTRIBUTES_BASE + 6          Minimal range
 * VID_AM_ATTRIBUTES_BASE + 7          Maximal range
 * VID_AM_ATTRIBUTES_BASE + 8          System tag
 * VID_AM_ATTRIBUTES_BASE + 9          Enum mapping count
 * VID_AM_ATTRIBUTES_BASE + 10 + n*2   n enum key
 * VID_AM_ATTRIBUTES_BASE + 11 + n*2   n enum value
 *
 * Second attribute starts at VID_AM_ATTRIBUTES_BASE + 256
 */
void ClientSession::getAssetManagementSchema(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());
   AssetManagementSchemaToMessage(&response);
   response.setField(VID_RCC, RCC_SUCCESS);
   sendMessage(response);
}

/**
 * Create new assert attribute
 *
 * Called by:
 * CMD_CREATE_ASSET_ATTRIBUTE
 *
 * Expected input parameters:
 * VID_NAME             attribute name
 * VID_DISPLAY_NAME     display name
 * VID_DATA_TYPE        data type AMDataType
 * VID_IS_MANDATORY     if this attribute is mandatory for filling
 * VID_IS_UNIQUE        if this attribute should be unique
 * VID_SCRIPT           auto fill script
 * VID_RANGE_MIN        min range
 * VID_RANGE_MAX        max range
 * VID_SYSTEM_TYPE      system type AMSystemType
 * VID_ENUM_COUNT       number of items in enum list
 * VID_AM_ENUM_MAP_BASE enum list base (key, value) one by one
 *
 * Return values:
 * VID_RCC                          Request completion code
 */
void ClientSession::createAssetAttribute(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());
   if (checkSysAccessRights(SYSTEM_ACCESS_MANAGE_AM_SCHEMA))
   {
      response.setField(VID_RCC, CreateAssetAttribute(request, *this));
   }
   else
   {
      response.setField(VID_RCC, RCC_ACCESS_DENIED);
      writeAuditLog(AUDIT_SYSCFG, false, 0, _T("Access denied on creating asset attribute"));
   }
   sendMessage(response);
}

/**
 * Update existing asset attribute
 *
 * Called by:
 * CMD_UPDATE_ASSET_ATTRIBUTE
 *
 * Expected input parameters:
 * VID_NAME             assert management attribute name
 * VID_DISPLAY_NAME     display name
 * VID_DATA_TYPE        data type AMDataType
 * VID_IS_MANDATORY     if this attribute is mandatory for filling
 * VID_IS_UNIQUE        if this attribute should be unique
 * VID_SCRIPT           auto fill script
 * VID_RANGE_MIN        min range
 * VID_RANGE_MAX        max range
 * VID_SYSTEM_TYPE      system type AMSystemType
 * VID_ENUM_COUNT       number of items in enum list
 * VID_AM_ENUM_MAP_BASE enum list base (key, value) one by one
 *
 * Return values:
 * VID_RCC                          Request completion code
 */
void ClientSession::updateAssetAttribute(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());
   if (checkSysAccessRights(SYSTEM_ACCESS_MANAGE_AM_SCHEMA))
   {
      response.setField(VID_RCC, UpdateAssetAttribute(request, *this));
   }
   else
   {
      response.setField(VID_RCC, RCC_ACCESS_DENIED);
      writeAuditLog(AUDIT_SYSCFG, false, 0, _T("Access denied on updating asset attribute"));
   }
   sendMessage(response);
}

/**
 * Delete asset attribute
 *
 * Called by:
 * CMD_DELETE_ASSET_ATTRIBUTE
 *
 * Expected input parameters:
 * VID_NAME     attribute name
 *
 * Return values:
 * VID_RCC                          Request completion code
 */
void ClientSession::deleteAssetAttribute(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());
   if (checkSysAccessRights(SYSTEM_ACCESS_MANAGE_AM_SCHEMA))
   {
      response.setField(VID_RCC, DeleteAssetAttribute(request, *this));
   }
   else
   {
      response.setField(VID_RCC, RCC_ACCESS_DENIED);
      writeAuditLog(AUDIT_SYSCFG, false, 0, _T("Access denied on deleting asset attribute"));
   }
   sendMessage(response);
}

/**
 * Set asset property
 *
 * Called by:
 * CMD_SET_ASSET_PROPERTY
 *
 * Expected input parameters:
 * VID_OBJECT_ID     object id
 * VID_NAME          attribute name
 * VID_VALUE         value
 *
 * Return values:
 * VID_RCC                          Request completion code
 * VID_ERROR_TEXT                   Error text if result is not success
 */
void ClientSession::setAssetProperty(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());
   shared_ptr<NetObj> object = FindObjectById(request.getFieldAsUInt32(VID_OBJECT_ID));
   if (object != nullptr)
   {
      SharedString name = request.getFieldAsSharedString(VID_NAME);
      if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_MODIFY))
      {
         if (object->getObjectClass() == OBJECT_ASSET)
         {
            json_t *oldObjectValues = object->toJson();
            SharedString value = request.getFieldAsSharedString(VID_VALUE);
            SharedString oldValue = static_cast<Asset&>(*object).getProperty(name);
            std::pair<uint32_t, String> result = static_cast<Asset&>(*object).setProperty(name, value);
            if (result.first == RCC_SUCCESS)
            {
               json_t *newObjectValues = object->toJson();
               WriteAssetChangeLog(object->getId(), name, oldValue.isNull() ? AssetOperation::Create : AssetOperation::Update, oldValue, value, m_dwUserId, static_cast<Asset&>(*object).getLinkedObjectId());
               writeAuditLogWithValues(AUDIT_OBJECTS, true, object->getId(), oldObjectValues, newObjectValues, _T("Asset property %s changed"), name.cstr());
               json_decref(newObjectValues);
            }
            else
            {
               response.setField(VID_ERROR_TEXT, result.second);
            }
            json_decref(oldObjectValues);
            response.setField(VID_RCC, result.first);
         }
         else
         {
            response.setField(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
         }
      }
      else
      {
         writeAuditLog(AUDIT_OBJECTS, false, object->getId(), _T("Access denied on setting asset property %s"), name.cstr());
         response.setField(VID_RCC, RCC_ACCESS_DENIED);
      }
   }
   else
   {
      response.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   sendMessage(response);
}

/**
 * Delete asset property
 *
 * Called by:
 * CMD_DELETE_ASSET_PROPERTY
 *
 * Expected input parameters:
 * VID_OBJECT_ID     object id
 * VID_NAME          attribute name
 *
 * Return values:
 * VID_RCC                          Request completion code
 */
void ClientSession::deleteAssetProperty(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());
   shared_ptr<NetObj> object = FindObjectById(request.getFieldAsUInt32(VID_OBJECT_ID));
   if (object != nullptr)
   {
      SharedString name = request.getFieldAsSharedString(VID_NAME);
      if (object->checkAccessRights(m_dwUserId, OBJECT_ACCESS_MODIFY))
      {
         if (object->getObjectClass() == OBJECT_ASSET)
         {
            json_t *oldValue = object->toJson();
            uint32_t result = static_cast<Asset&>(*object).deleteProperty(name);
            if (result == RCC_SUCCESS)
            {
               json_t *newValue = object->toJson();
               writeAuditLogWithValues(AUDIT_OBJECTS, true, object->getId(), oldValue, newValue, _T("Asset property %s deleted"), name.cstr());
               json_decref(newValue);
               WriteAssetChangeLog(object->getId(), name, AssetOperation::Delete, nullptr, nullptr, m_dwUserId, static_cast<Asset&>(*object).getLinkedObjectId());
            }
            else
            {
               response.setField(VID_ERROR_TEXT, _T("Attribute is mandatory"));
            }
            json_decref(oldValue);
            response.setField(VID_RCC, result);
         }
         else
         {
            response.setField(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
         }
      }
      else
      {
         response.setField(VID_RCC, RCC_ACCESS_DENIED);
         writeAuditLog(AUDIT_OBJECTS, false, object->getId(), _T("Access denied on deleting asset property %s"), name.cstr());
      }
   }
   else
   {
      response.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   sendMessage(response);
}

/**
 * Link asset with another object
 *
 * Called by:
 * CMD_LINK_ASSET
 *
 * Expected input parameters:
 * VID_ASSET_ID      asset object id
 * VID_OBJECT_ID     other object id
 *
 * Return values:
 * VID_RCC                          Request completion code
 */
void ClientSession::linkAsset(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

   shared_ptr<Asset> asset = static_pointer_cast<Asset>(FindObjectById(request.getFieldAsUInt32(VID_ASSET_ID), OBJECT_ASSET));
   shared_ptr<NetObj> newTarget = FindObjectById(request.getFieldAsUInt32(VID_OBJECT_ID));
   if ((asset != nullptr) && (newTarget != nullptr))
   {
      shared_ptr<NetObj> oldTarget = FindObjectById(asset->getLinkedObjectId());
      shared_ptr<Asset> oldAsset = (newTarget->getAssetId() != 0) ? static_pointer_cast<Asset>(FindObjectById(newTarget->getAssetId(), OBJECT_ASSET)) : shared_ptr<Asset>();
      if ((asset->getLinkedObjectId() == 0) || (oldTarget != nullptr))
      {
         if (asset->checkAccessRights(m_dwUserId, OBJECT_ACCESS_MODIFY) && newTarget->checkAccessRights(m_dwUserId, OBJECT_ACCESS_MODIFY) &&
             ((oldTarget == nullptr) || oldTarget->checkAccessRights(m_dwUserId, OBJECT_ACCESS_MODIFY)) &&
             ((oldAsset == nullptr) || oldAsset->checkAccessRights(m_dwUserId, OBJECT_ACCESS_MODIFY)))
         {
            if (IsValidAssetLinkTargetClass(newTarget->getObjectClass()))
            {
               LinkAsset(asset.get(), newTarget.get(), this);
               response.setField(VID_RCC, RCC_SUCCESS);
            }
            else
            {
               response.setField(VID_RCC, RCC_INCOMPATIBLE_OPERATION);
            }
         }
         else
         {
            writeAuditLog(AUDIT_OBJECTS, false, asset->getId(), _T("Access denied on linking this asset to object \"%s\" [%u]"), newTarget->getName(), newTarget->getId());
            writeAuditLog(AUDIT_OBJECTS, false, newTarget->getId(), _T("Access denied on linking this object to asset \"%s\" [%u]"), asset->getName(), asset->getId());
            if (oldTarget != nullptr)
               writeAuditLog(AUDIT_OBJECTS, false, oldTarget->getId(), _T("Access denied on unlinking this object from asset \"%s\" [%u]"), asset->getName(), asset->getId());
            response.setField(VID_RCC, RCC_ACCESS_DENIED);
         }
      }
      else
      {
         response.setField(VID_RCC, RCC_INTERNAL_ERROR);
      }
   }
   else
   {
      response.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   sendMessage(response);
}

/**
 * Remove asset's with another object
 *
 * Called by:
 * CMD_UNLINK_ASSET
 *
 * Expected input parameters:
 * VID_ASSET_ID      asset object id
 *
 * Return values:
 * VID_RCC                          Request completion code
 */
void ClientSession::unlinkAsset(const NXCPMessage& request)
{
   NXCPMessage response(CMD_REQUEST_COMPLETED, request.getId());

   shared_ptr<Asset> asset = static_pointer_cast<Asset>(FindObjectById(request.getFieldAsUInt32(VID_ASSET_ID), OBJECT_ASSET));
   if (asset != nullptr)
   {
      if (asset->getLinkedObjectId() != 0)
      {
         shared_ptr<NetObj> target = FindObjectById(asset->getLinkedObjectId());
         if (target != nullptr)
         {
            if (asset->checkAccessRights(m_dwUserId, OBJECT_ACCESS_MODIFY) && target->checkAccessRights(m_dwUserId, OBJECT_ACCESS_MODIFY))
            {
               UnlinkAsset(asset.get(), this);
               response.setField(VID_RCC, RCC_SUCCESS);
            }
            else
            {
               writeAuditLog(AUDIT_OBJECTS, false, asset->getId(), _T("Access denied on unlinking this asset from object \"%s\" [%u]"), target->getName(), target->getId());
               writeAuditLog(AUDIT_OBJECTS, false, target->getId(), _T("Access denied on unlinking this object from asset \"%s\" [%u]"), asset->getName(), asset->getId());
               response.setField(VID_RCC, RCC_ACCESS_DENIED);
            }
         }
         else
         {
            response.setField(VID_RCC, RCC_INTERNAL_ERROR);
         }
      }
      else
      {
         response.setField(VID_RCC, RCC_SUCCESS);
      }
   }
   else
   {
      response.setField(VID_RCC, RCC_INVALID_OBJECT_ID);
   }

   sendMessage(response);
}
