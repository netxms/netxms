/*
** NetXMS - Network Management System
** Log Parsing Library
** Copyright (C) 2003-2017 Victor Kirhenshtein
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

#include "libnxlp.h"
#include <winevt.h>
#include <winmeta.h>

/**
 * Log metadata property
 */
static void LogMetadataProperty(EVT_HANDLE pubMetadata, EVT_PUBLISHER_METADATA_PROPERTY_ID id, const TCHAR *name)
{
	WCHAR buffer[4096];
	PEVT_VARIANT p = PEVT_VARIANT(buffer);
	DWORD size;
	if (EvtGetPublisherMetadataProperty(pubMetadata, id, 0, sizeof(buffer), p, &size))
   {
      switch(p->Type)
      {
         case EvtVarTypeNull:
      	   nxlog_debug_tag(DEBUG_TAG, 5, _T("Publisher %s: NULL"), name);
            break;
         case EvtVarTypeString:
      	   nxlog_debug_tag(DEBUG_TAG, 5, _T("Publisher %s: %ls"), name, p->StringVal);
            break;
         case EvtVarTypeAnsiString:
      	   nxlog_debug_tag(DEBUG_TAG, 5, _T("Publisher %s: %hs"), name, p->AnsiStringVal);
            break;
         default:
      	   nxlog_debug_tag(DEBUG_TAG, 5, _T("Publisher %s: (variant type %d)"), name, (int)p->Type);
            break;
      }
   }
}

/**
 * Extract variables from event
 */
static StringList *ExtractVariables(EVT_HANDLE event)
{
   TCHAR buffer[4096];
   void *renderBuffer = buffer;

   static PCWSTR eventProperties[] = { L"Event/EventData/Data[1]" };
   EVT_HANDLE renderContext = EvtCreateRenderContext(0, nullptr, EvtRenderContextUser);
   if (renderContext == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("ExtractVariables: Call to EvtCreateRenderContext failed: %s"),
         GetSystemErrorText(GetLastError(), (TCHAR *)buffer, 4096));
      return nullptr;
   }

   StringList *variables = nullptr;

   // Get event values
   DWORD reqSize, propCount = 0;
   BOOL renderSuccess = EvtRender(renderContext, event, EvtRenderEventValues, sizeof(buffer), renderBuffer, &reqSize, &propCount);
   if (!renderSuccess && (GetLastError() == ERROR_INSUFFICIENT_BUFFER))
   {
      nxlog_debug_tag(DEBUG_TAG, 6, _T("ExtractVariables: Call to EvtRender requires larger buffer (%u bytes)"), reqSize);
      renderBuffer = MemAlloc(reqSize);
      renderSuccess = EvtRender(renderContext, event, EvtRenderEventValues, reqSize, renderBuffer, &reqSize, &propCount);
   }
   if (!renderSuccess)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("ExtractVariables: Call to EvtRender failed: %s"), GetSystemErrorText(GetLastError(), (TCHAR *)buffer, 4096));
      goto cleanup;
   }

   if (propCount > 0)
   {
      variables = new StringList();
      PEVT_VARIANT values = PEVT_VARIANT(renderBuffer);
      for (DWORD i = 0; i < propCount; i++)
      {
         switch (values[i].Type)
         {
            case EvtVarTypeString:
               variables->add(values[i].StringVal);
               break;
            case EvtVarTypeAnsiString:
               variables->addMBString(values[i].AnsiStringVal);
               break;
            case EvtVarTypeSByte:
               variables->add((INT32)values[i].SByteVal);
               break;
            case EvtVarTypeByte:
               variables->add((INT32)values[i].ByteVal);
               break;
            case EvtVarTypeInt16:
               variables->add((INT32)values[i].Int16Val);
               break;
            case EvtVarTypeUInt16:
               variables->add((UINT32)values[i].UInt16Val);
               break;
            case EvtVarTypeInt32:
               variables->add(values[i].Int32Val);
               break;
            case EvtVarTypeUInt32:
               variables->add(values[i].UInt32Val);
               break;
            case EvtVarTypeInt64:
               variables->add(values[i].Int64Val);
               break;
            case EvtVarTypeUInt64:
               variables->add(values[i].UInt64Val);
               break;
            case EvtVarTypeSingle:
               variables->add(values[i].SingleVal);
               break;
            case EvtVarTypeDouble:
               variables->add(values[i].DoubleVal);
               break;
            case EvtVarTypeBoolean:
               variables->add(values[i].BooleanVal ? _T("True") : _T("False"));
               break;
            default:
               variables->add(_T(""));
               break;
         }
      }
   }

cleanup:
   EvtClose(renderContext);
   if (renderBuffer != buffer)
      MemFree(renderBuffer);
   return variables;
}

/**
 * Subscription callback
 */
