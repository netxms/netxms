/*
** NetXMS - Network Management System
** Copyright (C) 2019 Raden Solutions
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


Mutex g_userAgentNotificationListMutex;
ObjectArray<UserAgentNotificationItem> g_userAgentNotificationList(0, 16, true);

/**
 * Init user agent messages
 */
void InitUserAgentNotifications()
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   DB_RESULT hResult = DBSelect(hdb, _T("SELECT id,message,objects,start_time,end_time,recall FROM user_agent_notifications"));
   if (hResult != NULL)
   {
      int count = DBGetNumRows(hResult);
      for(int i = 0; i < count; i++)
      {
         g_userAgentNotificationList.add(new UserAgentNotificationItem(hResult, i));
      }
      DBFreeResult(hResult);
   }
   DBConnectionPoolReleaseConnection(hdb);
   DbgPrintf(2, _T("%d user agent messages loaded"), g_userAgentNotificationList.size());
}

void DeleteOldUserAgentNotifications(DB_HANDLE hdb, UINT32 retentionTime)
{
   g_userAgentNotificationListMutex.lock();
   time_t now = time(NULL);
   Iterator<UserAgentNotificationItem> *it = g_userAgentNotificationList.iterator();
   while (it->hasNext())
   {
      UserAgentNotificationItem *uan = it->next();
      if((now - uan->getEndTime()) >= retentionTime && uan->hasNoRef())
      {
         it->remove();
         break;
      }
   }
   delete it;
   g_userAgentNotificationListMutex.unlock();

   TCHAR query[256];
   _sntprintf(query, sizeof(query) / sizeof(TCHAR), _T("DELETE FROM user_agent_notifications WHERE end_time>=") INT64_FMT,
         static_cast<INT64>(now - retentionTime));
   DBQuery(hdb, query);
}

