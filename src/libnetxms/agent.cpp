/* 
** NetXMS - Network Management System
** NetXMS Foundation Library
** Copyright (C) 2003-2013 Victor Kirhenshtein
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
** File: agent.cpp
**
**/

#include "libnetxms.h"

/**
 * Static data
 */
static void (* s_fpWriteLog)(int, int, const TCHAR *) = NULL;
static void (* s_fpSendTrap1)(UINT32, const TCHAR *, const char *, va_list) = NULL;
static void (* s_fpSendTrap2)(UINT32, const TCHAR *, int, TCHAR **) = NULL;
static bool (* s_fpSendFile)(void *, UINT32, const TCHAR *, long) = NULL;
static bool (* s_fpPushData)(const TCHAR *, const TCHAR *, UINT32) = NULL;

/**
 * Initialize subagent API
 */
void LIBNETXMS_EXPORTABLE InitSubAgentAPI(void (* writeLog)(int, int, const TCHAR *),
														void (* sendTrap1)(UINT32, const TCHAR *, const char *, va_list),
														void (* sendTrap2)(UINT32, const TCHAR *, int, TCHAR **),
														bool (* sendFile)(void *, UINT32, const TCHAR *, long),
														bool (* pushData)(const TCHAR *, const TCHAR *, UINT32))
{
   s_fpWriteLog = writeLog;
	s_fpSendTrap1 = sendTrap1;
	s_fpSendTrap2 = sendTrap2;
	s_fpSendFile = sendFile;
	s_fpPushData = pushData;
}

/**
 * Write message to agent's log
 */
void LIBNETXMS_EXPORTABLE AgentWriteLog(int logLevel, const TCHAR *format, ...)
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

void LIBNETXMS_EXPORTABLE AgentWriteLog2(int logLevel, const TCHAR *format, va_list args)
{
   TCHAR szBuffer[4096];

   if (s_fpWriteLog != NULL)
   {
      _vsntprintf(szBuffer, 4096, format, args);
      s_fpWriteLog(logLevel, 0, szBuffer);
   }
}

void LIBNETXMS_EXPORTABLE AgentWriteDebugLog(int level, const TCHAR *format, ...)
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

void LIBNETXMS_EXPORTABLE AgentWriteDebugLog2(int level, const TCHAR *format, va_list args)
{
   TCHAR szBuffer[4096];

   if (s_fpWriteLog != NULL)
   {
      _vsntprintf(szBuffer, 4096, format, args);
      s_fpWriteLog(EVENTLOG_DEBUG_TYPE, level, szBuffer);
   }
}


//
// Send trap from agent to server
//

void LIBNETXMS_EXPORTABLE AgentSendTrap(UINT32 dwEvent, const TCHAR *eventName, const char *pszFormat, ...)
{
   va_list args;

   if (s_fpSendTrap1 != NULL)
   {
      va_start(args, pszFormat);
      s_fpSendTrap1(dwEvent, eventName, pszFormat, args);
      va_end(args);
   }
}

void LIBNETXMS_EXPORTABLE AgentSendTrap2(UINT32 dwEvent, const TCHAR *eventName, int nCount, TCHAR **ppszArgList)
{
   if (s_fpSendTrap2 != NULL)
      s_fpSendTrap2(dwEvent, eventName, nCount, ppszArgList);
}


//
// Get arguments for parameters like name(arg1,...)
// Returns FALSE on processing error
//

static BOOL AgentGetParameterArgInternal(const TCHAR *param, int index, TCHAR *arg, int maxSize)
{
   const TCHAR *ptr1, *ptr2;
   int state, currIndex, pos;
   BOOL bResult = TRUE;

   arg[0] = 0;    // Default is empty string
   ptr1 = _tcschr(param, _T('('));
   if (ptr1 == NULL)
      return TRUE;  // No arguments at all
   for(ptr2 = ptr1 + 1, currIndex = 1, state = 0, pos = 0; state != -1; ptr2++)
   {
      switch(state)
      {
         case 0:  // Normal
            switch(*ptr2)
            {
               case _T(')'):
                  if (currIndex == index)
                     arg[pos] = 0;
                  state = -1;    // Finish processing
                  break;
               case _T('"'):
                  state = 1;     // String
                  break;
               case _T('\''):        // String, type 2
                  state = 2;
                  break;
               case _T(','):
                  if (currIndex == index)
                  {
                     arg[pos] = 0;
                     state = -1;
                  }
                  else
                  {
                     currIndex++;
                  }
                  break;
               case 0:
                  state = -1;       // Finish processing
                  bResult = FALSE;  // Set error flag
                  break;
               default:
                  if ((currIndex == index) && (pos < maxSize - 1))
                     arg[pos++] = *ptr2;
            }
            break;
         case 1:  // String in ""
            switch(*ptr2)
            {
               case _T('"'):
                  state = 0;     // Normal
                  break;
               case 0:
                  state = -1;    // Finish processing
                  bResult = FALSE;  // Set error flag
                  break;
               default:
                  if ((currIndex == index) && (pos < maxSize - 1))
                     arg[pos++] = *ptr2;
            }
            break;
         case 2:  // String in ''
            switch(*ptr2)
            {
               case _T('\''):
                  state = 0;     // Normal
                  break;
               case 0:
                  state = -1;    // Finish processing
                  bResult = FALSE;  // Set error flag
                  break;
               default:
                  if ((currIndex == index) && (pos < maxSize - 1))
                     arg[pos++] = *ptr2;
            }
            break;
      }
   }

   if (bResult)
      StrStrip(arg);
   return bResult;
}

