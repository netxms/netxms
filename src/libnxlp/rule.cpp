/*
** NetXMS - Network Management System
** Log Parsing Library
** Copyright (C) 2003-2018 Raden Solutions
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
LogParserRule::LogParserRule(LogParser *parser, const TCHAR *name, const TCHAR *regexp, UINT32 eventCode, const TCHAR *eventName,
									  int repeatInterval, int repeatCount, bool resetRepeat, const TCHAR *source,
									  UINT32 level, UINT32 idStart, UINT32 idEnd)
{
	String expandedRegexp;

	m_parser = parser;
	m_name = MemCopyString(CHECK_NULL_EX(name));
	expandMacros(regexp, expandedRegexp);
	m_regexp = MemCopyString(expandedRegexp);
	m_isValid = (_tregcomp(&m_preg, expandedRegexp, REG_EXTENDED | REG_ICASE) == 0);
	m_eventCode = eventCode;
	m_eventName = MemCopyString(eventName);
	m_pmatch = MemAllocArray<regmatch_t>(MAX_PARAM_COUNT);
	m_source = MemCopyString(source);
	m_level = level;
	m_idStart = idStart;
	m_idEnd = idEnd;
	m_context = NULL;
	m_contextAction = 0;
	m_contextToChange = NULL;
	m_isInverted = FALSE;
	m_breakOnMatch = FALSE;
	m_description = NULL;
   m_repeatInterval = repeatInterval;
	m_repeatCount = repeatCount;
	m_matchArray = new IntegerArray<time_t>();
	m_resetRepeat = resetRepeat;
	m_checkCount = 0;
	m_matchCount = 0;
	m_agentAction = NULL;
	m_agentActionArgs = new StringList();
   m_objectCounters = new HashMap<UINT32, ObjectRuleStats>(true);
}

/**
 * Copy constructor
 */
LogParserRule::LogParserRule(LogParserRule *src, LogParser *parser)
{
	m_parser = parser;
	m_name = MemCopyString(src->m_name);
	m_regexp = MemCopyString(src->m_regexp);
	m_isValid = (_tregcomp(&m_preg, m_regexp, REG_EXTENDED | REG_ICASE) == 0);
	m_eventCode = src->m_eventCode;
	m_eventName = MemCopyString(src->m_eventName);
   m_pmatch = MemAllocArray<regmatch_t>(MAX_PARAM_COUNT);
   m_source = MemCopyString(src->m_source);
	m_level = src->m_level;
	m_idStart = src->m_idStart;
	m_idEnd = src->m_idEnd;
	m_context = MemCopyString(src->m_context);
	m_contextAction = src->m_contextAction;
	m_contextToChange = MemCopyString(src->m_contextToChange);
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
   m_agentActionArgs = new StringList(src->m_agentActionArgs);
   m_objectCounters = new HashMap<UINT32, ObjectRuleStats>(true);
   restoreCounters(src);
}

/**
 * Destructor
 */
LogParserRule::~LogParserRule()
{
   MemFree(m_name);
	if (m_isValid)
		regfree(&m_preg);
	MemFree(m_pmatch);
	MemFree(m_description);
	MemFree(m_source);
	MemFree(m_regexp);
	MemFree(m_eventName);
	MemFree(m_context);
	MemFree(m_contextToChange);
	MemFree(m_agentAction);
	delete m_agentActionArgs;
	delete m_matchArray;
	delete m_objectCounters;
}

/**
 * Match line
 */
