/*
** NetXMS multiplatform core agent
** Copyright (C) 2003-2021 Raden Solutions
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
** File: notifications_sync.cpp
**
**/

#include "nxagentd.h"

#define DEBUG_TAG _T("notifications")
#define ONE_DAY (60 * 60 * 24)
#define SYNC_DATA_SIZE        1000
#define SYNC_DATA_SIZE_TEXT   _T("1000")

/**
 * Offline data expiration time (in days)
 */
extern uint32_t g_dcOfflineExpirationTime;

/**
 * Notification processor queue
 */
ObjectQueue<NXCPMessage> g_notificationProcessorQueue(256, Ownership::True);
static THREAD s_notificationProcessorThread = INVALID_THREAD_HANDLE;

/**
 * Synchronization status
 */
enum class SyncStatus
{
   ONLINE = 0,
   SYNCHRONIZING = 1
};

/**
 * Server synchronization status
 */
struct ServerRegistration
{
   uint64_t serverId;
   uint32_t recordId;
   SyncStatus status;
};

/**
 * Server status
 */
static ObjectArray<ServerRegistration> s_serverSyncStatus(16, 16, Ownership::True);
static Mutex s_serverSyncStatusLock;

/**
 * Process notifications - external subagent
 */
static void ExtSubagentLoaderNotificationProcessor()
{
   nxlog_debug_tag(DEBUG_TAG, 1, _T("Notification processor started"));
   while(true)
   {
      NXCPMessage *msg = g_notificationProcessorQueue.getOrBlock();
      if (msg == INVALID_POINTER_VALUE)
         break;

      nxlog_debug_tag(DEBUG_TAG, 8, _T("NotificationProcessor: sending message to master agent"));
      if (!SendMessageToMasterAgent(msg))
      {
         nxlog_debug_tag(DEBUG_TAG, 6, _T("NotificationProcessor: failed to send message to master agent"));
         if (g_notificationProcessorQueue.size() < 10000)
         {
            g_notificationProcessorQueue.insert(msg);
            msg = nullptr; // Prevent destruction
         }
      }
      delete msg;
   }
   nxlog_debug_tag(DEBUG_TAG, 1, _T("Notification processor stopped"));
}

/**
 * Process notifications - master agent
 */
static void MasterNotificationProcessor()
{
   nxlog_debug_tag(DEBUG_TAG, 1, _T("Notification processor started"));
   while(true)
   {
      NXCPMessage *msg = g_notificationProcessorQueue.getOrBlock();
      if (msg == INVALID_POINTER_VALUE)
         break;

      nxlog_debug_tag(DEBUG_TAG, 6, _T("NotificationProcessor: new message received"));

      DB_STATEMENT hStmt = nullptr;

      s_serverSyncStatusLock.lock();
      nxlog_debug_tag(DEBUG_TAG, 8, _T("NotificationProcessor: Server count: %d"), s_serverSyncStatus.size());
      for(int i = 0; i < s_serverSyncStatus.size(); i++)
      {
         bool sent = false;
         ServerRegistration *server = s_serverSyncStatus.get(i);
         if (server->status == SyncStatus::ONLINE)
         {
            g_sessionLock.lock();
            for(int j = 0; j < g_sessions.size(); j++)
            {
               CommSession *session = g_sessions.get(j);
               if ((session->getServerId() == server->serverId) && session->canAcceptTraps())
               {
                  sent = session->sendMessage(msg);
                  break;
               }
            }
            g_sessionLock.unlock();
         }

         if (!sent)
         {
            DB_HANDLE hdb = GetLocalDatabaseHandle();
            if (hdb != nullptr)
            {
               if (hStmt == nullptr)
               {
                  hStmt = DBPrepare(hdb, _T("INSERT INTO notification_data (server_id,id,serialized_data) VALUES (?,?,?)"), true);
               }
               if (hStmt != nullptr)
               {
                  NXCP_MESSAGE *rawMessage = msg->serialize(true);
                  char *base64Encoded = nullptr;
                  base64_encode_alloc(reinterpret_cast<char*>(rawMessage), ntohl(rawMessage->size), &base64Encoded);
                  MemFree(rawMessage);

                  DBBind(hStmt, 1, DB_SQLTYPE_BIGINT, server->serverId);
                  DBBind(hStmt, 2, DB_SQLTYPE_INTEGER, server->recordId++);
                  DBBind(hStmt, 3, DB_SQLTYPE_TEXT, DB_CTYPE_UTF8_STRING, base64Encoded, DB_BIND_DYNAMIC);
                  DBExecute(hStmt);
                  server->status = SyncStatus::SYNCHRONIZING;
                  nxlog_debug_tag(DEBUG_TAG, 6, _T("NotificationSender: Notification message saved to database"));
               }
            }
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG, 8, _T("NotificationSender: Notification message successfully forwarded to server"));
         }
      }
      s_serverSyncStatusLock.unlock();
      DBFreeStatement(hStmt);

      delete msg;
   }
   nxlog_debug_tag(DEBUG_TAG, 1, _T("Notification processor stopped"));
}

