/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2017 Victor Kirhenshtein
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
** File: appagent.cpp
**
**/

#include "appagent-internal.h"
#include <nxproc.h>

#if defined(_AIX) || defined(__sun)
#include <signal.h>
#endif

/**
 * Application agent config
 */
static APPAGENT_INIT s_config;
static bool s_initialized = false;

/**
 * Write log
 */
static void AppAgentWriteLog(int level, const TCHAR *format, ...)
{
	if (s_config.logger == NULL)
		return;

	va_list(args);
	va_start(args, format);
	s_config.logger(level, format, args);
	va_end(args);
}

/**
 * Get metric
 */
static APPAGENT_MSG *GetMetric(WCHAR *name, int length)
{
	TCHAR metricName[256];

#ifdef UNICODE
	wcslcpy(metricName, name, std::min(length, 256));
#else
	WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR, name, length, metricName, 256, NULL, NULL);
	metricName[std::min(length, 255)] = 0;
#endif

	for(int i = 0; i < s_config.numMetrics; i++)
	{
		if (MatchString(s_config.metrics[i].name, metricName, FALSE))
		{
			TCHAR value[256];
			int rcc = s_config.metrics[i].handler(metricName, s_config.metrics[i].userArg, value);

			APPAGENT_MSG *msg;
			if (rcc == APPAGENT_RCC_SUCCESS)
			{
				int len = (int)_tcslen(value) + 1;
				msg = NewMessage(APPAGENT_CMD_REQUEST_COMPLETED, rcc, len * sizeof(WCHAR));
#ifdef UNICODE
				wcscpy((WCHAR *)msg->payload, value);
#else
				MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, value, -1, (WCHAR *)msg->payload, len);
#endif
			}
			else
			{
				msg = NewMessage(APPAGENT_CMD_REQUEST_COMPLETED, rcc, 0);
			}
			return msg;
		}
	}
	return NewMessage(APPAGENT_CMD_REQUEST_COMPLETED, APPAGENT_RCC_NO_SUCH_METRIC, 0);
}

/**
 * Encode string into message
 */
static BYTE *EncodeString(BYTE *data, const TCHAR *str)
{
   BYTE *curr = data;
   int len = (int)_tcslen(str);
   if (len > 255)
      len = 255;
   *curr = (BYTE)len;
   curr += sizeof(TCHAR);
#ifdef UNICODE
   memcpy(curr, str, len * sizeof(TCHAR));
#else
	MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, str, -1, (WCHAR *)curr, len);
#endif
   curr += len * sizeof(WCHAR);
   return curr;
}

/**
 * List available metrics
 */
static APPAGENT_MSG *ListMetrics()
{
   int i;
   int msgLen = sizeof(INT16);
   for(i = 0; i < s_config.numMetrics; i++)
      msgLen += (int)((_tcslen(s_config.metrics[i].name) + _tcslen(s_config.metrics[i].description) + 2) * sizeof(TCHAR) + sizeof(INT16));

   APPAGENT_MSG *msg = NewMessage(APPAGENT_CMD_REQUEST_COMPLETED, APPAGENT_RCC_SUCCESS, msgLen);
   *((INT16 *)msg->payload) = (INT16)s_config.numMetrics;

   BYTE *curr = (BYTE *)msg->payload + sizeof(INT16);
   for(i = 0; i < s_config.numMetrics; i++)
   {
      *((INT16 *)curr) = (INT16)s_config.metrics[i].type;
      curr = EncodeString(curr + sizeof(INT16), s_config.metrics[i].name);
      EncodeString(curr, s_config.metrics[i].description);
   }

   return msg;
}

/**
 * Process incoming request
 */
static void ProcessRequest(NamedPipe *pipe, void *arg)
{
	AppAgentMessageBuffer *mb = new AppAgentMessageBuffer;

	AppAgentWriteLog(7, _T("ProcessRequest: connection established"));
	while(true)
	{
		APPAGENT_MSG *msg = ReadMessageFromPipe(pipe->handle(), mb);
		if (msg == NULL)
			break;
		AppAgentWriteLog(7, _T("ProcessRequest: received message %04X"), (unsigned int)msg->command);
		APPAGENT_MSG *response;
		switch(msg->command)
		{
			case APPAGENT_CMD_GET_METRIC:
				response = GetMetric((WCHAR *)msg->payload, msg->length - APPAGENT_MSG_HEADER_LEN);
				break;
         case APPAGENT_CMD_LIST_METRICS:
				response = ListMetrics();
            break;
			default:
				response = NewMessage(APPAGENT_CMD_REQUEST_COMPLETED, APPAGENT_RCC_BAD_REQUEST, 0);
				break;
		}
		free(msg);
		SendMessageToPipe(pipe->handle(), response);
		free(response);
	}
	AppAgentWriteLog(7, _T("ProcessRequest: connection closed"));
	delete mb;
}

#if defined(_AIX) || defined(__sun)

/**
 * Dummy signal handler
 */
static void DummySignalHandler(int s)
{
}

#endif

/**
 * Initialize application agent
 */
bool APPAGENT_EXPORTABLE AppAgentInit(APPAGENT_INIT *initData)
{
	if (s_initialized)
		return false;	// already initialized

	memcpy(&s_config, initData, sizeof(APPAGENT_INIT));
	if ((s_config.name == NULL) || (s_config.name[0] == 0))
		return false;

#if defined(_AIX) || defined(__sun)
   signal(SIGUSR2, DummySignalHandler);
#endif

	s_initialized = true;
	return true;
}

/**
 * Pipe listener
 */
static NamedPipeListener *s_listener = NULL;

/**
 * Start application agent
 */
void APPAGENT_EXPORTABLE AppAgentStart()
{
   if (!s_initialized || (s_listener != NULL))
      return;

   TCHAR name[64];
   _sntprintf(name, 64, _T("appagent.%s"), s_config.name);
   s_listener = NamedPipeListener::create(name, ProcessRequest, NULL, s_config.userId);
   if (s_listener != NULL)
      s_listener->start();
}

/**
 * Stop application agent
 */
void APPAGENT_EXPORTABLE AppAgentStop()
{
	if (s_initialized && (s_listener != NULL))
	{
	   s_listener->stop();
	   delete_and_null(s_listener);
	}
}

/**
 * Post event
 */
void APPAGENT_EXPORTABLE AppAgentPostEvent(int code, const TCHAR *name, const char *format, ...)
{
}
