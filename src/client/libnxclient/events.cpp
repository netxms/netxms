/*
** NetXMS - Network Management System
** Client Library
** Copyright (C) 2003-2014 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation; either version 3 of the License, or
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
** File: events.cpp
**
**/

#include "libnxclient.h"

/**
 * Controller constructor
 */
EventController::EventController(NXCSession *session) : Controller(session)
{
   m_eventTemplates = NULL;
   m_eventTemplateLock = MutexCreate();
}

/**
 * Controller destructor
 */
EventController::~EventController()
{
   delete m_eventTemplates;
   MutexDestroy(m_eventTemplateLock);
}

/**
 * Synchronize event templates
 */
UINT32 EventController::syncEventTemplates()
{
   ObjectArray<EventTemplate> *list = new ObjectArray<EventTemplate>(256, 256, true);
   UINT32 rcc = getEventTemplates(list);
   if (rcc != RCC_SUCCESS)
   {
      delete list;
      return rcc;
   }

   MutexLock(m_eventTemplateLock);
   delete m_eventTemplates;
   m_eventTemplates = list;
   MutexUnlock(m_eventTemplateLock);
   return RCC_SUCCESS;
}

/**
 * Get configured event templates
 */
UINT32 EventController::getEventTemplates(ObjectArray<EventTemplate> *templates)
{
   NXCPMessage msg;
   msg.setCode(CMD_LOAD_EVENT_DB);
   msg.setId(m_session->createMessageId());

   if (!m_session->sendMessage(&msg))
      return RCC_COMM_FAILURE;

   UINT32 rcc = m_session->waitForRCC(msg.getId());
   if (rcc != RCC_SUCCESS)
      return rcc;

   while(true)
   {
      NXCPMessage *response = m_session->waitForMessage(CMD_EVENT_DB_RECORD, msg.getId());
      if (response != NULL)
      {
         if (response->isEndOfSequence())
         {
            delete response;
            break;
         }
         templates->add(new EventTemplate(response));
         delete response;
      }
      else
      {
         rcc = RCC_TIMEOUT;
         break;
      }
   }
   return rcc;
}

/**
 * Get event name by code
 */
TCHAR *EventController::getEventName(UINT32 code, TCHAR *buffer, size_t bufferSize)
{
   MutexLock(m_eventTemplateLock);
   if (m_eventTemplates != NULL)
   {
      for(int i = 0; i < m_eventTemplates->size(); i++)
      {
         EventTemplate *t = m_eventTemplates->get(i);
         if (t->getCode() == code)
         {
            nx_strncpy(buffer, t->getName(), bufferSize);
            MutexUnlock(m_eventTemplateLock);
            return buffer;
         }
      }
   }
   MutexUnlock(m_eventTemplateLock);
   nx_strncpy(buffer, _T("<unknown>"), bufferSize);
   return buffer;
}

/**
 * Send event to server
 */
UINT32 EventController::sendEvent(UINT32 code, const TCHAR *name, UINT32 objectId, int argc, TCHAR **argv, const TCHAR *userTag)
{
   NXCPMessage msg;

   msg.setCode(CMD_TRAP);
   msg.setId(m_session->createMessageId());
   msg.setField(VID_EVENT_CODE, code);
   msg.setField(VID_EVENT_NAME, name);
   msg.setField(VID_OBJECT_ID, objectId);
	msg.setField(VID_USER_TAG, CHECK_NULL_EX(userTag));
   msg.setField(VID_NUM_ARGS, (UINT16)argc);
   for(int i = 0; i < argc; i++)
      msg.setField(VID_EVENT_ARG_BASE + i, argv[i]);

   if (!m_session->sendMessage(&msg))
      return RCC_COMM_FAILURE;
   return m_session->waitForRCC(msg.getId());
}

/**
 * Event template constructor
 */
EventTemplate::EventTemplate(NXCPMessage *msg)
{
   m_code = msg->getFieldAsUInt32(VID_EVENT_CODE);
   msg->getFieldAsString(VID_NAME, m_name, MAX_EVENT_NAME);
   m_severity = msg->getFieldAsInt32(VID_SEVERITY);
   m_flags = msg->getFieldAsUInt32(VID_FLAGS);
   m_messageTemplate = msg->getFieldAsString(VID_MESSAGE);
   m_description = msg->getFieldAsString(VID_DESCRIPTION);
}

/**
 * Event template destructor
 */
EventTemplate::~EventTemplate()
{
   safe_free(m_messageTemplate);
   safe_free(m_description);
}
