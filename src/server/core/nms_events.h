/* 
** Project X - Network Management System
** Copyright (C) 2003 Victor Kirhenshtein
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
** $module: nms_events.h
**
**/

#ifndef _nms_events_h_
#define _nms_events_h_


//
// Constants
//

#define FIRST_USER_EVENT_ID      100000

#define EVENT_SEVERITY_INFO      0
#define EVENT_SEVERITY_WARNING   1
#define EVENT_SEVERITY_MAJOR     2
#define EVENT_SEVERITY_HIGH      3
#define EVENT_SEVERITY_CRITICAL  4


//
// Event flags
//

#define EF_LOG                   0x0001


//
// System-defined events
//

#define EVENT_NODE_ADDED         1
#define EVENT_SUBNET_ADDED       2
#define EVENT_INTERFACE_ADDED    3
#define EVENT_INTERFACE_UP       4
#define EVENT_INTERFACE_DOWN     5
#define EVENT_NODE_NORMAL        6
#define EVENT_NODE_INFO          7
#define EVENT_NODE_WARNING       8
#define EVENT_NODE_ERROR         9
#define EVENT_NODE_CRITICAL      10
#define EVENT_NODE_UNKNOWN       11
#define EVENT_NODE_UNMANAGED     12


//
// Event template
//

struct EVENT_TEMPLATE
{
   DWORD dwId;
   DWORD dwSeverity;
   DWORD dwFlags;
   char *szMessageTemplate;
   char *szDescription;
};


//
// Event
//

class Event
{
private:
   DWORD m_dwId;
   DWORD m_dwSeverity;
   DWORD m_dwFlags;
   DWORD m_dwSource;
   char *m_szMessageText;
   DWORD m_dwNumParameters;
   char **m_pszParameters;
   time_t m_tTimeStamp;

   void ExpandMessageText(char *szMessageTemplate);

public:
   Event();
   Event(EVENT_TEMPLATE *pTemplate, DWORD dwSourceId, char *szFormat, va_list args);
   ~Event();

   DWORD Id(void) { return m_dwId; }
   DWORD Severity(void) { return m_dwSeverity; }
   DWORD Flags(void) { return m_dwFlags; }
   DWORD SourceId(void) { return m_dwSource; }
   const char *Message(void) { return m_szMessageText; }
   time_t TimeStamp(void) { return m_tTimeStamp; }
};


//
// Functions
//

BOOL InitEventSubsystem(void);
BOOL PostEvent(DWORD dwEventId, DWORD dwSourceId, char *szFormat, ...);


//
// Global variables
//

extern Queue *g_pEventQueue;

#endif   /* _nms_events_h_ */
