/* 
** NetXMS multiplatform core agent
** Copyright (C) 2003-2024 Raden Solutions
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
** File: event.cpp
**
**/

#include "nxagentd.h"
#include <stdarg.h>

#define DEBUG_TAG _T("events")

/**
 * Static data
 */
static uint64_t s_generatedEventsCount = 0;
static uint64_t s_eventIdBase = static_cast<uint64_t>(time(nullptr)) << 32;
static VolatileCounter s_eventIdCounter = 0;
static time_t s_lastEventTimestamp = 0;

/**
 * Send event to server
 */
void PostEvent(uint32_t eventCode, const TCHAR *eventName, time_t timestamp)
{
   if (nxlog_get_debug_level() >= 5)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("PostEvent(): event_code=%d, event_name=%s, timestamp=") INT64_FMT,
         eventCode, CHECK_NULL(eventName), static_cast<INT64>(timestamp));
   }

   NXCPMessage *msg = new NXCPMessage(CMD_TRAP, 0, 4); // Use version 4
   msg->setField(VID_TRAP_ID, s_eventIdBase | static_cast<uint64_t>(InterlockedIncrement(&s_eventIdCounter)));
   msg->setField(VID_EVENT_CODE, eventCode);
   if (eventName != nullptr)
      msg->setField(VID_EVENT_NAME, eventName);
   msg->setFieldFromTime(VID_TIMESTAMP, (timestamp != 0) ? timestamp : time(nullptr));

   s_generatedEventsCount++;
   s_lastEventTimestamp = time(nullptr);

   g_notificationProcessorQueue.put(msg);
}

/**
 * Send event - variant 2
 * Arguments:
 * eventCode   - Event code
 * eventName   - event name; to send event by name, eventCode must be set to 0
 * timestamp   - event timestamp
 * parameters  - event parameters
 */
void PostEvent(uint32_t eventCode, const TCHAR *eventName, time_t timestamp, const StringMap &parameters)
{
   if (nxlog_get_debug_level() >= 5)
   {
      StringBuffer argsText;
      int i = 0;
      for(KeyValuePair<const TCHAR> *pair : parameters)
      {
         argsText.append(_T(", arg["));
         argsText.append(i++);
         argsText.append(_T("]=\""));
         argsText.append(CHECK_NULL(pair->key));
         argsText.append(_T(":"));
         argsText.append(CHECK_NULL(pair->value));
         argsText.append(_T('"'));
      }
      nxlog_debug_tag(DEBUG_TAG, 5, _T("PostEvent(): event_code=%d, event_name=%s, timestamp=") INT64_FMT _T(", num_args=%d%s"),
                  eventCode, CHECK_NULL(eventName), static_cast<INT64>(timestamp), parameters.size(), (const TCHAR *)argsText);
   }

   NXCPMessage *msg = new NXCPMessage(CMD_TRAP, 0, 4); // Use version 4
   msg->setField(VID_TRAP_ID, s_eventIdBase | static_cast<uint64_t>(InterlockedIncrement(&s_eventIdCounter)));
   msg->setField(VID_EVENT_CODE, eventCode);
   if (eventName != nullptr)
      msg->setField(VID_EVENT_NAME, eventName);
   msg->setFieldFromTime(VID_TIMESTAMP, (timestamp != 0) ? timestamp : time(nullptr));
   msg->setField(VID_NUM_ARGS, static_cast<uint16_t>(parameters.size()));
   uint32_t paramBase = VID_EVENT_ARG_BASE;
   uint32_t nameBase = VID_EVENT_ARG_NAMES_BASE;
   for(KeyValuePair<const TCHAR> *pair : parameters)
   {
      msg->setField(nameBase++, pair->key);
      msg->setField(paramBase++, pair->value);
   }

   s_generatedEventsCount++;
   s_lastEventTimestamp = time(nullptr);

   g_notificationProcessorQueue.put(msg);
}

/**
 * Forward event from external subagent to server
 */
void ForwardEvent(NXCPMessage *msg)
{
	msg->setField(VID_TRAP_ID, s_eventIdBase | static_cast<uint64_t>(InterlockedIncrement(&s_eventIdCounter)));
	s_generatedEventsCount++;
	s_lastEventTimestamp = time(nullptr);
	g_notificationProcessorQueue.put(msg);
}

/**
 * Process push request
 */
static void ProcessEventSendRequest(NamedPipe *pipe, void *arg)
{
   TCHAR buffer[256];

   nxlog_debug_tag(DEBUG_TAG, 5, _T("ProcessEventSendRequest: connection established"));
   PipeMessageReceiver receiver(pipe->handle(), 8192, 1048576);  // 8K initial, 1M max
   while(true)
   {
      MessageReceiverResult result;
      NXCPMessage *msg = receiver.readMessage(5000, &result);
      if (msg == nullptr)
         break;
      nxlog_debug_tag(DEBUG_TAG, 6, _T("ProcessEventSendRequest: received message %s"), NXCPMessageCodeName(msg->getCode(), buffer));
      if (msg->getCode() == CMD_TRAP)
         ForwardEvent(msg);
      else
         delete msg;
   }
   nxlog_debug_tag(DEBUG_TAG, 5, _T("ProcessEventSendRequest: connection by user %s closed"), pipe->user());
}

/**
 * Pipe listener for event connector
 */
static NamedPipeListener *s_listener;

/**
 * Start event connector
 */
void StartEventConnector()
{
   shared_ptr<Config> config = g_config;
   const TCHAR *user = config->getValue(_T("/%agent/EventUser"), _T("*"));
   s_listener = NamedPipeListener::create(_T("nxagentd.events"), ProcessEventSendRequest, nullptr,
            (user != nullptr) && (user[0] != 0) && _tcscmp(user, _T("*")) ? user : nullptr);
   if (s_listener != nullptr)
      s_listener->start();
}

/**
 * Handler for event statistic DCIs
 */
LONG H_AgentEventSender(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
	switch(arg[0])
	{
		case 'G':
			ret_uint64(value, s_generatedEventsCount);
			break;
		case 'T':
			ret_uint64(value, static_cast<UINT64>(s_lastEventTimestamp));
			break;
		default:
			return SYSINFO_RC_UNSUPPORTED;
	}
	return SYSINFO_RC_SUCCESS;
}
