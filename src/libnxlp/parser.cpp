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
** File: parser.cpp
**
**/

#include "libnxlp.h"
#include <expat.h>

/**
 * Context state texts
 */
static const TCHAR *s_states[] = { _T("MANUAL"), _T("AUTO"), _T("INACTIVE") };

/**
 * XML parser state codes
 */
enum ParserState
{
   XML_STATE_INIT,
   XML_STATE_END,
   XML_STATE_ERROR,
   XML_STATE_PARSER,
   XML_STATE_RULES,
   XML_STATE_RULE,
   XML_STATE_MATCH,
   XML_STATE_METRICS,
   XML_STATE_METRIC,
   XML_STATE_EVENT,
   XML_STATE_FILE,
   XML_STATE_ID,
   XML_STATE_LEVEL,
   XML_STATE_SOURCE,
   XML_STATE_CONTEXT,
   XML_STATE_MACROS,
   XML_STATE_MACRO,
   XML_STATE_DESCRIPTION,
   XML_STATE_EXCLUSION_SCHEDULES,
   XML_STATE_EXCLUSION_SCHEDULE,
   XML_STATE_AGENT_ACTION,
   XML_STATE_LOG_NAME,
   XML_STATE_PUSH
};

/**
 * XML parser state for creating LogParser object from XML
 */
struct LogParser_XmlParserState
{
	LogParser *parser;
	ParserState state;
	StringBuffer regexp;
	StringBuffer event;
	TCHAR *eventTag;
	StringBuffer file;
	StringList files;
	IntegerArray<int32_t> encodings;
   IntegerArray<int32_t> preallocFlags;
   IntegerArray<int32_t> detectBrokenPreallocFlags;
   IntegerArray<int32_t> snapshotFlags;
   IntegerArray<int32_t> keepOpenFlags;
   IntegerArray<int32_t> ignoreMTimeFlags;
   IntegerArray<int32_t> rescanFlags;
   IntegerArray<int32_t> followSymlinksFlags;
   IntegerArray<int32_t> removeEscapeSeqFlags;
   StringBuffer logName;
   StringBuffer id;
   StringBuffer level;
   StringBuffer source;
   StringBuffer pushParam;
   int pushGroup;
   StringBuffer context;
   StringBuffer description;
   StringBuffer ruleName;
   StringBuffer agentAction;
   StringBuffer agentActionArgs;
	int contextAction;
	StringBuffer ruleContext;
	StringBuffer errorText;
	StringBuffer macroName;
	StringBuffer macro;
	StringBuffer schedule;
   bool ignoreCase;
	bool invertedRule;
	bool breakFlag;
	bool doNotSaveToDBFlag;
	int repeatCount;
	int repeatInterval;
	bool resetRepeat;
	int metricCapturGroup;
	bool metricPushFlag;
	StringBuffer metricName;
	StructArray<LogParserMetric> metrics;

   LogParser_XmlParserState() : encodings(4, 4), preallocFlags(4, 4), detectBrokenPreallocFlags(4, 4), snapshotFlags(4, 4), keepOpenFlags(4, 4), ignoreMTimeFlags(4, 4), metrics(0, 8)
	{
      state = XML_STATE_INIT;
      parser = nullptr;
      ignoreCase = true;
	   invertedRule = false;
	   breakFlag = false;
	   doNotSaveToDBFlag = false;
	   contextAction = CONTEXT_SET_AUTOMATIC;
	   repeatCount = 0;
	   repeatInterval = 0;
	   resetRepeat = true;
	   eventTag = nullptr;
	   pushGroup = 0;
	   metricCapturGroup = 1;
	   metricPushFlag = false;
	}
};

/**
 * Parser default constructor
 */
LogParser::LogParser() : m_rules(0, 16, Ownership::True), m_stopCondition(true)
{
	m_cb = nullptr;
	m_cbAction = nullptr;
	m_cbDataPush = nullptr;
   m_cbCopy = nullptr;
	m_userData = nullptr;
	m_name = nullptr;
	m_fileName = nullptr;
	m_fileEncoding = LP_FCP_ACP;
   m_fileCheckInterval = 10000;
	m_preallocatedFile = false;
	m_detectBrokenPrealloc = false;
	m_eventNameList = nullptr;
	m_eventResolver = nullptr;
	m_thread = INVALID_THREAD_HANDLE;
	m_recordsProcessed = 0;
	m_recordsMatched = 0;
	m_processAllRules = false;
   m_suspended = false;
   m_keepFileOpen = true;
   m_ignoreMTime = false;
   m_followSymlinks = false;
   m_removeEscapeSequences = false;
   m_rescan = false;
	m_status = LPS_INIT;
#ifdef _WIN32
   m_marker = nullptr;
#endif
   m_readBuffer = nullptr;
   m_readBufferSize = 0;
   m_textBuffer = nullptr;
   m_fileNameHash[0] = 0;
   m_offset = -1;
}

/**
 * Parser copy constructor
 */