BOOL LIBNETXMS_EXPORTABLE AgentGetParameterArgA(const TCHAR *param, int index, char *arg, int maxSize)
{
#ifdef UNICODE
	WCHAR *temp = (WCHAR *)malloc(maxSize * sizeof(WCHAR));
	BOOL success = AgentGetParameterArgInternal(param, index, temp, maxSize);
	if (success)
	{
		WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR, temp, -1, arg, maxSize, NULL, NULL);
		arg[maxSize - 1] = 0;
	}
	free(temp);
	return success;
#else
	return AgentGetParameterArgInternal(param, index, arg, maxSize);
#endif
}

BOOL LIBNETXMS_EXPORTABLE AgentGetParameterArgW(const TCHAR *param, int index, WCHAR *arg, int maxSize)
{
#ifdef UNICODE
	return AgentGetParameterArgInternal(param, index, arg, maxSize);
#else
	char *temp = (char *)malloc(maxSize);
	BOOL success = AgentGetParameterArgInternal(param, index, temp, maxSize);
	if (success)
	{
		MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, temp, -1, arg, maxSize);
		arg[maxSize - 1] = 0;
	}
	free(temp);
	return success;
#endif
}

/**
 * Send file to server
 */
BOOL LIBNETXMS_EXPORTABLE AgentSendFileToServer(void *session, UINT32 requestId, const TCHAR *file, long offset)
{
	if ((s_fpSendFile == NULL) || (session == NULL) || (file == NULL))
		return FALSE;
	return s_fpSendFile(session, requestId, file, offset);
}

/**
 * Push parameter's value
 */
BOOL LIBNETXMS_EXPORTABLE AgentPushParameterData(const TCHAR *parameter, const TCHAR *value)
{
	if (s_fpPushData == NULL)
		return FALSE;
	return s_fpPushData(parameter, value, 0);
}

BOOL LIBNETXMS_EXPORTABLE AgentPushParameterDataInt32(const TCHAR *parameter, LONG value)
{
	TCHAR buffer[64];

	_sntprintf(buffer, sizeof(buffer), _T("%d"), (int)value);
	return AgentPushParameterData(parameter, buffer);
}

BOOL LIBNETXMS_EXPORTABLE AgentPushParameterDataUInt32(const TCHAR *parameter, UINT32 value)
{
	TCHAR buffer[64];

	_sntprintf(buffer, sizeof(buffer), _T("%u"), (unsigned int)value);
	return AgentPushParameterData(parameter, buffer);
}

BOOL LIBNETXMS_EXPORTABLE AgentPushParameterDataInt64(const TCHAR *parameter, INT64 value)
{
	TCHAR buffer[64];

	_sntprintf(buffer, sizeof(buffer), INT64_FMT, value);
	return AgentPushParameterData(parameter, buffer);
}

BOOL LIBNETXMS_EXPORTABLE AgentPushParameterDataUInt64(const TCHAR *parameter, QWORD value)
{
	TCHAR buffer[64];

	_sntprintf(buffer, sizeof(buffer), UINT64_FMT, value);
	return AgentPushParameterData(parameter, buffer);
}

BOOL LIBNETXMS_EXPORTABLE AgentPushParameterDataDouble(const TCHAR *parameter, double value)
{
	TCHAR buffer[64];

	_sntprintf(buffer, sizeof(buffer), _T("%f"), value);
	return AgentPushParameterData(parameter, buffer);
}
