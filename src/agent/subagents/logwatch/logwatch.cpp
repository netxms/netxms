/*
** NetXMS LogWatch subagent
** Copyright (C) 2008-2017 Victor Kirhenshtein
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
** File: logwatch.cpp
**
**/

#include "logwatch.h"

#define DEBUG_TAG _T("logwatch")

/**
 * Configured parsers
 */
static ObjectArray<LogParser> s_parsers(16, 16, true);
static Mutex s_parserLock;

/**
 * Offline (missed during agent's downtime) events processing flag
 */
static bool s_processOfflineEvents;

/**
 * File parsing thread
 */
THREAD_RESULT THREAD_CALL ParserThreadFile(void *arg)
{
   static_cast<LogParser*>(arg)->monitorFile();
	return THREAD_OK;
}

#ifdef _WIN32

/**
 * Event log parsing thread
 */
THREAD_RESULT THREAD_CALL ParserThreadEventLog(void *arg)
{
   static_cast<LogParser*>(arg)->monitorEventLog(s_processOfflineEvents ? _T("LogWatch") : NULL);
	return THREAD_OK;
}

#endif

/**
 * Get parser statistics
 */
static LONG H_ParserStats(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
	TCHAR name[256];

	if (!AgentGetParameterArg(cmd, 1, name, 256))
		return SYSINFO_RC_UNSUPPORTED;

   s_parserLock.lock();

	LogParser *parser = NULL;
	for(int i = 0; i < s_parsers.size(); i++)
   {
      LogParser *p = s_parsers.get(i);
		if (!_tcsicmp(p->getName(), name))
		{
			parser = p;
			break;
		}
   }

   LONG rc = SYSINFO_RC_SUCCESS;
   if (parser != NULL)
   {
      switch (*arg)
      {
         case 'S':	// Status
            ret_string(value, parser->getStatusText());
            break;
         case 'M':	// Matched records
            ret_int(value, parser->getMatchedRecordsCount());
            break;
         case 'P':	// Processed records
            ret_int(value, parser->getProcessedRecordsCount());
            break;
         default:
            rc = SYSINFO_RC_UNSUPPORTED;
            break;
      }
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 8, _T("H_ParserStats: parser with name \"%s\" cannot be found"), name);
      rc = SYSINFO_RC_UNSUPPORTED;
   }

   s_parserLock.unlock();
   return rc;
}

/**
 * Get list of configured parsers
 */
static LONG H_ParserList(const TCHAR *cmd, const TCHAR *arg, StringList *value, AbstractCommSession *session)
{
   s_parserLock.lock();
   for(int i = 0; i < s_parsers.size(); i++)
		value->add(s_parsers.get(i)->getName());
   s_parserLock.unlock();
   return SYSINFO_RC_SUCCESS;
}

/**
 * Called by master agent at unload
 */
static void SubagentShutdown()
{
   for(int i = 0; i < s_parsers.size(); i++)
      s_parsers.get(i)->stop();

   CleanupLogParserLibrary();
}

/**
 * Callback for matched log records
 */
static void LogParserMatch(UINT32 eventCode, const TCHAR *eventName, const TCHAR *text, const TCHAR *source, UINT32 eventId,
                           UINT32 severity, StringList *cgs, StringList *variables, UINT64 recordId, UINT32 objectId,
                           int repeatCount, void *userArg, const TCHAR *agentAction, const StringList *agentActionArgs)
{
   int count = cgs->size() + 1 + ((variables != NULL) ? variables->size() : 0);
   TCHAR eventIdText[16], severityText[16], repeatCountText[16], recordIdText[32];
   _sntprintf(repeatCountText, 16, _T("%d"), repeatCount);
   if (source != NULL)
   {
      _sntprintf(eventIdText, 16, _T("%u"), eventId);
      _sntprintf(severityText, 16, _T("%u"), severity);
      _sntprintf(recordIdText, 32, UINT64_FMT, recordId);
      count += 4;
   }

   const TCHAR **list = (const TCHAR **)malloc(sizeof(const TCHAR *) * count);
   int i;
   for(i = 0; i < cgs->size(); i++)
      list[i] = cgs->get(i);

   if (source != NULL)
   {
      list[i++] = source;
      list[i++] = eventIdText;
      list[i++] = severityText;
      list[i++] = recordIdText;
   }
   list[i++] = repeatCountText;

   if (variables != NULL)
   {
      for(int j = 0; j < variables->size(); j++)
         list[i++] = variables->get(j);
   }

   AgentExecuteAction(agentAction, agentActionArgs);
   AgentSendTrap2(eventCode, eventName, count, list);
   free(list);
}