LogParser::LogParser(const LogParser *src) : m_rules(src->m_rules.size(), 16, Ownership::True), m_stopCondition(true)
{
   int count = src->m_rules.size();
	for(int i = 0; i < count; i++)
		m_rules.add(new LogParserRule(src->m_rules.get(i), this));

	m_macros.addAll(&src->m_macros);
	m_contexts.addAll(&src->m_contexts);
	m_exclusionSchedules.addAll(&src->m_exclusionSchedules);

	m_cb = src->m_cb;
   m_cbAction = src->m_cbAction;
   m_cbDataPush = src->m_cbDataPush;
   m_cbCopy = src->m_cbCopy;
   m_userData = src->m_userData;
	m_name = MemCopyString(src->m_name);
	m_fileName = MemCopyString(src->m_fileName);
	m_fileEncoding = src->m_fileEncoding;
   m_fileCheckInterval = src->m_fileCheckInterval;
   m_preallocatedFile = src->m_preallocatedFile;
	m_detectBrokenPrealloc = src->m_detectBrokenPrealloc;

	if (src->m_eventNameList != nullptr)
	{
		int count;
		for(count = 0; src->m_eventNameList[count].text != nullptr; count++);
		m_eventNameList = (count > 0) ? MemCopyBlock(src->m_eventNameList, sizeof(CodeLookupElement) * (count + 1)) : nullptr;
	}
	else
	{
		m_eventNameList = nullptr;
	}

	m_eventResolver = src->m_eventResolver;
	m_thread = INVALID_THREAD_HANDLE;
   m_recordsProcessed = 0;
	m_recordsMatched = 0;
	m_processAllRules = src->m_processAllRules;
   m_suspended = src->m_suspended;
   m_keepFileOpen = src->m_keepFileOpen;
   m_ignoreMTime = src->m_ignoreMTime;
   m_followSymlinks = src->m_followSymlinks;
   m_removeEscapeSequences = src->m_removeEscapeSequences;
   m_rescan = src->m_rescan;
	m_status = LPS_INIT;
#ifdef _WIN32
   m_marker = MemCopyString(src->m_marker);
#endif
   m_readBuffer = nullptr;
   m_readBufferSize = 0;
   m_textBuffer = nullptr;
   _tcscpy(m_fileNameHash, src->m_fileNameHash);
   m_offset = -1;
}

/**
 * Destructor
 */
LogParser::~LogParser()
{
	MemFree(m_name);
	MemFree(m_fileName);
#ifdef _WIN32
   MemFree(m_marker);
#endif
   MemFree(m_readBuffer);
   MemFree(m_textBuffer);
}

/**
 * Trace
 */
void LogParser::trace(int level, const TCHAR *format, ...)
{
   va_list args;
   va_start(args, format);
	nxlog_debug_tag2(DEBUG_TAG _T(".parser"), level, format, args);
	va_end(args);
}

/**
 * Add rule
 */
bool LogParser::addRule(LogParserRule *rule)
{
	bool valid = rule->isValid();
	if (valid)
	{
	   m_rules.add(rule);
	}
	else
	{
		delete rule;
	}
	return valid;
}

/**
 * Check context
 */
const TCHAR *LogParser::checkContext(LogParserRule *rule)
{
	const TCHAR *state;

	if (rule->getContext() == nullptr)
	{
		trace(7, _T("  rule has no context"));
		return s_states[CONTEXT_SET_MANUAL];
	}

	state = m_contexts.get(rule->getContext());
	if (state == nullptr)
	{
		trace(7, _T("  context '%s' inactive, rule should be skipped"), rule->getContext());
		return nullptr;	// Context inactive, don't use this rule
	}

	if (!_tcscmp(state, s_states[CONTEXT_CLEAR]))
	{
		trace(7, _T("  context '%s' inactive, rule should be skipped"), rule->getContext());
		return nullptr;
	}
	else
	{
		trace(7, _T("  context '%s' active (mode=%s)"), rule->getContext(), state);
		return state;
	}
}

/**
 * Match log record
 */
