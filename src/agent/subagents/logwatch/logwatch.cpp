/*
** NetXMS LogWatch subagent
** Copyright (C) 2008-2025 Raden Solutions
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
#include <nxstat.h>
#include <netxms-version.h>

#define DEBUG_TAG _T("logwatch")

/**
 * Configured parsers
 */
static ObjectArray<LogParser> s_parsers(0, 16, Ownership::True);
static ObjectArray<LogParser> s_templateParsers(0, 16, Ownership::True);
static Mutex s_parserLock;

/**
 * Offline (missed during agent's downtime) events processing flag
 */
static bool s_processOfflineEvents;

/**
 * File offset retention time
 */
static uint32_t s_offsetRetentionTime;

/**
 * File parsing thread
 */
static void ParserThreadFile(LogParser *parser, off_t startOffset)
{
   parser->monitorFile(startOffset);
}

#ifdef _WIN32

/**
 * Event log parsing thread
 */
static void ParserThreadEventLog(LogParser *parser)
{
   parser->monitorEventLog(s_processOfflineEvents ? _T("LogWatch") : nullptr);
}

#endif

/**
 * Get parser statistics
 */
static LONG H_ParserStats(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
	TCHAR name[256];
	if (!AgentGetMetricArg(cmd, 1, name, 256))
		return SYSINFO_RC_UNSUPPORTED;

   TCHAR fileName[MAX_PATH];
   AgentGetMetricArg(cmd, 2, fileName, MAX_PATH);

   s_parserLock.lock();

	LogParser *parser = nullptr;
	for(int i = 0; i < s_parsers.size(); i++)
   {
      LogParser *p = s_parsers.get(i);
		if (!_tcsicmp(p->getName(), name) && ((*fileName == 0) || !_tcsicmp(p->getFileName(), fileName)))
      {
         parser = p;
         break;
      }
   }

   LONG rc = SYSINFO_RC_SUCCESS;
   if (parser != nullptr)
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
         case 'T':   // Metric timestamp
            if (AgentGetMetricArg(cmd, 3, name, 256))
            {
               time_t timestamp;
               if (parser->getMetricTimestamp(name, &timestamp))
                  ret_int64(value, static_cast<int64_t>(timestamp));
               else
                  rc = SYSINFO_RC_NO_SUCH_INSTANCE;
            }
            else
            {
               rc = SYSINFO_RC_UNSUPPORTED;
            }
            break;
         case 'V':   // Metric value
            if (AgentGetMetricArg(cmd, 3, name, 256))
            {
               if (!parser->getMetricValue(name, value, MAX_RESULT_LENGTH))
                  rc = SYSINFO_RC_NO_SUCH_INSTANCE;
            }
            else
            {
               rc = SYSINFO_RC_UNSUPPORTED;
            }
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
 * Get metric data
 */
static LONG H_Metric(const TCHAR *cmd, const TCHAR *arg, TCHAR *value, AbstractCommSession *session)
{
   TCHAR name[256];
   if (!AgentGetParameterArg(cmd, 1, name, 256))
      return SYSINFO_RC_UNSUPPORTED;

   s_parserLock.lock();

   LONG rc = SYSINFO_RC_NO_SUCH_INSTANCE;
   for(int i = 0; i < s_parsers.size(); i++)
   {
      LogParser *p = s_parsers.get(i);
      if (*arg == 'T')
      {
         time_t timestamp;
         if (p->getMetricTimestamp(name, &timestamp))
         {
            ret_int64(value, static_cast<int64_t>(timestamp));
            rc = SYSINFO_RC_SUCCESS;
            break;
         }
      }
      else
      {
         if (p->getMetricValue(name, value, MAX_RESULT_LENGTH))
         {
            rc = SYSINFO_RC_SUCCESS;
            break;
         }
      }
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
 * Get list of parser metrics
 */
static LONG H_MetricList(const TCHAR *cmd, const TCHAR *arg, StringList *value, AbstractCommSession *session)
{
   s_parserLock.lock();
   for(int i = 0; i < s_parsers.size(); i++)
   {
      std::vector<const LogParserMetric*> metrics = s_parsers.get(i)->getMetrics();
      for(int j = 0; j < metrics.size(); j++)
         value->add(metrics.at(j)->name);
   }
   s_parserLock.unlock();
   return SYSINFO_RC_SUCCESS;
}

/**
 * Get all parser metrics as table
 */
static LONG H_MetricTable(const TCHAR *cmd, const TCHAR *arg, Table *value, AbstractCommSession *session)
{
   value->addColumn(_T("NAME"), DCI_DT_STRING, _T("Name"), true);
   value->addColumn(_T("TIMESTAMP"), DCI_DT_INT64, _T("Timestamp"));
   value->addColumn(_T("VALUE"), DCI_DT_STRING, _T("Value"));
   value->addColumn(_T("PUSH"), DCI_DT_STRING, _T("Push"));

   s_parserLock.lock();
   for(int i = 0; i < s_parsers.size(); i++)
   {
      std::vector<const LogParserMetric*> metrics = s_parsers.get(i)->getMetrics();
      for(int j = 0; j < metrics.size(); j++)
      {
         value->addRow();
         const LogParserMetric *m = metrics.at(j);
         value->set(0, m->name);
         value->set(1, static_cast<int64_t>(m->timestamp));
         value->set(2, m->value);
         value->set(3, BooleanToString(m->push));
      }
   }
   s_parserLock.unlock();
   return SYSINFO_RC_SUCCESS;
}

/**
 * Save parser file position to database
 */
static void SaveParserPosition(DB_HANDLE hdb, const LogParser *p)
{
   off_t currentPosition = p->getCurrentOffset();
   if (currentPosition < 0 || p->getStatus() != LogParserStatus::LPS_RUNNING)
      return;

   String parserId = DBPrepareString(hdb, p->getIdentifier());
   String fileName = DBPrepareString(hdb, p->getFileName());

   TCHAR query[1024];
   _sntprintf(query, 1024, _T("INSERT OR REPLACE INTO logwatch_files (name,path,size,last_update_time) VALUES (%s,%s,") UINT64_FMT _T(",%u)"),
      parserId.cstr(), fileName.cstr(), static_cast<int64_t>(currentPosition), static_cast<uint32_t>(time(nullptr)));

   if (!DBQuery(hdb, query))
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Failed to save file position for parser %s (file: %s)"), p->getName(), p->getFileName());
   }
}

/**
 * Save parser file position to database
 */
static void SaveParserPosition(const LogParser *p)
{
   DB_HANDLE hdb = AgentGetLocalDatabaseHandle();
   if (hdb != nullptr)
      SaveParserPosition(hdb, p);
}

/**
 * Save current file positions for all running parsers
 */
static void SaveFilePositions()
{
   DB_HANDLE hdb = AgentGetLocalDatabaseHandle();
   if (hdb == nullptr)
   {
      s_parserLock.lock();
      for (int i = 0; i < s_parsers.size(); i++)
      {
         SaveParserPosition(hdb, s_parsers.get(i));
      }
      s_parserLock.unlock();
   }

   AgentSetTimer(60000, SaveFilePositions);  // every minute
}

/**
 * Delete old file positions from database
 */
static void DeleteOldFilePositions()
{
   DB_HANDLE hdb = AgentGetLocalDatabaseHandle();
   if (hdb != nullptr)
   {
      TCHAR query[256];
      _sntprintf(query, 256, _T("DELETE FROM logwatch_files WHERE last_update_time<%d"),
         static_cast<int>(time(nullptr) - s_offsetRetentionTime));
      if (!DBQuery(hdb, query))
      {
         nxlog_debug_tag(DEBUG_TAG, 4, _T("Failed to delete old file positions"));
      }
   }

   AgentSetTimer(24 * 3600 * 1000, DeleteOldFilePositions);  // every day
}

/**
 * Restore parser file position from database
 */
static off_t RestoreParserFilePosition(LogParser *parser)
{
   DB_HANDLE hdb = AgentGetLocalDatabaseHandle();
   if (hdb == nullptr)
      return -1;

   // Prepare statement with bind parameters
   DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT size FROM logwatch_files WHERE name=? AND path=?"));
   if (hStmt == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Failed to prepare statement for restoring file position for parser %s (file: %s)"), parser->getName(), parser->getFileName());
      return -1;
   }

   const String &parserId = parser->getIdentifier();
   DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, parserId.cstr(), DB_BIND_STATIC);
   DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, parser->getFileName(), DB_BIND_STATIC);

   DB_RESULT hResult = DBSelectPrepared(hStmt);
   off_t position = -1;

   if (hResult != nullptr)
   {
      if (DBGetNumRows(hResult) > 0)
      {
         position = static_cast<off_t>(DBGetFieldInt64(hResult, 0, 0));
         nxlog_debug_tag(DEBUG_TAG, 6, _T("Restored file position ") INT64_FMT _T(" for parser %s (file: %s)"),
            position, parser->getName(), parser->getFileName());
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG, 6, _T("No saved position found for parser %s (file: %s)"), parser->getName(), parser->getFileName());
      }
      DBFreeResult(hResult);
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 4, _T("Database query failed while restoring position for parser %s (file: %s)"), parser->getName(), parser->getFileName());
   }

   DBFreeStatement(hStmt);
   return position;
}

