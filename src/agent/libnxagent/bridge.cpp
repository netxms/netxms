/*
** NetXMS - Network Management System
** Copyright (C) 2003-2015 Victor Kirhenshtein
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
static void (* s_fpWriteLog)(int, int, const TCHAR *) = NULL;
static void (* s_fpSendTrap1)(UINT32, const TCHAR *, const char *, va_list) = NULL;
static void (* s_fpSendTrap2)(UINT32, const TCHAR *, int, const TCHAR **) = NULL;
static AbstractCommSession *(* s_fpFindServerSession)(UINT64) = NULL;
static bool (* s_fpEnumerateSessions)(EnumerationCallbackResult (*)(AbstractCommSession *, void *), void *) = NULL;
static bool (* s_fpSendFile)(void *, UINT32, const TCHAR *, long, bool, VolatileCounter *) = NULL;
static bool (* s_fpPushData)(const TCHAR *, const TCHAR *, UINT32, time_t) = NULL;
static CONDITION s_agentShutdownCondition = INVALID_CONDITION_HANDLE;
static const TCHAR *s_dataDirectory = NULL;
static DB_HANDLE (*s_fpGetLocalDatabaseHandle)() = NULL;
static void (* s_fpExecuteAction)(const TCHAR *, const StringList *);

/**
 * Initialize subagent API
 */
void LIBNXAGENT_EXPORTABLE InitSubAgentAPI(void (* writeLog)(int, int, const TCHAR *),
                                           void (* sendTrap1)(UINT32, const TCHAR *, const char *, va_list),
                                           void (* sendTrap2)(UINT32, const TCHAR *, int, const TCHAR **),
                                           bool (* enumerateSessions)(EnumerationCallbackResult (*)(AbstractCommSession *, void *), void*),
                                           AbstractCommSession *(* findServerSession)(UINT64),
                                           bool (* sendFile)(void *, UINT32, const TCHAR *, long, bool, VolatileCounter *),
                                           bool (* pushData)(const TCHAR *, const TCHAR *, UINT32, time_t),
                                           DB_HANDLE (* getLocalDatabaseHandle)(),
                                           CONDITION shutdownCondition, const TCHAR *dataDirectory, void (* executeAction)(const TCHAR *, const StringList*))
{
   s_fpWriteLog = writeLog;
	s_fpSendTrap1 = sendTrap1;
	s_fpSendTrap2 = sendTrap2;
	s_fpEnumerateSessions = enumerateSessions;
   s_fpFindServerSession = findServerSession;
	s_fpSendFile = sendFile;
	s_fpPushData = pushData;
   s_agentShutdownCondition = shutdownCondition;
   s_dataDirectory = dataDirectory;
   s_fpGetLocalDatabaseHandle = getLocalDatabaseHandle;
   s_fpExecuteAction = executeAction;
}

/**
 * Write message to agent's log
 */
void LIBNXAGENT_EXPORTABLE AgentWriteLog(int logLevel, const TCHAR *format, ...)
{
   TCHAR szBuffer[4096];
   va_list args;

   if (s_fpWriteLog != NULL)
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

   if (s_fpWriteLog != NULL)
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

   if (s_fpWriteLog != NULL)
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

   if (s_fpWriteLog != NULL)
   {
      _vsntprintf(szBuffer, 4096, format, args);
      szBuffer[4095] = 0;
      s_fpWriteLog(EVENTLOG_DEBUG_TYPE, level, szBuffer);
   }
}

/**
 * Send trap from agent to server
 */
void LIBNXAGENT_EXPORTABLE AgentSendTrap(UINT32 dwEvent, const TCHAR *eventName, const char *pszFormat, ...)
{
   va_list args;

   if (s_fpSendTrap1 != NULL)
   {
      va_start(args, pszFormat);
      s_fpSendTrap1(dwEvent, eventName, pszFormat, args);
      va_end(args);
   }
}

/**
 * Send trap from agent to server
 */
void LIBNXAGENT_EXPORTABLE AgentSendTrap2(UINT32 dwEvent, const TCHAR *eventName, int nCount, const TCHAR **ppszArgList)
{
   if (s_fpSendTrap2 != NULL)
      s_fpSendTrap2(dwEvent, eventName, nCount, ppszArgList);
}

/**
 * Enumerates active agent sessions. Callback will be called for each valid session.
 * Callback must return _STOP to stop enumeration or _CONTINUE to continue.
 *
 * @return true if enumeration was stopped by callback
 */
bool LIBNXAGENT_EXPORTABLE AgentEnumerateSessions(EnumerationCallbackResult (* callback)(AbstractCommSession *, void *), void *data)
{
   return (s_fpEnumerateSessions != NULL) ? s_fpEnumerateSessions(callback, data) : false;
}

/**
 * Send file to server
 */
bool LIBNXAGENT_EXPORTABLE AgentSendFileToServer(void *session, UINT32 requestId, const TCHAR *file, long offset, 
                                                 bool allowCompression, VolatileCounter *cancellationFlag)
{
	if ((s_fpSendFile == NULL) || (session == NULL) || (file == NULL))
		return FALSE;
	return s_fpSendFile(session, requestId, file, offset, allowCompression, cancellationFlag);
}

/**
 * Push parameter's value
 */
bool LIBNXAGENT_EXPORTABLE AgentPushParameterData(const TCHAR *parameter, const TCHAR *value)
{
	if (s_fpPushData == NULL)
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
 * Get shutdown condition
 */
CONDITION LIBNXAGENT_EXPORTABLE AgentGetShutdownCondition()
{
   return s_agentShutdownCondition;
}

/**
 * Sleep and check for agent shutdown
 */
bool LIBNXAGENT_EXPORTABLE AgentSleepAndCheckForShutdown(UINT32 sleepTime)
{
   return ConditionWait(s_agentShutdownCondition, sleepTime);
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
 * @return server session object or NULL
 */
AbstractCommSession LIBNXAGENT_EXPORTABLE *AgentFindServerSession(UINT64 serverId)
{
   return (s_fpFindServerSession != NULL) ? s_fpFindServerSession(serverId) : NULL;
}

/**
 * Get handle to local database.
 *
 * @return database handle or NULL if not available
 */
DB_HANDLE LIBNXAGENT_EXPORTABLE AgentGetLocalDatabaseHandle()
{
   return (s_fpGetLocalDatabaseHandle != NULL) ? s_fpGetLocalDatabaseHandle() : NULL;
}

/**
 * Execute agent action or command line command
 *
 * @param agent action or command
 */
void LIBNXAGENT_EXPORTABLE AgentExecuteAction(const TCHAR *action, const StringList *args)
{
   if (s_fpExecuteAction != NULL)
      s_fpExecuteAction(action, args);
}