static DWORD WINAPI SubscribeCallback(EVT_SUBSCRIBE_NOTIFY_ACTION action, PVOID userContext, EVT_HANDLE event)
{
   if (action != EvtSubscribeActionDeliver)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("Subscribe callback called with action = %d"), action);
      return 0;
   }

	WCHAR buffer[4096], *msg = buffer;
   void *renderBuffer = buffer;
	DWORD reqSize, propCount = 0;
	EVT_HANDLE pubMetadata = NULL;
	BOOL success;
	TCHAR publisherName[MAX_PATH];

	// Create render context for event values - we need provider name,
	// event id, and severity level
	static PCWSTR eventProperties[] =
   {
      L"Event/System/Provider/@Name",
      L"Event/System/EventID",
      L"Event/System/Level",
      L"Event/System/Keywords",
      L"Event/System/EventRecordID",
      L"Event/System/TimeCreated/@SystemTime"
   };
	EVT_HANDLE renderContext = EvtCreateRenderContext(6, eventProperties, EvtRenderContextValues);
	if (renderContext == nullptr)
	{
		nxlog_debug_tag(DEBUG_TAG, 5, _T("Call to EvtCreateRenderContext failed: %s"), GetSystemErrorText(GetLastError(), buffer, 4096));
		return 0;
	}

	// Get event values
   BOOL renderSuccess = EvtRender(renderContext, event, EvtRenderEventValues, sizeof(buffer), renderBuffer, &reqSize, &propCount);
   if (!renderSuccess && (GetLastError() == ERROR_INSUFFICIENT_BUFFER))
   {
      nxlog_debug_tag(DEBUG_TAG, 6, _T("Call to EvtRender requires larger buffer (%u bytes)"), reqSize);
      renderBuffer = MemAlloc(reqSize);
      renderSuccess = EvtRender(renderContext, event, EvtRenderEventValues, reqSize, renderBuffer, &reqSize, &propCount);
   }
   if (!renderSuccess)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("Call to EvtRender failed: %s"), GetSystemErrorText(GetLastError(), buffer, 4096));
		goto cleanup;
	}

	// Publisher name
	PEVT_VARIANT values = PEVT_VARIANT(renderBuffer);
	if ((values[0].Type == EvtVarTypeString) && (values[0].StringVal != NULL))
	{
#ifdef UNICODE
		wcslcpy(publisherName, values[0].StringVal, MAX_PATH);
#else
		WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR, values[0].StringVal, -1, publisherName, MAX_PATH, NULL, NULL);
#endif
      static_cast<LogParser*>(userContext)->trace(7, _T("Publisher name is %s"), publisherName);
   }
	else
	{
      static_cast<LogParser*>(userContext)->trace(7, _T("Unable to get publisher name from event"));
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

   // Record ID
   uint64_t recordId = 0;
   if ((values[4].Type == EvtVarTypeUInt64) || (values[4].Type == EvtVarTypeInt64))
      recordId = values[4].UInt64Val;
   else if ((values[4].Type == EvtVarTypeUInt32) || (values[4].Type == EvtVarTypeInt32))
      recordId = values[4].UInt32Val;

   // Time created
   time_t timestamp;
   if (values[5].Type == EvtVarTypeFileTime)
   {
      timestamp = FileTimeToUnixTime(values[5].FileTimeVal);
   }
   else if (values[5].Type == EvtVarTypeSysTime)
   {
      FILETIME ft;
      SystemTimeToFileTime(values[5].SysTimeVal, &ft);
      timestamp = FileTimeToUnixTime(ft);
   }
   else
   {
      timestamp = time(NULL);
      static_cast<LogParser*>(userContext)->trace(7, _T("Cannot get timestamp from event (values[5].Type == %d)"), values[5].Type);
   }

	// Open publisher metadata
	pubMetadata = EvtOpenPublisherMetadata(NULL, values[0].StringVal, nullptr, MAKELCID(MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), SORT_DEFAULT), 0);
	if (pubMetadata == nullptr)
	{
		nxlog_debug_tag(DEBUG_TAG, 5, _T("Call to EvtOpenPublisherMetadata failed: %s"), GetSystemErrorText(GetLastError(), (TCHAR *)buffer, 4096));
		goto cleanup;
	}

	// Format message text
	success = EvtFormatMessage(pubMetadata, event, 0, 0, nullptr, EvtFormatMessageEvent, 4096, buffer, &reqSize);
	if (!success)
	{
      DWORD error = GetLastError();
		if ((error != ERROR_INSUFFICIENT_BUFFER) && (error != ERROR_EVT_UNRESOLVED_VALUE_INSERT))
		{
			nxlog_debug_tag(DEBUG_TAG, 5, _T("Call to EvtFormatMessage failed: error %u (%s)"), error, GetSystemErrorText(error, (TCHAR *)buffer, 4096));
			LogMetadataProperty(pubMetadata, EvtPublisherMetadataMessageFilePath, _T("message file"));
			LogMetadataProperty(pubMetadata, EvtPublisherMetadataParameterFilePath, _T("parameter file"));
			LogMetadataProperty(pubMetadata, EvtPublisherMetadataResourceFilePath, _T("resource file"));
			goto cleanup;
		}
      if (error == ERROR_INSUFFICIENT_BUFFER)
      {
		   msg = MemAllocStringW(reqSize);
		   success = EvtFormatMessage(pubMetadata, event, 0, 0, NULL, EvtFormatMessageEvent, reqSize, msg, &reqSize);
		   if (!success)
		   {
            error = GetLastError();
            if (error != ERROR_EVT_UNRESOLVED_VALUE_INSERT)
            {
			      nxlog_debug_tag(DEBUG_TAG, 5, _T("Retry call to EvtFormatMessage failed: error %u (%s)"), error, GetSystemErrorText(error, (TCHAR *)buffer, 4096));
			      LogMetadataProperty(pubMetadata, EvtPublisherMetadataMessageFilePath, _T("message file"));
			      LogMetadataProperty(pubMetadata, EvtPublisherMetadataParameterFilePath, _T("parameter file"));
			      LogMetadataProperty(pubMetadata, EvtPublisherMetadataResourceFilePath, _T("resource file"));
			      goto cleanup;
            }
		   }
      }
	}

   StringList *variables = ExtractVariables(event);
