/*
** NetXMS LogWatch subagent
** Copyright (C) 2008-2011 Victor Kirhenshtein
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
** File: wevt.cpp
**
**/

#define _WIN32_WINNT 0x0600
#include "logwatch.h"
#include <winevt.h>
#include <winmeta.h>


//
// Static data
//

static BOOL (WINAPI *_EvtClose)(EVT_HANDLE);
static EVT_HANDLE (WINAPI *_EvtCreateRenderContext)(DWORD, LPCWSTR *, DWORD);
static BOOL (WINAPI *_EvtFormatMessage)(EVT_HANDLE, EVT_HANDLE, DWORD, DWORD, PEVT_VARIANT, DWORD, DWORD, LPWSTR, PDWORD);
static EVT_HANDLE (WINAPI *_EvtOpenPublisherMetadata)(EVT_HANDLE, LPCWSTR, LPCWSTR, LCID, DWORD);
static BOOL (WINAPI *_EvtRender)(EVT_HANDLE, EVT_HANDLE, DWORD, DWORD, PVOID, PDWORD, PDWORD);
static EVT_HANDLE (WINAPI *_EvtSubscribe)(EVT_HANDLE, HANDLE, LPCWSTR, LPCWSTR, EVT_HANDLE, PVOID, EVT_SUBSCRIBE_CALLBACK, DWORD);


//
// Subscription callback
//

static DWORD WINAPI SubscribeCallback(EVT_SUBSCRIBE_NOTIFY_ACTION action, PVOID userContext, EVT_HANDLE event)
{
	WCHAR buffer[4096], *msg = buffer;
	DWORD reqSize, propCount = 0;
	EVT_HANDLE pubMetadata = NULL;
	BOOL success;
	TCHAR publisherName[MAX_PATH];

	// Create render context for event values - we need provider name,
	// event id, and severity level
	static PCWSTR eventProperties[] = { L"Event/System/Provider/@Name", L"Event/System/EventID", L"Event/System/Level" };
	EVT_HANDLE renderContext = _EvtCreateRenderContext(3, eventProperties, EvtRenderContextValues);
	if (renderContext == NULL)
	{
		AgentWriteDebugLog(5, _T("LogWatch: Call to EvtCreateRenderContext failed: %s"),
							    GetSystemErrorText(GetLastError(), (TCHAR *)buffer, 4096));
		return 0;
	}

	// Get event values
	if (!_EvtRender(renderContext, event, EvtRenderEventValues, 4096, buffer, &reqSize, &propCount))
	{
		AgentWriteDebugLog(5, _T("LogWatch: Call to EvtRender failed: %s"),
		                   GetSystemErrorText(GetLastError(), (TCHAR *)buffer, 4096));
		goto cleanup;
	}

	// Publisher name
	PEVT_VARIANT values = PEVT_VARIANT(buffer);
	if ((values[0].Type == EvtVarTypeString) && (values[0].StringVal != NULL))
	{
#ifdef UNICODE
		nx_strncpy(publisherName, values[0].StringVal, MAX_PATH);
#else
		WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR, values[0].StringVal, -1, publisherName, MAX_PATH, NULL, NULL);
#endif
		AgentWriteDebugLog(5, _T("LogWatch: publisher name is %s"), publisherName);
	}
	else
	{
		AgentWriteDebugLog(5, _T("LogWatch: unable to get publisher name from event"));
	}

	// Event id
	DWORD eventId = 0;
	if (values[1].Type == EvtVarTypeUInt16)
		eventId = values[1].UInt16Val;

	// Severity level
	DWORD level = 0;
	if (values[2].Type == EvtVarTypeByte)
	{
		switch(values[2].ByteVal)
		{
			case WINEVENT_LEVEL_CRITICAL:
			case WINEVENT_LEVEL_ERROR:
				level = EVENTLOG_ERROR_TYPE;
				break;
			case WINEVENT_LEVEL_WARNING :
				level = EVENTLOG_WARNING_TYPE;
				break;
			case WINEVENT_LEVEL_INFO:
			case WINEVENT_LEVEL_VERBOSE:
				level = EVENTLOG_INFORMATION_TYPE;
				break;
			default:
				level = EVENTLOG_INFORMATION_TYPE;
				break;
		}
	}

	// Open publisher metadata
	pubMetadata = _EvtOpenPublisherMetadata(NULL, values[0].StringVal, NULL, LOCALE_USER_DEFAULT, 0);
	if (pubMetadata == NULL)
	{
		AgentWriteDebugLog(5, _T("LogWatch: Call to EvtOpenPublisherMetadata failed: %s"),
							    GetSystemErrorText(GetLastError(), (TCHAR *)buffer, 4096));
		goto cleanup;
	}

	// Format message text
	success = _EvtFormatMessage(pubMetadata, event, 0, 0, NULL, EvtFormatMessageEvent, 4096, buffer, &reqSize);
	if (!success)
	{
		if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
		{
			AgentWriteDebugLog(5, _T("LogWatch: Call to EvtFormatMessage failed: %s"),
								    GetSystemErrorText(GetLastError(), (TCHAR *)buffer, 4096));
			goto cleanup;
		}
		msg = (WCHAR *)malloc(sizeof(WCHAR) * reqSize);
		success = _EvtFormatMessage(NULL, event, 0, 0, NULL, EvtFormatMessageEvent, reqSize, msg, &reqSize);
		if (!success)
		{
			AgentWriteDebugLog(5, _T("LogWatch: Call to EvtFormatMessage failed: %s"),
								    GetSystemErrorText(GetLastError(), (TCHAR *)buffer, 4096));
			goto cleanup;
		}
	}

