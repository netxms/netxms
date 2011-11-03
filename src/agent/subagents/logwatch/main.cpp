/*
** NetXMS LogWatch subagent
** Copyright (C) 2008-2011 Victor Kirhenshtein
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
** File: main.cpp
**
**/

#include "logwatch.h"


//
// Externals
//

#ifdef _WIN32
THREAD_RESULT THREAD_CALL ParserThreadEventLog(void *);
THREAD_RESULT THREAD_CALL ParserThreadEventLogV6(void *);
bool InitEventLogParsersV6();
#endif


//
// Global variables
//

CONDITION g_hCondShutdown = INVALID_CONDITION_HANDLE;


//
// Static data
//

#ifdef _NETWARE
static CONDITION m_hCondTerminate = INVALID_CONDITION_HANDLE;
#endif
static DWORD m_numParsers = 0;
static LogParser **m_parserList = NULL;


//
// File parsing thread
//

THREAD_RESULT THREAD_CALL ParserThreadFile(void *arg)
{
	((LogParser *)arg)->monitorFile(g_hCondShutdown, AgentWriteLog);
	return THREAD_OK;
}


//
// Get parser statistics
//

static LONG H_ParserStats(const TCHAR *cmd, const TCHAR *arg, TCHAR *value)
{
	TCHAR name[256];

	if (!AgentGetParameterArg(cmd, 1, name, 256))
		return SYSINFO_RC_UNSUPPORTED;

	LogParser *parser = NULL;
	for(DWORD i = 0; i < m_numParsers; i++)
		if (!_tcsicmp(m_parserList[i]->getName(), name))
		{
			parser = m_parserList[i];
			break;
		}

	if (parser == NULL)
	{
		AgentWriteDebugLog(8, _T("LogWatch: H_ParserStats: parser with name \"%s\" cannot be found"), name);
		return SYSINFO_RC_UNSUPPORTED;
	}

	switch(*arg)
	{
		case 'S':	// Status
			ret_string(value, parser->getStatus());
			break;
		case 'M':	// Matched records
			ret_int(value, parser->getMatchedRecordsCount());
			break;
		case 'P':	// Processed records
			ret_int(value, parser->getProcessedRecordsCount());
			break;
		default:
			return SYSINFO_RC_UNSUPPORTED;
	}
	return SYSINFO_RC_SUCCESS;
}


//
// Get list of configured parsers
//

static LONG H_ParserList(const TCHAR *cmd, const TCHAR *arg, StringList *value)
{
	for(DWORD i = 0; i < m_numParsers; i++)
		value->add(m_parserList[i]->getName());
	return SYSINFO_RC_SUCCESS;
}


//
// Called by master agent at unload
//

static void SubagentShutdown()
{
	DWORD i;

	if (g_hCondShutdown != INVALID_CONDITION_HANDLE)
		ConditionSet(g_hCondShutdown);

	for(i = 0; i < m_numParsers; i++)
	{
		ThreadJoin(m_parserList[i]->getThread());
		delete m_parserList[i];
	}
	safe_free(m_parserList);

#ifdef _WIN32
	CleanupEventLogParsers();
#endif

#ifdef _NETWARE
	// Notify main thread that NLM can exit
	ConditionSet(m_hCondTerminate);
#endif
}


//
// Callback for matched log records
//

static void LogParserMatch(DWORD event, const TCHAR *text, int paramCount,
                           TCHAR **paramList, DWORD objectId, void *userArg)
{
	AgentSendTrap2(event, paramCount, paramList);
}


//
// Trace callback
//

static void LogParserTrace(const TCHAR *format, va_list args)
{
	AgentWriteDebugLog2(7, format, args);
}


//
// Add parser from config parameter
//

static void AddParserFromConfig(const TCHAR *file)
{
	LogParser *parser;
	BYTE *xml;
	DWORD size;
	TCHAR error[1024];

	xml = LoadFile(file, &size);
	if (xml != NULL)
	{
		parser = new LogParser;
		if (parser->createFromXml((const char *)xml, size, error, 1024))
		{
			if (parser->getFileName() != NULL)
			{
				parser->setCallback(LogParserMatch);
				parser->setTraceCallback(LogParserTrace);
				m_numParsers++;
				m_parserList = (LogParser **)realloc(m_parserList, sizeof(LogParser *) * m_numParsers);
				m_parserList[m_numParsers - 1] = parser;
				AgentWriteDebugLog(1, _T("LogWatch: registered parser for file %s, trace level set to %d"),
				                   parser->getFileName(), parser->getTraceLevel());
#ifdef _WIN32
				AgentWriteDebugLog(7, _T("LogWatch: Process RSS after parser creation is ") INT64_FMT _T(" bytes"), GetProcessRSS());
#endif
			}
			else
			{
				delete parser;
				AgentWriteLog(EVENTLOG_ERROR_TYPE, _T("LogWatch: Parser configuration %s missing file name to parse"), file);
			}
		}
		else
		{
			delete parser;
			AgentWriteLog(EVENTLOG_ERROR_TYPE, _T("LogWatch: Cannot create parser from configuration file %s (%s)"), file, error);
		}
		free(xml);
	}
	else
	{
		AgentWriteLog(EVENTLOG_ERROR_TYPE, _T("LogWatch: Cannot load parser configuration file %s"), file);
	}
}


