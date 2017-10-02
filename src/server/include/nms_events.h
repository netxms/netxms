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
#include <jansson.h>


//
// Constants
//

#define EVENTLOG_MAX_MESSAGE_SIZE   255
#define EVENTLOG_MAX_USERTAG_SIZE   63

/**
 * Event template
 */
class EventTemplate : public RefCountObject
{
private:
   UINT32 m_code;
   int m_severity;
   uuid m_guid;
   TCHAR m_name[MAX_EVENT_NAME];
   UINT32 m_flags;
   TCHAR *m_messageTemplate;
   TCHAR *m_description;

protected:
   virtual ~EventTemplate();

public:
   EventTemplate(DB_RESULT hResult, int row);

   UINT32 getCode() const { return m_code; }
   int getSeverity() const { return m_severity; }
   const uuid& getGuid() const { return m_guid; }
   const TCHAR *getName() const { return m_name; }
   UINT32 getFlags() const { return m_flags; }
   const TCHAR *getMessageTemplate() const { return m_messageTemplate; }
   const TCHAR *getDescription() const { return m_description; }

   json_t *toJson() const;
};

/**
 * Event
 */
class NXCORE_EXPORTABLE Event
{
private:
   UINT64 m_id;
   UINT64 m_rootId;    // Root event id
   UINT32 m_code;
   int m_severity;
   UINT32 m_flags;
   UINT32 m_sourceId;
   UINT32 m_dciId;
	TCHAR m_name[MAX_EVENT_NAME];
   TCHAR *m_messageText;
   TCHAR *m_messageTemplate;
   time_t m_timeStamp;
	TCHAR *m_userTag;
	TCHAR *m_customMessage;
	Array m_parameters;
	StringList m_parameterNames;

public:
   Event();
   Event(const Event *src);
   Event(const EventTemplate *eventTemplate, UINT32 sourceId, UINT32 dciId, const TCHAR *userTag, const char *format, const TCHAR **names, va_list args);
   ~Event();

   UINT64 getId() const { return m_id; }
   UINT32 getCode() const { return m_code; }
   UINT32 getSeverity() const { return m_severity; }
   UINT32 getFlags() const { return m_flags; }
   UINT32 getSourceId() const { return m_sourceId; }
   UINT32 getDciId() const { return m_dciId; }
	const TCHAR *getName() const { return m_name; }
   const TCHAR *getMessage() const { return m_messageText; }
   const TCHAR *getUserTag() const { return m_userTag; }
   time_t getTimeStamp() const { return m_timeStamp; }

   void setSeverity(int severity) { m_severity = severity; }

   UINT64 getRootId() const { return m_rootId; }
   void setRootId(UINT64 id) { m_rootId = id; }

   void prepareMessage(NXCPMessage *msg) const;

   void expandMessageText();
   TCHAR *expandText(const TCHAR *textTemplate, const TCHAR *alarmMsg = NULL, const TCHAR *alarmKey = NULL);
   static TCHAR *expandText(Event *event, UINT32 sourceObject, const TCHAR *textTemplate, const TCHAR *alarmMsg, const TCHAR *alarmKey);
   void setMessage(const TCHAR *text) { free(m_messageText); m_messageText = _tcsdup_ex(text); }
   void setUserTag(const TCHAR *text) { free(m_userTag); m_userTag = _tcsdup_ex(text); }

   int getParametersCount() const { return m_parameters.size(); }
   const TCHAR *getParameter(int index) const { return (TCHAR *)m_parameters.get(index); }
   UINT32 getParameterAsULong(int index) const { const TCHAR *v = (TCHAR *)m_parameters.get(index); return (v != NULL) ? _tcstoul(v, NULL, 0) : 0; }
   UINT64 getParameterAsUInt64(int index) const { const TCHAR *v = (TCHAR *)m_parameters.get(index); return (v != NULL) ? _tcstoull(v, NULL, 0) : 0; }

	const TCHAR *getNamedParameter(const TCHAR *name) const { return getParameter(m_parameterNames.indexOfIgnoreCase(name)); }
   UINT32 getNamedParameterAsULong(const TCHAR *name) const { return getParameterAsULong(m_parameterNames.indexOfIgnoreCase(name)); }
   UINT64 getNamedParameterAsUInt64(const TCHAR *name) const { return getParameterAsUInt64(m_parameterNames.indexOfIgnoreCase(name)); }

