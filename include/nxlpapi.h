/* 
** NetXMS - Network Management System
** Log Parsing Library
** Copyright (C) 2003-2012 Victor Kirhenshtein
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

#ifdef _WIN32
#ifdef LIBNXLP_EXPORTS
#define LIBNXLP_EXPORTABLE __declspec(dllexport)
#else
#define LIBNXLP_EXPORTABLE __declspec(dllimport)
#endif
#else    /* _WIN32 */
#define LIBNXLP_EXPORTABLE
#endif

#include <netxms-regex.h>
#include <nms_util.h>


//
// Parser status
//

#define MAX_PARSER_STATUS_LEN	64

#define LPS_INIT              _T("INIT")
#define LPS_RUNNING           _T("RUNNING")
#define LPS_NO_FILE           _T("FILE MISSING")
#define LPS_OPEN_ERROR        _T("FILE OPEN ERROR")


//
// Context actions
//

#define CONTEXT_SET_MANUAL    0
#define CONTEXT_SET_AUTOMATIC 1
#define CONTEXT_CLEAR         2


//
// File encoding
//

#define LP_FCP_ACP      0
#define LP_FCP_UTF8     1
#define LP_FCP_UCS2     2
#define LP_FCP_UCS2_LE  3
#define LP_FCP_UCS4     4
#define LP_FCP_UCS4_LE  5


//
// Callback
// Parameters:
//    event id, event name, original text, number of parameters, list of parameters,
//    object id, user arg
//

typedef void (* LogParserCallback)(DWORD, const TCHAR *, const TCHAR *, int, TCHAR **, DWORD, void *);

//
// Log parser rule
//

class LIBNXLP_EXPORTABLE LogParser;

class LIBNXLP_EXPORTABLE LogParserRule
{
private:
	LogParser *m_parser;
	regex_t m_preg;
	DWORD m_eventCode;
	TCHAR *m_eventName;
	bool m_isValid;
	int m_numParams;
	regmatch_t *m_pmatch;
	TCHAR *m_regexp;
	TCHAR *m_source;
	DWORD m_level;
	DWORD m_idStart;
	DWORD m_idEnd;
	TCHAR *m_context;
	int m_contextAction;
	TCHAR *m_contextToChange;
	bool m_isInverted;
	bool m_breakOnMatch;
	TCHAR *m_description;

	void expandMacros(const TCHAR *regexp, String &out);

public:
	LogParserRule(LogParser *parser,
	              const TCHAR *regexp, DWORD eventCode = 0, const TCHAR *eventName = NULL,
					  int numParams = 0, const TCHAR *source = NULL, DWORD level = 0xFFFFFFFF,
					  DWORD idStart = 0, DWORD idEnd = 0xFFFFFFFF);
	LogParserRule(LogParserRule *src, LogParser *parser);
	~LogParserRule();

	bool isValid() { return m_isValid; }
	bool match(const TCHAR *line, LogParserCallback cb, DWORD objectId, void *userArg);
	bool matchEx(const TCHAR *source, DWORD eventId, DWORD level,
	             const TCHAR *line, LogParserCallback cb, DWORD objectId, void *userArg); 
	
	void setContext(const TCHAR *context) { safe_free(m_context); m_context = (context != NULL) ? _tcsdup(context) : NULL; }
	void setContextToChange(const TCHAR *context) { safe_free(m_contextToChange); m_contextToChange = (context != NULL) ? _tcsdup(context) : NULL; }
	void setContextAction(int action) { m_contextAction = action; }

	void setInverted(bool flag) { m_isInverted = flag; }
	BOOL isInverted() { return m_isInverted; }

	void setBreakFlag(bool flag) { m_breakOnMatch = flag; }
	BOOL getBreakFlag() { return m_breakOnMatch; }

	const TCHAR *getContext() { return m_context; }
	const TCHAR *getContextToChange() { return m_contextToChange; }
	int getContextAction() { return m_contextAction; }

	void setDescription(const TCHAR *descr) { safe_free(m_description); m_description = (descr != NULL) ? _tcsdup(descr) : NULL; }
	const TCHAR *getDescription() { return CHECK_NULL_EX(m_description); }

	void setSource(const TCHAR *source) { safe_free(m_source); m_source = (source != NULL) ? _tcsdup(source) : NULL; }
	const TCHAR *getSource() { return CHECK_NULL_EX(m_source); }