bool LogParser::matchLogRecord(bool hasAttributes, const TCHAR *source, uint32_t eventId,
      uint32_t level, const TCHAR *line, StringList *variables, uint64_t recordId,
      uint32_t objectId, time_t timestamp, const TCHAR *logName, bool *saveToDatabase)
{
	const TCHAR *state;
	bool matched = false;

	if (hasAttributes)
		trace(6, _T("Match event: source=\"%s\" id=%u level=%d text=\"%s\" recordId=") UINT64_FMT, source, eventId, level, line, recordId);
	else
		trace(6, _T("Match line: \"%s\""), line);

	m_recordsProcessed++;
	int i;
	for(i = 0; i < m_rules.size(); i++)
	{
	   LogParserRule *rule = m_rules.get(i);
		trace(7, _T("checking rule %d \"%s\""), i + 1, rule->getDescription());
		if ((state = checkContext(rule)) != nullptr)
		{
			bool ruleMatched = hasAttributes ?
			   rule->matchEx(source, eventId, level, line, variables, recordId, objectId, timestamp, logName, m_cb, m_cbDataPush, m_cbAction, m_userData) :
				rule->match(line, objectId, m_cb, m_cbDataPush, m_cbAction, logName, m_userData);
			if (ruleMatched)
			{
				trace(5, _T("rule %d \"%s\" matched"), i + 1, rule->getDescription());
				if (!matched)
					m_recordsMatched++;

				// Update context
				if (rule->getContextToChange() != nullptr)
				{
					m_contexts.set(rule->getContextToChange(), s_states[rule->getContextAction()]);
					trace(5, _T("rule %d \"%s\": context %s set to %s"), i + 1,
					      rule->getDescription(), rule->getContextToChange(), s_states[rule->getContextAction()]);
				}

				// Set context of this rule to inactive if rule context mode is "automatic reset"
				if (!_tcscmp(state, s_states[CONTEXT_SET_AUTOMATIC]))
				{
					m_contexts.set(rule->getContext(), s_states[CONTEXT_CLEAR]);
					trace(5, _T("rule %d \"%s\": context %s cleared because it was set to automatic reset mode"),
							i + 1, rule->getDescription(), rule->getContext());
				}
				matched = true;
				if (saveToDatabase != nullptr && rule->isDoNotSaveToDBFlag())
				{
				   trace(5, _T("rule %d \"%s\" set flag not to save data to database"), i + 1, rule->getDescription());
				   *saveToDatabase = false;
				}
				if (!m_processAllRules || rule->getBreakFlag())
					break;
			}
		}
	}
	if (i < m_rules.size())
		trace(6, _T("processing stopped at rule %d \"%s\"; result = %s"), i + 1,
				m_rules.get(i)->getDescription(), matched ? _T("true") : _T("false"));
	else
		trace(6, _T("Processing stopped at end of rules list; result = %s"), matched ? _T("true") : _T("false"));

   if (m_cbCopy != nullptr)
   {
      if (hasAttributes)
         m_cbCopy(line, source, eventId, level, m_userData);
      else
         m_cbCopy(line, nullptr, 0, 0, m_userData);
   }

	return matched;
}

/**
 * Match simple log line
 */
bool LogParser::matchLine(const TCHAR *line, const TCHAR *logName, uint32_t objectId)
{
   if (!m_removeEscapeSequences)
      return matchLogRecord(false, nullptr, 0, 0, line, nullptr, 0, objectId, 0, logName, nullptr);

   StringBuffer sb;
   for(const TCHAR *p = line; *p != 0; p++)
   {
      TCHAR ch = *p;
      if (ch == 27)
      {
         ch = *(++p);
         if (ch == '[')
         {
            while(*p != 0)
            {
               ch = *(++p);
               if (((ch >= 'A') && (ch <= 'Z')) || ((ch >= 'a') && (ch <= 'z')))
                  break;
            }
         }
         else if ((ch == '(') || (ch == ')'))
         {
            p++;
         }
      }
      else if ((ch >= 32) || (ch == '\r') || (ch == '\n') || (ch == '\t'))
      {
         sb.append(ch);
      }
   }
   return matchLogRecord(false, nullptr, 0, 0, sb, nullptr, 0, objectId, 0, logName, nullptr);
}

/**
 * Match log event (text with additional attributes - source, severity, event id)
 */
bool LogParser::matchEvent(const TCHAR *source, uint32_t eventId, uint32_t level, const TCHAR *line, StringList *variables,
         uint64_t recordId, uint32_t objectId, time_t timestamp, const TCHAR *logName, bool *saveToDatabase)
{
	return matchLogRecord(true, source, eventId, level, line, variables, recordId, objectId, timestamp, logName, saveToDatabase);
}

/**
 * Set associated file name
 */
void LogParser::setFileName(const TCHAR *name)
{
	MemFree(m_fileName);
	m_fileName = MemCopyString(name);
	if (m_name == nullptr)
		m_name = MemCopyString(name);	// Set parser name to file name

   BYTE hash[MD5_DIGEST_SIZE];
   CalculateMD5Hash((BYTE *)name, _tcslen(name) * sizeof(TCHAR), hash);
   BinToStr(hash, MD5_DIGEST_SIZE, m_fileNameHash);
}

/**
 * Set parser name
 */
void LogParser::setName(const TCHAR *name)
{
	MemFree(m_name);
	m_name = MemCopyString((name != nullptr) ? name : CHECK_NULL(m_fileName));
}

/**
 * Set parser GUID
 */
void LogParser::setGuid(const uuid& guid)
{
   m_guid = guid;
}

/**
 * Add macro
 */
void LogParser::addMacro(const TCHAR *name, const TCHAR *value)
{
	m_macros.set(name, value);
}

/**
 * Get macro
 */
const TCHAR *LogParser::getMacro(const TCHAR *name)
{
	const TCHAR *value;

	value = m_macros.get(name);
	return CHECK_NULL_EX(value);
}

/**
 * Create parser configuration from XML - callback for element start
 */
