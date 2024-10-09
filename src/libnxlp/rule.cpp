/*
** NetXMS - Network Management System
** Log Parsing Library
** Copyright (C) 2003-2023 Raden Solutions
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
** File: rule.cpp
**
**/

#include "libnxlp.h"

/**
 * Create empty capture group store
 */
CaptureGroupsStore::CaptureGroupsStore() : m_nameIndex(Ownership::False)
{
   m_numGroups = 0;
}

/**
 * Create capture group store
 */
CaptureGroupsStore::CaptureGroupsStore(const TCHAR *line, int *offsets, int cgcount, const HashMap<uint32_t, String>& groupNames) : m_nameIndex(Ownership::False)
{
   m_numGroups = cgcount - 1;
   for(int i = 1; i < cgcount; i++)
   {
      if (offsets[i * 2] == -1)
         continue;

      int len = offsets[i * 2 + 1] - offsets[i * 2];
      TCHAR *s = m_pool.allocateString(len + 1);
      memcpy(s, &line[offsets[i * 2]], len * sizeof(TCHAR));
      s[len] = 0;

      m_values[i - 1] = s;

      const String *name = groupNames.get(i);
      if (name != nullptr)
      {
         m_nameIndex.set(*name, s);
      }
      else
      {
         TCHAR buffer[32];
         _sntprintf(buffer, 32, _T("group_%d"), i);
         m_nameIndex.set(buffer, s);
      }
   }
}

/**
 * Constructor
 */
LogParserRule::LogParserRule(LogParser *parser, const TCHAR *name, const TCHAR *regexp, bool ignoreCase,
      uint32_t eventCode, const TCHAR *eventName, const TCHAR *eventTag, int repeatInterval, int repeatCount,
      bool resetRepeat, const StructArray<LogParserMetric>& metrics)
   : m_name(name), m_metrics(metrics), m_objectCounters(Ownership::True), m_groupName(Ownership::True)
{
	StringBuffer expandedRegexp;

	m_parser = parser;
	expandMacros(regexp, expandedRegexp);
	m_regexp = MemCopyString(expandedRegexp);
	m_eventCode = eventCode;
	m_eventName = MemCopyString(eventName);
   m_eventTag = MemCopyString(eventTag);
	memset(m_pmatch, 0, sizeof(m_pmatch));
	m_source = nullptr;
	m_level = 0xFFFFFFFF;
	m_idStart = 0;
	m_idEnd = 0xFFFFFFFF;
	m_context = nullptr;
	m_contextAction = 0;
	m_contextToChange = nullptr;
   m_ignoreCase = ignoreCase;
	m_isInverted = false;
	m_breakOnMatch = false;
	m_doNotSaveToDatabase = false;
	m_description = nullptr;
   m_repeatInterval = repeatInterval;
	m_repeatCount = repeatCount;
	m_matchArray = new IntegerArray<time_t>();
	m_resetRepeat = resetRepeat;
	m_checkCount = 0;
	m_matchCount = 0;
	m_agentAction = nullptr;
	m_logName = nullptr;
	m_agentActionArgs = new StringList();

   const char *eptr;
   int eoffset;
   m_preg = _pcre_compile_t(reinterpret_cast<const PCRE_TCHAR*>(m_regexp),
         m_ignoreCase ? PCRE_COMMON_FLAGS | PCRE_CASELESS : PCRE_COMMON_FLAGS, &eptr, &eoffset, nullptr);
   if (m_preg == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 3, _T("Regexp \"%s\" compilation error: %hs at offset %d"), m_regexp, eptr, eoffset);
   }
   else
   {
      updateGroupNames();
   }
}

/**
 * Copy constructor
 */
