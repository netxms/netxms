/* 
** NetXMS - Network Management System
** Log Parsing Library
** Copyright (C) 2008 Victor Kirhenshtein
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


#ifdef _WIN32
#include <netxms-regex.h>
#else
#include <regex.h>
#endif


//
// Context actions
//

#define CONTEXT_SET_MANUAL    0
#define CONTEXT_SET_AUTOMATIC 1
#define CONTEXT_CLEAR         2


//
// Callback
// Parameters:
//    event id, original text, number of parameters, list of parameters,
//    object id, user arg
//

typedef void (* LOG_PARSER_CALLBACK)(DWORD, const char *, int, char **, DWORD, void *);


//
// Log parser rule
//

class LIBNXLP_EXPORTABLE LogParserRule
{
private:
	regex_t m_preg;
	DWORD m_event;
	BOOL m_isValid;
	int m_numParams;
	regmatch_t *m_pmatch;
	TCHAR *m_source;
	DWORD m_level;
	DWORD m_idStart;
	DWORD m_idEnd;
	TCHAR *m_context;
	int m_contextAction;
	TCHAR *m_contextToChange;

public:
	LogParserRule(const char *regexp, DWORD event = 0, int numParams = 0,
	              const TCHAR *source = NULL, DWORD level = 0xFFFFFFFF,
					  DWORD idStart = 0xFFFFFFFF, DWORD idEnd = 0xFFFFFFFF);
	~LogParserRule();

	BOOL IsValid() { return m_isValid; }
	BOOL Match(const char *line, LOG_PARSER_CALLBACK cb, DWORD objectId, void *userArg);
	
	void SetContext(const TCHAR *context) { safe_free(m_context); m_context = (context != NULL) ? _tcsdup(context) : NULL; }
	void SetContextToChange(const TCHAR *context) { safe_free(m_contextToChange); m_contextToChange = (context != NULL) ? _tcsdup(context) : NULL; }
	void SetContextAction(int action) { m_contextAction = action; }
	
	const TCHAR *GetContext() { return m_context; }
	const TCHAR *GetContextToChange() { return m_contextToChange; }
	int GetContextAction() { return m_contextAction; }
};


//
// Log parser class
//

class LIBNXLP_EXPORTABLE LogParser
{
private:
	int m_numRules;
	LogParserRule **m_rules;
	StringMap m_contexts;
	LOG_PARSER_CALLBACK m_cb;
	void *m_userArg;
	TCHAR *m_fileName;
	CODE_TO_TEXT *m_eventNameList;
	BOOL (*m_eventResolver)(const TCHAR *, DWORD *);
	THREAD m_thread;	// Associated thread
	int m_recordsProcessed;
	int m_recordsMatched;
	
	const TCHAR *CheckContext(LogParserRule *rule);

public:
	LogParser();
	~LogParser();
	
	BOOL CreateFromXML(const char *xml, int xmlLen = -1, char *errorText = NULL, int errBufSize = 0);

	void SetFileName(const TCHAR *name);
	const TCHAR *GetFileName() { return m_fileName; }

	void SetThread(THREAD th) { m_thread = th; }
	THREAD GetThread() { return m_thread; }

	BOOL AddRule(const char *regexp, DWORD event = 0, int numParams = 0);
	BOOL AddRule(LogParserRule *rule);
	void SetCallback(LOG_PARSER_CALLBACK cb) { m_cb = cb; }
	void SetUserArg(void *arg) { m_userArg = arg; }
	void SetEventNameList(CODE_TO_TEXT *ctt) { m_eventNameList = ctt; }
	void SetEventNameResolver(BOOL (*cb)(const TCHAR *, DWORD *)) { m_eventResolver = cb; }
	DWORD ResolveEventName(const TCHAR *name, DWORD defVal = 0);

	BOOL MatchLine(const char *line, DWORD objectId = 0);

	int GetProcessedRecordsCount() { return m_recordsProcessed; }
	int GetMatchedRecordsCount() { return m_recordsMatched; }

#ifdef _WIN32
	BOOL MonitorFile(HANDLE stopEvent, void (*logger)(int, const TCHAR *, ...));
#else
	BOOL MonitorFile(CONDITION stopCondition, BOOL *stopFlag, void (*logger)(int, const TCHAR *, ...));
#endif
};

#endif