static void StartElement(void *userData, const char *name, const char **attrs)
{
	LogParser_XmlParserState *ps = static_cast<LogParser_XmlParserState*>(userData);

	if (!strcmp(name, "parser"))
	{
		ps->state = XML_STATE_PARSER;
		ps->parser->setProcessAllFlag(XMLGetAttrBoolean(attrs, "processAll", false));
      ps->parser->setFileCheckInterval(XMLGetAttrUInt32(attrs, "checkInterval", 10000));
		const char *name = XMLGetAttr(attrs, "name");
		if (name != nullptr)
		{
#ifdef UNICODE
			WCHAR *wname = WideStringFromUTF8String(name);
			ps->parser->setName(wname);
			MemFree(wname);
#else
			ps->parser->setName(name);
#endif
		}
	}
	else if (!strcmp(name, "file"))
	{
		ps->state = XML_STATE_FILE;
		const char *encoding = XMLGetAttr(attrs, "encoding");
		if (encoding != nullptr)
		{
			if ((*encoding == 0) || !stricmp(encoding, "auto"))
			{
				ps->encodings.add(LP_FCP_AUTO);
			}
			else if (!stricmp(encoding, "acp"))
			{
				ps->encodings.add(LP_FCP_ACP);
			}
			else if (!stricmp(encoding, "utf8") || !stricmp(encoding, "utf-8"))
			{
				ps->encodings.add(LP_FCP_UTF8);
			}
			else if (!stricmp(encoding, "ucs2") || !stricmp(encoding, "ucs-2") || !stricmp(encoding, "utf-16"))
			{
				ps->encodings.add(LP_FCP_UCS2);
			}
			else if (!stricmp(encoding, "ucs2le") || !stricmp(encoding, "ucs-2le") || !stricmp(encoding, "utf-16le"))
			{
				ps->encodings.add(LP_FCP_UCS2_LE);
			}
			else if (!stricmp(encoding, "ucs2be") || !stricmp(encoding, "ucs-2be") || !stricmp(encoding, "utf-16be"))
			{
				ps->encodings.add(LP_FCP_UCS2_BE);
			}
			else if (!stricmp(encoding, "ucs4") || !stricmp(encoding, "ucs-4") || !stricmp(encoding, "utf-32"))
			{
				ps->encodings.add(LP_FCP_UCS4);
			}
			else if (!stricmp(encoding, "ucs4le") || !stricmp(encoding, "ucs-4le") || !stricmp(encoding, "utf-32le"))
			{
				ps->encodings.add(LP_FCP_UCS4_LE);
			}
			else if (!stricmp(encoding, "ucs4be") || !stricmp(encoding, "ucs-4be") || !stricmp(encoding, "utf-32be"))
			{
				ps->encodings.add(LP_FCP_UCS4_BE);
			}
			else
			{
				ps->errorText = _T("Invalid file encoding");
				ps->state = XML_STATE_ERROR;
			}
		}
		else
		{
			ps->encodings.add(LP_FCP_AUTO);
		}
		ps->preallocFlags.add(XMLGetAttrBoolean(attrs, "preallocated", false) ? 1 : 0);
		ps->detectBrokenPreallocFlags.add(XMLGetAttrBoolean(attrs, "detectBrokenPrealloc", false) ? 1 : 0);
		ps->snapshotFlags.add(XMLGetAttrBoolean(attrs, "snapshot", false) ? 1 : 0);
		ps->keepOpenFlags.add(XMLGetAttrBoolean(attrs, "keepOpen", true) ? 1 : 0);
		ps->ignoreMTimeFlags.add(XMLGetAttrBoolean(attrs, "ignoreModificationTime", false) ? 1 : 0);
		ps->rescanFlags.add(XMLGetAttrBoolean(attrs, "rescan", false) ? 1 : 0);
		ps->followSymlinksFlags.add(XMLGetAttrBoolean(attrs, "followSymlinks", false) ? 1 : 0);
      ps->removeEscapeSeqFlags.add(XMLGetAttrBoolean(attrs, "removeEscapeSequences", false) ? 1 : 0);
   }
	else if (!strcmp(name, "macros"))
	{
		ps->state = XML_STATE_MACROS;
	}
	else if (!strcmp(name, "macro"))
	{
		const char *name;

		ps->state = XML_STATE_MACRO;
		name = XMLGetAttr(attrs, "name");
#ifdef UNICODE
		ps->macroName.clear();
		ps->macroName.appendUtf8String(name);
#else
		ps->macroName = CHECK_NULL_A(name);
#endif
		ps->macro = nullptr;
	}
	else if (!strcmp(name, "rules"))
	{
		ps->state = XML_STATE_RULES;
	}
	else if (!strcmp(name, "rule"))
	{
		ps->regexp.clear();
      ps->ignoreCase = true;
		ps->invertedRule = false;
		ps->event.clear();
		ps->context.clear();
		ps->contextAction = CONTEXT_SET_AUTOMATIC;
		ps->description.clear();
		ps->id.clear();
		ps->source.clear();
		ps->level.clear();
		ps->agentAction.clear();
		ps->metrics.clear();
      ps->logName.clear();
#ifdef UNICODE
		ps->ruleContext.clear();
		const char *context = XMLGetAttr(attrs, "context");
		if (context != nullptr)
			ps->ruleContext.appendUtf8String(context);
      ps->ruleName.clear();
      const char *name = XMLGetAttr(attrs, "name");
      if (name != nullptr)
         ps->ruleName.appendUtf8String(name);
#else
		ps->ruleContext = XMLGetAttr(attrs, "context");
      ps->ruleName = XMLGetAttr(attrs, "name");
#endif
		ps->breakFlag = XMLGetAttrBoolean(attrs, "break", false);
      ps->doNotSaveToDBFlag = XMLGetAttrBoolean(attrs, "doNotSaveToDatabase", false);
		ps->state = XML_STATE_RULE;
	}
	else if (!strcmp(name, "agentAction"))
	{
	   ps->state = XML_STATE_AGENT_ACTION;
	   const char *action = XMLGetAttr(attrs, "action");
	   if (action != nullptr)
	   {
#ifdef UNICODE
	      ps->agentAction.appendUtf8String(action);
#else
	      ps->agentAction = action;
#endif
	   }
	}
	else if (!strcmp(name, "match"))
	{
		ps->state = XML_STATE_MATCH;
      ps->ignoreCase = XMLGetAttrBoolean(attrs, "ignoreCase", true);
      ps->invertedRule = XMLGetAttrBoolean(attrs, "invert", false);
		ps->resetRepeat = XMLGetAttrBoolean(attrs, "reset", true);
		ps->repeatCount = XMLGetAttrInt(attrs, "repeatCount", 0);
		ps->repeatInterval = XMLGetAttrInt(attrs, "repeatInterval", 0);
	}
   else if (!strcmp(name, "metrics"))
   {
      ps->state = XML_STATE_METRICS;
   }
   else if (!strcmp(name, "metric"))
   {
      ps->state = XML_STATE_METRIC;
      ps->metricCapturGroup = XMLGetAttrInt(attrs, "group", 1);
      ps->metricPushFlag = XMLGetAttrBoolean(attrs, "push", false);
      ps->metricName.clear();
   }
	else if (!strcmp(name, "id") || !strcmp(name, "facility"))
	{
		ps->state = XML_STATE_ID;
	}
	else if (!strcmp(name, "level") || !strcmp(name, "severity"))
	{
		ps->state = XML_STATE_LEVEL;
	}
	else if (!strcmp(name, "source") || !strcmp(name, "tag"))
	{
		ps->state = XML_STATE_SOURCE;
	}
	else if (!strcmp(name, "push"))
	{
		ps->state = XML_STATE_PUSH;
		ps->pushGroup = XMLGetAttrInt(attrs, "group", 1);
      ps->pushParam.clear();
	}
	else if (!strcmp(name, "event"))
	{
		ps->state = XML_STATE_EVENT;

      const char *tag = XMLGetAttr(attrs, "tag");
      if (tag != nullptr)
      {
#ifdef UNICODE
         ps->eventTag = WideStringFromMBString(tag);
#else
         ps->eventTag = MemCopyStringA(tag);
#endif
      }
	}
	else if (!strcmp(name, "context"))
	{
		ps->state = XML_STATE_CONTEXT;

		const char *action = XMLGetAttr(attrs, "action");
		if (action == nullptr)
			action = "set";

		if (!strcmp(action, "set"))
		{
			const char *mode;

			mode = XMLGetAttr(attrs, "reset");
			if (mode == nullptr)
				mode = "auto";

			if (!strcmp(mode, "auto"))
			{
				ps->contextAction = CONTEXT_SET_AUTOMATIC;
			}
			else if (!strcmp(mode, "manual"))
			{
				ps->contextAction = CONTEXT_SET_MANUAL;
			}
			else
			{
				ps->errorText = _T("Invalid context reset mode");
				ps->state = XML_STATE_ERROR;
			}
		}
		else if (!strcmp(action, "clear"))
		{
			ps->contextAction = CONTEXT_CLEAR;
		}
		else
		{
			ps->errorText = _T("Invalid context action");
			ps->state = XML_STATE_ERROR;
		}
	}
   else if (!strcmp(name, "logName"))
   {
      ps->state = XML_STATE_LOG_NAME;
   }
	else if (!strcmp(name, "description"))
	{
		ps->state = XML_STATE_DESCRIPTION;
	}
   else if (!strcmp(name, "exclusionSchedules"))
   {
      ps->state = XML_STATE_EXCLUSION_SCHEDULES;
   }
   else if (!strcmp(name, "schedule"))
   {
      ps->state = XML_STATE_EXCLUSION_SCHEDULE;
   }
	else
	{
		ps->state = XML_STATE_ERROR;
	}
}