/**
 * Called by master agent at unload
 */
static void SubagentShutdown()
{
   SaveFilePositions();

   for(int i = 0; i < s_parsers.size(); i++)
      s_parsers.get(i)->stop();

   for(int i = 0; i < s_templateParsers.size(); i++)
      s_templateParsers.get(i)->stop();

   CleanupLogParserLibrary();
}

/**
 * Callback for matched log records
 */
static void LogParserMatch(const LogParserCallbackData& data)
{
   StringMap parameters;
   data.captureGroups->addAllToMap(&parameters);
   if (data.eventTag != nullptr)
   {
      parameters.set(_T("eventTag"), data.eventTag);
   }

   if (data.source != nullptr)
   {
      parameters.set(_T("source"), data.source);
      parameters.set(_T("eventId"), data.windowsEventId);
      parameters.set(_T("severity"), data.severity);
      parameters.set(_T("recordId"), data.recordId);
   }
   parameters.set(_T("repeatCount"), data.repeatCount);

   if (data.variables != nullptr)
   {
      for(int j = 0; j < data.variables->size(); j++)
      {
         TCHAR buffer[32];
         _sntprintf(buffer, 32, _T("variable%d"), j + 1);
         parameters.set(buffer, data.variables->get(j));
      }
   }
   parameters.set(_T("fileName"), data.logName);

   AgentPostEvent(data.eventCode, data.eventName, data.logRecordTimestamp, parameters);
}