	void setLevel(DWORD level) { m_level = level; }
	DWORD getLevel() { return m_level; }

	void setIdRange(DWORD start, DWORD end) { m_idStart = start; m_idEnd = end; }
	QWORD getIdRange() { return ((QWORD)m_idStart << 32) | (QWORD)m_idEnd; }

	const TCHAR *getRegexpSource() { return CHECK_NULL(m_regexp); }
};


//
// Log parser class
//

class LIBNXLP_EXPORTABLE LogParser
{
	friend bool LogParserRule::match(const TCHAR *, LogParserCallback, DWORD, void *);
	friend bool LogParserRule::matchEx(const TCHAR *, DWORD, DWORD, const TCHAR *, LogParserCallback, DWORD, void *);

private:
	int m_numRules;
	LogParserRule **m_rules;
	StringMap m_contexts;
	StringMap m_macros;
	LogParserCallback m_cb;
	void *m_userArg;
	TCHAR *m_fileName;
	int m_fileEncoding;
	TCHAR *m_name;
	CODE_TO_TEXT *m_eventNameList;
	bool (*m_eventResolver)(const TCHAR *, DWORD *);
	THREAD m_thread;	// Associated thread
	int m_recordsProcessed;
	int m_recordsMatched;
	bool m_processAllRules;
	int m_traceLevel;
	void (*m_traceCallback)(const TCHAR *, va_list);
	TCHAR m_status[MAX_PARSER_STATUS_LEN];
	
	const TCHAR *checkContext(LogParserRule *rule);
	void trace(int level, const TCHAR *format, ...);
	bool matchLogRecord(bool hasAttributes, const TCHAR *source, DWORD eventId, DWORD level, const TCHAR *line, DWORD objectId);

public:
	LogParser();
	LogParser(LogParser *src);
	~LogParser();
	
	static ObjectArray<LogParser> *createFromXml(const char *xml, int xmlLen = -1, 
		TCHAR *errorText = NULL, int errBufSize = 0, bool (*eventResolver)(const TCHAR *, DWORD *) = NULL);

	void setFileName(const TCHAR *name);
	const TCHAR *getFileName() { return m_fileName; }

	void setFileEncoding(int encoding) { m_fileEncoding = encoding; }
	int getFileEncoding() { return m_fileEncoding; }

	void setName(const TCHAR *name);
	const TCHAR *getName() { return m_name; }

	void setStatus(const TCHAR *status) { nx_strncpy(m_status, status, MAX_PARSER_STATUS_LEN); }
	const TCHAR *getStatus() { return m_status; }

	void setThread(THREAD th) { m_thread = th; }
	THREAD getThread() { return m_thread; }

	void setProcessAllFlag(bool flag) { m_processAllRules = flag; }
	bool getProcessAllFlag() { return m_processAllRules; }

	bool addRule(const TCHAR *regexp, DWORD eventCode = 0, const TCHAR *eventName = NULL, int numParams = 0);
	bool addRule(LogParserRule *rule);
	void setCallback(LogParserCallback cb) { m_cb = cb; }
	void setUserArg(void *arg) { m_userArg = arg; }
	void setEventNameList(CODE_TO_TEXT *ctt) { m_eventNameList = ctt; }
	void setEventNameResolver(bool (*cb)(const TCHAR *, DWORD *)) { m_eventResolver = cb; }
	DWORD resolveEventName(const TCHAR *name, DWORD defVal = 0);

	void addMacro(const TCHAR *name, const TCHAR *value);
	const TCHAR *getMacro(const TCHAR *name);

	bool matchLine(const TCHAR *line, DWORD objectId = 0);
	bool matchEvent(const TCHAR *source, DWORD eventId, DWORD level, const TCHAR *line, DWORD objectId = 0);

	int getProcessedRecordsCount() { return m_recordsProcessed; }
	int getMatchedRecordsCount() { return m_recordsMatched; }

	int getTraceLevel() { return m_traceLevel; }
	void setTraceLevel(int level) { m_traceLevel = level; }
	void setTraceCallback(void (*cb)(const TCHAR *, va_list)) { m_traceCallback = cb; }

	bool monitorFile(CONDITION stopCondition, void (*logger)(int, const TCHAR *, ...), bool readFromCurrPos = true);
};

#endif