/**
 * Create parser configuration from XML - callback for element end
 */
static void EndElement(void *userData, const char *name)
{
	LogParser_XmlParserState *ps = (LogParser_XmlParserState *)userData;

	if (ps->state == XML_STATE_ERROR)
      return;

	if (!strcmp(name, "parser"))
	{
		ps->state = XML_STATE_END;
	}
	else if (!strcmp(name, "file"))
	{
		ps->files.add(ps->file);
		ps->file.clear();
		ps->state = XML_STATE_PARSER;
	}
	else if (!strcmp(name, "macros"))
	{
		ps->state = XML_STATE_PARSER;
	}
	else if (!strcmp(name, "macro"))
	{
		ps->parser->addMacro(ps->macroName, ps->macro);
		ps->macroName.clear();
		ps->macro.clear();
		ps->state = XML_STATE_MACROS;
	}
	else if (!strcmp(name, "rules"))
	{
		ps->state = XML_STATE_PARSER;
	}
	else if (!strcmp(name, "rule"))
	{
      ps->event.trim();

		const TCHAR *eventName = nullptr;
		TCHAR *eptr;
		uint32_t eventCode = _tcstoul(ps->event, &eptr, 0);
		if (*eptr != 0)
		{
			eventCode = ps->parser->resolveEventName(ps->event, 0);
			if (eventCode == 0)
			{
				eventName = (const TCHAR *)ps->event;
			}
		}

		if (ps->regexp.isEmpty())
			ps->regexp = _T(".*");
		LogParserRule *rule = new LogParserRule(ps->parser, ps->ruleName, ps->regexp, ps->ignoreCase,
		         eventCode, eventName, ps->eventTag, ps->repeatInterval, ps->repeatCount, ps->resetRepeat,
		         ps->metrics);
		if (!ps->agentAction.isEmpty())
		   rule->setAgentAction(ps->agentAction);
		if (!ps->agentActionArgs.isEmpty())
		   rule->setAgentActionArgs(new StringList(ps->agentActionArgs, _T(" ")));
      if (!ps->logName.isEmpty())
         rule->setLogName(ps->logName);
		if (!ps->ruleContext.isEmpty())
			rule->setContext(ps->ruleContext);
		if (!ps->context.isEmpty())
		{
			rule->setContextToChange(ps->context);
			rule->setContextAction(ps->contextAction);
		}

		if (!ps->description.isEmpty())
			rule->setDescription(ps->description);

		if (!ps->source.isEmpty())
			rule->setSource(ps->source);

		if (!ps->level.isEmpty())
			rule->setLevel(_tcstoul(ps->level, nullptr, 0));

		if (!ps->id.isEmpty())
		{
			TCHAR *eptr;
			uint32_t start = _tcstoul(ps->id, &eptr, 0);
         uint32_t end;
			if (*eptr == 0)
			{
				end = start;
			}
			else
			{
				while((*eptr == ' '))
					++eptr;
				if (*eptr == '-')
				{
				   ++eptr;
	            while((*eptr == ' '))
	               ++eptr;
				   end = _tcstoul(eptr, nullptr, 0);
				}
				else
				{
               nxlog_debug_tag(DEBUG_TAG _T(".parser"), 4, _T("Invalid event ID range definition \"%s\""), ps->id.cstr());
				   end = start;
				}
			}
			rule->setIdRange(start, end);
		}

		rule->setInverted(ps->invertedRule);
		rule->setBreakFlag(ps->breakFlag);
      rule->setDoNotSaveToDBFlag(ps->doNotSaveToDBFlag);

		MemFreeAndNull(ps->eventTag);

		ps->parser->addRule(rule);
		ps->state = XML_STATE_RULES;
	}
	else if (!strcmp(name, "agentAction"))
	{
	   ps->state = XML_STATE_RULE;
	}
	else if (!strcmp(name, "match"))
	{
		ps->state = XML_STATE_RULE;
	}
   else if (!strcmp(name, "metrics"))
   {
      ps->state = XML_STATE_RULE;
   }
   else if (!strcmp(name, "metric"))
   {
      LogParserMetric *m = ps->metrics.addPlaceholder();
      memset(m, 0, sizeof(LogParserMetric));
      _tcslcpy(m->name, ps->metricName, MAX_PARAM_NAME);
      m->captureGroup = ps->metricCapturGroup;
      m->push = ps->metricPushFlag;
      ps->state = XML_STATE_METRICS;
   }
	else if (!strcmp(name, "id") || !strcmp(name, "facility"))
	{
		ps->state = XML_STATE_RULE;
	}
	else if (!strcmp(name, "level") || !strcmp(name, "severity"))
	{
		ps->state = XML_STATE_RULE;
	}
	else if (!strcmp(name, "source") || !strcmp(name, "tag"))
	{
		ps->state = XML_STATE_RULE;
	}
	else if (!strcmp(name, "event"))
	{
		ps->state = XML_STATE_RULE;
	}
	else if (!strcmp(name, "push"))
	{
	   LogParserMetric *m = ps->metrics.addPlaceholder();
	   memset(m, 0, sizeof(LogParserMetric));
	   _tcslcpy(m->name, ps->pushParam, MAX_PARAM_NAME);
	   m->captureGroup = ps->pushGroup;
	   m->push = true;
		ps->state = XML_STATE_RULE;
	}
	else if (!strcmp(name, "context"))
	{
		ps->state = XML_STATE_RULE;
	}
	else if (!strcmp(name, "description"))
	{
		ps->state = XML_STATE_RULE;
	}
   else if (!strcmp(name, "logName"))
   {
      ps->state = XML_STATE_RULE;
   }
   else if (!strcmp(name, "exclusionSchedules"))
   {
      ps->state = XML_STATE_PARSER;
   }
   else if (!strcmp(name, "schedule"))
   {
      ps->parser->addExclusionSchedule(ps->schedule);
      ps->schedule.clear();
      ps->state = XML_STATE_EXCLUSION_SCHEDULES;
   }
}