/**
 * Callback for action execution
 */
static void ExecuteAction(const TCHAR *action, const StringList& args, void *userData)
{
   AgentExecuteAction(action, args);
}

/**
 * Collect list of matching files from given base directory
 */
static StringList CollectMatchingFiles(const TCHAR *basePath, const TCHAR *fileTemplate, bool followSymlinks)
{
   StringList matchingFiles;

   TCHAR fname[MAX_PATH];
   ExpandFileName(fileTemplate, fname, MAX_PATH, true);

   _TDIR *dir = _topendir(basePath);
   if (dir != nullptr)
   {
      struct _tdirent *d;
      while((d = _treaddir(dir)) != nullptr)
      {
         if (!_tcscmp(d->d_name, _T(".")) || !_tcscmp(d->d_name, _T("..")))
            continue;

#if defined(__APPLE__) || defined(_WIN32) // Case insensitive
         if (MatchString(fname, d->d_name, false))
#else
         if (MatchString(fname, d->d_name, true))
#endif
         {
#if defined(_WIN32)
            if (d->d_type == DT_REG)
               matchingFiles.add(d->d_name);
#elif HAVE_DIRENT_D_TYPE
            if ((d->d_type == DT_REG) || ((d->d_type == DT_LNK) && followSymlinks))
               matchingFiles.add(d->d_name);
#else
            TCHAR path[MAX_PATH];
            _tcscpy(path, basePath);
            _tcslcat(path, d->d_name, MAX_PATH);
            NX_STAT_STRUCT st;
            if (CALL_STAT(path, &st) == 0)
            {
               if (S_ISREG(st.st_mode) || (followSymlinks && S_ISLNK(st.st_mode)))
                  matchingFiles.add(d->d_name);
            }
#endif
         }
      }
      _tclosedir(dir);
   }

   return matchingFiles;
}

/**
 * Update active parsers list from template
 */
