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
   DWORD dwCode;
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
   QWORD m_qwId;
   DWORD m_dwCode;
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

   QWORD Id(void) { return m_qwId; }
   DWORD Code(void) { return m_dwCode; }
   DWORD Severity(void) { return m_dwSeverity; }
   DWORD Flags(void) { return m_dwFlags; }
   DWORD SourceId(void) { return m_dwSource; }
   const char *Message(void) { return m_pszMessageText; }
   time_t TimeStamp(void) { return m_tTimeStamp; }

   void PrepareMessage(NXC_EVENT *pEventData);

   char *ExpandText(char *szTemplate);
};


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

   char m_szAlarmMessage[MAX_DB_STRING];
   int m_iAlarmSeverity;
   char m_szAlarmKey[MAX_DB_STRING];
   char m_szAlarmAckKey[MAX_DB_STRING];

   BOOL MatchSource(DWORD dwObjectId);
   BOOL MatchEvent(DWORD dwEventCode);
   BOOL MatchSeverity(DWORD dwSeverity);

   void GenerateAlarm(Event *pEvent);

public:
   EPRule(DWORD dwId);
   EPRule(DB_RESULT hResult, int iRow);
   EPRule(CSCPMessage *pMsg);
   ~EPRule();

   DWORD Id(void) { return m_dwId; }
   void SetId(DWORD dwNewId) { m_dwId = dwNewId; }
   BOOL LoadFromDB(void);
   void SaveToDB(void);
   BOOL ProcessEvent(Event *pEvent);
   void CreateMessage(CSCPMessage *pMsg);
};


//
// Event policy
//

class EventPolicy
{
private:
   DWORD m_dwNumRules;
   EPRule **m_ppRuleList;
   RWLOCK m_rwlock;

   void ReadLock(void) { RWLockReadLock(m_rwlock, INFINITE); }
   void WriteLock(void) { RWLockWriteLock(m_rwlock, INFINITE); }
   void Unlock(void) { RWLockUnlock(m_rwlock); }
   void Clear(void);

public:
   EventPolicy();
   ~EventPolicy();

   DWORD NumRules(void) { return m_dwNumRules; }
   BOOL LoadFromDB(void);
   void SaveToDB(void);
   void ProcessEvent(Event *pEvent);
   void SendToClient(ClientSession *pSession, DWORD dwRqId);
   void ReplacePolicy(DWORD dwNumRules, EPRule **ppRuleList);
};


//
// Functions
//

BOOL InitEventSubsystem(void);
void ShutdownEventSubsystem(void);
BOOL PostEvent(DWORD dwEventCode, DWORD dwSourceId, char *szFormat, ...);
void ReloadEvents(void);


//
// Global variables
//

extern Queue *g_pEventQueue;
extern EventPolicy *g_pEventPolicy;
extern char *g_szStatusText[];
extern char *g_szStatusTextSmall[];

#endif   /* _nms_events_h_ */
