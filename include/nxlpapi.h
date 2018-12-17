/*
** NetXMS - Network Management System
** Log Parsing Library
** Copyright (C) 2003-2018 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation; either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: nxlpapi.h
**
**/

#ifndef _nxlpapi_h_
#define _nxlpapi_h_

#ifdef LIBNXLP_EXPORTS
#define LIBNXLP_EXPORTABLE __EXPORT
#else
#define LIBNXLP_EXPORTABLE __IMPORT
#endif

#include <netxms-regex.h>
#include <nms_util.h>
#include <uuid.h>

/**
 * Parser status
 */
enum LogParserStatus
{
   LPS_INIT                = 0,
   LPS_RUNNING             = 1,
   LPS_NO_FILE             = 2,
   LPS_OPEN_ERROR          = 3,
   LPS_SUSPENDED           = 4,
   LPS_EVT_SUBSCRIBE_ERROR = 5,
   LPS_EVT_READ_ERROR      = 6,
   LPS_EVT_OPEN_ERROR      = 7,
   LPS_VSS_FAILURE         = 8
};

/**
 * Context actions
 */
enum LogParserContextAction
{
   CONTEXT_SET_MANUAL    = 0,
   CONTEXT_SET_AUTOMATIC = 1,
   CONTEXT_CLEAR         = 2
};

/**
 * File encoding
 */
enum LogParserFileEncoding
{
   LP_FCP_AUTO    = -1,
   LP_FCP_ACP     = 0,
   LP_FCP_UTF8    = 1,
   LP_FCP_UCS2    = 2,
   LP_FCP_UCS2_LE = 3,
   LP_FCP_UCS2_BE = 4,
   LP_FCP_UCS4    = 5,
   LP_FCP_UCS4_LE = 6,
   LP_FCP_UCS4_BE = 7
};

/**
 * Log parser callback
 * Parameters:
 *    NetXMS event code, NetXMS event name, original text, source,
 *    original event ID (facility), original severity,
 *    capture groups, variables, record id, object id, repeat count, user arg, agent action
 */
typedef void (* LogParserCallback)(UINT32, const TCHAR *, const TCHAR *, const TCHAR *, UINT32, UINT32, StringList *, StringList *, UINT64, UINT32, int, void *, const TCHAR *, const StringList *);

class LIBNXLP_EXPORTABLE LogParser;

#ifdef _WIN32

/**
 * Snapshot handle
 */
struct FileSnapshot
{
   void *handle;
   TCHAR *name;
};

#endif

/**
 * Per object rule statistics
 */
struct ObjectRuleStats
{
   int checkCount;
   int matchCount;
};

/**
 * Log parser rule
 */
class LIBNXLP_EXPORTABLE LogParserRule
{
   friend class LogParser;

private:
	LogParser *m_parser;
	TCHAR *m_name;
	regex_t m_preg;
	UINT32 m_eventCode;
	TCHAR *m_eventName;
	bool m_isValid;
	regmatch_t *m_pmatch;
	TCHAR *m_regexp;
	TCHAR *m_source;
	UINT32 m_level;
	UINT32 m_idStart;
	UINT32 m_idEnd;
	TCHAR *m_context;
	int m_contextAction;
	TCHAR *m_contextToChange;
	bool m_isInverted;
	bool m_breakOnMatch;
	TCHAR *m_description;
	int m_repeatInterval;
	int m_repeatCount;
   IntegerArray<time_t> *m_matchArray;
	bool m_resetRepeat;
	int m_checkCount;
	int m_matchCount;
	TCHAR *m_agentAction;
	StringList *m_agentActionArgs;
	HashMap<UINT32, ObjectRuleStats> *m_objectCounters;

	bool matchInternal(bool extMode, const TCHAR *source, UINT32 eventId, UINT32 level, const TCHAR *line, 
                      StringList *variables, UINT64 recordId, UINT32 objectId, LogParserCallback cb, void *userArg);
	bool matchRepeatCount();
   void expandMacros(const TCHAR *regexp, String &out);
   void incCheckCount(UINT32 objectId);
   void incMatchCount(UINT32 objectId);

public:
	LogParserRule(LogParser *parser, const TCHAR *name,
	              const TCHAR *regexp, UINT32 eventCode = 0, const TCHAR *eventName = NULL,
					  int repeatInterval = 0, int repeatCount = 0,
					  bool resetRepeat = true, const TCHAR *source = NULL, UINT32 level = 0xFFFFFFFF,
					  UINT32 idStart = 0, UINT32 idEnd = 0xFFFFFFFF);
	LogParserRule(LogParserRule *src, LogParser *parser);
	~LogParserRule();

