/* 
** NetXMS - Network Management System
** Log Parsing Library
** Copyright (C) 2008 Victor Kirhenshtein
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
** File: rule.cpp
**
**/

#include "libnxlp.h"

#ifdef _WIN32
#include <malloc.h>
#else
#include <alloca.h>
#endif


//
// Constructor
//

LogParserRule::LogParserRule(LogParser *parser,
									  const char *regexp, DWORD event, int numParams,
									  const TCHAR *source, DWORD level, 
									  DWORD idStart, DWORD idEnd)
{
	String expandedRegexp;

	m_parser = parser;
	ExpandMacros(regexp, expandedRegexp);
	m_isValid = (regcomp(&m_preg, expandedRegexp, REG_EXTENDED | REG_ICASE) == 0);
	m_event = event;
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
}


//
// Match line
//

BOOL LogParserRule::Match(const char *line, LOG_PARSER_CALLBACK cb, DWORD objectId, void *userArg)
{
	if (!m_isValid)
		return FALSE;

	if (m_isInverted)
	{
		if (regexec(&m_preg, line, 0, NULL, 0) != 0)
		{
			if ((cb != NULL) && (m_event != 0))
				cb(m_event, line, 0, NULL, objectId, userArg);
			return TRUE;
		}
	}
	else
	{
		if (regexec(&m_preg, line, (m_numParams > 0) ? m_numParams + 1 : 0, m_pmatch, 0) == 0)
		{
			if ((cb != NULL) && (m_event != 0))
			{
				char **params;
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

				cb(m_event, line, m_numParams, params, objectId, userArg);
				
				for(i = 0; i < m_numParams; i++)
					safe_free(params[i]);
			}		
			return TRUE;
		}
	}

	return FALSE;	// no match
}


//
// Expand macros in regexp
//

void LogParserRule::ExpandMacros(const char *regexp, String &out)
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
				out.AddString(prev, curr - prev - 1);
				out += _T("@");
			}
			else
			{
				// { should follow @
				if (*(curr + 1) == '{')
				{
					int i;

					out.AddString(prev, curr - prev);
					curr += 2;
					for(i = 0; (*curr != 0) && (*curr != '}'); i++)
						name[i] = *curr++;
					name[i] = 0;
					out += m_parser->GetMacro(name);
				}
				else
				{
					out.AddString(prev, curr - prev + 1);
				}
			}
			prev = curr + 1;
		}
	}
	out.AddString(prev, curr - prev);
}