//
// Subagent initialization
//

static BOOL SubagentInit(Config *config)
{
	ConfigEntry *parsers = config->getEntry(_T("/LogWatch/Parser"));
	if (parsers != NULL)
	{
		for(int i = 0; i < parsers->getValueCount(); i++)
			AddParserFromConfig(parsers->getValue(i));
	}

	// Additional initialization
#ifdef _WIN32
	THREAD_RESULT (THREAD_CALL *eventLogParserThread)(void *);
	if (InitEventLogParsersV6())
	{
		eventLogParserThread = ParserThreadEventLogV6;
	}
	else
	{
		eventLogParserThread = ParserThreadEventLog;
		InitEventLogParsers();
	}
#endif
	// Create shutdown condition and start parsing threads
	g_hCondShutdown = ConditionCreate(TRUE);
	for(int i = 0; i < (int)m_numParsers; i++)
	{
#ifdef _WIN32
		if (m_parserList[i]->getFileName()[0] == _T('*'))	// event log
		{
			m_parserList[i]->setThread(ThreadCreateEx(eventLogParserThread, 0, m_parserList[i]));
			// Seems that simultaneous calls to OpenEventLog() from two or more threads may
			// cause entire process to hang
			ThreadSleepMs(200);
		}
		else	// regular file
		{
			m_parserList[i]->setThread(ThreadCreateEx(ParserThreadFile, 0, m_parserList[i]));
		}
#else
		m_parserList[i]->setThread(ThreadCreateEx(ParserThreadFile, 0, m_parserList[i]));
#endif
	}

	return TRUE;
}


//
// Subagent information
//

static NETXMS_SUBAGENT_PARAM m_parameters[] =
{
	{ _T("LogWatch.Parser.Status(*)"), H_ParserStats, _T("S"), DCI_DT_INT, _T("Parser {instance} status") },
	{ _T("LogWatch.Parser.MatchedRecords(*)"), H_ParserStats, _T("M"), DCI_DT_INT, _T("Number of records matched by parser {instance}") },
	{ _T("LogWatch.Parser.ProcessedRecords(*)"), H_ParserStats, _T("P"), DCI_DT_INT, _T("Number of records processed by parser {instance}") }
};
static NETXMS_SUBAGENT_LIST m_lists[] =
{
	{ _T("LogWatch.ParserList"), H_ParserList, NULL }
};

static NETXMS_SUBAGENT_INFO m_info =
{
	NETXMS_SUBAGENT_INFO_MAGIC,
	_T("LOGWATCH"), _T(NETXMS_VERSION_STRING),
	SubagentInit, SubagentShutdown, NULL,
	sizeof(m_parameters) / sizeof(NETXMS_SUBAGENT_PARAM),
	m_parameters,
	sizeof(m_lists) / sizeof(NETXMS_SUBAGENT_LIST),
	m_lists,
	0, NULL,		// tables
	0, NULL,		// actions
	0, NULL		// push parameters
};


//
// Entry point for NetXMS agent
//

DECLARE_SUBAGENT_ENTRY_POINT(LOGWATCH)
{
	*ppInfo = &m_info;
	return TRUE;
}


//
// DLL entry point
//

#ifdef _WIN32

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	if (dwReason == DLL_PROCESS_ATTACH)
		DisableThreadLibraryCalls(hInstance);
	return TRUE;
}

#endif


//
// NetWare entry point
// We use main() instead of _init() and _fini() to implement
// automatic unload of the subagent after unload handler is called
//

#ifdef _NETWARE

int main(int argc, char *argv[])
{
   m_hCondTerminate = ConditionCreate(TRUE);
   ConditionWait(m_hCondTerminate, INFINITE);
   ConditionDestroy(m_hCondTerminate);
   sleep(1);
   return 0;
}

#endif
