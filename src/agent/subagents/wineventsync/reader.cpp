/*
** Windows Event Log Synchronization NetXMS subagent
** Copyright (C) 2020-2025 Raden Solutions
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
** File: wineventsync.cpp
**
**/

#include "wineventsync.h"

/**
 * Request ID
 */
static VolatileCounter64 s_requestId = static_cast<int64_t>(time(nullptr)) << 24;

/**
 * Parse source configuration
 */
static void ParseSources(const TCHAR *name, Config *config, const TCHAR *path, bool include, StringList *list)
{
   ConfigEntry *e = config->getEntry(path);
   if (e == nullptr)
      return;

   for (int i = 0; i < e->getValueCount(); i++)
   {
      const TCHAR *source = e->getValue(i);
      list->add(source);
      nxlog_debug_tag(DEBUG_TAG, 6,
         include ?
            _T("Explicitly including source \"%s\" into log \"%s\" synchronization") :
            _T("Excluding source \"%s\" from log \"%s\" synchronization"),
         source, name);
   }
}

/**
 *  Parse event range
 */
static bool ParseRange(const wchar_t *text, Range *range)
{
   wchar_t buffer[256];
   wcslcpy(buffer, text, 256);

   bool success = false;

   wchar_t* p = wcschr(buffer, L'-');
   if (p != nullptr)
   {
      *p = 0;
      p++;
      TrimW(buffer);
      TrimW(p);
      TCHAR* eptr1, * eptr2;
      range->start = wcstoul(buffer, &eptr1, 0);
      range->end = wcstoul(p, &eptr2, 0);
      if ((*eptr1 == 0) && (*eptr2 == 0) && (range->start <= range->end))
      {
         success = true;
      }
   }
   else
   {
      Trim(buffer);

      TCHAR* eptr;
      uint32_t value = _tcstoul(buffer, &eptr, 0);
      if (*eptr == 0)
      {
         range->start = range->end = value;
         success = true;
      }
   }

   return success;
}

/**
 * Parse event range configuration
 */
static void ParseEvents(const TCHAR *name, Config *config, const TCHAR *path, bool include, StructArray<Range> *list)
{
   ConfigEntry *e = config->getEntry(path);
   if (e == nullptr)
      return;

   for (int i = 0; i < e->getValueCount(); i++)
   {
      Range range;
      if (ParseRange(e->getValue(i), &range))
      {
         list->add(&range);
         if (range.start != range.end)
         {
            nxlog_debug_tag(DEBUG_TAG, 6,
               include ?
               _T("Explicitly including event range %u-%u into log \"%s\" synchronization") :
               _T("Excluding event range %u-%u from log \"%s\" synchronization"),
               range.start, range.end, name);
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG, 6,
               include ?
               _T("Explicitly including event %u into log \"%s\" synchronization") :
               _T("Excluding event %u from log \"%s\" synchronization"),
               range.start, name);
         }
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG, 6, _T("Invalid event code range \"%s\" in configuration"), e->getValue(i));
      }
   }
}

/**
 * Parse severity filter definition (either bit mask or list of severity names)
 */
static uint32_t ParseSeverityFilter(const wchar_t* severityFilter)
{
   wchar_t *eptr;
   uint32_t severityMask = wcstol(severityFilter, &eptr, 0);
   if (*eptr != 0)
   {
      // Cannot parse severity filter as number, try to parse as text
      severityMask = 0;
      StringList parts = String(severityFilter).split(_T(","));
      for (int i = 0; i < parts.size(); i++)
      {
         const TCHAR *p = parts.get(i);
         if (!_tcsicmp(p, _T("critical")))
            severityMask |= 0x100;
         else if (!_tcsicmp(p, _T("error")))
            severityMask |= EVENTLOG_ERROR_TYPE;
         else if (!_tcsicmp(p, _T("warning")))
            severityMask |= EVENTLOG_WARNING_TYPE;
         else if (!_tcsicmp(p, _T("info")) || !_tcsicmp(p, _T("information")))
            severityMask |= EVENTLOG_INFORMATION_TYPE;
         else if (!_tcsicmp(p, _T("AuditSuccess")))
            severityMask |= EVENTLOG_AUDIT_SUCCESS;
         else if (!_tcsicmp(p, _T("AuditFailure")))
            severityMask |= EVENTLOG_AUDIT_FAILURE;
      }
   }
   return severityMask;
}

/**
 * Create new reader
 */
