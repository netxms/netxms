/*
** NetXMS - Network Management System
** Log Parsing Library
** Copyright (C) 2003-2024 Raden Solutions
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
#include <vector>

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

#define LOGWATCH_MAX_NUM_CAPTURE_GROUPS 127

/**
 * Capture groups store
 */
class CaptureGroupsStore
{
private:
   MemoryPool m_pool;
   size_t m_numGroups;
   TCHAR *m_values[LOGWATCH_MAX_NUM_CAPTURE_GROUPS];
   StringObjectMap<TCHAR> m_nameIndex;

public:
   CaptureGroupsStore();
   CaptureGroupsStore(const TCHAR *line, int *offsets, int cgcount, const HashMap<uint32_t, String>& groupNames);

   const size_t size() const { return m_numGroups; }
   const TCHAR *getByPosition(size_t position) const { return (position < m_numGroups) ? m_values[position] : nullptr; }
   const TCHAR *getByName(const TCHAR *name) const { return m_nameIndex.get(name); }

   void addAllToMap(StringMap *target) const
   {
      for(KeyValuePair<TCHAR> *e : m_nameIndex)
         target->set(e->key, e->value);
   }
};

/**
 * Data for log parser callback
 */
struct LogParserCallbackData
{
   uint32_t eventCode;
   const TCHAR *eventName;
   const TCHAR *eventTag;
   const TCHAR *originalText;
   const TCHAR *source;
   union
   {
      uint32_t facility;
      uint32_t windowsEventId;
   };
   uint32_t severity;
   uint64_t recordId;
   time_t logRecordTimestamp;
   const StringList *variables;
   const TCHAR *logName;
   const CaptureGroupsStore *captureGroups;
   uint32_t repeatCount;
   uint32_t objectId;
   void *userData;
};

/**
 * Log parser callback
 * Parameters:
 *    NetXMS event code, NetXMS event name, event tag, original text, source,
 *    original event ID (facility), original severity,
 *    capture groups, variables, record id, object id, repeat count,
 *    log record timestamp, user data
 */
typedef std::function<void (const LogParserCallbackData& data)> LogParserCallback;
#ifdef _WIN32
template class LIBNXLP_EXPORTABLE std::function<void(const LogParserCallbackData& data)>;
#endif

/**
 * Log parser action callback
 * Parameters:
 *    NetXMS agent action, agent action arguments, user data
 */
typedef std::function<void (const TCHAR*, const StringList&, void*)> LogParserActionCallback;
#ifdef _WIN32
template class LIBNXLP_EXPORTABLE std::function<void(const TCHAR*, const StringList&, void*)>;
#endif

/**
 * Log parser data push callback
 * Parameters:
 *    NetXMS agent data push parameter name, parameter value
 */
typedef std::function<bool (const TCHAR* parameter, const TCHAR *value)> LogParserDataPushCallback;
#ifdef _WIN32
template class LIBNXLP_EXPORTABLE std::function<bool(const TCHAR* parameter, const TCHAR *value)>;
#endif

/**
 * Log parser copy callback
 * Parameters:
 *    record text, source, event ID (facility), severity, user data
 */
typedef std::function<void (const TCHAR*, const TCHAR*, uint32_t, uint32_t, void*)> LogParserCopyCallback;
#ifdef _WIN32
template class LIBNXLP_EXPORTABLE std::function<void(const TCHAR*, const TCHAR*, uint32_t, uint32_t, void*)>;
#endif

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
 * Metric defined by parser
 */
struct LogParserMetric
{
   TCHAR name[MAX_PARAM_NAME];
   int captureGroup;
   bool push;
   time_t timestamp;
   TCHAR value[MAX_DB_STRING];
};

#ifdef _WIN32
template class LIBNXLP_EXPORTABLE HashMap<uint32_t, ObjectRuleStats>;
template class LIBNXLP_EXPORTABLE HashMap<uint32_t, String>;
template class LIBNXLP_EXPORTABLE StructArray<LogParserMetric>;
#endif

/**
 * Log parser rule
 */
