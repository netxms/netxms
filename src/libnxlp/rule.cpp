/*
** NetXMS - Network Management System
** Log Parsing Library
** Copyright (C) 2003-2020 Raden Solutions
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

#define MAX_PARAM_COUNT 127

/**
 * Constructor
 */
LogParserRule::LogParserRule(LogParser *parser, const TCHAR *name, const TCHAR *regexp, bool ignoreCase,
      UINT32 eventCode, const TCHAR *eventName, const TCHAR *eventTag, int repeatInterval, int repeatCount,
      bool resetRepeat, const TCHAR *source, UINT32 level, UINT32 idStart, UINT32 idEnd)
{
	StringBuffer expandedRegexp;

	m_parser = parser;
	m_name = MemCopyString(CHECK_NULL_EX(name));
	expandMacros(regexp, expandedRegexp);
	m_regexp = MemCopyString(expandedRegexp);
	m_eventCode = eventCode;
	m_eventName = MemCopyString(eventName);
   m_eventTag = MemCopyString(eventTag);
	m_pmatch = MemAllocArray<int>(MAX_PARAM_COUNT * 3);
	m_source = MemCopyString(source);
	m_level = level;
	m_idStart = idStart;
	m_idEnd = idEnd;
	m_context = NULL;
	m_contextAction = 0;
	m_contextToChange = NULL;
   m_ignoreCase = ignoreCase;
	m_isInverted = false;
	m_breakOnMatch = false;
	m_doNotSaveToDatabase = false;
	m_description = NULL;
   m_repeatInterval = repeatInterval;
	m_repeatCount = repeatCount;
	m_matchArray = new IntegerArray<time_t>();
	m_resetRepeat = resetRepeat;
	m_checkCount = 0;
	m_matchCount = 0;
	m_agentAction = NULL;
	m_logName = NULL;
	m_agentActionArgs = new StringList();
   m_objectCounters = new HashMap<uint32_t, ObjectRuleStats>(Ownership::True);

   const char *eptr;
   int eoffset;
   m_preg = _pcre_compile_t(reinterpret_cast<const PCRE_TCHAR*>(m_regexp), 
         m_ignoreCase ? PCRE_COMMON_FLAGS | PCRE_CASELESS : PCRE_COMMON_FLAGS, &eptr, &eoffset, NULL);
   if (m_preg == NULL)
   {
      nxlog_debug_tag(DEBUG_TAG, 3, _T("Regexp \"%s\" compilation error: %hs at offset %d"), m_regexp, eptr, eoffset);
   }
}

/**
 * Copy constructor
 */
LogParserRule::LogParserRule(LogParserRule *src, LogParser *parser)
{
	m_parser = parser;
	m_name = MemCopyString(src->m_name);
	m_regexp = MemCopyString(src->m_regexp);
   m_eventCode = src->m_eventCode;
	m_eventName = MemCopyString(src->m_eventName);
   m_eventTag = MemCopyString(src->m_eventTag);
   m_pmatch = MemAllocArray<int>(MAX_PARAM_COUNT * 3);
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
   if (src->m_matchArray != NULL)
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
   m_objectCounters = new HashMap<uint32_t, ObjectRuleStats>(Ownership::True);
   restoreCounters(src);

   const char *eptr;
   int eoffset;
   m_preg = _pcre_compile_t(reinterpret_cast<const PCRE_TCHAR*>(m_regexp),
         m_ignoreCase ? PCRE_COMMON_FLAGS | PCRE_CASELESS : PCRE_COMMON_FLAGS, &eptr, &eoffset, NULL);
   if (m_preg == NULL)
   {
      nxlog_debug_tag(DEBUG_TAG, 3, _T("Regexp \"%s\" compilation error: %hs at offset %d"), m_regexp, eptr, eoffset);
   }
}

/**
 * Destructor
 */
LogParserRule::~LogParserRule()
{
   MemFree(m_name);
	if (m_preg != NULL)
		_pcre_free_t(m_preg);
	MemFree(m_pmatch);
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
	delete m_objectCounters;
}

/**
 * Match line
 */