#ifdef UNICODE
   static_cast<LogParser*>(userContext)->matchEvent(publisherName, eventId, level, msg, variables, recordId, 0, timestamp);
#else
	char *mbmsg = MBStringFromWideString(msg);
   static_cast<LogParser*>(userContext)->matchEvent(publisherName, eventId, level, mbmsg, variables, recordId, 0, timestamp);
	MemFree(mbmsg);
#endif
   delete variables;

   static_cast<LogParser*>(userContext)->saveLastProcessedRecordTimestamp(timestamp);

cleanup:
	if (pubMetadata != NULL)
		EvtClose(pubMetadata);
	EvtClose(renderContext);
	if (msg != buffer)
		MemFree(msg);
   if (renderBuffer != buffer)
      MemFree(renderBuffer);
   return 0;
}

/**
 * Event log parser thread
 */
bool LogParser::monitorEventLog(const TCHAR *markerPrefix)
{
   if (markerPrefix != nullptr)
   {
      size_t len = _tcslen(markerPrefix) + _tcslen(m_fileName) + 2;
      m_marker = MemAllocString(len);
      _sntprintf(m_marker, len, _T("%s.%s"), markerPrefix, &m_fileName[1]);
      nxlog_debug_tag(DEBUG_TAG, 3, _T("Created marker %s"), m_marker);
   }

   bool success;

   // Read old records if needed
   if (m_marker != nullptr)
   {
      time_t startTime = readLastProcessedRecordTimestamp();
      time_t now = time(nullptr);
      if (startTime < now)
      {
		   nxlog_debug_tag(DEBUG_TAG, 1, _T("Reading old events between %I64d and %I64d from %s"), (INT64)startTime, (INT64)now, &m_fileName[1]);

         WCHAR query[256];
         _snwprintf(query, 256, L"*[System/TimeCreated[timediff(@SystemTime) < %I64d]]", (INT64)(now - startTime) * 1000LL);
         nxlog_debug_tag(DEBUG_TAG, 4, _T("Event log query: \"%s\""), query);
         EVT_HANDLE handle = EvtQuery(nullptr, &m_fileName[1], query, EvtQueryChannelPath | EvtQueryForwardDirection);
         if (handle != nullptr)
         {
            EVT_HANDLE events[64];
            DWORD count;
            while(EvtNext(handle, 64, events, 5000, 0, &count))
            {
               for(DWORD i = 0; i < count; i++)
               {
                  SubscribeCallback(EvtSubscribeActionDeliver, this, events[i]);
                  EvtClose(events[i]);
               }
            }
   		   EvtClose(handle);
         }
         else
         {
            TCHAR buffer[1024];
   		   nxlog_debug_tag(DEBUG_TAG, 1, _T("EvtQuery failed (%s)"), GetSystemErrorText(GetLastError(), buffer, 1024));
         }
      }
   }

#ifdef UNICODE
	EVT_HANDLE handle = EvtSubscribe(NULL, NULL, &m_fileName[1], NULL, NULL, this, SubscribeCallback, EvtSubscribeToFutureEvents);
#else
	WCHAR channel[MAX_PATH];
	MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, &m_fileName[1], -1, channel, MAX_PATH);
	channel[MAX_PATH - 1] = 0;
	EVT_HANDLE handle = EvtSubscribe(NULL, NULL, channel, NULL, NULL, this, SubscribeCallback, EvtSubscribeToFutureEvents);
#endif
	if (handle != NULL)
	{
		nxlog_debug_tag(DEBUG_TAG, 1, _T("Start watching event log \"%s\""), &m_fileName[1]);
		setStatus(LPS_RUNNING);
		m_stopCondition.wait(INFINITE);
		nxlog_debug_tag(DEBUG_TAG, 1, _T("Stop watching event log \"%s\""), &m_fileName[1]);
		EvtClose(handle);
      success = true;
	}
	else
	{
		TCHAR buffer[1024];
		nxlog_debug_tag(DEBUG_TAG, 0, _T("Unable to open event log \"%s\" with EvtSubscribe(): %s"),
		               &m_fileName[1], GetSystemErrorText(GetLastError(), buffer, 1024));
		setStatus(LPS_EVT_SUBSCRIBE_ERROR);
      success = false;
	}
   return success;
}