class LIBNXLP_EXPORTABLE LogParserRule
{
   friend class LogParser;

private:
	LogParser *m_parser;
	String m_name;
	PCRE *m_preg;
	uint32_t m_eventCode;
	TCHAR *m_eventName;
	TCHAR *m_eventTag;
	int m_pmatch[LOGWATCH_MAX_NUM_CAPTURE_GROUPS * 3];
	TCHAR *m_regexp;
	TCHAR *m_source;
   uint32_t m_level;
   uint32_t m_idStart;
   uint32_t m_idEnd;
	TCHAR *m_context;
	int m_contextAction;
	TCHAR *m_contextToChange;
   bool m_ignoreCase;
	bool m_isInverted;
	bool m_breakOnMatch;
	bool m_doNotSaveToDatabase;
	TCHAR *m_description;
	int m_repeatInterval;
	int m_repeatCount;
   IntegerArray<time_t> *m_matchArray;
	bool m_resetRepeat;
	int m_checkCount;
	int m_matchCount;
	TCHAR *m_agentAction;
	TCHAR *m_logName;
	StringList *m_agentActionArgs;
	StructArray<LogParserMetric> m_metrics;
	HashMap<uint32_t, ObjectRuleStats> m_objectCounters;
   HashMap<uint32_t, String> m_groupName;

	bool matchInternal(bool extMode, const TCHAR *source, uint32_t eventId, uint32_t level, const TCHAR *line,
	         StringList *variables, uint64_t recordId, uint32_t objectId, time_t timestamp, const TCHAR *logName,
	         LogParserCallback cb, LogParserDataPushCallback cbDataPush, LogParserActionCallback cbAction, void *userData);
	bool matchRepeatCount(int *matchCount);
   void expandMacros(const TCHAR *regexp, StringBuffer &out);
   void incCheckCount(uint32_t objectId);
   void incMatchCount(uint32_t objectId);
   void updateGroupNames();

public:
	LogParserRule(LogParser *parser, const TCHAR *name,
	              const TCHAR *regexp, bool ignoreCase, uint32_t eventCode, const TCHAR *eventName,
					  const TCHAR *eventTag, int repeatInterval, int repeatCount,
					  bool resetRepeat, const StructArray<LogParserMetric>& metrics);
	LogParserRule(LogParserRule *src, LogParser *parser);
	~LogParserRule();

	const TCHAR *getName() const { return m_name.cstr(); }
	bool isValid() const { return m_preg != nullptr; }
	uint32_t getEventCode() const { return m_eventCode; }

   bool match(const TCHAR *line, uint32_t objectId, LogParserCallback cb, LogParserDataPushCallback cbDataPush,
         LogParserActionCallback cbAction, const TCHAR *fileName, void *userData)
   {
      return matchInternal(false, nullptr, 0, 0, line, nullptr, 0, objectId, 0, fileName, cb, cbDataPush, cbAction, userData);
   }
   bool matchEx(const TCHAR *source, uint32_t eventId, uint32_t level, const TCHAR *line, StringList *variables,
         uint64_t recordId, uint32_t objectId, time_t timestamp, const TCHAR *fileName, LogParserCallback cb,
         LogParserDataPushCallback cbDataPush, LogParserActionCallback cbAction, void *userData)
   {
      return matchInternal(true, source, eventId, level, line, variables, recordId, objectId, timestamp, fileName, cb, cbDataPush, cbAction, userData);
   }

   const StructArray<LogParserMetric>& getMetrics() const { return m_metrics; }

	void setLogName(const TCHAR *logName) { MemFree(m_logName); m_logName = MemCopyString(logName); }

   void setAgentAction(const TCHAR *agentAction) { MemFree(m_agentAction); m_agentAction = MemCopyString(agentAction); }
   void setAgentActionArgs(StringList *args) { delete(m_agentActionArgs); m_agentActionArgs = args; }

	void setContext(const TCHAR *context) { MemFree(m_context); m_context = MemCopyString(context); }
	void setContextToChange(const TCHAR *context) { MemFree(m_contextToChange); m_contextToChange = MemCopyString(context); }
	void setContextAction(int action) { m_contextAction = action; }

	void setInverted(bool flag) { m_isInverted = flag; }
	bool isInverted() const { return m_isInverted; }

	void setBreakFlag(bool flag) { m_breakOnMatch = flag; }
	bool getBreakFlag() const { return m_breakOnMatch; }

   void setDoNotSaveToDBFlag(bool flag) { m_doNotSaveToDatabase = flag; }
   bool isDoNotSaveToDBFlag() const { return m_doNotSaveToDatabase; }