bool LogParserRule::matchInternal(bool extMode, const TCHAR *source, UINT32 eventId, UINT32 level, const TCHAR *line,
         StringList *variables, UINT64 recordId, UINT32 objectId, time_t timestamp, LogParserCallback cb, void *context,
         const TCHAR *logName)
{
   incCheckCount(objectId);
   if (extMode)
   {
	   if (m_source != NULL)
	   {
		   m_parser->trace(6, _T("  matching source \"%s\" against pattern \"%s\""), source, m_source);
		   if (!MatchString(m_source, source, false))
		   {
			   m_parser->trace(6, _T("  source: no match"));
			   return false;
		   }
	   }

      if (m_logName != NULL)
      {
         m_parser->trace(6, _T("  matching file name \"%s\" against pattern \"%s\""), logName, m_logName);
         if (!MatchString(m_logName, logName, false))
         {
            m_parser->trace(6, _T("  file name: no match"));
            return false;
         }
      }

	   if ((eventId < m_idStart) || (eventId > m_idEnd))
	   {
		   m_parser->trace(6, _T("  event id 0x%08x not in range 0x%08x - 0x%08x"), eventId, m_idStart, m_idEnd);
		   return false;
	   }

	   if (!(m_level & level))
	   {
		   m_parser->trace(6, _T("  severity level 0x%04x not match mask 0x%04x"), level, m_level);
		   return false;
	   }
   }

	if (!isValid())
	{
		m_parser->trace(6, _T("  regexp is invalid: %s"), m_regexp);
		return false;
	}

	if (m_isInverted)
	{
		m_parser->trace(6, _T("  negated matching against regexp %s"), m_regexp);
		if ((_pcre_exec_t(m_preg, NULL, reinterpret_cast<const PCRE_TCHAR*>(line), static_cast<int>(_tcslen(line)), 0, 0, m_pmatch, MAX_PARAM_COUNT * 3) < 0) && matchRepeatCount())
		{
			m_parser->trace(6, _T("  matched"));
			if ((cb != NULL) && ((m_eventCode != 0) || (m_eventName != NULL)))
				cb(m_eventCode, m_eventName, m_eventTag, line, source, eventId, level, NULL, variables, recordId, objectId,
               ((m_repeatCount > 0) && (m_repeatInterval > 0)) ? m_matchArray->size() : 1, timestamp, m_agentAction, m_agentActionArgs, context);
			incMatchCount(objectId);
			return true;
		}
	}
	else
	{
		m_parser->trace(6, _T("  matching against regexp %s"), m_regexp);
		int cgcount = _pcre_exec_t(m_preg, NULL, reinterpret_cast<const PCRE_TCHAR*>(line), static_cast<int>(_tcslen(line)), 0, 0, m_pmatch, MAX_PARAM_COUNT * 3);
      m_parser->trace(7, _T("  pcre_exec returns %d"), cgcount);
		if ((cgcount >= 0) && matchRepeatCount())
		{
			m_parser->trace(6, _T("  matched"));
			if ((cb != NULL) && ((m_eventCode != 0) || (m_eventName != NULL)))
			{
			   if (cgcount == 0)
			      cgcount = MAX_PARAM_COUNT;
            StringList captureGroups;
				for(int i = 1; i < cgcount; i++)
				{
               if (m_pmatch[i * 2] == -1)
                  continue;

					int len = m_pmatch[i * 2 + 1] - m_pmatch[i * 2];
					TCHAR *s = MemAllocString(len + 1);
					memcpy(s, &line[m_pmatch[i * 2]], len * sizeof(TCHAR));
					s[len] = 0;
               captureGroups.addPreallocated(s);
				}

				cb(m_eventCode, m_eventName, m_eventTag, line, source, eventId, level, &captureGroups, variables, recordId, objectId,
               ((m_repeatCount > 0) && (m_repeatInterval > 0)) ? m_matchArray->size() : 1, timestamp, m_agentAction, m_agentActionArgs, context);
            m_parser->trace(8, _T("  callback completed"));
         }
         incMatchCount(objectId);
			return true;
		}
	}

	m_parser->trace(6, _T("  no match"));
	return false;	// no match
}

/**
 * Match line
 */
bool LogParserRule::match(const TCHAR *line, UINT32 objectId, LogParserCallback cb, void *context)
{
   return matchInternal(false, nullptr, 0, 0, line, nullptr, 0, objectId, 0, cb, context, nullptr);
}

/**
 * Match event
 */
bool LogParserRule::matchEx(const TCHAR *source, UINT32 eventId, UINT32 level, const TCHAR *line, StringList *variables,
                            UINT64 recordId, UINT32 objectId, time_t timestamp, LogParserCallback cb, void *context, const TCHAR *fileName)
{
   return matchInternal(true, source, eventId, level, line, variables, recordId, objectId, timestamp, cb, context, fileName);
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
bool LogParserRule::matchRepeatCount()
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
   ObjectRuleStats *s = m_objectCounters->get(objectId);
   if (s == nullptr)
   {
      s = new ObjectRuleStats();
      m_objectCounters->set(objectId, s);
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
   ObjectRuleStats *s = m_objectCounters->get(objectId);
   if (s == nullptr)
   {
      s = new ObjectRuleStats();
      m_objectCounters->set(objectId, s);
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
   ObjectRuleStats *s = m_objectCounters->get(objectId);
   return (s != nullptr) ? s->checkCount : 0;
}

/**
 * Get match count for specfic object
 */
int LogParserRule::getMatchCount(uint32_t objectId) const
{
   if (objectId == 0)
      return m_matchCount;
   ObjectRuleStats *s = m_objectCounters->get(objectId);
   return (s != nullptr) ? s->matchCount : 0;
}

/**
 * Callback for copying object counters
 */
static EnumerationCallbackResult RestoreCountersCallback(const uint32_t& key, ObjectRuleStats *src, HashMap<uint32_t, ObjectRuleStats> *counters)
{
   ObjectRuleStats *dst = new ObjectRuleStats;
   dst->checkCount = src->checkCount;
   dst->matchCount = src->matchCount;
   counters->set(key, dst);
   return _CONTINUE;
}

/**
 * Restore counters from previous rule version
 */
void LogParserRule::restoreCounters(const LogParserRule *rule)
{
   m_checkCount = rule->m_checkCount;
   m_matchCount = rule->m_matchCount;
   rule->m_objectCounters->forEach(RestoreCountersCallback, m_objectCounters);
}
