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
** File: uamessages.cpp
**
**/

#include "nxagentd.h"

#define MAX_USER_AGENT_MESSAGE_SIZE 1024

struct UserAgentMessage
{
   UINT32 m_id;
   TCHAR m_message[MAX_USER_AGENT_MESSAGE_SIZE];
};

Mutex g_userAgentMessageListMutex;
ObjectArray<UserAgentMessage> *g_userAgentMessageList = new ObjectArray<UserAgentMessage>(true);

/**
 * Notify on new user agent message on on message recall
 */
UINT32 AddUserAgentMessageChange(NXCPMessage *request)
{
   UserAgentMessage *uam = new UserAgentMessage();
   uam->m_id = request->getFieldAsUInt32(VID_USER_AGENT_MESSAGE_BASE);
   request->getFieldAsString(VID_USER_AGENT_MESSAGE_BASE+1, uam->m_message, MAX_USER_AGENT_MESSAGE_SIZE);
   g_userAgentMessageListMutex.lock();
   g_userAgentMessageList->add(uam);
   g_userAgentMessageListMutex.unlock();

   //TODO: Notify user agents about new message

   return RCC_SUCCESS;
}

/**
 * Notify on user agent message recall
 */
UINT32 RemoveUserAgentMessageChange(NXCPMessage *request)
{
   UINT32 id = request->getFieldAsUInt32(VID_USER_AGENT_MESSAGE_BASE);
   g_userAgentMessageListMutex.lock();
   Iterator<UserAgentMessage> *it = g_userAgentMessageList->iterator();
   while (it->hasNext())
   {
      UserAgentMessage *uam = it->next();
      if(uam->m_id == id)
         it->remove();
   }
   g_userAgentMessageListMutex.unlock();

   //TODO: Notify user agents about removed message

   return RCC_SUCCESS;
}

/**
 *
 */
UINT32 UpdateUserAgentMessageList(NXCPMessage *request)
{
   ObjectArray<UserAgentMessage> *tmp = new ObjectArray<UserAgentMessage>(true);
   int count = request->getFieldAsInt32(VID_USER_AGENT_MESSAGE_COUNT);
   int base = VID_USER_AGENT_MESSAGE_BASE;
   for (int i = 0; i < count; i++, base+=10)
   {
      UserAgentMessage *uam = new UserAgentMessage();
      uam->m_id = request->getFieldAsUInt32(base);
      request->getFieldAsString(base+1, uam->m_message, MAX_USER_AGENT_MESSAGE_SIZE);
      tmp->add(uam);
   }
   g_userAgentMessageListMutex.lock();
   delete g_userAgentMessageList;
   g_userAgentMessageList = tmp;
   g_userAgentMessageListMutex.unlock();

   //TODO: Notify user agents messages change

   return RCC_SUCCESS;
}
