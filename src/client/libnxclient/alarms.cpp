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
** File: alarms.cpp
**
**/

#include "libnxclient.h"

/**
 * Create alarm comment from NXCP message
 */
AlarmComment::AlarmComment(NXCPMessage *msg, UINT32 baseId)
{
   m_id = msg->getFieldAsUInt32(baseId);
   m_alarmId = msg->getFieldAsUInt32(baseId + 1);
   m_timestamp = (time_t)msg->getFieldAsUInt32(baseId + 2);
   m_userId = msg->getFieldAsUInt32(baseId + 3);
   m_text = msg->getFieldAsString(baseId + 4);
   if (m_text == NULL)
      m_text = _tcsdup(_T(""));
   m_userName = msg->getFieldAsString(baseId + 5);
   if (m_userName == NULL)
   {
      m_userName = (TCHAR *)malloc(32 * sizeof(TCHAR));
      _sntprintf(m_userName, 32, _T("[%u]"), m_userId);
   }
}

/**
 * Destructor for alarm comment
 */
AlarmComment::~AlarmComment()
{
   free(m_text);
}

/**
 * Acknowledge alarm
 */
UINT32 AlarmController::acknowledge(UINT32 alarmId, bool sticky, UINT32 timeout)
{
   NXCPMessage msg;
   msg.setId(m_session->createMessageId());
   msg.setCode(CMD_ACK_ALARM);
   msg.setField(VID_ALARM_ID, alarmId);
   msg.setField(VID_STICKY_FLAG, (UINT16)(sticky ? 1 : 0));
   msg.setField(VID_TIMESTAMP, timeout);

   if (!m_session->sendMessage(&msg))
      return RCC_COMM_FAILURE;

   return m_session->waitForRCC(msg.getId());
}

/**
 * Resolve alarm
 */
UINT32 AlarmController::resolve(UINT32 alarmId)
{
   NXCPMessage msg;
   msg.setId(m_session->createMessageId());
   msg.setCode(CMD_RESOLVE_ALARM);
   msg.setField(VID_ALARM_ID, alarmId);

   if (!m_session->sendMessage(&msg))
      return RCC_COMM_FAILURE;

   return m_session->waitForRCC(msg.getId());
}

/**
 * Terminate alarm
 */
UINT32 AlarmController::terminate(UINT32 alarmId)
{
   NXCPMessage msg;
   msg.setId(m_session->createMessageId());
   msg.setCode(CMD_TERMINATE_ALARM);
   msg.setField(VID_ALARM_ID, alarmId);

   if (!m_session->sendMessage(&msg))
      return RCC_COMM_FAILURE;

   return m_session->waitForRCC(msg.getId());
}

/**
 * Open helpdesk issue
 *
 * @param aarmId alarm ID
 * @param helpdeskRef buffer for helpdesk reference. Must be at least MAX_HELPDESK_REF_LEN characters long.
 */
UINT32 AlarmController::openHelpdeskIssue(UINT32 alarmId, TCHAR *helpdeskRef)
{
   NXCPMessage msg;

   msg.setCode(CMD_OPEN_HELPDESK_ISSUE);
   msg.setId(m_session->createMessageId());
   msg.setField(VID_ALARM_ID, alarmId);
   if (!m_session->sendMessage(&msg))
      return RCC_COMM_FAILURE;

   UINT32 rcc;
   NXCPMessage *response = m_session->waitForMessage(CMD_REQUEST_COMPLETED, msg.getId());
   if (response != NULL)
   {
      rcc = response->getFieldAsUInt32(VID_RCC);
      if (rcc == RCC_SUCCESS)
      {
         helpdeskRef[0] = 0;
         response->getFieldAsString(VID_HELPDESK_REF, helpdeskRef, MAX_HELPDESK_REF_LEN);
      }
      delete response;
   }
   else
   {
      rcc = RCC_TIMEOUT;
   }
   return rcc;
}

/**
 * Update alarm comment
 */
UINT32 AlarmController::updateComment(UINT32 alarmId, UINT32 commentId, const TCHAR *text)
{
   NXCPMessage msg;

   msg.setCode(CMD_UPDATE_ALARM_COMMENT);
   msg.setId(m_session->createMessageId());
   msg.setField(VID_ALARM_ID, alarmId);
   msg.setField(VID_COMMENT_ID, commentId);
   msg.setField(VID_COMMENTS, text);
   if (!m_session->sendMessage(&msg))
      return RCC_COMM_FAILURE;

   return m_session->waitForRCC(msg.getId());
}

/**
 * Add alarm comment
 */
