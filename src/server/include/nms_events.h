/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2013 Victor Kirhenshtein
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

/**
 * Event template
 */
struct EVENT_TEMPLATE
{
   UINT32 dwCode;
   UINT32 dwSeverity;
   UINT32 dwFlags;
   TCHAR *pszMessageTemplate;
   TCHAR *pszDescription;
   TCHAR szName[MAX_EVENT_NAME];
};

/**
 * Event
 */
class NXCORE_EXPORTABLE Event
{
private:
   UINT64 m_qwId;
   UINT64 m_qwRootId;    // Root event id
   UINT32 m_dwCode;
   int m_severity;
   UINT32 m_dwFlags;
   UINT32 m_dwSource;
	TCHAR m_name[MAX_EVENT_NAME];
   TCHAR *m_pszMessageText;
   TCHAR *m_pszMessageTemplate;
   time_t m_tTimeStamp;
	TCHAR *m_pszUserTag;
	TCHAR *m_pszCustomMessage;
	Array m_parameters;
	StringList m_parameterNames;

public:
   Event();
   Event(Event *src);
   Event(EVENT_TEMPLATE *pTemplate, UINT32 sourceId, const TCHAR *userTag, const char *format, const TCHAR **names, va_list args);
   ~Event();

   UINT64 getId() { return m_qwId; }
   UINT32 getCode() { return m_dwCode; }
   UINT32 getSeverity() { return m_severity; }
   UINT32 getFlags() { return m_dwFlags; }
   UINT32 getSourceId() { return m_dwSource; }
	const TCHAR *getName() { return m_name; }
   const TCHAR *getMessage() { return m_pszMessageText; }
   const TCHAR *getUserTag() { return m_pszUserTag; }
   time_t getTimeStamp() { return m_tTimeStamp; }

   void setSeverity(int severity) { m_severity = severity; }
   
   UINT64 getRootId() { return m_qwRootId; }
   void setRootId(UINT64 qwId) { m_qwRootId = qwId; }

   void prepareMessage(NXCPMessage *pMsg);

   void expandMessageText();
   TCHAR *expandText(const TCHAR *textTemplate, const TCHAR *alarmMsg = NULL, const TCHAR *alarmKey = NULL);
   static TCHAR *expandText(Event *event, UINT32 sourceObject, const TCHAR *textTemplate, const TCHAR *alarmMsg, const TCHAR *alarmKey);
   void setMessage(const TCHAR *text) { safe_free(m_pszMessageText); m_pszMessageText = _tcsdup(CHECK_NULL_EX(text)); }
   void setUserTag(const TCHAR *text) { safe_free(m_pszUserTag); m_pszUserTag = _tcsdup(CHECK_NULL_EX(text)); }

   UINT32 getParametersCount() { return m_parameters.size(); }
   const TCHAR *getParameter(int index) { return (TCHAR *)m_parameters.get(index); }
   UINT32 getParameterAsULong(int index) { const TCHAR *v = (TCHAR *)m_parameters.get(index); return (v != NULL) ? _tcstoul(v, NULL, 0) : 0; }
   UINT64 getParameterAsUInt64(int index) { const TCHAR *v = (TCHAR *)m_parameters.get(index); return (v != NULL) ? _tcstoull(v, NULL, 0) : 0; }

	const TCHAR *getNamedParameter(const TCHAR *name) { return getParameter(m_parameterNames.indexOfIgnoreCase(name)); }
   UINT32 getNamedParameterAsULong(const TCHAR *name) { return getParameterAsULong(m_parameterNames.indexOfIgnoreCase(name)); }
   UINT64 getNamedParameterAsUInt64(const TCHAR *name) { return getParameterAsUInt64(m_parameterNames.indexOfIgnoreCase(name)); }

	void addParameter(const TCHAR *name, const TCHAR *value);
	void setNamedParameter(const TCHAR *name, const TCHAR *value);
	void setParameter(int index, const TCHAR *name, const TCHAR *value);

   const TCHAR *getCustomMessage() { return CHECK_NULL_EX(m_pszCustomMessage); }
   void setCustomMessage(const TCHAR *message) { safe_free(m_pszCustomMessage); m_pszCustomMessage = (message != NULL) ? _tcsdup(message) : NULL; }
};

/**
 * Event policy rule
 */
class EPRule
{
private:
   UINT32 m_id;
   uuid m_guid;
   UINT32 m_dwFlags;
   UINT32 m_dwNumSources;
   UINT32 *m_pdwSourceList;
   UINT32 m_dwNumEvents;
   UINT32 *m_pdwEventList;
   UINT32 m_dwNumActions;
   UINT32 *m_pdwActionList;
   TCHAR *m_pszComment;
   TCHAR *m_pszScript;
   NXSL_VM *m_pScript;