	const TCHAR *getContext() const { return m_context; }
	const TCHAR *getContextToChange() const { return m_contextToChange; }
	int getContextAction() const { return m_contextAction; }

	void setDescription(const TCHAR *descr) { MemFree(m_description); m_description = MemCopyString(descr); }
	const TCHAR *getDescription() const { return CHECK_NULL_EX(m_description); }

	void setSource(const TCHAR *source) { MemFree(m_source); m_source = MemCopyString(source); }
	const TCHAR *getSource() const { return CHECK_NULL_EX(m_source); }

	void setLevel(uint32_t level) { m_level = level; }
   uint32_t getLevel() const { return m_level; }

	void setIdRange(uint32_t start, uint32_t end) { m_idStart = start; m_idEnd = end; }
	uint64_t getIdRange() const { return ((uint64_t)m_idStart << 32) | (uint64_t)m_idEnd; }

   void setRepeatInterval(int repeatInterval) { m_repeatInterval = repeatInterval; }
   int getRepeatInterval() const { return m_repeatInterval; }

   void setRepeatCount(int repeatCount) { m_repeatCount = repeatCount; }
   int getRepeatCount() const { return m_repeatCount; }

   void setRepeatReset(bool resetRepeat) { m_resetRepeat = resetRepeat; }
   bool isRepeatReset() const { return m_resetRepeat; }

	const TCHAR *getRegexpSource() const { return CHECK_NULL(m_regexp); }

   int getCheckCount(uint32_t objectId = 0) const;
   int getMatchCount(uint32_t objectId = 0) const;

   void restoreCounters(const LogParserRule& rule);
};

#ifdef _WIN32
template class LIBNXLP_EXPORTABLE ObjectArray<LogParserRule>;
#endif

/**
 * Log parser class
 */
class LIBNXLP_EXPORTABLE LogParser
{
private:
	ObjectArray<LogParserRule> m_rules;
	StringMap m_contexts;
	StringMap m_macros;
	LogParserCallback m_cb;
   LogParserDataPushCallback m_cbDataPush;
	LogParserActionCallback m_cbAction;
   LogParserCopyCallback m_cbCopy;
	void *m_userData;
	TCHAR *m_fileName;
	int m_fileEncoding;
   uint32_t m_fileCheckInterval;
	StringList m_exclusionSchedules;
	TCHAR *m_name;
	CodeLookupElement *m_eventNameList;
	bool (*m_eventResolver)(const TCHAR *, uint32_t *);
	THREAD m_thread;	// Associated thread
   Condition m_stopCondition;
   int m_recordsProcessed;
	int m_recordsMatched;
	bool m_preallocatedFile;
   bool m_detectBrokenPrealloc;
   bool m_keepFileOpen;
   bool m_ignoreMTime;
   bool m_followSymlinks;
   bool m_removeEscapeSequences;
   bool m_rescan;
	bool m_processAllRules;
   bool m_suspended;
	LogParserStatus m_status;
#ifdef _WIN32
   TCHAR *m_marker;
   bool m_useSnapshot;
#endif
   uuid m_guid;
   char *m_readBuffer;
   size_t m_readBufferSize;
   TCHAR *m_textBuffer;
   off_t m_offset;
   TCHAR m_fileNameHash[MD5_DIGEST_SIZE * 2 + 1];

	const TCHAR *checkContext(LogParserRule *rule);
	bool matchLogRecord(bool hasAttributes, const TCHAR *source, uint32_t eventId, uint32_t level, const TCHAR *line,
	         StringList *variables, uint64_t recordId, uint32_t objectId, time_t timestamp, const TCHAR *logName, bool *saveToDatabase);

	bool isExclusionPeriod();

   const LogParserRule *findRuleByName(const TCHAR *name) const;

   void setStatus(LogParserStatus status) { m_status = status; }

   off_t processNewRecords(int fh, const TCHAR *fileName);
   bool monitorFile2(off_t startOffset);

#ifdef _WIN32
   bool monitorFileWithSnapshot(off_t startOffset);
   time_t readLastProcessedRecordTimestamp();
#endif

public:
	LogParser();
	LogParser(const LogParser *src);
	~LogParser();

	static ObjectArray<LogParser> *createFromXml(const char *xml, ssize_t xmlLen = -1,
		TCHAR *errorText = nullptr, size_t errBufSize = 0, bool (*eventResolver)(const TCHAR *, uint32_t *) = nullptr);