static void UpdateParsersFromTemplate(LogParser *templateParser, StringObjectMap<LogParser> *activeParsers, const TCHAR *basePath, const TCHAR *fileTemplate, bool firstRun)
{
   StringList matchingFiles = CollectMatchingFiles(basePath, fileTemplate, templateParser->isFollowSymlinks());
   StringList monitoredFiles = activeParsers->keys();

   for(int i = 0; i < monitoredFiles.size();)
   {
      int index = matchingFiles.indexOf(monitoredFiles.get(i));
      if (index != -1)
      {
         SaveParserPosition(activeParsers->get(monitoredFiles.get(i)));
         matchingFiles.remove(index);
         monitoredFiles.remove(i);
      }
      else
      {
         i++;
      }
   }

   for(int i = 0; i < matchingFiles.size(); i++)
   {
      TCHAR path[MAX_PATH];
      _tcscpy(path, basePath);
      _tcslcat(path, matchingFiles.get(i), MAX_PATH);
      nxlog_debug_tag(DEBUG_TAG, 3, _T("New match for base path \"%s\" and template \"%s\": \"%s\""), basePath, fileTemplate, path);

      LogParser *p = new LogParser(templateParser);
      p->setFileName(path);
      p->setCallback(LogParserMatch);
      p->setDataPushCallback(AgentPushParameterData);
      p->setActionCallback(ExecuteAction);
      p->setThread(ThreadCreateEx(ParserThreadFile, p, firstRun ? RestoreParserFilePosition(p) : static_cast<off_t>(0)));
      activeParsers->set(matchingFiles.get(i), p);
   }

   for(int i = 0; i < monitoredFiles.size(); i++)
   {
      nxlog_debug_tag(DEBUG_TAG, 3, _T("File \"%s\" no longer matches template \"%s\" (base path \"%s\")"), monitoredFiles.get(i), fileTemplate, basePath);
      LogParser *p = activeParsers->unlink(monitoredFiles.get(i));
      p->stop();
      delete p;
   }
}

/**
 * File template watching thread
 */
static void TemplateParserThread(LogParser *parser)
{
   const TCHAR *fileTemplate = _tcsrchr(parser->getFileName(), FS_PATH_SEPARATOR_CHAR);
   if (fileTemplate == nullptr)
   {
      nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, _T("Cannot start template parser: cannot extract base path from file name template \"%s\""), parser->getFileName());
      return;
   }

   TCHAR basePath[MAX_PATH];
   memset(basePath, 0, sizeof basePath);
   memcpy(basePath, parser->getFileName(), ((fileTemplate - (parser->getFileName())) + 1) * sizeof(TCHAR));
   fileTemplate++;

   nxlog_debug_tag(DEBUG_TAG, 1, _T("Template parser started (base-path=\"%s\" template=\"%s\")"), basePath, fileTemplate);

   StringObjectMap<LogParser> activeParsers(Ownership::False);
   UpdateParsersFromTemplate(parser, &activeParsers, basePath, fileTemplate, true);
   while(!parser->getStopCondition()->wait(10000))
      UpdateParsersFromTemplate(parser, &activeParsers, basePath, fileTemplate, false);

   activeParsers.forEach(
      [] (const TCHAR *key, const LogParser *p) -> EnumerationCallbackResult
      {
         const_cast<LogParser*>(p)->stop();
         delete const_cast<LogParser*>(p);
         return _CONTINUE;
      });

   nxlog_debug_tag(DEBUG_TAG, 1, _T("Template parser stopped (base-path=\"%s\" template=\"%s\")"), basePath, fileTemplate);
}

/**
 * Add parser from config parameter
 */
