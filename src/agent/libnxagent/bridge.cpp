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
static void (* s_fpSendTrap2)(UINT32, const TCHAR *, int, TCHAR **) = NULL;
static bool (* s_fpEnumerateSessions)(bool (*)(AbstractCommSession *, void *), void *) = NULL;
static bool (* s_fpSendFile)(void *, UINT32, const TCHAR *, long) = NULL;
static bool (* s_fpPushData)(const TCHAR *, const TCHAR *, UINT32, time_t) = NULL;
static CONDITION s_agentShutdownCondition = INVALID_CONDITION_HANDLE;
static Config *s_registry = NULL;
static void (* s_fpSaveRegistry)() = NULL;
static const TCHAR *s_dataDirectory = NULL;

/**
 * Initialize subagent API
 */
void LIBNXAGENT_EXPORTABLE InitSubAgentAPI(void (* writeLog)(int, int, const TCHAR *),
														void (* sendTrap1)(UINT32, const TCHAR *, const char *, va_list),
														void (* sendTrap2)(UINT32, const TCHAR *, int, TCHAR **),
														bool (* enumerateSessions)(bool (*)(AbstractCommSession *, void *), void*),
														bool (* sendFile)(void *, UINT32, const TCHAR *, long),
														bool (* pushData)(const TCHAR *, const TCHAR *, UINT32, time_t),
                                          CONDITION shutdownCondition, Config *registry,
                                          void (* saveRegistry)(), const TCHAR *dataDirectory)
{
   s_fpWriteLog = writeLog;
	s_fpSendTrap1 = sendTrap1;
	s_fpSendTrap2 = sendTrap2;
	s_fpEnumerateSessions = enumerateSessions;
	s_fpSendFile = sendFile;
	s_fpPushData = pushData;
   s_agentShutdownCondition = shutdownCondition;
   s_registry = registry;
   s_fpSaveRegistry = saveRegistry;
   s_dataDirectory = dataDirectory;
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
void LIBNXAGENT_EXPORTABLE AgentSendTrap2(UINT32 dwEvent, const TCHAR *eventName, int nCount, TCHAR **ppszArgList)
{
   if (s_fpSendTrap2 != NULL)
      s_fpSendTrap2(dwEvent, eventName, nCount, ppszArgList);
}

/**
 * Goes throught all sessions and executes
 */
bool LIBNXAGENT_EXPORTABLE EnumerateSessions(bool (* callback)(AbstractCommSession *, void *), void *data)
{
   if (s_fpEnumerateSessions != NULL)
   {
      return s_fpEnumerateSessions(callback, data);
   }
   else
      return false;
}

/**
 * Send file to server
 */
bool LIBNXAGENT_EXPORTABLE AgentSendFileToServer(void *session, UINT32 requestId, const TCHAR *file, long offset)
{
	if ((s_fpSendFile == NULL) || (session == NULL) || (file == NULL))
		return FALSE;
	return s_fpSendFile(session, requestId, file, offset);
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
 * Open registry
 */
Config LIBNXAGENT_EXPORTABLE *AgentOpenRegistry()
{
	s_registry->lock();
	return s_registry;
}

/**
 * Close registry
 */
void LIBNXAGENT_EXPORTABLE AgentCloseRegistry(bool modified)
{
	if (modified)
   {
      if (s_fpSaveRegistry != NULL)
		   s_fpSaveRegistry();
   }
	s_registry->unlock();
}

/**
 * Get data directory
 */
const TCHAR LIBNXAGENT_EXPORTABLE *AgentGetDataDirectory()
{
   return s_dataDirectory;
}