/**
 * Add parser from config parameter
 */
static void AddParserFromConfig(const TCHAR *file, const uuid& guid)
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
               parser->setGuid(guid);
               s_parsers.add(parser);
               nxlog_debug_tag(DEBUG_TAG, 1, _T("Registered parser for file \"%s\", GUID %s, trace level %d"),
						parser->getFileName(), (const TCHAR *)guid.toString(), parser->getTraceLevel());
#ifdef _WIN32
					nxlog_debug_tag(DEBUG_TAG, 5, _T("Process RSS after parser creation is ") INT64_FMT _T(" bytes"), GetProcessRSS());
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

/**
 * Add to logwatch everything inside logwatch policy folder
 */
static void AddLogwatchPolicyFiles()
{
   const TCHAR *dataDir = AgentGetDataDirectory();
   TCHAR tail = dataDir[_tcslen(dataDir) - 1];
   
   TCHAR policyFolder[MAX_PATH];
   _sntprintf(policyFolder, MAX_PATH, _T("%s%s%s"), dataDir,
	           ((tail != '\\') && (tail != '/')) ? FS_PATH_SEPARATOR : _T(""),
              LOGPARSER_AP_FOLDER FS_PATH_SEPARATOR);

   nxlog_debug_tag(DEBUG_TAG, 1, _T("AddLogwatchPolicyFiles(): Log parser policy directory: %s"), policyFolder);

   _TDIR *dir = _topendir(policyFolder);
   if (dir != NULL)
   {
      struct _tdirent *d;
      while((d = _treaddir(dir)) != NULL)
      {
         if (!_tcscmp(d->d_name, _T(".")) || !_tcscmp(d->d_name, _T("..")))
         {
            continue;
         }

         TCHAR fullName[MAX_PATH];
         _tcslcpy(fullName, policyFolder, MAX_PATH);
         _tcslcat(fullName, d->d_name, MAX_PATH);

         NX_STAT_STRUCT st;
         if (CALL_STAT(fullName, &st) == 0)
         {
            if (S_ISREG(st.st_mode))
            {
               TCHAR buffer[128];
               TCHAR *p = _tcschr(d->d_name, _T('.'));
               if (p != NULL)
               {
                  size_t len = p - d->d_name;
                  if (len > 127)
                     len = 127;
                  memcpy(buffer, d->d_name, len * sizeof(TCHAR));
                  buffer[len] = 0;
               }
               else
               {
                  _tcslcpy(buffer, d->d_name, 128);
               }
               nxlog_debug_tag(DEBUG_TAG, 5, _T("Processing log parser policy %s"), buffer);
               AddParserFromConfig(fullName, uuid::parse(buffer));
            }
         }
      }
      _tclosedir(dir);
   }
}

/**
 * Subagent initialization
 */
static bool SubagentInit(Config *config)
{
   InitLogParserLibrary();

   s_processOfflineEvents = config->getValueAsBoolean(_T("/LogWatch/ProcessOfflineEvents"), false);

	ConfigEntry *parsers = config->getEntry(_T("/LogWatch/Parser"));
	if (parsers != NULL)
	{
		for(int i = 0; i < parsers->getValueCount(); i++)
			AddParserFromConfig(parsers->getValue(i), uuid::NULL_UUID);
	}
   AddLogwatchPolicyFiles();

	// Start parsing threads
   for(int i = 0; i < s_parsers.size(); i++)
	{
      LogParser *p = s_parsers.get(i);
#ifdef _WIN32
		if (p->getFileName()[0] == _T('*'))	// event log
		{
			p->setThread(ThreadCreateEx(ParserThreadEventLog, 0, p));
			// Seems that simultaneous calls to OpenEventLog() from two or more threads may
			// cause entire process to hang
			ThreadSleepMs(200);
		}
		else	// regular file
		{
			p->setThread(ThreadCreateEx(ParserThreadFile, 0, p));
		}
#else
		p->setThread(ThreadCreateEx(ParserThreadFile, 0, p));
#endif
	}

	return true;
}