	const TCHAR *getName() const { return m_name; }
	bool isValid() const { return m_isValid; }

	bool match(const TCHAR *line, UINT32 objectId, LogParserCallback cb, void *userArg);
	bool matchEx(const TCHAR *source, UINT32 eventId, UINT32 level, const TCHAR *line, StringList *variables, 
                UINT64 recordId, UINT32 objectId, LogParserCallback cb, void *userArg);

	void setAgentAction(const TCHAR *agentAction) { MemFree(m_agentAction); m_agentAction = MemCopyString(agentAction); }
	void setAgentActionArgs(StringList *args) { delete(m_agentActionArgs); m_agentActionArgs = args; }

	void setContext(const TCHAR *context) { MemFree(m_context); m_context = MemCopyString(context); }
	void setContextToChange(const TCHAR *context) { MemFree(m_contextToChange); m_contextToChange = MemCopyString(context); }
	void setContextAction(int action) { m_contextAction = action; }

	void setInverted(bool flag) { m_isInverted = flag; }
	bool isInverted() const { return m_isInverted; }

	void setBreakFlag(bool flag) { m_breakOnMatch = flag; }
	bool getBreakFlag() const { return m_breakOnMatch; }

	const TCHAR *getContext() const { return m_context; }
	const TCHAR *getContextToChange() const { return m_contextToChange; }
	int getContextAction() const { return m_contextAction; }

	void setDescription(const TCHAR *descr) { MemFree(m_description); m_description = MemCopyString(descr); }
	const TCHAR *getDescription() const { return CHECK_NULL_EX(m_description); }

	void setSource(const TCHAR *source) { MemFree(m_source); m_source = MemCopyString(source); }
	const TCHAR *getSource() const { return CHECK_NULL_EX(m_source); }

	void setLevel(UINT32 level) { m_level = level; }
	UINT32 getLevel() const { return m_level; }

	void setIdRange(UINT32 start, UINT32 end) { m_idStart = start; m_idEnd = end; }
	QWORD getIdRange() const { return ((QWORD)m_idStart << 32) | (QWORD)m_idEnd; }

   void setRepeatInterval(int repeatInterval) { m_repeatInterval = repeatInterval; }
   int getRepeatInterval() const { return m_repeatInterval; }

   void setRepeatCount(int repeatCount) { m_repeatCount = repeatCount; }
   int getRepeatCount() const { return m_repeatCount; }

   void setRepeatReset(bool resetRepeat) { m_resetRepeat = resetRepeat; }
   bool isRepeatReset() const { return m_resetRepeat; }

	const TCHAR *getRegexpSource() const { return CHECK_NULL(m_regexp); }

   int getCheckCount(UINT32 objectId = 0) const;
   int getMatchCount(UINT32 objectId = 0) const;

   void restoreCounters(const LogParserRule *rule);
};

/**
 * Log parser class
 */
class LIBNXLP_EXPORTABLE LogParser
{
private:
	ObjectArray<LogParserRule> *m_rules;
	StringMap m_contexts;
	StringMap m_macros;
	LogParserCallback m_cb;
	void *m_userArg;
	TCHAR *m_fileName;
	int m_fileEncoding;
	bool m_preallocatedFile;
	StringList m_exclusionSchedules;
	TCHAR *m_name;
	CODE_TO_TEXT *m_eventNameList;
	bool (*m_eventResolver)(const TCHAR *, UINT32 *);
	THREAD m_thread;	// Associated thread
   CONDITION m_stopCondition;
   int m_recordsProcessed;
	int m_recordsMatched;
	bool m_processAllRules;
   bool m_suspended;
   bool m_keepFileOpen;
   bool m_ignoreMTime;
	int m_traceLevel;
	LogParserStatus m_status;
#ifdef _WIN32
   TCHAR *m_marker;
   bool m_useSnapshot;
#endif
   uuid m_guid;

	const TCHAR *checkContext(LogParserRule *rule);
	bool matchLogRecord(bool hasAttributes, const TCHAR *source, UINT32 eventId, UINT32 level, const TCHAR *line, StringList *variables, UINT64 recordId, UINT32 objectId);

	bool isExclusionPeriod();

   const LogParserRule *findRuleByName(const TCHAR *name) const;

   int getCharSize() const;

   void setStatus(LogParserStatus status) { m_status = status; }

   bool monitorFile2(bool readFromCurrPos);

#ifdef _WIN32
   void parseEvent(EVENTLOGRECORD *rec);

   bool monitorFileWithSnapshot(bool readFromCurrPos);
   bool monitorEventLogV6();
	bool monitorEventLogV4();