EventLogReader::EventLogReader(const TCHAR *name, Config *config) : m_stopCondition(true), m_filters(0, 32)
{
   m_thread = INVALID_THREAD_HANDLE;
   m_name = MemCopyString(name);
   m_messageId = 1;

   StringBuffer path(L"/WinEventSync/");
   path.append(m_name);
   path.append(L"/IncludeSource");
   ParseSources(m_name, config, path, true, &m_includedSources);
   path.shrink(13);
   path.append(L"ExcludeSource");
   ParseSources(m_name, config, path, false, &m_excludedSources);
   path.shrink(13);
   path.append(L"IncludeEvent");
   ParseEvents(m_name, config, path, true, &m_includedEvents);
   path.shrink(12);
   path.append(L"ExcludeEvent");
   ParseEvents(m_name, config, path, false, &m_excludedEvents);
   path.shrink(12);

   path.append(L"SeverityFilter");
   const TCHAR *severityFilter = config->getValue(path);
   m_severityFilter = (severityFilter != nullptr) ? ParseSeverityFilter(severityFilter) : 0xFF;
   nxlog_debug_tag(DEBUG_TAG, 3, _T("Using default severity filter 0x%02X for reader \"%s\""), m_severityFilter, m_name);

   path.shrink(14);
   path.append(L"Filter");
   ConfigEntry *filters = config->getEntry(path);
   if (filters != nullptr)
   {
      for (int i = 0; i < filters->getValueCount(); i++)
      {
         Filter filter;
         memset(&filter, 0, sizeof(Filter));

         // Parse filter definition in form
         // action:source:id:severity
         StringList fields = String(filters->getValue(i)).split(L":", true);
         if (!wcsicmp(fields.get(0), L"accept"))
         {
            filter.accept = true;
         }
         else if (!wcsicmp(fields.get(0), L"reject"))
         {
            filter.accept = false;
         }
         else
         {
            nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, L"Invalid action in event filter \"%s\"", filters->getValue(i));
            continue;
         }

         if ((fields.size() > 1) && (*fields.get(1) != 0) && wcscmp(fields.get(1), L"*"))
         {
            filter.source = MemCopyString(fields.get(1));
         }

         if ((fields.size() > 2) && (*fields.get(2) != 0) && wcscmp(fields.get(2), L"*"))
         {
            if (!ParseRange(fields.get(2), &filter.eventId))
            {
               nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, L"Invalid ID range in event filter \"%s\"", filters->getValue(i));
               continue;
            }
         }
         else
         {
            filter.eventId.end = 0xFFFFFFFF;
         }

         if ((fields.size() > 3) && (*fields.get(3) != 0) && wcscmp(fields.get(3), L"*"))
         {
            filter.severity = ParseSeverityFilter(fields.get(3));
         }
         else
         {
            filter.severity = 0xFF;
         }

         m_filters.add(filter);
         nxlog_debug_tag(DEBUG_TAG, 3, L"Added filter \"%s:%s:%u-%u:0x%02X\" for reader \"%s\"", filter.accept ? L"accept" : L"reject",
            (filter.source != nullptr) ? filter.source : L"*", filter.eventId.start, filter.eventId.end, filter.severity, m_name);
      }
   }
}

/**
 * Reader destructor
 */
EventLogReader::~EventLogReader()
{
   MemFree(m_name);
   for (int i = 0; i < m_filters.size(); i++)
      MemFree(m_filters.get(i)->source);
}

/**
 * Start reader
 */
void EventLogReader::start()
{
   m_thread = ThreadCreateEx(this, &EventLogReader::run);
}

/**
 * Stop reader
 */
void EventLogReader::stop()
{
   nxlog_debug_tag(DEBUG_TAG, 4, _T("Waiting for event log reader \"%s\" to stop"), m_name);
   m_stopCondition.set();
   ThreadJoin(m_thread);
}

/**
 * Main loop
 */
