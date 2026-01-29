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
** File: wineventsync.h
**
**/

#ifndef _wineventsync_h_
#define _wineventsync_h_

#include <nms_common.h>
#include <nms_agent.h>
#include <nms_threads.h>
#include <winevt.h>
#if __has_include(<winmeta.h>)
#include <winmeta.h>
#else
// Some MinGW toolchains (e.g. llvm-mingw) do not ship winmeta.h.
// Define the few constants used below with their standard Windows SDK values.
#define WINEVENT_LEVEL_CRITICAL        0x1
#define WINEVENT_LEVEL_ERROR           0x2
#define WINEVENT_LEVEL_WARNING         0x3
#define WINEVENT_LEVEL_INFO            0x4
#define WINEVENT_LEVEL_VERBOSE         0x5
#define WINEVENT_KEYWORD_AUDIT_SUCCESS 0x0020000000000000ULL
#define WINEVENT_KEYWORD_AUDIT_FAILURE 0x0010000000000000ULL
#endif

#define DEBUG_TAG _T("wineventsync")

/**
 * Event range
 */
struct Range
{
   uint32_t start;
   uint32_t end;
};

/**
 * Event filter
 */
struct Filter
{
   wchar_t *source;
   Range eventId;
   uint32_t severity;
   bool accept;
};

/**
 * Event log reader
 */
class EventLogReader
{
private:
   THREAD m_thread;
   Condition m_stopCondition;
   TCHAR *m_name;
   EVT_HANDLE m_renderContext;
   uint32_t m_messageId;
   StringList m_includedSources;
   StringList m_excludedSources;
   StructArray<Range> m_includedEvents;
   StructArray<Range> m_excludedEvents;
   uint32_t m_severityFilter;
   StructArray<Filter> m_filters;
   bool m_processOfflineEvents;
   uint32_t m_maxOfflineEventAge;  // in days
   TCHAR m_registryKey[64];

   void run();
   void processOfflineEvents();
   void saveTimestamp(int64_t timestampMs);
   static DWORD WINAPI subscribeCallback(EVT_SUBSCRIBE_NOTIFY_ACTION action, PVOID context, EVT_HANDLE event);

public:
   EventLogReader(const TCHAR *name, Config *config, bool processOfflineEvents, uint32_t maxOfflineEventAge);
   ~EventLogReader();

   void start();
   void stop();

   const TCHAR *getName() const { return m_name; }
};

#endif