   time_t readLastProcessedRecordTimestamp();
#endif

public:
	LogParser();
	LogParser(const LogParser *src);
	~LogParser();

	static ObjectArray<LogParser> *createFromXml(const char *xml, int xmlLen = -1,
		TCHAR *errorText = NULL, int errBufSize = 0, bool (*eventResolver)(const TCHAR *, UINT32 *) = NULL);

   const TCHAR *getName() const { return m_name; }
   const uuid& getGuid() const { return m_guid;  }
	const TCHAR *getFileName() const { return m_fileName; }
	int getFileEncoding() const { return m_fileEncoding; }
   LogParserStatus getStatus() const { return m_status; }
   const TCHAR *getStatusText() const;
   bool isFilePreallocated() const { return m_preallocatedFile; }

	void setName(const TCHAR *name);
   void setFileName(const TCHAR *name);
   void setGuid(const uuid& guid);

	void setThread(THREAD th) { m_thread = th; }
	THREAD getThread() { return m_thread; }

	void setProcessAllFlag(bool flag) { m_processAllRules = flag; }
	bool getProcessAllFlag() const { return m_processAllRules; }

   void setKeepFileOpenFlag(bool flag) { m_keepFileOpen = flag; }
   bool getKeepFileOpenFlag() const { return m_keepFileOpen; }

   void setIgnoreMTimeFlag(bool flag) { m_ignoreMTime = flag; }
   bool getIgnoreMTimeFlag() const { return m_ignoreMTime; }

#ifdef _WIN32
   void setSnapshotMode(bool enable) { m_useSnapshot = enable;  }
   bool isSnapshotMode() const { return m_useSnapshot;  }
#endif

	bool addRule(const TCHAR *regexp, UINT32 eventCode = 0, const TCHAR *eventName = NULL, int repeatInterval = 0, int repeatCount = 0, bool resetRepeat = true);
	bool addRule(LogParserRule *rule);
	void setCallback(LogParserCallback cb) { m_cb = cb; }
	void setUserArg(void *arg) { m_userArg = arg; }
	void setEventNameList(CODE_TO_TEXT *ctt) { m_eventNameList = ctt; }
	void setEventNameResolver(bool (*cb)(const TCHAR *, UINT32 *)) { m_eventResolver = cb; }
	UINT32 resolveEventName(const TCHAR *name, UINT32 defVal = 0);

   void addExclusionSchedule(const TCHAR *schedule) { m_exclusionSchedules.add(schedule); }

	void addMacro(const TCHAR *name, const TCHAR *value);
	const TCHAR *getMacro(const TCHAR *name);

	bool matchLine(const TCHAR *line, UINT32 objectId = 0);
	bool matchEvent(const TCHAR *source, UINT32 eventId, UINT32 level, const TCHAR *line, StringList *variables, UINT64 recordId, UINT32 objectId = 0);

	int getProcessedRecordsCount() const { return m_recordsProcessed; }
	int getMatchedRecordsCount() const { return m_recordsMatched; }

	int getTraceLevel() const { return m_traceLevel; }
	void setTraceLevel(int level) { m_traceLevel = level; }

	bool monitorFile(bool readFromCurrPos = true);
#ifdef _WIN32
   bool monitorEventLog(const TCHAR *markerPrefix);
   void saveLastProcessedRecordTimestamp(time_t timestamp);
#endif

   int getRuleCheckCount(const TCHAR *ruleName, UINT32 objectId = 0) const { const LogParserRule *r = findRuleByName(ruleName); return (r != NULL) ? r->getCheckCount(objectId) : -1; }
   int getRuleMatchCount(const TCHAR *ruleName, UINT32 objectId = 0) const { const LogParserRule *r = findRuleByName(ruleName); return (r != NULL) ? r->getMatchCount(objectId) : -1; }

   void restoreCounters(const LogParser *parser);

   void stop();
   void suspend();
   void resume();

   void trace(int level, const TCHAR *format, ...);
};

/**
 * Init log parser library
 */
void LIBNXLP_EXPORTABLE InitLogParserLibrary();

/**
 * Cleanup event log parsig library
 */
void LIBNXLP_EXPORTABLE CleanupLogParserLibrary();

#ifdef _WIN32

/**
 * Create file snapshot
 */
FileSnapshot LIBNXLP_EXPORTABLE *CreateFileSnapshot(const TCHAR *path);

/**
* Destroy file snapshot
*/
void LIBNXLP_EXPORTABLE DestroyFileSnapshot(FileSnapshot *snapshot);

#endif

#endif