void FillUserAgentNotificationsAll(NXCPMessage *msg, Node *node)
{
   g_userAgentNotificationListMutex.lock();
   int base = VID_UA_NOTIFICATION_BASE;
   time_t now = time(NULL);
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

UserAgentNotificationItem *CreateNewUserAgentNotification(const TCHAR *message, IntegerArray<UINT32> *arr, time_t startTime, time_t endTime)
{
   g_userAgentNotificationListMutex.lock();
   UserAgentNotificationItem *uan = new UserAgentNotificationItem(message, arr, startTime, endTime);
   g_userAgentNotificationList.add(uan);
   uan->incRefCount();
   uan->incRefCount();
   g_userAgentNotificationListMutex.unlock();

   ThreadPoolExecute(g_clientThreadPool, uan, &UserAgentNotificationItem::processUpdate);
   NotifyClientSessions(NX_NOTIFY_USER_AGENT_MESSAGE_CHANGED, uan->getId());
   return uan;
}

/**
 * Create user agent message form database
 */
UserAgentNotificationItem::UserAgentNotificationItem(DB_RESULT result, int row)
{
   m_id = DBGetFieldULong(result, row, 0);
   DBGetField(result, row, 1, m_message, MAX_USER_AGENT_MESSAGE_SIZE);

   TCHAR objectList[MAX_DB_STRING];
   DBGetField(result, row, 2, objectList, MAX_DB_STRING);
   m_objectList = new IntegerArray<UINT32>(16, 16);
   int count;
   TCHAR **ids = SplitString(objectList, _T(','), &count);
   for(int i = 0; i < count; i++)
   {
      m_objectList->add(_tcstoul(ids[i], NULL, 10));
      free(ids[i]);
   }
   free(ids);

   m_startTime = DBGetFieldLong(result, row, 3);
   m_endTime = DBGetFieldLong(result, row, 4);
   m_recall = DBGetFieldLong(result, row, 5) ? true : false;
   m_refCount = 0;
}

/**
 * Create new user agent message
 */
UserAgentNotificationItem::UserAgentNotificationItem(const TCHAR *message, IntegerArray<UINT32> *objectList, time_t startTime, time_t endTime)
{
   m_id = CreateUniqueId(IDG_UA_MESSAGE);
   _tcslcpy(m_message, message, MAX_USER_AGENT_MESSAGE_SIZE);
   m_objectList = objectList;
   m_startTime = startTime;
   m_endTime = endTime;
   m_recall = false;
   m_refCount = 0;
}

/**
 * Check if user agent message is applicable on the node
 */
bool UserAgentNotificationItem::isApplicable(UINT32 nodeId)
{
   for (int i = 0; i < m_objectList->size(); i++)
   {
      UINT32 id = m_objectList->get(i);
      if(id == nodeId || IsParentObject(id, nodeId))
         return true;
   }
   return false;
}

/**
 * Check if object is node - then send update, if container - iterate over children
 */
static void SendUpdate(NXCPMessage *msg, NetObj *obj)
{
   if (obj->getObjectClass() == OBJECT_NODE)
   {
      AgentConnectionEx *conn = static_cast<Node *>(obj)->getAgentConnection(false);
      if(conn != NULL)
      {
         msg->setId(conn->generateRequestId());
         conn->sendMessage(msg);
      }
   }
   else
   {
      ObjectArray<NetObj> *list = obj->getChildList(OBJECT_NODE);
      for(int j = 0; j < list->size(); j++)
      {
         NetObj *obj = list->get(j);
         SendUpdate(msg, obj);
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
   for (int i = 0; i < m_objectList->size(); i++)
   {
      NetObj *obj = FindObjectById(m_objectList->get(i));
      SendUpdate(&msg, obj);
   }

   saveToDatabase();
   decRefCount();
}

/**
 * Fill message with user agent message data
 */
void UserAgentNotificationItem::fillMessage(UINT32 base, NXCPMessage *msg, bool fullInfo)
{
   msg->setField(base, m_id);
   msg->setField(base + 1, m_message, MAX_USER_AGENT_MESSAGE_SIZE);
   msg->setFieldFromTime(base + 2, m_startTime);
   msg->setFieldFromTime(base + 3, m_endTime);
   if (fullInfo) // do not send info to agent
   {
      msg->setFieldFromInt32Array(base + 4, m_objectList);
      msg->setField(base + 5, m_recall);
   }
}

/**
 * Save object to database
 */
void UserAgentNotificationItem::saveToDatabase()
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();

   static const TCHAR *columns[] = {
            _T("message"), _T("objects"), _T("start_time"), _T("end_time"), _T("recall"), NULL
         };

   DB_STATEMENT hStmt = DBPrepareMerge(hdb, _T("user_agent_notifications"), _T("id"), m_id, columns);
   if (hStmt != NULL)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, m_message, MAX_USER_AGENT_MESSAGE_SIZE);

      String buffer;
      for(int i = 0; i < m_objectList->size(); i++)
      {
         if (buffer.length() > 0)
            buffer.append(_T(','));
         buffer.append(m_objectList->get(i));
      }
      DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, buffer, MAX_USER_AGENT_MESSAGE_SIZE);
      DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, (UINT32)m_startTime);
      DBBind(hStmt, 4, DB_SQLTYPE_INTEGER, (UINT32)m_endTime);
      DBBind(hStmt, 5, DB_SQLTYPE_VARCHAR, (m_recall ? _T("1") : _T("0")), DB_BIND_STATIC);
      DBBind(hStmt, 6, DB_SQLTYPE_INTEGER, m_id);

      DBExecute(hStmt);
      DBFreeStatement(hStmt);
   }

   DBConnectionPoolReleaseConnection(hdb);
}

/**
 * Serealize object to json
 */
json_t *UserAgentNotificationItem::toJson() const
{
   json_t *root = json_object();
   json_object_set_new(root, "id", json_integer(m_id));
   json_object_set_new(root, "message", json_string_t(m_message));
   json_object_set_new(root, "objectList", m_objectList->toJson());
   json_object_set_new(root, "startTime", json_integer(m_startTime));
   json_object_set_new(root, "endTime", json_integer(m_endTime));
   json_object_set_new(root, "recalled", json_boolean(m_recall));
   return root;

}
