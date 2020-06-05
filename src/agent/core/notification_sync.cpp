/*
** NetXMS multiplatform core agent
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
** File: notifications_sync.cpp
**
**/

#include "nxagentd.h"

#define DEBUG_TAG _T("notifications")
#define ONE_DAY 60*60*24
#define SYNC_DATA_SIZE        1000
#define SYNC_DATA_SIZE_TEXT   _T("1000")

/**
 * Notification processor queue
 */
ObjectQueue<NXCPMessage> g_notificationProcessorQueue(256, Ownership::True);
static THREAD s_notificationProcesserThread = INVALID_THREAD_HANDLE;

extern uint32_t g_dcOfflineExpirationTime;
static uint32_t s_notificationId = 0;

enum class SyncStatus
{
   online = 0,
   synchroizing = 1
};

struct NotificationSyncStatus
{
   SyncStatus status;
   uint64_t serverId;
};

/**
 * Server status
 */
static ObjectArray<NotificationSyncStatus> s_serverSyncStatus(1, 16, Ownership::True);
static Mutex s_serverSyncStatusLock;

/**
 * Update notificaiton id
 */
void UpdateNotificationId()
{
   DB_HANDLE hdb = GetLocalDatabaseHandle();
   if (hdb == nullptr)
      return;
   DB_RESULT result = DBSelect(hdb, _T("SELECT MAX(id) FROM notification_data"));
   if (result != nullptr)
   {
      if (DBGetNumRows(result) > 0)
         s_notificationId = DBGetFieldULong(result, 0, 0) + 1;
      else
         s_notificationId = 1;

      DBFreeResult(result);
   }
}

/**
 * Process notifications
 */