#ifdef UNICODE
	((LogParser *)userContext)->matchEvent(publisherName, eventId, level, msg);
#else
	char *mbmsg = MBStringFromWideString(msg);
	((LogParser *)userContext)->matchEvent(publisherName, eventId, level, mbmsg);
	free(mbmsg);
#endif

cleanup:
	if (pubMetadata != NULL)
		_EvtClose(pubMetadata);
	_EvtClose(renderContext);
	if (msg != buffer)
		free(msg);
	return 0;
}


//
// Event log parser thread
//

THREAD_RESULT THREAD_CALL ParserThreadEventLogV6(void *arg)
{
	LogParser *parser = (LogParser *)arg;
	EVT_HANDLE handle;
	WCHAR channel[MAX_PATH];

	MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, &(parser->getFileName()[1]), -1, channel, MAX_PATH);
	channel[MAX_PATH - 1] = 0;
	handle = _EvtSubscribe(NULL, NULL, channel, NULL, NULL, arg,
	                       SubscribeCallback, EvtSubscribeToFutureEvents);
	if (handle != NULL)
	{
		AgentWriteDebugLog(1, _T("LogWatch: Start watching event log \"%s\" (using EvtSubscribe)"),
		                   &(parser->getFileName()[1]));
		parser->setStatus(LPS_RUNNING);
		WaitForSingleObject(g_hCondShutdown, INFINITE);
		AgentWriteDebugLog(1, _T("LogWatch: Stop watching event log \"%s\" (using EvtSubscribe)"),
		                   &(parser->getFileName()[1]));
		_EvtClose(handle);
	}
	else
	{
		TCHAR buffer[1024];
		AgentWriteLog(EVENTLOG_ERROR_TYPE, _T("LogWatch: Unable to open event log \"%s\" with EvtSubscribe(): %s"),
		                &(parser->getFileName()[1]), GetSystemErrorText(GetLastError(), buffer, 1024));
		parser->setStatus(_T("EVENT LOG SUBSCRIBE FAILED"));
	}

	return THREAD_OK;
}


//
// Initialize event log reader for Windows Vista and later
//

bool InitEventLogParsersV6()
{
	HMODULE module = LoadLibrary(_T("wevtapi.dll"));
	if (module == NULL)
	{
		TCHAR buffer[1024];
		AgentWriteDebugLog(1, _T("LogWatch: cannot load wevtapi.dll: %s"), GetSystemErrorText(GetLastError(), buffer, 1024));
		return false;
	}

	_EvtClose = (BOOL (WINAPI *)(EVT_HANDLE))GetProcAddress(module, "EvtClose");
	_EvtCreateRenderContext = (EVT_HANDLE (WINAPI *)(DWORD, LPCWSTR *, DWORD))GetProcAddress(module, "EvtCreateRenderContext");
	_EvtFormatMessage = (BOOL (WINAPI *)(EVT_HANDLE, EVT_HANDLE, DWORD, DWORD, PEVT_VARIANT, DWORD, DWORD, LPWSTR, PDWORD))GetProcAddress(module, "EvtFormatMessage");
	_EvtOpenPublisherMetadata = (EVT_HANDLE (WINAPI *)(EVT_HANDLE, LPCWSTR, LPCWSTR, LCID, DWORD))GetProcAddress(module, "EvtOpenPublisherMetadata");
	_EvtRender = (BOOL (WINAPI *)(EVT_HANDLE, EVT_HANDLE, DWORD, DWORD, PVOID, PDWORD, PDWORD))GetProcAddress(module, "EvtRender");
	_EvtSubscribe = (EVT_HANDLE (WINAPI *)(EVT_HANDLE, HANDLE, LPCWSTR, LPCWSTR, EVT_HANDLE, PVOID, EVT_SUBSCRIBE_CALLBACK, DWORD))GetProcAddress(module, "EvtSubscribe");

	return (_EvtClose != NULL) && (_EvtCreateRenderContext != NULL) && (_EvtFormatMessage != NULL) && 
	       (_EvtOpenPublisherMetadata != NULL) && (_EvtRender != NULL) && (_EvtSubscribe != NULL);
}
