/*
** NetXMS LogWatch subagent
** Copyright (C) 2008-2014 Victor Kirhenshtein
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

#ifdef _WIN32
THREAD_RESULT THREAD_CALL ParserThreadEventLog(void *);
THREAD_RESULT THREAD_CALL ParserThreadEventLogV6(void *);
bool InitEventLogParsersV6();
#endif

/**
 * Shutdown condition
 */
CONDITION g_hCondShutdown = INVALID_CONDITION_HANDLE;

/**
 * Configured parsers
 */
static ObjectArray<LogParser> s_parsers(16, 16, true);

/**
 * File parsing thread
 */
THREAD_RESULT THREAD_CALL ParserThreadFile(void *arg)
{
	((LogParser *)arg)->monitorFile(g_hCondShutdown, AgentWriteLog);
	return THREAD_OK;
}

/**
 * Get parser statistics
 */
static LONG H_ParserStats(const TCHAR *cmd, const TCHAR *arg, TCHAR *value)
{
	TCHAR name[256];

	if (!AgentGetParameterArg(cmd, 1, name, 256))
		return SYSINFO_RC_UNSUPPORTED;

	LogParser *parser = NULL;
	for(int i = 0; i < s_parsers.size(); i++)
   {
      LogParser *p = m_parsers.get(i);
		if (!_tcsicmp(p->getName(), name))
		{
			parser = p;
			break;
		}
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

/**
 * Get list of configured parsers
 */
static LONG H_ParserList(const TCHAR *cmd, const TCHAR *arg, StringList *value)
{
	for(int i = 0; i < s_parsers.size(); i++)
		value->add(s_parsers.get(i)->getName());
	return SYSINFO_RC_SUCCESS;
}

/**
 * Called by master agent at unload
 */
static void SubagentShutdown()
{
	DWORD i;

	if (g_hCondShutdown != INVALID_CONDITION_HANDLE)
		ConditionSet(g_hCondShutdown);

	for(int i = 0; i < s_parsers.size(); i++)
	{
		ThreadJoin(s_parsers.get(i)->getThread());
	}

#ifdef _WIN32
	CleanupEventLogParsers();
#endif

#ifdef _NETWARE
	// Notify main thread that NLM can exit
	ConditionSet(m_hCondTerminate);
#endif
}

/**
 * Callback for matched log records
 */
static void LogParserMatch(UINT32 eventCode, const TCHAR *eventName, const TCHAR *text,
                           const TCHAR *source, UINT32 eventId, UINT32 severity,
                           int cgCount, TCHAR **cgList, UINT32 objectId, void *userArg)
{
   if (source != NULL)
   {
      TCHAR eventIdText[16], severityText[16];
      _sntprintf(eventIdText, 16, _T("%u"), eventId);
      _sntprintf(severityText, 16, _T("%u"), severity);

      int count = cgCount + 3;
      TCHAR **list = (TCHAR **)malloc(sizeof(TCHAR *) * count);
      int i;
      for(i = 0; i < cgCount; i++)
         list[i] = cgList[i];
      list[i++] = (TCHAR *)source;
      list[i++] = eventIdText;
      list[i++] = severityText;
	   AgentSendTrap2(eventCode, eventName, count, list);
      free(list);
   }
   else
   {
	   AgentSendTrap2(eventCode, eventName, cgCount, cgList);
   }
}

/**
 * Trace callback
 */
static void LogParserTrace(const TCHAR *format, va_list args)
{
	AgentWriteDebugLog2(7, format, args);
}

/**
 * Add parser from config parameter
 */
static void AddParserFromConfig(const TCHAR *file)
{
	BYTE *xml;
	UINT32 size;
	TCHAR error[1024];

	xml = LoadFile(file, &size);
	if (xml != NULL)
	{
		ObjectArray<LogParser> *parsers = LogParser::createFromXml((const char *)xml, size, error, 1024);
		if (parsers != NULL)
		{
			for (int i = 0; i < parsers->size(); i++)
			{
				LogParser *parser = parsers->get(i);
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
					AgentWriteLog(EVENTLOG_ERROR_TYPE, _T("LogWatch: Parser configuration %s missing file name to parse (%d)"), file, i);
				}
			}
		}
		else
		{
			delete parsers;
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
		if (s_parsers.get(i)->getFileName()[0] == _T('*'))	// event log
		{
			s_parsers.get(i)->setThread(ThreadCreateEx(eventLogParserThread, 0, s_parsers.get(i)));
			// Seems that simultaneous calls to OpenEventLog() from two or more threads may
			// cause entire process to hang
			ThreadSleepMs(200);
		}
		else	// regular file
		{
			s_parsers.get(i)->setThread(ThreadCreateEx(ParserThreadFile, 0, s_parsers.get(i)));
		}
#else
		s_parsers.get(i)->setThread(ThreadCreateEx(ParserThreadFile, 0, s_parsers.get(i)));
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
	_T("LOGWATCH"), NETXMS_VERSION_STRING,
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