   const TCHAR *getName() const { return m_name; }
   const uuid& getGuid() const { return m_guid;  }
	const TCHAR *getFileName() const { return m_fileName; }
	int getFileEncoding() const { return m_fileEncoding; }
   uint32_t getFileCheckInterval() const { return m_fileCheckInterval; }
   LogParserStatus getStatus() const { return m_status; }
   const TCHAR *getStatusText() const;
   bool isFilePreallocated() const { return m_preallocatedFile; }
   bool isDetectBrokenPrealloc() const { return m_detectBrokenPrealloc; }
   bool isFollowSymlinks() const { return m_followSymlinks; }
   bool isRemoveEscapeSequences() const { return m_removeEscapeSequences; }
   void getEventList(HashSet<uint32_t> *eventList) const;
   bool isUsingEvent(uint32_t eventCode) const;
   int getCharSize() const;
   String getIdentifier() const { return (!m_guid.equals(uuid::NULL_UUID)) ? m_guid.toString() : m_fileNameHash; }

	void setName(const TCHAR *name);
   void setFileName(const TCHAR *name);
   void setFileEncoding(int encoding) { m_fileEncoding = encoding; }
   void setFileCheckInterval(uint32_t interval) { m_fileCheckInterval = interval; }
   void setFilePreallocated(bool isPreallocated) { m_preallocatedFile = isPreallocated; }
   void setGuid(const uuid& guid);

	void setThread(THREAD thread) { m_thread = thread; }
	THREAD getThread() { return m_thread; }
   Condition *getStopCondition() { return &m_stopCondition; }
   off_t getCurrentOffset() const { return m_offset; }

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

	bool addRule(LogParserRule *rule);
	void setCallback(LogParserCallback cb) { m_cb = cb; }
   void setActionCallback(LogParserActionCallback cb) { m_cbAction = cb; }
   void setDataPushCallback(LogParserDataPushCallback cb) { m_cbDataPush = cb; }
   void setCopyCallback(LogParserCopyCallback cb) { m_cbCopy = cb; }
   void setUserData(void *userData) { m_userData = userData; }
	void setEventNameList(CodeLookupElement *lookupTable) { m_eventNameList = lookupTable; }
	void setEventNameResolver(bool (*cb)(const TCHAR *, uint32_t *)) { m_eventResolver = cb; }
   uint32_t resolveEventName(const TCHAR *name, uint32_t defaultValue = 0);

   void addExclusionSchedule(const TCHAR *schedule) { m_exclusionSchedules.add(schedule); }

	void addMacro(const TCHAR *name, const TCHAR *value);
	const TCHAR *getMacro(const TCHAR *name);

	bool matchLine(const TCHAR *line, const TCHAR *logName, uint32_t objectId = 0);
	bool matchEvent(const TCHAR *source, uint32_t eventId, uint32_t level, const TCHAR *line, StringList *variables,
	         uint64_t recordId, uint32_t objectId = 0, time_t timestamp = 0, const TCHAR *logName = nullptr, bool *saveToDatabase = nullptr);

	int getProcessedRecordsCount() const { return m_recordsProcessed; }
	int getMatchedRecordsCount() const { return m_recordsMatched; }

	std::vector<const LogParserMetric*> getMetrics();
	bool getMetricValue(const TCHAR *name, TCHAR *buffer, size_t size);
   bool getMetricTimestamp(const TCHAR *name, time_t *timestamp);

   off_t scanFile(int fh, off_t startOffset, const TCHAR *fileName);
	bool monitorFile(off_t startOffset);
   
#ifdef _WIN32
   bool monitorEventLog(const TCHAR *markerPrefix);
   void saveLastProcessedRecordTimestamp(time_t timestamp);
#endif

   int getRuleCheckCount(const TCHAR *ruleName, uint32_t objectId = 0) const
   {
      const LogParserRule *r = findRuleByName(ruleName);
      return (r != nullptr) ? r->getCheckCount(objectId) : -1;
   }
   int getRuleMatchCount(const TCHAR *ruleName, uint32_t objectId = 0) const
   {
      const LogParserRule *r = findRuleByName(ruleName);
      return (r != nullptr) ? r->getMatchCount(objectId) : -1;
   }

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

/**
 * Skip block of zeroes within file
 */
bool LIBNXLP_EXPORTABLE SkipZeroBlock(int fh, int chsize);

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
