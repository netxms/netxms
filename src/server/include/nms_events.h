/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2009 Victor Kirhenshtein
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
** File: nms_events.h
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
   TCHAR *pszMessageTemplate;
   TCHAR *pszDescription;
   TCHAR szName[MAX_EVENT_NAME];
};


//
// Event
//

class Event
{
private:
   QWORD m_qwId;
   QWORD m_qwRootId;    // Root event id
   DWORD m_dwCode;
   DWORD m_dwSeverity;
   DWORD m_dwFlags;
   DWORD m_dwSource;
	TCHAR m_szName[MAX_EVENT_NAME];
   TCHAR *m_pszMessageText;
   TCHAR *m_pszMessageTemplate;
   DWORD m_dwNumParameters;
   TCHAR **m_ppszParameters;
   time_t m_tTimeStamp;
	TCHAR *m_pszUserTag;
	TCHAR *m_pszCustomMessage;

public:
   Event();
   Event(EVENT_TEMPLATE *pTemplate, DWORD dwSourceId, const TCHAR *pszUserTag, const char *szFormat, va_list args);
   ~Event();

   QWORD Id() { return m_qwId; }
   DWORD Code() { return m_dwCode; }
   DWORD Severity() { return m_dwSeverity; }
   DWORD Flags() { return m_dwFlags; }
   DWORD SourceId() { return m_dwSource; }
	const TCHAR *Name() { return m_szName; }
   const TCHAR *Message() { return m_pszMessageText; }
   const TCHAR *UserTag() { return m_pszUserTag; }
   time_t TimeStamp() { return m_tTimeStamp; }
   
   QWORD GetRootId() { return m_qwRootId; }
   void SetRootId(QWORD qwId) { m_qwRootId = qwId; }

   void PrepareMessage(CSCPMessage *pMsg);

   void ExpandMessageText();
   TCHAR *ExpandText(TCHAR *szTemplate, TCHAR *pszAlarmMsg = NULL);

   DWORD GetParametersCount() { return m_dwNumParameters; }
   char *GetParameter(DWORD dwIndex) { return (dwIndex < m_dwNumParameters) ? m_ppszParameters[dwIndex] : NULL; }
   DWORD GetParameterAsULong(DWORD dwIndex) { return (dwIndex < m_dwNumParameters) ? _tcstoul(m_ppszParameters[dwIndex], NULL, 0) : 0; }
   QWORD GetParameterAsUInt64(DWORD dwIndex) { return (dwIndex < m_dwNumParameters) ? _tcstoull(m_ppszParameters[dwIndex], NULL, 0) : 0; }
   
   void SetCustomMessage(const TCHAR *pszMessage) { safe_free(m_pszCustomMessage); m_pszCustomMessage = (pszMessage != NULL) ? _tcsdup(pszMessage) : NULL; }
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
   TCHAR *m_pszComment;
   TCHAR *m_pszScript;
   NXSL_Program *m_pScript;

   TCHAR m_szAlarmMessage[MAX_DB_STRING];
   int m_iAlarmSeverity;
   TCHAR m_szAlarmKey[MAX_DB_STRING];
	DWORD m_dwAlarmTimeout;
	DWORD m_dwAlarmTimeoutEvent;

	DWORD m_dwSituationId;
	TCHAR m_szSituationInstance[MAX_DB_STRING];
	StringMap m_situationAttrList;

   BOOL MatchSource(DWORD dwObjectId);
   BOOL MatchEvent(DWORD dwEventCode);
   BOOL MatchSeverity(DWORD dwSeverity);
   BOOL MatchScript(Event *pEvent);

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

   BOOL ActionInUse(DWORD dwActionId);
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

   BOOL ActionInUse(DWORD dwActionId);
};


//
// Functions
//

BOOL InitEventSubsystem(void);
void ShutdownEventSubsystem(void);
BOOL PostEvent(DWORD dwEventCode, DWORD dwSourceId, const char *pszFormat, ...);
BOOL PostEventWithTag(DWORD dwEventCode, DWORD dwSourceId, const TCHAR *pszUserTag, const char *pszFormat, ...);
BOOL PostEventEx(Queue *pQueue, DWORD dwEventCode, DWORD dwSourceId, 
                 const char *pszFormat, ...);
void ResendEvents(Queue *pQueue);
void ReloadEvents(void);
void DeleteEventTemplateFromList(DWORD dwEventCode);
void CorrelateEvent(Event *pEvent);
void CreateNXMPEventRecord(String &str, DWORD dwCode);
BOOL ResolveEventName(DWORD dwCode, TCHAR *pszBuffer);
EVENT_TEMPLATE *FindEventTemplateByCode(DWORD dwCode);
EVENT_TEMPLATE *FindEventTemplateByName(const TCHAR *pszName);


//
// Global variables
//

extern Queue *g_pEventQueue;
extern EventPolicy *g_pEventPolicy;
extern const char *g_szStatusText[];
extern const char *g_szStatusTextSmall[];
extern INT64 g_totalEventsProcessed;

#endif   /* _nms_events_h_ */