bool LogParserRule::matchInternal(bool extMode, const TCHAR *source, UINT32 eventId, UINT32 level, const TCHAR *line,
                                  StringList *variables, UINT64 recordId, UINT32 objectId, LogParserCallback cb, void *userArg)
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

	if (!m_isValid)
	{
		m_parser->trace(6, _T("  regexp is invalid: %s"), m_regexp);
		return false;
	}

	if (m_isInverted)
	{
		m_parser->trace(6, _T("  negated matching against regexp %s"), m_regexp);
		if ((_tregexec(&m_preg, line, 0, NULL, 0) != 0) && matchRepeatCount())
		{
			m_parser->trace(6, _T("  matched"));
			if ((cb != NULL) && ((m_eventCode != 0) || (m_eventName != NULL)))
				cb(m_eventCode, m_eventName, line, source, eventId, level, NULL, variables, recordId, objectId, 
               ((m_repeatCount > 0) && (m_repeatInterval > 0)) ? m_matchArray->size() : 1, userArg, m_agentAction, m_agentActionArgs);
			incMatchCount(objectId);
			return true;
		}
	}
	else
	{
		m_parser->trace(6, _T("  matching against regexp %s"), m_regexp);
		if ((_tregexec(&m_preg, line, MAX_PARAM_COUNT, m_pmatch, 0) == 0) && matchRepeatCount())
		{
			m_parser->trace(6, _T("  matched"));
			if ((cb != NULL) && ((m_eventCode != 0) || (m_eventName != NULL)))
			{
            StringList captureGroups;
				for(int i = 1; i < MAX_PARAM_COUNT; i++)
				{
               if (m_pmatch[i].rm_so == -1)
                  continue;

					int len = m_pmatch[i].rm_eo - m_pmatch[i].rm_so;
					TCHAR *s = (TCHAR *)MemAlloc((len + 1) * sizeof(TCHAR));
					memcpy(s, &line[m_pmatch[i].rm_so], len * sizeof(TCHAR));
					s[len] = 0;
               captureGroups.addPreallocated(s);
				}

				cb(m_eventCode, m_eventName, line, source, eventId, level, &captureGroups, variables, recordId, objectId,
               ((m_repeatCount > 0) && (m_repeatInterval > 0)) ? m_matchArray->size() : 1, userArg, m_agentAction, m_agentActionArgs);
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
bool LogParserRule::match(const TCHAR *line, UINT32 objectId, LogParserCallback cb, void *userArg)
{
   return matchInternal(false, NULL, 0, 0, line, NULL, 0, objectId, cb, userArg);
}

/**
 * Match event
 */
bool LogParserRule::matchEx(const TCHAR *source, UINT32 eventId, UINT32 level, const TCHAR *line, StringList *variables,
                            UINT64 recordId, UINT32 objectId, LogParserCallback cb, void *userArg)
{
   return matchInternal(true, source, eventId, level, line, variables, recordId, objectId, cb, userArg);
}

/**
 * Expand macros in regexp
 */
void LogParserRule::expandMacros(const TCHAR *regexp, String &out)
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
   time_t now = time(NULL);
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
void LogParserRule::incCheckCount(UINT32 objectId)
{
   m_checkCount++;
   if (objectId == 0)
      return;
   ObjectRuleStats *s = m_objectCounters->get(objectId);
   if (s == NULL)
   {
      s = new ObjectRuleStats();
      m_objectCounters->set(objectId, s);
   }
   s->checkCount++;
}

/**
 * Increment match count
 */
void LogParserRule::incMatchCount(UINT32 objectId)
{
   m_matchCount++;
   if (objectId == 0)
      return;
   ObjectRuleStats *s = m_objectCounters->get(objectId);
   if (s == NULL)
   {
      s = new ObjectRuleStats();
      m_objectCounters->set(objectId, s);
   }
   s->matchCount++;
}

/**
 * Get check count for specfic object
 */
int LogParserRule::getCheckCount(UINT32 objectId) const
{
   if (objectId == 0)
      return m_checkCount;
   ObjectRuleStats *s = m_objectCounters->get(objectId);
   return (s != NULL) ? s->checkCount : 0;
}

/**
 * Get match count for specfic object
 */
int LogParserRule::getMatchCount(UINT32 objectId) const
{
   if (objectId == 0)
      return m_matchCount;
   ObjectRuleStats *s = m_objectCounters->get(objectId);
   return (s != NULL) ? s->matchCount : 0;
}

/**
 * Callback for copying object counters
 */
static EnumerationCallbackResult RestoreCountersCallback(const void *key, const void *value, void *arg)
{
   ObjectRuleStats *s = new ObjectRuleStats;
   s->checkCount = ((const ObjectRuleStats *)value)->checkCount;
   s->matchCount = ((const ObjectRuleStats *)value)->matchCount;
   ((HashMap<UINT32, ObjectRuleStats> *)arg)->set(*((UINT32 *)key), s);
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
