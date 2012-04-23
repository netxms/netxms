/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2012 Victor Kirhenshtein
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

#define EVENTLOG_MAX_MESSAGE_SIZE   255
#define EVENTLOG_MAX_USERTAG_SIZE   63


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
   time_t m_tTimeStamp;
	TCHAR *m_pszUserTag;
	TCHAR *m_pszCustomMessage;
	Array m_parameters;
	StringList m_parameterNames;

public:
   Event();
   Event(EVENT_TEMPLATE *pTemplate, DWORD dwSourceId, const TCHAR *pszUserTag, const char *szFormat, const TCHAR **names, va_list args);
   ~Event();

   QWORD getId() { return m_qwId; }
   DWORD getCode() { return m_dwCode; }
   DWORD getSeverity() { return m_dwSeverity; }
   DWORD getFlags() { return m_dwFlags; }
   DWORD getSourceId() { return m_dwSource; }
	const TCHAR *getName() { return m_szName; }
   const TCHAR *getMessage() { return m_pszMessageText; }
   const TCHAR *getUserTag() { return m_pszUserTag; }
   time_t getTimeStamp() { return m_tTimeStamp; }
   
   QWORD getRootId() { return m_qwRootId; }
   void setRootId(QWORD qwId) { m_qwRootId = qwId; }

   void prepareMessage(CSCPMessage *pMsg);

   void expandMessageText();
   TCHAR *expandText(const TCHAR *szTemplate, const TCHAR *pszAlarmMsg = NULL);
   static TCHAR *expandText(Event *event, DWORD sourceObject, const TCHAR *szTemplate, const TCHAR *pszAlarmMsg);

   DWORD getParametersCount() { return m_parameters.size(); }
   const TCHAR *getParameter(int index) { return (TCHAR *)m_parameters.get(index); }
   DWORD getParameterAsULong(int index) { const TCHAR *v = (TCHAR *)m_parameters.get(index); return (v != NULL) ? _tcstoul(v, NULL, 0) : 0; }
   QWORD getParameterAsUInt64(int index) { const TCHAR *v = (TCHAR *)m_parameters.get(index); return (v != NULL) ? _tcstoull(v, NULL, 0) : 0; }

	const TCHAR *getNamedParameter(const TCHAR *name) { return getParameter(m_parameterNames.getIndexIgnoreCase(name)); }
   DWORD getNamedParameterAsULong(const TCHAR *name) { return getParameterAsULong(m_parameterNames.getIndexIgnoreCase(name)); }
   QWORD getNamedParameterAsUInt64(const TCHAR *name) { return getParameterAsUInt64(m_parameterNames.getIndexIgnoreCase(name)); }

	void addParameter(const TCHAR *name, const TCHAR *value);
	void setNamedParameter(const TCHAR *name, const TCHAR *value);

   const TCHAR *getCustomMessage() { return CHECK_NULL_EX(m_pszCustomMessage); }
   void setCustomMessage(const TCHAR *message) { safe_free(m_pszCustomMessage); m_pszCustomMessage = (message != NULL) ? _tcsdup(message) : NULL; }
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
BOOL PostEventWithNames(DWORD dwEventCode, DWORD dwSourceId, const char *pszFormat, const TCHAR **names, ...);
BOOL PostEventWithTag(DWORD dwEventCode, DWORD dwSourceId, const TCHAR *pszUserTag, const char *pszFormat, ...);
BOOL PostEventEx(Queue *pQueue, DWORD dwEventCode, DWORD dwSourceId, 
                 const char *pszFormat, ...);
void ResendEvents(Queue *pQueue);
void ReloadEvents(void);
void DeleteEventTemplateFromList(DWORD dwEventCode);
void CorrelateEvent(Event *pEvent);
void CreateNXMPEventRecord(String &str, DWORD dwCode);
BOOL EventNameFromCode(DWORD dwCode, TCHAR *pszBuffer);
DWORD EventCodeFromName(const TCHAR *name, DWORD defaultValue = 0);
EVENT_TEMPLATE *FindEventTemplateByCode(DWORD dwCode);
EVENT_TEMPLATE *FindEventTemplateByName(const TCHAR *pszName);


//
// Global variables
//

extern Queue *g_pEventQueue;
extern EventPolicy *g_pEventPolicy;
extern const TCHAR *g_szStatusText[];
extern const TCHAR *g_szStatusTextSmall[];
extern INT64 g_totalEventsProcessed;

#endif   /* _nms_events_h_ */