LogParserRule::LogParserRule(LogParserRule *src, LogParser *parser) : m_name(src->m_name), m_metrics(src->m_metrics), m_objectCounters(Ownership::True), m_groupName(Ownership::True)
{
	m_parser = parser;
	m_regexp = MemCopyString(src->m_regexp);
   m_eventCode = src->m_eventCode;
	m_eventName = MemCopyString(src->m_eventName);
   m_eventTag = MemCopyString(src->m_eventTag);
   memset(m_pmatch, 0, sizeof(m_pmatch));
   m_source = MemCopyString(src->m_source);
	m_level = src->m_level;
	m_idStart = src->m_idStart;
	m_idEnd = src->m_idEnd;
	m_context = MemCopyString(src->m_context);
	m_contextAction = src->m_contextAction;
	m_contextToChange = MemCopyString(src->m_contextToChange);
   m_ignoreCase = src->m_ignoreCase;
	m_isInverted = src->m_isInverted;
	m_breakOnMatch = src->m_breakOnMatch;
	m_description = MemCopyString(src->m_description);
   m_repeatInterval = src->m_repeatInterval;
	m_repeatCount = src->m_repeatCount;
	m_resetRepeat = src->m_resetRepeat;
   if (src->m_matchArray != nullptr)
   {
      m_matchArray = new IntegerArray<time_t>(src->m_matchArray->size(), 16);
      for(int i = 0; i < src->m_matchArray->size(); i++)
         m_matchArray->add(src->m_matchArray->get(i));
   }
   else
   {
      m_matchArray = new IntegerArray<time_t>();
   }
   m_agentAction = MemCopyString(src->m_agentAction);
   m_logName = MemCopyString(src->m_logName);
   m_agentActionArgs = new StringList(src->m_agentActionArgs);
   restoreCounters(*src);

   const char *eptr;
   int eoffset;
   m_preg = _pcre_compile_t(reinterpret_cast<const PCRE_TCHAR*>(m_regexp),
         m_ignoreCase ? PCRE_COMMON_FLAGS | PCRE_CASELESS : PCRE_COMMON_FLAGS, &eptr, &eoffset, nullptr);
   if (m_preg == nullptr)
   {
      nxlog_debug_tag(DEBUG_TAG, 3, _T("Regexp \"%s\" compilation error: %hs at offset %d"), m_regexp, eptr, eoffset);
   }
   else
   {
      updateGroupNames();
   }
}

/**
 * Destructor
 */
LogParserRule::~LogParserRule()
{
	if (m_preg != nullptr)
		_pcre_free_t(m_preg);
	MemFree(m_description);
	MemFree(m_source);
	MemFree(m_regexp);
	MemFree(m_eventName);
	MemFree(m_eventTag);
	MemFree(m_context);
	MemFree(m_contextToChange);
	MemFree(m_agentAction);
	MemFree(m_logName);
	delete m_agentActionArgs;
	delete m_matchArray;
}

/**
 * Update group name to group index map
 */
void LogParserRule::updateGroupNames()
{
   int nameCount;
   _pcre_fullinfo_t(m_preg, nullptr, PCRE_INFO_NAMECOUNT, &nameCount);
   if (nameCount > 0)
   {
      unsigned char *namesTable;
      int nameEntrySize;
      _pcre_fullinfo_t(m_preg, nullptr, PCRE_INFO_NAMETABLE, &namesTable);
      _pcre_fullinfo_t(m_preg, nullptr, PCRE_INFO_NAMEENTRYSIZE, &nameEntrySize);

      unsigned char *entryStart = namesTable;
      for (int i = 0; i < nameCount; i++)
      {
         int numSize = 1;
#ifdef UNICODE
#ifdef UNICODE_UCS2
         uint32_t n = *((uint16_t *)entryStart);
#else /* UNICODE_UCS2 */
         uint32_t n = *((uint32_t *)entryStart);
#endif /* UNICODE_UCS2 */
#else /* UNICODE */
        uint32_t n = (entryStart[0] << 8) | entryStart[1];
        numSize = 2;
#endif /* UNICODE */
         m_groupName.set(n, new String((const TCHAR *)entryStart + numSize));
         entryStart += nameEntrySize * sizeof(TCHAR);
      }
   }
}

/**
 * Match line
 */