/**
 * Synchronize stored notifications with server
 */
static void NotificationSync(const shared_ptr<CommSession>& session)
{
   DB_HANDLE hdb = GetLocalDatabaseHandle();
   if (hdb == nullptr)
      return;

   nxlog_debug_tag(DEBUG_TAG, 1, _T("Notification synchronization started"));
   TCHAR query[256];
   bool locked = false;
   bool success = true;
   int count = 0;
   while (!IsShutdownInProgress())
   {
      uint32_t lastId = 0;
      _sntprintf(query, 256, _T("SELECT serialized_data,id FROM notification_data WHERE server_id=") UINT64_FMT _T(" ORDER BY id LIMIT ") SYNC_DATA_SIZE_TEXT, session->getServerId());
      DB_RESULT result = DBSelect(hdb, query);
      if (result == nullptr)
         break;

      count = DBGetNumRows(result);
      for(int i = 0; i < count; i++)
      {
         char *base64Msg = DBGetFieldUTF8(result, i, 0, nullptr, 0);
         char *rawMsg = nullptr;
         size_t outlen = 0;
         base64_decode_alloc(base64Msg, strlen(base64Msg), &rawMsg, &outlen);
         MemFree(base64Msg);

         NXCPMessage *msg = NXCPMessage::deserialize(reinterpret_cast<NXCP_MESSAGE*>(rawMsg));
         MemFree(rawMsg);
         if (msg == nullptr)
         {
            nxlog_debug_tag(DEBUG_TAG, 1, _T("NotificationSync: failed to deserialize message"));
            continue;
         }

         success = session->sendMessage(msg);
         delete msg;

         if (!success)
            break;

         lastId = DBGetFieldULong(result, i, 1);
      }
      DBFreeResult(result);

      if (lastId > 0)
      {
         nxlog_debug_tag(DEBUG_TAG, 7, _T("NotificationSync: Delete records with ID below %u for server ") UINT64X_FMT(_T("016")), lastId, session->getServerId());
         _sntprintf(query, 256, _T("DELETE FROM notification_data WHERE server_id=") UINT64_FMT _T(" AND id <=%d"), session->getServerId(), lastId);
         DBQuery(hdb, query);
      }

      if ((count < SYNC_DATA_SIZE) && !locked)
      {
         locked = true;
         s_serverSyncStatusLock.lock();
         continue;
      }

      if (!success || (count < SYNC_DATA_SIZE))
      {
         break;
      }
   }

   if (success && (count < SYNC_DATA_SIZE) && !IsShutdownInProgress())
   {
      for(int i = 0; i < s_serverSyncStatus.size(); i++)
      {
         ServerRegistration *server = s_serverSyncStatus.get(i);
         if (session->getServerId() == server->serverId)
         {
            server->status = SyncStatus::ONLINE;
            server->recordId = 1;
         }
      }
   }

   if (locked)
      s_serverSyncStatusLock.unlock();

   if (!IsShutdownInProgress())
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Vacuum local database"));
      DBQuery(hdb, _T("VACUUM"));
   }

   nxlog_debug_tag(DEBUG_TAG, 1, _T("Notification synchronization finished"));
}

/**
 * Update server registration record in database
 */
static void UpdateServerRegistration(uint64_t serverId, time_t lastConnect)
{
   DB_HANDLE hdb = GetLocalDatabaseHandle();
   if (hdb == nullptr)
      return;

   DB_STATEMENT hStmt;
   if (IsDatabaseRecordExist(hdb, _T("notification_servers"), _T("server_id"), serverId))
      hStmt = DBPrepare(hdb, _T("UPDATE notification_servers SET last_connection_time=? WHERE server_id=?"));
   else
      hStmt = DBPrepare(hdb, _T("INSERT INTO notification_servers (last_connection_time,server_id) VALUES (?,?)"));
   if (hStmt != nullptr)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(lastConnect));
      DBBind(hStmt, 2, DB_SQLTYPE_BIGINT, serverId);
      DBExecute(hStmt);
      DBFreeStatement(hStmt);
   }
}

/**
 * Remove old server information
 */
