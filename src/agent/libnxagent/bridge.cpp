/*
** NetXMS - Network Management System
** Copyright (C) 2003-2020 Victor Kirhenshtein
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
** File: bridge.cpp
**
**/

#include "libnxagent.h"

/**
 * Static data
 */
static void (* s_fpWriteLog)(int, int, const TCHAR *) = nullptr;
static void (* s_fpPostEvent1)(uint32_t, const TCHAR *, time_t, const char *, va_list) = nullptr;
static void (* s_fpPostEvent2)(uint32_t, const TCHAR *, time_t, int, const TCHAR **) = nullptr;
static AbstractCommSession *(* s_fpFindServerSession)(UINT64) = nullptr;
static bool (* s_fpEnumerateSessions)(EnumerationCallbackResult (*)(AbstractCommSession *, void *), void *) = nullptr;
static bool (* s_fpSendFile)(void *, UINT32, const TCHAR *, long, bool, VolatileCounter *) = nullptr;
static bool (* s_fpPushData)(const TCHAR *, const TCHAR *, UINT32, time_t) = nullptr;
static const TCHAR *s_dataDirectory = nullptr;
static DB_HANDLE (*s_fpGetLocalDatabaseHandle)() = nullptr;
static void (*s_fpExecuteAction)(const TCHAR*, const StringList&) = nullptr;
static bool (*s_fpGetScreenInfoForUserSession)(uint32_t, uint32_t *, uint32_t *, uint32_t *) = nullptr;
static void (*s_fpQueueNotificationMessage)(NXCPMessage *) = nullptr;

/**
 * Initialize subagent API
 */
void LIBNXAGENT_EXPORTABLE InitSubAgentAPI(
      void (*writeLog)(int, int, const TCHAR *),
      void (*postEvent1)(uint32_t, const TCHAR *, time_t, const char *, va_list),
      void (*postEvent2)(uint32_t, const TCHAR *, time_t, int, const TCHAR **),
      bool (*enumerateSessions)(EnumerationCallbackResult (*)(AbstractCommSession *, void *), void*),
      AbstractCommSession *(*findServerSession)(UINT64),
      bool (*sendFile)(void *, UINT32, const TCHAR *, long, bool, VolatileCounter *),
      bool (*pushData)(const TCHAR *, const TCHAR *, UINT32, time_t),
      DB_HANDLE (*getLocalDatabaseHandle)(),
      const TCHAR *dataDirectory,
      void (*executeAction)(const TCHAR*, const StringList&),
      bool (*getScreenInfoForUserSession)(uint32_t, uint32_t *, uint32_t *, uint32_t *),
      void (*queueNotificationMessage)(NXCPMessage*))
{
   s_fpWriteLog = writeLog;
	s_fpPostEvent1 = postEvent1;
	s_fpPostEvent2 = postEvent2;
	s_fpEnumerateSessions = enumerateSessions;
   s_fpFindServerSession = findServerSession;
	s_fpSendFile = sendFile;
	s_fpPushData = pushData;
   s_dataDirectory = dataDirectory;
   s_fpGetLocalDatabaseHandle = getLocalDatabaseHandle;
   s_fpExecuteAction = executeAction;
   s_fpGetScreenInfoForUserSession = getScreenInfoForUserSession;
   s_fpQueueNotificationMessage = queueNotificationMessage;
}

/**
 * Write message to agent's log
 */
void LIBNXAGENT_EXPORTABLE AgentWriteLog(int logLevel, const TCHAR *format, ...)
{
   TCHAR szBuffer[4096];
   va_list args;

   if (s_fpWriteLog != nullptr)
   {
      va_start(args, format);
      _vsntprintf(szBuffer, 4096, format, args);
      va_end(args);
      szBuffer[4095] = 0;
      s_fpWriteLog(logLevel, 0, szBuffer);
   }
}

/**
 * Write message to agent's log
 */
void LIBNXAGENT_EXPORTABLE AgentWriteLog2(int logLevel, const TCHAR *format, va_list args)
{
   TCHAR szBuffer[4096];

   if (s_fpWriteLog != nullptr)
   {
      _vsntprintf(szBuffer, 4096, format, args);
      szBuffer[4095] = 0;
      s_fpWriteLog(logLevel, 0, szBuffer);
   }
}

/**
 * Write debug message to agent's log
 */
void LIBNXAGENT_EXPORTABLE AgentWriteDebugLog(int level, const TCHAR *format, ...)
{
   TCHAR szBuffer[4096];
   va_list args;

   if (s_fpWriteLog != nullptr)
   {
      va_start(args, format);
      _vsntprintf(szBuffer, 4096, format, args);
      va_end(args);
      szBuffer[4095] = 0;
      s_fpWriteLog(EVENTLOG_DEBUG_TYPE, level, szBuffer);
   }
}

/**
 * Write debug message to agent's log
 */
void LIBNXAGENT_EXPORTABLE AgentWriteDebugLog2(int level, const TCHAR *format, va_list args)
{
   TCHAR szBuffer[4096];

   if (s_fpWriteLog != nullptr)
   {
      _vsntprintf(szBuffer, 4096, format, args);
      szBuffer[4095] = 0;
      s_fpWriteLog(EVENTLOG_DEBUG_TYPE, level, szBuffer);
   }
}

/**
 * Send event from agent to server
 */
void LIBNXAGENT_EXPORTABLE AgentPostEvent(uint32_t eventCode, const TCHAR *eventName, time_t timestamp, const char *format, ...)
{
   if (s_fpPostEvent1 != nullptr)
   {
      va_list args;
      va_start(args, format);
      s_fpPostEvent1(eventCode, eventName, timestamp, format, args);
      va_end(args);
   }
}