bool LogParserRule::matchInternal(bool extMode, const TCHAR *source, uint32_t eventId, uint32_t level, const TCHAR *line,
         StringList *variables, uint64_t recordId, uint32_t objectId, time_t timestamp, const TCHAR *logName, LogParserCallback cb,
         LogParserDataPushCallback cbDataPush, LogParserActionCallback cbAction, void *userData)
{
   incCheckCount(objectId);
   if (extMode)
   {
	   if (m_source != nullptr)
	   {
		   m_parser->trace(7, _T("  matching source \"%s\" against pattern \"%s\""), source, m_source);
		   if (!MatchString(m_source, source, false))
		   {
			   m_parser->trace(7, _T("  source: no match"));
			   return false;
		   }
	   }

      if (m_logName != nullptr)
      {
         m_parser->trace(7, _T("  matching file name \"%s\" against pattern \"%s\""), logName, m_logName);
         if (!MatchString(m_logName, logName, false))
         {
            m_parser->trace(7, _T("  file name: no match"));
            return false;
         }
      }

	   if ((eventId < m_idStart) || (eventId > m_idEnd))
	   {
		   m_parser->trace(7, _T("  event id %u not in range %u - %u"), eventId, m_idStart, m_idEnd);
		   return false;
	   }

	   if (!(m_level & level))
	   {
		   m_parser->trace(7, _T("  severity level 0x%04x not match mask 0x%04x"), level, m_level);
		   return false;
	   }
   }

	if (!isValid())
	{
		m_parser->trace(7, _T("  regexp is invalid: %s"), m_regexp);
		return false;
	}

	if (m_isInverted)
	{
		m_parser->trace(7, _T("  negated matching against regexp %s"), m_regexp);
		int matchCount;
		if ((_pcre_exec_t(m_preg, nullptr, reinterpret_cast<const PCRE_TCHAR*>(line), static_cast<int>(_tcslen(line)), 0, 0, m_pmatch, LOGWATCH_MAX_NUM_CAPTURE_GROUPS * 3) < 0) && matchRepeatCount(&matchCount))
		{
			m_parser->trace(7, _T("  matched"));
			if ((cb != nullptr) && ((m_eventCode != 0) || (m_eventName != nullptr)))
			{
			   CaptureGroupsStore captureGroups;
            LogParserCallbackData data;
            data.captureGroups = &captureGroups;
            data.eventCode = m_eventCode;
            data.eventName = m_eventName;
            data.eventTag = m_eventTag;
            data.facility = eventId;
            data.logName = logName;
            data.logRecordTimestamp = timestamp;
            data.objectId = objectId;
            data.originalText = line;
            data.recordId = recordId;
            data.repeatCount = ((m_repeatCount > 0) && (m_repeatInterval > 0)) ? matchCount : 1;
            data.severity = level;
            data.source = source;
            data.userData = userData;
            data.variables = variables;
            cb(data);
			}

         if ((cbAction != nullptr) && (m_agentAction != nullptr))
            cbAction(m_agentAction, *m_agentActionArgs, userData);

         incMatchCount(objectId);
			return true;
		}
	}
	else
	{
		m_parser->trace(7, _T("  matching against regexp %s"), m_regexp);
		int cgcount = _pcre_exec_t(m_preg, nullptr, reinterpret_cast<const PCRE_TCHAR*>(line), static_cast<int>(_tcslen(line)), 0, 0, m_pmatch, LOGWATCH_MAX_NUM_CAPTURE_GROUPS * 3);

		m_parser->trace(7, _T("  pcre_exec returns %d"), cgcount);
		int matchCount;
		if ((cgcount >= 0) && matchRepeatCount(&matchCount))
		{
			m_parser->trace(7, _T("  matched"));

			if (cgcount == 0)
            cgcount = LOGWATCH_MAX_NUM_CAPTURE_GROUPS;

			CaptureGroupsStore captureGroups(line, m_pmatch, cgcount, m_groupName);

			if (!m_metrics.isEmpty())
			{
			   time_t now = time(nullptr);
            for(int i = 0; i < m_metrics.size(); i++)
            {
               LogParserMetric *m = m_metrics.get(i);

               const TCHAR *v = CHECK_NULL_EX(captureGroups.getByPosition(m->captureGroup - 1));
               _tcslcpy(m->value, v, MAX_DB_STRING);
               m->timestamp = now;
               m_parser->trace(6, _T("Metric \"%s\" set to \"%s\""), m->name, v);

               if (m->push && (cbDataPush != nullptr) && (m->captureGroup > 0) && (captureGroups.size() >= m->captureGroup))
               {
                  m_parser->trace(6, _T("Calling data push callback for metric \"%s\" = \"%s\""), m->name, v);
                  cbDataPush(m->name, v);
               }
            }
			}

			if ((cb != nullptr) && ((m_eventCode != 0) || (m_eventName != nullptr)))
			{
			   LogParserCallbackData data;
			   data.captureGroups = &captureGroups;
			   data.eventCode = m_eventCode;
			   data.eventName = m_eventName;
			   data.eventTag = m_eventTag;
			   data.facility = eventId;
			   data.logName = logName;
			   data.logRecordTimestamp = timestamp;
            data.objectId = objectId;
			   data.originalText = line;
			   data.recordId = recordId;
			   data.repeatCount = ((m_repeatCount > 0) && (m_repeatInterval > 0)) ? matchCount : 1;
			   data.severity = level;
			   data.source = source;
			   data.userData = userData;
			   data.variables = variables;
				cb(data);
            m_parser->trace(8, _T("  callback completed"));
			}

			if ((cbAction != nullptr) && (m_agentAction != nullptr))
			   cbAction(m_agentAction, *m_agentActionArgs, userData);

			incMatchCount(objectId);
			return true;
		}
	}

	m_parser->trace(7, _T("  no match"));
	return false;	// no match
}