static void NotificationHousekeeper()
{
   DB_HANDLE hdb = GetLocalDatabaseHandle();
   if (hdb == nullptr)
      return;

   nxlog_debug_tag(DEBUG_TAG, 1, _T("Notification housekeeper started"));
   DB_RESULT result = DBSelect(hdb, _T("SELECT server_id,last_connection_time FROM notification_servers"));
   time_t now = time(nullptr);
   if (result != nullptr)
   {
      int count = DBGetNumRows(result);
      for(int i = 0; i < count; i++)
      {
         uint64_t serverId = DBGetFieldUInt64(result, i, 0);
         time_t lastConnectionTime = DBGetFieldULong(result, i, 1);
         time_t retentionTimeInSec = g_dcOfflineExpirationTime * ONE_DAY;
         if ((now - lastConnectionTime) >  retentionTimeInSec)
         {
            // remove server from the list
            s_serverSyncStatusLock.lock();
            for(int j = 0; j < s_serverSyncStatus.size(); j++)
               if (s_serverSyncStatus.get(j)->serverId == serverId)
               {
                  s_serverSyncStatus.remove(j);
                  break;
               }
            s_serverSyncStatusLock.unlock();

            // delete from database
            nxlog_debug_tag(DEBUG_TAG, 5, _T("Delete old notifications for server ") UINT64X_FMT(_T("016")), serverId);
            if (DBBegin(hdb))
            {
               TCHAR query[256];
               _sntprintf(query, 256, _T("DELETE FROM notification_servers WHERE server_id=") UINT64_FMT, serverId);
               bool success = DBQuery(hdb, query);
               if (success)
               {
                  _sntprintf(query, 256, _T("DELETE FROM notification_data WHERE server_id=") UINT64_FMT, serverId);
                  success = DBQuery(hdb, query);
               }
               if (success)
                  DBCommit(hdb);
               else
                  DBRollback(hdb);
            }
         }
      }
      DBFreeResult(result);
   }

   // Update last connection time for all connected sessions
   g_sessionLock.lock();
   for(int i = 0; i < g_sessions.size(); i++)
   {
      CommSession *session = g_sessions.get(i);
      if (session->canAcceptTraps())
      {
         UpdateServerRegistration(session->getServerId(), now);
      }
   }
   g_sessionLock.unlock();
   ThreadPoolScheduleRelative(g_commThreadPool, ONE_DAY * 1000, NotificationHousekeeper);
   nxlog_debug_tag(DEBUG_TAG, 1, _T("Notification housekeeper execution completed"));
}

/**
 * Register session for notification processing
 */
void RegisterSessionForNotifications(const shared_ptr<CommSession>& session)
{
   time_t now = time(nullptr);
   UpdateServerRegistration(session->getServerId(), now);

   s_serverSyncStatusLock.lock();
   int i;
   for(i = 0; i < s_serverSyncStatus.size(); i++)
      if (s_serverSyncStatus.get(i)->serverId == session->getServerId())
      {
         s_serverSyncStatus.get(i)->status = SyncStatus::SYNCHRONIZING;
         break;
      }
   if (i == s_serverSyncStatus.size()) // New server
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("RegisterSessionForNotifications: registering new server ") UINT64X_FMT(_T("016")), session->getServerId());
      auto s = new ServerRegistration();
      s->serverId = session->getServerId();
      s->recordId = 0;
      s->status = SyncStatus::ONLINE;
      s_serverSyncStatus.add(s);
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("RegisterSessionForNotifications: starting background sync for server ") UINT64X_FMT(_T("016")), session->getServerId());
      TCHAR key[64];
      _sntprintf(key, 64, _T("NSync-") UINT64X_FMT(_T("016")), session->getServerId());
      ThreadPoolExecuteSerialized(g_commThreadPool, key, NotificationSync, session);
   }
   s_serverSyncStatusLock.unlock();
}

/**
 * Start notification processor
 */
void StartNotificationProcessor()
{
   if (g_dwFlags & AF_SUBAGENT_LOADER)
   {
      s_notificationProcessorThread = ThreadCreateEx(ExtSubagentLoaderNotificationProcessor);
   }
   else
   {
      DB_HANDLE hdb = GetLocalDatabaseHandle();
      if(hdb != nullptr)
      {
         DB_RESULT result = DBSelect(hdb, _T("SELECT server_id,coalesce((SELECT max(id) FROM notification_data d WHERE d.server_id=s.server_id),0) AS notification_id FROM notification_servers s"));
         if (result != nullptr)
         {
            int count = DBGetNumRows(result);
            for(int i = 0; i < count; i++)
            {
               auto s = new ServerRegistration();
               s->serverId = DBGetFieldUInt64(result, i, 0);
               s->recordId = DBGetFieldLong(result, i, 1) + 1;
               s->status = SyncStatus::SYNCHRONIZING;
               s_serverSyncStatus.add(s);
            }
            DBFreeResult(result);
         }
      }
      nxlog_debug_tag(DEBUG_TAG, 4, _T("StartNotificationProcessor: Loaded %d servers"), s_serverSyncStatus.size());

      s_notificationProcessorThread = ThreadCreateEx(MasterNotificationProcessor);
      ThreadPoolExecute(g_commThreadPool, NotificationHousekeeper);
   }
}

/**
 * Stop notification processor
 */
void StopNotificationProcessor()
{
   nxlog_debug_tag(DEBUG_TAG, 5, _T("Waiting for notification processor termination"));
   g_notificationProcessorQueue.setShutdownMode();
   ThreadJoin(s_notificationProcessorThread);
}

/**
 * Put message to notification queue. Ownership will be taken by the queue.
 */
void QueueNotificationMessage(NXCPMessage *msg)
{
   msg->setField(VID_ZONE_UIN, g_zoneUIN);
   g_notificationProcessorQueue.put(msg);
}

/**
 * Handler for notification information parameters
 */
LONG H_NotificationStats(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   ret_uint(value, static_cast<uint32_t>(g_notificationProcessorQueue.size()));
   return SYSINFO_RC_SUCCESS;
}