UINT32 AlarmController::addComment(UINT32 alarmId, const TCHAR *text)
{
   return updateComment(alarmId, 0, text);
}

/**
 * Get alarm comments
 *
 * Comments array must be deleted by the caller.
 */
UINT32 AlarmController::getComments(UINT32 alarmId, ObjectArray<AlarmComment> **comments)
{
   NXCPMessage msg;

   *comments = NULL;

   msg.setCode(CMD_GET_ALARM_COMMENTS);
   msg.setId(m_session->createMessageId());
   msg.setField(VID_ALARM_ID, alarmId);
   if (!m_session->sendMessage(&msg))
      return RCC_COMM_FAILURE;

   UINT32 rcc;
   NXCPMessage *response = m_session->waitForMessage(CMD_REQUEST_COMPLETED, msg.getId());
   if (response != NULL)
   {
      rcc = response->getFieldAsUInt32(VID_RCC);
      if (rcc == RCC_SUCCESS)
      {
         int count = response->getFieldAsInt32(VID_NUM_ELEMENTS);
         ObjectArray<AlarmComment> *list = new ObjectArray<AlarmComment>(count, 16, true);
         UINT32 fieldId = VID_ELEMENT_LIST_BASE;
         for(int i = 0; i < count; i++)
         {
            list->add(new AlarmComment(response, fieldId));
            fieldId += 10;
         }
         *comments = list;
      }
      delete response;
   }
   else
   {
      rcc = RCC_TIMEOUT;
   }
   return rcc;
}

/**
 * Load all alarms from server. Returned array must be destroyed by caller.
 */
UINT32 AlarmController::getAll(ObjectArray<NXC_ALARM> **alarms)
{
   *alarms = NULL;

   NXCPMessage msg;
   msg.setCode(CMD_GET_ALL_ALARMS);
   msg.setId(m_session->createMessageId());
   if (!m_session->sendMessage(&msg))
      return RCC_COMM_FAILURE;

   ObjectArray<NXC_ALARM> *list = new ObjectArray<NXC_ALARM>(256, 256, true);
   UINT32 alarmId;
   UINT32 rcc = RCC_SUCCESS;
   do
   {
      NXCPMessage *response = m_session->waitForMessage(CMD_ALARM_DATA, msg.getId());
      if (response != NULL)
      {
         alarmId = response->getFieldAsUInt32(VID_ALARM_ID);
         if (alarmId != 0)  // 0 is end of list indicator
         {
            NXC_ALARM *alarm = createAlarmFromMessage(response);
            list->add(alarm);
         }
         delete response;
      }
      else
      {
         rcc = RCC_TIMEOUT;
         alarmId = 0;
      }
   }
   while(alarmId != 0);

   // Destroy results on failure or save on success
   if (rcc == RCC_SUCCESS)
   {
      *alarms = list;
   }
   else
   {
      delete list;
   }

   return rcc;
}

/**
 * Create alarm record from NXCP message
 */
NXC_ALARM *AlarmController::createAlarmFromMessage(NXCPMessage *msg)
{
   NXC_ALARM *alarm = new NXC_ALARM();
   alarm->alarmId = msg->getFieldAsUInt32(VID_ALARM_ID);
   alarm->ackByUser = msg->getFieldAsUInt32(VID_ACK_BY_USER);
   alarm->resolvedByUser = msg->getFieldAsUInt32(VID_RESOLVED_BY_USER);
   alarm->termByUser = msg->getFieldAsUInt32(VID_TERMINATED_BY_USER);
   alarm->sourceEventId = msg->getFieldAsUInt64(VID_EVENT_ID);
   alarm->sourceEventCode = msg->getFieldAsUInt32(VID_EVENT_CODE);
   alarm->sourceObject = msg->getFieldAsUInt32(VID_OBJECT_ID);
   alarm->dciId = msg->getFieldAsUInt32(VID_DCI_ID);
   alarm->creationTime = msg->getFieldAsUInt32(VID_CREATION_TIME);
   alarm->lastChangeTime = msg->getFieldAsUInt32(VID_LAST_CHANGE_TIME);
   msg->getFieldAsString(VID_ALARM_KEY, alarm->key, MAX_DB_STRING);
	msg->getFieldAsString(VID_ALARM_MESSAGE, alarm->message, MAX_EVENT_MSG_LENGTH);
   alarm->state = (BYTE)msg->getFieldAsUInt16(VID_STATE);
   alarm->currentSeverity = (BYTE)msg->getFieldAsUInt16(VID_CURRENT_SEVERITY);
   alarm->originalSeverity = (BYTE)msg->getFieldAsUInt16(VID_ORIGINAL_SEVERITY);
   alarm->repeatCount = msg->getFieldAsUInt32(VID_REPEAT_COUNT);
   alarm->helpDeskState = (BYTE)msg->getFieldAsUInt16(VID_HELPDESK_STATE);
   msg->getFieldAsString(VID_HELPDESK_REF, alarm->helpDeskRef, MAX_HELPDESK_REF_LEN);
	alarm->timeout = msg->getFieldAsUInt32(VID_ALARM_TIMEOUT);
	alarm->timeoutEvent = msg->getFieldAsUInt32(VID_ALARM_TIMEOUT_EVENT);
	alarm->noteCount = msg->getFieldAsUInt32(VID_NUM_COMMENTS);
   alarm->userData = NULL;
   return alarm;
}

