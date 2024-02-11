/*
** NetXMS - Network Management System
** Copyright (C) 2019-2022 Raden Solutions
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
** File: ua_notification_item.cpp
**
**/

#include "nxcore.h"

#define DEBUG_TAG _T("useragent")

Mutex g_userAgentNotificationListMutex;
ObjectArray<UserAgentNotificationItem> g_userAgentNotificationList(0, 16, Ownership::True);

/**
 * Init user agent messages
 */
void InitUserAgentNotifications()
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   DB_RESULT hResult = DBSelect(hdb, _T("SELECT id,message,objects,start_time,end_time,recall,on_startup,creation_time,created_by FROM user_agent_notifications"));
   if (hResult != nullptr)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         g_userAgentNotificationList.add(new UserAgentNotificationItem(hResult, i));
      }
      DBFreeResult(hResult);
   }
   DBConnectionPoolReleaseConnection(hdb);
   nxlog_debug_tag(DEBUG_TAG, 2, _T("%d user agent messages loaded"), g_userAgentNotificationList.size());
}

/**
 * Delete expired user support application notifications
 */
void DeleteExpiredUserAgentNotifications(DB_HANDLE hdb, UINT32 retentionTime)
{
   g_userAgentNotificationListMutex.lock();
   time_t now = time(nullptr);
   Iterator<UserAgentNotificationItem> it = g_userAgentNotificationList.begin();
   while (it.hasNext())
   {
      UserAgentNotificationItem *uan = it.next();
      if((now - uan->getEndTime()) >= retentionTime && uan->hasNoRef())
      {
         it.remove();
         break;
      }
   }
   g_userAgentNotificationListMutex.unlock();

   TCHAR query[256];
   _sntprintf(query, sizeof(query) / sizeof(TCHAR), _T("DELETE FROM user_agent_notifications WHERE end_time>=") INT64_FMT,
         static_cast<INT64>(now - retentionTime));
   DBQuery(hdb, query);

   _sntprintf(query, sizeof(query) / sizeof(TCHAR), _T("DELETE FROM user_agent_notifications WHERE creation_time>=") INT64_FMT _T(" AND end_time=0"),
         static_cast<INT64>(now - retentionTime));
   DBQuery(hdb, query);
}

void FillUserAgentNotificationsAll(NXCPMessage *msg, Node *node)
{
   g_userAgentNotificationListMutex.lock();
   int base = VID_UA_NOTIFICATION_BASE;
   time_t now = time(nullptr);
   int count = 0;
   for (int i = 0; i < g_userAgentNotificationList.size(); i++)
   {
      UserAgentNotificationItem *uan = g_userAgentNotificationList.get(i);
      if ((uan->getEndTime() >= now) && !uan->isRecalled() && uan->isApplicable(node->getId()))
      {
         uan->fillMessage(base, msg, false);
         base += 10;
         count++;
      }
   }
   msg->setField(VID_UA_NOTIFICATION_COUNT, count);
   g_userAgentNotificationListMutex.unlock();
}

/**
 * Create new user support application notification
 */
UserAgentNotificationItem *CreateNewUserAgentNotification(const TCHAR *message, const IntegerArray<uint32_t>& objects, time_t startTime, time_t endTime, bool onStartup, uint32_t userId)
{
   g_userAgentNotificationListMutex.lock();
   UserAgentNotificationItem *uan = new UserAgentNotificationItem(message, objects, startTime, endTime, onStartup, userId);
   g_userAgentNotificationList.add(uan);
   uan->incRefCount();
   uan->incRefCount();
   g_userAgentNotificationListMutex.unlock();

   nxlog_debug_tag(DEBUG_TAG, 4, _T("New user support application notification created (id=%u msg=%s)"), uan->getId(), uan->getMessage());

   ThreadPoolExecute(g_clientThreadPool, uan, &UserAgentNotificationItem::processUpdate);
   NotifyClientSessions(NX_NOTIFY_USER_AGENT_MESSAGE_CHANGED, uan->getId());
   return uan;
}

/**
 * Create user agent message form database
 */
UserAgentNotificationItem::UserAgentNotificationItem(DB_RESULT result, int row) : m_objects(64, 64)
{
   m_id = DBGetFieldULong(result, row, 0);
   DBGetField(result, row, 1, m_message, MAX_USER_AGENT_MESSAGE_SIZE);

   TCHAR objectList[MAX_DB_STRING];
   DBGetField(result, row, 2, objectList, MAX_DB_STRING);
   int count;
   TCHAR **ids = SplitString(objectList, _T(','), &count);
   for(int i = 0; i < count; i++)
   {
      m_objects.add(_tcstoul(ids[i], nullptr, 10));
      MemFree(ids[i]);
   }
   MemFree(ids);

   m_startTime = DBGetFieldLong(result, row, 3);
   m_endTime = DBGetFieldLong(result, row, 4);
   m_recall = DBGetFieldLong(result, row, 5) ? true : false;
   m_onStartup = DBGetFieldLong(result, row, 6) ? true : false;
   m_creationTime = DBGetFieldLong(result, row, 7);
   m_creatorId = DBGetFieldLong(result, row, 8);
   m_refCount = 0;
}

/**
 * Create new user agent message
 */