   TCHAR m_szAlarmMessage[MAX_EVENT_MSG_LENGTH];
   int m_iAlarmSeverity;
   TCHAR m_szAlarmKey[MAX_DB_STRING];
	UINT32 m_dwAlarmTimeout;
	UINT32 m_dwAlarmTimeoutEvent;

	UINT32 m_dwSituationId;
	TCHAR m_szSituationInstance[MAX_DB_STRING];
	StringMap m_situationAttrList;

   bool matchSource(UINT32 dwObjectId);
   bool matchEvent(UINT32 eventCode);
   bool matchSeverity(UINT32 dwSeverity);
   bool matchScript(Event *pEvent);

   void generateAlarm(Event *pEvent);

public:
   EPRule(UINT32 id);
   EPRule(DB_RESULT hResult, int row);
   EPRule(NXCPMessage *msg);
   EPRule(ConfigEntry *config);
   ~EPRule();

   UINT32 getId() const { return m_id; }
   const uuid& getGuid() const { return m_guid; }
   void setId(UINT32 dwNewId) { m_id = dwNewId; }
   bool loadFromDB(DB_HANDLE hdb);
	void saveToDB(DB_HANDLE hdb);
   bool processEvent(Event *pEvent);
   void createMessage(NXCPMessage *pMsg);
   void createNXMPRecord(String &str);

   bool isActionInUse(UINT32 dwActionId);
};

/**
 * Event policy
 */
class EventPolicy
{
private:
   UINT32 m_dwNumRules;
   EPRule **m_ppRuleList;
   RWLOCK m_rwlock;

   void readLock() { RWLockReadLock(m_rwlock, INFINITE); }
   void writeLock() { RWLockWriteLock(m_rwlock, INFINITE); }
   void unlock() { RWLockUnlock(m_rwlock); }
   void clear();

public:
   EventPolicy();
   ~EventPolicy();

   UINT32 getNumRules() { return m_dwNumRules; }
   bool loadFromDB();
   void saveToDB();
   void processEvent(Event *pEvent);
   void sendToClient(ClientSession *pSession, UINT32 dwRqId);
   void replacePolicy(UINT32 dwNumRules, EPRule **ppRuleList);
   void exportRule(String& str, const uuid& guid);
   void importRule(EPRule *rule);

   bool isActionInUse(UINT32 dwActionId);
};

/**
 * Functions
 */
BOOL InitEventSubsystem();
void ShutdownEventSubsystem();
void ReloadEvents();
void DeleteEventTemplateFromList(UINT32 eventCode);
void CorrelateEvent(Event *pEvent);
void CreateNXMPEventRecord(String &str, UINT32 eventCode);

BOOL EventNameFromCode(UINT32 eventCode, TCHAR *pszBuffer);
UINT32 NXCORE_EXPORTABLE EventCodeFromName(const TCHAR *name, UINT32 defaultValue = 0);
EVENT_TEMPLATE *FindEventTemplateByCode(UINT32 eventCode);
EVENT_TEMPLATE *FindEventTemplateByName(const TCHAR *pszName);

BOOL NXCORE_EXPORTABLE PostEvent(UINT32 eventCode, UINT32 sourceId, const char *format, ...);
UINT64 NXCORE_EXPORTABLE PostEvent2(UINT32 eventCode, UINT32 sourceId, const char *format, ...);
BOOL NXCORE_EXPORTABLE PostEventWithNames(UINT32 eventCode, UINT32 sourceId, const char *format, const TCHAR **names, ...);
BOOL NXCORE_EXPORTABLE PostEventWithNames(UINT32 eventCode, UINT32 sourceId, StringMap *parameters);
BOOL NXCORE_EXPORTABLE PostEventWithTagAndNames(UINT32 eventCode, UINT32 sourceId, const TCHAR *userTag, const char *format, const TCHAR **names, ...);
BOOL NXCORE_EXPORTABLE PostEventWithTag(UINT32 eventCode, UINT32 sourceId, const TCHAR *userTag, const char *format, ...);
BOOL NXCORE_EXPORTABLE PostEventEx(Queue *queue, UINT32 eventCode, UINT32 sourceId, const char *format, ...);
void NXCORE_EXPORTABLE ResendEvents(Queue *queue);

const TCHAR NXCORE_EXPORTABLE *GetStatusAsText(int status, bool allCaps);

/**
 * Global variables
 */
extern Queue *g_pEventQueue;
extern EventPolicy *g_pEventPolicy;
extern INT64 g_totalEventsProcessed;

#endif   /* _nms_events_h_ */