static void AddParserFromConfig(const TCHAR *file, const uuid& guid)
{
	size_t size;
	BYTE *xml = LoadFile(file, &size);
	if (xml != nullptr)
	{
	   TCHAR error[1024];
		ObjectArray<LogParser> *parsers = LogParser::createFromXml((const char *)xml, size, error, 1024);
		if (parsers != nullptr)
		{
			for (int i = 0; i < parsers->size(); i++)
			{
				LogParser *parser = parsers->get(i);
				const TCHAR *parserFileName = parser->getFileName();
				if (parserFileName != nullptr && parserFileName[0] != 0)
				{
               if (_tcscspn(parserFileName + 1, _T("*?")) != _tcslen(parserFileName + 1))
               {
                  parser->setGuid(guid);
                  s_templateParsers.add(parser);
                  nxlog_debug_tag(DEBUG_TAG, 1, _T("Registered parser for file template \"%s\" (GUID = %s)"), parserFileName, guid.toString().cstr());
               }
               else
               {
                  parser->setCallback(LogParserMatch);
                  parser->setDataPushCallback(AgentPushParameterData);
                  parser->setActionCallback(ExecuteAction);
                  parser->setGuid(guid);
                  s_parsers.add(parser);
                  nxlog_debug_tag(DEBUG_TAG, 1, _T("Registered parser for file \"%s\" (GUID = %s)"), parserFileName, guid.toString().cstr());
#ifdef _WIN32
                  nxlog_debug_tag(DEBUG_TAG, 5, _T("Process RSS after parser creation is ") INT64_FMT _T(" bytes"), GetProcessRSS());
#endif
               }
				}
				else
				{
					delete parser;
					AgentWriteLog(EVENTLOG_ERROR_TYPE, _T("LogWatch: Parser configuration %s missing file name to parse (%d)"), file, i);
				}
			}
         delete parsers;
		}
		else
		{
			AgentWriteLog(EVENTLOG_ERROR_TYPE, _T("LogWatch: Cannot create parser from configuration file %s (%s)"), file, error);
		}
		MemFree(xml);
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
              SUBDIR_LOGPARSER_POLICY FS_PATH_SEPARATOR);

   nxlog_debug_tag(DEBUG_TAG, 1, _T("AddLogwatchPolicyFiles(): Log parser policy directory: %s"), policyFolder);

   _TDIR *dir = _topendir(policyFolder);
   if (dir != nullptr)
   {
      struct _tdirent *d;
      while((d = _treaddir(dir)) != nullptr)
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
               if (p != nullptr)
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
   uint32_t offset = config->getValueAsUInt(_T("/LogWatch/LogOffsetRetentionTime"), 10);
   s_offsetRetentionTime = offset * 24 * 3600;

	ConfigEntry *parsers = config->getEntry(_T("/LogWatch/Parser"));
	if (parsers != nullptr)
	{
		for(int i = 0; i < parsers->getValueCount(); i++)
			AddParserFromConfig(parsers->getValue(i), uuid::NULL_UUID);
	}
   AddLogwatchPolicyFiles();
	// Start parsing threads
   for (int i = 0; i < s_parsers.size(); i++)
	{
      LogParser *p = s_parsers.get(i);
      off_t startPos = RestoreParserFilePosition(p);
#ifdef _WIN32
		if (p->getFileName()[0] == _T('*'))	// event log
		{
			p->setThread(ThreadCreateEx(ParserThreadEventLog, p));
			// Seems that simultaneous calls to OpenEventLog() from two or more threads may
			// cause entire process to hang
			ThreadSleepMs(200);
		}
		else	// regular file
		{
			p->setThread(ThreadCreateEx(ParserThreadFile, p, startPos));
		}
#else
		p->setThread(ThreadCreateEx(ParserThreadFile, p, startPos));
#endif
	}

   for (int i = 0; i < s_templateParsers.size(); i++)
   {
      LogParser *p = s_templateParsers.get(i);
      p->setThread(ThreadCreateEx(TemplateParserThread, p));
   }

   AgentSetTimer(60000, SaveFilePositions);  // every minute
   DeleteOldFilePositions();

	return true;
}

/**
 * Agent notification handler
 */
static void OnAgentNotify(UINT32 code, void *data)
{
   if (code != AGENT_NOTIFY_POLICY_INSTALLED)
      return;

   // data points to PolicyChangeNotification object
   PolicyChangeNotification *n = static_cast<PolicyChangeNotification*>(data);
   if (_tcscmp(n->type, _T("LogParserConfig")))
      return;

   s_parserLock.lock();
   for (int i = 0; i < s_parsers.size(); i++)
   {
      LogParser *p = s_parsers.get(i);
      if (!p->getGuid().equals(n->guid))
         continue;

      if (n->sameContent)
      {
         s_parserLock.unlock();
         return;
      }

      nxlog_debug_tag(DEBUG_TAG, 3, _T("Reloading parser for file %s"), p->getFileName());
      SaveParserPosition(p);
      p->stop();
      s_parsers.remove(i);
      i--;
   }

   for (int i = 0; i < s_templateParsers.size(); i++)
   {
      LogParser *p = s_templateParsers.get(i);
      nxlog_debug_tag(DEBUG_TAG, 3, _T("Reloading parser for file %s"), p->getFileName());
      p->stop();
   }
   s_templateParsers.clear();

   const TCHAR *dataDir = AgentGetDataDirectory();
   TCHAR tail = dataDir[_tcslen(dataDir) - 1];

   TCHAR policyFile[MAX_PATH];
   _sntprintf(policyFile, MAX_PATH, _T("%s%s%s%s.xml"), dataDir,
         ((tail != '\\') && (tail != '/')) ? FS_PATH_SEPARATOR : _T(""),
         SUBDIR_LOGPARSER_POLICY FS_PATH_SEPARATOR, (const TCHAR *)n->guid.toString());
   AddParserFromConfig(policyFile, n->guid);

   // Start parsing threads
   for (int i = 0; i < s_parsers.size(); i++)
   {
      LogParser *p = s_parsers.get(i);
      if (!p->getGuid().equals(n->guid))
         continue;

      off_t startPos = RestoreParserFilePosition(p);
#ifdef _WIN32
      if (p->getFileName()[0] == _T('*'))	// event log
      {
         p->setThread(ThreadCreateEx(ParserThreadEventLog, p));
         // Seems that simultaneous calls to OpenEventLog() from two or more threads may
         // cause entire process to hang
         ThreadSleepMs(200);
      }
      else	// regular file
      {
         p->setThread(ThreadCreateEx(ParserThreadFile, p, startPos));
      }
#else
      p->setThread(ThreadCreateEx(ParserThreadFile, p, startPos));
#endif
   }

   for (int i = 0; i < s_templateParsers.size(); i++)
   {
      LogParser *p = s_templateParsers.get(i);
      p->setThread(ThreadCreateEx(TemplateParserThread, p));
   }

   s_parserLock.unlock();
}

/**
 * Supported parameters
 */
static NETXMS_SUBAGENT_PARAM s_parameters[] =
{
   { _T("LogWatch.MetricTimestamp(*)"), H_Metric, _T("T"), DCI_DT_STRING, _T("Metric {instance} timestamp") },
   { _T("LogWatch.MetricValue(*)"), H_Metric, _T("V"), DCI_DT_STRING, _T("Metric {instance} value") },
	{ _T("LogWatch.Parser.Status(*)"), H_ParserStats, _T("S"), DCI_DT_STRING, _T("Parser {instance} status") },
	{ _T("LogWatch.Parser.MatchedRecords(*)"), H_ParserStats, _T("M"), DCI_DT_INT, _T("Number of records matched by parser {instance}") },
   { _T("LogWatch.Parser.MetricTimestamp(*)"), H_ParserStats, _T("T"), DCI_DT_STRING, _T("Parser {instance} metric timestamp") },
   { _T("LogWatch.Parser.MetricValue(*)"), H_ParserStats, _T("V"), DCI_DT_STRING, _T("Parser {instance} metric value") },
	{ _T("LogWatch.Parser.ProcessedRecords(*)"), H_ParserStats, _T("P"), DCI_DT_INT, _T("Number of records processed by parser {instance}") }
};

/**
 * Supported lists
 */
static NETXMS_SUBAGENT_LIST s_lists[] =
{
   { _T("LogWatch.Metrics"), H_MetricList, nullptr },
	{ _T("LogWatch.Parsers"), H_ParserList, nullptr }
};

/**
 * Supported tables
 */
static NETXMS_SUBAGENT_TABLE s_tables[] =
{
   { _T("LogWatch.Metrics"), H_MetricTable, nullptr, _T("NAME"), _T("Parser metrics") }
};

/**
 * Subagent information
 */
static NETXMS_SUBAGENT_INFO s_info =
{
	NETXMS_SUBAGENT_INFO_MAGIC,
	_T("LOGWATCH"), NETXMS_VERSION_STRING,
	SubagentInit, SubagentShutdown, nullptr, OnAgentNotify, nullptr,
	sizeof(s_parameters) / sizeof(NETXMS_SUBAGENT_PARAM),
	s_parameters,
	sizeof(s_lists) / sizeof(NETXMS_SUBAGENT_LIST),
	s_lists,
   sizeof(s_tables) / sizeof(NETXMS_SUBAGENT_TABLE),
   s_tables,
	0, nullptr,		// actions
	0, nullptr		// push parameters
};

/**
 * Entry point for NetXMS agent
 */
DECLARE_SUBAGENT_ENTRY_POINT(LOGWATCH)
{
	*ppInfo = &s_info;
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