/**
 * Agent notification handler
 */
static void OnAgentNotify(UINT32 code, void *data)
{
   if (code != AGENT_NOTIFY_POLICY_INSTALLED)
      return;

   // data points to uuid object
   uuid policyId = *static_cast<uuid*>(data);

   s_parserLock.lock();
   for (int i = 0; i < s_parsers.size(); i++)
   {
      LogParser *p = s_parsers.get(i);
      if (!p->getGuid().equals(policyId))
         continue;

      nxlog_debug_tag(DEBUG_TAG, 3, _T("Reloading parser for file %s"), p->getFileName());
      p->stop();
      s_parsers.remove(i);
      i--;
   }

   const TCHAR *dataDir = AgentGetDataDirectory();
   TCHAR tail = dataDir[_tcslen(dataDir) - 1];

   TCHAR policyFile[MAX_PATH];
   _sntprintf(policyFile, MAX_PATH, _T("%s%s%s%s.xml"), dataDir,
      ((tail != '\\') && (tail != '/')) ? FS_PATH_SEPARATOR : _T(""),
      LOGPARSER_AP_FOLDER FS_PATH_SEPARATOR, (const TCHAR *)policyId.toString());
   AddParserFromConfig(policyFile, policyId);

   // Start parsing threads
   for (int i = 0; i < s_parsers.size(); i++)
   {
      LogParser *p = s_parsers.get(i);
      if (!p->getGuid().equals(policyId))
         continue;

#ifdef _WIN32
      if (p->getFileName()[0] == _T('*'))	// event log
      {
         p->setThread(ThreadCreateEx(ParserThreadEventLog, 0, p));
         // Seems that simultaneous calls to OpenEventLog() from two or more threads may
         // cause entire process to hang
         ThreadSleepMs(200);
      }
      else	// regular file
      {
         p->setThread(ThreadCreateEx(ParserThreadFile, 0, p));
      }
#else
      p->setThread(ThreadCreateEx(ParserThreadFile, 0, p));
#endif
   }

   s_parserLock.unlock();
}

/**
 * Supported parameters
 */
static NETXMS_SUBAGENT_PARAM s_parameters[] =
{
	{ _T("LogWatch.Parser.Status(*)"), H_ParserStats, _T("S"), DCI_DT_STRING, _T("Parser {instance} status") },
	{ _T("LogWatch.Parser.MatchedRecords(*)"), H_ParserStats, _T("M"), DCI_DT_INT, _T("Number of records matched by parser {instance}") },
	{ _T("LogWatch.Parser.ProcessedRecords(*)"), H_ParserStats, _T("P"), DCI_DT_INT, _T("Number of records processed by parser {instance}") }
};

/**
 * Supported lists
 */
static NETXMS_SUBAGENT_LIST s_lists[] =
{
	{ _T("LogWatch.ParserList"), H_ParserList, NULL }
};

/**
 * Subagent information
 */
static NETXMS_SUBAGENT_INFO m_info =
{
	NETXMS_SUBAGENT_INFO_MAGIC,
	_T("LOGWATCH"), NETXMS_BUILD_TAG,
	SubagentInit, SubagentShutdown, NULL, OnAgentNotify,
	sizeof(s_parameters) / sizeof(NETXMS_SUBAGENT_PARAM),
	s_parameters,
	sizeof(s_lists) / sizeof(NETXMS_SUBAGENT_LIST),
	s_lists,
	0, NULL,		// tables
	0, NULL,		// actions
	0, NULL		// push parameters
};

/**
 * Entry point for NetXMS agent
 */
DECLARE_SUBAGENT_ENTRY_POINT(LOGWATCH)
{
	*ppInfo = &m_info;
	return true;
}

#ifdef _WIN32

/**
 * DLL entry point
 */
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	if (dwReason == DLL_PROCESS_ATTACH)
		DisableThreadLibraryCalls(hInstance);
	return TRUE;
}

#endif