static THREAD_RESULT THREAD_CALL NotificationSender(void *arg)
{
   nxlog_debug_tag(DEBUG_TAG, 1, _T("Notification sender thread started"));
   while(true)
   {
      NXCPMessage *msg = static_cast<NXCPMessage*>(g_notificationProcessorQueue.getOrBlock());
      if (msg == INVALID_POINTER_VALUE)
         break;

      nxlog_debug_tag(DEBUG_TAG, 6, _T("NotificationSender: Message came"));

      if (g_dwFlags & AF_SUBAGENT_LOADER)
      {
         nxlog_debug_tag(DEBUG_TAG, 8, _T("NotificationSender: Sending message to master agent"));
         if(!SendMessageToMasterAgent(msg))
         {
            nxlog_debug_tag(DEBUG_TAG, 6, _T("NotificationSender: Filed to send message to master agent"));
         }
      }
      else
      {
         DB_HANDLE hdb = GetLocalDatabaseHandle();
         DB_STATEMENT hStmt = nullptr;

         s_serverSyncStatusLock.lock();
         nxlog_debug_tag(DEBUG_TAG, 8, _T("NotificationSender: Server count: %d"), s_serverSyncStatus.size());
         for(int i = 0; i < s_serverSyncStatus.size(); i++)
         {
            bool sent = false;
            if (s_serverSyncStatus.get(i)->status == SyncStatus::online)
            {
               MutexLock(g_hSessionListAccess);
               for(int j = 0; j < g_dwMaxSessions; j++)
               {
                  if (g_pSessionList[j] != nullptr)
                  {
                     if (g_pSessionList[j]->getServerId() == s_serverSyncStatus.get(i)->serverId &&
                           g_pSessionList[j]->canAcceptTraps())
                     {
                        g_pSessionList[j]->sendMessage(msg);
                        sent = true;
                        break;
                     }
                  }
               }
               MutexUnlock(g_hSessionListAccess);
            }

            if (!sent)
            {
               if (hdb != nullptr)
               {
                  if (hStmt == nullptr)
                  {
                     hStmt = DBPrepare(hdb, _T("INSERT INTO notification_data (server_id,serialized_data,id) VALUES (?,?,?)"), true);
                  }
                  if(hStmt != nullptr)
                  {
                     NXCP_MESSAGE *rawMessage = msg->serialize(true);
                     char *base64Encoded = nullptr;
                     base64_encode_alloc(reinterpret_cast<char*>(rawMessage), ntohl(rawMessage->size), &base64Encoded);
                     free(rawMessage);

                     DBBind(hStmt, 1, DB_SQLTYPE_BIGINT, s_serverSyncStatus.get(i)->serverId);
                     DBBind(hStmt, 2, DB_SQLTYPE_TEXT, DB_CTYPE_UTF8_STRING, base64Encoded, DB_BIND_DYNAMIC);
                     DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, s_notificationId++);
                     DBExecute(hStmt);
                     s_serverSyncStatus.get(i)->status = SyncStatus::synchroizing;
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
      }
      delete msg;
   }
   nxlog_debug_tag(DEBUG_TAG, 1, _T("Notification sender thread stopped"));
   return THREAD_OK;
}

/**
 * Synchronize stored notifications with server
 */
static void NotificationSync(CommSession *session)
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
      if (result != nullptr)
      {
         count = DBGetNumRows(result);
         for(int i = 0; i < count; i++)
         {
            char *base64Msg = DBGetFieldUTF8(result, i, 0, nullptr, 0);
            char *rawMsg = nullptr;
            size_t outlen = 0;
            base64_decode_alloc(base64Msg, strlen(base64Msg), &rawMsg, &outlen);
            MemFree(base64Msg);

            NXCPMessage *msg = NXCPMessage::deserialize(reinterpret_cast<NXCP_MESSAGE *>(rawMsg));
            MemFree(rawMsg);
            if (msg == nullptr)
            {
               nxlog_debug_tag(DEBUG_TAG, 1, _T("NotificationSync: Filed to deserialize message"));
               continue;
            }

            success = session->sendMessage(msg);
            delete msg;

            if (success)
               lastId = DBGetFieldULong(result, i, 1);
            else
               break;
         }
         DBFreeResult(result);

         if (lastId > 0)
         {
            nxlog_debug_tag(DEBUG_TAG, 7, _T("NotificationSync: Delete records starting forms from %d id"), lastId);
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
      else
         break;
   }

   if(success && count < SYNC_DATA_SIZE)
   {
      for(int i = 0; i < s_serverSyncStatus.size(); i++)
      {
         if (session->getServerId() == s_serverSyncStatus.get(i)->serverId)
         {
            s_serverSyncStatus.get(i)->status = SyncStatus::online;
         }
      }
      UpdateNotificationId();
   }

   if (locked)
      s_serverSyncStatusLock.unlock();

   nxlog_debug_tag(DEBUG_TAG, 4, _T("Vacuum local database"));
   DBQuery(hdb, _T("VACUUM"));

   nxlog_debug_tag(DEBUG_TAG, 1, _T("Notification synchronization finished"));
   session->decRefCount();
}

/**
 * Update server last connection time
 */
static void UpdateLastConnectionTime(uint64_t serverId, time_t now)
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
      DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, now);
      DBBind(hStmt, 2, DB_SQLTYPE_BIGINT, serverId);
      DBExecute(hStmt);
      DBFreeStatement(hStmt);
   }
}

/**
 * Remove old server information
 */