/**
 * Create parser configuration from XML - callback for element data
 */
static void CharData(void *userData, const XML_Char *s, int len)
{
	LogParser_XmlParserState *ps = (LogParser_XmlParserState *)userData;

	switch(ps->state)
	{
	   case XML_STATE_AGENT_ACTION:
	      ps->agentActionArgs.appendUtf8String(s, len);
	      break;
		case XML_STATE_MATCH:
			ps->regexp.appendUtf8String(s, len);
			break;
      case XML_STATE_METRIC:
         ps->metricName.appendUtf8String(s, len);
         break;
		case XML_STATE_ID:
			ps->id.appendUtf8String(s, len);
			break;
		case XML_STATE_LEVEL:
			ps->level.appendUtf8String(s, len);
			break;
		case XML_STATE_SOURCE:
			ps->source.appendUtf8String(s, len);
			break;
		case XML_STATE_PUSH:
			ps->pushParam.appendUtf8String(s, len);
			break;
		case XML_STATE_EVENT:
			ps->event.appendUtf8String(s, len);
			break;
		case XML_STATE_FILE:
			ps->file.appendUtf8String(s, len);
			break;
		case XML_STATE_CONTEXT:
			ps->context.appendUtf8String(s, len);
			break;
		case XML_STATE_DESCRIPTION:
			ps->description.appendUtf8String(s, len);
			break;
      case XML_STATE_LOG_NAME:
         ps->logName.appendUtf8String(s, len);
         break;
		case XML_STATE_MACRO:
			ps->macro.appendUtf8String(s, len);
			break;
		case XML_STATE_EXCLUSION_SCHEDULE:
         ps->schedule.appendUtf8String(s, len);
         break;
		default:
			break;
	}
}

