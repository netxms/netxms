/*
** NetXMS - Network Management System
** Client Library
** Copyright (C) 2003-2022 Victor Kirhenshtein
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
EventController::EventController(NXCSession *session) : Controller(session), m_eventTemplateLock(MutexType::FAST)
{
   m_eventTemplates = nullptr;
}

/**
 * Controller destructor
 */
EventController::~EventController()
{
   delete m_eventTemplates;
}

/**
 * Synchronize event templates
 */
uint32_t EventController::syncEventTemplates()
{
   ObjectArray<EventTemplate> *list = new ObjectArray<EventTemplate>(256, 256, Ownership::True);
   uint32_t rcc = getEventTemplates(list);
   if (rcc != RCC_SUCCESS)
   {
      delete list;
      return rcc;
   }

   m_eventTemplateLock.lock();
   delete m_eventTemplates;
   m_eventTemplates = list;
   m_eventTemplateLock.unlock();
   return RCC_SUCCESS;
}

/**
 * Get configured event templates
 */
uint32_t EventController::getEventTemplates(ObjectArray<EventTemplate> *templates)
{
   NXCPMessage msg(CMD_LOAD_EVENT_DB, m_session->createMessageId());

   if (!m_session->sendMessage(&msg))
      return RCC_COMM_FAILURE;

   NXCPMessage *response = m_session->waitForMessage(CMD_REQUEST_COMPLETED, msg.getId());
   if (response == nullptr)
      return RCC_TIMEOUT;

   uint32_t rcc = response->getFieldAsUInt32(VID_RCC);
   if (rcc != RCC_SUCCESS)
   {
      delete response;
      return rcc;
   }

   int count = response->getFieldAsInt32(VID_NUM_EVENTS);
   uint32_t fieldId = VID_ELEMENT_LIST_BASE;
   for(int i = 0; i < count; i++)
   {
      templates->add(new EventTemplate(*response, fieldId));
      fieldId += 10;
   }
   delete response;
   return RCC_SUCCESS;
}

/**
 * Get event name by code
 */
TCHAR *EventController::getEventName(uint32_t code, TCHAR *buffer, size_t bufferSize)
{
   m_eventTemplateLock.lock();
   if (m_eventTemplates != nullptr)
   {
      for(int i = 0; i < m_eventTemplates->size(); i++)
      {
         EventTemplate *t = m_eventTemplates->get(i);
         if (t->getCode() == code)
         {
            _tcslcpy(buffer, t->getName(), bufferSize);
            m_eventTemplateLock.unlock();
            return buffer;
         }
      }
   }
   m_eventTemplateLock.unlock();
   _tcslcpy(buffer, _T("<unknown>"), bufferSize);
   return buffer;
}

/**
 * Send event to server
 */
uint32_t EventController::sendEvent(uint32_t code, const TCHAR *name, uint32_t objectId, int argc, TCHAR **argv, const TCHAR *userTag, bool useNamedParameters)
{
   NXCPMessage msg(CMD_TRAP, m_session->createMessageId());
   msg.setField(VID_EVENT_CODE, code);
   msg.setField(VID_EVENT_NAME, name);
   msg.setField(VID_OBJECT_ID, objectId);
	msg.setField(VID_USER_TAG, CHECK_NULL_EX(userTag));
   msg.setField(VID_NUM_ARGS, (UINT16)argc);

   for(int i = 0; i < argc; i++)
   {
      if (useNamedParameters)
      {
         TCHAR *separator = _tcschr(argv[i], '=');
         if (separator != nullptr)
         {
            *separator = 0;
            msg.setField(VID_EVENT_ARG_BASE + i, separator + 1);
            msg.setField(VID_EVENT_ARG_NAMES_BASE + i, argv[i]);
         }
         else
         {
            TCHAR buffer[32];
            _sntprintf(buffer, 32, _T("parameter%d"), i + 1);
            msg.setField(VID_EVENT_ARG_BASE + i, argv[i]);
            msg.setField(VID_EVENT_ARG_NAMES_BASE + i, buffer);
         }
      }
      else
      {
         msg.setField(VID_EVENT_ARG_BASE + i, argv[i]);
      }
   }

   if (!m_session->sendMessage(&msg))
      return RCC_COMM_FAILURE;
   return m_session->waitForRCC(msg.getId());
}

/**
 * Event template constructor
 */
EventTemplate::EventTemplate(const NXCPMessage& msg, uint32_t baseId)
{
   m_code = msg.getFieldAsUInt32(baseId + 1);
   m_description = msg.getFieldAsString(baseId + 2);
   msg.getFieldAsString(baseId + 3, m_name, MAX_EVENT_NAME);
   m_severity = msg.getFieldAsInt32(baseId + 4);
   m_flags = msg.getFieldAsUInt32(baseId + 5);
   m_messageTemplate = msg.getFieldAsString(baseId + 6);
   m_tags = msg.getFieldAsString(baseId + 7);
   m_guid = msg.getFieldAsGUID(baseId + 8);
}

/**
 * Event template destructor
 */
EventTemplate::~EventTemplate()
{
   MemFree(m_messageTemplate);
   MemFree(m_description);
   MemFree(m_tags);
}