void EventLogReader::run()
{
   // Create render context for event values - we need provider name,
   // event id, and severity level
   static PCWSTR eventProperties[] =
   {
      L"Event/System/Provider/@Name",
      L"Event/System/EventID",
      L"Event/System/Level",
      L"Event/System/Keywords",
      L"Event/System/TimeCreated/@SystemTime"
   };
   m_renderContext = EvtCreateRenderContext(5, eventProperties, EvtRenderContextValues);
   if (m_renderContext == nullptr)
   {
      TCHAR buffer[1024];
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Cannot create event render context (%s)"),
            GetSystemErrorText(GetLastError(), buffer, 1024));
      return;
   }

   EVT_HANDLE handle = EvtSubscribe(nullptr, nullptr, m_name, nullptr, nullptr, this, subscribeCallback, EvtSubscribeToFutureEvents);
   if (handle == nullptr)
   {
      TCHAR buffer[1024];
      nxlog_write_tag(NXLOG_ERROR, DEBUG_TAG, _T("Cannot open event log \"%s\" (%s)"),
         m_name, GetSystemErrorText(GetLastError(), buffer, 1024));
      EvtClose(m_renderContext);
      return;
   }

   nxlog_debug_tag(DEBUG_TAG, 2, _T("Event log reader \"%s\" started"), m_name);
   m_stopCondition.wait(INFINITE);
   EvtClose(handle);
   EvtClose(m_renderContext);
   nxlog_debug_tag(DEBUG_TAG, 2, _T("Event log reader \"%s\" stopped"), m_name);
}

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
      switch (p->Type)
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
 * Event subscribe callback
 */
DWORD WINAPI EventLogReader::subscribeCallback(EVT_SUBSCRIBE_NOTIFY_ACTION action, PVOID context, EVT_HANDLE event)
{
   EventLogReader *reader = static_cast<EventLogReader*>(context);

   WCHAR xmlBuffer[8192], *xml = xmlBuffer;
   WCHAR buffer[4096], *messageText = buffer;
   void *renderBuffer = buffer;
   EVT_HANDLE pubMetadata = nullptr;

   // Get event values
   DWORD reqSize, propCount = 0;
   BOOL success = EvtRender(reader->m_renderContext, event, EvtRenderEventValues, sizeof(buffer), renderBuffer, &reqSize, &propCount);
   if (!success && (GetLastError() == ERROR_INSUFFICIENT_BUFFER))
   {
      nxlog_debug_tag(DEBUG_TAG, 6, _T("Call to EvtRender requires larger buffer (%u bytes)"), reqSize);
      renderBuffer = MemAlloc(reqSize);
      success = EvtRender(reader->m_renderContext, event, EvtRenderEventValues, reqSize, renderBuffer, &reqSize, &propCount);
   }
   if (!success)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("Call to EvtRender failed: %s"), GetSystemErrorText(GetLastError(), buffer, 4096));
      goto cleanup;
   }

   // Publisher name
   TCHAR publisherName[MAX_PATH];
   PEVT_VARIANT values = PEVT_VARIANT(renderBuffer);
   if ((values[0].Type == EvtVarTypeString) && (values[0].StringVal != NULL))
   {
#ifdef UNICODE
      wcslcpy(publisherName, values[0].StringVal, MAX_PATH);
#else
      WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK | WC_DEFAULTCHAR, values[0].StringVal, -1, publisherName, MAX_PATH, NULL, NULL);