/**
 * Create parser configuration from XML. Returns array of identical parsers,
 * one for each <file> tag in source XML. Resulting parsers only differs with file names.
 */
ObjectArray<LogParser> *LogParser::createFromXml(const char *xml, ssize_t xmlLen, TCHAR *errorText,
      size_t errBufSize, bool (*eventResolver)(const TCHAR *, uint32_t *))
{
	ObjectArray<LogParser> *parsers = nullptr;

	XML_Parser parser = XML_ParserCreate(nullptr);
	LogParser_XmlParserState state;
	state.parser = new LogParser;
	state.parser->setEventNameResolver(eventResolver);
	XML_SetUserData(parser, &state);
	XML_SetElementHandler(parser, StartElement, EndElement);
	XML_SetCharacterDataHandler(parser, CharData);
	bool success = (XML_Parse(parser, xml, static_cast<int>((xmlLen == -1) ? strlen(xml) : xmlLen), TRUE) != XML_STATUS_ERROR);
	if (!success && (errorText != nullptr))
	{
		_sntprintf(errorText, errBufSize, _T("%hs at line %d"),
			   XML_ErrorString(XML_GetErrorCode(parser)), static_cast<int>(XML_GetCurrentLineNumber(parser)));
	}
	XML_ParserFree(parser);
	if (success && (state.state == XML_STATE_ERROR))
	{
		if (errorText != nullptr)
			_tcslcpy(errorText, state.errorText, errBufSize);
      delete state.parser;
	}
	else if (success)
	{
		parsers = new ObjectArray<LogParser>;
		if (state.files.size() > 0)
		{
			for(int i = 0; i < state.files.size(); i++)
			{
				LogParser *p = (i > 0) ? new LogParser(state.parser) : state.parser;
				p->setFileName(state.files.get(i));
				p->m_fileEncoding = state.encodings.get(i);
				p->m_preallocatedFile = (state.preallocFlags.get(i) != 0);
				p->m_detectBrokenPrealloc = (state.detectBrokenPreallocFlags.get(i) != 0);
				p->m_keepFileOpen = (state.keepOpenFlags.get(i) != 0);
				p->m_ignoreMTime = (state.ignoreMTimeFlags.get(i) != 0);
				p->m_followSymlinks = (state.followSymlinksFlags.get(i) != 0);
            p->m_removeEscapeSequences = (state.removeEscapeSeqFlags.get(i) != 0);
            p->m_rescan = (state.rescanFlags.get(i) != 0);
#ifdef _WIN32
            p->m_useSnapshot = (state.snapshotFlags.get(i) != 0);
#endif
				parsers->add(p);
			}
		}
		else
		{
			// It is possible to have parser without <file> tag, syslog parser for example
			parsers->add(state.parser);
		}
	}
	else
	{
      delete state.parser;
	}

	return parsers;
}

/**
 * Resolve event name
 */
uint32_t LogParser::resolveEventName(const TCHAR *name, uint32_t defaultValue)
{
	if (m_eventNameList != nullptr)
	{
		for(int i = 0; m_eventNameList[i].text != NULL; i++)
			if (!_tcsicmp(name, m_eventNameList[i].text))
				return m_eventNameList[i].code;
	}

	if (m_eventResolver != nullptr)
	{
		uint32_t val;
		if (m_eventResolver(name, &val))
			return val;
	}

	return defaultValue;
}