UserAgentNotificationItem::UserAgentNotificationItem(const TCHAR *message, const IntegerArray<uint32_t>& objects, time_t startTime, time_t endTime, bool startup, uint32_t userId) : m_objects(objects)
{
   m_id = CreateUniqueId(IDG_UA_MESSAGE);
   _tcslcpy(m_message, message, MAX_USER_AGENT_MESSAGE_SIZE);
   m_startTime = startTime;
   m_endTime = endTime;
   m_recall = false;
   m_onStartup = startup;
   m_refCount = 0;
   m_creationTime = time(nullptr);
   m_creatorId = userId;
}

/**
 * Check if user agent message is applicable on the node
 */
bool UserAgentNotificationItem::isApplicable(uint32_t nodeId) const
{
   for (int i = 0; i < m_objects.size(); i++)
   {
      uint32_t id = m_objects.get(i);
      if ((id == nodeId) || IsParentObject(id, nodeId))
         return true;
   }
   return false;
}

/**
 * Check if object is node - then send update, if container - iterate over children
 */
static void SendUpdate(NXCPMessage *msg, NetObj *object)
{
   if (object->getObjectClass() == OBJECT_NODE)
   {
      if ((static_cast<Node*>(object)->getCapabilities() & NC_HAS_USER_AGENT) == 0)
         return;

      shared_ptr<AgentConnectionEx> conn = static_cast<Node *>(object)->getAgentConnection(false);
      if (conn != nullptr)
      {
         msg->setId(conn->generateRequestId());
         conn->sendMessage(msg);
      }
   }
   else
   {
      unique_ptr<SharedObjectArray<NetObj>> children = object->getChildren();
      for(int i = 0; i < children->size(); i++)
      {
         SendUpdate(msg, children->get(i));
      }
   }
}

/**
 * Method called on user agent message creation or update(recall)
 */
void UserAgentNotificationItem::processUpdate()
{
   NXCPMessage msg;
   msg.setCode(m_recall ? CMD_RECALL_UA_NOTIFICATION : CMD_ADD_UA_NOTIFICATION);
   fillMessage(VID_UA_NOTIFICATION_BASE, &msg, false);

   //create and fill message
   for (int i = 0; i < m_objects.size(); i++)
   {
      shared_ptr<NetObj> object = FindObjectById(m_objects.get(i));
      if (object != nullptr)
         SendUpdate(&msg, object.get());
   }

   saveToDatabase();
   decRefCount();
}

/**
 * Fill message with user agent message data
 */
void UserAgentNotificationItem::fillMessage(UINT32 base, NXCPMessage *msg, bool fullInfo) const
{
   msg->setField(base, m_id);
   msg->setField(base + 1, m_message);
   msg->setFieldFromTime(base + 2, m_startTime);
   msg->setFieldFromTime(base + 3, m_endTime);
   msg->setField(base + 4, m_onStartup);
   if (fullInfo) // do not send info to agent
   {
      msg->setFieldFromInt32Array(base + 5, &m_objects);
      msg->setField(base + 6, m_recall);
      msg->setFieldFromTime(base + 7, m_creationTime);
      msg->setField(base + 8, m_creatorId);
   }
}

/**
 * Save object to database
 */
void UserAgentNotificationItem::saveToDatabase()
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   static const TCHAR *columns[] = {
      _T("message"), _T("objects"), _T("start_time"), _T("end_time"), _T("recall"), _T("on_startup"),
      _T("creation_time"),  _T("created_by"),  nullptr
   };

   DB_STATEMENT hStmt = DBPrepareMerge(hdb, _T("user_agent_notifications"), _T("id"), m_id, columns);
   if (hStmt != nullptr)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, m_message, DB_BIND_STATIC, MAX_USER_AGENT_MESSAGE_SIZE - 1);

      StringBuffer buffer;
      for(int i = 0; i < m_objects.size(); i++)
      {
         if (buffer.length() > 0)
            buffer.append(_T(','));
         buffer.append(m_objects.get(i));
      }
      DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, buffer, DB_BIND_STATIC, MAX_USER_AGENT_MESSAGE_SIZE - 1);
      DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(m_startTime));
      DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(m_endTime));
      DBBind(hStmt, 5, DB_SQLTYPE_VARCHAR, (m_recall ? _T("1") : _T("0")), DB_BIND_STATIC);
      DBBind(hStmt, 6, DB_SQLTYPE_VARCHAR, (m_onStartup ? _T("1") : _T("0")), DB_BIND_STATIC);
      DBBind(hStmt, 7, DB_SQLTYPE_INTEGER, static_cast<uint32_t>(m_creationTime));
      DBBind(hStmt, 8, DB_SQLTYPE_INTEGER, m_creatorId);
      DBBind(hStmt, 9, DB_SQLTYPE_INTEGER, m_id);

      DBExecute(hStmt);
      DBFreeStatement(hStmt);
   }

   DBConnectionPoolReleaseConnection(hdb);
}

/**
 * Serialize object to json
 */
json_t *UserAgentNotificationItem::toJson() const
{
   json_t *root = json_object();
   json_object_set_new(root, "id", json_integer(m_id));
   json_object_set_new(root, "message", json_string_t(m_message));
   json_object_set_new(root, "objectList", m_objects.toJson());
   json_object_set_new(root, "startTime", json_integer(m_startTime));
   json_object_set_new(root, "endTime", json_integer(m_endTime));
   json_object_set_new(root, "recalled", json_boolean(m_recall));
   json_object_set_new(root, "creationTime", json_boolean(m_creationTime));
   json_object_set_new(root, "createdBy", json_boolean(m_creatorId));
   return root;
}