	void addParameter(const TCHAR *name, const TCHAR *value);
	void setNamedParameter(const TCHAR *name, const TCHAR *value);
	void setParameter(int index, const TCHAR *name, const TCHAR *value);

   const TCHAR *getCustomMessage() const { return CHECK_NULL_EX(m_customMessage); }
   void setCustomMessage(const TCHAR *message) { free(m_customMessage); m_customMessage = _tcsdup_ex(message); }

   String createJson();
};

/**
 * Defines for type of persistent storage action
 */
 #define PSTORAGE_SET      1
 #define PSTORAGE_DELETE   2

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
	IntegerArray<UINT32> *m_alarmCategoryList;
	StringMap m_pstorageSetActions;
	StringList m_pstorageDeleteActions;

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
	bool saveToDB(DB_HANDLE hdb);
   bool processEvent(Event *pEvent);
   void createMessage(NXCPMessage *pMsg);
   void createNXMPRecord(String &str);
   json_t *toJson() const;

   bool isActionInUse(UINT32 dwActionId);
   bool isCategoryInUse(UINT32 categoryId) const { return m_alarmCategoryList->contains(categoryId); }
};

/**
 * Event policy
 */
class EventPolicy
{
private:
   ObjectArray<EPRule> m_rules;
   RWLOCK m_rwlock;

   void readLock() const { RWLockReadLock(m_rwlock, INFINITE); }
   void writeLock() { RWLockWriteLock(m_rwlock, INFINITE); }
   void unlock() const { RWLockUnlock(m_rwlock); }

public:
   EventPolicy();
   ~EventPolicy();

   UINT32 getNumRules() const { return m_rules.size(); }
   bool loadFromDB();
   bool saveToDB() const;
   void processEvent(Event *pEvent);
   void sendToClient(ClientSession *pSession, UINT32 dwRqId) const;
   void replacePolicy(UINT32 dwNumRules, EPRule **ppRuleList);
   void exportRule(String& str, const uuid& guid) const;
   void importRule(EPRule *rule);
   void removeRuleCategory (UINT32 categoryId);
   json_t *toJson() const;

   bool isActionInUse(UINT32 actionId) const;
   bool isCategoryInUse(UINT32 categoryId) const;
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

bool EventNameFromCode(UINT32 eventCode, TCHAR *buffer);
UINT32 NXCORE_EXPORTABLE EventCodeFromName(const TCHAR *name, UINT32 defaultValue = 0);
EventTemplate *FindEventTemplateByCode(UINT32 eventCode);
EventTemplate *FindEventTemplateByName(const TCHAR *pszName);

bool NXCORE_EXPORTABLE PostEvent(UINT32 eventCode, UINT32 sourceId, const char *format, ...);
bool NXCORE_EXPORTABLE PostDciEvent(UINT32 eventCode, UINT32 sourceId, UINT32 dciId, const char *format, ...);
UINT64 NXCORE_EXPORTABLE PostEvent2(UINT32 eventCode, UINT32 sourceId, const char *format, ...);
bool NXCORE_EXPORTABLE PostEventWithNames(UINT32 eventCode, UINT32 sourceId, const char *format, const TCHAR **names, ...);
bool NXCORE_EXPORTABLE PostEventWithNames(UINT32 eventCode, UINT32 sourceId, StringMap *parameters);
bool NXCORE_EXPORTABLE PostDciEventWithNames(UINT32 eventCode, UINT32 sourceId, UINT32 dciId, const char *format, const TCHAR **names, ...);
bool NXCORE_EXPORTABLE PostDciEventWithNames(UINT32 eventCode, UINT32 sourceId, UINT32 dciId, StringMap *parameters);
bool NXCORE_EXPORTABLE PostEventWithTagAndNames(UINT32 eventCode, UINT32 sourceId, const TCHAR *userTag, const char *format, const TCHAR **names, ...);
bool NXCORE_EXPORTABLE PostEventWithTag(UINT32 eventCode, UINT32 sourceId, const TCHAR *userTag, const char *format, ...);
bool NXCORE_EXPORTABLE PostEventEx(Queue *queue, UINT32 eventCode, UINT32 sourceId, const char *format, ...);
void NXCORE_EXPORTABLE ResendEvents(Queue *queue);

const TCHAR NXCORE_EXPORTABLE *GetStatusAsText(int status, bool allCaps);

/**
 * Global variables
 */
extern Queue *g_pEventQueue;
extern EventPolicy *g_pEventPolicy;
extern INT64 g_totalEventsProcessed;

#endif   /* _nms_events_h_ */