/**
 * Find rule by name
 */
const LogParserRule *LogParser::findRuleByName(const TCHAR *name) const
{
   for(int i = 0; i < m_rules.size(); i++)
   {
      LogParserRule *rule = m_rules.get(i);
      if (!_tcsicmp(rule->getName(), name))
         return rule;
   }
   return nullptr;
}

/**
 * Restore counters from previous parser copy
 */
void LogParser::restoreCounters(const LogParser *parser)
{
   for(int i = 0; i < m_rules.size(); i++)
   {
      const LogParserRule *rule = parser->findRuleByName(m_rules.get(i)->getName());
      if (rule != nullptr)
      {
         m_rules.get(i)->restoreCounters(*rule);
      }
   }
}

/**
 * Get character size in bytes for parser's encoding
 */
int LogParser::getCharSize() const
{
   switch(m_fileEncoding)
   {
      case LP_FCP_UCS4_BE:
      case LP_FCP_UCS4_LE:
      case LP_FCP_UCS4:
         return 4;
      case LP_FCP_UCS2_BE:
      case LP_FCP_UCS2_LE:
      case LP_FCP_UCS2:
         return 2;
      default:
         return 1;
   }
}

/**
 * Check for exclusion period
 */
bool LogParser::isExclusionPeriod()
{
   if (m_suspended)
      return true;

   if (m_exclusionSchedules.isEmpty())
      return false;

   time_t now = time(nullptr);
   struct tm localTime;
#if HAVE_LOCALTIME_R
   localtime_r(&now, &localTime);
#else
   memcpy(&localTime, localtime(&now), sizeof(struct tm));
#endif
   for(int i = 0; i < m_exclusionSchedules.size(); i++)
   {
      if (MatchSchedule(m_exclusionSchedules.get(i), NULL, &localTime, now))
         return true;
   }
   return false;
}

/**
 * Stop parser
 */
void LogParser::stop()
{
   m_stopCondition.set();
   ThreadJoin(m_thread);
   m_thread = INVALID_THREAD_HANDLE;
}

/**
 * Suspend parser
 */
void LogParser::suspend()
{
   m_suspended = true;
}

/**
 * Resume parser
 */
void LogParser::resume()
{
   m_suspended = false;
}

/**
 * Get status text
 */
const TCHAR *LogParser::getStatusText() const
{
   static const TCHAR *texts[] = {
      _T("INIT"),
      _T("RUNNING"),
      _T("FILE MISSING"),
      _T("FILE OPEN ERROR"),
      _T("SUSPENDED"),
      _T("EVENT LOG SUBSCRIBE FAILED"),
      _T("EVENT LOG READ ERROR"),
      _T("EVENT LOG OPEN ERROR"),
      _T("VSS FAILURE")
   };
   return texts[m_status];
}

/**
 * Get all events used in this log parser
 */
void LogParser::getEventList(HashSet<uint32_t> *eventList) const
{
   for(int i = 0; i < m_rules.size(); i++)
   {
      eventList->put(m_rules.get(i)->getEventCode());
   }
}

/**
 * Searches this log parser for the specified event.
 * @return True if the event was found, false otherwise.
 */
bool LogParser::isUsingEvent(uint32_t eventCode) const
{
   for (int i = 0; i < m_rules.size(); i++)
      if (m_rules.get(i)->getEventCode() == eventCode)
         return true;
   return false;
}

/**
 * Get list of all provided metrics
 */
std::vector<const LogParserMetric*> LogParser::getMetrics()
{
   std::vector<const LogParserMetric*> metrics;
   for (int i = 0; i < m_rules.size(); i++)
   {
      const StructArray<LogParserMetric>& ruleMetrics = m_rules.get(i)->getMetrics();
      for(int j = 0; j < ruleMetrics.size(); j++)
         metrics.push_back(ruleMetrics.get(j));
   }
   return metrics;
}

/**
 * Get metric value
 */
bool LogParser::getMetricValue(const TCHAR *name, TCHAR *buffer, size_t size)
{
   for (int i = 0; i < m_rules.size(); i++)
   {
      const StructArray<LogParserMetric>& ruleMetrics = m_rules.get(i)->getMetrics();
      for(int j = 0; j < ruleMetrics.size(); j++)
      {
         LogParserMetric *m = ruleMetrics.get(j);
         if (!_tcsicmp(m->name, name))
         {
            _tcslcpy(buffer, m->value, size);
            return true;
         }
      }
   }
   return false;
}

/**
 * Get metric timestamp
 */
bool LogParser::getMetricTimestamp(const TCHAR *name, time_t *timestamp)
{
   for (int i = 0; i < m_rules.size(); i++)
   {
      const StructArray<LogParserMetric>& ruleMetrics = m_rules.get(i)->getMetrics();
      for(int j = 0; j < ruleMetrics.size(); j++)
      {
         LogParserMetric *m = ruleMetrics.get(j);
         if (!_tcsicmp(m->name, name))
         {
            *timestamp = m->timestamp;
            return true;
         }
      }
   }
   return false;
}
