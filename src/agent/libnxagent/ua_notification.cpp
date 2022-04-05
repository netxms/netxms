/*
** NetXMS - Network Management System
** Copyright (C) 2003-2020 Raden Solutions
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published
** by the Free Software Foundation; either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: user_notification.cpp
**
**/

#include "libnxagent.h"

/**
 * User support application notification constructor
 */
UserAgentNotification::UserAgentNotification(uint64_t serverId, const NXCPMessage *msg, uint32_t baseId)
      : m_id(serverId, msg->getFieldAsUInt32(baseId))
{
   m_message = msg->getFieldAsString(baseId + 1);
   m_startTime = msg->getFieldAsTime(baseId + 2);
   m_endTime = msg->getFieldAsTime(baseId + 3);
   m_startup = msg->getFieldAsBoolean(baseId + 4);
   m_read = false;
}

/**
 * User support application notification constructor
 */
UserAgentNotification::UserAgentNotification(const NXCPMessage *msg, uint32_t baseId)
      : m_id(msg->getFieldAsUInt64(baseId + 9), msg->getFieldAsUInt32(baseId))
{
   m_message = msg->getFieldAsString(baseId + 1);
   m_startTime = msg->getFieldAsTime(baseId + 2);
   m_endTime = msg->getFieldAsTime(baseId + 3);
   m_startup = msg->getFieldAsBoolean(baseId + 4);
   m_read = false;
}

/**
 * User support application notification constructor
 */
UserAgentNotification::UserAgentNotification(uint64_t serverId, uint32_t notificationId, TCHAR *message, time_t start, time_t end, bool startup)
      : m_id(serverId, notificationId)
{
   m_message = message;
   m_startTime = start;
   m_endTime = end;
   m_startup = startup;
   m_read = false;
}

/**
 * Copy constructor
 */
UserAgentNotification::UserAgentNotification(const UserAgentNotification *src) : m_id(src->m_id)
{
   m_message = MemCopyString(src->m_message);
   m_startTime = src->m_startTime;
   m_endTime = src->m_endTime;
   m_startup = src->m_startup;
   m_read = src->m_read;
}

/**
 * User support application notification destructor
 */
UserAgentNotification::~UserAgentNotification()
{
   MemFree(m_message);
}

/**
 * Prepare NXCP message with user support application notification info
 */
void UserAgentNotification::fillMessage(NXCPMessage *msg, uint32_t baseId)
{
   msg->setField(baseId, m_id.objectId);
   msg->setField(baseId + 1, m_message);
   msg->setFieldFromTime(baseId + 2, m_startTime);
   msg->setFieldFromTime(baseId + 3, m_endTime);
   msg->setFieldFromTime(baseId + 4, m_startup);
   msg->setField(baseId + 9, m_id.serverId);
}

/**
 * Save notification object to agent database
 */
void UserAgentNotification::saveToDatabase(DB_HANDLE db)
{
   TCHAR query[2048];
   _sntprintf(query, 2048,
         _T("INSERT INTO user_agent_notifications (server_id,notification_id,message,start_time,end_time,on_startup)")
         _T(" VALUES (") INT64_FMT _T(",%u,%s,%u,%u,%s)"), m_id.serverId, m_id.objectId,
         (const TCHAR *)DBPrepareString(db, m_message, 1023), static_cast<unsigned int>(m_startTime),
         static_cast<unsigned int>(m_endTime), m_startup ? _T("1") : _T("0"));
   DBQuery(db, query);
}
