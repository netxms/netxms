/*
** NetXMS - Network Management System
** Log Parsing Library
** Copyright (C) 2003-2014 Victor Kirhenshtein
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
#include "libnxlp.h"
#include <winevt.h>
#include <winmeta.h>

/**
 * Static data
 */
static BOOL (WINAPI *_EvtClose)(EVT_HANDLE);
static EVT_HANDLE (WINAPI *_EvtCreateRenderContext)(DWORD, LPCWSTR *, DWORD);
static BOOL (WINAPI *_EvtGetPublisherMetadataProperty)(EVT_HANDLE, EVT_PUBLISHER_METADATA_PROPERTY_ID, DWORD, DWORD, PEVT_VARIANT, PDWORD);
static BOOL (WINAPI *_EvtFormatMessage)(EVT_HANDLE, EVT_HANDLE, DWORD, DWORD, PEVT_VARIANT, DWORD, DWORD, LPWSTR, PDWORD);
static BOOL (WINAPI *_EvtNext)(EVT_HANDLE, DWORD, EVT_HANDLE *, DWORD, DWORD, PDWORD);
static EVT_HANDLE (WINAPI *_EvtOpenPublisherMetadata)(EVT_HANDLE, LPCWSTR, LPCWSTR, LCID, DWORD);
static EVT_HANDLE (WINAPI *_EvtQuery)(EVT_HANDLE, LPCWSTR, LPCWSTR, DWORD);
static BOOL (WINAPI *_EvtRender)(EVT_HANDLE, EVT_HANDLE, DWORD, DWORD, PVOID, PDWORD, PDWORD);
static EVT_HANDLE (WINAPI *_EvtSubscribe)(EVT_HANDLE, HANDLE, LPCWSTR, LPCWSTR, EVT_HANDLE, PVOID, EVT_SUBSCRIBE_CALLBACK, DWORD);

/**
 * Log metadata property
 */
static void LogMetadataProperty(EVT_HANDLE pubMetadata, EVT_PUBLISHER_METADATA_PROPERTY_ID id, const TCHAR *name)
{
	WCHAR buffer[4096];
	PEVT_VARIANT p = PEVT_VARIANT(buffer);
	DWORD size;
	if (_EvtGetPublisherMetadataProperty(pubMetadata, id, 0, sizeof(buffer), p, &size))
   {
      switch(p->Type)
      {
         case EvtVarTypeNull:
      	   LogParserTrace(5, _T("LogWatch: publisher %s: NULL"), name);
            break;
         case EvtVarTypeString:
      	   LogParserTrace(5, _T("LogWatch: publisher %s: %ls"), name, p->StringVal);
            break;
         case EvtVarTypeAnsiString:
      	   LogParserTrace(5, _T("LogWatch: publisher %s: %hs"), name, p->AnsiStringVal);
            break;
         default:
      	   LogParserTrace(5, _T("LogWatch: publisher %s: (variant type %d)"), name, (int)p->Type);
            break;
      }
   }
}

/**
 * Subscription callback
 */