#endif
      nxlog_debug_tag(DEBUG_TAG, 7, _T("Publisher name is %s"), publisherName);
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 7, _T("Unable to get publisher name from event"));
   }

   // Check event source against filters
   bool explicitlyIncluded = false;
   for (int i = 0; i < reader->m_includedSources.size(); i++)
   {
      if (MatchString(reader->m_includedSources.get(i), publisherName, false))
      {
         explicitlyIncluded = true;
         break;
      }
   }
   if (!explicitlyIncluded)
   {
      for (int i = 0; i < reader->m_excludedSources.size(); i++)
      {
         if (MatchString(reader->m_excludedSources.get(i), publisherName, false))
         {
            nxlog_debug_tag(DEBUG_TAG, 5, _T("Event source %s filtered"), publisherName);
            goto cleanup;
         }
      }
   }

   // Event ID
   uint32_t eventId = 0;
   if (values[1].Type == EvtVarTypeUInt16)
      eventId = values[1].UInt16Val;

   // Check event ID against filters
   explicitlyIncluded = false;
   for (int i = 0; i < reader->m_includedEvents.size(); i++)
   {
      Range *r = reader->m_includedEvents.get(i);
      if ((eventId >= r->start) && (eventId <= r->end))
      {
         explicitlyIncluded = true;
         break;
      }
   }
   if (!explicitlyIncluded)
   {
      for (int i = 0; i < reader->m_excludedEvents.size(); i++)
      {
         Range *r = reader->m_excludedEvents.get(i);
         if ((eventId >= r->start) && (eventId <= r->end))
         {
            nxlog_debug_tag(DEBUG_TAG, 5, _T("Event ID %u filtered"), eventId);
            goto cleanup;
         }
      }
   }

   // Severity level
   uint32_t level = 0;
   if (values[2].Type == EvtVarTypeByte)
   {
      switch (values[2].ByteVal)
      {
         case WINEVENT_LEVEL_CRITICAL:
            level = 0x0100;
            break;
         case WINEVENT_LEVEL_ERROR:
            level = EVENTLOG_ERROR_TYPE;
            break;
         case WINEVENT_LEVEL_WARNING:
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

   if ((level & reader->m_severityFilter) == 0)
   {
      nxlog_debug_tag(DEBUG_TAG, 5, _T("Event severity level 0x%03X filtered"), level);
      goto cleanup;
   }

   // Check advanced filters
   for (int i = 0; i < reader->m_filters.size(); i++)
   {
      Filter *filter = reader->m_filters.get(i);
      if (((level & filter->severity) != 0) &&
          (eventId >= filter->eventId.start) && (eventId <= filter->eventId.end) &&
          ((filter->source == nullptr) || MatchString(filter->source, publisherName, false)))
      {
         if (!filter->accept)
         {
            nxlog_debug_tag(DEBUG_TAG, 5, _T("Event %u (severity %d) from \"%s\" filtered by advanced filter #%d"), eventId, level, publisherName, i + 1);
            goto cleanup;
         }
         break;
      }
   }

   // Time created
   time_t timestamp;
   if (values[4].Type == EvtVarTypeFileTime)
   {
      timestamp = FileTimeToUnixTime(values[4].FileTimeVal);
   }
   else if (values[4].Type == EvtVarTypeSysTime)
   {
      FILETIME ft;
      SystemTimeToFileTime(values[4].SysTimeVal, &ft);
      timestamp = FileTimeToUnixTime(ft);
   }
   else
   {
      timestamp = time(nullptr);
      nxlog_debug_tag(DEBUG_TAG, 7, _T("Cannot get timestamp from event (values[5].Type == %d)"), values[4].Type);
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
         messageText = MemAllocStringW(reqSize);
         success = EvtFormatMessage(pubMetadata, event, 0, 0, nullptr, EvtFormatMessageEvent, reqSize, messageText, &reqSize);
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

   // Get message XML
   success = EvtFormatMessage(pubMetadata, event, 0, 0, nullptr, EvtFormatMessageXml, 8192, xmlBuffer, &reqSize);
   if (!success)
   {
      DWORD error = GetLastError();
      if ((error != ERROR_INSUFFICIENT_BUFFER) && (error != ERROR_EVT_UNRESOLVED_VALUE_INSERT))
      {
         nxlog_debug_tag(DEBUG_TAG, 5, _T("Call to EvtFormatMessage(XML) failed: error %u (%s)"), error, GetSystemErrorText(error, (TCHAR *)buffer, 4096));
         goto cleanup;
      }
      if (error == ERROR_INSUFFICIENT_BUFFER)
      {
         xml = MemAllocStringW(reqSize);
         success = EvtFormatMessage(pubMetadata, event, 0, 0, nullptr, EvtFormatMessageXml, reqSize, xml, &reqSize);
         if (!success)
         {
            error = GetLastError();
            if (error != ERROR_EVT_UNRESOLVED_VALUE_INSERT)
            {
               nxlog_debug_tag(DEBUG_TAG, 5, _T("Retry call to EvtFormatMessage(XML) failed: error %u (%s)"), error, GetSystemErrorText(error, (TCHAR *)buffer, 4096));
               goto cleanup;
            }
         }
      }
   }

   NXCPMessage *msg = new NXCPMessage(CMD_WINDOWS_EVENT, reader->m_messageId++);
   msg->setField(VID_REQUEST_ID, InterlockedIncrement64(&s_requestId));
   msg->setField(VID_LOG_NAME, reader->m_name);
   msg->setFieldFromTime(VID_TIMESTAMP, timestamp);
   msg->setField(VID_EVENT_SOURCE, publisherName);
   msg->setField(VID_SEVERITY, level);
   msg->setField(VID_EVENT_CODE, eventId);
   msg->setField(VID_MESSAGE, messageText);
   msg->setField(VID_RAW_DATA, xml);
   AgentQueueNotificationMessage(msg);  // Takes ownership of message object
   nxlog_debug_tag(DEBUG_TAG, 7, _T("New event from \"%s\" sent to notification queue: %u(%u): \"%s\""),
         reader->m_name, eventId, level, messageText);

cleanup:
   if (pubMetadata != nullptr)
      EvtClose(pubMetadata);
   if (messageText != buffer)
      MemFree(messageText);
   if (xml != xmlBuffer)
      MemFree(xml);
   if (renderBuffer != buffer)
      MemFree(renderBuffer);
   return 0;
}
