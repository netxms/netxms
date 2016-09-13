/*
** NetXMS - Network Management System
** Log Parsing Library
** Copyright (C) 2003-2016 Raden Solutions
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
 * Constructor
 */
LogParserRule::LogParserRule(LogParser *parser, const TCHAR *regexp, UINT32 eventCode, const TCHAR *eventName,
									  int numParams, int repeatInterval, int repeatCount, bool resetRepeat, const TCHAR *source,
									  UINT32 level, UINT32 idStart, UINT32 idEnd)
{
	String expandedRegexp;

	m_parser = parser;
	expandMacros(regexp, expandedRegexp);
	m_regexp = _tcsdup(expandedRegexp);
	m_isValid = (_tregcomp(&m_preg, expandedRegexp, REG_EXTENDED | REG_ICASE) == 0);
	m_eventCode = eventCode;
	m_eventName = (eventName != NULL) ? _tcsdup(eventName) : NULL;
	m_numParams = numParams;
	m_pmatch = (numParams > 0) ? (regmatch_t *)malloc(sizeof(regmatch_t) * (numParams + 1)) : NULL;
	m_source = (source != NULL) ? _tcsdup(source) : NULL;
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
}

/**
 * Copy constructor
 */
LogParserRule::LogParserRule(LogParserRule *src, LogParser *parser)
{
	m_parser = parser;
	m_regexp = _tcsdup(src->m_regexp);
	m_isValid = (_tregcomp(&m_preg, m_regexp, REG_EXTENDED | REG_ICASE) == 0);
	m_eventCode = src->m_eventCode;
	m_eventName = (src->m_eventName != NULL) ? _tcsdup(src->m_eventName) : NULL;
	m_numParams = src->m_numParams;
	m_pmatch = (m_numParams > 0) ? (regmatch_t *)malloc(sizeof(regmatch_t) * (m_numParams + 1)) : NULL;
	m_source = (src->m_source != NULL) ? _tcsdup(src->m_source) : NULL;
	m_level = src->m_level;
	m_idStart = src->m_idStart;
	m_idEnd = src->m_idEnd;
	m_context = (src->m_context != NULL) ? _tcsdup(src->m_context) : NULL;
	m_contextAction = src->m_contextAction;
	m_contextToChange = (src->m_contextToChange != NULL) ? _tcsdup(src->m_contextToChange) : NULL;
	m_isInverted = src->m_isInverted;
	m_breakOnMatch = src->m_breakOnMatch;
	m_description = (src->m_description != NULL) ? _tcsdup(src->m_description) : NULL;
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
   m_checkCount = src->m_checkCount;
   m_matchCount = src->m_matchCount;
}

/**
 * Destructor
 */
LogParserRule::~LogParserRule()
{
	if (m_isValid)
		regfree(&m_preg);
	safe_free(m_pmatch);
	safe_free(m_description);
	safe_free(m_source);
	safe_free(m_regexp);
	safe_free(m_eventName);
	safe_free(m_context);
	safe_free(m_contextToChange);
	delete m_matchArray;
}

/**
 * Match line
 */
bool LogParserRule::matchInternal(bool extMode, const TCHAR *source, UINT32 eventId, UINT32 level,
								          const TCHAR *line, LogParserCallback cb, UINT32 objectId, void *userArg)
{
   m_checkCount++;
   if (extMode)
   {
	   if (m_source != NULL)
	   {
		   m_parser->trace(6, _T("  matching source \"%s\" against pattern \"%s\""), source, m_source);
		   if (!MatchString(m_source, source, FALSE))
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
				cb(m_eventCode, m_eventName, line, source, eventId, level, 0, NULL, objectId, 
               ((m_repeatCount > 0) && (m_repeatInterval > 0)) ? m_matchArray->size() : 1, userArg);
			m_matchCount++;
			return true;
		}
	}
	else
	{
		m_parser->trace(6, _T("  matching against regexp %s"), m_regexp);
		if ((_tregexec(&m_preg, line, (m_numParams > 0) ? m_numParams + 1 : 0, m_pmatch, 0) == 0) && matchRepeatCount())
		{
			m_parser->trace(6, _T("  matched"));
			if ((cb != NULL) && ((m_eventCode != 0) || (m_eventName != NULL)))
			{
				TCHAR **params = NULL;
				int i, len;

				if (m_numParams > 0)
				{
#if HAVE_ALLOCA
					params = (TCHAR **)alloca(sizeof(TCHAR *) * m_numParams);
#else
					params = (TCHAR **)malloc(sizeof(TCHAR *) * m_numParams);
#endif
					for(i = 0; i < m_numParams; i++)
					{
						if (m_pmatch[i + 1].rm_so != -1)
						{
							len = m_pmatch[i + 1].rm_eo - m_pmatch[i + 1].rm_so;
							params[i] = (TCHAR *)malloc((len + 1) * sizeof(TCHAR));
							memcpy(params[i], &line[m_pmatch[i + 1].rm_so], len * sizeof(TCHAR));
							params[i][len] = 0;
						}
						else
						{
							params[i] = _tcsdup(_T(""));
						}
					}
				}

				cb(m_eventCode, m_eventName, line, source, eventId, level, m_numParams, params, objectId, 
               ((m_repeatCount > 0) && (m_repeatInterval > 0)) ? m_matchArray->size() : 1, userArg);

				for(i = 0; i < m_numParams; i++)
					free(params[i]);
#if !HAVE_ALLOCA
            free(params);
#endif

            m_matchCount++;
			}
			return true;
		}
	}

	m_parser->trace(6, _T("  no match"));
	return false;	// no match
}

/**
 * Match line
 */
bool LogParserRule::match(const TCHAR *line, LogParserCallback cb, UINT32 objectId, void *userArg)
{
   return matchInternal(false, NULL, 0, 0, line, cb, objectId, userArg);
}

/**
 * Match event
 */
bool LogParserRule::matchEx(const TCHAR *source, UINT32 eventId, UINT32 level,
								    const TCHAR *line, LogParserCallback cb, UINT32 objectId, void *userArg)
{
   return matchInternal(true, source, eventId, level, line, cb, objectId, userArg);
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