/**
 * Send event from agent to server
 */
void LIBNXAGENT_EXPORTABLE AgentPostEvent2(uint32_t eventCode, const TCHAR *eventName, time_t timestamp, int count, const TCHAR **args)
{
   if (s_fpPostEvent2 != nullptr)
      s_fpPostEvent2(eventCode, eventName, timestamp, count, args);
}

/**
 * Enumerates active agent sessions. Callback will be called for each valid session.
 * Callback must return _STOP to stop enumeration or _CONTINUE to continue.
 *
 * @return true if enumeration was stopped by callback
 */
bool LIBNXAGENT_EXPORTABLE AgentEnumerateSessions(EnumerationCallbackResult (* callback)(AbstractCommSession *, void *), void *data)
{
   return (s_fpEnumerateSessions != nullptr) ? s_fpEnumerateSessions(callback, data) : false;
}

/**
 * Send file to server
 */
bool LIBNXAGENT_EXPORTABLE AgentSendFileToServer(void *session, UINT32 requestId, const TCHAR *file, long offset, 
                                                 bool allowCompression, VolatileCounter *cancellationFlag)
{
	if ((s_fpSendFile == nullptr) || (session == nullptr) || (file == nullptr))
		return FALSE;
	return s_fpSendFile(session, requestId, file, offset, allowCompression, cancellationFlag);
}

/**
 * Push parameter's value
 */
bool LIBNXAGENT_EXPORTABLE AgentPushParameterData(const TCHAR *parameter, const TCHAR *value)
{
	if (s_fpPushData == nullptr)
		return FALSE;
	return s_fpPushData(parameter, value, 0, 0);
}

/**
 * Push parameter's value
 */
bool LIBNXAGENT_EXPORTABLE AgentPushParameterDataInt32(const TCHAR *parameter, LONG value)
{
	TCHAR buffer[64];

	_sntprintf(buffer, sizeof(buffer), _T("%d"), (int)value);
	return AgentPushParameterData(parameter, buffer);
}

/**
 * Push parameter's value
 */
bool LIBNXAGENT_EXPORTABLE AgentPushParameterDataUInt32(const TCHAR *parameter, UINT32 value)
{
	TCHAR buffer[64];

	_sntprintf(buffer, sizeof(buffer), _T("%u"), (unsigned int)value);
	return AgentPushParameterData(parameter, buffer);
}

/**
 * Push parameter's value
 */
bool LIBNXAGENT_EXPORTABLE AgentPushParameterDataInt64(const TCHAR *parameter, INT64 value)
{
	TCHAR buffer[64];

	_sntprintf(buffer, sizeof(buffer), INT64_FMT, value);
	return AgentPushParameterData(parameter, buffer);
}

/**
 * Push parameter's value
 */
bool LIBNXAGENT_EXPORTABLE AgentPushParameterDataUInt64(const TCHAR *parameter, QWORD value)
{
	TCHAR buffer[64];

	_sntprintf(buffer, sizeof(buffer), UINT64_FMT, value);
	return AgentPushParameterData(parameter, buffer);
}

/**
 * Push parameter's value
 */
bool LIBNXAGENT_EXPORTABLE AgentPushParameterDataDouble(const TCHAR *parameter, double value)
{
	TCHAR buffer[64];

	_sntprintf(buffer, sizeof(buffer), _T("%f"), value);
	return AgentPushParameterData(parameter, buffer);
}

/**
 * Get data directory
 */
const TCHAR LIBNXAGENT_EXPORTABLE *AgentGetDataDirectory()
{
   return s_dataDirectory;
}

/**
 * Find server session. Caller must call decRefCount() for session object when finished.
 *
 * @param serverId server ID
 * @return server session object or nullptr
 */
AbstractCommSession LIBNXAGENT_EXPORTABLE *AgentFindServerSession(UINT64 serverId)
{
   return (s_fpFindServerSession != nullptr) ? s_fpFindServerSession(serverId) : nullptr;
}

/**
 * Get handle to local database.
 *
 * @return database handle or nullptr if not available
 */
DB_HANDLE LIBNXAGENT_EXPORTABLE AgentGetLocalDatabaseHandle()
{
   return (s_fpGetLocalDatabaseHandle != nullptr) ? s_fpGetLocalDatabaseHandle() : nullptr;
}

/**
 * Execute agent action or command line command
 *
 * @param agent action or command
 */
void LIBNXAGENT_EXPORTABLE AgentExecuteAction(const TCHAR *action, const StringList& args)
{
   if (s_fpExecuteAction != nullptr)
      s_fpExecuteAction(action, args);
}

/**
 * Get screen information for given user session via session agent.
 */
bool LIBNXAGENT_EXPORTABLE AgentGetScreenInfoForUserSession(uint32_t sessionId, uint32_t *width, uint32_t *height, uint32_t *bpp)
{
   return (s_fpGetScreenInfoForUserSession != nullptr) ? s_fpGetScreenInfoForUserSession(sessionId, width, height, bpp) : false;
}

/**
 * Add message to notification queue. Ownership will be taken by the queue and
 * pointer to message should be considered invalid after this call.
 */
void LIBNXAGENT_EXPORTABLE AgentQueueNotifictionMessage(NXCPMessage *msg)
{
   if (s_fpQueueNotificationMessage != nullptr)
      s_fpQueueNotificationMessage(msg);
   else
      delete msg;
}
