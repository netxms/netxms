/* 
** NetXMS - Network Management System
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

#include <nxevent.h>


//
// Constants
//

#define FIRST_USER_EVENT_ID      100000


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
   char *m_pszMessageText;
   DWORD m_dwNumParameters;
   char **m_ppszParameters;
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
   const char *Message(void) { return m_pszMessageText; }
   time_t TimeStamp(void) { return m_tTimeStamp; }

   void PrepareMessage(NXC_EVENT *pEventData);

   char *ExpandText(char *szTemplate);
};


//
// Event policy rule flags
//

#define RF_STOP_PROCESSING    0x0001
#define RF_NEGATED_SOURCE     0x0002
#define RF_NEGATED_EVENTS     0x0004
#define RF_SEVERITY_INFO      0x0100
#define RF_SEVERITY_MINOR     0x0200
#define RF_SEVERITY_WARNING   0x0400
#define RF_SEVERITY_MAJOR     0x0800
#define RF_SEVERITY_CRITICAL  0x1000


//
// Event source structure
//

typedef struct
{
   int iType;
   DWORD dwObjectId;
} EVENT_SOURCE;


//
// Event policy rule
//

class EPRule
{
private:
   DWORD m_dwId;
   DWORD m_dwFlags;
   DWORD m_dwNumSources;
   DWORD *m_pdwSourceList;
   DWORD m_dwNumEvents;
   DWORD *m_pdwEventList;
   DWORD m_dwNumActions;
   DWORD *m_pdwActionList;
   char *m_pszComment;

   BOOL MatchSource(DWORD dwObjectId);
   BOOL MatchEvent(DWORD dwEventId);
   BOOL MatchSeverity(DWORD dwSeverity);

public:
   EPRule(DWORD dwId, char *szComment, DWORD dwFlags);
   ~EPRule();

   DWORD Id(void) { return m_dwId; }

   BOOL LoadFromDB(void);

   BOOL ProcessEvent(Event *pEvent);
};


//
// Event policy
//

class EventPolicy
{
private:
   DWORD m_dwNumRules;
   EPRule **m_ppRuleList;

public:
   EventPolicy();
   ~EventPolicy();

   BOOL LoadFromDB(void);
   void ProcessEvent(Event *pEvent);
};


//
// Functions
//

BOOL InitEventSubsystem(void);
BOOL PostEvent(DWORD dwEventId, DWORD dwSourceId, char *szFormat, ...);
void ReloadEvents(void);


//
// Global variables
//

extern Queue *g_pEventQueue;
extern EventPolicy *g_pEventPolicy;
extern char *g_szStatusText[];
extern char *g_szStatusTextSmall[];

#endif   /* _nms_events_h_ */