/**
 * Expand macros in regexp
 */
void LogParserRule::expandMacros(const TCHAR *regexp, StringBuffer &out)
{
	const TCHAR *curr, *prev;
	TCHAR name[256];

	for(curr = prev = regexp; *curr != 0; curr++)
	{
		if (*curr == _T('@'))
		{
			// Check for escape
			if ((curr != regexp) && (*(curr - 1) == _T('\\')))
			{
				out.append(prev, (size_t)(curr - prev - 1));
				out += _T("@");
			}
			else
			{
				// { should follow @
				if (*(curr + 1) == _T('{'))
				{
					int i;

					out.append(prev, (size_t)(curr - prev));
					curr += 2;
					for(i = 0; (*curr != 0) && (*curr != '}'); i++)
						name[i] = *curr++;
					name[i] = 0;
					out += m_parser->getMacro(name);
				}
				else
				{
					out.append(prev, (size_t)(curr - prev + 1));
				}
			}
			prev = curr + 1;
		}
	}
	out.append(prev, (size_t)(curr - prev));
}

/**
 * Match repeat count
 */
bool LogParserRule::matchRepeatCount(int *matchCount)
{
   if ((m_repeatCount == 0) || (m_repeatInterval == 0))
      return true;

   // remove expired matches
   time_t now = time(nullptr);
   for(int i = 0; i < m_matchArray->size(); i++)
   {
      if (m_matchArray->get(i) >= (now - m_repeatInterval))
         break;
      m_matchArray->remove(i);
      i--;
   }

   m_matchArray->add(now);
   bool match = m_matchArray->size() >= m_repeatCount;
   *matchCount = m_matchArray->size();
   if (m_resetRepeat && match)
      m_matchArray->clear();
   return match;
}

/**
 * Increment check count
 */
void LogParserRule::incCheckCount(uint32_t objectId)
{
   m_checkCount++;
   if (objectId == 0)
      return;
   ObjectRuleStats *s = m_objectCounters.get(objectId);
   if (s == nullptr)
   {
      s = new ObjectRuleStats();
      m_objectCounters.set(objectId, s);
   }
   s->checkCount++;
}

/**
 * Increment match count
 */
void LogParserRule::incMatchCount(uint32_t objectId)
{
   m_matchCount++;
   if (objectId == 0)
      return;
   ObjectRuleStats *s = m_objectCounters.get(objectId);
   if (s == nullptr)
   {
      s = new ObjectRuleStats();
      m_objectCounters.set(objectId, s);
   }
   s->matchCount++;
}

/**
 * Get check count for specfic object
 */
int LogParserRule::getCheckCount(uint32_t objectId) const
{
   if (objectId == 0)
      return m_checkCount;
   ObjectRuleStats *s = m_objectCounters.get(objectId);
   return (s != nullptr) ? s->checkCount : 0;
}

/**
 * Get match count for specfic object
 */
int LogParserRule::getMatchCount(uint32_t objectId) const
{
   if (objectId == 0)
      return m_matchCount;
   ObjectRuleStats *s = m_objectCounters.get(objectId);
   return (s != nullptr) ? s->matchCount : 0;
}

/**
 * Restore counters from previous rule version
 */
void LogParserRule::restoreCounters(const LogParserRule& rule)
{
   m_checkCount = rule.m_checkCount;
   m_matchCount = rule.m_matchCount;
   rule.m_objectCounters.forEach(
      [this] (const uint32_t& key, ObjectRuleStats *src) -> EnumerationCallbackResult
      {
         ObjectRuleStats *dst = new ObjectRuleStats;
         dst->checkCount = src->checkCount;
         dst->matchCount = src->matchCount;
         this->m_objectCounters.set(key, dst);
         return _CONTINUE;
      });
}