void NotificationHousekeeper(void *arg)
{
   //Remove retained server
   DB_HANDLE hdb = GetLocalDatabaseHandle();
   if (hdb == nullptr)
      return;

   nxlog_debug_tag(DEBUG_TAG, 1, _T("Notification housekeeper thread started"));
   DB_RESULT result = DBSelect(hdb, _T("SELECT server_id,last_connection_time FROM notification_servers"));
   time_t now = time(nullptr);
   uint32_t retentinTimeInSec = g_dcOfflineExpirationTime * ONE_DAY;
   if (result != nullptr)
   {
      int count = DBGetNumRows(result);
      for(int i = 0; i < count; i++)
      {
         uint64_t serverId = DBGetFieldUInt64(result, i, 0);
         time_t lastConnectionTime = DBGetFieldULong(result, i, 1);
         if ((now - lastConnectionTime) >  retentinTimeInSec)
         {
            //remove server form the list
            s_serverSyncStatusLock.lock();
            for(int j = 0; j < s_serverSyncStatus.size(); j++)
               if(s_serverSyncStatus.get(j)->serverId == serverId)
               {
                  s_serverSyncStatus.remove(j);
                  break;
               }
            s_serverSyncStatusLock.unlock();

            //delete form database
            nxlog_debug_tag(DEBUG_TAG, 5, _T("Delete old data for server: ")UINT64X_FMT(_T("016")), serverId);
            TCHAR query[256];
            DBBegin(hdb);
            _sntprintf(query, 256, _T("DELETE FROM notification_servers WHERE server_id=") UINT64_FMT, serverId);
            DBQuery(hdb, query);
            _sntprintf(query, 256, _T("DELETE FROM notification_data WHERE server_id=") UINT64_FMT, serverId);
            DBQuery(hdb, query);
            DBCommit(hdb);
         }
      }
      DBFreeResult(result);
   }

   //Update notification id
   s_serverSyncStatusLock.lock();
   UpdateNotificationId();
   s_serverSyncStatusLock.unlock();

   //Update last connection time for all connected sessions
   MutexLock(g_hSessionListAccess);
   for(int i = 0; i < g_dwMaxSessions; i++)
   {
      if (g_pSessionList[i] != nullptr)
      {
         if (g_pSessionList[i]->canAcceptTraps())
         {
            UpdateLastConnectionTime(g_pSessionList[i]->getServerId(), now);
         }
      }
   }
   MutexUnlock(g_hSessionListAccess);
   ThreadPoolScheduleRelative(g_commThreadPool, ONE_DAY*1000, NotificationHousekeeper, nullptr);
   nxlog_debug_tag(DEBUG_TAG, 1, _T("Notification housekeeper thread stopped"));
}

/**
 * Update server information on incomming session and start reconcilation thread if required
 */
void OnIncommingSesstion(CommSession *session)
{
   time_t now = time(nullptr);
   UpdateLastConnectionTime(session->getServerId(), now);
   session->incRefCount();

   s_serverSyncStatusLock.lock();
   for(int i = 0; i < s_serverSyncStatus.size(); i++)
      if(s_serverSyncStatus.get(i)->serverId == session->getServerId())
      {
         s_serverSyncStatus.get(i)->status = SyncStatus::synchroizing;
         break;
      }
   s_serverSyncStatusLock.unlock();

   TCHAR serverIAsText[32];
   _sntprintf(serverIAsText, 32, UINT64X_FMT(_T("016")), session->getServerId());

   ThreadPoolExecuteSerialized(g_commThreadPool, serverIAsText, NotificationSync, session);
}

/**
 * Start data synchronization thread
 */
void StartNotificationSync()
{
   DB_HANDLE hdb = GetLocalDatabaseHandle();
   DB_RESULT result = nullptr;
   if(hdb != nullptr)
   {
      result = DBSelect(hdb, _T("SELECT server_id FROM notification_servers"));
   }
   if (result != nullptr)
   {
      int count = DBGetNumRows(result);
      for(int i = 0; i < count; i++)
      {
         NotificationSyncStatus *si = new NotificationSyncStatus();
         si->serverId = DBGetFieldUInt64(result, i, 0);
         si->status = SyncStatus::synchroizing;
         s_serverSyncStatus.add(si);
      }
      DBFreeResult(result);
   }
   nxlog_debug_tag(DEBUG_TAG, 7, _T("StartNotificationSync: Loaded %d servers"), s_serverSyncStatus.size());
   UpdateNotificationId();

   s_notificationProcesserThread = ThreadCreateEx(NotificationSender, 0, nullptr);
   ThreadPoolExecute(g_commThreadPool, NotificationHousekeeper, nullptr);
}

/**
 * Stop data sync
 */
void ShutdownNotificationSync()
{
   nxlog_debug_tag(DEBUG_TAG, 5, _T("Waiting for notification sender thread termination"));
   g_notificationProcessorQueue.setShutdownMode();
   ThreadJoin(s_notificationProcesserThread);
}

/**
 * Handler for notification information parameters
 */
LONG H_NotificationStats(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   ret_uint(value, static_cast<UINT32>(g_notificationProcessorQueue.size()));
   return SYSINFO_RC_SUCCESS;
}