/**
 * Format text from alarm data
 * Valid format specifiers are following:
 *		%a Primary IP address of source object
 *		%A Primary host name of source object
 *    %c Repeat count
 *    %e Event code
 *    %E Event name
 *    %h Helpdesk state as number
 *    %H Helpdesk state as text
 *    %i Source object identifier
 *    %I Alarm identifier
 *    %m Message text
 *    %n Source object name
 *    %s Severity as number
 *    %S Severity as text
 *    %x Alarm state as number
 *    %X Alarm state as text
 *    %% Percent sign
 */
TCHAR *AlarmController::formatAlarmText(NXC_ALARM *alarm, const TCHAR *format)
{
	static const TCHAR *alarmState[] = { _T("OUTSTANDING"), _T("ACKNOWLEDGED"), _T("TERMINATED") };
	static const TCHAR *helpdeskState[] = { _T("IGNORED"), _T("OPEN"), _T("CLOSED") };
	static const TCHAR *severityText[] = { _T("NORMAL"), _T("WARNING"), _T("MINOR"), _T("MAJOR"), _T("CRITICAL") };

   ObjectController *oc = (ObjectController *)m_session->getController(CONTROLLER_OBJECTS);
   AbstractObject *object = oc->findObjectById(alarm->sourceObject);
   if (object == NULL)
   {
	   oc->syncSingleObject(alarm->sourceObject);
	   object = oc->findObjectById(alarm->sourceObject);
   }

	String out;
	const TCHAR *prev, *curr;
   TCHAR buffer[128];
	for(prev = format; *prev != 0; prev = curr)
	{
		curr = _tcschr(prev, _T('%'));
		if (curr == NULL)
		{
			out += prev;
			break;
		}
		out.append(prev, (size_t)(curr - prev));
		curr++;
		switch(*curr)
		{
			case '%':
				out += _T("%");
				break;
			case 'a':
            out += (object != NULL) ? object->getPrimaryIP().toString(buffer) : _T("<unknown>");
				break;
			case 'A':
            out += ((object != NULL) && (object->getObjectClass() == OBJECT_NODE)) ? ((Node *)object)->getPrimaryHostname() : _T("<unknown>");
				break;
			case 'c':
				out.appendFormattedString(_T("%u"), (unsigned int)alarm->repeatCount);
				break;
         case 'd':
            out.appendFormattedString(_T("%u"), (unsigned int)alarm->dciId);
            break;
			case 'e':
				out.appendFormattedString(_T("%u"), (unsigned int)alarm->sourceEventCode);
				break;
			case 'E':
            out += ((EventController *)m_session->getController(CONTROLLER_EVENTS))->getEventName(alarm->sourceEventCode, buffer, 128);
				break;
			case 'h':
				out.appendFormattedString(_T("%d"), (int)alarm->helpDeskState);
				break;
			case 'H':
				out += helpdeskState[alarm->helpDeskState];
				break;
			case 'i':
				out.appendFormattedString(_T("%u"), (unsigned int)alarm->sourceObject);
				break;
			case 'I':
				out.appendFormattedString(_T("%u"), (unsigned int)alarm->alarmId);
				break;
			case 'm':
				out.append(alarm->message);
				break;
			case 'n':
				out.append((object != NULL) ? object->getName() : _T("<unknown>"));
				break;
			case 's':
				out.appendFormattedString(_T("%d"), (int)alarm->currentSeverity);
				break;
			case 'S':
				out += severityText[alarm->currentSeverity];
				break;
			case 'x':
				out.appendFormattedString(_T("%d"), (int)alarm->state);
				break;
			case 'X':
				out += alarmState[alarm->state];
				break;
			case 0:
				curr--;
				break;
			default:
				break;
		}
		curr++;
	}
	return _tcsdup((const TCHAR *)out);
}
