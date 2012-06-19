/* 
** NetXMS - Network Management System
** Log Parsing Library
** Copyright (C) 2003-2012 Victor Kirhenshtein
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

#if HAVE_ALLOCA_H
#include <alloca.h>
#endif


//
// Constructor
//

LogParserRule::LogParserRule(LogParser *parser, const char *regexp, DWORD eventCode, const char *eventName,
									  int numParams, const TCHAR *source, DWORD level, DWORD idStart, DWORD idEnd)
{
	String expandedRegexp;

	m_parser = parser;
	expandMacros(regexp, expandedRegexp);
	m_regexp = _tcsdup(expandedRegexp);
	m_isValid = (_tregcomp(&m_preg, expandedRegexp, REG_EXTENDED | REG_ICASE) == 0);
	m_eventCode = eventCode;
	m_eventName = (eventName != NULL) ? strdup(eventName) : NULL;
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
}


//
// Destructor
//

LogParserRule::~LogParserRule()
{
	if (m_isValid)
		regfree(&m_preg);
	safe_free(m_pmatch);
	safe_free(m_description);
	safe_free(m_source);
	safe_free(m_regexp);
	safe_free(m_eventName);
}


//
// Match line
//

bool LogParserRule::match(const char *line, LogParserCallback cb, DWORD objectId, void *userArg)
{
	if (!m_isValid)
	{
		m_parser->trace(6, _T("  regexp is invalid: %s"), m_regexp);
		return false;
	}

	if (m_isInverted)
	{
		m_parser->trace(6, _T("  negated matching against regexp %s"), m_regexp);
		if (_tregexec(&m_preg, line, 0, NULL, 0) != 0)
		{
			m_parser->trace(6, _T("  matched"));
			if ((cb != NULL) && ((m_eventCode != 0) || (m_eventName != NULL)))
				cb(m_eventCode, m_eventName, line, 0, NULL, objectId, userArg);
			return true;
		}
	}
	else
	{
		m_parser->trace(6, _T("  matching against regexp %s"), m_regexp);
		if (_tregexec(&m_preg, line, (m_numParams > 0) ? m_numParams + 1 : 0, m_pmatch, 0) == 0)
		{
			m_parser->trace(6, _T("  matched"));
			if ((cb != NULL) && ((m_eventCode != 0) || (m_eventName != NULL)))
			{
				char **params = NULL;
				int i, len;

				if (m_numParams > 0)
				{
					params = (char **)alloca(sizeof(char *) * m_numParams);
					for(i = 0; i < m_numParams; i++)
					{
						if (m_pmatch[i + 1].rm_so != -1)
						{
							len = m_pmatch[i + 1].rm_eo - m_pmatch[i + 1].rm_so;
							params[i] = (char *)malloc(len + 1);
							memcpy(params[i], &line[m_pmatch[i + 1].rm_so], len);
							params[i][len] = 0;
						}
						else
						{
							params[i] = strdup("");
						}
					}
				}

				cb(m_eventCode, m_eventName, line, m_numParams, params, objectId, userArg);
				
				for(i = 0; i < m_numParams; i++)
					safe_free(params[i]);
			}		
			return true;
		}
	}

	m_parser->trace(6, _T("  no match"));
	return false;	// no match
}


//
// Match event
//

bool LogParserRule::matchEx(const TCHAR *source, DWORD eventId, DWORD level,
								  const char *line, LogParserCallback cb, DWORD objectId, void *userArg)
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

	return match(line, cb, objectId, userArg);
}


//
// Expand macros in regexp
//

void LogParserRule::expandMacros(const char *regexp, String &out)
{
	const char *curr, *prev;
	char name[256];

	for(curr = prev = regexp; *curr != 0; curr++)
	{
		if (*curr == '@')
		{
			// Check for escape
			if ((curr != regexp) && (*(curr - 1) == '\\'))
			{
				out.addString(prev, (DWORD)(curr - prev - 1));
				out += _T("@");
			}
			else
			{
				// { should follow @
				if (*(curr + 1) == '{')
				{
					int i;

					out.addString(prev, (DWORD)(curr - prev));
					curr += 2;
					for(i = 0; (*curr != 0) && (*curr != '}'); i++)
						name[i] = *curr++;
					name[i] = 0;
					out += m_parser->getMacro(name);
				}
				else
				{
					out.addString(prev, (DWORD)(curr - prev + 1));
				}
			}
			prev = curr + 1;
		}
	}
	out.addString(prev, (DWORD)(curr - prev));
}
