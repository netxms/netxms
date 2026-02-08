/*
** NetXMS - Network Management System
** Copyright (C) 2003-2025 Victor Kirhenshtein
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
static void (*s_fpPostEvent1)(uint32_t, const TCHAR*, time_t) = nullptr;
static void (*s_fpPostEvent2)(uint32_t, const TCHAR*, time_t, const StringMap&) = nullptr;
static shared_ptr<AbstractCommSession> (*s_fpFindServerSession)(uint64_t) = nullptr;
static bool (*s_fpEnumerateSessions)(EnumerationCallbackResult (*)(AbstractCommSession *, void *), void *) = nullptr;
static bool (*s_fpPushData)(const TCHAR *, const TCHAR *, uint32_t, Timestamp) = nullptr;
static const TCHAR *s_dataDirectory = nullptr;
static DB_HANDLE (*s_fpGetLocalDatabaseHandle)() = nullptr;
static void (*s_fpExecuteAction)(const TCHAR*, const StringList&) = nullptr;
static bool (*s_fpGetScreenInfoForUserSession)(uint32_t, uint32_t *, uint32_t *, uint32_t *) = nullptr;
static void (*s_fpQueueNotificationMessage)(NXCPMessage*) = nullptr;
static void (*s_fpRegisterProblem)(int, const TCHAR*, const TCHAR*) = nullptr;
static void (*s_fpUnregisterProblem)(const TCHAR*) = nullptr;
static ThreadPool *s_timerThreadPool = nullptr;

/**
 * Initialize subagent API
 */
void LIBNXAGENT_EXPORTABLE InitSubAgentAPI(
      void (*postEvent1)(uint32_t, const TCHAR*, time_t),
      void (*postEvent2)(uint32_t, const TCHAR*, time_t, const StringMap&),
      bool (*enumerateSessions)(EnumerationCallbackResult (*)(AbstractCommSession *, void *), void*),
      shared_ptr<AbstractCommSession> (*findServerSession)(uint64_t),
      bool (*pushData)(const TCHAR *, const TCHAR *, uint32_t, Timestamp),
      DB_HANDLE (*getLocalDatabaseHandle)(),
      const TCHAR *dataDirectory,
      void (*executeAction)(const TCHAR*, const StringList&),
      bool (*getScreenInfoForUserSession)(uint32_t, uint32_t *, uint32_t *, uint32_t *),
      void (*queueNotificationMessage)(NXCPMessage*),
      void (*registerProblem)(int, const TCHAR*, const TCHAR*),
      void (*unregisterProblem)(const TCHAR*),
      ThreadPool *timerThreadPool)
{
   s_fpPostEvent1 = postEvent1;
   s_fpPostEvent2 = postEvent2;
	s_fpEnumerateSessions = enumerateSessions;
   s_fpFindServerSession = findServerSession;
	s_fpPushData = pushData;
   s_dataDirectory = dataDirectory;
   s_fpGetLocalDatabaseHandle = getLocalDatabaseHandle;
   s_fpExecuteAction = executeAction;
   s_fpGetScreenInfoForUserSession = getScreenInfoForUserSession;
   s_fpQueueNotificationMessage = queueNotificationMessage;
   s_fpRegisterProblem = registerProblem;
   s_fpUnregisterProblem = unregisterProblem;
   s_timerThreadPool = timerThreadPool;
}

/**
 * Send event from agent to server
 */
void LIBNXAGENT_EXPORTABLE AgentPostEvent(uint32_t eventCode, const TCHAR *eventName, time_t timestamp)
{
   if (s_fpPostEvent1 != nullptr)
      s_fpPostEvent1(eventCode, eventName, timestamp);
}

/**
 * Send event from agent to server
 */
void LIBNXAGENT_EXPORTABLE AgentPostEvent(uint32_t eventCode, const TCHAR *eventName, time_t timestamp, const StringMap &args)
{
   if (s_fpPostEvent2 != nullptr)
      s_fpPostEvent2(eventCode, eventName, timestamp, args);
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
 * Push parameter's value
 */
bool LIBNXAGENT_EXPORTABLE AgentPushParameterData(const TCHAR *parameter, const TCHAR *value)
{
	if (s_fpPushData == nullptr)
		return FALSE;
	return s_fpPushData(parameter, value, 0, Timestamp::fromMilliseconds(0));
}

/**
 * Push parameter's value
 */
bool LIBNXAGENT_EXPORTABLE AgentPushParameterDataInt32(const TCHAR *parameter, int32_t value)
{
	TCHAR buffer[64];
	return AgentPushParameterData(parameter, IntegerToString(value, buffer));
}

/**
 * Push parameter's value
 */
bool LIBNXAGENT_EXPORTABLE AgentPushParameterDataUInt32(const TCHAR *parameter, uint32_t value)
{
   TCHAR buffer[64];
   return AgentPushParameterData(parameter, IntegerToString(value, buffer));
}

/**
 * Push parameter's value
 */
bool LIBNXAGENT_EXPORTABLE AgentPushParameterDataInt64(const TCHAR *parameter, int64_t value)
{
   TCHAR buffer[64];
   return AgentPushParameterData(parameter, IntegerToString(value, buffer));
}

/**
 * Push parameter's value
 */
bool LIBNXAGENT_EXPORTABLE AgentPushParameterDataUInt64(const TCHAR *parameter, uint64_t value)
{
   TCHAR buffer[64];
   return AgentPushParameterData(parameter, IntegerToString(value, buffer));
}

/**
 * Push parameter's value
 */
bool LIBNXAGENT_EXPORTABLE AgentPushParameterDataDouble(const TCHAR *parameter, double value)
{
	TCHAR buffer[64];
	_sntprintf(buffer, 64, _T("%f"), value);
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
shared_ptr<AbstractCommSession> LIBNXAGENT_EXPORTABLE AgentFindServerSession(uint64_t serverId)
{
   return (s_fpFindServerSession != nullptr) ? s_fpFindServerSession(serverId) : shared_ptr<AbstractCommSession>();
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
void LIBNXAGENT_EXPORTABLE AgentQueueNotificationMessage(NXCPMessage *msg)
{
   if (s_fpQueueNotificationMessage != nullptr)
      s_fpQueueNotificationMessage(msg);
   else
      delete msg;
}

/**
 * Register agent problem
 */
void LIBNXAGENT_EXPORTABLE AgentRegisterProblem(int severity, const TCHAR *key, const TCHAR *message)
{
   if (s_fpRegisterProblem != nullptr)
      s_fpRegisterProblem(severity, key, message);
}

/**
 * Unregister agent problem with given key
 */
void LIBNXAGENT_EXPORTABLE AgentUnregisterProblem(const TCHAR *key)
{
   if (s_fpUnregisterProblem != nullptr)
      s_fpUnregisterProblem(key);
}

/**
 * Set timer
 */
void LIBNXAGENT_EXPORTABLE AgentSetTimer(uint32_t delay, std::function<void()> callback)
{
   if (s_timerThreadPool != nullptr && !IsShutdownInProgress())
      ThreadPoolScheduleRelative(s_timerThreadPool, delay, callback);
}