static DWORD WINAPI SubscribeCallback(EVT_SUBSCRIBE_NOTIFY_ACTION action, PVOID userContext, EVT_HANDLE event)
{
	WCHAR buffer[4096], *msg = buffer;
	DWORD reqSize, propCount = 0;
	EVT_HANDLE pubMetadata = NULL;
	BOOL success;
	TCHAR publisherName[MAX_PATH];

	// Create render context for event values - we need provider name,
	// event id, and severity level
	static PCWSTR eventProperties[] = { L"Event/System/Provider/@Name", L"Event/System/EventID", L"Event/System/Level", L"Event/System/Keywords" };
	EVT_HANDLE renderContext = _EvtCreateRenderContext(4, eventProperties, EvtRenderContextValues);
	if (renderContext == NULL)
	{
		LogParserTrace(5, _T("LogWatch: Call to EvtCreateRenderContext failed: %s"),
							    GetSystemErrorText(GetLastError(), (TCHAR *)buffer, 4096));
		return 0;
	}

	// Get event values
	if (!_EvtRender(renderContext, event, EvtRenderEventValues, 4096, buffer, &reqSize, &propCount))
	{
		LogParserTrace(5, _T("LogWatch: Call to EvtRender failed: %s"),
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
		LogParserTrace(6, _T("LogWatch: publisher name is %s"), publisherName);
	}
	else
	{
		LogParserTrace(6, _T("LogWatch: unable to get publisher name from event"));
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
            level = 0x0100;
            break;
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

	// Keywords
	if (values[3].UInt64Val & WINEVENT_KEYWORD_AUDIT_SUCCESS)
		level = EVENTLOG_AUDIT_SUCCESS;
	else if (values[3].UInt64Val & WINEVENT_KEYWORD_AUDIT_FAILURE)
		level = EVENTLOG_AUDIT_FAILURE;

	// Open publisher metadata
	pubMetadata = _EvtOpenPublisherMetadata(NULL, values[0].StringVal, NULL, MAKELCID(MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), SORT_DEFAULT), 0);
	if (pubMetadata == NULL)
	{
		LogParserTrace(5, _T("LogWatch: Call to EvtOpenPublisherMetadata failed: %s"),
							    GetSystemErrorText(GetLastError(), (TCHAR *)buffer, 4096));
		goto cleanup;
	}

	// Format message text
	success = _EvtFormatMessage(pubMetadata, event, 0, 0, NULL, EvtFormatMessageEvent, 4096, buffer, &reqSize);
	if (!success)
	{
		if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
		{
			LogParserTrace(5, _T("LogWatch: Call to EvtFormatMessage failed: %s"),
								    GetSystemErrorText(GetLastError(), (TCHAR *)buffer, 4096));
			LogMetadataProperty(pubMetadata, EvtPublisherMetadataMessageFilePath, _T("message file"));
			LogMetadataProperty(pubMetadata, EvtPublisherMetadataParameterFilePath, _T("parameter file"));
			LogMetadataProperty(pubMetadata, EvtPublisherMetadataResourceFilePath, _T("resource file"));
			goto cleanup;
		}
		msg = (WCHAR *)malloc(sizeof(WCHAR) * reqSize);
		success = _EvtFormatMessage(NULL, event, 0, 0, NULL, EvtFormatMessageEvent, reqSize, msg, &reqSize);
		if (!success)
		{
			LogParserTrace(5, _T("LogWatch: Call to EvtFormatMessage failed: %s"),
								    GetSystemErrorText(GetLastError(), (TCHAR *)buffer, 4096));
			LogMetadataProperty(pubMetadata, EvtPublisherMetadataMessageFilePath, _T("message file"));
			LogMetadataProperty(pubMetadata, EvtPublisherMetadataParameterFilePath, _T("parameter file"));
			LogMetadataProperty(pubMetadata, EvtPublisherMetadataResourceFilePath, _T("resource file"));
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

   ((LogParser *)userContext)->saveLastProcessedRecordTimestamp(time(NULL));

cleanup:
	if (pubMetadata != NULL)
		_EvtClose(pubMetadata);
	_EvtClose(renderContext);
	if (msg != buffer)
		free(msg);
	return 0;
}

/**
 * Event log parser thread
 */
bool LogParser::monitorEventLogV6(CONDITION stopCondition)
{
   bool success;

   // Read old records if needed
   if (m_marker != NULL)
   {
      time_t startTime = readLastProcessedRecordTimestamp();
      time_t now = time(NULL);
      if (startTime < now)
      {
		   LogParserTrace(1, _T("LogWatch: reading old events between %I64d and %I64d"), (INT64)startTime, (INT64)now);

         WCHAR query[256];
         _snwprintf(query, 256, L"*[System/TimeCreated[timediff(@SystemTime) < %I64d]]", (INT64)(time(NULL) - startTime) * 1000LL);
         EVT_HANDLE handle = _EvtQuery(NULL, &m_fileName[1], query, EvtQueryChannelPath | EvtQueryForwardDirection);
         if (handle != NULL)
         {
            EVT_HANDLE events[64];
            DWORD count;
            while(_EvtNext(handle, 64, events, 5000, 0, &count))
            {
               for(DWORD i = 0; i < count; i++)
               {
                  SubscribeCallback(EvtSubscribeActionDeliver, this, events[i]);
                  _EvtClose(events[i]);
               }
            }
   		   _EvtClose(handle);
         }
         else
         {
            TCHAR buffer[1024];
   		   LogParserTrace(1, _T("LogWatch: EvtQuery failed (%s)"), GetSystemErrorText(GetLastError(), buffer, 1024));
         }
      }
   }

#ifdef UNICODE
	EVT_HANDLE handle = _EvtSubscribe(NULL, NULL, &m_fileName[1], NULL, NULL, this, SubscribeCallback, EvtSubscribeToFutureEvents);
#else
	WCHAR channel[MAX_PATH];
	MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, &m_fileName[1], -1, channel, MAX_PATH);
	channel[MAX_PATH - 1] = 0;
	EVT_HANDLE handle = _EvtSubscribe(NULL, NULL, channel, NULL, NULL, this, SubscribeCallback, EvtSubscribeToFutureEvents);
#endif
	if (handle != NULL)
	{
		LogParserTrace(1, _T("LogWatch: Start watching event log \"%s\" (using EvtSubscribe)"), &m_fileName[1]);
		setStatus(LPS_RUNNING);
		WaitForSingleObject(stopCondition, INFINITE);
		LogParserTrace(1, _T("LogWatch: Stop watching event log \"%s\" (using EvtSubscribe)"), &m_fileName[1]);
		_EvtClose(handle);
      success = true;
	}
	else
	{
		TCHAR buffer[1024];
		LogParserTrace(0, _T("LogWatch: Unable to open event log \"%s\" with EvtSubscribe(): %s"),
		               &m_fileName[1], GetSystemErrorText(GetLastError(), buffer, 1024));
		setStatus(_T("EVENT LOG SUBSCRIBE FAILED"));
      success = false;
	}
   return success;
}

/**
 * Initialize event log reader for Windows Vista and later
 */
bool InitEventLogParsersV6()
{
	HMODULE module = LoadLibrary(_T("wevtapi.dll"));
	if (module == NULL)
	{
		TCHAR buffer[1024];
		LogParserTrace(1, _T("LogWatch: cannot load wevtapi.dll: %s"), GetSystemErrorText(GetLastError(), buffer, 1024));
		return false;
	}

	_EvtClose = (BOOL (WINAPI *)(EVT_HANDLE))GetProcAddress(module, "EvtClose");
	_EvtCreateRenderContext = (EVT_HANDLE (WINAPI *)(DWORD, LPCWSTR *, DWORD))GetProcAddress(module, "EvtCreateRenderContext");
	_EvtGetPublisherMetadataProperty = (BOOL (WINAPI *)(EVT_HANDLE, EVT_PUBLISHER_METADATA_PROPERTY_ID, DWORD, DWORD, PEVT_VARIANT, PDWORD))GetProcAddress(module, "EvtGetPublisherMetadataProperty");
	_EvtFormatMessage = (BOOL (WINAPI *)(EVT_HANDLE, EVT_HANDLE, DWORD, DWORD, PEVT_VARIANT, DWORD, DWORD, LPWSTR, PDWORD))GetProcAddress(module, "EvtFormatMessage");
   _EvtNext = (BOOL (WINAPI *)(EVT_HANDLE, DWORD, EVT_HANDLE *, DWORD, DWORD, PDWORD))GetProcAddress(module, "EvtNext");
   _EvtOpenPublisherMetadata = (EVT_HANDLE (WINAPI *)(EVT_HANDLE, LPCWSTR, LPCWSTR, LCID, DWORD))GetProcAddress(module, "EvtOpenPublisherMetadata");
	_EvtQuery = (EVT_HANDLE (WINAPI *)(EVT_HANDLE, LPCWSTR, LPCWSTR, DWORD))GetProcAddress(module, "EvtQuery");
	_EvtRender = (BOOL (WINAPI *)(EVT_HANDLE, EVT_HANDLE, DWORD, DWORD, PVOID, PDWORD, PDWORD))GetProcAddress(module, "EvtRender");
	_EvtSubscribe = (EVT_HANDLE (WINAPI *)(EVT_HANDLE, HANDLE, LPCWSTR, LPCWSTR, EVT_HANDLE, PVOID, EVT_SUBSCRIBE_CALLBACK, DWORD))GetProcAddress(module, "EvtSubscribe");

	return (_EvtClose != NULL) && (_EvtCreateRenderContext != NULL) &&
          (_EvtGetPublisherMetadataProperty != NULL) && (_EvtFormatMessage != NULL) && 
	       (_EvtNext != NULL) && (_EvtOpenPublisherMetadata != NULL) &&
          (_EvtQuery != NULL) && (_EvtRender != NULL) && (_EvtSubscribe != NULL);
}
